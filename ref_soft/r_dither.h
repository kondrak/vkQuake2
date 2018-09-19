#ifndef __R_DITHER__
#define __R_DITHER__
/*
Copyright (C) 2012 Szilard Biro

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

/*

http://www.flipcode.com/archives/Texturing_As_In_Unreal.shtml

          (X&1)==0        (X&1==1)
         +---------------------------------
(Y&1)==0 | u+=.25,v+=.00  u+=.50,v+=.75
(Y&1)==1 | u+=.75,v+=.50  u+=.00,v+=.25

*/

#if 1

#define DitherKernel2(u, v, X, Y) \
	if ((Y)&1) { \
		if ((X)&1) { \
			(u) = (u) + (32768); \
			(v) = (v) + (49152); \
		} else { \
			(u) = (u) + (16384); \
		} \
	} else { \
		if ((X)&1) { \
			(v) = (v) + (32768); \
		} else { \
			(u) = (u) + (49152); \
			(v) = (v) + (16384); \
		} \
	}

#else

#define DitherKernel2(u, v, X, Y) \
	if ((Y)&1) { \
		if ((X)&1) { \
			(u) = (u) + (65536 * 0.50); /* 50 */ \
			(v) = (v) + (65536 * 0.75); /* 75 */ \
		} else { \
			(u) = (u) + (65536 * 0.25); /* 25 */ \
            (v) = (v) + (65536 * 0.0); /* 00 */ \
		} \
	} else { \
		if ((X)&1) { \
            (u) = (u) + (65536 * 0.0); /* 00 */ \
			(v) = (v) + (65536 * 0.5); /* 25 */ \
		} else { \
			(u) = (u) + (65536 * 0.75); /* 75 */ \
			(v) = (v) + (65536 * 0.25); /* 50 */ \
		} \
	}

#endif

#endif
