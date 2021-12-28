#include "g_local.h"


/*
=================
check_dodge

This is a support routine used when a client is firing
a non-instant attack weapon.  It checks to see if a
monster's dodge function should be called.
=================
*/
/*static*/ void check_dodge(edict_t *self, vec3_t start, vec3_t dir, int speed)
{
	vec3_t	end;
	vec3_t	v;
	trace_t	tr;
	float	eta;

	// easy mode only ducks one quarter the time
	if (skill->value == 0)
	{
		if (random() > 0.25)
			return;
	}

	VectorMA(start, 8192, dir, end);
	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);
	if ((tr.ent) && (tr.ent->svflags & SVF_MONSTER) && (tr.ent->health > 0) && (tr.ent->monsterinfo.dodge) && infront(tr.ent, self))
	{
		VectorSubtract(tr.endpos, start, v);
		eta = (VectorLength(v) - tr.ent->maxs[0]) / speed;
		tr.ent->monsterinfo.dodge(tr.ent, self, eta);
	}
}

//CW++	Infantry guards can roll-dodge MG bursts
/*static*/ void check_dodge_inf_mg(edict_t *self, vec3_t start, vec3_t dir)
{
	vec3_t	end;
	trace_t	tr;
	float	r;

//	Don't evade all the time.

	r = random();
	if (skill->value == 0)
	{
		if (r > 0.06)
			return;
	}
	else if (skill->value == 1)
		{
		if (r > 0.12)
			return;
	}
	else if (skill->value > 1)
		{
		if (r > 0.18)
			return;
	}

//	Perform evasion calcs.

	VectorMA(start, 2048, dir, end);
	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);
	if ((tr.ent) && (tr.ent->svflags & SVF_MONSTER) && (tr.ent->health > 0) && (tr.ent->monsterinfo.dodge) && infront(tr.ent, self))
	{
		if (!Q_stricmp(tr.ent->common_name, "Enforcer"))
			infantry_dodge(tr.ent, self, 1.1);	//don't duck if can't roll
	}
}
//CW--

/*
=================
fire_hit

Used for all impact (hit/punch/slash) attacks
=================
*/
qboolean fire_hit (edict_t *self, vec3_t aim, int damage, int kick)
{
	trace_t		tr;
	vec3_t		forward, right, up;
	vec3_t		v;
	vec3_t		point;
	float		range;
	vec3_t		dir;

	// Lazarus: Paranoia check
	if (!self->enemy)
		return false;

	//see if enemy is in range
	VectorSubtract (self->enemy->s.origin, self->s.origin, dir);
	range = VectorLength(dir);
	if (range > aim[0])
		return false;

	if (aim[1] > self->mins[0] && aim[1] < self->maxs[0])
	{
		// the hit is straight on so back the range up to the edge of their bbox
		range -= self->enemy->maxs[0];
	}
	else
	{
		// this is a side hit so adjust the "right" value out to the edge of their bbox
		if (aim[1] < 0)
			aim[1] = self->enemy->mins[0];
		else
			aim[1] = self->enemy->maxs[0];
	}

	VectorMA (self->s.origin, range, dir, point);

	tr = gi.trace (self->s.origin, NULL, NULL, point, self, MASK_SHOT);
	if (tr.fraction < 1)
	{
		if (!tr.ent->takedamage)
			return false;
		// if it will hit any client/monster then hit the one we wanted to hit
		if ((tr.ent->svflags & SVF_MONSTER) || (tr.ent->client))
			tr.ent = self->enemy;
	}

	AngleVectors(self->s.angles, forward, right, up);
	VectorMA (self->s.origin, range, forward, point);
	VectorMA (point, aim[1], right, point);
	VectorMA (point, aim[2], up, point);
	VectorSubtract (point, self->enemy->s.origin, dir);

	// do the damage
	T_Damage (tr.ent, self, self, dir, point, vec3_origin, damage, kick/2, DAMAGE_NO_KNOCKBACK, MOD_HIT);

	if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client))
		return false;

	// do our special form of knockback here
	VectorMA (self->enemy->absmin, 0.5, self->enemy->size, v);
	VectorSubtract (v, point, v);
	VectorNormalize (v);
	VectorMA (self->enemy->velocity, kick, v, self->enemy->velocity);
	if (self->enemy->velocity[2] > 0)
		self->enemy->groundentity = NULL;
	return true;
}


/*
=================
fire_lead

This is an internal support routine used for bullet/pellet based weapons.
=================
*/
/*static*/ void fire_lead (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, int mod)
{
	trace_t		tr;
	vec3_t		dir;
	vec3_t		forward, right, up;
	vec3_t		end;
	float		r;
	float		u;
	vec3_t		water_start;
	qboolean	water = false;
	int			content_mask = MASK_SHOT | MASK_WATER;

	tr = gi.trace(self->s.origin, NULL, NULL, start, self, MASK_SHOT);
	if (!(tr.fraction < 1.0))
	{
		vectoangles (aimdir, dir);
		AngleVectors (dir, forward, right, up);

		r = crandom()*hspread;
		u = crandom()*vspread;
		VectorMA (start, 8192, forward, end);
		VectorMA (end, r, right, end);
		VectorMA (end, u, up, end);

		if (gi.pointcontents (start) & MASK_WATER)
		{
			water = true;
			VectorCopy (start, water_start);
			content_mask &= ~MASK_WATER;
		}

		tr = gi.trace (start, NULL, NULL, end, self, content_mask);

		// see if we hit water
		if (tr.contents & MASK_WATER)
		{
			int		color;

			water = true;
			VectorCopy (tr.endpos, water_start);

			if (!VectorCompare (start, tr.endpos))
			{
				if (tr.ent->svflags & SVF_MUD)
					color = SPLASH_BROWN_WATER;
				else if (tr.contents & CONTENTS_WATER)
				{
					if (strcmp(tr.surface->name, "*brwater") == 0)
						color = SPLASH_BROWN_WATER;
					else
						color = SPLASH_BLUE_WATER;
				}
				else if (tr.contents & CONTENTS_SLIME)
					color = SPLASH_SLIME;
				else if (tr.contents & CONTENTS_LAVA)
					color = SPLASH_LAVA;
				else
					color = SPLASH_UNKNOWN;

				if (color != SPLASH_UNKNOWN)
				{
					gi.WriteByte (svc_temp_entity);
					gi.WriteByte (TE_SPLASH);
					gi.WriteByte (8);
					gi.WritePosition (tr.endpos);
					gi.WriteDir (tr.plane.normal);
					gi.WriteByte (color);
					gi.multicast (tr.endpos, MULTICAST_PVS);
				}

				// change bullet's course when it enters water
				VectorSubtract (end, start, dir);
				vectoangles (dir, dir);
				AngleVectors (dir, forward, right, up);
				r = crandom()*hspread*2;
				u = crandom()*vspread*2;
				VectorMA (water_start, 8192, forward, end);
				VectorMA (end, r, right, end);
				VectorMA (end, u, up, end);
			}

			// re-trace ignoring water this time
			tr = gi.trace (water_start, NULL, NULL, end, self, MASK_SHOT);
		}
	}

	// send gun puff / flash
	if (!((tr.surface) && (tr.surface->flags & SURF_SKY)))
	{
		if (tr.fraction < 1.0)
		{
			if (tr.ent->takedamage)
			{
				T_Damage (tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, DAMAGE_BULLET, mod);
			}
			else
			{
				if (strncmp (tr.surface->name, "sky", 3) != 0)
				{
					gi.WriteByte (svc_temp_entity);
					gi.WriteByte (te_impact);
					gi.WritePosition (tr.endpos);
					gi.WriteDir (tr.plane.normal);
					gi.multicast (tr.endpos, MULTICAST_PVS);

					if (level.num_reflectors)
						ReflectSparks(te_impact,tr.endpos,tr.plane.normal);

					if (self->client)
						PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
				}
			}
		}
	}

	// if went through water, determine where the end and make a bubble trail
	if (water)
	{
		vec3_t	pos;

		VectorSubtract (tr.endpos, water_start, dir);
		VectorNormalize (dir);
		VectorMA (tr.endpos, -2, dir, pos);
		if (gi.pointcontents (pos) & MASK_WATER)
			VectorCopy (pos, tr.endpos);
		else
			tr = gi.trace (pos, NULL, NULL, water_start, tr.ent, MASK_WATER);

		VectorAdd (water_start, tr.endpos, pos);
		VectorScale (pos, 0.5, pos);

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BUBBLETRAIL);
		gi.WritePosition (water_start);
		gi.WritePosition (tr.endpos);
		gi.multicast (pos, MULTICAST_PVS);
	}
}


/*
=================
fire_bullet

Fires a single round.  Used for machinegun and chaingun.  Would be fine for
pistols, rifles, etc....
=================
*/
void fire_bullet (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int mod)
{
	fire_lead (self, start, aimdir, damage, kick, TE_GUNSHOT, hspread, vspread, mod);

//CW++
	if (self->client && (MOD_MACHINEGUN == mod))
		check_dodge_inf_mg(self, start, aimdir);
//CW--
}


/*
=================
fire_shotgun

Shoots shotgun pellets.  Used by shotgun and super shotgun.
=================
*/
void fire_shotgun (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int mod)
{
	int		i;

	for (i = 0; i < count; i++)
		fire_lead (self, start, aimdir, damage, kick, TE_SHOTGUN, hspread, vspread, mod);
}


/*
=================
fire_blaster

Fires a single blaster bolt.  Used by the blaster and hyper blaster.
=================
*/

void blaster_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int		mod;
	int		tempevent;

	if (other == self->owner)	
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

//CW+++ If the bolt hits a func_force_wall, we want it to bounce off. The actual
//		actual reversal of the entity's velocity happens automatically as part of
//		the game physics, so all we need to do here is make sure that the bolt
//		(a) isn't G_FreeEdict'ed, and (b) can now harm the player.
	
	if (other->classname && !Q_stricmp(other->classname, "force_wall"))
	{
		if (other->spawnflags & FWALL_IMPACTSOUND)
			gi.sound(other, CHAN_GIZMO, gi.soundindex("smdfx/fieldimpact.wav"), 1, ATTN_NORM, 0);

		if (other->spawnflags & FWALL_REFLECT)
		{
			self->owner = self;
			return;
		}
	}
//CW---

	if (other->takedamage)
	{
		if (self->spawnflags & 1)
			mod = MOD_HYPERBLASTER;
		else
			mod = MOD_BLASTER;
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 1, DAMAGE_ENERGY, mod);
	}
	else
	{
		if (self->style == BLASTER_GREEN) //green
			tempevent = TE_BLASTER2;
		else if (self->style == BLASTER_BLUE) //blue
	#ifdef KMQUAKE2_ENGINE_MOD // Knightmare- looks better than flechette
			tempevent =  TE_BLUEHYPERBLASTER;
	#else
			tempevent = TE_FLECHETTE;
	#endif
	#ifdef KMQUAKE2_ENGINE_MOD
		else if (self->style == BLASTER_RED) //red
			tempevent =  TE_REDBLASTER;
	#endif
		else //standard yellow
			tempevent = TE_BLASTER;

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(tempevent);
		gi.WritePosition(self->s.origin);
		if (!plane)
			gi.WriteDir(vec3_origin);
		else
			gi.WriteDir(plane->normal);

		gi.multicast(self->s.origin, MULTICAST_PVS);

		if (level.num_reflectors)
		{
			if (!plane)
				ReflectSparks(TE_BLASTER,self->s.origin,vec3_origin);
			else
				ReflectSparks(TE_BLASTER,self->s.origin,plane->normal);
		}
	}
	G_FreeEdict(self);
}

void fire_blaster(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect, qboolean hyper, int color)
{
	edict_t	*bolt;
	trace_t	tr;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->svflags = SVF_DEADMONSTER;
	// yes, I know it looks weird that projectiles are deadmonsters
	// what this means is that when prediction is used against the object
	// (blaster/hyperblaster shots), the player won't be solid clipped against
	// the object.  Right now trying to run into a firing hyperblaster
	// is very jerky since you are predicted 'against' the shots.
	VectorCopy(start, bolt->s.origin);
	VectorCopy(start, bolt->s.old_origin);
	vectoangles(dir, bolt->s.angles);
	VectorScale(dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= effect;
	VectorClear(bolt->mins);
	VectorClear(bolt->maxs);

	if (color == BLASTER_GREEN) //green
		bolt->s.modelindex = gi.modelindex ("models/objects/laser2/tris.md2");
	else if (color == BLASTER_BLUE) //blue
		bolt->s.modelindex = gi.modelindex ("models/objects/blaser/tris.md2");
	else if (color == BLASTER_RED) //red
		bolt->s.modelindex = gi.modelindex ("models/objects/rlaser/tris.md2");
	else //standard orange
		bolt->s.modelindex = gi.modelindex("models/objects/laser/tris.md2");
	bolt->style = color;

	bolt->s.sound = gi.soundindex("misc/lasfly.wav");
	bolt->owner = self;
	bolt->touch = blaster_touch;
	bolt->nextthink = level.time + 2;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	bolt->classname = "bolt";
	if (hyper)
		bolt->spawnflags = 1;
	gi.linkentity(bolt);

	if (self->client)
		check_dodge(self, bolt->s.origin, dir, speed);

	tr = gi.trace(self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);
	if (tr.fraction < 1.0)
	{
		VectorMA(bolt->s.origin, -10, dir, bolt->s.origin);
		bolt->touch (bolt, tr.ent, NULL, NULL);
	}
}	

// SP_bolt should ONLY be used for blaster/hyperblaster bolts that have
// changed maps via trigger_transition. It should NOT be used for map
// entities.
void bolt_delayed_start (edict_t *bolt)
{
	if (g_edicts[1].linkcount)
	{
		VectorScale(bolt->movedir,bolt->moveinfo.speed,bolt->velocity);
		bolt->nextthink = level.time + 2;
		bolt->think = G_FreeEdict;
		gi.linkentity(bolt);
	}
	else
		bolt->nextthink = level.time + FRAMETIME;
}
void SP_bolt(edict_t *bolt)
{
	bolt->s.modelindex = gi.modelindex ("models/objects/laser/tris.md2");
	bolt->s.sound = gi.soundindex ("misc/lasfly.wav");
	bolt->touch = blaster_touch;
	VectorCopy(bolt->velocity,bolt->movedir);
	VectorNormalize(bolt->movedir);
	bolt->moveinfo.speed = VectorLength(bolt->velocity);
	VectorClear(bolt->velocity);
	bolt->think = bolt_delayed_start;
	bolt->nextthink = level.time + FRAMETIME;
	gi.linkentity(bolt);
}

//CW+++
/*
=================
fire_plasma

Fires a single plasma bolt.  Used by monster_brains.
=================
*/
void plasma_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int puff_type = TE_SHIELD_SPARKS;

	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

//CW+++ If the bolt hits a func_force_wall, we want it to bounce off. The actual
//		actual reversal of the entity's velocity happens automatically as part of
//		the game physics, so all we need to do here is make sure that the bolt
//		isn't G_FreeEdict'ed.
	if (other->classname && !Q_stricmp(other->classname, "force_wall"))
	{
		if (other->spawnflags & FWALL_IMPACTSOUND)
			gi.sound(other, CHAN_GIZMO, gi.soundindex("smdfx/fieldimpact.wav"), 1, ATTN_NORM, 0);

		if (other->spawnflags & FWALL_REFLECT)
		{
			self->owner = self;
			return;
		}
	}
//CW---

	if (other->takedamage)
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 1, DAMAGE_ENERGY, MOD_PLASMA);
	else
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(puff_type);
		gi.WritePosition (self->s.origin);
		if (!plane)
			gi.WriteDir(vec3_origin);
		else
			gi.WriteDir(plane->normal);

		gi.multicast(self->s.origin, MULTICAST_PVS);
		if (level.num_reflectors)
		{
			if (!plane)
				ReflectSparks(puff_type, self->s.origin, vec3_origin);
			else
				ReflectSparks(puff_type, self->s.origin, plane->normal);
		}
	}
	G_FreeEdict(self);
}

void fire_plasma(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect)
{
	edict_t	*bolt;
	trace_t	tr;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->svflags = SVF_DEADMONSTER;
	VectorCopy(start, bolt->s.origin);
	VectorCopy(start, bolt->s.old_origin);
	vectoangles(dir, bolt->s.angles);
	VectorScale(dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= effect;
	VectorClear(bolt->mins);
	VectorClear(bolt->maxs);
	bolt->s.modelindex = gi.modelindex("sprites/s_plasma.sp2");
	bolt->s.sound = gi.soundindex("misc/lasfly.wav");
	bolt->owner = self;
	bolt->touch = plasma_touch;
	bolt->nextthink = level.time + 2;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	bolt->classname = "plasma_bolt";
	gi.linkentity(bolt);

	if (self->client)
		check_dodge(self, bolt->s.origin, dir, speed);

	tr = gi.trace(self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);
	if (tr.fraction < 1.0)
	{
		VectorMA (bolt->s.origin, -10, dir, bolt->s.origin);
		bolt->touch(bolt, tr.ent, NULL, NULL);
	}
}

void SP_plasma_bolt(edict_t *bolt)
{
	bolt->s.modelindex = gi.modelindex("sprites/s_plasma.sp2");
	bolt->s.sound = gi.soundindex("misc/lasfly.wav");
	bolt->touch = plasma_touch;
	VectorCopy(bolt->velocity, bolt->movedir);
	VectorNormalize(bolt->movedir);
	bolt->moveinfo.speed = VectorLength(bolt->velocity);
	VectorClear(bolt->velocity);
	bolt->think = bolt_delayed_start;
	bolt->nextthink = level.time + FRAMETIME;
	gi.linkentity(bolt);
}
//CW---

/*
=================
fire_grenade
=================
*/

// Lazarus additions: next_grenade and prev_grenade linked lists facilitate checking
// for grenades near monsters, so that monsters can evade w/o bogging down the game

void Grenade_Evade (edict_t *monster)
{
	edict_t	*grenade;
	vec3_t	grenade_vec;
	float	grenade_dist=0.f, best_r, best_yaw=0.f, r;
	float	yaw;
	int		i;
	vec3_t	forward;
	vec3_t	pos, best_pos;
	trace_t	tr;

	// We assume on entry here that monster is alive and that he's not already
	// AI_CHASE_THING
	grenade = world->next_grenade;
	while(grenade)
	{
		// we only care about grenades on the ground
		if (grenade->inuse && grenade->groundentity)
		{
			// if it ain't in the PVS, it can't hurt us (I think?)
			if (gi.inPVS(grenade->s.origin,monster->s.origin))
			{
				VectorSubtract(grenade->s.origin,monster->s.origin,grenade_vec);
				grenade_dist = VectorNormalize(grenade_vec);
				if (grenade_dist <= grenade->dmg_radius)
					break;
			}
		}
		grenade = grenade->next_grenade;
	}
	if (!grenade)
		return;
	// Find best escape route.
	best_r = 9999;
	for (i=0; i<8; i++)
	{
		yaw = anglemod( i*45 );
		forward[0] = cos( DEG2RAD(yaw) );
		forward[1] = sin( DEG2RAD(yaw) );
		forward[2] = 0;
		// Estimate of required distance to run. This is conservative.
		r = grenade->dmg_radius + grenade_dist*DotProduct(forward,grenade_vec) + monster->size[0] + 16;
		if ( r < best_r )
		{
			VectorMA(monster->s.origin,r,forward,pos);
			tr = gi.trace(monster->s.origin,monster->mins,monster->maxs,pos,monster,MASK_MONSTERSOLID);
			if (tr.fraction < 1.0)
				continue;
			best_r = r;
			best_yaw = yaw;
			VectorCopy(tr.endpos,best_pos);
		}
	}
	if (best_r < 9000)
	{
		edict_t	*thing = SpawnThing();
		VectorCopy(best_pos,thing->s.origin);
		thing->touch_debounce_time = grenade->nextthink;
		thing->target_ent = monster;
		ED_CallSpawn(thing);
		monster->ideal_yaw  = best_yaw;
		monster->movetarget = monster->goalentity = thing;
		monster->monsterinfo.aiflags &= ~AI_SOUND_TARGET;
		monster->monsterinfo.aiflags |= (AI_CHASE_THING | AI_EVADE_GRENADE);
		monster->monsterinfo.run(monster);
		monster->next_grenade = grenade;
	}
}

/*static*/ void Grenade_Add_To_Chain (edict_t *grenade)
{
	edict_t	*ancestor;

	ancestor = world;
	while(ancestor->next_grenade && ancestor->next_grenade->inuse)
		ancestor = ancestor->next_grenade;
	ancestor->next_grenade = grenade;
	grenade->prev_grenade = ancestor;
}

/*static*/ void Grenade_Remove_From_Chain (edict_t *grenade)
{
	if (grenade->prev_grenade)
	{
		// "prev_grenade" should always be valid for other than player-thrown
		// grenades that explode in player's hand
		grenade->prev_grenade->next_grenade = grenade->next_grenade;
		if (grenade->next_grenade)
			grenade->next_grenade->prev_grenade = grenade->prev_grenade;
	}
}

/*static*/ void Grenade_Explode (edict_t *ent)
{
	vec3_t		origin;
	int			mod;
	int			type;

	Grenade_Remove_From_Chain(ent);

	if (ent->owner && ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	//FIXME: if we are onground then raise our Z just a bit since we are a point?
	if (ent->enemy)
	{
		float	points;
		vec3_t	v;
		vec3_t	dir;

		VectorAdd (ent->enemy->mins, ent->enemy->maxs, v);
		VectorMA (ent->enemy->s.origin, 0.5, v, v);
		VectorSubtract (ent->s.origin, v, v);
		points = ent->dmg - 0.5 * VectorLength (v);
		VectorSubtract (ent->enemy->s.origin, ent->s.origin, dir);
		if (ent->spawnflags & 1)
			mod = MOD_HANDGRENADE;
		else
			mod = MOD_GRENADE;
		T_Damage (ent->enemy, ent, ent->owner, dir, ent->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
	}

	if (ent->spawnflags & 2)
		mod = MOD_HELD_GRENADE;
	else if (ent->spawnflags & 1)
		mod = MOD_HG_SPLASH;
	else
		mod = MOD_G_SPLASH;
	T_RadiusDamage(ent, ent->owner, ent->dmg, ent->enemy, ent->dmg_radius, mod, -0.5);

	VectorMA (ent->s.origin, -0.02, ent->velocity, origin);
	gi.WriteByte (svc_temp_entity);
	if (ent->waterlevel)
	{
		if (ent->groundentity)
			type = TE_GRENADE_EXPLOSION_WATER;
		else
			type = TE_ROCKET_EXPLOSION_WATER;
	}
	else
	{
		if (ent->groundentity)
			type = TE_GRENADE_EXPLOSION;
		else
			type = TE_ROCKET_EXPLOSION;
	}
	gi.WriteByte (type);
	gi.WritePosition (origin);
	gi.multicast (ent->s.origin, MULTICAST_PHS);

	if (level.num_reflectors)
		ReflectExplosion (type,origin);

	G_FreeEdict (ent);
}

/*static*/ void Grenade_Touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		Grenade_Remove_From_Chain (ent);
		G_FreeEdict (ent);
		return;
	}

	if (!other->takedamage)
	{
		if (ent->spawnflags & 1)
		{
			if (random() > 0.5)
				gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/hgrenb1a.wav"), 1, ATTN_NORM, 0);
			else
				gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/hgrenb2a.wav"), 1, ATTN_NORM, 0);
		}
		else
		{
			gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/grenlb1b.wav"), 1, ATTN_NORM, 0);
		}
		return;
	}

	ent->enemy = other;
	Grenade_Explode (ent);
}

/*static*/ void ContactGrenade_Touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		Grenade_Remove_From_Chain (ent);
		G_FreeEdict (ent);
		return;
	}

	if (other->takedamage)
		ent->enemy = other;

	Grenade_Explode (ent);
}

void fire_grenade (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius, qboolean contact)
{
	edict_t	*grenade;
	vec3_t	dir;
	vec3_t	forward, right, up;

	vectoangles (aimdir, dir);
	AngleVectors (dir, forward, right, up);

	grenade = G_Spawn();
	VectorCopy (start, grenade->s.origin);
	VectorScale (aimdir, speed, grenade->velocity);
	// Lazarus - keep same vertical boost for players, but monsters do a better job
	//           of calculating aim direction, so throw that out
	if (self->client)
		VectorMA (grenade->velocity, 200 + crandom() * 10.0, up, grenade->velocity);
	else
		VectorMA (grenade->velocity, crandom() * 10.0, up, grenade->velocity);

	VectorMA (grenade->velocity, crandom() * 10.0, right, grenade->velocity);
	// Lazarus: Add owner velocity
//	VectorAdd (grenade->velocity, self->velocity, grenade->velocity);
	// NO. This is too unrealistic. Instead, if owner is riding a moving entity,
	//     add velocity of the thing he's riding
	if (self->groundentity)
		VectorAdd (grenade->velocity, self->groundentity->velocity, grenade->velocity);


	VectorSet (grenade->avelocity, 300, 300, 300);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->clipmask = MASK_SHOT | CONTENTS_GRILL;	//CW: can't pass through a grill
	grenade->solid = SOLID_BBOX;
	grenade->s.effects |= EF_GRENADE;
	VectorClear (grenade->mins);
	VectorClear (grenade->maxs);
	grenade->s.modelindex = gi.modelindex ("models/objects/grenade/tris.md2");
	grenade->owner = self;
	if (contact)
		grenade->touch = ContactGrenade_Touch;
	else
		grenade->touch = Grenade_Touch;
	grenade->nextthink = level.time + timer;
	grenade->think = Grenade_Explode;
	grenade->dmg = damage;
	grenade->dmg_radius = damage_radius;
	grenade->classname = "grenade";

	Grenade_Add_To_Chain (grenade);

	gi.linkentity (grenade);
}

void fire_grenade2 (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius, qboolean held)
{
	edict_t	*grenade;
	vec3_t	dir;
	vec3_t	forward, right, up;

	vectoangles (aimdir, dir);
	AngleVectors (dir, forward, right, up);

	grenade = G_Spawn();
	VectorCopy (start, grenade->s.origin);
	VectorScale (aimdir, speed, grenade->velocity);
	VectorMA (grenade->velocity, 200 + crandom() * 10.0, up, grenade->velocity);
	VectorMA (grenade->velocity, crandom() * 10.0, right, grenade->velocity);

	// Lazarus: Add owner velocity
//	VectorAdd (grenade->velocity, self->velocity, grenade->velocity);
	// NO. This is too unrealistic. Instead, if owner is riding a moving entity,
	//     add velocity of the thing he's riding
	if (self->groundentity)
		VectorAdd (grenade->velocity, self->groundentity->velocity, grenade->velocity);
	
	VectorSet (grenade->avelocity, 300, 300, 300);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->clipmask = MASK_SHOT | CONTENTS_GRILL;	//CW: can't pass through a grill
	grenade->solid = SOLID_BBOX;
	grenade->s.effects |= EF_GRENADE;
	VectorClear (grenade->mins);
	VectorClear (grenade->maxs);
	grenade->s.modelindex = gi.modelindex ("models/objects/grenade2/tris.md2");
	grenade->owner = self;
	grenade->touch = Grenade_Touch;
	grenade->nextthink = level.time + timer;
	grenade->think = Grenade_Explode;
	grenade->dmg = damage;
	grenade->dmg_radius = damage_radius;
	grenade->classname = "hgrenade";
	if (held)
		grenade->spawnflags = 3;
	else
		grenade->spawnflags = 1;
	grenade->s.sound = gi.soundindex("weapons/hgrenc1b.wav");

	if (timer <= 0.0)
		Grenade_Explode (grenade);
	else
	{
		gi.sound (self, CHAN_WEAPON, gi.soundindex ("weapons/hgrent1a.wav"), 1, ATTN_NORM, 0);
		Grenade_Add_To_Chain (grenade);
		gi.linkentity (grenade);
	}
}

// NOTE: SP_grenade and SP_handgrenade should ONLY be used to spawn grenades that change
//       maps via a trigger_transition. They should NOT be used for map entities

void grenade_delayed_start (edict_t *grenade)
{
	if (g_edicts[1].linkcount)
	{
		VectorScale(grenade->movedir,grenade->moveinfo.speed,grenade->velocity);
		grenade->movetype  = MOVETYPE_BOUNCE;
		grenade->nextthink = level.time + 2.5;
		grenade->think     = Grenade_Explode;
		gi.linkentity(grenade);
	}
	else
		grenade->nextthink = level.time + FRAMETIME;
}

void SP_grenade (edict_t *grenade)
{
	grenade->s.modelindex = gi.modelindex ("models/objects/grenade/tris.md2");
	grenade->touch = Grenade_Touch;

	// For SP, freeze grenade until player spawns in
	if (game.maxclients == 1)
	{
		grenade->movetype  = MOVETYPE_NONE;
		VectorCopy(grenade->velocity,grenade->movedir);
		VectorNormalize(grenade->movedir);
		grenade->moveinfo.speed = VectorLength(grenade->velocity);
		VectorClear(grenade->velocity);
		grenade->think     = grenade_delayed_start;
		grenade->nextthink = level.time + FRAMETIME;
	}
	else
	{
		grenade->movetype  = MOVETYPE_BOUNCE;
		grenade->nextthink = level.time + 2.5;
		grenade->think     = Grenade_Explode;
	}
	gi.linkentity (grenade);

}
void handgrenade_delayed_start (edict_t *grenade)
{
	if (g_edicts[1].linkcount)
	{
		VectorScale(grenade->movedir,grenade->moveinfo.speed,grenade->velocity);
		grenade->movetype  = MOVETYPE_BOUNCE;
		grenade->nextthink = level.time + 2.5;
		grenade->think     = Grenade_Explode;
		if (grenade->owner)
			gi.sound (grenade->owner, CHAN_WEAPON, gi.soundindex ("weapons/hgrent1a.wav"), 1, ATTN_NORM, 0);
		gi.linkentity(grenade);
	}
	else
		grenade->nextthink = level.time + FRAMETIME;
}

void SP_handgrenade (edict_t *grenade)
{
	grenade->s.modelindex = gi.modelindex ("models/objects/grenade2/tris.md2");
	grenade->touch = Grenade_Touch;

	// For SP, freeze grenade until player spawns in
	if (game.maxclients == 1)
	{
		grenade->movetype  = MOVETYPE_NONE;
		VectorCopy(grenade->velocity,grenade->movedir);
		VectorNormalize(grenade->movedir);
		grenade->moveinfo.speed = VectorLength(grenade->velocity);
		VectorClear(grenade->velocity);
		grenade->think     = handgrenade_delayed_start;
		grenade->nextthink = level.time + FRAMETIME;
	}
	else
	{
		grenade->movetype  = MOVETYPE_BOUNCE;
		grenade->nextthink = level.time + 2.5;
		grenade->think     = Grenade_Explode;
	}
	gi.linkentity (grenade);

}

// Lazarus: homing rocket
void homing_think (edict_t *self)
{
	trace_t	tr;
	vec3_t	dir, target;
	vec_t	speed;

	if (level.time > self->endtime)
	{
		if (self->owner->client && (self->owner->client->homing_rocket == self))
			self->owner->client->homing_rocket = NULL;
		BecomeExplosion1(self);
		return;
	}
	if (self->enemy && self->enemy->inuse)
	{
		VectorMA(self->enemy->absmin,0.5,self->enemy->size,target);
		tr = gi.trace(self->s.origin,vec3_origin,vec3_origin,target,self,MASK_OPAQUE);
		if (tr.fraction == 1)
		{
			// target in view; apply correction
			VectorSubtract(target, self->s.origin, dir);
			VectorNormalize(dir);
			if (self->enemy->client)
				VectorScale(dir, 0.8+0.1*skill->value, dir);
			else
				VectorScale(dir, 1.0, dir);  // 0=no correction, 1=turn on a dime
			VectorAdd(dir, self->movedir, dir);
			VectorNormalize(dir);
			VectorCopy(dir, self->movedir);
			vectoangles(dir, self->s.angles);
			speed = VectorLength(self->velocity);
			VectorScale(dir, speed, self->velocity);

			if (level.time >= self->starttime && self->starttime > 0)
			{
				if (level.time > self->owner->fly_sound_debounce_time)
				{
					// this prevents multiple lockon sounds resulting from
					// monsters firing multiple rockets in quick succession
					if (self->enemy->client)
						gi.sound (self->enemy, CHAN_AUTO, gi.soundindex ("weapons/homing/lockon.wav"), 1, ATTN_NORM, 0);
					self->owner->fly_sound_debounce_time = level.time + 2.0;
				}
				self->starttime = 0;
			}
		}
	}
	self->nextthink = level.time + FRAMETIME;
}

/*
=================
fire_rocket
=================
*/
// Lazarus: Rocket_Evade tells monsters to get da hell outta da way

void Rocket_Evade (edict_t *rocket, vec3_t	dir, float speed)
{
	float	rocket_dist, best_r, best_yaw = 0.0, dist, r;
	float	time;
	float	dot;
	float	yaw;
	int		i;
	edict_t	*ent=NULL;
	trace_t	tr;
	vec3_t	hitpoint;
	vec3_t	forward, pos, best_pos;
	vec3_t	rocket_vec, vec;

	// Find out what rocket will hit, assuming everything remains static
	VectorMA(rocket->s.origin, 8192, dir, rocket_vec);
	tr = gi.trace(rocket->s.origin,rocket->mins,rocket->maxs,rocket_vec,rocket,MASK_SHOT|CONTENTS_GRILL);	//CW: can't pass through a grill
	VectorCopy(tr.endpos, hitpoint);
	VectorSubtract(hitpoint, rocket->s.origin, vec);
	dist = VectorLength(vec);
	time = dist / speed;

	while ((ent = findradius(ent, hitpoint, rocket->dmg_radius)) != NULL)
	{
		if (!ent->inuse)
			continue;
		if (!(ent->svflags & SVF_MONSTER))
			continue;
		if (!ent->takedamage)
			continue;
		if (ent->health <= 0)
			continue;
		if (!ent->monsterinfo.run)	// takes care of turret_driver
			continue;
		if (rocket->owner == ent)
			continue;

		VectorSubtract(hitpoint, ent->s.origin, rocket_vec);
		rocket_dist = VectorNormalize(rocket_vec);

		// Not much hope in evading if distance is < 1K or so (//CW: unless Chick or Infantry)}

//CW++
		if (!Q_stricmp(ent->common_name, "Iron Maiden"))
		{
			if ((time > 0.2) && (time < 0.8))
			{
				chick_dodge(ent, rocket->owner, 1.1);	//force backflip
				continue;
			}
		}
		else if (!Q_stricmp(ent->common_name, "Enforcer"))
		{
			if (time > 0.2)
			{
				infantry_dodge(ent, rocket->owner, 1.1);	//don't duck if can't roll
				continue;
			}
		}
//CW--
		else if (rocket_dist < 1024)
			continue;

		// Find best escape route.
		best_r = 9999;
		for (i=0; i<8; i++)
		{
			yaw = anglemod( i*45 );
			forward[0] = cos( DEG2RAD(yaw) );
			forward[1] = sin( DEG2RAD(yaw) );
			forward[2] = 0;
			dot = DotProduct(forward,dir);
			if ((dot > 0.96) || (dot < -0.96))
				continue;
			// Estimate of required distance to run. This is conservative.
			r = rocket->dmg_radius + rocket_dist*DotProduct(forward,rocket_vec) + ent->size[0] + 16;
			if (r < best_r)
			{
				VectorMA(ent->s.origin,r,forward,pos);
				tr = gi.trace(ent->s.origin,ent->mins,ent->maxs,pos,ent,MASK_MONSTERSOLID);
				if (tr.fraction < 1.0)
					continue;
				best_r = r;
				best_yaw = yaw;
				VectorCopy(tr.endpos,best_pos);
			}
		}

		if (best_r < 9000)
		{
			edict_t	*thing = SpawnThing();
			VectorCopy(best_pos,thing->s.origin);
			thing->touch_debounce_time = level.time + time;
			thing->target_ent = ent;
			ED_CallSpawn(thing);
			ent->ideal_yaw  = best_yaw;
			ent->movetarget = ent->goalentity = thing;
			ent->monsterinfo.aiflags &= ~AI_SOUND_TARGET;
			ent->monsterinfo.aiflags |= (AI_CHASE_THING | AI_EVADE_GRENADE);
			ent->monsterinfo.run(ent);
		}
	}
}

void rocket_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t	origin;
	int		n;
	int		type;

	if (other == ent->owner)
		return;

	if (ent->owner->client && (ent->owner->client->homing_rocket == ent))
		ent->owner->client->homing_rocket = NULL;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (ent);
		return;
	}

	if (ent->owner && ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	// calculate position for the explosion entity
	VectorMA (ent->s.origin, -0.02, ent->velocity, origin);

	if (other->takedamage)
	{
		T_Damage (other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, 0, MOD_ROCKET);
	}
	else
	{
		// don't throw any debris in net games
		if (!deathmatch->value && !coop->value)
		{
			if ((surf) && !(surf->flags & (SURF_WARP|SURF_TRANS33|SURF_TRANS66|SURF_FLOWING)))
			{
				n = rand() % 5;
				while(n--)
					ThrowDebris (ent, "models/objects/debris2/tris.md2", 2, ent->s.origin, 0, 0);
			}
		}
	}

	// Lazarus: bad monsters have a large damage radius
	if (ent->owner && (ent->owner->svflags & SVF_MONSTER) && !(ent->owner->monsterinfo.aiflags & AI_GOOD_GUY))
		T_RadiusDamage(ent, ent->owner, ent->radius_dmg, other, ent->dmg_radius + 17.5*skill->value, MOD_R_SPLASH, -2.0/(4.0+skill->value) );
	else
		T_RadiusDamage(ent, ent->owner, ent->radius_dmg, other, ent->dmg_radius, MOD_R_SPLASH, -0.5);

	gi.WriteByte (svc_temp_entity);
	if (ent->waterlevel)
		type = TE_ROCKET_EXPLOSION_WATER;
	else
		type = TE_ROCKET_EXPLOSION;
	gi.WriteByte (type);
	gi.WritePosition (origin);
	gi.multicast (ent->s.origin, MULTICAST_PHS);

	if (level.num_reflectors)
		ReflectExplosion(type,origin);

	G_FreeEdict (ent);
}

/*static*/ void rocket_explode (edict_t *ent)
{
	vec3_t	origin;
	int		type;

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	// calculate position for the explosion entity
	VectorMA (ent->s.origin, -0.02, ent->velocity, origin);

	T_RadiusDamage(ent, ent->owner, ent->radius_dmg, NULL, ent->dmg_radius, MOD_R_SPLASH, -0.5);

	gi.WriteByte (svc_temp_entity);
	if (ent->waterlevel)
		type = TE_ROCKET_EXPLOSION_WATER;
	else
		type = TE_ROCKET_EXPLOSION;
	gi.WriteByte (type);
	gi.WritePosition (origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	if (level.num_reflectors)
		ReflectExplosion(type,origin);

	G_FreeEdict (ent);
}
/*static*/ void rocket_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->takedamage = DAMAGE_NO;
	self->nextthink = level.time + FRAMETIME;
	self->think = rocket_explode;
}

void fire_rocket (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage, edict_t *home_target)
{
	edict_t	*rocket;

	rocket = G_Spawn();
	VectorCopy (start, rocket->s.origin);
	VectorCopy (dir, rocket->movedir);
	vectoangles (dir, rocket->s.angles);
	VectorScale (dir, speed, rocket->velocity);
	// Lazarus: add shooter's lateral velocity
	if (rocket_strafe->value)
	{
		vec3_t	right, up;
		vec3_t	lateral_speed;

		AngleVectors(self->s.angles,NULL,right,up);
		VectorCopy(self->velocity,lateral_speed);
		lateral_speed[0] *= fabs(right[0]);
		lateral_speed[1] *= fabs(right[1]);
		lateral_speed[2] *= fabs(up[2]);
		VectorAdd(rocket->velocity,lateral_speed,rocket->velocity);
	}

	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->clipmask = MASK_SHOT | CONTENTS_GRILL;	//CW: can't pass through a grill
	rocket->solid = SOLID_BBOX;
	rocket->s.effects |= EF_ROCKET;
	rocket->s.renderfx |=	RF_IR_VISIBLE|RF_NOSHADOW; // Knightmare- no shadow
	VectorClear (rocket->mins);
	VectorClear (rocket->maxs);
	if (home_target)
		rocket->s.modelindex = gi.modelindex ("models/objects/hrocket/tris.md2");
	else
		rocket->s.modelindex = gi.modelindex ("models/objects/rocket/tris.md2");
	rocket->owner = self;
	rocket->touch = rocket_touch;
	rocket->dmg = damage;
	rocket->radius_dmg = radius_damage;
	rocket->dmg_radius = damage_radius;
	rocket->s.sound = gi.soundindex ("weapons/rockfly.wav");

	if (home_target)
	{
		// homers are shootable
		VectorSet(rocket->mins, -10, -3, 0);
		VectorSet(rocket->maxs,  10,  3, 6);
		rocket->mass = 10;
		rocket->health = 5;
		rocket->die = rocket_die;
		rocket->takedamage = DAMAGE_YES;
		rocket->monsterinfo.aiflags = AI_NOSTEP;

		rocket->enemy     = home_target;
		rocket->classname = "homing rocket";
		rocket->nextthink = level.time + FRAMETIME;
		rocket->think = homing_think;
		rocket->starttime = level.time + 0.3; // play homing sound on 3rd frame
		rocket->endtime   = level.time + 8000/speed;

		if (self->client)
		{
			self->client->homing_rocket = rocket;
//			check_dodge (self, rocket->s.origin, dir, speed);
		}
		Rocket_Evade(rocket, dir, speed);
	}
	else
	{
		rocket->classname = "rocket";
		rocket->nextthink = level.time + 8000/speed;
		rocket->think = G_FreeEdict;
		Rocket_Evade(rocket, dir, speed);
	}

	gi.linkentity (rocket);
}

// NOTE: SP_rocket should ONLY be used to spawn rockets that change maps
//       via a trigger_transition. It should NOT be used for map entities

void rocket_delayed_start (edict_t *rocket)
{
	if (g_edicts[1].linkcount)
	{
		VectorScale(rocket->movedir,rocket->moveinfo.speed,rocket->velocity);
		rocket->nextthink = level.time + 8000/rocket->moveinfo.speed;
		rocket->think = G_FreeEdict;
		gi.linkentity(rocket);
	}
	else
		rocket->nextthink = level.time + FRAMETIME;
}

void SP_rocket (edict_t *rocket)
{
	vec3_t	dir;

	rocket->s.modelindex = gi.modelindex ("models/objects/rocket/tris.md2");
	rocket->s.sound      = gi.soundindex ("weapons/rockfly.wav");
	rocket->touch = rocket_touch;
	AngleVectors(rocket->s.angles,dir,NULL,NULL);
	VectorCopy (dir, rocket->movedir);
	rocket->moveinfo.speed = VectorLength(rocket->velocity);
	if (rocket->moveinfo.speed <= 0)
		rocket->moveinfo.speed = 650;

	// For SP, freeze rocket until player spawns in
	if (game.maxclients == 1)
	{
		VectorClear(rocket->velocity);
		rocket->think = rocket_delayed_start;
		rocket->nextthink = level.time + FRAMETIME;
	}
	else
	{
		rocket->think = G_FreeEdict;
		rocket->nextthink = level.time + 8000/rocket->moveinfo.speed;
	}
	gi.linkentity (rocket);
}

/*
=================
fire_rail
=================
*/
void fire_rail (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick)
{
	vec3_t		from;
	vec3_t		end;
	trace_t		tr;
	edict_t		*ignore;
	int			mask, tempevent, i=0;
	qboolean	water;

	//Knightmare- changeable trail color
#ifdef KMQUAKE2_ENGINE_MOD
	if (self->client && sk_rail_color->value == 2)
		tempevent = TE_RAILTRAIL2;
	else
#endif
		tempevent = TE_RAILTRAIL;

	VectorMA (start, 8192, aimdir, end);
	VectorCopy (start, from);
	ignore = self;
	water = false;
	mask = MASK_SHOT|CONTENTS_SLIME|CONTENTS_LAVA;
	while (ignore && i<256)
	{
		tr = gi.trace (from, NULL, NULL, end, ignore, mask);

		if (tr.contents & (CONTENTS_SLIME|CONTENTS_LAVA))
		{
			mask &= ~(CONTENTS_SLIME|CONTENTS_LAVA);
			water = true;
		}
		else
		{
			//ZOID--added so rail goes through SOLID_BBOX entities (gibs, etc)
			if ((tr.ent->svflags & SVF_MONSTER) || (tr.ent->client) ||
				(tr.ent->solid == SOLID_BBOX))
				ignore = tr.ent;
			else
				ignore = NULL;

			if ((tr.ent != self) && (tr.ent->takedamage))
				T_Damage (tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, 0, MOD_RAILGUN);
		}

		VectorCopy (tr.endpos, from);
		i++;
	}

	// send gun puff / flash
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (tempevent);
	gi.WritePosition (start);
	gi.WritePosition (tr.endpos);
	gi.multicast (self->s.origin, MULTICAST_PHS);

	if (level.num_reflectors)
		ReflectTrail(tempevent,start,tr.endpos);

	if (water)
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (tempevent);
		gi.WritePosition (start);
		gi.WritePosition (tr.endpos);
		gi.multicast (tr.endpos, MULTICAST_PHS);
	}

	if (self->client)
		PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
}

/*
=================
fire_bfg
=================
*/
void bfg_explode (edict_t *self)
{
	edict_t	*ent;
	float	points;
	vec3_t	v;
	float	dist;

	if (self->s.frame == 0)
	{
		// the BFG effect
		ent = NULL;
		while ((ent = findradius(ent, self->s.origin, self->dmg_radius)) != NULL)
		{
			if (!ent->takedamage)
				continue;
			if (ent == self->owner)
				continue;
			if (!CanDamage (ent, self))
				continue;
			if (!CanDamage (ent, self->owner))
				continue;

			VectorAdd (ent->mins, ent->maxs, v);
			VectorMA (ent->s.origin, 0.5, v, v);
			VectorSubtract (self->s.origin, v, v);
			dist = VectorLength(v);
			points = self->radius_dmg * (1.0 - sqrt(dist/self->dmg_radius));
			if (ent == self->owner)
				points = points * 0.5;

			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_BFG_EXPLOSION);
			gi.WritePosition (ent->s.origin);
			gi.multicast (ent->s.origin, MULTICAST_PHS);

			if (level.num_reflectors)
				ReflectExplosion(TE_BFG_EXPLOSION,ent->s.origin);

			T_Damage (ent, self, self->owner, self->velocity, ent->s.origin, vec3_origin, (int)points, 0, DAMAGE_ENERGY, MOD_BFG_EFFECT);
		}
	}

	self->nextthink = level.time + FRAMETIME;
	self->s.frame++;
	if (self->s.frame == 5)
		self->think = G_FreeEdict;
}

void bfg_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	// core explosion - prevents firing it into the wall/floor
	if (other->takedamage)
		T_Damage (other, self, self->owner, self->velocity, self->s.origin, plane->normal, sk_bfg_rdamage->value, 0, 0, MOD_BFG_BLAST);
	T_RadiusDamage(self, self->owner, sk_bfg_rdamage->value, other, 100, MOD_BFG_BLAST, -0.5);

	gi.sound (self, CHAN_VOICE, gi.soundindex ("weapons/bfg__x1b.wav"), 1, ATTN_NORM, 0);
	self->solid = SOLID_NOT;
	self->touch = NULL;
	VectorMA (self->s.origin, -1 * FRAMETIME, self->velocity, self->s.origin);
	VectorClear (self->velocity);
	self->s.modelindex = gi.modelindex ("sprites/s_bfg3.sp2");
	self->s.frame = 0;
	self->s.sound = 0;
	self->s.effects &= ~EF_ANIM_ALLFAST;
	self->think = bfg_explode;
	self->nextthink = level.time + FRAMETIME;
	self->enemy = other;

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BFG_BIGEXPLOSION);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	if (level.num_reflectors)
		ReflectExplosion(TE_BFG_BIGEXPLOSION,self->s.origin);
}


void bfg_think (edict_t *self)
{
	edict_t	*ent;
	edict_t	*ignore;
	vec3_t	point;
	vec3_t	dir;
	vec3_t	start;
	vec3_t	end;
	int		dmg;
	trace_t	tr;

	if (deathmatch->value)
		dmg = sk_bfg_damage2_dm->value;	//	5
	else
		dmg = sk_bfg_damage2->value;	//	10

	ent = NULL;
	while ((ent = findradius(ent, self->s.origin, 256)) != NULL)
	{
		if (ent == self)
			continue;

		if (ent == self->owner)
			continue;

		if (!ent->takedamage)
			continue;

		if (!(ent->svflags & SVF_MONSTER) && (!ent->client) && (strcmp(ent->classname, "misc_explobox") != 0))
			continue;

		VectorMA (ent->absmin, 0.5, ent->size, point);

		VectorSubtract (point, self->s.origin, dir);
		VectorNormalize (dir);

		ignore = self;
		VectorCopy (self->s.origin, start);
		VectorMA (start, 2048, dir, end);
		while(1)
		{
			tr = gi.trace (start, NULL, NULL, end, ignore, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);

			if (!tr.ent)
				break;

			// hurt it if we can
			if ((tr.ent->takedamage) && !(tr.ent->flags & FL_IMMUNE_LASER) && (tr.ent != self->owner))
				T_Damage (tr.ent, self, self->owner, dir, tr.endpos, vec3_origin, dmg, 1, DAMAGE_ENERGY, MOD_BFG_LASER);

			// if we hit something that's not a monster or player we're done
			if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client))
			{
				gi.WriteByte (svc_temp_entity);
				gi.WriteByte (TE_LASER_SPARKS);
				gi.WriteByte (4);
				gi.WritePosition (tr.endpos);
				gi.WriteDir (tr.plane.normal);
				gi.WriteByte (self->s.skinnum);
				gi.multicast (tr.endpos, MULTICAST_PVS);
				break;
			}

			ignore = tr.ent;
			VectorCopy (tr.endpos, start);
		}

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BFG_LASER);
		gi.WritePosition (self->s.origin);
		gi.WritePosition (tr.endpos);
		gi.multicast (self->s.origin, MULTICAST_PHS);

		if (level.num_reflectors)
			ReflectTrail(TE_BFG_LASER,self->s.origin,tr.endpos);
	}

	self->nextthink = level.time + FRAMETIME;
}


void fire_bfg (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius)
{
	edict_t	*bfg;

	bfg = G_Spawn();
	VectorCopy (start, bfg->s.origin);
	VectorCopy (dir, bfg->movedir);
	vectoangles (dir, bfg->s.angles);
	VectorScale (dir, speed, bfg->velocity);
	bfg->movetype = MOVETYPE_FLYMISSILE;
	bfg->clipmask = MASK_SHOT;
	bfg->solid = SOLID_BBOX;
	bfg->s.effects |= EF_BFG | EF_ANIM_ALLFAST;
	VectorClear (bfg->mins);
	VectorClear (bfg->maxs);
	bfg->s.modelindex = gi.modelindex ("sprites/s_bfg1.sp2");
	bfg->owner = self;
	bfg->touch = bfg_touch;
	bfg->nextthink = level.time + 8000/speed;
	bfg->think = G_FreeEdict;
	bfg->radius_dmg = damage;
	bfg->dmg_radius = damage_radius;
	bfg->classname = "bfg blast";
	bfg->s.sound = gi.soundindex ("weapons/bfg__l1a.wav");

	bfg->think = bfg_think;
	bfg->nextthink = level.time + FRAMETIME;
	bfg->teammaster = bfg;
	bfg->teamchain = NULL;

	if (self->client)
		check_dodge (self, bfg->s.origin, dir, speed);

	gi.linkentity (bfg);
}
//==========================================================================================
//
// AimGrenade finds the correct aim vector to get a grenade from start to target at initial
// velocity = speed. Returns false if grenade can't make it to target.
//
//==========================================================================================
qboolean AimGrenade (edict_t *self, vec3_t start, vec3_t target, vec_t speed, vec3_t aim)
{
	vec3_t		angles, forward, right, up;
	vec3_t		from_origin, from_muzzle;
	vec3_t		aim_point;
	vec_t		xo, yo;
	vec_t		x;
	float		cosa, t, vx, y;
	float		drop;
	float		last_error, v_error;
	int			i;
	vec3_t		last_aim;

	VectorCopy(target,aim_point);
	VectorSubtract(aim_point,self->s.origin,from_origin);
	VectorSubtract(aim_point, start, from_muzzle);

	if (self->svflags & SVF_MONSTER)
	{
		VectorCopy(from_muzzle,aim);
		VectorNormalize(aim);
		yo = from_muzzle[2];
		xo = sqrt(from_muzzle[0]*from_muzzle[0] + from_muzzle[1]*from_muzzle[1]);
	}
	else
	{
		VectorCopy(from_origin,aim);
		VectorNormalize(aim);
		yo = from_origin[2];
		xo = sqrt(from_origin[0]*from_origin[0] + from_origin[1]*from_origin[1]);
	}

	// If resulting aim vector is looking straight up or straight down, we're 
	// done. Actually now that I write this down and think about it... should
	// probably check straight up to make sure grenade will actually reach the
	// target.
	if ( (aim[2] == 1.0) || (aim[2] == -1.0))
		return true;

	// horizontal distance to target from muzzle
	x = sqrt( from_muzzle[0]*from_muzzle[0] + from_muzzle[1]*from_muzzle[1]);
	cosa = sqrt(aim[0]*aim[0] + aim[1]*aim[1]);
	// constant horizontal velocity (since grenades don't have drag)
	vx = speed * cosa;
	// time to reach target x
	t = x/vx;
	// if flight time is less than one frame, no way grenade will drop much,
	// shoot the sucker now.
	if (t < FRAMETIME)
		return true;
	// in that time, grenade will drop this much:
	drop = 0.5*sv_gravity->value*t*t;
	y = speed*aim[2]*t - drop;
	v_error = target[2] - start[2] - y;

	// if we're fairly close and we'll hit target at current angle,
	// no need for all this, just shoot it
	if ( (x < 128) && (fabs(v_error) < 16) )
		return true;

	last_error = 100000.;
	VectorCopy(aim,last_aim);

	// Unfortunately there is no closed-form solution for this problem,
	// so we creep up on an answer and balk if it takes more than 
	// 10 iterations to converge to the tolerance we'll accept.
	for (i=0; i<10 && fabs(v_error) > 4 && fabs(v_error) < fabs(last_error); i++)
	{
		last_error = v_error;
		aim[2] = cosa * (yo + drop)/xo;
		VectorNormalize(aim);
		if (!(self->svflags & SVF_MONSTER))
		{
			vectoangles(aim,angles);
			AngleVectors(angles, forward, right, up);
			G_ProjectSource2(self->s.origin,self->move_origin,forward,right,up,start);
			VectorSubtract(aim_point,start,from_muzzle);
			x = sqrt(from_muzzle[0]*from_muzzle[0] + from_muzzle[1]*from_muzzle[1]);
		}
		cosa = sqrt(aim[0]*aim[0] + aim[1]*aim[1]);
		vx = speed * cosa;
		t = x/vx;
		drop = 0.5*sv_gravity->value*t*t;
		y = speed*aim[2]*t - drop;
		v_error = target[2] - start[2] - y;
		if (fabs(v_error) < fabs(last_error))
			VectorCopy(aim,last_aim);
	}
	
	if (i >= 10 || v_error > 64)
		return false;
	if (fabs(v_error) > fabs(last_error))
	{
		VectorCopy(last_aim,aim);
		if (!(self->svflags & SVF_MONSTER))
		{
			vectoangles(aim,angles);
			AngleVectors(angles, forward, right, up);
			G_ProjectSource2(self->s.origin,self->move_origin,forward,right,up,start);
			VectorSubtract(aim_point,start,from_muzzle);
		}
	}
	
	// Sanity check... if launcher is at the same elevation or a bit above the 
	// target entity, check to make sure he won't bounce grenades off the 
	// top of a doorway or other obstruction. If he WOULD do that, then figure out 
	// the max elevation angle that will get the grenade through the door, and 
	// hope we get a good bounce.
	if ( (start[2] - target[2] < 160) &&
		(start[2] - target[2] > -16)   )
	{
		trace_t	tr;
		vec3_t	dist;
		
		tr = gi.trace(start,vec3_origin,vec3_origin,aim_point,self,MASK_SOLID | CONTENTS_GRILL);	//CW: can't pass through a grill
		if ( (tr.fraction < 1.0) && (!self->enemy || (tr.ent != self->enemy) )) {
			// OK... the aim vector hit a solid, but would the grenade actually hit?
			int		contents;
			cosa = sqrt(aim[0]*aim[0] + aim[1]*aim[1]);
			vx = speed * cosa;
			VectorSubtract(tr.endpos,start,dist);
			dist[2] = 0;
			x = VectorLength(dist);
			t = x/vx;
			drop = 0.5*sv_gravity->value*t*(t+FRAMETIME);
			tr.endpos[2] -= drop;
			// move just a bit in the aim direction
			tr.endpos[0] += aim[0];
			tr.endpos[1] += aim[1];
			contents = gi.pointcontents(tr.endpos);
			while((contents & MASK_SOLID) && (aim_point[2] > target[2])) {
				aim_point[2] -= 8.0;
				VectorSubtract(aim_point,self->s.origin,from_origin);
				VectorCopy(from_origin,aim);
				VectorNormalize(aim);
				if (!(self->svflags & SVF_MONSTER))
				{
					vectoangles(aim,angles);
					AngleVectors(angles, forward, right, up);
					G_ProjectSource2(self->s.origin,self->move_origin,forward,right,up,start);
					VectorSubtract(aim_point,start,from_muzzle);
				}
				tr = gi.trace(start,vec3_origin,vec3_origin,aim_point,self,MASK_SOLID);
				if (tr.fraction < 1.0) {
					cosa = sqrt(aim[0]*aim[0] + aim[1]*aim[1]);
					vx = speed * cosa;
					VectorSubtract(tr.endpos,start,dist);
					dist[2] = 0;
					x = VectorLength(dist);
					t = x/vx;
					drop = 0.5*sv_gravity->value*t*(t+FRAMETIME);
					tr.endpos[2] -= drop;
					tr.endpos[0] += aim[0];
					tr.endpos[1] += aim[1];
					contents = gi.pointcontents(tr.endpos);
				}
			}
		}
	}
	return true;
}

