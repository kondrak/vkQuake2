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
** QVK.H
*/

#ifndef __QVK_H__
#define __QVK_H__

#ifdef _WIN32
#  include <windows.h>
#endif

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

typedef struct
{
	VkPhysicalDevice physical;
	VkDevice		 logical;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures   features;
	VkQueue gfxQueue;
	VkQueue presentQueue;
	VkQueue transferQueue;
	int gfxFamilyIndex;
	int presentFamilyIndex;
	int transferFamilyIndex;
} qvkdevice_t;

typedef struct
{
	VkSwapchainKHR sc;
	VkFormat format;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
	VkImage *images;
	int imageCount;
} qvkswapchain_t;

typedef struct 
{
	VkImage image;
	VmaAllocation allocation;
	VkImageView   imageView;
	VkSampler sampler;
	VkSharingMode sharingMode;
	VkSampleCountFlagBits sampleCount;
	VkFormat  format;
	VkFilter  minFilter;
	VkFilter  magFilter;
	// mipmap settings
	uint32_t mipLevels;
	float mipLodBias;
	float mipMinLod;
	VkSamplerMipmapMode mipmapMode;
	VkFilter mipmapFilter;
} qvktexture_t;

#define QVVKTEXTURE_INIT     { \
	.image = VK_NULL_HANDLE, \
	.allocation = VK_NULL_HANDLE, \
	.imageView = VK_NULL_HANDLE, \
	.sampler = VK_NULL_HANDLE, \
	.sharingMode = VK_SHARING_MODE_MAX_ENUM, \
	.sampleCount = VK_SAMPLE_COUNT_1_BIT, \
	.format = VK_FORMAT_R8G8B8A8_UNORM, \
	.minFilter = VK_FILTER_LINEAR, \
	.magFilter = VK_FILTER_LINEAR, \
	.mipLevels = 1, \
	.mipLodBias = 0.f, \
	.mipMinLod = 0.f, \
	.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR, \
	.mipmapFilter = VK_FILTER_LINEAR, \
}

typedef struct
{
	VkRenderPass rp;
	VkAttachmentLoadOp colorLoadOp;
	VkSampleCountFlagBits sampleCount;
} qvkrenderpass_t;

typedef enum
{
	RT_STANDARD = 0,
	RT_MSAA = 1,
	RT_COUNT = 2
} qvkrendertype_t;

// Vulkan instance
extern VkInstance vk_instance;
// Vulkan surface
extern VkSurfaceKHR vk_surface;
// Vulkan device
extern qvkdevice_t vk_device;
// Vulkan memory allocator
extern VmaAllocator vk_malloc;
// Vulkan swapchain
extern qvkswapchain_t vk_swapchain;
// Vulkan renderpasses (standard and MSAA)
extern qvkrenderpass_t vk_renderpasses[RT_COUNT];
// Vulkan MSAA renderpass
extern qvkrenderpass_t vk_msaaRenderpass;
// Vulkan command pools
extern VkCommandPool vk_commandPool;
extern VkCommandPool vk_transferCommandPool;

// function pointers
extern PFN_vkCreateDebugUtilsMessengerEXT qvkCreateDebugUtilsMessengerEXT;
extern PFN_vkDestroyDebugUtilsMessengerEXT qvkDestroyDebugUtilsMessengerEXT;

qboolean	QVk_Init();
void		QVk_Shutdown( void );
void		QVk_CreateValidationLayers();
void		QVk_DestroyValidationLayers();
qboolean	QVk_CreateDevice();
VkResult	QVk_CreateSwapchain();
VkResult	QVk_CreateRenderpass(qvkrenderpass_t *renderpass);
VkFormat	QVk_FindDepthFormat();
VkResult	QVk_CreateCommandPool(VkCommandPool *commandPool, uint32_t queueFamilyIndex);
VkResult	QVk_CreateImageView(const VkImage *image, VkImageAspectFlags aspectFlags, VkImageView *imageView, VkFormat format, uint32_t mipLevels);
VkResult	QVk_CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memUsage, qvktexture_t *texture);
void		QVk_CreateDepthBuffer(VkSampleCountFlagBits sampleCount, qvktexture_t *depthBuffer);
void		QVk_CreateColorBuffer(VkSampleCountFlagBits sampleCount, qvktexture_t *colorBuffer);
VkResult	QVk_BeginCommand(const VkCommandBuffer *commandBuffer);
void		QVk_SubmitCommand(const VkCommandBuffer *commandBuffer, const VkQueue *queue);
VkCommandBuffer QVk_CreateCommandBuffer(const VkCommandPool *commandPool, VkCommandBufferLevel level);
const char *QVk_GetError(VkResult errorCode);
VkResult	QVk_BeginFrame();
VkResult	QVk_EndFrame();
void		QVk_RecreateSwapchain();
#endif
