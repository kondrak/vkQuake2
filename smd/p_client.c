#include "g_local.h"
#include "m_player.h"

#define MUD1BASE 0.20
#define MUD1AMP  0.08
#define MUD3     0.08

void ClientUserinfoChanged (edict_t *ent, char *userinfo);
void SP_misc_teleporter_dest (edict_t *ent);

//
// Gross, ugly, disgustuing hack section
//

// this function is an ugly as hell hack to fix some map flaws
//
// the coop spawn spots on some maps are SNAFU.  There are coop spots
// with the wrong targetname as well as spots with no name at all
//
// we use carnal knowledge of the maps to fix the coop spot targetnames to match
// that of the nearest named single player spot

/*static*/ void SP_FixCoopSpots (edict_t *self)
{
	edict_t	*spot;
	vec3_t	d;

	spot = NULL;

	while(1)
	{
		spot = G_Find(spot, FOFS(classname), "info_player_start");
		if (!spot)
			return;
		if (!spot->targetname)
			continue;
		VectorSubtract(self->s.origin, spot->s.origin, d);
		if (VectorLength(d) < 384)
		{
			if ((!self->targetname) || Q_stricmp(self->targetname, spot->targetname) != 0)
			{
//				gi.dprintf("FixCoopSpots changed %s at %s targetname from %s to %s\n", self->classname, vtos(self->s.origin), self->targetname, spot->targetname);
				self->targetname = spot->targetname;
			}
			return;
		}
	}
}

// now if that one wasn't ugly enough for you then try this one on for size
// some maps don't have any coop spots at all, so we need to create them
// where they should have been

/*static*/ void SP_CreateCoopSpots (edict_t *self)
{
	edict_t	*spot;

	if(Q_stricmp(level.mapname, "security") == 0)
	{
		spot = G_Spawn();
		spot->classname = "info_player_coop";
		spot->s.origin[0] = 188 - 64;
		spot->s.origin[1] = -164;
		spot->s.origin[2] = 80;
		spot->targetname = "jail3";
		spot->s.angles[1] = 90;

		spot = G_Spawn();
		spot->classname = "info_player_coop";
		spot->s.origin[0] = 188 + 64;
		spot->s.origin[1] = -164;
		spot->s.origin[2] = 80;
		spot->targetname = "jail3";
		spot->s.angles[1] = 90;

		spot = G_Spawn();
		spot->classname = "info_player_coop";
		spot->s.origin[0] = 188 + 128;
		spot->s.origin[1] = -164;
		spot->s.origin[2] = 80;
		spot->targetname = "jail3";
		spot->s.angles[1] = 90;

		return;
	}
}


/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
The normal starting point for a level.
*/
void SP_info_player_start(edict_t *self)
{
	if (!coop->value)
		return;
	if(Q_stricmp(level.mapname, "security") == 0)
	{
		// invoke one of our gross, ugly, disgusting hacks
		self->think = SP_CreateCoopSpots;
		self->nextthink = level.time + FRAMETIME;
	}
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for deathmatch games
*/
void SP_info_player_deathmatch(edict_t *self)
{
	if (!deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}
	SP_misc_teleporter_dest (self);
}

/*QUAKED info_player_coop (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for coop games
*/

void SP_info_player_coop(edict_t *self)
{
	if (!coop->value)
	{
		G_FreeEdict (self);
		return;
	}

	if((Q_stricmp(level.mapname, "jail2") == 0)   ||
	   (Q_stricmp(level.mapname, "jail4") == 0)   ||
	   (Q_stricmp(level.mapname, "mine1") == 0)   ||
	   (Q_stricmp(level.mapname, "mine2") == 0)   ||
	   (Q_stricmp(level.mapname, "mine3") == 0)   ||
	   (Q_stricmp(level.mapname, "mine4") == 0)   ||
	   (Q_stricmp(level.mapname, "lab") == 0)     ||
	   (Q_stricmp(level.mapname, "boss1") == 0)   ||
	   (Q_stricmp(level.mapname, "fact3") == 0)   ||
	   (Q_stricmp(level.mapname, "biggun") == 0)  ||
	   (Q_stricmp(level.mapname, "space") == 0)   ||
	   (Q_stricmp(level.mapname, "command") == 0) ||
	   (Q_stricmp(level.mapname, "power2") == 0) ||
	   (Q_stricmp(level.mapname, "strike") == 0))
	{
		// invoke one of our gross, ugly, disgusting hacks
		self->think = SP_FixCoopSpots;
		self->nextthink = level.time + FRAMETIME;
	}
}


/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/
void SP_info_player_intermission(void)
{
}


//=======================================================================


void player_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	// player pain is handled at the end of the frame in P_DamageFeedback
}


qboolean IsFemale (edict_t *ent)
{
	char		*info;

	if (!ent->client)
		return false;

	info = Info_ValueForKey (ent->client->pers.userinfo, "gender");
	if (info[0] == 'f' || info[0] == 'F')
		return true;
	return false;
}

qboolean IsNeutral (edict_t *ent)
{
	char		*info;

	if (!ent->client)
		return false;

	info = Info_ValueForKey (ent->client->pers.userinfo, "gender");
	if (info[0] != 'f' && info[0] != 'F' && info[0] != 'm' && info[0] != 'M')
		return true;
	return false;
}

void ClientObituary (edict_t *self, edict_t *inflictor, edict_t *attacker)
{
	int			mod;
	char		*message;
	char		*message2;
	qboolean	ff;

	if (coop->value && attacker->client)
		meansOfDeath |= MOD_FRIENDLY_FIRE;

	if (deathmatch->value || coop->value)
	{
		ff = meansOfDeath & MOD_FRIENDLY_FIRE;
		mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;
		message = NULL;
		message2 = "";

		switch (mod)
		{
		case MOD_SUICIDE:
			message = "suicides";
			break;
		case MOD_FALLING:
			message = "cratered";
			break;
		case MOD_CRUSH:
			message = "was squished";
			break;
		case MOD_WATER:
			message = "sank like a rock";
			break;
		case MOD_SLIME:
			message = "melted";
			break;
		case MOD_LAVA:
			message = "does a back flip into the lava";
			break;
		case MOD_EXPLOSIVE:
		case MOD_BARREL:
			message = "blew up";
			break;
		case MOD_EXIT:
			message = "found a way out";
			break;
		case MOD_TARGET_LASER:
			message = "saw the light";
			break;
		case MOD_TARGET_BLASTER:
			message = "got blasted";
			break;
		case MOD_BOMB:
		case MOD_SPLASH:
		case MOD_TRIGGER_HURT:
		case MOD_VEHICLE:
			message = "was in the wrong place";
			break;
		}
		if (attacker == self)
		{
			switch (mod)
			{
			case MOD_HELD_GRENADE:
				message = "tried to put the pin back in";
				break;
			case MOD_HG_SPLASH:
			case MOD_G_SPLASH:
				if (IsNeutral(self))
					message = "tripped on its own grenade";
				else if (IsFemale(self))
					message = "tripped on her own grenade";
				else
					message = "tripped on his own grenade";
				break;
			case MOD_R_SPLASH:
				if (IsNeutral(self))
					message = "blew itself up";
				else if (IsFemale(self))
					message = "blew herself up";
				else
					message = "blew himself up";
				break;
			case MOD_BFG_BLAST:
				message = "should have used a smaller gun";
				break;
			default:
				if (IsNeutral(self))
					message = "killed itself";
				else if (IsFemale(self))
					message = "killed herself";
				else
					message = "killed himself";
				break;
			}
		}
		if (message)
		{
			gi.bprintf (PRINT_MEDIUM, "%s %s.\n", self->client->pers.netname, message);
			if (deathmatch->value)
				self->client->resp.score--;
			self->enemy = NULL;
			return;
		}

		self->enemy = attacker;
		if (attacker && attacker->client)
		{
			switch (mod)
			{
			case MOD_BLASTER:
				message = "was blasted by";
				break;
			case MOD_SHOTGUN:
				message = "was gunned down by";
				break;
			case MOD_SSHOTGUN:
				message = "was blown away by";
				message2 = "'s super shotgun";
				break;
			case MOD_MACHINEGUN:
				message = "was machinegunned by";
				break;
			case MOD_CHAINGUN:
				message = "was cut in half by";
				message2 = "'s chaingun";
				break;
			case MOD_GRENADE:
				message = "was popped by";
				message2 = "'s grenade";
				break;
			case MOD_G_SPLASH:
				message = "was shredded by";
				message2 = "'s shrapnel";
				break;
			case MOD_ROCKET:
				message = "ate";
				message2 = "'s rocket";
				break;
			case MOD_R_SPLASH:
				message = "almost dodged";
				message2 = "'s rocket";
				break;
			case MOD_HYPERBLASTER:
				message = "was melted by";
				message2 = "'s hyperblaster";
				break;
			case MOD_RAILGUN:
				message = "was railed by";
				break;
			case MOD_BFG_LASER:
				message = "saw the pretty lights from";
				message2 = "'s BFG";
				break;
			case MOD_BFG_BLAST:
				message = "was disintegrated by";
				message2 = "'s BFG blast";
				break;
			case MOD_BFG_EFFECT:
				message = "couldn't hide from";
				message2 = "'s BFG";
				break;
			case MOD_HANDGRENADE:
				message = "caught";
				message2 = "'s handgrenade";
				break;
			case MOD_HG_SPLASH:
				message = "didn't see";
				message2 = "'s handgrenade";
				break;
			case MOD_HELD_GRENADE:
				message = "feels";
				message2 = "'s pain";
				break;
			case MOD_TELEFRAG:
				message = "tried to invade";
				message2 = "'s personal space";
				break;
			case MOD_VEHICLE:
				message = "was splattered by";
				message2 = "'s vehicle";
				break;
			case MOD_KICK:
				message = "was booted by";
				message2 = "'s foot";
				break;
			case MOD_PLASMA:	//CW: plasma weaponry
				message = "was melted by";
				message2 = "'s plasma stream";
				break;
			}
			if (message)
			{
				gi.bprintf (PRINT_MEDIUM,"%s %s %s%s\n", self->client->pers.netname, message, attacker->client->pers.netname, message2);
				if (deathmatch->value)
				{
					if (ff)
						attacker->client->resp.score--;
					else
						attacker->client->resp.score++;
				}
				return;
			}
		}
	}

	gi.bprintf (PRINT_MEDIUM,"%s died.\n", self->client->pers.netname);
	if (deathmatch->value)
		self->client->resp.score--;
}


void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);

void TossClientWeapon (edict_t *self)
{
	gitem_t		*item;
	edict_t		*drop;
	qboolean	quad;
	float		spread;

	if (!deathmatch->value)
		return;

	item = self->client->pers.weapon;
	if (! self->client->pers.inventory[self->client->ammo_index] )
		item = NULL;
	if (item && (strcmp (item->pickup_name, "Blaster") == 0))
		item = NULL;
	if (item && (strcmp (item->pickup_name, "No Weapon") == 0))
		item = NULL;

	if (!((int)(dmflags->value) & DF_QUAD_DROP))
		quad = false;
	else
		quad = (self->client->quad_framenum > (level.framenum + 10));

	if (item && quad)
		spread = 22.5;
	else
		spread = 0.0;

	if (item)
	{
		self->client->v_angle[YAW] -= spread;
		drop = Drop_Item (self, item);
		self->client->v_angle[YAW] += spread;
		drop->spawnflags = DROPPED_PLAYER_ITEM;
	}

	if (quad)
	{
		self->client->v_angle[YAW] += spread;
		drop = Drop_Item (self, FindItemByClassname ("item_quad"));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= DROPPED_PLAYER_ITEM;

		drop->touch = Touch_Item;
		drop->nextthink = level.time + (self->client->quad_framenum - level.framenum) * (float)FRAMETIME;
		drop->think = G_FreeEdict;
	}
}


/*
==================
LookAtKiller
==================
*/
void LookAtKiller (edict_t *self, edict_t *inflictor, edict_t *attacker)
{
	vec3_t		dir;

	if (attacker && attacker != world && attacker != self)
	{
		VectorSubtract (attacker->s.origin, self->s.origin, dir);
	}
	else if (inflictor && inflictor != world && inflictor != self)
	{
		VectorSubtract (inflictor->s.origin, self->s.origin, dir);
	}
	else
	{
		self->client->killer_yaw = self->s.angles[YAW];
		return;
	}

	if (dir[0])
		self->client->killer_yaw = 180/M_PI*atan2(dir[1], dir[0]);
	else {
		self->client->killer_yaw = 0;
		if (dir[1] > 0)
			self->client->killer_yaw = 90;
		else if (dir[1] < 0)
			self->client->killer_yaw = -90;
	}
	if (self->client->killer_yaw < 0)
		self->client->killer_yaw += 360;
	

}

/*
==================
player_die
==================
*/
void player_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

	// tpp
	if (self->client->chasetoggle)
	{
		ChasecamRemove (self, OPTION_OFF);
		self->client->pers.chasetoggle = 1;
	}
	else
		self->client->pers.chasetoggle = 0;
	// end tpp

	self->client->pers.spawn_landmark = false; // paranoia check
	self->client->pers.spawn_levelchange = false;
	SetLazarusCrosshair(self); //backup crosshair
	self->client->zooming = 0;
	self->client->zoomed = false;
	SetSensitivities(self,true);

	if(self->client->spycam)
		camera_off(self);

	if(self->turret)
		turret_disengage(self->turret);

	if(self->client->textdisplay) Text_Close(self);

	VectorClear (self->avelocity);

	self->takedamage = DAMAGE_YES;
	self->movetype = MOVETYPE_TOSS;

	self->s.modelindex2 = 0;	// remove linked weapon model

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;

	self->s.sound = 0;
	self->client->weapon_sound = 0;

	self->maxs[2] = -8;

//	self->solid = SOLID_NOT;
	self->svflags |= SVF_DEADMONSTER;

	if (!self->deadflag)
	{
		self->client->respawn_time = level.time + 1.0;
		LookAtKiller (self, inflictor, attacker);
		self->client->ps.pmove.pm_type = PM_DEAD;
		ClientObituary (self, inflictor, attacker);
		TossClientWeapon (self);
		if (deathmatch->value)
			Cmd_Help_f (self);		// show scores

		// clear inventory
		// this is kind of ugly, but it's how we want to handle keys in coop
		for (n = 0; n < game.num_items; n++)
		{
			if (coop->value && itemlist[n].flags & IT_KEY)
				self->client->resp.coop_respawn.inventory[n] = self->client->pers.inventory[n];
			self->client->pers.inventory[n] = 0;
		}
	}

	// remove powerups
	self->client->quad_framenum = 0;
	self->client->invincible_framenum = 0;
	self->client->breather_framenum = 0;
	self->client->enviro_framenum = 0;
	self->flags &= ~(FL_POWER_SHIELD|FL_POWER_SCREEN);
	self->client->flashlight = false;

	// turn off alt-fire mode if on
	self->client->pers.fire_mode=0;
	self->client->nNewLatch &= ~BUTTON_ATTACK2;

//	if (self->health < self->gib_health)
	if (self->health < player_gib_health->value)
	{	// gib
		int num_giblets = 4;
		//if (deathmatch->value && (self->health < (self->gib_health*2)))
		if (deathmatch->value && (self->health < (player_gib_health->value*2)))
			num_giblets = 8;

		gi.sound (self, CHAN_BODY, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n= 0; n < num_giblets; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		if (mega_gibs->value)
		{
			ThrowGib (self, "models/objects/gibs/arm/tris.md2", damage, GIB_ORGANIC);
			ThrowGib (self, "models/objects/gibs/arm/tris.md2", damage, GIB_ORGANIC);
			ThrowGib (self, "models/objects/gibs/leg/tris.md2", damage, GIB_ORGANIC);
			ThrowGib (self, "models/objects/gibs/leg/tris.md2", damage, GIB_ORGANIC);
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
			ThrowGib (self, "models/objects/gibs/bone2/tris.md2", damage, GIB_ORGANIC);
		}
		ThrowClientHead (self, damage);

		self->takedamage = DAMAGE_NO;
	}
	else
	{	// normal death
		if (!self->deadflag)
		{
			static int i;

			i = (i+1)%3;
			// start a death animation
			self->client->anim_priority = ANIM_DEATH;
			if (self->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				self->s.frame = FRAME_crdeath1-1;
				self->client->anim_end = FRAME_crdeath5;
			}
			else switch (i)
			{
			case 0:
				self->s.frame = FRAME_death101-1;
				self->client->anim_end = FRAME_death106;
				break;
			case 1:
				self->s.frame = FRAME_death201-1;
				self->client->anim_end = FRAME_death206;
				break;
			case 2:
				self->s.frame = FRAME_death301-1;
				self->client->anim_end = FRAME_death308;
				break;
			}
			gi.sound (self, CHAN_VOICE, gi.soundindex(va("*death%i.wav", (rand()%4)+1)), 1, ATTN_NORM, 0);
		}
	}

#ifdef JETPACK_MOD
	if ( self->client->jetpack )
	{
		Jet_BecomeExplosion( self, damage );
		/*stop jetting when dead*/
		self->client->jetpack_framenum = 0;
		self->client->jetpack = false;
		// DWH: force player to gib
	//	self->health = self->gib_health-1;
		self->health = player_gib_health->value-1;
	}
#endif

	self->deadflag = DEAD_DEAD;
	gi.linkentity (self);

}

//=======================================================================

void SwitchToBestStartWeapon (gclient_t *client)
{
	if (!client)
		return;

	if ( client->pers.inventory[slugs_index]
		&&  client->pers.inventory[ITEM_INDEX(FindItem("railgun"))] )
	{
		client->pers.weapon = FindItem ("railgun");
		return;
	}
	if ( client->pers.inventory[cells_index]
		&&  client->pers.inventory[ITEM_INDEX(FindItem("hyperblaster"))] )
	{
		client->pers.weapon = FindItem ("hyperblaster");
		return;
	}
	if ( client->pers.inventory[bullets_index]
		&&  client->pers.inventory[ITEM_INDEX(FindItem("chaingun"))] )
	{
		client->pers.weapon = FindItem ("chaingun");
		return;
	}
	if ( client->pers.inventory[bullets_index]
		&&  client->pers.inventory[ITEM_INDEX(FindItem("machinegun"))] )
	{
		client->pers.weapon = FindItem ("machinegun");
		return;
	}
	if ( client->pers.inventory[shells_index] > 1
		&&  client->pers.inventory[ITEM_INDEX(FindItem("super shotgun"))] )
	{
		client->pers.weapon = FindItem ("super shotgun");
		return;
	}
	if ( client->pers.inventory[shells_index]
		&&  client->pers.inventory[ITEM_INDEX(FindItem("shotgun"))] )
	{
		client->pers.weapon = FindItem ("shotgun");
		return;
	}
	// DWH: Dude may not HAVE a blaster
	//ent->client->newweapon = FindItem ("blaster");
	if ( client->pers.inventory[ITEM_INDEX(FindItem("blaster"))] )
		client->pers.weapon = FindItem ("blaster");
	else
		client->pers.weapon = FindItem ("No Weapon");
}

void SelectStartWeapon (gclient_t *client, int style)
{
	gitem_t	*item;
	int		n;

	// Lazarus: We allow choice of weapons (or no weapon) at startup
	// If style is non-zero, first clear player inventory of all
	// weapons and ammo that might have been passed over through
	// target_changelevel or acquired when previously called by 
	// InitClientPersistant
	if(style)
	{
		for (n = 0; n < MAX_ITEMS; n++)
		{
			if (itemlist[n].flags & IT_WEAPON)
				client->pers.inventory[n] = 0;
		}
		client->pers.inventory[shells_index]   = 0;
		client->pers.inventory[bullets_index]  = 0;
		client->pers.inventory[grenades_index] = 0;
		client->pers.inventory[rockets_index]  = 0;
		client->pers.inventory[cells_index]    = 0;
		client->pers.inventory[slugs_index]    = 0;
		client->pers.inventory[homing_index]   = 0;
	}

	switch(style)
	{
	case -1:
		item = FindItem("No Weapon");
		break;
	case -2:
	case  2:
		item = FindItem("Shotgun");
		break;
	case -3:
	case  3:
		item = FindItem("Super Shotgun");
		break;
	case -4:
	case  4:
		item = FindItem("Machinegun");
		break;
	case -5:
	case  5:
		item = FindItem("Chaingun");
		break;
	case -6:
	case  6:
		item = FindItem("Grenade Launcher");
		break;
	case -7:
	case  7:
		item = FindItem("Rocket Launcher");
		break;
	case -8:
	case  8:
		item = FindItem("HyperBlaster");
		break;
	case -9:
	case  9:
		item = FindItem("Railgun");
		break;
	case -10:
	case  10:
		item = FindItem("BFG10K");
		break;
	default:
		item = FindItem("Blaster");
		break;
	}
	client->pers.selected_item = ITEM_INDEX(item);
	client->pers.inventory[client->pers.selected_item] = 1;
	client->pers.weapon = item;

	// Lazarus: If default weapon is NOT "No Weapon", then give player
	//          a blaster
	if(style > 1)
		client->pers.inventory[ITEM_INDEX(FindItem("Blaster"))] = 1;

	// and give him standard ammo
	if (item->ammo)
	{
		gitem_t	*ammo;

		ammo = FindItem (item->ammo);
		if ( deathmatch->value && ((int)dmflags->value & DF_INFINITE_AMMO) )
			client->pers.inventory[ITEM_INDEX(ammo)] += 1000;
		else
			client->pers.inventory[ITEM_INDEX(ammo)] += ammo->quantity;
	}

	// Knightmare- DM start values
	if (deathmatch->value)
	{
		client->pers.inventory[ITEM_INDEX(FindItem("Shells"))] = sk_dm_start_shells->value;
		client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] = sk_dm_start_bullets->value;
		client->pers.inventory[ITEM_INDEX(FindItem("Rockets"))] = sk_dm_start_rockets->value;
		client->pers.inventory[ITEM_INDEX(FindItem("Homing Rockets"))] = sk_dm_start_homing->value;
		client->pers.inventory[ITEM_INDEX(FindItem("Grenades"))] = sk_dm_start_grenades->value;
		client->pers.inventory[ITEM_INDEX(FindItem("Cells"))] = sk_dm_start_cells->value;
		client->pers.inventory[ITEM_INDEX(FindItem("Slugs"))] = sk_dm_start_slugs->value;

		client->pers.inventory[ITEM_INDEX(FindItem("Shotgun"))] = sk_dm_start_shotgun->value;
		client->pers.inventory[ITEM_INDEX(FindItem("Super Shotgun"))] = sk_dm_start_sshotgun->value;
		client->pers.inventory[ITEM_INDEX(FindItem("Machinegun"))] = sk_dm_start_machinegun->value;
		client->pers.inventory[ITEM_INDEX(FindItem("Chaingun"))] = sk_dm_start_chaingun->value;
		client->pers.inventory[ITEM_INDEX(FindItem("Grenade Launcher"))] = sk_dm_start_grenadelauncher->value;
		client->pers.inventory[ITEM_INDEX(FindItem("Rocket Launcher"))] = sk_dm_start_rocketlauncher->value;
		client->pers.inventory[ITEM_INDEX(FindItem("Homing Rocket Launcher"))] = sk_dm_start_rocketlauncher->value;
		client->pers.inventory[ITEM_INDEX(FindItem("HyperBlaster"))] = sk_dm_start_hyperblaster->value;
		client->pers.inventory[ITEM_INDEX(FindItem("Railgun"))] = sk_dm_start_railgun->value;
		client->pers.inventory[ITEM_INDEX(FindItem("BFG10K"))] = sk_dm_start_bfg->value;
		SwitchToBestStartWeapon (client);
	}
}

/*
==============
InitClientPersistant

This is only called when the game first initializes in single player,
but is called after each death and level change in deathmatch
==============
*/
void InitClientPersistant (gclient_t *client, int style)
{
	memset (&client->pers, 0, sizeof(client->pers));

	client->homing_rocket = NULL;
	SelectStartWeapon (client, style);

	client->pers.health			= 100;
	if (deathmatch->value)
		client->pers.max_health	= sk_max_health_dm->value;
	else
		client->pers.max_health	= sk_max_health->value;

	client->pers.max_bullets	= sk_max_bullets->value;
	client->pers.max_shells		= sk_max_shells->value;
	client->pers.max_rockets	= sk_max_rockets->value;
	client->pers.max_grenades	= sk_max_grenades->value;
	client->pers.max_cells		= sk_max_cells->value;
	client->pers.max_slugs		= sk_max_slugs->value;
	client->pers.max_fuel       = sk_max_fuel->value;
	client->pers.max_homing_rockets = sk_max_rockets->value;
	client->pers.fire_mode      = 0;  // Lazarus alternate fire mode

	client->pers.connected = true;
// tpp
	//Default chasecam to tpp setting
    client->pers.chasetoggle = tpp->value;
// end tpp

	// Lazarus
	client->zooming = 0;
	client->zoomed = false;
	client->spycam = NULL;
	client->pers.spawn_landmark = false;
	client->pers.spawn_levelchange = false;
}


void InitClientResp (gclient_t *client)
{
	memset (&client->resp, 0, sizeof(client->resp));
	client->resp.enterframe = level.framenum;
	client->resp.coop_respawn = client->pers;
}

/*
==================
SaveClientData

Some information that should be persistant, like health, 
is still stored in the edict structure, so it needs to
be mirrored out to the client structure before all the
edicts are wiped.
==================
*/
void SaveClientData (void)
{
	int		i;
	edict_t	*ent;

	for (i=0 ; i<game.maxclients ; i++)
	{
		ent = &g_edicts[1+i];
		if (!ent->inuse)
			continue;
		// tpp
		game.clients[i].pers.chasetoggle = ent->client->pers.chasetoggle;
        // end tpp
		game.clients[i].pers.newweapon = ent->client->newweapon;
		game.clients[i].pers.health = ent->health;
		game.clients[i].pers.max_health = ent->max_health;
		game.clients[i].pers.savedFlags = (ent->flags & (FL_GODMODE|FL_NOTARGET|FL_POWER_SHIELD|FL_POWER_SCREEN));
		if (coop->value)
			game.clients[i].pers.score = ent->client->resp.score;
	}
}

void FetchClientEntData (edict_t *ent)
{
	ent->health = ent->client->pers.health;
	ent->gib_health = player_gib_health->value; // was -40
	ent->max_health = ent->client->pers.max_health;
	ent->flags |= ent->client->pers.savedFlags;
	if (coop->value)
		ent->client->resp.score = ent->client->pers.score;
}



/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
PlayersRangeFromSpot

Returns the distance to the nearest player from the given spot
================
*/
float	PlayersRangeFromSpot (edict_t *spot)
{
	edict_t	*player;
	float	bestplayerdistance;
	vec3_t	v;
	int		n;
	float	playerdistance;


	bestplayerdistance = 9999999;

	for (n = 1; n <= maxclients->value; n++)
	{
		player = &g_edicts[n];

		if (!player->inuse)
			continue;

		if (player->health <= 0)
			continue;

		VectorSubtract (spot->s.origin, player->s.origin, v);
		playerdistance = VectorLength (v);

		if (playerdistance < bestplayerdistance)
			bestplayerdistance = playerdistance;
	}

	return bestplayerdistance;
}

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point, but NOT the two points closest
to other players
================
*/
edict_t *SelectRandomDeathmatchSpawnPoint (void)
{
	edict_t	*spot, *spot1, *spot2;
	int		count = 0;
	int		selection;
	float	range, range1, range2;

	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		count++;
		range = PlayersRangeFromSpot(spot);
		if (range < range1)
		{
			range1 = range;
			spot1 = spot;
		}
		else if (range < range2)
		{
			range2 = range;
			spot2 = spot;
		}
	}

	if (!count)
		return NULL;

	if (count <= 2)
	{
		spot1 = spot2 = NULL;
	}
	// Lazarus: This is wrong. If there is no spot1 or spot2, all spots should
	// be valid.
//	else
//		count -= 2;
	else
	{
		if(spot1) count--;
		if(spot2) count--;
	}

	selection = rand() % count;

	spot = NULL;
	do
	{
		spot = G_Find (spot, FOFS(classname), "info_player_deathmatch");
		if (spot == spot1 || spot == spot2)
			selection++;
	} while(selection--);

	return spot;
}

/*
================
SelectFarthestDeathmatchSpawnPoint

================
*/
edict_t *SelectFarthestDeathmatchSpawnPoint (void)
{
	edict_t	*bestspot;
	float	bestdistance, bestplayerdistance;
	edict_t	*spot;


	spot = NULL;
	bestspot = NULL;
	bestdistance = 0;
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		bestplayerdistance = PlayersRangeFromSpot (spot);

		if (bestplayerdistance > bestdistance)
		{
			bestspot = spot;
			bestdistance = bestplayerdistance;
		}
	}

	if (bestspot)
	{
		return bestspot;
	}

	// if there is a player just spawned on each and every start spot
	// we have no choice to turn one into a telefrag meltdown
	spot = G_Find (NULL, FOFS(classname), "info_player_deathmatch");

	return spot;
}

edict_t *SelectDeathmatchSpawnPoint (void)
{
	if ( (int)(dmflags->value) & DF_SPAWN_FARTHEST)
		return SelectFarthestDeathmatchSpawnPoint ();
	else
		return SelectRandomDeathmatchSpawnPoint ();
}


edict_t *SelectCoopSpawnPoint (edict_t *ent)
{
	int		index;
	edict_t	*spot = NULL;
	char	*target;

	index = ent->client - game.clients;

	// player 0 starts in normal player spawn point
	if (!index)
		return NULL;

	spot = NULL;

	// assume there are four coop spots at each spawnpoint
	while (1)
	{
		spot = G_Find (spot, FOFS(classname), "info_player_coop");
		if (!spot)
			return NULL;	// we didn't have enough...

		target = spot->targetname;
		if (!target)
			target = "";
		if ( Q_stricmp(game.spawnpoint, target) == 0 )
		{	// this is a coop spawn point for one of the clients here
			index--;
			if (!index)
				return spot;		// this is it
		}
	}
	return spot;
}


/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, coop start, etc
============
*/
void	SelectSpawnPoint (edict_t *ent, vec3_t origin, vec3_t angles, int *style, int *health)
{
	edict_t	*spot = NULL;

	if (deathmatch->value)
		spot = SelectDeathmatchSpawnPoint ();
	else if (coop->value)
		spot = SelectCoopSpawnPoint (ent);

	// find a single player start spot
	if (!spot)
	{
		while ((spot = G_Find (spot, FOFS(classname), "info_player_start")) != NULL)
		{
			if (!game.spawnpoint[0] && !spot->targetname)
				break;

			if (!game.spawnpoint[0] || !spot->targetname)
				continue;

			if (Q_stricmp(game.spawnpoint, spot->targetname) == 0)
				break;
		}

		if (!spot)
		{
			if (!game.spawnpoint[0])
			{	// there wasn't a spawnpoint without a target, so use any
				spot = G_Find (spot, FOFS(classname), "info_player_start");
			}
			if (!spot)
				gi.error ("Couldn't find spawn point %s\n", game.spawnpoint);
		}
	}

	if (spot)
	{
		*style = spot->style;
		*health = spot->health;
		VectorCopy(spot->s.origin, origin);
		origin[2] += 9;
		VectorCopy(spot->s.angles, angles);

		if (!deathmatch->value && !coop->value) {

			spot->count--;
			if (!spot->count) {
				spot->think = G_FreeEdict;
				spot->nextthink = level.time + 1;
			}
		}
	}
}

//======================================================================


void InitBodyQue (void)
{
	// DWH: bodyque isn't used in SP, so why reserve space for it?
	if (deathmatch->value || coop->value) {

		int		i;
		edict_t	*ent;

		level.body_que = 0;
		for (i=0; i<BODY_QUEUE_SIZE ; i++)
		{
			ent = G_Spawn();
			ent->classname = "bodyque";
		}
	}
}

void body_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int	n;

//	if (self->health < self->gib_health)
	if (self->health < player_gib_health->value)
	{
		int num_giblets = 4;
		//if (deathmatch->value && (self->health < (self->gib_health*2)))
		if (deathmatch->value && (self->health < (player_gib_health->value*2)))
			num_giblets = 8;

		gi.sound (self, CHAN_BODY, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n= 0; n < num_giblets; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		if (mega_gibs->value)
		{
			ThrowGib (self, "models/objects/gibs/arm/tris.md2", damage, GIB_ORGANIC);
			ThrowGib (self, "models/objects/gibs/arm/tris.md2", damage, GIB_ORGANIC);
			ThrowGib (self, "models/objects/gibs/leg/tris.md2", damage, GIB_ORGANIC);
			ThrowGib (self, "models/objects/gibs/leg/tris.md2", damage, GIB_ORGANIC);
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
			ThrowGib (self, "models/objects/gibs/bone2/tris.md2", damage, GIB_ORGANIC);
		}
		self->s.origin[2] -= 48;
		ThrowClientHead (self, damage);
		self->takedamage = DAMAGE_NO;
	}
}

void CopyToBodyQue (edict_t *ent)
{
	edict_t		*body;

	// grab a body que and cycle to the next one
	body = &g_edicts[(int)maxclients->value + level.body_que + 1];
	level.body_que = (level.body_que + 1) % BODY_QUEUE_SIZE;

	// FIXME: send an effect on the removed body

	gi.unlinkentity (ent);

	gi.unlinkentity (body);
	body->s = ent->s;
	body->s.number = body - g_edicts;

	body->svflags = ent->svflags;
	VectorCopy (ent->mins, body->mins);
	VectorCopy (ent->maxs, body->maxs);
	VectorCopy (ent->absmin, body->absmin);
	VectorCopy (ent->absmax, body->absmax);
	VectorCopy (ent->size, body->size);
	body->solid = ent->solid;
	body->clipmask = ent->clipmask;
	body->owner = ent->owner;
	body->movetype = ent->movetype;

	body->die = body_die;
	body->takedamage = DAMAGE_YES;

	gi.linkentity (body);
}


void respawn (edict_t *self)
{
	// tpp
	if (self->crosshair)
		G_FreeEdict(self->crosshair);
	self->crosshair = NULL;
	if (self->client->oldplayer)
		G_FreeEdict (self->client->oldplayer);
	self->client->oldplayer = NULL;
	if (self->client->chasecam)
		G_FreeEdict (self->client->chasecam);
	self->client->chasecam = NULL;
	// end tpp
	if (deathmatch->value || coop->value)
	{
		// spectator's don't leave bodies
		if (self->movetype != MOVETYPE_NOCLIP)
			CopyToBodyQue (self);
		self->svflags &= ~SVF_NOCLIENT;
		PutClientInServer (self);

		// add a teleportation effect
		self->s.event = EV_PLAYER_TELEPORT;

		// hold in place briefly
		self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		self->client->ps.pmove.pm_time = 14;

		self->client->respawn_time = level.time;

		return;
	}
	// restart the entire server
	gi.AddCommandString ("menu_loadgame\n");
}

/* 
 * only called when pers.spectator changes
 * note that resp.spectator should be the opposite of pers.spectator here
 */
void spectator_respawn (edict_t *ent)
{
	int i, numspec;

	// if the user wants to become a spectator, make sure he doesn't
	// exceed max_spectators

	if (ent->client->pers.spectator) {
		char *value = Info_ValueForKey (ent->client->pers.userinfo, "spectator");
		if (*spectator_password->string && 
			strcmp(spectator_password->string, "none") && 
			strcmp(spectator_password->string, value)) {
			gi.cprintf(ent, PRINT_HIGH, "Spectator password incorrect.\n");
			ent->client->pers.spectator = false;
			gi.WriteByte (svc_stufftext);
			gi.WriteString ("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}

		// count spectators
		for (i = 1, numspec = 0; i <= maxclients->value; i++)
			if (g_edicts[i].inuse && g_edicts[i].client->pers.spectator)
				numspec++;

		if (numspec >= maxspectators->value) {
			gi.cprintf(ent, PRINT_HIGH, "Server spectator limit is full.");
			ent->client->pers.spectator = false;
			// reset his spectator var
			gi.WriteByte (svc_stufftext);
			gi.WriteString ("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}
	} else {
		// he was a spectator and wants to join the game
		// he must have the right password
		char *value = Info_ValueForKey (ent->client->pers.userinfo, "password");
		if (*password->string && strcmp(password->string, "none") && 
			strcmp(password->string, value)) {
			gi.cprintf(ent, PRINT_HIGH, "Password incorrect.\n");
			ent->client->pers.spectator = true;
			gi.WriteByte (svc_stufftext);
			gi.WriteString ("spectator 1\n");
			gi.unicast(ent, true);
			return;
		}
	}

	// clear client on respawn
	ent->client->resp.score = ent->client->pers.score = 0;

	ent->svflags &= ~SVF_NOCLIENT;
	PutClientInServer (ent);

	// add a teleportation effect
	if (!ent->client->pers.spectator)  {
		// send effect
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_LOGIN);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		// hold in place briefly
		ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		ent->client->ps.pmove.pm_time = 14;
	}

	ent->client->respawn_time = level.time;

	if (ent->client->pers.spectator) 
		gi.bprintf (PRINT_HIGH, "%s has moved to the sidelines\n", ent->client->pers.netname);
	else
		gi.bprintf (PRINT_HIGH, "%s joined the game\n", ent->client->pers.netname);
}

//==============================================================


/*
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
void PutClientInServer (edict_t *ent)
{
	extern	int			nostatus;
	vec3_t				mins = {-16, -16, -24};
	vec3_t				maxs = {16, 16, 32};
	int					index;
	vec3_t				spawn_origin, spawn_angles, spawn_viewangles;
	gclient_t			*client;
	int		i;
	// tpp
	int					chasetoggle;
	gitem_t				*newweapon;
    char				userinfo[MAX_INFO_STRING];
	// end tpp
	qboolean			spawn_landmark;
	qboolean			spawn_levelchange;
	int					spawn_gunframe;
	int					spawn_modelframe;
	int					spawn_anim_end;
	int					spawn_pm_flags=0;
	int					spawn_style;
	int					spawn_health;
	client_persistant_t	saved;
	client_respawn_t	resp;

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	SelectSpawnPoint (ent, spawn_origin, spawn_angles, &spawn_style, &spawn_health);

	index = ent-g_edicts-1;
	client = ent->client;
	// tpp
	chasetoggle = client->pers.chasetoggle;
    // end tpp
	newweapon = client->pers.newweapon;
	spawn_landmark   = client->pers.spawn_landmark;
	spawn_levelchange= client->pers.spawn_levelchange;
	spawn_gunframe   = client->pers.spawn_gunframe;
	spawn_modelframe = client->pers.spawn_modelframe;
	spawn_anim_end   = client->pers.spawn_anim_end;
	client->pers.spawn_landmark = false;
	client->pers.spawn_levelchange = false;

	if(spawn_landmark)
	{
		spawn_origin[2] -= 9;
		VectorAdd(spawn_origin,client->pers.spawn_offset,spawn_origin);
		VectorCopy(client->pers.spawn_angles,spawn_angles);
		VectorCopy(client->pers.spawn_viewangles,spawn_viewangles);
		VectorCopy(client->pers.spawn_velocity,ent->velocity);
		spawn_pm_flags = client->pers.spawn_pm_flags;
	}

	// deathmatch wipes most client data every spawn
	if (deathmatch->value)
	{
		char		userinfo[MAX_INFO_STRING];

		resp = client->resp;
		memcpy (userinfo, client->pers.userinfo, sizeof(userinfo));
		InitClientPersistant (client,spawn_style);
		ClientUserinfoChanged (ent, userinfo);
	}
	else if (coop->value)
	{
//		int			n;
		char		userinfo[MAX_INFO_STRING];

		resp = client->resp;
		memcpy (userinfo, client->pers.userinfo, sizeof(userinfo));
		resp.coop_respawn.game_helpchanged = client->pers.game_helpchanged;
		resp.coop_respawn.helpchanged = client->pers.helpchanged;
		client->pers = resp.coop_respawn;
		ClientUserinfoChanged (ent, userinfo);
		if (resp.score > client->pers.score)
			client->pers.score = resp.score;
	}
	else
	{
		memset (&resp, 0, sizeof(resp));
	}
	// tpp
	// A bug in Q2 that you couldn't see without thirdpp
	memcpy (userinfo, client->pers.userinfo, sizeof(userinfo));
	ClientUserinfoChanged (ent, userinfo);
	// end tpp

	// clear everything but the persistant data
	saved = client->pers;
	memset (client, 0, sizeof(*client));
	client->pers = saved;
	if (client->pers.health <= 0)
		InitClientPersistant(client,spawn_style);
	else if(spawn_style)
		SelectStartWeapon(client,spawn_style);

	client->resp = resp;
	// tpp
	client->pers.chasetoggle = chasetoggle;
	// end tpp
	client->pers.newweapon = newweapon;

	// copy some data from the client to the entity
	FetchClientEntData (ent);

	// Lazarus: Starting health < max. Presumably player was hurt in a crash
	if( (spawn_health > 0) && !deathmatch->value && !coop->value)
		ent->health = min(ent->health, spawn_health);

	// clear entity values
	ent->groundentity = NULL;
	ent->client = &game.clients[index];
	ent->takedamage = DAMAGE_AIM;
	ent->movetype = MOVETYPE_WALK;
	ent->viewheight = 22;
	ent->inuse = true;
	ent->classname = "player";
	ent->mass = 200;
	ent->solid = SOLID_BBOX;
	ent->deadflag = DEAD_NO;
	ent->air_finished = level.time + 12;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->model = "players/male/tris.md2";
	ent->pain = player_pain;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags &= ~FL_NO_KNOCKBACK;
	ent->svflags &= ~SVF_DEADMONSTER;
	// tpp
	ent->svflags &= ~SVF_NOCLIENT;
	// turn on prediction
	ent->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
	// end tpp
	ent->client->spycam = NULL;
	ent->client->camplayer = NULL;

	VectorCopy (mins, ent->mins);
	VectorCopy (maxs, ent->maxs);

	if(!spawn_landmark)
		VectorClear (ent->velocity);

	// clear playerstate values
	memset (&ent->client->ps, 0, sizeof(client->ps));

	if(spawn_landmark)
		client->ps.pmove.pm_flags = spawn_pm_flags;

	client->ps.pmove.origin[0] = spawn_origin[0]*8;
	client->ps.pmove.origin[1] = spawn_origin[1]*8;
	client->ps.pmove.origin[2] = spawn_origin[2]*8;

	if (deathmatch->value && ((int)dmflags->value & DF_FIXED_FOV))
	{
		client->ps.fov = 90;
	}
	else
	{
		client->ps.fov = atoi(Info_ValueForKey(client->pers.userinfo, "fov"));
		if (client->ps.fov < 1)
			client->ps.fov = 90;
		else if (client->ps.fov > 160)
			client->ps.fov = 160;
	}
	// DWH
	client->original_fov  = client->ps.fov;
	// end DWH

	client->ps.gunindex = gi.modelindex(client->pers.weapon->view_model);

	// Server-side speed control stuff
#ifdef KMQUAKE2_ENGINE_MOD
	client->ps.maxspeed = player_max_speed->value;
	client->ps.duckspeed = player_crouch_speed->value;
	client->ps.accel = player_accel->value;
	client->ps.stopspeed = player_stopspeed->value;
#endif

	// clear entity state values
	ent->s.effects = 0;
	ent->s.modelindex = (MAX_MODELS-1);		// was 255, will use the skin specified model

	if(ITEM_INDEX(client->pers.weapon) == noweapon_index)
		ent->s.modelindex2 = 0;
	else
		ent->s.modelindex2 = (MAX_MODELS-1);	// was 255, custom gun model

	// sknum is player num and weapon number
	// weapon number will be added in changeweapon
	ent->s.skinnum = ent - g_edicts - 1;

	ent->s.frame = 0;
	VectorCopy (spawn_origin, ent->s.origin);
	ent->s.origin[2] += 1;	// make sure off ground
	VectorCopy (ent->s.origin, ent->s.old_origin);

	// set the delta angle
	for (i=0 ; i<3 ; i++)
	{
		client->ps.pmove.delta_angles[i] = ANGLE2SHORT(spawn_angles[i] - client->resp.cmd_angles[i]);
	}

	ent->s.angles[PITCH] = ent->s.angles[ROLL]  = 0;
	ent->s.angles[YAW]   = spawn_angles[YAW];
	if(spawn_landmark)
	{
		VectorCopy(spawn_viewangles, client->ps.viewangles);
//		client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
	}
	else
		VectorCopy(ent->s.angles,    client->ps.viewangles);
	VectorCopy (client->ps.viewangles, client->v_angle);

	// spawn a spectator
	if (client->pers.spectator) {
		client->chase_target = NULL;

		client->resp.spectator = true;

		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->ps.gunindex = 0;
		gi.linkentity (ent);
		return;
	} else
		client->resp.spectator = false;

	// DWH:
	client->flashlight = false;
	client->secs_per_frame = 0.025;		// assumed 40 fps until we know better
	client->fps_time_start = level.time;

	if (!KillBox (ent))
	{	// could't spawn in?
	}

	gi.linkentity (ent);

	// tpp
	client->chasetoggle = 0;
	// If chasetoggle set then turn on (delayed start of 5 frames - 0.5s)
	if(client->pers.chasetoggle)
		client->delayedstart = 5;
	// end tpp

	if(spawn_levelchange && !client->pers.chasetoggle && !client->pers.newweapon)
	{
		// we already had a weapon when the level changed... no need to bring it up
		int	i;

		client->pers.lastweapon  = client->pers.weapon;
		client->newweapon        = NULL;
		client->machinegun_shots = 0;
		i = ((client->pers.weapon->weapmodel & 0xff) << 8);
		ent->s.skinnum = (ent - g_edicts - 1) | i;
		if (client->pers.weapon->ammo)
			client->ammo_index = ITEM_INDEX(FindItem(client->pers.weapon->ammo));
		else
			client->ammo_index = 0;
		client->weaponstate = WEAPON_READY;
		client->ps.gunframe = 0;
		client->ps.gunindex = gi.modelindex(client->pers.weapon->view_model);
		client->ps.gunframe = spawn_gunframe;
		ent->s.frame        = spawn_modelframe;
		client->anim_end    = spawn_anim_end;
	}
	else
	{
		// force the current weapon up
		client->newweapon = client->pers.weapon;
		ChangeWeapon (ent);
	}

	// Paril's fix for this getting reset after map changes
	if (!ent->client->pers.connected)
		ent->client->pers.connected = true;
}

/*
=====================
ClientBeginDeathmatch

A client has just connected to the server in 
deathmatch mode, so clear everything out before starting them.
=====================
*/
void ClientBeginDeathmatch (edict_t *ent)
{
	G_InitEdict (ent);

	InitClientResp (ent->client);

	// locate ent at a spawn point
	PutClientInServer (ent);

	if (level.intermissiontime)
	{
		MoveClientToIntermission (ent);
	}
	else
	{
		// send effect
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_LOGIN);
		gi.multicast (ent->s.origin, MULTICAST_PVS);
	}

	gi.bprintf (PRINT_HIGH, "%s entered the game\n", ent->client->pers.netname);

	// make sure all view stuff is valid
	ClientEndServerFrame (ent);
}


/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the game.  This will happen every level load.
============
*/
void ClientBegin (edict_t *ent)
{
	int		i;

	ent->client = game.clients + (ent - g_edicts - 1);

	// Lazarus: Set the alias for our alternate attack
	stuffcmd(ent, "alias +attack2 attack2_on; alias -attack2 attack2_off\n");
	
	if (deathmatch->value)
	{
		ClientBeginDeathmatch (ent);
		return;
	}

	Fog_Off();

	stuffcmd(ent,"alias +zoomin zoomin;alias -zoomin zoominstop\n");
	stuffcmd(ent,"alias +zoomout zoomout;alias -zoomout zoomoutstop\n");
	stuffcmd(ent,"alias +zoom zoomon;alias -zoom zoomoff\n");

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent->inuse == true)
	{
		// the client has cleared the client side viewangles upon
		// connecting to the server, which is different than the
		// state when the game is saved, so we need to compensate
		// with deltaangles
		for (i=0 ; i<3 ; i++)
			ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->client->ps.viewangles[i]);
	}
	else
	{
		// a spawn point will completely reinitialize the entity
		// except for the persistant data that was initialized at
		// ClientConnect() time
		G_InitEdict (ent);
		ent->classname = "player";
		InitClientResp (ent->client);
		PutClientInServer (ent);
	}

	if (level.intermissiontime)
	{
		MoveClientToIntermission (ent);
	}
	else
	{
		// send effect if in a multiplayer game
		if (game.maxclients > 1)
		{
			gi.WriteByte (svc_muzzleflash);
			gi.WriteShort (ent-g_edicts);
			gi.WriteByte (MZ_LOGIN);
			gi.multicast (ent->s.origin, MULTICAST_PVS);

			gi.bprintf (PRINT_HIGH, "%s entered the game\n", ent->client->pers.netname);
		}
	}

	// DWH
	SetLazarusCrosshair(ent); //backup crosshair
	SetSensitivities(ent,true);

	if (game.maxclients == 1)
	{
		// For SP games, check for monsters who were mad at player
		// in previous level and have changed levels with the player
		edict_t	*monster;
		for(i=2; i<globals.num_edicts; i++)
		{
			monster = &g_edicts[i];
			if(!monster->inuse)
				continue;
			if(!(monster->svflags & SVF_MONSTER))
				continue;
			if(monster->health <= 0)
				continue;
			if(monster->monsterinfo.aiflags & AI_RESPAWN_FINDPLAYER)
			{
				monster->monsterinfo.aiflags &= ~AI_RESPAWN_FINDPLAYER;
				if(!monster->enemy)
				{
					monster->enemy = ent;
					FoundTarget(monster);
				}
			}
		}
	}

	// make sure all view stuff is valid
	ClientEndServerFrame (ent);
}

/*
===========
ClientUserInfoChanged

called whenever the player updates a userinfo variable.

The game can override any of the settings in place
(forcing skins or names, etc) before copying it off.
============
*/
void ClientUserinfoChanged (edict_t *ent, char *userinfo)
{
	char	*s;
	int		playernum;

	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo))
	{
		strcpy (userinfo, "\\name\\badinfo\\skin\\male/grunt");
	}

	// set name
	s = Info_ValueForKey (userinfo, "name");
	strncpy (ent->client->pers.netname, s, sizeof(ent->client->pers.netname)-1);

	// set spectator
	s = Info_ValueForKey (userinfo, "spectator");
	// spectators are only supported in deathmatch
	if (deathmatch->value && *s && strcmp(s, "0"))
		ent->client->pers.spectator = true;
	else
		ent->client->pers.spectator = false;

	// set skin
	s = Info_ValueForKey (userinfo, "skin");

	playernum = ent-g_edicts-1;

	// combine name and skin into a configstring
	gi.configstring (CS_PLAYERSKINS+playernum, va("%s\\%s", ent->client->pers.netname, s) );

	// fov
	if (deathmatch->value && ((int)dmflags->value & DF_FIXED_FOV))
	{
		ent->client->ps.fov = 90;
		ent->client->original_fov = ent->client->ps.fov;
	}
	else
	{
		float	new_fov;

		new_fov = atoi(Info_ValueForKey(userinfo, "fov"));
		if (new_fov < 1)
			new_fov = 90;
		else if (new_fov > 160)
			new_fov = 160;
		if(new_fov != ent->client->original_fov) {
			ent->client->ps.fov = new_fov;
			ent->client->original_fov = new_fov;
		}
	}

	// handedness
	s = Info_ValueForKey (userinfo, "hand");
	if (strlen(s))
		ent->client->pers.hand = atoi(s);

	// save off the userinfo in case we want to check something later
	strncpy (ent->client->pers.userinfo, userinfo, sizeof(ent->client->pers.userinfo)-1);
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
qboolean ClientConnect (edict_t *ent, char *userinfo)
{
	char	*value;

	// check to see if they are on the banned IP list
	value = Info_ValueForKey (userinfo, "ip");
	if (SV_FilterPacket(value)) {
		Info_SetValueForKey(userinfo, "rejmsg", "Banned.");
		return false;
	}

	// check for a spectator
	value = Info_ValueForKey (userinfo, "spectator");
	if (deathmatch->value && *value && strcmp(value, "0")) {
		int i, numspec;

		if (*spectator_password->string && 
			strcmp(spectator_password->string, "none") && 
			strcmp(spectator_password->string, value)) {
			Info_SetValueForKey(userinfo, "rejmsg", "Spectator password required or incorrect.");
			return false;
		}

		// count spectators
		for (i = numspec = 0; i < maxclients->value; i++)
			if (g_edicts[i+1].inuse && g_edicts[i+1].client->pers.spectator)
				numspec++;

		if (numspec >= maxspectators->value) {
			Info_SetValueForKey(userinfo, "rejmsg", "Server spectator limit is full.");
			return false;
		}
	} else {
		// check for a password
		value = Info_ValueForKey (userinfo, "password");
		if (*password->string && strcmp(password->string, "none") && 
			strcmp(password->string, value)) {
			Info_SetValueForKey(userinfo, "rejmsg", "Password required or incorrect.");
			return false;
		}
	}


	// they can connect
	ent->client = game.clients + (ent - g_edicts - 1);

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent->inuse == false)
	{
		// clear the respawning variables
		InitClientResp (ent->client);
		if (!game.autosaved || !ent->client->pers.weapon)
			InitClientPersistant (ent->client,world->style);
	}

	ClientUserinfoChanged (ent, userinfo);

	if (game.maxclients > 1)
		gi.dprintf ("%s connected\n", ent->client->pers.netname);

	ent->svflags = 0; // make sure we start with known default
	ent->client->pers.connected = true;

	return true;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void ClientDisconnect (edict_t *ent)
{
	int		playernum;

	if (!ent->client)
		return;

	// tpp
	if(ent->client->chasetoggle)
		ChasecamRemove(ent,OPTION_OFF);
	// end tpp

	// DWH
	ent->client->zooming = 0;
	ent->client->zoomed = false;
	SetSensitivities(ent,true);
	// end DWH

	if(ent->client->textdisplay) Text_Close(ent);

	gi.bprintf (PRINT_HIGH, "%s disconnected\n", ent->client->pers.netname);

	// send effect
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_LOGOUT);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	gi.unlinkentity (ent);
	ent->s.modelindex = 0;
	ent->solid = SOLID_NOT;
	ent->inuse = false;
	ent->classname = "disconnected";
	ent->client->pers.connected = false;

	if(ent->client->spycam)
		camera_off(ent);

	playernum = ent-g_edicts-1;
	gi.configstring (CS_PLAYERSKINS+playernum, "");

}


//==============================================================


edict_t	*pm_passent;

// pmove doesn't need to know about passent and contentmask
trace_t	PM_trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	if (pm_passent->health > 0)
		return gi.trace (start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
	else
		return gi.trace (start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
}

unsigned CheckBlock (void *b, int c)
{
	int	v,i;
	v = 0;
	for (i=0 ; i<c ; i++)
		v+= ((byte *)b)[i];
	return v;
}
void PrintPmove (pmove_t *pm)
{
	unsigned	c1, c2;

	c1 = CheckBlock (&pm->s, sizeof(pm->s));
	c2 = CheckBlock (&pm->cmd, sizeof(pm->cmd));
	Com_Printf ("sv %3i:%i %i\n", pm->cmd.impulse, c1, c2);
}

// DWH
//==========================================================================
// DWH: PM_CmdScale was ripped from Q3 source
//==========================================================================
float PM_CmdScale( usercmd_t *cmd ) {
	int		max;
	float	total;
	float	scale;

	max = abs( cmd->forwardmove );
	if ( abs( cmd->sidemove ) > max ) {
		max = abs( cmd->sidemove );
	}
	if ( abs( cmd->upmove ) > max ) {
		max = abs( cmd->upmove );
	}
	if ( !max ) {
		return 0;
	}

	total = sqrt( (double)cmd->forwardmove * cmd->forwardmove
		+ (double)cmd->sidemove * cmd->sidemove + (double)cmd->upmove * cmd->upmove );
	scale = max / total;

	return scale;
}
void RemovePush(edict_t *ent)
{
	ent->client->push->s.sound = 0;
	ent->client->push->activator = NULL;
	ent->client->push = NULL;
	ent->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
	// If tpp is NOT always on, and auto-switch for func_pushables IS on,
	// and we're currently in third-person view, switch it off
	// Knightmare- don't autoswitch if client-side chasecam is on
#ifdef KMQUAKE2_ENGINE_MOD
	if (!tpp->value && tpp_auto->value && (!cl_thirdperson->value || deathmatch->value || coop->value) && ent->client->chasetoggle)
#else
	if(!tpp->value && tpp_auto->value && ent->client->chasetoggle)
#endif
		Cmd_Chasecam_Toggle(ent);
}

void ClientPushPushable(edict_t *ent)
{
	edict_t		*box = ent->client->push;
	vec_t		dist;
	vec3_t		new_origin, v, vbox;

	VectorAdd (box->absmax,box->absmin,vbox);
	VectorScale(vbox,0.5,vbox);
	if (point_infront(ent,vbox))
	{
		VectorSubtract(ent->s.origin,box->offset,new_origin);
		VectorSubtract(new_origin,box->s.origin,v);
		v[2] = 0;
		dist = VectorLength(v);
		if(dist > 8)
		{
			// func_pushable got hung up somehow. Break off contact
			RemovePush(ent);
		}
		else if(dist > 0)
		{
			if(!box->speaker)
				box->s.sound = box->noise_index;
			box_walkmove( box, vectoyaw(v), dist );
		}
		else
			box->s.sound = 0;
	}
	else
		RemovePush(ent);
}

void ClientSpycam(edict_t *ent)
{
	gclient_t	*client = ent->client;
	edict_t		*camera = ent->client->spycam;
	pmove_t		pm;
	qboolean	is_actor;
	trace_t		tr;
	vec3_t		forward, left, up;
	vec3_t		dir, start;
	float		dist;
	int			i;

	memset (&pm, 0, sizeof(pm));
	if(client->ucmd.sidemove && level.time > ent->last_move_time + 1)
	{
		camera->flags &= ~FL_ROBOT;
		if(camera->viewer == ent)
			camera->viewer = NULL;
		if(client->ucmd.sidemove > 0)
			camera = G_FindNextCamera(camera,client->monitor);
		else
			camera = G_FindPrevCamera(camera,client->monitor);
		
		if(camera)
		{
			if(!camera->viewer)
				camera->viewer = ent;
			client->spycam = camera;
			VectorAdd(camera->s.origin,camera->move_origin,ent->s.origin);
			if(camera->viewmessage)
				gi.centerprintf(ent,camera->viewmessage);
			ent->last_move_time = level.time;
		}
		else
			camera = client->spycam;
		if(camera->monsterinfo.aiflags & AI_ACTOR)
		{
			camera->flags |= FL_ROBOT;
			if(camera->monsterinfo.aiflags & AI_FOLLOW_LEADER)
			{
				camera->monsterinfo.aiflags &= ~AI_FOLLOW_LEADER;
				camera->monsterinfo.old_leader  = NULL;
				camera->monsterinfo.leader      = NULL;
				camera->movetarget = camera->goalentity = NULL;
				camera->monsterinfo.stand(camera);
			}
		}
	}
	if((camera->svflags & SVF_MONSTER) && (camera->monsterinfo.aiflags & AI_ACTOR))
		is_actor = true;
	else
		is_actor = false;
	if(camera->enemy && (camera->enemy->deadflag || !camera->enemy->inuse))
		camera->enemy = NULL;
	AngleVectors(camera->s.angles,forward,left,up);

	if(is_actor && !camera->enemy)
	{
		if((abs(client->ucmd.forwardmove) > 199) && (camera->groundentity))
		{
			// walk/run
			
			edict_t	*thing;
			vec3_t	end;
			float	dist;

			thing  = camera->vehicle;
			
			VectorMA(camera->s.origin,8192,forward,end);
			tr = gi.trace(camera->s.origin,camera->mins,camera->maxs,end,camera,MASK_SOLID);
			if(client->ucmd.forwardmove < 0)
			{
				trace_t	back;
				VectorMA(camera->s.origin,-8192,forward,end);
				back = gi.trace(camera->s.origin,camera->mins,camera->maxs,end,camera,MASK_SOLID);
				VectorSubtract(back.endpos,camera->s.origin,end);
				dist = VectorLength(end);
				VectorCopy(tr.endpos,end);
			}
			else
			{
				VectorSubtract(tr.endpos,camera->s.origin,end);
				dist = VectorLength(end) - 8;
				VectorMA(camera->s.origin,dist,forward,end);
			}
			if(dist > 8)
			{
				if(!thing || !thing->inuse || Q_stricmp(thing->classname,"thing"))
					thing = camera->vehicle = SpawnThing();
				thing->touch_debounce_time = level.time + 5.0;
				thing->target_ent = camera;
				VectorCopy(end,thing->s.origin);
				ED_CallSpawn(thing);
				camera->monsterinfo.aiflags |= AI_CHASE_THING;
				camera->monsterinfo.aiflags &= ~(AI_CHICKEN | AI_STAND_GROUND);
				camera->monsterinfo.pausetime = 0;
				camera->movetarget = camera->goalentity = thing;
				camera->monsterinfo.old_leader = NULL;
				camera->monsterinfo.leader = thing;
				VectorSubtract (thing->s.origin, camera->s.origin, dir);
				camera->ideal_yaw = vectoyaw(dir);
				if(client->ucmd.forwardmove > 300)
					actor_run(camera);
				else if(client->ucmd.forwardmove > 199)
					actor_walk(camera);
				else if(client->ucmd.forwardmove < -300)
					actor_run_back(camera);
				else
					actor_walk_back(camera);
			}
			else if(thing)
			{
				camera->monsterinfo.aiflags &= ~AI_CHASE_THING;
				camera->movetarget = camera->goalentity = NULL;
				G_FreeEdict(thing);
				camera->vehicle = NULL;
				actor_stand(camera);
			}
		}
		if((client->ucmd.forwardmove == 0) && (camera->groundentity))
		{
			// stop
			edict_t	*thing = camera->vehicle;
			if(thing)
			{
				camera->monsterinfo.aiflags &= ~AI_CHASE_THING;
				camera->movetarget = camera->goalentity = NULL;
				G_FreeEdict(thing);
				camera->vehicle = NULL;
				actor_stand(camera);
			}
		}
		
		if(client->ucmd.upmove)
		{
			if((client->ucmd.upmove > 0) && camera->groundentity && !camera->waterlevel)
			{
				// jump
				if(client->ucmd.forwardmove > 300)
					VectorScale(forward,400,camera->velocity);
				else if(client->ucmd.forwardmove > 199)
					VectorScale(forward,200,camera->velocity);
				else if(client->ucmd.forwardmove < -300)
					VectorScale(forward,-400,camera->velocity);
				else if(client->ucmd.forwardmove < -199)
					VectorScale(forward,-200,camera->velocity);
				camera->velocity[2] = 250;
				camera->monsterinfo.savemove = camera->monsterinfo.currentmove;
				actor_jump(camera);
				camera->groundentity = NULL;
			}
			else if((client->ucmd.upmove < 0) && (camera->groundentity) && !(camera->monsterinfo.aiflags & AI_CROUCH))
			{
				// crouch
				if( (camera->monsterinfo.currentmove == &actor_move_walk)   ||
					(camera->monsterinfo.currentmove == &actor_move_run)    ||
					(camera->monsterinfo.currentmove == &actor_move_run_bad)  )
				{
					camera->monsterinfo.currentmove = &actor_move_crouchwalk;
					camera->maxs[2] -= 28;
					camera->viewheight -= 28;
					camera->move_origin[2] -= 28;
					camera->monsterinfo.aiflags |= AI_CROUCH;
				}
				else if( (camera->monsterinfo.currentmove == &actor_move_walk_back) ||
						 (camera->monsterinfo.currentmove == &actor_move_run_back)     )
				{
					camera->monsterinfo.currentmove = &actor_move_crouchwalk_back;
					camera->maxs[2] -= 28;
					camera->viewheight -= 28;
					camera->move_origin[2] -= 28;
					camera->monsterinfo.aiflags |= AI_CROUCH;
				}
				else if (camera->monsterinfo.currentmove == &actor_move_stand)
				{
					camera->monsterinfo.currentmove = &actor_move_crouch;
					camera->maxs[2] -= 28;
					camera->viewheight -= 28;
					camera->move_origin[2] -= 28;
					camera->monsterinfo.aiflags |= AI_CROUCH;
				}
			}
		}
		if( (client->ucmd.upmove >= 0) && (camera->monsterinfo.aiflags & AI_CROUCH))
		{
			// come out of crouch
			camera->maxs[2] += 28;
			camera->viewheight += 28;
			camera->move_origin[2] += 28;
			camera->monsterinfo.aiflags &= ~AI_CROUCH;
			if(camera->monsterinfo.currentmove == &actor_move_crouchwalk)
				actor_walk(camera);
			else if(camera->monsterinfo.currentmove == &actor_move_crouchwalk_back)
				actor_walk_back(camera);
			else if(camera->monsterinfo.currentmove == &actor_move_crouch)
				actor_stand(camera);
		}
	}

	client->ps.pmove.pm_type = PM_FREEZE;
	if(camera->viewer == ent) {
		if( (client->old_owner_angles[0] != client->ucmd.angles[0]) ||
			(client->old_owner_angles[1] != client->ucmd.angles[1])   )
		{
			// Give game a bit of time to catch up after player
			// causes ucmd pitch angle to roll over... otherwise
			// we'll hit on the above test even though player
			// hasn't hit +lookup/+lookdown
			float	delta;
			delta = level.time - camera->touch_debounce_time;
			if( delta < 0 || delta > 1.0)
			{
				if(is_actor)
				{
					float	diff;
					diff = SHORT2ANGLE(client->ucmd.angles[1] - client->old_owner_angles[1]);
					if(diff < -180)
						diff += 360;
					if(diff > 180)
						diff -= 360;
					camera->ideal_yaw += diff;
					if((fabs(diff) > 100) && camera->vehicle)
					{
						vec3_t	angles;
						vec3_t	end, f;
						VectorSet(angles,0,camera->ideal_yaw,0);
						AngleVectors(angles,f,NULL,NULL);
						VectorMA(camera->s.origin,8192,f,end);
						tr = gi.trace(camera->s.origin,camera->mins,camera->maxs,end,camera,MASK_SOLID);
						VectorCopy(tr.endpos,camera->vehicle->s.origin);
						camera->vehicle->touch_debounce_time = level.time + 5.0;
						gi.linkentity(camera->vehicle);
					}
					ai_turn(camera,0.);
					diff = SHORT2ANGLE(client->ucmd.angles[0]-client->old_owner_angles[0]);
					if(diff < -180)
						diff += 360;
					if(diff > 180)
						diff -= 360;
					camera->move_angles[0] += diff;
					client->old_owner_angles[0] = client->ucmd.angles[0];
					client->old_owner_angles[1] = client->ucmd.angles[1];
				}
			}
		}
		if ( client->ucmd.buttons & BUTTON_ATTACK && camera->sounds >= 0 ) {
			if (level.time >= camera->monsterinfo.attack_finished) {
				client->latched_buttons &= ~BUTTON_ATTACK;
				if (!Q_stricmp(camera->classname,"turret_breach")
				    || !Q_stricmp(camera->classname,"model_turret"))	//CW: added to allow model_turrets to be remotely fired
				{
					if(camera->sounds==5 || camera->sounds==6)
						camera->monsterinfo.attack_finished = level.time;
					else
						camera->monsterinfo.attack_finished = level.time + 1.0;
					turret_breach_fire(camera);
				}
				else if(is_actor)
				{
					if(!camera->enemy)
					{
						edict_t	*target;
						target = LookingAt(ent,0,NULL,NULL);
						if(target && target->takedamage && (target != client->camplayer))
						{
							if(camera->vehicle)
							{
								// Currently following "thing" - turn that off
								camera->monsterinfo.aiflags &= ~AI_CHASE_THING;
								camera->movetarget = camera->goalentity = NULL;
								G_FreeEdict(camera->vehicle);
								camera->vehicle = NULL;
							}
							camera->enemy = target;
							actor_fire(camera);
							camera->enemy = NULL;
							if(camera->monsterinfo.aiflags & AI_HOLD_FRAME)
								camera->monsterinfo.attack_finished = level.time + FRAMETIME;
							else
								camera->monsterinfo.attack_finished = level.time + 1.0;
						}
					}
				}
			}
		}
		if(client->zoomed) {
			camera->touch_debounce_time = 
				max(camera->touch_debounce_time, level.time + 1.0);
		}
	}

	VectorMA(camera->s.origin, camera->move_origin[0],forward,start);
	VectorMA(start,           -camera->move_origin[1],left,   start);
	VectorMA(start,            camera->move_origin[2],up,     start);
	
	tr = gi.trace(camera->s.origin, NULL, NULL, start, camera, MASK_SOLID);
	if(tr.fraction < 1.0)
	{
		VectorSubtract(tr.endpos,camera->s.origin,dir);
		dist = VectorNormalize(dir) - 2;
		if(dist < 0) dist = 0.;
		VectorMA(camera->s.origin,dist,dir,start);
	}
	VectorCopy(start,ent->s.origin);
	VectorCopy(camera->velocity,ent->velocity);
	
	client->resp.cmd_angles[0] = SHORT2ANGLE(client->ucmd.angles[0]);
	client->resp.cmd_angles[1] = SHORT2ANGLE(client->ucmd.angles[1]);
	client->resp.cmd_angles[2] = SHORT2ANGLE(client->ucmd.angles[2]);
	
	memset (&pm, 0, sizeof(pm));
	pm.s = client->ps.pmove;
	for (i=0 ; i<3 ; i++) {
		pm.s.origin[i] = ent->s.origin[i]*8;
		client->ps.pmove.delta_angles[i] = 
			ANGLE2SHORT(client->ps.viewangles[i] - client->resp.cmd_angles[i]);
	}
	if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
		pm.snapinitial = true;
	pm.cmd = client->ucmd;
	pm.trace = PM_trace;	// adds default parms
	pm.pointcontents = gi.pointcontents;
	
	gi.Pmove (&pm);
	
	gi.linkentity (ent);
//	client->old_owner_angles[0] = client->ucmd.angles[0];
//	client->old_owner_angles[1] = client->ucmd.angles[1];
	
	G_TouchTriggers (ent); // we'll only allow touching trigger_look with "Cam Owner" SF
	
}
/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/
void ClientThink (edict_t *ent, usercmd_t *ucmd)
{
	gclient_t	*client;
	edict_t		*other;
	edict_t		*ground;
	pmove_t		pm;
	vec_t		t;
	vec3_t		view;
	vec3_t		oldorigin, oldvelocity;
	int			i, j;
	float		ground_speed;
//	short		save_forwardmove;

	level.current_entity = ent;
	client = ent->client;
	// Lazarus: Copy latest usercmd stuff for use in other routines
	client->ucmd = *ucmd;

	VectorCopy(ent->s.origin,oldorigin);
	VectorCopy(ent->velocity,oldvelocity);
	ground = ent->groundentity;

	if(ground && (ground->movetype == MOVETYPE_PUSH) && (ground != world) && ground->turn_rider)
		ground_speed = VectorLength(ground->velocity);
	else
		ground_speed = 0;

	if( (ent->in_mud)               ||
		(ent->client->push)         ||
		(ent->vehicle)              ||
		(ent->client->chasetoggle)  ||
		(ent->turret)               ||
		(ent->client->spycam)       ||
		(ground_speed > 0)            )
		ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
	else
		ent->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;

	// Server-side speed control stuff
#ifdef KMQUAKE2_ENGINE_MOD
	client->ps.maxspeed = player_max_speed->value;
	client->ps.duckspeed = player_crouch_speed->value;
	client->ps.accel = player_accel->value;
	client->ps.stopspeed = player_stopspeed->value;
#endif

	if(client->startframe == 0)
		client->startframe = level.framenum;

	client->fps_frames++;
	if(client->fps_frames >= 100) {
		client->secs_per_frame = (level.time-client->fps_time_start)/100;
		client->fps_frames = 0;
		client->fps_time_start = level.time;
		client->frame_zoomrate = zoomrate->value * client->secs_per_frame;
	}
	VectorCopy(ent->s.origin,view);
	view[2] += ent->viewheight;
	Fog(view);

// MUD - get mud level
	if(level.mud_puddles)
	{
		edict_t	*mud;

		ent->in_mud = 0;
		for(i=game.maxclients+1; i<globals.num_edicts && !ent->in_mud; i++)
		{
			mud = &g_edicts[i];
			if(!mud->inuse) continue;
			if(!(mud->svflags & SVF_MUD)) continue;
			if(ent->absmin[0] > mud->absmax[0]) continue;
			if(ent->absmin[1] > mud->absmax[1]) continue;
			if(ent->absmin[2] > mud->absmax[2]) continue;
			if(ent->absmax[0] < mud->absmin[0]) continue;
			if(ent->absmax[1] < mud->absmin[1]) continue;
			if(ent->absmax[2] < mud->absmin[2]) continue;
			ent->in_mud = 1;
			if(ent->s.origin[2] < mud->absmax[2])
				ent->in_mud = 2;
			if(ent->s.origin[2] + ent->viewheight < mud->absmax[2])
				ent->in_mud = 3;
		}
	}

// USE - special actions taken when +use is pressed
	if(!client->use && (ucmd->buttons & BUTTON_USE))
	{
		// use key was NOT pressed, but now is
		client->use = 1;
		if(client->spycam)
			camera_off(ent);
		else
		{
			edict_t *viewing;
			vec3_t	intersect;
			float	range;

			viewing = LookingAt(ent,0,intersect,&range);
			if(viewing && viewing->classname)
			{
				if(!strcmp(viewing->classname,"crane_control"))
					crane_control_action(viewing,ent,intersect);
				if(!strcmp(viewing->classname,"target_lock_digit"))
					lock_digit_increment(viewing,ent);
				if(!strcmp(viewing->classname,"func_trainbutton") && (viewing->spawnflags & 1))
					trainbutton_use(viewing,ent,ent);
				if(!strcmp(viewing->classname,"func_monitor") && range <= 100) {
					use_camera(viewing,ent,ent);
					if(client->spycam && client->spycam->viewer == ent) {
						client->old_owner_angles[0] = ucmd->angles[0];
						client->old_owner_angles[1] = ucmd->angles[1];
					}
				}
				if(viewing->monsterinfo.aiflags & AI_ACTOR)
				{
					if(viewing->monsterinfo.aiflags & AI_FOLLOW_LEADER)
					{
						viewing->monsterinfo.aiflags    &= ~AI_FOLLOW_LEADER;
						viewing->monsterinfo.old_leader  = NULL;
						viewing->monsterinfo.leader      = NULL;
						viewing->movetarget = viewing->goalentity = NULL;
						viewing->monsterinfo.stand(viewing);
					}
					else
					{
						vec3_t	dir;
						viewing->monsterinfo.aiflags |= AI_FOLLOW_LEADER;
						viewing->monsterinfo.leader   = ent;
						VectorSubtract(ent->s.origin,viewing->s.origin,dir);
						viewing->ideal_yaw = vectoyaw(dir);
						if(fabs((double)viewing->s.angles[YAW] - (double)viewing->ideal_yaw) < 90)
							actor_salute(viewing);
					}
				}
			}
		}
	}

	if(ucmd->buttons & BUTTON_USE)
		client->use = 1;
	else
		client->use = 0;

	if( client->push )
	{
		// currently pushing or pulling a func_pushable
		if( !client->use )
		{
			// whoops - released USE key
			RemovePush(ent);
		}
		else if( (!ent->groundentity) && (ent->waterlevel==0 || client->push->waterlevel == 0 ) )
		{
			// oops, we fall down
			RemovePush(ent);
		}
		else
		{
			// Scale client velocity by mass of func_pushable
			t = VectorLength(ent->velocity);
			if( t > client->maxvelocity )
				VectorScale(ent->velocity, client->maxvelocity/t, ent->velocity);
			client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
			t = 200./client->push->mass;
			ucmd->forwardmove *= t;
			ucmd->sidemove    *= t;
		}
	}

	if(ent->turret && ucmd->upmove > 10)
		turret_disengage(ent->turret);

// INTERMISSION
	if (level.intermissiontime)
	{
		// tpp
		if (client->chasetoggle)
			ChasecamRemove (ent, OPTION_OFF);
		// end tpp
		// Lazarus spycam
		if (client->spycam)
			camera_off(ent);
		client->ps.pmove.pm_type = PM_FREEZE;
		// can exit intermission after five seconds
		if (level.time > level.intermissiontime + 5.0 
			&& (ucmd->buttons & BUTTON_ANY) )
			level.exitintermission = true;

		return;
	}

	if (ent->target_ent && !Q_stricmp(ent->target_ent->classname,"target_monitor"))
	{
		edict_t	*monitor = ent->target_ent;
		if(monitor->target_ent && monitor->target_ent->inuse)
		{
			if(monitor->spawnflags & 2)
				VectorCopy(monitor->target_ent->s.angles,client->ps.viewangles);
			else
			{
				vec3_t	dir;
				VectorSubtract(monitor->target_ent->s.origin,monitor->s.origin,dir);
				vectoangles(dir,client->ps.viewangles);
			}
		}
		else
			VectorCopy (monitor->s.angles, client->ps.viewangles);
		VectorCopy(monitor->s.origin,ent->s.origin);
		client->ps.pmove.pm_type = PM_FREEZE;
		return;
	}

// THIRDPERSON VIEW in/out
// if NOT pushing something AND in third person AND use key is pressed,
// move viewpoint in/out
	if(client->chasetoggle && !client->push) {
		if ((ucmd->buttons & BUTTON_USE) && (!deathmatch->value))
		{
			client->use = 1;
			if ((ucmd->forwardmove < 0) && (client->zoom < 100))
				client->zoom++;
			else if ((ucmd->forwardmove > 0) && (client->zoom > -40))
				client->zoom--;
			ucmd->forwardmove = 0;
			ucmd->sidemove = 0;
		}
		else if (client->use)
		{
			//client->zoom = 0;
			if (client->oldplayer)
			{
				// set angles
				for (i=0 ; i<3 ; i++)
				{
					ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->client->oldplayer->s.angles[i] - ent->client->resp.cmd_angles[i]);
				}
			}
			client->use = 0;
		}
	}

// ZOOM
	if (client->zooming) {
		client->pers.hand = 2;
		if(client->zooming > 0) {
			if(client->ps.fov > 5) {
				client->ps.fov -= client->frame_zoomrate;
				if(client->ps.fov < 5)
					client->ps.fov = 5;
			} else {
				client->ps.fov = 5;
			}
			client->zoomed = true;
		} else {
			if(client->ps.fov < client->original_fov) {
				client->ps.fov += client->frame_zoomrate;
				if(client->ps.fov > client->original_fov) {
					client->ps.fov = client->original_fov;
					client->zoomed = false;
				} else
					client->zoomed = true;
			} else {
				client->ps.fov = client->original_fov;
				client->zoomed = false;
			}
		}
	}

// SPYCAM
	if (client->spycam) {
		ClientSpycam(ent);
		return; // no movement while in cam
    } // END SPYCAM

	pm_passent = ent;

	// Lazarus: developer item movement
	if(client->use && client->shift_dir)
		ShiftItem(ent, client->shift_dir);

	if (client->chase_target) {

		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

	} else {

		// set up for pmove
		memset (&pm, 0, sizeof(pm));

		if (ent->movetype == MOVETYPE_NOCLIP)
			client->ps.pmove.pm_type = PM_SPECTATOR;
		else if (ent->s.modelindex != (MAX_MODELS-1))	// was 255
			client->ps.pmove.pm_type = PM_GIB;
		else if (ent->deadflag)
			client->ps.pmove.pm_type = PM_DEAD;
		else
			client->ps.pmove.pm_type = PM_NORMAL;

		if(level.time > ent->gravity_debounce_time)
			client->ps.pmove.gravity = sv_gravity->value;
		else
			client->ps.pmove.gravity = 0;

#ifdef JETPACK_MOD
		if ( client->jetpack )
		{
			if( (ucmd->upmove != 0) || (ucmd->forwardmove != 0) || (ucmd->sidemove != 0) )
			{
				if(ucmd->upmove > 0 || !ent->groundentity)
				{
					if(!client->jetpack_thrusting)
					{
						gi.sound (ent, CHAN_AUTO, gi.soundindex("jetpack/rev.wav"), 1, ATTN_NORM, 0);
						client->jetpack_start_thrust = level.framenum;
					}
					client->jetpack_thrusting = true;
				}
				else
					client->jetpack_thrusting = false;
			}
			else
				client->jetpack_thrusting = false;

			if(client->jetpack_framenum + client->pers.inventory[fuel_index] > level.framenum)
			{
				if(jetpack_weenie->value)
				{
					Jet_ApplyJet( ent, ucmd );
					if(client->jetpack_framenum < level.framenum)
					{
						if(!client->jetpack_infinite)
							client->pers.inventory[fuel_index] -= 10;
						client->jetpack_framenum = level.framenum + 10;
					}
				}
				else
				{
					if(client->jetpack_thrusting)
						Jet_ApplyJet( ent, ucmd );
					if(client->jetpack_framenum <= level.framenum)
					{
						if(client->jetpack_thrusting)
						{
							if(!client->jetpack_infinite)
								client->pers.inventory[fuel_index] -= 11;
							client->jetpack_framenum = level.framenum + 10;
						}
						else
						{
							if(!client->jetpack_infinite)
								client->pers.inventory[fuel_index]--;
							client->jetpack_framenum = level.framenum + 10;
						}
					}
					if(ucmd->upmove == 0)
					{
						// accelerate to 75% gravity in 2 seconds
						float	gravity;
						float	g_max = 0.75 * sv_gravity->value;

						gravity = g_max * (level.framenum - client->jetpack_last_thrust)/20;
						if(gravity > g_max) gravity = g_max;
						client->ps.pmove.gravity = (short)gravity;
					}
					else
						client->jetpack_last_thrust = level.framenum;
				}
			}
			else
			{
				client->jetpack = false;
				ent->s.frame = FRAME_jump2;	// reset from stand to avoid goofiness
			}
		}
#endif    // #ifdef JETPACK_MOD

		pm.s = client->ps.pmove;

		for (i=0 ; i<3 ; i++)
		{
			pm.s.origin[i] = ent->s.origin[i]*8;
			pm.s.velocity[i] = ent->velocity[i]*8;
		}

		if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
		{
			pm.snapinitial = true;
	//		gi.dprintf ("pmove changed!\n");
		}

		pm.cmd = *ucmd;

		pm.trace = PM_trace;	// adds default parms
		pm.pointcontents = gi.pointcontents;

		if(ent->vehicle)
			pm.s.pm_flags |= PMF_ON_GROUND;

		// perform a pmove
		gi.Pmove (&pm);

		// save results of pmove
		client->ps.pmove = pm.s;
		client->old_pmove = pm.s;

		for (i=0 ; i<3 ; i++)
		{
			ent->s.origin[i] = pm.s.origin[i]*0.125;
			ent->velocity[i] = pm.s.velocity[i]*0.125;
		}
		VectorCopy (pm.mins, ent->mins);
		VectorCopy (pm.maxs, ent->maxs);

		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

#ifdef JETPACK_MOD
		if ( client->jetpack && jetpack_weenie->value )
		{
			if( pm.groundentity )		// are we on ground
				if ( Jet_AvoidGround(ent) )	// then lift us if possible
					pm.groundentity = NULL;		// now we are no longer on ground
		}
#endif

// MUD - "correct" Pmove physics
		if(pm.waterlevel && ent->in_mud)
		{
			vec3_t	point;
			vec3_t	end;
			
			vec3_t	deltapos, deltavel;
			float	frac;

			pm.watertype |= CONTENTS_MUD;
			ent->in_mud  = pm.waterlevel;
			VectorSubtract(ent->s.origin,oldorigin,deltapos);
			VectorSubtract(ent->velocity,oldvelocity,deltavel);
			if(pm.waterlevel == 1)
			{
				frac = MUD1BASE + MUD1AMP*sin( (float)(level.framenum%10)/10.*2*M_PI);
				ent->s.origin[0] = oldorigin[0]   + frac*deltapos[0];
				ent->s.origin[1] = oldorigin[1]   + frac*deltapos[1];
				ent->s.origin[2] = oldorigin[2]   + 0.75*deltapos[2];
				ent->velocity[0] = oldvelocity[0] + frac*deltavel[0];
				ent->velocity[1] = oldvelocity[1] + frac*deltavel[1];
				ent->velocity[2] = oldvelocity[2] + 0.75*deltavel[2];
			}
			else if(pm.waterlevel == 2)
			{
				trace_t	tr;
				float	dist;
				
				VectorCopy(oldorigin,point);
				point[2] += ent->maxs[2];
				end[0] = point[0]; end[1] = point[1]; end[2] = oldorigin[2] + ent->mins[2];
				tr = gi.trace(point,NULL,NULL,end,ent,CONTENTS_WATER);
				dist = point[2] - tr.endpos[2];
				// frac = waterlevel 1 frac at dist=32 or more,
				//      = waterlevel 3 frac at dist=10 or less
				if(dist <= 10)
					frac = MUD3;
				else
					frac = MUD3 + (dist-10.f)/22.f*((float)MUD1BASE-MUD3);
				ent->s.origin[0] = oldorigin[0]   + frac*deltapos[0];
				ent->s.origin[1] = oldorigin[1]   + frac*deltapos[1];
				ent->s.origin[2] = oldorigin[2]   + frac*deltapos[2];
				ent->velocity[0] = oldvelocity[0] + frac*deltavel[0];
				ent->velocity[1] = oldvelocity[1] + frac*deltavel[1];
				ent->velocity[2] = oldvelocity[2] + frac*deltavel[2];
				if(!ent->groundentity)
				{
					// Player can't possibly move up
					ent->s.origin[2] = min(oldorigin[2], ent->s.origin[2]);
					ent->velocity[2] = min(oldvelocity[2],ent->velocity[2]);
					ent->velocity[2] = min(-10,ent->velocity[2]);
				}
			}
			else
			{
				ent->s.origin[0] = oldorigin[0]   + MUD3*deltapos[0];
				ent->s.origin[1] = oldorigin[1]   + MUD3*deltapos[1];
				ent->velocity[0] = oldvelocity[0] + MUD3*deltavel[0];
				ent->velocity[1] = oldvelocity[1] + MUD3*deltavel[1];
				if(ent->groundentity)
				{
					ent->s.origin[2] = oldorigin[2]   + MUD3*deltapos[2];
					ent->velocity[2] = oldvelocity[2] + MUD3*deltavel[2];
				}
				else
				{
					ent->s.origin[2] = min(oldorigin[2],ent->s.origin[2]);
					ent->velocity[2] = min(oldvelocity[2], 0);
				}
			}
		}
		else
			ent->in_mud = 0;
// end MUD

		if (ent->groundentity && !pm.groundentity && (pm.cmd.upmove >= 10) && (pm.waterlevel == 0) && !client->jetpack)
		{
			// Knightmare- allow disabling of STUPID grunting when jumping
			if ((deathmatch->value || player_jump_sounds->value) && !ent->vehicle)
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
				PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
			}
			// Paril's vehicle targeting
			if (ent->vehicle)
				G_UseTargets (ent->vehicle, ent);
			// Lazarus: temporarily match velocities with entity we just
			//          jumped from
			VectorAdd(ent->groundentity->velocity,ent->velocity,ent->velocity);
		}

		if (ent->groundentity && !pm.groundentity && (pm.cmd.upmove >= 10) && (pm.waterlevel == 0))
			ent->client->jumping = 1;

		if (ent->deadflag != DEAD_FROZEN)
			ent->viewheight = pm.viewheight;
		ent->waterlevel = pm.waterlevel;
		ent->watertype = pm.watertype;
		ent->groundentity = pm.groundentity;
		if (pm.groundentity)
			ent->groundentity_linkcount = pm.groundentity->linkcount;

		// Lazarus - lie about ground when driving a vehicle.
		//           Pmove apparently doesn't think the ground
		//           can be "owned"
		if (ent->vehicle && !ent->groundentity)
		{
			ent->groundentity = ent->vehicle;
			ent->groundentity_linkcount = ent->vehicle->linkcount;
		}


		if (ent->deadflag)
		{
			if (ent->deadflag != DEAD_FROZEN)
			{
				client->ps.viewangles[ROLL] = 40;
				client->ps.viewangles[PITCH] = -15;
				client->ps.viewangles[YAW] = client->killer_yaw;
			}
		}
		else
		{
			VectorCopy (pm.viewangles, client->v_angle);
			VectorCopy (pm.viewangles, client->ps.viewangles);
		}

#ifdef JETPACK_MOD
		if ( client->jetpack && !(ucmd->buttons & BUTTONS_ATTACK))
			ent->s.frame = FRAME_stand20;
#endif

		gi.linkentity (ent);

		if (ent->movetype != MOVETYPE_NOCLIP)
			G_TouchTriggers (ent);

		if( (world->effects & FX_WORLDSPAWN_JUMPKICK) && (ent->client->jumping) && (ent->solid != SOLID_NOT))
			kick_attack(ent);

		// touch other objects
		// Lazarus: but NOT if game is frozen
		if(!level.freeze)
		{
			for (i=0 ; i<pm.numtouch ; i++)
			{
				other = pm.touchents[i];
				for (j=0 ; j<i ; j++)
					if (pm.touchents[j] == other)
						break;
					if (j != i)
						continue;	// duplicated
					if (!other->touch)
						continue;
					other->touch (other, ent, NULL, NULL);
			}
		}
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// save light level the player is standing on for
	// monster sighting AI
	ent->light_level = ucmd->lightlevel;

	// CDawg - add here!
	if (ucmd->forwardmove < -1)
		ent->client->backpedaling = true;
	else
		ent->client->backpedaling = false;
	// CDawg end here! 

	// fire weapon from final position if needed
	if (client->latched_buttons & BUTTONS_ATTACK)
	{
		if (client->resp.spectator) {

			client->latched_buttons = 0;

			if (client->chase_target) {
				client->chase_target = NULL;
				client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
			} else
				GetChaseTarget(ent);

		} else if (!client->weapon_thunk) {
			client->weapon_thunk = true;
			Think_Weapon (ent);
		}
	}

	if (client->resp.spectator) {
		if (ucmd->upmove >= 10) {
			if (!(client->ps.pmove.pm_flags & PMF_JUMP_HELD)) {
				client->ps.pmove.pm_flags |= PMF_JUMP_HELD;
				if (client->chase_target)
					ChaseNext(ent);
				else
					GetChaseTarget(ent);
			}
		} else
			client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
	}

	// update chase cam if being followed
	for (i = 1; i <= maxclients->value; i++) {
		other = g_edicts + i;
		if (other->inuse && other->client->chase_target == ent)
			UpdateChaseCam(other);
	}

	if(client->push != NULL)
	{
		if(client->use &&
			( (ucmd->forwardmove != 0) || (ucmd->sidemove != 0) ) )
			ClientPushPushable(ent);
		else
			client->push->s.sound = 0;
	}

}


/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
void ClientBeginServerFrame (edict_t *ent)
{
	gclient_t	*client;
	int			buttonMask;

	if (level.intermissiontime)
		return;

	client = ent->client;

	// DWH
	if(client->spycam)
		client = client->camplayer->client;

	// tpp
	if (client->delayedstart > 0)
		client->delayedstart--;
	if (client->delayedstart == 1)
		ChasecamStart (ent);
	// end tpp

	if (deathmatch->value &&
		client->pers.spectator != client->resp.spectator &&
		(level.time - client->respawn_time) >= 5) {
		spectator_respawn(ent);
		return;
	}

	// run weapon animations if it hasn't been done by a ucmd_t
	if (!client->weapon_thunk && !client->resp.spectator)
		Think_Weapon (ent);
	else
		client->weapon_thunk = false;

	if (ent->deadflag)
	{
		// wait for any button just going down
		if ( level.time > client->respawn_time)
		{
			// tpp
	        if (ent->crosshair)
				G_FreeEdict(ent->crosshair);
			ent->crosshair = NULL;
			if (ent->client->oldplayer)
				G_FreeEdict (ent->client->oldplayer);
			ent->client->oldplayer = NULL;
			if (ent->client->chasecam)
				G_FreeEdict (ent->client->chasecam);
			ent->client->chasecam = NULL;
			// end tpp

			// in deathmatch, only wait for attack button
			if (deathmatch->value)
				buttonMask = BUTTONS_ATTACK;
			else
				buttonMask = -1;

			if ( ( client->latched_buttons & buttonMask ) ||
				(deathmatch->value && ((int)dmflags->value & DF_FORCE_RESPAWN) ) )
			{
				respawn(ent);
				client->latched_buttons = 0;
			}
		}
		return;
	}

	// add player trail so monsters can follow
	// DWH: Don't add player trail for players in camera
	if (!deathmatch->value && !client->spycam)
		if (!visible (ent, PlayerTrail_LastSpot() ) )
			PlayerTrail_Add (ent->s.old_origin);

	client->latched_buttons = 0;
}

