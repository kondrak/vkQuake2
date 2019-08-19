/*
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

#include "vk_local.h"

// internal helper
static const char *presentModeString(VkPresentModeKHR presentMode)
{
#define PMSTR(r) case VK_ ##r: return "VK_"#r
	switch (presentMode)
	{
		PMSTR(PRESENT_MODE_IMMEDIATE_KHR);
		PMSTR(PRESENT_MODE_MAILBOX_KHR);
		PMSTR(PRESENT_MODE_FIFO_KHR);
		PMSTR(PRESENT_MODE_FIFO_RELAXED_KHR);
		default: return "<unknown>";
	}
#undef PMSTR
	return "UNKNOWN PRESENT MODE";
}

// internal helper
static VkSurfaceFormatKHR getSwapSurfaceFormat(const VkSurfaceFormatKHR *surfaceFormats, uint32_t formatCount)
{
	VkSurfaceFormatKHR swapSurfaceFormat;

	for (size_t i = 0; i < formatCount; ++i)
	{
		if (surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
			surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM)
		{
			swapSurfaceFormat.colorSpace = surfaceFormats[i].colorSpace;
			swapSurfaceFormat.format = surfaceFormats[i].format;
			return swapSurfaceFormat;
		}
	}
	// no preferred format, so get the first one from list
	swapSurfaceFormat.colorSpace = surfaceFormats[0].colorSpace;
	swapSurfaceFormat.format = surfaceFormats[0].format;

	return swapSurfaceFormat;
}

// internal helper
static VkPresentModeKHR getSwapPresentMode(const VkPresentModeKHR *presentModes, uint32_t presentModesCount, VkPresentModeKHR desiredMode)
{
	// check if the desired present mode is supported
	for (uint32_t i = 0; i < presentModesCount; ++i)
	{
		// mode supported, nothing to do here
		if (presentModes[i] == desiredMode)
		{
			vk_config.present_mode = presentModeString(desiredMode);
			ri.Con_Printf(PRINT_ALL, "...using present mode: %s\n", vk_config.present_mode);
			return desiredMode;
		}
	}

	// PRESENT_MODE_FIFO_KHR is guaranteed to exist due to spec requirements
	VkPresentModeKHR usedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	// preferred present mode not found - choose the next best thing
	for (uint32_t i = 0; i < presentModesCount; ++i)
	{
		// always prefer mailbox for triple buffering
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			usedPresentMode = presentModes[i];
			break;
		}
		else if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			usedPresentMode = presentModes[i];
		}
	}

	vk_config.present_mode = presentModeString(usedPresentMode);
	ri.Con_Printf(PRINT_ALL, "...present mode %s not supported, using present mode: %s\n", presentModeString(desiredMode), vk_config.present_mode);
	return usedPresentMode;
}

// internal helper
static VkCompositeAlphaFlagBitsKHR getSupportedCompositeAlpha(VkCompositeAlphaFlagsKHR supportedFlags)
{
	VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[] = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
	};

	for (int i = 0; i < 4; ++i)
	{
		if (supportedFlags & compositeAlphaFlags[i])
			return compositeAlphaFlags[i];
	}

	return VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
}

VkResult QVk_CreateSwapchain()
{
	VkSurfaceCapabilitiesKHR surfaceCaps;
	VkSurfaceFormatKHR *surfaceFormats = NULL;
	VkPresentModeKHR *presentModes = NULL;
	uint32_t formatCount, presentModesCount;
	VK_VERIFY(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_device.physical, vk_surface, &surfaceCaps));
	VK_VERIFY(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_device.physical, vk_surface, &formatCount, NULL));
	VK_VERIFY(vkGetPhysicalDeviceSurfacePresentModesKHR(vk_device.physical, vk_surface, &presentModesCount, NULL));

	if (formatCount > 0)
	{
		surfaceFormats = (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
		VK_VERIFY(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_device.physical, vk_surface, &formatCount, surfaceFormats));
	}

	if (presentModesCount > 0)
	{
		presentModes = (VkPresentModeKHR *)malloc(presentModesCount * sizeof(VkPresentModeKHR));
		VK_VERIFY(vkGetPhysicalDeviceSurfacePresentModesKHR(vk_device.physical, vk_surface, &presentModesCount, presentModes));

		ri.Con_Printf(PRINT_ALL, "Supported present modes: ");
		for (int i = 0; i < presentModesCount; i++)
		{
			ri.Con_Printf(PRINT_ALL, "%s ", presentModeString(presentModes[i]));
			vk_config.supported_present_modes[i] = presentModeString(presentModes[i]);
		}
		ri.Con_Printf(PRINT_ALL, "\n");
	}

	VkSurfaceFormatKHR swapSurfaceFormat = getSwapSurfaceFormat(surfaceFormats, formatCount);
	VkPresentModeKHR swapPresentMode = getSwapPresentMode(presentModes, presentModesCount, vk_vsync->value > 0 ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR);
	free(surfaceFormats);
	free(presentModes);

	VkExtent2D extent = surfaceCaps.currentExtent;
	if(extent.width == UINT32_MAX || extent.height == UINT32_MAX)
	{
		extent.width = max(surfaceCaps.minImageExtent.width, min(surfaceCaps.maxImageExtent.width, vid.width));
		extent.height = max(surfaceCaps.minImageExtent.height, min(surfaceCaps.maxImageExtent.height, vid.height));
	}

	// request at least 2 images - this fixes fullscreen crashes on AMD when launching the game in fullscreen
	uint32_t imageCount = max(2, surfaceCaps.minImageCount);
	if (swapPresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		imageCount = max(3, surfaceCaps.minImageCount);

	if (surfaceCaps.maxImageCount > 0)
		imageCount = min(imageCount, surfaceCaps.maxImageCount);

	VkSwapchainKHR oldSwapchain = vk_swapchain.sc;
	VkSwapchainCreateInfoKHR scCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.surface = vk_surface,
		.minImageCount = imageCount,
		.imageFormat = swapSurfaceFormat.format,
		.imageColorSpace = swapSurfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, // TRANSFER_SRC_BIT is required for taking screenshots
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.preTransform = surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : surfaceCaps.currentTransform,
		.compositeAlpha = getSupportedCompositeAlpha(surfaceCaps.supportedCompositeAlpha),
		.presentMode = swapPresentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = oldSwapchain
	};

	uint32_t queueFamilyIndices[] = { (uint32_t)vk_device.gfxFamilyIndex, (uint32_t)vk_device.presentFamilyIndex };
	if (vk_device.presentFamilyIndex != vk_device.gfxFamilyIndex)
	{
		scCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		scCreateInfo.queueFamilyIndexCount = 2;
		scCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}

	vk_swapchain.format = swapSurfaceFormat.format;
	vk_swapchain.extent = extent;
	ri.Con_Printf(PRINT_ALL, "...trying swapchain extent: %dx%d\n", vk_swapchain.extent.width, vk_swapchain.extent.height);
	ri.Con_Printf(PRINT_ALL, "...trying swapchain image format: %d\n", vk_swapchain.format);

	VkResult res = vkCreateSwapchainKHR(vk_device.logical, &scCreateInfo, NULL, &vk_swapchain.sc);
	if (res != VK_SUCCESS)
		return res;

	VK_VERIFY(vkGetSwapchainImagesKHR(vk_device.logical, vk_swapchain.sc, &imageCount, NULL));
	vk_swapchain.images = (VkImage *)malloc(imageCount * sizeof(VkImage));
	vk_swapchain.imageCount = imageCount;
	res = vkGetSwapchainImagesKHR(vk_device.logical, vk_swapchain.sc, &imageCount, vk_swapchain.images);

	if (oldSwapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(vk_device.logical, oldSwapchain, NULL);

	return res;
}
