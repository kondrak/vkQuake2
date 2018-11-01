/*
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

static VkResult createBuffer(VkDeviceSize size, qvkbuffer_t *dstBuffer, const qvkbufferopts_t options)
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
	if (options.vmaUsage != VMA_MEMORY_USAGE_CPU_ONLY && vk_device.gfxFamilyIndex != vk_device.transferFamilyIndex)
	{
		uint32_t queueFamilies[] = { (uint32_t)vk_device.gfxFamilyIndex, (uint32_t)vk_device.transferFamilyIndex };
		bcInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		bcInfo.queueFamilyIndexCount = 2;
		bcInfo.pQueueFamilyIndices = queueFamilies;
	}

	VmaAllocationCreateInfo vmallocInfo = {
		.flags = options.vmaFlags,
		.usage = options.vmaUsage,
		.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		.preferredFlags = options.memFlags,
		.memoryTypeBits = 0,
		.pool = VK_NULL_HANDLE,
		.pUserData = NULL
	};

	return vmaCreateBuffer(vk_malloc, &bcInfo, &vmallocInfo, &dstBuffer->buffer, &dstBuffer->allocation, &dstBuffer->allocInfo);
}

// internal helper
static void createStagedBuffer(const void *data, VkDeviceSize size, qvkbuffer_t *dstBuffer, qvkbufferopts_t bufferOpts, qvkbuffer_t *stagingBuffer)
{
	qvkbuffer_t *stgBuffer = stagingBuffer;
	// create/release internal staging buffer if NULL has been passed
	if (!stagingBuffer)
	{
		stgBuffer = (qvkbuffer_t *)malloc(sizeof(qvkbuffer_t));
		VK_VERIFY(QVk_CreateStagingBuffer(size, stgBuffer));
	}

	void *dst;
	vmaMapMemory(vk_malloc, stgBuffer->allocation, &dst);
	memcpy(dst, data, (size_t)size);
	vmaUnmapMemory(vk_malloc, stgBuffer->allocation);

	VK_VERIFY(createBuffer(size, dstBuffer, bufferOpts));
	copyBuffer(&stgBuffer->buffer, &dstBuffer->buffer, size);

	if (!stagingBuffer)
	{
		QVk_FreeBuffer(stgBuffer);
		free(stgBuffer);
	}
}

void QVk_FreeBuffer(qvkbuffer_t *buffer)
{
	vmaDestroyBuffer(vk_malloc, buffer->buffer, buffer->allocation);
	buffer->buffer = VK_NULL_HANDLE;
	buffer->allocation = VK_NULL_HANDLE;
}

VkResult QVk_CreateStagingBuffer(VkDeviceSize size, qvkbuffer_t *dstBuffer)
{
	qvkbufferopts_t stagingOpts = {
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		.vmaUsage = VMA_MEMORY_USAGE_CPU_ONLY,
		.vmaFlags = 0
	};

	return createBuffer(size, dstBuffer, stagingOpts);
}

VkResult QVk_CreateUniformBuffer(VkDeviceSize size, qvkbuffer_t *dstBuffer)
{
	qvkbufferopts_t dstOpts = {
	dstOpts.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	dstOpts.memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	dstOpts.vmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
	dstOpts.vmaFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT
	};

	return createBuffer(size, dstBuffer, dstOpts);
}

void QVk_CreateVertexBuffer(const void *data, VkDeviceSize size, qvkbuffer_t *dstBuffer, qvkbuffer_t *stagingBuffer)
{
	qvkbufferopts_t dstOpts = {
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY,
		.vmaFlags = 0
	};

	createStagedBuffer(data, size, dstBuffer, dstOpts, stagingBuffer);
}

void QVk_CreateIndexBuffer(const void *data, VkDeviceSize size, qvkbuffer_t *dstBuffer, qvkbuffer_t *stagingBuffer)
{
	qvkbufferopts_t dstOpts = {
		.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		.memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		.vmaUsage = VMA_MEMORY_USAGE_GPU_ONLY,
		.vmaFlags = 0
	};

	createStagedBuffer(data, size, dstBuffer, dstOpts, stagingBuffer);
}
