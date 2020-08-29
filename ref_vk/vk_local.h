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
#ifndef __VK_LOCAL_H__
#define __VK_LOCAL_H__

#ifdef _WIN32
#  include <windows.h>
#  define VK_USE_PLATFORM_WIN32_KHR
#endif

#if defined(__linux__) || defined(__FreeBSD__)
#  define VK_USE_PLATFORM_XLIB_KHR
#endif

#ifdef __APPLE__
#  define VK_USE_PLATFORM_MACOS_MVK
#endif

#include <stdio.h>

#include <vulkan/vulkan.h>
#include <math.h>

#include "../client/ref.h"

#include "qvk.h"

#define	REF_VERSION	"Vulkan (vkQuake2 v"VKQUAKE2_VERSION")"

// verify if VkResult is VK_SUCCESS
#ifdef _DEBUG
#define VK_VERIFY(x) { \
		VkResult res = (x); \
		if(res != VK_SUCCESS) { \
			ri.Con_Printf(PRINT_ALL, "VkResult verification failed: %s in %s:%d\n", QVk_GetError(res), __FILE__, __LINE__); \
			assert(res == VK_SUCCESS && "VkResult verification failed!"); \
		} \
}
#else
#	define VK_VERIFY(x) (void)(x)
#endif

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

typedef struct
{
	unsigned		width, height;			// coordinates from main game
} viddef_t;

extern	viddef_t	vid;


/*

  skins will be outline flood filled and mip mapped
  pics and sprites with alpha will be outline flood filled
  pic won't be mip mapped

  model skin
  sprite frame
  wall texture
  pic

*/

typedef enum 
{
	it_skin,
	it_sprite,
	it_wall,
	it_pic,
	it_sky
} imagetype_t;

typedef struct image_s
{
	char	name[MAX_QPATH];			// game path, including extension
	imagetype_t	type;
	int		width, height;				// source image
	int		upload_width, upload_height;	// after power of two and picmip
	int		registration_sequence;		// 0 = free
	struct msurface_s	*texturechain;	// for sort-by-texture world drawing
	qvktexture_t vk_texture;			// Vulkan texture handle
	float	sl, tl, sh, th;				// 0,0 - 1,1 unless part of the scrap
	qboolean	scrap;

} image_t;

#define		MAX_VKTEXTURES	1024

//===================================================================

typedef enum
{
	rserr_ok,

	rserr_invalid_fullscreen,
	rserr_invalid_mode,

	rserr_unknown
} rserr_t;

#include "vk_model.h"

#define	MAX_LBM_HEIGHT		480

#define BACKFACE_EPSILON	0.01


//====================================================

extern	image_t		vktextures[MAX_VKTEXTURES];
extern	int			numvktextures;

extern	image_t		*r_notexture;
extern	image_t		*r_particletexture;
extern	entity_t	*currententity;
extern	model_t		*currentmodel;
extern	int			r_visframecount;
extern	int			r_framecount;
extern	cplane_t	frustum[4];
extern	int			c_brush_polys, c_alias_polys;

//
// view origin
//
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_newrefdef;
extern	int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern	cvar_t	*r_norefresh;
extern	cvar_t	*r_lefthand;
extern	cvar_t	*r_drawentities;
extern	cvar_t	*r_drawworld;
extern	cvar_t	*r_speeds;
extern	cvar_t	*r_fullbright;
extern	cvar_t	*r_novis;
extern	cvar_t	*r_nocull;
extern	cvar_t	*r_lerpmodels;

extern	cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level

extern	cvar_t	*vk_validation;
extern	cvar_t	*vk_mode;
extern	cvar_t	*vk_bitdepth;
extern	cvar_t	*vk_log;
extern	cvar_t	*vk_picmip;
extern	cvar_t	*vk_skymip;
extern	cvar_t	*vk_round_down;
extern	cvar_t	*vk_flashblend;
extern	cvar_t	*vk_finish;
extern	cvar_t	*vk_clear;
extern	cvar_t	*vk_lockpvs;
extern	cvar_t	*vk_polyblend;
extern	cvar_t	*vk_modulate;
extern	cvar_t	*vk_shadows;
extern	cvar_t	*vk_particle_size;
extern	cvar_t	*vk_particle_att_a;
extern	cvar_t	*vk_particle_att_b;
extern	cvar_t	*vk_particle_att_c;
extern	cvar_t	*vk_particle_min_size;
extern	cvar_t	*vk_particle_max_size;
extern	cvar_t	*vk_point_particles;
extern	cvar_t	*vk_dynamic;
extern	cvar_t	*vk_showtris;
extern	cvar_t	*vk_lightmap;
extern	cvar_t	*vk_texturemode;
extern	cvar_t	*vk_lmaptexturemode;
extern	cvar_t	*vk_aniso;
extern	cvar_t	*vk_sampleshading;
extern	cvar_t	*vk_vsync;
extern	cvar_t	*vk_device_idx;
extern	cvar_t	*vk_fullscreen_exclusive;

extern	cvar_t	*vid_fullscreen;
extern	cvar_t	*vid_gamma;

extern	cvar_t	*intensity;

extern	int		c_visible_lightmaps;
extern	int		c_visible_textures;

extern	float	r_viewproj_matrix[16];

void R_LightPoint (vec3_t p, vec3_t color);
void R_PushDlights (void);

//====================================================================

extern	model_t	*r_worldmodel;

extern	unsigned	d_8to24table[256];

extern	int		registration_sequence;
extern	qvksampler_t vk_current_sampler;
extern	qvksampler_t vk_current_lmap_sampler;

qboolean R_Init( void *hinstance, void *hWnd );
void	 R_Shutdown( void );

void R_RenderView (refdef_t *fd);
void Vk_ScreenShot_f (void);
void R_DrawAliasModel (entity_t *e);
void R_DrawBrushModel (entity_t *e);
void R_DrawSpriteModel (entity_t *e);
void R_DrawBeam( entity_t *e );
void R_DrawWorld (void);
void R_RenderDlights (void);
void R_DrawAlphaSurfaces (void);
void R_RenderBrushPoly (msurface_t *fa, float *modelMatrix, float alpha);
void R_InitParticleTexture (void);
void Draw_InitLocal (void);
void Vk_SubdivideSurface (msurface_t *fa);
qboolean R_CullBox (vec3_t mins, vec3_t maxs);
void R_RotateForEntity (entity_t *e, float *mvMatrix);
void R_MarkLeaves (void);

void EmitWaterPolys (msurface_t *fa, image_t *texture, float *modelMatrix, float *color);
void R_AddSkySurface (msurface_t *fa);
void R_ClearSkyBox (void);
void R_DrawSkyBox (void);
void R_MarkLights (dlight_t *light, int bit, mnode_t *node);

void COM_StripExtension (char *in, char *out);

void	Draw_GetPicSize (int *w, int *h, char *name);
void	Draw_Pic (int x, int y, char *name);
void	Draw_StretchPic (int x, int y, int w, int h, char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_TileClear (int x, int y, int w, int h, char *name);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_FadeScreen (void);
void	Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data);

void	R_BeginFrame( float camera_separation );
void	R_EndFrame( void );
void	R_SetPalette ( const unsigned char *palette);

int		Draw_GetPalette (void);

void Vk_ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight);

struct image_s *R_RegisterSkin (char *name);

void LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height);
image_t *Vk_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type, int bits, qvksampler_t *samplerType);
image_t	*Vk_FindImage (char *name, imagetype_t type, qvksampler_t *samplerType);
void	Vk_TextureMode( char *string );
void	Vk_LmapTextureMode( char *string );
void	Vk_ImageList_f (void);

void	Vk_InitImages (void);
void	Vk_ShutdownImages (void);
void	Vk_FreeUnusedImages (void);
void	Vk_DrawParticles( int n, const particle_t particles[], const unsigned colortable[768] );

void Mat_Identity(float *matrix);
void Mat_Mul(float *m1, float *m2, float *res);
void Mat_Translate(float *matrix, float x, float y, float z);
void Mat_Rotate(float *matrix, float deg, float x, float y, float z);
void Mat_Scale(float *matrix, float x, float y, float z);
void Mat_Perspective(float *matrix, float *correction_matrix, float fovy, float aspect, float zNear, float zFar);
void Mat_Ortho(float *matrix, float left, float right, float bottom, float top, float zNear, float zFar);

typedef struct
{
	uint32_t    vk_version;
	const char *vendor_name;
	const char *device_type;
	const char *present_mode;
	const char *supported_present_modes[256];
	const char *extensions[256];
	const char *layers[256];
	uint32_t    vertex_buffer_usage;
	uint32_t    vertex_buffer_max_usage;
	uint32_t    vertex_buffer_size;
	uint32_t    index_buffer_usage;
	uint32_t    index_buffer_max_usage;
	uint32_t    index_buffer_size;
	uint32_t    uniform_buffer_usage;
	uint32_t    uniform_buffer_max_usage;
	uint32_t    uniform_buffer_size;
	uint32_t    triangle_fan_index_usage;
	uint32_t    triangle_fan_index_max_usage;
	uint32_t    triangle_fan_index_count;
	uint32_t    allocated_ubo_descriptor_set_count;
	uint32_t    allocated_sampler_descriptor_set_count;
	uint32_t    ubo_descriptor_set_count;
	uint32_t    sampler_descriptor_set_count;
	int         swapchain_image_count;
	qboolean    vk_ext_debug_utils_supported;
	qboolean    vk_ext_debug_report_supported;
	qboolean    vk_ext_full_screen_exclusive_available; // the extension is available
	qboolean    vk_ext_full_screen_exclusive_possible;  // extension dependencies are available
	qboolean    vk_full_screen_exclusive_enabled;
	qboolean    vk_full_screen_exclusive_acquired;
} vkconfig_t;

#define MAX_LIGHTMAPS 128
#define DYNLIGHTMAP_OFFSET MAX_LIGHTMAPS

typedef struct
{
	float inverse_intensity;
	qboolean fullscreen;

	int     prev_mode;

	unsigned char *d_16to8table;

	qvktexture_t lightmap_textures[MAX_LIGHTMAPS*2];

	int	currenttextures[2];
	int currenttmu;

	float camera_separation;
	qboolean stereo_enabled;

	VkPipeline current_pipeline;
#ifdef _WIN32
	VkSurfaceFullScreenExclusiveInfoEXT full_screen_exclusive_info;
#endif
} vkstate_t;

extern vkconfig_t  vk_config;
extern vkstate_t   vk_state;

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

extern	refimport_t	ri;


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

// Windows-only feature for now
#ifdef _WIN32
#define FULL_SCREEN_EXCLUSIVE_ENABLED
#endif

void		Vkimp_BeginFrame( float camera_separation );
void		Vkimp_EndFrame( void );
int 		Vkimp_Init( void *hinstance, void *hWnd );
void		Vkimp_Shutdown( void );
int			Vkimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen );
void		Vkimp_AppActivate( qboolean active );
void		Vkimp_EnableLogging( qboolean enable );
void		Vkimp_LogNewFrame( void );
void		Vkimp_GetInstanceExtensions(char **extensions, uint32_t *extCount);
VkResult	Vkimp_CreateSurface(void);
VkSurfaceCapabilitiesKHR	Vkimp_SetupFullScreenExclusive(void);

#endif
