/*
==============================================================================

hover

==============================================================================
*/

#include "g_local.h"
#include "m_hover.h"

//CW+++
#define BOLT_BASESPEED		800
#define BOLT_SKILLSPEED		100
#define RKT_BASESPEED		500
#define RKT_SKILLSPEED		100
#define AIM_R_BASE			1500
#define AIM_R_SKILL			250
//CW---

qboolean visible (edict_t *self, edict_t *other);

static int	sound_pain1;
static int	sound_pain2;
static int	sound_death1;
static int	sound_death2;
static int	sound_sight;
static int	sound_search1;
static int	sound_search2;
static int	sound_attack_rocket;	//CW
static int	sound_attack_blaster;	//CW


void hover_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void hover_search (edict_t *self)
{
	if (random() < 0.5)
		gi.sound (self, CHAN_VOICE, sound_search1, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_search2, 1, ATTN_NORM, 0);
}


void hover_run (edict_t *self);
void hover_stand (edict_t *self);
void hover_dead (edict_t *self);
void hover_attack (edict_t *self);
void hover_reattack (edict_t *self);
void hover_fire_blaster (edict_t *self);
void hover_fire_rocket (edict_t *self);
void hover_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

mframe_t hover_frames_stand [] =
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
mmove_t	hover_move_stand = {FRAME_stand01, FRAME_stand30, hover_frames_stand, NULL};

mframe_t hover_frames_stop1 [] =
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
mmove_t hover_move_stop1 = {FRAME_stop101, FRAME_stop109, hover_frames_stop1, NULL};

mframe_t hover_frames_stop2 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t hover_move_stop2 = {FRAME_stop201, FRAME_stop208, hover_frames_stop2, NULL};

mframe_t hover_frames_takeoff [] =
{
	ai_move,	0,	NULL,
	ai_move,	-2,	NULL,
	ai_move,	5,	NULL,
	ai_move,	-1,	NULL,
	ai_move,	1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	-1,	NULL,
	ai_move,	-1,	NULL,
	ai_move,	-1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	2,	NULL,
	ai_move,	1,	NULL,
	ai_move,	1,	NULL,
	ai_move,	-6,	NULL,
	ai_move,	-9,	NULL,
	ai_move,	1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	2,	NULL,
	ai_move,	1,	NULL,
	ai_move,	1,	NULL,
	ai_move,	1,	NULL,
	ai_move,	2,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	3,	NULL,
	ai_move,	2,	NULL,
	ai_move,	0,	NULL
};
mmove_t hover_move_takeoff = {FRAME_takeof01, FRAME_takeof30, hover_frames_takeoff, NULL};

mframe_t hover_frames_pain3 [] =
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
mmove_t hover_move_pain3 = {FRAME_pain301, FRAME_pain309, hover_frames_pain3, hover_run};

mframe_t hover_frames_pain2 [] =
{
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
mmove_t hover_move_pain2 = {FRAME_pain201, FRAME_pain212, hover_frames_pain2, hover_run};

mframe_t hover_frames_pain1 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	-8,	NULL,
	ai_move,	-4,	NULL,
	ai_move,	-6,	NULL,
	ai_move,	-4,	NULL,
	ai_move,	-3,	NULL,
	ai_move,	1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	3,	NULL,
	ai_move,	1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	3,	NULL,
	ai_move,	2,	NULL,
	ai_move,	7,	NULL,
	ai_move,	1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	5,	NULL,
	ai_move,	3,	NULL,
	ai_move,	4,	NULL
};
mmove_t hover_move_pain1 = {FRAME_pain101, FRAME_pain128, hover_frames_pain1, hover_run};

mframe_t hover_frames_land [] =
{
	ai_move,	0,	NULL
};
mmove_t hover_move_land = {FRAME_land01, FRAME_land01, hover_frames_land, NULL};

mframe_t hover_frames_forward [] =
{
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
mmove_t hover_move_forward = {FRAME_forwrd01, FRAME_forwrd35, hover_frames_forward, NULL};

mframe_t hover_frames_walk [] =
{
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL
};
mmove_t hover_move_walk = {FRAME_forwrd01, FRAME_forwrd35, hover_frames_walk, NULL};

mframe_t hover_frames_run [] =
{
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL
};
mmove_t hover_move_run = {FRAME_forwrd01, FRAME_forwrd35, hover_frames_run, NULL};

mframe_t hover_frames_death1 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	-10,NULL,
	ai_move,	3,	NULL,
	ai_move,	5,	NULL,
	ai_move,	4,	NULL,
	ai_move,	7,	NULL
};
mmove_t hover_move_death1 = {FRAME_death101, FRAME_death111, hover_frames_death1, hover_dead};

mframe_t hover_frames_backward [] =
{
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
mmove_t hover_move_backward = {FRAME_backwd01, FRAME_backwd24, hover_frames_backward, NULL};

mframe_t hover_frames_start_attack [] =
{
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL
};
mmove_t hover_move_start_attack = {FRAME_attak101, FRAME_attak103, hover_frames_start_attack, hover_attack};

mframe_t hover_frames_attack1 [] =
{
	ai_charge,	-10,	hover_fire_blaster,
	ai_charge,	-10,	hover_fire_blaster,
	ai_charge,	0,		hover_reattack,
};
mmove_t hover_move_attack1 = {FRAME_attak104, FRAME_attak106, hover_frames_attack1, NULL};

//CW+++ new attack type: rockets.
mframe_t hover_frames_attack2 [] =
{
	ai_charge,	-10,	hover_fire_rocket,
	ai_charge,	0,		NULL,
	ai_charge,	0,		hover_reattack,
};
mmove_t hover_move_attack2 = {FRAME_attak104, FRAME_attak106, hover_frames_attack2, NULL};
//CW---

mframe_t hover_frames_end_attack [] =
{
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL
};
mmove_t hover_move_end_attack = {FRAME_attak107, FRAME_attak108, hover_frames_end_attack, hover_run};

void hover_reattack (edict_t *self)
{
	vec3_t	flightpath;	//CW
	float r;

//CW++	Fix potential crashes
	if (!self->enemy)
		return;
//CW--

	if (self->enemy->health > 0)	//CW: originally included visibility check, but that was moved to blaster reattacks
	{								//    only - makes "rocket barrages" whilst the player is in cover more likely :)

//CW++	Was 0.6 for all skill levels. Refire now depends on skill level and weapon type. 

		r = random();
		VectorSubtract(self->enemy->s.origin, self->s.origin, flightpath);
		if ((self->spawnflags & SF_MONSTER_SPECIAL) && (VectorLength(flightpath) > 150))
		{
			if (r <= (0.4 + 0.15*skill->value))
			{
				self->monsterinfo.currentmove = &hover_move_attack2;
				return;
			}
		}
		else
		{
			if (r <= (0.5 + 0.15*skill->value))
			{
				self->monsterinfo.currentmove = &hover_move_attack1;
				return;
			}
		}
//CW---
	}
	self->monsterinfo.currentmove = &hover_move_end_attack;
}

//CW+++ Monsters flagged as 'special' fire rockets instead of blaster bolts. INCOMING!
void hover_fire_rocket (edict_t *self)
{
	trace_t	tr;
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
	int		rocket_speed;

	if (!self->enemy || !self->enemy->inuse)
		return;

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[self->count?MZ2_HOVER_ROCKET_1:MZ2_HOVER_ROCKET_2], forward, right, start);

	VectorCopy (self->enemy->s.origin, end);
	if (start[2] < self->enemy->absmin[2])
		end[2] += self->enemy->viewheight;		//shoot at player's face if below them
	else
		end[2] = self->enemy->absmin[2];		//shoot at player's feet if above them

//	Lazarus: fog reduction of accuracy
	if (self->monsterinfo.visibility < FOG_CANSEEGOOD)
	{
		end[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		end[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		end[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
	}
	VectorSubtract(end, start, aim);

//	Lead the target...

	rocket_speed = RKT_BASESPEED + (RKT_SKILLSPEED * skill->value);
	time = VectorLength(aim) / rocket_speed;
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

//	Check that a solid isn't close in front before firing a rocket.

	tr = gi.trace(start, NULL, NULL, self->enemy->s.origin, self, MASK_SHOT|CONTENTS_GRILL);
	if ((tr.fraction < 1) && (tr.ent != self->enemy))
	{
		VectorSubtract(tr.endpos, start, dir);
		if (VectorNormalize(dir) < 150)
			return;
	}

//	Fire!

	gi.sound(self, CHAN_WEAPON, sound_attack_rocket, 1, ATTN_NORM, 0);
	monster_fire_rocket(self, start, aim, 20, rocket_speed, self->count?MZ2_HOVER_ROCKET_1:MZ2_HOVER_ROCKET_2, NULL);
	self->count = !self->count;
}
//CW---

void hover_fire_blaster (edict_t *self)
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
	int		flash_number;

	if (!self->enemy || !self->enemy->inuse)	//CW
		return;

	if (self->s.frame == FRAME_attak104)
		effect = EF_HYPERBLASTER;
	else
		effect = 0;

	flash_number = (self->count) ? MZ2_HOVER_BLASTER_1 : MZ2_HOVER_BLASTER_2;	//CW
	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[flash_number], forward, right, start);	//CW: was just offset 1
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

	gi.sound(self, CHAN_WEAPON, sound_attack_blaster, 1, ATTN_NORM, 0);
//CW---
	monster_fire_blaster (self, start, aim, 2, bolt_speed, flash_number, effect, BLASTER_ORANGE);	//CW: dmg was 1, speed was 1000; Knightmare- changed damage to 2
	self->count ^= 1;	//CW: alternate between shoulder launchers
}

void hover_stand (edict_t *self)
{
		self->monsterinfo.currentmove = &hover_move_stand;
}

void hover_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &hover_move_stand;
	else
		self->monsterinfo.currentmove = &hover_move_run;
}

void hover_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &hover_move_walk;
}

void hover_start_attack (edict_t *self)
{
	self->monsterinfo.currentmove = &hover_move_start_attack;
}

void hover_attack(edict_t *self)
{
//CW++ For special monsters, fire rockets if not too close.
	vec3_t	flightpath;

	if (!self->enemy)
		return;

	if ((visible(self, self->enemy)) && (self->enemy->health > 0))
	{
		VectorSubtract(self->enemy->s.origin, self->s.origin, flightpath);
		if ((self->spawnflags & SF_MONSTER_SPECIAL) && (VectorLength(flightpath) >= 200))
			self->monsterinfo.currentmove = &hover_move_attack2;
		else
//CW--
		self->monsterinfo.currentmove = &hover_move_attack1;
	}
}

//CW+++
void hover_dodge (edict_t *self, edict_t *attacker, float eta)
{
	vec3_t		v_rollend;
	trace_t		trace;
	qboolean	leftOK = false;
	qboolean	rightOK = false;

	if (!self->enemy)
		self->enemy = attacker;

	if (random() > (0.3 + (0.15 * skill->value)))
		return;

//	Check that left hand side is clear for dodge.

	v_rollend[0] = self->s.origin[0] + (64.0 * cos((self->s.angles[1]+90)*PI/180));
	v_rollend[1] = self->s.origin[1] + (64.0 * sin((self->s.angles[1]+90)*PI/180));
	v_rollend[2] = self->s.origin[2];
	trace = gi.trace(self->s.origin, self->mins, self->maxs, v_rollend, self, MASK_MONSTERSOLID);
	if (trace.fraction == 1.0)
		leftOK = true;

//	Check that right hand side is clear for dodge.

	v_rollend[0] = self->s.origin[0] + (64.0 * cos((self->s.angles[1]-90)*PI/180));
	v_rollend[1] = self->s.origin[1] + (64.0 * sin((self->s.angles[1]-90)*PI/180));
	trace = gi.trace(self->s.origin, self->mins, self->maxs, v_rollend, self, MASK_MONSTERSOLID);
	if (trace.fraction == 1.0)
		rightOK = true;

	if (!(leftOK || rightOK))
		return;
	
	if (leftOK && rightOK)
	{
		if (random() <= 0.5)
			self->moreflags = 1;
		else
			self->moreflags = -1;
	}
	else if (leftOK)
		self->moreflags = 1;
	else
		self->moreflags = -1;

	ai_strafe(self, 8.0);	// Knightmare- was 32.0
}
//CW---

void hover_pain (edict_t *self, edict_t *other, float kick, int damage)
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

	if (damage <= 25)
	{
		if (random() < 0.5)
		{
			gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = &hover_move_pain3;
		}
		else
		{
			gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = &hover_move_pain2;
		}
	}
	else
	{
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &hover_move_pain1;
	}
}

void hover_deadthink (edict_t *self)
{
	int		n;

	if (!self->groundentity && level.time < self->timestamp)
	{
		self->nextthink = level.time + FRAMETIME;
		return;
	}
	// Knightmare- gibs!
	gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
	for (n= 0; n < 8; n++)
		ThrowGib (self, "models/objects/gibs/sm_metal/tris.md2", 200, GIB_METALLIC);
	for (n= 0; n < 2; n++)
		ThrowGib (self, "models/objects/gibs/gear/tris.md2", 200, GIB_METALLIC);
	for (n= 0; n < 2; n++)
		ThrowGib (self, "models/objects/gibs/bone/tris.md2", 200, GIB_ORGANIC);
	for (n= 0; n < 6; n++)
		ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", 200, GIB_ORGANIC);
	ThrowGib (self, "models/objects/gibs/head2/tris.md2", 200, GIB_ORGANIC);
	BecomeExplosion1(self);
}

void hover_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->think = hover_deadthink;
	self->nextthink = level.time + FRAMETIME;
	self->timestamp = level.time + 15;
	gi.linkentity (self);
}

void hover_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

// check for gib
	if (self->health <= self->gib_health && !(self->spawnflags & SF_MONSTER_NOGIB))
	{	// Knightmare- more gibs!
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n= 0; n < 8; n++)
			ThrowGib (self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);
		for (n= 0; n < 2; n++)
			ThrowGib (self, "models/objects/gibs/gear/tris.md2", damage, GIB_METALLIC);
		for (n= 0; n < 2; n++)
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n= 0; n < 6; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowGib (self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		BecomeExplosion1(self);
	//	ThrowHead (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
	//	self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

// regular death
	if (random() < 0.5)
		gi.sound (self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_death2, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &hover_move_death1;
}

/*QUAKED monster_hover (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_hover (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	sound_pain1 = gi.soundindex ("hover/hovpain1.wav");	
	sound_pain2 = gi.soundindex ("hover/hovpain2.wav");	
	sound_death1 = gi.soundindex ("hover/hovdeth1.wav");	
	sound_death2 = gi.soundindex ("hover/hovdeth2.wav");	
	sound_sight = gi.soundindex ("hover/hovsght1.wav");	
	sound_search1 = gi.soundindex ("hover/hovsrch1.wav");	
	sound_search2 = gi.soundindex ("hover/hovsrch2.wav");
	self->s.sound = gi.soundindex ("hover/hovidle1.wav");

//CW+++
	sound_attack_blaster = gi.soundindex("hover/hovatck1.wav");
	if (self->spawnflags & SF_MONSTER_SPECIAL)
		sound_attack_rocket = gi.soundindex("chick/chkatck2.wav");
//CW---

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	// Lazarus: special purpose skins
	if ( self->style )
	{
		PatchMonsterModel("models/monsters/hover/tris.md2");
		self->s.skinnum = self->style * 2;
	}
	
	self->s.modelindex = gi.modelindex("models/monsters/hover/tris.md2");
	VectorSet (self->mins, -24, -24, -24);
	VectorSet (self->maxs, 24, 24, 32);

	// Lazarus: mapper-configurable health
	if(!self->health)
		self->health = 240;
	if(!self->gib_health)
		self->gib_health = -100;
	if(!self->mass)
		self->mass = 150;

	self->count = 0;		//CW: in case mapper has set this by mistake
	self->pain = hover_pain;
	self->die = hover_die;

	self->monsterinfo.stand = hover_stand;
	self->monsterinfo.walk = hover_walk;
	self->monsterinfo.run = hover_run;
	self->monsterinfo.dodge = hover_dodge;		//CW
	self->monsterinfo.attack = hover_start_attack;
	self->monsterinfo.sight = hover_sight;
	self->monsterinfo.search = hover_search;

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
	
	gi.linkentity (self);
	self->monsterinfo.currentmove = &hover_move_stand;	
	if(self->health < 0)
	{
		mmove_t	*deathmoves[] = {&hover_move_death1,
								 NULL};
		M_SetDeath(self,(mmove_t **)&deathmoves);
	}
	self->common_name = "Icarus";
	self->monsterinfo.scale = MODEL_SCALE;

	flymonster_start (self);
}
