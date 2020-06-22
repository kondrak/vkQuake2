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
** VK_IMP.M
**
** This file contains ALL Linux specific stuff having to do with the
** Vulkan refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** Vkimp_EndFrame
** Vkimp_Init
** Vkimp_Shutdown
** Vkimp_SwitchFullscreen
**
*/

#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#import <CoreGraphics/CGDirectDisplay.h>
#import <QuartzCore/CAMetalLayer.h>

#include "../ref_vk/vk_local.h"

#include "../client/keys.h"

#include "../linux/rw_linux.h"
#include "../macos/vk_macos.h"
#include "../macos/win_macos.h"

vkwstate_t vkw_state;

/*****************************************************************************/
/* MOUSE                                                                     */
/*****************************************************************************/

// this is inside the renderer shared lib, so these are called from vid_dylib

static qboolean        mouse_avail;
static int	old_mouse_x, old_mouse_y;

static cvar_t	*m_filter;
static cvar_t	*in_mouse;
static cvar_t	*in_dgamouse;

static qboolean vidmode_active = false;

static qboolean	mlooking;

// state struct passed in Init
in_state_t	*in_state;
int			mx, my;
qboolean	mouse_active = false;

static cvar_t *sensitivity;
static cvar_t *lookstrafe;
static cvar_t *m_side;
static cvar_t *m_yaw;
static cvar_t *m_pitch;
static cvar_t *m_forward;
static cvar_t *freelook;


static void Force_CenterView_f (void)
{
	in_state->viewangles[PITCH] = 0;
}

static void RW_IN_MLookDown (void) 
{ 
	mlooking = true; 
}

static void RW_IN_MLookUp (void) 
{
	mlooking = false;
	in_state->IN_CenterView_fp ();
}

void RW_IN_Init(in_state_t *in_state_p)
{
	in_state = in_state_p;

	// mouse variables
	m_filter = ri.Cvar_Get ("m_filter", "0", 0);
	in_mouse = ri.Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
	in_dgamouse = ri.Cvar_Get ("in_dgamouse", "1", CVAR_ARCHIVE);
	freelook = ri.Cvar_Get( "freelook", "0", 0 );
	lookstrafe = ri.Cvar_Get ("lookstrafe", "0", 0);
	sensitivity = ri.Cvar_Get ("sensitivity", "3", 0);
	m_pitch = ri.Cvar_Get ("m_pitch", "0.022", 0);
	m_yaw = ri.Cvar_Get ("m_yaw", "0.022", 0);
	m_forward = ri.Cvar_Get ("m_forward", "1", 0);
	m_side = ri.Cvar_Get ("m_side", "0.8", 0);

	ri.Cmd_AddCommand ("+mlook", RW_IN_MLookDown);
	ri.Cmd_AddCommand ("-mlook", RW_IN_MLookUp);

	ri.Cmd_AddCommand ("force_centerview", Force_CenterView_f);

	mx = my = 0.0;
	mouse_avail = true;
}

void RW_IN_Shutdown(void)
{
	mouse_avail = false;
}

/*
===========
IN_Commands
===========
*/
void RW_IN_Commands (void)
{
}

/*
===========
IN_Move
===========
*/
void RW_IN_Move (usercmd_t *cmd)
{
	if (!mouse_avail)
		return;

	if (m_filter->value)
	{
		mx = (mx + old_mouse_x) * 0.5;
		my = (my + old_mouse_y) * 0.5;
	}

	old_mouse_x = mx;
	old_mouse_y = my;

	mx *= sensitivity->value;
	my *= sensitivity->value;

// add mouse X/Y movement to cmd
	if ( (*in_state->in_strafe_state & 1) || 
		(lookstrafe->value && mlooking ))
		cmd->sidemove += m_side->value * mx;
	else
		in_state->viewangles[YAW] -= m_yaw->value * mx;

	if ( (mlooking || freelook->value) && 
		!(*in_state->in_strafe_state & 1))
	{
		in_state->viewangles[PITCH] += m_pitch->value * my;
	}
	else
	{
		cmd->forwardmove -= m_forward->value * my;
	}

	mx = my = 0;
}

static void IN_DeactivateMouse( void ) 
{
	if (!mouse_avail)
		return;

	if (mouse_active) {
		mouse_active = false;
		CGDisplayShowCursor(kCGDirectMainDisplay);
	}
}

static void IN_ActivateMouse( void ) 
{
	if (!mouse_avail)
		return;

	if (!mouse_active) {
		mx = my = 0; // don't spazz
		mouse_active = true;
		CGDisplayHideCursor(kCGDirectMainDisplay);
	}
}

void RW_IN_Frame (void)
{
}

void RW_IN_Activate(qboolean active)
{
	if (active || vidmode_active)
		IN_ActivateMouse();
	else
		IN_DeactivateMouse ();
}

/*****************************************************************************/
/* KEYBOARD                                                                  */
/*****************************************************************************/
Key_Event_fp_t Key_Event_fp;

void KBD_Init(Key_Event_fp_t fp)
{
	Key_Event_fp = fp;
}

void KBD_Update(void)
{
	CocoaHandleEvents();
}

void KBD_Close(void)
{
}

/*****************************************************************************/
static void signal_handler(int sig)
{
	printf("Received signal %d, exiting...\n", sig);
	Vkimp_Shutdown();
	_exit(0);
}

static void InitSig(void)
{
	signal(SIGHUP, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGTRAP, signal_handler);
	signal(SIGIOT, signal_handler);
	signal(SIGBUS, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
}

/*
** Vkimp_SetMode
*/
int Vkimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
	int width, height;
	ri.Con_Printf(PRINT_ALL, "Initializing Metal display\n");

	if(fullscreen)
		ri.Con_Printf(PRINT_ALL, "...setting fullscreen mode %d:", mode);
	else
		ri.Con_Printf(PRINT_ALL, "...setting mode %d:", mode);

	if(!ri.Vid_GetModeInfo(&width, &height, mode))
	{
		ri.Con_Printf(PRINT_ALL, " invalid mode\n");
		return rserr_invalid_mode;
	}

	ri.Con_Printf(PRINT_ALL, " %d %d\n", width, height);

	// destroy the existing window
	Vkimp_Shutdown();

	cvar_t *vid_xpos = ri.Cvar_Get("vid_xpos", "3", CVAR_ARCHIVE);
	cvar_t *vid_ypos = ri.Cvar_Get("vid_ypos", "22", CVAR_ARCHIVE);
	CocoaCreateWindow((int)vid_xpos->value, (int)vid_ypos->value, &width, &height, fullscreen);

	*pwidth = width;
	*pheight = height;
	ri.Vid_NewWindow(width, height);
	return rserr_ok;
}

void Vkimp_GetInstanceExtensions(char **extensions, uint32_t *extCount)
{
	// check if we can use optional instance extensions
	uint32_t instanceExtCount;
	VK_VERIFY(vkEnumerateInstanceExtensionProperties(NULL, &instanceExtCount, NULL));

	if (instanceExtCount > 0)
	{
		VkExtensionProperties *availableExtensions = (VkExtensionProperties *)malloc(sizeof(VkExtensionProperties) * instanceExtCount);
		VK_VERIFY(vkEnumerateInstanceExtensionProperties(NULL, &instanceExtCount, availableExtensions));

		for (int i = 0; i < instanceExtCount; ++i)
		{
#if DEBUG_UTILS_AVAILABLE
			vk_config.vk_ext_debug_utils_supported |= strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, availableExtensions[i].extensionName) == 0;
#endif
#if DEBUG_REPORT_AVAILABLE
			vk_config.vk_ext_debug_report_supported |= strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, availableExtensions[i].extensionName) == 0;
#endif
		}

		free(availableExtensions);
	}

	if (extensions)
	{
		extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;
		extensions[1] = VK_MVK_MACOS_SURFACE_EXTENSION_NAME;
	}

	if (extCount)
		*extCount = 2;
}

VkResult Vkimp_CreateSurface()
{
	VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
		.pNext = NULL,
		.flags = 0,
		.pView = CocoaAddMetalView()
	};
	
	return vkCreateMacOSSurfaceMVK(vk_instance, &surfaceCreateInfo, NULL, &vk_surface);
}

/*
** Vkimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the Vulkan
** subsystem.
**
*/
void Vkimp_Shutdown( void )
{
	CocoaDestroyWindow();
}

/*
** Vkimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of Vulkan.
*/
int Vkimp_Init( void *hinstance, void *wndproc )
{
	InitSig();

	return true;
}

/*
** Vkimp_BeginFrame
*/
void Vkimp_BeginFrame( float camera_seperation )
{
}

/*
** Vkimp_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a Vkimp
** function and instead do a call to Vkimp_SwapBuffers.
*/
void Vkimp_EndFrame (void)
{
}

/*
** Vkimp_AppActivate
*/
void Vkimp_AppActivate( qboolean active )
{
}
