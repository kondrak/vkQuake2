#include "g_local.h"

#define RFAST   -3
#define RMEDIUM -2
#define RSLOW   -1
#define STOP     0
#define SLOW     1
#define MEDIUM   2
#define FAST     3

#define VEHICLE_BLOCK_STOPS 4

void func_vehicle_explode (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	vec3_t	origin;
	vec3_t	chunkorigin;
	vec3_t	size;
	int		count;
	int		mass;

	if(self->deathtarget)
	{
		self->target = self->deathtarget;
		G_UseTargets (self, attacker);
	}

	// bmodel origins are (0 0 0), we need to adjust that here
	VectorScale (self->size, 0.5, size);
	VectorAdd (self->absmin, size, origin);
	VectorCopy (origin, self->s.origin);

	self->takedamage = DAMAGE_NO;

	if (self->dmg)
		T_RadiusDamage (self, attacker, self->dmg, NULL, self->dmg+40, MOD_EXPLOSIVE, -0.5);

	VectorSubtract (self->s.origin, inflictor->s.origin, self->velocity);
	VectorNormalize (self->velocity);
	VectorScale (self->velocity, 150, self->velocity);

	// start chunks towards the center
	VectorScale (size, 0.5, size);

	mass = self->mass;

	// big chunks
	if (mass >= 100)
	{
		count = mass / 100;
		if (count > 8)
			count = 8;
		while(count--)
		{
			chunkorigin[0] = origin[0] + crandom() * size[0];
			chunkorigin[1] = origin[1] + crandom() * size[1];
			chunkorigin[2] = origin[2] + crandom() * size[2];
			ThrowDebris (self, "models/objects/debris1/tris.md2", 1, chunkorigin, 0, 0);
		}
	}

	// small chunks
	count = mass / 25;
	if (count > 16)
		count = 16;
	while(count--)
	{
		chunkorigin[0] = origin[0] + crandom() * size[0];
		chunkorigin[1] = origin[1] + crandom() * size[1];
		chunkorigin[2] = origin[2] + crandom() * size[2];
		ThrowDebris (self, "models/objects/debris2/tris.md2", 2, chunkorigin, 0, 0);
	}

	if (self->dmg)
		BecomeExplosion1 (self);
	else
		G_FreeEdict (self);
}

void vehicle_blocked (edict_t *self, edict_t *other)
{
	edict_t	*attacker;

	if((self->spawnflags & VEHICLE_BLOCK_STOPS) || (other == world))
	{
		VectorClear(self->velocity);
		VectorClear(self->avelocity);
		self->moveinfo.current_speed = 0;
		gi.linkentity(self);
		return;
	}
	if (other->takedamage)
	{
		if (self->teammaster->owner)
			attacker = self->teammaster->owner;
		else
			attacker = self->owner;
		T_Damage (other, self, attacker, vec3_origin, other->s.origin, vec3_origin, self->teammaster->dmg, 10, 0, MOD_CRUSH);
	}
	else
	{
		VectorClear(self->velocity);
		VectorClear(self->avelocity);
		self->moveinfo.current_speed = 0;
		self->moveinfo.state = STOP;
		gi.linkentity(self);
	}

	if (!(other->svflags & SVF_MONSTER) && (!other->client))
	{
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, 0, MOD_CRUSH);

		if (other)
			BecomeExplosion1 (other);
		return;
	}
}

// Not needed, because collisions are effectively prevented by the vehicle physics in
// g_phys.c.
void vehicle_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t	dir, v;
	vec3_t	new_origin, new_velocity;
	vec_t	points;
	vec_t	vspeed, mspeed;
	vec_t	knockback;
	vec3_t	end;
	trace_t	tr;

	if (other == world || (self->spawnflags & VEHICLE_BLOCK_STOPS) )
	{
		VectorClear(self->velocity);
		VectorClear(self->avelocity);
		self->moveinfo.current_speed = 0;
		gi.linkentity(self);
	}
	if (!self->owner) return;         // if vehicle isn't being driven, it can't hurt anybody
	if (other == self->owner) return; // can't hurt the driver
	if (other->takedamage == DAMAGE_NO) return;
	// we damage func_explosives elsewhere. About all that's left to hurt are
	// players and monsters
	if (!other->client && !(other->svflags & SVF_MONSTER)) return;
	vspeed = VectorLength(self->velocity);
	if(!vspeed) return;
	VectorSubtract(other->s.origin,self->s.origin,dir);
	dir[2] = 0;
	VectorNormalize(dir);
	VectorCopy(self->velocity,v);
	VectorNormalize(v);
	// damage and knockback are proportional to square of velocity * mass of vehicle.
	// Lessee... with a mass=2000 vehicle traveling 400 units/sec, give 100 points
	// damage and a velocity of 160 to a 200 mass monster.
	vspeed *= DotProduct(dir,v);
	mspeed = VectorLength(other->velocity) * DotProduct(dir,v);
	vspeed -= mspeed;
	if(vspeed <= 0.) return;
	// for speed < 200, don't do damage but move monster
	if(vspeed < 200) {
		if(other->mass > self->mass) vspeed *= self->mass/other->mass;
		VectorMA(other->velocity,vspeed,dir,new_velocity);
		VectorMA(other->s.origin,FRAMETIME,new_velocity,new_origin);
		new_origin[2] += 2;
		// if the move would place the monster in a solid, make him go splat
		VectorCopy(new_origin,end);
		end[2] -= 1;
		tr = gi.trace(new_origin,other->mins,other->maxs,end,self,CONTENTS_SOLID);
		if(tr.startsolid)
			// splat
			T_Damage (other, self, self->owner, dir, self->s.origin, vec3_origin,
						other->health - other->gib_health + 1, 0, 0, MOD_VEHICLE);
			
		else
		{
			// go ahead and move the bastard
			VectorCopy(new_velocity,other->velocity);
			VectorCopy(new_origin,other->s.origin);
			gi.linkentity(other);
		}
		return;
	}
	if (other->damage_debounce_time > level.time) return;
	other->damage_debounce_time = level.time + 0.2;
	points = 100. * (self->mass/2000 * vspeed*vspeed/160000);
	// knockback takes too long to take effect. If we can move him w/o throwing him
	// into a solid, do so NOW
	dir[2] = 0.2; // make knockback slightly upward
	VectorMA(other->velocity,vspeed,dir,new_velocity);
	VectorMA(other->s.origin,FRAMETIME,new_velocity,new_origin);
	if(gi.pointcontents(new_origin) & CONTENTS_SOLID)
		knockback = (160./500.) * 200. * (self->mass/2000 * vspeed*vspeed/160000);
	else {
		knockback = 0;
		VectorCopy(new_velocity,other->velocity);
		VectorCopy(new_origin,  other->s.origin);
	}
	T_Damage (other, self, self->owner, dir, self->s.origin, vec3_origin,
		(int)points, (int)knockback, 0, MOD_VEHICLE);
	gi.linkentity(other);
}


void vehicle_disengage (edict_t *vehicle)
{
	edict_t *driver;
	vec3_t	forward, left, f1, l1;

	driver = vehicle->owner;
	if(!driver) return;

	AngleVectors(vehicle->s.angles, forward, left, NULL);
	VectorCopy(vehicle->velocity,driver->velocity);
	VectorScale(forward,vehicle->move_origin[0],f1);
	VectorScale(left,-vehicle->move_origin[1],l1);
	VectorAdd(vehicle->s.origin,f1,driver->s.origin);
	VectorAdd(driver->s.origin,l1,driver->s.origin);
	driver->s.origin[2] += vehicle->move_origin[2];
	
	driver->vehicle = NULL;
	driver->client->vehicle_framenum = level.framenum;
	driver->movetype = MOVETYPE_WALK;
	driver->gravity = 1;
	// turn ON client side prediction for this player
	driver->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
	vehicle->s.sound = 0;
	gi.linkentity (driver);
	vehicle->owner = NULL;
}

void vehicle_think (edict_t *self)
{
	float  aspeed, speed, newspeed;
	vec3_t forward, left, f1, l1, v;

	self->nextthink = level.time + FRAMETIME;

	VectorCopy(self->oldvelocity,v);
	v[2] = 0;
	speed = VectorLength(v);
	if(speed > 0)
		self->s.effects |= EF_ANIM_ALL;
	else
		self->s.effects &= ~EF_ANIM_ALL;
	AngleVectors(self->s.angles, forward, left, NULL);
	if(DotProduct(forward,self->oldvelocity) < 0) speed = -speed;
	self->moveinfo.current_speed = speed;

	if (self->owner)
	{
		// ... then we have a driver
		if (self->owner->health <= 0)
		{
			vehicle_disengage(self);
			return;
		}
		if (self->owner->client->use)
		{
			// if he's pressing the use key, and he didn't just
			// get on or off, disengage
			if(level.framenum - self->owner->client->vehicle_framenum > 2)
			{
				VectorCopy(self->velocity,self->oldvelocity);
				vehicle_disengage(self);
				return;
			}
		}
		if (self->owner->client->ucmd.forwardmove != 0 && level.time > self->moveinfo.wait)
		{
			if(self->owner->client->ucmd.forwardmove > 0)
			{
				if(self->moveinfo.state < FAST)
				{
					self->moveinfo.state++;
					self->moveinfo.next_speed = self->moveinfo.state * self->speed/3;
					self->moveinfo.wait = level.time + FRAMETIME;
				}
			}
			else
			{
				if(self->moveinfo.state > RFAST)
				{
					self->moveinfo.state--;
					self->moveinfo.next_speed = self->moveinfo.state * self->speed/3;
					self->moveinfo.wait = level.time + FRAMETIME;
				}
			}
		}
		if(self->moveinfo.current_speed < self->moveinfo.next_speed)
		{
			speed = self->moveinfo.current_speed + self->accel/10;
			if(speed > self->moveinfo.next_speed) speed = self->moveinfo.next_speed;
		}
		else if(self->moveinfo.current_speed > self->moveinfo.next_speed)
		{
			speed = self->moveinfo.current_speed - self->decel/10;
			if(speed < self->moveinfo.next_speed) speed = self->moveinfo.next_speed;
		}
		VectorScale(forward,speed,self->velocity);
		if (self->owner->client->ucmd.sidemove != 0 && speed != 0 )
		{
			aspeed = 180.*speed/(M_PI*self->radius);
			if(self->owner->client->ucmd.sidemove > 0) aspeed = -aspeed;
			self->avelocity[1] = aspeed;
		}
		else
			self->avelocity[1] = 0;

		if(speed != 0)
			self->s.sound = self->noise_index;
		else
			self->s.sound = self->noise_index2;
#ifdef LOOP_SOUND_ATTENUATION
		self->s.attenuation = self->attenuation;
#endif

		gi.linkentity(self);

		// Copy velocities and set position of driver
		VectorCopy(self->velocity,self->owner->velocity);
		VectorScale(forward,self->move_origin[0],f1);
		VectorScale(left,-self->move_origin[1],l1);
		VectorAdd(self->s.origin,f1,self->owner->s.origin);
		VectorAdd(self->owner->s.origin,l1,self->owner->s.origin);
		self->owner->s.origin[2] += self->move_origin[2];
		// If moving, turn driver
		if(speed != 0)
		{
			float	yaw;
			yaw = self->avelocity[1]*FRAMETIME;
			self->owner->s.angles[YAW] += yaw;
			self->owner->client->ps.pmove.delta_angles[YAW] += ANGLE2SHORT(yaw);
			self->owner->client->ps.pmove.pm_type = PM_FREEZE;
		}
		VectorCopy(self->velocity,self->oldvelocity);
		gi.linkentity(self->owner);
	}
	else
	{
		int		i;
		edict_t	*ent;
		vec3_t	dir, drive;
		//
		// No driver
		//
		// if vehicle has stopped, drop it to ground.
		// otherwise slow it down
		if(speed==0)
		{
			if(!self->groundentity)
				SV_AddGravity (self);
		}
		else
		{
			// no driver... slow to an eventual stop in no more than 5 sec.
			self->moveinfo.next_speed = 0;
			self->moveinfo.state = STOP;
			if(speed > 0)
				newspeed = max(0.,speed - self->speed/50);
			else
				newspeed = min(0.,speed + self->speed/50);
			VectorScale(forward,newspeed,self->velocity);
			VectorScale(self->avelocity,newspeed/speed,self->avelocity);
			VectorCopy(self->velocity,self->oldvelocity);
			gi.linkentity(self);
		}
		
		// check if a player has mounted the vehicle
		// first get driving position
		VectorScale(forward,self->move_origin[0],f1);
		VectorScale(left,-self->move_origin[1],l1);
		VectorAdd(self->s.origin,f1,drive);
		VectorAdd(drive,l1,drive);
		drive[2] += self->move_origin[2];
		// find a player
		for (i=1, ent=&g_edicts[1] ; i<=maxclients->value ; i++, ent++) {
			if (!ent->inuse) continue;
			if (ent->movetype == MOVETYPE_NOCLIP) continue;
			if (!ent->client->use) continue;
			if (level.framenum - ent->client->vehicle_framenum <= 2) continue;

			// determine distance from vehicle "move_origin"
			VectorSubtract(drive,ent->s.origin,dir);
			if (fabs(dir[2]) < 64)
				dir[2] = 0;

			if (VectorLength(dir) < 16) {
				ent->client->vehicle_framenum = level.framenum;
				// player has taken control of vehicle
				// move vehicle up slightly to avoid roundoff collisions
				self->s.origin[2] += 1;
				gi.linkentity(self);

				if(self->message)
					gi.centerprintf(ent,self->message);
				self->owner = ent;
				ent->movetype = MOVETYPE_PUSH;
				ent->gravity = 0;
				ent->vehicle = self;
				// turn off client side prediction for this player
				ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
				// force a good driving position
				VectorCopy(drive,self->owner->s.origin);
				gi.linkentity(ent);
				// vehicle idle noise
				self->s.sound  = self->noise_index2;
		#ifdef LOOP_SOUND_ATTENUATION
				self->s.attenuation = self->attenuation;
		#endif
				// reset wait time so we can start accelerating
				self->moveinfo.wait = 0;
			}
		}
	}
	if(self->movewith_next && (self->movewith_next->movewith_ent == self))
		set_child_movement(self);
}

void turn_vehicle (edict_t *self)
{
	self->s.angles[YAW] = self->ideal_yaw;
	gi.linkentity(self);
	self->prethink = NULL;
}
void SP_func_vehicle (edict_t *self)
{
	self->ideal_yaw = self->s.angles[YAW];
	VectorClear (self->s.angles);
	self->solid = SOLID_BSP;
	gi.setmodel (self, self->model);

	self->movetype   = MOVETYPE_VEHICLE;

	if (!self->speed)
		self->speed  = 200;
	if (!self->accel)
		self->accel = self->speed; // accelerates to full speed in 1 second (approximate).
	if (!self->decel)
		self->decel = self->accel;
	if (!self->mass)
		self->mass   = 2000;
	if (!self->radius)
		self->radius = 256;
	self->blocked    = vehicle_blocked;
	self->touch      = vehicle_touch;
	self->think      = vehicle_think;
	self->nextthink  = level.time + FRAMETIME;
	self->noise_index  = gi.soundindex("engine/engine.wav");
	self->noise_index2 = gi.soundindex("engine/idle.wav");

#ifdef LOOP_SOUND_ATTENUATION
	if (self->attenuation <= 0)
		self->attenuation = ATTN_IDLE;
#endif

	VectorClear(self->velocity);
	VectorClear(self->avelocity);
	self->moveinfo.current_speed = 0;
	self->moveinfo.state = STOP;
	gi.linkentity (self);
	VectorCopy(self->size,self->org_size);

	if (self->ideal_yaw != 0)
		self->prethink = turn_vehicle;
	if (self->health) {
		self->die = func_vehicle_explode;
		self->takedamage = DAMAGE_YES;
	} else
		self->takedamage = DAMAGE_NO;
}
