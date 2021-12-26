/*
==============================================================================

brain

==============================================================================
*/

#include "g_local.h"
#include "m_brain.h"

//CW+++
#define BOLT_BASESPEED		500
#define BOLT_SKILLSPEED		150
#define AIM_R_BASE			1500
#define AIM_R_SKILL			400
#define DRAIN_DIST			250
#define DRAIN_SUCK_PWR		125
#define DRAIN_SUCK_SF		25
//CW---

static int	sound_chest_open;
static int	sound_tentacles_extend;
static int	sound_tentacles_retract;
static int	sound_death;
static int	sound_idle1;
static int	sound_idle2;
static int	sound_idle3;
static int	sound_pain1;
static int	sound_pain2;
static int	sound_sight;
static int	sound_search;
static int	sound_melee1;
static int	sound_melee2;
static int	sound_melee3;

//CW+++
static int	sound_plasma;
static int	sound_launch;
static int	sound_suck;
vec3_t		offset = {8, 0, 17};
//CW---

void brain_run (edict_t *self);
void brain_dead (edict_t *self);


void brain_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void brain_search (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

//
// STAND
//
mframe_t brain_frames_stand [] =
{
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,

	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,

	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL
};
mmove_t brain_move_stand = {FRAME_stand01, FRAME_stand30, brain_frames_stand, NULL};

void brain_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &brain_move_stand;
}

//
// IDLE
//
mframe_t brain_frames_idle [] =
{
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,

	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,

	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL
};
mmove_t brain_move_idle = {FRAME_stand31, FRAME_stand60, brain_frames_idle, brain_stand};

void brain_idle (edict_t *self)
{
	if(!(self->spawnflags & SF_MONSTER_AMBUSH))
		gi.sound (self, CHAN_AUTO, sound_idle3, 1, ATTN_IDLE, 0);
	self->monsterinfo.currentmove = &brain_move_idle;
}

//
// WALK
//
mframe_t brain_frames_walk1 [] =
{
	ai_walk,	7,	NULL,
	ai_walk,	2,	NULL,
	ai_walk,	3,	NULL,
	ai_walk,	3,	NULL,
	ai_walk,	1,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	9,	NULL,
	ai_walk,	-4,	NULL,
	ai_walk,	-1,	NULL,
	ai_walk,	2,	NULL
};
mmove_t brain_move_walk1 = {FRAME_walk101, FRAME_walk111, brain_frames_walk1, NULL};

// NB: walk2 is FUBAR; do not use
void brain_walk2_cycle (edict_t *self)
{
	if (random() > 0.1)
		self->monsterinfo.nextframe = FRAME_walk220;
}

mframe_t brain_frames_walk2[] =
{
	ai_walk,	3,	NULL,
	ai_walk,	-2,	NULL,
	ai_walk,	-4,	NULL,
	ai_walk,	-3,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	1,	NULL,
	ai_walk,	12,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	-3,	NULL,
	ai_walk,	0,	NULL,

	ai_walk,	-2,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	1,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	10,	NULL,		// Cycle Start

	ai_walk,	-1,	NULL,
	ai_walk,	7,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	3,	NULL,
	ai_walk,	-3,	NULL,
	ai_walk,	2,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	-3,	NULL,
	ai_walk,	2,	NULL,
	ai_walk,	0,	NULL,

	ai_walk,	4,	brain_walk2_cycle,
	ai_walk,	-1,	NULL,
	ai_walk,	-1,	NULL,
	ai_walk,	-8,	NULL,		
	ai_walk,	0,	NULL,
	ai_walk,	1,	NULL,
	ai_walk,	5,	NULL,
	ai_walk,	2,	NULL,
	ai_walk,	-1,	NULL,
	ai_walk,	-5,	NULL
};
mmove_t brain_move_walk2 = {FRAME_walk201, FRAME_walk240, brain_frames_walk2, NULL};

void brain_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &brain_move_walk1;
	//CW: originally included a 50% chance of walk2 (commented out by JC)
}

mframe_t brain_frames_defense [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t brain_move_defense = {FRAME_defens01, FRAME_defens08, brain_frames_defense, NULL};

mframe_t brain_frames_pain3 [] =
{
	ai_move,	-2,	NULL,
	ai_move,	2,	NULL,
	ai_move,	1,	NULL,
	ai_move,	3,	NULL,
	ai_move,	0,	NULL,
	ai_move,	-4,	NULL
};
mmove_t brain_move_pain3 = {FRAME_pain301, FRAME_pain306, brain_frames_pain3, brain_run};

mframe_t brain_frames_pain2 [] =
{
	ai_move,	-2,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	3,	NULL,
	ai_move,	1,	NULL,
	ai_move,	-2,	NULL
};
mmove_t brain_move_pain2 = {FRAME_pain201, FRAME_pain208, brain_frames_pain2, brain_run};

mframe_t brain_frames_pain1 [] =
{
	ai_move,	-6,	NULL,
	ai_move,	-2,	NULL,
	ai_move,	-6,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	1,	NULL,
	ai_move,	7,	NULL,
	ai_move,	0,	NULL,
	ai_move,	3,	NULL,
	ai_move,	-1,	NULL
};
mmove_t brain_move_pain1 = {FRAME_pain101, FRAME_pain121, brain_frames_pain1, brain_run};

//
// DUCK
//
void brain_duck_down (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32;
	self->takedamage = DAMAGE_YES;
	gi.linkentity (self);
}

void brain_duck_hold (edict_t *self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

void brain_duck_up (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = DAMAGE_AIM;
	gi.linkentity (self);
}

mframe_t brain_frames_duck [] =
{
	ai_move,	0,	NULL,
	ai_move,	-2,	brain_duck_down,
	ai_move,	17,	brain_duck_hold,
	ai_move,	-3,	NULL,
	ai_move,	-1,	brain_duck_up,
	ai_move,	-5,	NULL,
	ai_move,	-6,	NULL,
	ai_move,	-6,	NULL
};
mmove_t brain_move_duck = {FRAME_duck01, FRAME_duck08, brain_frames_duck, brain_run};

void brain_dodge (edict_t *self, edict_t *attacker, float eta)
{
	if (!self->enemy)
		self->enemy = attacker;

//CW+++
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	if (self->busy)			// don't want to prematurely abort tentacle attacks
		return;

	if (self->count > 2)	// player has learnt to aim low, so don't bother ducking
		return;

	if (eta > 1.0)			// would be vulnerable for too long
		return;
//CW---

	if (random() > 0.25)
		return;

	self->monsterinfo.pausetime = level.time + eta + 0.5;
	self->monsterinfo.currentmove = &brain_move_duck;
}

mframe_t brain_frames_death2 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	9,	NULL,
	ai_move,	0,	NULL
};
mmove_t brain_move_death2 = {FRAME_death201, FRAME_death205, brain_frames_death2, brain_dead};

mframe_t brain_frames_death1 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	-2,	NULL,
	ai_move,	9,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t brain_move_death1 = {FRAME_death101, FRAME_death118, brain_frames_death1, brain_dead};

//
// MELEE
//
void brain_swing_right (edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_melee1, 1, ATTN_NORM, 0);
}

void brain_hit_right (edict_t *self)
{
	vec3_t	aim;

	VectorSet(aim, MELEE_DISTANCE, self->maxs[0], 8);
	if (fire_hit(self, aim, (15 + (rand() %5)), 40))
		gi.sound(self, CHAN_WEAPON, sound_melee3, 1, ATTN_NORM, 0);
}

void brain_swing_left (edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_melee2, 1, ATTN_NORM, 0);
}

void brain_hit_left (edict_t *self)
{
	vec3_t	aim;

	VectorSet(aim, MELEE_DISTANCE, self->mins[0], 8);
	if (fire_hit(self, aim, (15 + (rand() %5)), 40))
		gi.sound(self, CHAN_WEAPON, sound_melee3, 1, ATTN_NORM, 0);
}

mframe_t brain_frames_attack1 [] =
{
	ai_charge,	8,	NULL,
	ai_charge,	3,	NULL,
	ai_charge,	5,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	-3,	brain_swing_right,
	ai_charge,	0,	NULL,
	ai_charge,	-5,	NULL,
	ai_charge,	-7,	brain_hit_right,
	ai_charge,	0,	NULL,
	ai_charge,	6,	brain_swing_left,
	ai_charge,	1,	NULL,
	ai_charge,	2,	brain_hit_left,
	ai_charge,	-3,	NULL,
	ai_charge,	6,	NULL,
	ai_charge,	-1,	NULL,
	ai_charge,	-3,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	-11,NULL
};
mmove_t brain_move_attack1 = {FRAME_attak101, FRAME_attak118, brain_frames_attack1, brain_run};

void brain_chest_open (edict_t *self)
{
	self->spawnflags &= ~65536;
	self->monsterinfo.power_armor_type = POWER_ARMOR_NONE;
	gi.sound (self, CHAN_BODY, sound_chest_open, 1, ATTN_NORM, 0);
	self->busy = true;		//CW: stop pain reactions from prematurely aborting attacks
}

void brain_tentacle_attack (edict_t *self)
{
	vec3_t	aim;

	VectorSet (aim, MELEE_DISTANCE, 0, 8);
	if (fire_hit(self, aim, (10 + (rand() %5)), -600) && skill->value > 0)
		self->spawnflags |= 65536;

	gi.sound (self, CHAN_WEAPON, sound_tentacles_retract, 1, ATTN_NORM, 0);
}

void brain_chest_closed (edict_t *self)
{
	self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
	if (self->spawnflags & 65536)
	{
		self->spawnflags &= ~65536;
		self->monsterinfo.currentmove = &brain_move_attack1;
	}
	self->busy = false;		//CW: reset, so pain anims can occur again as normal
}

mframe_t brain_frames_attack2 [] =
{
	ai_charge,	5,	NULL,
	ai_charge,	-4,	NULL,
	ai_charge,	-4,	NULL,
	ai_charge,	-3,	NULL,
	ai_charge,	0,	brain_chest_open,
	ai_charge,	0,	NULL,
	ai_charge,	13,	brain_tentacle_attack,
	ai_charge,	0,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	-9,	brain_chest_closed,
	ai_charge,	0,	NULL,
	ai_charge,	4,	NULL,
	ai_charge,	3,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	-3,	NULL,
	ai_charge,	-6,	NULL
};
mmove_t brain_move_attack2 = {FRAME_attak201, FRAME_attak217, brain_frames_attack2, brain_run};

//CW+++ Modified _attack2: goes straight to tentacle attack
mframe_t brain_frames_attack3 [] =
{
	ai_charge,	0,	NULL,
	ai_charge,	13,	brain_tentacle_attack,
	ai_charge,	0,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	-9,	brain_chest_closed,
	ai_charge,	0,	NULL,
	ai_charge,	4,	NULL,
	ai_charge,	3,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	-3,	NULL,
	ai_charge,	-6,	NULL
};
mmove_t brain_move_attack3 = {FRAME_attak206, FRAME_attak217, brain_frames_attack3, brain_run};
//CW---

//
// RUN
//
mframe_t brain_frames_run [] =
{
	ai_run,	9,	NULL,
	ai_run,	2,	NULL,
	ai_run,	3,	NULL,
	ai_run,	3,	NULL,
	ai_run,	1,	NULL,
	ai_run,	0,	NULL,
	ai_run,	0,	NULL,
	ai_run,	10,	NULL,
	ai_run,	-4,	NULL,
	ai_run,	-1,	NULL,
	ai_run,	2,	NULL
};
mmove_t brain_move_run = {FRAME_walk101, FRAME_walk111, brain_frames_run, NULL};

void brain_run(edict_t *self)
{
	self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &brain_move_stand;
	else
		self->monsterinfo.currentmove = &brain_move_run;
}

//CW+++ Give the Brain dude a plasma weapon (if flagged as 'special').
// Note that the original 'brain_melee' subroutine has been replaced with 'brain_attack'
/*
void brain_melee(edict_t *self)
{
	if (random() <= 0.5)
		self->monsterinfo.currentmove = &brain_move_attack1;
	else
		self->monsterinfo.currentmove = &brain_move_attack2;
}
*/

void brain_plasma_attack(edict_t *self)
{
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
	int		flash_number;

	if (!self->enemy || !self->enemy->inuse)
		return;

//	Don't fire if player is not directly in front (as it looks weird), or is out of sight.

	if (!infront(self, self->enemy) || !visible(self, self->enemy))
		return;

//	Determine initial start and end vectors.

	flash_number = (self->moreflags) ? MZ2_BRAIN_PLASMA_1 : MZ2_BRAIN_PLASMA_2;
	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);
	VectorCopy(self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight - 4;
	
//	Lazarus: fog reduction of accuracy.

	if (self->monsterinfo.visibility < FOG_CANSEEGOOD)
	{
		end[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		end[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		end[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
	}
	VectorSubtract(end, start, aim);
		
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

//	Eat searing plasma death, player scum!

	gi.sound(self, CHAN_WEAPON, sound_plasma, 1, ATTN_NORM, 0);
	monster_fire_plasma(self, start, aim, 5, bolt_speed, flash_number, (self->moreflags?EF_BLUEHYPERBLASTER:0));
	self->moreflags ^= 1;
}

mframe_t brain_frames_attack_plasma [] =
{
	ai_charge,	9,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	3,	brain_plasma_attack,
	ai_charge,	3,	brain_plasma_attack,
	ai_charge,	1,	NULL,
	ai_charge,	0,	brain_plasma_attack,
	ai_charge,	0,	brain_plasma_attack,
	ai_charge,	10,	NULL,
	ai_charge,	-4,	brain_plasma_attack,
	ai_charge,	-1,	brain_plasma_attack,
	ai_charge,	2,	NULL
};
mmove_t brain_move_attack_plasma = {FRAME_walk101, FRAME_walk111, brain_frames_attack_plasma, brain_run};

// Long tentacle drain attack.

qboolean brain_drain_attack_ok(vec3_t start, vec3_t end)
{
	vec3_t dir;
	vec3_t angles;

//	Check for max distance.

	VectorSubtract(start, end, dir);
	if (VectorLength(dir) > (DRAIN_DIST + (skill->value * 100)))
		return false;

//	Check for min/max pitch.

	vectoangles(dir, angles);
	if (angles[0] < -180)
		angles[0] += 360;

	if (fabs(angles[0]) > 30)
		return false;

	return true;
}

void brain_drain_attack(edict_t *self)
{
	vec3_t	start;
	vec3_t	forward;
	vec3_t	right;
	vec3_t	end;
	vec3_t	dir;
	trace_t	tr;
	int		damage;
	int		suck_power;

	if (!self->enemy || !self->enemy->inuse)
		return;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
	VectorCopy(self->enemy->s.origin, end);

	if (!brain_drain_attack_ok(start, end))
	{
		end[2] = self->enemy->s.origin[2] + self->enemy->maxs[2] - 8;
		if (!brain_drain_attack_ok(start, end))
		{
			end[2] = self->enemy->s.origin[2] + self->enemy->mins[2] + 8;
			if (!brain_drain_attack_ok(start, end))
				return;
		}
	}

	VectorCopy(self->enemy->s.origin, end);
	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);
	if (tr.ent != self->enemy)
		return;

	if (self->s.frame == FRAME_walk222)
	{
		gi.sound(self, CHAN_WEAPON, sound_launch, 1, ATTN_NORM, 0);
		return;
	}
	else if (self->s.frame == FRAME_walk223)
		damage = 5;
	else
	{
		damage = 2;
		if (self->s.frame == FRAME_walk225)
			gi.sound (self, CHAN_WEAPON, sound_suck, 1, ATTN_NORM, 0);
	}

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_PARASITE_ATTACK);
	gi.WriteShort(self - g_edicts);
	gi.WritePosition(start);
	gi.WritePosition(end);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	VectorSubtract(start, end, dir);
	suck_power = DRAIN_SUCK_PWR + (skill->value * DRAIN_SUCK_SF);
	T_Damage(self->enemy, self, self, dir, self->enemy->s.origin, vec3_origin, damage, suck_power, 0, MOD_UNKNOWN);
}

void brain_check_melee(edict_t *self)
{
//CW++	Fix potential crashes
	if (!self->enemy)
	{
		self->monsterinfo.currentmove = &brain_move_walk1;
		return;
	}
//CW--

	self->busy = false;
	if ((self->enemy->health > 0) && (range(self, self->enemy) == RANGE_MELEE) && (visible(self, self->enemy)))
	{
		if (random() < 0.5)
			self->monsterinfo.currentmove = &brain_move_attack1;
		else
			self->monsterinfo.currentmove = &brain_move_attack3;	//modified _attack2
	}
	else
		self->monsterinfo.currentmove = &brain_move_walk1;
}

mframe_t brain_frames_attack_drain[] =
{
	ai_charge,	10,	NULL,
	ai_charge,	-1,	NULL,
	ai_charge,	7,	brain_drain_attack,	//222
	ai_charge,	0,	brain_drain_attack,
	ai_charge,	3,	brain_drain_attack,
	ai_charge,	-3,	brain_drain_attack,
	ai_charge,	2,	brain_drain_attack,
	ai_charge,	4,	brain_drain_attack,	//227: >= medium
	ai_charge,	-3,	brain_drain_attack,
	ai_charge,	2,	brain_drain_attack,	//229: >= hard
	ai_charge,	0,	brain_drain_attack,
	ai_charge,	4,	brain_check_melee,
};
mmove_t brain_move_attack_drain = {FRAME_walk220, FRAME_walk231, brain_frames_attack_drain, NULL};

void brain_drain_check(edict_t *self)
{
	if (!self->enemy || !self->enemy->inuse)
		return;

	if (range(self, self->enemy) == RANGE_MELEE)
	{
		if (random() <= 0.5)
			self->monsterinfo.currentmove = &brain_move_attack1;
		else
			self->monsterinfo.currentmove = &brain_move_attack2;
	}
	else
		self->monsterinfo.currentmove = &brain_move_attack_drain;
}

mframe_t brain_frames_preattack_drain[] =
{
	ai_charge,	10,	brain_chest_open,
	ai_charge,	-1,	NULL,
	ai_charge,	7,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	3,	NULL,
	ai_charge,	-3,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	4,	NULL,
	ai_charge,	-3,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	4,	brain_drain_check
};
mmove_t brain_move_preattack_drain = {FRAME_walk220, FRAME_walk231, brain_frames_preattack_drain, NULL};

void brain_attack(edict_t *self)	//Attack of the BRAINS! They came from OUTER SPACE!! (cue dramatic music)
{
	if (range(self, self->enemy) == RANGE_MELEE)
	{
		if (random() <= 0.5)
			self->monsterinfo.currentmove = &brain_move_attack1;
		else
			self->monsterinfo.currentmove = &brain_move_attack2;
	}
	else
	{
		if ((self->spawnflags & SF_MONSTER_SPECIAL) && (random() < 0.5))
			self->monsterinfo.currentmove = &brain_move_attack_plasma;
		else
			self->monsterinfo.currentmove = &brain_move_preattack_drain;
	}
}
//CW---

void brain_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	float	r;

	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;

	if (skill->value > 1)  
		return;			// no pain anims in nightmare (CW: or hard)

	if (damage <= 10)	//CW: shrug off low damage
		return;

	r = random();
	if (r < 0.33)
	{
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &brain_move_pain1;
	}
	else if (r < 0.66)
	{
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &brain_move_pain2;
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &brain_move_pain3;
	}
}

void brain_dead (edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
	M_FlyCheck(self);

//	Lazarus monster fade.

	if (world->effects & FX_WORLDSPAWN_CORPSEFADE)
	{
		self->think = FadeDieSink;
		self->nextthink = level.time + corpse_fadetime->value;
	}
}

void brain_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

	self->s.effects = 0;
	self->monsterinfo.power_armor_type = POWER_ARMOR_NONE;

//	Check for gib.

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

//	Regular death.

	gi.sound (self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	if (random() <= 0.5)
		self->monsterinfo.currentmove = &brain_move_death1;
	else
		self->monsterinfo.currentmove = &brain_move_death2;
}

/*QUAKED monster_brain (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_brain (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	sound_chest_open = gi.soundindex("brain/brnatck1.wav");
	sound_tentacles_extend = gi.soundindex("brain/brnatck2.wav");
	sound_tentacles_retract = gi.soundindex("brain/brnatck3.wav");
	sound_death = gi.soundindex("brain/brndeth1.wav");
	sound_idle1 = gi.soundindex("brain/brnidle1.wav");
	sound_idle2 = gi.soundindex("brain/brnidle2.wav");
	sound_idle3 = gi.soundindex("brain/brnlens1.wav");
	sound_pain1 = gi.soundindex("brain/brnpain1.wav");
	sound_pain2 = gi.soundindex("brain/brnpain2.wav");
	sound_sight = gi.soundindex("brain/brnsght1.wav");
	sound_search = gi.soundindex("brain/brnsrch1.wav");
	sound_melee1 = gi.soundindex("brain/melee1.wav");
	sound_melee2 = gi.soundindex("brain/melee2.wav");
	sound_melee3 = gi.soundindex("brain/melee3.wav");

//CW+++
	sound_launch = gi.soundindex("parasite/paratck1.wav");
	sound_suck = gi.soundindex("parasite/paratck3.wav");
	if (self->spawnflags & SF_MONSTER_SPECIAL)
		sound_plasma = gi.soundindex("brain/brnatck4.wav");

//CW---

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

//	Lazarus: special purpose skins.

	if (self->style)
	{
		PatchMonsterModel("models/monsters/brain/tris.md2");
		self->s.skinnum = self->style * 2;
	}
	
	self->s.modelindex = gi.modelindex("models/monsters/brain/tris.md2");
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 32);

//	Lazarus: mapper-configurable health.

	if (!self->health)
		self->health = 300;
	if (!self->gib_health)
		self->gib_health = -150;
	if (!self->mass)
		self->mass = 400;

	self->pain = brain_pain;
	self->die = brain_die;
	self->monsterinfo.stand = brain_stand;
	self->monsterinfo.walk = brain_walk;
	self->monsterinfo.run = brain_run;
	self->monsterinfo.dodge = brain_dodge;
	self->monsterinfo.attack = brain_attack;	//CW: allows for non-melee attack
	//self->monsterinfo.melee = NULL;				//CW: was brain_melee
	self->monsterinfo.sight = brain_sight;
	self->monsterinfo.search = brain_search;
	self->monsterinfo.idle = brain_idle;
	self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
	self->monsterinfo.power_armor_power = 100;

	//if (!self->monsterinfo.flies)
	//	self->monsterinfo.flies = 0.05;			//CW: was 0.10

	self->common_name = "Brains";

	// Knightmare- added sparks and blood type
	if (!self->blood_type)
		self->blood_type = 3; //sparks and blood

	gi.linkentity (self);

	self->monsterinfo.currentmove = &brain_move_stand;	
	if (self->health < 0)
	{
		mmove_t	*deathmoves[] = {&brain_move_death1, &brain_move_death2, NULL};
		M_SetDeath(self, (mmove_t **)&deathmoves);
	}
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start(self);
}
