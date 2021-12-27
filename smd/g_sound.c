#include "g_local.h"

#ifdef DISABLE_FMOD

void PlayFootstep (edict_t *ent, footstep_t index)	{}
void FootStep (edict_t *ent)	{}
qboolean FMOD_Init ()
{
	return false;
}
void FMOD_Shutdown ()	{}
void FMOD_UpdateListenerPos ()	{}
void FMOD_UpdateSpeakerPos (edict_t *speaker)	{}
void FMOD_Stop ()	{}
void CheckEndMusic (edict_t *ent)	{}
void CheckEndSample (edict_t *ent)	{}
int FMOD_PlaySound (edict_t *ent)
{
	return 0;
}
void FMOD_StopSound (edict_t *ent, qboolean free)	{}
void target_playback_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)	{}
void target_playback_fadeout (edict_t *ent)	{}
void target_playback_fadein (edict_t *ent)	{}
qboolean FMOD_IsPlaying (edict_t *ent)
{
	return false;
}
void Use_Target_Playback (edict_t *ent, edict_t *other, edict_t *activator)	{}
void target_playback_delayed_start (edict_t *ent)	{}
void target_playback_delayed_restart (edict_t *ent)	{}
void SP_target_playback (edict_t *ent)	{}
#ifdef FMOD_FOOTSTEPS
void ReadTextureSurfaceAssignments ()	{}
#endif // FMOD_FOOTSTEPS

#else // DISABLE_FMOD

#include "fmod.h"
#include <windows.h>

#define SF_PLAYBACK_LOOP          1
#define SF_PLAYBACK_TOGGLE        2
#define SF_PLAYBACK_MUSIC         4	 // Is set, mapper says this is "music" and won't be played if fmod_nomusic is set
#define SF_PLAYBACK_STARTON       8
#define SF_PLAYBACK_3D           16  // Position info is used
#define SF_PLAYBACK_MUSICFILE    64	 // Internal use, used to distinguish .mod, .s3m, .xm files
#define SF_PLAYBACK_SAMPLE      128  // Used to distinguish samples from streams
#define SF_PLAYBACK_TURNED_OFF  256	 // Internal use, set when a target_playback is TURNED OFF rather
                                     // than simply reaching the end

HMODULE		hFMOD=(HMODULE)NULL;	// Handle of fmod.dll

// Knightmare- this is now handled client-side
#ifdef FMOD_FOOTSTEPS
qboolean	qFMOD_Footsteps;		// Set to false in SP_worldspawn for DM/coop

struct texsurf_s
{
	int		step_id;
	char	tex[16];
};
typedef struct texsurf_s texsurf_t;
#define MAX_TEX_SURF 256
texsurf_t tex_surf[MAX_TEX_SURF];
int	num_texsurfs;
#endif // FMOD_FOOTSTEPS


/* Here's what Lazarus does at the end of SP_worldspawn:

	if(footstep_sounds->value)
		world->effects |= FX_WORLDSPAWN_STEPSOUNDS;

  	if(deathmatch->value || coop->value)
		qFMOD_Footsteps = false;
	else if(world->effects & FX_WORLDSPAWN_STEPSOUNDS)
	{
		qFMOD_Footsteps = true;
		FMOD_Init();
	}
	else
		qFMOD_Footsteps = false;

  You'll notice that we don't use FMOD to play any footstep sounds UNLESS a map effects flag is 
  set - the only good reason for this requirement is kinda complicated: 1) FMOD doesn't like
  "s_primary 1" at all, and using that setting will foul up ALL sounds on some sound cards
  whether any FMOD functions are used or not. 2) We want to precache footstep sounds for
  performance reasons. However, we can't know at runtime whether any surfaces in the map
  use new surface flags for footsteps. So.. we want to call FMOD_Init to load FMOD and 
  precache footsteps, but we DON'T want to do that unless we really need to so that players
  who normally use "s_primary 1" can continue to do so.

  The footstep_sounds cvar forces STEPSOUNDS on, AND tells the code to check for the 
  existence of and read texsurfs.txt in the game folder. Surface flag values override settings
  in texsurfs.txt.

  One nice optimization - once a match is found for a texture in texsurfs.txt, the 
  applicable surface_t structure has the appropriate surface flag set by the code. This means
  that subsequent checks of the surface will involve only an integer comparison rather than
  searching through the table with many string comparisons. Also, if a texture is NOT found
  in texsurfs.txt, SURF_STANDARD gets set so that we don't need to check this surface again.
*/

/* All of the following isn't strictly necessary. Rather than using LoadLibrary and 
   GetProcAddress for all the FMOD functions, you COULD just link to their import
   library. However, that means that your DLL will fail to load if FMOD.DLL isn't
   found. Doing it this way removes that problem. Note that you'll also need to
   comment out all of the API definitions in FMOD.H if you use this method. */

typedef signed char (__stdcall *FMUSIC_FREESONG) (FMUSIC_MODULE *mod);
typedef int (__stdcall *FMUSIC_GETMASTERVOLUME) (FMUSIC_MODULE *mod);
typedef signed char (__stdcall *FMUSIC_ISPLAYING) (FMUSIC_MODULE *mod);
typedef FMUSIC_MODULE *(__stdcall *FMUSIC_LOADSONG) (const char *name);
typedef signed char (__stdcall *FMUSIC_SETMASTERVOLUME) (FMUSIC_MODULE *mod,int volume);
typedef signed char (__stdcall *FMUSIC_PLAYSONG) (FMUSIC_MODULE *mod);
typedef void (__stdcall *FMUSIC_STOPALLSONGS) ();
typedef signed char (__stdcall *FMUSIC_STOPSONG) (FMUSIC_MODULE *mod);

typedef void (__stdcall *FSOUND_3D_LISTENER_SETATTRIBUTES) (float *pos,float *vel,float fx,float fy,float fz,float tx,float ty,float tz);
typedef void (__stdcall *FSOUND_3D_LISTENER_SETDISTANCEFACTOR) (float scale);
typedef void (__stdcall *FSOUND_3D_LISTENER_SETDOPPLERFACTOR) (float scale);
typedef void (__stdcall *FSOUND_3D_LISTENER_SETROLLOFFFACTOR) (float scale);
typedef signed char (__stdcall *FSOUND_3D_SETATTRIBUTES) (int channel,float *pos,float *vel);
typedef void (__stdcall *FSOUND_3D_UPDATE) ();
typedef void (__stdcall *FSOUND_CLOSE) ();
typedef int (__stdcall *FSOUND_GETERROR) ();
typedef int (__stdcall *FSOUND_GETVOLUME) (int channel);
typedef signed char (__stdcall *FSOUND_INIT) (int mixrate,
											  int maxsoftwarechannels,
											  unsigned int flags );
typedef signed char (__stdcall *FSOUND_ISPLAYING) (int channel);
typedef int (__stdcall *FSOUND_PLAYSOUND3DATTRIB) (int channel,
												   FSOUND_SAMPLE *sptr,
												   int freq,
												   int vol,
												   int pan,
												   float *pos,
												   float *vel);
typedef signed char (__stdcall *FSOUND_REVERB_SETMIX) (int channel, float mix);
typedef void (__stdcall *FSOUND_SAMPLE_FREE) (FSOUND_SAMPLE *sptr);
typedef FSOUND_SAMPLE *(__stdcall *FSOUND_SAMPLE_LOAD) (int index,const char *name,unsigned int mode,int memlength);
typedef signed char (__stdcall *FSOUND_SAMPLE_SETMINMAXDISTANCE) (FSOUND_SAMPLE *sptr,float min,float max);

typedef signed char (__stdcall *FSOUND_SETBUFFERSIZE) (int len_ms);
typedef signed char (__stdcall *FSOUND_SETMIXER) (int mixer);
typedef signed char (__stdcall *FSOUND_SETOUTPUT) (int outputtype);
typedef signed char (__stdcall *FSOUND_SETVOLUME) (int channel, int volume);
typedef signed char (__stdcall *FSOUND_STOPSOUND) (int channel);
typedef signed char (__stdcall *FSOUND_STREAM_CLOSE) (FSOUND_STREAM *stream);
typedef int	(__stdcall *FSOUND_STREAM_GETLENGTHMS) (FSOUND_STREAM *stream);
typedef FSOUND_STREAM *(__stdcall *FSOUND_STREAM_OPENFILE) (const char *filename,
															unsigned int mode,
															int memlength);
typedef int (__stdcall *FSOUND_STREAM_PLAY) (int channel, FSOUND_STREAM *stream);
typedef int (__stdcall *FSOUND_STREAM_PLAY3DATTRIB) (int channel,
													 FSOUND_STREAM *stream,
													 int freq,
													 int vol,
													 int pan,
													 float *pos,
													 float *vel );
typedef signed char (__stdcall *FSOUND_STREAM_SETENDCALLBACK) (FSOUND_STREAM *stream,
															   FSOUND_STREAMCALLBACK callback,
															   int userdata);


FMUSIC_FREESONG							FMUSIC_FreeSong;
FMUSIC_GETMASTERVOLUME					FMUSIC_GetMasterVolume;
FMUSIC_ISPLAYING						FMUSIC_IsPlaying;
FMUSIC_LOADSONG							FMUSIC_LoadSong;
FMUSIC_PLAYSONG							FMUSIC_PlaySong;
FMUSIC_SETMASTERVOLUME					FMUSIC_SetMasterVolume;
FMUSIC_STOPALLSONGS						FMUSIC_StopAllSongs;
FMUSIC_STOPSONG							FMUSIC_StopSong;

FSOUND_3D_LISTENER_SETATTRIBUTES		FSOUND_3D_Listener_SetAttributes;
FSOUND_3D_LISTENER_SETDISTANCEFACTOR	FSOUND_3D_Listener_SetDistanceFactor;
FSOUND_3D_LISTENER_SETDOPPLERFACTOR		FSOUND_3D_Listener_SetDopplerFactor;
FSOUND_3D_LISTENER_SETROLLOFFFACTOR		FSOUND_3D_Listener_SetRolloffFactor;
FSOUND_3D_SETATTRIBUTES                 FSOUND_3D_SetAttributes;
FSOUND_3D_UPDATE						FSOUND_3D_Update;
FSOUND_CLOSE							FSOUND_Close;
FSOUND_GETERROR							FSOUND_GetError;
FSOUND_GETVOLUME						FSOUND_GetVolume;
FSOUND_INIT								FSOUND_Init;
FSOUND_ISPLAYING						FSOUND_IsPlaying;
FSOUND_PLAYSOUND3DATTRIB				FSOUND_PlaySound3DAttrib;
FSOUND_REVERB_SETMIX                    FSOUND_Reverb_SetMix;
FSOUND_SAMPLE_FREE						FSOUND_Sample_Free;
FSOUND_SAMPLE_LOAD						FSOUND_Sample_Load;
FSOUND_SAMPLE_SETMINMAXDISTANCE			FSOUND_Sample_SetMinMaxDistance;
FSOUND_SETOUTPUT						FSOUND_SetOutput;
FSOUND_SETVOLUME						FSOUND_SetVolume;
FSOUND_STOPSOUND						FSOUND_StopSound;
FSOUND_STREAM_CLOSE						FSOUND_Stream_Close;
FSOUND_STREAM_GETLENGTHMS				FSOUND_Stream_GetLengthMs;
FSOUND_STREAM_OPENFILE					FSOUND_Stream_OpenFile;
FSOUND_STREAM_PLAY						FSOUND_Stream_Play;
FSOUND_STREAM_PLAY3DATTRIB				FSOUND_Stream_Play3DAttrib;
FSOUND_STREAM_SETENDCALLBACK			FSOUND_Stream_SetEndCallback;

/* Lazarus currently has 44 footstep sounds. These sounds are precached to avoid
   a barely noticeable hiccup when the sounds are first played. The #defines for the 
   Footstep[] indices are in q_shared.h */

//Knightmare- this is now handled client-side
#ifdef FMOD_FOOTSTEPS
FSOUND_SAMPLE *Footstep[44];

void PlayFootstep(edict_t *ent, footstep_t index)
{
	if(index < 0)
		ent->s.event = EV_FOOTSTEP;
	else
	{
		if(hFMOD && Footstep[index])
			FSOUND_PlaySound3DAttrib(FSOUND_FREE, Footstep[index], -1, 128, -1, NULL, NULL );
		else
			ent->s.event = EV_FOOTSTEP;
	}
	/* Mapper has the option of setting a worldspawn flag to generate a "player_noise"
	   entity when the player makes a footstep sound. Normally, monsters ignore footsteps. */
	if(world->effects & FX_WORLDSPAWN_ALERTSOUNDS)
		PlayerNoise(ent,ent->s.origin,PNOISE_SELF);
}

static char *FootFile(char *in)
{
	static char filename[256];
	static char prefix[256];
	static int	init=0;

	if(!init)
	{
		cvar_t	*basedir, *gamedir;
		basedir = gi.cvar("basedir", "", 0);
		gamedir = gi.cvar("gamedir", "", 0);
		if(strlen(gamedir->string))
			sprintf(prefix,"%s/%s/sound/player/",basedir->string,gamedir->string);
		else
			sprintf(prefix,"%s/sound/player/",basedir->string);
		init = 1;
	}
	sprintf(filename,"%s%s",prefix,in);
	return filename;
}
static void PrecacheFootsteps()
{
	int	mode=FSOUND_LOOP_OFF | FSOUND_2D;

	if(!hFMOD)
		return;

	Footstep[FOOTSTEP_SLOSH1]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_slosh1.wav"),  mode, 0);
	Footstep[FOOTSTEP_SLOSH2]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_slosh2.wav"),  mode, 0);
	Footstep[FOOTSTEP_SLOSH3]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_slosh3.wav"),  mode, 0);
	Footstep[FOOTSTEP_SLOSH4]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_slosh4.wav"),  mode, 0);
	Footstep[FOOTSTEP_LADDER1] = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_ladder1.wav"), mode, 0);
	Footstep[FOOTSTEP_LADDER2] = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_ladder2.wav"), mode, 0);
	Footstep[FOOTSTEP_LADDER3] = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_ladder3.wav"), mode, 0);
	Footstep[FOOTSTEP_LADDER4] = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_ladder4.wav"), mode, 0);
	Footstep[FOOTSTEP_METAL1]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_metal1.wav"),  mode, 0);
	Footstep[FOOTSTEP_METAL2]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_metal2.wav"),  mode, 0);
	Footstep[FOOTSTEP_METAL3]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_metal3.wav"),  mode, 0);
	Footstep[FOOTSTEP_METAL4]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_metal4.wav"),  mode, 0);
	Footstep[FOOTSTEP_DIRT1]   = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_dirt1.wav"),   mode, 0);
	Footstep[FOOTSTEP_DIRT2]   = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_dirt2.wav"),   mode, 0);
	Footstep[FOOTSTEP_DIRT3]   = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_dirt3.wav"),   mode, 0);
	Footstep[FOOTSTEP_DIRT4]   = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_dirt4.wav"),   mode, 0);
	Footstep[FOOTSTEP_VENT1]   = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_duct1.wav"),   mode, 0);
	Footstep[FOOTSTEP_VENT2]   = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_duct2.wav"),   mode, 0);
	Footstep[FOOTSTEP_VENT3]   = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_duct3.wav"),   mode, 0);
	Footstep[FOOTSTEP_VENT4]   = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_duct4.wav"),   mode, 0);
	Footstep[FOOTSTEP_GRATE1]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_grate1.wav"),  mode, 0);
	Footstep[FOOTSTEP_GRATE2]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_grate2.wav"),  mode, 0);
	Footstep[FOOTSTEP_GRATE3]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_grate3.wav"),  mode, 0);
	Footstep[FOOTSTEP_GRATE4]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_grate4.wav"),  mode, 0);
	Footstep[FOOTSTEP_TILE1]   = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_tile1.wav"),   mode, 0);
	Footstep[FOOTSTEP_TILE2]   = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_tile2.wav"),   mode, 0);
	Footstep[FOOTSTEP_TILE3]   = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_tile3.wav"),   mode, 0);
	Footstep[FOOTSTEP_TILE4]   = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_tile4.wav"),   mode, 0);
	Footstep[FOOTSTEP_GRASS1]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_grass1.wav"),  mode, 0);
	Footstep[FOOTSTEP_GRASS2]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_grass2.wav"),  mode, 0);
	Footstep[FOOTSTEP_GRASS3]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_grass3.wav"),  mode, 0);
	Footstep[FOOTSTEP_GRASS4]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_grass4.wav"),  mode, 0);
	Footstep[FOOTSTEP_SNOW1]   = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_snow1.wav"),   mode, 0);
	Footstep[FOOTSTEP_SNOW2]   = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_snow2.wav"),   mode, 0);
	Footstep[FOOTSTEP_SNOW3]   = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_snow3.wav"),   mode, 0);
	Footstep[FOOTSTEP_SNOW4]   = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_snow4.wav"),   mode, 0);
	Footstep[FOOTSTEP_CARPET1] = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_carpet1.wav"), mode, 0);
	Footstep[FOOTSTEP_CARPET2] = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_carpet2.wav"), mode, 0);
	Footstep[FOOTSTEP_CARPET3] = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_carpet3.wav"), mode, 0);
	Footstep[FOOTSTEP_CARPET4] = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_carpet4.wav"), mode, 0);
	Footstep[FOOTSTEP_FORCE1]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_force1.wav"),  mode, 0);
	Footstep[FOOTSTEP_FORCE2]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_force2.wav"),  mode, 0);
	Footstep[FOOTSTEP_FORCE3]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_force3.wav"),  mode, 0);
	Footstep[FOOTSTEP_FORCE4]  = FSOUND_Sample_Load(FSOUND_FREE,FootFile("pl_force4.wav"),  mode, 0);
}

/*
=================
FootStep

Plays appropriate footstep sound depending on surface flags of the ground.
Since this is a replacement for plain Jane EV_FOOTSTEP, we already know
the player is definitely on the ground when this is called.
=================
*/
void FootStep(edict_t *ent)
{
	static int iSkipStep = 0;

	trace_t	tr;
	vec3_t	end;
	int		r;
	int		surface;

	r = (rand() & 1) + ent->client->leftfoot*2;
	ent->client->leftfoot = 1 - ent->client->leftfoot;

	if(qFMOD_Footsteps)
	{
		if((ent->waterlevel == 1) && (ent->watertype & CONTENTS_WATER))
		{
			// Slosh sounds
			PlayFootstep(ent,FOOTSTEP_SLOSH1 + r);
			return;
		}
		if((ent->waterlevel == 2) && (ent->watertype & CONTENTS_WATER))
		{
			// Wade sounds
			if(iSkipStep == 0)
			{
				iSkipStep++;
				return;
			}
			if(iSkipStep++ == 3)
				iSkipStep = 0;
			
			switch(r) {
			case 0:	gi.sound(ent,CHAN_VOICE,gi.soundindex("player/wade1.wav"),1.0,ATTN_NORM,0);	break;
			case 1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/wade2.wav"),1.0,ATTN_NORM,0); break;
			case 2: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/wade1.wav"),1.0,ATTN_NORM,0); break;
			case 3: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/wade3.wav"),1.0,ATTN_NORM,0); break;
			}
			if(world->effects & FX_WORLDSPAWN_ALERTSOUNDS)
				PlayerNoise(ent,ent->s.origin,PNOISE_SELF);
			return;
		}
		VectorCopy(ent->s.origin,end);
		end[2] -= 64;
		tr = gi.trace(ent->s.origin,NULL,NULL,end,ent,MASK_SOLID | MASK_WATER);
		if(!tr.surface)
		{
			PlayFootstep(ent,-1);
			return;
		}
		
		surface = tr.surface->flags & SURF_STEPMASK;
		switch (surface) {
		case SURF_METAL:
			PlayFootstep(ent,FOOTSTEP_METAL1 + r);
			break;
		case SURF_DIRT:
			PlayFootstep(ent,FOOTSTEP_DIRT1 + r);
			break;
		case SURF_VENT:
			PlayFootstep(ent,FOOTSTEP_VENT1 + r);
			break;
		case SURF_GRATE:
			PlayFootstep(ent,FOOTSTEP_GRATE1 + r);
			break;
		case SURF_TILE:
			PlayFootstep(ent,FOOTSTEP_TILE1 + r);
			break;
		case SURF_GRASS:
			PlayFootstep(ent,FOOTSTEP_GRASS1 + r);
			break;
		case SURF_SNOW:
			PlayFootstep(ent,FOOTSTEP_SNOW1 + r);
			break;
		case SURF_CARPET:
			PlayFootstep(ent,FOOTSTEP_CARPET1 + r);
			break;
		case SURF_FORCE:
			PlayFootstep(ent,FOOTSTEP_FORCE1 + r);
			break;
		case SURF_STANDARD:
			PlayFootstep(ent,-1);
			break;
		default:
			if(footstep_sounds->value && num_texsurfs)
			{
				int	i;
				int step=-1;
				for(i=0; i<num_texsurfs; i++)
				{
					if(strstr(tr.surface->name,tex_surf[i].tex))
					{
						if(!tex_surf[i].step_id)
						{
							step = -1;
							tr.surface->flags |= SURF_STANDARD;
						}
						else
						{
							step = 4*(tex_surf[i].step_id-1) + r;
							tr.surface->flags |= (SURF_METAL << (tex_surf[i].step_id - 1));
						}
						break;
					}
				}
				PlayFootstep(ent,step);
			}
			else
				PlayFootstep(ent,-1);
		}
	}
	else
	{	// Use normal Q2 sound. Note that no provisions are made for potential
		// Index Overflow problems. Coders need to make sure that mappers are
		// aware of this for multiplayer (non-FMOD) maps.
		if((ent->waterlevel == 1) && (ent->watertype & CONTENTS_WATER))
		{
			// Slosh sounds
			switch(r)
			{
			case 0:	gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_slosh1.wav"),1.0,ATTN_NORM,0); break;
			case 1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_slosh3.wav"),1.0,ATTN_NORM,0); break;
			case 2:	gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_slosh2.wav"),1.0,ATTN_NORM,0);	break;
			case 3:	gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_slosh4.wav"),1.0,ATTN_NORM,0);	break;
			}
			if(world->effects & FX_WORLDSPAWN_ALERTSOUNDS)
				PlayerNoise(ent,ent->s.origin,PNOISE_SELF);
			return;
		}
		if((ent->waterlevel == 2) && (ent->watertype & CONTENTS_WATER))
		{
			// Wade sounds
			if(iSkipStep == 0)
			{
				iSkipStep++;
				return;
			}
			if(iSkipStep++ == 3)
				iSkipStep = 0;
			
			switch(r) {
			case 0:	gi.sound(ent,CHAN_VOICE,gi.soundindex("player/wade1.wav"),1.0,ATTN_NORM,0);	break;
			case 1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/wade2.wav"),1.0,ATTN_NORM,0); break;
			case 2: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/wade1.wav"),1.0,ATTN_NORM,0); break;
			case 3: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/wade3.wav"),1.0,ATTN_NORM,0); break;
			}
			if(world->effects & FX_WORLDSPAWN_ALERTSOUNDS)
				PlayerNoise(ent,ent->s.origin,PNOISE_SELF);
			return;
		}
		VectorCopy(ent->s.origin,end);
		end[2] -= 32;
		tr = gi.trace(ent->s.origin,NULL,NULL,end,ent,MASK_SOLID | MASK_WATER);
		if(!tr.surface)
		{
			PlayFootstep(ent,-1);
			return;
		}
		
		surface = tr.surface->flags & SURF_STEPMASK;
		switch (surface) {
		case SURF_METAL:
			switch (r) {
			case 0: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_metal1.wav"),1.0,ATTN_NORM,0); break;
			case 1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_metal3.wav"),1.0,ATTN_NORM,0); break;
			case 2: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_metal2.wav"),1.0,ATTN_NORM,0); break;
			case 3: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_metal4.wav"),1.0,ATTN_NORM,0); break;
			}
			break;
		case SURF_DIRT:
			switch (r) {
			case 0: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_dirt1.wav"),1.0,ATTN_NORM,0); break;
			case 1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_dirt3.wav"),1.0,ATTN_NORM,0); break;
			case 2: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_dirt2.wav"),1.0,ATTN_NORM,0); break;
			case 3: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_dirt4.wav"),1.0,ATTN_NORM,0); break;
			}
			break;
		case SURF_VENT:
			switch (r) {
			case 0: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_duct1.wav"),1.0,ATTN_NORM,0); break;
			case 1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_duct3.wav"),1.0,ATTN_NORM,0); break;
			case 2: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_duct2.wav"),1.0,ATTN_NORM,0); break;
			case 3: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_duct4.wav"),1.0,ATTN_NORM,0); break;
			}
			break;
		case SURF_GRATE:
			switch (r) {
			case 0: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_grate1.wav"),1.0,ATTN_NORM,0); break;
			case 1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_grate3.wav"),1.0,ATTN_NORM,0); break;
			case 2: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_grate2.wav"),1.0,ATTN_NORM,0); break;
			case 3: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_grate4.wav"),1.0,ATTN_NORM,0); break;
			}
			break;
		case SURF_TILE:
			switch (r) {
			case 0: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_tile1.wav"),1.0,ATTN_NORM,0); break;
			case 1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_tile3.wav"),1.0,ATTN_NORM,0); break;
			case 2: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_tile2.wav"),1.0,ATTN_NORM,0); break;
			case 3: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_tile4.wav"),1.0,ATTN_NORM,0); break;
			}
			break;
		case SURF_GRASS:
			switch (r) {
			case 0: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_grass1.wav"),1.0,ATTN_NORM,0); break;
			case 1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_grass3.wav"),1.0,ATTN_NORM,0); break;
			case 2: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_grass2.wav"),1.0,ATTN_NORM,0); break;
			case 3: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_grass4.wav"),1.0,ATTN_NORM,0); break;
			}
			break;
		case SURF_SNOW:
			switch (r) {
			case 0: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_snow1.wav"),1.0,ATTN_NORM,0); break;
			case 1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_snow3.wav"),1.0,ATTN_NORM,0); break;
			case 2: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_snow2.wav"),1.0,ATTN_NORM,0); break;
			case 3: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_snow4.wav"),1.0,ATTN_NORM,0); break;
			}
			break;
		case SURF_CARPET:
			switch (r) {
			case 0: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_carpet1.wav"),1.0,ATTN_NORM,0); break;
			case 1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_carpet3.wav"),1.0,ATTN_NORM,0); break;
			case 2: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_carpet2.wav"),1.0,ATTN_NORM,0); break;
			case 3: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_carpet4.wav"),1.0,ATTN_NORM,0); break;
			}
			break;
		case SURF_FORCE:
			switch (r) {
			case 0: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_force1.wav"),1.0,ATTN_NORM,0); break;
			case 1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_force3.wav"),1.0,ATTN_NORM,0); break;
			case 2: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_force2.wav"),1.0,ATTN_NORM,0); break;
			case 3: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_force4.wav"),1.0,ATTN_NORM,0); break;
			}
			break;
		default:
			if(footstep_sounds->value && num_texsurfs)
			{
				int	i;
				int step=-1;
				for(i=0; i<num_texsurfs; i++)
				{
					if(strstr(tr.surface->name,tex_surf[i].tex))
					{
						if(!tex_surf[i].step_id)
						{
							step = -1;
							tr.surface->flags |= SURF_STANDARD;
						}
						else
						{
							step = 4*(tex_surf[i].step_id-1) + r;
							tr.surface->flags |= (SURF_METAL << (tex_surf[i].step_id - 1));
						}
						break;
					}
				}
				switch(step)
				{
				case FOOTSTEP_METAL1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_metal1.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_METAL2: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_metal3.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_METAL3: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_metal2.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_METAL4: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_metal4.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_DIRT1:  gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_dirt1.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_DIRT2:  gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_dirt3.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_DIRT3:  gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_dirt2.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_DIRT4:  gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_dirt4.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_VENT1:  gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_duct1.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_VENT2:  gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_duct3.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_VENT3:  gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_duct2.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_VENT4:  gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_duct4.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_GRATE1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_grate1.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_GRATE2: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_grate3.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_GRATE3: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_grate2.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_GRATE4: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_grate4.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_TILE1:  gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_tile1.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_TILE2:  gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_tile3.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_TILE3:  gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_tile2.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_TILE4:  gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_tile4.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_GRASS1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_grass1.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_GRASS2: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_grass3.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_GRASS3: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_grass2.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_GRASS4: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_grass4.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_SNOW1:  gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_snow1.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_SNOW2:  gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_snow3.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_SNOW3:  gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_snow2.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_SNOW4:  gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_snow4.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_CARPET1:gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_carpet1.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_CARPET2:gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_carpet3.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_CARPET3:gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_carpet2.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_CARPET4:gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_carpet4.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_FORCE1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_force1.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_FORCE2: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_force3.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_FORCE3: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_force2.wav"),1.0,ATTN_NORM,0); break;
				case FOOTSTEP_FORCE4: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_force4.wav"),1.0,ATTN_NORM,0); break;
				default: ent->s.event = EV_FOOTSTEP;
				}
			}
			else
				ent->s.event = EV_FOOTSTEP;
		}
		if(world->effects & FX_WORLDSPAWN_ALERTSOUNDS)
			PlayerNoise(ent,ent->s.origin,PNOISE_SELF);
	}
}

void ReadTextureSurfaceAssignments()
{
	cvar_t	*basedir, *gamedir;
	char	filename[256];
	FILE	*f;
	char	line[80];

	num_texsurfs = 0;

	basedir = gi.cvar("basedir", "", 0);
	gamedir = gi.cvar("gamedir", "", 0);
	strcpy(filename,basedir->string);
	if(strlen(gamedir->string))
	{
		strcat(filename,"\\");
		strcat(filename,gamedir->string);
	}
	strcat(filename,"\\texsurfs.txt");
	f = fopen(filename,"rt");
	if(!f)
		return;

	while(fgets(line, sizeof(line), f) && num_texsurfs < MAX_TEX_SURF)
	{
		(void)sscanf(line,"%d %s",&tex_surf[num_texsurfs].step_id,tex_surf[num_texsurfs].tex);
//		gi.dprintf("%d %s\n",tex_surf[num_texsurfs].step_id,tex_surf[num_texsurfs].tex);
		num_texsurfs++;
	}
	fclose(f);
}
#endif //FMOD footsteps

/* All other footstep-related code is in p_view.c. Short version: replace all 
   "ent->s.event = EV_FOOTSTEP" with a call to Footstep and check out G_SetClientEvent,
   where water and ladder sounds are played. */

qboolean FMOD_Init()
{
	char	dllname[_MAX_PATH];

	if(!deathmatch)
		deathmatch = gi.cvar ("deathmatch", "0", CVAR_LATCH);

	/* This is a Lazarus-specific cvar that overrides the decision to not use FMOD in 
	   DM. Only reason to do this is if you want to use FMOD while playing against bots. 
	   Don't ask me about the crazy name - this is what MD suggested :-) */
	if(!packet_fmod_playback)
		packet_fmod_playback = gi.cvar("packet_fmod_playback", "0", CVAR_SERVERINFO);

	/* FMOD really really REALLY does not like "s_primary 1", particularly on better
	   sound cards. We tried all sorts of workarounds to no avail before settling on
	   bitching at the player and making it HIS fault. */
	if(!s_primary)
		s_primary = gi.cvar("s_primary", "0", 0);
	if(s_primary->value)
	{
		gi.dprintf("target_playback requires s_primary be set to 0.\n"
			       "At the console type:\n"
				   "s_primary 0;sound_restart\n");
	}

	if(deathmatch->value && !packet_fmod_playback->value)
	{
		hFMOD = (HMODULE)(NULL);
		return false;
	}
	GameDirRelativePath("fmod.dll",dllname);
	hFMOD = LoadLibrary(dllname);
	if(hFMOD)
	{
		FMUSIC_FreeSong           = (FMUSIC_FREESONG)GetProcAddress(hFMOD,"_FMUSIC_FreeSong@4");
		FMUSIC_GetMasterVolume    = (FMUSIC_GETMASTERVOLUME)GetProcAddress(hFMOD,"_FMUSIC_GetMasterVolume@4");
		FMUSIC_IsPlaying          = (FMUSIC_ISPLAYING)GetProcAddress(hFMOD,"_FMUSIC_IsPlaying@4");
		FMUSIC_LoadSong           = (FMUSIC_LOADSONG)GetProcAddress(hFMOD,"_FMUSIC_LoadSong@4");
		FMUSIC_PlaySong           = (FMUSIC_PLAYSONG)GetProcAddress(hFMOD,"_FMUSIC_PlaySong@4");
		FMUSIC_SetMasterVolume    = (FMUSIC_SETMASTERVOLUME)GetProcAddress(hFMOD,"_FMUSIC_SetMasterVolume@8");
		FMUSIC_StopAllSongs       = (FMUSIC_STOPALLSONGS)GetProcAddress(hFMOD,"_FMUSIC_StopAllSongs@0");
		FMUSIC_StopSong           = (FMUSIC_STOPSONG)GetProcAddress(hFMOD,"_FMUSIC_StopSong@4");

		FSOUND_3D_Listener_SetAttributes = (FSOUND_3D_LISTENER_SETATTRIBUTES)GetProcAddress(hFMOD,"_FSOUND_3D_Listener_SetAttributes@32");
		FSOUND_3D_Listener_SetDistanceFactor = (FSOUND_3D_LISTENER_SETDISTANCEFACTOR)GetProcAddress(hFMOD,"_FSOUND_3D_Listener_SetDistanceFactor@4");
		FSOUND_3D_Listener_SetDopplerFactor = (FSOUND_3D_LISTENER_SETDOPPLERFACTOR)GetProcAddress(hFMOD,"_FSOUND_3D_Listener_SetDopplerFactor@4");
		FSOUND_3D_Listener_SetRolloffFactor = (FSOUND_3D_LISTENER_SETROLLOFFFACTOR)GetProcAddress(hFMOD,"_FSOUND_3D_Listener_SetRolloffFactor@4");
		FSOUND_3D_SetAttributes   = (FSOUND_3D_SETATTRIBUTES)GetProcAddress(hFMOD,"_FSOUND_3D_SetAttributes@12");
		FSOUND_3D_Update          = (FSOUND_3D_UPDATE)GetProcAddress(hFMOD,"_FSOUND_3D_Update@0");
		FSOUND_Close              = (FSOUND_CLOSE)GetProcAddress(hFMOD,"_FSOUND_Close@0");
		FSOUND_GetError           = (FSOUND_GETERROR)GetProcAddress(hFMOD,"_FSOUND_GetError@0");
		FSOUND_GetVolume          = (FSOUND_GETVOLUME)GetProcAddress(hFMOD,"_FSOUND_GetVolume@4");
		FSOUND_Init               = (FSOUND_INIT)GetProcAddress(hFMOD,"_FSOUND_Init@12");
		FSOUND_IsPlaying          = (FSOUND_ISPLAYING)GetProcAddress(hFMOD,"_FSOUND_IsPlaying@4");
		FSOUND_PlaySound3DAttrib  = (FSOUND_PLAYSOUND3DATTRIB)GetProcAddress(hFMOD,"_FSOUND_PlaySound3DAttrib@28");
		FSOUND_Reverb_SetMix      = (FSOUND_REVERB_SETMIX)GetProcAddress(hFMOD,"_FSOUND_Reverb_SetMix@8");
		FSOUND_Sample_Free        = (FSOUND_SAMPLE_FREE)GetProcAddress(hFMOD,"_FSOUND_Sample_Free@4");
		FSOUND_Sample_Load        = (FSOUND_SAMPLE_LOAD)GetProcAddress(hFMOD,"_FSOUND_Sample_Load@16");
		FSOUND_Sample_SetMinMaxDistance = (FSOUND_SAMPLE_SETMINMAXDISTANCE)GetProcAddress(hFMOD,"_FSOUND_Sample_SetMinMaxDistance@12");

		FSOUND_SetOutput          = (FSOUND_SETOUTPUT)GetProcAddress(hFMOD,"_FSOUND_SetOutput@4");
		FSOUND_SetVolume          = (FSOUND_SETVOLUME)GetProcAddress(hFMOD,"_FSOUND_SetVolume@8");
		FSOUND_StopSound          = (FSOUND_STOPSOUND)GetProcAddress(hFMOD,"_FSOUND_StopSound@4");
		FSOUND_Stream_Close       = (FSOUND_STREAM_CLOSE)GetProcAddress(hFMOD,"_FSOUND_Stream_Close@4");
		FSOUND_Stream_GetLengthMs = (FSOUND_STREAM_GETLENGTHMS)GetProcAddress(hFMOD,"_FSOUND_Stream_GetLengthMs@4");
		FSOUND_Stream_OpenFile    = (FSOUND_STREAM_OPENFILE)GetProcAddress(hFMOD,"_FSOUND_Stream_OpenFile@12");
		FSOUND_Stream_Play        = (FSOUND_STREAM_PLAY)GetProcAddress(hFMOD,"_FSOUND_Stream_Play@8");
		FSOUND_Stream_Play3DAttrib= (FSOUND_STREAM_PLAY3DATTRIB)GetProcAddress(hFMOD,"_FSOUND_Stream_Play3DAttrib@28");
		FSOUND_Stream_SetEndCallback = (FSOUND_STREAM_SETENDCALLBACK)GetProcAddress(hFMOD,"_FSOUND_Stream_SetEndCallback@12");

		if(!FSOUND_Init(44100, 32, 0))
		{
			gi.dprintf("FSOUND_Init failed, error code=%d\n",FSOUND_GetError());
			FSOUND_Close();
			FreeLibrary(hFMOD);
			hFMOD = (HMODULE)(-1);
			return false;
		}
		FSOUND_3D_Listener_SetRolloffFactor(world->attenuation);
		FSOUND_3D_Listener_SetDopplerFactor(world->moveinfo.distance);

//Knightmare- this is now handled client-side
#ifdef FMOD_FOOTSTEPS
		if(qFMOD_Footsteps)
			PrecacheFootsteps();
#endif

		return true;
	}
	return false;
}

/* FMOD_Shutdown is called by ShutdownGame and also by our goofy 
   "sound_restart" console command (which I don't recommend duplicating). */

void FMOD_Shutdown()
{
	if(hFMOD)
	{
		FMOD_Stop();		// stops all target_playback sounds
		FSOUND_Close();
		FreeLibrary(hFMOD);
		hFMOD = (HMODULE)NULL;
	}
}

/*======================================================================================

  Everything below this point is related to target_playback. Don't want target_playback?
  Then you're done :-)

  Add these to the edict_s structure in g_local.h

  	// FMOD
	int			*stream;	// Actually a FSOUND_STREAM * or FMUSIC_MODULE *
	int			channel;

  Only other real change is in WriteLevel. The purpose of the mess below is
  to restart a target_playback that was playing when the player left a level
  if he returns to the same level. The linkcount business is used in 
  target_playback_delayed_restart to postpone playback until the player is
  conscious - initial version didn't do this and it was disconcerting to
  players to have music crank up while the level was loading.

  // write out all the entities
	for (i=0 ; i<globals.num_edicts ; i++)
	{
		ent = &g_edicts[i];
		if (!ent->inuse)
			continue;
		if (!Q_stricmp(ent->classname,"target_playback"))
		{
			edict_t	e;
			memcpy(&e,ent,sizeof(edict_t));
			if(FMOD_IsPlaying(ent))
			{
				e.think = target_playback_delayed_restart;
				e.nextthink = level.time + 1.0;
				// A misuse of groundentity_linkcount, but
				// maybe nobody is watching.
				e.groundentity_linkcount = g_edicts[1].linkcount;
			}
			else
			{
				e.think = NULL;
				e.nextthink = 0;
			}
			e.stream = NULL;
			fwrite (&i, sizeof(i), 1, f);
			WriteEdict (f, &e);
		}
		else
		{
			fwrite (&i, sizeof(i), 1, f);
			WriteEdict (f, ent);
		}
	}

=======================================================================================*/

/* FMOD_UpdateListenerPos is called every frame from G_RunFrame, but ONLY if the 
   map contains 3D sounds. Add this to the end of G_RunFrame:

	if ( (level.num_3D_sounds > 0) && (game.maxclients == 1))
		FMOD_UpdateListenerPos();

   level.num_3D_sounds is incremented in SP_target_playback for 3D sounds and
   decremented if a 3D target_playback "dies".

*/
void FMOD_UpdateListenerPos()
{
	vec3_t	pos, vel, forward, up;
	edict_t	*player = &g_edicts[1];

	if(!player || !player->inuse)
		return;

	if(!hFMOD) return;

	AngleVectors(player->s.angles,forward,NULL,up);
	VectorSet(pos,player->s.origin[1],player->s.origin[2],player->s.origin[0]);
	VectorSet(vel,player->velocity[1],player->velocity[2],player->velocity[0]);
	VectorScale(pos,0.025,pos);
	VectorScale(vel,0.025,vel);
	FSOUND_3D_Listener_SetAttributes(pos,vel,-forward[1],forward[2],-forward[0],-up[1],up[2],-up[0]);
	FSOUND_3D_Update();
}

/* FMOD_UpdateSpeakerPos is called whenever a 3D target_playback moves. Only way to
   move a target_playback in Lazarus currently is to set it to "movewith" a train.
   Lazarus uses 40 Q2 units/meter scale factor (that's where the 0.025 below comes
   in). FMOD *does* allow you to set a scale factor that supposedly allows you to
   use whatever units you want rather than multiplying position and velocity every
   time, but I found that it screws up the attenuation with distance features.     */

void FMOD_UpdateSpeakerPos(edict_t *speaker)
{
	vec3_t	pos, vel;

	if(!hFMOD) return;

	if(!speaker->stream)
		return;

	VectorSet(pos,speaker->s.origin[1],speaker->s.origin[2],speaker->s.origin[0]);
	VectorSet(vel,speaker->velocity[1],speaker->velocity[2],speaker->velocity[0]);
	VectorScale(pos,0.025,pos);
	VectorScale(vel,0.025,vel);
	FSOUND_3D_SetAttributes(speaker->channel,pos,vel);
}

/* FMOD_Stop stops all playback, but doesn't kill FMOD. It is called by
   FMOD_Shutdown and use_target_changelevel. */

void FMOD_Stop()
{
	if(hFMOD)
	{
		edict_t	*e;
		int		i;

		for(i=game.maxclients+1; i<globals.num_edicts; i++)
		{
			// Stop all playback. Make DAMN SURE "stream" is never
			// used by any entity other than target_playback, or
			// this will break for sure.
			e = &g_edicts[i];
			if(!e->inuse)
				continue;
			if(!e->stream)
				continue;
			if(e->spawnflags & SF_PLAYBACK_MUSICFILE)
			{
				if(FMUSIC_IsPlaying((FMUSIC_MODULE *)e->stream))
					FMUSIC_StopSong ((FMUSIC_MODULE *)e->stream);
				FMUSIC_FreeSong ((FMUSIC_MODULE *)e->stream);
			}
			else if(e->spawnflags & SF_PLAYBACK_SAMPLE)
			{
				FSOUND_StopSound(e->channel);
				FSOUND_Sample_Free((FSOUND_SAMPLE *)e->stream);
			}
			else
				FSOUND_Stream_Close((FSOUND_STREAM *)e->stream);
			e->stream = NULL;
		}
	}

}

/* Music files don't have a "duration", or at least not one that you can have
   reported by FMOD. So if you want to ramp them down, loop them, or do other 
   effects at the end of a song you'll have to have the music-playing 
   target_playback "think" with CheckEndMusic. */

void CheckEndMusic( edict_t *ent )
{
	if(!ent->inuse)
		return;
	if(!ent->stream)
		return;
	if(!(ent->spawnflags & SF_PLAYBACK_MUSICFILE))
		return;

	if(FMUSIC_IsPlaying((FMUSIC_MODULE *)ent->stream))
		ent->nextthink = level.time + 1.0;
	else
	{
		if(!(ent->spawnflags & SF_PLAYBACK_TURNED_OFF))
		{
			if(ent->spawnflags & SF_PLAYBACK_LOOP)
			{
				// We didn't turn it off, so crank it back up
				FMOD_PlaySound(ent);
				return;
			}
			if(ent->target)
			{
				char	*save_message;
				save_message = ent->message;
				ent->message = NULL;
				G_UseTargets(ent, ent->activator);
				ent->message = save_message;
			}
		}
		FMUSIC_FreeSong ((FMUSIC_MODULE *)ent->stream);
		ent->stream = NULL;
		if(!ent->count)
			G_FreeEdict(ent);
	}
}

void CheckEndSample(edict_t *ent)
{
	if(!ent->inuse)
		return;
	if(!ent->stream)
		return;
	if(FSOUND_IsPlaying(ent->channel))
		ent->nextthink = level.time + 1.0;
	else
	{
		if(!(ent->spawnflags & SF_PLAYBACK_TURNED_OFF))
		{
			if(ent->target)
			{
				char	*save_message;
				save_message = ent->message;
				ent->message = NULL;
				G_UseTargets(ent, ent->activator);
				ent->message = save_message;
			}
		}
		FSOUND_StopSound(ent->channel);
		FSOUND_Sample_Free((FSOUND_SAMPLE *)ent->stream);
		ent->stream = NULL;
		if(!ent->count)
			G_FreeEdict(ent);
	}
}

static signed char EndStream(FSOUND_STREAM *stream,void *buff,int len,int param)
{
	edict_t	*e;

	e = (edict_t *)(intptr_t)param;
	if(!e)
		return TRUE;
	if(!e->inuse)
		return TRUE;

	if(!(e->spawnflags & SF_PLAYBACK_TURNED_OFF))
	{
		if(e->target)
		{
			char	*save_message;
			save_message = e->message;
			e->message = NULL;
			G_UseTargets(e, e->activator);
			e->message = save_message;
		}
	}
	FSOUND_Stream_Close(stream);
	e->stream = NULL;
	if(!e->count)
		G_FreeEdict(e);

	return TRUE;
}

int FMOD_PlaySound(edict_t *ent)
{
	int	mode;

	if(!hFMOD)
		FMOD_Init();

	if(!hFMOD)
	{
		gi.dprintf("FMOD.DLL not loaded\n");
		return 0;
	}

	if(ent->spawnflags & SF_PLAYBACK_MUSICFILE)
	{
		ent->stream = (int *)FMUSIC_LoadSong(ent->message);
		if(ent->stream)
		{
			FMUSIC_SetMasterVolume((FMUSIC_MODULE *)ent->stream,(int)(ent->volume));
			if( FMUSIC_PlaySong((FMUSIC_MODULE *)ent->stream) )
			{
				ent->think = CheckEndMusic;
				ent->nextthink = level.time + 1.0;
				return 1;
			}
			else
			{
				gi.dprintf("FMUSIC_PlaySong failed on %s\n",ent->message);
				return 0;
			}
		}
		else
		{
			gi.dprintf("FMUSIC_LoadSong failed on %s\n",ent->message);
			return 0;
		}
	}
	else if(ent->spawnflags & SF_PLAYBACK_SAMPLE)
	{
		if(!ent->stream)
		{
			if(ent->spawnflags & SF_PLAYBACK_LOOP)
				mode = FSOUND_LOOP_NORMAL;
			else
				mode = FSOUND_LOOP_OFF;

			if(!(ent->spawnflags & SF_PLAYBACK_3D))
				mode |= FSOUND_2D;

			ent->stream = (int *)FSOUND_Sample_Load(FSOUND_FREE,ent->message, mode, 0);
		}
		if(ent->stream)
		{
			int		volume;

			if(ent->fadein > 0)
				volume = 0;
			else
				volume = (int)(ent->volume);

			if(ent->spawnflags & SF_PLAYBACK_3D)
			{
				vec3_t	pos;

				VectorSet(pos,ent->s.origin[1],ent->s.origin[2],ent->s.origin[0]);
				VectorScale(pos,0.025,pos);
				ent->channel = FSOUND_PlaySound3DAttrib(FSOUND_FREE, (FSOUND_SAMPLE *)ent->stream, -1, volume, -1, pos, NULL );
				FSOUND_Sample_SetMinMaxDistance( (FSOUND_SAMPLE *)ent->stream, ent->moveinfo.distance, 1000000000.0f);
				FSOUND_Reverb_SetMix(ent->channel,1.0f);
			}
			else
			{
				ent->channel = FSOUND_PlaySound3DAttrib(FSOUND_FREE, (FSOUND_SAMPLE *)ent->stream, -1, volume, -1, NULL, NULL );
				FSOUND_Reverb_SetMix(ent->channel,1.0f);
			}

			if(ent->channel >= 0)
			{
				ent->spawnflags &= ~SF_PLAYBACK_TURNED_OFF;
				return 1;
			}
			else
			{
				gi.dprintf("FSOUND_PlaySound3DAttrib failed on %s\n",ent->message);
				return 0;
			}
		}
		else
		{
			gi.dprintf("FSOUND_Sample_Load failed on %s\n",ent->message);
			ent->channel  = -1;
			return 0;
		}
	}
	else
	{
		if(ent->spawnflags & SF_PLAYBACK_LOOP)
			mode = FSOUND_LOOP_NORMAL;
		else
			mode = FSOUND_LOOP_OFF;

		// ??? Don't use FSOUND_HW3D because A3D cards screw up???

		if(!(ent->spawnflags & SF_PLAYBACK_3D))
			mode |= FSOUND_2D;

		ent->stream = (int *)FSOUND_Stream_OpenFile(ent->message, mode, 0);
		if(ent->stream)
		{
			int		volume;

			if(ent->fadein > 0)
				volume = 0;
			else
				volume = (int)(ent->volume);

			if(ent->spawnflags & SF_PLAYBACK_3D)
			{
				vec3_t	pos;

				VectorSet(pos,ent->s.origin[1],ent->s.origin[2],ent->s.origin[0]);
				VectorScale(pos,0.025,pos);
				ent->channel = FSOUND_Stream_Play3DAttrib(FSOUND_FREE, (FSOUND_STREAM *)ent->stream, -1, volume, -1, pos, NULL );
				FSOUND_Reverb_SetMix(ent->channel,1.0f);
			}
			else
			{
				ent->channel = FSOUND_Stream_Play3DAttrib(FSOUND_FREE, (FSOUND_STREAM *)ent->stream, -1, volume, -1, NULL, NULL );
				FSOUND_Reverb_SetMix(ent->channel,1.0f);
			}

			if(ent->channel >= 0)
			{
				ent->spawnflags &= ~SF_PLAYBACK_TURNED_OFF;
				FSOUND_Stream_SetEndCallback((FSOUND_STREAM *)ent->stream,EndStream,(intptr_t)ent);
				return 1;
			}
			else
			{
				gi.dprintf("FSOUND_Stream_Play3DAttrib failed on %s\n",ent->message);
				return 0;
			}
		}
		else
		{
			gi.dprintf("FSOUND_Stream_OpenFile failed on %s\n",ent->message);
			ent->channel  = -1;
			return 0;
		}
	}
}


void FMOD_StopSound (edict_t *ent, qboolean free)
{
	if(!hFMOD)
		return;

	ent->spawnflags |= SF_PLAYBACK_TURNED_OFF;

	if(ent->spawnflags & SF_PLAYBACK_MUSICFILE)
	{
		if(FMUSIC_IsPlaying((FMUSIC_MODULE *)ent->stream))
			FMUSIC_StopSong ((FMUSIC_MODULE *)ent->stream);
		if(free)
		{
			FMUSIC_FreeSong ((FMUSIC_MODULE *)ent->stream);
			ent->stream = NULL;
		}
	}
	else if(ent->spawnflags & SF_PLAYBACK_SAMPLE)
	{
		if(FSOUND_IsPlaying(ent->channel))
			FSOUND_StopSound(ent->channel);
		ent->channel = -1;
		if(free)
		{
			FSOUND_Sample_Free((FSOUND_SAMPLE *)ent->stream);
			ent->stream = NULL;
		}
	}
	else
	{
		if(FSOUND_IsPlaying(ent->channel))
			FSOUND_StopSound(ent->channel);
		ent->channel = -1;
		if(free)
		{
			FSOUND_Stream_Close((FSOUND_STREAM *)ent->stream);
			ent->stream = NULL;
		}
	}
	
	if(!ent->count)
		G_FreeEdict(ent);
}

void target_playback_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if(!hFMOD)
		return;

	// if it's 3D, decrement level 3D count for efficiency
	if(self->spawnflags & SF_PLAYBACK_3D)
		level.num_3D_sounds--;

	self->count = 0;
	FMOD_StopSound(self,true);
}

void target_playback_fadeout (edict_t *ent)
{
	int	volume;

	if(!ent->stream)
		return;

	if(ent->fadeout <= 0) // should never have come here?
		return;

	if(level.time - ent->timestamp < ent->fadeout)
	{
		volume = (int)(ent->density * (1.0 - (level.time-ent->timestamp)/ent->fadeout));
		if(ent->spawnflags & SF_PLAYBACK_MUSICFILE)
			FMUSIC_SetMasterVolume((FMUSIC_MODULE *)ent->stream,volume);
		else
			FSOUND_SetVolume(ent->channel,volume);
		ent->nextthink = level.time + FRAMETIME;
	}
}

void target_playback_fadein (edict_t *ent)
{
	int	volume;

	if(!ent->stream)
		return;

	if(ent->fadein <= 0) // should never have come here?
		return;

	if(level.time > ent->timestamp + ent->fadein)
		return;

	volume = (int)(ent->volume*(level.time - ent->timestamp)/ent->fadein);
	if(ent->spawnflags & SF_PLAYBACK_MUSICFILE)
		FMUSIC_SetMasterVolume((FMUSIC_MODULE *)ent->stream,volume);
	else
	{
		if(!FSOUND_SetVolume(ent->channel,volume))
			gi.dprintf("FSOUND_SetVolume failed, error code=%d, channel=%d\n",FSOUND_GetError(),ent->channel);
	}
	ent->nextthink = level.time + FRAMETIME;
}

qboolean FMOD_IsPlaying(edict_t *ent)
{
	if(ent->spawnflags & SF_PLAYBACK_MUSICFILE)
	{
		if(ent->stream)
			return FMUSIC_IsPlaying((FMUSIC_MODULE *)ent->stream);
		else
			return false;
	}
	else if(ent->channel >= 0)
	{
		return FSOUND_IsPlaying(ent->channel);
	}
	return false;
}

void Use_Target_Playback (edict_t *ent, edict_t *other, edict_t *activator)
{
	vec3_t		pos = {0, 0, 0};
	vec3_t		vel = {0, 0, 0};
	int			volume = 255;

	if(fmod_nomusic->value && (ent->spawnflags & SF_PLAYBACK_MUSIC))
		return;

	if(s_primary->value)
	{
		gi.dprintf("target_playback requires s_primary be set to 0.\n"
			       "At the console type:\n"
				   "s_primary 0;sound_restart\n");
		return;
	}

	if(!ent->count)
	{
		if(ent->stream)
			FMOD_StopSound(ent,true);
		else
			G_FreeEdict(ent);
		return;
	}

	if(FMOD_IsPlaying(ent))
	{
		if(ent->spawnflags & (SF_PLAYBACK_LOOP | SF_PLAYBACK_TOGGLE))
		{
			if(ent->fadeout > 0)
			{
				if(ent->spawnflags & SF_PLAYBACK_MUSICFILE)
					ent->density = FMUSIC_GetMasterVolume((FMUSIC_MODULE *)ent->stream);
				else
					ent->density = FSOUND_GetVolume(ent->channel);
				ent->timestamp = level.time;
				ent->think = target_playback_fadeout;
				ent->think(ent);
			}
			else
			{
				FMOD_StopSound (ent, ((ent->spawnflags & SF_PLAYBACK_SAMPLE) ? false : true ));
				ent->think = NULL;
				ent->nextthink = 0;
			}
			return;
		}
		else
		{
			// Not toggled - just turn it off then restart it below
			FMOD_StopSound (ent, ((ent->spawnflags & SF_PLAYBACK_SAMPLE) ? false : true ));
			ent->think = NULL;
			ent->nextthink = 0;
		}
	}

	if( !FMOD_PlaySound(ent) )
		return;

	ent->count--;
	ent->timestamp = level.time;
	ent->activator = activator;
	if(ent->fadein > 0)
	{
		ent->think     = target_playback_fadein;
		ent->nextthink = level.time + FRAMETIME;
	}
}

void target_playback_delayed_start (edict_t *ent)
{
	if(!g_edicts[1].linkcount || (ent->groundentity_linkcount == g_edicts[1].linkcount))
		ent->nextthink = level.time + FRAMETIME;
	else
		ent->use(ent,world,world);
}

void target_playback_delayed_restart (edict_t *ent)
{
	if(!g_edicts[1].linkcount || (ent->groundentity_linkcount == g_edicts[1].linkcount))
		ent->nextthink = level.time + FRAMETIME;
	else
	{
		ent->think = target_playback_delayed_start;
		ent->nextthink = level.time + 2.0;
	}
}

void SP_target_playback (edict_t *ent)
{
	char	filename[_MAX_PATH];

	if(!hFMOD)
		FMOD_Init();

	if(!hFMOD)
	{
		gi.dprintf("fmod.dll not loaded, cannot use target_playback.\n");
		G_FreeEdict(ent);
		return;
	}
	if(!st.noise)
	{
		gi.dprintf("target_playback with no noise set at %s\n", vtos(ent->s.origin));
		G_FreeEdict(ent);
		return;
	}
	GameDirRelativePath(st.noise,filename);
	ent->message = gi.TagMalloc((int)strlen(filename)+1,TAG_LEVEL);
	strcpy(ent->message,filename);

	if (ent->fadein < 0)
		ent->fadein = 0;
	if (ent->fadeout < 0)
		ent->fadeout = 0;

	strlwr(ent->message);
	if( strstr(ent->message,".mod") ||
		strstr(ent->message,".s3m") ||
		strstr(ent->message,".xm")  ||
		strstr(ent->message,".mid")    )
		ent->spawnflags |= SF_PLAYBACK_MUSICFILE;

	if (!ent->volume)
		ent->volume = 1.0;
	ent->volume *= 255;
	if (!ent->count)
		ent->count = -1;

	if (ent->spawnflags & SF_PLAYBACK_3D)
	{
		if(!strstr(ent->message,".mp3"))
			ent->spawnflags |= SF_PLAYBACK_SAMPLE;
		level.num_3D_sounds++;

		if(st.distance)
			ent->moveinfo.distance = st.distance * 0.025f;
		else
			ent->moveinfo.distance = 5.0f;
	}
	else
		ent->spawnflags &= ~SF_PLAYBACK_SAMPLE;

	ent->use = Use_Target_Playback;
	ent->die = target_playback_die;

	if (ent->spawnflags & SF_PLAYBACK_STARTON)
	{
		ent->think = target_playback_delayed_start;
		ent->nextthink = level.time + 1.0;
	}

	if(ent->movewith)
		ent->movetype = MOVETYPE_PUSH;
	else
		ent->movetype = MOVETYPE_NONE;
	gi.linkentity (ent);

	ent->stream  = NULL;
	ent->channel = -1;
	if(ent->spawnflags & SF_PLAYBACK_SAMPLE)
	{
		// precache sound to prevent stutter when it finally IS played
		int	mode;
		if(ent->spawnflags & SF_PLAYBACK_LOOP)
			mode = FSOUND_LOOP_NORMAL;
		else
			mode = FSOUND_LOOP_OFF;

		if(!(ent->spawnflags & SF_PLAYBACK_3D))
			mode |= FSOUND_2D;

		ent->stream = (int *)FSOUND_Sample_Load(FSOUND_FREE,ent->message, mode, 0);
	}
}

#endif // DISABLE_FMOD
