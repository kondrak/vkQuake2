/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// r_light.c

#include "r_local.h"

int	r_dlightframecount;


/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
void R_MarkLights(dlight_t *light, int bit, mnode_t *node)
{
	mplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;

	if (node->contents != -1)
		return;

	splitplane = node->plane;
	dist = DotProduct(light->origin, splitplane->normal) - splitplane->dist;

	//=====
	//PGM
	i = light->intensity;
	if (i<0)
		i = -i;
	//PGM
	//=====

	if (dist > i)	// PGM (dist > light->intensity)
	{
		R_MarkLights(light, bit, node->children[0]);
		return;
	}
	if (dist < -i)	// PGM (dist < -light->intensity)
	{
		R_MarkLights(light, bit, node->children[1]);
		return;
	}

	// mark the polygons
	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i = 0; i < node->numsurfaces; i++, surf++)
	{
		if (surf->dlightframe != r_dlightframecount)
		{
			surf->dlightbits = 0;
			surf->dlightframe = r_dlightframecount;
		}
		surf->dlightbits |= bit;
	}

	R_MarkLights(light, bit, node->children[0]);
	R_MarkLights(light, bit, node->children[1]);
}


/*
=============
R_PushDlights
=============
*/
void R_PushDlights(model_t *model)
{
	int		i;
	dlight_t	*l;

	r_dlightframecount = r_framecount;
	for (i = 0, l = r_newrefdef.dlights; i < r_newrefdef.num_dlights; i++, l++)
	{
		R_MarkLights(l, 1 << i,
			model->nodes + model->firstnode);
	}
}


/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

vec3_t	pointcolor;
mplane_t		*lightplane;		// used as shadow plane
vec3_t			lightspot;

int RecursiveLightPoint(mnode_t *node, vec3_t start, vec3_t end)
{
	float		front, back, frac;
	int			side;
	mplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int			s, t, ds, dt;
	int			i;
	mtexinfo_t	*tex;
	byte		*lightmap;
	float		*scales;
	int			maps;
	float		samp;
	int			r;

	if (node->contents != -1)
		return -1;		// didn't hit anything

	// calculate mid point

	// FIXME: optimize for axial
	plane = node->plane;
	front = DotProduct(start, plane->normal) - plane->dist;
	back = DotProduct(end, plane->normal) - plane->dist;
	side = front < 0;

	if ((back < 0) == side)
		return RecursiveLightPoint(node->children[side], start, end);

	frac = front / (front - back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;
	if (plane->type < 3)	// axial planes
		mid[plane->type] = plane->dist;

	// go down front side	
	r = RecursiveLightPoint(node->children[side], start, mid);
	if (r >= 0)
		return r;		// hit something

	if ((back < 0) == side)
		return -1;		// didn't hit anuthing

	// check for impact on this node
	VectorCopy(mid, lightspot);
	lightplane = plane;

	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i = 0; i < node->numsurfaces; i++, surf++)
	{
		if (surf->flags&(SURF_DRAWTURB | SURF_DRAWSKY))
			continue;	// no lightmaps

		tex = surf->texinfo;

		s = DotProduct(mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct(mid, tex->vecs[1]) + tex->vecs[1][3];
		if (s < surf->texturemins[0] ||
			t < surf->texturemins[1])
			continue;

		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];

		if (ds > surf->extents[0] || dt > surf->extents[1])
			continue;

		if (!surf->samples)
			return 0;

		ds >>= 4;
		dt >>= 4;

		lightmap = surf->samples;
		VectorCopy(vec3_origin, pointcolor);
		if (lightmap)
		{
			lightmap += dt * ((surf->extents[0] >> 4) + 1) + ds;

			for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255;
				maps++)
			{
				samp = *lightmap * /* 0.5 * */ (1.0 / 255);	// adjust for gl scale
				scales = r_newrefdef.lightstyles[surf->styles[maps]].rgb;
				VectorMA(pointcolor, samp, scales, pointcolor);
				lightmap += ((surf->extents[0] >> 4) + 1) *
					((surf->extents[1] >> 4) + 1);
			}
		}

		return 1;
	}

	// go down back side
	return RecursiveLightPoint(node->children[!side], mid, end);
}


/*
===============
R_LightPoint
===============
*/

void R_LightPoint(vec3_t p, vec3_t color)
{
	vec3_t		end;
	float		r;
	int			lnum;
	dlight_t	*dl;
	vec3_t		dist;
	float		add;

	if (!r_worldmodel->lightdata)
	{
		color[0] = color[1] = color[2] = 1.0;
		return;
	}

	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;

	r = RecursiveLightPoint(r_worldmodel->nodes, p, end);

	if (r == -1)
	{
		VectorCopy(vec3_origin, color);
	}
	else
	{
		VectorCopy(pointcolor, color);
	}

	//
	// add dynamic lights
	//
	for (lnum = 0; lnum<r_newrefdef.num_dlights; lnum++)
	{
		dl = &r_newrefdef.dlights[lnum];
		VectorSubtract(currententity->origin,
			dl->origin,
			dist);
		add = dl->intensity - VectorLength(dist);
		add *= (1.0 / 256);
		if (add > 0)
		{
			VectorMA(color, add, dl->color, color);
		}
	}
}





// Colored...


int RecursiveLightPointColor(mnode_t *node, vec3_t start, vec3_t end)
{
	float		front, back, frac;
	int			side;
	mplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int			s, t, ds, dt;
	int			i;
	mtexinfo_t	*tex;
	byte		*lightmap;
	int			maps;
	int			r;

	if (node->contents != -1)
		return -1;		// didn't hit anything

	// calculate mid point

	// FIXME: optimize for axial
	plane = node->plane;
	front = DotProduct(start, plane->normal) - plane->dist;
	back = DotProduct(end, plane->normal) - plane->dist;
	side = front < 0;

	if ((back < 0) == side)
		return RecursiveLightPointColor(node->children[side], start, end);

	frac = front / (front - back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;

	// go down front side	
	r = RecursiveLightPointColor(node->children[side], start, mid);
	if (r >= 0)
		return r;		// hit something

	if ((back < 0) == side)
		return -1;		// didn't hit anuthing

	// check for impact on this node
	VectorCopy(mid, lightspot);
	lightplane = plane;

	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i = 0; i < node->numsurfaces; i++, surf++)
	{
		if (surf->flags&(SURF_DRAWTURB | SURF_DRAWSKY))
			continue;	// no lightmaps

		tex = surf->texinfo;

		s = DotProduct(mid, tex->vecs[0]) + tex->vecs[0][3];
		t = DotProduct(mid, tex->vecs[1]) + tex->vecs[1][3];;

		if (s < surf->texturemins[0] ||
			t < surf->texturemins[1])
			continue;

		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];

		if (ds > surf->extents[0] || dt > surf->extents[1])
			continue;

		if (!surf->samples)
			return 0;

		ds >>= 4;
		dt >>= 4;

		lightmap = surf->samples;
		VectorCopy(vec3_origin, pointcolor);
		if (lightmap)
		{
			vec3_t scale;

			lightmap += 3 * (dt * ((surf->extents[0] >> 4) + 1) + ds);

			for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255;
				maps++)
			{
				for (i = 0; i < 3; i++)
					scale[i] = 1 * r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

				pointcolor[0] += lightmap[0] * scale[0] * (1.0 / 255);
				pointcolor[1] += lightmap[1] * scale[1] * (1.0 / 255);
				pointcolor[2] += lightmap[2] * scale[2] * (1.0 / 255);
				lightmap += 3 * ((surf->extents[0] >> 4) + 1) *
					((surf->extents[1] >> 4) + 1);
			}
		}

		return 1;
	}

	// go down back side
	return RecursiveLightPointColor(node->children[!side], mid, end);
}

/*
===============
R_LightPointColor
===============
*/
void R_LightPointColor(vec3_t p, vec3_t color)
{
	vec3_t		end;
	float		r;
	int			lnum;
	dlight_t	*dl;
	float		light;
	vec3_t		dist;
	float		add;

	if (!r_worldmodel->lightdata)
	{
		color[0] = color[1] = color[2] = 1.0;
		return;
	}

	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048;

	r = RecursiveLightPointColor(r_worldmodel->nodes, p, end);

	if (r == -1)
	{
		VectorCopy(vec3_origin, color);
	}
	else
	{
		VectorCopy(pointcolor, color);
	}

	//
	// add dynamic lights
	//
	light = 0;
	dl = r_newrefdef.dlights;
	for (lnum = 0; lnum<r_newrefdef.num_dlights; lnum++, dl++)
	{
		VectorSubtract(currententity->origin,
			dl->origin,
			dist);
		add = dl->intensity - VectorLength(dist);
		add *= (1.0 / 256);
		if (add > 0)
		{
			VectorMA(color, add, dl->color, color);
		}
	}

	VectorScale(color, 1, color);
}



//===================================================================


unsigned		blocklights[1024 * 3];	// allow some very large lightmaps // leilei - *3 added

/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLights(void)
{
	msurface_t *surf;
	int			lnum;
	int			sd, td;
	float		dist, rad, minlight;
	vec3_t		impact, local;
	int			s, t;
	int			i;
	int			smax, tmax;
	mtexinfo_t	*tex;
	dlight_t	*dl;
	int			negativeLight;	//PGM

	surf = r_drawsurf.surf;
	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;
	tex = surf->texinfo;

	for (lnum = 0; lnum < r_newrefdef.num_dlights; lnum++)
	{
		if (!(surf->dlightbits & (1 << lnum)))
			continue;		// not lit by this light

		dl = &r_newrefdef.dlights[lnum];
		rad = dl->intensity;

		//=====
		//PGM
		negativeLight = 0;
		if (rad < 0)
		{
			negativeLight = 1;
			rad = -rad;
		}
		//PGM
		//=====

		dist = DotProduct(dl->origin, surf->plane->normal) -
			surf->plane->dist;
		rad -= fabs(dist);
		minlight = 32;		// dl->minlight;
		if (rad < minlight)
			continue;
		minlight = rad - minlight;

		for (i = 0; i < 3; i++)
		{
			impact[i] = dl->origin[i] -
				surf->plane->normal[i] * dist;
		}

		local[0] = DotProduct(impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct(impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];

		for (t = 0; t < tmax; t++)
		{
			td = local[1] - t * 16;
			if (td < 0)
				td = -td;
			for (s = 0; s < smax; s++)
			{
				sd = local[0] - s * 16;
				if (sd < 0)
					sd = -sd;
				if (sd > td)
					dist = sd + (td >> 1);
				else
					dist = td + (sd >> 1);
				//====
				//PGM
				if (!negativeLight)
				{
					if (dist < minlight)
						blocklights[t*smax + s] += (rad - dist) * 256;
				}
				else
				{
					if (dist < minlight)
						blocklights[t*smax + s] -= (rad - dist) * 256;
					if (blocklights[t*smax + s] < minlight)
						blocklights[t*smax + s] = minlight;
				}
				//PGM
				//====
			}
		}
	}
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the 8.8 format in blocklights
===============
*/
void R_BuildLightMap(void)
{
	int			smax, tmax;
	int			t;
	int			i, size;
	byte		*lightmap;
	unsigned	scale;
	int			maps;
	msurface_t	*surf;

	surf = r_drawsurf.surf;

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;
	size = smax*tmax;

	if (r_fullbright->value || !r_worldmodel->lightdata)
	{
		for (i = 0; i < size; i++)
			blocklights[i] = 0;
		return;
	}

	// clear to no light
	for (i = 0; i < size; i++)
		blocklights[i] = 0;


	// add all the lightmaps
	lightmap = surf->samples;
	if (lightmap)
	for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255;
		maps++)
	{
		scale = r_drawsurf.lightadj[maps];	// 8.8 fraction		
		for (i = 0; i < size; i++)
			blocklights[i] += lightmap[i] * scale;
		lightmap += size;	// skip to next lightmap
	}

	// add all the dynamic lights
	if (surf->dlightframe == r_framecount)
		R_AddDynamicLights();

	// bound, invert, and shift
	for (i = 0; i < size; i++)
	{
		t = (int)blocklights[i];
		if (t < 0)
			t = 0;
		t = (255 * 256 - t) >> (8 - VID_CBITS);

		if (t < (1 << 6))
			t = (1 << 6);

		blocklights[i] = t;
	}
}


// leilei - colored lights
// and spike too , fteqw version tweaked


/*
===============
R_AddDynamicLights
===============
*/
void R_AddDynamicLightsRGB(void)
{
	msurface_t *surf;
	int			lnum;
	int			sd, td;
	float		dist, rad, minlight;
	vec3_t		impact, local;
	int			s, t;
	int			i;
	int			smax, tmax;
	mtexinfo_t	*tex;
	dlight_t	*dl;
	int			negativeLight;	//PGM
	float		cred, cgreen, cblue, brightness;
	unsigned	*bl;
	surf = r_drawsurf.surf;
	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;
	tex = surf->texinfo;

	for (lnum = 0; lnum < r_newrefdef.num_dlights; lnum++)
	{
		if (!(surf->dlightbits & (1 << lnum)))
			continue;		// not lit by this light

		dl = &r_newrefdef.dlights[lnum];
		rad = dl->intensity;

		//=====
		//PGM
		negativeLight = 0;
		if (rad < 0)
		{
			negativeLight = 1;
			rad = -rad;
		}
		//PGM
		//=====

		dist = DotProduct(dl->origin, surf->plane->normal) -
			surf->plane->dist;
		rad -= fabs(dist);
		minlight = 32;		// dl->minlight;
		if (rad < minlight)
			continue;
		minlight = rad - minlight;

		for (i = 0; i < 3; i++)
		{
			impact[i] = dl->origin[i] -
				surf->plane->normal[i] * dist;
		}

		local[0] = DotProduct(impact, tex->vecs[0]) + tex->vecs[0][3];
		local[1] = DotProduct(impact, tex->vecs[1]) + tex->vecs[1][3];

		local[0] -= surf->texturemins[0];
		local[1] -= surf->texturemins[1];
		cred = dl[lnum].color[0] * 256.0f;
		cgreen = dl[lnum].color[1] * 256.0f;
		cblue = dl[lnum].color[2] * 256.0f;

		bl = blocklights;
		for (t = 0; t < tmax; t++)
		{
			td = local[1] - t * 16;
			if (td < 0)
				td = -td;
			for (s = 0; s < smax; s++)
			{
				sd = local[0] - s * 16;
				if (sd < 0)
					sd = -sd;
				if (sd > td)
					dist = sd + (td >> 1);
				else
					dist = td + (sd >> 1);
				//====
				//PGM

				if (!negativeLight)
				{
					if (dist < minlight)
					{
						brightness = rad - dist;
						bl[0] += (int)(brightness * cred);
						bl[1] += (int)(brightness * cgreen);
						bl[2] += (int)(brightness * cblue);
					}
					bl += 3;
				}
				else
				{
					if (dist < minlight)
					{
						brightness = rad - dist;
						bl[0] -= (int)(brightness * cred);
						bl[1] -= (int)(brightness * cgreen);
						bl[2] -= (int)(brightness * cblue);
						if (bl[0] < minlight) bl[0] = minlight;
						if (bl[1] < minlight) bl[1] = minlight;
						if (bl[2] < minlight) bl[2] = minlight;
					}
					bl += 3;
				}
				//PGM
				//====
			}
		}
	}
}

void R_BuildLightMapRGB(void)
{
	int			smax, tmax;
	int			i, size;
	byte		*lightmap;
	int			scale;
	int			maps;
	msurface_t	*surf;
	int r;
	surf = r_drawsurf.surf;

	smax = (surf->extents[0] >> 4) + 1;
	tmax = (surf->extents[1] >> 4) + 1;
	size = smax*tmax * 3;
	lightmap = surf->samples;

	if (r_fullbright->value || !r_worldmodel->lightdata)
	{
		for (i = 0; i < size; i++){
			blocklights[i] = 0;
			blocklights[i + 1] = 0;
			blocklights[i + 2] = 0;
		}
		return;
	}


	// clear to ambient
	for (i = 0; i < size; i++)
		blocklights[i] = r_refdef.ambientlight << 8;


	// add all the lightmaps
	if (lightmap)
	for (maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255;
		maps++)
	{
		scale = r_drawsurf.lightadj[maps];	// 8.8 fraction		
		for (i = 0; i < size; i += 3)
		{
			blocklights[i] += lightmap[i] * scale;
			blocklights[i + 1] += lightmap[i + 1] * scale;
			blocklights[i + 2] += lightmap[i + 2] * scale;
		}
		lightmap += size;	// skip to next lightmap
	}

	// add all the dynamic lights
	if (surf->dlightframe == r_framecount)
		R_AddDynamicLightsRGB();

	// bound, invert, and shift

	// how quake2 should have really looked 
	//qb: always do 'overbrights' on colored lights.
	for (i = 0; i < size; i++)
	{
		r = blocklights[i];
		blocklights[i] = (r < 256) ? 256 : (r > 65536) ? 65536 : r;	// leilei - made min 256 to rid visual artifacts and gain speed
	}
}

// o^_^o
