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

#ifndef __VK_SHADERS_H__
#define __VK_SHADERS_H__
#include <stdint.h>

// textured quad (alpha)
extern const uint32_t basic_vert_spv[];
extern const size_t basic_vert_size;

extern const uint32_t basic_frag_spv[];
extern const size_t basic_frag_size;

// colored quad (alpha)
extern const uint32_t basic_color_quad_vert_spv[];
extern const size_t basic_color_quad_vert_size;

extern const uint32_t basic_color_quad_frag_spv[];
extern const size_t basic_color_quad_frag_size;

// textured model
extern const uint32_t model_vert_spv[];
extern const size_t model_vert_size;

extern const uint32_t model_frag_spv[];
extern const size_t model_frag_size;

// untextured null model
extern const uint32_t nullmodel_vert_spv[];
extern const size_t nullmodel_vert_size;

extern const uint32_t nullmodel_frag_spv[];
extern const size_t nullmodel_frag_size;
#endif
