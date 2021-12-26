
#include "g_local.h"

//===============================
// BLOCKED Logic
//===============================

/*
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_DEBUGTRAIL);
		gi.WritePosition (pt1);
		gi.WritePosition (pt2);
		gi.multicast (pt1, MULTICAST_PVS);	

		self->nextthink = level.time + 10;
*/

// plat states, copied from g_func.c

#define	STATE_TOP			0
#define	STATE_BOTTOM		1
#define STATE_UP			2
#define STATE_DOWN			3

qboolean face_wall (edict_t *self);
void HuntTarget (edict_t *self);

// PMM
qboolean parasite_drain_attack_ok (vec3_t start, vec3_t end);


// blocked_checkshot
//	shotchance: 0-1, chance they'll take the shot if it's clear.
qboolean blocked_checkshot (edict_t *self, float shotChance)
{
	qboolean	playerVisible;

	if(!self->enemy)
		return false;

	// blocked checkshot is only against players. this will
	// filter out player sounds and other shit they should
	// not be firing at.
	if(!(self->enemy->client))
		return false;

	if (random() < shotChance)
		return false;

	// PMM - special handling for the parasite
	if (!strcmp(self->classname, "monster_parasite"))
	{
		vec3_t	f, r, offset, start, end;
		trace_t	tr;
		AngleVectors (self->s.angles, f, r, NULL);
		VectorSet (offset, 24, 0, 6);
		G_ProjectSource (self->s.origin, offset, f, r, start);

		VectorCopy (self->enemy->s.origin, end);
		if (!parasite_drain_attack_ok(start, end))
		{
			end[2] = self->enemy->s.origin[2] + self->enemy->maxs[2] - 8;
			if (!parasite_drain_attack_ok(start, end))
			{
				end[2] = self->enemy->s.origin[2] + self->enemy->mins[2] + 8;
				if (!parasite_drain_attack_ok(start, end))
					return false;
			}
		}
		VectorCopy (self->enemy->s.origin, end);

		tr = gi.trace (start, NULL, NULL, end, self, MASK_SHOT);
		if (tr.ent != self->enemy)
		{
			self->monsterinfo.aiflags |= AI_BLOCKED;
			
			if(self->monsterinfo.attack)
				self->monsterinfo.attack(self);
			
			self->monsterinfo.aiflags &= ~AI_BLOCKED;
			return true;
		}
	}

	playerVisible = visible (self, self->enemy);
	return false;
}

// blocked_checkplat
//	dist: how far they are trying to walk.
qboolean blocked_checkplat (edict_t *self, float dist)
{
	int			playerPosition;
	trace_t		trace;
	vec3_t		pt1, pt2;
	vec3_t		forward;
	edict_t		*plat;

	if(!self->enemy)
		return false;

	// check player's relative altitude
	if(self->enemy->absmin[2] >= self->absmax[2])
		playerPosition = 1;
	else if(self->enemy->absmax[2] <= self->absmin[2])
		playerPosition = -1;
	else
		playerPosition = 0;

	// if we're close to the same position, don't bother trying plats.
	if(playerPosition == 0)
		return false;

	plat = NULL;

	// see if we're already standing on a plat.
	if(self->groundentity && self->groundentity != world)
	{
		if(!strncmp(self->groundentity->classname, "func_plat", 8))
			plat = self->groundentity;
	}

	// if we're not, check to see if we'll step onto one with this move
	if(!plat)
	{
		AngleVectors (self->s.angles, forward, NULL, NULL);
		VectorMA(self->s.origin, dist, forward, pt1);
		VectorCopy (pt1, pt2);
		pt2[2] -= 384;

		trace = gi.trace(pt1, vec3_origin, vec3_origin, pt2, self, MASK_MONSTERSOLID);
		if(trace.fraction < 1 && !trace.allsolid && !trace.startsolid)
		{
			if(!strncmp(trace.ent->classname, "func_plat", 8))
			{
				plat = trace.ent;
			}
		}
	}

	// if we've found a plat, trigger it.
	if(plat && plat->use)
	{
		if (playerPosition == 1)
		{
			if((self->groundentity == plat && plat->moveinfo.state == STATE_BOTTOM) ||
				(self->groundentity != plat && plat->moveinfo.state == STATE_TOP))
			{
//				if(g_showlogic && g_showlogic->value)
//					gi.dprintf("player above, and plat will raise. using!\n");
				plat->use (plat, self, self);
				return true;			
			}
		}
		else if(playerPosition == -1)
		{
			if((self->groundentity == plat && plat->moveinfo.state == STATE_TOP) ||
				(self->groundentity != plat && plat->moveinfo.state == STATE_BOTTOM))
			{
//				if(g_showlogic && g_showlogic->value)
//					gi.dprintf("player below, and plat will lower. using!\n");
				plat->use (plat, self, self);
				return true;
			}
		}
//		if(g_showlogic && g_showlogic->value)
//			gi.dprintf("hit a plat, not using. ppos: %d   plat: %d\n", playerPosition, plat->moveinfo.state);
	}

	return false;
}

// blocked_checkjump
//	dist: how far they are trying to walk.
//  maxDown/maxUp: how far they'll ok a jump for. set to 0 to disable that direction.
qboolean blocked_checkjump (edict_t *self, float dist, float maxDown, float maxUp)
{
	int			playerPosition;
	trace_t		trace;
	vec3_t		pt1, pt2;
	vec3_t		forward, up;
	vec_t		d0, d1;
	edict_t		*target;

	// Lazarus: Rogue only did this for enemies. We do it for enemies or
	//          movetargets

	if(!self->monsterinfo.jump)
		return false;
	if(self->enemy)
		target = self->enemy;
	else if(self->movetarget)
		target = self->movetarget;
	else
		return false;

	VectorSubtract(target->s.origin,self->s.origin,pt1);
	d0 = VectorLength(pt1);

	AngleVectors (self->s.angles, forward, NULL, up);
	VectorMA(self->s.origin, 48, forward, pt1);

	if(target->absmin[2] > (self->absmin[2] + 16))
		playerPosition = 1;
	else if(target->absmin[2] < (self->absmin[2] - 16))
		playerPosition = -1;
	else
		playerPosition = 0;

	if(playerPosition == -1 && maxDown)
	{
		// check to make sure we can even get to the spot we're going to "fall" from
		trace = gi.trace(self->s.origin, self->mins, self->maxs, pt1, self, MASK_MONSTERSOLID);
		if(trace.fraction < 1)
			return false;

		VectorCopy (pt1, pt2);
		pt2[2] = self->mins[2] - maxDown - 1;

		trace = gi.trace(pt1, vec3_origin, vec3_origin, pt2, self, MASK_MONSTERSOLID | MASK_WATER);
		if(trace.fraction < 1 && !trace.allsolid && !trace.startsolid)
		{
			if((self->absmin[2] - trace.endpos[2]) >= 24 && trace.contents & MASK_SOLID)
			{
				if( (target->absmin[2] - trace.endpos[2]) > 32)
					return false;
				if(trace.plane.normal[2] < 0.9)
					return false;
				VectorSubtract(target->s.origin,trace.endpos,pt1);
				d1 = VectorLength(pt1);
				if(d0 < d1)
					return false;
				self->velocity[0] = forward[0]*dist*10;
				self->velocity[1] = forward[1]*dist*10;
				self->velocity[2] = max(self->velocity[2],100);
				gi.linkentity(self);
				return true;
			}
		}
	}
	else if(playerPosition == 1 && maxUp)
	{
		VectorCopy(pt1, pt2);
		pt1[2] = self->absmax[2] + maxUp;

		trace = gi.trace(pt1, vec3_origin, vec3_origin, pt2, self, MASK_MONSTERSOLID | MASK_WATER);
		if(trace.fraction < 1 && !trace.allsolid && !trace.startsolid)
		{
			if((trace.endpos[2] - self->absmin[2]) <= maxUp && trace.contents & MASK_SOLID)
			{
				VectorSubtract(target->s.origin,trace.endpos,pt1);
				d1 = VectorLength(pt1);
				if(d0 < d1)
					return false;
				face_wall(self);
				self->monsterinfo.jump(self);
				self->velocity[0] = forward[0]*dist*10;
				self->velocity[1] = forward[1]*dist*10;
				self->velocity[2] = max(self->velocity[2],200);
				gi.linkentity(self);
				return true;
			}
		}
	}
	return false;
}

// checks to see if another coop player is nearby, and will switch.
qboolean blocked_checknewenemy (edict_t *self)
{
/*
	int		player;
	edict_t *ent;

	if (!(coop->value))
		return false;

	for (player = 1; player <= game.maxclients; player++)
	{
		ent = &g_edicts[player];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;
		if (ent == self->enemy)
			continue;

		if (visible (self, ent))
		{
			if (g_showlogic && g_showlogic->value)
				gi.dprintf ("B_CNE: %s acquired new enemy %s\n", self->classname, ent->client->pers.netname);

			self->enemy = ent;
			FoundTarget (self);
			return true;
		}
	}

	return false;
*/
	return false;
}

// *************************
// HINT PATHS
// *************************

#define HINT_ENDPOINT		0x0001

int	hint_paths_present;
edict_t *hint_path_start[MAX_HINT_CHAINS];
int	num_hint_paths;

//
// AI code
//

// =============
// hintpath_findstart - given any hintpath node, finds the start node
// =============
edict_t	*hintpath_findstart(edict_t *ent)
{
	edict_t		*e;
	edict_t		*last;
	int			field;

	if(ent->target)		// starting point
	{
		last = world;
		field = FOFS(targetname);
		e = G_Find(NULL, field, ent->target);
		while(e)
		{
			last = e;
			if(!e->target)
				break;
			e = G_Find(NULL, field, e->target);
		}
	}
	else				// end point
	{
		last = world;
		field = FOFS(target);
		e = G_Find(NULL, field, ent->targetname);
		while(e)
		{
			last = e;
			if(!e->targetname)
				break;
			e = G_Find(NULL, field, e->targetname);
		}
	}

	if(!(last->spawnflags & HINT_ENDPOINT))
	{
//		gi.dprintf ("end of chain is not HINT_ENDPOINT\n");
		return NULL;
	}

	if(last == world)
		last = NULL;
	return last;
}

// =============
// hintpath_other_end - given one endpoint of a hintpath, returns the other end.
// =============
edict_t	*hintpath_other_end(edict_t *ent)
{
	edict_t		*e;
	edict_t		*last;
	int			field;

	if(ent->target)		// starting point
	{
		last = world;
		field = FOFS(targetname);
		e = G_Find(NULL, field, ent->target);
		while(e)
		{
			last = e;
			if(!e->target)
				break;
			e = G_Find(NULL, field, e->target);
		}
	}
	else				// end point
	{
		last = world;
		field = FOFS(target);
		e = G_Find(NULL, field, ent->targetname);
		while(e)
		{
			last = e;
			if(!e->targetname)
				break;
			e = G_Find(NULL, field, e->targetname);
		}
	}

	if(!(last->spawnflags & HINT_ENDPOINT))
	{
//		gi.dprintf ("end of chain is not HINT_ENDPOINT\n");
		return NULL;
	}

	if(last == world)
		last = NULL;
	return last;
}

// =============
// hintpath_go - starts a monster (self) moving towards the hintpath (point)
//		disables all contrary AI flags.
// =============
void hintpath_go (edict_t *self, edict_t *point)
{
	vec3_t	dir;
	vec3_t	angles;

	VectorSubtract(point->s.origin, self->s.origin, dir);
	vectoangles2(dir, angles);

	self->ideal_yaw = angles[YAW];
	self->goalentity = self->movetarget = point;
	self->monsterinfo.pausetime = 0;
	self->monsterinfo.aiflags |= AI_HINT_PATH;
	self->monsterinfo.aiflags &= ~(AI_SOUND_TARGET | AI_PURSUIT_LAST_SEEN | AI_PURSUE_NEXT | AI_PURSUE_TEMP);
	// run for it
	self->monsterinfo.search_time = level.time;
	self->monsterinfo.run (self);
}

// =============
// hintpath_stop - bails a monster out of following hint paths
// =============
void hintpath_stop (edict_t *self)
{
	self->goalentity = NULL;
	self->movetarget = NULL;
//	self->monsterinfo.last_hint = NULL;
	self->monsterinfo.last_hint_time = level.time;
	self->monsterinfo.goal_hint = NULL;
	self->monsterinfo.aiflags &= ~AI_HINT_PATH;
	if (has_valid_enemy(self))
	{
		// if we can see our target, go nuts
		if (visible(self, self->enemy))
		{
			FoundTarget (self);
			return;
		}
		// otherwise, keep chasing
		HuntTarget (self);
		return;
	}
	// if our enemy is no longer valid, forget about our enemy and go into stand
	self->enemy = NULL;
		// we need the pausetime otherwise the stand code
		// will just revert to walking with no target and
		// the monsters will wonder around aimlessly trying
		// to hunt the world entity
	self->monsterinfo.pausetime = level.time + 100000000;
	self->monsterinfo.stand (self);
}

// =============
// monsterlost_checkhint - the monster (self) will check around for valid hintpaths.
//		a valid hintpath is one where the two endpoints can see both the monster
//		and the monster's enemy. if only one person is visible from the endpoints,
//		it will not go for it.
// =============
qboolean monsterlost_checkhint2 (edict_t *self);

qboolean monsterlost_checkhint (edict_t *self)
{
	edict_t		*e, *monster_pathchain, *target_pathchain, *checkpoint;
	edict_t		*closest;
	float		closest_range = 1000000;
	edict_t		*start, *destination;
	int			field;
	int			count1=0, count2=0, count3=0, count4=0, count5=0;
	float		r;
	int			i;
	qboolean	hint_path_represented[MAX_HINT_CHAINS];

	// if there are no hint paths on this map, exit immediately.
	if (!hint_paths_present)
		return false;

//CW+++ Don't look for hintpaths if Mr. Mapping Dude doesn't want me to.
	if (self->spawnflags & SF_MONSTER_NOHINT)
		return false;
//CW---

	if (!self->enemy)
		return false;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return false;
	
	if (!strcmp(self->classname, "monster_turret"))
		return false;

	monster_pathchain = NULL;

	field = FOFS(classname);

	// find all the hint_paths.
	// FIXME - can we not do this every time?
	for (i=0; i < num_hint_paths; i++)
	{
		e = hint_path_start[i];
		while(e)
		{
			count1++;
			if (e->monster_hint_chain)
				e->monster_hint_chain = NULL;
			if (monster_pathchain)
			{
				checkpoint->monster_hint_chain = e;
				checkpoint = e;
			}
			else
			{
				monster_pathchain = e;
				checkpoint = e;
			}
			e = e->hint_chain;
		}
	}
	
	// filter them by distance and visibility to the monster
	e = monster_pathchain;
	checkpoint = NULL;
	while (e)
	{
		r = realrange (self, e);

//		if (r > 512)
//			count3++;

		if (r > 512)
		{
			count2++;
			if (checkpoint)
			{
				checkpoint->monster_hint_chain = e->monster_hint_chain;
				e->monster_hint_chain = NULL;
				e = checkpoint->monster_hint_chain;
				continue;
			}
			else
			{
				// use checkpoint as temp pointer
				checkpoint = e;
				e = e->monster_hint_chain;
				checkpoint->monster_hint_chain = NULL;
				// and clear it again
				checkpoint = NULL;
				// since we have yet to find a valid one (or else checkpoint would be set) move the
				// start of monster_pathchain
				monster_pathchain = e;
				continue;
			}
		}
		if (!visible(self, e))
		{
			count4++;
			if (checkpoint)
			{
				checkpoint->monster_hint_chain = e->monster_hint_chain;
				e->monster_hint_chain = NULL;
				e = checkpoint->monster_hint_chain;
				continue;
			}
			else
			{
				// use checkpoint as temp pointer
				checkpoint = e;
				e = e->monster_hint_chain;
				checkpoint->monster_hint_chain = NULL;
				// and clear it again
				checkpoint = NULL;
				// since we have yet to find a valid one (or else checkpoint would be set) move the
				// start of monster_pathchain
				monster_pathchain = e;
				continue;
			}
		}
		count5++;
		checkpoint = e;
		e = e->monster_hint_chain;
	}

	// at this point, we have a list of all of the eligible hint nodes for the monster
	// we now take them, figure out what hint chains they're on, and traverse down those chains,
	// seeing whether any can see the player
	//
	// first, we figure out which hint chains we have represented in monster_pathchain
	if (count5 == 0)
		return false;

	for (i=0; i < num_hint_paths; i++)
	{
		hint_path_represented[i] = false;
	}
	e = monster_pathchain;
	checkpoint = NULL;
	while (e)
	{
		if ((e->hint_chain_id < 0) || (e->hint_chain_id > num_hint_paths))
			return false;
		hint_path_represented[e->hint_chain_id] = true;
		e = e->monster_hint_chain;
	}

	count1 = 0;
	count2 = 0;
	count3 = 0;
	count4 = 0;
	count5 = 0;

	// now, build the target_pathchain which contains all of the hint_path nodes we need to check for
	// validity (within range, visibility)
	target_pathchain = NULL;
	checkpoint = NULL;
	for (i=0; i < num_hint_paths; i++)
	{
		// if this hint chain is represented in the monster_hint_chain, add all of it's nodes to the target_pathchain
		// for validity checking
		if (hint_path_represented[i])
		{
			e = hint_path_start[i];
			while (e)
			{
				if (target_pathchain)
				{
					checkpoint->target_hint_chain = e;
					checkpoint = e;
				}
				else
				{
					target_pathchain = e;
					checkpoint = e;
				}
				e = e->hint_chain;
			}
		}
	}

	// target_pathchain is a list of all of the hint_path nodes we need to check for validity relative to the target
	e = target_pathchain;
	checkpoint = NULL;
	while (e)
	{
		r = realrange (self->enemy, e);

		if (r > 512)
		{
			count2++;
			if (checkpoint)
			{
				checkpoint->target_hint_chain = e->target_hint_chain;
				e->target_hint_chain = NULL;
				e = checkpoint->target_hint_chain;
				continue;
			}
			else
			{
				// use checkpoint as temp pointer
				checkpoint = e;
				e = e->target_hint_chain;
				checkpoint->target_hint_chain = NULL;
				// and clear it again
				checkpoint = NULL;
				target_pathchain = e;
				continue;
			}
		}
		if (!visible(self->enemy, e))
		{
			count4++;
			if (checkpoint)
			{
				checkpoint->target_hint_chain = e->target_hint_chain;
				e->target_hint_chain = NULL;
				e = checkpoint->target_hint_chain;
				continue;
			}
			else
			{
				// use checkpoint as temp pointer
				checkpoint = e;
				e = e->target_hint_chain;
				checkpoint->target_hint_chain = NULL;
				// and clear it again
				checkpoint = NULL;
				target_pathchain = e;
				continue;
			}
		}
		// if it passes all the tests, it's a keeper
		count5++;
		checkpoint = e;
		e = e->target_hint_chain;
	}
	
	// at this point we should have:
	// monster_pathchain - a list of "monster valid" hint_path nodes linked together by monster_hint_chain
	// target_pathcain - a list of "target valid" hint_path nodes linked together by target_hint_chain.  these
	//                   are filtered such that only nodes which are on the same chain as "monster valid" nodes
	//
	// Now, we figure out which "monster valid" node we want to use
	// 
	// To do this, we first off make sure we have some target nodes.  If we don't, there are no valid hint_path nodes
	// for us to take
	//
	// If we have some, we filter all of our "monster valid" nodes by which ones have "target valid" nodes on them
	//
	// Once this filter is finished, we select the closest "monster valid" node, and go to it.

	if (count5 == 0)
		return false;

	// reuse the hint_chain_represented array, this time to see which chains are represented by the target
	for (i=0; i < num_hint_paths; i++)
	{
		hint_path_represented[i] = false;
	}

	e = target_pathchain;
	checkpoint = NULL;
	while (e)
	{
		if ((e->hint_chain_id < 0) || (e->hint_chain_id > num_hint_paths))
			return false;
		hint_path_represented[e->hint_chain_id] = true;
		e = e->target_hint_chain;
	}
	
	// traverse the monster_pathchain - if the hint_node isn't represented in the "target valid" chain list, 
	// remove it
	// if it is on the list, check it for range from the monster.  If the range is the closest, keep it
	//
	closest = NULL;
	e = monster_pathchain;
	while (e)
	{
		if (!(hint_path_represented[e->hint_chain_id]))
		{
			checkpoint = e->monster_hint_chain;
			e->monster_hint_chain = NULL;
			e = checkpoint;
			continue;
		}
		r = realrange(self, e);
		if (r < closest_range) {
			closest = e;
			closest_range = r;  // Lazarus: This was left out, ya doofuses
		}
		e = e->monster_hint_chain;
	}

	if (!closest)
		return false;

	start = closest;
	// now we know which one is the closest to the monster .. this is the one the monster will go to
	// we need to finally determine what the DESTINATION node is for the monster .. walk down the hint_chain,
	// and find the closest one to the player

	closest = NULL;
	closest_range = 10000000;
	e = target_pathchain;
	while (e)
	{
		if (start->hint_chain_id == e->hint_chain_id)
		{
			r = realrange(self->enemy, e);	// Lazarus: This WAS realrange(self, e)... that can't be right
			if (r < closest_range) {
				closest = e;
				closest_range = r;	// Lazarus: again, you idjits left this out
			}
		}
		e = e->target_hint_chain;
	}

	if (!closest)
		return false;
	
	destination = closest;

	self->monsterinfo.goal_hint = destination;
//	self->monsterinfo.last_hint = NULL;
	hintpath_go(self, start);

	return true;
}
//
// Path code
//

// =============
// hint_path_touch - someone's touched the hint_path
// =============
void hint_path_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t		*e, *goal, *next;
//	int			chain;			 // direction - (-1) = upstream, (1) = downstream, (0) = done
	qboolean	goalFound = false;

	if(other->monsterinfo.aiflags & AI_MEDIC_PATROL) 
	{
		if(other->movetarget == self)
			medic_NextPatrolPoint(other,self);
		return;
	}

	if(other->monsterinfo.aiflags & AI_HINT_TEST)
	{
		if(other->movetarget == self)
			HintTestNext(other,self);
		return;
	}

	// make sure we're the target of its obsession
	if(other->movetarget == self)
	{
		goal = other->monsterinfo.goal_hint;
		
		// if the monster is where he wants to be
		if (goal == self)
		{
			hintpath_stop (other);
			return;
		}
		else
		{
			// if we aren't, figure out which way we want to go
			e = hint_path_start[self->hint_chain_id];
			while (e)
			{
				// if we get up to ourselves on the hint chain, we're going down it
				if (e == self)
				{
					next = e->hint_chain;
					break;
				}
				if (e == goal)
					goalFound = true;
				// if we get to where the next link on the chain is this hint_path and have found the goal on the way
				// we're going upstream, so remember who the previous link is
				if ((e->hint_chain == self) && goalFound)
				{
					next = e;
					break;
				}
				e = e->hint_chain;
			}
		}

		// if we couldn't find it, have the monster go back to normal hunting.
		if(!next)
		{
			hintpath_stop(other);
			return;
		}

		// set the last_hint entry to this hint_path, and
		// send him on his way
		hintpath_go(other, next);

		// have the monster freeze if the hint path we just touched has a wait time
		// on it, for example, when riding a plat.
		if(self->wait)
			other->nextthink = level.time + self->wait;
	}
}

/*QUAKED hint_path (.5 .3 0) (-8 -8 -8) (8 8 8) END
Target: next hint path

END - set this flag on the endpoints of each hintpath.

"wait" - set this if you want the monster to freeze when they touch this hintpath
*/
void SP_hint_path (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	if (!self->targetname && !self->target)
	{
		gi.dprintf ("unlinked hint_path at %s\n", vtos(self->s.origin));
		G_FreeEdict (self);
		return;
	}

	// Lazarus: Corrections for mappers that can't follow instructions :-)
	if (!self->targetname)
		self->spawnflags |= HINT_ENDPOINT;
	if (!self->target)
		self->spawnflags |= HINT_ENDPOINT;

	self->solid = SOLID_TRIGGER;
	self->touch = hint_path_touch;
	VectorSet (self->mins, -8, -8, -8);
	VectorSet (self->maxs, 8, 8, 8);
	self->svflags |= SVF_NOCLIENT;
	gi.linkentity (self);
}

//int	hint_paths_present;
//edict_t *hint_path_start[100];
//int	num_hint_paths;

// ============
// InitHintPaths - Called by InitGame (g_save) to enable quick exits if valid
// ============
void InitHintPaths (void)
{
	edict_t		*e, *current;
	int			field, i, count2;
	qboolean	errors = false;

	hint_paths_present = 0;
	
	// check all the hint_paths.
	field = FOFS(classname);
	e = G_Find(NULL, field, "hint_path");
	if(e)
		hint_paths_present = 1;
	else
		return;

	memset (hint_path_start, 0, MAX_HINT_CHAINS*sizeof (edict_t *));
	num_hint_paths = 0;
	while(e)
	{
		if(e->spawnflags & HINT_ENDPOINT)
		{
			if (e->target) // start point
			{
				if (e->targetname) // this is a bad end, ignore it
				{
					gi.dprintf ("Hint path at %s marked as endpoint with both target (%s) and targetname (%s)\n",
						vtos (e->s.origin), e->target, e->targetname);
					errors = true;
				}
				else
				{
					if (num_hint_paths >= MAX_HINT_CHAINS)
					{
//						gi.dprintf ("Only %d hint chains allowed.  Connect some together!\n", MAX_HINT_CHAINS);
						break;
					}
					hint_path_start[num_hint_paths++] = e;
				}
			}
		}
		e = G_Find(e, field, "hint_path");
	}

	field = FOFS(targetname);
	for (i=0; i< num_hint_paths; i++)
	{
		count2 = 1;
		current = hint_path_start[i];
		current->hint_chain_id = i;
		e = G_Find(NULL, field, current->target);
		if (G_Find(e, field, current->target))
		{
			gi.dprintf ("\nForked hint path at %s detected for chain %d, target %s\n", 
				vtos (current->s.origin), num_hint_paths, current->target);
			hint_path_start[i]->hint_chain = NULL;
			count2 = 0;
			errors = true;
			continue;
		}
		while (e)
		{
			if (e->hint_chain)
			{
				gi.dprintf ("\nCircular hint path at %s detected for chain %d, targetname %s\n", 
					vtos (e->s.origin), num_hint_paths, e->targetname);
				hint_path_start[i]->hint_chain = NULL;
				count2 = 0;
				errors = true;
				break;
			}
			count2++;
			current->hint_chain = e;
			current = e;
			current->hint_chain_id = i;
			if (!current->target)
				break;
			e = G_Find(NULL, field, current->target);
			if (G_Find(e, field, current->target))
			{
				gi.dprintf ("\nForked hint path at %s detected for chain %d, target %s\n", 
					vtos (current->s.origin), num_hint_paths, current->target);
				hint_path_start[i]->hint_chain = NULL;
				count2 = 0;
				break;
			}
		}
	}
}

// *****************************
//	MISCELLANEOUS STUFF
// *****************************

// PMM - inback
// use to see if opponent is behind you (not to side)
// if it looks a lot like infront, well, there's a reason

qboolean inback (edict_t *self, edict_t *other)
{
	vec3_t	vec;
	float	dot;
	vec3_t	forward;
	
	AngleVectors (self->s.angles, forward, NULL, NULL);
	VectorSubtract (other->s.origin, self->s.origin, vec);
	VectorNormalize (vec);
	dot = DotProduct (vec, forward);
	
	if (dot < -0.3)
		return true;
	return false;
}

float realrange (edict_t *self, edict_t *other)
{
	vec3_t dir;
	
	VectorSubtract (self->s.origin, other->s.origin, dir);
	return VectorLength(dir);
}

qboolean face_wall (edict_t *self)
{
	vec3_t	pt;
	vec3_t	forward;
	vec3_t	ang;
	trace_t	tr;

	AngleVectors (self->s.angles, forward, NULL, NULL);
	VectorMA(self->s.origin, 64, forward, pt);
	tr = gi.trace(self->s.origin, vec3_origin, vec3_origin, pt, self, MASK_MONSTERSOLID);
	if(tr.fraction < 1 && !tr.allsolid && !tr.startsolid)
	{
		vectoangles2(tr.plane.normal, ang);
		self->ideal_yaw = ang[YAW] + 180;
		if(self->ideal_yaw > 360)
			self->ideal_yaw -= 360;

		M_ChangeYaw(self);
		return true;
	}

	return false;
}

//
// Monster "Bad" Areas
// 

void badarea_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
//	drawbbox(ent);
}

edict_t *SpawnBadArea(vec3_t mins, vec3_t maxs, float lifespan, edict_t *owner)
{
	edict_t *badarea;
	vec3_t	origin;
	
	VectorAdd(mins, maxs, origin);
	VectorScale(origin, 0.5, origin);

	VectorSubtract(maxs, origin, maxs);
	VectorSubtract(mins, origin, mins);

	badarea = G_Spawn();
	VectorCopy(origin, badarea->s.origin);
	VectorCopy(maxs, badarea->maxs);
	VectorCopy(mins, badarea->mins);
	badarea->touch = badarea_touch;
	badarea->movetype = MOVETYPE_NONE;
	badarea->solid = SOLID_TRIGGER;
	badarea->classname = "bad_area";
	gi.linkentity (badarea);

	if(lifespan)
	{
		badarea->think = G_FreeEdict;
		badarea->nextthink = level.time + lifespan;
	}
	if(owner)
	{
		badarea->owner = owner;
	}

//	drawbbox(badarea);
	return badarea;
}

// CheckForBadArea
//		This is a customized version of G_TouchTriggers that will check
//		for bad area triggers and return them if they're touched.
edict_t *CheckForBadArea(edict_t *ent)
{
	int			i, num;
	edict_t		*touch[MAX_EDICTS], *hit;
	vec3_t		mins, maxs;

	VectorAdd(ent->s.origin, ent->mins, mins);
	VectorAdd(ent->s.origin, ent->maxs, maxs);

	num = gi.BoxEdicts (mins, maxs, touch, MAX_EDICTS, AREA_TRIGGERS);

//	drawbbox(ent);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i=0 ; i<num ; i++)
	{
		hit = touch[i];
		if (!hit->inuse)
			continue;
		if (hit->touch == badarea_touch)
		{
			return hit;
		}
	}
	
	return NULL;
}

// predictive calculator
// target is who you want to shoot
// start is where the shot comes from
// bolt_speed is how fast the shot is
// eye_height is a boolean to say whether or not to adjust to targets eye_height
// offset is how much time to miss by
// aimdir is the resulting aim direction (pass in NULL if you don't want it)
// aimpoint is the resulting aimpoint (pass in NULL if don't want it)
void PredictAim (edict_t *target, vec3_t start, float bolt_speed, qboolean eye_height, float offset, vec3_t aimdir, vec3_t aimpoint)
{
	vec3_t dir, vec;
	float dist, time;

	if (!target || !target->inuse)
	{
		VectorCopy (vec3_origin, aimdir);
		return;
	}

	VectorSubtract(target->s.origin, start, dir);
	if (eye_height)
		dir[2] += target->viewheight;
	dist = VectorLength(dir);
	time = dist / bolt_speed;


	VectorMA(target->s.origin, time - offset, target->velocity, vec);

	if (eye_height)
		vec[2] += target->viewheight;

	if (aimdir)
	{
		VectorSubtract (vec, start, aimdir);
		VectorNormalize (aimdir);
	}
	
	if (aimpoint)
	{
		VectorCopy (vec, aimpoint);
	}
}


qboolean below (edict_t *self, edict_t *other)
{
	vec3_t	vec;
	float	dot;
	vec3_t	down;
	
	VectorSubtract (other->s.origin, self->s.origin, vec);
	VectorNormalize (vec);
	VectorSet (down, 0, 0, -1);
	dot = DotProduct (vec, down);
	
	if (dot > 0.95)  // 18 degree arc below
		return true;
	return false;
}

void drawbbox (edict_t *self)
{
	int	lines[4][3] = {
		{1, 2, 4},
		{1, 2, 7},
		{1, 4, 5},
		{2, 4, 7}
	};

	int starts[4] = {0, 3, 5, 6};

	vec3_t pt[8];
	int i, j, k;
	vec3_t coords[2];
	vec3_t newbox;
	vec3_t f,r,u, dir;

	VectorCopy (self->absmin, coords[0]);
	VectorCopy (self->absmax, coords[1]);

	for (i=0; i<=1; i++)
	{
		for (j=0; j<=1; j++)
		{
			for (k=0; k<=1; k++)
			{
				pt[4*i+2*j+k][0] = coords[i][0];
				pt[4*i+2*j+k][1] = coords[j][1];
				pt[4*i+2*j+k][2] = coords[k][2];
			}
		}
	}
	
	for (i=0; i<= 3; i++)
	{
		for (j=0; j<= 2; j++)
		{
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_DEBUGTRAIL);
			gi.WritePosition (pt[starts[i]]);
			gi.WritePosition (pt[lines[i][j]]);
			gi.multicast (pt[starts[i]], MULTICAST_ALL);	
		}
	}

	vectoangles2 (self->s.angles, dir);
	AngleVectors (dir, f, r, u);

	VectorMA (self->s.origin, 50, f, newbox);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (self->s.origin);
	gi.WritePosition (newbox);
	gi.multicast (self->s.origin, MULTICAST_PVS);	
	VectorClear (newbox);

	VectorMA (self->s.origin, 50, r, newbox);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (self->s.origin);
	gi.WritePosition (newbox);
	gi.multicast (self->s.origin, MULTICAST_PVS);	
	VectorClear (newbox);

	VectorMA (self->s.origin, 50, u, newbox);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (self->s.origin);
	gi.WritePosition (newbox);
	gi.multicast (self->s.origin, MULTICAST_PVS);	
	VectorClear (newbox);
}

//=========================
//=========================
qboolean has_valid_enemy (edict_t *self)
{
	if (!self->enemy)
		return false;

	if (!self->enemy->inuse)
		return false;

	// Lazarus: this doesn't take into account medics pursuing dead monsters
//	if (self->enemy->health < 1)
//		return false;
	if (self->monsterinfo.aiflags & AI_MEDIC)
	{
		if(self->enemy->health > 0)
			return false;
	}
	else if(self->enemy->health < 1)
		return false;

	return true;
}

// this returns a randomly selected coop player who is visible to self
// returns NULL if bad

edict_t * PickCoopTarget (edict_t *self)
{
	// no more than 4 players in coop, so..
	edict_t *targets[4];
	int		num_targets = 0, targetID;
	edict_t *ent;
	int		player;

	// if we're not in coop, this is a noop
	if (!coop || !coop->value)
		return NULL;

	memset (targets, 0, 4*sizeof(edict_t *));

	for (player = 1; player <= game.maxclients; player++)
	{
		ent = &g_edicts[player];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;
		if (visible(self, ent))
			targets[num_targets++] = ent;
	}

	if (!num_targets)
		return NULL;

	// get a number from 0 to (num_targets-1)
	targetID = (random() * (float)num_targets);
	
	// just in case we got a 1.0 from random
	if (targetID == num_targets)
		targetID--;

	return targets[targetID];
}

// only meant to be used in coop
int CountPlayers (void)
{
	edict_t *ent;
	int		count = 0;
	int		player;

	// if we're not in coop, this is a noop
	if (!coop || !coop->value)
		return 1;

	for (player = 1; player <= game.maxclients; player++)
	{
		ent = &g_edicts[player];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;
		count++;
	}
/*
	ent = g_edicts+1; // skip the worldspawn
	while (ent)
	{
		if ((ent->client) && (ent->inuse))
		{
			ent++;
			count++;
		}
		else
			ent = NULL;
	}
*/
	return count;
}

//*******************
// JUMPING AIDS
//*******************

void monster_jump_start (edict_t *self)
{
	self->timestamp = level.time;
}

qboolean monster_jump_finished (edict_t *self)
{
	if ((level.time - self->timestamp) > 3)
		return true;
// Lazarus: Ummm? I suppose this should return false here, though this
//          line was not included in Rogue's code
	return false;
}



