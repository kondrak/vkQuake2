#include "g_local.h"

#define IF_VISIBLE	8
#define SEEK_PLAYER	128
#define ANIM_MASK	(EF_ANIM01|EF_ANIM23|EF_ANIM_ALL|EF_ANIM_ALLFAST)		//CW

static float fCrosshair;	//CW: store crosshair value for [target_monitor] use
extern char *single_statusbar;		//CW: reference external definition in g_spawn.c

///CW++ Holographic "repair catalogue" of monsters.
#define	HOLO_SIZE	12

char *holo_list[HOLO_SIZE] =   {"models/monsters/berserk/tris.md2",
								"models/monsters/brain/tris.md2",
								"models/monsters/flipper/tris.md2",
								"models/monsters/float/tris.md2",
								"models/monsters/flyer/tris.md2",
								"models/monsters/gladiatr/tris.md2",
								"models/monsters/gunner/tris.md2",
								"models/monsters/hover/tris.md2",
								"models/monsters/infantry/tris.md2",
								"models/monsters/parasite/tris.md2",
								"models/monsters/soldier/tris.md2",
								"players/walker/tris.md2"};
//CW--

/*QUAKED target_temp_entity (1 0 0) (-8 -8 -8) (8 8 8)
Fire an origin based temp entity event to the clients.
"style"		type byte
*/
void Use_Target_Tent (edict_t *self, edict_t *other, edict_t *activator)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (self->style);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	self->count--;
	if (!self->count)
	{
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}

}

void SP_target_temp_entity (edict_t *ent)
{
	ent->use = Use_Target_Tent;
}


//==========================================================

//==========================================================

/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) looped-on looped-off reliable changelevel
"noise"		wav file to play
"attenuation"
DWH
-2 = only played (full volume) for player who triggered the target_speaker
end DWH

-1 = none, send to whole level
 1 = normal fighting sounds
 2 = idle sound level
 3 = ambient sound level
"volume"	0.0 to 1.0

Normal sounds play each time the target is used.  The reliable flag can be set for crucial voiceovers.

Looped sounds are always atten 3 / vol 1, and the use function toggles it on/off.
Multiple identical looping sounds will just increase volume without any speed cost.

Changelevel spawnflag added for Lazarus. This should ONLY be applied in the code,
and is an indication that the "message" key contains the noise.
*/
void Use_Target_Speaker (edict_t *ent, edict_t *other, edict_t *activator)
{
	int		chan;

	if (ent->spawnflags & 3)
	{	// looping sound toggles
		if (ent->s.sound) {
			ent->s.sound = 0;	// turn it off
			ent->nextthink = 0;
		}
		else
			ent->s.sound = ent->noise_index;	// start it
#ifdef LOOP_SOUND_ATTENUATION
			ent->s.attenuation = ent->attenuation;
#endif
	}
	else
	{
		if (ent->attenuation == -2) {
			if (ent->spawnflags & 4)
				chan = CHAN_VOICE|CHAN_RELIABLE;
			else
				chan = CHAN_VOICE;
			gi.sound (activator, chan, ent->noise_index, 1, ATTN_NORM, 0);
		}
		else
		{	// normal sound
			if (ent->spawnflags & 4)
				chan = CHAN_VOICE|CHAN_RELIABLE;
			else
				chan = CHAN_VOICE;
			// use a positioned_sound, because this entity won't normally be
			// sent to any clients because it is invisible
			gi.positioned_sound (ent->s.origin, ent, chan, ent->noise_index, ent->volume, ent->attenuation, 0);
		}

		ent->count--;
		if (!ent->count) {
			ent->think = G_FreeEdict;
			ent->nextthink = level.time + 1;
		}
	}
}

void SP_target_speaker (edict_t *ent)
{
	if (!(ent->spawnflags & 8))
	{
		if (!st.noise)
		{
			gi.dprintf("target_speaker with no noise set at %s\n", vtos(ent->s.origin));
			G_FreeEdict(ent);
			return;
		}
		// DWH: Use "message" key to store noise for speakers that change levels
		//      via trigger_transition
		if (!strstr (st.noise, ".wav"))
		{
			ent->message = gi.TagMalloc((int)strlen(st.noise)+5,TAG_LEVEL);
			sprintf(ent->message,"%s.wav", st.noise);
		}
		else
		{
			ent->message = gi.TagMalloc((int)strlen(st.noise)+1,TAG_LEVEL);
			strcpy(ent->message,st.noise);
		}
	}
	ent->noise_index = gi.soundindex (ent->message);
	ent->spawnflags &= ~8;

	if (!ent->volume)
		ent->volume = 1.0;

	if (!ent->attenuation)
		ent->attenuation = 1.0;
	else if (ent->attenuation == -1)	// use -1 so 0 defaults to 1
		ent->attenuation = 0;

	// check for prestarted looping sound
	if (ent->spawnflags & 1) {
		ent->s.sound = ent->noise_index;
#ifdef LOOP_SOUND_ATTENUATION
		ent->s.attenuation = ent->attenuation;
#endif
	}

	ent->use = Use_Target_Speaker;

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	gi.linkentity (ent);
}


//==========================================================

void Use_Target_Help (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->message)
	{
		if (self->spawnflags & 1)
			strncpy (game.helpmessage1, self->message, sizeof(game.helpmessage2)-1);
		else
			strncpy (game.helpmessage2, self->message, sizeof(game.helpmessage1)-1);
	}

	game.helpchanged++;

	self->count--;
	if (!self->count) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}
}

/*QUAKED target_help (1 0 1) (-16 -16 -24) (16 16 24) help1
When fired, the "message" key becomes the current personal computer string, and the message light will be set on all clients status bars.
*/
void SP_target_help(edict_t *ent)
{
	if (deathmatch->value)
	{	// auto-remove for deathmatch
		G_FreeEdict (ent);
		return;
	}

	// Lazarus: we allow blank message if world->effects is "help=pic only"
	if (!ent->message && !(world->effects & FX_WORLDSPAWN_NOHELP))
	{
		gi.dprintf ("%s with no message at %s\n", ent->classname, vtos(ent->s.origin));
		G_FreeEdict (ent);
		return;
	}
	ent->use = Use_Target_Help;
}

//==========================================================

/*QUAKED target_secret (1 0 1) (-8 -8 -8) (8 8 8)
Counts a secret found.
These are single use targets.

Lazarus:
DISABLED SF=1

*/
void use_target_secret (edict_t *ent, edict_t *other, edict_t *activator)
{
	if (ent->spawnflags & 1)
	{
		ent->spawnflags &= ~1;
		level.total_secrets++;
		return;
	}
	gi.sound (ent, CHAN_VOICE, ent->noise_index, 1, ATTN_NORM, 0);

	level.found_secrets++;

	G_UseTargets (ent, activator);
	G_FreeEdict (ent);
}

void SP_target_secret (edict_t *ent)
{
	if (deathmatch->value)
	{	// auto-remove for deathmatch
		G_FreeEdict (ent);
		return;
	}

	ent->use = use_target_secret;
	if (!st.noise)
		st.noise = "misc/secret.wav";
	ent->noise_index = gi.soundindex (st.noise);
	ent->svflags = SVF_NOCLIENT;

	if (!(ent->spawnflags & 1))
		level.total_secrets++;

	// map bug hack
	if (!Q_stricmp(level.mapname, "mine3") && ent->s.origin[0] == 280 && ent->s.origin[1] == -2048 && ent->s.origin[2] == -624)
		ent->message = "You have found a secret area.";
}

//==========================================================

/*QUAKED target_goal (1 0 1) (-8 -8 -8) (8 8 8)
Counts a goal completed.
These are single use targets.

Lazarus:
DISABLED SF=1

*/
void use_target_goal (edict_t *ent, edict_t *other, edict_t *activator)
{
	if (ent->spawnflags & 1)
	{
		ent->spawnflags &= ~1;
		level.total_goals++;
		return;
	}
	gi.sound (ent, CHAN_VOICE, ent->noise_index, 1, ATTN_NORM, 0);

	level.found_goals++;

	if (level.found_goals == level.total_goals)
		gi.configstring (CS_CDTRACK, "0");

	G_UseTargets (ent, activator);
	G_FreeEdict (ent);
}

void SP_target_goal (edict_t *ent)
{
	if (deathmatch->value)
	{	// auto-remove for deathmatch
		G_FreeEdict (ent);
		return;
	}

	ent->use = use_target_goal;
	if (!st.noise)
		st.noise = "misc/secret.wav";
	ent->noise_index = gi.soundindex (st.noise);
	ent->svflags = SVF_NOCLIENT;

	if (!(ent->spawnflags & 1))
		level.total_goals++;
}

//==========================================================


/*QUAKED target_explosion (1 0 0) (-8 -8 -8) (8 8 8)
Spawns an explosion temporary entity when used.

"delay"		wait this long before going off
"dmg"		how much radius damage should be done, defaults to 0
*/
void target_explosion_explode (edict_t *self)
{
	float		save;

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PHS);

	if (level.num_reflectors)
		ReflectExplosion (TE_EXPLOSION1,self->s.origin);

	T_RadiusDamage (self, self->activator, self->dmg, NULL, self->dmg+40, MOD_EXPLOSIVE, -0.5);

	save = self->delay;
	self->delay = 0;
	G_UseTargets (self, self->activator);
	self->delay = save;

	self->count--;
	if (!self->count) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}
}

void use_target_explosion (edict_t *self, edict_t *other, edict_t *activator)
{
	self->activator = activator;

	if (!self->delay)
	{
		target_explosion_explode (self);
		return;
	}

	self->think = target_explosion_explode;
	self->nextthink = level.time + self->delay;
}

void SP_target_explosion (edict_t *ent)
{
	ent->use = use_target_explosion;
	ent->svflags = SVF_NOCLIENT;
}


//==========================================================

/*QUAKED target_changelevel (1 0 0) (-8 -8 -8) (8 8 8)
Changes level to "map" when fired

Lazarus spawnflags:
 1 CLEAR_INVENTORY: Removes all pickups other than weapons, restore health to 100
 2 LANDMARK:        If set, player position when spawning in the next map will be at the 
                    same offset from the info_player_start as his current position relative
                    to the target_changelevel. Velocity, angles, and crouch state will be
                    preserved across maps.
 4 NO_GUN           Sets cl_gun 0 and crosshair 0 for the next map/demo only
 8 EASY             Sets skill 0 for next map
16 NORMAL           Sets skill 1 for next map
32 HARD             Sets skill 2 for next map
64 NIGHTMARE        Sets skill 3 for next map
*/
void use_target_changelevel (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t	*transition;
	extern	int	nostatus;

	if (level.intermissiontime)
		return;		// already activated

	if (!deathmatch->value && !coop->value)
	{
		if (g_edicts[1].health <= 0)
			return;
	}

	// if noexit, do a ton of damage to other
	if (deathmatch->value && !( (int)dmflags->value & DF_ALLOW_EXIT) && other != world)
	{
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, 10 * other->max_health, 1000, 0, MOD_EXIT);
		return;
	}

	FMOD_Stop();

	// if multiplayer, let everyone know who hit the exit
	if (deathmatch->value)
	{
		if (activator && activator->client)
			gi.bprintf (PRINT_HIGH, "%s exited the level.\n", activator->client->pers.netname);
	}

	if (activator->client)
	{
		if (activator->client->chasetoggle)
		{
			ChasecamRemove (activator, OPTION_OFF);
			activator->client->pers.chasetoggle = 1;
		}
		else
			activator->client->pers.chasetoggle = 0;

		if (!activator->vehicle)
			activator->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
	}

	// if going to a new unit, clear cross triggers
	if (strstr(self->map, "*"))
	{
		game.serverflags &= ~(SFL_CROSS_TRIGGER_MASK);
		game.lock_code[0] = 0;
		game.lock_revealed = 0;
		game.lock_hud = 0;
		game.transition_ents = 0;

		if (activator->client)
		{
			activator->client->pers.spawn_landmark = false;
			activator->client->pers.spawn_levelchange = false;
		}
	}
	else
	{
		if (self->spawnflags & 2 && activator->client)
		{
			activator->client->pers.spawn_landmark = true;
			VectorSubtract(activator->s.origin,self->s.origin,
				activator->client->pers.spawn_offset);
			VectorCopy(activator->velocity,activator->client->pers.spawn_velocity);
			VectorCopy(activator->s.angles,activator->client->pers.spawn_angles);
			activator->client->pers.spawn_angles[ROLL] = 0;
			VectorCopy(activator->client->ps.viewangles,activator->client->pers.spawn_viewangles);
			activator->client->pers.spawn_pm_flags = activator->client->ps.pmove.pm_flags;
			if (self->s.angles[YAW])
			{
				vec3_t	angles;
				vec3_t	forward, right, v;

				angles[PITCH] = angles[ROLL] = 0.;
				angles[YAW] = self->s.angles[YAW];
				AngleVectors(angles,forward,right,NULL);
				VectorNegate(right,right);
				VectorCopy(activator->client->pers.spawn_offset,v);
				G_ProjectSource (vec3_origin,
					             v, forward, right,
								 activator->client->pers.spawn_offset);
				VectorCopy(activator->client->pers.spawn_velocity,v);
				G_ProjectSource (vec3_origin,
					             v, forward, right,
								 activator->client->pers.spawn_velocity);
				activator->client->pers.spawn_angles[YAW] += angles[YAW];
				activator->client->pers.spawn_viewangles[YAW] += angles[YAW];
			}
		}		
		else
		{
			activator->client->pers.spawn_landmark = false;
		}

		if ((self->spawnflags & 4) && activator->client && !deathmatch->value && !coop->value)
		{
			nostatus = 1;
			stuffcmd(activator,"cl_gun 0;crosshair 0\n");
			activator->client->pers.hand = 2;
		}

		activator->client->pers.spawn_levelchange = true;
		activator->client->pers.spawn_gunframe    = activator->client->ps.gunframe;
		activator->client->pers.spawn_modelframe  = activator->s.frame;
		activator->client->pers.spawn_anim_end    = activator->client->anim_end;
	}

	if (level.next_skill > 0)
	{
		gi.cvar_forceset("skill", va("%d",level.next_skill-1));
		level.next_skill = 0;	// reset
	}
	else if (self->spawnflags & 8)
		gi.cvar_forceset("skill", "0");
	else if (self->spawnflags & 16)
		gi.cvar_forceset("skill", "1");
	else if (self->spawnflags & 32)
		gi.cvar_forceset("skill", "2");
	else if (self->spawnflags & 64)
		gi.cvar_forceset("skill", "3");

	if (self->spawnflags & 1)
	{
		int n;
		if (activator && activator->client)
		{
			for (n = 0; n < MAX_ITEMS; n++)
			{
				// Keep blaster
				if (!(itemlist[n].flags & IT_WEAPON) || itemlist[n].weapmodel != WEAP_BLASTER )
					activator->client->pers.inventory[n] = 0;
			}
			// Switch to blaster
			if ( activator->client->pers.inventory[ITEM_INDEX(FindItem("blaster"))] )
				activator->client->newweapon = FindItem ("blaster");
			else
				activator->client->newweapon = FindItem ("No Weapon");
			ChangeWeapon(activator);
			activator->client->pers.health = activator->health = 100;
		}
	}
	game.transition_ents = 0;
	if (self->spawnflags & 2 && activator->client)
	{
		transition = G_Find(NULL,FOFS(classname),"trigger_transition");
		while(transition)
		{
			if (!Q_stricmp(transition->targetname,self->targetname))
			{
				game.transition_ents = trigger_transition_ents(self,transition);
				break;
			}
			transition = G_Find(transition,FOFS(classname),"trigger_transition");
		}
	}
	BeginIntermission (self);
}

void SP_target_changelevel (edict_t *ent)
{
	if (!ent->map)
	{
		gi.dprintf("target_changelevel with no map at %s\n", vtos(ent->s.origin));
		G_FreeEdict (ent);
		return;
	}

	if ((deathmatch->value || coop->value) && (ent->spawnflags & 2))
	{
		gi.dprintf("target_changelevel at %s\nLANDMARK only valid in single-player\n",
			vtos(ent->s.origin));
		ent->spawnflags &= ~2;
	}
	// ugly hack because *SOMEBODY* screwed up their map
	if ((Q_stricmp(level.mapname, "fact1") == 0) && (Q_stricmp(ent->map, "fact3") == 0))
	   ent->map = "fact3$secret1";

	ent->use = use_target_changelevel;
	ent->svflags = SVF_NOCLIENT;
}


//==========================================================

/*QUAKED target_splash (1 0 0) (-8 -8 -8) (8 8 8)
Creates a particle splash effect when used.

Set "sounds" to one of the following:
  1) sparks
  2) blue water
  3) brown water
  4) slime
  5) lava
  6) blood

"count"	how many pixels in the splash
"dmg"	if set, does a radius damage at this location when it splashes
		useful for lava/sparks
*/


void use_target_splash (edict_t *self, edict_t *other, edict_t *activator)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_SPLASH);
	gi.WriteByte (self->count);
	gi.WritePosition (self->s.origin);
	gi.WriteDir (self->movedir);
	gi.WriteByte (self->sounds);
	gi.multicast (self->s.origin, MULTICAST_PVS);
	if (self->dmg)
		T_RadiusDamage (self, activator, self->dmg, NULL, self->dmg+40, MOD_SPLASH, -0.5);
}

void SP_target_splash (edict_t *self)
{
	self->use = use_target_splash;
	G_SetMovedir (self->s.angles, self->movedir);
	if (!self->count)
		self->count = 32;
	self->svflags = SVF_NOCLIENT;
}

//==========================================================

/*QUAKED target_spawner (1 0 0) (-8 -8 -8) (8 8 8)
Set target to the type of entity you want spawned.
Useful for spawning monsters and gibs in the factory levels.

For monsters:
	Set direction to the facing you want it to have.

For gibs:
	Set direction if you want it moving and
	speed how fast it should be moving otherwise it
	will just be dropped
*/
void ED_CallSpawn (edict_t *ent);

void use_target_spawner (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t	*ent;

	ent = G_Spawn();
	ent->classname = self->target;
	VectorCopy (self->s.origin, ent->s.origin);
	VectorCopy (self->s.angles, ent->s.angles);
	ED_CallSpawn (ent);
	gi.unlinkentity (ent);
	KillBox (ent);
	gi.linkentity (ent);
	if (self->speed)
		VectorCopy (self->movedir, ent->velocity);

	self->count--;
	if (!self->count)
	{
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}

}


void SP_target_spawner (edict_t *self)
{
	self->use = use_target_spawner;
	self->svflags = SVF_NOCLIENT;
	if (self->speed)
	{
		G_SetMovedir (self->s.angles, self->movedir);
		VectorScale (self->movedir, self->speed, self->movedir);
	}
}

//==========================================================

/*QUAKED target_blaster (1 0 0) (-8 -8 -8) (8 8 8) NOTRAIL NOEFFECTS
Fires a blaster bolt in the set direction when triggered.

dmg		default is 15
speed	default is 1000
*/

/* Lazarus:
sounds - weapon choice
0 = blaster
1 = railgun
2 = rocket
3 = bfg
4 = homing rocket
5 = machinegun
6 = grenade
*/

void use_target_blaster (edict_t *self, edict_t *other, edict_t *activator)
{
	vec3_t	movedir, start, target;
	int effect;

	VectorCopy(self->s.origin,start);
	if (self->enemy)
	{
		if (self->sounds == 6)
		{
			if (!AimGrenade (self, start, self->enemy->s.origin, self->speed, movedir))
				return;
		}
		else
		{
			VectorMA(self->enemy->absmin,0.5,self->enemy->size,target);
			VectorSubtract(target,start,movedir);
			VectorNormalize(movedir);
		}
	} else
		VectorCopy(self->movedir,movedir);

	if (self->spawnflags & 2)
		effect = 0;
	else if (self->spawnflags & 1)
		effect = EF_HYPERBLASTER;
	else
		effect = EF_BLASTER;

	// Lazarus: weapon choices
	if (self->sounds == 1)
	{
		fire_rail (self, start, movedir, self->dmg, 0);
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (self-g_edicts);
		gi.WriteByte (MZ_RAILGUN);
		gi.multicast (start, MULTICAST_PVS);
	}
	else if (self->sounds == 2)
	{
		fire_rocket(self, start, movedir, self->dmg, self->speed, self->dmg, self->dmg, NULL);
		gi.positioned_sound (start, self, CHAN_WEAPON, gi.soundindex("weapons/rocklf1a.wav"), 1, ATTN_NORM, 0);
	}
	else if (self->sounds == 3)
	{
		fire_bfg(self, start, movedir, self->dmg, self->speed, self->dmg);
		gi.positioned_sound (start, self, CHAN_WEAPON, gi.soundindex("weapons/laser2.wav"), 1, ATTN_NORM, 0);
	}
	else if (self->sounds == 4)
	{
		fire_rocket(self, start, movedir, self->dmg, self->speed, self->dmg, self->dmg, self->enemy);
		gi.positioned_sound (start, self, CHAN_WEAPON, gi.soundindex("weapons/rocklf1a.wav"), 1, ATTN_NORM, 0);
	}
	else if (self->sounds == 5)
	{
		fire_bullet(self, start, movedir, self->dmg, 2, 0, 0, MOD_TARGET_BLASTER);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_CHAINFIST_SMOKE);
		gi.WritePosition(start);
		gi.multicast(start, MULTICAST_PVS);
		gi.positioned_sound(start,self,CHAN_WEAPON,gi.soundindex(va("weapons/machgf%db.wav",rand() % 5 + 1)),1,ATTN_NORM,0);
	}
	else if (self->sounds == 6)
	{
		fire_grenade(self, start, movedir, self->dmg, self->speed, 2.5, self->dmg+40, false);
		gi.WriteByte (svc_muzzleflash2);
		gi.WriteShort (self - g_edicts);
		gi.WriteByte (MZ2_GUNNER_GRENADE_1);
		gi.multicast (start, MULTICAST_PVS);
	}
	else {
		fire_blaster (self, start, movedir, self->dmg, self->speed, effect, MOD_TARGET_BLASTER, BLASTER_ORANGE);
		gi.sound (self, CHAN_VOICE, self->noise_index, 1, ATTN_NORM, 0);
	}
}

void target_blaster_think (edict_t *self)
{
	edict_t *ent;
	edict_t	*player;
	trace_t	tr;
	vec3_t	target;
	int		i;

	if (self->spawnflags & SEEK_PLAYER) {
		// this takes precedence over everything else

		// If we are currently targeting a non-player, reset and look for
		// a player
		if (self->enemy && !self->enemy->client)
			self->enemy = NULL;

		// Is currently targeted player alive and not using notarget?
		if (self->enemy) {
			if (self->enemy->flags & FL_NOTARGET)
				self->enemy = NULL;
			else if (!self->enemy->inuse || self->enemy->health < 0)
				self->enemy = NULL;
		}

		// We have a live not-notarget player as target. If IF_VISIBLE is 
		// set, see if we can see him
		if (self->enemy && (self->spawnflags & IF_VISIBLE) ) {
			VectorMA(self->enemy->absmin,0.5,self->enemy->size,target);
			tr = gi.trace(self->s.origin,vec3_origin,vec3_origin,target,self,MASK_OPAQUE);
			if (tr.fraction != 1.0)
				self->enemy = NULL;
		}

		// If we STILL have an enemy, then he must be a good player target. Frag him
		if (self->enemy) {
			use_target_blaster(self,self,self);
			if (self->wait)
				self->nextthink = level.time + self->wait;
			return;
		}
	
		// Find a player - note that we search the entire entity list so we'll
		// also hit on func_monitor-viewing fake players
		for(i=1, player=g_edicts+1; i<globals.num_edicts && !self->enemy; i++, player++) {
			if (!player->inuse) continue;
			if (!player->client) continue;
			if (player->svflags & SVF_NOCLIENT) continue;
			if (player->health >= 0 && !(player->flags & FL_NOTARGET) ) {
				if (self->spawnflags & IF_VISIBLE) {
					// player must be seen to shoot
					VectorMA(player->s.origin,0.5,player->size,target);
					tr = gi.trace(self->s.origin,vec3_origin,vec3_origin,target,self,MASK_OPAQUE);
					if (tr.fraction == 1.0)
						self->enemy = player;
				} else {
					// we don't care whether he can be seen
					self->enemy = player;
				}
			}
		}
		// If we have an enemy, shoot
		if (self->enemy) {
			use_target_blaster(self,self,self);
			if (self->wait)
				self->nextthink = level.time + self->wait;
			return;
		}
	}

	// If we get to this point, then either SEEK_PLAYER wasn't set or we couldn't find
	// a live, notarget player.

	if (self->target) {
		if (!(self->spawnflags & IF_VISIBLE)) {
			// have a target, don't care whether it's visible; cannot be a gibbed monster
			self->enemy = NULL;
			ent = G_Find (NULL, FOFS(targetname), self->target);
			while(ent && !self->enemy) {
				// if target is not a monster, we're done
				if ( !(ent->svflags & SVF_MONSTER)) {
					self->enemy = ent;
					break;
				}
				ent = G_Find(ent, FOFS(targetname), self->target);
			}
		} else {
			// has a target, but must be visible and not a monster
			self->enemy = NULL;
			ent = G_Find (NULL, FOFS(targetname), self->target);
			while(ent && !self->enemy) {
				// if the target isn't a monster, we don't care whether 
				// it can be seen or not.
				if ( !(ent->svflags & SVF_MONSTER) ) {
					self->enemy = ent;
					break;
				}
				if ( ent->health > ent->gib_health)
				{
					// Not a gibbed monster
					VectorMA(ent->absmin,0.5,ent->size,target);
					tr = gi.trace(self->s.origin,vec3_origin,vec3_origin,target,self,MASK_OPAQUE);
					if (tr.fraction == 1.0)
					{
						self->enemy = ent;
						break;
					}
				}
				ent = G_Find(ent, FOFS(targetname), self->target);
			}
		}
	}
	if (self->enemy || !(self->spawnflags & IF_VISIBLE) ) {
		use_target_blaster(self,self,self);
		if (self->wait)
			self->nextthink = level.time + self->wait;
	} else if (self->wait)
		self->nextthink = level.time + FRAMETIME;
}

void find_target_blaster_target(edict_t *self, edict_t *other, edict_t *activator)
{
	target_blaster_think(self);
}

void toggle_target_blaster (edict_t *self, edict_t *other, edict_t *activator)
{
	// used for target_blasters with a "wait" value

	self->activator = activator;
	if (self->spawnflags & 4) {

		self->count--;
		if (!self->count) {
			self->think = G_FreeEdict;
			self->nextthink = level.time + 1;
		}
		else
		{
			self->spawnflags &= ~4;
			self->nextthink = 0;
		}
	} else {
		self->spawnflags |= 4;
		self->think (self);
	}
}

void target_blaster_init (edict_t *self)
{
	if (self->target) {
		edict_t	*ent;
		ent = G_Find (NULL, FOFS(targetname), self->target);
		if (!ent)
			gi.dprintf("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
		self->enemy = ent;
	}
}
void SP_target_blaster (edict_t *self)
{
	G_SetMovedir (self->s.angles, self->movedir);
	self->noise_index = gi.soundindex ("weapons/laser2.wav");

	if (!self->dmg)
		self->dmg = 15;
	if (!self->speed)
		self->speed = 1000;

	// If SEEK_PLAYER is not set and there's no target, then
	// IF_VISIBLE is meaningless
	if (!(self->spawnflags & 128) && !self->target)
		self->spawnflags &= ~16;

	if (self->wait) {
		// toggled target_blaster
		self->use = toggle_target_blaster;
		self->enemy = NULL; // for now
		self->think = target_blaster_think;
		if (self->spawnflags & 4)
			self->nextthink = level.time + 1;
		else
			self->nextthink = 0;
	} else if (self->target || (self->spawnflags & SEEK_PLAYER)) {
		self->use = find_target_blaster_target;
		if (self->target) {
			self->think = target_blaster_init;
			self->nextthink = level.time + 2*FRAMETIME;
		}
	} else {
		// normal targeted target_blaster
		self->use = use_target_blaster;
	}

	gi.linkentity(self);
	self->svflags = SVF_NOCLIENT;
}


//==========================================================

/*QUAKED target_crosslevel_trigger (.5 .5 .5) (-8 -8 -8) (8 8 8) trigger1 trigger2 trigger3 trigger4 trigger5 trigger6 trigger7 trigger8
Once this trigger is touched/used, any trigger_crosslevel_target with the same trigger number is automatically used when a level is started within the same unit.  It is OK to check multiple triggers.  Message, delay, target, and killtarget also work.
*/
void trigger_crosslevel_trigger_use (edict_t *self, edict_t *other, edict_t *activator)
{
	game.serverflags |= self->spawnflags;
	// DWH: By most editors, the trigger should be able to fire targets. Added
	//      the following line:
	G_UseTargets (self, activator);
	G_FreeEdict (self);
}

void SP_target_crosslevel_trigger (edict_t *self)
{
	self->svflags = SVF_NOCLIENT;
	self->use = trigger_crosslevel_trigger_use;
}

/*QUAKED target_crosslevel_target (.5 .5 .5) (-8 -8 -8) (8 8 8) trigger1 trigger2 trigger3 trigger4 trigger5 trigger6 trigger7 trigger8
Triggered by a trigger_crosslevel elsewhere within a unit.  If multiple triggers are checked, all must be true.  Delay, target and
killtarget also work.

"delay"		delay before using targets if the trigger has been activated (default 1)
*/
void target_crosslevel_target_think (edict_t *self)
{
	if (self->spawnflags == (game.serverflags & SFL_CROSS_TRIGGER_MASK & self->spawnflags))
	{
		G_UseTargets (self, self);
		G_FreeEdict (self);
	}
}

void SP_target_crosslevel_target (edict_t *self)
{
	if (! self->delay)
		self->delay = 1;
	self->svflags = SVF_NOCLIENT;

	self->think = target_crosslevel_target_think;
	self->nextthink = level.time + self->delay;
}

//==========================================================

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON RED GREEN BLUE YELLOW ORANGE FAT
When triggered, fires a laser.  You can either set a target
or a direction.
*/

// DWH - player-seeking laser stuff
void target_laser_ps_think (edict_t *self)
{
	edict_t	*ignore;
	edict_t *player;
	trace_t	tr;
	vec3_t	start;
	vec3_t	end;
	vec3_t	point;
	vec3_t	last_movedir;
	vec3_t	target;
	int		count;
	int		i;

	if (self->wait > 0)
	{
		if ( level.time >= self->starttime ) {
			self->starttime = level.time + self->wait;
			self->endtime   = level.time + self->delay;
		}
		else if ( level.time >= self->endtime ) {
			self->nextthink = level.time + FRAMETIME;
			return;
		}
	}

	if (self->spawnflags & 0x80000000)
		count = 8;    // spark count
	else
		count = 4;

	if (self->enemy)
	{
		if (self->enemy->flags & FL_NOTARGET || (self->enemy->health < self->enemy->gib_health) )
			self->enemy = NULL;
		else {
			// first make sure laser can see the center of the enemy
			VectorMA(self->enemy->absmin,0.5,self->enemy->size,target);
			tr = gi.trace(self->s.origin,vec3_origin,vec3_origin,target,self,MASK_OPAQUE);
			if (tr.fraction != 1.0)
				self->enemy = NULL;
		}
	}
	if (!self->enemy)
	{
		// find a player - as with target_blaster, search entire entity list so
		// we'll pick up fake players representing camera-viewers
		for(i=1, player=g_edicts+1; i<globals.num_edicts && !self->enemy; i++, player++) {
			if (!player->inuse) continue;
			if (!player->client) continue;
			if (player->svflags & SVF_NOCLIENT) continue;
			if ((player->health >= player->gib_health) && !(player->flags & FL_NOTARGET) ) {
				VectorMA(player->absmin,0.5,player->size,target);
				tr = gi.trace(self->s.origin,vec3_origin,vec3_origin,target,self,MASK_OPAQUE);
				if (tr.fraction == 1.0) {
					self->enemy = player;
					self->spawnflags |= 0x80000001;
					count = 8;
				}
			}
		}
	}
	if (!self->enemy)
	{
		self->svflags |= SVF_NOCLIENT;
		self->nextthink = level.time + FRAMETIME;
		return;
	}

	self->svflags &= ~SVF_NOCLIENT;
	VectorCopy (self->movedir, last_movedir);
	VectorMA (self->enemy->absmin, 0.5, self->enemy->size, point);
	VectorSubtract (point, self->s.origin, self->movedir);
	VectorNormalize (self->movedir);
	if (!VectorCompare(self->movedir, last_movedir))
		self->spawnflags |= 0x80000000;

	ignore = self;
	VectorCopy (self->s.origin, start);
	VectorMA (start, 2048, self->movedir, end);
	while(1)
	{
		tr = gi.trace (start, NULL, NULL, end, ignore, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);

		if (!tr.ent)
			break;

		// hurt it if we can
		if (tr.ent->takedamage && !self->style)
		{
			if (!(tr.ent->flags & FL_IMMUNE_LASER) && (self->dmg > 0) )
				T_Damage (tr.ent, self, self->activator, self->movedir, tr.endpos, vec3_origin, self->dmg, 1, DAMAGE_ENERGY, MOD_TARGET_LASER);
			else if (self->dmg < 0)
			{
				tr.ent->health -= self->dmg;
				if (tr.ent->health > tr.ent->max_health)
					tr.ent->health = tr.ent->max_health;
			}
		}

		// if we hit something that's not a monster or player or is immune to lasers, we're done
		if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client))
		{
			if ((self->spawnflags & 0x80000000) && (self->style != 3))
			{
				self->spawnflags &= ~0x80000000;
				gi.WriteByte (svc_temp_entity);
				gi.WriteByte (TE_LASER_SPARKS);
				gi.WriteByte (count);
				gi.WritePosition (tr.endpos);
				gi.WriteDir (tr.plane.normal);
				gi.WriteByte (self->s.skinnum);
				gi.multicast (tr.endpos, MULTICAST_PVS);
			}
			break;
		}

		ignore = tr.ent;
		VectorCopy (tr.endpos, start);
	}
	VectorCopy (tr.endpos, self->s.old_origin);
	self->nextthink = level.time + FRAMETIME;
}

void target_laser_ps_on (edict_t *self)
{
	if (!self->activator)
		self->activator = self;

	self->spawnflags |= 0x80000001;
//	self->svflags &= ~SVF_NOCLIENT;
	if (self->wait > 0)
	{
		self->starttime = level.time + self->wait;
		self->endtime = level.time + self->delay;
	}
	target_laser_ps_think (self);
}

void target_laser_ps_off (edict_t *self)
{
	self->spawnflags &= ~1;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0;
}

void target_laser_ps_use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->activator = activator;
	if (self->spawnflags & 1)
	{
		target_laser_ps_off (self);
		self->count--;
		if (!self->count)
		{
			self->think = G_FreeEdict;
			self->nextthink = level.time + 1;
		}
	}
	else
		target_laser_ps_on (self);
}

void target_laser_think (edict_t *self)
{
	edict_t	*ignore;
	vec3_t	start;
	vec3_t	end;
	trace_t	tr;
	vec3_t	point;
	vec3_t	last_movedir;
	int		count;

	// DWH
	if (self->wait > 0)
	{
		// pulsed laser
		if (level.time >= self->starttime)
		{
			self->starttime = level.time + self->wait;
			self->endtime   = level.time + self->delay;
		}
		else if (level.time >= self->endtime)
		{
			self->nextthink = level.time + FRAMETIME;
			return;
		}
	}
	// end DWH

	if (self->spawnflags & 0x80000000)
		count = 8;
	else
		count = 4;

	if (self->enemy)
	{
		VectorCopy (self->movedir, last_movedir);
		VectorMA (self->enemy->absmin, 0.5, self->enemy->size, point);
		VectorSubtract (point, self->s.origin, self->movedir);
		VectorNormalize (self->movedir);
		if (!VectorCompare(self->movedir, last_movedir))
			self->spawnflags |= 0x80000000;
	}

	ignore = self;
	VectorCopy (self->s.origin, start);
	VectorMA (start, 2048, self->movedir, end);
	while(1)
	{
		tr = gi.trace (start, NULL, NULL, end, ignore, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);

		if (!tr.ent)
			break;

		// hurt it if we can
		if (tr.ent->takedamage && !self->style)
		{
			if (!(tr.ent->flags & FL_IMMUNE_LASER) && (self->dmg > 0))
				T_Damage (tr.ent, self, self->activator, self->movedir, tr.endpos, vec3_origin, self->dmg, 1, DAMAGE_ENERGY, MOD_TARGET_LASER);
			else if (self->dmg < 0)
			{
				tr.ent->health -= self->dmg;
				if (tr.ent->health > tr.ent->max_health)
					tr.ent->health = tr.ent->max_health;
			}
		}

		// if we hit something that's not a monster or player or is immune to lasers, we're done
		if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client) )
		{
			if ((self->spawnflags & 0x80000000) && (self->style != 3))
			{
				self->spawnflags &= ~0x80000000;
				gi.WriteByte (svc_temp_entity);
				gi.WriteByte (TE_LASER_SPARKS);
				gi.WriteByte (count);
				gi.WritePosition (tr.endpos);
				gi.WriteDir (tr.plane.normal);
				gi.WriteByte (self->s.skinnum);
				gi.multicast (tr.endpos, MULTICAST_PVS);
			}
			break;
		}

		ignore = tr.ent;
		VectorCopy (tr.endpos, start);
	}

	VectorCopy (tr.endpos, self->s.old_origin);

	self->nextthink = level.time + FRAMETIME;
}

void target_laser_on (edict_t *self)
{
	if (self->wait > 0)
	{
		self->starttime = level.time + self->wait;
		self->endtime = level.time + self->delay;
	}
	if (!self->activator)
		self->activator = self;
	self->spawnflags |= 0x80000001;
	self->svflags &= ~SVF_NOCLIENT;
	target_laser_think (self);
}

void target_laser_off (edict_t *self)
{
	self->spawnflags &= ~1;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0;
}

void target_laser_use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->activator = activator;
	if (self->spawnflags & 1)
	{
		target_laser_off (self);
		self->count--;
		if (!self->count)
		{
			self->think = G_FreeEdict;
			self->nextthink = level.time + 1;
		}
	}
	else
		target_laser_on (self);
}

void target_laser_start (edict_t *self)
{
	edict_t *ent;

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->s.renderfx |= RF_BEAM|RF_TRANSLUCENT;
	self->s.modelindex = 1;			// must be non-zero

	// set the beam diameter
	if (self->mass > 1)
		self->s.frame = self->mass;
	else if (self->spawnflags & 64)
		self->s.frame = 16;
	else
		self->s.frame = 4;

	// set the color
	if (self->spawnflags & 2)
		self->s.skinnum = 0xf2f2f0f0;
	else if (self->spawnflags & 4)
		self->s.skinnum = 0xd0d1d2d3;
	else if (self->spawnflags & 8)
		self->s.skinnum = 0xf3f3f1f1;
	else if (self->spawnflags & 16)
		self->s.skinnum = 0xdcdddedf;
	else if (self->spawnflags & 32)
		self->s.skinnum = 0xe0e1e2e3;


	if (!self->dmg)
		self->dmg = 1;
	VectorSet (self->mins, -8, -8, -8);
	VectorSet (self->maxs, 8, 8, 8);

	// DWH

	// pulsed laser
	if (self->wait > 0)
	{
		if (self->delay >= self->wait)
		{
			gi.dprintf("target_laser at %s, delay must be < wait.\n", vtos(self->s.origin));
			self->wait = 0;
		}
		else if (self->delay == 0.)
		{
			gi.dprintf("target_laser at %s, wait > 0 but delay = 0\n", vtos(self->s.origin));
			self->wait = 0;
		}
	}

	if (self->spawnflags & 128)
	{
		// player-seeking laser
		self->enemy = NULL;
		self->use = target_laser_ps_use;
		self->think = target_laser_ps_think;
		gi.linkentity(self);
		if (self->spawnflags & 1)
			target_laser_ps_on(self);
		else
			target_laser_ps_off(self);
		return;
	}
	// end DWH

	if (!self->enemy)
	{
		if (self->target)
		{
			ent = G_Find (NULL, FOFS(targetname), self->target);
			if (!ent)
				gi.dprintf ("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
			self->enemy = ent;
		}
		else
		{
			G_SetMovedir (self->s.angles, self->movedir);
		}
	}
	self->use = target_laser_use;
	self->think = target_laser_think;

	gi.linkentity (self);

	if (self->spawnflags & 1)
		target_laser_on (self);
	else
		target_laser_off (self);
}

void SP_target_laser (edict_t *self)
{
	// let everything else get spawned before we start firing
	self->think = target_laser_start;
	self->nextthink = level.time + 1;
}

//==========================================================

/*QUAKED target_lightramp (0 .5 .8) (-8 -8 -8) (8 8 8) TOGGLE
speed		How many seconds the ramping will take
message		two letters; starting lightlevel and ending lightlevel
*/

#define LIGHTRAMP_TOGGLE 1
#define LIGHTRAMP_CUSTOM 2
#define LIGHTRAMP_LOOP	 4
#define LIGHTRAMP_ACTIVE 128

void target_lightramp_think (edict_t *self)
{
	char	style[2];

	if (self->spawnflags & LIGHTRAMP_CUSTOM) {
		if (self->movedir[2] > 0)
			style[0] = self->message[(int)self->movedir[0]];
		else
			style[0] = self->message[(int)(self->movedir[1]-self->movedir[0])];
		self->movedir[0]++;
	} else {
		style[0] = 'a' + self->movedir[0] + (level.time - self->timestamp) / FRAMETIME * self->movedir[2];
	}
	style[1] = 0;
	gi.configstring (CS_LIGHTS+self->enemy->style, style);

	if (self->spawnflags & LIGHTRAMP_CUSTOM) {
		if ((self->movedir[0] <= self->movedir[1]) ||
		   ((self->spawnflags & LIGHTRAMP_LOOP) && (self->spawnflags & LIGHTRAMP_ACTIVE)) ) {
			self->nextthink = level.time + FRAMETIME;
			if (self->movedir[0] > self->movedir[1]) {
				self->movedir[0] = 0;
				if (self->spawnflags & LIGHTRAMP_TOGGLE)
					self->movedir[2] *= -1;
			}
		} else {
			self->movedir[0] = 0;
			if (self->spawnflags & LIGHTRAMP_TOGGLE)
				self->movedir[2] *= -1;

			self->count--;
			if (!self->count) {
				self->think = G_FreeEdict;
				self->nextthink = level.time + 1;
			}
		}
	} else {
		if ( (level.time - self->timestamp) < self->speed) {
			self->nextthink = level.time + FRAMETIME;
		} else if (self->spawnflags & LIGHTRAMP_TOGGLE) {
			char	temp;
			
			temp = self->movedir[0];
			self->movedir[0] = self->movedir[1];
			self->movedir[1] = temp;
			self->movedir[2] *= -1;
			if ( (self->spawnflags & LIGHTRAMP_LOOP) && (self->spawnflags & LIGHTRAMP_ACTIVE) ) {
				self->timestamp = level.time;
				self->nextthink = level.time + FRAMETIME;
			}
		} else if ((self->spawnflags & LIGHTRAMP_LOOP) && (self->spawnflags & LIGHTRAMP_ACTIVE)) {
			// Not toggled, looping. Start sequence over
			self->timestamp = level.time;
			self->nextthink = level.time + FRAMETIME;
		} else {
			self->count--;
			if (!self->count) {
				self->think = G_FreeEdict;
				self->nextthink = level.time + 1;
			}
		}
	}
}

void target_lightramp_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->spawnflags & LIGHTRAMP_LOOP) {
		if (self->spawnflags & LIGHTRAMP_ACTIVE) {
			self->spawnflags &= ~LIGHTRAMP_ACTIVE;	// already on, turn it off
			target_lightramp_think(self);
			return;
		} else {
			self->spawnflags |= LIGHTRAMP_ACTIVE;
		}
	}

	if (!self->enemy)
	{
		edict_t		*e;

		// check all the targets
		e = NULL;
		while (1)
		{
			e = G_Find (e, FOFS(targetname), self->target);
			if (!e)
				break;
			if (strcmp(e->classname, "light") != 0)
			{
				gi.dprintf("%s at %s ", self->classname, vtos(self->s.origin));
				gi.dprintf("target %s (%s at %s) is not a light\n", self->target, e->classname, vtos(e->s.origin));
			}
			else
			{
				self->enemy = e;
			}
		}

		if (!self->enemy)
		{
			gi.dprintf("%s target %s not found at %s\n", self->classname, self->target, vtos(self->s.origin));
			G_FreeEdict (self);
			return;
		}
	}

	self->timestamp = level.time;
	target_lightramp_think (self);
}

void SP_target_lightramp (edict_t *self)
{
	// DWH: CUSTOM spawnflag allows custom light switching, speed is ignored
	if (self->spawnflags & LIGHTRAMP_CUSTOM) {
		if (!self->message || strlen(self->message) < 2) {
			gi.dprintf("custom target_lightramp has bad ramp (%s) at %s\n", self->message, vtos(self->s.origin));
			G_FreeEdict (self);
			return;
		}
	} else {
		if (!self->message || strlen(self->message) != 2 || self->message[0] < 'a' || self->message[0] > 'z' || self->message[1] < 'a' || self->message[1] > 'z' || self->message[0] == self->message[1])
		{
			gi.dprintf("target_lightramp has bad ramp (%s) at %s\n", self->message, vtos(self->s.origin));
			G_FreeEdict (self);
			return;
		}
	}

	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	if (!self->target)
	{
		gi.dprintf("%s with no target at %s\n", self->classname, vtos(self->s.origin));
		G_FreeEdict (self);
		return;
	}

	self->svflags |= SVF_NOCLIENT;
	self->use = target_lightramp_use;
	self->think = target_lightramp_think;

	if (self->spawnflags & LIGHTRAMP_CUSTOM) {
		self->movedir[0] = 0;                        // index into message
		self->movedir[1] = strlen(self->message)-1;  // position of last character
		self->movedir[2] = 1;                        // direction = start->end
	} else {
		self->movedir[0] = self->message[0] - 'a';
		self->movedir[1] = self->message[1] - 'a';
		self->movedir[2] = (self->movedir[1] - self->movedir[0]) / (self->speed / FRAMETIME);
	}

	self->spawnflags &= ~LIGHTRAMP_ACTIVE;		// not currently on
}

//==========================================================

/*QUAKED target_earthquake (1 0 0) (-8 -8 -8) (8 8 8)
When triggered, this initiates a level-wide earthquake.
All players and monsters are affected.
"speed"		severity of the quake (default:200)
"count"		duration of the quake (default:5)
*/

void target_earthquake_think (edict_t *self)
{
	int		i;
	edict_t	*e;

	if (self->last_move_time < level.time)
	{
		gi.positioned_sound (self->s.origin, self, CHAN_AUTO, self->noise_index, 1.0, ATTN_NONE, 0);
		self->last_move_time = level.time + 0.5;
	}

	for (i=1, e=g_edicts+i; i < globals.num_edicts; i++,e++)
	{
		if (!e->inuse)
			continue;
		if (!e->client)
			continue;
		if (!e->groundentity)
			continue;
		// Lazarus: special case for tracktrain riders - 
		// earthquakes hurt 'em too bad, so don't shake 'em
		if ((e->groundentity->flags & FL_TRACKTRAIN) && (e->groundentity->moveinfo.state))
			continue;
		e->groundentity = NULL;
		e->velocity[0] += crandom()* 150;
		e->velocity[1] += crandom()* 150;
		e->velocity[2] = self->speed * (100.0 / e->mass);
	}

	if (level.time < self->timestamp)
		self->nextthink = level.time + FRAMETIME;
}

void target_earthquake_use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->timestamp = level.time + self->count;
	self->nextthink = level.time + FRAMETIME;
	self->activator = activator;
	self->last_move_time = 0;
}

void SP_target_earthquake (edict_t *self)
{
	if (!self->targetname)
		gi.dprintf("untargeted %s at %s\n", self->classname, vtos(self->s.origin));

	if (!self->count)
		self->count = 5;

	if (!self->speed)
		self->speed = 200;

	self->svflags |= SVF_NOCLIENT;
	self->think = target_earthquake_think;
	self->use = target_earthquake_use;

	self->noise_index = gi.soundindex ("world/quake.wav");
}

// DWH
//
// Tremor stuff follows
//
//
// target_locator can be used to move entities to a random selection
// from a series of path_corners. Move takes place at level start ONLY.
//
void target_locator_init(edict_t *self)
{
	int num_points=0;
	int i, N, nummoves;
	qboolean looped;
	edict_t *tgt0, *tgtlast=NULL, *target, *next;
	edict_t *move;

	move = NULL;
	move = G_Find(move,FOFS(targetname),self->target);

	if (!move)
	{
		gi.dprintf("Target of target_locator (%s) not found.\n",
			self->target);
		G_FreeEdict(self);
		return;
	}
	target = G_Find(NULL,FOFS(targetname),self->pathtarget);
	if (!target)
	{
		gi.dprintf("Pathtarget of target_locator (%s) not found.\n",
			self->pathtarget);
		G_FreeEdict(self);
		return;
	}

	srand(time(NULL));
	tgt0 = target;
	next = NULL;
	target->spawnflags &= 0x7FFE;
	while(next != tgt0)
	{
		if (target->target)
		{
			next = G_Find(NULL,FOFS(targetname),target->target);
			if ((!next) || (next==tgt0)) tgtlast = target;
			if (!next)
			{
				gi.dprintf("Target %s of path_corner at %s not found.\n",
					target->target,vtos(target->s.origin));
				break;
			}
			target = next;
			target->spawnflags &= 0x7FFE;
			num_points++;
		}
		else
		{
			next = tgt0;
			tgtlast = target;
		}
	}
	if (!num_points) num_points=1;
	
	nummoves = 1;
	while(move)
	{
		if (nummoves > num_points) break;  // more targets than path_corners

		N = rand() % num_points;
		i = 0;
		next   = tgt0;
		looped = false;
		while(i<=N)
		{
			target = next;
			if (!(target->spawnflags & 1)) i++;
			if (target==tgtlast)
			{
				// We've looped thru all path_corners, but not
				// reached the target number yet. This can only
				// happen in the case of multiple targets. Use the
				// next available path_corner.
				looped = true;
			}
			if (looped && !(target->spawnflags & 1)) i = N+1;
			next = G_Find(NULL,FOFS(targetname),target->target);
		}
		target->spawnflags |= 1;

		// Assumptions here: SOLID_BSP entities are assumed to be brush models,
		// all others are point ents
		if (move->solid == SOLID_BSP)
		{
			vec3_t origin;
			VectorAdd(move->absmin,move->absmax,origin);
			VectorScale(origin,0.5,origin);
			VectorSubtract(target->s.origin,origin,move->s.origin);
		}
		else {
			VectorCopy(target->s.origin,move->s.origin);
			VectorCopy(target->s.angles,move->s.angles);
		}
		M_droptofloor(move);
		gi.linkentity(move);
		move = G_Find(move,FOFS(targetname),self->target);
		nummoves++;
	}
	// All done, go away
	G_FreeEdict(self);
}
void SP_target_locator(edict_t *self)
{
	if (!self->target)
	{
		gi.dprintf("target_locator w/o target at %s\n",vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}
	if (!self->pathtarget)
	{
		gi.dprintf("target_locator w/o pathtarget at %s\n",vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}
	self->think = target_locator_init;
	self->nextthink = level.time + 2*FRAMETIME;
	gi.linkentity(self);
}

//
// TARGET_ANGER
//
// target      Monster(s) to make angry
// killtarget  Entity to get angry at
// pathtarget  Entity to run to
// Spawnflags:
// HOLD (16)   Stand in place
// BRUTAL (32) Gib killtarget

void use_target_anger(edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t		*kill_me=NULL, *movetarget;
	edict_t		*t;
	vec3_t		vec;
	float		dist, best_dist;
	edict_t		*best_target;

	if (self->pathtarget)
		movetarget = G_PickTarget(self->pathtarget);
	else
		movetarget = NULL;

	if (self->target)
	{
		t = NULL;
		while ((t = G_Find (t, FOFS(targetname), self->target)))
		{
			if (t == self)
			{
				gi.dprintf ("WARNING: entity used itself.\n");
			}
			else
			{
				if (t->use)
				{
					if (t->health < 0)
						return;
					if (self->movedir[2] > 0)
					{
						t->velocity[0] = self->movedir[0] * self->speed;
						t->velocity[1] = self->movedir[1] * self->speed;
						if (t->groundentity)
						{
							t->groundentity = NULL;
							t->velocity[2] = self->movedir[2];
							if (t->monsterinfo.aiflags & AI_ACTOR)
								gi.sound (self, CHAN_VOICE, t->actor_sound_index[0], 1, ATTN_NORM, 0);
						}
					}
					if (self->killtarget)
					{
						kill_me = G_Find(NULL, FOFS(targetname), self->killtarget);
						if (kill_me)
						{
							best_dist = 9000.;
							best_target = NULL;
							if (kill_me->health > 0)
							{
								VectorSubtract(kill_me->s.origin,t->s.origin,vec);
								best_dist = VectorLength(vec);
								best_target = kill_me;
							}
							while (kill_me)
							{
								kill_me = G_Find(kill_me, FOFS(targetname), self->killtarget);
								if (!kill_me)
									break;
								if (!kill_me->inuse)
									continue;
								if (kill_me->health <= 0)
									continue;
								VectorSubtract(kill_me->s.origin,t->s.origin,vec);
								dist = VectorLength(vec);
								if (dist < best_dist)
								{
									best_dist = dist;
									best_target = kill_me;
								}
							}
							kill_me = best_target;
						}
					}
					if (kill_me)
					{
						// Make whatever a "good guy" so the monster will try to kill it!
						kill_me->monsterinfo.aiflags |= AI_GOOD_GUY;
						t->enemy = t->goalentity = kill_me;
						t->monsterinfo.aiflags |= AI_TARGET_ANGER;
						if (movetarget)
							t->movetarget = movetarget;
						FoundTarget (t);
					}
					else
					{
						t->monsterinfo.pausetime = 0;
						t->goalentity = t->movetarget = movetarget;
					}

					if ((self->spawnflags & 16) && !(t->flags & (FL_SWIM|FL_FLY)))
					{
						t->monsterinfo.pausetime = level.time + 100000000;
						t->monsterinfo.aiflags |= AI_STAND_GROUND;
						t->monsterinfo.stand (t);
					}
					if (self->spawnflags & 32)
						t->monsterinfo.aiflags |= AI_BRUTAL;
				}
			}
			if (!self->inuse)
			{
				gi.dprintf("entity was removed while using targets\n");
				return;
			}
		}
	}

	self->count--;
	if (!self->count) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}

}

void SP_target_anger(edict_t *self)
{
	if (deathmatch->value) {
		G_FreeEdict(self);
		return;
	}
	if (!self->target) {
		gi.dprintf("target_anger with no target set at %s\n",vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}
	if (!self->killtarget && !self->pathtarget) {
		gi.dprintf("target_anger with no killtarget or\npathtarget set at %s\n",vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}
	// pathtarget is incompatible with HOLD SF
	if (self->pathtarget && (self->spawnflags & 16))
	{
		gi.dprintf("target anger at %s,\npathtarget is incompatible with HOLD\n",
			vtos(self->s.origin));
		self->spawnflags &= ~16;
	}

	G_SetMovedir (self->s.angles, self->movedir);
	self->movedir[2] = st.height;
	self->use = use_target_anger;
}

// target_monsterbattle serves the same purpose as target_anger, but 
// ends up turning a dmgteam group of monsters against another dmgteam

void use_target_monsterbattle(edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t *grouch, *grouchmate;
	edict_t *target, *targetmate;

	grouch = G_Find(NULL,FOFS(targetname),self->target);
	if (!grouch) return;
	if (!grouch->inuse) return;
	target = G_Find(NULL,FOFS(targetname),self->killtarget);
	if (!target) return;
	if (!target->inuse) return;
	if (grouch->dmgteam) {
		grouchmate = G_Find(NULL,FOFS(dmgteam),grouch->dmgteam);
		while(grouchmate) {
			grouchmate->monsterinfo.aiflags |= AI_FREEFORALL;
			grouchmate = G_Find(grouchmate,FOFS(dmgteam),grouch->dmgteam);
		}
	}
	if (target->dmgteam) {
		targetmate = G_Find(NULL,FOFS(dmgteam),target->dmgteam);
		while(targetmate) {
			targetmate->monsterinfo.aiflags |= AI_FREEFORALL;
			targetmate = G_Find(targetmate,FOFS(dmgteam),target->dmgteam);
		}
	}
	grouch->enemy = target;
	grouch->monsterinfo.aiflags |= AI_TARGET_ANGER;
	FoundTarget(grouch);

	self->count--;
	if (!self->count) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}
}
void SP_target_monsterbattle(edict_t *self)
{
	if (deathmatch->value) {
		G_FreeEdict(self);
		return;
	}
	if (!self->target) {
		gi.dprintf("target_monsterbattle with no target set at %s\n",vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}
	if (!self->killtarget) {
		gi.dprintf("target_monsterbattle with no killtarget set at %s\n",vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}
	self->use = use_target_monsterbattle;
}

/*====================================================================================
   TARGET_ROCKS
======================================================================================*/
void directed_debris_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	G_FreeEdict (self);
}
void FadeDieThink(edict_t *);
void ThrowRock (edict_t *self, char *modelname, float speed, vec3_t origin, vec3_t size, int mass)
{
	edict_t	*chunk;
	vec_t	var = speed/5;

	chunk = G_Spawn();
	VectorCopy (origin, chunk->s.origin);
	gi.setmodel (chunk, modelname);
	VectorCopy(size,chunk->maxs);
	VectorScale(chunk->maxs,0.5,chunk->maxs);
	VectorNegate(chunk->maxs,chunk->mins);
	chunk->velocity[0] = speed * self->movedir[0] + var * crandom();
	chunk->velocity[1] = speed * self->movedir[1] + var * crandom();
	chunk->velocity[2] = speed * self->movedir[2] + var * crandom();
	chunk->movetype = MOVETYPE_DEBRIS;
	chunk->attenuation = 0.5;
	chunk->solid = SOLID_NOT;
	chunk->avelocity[0] = random()*600;
	chunk->avelocity[1] = random()*600;
	chunk->avelocity[2] = random()*600;
	chunk->think = FadeDieThink;
	chunk->nextthink = level.time + 15 + random()*5;
	chunk->s.frame = 0;
	chunk->flags = 0;
	chunk->classname = "debris";
	chunk->takedamage = DAMAGE_YES;
	chunk->die = directed_debris_die;
	chunk->mass = mass;
	gi.linkentity (chunk);
}

void use_target_rocks (edict_t *self, edict_t *other, edict_t *activator)
{
	vec3_t	chunkorigin;
	vec3_t	size, source;
	vec_t	mass;
	int		count;
	char	modelname[64];

	VectorSet(source,8,8,8);
	mass = self->mass;
	// big chunks
	if (mass >= 100)
	{
		sprintf(modelname,"models/objects/rock%d/tris.md2",self->style*2+1);
		count = mass / 100;
		if (count > 16)
			count = 16;
		VectorSet(size,8,8,8);
		while(count--)
		{
			chunkorigin[0] = self->s.origin[0] + crandom() * source[0];
			chunkorigin[1] = self->s.origin[1] + crandom() * source[1];
			chunkorigin[2] = self->s.origin[2] + crandom() * source[2];
			ThrowRock (self, modelname, self->speed, chunkorigin, size, 100);
		}
	}
	// small chunks
	count = mass / 25;
	sprintf(modelname,"models/objects/rock%d/tris.md2",self->style*2+2);
	if (count > 16)
		count = 16;
	VectorSet(size,4,4,4);
	while(count--)
	{
		chunkorigin[0] = self->s.origin[0] + crandom() * source[0];
		chunkorigin[1] = self->s.origin[1] + crandom() * source[1];
		chunkorigin[2] = self->s.origin[2] + crandom() * source[2];
		ThrowRock (self, modelname, self->speed, chunkorigin, size, 25);
	}
	if (self->dmg)
		T_RadiusDamage (self, activator, self->dmg, NULL, self->dmg+40, MOD_SPLASH, -0.5);

	self->count--;
	if (!self->count) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}

}

void SP_target_rocks (edict_t *self)
{
	gi.modelindex ("models/objects/rock1/tris.md2");
	gi.modelindex ("models/objects/rock2/tris.md2");
	if (!self->speed)
		self->speed = 400;
	if (!self->mass)
		self->mass = 500;
	self->use = use_target_rocks;
	G_SetMovedir (self->s.angles, self->movedir);
	self->svflags = SVF_NOCLIENT;
}

/*====================================================================================
   TARGET_ROTATION
======================================================================================*/

void use_target_rotation (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t	*target;
	int		i, pick;
	char	*p1, *p2;
	char	targetname[256];

	if (self->spawnflags & 2) {
		// random pick
		pick = self->sounds * random();
		if (pick == self->sounds) pick--;
	} else {
		pick = self->mass;
		if (pick == self->sounds) {
			if (self->spawnflags & 1)    // no loop
				return;
			else
				pick = 0;
		}
		self->mass = pick+1;
	}
	p1 = self->target;
	p2 = targetname;
	memset(targetname,0,sizeof(targetname));
	// skip over pick commas
	for(i=0; i<pick; i++) {
		p1 = strstr(p1,",");
		if (p1)
			p1++;
		else
			return;	// should never happen
	}
	while(*p1 != 0 && *p1 != ',') {
		*p2 = *p1;
		p1++;
		p2++;
	}
	target = G_Find(NULL,FOFS(targetname),targetname);
	while(target) {
		if (target->inuse && target->use)
			target->use(target,other,activator);
		target = G_Find(target,FOFS(targetname),targetname);
	}
	self->count--;
	if (!self->count) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}
}

void SP_target_rotation (edict_t *self)
{
	char	*p;

	if (!self->target) {
		gi.dprintf("target_rotation without a target at %s\n",vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}
	if ( (self->spawnflags & 3) == 3) {
		gi.dprintf("target_rotation at %s: NO_LOOP and RANDOM are mutually exclusive.\n");
		self->spawnflags = 2;
	}
	self->use = use_target_rotation;
	self->svflags = SVF_NOCLIENT;
	self->mass = 0;  // index of currently selected target
	self->sounds = 0; // number of comma-delimited targets in target string
	p = self->target;
	while( (p = strstr(p,",")) != NULL) {
		self->sounds++;
		p++;
	}
	self->sounds++;
}

/*====================================================================================
   TARGET_EFFECT
======================================================================================*/

/* Unknowns or not supported
TE_FLAME,              32  Rogue flamethrower, never implemented
TE_FORCEWALL,          37  ??
*/

//=========================================================================
/* Spawns an effect at the entity origin
  TE_FLASHLIGHT         36
*/
void target_effect_at (edict_t *self, edict_t *activator)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (self->style);
	gi.WritePosition (self->s.origin);
	gi.WriteShort (self - g_edicts);
	gi.multicast (self->s.origin, MULTICAST_PVS);
}
/* Poor man's target_steam
  TE_STEAM           40
*/
void target_effect_steam (edict_t *self, edict_t *activator)
{
	static int nextid;
	int	wait;

	if (self->wait)
		wait = self->wait*1000;
	else
		wait = 0;

	if (nextid > 20000)
		nextid = nextid %20000;
	nextid++;

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (self->style);
	gi.WriteShort (nextid);
	gi.WriteByte (self->count);
	gi.WritePosition (self->s.origin);
	gi.WriteDir (self->movedir);
	gi.WriteByte (self->sounds&0xff);
	gi.WriteShort ( (int)(self->speed) );
	gi.WriteLong ( (int)(wait) );
	gi.multicast (self->s.origin, MULTICAST_PVS);

	if (level.num_reflectors)
		ReflectSteam (self->s.origin,self->movedir,self->count,self->sounds,(int)(self->speed),wait,nextid);
}

//=========================================================================
/*
Spawns (style) Splash with (count) particles of (sounds) color at (origin)
moving in (movedir) direction.

  TE_SPLASH             10 Randomly shaded shower of particles
  TE_LASER_SPARKS       15 Splash particles obey gravity
  TE_WELDING_SPARKS     25 Splash particles with flash of light at {origin}
*/
//=========================================================================
void target_effect_splash (edict_t *self, edict_t *activator)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(self->style);
	gi.WriteByte(self->count);
	gi.WritePosition(self->s.origin);
	gi.WriteDir(self->movedir);
	gi.WriteByte(self->sounds);
	gi.multicast(self->s.origin, MULTICAST_PVS);
}

//======================================================
/*
Spawns a trail of (type) from (start) to (end) and Broadcasts to all
in Potentially Visible Set from vector (origin)

  TE_RAILTRAIL             3 Spawns a blue spiral trail filled with white smoke
  TE_BUBBLETRAIL          11 Spawns a trail of bubbles
  TE_PARASITE_ATTACK      16
  TE_MEDIC_CABLE_ATTACK   19
  TE_BFG_LASER            23 Spawns a green laser
  TE_GRAPPLE_CABLE        24
  TE_RAILTRAIL2           31 NOT IMPLEMENTED IN ENGINE
  TE_DEBUGTRAIL           34
  TE_HEATBEAM,            38 Requires Rogue model
  TE_MONSTER_HEATBEAM,    39 Requires Rogue model
  TE_BUBBLETRAIL2         41
*/
//======================================================
void target_effect_trail (edict_t *self, edict_t *activator)
{
	edict_t	*target;

	if (!self->target) return;
	target = G_Find(NULL,FOFS(targetname),self->target);
	if (!target) return;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(self->style);
	if ((self->style == TE_PARASITE_ATTACK) || (self->style==TE_MEDIC_CABLE_ATTACK) ||
	   (self->style == TE_HEATBEAM)        || (self->style==TE_MONSTER_HEATBEAM)   ||
	   (self->style == TE_GRAPPLE_CABLE)                                             )
		gi.WriteShort(self-g_edicts);
	gi.WritePosition(self->s.origin);
	gi.WritePosition(target->s.origin);
	if (self->style == TE_GRAPPLE_CABLE) {
		gi.WritePosition(vec3_origin);
	}
	gi.multicast(self->s.origin, MULTICAST_PVS);

	if (level.num_reflectors)
	{
		if ((self->style == TE_RAILTRAIL) || (self->style == TE_BUBBLETRAIL) ||
		   (self->style == TE_BFG_LASER) || (self->style == TE_DEBUGTRAIL)  ||
		   (self->style == TE_BUBBLETRAIL2))
		   ReflectTrail(self->style,self->s.origin,target->s.origin);
	}
}
//===========================================================================
/* TE_LIGHTNING   33 Lightning bolt

  Similar but slightly different syntax to trail stuff */
void target_effect_lightning(edict_t *self, edict_t *activator)
{
	edict_t	*target;

	if (!self->target) return;
	target = G_Find(NULL,FOFS(targetname),self->target);
	if (!target) return;

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (self->style);
	gi.WriteShort (target - g_edicts);		// destination entity
	gi.WriteShort (self - g_edicts);		// source entity
	gi.WritePosition (target->s.origin);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);
}
//===========================================================================
/*
Spawns sparks of (type) from (start) in direction of (movdir) and
Broadcasts to all in Potentially Visible Set from vector (origin)

  TE_GUNSHOT           0 Spawns a grey splash of particles, with a bullet puff
  TE_BLOOD             1 Spawns a spurt of red blood
  TE_BLASTER           2 Spawns a blaster sparks
  TE_SHOTGUN           4 Spawns a small grey splash of spark particles, with a bullet puff
  TE_SPARKS            9 Spawns a red/gold splash of spark particles
  TE_SCREEN_SPARKS    12 Spawns a large green/white splash of sparks
  TE_SHIELD_SPARKS    13 Spawns a large blue/violet splash of sparks
  TE_BULLET_SPARKS    14 Same as TE_SPARKS, with a bullet puff and richochet sound
  TE_GREENBLOOD       26 Spurt of green (actually kinda yellow) blood
  TE_BLUEHYPERBLASTER 27 NOT IMPLEMENTED
  TE_BLASTER2         30 Green/white sparks with a yellow/white flash
  TE_MOREBLOOD        42 
  TE_HEATBEAM_SPARKS  43
  TE_HEATBEAM_STEAM   44
  TE_CHAINFIST_SMOKE  45 
  TE_ELECTRIC_SPARKS  46
  TE_FLECHETTE        55
*/
//======================================================
void target_effect_sparks (edict_t *self, edict_t *activator)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(self->style);
	gi.WritePosition(self->s.origin);
	if (self->style != TE_CHAINFIST_SMOKE) 
		gi.WriteDir(self->movedir);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	if (level.num_reflectors)
		ReflectSparks(self->style,self->s.origin,self->movedir);
}

//======================================================
/*
Spawns a (type) effect at (start} and Broadcasts to all in the
Potentially Hearable set from vector (origin)

  TE_EXPLOSION1               5 airburst
  TE_EXPLOSION2               6 ground burst
  TE_ROCKET_EXPLOSION         7 rocket explosion
  TE_GRENADE_EXPLOSION        8 grenade explosion
  TE_ROCKET_EXPLOSION_WATER  17 underwater rocket explosion
  TE_GRENADE_EXPLOSION_WATER 18 underwater grenade explosion
  TE_BFG_EXPLOSION           20 BFG explosion sprite
  TE_BFG_BIGEXPLOSION        21 BFG particle explosion
  TE_BOSSTPORT               22 
  TE_PLASMA_EXPLOSION        28
  TE_PLAIN_EXPLOSION         35
  TE_TRACKER_EXPLOSION       47
  TE_TELEPORT_EFFECT	     48
  TE_DBALL_GOAL              49 Identical to TE_TELEPORT_EFFECT?
  TE_NUKEBLAST               51 
  TE_WIDOWSPLASH             52
  TE_EXPLOSION1_BIG          53  Works, but requires Rogue models/objects/r_explode2
  TE_EXPLOSION1_NP           54
*/
//==============================================================================
void target_effect_explosion (edict_t *self, edict_t *activator)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(self->style);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS);

	if (level.num_reflectors)
		ReflectExplosion (self->style, self->s.origin);

}
//===============================================================================
/*  TE_TUNNEL_SPARKS    29 
    Similar to other splash effects, but Xatrix does some funky things with
	the origin so we'll do the same */

void target_effect_tunnel_sparks (edict_t *self, edict_t *activator)
{
	vec3_t	origin;
	int		i;

	VectorCopy(self->s.origin,origin);
	for (i=0; i<self->count; i++)
	{
		origin[2] += (self->speed * 0.01) * (i + random());
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (self->style);
		gi.WriteByte (1);
		gi.WritePosition (origin);
		gi.WriteDir (vec3_origin);
		gi.WriteByte (self->sounds + (rand()&7));  // color
		gi.multicast (self->s.origin, MULTICAST_PVS);
	}
}
//===============================================================================
/*  TE_WIDOWBEAMOUT     50
*/
void target_effect_widowbeam(edict_t *self, edict_t *activator)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_WIDOWBEAMOUT);
	gi.WriteShort (20001);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);
}
//===============================================================================

void target_effect_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->spawnflags & 1) {
		// currently looped on - turn it off
		self->spawnflags &= ~1;
		self->spawnflags |= 2;
		self->nextthink = 0;
		return;
	}
	if (self->spawnflags & 2) {
		// currently looped off - turn it on
		self->spawnflags &= ~2;
		self->spawnflags |= 1;
		self->nextthink = level.time + self->wait;
	}
	if (self->spawnflags & 4) {
		// "if_moving" set. If movewith target isn't moving,
		// don't play
		edict_t	*mover;
		if (!self->movewith) return;
		mover = G_Find(NULL,FOFS(targetname),self->movewith);
		if (!mover) return;
		if (!VectorLength(mover->velocity)) return;
	}
	self->play(self,activator);
}
void target_effect_think(edict_t *self)
{
	self->play(self,NULL);
	self->nextthink = level.time + self->wait;
}
//===============================================================================
void SP_target_effect (edict_t *self)
{
	if (self->movewith)
		self->movetype = MOVETYPE_PUSH;
	else
		self->movetype = MOVETYPE_NONE;

	switch (self->style ) {
	case TE_FLASHLIGHT:
		self->play = target_effect_at;
		break;
	case TE_STEAM:
		self->play = target_effect_steam;
		G_SetMovedir (self->s.angles, self->movedir);
		if (!self->count)
			self->count = 32;
		if (!self->sounds)
			self->sounds = 8;
		if (!self->speed)
			self->speed = 75;
		break;
	case TE_SPLASH:
	case TE_LASER_SPARKS:
	case TE_WELDING_SPARKS:
		self->play = target_effect_splash;
		G_SetMovedir (self->s.angles, self->movedir);
		if (!self->count)
			self->count = 32;
		break;
	case TE_RAILTRAIL:
	case TE_BUBBLETRAIL:
	case TE_PARASITE_ATTACK:
	case TE_MEDIC_CABLE_ATTACK:
	case TE_BFG_LASER:
	case TE_GRAPPLE_CABLE:
	case TE_DEBUGTRAIL:
	case TE_HEATBEAM:
	case TE_MONSTER_HEATBEAM:
	case TE_BUBBLETRAIL2:
		if (!self->target) {
			gi.dprintf("%s at %s with style=%d needs a target\n",self->classname,vtos(self->s.origin),self->style);
			G_FreeEdict(self);
		} else
			self->play = target_effect_trail;
		break;
	case TE_LIGHTNING:
		if (!self->target) {
			gi.dprintf("%s at %s with style=%d needs a target\n",self->classname,vtos(self->s.origin),self->style);
			G_FreeEdict(self);
		} else
			self->play = target_effect_lightning;
		break;
	case TE_GUNSHOT:
	case TE_BLOOD:
	case TE_BLASTER:
	case TE_SHOTGUN:
	case TE_SPARKS:
	case TE_SCREEN_SPARKS:
	case TE_SHIELD_SPARKS:
	case TE_BULLET_SPARKS:
	case TE_GREENBLOOD:
	case TE_BLASTER2:
	case TE_MOREBLOOD:
	case TE_HEATBEAM_SPARKS:
	case TE_HEATBEAM_STEAM:
	case TE_CHAINFIST_SMOKE:
	case TE_ELECTRIC_SPARKS:
	case TE_FLECHETTE:
		self->play = target_effect_sparks;
		G_SetMovedir (self->s.angles, self->movedir);
		break;
	case TE_EXPLOSION1:
	case TE_EXPLOSION2:
	case TE_ROCKET_EXPLOSION:
	case TE_GRENADE_EXPLOSION:
	case TE_ROCKET_EXPLOSION_WATER:
	case TE_GRENADE_EXPLOSION_WATER:
	case TE_BFG_EXPLOSION:
	case TE_BFG_BIGEXPLOSION:
	case TE_BOSSTPORT:
	case TE_PLASMA_EXPLOSION:
	case TE_PLAIN_EXPLOSION:
	case TE_TRACKER_EXPLOSION:
	case TE_TELEPORT_EFFECT:
	case TE_DBALL_GOAL:
	case TE_NUKEBLAST:
	case TE_WIDOWSPLASH:
	case TE_EXPLOSION1_BIG:
	case TE_EXPLOSION1_NP:
		self->play = target_effect_explosion;
		break;
	case TE_TUNNEL_SPARKS:
		if (!self->count)
			self->count = 32;
		if (!self->sounds)
			self->sounds = 116; // Light blue, same color used by Xatrix
		self->play = target_effect_tunnel_sparks;
		break;
	case TE_WIDOWBEAMOUT:
		self->play = target_effect_widowbeam;
		G_SetMovedir (self->s.angles, self->movedir);
		break;
	default:
		gi.dprintf("%s at %s: bad style %d\n",self->classname,vtos(self->s.origin),self->style);
	}
	self->use = target_effect_use;
	self->think = target_effect_think;
	if (self->spawnflags & 1)
		self->nextthink = level.time + 1;

}

/*=====================================================================================
 TARGET_ATTRACTOR - pulls target entity towards its origin
 target     - Targetname of entity to attract. Ignored if PLAYER spawnflag is set
 pathtarget - Entity or entities to "use" when distance criteria is met.
 speed      - Minimum speed to pull target with. Must use a value > sv_gravity/10 
              to overcome gravity when pulling up.
 distance   - When target is within "distance" units of target_attractor, attraction
              is shut off. Use a value < 0 to hold target in place. 0 will be reset
              to 1.
 sounds     - effect to use. ONLY VALID for PLAYER or MONSTER, not target. If sounds
              is non-zero, SINGLE_TARGET and SIGHT are automatically set
      0 = none
      1 = medic cable
      2 = green laser
              
 Spawnflags:   1 - START_ON
               2 - PLAYER (attract player, ignore "target"
               4 - NO_GRAVITY - turns off gravity for target. W/O this flag you'll
                   get an annoying jitter when pulling players up.
               8 - MONSTER - attract ALL monsters, ignore "target"
              16 - SIGHT - must have LOS to target to attract it
              32 - SINGLE_TARGET - will select best target
              64 - PATHTARGET_FIRE - used internally only
=======================================================================================*/
#define ATTRACTOR_ON          1
#define ATTRACTOR_PLAYER      2
#define ATTRACTOR_NO_GRAVITY  4
#define ATTRACTOR_MONSTER     8
#define ATTRACTOR_SIGHT      16
#define ATTRACTOR_SINGLE     32
#define ATTRACTOR_PATHTARGET 64

void target_attractor_think_single(edict_t *self)
{
	edict_t	*ent, *target, *previous_target;
	trace_t	tr;
	vec3_t	dir, targ_org;
	vec_t	dist, speed;
	vec_t	best_dist;
	vec3_t	forward, right;
	int		i;
	int		num_targets = 0;
	
	if (!(self->spawnflags & ATTRACTOR_ON)) return;

	previous_target = self->target_ent;
	target      = NULL;
	best_dist   = 8192;

	if (self->spawnflags & ATTRACTOR_PLAYER) {
		for(i=1, ent=&g_edicts[i]; i<=game.maxclients; i++, ent++) {
			if (!ent->inuse) continue;
			if (ent->health <= 0) continue;
			num_targets++;
			VectorSubtract(self->s.origin,ent->s.origin,dir);
			dist = VectorLength(dir);
			if (dist > self->moveinfo.distance) continue;
			if (self->spawnflags & ATTRACTOR_SIGHT) {
				tr = gi.trace(self->s.origin,vec3_origin,vec3_origin,ent->s.origin,NULL,MASK_OPAQUE | MASK_SHOT);
				if (tr.ent != ent) continue;
			}
			if (dist < best_dist) {
				best_dist = dist;
				target = ent;
			}
		}
	}
	if (self->spawnflags & ATTRACTOR_MONSTER) {
		for(i=1, ent=&g_edicts[i]; i<=globals.num_edicts; i++, ent++) {
			if (!ent->inuse) continue;
			if (ent->health <= 0) continue;
			if (!(ent->svflags & SVF_MONSTER)) continue;
			num_targets++;
			VectorSubtract(self->s.origin,ent->s.origin,dir);
			dist = VectorLength(dir);
			if (dist > self->moveinfo.distance) continue;
			if (self->spawnflags & ATTRACTOR_SIGHT) {
				tr = gi.trace(self->s.origin,vec3_origin,vec3_origin,ent->s.origin,NULL,MASK_OPAQUE | MASK_SHOT);
				if (tr.ent != ent) continue;
			}
			if (dist < best_dist) {
				best_dist = dist;
				target = ent;
			}
		}
	}
	if (!(self->spawnflags & (ATTRACTOR_PLAYER | ATTRACTOR_MONSTER))) {
		ent = G_Find(NULL,FOFS(targetname),self->target);
		while(ent) {
			if (!ent->inuse) continue;
			num_targets++;
			VectorAdd(ent->s.origin,ent->origin_offset,targ_org);
			VectorSubtract(self->s.origin,targ_org,dir);
			dist = VectorLength(dir);
			if (dist > self->moveinfo.distance) continue;
			if (self->spawnflags & ATTRACTOR_SIGHT) {
				tr = gi.trace(self->s.origin,vec3_origin,vec3_origin,targ_org,NULL,MASK_OPAQUE | MASK_SHOT);
				if (tr.ent != ent) continue;
			}
			if (dist < best_dist) {
				best_dist = dist;
				target = ent;
			}
			ent = G_Find(ent,FOFS(targetname),self->target);
		}
	}
	self->target_ent = target;
	if (!target) {
		if (num_targets > 0) self->nextthink = level.time + FRAMETIME;
		return;
	}

	if (target != previous_target)
		self->moveinfo.speed = 0;

	if (self->moveinfo.speed != self->speed) {
		if (self->speed > 0)
			self->moveinfo.speed = min(self->speed, self->moveinfo.speed + self->accel);
		else
			self->moveinfo.speed = max(self->speed, self->moveinfo.speed + self->accel);
	}

	VectorAdd(target->s.origin,target->origin_offset,targ_org);
	VectorSubtract(self->s.origin,targ_org,dir);
	dist = VectorLength(dir);
	if (readout->value) gi.dprintf("distance=%g, pull speed=%g\n",dist,self->moveinfo.speed);

	if ((self->pathtarget) && (self->spawnflags & ATTRACTOR_PATHTARGET))
	{
		if (dist == 0) {
			// fire pathtarget when close
			ent = G_Find(NULL,FOFS(targetname),self->pathtarget);
			while(ent) {
				if (ent->use)
					ent->use(ent,self,self);
				ent = G_Find(ent,FOFS(targetname),self->pathtarget);
			}
			self->spawnflags &= ~ATTRACTOR_PATHTARGET;
		}
	}
	VectorNormalize(dir);
	speed = VectorNormalize(target->velocity);
	speed = max(fabs(self->moveinfo.speed),speed);
	if (self->moveinfo.speed < 0) speed = -speed;
	if (speed > dist*10) {
		speed = dist*10;
		VectorScale(dir,speed,target->velocity);
		// if NO_GRAVITY is NOT set, and target would normally be affected by gravity,
		// counteract gravity during the last move
		if ( !(self->spawnflags & ATTRACTOR_NO_GRAVITY) ) {
			if ( (target->movetype == MOVETYPE_BOUNCE  ) ||
				(target->movetype == MOVETYPE_PUSHABLE) ||
				(target->movetype == MOVETYPE_STEP    ) ||
				(target->movetype == MOVETYPE_TOSS    ) ||
				(target->movetype == MOVETYPE_DEBRIS  )   ) {
				target->velocity[2] += target->gravity * sv_gravity->value * FRAMETIME;
			}
		}
	} else
		VectorScale(dir,speed,target->velocity);
	// Add attractor velocity in case it's a movewith deal
	VectorAdd(target->velocity,self->velocity,target->velocity);
	if (target->client) {
		float	scale;
		if (target->groundentity || target->waterlevel > 1) {
			if (target->groundentity)
				scale = 0.75;
			else
				scale = 0.375;
			// Players - add movement stuff so he MAY be able to escape
			AngleVectors (target->client->v_angle, forward, right, NULL);
			for (i=0 ; i<3 ; i++)
				target->velocity[i] += scale * forward[i] * target->client->ucmd.forwardmove +
				scale * right[i]   * target->client->ucmd.sidemove; 
			target->velocity[2] += scale * target->client->ucmd.upmove;
		}
	}
	// If target is on the ground and attractor is overhead, give 'em a little nudge.
	// This is only really necessary for players
	if (target->groundentity && (self->s.origin[2] > target->absmax[2])) {
		target->s.origin[2] += 1;
		target->groundentity = NULL;
	}

	if (self->sounds) {
		vec3_t	new_origin;

		if (target->client)
			VectorCopy(target->s.origin,new_origin);
		else
			VectorMA(targ_org,FRAMETIME,target->velocity,new_origin);
		
		switch(self->sounds) {
		case 1:
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_MEDIC_CABLE_ATTACK);
			gi.WriteShort(self-g_edicts);
			gi.WritePosition(self->s.origin);
			gi.WritePosition(new_origin);
			gi.multicast(self->s.origin, MULTICAST_PVS);
			break;
		case 2:
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_BFG_LASER);
			gi.WritePosition(self->s.origin);
			gi.WritePosition(new_origin);
			gi.multicast(self->s.origin, MULTICAST_PVS);
		}
	}

	
	if (self->spawnflags & ATTRACTOR_NO_GRAVITY)
		target->gravity_debounce_time = level.time + 2*FRAMETIME;
	gi.linkentity(target);
	
	if (!num_targets) {
		// shut 'er down
		self->spawnflags &= ~ATTRACTOR_ON;
	} else {
		self->nextthink = level.time + FRAMETIME;
	}
}

void target_attractor_think(edict_t *self)
{
	edict_t	*ent, *target;
	trace_t	tr;
	vec3_t	dir, targ_org;
	vec_t	dist, speed;
	vec3_t	forward, right;
	int		i;
	int		ent_start;
	int		num_targets = 0;

	if (!(self->spawnflags & ATTRACTOR_ON)) return;

	if (self->moveinfo.speed != self->speed) {
		if (self->speed > 0)
			self->moveinfo.speed = min(self->speed, self->moveinfo.speed + self->accel);
		else
			self->moveinfo.speed = max(self->speed, self->moveinfo.speed + self->accel);
	}

	target = NULL;
	ent_start = 1;
	while(true)
	{
		if (self->spawnflags & (ATTRACTOR_PLAYER | ATTRACTOR_MONSTER))
		{
			target = NULL;
			for(i=ent_start, ent=&g_edicts[ent_start];i<globals.num_edicts && !target; i++, ent++)
			{
				if ((self->spawnflags & ATTRACTOR_PLAYER) && ent->client && ent->inuse)
				{
					target = ent;
					ent_start = i+1;
					continue;
				}
				if ((self->spawnflags & ATTRACTOR_MONSTER) && (ent->svflags & SVF_MONSTER) && (ent->inuse))
				{
					target = ent;
					ent_start = i+1;
				}
			}
		} else
			target = G_Find(target,FOFS(targetname),self->target);
		if (!target) break;
		if (!target->inuse) continue;
		if ( ((target->client) || (target->svflags & SVF_MONSTER)) && (target->health <= 0)) continue;
		num_targets++;
		
		VectorAdd(target->s.origin,target->origin_offset,targ_org);
		VectorSubtract(self->s.origin,targ_org,dir);
		dist = VectorLength(dir);

		if (self->spawnflags & ATTRACTOR_SIGHT) {
			tr = gi.trace(self->s.origin,vec3_origin,vec3_origin,target->s.origin,NULL,MASK_OPAQUE | MASK_SHOT);
			if (tr.ent != target) continue;
		}

		if (readout->value) gi.dprintf("distance=%g, pull speed=%g\n",dist,self->moveinfo.speed);
		if (dist > self->moveinfo.distance)
			continue;
		
		if ((self->pathtarget) && (self->spawnflags & ATTRACTOR_PATHTARGET))
		{
			if (dist == 0) {
				// fire pathtarget when close
				ent = G_Find(NULL,FOFS(targetname),self->pathtarget);
				while(ent) {
					if (ent->use)
						ent->use(ent,self,self);
					ent = G_Find(ent,FOFS(targetname),self->pathtarget);
				}
				self->spawnflags &= ~ATTRACTOR_PATHTARGET;
			}
		}
		VectorNormalize(dir);
		speed = VectorNormalize(target->velocity);
		speed = max(fabs(self->moveinfo.speed),speed);
		if (self->moveinfo.speed < 0) speed = -speed;
		if (speed > dist*10) {
			speed = dist*10;
			VectorScale(dir,speed,target->velocity);
			// if NO_GRAVITY is NOT set, and target would normally be affected by gravity,
			// counteract gravity during the last move
			if ( !(self->spawnflags & ATTRACTOR_NO_GRAVITY) ) {
				if ( (target->movetype == MOVETYPE_BOUNCE  ) ||
					(target->movetype == MOVETYPE_PUSHABLE) ||
					(target->movetype == MOVETYPE_STEP    ) ||
					(target->movetype == MOVETYPE_TOSS    ) ||
					(target->movetype == MOVETYPE_DEBRIS  )   ) {
					target->velocity[2] += target->gravity * sv_gravity->value * FRAMETIME;
				}
			}
		} else
			VectorScale(dir,speed,target->velocity);
		// Add attractor velocity in case it's a movewith deal
		VectorAdd(target->velocity,self->velocity,target->velocity);
		if (target->client) {
			float	scale;
			if (target->groundentity || target->waterlevel > 1) {
				if (target->groundentity)
					scale = 0.75;
				else
					scale = 0.375;
				// Players - add movement stuff so he MAY be able to escape
				AngleVectors (target->client->v_angle, forward, right, NULL);
				for (i=0 ; i<3 ; i++)
					target->velocity[i] += scale * forward[i] * target->client->ucmd.forwardmove +
					                       scale * right[i]   * target->client->ucmd.sidemove; 
				target->velocity[2] += scale * target->client->ucmd.upmove;
			}
		}
		// If target is on the ground and attractor is overhead, give 'em a little nudge.
		// This is only really necessary for players
		if (target->groundentity && (self->s.origin[2] > target->absmax[2])) {
			target->s.origin[2] += 1;
			target->groundentity = NULL;
		}
		if (self->spawnflags & ATTRACTOR_NO_GRAVITY)
			target->gravity_debounce_time = level.time + 2*FRAMETIME;
		gi.linkentity(target);
	}
	if (!num_targets) {
		// shut 'er down
		self->spawnflags &= ~ATTRACTOR_ON;
	} else {
		self->nextthink = level.time + FRAMETIME;
	}
}

void use_target_attractor(edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->spawnflags & ATTRACTOR_ON) {
		self->count--;
		if (!self->count) {
			self->think = G_FreeEdict;
			self->nextthink = level.time + 1;
		}
		else
		{
			self->spawnflags &= ~ATTRACTOR_ON;
			self->s.sound = 0;
			self->target_ent  = NULL;
			self->nextthink   = 0;
		}
	} else {
		self->spawnflags |= (ATTRACTOR_ON + ATTRACTOR_PATHTARGET);
		self->s.sound = self->noise_index;
	#ifdef LOOP_SOUND_ATTENUATION
		self->s.attenuation = self->attenuation;
	#endif

		if (self->spawnflags & ATTRACTOR_SINGLE)
			self->think = target_attractor_think_single;
		else
			self->think = target_attractor_think;
		self->moveinfo.speed = 0;
		gi.linkentity(self);
		self->think(self);
	}
}

void SP_target_attractor(edict_t *self)
{
	if (!self->target && !(self->spawnflags & ATTRACTOR_PLAYER) &&
		!(self->spawnflags & ATTRACTOR_MONSTER)) {
		gi.dprintf("target_attractor without a target at %s\n",vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}
	if (self->sounds) {
//		if ((self->spawnflags & ATTRACTOR_PLAYER) || (self->spawnflags & ATTRACTOR_MONSTER)) {
			self->spawnflags |= (ATTRACTOR_SIGHT | ATTRACTOR_SINGLE);
//		} else {
//			gi.dprintf("Target_attractor sounds key is only valid\n"
//				       "for PLAYER or MONSTER. Setting sounds=0\n");
//		}
	}
	if (st.distance)
		self->moveinfo.distance = st.distance;
	else
		self->moveinfo.distance = 8192;

	self->solid = SOLID_NOT;
	if (self->movewith)
		self->movetype = MOVETYPE_PUSH;
	else
		self->movetype = MOVETYPE_NONE;
	self->use = use_target_attractor;

	if (st.noise)
		self->noise_index = gi.soundindex(st.noise);
	else
		self->noise_index = 0;

	if (!self->speed)
		self->speed = 100;

	if (!self->accel)
		self->accel = self->speed;
	else {
		self->accel *= 0.1;
		if (self->accel > self->speed)
			self->accel = self->speed;
	}

	if (self->spawnflags & ATTRACTOR_ON) {
		if (self->spawnflags & ATTRACTOR_SINGLE)
			self->think = target_attractor_think_single;
		else
			self->think = target_attractor_think;
		if (self->sounds)
			self->nextthink = level.time + 2*FRAMETIME;
		else
			self->think(self);
	}
}

/*===================================================================
 TARGET_CD
 Play the CD track specified by the "sounds" value, looping the
 track "dmg" times. This does NOT override player's option to
 disable CD music, and of course does nothing if a music CD is not
 in place. If "dmg" is not specified, track is looped cd_loopcount
 (default=4) times.
===================================================================*/
void use_target_CD (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->musictrack && strlen(self->musictrack))
		gi.configstring (CS_CDTRACK, self->musictrack);
	else
		gi.configstring (CS_CDTRACK, va("%d",self->sounds));
	if ((self->dmg > 0) && (!deathmatch->value) && (!coop->value))
		stuffcmd(&g_edicts[1],va("cd_loopcount %d\n",self->dmg));

	self->count--;
	if (!self->count) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}
}

void SP_target_CD (edict_t *self)
{
	self->use = use_target_CD;
	if (!self->dmg)
		self->dmg = lazarus_cd_loop->value;
	gi.linkentity(self);
}
/*===================================================================
 TARGET_MONITOR
 Move the player's viewpoint to the target_monitor origin, 
 gives the target_monitor angles to the player, and freezes him
 for "wait" seconds (default=3). If wait < 0, target_monitor
 must be targeted a second time to free the player.
===================================================================*/
#define SF_MONITOR_CHASECAM       1
#define SF_MONITOR_EYEBALL        2		// Same as CHASECAM, but viewpoint is at the target
										// entity's viewheight, and target entity is made
										// invisible while in use
#define SF_MONITOR_CAMERAEFFECT   4		// Knightmare- camera effect
#define SF_MONITOR_LETTERBOX      8		// Knightmare- letterboxing

void target_monitor_off (edict_t *self)
{
	int		i;
	edict_t	*faker;
	edict_t	*player;

	player = self->child;
	if (!player) return;

	if (self->spawnflags & SF_MONITOR_EYEBALL)
	{
		if (self->target_ent)
			self->target_ent->svflags &= ~SVF_NOCLIENT;
	}
	faker = player->client->camplayer;
	VectorCopy(faker->s.origin,player->s.origin);
//	free(faker->client); 
	gi.TagFree(faker->client); 
	G_FreeEdict (faker); 
	player->client->ps.pmove.origin[0] = player->s.origin[0]*8;
	player->client->ps.pmove.origin[1] = player->s.origin[1]*8;
	player->client->ps.pmove.origin[2] = player->s.origin[2]*8;
	for (i=0 ; i<3 ; i++)
		player->client->ps.pmove.delta_angles[i] = 
			ANGLE2SHORT(player->client->org_viewangles[i] - player->client->resp.cmd_angles[i]);
	VectorCopy(player->client->org_viewangles, player->client->resp.cmd_angles);
	VectorCopy(player->client->org_viewangles, player->s.angles);
	VectorCopy(player->client->org_viewangles, player->client->ps.viewangles);
	VectorCopy(player->client->org_viewangles, player->client->v_angle);
	
	player->client->ps.gunindex        = gi.modelindex(player->client->pers.weapon->view_model);
	player->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
	player->client->ps.pmove.pm_type   = PM_NORMAL;
#ifdef KMQUAKE2_ENGINE_MOD
	player->client->ps.rdflags		  &= ~(RDF_CAMERAEFFECT|RDF_LETTERBOX); // Knightmare- letterboxing
#endif
	player->svflags                   &= ~SVF_NOCLIENT;
	player->clipmask                   = MASK_PLAYERSOLID;
	player->solid                      = SOLID_BBOX;
	player->viewheight                 = 22;
	player->client->camplayer          = NULL;
	player->target_ent                 = NULL;
	gi.unlinkentity(player);
	KillBox(player);
	gi.linkentity(player);

	if (self->noise_index)
		gi.sound (player, CHAN_VOICE, self->noise_index, 1, ATTN_NORM, 0);

	// if we were previously in third person view, restore it
	if (tpp->value)
		Cmd_Chasecam_Toggle (player);

//CW+++  Switch crosshair and HUD back on again.
	stuffcmd(player, va("crosshair %f\n", fCrosshair));
	gi.configstring (CS_STATUSBAR, single_statusbar);
//CW---

	self->child = NULL;
	gi.linkentity(self);

	self->count--;
	if (!self->count) {
		self->use = NULL;
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}
}

void target_monitor_move (edict_t *self)
{
	// "chase cam"
	trace_t	trace;
	vec3_t	forward, o, goal;

	if (!self->target_ent || !self->target_ent->inuse)
	{
		if (self->wait)
		{
			self->think = target_monitor_off;
			self->nextthink = self->monsterinfo.attack_finished;
		}
		return;
	}

	if ( (self->monsterinfo.attack_finished > 0) &&
		(level.time > self->monsterinfo.attack_finished))
	{
		target_monitor_off(self);
		return;
	}

	AngleVectors(self->target_ent->s.angles,forward,NULL,NULL);
	VectorMA(self->target_ent->s.origin, -self->moveinfo.distance, forward, o);

	o[2] += self->viewheight;

	VectorSubtract(o,self->s.origin,o);
	VectorMA(self->s.origin,0.2,o,o);

	trace = gi.trace(self->target_ent->s.origin, NULL, NULL, o, self, MASK_SOLID);
	VectorCopy(trace.endpos, goal);
	VectorMA(goal, 2, forward, goal);

	// pad for floors and ceilings
	VectorCopy(goal, o);
	o[2] += 6;
	trace = gi.trace(goal, NULL, NULL, o, self, MASK_SOLID);
	if (trace.fraction < 1) {
		VectorCopy(trace.endpos, goal);
		goal[2] -= 6;
	}

	VectorCopy(goal, o);
	o[2] -= 6;
	trace = gi.trace(goal, NULL, NULL, o, self, MASK_SOLID);
	if (trace.fraction < 1) {
		VectorCopy(trace.endpos, goal);
		goal[2] += 6;
	}

	VectorCopy(goal, self->s.origin);
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}
void use_target_monitor (edict_t *self, edict_t *other, edict_t *activator)
{
	int			i;
	edict_t		*faker;
	edict_t		*monster;
	gclient_t	*cl;

	if (activator && !activator->client)	//CW - added activator check
		return;

//CW++  Remove crosshair and HUD whilst looking through monitor.
	fCrosshair = crosshair->value;
	stuffcmd(activator, "crosshair 0\n");
	gi.configstring (CS_STATUSBAR, NULL);
//CW--

	if (self->child)
	{
		if (self->wait < 0)
			target_monitor_off(self);
		return;
	}

	if (self->target)
		self->target_ent = G_Find(NULL,FOFS(targetname),self->target);

	// if this is a CHASE_CAM target_monitor and the target no longer
	// exists, remove this target_monitor and exit
	if (self->spawnflags & SF_MONITOR_CHASECAM)
	{
		if (!self->target_ent || !self->target_ent->inuse)
		{
			G_FreeEdict(self);
			return;
		}
	}
	// save current viewangles
	VectorCopy(activator->client->v_angle,activator->client->org_viewangles);

	// create a fake player to stand in real player's position
	faker = activator->client->camplayer = G_Spawn();
	faker->s.frame = activator->s.frame; 
	VectorCopy (activator->s.origin, faker->s.origin); 
	VectorCopy (activator->velocity, faker->velocity); 
	VectorCopy (activator->s.angles, faker->s.angles); 
	faker->s = activator->s;
	faker->takedamage   = DAMAGE_NO;					// so monsters won't attack
	faker->flags       |= FL_NOTARGET;					// ... just to make sure
//	faker->movetype     = MOVETYPE_WALK;
	faker->movetype     = MOVETYPE_TOSS;
	faker->groundentity = activator->groundentity;
	faker->viewheight   = activator->viewheight;
	faker->inuse        = true;
	faker->classname    = "camplayer";
	faker->mass         = activator->mass;
	faker->solid        = SOLID_BBOX;
	faker->deadflag     = DEAD_NO;
	faker->clipmask     = MASK_PLAYERSOLID;
	faker->health       = 100000;						// invulnerable
	faker->light_level  = activator->light_level;
	faker->think        = faker_animate;
	faker->nextthink    = level.time + FRAMETIME;
	VectorCopy(activator->mins,faker->mins);
	VectorCopy(activator->maxs,faker->maxs);
    // create a client so you can pick up items/be shot/etc while in camera
//	cl = (gclient_t *) malloc(sizeof(gclient_t)); 
	cl = (gclient_t *) gi.TagMalloc(sizeof(gclient_t), TAG_LEVEL); 
	faker->client = cl; 
	faker->target_ent = activator;
	gi.linkentity (faker); 

	if (self->target_ent && self->target_ent->inuse)
	{
		if (self->spawnflags & SF_MONITOR_EYEBALL)
			VectorCopy(self->target_ent->s.angles, activator->client->ps.viewangles);
		else
		{
			vec3_t	dir;
			VectorSubtract(self->target_ent->s.origin,self->s.origin,dir);
			vectoangles(dir,activator->client->ps.viewangles);
		}
	}
	else
		VectorCopy (self->s.angles, activator->client->ps.viewangles);

	VectorCopy (self->s.origin, activator->s.origin);
	activator->client->ps.pmove.origin[0] = self->s.origin[0]*8;
	activator->client->ps.pmove.origin[1] = self->s.origin[1]*8;
	activator->client->ps.pmove.origin[2] = self->s.origin[2]*8;
	activator->client->ps.pmove.pm_type = PM_FREEZE;
#ifdef KMQUAKE2_ENGINE_MOD
	// Knightmare- camera effect and letterboxing
	if (self->spawnflags & SF_MONITOR_CAMERAEFFECT)
		activator->client->ps.rdflags |= RDF_CAMERAEFFECT;
	if (self->spawnflags & SF_MONITOR_LETTERBOX)
		activator->client->ps.rdflags |= RDF_LETTERBOX;
#endif
	activator->svflags                 |= SVF_NOCLIENT;
	activator->solid                    = SOLID_NOT;
	activator->viewheight               = 0;
	if (activator->client->chasetoggle)
	{
		Cmd_Chasecam_Toggle (activator);
		activator->client->pers.chasetoggle = 1;
	}
	else
		activator->client->pers.chasetoggle = 0;
	activator->clipmask = 0;
	VectorClear(activator->velocity);
	activator->client->ps.gunindex = 0; 
	activator->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
	gi.linkentity(activator);

	gi.unlinkentity(faker);
	KillBox(faker);
	gi.linkentity(faker);

	// check to see if player is the enemy of any monster.
	for (i=maxclients->value+1, monster=g_edicts+i; i<globals.num_edicts; i++, monster++)
	{
		if (!monster->inuse) continue;
		if (!(monster->svflags & SVF_MONSTER)) continue;
		if (monster->enemy == activator)
		{
			monster->enemy = NULL;
			monster->oldenemy = NULL;
			if (monster->goalentity == activator)
				monster->goalentity = NULL;
			if (monster->movetarget == activator)
				monster->movetarget = NULL;
			monster->monsterinfo.attack_finished = level.time + 1;
			FindTarget(monster);
		}
	}

	activator->target_ent = self;
	self->child = activator;

	if (self->noise_index)
		gi.sound (activator, CHAN_VOICE, self->noise_index, 1, ATTN_NORM, 0);

	if (self->spawnflags & SF_MONITOR_CHASECAM)
	{
		if (self->wait > 0)
			self->monsterinfo.attack_finished = level.time + self->wait;
		else
			self->monsterinfo.attack_finished = 0;

		if (self->spawnflags & SF_MONITOR_EYEBALL)
		{
			self->viewheight = self->target_ent->viewheight;
			self->target_ent->svflags |= SVF_NOCLIENT;
		}
		VectorCopy(self->target_ent->s.origin,self->s.origin);
		self->think = target_monitor_move;
		self->think(self);
	}
	else if (self->wait > 0)
	{
		self->think = target_monitor_off;
		self->nextthink = level.time + self->wait;
	}
}

void SP_target_monitor (edict_t *self)
{
	char	buffer[MAX_QPATH];

	if (!self->wait)
		self->wait = 3;
	self->use = use_target_monitor;
	self->movetype = MOVETYPE_NOCLIP;
	if (st.noise)
	{
		if (!strstr (st.noise, ".wav"))
			Com_sprintf (buffer, sizeof(buffer), "%s.wav", st.noise);
		else
			strncpy (buffer, st.noise, sizeof(buffer));
		self->noise_index = gi.soundindex (buffer);
	}

	if (self->spawnflags & SF_MONITOR_EYEBALL)
		self->spawnflags |= SF_MONITOR_CHASECAM;

	if (self->spawnflags & SF_MONITOR_CHASECAM)
	{	// chase cam
		if (self->spawnflags & SF_MONITOR_EYEBALL)
		{
			self->moveinfo.distance = 0;
			self->viewheight = 0;
		}
		else
		{
			if (st.distance)
				self->moveinfo.distance = st.distance;
			else
				self->moveinfo.distance = 128;

			if (st.height)
				self->viewheight = st.height;
			else
				self->viewheight = 16;
		}

		// MUST have target
		if (!self->target)
		{
			gi.dprintf("CHASECAM target_monitor with no target at %s\n",vtos(self->s.origin));
			self->spawnflags &= ~(SF_MONITOR_CHASECAM | SF_MONITOR_EYEBALL);
		}
		else if (self->movewith)
		{
			gi.dprintf("CHASECAM target_monitor cannot use 'movewith'\n");
			self->spawnflags &= ~(SF_MONITOR_CHASECAM | SF_MONITOR_EYEBALL);
		}
	}

	gi.linkentity(self);
}

/*====================================================================================
 TARGET_ANIMATION causes the target entity to use the animation frames
 "startframe" through "startframe" + "framenumbers" - 1.

 Spawnflags:
 ACTIVATOR = 1 - target_animation acts on it's activator rather than
                 it's target

 "message" - specifies allowable classname to animate. This prevents 
             animating entities with inapplicable frame numbers
=====================================================================================*/
void target_animate (edict_t *ent)
{
	if ( (ent->s.frame <  ent->monsterinfo.currentmove->firstframe) ||
		(ent->s.frame >= ent->monsterinfo.currentmove->lastframe )    )
	{
		if (ent->monsterinfo.currentmove->endfunc)
		{
			ent->think     = ent->monsterinfo.currentmove->endfunc;
			ent->nextthink = level.time + FRAMETIME;
		}
		else if (ent->svflags & SVF_MONSTER)
		{
			// Hopefully we don't get here, but if we DO then we definitely 
			// need for monsters/actors to turn their brains back on.
			ent->think     = monster_think;
			ent->nextthink = level.time + FRAMETIME;
		}
		else
		{
			ent->think     = NULL;
			ent->nextthink = 0;
		}
		ent->monsterinfo.currentmove = ent->monsterinfo.savemove;
		return;
	}
	ent->s.frame++;
	ent->nextthink = level.time + FRAMETIME;
	gi.linkentity(ent);
}

void target_animation_use (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t	*target = NULL;

	if (level.time < self->touch_debounce_time)
		return;
	if (self->spawnflags & 1)
	{
		if (activator && activator->client)
			return;
		if (self->message && Q_stricmp(self->message, activator->classname))
			return;
		if (!self->target)
			target = activator;
	}
	if (!target)
	{
		if (!self->target)
			return;
		target = G_Find(NULL,FOFS(targetname),self->target);
		if (!target)
			return;
	}
	// Don't allow target to be animated if ALREADY under influence of
	// another target_animation
	if (target->think == target_animate)
		return;
	self->monsterinfo.currentmove->firstframe = self->startframe;
	self->monsterinfo.currentmove->lastframe  = self->startframe + self->framenumbers - 1;
	self->monsterinfo.currentmove->frame      = NULL;
	self->monsterinfo.currentmove->endfunc    = target->think;
	target->s.frame = self->startframe;
	target->think   = target_animate;
	target->monsterinfo.savemove    = target->monsterinfo.currentmove;
	target->monsterinfo.currentmove = self->monsterinfo.currentmove;
	target->nextthink = level.time + FRAMETIME;
	gi.linkentity(target);

	self->count--;
	if (!self->count)
		G_FreeEdict(self);
	else
		self->touch_debounce_time = level.time + (self->framenumbers+1)*FRAMETIME;
}

void SP_target_animation (edict_t *self)
{
	mmove_t	*move;
	if (!self->target && !(self->spawnflags & 1))
	{
		gi.dprintf("target_animation w/o a target at %s\n",vtos(self->s.origin));
		G_FreeEdict(self);
	}
	switch(self->sounds)
	{
	case 1:
		// actor jump
		self->startframe = 66;
		self->framenumbers = 6;
		break;
	case 2:
		// actor flip
		self->startframe = 72;
		self->framenumbers = 12;
		break;
	case 3:
		// actor salute
		self->startframe = 84;
		self->framenumbers = 11;
		break;
	case 4:
		// actor taunt
		self->startframe = 95;
		self->framenumbers = 17;
		break;
	case 5:
		// actor wave
		self->startframe = 112;
		self->framenumbers = 11;
		break;
	case 6:
		// actor point
		self->startframe = 123;
		self->framenumbers = 12;
		break;
	default:
		if (!self->framenumbers)
			self->framenumbers = 1;
	}
	self->use = target_animation_use;
	move = gi.TagMalloc(sizeof(mmove_t), TAG_LEVEL);
	self->monsterinfo.currentmove = move;
}
/*===================================================================================
 TARGET_FAILURE - Halts the game, fades the screen to black and displays
                  a message explaining to the player how he screwed up.
                  Optionally plays a sound.
====================================================================================*/
void target_failure_wipe (edict_t *self)
{
	edict_t	*player;

	player = &g_edicts[1];	// Gotta be, since this is SP only
	if (player->client->textdisplay) Text_Close(player);
}

void target_failure_player_die (edict_t *player)
{
	int		n;

	// player_die w/o... umm... dying

	if (player->client->chasetoggle)
	{
		ChasecamRemove (player, OPTION_OFF);
		player->client->pers.chasetoggle = 1;
	}
	else
		player->client->pers.chasetoggle = 0;
	player->client->pers.spawn_landmark = false; // paranoia check
	player->client->pers.spawn_levelchange = false;
	player->client->zooming = 0;
	player->client->zoomed = false;
	SetSensitivities(player,true);
	if (player->client->spycam)
		camera_off(player);
	VectorClear (player->avelocity);
	player->takedamage = DAMAGE_NO;
	player->movetype = MOVETYPE_NONE;
	player->s.modelindex2 = 0;	// remove linked weapon model
	player->s.sound = 0;
	player->client->weapon_sound = 0;
	player->svflags |= SVF_DEADMONSTER;
	player->client->respawn_time = level.time + 1.0;
	player->client->ps.gunindex = 0;
	// clear inventory
	for (n = 0; n < game.num_items; n++)
	{
		player->client->pers.inventory[n] = 0;
	}
	// remove powerups
	player->client->quad_framenum = 0;
	player->client->invincible_framenum = 0;
	player->client->breather_framenum = 0;
	player->client->enviro_framenum = 0;
	player->flags &= ~(FL_POWER_SHIELD|FL_POWER_SCREEN);
	player->client->flashlight = false;
	player->deadflag = DEAD_FROZEN;
	gi.linkentity (player);
}
void target_failure_think (edict_t *self)
{
	target_failure_player_die(self->target_ent);
	self->target_ent = NULL;
	self->think = target_failure_wipe;
	self->nextthink = level.time + 10;
}

void target_failure_fade_lights (edict_t *self)
{
	char lightvalue[2];
	char values[] = "abcdefghijklm";

	lightvalue[0] = values[self->flags];
	lightvalue[1] = 0;
	gi.configstring(CS_LIGHTS+0, lightvalue);
	if (self->flags)
	{
		self->flags--;
		self->nextthink = level.time + 0.2;
	}
	else
	{
		target_failure_player_die(self->target_ent);
		self->target_ent = NULL;
		self->think = target_failure_wipe;
		self->nextthink = level.time + 10;
	}
}

void Use_Target_Text(edict_t *self, edict_t *other, edict_t *activator);
void use_target_failure (edict_t *self, edict_t *other, edict_t *activator)
{
	if (!activator->client)
		return;

	if (self->target_ent)
		return;

	if (strlen(self->message))
		Use_Target_Text(self,other,activator);

	if (self->noise_index)
		gi.sound (activator, CHAN_VOICE|CHAN_RELIABLE, self->noise_index, 1, ATTN_NORM, 0);

	self->target_ent = activator;
	if (strcmp(vid_ref->string,"gl"))
	{
		self->flags = 12;
		self->think = target_failure_fade_lights;
		self->nextthink = level.time + FRAMETIME;
	}
	else
	{
		activator->client->fadestart= level.framenum;
		activator->client->fadein   = level.framenum + 40;
		activator->client->fadehold = activator->client->fadein + 100000;
		activator->client->fadeout  = 0;
		activator->client->fadecolor[0] = 0;
		activator->client->fadecolor[1] = 0;
		activator->client->fadecolor[2] = 0;
		activator->client->fadealpha    = 1.0;
		self->think = target_failure_think;
		self->nextthink = level.time + 4;
	}
	activator->deadflag = DEAD_FROZEN;
	gi.linkentity(activator);
}

void SP_target_failure (edict_t *self)
{
	if (deathmatch->value || coop->value)
	{	// SP only
		G_FreeEdict (self);
		return;
	}
	self->use = use_target_failure;
	if (st.noise)
		self->noise_index = gi.soundindex(st.noise);
}

/*=====================================================================================
 TARGET_CHANGE - copies selected fields to target.
 Target value should be of the form "entitytotarget,new target value".
 All other keys are replacements for the targeted entity, NOT the
 target_change.
======================================================================================*/
void use_target_change (edict_t *self, edict_t *other, edict_t *activator)
{
	char	*buffer;
	char	*target;
	char	*newtarget;
	int		L;
	int		newteams=0;
	edict_t	*target_ent;

	if (!self->target)
		return;
	L = (int)strlen(self->target);
//	buffer = (char *)malloc(L+1);
	buffer = (char *)gi.TagMalloc(L+1, TAG_LEVEL);
	strcpy(buffer,self->target);
	newtarget = strstr(buffer,",");
	if (newtarget)
	{
		*newtarget = 0;
		newtarget++;
	}
	target = buffer;
	target_ent = G_Find(NULL,FOFS(targetname),target);
	while(target_ent)
	{
		if (newtarget && strlen(newtarget))
			target_ent->target = G_CopyString(newtarget);
		if (self->newtargetname && strlen(self->newtargetname))
			target_ent->targetname = G_CopyString(self->newtargetname);
		if (self->team && strlen(self->team))
		{
			target_ent->team = G_CopyString(self->team);
			newteams++;
		}
		if (VectorLength(self->s.angles))
		{
			VectorCopy (self->s.angles, target_ent->s.angles);
			if (target_ent->solid == SOLID_BSP)
				G_SetMovedir (target_ent->s.angles, target_ent->movedir);
		}
		if (self->deathtarget && strlen(self->deathtarget))
			target_ent->deathtarget = G_CopyString(self->deathtarget);
		if (self->pathtarget && strlen(self->pathtarget))
			target_ent->pathtarget = G_CopyString(self->pathtarget);
		if (self->killtarget && strlen(self->killtarget))
			target_ent->killtarget = G_CopyString(self->killtarget);
		if (self->message && strlen(self->message))
			target_ent->message = G_CopyString(self->message);
		if (self->delay > 0)
			target_ent->delay = self->delay;
		if (self->dmg > 0)
			target_ent->dmg = self->dmg;
		if (self->health > 0)
			target_ent->health = self->health;
		if (self->mass > 0)
			target_ent->mass = self->mass;
		if (self->pitch_speed > 0)
			target_ent->pitch_speed = self->pitch_speed;
		if (self->random > 0)
			target_ent->random = self->random;
		if (self->roll_speed > 0)
			target_ent->roll_speed = self->roll_speed;
		if (self->wait > 0)
			target_ent->wait = self->wait;
		if (self->yaw_speed > 0)
			target_ent->yaw_speed = self->yaw_speed;
		if (self->noise_index)
		{
			if (target_ent->s.sound == target_ent->noise_index)
				target_ent->s.sound = target_ent->noise_index = self->noise_index;
			else
				target_ent->noise_index = self->noise_index;
		}
#ifdef LOOP_SOUND_ATTENUATION
		if(self->attenuation)
		{
			if(target_ent->s.attenuation == target_ent->attenuation)
				target_ent->s.attenuation = target_ent->attenuation = self->attenuation;
			else
				target_ent->attenuation = self->attenuation;
		}
#endif
		if (self->spawnflags)
		{
			target_ent->spawnflags = self->spawnflags;
			// special cases:
			if (!Q_stricmp(target_ent->classname,"model_train"))
			{
				if (target_ent->spawnflags & 32)
				{
					target_ent->spawnflags &= ~32;
					target_ent->spawnflags |= 8;
				}
				if (target_ent->spawnflags & 64)
				{
					target_ent->spawnflags &= ~64;
					target_ent->spawnflags |= 16;
				}
			}
		}
		gi.linkentity(target_ent);
		target_ent = G_Find(target_ent,FOFS(targetname),target);
	}
//	free(buffer);
	gi.TagFree(buffer);
	if (newteams)
		G_FindTeams();
}

void SP_target_change (edict_t *self)
{
	self->use = use_target_change;
	if (st.noise)
		self->noise_index = gi.soundindex(st.noise);
}

/*====================================================================================
   TARGET_MOVEWITH - sets an entity to "movewith" it's pathtarget (or remove
   that setting if DETACH (=1) is set)
======================================================================================*/

void movewith_detach (edict_t *child)
{
	edict_t	*e;
	edict_t	*parent=NULL;
	int		i;
				
	for(i=1; i<globals.num_edicts && !parent; i++) {
		e = g_edicts + i;
		if (e->movewith_next == child) parent=e;
	}
	if (parent) parent->movewith_next = child->movewith_next;
		
	child->movewith_next = NULL;
	child->movewith = NULL;
	child->movetype = child->org_movetype;
	// if monster, give 'em a small vertical boost
	if (child->svflags & SVF_MONSTER)
		child->s.origin[2] += 2;
	gi.linkentity(child);
}

void use_target_movewith (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t	*target;

	if (!self->target)
		return;
	target = G_Find(NULL,FOFS(targetname),self->target);

	if (self->spawnflags & 1)
	{
		// Detach
		while(target)
		{
			if (target->movewith_ent)
				movewith_detach(target);
			target = G_Find(target,FOFS(targetname),self->target);
		}
	}
	else
	{
		// Attach
		edict_t	*parent;
		edict_t	*e;
		edict_t	*previous;

		parent = G_Find(NULL,FOFS(targetname),self->pathtarget);
		if (!parent || !parent->inuse)
			return;
		while(target)
		{
			if (!target->movewith_ent || (target->movewith_ent != parent) )
			{
				if (target->movewith_ent)
					movewith_detach(target);
		
				target->movewith_ent = parent;
				VectorCopy(parent->s.angles,target->parent_attach_angles);
				if (target->org_movetype < 0)
					target->org_movetype = target->movetype;
				if (target->movetype != MOVETYPE_NONE)
					target->movetype = MOVETYPE_PUSH;
				VectorCopy(target->mins,target->org_mins);
				VectorCopy(target->maxs,target->org_maxs);
				VectorSubtract(target->s.origin,parent->s.origin,target->movewith_offset);
				e = parent->movewith_next;
				previous = parent;
				while(e)
				{
					previous = e;
					e = previous->movewith_next;
				}
				previous->movewith_next = target;
				gi.linkentity(target);
			}
			target = G_Find(target,FOFS(targetname),self->target);
		}
	}

	self->count--;
	if (!self->count) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}
}

void SP_target_movewith (edict_t *self)
{
	if (!self->target)
	{
		gi.dprintf("target_movewith with no target\n");
		G_FreeEdict(self);
		return;
	}
	if (!(self->spawnflags & 1) && !self->pathtarget)
	{
		gi.dprintf("target_movewith w/o DETACH and no pathtarget\n");
		G_FreeEdict(self);
		return;
	}
	self->use = use_target_movewith;
}

/*====================================================================================
 TARGET_SET_EFFECT - sets s.effects and/or s.renderfx for targeted entity
 Style
 0 = Copy
 1 = NOT
 2 = XOR (toggle)
======================================================================================*/
 
void use_target_set_effect (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t *target;

	target = G_Find(NULL,FOFS(targetname),self->target);
	while(target)
	{
		if (self->style == 1)
		{
			target->s.effects &= ~self->effects;
			target->s.renderfx &= ~self->renderfx;
		}
		else if (self->style == 2)
		{
			target->s.effects ^= self->effects;
			target->s.renderfx ^= self->renderfx;
		}
		else
		{
			target->s.effects = self->effects;
			target->s.renderfx = self->renderfx;
		}
		gi.linkentity(target);
		target = G_Find(target,FOFS(targetname),self->target);
	}
}

void SP_target_set_effect (edict_t *self)
{
	if (!self->target)
	{
		gi.dprintf("target_set_effect w/o a target at %s\n",vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}
	self->use = use_target_set_effect;
}

/*=============================================================================
 TARGET_SKY - uses the "sky" string as parameter to "sky" console command.
 Best used in areas where player cannot see the sky, of course.

 NOTE: Tried using a START_ON spawnflag, but it ends up being meaningless.
       If we don't delay use_target_sky a bit at level start, we get
       "SZ_GetSpace: overflow without allowoverflow set", and if we use
       a sufficient delay to get around the error then the original sky
       is visible before the switch.
=============================================================================*/
void use_target_sky (edict_t *self, edict_t *other, edict_t *activator)
{
	gi.configstring(CS_SKY,self->pathtarget);
	stuffcmd(&g_edicts[1],va("sky %s\n",self->pathtarget));
	self->count--;
	if (!self->count) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}
}

void SP_target_sky (edict_t *self)
{
	if (!st.sky || !*st.sky)
	{
		gi.dprintf("Target_sky with no sky string at %s\n",vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}
	self->pathtarget = gi.TagMalloc((int)strlen(st.sky)+1,TAG_LEVEL);
	strcpy(self->pathtarget,st.sky);
	self->use = use_target_sky;
}

/*=============================================================================
 TARGET_FADE fades the screen to a color
 color    = R,G,B components of fade color, 0-1
 alpha    = opacity of fade. 0=no effect, 1=solid color
 fadein   = time in seconds from trigger until full alpha
 fadeout  = time in seconds after fadein+holdtime from full alpha to clear screen
 holdtime = time to hold the effect at full alpha value. -1 = permanent
=============================================================================*/
void use_target_fade (edict_t *self, edict_t *other, edict_t *activator)
{
	if (!activator->client)
		return;

	activator->client->fadestart= level.framenum;
	activator->client->fadein   = level.framenum + self->fadein*10;
	activator->client->fadehold = activator->client->fadein + self->holdtime*10;
	activator->client->fadeout  = activator->client->fadehold + self->fadeout*10;
	activator->client->fadecolor[0] = self->color[0];
	activator->client->fadecolor[1] = self->color[1];
	activator->client->fadecolor[2] = self->color[2];
	activator->client->fadealpha    = self->alpha;

	self->count--;
	if (!self->count) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}
}

void SP_target_fade (edict_t *self)
{
	self->use = use_target_fade;
	if (self->fadein < 0)
		self->fadein = 0;
	if (self->holdtime < 0)
	{
		self->count = 1;
		self->holdtime = 10000;
	}
	if (self->fadeout < 0)
		self->fadeout = 0;
}

/*=============================================================================
  TARGET_CLONE
  Spawns a new entity, using model and other parameters of source entity.

  source        - targetname of source entity for model
  newtargetname - targetname to assign to spawned entity
  target        - target to assign to spawned entity
  count         - number of spawner uses before being freed
  Spawnflags
              1 = START_ON
=============================================================================*/
void button_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
void Think_CalcMoveSpeed (edict_t *self);
void Think_SpawnDoorTrigger (edict_t *ent);
void func_train_find (edict_t *self);
void clone (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t	*parent;
	edict_t	*child;
	int		newteams=0;

	parent = G_Find(NULL,FOFS(targetname),self->source);
	if (!parent)
		return;
	child = G_Spawn();
	child->classname = gi.TagMalloc((int)strlen(parent->classname)+1,TAG_LEVEL);
	strcpy(child->classname,parent->classname);
	child->s.modelindex = parent->s.modelindex;
	VectorCopy(self->s.origin,child->s.origin);
	child->svflags    = parent->svflags;
	VectorCopy(parent->mins,child->mins);
	VectorCopy(parent->maxs,child->maxs);
	VectorCopy(parent->size,child->size);

	if (self->newtargetname && strlen(self->newtargetname))
		child->targetname = G_CopyString(self->newtargetname);
	if (self->team && strlen(self->team))
	{
		child->team = G_CopyString(self->team);
		newteams++;
	}
	if (self->target && strlen(self->target))
		child->target = G_CopyString(self->target);

	if (parent->deathtarget && strlen(parent->deathtarget))
		child->deathtarget = G_CopyString(parent->deathtarget);
	if (parent->destroytarget && strlen(parent->destroytarget))
		child->destroytarget = G_CopyString(parent->destroytarget);
	if (parent->killtarget && strlen(parent->killtarget))
		child->killtarget = G_CopyString(parent->killtarget);

	child->solid        = parent->solid;
	child->clipmask     = parent->clipmask;
	child->movetype     = parent->movetype;
	child->mass         = parent->mass;
	child->health       = parent->health;
	child->max_health   = parent->max_health;
	child->takedamage   = parent->takedamage;
	child->dmg          = parent->dmg;
	child->sounds       = parent->sounds;
	child->speed        = parent->speed;
	child->accel        = parent->accel;
	child->decel        = parent->decel;
	child->gib_type     = parent->gib_type;
	child->noise_index  = parent->noise_index;
	child->noise_index2 = parent->noise_index2;
	child->wait         = parent->wait;
	child->delay        = parent->delay;
	child->random       = parent->random;
	child->style        = parent->style;
	child->flags        = parent->flags;
	child->blocked      = parent->blocked;
	child->touch        = parent->touch;
	child->use          = parent->use;
	child->pain         = parent->pain;
	child->die          = parent->die;
	child->s.effects    = parent->s.effects;
	child->s.skinnum    = parent->s.skinnum;
	child->item         = parent->item;
	child->moveinfo.sound_start  = parent->moveinfo.sound_start;
	child->moveinfo.sound_middle = parent->moveinfo.sound_middle;
	child->moveinfo.sound_end    = parent->moveinfo.sound_end;
	VectorCopy(parent->movedir,child->movedir);
	VectorCopy(self->s.angles, child->s.angles);
	if (VectorLength(child->s.angles) != 0)
	{
		if (child->s.angles[YAW] == 90 || child->s.angles[YAW] == 270)
		{
			// We're correct for these angles, not even gonna bother with others
			vec_t	temp;
			temp           = child->size[0];
			child->size[0] = child->size[1];
			child->size[1] = temp;
			temp           = child->mins[0];
			if (child->s.angles[YAW] == 90)
			{
				child->mins[0] = -child->maxs[1];
				child->maxs[1] = child->maxs[0];
				child->maxs[0] = -child->mins[1];
				child->mins[1] = temp;
			}
			else
			{
				child->mins[0] = child->mins[1];
				child->mins[1] = -child->maxs[0];
				child->maxs[0] = child->maxs[1]; 
				child->maxs[1] = -temp;
			}
		}
		vectoangles(child->movedir,child->movedir);
		child->movedir[PITCH] += child->s.angles[PITCH];
		child->movedir[YAW]   += child->s.angles[YAW];
		child->movedir[ROLL]  += child->s.angles[ROLL];
		if (child->movedir[PITCH] > 360) child->movedir[PITCH] -= 360;
		if (child->movedir[YAW]   > 360) child->movedir[YAW]   -= 360;
		if (child->movedir[ROLL]  > 360) child->movedir[ROLL]  -= 360;
		AngleVectors(child->movedir,child->movedir,NULL,NULL);
	}
	VectorAdd(child->s.origin,child->mins,child->absmin);
	VectorAdd(child->s.origin,child->maxs,child->absmax);

	child->spawnflags = parent->spawnflags;
	// classname-specific stuff
	if (!Q_stricmp(child->classname,"func_button"))
	{
		VectorCopy(child->s.origin,child->pos1);
		child->moveinfo.distance = parent->moveinfo.distance;
		VectorMA(child->pos1, child->moveinfo.distance, child->movedir, child->pos2);
		child->moveinfo.state = 1;
		child->moveinfo.speed = child->speed;
		child->moveinfo.accel = child->accel;
		child->moveinfo.decel = child->decel;
		child->moveinfo.wait  = child->wait;
		VectorCopy(child->pos1,     child->moveinfo.start_origin);
		VectorCopy(child->s.angles, child->moveinfo.start_angles);
		VectorCopy(child->pos2,     child->moveinfo.end_origin);
		VectorCopy(child->s.angles, child->moveinfo.end_angles);
		if (!child->targetname)
			child->touch = button_touch;
	}
	else if (!Q_stricmp(child->classname,"func_door"))
	{
		VectorCopy(child->s.origin,child->pos1);
		child->moveinfo.distance = parent->moveinfo.distance;
		VectorMA(child->pos1, child->moveinfo.distance, child->movedir, child->pos2);
		child->moveinfo.state = 1;
		child->moveinfo.speed = child->speed;
		child->moveinfo.accel = child->accel;
		child->moveinfo.decel = child->decel;
		child->moveinfo.wait  = child->wait;
		VectorCopy(child->pos1,     child->moveinfo.start_origin);
		VectorCopy(child->s.angles, child->moveinfo.start_angles);
		VectorCopy(child->pos2,     child->moveinfo.end_origin);
		VectorCopy(child->s.angles, child->moveinfo.end_angles);
		if (child->health || child->targetname)
			child->think = Think_CalcMoveSpeed;
		else
			child->think = Think_SpawnDoorTrigger;
		child->nextthink = level.time + FRAMETIME;
	}
	else if (!Q_stricmp(child->classname,"func_door_rotating"))
	{
		VectorClear(child->s.angles);
		VectorCopy(parent->s.angles,child->s.angles);
		VectorCopy(parent->pos1, child->pos1);
		VectorCopy(parent->pos2, child->pos2);
		child->moveinfo.distance = parent->moveinfo.distance;
		child->moveinfo.state = 1;
		child->moveinfo.speed = child->speed;
		child->moveinfo.accel = child->accel;
		child->moveinfo.decel = child->decel;
		child->moveinfo.wait  = child->wait;
		VectorCopy(child->s.origin, child->moveinfo.start_origin);
		VectorCopy(child->pos1,     child->moveinfo.start_angles);
		VectorCopy(child->s.origin, child->moveinfo.end_origin);
		VectorCopy(child->pos2,     child->moveinfo.end_angles);
		if (child->health || child->targetname)
			child->think = Think_CalcMoveSpeed;
		else
			child->think = Think_SpawnDoorTrigger;
		child->nextthink = level.time + FRAMETIME;
	}
	else if (!Q_stricmp(child->classname,"func_rotating"))
	{
		VectorClear(child->s.angles);
		if (child->spawnflags & 1)
			child->use (child, NULL, NULL);
	}
	else if (!Q_stricmp(child->classname,"func_train"))
	{
		VectorClear(self->s.angles);
		child->smooth_movement = parent->smooth_movement;
		child->pitch_speed     = parent->pitch_speed;
		child->yaw_speed       = parent->yaw_speed;
		child->roll_speed      = parent->roll_speed;
		child->moveinfo.speed = child->speed;
		child->moveinfo.accel = child->moveinfo.decel = child->moveinfo.speed;
		child->think = func_train_find;
		child->nextthink = level.time + FRAMETIME;
		if (child->moveinfo.sound_middle || parent->noise_index)
		{
			edict_t *speaker;
			if (child->moveinfo.sound_middle)
				child->noise_index = child->moveinfo.sound_middle;
			else
				child->noise_index = parent->noise_index;
			child->moveinfo.sound_middle = 0;
			speaker = G_Spawn();
			speaker->classname   = "moving_speaker";
			speaker->s.sound     = 0;
			speaker->volume      = 1;
			speaker->attenuation = 1;
			speaker->owner       = child;
			speaker->think       = Moving_Speaker_Think;
			speaker->nextthink   = level.time + 2*FRAMETIME;
			speaker->spawnflags  = 7;       // owner must be moving to play
			child->speaker        = speaker;
			if (VectorLength(child->s.origin))
				VectorCopy(child->s.origin,speaker->s.origin);
			else {
				VectorAdd(child->absmin,child->absmax,speaker->s.origin);
				VectorScale(speaker->s.origin,0.5,speaker->s.origin);
			}
			VectorSubtract(speaker->s.origin,child->s.origin,speaker->offset);
		}
	}
	gi.unlinkentity(child);
	KillBox(child);
	gi.linkentity(child);
	if (self->s.angles[YAW] != 0)
	{
		VectorAdd(child->s.origin,child->mins,child->absmin);
		VectorAdd(child->s.origin,child->maxs,child->absmax);
	}
	self->count--;
	if (!self->count)
		G_FreeEdict(self);
	if (newteams)
		G_FindTeams();
}

void target_clone_starton (edict_t *self)
{
	self->use(self,NULL,NULL);
}

void SP_target_clone (edict_t *self)
{
	if (!self->source)
	{
		gi.dprintf("%s with no source at %s\n",
			self->classname,vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}
	self->use = clone;
	if (self->spawnflags & 1)
	{
		self->think = target_clone_starton;
		self->nextthink = level.time + 2;
	}
}

void use_target_skill (edict_t *self, edict_t *other, edict_t *activator)
{
	level.next_skill = self->style + 1;
	self->count--;
	if (!self->count)
		G_FreeEdict(self);
}

void SP_target_skill (edict_t *self)
{
	self->use = use_target_skill;
}

//CW++
/*=============================================================================
 target_holo
 Displays a model from the 'holo_list' array, advancing to the next one
 in the list each time it is used (wraps around).

 Model 'effects' and 'renderfx' fields can be set as usual; it is suggested
 that a combination of rotating and translucent styles are used to give a
 "holographic" effect. These weren't hardcoded, though, to allow mappers
 more flexibility with this entity.

 "count" = initial model number to be displayed (see the declaration
 of holo_list).
=============================================================================*/
void target_holo_think(edict_t *self) 
{
	self->s.frame++;
	if (self->s.frame >= self->framenumbers)
		self->s.frame = self->startframe;
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}

void target_holo_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->usermodel = G_CopyString(holo_list[++self->count]);
	self->s.modelindex = gi.modelindex(self->usermodel);
	gi.linkentity(self);
	if (self->count == (HOLO_SIZE - 1))
		self->count = 0;
}

void SP_target_holo(edict_t *self)
{
	if ((self->count >= (HOLO_SIZE - 1)) || (self->count < 0))
		self->count = 0;

	self->s.modelindex = gi.modelindex(holo_list[self->count]);
	self->solid = SOLID_NOT;

	if (self->movewith)
		self->movetype = MOVETYPE_NOCLIP;
	else
		self->movetype = MOVETYPE_NONE;

	switch (self->style)
	{
		case 1 : self->s.effects |= EF_ANIM01; break;
		case 2 : self->s.effects |= EF_ANIM23; break;
		case 3 : self->s.effects |= EF_ANIM_ALL; break;
		case 4 : self->s.effects |= EF_ANIM_ALLFAST; break;
	}
	self->s.effects  |= self->effects;
	self->s.renderfx |= self->renderfx;

	if (self->startframe < 0)
		self->startframe = 0;
	if (!self->framenumbers)
		self->framenumbers = 1;
	self->framenumbers += self->startframe;
	self->s.frame = self->startframe;

	if (st.noise)
		self->noise_index = gi.soundindex(st.noise);
	self->s.sound = self->noise_index;

	if (!(self->s.effects & ANIM_MASK) && (self->framenumbers > 1))
	{
		self->think = target_holo_think;
		self->nextthink = level.time + (2 * FRAMETIME);
	}
	self->use = target_holo_use;
	gi.linkentity(self);
}


//==========================================================
/*QUAKED target_bubbles (1 0 0) (-8 -8 -8) (8 8 8) StartOn
Continuously spawns a number of sprites that float upwards and vanish
when they reach a specified height. If no usermodel is specified, the
sprites will be taken from the file "sprites/s_bubble.sp2" by default.

"usermodel" SP2 file
"startframe" PCX frame number
"speed" vertical velocity (default = 40)
"random" random +/- amount to vary vertical velocity by 
"height" height above parent entity at which to vanish (default = 128)
"mass" number of sprite groups spawned per second (default = 5; max = 10)
"mass2" number of sprites per group (default = 1)
"radius" random +/- amount to scatter bubbles in XY plane
"count"	 number of times entity can be toggled off
"movewith" targetname of train or other mover
"targetname" name of entity (allows it to be toggled on/off)
"spawnflags" 1 = Start on (if targetname set)
"effects" as usual
"renderfx" as usual
*/

void target_bubbles_on(edict_t *self, edict_t *other, edict_t *activator);

void target_bubbles_think(edict_t *self)
{
	edict_t *bubble;
	vec3_t	offset;
	float	v_speed;
	int		i;

	for (i = 0; i < self->mass2; ++i)
	{
		bubble = G_Spawn();
		bubble->s.modelindex = gi.modelindex(self->usermodel);
		bubble->s.frame = self->startframe;
		bubble->s.effects = self->effects;
		bubble->s.renderfx = self->renderfx;
		bubble->movetype = MOVETYPE_NOCLIP;

		VectorSet(offset, crandom()*self->radius, crandom()*self->radius, 0);
		VectorAdd(self->s.origin, offset, bubble->s.origin);
		
		VectorSet(bubble->movedir, 0, 0, 1);
		v_speed = self->speed + (crandom() * self->random);
		VectorSet(bubble->velocity, 0, 0, v_speed);

		bubble->nextthink = level.time + (self->pos2[2] / v_speed);
		bubble->think = G_FreeEdict;
		gi.linkentity(bubble);
	}

	self->nextthink = level.time + (1.0 / self->mass);
}

void target_bubbles_off(edict_t *self, edict_t *other, edict_t *activator)
{
	self->nextthink = 0;
	self->use = target_bubbles_on;

	--self->count;
	if (!self->count)
	{
		self->think = G_FreeEdict;
		self->use = NULL;
		self->nextthink = level.time + 1;
	}
}

void target_bubbles_on(edict_t *self, edict_t *other, edict_t *activator)
{
	self->nextthink = level.time + FRAMETIME;
	self->think = target_bubbles_think;
	self->use = target_bubbles_off;
}

void SP_target_bubbles(edict_t *self)
{
	self->svflags |= SVF_NOCLIENT;

//	Initialise default values.

	if (!self->usermodel)
		self->usermodel = G_CopyString("sprites/s_bubbles.sp2");

	if (!st.height)
		self->pos2[2] = 128;
	else
		self->pos2[2] = st.height;

	if (!self->speed)
		self->speed = 40; 

	if (!self->mass)
		self->mass = 5;

	if (self->mass > 10)
		self->mass = 10;

	if (!self->mass2)
		self->mass2 = 1;

//	If targeted then wait for activation, otherwise start bubbling!

	if (self->targetname)
	{
		if (self->spawnflags & 1)
		{
			self->think = target_bubbles_think;
			self->nextthink = level.time + FRAMETIME;
			self->use = target_bubbles_off;
		}
		else
			self->use = target_bubbles_on;
	}
	else
	{
		self->think = target_bubbles_think;
		self->nextthink = level.time + FRAMETIME;
	}
}


/*=============================================================================
QUAKED target_viewshake (1 0 0) (-8 -8 -8) (8 8 8)
Shakes the player's viewing angle briefly.

"angle" = +/- degrees in horizontal plane (only YAW used)
"yaw_speed" = amplitude decay factor (lower => longer shake)
"count"	= number of times entity can be used
*/
void target_viewshake_think(edict_t *self)
{
	self->target_ent->client->ps.viewangles[YAW] += self->ideal_yaw;
	self->ideal_yaw *= -(1.0 - self->angle);

	if (fabs(self->ideal_yaw) < 0.01)
	{
		if (self->count == 0)
			G_FreeEdict(self);
		else
		{
			self->target_ent->client->ps.viewangles[YAW] = self->holdtime;
			self->nextthink = 0.0;
		}

		return;
	}

	self->angle += self->yaw_speed;
	self->nextthink = level.time + FRAMETIME;
}

void target_viewshake_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->target_ent = activator;
	self->holdtime = self->target_ent->client->ps.viewangles[YAW];

	self->ideal_yaw = self->s.angles[YAW];
	self->angle = 0.0;

	self->think = target_viewshake_think;
	self->nextthink = level.time + FRAMETIME;

	self->count--;
}

void SP_target_viewshake(edict_t *self)
{
	if (!self->targetname)
	{
		gi.dprintf("target_viewshake with no targetname at %s\n", vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	self->use = target_viewshake_use;
	self->svflags |= SVF_NOCLIENT;
}
//CW--
