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
static void copyBuffer(const VkBuffer *src, VkBuffer *dst, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = QVk_CreateCommandBuffer(&vk_transferCommandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	QVk_BeginCommand(&commandBuffer);

	VkBufferCopy copyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size
	};
	vkCmdCopyBuffer(commandBuffer, *src, *dst, 1, &copyRegion);

	QVk_SubmitCommand(&commandBuffer, &vk_device.transferQueue);
	vkFreeCommandBuffers(vk_device.logical, vk_transferCommandPool, 1, &commandBuffer);
}

// internal helper
static void createStagedBuffer(const void *data, VkDeviceSize size, qvkbuffer_t *dstBuffer, qvkbufferopts_t bufferOpts, qvkbuffer_t *stagingBuffer)
{
	qvkbuffer_t *stgBuffer = stagingBuffer;
	// create/release internal staging buffer if NULL has been passed
	if (!stagingBuffer)
	{
		stgBuffer = (qvkbuffer_t *)malloc(sizeof(qvkbuffer_t));
		VK_VERIFY(QVk_CreateStagingBuffer(size, stgBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_HOST_CACHED_BIT));
	}

	if (data)
	{
		void *dst;
		// staging buffers in vkQuake2 are required to be host coherent, so no flushing/invalidation is involved
		VK_VERIFY(vmaMapMemory(vk_malloc, stgBuffer->allocation, &dst));
		memcpy(dst, data, (size_t)size);
		vmaUnmapMemory(vk_malloc, stgBuffer->allocation);
	}

	VK_VERIFY(QVk_CreateBuffer(size, dstBuffer, bufferOpts));
	copyBuffer(&stgBuffer->buffer, &dstBuffer->buffer, size);

	if (!stagingBuffer)
	{
		QVk_FreeBuffer(stgBuffer);
		free(stgBuffer);
	}
}

VkResult QVk_CreateBuffer(VkDeviceSize size, qvkbuffer_t *dstBuffer, const qvkbufferopts_t options)
{
	VkBufferCreateInfo bcInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.size = size,
		.usage = options.usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
	};

	// separate transfer queue makes sense only if the buffer is targetted for being transfered to GPU, so ignore it if it's CPU-only
	uint32_t queueFamilies[] = { (uint32_t)vk_device.gfxFamilyIndex, (uint32_t)vk_device.transferFamilyIndex };
	if (options.vmaUsage != VMA_MEMORY_USAGE_CPU_ONLY && vk_device.gfxFamilyIndex != vk_device.transferFamilyIndex)
	{
		bcInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		bcInfo.queueFamilyIndexCount = 2;
		bcInfo.pQueueFamilyIndices = queueFamilies;
	}

	VmaAllocationCreateInfo vmallocInfo = {
		.flags = options.vmaFlags,
		.usage = options.vmaUsage,
		.requiredFlags = options.reqMemFlags,
		.preferredFlags = options.prefMemFlags,
		.memoryTypeBits = 0,
		.pool = VK_NULL_HANDLE,
		.pUserData = NULL
	};

	dstBuffer->currentOffset = 0;
	return vmaCreateBuffer(vk_malloc, &bcInfo, &vmallocInfo, &dstBuffer->buffer, &dstBuffer->allocation, &dstBuffer->allocInfo);
}

void QVk_FreeBuffer(qvkbuffer_t *buffer)
{
	vmaDestroyBuffer(vk_malloc, buffer->buffer, buffer->allocation);
	buffer->buffer = VK_NULL_HANDLE;
	buffer->allocation = VK_NULL_HANDLE;
	buffer->currentOffset = 0;
}

VkResult QVk_CreateStagingBuffer(VkDeviceSize size, qvkbuffer_t *dstBuffer, VkMemoryPropertyFlags reqMemFlags, VkMemoryPropertyFlags prefMemFlags)
{
	qvkbufferopts_t stagingOpts = {
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.reqMemFlags = reqMemFlags,
		.prefMemFlags = prefMemFlags,
		.vmaUsage = VMA_MEMORY_USAGE_CPU_ONLY,
		.vmaFlags = 0
	};

	return QVk_CreateBuffer(size, dstBuffer, stagingOpts);
}

VkResult QVk_CreateUniformBuffer(VkDeviceSize size, qvkbuffer_t *dstBuffer, VkMemoryPropertyFlags reqMemFlags, VkMemoryPropertyFlags prefMemFlags)
{
	qvkbufferopts_t dstOpts = {
		.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		.reqMemFlags = reqMemFlags,
		.prefMemFlags = prefMemFlags,
		.vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
		// When resizing dynamic uniform buffers on Intel, the Linux driver may throw a warning:
		// "Mapping an image with layout VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL can result in undefined behavior if this memory is used by the device. Only GENERAL or PREINITIALIZED should be used."
		// Minor annoyance but we don't want any validation warnings, so we create dedicated allocation for uniform buffer.
		// more details: https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/issues/34
		// Note that this is a false positive which in other cases could be ignored: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/general_considerations.html#general_considerations_validation_layer_warnings
		.vmaFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
	};

	return QVk_CreateBuffer(size, dstBuffer, dstOpts);
}

void QVk_CreateVertexBuffer(const void *data, VkDeviceSize size, qvkbuffer_t *dstBuffer, qvkbuffer_t *stagingBuffer, VkMemoryPropertyFlags reqMemFlags, VkMemoryPropertyFlags prefMemFlags)
{
	qvkbufferopts_t dstOpts = {
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.reqMemFlags = reqMemFlags,
		.prefMemFlags = prefMemFlags,
		.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY,
		.vmaFlags = 0
	};

	createStagedBuffer(data, size, dstBuffer, dstOpts, stagingBuffer);
}

void QVk_CreateIndexBuffer(const void *data, VkDeviceSize size, qvkbuffer_t *dstBuffer, qvkbuffer_t *stagingBuffer, VkMemoryPropertyFlags reqMemFlags, VkMemoryPropertyFlags prefMemFlags)
{
	qvkbufferopts_t dstOpts = {
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		.reqMemFlags = reqMemFlags,
		.prefMemFlags = prefMemFlags,
		.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY,
		.vmaFlags = 0
	};

	createStagedBuffer(data, size, dstBuffer, dstOpts, stagingBuffer);
}
