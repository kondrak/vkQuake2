#include "g_local.h"

//====================================================================

/* FUNC_PENDULUM

SF 1 = START_ON
   2 = STOP_AT_TOP
   8 = SLOW  (internal use only, used to slow a pendulum to a stop after
              being blocked)
   16= STOPPING (internal use only, set when a STOP_AT_TOP pendulum is triggered off)

radius       = length of pendulum, used for motion equation (not impact)
distance     = total arc that pendulum moves through, must be < 360
move_origin  = vector from origin to c.g. of pendulum, used for impact
mass         = mass of pendulum, used for knockback of func_pushables (not players/monsters)
*/

#define SF_PENDULUM_STARTON      1
#define SF_PENDULUM_STOP_AT_TOP  2
#define SF_PENDULUM_SLOW         8
#define SF_PENDULUM_STOPPING    16

void box_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);
void pendulum_blocked (edict_t *self, edict_t *other)
{
	trace_t	trace;
	vec3_t	angles;
	vec3_t	forward, left, up;
	vec3_t	dir;
	vec3_t	f1, l1, u1;
	vec3_t	point;
	vec3_t	origin;
	vec3_t	new_velocity;
	vec3_t	new_origin;
	float	speed;
	int		damage;

	// Since this routine is called in response to being blocked, 
	// the current s.angles is for the LAST frame, not the proposed
	// angles for the current frame. Since we're basically overriding
	// normal physics here, go ahead and move to new location. This
	// means that later on we have to move blocker NOW rather than
	// relying on its velocity to get it out of the way.
	// BUT... trouble is doing this with players ends up giving
	// goofy direction in some cases. For players/monsters, use old
	// angles but STILL move 'em out of the way immediately
	if(other->client || (other->svflags & SVF_MONSTER))
		VectorCopy(self->s.angles,angles);
	else
		VectorMA(self->s.angles,FRAMETIME,self->avelocity,angles);

	AngleVectors(angles,forward,left,up);

	speed = fabs(self->avelocity[ROLL]) * M_PI / 180. * self->radius;
	if( (level.time > self->touch_debounce_time) && (speed > 200) )
	{
		damage = (int)( self->dmg * (speed-200)/100 );
		self->touch_debounce_time = level.time + 0.5;
	}
	else
		damage = 0;

	VectorAdd(other->s.origin,other->origin_offset,origin);
	VectorCopy(left,dir);
	if(self->avelocity[ROLL] > 0)
		VectorNegate(dir,dir);

	VectorScale(forward,self->move_origin[0],f1);
	VectorScale(left,-self->move_origin[1],l1);
	VectorScale(up,self->move_origin[2],u1);
	VectorAdd(self->s.origin,f1,point);
	VectorAdd(point,l1,point);
	VectorAdd(point,u1,point);
	VectorSubtract(origin,point,point);
	VectorNormalize(point);

	if(other->client || (other->svflags & SVF_MONSTER))
	{
		if((point[2] < -0.7) && other->groundentity)
		{
			T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, DAMAGE_NO_PROTECTION, MOD_CRUSH);
			return;
		}

		dir[2] = max(1.0,fabs(dir[2]));
		VectorNormalize(dir);
		// Normal kickback takes too long to take effect and allows embedment. Move
		// the blocker NOW.
		// Give a minimum speed so we can get the poor fool out of the way
		speed = max(100,speed);
		VectorScale(dir,speed,new_velocity);
		VectorMA(other->s.origin,FRAMETIME,new_velocity,new_origin);
		other->solid = SOLID_NOT;
		gi.linkentity(other);
		trace = gi.trace(other->s.origin,other->mins,other->maxs,new_origin,self,other->clipmask);
		VectorCopy(trace.endpos,other->s.origin);
		VectorCopy(new_velocity,other->velocity);
		other->solid = SOLID_BBOX;
		gi.linkentity(other);
		T_Damage (other, self, self, dir, other->s.origin, vec3_origin, damage, 0, 0, MOD_CRUSH);
	}
	else if(other->solid == SOLID_BSP)
	{
		// Other is most likely a func_pushable, since almost all other bmodels aren't
		// clipped to MOVETYPE_PUSH

		vec3_t		org, mins, maxs;
		vec3_t		vn2;
		qboolean	block;
		float		e=self->attenuation;	// coefficient of restitution
		float		m=(float)(other->mass)/(float)(self->mass);
		float		v11 = speed;
		float		v21;		// Initial speed of other in the impact direction
		float		v12, v22;
		float		new_rspeed;
		float		sgor, time, wave;

		if(v11 >= 100)
			gi.sound (self, 0, self->noise_index, 1, 1, 0);

		// If other is on the ground, push it UP regardless of dir
		if(other->groundentity)
			dir[2] = max(1.0,fabs(dir[2]));
		VectorNormalize(dir);

		// If pendulum hits crate from above and crate is on the ground,
		// destroy the crate. This may not be realistic, but there's really
		// no other way since if we stop the pendulum we'd then have to 
		// continously monitor whether the crate moved away or not.
		if((point[2] < -0.7) && (other->velocity[2] == 0))
		{
			box_die (other, self, self, 100000, point);
			return;
		}
		if(e > 0)
		{
			v21 = VectorLength(other->velocity);
			if(v21 > 0)
			{
				VectorCopy(other->velocity,vn2);
				VectorNormalize(vn2);
				v21 *= DotProduct(dir,vn2);
			}

			v22 = ( e*(v11-v21) + v11 + m*v21 ) / (1.0 + m);
			v12 = v22 + e*(v21-v11);
//			gi.dprintf("v11=%g, v21=%g, v12=%g, v22=%g\n",v11,v21,v12,v22);
//			gi.dprintf("av[ROLL]=%g, roll=%g\n",self->avelocity[ROLL],angles[ROLL]);
		}
		else
		{
			v12 = v11;
			if(other->mass > self->mass)
			{
				block = true;
				VectorClear(self->avelocity);
				gi.linkentity(self);
				goto deadstop;
			}
			else
				v22 = v11 * (float)self->mass/(float)other->mass;
		}
		VectorScale(dir,v22,new_velocity);

		if(v12 < 0)
		{
			// Reverse rotation.
			new_rspeed = fabs(v12) / (M_PI / 180. * self->radius);
			if(self->avelocity[ROLL] > 0)
				self->avelocity[ROLL] = -new_rspeed;
			else
				self->avelocity[ROLL] = new_rspeed;
		}
		else
		{
			// Continuing to move in same direction, though slower.
			new_rspeed = v12 / (M_PI / 180. * self->radius);
			if(self->avelocity[ROLL] > 0)
				self->avelocity[ROLL] = new_rspeed;
			else
				self->avelocity[ROLL] = -new_rspeed;
		}
		sgor = sqrt( (float)sv_gravity->value / self->radius );
		wave = fabs( self->avelocity[ROLL] / (angles[ROLL] * sgor) );
		wave = atan(wave);
		if(self->avelocity[ROLL] >= 0)
		{
			if(angles[ROLL] > 0)
				wave = M_PI - wave;
		}
		else
		{
			if(angles[ROLL] > 0)
				wave = M_PI + wave;
			else
				wave = 2*M_PI - wave;
		}
		time = wave/sgor;
		self->startframe = level.framenum - time*10.;
		self->moveinfo.start_angles[ROLL] = -fabs(angles[ROLL] / cos(wave));

		// Now we know the new pendulum velocity and crate velocity, *assuming*
		// nothing else is in the way. Now check to see if crate hits anything
		// else.

		VectorAdd(other->s.origin,other->origin_offset,org);
		VectorMA(org,FRAMETIME,new_velocity,new_origin);

		// Temporarily make crate nonsolid so we can ignore pendulum in our trace
		// (rather than crate)
		other->solid = SOLID_NOT;
		gi.linkentity(other);
		VectorSubtract(other->mins,other->origin_offset,mins);
		VectorSubtract(other->maxs,other->origin_offset,maxs);
		trace = gi.trace (org, mins, maxs, new_origin, self, other->clipmask);
		// restore solidity of crate
		other->solid = SOLID_BSP;
		if(trace.startsolid)
		{
			// Things are completely fouled up. Nuke other and go away.
			T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, 0, MOD_CRUSH);
			if (other)
				BecomeExplosion1 (other);
			return;
		}
		else if(trace.fraction < 1.0)
		{
			vec3_t	vec;
			float	dist;
			VectorSubtract(trace.endpos,org,vec);
			dist = VectorLength(vec);

			if( (trace.ent->client) || (trace.ent->flags & SVF_MONSTER))
			{
				float	delta=FRAMETIME*VectorLength(new_velocity);

				// If a player or monster is in the way of the crate, AND
				// the pendulum speed is > 100, throw 'em out of the way.
				// If pendulum tangential speed is < 100, give up.
				if(speed < 100)
					block = true;
				else
				{
					if(dist < delta)
					{
						VectorScale(new_velocity,1.25,trace.ent->velocity);
						VectorMA(trace.ent->s.origin,FRAMETIME,trace.ent->velocity,trace.ent->s.origin);
						gi.linkentity(trace.ent);
					}
					block = false;
				}
			}
			else
			{
				if(dist < speed*FRAMETIME)
				{
					block = true;
					VectorScale(vec,10.,other->velocity);
					VectorMA(other->s.origin,FRAMETIME,other->velocity,other->s.origin);
				}
				else
					block = false;
			}
		}
		else
			block = false;

		if(!block)
		{
			VectorCopy(new_velocity,other->velocity);
			VectorMA(other->s.origin,FRAMETIME,other->velocity,other->s.origin);
		}
		gi.linkentity(other);

		// Final checks:
		// 1) If pendulum after-impact speed is < 100, that's too damn slow.
		//    Lie and say it's blocked
		// 2) If not blocked, in its new position test for intersection
		// of crate and pendulum. If they intersect, then most likely pendulum is
		// moving VERY slowly and we need to reverse direction NOW to prevent
		// embedment.
		if(!block)
		{
			if(fabs(v12) < 100)
				block = true;
			else
			{
				VectorAdd(other->s.origin,other->origin_offset,org);
				trace = gi.trace(org,mins,maxs,org,other,MASK_SOLID);
				if(trace.startsolid)
					block = true;
			}
		}
deadstop:
		T_Damage (other, self, self, dir, other->s.origin, vec3_origin, damage, 0, 0, MOD_CRUSH);
		if( block )
		{
			// Then this sucker will still be in the way. Reverse rotation, slow, or stop
			if(fabs(angles[ROLL]) > 2)
			{
				if(abs(angles[ROLL]) < 10)
					self->spawnflags |= SF_PENDULUM_SLOW;
				self->moveinfo.start_angles[ROLL] = angles[ROLL];
				VectorClear(self->avelocity);
				self->startframe = 0;
			}
			else
			{
				self->spawnflags &= ~SF_PENDULUM_STARTON;
				self->moveinfo.start_angles[ROLL] = 0;
				VectorClear(self->s.angles);
				VectorClear(self->avelocity);
			}
			gi.linkentity(self);
		}
		else if((fabs(self->avelocity[ROLL]) < 10) && (fabs(self->s.angles[ROLL]) < 10))
		{
			self->spawnflags |= SF_PENDULUM_SLOW;
			self->moveinfo.start_angles[ROLL] = angles[ROLL];
			VectorClear(self->avelocity);
			self->startframe = 0;
		}
	}
	else
	{
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, 0, MOD_CRUSH);
		// if it's still there, nuke it
		if (other)
			BecomeExplosion1 (other);
	}
}

void pendulum_rotate (edict_t *self)
{
	float	this_angle;
	float	wave;
	float	sgor;

	if(!(self->spawnflags & SF_PENDULUM_STARTON))
		return;

	if(self->spawnflags & SF_PENDULUM_SLOW)
	{
		if(self->startframe == 0)
		{
			// Then we just started moving again after being blocked
			self->avelocity[ROLL] = -self->s.angles[ROLL];
			self->startframe = level.framenum;
		}
		else
		{
			float	next_angle;

			next_angle = self->s.angles[ROLL] + self->avelocity[ROLL]*FRAMETIME;
			if( (next_angle >= 0 && self->s.angles[ROLL] < 0) ||
				(next_angle <= 0 && self->s.angles[ROLL] > 0)   )
			{
				VectorClear(self->s.angles);
				VectorClear(self->avelocity);
				gi.linkentity(self);
				return;
			}
		}
		self->nextthink = level.time + FRAMETIME;
	}
	else
	{
		float	old_velocity = self->avelocity[ROLL];

		if(!self->startframe)
			self->startframe = level.framenum;

		sgor = sqrt( (float)sv_gravity->value / self->radius );
		wave = sgor * (level.framenum - self->startframe) * 0.1;
		this_angle = self->moveinfo.start_angles[ROLL] * cos(wave);
		self->avelocity[ROLL] = -self->moveinfo.start_angles[ROLL] * sgor * sin(wave);
		if( (self->spawnflags & SF_PENDULUM_STOPPING) && (cos(wave) > 0.0))
		{
			if( ((old_velocity > 0) && (self->avelocity[ROLL] <= 0)) ||
				((old_velocity < 0) && (self->avelocity[ROLL] >= 0))    )
			{
				self->spawnflags &= ~SF_PENDULUM_STARTON;
				VectorClear(self->avelocity);
				self->nextthink = 0;
				gi.linkentity(self);
				return;
			}
		}
		self->s.angles[ROLL] = this_angle;
		self->nextthink = level.time + FRAMETIME;
	}
	gi.linkentity(self);
}

void pendulum_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if(self->spawnflags & SF_PENDULUM_STARTON)
	{
		if(self->spawnflags & SF_PENDULUM_STOP_AT_TOP)
		{
			self->spawnflags |= SF_PENDULUM_STOPPING;
		}
		else
		{
			VectorClear(self->avelocity);
			self->spawnflags &= ~SF_PENDULUM_STARTON;
			gi.linkentity(self);
		}
	}
	else
	{
		self->spawnflags |= SF_PENDULUM_STARTON;
		self->spawnflags &= ~SF_PENDULUM_STOPPING;
		self->think = pendulum_rotate;

		if(self->delay > 0)
		{
			float	delay;
			delay = self->delay * 2.0 * M_PI * sqrt(self->radius/(float)sv_gravity->value);
			delay = 0.1 * (int)(10*delay);
			self->nextthink  = level.time + delay;
			self->startframe = level.framenum + delay*10;
			if(!(self->spawnflags & SF_PENDULUM_STOP_AT_TOP))
				self->delay = 0;
		}
		else
		{
			if(self->s.angles[ROLL] == self->moveinfo.start_angles[ROLL])
				self->startframe = level.framenum;
			else
			{
				float	t;
				t = acos (self->s.angles[ROLL] / self->moveinfo.start_angles[ROLL]);
				t /= sqrt((float)sv_gravity->value / self->radius);
				self->startframe = level.framenum - t*10;
			}
			self->think(self);
		}
	}
}

void pendulum_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	// Mostly copied from func_explosive_explode. We can't use that function because
	// the origin is a bit different.

	vec3_t	origin;
	vec3_t	forward, left, up;
	vec3_t	chunkorigin;
	vec3_t	size;
	int		count;
	int		mass;

	// Particles originate from business end of pendulum
	AngleVectors(self->s.angles,forward,left,up);
	VectorScale(forward,self->move_origin[0],forward);
	VectorScale(left,-self->move_origin[1],left);
	VectorScale(up,self->move_origin[2],up);
	VectorAdd(self->s.origin,forward,origin);
	VectorAdd(origin,left,origin);
	VectorAdd(origin,up,origin);

	self->mass *= 2;
	self->takedamage = DAMAGE_NO;

	VectorSubtract (origin, self->enemy->s.origin, self->velocity);
	VectorNormalize (self->velocity);
	VectorScale (self->velocity, 150, self->velocity);

	// start chunks towards the center
	VectorScale (size, 0.5, size);

	mass = self->mass;
	if (!mass)
		mass = 75;

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
	G_FreeEdict (self);
}

void SP_func_pendulum (edict_t *ent)
{
	float	max_speed;

	ent->solid = SOLID_BSP;
	ent->movetype = MOVETYPE_PENDULUM;
	if(!st.distance)
		ent->moveinfo.distance = 90;
	else
		ent->moveinfo.distance = st.distance;

	if(st.noise)
		ent->noise_index = gi.soundindex(st.noise);
	else
		ent->noise_index = gi.soundindex("world/land.wav");

	if(ent->moveinfo.distance >= 360)
	{
		gi.dprintf("func_pendulum distance must be < 360\n");
		ent->moveinfo.distance = 359.;
	}

	if (!ent->speed)
		ent->speed = 100;
	if (!ent->radius)
		ent->radius = 100;
	if (!ent->mass)
		ent->mass = 200;
	if (st.phase > 0)
		ent->delay = st.phase;
	else
		ent->delay = 0;
	if(ent->delay > 1.0)
		ent->delay -= (int)(ent->delay);

	// Coefficient of restitution - default = 0.5, must be <= 1.0
	if(ent->attenuation == 0.0)
		ent->attenuation = 0.5;
	else if(ent->attenuation > 1.0)
		ent->attenuation = 1.0;

	if (!ent->dmg)
		ent->dmg = 5;
	// This is the damage delivered by the pendulum at max speed. Convert to
	// a damage scale used in our damage equation.
	max_speed = ent->moveinfo.distance/2 * M_PI / 180. * sqrt((float)sv_gravity->value * ent->radius );
	if(max_speed <= 200.)
		ent->dmg = 0;
	else
	{
		float	dmg;
		dmg = (float)(ent->dmg) * 100. / (max_speed - 200.);
		ent->dmg = (int)(dmg - 0.5) + 1;
	}
	if(ent->health > 0)
	{
		ent->die = pendulum_die;
		ent->takedamage = DAMAGE_YES;
	}

	ent->blocked = pendulum_blocked;
//	ent->touch = pendulum_touch;

	if(!ent->accel)
		ent->accel = 1;
	else if (ent->accel > ent->speed)
		ent->accel = ent->speed;

	if(!ent->decel)
		ent->decel = 1;
	else if (ent->decel > ent->speed)
		ent->decel = ent->speed;

	gi.setmodel (ent, ent->model);

	ent->s.angles[ROLL] = ent->moveinfo.distance/2;
	ent->moveinfo.start_angles[ROLL] = ent->s.angles[ROLL];
	if(ent->spawnflags & SF_PENDULUM_STARTON)
	{
		ent->think = pendulum_rotate;
		ent->nextthink = level.time + FRAMETIME;
	}
	else
	{
		ent->use = pendulum_use;
	}
	gi.linkentity (ent);

}

