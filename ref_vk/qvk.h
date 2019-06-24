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
** QVK.H
*/

#ifndef __QVK_H__
#define __QVK_H__

#ifdef _WIN32
#  include <windows.h>
#endif

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
#include "vk_shaders.h"

// Vulkan device
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

// Vulkan swapchain
typedef struct
{
	VkSwapchainKHR sc;
	VkFormat format;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
	VkImage *images;
	int imageCount;
} qvkswapchain_t;

// available sampler types
typedef enum
{
	S_NEAREST = 0,
	S_LINEAR = 1,
	S_MIPMAP_NEAREST = 2,
	S_MIPMAP_LINEAR = 3,
	S_ANISO_NEAREST = 4,
	S_ANISO_LINEAR = 5,
	S_ANISO_MIPMAP_NEAREST = 6,
	S_ANISO_MIPMAP_LINEAR = 7,
	S_SAMPLER_CNT = 8
} qvksampler_t;

// texture object
typedef struct 
{
	VkImage image;
	VmaAllocation allocation;
	VmaAllocationCreateFlags vmaFlags;
	VkImageView   imageView;
	VkSharingMode sharingMode;
	VkSampleCountFlagBits sampleCount;
	VkFormat  format;
	VkDescriptorSet descriptorSet;
	// mipmap settings
	uint32_t mipLevels;
} qvktexture_t;

#define QVVKTEXTURE_INIT     { \
	.image = VK_NULL_HANDLE, \
	.allocation = VK_NULL_HANDLE, \
	.vmaFlags = 0, \
	.imageView = VK_NULL_HANDLE, \
	.sharingMode = VK_SHARING_MODE_MAX_ENUM, \
	.sampleCount = VK_SAMPLE_COUNT_1_BIT, \
	.format = VK_FORMAT_R8G8B8A8_UNORM, \
	.descriptorSet = VK_NULL_HANDLE, \
	.mipLevels = 1, \
}

#define QVVKTEXTURE_CLEAR(i)     { \
	(i).image = VK_NULL_HANDLE; \
	(i).allocation = VK_NULL_HANDLE; \
	(i).vmaFlags = 0; \
	(i).imageView = VK_NULL_HANDLE; \
	(i).sharingMode = VK_SHARING_MODE_MAX_ENUM; \
	(i).sampleCount = VK_SAMPLE_COUNT_1_BIT; \
	(i).format = VK_FORMAT_R8G8B8A8_UNORM; \
	(i).mipLevels = 1; \
}

// Vulkan renderpass
typedef struct
{
	VkRenderPass rp;
	VkAttachmentLoadOp colorLoadOp;
	VkSampleCountFlagBits sampleCount;
} qvkrenderpass_t;

// Vulkan buffer
typedef struct
{
	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo allocInfo;
	VkDeviceSize currentOffset;
} qvkbuffer_t;

// Vulkan staging buffer
typedef struct
{
	qvkbuffer_t buffer;
	VkCommandBuffer cmdBuffer;
	VkFence fence;
	qboolean submitted;
} qvkstagingbuffer_t;

// Vulkan buffer options
typedef struct
{
	VkBufferUsageFlags usage;
	VkMemoryPropertyFlags reqMemFlags;
	VkMemoryPropertyFlags prefMemFlags;
	VmaMemoryUsage vmaUsage;
	VmaAllocationCreateFlags vmaFlags;
} qvkbufferopts_t;

// Vulkan pipeline
typedef struct
{
	VkPipelineLayout layout;
	VkPipeline pl;
	VkPipelineCreateFlags flags;
	VkPolygonMode mode;
	VkCullModeFlags cullMode;
	VkPrimitiveTopology topology;
	VkPipelineColorBlendAttachmentState blendOpts;
	VkBool32 depthTestEnable;
	VkBool32 depthWriteEnable;
} qvkpipeline_t;

// Vulkan shader
typedef struct
{
	VkPipelineShaderStageCreateInfo createInfo;
	VkShaderModule module;
} qvkshader_t;

#define QVKPIPELINE_INIT { \
	.layout = VK_NULL_HANDLE, \
	.pl = VK_NULL_HANDLE, \
	.flags = 0, \
	.mode = VK_POLYGON_MODE_FILL, \
	.cullMode = VK_CULL_MODE_BACK_BIT, \
	.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, \
	.blendOpts = { \
		.blendEnable = VK_FALSE, \
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, \
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, \
		.colorBlendOp = VK_BLEND_OP_ADD, \
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, \
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, \
		.alphaBlendOp = VK_BLEND_OP_ADD, \
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT \
	}, \
	.depthTestEnable = VK_TRUE, \
	.depthWriteEnable = VK_TRUE \
}

// type of renderpass (non-MSAA/MSAA)
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
// Vulkan renderpass currently in use (standard or MSAA)
extern qvkrenderpass_t vk_activeRenderpass;
// Vulkan command buffer currently in use
extern VkCommandBuffer vk_activeCmdbuffer;
// Vulkan command pools
extern VkCommandPool vk_commandPool;
extern VkCommandPool vk_transferCommandPool;
// Vulkan descriptor pool
extern VkDescriptorPool vk_descriptorPool;

// Vulkan descriptor sets
extern VkDescriptorSetLayout vk_uboDescSetLayout;
extern VkDescriptorSetLayout vk_samplerDescSetLayout;

// *** pipelines ***
extern qvkpipeline_t vk_drawTexQuadPipeline;
extern qvkpipeline_t vk_drawColorQuadPipeline;
extern qvkpipeline_t vk_drawModelPipelineStrip;
extern qvkpipeline_t vk_drawModelPipelineFan;
extern qvkpipeline_t vk_drawNoDepthModelPipelineStrip;
extern qvkpipeline_t vk_drawNoDepthModelPipelineFan;
extern qvkpipeline_t vk_drawLefthandModelPipelineStrip;
extern qvkpipeline_t vk_drawLefthandModelPipelineFan;
extern qvkpipeline_t vk_drawNullModel;
extern qvkpipeline_t vk_drawParticlesPipeline;
extern qvkpipeline_t vk_drawPointParticlesPipeline;
extern qvkpipeline_t vk_drawSpritePipeline;
extern qvkpipeline_t vk_drawPolyPipeline;
extern qvkpipeline_t vk_drawPolyLmapPipeline;
extern qvkpipeline_t vk_drawPolyWarpPipeline;
extern qvkpipeline_t vk_drawBeamPipeline;
extern qvkpipeline_t vk_drawSkyboxPipeline;
extern qvkpipeline_t vk_drawDLightPipeline;
extern qvkpipeline_t vk_showTrisPipeline;
extern qvkpipeline_t vk_shadowsPipelineStrip;
extern qvkpipeline_t vk_shadowsPipelineFan;

// function pointers
extern PFN_vkCreateDebugUtilsMessengerEXT qvkCreateDebugUtilsMessengerEXT;
extern PFN_vkDestroyDebugUtilsMessengerEXT qvkDestroyDebugUtilsMessengerEXT;

// The Interface Functions (tm)
qboolean	QVk_Init(void);
void		QVk_Shutdown(void);
void		QVk_CreateValidationLayers(void);
void		QVk_DestroyValidationLayers(void);
qboolean	QVk_CreateDevice(int preferredDeviceIdx);
VkResult	QVk_CreateSwapchain(void);
VkResult	QVk_CreateRenderpass(qvkrenderpass_t *renderpass);
VkFormat	QVk_FindDepthFormat(void);
VkResult	QVk_CreateCommandPool(VkCommandPool *commandPool, uint32_t queueFamilyIndex);
VkResult	QVk_CreateImageView(const VkImage *image, VkImageAspectFlags aspectFlags, VkImageView *imageView, VkFormat format, uint32_t mipLevels);
VkResult	QVk_CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memUsage, qvktexture_t *texture);
void		QVk_CreateDepthBuffer(VkSampleCountFlagBits sampleCount, qvktexture_t *depthBuffer);
void		QVk_CreateColorBuffer(VkSampleCountFlagBits sampleCount, qvktexture_t *colorBuffer);
void		QVk_CreateTexture(qvktexture_t *texture, const unsigned char *data, uint32_t width, uint32_t height, qvksampler_t samplerType);
void		QVk_UpdateTextureData(qvktexture_t *texture, const unsigned char *data, uint32_t offset_x, uint32_t offset_y, uint32_t width, uint32_t height);
VkSampler	QVk_UpdateTextureSampler(qvktexture_t *texture, qvksampler_t samplerType);
void		QVk_ReleaseTexture(qvktexture_t *texture);
void		QVk_ReadPixels(uint8_t *dstBuffer, uint32_t width, uint32_t height);
VkResult	QVk_BeginCommand(const VkCommandBuffer *commandBuffer);
void		QVk_SubmitCommand(const VkCommandBuffer *commandBuffer, const VkQueue *queue);
VkCommandBuffer QVk_CreateCommandBuffer(const VkCommandPool *commandPool, VkCommandBufferLevel level);
const char*	QVk_GetError(VkResult errorCode);
VkResult	QVk_BeginFrame(void);
VkResult	QVk_EndFrame(void);
void		QVk_RecreateSwapchain(void);
VkResult	QVk_CreateBuffer(VkDeviceSize size, qvkbuffer_t *dstBuffer, const qvkbufferopts_t options);
void		QVk_FreeBuffer(qvkbuffer_t *buffer);
VkResult	QVk_CreateStagingBuffer(VkDeviceSize size, qvkbuffer_t *dstBuffer, VkMemoryPropertyFlags reqMemFlags, VkMemoryPropertyFlags prefMemFlags);
VkResult	QVk_CreateUniformBuffer(VkDeviceSize size, qvkbuffer_t *dstBuffer, VkMemoryPropertyFlags reqMemFlags, VkMemoryPropertyFlags prefMemFlags);
void		QVk_CreateVertexBuffer(const void *data, VkDeviceSize size, qvkbuffer_t *dstBuffer, qvkbuffer_t *stagingBuffer, VkMemoryPropertyFlags reqMemFlags, VkMemoryPropertyFlags prefMemFlags);
void		QVk_CreateIndexBuffer(const void *data, VkDeviceSize size, qvkbuffer_t *dstBuffer, qvkbuffer_t *stagingBuffer, VkMemoryPropertyFlags reqMemFlags, VkMemoryPropertyFlags prefMemFlags);
qvkshader_t QVk_CreateShader(const uint32_t *shaderSrc, size_t shaderCodeSize, VkShaderStageFlagBits shaderStage);
void		QVk_CreatePipeline(const VkDescriptorSetLayout *descriptorLayout, const uint32_t desLayoutCount, const VkPipelineVertexInputStateCreateInfo *vertexInputInfo, qvkpipeline_t *pipeline, const qvkshader_t *shaders, uint32_t shaderCount, VkPushConstantRange *pcRange);
void		QVk_DestroyPipeline(qvkpipeline_t *pipeline);
uint8_t*	QVk_GetVertexBuffer(VkDeviceSize size, VkBuffer *dstBuffer, VkDeviceSize *dstOffset);
uint8_t*	QVk_GetIndexBuffer(VkDeviceSize size, VkDeviceSize *dstOffset);
uint8_t*	QVk_GetUniformBuffer(VkDeviceSize size, uint32_t *dstOffset, VkDescriptorSet *dstUboDescriptorSet);
uint8_t*	QVk_GetStagingBuffer(VkDeviceSize size, int alignment, VkCommandBuffer *cmdBuffer, VkBuffer *buffer, uint32_t *dstOffset);
VkBuffer	QVk_GetTriangleFanIbo(VkDeviceSize indexCount);
void		QVk_DrawColorRect(float *ubo, VkDeviceSize uboSize);
void		QVk_DrawTexRect(float *ubo, VkDeviceSize uboSize, qvktexture_t *texture);
void		QVk_BindPipeline(qvkpipeline_t *pipeline);
void		QVk_SubmitStagingBuffers(void);
#endif
