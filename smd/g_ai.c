// g_ai.c

#include "g_local.h"

qboolean FindTarget (edict_t *self);
extern cvar_t	*maxclients;

qboolean ai_checkattack (edict_t *self, float dist);
edict_t *medic_FindDeadMonster (edict_t *self);

qboolean	enemy_vis;
qboolean	enemy_infront;
int			enemy_range;
float		enemy_yaw;
/*
=================
AI_SetSightClient

Called once each frame to set level.sight_client to the
player to be checked for in findtarget.

If all clients are either dead or in notarget, sight_client
will be null.

In coop games, sight_client will cycle between the clients.
=================
*/
void AI_SetSightClient (void)
{
	edict_t	*ent;
	int		start, check;

	if (level.sight_client == NULL)
		start = 1;
	else
		start = level.sight_client - g_edicts;

	check = start;
	while (1)
	{
		check++;
		if (check > game.maxclients)
			check = 1;
		ent = &g_edicts[check];
		if (ent->inuse
			&& ent->health > 0
			&& !(ent->flags & (FL_NOTARGET|FL_DISGUISED) ) )
		{
			// If player is using func_monitor, make the sight_client = the fake player at the
			// monitor currently taking the player's place. Do NOT do this for players using a
			// target_monitor, though... in this case both player and fake player are ignored.

			if(ent->client && ent->client->camplayer)
			{
				if(ent->client->spycam)
				{
					level.sight_client = ent->client->camplayer;
					return;
				}
			}
			else
			{
				level.sight_client = ent;
				return;		// got one
			}
		}
		if (check == start)
		{
			level.sight_client = NULL;
			return;		// nobody to see
		}
	}
}

//============================================================================

/*
=============
ai_move

Move the specified distance at current facing.
This replaces the QC functions: ai_forward, ai_back, ai_pain, and ai_painforward
==============
*/
void ai_move(edict_t *self, float dist)
{
	M_walkmove (self, self->s.angles[YAW], dist);
}

//CW+++
/*
=============
ai_strafe

Move sideways the specified distance.
==============
*/
void ai_strafe(edict_t *self, float dist)
{
	float sideang;

	if (self->moreflags == -1)		//set for roll dodging to the right
		sideang = self->s.angles[YAW] - 90.0;
	else
		sideang = self->s.angles[YAW] + 90.0;

	if (sideang >= 180.0)
		sideang -= 360.0;
	if (sideang <= 180.0)
		sideang += 360.0;

	M_walkmove(self, sideang, dist);
}
//CW---

/*
=============
ai_stand

Used for standing around and looking for players
Distance is for slight position adjustments needed by the animations
==============
*/
void ai_stand (edict_t *self, float dist)
{
	vec3_t	v;

	if (dist)
		M_walkmove (self, self->s.angles[YAW], dist);

	if(self->monsterinfo.aiflags & AI_FOLLOW_LEADER)
	{
		if(!self->enemy || !self->enemy->inuse)
		{
			self->movetarget = self->goalentity = self->monsterinfo.leader;
			self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
			self->monsterinfo.pausetime = 0;
		}
	}

	if ( (self->monsterinfo.aiflags & AI_CHICKEN) )
	{
		if ( (level.framenum - self->monsterinfo.chicken_framenum > 200) ||
			 (self->enemy && (self->enemy->last_attacked_framenum > level.framenum - 2) ) )
		{
			self->monsterinfo.aiflags &= ~(AI_CHICKEN | AI_STAND_GROUND);
			self->monsterinfo.pausetime = 0;
			if(self->enemy)
				FoundTarget(self);
		}
	}

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		if (self->enemy && self->enemy->inuse)
		{
			float	length;

			VectorSubtract (self->enemy->s.origin, self->s.origin, v);
			length = VectorLength(v);
			self->ideal_yaw = vectoyaw(v);

			if((level.time >= self->monsterinfo.rangetime) && (self->monsterinfo.aiflags & AI_RANGE_PAUSE))
			{
				if((length < self->monsterinfo.ideal_range[0]) && (rand() & 3))
					self->monsterinfo.rangetime = level.time + 0.5;
				if((length < self->monsterinfo.ideal_range[1]) && (length > self->monsterinfo.ideal_range[0]) && (rand() & 1))
					self->monsterinfo.rangetime = level.time + 0.2;
			}

			if (self->s.angles[YAW] != self->ideal_yaw && self->monsterinfo.aiflags & AI_RANGE_PAUSE)
			{
				if(self->monsterinfo.rangetime < level.time)
				{
					// Lazarus: Don't run if we're still too close
					if(self->monsterinfo.min_range > 0)
					{
						if(length > self->monsterinfo.min_range)
						{
							self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_RANGE_PAUSE);
							self->monsterinfo.run (self);
						}
					}
					else
					{
						self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_RANGE_PAUSE);
						self->monsterinfo.run (self);
					}
				}
			}
			M_ChangeYaw (self);
			ai_checkattack (self, 0);
			if(!enemy_vis && (self->monsterinfo.aiflags & AI_RANGE_PAUSE))
			{
				self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_RANGE_PAUSE);
				self->monsterinfo.run (self);
			}
		}
		else
			FindTarget (self);
		return;
	}

	if (FindTarget (self))
		return;
	
	if (level.time > self->monsterinfo.pausetime)
	{
		// Lazarus: Solve problem of monsters pausing at path_corners, taking off in 
		//          original direction
		if(self->enemy && self->enemy->inuse)
			VectorSubtract (self->enemy->s.origin, self->s.origin, v);
		else if(self->goalentity)
			VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
		else {
			self->monsterinfo.pausetime = level.time + random() * 15;
			return;
		}
		self->ideal_yaw = vectoyaw (v);

		// Lazarus: Let misc_actors who are following their leader RUN even when not mad
		if( (self->monsterinfo.aiflags & AI_FOLLOW_LEADER) && (self->movetarget) &&
			(self->movetarget->inuse) )
		{
			float	R;
			R = realrange(self,self->movetarget);
			if(R > ACTOR_FOLLOW_RUN_RANGE)
				self->monsterinfo.run (self);
			else if(R > ACTOR_FOLLOW_STAND_RANGE || !self->movetarget->client)
				self->monsterinfo.walk (self);
		}
		else
			self->monsterinfo.walk (self);
		return;
	}

	if (!(self->spawnflags & SF_MONSTER_SIGHT) && (self->monsterinfo.idle) && (level.time > self->monsterinfo.idle_time))
	{
		if(self->monsterinfo.aiflags & AI_MEDIC)
			abortHeal(self,false);

		if (self->monsterinfo.idle_time)
		{
			self->monsterinfo.idle (self);
			self->monsterinfo.idle_time = level.time + 15 + random() * 15;
		}
		else
		{
			self->monsterinfo.idle_time = level.time + random() * 15;
		}
	}
}


void ai_walk (edict_t *self, float dist)
{
	// Lazarus: If we're following the leader and have no enemy, run to him
	if ((!self->enemy) && (self->monsterinfo.aiflags & AI_FOLLOW_LEADER))
		self->movetarget = self->goalentity = self->monsterinfo.leader;

	M_MoveToGoal (self, dist);

	// check for noticing a player
	if (FindTarget (self))
		return;

	if ((self->monsterinfo.search) && (level.time > self->monsterinfo.idle_time))
	{
		if(self->monsterinfo.aiflags & AI_MEDIC)
			abortHeal(self,false);

		if (self->monsterinfo.idle_time)
		{
			self->monsterinfo.search (self);
			self->monsterinfo.idle_time = level.time + 15 + random() * 15;
		}
		else
		{
			self->monsterinfo.idle_time = level.time + random() * 15;
		}
	}
}


/*
=============
ai_charge

Turns towards target and advances
Use this call with a distance of 0 to replace ai_face
==============
*/
void ai_charge (edict_t *self, float dist)
{
	vec3_t	v;

	// Lazarus: Check for existence and validity of enemy.
	// This is normally not necessary, but target_anger making 
	// monster mad at a static object (a pickup, for example)
	// previously resulted in weirdness here
	if (!self->enemy || !self->enemy->inuse)
		return;

	VectorSubtract (self->enemy->s.origin, self->s.origin, v);
	self->ideal_yaw = vectoyaw(v);
	M_ChangeYaw (self);
	if (dist)
		M_walkmove (self, self->s.angles[YAW], dist);
}


/*
=============
ai_turn

don't move, but turn towards ideal_yaw
Distance is for slight position adjustments needed by the animations
=============
*/
void ai_turn (edict_t *self, float dist)
{
	if (dist)
		M_walkmove (self, self->s.angles[YAW], dist);

	if (FindTarget (self))
		return;
	
	M_ChangeYaw (self);
}



/*

.enemy
Will be world if not currently angry at anyone.

.movetarget
The next path spot to walk toward.  If .enemy, ignore .movetarget.
When an enemy is killed, the monster will try to return to it's path.

.hunt_time
Set to time + something when the player is in sight, but movement straight for
him is blocked.  This causes the monster to use wall following code for
movement direction instead of sighting on the player.

.ideal_yaw
A yaw angle of the intended direction, which will be turned towards at up
to 45 deg / state.  If the enemy is in view and hunt_time is not active,
this will be the exact line towards the enemy.

.pausetime
A monster will leave it's stand state and head towards it's .movetarget when
time > .pausetime.

walkmove(angle, speed) primitive is all or nothing
*/

/*
=============
range

returns the range catagorization of an entity reletive to self
0	melee range, will become hostile even if back is turned
1	visibility and infront, or visibility and show hostile
2	infront and show hostile
3	only triggered by damage
=============
*/
int range (edict_t *self, edict_t *other)
{
	vec3_t	v;
	float	len;

	VectorSubtract (self->s.origin, other->s.origin, v);
	len = VectorLength (v);
	if (len < MELEE_DISTANCE)
		return RANGE_MELEE;
	if (len < 500)
		return RANGE_NEAR;
	if (len < self->monsterinfo.max_range)
		return RANGE_MID;
	return RANGE_FAR;
}

/*
=============
visible

returns 1 if the entity is visible to self, even if not infront ()
=============
*/
qboolean visible (edict_t *self, edict_t *other)
{
	vec3_t	spot1;
	vec3_t	spot2;
	trace_t	trace;

//CW++
	if (!self)
	{
		gi.dprintf("**ERROR: Invalid [self] pointer passed to visible()\n");
		return false;
	}

	if (!other)
	{
		gi.dprintf("**ERROR: Invalid [other] pointer passed to visible()\n");
		return false;
	}
//CW--

	VectorCopy (self->s.origin, spot1);
	spot1[2] += self->viewheight;
	VectorCopy (other->s.origin, spot2);
	spot2[2] += other->viewheight;
	trace = gi.trace (spot1, vec3_origin, vec3_origin, spot2, self, MASK_OPAQUE);

	// Lazarus: Take fog into account for monsters

	if ( (trace.fraction == 1.0) || (trace.ent == other))
	{
		if( (level.active_fog) && (self->svflags & SVF_MONSTER) )
		{
			fog_t	*pfog;
			float	r;
			float	dw;
			vec3_t	v;

			pfog = &level.fog;
			VectorSubtract(spot2,spot1,v);
			r = VectorLength(v);
			switch(pfog->Model)
			{
			case 1:
				dw = pfog->Density/10000. * r;
				self->monsterinfo.visibility = exp( -dw );
				break;
			case 2:
				dw = pfog->Density/10000. * r;
				self->monsterinfo.visibility = exp( -dw*dw );
				break;
			default:
				if((r < pfog->Near) || (pfog->Near == pfog->Far))
					self->monsterinfo.visibility = 1.0;
				else if(r > pfog->Far)
					self->monsterinfo.visibility = 0.0;
				else
					self->monsterinfo.visibility = 1.0 - (r - pfog->Near)/(pfog->Far - pfog->Near);
				break;
			}
//			if(developer->value)
//				gi.dprintf("r=%g, vis=%g\n",r,self->monsterinfo.visibility);
			if(self->monsterinfo.visibility < 0.05)
				return false;
			else
				return true;
		}
		else
		{
			self->monsterinfo.visibility = 1.0;
			return true;
		}
	}
	return false;
}


/*
=============
infront

returns 1 if the entity is in front (in sight) of self
=============
*/
qboolean infront (edict_t *self, edict_t *other)
{
	vec3_t	vec;
	float	dot;
	vec3_t	forward;
	
	AngleVectors (self->s.angles, forward, NULL, NULL);
	VectorSubtract (other->s.origin, self->s.origin, vec);
	VectorNormalize (vec);
	dot = DotProduct (vec, forward);
	
	if (dot > 0.3)
		return true;
	return false;
}

/*
=============
canReach

similar to visible, but uses a different mask
=============
*/
qboolean canReach (edict_t *self, edict_t *other)
{
	vec3_t	spot1;
	vec3_t	spot2;
	trace_t	trace;

	VectorCopy (self->s.origin, spot1);
	spot1[2] += self->viewheight;
	VectorCopy (other->s.origin, spot2);
	spot2[2] += other->viewheight;
	trace = gi.trace (spot1, vec3_origin, vec3_origin, spot2, self, MASK_SHOT|MASK_WATER);
	
	if (trace.fraction == 1.0 || trace.ent == other)		// PGM
		return true;
	return false;
}

//============================================================================

void HuntTarget (edict_t *self)
{
	vec3_t	vec;

	// Lazarus: avert impending disaster
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->goalentity = self->enemy;
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.stand (self);
	else
		self->monsterinfo.run (self);
	VectorSubtract (self->enemy->s.origin, self->s.origin, vec);
	self->ideal_yaw = vectoyaw(vec);
	// wait a while before first attack
	if (!(self->monsterinfo.aiflags & AI_STAND_GROUND))
		AttackFinished (self, 1);
}

void FoundTarget (edict_t *self)
{
	edict_t	*goodguy;
	vec3_t	v;
//	trace_t	tr;

	// Lazarus: avert impending disaster
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	if (self->monsterinfo.aiflags & AI_CHICKEN)
		return;

	// let other monsters see this monster for a while
	if (self->enemy->client)
	{
		self->enemy->flags &= ~FL_DISGUISED;

		level.sight_entity = self;
		level.sight_entity_framenum = level.framenum;
		level.sight_entity->light_level = 128;

		goodguy = NULL;
		goodguy = G_Find(NULL,FOFS(dmgteam),"player");
		while(goodguy)
		{
			if(goodguy->health > 0)
			{
				if(!goodguy->enemy)
				{
					if(goodguy->monsterinfo.aiflags & AI_ACTOR)
					{
						// Can he see enemy?
//						tr = gi.trace(goodguy->s.origin,vec3_origin,vec3_origin,self->enemy->s.origin,goodguy,MASK_OPAQUE);
//						if(tr.fraction == 1.0)
						if(gi.inPVS(goodguy->s.origin,self->enemy->s.origin))
						{
							goodguy->monsterinfo.aiflags |= AI_FOLLOW_LEADER;
							goodguy->monsterinfo.old_leader = NULL;
							goodguy->monsterinfo.leader = self->enemy;
						}
					}
				}
			}
			goodguy = G_Find(goodguy,FOFS(dmgteam),"player");
		}
	}
	self->show_hostile = level.time + 1;		// wake up other monsters
	VectorCopy(self->enemy->s.origin, self->monsterinfo.last_sighting);
	self->monsterinfo.trail_time = level.time;
	if (!self->combattarget)
	{
		HuntTarget (self);
		return;
	}

	self->goalentity = self->movetarget = G_PickTarget(self->combattarget);
	if (!self->movetarget)
	{
		self->goalentity = self->movetarget = self->enemy;
		HuntTarget (self);
		gi.dprintf("%s at %s, combattarget %s not found\n", self->classname, vtos(self->s.origin), self->combattarget);
		return;
	}
	// Lazarus: Huh? How come yaw for combattarget isn't set?
	VectorSubtract(self->movetarget->s.origin,self->s.origin,v);
	self->ideal_yaw = vectoyaw(v);

	// clear out our combattarget, these are a one shot deal
	self->combattarget = NULL;
	self->monsterinfo.aiflags |= AI_COMBAT_POINT;

	// clear the targetname, that point is ours!
	// Lazarus: Why, why, why???? This doesn't remove the point_combat, only makes it inaccessible
	// to other monsters. 
	//self->movetarget->targetname = NULL;
	self->monsterinfo.pausetime = 0;

	// run for it
	self->monsterinfo.run (self);
}


/*
===========
FindTarget

Self is currently not attacking anything, so try to find a target

Returns TRUE if an enemy was sighted

When a player fires a missile, the point of impact becomes a fakeplayer so
that monsters that see the impact will respond as if they had seen the
player.

To avoid spending too much time, only a single client (or fakeclient) is
checked each frame.  This means multi player games will have slightly
slower noticing monsters.
============
*/
qboolean FindTarget (edict_t *self)
{
	edict_t		*client=NULL;
	qboolean	heardit;
	int			r;

	if(self->monsterinfo.aiflags & (AI_CHASE_THING | AI_HINT_TEST))
		return false;

	if (self->monsterinfo.aiflags & AI_GOOD_GUY)
	{
		if (self->goalentity && self->goalentity->inuse && self->goalentity->classname)
		{
			if (strcmp(self->goalentity->classname, "target_actor") == 0)
				return false;
		}

		// Lazarus: Look for monsters
		if( !self->enemy )
		{
			if( self->monsterinfo.aiflags & AI_FOLLOW_LEADER )
			{
				int		i;
				edict_t	*e;
				edict_t	*best=NULL;
				vec_t	dist, best_dist;

				best_dist = self->monsterinfo.max_range;
				for(i=game.maxclients+1; i<globals.num_edicts; i++) {
					e = &g_edicts[i];
					if(!e->inuse)
						continue;
					if(!(e->svflags & SVF_MONSTER))
						continue;
					if(e->svflags & SVF_NOCLIENT)
						continue;
					if(e->solid == SOLID_NOT)
						continue;
					if(e->monsterinfo.aiflags & AI_GOOD_GUY)
						continue;
					if(!visible(self,e))
						continue;
					if ( self->monsterinfo.aiflags & AI_BRUTAL )
					{
						if (e->health <= e->gib_health)
							continue;
					}
					else if (e->health <= 0)
						continue;
					dist = realrange(self,e);
					if(dist < best_dist)
					{
						best_dist = dist;
						best = e;
					}
				}
				if(best)
				{
					self->enemy = best;
					FoundTarget(self);
					return true;
				}
			}
			return false;
		}
		else if (level.time < self->monsterinfo.pausetime )
			return false;
		else {
			if (!visible (self, self->enemy))
				return false;
			else {
				FoundTarget(self);
				return true;
			}
		}
	}

	// if we're going to a combat point, just proceed
	if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
		return false;

// if the first spawnflag bit is set, the monster will only wake up on
// really seeing the player, not another monster getting angry or hearing
// something

// revised behavior so they will wake up if they "see" a player make a noise
// but not weapon impact/explosion noises

	heardit = false;
	if ((level.sight_entity_framenum >= (level.framenum - 1)) && !(self->spawnflags & SF_MONSTER_SIGHT) )
	{
		client = level.sight_entity;
		if (client->enemy == self->enemy)
		{
			return false;
		}
	}
	else if (level.disguise_violation_framenum > level.framenum)
	{
		client = level.disguise_violator;
	}
	else if (level.sound_entity_framenum >= (level.framenum - 1))
	{
		client = level.sound_entity;
		heardit = true;
	}
	else if (!(self->enemy) && (level.sound2_entity_framenum >= (level.framenum - 1)) && !(self->spawnflags & SF_MONSTER_SIGHT) )
	{
		client = level.sound2_entity;
		heardit = true;
	}
	else
	{
		client = level.sight_client;
		if (!client)
			return false;	// no clients to get mad at
	}

	// if the entity went away, forget it
	if (!client || !client->inuse)
		return false;

	// Lazarus
	if(client->client && client->client->camplayer)
		client = client->client->camplayer;

	if (client == self->enemy)
		return true;	// JDC false;

	// Lazarus: Force idle medics to look for dead monsters
	if (!self->enemy && !Q_stricmp(self->classname,"monster_medic"))
	{
		if(medic_FindDeadMonster(self))
			return true;
	}

	//PMM - hintpath coop fix
	if ((self->monsterinfo.aiflags & AI_HINT_PATH) && (coop) && (coop->value))
		heardit = false;
	// pmm

	if (client->client)
	{
		if (client->flags & FL_NOTARGET)
			return false;
	}
	else if (client->svflags & SVF_MONSTER)
	{
		if (!client->enemy)
			return false;
		if (client->enemy->flags & FL_NOTARGET)
			return false;
	}
	else if (heardit)
	{
		if (client->owner && (client->owner->flags & FL_NOTARGET))
			return false;
	}
	else
		return false;

	if (!heardit)
	{
		r = range (self, client);

		if (r == RANGE_FAR)
			return false;

// this is where we would check invisibility

		// is client in an spot too dark to be seen?
		if (client->light_level <= 5) 
			return false;

		if (!visible (self, client))
			return false;

		if (r == RANGE_NEAR)
		{
			if (client->show_hostile < level.time && !infront (self, client))
				return false;
		}
		else if (r == RANGE_MID)
		{
			if (!infront (self, client))
				return false;
		}

		self->enemy = client;

		if (strcmp(self->enemy->classname, "player_noise") != 0)
		{
			self->monsterinfo.aiflags &= ~AI_SOUND_TARGET;

			if (!self->enemy->client)
			{
				self->enemy = self->enemy->enemy;
				if (!self->enemy->client)
				{
					self->enemy = NULL;
					return false;
				}
			}
		}
	}
	else	// heardit
	{
		vec3_t	temp;

		if (self->spawnflags & SF_MONSTER_SIGHT)
		{
			if (!visible (self, client))
				return false;
		}
		else
		{
			if (!gi.inPHS(self->s.origin, client->s.origin))
				return false;
		}

		VectorSubtract (client->s.origin, self->s.origin, temp);

		if (VectorLength(temp) > 1000)	// too far to hear
		{
			return false;
		}

		// check area portals - if they are different and not connected then we can't hear it
		if (client->areanum != self->areanum)
			if (!gi.AreasConnected(self->areanum, client->areanum))
				return false;

		self->ideal_yaw = vectoyaw(temp);
		M_ChangeYaw (self);

		// hunt the sound for a bit; hopefully find the real player
		self->monsterinfo.aiflags |= AI_SOUND_TARGET;
		self->enemy = client;
	}

//
// got one
//
	// PMM - if we got an enemy, we need to bail out of hint paths, so take over here
	if (self->monsterinfo.aiflags & AI_HINT_PATH)
	{
		// this calls foundtarget for us
		hintpath_stop (self);
	}
	else if(self->monsterinfo.aiflags & AI_MEDIC_PATROL)
	{
		medic_StopPatrolling (self);
	}
	else
	{
		FoundTarget (self);
	}
	// pmm

	if (!(self->monsterinfo.aiflags & AI_SOUND_TARGET) && (self->monsterinfo.sight))
		self->monsterinfo.sight (self, self->enemy);

	return true;
}


//=============================================================================

/*
============
FacingIdeal

============
*/
qboolean FacingIdeal(edict_t *self)
{
	float	delta;

	delta = anglemod(self->s.angles[YAW] - self->ideal_yaw);
	if (delta > 45 && delta < 315)
		return false;
	return true;
}


//=============================================================================

qboolean M_CheckAttack (edict_t *self)
{
	vec3_t	spot1, spot2;
	float	chance;
	trace_t	tr;

	// Lazarus: Paranoia check
	if (!self->enemy)
		return false;

	if (self->enemy->health > 0)
	{
	// see if any entities are in the way of the shot
		VectorCopy (self->s.origin, spot1);
		spot1[2] += self->viewheight;
		VectorCopy (self->enemy->s.origin, spot2);
		spot2[2] += self->enemy->viewheight;
		tr = gi.trace (spot1, NULL, NULL, spot2, self, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_SLIME|CONTENTS_LAVA|CONTENTS_WINDOW);

		// do we have a clear shot?
		if (tr.ent != self->enemy)
			return false;
	}
	
	// melee attack
	if (enemy_range == RANGE_MELEE)
	{
		// don't always melee in easy mode
		if (skill->value == 0 && (rand()&3) )
			return false;
		if (self->monsterinfo.melee)
			self->monsterinfo.attack_state = AS_MELEE;
		else
			self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}
	
// missile attack
	if (!self->monsterinfo.attack)
		return false;
		
	if (level.time < self->monsterinfo.attack_finished)
		return false;
		
	if (enemy_range == RANGE_FAR)
		return false;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		chance = 0.4;
	}
	else if (enemy_range == RANGE_MELEE)
	{
		chance = 0.2;
	}
	else if (enemy_range == RANGE_NEAR)
	{
		chance = 0.1;
	}
	else if (enemy_range == RANGE_MID)
	{
		chance = 0.02;
	}
	else
		return false;

	if (skill->value == 0)
		chance *= 0.5;
	else if (skill->value >= 2)
		chance *= 2;

	if (random () < chance)
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		self->monsterinfo.attack_finished = level.time + 2*random();
		return true;
	}

	if (self->flags & FL_FLY)
	{
		if (random() < 0.3)
			self->monsterinfo.attack_state = AS_SLIDING;
		else
			self->monsterinfo.attack_state = AS_STRAIGHT;
	}

	return false;
}


/*
=============
ai_run_melee

Turn and close until within an angle to launch a melee attack
=============
*/
void ai_run_melee(edict_t *self)
{
	self->ideal_yaw = enemy_yaw;
	M_ChangeYaw (self);

	if (FacingIdeal(self))
	{
		self->monsterinfo.melee (self);
		self->monsterinfo.attack_state = AS_STRAIGHT;
	}
}


/*
=============
ai_run_missile

Turn in place until within an angle to launch a missile attack
=============
*/
void ai_run_missile(edict_t *self)
{
	self->ideal_yaw = enemy_yaw;
	M_ChangeYaw (self);

	if (FacingIdeal(self))
	{
		self->monsterinfo.attack (self);
		self->monsterinfo.attack_state = AS_STRAIGHT;
	}
};


/*
=============
ai_run_slide

Strafe sideways, but stay at aproximately the same range
=============
*/
void ai_run_slide(edict_t *self, float distance)
{
	float	ofs;
	
	self->ideal_yaw = enemy_yaw;
	M_ChangeYaw (self);

	if (self->monsterinfo.lefty)
		ofs = 90;
	else
		ofs = -90;
	
	if (M_walkmove (self, self->ideal_yaw + ofs, distance))
		return;
		
	self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
	M_walkmove (self, self->ideal_yaw - ofs, distance);
}


/*
=============
ai_checkattack

Decides if we're going to attack or do something else
used by ai_run and ai_stand
=============
*/
qboolean ai_checkattack (edict_t *self, float dist)
{
	vec3_t		temp;
	qboolean	hesDeadJim;

	// this causes monsters to run blindly to the combat point w/o firing
	if (self->goalentity)
	{
		if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
			return false;

		if (self->monsterinfo.aiflags & AI_SOUND_TARGET)
		{
			if ((level.time - self->enemy->teleport_time) > 5.0)
			{
				if (self->goalentity == self->enemy)
					if (self->movetarget)
						self->goalentity = self->movetarget;
					else
						self->goalentity = NULL;
				self->monsterinfo.aiflags &= ~AI_SOUND_TARGET;
				if (self->monsterinfo.aiflags & AI_TEMP_STAND_GROUND)
					self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
			}
			else
			{
				self->show_hostile = level.time + 1;
				return false;
			}
		}
	}

	enemy_vis = false;

// see if the enemy is dead
	hesDeadJim = false;
	if ((!self->enemy) || (!self->enemy->inuse))
	{
		hesDeadJim = true;
	}
	else if (self->monsterinfo.aiflags & AI_MEDIC)
	{
		if (self->enemy->health > 0)
		{
			hesDeadJim = true;
			self->monsterinfo.aiflags &= ~AI_MEDIC;
		}
	}
	else
	{
		if (self->monsterinfo.aiflags & AI_BRUTAL)
		{
			// Lazarus: This value should be enemy class-dependent
			//if (self->enemy->health <= -80)
			if (self->enemy->health <= self->enemy->gib_health)
				hesDeadJim = true;
		}
		else
		{
			if (self->enemy->health <= 0)
				hesDeadJim = true;
		}
	}

	if (hesDeadJim)
	{
		self->enemy = NULL;
	// FIXME: look all around for other targets
		if (self->oldenemy && self->oldenemy->health > 0)
		{
			self->enemy = self->oldenemy;
			self->oldenemy = NULL;
			HuntTarget (self);
		}
		else
		{
			if (self->movetarget)
			{
				self->goalentity = self->movetarget;

				// Lazarus: Let misc_actors who are following their leader RUN even when not mad
				if( (self->monsterinfo.aiflags & AI_FOLLOW_LEADER) &&
					(self->movetarget) &&
					(self->movetarget->inuse) )
				{
					float	R;

					R = realrange(self,self->movetarget); 
					if(R > ACTOR_FOLLOW_RUN_RANGE)
						self->monsterinfo.run (self);
					else if(R > ACTOR_FOLLOW_STAND_RANGE || !self->movetarget->client)
						self->monsterinfo.walk (self);
					else {
						self->monsterinfo.pausetime = level.time + 0.5;
						self->monsterinfo.stand (self);
					}
				}
				else
					self->monsterinfo.walk (self);
			}
			else
			{
				// we need the pausetime otherwise the stand code
				// will just revert to walking with no target and
				// the monsters will wonder around aimlessly trying
				// to hunt the world entity
				self->monsterinfo.pausetime = level.time + 100000000;
				self->monsterinfo.stand (self);
			}
			return true;
		}
	}

	self->show_hostile = level.time + 1;		// wake up other monsters

// check knowledge of enemy

	enemy_vis = visible(self, self->enemy);

	if (enemy_vis)
	{
		self->monsterinfo.search_time = level.time + 5;
		VectorCopy (self->enemy->s.origin, self->monsterinfo.last_sighting);
	}

// look for other coop players here
//	if (coop && self->monsterinfo.search_time < level.time)
//	{
//		if (FindTarget (self))
//			return true;
//	}

	if (self->monsterinfo.aiflags & AI_CHICKEN)
	{
		if (enemy_vis)
		{
			if (ai_chicken(self,self->enemy))
				return false;
			else
			{
				self->monsterinfo.aiflags &= ~(AI_CHICKEN | AI_STAND_GROUND);
				self->monsterinfo.pausetime = 0;
				FoundTarget(self);
			}
		}
		else
			return false;
	}

	enemy_infront = infront(self, self->enemy);
	enemy_range = range(self, self->enemy);
	VectorSubtract (self->enemy->s.origin, self->s.origin, temp);
	enemy_yaw = vectoyaw(temp);


	// JDC self->ideal_yaw = enemy_yaw;

	if (self->monsterinfo.attack_state == AS_MISSILE)
	{
		ai_run_missile (self);
		return true;
	}
	if (self->monsterinfo.attack_state == AS_MELEE)
	{
		ai_run_melee (self);
		return true;
	}

	// if enemy is not currently visible, we will never attack
	if (!enemy_vis)
		return false;

	if( self->monsterinfo.checkattack (self) )
	{
		self->enemy->last_attacked_framenum = level.framenum;
		return true;
	}
	else
		return false;
}


#define HINT_PATH_START_TIME 3
#define HINT_PATH_RESTART_TIME 5

/*
=============
ai_run

The monster has an enemy it is trying to kill
=============
*/
void ai_run (edict_t *self, float dist)
{
	vec3_t		v;
	edict_t		*tempgoal;
	edict_t		*save;
	qboolean	new;
	edict_t		*marker;
	float		d1, d2;
	trace_t		tr;
	vec3_t		v_forward, v_right;
	float		left, center, right;
	vec3_t		left_target, right_target;
	qboolean	alreadyMoved = false;
	qboolean	gotcha=false;
	edict_t		*realEnemy;

	// if we're going to a combat point, just proceed
	if ( self->monsterinfo.aiflags & (AI_COMBAT_POINT | AI_CHASE_THING | AI_HINT_TEST))
	{
		M_MoveToGoal (self, dist);
		return;
	}

	if ( self->monsterinfo.aiflags & AI_MEDIC_PATROL )
	{
		if (!FindTarget(self))
		{
			M_MoveToGoal(self,dist);
			return;
		}
	}


	// Lazarus: If we're following the leader and have no enemy, go ahead
	if ((!self->enemy) && (self->monsterinfo.aiflags & AI_FOLLOW_LEADER))
	{
		self->movetarget = self->goalentity = self->monsterinfo.leader;
		if(!self->movetarget)
		{
			self->monsterinfo.pausetime = level.time + 2;
			self->monsterinfo.stand(self);
			return;
		}
		self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
		self->monsterinfo.pausetime = 0;
		M_MoveToGoal (self, dist);
		return;
	}
//==========
//PGM
	// if we're currently looking for a hint path
	if (self->monsterinfo.aiflags & AI_HINT_PATH)
	{
		// determine direction to our destination hintpath.
		// FIXME - is this needed EVERY time? I was having trouble with them
		// sometimes not going after it, and this fixed it.
//		VectorSubtract(self->movetarget->s.origin, self->s.origin, v);
//		vectoangles(v, v_forward);
//		self->ideal_yaw = v_forward[YAW];
//		gi.dprintf("seeking hintpath. origin: %s %0.1f\n", vtos(v), self->ideal_yaw);
		M_MoveToGoal (self, dist);
		if(!self->inuse)
			return;			// PGM - g_touchtrigger free problem
//		return;

		// first off, make sure we're looking for the player, not a noise he made
		if (self->enemy)
		{
			if (self->enemy->inuse)
			{
				if (strcmp(self->enemy->classname, "player_noise") != 0)
					realEnemy = self->enemy;
				else if (self->enemy->owner)
					realEnemy = self->enemy->owner;
				else // uh oh, can't figure out enemy, bail
				{
					self->enemy = NULL;
					hintpath_stop (self);
					return;
				}
			}
			else
			{
				self->enemy = NULL;
				hintpath_stop (self);
				return;
			}
		}
		else
		{
			hintpath_stop (self);
			return;
		}

		if (coop && coop->value)
		{
			// if we're in coop, check my real enemy first .. if I SEE him, set gotcha to true
			if (self->enemy && visible(self, realEnemy))
				gotcha = true;
			else // otherwise, let FindTarget bump us out of hint paths, if appropriate
				FindTarget(self);
		}
		else
		{
			// Lazarus: special case for medics with AI_MEDIC and AI_HINT_PATH set. If
			//          range is farther than MEDIC_MAX_HEAL_DISTANCE we essentially
			//          lie... pretend the enemy isn't seen.
			if ( self->monsterinfo.aiflags & AI_MEDIC )
			{
				if(realrange(self,realEnemy) > MEDIC_MAX_HEAL_DISTANCE)
				{
					// Since we're on a hint_path trying to get in position to 
					// heal monster, rather than actually healing him,
					// allow more time
					self->timestamp = level.time + MEDIC_TRY_TIME;
					return;
				}
			}

			if (self->enemy && visible(self, realEnemy))
			{
				//CWFIXME: switch to hintpath searching if efforts to reach a visible player have been fruitless for a few seconds
				gotcha = true;
			}
		}
		
		// if we see the player, stop following hintpaths.
		if (gotcha)
		{
			// disconnect from hintpaths and start looking normally for players.
			hintpath_stop (self);
		}
		return;
	}
//PGM
//==========

	if (self->monsterinfo.aiflags & AI_SOUND_TARGET)
	{
		if (self->enemy)
			VectorSubtract (self->s.origin, self->enemy->s.origin, v);

		if ((!self->enemy) || (VectorLength(v) < 64))
		{
			self->monsterinfo.aiflags |= (AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
			self->monsterinfo.stand (self);
			return;
		}

		M_MoveToGoal (self, dist);
		// PMM - prevent double moves for sound_targets
		alreadyMoved = true;
		// pmm
		if(!self->inuse)
			return;			// PGM - g_touchtrigger free problem

		if (!FindTarget (self))
			return;
	}

	if (ai_checkattack (self, dist))
		return;

	if (self->monsterinfo.attack_state == AS_SLIDING)
	{
		ai_run_slide (self, dist);
		return;
	}

	if ((self->enemy) && (self->enemy->inuse) && (enemy_vis))
	{
//		if (self.aiflags & AI_LOST_SIGHT)
//			dprint("regained sight\n");
		if(!alreadyMoved)
			M_MoveToGoal (self, dist);
		if(!self->inuse)
			return;
		self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
		VectorCopy (self->enemy->s.origin, self->monsterinfo.last_sighting);
		self->monsterinfo.trail_time = level.time;
		return;
	}

//=======
//PGM
	// if we've been looking (unsuccessfully) for the player for 3 seconds
	if((self->monsterinfo.trail_time + HINT_PATH_START_TIME) <= level.time)
	{
		// and we haven't checked for valid hint paths in the last 5 seconds
		if((self->monsterinfo.last_hint_time + HINT_PATH_RESTART_TIME) <= level.time)
		{
			// check for hint_paths.
			self->monsterinfo.last_hint_time = level.time;
			if(monsterlost_checkhint(self))
				return;
		}
	}
//PGM
//=======

	// coop will change to another enemy if visible
	if (coop->value)
	{	// FIXME: insane guys get mad with this, which causes crashes!
		if (FindTarget (self))
			return;
	}

	// Lazarus: for medics, IF hint_paths are present then cut back a bit on max
	//          search time and let him go idle so he'll start tracking hint_paths
	if (self->monsterinfo.search_time)
	{
		if (!Q_stricmp(self->classname,"monster_medic") && hint_paths_present)
		{
			if(developer->value)
				gi.dprintf("medic search_time=%g\n",level.time - self->monsterinfo.search_time);

			if (level.time > (self->monsterinfo.search_time + 15))
			{
				if(developer->value)
					gi.dprintf("medic search timeout, going idle\n");

				if(!alreadyMoved)
					M_MoveToGoal (self, dist);
				self->monsterinfo.search_time = 0;
				if(self->goalentity == self->enemy)
					self->goalentity = NULL;
				if(self->movetarget == self->enemy)
					self->movetarget = NULL;
				self->enemy = self->oldenemy = NULL;
				self->monsterinfo.pausetime = level.time + 2;
				self->monsterinfo.stand(self);
				return;
			}
		}
		else if (level.time > (self->monsterinfo.search_time + 20))
		{
			if(!alreadyMoved)
				M_MoveToGoal (self, dist);
			self->monsterinfo.search_time = 0;
			return;
		}
	}

	save = self->goalentity;
	tempgoal = G_Spawn();
	self->goalentity = tempgoal;

	new = false;

	if (!(self->monsterinfo.aiflags & AI_LOST_SIGHT))
	{
		// just lost sight of the player, decide where to go first
//		dprint("lost sight of player, last seen at "); dprint(vtos(self.last_sighting)); dprint("\n");
		self->monsterinfo.aiflags |= (AI_LOST_SIGHT | AI_PURSUIT_LAST_SEEN);
		self->monsterinfo.aiflags &= ~(AI_PURSUE_NEXT | AI_PURSUE_TEMP);
		new = true;
	}

	if (self->monsterinfo.aiflags & AI_PURSUE_NEXT)
	{
		self->monsterinfo.aiflags &= ~AI_PURSUE_NEXT;
//		dprint("reached current goal: "); dprint(vtos(self.origin)); dprint(" "); dprint(vtos(self.last_sighting)); dprint(" "); dprint(ftos(vlen(self.origin - self.last_sighting))); dprint("\n");

		// give ourself more time since we got this far
		self->monsterinfo.search_time = level.time + 5;

		if (self->monsterinfo.aiflags & AI_PURSUE_TEMP)
		{
//			dprint("was temp goal; retrying original\n");
			self->monsterinfo.aiflags &= ~AI_PURSUE_TEMP;
			marker = NULL;
			VectorCopy (self->monsterinfo.saved_goal, self->monsterinfo.last_sighting);
			new = true;
		}
		else if (self->monsterinfo.aiflags & AI_PURSUIT_LAST_SEEN)
		{
			self->monsterinfo.aiflags &= ~AI_PURSUIT_LAST_SEEN;
			marker = PlayerTrail_PickFirst (self);
		}
		else
		{
			marker = PlayerTrail_PickNext (self);
		}

		if (marker)
		{
			VectorCopy (marker->s.origin, self->monsterinfo.last_sighting);
			self->monsterinfo.trail_time = marker->timestamp;
			self->s.angles[YAW] = self->ideal_yaw = marker->s.angles[YAW];
//			dprint("heading is "); dprint(ftos(self.ideal_yaw)); dprint("\n");

//			debug_drawline(self.origin, self.last_sighting, 52);
			new = true;
		}
	}

	VectorSubtract (self->s.origin, self->monsterinfo.last_sighting, v);
	d1 = VectorLength(v);
	if (d1 <= dist)
	{
		self->monsterinfo.aiflags |= AI_PURSUE_NEXT;
		dist = d1;
	}

	VectorCopy (self->monsterinfo.last_sighting, self->goalentity->s.origin);

	if (new)
	{
		tr = gi.trace(self->s.origin, self->mins, self->maxs, self->monsterinfo.last_sighting, self, MASK_PLAYERSOLID);
		if (tr.fraction < 1)
		{
			VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
			d1 = VectorLength(v);
			center = tr.fraction;
			d2 = d1 * ((center+1)/2);
			self->s.angles[YAW] = self->ideal_yaw = vectoyaw(v);
			AngleVectors(self->s.angles, v_forward, v_right, NULL);

			VectorSet(v, d2, -16, 0);
			G_ProjectSource (self->s.origin, v, v_forward, v_right, left_target);
			tr = gi.trace(self->s.origin, self->mins, self->maxs, left_target, self, MASK_PLAYERSOLID);
			left = tr.fraction;

			VectorSet(v, d2, 16, 0);
			G_ProjectSource (self->s.origin, v, v_forward, v_right, right_target);
			tr = gi.trace(self->s.origin, self->mins, self->maxs, right_target, self, MASK_PLAYERSOLID);
			right = tr.fraction;

			center = (d1*center)/d2;
			if (left >= center && left > right)
			{
				if (left < 1)
				{
					VectorSet(v, d2 * left * 0.5, -16, 0);
					G_ProjectSource (self->s.origin, v, v_forward, v_right, left_target);
				}
				VectorCopy (self->monsterinfo.last_sighting, self->monsterinfo.saved_goal);
				self->monsterinfo.aiflags |= AI_PURSUE_TEMP;
				VectorCopy (left_target, self->goalentity->s.origin);
				VectorCopy (left_target, self->monsterinfo.last_sighting);
				VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
				self->s.angles[YAW] = self->ideal_yaw = vectoyaw(v);
			}
			else if (right >= center && right > left)
			{
				if (right < 1)
				{
					VectorSet(v, d2 * right * 0.5, 16, 0);
					G_ProjectSource (self->s.origin, v, v_forward, v_right, right_target);
				}
				VectorCopy (self->monsterinfo.last_sighting, self->monsterinfo.saved_goal);
				self->monsterinfo.aiflags |= AI_PURSUE_TEMP;
				VectorCopy (right_target, self->goalentity->s.origin);
				VectorCopy (right_target, self->monsterinfo.last_sighting);
				VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
				self->s.angles[YAW] = self->ideal_yaw = vectoyaw(v);
			}
		}
	}

	M_MoveToGoal (self, dist);
	if(!self->inuse)
		return;

	G_FreeEdict(tempgoal);

	if (self)
		self->goalentity = save;
}

static int chase_angle[] = {270,450,225,495,540};
qboolean ai_chicken (edict_t *self, edict_t *badguy)
{
	int		i;
	edict_t	*thing;
	vec3_t	atk, dir, best_dir, end, forward;
	vec_t	travel, yaw;
	vec3_t	mins, maxs;
	vec3_t	testpos;
	vec_t	best_dist=0;
	trace_t	trace1, trace2;

	// No point in hiding from attacker if he's gone
	if(!badguy || !badguy->inuse)
		return false;

	if(!self || !self->inuse || (self->health <= 0))
		return false;

	if(!actorchicken->value)
		return false;

	// If we've already been here, quit
	if(self->monsterinfo.aiflags & AI_CHICKEN)
	{
		if(self->movetarget && !Q_stricmp(self->movetarget->classname,"thing"))
			return true;
	}

	VectorCopy(self->mins,mins);
	mins[2] += 18;
	if(mins[2] > 0) mins[2] = 0;
	VectorCopy(self->maxs,maxs);

	// Find a vector that will hide the actor from his enemy
	VectorCopy(badguy->s.origin,atk);
	atk[2] += badguy->viewheight;
	VectorClear(best_dir);
	AngleVectors(self->s.angles,forward,NULL,NULL);
	dir[2] = 0;
	for(travel=512; travel>63 && best_dist == 0; travel /= 2)
	{
		for(i=0; i<5 && best_dist == 0; i++)
		{
			yaw = self->s.angles[YAW] + chase_angle[i];
			yaw = (int)(yaw/45)*45;
			yaw = anglemod(yaw);
			yaw *= M_PI/180;
			dir[0] = cos(yaw);
			dir[1] = sin(yaw);
			VectorMA(self->s.origin,travel,dir,end);
			trace1 = gi.trace(self->s.origin,mins,maxs,end,self,MASK_MONSTERSOLID);
			// Test whether proposed position can be seen by badguy. Test
			// isn't foolproof - tests against 1) new origin, 2) new origin + maxs,
			// 3) new origin + mins, and 4) new origin + min x,y, max z.
			trace2 = gi.trace(trace1.endpos,NULL,NULL,atk,self,MASK_SOLID);
			if(trace2.fraction == 1.0) continue;

			VectorAdd(trace1.endpos,self->maxs,testpos);
			trace2 = gi.trace(testpos,NULL,NULL,atk,self,MASK_SOLID);
			if(trace2.fraction == 1.0) continue;

			VectorAdd(trace1.endpos,self->mins,testpos);
			trace2 = gi.trace(testpos,NULL,NULL,atk,self,MASK_SOLID);
			if(trace2.fraction == 1.0) continue;

			testpos[2] = trace1.endpos[2] + self->maxs[2];
			trace2 = gi.trace(testpos,NULL,NULL,atk,self,MASK_SOLID);
			if(trace2.fraction == 1.0) continue;

			best_dist = trace1.fraction * travel;
			if(best_dist < 32) // not much point to this move
				continue;
			VectorCopy(dir,best_dir);
		}
	}
	return false;

	if(best_dist < 32)
		return false;

	// This snaps the angles, which may not be all that good but it sure
	// is quicker than turning in place
	vectoangles(best_dir,self->s.angles);
	thing = SpawnThing();
	VectorMA(self->s.origin,best_dist,best_dir,thing->s.origin);
	thing->touch_debounce_time = level.time + 3.0;
	thing->target_ent = self;
	ED_CallSpawn(thing);
	self->movetarget = self->goalentity = thing;
	self->monsterinfo.aiflags &= ~(AI_SOUND_TARGET | AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
	self->monsterinfo.pausetime = 0;
	self->monsterinfo.aiflags |= (AI_CHASE_THING | AI_CHICKEN);
	gi.linkentity(self);
	self->monsterinfo.run(self);
	self->monsterinfo.chicken_framenum = level.framenum;
	return true;
}
