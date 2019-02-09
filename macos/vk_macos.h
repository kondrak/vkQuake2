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
#ifndef __APPLE__
#error You shouldnt be including this file on non-MacOS platforms
#endif

#ifndef __VK_MACOS_H__
#define __VK_MACOS_H__

typedef struct
{
	FILE *log_fp;
} vkwstate_t;

extern vkwstate_t vkw_state;

#endif
