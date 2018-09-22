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

// vk_mem_alloc.h(10522): warning C4477: 'fprintf' : format string '%llu' requires an argument of type 'unsigned __int64', but variadic argument 7 has type 'const size_t'
#pragma warning( disable: 4477 )

#define VMA_IMPLEMENTATION
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
