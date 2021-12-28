/*
==============================================================================

GUNNER

==============================================================================
*/

#include "g_local.h"
#include "m_gunner.h"


static int	sound_pain;
static int	sound_pain2;
static int	sound_death;
static int	sound_idle;
static int	sound_open;
static int	sound_search;
static int	sound_sight;

// NOTE: Original gunner grenade velocity was 600 units/sec, but then 
//       fire_grenade added 200 units/sec in a direction perpendicular
//       to the aim direction. We've removed that from fire_grenade 
//       (for the gunner, not for players) since the gunner now shoots 
//       smarter, and adjusted things so that the initial velocity out 
//       of the barrel is the same.
#define GRENADE_VELOCITY 632.4555320337
#define GRENADE_VELOCITY_SQUARED 400000

void gunner_idlesound (edict_t *self)
{
	if (!(self->spawnflags & SF_MONSTER_AMBUSH))
		gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void gunner_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void gunner_search (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}


qboolean visible (edict_t *self, edict_t *other);
void GunnerGrenade (edict_t *self);
void GunnerFire (edict_t *self);
void gunner_fire_chain(edict_t *self);
void gunner_refire_chain(edict_t *self);


void gunner_stand (edict_t *self);

mframe_t gunner_frames_fidget [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, gunner_idlesound,
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
	ai_stand, 0, NULL
};
mmove_t	gunner_move_fidget = {FRAME_stand31, FRAME_stand70, gunner_frames_fidget, gunner_stand};

void gunner_fidget (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return;
	if (random() <= 0.05)
		self->monsterinfo.currentmove = &gunner_move_fidget;
}

mframe_t gunner_frames_stand [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, gunner_fidget,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, gunner_fidget,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, gunner_fidget
};
mmove_t	gunner_move_stand = {FRAME_stand01, FRAME_stand30, gunner_frames_stand, NULL};

void gunner_stand (edict_t *self)
{
		self->monsterinfo.currentmove = &gunner_move_stand;
}


mframe_t gunner_frames_walk [] =
{
	ai_walk, 0, NULL,
	ai_walk, 3, NULL,
	ai_walk, 4, NULL,
	ai_walk, 5, NULL,
	ai_walk, 7, NULL,
	ai_walk, 2, NULL,
	ai_walk, 6, NULL,
	ai_walk, 4, NULL,
	ai_walk, 2, NULL,
	ai_walk, 7, NULL,
	ai_walk, 5, NULL,
	ai_walk, 7, NULL,
	ai_walk, 4, NULL
};
mmove_t gunner_move_walk = {FRAME_walk07, FRAME_walk19, gunner_frames_walk, NULL};

void gunner_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &gunner_move_walk;
}

mframe_t gunner_frames_run [] =
{
	ai_run, 26, NULL,
	ai_run, 9,  NULL,
	ai_run, 9,  NULL,
	ai_run, 9,  NULL,
	ai_run, 15, NULL,
	ai_run, 10, NULL,
	ai_run, 13, NULL,
	ai_run, 6,  NULL
};

mmove_t gunner_move_run = {FRAME_run01, FRAME_run08, gunner_frames_run, NULL};

void gunner_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &gunner_move_stand;
	else
		self->monsterinfo.currentmove = &gunner_move_run;
}

mframe_t gunner_frames_runandshoot [] =
{
	ai_run, 32, NULL,
	ai_run, 15, NULL,
	ai_run, 10, NULL,
	ai_run, 18, NULL,
	ai_run, 8,  NULL,
	ai_run, 20, NULL
};

mmove_t gunner_move_runandshoot = {FRAME_runs01, FRAME_runs06, gunner_frames_runandshoot, NULL};

void gunner_runandshoot (edict_t *self)
{
	self->monsterinfo.currentmove = &gunner_move_runandshoot;
}

mframe_t gunner_frames_pain3 [] =
{
	ai_move, -3, NULL,
	ai_move, 1,	 NULL,
	ai_move, 1,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 1,	 NULL
};
mmove_t gunner_move_pain3 = {FRAME_pain301, FRAME_pain305, gunner_frames_pain3, gunner_run};

mframe_t gunner_frames_pain2 [] =
{
	ai_move, -2, NULL,
	ai_move, 11, NULL,
	ai_move, 6,	 NULL,
	ai_move, 2,	 NULL,
	ai_move, -1, NULL,
	ai_move, -7, NULL,
	ai_move, -2, NULL,
	ai_move, -7, NULL
};
mmove_t gunner_move_pain2 = {FRAME_pain201, FRAME_pain208, gunner_frames_pain2, gunner_run};

mframe_t gunner_frames_pain1 [] =
{
	ai_move, 2,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, -5, NULL,
	ai_move, 3,	 NULL,
	ai_move, -1, NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 1,	 NULL,
	ai_move, 1,	 NULL,
	ai_move, 2,	 NULL,
	ai_move, 1,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL
};
mmove_t gunner_move_pain1 = {FRAME_pain101, FRAME_pain118, gunner_frames_pain1, gunner_run};

void gunner_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;

	if (rand()&1)
		gi.sound (self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	if (skill->value > 1)  
		return;		// no pain anims in nightmare (CW: or hard)

	if (damage <= 10)	//CW: shrug off low damage
		return;

	if (damage <= 20)	//CW: increased damage resistance
		self->monsterinfo.currentmove = &gunner_move_pain3;
	else if (damage <= 35)	//CW: increased damage resistance
		self->monsterinfo.currentmove = &gunner_move_pain2;
	else
		self->monsterinfo.currentmove = &gunner_move_pain1;
}

void gunner_dead (edict_t *self)
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

mframe_t gunner_frames_death [] =
{
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, -7, NULL,
	ai_move, -3, NULL,
	ai_move, -5, NULL,
	ai_move, 8,	 NULL,
	ai_move, 6,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL
};
mmove_t gunner_move_death = {FRAME_death01, FRAME_death11, gunner_frames_death, gunner_dead};

void gunner_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

	self->s.skinnum |= 1;
	self->monsterinfo.power_armor_type = POWER_ARMOR_NONE;
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
	gi.sound (self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &gunner_move_death;
}

qboolean gunner_grenade_check(edict_t *self)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		target;
	trace_t		tr;
	vec3_t		dir;
	vec3_t		vhorz;
	float		horz,vertmax;

	if (!self->enemy)
		return false;

	// if the player is above my head, use machinegun.

//	if (self->absmax[2] <= self->enemy->absmin[2])
//		return false;

	// Lazarus: We can do better than that... see below


	// check to see that we can trace to the player before we start
	// tossing grenades around.
	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_GUNNER_GRENADE_1], forward, right, start);

	// see if we're too close
	VectorSubtract (self->enemy->s.origin, self->s.origin, dir);
	if (VectorLength(dir) < 100)
		return false;

	// Lazarus: Max vertical distance - this is approximate and conservative
	VectorCopy(dir,vhorz);
	vhorz[2] = 0;
	horz = VectorLength(vhorz);
	vertmax = (GRENADE_VELOCITY_SQUARED)/(2*sv_gravity->value) -
		0.5*sv_gravity->value*horz*horz/GRENADE_VELOCITY_SQUARED;
	if (dir[2] > vertmax) 
		return false;

	// Lazarus: Make sure there's a more-or-less clear flight path to target
	// Rogue checked target origin, but if target is above gunner then the trace
	// would almost always hit the platform the target was standing on
	VectorCopy(self->enemy->s.origin,target);
	target[2] = self->enemy->absmax[2];
	tr = gi.trace(start, vec3_origin, vec3_origin, target, self, MASK_SHOT);
	if (tr.ent == self->enemy || tr.fraction == 1)
		return true;
	// Repeat for feet... in case we're looking down at a target standing under,
	// for example, a short doorway
	target[2] = self->enemy->absmin[2];
	tr = gi.trace(start, vec3_origin, vec3_origin, target, self, MASK_SHOT);
	if (tr.ent == self->enemy || tr.fraction == 1)
		return true;

	return false;
}

void gunner_duck_down (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;
	if (skill->value >= 2)
	{
		// Lazarus: Added check for goodness of grenade firing
		if (random() > 0.5 && gunner_grenade_check(self))
			GunnerGrenade(self);
	}

	self->maxs[2] -= 32;
	self->takedamage = DAMAGE_YES;
	//self->monsterinfo.pausetime = level.time + 1;		//CW: moved to gunner_dodge()
	gi.linkentity (self);
}

void gunner_duck_hold (edict_t *self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

void gunner_duck_up (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = DAMAGE_AIM;
	gi.linkentity (self);
}

mframe_t gunner_frames_duck [] =
{
	ai_move, 1,  gunner_duck_down,
	ai_move, 1,  NULL,
	ai_move, 1,  gunner_duck_hold,
	ai_move, 0,  NULL,
	ai_move, -1, NULL,
	ai_move, -1, NULL,
	ai_move, 0,  gunner_duck_up,
	ai_move, -1, NULL
};
mmove_t	gunner_move_duck = {FRAME_duck01, FRAME_duck08, gunner_frames_duck, gunner_run};

void gunner_dodge (edict_t *self, edict_t *attacker, float eta)
{
	if (!self->enemy)						//CW: moved to start
		self->enemy = attacker;

//CW+++
	if (self->count > 2)					// player has learnt to aim low, so don't bother ducking
		return;

	if ((self->busy) && (random() < 0.5))	//don't always dodge if currently firing
		return;

	if (eta > 1.0)							// would be vulnerable for too long
		return;
//CW---

	if (random() > 0.25)
		return;

	self->monsterinfo.pausetime = level.time + eta + 0.5;	//CW: was +1.0
	self->monsterinfo.currentmove = &gunner_move_duck;
}

void gunner_opengun (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_open, 1, ATTN_IDLE, 0);
}

void GunnerFire (edict_t *self)
{
	vec3_t	start;
	vec3_t	forward, right, up;		//CW
	vec3_t	target;
	vec3_t	aim;
	vec3_t	dir;					//CW++
	float	r, u;					//CW++
	int		flash_number;

//CW++	Fix potential crashes
	if (!self->enemy)
		return;
//CW--

	flash_number = MZ2_GUNNER_MACHINEGUN_1 + (self->s.frame - FRAME_attak216);

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[flash_number], forward, right, start);

	// project enemy back a bit and target there
	VectorCopy (self->enemy->s.origin, target);
	VectorMA (target, -0.05*(3-skill->value), self->enemy->velocity, target);	//CW vary projection scale factor based on skill
	target[2] += self->enemy->viewheight;

	// Lazarus fog reduction of accuracy
	if (self->monsterinfo.visibility < FOG_CANSEEGOOD)
	{
		target[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		target[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		target[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
	}

	VectorSubtract (target, start, aim);
//CW++
//	Degrade aiming based on skill.
	
	if (skill->value < 3)
	{
		vectoangles (aim, dir);
		AngleVectors (dir, forward, right, up);

		r = crandom()*(1000 - 290*skill->value);
		u = crandom()*(500 - 140*skill->value);
		VectorMA (start, 8192, forward, target);
		VectorMA (target, r, right, target);
		VectorMA (target, u, up, target);

		VectorSubtract (target, start, aim);
	}
//CW--
	VectorNormalize (aim);
	monster_fire_bullet (self, start, aim, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}

//CW+++ Ensure dodging routines can be executed as normal.
void SetNotBusy(edict_t *self)
{
	self->busy = false;
}
//CW---

void GunnerGrenade (edict_t *self)
{
	vec3_t	start,target;
	vec3_t	forward, right, up;
	vec3_t	aim;
	vec_t	monster_speed;
	int		flash_number;

	if (!self->enemy || !self->enemy->inuse)
		return;

	if (self->s.frame == FRAME_attak105)
		flash_number = MZ2_GUNNER_GRENADE_1;
	else if (self->s.frame == FRAME_attak108)
	{
		if (skill->value == 0)
			return;
		flash_number = MZ2_GUNNER_GRENADE_2;
	}
	else if (self->s.frame == FRAME_attak111)
		flash_number = MZ2_GUNNER_GRENADE_3;
	else // (self->s.frame == FRAME_attak114)
	{
		if (skill->value < 2)
			return;
		flash_number = MZ2_GUNNER_GRENADE_4;
	}

	AngleVectors (self->s.angles, forward, right, up);
	G_ProjectSource (self->s.origin, monster_flash_offset[flash_number], forward, right, start);

	// Lazarus - Improved gunner's grenade-aiming ability quite a bit

//	VectorCopy (forward, aim);

	// aim at enemy's feet if he's at same elevation or lower. otherwise aim at origin
	VectorCopy(self->enemy->s.origin,target);
	if (self->enemy->absmin[2] <= self->absmax[2]) target[2] = self->enemy->absmin[2];

	// Lazarus fog reduction of accuracy
	if (self->monsterinfo.visibility < FOG_CANSEEGOOD)
	{
		target[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		target[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		target[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
	}
	// Lazarus - skill level-dependent accuracy
/*	if (skill->value < 2)
	{
		float	dist;

		VectorSubtract(target, start, aim);
		dist = VectorLength (aim);
		dist = min(dist,512);

		target[0] += crandom() * dist/8 * (2 - skill->value);
		target[1] += crandom() * dist/8 * (2 - skill->value);
	} */

	// lead target... 20, 35, 50, 65 chance of leading
	if (random() < (0.2 + skill->value * 0.15))
	{
		float	dist;
		float	time;

		VectorSubtract(target, start, aim);
		dist = VectorLength (aim);
		time = dist/GRENADE_VELOCITY;  // Not correct, but better than nothin'
		VectorMA(target, time, self->enemy->velocity, target);
	}

	AimGrenade (self, start, target, GRENADE_VELOCITY, aim);
	// Lazarus - take into account (sort of) feature of adding shooter's velocity to
	// grenade velocity
	monster_speed = VectorLength(self->velocity);
	if (monster_speed > 0)
	{
		vec3_t	v1;
		vec_t	delta;

		VectorCopy(self->velocity,v1);
		VectorNormalize(v1);
		delta = -monster_speed/GRENADE_VELOCITY;
		VectorMA(aim,delta,v1,aim);
		VectorNormalize(aim);
	}

	monster_fire_grenade (self, start, aim, 50, (int)GRENADE_VELOCITY, flash_number);
}

mframe_t gunner_frames_attack_chain [] =
{
	/*
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	*/
	ai_charge, 0, gunner_opengun,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t gunner_move_attack_chain = {FRAME_attak209, FRAME_attak215, gunner_frames_attack_chain, gunner_fire_chain};

mframe_t gunner_frames_fire_chain [] =
{
	ai_charge,   0, GunnerFire,
	ai_charge,   0, GunnerFire,
	ai_charge,   0, GunnerFire,
	ai_charge,   0, GunnerFire,
	ai_charge,   0, GunnerFire,
	ai_charge,   0, GunnerFire,
	ai_charge,   0, GunnerFire,
	ai_charge,   0, GunnerFire
};
mmove_t gunner_move_fire_chain = {FRAME_attak216, FRAME_attak223, gunner_frames_fire_chain, gunner_refire_chain};

mframe_t gunner_frames_endfire_chain [] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t gunner_move_endfire_chain = {FRAME_attak224, FRAME_attak230, gunner_frames_endfire_chain, gunner_run};

mframe_t gunner_frames_attack_grenade [] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, GunnerGrenade,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, GunnerGrenade,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, GunnerGrenade,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, GunnerGrenade,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, SetNotBusy
};
mmove_t gunner_move_attack_grenade = {FRAME_attak101, FRAME_attak121, gunner_frames_attack_grenade, gunner_run};

void gunner_attack(edict_t *self)
{
//CW++	Fix potential crashes
	if (!self->enemy)
		return;
//CW--

	self->busy = true;				//CW: ensure dodging routines don't get executed
	if (range (self, self->enemy) == RANGE_MELEE)
		self->monsterinfo.currentmove = &gunner_move_attack_chain;
	else
	{
		// Lazarus: Added bit of logic to check whether grenades are good to fire,
		//      and made it more likely to fire grenades on harder skill levels
		if (random() <= (0.5 + 0.05*skill->value) && gunner_grenade_check(self) )
			self->monsterinfo.currentmove = &gunner_move_attack_grenade;
		else
			self->monsterinfo.currentmove = &gunner_move_attack_chain;
	}
}

void gunner_fire_chain(edict_t *self)
{
	self->monsterinfo.currentmove = &gunner_move_fire_chain;
}

void gunner_refire_chain(edict_t *self)
{
//CW++	Fix potential crashes
	if (!self->enemy)
		return;
//CW--

	if ((self->enemy->health > 0) && (visible(self, self->enemy)))
	{
		if (random() <= (0.4 + 0.1*skill->value))	//CW: was <= 0.5
		{
			self->monsterinfo.currentmove = &gunner_move_fire_chain;
			return;
		}
	}
	self->monsterinfo.currentmove = &gunner_move_endfire_chain;
	self->busy = false;		//CW: ensure dodging routines can be executed as normal
}

mframe_t gunner_frames_jump [] =
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
mmove_t gunner_move_jump = { FRAME_run01, FRAME_run08, gunner_frames_jump, gunner_run };

void gunner_jump (edict_t *self)
{
	self->monsterinfo.currentmove = &gunner_move_jump;
}

/*QUAKED monster_gunner (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_gunner (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	sound_death = gi.soundindex ("gunner/death1.wav");	
	sound_pain = gi.soundindex ("gunner/gunpain2.wav");	
	sound_pain2 = gi.soundindex ("gunner/gunpain1.wav");	
	sound_idle = gi.soundindex ("gunner/gunidle1.wav");	
	sound_open = gi.soundindex ("gunner/gunatck1.wav");	
	sound_search = gi.soundindex ("gunner/gunsrch1.wav");	
	sound_sight = gi.soundindex ("gunner/sight1.wav");	
	if (monsterjump->value)
	{
		self->monsterinfo.jump = gunner_jump;
		self->monsterinfo.jumpup = 48;
		self->monsterinfo.jumpdn = 120;		//CW: was 64
	}

	gi.soundindex ("gunner/gunatck2.wav");
	gi.soundindex ("gunner/gunatck3.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	// Lazarus: special purpose skins
	if ( self->style )
	{
		PatchMonsterModel("models/monsters/gunner/tris.md2");
		self->s.skinnum = self->style * 2;
	}
	
	self->s.modelindex = gi.modelindex ("models/monsters/gunner/tris.md2");
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 32);

	// Lazarus: mapper-configurable health
	if (!self->health)
		self->health = 175;
	if (!self->gib_health)
		self->gib_health = -140;	//CW: was -70
	if (!self->mass)
		self->mass = 200;

	self->pain = gunner_pain;
	self->die = gunner_die;

	self->monsterinfo.stand = gunner_stand;
	self->monsterinfo.walk = gunner_walk;
	self->monsterinfo.run = gunner_run;
	self->monsterinfo.dodge = gunner_dodge;
	self->monsterinfo.attack = gunner_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = gunner_sight;
	self->monsterinfo.search = gunner_search;

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
	//	self->monsterinfo.flies = 0.10;	//CW: was 0.30

	// Lazarus - use move_origin for grenade aiming
	VectorCopy(monster_flash_offset[MZ2_GUNNER_GRENADE_1], self->move_origin);

	gi.linkentity (self);
	self->monsterinfo.currentmove = &gunner_move_stand;	
	if (self->health < 0)
	{
		mmove_t	*deathmoves[] = {&gunner_move_death,
								 NULL};
		M_SetDeath(self,(mmove_t **)&deathmoves);
	}
	self->common_name = "Gunner";

	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start (self);
}
