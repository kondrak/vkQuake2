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
** VK_COMMON.C
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
#ifdef _WIN32
#include "../win32/vk_win.h"
#endif
#ifdef __linux__
#include "../linux/vk_linux.h"
#endif
#ifdef __APPLE__
#include "../macos/vk_macos.h"
#endif

FILE *vk_logfp = NULL;

// Vulkan instance, surface and memory allocator
VkInstance	 vk_instance = VK_NULL_HANDLE;
VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
VmaAllocator vk_malloc  = VK_NULL_HANDLE;

// Vulkan device
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

// Vulkan swapchain
qvkswapchain_t vk_swapchain = {
	.sc = VK_NULL_HANDLE,
	.format = VK_FORMAT_UNDEFINED,
	.presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
	.extent = { 0, 0 },
	.images = NULL,
	.imageCount = 0
};

// Vulkan renderpasses (standard and MSAA)
qvkrenderpass_t vk_renderpasses[RT_COUNT] = { 
	{
		.rp = VK_NULL_HANDLE,
		.colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.sampleCount = VK_SAMPLE_COUNT_1_BIT
	},
	{
		.rp = VK_NULL_HANDLE,
		.colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.sampleCount = VK_SAMPLE_COUNT_4_BIT
	} 
};

// Vulkan pools
VkCommandPool vk_commandPool = VK_NULL_HANDLE;
VkCommandPool vk_transferCommandPool = VK_NULL_HANDLE;
VkDescriptorPool vk_descriptorPool = VK_NULL_HANDLE;
static VkCommandPool vk_stagingCommandPool = VK_NULL_HANDLE;
// Vulkan image views
VkImageView *vk_imageviews = NULL;
// Vulkan framebuffers
VkFramebuffer *vk_framebuffers[RT_COUNT];
// depth buffer
qvktexture_t vk_depthbuffer = QVVKTEXTURE_INIT;
// render targets for MSAA
qvktexture_t vk_msaaColorbuffer = QVVKTEXTURE_INIT;
qvktexture_t vk_msaaDepthbuffer = QVVKTEXTURE_INIT;
// viewport and scissor
VkViewport vk_viewport = { .0f, .0f, .0f, .0f, .0f, .0f };
VkRect2D vk_scissor = { { 0, 0 }, { 0, 0 } };

#define NUM_CMDBUFFERS 2
// Vulkan command buffers
VkCommandBuffer *vk_commandbuffers = NULL;
// command buffer double buffering fences
VkFence vk_fences[NUM_CMDBUFFERS];
// semaphore: signal when next image is available for rendering
VkSemaphore vk_imageAvailableSemaphores[NUM_CMDBUFFERS];
// semaphore: signal when rendering to current command buffer is complete
VkSemaphore vk_renderFinishedSemaphores[NUM_CMDBUFFERS];
// tracker variables
qvkrenderpass_t vk_activeRenderpass;
VkFramebuffer   vk_activeFramebuffer = VK_NULL_HANDLE;
VkCommandBuffer vk_activeCmdbuffer = VK_NULL_HANDLE;
// index of active command buffer
int vk_activeBufferIdx = 0;
// index of currently acquired image
uint32_t vk_imageIndex = 0;
// index of currently used staging buffer
int vk_activeStagingBuffer = 0;
// started rendering frame?
static qboolean vk_frameStarted = false;

// render pipelines
qvkpipeline_t vk_drawTexQuadPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawColorQuadPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawModelPipelineStrip = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawModelPipelineFan = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawNoDepthModelPipelineStrip = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawNoDepthModelPipelineFan = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawLefthandModelPipelineStrip = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawLefthandModelPipelineFan = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawNullModel = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawParticlesPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawPointParticlesPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawSpritePipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawPolyPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawPolyLmapPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawPolyWarpPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawBeamPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawSkyboxPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_drawDLightPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_showTrisPipeline = QVKPIPELINE_INIT;
qvkpipeline_t vk_shadowsPipelineStrip = QVKPIPELINE_INIT;
qvkpipeline_t vk_shadowsPipelineFan = QVKPIPELINE_INIT;

// samplers
static VkSampler vk_samplers[S_SAMPLER_CNT];

PFN_vkCreateDebugUtilsMessengerEXT qvkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT qvkDestroyDebugUtilsMessengerEXT;

#define VK_INPUTBIND_DESC(s) { \
	.binding = 0, \
	.stride = s, \
	.inputRate = VK_VERTEX_INPUT_RATE_VERTEX \
};

#define VK_INPUTATTR_DESC(l, f, o) { \
	.binding = 0, \
	.location = l, \
	.format = f, \
	.offset = o \
}

#define VK_VERTEXINPUT_CINF(b, a) { \
	.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, \
	.pNext = NULL, \
	.flags = 0, \
	.vertexBindingDescriptionCount = 1, \
	.pVertexBindingDescriptions = &b, \
	.vertexAttributeDescriptionCount = sizeof(a) / sizeof(a[0]), \
	.pVertexAttributeDescriptions = a \
}

#define VK_VERTINFO(name, bindSize, ...) \
	VkVertexInputAttributeDescription attrDesc##name[] = { __VA_ARGS__ }; \
	VkVertexInputBindingDescription name##bindingDesc = VK_INPUTBIND_DESC(bindSize); \
	VkPipelineVertexInputStateCreateInfo vertInfo##name = VK_VERTEXINPUT_CINF(name##bindingDesc, attrDesc##name);

#define VK_LOAD_VERTFRAG_SHADERS(shaders, namevert, namefrag) \
	vkDestroyShaderModule(vk_device.logical, shaders[0].module, NULL); \
	vkDestroyShaderModule(vk_device.logical, shaders[1].module, NULL); \
	shaders[0] = QVk_CreateShader(namevert##_vert_spv, namevert##_vert_size, VK_SHADER_STAGE_VERTEX_BIT); \
	shaders[1] = QVk_CreateShader(namefrag##_frag_spv, namefrag##_frag_size, VK_SHADER_STAGE_FRAGMENT_BIT);

// global static buffers (reused, never changing)
qvkbuffer_t vk_texRectVbo;
qvkbuffer_t vk_colorRectVbo;
qvkbuffer_t vk_rectIbo;
qvkbuffer_t vk_triangleFanIbo;

// global dynamic buffers (double buffered)
#define NUM_DYNBUFFERS 2
static qvkbuffer_t vk_dynVertexBuffers[NUM_DYNBUFFERS];
static qvkbuffer_t vk_dynIndexBuffers[NUM_DYNBUFFERS]; // index buffers are unused right now
static qvkbuffer_t vk_dynUniformBuffers[NUM_DYNBUFFERS];
static VkDescriptorSet vk_uboDescriptorSets[NUM_DYNBUFFERS];
static qvkstagingbuffer_t vk_stagingBuffers[NUM_DYNBUFFERS];
static int vk_activeDynBufferIdx = 0;

// swap buffers used if primary dynamic buffers get full
static qvkbuffer_t vk_swapVertexBuffers[NUM_DYNBUFFERS];
static qvkbuffer_t vk_swapIndexBuffers[NUM_DYNBUFFERS]; // index buffers are unused right now
static qvkbuffer_t vk_swapUniformBuffers[NUM_DYNBUFFERS];
static VkDescriptorSet vk_swapDescriptorSets[NUM_DYNBUFFERS];

// by how much will the dynamic buffers be resized if we run out of space?
#define BUFFER_RESIZE_FACTOR 2.f
#define UNIFORM_ALLOC_SIZE 1024
// start values for dynamic buffer sizes - bound to change if the application runs out of space (size in kB)
#define VERTEX_BUFFER_SIZE (512 * 1024)
#define INDEX_BUFFER_SIZE (8 * 1024)
#define UNIFORM_BUFFER_SIZE (1024 * 1024)
// staging buffer is constant in size but has a max limit beyond which it will be submitted
#define STAGING_BUFFER_MAXSIZE (8192 * 1024)
// index count in triangle fan buffer (assuming max 84 triangles per object)
#define TRIANGLE_FAN_IBO_MAXSIZE 252

// Vulkan common descriptor sets for UBO, primary texture sampler and optional lightmap texture
VkDescriptorSetLayout vk_uboDescSetLayout;
VkDescriptorSetLayout vk_samplerDescSetLayout;
VkDescriptorSetLayout vk_samplerLightmapDescSetLayout;

extern cvar_t *vk_msaa;

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

// internal helper
static VkSampleCountFlagBits GetSampleCount()
{
	static VkSampleCountFlagBits msaaModes[] = {
		VK_SAMPLE_COUNT_1_BIT,
		VK_SAMPLE_COUNT_2_BIT,
		VK_SAMPLE_COUNT_4_BIT,
		VK_SAMPLE_COUNT_8_BIT,
		VK_SAMPLE_COUNT_16_BIT
	};

	return msaaModes[(int)vk_msaa->value];
}

// internal helper
static void DestroyImageViews()
{
	if(!vk_imageviews)
		return;

	for (int i = 0; i < vk_swapchain.imageCount; i++)
	{
		vkDestroyImageView(vk_device.logical, vk_imageviews[i], NULL);
	}
	free(vk_imageviews);
	vk_imageviews = NULL;
}

// internal helper
static VkResult CreateImageViews()
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

// internal helper
static void DestroyFramebuffers()
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

// internal helper
static VkResult CreateFramebuffers(const qvkrenderpass_t *renderpass, qvkrendertype_t framebufferType)
{
	VkResult res = VK_SUCCESS;
	vk_framebuffers[framebufferType] = (VkFramebuffer *)malloc(vk_swapchain.imageCount * sizeof(VkFramebuffer));

	VkFramebufferCreateInfo fbCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
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

// internal helper
static void CreateDrawBuffers()
{
	QVk_CreateDepthBuffer(vk_renderpasses[RT_STANDARD].sampleCount, &vk_depthbuffer);
	ri.Con_Printf(PRINT_ALL, "...created depth buffer\n");
	QVk_CreateDepthBuffer(vk_renderpasses[RT_MSAA].sampleCount, &vk_msaaDepthbuffer);
	ri.Con_Printf(PRINT_ALL, "...created MSAAx%d depth buffer\n", vk_renderpasses[1].sampleCount);
	QVk_CreateColorBuffer(vk_renderpasses[RT_MSAA].sampleCount, &vk_msaaColorbuffer);
	ri.Con_Printf(PRINT_ALL, "...created MSAAx%d color buffer\n", vk_renderpasses[1].sampleCount);
}

// internal helper
static void DestroyDrawBuffer(qvktexture_t *drawBuffer)
{
	if (drawBuffer->image != VK_NULL_HANDLE)
	{
		vmaDestroyImage(vk_malloc, drawBuffer->image, drawBuffer->allocation);
		vkDestroyImageView(vk_device.logical, drawBuffer->imageView, NULL);
		drawBuffer->image = VK_NULL_HANDLE;
		drawBuffer->imageView = VK_NULL_HANDLE;
	}
}

// internal helper
static void DestroyDrawBuffers()
{
	DestroyDrawBuffer(&vk_depthbuffer);
	DestroyDrawBuffer(&vk_msaaDepthbuffer);
	DestroyDrawBuffer(&vk_msaaColorbuffer);
}

// internal helper
static void CreateDescriptorSetLayouts()
{
	VkDescriptorSetLayoutBinding layoutBinding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = NULL
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.bindingCount = 1,
		.pBindings = &layoutBinding
	};

	// uniform buffer object layout
	VK_VERIFY(vkCreateDescriptorSetLayout(vk_device.logical, &layoutInfo, NULL, &vk_uboDescSetLayout));

	// sampler layout
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	VK_VERIFY(vkCreateDescriptorSetLayout(vk_device.logical, &layoutInfo, NULL, &vk_samplerDescSetLayout));
	// secondary sampler: lightmaps
	VK_VERIFY(vkCreateDescriptorSetLayout(vk_device.logical, &layoutInfo, NULL, &vk_samplerLightmapDescSetLayout));
}

// internal helper
static void CreateSamplers()
{
	VkSamplerCreateInfo samplerInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.magFilter = VK_FILTER_NEAREST,
		.minFilter = VK_FILTER_NEAREST,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.f,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 1.f,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.f,
		.maxLod = 1.f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};

	VK_VERIFY(vkCreateSampler(vk_device.logical, &samplerInfo, NULL, &vk_samplers[S_NEAREST]));

	samplerInfo.maxLod = FLT_MAX;
	VK_VERIFY(vkCreateSampler(vk_device.logical, &samplerInfo, NULL, &vk_samplers[S_MIPMAP_NEAREST]));

	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	VK_VERIFY(vkCreateSampler(vk_device.logical, &samplerInfo, NULL, &vk_samplers[S_MIPMAP_LINEAR]));

	samplerInfo.maxLod = 1.f;
	VK_VERIFY(vkCreateSampler(vk_device.logical, &samplerInfo, NULL, &vk_samplers[S_LINEAR]));

	// aniso samplers
	assert((vk_device.properties.limits.maxSamplerAnisotropy > 1.f) && "maxSamplerAnisotropy is 1");

	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = vk_device.properties.limits.maxSamplerAnisotropy;
	samplerInfo.maxLod = 1.f;

	VK_VERIFY(vkCreateSampler(vk_device.logical, &samplerInfo, NULL, &vk_samplers[S_ANISO_NEAREST]));

	samplerInfo.maxLod = FLT_MAX;
	VK_VERIFY(vkCreateSampler(vk_device.logical, &samplerInfo, NULL, &vk_samplers[S_ANISO_MIPMAP_NEAREST]));

	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	VK_VERIFY(vkCreateSampler(vk_device.logical, &samplerInfo, NULL, &vk_samplers[S_ANISO_MIPMAP_LINEAR]));

	samplerInfo.maxLod = 1.f;
	VK_VERIFY(vkCreateSampler(vk_device.logical, &samplerInfo, NULL, &vk_samplers[S_ANISO_LINEAR]));
}

// internal helper
static void DestroySamplers()
{
	int i;
	for (i = 0; i < S_SAMPLER_CNT; ++i)
	{
		if (vk_samplers[i] != VK_NULL_HANDLE)
			vkDestroySampler(vk_device.logical, vk_samplers[i], NULL);

		vk_samplers[i] = VK_NULL_HANDLE;
	}
}

// internal helper
static void CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSizes[] = {
		// UBO
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			.descriptorCount = 16
		},
		// sampler
		{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = MAX_VKTEXTURES + 1
		}
	};

	VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = MAX_VKTEXTURES + 32,
		.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]),
		.pPoolSizes = poolSizes,
	};

	VK_VERIFY(vkCreateDescriptorPool(vk_device.logical, &poolInfo, NULL, &vk_descriptorPool));
}

// internal helper
static void CreateDynamicBuffers()
{
	for (int i = 0; i < NUM_DYNBUFFERS; ++i)
	{
		QVk_CreateVertexBuffer(NULL, vk_config.vertex_buffer_size, &vk_dynVertexBuffers[i], NULL, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
		QVk_CreateIndexBuffer(NULL, vk_config.index_buffer_size, &vk_dynIndexBuffers[i], NULL, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
		QVk_CreateUniformBuffer(vk_config.uniform_buffer_size, &vk_dynUniformBuffers[i], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
		// keep dynamic buffers persistently mapped
		vmaMapMemory(vk_malloc, vk_dynVertexBuffers[i].allocation, &vk_dynVertexBuffers[i].allocInfo.pMappedData);
		vmaMapMemory(vk_malloc, vk_dynIndexBuffers[i].allocation, &vk_dynIndexBuffers[i].allocInfo.pMappedData);
		vmaMapMemory(vk_malloc, vk_dynUniformBuffers[i].allocation, &vk_dynUniformBuffers[i].allocInfo.pMappedData);
		// create descriptor set
		VkDescriptorSetAllocateInfo dsAllocInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = NULL,
			.descriptorPool = vk_descriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &vk_uboDescSetLayout
		};

		VK_VERIFY(vkAllocateDescriptorSets(vk_device.logical, &dsAllocInfo, &vk_uboDescriptorSets[i]));

		VkDescriptorBufferInfo bufferInfo = {
			.buffer = vk_dynUniformBuffers[i].buffer,
			.offset = 0,
			.range = UNIFORM_ALLOC_SIZE
		};

		VkWriteDescriptorSet descriptorWrite = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = NULL,
			.dstSet = vk_uboDescriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			.pImageInfo = NULL,
			.pBufferInfo = &bufferInfo,
			.pTexelBufferView = NULL,
		};

		vkUpdateDescriptorSets(vk_device.logical, 1, &descriptorWrite, 0, NULL);
	}
}

// internal helper
static void SwapFullBuffers()
{
	if (vk_dynVertexBuffers[vk_activeDynBufferIdx].full ||
		vk_dynUniformBuffers[vk_activeDynBufferIdx].full ||
		vk_dynIndexBuffers[vk_activeDynBufferIdx].full)
	{
		vkDeviceWaitIdle(vk_device.logical);

#define SWAPBUF(dynBuffer, swapBuffer, buffName, buffSize, freeDescSet) \
		if (dynBuffer[vk_activeDynBufferIdx].full) \
		{ \
			ri.Con_Printf(PRINT_ALL, "Resizing dynamic "buffName" buffer to %ukB\n", buffSize / 1024); \
			for (int i = 0; i < NUM_DYNBUFFERS; i++) \
			{ \
				vmaUnmapMemory(vk_malloc, dynBuffer[i].allocation); \
				QVk_FreeBuffer(&dynBuffer[i]); \
				dynBuffer[i] = swapBuffer[i]; \
				/* free descriptor sets associated with the (uniform) buffer */\
				if (freeDescSet) \
				{ \
					vkFreeDescriptorSets(vk_device.logical, vk_descriptorPool, 1, &vk_uboDescriptorSets[i]); \
					vk_uboDescriptorSets[i] = vk_swapDescriptorSets[i]; \
				} \
			} \
		}

		SWAPBUF(vk_dynVertexBuffers, vk_swapVertexBuffers, "vertex", vk_config.vertex_buffer_size, 0);
		SWAPBUF(vk_dynIndexBuffers, vk_swapIndexBuffers, "index", vk_config.index_buffer_size, 0);
		SWAPBUF(vk_dynUniformBuffers, vk_swapUniformBuffers, "uniform", vk_config.uniform_buffer_size, 1);
	}
}

// internal helper
static int NextPow2(int v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

// internal helper
static void CreateTriangleFanIndexBuffer()
{
	VkDeviceSize bufferSize = TRIANGLE_FAN_IBO_MAXSIZE * sizeof(uint16_t);
	VkBuffer stagingBuffer;
	VkCommandBuffer cmdBuffer;
	uint32_t stagingOffset;
	int idx = 0;
	uint16_t *stagingMemory = (uint16_t *)QVk_GetStagingBuffer(bufferSize, 1, &cmdBuffer, &stagingBuffer, &stagingOffset);

	// fill the index buffer so that we can emulate triangle fans via triangle lists
	for (int i = 0; i < TRIANGLE_FAN_IBO_MAXSIZE / 3; ++i)
	{
		stagingMemory[idx++] = 0;
		stagingMemory[idx++] = i + 1;
		stagingMemory[idx++] = i + 2;
	}

	QVk_CreateIndexBuffer(stagingMemory, bufferSize, &vk_triangleFanIbo, NULL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
}

// internal helper
static void CreateStagingBuffers()
{
	QVk_CreateCommandPool(&vk_stagingCommandPool, vk_device.gfxFamilyIndex);

	VkFenceCreateInfo fCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = 0
	};

	for (int i = 0; i < NUM_DYNBUFFERS; ++i)
	{
		QVk_CreateStagingBuffer(STAGING_BUFFER_MAXSIZE, &vk_stagingBuffers[i].buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
		vmaMapMemory(vk_malloc, vk_stagingBuffers[i].buffer.allocation, &vk_stagingBuffers[i].buffer.allocInfo.pMappedData);
		vk_stagingBuffers[i].submitted = false;

		VK_VERIFY(vkCreateFence(vk_device.logical, &fCreateInfo, NULL, &vk_stagingBuffers[i].fence));

		vk_stagingBuffers[i].cmdBuffer = QVk_CreateCommandBuffer(&vk_stagingCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VK_VERIFY(QVk_BeginCommand(&vk_stagingBuffers[i].cmdBuffer));
	}
}

// internal helper
static void SubmitStagingBuffer(int index)
{
	VkMemoryBarrier memory_barrier;
	memset(&memory_barrier, 0, sizeof(memory_barrier));
	memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	memory_barrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	vkCmdPipelineBarrier(vk_stagingBuffers[index].cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, &memory_barrier, 0, NULL, 0, NULL);

	vkEndCommandBuffer(vk_stagingBuffers[index].cmdBuffer);

	VkSubmitInfo submit_info;
	memset(&submit_info, 0, sizeof(submit_info));
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &vk_stagingBuffers[index].cmdBuffer;

	vkQueueSubmit(vk_device.gfxQueue, 1, &submit_info, vk_stagingBuffers[index].fence);

	vk_stagingBuffers[index].submitted = true;
	vk_activeStagingBuffer = (vk_activeStagingBuffer + 1) % NUM_DYNBUFFERS;
}

// internal helper
static void CreateStaticBuffers()
{
	const float texVerts[] = {	-1., -1., 0., 0.,
								 1.,  1., 1., 1.,
								-1.,  1., 0., 1.,
								 1., -1., 1., 0. };

	const float colorVerts[] = { -1., -1.,
								  1.,  1.,
								 -1.,  1.,
								  1., -1. };

	const uint32_t indices[] = { 0, 1, 2, 0, 3, 1 };

	QVk_CreateVertexBuffer(texVerts, sizeof(texVerts), &vk_texRectVbo, NULL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	QVk_CreateVertexBuffer(colorVerts, sizeof(colorVerts), &vk_colorRectVbo, NULL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
	QVk_CreateIndexBuffer(indices, sizeof(indices), &vk_rectIbo, NULL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
}

// internal helper
static void CreatePipelines()
{
	// shared pipeline vertex input state create infos
	VK_VERTINFO(RG, sizeof(float) * 2, VK_INPUTATTR_DESC(0, VK_FORMAT_R32G32_SFLOAT, 0));

	VK_VERTINFO(RGB, sizeof(float) * 3, VK_INPUTATTR_DESC(0, VK_FORMAT_R32G32B32_SFLOAT, 0));

	VK_VERTINFO(RG_RG, sizeof(float) * 4,	VK_INPUTATTR_DESC(0, VK_FORMAT_R32G32_SFLOAT, 0),
											VK_INPUTATTR_DESC(1, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 2));

	VK_VERTINFO(RGB_RG, sizeof(float) * 5,	VK_INPUTATTR_DESC(0, VK_FORMAT_R32G32B32_SFLOAT, 0), 
											VK_INPUTATTR_DESC(1, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3));

	VK_VERTINFO(RGB_RGB, sizeof(float) * 6,	VK_INPUTATTR_DESC(0, VK_FORMAT_R32G32B32_SFLOAT, 0),
											VK_INPUTATTR_DESC(1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3));

	VK_VERTINFO(RGB_RGBA,  sizeof(float) * 7,	VK_INPUTATTR_DESC(0, VK_FORMAT_R32G32B32_SFLOAT, 0),
												VK_INPUTATTR_DESC(1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 3));

	VK_VERTINFO(RGB_RG_RG, sizeof(float) * 7,	VK_INPUTATTR_DESC(0, VK_FORMAT_R32G32B32_SFLOAT, 0),
												VK_INPUTATTR_DESC(1, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 3),
												VK_INPUTATTR_DESC(2, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 5));

	VK_VERTINFO(RGB_RGBA_RG, sizeof(float) * 9,	VK_INPUTATTR_DESC(0, VK_FORMAT_R32G32B32_SFLOAT, 0),
												VK_INPUTATTR_DESC(1, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(float) * 3),
												VK_INPUTATTR_DESC(2, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 7));

	// shared descriptor set layouts
	VkDescriptorSetLayout samplerUboDsLayouts[] = { vk_samplerDescSetLayout, vk_uboDescSetLayout };
	VkDescriptorSetLayout samplerUboLmapDsLayouts[] = { vk_samplerDescSetLayout, vk_uboDescSetLayout, vk_samplerLightmapDescSetLayout };

	// shader array (vertex and fragment, no compute... yet)
	qvkshader_t shaders[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };

	// we'll be using some push constants in vertex shaders
	// size accomodates for maximum number of uploaded elements (should probably be checked against the hardware's maximum supported value)
	VkPushConstantRange pushConstantRange = {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0,
		.size = 32 * sizeof(float)
	};

	// textured quad pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, basic, basic);
	vk_drawTexQuadPipeline.depthTestEnable = VK_FALSE;
	QVk_CreatePipeline(samplerUboDsLayouts, 2, &vertInfoRG_RG, &vk_drawTexQuadPipeline, shaders, 2, &pushConstantRange);

	// draw particles pipeline (using a texture)
	VK_LOAD_VERTFRAG_SHADERS(shaders, particle, basic);
	vk_drawParticlesPipeline.depthWriteEnable = VK_FALSE;
	vk_drawParticlesPipeline.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(&vk_samplerDescSetLayout, 1, &vertInfoRGB_RGBA_RG, &vk_drawParticlesPipeline, shaders, 2, &pushConstantRange);

	// draw particles pipeline (using point list)
	VK_LOAD_VERTFRAG_SHADERS(shaders, point_particle, point_particle);
	vk_drawPointParticlesPipeline.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	vk_drawPointParticlesPipeline.depthWriteEnable = VK_FALSE;
	vk_drawPointParticlesPipeline.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(&vk_uboDescSetLayout, 1, &vertInfoRGB_RGBA, &vk_drawPointParticlesPipeline, shaders, 2, &pushConstantRange);

	// colored quad pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, basic_color_quad, basic_color_quad);
	vk_drawColorQuadPipeline.depthTestEnable = VK_FALSE;
	vk_drawColorQuadPipeline.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(&vk_uboDescSetLayout, 1, &vertInfoRG, &vk_drawColorQuadPipeline, shaders, 2, &pushConstantRange);

	// untextured null model
	VK_LOAD_VERTFRAG_SHADERS(shaders, nullmodel, basic_color_quad);
	vk_drawNullModel.cullMode = VK_CULL_MODE_NONE;
	vk_drawNullModel.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	QVk_CreatePipeline(&vk_uboDescSetLayout, 1, &vertInfoRGB_RGB, &vk_drawNullModel, shaders, 2, &pushConstantRange);

	// textured model
	VK_LOAD_VERTFRAG_SHADERS(shaders, model, model);
	vk_drawModelPipelineStrip.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	vk_drawModelPipelineStrip.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(samplerUboDsLayouts, 2, &vertInfoRGB_RGBA_RG, &vk_drawModelPipelineStrip, shaders, 2, &pushConstantRange);

	vk_drawModelPipelineFan.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	vk_drawModelPipelineFan.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(samplerUboDsLayouts, 2, &vertInfoRGB_RGBA_RG, &vk_drawModelPipelineFan, shaders, 2, &pushConstantRange);

	// dedicated model pipelines for translucent objects with depth write disabled
	vk_drawNoDepthModelPipelineStrip.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	vk_drawNoDepthModelPipelineStrip.depthWriteEnable = VK_FALSE;
	vk_drawNoDepthModelPipelineStrip.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(samplerUboDsLayouts, 2, &vertInfoRGB_RGBA_RG, &vk_drawNoDepthModelPipelineStrip, shaders, 2, &pushConstantRange);

	vk_drawNoDepthModelPipelineFan.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	vk_drawNoDepthModelPipelineFan.depthWriteEnable = VK_FALSE;
	vk_drawNoDepthModelPipelineFan.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(samplerUboDsLayouts, 2, &vertInfoRGB_RGBA_RG, &vk_drawNoDepthModelPipelineFan, shaders, 2, &pushConstantRange);

	// dedicated model pipelines for when left-handed weapon model is drawn
	vk_drawLefthandModelPipelineStrip.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	vk_drawLefthandModelPipelineStrip.cullMode = VK_CULL_MODE_FRONT_BIT;
	QVk_CreatePipeline(samplerUboDsLayouts, 2, &vertInfoRGB_RGBA_RG, &vk_drawLefthandModelPipelineStrip, shaders, 2, &pushConstantRange);

	vk_drawLefthandModelPipelineFan.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	vk_drawLefthandModelPipelineFan.cullMode = VK_CULL_MODE_FRONT_BIT;
	QVk_CreatePipeline(samplerUboDsLayouts, 2, &vertInfoRGB_RGBA_RG, &vk_drawLefthandModelPipelineFan, shaders, 2, &pushConstantRange);

	// draw sprite pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, sprite, basic);
	vk_drawSpritePipeline.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(&vk_samplerDescSetLayout, 1, &vertInfoRGB_RG, &vk_drawSpritePipeline, shaders, 2, &pushConstantRange);

	// draw polygon pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, polygon, basic);
	vk_drawPolyPipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	vk_drawPolyPipeline.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(samplerUboDsLayouts, 2, &vertInfoRGB_RG, &vk_drawPolyPipeline, shaders, 2, &pushConstantRange);

	// draw lightmapped polygon
	VK_LOAD_VERTFRAG_SHADERS(shaders, polygon_lmap, polygon_lmap);
	vk_drawPolyLmapPipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	QVk_CreatePipeline(samplerUboLmapDsLayouts, 3, &vertInfoRGB_RG_RG, &vk_drawPolyLmapPipeline, shaders, 2, &pushConstantRange);

	// draw polygon with warp effect (liquid) pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, polygon_warp, basic);
	vk_drawPolyWarpPipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	vk_drawPolyWarpPipeline.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(samplerUboLmapDsLayouts, 2, &vertInfoRGB_RG, &vk_drawPolyWarpPipeline, shaders, 2, &pushConstantRange);

	// draw beam pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, beam, basic_color_quad);
	vk_drawBeamPipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	vk_drawBeamPipeline.depthWriteEnable = VK_FALSE;
	vk_drawBeamPipeline.blendOpts.blendEnable = VK_TRUE;
	QVk_CreatePipeline(&vk_uboDescSetLayout, 1, &vertInfoRGB, &vk_drawBeamPipeline, shaders, 2, &pushConstantRange);

	// draw skybox pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, skybox, basic);
	QVk_CreatePipeline(samplerUboDsLayouts, 2, &vertInfoRGB_RG, &vk_drawSkyboxPipeline, shaders, 2, &pushConstantRange);

	// draw dynamic light pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, d_light, basic_color_quad);
	vk_drawDLightPipeline.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	vk_drawDLightPipeline.depthWriteEnable = VK_FALSE;
	vk_drawDLightPipeline.cullMode = VK_CULL_MODE_FRONT_BIT;
	vk_drawDLightPipeline.blendOpts.blendEnable = VK_TRUE;
	vk_drawDLightPipeline.blendOpts.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	vk_drawDLightPipeline.blendOpts.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	vk_drawDLightPipeline.blendOpts.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	vk_drawDLightPipeline.blendOpts.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	QVk_CreatePipeline(&vk_uboDescSetLayout, 1, &vertInfoRGB_RGB, &vk_drawDLightPipeline, shaders, 2, &pushConstantRange);

	// vk_showtris render pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, d_light, basic_color_quad);
	vk_showTrisPipeline.cullMode = VK_CULL_MODE_NONE;
	vk_showTrisPipeline.depthTestEnable = VK_FALSE;
	vk_showTrisPipeline.depthWriteEnable = VK_FALSE;
	vk_showTrisPipeline.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	QVk_CreatePipeline(&vk_uboDescSetLayout, 1, &vertInfoRGB_RGB, &vk_showTrisPipeline, shaders, 2, &pushConstantRange);

	//vk_shadows render pipeline
	VK_LOAD_VERTFRAG_SHADERS(shaders, shadows, basic_color_quad);
	vk_shadowsPipelineStrip.blendOpts.blendEnable = VK_TRUE;
	vk_shadowsPipelineStrip.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	QVk_CreatePipeline(&vk_uboDescSetLayout, 1, &vertInfoRGB, &vk_shadowsPipelineStrip, shaders, 2, &pushConstantRange);

	vk_shadowsPipelineFan.blendOpts.blendEnable = VK_TRUE;
	vk_shadowsPipelineFan.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	QVk_CreatePipeline(&vk_uboDescSetLayout, 1, &vertInfoRGB, &vk_shadowsPipelineFan, shaders, 2, &pushConstantRange);

	// final shader cleanup
	vkDestroyShaderModule(vk_device.logical, shaders[0].module, NULL);
	vkDestroyShaderModule(vk_device.logical, shaders[1].module, NULL);
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

		QVk_DestroyPipeline(&vk_drawTexQuadPipeline);
		QVk_DestroyPipeline(&vk_drawColorQuadPipeline);
		QVk_DestroyPipeline(&vk_drawNullModel);
		QVk_DestroyPipeline(&vk_drawModelPipelineStrip);
		QVk_DestroyPipeline(&vk_drawModelPipelineFan);
		QVk_DestroyPipeline(&vk_drawNoDepthModelPipelineStrip);
		QVk_DestroyPipeline(&vk_drawNoDepthModelPipelineFan);
		QVk_DestroyPipeline(&vk_drawLefthandModelPipelineStrip);
		QVk_DestroyPipeline(&vk_drawLefthandModelPipelineFan);
		QVk_DestroyPipeline(&vk_drawParticlesPipeline);
		QVk_DestroyPipeline(&vk_drawPointParticlesPipeline);
		QVk_DestroyPipeline(&vk_drawSpritePipeline);
		QVk_DestroyPipeline(&vk_drawPolyPipeline);
		QVk_DestroyPipeline(&vk_drawPolyLmapPipeline);
		QVk_DestroyPipeline(&vk_drawPolyWarpPipeline);
		QVk_DestroyPipeline(&vk_drawBeamPipeline);
		QVk_DestroyPipeline(&vk_drawSkyboxPipeline);
		QVk_DestroyPipeline(&vk_drawDLightPipeline);
		QVk_DestroyPipeline(&vk_showTrisPipeline);
		QVk_DestroyPipeline(&vk_shadowsPipelineStrip);
		QVk_DestroyPipeline(&vk_shadowsPipelineFan);
		QVk_FreeBuffer(&vk_texRectVbo);
		QVk_FreeBuffer(&vk_colorRectVbo);
		QVk_FreeBuffer(&vk_rectIbo);
		QVk_FreeBuffer(&vk_triangleFanIbo);
		for (int i = 0; i < NUM_DYNBUFFERS; ++i)
		{
			if(vk_dynUniformBuffers[i].buffer != VK_NULL_HANDLE)
			{
				vmaUnmapMemory(vk_malloc, vk_dynUniformBuffers[i].allocation);
				QVk_FreeBuffer(&vk_dynUniformBuffers[i]);
			}
			if(vk_dynIndexBuffers[i].buffer != VK_NULL_HANDLE)
			{
				vmaUnmapMemory(vk_malloc, vk_dynIndexBuffers[i].allocation);
				QVk_FreeBuffer(&vk_dynIndexBuffers[i]);
			}
			if(vk_dynVertexBuffers[i].buffer != VK_NULL_HANDLE)
			{
				vmaUnmapMemory(vk_malloc, vk_dynVertexBuffers[i].allocation);
				QVk_FreeBuffer(&vk_dynVertexBuffers[i]);
			}
			if(vk_stagingBuffers[i].buffer.buffer != VK_NULL_HANDLE)
			{
				vmaUnmapMemory(vk_malloc, vk_stagingBuffers[i].buffer.allocation);
				QVk_FreeBuffer(&vk_stagingBuffers[i].buffer);
				vkDestroyFence(vk_device.logical, vk_stagingBuffers[i].fence, NULL);
			}
		}
		if(vk_descriptorPool != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(vk_device.logical, vk_descriptorPool, NULL);
		if(vk_uboDescSetLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(vk_device.logical, vk_uboDescSetLayout, NULL);
		if(vk_samplerDescSetLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(vk_device.logical, vk_samplerDescSetLayout, NULL);
		if(vk_samplerLightmapDescSetLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(vk_device.logical, vk_samplerLightmapDescSetLayout, NULL);
		for (int i = 0; i < RT_COUNT; i++)
		{
			if (vk_renderpasses[i].rp != VK_NULL_HANDLE)
				vkDestroyRenderPass(vk_device.logical, vk_renderpasses[i].rp, NULL);
			vk_renderpasses[i].rp = VK_NULL_HANDLE;
		}
		if (vk_commandbuffers)
		{
			vkFreeCommandBuffers(vk_device.logical, vk_commandPool, NUM_CMDBUFFERS, vk_commandbuffers);
			free(vk_commandbuffers);
			vk_commandbuffers = NULL;
		}
		if (vk_commandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(vk_device.logical, vk_commandPool, NULL);
		if (vk_transferCommandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(vk_device.logical, vk_transferCommandPool, NULL);
		if(vk_stagingCommandPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(vk_device.logical, vk_stagingCommandPool, NULL);
		DestroySamplers();
		DestroyFramebuffers();
		DestroyImageViews();
		DestroyDrawBuffers();
		if (vk_swapchain.sc != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(vk_device.logical, vk_swapchain.sc, NULL);
			free(vk_swapchain.images);
			vk_swapchain.sc = VK_NULL_HANDLE;
			vk_swapchain.images = NULL;
			vk_swapchain.imageCount = 0;
		}
		for (int i = 0; i < NUM_CMDBUFFERS; ++i)
		{
			vkDestroySemaphore(vk_device.logical, vk_imageAvailableSemaphores[i], NULL);
			vkDestroySemaphore(vk_device.logical, vk_renderFinishedSemaphores[i], NULL);
			vkDestroyFence(vk_device.logical, vk_fences[i], NULL);
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
		vk_activeFramebuffer = VK_NULL_HANDLE;
		vk_activeCmdbuffer = VK_NULL_HANDLE;
		vk_descriptorPool = VK_NULL_HANDLE;
		vk_uboDescSetLayout = VK_NULL_HANDLE;
		vk_samplerDescSetLayout = VK_NULL_HANDLE;
		vk_samplerLightmapDescSetLayout = VK_NULL_HANDLE;
		vk_commandPool = VK_NULL_HANDLE;
		vk_transferCommandPool = VK_NULL_HANDLE;
		vk_stagingCommandPool = VK_NULL_HANDLE;
		vk_activeBufferIdx = 0;
		vk_imageIndex = 0;
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
	PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion = (PFN_vkEnumerateInstanceVersion)vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceVersion");
	uint32_t instanceVersion = VK_API_VERSION_1_0;

	if (vkEnumerateInstanceVersion)
	{
		VK_VERIFY(vkEnumerateInstanceVersion(&instanceVersion));
	}

	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = "Quake 2",
		.applicationVersion = VK_MAKE_VERSION(3, 21, 0),
		.pEngineName = "id Tech 2",
		.engineVersion = VK_MAKE_VERSION(2, 0, 0),
		.apiVersion = instanceVersion
	};

	uint32_t extCount;
	char **wantedExtensions;
	memset((char*)vk_config.supported_present_modes, 0, 256);
	memset((char*)vk_config.extensions, 0, 256);
	memset((char*)vk_config.layers, 0, 256);
	vk_config.vk_version = instanceVersion;
	vk_config.vertex_buffer_usage  = 0;
	vk_config.vertex_buffer_max_usage = 0;
	vk_config.vertex_buffer_size   = VERTEX_BUFFER_SIZE;
	vk_config.index_buffer_usage   = 0;
	vk_config.index_buffer_max_usage = 0;
	vk_config.index_buffer_size    = INDEX_BUFFER_SIZE;
	vk_config.uniform_buffer_usage = 0;
	vk_config.uniform_buffer_max_usage = 0;
	vk_config.uniform_buffer_size  = UNIFORM_BUFFER_SIZE;

	Vkimp_GetSurfaceExtensions(NULL, &extCount);

	if (vk_validation->value)
		extCount++;

	wantedExtensions = (char **)malloc(extCount * sizeof(const char *));
	Vkimp_GetSurfaceExtensions(wantedExtensions, NULL);

	if (vk_validation->value)
		wantedExtensions[extCount - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

	ri.Con_Printf(PRINT_ALL, "Enabled extensions: ");
	for (int i = 0; i < extCount; i++)
	{
		ri.Con_Printf(PRINT_ALL, "%s ", wantedExtensions[i]);
		vk_config.extensions[i] = wantedExtensions[i];
	}
	ri.Con_Printf(PRINT_ALL, "\n");

	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = extCount,
		.ppEnabledExtensionNames = (const char* const*)wantedExtensions
	};

	const char *validationLayers[] = { "VK_LAYER_LUNARG_standard_validation" };
	if (vk_validation->value)
	{
		createInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(validationLayers[0]);
		createInfo.ppEnabledLayerNames = validationLayers;
		for (int i = 0; i < createInfo.enabledLayerCount; i++)
		{
			vk_config.layers[i] = validationLayers[i];
		}
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

	// create Vulkan device - see if the user prefers any specific device if there's more than one GPU in the system
	QVk_CreateDevice((int)vk_device_idx->value);
	// create memory allocator
	VmaAllocatorCreateInfo allocInfo = {
		.flags = 0,
		.physicalDevice = vk_device.physical,
		.device = vk_device.logical,
		.preferredLargeHeapBlockSize = 0,
		.pAllocationCallbacks = NULL,
		.pDeviceMemoryCallbacks = NULL,
		.frameInUseCount = 0,
		.pHeapSizeLimit = NULL,
		.pVulkanFunctions = NULL,
		.pRecordSettings = NULL
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

	// set viewport and scissor
	vk_viewport.x = 0.f;
	vk_viewport.y = 0.f;
	vk_viewport.minDepth = 0.f;
	vk_viewport.maxDepth = 1.f;
	vk_viewport.width = (float)vid.width;
	vk_viewport.height = (float)vid.height;
	vk_scissor.offset.x = 0;
	vk_scissor.offset.y = 0;
	vk_scissor.extent = vk_swapchain.extent;

	// setup fences and semaphores
	VkFenceCreateInfo fCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	VkSemaphoreCreateInfo sCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};
	for (int i = 0; i < NUM_CMDBUFFERS; ++i)
	{
		VK_VERIFY(vkCreateFence(vk_device.logical, &fCreateInfo, NULL, &vk_fences[i]));
		VK_VERIFY(vkCreateSemaphore(vk_device.logical, &sCreateInfo, NULL, &vk_imageAvailableSemaphores[i]));
		VK_VERIFY(vkCreateSemaphore(vk_device.logical, &sCreateInfo, NULL, &vk_renderFinishedSemaphores[i]));
	}
	ri.Con_Printf(PRINT_ALL, "...created synchronization objects\n");

	// setup render passes
	for (int i = 0; i < RT_COUNT; ++i)
	{
		vk_renderpasses[i].colorLoadOp = vk_clear->value ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	}

	res = QVk_CreateRenderpass(&vk_renderpasses[0]);
	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan renderpass: %s\n", QVk_GetError(res));
		return false;
	}
	ri.Con_Printf(PRINT_ALL, "...created Vulkan renderpass\n");
	VkSampleCountFlagBits msaaMode = GetSampleCount();
	VkSampleCountFlagBits supportedMsaa = vk_device.properties.limits.framebufferColorSampleCounts;
	if(!(supportedMsaa & msaaMode))
	{
		ri.Con_Printf(PRINT_ALL, "MSAAx%d mode not supported, aborting...\n", msaaMode);
		ri.Cvar_Set("vk_msaa", "0");
		msaaMode = VK_SAMPLE_COUNT_1_BIT;
		// avoid secondary video reload
		vk_msaa->modified = false;
	}

	if (msaaMode != VK_SAMPLE_COUNT_1_BIT)
		vk_renderpasses[1].sampleCount = msaaMode;
	res = QVk_CreateRenderpass(&vk_renderpasses[1]);
	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan MSAAx%d renderpass: %s\n", QVk_GetError(res), vk_renderpasses[1].sampleCount);
		return false;
	}
	ri.Con_Printf(PRINT_ALL, "...created Vulkan MSAAx%d renderpass\n", vk_renderpasses[1].sampleCount);

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
	ri.Con_Printf(PRINT_ALL, "...created %d Vulkan framebuffers\n", vk_swapchain.imageCount);
	res = CreateFramebuffers(&vk_renderpasses[RT_MSAA], RT_MSAA);
	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan MSAA framebuffers: %s\n", QVk_GetError(res));
		return false;
	}
	ri.Con_Printf(PRINT_ALL, "...created %d Vulkan MSAA framebuffers\n", vk_swapchain.imageCount);

	// setup command buffers (double buffering)
	vk_commandbuffers = (VkCommandBuffer *)malloc(NUM_CMDBUFFERS * sizeof(VkCommandBuffer));

	VkCommandBufferAllocateInfo cbInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = vk_commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = NUM_CMDBUFFERS
	};

	res = vkAllocateCommandBuffers(vk_device.logical, &cbInfo, vk_commandbuffers);
	if (res != VK_SUCCESS)
	{
		ri.Con_Printf(PRINT_ALL, "QVk_Init(): Could not create Vulkan commandbuffers: %s\n", QVk_GetError(res));
		free(vk_commandbuffers);
		vk_commandbuffers = NULL;
		return false;
	}
	ri.Con_Printf(PRINT_ALL, "...created %d Vulkan commandbuffers\n", NUM_CMDBUFFERS);

	// initialize tracker variables
	if (msaaMode != VK_SAMPLE_COUNT_1_BIT)
	{
		vk_activeRenderpass  = vk_renderpasses[RT_MSAA];
		vk_activeFramebuffer = *vk_framebuffers[RT_MSAA];
	}
	else
	{
		vk_activeRenderpass  = vk_renderpasses[RT_STANDARD];
		vk_activeFramebuffer = *vk_framebuffers[RT_STANDARD];
	}
	vk_activeCmdbuffer = vk_commandbuffers[vk_activeBufferIdx];

	CreateDescriptorSetLayouts();
	CreateDescriptorPool();
	// create static vertex/index buffers reused in the games
	CreateStaticBuffers();
	// create vertex, index and uniform buffer pools
	CreateDynamicBuffers();
	// create staging buffers
	CreateStagingBuffers();
	// create index buffer for triangle fan emulation
	CreateTriangleFanIndexBuffer();
	CreatePipelines();
	CreateSamplers();

	return true;
}

VkResult QVk_BeginFrame()
{
	// reset tracking variables
	vk_state.current_pipeline = VK_NULL_HANDLE;
	vk_config.vertex_buffer_usage  = 0;
	vk_config.index_buffer_usage   = 0;
	vk_config.uniform_buffer_usage = 0;

	VkResult result = vkAcquireNextImageKHR(vk_device.logical, vk_swapchain.sc, UINT32_MAX, vk_imageAvailableSemaphores[vk_activeBufferIdx], VK_NULL_HANDLE, &vk_imageIndex);
	vk_activeCmdbuffer = vk_commandbuffers[vk_activeBufferIdx];
	vk_activeFramebuffer = (vk_activeRenderpass.sampleCount == VK_SAMPLE_COUNT_1_BIT) ? vk_framebuffers[RT_STANDARD][vk_imageIndex] : vk_framebuffers[RT_MSAA][vk_imageIndex];

	// swap dynamic buffers
	vk_activeDynBufferIdx = (vk_activeDynBufferIdx + 1) % NUM_DYNBUFFERS;
	vk_dynUniformBuffers[vk_activeDynBufferIdx].currentOffset = 0;
	vk_dynVertexBuffers[vk_activeDynBufferIdx].currentOffset = 0;
	vk_dynIndexBuffers[vk_activeDynBufferIdx].currentOffset = 0;
	vmaInvalidateAllocation(vk_malloc, vk_dynUniformBuffers[vk_activeDynBufferIdx].allocation, 0, VK_WHOLE_SIZE);
	vmaInvalidateAllocation(vk_malloc, vk_dynVertexBuffers[vk_activeDynBufferIdx].allocation, 0, VK_WHOLE_SIZE);
	vmaInvalidateAllocation(vk_malloc, vk_dynIndexBuffers[vk_activeDynBufferIdx].allocation, 0, VK_WHOLE_SIZE);

	// swapchain has become incompatible - need to recreate it
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		ri.Con_Printf(PRINT_ALL, "Vulkan swapchain incompatible after vkAcquireNextImageKHR - rebuilding!\n");
		QVk_RecreateSwapchain();
		return result;
	}

	VK_VERIFY(vkWaitForFences(vk_device.logical, 1, &vk_fences[vk_activeBufferIdx], VK_TRUE, UINT32_MAX));
	VK_VERIFY(vkResetFences(vk_device.logical, 1, &vk_fences[vk_activeBufferIdx]));

	assert((result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR) && "Could not acquire swapchain image!");

	// setup command buffers and render pass for drawing
	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = NULL
	};

	VK_VERIFY(vkBeginCommandBuffer(vk_commandbuffers[vk_activeBufferIdx], &beginInfo));

	VkClearValue clearColors[2] = {
		{ .color = { 1.f, .0f, .5f, 1.f } },
		{ .depthStencil = { 1.f, 0 } }
	};
	VkRenderPassBeginInfo renderBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = vk_activeRenderpass.rp,
		.framebuffer = vk_activeFramebuffer,
		.renderArea.offset = { 0, 0 },
		.renderArea.extent = vk_swapchain.extent,
		.clearValueCount = 2,
		.pClearValues = clearColors
	};

	vkCmdBeginRenderPass(vk_commandbuffers[vk_activeBufferIdx], &renderBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdSetViewport(vk_commandbuffers[vk_activeBufferIdx], 0, 1, &vk_viewport);
	vkCmdSetScissor(vk_commandbuffers[vk_activeBufferIdx], 0, 1, &vk_scissor);

	vk_frameStarted = true;
	return VK_SUCCESS;
}

VkResult QVk_EndFrame()
{
	// continue only if QVk_BeginFrame() had been previously issued
	if (!vk_frameStarted)
		return VK_NOT_READY;

	// submit
	QVk_SubmitStagingBuffers();
	vmaFlushAllocation(vk_malloc, vk_dynUniformBuffers[vk_activeDynBufferIdx].allocation, 0, VK_WHOLE_SIZE);
	vmaFlushAllocation(vk_malloc, vk_dynVertexBuffers[vk_activeDynBufferIdx].allocation, 0, VK_WHOLE_SIZE);
	vmaFlushAllocation(vk_malloc, vk_dynIndexBuffers[vk_activeDynBufferIdx].allocation, 0, VK_WHOLE_SIZE);

	vkCmdEndRenderPass(vk_commandbuffers[vk_activeBufferIdx]);
	VK_VERIFY(vkEndCommandBuffer(vk_commandbuffers[vk_activeBufferIdx]));

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vk_imageAvailableSemaphores[vk_activeBufferIdx],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &vk_renderFinishedSemaphores[vk_activeBufferIdx],
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &vk_commandbuffers[vk_activeBufferIdx]
	};

	VK_VERIFY(vkQueueSubmit(vk_device.gfxQueue, 1, &submitInfo, vk_fences[vk_activeBufferIdx]));

	// present
	VkSwapchainKHR swapChains[] = { vk_swapchain.sc };
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vk_renderFinishedSemaphores[vk_activeBufferIdx],
		.swapchainCount = 1,
		.pSwapchains = swapChains,
		.pImageIndices = &vk_imageIndex,
		.pResults = NULL
	};

	VkResult renderResult = vkQueuePresentKHR(vk_device.presentQueue, &presentInfo);

	// recreate swapchain if it's out of date
	if (renderResult == VK_ERROR_OUT_OF_DATE_KHR || renderResult == VK_SUBOPTIMAL_KHR)
	{
		ri.Con_Printf(PRINT_ALL, "Vulkan swapchain out of date/suboptimal after vkQueuePresentKHR - rebuilding!\n");
		QVk_RecreateSwapchain();
	}

	vk_activeBufferIdx = (vk_activeBufferIdx + 1) % NUM_CMDBUFFERS;

	// release full dynamic buffers and start using resized ones, if applicable
	SwapFullBuffers();

	vk_frameStarted = false;
	return renderResult;
}

void QVk_RecreateSwapchain()
{
	vkDeviceWaitIdle( vk_device.logical );
	DestroyFramebuffers();
	DestroyImageViews();
	VK_VERIFY( QVk_CreateSwapchain() );
	vk_viewport.width = (float)vid.width;
	vk_viewport.height = (float)vid.height;
	vk_scissor.extent = vk_swapchain.extent;
	DestroyDrawBuffers();
	CreateDrawBuffers();
	VK_VERIFY( CreateImageViews() );
	VK_VERIFY( CreateFramebuffers( &vk_renderpasses[RT_STANDARD], RT_STANDARD ) );
	VK_VERIFY( CreateFramebuffers( &vk_renderpasses[RT_MSAA], RT_MSAA ) );
}

uint8_t *QVk_GetVertexBuffer(VkDeviceSize size, VkBuffer *dstBuffer, VkDeviceSize *dstOffset)
{
	qvkbuffer_t *currentBuffer = &vk_dynVertexBuffers[vk_activeDynBufferIdx];

	if (currentBuffer->full)
		currentBuffer = &vk_swapVertexBuffers[vk_activeDynBufferIdx];

	if (currentBuffer->currentOffset + size > vk_config.vertex_buffer_size)
	{
		currentBuffer->full = true;
		vk_config.vertex_buffer_size = max(vk_config.vertex_buffer_size * BUFFER_RESIZE_FACTOR, NextPow2(size));

		for (int i = 0; i < NUM_DYNBUFFERS; ++i)
		{
			QVk_CreateVertexBuffer(NULL, vk_config.vertex_buffer_size, &vk_swapVertexBuffers[i], NULL, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
			vmaMapMemory(vk_malloc, vk_swapVertexBuffers[i].allocation, &vk_swapVertexBuffers[i].allocInfo.pMappedData);
		}

		currentBuffer = &vk_swapVertexBuffers[vk_activeDynBufferIdx];
	}

	*dstOffset = currentBuffer->currentOffset;
	*dstBuffer = currentBuffer->buffer;
	currentBuffer->currentOffset += size;

	vk_config.vertex_buffer_usage += size;
	if (vk_config.vertex_buffer_max_usage < vk_config.vertex_buffer_usage)
		vk_config.vertex_buffer_max_usage = vk_config.vertex_buffer_usage;
	
	return (uint8_t *)currentBuffer->allocInfo.pMappedData + (*dstOffset);
}

uint8_t *QVk_GetIndexBuffer(VkDeviceSize size, VkDeviceSize *dstOffset)
{
	// align to 4 bytes, so that we can reuse the buffer for both VK_INDEX_TYPE_UINT16 and VK_INDEX_TYPE_UINT32
	const int align_mod = size % 4;
	const uint32_t aligned_size = ((size % 4) == 0) ? size : (size + 4 - align_mod);
	qvkbuffer_t *currentBuffer = &vk_dynIndexBuffers[vk_activeDynBufferIdx];

	if (currentBuffer->full)
		currentBuffer = &vk_swapIndexBuffers[vk_activeDynBufferIdx];

	if (currentBuffer->currentOffset + aligned_size > vk_config.index_buffer_size)
	{
		currentBuffer->full = true;
		vk_config.index_buffer_size = max(vk_config.index_buffer_size * BUFFER_RESIZE_FACTOR, NextPow2(size));

		for (int i = 0; i < NUM_DYNBUFFERS; ++i)
		{
			QVk_CreateIndexBuffer(NULL, vk_config.index_buffer_size, &vk_swapIndexBuffers[i], NULL, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
			vmaMapMemory(vk_malloc, vk_swapIndexBuffers[i].allocation, &vk_swapIndexBuffers[i].allocInfo.pMappedData);
		}

		currentBuffer = &vk_swapIndexBuffers[vk_activeDynBufferIdx];
	}

	*dstOffset = currentBuffer->currentOffset;
	currentBuffer->currentOffset += aligned_size;

	vk_config.index_buffer_usage += aligned_size;
	if (vk_config.index_buffer_max_usage < vk_config.index_buffer_usage)
		vk_config.index_buffer_max_usage = vk_config.index_buffer_usage;


	return (uint8_t *)currentBuffer->allocInfo.pMappedData + (*dstOffset);
}

uint8_t *QVk_GetUniformBuffer(VkDeviceSize size, uint32_t *dstOffset, VkDescriptorSet *dstUboDescriptorSet)
{
	// 0x100 alignment is required by Vulkan spec
	const int align_mod = size % 256;
	const uint32_t aligned_size = ((size % 256) == 0) ? size : (size + 256 - align_mod);
	qvkbuffer_t *currentBuffer = &vk_dynUniformBuffers[vk_activeDynBufferIdx];
	VkDescriptorSet *currentDescSet = &vk_uboDescriptorSets[vk_activeDynBufferIdx];

	if (currentBuffer->full)
	{
		currentBuffer = &vk_swapUniformBuffers[vk_activeDynBufferIdx];
		currentDescSet = &vk_swapDescriptorSets[vk_activeDynBufferIdx];
	}

	if (currentBuffer->currentOffset + UNIFORM_ALLOC_SIZE > vk_config.uniform_buffer_size)
	{
		currentBuffer->full = true;
		vk_config.uniform_buffer_size = max(vk_config.uniform_buffer_size * BUFFER_RESIZE_FACTOR, NextPow2(size));

		for (int i = 0; i < NUM_DYNBUFFERS; ++i)
		{
			QVk_CreateUniformBuffer(vk_config.uniform_buffer_size, &vk_swapUniformBuffers[i], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
			vmaMapMemory(vk_malloc, vk_swapUniformBuffers[i].allocation, &vk_swapUniformBuffers[i].allocInfo.pMappedData);

			// create descriptor set
			VkDescriptorSetAllocateInfo dsAllocInfo = {
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.pNext = NULL,
				.descriptorPool = vk_descriptorPool,
				.descriptorSetCount = 1,
				.pSetLayouts = &vk_uboDescSetLayout
			};

			VK_VERIFY(vkAllocateDescriptorSets(vk_device.logical, &dsAllocInfo, &vk_swapDescriptorSets[i]));

			VkDescriptorBufferInfo bufferInfo = {
				.buffer = vk_swapUniformBuffers[i].buffer,
				.offset = 0,
				.range = UNIFORM_ALLOC_SIZE
			};

			VkWriteDescriptorSet descriptorWrite = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = NULL,
				.dstSet = vk_swapDescriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
				.pImageInfo = NULL,
				.pBufferInfo = &bufferInfo,
				.pTexelBufferView = NULL,
			};

			vkUpdateDescriptorSets(vk_device.logical, 1, &descriptorWrite, 0, NULL);
		}

		currentBuffer = &vk_swapUniformBuffers[vk_activeDynBufferIdx];
	}

	*dstOffset = currentBuffer->currentOffset;
	*dstUboDescriptorSet = *currentDescSet;
	currentBuffer->currentOffset += aligned_size;

	vk_config.uniform_buffer_usage += aligned_size;
	if (vk_config.uniform_buffer_max_usage < vk_config.uniform_buffer_usage)
		vk_config.uniform_buffer_max_usage = vk_config.uniform_buffer_usage;

	return (uint8_t *)currentBuffer->allocInfo.pMappedData + (*dstOffset);
}

uint8_t *QVk_GetStagingBuffer(VkDeviceSize size, int alignment, VkCommandBuffer *cmdBuffer, VkBuffer *buffer, uint32_t *dstOffset)
{
	qvkstagingbuffer_t * staging_buffer = &vk_stagingBuffers[vk_activeStagingBuffer];
	const int align_mod = staging_buffer->buffer.currentOffset % alignment;
	staging_buffer->buffer.currentOffset = ((staging_buffer->buffer.currentOffset % alignment) == 0)
		? staging_buffer->buffer.currentOffset : (staging_buffer->buffer.currentOffset + alignment - align_mod);

	if (size > STAGING_BUFFER_MAXSIZE)
		Sys_Error("Cannot allocate staging buffer space");

	if ((staging_buffer->buffer.currentOffset + size) >= STAGING_BUFFER_MAXSIZE && !staging_buffer->submitted)
		SubmitStagingBuffer(vk_activeStagingBuffer);

	staging_buffer = &vk_stagingBuffers[vk_activeStagingBuffer];
	if (staging_buffer->submitted)
	{
		VK_VERIFY(vkWaitForFences(vk_device.logical, 1, &staging_buffer->fence, VK_TRUE, UINT64_MAX));
		VK_VERIFY(vkResetFences(vk_device.logical, 1, &staging_buffer->fence));

		staging_buffer->buffer.currentOffset = 0;
		staging_buffer->submitted = false;

		VkCommandBufferBeginInfo command_buffer_begin_info;
		memset(&command_buffer_begin_info, 0, sizeof(command_buffer_begin_info));
		command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_VERIFY(vkBeginCommandBuffer(staging_buffer->cmdBuffer, &command_buffer_begin_info));
	}

	if (cmdBuffer)
		*cmdBuffer = staging_buffer->cmdBuffer;
	if (buffer)
		*buffer = staging_buffer->buffer.buffer;
	if (dstOffset)
		*dstOffset = staging_buffer->buffer.currentOffset;

	unsigned char *data = (uint8_t *)staging_buffer->buffer.allocInfo.pMappedData + staging_buffer->buffer.currentOffset;
	staging_buffer->buffer.currentOffset += size;

	return data;
}

void QVk_SubmitStagingBuffers()
{
	for (int i = 0; i < NUM_DYNBUFFERS; ++i)
	{
		if (!vk_stagingBuffers[i].submitted && vk_stagingBuffers[i].buffer.currentOffset > 0)
			SubmitStagingBuffer(i);
	}
}

VkSampler QVk_UpdateTextureSampler(qvktexture_t *texture, qvksampler_t samplerType)
{
	assert((vk_samplers[samplerType] != VK_NULL_HANDLE) && "Sampler is VK_NULL_HANDLE!");

	VkDescriptorImageInfo dImgInfo = {
		.sampler = vk_samplers[samplerType],
		.imageView = texture->imageView,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	VkWriteDescriptorSet writeSet = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = NULL,
		.dstSet = texture->descriptorSet,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &dImgInfo,
		.pBufferInfo = NULL,
		.pTexelBufferView = NULL
	};

	vkUpdateDescriptorSets(vk_device.logical, 1, &writeSet, 0, NULL);

	return vk_samplers[samplerType];
}

void QVk_DrawColorRect(float *ubo, VkDeviceSize uboSize)
{
	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *vertData = QVk_GetUniformBuffer(uboSize, &uboOffset, &uboDescriptorSet);
	memcpy(vertData, ubo, uboSize);

	QVk_BindPipeline(&vk_drawColorQuadPipeline);
	VkDeviceSize offsets = 0;
	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawColorQuadPipeline.layout, 0, 1, &uboDescriptorSet, 1, &uboOffset);
	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vk_colorRectVbo.buffer, &offsets);
	vkCmdBindIndexBuffer(vk_activeCmdbuffer, vk_rectIbo.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(vk_activeCmdbuffer, 6, 1, 0, 0, 0);
}

void QVk_DrawTexRect(float *ubo, VkDeviceSize uboSize, qvktexture_t *texture)
{
	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *uboData = QVk_GetUniformBuffer(uboSize, &uboOffset, &uboDescriptorSet);
	memcpy(uboData, ubo, uboSize);

	QVk_BindPipeline(&vk_drawTexQuadPipeline);
	VkDeviceSize offsets = 0;
	VkDescriptorSet descriptorSets[] = { texture->descriptorSet, uboDescriptorSet };
	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawTexQuadPipeline.layout, 0, 2, descriptorSets, 1, &uboOffset);
	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vk_texRectVbo.buffer, &offsets);
	vkCmdBindIndexBuffer(vk_activeCmdbuffer, vk_rectIbo.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(vk_activeCmdbuffer, 6, 1, 0, 0, 0);
}

void QVk_BindPipeline(qvkpipeline_t *pipeline)
{
	if (vk_state.current_pipeline != pipeline->pl)
	{
		vkCmdBindPipeline(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pl);
		vk_state.current_pipeline = pipeline->pl;
	}
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
		default: return "<unknown>";
	}
#undef ERRSTR
	return "UNKNOWN ERROR";
}

void Vkimp_EnableLogging(qboolean enable)
{
	if (enable)
	{
		if (!vkw_state.log_fp)
		{
			struct tm *newtime;
			time_t aclock;
			char buffer[1024];

			time(&aclock);
			newtime = localtime(&aclock);

			asctime(newtime);

			Com_sprintf(buffer, sizeof(buffer), "%s/vk.log", ri.FS_Gamedir());
			vkw_state.log_fp = fopen(buffer, "wt");

			fprintf(vkw_state.log_fp, "%s\n", asctime(newtime));
		}
		vk_logfp = vkw_state.log_fp;
	}
	else
	{
		vk_logfp = NULL;
	}
}

void Vkimp_LogNewFrame( void )
{
	fprintf( vkw_state.log_fp, "*** R_BeginFrame ***\n" );
}
