#include "g_local.h"

game_locals_t	game;
level_locals_t	level;
game_import_t	gi;
game_export_t	globals;
spawn_temp_t	st;

int	sm_meat_index;
int	snd_fry;
int meansOfDeath;

edict_t		*g_edicts;

cvar_t	*deathmatch;
cvar_t	*coop;
cvar_t	*dmflags;
cvar_t	*skill;
cvar_t	*fraglimit;
cvar_t	*timelimit;
cvar_t	*password;
cvar_t	*spectator_password;
cvar_t	*needpass;
cvar_t	*maxclients;
cvar_t	*maxspectators;
cvar_t	*maxentities;
cvar_t	*g_select_empty;
cvar_t	*dedicated;

cvar_t	*filterban;

cvar_t	*sv_maxvelocity;
cvar_t	*sv_gravity;

cvar_t	*sv_rollspeed;
cvar_t	*sv_rollangle;
cvar_t	*gun_x;
cvar_t	*gun_y;
cvar_t	*gun_z;

cvar_t	*run_pitch;
cvar_t	*run_roll;
cvar_t	*bob_up;
cvar_t	*bob_pitch;
cvar_t	*bob_roll;

cvar_t	*sv_cheats;

cvar_t	*flood_msgs;
cvar_t	*flood_persecond;
cvar_t	*flood_waitdelay;

cvar_t	*sv_maplist;

cvar_t	*actorchicken;
cvar_t	*actorjump;
cvar_t	*actorscram;
cvar_t	*alert_sounds;
cvar_t	*allow_download;
cvar_t	*allow_fog;			// Set to 0 for no fog
cvar_t	*bounce_bounce;
cvar_t	*bounce_minv;
cvar_t	*cd_loopcount;
cvar_t	*cl_gun;
cvar_t	*cl_thirdperson; // Knightmare added
cvar_t	*corpse_fade;
cvar_t	*corpse_fadetime;
cvar_t	*crosshair;
cvar_t	*developer;
cvar_t	*fmod_nomusic;
cvar_t	*footstep_sounds;
cvar_t	*fov;
cvar_t	*gl_clear;
cvar_t	*gl_driver;
cvar_t	*gl_driver_fog;
cvar_t	*hand;
cvar_t	*jetpack_weenie;
cvar_t	*joy_pitchsensitivity;
cvar_t	*joy_yawsensitivity;
cvar_t	*jump_kick;
cvar_t	*lazarus_cd_loop;
cvar_t	*lazarus_cl_gun;
cvar_t	*lazarus_crosshair;
cvar_t	*lazarus_gl_clear;
cvar_t	*lazarus_joyp;
cvar_t	*lazarus_joyy;
cvar_t	*lazarus_pitch;
cvar_t	*lazarus_yaw;
cvar_t	*lights;
cvar_t	*lightsmin;
cvar_t	*m_pitch;
cvar_t	*m_yaw;
cvar_t	*monsterjump;
cvar_t	*packet_fmod_playback;
cvar_t	*readout;
cvar_t	*rocket_strafe;
cvar_t	*rotate_distance;
cvar_t	*s_primary;
cvar_t	*shift_distance;
cvar_t	*sv_maxgibs;
cvar_t	*turn_rider;
cvar_t	*vid_ref;
cvar_t	*zoomrate;
cvar_t	*zoomsnap;

cvar_t	*sv_stopspeed;	//PGM	 (this was a define in g_phys.c)
cvar_t	*sv_step_fraction;	// Knightmare- this was a define in p_view.c


void SpawnEntities (char *mapname, char *entities, char *spawnpoint);
void ClientThink (edict_t *ent, usercmd_t *cmd);
qboolean ClientConnect (edict_t *ent, char *userinfo);
void ClientUserinfoChanged (edict_t *ent, char *userinfo);
void ClientDisconnect (edict_t *ent);
void ClientBegin (edict_t *ent);
void ClientCommand (edict_t *ent);
void RunEntity (edict_t *ent);
void WriteGame (char *filename, qboolean autosave);
void ReadGame (char *filename);
void WriteLevel (char *filename);
void ReadLevel (char *filename);
void InitGame (void);
void G_RunFrame (void);

//===================================================================

void ShutdownGame (void)
{
	gi.dprintf ("==== ShutdownGame ====\n");
	if (!deathmatch->value && !coop->value) {
		gi.cvar_forceset("m_pitch", va("%f",lazarus_pitch->value));
		gi.cvar_forceset("cd_loopcount", va("%d",lazarus_cd_loop->value));
		gi.cvar_forceset("gl_clear", va("%d", lazarus_gl_clear->value));
	}
	// Lazarus: Turn off fog if it's on
	if (!dedicated->value)
		Fog_Off();
	// and shut down FMOD
	FMOD_Shutdown();

	gi.FreeTags (TAG_LEVEL);
	gi.FreeTags (TAG_GAME);
}


game_import_t RealFunc;
int	max_modelindex;
int	max_soundindex;


int Debug_Modelindex (char *name)
{
	int	modelnum;
	modelnum = RealFunc.modelindex(name);
	if (modelnum > max_modelindex)
	{
		gi.dprintf("Model %03d %s\n",modelnum,name);
		max_modelindex = modelnum;
	}
	return modelnum;
}

int Debug_Soundindex (char *name)
{
	int soundnum;
	soundnum = RealFunc.soundindex(name);
	if (soundnum > max_soundindex)
	{
		gi.dprintf("Sound %03d %s\n",soundnum,name);
		max_soundindex = soundnum;
	}
	return soundnum;
}

/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
game_export_t *GetGameAPI (game_import_t *import)
{
	gi = *import;

	globals.apiversion = GAME_API_VERSION;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities;

	globals.WriteGame = WriteGame;
	globals.ReadGame = ReadGame;
	globals.WriteLevel = WriteLevel;
	globals.ReadLevel = ReadLevel;

	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;

	globals.RunFrame = G_RunFrame;

	globals.ServerCommand = ServerCommand;

	globals.edict_size = sizeof(edict_t);

	gl_driver = gi.cvar ("gl_driver", "", 0);
	vid_ref = gi.cvar ("vid_ref", "", 0);
	gl_driver_fog = gi.cvar ("gl_driver_fog", "opengl32", CVAR_NOSET | CVAR_ARCHIVE);

	Fog_Init();

	developer = gi.cvar("developer", "0", CVAR_SERVERINFO);
	readout   = gi.cvar("readout", "0", CVAR_SERVERINFO);
	if (readout->value)
	{
		max_modelindex = 0;
		max_soundindex = 0;
		RealFunc.modelindex = gi.modelindex;
		gi.modelindex       = Debug_Modelindex;
		RealFunc.soundindex = gi.soundindex;
		gi.soundindex       = Debug_Soundindex;
	}

	return &globals;
}

#ifndef GAME_HARD_LINKED
// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	gi.error (ERR_FATAL, "%s", text);
}

void Com_Printf (char *msg, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	gi.dprintf ("%s", text);
}

#endif

/*
=================
ClientEndServerFrames
=================
*/
void ClientEndServerFrames (void)
{
	int		i;
	edict_t	*ent;

	// calc the player views now that all pushing
	// and damage has been added
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		ClientEndServerFrame (ent);
	}

	//reflection stuff -- modified from psychospaz' original code
	if (level.num_reflectors)
	{
		ent = &g_edicts[0];
		for (i=0 ; i<globals.num_edicts ; i++, ent++) //pointers, not as slow as you think
		{
			if (!ent->inuse)
				continue;
			if (!ent->s.modelindex)
				continue;
//			if (ent->s.effects & EF_ROTATE)
//				continue;
			if (ent->flags & FL_REFLECT)
				continue;
			if (!ent->client && (ent->svflags & SVF_NOCLIENT))
				continue;
			if (ent->client && !ent->client->chasetoggle && (ent->svflags & SVF_NOCLIENT))
				continue;
			if (ent->svflags&SVF_MONSTER && ent->solid!=SOLID_BBOX)
				continue;
			if ( (ent->solid == SOLID_BSP) && (ent->movetype != MOVETYPE_PUSHABLE))
				continue;
			if (ent->client && (ent->client->resp.spectator || ent->health<=0 || ent->deadflag == DEAD_DEAD))
				continue;		
			AddReflection(ent);	
		}
	}
}

/*
=================
CreateTargetChangeLevel

Returns the created target changelevel
=================
*/
edict_t *CreateTargetChangeLevel(char *map)
{
	edict_t *ent;

	ent = G_Spawn ();
	ent->classname = "target_changelevel";
	Com_sprintf(level.nextmap, sizeof(level.nextmap), "%s", map);
	ent->map = level.nextmap;
	return ent;
}

/*
=================
EndDMLevel

The timelimit or fraglimit has been exceeded
=================
*/
void EndDMLevel (void)
{
	edict_t		*ent;
	char *s, *t, *f;
	static const char *seps = " ,\n\r";

	// stay on same level flag
	if ((int)dmflags->value & DF_SAME_LEVEL)
	{
		BeginIntermission (CreateTargetChangeLevel (level.mapname) );
		return;
	}

	// see if it's in the map list
	if (*sv_maplist->string) {
		s = strdup(sv_maplist->string);
		f = NULL;
		t = strtok(s, seps);
		while (t != NULL) {
			if (Q_stricmp(t, level.mapname) == 0) {
				// it's in the list, go to the next one
				t = strtok(NULL, seps);
				if (t == NULL) { // end of list, go to first one
					if (f == NULL) // there isn't a first one, same level
						BeginIntermission (CreateTargetChangeLevel (level.mapname) );
					else
						BeginIntermission (CreateTargetChangeLevel (f) );
				} else
					BeginIntermission (CreateTargetChangeLevel (t) );
				free(s);
				return;
			}
			if (!f)
				f = t;
			t = strtok(NULL, seps);
		}
		free(s);
	}

	if (level.nextmap[0]) // go to a specific map
		BeginIntermission (CreateTargetChangeLevel (level.nextmap) );
	else {	// search for a changelevel
		ent = G_Find (NULL, FOFS(classname), "target_changelevel");
		if (!ent)
		{	// the map designer didn't include a changelevel,
			// so create a fake ent that goes back to the same level
			BeginIntermission (CreateTargetChangeLevel (level.mapname) );
			return;
		}
		BeginIntermission (ent);
	}
}


/*
=================
CheckNeedPass
=================
*/
void CheckNeedPass (void)
{
	int need;

	// if password or spectator_password has changed, update needpass
	// as needed
	if (password->modified || spectator_password->modified) 
	{
		password->modified = spectator_password->modified = false;

		need = 0;

		if (*password->string && Q_stricmp(password->string, "none"))
			need |= 1;
		if (*spectator_password->string && Q_stricmp(spectator_password->string, "none"))
			need |= 2;

		gi.cvar_set("needpass", va("%d", need));
	}
}

/*
=================
CheckDMRules
=================
*/
void CheckDMRules (void)
{
	int			i;
	gclient_t	*cl;

	if (level.intermissiontime)
		return;

	if (!deathmatch->value)
		return;

	if (timelimit->value)
	{
		if (level.time >= timelimit->value*60)
		{
			gi.bprintf (PRINT_HIGH, "Timelimit hit.\n");
			EndDMLevel ();
			return;
		}
	}

	if (fraglimit->value)
	{
		for (i=0 ; i<maxclients->value ; i++)
		{
			cl = game.clients + i;
			if (!g_edicts[i+1].inuse)
				continue;

			if (cl->resp.score >= fraglimit->value)
			{
				gi.bprintf (PRINT_HIGH, "Fraglimit hit.\n");
				EndDMLevel ();
				return;
			}
		}
	}
}


/*
=============
ExitLevel
=============
*/
void ExitLevel (void)
{
	int		i;
	edict_t	*ent;
	char	command [256];

	Com_sprintf (command, sizeof(command), "gamemap \"%s\"\n", level.changemap);
	gi.AddCommandString (command);
	level.changemap = NULL;
	level.exitintermission = 0;
	level.intermissiontime = 0;
	ClientEndServerFrames ();

	// clear some things before going to next level
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
//		if (ent->health > ent->client->pers.max_health) //CW: allow MH benefit to continue over level change
//			ent->health = ent->client->pers.max_health;
	}

}

/*
================
G_RunFrame

Advances the world by 0.1 seconds
================
*/
void G_RunFrame (void)
{
	int		i;
	edict_t	*ent;

	if (level.freeze)
	{
		level.freezeframes++;
		if (level.freezeframes >= sk_stasis_time->value*10)
			level.freeze = false;
	} else
		level.framenum++;

	level.time = level.framenum * FRAMETIME;

	// choose a client for monsters to target this frame
	AI_SetSightClient ();

	// exit intermissions

	if (level.exitintermission)
	{
		ExitLevel ();
		return;
	}

	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	ent = &g_edicts[0];
	for (i=0 ; i<globals.num_edicts ; i++, ent++)
	{
		if (!ent->inuse)
			continue;

		level.current_entity = ent;

		VectorCopy (ent->s.origin, ent->s.old_origin);

		// if the ground entity moved, make sure we are still on it
		if ((ent->groundentity) && (ent->groundentity->linkcount != ent->groundentity_linkcount))
		{
			ent->groundentity = NULL;
			if ( !(ent->flags & (FL_SWIM|FL_FLY)) && (ent->svflags & SVF_MONSTER) )
			{
				M_CheckGround (ent);
			}
		}

		if (i > 0 && i <= maxclients->value)
		{
			ClientBeginServerFrame (ent);
			continue;
		}

		G_RunEntity (ent);
	}

	// FMOD stuff:
	if ( (level.num_3D_sounds > 0) && (game.maxclients == 1))
		FMOD_UpdateListenerPos();

	// see if it is time to end a deathmatch
	CheckDMRules ();

	// see if needpass needs updated
	CheckNeedPass ();

	// build the playerstate_t structures for all players
	ClientEndServerFrames ();

}

