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

VkInstance	 vk_instance = VK_NULL_HANDLE;
VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
VmaAllocator vk_malloc  = VK_NULL_HANDLE;
qvkdevice_t	 vk_device = {
	.physical = VK_NULL_HANDLE,
	.logical = VK_NULL_HANDLE,
	.gfxQueue = VK_NULL_HANDLE,
	.presentQueue = VK_NULL_HANDLE,
	.transferQueue = VK_NULL_HANDLE,
	.gfxFamilyIndex = -1,
	.presentFamilyIndex = -1,
	.transferFamilyIndex = -1
};
qvkswapchain_t vk_swapchain = {
	.sc = VK_NULL_HANDLE,
	.format = VK_FORMAT_UNDEFINED,
	.presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
	.extent = { 0, 0 },
	.images = NULL,
	.imageCount = 0
};

qvkrenderpass_t vk_renderpasses[RT_COUNT] = { 
	{
		.rp = VK_NULL_HANDLE,
		.colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.sampleCount = VK_SAMPLE_COUNT_1_BIT
	},
	{
		.rp = VK_NULL_HANDLE,
		.colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.sampleCount = VK_SAMPLE_COUNT_8_BIT
	} 
};

VkCommandPool vk_commandPool = VK_NULL_HANDLE;
VkCommandPool vk_transferCommandPool = VK_NULL_HANDLE;

// Vulkan image views
VkImageView *vk_imageviews = NULL;
// Vulkan framebuffers
VkFramebuffer *vk_framebuffers[RT_COUNT];
// depth buffer
qvktexture_t vk_depthbuffer = QVVKTEXTURE_INIT;
// render targets for MSAA
qvktexture_t vk_msaaColorbuffer = QVVKTEXTURE_INIT;
qvktexture_t vk_msaaDepthbuffer = QVVKTEXTURE_INIT;

PFN_vkCreateDebugUtilsMessengerEXT qvkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT qvkDestroyDebugUtilsMessengerEXT;

VkFormat QVk_FindDepthFormat()
{
	VkFormat depthFormats[] = {
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM_S8_UINT,
		VK_FORMAT_D16_UNORM
	};

	for (int i = 0; i < 5; ++i)
	{
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(vk_device.physical, depthFormats[i], &formatProps);

		if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			return depthFormats[i];
	}

	return VK_FORMAT_D16_UNORM;
}

// internal helpers
void DestroyImageViews()
{
	for (int i = 0; i < vk_swapchain.imageCount; i++)
		vkDestroyImageView(vk_device.logical, vk_imageviews[i], NULL);
	free(vk_imageviews);
	vk_imageviews = NULL;
}

VkResult CreateImageViews()
{
	VkResult res = VK_SUCCESS;
	vk_imageviews = (VkImageView *)malloc(vk_swapchain.imageCount * sizeof(VkImageView));

	for (size_t i = 0; i < vk_swapchain.imageCount; ++i)
	{
		VkResult res = QVk_CreateImageView(&vk_swapchain.images[i], VK_IMAGE_ASPECT_COLOR_BIT, &vk_imageviews[i], vk_swapchain.format, 1);

		if (res != VK_SUCCESS)
		{
			DestroyImageViews();
			return res;
		}
	}

	return res;
}

void DestroyFramebuffers()
{
	for (int f = 0; f < RT_COUNT; f++)
	{
		if (vk_framebuffers[f])
		{
			for (int i = 0; i < vk_swapchain.imageCount; ++i)
			{
				vkDestroyFramebuffer(vk_device.logical, vk_framebuffers[f][i], NULL);
			}

			free(vk_framebuffers[f]);
			vk_framebuffers[f] = NULL;
		}
	}
}

VkResult CreateFramebuffers(const qvkrenderpass_t *renderpass, qvkrendertype_t framebufferType)
{
	VkResult res = VK_SUCCESS;
	vk_framebuffers[framebufferType] = (VkFramebuffer *)malloc(vk_swapchain.imageCount * sizeof(VkFramebuffer));

	VkFramebufferCreateInfo fbCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = renderpass->rp,
		.attachmentCount = (renderpass->sampleCount != VK_SAMPLE_COUNT_1_BIT) ? 3 : 2,
		.width = vk_swapchain.extent.width,
		.height = vk_swapchain.extent.height,
		.layers = 1
	};

	for (size_t i = 0; i < vk_swapchain.imageCount; ++i)
	{
		VkImageView attachments[] = { vk_imageviews[i], vk_depthbuffer.imageView };
		VkImageView attachmentsMSAA[] = { vk_msaaColorbuffer.imageView, vk_msaaDepthbuffer.imageView, vk_imageviews[i] };

		fbCreateInfo.pAttachments = (renderpass->sampleCount != VK_SAMPLE_COUNT_1_BIT) ? attachmentsMSAA : attachments;
		VkResult result = vkCreateFramebuffer(vk_device.logical, &fbCreateInfo, NULL, &vk_framebuffers[framebufferType][i]);

		if (result != VK_SUCCESS)
		{
			DestroyFramebuffers();
			return res;
		}
	}

	return res;
}

void CreateDrawBuffers()
{
	QVk_CreateDepthBuffer(vk_renderpasses[RT_STANDARD].sampleCount, &vk_depthbuffer);
	ri.Con_Printf(PRINT_ALL, "...created depth buffer\n");
	QVk_CreateDepthBuffer(vk_renderpasses[RT_MSAA].sampleCount, &vk_msaaDepthbuffer);
	ri.Con_Printf(PRINT_ALL, "...created MSAA depth buffer\n");
	QVk_CreateColorBuffer(vk_renderpasses[RT_MSAA].sampleCount, &vk_msaaColorbuffer);
	ri.Con_Printf(PRINT_ALL, "...created MSAA color buffer\n");
}

void DestroyDrawBuffer(qvktexture_t *drawBuffer)
{
	if (drawBuffer->image != VK_NULL_HANDLE)
	{
		vmaDestroyImage(vk_malloc, drawBuffer->image, drawBuffer->allocation);
		vkDestroyImageView(vk_device.logical, drawBuffer->imageView, NULL);
		drawBuffer->image = VK_NULL_HANDLE;
		drawBuffer->imageView = VK_NULL_HANDLE;
	}
}

void DestroyDrawBuffers()
{
	DestroyDrawBuffer(&vk_depthbuffer);
	DestroyDrawBuffer(&vk_msaaDepthbuffer);
	DestroyDrawBuffer(&vk_msaaColorbuffer);
}

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
		vkDeviceWaitIdle(vk_device.logical);

		for (int i = 0; i < RT_COUNT; i++)
		{
			if (vk_renderpasses[i].rp != VK_NULL_HANDLE)
				vkDestroyRenderPass(vk_device.logical, vk_renderpasses[i].rp, NULL);
			vk_renderpasses[i].rp = VK_NULL_HANDLE;
		}
		if (vk_commandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(vk_device.logical, vk_commandPool, NULL);
		if (vk_transferCommandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(vk_device.logical, vk_transferCommandPool, NULL);
		DestroyFramebuffers();
		DestroyImageViews();
		DestroyDrawBuffers();
		if (vk_swapchain.sc != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(vk_device.logical, vk_swapchain.sc, NULL);
			free(vk_swapchain.images);
			vk_swapchain.images = NULL;
			vk_swapchain.imageCount = 0;
		}
		if (vk_malloc != VK_NULL_HANDLE)
			vmaDestroyAllocator(vk_malloc);
		if (vk_device.logical != VK_NULL_HANDLE)
			vkDestroyDevice(vk_device.logical, NULL);
		if(vk_surface != VK_NULL_HANDLE)
			vkDestroySurfaceKHR(vk_instance, vk_surface, NULL);
		QVk_DestroyValidationLayers();

		vkDestroyInstance(vk_instance, NULL);
		vk_instance = VK_NULL_HANDLE;
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

	if(vk_validation->value)
		extCount++;

	wantedExtensions = (char **)malloc(extCount * sizeof(const char *));
	Vkimp_GetSurfaceExtensions(wantedExtensions, NULL);

	if (vk_validation->value)
		wantedExtensions[extCount-1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

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

	if (vk_validation->value)
	{
		const char *validationLayers[] = { "VK_LAYER_LUNARG_standard_validation" };
		createInfo.enabledLayerCount = sizeof(validationLayers)/sizeof(validationLayers[0]);
		createInfo.ppEnabledLayerNames = validationLayers;
	}

	VkResult res = vkCreateInstance(&createInfo, NULL, &vk_instance);
	free(wantedExtensions);

	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan instance: %s\n", QVk_GetError(res));
		return false;
	}
	ri.Con_Printf(PRINT_ALL, "...created Vulkan instance\n");

	// initialize function pointers
	qvkCreateDebugUtilsMessengerEXT  = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkCreateDebugUtilsMessengerEXT");
	qvkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk_instance, "vkDestroyDebugUtilsMessengerEXT");

	if (vk_validation->value)
		QVk_CreateValidationLayers();

	res = Vkimp_CreateSurface();
	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan surface: %s\n", QVk_GetError(res));
		return false;
	}
	ri.Con_Printf(PRINT_ALL, "...created Vulkan surface\n");

	// create Vulkan device
	QVk_CreateDevice();
	// create memory allocator
	VmaAllocatorCreateInfo allocInfo = {
		.physicalDevice = vk_device.physical,
		.device = vk_device.logical
	};

	res = vmaCreateAllocator(&allocInfo, &vk_malloc);
	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan memory allocator: %s\n", QVk_GetError(res));
		return false;
	}
	ri.Con_Printf(PRINT_ALL, "...created Vulkan memory allocator\n");

	// setup swapchain
	res = QVk_CreateSwapchain();
	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan swapchain: %s\n", QVk_GetError(res));
		return false;
	}
	ri.Con_Printf(PRINT_ALL, "...created Vulkan swapchain\n");

	// setup render passes
	res = QVk_CreateRenderpass(&vk_renderpasses[0]);
	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan renderpass: %s\n", QVk_GetError(res));
		return false;
	}
	ri.Con_Printf(PRINT_ALL, "...created Vulkan renderpass\n");
	res = QVk_CreateRenderpass(&vk_renderpasses[1]);
	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan MSAA renderpass: %s\n", QVk_GetError(res));
		return false;
	}
	ri.Con_Printf(PRINT_ALL, "...created Vulkan MSAA renderpass\n");

	// setup command pools
	res = QVk_CreateCommandPool(&vk_commandPool, vk_device.gfxFamilyIndex);
	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan command pool for graphics: %s\n", QVk_GetError(res));
		return false;
	}
	res = QVk_CreateCommandPool(&vk_transferCommandPool, vk_device.transferFamilyIndex);
	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan command pool for transfer: %s\n", QVk_GetError(res));
		return false;
	}
	ri.Con_Printf(PRINT_ALL, "...created Vulkan command pools\n");

	// setup draw buffers
	CreateDrawBuffers();

	// setup image views
	res = CreateImageViews();
	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan image views: %s\n", QVk_GetError(res));
		return false;
	}
	ri.Con_Printf(PRINT_ALL, "...created %d Vulkan image view(s)\n", vk_swapchain.imageCount);

	// setup framebuffers
	res = CreateFramebuffers(&vk_renderpasses[RT_STANDARD], RT_STANDARD);
	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan framebuffers: %s\n", QVk_GetError(res));
		return false;
	}
	ri.Con_Printf(PRINT_ALL, "...created Vulkan framebuffers\n");
	res = CreateFramebuffers(&vk_renderpasses[RT_MSAA], RT_MSAA);
	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan MSAA framebuffers: %s\n", QVk_GetError(res));
		return false;
	}
	ri.Con_Printf(PRINT_ALL, "...created Vulkan MSAA framebuffers\n");

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



