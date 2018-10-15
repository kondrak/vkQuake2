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

glconfig_t gl_config;
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

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_base_world_matrix[16];

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
cvar_t	*gl_polyblend;
cvar_t	*gl_flashblend;
cvar_t	*gl_playermip;
cvar_t  *gl_saturatelighting;
cvar_t	*gl_swapinterval;
cvar_t	*gl_texturemode;
cvar_t	*gl_texturealphamode;
cvar_t	*gl_texturesolidmode;
cvar_t	*gl_lockpvs;

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


void R_RotateForEntity (entity_t *e)
{
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

}

//==================================================================================

/*
=============
R_DrawNullModel
=============
*/
void R_DrawNullModel (void)
{

}

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{

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

}

//=======================================================================

int SignbitsForPlane (cplane_t *out)
{
    return 0;
}


void R_SetFrustum (void)
{

}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
}

/*
=============
R_SetupVulkan
=============
*/
void R_SetupVulkan (void)
{
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

}


void R_SetVulkan2D (void)
{

}


/*
====================
R_SetLightLevel

====================
*/
void R_SetLightLevel (void)
{

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

	vid_fullscreen = ri.Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
	vid_gamma = ri.Cvar_Get("vid_gamma", "1.0", CVAR_ARCHIVE);
	vid_ref = ri.Cvar_Get("vid_ref", "soft", CVAR_ARCHIVE);
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
