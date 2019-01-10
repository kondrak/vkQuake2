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
// d_scan.c
//
// Portable C scan-level rasterization code, all pixel depths.

#include "r_local.h"
#include "r_dither.h"

//qb:  static vars for several functions
static int          count, spancount;
static byte         *pbase, *pdest;
static fixed16_t    s, t, snext, tnext, sstep, tstep;
static float        sdivz, tdivz, zi, z, du, dv, spancountminus1;
static float        sdivzstepu, tdivzstepu, zistepu;
static int		    izi, izistep; // mankrip
static short		*pz; // mankrip

static byte	*r_turb_pbase, *r_turb_pdest;
static fixed16_t		r_turb_s, r_turb_t, r_turb_sstep, r_turb_tstep;
static int				r_turb_spancount;

int				*r_turb_turb;

void D_DrawTurbulent8Span(espan_t *pspan);

/*
=============
D_WarpScreen

this performs a slight compression of the screen at the same time as
the sine warp, to keep the edges from wrapping
=============
*/
void D_WarpScreen(void)
{
	static int		w, h;
	static int		u, v, u2, v2;
	static byte		*dest;
	static int		*turb;
	static int		*col;
	static byte		**row;

	static int	cached_width, cached_height;
	static byte	*rowptr[4200 + AMP2 * 2]; //qb: 4K... was 1200
	static int	column[2400 + AMP2 * 2]; //qb: 4K... was 1600

	//
	// these are constant over resolutions, and can be saved
	//
	w = r_newrefdef.width;
	h = r_newrefdef.height;
	if (w != cached_width || h != cached_height)
	{
		cached_width = w;
		cached_height = h;
		for (v = 0; v < h + AMP2 * 2; v++)
		{
			v2 = (int)((float)v / (h + AMP2 * 2) * r_refdef.vrect.height);
			rowptr[v] = r_warpbuffer + (r_warpwidth * v2);
		}

		for (u = 0; u < w + AMP2 * 2; u++)
		{
			u2 = (int)((float)u / (w + AMP2 * 2) * r_refdef.vrect.width);
			column[u] = u2;
		}
	}

	turb = intsintable + ((int)(r_newrefdef.time*SPEED)&(CYCLE - 1));
	dest = vid.buffer + r_newrefdef.y * vid.rowbytes + r_newrefdef.x;

	for (v = 0; v < h; v++, dest += vid.rowbytes)
	{
		col = &column[turb[v]];
		row = &rowptr[v];
		for (u = 0; u < w; u += 4)
		{
			dest[u + 0] = row[turb[u + 0]][col[u + 0]];
			dest[u + 1] = row[turb[u + 1]][col[u + 1]];
			dest[u + 2] = row[turb[u + 2]][col[u + 2]];
			dest[u + 3] = row[turb[u + 3]][col[u + 3]];
		}
	}
}


#if	!id386

/*
=============
D_DrawTurbulent8Span
=============
*/
void D_DrawTurbulent8Span(espan_t *pspan)
{
	static int		sturb, tturb;

	//  float ditht, diths;

	do
	{
		if (sw_transmooth->value /*> 1*/)
		{
			sturb = r_turb_s + r_turb_turb[(r_turb_t >> 16)&(CYCLE - 1)];
			tturb = r_turb_t + r_turb_turb[(r_turb_s >> 16)&(CYCLE - 1)];

			DitherKernel2(sturb, tturb, pspan->u + r_turb_spancount, pspan->v);

			tturb = (tturb >> 16) & 63;
			sturb = (sturb >> 16) & 63;
		}
		else
		{
			sturb = ((r_turb_s + r_turb_turb[(r_turb_t >> 16)&(CYCLE - 1)]) >> 16) & 63;
			tturb = ((r_turb_t + r_turb_turb[(r_turb_s >> 16)&(CYCLE - 1)]) >> 16) & 63;
		}

		*r_turb_pdest++ = *(r_turb_pbase + (tturb << 6) + sturb);
		r_turb_s += r_turb_sstep;
		r_turb_t += r_turb_tstep;
	} while (--r_turb_spancount > 0);
}

#endif	// !id386


/*
=============
Turbulent8
=============
*/
void Turbulent8(espan_t *pspan)
{

	r_turb_turb = sintable + ((int)(r_newrefdef.time*SPEED)&(CYCLE - 1));

	r_turb_sstep = 0;	// keep compiler happy
	r_turb_tstep = 0;	// ditto

	r_turb_pbase = (unsigned char *)cacheblock;

	sdivzstepu = d_sdivzstepu * 16;
	tdivzstepu = d_tdivzstepu * 16;
	zistepu = d_zistepu * 16;

	do
	{
		r_turb_pdest = (unsigned char *)((byte *)d_viewbuffer +
			(r_screenwidth * pspan->v) + pspan->u);

		count = pspan->count;

		// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

		r_turb_s = (int)(sdivz * z) + sadjust;
		if (r_turb_s > bbextents)
			r_turb_s = bbextents;
		else if (r_turb_s < 0)
			r_turb_s = 0;

		r_turb_t = (int)(tdivz * z) + tadjust;
		if (r_turb_t > bbextentt)
			r_turb_t = bbextentt;
		else if (r_turb_t < 0)
			r_turb_t = 0;

		do
		{
			// calculate s and t at the far end of the span
			if (count >= 16)
				r_turb_spancount = 16;
			else
				r_turb_spancount = count;

			count -= r_turb_spancount;

			if (count)
			{
				// calculate s/z, t/z, zi->fixed s and t at far end of span,
				// calculate s and t steps across span by shifting
				sdivz += sdivzstepu;
				tdivz += tdivzstepu;
				zi += zistepu;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;	// prevent round-off error on <0 steps from
				//  from causing overstepping & running off the
				//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;	// guard against round-off error on <0 steps

				r_turb_sstep = (snext - r_turb_s) >> 4;
				r_turb_tstep = (tnext - r_turb_t) >> 4;
			}
			else
			{
				// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so
				// can't step off polygon), clamp, calculate s and t steps across
				// span by division, biasing steps low so we don't run off the
				// texture
				spancountminus1 = (float)(r_turb_spancount - 1);
				sdivz += d_sdivzstepu * spancountminus1;
				tdivz += d_tdivzstepu * spancountminus1;
				zi += d_zistepu * spancountminus1;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;	// prevent round-off error on <0 steps from
				//  from causing overstepping & running off the
				//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;	// guard against round-off error on <0 steps

				if (r_turb_spancount > 1)
				{
					r_turb_sstep = (snext - r_turb_s) / (r_turb_spancount - 1);
					r_turb_tstep = (tnext - r_turb_t) / (r_turb_spancount - 1);
				}
			}

			r_turb_s = r_turb_s & ((CYCLE << 16) - 1);
			r_turb_t = r_turb_t & ((CYCLE << 16) - 1);

			D_DrawTurbulent8Span(pspan);

			r_turb_s = snext;
			r_turb_t = tnext;

		} while (count > 0);

	} while ((pspan = pspan->pnext) != NULL);
}

//====================
//PGM
/*
=============
NonTurbulent8 - this is for drawing scrolling textures. they're warping water textures
but the turbulence is automatically 0.
=============
*/
void NonTurbulent8(espan_t *pspan)
{

	//	r_turb_turb = sintable + ((int)(r_newrefdef.time*SPEED)&(CYCLE-1));
	r_turb_turb = blanktable;

	r_turb_sstep = 0;	// keep compiler happy
	r_turb_tstep = 0;	// ditto

	r_turb_pbase = (unsigned char *)cacheblock;

	sdivzstepu = d_sdivzstepu * 16;
	tdivzstepu = d_tdivzstepu * 16;
	zistepu = d_zistepu * 16;

	do
	{
		r_turb_pdest = (unsigned char *)((byte *)d_viewbuffer +
			(r_screenwidth * pspan->v) + pspan->u);

		count = pspan->count;

		// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

		r_turb_s = (int)(sdivz * z) + sadjust;
		if (r_turb_s > bbextents)
			r_turb_s = bbextents;
		else if (r_turb_s < 0)
			r_turb_s = 0;

		r_turb_t = (int)(tdivz * z) + tadjust;
		if (r_turb_t > bbextentt)
			r_turb_t = bbextentt;
		else if (r_turb_t < 0)
			r_turb_t = 0;

		do
		{
			// calculate s and t at the far end of the span
			if (count >= 16)
				r_turb_spancount = 16;
			else
				r_turb_spancount = count;

			count -= r_turb_spancount;

			if (count)
			{
				// calculate s/z, t/z, zi->fixed s and t at far end of span,
				// calculate s and t steps across span by shifting
				sdivz += sdivzstepu;
				tdivz += tdivzstepu;
				zi += zistepu;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point

				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;	// prevent round-off error on <0 steps from
				//  from causing overstepping & running off the
				//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;	// guard against round-off error on <0 steps

				r_turb_sstep = (snext - r_turb_s) >> 4;
				r_turb_tstep = (tnext - r_turb_t) >> 4;
			}
			else
			{
				// calculate s/z, t/z, zi->fixed s and t at last pixel in span (so
				// can't step off polygon), clamp, calculate s and t steps across
				// span by division, biasing steps low so we don't run off the
				// texture
				spancountminus1 = (float)(r_turb_spancount - 1);
				sdivz += d_sdivzstepu * spancountminus1;
				tdivz += d_tdivzstepu * spancountminus1;
				zi += d_zistepu * spancountminus1;
				z = (float)0x10000 / zi;	// prescale to 16.16 fixed-point
				snext = (int)(sdivz * z) + sadjust;
				if (snext > bbextents)
					snext = bbextents;
				else if (snext < 16)
					snext = 16;	// prevent round-off error on <0 steps from
				//  from causing overstepping & running off the
				//  edge of the texture

				tnext = (int)(tdivz * z) + tadjust;
				if (tnext > bbextentt)
					tnext = bbextentt;
				else if (tnext < 16)
					tnext = 16;	// guard against round-off error on <0 steps

				if (r_turb_spancount > 1)
				{
					r_turb_sstep = (snext - r_turb_s) / (r_turb_spancount - 1);
					r_turb_tstep = (tnext - r_turb_t) / (r_turb_spancount - 1);
				}
			}

			r_turb_s = r_turb_s & ((CYCLE << 16) - 1);
			r_turb_t = r_turb_t & ((CYCLE << 16) - 1);

			D_DrawTurbulent8Span(pspan);

			r_turb_s = snext;
			r_turb_t = tnext;

		} while (count > 0);

	} while ((pspan = pspan->pnext) != NULL);
}
//PGM
//====================


#if	!id386

/*
=============
D_DrawSpans16

FIXME: actually make this subdivide by 16 instead of 8!!!  qb:  OK!!!!
=============
*/

/*==============================================
//unrolled- mh, MK, qbism
//============================================*/


//qb: this one does a simple motion blur, but leaves artificacts (smears on walls) due to alphamap imperfections.
//#define WRITEPDEST_MB(i)  { pdest[i] = vid.alphamap[*(pbase + (s >> 16) + (t >> 16) * cachewidth)*256+pdest[i]]; s+=sstep; t+=tstep;}

//qbism: pointer to pbase and macroize idea from mankrip
#define WRITEPDEST(i)   { pdest[i] = *(pbase + (s >> 16) + (t >> 16) * cachewidth); s+=sstep; t+=tstep;}

void D_DrawSpans16(espan_t *pspan) //qb: up it from 8 to 16.  This + unroll = big speed gain!
{
	sstep = 0;   // keep compiler happy
	tstep = 0;   // ditto

	pbase = (byte *)cacheblock;
	sdivzstepu = d_sdivzstepu * 16;
	tdivzstepu = d_tdivzstepu * 16;
	zistepu = d_zistepu * 16;

	do
	{
		pdest = (byte *)((byte *)d_viewbuffer + (r_screenwidth * pspan->v) + pspan->u);
		count = pspan->count >> 4;

		spancount = pspan->count % 16;

		// calculate the initial s/z, t/z, 1/z, s, and t and clamp
		du = (float)pspan->u;
		dv = (float)pspan->v;

		sdivz = d_sdivzorigin + dv*d_sdivzstepv + du*d_sdivzstepu;
		tdivz = d_tdivzorigin + dv*d_tdivzstepv + du*d_tdivzstepu;
		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point

		s = (int)(sdivz * z) + sadjust;
		if (s < 0) s = 0;
		else if (s > bbextents) s = bbextents;

		t = (int)(tdivz * z) + tadjust;
		if (t < 0) t = 0;
		else if (t > bbextentt) t = bbextentt;

		while (count-- > 0) // Manoel Kasimier
		{
			sdivz += sdivzstepu;
			tdivz += tdivzstepu;
			zi += zistepu;
			z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point

			snext = (int)(sdivz * z) + sadjust;
			if (snext < 16) snext = 16;
			else if (snext > bbextents) snext = bbextents;

			tnext = (int)(tdivz * z) + tadjust;
			if (tnext < 16) tnext = 16;
			else if (tnext > bbextentt) tnext = bbextentt;

			sstep = (snext - s) >> 4;
			tstep = (tnext - t) >> 4;
			pdest += 16;
			WRITEPDEST(-16);
			WRITEPDEST(-15);
			WRITEPDEST(-14);
			WRITEPDEST(-13);
			WRITEPDEST(-12);
			WRITEPDEST(-11);
			WRITEPDEST(-10);
			WRITEPDEST(-9);
			WRITEPDEST(-8);
			WRITEPDEST(-7);
			WRITEPDEST(-6);
			WRITEPDEST(-5);
			WRITEPDEST(-4);
			WRITEPDEST(-3);
			WRITEPDEST(-2);
			WRITEPDEST(-1);
			s = snext;
			t = tnext;
		}
		if (spancount > 0)
		{
			spancountminus1 = (float)(spancount - 1);
			sdivz += d_sdivzstepu * spancountminus1;
			tdivz += d_tdivzstepu * spancountminus1;
			zi += d_zistepu * spancountminus1;
			z = (float)0x10000 / zi;   // prescale to 16.16 fixed-point

			snext = (int)(sdivz * z) + sadjust;
			if (snext < 16) snext = 16;
			else if (snext > bbextents) snext = bbextents;

			tnext = (int)(tdivz * z) + tadjust;
			if (tnext < 16) tnext = 16;
			else if (tnext > bbextentt) tnext = bbextentt;

			if (spancount > 1)
			{
				sstep = (snext - s) / (spancount - 1);
				tstep = (tnext - t) / (spancount - 1);
			}

			pdest += spancount;

			switch (spancount)
			{
			case 16:
				WRITEPDEST(-16);
			case 15:
				WRITEPDEST(-15);
			case 14:
				WRITEPDEST(-14);
			case 13:
				WRITEPDEST(-13);
			case 12:
				WRITEPDEST(-12);
			case 11:
				WRITEPDEST(-11);
			case 10:
				WRITEPDEST(-10);
			case  9:
				WRITEPDEST(-9);
			case  8:
				WRITEPDEST(-8);
			case  7:
				WRITEPDEST(-7);
			case  6:
				WRITEPDEST(-6);
			case  5:
				WRITEPDEST(-5);
			case  4:
				WRITEPDEST(-4);
			case  3:
				WRITEPDEST(-3);
			case  2:
				WRITEPDEST(-2);
			case  1:
				WRITEPDEST(-1);
				break;
			}
		}
	} while ((pspan = pspan->pnext) != NULL);
}

#endif


#if	!id386

/*
=============
D_DrawZSpans
=============
*/
void D_DrawZSpans(espan_t *pspan)
{
	short			*pdest;
	unsigned		ltemp;

	// FIXME: check for clamping/range problems
	// we count on FP exceptions being turned off to avoid range problems
	izistep = (int)(d_zistepu * 0x8000 * 0x10000);

	do
	{
		pdest = d_pzbuffer + (d_zwidth * pspan->v) + pspan->u;

		count = pspan->count;

		// calculate the initial 1/z
		du = (float)pspan->u;
		dv = (float)pspan->v;

		zi = d_ziorigin + dv*d_zistepv + du*d_zistepu;
		// we count on FP exceptions being turned off to avoid range problems
		izi = (int)(zi * 0x8000 * 0x10000);

		if ((intptr_t)pdest & 0x02)
		{
			*pdest++ = (short)(izi >> 16);
			izi += izistep;
			count--;
		}

		if ((spancount = count >> 1) > 0)
		{
			do
			{
				ltemp = izi >> 16;
				izi += izistep;
				ltemp |= izi & 0xFFFF0000;
				izi += izistep;
				*(int *)pdest = ltemp;
				pdest += 2;
			} while (--spancount > 0);
		}

		if (count & 1)
			*pdest = (short)(izi >> 16);

	} while ((pspan = pspan->pnext) != NULL);
}

#endif

