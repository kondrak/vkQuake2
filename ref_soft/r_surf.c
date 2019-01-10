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
// r_surf.c: surface-related refresh code

#include "r_local.h"

drawsurf_t	r_drawsurf;

int				lightleft, sourcesstep, blocksize, sourcetstep;
int				lightdelta, lightdeltastep;
int				lightright, lightleftstep, lightrightstep, blockdivshift;
unsigned		blockdivmask;
void			*prowdestbase;
unsigned char	*pbasesource;
int				surfrowbytes;	// used by ASM files
//unsigned		*r_lightptr;
int				*r_lightptr; // leilei - colored lighting
int				r_stepback;
int				r_lightwidth;
int				r_numhblocks, r_numvblocks;
unsigned char	*r_source, *r_sourcemax;

void R_DrawSurfaceBlock8_mip0 (void);
void R_DrawSurfaceBlock8_mip1 (void);
void R_DrawSurfaceBlock8_mip2 (void);
void R_DrawSurfaceBlock8_mip3 (void);

static void	(*surfmiptable[4])(void) = {
	R_DrawSurfaceBlock8_mip0,
	R_DrawSurfaceBlock8_mip1,
	R_DrawSurfaceBlock8_mip2,
	R_DrawSurfaceBlock8_mip3
};



// leilei - Colored Lights - Begin

// leilei - Colored Lights


int		lightlefta[3], sourcesstep, blocksize, sourcetstep;
int		lightdelta, lightdeltastep;
int		lightrighta[3], lightleftstepa[3], lightrightstepa[3], blockdivshift;

// Macros

// Macros for initiating the RGB light deltas.
#define MakeLightDelta() { light[0] =  lightrighta[0];	light[1] =  lightrighta[1];	light[2] =  lightrighta[2];};
#define PushLightDelta() { light[0] += lightdelta[0];	light[1] += lightdelta[1];	light[2] += lightdelta[2]; };
#define FinishLightDelta() { psource += sourcetstep; lightrighta[0] += lightrightstepa[0];lightlefta[0] += lightleftstepa[0];lightdelta[0] += lightdeltastep[0]; lightrighta[1] += lightrightstepa[1];lightlefta[1] += lightleftstepa[1];lightdelta[1] += lightdeltastep[1]; lightrighta[2] += lightrightstepa[2];lightlefta[2] += lightleftstepa[2];lightdelta[2] += lightdeltastep[2]; prowdest += surfrowbytes;}

// High Colored Light Quality //qb: preserve alphatest
#define MIP8RGBX(i) {  	pix = psource[i]; if(pix == 255) prowdest[i] = 255; else{pix24 = (unsigned char *)&d_8to24table[pix];   \
	trans[0] = (pix24[0] * (light[0])) >> 17; trans[1] = (pix24[1] * (light[1])) >> 17; trans[2] = (pix24[2] * (light[2])) >> 17; \
if (trans[0] & ~63) trans[0] = 63; if (trans[1] & ~63) trans[1] = 63; if (trans[2] & ~63) trans[2] = 63; prowdest[i] = palmap2[trans[0]][trans[1]][trans[2]]; }}

#define Mip0Stuff(i) { MakeLightDelta(); i(15); PushLightDelta(); i(14); PushLightDelta(); PushLightDelta(); i(13); PushLightDelta(); i(12); PushLightDelta(); i(11); PushLightDelta(); i(10); PushLightDelta(); i(9); PushLightDelta(); i(8); PushLightDelta(); i(7); PushLightDelta(); i(6); PushLightDelta(); i(5); PushLightDelta(); i(4); PushLightDelta(); i(3); PushLightDelta(); i(2); PushLightDelta(); i(1); PushLightDelta(); i(0);  FinishLightDelta();}
#define Mip1Stuff(i) { MakeLightDelta(); i(7); PushLightDelta(); i(6); PushLightDelta(); i(5); PushLightDelta(); i(4); PushLightDelta(); i(3); PushLightDelta(); i(2); PushLightDelta(); i(1); PushLightDelta(); i(0); FinishLightDelta();}
#define Mip2Stuff(i) { MakeLightDelta();i(3); PushLightDelta(); i(2); PushLightDelta(); i(1); PushLightDelta(); i(0); FinishLightDelta();}
#define Mip3Stuff(i) { MakeLightDelta(); i(1); PushLightDelta(); i(0); FinishLightDelta();}

// o^_^o

static void R_DrawSurfaceBlock8RGBX_mip0(void);
static void R_DrawSurfaceBlock8RGBX_mip1(void);
static void R_DrawSurfaceBlock8RGBX_mip2(void);
static void R_DrawSurfaceBlock8RGBX_mip3(void);


static void	(*surfmiptable8RGB[4])(void) =
{
	R_DrawSurfaceBlock8RGBX_mip0,
	R_DrawSurfaceBlock8RGBX_mip1,
	R_DrawSurfaceBlock8RGBX_mip2,
	R_DrawSurfaceBlock8RGBX_mip3
};

//extern	int			host_fullbrights;   // for preserving fullbrights in color operations
//extern byte		*lmmap;
extern byte	palmap2[64][64][64];		//  Colored Lighting Lookup Table



// leilei - Colored Lights - End

void R_BuildLightMap (void);
//extern	unsigned		blocklights[1024];	// allow some very large lightmaps
//extern	unsigned		blocklights[1024*3];	// leilei - colored lights
extern	unsigned		blocklights[18*18*3];	// leilei - colored lights

float           surfscale;
qboolean        r_cache_thrash;         // set if surface cache is thrashing

int         sc_size;
surfcache_t	*sc_rover, *sc_base;

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
===============
R_DrawSurface
===============
*/
void R_DrawSurface (void)
{
	unsigned char	*basetptr;
	int				smax, tmax, twidth;
	int				u;
	int				soffset, basetoffset, texwidth;
	int				horzblockstep;
	unsigned char	*pcolumndest;
	void			(*pblockdrawer)(void);
	image_t			*mt;

	surfrowbytes = r_drawsurf.rowbytes;

	mt = r_drawsurf.image;
	
	r_source = mt->pixels[r_drawsurf.surfmip];
	
// the fractional light values should range from 0 to (VID_GRADES - 1) << 16
// from a source range of 0 - 255
	
	texwidth = mt->width >> r_drawsurf.surfmip;

	blocksize = 16 >> r_drawsurf.surfmip;
	blockdivshift = 4 - r_drawsurf.surfmip;
	blockdivmask = (1 << blockdivshift) - 1;
	
	r_lightwidth = (r_drawsurf.surf->extents[0]>>4)+1;

	r_numhblocks = r_drawsurf.surfwidth >> blockdivshift;
	r_numvblocks = r_drawsurf.surfheight >> blockdivshift;

//==============================
	if (coloredlights)
		pblockdrawer = surfmiptable8RGB[r_drawsurf.surfmip];	// leilei - colored lights
	else
		pblockdrawer = surfmiptable[r_drawsurf.surfmip];

// TODO: only needs to be set when there is a display settings change
	horzblockstep = blocksize;

	smax = mt->width >> r_drawsurf.surfmip;
	twidth = texwidth;
	tmax = mt->height >> r_drawsurf.surfmip;
	sourcetstep = texwidth;
	r_stepback = tmax * twidth;

	r_sourcemax = r_source + (tmax * smax);

	soffset = r_drawsurf.surf->texturemins[0];
	basetoffset = r_drawsurf.surf->texturemins[1];

// << 16 components are to guarantee positive values for %
	soffset = ((soffset >> r_drawsurf.surfmip) + (smax << 16)) % smax;
	basetptr = &r_source[((((basetoffset >> r_drawsurf.surfmip) 
		+ (tmax << 16)) % tmax) * twidth)];

	pcolumndest = r_drawsurf.surfdat;

	for (u=0 ; u<r_numhblocks; u++)
	{
			// leilei - colored lights
		if (coloredlights)
			r_lightptr = (int*)blocklights + u * 3;
		else
			// o^_^o
			r_lightptr = (int*)blocklights + u;

		prowdestbase = pcolumndest;

		pbasesource = basetptr + soffset;

		(*pblockdrawer)();

		soffset = soffset + blocksize;
		if (soffset >= smax)
			soffset = 0;

		pcolumndest += horzblockstep;
	}
}


//=============================================================================

#if	!id386

/*
================
R_DrawSurfaceBlock8_mip0
================
*/
void R_DrawSurfaceBlock8_mip0 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 4;
		lightrightstep = (r_lightptr[1] - lightright) >> 4;

		for (i=0 ; i<16 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 4;

			light = lightright;

			for (b=15; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & 0xFF00) + pix];
				light += lightstep;
			}
	
			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_mip1
================
*/
void R_DrawSurfaceBlock8_mip1 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 3;
		lightrightstep = (r_lightptr[1] - lightright) >> 3;

		for (i=0 ; i<8 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 3;

			light = lightright;

			for (b=7; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & 0xFF00) + pix];
				light += lightstep;
			}
	
			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_mip2
================
*/
void R_DrawSurfaceBlock8_mip2 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 2;
		lightrightstep = (r_lightptr[1] - lightright) >> 2;

		for (i=0 ; i<4 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 2;

			light = lightright;

			for (b=3; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & 0xFF00) + pix];
				light += lightstep;
			}
	
			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}


/*
================
R_DrawSurfaceBlock8_mip3
================
*/
void R_DrawSurfaceBlock8_mip3 (void)
{
	int				v, i, b, lightstep, lighttemp, light;
	unsigned char	pix, *psource, *prowdest;

	psource = pbasesource;
	prowdest = prowdestbase;

	for (v=0 ; v<r_numvblocks ; v++)
	{
	// FIXME: make these locals?
	// FIXME: use delta rather than both right and left, like ASM?
		lightleft = r_lightptr[0];
		lightright = r_lightptr[1];
		r_lightptr += r_lightwidth;
		lightleftstep = (r_lightptr[0] - lightleft) >> 1;
		lightrightstep = (r_lightptr[1] - lightright) >> 1;

		for (i=0 ; i<2 ; i++)
		{
			lighttemp = lightleft - lightright;
			lightstep = lighttemp >> 1;

			light = lightright;

			for (b=1; b>=0; b--)
			{
				pix = psource[b];
				prowdest[b] = ((unsigned char *)vid.colormap)
						[(light & 0xFF00) + pix];
				light += lightstep;
			}
	
			psource += sourcetstep;
			lightright += lightrightstep;
			lightleft += lightleftstep;
			prowdest += surfrowbytes;
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
	}
}

#endif


//============================================================================


/*
================
R_InitCaches

================
*/
void R_InitCaches (void)
{
	int		size;
	int		pix;

	// calculate size to allocate
	if (sw_surfcacheoverride->value)
	{
		size = sw_surfcacheoverride->value;
	}
	else
	{
		size = SURFCACHE_SIZE_AT_320X240;

		pix = vid.width*vid.height;
		if (pix > 64000)
			size += (pix-64000)*3;
	}		

	// round up to page size
	size = (size + 8191) & ~8191;

	ri.Con_Printf (PRINT_ALL,"%ik surface cache\n", size/1024);

	sc_size = size;
	sc_base = (surfcache_t *)malloc(size);
	sc_rover = sc_base;
	
	sc_base->next = NULL;
	sc_base->owner = NULL;
	sc_base->size = sc_size;
}


/*
==================
D_FlushCaches
==================
*/
void D_FlushCaches (void)
{
	surfcache_t     *c;
	
	if (!sc_base)
		return;

	for (c = sc_base ; c ; c = c->next)
	{
		if (c->owner)
			*c->owner = NULL;
	}
	
	sc_rover = sc_base;
	sc_base->next = NULL;
	sc_base->owner = NULL;
	sc_base->size = sc_size;
}

/*
=================
D_SCAlloc
=================
*/
surfcache_t     *D_SCAlloc (int width, int size)
{
	surfcache_t             *new;
	qboolean                wrapped_this_time;

	if ((width < 0) || (width > 256))
		ri.Sys_Error (ERR_FATAL,"D_SCAlloc: bad cache width %d\n", width);

	if ((size <= 0) || (size > 0x10000))
		ri.Sys_Error (ERR_FATAL,"D_SCAlloc: bad cache size %d\n", size);
	
	size = (intptr_t)&((surfcache_t *)0)->data[size];
	size = (size + 3) & ~3;
	if (size > sc_size)
		ri.Sys_Error (ERR_FATAL,"D_SCAlloc: %i > cache size of %i",size, sc_size);

// if there is not size bytes after the rover, reset to the start
	wrapped_this_time = false;

	if ( !sc_rover || (byte *)sc_rover - (byte *)sc_base > sc_size - size)
	{
		if (sc_rover)
		{
			wrapped_this_time = true;
		}
		sc_rover = sc_base;
	}
		
// colect and free surfcache_t blocks until the rover block is large enough
	new = sc_rover;
	if (sc_rover->owner)
		*sc_rover->owner = NULL;
	
	while (new->size < size)
	{
	// free another
		sc_rover = sc_rover->next;
		if (!sc_rover)
			ri.Sys_Error (ERR_FATAL,"D_SCAlloc: hit the end of memory");
		if (sc_rover->owner)
			*sc_rover->owner = NULL;
			
		new->size += sc_rover->size;
		new->next = sc_rover->next;
	}

// create a fragment out of any leftovers
	if (new->size - size > 256)
	{
		sc_rover = (surfcache_t *)( (byte *)new + size);
		sc_rover->size = new->size - size;
		sc_rover->next = new->next;
		sc_rover->width = 0;
		sc_rover->owner = NULL;
		new->next = sc_rover;
		new->size = size;
	}
	else
		sc_rover = new->next;
	
	new->width = width;
// DEBUG
	if (width > 0)
		new->height = (size - sizeof(*new) + sizeof(new->data)) / width;

	new->owner = NULL;              // should be set properly after return

	if (d_roverwrapped)
	{
		if (wrapped_this_time || (sc_rover >= d_initial_rover))
			r_cache_thrash = true;
	}
	else if (wrapped_this_time)
	{       
		d_roverwrapped = true;
	}

	return new;
}


/*
=================
D_SCDump
=================
*/
void D_SCDump (void)
{
	surfcache_t             *test;

	for (test = sc_base ; test ; test = test->next)
	{
		if (test == sc_rover)
			ri.Con_Printf (PRINT_ALL,"ROVER:\n");
		ri.Con_Printf (PRINT_ALL,"%p : %i bytes     %i width\n",test, test->size, test->width);
	}
}

//=============================================================================

// if the num is not a power of 2, assume it will not repeat

int     MaskForNum (int num)
{
	if (num==128)
		return 127;
	if (num==64)
		return 63;
	if (num==32)
		return 31;
	if (num==16)
		return 15;
	return 255;
}

int D_log2 (int num)
{
	int     c;
	
	c = 0;
	
	while (num>>=1)
		c++;
	return c;
}

//=============================================================================
void R_BuildLightMapRGB (void);
/*
================
D_CacheSurface
================
*/
surfcache_t *D_CacheSurface (msurface_t *surface, int miplevel)
{
	surfcache_t     *cache;

//
// if the surface is animating or flashing, flush the cache
//
	r_drawsurf.image = R_TextureAnimation (surface->texinfo);
	r_drawsurf.lightadj[0] = r_newrefdef.lightstyles[surface->styles[0]].white*128;
	r_drawsurf.lightadj[1] = r_newrefdef.lightstyles[surface->styles[1]].white*128;
	r_drawsurf.lightadj[2] = r_newrefdef.lightstyles[surface->styles[2]].white*128;
	r_drawsurf.lightadj[3] = r_newrefdef.lightstyles[surface->styles[3]].white*128;
	
//
// see if the cache holds apropriate data
//
	cache = surface->cachespots[miplevel];

	if (cache && !cache->dlight && surface->dlightframe != r_framecount
			&& cache->image == r_drawsurf.image
			&& cache->lightadj[0] == r_drawsurf.lightadj[0]
			&& cache->lightadj[1] == r_drawsurf.lightadj[1]
			&& cache->lightadj[2] == r_drawsurf.lightadj[2]
			&& cache->lightadj[3] == r_drawsurf.lightadj[3] )
		return cache;

//
// determine shape of surface
//
	surfscale = 1.0 / (1<<miplevel);
	r_drawsurf.surfmip = miplevel;
	r_drawsurf.surfwidth = surface->extents[0] >> miplevel;
	r_drawsurf.rowbytes = r_drawsurf.surfwidth;
	r_drawsurf.surfheight = surface->extents[1] >> miplevel;
	
//
// allocate memory if needed
//
	if (!cache)     // if a texture just animated, don't reallocate it
	{
		cache = D_SCAlloc (r_drawsurf.surfwidth,
						   r_drawsurf.surfwidth * r_drawsurf.surfheight);
		surface->cachespots[miplevel] = cache;
		cache->owner = &surface->cachespots[miplevel];
		cache->mipscale = surfscale;
	}
	
	if (surface->dlightframe == r_framecount)
		cache->dlight = 1;
	else
		cache->dlight = 0;

	r_drawsurf.surfdat = (pixel_t *)cache->data;
	
	cache->image = r_drawsurf.image;
	cache->lightadj[0] = r_drawsurf.lightadj[0];
	cache->lightadj[1] = r_drawsurf.lightadj[1];
	cache->lightadj[2] = r_drawsurf.lightadj[2];
	cache->lightadj[3] = r_drawsurf.lightadj[3];

//
// draw and light the surface texture
//
	r_drawsurf.surf = surface;

	c_surf++;

	// calculate the lightings
	if (coloredlights)
		R_BuildLightMapRGB();	// leilei - colored lights
	else
		R_BuildLightMap();
	
	// rasterize the surface into the cache
	R_DrawSurface ();

	return cache;
}




// leilei
// here it comes


void R_DrawSurfaceBlock8RGBX_mip0()
{
	unsigned int				v, i; 
	unsigned int light[3];
	unsigned int lightdelta[3], lightdeltastep[3];
	unsigned char	pix, *psource, *prowdest;
	unsigned char *pix24;
	unsigned trans[3];
	psource = pbasesource;
	prowdest = prowdestbase;

#pragma loop(hint_parallel(8)) //qb: try this
	for (v=0 ; v<r_numvblocks ; v++)
	{
		lightlefta[0] = r_lightptr[0];
		lightrighta[0] = r_lightptr[3];
		lightlefta[1] = r_lightptr[0+1];
		lightrighta[1] = r_lightptr[3+1];
		lightlefta[2] = r_lightptr[0+2];
		lightrighta[2] = r_lightptr[3+2];

		lightdelta[0] = (lightlefta[0] - lightrighta[0])  >> 4; 
		lightdelta[1] = (lightlefta[1] - lightrighta[1])  >> 4;  
		lightdelta[2] = (lightlefta[2] - lightrighta[2])  >> 4; 


		r_lightptr += r_lightwidth * 3;

		lightleftstepa[0] = (r_lightptr[0] - lightlefta[0]) >> 4;
		lightrightstepa[0] = (r_lightptr[3] - lightrighta[0]) >> 4;

		lightleftstepa[1] = (r_lightptr[0+1] - lightlefta[1]) >> 4;
		lightrightstepa[1] = (r_lightptr[3+1] - lightrighta[1]) >> 4;

		lightleftstepa[2] = (r_lightptr[0+2] - lightlefta[2]) >> 4;
		lightrightstepa[2] = (r_lightptr[3+2] - lightrighta[2]) >> 4;

		lightdeltastep[0] = (lightleftstepa[0] - lightrightstepa[0]) >> 4;
		lightdeltastep[1] = (lightleftstepa[1] - lightrightstepa[1]) >> 4;
		lightdeltastep[2] = (lightleftstepa[2] - lightrightstepa[2]) >> 4;


		for (i=0 ; i<16 ; i++)
		{
			Mip0Stuff(MIP8RGBX);
		
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
		
	}
}




void R_DrawSurfaceBlock8RGBX_mip1()
{
	unsigned int				v, i; 
	unsigned int light[3];
	unsigned int lightdelta[3], lightdeltastep[3];
	unsigned char	pix, *psource, *prowdest;
	unsigned char *pix24;
	unsigned trans[3];
	psource = pbasesource;
	
	prowdest = prowdestbase;

#pragma loop(hint_parallel(8)) //qb: try this
	for (v=0 ; v<r_numvblocks ; v++)
	{
		lightlefta[0] = r_lightptr[0];
		lightrighta[0] = r_lightptr[3];
		lightlefta[1] = r_lightptr[0+1];
		lightrighta[1] = r_lightptr[3+1];
		lightlefta[2] = r_lightptr[0+2];
		lightrighta[2] = r_lightptr[3+2];

		lightdelta[0] = (lightlefta[0] - lightrighta[0])  >> 3; 
		lightdelta[1] = (lightlefta[1] - lightrighta[1])  >> 3;  
		lightdelta[2] = (lightlefta[2] - lightrighta[2])  >> 3; 


		r_lightptr += r_lightwidth * 3;

		lightleftstepa[0] = (r_lightptr[0] - lightlefta[0]) >> 3;
		lightrightstepa[0] = (r_lightptr[3] - lightrighta[0]) >> 3;

		lightleftstepa[1] = (r_lightptr[0+1] - lightlefta[1]) >> 3;
		lightrightstepa[1] = (r_lightptr[3+1] - lightrighta[1]) >> 3;

		lightleftstepa[2] = (r_lightptr[0+2] - lightlefta[2]) >> 3;
		lightrightstepa[2] = (r_lightptr[3+2] - lightrighta[2]) >> 3;

		lightdeltastep[0] = (lightleftstepa[0] - lightrightstepa[0]) >> 3;
		lightdeltastep[1] = (lightleftstepa[1] - lightrightstepa[1]) >> 3;
		lightdeltastep[2] = (lightleftstepa[2] - lightrightstepa[2]) >> 3;

		for (i=0 ; i<8 ; i++)
		{
			Mip1Stuff(MIP8RGBX);


		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
		
	}
}



void R_DrawSurfaceBlock8RGBX_mip2()
{
	unsigned int				v, i; 
	unsigned int light[3];
	unsigned int lightdelta[3], lightdeltastep[3];
	unsigned char	pix, *psource, *prowdest;
	unsigned char *pix24;
	unsigned trans[3];
	psource = pbasesource;
	
	prowdest = prowdestbase;

#pragma loop(hint_parallel(8)) //qb: try this
	for (v=0 ; v<r_numvblocks ; v++)
	{
		lightlefta[0] = r_lightptr[0];
		lightrighta[0] = r_lightptr[3];
		lightlefta[1] = r_lightptr[0+1];
		lightrighta[1] = r_lightptr[3+1];
		lightlefta[2] = r_lightptr[0+2];
		lightrighta[2] = r_lightptr[3+2];

		lightdelta[0] = (lightlefta[0] - lightrighta[0])  >> 2; 
		lightdelta[1] = (lightlefta[1] - lightrighta[1])  >> 2;  
		lightdelta[2] = (lightlefta[2] - lightrighta[2])  >> 2; 


		r_lightptr += r_lightwidth * 3;

		lightleftstepa[0] = (r_lightptr[0] - lightlefta[0]) >> 2;
		lightrightstepa[0] = (r_lightptr[3] - lightrighta[0]) >> 2;

		lightleftstepa[1] = (r_lightptr[0+1] - lightlefta[1]) >> 2;
		lightrightstepa[1] = (r_lightptr[3+1] - lightrighta[1]) >> 2;

		lightleftstepa[2] = (r_lightptr[0+2] - lightlefta[2]) >> 2;
		lightrightstepa[2] = (r_lightptr[3+2] - lightrighta[2]) >> 2;

		lightdeltastep[0] = (lightleftstepa[0] - lightrightstepa[0]) >> 2;
		lightdeltastep[1] = (lightleftstepa[1] - lightrightstepa[1]) >> 2;
		lightdeltastep[2] = (lightleftstepa[2] - lightrightstepa[2]) >> 2;

		for (i=0 ; i<4 ; i++)
		{
			Mip2Stuff(MIP8RGBX);


		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
		
	}
}


void R_DrawSurfaceBlock8RGBX_mip3()
{
	unsigned int				v, i; 
	unsigned int  light[3];
	unsigned int lightdelta[3], lightdeltastep[3];
	unsigned char	pix, *psource, *prowdest;
	unsigned char *pix24;
	unsigned trans[3];
	psource = pbasesource;
	
	prowdest = prowdestbase;

#pragma loop(hint_parallel(8)) //qb: try this
	for (v=0 ; v<r_numvblocks ; v++)
	{
		lightlefta[0] = r_lightptr[0];
		lightrighta[0] = r_lightptr[3];
		lightlefta[1] = r_lightptr[0+1];
		lightrighta[1] = r_lightptr[3+1];
		lightlefta[2] = r_lightptr[0+2];
		lightrighta[2] = r_lightptr[3+2];

		lightdelta[0] = (lightlefta[0] - lightrighta[0])  >> 1; 
		lightdelta[1] = (lightlefta[1] - lightrighta[1])  >> 1;  
		lightdelta[2] = (lightlefta[2] - lightrighta[2])  >> 1; 


		r_lightptr += r_lightwidth * 3;

		lightleftstepa[0] = (r_lightptr[0] - lightlefta[0]) >> 1;
		lightrightstepa[0] = (r_lightptr[3] - lightrighta[0]) >> 1;

		lightleftstepa[1] = (r_lightptr[0+1] - lightlefta[1]) >> 1;
		lightrightstepa[1] = (r_lightptr[3+1] - lightrighta[1]) >> 1;

		lightleftstepa[2] = (r_lightptr[0+2] - lightlefta[2]) >> 1;
		lightrightstepa[2] = (r_lightptr[3+2] - lightrighta[2]) >> 1;

		lightdeltastep[0] = (lightleftstepa[0] - lightrightstepa[0]) >> 1;
		lightdeltastep[1] = (lightleftstepa[1] - lightrightstepa[1]) >> 1;
		lightdeltastep[2] = (lightleftstepa[2] - lightrightstepa[2]) >> 1;

		for (i=0 ; i<2 ; i++)
		{
			Mip3Stuff(MIP8RGBX);

	
		}

		if (psource >= r_sourcemax)
			psource -= r_stepback;
		
	}
}






