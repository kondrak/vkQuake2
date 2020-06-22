/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2018-2019 Krzysztof Kondrak

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/*
** VK_IMP.C
**
** This file contains ALL Win32 specific stuff having to do with the
** Vulkan refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** Vkimp_EndFrame
** Vkimp_Init
** Vkimp_Shutdown
**
*/
#include <assert.h>
#include <windows.h>
#include "../ref_vk/vk_local.h"
#include "vk_win.h"
#include "winquake.h"

vkwstate_t vkw_state;
VkSurfaceFullScreenExclusiveWin32InfoEXT g_surface_full_screen_exclusive_win32_info;

extern cvar_t *vid_fullscreen;
extern cvar_t *vid_ref;

/*
** VID_CreateWindow
*/
#define	WINDOW_CLASS_NAME	"Quake 2"

qboolean VID_CreateWindow( int width, int height, qboolean fullscreen )
{
	WNDCLASS		wc;
	RECT			r;
	cvar_t			*vid_xpos, *vid_ypos;
	int				stylebits;
	int				x, y, w, h;
	int				exstyle;

	/* Register the frame class */
	wc.style         = 0;
	wc.lpfnWndProc   = (WNDPROC)vkw_state.wndproc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = vkw_state.hInstance;
	wc.hIcon         = 0;
	wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wc.hbrBackground = (void *)COLOR_GRAYTEXT;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = WINDOW_CLASS_NAME;

	if (!RegisterClass (&wc) )
		ri.Sys_Error (ERR_FATAL, "Couldn't register window class");

	if (fullscreen)
	{
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP|WS_VISIBLE;
	}
	else
	{
		exstyle = 0;
		stylebits = WINDOW_STYLE;
	}

	r.left = 0;
	r.top = 0;
	r.right  = width;
	r.bottom = height;

	AdjustWindowRect (&r, stylebits, FALSE);

	w = r.right - r.left;
	h = r.bottom - r.top;

	vid_xpos = ri.Cvar_Get ("vid_xpos", "0", 0);
	vid_ypos = ri.Cvar_Get ("vid_ypos", "0", 0);
	x = vid_xpos->value;
	y = vid_ypos->value;

	vkw_state.hWnd = CreateWindowEx (
		 exstyle, 
		 WINDOW_CLASS_NAME,
		 "Quake 2 (Vulkan) "CPUSTRING,
		 stylebits,
		 x, y, w, h,
		 NULL,
		 NULL,
		 vkw_state.hInstance,
		 NULL);

	if (!vkw_state.hWnd)
		ri.Sys_Error (ERR_FATAL, "Couldn't create window");

	memset( &vkw_state.monInfo, 0, sizeof(MONITORINFOEX) );
	vkw_state.monInfo.cbSize = sizeof(MONITORINFOEX);
	vkw_state.monitor = MonitorFromWindow(vkw_state.hWnd, MONITOR_DEFAULTTOPRIMARY);
	GetMonitorInfo( vkw_state.monitor, (LPMONITORINFO)&vkw_state.monInfo );

	if (fullscreen)
	{
		DEVMODE dm;
		memset(&dm, 0, sizeof(dm));

		dm.dmSize = sizeof(dm);
		dm.dmPelsWidth = width;
		dm.dmPelsHeight = height;
		dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

		if (vk_bitdepth->value != 0)
		{
			dm.dmBitsPerPel = vk_bitdepth->value;
			dm.dmFields |= DM_BITSPERPEL;
		}

		if ( ChangeDisplaySettingsEx( vkw_state.monInfo.szDevice, &dm, NULL, CDS_FULLSCREEN, NULL ) != DISP_CHANGE_SUCCESSFUL )
		{
			return false;
		}

		vkw_state.monitor = MonitorFromWindow(vkw_state.hWnd, MONITOR_DEFAULTTOPRIMARY);
		GetMonitorInfo( vkw_state.monitor, (LPMONITORINFO)&vkw_state.monInfo );
		SetWindowPos( vkw_state.hWnd, NULL, vkw_state.monInfo.rcMonitor.left, vkw_state.monInfo.rcMonitor.top, width, height,
			SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOOWNERZORDER | SWP_NOREPOSITION | SWP_NOZORDER );
	}
	else
	{
		ChangeDisplaySettingsEx( vkw_state.monInfo.szDevice, NULL, NULL, 0, NULL );
	}

	ShowWindow( vkw_state.hWnd, SW_SHOW );
	UpdateWindow( vkw_state.hWnd );
	SetForegroundWindow( vkw_state.hWnd );
	SetFocus( vkw_state.hWnd );

	// let the sound and input subsystems know about the new window
	ri.Vid_NewWindow (width, height);

	return true;
}

void Vkimp_GetInstanceExtensions(char **extensions, uint32_t *extCount)
{
	// check available instance extensions and see if we can use VK_EXT_full_screen_exclusive
	uint32_t instanceExtCount;
	VK_VERIFY(vkEnumerateInstanceExtensionProperties(NULL, &instanceExtCount, NULL));

	if (instanceExtCount > 0)
	{
		qboolean getSurfaceCapabilities2 = false;
		qboolean getPhysicalDeviceProperties2 = false;
		VkExtensionProperties *availableExtensions = (VkExtensionProperties *)malloc(sizeof(VkExtensionProperties) * instanceExtCount);
		VK_VERIFY(vkEnumerateInstanceExtensionProperties(NULL, &instanceExtCount, availableExtensions));

		for (int i = 0; i < instanceExtCount; ++i)
		{
			getSurfaceCapabilities2 |= strcmp(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, availableExtensions[i].extensionName) == 0;
			getPhysicalDeviceProperties2 |= strcmp(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, availableExtensions[i].extensionName) == 0;
#if DEBUG_UTILS_AVAILABLE
			vk_config.vk_ext_debug_utils_supported |= strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, availableExtensions[i].extensionName) == 0;
#endif
#if DEBUG_REPORT_AVAILABLE
			vk_config.vk_ext_debug_report_supported |= strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, availableExtensions[i].extensionName) == 0;
#endif
		}

		// VK_EXT_full_screen_exclusive specification requires VK_KHR_get_surface_capabilities2 and VK_KHR_get_physical_device_properties2
		vk_config.vk_ext_full_screen_exclusive_possible = getSurfaceCapabilities2 && getPhysicalDeviceProperties2;

		free(availableExtensions);
	}

	if (extensions)
	{
		extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;
		extensions[1] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
		if (vk_config.vk_ext_full_screen_exclusive_possible)
		{
			extensions[2] = VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME;
			extensions[3] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;
		}
	}

	if (extCount)
		*extCount = vk_config.vk_ext_full_screen_exclusive_possible ? 4 : 2;
}

VkResult Vkimp_CreateSurface()
{
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.hinstance = GetModuleHandle(NULL),
		.hwnd = vkw_state.hWnd,
	};

	return vkCreateWin32SurfaceKHR(vk_instance, &surfaceCreateInfo, NULL, &vk_surface);
}

VkSurfaceCapabilitiesKHR Vkimp_SetupFullScreenExclusive()
{
	g_surface_full_screen_exclusive_win32_info.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_WIN32_INFO_EXT;
	g_surface_full_screen_exclusive_win32_info.pNext = NULL;
	g_surface_full_screen_exclusive_win32_info.hmonitor = vkw_state.monitor;

	vk_state.full_screen_exclusive_info.sType = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
	vk_state.full_screen_exclusive_info.pNext = &g_surface_full_screen_exclusive_win32_info;
	vk_state.full_screen_exclusive_info.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT;

	VkPhysicalDeviceSurfaceInfo2KHR surfInfo2 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
		.pNext = &vk_state.full_screen_exclusive_info,
		.surface = vk_surface
	};
	VkSurfaceCapabilitiesFullScreenExclusiveEXT surfCapFse = {
		.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_FULL_SCREEN_EXCLUSIVE_EXT,
		.pNext = NULL,
		.fullScreenExclusiveSupported = VK_FALSE
	};
	VkSurfaceCapabilities2KHR surfCap2 = {
		.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR,
		.pNext = &surfCapFse
	};

	PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR qvkGetPhysicalDeviceSurfaceCapabilities2KHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR)vkGetInstanceProcAddr(vk_instance, "vkGetPhysicalDeviceSurfaceCapabilities2KHR");
	VK_VERIFY(qvkGetPhysicalDeviceSurfaceCapabilities2KHR(vk_device.physical, &surfInfo2, &surfCap2));
	vk_config.vk_full_screen_exclusive_enabled = surfCapFse.fullScreenExclusiveSupported;
	return surfCap2.surfaceCapabilities;
}

/*
** Vkimp_SetMode
*/
rserr_t Vkimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
	int width, height;
	const char *win_fs[] = { "W", "FS" };

	ri.Con_Printf( PRINT_ALL, "Initializing Vulkan display\n");

	ri.Con_Printf (PRINT_ALL, "...setting mode %d:", mode );

	if ( !ri.Vid_GetModeInfo( &width, &height, mode ) )
	{
		ri.Con_Printf( PRINT_ALL, " invalid mode\n" );
		return rserr_invalid_mode;
	}

	ri.Con_Printf( PRINT_ALL, " %d %d %s\n", width, height, win_fs[fullscreen] );

	// destroy the existing window
	if (vkw_state.hWnd)
	{
		Vkimp_Shutdown ();
	}

	// do a CDS if needed
	if ( fullscreen )
	{
		ri.Con_Printf( PRINT_ALL, "...attempting fullscreen\n" );

		if ( vk_bitdepth->value != 0 )
		{
			ri.Con_Printf( PRINT_ALL, "...using vk_bitdepth of %d\n", ( int ) vk_bitdepth->value );
		}
		else
		{
			HDC hdc = GetDC( NULL );
			int bitspixel = GetDeviceCaps( hdc, BITSPIXEL );

			ri.Con_Printf( PRINT_ALL, "...using desktop display depth of %d\n", bitspixel );

			ReleaseDC( 0, hdc );
		}

		ri.Con_Printf( PRINT_ALL, "...calling CDS: " );
		if ( VID_CreateWindow(width, height, true) )
		{
			*pwidth = width;
			*pheight = height;

			vk_state.fullscreen = true;

			ri.Con_Printf( PRINT_ALL, "ok\n" );
			return rserr_ok;
		}
		else
		{
			ri.Con_Printf( PRINT_ALL, " failed\n" );
			ri.Con_Printf( PRINT_ALL, "...setting windowed mode\n" );

			DestroyWindow(vkw_state.hWnd);
			UnregisterClass(WINDOW_CLASS_NAME, vkw_state.hInstance);
			VID_CreateWindow(width, height, false);

			*pwidth = width;
			*pheight = height;
			vk_state.fullscreen = false;
			return rserr_invalid_fullscreen;
		}
	}
	else
	{
		ri.Con_Printf( PRINT_ALL, "...setting windowed mode\n" );

		*pwidth = width;
		*pheight = height;
		vk_state.fullscreen = false;
		if ( !VID_CreateWindow (width, height, false) )
			return rserr_invalid_mode;
	}

	return rserr_ok;
}

/*
** Vkimp_Shutdown
**
** For Vulkan, the OS-specific part here is only destroying the window.
**
*/
void Vkimp_Shutdown( void )
{
	if (vkw_state.hWnd)
	{
		DestroyWindow(vkw_state.hWnd);
		vkw_state.hWnd = NULL;
	}

	if (vkw_state.log_fp)
	{
		fclose(vkw_state.log_fp);
		vkw_state.log_fp = 0;
	}

	UnregisterClass(WINDOW_CLASS_NAME, vkw_state.hInstance);

	if (vk_state.fullscreen)
	{
		ChangeDisplaySettingsEx( vkw_state.monInfo.szDevice, NULL, NULL, 0, NULL );
		vk_state.fullscreen = false;
	}
}


/*
** Vkimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of Vulkan. 
*/
qboolean Vkimp_Init( void *hinstance, void *wndproc )
{
#define OSR2_BUILD_NUMBER 1111

	OSVERSIONINFO	vinfo;

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	vkw_state.allowdisplaydepthchange = false;

	if ( GetVersionEx( &vinfo) )
	{
		if ( vinfo.dwMajorVersion > 4 )
		{
			vkw_state.allowdisplaydepthchange = true;
		}
		else if ( vinfo.dwMajorVersion == 4 )
		{
			if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
			{
				vkw_state.allowdisplaydepthchange = true;
			}
			else if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
			{
				if ( LOWORD( vinfo.dwBuildNumber ) >= OSR2_BUILD_NUMBER )
				{
					vkw_state.allowdisplaydepthchange = true;
				}
			}
		}
	}
	else
	{
		ri.Con_Printf( PRINT_ALL, "Vkimp_Init() - GetVersionEx failed\n" );
		return false;
	}

	vkw_state.hInstance = ( HINSTANCE ) hinstance;
	vkw_state.wndproc = wndproc;

	return true;
}

/*
** Vkimp_BeginFrame
*/
void Vkimp_BeginFrame( float camera_separation )
{
	if (vk_bitdepth->modified)
	{
		if (vk_bitdepth->value != 0 && !vkw_state.allowdisplaydepthchange)
		{
			ri.Cvar_SetValue("vk_bitdepth", 0);
			ri.Con_Printf(PRINT_ALL, "vk_bitdepth requires Win95 OSR2.x or WinNT 4.x\n");
		}
		vk_bitdepth->modified = false;
	}
}

/*
** Vkimp_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.
*/
void Vkimp_EndFrame (void)
{
}

/*
** Vkimp_AppActivate
*/
void Vkimp_AppActivate( qboolean active )
{
	if ( active )
	{
		SetForegroundWindow( vkw_state.hWnd );
		ShowWindow( vkw_state.hWnd, SW_RESTORE );
	}
	else
	{
		if ( vid_fullscreen->value )
			ShowWindow( vkw_state.hWnd, SW_MINIMIZE );
	}
}
