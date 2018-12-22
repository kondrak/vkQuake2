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
			data[y][x][1] = 0;
			data[y][x][2] = 0;
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
	byte		*buffer;
	char		picname[80];
	char		checkname[MAX_OSPATH];
	int			i, c, temp;
	FILE		*f;

	// create the scrnshots directory if it doesn't exist
	Com_sprintf(checkname, sizeof(checkname), "%s/scrnshot", ri.FS_Gamedir());
	Sys_Mkdir(checkname);

	// 
	// find a file name to save it to 
	// 
	strcpy(picname, "quake00.tga");

	for (i = 0; i <= 99; i++)
	{
		picname[5] = i / 10 + '0';
		picname[6] = i % 10 + '0';
		Com_sprintf(checkname, sizeof(checkname), "%s/scrnshot/%s", ri.FS_Gamedir(), picname);
		f = fopen(checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose(f);
	}
	if (i == 100)
	{
		ri.Con_Printf(PRINT_ALL, "SCR_ScreenShot_f: Couldn't create a file\n");
		return;
	}


	buffer = malloc(vid.width*vid.height * 3 + 18);
	memset(buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = vid.width & 255;
	buffer[13] = vid.width >> 8;
	buffer[14] = vid.height & 255;
	buffer[15] = vid.height >> 8;
	buffer[16] = 24;	// pixel size

	// TODO: read swapchain image in Vulkan
	//qglReadPixels(0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, buffer + 18);

	// swap rgb to bgr
	c = 18 + vid.width*vid.height * 3;
	for (i = 18; i < c; i += 3)
	{
		temp = buffer[i];
		buffer[i] = buffer[i + 2];
		buffer[i + 2] = temp;
	}

	f = fopen(checkname, "wb");
	fwrite(buffer, 1, c, f);
	fclose(f);

	free(buffer);
	ri.Con_Printf(PRINT_ALL, "Wrote %s\n", picname);
} 

/*
** Vk_Strings_f
*/
void Vk_Strings_f(void)
{
	int i = 0;
	char ver[] = { "vkQuake2 v"VKQUAKE2_VERSION };
	for (i = 0; i < strlen(ver); i++)
		ver[i] += 128;

	ri.Con_Printf(PRINT_ALL, "\n%s\n", ver);
	ri.Con_Printf(PRINT_ALL, "------------------------------------\n");
	ri.Con_Printf(PRINT_ALL, "Vulkan API: %d.%d\n",  VK_VERSION_MAJOR(vk_config.vk_version),
													 VK_VERSION_MINOR(vk_config.vk_version));
	ri.Con_Printf(PRINT_ALL, "Header version: %d\n", VK_HEADER_VERSION);
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
	i = 0;
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
