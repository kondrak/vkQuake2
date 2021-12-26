/*
==============================================================================

INFANTRY

==============================================================================
*/

#include "g_local.h"
#include "m_infantry.h"

//CW++
vec3_t VEC_UPWARDS = {0, 0, 1};

#define BOLT_BASESPEED		500
#define BOLT_SKILLSPEED		150
#define AIM_R_BASE			1500
#define AIM_R_SKILL			250
#define INF_ROLL_TIME		0.5				//duration of roll (seconds)
#define INF_ROLL_DIST		8				//distance rolled per server frame, Knightmare- was 10
//CW--

void InfantryMachineGun (edict_t *self);
void InfantryHyperBlaster (edict_t *self);	//CW

static int	sound_pain1;
static int	sound_pain2;
static int	sound_die1;
static int	sound_die2;

static int	sound_blaster;		//CW
static int	sound_weapon_cock;
static int	sound_punch_swing;
static int	sound_punch_hit;
static int	sound_sight;
static int	sound_search;
static int	sound_idle;

float inf_roll_z[5] = {0, -12, -10, 10, 12};	//CW: z-axis origin offsets during roll frames

mframe_t infantry_frames_stand [] =
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
	ai_stand, 0, NULL
};
mmove_t infantry_move_stand = {FRAME_stand50, FRAME_stand71, infantry_frames_stand, NULL};

void infantry_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &infantry_move_stand;
}


mframe_t infantry_frames_fidget [] =
{
	ai_stand, 1,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 3,  NULL,
	ai_stand, 6,  NULL,
	ai_stand, 3,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, -1, NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, -2, NULL,
	ai_stand, 1,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, -1, NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, -1, NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, -1, NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 1,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, -1, NULL,
	ai_stand, -1, NULL,
	ai_stand, 0,  NULL,
	ai_stand, -3, NULL,
	ai_stand, -2, NULL,
	ai_stand, -3, NULL,
	ai_stand, -3, NULL,
	ai_stand, -2, NULL
};
mmove_t infantry_move_fidget = {FRAME_stand01, FRAME_stand49, infantry_frames_fidget, infantry_stand};

void infantry_fidget (edict_t *self)
{
	self->monsterinfo.currentmove = &infantry_move_fidget;
	if (!(self->spawnflags & SF_MONSTER_AMBUSH))
		gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

mframe_t infantry_frames_walk [] =
{
	ai_walk, 5,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 5,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 5,  NULL,
	ai_walk, 6,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 5,  NULL
};
mmove_t infantry_move_walk = {FRAME_walk03, FRAME_walk14, infantry_frames_walk, NULL};

void infantry_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &infantry_move_walk;
}

mframe_t infantry_frames_run [] =
{
	ai_run, 10, NULL,
	ai_run, 20, NULL,
	ai_run, 5,  NULL,
	ai_run, 7,  NULL,
	ai_run, 30, NULL,
	ai_run, 35, NULL,
	ai_run, 2,  NULL,
	ai_run, 6,  NULL
};
mmove_t infantry_move_run = {FRAME_run01, FRAME_run08, infantry_frames_run, NULL};

void infantry_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &infantry_move_stand;
	else
		self->monsterinfo.currentmove = &infantry_move_run;
}


mframe_t infantry_frames_pain1 [] =
{
	ai_move, -3, NULL,
	ai_move, -2, NULL,
	ai_move, -1, NULL,
	ai_move, -2, NULL,
	ai_move, -1, NULL,
	ai_move, 1,  NULL,
	ai_move, -1, NULL,
	ai_move, 1,  NULL,
	ai_move, 6,  NULL,
	ai_move, 2,  NULL
};
mmove_t infantry_move_pain1 = {FRAME_pain101, FRAME_pain110, infantry_frames_pain1, infantry_run};

mframe_t infantry_frames_pain2 [] =
{
	ai_move, -3, NULL,
	ai_move, -3, NULL,
	ai_move, 0,  NULL,
	ai_move, -1, NULL,
	ai_move, -2, NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 2,  NULL,
	ai_move, 5,  NULL,
	ai_move, 2,  NULL
};
mmove_t infantry_move_pain2 = {FRAME_pain201, FRAME_pain210, infantry_frames_pain2, infantry_run};

void infantry_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	int		n;

	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;
	
	if (skill->value > 1)  
		return;		// no pain anims in nightmare (CW: or hard)

	if (damage <= 10)	//CW: shrug off low damage
		return;

	n = rand() % 2;
	if (n == 0)
	{
		self->monsterinfo.currentmove = &infantry_move_pain1;
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	}
	else
	{
		self->monsterinfo.currentmove = &infantry_move_pain2;
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	}
}


vec3_t	aimangles[] =
{
	0.0, 5.0, 0.0,
	10.0, 15.0, 0.0,
	20.0, 25.0, 0.0,
	25.0, 35.0, 0.0,
	30.0, 40.0, 0.0,
	30.0, 45.0, 0.0,
	25.0, 50.0, 0.0,
	20.0, 40.0, 0.0,
	15.0, 35.0, 0.0,
	40.0, 35.0, 0.0,
	70.0, 35.0, 0.0,
	90.0, 35.0, 0.0
};

//CW+++ Monsters flagged as 'special' fire blaster bolts instead of bullets. Melty melty!
void InfantryHyperBlaster (edict_t *self)
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
	int		effect;

	if (self->s.frame == FRAME_attak111)
	{
		if (!self->enemy || !self->enemy->inuse)
			return;

		flash_number = MZ2_INFANTRY_BLASTER_1;
		if (self->moreflags-- <= 0)
		{
			effect = EF_HYPERBLASTER;
			self->moreflags = 2;
		}
		else
			effect = 0;

		AngleVectors(self->s.angles, forward, right, NULL);
		G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);
		VectorCopy(self->enemy->s.origin, end);
		end[2] += self->enemy->viewheight - 4;

//		Lazarus: fog reduction of accuracy.

		if (self->monsterinfo.visibility < FOG_CANSEEGOOD)
		{
			end[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
			end[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
			end[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		}
		VectorSubtract(end, start, aim);

//		Lead the target...

		bolt_speed = BOLT_BASESPEED + (BOLT_SKILLSPEED * skill->value);
		time = VectorLength(aim) / bolt_speed;
		end[0] += time * self->enemy->velocity[0];
		end[1] += time * self->enemy->velocity[1];
		VectorSubtract(end, start, aim);

//		...and degrade accuracy based on skill level and target velocity.

		vectoangles(aim, dir);
		AngleVectors(dir, forward, right, up);
		xy_velsq = (self->enemy->velocity[0]*self->enemy->velocity[0]) + (self->enemy->velocity[1]*self->enemy->velocity[1]);
		r = crandom() * (AIM_R_BASE - (AIM_R_SKILL * skill->value)) * (xy_velsq / 90000);
		VectorMA(start, 8192, forward, end);
		VectorMA(end, r, right, end);
		VectorSubtract(end, start, aim);
	}
	else
	{
		bolt_speed = BOLT_BASESPEED;
		flash_number = MZ2_INFANTRY_BLASTER_2 + (self->s.frame - FRAME_death211);
		if (self->moreflags-- <= 0)
		{
			effect = EF_HYPERBLASTER;
			self->moreflags = 2;
		}
		else
			effect = 0;

		AngleVectors(self->s.angles, forward, right, NULL);
		G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);
		VectorSubtract(self->s.angles, aimangles[flash_number-MZ2_INFANTRY_BLASTER_2], dir);
		AngleVectors(dir, aim, NULL, NULL);
	}

	gi.sound(self, CHAN_WEAPON, sound_blaster, 1, ATTN_NORM, 0);
	monster_fire_blaster(self, start, aim, 2, bolt_speed, flash_number, effect, BLASTER_ORANGE);	//CW: speed was 1000
}
//CW---

void InfantryMachineGun (edict_t *self)
{
	vec3_t	start, target;
	vec3_t	forward, right;
	vec3_t	vec;
	int		flash_number;

	if (self->s.frame == FRAME_attak111)
	{
		flash_number = MZ2_INFANTRY_MACHINEGUN_1;
		AngleVectors (self->s.angles, forward, right, NULL);
		G_ProjectSource (self->s.origin, monster_flash_offset[flash_number], forward, right, start);

		if (self->enemy && self->enemy->inuse)
		{
			VectorMA (self->enemy->s.origin, -0.2, self->enemy->velocity, target);
			target[2] += self->enemy->viewheight;

			// Lazarus fog reduction of accuracy
			if (self->monsterinfo.visibility < FOG_CANSEEGOOD)
			{
				target[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
				target[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
				target[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
			}

			VectorSubtract (target, start, forward);
			VectorNormalize (forward);
		}
		else
		{
			AngleVectors (self->s.angles, forward, right, NULL);
		}
	}
	else
	{
		flash_number = MZ2_INFANTRY_MACHINEGUN_2 + (self->s.frame - FRAME_death211);

		AngleVectors (self->s.angles, forward, right, NULL);
		G_ProjectSource (self->s.origin, monster_flash_offset[flash_number], forward, right, start);

		VectorSubtract (self->s.angles, aimangles[flash_number-MZ2_INFANTRY_MACHINEGUN_2], vec);
		AngleVectors (vec, forward, NULL, NULL);
	}

	monster_fire_bullet (self, start, forward, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}

void infantry_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_BODY, sound_sight, 1, ATTN_NORM, 0);
}

void infantry_dead (edict_t *self)
{
	// Lazarus: Stupid... if flies aren't set by M_FlyCheck, monster_think
	//          will cause us to come back here over and over and over
	//          until flies ARE set or monster is gibbed.
	//          This line fixes that:
	self->nextthink = 0;

//CW+++ Reset from roll dodge.
	self->s.angles[2] = 0.0;
	self->avelocity[2] = 0.0;
//CW---

	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity (self);
	M_FlyCheck (self);

	// Lazarus monster fade
	if (world->effects & FX_WORLDSPAWN_CORPSEFADE)
	{
		self->think = FadeDieSink;
		self->nextthink = level.time + corpse_fadetime->value;
	}
}

//CW++
void blood_spurt(edict_t *self)
{
	vec3_t	org_neck;

	org_neck[0] = self->s.origin[0];
	org_neck[1] = self->s.origin[1];
	org_neck[2] = self->s.origin[2] + 24.0;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BLOOD);
	gi.WritePosition(org_neck);
	gi.WriteDir(VEC_UPWARDS);
	gi.multicast(self->s.origin, MULTICAST_PVS);
}
//CW--

mframe_t infantry_frames_death1 [] =
{
	ai_move, -4, NULL,
	ai_move, 0,  blood_spurt,					//CW
	ai_move, 0,  NULL,
	ai_move, -1, NULL,
	ai_move, -4, blood_spurt,					//CW
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  blood_spurt,					//CW
	ai_move, -1, NULL,
	ai_move, 3,  NULL,
	ai_move, 1,  blood_spurt,					//CW
	ai_move, 1,  NULL,
	ai_move, -2, NULL,
	ai_move, 2,  NULL,
	ai_move, 2,  NULL,
	ai_move, 9,  NULL,
	ai_move, 9,  NULL,
	ai_move, 5,  NULL,
	ai_move, -3, NULL,
	ai_move, -3, NULL
};
mmove_t infantry_move_death1 = {FRAME_death101, FRAME_death120, infantry_frames_death1, infantry_dead};

// Off with his head (machinegun)
mframe_t infantry_frames_death2 [] =
{
	ai_move, 0,   blood_spurt,					//CW
	ai_move, 1,   NULL,
	ai_move, 5,   NULL,
	ai_move, -1,  blood_spurt,					//CW
	ai_move, 0,   NULL,
	ai_move, 1,   NULL,
	ai_move, 1,   blood_spurt,					//CW
	ai_move, 4,   NULL,
	ai_move, 3,   NULL,
	ai_move, 0,   blood_spurt,					//CW
	ai_move, -2,  InfantryMachineGun,
	ai_move, -2,  InfantryMachineGun,
	ai_move, -3,  InfantryMachineGun,
	ai_move, -1,  InfantryMachineGun,
	ai_move, -2,  InfantryMachineGun,
	ai_move, 0,   InfantryMachineGun,
	ai_move, 2,   InfantryMachineGun,
	ai_move, 2,   InfantryMachineGun,
	ai_move, 3,   InfantryMachineGun,
	ai_move, -10, InfantryMachineGun,
	ai_move, -7,  InfantryMachineGun,
	ai_move, -8,  InfantryMachineGun,
	ai_move, -6,  NULL,
	ai_move, 4,   NULL,
	ai_move, 0,   NULL
};
mmove_t infantry_move_death2 = {FRAME_death201, FRAME_death225, infantry_frames_death2, infantry_dead};

mframe_t infantry_frames_death3 [] =
{
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, -6,  NULL,
	ai_move, -11, NULL,
	ai_move, -3,  NULL,
	ai_move, -11, NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL
};
mmove_t infantry_move_death3 = {FRAME_death301, FRAME_death309, infantry_frames_death3, infantry_dead};

//CW+++
// Off with his head (hyperblaster)
mframe_t infantry_frames_death4 [] =
{
	ai_move, 0,   blood_spurt,					//CW
	ai_move, 1,   NULL,
	ai_move, 5,   NULL,
	ai_move, -1,  blood_spurt,					//CW
	ai_move, 0,   NULL,
	ai_move, 1,   NULL,
	ai_move, 1,   blood_spurt,					//CW
	ai_move, 4,   NULL,
	ai_move, 3,   NULL,
	ai_move, 0,   blood_spurt,					//CW
	ai_move, -2,  InfantryHyperBlaster,
	ai_move, -2,  InfantryHyperBlaster,
	ai_move, -3,  InfantryHyperBlaster,
	ai_move, -1,  InfantryHyperBlaster,
	ai_move, -2,  InfantryHyperBlaster,
	ai_move, 0,   InfantryHyperBlaster,
	ai_move, 2,   InfantryHyperBlaster,
	ai_move, 2,   InfantryHyperBlaster,
	ai_move, 3,   InfantryHyperBlaster,
	ai_move, -10, InfantryHyperBlaster,
	ai_move, -7,  InfantryHyperBlaster,
	ai_move, -8,  InfantryHyperBlaster,
	ai_move, -6,  NULL,
	ai_move, 4,   NULL,
	ai_move, 0,   NULL
};
mmove_t infantry_move_death4 = {FRAME_death201, FRAME_death225, infantry_frames_death4, infantry_dead};

void infantry_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

	self->s.skinnum |= 1;
	self->monsterinfo.power_armor_type = POWER_ARMOR_NONE;
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
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	n = rand() % 4;													//CW: was 3

	if (n == 0)
	{
		self->monsterinfo.currentmove = &infantry_move_death1;
		gi.sound (self, CHAN_VOICE, sound_die2, 1, ATTN_NORM, 0);
	}
	else if (n > 1)													//CW: more likely
	{
		if (self->spawnflags & SF_MONSTER_SPECIAL)					//CW: wild HB firing
			self->monsterinfo.currentmove = &infantry_move_death4;
		else
			self->monsterinfo.currentmove = &infantry_move_death2;
		gi.sound (self, CHAN_VOICE, sound_die1, 1, ATTN_NORM, 0);
	}
	else
	{
		self->monsterinfo.currentmove = &infantry_move_death3;
		gi.sound (self, CHAN_VOICE, sound_die2, 1, ATTN_NORM, 0);
	}
}

void infantry_duck_down (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32;
	self->takedamage = DAMAGE_YES;
	gi.linkentity(self);
}

void infantry_duck_hold (edict_t *self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

void infantry_duck_up (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = DAMAGE_AIM;
	gi.linkentity(self);
}

mframe_t infantry_frames_duck [] =
{
	ai_move, -2, infantry_duck_down,
	ai_move, -5, infantry_duck_hold,
	ai_move, 3,  NULL,
	ai_move, 4,  infantry_duck_up,
	ai_move, 0,  NULL
};
mmove_t infantry_move_duck = {FRAME_duck01, FRAME_duck05, infantry_frames_duck, infantry_run};

//CW+++ Dodge by rolling along the ground.
void infantry_start_roll(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->bobframe = 0;
	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.pausetime = level.time + INF_ROLL_TIME + FRAMETIME;
	self->avelocity[2] = self->moreflags * (360.0 / INF_ROLL_TIME);
	self->bob = self->s.origin[2];
	self->s.origin[2] += inf_roll_z[self->bobframe++];
	gi.linkentity(self);
}

void infantry_roll(edict_t *self)
{
	if (level.time > self->monsterinfo.pausetime)
	{
		self->avelocity[2] = 0.0;
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	}
	else
	{
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
		self->s.origin[2] += inf_roll_z[self->bobframe++];
		ai_strafe(self, INF_ROLL_DIST);
		gi.linkentity(self);
	}
}

void infantry_end_roll(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = DAMAGE_AIM;
	self->s.origin[2] = self->bob;
	self->s.angles[2] = 0;
	self->avelocity[2] = 0.0;
	gi.linkentity(self);
}

mframe_t infantry_frames_roll[] =
{
	ai_strafe, INF_ROLL_DIST, infantry_start_roll,
	ai_move,  0, infantry_roll,
	ai_move,  0, NULL,
	ai_move,  0, NULL,
	ai_move,  0, infantry_end_roll
};
mmove_t infantry_move_roll = {FRAME_duck01, FRAME_duck05, infantry_frames_roll, infantry_run};
//CW---

void infantry_dodge(edict_t *self, edict_t *attacker, float eta)
{
//CW++
	vec3_t		v_rollend;
	trace_t		trace;
	qboolean	leftOK = false;
	qboolean	rightOK = false;	
//CW--

	if (!self->enemy)
		self->enemy = attacker;

//CW++
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	if (self->busy)			//don't dodge if currently firing
		return;
//CW--

	if (random() > (0.5 + (0.1 * skill->value)))	//CW: was 0.25; changed due to roll-dodging
		return;

//CW++
//	Check that left hand side is clear for roll dodge.

	v_rollend[0] = self->s.origin[0] + ((INF_ROLL_TIME/FRAMETIME) * INF_ROLL_DIST * cos((self->s.angles[1]+90)*PI/180));
	v_rollend[1] = self->s.origin[1] + ((INF_ROLL_TIME/FRAMETIME) * INF_ROLL_DIST * sin((self->s.angles[1]+90)*PI/180));
	v_rollend[2] = self->s.origin[2];
	trace = gi.trace(self->s.origin, self->mins, self->maxs, v_rollend, self, MASK_MONSTERSOLID);
	if (trace.fraction == 1.0)
		leftOK = true;

//	Check that right hand side is clear for roll dodge.

	v_rollend[0] = self->s.origin[0] + ((INF_ROLL_TIME/FRAMETIME) * INF_ROLL_DIST * cos((self->s.angles[1]-90)*PI/180));
	v_rollend[1] = self->s.origin[1] + ((INF_ROLL_TIME/FRAMETIME) * INF_ROLL_DIST * sin((self->s.angles[1]-90)*PI/180));
	trace = gi.trace(self->s.origin, self->mins, self->maxs, v_rollend, self, MASK_MONSTERSOLID);
	if (trace.fraction == 1.0)
		rightOK = true;

//	If there's no room to roll, then duck...

	if (!(leftOK || rightOK))
	{
		if (self->count > 2)	// player has learned to aim low, so don't bother ducking
			return;

		if (eta > 1.0)			// would be vulnerable for too long
			return;
		else
		{
			self->monsterinfo.pausetime = level.time + eta + 0.5;
			self->monsterinfo.currentmove = &infantry_move_duck;
		}
	}

//	...otherwise, decide which way to roll, and go for it!

	else
	{
		self->moreflags = 0;
		self->pain_debounce_time = level.time + INF_ROLL_TIME + FRAMETIME;
		self->monsterinfo.currentmove = &infantry_move_roll;

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
	}
//CW--
}

void infantry_cock_gun (edict_t *self)
{
	int n;
	
	self->busy = true;		//CW: ensure dodging routines don't get executed
	gi.sound(self, CHAN_WEAPON, sound_weapon_cock, 1, ATTN_NORM, 0);
	n = (rand() & 15) + 10 + (5 * skill->value);	//CW: make them more brutal on higher skills (was just +10)			
	self->monsterinfo.pausetime = level.time + (n * FRAMETIME);
}

void infantry_fire (edict_t *self)
{
//CW+++ If special, fire HB instead.
	if (self->spawnflags & SF_MONSTER_SPECIAL)
		InfantryHyperBlaster(self);
	else
//CW---
		InfantryMachineGun(self);

	if (level.time >= self->monsterinfo.pausetime)
	{
		self->busy = false;		//CW: reset so dodging routines can be executed as normal
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	}
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

mframe_t infantry_frames_attack1 [] =
{
	ai_charge, 4,  NULL,
	ai_charge, -1, NULL,
	ai_charge, -1, NULL,
	ai_charge, 0,  infantry_cock_gun,
	ai_charge, -1, NULL,
	ai_charge, 1,  NULL,
	ai_charge, 1,  NULL,
	ai_charge, 2,  NULL,
	ai_charge, -2, NULL,
	ai_charge, -3, NULL,
	ai_charge, 1,  infantry_fire,
	ai_charge, 5,  NULL,
	ai_charge, -1, NULL,
	ai_charge, -2, NULL,
	ai_charge, -3, NULL
};

mmove_t infantry_move_attack1 = {FRAME_attak101, FRAME_attak115, infantry_frames_attack1, infantry_run};


void infantry_swing (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_punch_swing, 1, ATTN_NORM, 0);
}

void infantry_smack (edict_t *self)
{
	vec3_t	aim;

	VectorSet (aim, MELEE_DISTANCE, 0, 0);
	if (fire_hit (self, aim, (5 + (rand() % 5)), 50))
		gi.sound (self, CHAN_WEAPON, sound_punch_hit, 1, ATTN_NORM, 0);
}

mframe_t infantry_frames_attack2 [] =
{
	ai_charge, 3, NULL,
	ai_charge, 6, NULL,
	ai_charge, 0, infantry_swing,
	ai_charge, 8, NULL,
	ai_charge, 5, NULL,
	ai_charge, 8, infantry_smack,
	ai_charge, 6, NULL,
	ai_charge, 3, NULL,
};
mmove_t infantry_move_attack2 = {FRAME_attak201, FRAME_attak208, infantry_frames_attack2, infantry_run};

void infantry_attack(edict_t *self)
{
//CW++	Fix potential crashes
	if (!self->enemy)
		return;
//CW--

	if (range(self, self->enemy) == RANGE_MELEE)
		self->monsterinfo.currentmove = &infantry_move_attack2;
	else
		self->monsterinfo.currentmove = &infantry_move_attack1;
}

//Jump
mframe_t infantry_frames_jump [] =
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
mmove_t infantry_move_jump = { FRAME_run01, FRAME_run08, infantry_frames_jump, infantry_run };

void infantry_jump (edict_t *self)
{
	self->monsterinfo.currentmove = &infantry_move_jump;
}

/*QUAKED monster_infantry (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_infantry (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	sound_pain1 = gi.soundindex ("infantry/infpain1.wav");
	sound_pain2 = gi.soundindex ("infantry/infpain2.wav");
	sound_die1 = gi.soundindex ("infantry/infdeth1.wav");
	sound_die2 = gi.soundindex ("infantry/infdeth2.wav");
	sound_punch_swing = gi.soundindex ("infantry/infatck2.wav");
	sound_punch_hit = gi.soundindex ("infantry/melee2.wav");
	sound_sight = gi.soundindex ("infantry/infsght1.wav");
	sound_search = gi.soundindex ("infantry/infsrch1.wav");
	sound_idle = gi.soundindex ("infantry/infidle1.wav");

//CW+++ Hyperblaster sounds for special monsters.
	if (self->spawnflags & SF_MONSTER_SPECIAL)
	{
		sound_blaster = gi.soundindex("hover/hovatck1.wav");
		sound_weapon_cock = gi.soundindex("infantry/infhb1.wav");
	}
	else
		sound_weapon_cock = gi.soundindex("infantry/infatck3.wav");
//CW---

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	// Lazarus: special purpose skins
	if (self->style)
	{
		PatchMonsterModel("models/monsters/infantry/tris.md2");
		self->s.skinnum = self->style * 2;
	}

	self->s.modelindex = gi.modelindex("models/monsters/infantry/tris.md2");
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 32);

	// Lazarus: mapper-configurable health
	if (!self->health)
		self->health = 100;
	if (!self->gib_health)
		self->gib_health = -100;	//CW: was -40
	if (!self->mass)
		self->mass = 200;

	self->pain = infantry_pain;
	self->die = infantry_die;

	self->monsterinfo.stand = infantry_stand;
	self->monsterinfo.walk = infantry_walk;
	self->monsterinfo.run = infantry_run;
	self->monsterinfo.dodge = infantry_dodge;
	self->monsterinfo.attack = infantry_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = infantry_sight;
	self->monsterinfo.idle = infantry_fidget;

	if (monsterjump->value)
	{
		self->monsterinfo.jump = infantry_jump;
		self->monsterinfo.jumpup = 48;
		self->monsterinfo.jumpdn = 160;
	}

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

	if (!self->monsterinfo.flies)
		self->monsterinfo.flies = 0.20;	//CW: was 0.40

	gi.linkentity (self);
	self->monsterinfo.currentmove = &infantry_move_stand;

	if (self->health < 0)
	{
		mmove_t	*deathmoves[] = {&infantry_move_death1,
			                     &infantry_move_death2,
								 &infantry_move_death3,
								 NULL};
		M_SetDeath(self,(mmove_t **)&deathmoves);
	}

	self->common_name = "Enforcer";
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start (self);
}

