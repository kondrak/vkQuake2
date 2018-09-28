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
** QVK_WIN.C
**
** This file implements the operating system binding of Vk to QVk function
** pointers.  When doing a port of Quake2 you must implement the following
** two functions:
**
** QVk_Init() - loads libraries, assigns function pointers, etc.
** QVk_Shutdown() - unloads libraries, NULLs function pointers
*/
#include <float.h>
#include "../ref_vk/vk_local.h"
#include "vk_win.h"

#ifdef _DEBUG
#	define VALIDATION_LAYERS_ON
#endif

VkInstance	 vk_instance = VK_NULL_HANDLE;
VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
VmaAllocator vk_malloc  = VK_NULL_HANDLE;

PFN_vkCreateDebugUtilsMessengerEXT qvkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT qvkDestroyDebugUtilsMessengerEXT;


/*
** QVk_Shutdown
**
** Destroy all Vulkan related resources.
*/
void QVk_Shutdown( void )
{
	if (vk_instance != VK_NULL_HANDLE)
	{
		ri.Con_Printf(PRINT_ALL, "Shutting down Vulkan\n");
		vkDestroySurfaceKHR(vk_instance, vk_surface, NULL);
#ifdef VALIDATION_LAYERS_ON
		QVk_DestroyValidationLayers();
#endif
		vkDestroyInstance(vk_instance, NULL);
	}
}

#	pragma warning (disable : 4113 4133 4047 )

/*
** QVk_Init
**
** This is responsible for initializing Vulkan.
** 
*/
qboolean QVk_Init()
{
	ri.Con_Printf(PRINT_ALL, "Initializing Vulkan display\n");

	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Quake 2",
		.applicationVersion = VK_MAKE_VERSION(3, 21, 0),
		.pEngineName = "id Tech 2",
		.engineVersion = VK_MAKE_VERSION(2, 0, 0),
		.apiVersion = VK_API_VERSION_1_1
	};

	uint32_t extCount;
	char **wantedExtensions;

	Vkimp_GetSurfaceExtensions(NULL, &extCount);

#ifdef VALIDATION_LAYERS_ON
	extCount++;
#endif

	wantedExtensions = (char **)malloc(extCount * sizeof(const char *));
	Vkimp_GetSurfaceExtensions(wantedExtensions, NULL);

#ifdef VALIDATION_LAYERS_ON
	wantedExtensions[extCount-1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#endif

	ri.Con_Printf(PRINT_ALL, "Vulkan extensions: ");
	for (int i = 0; i < extCount; i++)
	{
		ri.Con_Printf(PRINT_ALL, "%s ", wantedExtensions[i]);
	}
	ri.Con_Printf(PRINT_ALL, "\n");

	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = extCount,
		.enabledLayerCount = 0,
		.ppEnabledExtensionNames = wantedExtensions
	};

#ifdef VALIDATION_LAYERS_ON
	const char *validationLayers[] = { "VK_LAYER_LUNARG_standard_validation" };
	createInfo.enabledLayerCount = 1;
	createInfo.ppEnabledLayerNames = validationLayers;
#endif
	VkResult res = vkCreateInstance(&createInfo, NULL, &vk_instance);
	free(wantedExtensions);

	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan instance: %s\n", QVk_GetError(res));
		return false;
	}
	ri.Con_Printf(PRINT_ALL, "...Successfully created Vulkan instance\n");

	// initialize function pointers
	qvkCreateDebugUtilsMessengerEXT  = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkCreateDebugUtilsMessengerEXT");
	qvkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkDestroyDebugUtilsMessengerEXT");

#ifdef VALIDATION_LAYERS_ON
	QVk_CreateValidationLayers();
#endif

	res = Vkimp_CreateSurface();
	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan surface: %s\n", QVk_GetError(res));
		return false;
	}
	ri.Con_Printf(PRINT_ALL, "...Successfully created Vulkan surface\n");

	return false;
}

const char *QVk_GetError(VkResult errorCode)
{
#define ERRSTR(r) case VK_ ##r: return "VK_"#r
	switch (errorCode)
	{
		ERRSTR(NOT_READY);
		ERRSTR(TIMEOUT);
		ERRSTR(EVENT_SET);
		ERRSTR(EVENT_RESET);
		ERRSTR(INCOMPLETE);
		ERRSTR(ERROR_OUT_OF_HOST_MEMORY);
		ERRSTR(ERROR_OUT_OF_DEVICE_MEMORY);
		ERRSTR(ERROR_INITIALIZATION_FAILED);
		ERRSTR(ERROR_DEVICE_LOST);
		ERRSTR(ERROR_MEMORY_MAP_FAILED);
		ERRSTR(ERROR_LAYER_NOT_PRESENT);
		ERRSTR(ERROR_EXTENSION_NOT_PRESENT);
		ERRSTR(ERROR_FEATURE_NOT_PRESENT);
		ERRSTR(ERROR_INCOMPATIBLE_DRIVER);
		ERRSTR(ERROR_TOO_MANY_OBJECTS);
		ERRSTR(ERROR_FORMAT_NOT_SUPPORTED);
		ERRSTR(ERROR_SURFACE_LOST_KHR);
		ERRSTR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
		ERRSTR(SUBOPTIMAL_KHR);
		ERRSTR(ERROR_OUT_OF_DATE_KHR);
		ERRSTR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
		ERRSTR(ERROR_VALIDATION_FAILED_EXT);
		ERRSTR(ERROR_INVALID_SHADER_NV);
	}
#undef ERRSTR
	return "UNKNOWN ERROR";
}

void Vkimp_LogNewFrame( void )
{
	fprintf( vkw_state.log_fp, "*** R_BeginFrame ***\n" );
}

#pragma warning (default : 4113 4133 4047 )



