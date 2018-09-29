/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2018 Krzysztof Kondrak

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

qboolean Vkimp_InitVulkan (void);

vkwstate_t vkw_state;

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

	if (fullscreen)
	{
		x = 0;
		y = 0;
	}
	else
	{
		vid_xpos = ri.Cvar_Get ("vid_xpos", "0", 0);
		vid_ypos = ri.Cvar_Get ("vid_ypos", "0", 0);
		x = vid_xpos->value;
		y = vid_ypos->value;
	}

	vkw_state.hWnd = CreateWindowEx (
		 exstyle, 
		 WINDOW_CLASS_NAME,
		 "Quake 2",
		 stylebits,
		 x, y, w, h,
		 NULL,
		 NULL,
		 vkw_state.hInstance,
		 NULL);

	if (!vkw_state.hWnd)
		ri.Sys_Error (ERR_FATAL, "Couldn't create window");
	
	ShowWindow( vkw_state.hWnd, SW_SHOW );
	UpdateWindow( vkw_state.hWnd );

	// init all the gl stuff for the window
	if (!Vkimp_InitVulkan ())
	{
		ri.Con_Printf( PRINT_ALL, "VID_CreateWindow() - Vkimp_InitVulkan failed\n");
		return false;
	}

	SetForegroundWindow( vkw_state.hWnd );
	SetFocus( vkw_state.hWnd );

	// let the sound and input subsystems know about the new window
	ri.Vid_NewWindow (width, height);

	return true;
}

void Vkimp_GetSurfaceExtensions(char **extensions, uint32_t *extCount)
{
	if (extensions)
	{
		extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;
		extensions[1] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
	}

	if (extCount)
		*extCount = 2;
}

VkResult Vkimp_CreateSurface()
{
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hwnd = vkw_state.hWnd,
		.hinstance = GetModuleHandle(NULL)
	};

	return vkCreateWin32SurfaceKHR(vk_instance, &surfaceCreateInfo, NULL, &vk_surface);
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
		DEVMODE dm;

		ri.Con_Printf( PRINT_ALL, "...attempting fullscreen\n" );

		memset( &dm, 0, sizeof( dm ) );

		dm.dmSize = sizeof( dm );

		dm.dmPelsWidth  = width;
		dm.dmPelsHeight = height;
		dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

		if ( gl_bitdepth->value != 0 )
		{
			dm.dmBitsPerPel = gl_bitdepth->value;
			dm.dmFields |= DM_BITSPERPEL;
			ri.Con_Printf( PRINT_ALL, "...using gl_bitdepth of %d\n", ( int ) gl_bitdepth->value );
		}
		else
		{
			HDC hdc = GetDC( NULL );
			int bitspixel = GetDeviceCaps( hdc, BITSPIXEL );

			ri.Con_Printf( PRINT_ALL, "...using desktop display depth of %d\n", bitspixel );

			ReleaseDC( 0, hdc );
		}

		ri.Con_Printf( PRINT_ALL, "...calling CDS: " );
		if ( ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) == DISP_CHANGE_SUCCESSFUL )
		{
			*pwidth = width;
			*pheight = height;

			vk_state.fullscreen = true;

			ri.Con_Printf( PRINT_ALL, "ok\n" );

			if ( !VID_CreateWindow (width, height, true) )
				return rserr_invalid_mode;

			return rserr_ok;
		}
		else
		{
			*pwidth = width;
			*pheight = height;

			ri.Con_Printf( PRINT_ALL, "failed\n" );

			ri.Con_Printf( PRINT_ALL, "...calling CDS assuming dual monitors:" );

			dm.dmPelsWidth = width * 2;
			dm.dmPelsHeight = height;
			dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

			if ( gl_bitdepth->value != 0 )
			{
				dm.dmBitsPerPel = gl_bitdepth->value;
				dm.dmFields |= DM_BITSPERPEL;
			}

			/*
			** our first CDS failed, so maybe we're running on some weird dual monitor
			** system 
			*/
			if ( ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL )
			{
				ri.Con_Printf( PRINT_ALL, " failed\n" );

				ri.Con_Printf( PRINT_ALL, "...setting windowed mode\n" );

				ChangeDisplaySettings( 0, 0 );

				*pwidth = width;
				*pheight = height;
				vk_state.fullscreen = false;
				if ( !VID_CreateWindow (width, height, false) )
					return rserr_invalid_mode;
				return rserr_invalid_fullscreen;
			}
			else
			{
				ri.Con_Printf( PRINT_ALL, " ok\n" );
				if ( !VID_CreateWindow (width, height, true) )
					return rserr_invalid_mode;

				vk_state.fullscreen = true;
				return rserr_ok;
			}
		}
	}
	else
	{
		ri.Con_Printf( PRINT_ALL, "...setting windowed mode\n" );

		ChangeDisplaySettings( 0, 0 );

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
** This routine does all OS specific shutdown procedures for the Vulkan
** subsystem.  Under Vulkan this means NULLing out the current DC and
** HGLRC, deleting the rendering context, and releasing the DC acquired
** for the window.  The state structure is also nulled out.
**
*/
void Vkimp_Shutdown( void )
{
	if (vkw_state.hDC)
	{
		if (!ReleaseDC(vkw_state.hWnd, vkw_state.hDC))
			ri.Con_Printf(PRINT_ALL, "ref_gl::R_Shutdown() - ReleaseDC failed\n");
		vkw_state.hDC = NULL;
	}
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
		ChangeDisplaySettings(0, 0);
		vk_state.fullscreen = false;
	}
}


/*
** Vkimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of Vulkan. Under Win32 this means dealing with the pixelformats and
** doing the wgl interface stuff.
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

qboolean Vkimp_InitVulkan (void)
{
    return true;
}

/*
** Vkimp_BeginFrame
*/
void Vkimp_BeginFrame( float camera_separation )
{

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
