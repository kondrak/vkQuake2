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
// vk_mesh.c: triangle model functions

#include "vk_local.h"

/*
=============================================================

  ALIAS MODELS

=============================================================
*/

#define NUMVERTEXNORMALS	162

float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

typedef float vec4_t[4];

static	vec4_t	s_lerped[MAX_VERTS];

vec3_t	shadevector;
float	shadelight[3];

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "anormtab.h"
;

float	*shadedots = r_avertexnormal_dots[0];

extern float r_view_matrix[16];
extern float r_projection_matrix[16];
extern float r_viewproj_matrix[16];

// correction matrix with "hacked depth" for models with RF_DEPTHHACK flag set
static float r_vulkan_correction_dh[16] = { 1.f,  0.f, 0.f, 0.f,
											0.f, -1.f, 0.f, 0.f,
											0.f,  0.f, .3f, 0.f,
											0.f,  0.f, .3f, 1.f
										  };

void Vk_LerpVerts( int nverts, dtrivertx_t *v, dtrivertx_t *ov, dtrivertx_t *verts, float *lerp, float move[3], float frontv[3], float backv[3] )
{
	int i;

	//PMM -- added RF_SHELL_DOUBLE, RF_SHELL_HALF_DAM
	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) )
	{
		for (i=0 ; i < nverts; i++, v++, ov++, lerp+=4 )
		{
			float *normal = r_avertexnormals[verts[i].lightnormalindex];

			lerp[0] = move[0] + ov->v[0]*backv[0] + v->v[0]*frontv[0] + normal[0] * POWERSUIT_SCALE;
			lerp[1] = move[1] + ov->v[1]*backv[1] + v->v[1]*frontv[1] + normal[1] * POWERSUIT_SCALE;
			lerp[2] = move[2] + ov->v[2]*backv[2] + v->v[2]*frontv[2] + normal[2] * POWERSUIT_SCALE; 
		}
	}
	else
	{
		for (i=0 ; i < nverts; i++, v++, ov++, lerp+=4)
		{
			lerp[0] = move[0] + ov->v[0]*backv[0] + v->v[0]*frontv[0];
			lerp[1] = move[1] + ov->v[1]*backv[1] + v->v[1]*frontv[1];
			lerp[2] = move[2] + ov->v[2]*backv[2] + v->v[2]*frontv[2];
		}
	}

}

/*
=============
Vk_DrawAliasFrameLerp

interpolates between two frames and origins
FIXME: batch lerp all vertexes
=============
*/
void Vk_DrawAliasFrameLerp (dmdl_t *paliashdr, float backlerp, image_t *skin, float *modelMatrix, int leftHandOffset, int translucentIdx)
{
	float 	l;
	daliasframe_t	*frame, *oldframe;
	dtrivertx_t	*v, *ov, *verts;
	int		*order;
	int		count;
	float	frontlerp;
	float	alpha;
	vec3_t	move, delta, vectors[3];
	vec3_t	frontv, backv;
	int		i;
	int		index_xyz;
	float	*lerp;

	frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames 
		+ currententity->frame * paliashdr->framesize);
	verts = v = frame->verts;

	oldframe = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames 
		+ currententity->oldframe * paliashdr->framesize);
	ov = oldframe->verts;

	order = (int *)((byte *)paliashdr + paliashdr->ofs_glcmds);

	if (currententity->flags & RF_TRANSLUCENT)
		alpha = currententity->alpha;
	else
		alpha = 1.0;

	frontlerp = 1.0 - backlerp;

	// move should be the delta back to the previous frame * backlerp
	VectorSubtract (currententity->oldorigin, currententity->origin, delta);
	AngleVectors (currententity->angles, vectors[0], vectors[1], vectors[2]);

	move[0] = DotProduct (delta, vectors[0]);	// forward
	move[1] = -DotProduct (delta, vectors[1]);	// left
	move[2] = DotProduct (delta, vectors[2]);	// up

	VectorAdd (move, oldframe->translate, move);

	for (i=0 ; i<3 ; i++)
	{
		move[i] = backlerp*move[i] + frontlerp*frame->translate[i];
	}

	for (i=0 ; i<3 ; i++)
	{
		frontv[i] = frontlerp*frame->scale[i];
		backv[i] = backlerp*oldframe->scale[i];
	}

	lerp = s_lerped[0];

	Vk_LerpVerts( paliashdr->num_xyz, v, ov, verts, lerp, move, frontv, backv );

	enum {
		TRIANGLE_STRIP = 0,
		TRIANGLE_FAN = 1
	} pipelineIdx;

	typedef struct {
		float vertex[3];
		float color[4];
		float texCoord[2];
	} modelvert;

	int vertCounts[2] = { 0, 0 };
	static modelvert vertList[2][MAX_VERTS];
	int pipeCounters[2] = { 0, 0 };
	VkDeviceSize maxTriangleFanIdxCnt = 0;

	static struct {
		int vertexCount;
		int firstVertex;
	} drawInfo[2][MAX_VERTS];

	drawInfo[0][0].firstVertex = 0;
	drawInfo[1][0].firstVertex = 0;

	struct {
		float model[16];
		int textured;
	} meshUbo;

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		if (count < 0)
		{
			count = -count;
			pipelineIdx = TRIANGLE_FAN;
		}
		else
		{
			pipelineIdx = TRIANGLE_STRIP;
		}

		drawInfo[pipelineIdx][pipeCounters[pipelineIdx]].vertexCount = count;
		maxTriangleFanIdxCnt = max(maxTriangleFanIdxCnt, ((count - 2) * 3));

		if (currententity->flags & (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE))
		{
			meshUbo.textured = 0;
			do
			{
				int vertIdx = vertCounts[pipelineIdx];
				// unused in this case, since texturing is disabled
				vertList[pipelineIdx][vertIdx].texCoord[0] = 0.f;
				vertList[pipelineIdx][vertIdx].texCoord[1] = 0.f;
				index_xyz = order[2];
				order += 3;

				vertList[pipelineIdx][vertIdx].color[0] = shadelight[0];
				vertList[pipelineIdx][vertIdx].color[1] = shadelight[1];
				vertList[pipelineIdx][vertIdx].color[2] = shadelight[2];
				vertList[pipelineIdx][vertIdx].color[3] = alpha;

				vertList[pipelineIdx][vertIdx].vertex[0] = s_lerped[index_xyz][0];
				vertList[pipelineIdx][vertIdx].vertex[1] = s_lerped[index_xyz][1];
				vertList[pipelineIdx][vertIdx].vertex[2] = s_lerped[index_xyz][2];
				vertCounts[pipelineIdx]++;
			} while (--count);
		}
		else
		{
			meshUbo.textured = 1;
			do
			{
				int vertIdx = vertCounts[pipelineIdx];
				// texture coordinates come from the draw list
				vertList[pipelineIdx][vertIdx].texCoord[0] = ((float *)order)[0];
				vertList[pipelineIdx][vertIdx].texCoord[1] = ((float *)order)[1];
				index_xyz = order[2];
				order += 3;

				// normals and vertexes come from the frame list
				l = shadedots[verts[index_xyz].lightnormalindex];

				vertList[pipelineIdx][vertIdx].color[0] = l * shadelight[0];
				vertList[pipelineIdx][vertIdx].color[1] = l * shadelight[1];
				vertList[pipelineIdx][vertIdx].color[2] = l * shadelight[2];
				vertList[pipelineIdx][vertIdx].color[3] = alpha;

				vertList[pipelineIdx][vertIdx].vertex[0] = s_lerped[index_xyz][0];
				vertList[pipelineIdx][vertIdx].vertex[1] = s_lerped[index_xyz][1];
				vertList[pipelineIdx][vertIdx].vertex[2] = s_lerped[index_xyz][2];
				vertCounts[pipelineIdx]++;
			} while (--count);
		}

		pipeCounters[pipelineIdx]++;
		drawInfo[pipelineIdx][pipeCounters[pipelineIdx]].firstVertex = vertCounts[pipelineIdx];
	}

	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *uboData = QVk_GetUniformBuffer(sizeof(meshUbo), &uboOffset, &uboDescriptorSet);
	memcpy(meshUbo.model, modelMatrix, sizeof(float) * 16);
	memcpy(uboData, &meshUbo, sizeof(meshUbo));

	// player configuration screen model is using the UI renderpass
	int pidx = r_newrefdef.rdflags & RDF_NOWORLDMODEL ? RP_UI : RP_WORLD;
	// non-depth write alias models don't occur with RF_WEAPONMODEL set, so no need for additional left-handed pipelines
	qvkpipeline_t pipelines[2][4] = { { vk_drawModelPipelineStrip[pidx], vk_drawModelPipelineFan[pidx], vk_drawLefthandModelPipelineStrip, vk_drawLefthandModelPipelineFan },
									  { vk_drawNoDepthModelPipelineStrip, vk_drawNoDepthModelPipelineFan, vk_drawLefthandModelPipelineStrip, vk_drawLefthandModelPipelineFan } };
	for (int p = 0; p < 2; p++)
	{
		VkDeviceSize vaoSize = sizeof(modelvert) * vertCounts[p];
		VkBuffer vbo;
		VkDeviceSize vboOffset;
		uint8_t *vertData = QVk_GetVertexBuffer(vaoSize, &vbo, &vboOffset);
		memcpy(vertData, vertList[p], vaoSize);

		QVk_BindPipeline(&pipelines[translucentIdx][p + leftHandOffset]);
		VkDescriptorSet descriptorSets[] = { skin->vk_texture.descriptorSet, uboDescriptorSet };
		vkCmdPushConstants(vk_activeCmdbuffer, pipelines[translucentIdx][p + leftHandOffset].layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(r_viewproj_matrix), r_viewproj_matrix);
		vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[translucentIdx][p + leftHandOffset].layout, 0, 2, descriptorSets, 1, &uboOffset);
		vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);

		if (p == TRIANGLE_STRIP)
		{
			for (i = 0; i < pipeCounters[p]; i++)
			{
				vkCmdDraw(vk_activeCmdbuffer, drawInfo[p][i].vertexCount, 1, drawInfo[p][i].firstVertex, 0);
			}
		}
		else
		{
			vkCmdBindIndexBuffer(vk_activeCmdbuffer, QVk_GetTriangleFanIbo(maxTriangleFanIdxCnt), 0, VK_INDEX_TYPE_UINT16);

			for (i = 0; i < pipeCounters[p]; i++)
			{
				vkCmdDrawIndexed(vk_activeCmdbuffer, (drawInfo[p][i].vertexCount - 2) * 3, 1, 0, drawInfo[p][i].firstVertex, 0);
			}
		}
	}
}


/*
=============
Vk_DrawAliasShadow
=============
*/
extern	vec3_t			lightspot;

void Vk_DrawAliasShadow (dmdl_t *paliashdr, int posenum, float *modelMatrix)
{
	dtrivertx_t	*verts;
	int		*order;
	vec3_t	point;
	float	height, lheight;
	int		count;
	int		i;
	daliasframe_t	*frame;
	qvkpipeline_t pipelines[2] = { vk_shadowsPipelineStrip, vk_shadowsPipelineFan };

	enum {
		TRIANGLE_STRIP = 0,
		TRIANGLE_FAN = 1
	} pipelineIdx;

	lheight = currententity->origin[2] - lightspot[2];

	frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames 
		+ currententity->frame * paliashdr->framesize);
	verts = frame->verts;

	height = 0;

	order = (int *)((byte *)paliashdr + paliashdr->ofs_glcmds);

	height = -lheight + 1.0;

	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *uboData = QVk_GetUniformBuffer(sizeof(float) * 16, &uboOffset, &uboDescriptorSet);
	memcpy(uboData, modelMatrix, sizeof(float) * 16);

	static vec3_t shadowverts[MAX_VERTS];
	while (1)
	{
		i = 0;
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		if (count < 0)
		{
			count = -count;
			pipelineIdx = TRIANGLE_FAN;
		}
		else
		{
			pipelineIdx = TRIANGLE_STRIP;
		}

		do
		{
			// normals and vertexes come from the frame list
			memcpy( point, s_lerped[order[2]], sizeof( point ) );

			point[0] -= shadevector[0]*(point[2]+lheight);
			point[1] -= shadevector[1]*(point[2]+lheight);
			point[2] = height;

			shadowverts[i][0] = point[0];
			shadowverts[i][1] = point[1];
			shadowverts[i][2] = point[2];

			order += 3;
			i++;
		} while (--count);

		if (i > 0)
		{
			VkDeviceSize vaoSize = sizeof(vec3_t) * i;
			VkBuffer vbo;
			VkDeviceSize vboOffset;
			uint8_t *vertData = QVk_GetVertexBuffer(vaoSize, &vbo, &vboOffset);
			memcpy(vertData, shadowverts, vaoSize);

			QVk_BindPipeline(&pipelines[pipelineIdx]);
			vkCmdPushConstants(vk_activeCmdbuffer, pipelines[pipelineIdx].layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(r_viewproj_matrix), r_viewproj_matrix);
			vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines[pipelineIdx].layout, 0, 1, &uboDescriptorSet, 1, &uboOffset);
			vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &vboOffset);

			if (pipelineIdx == TRIANGLE_STRIP)
			{
				vkCmdDraw(vk_activeCmdbuffer, i, 1, 0, 0);
			}
			else
			{
				vkCmdBindIndexBuffer(vk_activeCmdbuffer, QVk_GetTriangleFanIbo((i - 2) * 3), 0, VK_INDEX_TYPE_UINT16);
				vkCmdDrawIndexed(vk_activeCmdbuffer, (i - 2) * 3, 1, 0, 0, 0);
			}
		}
	}
}


/*
** R_CullAliasModel
*/
static qboolean R_CullAliasModel( vec3_t bbox[8], entity_t *e )
{
	int i;
	vec3_t		mins, maxs;
	dmdl_t		*paliashdr;
	vec3_t		vectors[3];
	vec3_t		thismins, oldmins, thismaxs, oldmaxs;
	daliasframe_t *pframe, *poldframe;
	vec3_t angles;

	paliashdr = (dmdl_t *)currentmodel->extradata;

	if ( ( e->frame >= paliashdr->num_frames ) || ( e->frame < 0 ) )
	{
		ri.Con_Printf (PRINT_ALL, "R_CullAliasModel %s: no such frame %d\n", 
			currentmodel->name, e->frame);
		e->frame = 0;
	}
	if ( ( e->oldframe >= paliashdr->num_frames ) || ( e->oldframe < 0 ) )
	{
		ri.Con_Printf (PRINT_ALL, "R_CullAliasModel %s: no such oldframe %d\n", 
			currentmodel->name, e->oldframe);
		e->oldframe = 0;
	}

	pframe = ( daliasframe_t * ) ( ( byte * ) paliashdr + 
		                              paliashdr->ofs_frames +
									  e->frame * paliashdr->framesize);

	poldframe = ( daliasframe_t * ) ( ( byte * ) paliashdr + 
		                              paliashdr->ofs_frames +
									  e->oldframe * paliashdr->framesize);

	/*
	** compute axially aligned mins and maxs
	*/
	if ( pframe == poldframe )
	{
		for ( i = 0; i < 3; i++ )
		{
			mins[i] = pframe->translate[i];
			maxs[i] = mins[i] + pframe->scale[i]*255;
		}
	}
	else
	{
		for ( i = 0; i < 3; i++ )
		{
			thismins[i] = pframe->translate[i];
			thismaxs[i] = thismins[i] + pframe->scale[i]*255;

			oldmins[i]  = poldframe->translate[i];
			oldmaxs[i]  = oldmins[i] + poldframe->scale[i]*255;

			if ( thismins[i] < oldmins[i] )
				mins[i] = thismins[i];
			else
				mins[i] = oldmins[i];

			if ( thismaxs[i] > oldmaxs[i] )
				maxs[i] = thismaxs[i];
			else
				maxs[i] = oldmaxs[i];
		}
	}

	/*
	** compute a full bounding box
	*/
	for ( i = 0; i < 8; i++ )
	{
		vec3_t   tmp;

		if ( i & 1 )
			tmp[0] = mins[0];
		else
			tmp[0] = maxs[0];

		if ( i & 2 )
			tmp[1] = mins[1];
		else
			tmp[1] = maxs[1];

		if ( i & 4 )
			tmp[2] = mins[2];
		else
			tmp[2] = maxs[2];

		VectorCopy( tmp, bbox[i] );
	}

	/*
	** rotate the bounding box
	*/
	VectorCopy( e->angles, angles );
	angles[YAW] = -angles[YAW];
	AngleVectors( angles, vectors[0], vectors[1], vectors[2] );

	for ( i = 0; i < 8; i++ )
	{
		vec3_t tmp;

		VectorCopy( bbox[i], tmp );

		bbox[i][0] = DotProduct( vectors[0], tmp );
		bbox[i][1] = -DotProduct( vectors[1], tmp );
		bbox[i][2] = DotProduct( vectors[2], tmp );

		VectorAdd( e->origin, bbox[i], bbox[i] );
	}

	{
		int p, f, aggregatemask = ~0;

		for ( p = 0; p < 8; p++ )
		{
			int mask = 0;

			for ( f = 0; f < 4; f++ )
			{
				float dp = DotProduct( frustum[f].normal, bbox[p] );

				if ( ( dp - frustum[f].dist ) < 0 )
				{
					mask |= ( 1 << f );
				}
			}

			aggregatemask &= mask;
		}

		if ( aggregatemask )
		{
			return true;
		}

		return false;
	}
}

/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel (entity_t *e)
{
	int			i;
	int			leftHandOffset = 0;
	dmdl_t		*paliashdr;
	float		an;
	vec3_t		bbox[8];
	image_t		*skin;
	float		prev_viewproj[16];

	if ( !( e->flags & RF_WEAPONMODEL ) )
	{
		if ( R_CullAliasModel( bbox, e ) )
			return;
	}

	if ( e->flags & RF_WEAPONMODEL )
	{
		if ( r_lefthand->value == 2 )
			return;
	}

	paliashdr = (dmdl_t *)currentmodel->extradata;

	//
	// get lighting information
	//
	// PMM - rewrote, reordered to handle new shells & mixing
	// PMM - 3.20 code .. replaced with original way of doing it to keep mod authors happy
	//
	if ( currententity->flags & ( RF_SHELL_HALF_DAM | RF_SHELL_GREEN | RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE ) )
	{
		VectorClear (shadelight);
		if (currententity->flags & RF_SHELL_HALF_DAM)
		{
				shadelight[0] = 0.56;
				shadelight[1] = 0.59;
				shadelight[2] = 0.45;
		}
		if ( currententity->flags & RF_SHELL_DOUBLE )
		{
			shadelight[0] = 0.9;
			shadelight[1] = 0.7;
		}
		if ( currententity->flags & RF_SHELL_RED )
			shadelight[0] = 1.0;
		if ( currententity->flags & RF_SHELL_GREEN )
			shadelight[1] = 1.0;
		if ( currententity->flags & RF_SHELL_BLUE )
			shadelight[2] = 1.0;
	}
	else if ( currententity->flags & RF_FULLBRIGHT )
	{
		for (i=0 ; i<3 ; i++)
			shadelight[i] = 1.0;
	}
	else
	{
		R_LightPoint (currententity->origin, shadelight);

		// player lighting hack for communication back to server
		// big hack!
		if ( currententity->flags & RF_WEAPONMODEL )
		{
			// pick the greatest component, which should be the same
			// as the mono value returned by software
			if (shadelight[0] > shadelight[1])
			{
				if (shadelight[0] > shadelight[2])
					r_lightlevel->value = 150*shadelight[0];
				else
					r_lightlevel->value = 150*shadelight[2];
			}
			else
			{
				if (shadelight[1] > shadelight[2])
					r_lightlevel->value = 150*shadelight[1];
				else
					r_lightlevel->value = 150*shadelight[2];
			}
		}
	}

	if ( currententity->flags & RF_MINLIGHT )
	{
		for (i=0 ; i<3 ; i++)
			if (shadelight[i] > 0.1)
				break;
		if (i == 3)
		{
			shadelight[0] = 0.1;
			shadelight[1] = 0.1;
			shadelight[2] = 0.1;
		}
	}

	if ( currententity->flags & RF_GLOW )
	{	// bonus items will pulse with time
		float	scale;
		float	min;

		scale = 0.1 * sin(r_newrefdef.time*7);
		for (i=0 ; i<3 ; i++)
		{
			min = shadelight[i] * 0.8;
			shadelight[i] += scale;
			if (shadelight[i] < min)
				shadelight[i] = min;
		}
	}

// =================
// PGM	ir goggles color override
	if ( r_newrefdef.rdflags & RDF_IRGOGGLES && currententity->flags & RF_IR_VISIBLE)
	{
		shadelight[0] = 1.0;
		shadelight[1] = 0.0;
		shadelight[2] = 0.0;
	}
// PGM	
// =================

	shadedots = r_avertexnormal_dots[((int)(currententity->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];
	
	an = currententity->angles[1]/180*M_PI;
	shadevector[0] = cos(-an);
	shadevector[1] = sin(-an);
	shadevector[2] = 1;
	VectorNormalize (shadevector);

	//
	// locate the proper data
	//

	c_alias_polys += paliashdr->num_tris;

	//
	// draw all the triangles
	//
	if (currententity->flags & RF_DEPTHHACK || r_newrefdef.rdflags & RDF_NOWORLDMODEL) { // hack the depth range to prevent view model from poking into walls
		extern float r_proj_aspect, r_proj_fovy;
		// use different range for player setup screen so it doesn't collide with the viewmodel
		r_vulkan_correction_dh[10] = 0.3f - (r_newrefdef.rdflags & RDF_NOWORLDMODEL) * 0.1f;
		r_vulkan_correction_dh[14] = 0.3f - (r_newrefdef.rdflags & RDF_NOWORLDMODEL) * 0.1f;

		memcpy(prev_viewproj, r_viewproj_matrix, sizeof(r_viewproj_matrix));
		Mat_Perspective(r_projection_matrix, r_vulkan_correction_dh, r_proj_fovy, r_proj_aspect, 4, 4096);
		Mat_Mul(r_view_matrix, r_projection_matrix, r_viewproj_matrix);
	}

	if ( ( currententity->flags & RF_WEAPONMODEL ) && ( r_lefthand->value == 1.0F ) )
	{
		Mat_Scale(r_viewproj_matrix, -1.f, 1.f, 1.f);
		leftHandOffset = 2;
	}

	e->angles[PITCH] = -e->angles[PITCH];	// sigh.
	float model[16];
	Mat_Identity(model);
	R_RotateForEntity (e, model);
	e->angles[PITCH] = -e->angles[PITCH];	// sigh.

	// select skin
	if (currententity->skin)
		skin = currententity->skin;	// custom player skin
	else
	{
		if (currententity->skinnum >= MAX_MD2SKINS)
			skin = currentmodel->skins[0];
		else
		{
			skin = currentmodel->skins[currententity->skinnum];
			if (!skin)
				skin = currentmodel->skins[0];
		}
	}
	if (!skin)
		skin = r_notexture;	// fallback...

	// draw it
	if ( (currententity->frame >= paliashdr->num_frames) 
		|| (currententity->frame < 0) )
	{
		ri.Con_Printf (PRINT_ALL, "R_DrawAliasModel %s: no such frame %d\n",
			currentmodel->name, currententity->frame);
		currententity->frame = 0;
		currententity->oldframe = 0;
	}

	if ( (currententity->oldframe >= paliashdr->num_frames)
		|| (currententity->oldframe < 0))
	{
		ri.Con_Printf (PRINT_ALL, "R_DrawAliasModel %s: no such oldframe %d\n",
			currentmodel->name, currententity->oldframe);
		currententity->frame = 0;
		currententity->oldframe = 0;
	}

	if ( !r_lerpmodels->value )
		currententity->backlerp = 0;
	Vk_DrawAliasFrameLerp (paliashdr, currententity->backlerp, skin, model, leftHandOffset, currententity->flags & RF_TRANSLUCENT ? 1 : 0);

	if ( ( currententity->flags & RF_WEAPONMODEL ) && ( r_lefthand->value == 1.0F ) )
	{
		Mat_Scale(r_viewproj_matrix, -1.f, 1.f, 1.f);
	}

	if (currententity->flags & RF_DEPTHHACK || r_newrefdef.rdflags & RDF_NOWORLDMODEL)
	{
		memcpy(r_viewproj_matrix, prev_viewproj, sizeof(prev_viewproj));
	}

	if (vk_shadows->value && !(currententity->flags & (RF_TRANSLUCENT | RF_WEAPONMODEL)))
	{
		float model[16];
		Mat_Identity(model);
		R_RotateForEntity(e, model);
		Vk_DrawAliasShadow (paliashdr, currententity->frame, model);
	}
}
