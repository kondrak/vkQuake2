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

#include "vk_local.h"

image_t		vktextures[MAX_VKTEXTURES];
int			numvktextures;
int			base_textureid;		// gltextures[i] = base_textureid+i
// texture for storing raw image data (cinematics, endscreens, etc.)
qvktexture_t vk_rawTexture = QVVKTEXTURE_INIT;

static byte			 intensitytable[256];
static unsigned char gammatable[256];

cvar_t		*intensity;
extern cvar_t	*vk_mip_nearfilter;

unsigned	d_8to24table[256];

uint32_t Vk_Upload8 (byte *data, int width, int height,  qboolean mipmap, qboolean is_sky );
uint32_t Vk_Upload32 (unsigned *data, int width, int height,  qboolean mipmap);

// default global texture and lightmap samplers
qvksampler_t vk_current_sampler = S_MIPMAP_LINEAR;
qvksampler_t vk_current_lmap_sampler = S_MIPMAP_LINEAR;

// internal helper
static VkImageAspectFlags getDepthStencilAspect(VkFormat depthFormat)
{
	switch (depthFormat)
	{
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
	case VK_FORMAT_D24_UNORM_S8_UINT:
	case VK_FORMAT_D16_UNORM_S8_UINT:
		return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	default:
		return VK_IMAGE_ASPECT_DEPTH_BIT;
	}
}

// internal helper
static void transitionImageLayout(const VkCommandBuffer *cmdBuffer, const VkQueue *queue, const qvktexture_t *texture, const VkImageLayout oldLayout, const VkImageLayout newLayout)
{
	VkPipelineStageFlags srcStage = 0;
	VkPipelineStageFlags dstStage = 0;

	VkImageMemoryBarrier imgBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = NULL,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = texture->image,
		.subresourceRange.baseMipLevel = 0, // no mip mapping levels
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
		.subresourceRange.levelCount = texture->mipLevels
	};

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		imgBarrier.subresourceRange.aspectMask = getDepthStencilAspect(texture->format);
	}
	else
	{
		imgBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		imgBarrier.srcAccessMask = 0;
		imgBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	// transiton that may occur when updating existing image
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		imgBarrier.srcAccessMask = 0;
		imgBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		if (vk_device.transferQueue == vk_device.gfxQueue)
		{
			imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			if (vk_device.transferQueue == *queue)
			{
				// if the image is exclusively shared, start queue ownership transfer process (release) - only for VK_SHARING_MODE_EXCLUSIVE
				imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imgBarrier.dstAccessMask = 0;
				imgBarrier.srcQueueFamilyIndex = vk_device.transferFamilyIndex;
				imgBarrier.dstQueueFamilyIndex = vk_device.gfxFamilyIndex;
				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			}
			else
			{
				// continuing queue transfer (acquisition) - this will only happen for VK_SHARING_MODE_EXCLUSIVE images
				if (texture->sharingMode == VK_SHARING_MODE_EXCLUSIVE)
				{
					imgBarrier.srcAccessMask = 0;
					imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					imgBarrier.srcQueueFamilyIndex = vk_device.transferFamilyIndex;
					imgBarrier.dstQueueFamilyIndex = vk_device.gfxFamilyIndex;
					srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				}
				else
				{
					imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
					dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				}
			}
		}
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		imgBarrier.srcAccessMask = 0;
		imgBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		imgBarrier.srcAccessMask = 0;
		imgBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else
	{
		assert(0 && !"Invalid image stage!");
	}

	vkCmdPipelineBarrier(*cmdBuffer, srcStage, dstStage, 0, 0, NULL, 0, NULL, 1, &imgBarrier);
}

// internal helper
static void generateMipmaps(const VkCommandBuffer *cmdBuffer, const qvktexture_t *texture, uint32_t width, uint32_t height)
{
	int32_t mipWidth = width;
	int32_t mipHeight = height;
	VkFilter mipFilter = vk_mip_nearfilter->value > 0 ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;

	VkImageMemoryBarrier imgBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = NULL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = texture->image,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1
	};

	// copy rescaled mip data between consecutive levels (each higher level is half the size of the previous level)
	for (uint32_t i = 1; i < texture->mipLevels; ++i)
	{
		imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imgBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imgBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imgBarrier.subresourceRange.baseMipLevel = i - 1;

		vkCmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &imgBarrier);

		VkImageBlit blit = {
			.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.srcSubresource.mipLevel = i - 1,
			.srcSubresource.baseArrayLayer = 0,
			.srcSubresource.layerCount = 1,
			.srcOffsets[0] = { 0, 0, 0 },
			.srcOffsets[1] = { mipWidth, mipHeight, 1 },
			.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.dstSubresource.mipLevel = i,
			.dstSubresource.baseArrayLayer = 0,
			.dstSubresource.layerCount = 1,
			.dstOffsets[0] = { 0, 0, 0 },
			.dstOffsets[1] = { mipWidth > 1 ? mipWidth >> 1 : 1,
							  mipHeight > 1 ? mipHeight >> 1 : 1, 1 } // each mip level is half the size of the previous level
		};

		// src image == dst image, because we're blitting between different mip levels of the same image
		vkCmdBlitImage(*cmdBuffer, texture->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, mipFilter);

		imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imgBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		vkCmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &imgBarrier);

		// avoid zero-sized mip levels
		if (mipWidth > 1)  mipWidth >>= 1;
		if (mipHeight > 1) mipHeight >>= 1;
	}

	imgBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imgBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imgBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imgBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imgBarrier.subresourceRange.baseMipLevel = texture->mipLevels - 1;

	vkCmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &imgBarrier);
}

// internal helper
static void createTextureImage(qvktexture_t *dstTex, const unsigned char *data, uint32_t width, uint32_t height)
{
	int unifiedTransferAndGfx = vk_device.transferQueue == vk_device.gfxQueue ? 1 : 0;
	// assuming 32bit images
	uint32_t imageSize = width * height * 4;

	VkBuffer staging_buffer;
	VkCommandBuffer command_buffer;
	uint32_t staging_offset;
	void *imgData = QVk_GetStagingBuffer(imageSize, 4, &command_buffer, &staging_buffer, &staging_offset);
	memcpy(imgData, data, (size_t)imageSize);

	VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	// set extra image usage flag if we're dealing with mipmapped image - will need it for copying data between mip levels
	if (dstTex->mipLevels > 1)
		imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	VK_VERIFY(QVk_CreateImage(width, height, dstTex->format, VK_IMAGE_TILING_OPTIMAL, imageUsage, VMA_MEMORY_USAGE_GPU_ONLY, dstTex));

	transitionImageLayout(&command_buffer, &vk_device.transferQueue, dstTex, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	// copy buffer to image
	VkBufferImageCopy region = {
		.bufferOffset = staging_offset,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.imageSubresource.mipLevel = 0,
		.imageSubresource.baseArrayLayer = 0,
		.imageSubresource.layerCount = 1,
		.imageOffset = { 0, 0, 0 },
		.imageExtent = { width, height, 1 }
	};

	vkCmdCopyBufferToImage(command_buffer, staging_buffer, dstTex->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	if (dstTex->mipLevels > 1)
	{
		// vkCmdBlitImage requires a queue with GRAPHICS_BIT present
		generateMipmaps(&command_buffer, dstTex, width, height);
	}
	else
	{
		// for non-unified transfer and graphics, this step begins queue ownership transfer to graphics queue (for exclusive sharing only)
		if (unifiedTransferAndGfx || dstTex->sharingMode == VK_SHARING_MODE_EXCLUSIVE)
			transitionImageLayout(&command_buffer, &vk_device.transferQueue, dstTex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		if (!unifiedTransferAndGfx)
		{
			transitionImageLayout(&command_buffer, &vk_device.gfxQueue, dstTex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}
}

VkResult QVk_CreateImageView(const VkImage *image, VkImageAspectFlags aspectFlags, VkImageView *imageView, VkFormat format, uint32_t mipLevels)
{
	VkImageViewCreateInfo ivCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.image = *image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		.subresourceRange.aspectMask = aspectFlags,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.layerCount = 1,
		.subresourceRange.levelCount = mipLevels
	};

	return vkCreateImageView(vk_device.logical, &ivCreateInfo, NULL, imageView);
}

VkResult QVk_CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaMemoryUsage memUsage, qvktexture_t *texture)
{
	VkImageCreateInfo imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent.width = width,
		.extent.height = height,
		.extent.depth = 1,
		.mipLevels = texture->mipLevels,
		.arrayLayers = 1,
		.format = format,
		.tiling = tiling,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.samples = texture->sampleCount,
		.flags = 0
	};

	uint32_t queueFamilies[] = { (uint32_t)vk_device.gfxFamilyIndex, (uint32_t)vk_device.transferFamilyIndex };
	if (vk_device.gfxFamilyIndex != vk_device.transferFamilyIndex)
	{
		imageInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		imageInfo.queueFamilyIndexCount = 2;
		imageInfo.pQueueFamilyIndices = queueFamilies;
	}

	VmaAllocationCreateInfo vmallocInfo = {
		.flags = texture->vmaFlags,
		.usage = memUsage
	};

	texture->sharingMode = imageInfo.sharingMode;
	return vmaCreateImage(vk_malloc, &imageInfo, &vmallocInfo, &texture->image, &texture->allocation, &texture->allocInfo);
}

void QVk_CreateDepthBuffer(VkSampleCountFlagBits sampleCount, qvktexture_t *depthBuffer)
{
	depthBuffer->format = QVk_FindDepthFormat();
	depthBuffer->sampleCount = sampleCount;
	// On 64-bit builds, Intel drivers throw a warning:
	// "Mapping an image with layout VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL can result in undefined behavior if this memory is used by the device. Only GENERAL or PREINITIALIZED should be used."
	// Minor annoyance but we don't want any validation warnings, so we create dedicated allocation for depth buffer.
	// more details: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/issues/34
	// Note that this is a false positive which in other cases could be ignored: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/general_considerations.html#general_considerations_validation_layer_warnings
	depthBuffer->vmaFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

	VK_VERIFY(QVk_CreateImage(vk_swapchain.extent.width, vk_swapchain.extent.height, depthBuffer->format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY, depthBuffer));
	VK_VERIFY(QVk_CreateImageView(&depthBuffer->image, getDepthStencilAspect(depthBuffer->format), &depthBuffer->imageView, depthBuffer->format, depthBuffer->mipLevels));
}

void QVk_CreateColorBuffer(VkSampleCountFlagBits sampleCount, qvktexture_t *colorBuffer, int extraFlags)
{
	colorBuffer->format = vk_swapchain.format;
	colorBuffer->sampleCount = sampleCount;
	// On 64-bit builds, Intel drivers throw a warning:
	// "Mapping an image with layout VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL can result in undefined behavior if this memory is used by the device. Only GENERAL or PREINITIALIZED should be used."
	// Minor annoyance but we don't want any validation warnings, so we create dedicated allocation for color buffer.
	// more details: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/issues/34
	// Note that this is a false positive which in other cases could be ignored: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/general_considerations.html#general_considerations_validation_layer_warnings
	colorBuffer->vmaFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

	VK_VERIFY(QVk_CreateImage(vk_swapchain.extent.width, vk_swapchain.extent.height, colorBuffer->format, VK_IMAGE_TILING_OPTIMAL, extraFlags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VMA_MEMORY_USAGE_GPU_ONLY, colorBuffer));
	VK_VERIFY(QVk_CreateImageView(&colorBuffer->image, VK_IMAGE_ASPECT_COLOR_BIT, &colorBuffer->imageView, colorBuffer->format, colorBuffer->mipLevels));
}

void QVk_CreateTexture(qvktexture_t *texture, const unsigned char *data, uint32_t width, uint32_t height, qvksampler_t samplerType)
{
	createTextureImage(texture, data, width, height);
	VK_VERIFY(QVk_CreateImageView(&texture->image, VK_IMAGE_ASPECT_COLOR_BIT, &texture->imageView, texture->format, texture->mipLevels));

	// create descriptor set for the texture
	VkDescriptorSetAllocateInfo dsAllocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL,
		.descriptorPool = vk_descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &vk_samplerDescSetLayout
	};

	VK_VERIFY(vkAllocateDescriptorSets(vk_device.logical, &dsAllocInfo, &texture->descriptorSet));
	vk_config.allocated_sampler_descriptor_set_count++;

	// attach sampler
	QVk_UpdateTextureSampler(texture, samplerType);
}

void QVk_UpdateTextureData(qvktexture_t *texture, const unsigned char *data, uint32_t offset_x, uint32_t offset_y, uint32_t width, uint32_t height)
{
	int unifiedTransferAndGfx = vk_device.transferQueue == vk_device.gfxQueue ? 1 : 0;
	// assuming 32bit images
	uint32_t imageSize = width * height * 4;

	VkBuffer staging_buffer;
	VkCommandBuffer command_buffer;
	uint32_t staging_offset;
	void *imgData = QVk_GetStagingBuffer(imageSize, 4, &command_buffer, &staging_buffer, &staging_offset);
	memcpy(imgData, data, (size_t)imageSize);

	transitionImageLayout(&command_buffer, &vk_device.transferQueue, texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	// copy buffer to image
	VkBufferImageCopy region = {
		.bufferOffset = staging_offset,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.imageSubresource.mipLevel = 0,
		.imageSubresource.baseArrayLayer = 0,
		.imageSubresource.layerCount = 1,
		.imageOffset = { offset_x, offset_y, 0 },
		.imageExtent = { width, height, 1 }
	};

	vkCmdCopyBufferToImage(command_buffer, staging_buffer, texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	if (texture->mipLevels > 1)
	{
		// vkCmdBlitImage requires a queue with GRAPHICS_BIT present
		generateMipmaps(&command_buffer, texture, width, height);
	}
	else
	{
		// for non-unified transfer and graphics, this step begins queue ownership transfer to graphics queue (for exclusive sharing only)
		if (unifiedTransferAndGfx || texture->sharingMode == VK_SHARING_MODE_EXCLUSIVE)
			transitionImageLayout(&command_buffer, &vk_device.transferQueue, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		if (!unifiedTransferAndGfx)
		{
			transitionImageLayout(&command_buffer, &vk_device.gfxQueue, texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}
}

void QVk_ReleaseTexture(qvktexture_t *texture)
{
	QVk_SubmitStagingBuffers();
	vkDeviceWaitIdle(vk_device.logical);

	if (texture->image != VK_NULL_HANDLE)
		vmaDestroyImage(vk_malloc, texture->image, texture->allocation);
	if (texture->imageView != VK_NULL_HANDLE)
		vkDestroyImageView(vk_device.logical, texture->imageView, NULL);
	if (texture->descriptorSet != VK_NULL_HANDLE)
	{
		vkFreeDescriptorSets(vk_device.logical, vk_descriptorPool, 1, &texture->descriptorSet);
		vk_config.allocated_sampler_descriptor_set_count--;
	}

	texture->image = VK_NULL_HANDLE;
	texture->imageView = VK_NULL_HANDLE;
	texture->descriptorSet = VK_NULL_HANDLE;
}

void QVk_ReadPixels(uint8_t *dstBuffer, uint32_t width, uint32_t height)
{
	qvkbuffer_t buff;
	VkCommandBuffer cmdBuffer;
	extern int vk_activeBufferIdx;

	qvkbufferopts_t buffOpts = {
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		.reqMemFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		.prefMemFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
		.vmaUsage = VMA_MEMORY_USAGE_CPU_ONLY,
		// When taking a screenshot on Intel, the Linux driver may throw a warning:
		// "Mapping an image with layout VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL can result in undefined behavior if this memory is used by the device. Only GENERAL or PREINITIALIZED should be used."
		// Minor annoyance but we don't want any validation warnings, so we create dedicated allocation for the image buffer.
		// more details: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/issues/34
		// Note that this is a false positive which in other cases could be ignored: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/general_considerations.html#general_considerations_validation_layer_warnings
		.vmaFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
	};

	VK_VERIFY(QVk_CreateBuffer(width * height * 4, &buff, buffOpts));
	cmdBuffer = QVk_CreateCommandBuffer(&vk_commandPool[vk_activeBufferIdx], VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VK_VERIFY(QVk_BeginCommand(&cmdBuffer));
	
	// transition the current swapchain image to be a source of data transfer to our buffer
	VkImageMemoryBarrier imgBarrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = NULL,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = vk_swapchain.images[vk_activeBufferIdx],
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
		.subresourceRange.levelCount = 1
	};

	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &imgBarrier);

	VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = width,
		.bufferImageHeight = height,
		.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.imageSubresource.mipLevel = 0,
		.imageSubresource.baseArrayLayer = 0,
		.imageSubresource.layerCount = 1,
		.imageOffset = { 0, 0, 0 },
		.imageExtent = { width, height, 1 }
	};

	// copy the swapchain image
	vkCmdCopyImageToBuffer(cmdBuffer, vk_swapchain.images[vk_activeBufferIdx], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buff.buffer, 1, &region);
	VK_VERIFY(vkDeviceWaitIdle(vk_device.logical));
	QVk_SubmitCommand(&cmdBuffer, &vk_device.gfxQueue);

	// store image in destination buffer
	memcpy(dstBuffer, (uint8_t *)buff.allocInfo.pMappedData, width * height * 4);

	QVk_FreeBuffer(&buff);
}

/*
===============
Vk_ImageList_f
===============
*/
void	Vk_ImageList_f (void)
{
	int		i;
	image_t	*image;
	int		texels;

	ri.Con_Printf(PRINT_ALL, "------------------\n");
	texels = 0;

	for (i = 0, image = vktextures; i < numvktextures; i++, image++)
	{
		if (image->vk_texture.image == VK_NULL_HANDLE)
			continue;
		texels += image->upload_width*image->upload_height;
		switch (image->type)
		{
		case it_skin:
			ri.Con_Printf(PRINT_ALL, "M");
			break;
		case it_sprite:
			ri.Con_Printf(PRINT_ALL, "S");
			break;
		case it_wall:
			ri.Con_Printf(PRINT_ALL, "W");
			break;
		case it_pic:
			ri.Con_Printf(PRINT_ALL, "P");
			break;
		default:
			ri.Con_Printf(PRINT_ALL, " ");
			break;
		}

		ri.Con_Printf(PRINT_ALL, " %3i %3i RGB: %s\n",
			image->upload_width, image->upload_height, image->name);
	}
	ri.Con_Printf(PRINT_ALL, "Total texel count (not counting mipmaps): %i\n", texels);
}


/*
=============================================================================

  scrap allocation

  Allocate all the little status bar obejcts into a single texture
  to crutch up inefficient hardware / drivers

=============================================================================
*/

// define 3 scrap textures: scrap 0 for crosshair images, scrap 1 and 2 for everything else
#define	MAX_SCRAPS		3
#define	BLOCK_WIDTH		256
#define	BLOCK_HEIGHT	256

int			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT];
// textures for storing scrap image data (tiny image atlas)
qvktexture_t vk_scrapTextures[MAX_SCRAPS] = { QVVKTEXTURE_INIT };

// returns a texture number and the position inside it
int Scrap_AllocBlock (int w, int h, int *x, int *y, int scrap_offset)
{
	int		i, j;
	int		best, best2;
	int		texnum;

	for (texnum=scrap_offset ; texnum<MAX_SCRAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (scrap_allocated[texnum][i+j] >= best)
					break;
				if (scrap_allocated[texnum][i+j] > best2)
					best2 = scrap_allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			scrap_allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	return -1;
//	Sys_Error ("Scrap_AllocBlock: full");
}

typedef struct
{
	char *name;
	qvksampler_t samplerType;
} vkmode_t;

vkmode_t modes[] = {
	{"VK_NEAREST", S_NEAREST },
	{"VK_LINEAR", S_LINEAR },
	{"VK_MIPMAP_NEAREST", S_MIPMAP_NEAREST },
	{"VK_MIPMAP_LINEAR", S_MIPMAP_LINEAR }
};

#define NUM_VK_MODES (sizeof(modes) / sizeof (vkmode_t))

/*
 ===============
 Vk_TextureMode
 ===============
 */
void Vk_TextureMode( char *string )
{
	int		i,j;
	image_t	*image;
	static char prev_mode[32] = { "VK_MIPMAP_LINEAR" };

	for (i = 0; i < NUM_VK_MODES; i++)
	{
		if (!Q_stricmp(modes[i].name, string))
			break;
	}

	if (i == NUM_VK_MODES)
	{
		ri.Con_Printf(PRINT_ALL, "bad filter name (valid values: VK_NEAREST, VK_LINEAR, VK_MIPMAP_NEAREST, VK_MIPMAP_LINEAR)\n");
		ri.Cvar_Set("vk_texturemode", prev_mode);
		return;
	}

	memcpy(prev_mode, string, strlen(string));
	prev_mode[strlen(string)] = '\0';

	i += vk_aniso->value > 0 && vk_device.features.samplerAnisotropy ? NUM_VK_MODES : 0;
	vk_current_sampler = i;

	vkDeviceWaitIdle(vk_device.logical);
	for (j = 0, image = vktextures; j < numvktextures; j++, image++)
	{
		// skip console characters - we want them unfiltered at all times
		if (image->vk_texture.image != VK_NULL_HANDLE && Q_stricmp(image->name, "pics/conchars.pcx"))
			QVk_UpdateTextureSampler(&image->vk_texture, i);
	}

	// use S_NEAREST for scrap 0 (crosshair images)
	if (vk_scrapTextures[0].image != VK_NULL_HANDLE)
		QVk_UpdateTextureSampler(&vk_scrapTextures[0], S_NEAREST);

	for (j = 1; j < MAX_SCRAPS; j++)
	{
		if (vk_scrapTextures[j].image != VK_NULL_HANDLE)
			QVk_UpdateTextureSampler(&vk_scrapTextures[j], i);
	}

	if (vk_rawTexture.image != VK_NULL_HANDLE)
		QVk_UpdateTextureSampler(&vk_rawTexture, i);
}

/*
 ===============
 Vk_LmapTextureMode
 ===============
 */
void Vk_LmapTextureMode( char *string )
{
	int		i,j;
	static char prev_mode[32] = { "VK_MIPMAP_LINEAR" };

	for (i = 0; i < NUM_VK_MODES; i++)
	{
		if (!Q_stricmp(modes[i].name, string))
			break;
	}

	if (i == NUM_VK_MODES)
	{
		ri.Con_Printf(PRINT_ALL, "bad filter name (valid values: VK_NEAREST, VK_LINEAR, VK_MIPMAP_NEAREST, VK_MIPMAP_LINEAR)\n");
		ri.Cvar_Set("vk_lmaptexturemode", prev_mode);
		return;
	}

	memcpy(prev_mode, string, strlen(string));
	prev_mode[strlen(string)] = '\0';

	i += vk_aniso->value > 0 && vk_device.features.samplerAnisotropy ? NUM_VK_MODES : 0;
	vk_current_lmap_sampler = i;

	vkDeviceWaitIdle(vk_device.logical);
	for (j = 0; j < MAX_LIGHTMAPS*2; j++)
	{
		if (vk_state.lightmap_textures[j].image != VK_NULL_HANDLE)
			QVk_UpdateTextureSampler(&vk_state.lightmap_textures[j], i);
	}
}

/*
=================================================================

PCX LOADING

=================================================================
*/


/*
==============
LoadPCX
==============
*/
void LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height)
{
	byte	*raw;
	pcx_t	*pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	byte	*out, *pix;

	*pic = NULL;
	*palette = NULL;

	//
	// load the file
	//
	len = ri.FS_LoadFile (filename, (void **)&raw);
	if (!raw)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "Bad pcx file %s\n", filename);
		return;
	}

	//
	// parse the PCX file
	//
	pcx = (pcx_t *)raw;

    pcx->xmin = LittleShort(pcx->xmin);
    pcx->ymin = LittleShort(pcx->ymin);
    pcx->xmax = LittleShort(pcx->xmax);
    pcx->ymax = LittleShort(pcx->ymax);
    pcx->hres = LittleShort(pcx->hres);
    pcx->vres = LittleShort(pcx->vres);
    pcx->bytes_per_line = LittleShort(pcx->bytes_per_line);
    pcx->palette_type = LittleShort(pcx->palette_type);

	raw = &pcx->data;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax >= 640
		|| pcx->ymax >= 480)
	{
		ri.Con_Printf (PRINT_ALL, "Bad pcx file %s\n", filename);
		return;
	}

	out = malloc ( (pcx->ymax+1) * (pcx->xmax+1) );

	*pic = out;

	pix = out;

	if (palette)
	{
		*palette = malloc(768);
		memcpy (*palette, (byte *)pcx + len - 768, 768);
	}

	if (width)
		*width = pcx->xmax+1;
	if (height)
		*height = pcx->ymax+1;

	for (y=0 ; y<=pcx->ymax ; y++, pix += pcx->xmax+1)
	{
		for (x=0 ; x<=pcx->xmax ; )
		{
			dataByte = *raw++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			}
			else
				runLength = 1;

			while(runLength-- > 0)
				pix[x++] = dataByte;
		}

	}

	if ( raw - (byte *)pcx > len)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "PCX file %s was malformed", filename);
		free (*pic);
		*pic = NULL;
	}

	ri.FS_FreeFile (pcx);
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*
=============
LoadTGA
=============
*/
void LoadTGA (char *name, byte **pic, int *width, int *height)
{
	int		columns, rows, numPixels;
	byte	*pixbuf;
	int		row, column;
	byte	*buf_p;
	byte	*buffer;
	int		length;
	TargaHeader		targa_header;
	byte			*targa_rgba;
	byte tmp[2];

	*pic = NULL;

	//
	// load the file
	//
	length = ri.FS_LoadFile (name, (void **)&buffer);
	if (!buffer)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "Bad tga file %s\n", name);
		return;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;
	
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_index = LittleShort ( *((short *)tmp) );
	buf_p+=2;
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_length = LittleShort ( *((short *)tmp) );
	buf_p+=2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.y_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.width = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.height = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.image_type!=2 
		&& targa_header.image_type!=10) 
		ri.Sys_Error (ERR_DROP, "LoadTGA: Only type 2 and 10 targa RGB images supported\n");

	if (targa_header.colormap_type !=0 
		|| (targa_header.pixel_size!=32 && targa_header.pixel_size!=24))
		ri.Sys_Error (ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	targa_rgba = malloc (numPixels*4);
	*pic = targa_rgba;

	if (targa_header.id_length != 0)
		buf_p += targa_header.id_length;  // skip TARGA image comment
	
	if (targa_header.image_type==2) {  // Uncompressed, RGB images
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; column++) {
				unsigned char red,green,blue,alphabyte;
				switch (targa_header.pixel_size) {
					case 24:
							
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = 255;
							break;
					case 32:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphabyte;
							break;
				}
			}
		}
	}
	else if (targa_header.image_type==10) {   // Runlength encoded RGB images
		unsigned char red = 0,green = 0,blue = 0,alphabyte = 0,packetHeader,packetSize,j;
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; ) {
				packetHeader= *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) {        // run-length packet
					switch (targa_header.pixel_size) {
						case 24:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = 255;
								break;
						case 32:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = *buf_p++;
								break;
					}
	
					for(j=0;j<packetSize;j++) {
						*pixbuf++=red;
						*pixbuf++=green;
						*pixbuf++=blue;
						*pixbuf++=alphabyte;
						column++;
						if (column==columns) { // run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
				else {                            // non run-length packet
					for(j=0;j<packetSize;j++) {
						switch (targa_header.pixel_size) {
							case 24:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = 255;
									break;
							case 32:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									alphabyte = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = alphabyte;
									break;
						}
						column++;
						if (column==columns) { // pixel packet run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}						
					}
				}
			}
			breakOut:;
		}
	}

	ri.FS_FreeFile (buffer);
}


/*
====================================================================

IMAGE FLOOD FILLING

====================================================================
*/


/*
=================
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes
=================
*/

typedef struct
{
	short		x, y;
} floodfill_t;

// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
	if (pos[off] == fillcolor) \
	{ \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
	} \
	else if (pos[off] != 255) fdc = pos[off]; \
}

void R_FloodFillSkin( byte *skin, int skinwidth, int skinheight )
{
	byte				fillcolor = *skin; // assume this is the pixel to fill
	floodfill_t			fifo[FLOODFILL_FIFO_SIZE];
	int					inpt = 0, outpt = 0;
	int					filledcolor = -1;
	int					i;

	if (filledcolor == -1)
	{
		filledcolor = 0;
		// attempt to find opaque black
		for (i = 0; i < 256; ++i)
			if (d_8to24table[i] == (255 << 0)) // alpha 1.0
			{
				filledcolor = i;
				break;
			}
	}

	// can't fill to filled color or to transparent color (used as visited marker)
	if ((fillcolor == filledcolor) || (fillcolor == 255))
	{
		//printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
		return;
	}

	fifo[inpt].x = 0, fifo[inpt].y = 0;
	inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

	while (outpt != inpt)
	{
		int			x = fifo[outpt].x, y = fifo[outpt].y;
		int			fdc = filledcolor;
		byte		*pos = &skin[x + skinwidth * y];

		outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

		if (x > 0)				FLOODFILL_STEP( -1, -1, 0 );
		if (x < skinwidth - 1)	FLOODFILL_STEP( 1, 1, 0 );
		if (y > 0)				FLOODFILL_STEP( -skinwidth, 0, -1 );
		if (y < skinheight - 1)	FLOODFILL_STEP( skinwidth, 0, 1 );
		skin[x + skinwidth * y] = fdc;
	}
}

//=======================================================


/*
================
Vk_ResampleTexture
================
*/
void Vk_ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight)
{
	int		i, j;
	unsigned	*inrow, *inrow2;
	unsigned	frac, fracstep;
	unsigned	p1[1024], p2[1024];
	byte		*pix1, *pix2, *pix3, *pix4;

	fracstep = inwidth*0x10000/outwidth;

	frac = fracstep>>2;
	for (i=0 ; i<outwidth ; i++)
	{
		p1[i] = 4*(frac>>16);
		frac += fracstep;
	}
	frac = 3*(fracstep>>2);
	for (i=0 ; i<outwidth ; i++)
	{
		p2[i] = 4*(frac>>16);
		frac += fracstep;
	}

	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(int)((i+0.25)*inheight/outheight);
		inrow2 = in + inwidth*(int)((i+0.75)*inheight/outheight);

		for (j=0 ; j<outwidth ; j++)
		{
			pix1 = (byte *)inrow + p1[j];
			pix2 = (byte *)inrow + p2[j];
			pix3 = (byte *)inrow2 + p1[j];
			pix4 = (byte *)inrow2 + p2[j];
			((byte *)(out+j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			((byte *)(out+j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			((byte *)(out+j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			((byte *)(out+j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}
}

/*
================
Vk_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void Vk_LightScaleTexture (unsigned *in, int inwidth, int inheight, qboolean only_gamma )
{
	if ( only_gamma )
	{
		int		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;
		for (i=0 ; i<c ; i++, p+=4)
		{
			p[0] = gammatable[p[0]];
			p[1] = gammatable[p[1]];
			p[2] = gammatable[p[2]];
		}
	}
	else
	{
		int		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;
		for (i=0 ; i<c ; i++, p+=4)
		{
			p[0] = gammatable[intensitytable[p[0]]];
			p[1] = gammatable[intensitytable[p[1]]];
			p[2] = gammatable[intensitytable[p[2]]];
		}
	}
}

/*
===============
Vk_Upload32

Returns number of mip levels
===============
*/
int		upload_width, upload_height;
unsigned int texBuffer[256 * 256];

uint32_t Vk_Upload32 (unsigned *data, int width, int height,  qboolean mipmap)
{
	unsigned	scaled[256 * 256];
	int			scaled_width, scaled_height;

	for (scaled_width = 1; scaled_width < width; scaled_width <<= 1)
		;
	if (vk_round_down->value && scaled_width > width && mipmap)
		scaled_width >>= 1;
	for (scaled_height = 1; scaled_height < height; scaled_height <<= 1)
		;
	if (vk_round_down->value && scaled_height > height && mipmap)
		scaled_height >>= 1;

	// let people sample down the world textures for speed
	if (mipmap)
	{
		scaled_width >>= (int)vk_picmip->value;
		scaled_height >>= (int)vk_picmip->value;
	}

	// don't ever bother with >256 textures
	if (scaled_width > 256)
		scaled_width = 256;
	if (scaled_height > 256)
		scaled_height = 256;

	if (scaled_width < 1)
		scaled_width = 1;
	if (scaled_height < 1)
		scaled_height = 1;

	upload_width = scaled_width;
	upload_height = scaled_height;

	if (scaled_width * scaled_height > sizeof(scaled) / 4)
		ri.Sys_Error(ERR_DROP, "Vk_Upload32: too big");

	if (scaled_width == width && scaled_height == height)
	{
		if (!mipmap)
		{
			memcpy(texBuffer, data, scaled_width * scaled_height * 4);
			return 1;
		}
		memcpy(scaled, data, width*height * 4);
	}
	else
		Vk_ResampleTexture(data, width, height, scaled, scaled_width, scaled_height);

	Vk_LightScaleTexture(scaled, scaled_width, scaled_height, !mipmap);

	memcpy(texBuffer, scaled, sizeof(scaled));
	if (mipmap)
	{
		int		miplevel = 1;

		while (scaled_width > 1 || scaled_height > 1)
		{
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;
		}

		return miplevel;
	}

	return 1;
}

/*
===============
Vk_Upload8

Returns number of mip levels
===============
*/

uint32_t Vk_Upload8 (byte *data, int width, int height,  qboolean mipmap, qboolean is_sky )
{
	static unsigned	trans[512 * 256];
	int			i, s;
	int			p;

	s = width * height;

	if (s > sizeof(trans) / 4)
		ri.Sys_Error(ERR_DROP, "Vk_Upload8: too large");

	for (i = 0; i < s; i++)
	{
		p = data[i];
		trans[i] = d_8to24table[p];

		if (p == 255)
		{	// transparent, so scan around for another color
			// to avoid alpha fringes
			// FIXME: do a full flood fill so mips work...
			if (i > width && data[i - width] != 255)
				p = data[i - width];
			else if (i < s - width && data[i + width] != 255)
				p = data[i + width];
			else if (i > 0 && data[i - 1] != 255)
				p = data[i - 1];
			else if (i < s - 1 && data[i + 1] != 255)
				p = data[i + 1];
			else
				p = 0;
			// copy rgb components
			((byte *)&trans[i])[0] = ((byte *)&d_8to24table[p])[0];
			((byte *)&trans[i])[1] = ((byte *)&d_8to24table[p])[1];
			((byte *)&trans[i])[2] = ((byte *)&d_8to24table[p])[2];
		}
	}

	return Vk_Upload32(trans, width, height, mipmap);
}


/*
================
Vk_LoadPic

This is also used as an entry point for the generated r_notexture
================
*/
image_t *Vk_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type, int bits, qvksampler_t *samplerType)
{
	image_t		*image;
	int			i;

	// find a free image_t
	for (i = 0, image = vktextures; i<numvktextures; i++, image++)
	{
		if (image->vk_texture.image == VK_NULL_HANDLE && !image->scrap)
			break;
	}
	if (i == numvktextures)
	{
		if (numvktextures == MAX_VKTEXTURES)
			ri.Sys_Error(ERR_DROP, "MAX_VKTEXTURES");
		numvktextures++;
	}
	image = &vktextures[i];

	if (strlen(name) >= sizeof(image->name))
		ri.Sys_Error(ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name);
	strcpy(image->name, name);
	image->registration_sequence = registration_sequence;
	// zero-clear Vulkan texture handle
	QVVKTEXTURE_CLEAR(image->vk_texture);
	image->width = width;
	image->height = height;
	image->type = type;

	if (type == it_skin && bits == 8)
		R_FloodFillSkin(pic, width, height);

	// load little pics into the scrap
	if (image->type == it_pic && bits == 8
		&& image->width < 64 && image->height < 64)
	{
		int		x, y;
		int		i, j, k;
		int		texnum;
		qboolean	is_crosshair = !Q_stricmp(image->name, "pics/ch1.pcx") || !Q_stricmp(image->name, "pics/ch2.pcx") || !Q_stricmp(image->name, "pics/ch3.pcx");

		// store crosshair images exclusively in scrap 0
		texnum = Scrap_AllocBlock(image->width, image->height, &x, &y, is_crosshair ? 0 : 1);
		if (texnum == -1)
			goto nonscrap;

		// copy the texels into the scrap block
		k = 0;
		for (i = 0; i<image->height; i++)
			for (j = 0; j<image->width; j++, k++)
				scrap_texels[texnum][(y + i)*BLOCK_WIDTH + x + j] = pic[k];
		image->scrap = true;
		image->sl = (x + 0.01) / (float)BLOCK_WIDTH;
		image->sh = (x + image->width - 0.01) / (float)BLOCK_WIDTH;
		image->tl = (y + 0.01) / (float)BLOCK_WIDTH;
		image->th = (y + image->height - 0.01) / (float)BLOCK_WIDTH;
		image->upload_width = BLOCK_WIDTH;
		image->upload_height = BLOCK_HEIGHT;

		// update scrap data
		Vk_Upload8(scrap_texels[texnum], BLOCK_WIDTH, BLOCK_HEIGHT, false, false);

		if (vk_scrapTextures[texnum].image != VK_NULL_HANDLE)
		{
			QVk_UpdateTextureData(&vk_scrapTextures[texnum], (unsigned char*)texBuffer, 0, 0, image->upload_width, image->upload_height);
		}
		else
		{
			QVVKTEXTURE_CLEAR(vk_scrapTextures[texnum]);
			qvksampler_t scrap_sampler = samplerType ? *samplerType : vk_current_sampler;
			// don't use linear filtering for scrap 0 - this fixes display issues for the dot crosshair and makes it look consistent across different values of hudscale cvar
			QVk_CreateTexture(&vk_scrapTextures[texnum], (unsigned char*)texBuffer, image->upload_width, image->upload_height, is_crosshair ? S_NEAREST : scrap_sampler);
			QVk_DebugSetObjectName((uint64_t)vk_scrapTextures[texnum].image, VK_OBJECT_TYPE_IMAGE, va("Image: %s", name));
			QVk_DebugSetObjectName((uint64_t)vk_scrapTextures[texnum].imageView, VK_OBJECT_TYPE_IMAGE_VIEW, va("Image View: %s", name));
			QVk_DebugSetObjectName((uint64_t)vk_scrapTextures[texnum].descriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET, va("Descriptor Set: %s", name));
			QVk_DebugSetObjectName((uint64_t)vk_scrapTextures[texnum].allocInfo.deviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, "Memory: scrap texture");
		}

		image->vk_texture = vk_scrapTextures[texnum];
	}
	else
	{
	nonscrap:
		image->scrap = false;
		if (bits == 8)
			image->vk_texture.mipLevels = Vk_Upload8(pic, width, height, (image->type != it_pic && image->type != it_sky), image->type == it_sky);
		else
			image->vk_texture.mipLevels = Vk_Upload32((unsigned *)pic, width, height, (image->type != it_pic && image->type != it_sky));

		image->upload_width = upload_width;		// after power of 2 and scales
		image->upload_height = upload_height;
		image->sl = 0;
		image->sh = 1;
		image->tl = 0;
		image->th = 1;

		QVk_CreateTexture(&image->vk_texture, (unsigned char*)texBuffer, image->upload_width, image->upload_height, samplerType ? *samplerType : vk_current_sampler);
		QVk_DebugSetObjectName((uint64_t)image->vk_texture.image, VK_OBJECT_TYPE_IMAGE, va("Image: %s", name));
		QVk_DebugSetObjectName((uint64_t)image->vk_texture.imageView, VK_OBJECT_TYPE_IMAGE_VIEW, va("Image View: %s", name));
		QVk_DebugSetObjectName((uint64_t)image->vk_texture.descriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET, va("Descriptor Set: %s", name));
		QVk_DebugSetObjectName((uint64_t)image->vk_texture.allocInfo.deviceMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, "Memory: game textures");
	}

	return image;
}


/*
================
Vk_LoadWal
================
*/
image_t *Vk_LoadWal (char *name)
{
	miptex_t	*mt;
	int			width, height, ofs;
	image_t		*image;

	ri.FS_LoadFile (name, (void **)&mt);
	if (!mt)
	{
		ri.Con_Printf (PRINT_ALL, "Vk_FindImage: can't load %s\n", name);
		return r_notexture;
	}

	width = LittleLong (mt->width);
	height = LittleLong (mt->height);
	ofs = LittleLong (mt->offsets[0]);

	image = Vk_LoadPic (name, (byte *)mt + ofs, width, height, it_wall, 8, NULL);

	ri.FS_FreeFile ((void *)mt);

	return image;
}

/*
===============
Vk_FindImage

Finds or loads the given image
===============
*/
image_t	*Vk_FindImage (char *name, imagetype_t type, qvksampler_t *samplerType)
{
	image_t	*image;
	int		i, len;
	byte	*pic, *palette;
	int		width, height;

	if (!name)
		return NULL;	//	ri.Sys_Error (ERR_DROP, "Vk_FindImage: NULL name");
	len = (int)strlen(name);
	if (len<5)
		return NULL;	//	ri.Sys_Error (ERR_DROP, "Vk_FindImage: bad name: %s", name);

	// look for it
	for (i=0, image=vktextures ; i<numvktextures ; i++,image++)
	{
		if (!strcmp(name, image->name))
		{
			image->registration_sequence = registration_sequence;
			return image;
		}
	}

	//
	// load the pic from disk
	//
	pic = NULL;
	palette = NULL;
	if (!strcmp(name+len-4, ".pcx"))
	{
		LoadPCX (name, &pic, &palette, &width, &height);
		if (!pic)
			return NULL; // ri.Sys_Error (ERR_DROP, "Vk_FindImage: can't load %s", name);
		image = Vk_LoadPic (name, pic, width, height, type, 8, samplerType);
	}
	else if (!strcmp(name+len-4, ".wal"))
	{
		image = Vk_LoadWal (name);
	}
	else if (!strcmp(name+len-4, ".tga"))
	{
		LoadTGA (name, &pic, &width, &height);
		if (!pic)
			return NULL; // ri.Sys_Error (ERR_DROP, "Vk_FindImage: can't load %s", name);
		image = Vk_LoadPic (name, pic, width, height, type, 32, samplerType);
	}
	else
		return NULL;	//	ri.Sys_Error (ERR_DROP, "Vk_FindImage: bad extension on: %s", name);


	if (pic)
		free(pic);
	if (palette)
		free(palette);

	return image;
}



/*
===============
R_RegisterSkin
===============
*/
struct image_s *R_RegisterSkin (char *name)
{
	return Vk_FindImage (name, it_skin, NULL);
}


/*
================
Vk_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void Vk_FreeUnusedImages (void)
{
	int		i;
	image_t	*image;

	// never free r_notexture or particle texture
	r_notexture->registration_sequence = registration_sequence;
	r_particletexture->registration_sequence = registration_sequence;

	for (i = 0, image = vktextures; i < numvktextures; i++, image++)
	{
		if (image->registration_sequence == registration_sequence)
			continue;		// used this sequence
		if (!image->registration_sequence)
			continue;		// free image_t slot
		if (image->type == it_pic)
			continue;		// don't free pics
		// free it
		QVk_ReleaseTexture(&image->vk_texture);
		memset(image, 0, sizeof(*image));
	}
}


/*
===============
Draw_GetPalette
===============
*/
int Draw_GetPalette (void)
{
	int		i;
	int		r, g, b;
	unsigned	v;
	byte	*pic, *pal;
	int		width, height;

	// get the palette

	LoadPCX ("pics/colormap.pcx", &pic, &pal, &width, &height);
	if (!pal)
		ri.Sys_Error (ERR_FATAL, "Couldn't load pics/colormap.pcx");

	for (i=0 ; i<256 ; i++)
	{
		r = pal[i*3+0];
		g = pal[i*3+1];
		b = pal[i*3+2];
		
		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		d_8to24table[i] = LittleLong(v);
	}

	d_8to24table[255] &= LittleLong(0xffffff);	// 255 is transparent

	free (pic);
	free (pal);

	return 0;
}


/*
===============
Vk_InitImages
===============
*/
void	Vk_InitImages (void)
{
	int		i, j;
	float	g = vid_gamma->value;

	registration_sequence = 1;

	// init intensity conversions
	intensity = ri.Cvar_Get("intensity", "2", 0);

	if (intensity->value <= 1)
		ri.Cvar_Set("intensity", "1");

	vk_state.inverse_intensity = 1 / intensity->value;

	Draw_GetPalette();

	for (i = 0; i < 256; i++)
	{
		if (g == 1)
		{
			gammatable[i] = i;
		}
		else
		{
			float inf;

			inf = 255 * pow((i + 0.5) / 255.5, g) + 0.5;
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			gammatable[i] = inf;
		}
	}

	for (i = 0; i<256; i++)
	{
		j = i * intensity->value;
		if (j > 255)
			j = 255;
		intensitytable[i] = j;
	}
}

/*
===============
Vk_ShutdownImages
===============
*/
void	Vk_ShutdownImages (void)
{
	int		i;
	image_t	*image;

	for (i = 0, image = vktextures; i<numvktextures; i++, image++)
	{
		if (!image->registration_sequence)
			continue;		// free image_t slot
		// free it (scraps are stored in a separate Vulkan buffer)
		if (!image->scrap)
			QVk_ReleaseTexture(&image->vk_texture);
		memset(image, 0, sizeof(*image));
	}

	QVk_ReleaseTexture(&vk_rawTexture);

	for(i = 0; i < MAX_SCRAPS; i++)
		QVk_ReleaseTexture(&vk_scrapTextures[i]);

	for(i = 0; i < MAX_LIGHTMAPS*2; i++)
		QVk_ReleaseTexture(&vk_state.lightmap_textures[i]);
}

