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

// draw.c

#include "vk_local.h"

image_t		*draw_chars;

extern	qboolean	scrap_dirty;
void Scrap_Upload (void);


/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal (void)
{
	// load console characters (don't bilerp characters)
	draw_chars = Vk_FindImage("pics/conchars.pcx", it_pic);
}



/*
================
Draw_Char

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Char (int x, int y, int num)
{
	int				row, col;
	float			frow, fcol, size;

	num &= 255;

	if ((num & 127) == 32)
		return;		// space

	if (y <= -8)
		return;			// totally off screen

	row = num >> 4;
	col = num & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	/*GL_Bind(draw_chars->texnum);

	qglBegin(GL_QUADS);
	qglTexCoord2f(fcol, frow);
	qglVertex2f(x, y);
	qglTexCoord2f(fcol + size, frow);
	qglVertex2f(x + 8, y);
	qglTexCoord2f(fcol + size, frow + size);
	qglVertex2f(x + 8, y + 8);
	qglTexCoord2f(fcol, frow + size);
	qglVertex2f(x, y + 8);
	qglEnd();*/
}

/*
=============
Draw_FindPic
=============
*/
image_t	*Draw_FindPic (char *name)
{
	image_t *vk;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf(fullname, sizeof(fullname), "pics/%s.pcx", name);
		vk = Vk_FindImage(fullname, it_pic);
	}
	else
		vk = Vk_FindImage(name + 1, it_pic);

	return vk;
}

/*
=============
Draw_GetPicSize
=============
*/
void Draw_GetPicSize (int *w, int *h, char *pic)
{

}

/*
=============
Draw_StretchPic
=============
*/

extern qvkbuffer_t vertexBuffer;
extern qvkbuffer_t indexBuffer;
extern qvkbuffer_t uniformBuffer;
extern VkDescriptorSet  descriptorSet;

void Draw_StretchPic (int x, int y, int w, int h, char *pic)
{
	image_t *vk;

	vk = Draw_FindPic(pic);
	if (!vk)
	{
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	// proof of concept, not final solution ;)
	vkDeviceWaitIdle(vk_device.logical);
	QVk_TempUpdateDescriptor(&vk->vk_texture);

	float imgTransform[] = { (float)x / vid.width, (float)y / vid.height,
							 (float)w / vid.width, (float)h / vid.height,
							  vk->sl,				vk->tl, 
							  vk->sh - vk->sl,		vk->th - vk->tl };
	void *data;
	vmaMapMemory(vk_malloc, uniformBuffer.allocation, &data);
	memcpy(data, &imgTransform, sizeof(imgTransform));
	vmaUnmapMemory(vk_malloc, uniformBuffer.allocation);

	vkCmdBindPipeline(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_console_pipeline.pl);

	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(vk_activeCmdbuffer, 0, 1, &vertexBuffer.buffer, offsets);
	vkCmdBindIndexBuffer(vk_activeCmdbuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(vk_activeCmdbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_console_pipeline.layout, 0, 1, &descriptorSet, 0, NULL);
	vkCmdDrawIndexed(vk_activeCmdbuffer, 6, 1, 0, 0, 0);

/*	if (scrap_dirty)
		Scrap_Upload();
*/
}


/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, char *pic)
{

}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h, char *pic)
{

}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{

}

//=============================================================================

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{

}


//====================================================================


/*
=============
Draw_StretchRaw
=============
*/
extern unsigned	r_rawpalette[256];

void Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data)
{

}
