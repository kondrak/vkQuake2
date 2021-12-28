#include "g_local.h"
#include "m_player.h"

int	nostatus = 0;


void RotateAngles(vec3_t in, vec3_t delta, vec3_t out)
{
	// Rotates input angles (in) by delta angles around
	// the local coordinate system, returns new angles
	// in out.
	vec3_t	X={1,0,0};
	vec3_t	Y={0,1,0};
	float	angle, c, s;
	float	xtemp, ytemp, ztemp;

	if(delta[ROLL] != 0)
	{
		// Rotate about the X axis by delta roll
		angle = DEG2RAD(delta[ROLL]);
		c = cos(angle);
		s = sin(angle);
		ytemp = c*X[1] - s*X[2];
		ztemp = c*X[2] + s*X[1];
		X[1] = ytemp; X[2] = ztemp;
		ytemp = c*Y[1] - s*Y[2];
		ztemp = c*Y[2] + s*Y[1];
		Y[1] = ytemp; Y[2] = ztemp;
	}
	if(delta[PITCH] != 0)
	{
		// Rotate about the Y axis by delta yaw
		angle = -DEG2RAD(delta[PITCH]);
		c = cos(angle);
		s = sin(angle);
		ztemp = c*X[2] - s*X[0];
		xtemp = c*X[0] + s*X[2];
		X[0] = xtemp; X[2] = ztemp;
		ztemp = c*Y[2] - s*Y[0];
		xtemp = c*Y[0] + s*Y[2];
		Y[0] = xtemp; Y[2] = ztemp;
	}
	if(delta[YAW] != 0)
	{
		// Rotate about the Z axis by delta yaw
		angle = DEG2RAD(delta[YAW]);
		c = cos(angle);
		s = sin(angle);
		xtemp = c*X[0] - s*X[1];
		ytemp = c*X[1] + s*X[0];
		X[0] = xtemp; X[1] = ytemp;
		xtemp = c*Y[0] - s*Y[1];
		ytemp = c*Y[1] + s*Y[0];
		Y[0] = xtemp; Y[1] = ytemp;
	}
	if(in[ROLL] != 0)
	{
		// Rotate about X axis by input roll
		angle = DEG2RAD(in[ROLL]);
		c = cos(angle);
		s = sin(angle);
		ytemp = c*X[1] - s*X[2];
		ztemp = c*X[2] + s*X[1];
		X[1] = ytemp; X[2] = ztemp;
		ytemp = c*Y[1] - s*Y[2];
		ztemp = c*Y[2] + s*Y[1];
		Y[1] = ytemp; Y[2] = ztemp;
	}
	if(in[PITCH] != 0)
	{
		// Rotate about Y axis by input pitch
		angle = -DEG2RAD(in[PITCH]);
		c = cos(angle);
		s = sin(angle);
		ztemp = c*X[2] - s*X[0];
		xtemp = c*X[0] + s*X[2];
		X[0] = xtemp; X[2] = ztemp;
		ztemp = c*Y[2] - s*Y[0];
		xtemp = c*Y[0] + s*Y[2];
		Y[0] = xtemp; Y[2] = ztemp;
	}
	if(in[YAW] != 0)
	{
		// Rotate about Z axis by input yaw
		angle = DEG2RAD(in[YAW]);
		c = cos(angle);
		s = sin(angle);
		xtemp = c*X[0] - s*X[1];
		ytemp = c*X[1] + s*X[0];
		X[0] = xtemp; X[1] = ytemp;
		xtemp = c*Y[0] - s*Y[1];
		ytemp = c*Y[1] + s*Y[0];
		Y[0] = xtemp; Y[1] = ytemp;
	}

	out[YAW]   = (180./M_PI) * atan2(X[1],X[0]);
	if(out[YAW] != 0)
	{
		angle = -DEG2RAD(out[YAW]);
		c = cos(angle);
		s = sin(angle);
		xtemp = c*X[0] - s*X[1];
		ytemp = c*X[1] + s*X[0];
		X[0] = xtemp; X[1] = ytemp;
		xtemp = c*Y[0] - s*Y[1];
		ytemp = c*Y[1] + s*Y[0];
		Y[0] = xtemp; Y[1] = ytemp;
	}
	
	out[PITCH] = (180./M_PI) * atan2(X[2],X[0]);
	if(out[PITCH] != 0)
	{
		angle = DEG2RAD(out[PITCH]);
		c = cos(angle);
		s = sin(angle);
		ztemp = c*Y[2] - s*Y[0];
		xtemp = c*Y[0] + s*Y[2];
		Y[0] = xtemp; Y[2] = ztemp;
	}
	out[ROLL]  = (180./M_PI) * atan2(Y[2],Y[1]);

}

void laser_sight_think(edict_t *laser)
{
	edict_t	*player;
	vec3_t	end, forward, right, offset;
	trace_t	tr;


	if(!laser->activator)
		return;
	player = laser->activator;

	AngleVectors (player->client->v_angle, forward, right, NULL);
	VectorSet(offset, 16, 8, player->viewheight-8);

	if (player->client->pers.hand == LEFT_HANDED)
		offset[1] *= -1;
	else if (player->client->pers.hand == CENTER_HANDED)
		offset[1] = 0;
	G_ProjectSource (player->s.origin, offset, forward, right, laser->s.origin);
	VectorMA (laser->s.origin, 2048, forward, end);
	tr = gi.trace (laser->s.origin, laser->mins, laser->maxs, end, player, MASK_SHOT);
	VectorCopy (tr.endpos, laser->s.origin);
	gi.linkentity(laser);
	laser->nextthink = level.time + FRAMETIME;
}

void SaveEntProps(edict_t *e, FILE *f)
{
	fprintf(f,
		"================================\n"
		"entity_state_t\n"
		"   number      = %d\n"
		"   origin      = %s\n"
		"   angles      = %s\n"
		"   old_origin  = %s\n"
		"   modelindex  = %d\n"
		"   modelindex2 = %d\n"
		"   modelindex3 = %d\n"
		"   modelindex4 = %d\n"
#ifdef KMQUAKE2_ENGINE_MOD
		"   modelindex5 = %d\n"
		"   modelindex6 = %d\n"
#endif
		"   frame       = %d\n"
		"   skinnum     = %d\n"
		"   effects     = 0x%08x\n"
		"   solid       = 0x%08x\n"
		"   sound       = %d\n"
		"   event       = %d\n",
		e->s.number,vtos(e->s.origin),vtos(e->s.angles),
		vtos(e->s.old_origin),e->s.modelindex,e->s.modelindex2,
		e->s.modelindex3,e->s.modelindex4,
#ifdef KMQUAKE2_ENGINE_MOD
		e->s.modelindex5,e->s.modelindex6,
#endif
		e->s.frame,
		e->s.skinnum,e->s.effects,e->s.solid,e->s.sound,
		e->s.event);
	fprintf(f,"inuse       = %d\n"
		"linkcount   = %d\n"
		"svflags     = 0x%08x\n"
		"mins        = %s\n"
		"maxs        = %s\n"
		"absmin      = %s\n"
		"absmax      = %s\n"
		"size        = %s\n"
		"solid       = 0x%08x\n"
		"clipmask    = 0x%08x\n",
		e->inuse,e->linkcount,e->svflags,vtos(e->mins),
		vtos(e->maxs),vtos(e->absmin),vtos(e->absmax),
		vtos(e->size),e->solid,e->clipmask);
	fprintf(f,"movetype    = 0x%08x\n"
		"flags       = 0x%08x\n"
		"freetime    = %g\n"
		"message     = %s\n"
		"key_message = %s\n"
		"classname   = %s\n"
		"spawnflags  = 0x%08x\n"
		"timestamp   = %g\n"
		"angle       = %g\n"
		"target      = %s\n"
		"targetname  = %s\n"
		"killtarget  = %s\n"
		"team        = %s\n"
		"pathtarget  = %s\n"
		"deathtarget = %s\n"
		"combattarget= %s\n"
		"dmgteam     = %s\n",
		e->movetype,e->flags,e->freetime,e->message,e->key_message,
		e->classname,e->spawnflags,e->timestamp,e->angle,e->target,
		e->targetname,e->killtarget,e->team,e->pathtarget,e->deathtarget,
		e->combattarget,e->dmgteam);
	fprintf(f,"speed       = %g\n"
		"accel       = %g\n"
		"decel       = %g\n"
		"movedir     = %s\n"
		"pos1        = %s\n"
		"pos2        = %s\n"
		"velocity    = %s\n"
		"avelocity   = %s\n"
		"mass        = %d\n"
		"air_finished= %g\n"
		"gravity     = %g\n"
		"yaw_speed   = %g\n"
		"ideal_yaw   = %g\n"
		"pitch_speed = %g\n"
		"ideal_pitch = %g\n"
		"ideal_roll  = %g\n"
		"roll        = %g\n"
		"groundentity= %s\n",
		e->speed,e->accel,e->decel,vtos(e->movedir),vtos(e->pos1),
		vtos(e->pos2),vtos(e->velocity),vtos(e->avelocity),
		e->mass,e->air_finished,e->gravity,e->yaw_speed,e->ideal_yaw,
		e->pitch_speed,e->ideal_pitch,e->ideal_roll,e->roll,
		(e->groundentity ? e->groundentity->classname : "None") );
	fprintf(f,"touch_debounce_time  = %g\n"
		"pain_debounce_time   = %g\n"
		"damage_debounce_time = %g\n"
		"gravity_debounce_time= %g\n"
		"fly_debounce_time    = %g\n"
		"last_move_time       = %g\n",
		e->touch_debounce_time,e->pain_debounce_time,
		e->damage_debounce_time,e->gravity_debounce_time,
		e->fly_sound_debounce_time,e->last_move_time);
	fprintf(f,"health      = %d\n"
		"max_health  = %d\n"
		"gib_health  = %d\n"
		"deadflag    = %d\n"
		"show_hostile= %d\n"
		"health2     = %d\n"
		"mass2       = %d\n"
		"powerarmor_time=%g\n",
		e->health,e->max_health,e->gib_health,e->deadflag,e->show_hostile,
		e->health2,e->mass2,e->powerarmor_time);
	fprintf(f,"viewheight  = %d\n"
		"takedamage  = %d\n"
		"dmg         = %d\n"
		"radius_dmg  = %d\n"
		"dmg_radius  = %g\n"
		"sounds      = %d\n"
		"count       = %d\n",
		e->viewheight,e->takedamage,e->dmg,e->radius_dmg,e->dmg_radius,
		e->sounds,e->count);
	fprintf(f,"noise_index = %d\n"
		"noise_index2= %d\n"
		"volume      = %g\n"
		"attenuation = %g\n"
		"wait        = %g\n"
		"delay       = %g\n"
		"random      = %g\n"
		"starttime   = %g\n"
		"endtime     = %g\n"
		"teleport_time=%g\n"
		"watertype   = %d\n"
		"waterlevel  = %d\n"
		"move_origin = %s\n"
		"move_angles = %s\n",
		e->noise_index,e->noise_index2,e->volume,e->attenuation,
		e->wait,e->delay,e->random,e->starttime,e->endtime,e->teleport_time,
		e->watertype,e->waterlevel,vtos(e->move_origin),vtos(e->move_angles));
	fprintf(f,"light_level = %d\n"
		"style       = %d\n",
		e->light_level,e->style);
	fprintf(f,"enemy = %s\n",(e->enemy ? e->enemy->classname : "NULL"));
	fprintf(f,"enemy->inuse? %s\n",(e->enemy && e->enemy->inuse ? "Y" : "N"));
	fprintf(f,"moveinfo_t\n"
		"   start_origin    = %s\n"
		"   start_angles    = %s\n"
		"   end_origin      = %s\n"
		"   end_angles      = %s\n"
		"   sound_start     = %d\n"
		"   sound_middle    = %d\n"
		"   sound_end       = %d\n"
		"   accel           = %g\n"
		"   speed           = %g\n"
		"   decel           = %g\n"
		"   distance        = %g\n"
		"   wait            = %g\n"
		"   state           = %d\n"
		"   dir             = %s\n"
		"   current_speed   = %g\n"
		"   move_speed      = %g\n"
		"   next_speed      = %g\n"
		"   remaining_dist  = %g\n"
		"   decel_distance  = %g\n",
		vtos(e->moveinfo.start_origin),
		vtos(e->moveinfo.start_angles),
		vtos(e->moveinfo.end_origin),
		vtos(e->moveinfo.end_angles),
		e->moveinfo.sound_start,e->moveinfo.sound_middle,
		e->moveinfo.sound_end,e->moveinfo.accel,e->moveinfo.speed,
		e->moveinfo.decel,e->moveinfo.distance,e->moveinfo.wait,
		e->moveinfo.state,vtos(e->moveinfo.dir),e->moveinfo.current_speed,
		e->moveinfo.move_speed,e->moveinfo.next_speed,
		e->moveinfo.remaining_distance,e->moveinfo.decel_distance);
	fprintf(f,"monsterinfo\n"
		"   aiflags          = 0x%08x\n"
		"   nextframe        = %d\n"
		"   scale            = %g\n"
		"   pausetime        = %g\n"
		"   attack_finished  = %g\n"
		"   saved_goal       = %s\n"
		"   search_time      = %g\n"
		"   trail_time       = %g\n"
		"   last_sighting    = %s\n"
		"   attack_state     = %d\n"
		"   lefty            = %d\n"
		"   idle_time        = %g\n"
		"   linkcount        = %d\n"
		"   power_armor_type = %d\n"
		"   power_armor_power= %d\n"
		"   min_range        = %g\n",
		e->monsterinfo.aiflags,e->monsterinfo.nextframe,
		e->monsterinfo.scale,e->monsterinfo.pausetime,
		e->monsterinfo.attack_finished,vtos(e->monsterinfo.saved_goal),
		e->monsterinfo.search_time,e->monsterinfo.trail_time,
		vtos(e->monsterinfo.last_sighting),e->monsterinfo.attack_state,
		e->monsterinfo.lefty,e->monsterinfo.idle_time,
		e->monsterinfo.linkcount,e->monsterinfo.power_armor_type,
		e->monsterinfo.power_armor_power,e->monsterinfo.min_range);
}

void ShiftItem(edict_t *ent, int direction)
{
	vec3_t      end, forward, start;
	vec3_t		move;

	edict_t		*target;

	if(!ent->client) return;

	target = LookingAt(ent,0,NULL,NULL);
	if(!target) return;

	ent->client->shift_dir = direction;
	
	VectorClear(move);
	VectorCopy(ent->s.origin,start);
	VectorAdd(target->s.origin,target->origin_offset,end);
	VectorSubtract(end,start,forward);
	VectorNormalize(forward);
	VectorScale(forward,shift_distance->value,forward);
	if(direction & 1)
	{
		if(fabs(forward[0]) > fabs(forward[1]))
			move[1] += forward[0];
		else
			move[0] -= forward[1];
	}
	if(direction & 2)
	{
		if(fabs(forward[0]) > fabs(forward[1]))
			move[1] -= forward[0];
		else
			move[0] += forward[1];
	}
	if(direction & 4)
	{
		if(fabs(forward[0]) > fabs(forward[1]))
			move[0] += forward[0];
		else
			move[1] += forward[1];
	}
	if(direction & 8)
	{
		if(fabs(forward[0]) > fabs(forward[1]))
			move[0] -= forward[0];
		else
			move[1] -= forward[1];
	}
	if(direction & 16)
		move[2] += shift_distance->value;

	if(direction & 32)
		move[2] -= shift_distance->value;

	if(direction & 64) {
		if( target->movetype == MOVETYPE_TOSS     ||
			target->movetype == MOVETYPE_BOUNCE   ||
			target->movetype == MOVETYPE_STEP     ||
			target->movetype == MOVETYPE_PUSHABLE ||
			target->movetype == MOVETYPE_DEBRIS      ) {
			M_droptofloor(target);
		}
	}

	if(direction & 128) {
		target->s.angles[PITCH] += rotate_distance->value;
		if(target->s.angles[PITCH] > 360) target->s.angles[PITCH] -= 360;
		if(target->s.angles[PITCH] <   0) target->s.angles[PITCH] += 360;
	}
	if(direction & 256) {
		target->s.angles[YAW] += rotate_distance->value;
		if(target->s.angles[YAW] > 360) target->s.angles[YAW] -= 360;
		if(target->s.angles[YAW] <   0) target->s.angles[YAW] += 360;
	}
	if(direction & 512) {
		target->s.angles[ROLL] += rotate_distance->value;
		if(target->s.angles[ROLL] > 360) target->s.angles[ROLL] -= 360;
		if(target->s.angles[ROLL] <   0) target->s.angles[ROLL] += 360;
	}

	VectorAdd(target->s.origin,move,target->s.origin);
	if(!(direction & 64)) target->gravity_debounce_time = level.time + 1.0;
	gi.linkentity(target);
}

char *ClientTeam (edict_t *ent)
{
	char		*p;
	static char	value[512];

	value[0] = 0;

	if (!ent->client)
		return value;

	strcpy(value, Info_ValueForKey (ent->client->pers.userinfo, "skin"));
	p = strchr(value, '/');
	if (!p)
		return value;

	if ((int)(dmflags->value) & DF_MODELTEAMS)
	{
		*p = 0;
		return value;
	}

	// if ((int)(dmflags->value) & DF_SKINTEAMS)
	return ++p;
}

qboolean OnSameTeam (edict_t *ent1, edict_t *ent2)
{
	char	ent1Team [512];
	char	ent2Team [512];

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		return false;

	strcpy (ent1Team, ClientTeam (ent1));
	strcpy (ent2Team, ClientTeam (ent2));

	if (strcmp(ent1Team, ent2Team) == 0)
		return true;
	return false;
}


void SelectNextItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->menu) {
		PMenu_Next(ent);
		return;
	} else if (cl->textdisplay) {
		Text_Next (ent);
		return;
	} else if (cl->chase_target) {
		ChaseNext(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void SelectPrevItem (edict_t *ent, int itflags)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;

	cl = ent->client;

	if (cl->menu) {
		PMenu_Prev(ent);
		return;
	} else if (cl->textdisplay) {
		Text_Prev (ent);
		return;
	} else if (cl->chase_target) {
		ChasePrev(ent);
		return;
	}

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (cl->pers.selected_item + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		return;
	}

	cl->pers.selected_item = -1;
}

void ValidateSelectedItem (edict_t *ent)
{
	gclient_t	*cl;

	cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return;		// valid

	SelectNextItem (ent, -1);
}


//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f (edict_t *ent)
{
	char		*name;
	gitem_t		*it;
	int			index;
	int			i;
	qboolean	give_all;
	edict_t		*it_ent;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	//Knightmare- override ammo pickup values with cvars
	SetAmmoPickupValues ();

	name = gi.args();

	if(!Q_stricmp(name,"jetpack"))
	{
		if(!developer->value)
		{
			gi.cprintf(ent, PRINT_HIGH, "Jetpack not available via give cheat\n");
			return;
		}
		else
		{
			gitem_t *fuel;
			fuel = FindItem("fuel");
			Add_Ammo(ent,fuel,500);
		}

	}
	if(!developer->value)
	{
		if( !Q_stricmp(name,"flashlight")      ||
			!Q_stricmp(name,"fuel")            ||
			!Q_stricmp(name,"homing rockets") ||
			!Q_stricmp(name,"stasis generator")   )
		{

			gi.cprintf(ent, PRINT_HIGH, "%s not available via give cheat\n",name);
			return;
		}
	}

	if (Q_stricmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if (give_all || Q_stricmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "weapons") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "ammo") == 0)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			if (it->classname && !Q_stricmp(it->classname,"ammo_fuel") && !developer->value)
				continue;
			if (it->classname && !Q_stricmp(it->classname,"ammo_homing_missiles") && !developer->value)
				continue;
			Add_Ammo (ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "armor") == 0)
	{
		gitem_armor_t	*info;

		it = FindItem("Jacket Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Combat Armor");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Body Armor");
		info = (gitem_armor_t *)it->info;
		ent->client->pers.inventory[ITEM_INDEX(it)] = info->max_count;

		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp(name, "Power Shield") == 0)
	{
		it = FindItem("Power Shield");
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);

		if (!give_all)
			return;
	}

	if (give_all)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR|IT_WEAPON|IT_AMMO))
				continue;
			if (it->classname && !Q_stricmp(it->classname,"item_jetpack") && !developer->value)
				continue;
			if (it->classname && !Q_stricmp(it->classname,"item_flashlight") && !developer->value)
				continue;
			if (it->classname && !Q_stricmp(it->classname,"item_freeze") && !developer->value)
				continue;
			ent->client->pers.inventory[i] = 1;
		}
		return;
	}

	it = FindItem (name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem (name);
		if (!it)
		{
			gi.cprintf (ent, PRINT_HIGH, "unknown item\n");
			return;
		}
	}

	if (!it->pickup)
	{
		gi.cprintf (ent, PRINT_HIGH, "non-pickup item\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO)
	{
		// Lazarus: Bleah - special case for "homing rockets"
		if (it->tag == AMMO_HOMING_ROCKETS)
		{
			if (gi.argc() == 4)
				ent->client->pers.inventory[index] = atoi(gi.argv(3));
			else
				ent->client->pers.inventory[index] += it->quantity;
		}
		else
		{
			if (gi.argc() == 3)
				ent->client->pers.inventory[index] = atoi(gi.argv(2));
			else
				ent->client->pers.inventory[index] += it->quantity;
		}
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE) )
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET) )
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f (edict_t *ent)
{
	char	*msg;

	if (deathmatch->value && !sv_cheats->value)
	{
		gi.cprintf (ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		ent->solid    = SOLID_BBOX;	// Lazarus
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid    = SOLID_NOT;	// Lazarus
		msg = "noclip ON\n";
	}

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	index = ITEM_INDEX(it);
#ifdef JETPACK_MOD
	if(!strcmp(s,"jetpack"))
	{
		// Special case - turns on/off
		if(!ent->client->jetpack)
		{
			if(ent->waterlevel > 0)
				return;
			if(!ent->client->pers.inventory[index])
			{
				gi.cprintf(ent, PRINT_HIGH, "Out of item: %s\n", s);
				return;
			}
			else if(ent->client->pers.inventory[fuel_index] <= 0)
			{
				gi.cprintf(ent, PRINT_HIGH, "No fuel for: %s\n", s);
				return;
			}
		}
		it->use(ent,it);
		return;
	}
#endif
	if (!strcmp(s,"stasis generator"))
	{
		// Special case - turn freeze off if already on
		if(level.freeze)
		{
			level.freeze = false;
			level.freezeframes = 0;
			return;
		}
	}
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->use (ent, it);
}


/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f (edict_t *ent)
{
	int			index;
	gitem_t		*it;
	char		*s;

	s = gi.args();
	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	index = ITEM_INDEX(it);
	if (!ent->client->pers.inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop (ent, it);
}


/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f (edict_t *ent)
{
	int			i;
	gclient_t	*cl;

	cl = ent->client;

	cl->showscores = false;
	cl->showhelp = false;

	if (cl->menu) {
		PMenu_Close(ent);
		ent->client->update_chase = true;
		return;
	}

	if (cl->textdisplay)
	{
		Text_Close(ent);
		return;
	}

	if (cl->showinventory)
	{
		cl->showinventory = false;
		return;
	}

	cl->showinventory = true;

	gi.WriteByte (svc_inventory);
	for (i=0 ; i<MAX_ITEMS ; i++)
	{
		// Don't show "No Weapon" or "Homing Rocket Launcher" in inventory
		if((i == noweapon_index) || (i == hml_index))
			gi.WriteShort (0);
		else if((i == fuel_index) && (ent->client->jetpack_infinite))
			gi.WriteShort (0);
		else
			gi.WriteShort (cl->pers.inventory[i]);
	}
	gi.unicast (ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t *ent)
{
	gitem_t		*it;

	if (ent->client->menu) {
		PMenu_Select(ent);
		return;
	}

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}

#ifdef JETPACK_MOD
	if(!strcmp(it->classname,"item_jetpack"))
	{
		if(!ent->client->jetpack)
		{
			if(ent->waterlevel > 0)
				return;
			if(ent->client->pers.inventory[fuel_index] <= 0)
			{
				gi.cprintf(ent, PRINT_HIGH, "No fuel for jetpack\n" );
				return;
			}
		}
	}
#endif
	
	it->use (ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f (edict_t *ent)
{
	gclient_t	*cl;
	int			i, index;
	gitem_t		*it;
	int			selected_weapon;

	cl = ent->client;

	if (!cl->pers.weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->pers.weapon);

	// scan  for the next valid one
	for (i=1 ; i<=MAX_ITEMS ; i++)
	{
		index = (selected_weapon + MAX_ITEMS - i)%MAX_ITEMS;
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (! (it->flags & IT_WEAPON) )
			continue;
		it->use (ent, it);
		if (cl->pers.weapon == it)
			return;	// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f (edict_t *ent)
{
	gclient_t	*cl;
	int			index;
	gitem_t		*it;

	cl = ent->client;

	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	index = ITEM_INDEX(cl->pers.lastweapon);
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (! (it->flags & IT_WEAPON) )
		return;
	it->use (ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f (edict_t *ent)
{
	gitem_t		*it;

	ValidateSelectedItem (ent);

	if (ent->client->pers.selected_item == -1)
	{
		gi.cprintf (ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	it->drop (ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f (edict_t *ent)
{
	if((level.time - ent->client->respawn_time) < 5)
		return;
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	meansOfDeath = MOD_SUICIDE;
	player_die (ent, ent, ent, 100000, vec3_origin);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f (edict_t *ent)
{
	ent->client->showscores = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;

	if (ent->client->menu)
		PMenu_Close(ent);
	if (ent->client->textdisplay)
		Text_Close(ent);
	ent->client->update_chase = true;
}


int PlayerSort (void const *a, void const *b)
{
	int		anum, bnum;

	anum = *(int *)a;
	bnum = *(int *)b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f (edict_t *ent)
{
	int		i;
	int		count;
	char	small[64];
	char	large[1280];
	int		index[256];

	count = 0;
	for (i = 0 ; i < maxclients->value ; i++)
		if (game.clients[i].pers.connected)
		{
			index[count] = i;
			count++;
		}

	// sort by frags
	qsort (index, count, sizeof(index[0]), PlayerSort);

	// print information
	large[0] = 0;

	for (i = 0 ; i < count ; i++)
	{
		Com_sprintf (small, sizeof(small), "%3i %s\n",
			game.clients[index[i]].ps.stats[STAT_FRAGS],
			game.clients[index[i]].pers.netname);
		if (strlen (small) + strlen(large) > sizeof(large) - 100 )
		{	// can't print all of them in one packet
			strcat (large, "...\n");
			break;
		}
		strcat (large, small);
	}

	gi.cprintf (ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f (edict_t *ent)
{
	int		i;

	i = atoi (gi.argv(1));

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	ent->client->anim_priority = ANIM_WAVE;

	switch (i)
	{
	case 0:
		gi.cprintf (ent, PRINT_HIGH, "flipoff\n");
		ent->s.frame = FRAME_flip01-1;
		ent->client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf (ent, PRINT_HIGH, "salute\n");
		ent->s.frame = FRAME_salute01-1;
		ent->client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf (ent, PRINT_HIGH, "taunt\n");
		ent->s.frame = FRAME_taunt01-1;
		ent->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf (ent, PRINT_HIGH, "wave\n");
		ent->s.frame = FRAME_wave01-1;
		ent->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf (ent, PRINT_HIGH, "point\n");
		ent->s.frame = FRAME_point01-1;
		ent->client->anim_end = FRAME_point12;
		break;
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t *ent, qboolean team, qboolean arg0)
{
	int		i, j;
	edict_t	*other;
	char	*p;
	char	text[2048];
	gclient_t *cl;

	if (gi.argc () < 2 && !arg0)
		return;

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		team = false;

	if (team)
		Com_sprintf (text, sizeof(text), "(%s): ", ent->client->pers.netname);
	else
		Com_sprintf (text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0)
	{
		strcat (text, gi.argv(0));
		strcat (text, " ");
		strcat (text, gi.args());
	}
	else
	{
		p = gi.args();

		if (*p == '"')
		{
			p++;
			p[strlen(p)-1] = 0;
		}
		strcat(text, p);
	}

	// don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	if (flood_msgs->value) {
		cl = ent->client;

        if (level.time < cl->flood_locktill) {
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n",
				(int)(cl->flood_locktill - level.time));
            return;
        }
        i = cl->flood_whenhead - flood_msgs->value + 1;
        if (i < 0)
            i = (sizeof(cl->flood_when)/sizeof(cl->flood_when[0])) + i;
		if (cl->flood_when[i] && 
			level.time - cl->flood_when[i] < flood_persecond->value) {
			cl->flood_locktill = level.time + flood_waitdelay->value;
			gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n",
				(int)flood_waitdelay->value);
            return;
        }
		cl->flood_whenhead = (cl->flood_whenhead + 1) %
			(sizeof(cl->flood_when)/sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (team)
		{
			if (!OnSameTeam(ent, other))
				continue;
		}
		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}
}

void Cmd_PlayerList_f(edict_t *ent)
{
	int i;
	char st[80];
	char text[1400];
	edict_t *e2;

	// connect time, ping, score, name
	*text = 0;
	for (i = 0, e2 = g_edicts + 1; i < maxclients->value; i++, e2++) {
		if (!e2->inuse)
			continue;

		Com_sprintf(st, sizeof(st), "%02d:%02d %4d %3d %s%s\n",
			(level.framenum - e2->client->resp.enterframe) / 600,
			((level.framenum - e2->client->resp.enterframe) % 600)/10,
			e2->client->ping,
			e2->client->resp.score,
			e2->client->pers.netname,
			e2->client->resp.spectator ? " (spectator)" : "");
		if (strlen(text) + strlen(st) > sizeof(text) - 50) {
			sprintf(text+strlen(text), "And more...\n");
			gi.cprintf(ent, PRINT_HIGH, "%s", text);
			return;
		}
		strcat(text, st);
	}
	gi.cprintf(ent, PRINT_HIGH, "%s", text);
}

void DrawBBox(edict_t *ent)
{
	vec3_t	p1, p2;
	vec3_t	origin;

	VectorCopy(ent->s.origin,origin);
	VectorSet(p1,origin[0]+ent->mins[0],origin[1]+ent->mins[1],origin[2]+ent->mins[2]);
	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->mins[1],origin[2]+ent->maxs[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->maxs[1],origin[2]+ent->mins[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->mins[1],origin[2]+ent->mins[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);

	VectorSet(p1,origin[0]+ent->maxs[0],origin[1]+ent->maxs[1],origin[2]+ent->mins[2]);
	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->maxs[1],origin[2]+ent->maxs[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->mins[1],origin[2]+ent->mins[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->maxs[1],origin[2]+ent->mins[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);

	VectorSet(p1,origin[0]+ent->maxs[0],origin[1]+ent->mins[1],origin[2]+ent->maxs[2]);
	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->mins[1],origin[2]+ent->mins[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->maxs[1],origin[2]+ent->maxs[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->mins[1],origin[2]+ent->maxs[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);

	VectorSet(p1,origin[0]+ent->mins[0],origin[1]+ent->maxs[1],origin[2]+ent->maxs[2]);
	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->maxs[1],origin[2]+ent->mins[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
	VectorSet(p2,origin[0]+ent->mins[0],origin[1]+ent->mins[1],origin[2]+ent->maxs[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
	VectorSet(p2,origin[0]+ent->maxs[0],origin[1]+ent->maxs[1],origin[2]+ent->maxs[2]);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (p1);
	gi.WritePosition (p2);
	gi.multicast (p1, MULTICAST_ALL);
}
void Cmd_Bbox_f (edict_t *ent)
{
	edict_t	*viewing;

	viewing = LookingAt(ent, 0, NULL, NULL);
	if(!viewing) return;
	DrawBBox(viewing);
}

void SetLazarusCrosshair (edict_t *ent)
{
	if (deathmatch->value || coop->value) return;
	if (!ent->inuse) return;
	if (!ent->client) return;
	if (ent->client->zoomed || ent->client->zooming)
		return;

	gi.cvar_forceset("lazarus_crosshair",      va("%d",(int)(crosshair->value)));
	gi.cvar_forceset("lazarus_cl_gun",         va("%d",(int)(cl_gun->value)));
}

void SetSensitivities(edict_t *ent,qboolean reset)
{
	char	string[512];

	if(deathmatch->value || coop->value) return;
	if(!ent->inuse) return;
	if(!ent->client) return;
	if(reset)
	{
#ifndef KMQUAKE2_ENGINE_MOD // engine has zoom autosensitivity
		gi.cvar_set ("m_pitch", va("%f", lazarus_pitch->value));
		gi.cvar_set ("m_yaw", va("%f", lazarus_yaw->value));
		gi.cvar_set ("joy_pitchsensitivity", va("%f", lazarus_joyp->value));
		gi.cvar_set ("joy_yawsensitivity", va("%f", lazarus_joyy->value));
//		m_pitch->value              = lazarus_pitch->value;
//		m_yaw->value                = lazarus_yaw->value;
//		joy_pitchsensitivity->value = lazarus_joyp->value;
//		joy_yawsensitivity->value   = lazarus_joyy->value;
#endif
		if (crosshair->value != lazarus_crosshair->value)
		{
			Com_sprintf(string, sizeof(string), "crosshair %i\n",(int)(lazarus_crosshair->value));
			stuffcmd(ent,string);
		}
		if (cl_gun->value != lazarus_cl_gun->value)
		{
			Com_sprintf(string, sizeof(string), "cl_gun %i\n",(int)(lazarus_cl_gun->value));
			stuffcmd(ent,string);
		}
		ent->client->pers.hand = hand->value;
	}
	else
	{
		float	ratio;

		//save in lazarus_crosshair
		Com_sprintf(string, sizeof(string), "lazarus_crosshair %i\n", atoi(crosshair->string));
		stuffcmd(ent,string);
		Com_sprintf(string, sizeof(string), "crosshair 0\n");
		stuffcmd(ent,string);

		Com_sprintf(string, sizeof(string), "lazarus_cl_gun %i\n", atoi(cl_gun->string));
		stuffcmd(ent,string);
		Com_sprintf(string, sizeof(string), "cl_gun 0\n");
		stuffcmd(ent,string);

		if(!ent->client->sensitivities_init)
		{
#ifndef KMQUAKE2_ENGINE_MOD // engine has zoom autosensitivity
			ent->client->m_pitch = m_pitch->value;
			ent->client->m_yaw   = m_yaw->value;
			ent->client->joy_pitchsensitivity = joy_pitchsensitivity->value;
			ent->client->joy_yawsensitivity   = joy_yawsensitivity->value;
#endif
			ent->client->sensitivities_init = true;
		}
		if (ent->client->ps.fov >= ent->client->original_fov)
		{
			ratio = 1.;
		/*	if (cl_gun->value != lazarus_cl_gun->value) {
				Com_sprintf(string, sizeof(string), "cl_gun %i\n",(int)(lazarus_cl_gun->value));
				stuffcmd(ent,string);
			}
			if (crosshair->value != lazarus_crosshair->value)
			{
				Com_sprintf(string, sizeof(string), "crosshair %i\n",(int)(lazarus_crosshair->value));
				stuffcmd(ent,string);
			}
			ent->client->pers.hand = hand->value;*/
		}
		else
		{
			ratio = ent->client->ps.fov / ent->client->original_fov;
		}
#ifndef KMQUAKE2_ENGINE_MOD // engine has zoom autosensitivity
		gi.cvar_set ("m_pitch", va("%f", ent->client->m_pitch * ratio));
		gi.cvar_set ("m_yaw", va("%f", ent->client->m_yaw * ratio));
		gi.cvar_set ("joy_pitchsensitivity", va("%f", ent->client->joy_pitchsensitivity * ratio));
		gi.cvar_set ("joy_yawsensitivity", va("%f", ent->client->joy_yawsensitivity * ratio));
//		m_pitch->value = ent->client->m_pitch * ratio;
//		m_yaw->value   = ent->client->m_yaw * ratio;
//		joy_pitchsensitivity->value = ent->client->joy_pitchsensitivity * ratio;
//		joy_yawsensitivity->value   = ent->client->joy_yawsensitivity * ratio;
#endif
	}
#ifndef KMQUAKE2_ENGINE_MOD // engine has zoom autosensitivity
	sprintf(string,"m_pitch %g;m_yaw %g;joy_pitchsensitivity %g;joy_yawsensitivity %g\n",
		m_pitch->value,m_yaw->value,joy_pitchsensitivity->value,joy_yawsensitivity->value);
	stuffcmd(ent,string);
#endif
}

/*
=====================
Cmd_attack2_f
Alternate firing mode
=====================
*/
void Cmd_attack2_f(edict_t *ent, qboolean bOn)
{
	if(!ent->client) return;
	if(ent->health <= 0) return;

	if(bOn)
	{
		ent->client->pers.fire_mode=1;
		ent->client->nNewLatch |= BUTTON_ATTACK2;
	}
	else
	{
		ent->client->pers.fire_mode=0;
		ent->client->nNewLatch &= ~BUTTON_ATTACK2;
	}
}

void decoy_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	BecomeExplosion1(self);
}

void decoy_think(edict_t *self)
{
	if(self->s.frame < 0 || self->s.frame > 39)
	{
		self->s.frame = 0;
	}
	else
	{
		self->s.frame++;
		if(self->s.frame > 39)
			self->s.frame = 0;
	}

	// Every 2 seconds, make visible monsters mad at me
	if(level.framenum % 20 == 0)
	{
		edict_t	*e;
		int		i;

		for(i=game.maxclients+1; i<globals.num_edicts; i++)
		{
			e = &g_edicts[i];
			if(!e->inuse)
				continue;
			if(!(e->svflags & SVF_MONSTER))
				continue;
			if(e->monsterinfo.aiflags & AI_GOOD_GUY)
				continue;
			if(!visible(e,self))
				continue;
			if(e->enemy == self)
				continue;
			e->enemy = e->goalentity = self;
			e->monsterinfo.aiflags |= AI_TARGET_ANGER;
			FoundTarget (e);
		}
	}

	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}

void forcewall_think(edict_t *self)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_FORCEWALL);
	gi.WritePosition (self->pos1);
	gi.WritePosition (self->pos2);
	gi.WriteByte  (self->style);
	gi.multicast (self->s.origin, MULTICAST_PVS);
	self->nextthink = level.time + FRAMETIME;
}

void SpawnForcewall(edict_t	*player)
{
	edict_t  *wall;
	vec3_t	forward, point, start;
	trace_t  tr;
	
	wall = G_Spawn();
	VectorCopy(player->s.origin,start);
	start[2] += player->viewheight;
	AngleVectors(player->client->v_angle,forward,NULL,NULL);
	VectorMA(start,8192,forward,point);
	tr = gi.trace(start,NULL,NULL,point,player,MASK_SOLID);
	VectorCopy(tr.endpos,wall->s.origin);
	
	if(fabs(forward[0]) > fabs(forward[1]))
	{
		wall->pos1[0] = wall->pos2[0] = wall->s.origin[0];
		wall->mins[0] =  -1;
		wall->maxs[0] =   1;
		
		VectorCopy(wall->s.origin,point);
		point[1] -= 8192;
		tr = gi.trace(wall->s.origin,NULL,NULL,point,NULL,MASK_SOLID);
		wall->pos1[1] = tr.endpos[1];
		wall->mins[1] = wall->pos1[1] - wall->s.origin[1];
		
		point[1] = wall->s.origin[1] + 8192;
		tr = gi.trace(wall->s.origin,NULL,NULL,point,NULL,MASK_SOLID);
		wall->pos2[1] = tr.endpos[1];
		wall->maxs[1] = wall->pos2[1] - wall->s.origin[1];
	}
	else
	{
		VectorCopy(wall->s.origin,point);
		point[0] -= 8192;
		tr = gi.trace(wall->s.origin,NULL,NULL,point,NULL,MASK_SOLID);
		wall->pos1[0] = tr.endpos[0];
		wall->mins[0] = wall->pos1[0] - wall->s.origin[0];
		
		point[0] = wall->s.origin[0] + 8192;
		tr = gi.trace(wall->s.origin,NULL,NULL,point,NULL,MASK_SOLID);
		wall->pos2[0] = tr.endpos[0];
		wall->maxs[0] = wall->pos2[0] - wall->s.origin[0];
		
		wall->pos1[1] = wall->pos2[1] = wall->s.origin[1];
		wall->mins[1] = -1;
		wall->maxs[1] =  1;
	}
	wall->mins[2] = 0;
	
	VectorCopy(wall->s.origin,point);
	point[2] = wall->s.origin[2] + 8192;
	tr = gi.trace(wall->s.origin,NULL,NULL,point,NULL,MASK_SOLID);
	wall->maxs[2] = tr.endpos[2] - wall->s.origin[2];
	wall->pos1[2] = wall->pos2[2] = tr.endpos[2];
	
	wall->style = 208;	// Color from Q2 palette
	wall->movetype = MOVETYPE_NONE;
	wall->solid = SOLID_BBOX;
	wall->clipmask = MASK_PLAYERSOLID | MASK_MONSTERSOLID;
	wall->think = forcewall_think;
	wall->nextthink = level.time + FRAMETIME;
	wall->svflags = SVF_NOCLIENT;
	wall->classname = "forcewall";
	wall->activator = player;
	wall->owner = player;
	gi.linkentity(wall);
}

void ForcewallOff(edict_t *player)
{
	vec3_t	forward, point, start;
	trace_t  tr;
	
	VectorCopy(player->s.origin,start);
	start[2] += player->viewheight;
	AngleVectors(player->client->v_angle,forward,NULL,NULL);
	VectorMA(start,8192,forward,point);
	tr = gi.trace(start,NULL,NULL,point,player,MASK_SHOT);
	if(Q_stricmp(tr.ent->classname,"forcewall"))
	{
		gi.cprintf(player,PRINT_HIGH,"Not a forcewall!\n");
		return;
	}
	if(tr.ent->activator != player)
	{
		gi.cprintf(player,PRINT_HIGH,"You don't own this forcewall, bub!\n");
		return;
	}
	G_FreeEdict(tr.ent);
}

/*
=================
ClientCommand
=================
*/
void Restart_FMOD(edict_t *self)
{
	FMOD_Init();
	G_FreeEdict(self);
}
void ClientCommand (edict_t *ent)
{
	char	*cmd;
	char	*parm;

	if (!ent->client)
		return;		// not fully in game yet

	cmd = gi.argv(0);
	if(gi.argc() < 2)
		parm = NULL;
	else
		parm = gi.argv(1);

	if (Q_stricmp (cmd, "players") == 0)
	{
		Cmd_Players_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "say") == 0)
	{
		Cmd_Say_f (ent, false, false);
		return;
	}
	if (Q_stricmp (cmd, "say_team") == 0)
	{
		Cmd_Say_f (ent, true, false);
		return;
	}
	if (Q_stricmp (cmd, "score") == 0)
	{
		Cmd_Score_f (ent);
		return;
	}
	if (Q_stricmp (cmd, "help") == 0)
	{
		Cmd_Help_f (ent);
		return;
	}

	if (level.intermissiontime)
		return;

	if (Q_stricmp (cmd, "use") == 0)
		Cmd_Use_f (ent);
	else if (Q_stricmp (cmd, "drop") == 0)
		Cmd_Drop_f (ent);
	else if (Q_stricmp (cmd, "give") == 0)
		Cmd_Give_f (ent);
	else if (Q_stricmp (cmd, "god") == 0)
		Cmd_God_f (ent);
	else if (Q_stricmp (cmd, "notarget") == 0)
		Cmd_Notarget_f (ent);
	else if (Q_stricmp (cmd, "noclip") == 0)
		Cmd_Noclip_f (ent);
	else if (Q_stricmp (cmd, "inven") == 0)
		Cmd_Inven_f (ent);
	else if (Q_stricmp (cmd, "invnext") == 0)
		SelectNextItem (ent, -1);
	else if (Q_stricmp (cmd, "invprev") == 0)
		SelectPrevItem (ent, -1);
	else if (Q_stricmp (cmd, "invnextw") == 0)
		SelectNextItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invprevw") == 0)
		SelectPrevItem (ent, IT_WEAPON);
	else if (Q_stricmp (cmd, "invnextp") == 0)
		SelectNextItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invprevp") == 0)
		SelectPrevItem (ent, IT_POWERUP);
	else if (Q_stricmp (cmd, "invuse") == 0)
		Cmd_InvUse_f (ent);
	else if (Q_stricmp (cmd, "invdrop") == 0)
		Cmd_InvDrop_f (ent);
	else if (Q_stricmp (cmd, "weapprev") == 0)
		Cmd_WeapPrev_f (ent);
	else if (Q_stricmp (cmd, "weapnext") == 0)
		Cmd_WeapNext_f (ent);
	else if (Q_stricmp (cmd, "weaplast") == 0)
		Cmd_WeapLast_f (ent);
	else if (Q_stricmp (cmd, "kill") == 0)
		Cmd_Kill_f (ent);
	else if (Q_stricmp (cmd, "putaway") == 0)
		Cmd_PutAway_f (ent);
	else if (Q_stricmp (cmd, "wave") == 0)
		Cmd_Wave_f (ent);
	else if (Q_stricmp(cmd, "playerlist") == 0)
		Cmd_PlayerList_f(ent);

#ifdef FLASHLIGHT_MOD
#if FLASHLIGHT_USE==POWERUP_USE_ITEM
	else if (Q_stricmp (cmd, "flashlight") == 0)
		Use_Flashlight_f (ent, (gitem_t *)NULL);
#endif
#endif

	// ==================== fog stuff =========================
	else if (developer->value && !Q_stricmp(cmd,"fog"))
		Cmd_Fog_f(ent);
	else if (developer->value && !Q_strncasecmp(cmd, "fog_", 4))
		Cmd_Fog_f(ent);
	// ================ end fog stuff =========================

	// tpp
	else if (Q_stricmp (cmd, "thirdperson") == 0) {
		Cmd_Chasecam_Toggle (ent);
		tpp->value = ent->client->chasetoggle;
	}

	// alternate attack mode
	else if (!Q_stricmp(cmd,"attack2_off"))
		Cmd_attack2_f(ent,false);
	else if (!Q_stricmp(cmd,"attack2_on"))
		Cmd_attack2_f(ent,true);

	// zoom
	else if (!Q_stricmp(cmd, "zoomin")) {
		if (!deathmatch->value && !coop->value && !ent->client->chasetoggle) {
			if (ent->client->ps.fov > 5) {
				if(cl_gun->value)
					stuffcmd(ent,"cl_gun 0\n");
				ent->client->frame_zoomrate = zoomrate->value * ent->client->secs_per_frame;
				ent->client->zooming = 1;
				ent->client->zoomed = true;
			}
		}
	}
	else if (!Q_stricmp(cmd, "zoomout")) {
		if (!deathmatch->value && !coop->value && !ent->client->chasetoggle) {
			if (ent->client->ps.fov < ent->client->original_fov) {
				if (cl_gun->value)
					stuffcmd(ent,"cl_gun 0\n");
				ent->client->frame_zoomrate = zoomrate->value * ent->client->secs_per_frame;
				ent->client->zooming = -1;
				ent->client->zoomed = true;
			}
		}
	}
	else if (!Q_stricmp(cmd, "zoom")) {
		if(!deathmatch->value && !coop->value && !ent->client->chasetoggle) {
			if(!parm) {
				gi.dprintf("syntax: zoom [0/1]  (0=off, 1=on)\n");
			} else if(!atoi(parm)) {
				ent->client->ps.fov = ent->client->original_fov;
				ent->client->zooming = 0;
				ent->client->zoomed = false;
				SetSensitivities(ent,true);
			} else if (!ent->client->zoomed && !ent->client->zooming) {
				ent->client->ps.fov = zoomsnap->value;
				ent->client->pers.hand = 2;
				if(cl_gun->value)
					stuffcmd(ent,"cl_gun 0\n");
				ent->client->zooming = 0;
				ent->client->zoomed = true;
				SetSensitivities(ent,false);
			}
		}
	}
	else if (!Q_stricmp(cmd, "zoomoff")) {
		if (!deathmatch->value && !coop->value && !ent->client->chasetoggle) {
			if (ent->client->zoomed && !ent->client->zooming) {
				ent->client->ps.fov = ent->client->original_fov;
				ent->client->zooming = 0;
				ent->client->zoomed = false;
				SetSensitivities(ent,true);
			}
		}
	}
	else if (!Q_stricmp(cmd, "zoomon")) {
		if (!deathmatch->value && !coop->value && !ent->client->chasetoggle) {
			if (!ent->client->zoomed && !ent->client->zooming) {
				ent->client->ps.fov = zoomsnap->value;
				ent->client->pers.hand = 2;
				if(cl_gun->value)
					stuffcmd(ent,"cl_gun 0\n");
				ent->client->zooming = 0;
				ent->client->zoomed = true;
				SetSensitivities(ent,false);
			}
		}
	}
	else if (!Q_stricmp(cmd, "zoominstop")) {
		if (!deathmatch->value && !coop->value && !ent->client->chasetoggle) {
			if (ent->client->zooming > 0) {
				ent->client->zooming = 0;
				if (ent->client->ps.fov == ent->client->original_fov) {
					ent->client->zoomed = false;
					SetSensitivities(ent,true);
				} else {
					gi.cvar_forceset("zoomsnap",va("%f",ent->client->ps.fov));
					SetSensitivities(ent,false);
				}
			}
		}
	}
	else if (!Q_stricmp(cmd, "zoomoutstop")) {
		if (!deathmatch->value && !coop->value && !ent->client->chasetoggle) {
			if (ent->client->zooming < 0) {
				ent->client->zooming = 0;
				if (ent->client->ps.fov == ent->client->original_fov) {
					ent->client->zoomed = false;
					SetSensitivities(ent,true);
				} else {
					gi.cvar_forceset("zoomsnap",va("%f",ent->client->ps.fov));
					SetSensitivities(ent,false);
				}
			}
		}
	}

	else if(!Q_stricmp(cmd, "entlist")) {
		if(parm) {
			edict_t	*e;
			FILE	*f;
			int		i;
			vec3_t	origin;
			int		count;

			f = fopen(parm,"w");
			if(f) {
				fprintf(f,"Movetype codes\n"
					      " 0 MOVETYPE_NONE\n"
						  " 1 MOVETYPE_NOCLIP\n"
						  " 2 MOVETYPE_PUSH       (most moving brush models)\n"
						  " 3 MOVETYPE_STOP       (buttons)\n"
						  " 4 MOVETYPE_WALK       (players only)\n"
						  " 5 MOVETYPE_STEP       (monsters)\n"
						  " 6 MOVETYPE_FLY        (never used)\n"
						  " 7 MOVETYPE_TOSS       (gibs, normal debris)\n"
						  " 8 MOVETYPE_FLYMISSILE (rockets)\n"
						  " 9 MOVETYPE_BOUNCE     (grenades)\n"
						  "10 MOVETYPE_VEHICLE    (Lazarus func_vehicle)\n"
						  "11 MOVETYPE_PUSHABLE   (Lazarus func_pushable)\n"
						  "12 MOVETYPE_DEBRIS     (Lazarus target_rocks)\n"
						  "13 MOVETYPE_RAIN       (Lazarus precipitation)\n\n");
				fprintf(f,"Solid codes\n"
					      " 0 SOLID_NOT       no interaction with other objects\n"
						  " 1 SOLID_TRIGGER   trigger fields, pickups\n"
						  " 2 SOLID_BBOX      solid point entities\n"
						  " 3 SOLID_BSP       brush models\n\n");
				fprintf(f,"CONTENT_ codes (clipmask)\n"
					      " 0x00000001 SOLID\n"
						  " 0x00000002 WINDOW\n"
						  " 0x00000004 AUX\n"
						  " 0x00000008 LAVA\n"
						  " 0x00000010 SLIME\n"
						  " 0x00000020 WATER\n"
						  " 0x00000040 MIST\n"
						  " 0x00008000 AREAPORTAL\n"
						  " 0x00010000 PLAYERCLIP\n"
						  " 0x00020000 MONSTERCLIP\n"
						  " 0x00040000 CURRENT_0\n"
						  " 0x00080000 CURRENT_90\n"
						  " 0x00100000 CURRENT_180\n"
						  " 0x00200000 CURRENT_270\n"
						  " 0x00400000 CURRENT_UP\n"
						  " 0x00800000 CURRENT_DOWN\n"
						  " 0x01000000 ORIGIN\n"
						  " 0x02000000 MONSTER\n"
						  " 0x04000000 DEADMONSTER\n"
						  " 0x08000000 DETAIL\n"
						  " 0x10000000 TRANSLUCENT\n"
						  " 0x20000000 LADDER\n\n");
				fprintf(f,"NOTE: \"freed\" indicates an empty slot in the edicts array.\n\n");

				fprintf(f,"============================================================\n");
				count = 0;
				for(i=0, e=&g_edicts[0]; i<globals.num_edicts; i++, e++) {
					VectorAdd(e->s.origin,e->origin_offset,origin);
					fprintf(f,"entity #%d, classname = %s at %s, velocity = %s\n",i,e->classname,vtos(origin),vtos(e->velocity));
					fprintf(f,"health=%d, mass=%d, dmg=%d, wait=%g, angles=%s\n",e->health, e->mass, e->dmg, e->wait, vtos(e->s.angles));
					fprintf(f,"targetname=%s, target=%s, spawnflags=0x%04x\n",e->targetname,e->target,e->spawnflags);
					fprintf(f,"absmin,absmax,size=%s, %s, %s\n",vtos(e->absmin),vtos(e->absmax),vtos(e->size));
					fprintf(f,"groundentity=%s\n",(e->groundentity ? e->groundentity->classname : "NULL"));
					if(e->classname)
					{
						// class-specific output
						if(!Q_stricmp(e->classname,"target_changelevel"))
							fprintf(f,"map=%s\n",e->map);
					}
					fprintf(f,"movetype=%d, solid=%d, clipmask=0x%08x\n",e->movetype,e->solid,e->clipmask);
					fprintf(f,"================================================================================\n");
					if(e->inuse) count++;
				}
				fprintf(f,"Total number of entities = %d\n",count);
				fclose(f);
				gi.dprintf("done!\n");
			} else {
				gi.dprintf("Error opening %s\n",parm);
			}
		} else {
			gi.dprintf("syntax: entlist <filename>\n");
		}
	}
#ifndef DISABLE_FMOD
	else if(!Q_stricmp(cmd, "playsound"))
	{
		vec3_t	pos = {0, 0, 0};
		vec3_t	vel = {0, 0, 0};
		if(s_primary->value)
		{
			gi.dprintf("target_playback requires s_primary be set to 0.\n"
				       "At the console type:\n"
					   "s_primary 0;sound_restart\n");
			return;
		}
		if(parm)
		{
			edict_t *temp;

			strlwr(parm);
			temp = G_Spawn();
			temp->message = parm;
			temp->volume = 255;

			if( strstr(parm,".mod") ||
				strstr(parm,".s3m") ||
				strstr(parm,".xm")  ||
				strstr(parm,".mid")   )
				temp->spawnflags |= 8;

			FMOD_PlaySound(temp);
			G_FreeEdict(temp);
		}
		else
			gi.dprintf("syntax: playsound <soundfile>, path relative to gamedir\n");
	}
#endif // DISABLE_FMOD
	else if(!Q_stricmp(cmd, "properties"))
	{
		if(parm) {
			char	filename[_MAX_PATH];
			edict_t	*e;
			FILE	*f;
//			int		i;

			e = LookingAt(ent,0,NULL,NULL);
			if(!e) return;
	
			GameDirRelativePath(parm,filename);
			strcat(filename,".txt");
			f = fopen(filename,"w");
//			for(i=0; i<globals.num_edicts; i++)
//			{
//				e = &g_edicts[i];
				SaveEntProps(e,f);
//			}
			fclose(f);
		} else {
			gi.dprintf("syntax: properties <filename>\n");
		}
	}
	else if(!Q_stricmp(cmd,"go"))
	{
		edict_t *viewing;
		float	range;

		viewing = LookingAt(ent,0,NULL,&range);
		if(range > 512)
			return;
		if(!(viewing->monsterinfo.aiflags & AI_ACTOR))
			return;
		if(viewing->enemy)
			return;
		if(!(viewing->monsterinfo.aiflags & AI_FOLLOW_LEADER))
			return;
		actor_moveit(ent,viewing);
	}
	else if(!Q_stricmp(cmd,"hud"))
	{
		if(parm)
		{
			int	state = atoi(parm);

			if(state)
				Hud_On();
			else
				Hud_Off();
		}
		else
			Cmd_ToggleHud();
	}
	else if(!Q_stricmp(cmd,"whatsit"))
	{
		if(parm)
		{
			int state = atoi(parm);
			if(state)
				world->effects |= FX_WORLDSPAWN_WHATSIT;
			else
				world->effects &= ~FX_WORLDSPAWN_WHATSIT;
		}
		else
			world->effects ^= FX_WORLDSPAWN_WHATSIT;
	}

/*	else if(!Q_stricmp(cmd,"lsight"))
	{
		if(ent->client->laser_sight)
		{
			G_FreeEdict(ent->client->laser_sight);
			ent->client->laser_sight = NULL;
		}
		else
		{
			edict_t	*laser;

			laser = G_Spawn();
			ent->client->laser_sight = laser;
			laser->movetype = MOVETYPE_NOCLIP;
			laser->solid = SOLID_NOT;
			laser->s.effects = EF_SPHERETRANS;
			laser->s.modelindex = gi.modelindex("sprites/laserdot.sp2");
			laser->dmg = 0;
			VectorSet (laser->mins, -1, -1, -1);
			VectorSet (laser->maxs,  1,  1,  1);
			laser->activator = ent;
			laser->think = laser_sight_think;
			laser->think(laser);
		}
	} */
	else if(!Q_stricmp(cmd,"whereis"))
	{
		if(parm)
		{
			edict_t	*e;
			int		i;
			int		count=0;

			for(i=1; i<globals.num_edicts; i++)
			{
				e = &g_edicts[i];
				if(e->classname && !Q_stricmp(parm,e->classname))
				{
					count++;
					gi.dprintf("%d. %s\n",count,vtos(e->s.origin));
				}
			}
			if(!count)
				gi.dprintf("none found\n");
		}
		else
			gi.dprintf("syntax: whereis <classname>\n");
	}

	else if(!Q_stricmp(cmd,"sound_restart"))
	{
		// replacement for snd_restart to get around DirectSound/FMOD problem
		edict_t	*temp;
		FMOD_Shutdown();
		stuffcmd(ent,"snd_restart\n");
		temp = G_Spawn();
		temp->think = Restart_FMOD;
		temp->nextthink = level.time + 2;
	}
	// debugging/developer stuff
	else if(developer->value) {
		if (!Q_stricmp(cmd,"lightswitch"))
			ToggleLights();
		else if (!Q_stricmp(cmd,"bbox"))
			Cmd_Bbox_f (ent);
		else if(!Q_stricmp(cmd,"forcewall"))
		{
			SpawnForcewall(ent);
		}
		else if(!Q_stricmp(cmd,"forcewall_off"))
		{
			ForcewallOff(ent);
		}
		else if (!Q_stricmp(cmd,"freeze"))
		{
			if(level.freeze)
				level.freeze = false;
			else
			{
				if(ent->client->jetpack)
					gi.dprintf("Cannot use freeze while using jetpack\n");
				else
					level.freeze = true;
			}
		}
		else if (!Q_stricmp(cmd,"hint_test"))
		{
			edict_t *viewing;
			int		result;
			viewing = LookingAt(ent,LOOKAT_MD2,NULL,NULL);
			if(!viewing)
				return;
			if(viewing->monsterinfo.aiflags & AI_HINT_TEST)
			{
				viewing->monsterinfo.aiflags &= ~AI_HINT_TEST;
				gi.dprintf("%s (%s): Back to my normal self now.\n",
					viewing->classname,viewing->targetname);
				return;
			}
			if(!(viewing->svflags & SVF_MONSTER))
				gi.dprintf("hint_test is only valid for monsters and actors.\n");
			result = HintTestStart(viewing);
			switch(result)
			{
			case -1:
				gi.dprintf("%s (%s): I cannot see any hint_paths from here.\n");
				break;
			case  0:
				gi.dprintf("This map does not contain hint_paths.\n");
				break;
			case  1:
				gi.dprintf("%s (%s) searching for hint_path %s at %s. %s\n",
			 			viewing->classname, (viewing->targetname ? viewing->targetname : "<noname>"),
						(viewing->movetarget->targetname ? viewing->movetarget->targetname : "<noname>"),
						vtos(viewing->movetarget->s.origin),
						visible(viewing,viewing->movetarget) ? "I see it." : "I don't see it.");
				break;
			default: gi.dprintf("Unknown error\n");
			}
		}
		else if (!Q_stricmp(cmd,"id")) {
			edict_t *viewing;
			vec3_t	origin;
			float	range;
			viewing = LookingAt(ent,0,NULL,&range);
			if(!viewing) 
				return;
			VectorAdd(viewing->s.origin,viewing->origin_offset,origin);
			gi.dprintf("classname = %s at %s, velocity = %s\n",viewing->classname,vtos(origin),vtos(viewing->velocity));
			gi.dprintf("health=%d, mass=%d, dmg=%d, wait=%g, sounds=%d, angles=%s, movetype=%d\n",viewing->health, viewing->mass, viewing->dmg, viewing->wait, viewing->sounds, vtos(viewing->s.angles), viewing->movetype);
			gi.dprintf("targetname=%s, target=%s, spawnflags=0x%04x\n",viewing->targetname,viewing->target,viewing->spawnflags);
			gi.dprintf("absmin,absmax,size=%s, %s, %s, range=%g\n",vtos(viewing->absmin),vtos(viewing->absmax),vtos(viewing->size),range);
			gi.dprintf("groundentity=%s\n",(viewing->groundentity ? viewing->groundentity->classname : "NULL"));
		}
		else if (!Q_stricmp(cmd, "item_left"))
			ShiftItem(ent,1);
		else if (!Q_stricmp(cmd, "item_right"))
			ShiftItem(ent,2);
		else if (!Q_stricmp(cmd, "item_forward"))
			ShiftItem(ent,4);
		else if (!Q_stricmp(cmd, "item_back"))
			ShiftItem(ent,8);
		else if (!Q_stricmp(cmd, "item_up"))
			ShiftItem(ent,16);
		else if (!Q_stricmp(cmd, "item_down"))
			ShiftItem(ent,32);
		else if (!Q_stricmp(cmd, "item_drop"))
			ShiftItem(ent,64);
		else if (!Q_stricmp(cmd, "item_pitch"))
			ShiftItem(ent,128);
		else if (!Q_stricmp(cmd, "item_yaw"))
			ShiftItem(ent,256);
		else if (!Q_stricmp(cmd, "item_roll"))
			ShiftItem(ent,512);
		else if (!Q_stricmp(cmd, "item_release"))
			ent->client->shift_dir = 0;
		else if(!Q_stricmp(cmd,"medic_test"))
		{
			extern	int	medic_test;
			if(parm)
				medic_test = atoi(parm);
			else if(medic_test)
				medic_test = 0;
			else
				medic_test = 1;
			gi.dprintf("medic_test is %s\n",(medic_test ? "on" : "off"));
		}
		else if (strstr(cmd, "muzzle")) {
			edict_t	*viewing;
			viewing = LookingAt(ent,0,NULL,NULL);
			if(!viewing)
				return;
			if(!viewing->classname)
				return;
			if(!(viewing->monsterinfo.aiflags & AI_ACTOR))
				return;
			if(gi.argc() < 2)
			{
				gi.dprintf("Muzzle offset=%g, %g, %g\n",
					viewing->muzzle[0],viewing->muzzle[1],viewing->muzzle[2]);
			}
			else
			{
				if(!Q_stricmp(cmd,"muzzlex"))
					viewing->muzzle[0] = atof(gi.argv(1));
				else if(!Q_stricmp(cmd,"muzzley"))
					viewing->muzzle[1] = atof(gi.argv(1));
				else if(!Q_stricmp(cmd,"muzzlez"))
					viewing->muzzle[2] = atof(gi.argv(1));
				else
					gi.dprintf("Syntax: muzzle[x|y|z] <value>\n");
			}
		}
		else if(!Q_stricmp(cmd,"range"))
		{
			vec3_t	forward, point, start;
			trace_t	tr;
			VectorCopy(ent->s.origin,start);

			start[2] += ent->viewheight;
			AngleVectors(ent->client->v_angle,forward,NULL,NULL);
			VectorMA(start,8192,forward,point);
			tr = gi.trace(start,NULL,NULL,point,ent,MASK_SOLID);
			VectorSubtract(tr.endpos,start,point);
			gi.dprintf("range=%g\n",VectorLength(point));
		}
		else if (!Q_stricmp(cmd,"setskill")) {
			if(gi.argc() < 2)
				gi.dprintf("Syntax: setskill X\n");
			else
			{
				int	s = atoi(gi.argv(1));
				gi.cvar_forceset("skill", va("%i", s));
			}
		}
		else if (!Q_stricmp(cmd,"sk")) {
			edict_t *viewing;
			int		skinnum;

			viewing = LookingAt(ent,0,NULL,NULL);
			if(!viewing) 
				return;

			if(parm) {
				skinnum = atoi(parm);
				viewing->s.skinnum = skinnum;
				gi.linkentity(viewing);
			}
			else
				gi.dprintf("Currently using skin #%i\n",viewing->s.skinnum);

		}
		else if(!Q_stricmp(cmd,"spawn"))
		{
			edict_t	*e;
			vec3_t	forward;
		
			if(!parm)
			{
				gi.dprintf("syntax: spawn <classname>\n");
				return;
			}
			e = G_Spawn();
			e->classname = gi.TagMalloc((int)strlen(parm)+1,TAG_LEVEL);
			strcpy(e->classname,parm);
			AngleVectors(ent->client->v_angle,forward,NULL,NULL);
			VectorMA(ent->s.origin,128,forward,e->s.origin);
			e->s.angles[YAW] = ent->s.angles[YAW];
			ED_CallSpawn(e);
		}
		else if(!Q_stricmp(cmd,"spawngoodguy"))
		{
			edict_t	*e;
			vec3_t	forward;
		
			if(gi.argc() < 3)
			{
				gi.dprintf("syntax: spawngoodguy <modelname> <weapon>\n");
				return;
			}
			e = G_Spawn();
			e->classname = gi.TagMalloc(12,TAG_LEVEL);
			strcpy(e->classname,"misc_actor");
			e->usermodel = gi.argv(1);
			e->sounds = atoi(gi.argv(2));
			e->spawnflags = SF_MONSTER_GOODGUY;
			AngleVectors(ent->client->v_angle,forward,NULL,NULL);
			VectorMA(ent->s.origin,128,forward,e->s.origin);
			e->s.origin[2] = max(e->s.origin[2],ent->s.origin[2] + 8);
			e->s.angles[YAW] = ent->s.angles[YAW];
			ED_CallSpawn(e);
			actor_files();
		}
		else if(!Q_stricmp(cmd,"spawnself"))
		{
			edict_t	*decoy;
			vec3_t	forward;

			decoy = G_Spawn();
			decoy->classname    = "fakeplayer";
			memcpy(&decoy->s,&ent->s,sizeof(entity_state_t));
			decoy->s.number     = decoy-g_edicts;
			decoy->s.frame      = ent->s.frame; 
			AngleVectors(ent->client->v_angle,forward,NULL,NULL);
			VectorMA(ent->s.origin,64,forward,decoy->s.origin);
			decoy->s.angles[YAW] = ent->s.angles[YAW]; 
			decoy->takedamage   = DAMAGE_AIM;
			decoy->flags        = (ent->flags & FL_NOTARGET);
			decoy->movetype     = MOVETYPE_TOSS;
			decoy->viewheight   = ent->viewheight;
			decoy->mass         = ent->mass;
			decoy->solid        = SOLID_BBOX;
			decoy->deadflag     = DEAD_NO;
			decoy->clipmask     = MASK_PLAYERSOLID;
			decoy->health       = ent->health;
			decoy->light_level  = ent->light_level;
			decoy->think        = decoy_think;
			decoy->monsterinfo.aiflags = AI_GOOD_GUY;
			decoy->die          = decoy_die;
			decoy->nextthink    = level.time + FRAMETIME;
			VectorCopy(ent->mins,decoy->mins);
			VectorCopy(ent->maxs,decoy->maxs);
			gi.linkentity (decoy); 
		}
		else if (!Q_stricmp(cmd,"switch")) {
			extern mmove_t	actor_move_switch;
			edict_t *viewing;

			viewing = LookingAt(ent,0,NULL,NULL);
			if(!viewing)
				return;
			if(!(viewing->monsterinfo.aiflags & AI_ACTOR))
			{
				gi.dprintf("Must be a misc_actor\n");
				return;
			}
			viewing->monsterinfo.currentmove = &actor_move_switch;
		}
#ifndef KMQUAKE2_ENGINE_MOD // these functions moved clientside in engine
		else if(!Q_stricmp(cmd,"texture")) {
			trace_t	tr;
			vec3_t	forward, start, end;

			if(ent->client->chasetoggle)
				VectorCopy(ent->client->chasecam->s.origin,start);
			else {
				VectorCopy(ent->s.origin, start);
				start[2] += ent->viewheight;
			}
			AngleVectors(ent->client->v_angle, forward, NULL, NULL);
			VectorMA(start, 8192, forward, end);
			tr = gi.trace(start,NULL,NULL,end,ent,MASK_ALL);
			if(!tr.ent)
				gi.dprintf("Nothing hit?\n");
			else {
				if(!tr.surface)
					gi.dprintf("Not a brush\n");
				else
					gi.dprintf("Texture=%s, surface=0x%08x, value=%d\n",tr.surface->name,tr.surface->flags,tr.surface->value);
			}
		}
		else if(!Q_stricmp(cmd,"surf")) {
			trace_t	tr;
			vec3_t	forward, start, end;
			int		s;

			if(gi.argc() < 2)
			{
				gi.dprintf("Syntax: surf <value>\n");
				return;
			}
			else
				s = atoi(gi.argv(1));

			if(ent->client->chasetoggle)
				VectorCopy(ent->client->chasecam->s.origin,start);
			else {
				VectorCopy(ent->s.origin, start);
				start[2] += ent->viewheight;
			}
			AngleVectors(ent->client->v_angle, forward, NULL, NULL);
			VectorMA(start, 8192, forward, end);
			tr = gi.trace(start,NULL,NULL,end,ent,MASK_ALL);
			if(!tr.ent)
				gi.dprintf("Nothing hit?\n");
			else
			{
				if(!tr.surface)
					gi.dprintf("Not a brush\n");
				else
					tr.surface->flags = s;
			}
		}
#endif	// KMQUAKE2_ENGINE_MOD
		else
			Cmd_Say_f (ent, false, true);
	}
	// end debugging stuff

	else	// anything that doesn't match a command will be a chat
		Cmd_Say_f (ent, false, true);
}
