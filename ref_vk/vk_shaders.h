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

//
// game shaders, compiled offline
//

#ifndef __VK_SHADERS_H__
#define __VK_SHADERS_H__
#include <stdint.h>
#include <stddef.h>

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

// null model
extern const uint32_t nullmodel_vert_spv[];
extern const size_t nullmodel_vert_size;

// particle (texture)
extern const uint32_t particle_vert_spv[];
extern const size_t particle_vert_size;

// particle (point)
extern const uint32_t point_particle_vert_spv[];
extern const size_t point_particle_vert_size;

extern const uint32_t point_particle_frag_spv[];
extern const size_t point_particle_frag_size;

// sprite model
extern const uint32_t sprite_vert_spv[];
extern const size_t sprite_vert_size;

// beam
extern const uint32_t beam_vert_spv[];
extern const size_t beam_vert_size;

// skybox
extern const uint32_t skybox_vert_spv[];
extern const size_t skybox_vert_size;

// dynamic lights
extern const uint32_t d_light_vert_spv[];
extern const size_t d_light_vert_size;

// textured, alpha blended polygon
extern const uint32_t polygon_vert_spv[];
extern const size_t polygon_vert_size;

// textured, lightmapped polygon
extern const uint32_t polygon_lmap_vert_spv[];
extern const size_t polygon_lmap_vert_size;

extern const uint32_t polygon_lmap_frag_spv[];
extern const size_t polygon_lmap_frag_size;

// warped polygon (liquids)
extern const uint32_t polygon_warp_vert_spv[];
extern const size_t polygon_warp_vert_size;

// entity shadows
extern const uint32_t shadows_vert_spv[];
extern const size_t shadows_vert_size;

// postprocess
extern const uint32_t postprocess_vert_spv[];
extern const size_t postprocess_vert_size;

extern const uint32_t postprocess_frag_spv[];
extern const size_t postprocess_frag_size;

// underwater vision warp
extern const uint32_t world_warp_vert_spv[];
extern const size_t world_warp_vert_size;

extern const uint32_t world_warp_frag_spv[];
extern const size_t world_warp_frag_size;

#endif
