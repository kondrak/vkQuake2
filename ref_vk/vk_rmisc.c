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
// r_misc.c

#include "vk_local.h"

/*
==================
R_InitParticleTexture
==================
*/
byte	dottexture[8][8] =
{
	{0,0,0,0,0,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
};

void R_InitParticleTexture (void)
{
	int		x,y;
	byte	data[8][8][4];

	//
	// particle texture
	//
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y]*255;
		}
	}
	r_particletexture = Vk_LoadPic ("***particle***", (byte *)data, 8, 8, it_sprite, 32, NULL);

	//
	// also use this for bad textures, but without alpha
	//
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = dottexture[x&3][y&3]*255;
			data[y][x][1] = 0; // dottexture[x&3][y&3]*255;
			data[y][x][2] = 0; //dottexture[x&3][y&3]*255;
			data[y][x][3] = 255;
		}
	}
	r_notexture = Vk_LoadPic ("***r_notexture***", (byte *)data, 8, 8, it_wall, 32, NULL);
}


/* 
============================================================================== 
 
						SCREEN SHOTS 
 
============================================================================== 
*/ 

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/* 
================== 
Vk_ScreenShot_f
================== 
*/  
void Vk_ScreenShot_f (void) 
{

} 

/*
** Vk_Strings_f
*/
void Vk_Strings_f(void)
{
	ri.Con_Printf(PRINT_ALL, "------------------------------------\n");
	ri.Con_Printf(PRINT_ALL, "Vulkan API: %d.%d.%d\n",  VK_VERSION_MAJOR(vk_config.vk_version),
														VK_VERSION_MINOR(vk_config.vk_version),
														VK_VERSION_PATCH(vk_config.vk_version));
	ri.Con_Printf(PRINT_ALL, "Physical device:\n");
	ri.Con_Printf(PRINT_ALL, "   apiVersion: %d.%d.%d\n"
							 "   deviceID: %d\n"
							 "   deviceName: %s\n"
							 "   deviceType: %s\n"
							 "   gfx/present/transfer: %d/%d/%d\n", VK_VERSION_MAJOR(vk_config.api_version),
																	VK_VERSION_MINOR(vk_config.api_version),
																	VK_VERSION_PATCH(vk_config.api_version),
																	vk_config.device_id,
																	vk_config.device_name,
																	vk_config.device_type,
																	vk_config.gfx_family_idx, vk_config.present_family_idx, vk_config.transfer_family_idx);
	ri.Con_Printf(PRINT_ALL, "Enabled extensions: ");
	int i = 0;
	while(vk_config.extensions[i])
	{
		ri.Con_Printf(PRINT_ALL, "%s ", vk_config.extensions[i++]);
	}
	ri.Con_Printf(PRINT_ALL, "\nEnabled layers: ");

	i = 0;
	while(vk_config.layers[i])
	{
		ri.Con_Printf(PRINT_ALL, "%s ", vk_config.layers[i++]);
	}
	ri.Con_Printf(PRINT_ALL, "\n");
}

/*
** Vk_SetDefaultState
*/
void Vk_SetDefaultState( void )
{

}
