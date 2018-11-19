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
// r_main.c
#include "vk_local.h"

void R_Clear (void);

viddef_t	vid;

refimport_t	ri;

model_t		*r_worldmodel;

float		gldepthmin, gldepthmax;

vkconfig_t vk_config;
vkstate_t  vk_state;

image_t		*r_notexture;		// use for bad textures
image_t		*r_particletexture;	// little dot for particles

entity_t	*currententity;
model_t		*currentmodel;

cplane_t	frustum[4];

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

int			c_brush_polys, c_alias_polys;

float		v_blend[4];			// final blending color

void Vk_Strings_f(void);

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_projection_matrix[16];
float	r_view_matrix[16];
// correction matrix for perspective in Vulkan
float	r_vulkan_correction[16] = { 1.f,  0.f, 0.f, 0.f,
									0.f, -1.f, 0.f, 0.f,
									0.f,  0.f, .5f, 0.f,
									0.f,  0.f, .5f, 1.f
								  };

//
// screen size info
//
refdef_t	r_newrefdef;

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_lerpmodels;
cvar_t	*r_lefthand;

cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

cvar_t	*vk_validation;
cvar_t	*vk_mode;
cvar_t	*vk_bitdepth;
cvar_t	*vk_log;
cvar_t	*vk_picmip;
cvar_t	*vk_round_down;
cvar_t	*vk_flashblend;
cvar_t	*vk_finish;
cvar_t	*vk_lockpvs;
cvar_t	*vk_polyblend;

cvar_t	*gl_particle_min_size;
cvar_t	*gl_particle_max_size;
cvar_t	*gl_particle_size;
cvar_t	*gl_particle_att_a;
cvar_t	*gl_particle_att_b;
cvar_t	*gl_particle_att_c;

cvar_t	*gl_ext_swapinterval;
cvar_t	*gl_ext_pointparameters;
cvar_t	*gl_ext_compiled_vertex_array;

cvar_t	*gl_lightmap;
cvar_t	*gl_shadows;
cvar_t	*gl_dynamic;
cvar_t  *gl_monolightmap;
cvar_t	*gl_modulate;
cvar_t	*gl_nobind;
cvar_t	*gl_skymip;
cvar_t	*gl_showtris;
cvar_t	*gl_cull;
cvar_t	*gl_playermip;
cvar_t  *gl_saturatelighting;
cvar_t	*gl_swapinterval;
cvar_t	*gl_texturemode;
cvar_t	*gl_texturealphamode;
cvar_t	*gl_texturesolidmode;

cvar_t	*vid_fullscreen;
cvar_t	*vid_gamma;
cvar_t	*vid_ref;

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

	if (r_nocull->value)
		return false;

	for (i=0 ; i<4 ; i++)
		if ( BOX_ON_PLANE_SIDE(mins, maxs, &frustum[i]) == 2)
			return true;
	return false;
}


void R_RotateForEntity (entity_t *e, float *mvMatrix)
{
	Mat_Rotate(mvMatrix, -e->angles[2], 1.f, 0.f, 0.f);
	Mat_Rotate(mvMatrix, -e->angles[0], 0.f, 1.f, 0.f);
	Mat_Rotate(mvMatrix,  e->angles[1], 0.f, 0.f, 1.f);
	Mat_Translate(mvMatrix, e->origin[0], e->origin[1], e->origin[2]);
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/


/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel (entity_t *e)
{
	float alpha = 1.0F;
	vec3_t	point;
	dsprframe_t	*frame;
	float		*up, *right;
	dsprite_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache

	psprite = (dsprite_t *)currentmodel->extradata;

	e->frame %= psprite->numframes;

	frame = &psprite->frames[e->frame];

	// normal sprite
	up = vup;
	right = vright;

	if (e->flags & RF_TRANSLUCENT)
		alpha = e->alpha;

	/*if (alpha != 1.0F)
		qglEnable(GL_BLEND);

	qglColor4f(1, 1, 1, alpha);

	GL_Bind(currentmodel->skins[e->frame]->texnum);

	GL_TexEnv(GL_MODULATE);

	if (alpha == 1.0)
		qglEnable(GL_ALPHA_TEST);
	else
		qglDisable(GL_ALPHA_TEST);

	qglBegin(GL_QUADS);

	qglTexCoord2f(0, 1);
	VectorMA(e->origin, -frame->origin_y, up, point);
	VectorMA(point, -frame->origin_x, right, point);
	qglVertex3fv(point);

	qglTexCoord2f(0, 0);
	VectorMA(e->origin, frame->height - frame->origin_y, up, point);
	VectorMA(point, -frame->origin_x, right, point);
	qglVertex3fv(point);

	qglTexCoord2f(1, 0);
	VectorMA(e->origin, frame->height - frame->origin_y, up, point);
	VectorMA(point, frame->width - frame->origin_x, right, point);
	qglVertex3fv(point);

	qglTexCoord2f(1, 1);
	VectorMA(e->origin, -frame->origin_y, up, point);
	VectorMA(point, frame->width - frame->origin_x, right, point);
	qglVertex3fv(point);

	qglEnd();

	qglDisable(GL_ALPHA_TEST);
	GL_TexEnv(GL_REPLACE);

	if (alpha != 1.0F)
		qglDisable(GL_BLEND);

	qglColor4f(1, 1, 1, 1);*/
}

//==================================================================================

/*
=============
R_DrawNullModel
=============
*/
void R_DrawNullModel (void)
{
	vec3_t	shadelight;
	int		i,j;

	if (currententity->flags & RF_FULLBRIGHT)
		shadelight[0] = shadelight[1] = shadelight[2] = 1.0F;
	else
		R_LightPoint(currententity->origin, shadelight);

	float mvp[16];
	float viewproj[16];
	float model[16];
	memcpy(model, r_world_matrix, sizeof(float) * 16);
	R_RotateForEntity(currententity, model);
	Mat_Mul(r_view_matrix, r_projection_matrix, viewproj);
	Mat_Mul(model, viewproj, mvp);

	vec3_t verts[24];
	verts[0][0] = 0.f;
	verts[0][1] = 0.f;
	verts[0][2] = -16.f;
	verts[1][0] = shadelight[0];
	verts[1][1] = shadelight[1];
	verts[1][2] = shadelight[2];

	for (i = 2, j = 0; i < 12; i+=2, j++)
	{
		verts[i][0] = 16 * cos(j*M_PI / 2);
		verts[i][1] = 16 * sin(j*M_PI / 2);
		verts[i][2] = 0.f;
		verts[i+1][0] = shadelight[0];
		verts[i+1][1] = shadelight[1];
		verts[i+1][2] = shadelight[2];
	}

	verts[12][0] = 0.f;
	verts[12][1] = 0.f;
	verts[12][2] = 16.f;
	verts[13][0] = shadelight[0];
	verts[13][1] = shadelight[1];
	verts[13][2] = shadelight[2];

	for (i = 23, j = 4; i > 13; i-=2, j--)
	{
		verts[i-1][0] = 16 * cos(j*M_PI / 2);
		verts[i-1][1] = 16 * sin(j*M_PI / 2);
		verts[i-1][2] = 0.f;
		verts[i][0] = shadelight[0];
		verts[i][1] = shadelight[1];
		verts[i][2] = shadelight[2];
	}

	VkBuffer vbo;
	uint32_t vboOffset;
	uint8_t *data = QVk_GetVertexBuffer(sizeof(verts), &vbo, &vboOffset);
	memcpy(data, verts, sizeof(verts));

	uint32_t uboOffset;
	VkDescriptorSet uboDescriptorSet;
	uint8_t *uboData = QVk_GetUniformBuffer(sizeof(mvp), &uboOffset, &uboDescriptorSet);
	memcpy(uboData, &mvp, sizeof(mvp));

	vkCmdBindPipeline(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawNullModel.pl);
	VkDescriptorSet descriptorSets[] = { uboDescriptorSet };
	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawNullModel.layout, 0, 1, descriptorSets, 1, &uboOffset);
	VkDeviceSize offsets = 0;
	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vbo, &offsets);
	vkCmdDraw(vk_activeCmdbuffer, 6, 1, 0, 0);
	vkCmdDraw(vk_activeCmdbuffer, 6, 1, 6, 0);
}

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int		i;

	if (!r_drawentities->value)
		return;

	// draw non-transparent first
	for (i = 0; i<r_newrefdef.num_entities; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (currententity->flags & RF_TRANSLUCENT)
			continue;	// solid

		if (currententity->flags & RF_BEAM)
		{
			R_DrawBeam(currententity);
		}
		else
		{
			currentmodel = currententity->model;
			if (!currentmodel)
			{
				R_DrawNullModel();
				continue;
			}
			switch (currentmodel->type)
			{
			case mod_alias:
				R_DrawAliasModel(currententity);
				break;
			case mod_brush:
				R_DrawBrushModel(currententity);
				break;
			case mod_sprite:
				R_DrawSpriteModel(currententity);
				break;
			default:
				ri.Sys_Error(ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}

	// draw transparent entities
	// we could sort these if it ever becomes a problem...
	//qglDepthMask(0);		// no z writes
	for (i = 0; i<r_newrefdef.num_entities; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (!(currententity->flags & RF_TRANSLUCENT))
			continue;	// solid

		if (currententity->flags & RF_BEAM)
		{
			R_DrawBeam(currententity);
		}
		else
		{
			currentmodel = currententity->model;

			if (!currentmodel)
			{
				R_DrawNullModel();
				continue;
			}
			switch (currentmodel->type)
			{
			case mod_alias:
				R_DrawAliasModel(currententity);
				break;
			case mod_brush:
				R_DrawBrushModel(currententity);
				break;
			case mod_sprite:
				R_DrawSpriteModel(currententity);
				break;
			default:
				ri.Sys_Error(ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}
	//qglDepthMask(1);		// back to writing
}

/*
** GL_DrawParticles
**
*/
void Vk_DrawParticles( int num_particles, const particle_t particles[], const unsigned colortable[768] )
{
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles (void)
{
}

/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	if (!vk_polyblend->value)
		return;
	if (!v_blend[3])
		return;

	/*qglDisable(GL_ALPHA_TEST);
	qglEnable(GL_BLEND);
	qglDisable(GL_DEPTH_TEST);
	qglDisable(GL_TEXTURE_2D);

	qglLoadIdentity();

	// FIXME: get rid of these
	qglRotatef(-90, 1, 0, 0);	    // put Z going up
	qglRotatef(90, 0, 0, 1);	    // put Z going up

	qglColor4fv(v_blend);

	qglBegin(GL_QUADS);

	qglVertex3f(10, 100, 100);
	qglVertex3f(10, -100, 100);
	qglVertex3f(10, -100, -100);
	qglVertex3f(10, 100, -100);
	qglEnd();

	qglDisable(GL_BLEND);
	qglEnable(GL_TEXTURE_2D);
	qglEnable(GL_ALPHA_TEST);

	qglColor4f(1, 1, 1, 1);*/
}

//=======================================================================

int SignbitsForPlane (cplane_t *out)
{
    return 0;
}


void R_SetFrustum (void)
{
	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector(frustum[0].normal, vup, vpn, -(90 - r_newrefdef.fov_x / 2));
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector(frustum[1].normal, vup, vpn, 90 - r_newrefdef.fov_x / 2);
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector(frustum[2].normal, vright, vpn, 90 - r_newrefdef.fov_y / 2);
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector(frustum[3].normal, vright, vpn, -(90 - r_newrefdef.fov_y / 2));

	for (int i = 0; i < 4; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct(r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane(&frustum[i]);
	}
}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	int i;
	mleaf_t	*leaf;

	r_framecount++;

	// build the transformation matrix for the given view angles
	VectorCopy(r_newrefdef.vieworg, r_origin);

	AngleVectors(r_newrefdef.viewangles, vpn, vright, vup);

	// current viewcluster
	if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf(r_origin, r_worldmodel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy(r_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf(temp, r_worldmodel);
			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2))
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy(r_origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf(temp, r_worldmodel);
			if (!(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2))
				r_viewcluster2 = leaf->cluster;
		}
	}

	for (i = 0; i < 4; i++)
		v_blend[i] = r_newrefdef.blend[i];

	c_brush_polys = 0;
	c_alias_polys = 0;

	// clear out the portion of the screen that the NOWORLDMODEL defines
	// unlike OpenGL, draw a rectangle in proper location - it's easier to do in Vulkan
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
	{
		float clearArea[] = { (float)r_newrefdef.x / vid.width, (float)r_newrefdef.y / vid.height,
							  (float)r_newrefdef.width / vid.width, (float)r_newrefdef.height / vid.height,
							  0.f, 0.f, 1.f, 1.f, .3f, .3f, .3f, 1.f };
		uint32_t uboOffset;
		VkDescriptorSet uboDescriptorSet;
		uint8_t *data = QVk_GetUniformBuffer(sizeof(clearArea), &uboOffset, &uboDescriptorSet);
		memcpy(data, &clearArea, sizeof(clearArea));

		vkCmdBindPipeline(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawColorQuadPipeline.pl);
		VkDeviceSize offsets = 0;
		VkDescriptorSet descriptorSets[] = { uboDescriptorSet };
		vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vk_rectVbo.buffer, &offsets);
		vkCmdBindIndexBuffer(vk_activeCmdbuffer, vk_rectIbo.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_drawColorQuadPipeline.layout, 0, 1, descriptorSets, 1, &uboOffset);
		vkCmdDrawIndexed(vk_activeCmdbuffer, 6, 1, 0, 0, 0);
	}
}

void Mat_Identity(float *matrix)
{
	matrix[0] = 1.f;
	matrix[1] = 0.f;
	matrix[2] = 0.f;
	matrix[3] = 0.f;
	matrix[4] = 0.f;
	matrix[5] = 1.f;
	matrix[6] = 0.f;
	matrix[7] = 0.f;
	matrix[8] = 0.f;
	matrix[9] = 0.f;
	matrix[10] = 1.f;
	matrix[11] = 0.f;
	matrix[12] = 0.f;
	matrix[13] = 0.f;
	matrix[14] = 0.f;
	matrix[15] = 1.f;
}

void Mat_Mul(float *m1, float *m2, float *res)
{
	float mul[16] = { m1[0] * m2[0] + m1[1] * m2[4] + m1[2] * m2[8] + m1[3] * m2[12],
					  m1[0] * m2[1] + m1[1] * m2[5] + m1[2] * m2[9] + m1[3] * m2[13],
					  m1[0] * m2[2] + m1[1] * m2[6] + m1[2] * m2[10] + m1[3] * m2[14],
					  m1[0] * m2[3] + m1[1] * m2[7] + m1[2] * m2[11] + m1[3] * m2[15],
					  m1[4] * m2[0] + m1[5] * m2[4] + m1[6] * m2[8] + m1[7] * m2[12],
					  m1[4] * m2[1] + m1[5] * m2[5] + m1[6] * m2[9] + m1[7] * m2[13],
					  m1[4] * m2[2] + m1[5] * m2[6] + m1[6] * m2[10] + m1[7] * m2[14],
					  m1[4] * m2[3] + m1[5] * m2[7] + m1[6] * m2[11] + m1[7] * m2[15],
					  m1[8] * m2[0] + m1[9] * m2[4] + m1[10] * m2[8] + m1[11] * m2[12],
					  m1[8] * m2[1] + m1[9] * m2[5] + m1[10] * m2[9] + m1[11] * m2[13],
					  m1[8] * m2[2] + m1[9] * m2[6] + m1[10] * m2[10] + m1[11] * m2[14],
					  m1[8] * m2[3] + m1[9] * m2[7] + m1[10] * m2[11] + m1[11] * m2[15],
					  m1[12] * m2[0] + m1[13] * m2[4] + m1[14] * m2[8] + m1[15] * m2[12],
					  m1[12] * m2[1] + m1[13] * m2[5] + m1[14] * m2[9] + m1[15] * m2[13],
					  m1[12] * m2[2] + m1[13] * m2[6] + m1[14] * m2[10] + m1[15] * m2[14],
					  m1[12] * m2[3] + m1[13] * m2[7] + m1[14] * m2[11] + m1[15] * m2[15]
	};

	memcpy(res, mul, sizeof(float) * 16);
}

void Mat_Translate(float *matrix, float x, float y, float z)
{
	float t[16] = { 1.f, 0.f, 0.f, 0.f,
					0.f, 1.f, 0.f, 0.f,
					0.f, 0.f, 1.f, 0.f,
					  x,   y,   z, 1.f };

	Mat_Mul(matrix, t, matrix);
}

void Mat_Rotate(float *matrix, float deg, float x, float y, float z)
{
	double c = cos(deg * M_PI / 180.0);
	double s = sin(deg * M_PI / 180.0);
	double cd = 1.0 - c;

	float r[16] = { x*x*cd + c,   y*x*cd + z*s, x*z*cd - y*s,  0.f,
					x*y*cd - z*s, y*y*cd + c,	 y*z*cd + x*s, 0.f,
					x*z*cd + y*s, y*z*cd - x*s, z*z*cd + c,	   0.f,
					0.f,		  0.f,			0.f,		   1.f
	};

	Mat_Mul(matrix, r, matrix);
}

void Mat_Scale(float *matrix, float x, float y, float z)
{
	float s[16] = {   x, 0.f, 0.f, 0.f,
					0.f,   y, 0.f, 0.f,
					0.f, 0.f,   z, 0.f,
					0.f, 0.f, 0.f, 1.f
	};

	Mat_Mul(matrix, s, matrix);
}

void Mat_Perspective(float *matrix, float fovy, float aspect,
	float zNear, float zFar)
{
	float xmin, xmax, ymin, ymax;

	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	xmin += -(2 * vk_state.camera_separation) / zNear;
	xmax += -(2 * vk_state.camera_separation) / zNear;

	float proj[16];
	memset(proj, 0, sizeof(float) * 16);
	proj[0] = 2.f * zNear / (xmax - xmin);
	proj[2] = (xmax + xmin) / (xmax - xmin);
	proj[5] = 2.f * zNear / (ymax - ymin);
	proj[6] = (ymax + ymin) / (ymax - ymin);
	proj[10] = -(zFar + zNear) / (zFar - zNear);
	proj[11] = -1.f;
	proj[14] = -2.f * zFar * zNear / (zFar - zNear);

	// Convert projection matrix to Vulkan coordinate system (https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/)
	Mat_Mul(proj, r_vulkan_correction, matrix);
}


/*
=============
R_SetupVulkan
=============
*/
void R_SetupVulkan (void)
{
	float	screenaspect;
	int		x, x2, y2, y, w, h;

	//
	// set up viewport
	//
	x = floor(r_newrefdef.x * vid.width / vid.width);
	x2 = ceil((r_newrefdef.x + r_newrefdef.width) * vid.width / vid.width);
	y = floor(vid.height - r_newrefdef.y * vid.height / vid.height);
	y2 = ceil(vid.height - (r_newrefdef.y + r_newrefdef.height) * vid.height / vid.height);

	w = x2 - x;
	h = y - y2;

	VkViewport viewport = {
		.x = x,
		.y = vid.height - h - y2,
		.width = w,
		.height = h,
		.minDepth = 0.f,
		.maxDepth = 1.f,
	};
	vkCmdSetViewport(vk_activeCmdbuffer, 0, 1, &viewport);

	// set up projection matrix
	screenaspect = (float)r_newrefdef.width / r_newrefdef.height;
	Mat_Identity(r_projection_matrix);
	Mat_Perspective(r_projection_matrix, r_newrefdef.fov_y, screenaspect, 4, 4096);

	// set up view matrix
	Mat_Identity(r_view_matrix);
	// set up model matrix
	Mat_Identity(r_world_matrix);
	// put Z going up
	Mat_Rotate(r_view_matrix, 90.f, 0.f, 0.f, 1.f);
	Mat_Rotate(r_view_matrix, -90.f, 1.f, 0.f, 0.f);
	Mat_Rotate(r_view_matrix, -r_newrefdef.viewangles[1], 0.f, 0.f, 1.f);
	Mat_Rotate(r_view_matrix, -r_newrefdef.viewangles[0], 0.f, 1.f, 0.f);
	Mat_Rotate(r_view_matrix, -r_newrefdef.viewangles[2], 1.f, 0.f, 0.f);
	Mat_Translate(r_view_matrix, -r_newrefdef.vieworg[0], -r_newrefdef.vieworg[1], -r_newrefdef.vieworg[2]);

	//
	// set drawing parms
	//
	/*if (gl_cull->value)
		qglEnable(GL_CULL_FACE);
	else
		qglDisable(GL_CULL_FACE);

	qglDisable(GL_BLEND);
	qglDisable(GL_ALPHA_TEST);
	qglEnable(GL_DEPTH_TEST);*/
}

/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
}

void R_Flash( void )
{
	R_PolyBlend ();
}

/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/
void R_RenderView (refdef_t *fd)
{
	if (r_norefresh->value)
		return;

	r_newrefdef = *fd;

	if (!r_worldmodel && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
		ri.Sys_Error(ERR_DROP, "R_RenderView: NULL worldmodel");

	if (r_speeds->value)
	{
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	R_PushDlights();

	// added for compatibility sake with OpenGL implementation - don't use it!
	if (vk_finish->value)
		vkDeviceWaitIdle(vk_device.logical);

	R_SetupFrame();

	R_SetFrustum();

	R_SetupVulkan();

	R_MarkLeaves();	// done here so we know if we're in water

	R_DrawWorld();

	R_DrawEntitiesOnList();

	R_RenderDlights();

	R_DrawParticles();

	R_DrawAlphaSurfaces();

	R_Flash();

	if (r_speeds->value)
	{
		ri.Con_Printf(PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps\n",
			c_brush_polys,
			c_alias_polys,
			c_visible_textures,
			c_visible_lightmaps);
	}
}


void R_SetVulkan2D (void)
{
	extern VkViewport vk_viewport;
	vkCmdSetViewport(vk_activeCmdbuffer, 0, 1, &vk_viewport);
	/*
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity ();
	qglOrtho  (0, vid.width, vid.height, 0, -99999, 99999);
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity ();
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_CULL_FACE);
	qglDisable (GL_BLEND);
	qglEnable (GL_ALPHA_TEST);
	*/
}


/*
====================
R_SetLightLevel

====================
*/
void R_SetLightLevel (void)
{
	vec3_t		shadelight;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	// save off light value for server to look at (BIG HACK!)

	R_LightPoint(r_newrefdef.vieworg, shadelight);

	// pick the greatest component, which should be the same
	// as the mono value returned by software
	if (shadelight[0] > shadelight[1])
	{
		if (shadelight[0] > shadelight[2])
			r_lightlevel->value = 150 * shadelight[0];
		else
			r_lightlevel->value = 150 * shadelight[2];
	}
	else
	{
		if (shadelight[1] > shadelight[2])
			r_lightlevel->value = 150 * shadelight[1];
		else
			r_lightlevel->value = 150 * shadelight[2];
	}
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame (refdef_t *fd)
{
	R_RenderView( fd );
	R_SetLightLevel ();
	R_SetVulkan2D ();
}


void R_Register( void )
{
	r_lefthand = ri.Cvar_Get("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	r_norefresh = ri.Cvar_Get("r_norefresh", "0", 0);
	r_fullbright = ri.Cvar_Get("r_fullbright", "0", 0);
	r_drawentities = ri.Cvar_Get("r_drawentities", "1", 0);
	r_drawworld = ri.Cvar_Get("r_drawworld", "1", 0);
	r_novis = ri.Cvar_Get("r_novis", "0", 0);
	r_nocull = ri.Cvar_Get("r_nocull", "0", 0);
	r_lerpmodels = ri.Cvar_Get("r_lerpmodels", "1", 0);
	r_speeds = ri.Cvar_Get("r_speeds", "0", 0);

	r_lightlevel = ri.Cvar_Get("r_lightlevel", "0", 0);

#ifdef _DEBUG
	vk_validation = ri.Cvar_Get("vk_validation", "2", 0);
#else
	vk_validation = ri.Cvar_Get("vk_validation", "0", 0);
#endif
	vk_mode = ri.Cvar_Get("vk_mode", "3", CVAR_ARCHIVE);
	vk_bitdepth = ri.Cvar_Get("vk_bitdepth", "0", 0);
	vk_log = ri.Cvar_Get("vk_log", "0", 0);
	vk_picmip = ri.Cvar_Get("vk_picmip", "0", 0);
	vk_round_down = ri.Cvar_Get("vk_round_down", "1", 0);
	vk_flashblend = ri.Cvar_Get("vk_flashblend", "0", 0);
	vk_finish = ri.Cvar_Get("vk_finish", "0", CVAR_ARCHIVE);
	vk_lockpvs = ri.Cvar_Get("vk_lockpvs", "0", 0);
	vk_polyblend = ri.Cvar_Get("vk_polyblend", "1", 0);

	vid_fullscreen = ri.Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
	vid_gamma = ri.Cvar_Get("vid_gamma", "1.0", CVAR_ARCHIVE);
	vid_ref = ri.Cvar_Get("vid_ref", "soft", CVAR_ARCHIVE);

	ri.Cmd_AddCommand("vk_strings", Vk_Strings_f);
}

/*
==================
R_SetMode
==================
*/
qboolean R_SetMode (void)
{
	rserr_t err;
	qboolean fullscreen;

	fullscreen = vid_fullscreen->value;

	vid_fullscreen->modified = false;
	vk_mode->modified = false;

	if ((err = Vkimp_SetMode(&vid.width, &vid.height, vk_mode->value, fullscreen)) == rserr_ok)
	{
		vk_state.prev_mode = vk_mode->value;
	}
	else
	{
		if (err == rserr_invalid_fullscreen)
		{
			ri.Cvar_SetValue("vid_fullscreen", 0);
			vid_fullscreen->modified = false;
			ri.Con_Printf(PRINT_ALL, "ref_vk::R_SetMode() - fullscreen unavailable in this mode\n");
			if ((err = Vkimp_SetMode(&vid.width, &vid.height, vk_mode->value, false)) == rserr_ok)
				return true;
		}
		else if (err == rserr_invalid_mode)
		{
			ri.Cvar_SetValue("vk_mode", vk_state.prev_mode);
			vk_mode->modified = false;
			ri.Con_Printf(PRINT_ALL, "ref_vk::R_SetMode() - invalid mode\n");
		}

		// try setting it back to something safe
		if ((err = Vkimp_SetMode(&vid.width, &vid.height, vk_state.prev_mode, false)) != rserr_ok)
		{
			ri.Con_Printf(PRINT_ALL, "ref_vk::R_SetMode() - could not revert to safe mode\n");
			return false;
		}
	}
	return true;
}

/*
===============
R_Init
===============
*/
qboolean R_Init( void *hinstance, void *hWnd )
{
	ri.Con_Printf(PRINT_ALL, "ref_vk version: "REF_VERSION"\n");

	R_Register();

	// create the window (OS-specific)
	if (!Vkimp_Init(hinstance, hWnd))
	{
		return false;
	}

	// set our "safe" modes
	vk_state.prev_mode = 3;
	// set video mode/screen resolution
	if (!R_SetMode())
	{
		ri.Con_Printf(PRINT_ALL, "ref_vk::R_Init() - could not R_SetMode()\n");
		return false;
	}
	ri.Vid_MenuInit();

	// window is ready, initialize Vulkan now
	if (!QVk_Init())
	{
		ri.Con_Printf(PRINT_ALL, "ref_vk::R_Init() - could not initialize Vulkan!\n");
		return false;
	}

	ri.Con_Printf(PRINT_ALL, "Successfully initialized Vulkan!\n");

	Vk_InitImages();
	Mod_Init();
	R_InitParticleTexture();
	Draw_InitLocal();

	return true;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown (void)
{
	ri.Cmd_RemoveCommand("vk_strings");

	vkDeviceWaitIdle(vk_device.logical);

	Mod_FreeAll();
	Vk_ShutdownImages();

	// Shutdown Vulkan subsystem
	QVk_Shutdown();
	// shut down OS specific Vulkan stuff (in our case: window)
	Vkimp_Shutdown();
}



/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginFrame( float camera_separation )
{
	/*
	** change modes if necessary
	*/
	if (vk_mode->modified || vid_fullscreen->modified)
	{
		cvar_t	*ref = ri.Cvar_Get("vid_ref", "vk", 0);
		ref->modified = true;
	}

	if (vk_log->modified)
	{
		Vkimp_EnableLogging(vk_log->value);
		vk_log->modified = false;
	}

	if (vk_log->value)
	{
		Vkimp_LogNewFrame();
	}

	Vkimp_BeginFrame(camera_separation);
	QVk_BeginFrame();
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_EndFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void R_EndFrame( void )
{
	QVk_EndFrame();
}

/*
=============
R_SetPalette
=============
*/
unsigned r_rawpalette[256];

void R_SetPalette ( const unsigned char *palette)
{
	int		i;

	byte *rp = (byte *)r_rawpalette;

	if (palette)
	{
		for (i = 0; i < 256; i++)
		{
			rp[i * 4 + 0] = palette[i * 3 + 0];
			rp[i * 4 + 1] = palette[i * 3 + 1];
			rp[i * 4 + 2] = palette[i * 3 + 2];
			rp[i * 4 + 3] = 0xff;
		}
	}
	else
	{
		for (i = 0; i < 256; i++)
		{
			rp[i * 4 + 0] = d_8to24table[i] & 0xff;
			rp[i * 4 + 1] = (d_8to24table[i] >> 8) & 0xff;
			rp[i * 4 + 2] = (d_8to24table[i] >> 16) & 0xff;
			rp[i * 4 + 3] = 0xff;
		}
	}
}

/*
** R_DrawBeam
*/
void R_DrawBeam( entity_t *e )
{

}

//===================================================================


void	R_BeginRegistration (char *map);
struct model_s	*R_RegisterModel (char *name);
struct image_s	*R_RegisterSkin (char *name);
void R_SetSky (char *name, float rotate, vec3_t axis);
void	R_EndRegistration (void);

void	R_RenderFrame (refdef_t *fd);

struct image_s	*Draw_FindPic (char *name);

void	Draw_Pic (int x, int y, char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_TileClear (int x, int y, int w, int h, char *name);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_FadeScreen (void);

/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t GetRefAPI (refimport_t rimp )
{
	refexport_t	re;

	ri = rimp;

	re.api_version = API_VERSION;

	re.BeginRegistration = R_BeginRegistration;
	re.RegisterModel = R_RegisterModel;
	re.RegisterSkin = R_RegisterSkin;
	re.RegisterPic = Draw_FindPic;
	re.SetSky = R_SetSky;
	re.EndRegistration = R_EndRegistration;

	re.RenderFrame = R_RenderFrame;

	re.DrawGetPicSize = Draw_GetPicSize;
	re.DrawPic = Draw_Pic;
	re.DrawStretchPic = Draw_StretchPic;
	re.DrawChar = Draw_Char;
	re.DrawTileClear = Draw_TileClear;
	re.DrawFill = Draw_Fill;
	re.DrawFadeScreen= Draw_FadeScreen;

	re.DrawStretchRaw = Draw_StretchRaw;

	re.Init = R_Init;
	re.Shutdown = R_Shutdown;

	re.CinematicSetPalette = R_SetPalette;
	re.BeginFrame = R_BeginFrame;
	re.EndFrame = R_EndFrame;

	re.AppActivate = Vkimp_AppActivate;

	Swap_Init ();

	return re;
}


#ifndef REF_HARD_LINKED
// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	vsprintf (text, error, argptr);
	va_end (argptr);

	ri.Sys_Error (ERR_FATAL, "%s", text);
}

void Com_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, fmt);
	vsprintf (text, fmt, argptr);
	va_end (argptr);

	ri.Con_Printf (PRINT_ALL, "%s", text);
}

#endif
