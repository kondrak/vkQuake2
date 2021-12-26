#include "g_local.h"

qboolean has_valid_enemy (edict_t *self);
void HuntTarget (edict_t *self);

edict_t *SpawnThing()
{
	edict_t	*thing;
	thing = G_Spawn();
	thing->classname = gi.TagMalloc(6,TAG_LEVEL);
	strcpy(thing->classname,"thing");
	return thing;
}

void thing_restore_leader (edict_t *self)
{
	edict_t	*monster;
	monster = self->target_ent;
	if(!monster || !monster->inuse)
	{
		G_FreeEdict(self);
		return;
	}
	if(monster->monsterinfo.old_leader && monster->monsterinfo.old_leader->inuse)
	{
		if(VectorCompare(monster->monsterinfo.old_leader->s.origin,self->move_origin))
		{
			self->nextthink = level.time + 0.5;
			return;
		}
		monster->monsterinfo.leader = monster->monsterinfo.old_leader;
		monster->monsterinfo.old_leader = NULL;
		monster->movetarget = monster->goalentity = monster->monsterinfo.leader;
	}
	else
	{
		monster->monsterinfo.aiflags &= ~AI_FOLLOW_LEADER;
	}
	monster->vehicle = NULL;
	monster->monsterinfo.aiflags &= ~(AI_CHASE_THING | AI_SEEK_COVER | AI_EVADE_GRENADE);
	gi.linkentity(monster);
	G_FreeEdict(self);
}
void thing_think (edict_t *self)
{
	vec3_t	vec;
	vec_t	dist;
	edict_t	*monster;

	monster = self->target_ent;
	if(level.time <= self->touch_debounce_time)
	{
		if(monster && monster->inuse)
		{
			if(monster->movetarget == self)
			{
				if(monster->health > 0)
				{
					VectorSubtract(monster->s.origin,self->s.origin,vec);
					vec[2] = 0;
					dist = VectorLength(vec);
					if(dist >= monster->size[0])
					{
						self->nextthink = level.time + FRAMETIME;
						return;
					}
				}
			}
		}
	}
	if(!monster || !monster->inuse || (monster->health <= 0))
	{
		G_FreeEdict(self);
		return;
	}
	if(monster->goalentity == self)
		monster->goalentity = NULL;
	if(monster->movetarget == self)
		monster->movetarget = NULL;
	if(monster->monsterinfo.aiflags & AI_FOLLOW_LEADER)
	{
		monster->monsterinfo.leader = NULL;
		if(monster->monsterinfo.old_leader && monster->monsterinfo.old_leader->inuse)
		{
			monster->monsterinfo.pausetime = level.time + 2;
			monster->monsterinfo.stand(monster);
			VectorCopy(monster->monsterinfo.old_leader->s.origin,self->move_origin);
			self->nextthink = level.time + 2;
			self->think     = thing_restore_leader;
			gi.linkentity(monster);
			return;
		}
		else
		{
			monster->monsterinfo.aiflags &= ~AI_FOLLOW_LEADER;
		}
	}
	if(monster->monsterinfo.aiflags & AI_CHICKEN)
	{
		monster->monsterinfo.stand(monster);
		monster->monsterinfo.aiflags |= AI_STAND_GROUND;
		monster->monsterinfo.pausetime = level.time + 100000;
	}
	monster->vehicle = NULL;
	monster->monsterinfo.aiflags &= ~(AI_CHASE_THING | AI_SEEK_COVER | AI_EVADE_GRENADE);

	G_FreeEdict(self);
	if (has_valid_enemy(monster))
	{
		monster->monsterinfo.pausetime = 0;
		monster->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
		monster->goalentity = monster->enemy;
		if (visible(monster, monster->enemy))
		{
			FoundTarget (monster);
			return;
		}
		HuntTarget (monster);
		return;
	}
	monster->enemy = NULL;
	monster->monsterinfo.pausetime = level.time + 100000000;
	monster->monsterinfo.stand (monster);
}

void thing_think_pause(edict_t *self)
{
	edict_t	*monster;
	if(level.time > self->touch_debounce_time)
	{
		thing_think(self);
		return;
	}
	monster = self->target_ent;
	if(!monster || !monster->inuse)
	{
		G_FreeEdict(self);
		return;
	}
	if(has_valid_enemy(monster))
	{
		vec3_t	dir;
		vec3_t	angles;

		if(visible(monster->enemy,monster))
		{
			self->touch_debounce_time = 0;
			thing_think(self);
			return;
		}
		VectorSubtract(monster->enemy->s.origin,monster->s.origin,dir);
		VectorNormalize(dir);
		vectoangles(dir,angles);
		monster->ideal_yaw = angles[YAW];
		M_ChangeYaw(monster);
	}
	self->nextthink = level.time + FRAMETIME;
}

void thing_grenade_boom (edict_t *self)
{
	edict_t	*monster;

	monster = self->target_ent;
	G_FreeEdict(self);
	if(!monster || !monster->inuse || (monster->health <= 0))
		return;
	monster->monsterinfo.aiflags &= ~(AI_CHASE_THING | AI_EVADE_GRENADE | AI_STAND_GROUND);
	monster->monsterinfo.pausetime = 0;
	if(monster->enemy)
		monster->monsterinfo.run(monster);
}

void thing_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if(self->target_ent != other)
		return;
	if(other->health <= 0)
	{
		G_FreeEdict(self);
		return;
	}
	self->touch = NULL;
	if( self->target_ent->monsterinfo.aiflags & AI_SEEK_COVER )
	{
		edict_t	*monster;
		// For monster/actor seeking cover after firing, 
		// pause a random bit before resuming attack
		self->touch_debounce_time = level.time + random()*6.;
		monster = self->target_ent;
		monster->monsterinfo.stand(monster);
		monster->monsterinfo.pausetime = self->touch_debounce_time;
		self->think = thing_think_pause;
		self->think(self);
		return;
	}
	if( self->target_ent->monsterinfo.aiflags & AI_EVADE_GRENADE )
	{
		edict_t	*grenade = other->next_grenade;

		if(other->goalentity == self)
			other->goalentity = NULL;
		if(other->movetarget == self)
			other->movetarget = NULL;
		other->vehicle = NULL;
		if(grenade)
		{
			// make sure this is still a grenade
			if(grenade->inuse)
			{
				if(Q_stricmp(grenade->classname,"grenade") && Q_stricmp(grenade->classname,"hgrenade"))
					other->next_grenade = grenade = NULL;
			}
			else
				other->next_grenade = grenade = NULL;
		}
		if(grenade)
		{
			if(self->touch_debounce_time > level.time)
			{
				other->monsterinfo.pausetime = self->touch_debounce_time + 0.1;
				other->monsterinfo.aiflags |= AI_STAND_GROUND;
				other->monsterinfo.stand(other);
			}
			else
				other->monsterinfo.pausetime = 0;

			other->enemy = grenade->owner;
			if (has_valid_enemy(other))
			{
				other->goalentity = other->enemy;
				if (visible(other, other->enemy))
					FoundTarget (other);
				else
					HuntTarget (other);
			}
			else
			{
				other->enemy = NULL;
				other->monsterinfo.stand (other);
			}
			if(other->monsterinfo.pausetime > 0)
			{
				self->think = thing_grenade_boom;
				self->nextthink = other->monsterinfo.pausetime;
				return;
			}
			other->monsterinfo.aiflags &= ~(AI_CHASE_THING | AI_EVADE_GRENADE);
		}
		else if (has_valid_enemy(other))
		{
			other->monsterinfo.aiflags &= ~(AI_CHASE_THING | AI_EVADE_GRENADE);
			other->goalentity = other->enemy;
			if (visible(other, other->enemy))
				FoundTarget (other);
			else
				HuntTarget (other);
		}
		else
		{
			other->enemy = NULL;
			other->monsterinfo.pausetime = level.time + 100000000;
			other->monsterinfo.aiflags &= ~(AI_CHASE_THING | AI_EVADE_GRENADE);
			other->monsterinfo.stand (other);
		}
		G_FreeEdict(self);
		return;
	}
	self->touch_debounce_time = 0;
	thing_think(self);
}

void SP_thing (edict_t *self)
{
	self->solid                = SOLID_TRIGGER;
	VectorSet(self->mins,-4,-4,-4);
	VectorSet(self->maxs, 4, 4, 4);
	self->movetype             = MOVETYPE_NONE;
	self->monsterinfo.aiflags |= AI_GOOD_GUY;
	self->svflags             |= SVF_MONSTER;
	self->health               = 1000;
	self->takedamage           = DAMAGE_NO;
	if(developer->value) {
		gi.setmodel (self, "models/items/c_head/tris.md2");
		self->s.effects |= EF_BLASTER;
	}
	self->touch = thing_touch;
	self->think = thing_think;
	self->nextthink = level.time + 2;
	gi.linkentity(self);
}
