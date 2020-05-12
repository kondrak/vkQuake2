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

VkResult QVk_BeginCommand(const VkCommandBuffer *commandBuffer)
{
	VkCommandBufferBeginInfo cmdInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = NULL
	};

	return vkBeginCommandBuffer(*commandBuffer, &cmdInfo);
}

void QVk_SubmitCommand(const VkCommandBuffer *commandBuffer, const VkQueue *queue)
{
	VK_VERIFY(vkEndCommandBuffer(*commandBuffer));

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = NULL,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = NULL,
		.pWaitDstStageMask = NULL,
		.commandBufferCount = 1,
		.pCommandBuffers = commandBuffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = NULL
	};

	VkFenceCreateInfo fCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};

	VkFence queueFence;
	VK_VERIFY(vkCreateFence(vk_device.logical, &fCreateInfo, NULL, &queueFence));
	VK_VERIFY(vkQueueSubmit(*queue, 1, &submitInfo, queueFence));
	VK_VERIFY(vkWaitForFences(vk_device.logical, 1, &queueFence, VK_TRUE, UINT64_MAX));

	vkDestroyFence(vk_device.logical, queueFence, NULL);
}

VkResult QVk_CreateCommandPool(VkCommandPool *commandPool, uint32_t queueFamilyIndex)
{
	VkCommandPoolCreateInfo cpCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		.queueFamilyIndex = queueFamilyIndex
	};

	return vkCreateCommandPool(vk_device.logical, &cpCreateInfo, NULL, commandPool);
}

VkCommandBuffer QVk_CreateCommandBuffer(const VkCommandPool *commandPool, VkCommandBufferLevel level)
{
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = *commandPool,
		.level = level,
		.commandBufferCount = 1
	};

	VK_VERIFY(vkAllocateCommandBuffers(vk_device.logical, &allocInfo, &commandBuffer));
	return commandBuffer;
}
