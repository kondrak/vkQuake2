/*
==============================================================================

MEDIC

==============================================================================
*/

#include "g_local.h"
#include "m_medic.h"

//CW+++
#define BOLT_BASESPEED		800
#define BOLT_SKILLSPEED		100
#define AIM_R_BASE			1500
#define AIM_R_SKILL			250
//CW---

qboolean visible (edict_t *self, edict_t *other);

int			medic_test=0;
static int	sound_idle1;
static int	sound_pain1;
static int	sound_pain2;
static int	sound_die;
static int	sound_sight;
static int	sound_search;
static int	sound_hook_launch;
static int	sound_hook_hit;
static int	sound_hook_heal;
static int	sound_hook_retract;

void M_SetEffects (edict_t *ent);

void deadmonster_think (edict_t *self)
{
	// Lazarus. This turns off the "owner" value of a dead monster
	// that medic previously aborted on after 2 seconds. If the delay
	// was NOT used, medic might just rotate around on top of monster
	// continuously trying to revive him unsuccessfully. And if owner
	// isn't turned off, medic would never try to revive the monster
	// once an abortHeal was called.
	if (self->target_ent && self->target_ent->inuse) {
		edict_t	*deadmonster = self->target_ent;
		if (deadmonster->monsterinfo.healer) {
			edict_t *medic = deadmonster->monsterinfo.healer;
			if (medic->inuse) {
				vec3_t	dir;
				VectorSubtract(medic->s.origin,deadmonster->s.origin,dir);
				if (VectorLength(dir) < 64) {
					self->nextthink = level.time + 1.0;
					return;
				}
			} else
				deadmonster->monsterinfo.healer = NULL;
		}
		deadmonster->owner = NULL;
	}
	G_FreeEdict(self);
}

void cleanupHeal (edict_t *self, qboolean change_frame)
{
	// clean up target, if we have one and it's legit
	if (self->enemy && self->enemy->inuse)
	{
		edict_t	*temp;	// Lazarus

		//self->enemy->monsterinfo.healer = NULL;
		self->enemy->monsterinfo.aiflags &= ~AI_RESURRECTING;
		self->enemy->takedamage = DAMAGE_AIM;
		// Lazarus
		temp = G_Spawn();
		temp->svflags = SVF_NOCLIENT;
		temp->target_ent = self->enemy;
		temp->think = deadmonster_think;
		temp->nextthink = level.time + 2.0;
		gi.linkentity(temp);
		M_SetEffects (self->enemy);
	}

	if (change_frame)
		self->monsterinfo.nextframe = FRAME_attack52;
}

void DeleteBadMedic(edict_t *self)
{
	edict_t	*monster;

	monster = self->activator;
	if (monster)
	{
		if (self->monsterinfo.badMedic1)
			monster->monsterinfo.badMedic1 = NULL;
		if (self->monsterinfo.badMedic2)
			monster->monsterinfo.badMedic2 = NULL;
	}
	G_FreeEdict(self);
}
void abortHeal (edict_t *self, qboolean mark)
{
	edict_t	*temp;

	// clean up target
	cleanupHeal (self, true);

	if ((mark) && (self->enemy) && (self->enemy->inuse))
	{
		if ((self->enemy->monsterinfo.badMedic1) && (self->enemy->monsterinfo.badMedic1->inuse)
			&& (!strncmp(self->enemy->monsterinfo.badMedic1->classname, "monster_medic", 13)) )
			self->enemy->monsterinfo.badMedic2 = self;
		else
			self->enemy->monsterinfo.badMedic1 = self;

		temp = G_Spawn();
		temp->activator = self->enemy;
		if (self == self->enemy->monsterinfo.badMedic1)
			temp->monsterinfo.badMedic1 = self;
		else
			temp->monsterinfo.badMedic2 = self;
		temp->think = DeleteBadMedic;
		temp->nextthink = level.time + 60;
	}

	// clean up self
	self->monsterinfo.aiflags &= ~AI_MEDIC;
	if ((self->oldenemy) && (self->oldenemy->inuse))
		self->enemy = self->oldenemy;
	else
		self->enemy = NULL;

	self->monsterinfo.medicTries = 0;
}

// Lazarus: embedded returns true if argument entity's bounding box intersects
//          a solid.
qboolean embedded (edict_t *ent)
{
	trace_t	tr;

	tr = gi.trace(ent->s.origin,ent->mins,ent->maxs,ent->s.origin,ent,MASK_MONSTERSOLID);
	if (tr.startsolid)
		return true;
	else
		return false;
}

edict_t *medic_FindDeadMonster (edict_t *self)
{
	edict_t	*ent = NULL;
	edict_t	*best = NULL;

	while ((ent = findradius(ent, self->s.origin, 1024)) != NULL)
	{
		if (ent == self)
			continue;
		if (!(ent->svflags & SVF_MONSTER))
			continue;
		if (ent->monsterinfo.aiflags & AI_GOOD_GUY)
			continue;
		if (ent->owner)
			continue;
		if (ent->health > 0)
			continue;
		if (ent->nextthink && (ent->think != M_FliesOff) && (ent->think != M_FliesOn))
			continue;
		// check to make sure we haven't bailed on this guy already
		if ((ent->monsterinfo.badMedic1 == self) || (ent->monsterinfo.badMedic2 == self))
			continue;
		if (!visible(self, ent))
			continue;
		if (embedded(ent))
			continue;
		if (!canReach(self,ent))
			continue;
		if (!best)
		{
			best = ent;
			continue;
		}
		if (ent->max_health <= best->max_health)
			continue;
		best = ent;
	}

	if (best)
	{
		self->oldenemy = self->enemy;
		self->enemy = best;
		self->enemy->owner = best;
		self->monsterinfo.aiflags |= AI_MEDIC;
		self->monsterinfo.aiflags &= ~AI_MEDIC_PATROL;
		self->monsterinfo.medicTries = 0;
		self->movetarget = self->goalentity = NULL;
		self->enemy->monsterinfo.healer = self;
		self->timestamp = level.time + MEDIC_TRY_TIME;
		FoundTarget (self);

		if (developer->value)
			gi.dprintf("medic found dead monster: %s at %s\n",
			best->classname,vtos(best->s.origin));

	}
	return best;
}

void medic_StopPatrolling (edict_t *self)
{
	self->goalentity = NULL;
	self->movetarget = NULL;
	self->monsterinfo.aiflags &= ~AI_MEDIC_PATROL;
	if (!(self->monsterinfo.aiflags & AI_MEDIC))
	{
		if (medic_FindDeadMonster(self))
			return;
	}
	if (has_valid_enemy(self))
	{
		if (visible(self, self->enemy))
		{
			FoundTarget (self);
			return;
		}
		HuntTarget (self);
		return;
	}
	if (self->monsterinfo.aiflags & AI_MEDIC)
		abortHeal(self,false);
}

void medic_NextPatrolPoint (edict_t *self, edict_t *hint)
{
	edict_t		*next=NULL;
	edict_t		*e;
	vec3_t		dir;
	qboolean	switch_paths=false;

	self->monsterinfo.aiflags &= ~AI_MEDIC_PATROL;

//	if (self->monsterinfo.aiflags & AI_MEDIC)
//		return;

	if (self->goalentity == hint)
		self->goalentity = NULL;
	if (self->movetarget == hint)
		self->movetarget = NULL;
	if (!(self->monsterinfo.aiflags & AI_MEDIC))
	{
		if (medic_FindDeadMonster(self))
			return;
	}
	if (self->monsterinfo.pathdir == 1)
	{
		if (hint->hint_chain)
			next = hint->hint_chain;
		else
		{
			self->monsterinfo.pathdir = -1;
			switch_paths = true;
		}
	}
	if (self->monsterinfo.pathdir == -1)
	{
		e = hint_path_start[hint->hint_chain_id];
		while(e)
		{
			if (e->hint_chain == hint)
			{
				next = e;
				break;
			}
			e = e->hint_chain;
		}
	}
	if (!next)
	{
		self->monsterinfo.pathdir = 1;
		next = hint->hint_chain;
		switch_paths = true;
	}
	// If switch_paths is true, we reached the end of a hint_chain. Just for grins,
	// search for *another* visible hint_path chain and use it if it's reasonably close
	if (switch_paths && num_hint_paths > 1)
	{
		edict_t	*e;
		edict_t	*alternate=NULL;
		float	dist;
		vec3_t	dir;
		int		i;
		float	bestdistance=512;

		for(i=game.maxclients+1; i<globals.num_edicts; i++)
		{
			e = &g_edicts[i];
			if (!e->inuse)
				continue;
			if (Q_stricmp(e->classname,"hint_path"))
				continue;
			if (next && (e->hint_chain_id == next->hint_chain_id))
				continue;
			if (!visible(self,e))
				continue;
			if (!canReach(self,e))
				continue;
			VectorSubtract(e->s.origin,self->s.origin,dir);
			dist = VectorLength(dir);
			if (dist < bestdistance)
			{
				alternate = e;
				bestdistance = dist;
			}
		}
		if (alternate)
			next = alternate;
	}
	if (next)
	{
		self->hint_chain_id = next->hint_chain_id;
		VectorSubtract(next->s.origin, self->s.origin, dir);
		self->ideal_yaw = vectoyaw(dir);
		self->goalentity = self->movetarget = next;
		self->monsterinfo.pausetime = 0;
		self->monsterinfo.aiflags |= AI_MEDIC_PATROL;
		self->monsterinfo.aiflags &= ~(AI_SOUND_TARGET | AI_PURSUIT_LAST_SEEN | AI_PURSUE_NEXT | AI_PURSUE_TEMP);
		// run for it
		self->monsterinfo.run (self);
	}
	else
	{
		self->monsterinfo.pausetime = level.time + 100000000;
		self->monsterinfo.stand (self);
	}
}

void medic_idle (edict_t *self)
{
	if (!(self->spawnflags & SF_MONSTER_AMBUSH))
		gi.sound (self, CHAN_VOICE, sound_idle1, 1, ATTN_IDLE, 0);

	if (self->monsterinfo.aiflags & AI_MEDIC)
	{
		// Then we must have reached this point after losing sight
		// of our patient.
		abortHeal(self,false);
	}

	if (medic_FindDeadMonster(self))
		return;

	// If the map has hint_paths, AND the medic isn't at a HOLD point_combat,
	// AND the medic has previously called FoundTarget (trail_time set to
	// level.time), then look for hint_path chain and follow it, hopefully
	// to find monsters to resurrect
	if (self->monsterinfo.aiflags & AI_HINT_TEST)
		return;

	if (hint_paths_present && !(self->monsterinfo.aiflags & AI_STAND_GROUND)
		&& ((self->monsterinfo.trail_time > 0) || medic_test) )
	{
		edict_t	*e;
		edict_t	*hint=NULL;
		float	dist;
		vec3_t	dir;
		int		i;
		float	bestdistance=99999;

		for(i=game.maxclients+1; i<globals.num_edicts; i++)
		{
			e = &g_edicts[i];
			if (!e->inuse)
				continue;
			if (Q_stricmp(e->classname,"hint_path"))
				continue;
			if (!visible(self,e))
				continue;
			if (!canReach(self,e))
				continue;
			VectorSubtract(e->s.origin,self->s.origin,dir);
			dist = VectorLength(dir);
			if (dist < bestdistance)
			{
				hint = e;
				bestdistance = dist;
			}
		}
		if (hint)
		{
			self->hint_chain_id = hint->hint_chain_id;
			if (!self->monsterinfo.pathdir)
				self->monsterinfo.pathdir = 1;
			VectorSubtract(hint->s.origin, self->s.origin, dir);
			self->ideal_yaw = vectoyaw(dir);
			self->goalentity = self->movetarget = hint;
			self->monsterinfo.pausetime = 0;
			self->monsterinfo.aiflags |= AI_MEDIC_PATROL;
			self->monsterinfo.aiflags &= ~(AI_SOUND_TARGET | AI_PURSUIT_LAST_SEEN | AI_PURSUE_NEXT | AI_PURSUE_TEMP);
			// run for it
			self->monsterinfo.run (self);
		}
	}
}

void medic_search (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_search, 1, ATTN_IDLE, 0);

	if (!self->oldenemy)
		medic_FindDeadMonster(self);
}

void medic_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}


mframe_t medic_frames_stand [] =
{
	ai_stand, 0, medic_idle,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

};
mmove_t medic_move_stand = {FRAME_wait1, FRAME_wait90, medic_frames_stand, NULL};

void medic_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &medic_move_stand;
}


mframe_t medic_frames_walk [] =
{
	ai_walk, 6.2,	NULL,
	ai_walk, 18.1,  NULL,
	ai_walk, 1,		NULL,
	ai_walk, 9,		NULL,
	ai_walk, 10,	NULL,
	ai_walk, 9,		NULL,
	ai_walk, 11,	NULL,
	ai_walk, 11.6,  NULL,
	ai_walk, 2,		NULL,
	ai_walk, 9.9,	NULL,
	ai_walk, 14,	NULL,
	ai_walk, 9.3,	NULL
};
mmove_t medic_move_walk = {FRAME_walk1, FRAME_walk12, medic_frames_walk, NULL};

void medic_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &medic_move_walk;
}


mframe_t medic_frames_run [] =
{
	ai_run, 18,		NULL,
	ai_run, 22.5,	NULL,
	ai_run, 25.4,	NULL,
	ai_run, 23.4,	NULL,
	ai_run, 24,		NULL,
	ai_run, 35.6,	NULL
	
};
mmove_t medic_move_run = {FRAME_run1, FRAME_run6, medic_frames_run, NULL};

void medic_run (edict_t *self)
{
	self->busy = false;		//CW: reset so dodging routines can be executed as normal
	if (!(self->monsterinfo.aiflags & AI_MEDIC))
	{
		if (medic_FindDeadMonster(self))
			return;
	}

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &medic_move_stand;
	else
		self->monsterinfo.currentmove = &medic_move_run;
}


mframe_t medic_frames_pain1 [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t medic_move_pain1 = {FRAME_paina1, FRAME_paina8, medic_frames_pain1, medic_run};

mframe_t medic_frames_pain2 [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t medic_move_pain2 = {FRAME_painb1, FRAME_painb15, medic_frames_pain2, medic_run};

void medic_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;

	if (skill->value > 1)  
		return;		// no pain anims in nightmare (CW: or hard)

	if (damage <= 10)	//CW: shrug off low damage
		return;

	if (random() < 0.5)
	{
		self->monsterinfo.currentmove = &medic_move_pain1;
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	}
	else
	{
		self->monsterinfo.currentmove = &medic_move_pain2;
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	}
}

void medic_fire_blaster (edict_t *self)
{
//CW+++
	vec3_t	forward;
	vec3_t	right;
	vec3_t	up;
	vec3_t	start;
	vec3_t	end;
	vec3_t	dir;
	vec3_t	aim;
	float	time;
	float	r;
	float	xy_velsq;
	int		bolt_speed;
//CW---
	int		effect;

	if (!self->enemy || !self->enemy->inuse)	//CW
		return;

	if ((self->s.frame == FRAME_attack9) || (self->s.frame == FRAME_attack12))
		effect = EF_BLASTER;
	else if ((self->s.frame == FRAME_attack19) || (self->s.frame == FRAME_attack22) || (self->s.frame == FRAME_attack25) || (self->s.frame == FRAME_attack28))
		effect = EF_HYPERBLASTER;
	else
		effect = 0;

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_MEDIC_BLASTER_1], forward, right, start);
	VectorCopy (self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;

	// Lazarus fog reduction of accuracy
	if (self->monsterinfo.visibility < FOG_CANSEEGOOD)
	{
		end[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		end[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		end[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
	}
	VectorSubtract (end, start, aim);

//CW+++
//	Lead the target...

	bolt_speed = BOLT_BASESPEED + (BOLT_SKILLSPEED * skill->value);
	time = VectorLength(aim) / bolt_speed;
	end[0] += time * self->enemy->velocity[0];
	end[1] += time * self->enemy->velocity[1];
	VectorSubtract(end, start, aim);

//	...and degrade accuracy based on skill level and target velocity.

	vectoangles(aim, dir);
	AngleVectors(dir, forward, right, up);
	xy_velsq = (self->enemy->velocity[0]*self->enemy->velocity[0]) + (self->enemy->velocity[1]*self->enemy->velocity[1]);
	r = crandom() * (AIM_R_BASE - (AIM_R_SKILL * skill->value)) * (xy_velsq / 90000);
	VectorMA(start, 8192, forward, end);
	VectorMA(end, r, right, end);
	VectorSubtract(end, start, aim);
	VectorNormalize(aim);
//CW---
	monster_fire_blaster (self, start, aim, 4, bolt_speed, MZ2_MEDIC_BLASTER_1, effect, BLASTER_ORANGE);		//CW: dmg was 2, speed was 1000
}


void medic_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
	M_FlyCheck (self);

	// Lazarus monster fade
	if (world->effects & FX_WORLDSPAWN_CORPSEFADE)
	{
		self->think=FadeDieSink;
		self->nextthink=level.time+corpse_fadetime->value;
	}
}

mframe_t medic_frames_death [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t medic_move_death = {FRAME_death1, FRAME_death30, medic_frames_death, medic_dead};

void medic_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

	self->s.skinnum |= 1;
	self->monsterinfo.power_armor_type = POWER_ARMOR_NONE;
	// if we had a pending patient, free him up for another medic
	if ((self->enemy) && (self->enemy->owner == self))
		self->enemy->owner = NULL;

// check for gib
	if (self->health <= self->gib_health && !(self->spawnflags & SF_MONSTER_NOGIB))
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n= 0; n < 2; n++)
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n= 0; n < 4; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowHead (self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

// regular death
	gi.sound (self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	self->monsterinfo.currentmove = &medic_move_death;
}


void medic_duck_down (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;
	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32;
	self->takedamage = DAMAGE_YES;
	//self->monsterinfo.pausetime = level.time + 1;		//CW: moved to medic_dodge()
	gi.linkentity (self);
}

void medic_duck_hold (edict_t *self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

void medic_duck_up (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = DAMAGE_AIM;
	gi.linkentity (self);
}

mframe_t medic_frames_duck [] =
{
	ai_move, -1,	NULL,
	ai_move, -1,	NULL,
	ai_move, -1,	medic_duck_down,
	ai_move, -1,	medic_duck_hold,
	ai_move, -1,	NULL,
	ai_move, -1,	NULL,
	ai_move, -1,	medic_duck_up,
	ai_move, -1,	NULL,
	ai_move, -1,	NULL,
	ai_move, -1,	NULL,
	ai_move, -1,	NULL,
	ai_move, -1,	NULL,
	ai_move, -1,	NULL,
	ai_move, -1,	NULL,
	ai_move, -1,	NULL,
	ai_move, -1,	NULL
};
mmove_t medic_move_duck = {FRAME_duck1, FRAME_duck16, medic_frames_duck, medic_run};

void medic_dodge (edict_t *self, edict_t *attacker, float eta)
{
	if (!self->enemy)							//CW: moved to start
		self->enemy = attacker;

//CW+++
	if (self->monsterinfo.aiflags & AI_MEDIC)
		return;

	if ((self->busy) && (random() < 0.5))		//don't always dodge if currently firing
		return;

	if (self->count > 2)						// player has learnt to aim low, so don't bother ducking
		return;

	if (eta > 1.0)								// would be vulnerable for too long
		return;
//CW---

	if (random() > 0.25)
		return;

	self->monsterinfo.pausetime = level.time + eta + 0.5;	//CW: was +1.0
	self->monsterinfo.currentmove = &medic_move_duck;
}

mframe_t medic_frames_attackHyperBlaster [] =
{
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	medic_fire_blaster,
	ai_charge, 0,	medic_fire_blaster,
	ai_charge, 0,	medic_fire_blaster,
	ai_charge, 0,	medic_fire_blaster,
	ai_charge, 0,	medic_fire_blaster,
	ai_charge, 0,	medic_fire_blaster,
	ai_charge, 0,	medic_fire_blaster,
	ai_charge, 0,	medic_fire_blaster,
	ai_charge, 0,	medic_fire_blaster,
	ai_charge, 0,	medic_fire_blaster,
	ai_charge, 0,	medic_fire_blaster,
	ai_charge, 0,	medic_fire_blaster
};
mmove_t medic_move_attackHyperBlaster = {FRAME_attack15, FRAME_attack30, medic_frames_attackHyperBlaster, medic_run};


void medic_continue (edict_t *self)
{
//CW++	Fix potential crashes
	if (!self->enemy)
		return;
//CW--

	if (visible(self, self->enemy))
		if (random() <= 0.95)
			self->monsterinfo.currentmove = &medic_move_attackHyperBlaster;
}


mframe_t medic_frames_attackBlaster [] =
{
	ai_charge, 0,	NULL,
	ai_charge, 5,	NULL,
	ai_charge, 5,	NULL,
	ai_charge, 3,	NULL,
	ai_charge, 2,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	medic_fire_blaster,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	medic_fire_blaster,	
	ai_charge, 0,	NULL,
	ai_charge, 0,	medic_continue	// Change to medic_continue... Else, go to frame 32
};
mmove_t medic_move_attackBlaster = {FRAME_attack1, FRAME_attack14, medic_frames_attackBlaster, medic_run};


void medic_hook_launch (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_hook_launch, 1, ATTN_NORM, 0);
}

void ED_CallSpawn (edict_t *ent);

static vec3_t	medic_cable_offsets[] =
{
	45.0,  -9.2, 15.5,
	48.4,  -9.7, 15.2,
	47.8,  -9.8, 15.8,
	47.3,  -9.3, 14.3,
	45.4, -10.1, 13.1,
	41.9, -12.7, 12.0,
	37.8, -15.8, 11.2,
	34.3, -18.4, 10.7,
	32.7, -19.7, 10.4,
	32.7, -19.7, 10.4
};

void medic_cable_attack (edict_t *self)
{
	vec3_t	offset, start, end, f, r;
	trace_t	tr;
	vec3_t	dir;

	if ((!self->enemy) || (!self->enemy->inuse) || (self->enemy->health <= self->enemy->gib_health))
	{
		abortHeal (self,false);
		return;
	}
	// Lazarus: check embeddment
	if (embedded(self->enemy))
	{
		abortHeal (self,false);
		return;
	}

	// see if our enemy has changed to a client, or our target has more than 0 health,
	// abort it .. we got switched to someone else due to damage
	if ((self->enemy->client) || (self->enemy->health > 0)) {
		abortHeal (self,false);
		return;
	}

	AngleVectors (self->s.angles, f, r, NULL);
	VectorCopy (medic_cable_offsets[self->s.frame - FRAME_attack42], offset);
	G_ProjectSource (self->s.origin, offset, f, r, start);

	// check for max distance
	// Lazarus: Not needed, done in checkattack
	// check for min distance
	VectorSubtract (self->enemy->s.origin, start, dir);
/*	distance = VectorLength(dir);
	if (distance < MEDIC_MIN_DISTANCE) {
		gi.dprintf("MEDIC_MIN_DISTANCE\n");
		abortHeal (self,false);
		return;
	} */

	// Lazarus: Check for enemy behind muzzle... don't do these guys, 'cause usually this
	// results in monster entanglement
	VectorNormalize(dir);
	if (DotProduct(dir,f) < 0.) {
		abortHeal (self,false);
		return;
	}

	// check for min/max pitch
	// Rogue takes this out... makes medic more likely to heal and 
	// comments say "doesn't look bad when it fails"... we'll see
/*	vectoangles (dir, angles);
	if (angles[0] < -180)
		angles[0] += 360;
	if (fabs(angles[0]) > 45)
		return; */

	tr = gi.trace (start, NULL, NULL, self->enemy->s.origin, self, MASK_SHOT);
	if (tr.fraction != 1.0 && tr.ent != self->enemy)
	{
		if (tr.ent == world)
		{
			// give up on second try
			if (self->monsterinfo.medicTries > 1)
			{
				abortHeal (self,true);
				return;
			}
			self->monsterinfo.medicTries++;
			cleanupHeal (self, 1);
			return;
		}
		abortHeal (self,false);
		return;
	}

	if (self->s.frame == FRAME_attack43)
	{
		gi.sound (self->enemy, CHAN_AUTO, sound_hook_hit, 1, ATTN_NORM, 0);
		self->enemy->monsterinfo.aiflags |= AI_RESURRECTING;
		M_SetEffects(self->enemy);
	}
	else if (self->s.frame == FRAME_attack50)
	{
		self->enemy->spawnflags &= SF_MONSTER_NOGIB;
		self->enemy->monsterinfo.aiflags = 0;
		self->enemy->target = NULL;
		self->enemy->targetname = NULL;
		self->enemy->combattarget = NULL;
		self->enemy->deathtarget = NULL;
		self->enemy->owner = self;
		// Lazarus: reset initially dead monsters to use the INVERSE of their
		// initial health, and force gib_health to default value
		if (self->enemy->max_health < 0)
		{
			self->enemy->max_health = -self->enemy->max_health;
			self->enemy->gib_health = 0;
		}
		self->enemy->health = self->enemy->max_health;
		self->enemy->takedamage = DAMAGE_AIM;
		self->enemy->flags &= ~FL_NO_KNOCKBACK;
		self->enemy->pain_debounce_time = 0;
		self->enemy->damage_debounce_time = 0;
		self->enemy->deadflag = DEAD_NO;
		if (self->enemy->s.effects & EF_FLIES)
			M_FliesOff(self->enemy);
		ED_CallSpawn (self->enemy);
		self->enemy->monsterinfo.healer = NULL;
		self->enemy->owner = NULL;
		if (self->enemy->think)
		{
			self->enemy->nextthink = level.time;
			self->enemy->think (self->enemy);
		}
		self->enemy->monsterinfo.aiflags &= ~AI_RESURRECTING;
		M_SetEffects(self->enemy);
		if (self->oldenemy && self->oldenemy->client)
		{
			self->enemy->enemy = self->oldenemy;
			FoundTarget (self->enemy);
		}
		else
		{
			// Lazarus: this should make oblivious monsters 
			// find player again
			self->enemy->enemy = NULL;
		}

	}
	else
	{
		if (self->s.frame == FRAME_attack44)
			gi.sound (self, CHAN_WEAPON, sound_hook_heal, 1, ATTN_NORM, 0);
	}

	// adjust start for beam origin being in middle of a segment
	// Lazarus: This isn't right... this causes cable start point to be well above muzzle
	//          when target is closeby. f should be vector from muzzle to target, not
	//          the forward viewing direction. PLUS... 8 isn't right... the model is
	//          actually 32 units long, so use 16.. fixed below.
//	VectorMA (start, 8, f, start);

	// adjust end z for end spot since the monster is currently dead
	VectorCopy (self->enemy->s.origin, end);
	end[2] = self->enemy->absmin[2] + self->enemy->size[2] / 2;

	// Lazarus fix
	VectorSubtract(end,start,f);
	VectorNormalize(f);
	VectorMA(start,16,f,start);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_MEDIC_CABLE_ATTACK);
	gi.WriteShort (self - g_edicts);
	gi.WritePosition (start);
	gi.WritePosition (end);
	gi.multicast (self->s.origin, MULTICAST_PVS);
}

void medic_hook_retract (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_hook_retract, 1, ATTN_NORM, 0);
	if (self->enemy)
	{
		self->enemy->monsterinfo.aiflags &= ~AI_RESURRECTING;
		M_SetEffects (self->enemy);
	}
}

mframe_t medic_frames_attackCable [] =
{
	ai_move, 2,		NULL,
	ai_move, 3,		NULL,
	ai_move, 5,		NULL,
	ai_move, 4.4,	NULL,
	ai_charge, 4.7,	NULL,
	ai_charge, 5,	NULL,
	ai_charge, 6,	NULL,
	ai_charge, 4,	NULL,
	ai_charge, 0,	NULL,
	ai_move, 0,		medic_hook_launch,
	ai_move, 0,		medic_cable_attack,
	ai_move, 0,		medic_cable_attack,
	ai_move, 0,		medic_cable_attack,
	ai_move, 0,		medic_cable_attack,
	ai_move, 0,		medic_cable_attack,
	ai_move, 0,		medic_cable_attack,
	ai_move, 0,		medic_cable_attack,
	ai_move, 0,		medic_cable_attack,
	ai_move, 0,		medic_cable_attack,
	ai_move, -15,	medic_hook_retract,
	ai_move, -1.5,	NULL,
	ai_move, -1.2,	NULL,
	ai_move, -3,	NULL,
	ai_move, -2,	NULL,
	ai_move, 0.3,	NULL,
	ai_move, 0.7,	NULL,
	ai_move, 1.2,	NULL,
	ai_move, 1.3,	NULL
};
mmove_t medic_move_attackCable = {FRAME_attack33, FRAME_attack60, medic_frames_attackCable, medic_run};


void medic_attack(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_MEDIC)
		self->monsterinfo.currentmove = &medic_move_attackCable;
	else
	{
		self->busy = true;		//CW: ensure dodging routines don't get executed
		self->monsterinfo.currentmove = &medic_move_attackBlaster;
	}
}

qboolean medic_checkattack (edict_t *self)
{
	if (!(self->monsterinfo.aiflags & AI_MEDIC))
	{
		if ( medic_FindDeadMonster(self) )
			return false;
	}

	if (self->monsterinfo.aiflags & AI_MEDIC)
	{
		float	r;
		vec3_t	forward, right, offset, start;
		trace_t	tr;

		// if we have 5 seconds or less before a timeout,
		// look for a hint_path to the target
		if ( (self->timestamp < level.time + 5) &&
			 (self->monsterinfo.last_hint_time + 5 < level.time) )
		{
			// check for hint_paths.
			self->monsterinfo.last_hint_time = level.time;
			if (monsterlost_checkhint(self))
			{
				if (developer->value)
					gi.dprintf("medic at %s using hint_paths to find %s\n",
					vtos(self->s.origin), self->enemy->classname);
				self->timestamp = level.time + MEDIC_TRY_TIME;
				return false;
			}
		}

		// if we ran out of time, give up
		if (self->timestamp < level.time)
		{
			if (developer->value)
				gi.dprintf("medic at %s timed out, abort heal\n",vtos(self->s.origin));
			abortHeal (self, true);
			self->timestamp = 0;
			return false;
		}
		
		// if our target went away
		if ((!self->enemy) || (!self->enemy->inuse)) {
			abortHeal (self,false);
			return false;
		}
		// if target is embedded in a solid
		if (embedded(self->enemy))
		{
			abortHeal (self,false);
			return false;
		}
		r = realrange(self,self->enemy);
		if (r > MEDIC_MAX_HEAL_DISTANCE+10) {
			self->monsterinfo.attack_state = AS_STRAIGHT;
//			abortHeal(self,false);
			return false;
		} else if (r < MEDIC_MIN_DISTANCE) {
			abortHeal(self,false);
			return false;
		}
		// Lazarus 1.6.2.3: if point-to-point vector from cable to
		// target is blocked by a solid
		AngleVectors (self->s.angles, forward, right, NULL);
		// Offset [8] has the largest displacement to the left... not a sure
		// thing but this one should be the most severe test.
		VectorCopy (medic_cable_offsets[8], offset);
		G_ProjectSource (self->s.origin, offset, forward, right, start);
		tr = gi.trace(start,NULL,NULL,self->enemy->s.origin,self,MASK_SHOT|MASK_WATER);
		if (tr.fraction < 1.0 && tr.ent != self->enemy)
			return false;
		medic_attack(self);
		return true;
	}

	// Lazarus: NEVER attack other monsters
	if ((self->enemy) && (self->enemy->svflags & SVF_MONSTER))
	{
		self->enemy = self->oldenemy;
		self->oldenemy = NULL;
		if (self->enemy && self->enemy->inuse)
		{
			if (visible(self,self->enemy))
				FoundTarget(self);
			else
				HuntTarget(self);
		}
		return false;
	}

	return M_CheckAttack (self);
}


/*QUAKED monster_medic (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_medic (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	if (world->effects & FX_WORLDSPAWN_CORPSEFADE)
	{
		G_FreeEdict (self);
		return;
	}
	sound_idle1 = gi.soundindex ("medic/idle.wav");
	sound_pain1 = gi.soundindex ("medic/medpain1.wav");
	sound_pain2 = gi.soundindex ("medic/medpain2.wav");
	sound_die = gi.soundindex ("medic/meddeth1.wav");
	sound_sight = gi.soundindex ("medic/medsght1.wav");
	sound_search = gi.soundindex ("medic/medsrch1.wav");
	sound_hook_launch = gi.soundindex ("medic/medatck2.wav");
	sound_hook_hit = gi.soundindex ("medic/medatck3.wav");
	sound_hook_heal = gi.soundindex ("medic/medatck4.wav");
	sound_hook_retract = gi.soundindex ("medic/medatck5.wav");

	gi.soundindex ("medic/medatck1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	// Lazarus: special purpose skins
	if ( self->style )
	{
		PatchMonsterModel("models/monsters/medic/tris.md2");
		self->s.skinnum = self->style * 2;
	}
	
	self->s.modelindex = gi.modelindex ("models/monsters/medic/tris.md2");
	VectorSet (self->mins, -24, -24, -24);
	VectorSet (self->maxs, 24, 24, 32);

	// Lazarus: mapper-configurable health
	if (!self->health)
		self->health = 300;
	if (!self->gib_health)
		self->gib_health = -200;	//CW: was -130
	if (!self->mass)
		self->mass = 400;

	self->pain = medic_pain;
	self->die = medic_die;

	self->monsterinfo.stand = medic_stand;
	self->monsterinfo.walk = medic_walk;
	self->monsterinfo.run = medic_run;
	self->monsterinfo.dodge = medic_dodge;
	self->monsterinfo.attack = medic_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = medic_sight;
	self->monsterinfo.idle = medic_idle;
	self->monsterinfo.search = medic_search;
	self->monsterinfo.checkattack = medic_checkattack;

	// Knightmare- added sparks and blood type
	if (!self->blood_type)
		self->blood_type = 3; //sparks and blood

//CW+++ Use negative powerarmor values to give the monster a Power Screen.
	if (self->powerarmor < 0)
	{
		self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
		self->monsterinfo.power_armor_power = -self->powerarmor;
	}
//CW---
//DWH+++
	else if (self->powerarmor > 0)
	{
		self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
		self->monsterinfo.power_armor_power = self->powerarmor;
	}
//DWH---

	//if (!self->monsterinfo.flies)
	//	self->monsterinfo.flies = 0.10;	//CW: was 0.15

	gi.linkentity (self);
	self->monsterinfo.currentmove = &medic_move_stand;
	if (self->health < 0)
	{
		mmove_t	*deathmoves[] = {&medic_move_death,
								 NULL};
		M_SetDeath(self,(mmove_t **)&deathmoves);
	}
	self->common_name = "Medic";
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start (self);
}
