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
static VkShaderModule loadShader(const uint32_t *shaderSrc, size_t codeSize)
{
	VkShaderModule shaderModule = VK_NULL_HANDLE;
	VkShaderModuleCreateInfo smCreateInfo = {
	smCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	smCreateInfo.pNext = NULL,
	smCreateInfo.flags = 0,
	smCreateInfo.codeSize = codeSize,
	smCreateInfo.pCode = shaderSrc
	};

	VK_VERIFY(vkCreateShaderModule(vk_device.logical, &smCreateInfo, NULL, &shaderModule));
	return shaderModule;
}
