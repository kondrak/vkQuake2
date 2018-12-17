/*
Copyright (C) 1997-2001 Id Software, Inc.
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
// vk_rsurf.c: surface-related refresh code
#include <assert.h>

#include "vk_local.h"

static vec3_t	modelorg;		// relative to viewpoint

msurface_t	*r_alpha_surfaces;

#define DYNAMIC_LIGHT_WIDTH  128
#define DYNAMIC_LIGHT_HEIGHT 128

#define LIGHTMAP_BYTES 4

#define	BLOCK_WIDTH		128
#define	BLOCK_HEIGHT	128


int		c_visible_lightmaps;
int		c_visible_textures;

#define VK_LIGHTMAP_FORMAT VK_FORMAT_R8G8B8A8_UNORM

typedef struct
{
	int internal_format;
	int	current_lightmap_texture;

	msurface_t	*lightmap_surfaces[MAX_LIGHTMAPS];

	int			allocated[BLOCK_WIDTH];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	byte		lightmap_buffer[4*BLOCK_WIDTH*BLOCK_HEIGHT];
} vklightmapstate_t;

static vklightmapstate_t vk_lms;


static void		LM_InitBlock( void );
static void		LM_UploadBlock( qboolean dynamic );
static qboolean	LM_AllocBlock (int w, int h, int *x, int *y);
static void		RgbToRgba(unsigned char *src, unsigned char *dst, size_t size);

extern void R_SetCacheState( msurface_t *surf );
extern void R_BuildLightMap (msurface_t *surf, byte *dest, int stride);

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
===============
R_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
image_t *R_TextureAnimation (mtexinfo_t *tex)
{
	int		c;

	if (!tex->next)
		return tex->image;

	c = currententity->frame % tex->numframes;
	while (c)
	{
		tex = tex->next;
		c--;
	}

	return tex->image;
}

/*
================
DrawVkPoly
================
*/
void DrawVkPoly (vkpoly_t *p, image_t *texture, float *color)
{
	int		i;
	float	*v;

	typedef struct {
		float vertex[3];
		float texCoord[2];
	} polyvert;

	struct {
		float mvp[16];
		float color[4];
	} polyUbo;

	polyUbo.color[0] = color[0];
	polyUbo.color[1] = color[1];
	polyUbo.color[2] = color[2];
	polyUbo.color[3] = color[3];

	polyvert verts[MAX_VERTS];

	v = p->verts[0];
	for (i = 0; i < p->numverts; i++, v += VERTEXSIZE)
	{
		verts[i].vertex[0] = v[0];
		verts[i].vertex[1] = v[1];
		verts[i].vertex[2] = v[2];
		verts[i].texCoord[0] = v[3];
		verts[i].texCoord[1] = v[4];
	}

	memcpy(polyUbo.mvp, r_viewproj_matrix, sizeof(r_viewproj_matrix));

	QVk_BindPipeline(&vk_drawPolyPipeline);

	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *uboData = QVk_GetUniformBuffer(sizeof(polyUbo), &uboOffset, &uboDescriptorSet);
	memcpy(uboData, &polyUbo, sizeof(polyUbo));

	VkBuffer vbo;
	VkDeviceSize vboOffset;
	uint8_t *data = QVk_GetVertexBuffer(sizeof(polyvert) * p->numverts, &vbo, &vboOffset);
	memcpy(data, verts, sizeof(polyvert) * p->numverts);

	VkDescriptorSet descriptorSets[] = { uboDescriptorSet, texture->vk_texture.descriptorSet };
	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawPolyPipeline.layout, 0, 2, descriptorSets, 1, &uboOffset);
	vkCmdDraw(vk_activeCmdbuffer, p->numverts, 1, 0, 0);
}

//============
//PGM
/*
================
DrawVkFlowingPoly -- version of DrawVkPoly that handles scrolling texture
================
*/
void DrawVkFlowingPoly (msurface_t *fa, image_t *texture, float *color)
{
	int		i;
	float	*v;
	vkpoly_t *p;
	float	scroll;

	typedef struct {
		float vertex[3];
		float texCoord[2];
	} polyvert;

	struct {
		float mvp[16];
		float color[4];
	} polyUbo;

	polyUbo.color[0] = color[0];
	polyUbo.color[1] = color[1];
	polyUbo.color[2] = color[2];
	polyUbo.color[3] = color[3];

	polyvert verts[MAX_VERTS];

	p = fa->polys;

	scroll = -64 * ((r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0));
	if (scroll == 0.0)
		scroll = -64.0;

	v = p->verts[0];
	for (i = 0; i < p->numverts; i++, v += VERTEXSIZE)
	{
		verts[i].vertex[0] = v[0];
		verts[i].vertex[1] = v[1];
		verts[i].vertex[2] = v[2];
		verts[i].texCoord[0] = v[3] + scroll;
		verts[i].texCoord[1] = v[4];
	}

	memcpy(polyUbo.mvp, r_viewproj_matrix, sizeof(r_viewproj_matrix));

	QVk_BindPipeline(&vk_drawPolyPipeline);

	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *uboData = QVk_GetUniformBuffer(sizeof(polyUbo), &uboOffset, &uboDescriptorSet);
	memcpy(uboData, &polyUbo, sizeof(polyUbo));

	VkBuffer vbo;
	VkDeviceSize vboOffset;
	uint8_t *data = QVk_GetVertexBuffer(sizeof(polyvert) * p->numverts, &vbo, &vboOffset);
	memcpy(data, verts, sizeof(polyvert) * p->numverts);

	VkDescriptorSet descriptorSets[] = { uboDescriptorSet, texture->vk_texture.descriptorSet };
	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawPolyPipeline.layout, 0, 2, descriptorSets, 1, &uboOffset);
	vkCmdDraw(vk_activeCmdbuffer, p->numverts, 1, 0, 0);
}
//PGM
//============

/*
** R_DrawTriangleOutlines
*/
void R_DrawTriangleOutlines (void)
{
	int			i, j, k;
	vkpoly_t	*p;

	if (!vk_showtris->value)
		return;

	VkBuffer vbo;
	VkDeviceSize vboOffset;
	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	vec3_t verts[4];

	QVk_BindPipeline(&vk_showTrisPipeline);

	uint8_t *uboData = QVk_GetUniformBuffer(sizeof(r_viewproj_matrix), &uboOffset, &uboDescriptorSet);
	memcpy(uboData, &r_viewproj_matrix, sizeof(r_viewproj_matrix));

	VkDescriptorSet descriptorSets[] = { uboDescriptorSet };
	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_showTrisPipeline.layout, 0, 1, descriptorSets, 1, &uboOffset);

	for (i = 0; i < MAX_LIGHTMAPS; i++)
	{
		msurface_t *surf;

		for (surf = vk_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain)
		{
			p = surf->polys;
			for (; p; p = p->chain)
			{
				for (j = 2, k = 0; j < p->numverts; j++, k++)
				{
					verts[0][0] = p->verts[0][0];
					verts[0][1] = p->verts[0][1];
					verts[0][2] = p->verts[0][2];

					verts[1][0] = p->verts[j - 1][0];
					verts[1][1] = p->verts[j - 1][1];
					verts[1][2] = p->verts[j - 1][2];

					verts[2][0] = p->verts[j][0];
					verts[2][1] = p->verts[j][1];
					verts[2][2] = p->verts[j][2];

					verts[3][0] = p->verts[0][0];
					verts[3][1] = p->verts[0][1];
					verts[3][2] = p->verts[0][2];

					uint8_t *data = QVk_GetVertexBuffer(4 * sizeof(vec3_t), &vbo, &vboOffset);
					memcpy(data, verts, sizeof(vec3_t) * 4);

					vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
					vkCmdDraw(vk_activeCmdbuffer, 4, 1, 0, 0);
				}
			}
		}
	}
}

/*
** DrawVkPolyChain
*/
void DrawVkPolyChain( vkpoly_t *p, float soffset, float toffset, image_t *texture, float *color )
{
	int polycount = 0;

	typedef struct {
		float vertex[3];
		float texCoord[2];
	} polyvert;

	struct {
		float mvp[16];
		float color[4];
	} polyUbo;

	polyUbo.color[0] = color[0];
	polyUbo.color[1] = color[1];
	polyUbo.color[2] = color[2];
	polyUbo.color[3] = color[3];

	polyvert verts[MAX_VERTS];
	memcpy(polyUbo.mvp, r_viewproj_matrix, sizeof(r_viewproj_matrix));

	QVk_BindPipeline(&vk_drawPolyPipeline);

	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *uboData = QVk_GetUniformBuffer(sizeof(polyUbo), &uboOffset, &uboDescriptorSet);
	memcpy(uboData, &polyUbo, sizeof(polyUbo));

	VkBuffer vbo;
	VkDeviceSize vboOffset;
	VkDescriptorSet descriptorSets[] = { uboDescriptorSet, texture->vk_texture.descriptorSet };
	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawPolyPipeline.layout, 0, 2, descriptorSets, 1, &uboOffset);


	if (soffset == 0 && toffset == 0)
	{
		for (; p != 0; p = p->chain)
		{
			float *v;
			int j;

			v = p->verts[0];
			for (j = 0; j < p->numverts; j++, v += VERTEXSIZE)
			{
				verts[j].vertex[0] = v[0];
				verts[j].vertex[1] = v[1];
				verts[j].vertex[2] = v[2];
				verts[j].texCoord[0] = v[5];
				verts[j].texCoord[1] = v[6];
			}

			uint8_t *data = QVk_GetVertexBuffer(sizeof(polyvert) * p->numverts, &vbo, &vboOffset);
			memcpy(data, verts, sizeof(polyvert) * p->numverts);

			vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
			vkCmdDraw(vk_activeCmdbuffer, p->numverts, 1, 0, 0);
		}
	}
	else
	{
		for (; p != 0; p = p->chain)
		{
			float *v;
			int j;

			v = p->verts[0];
			for (j = 0; j < p->numverts; j++, v += VERTEXSIZE)
			{
				verts[j].vertex[0] = v[0];
				verts[j].vertex[1] = v[1];
				verts[j].vertex[2] = v[2];
				verts[j].texCoord[0] = v[5] - soffset;
				verts[j].texCoord[1] = v[6] - toffset;
			}

			uint8_t *data = QVk_GetVertexBuffer(sizeof(polyvert) * p->numverts, &vbo, &vboOffset);
			memcpy(data, verts, sizeof(polyvert) * p->numverts);

			vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
			vkCmdDraw(vk_activeCmdbuffer, p->numverts, 1, 0, 0);
		}
	}
}

/*
================
R_RenderBrushPoly
================
*/
void R_RenderBrushPoly (msurface_t *fa, float *modelMatrix, float alpha)
{
	int			maps;
	image_t		*image;
	qboolean is_dynamic = false;
	float		color[4] = { 1.f, 1.f, 1.f, alpha };
	c_brush_polys++;

	image = R_TextureAnimation(fa->texinfo);

	if (fa->flags & SURF_DRAWTURB)
	{
		color[0] = color[1] = color[2] = vk_state.inverse_intensity;
		color[3] = 1.f;
		// warp texture, no lightmaps
		EmitWaterPolys(fa, image, modelMatrix, color);
		return;
	}

	//======
	//PGM
	if (fa->texinfo->flags & SURF_FLOWING)
		DrawVkFlowingPoly(fa, image, color);
	else
		DrawVkPoly(fa->polys, image, color);
	//PGM
	//======

	/*
	** check for lightmap modification
	*/
	for (maps = 0; maps < MAXLIGHTMAPS && fa->styles[maps] != 255; maps++)
	{
		if (r_newrefdef.lightstyles[fa->styles[maps]].white != fa->cached_light[maps])
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if ((fa->dlightframe == r_framecount))
	{
	dynamic:
		if (vk_dynamic->value)
		{
			if (!(fa->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP)))
			{
				is_dynamic = true;
			}
		}
	}

	if (is_dynamic)
	{
		if ((fa->styles[maps] >= 32 || fa->styles[maps] == 0) && (fa->dlightframe != r_framecount))
		{
			unsigned	temp[34 * 34];
			int			smax, tmax;

			smax = (fa->extents[0] >> 4) + 1;
			tmax = (fa->extents[1] >> 4) + 1;

			R_BuildLightMap(fa, (void *)temp, smax * 4);
			R_SetCacheState(fa);

			QVk_UpdateTexture(&vk_state.lightmap_textures[fa->lightmaptexturenum], (unsigned char*)temp, fa->light_s, fa->light_t, smax, tmax);
			
			fa->lightmapchain = vk_lms.lightmap_surfaces[fa->lightmaptexturenum];
			vk_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
		}
		else
		{
			fa->lightmapchain = vk_lms.lightmap_surfaces[0];
			vk_lms.lightmap_surfaces[0] = fa;
		}
	}
	else
	{
		fa->lightmapchain = vk_lms.lightmap_surfaces[fa->lightmaptexturenum];
		vk_lms.lightmap_surfaces[fa->lightmaptexturenum] = fa;
	}
}


/*
================
R_DrawAlphaSurfaces

Draw water surfaces and windows.
The BSP tree is waled front to back, so unwinding the chain
of alpha_surfaces will draw back to front, giving proper ordering.
================
*/
void R_DrawAlphaSurfaces (void)
{
	msurface_t	*s;
	float		intens;

	// the textures are prescaled up for a better lighting range,
	// so scale it back down
	intens = vk_state.inverse_intensity;
	float color[4] = { intens, intens, intens, 1.f };

	for (s = r_alpha_surfaces; s; s = s->texturechain)
	{
		c_brush_polys++;
		if (s->texinfo->flags & SURF_TRANS33)
			color[3] = 0.33f;
		else if (s->texinfo->flags & SURF_TRANS66)
			color[3] = 0.66f;

		if (s->flags & SURF_DRAWTURB)
			EmitWaterPolys(s, s->texinfo->image, NULL, color);
		else if (s->texinfo->flags & SURF_FLOWING)			// PGM	9/16/98
			DrawVkFlowingPoly(s, s->texinfo->image, color);	// PGM
		else
			DrawVkPoly(s->polys, s->texinfo->image, color);
	}

	r_alpha_surfaces = NULL;
}

/*
================
DrawTextureChains
================
*/
void DrawTextureChains (void)
{
	int		i;
	msurface_t	*s;
	image_t		*image;

	c_visible_textures = 0;

	for (i = 0, image = vktextures; i < numvktextures; i++, image++)
	{
		if (!image->registration_sequence)
			continue;
		if (!image->texturechain)
			continue;
		c_visible_textures++;

		for (s = image->texturechain; s; s = s->texturechain)
		{
			if (!(s->flags & SURF_DRAWTURB))
				R_RenderBrushPoly(s, NULL, 1.f);
		}
	}

	for (i = 0, image = vktextures; i < numvktextures; i++, image++)
	{
		if (!image->registration_sequence)
			continue;
		s = image->texturechain;
		if (!s)
			continue;

		for (; s; s = s->texturechain)
		{
			if (s->flags & SURF_DRAWTURB)
				R_RenderBrushPoly(s, NULL, 1.f);
		}

		image->texturechain = NULL;
	}
}


static void Vk_RenderLightmappedPoly( msurface_t *surf, float *modelMatrix, float alpha )
{
	int		i, nv = surf->polys->numverts;
	int		map;
	float	*v;
	image_t *image = R_TextureAnimation(surf->texinfo);
	qboolean is_dynamic = false;
	unsigned lmtex = surf->lightmaptexturenum;
	vkpoly_t *p;

	typedef struct {
		float vertex[3];
		float texCoord[2];
		float texCoordLmap[2];
	} lmappolyvert;

	lmappolyvert verts[MAX_VERTS];

	struct {
		float mvp[16];
	} lmapPolyUbo;

	if (modelMatrix)
	{
		Mat_Mul(modelMatrix, r_viewproj_matrix, lmapPolyUbo.mvp);
	}
	else
	{
		memcpy(lmapPolyUbo.mvp, r_viewproj_matrix, sizeof(r_viewproj_matrix));
	}

	QVk_BindPipeline(&vk_drawPolyLmapPipeline);

	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *uboData = QVk_GetUniformBuffer(sizeof(lmapPolyUbo), &uboOffset, &uboDescriptorSet);
	memcpy(uboData, &lmapPolyUbo, sizeof(lmapPolyUbo));

	for (map = 0; map < MAXLIGHTMAPS && surf->styles[map] != 255; map++)
	{
		if (r_newrefdef.lightstyles[surf->styles[map]].white != surf->cached_light[map])
			goto dynamic;
	}

	// dynamic this frame or dynamic previously
	if ((surf->dlightframe == r_framecount))
	{
	dynamic:
		if (vk_dynamic->value)
		{
			if (!(surf->texinfo->flags & (SURF_SKY | SURF_TRANS33 | SURF_TRANS66 | SURF_WARP)))
			{
				is_dynamic = true;
			}
		}
	}

	if (is_dynamic)
	{
		unsigned	temp[128 * 128];
		int			smax, tmax;

		if ((surf->styles[map] >= 32 || surf->styles[map] == 0) && (surf->dlightframe != r_framecount))
		{
			smax = (surf->extents[0] >> 4) + 1;
			tmax = (surf->extents[1] >> 4) + 1;

			R_BuildLightMap(surf, (void *)temp, smax * 4);
			R_SetCacheState(surf);

			lmtex = surf->lightmaptexturenum;
			QVk_UpdateTexture(&vk_state.lightmap_textures[surf->lightmaptexturenum], (unsigned char *)temp, surf->light_s, surf->light_t, smax, tmax);
		}
		else
		{
			smax = (surf->extents[0] >> 4) + 1;
			tmax = (surf->extents[1] >> 4) + 1;

			R_BuildLightMap(surf, (void *)temp, smax * 4);

			lmtex = surf->lightmaptexturenum + DYNLIGHTMAP_OFFSET;
			QVk_UpdateTexture(&vk_state.lightmap_textures[lmtex], (unsigned char *)temp, surf->light_s, surf->light_t, smax, tmax);
		}

		c_brush_polys++;

		//==========
		//PGM
		if (surf->texinfo->flags & SURF_FLOWING)
		{
			float scroll;

			scroll = -64 * ((r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0));
			if (scroll == 0.0)
				scroll = -64.0;

			VkBuffer vbo;
			VkDeviceSize vboOffset;
			VkDescriptorSet descriptorSets[] = { uboDescriptorSet, image->vk_texture.descriptorSet, vk_state.lightmap_textures[lmtex].descriptorSet };
			vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawPolyLmapPipeline.layout, 0, 3, descriptorSets, 1, &uboOffset);

			for (p = surf->polys; p; p = p->chain)
			{
				v = p->verts[0];
				for (i = 0; i < nv; i++, v += VERTEXSIZE)
				{
					verts[i].vertex[0] = v[0];
					verts[i].vertex[1] = v[1];
					verts[i].vertex[2] = v[2];
					verts[i].texCoord[0] = v[3] + scroll;
					verts[i].texCoord[1] = v[4];
					verts[i].texCoordLmap[0] = v[5];
					verts[i].texCoordLmap[1] = v[6];
				}

				uint8_t *data = QVk_GetVertexBuffer(sizeof(lmappolyvert) * nv, &vbo, &vboOffset);
				memcpy(data, verts, sizeof(lmappolyvert) * nv);

				vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
				vkCmdDraw(vk_activeCmdbuffer, nv, 1, 0, 0);
			}
		}
		else
		{
			VkBuffer vbo;
			VkDeviceSize vboOffset;
			VkDescriptorSet descriptorSets[] = { uboDescriptorSet, image->vk_texture.descriptorSet, vk_state.lightmap_textures[lmtex].descriptorSet };
			vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawPolyLmapPipeline.layout, 0, 3, descriptorSets, 1, &uboOffset);

			for (p = surf->polys; p; p = p->chain)
			{
				v = p->verts[0];
				for (i = 0; i < nv; i++, v += VERTEXSIZE)
				{
					verts[i].vertex[0] = v[0];
					verts[i].vertex[1] = v[1];
					verts[i].vertex[2] = v[2];
					verts[i].texCoord[0] = v[3];
					verts[i].texCoord[1] = v[4];
					verts[i].texCoordLmap[0] = v[5];
					verts[i].texCoordLmap[1] = v[6];
				}

				uint8_t *data = QVk_GetVertexBuffer(sizeof(lmappolyvert) * nv, &vbo, &vboOffset);
				memcpy(data, verts, sizeof(lmappolyvert) * nv);

				vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
				vkCmdDraw(vk_activeCmdbuffer, nv, 1, 0, 0);
			}
		}
		//PGM
		//==========
	}
	else
	{
		c_brush_polys++;

		//==========
		//PGM
		if (surf->texinfo->flags & SURF_FLOWING)
		{
			float scroll;

			scroll = -64 * ((r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0));
			if (scroll == 0.0)
				scroll = -64.0;

			for (p = surf->polys; p; p = p->chain)
			{
				v = p->verts[0];
				for (i = 0; i < nv; i++, v += VERTEXSIZE)
				{
					verts[i].vertex[0] = v[0];
					verts[i].vertex[1] = v[1];
					verts[i].vertex[2] = v[2];
					verts[i].texCoord[0] = v[3] + scroll;
					verts[i].texCoord[1] = v[4];
					verts[i].texCoordLmap[0] = v[5];
					verts[i].texCoordLmap[1] = v[6];
				}
				VkBuffer vbo;
				VkDeviceSize vboOffset;
				uint8_t *data = QVk_GetVertexBuffer(sizeof(lmappolyvert) * nv, &vbo, &vboOffset);
				memcpy(data, verts, sizeof(lmappolyvert) * nv);

				VkDescriptorSet descriptorSets[] = { uboDescriptorSet, image->vk_texture.descriptorSet, vk_state.lightmap_textures[lmtex].descriptorSet };
				vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
				vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawPolyLmapPipeline.layout, 0, 3, descriptorSets, 1, &uboOffset);
				vkCmdDraw(vk_activeCmdbuffer, nv, 1, 0, 0);
			}
		}
		else
		{
			//PGM
			//==========
			for (p = surf->polys; p; p = p->chain)
			{
				v = p->verts[0];
				for (i = 0; i < nv; i++, v += VERTEXSIZE)
				{
					verts[i].vertex[0] = v[0];
					verts[i].vertex[1] = v[1];
					verts[i].vertex[2] = v[2];
					verts[i].texCoord[0] = v[3];
					verts[i].texCoord[1] = v[4];
					verts[i].texCoordLmap[0] = v[5];
					verts[i].texCoordLmap[1] = v[6];
				}
				VkBuffer vbo;
				VkDeviceSize vboOffset;
				uint8_t *data = QVk_GetVertexBuffer(sizeof(lmappolyvert) * nv, &vbo, &vboOffset);
				memcpy(data, verts, sizeof(lmappolyvert) * nv);

				VkDescriptorSet descriptorSets[] = { uboDescriptorSet, image->vk_texture.descriptorSet, vk_state.lightmap_textures[lmtex].descriptorSet };
				vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);
				vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawPolyLmapPipeline.layout, 0, 3, descriptorSets, 1, &uboOffset);
				vkCmdDraw(vk_activeCmdbuffer, nv, 1, 0, 0);
			}
			//==========
			//PGM
		}
		//PGM
		//==========
	}
}

/*
=================
R_DrawInlineBModel
=================
*/
void R_DrawInlineBModel (float *modelMatrix)
{
	int			i, k;
	cplane_t	*pplane;
	float		dot;
	msurface_t	*psurf;
	dlight_t	*lt;
	float		alpha = 1.f;

	// calculate dynamic lighting for bmodel
	if (!vk_flashblend->value)
	{
		lt = r_newrefdef.dlights;
		for (k = 0; k<r_newrefdef.num_dlights; k++, lt++)
		{
			R_MarkLights(lt, 1 << k, currentmodel->nodes + currentmodel->firstnode);
		}
	}

	psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];

	if (currententity->flags & RF_TRANSLUCENT)
	{
		alpha = .25f;
	}

	//
	// draw texture
	//
	for (i = 0; i<currentmodel->nummodelsurfaces; i++, psurf++)
	{
		// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct(modelorg, pplane->normal) - pplane->dist;

		// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (psurf->texinfo->flags & (SURF_TRANS33 | SURF_TRANS66))
			{	// add to the translucent chain
				psurf->texturechain = r_alpha_surfaces;
				r_alpha_surfaces = psurf;
			}
			else if (!(psurf->flags & SURF_DRAWTURB) && !vk_showtris->value)
			{
				Vk_RenderLightmappedPoly(psurf, modelMatrix, alpha);
			}
			else
			{
				R_RenderBrushPoly(psurf, modelMatrix, alpha);
			}
		}
	}
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel (entity_t *e)
{
	vec3_t		mins, maxs;
	int			i;
	qboolean	rotated;

	if (currentmodel->nummodelsurfaces == 0)
		return;

	currententity = e;
	//gl_state.currenttextures[0] = gl_state.currenttextures[1] = -1;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;
		for (i = 0; i<3; i++)
		{
			mins[i] = e->origin[i] - currentmodel->radius;
			maxs[i] = e->origin[i] + currentmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd(e->origin, currentmodel->mins, mins);
		VectorAdd(e->origin, currentmodel->maxs, maxs);
	}

	if (R_CullBox(mins, maxs))
		return;

	memset(vk_lms.lightmap_surfaces, 0, sizeof(vk_lms.lightmap_surfaces));

	VectorSubtract(r_newrefdef.vieworg, e->origin, modelorg);
	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy(modelorg, temp);
		AngleVectors(e->angles, forward, right, up);
		modelorg[0] = DotProduct(temp, forward);
		modelorg[1] = -DotProduct(temp, right);
		modelorg[2] = DotProduct(temp, up);
	}

	e->angles[0] = -e->angles[0];	// stupid quake bug
	e->angles[2] = -e->angles[2];	// stupid quake bug
	float model[16];
	Mat_Identity(model);
	R_RotateForEntity(e, model);
	e->angles[0] = -e->angles[0];	// stupid quake bug
	e->angles[2] = -e->angles[2];	// stupid quake bug

	R_DrawInlineBModel(model);
}

/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode (mnode_t *node)
{
	int			c, side, sidebit;
	cplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	float		dot;
	image_t		*image;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;
	if (R_CullBox (node->minmaxs, node->minmaxs+3))
		return;
	
// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		pleaf = (mleaf_t *)node;

		// check for door connected areas
		if (r_newrefdef.areabits)
		{
			if (! (r_newrefdef.areabits[pleaf->area>>3] & (1<<(pleaf->area&7)) ) )
				return;		// not visible
		}

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				(*mark)->visframe = r_framecount;
				mark++;
			} while (--c);
		}

		return;
	}

// node is just a decision point, so go down the apropriate sides

// find which side of the node we are on
	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
		dot = modelorg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = modelorg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = modelorg[2] - plane->dist;
		break;
	default:
		dot = DotProduct (modelorg, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0)
	{
		side = 0;
		sidebit = 0;
	}
	else
	{
		side = 1;
		sidebit = SURF_PLANEBACK;
	}

// recurse down the children, front side first
	R_RecursiveWorldNode (node->children[side]);

	// draw stuff
	for ( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c ; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;

		if ( (surf->flags & SURF_PLANEBACK) != sidebit )
			continue;		// wrong side

		if (surf->texinfo->flags & SURF_SKY)
		{	// just adds to visible sky bounds
			R_AddSkySurface (surf);
		}
		else if (surf->texinfo->flags & (SURF_TRANS33|SURF_TRANS66))
		{	// add to the translucent chain
			surf->texturechain = r_alpha_surfaces;
			r_alpha_surfaces = surf;
		}
		else
		{
			if (!(surf->flags & SURF_DRAWTURB) && !vk_showtris->value)
			{
				Vk_RenderLightmappedPoly(surf, NULL, 1.f);
			}
			else
			{
				// the polygon is visible, so add it to the texture
				// sorted chain
				// FIXME: this is a hack for animation
				image = R_TextureAnimation(surf->texinfo);
				surf->texturechain = image->texturechain;
				image->texturechain = surf;
			}
		}
	}

	// recurse down the back side
	R_RecursiveWorldNode (node->children[!side]);
}


/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld (void)
{
	entity_t	ent;

	if (!r_drawworld->value)
		return;

	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
		return;

	currentmodel = r_worldmodel;

	VectorCopy (r_newrefdef.vieworg, modelorg);

	// auto cycle the world frame for texture animation
	memset (&ent, 0, sizeof(ent));
	ent.frame = (int)(r_newrefdef.time*2);
	currententity = &ent;

	//gl_state.currenttextures[0] = gl_state.currenttextures[1] = -1;
	memset (vk_lms.lightmap_surfaces, 0, sizeof(vk_lms.lightmap_surfaces));
	R_ClearSkyBox ();

	R_RecursiveWorldNode (r_worldmodel->nodes);

	/*
	** theoretically nothing should happen in the next two functions
	** if multitexture is enabled - in practice, this code renders non-transparent liquids!
	*/
	DrawTextureChains ();

	R_DrawSkyBox ();

	R_DrawTriangleOutlines ();
}


/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
void R_MarkLeaves (void)
{
	byte	*vis;
	byte	fatvis[MAX_MAP_LEAFS/8];
	mnode_t	*node;
	int		i, c;
	mleaf_t	*leaf;
	int		cluster;

	if (r_oldviewcluster == r_viewcluster && r_oldviewcluster2 == r_viewcluster2 && !r_novis->value && r_viewcluster != -1)
		return;

	// development aid to let you run around and see exactly where
	// the pvs ends
	if (vk_lockpvs->value)
		return;

	r_visframecount++;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if (r_novis->value || r_viewcluster == -1 || !r_worldmodel->vis)
	{
		// mark everything
		for (i=0 ; i<r_worldmodel->numleafs ; i++)
			r_worldmodel->leafs[i].visframe = r_visframecount;
		for (i=0 ; i<r_worldmodel->numnodes ; i++)
			r_worldmodel->nodes[i].visframe = r_visframecount;
		return;
	}

	vis = Mod_ClusterPVS (r_viewcluster, r_worldmodel);
	// may have to combine two clusters because of solid water boundaries
	if (r_viewcluster2 != r_viewcluster)
	{
		memcpy (fatvis, vis, (r_worldmodel->numleafs+7)/8);
		vis = Mod_ClusterPVS (r_viewcluster2, r_worldmodel);
		c = (r_worldmodel->numleafs+31)/32;
		for (i=0 ; i<c ; i++)
			((int *)fatvis)[i] |= ((int *)vis)[i];
		vis = fatvis;
	}
	
	for (i=0,leaf=r_worldmodel->leafs ; i<r_worldmodel->numleafs ; i++, leaf++)
	{
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;
		if (vis[cluster>>3] & (1<<(cluster&7)))
		{
			node = (mnode_t *)leaf;
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}



/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

static void LM_InitBlock( void )
{
	memset( vk_lms.allocated, 0, sizeof( vk_lms.allocated ) );
}

static void LM_UploadBlock( qboolean dynamic )
{
	int texture;
	int height = 0;

	if ( dynamic )
	{
		texture = 0;
	}
	else
	{
		texture = vk_lms.current_lightmap_texture;
	}

	if ( dynamic )
	{
		int i;

		for ( i = 0; i < BLOCK_WIDTH; i++ )
		{
			if ( vk_lms.allocated[i] > height )
				height = vk_lms.allocated[i];
		}
		QVk_UpdateTexture(&vk_state.lightmap_textures[texture], vk_lms.lightmap_buffer, 0, 0, BLOCK_WIDTH, height);
	}
	else
	{
		if (vk_state.lightmap_textures[texture].image != VK_NULL_HANDLE)
			QVk_UpdateTexture(&vk_state.lightmap_textures[texture], vk_lms.lightmap_buffer, 0, 0, BLOCK_WIDTH, BLOCK_HEIGHT);
		else
		{
			QVVKTEXTURE_CLEAR(vk_state.lightmap_textures[texture]);
			vk_state.lightmap_textures[texture].format = VK_LIGHTMAP_FORMAT;
			qvktextureopts_t defaultTexOpts = QVVKTEXTUREOPTS_INIT;
			QVk_CreateTexture(&vk_state.lightmap_textures[texture], vk_lms.lightmap_buffer, BLOCK_WIDTH, BLOCK_HEIGHT, &defaultTexOpts);
		}
		if ( ++vk_lms.current_lightmap_texture == MAX_LIGHTMAPS )
			ri.Sys_Error( ERR_DROP, "LM_UploadBlock() - MAX_LIGHTMAPS exceeded\n" );
	}
}

// returns a texture number and the position inside it
static qboolean LM_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;

	best = BLOCK_HEIGHT;

	for (i=0 ; i<BLOCK_WIDTH-w ; i++)
	{
		best2 = 0;

		for (j=0 ; j<w ; j++)
		{
			if (vk_lms.allocated[i+j] >= best)
				break;
			if (vk_lms.allocated[i+j] > best2)
				best2 = vk_lms.allocated[i+j];
		}
		if (j == w)
		{	// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > BLOCK_HEIGHT)
		return false;

	for (i=0 ; i<w ; i++)
		vk_lms.allocated[*x + i] = best + h;

	return true;
}

static void RgbToRgba(unsigned char *src, unsigned char *dst, size_t size)
{
	// few GPUs support true 24bit textures, so we need to convert lightmaps to 32bit for Vulkan to work
	memset(dst, 255, size);
	for (int i = 0; i < size; i += 4)
		memcpy(dst + i, src + i, 3);
}

/*
================
Vk_BuildPolygonFromSurface
================
*/
void Vk_BuildPolygonFromSurface(msurface_t *fa)
{
	int			i, lindex, lnumverts;
	medge_t		*pedges, *r_pedge;
	int			vertpage;
	float		*vec;
	float		s, t;
	vkpoly_t	*poly;
	vec3_t		total;

// reconstruct the polygon
	pedges = currentmodel->edges;
	lnumverts = fa->numedges;
	vertpage = 0;

	VectorClear (total);
	//
	// draw texture
	//
	poly = Hunk_Alloc (sizeof(vkpoly_t) + (lnumverts-4) * VERTEXSIZE*sizeof(float));
	poly->next = fa->polys;
	poly->flags = fa->flags;
	fa->polys = poly;
	poly->numverts = lnumverts;

	for (i=0 ; i<lnumverts ; i++)
	{
		lindex = currentmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &pedges[lindex];
			vec = currentmodel->vertexes[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &pedges[-lindex];
			vec = currentmodel->vertexes[r_pedge->v[1]].position;
		}
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->image->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->image->height;

		VectorAdd (total, vec, total);
		VectorCopy (vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		//
		// lightmap texture coordinates
		//
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s*16;
		s += 8;
		s /= BLOCK_WIDTH*16; //fa->texinfo->texture->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t*16;
		t += 8;
		t /= BLOCK_HEIGHT*16; //fa->texinfo->texture->height;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;
	}

	poly->numverts = lnumverts;

}

/*
========================
Vk_CreateSurfaceLightmap
========================
*/
void Vk_CreateSurfaceLightmap (msurface_t *surf)
{
	int		smax, tmax;
	byte	*base;

	if (surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB))
		return;

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;

	if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
	{
		LM_UploadBlock( false );
		LM_InitBlock();
		if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
		{
			ri.Sys_Error( ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed\n", smax, tmax );
		}
	}

	surf->lightmaptexturenum = vk_lms.current_lightmap_texture;

	base = vk_lms.lightmap_buffer;
	base += (surf->light_t * BLOCK_WIDTH + surf->light_s) * LIGHTMAP_BYTES;

	R_SetCacheState( surf );
	R_BuildLightMap (surf, base, BLOCK_WIDTH*LIGHTMAP_BYTES);
}


/*
==================
Vk_BeginBuildingLightmaps

==================
*/
void Vk_BeginBuildingLightmaps (model_t *m)
{
	static lightstyle_t	lightstyles[MAX_LIGHTSTYLES];
	int				i;
	unsigned		dummy[BLOCK_WIDTH * BLOCK_HEIGHT];

	memset(vk_lms.allocated, 0, sizeof(vk_lms.allocated));

	r_framecount = 1;		// no dlightcache

	/*
	** setup the base lightstyles so the lightmaps won't have to be regenerated
	** the first time they're seen
	*/
	for (i = 0; i < MAX_LIGHTSTYLES; i++)
	{
		lightstyles[i].rgb[0] = 1;
		lightstyles[i].rgb[1] = 1;
		lightstyles[i].rgb[2] = 1;
		lightstyles[i].white = 3;
	}
	r_newrefdef.lightstyles = lightstyles;

	//vk_lms.current_lightmap_texture = 1;

	/*
	** if mono lightmaps are enabled and we want to use alpha
	** blending (a,1-a) then we're likely running on a 3DLabs
	** Permedia2.  In a perfect world we'd use a GL_ALPHA lightmap
	** in order to conserve space and maximize bandwidth, however
	** this isn't a perfect world.
	**
	** So we have to use alpha lightmaps, but stored in GL_RGBA format,
	** which means we only get 1/16th the color resolution we should when
	** using alpha lightmaps.  If we find another board that supports
	** only alpha lightmaps but that can at least support the GL_ALPHA
	** format then we should change this code to use real alpha maps.
	*/
	if (toupper(vk_monolightmap->string[0]) == 'A')
	{
		vk_lms.internal_format = gl_tex_alpha_format;
	}
	/*
	** try to do hacked colored lighting with a blended texture
	*/
	else if (toupper(vk_monolightmap->string[0]) == 'C')
	{
		vk_lms.internal_format = gl_tex_alpha_format;
	}
	else if (toupper(vk_monolightmap->string[0]) == 'I')
	{
		//vk_lms.internal_format = GL_INTENSITY8;
	}
	else if (toupper(vk_monolightmap->string[0]) == 'L')
	{
		//vk_lms.internal_format = GL_LUMINANCE8;
	}
	else
	{
		vk_lms.internal_format = gl_tex_solid_format;
	}

	/*
	** initialize the dynamic lightmap textures
	*/
	if (vk_state.lightmap_textures[DYNLIGHTMAP_OFFSET].image == VK_NULL_HANDLE)
	{
		qvktextureopts_t defaultTexOpts = QVVKTEXTUREOPTS_INIT;
		for (i = DYNLIGHTMAP_OFFSET; i < MAX_LIGHTMAPS*2; i++)
		{
			QVVKTEXTURE_CLEAR(vk_state.lightmap_textures[i]);
			vk_state.lightmap_textures[i].format = VK_LIGHTMAP_FORMAT;
			QVk_CreateTexture(&vk_state.lightmap_textures[i], (unsigned char*)dummy, BLOCK_WIDTH, BLOCK_HEIGHT, &defaultTexOpts);
		}
	}
}

/*
=======================
Vk_EndBuildingLightmaps
=======================
*/
void Vk_EndBuildingLightmaps (void)
{
	LM_UploadBlock( false );
}

