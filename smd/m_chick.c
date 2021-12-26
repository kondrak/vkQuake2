/*
==============================================================================

chick

==============================================================================
*/

#include "g_local.h"
#include "m_chick.h"

//CW+++
#define CHICK_FLIP_TIME		0.5		//duration of backflip (seconds)
#define CHICK_FLIP_SPEED	100		//horizontal velocity for backflip
//CW---

qboolean visible (edict_t *self, edict_t *other);

void chick_stand (edict_t *self);
void chick_run (edict_t *self);
void chick_reslash(edict_t *self);
void chick_rerocket(edict_t *self);
void chick_attack1(edict_t *self);

static int	sound_missile_prelaunch;
static int	sound_missile_launch;
static int	sound_melee_swing;
static int	sound_melee_hit;
static int	sound_missile_reload;
static int	sound_death1;
static int	sound_death2;
static int	sound_fall_down;
static int	sound_idle1;
static int	sound_idle2;
static int	sound_pain1;
static int	sound_pain2;
static int	sound_pain3;
static int	sound_sight;
static int	sound_search;

float chick_flip_z[6] = {60, 48, 28, 20, -10, -15};	//CW: z-axis origin offsets during flip frames

void ChickMoan (edict_t *self)
{
	if(!(self->spawnflags & SF_MONSTER_AMBUSH))
	{
		if (random() < 0.5)
			gi.sound (self, CHAN_VOICE, sound_idle1, 1, ATTN_IDLE, 0);
		else
			gi.sound (self, CHAN_VOICE, sound_idle2, 1, ATTN_IDLE, 0);
	}
}

mframe_t chick_frames_fidget [] =
{
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  ChickMoan,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL,
	ai_stand, 0,  NULL
};
mmove_t chick_move_fidget = {FRAME_stand201, FRAME_stand230, chick_frames_fidget, chick_stand};

void chick_fidget (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return;
	if (random() <= 0.3)
		self->monsterinfo.currentmove = &chick_move_fidget;
}

mframe_t chick_frames_stand [] =
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
	ai_stand, 0, chick_fidget,

};
mmove_t chick_move_stand = {FRAME_stand101, FRAME_stand130, chick_frames_stand, NULL};

void chick_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &chick_move_stand;
}

mframe_t chick_frames_start_run [] =
{
	ai_run, 1,  NULL,
	ai_run, 0,  NULL,
	ai_run, 0,	 NULL,
	ai_run, -1, NULL, 
	ai_run, -1, NULL, 
	ai_run, 0,  NULL,
	ai_run, 1,  NULL,
	ai_run, 3,  NULL,
	ai_run, 6,	 NULL,
	ai_run, 3,	 NULL
};
mmove_t chick_move_start_run = {FRAME_walk01, FRAME_walk10, chick_frames_start_run, chick_run};

mframe_t chick_frames_run [] =
{
	ai_run, 6,	NULL,
	ai_run, 8,  NULL,
	ai_run, 13, NULL,
	ai_run, 5,  NULL,
	ai_run, 7,  NULL,
	ai_run, 4,  NULL,
	ai_run, 11, NULL,
	ai_run, 5,  NULL,
	ai_run, 9,  NULL,
	ai_run, 7,  NULL

};

mmove_t chick_move_run = {FRAME_walk11, FRAME_walk20, chick_frames_run, NULL};

mframe_t chick_frames_walk [] =
{
	ai_walk, 6,	 NULL,
	ai_walk, 8,  NULL,
	ai_walk, 13, NULL,
	ai_walk, 5,  NULL,
	ai_walk, 7,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 11, NULL,
	ai_walk, 5,  NULL,
	ai_walk, 9,  NULL,
	ai_walk, 7,  NULL
};

mmove_t chick_move_walk = {FRAME_walk11, FRAME_walk20, chick_frames_walk, NULL};

void chick_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &chick_move_walk;
}

void chick_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.currentmove = &chick_move_stand;
		return;
	}

	if (self->monsterinfo.currentmove == &chick_move_walk ||
		self->monsterinfo.currentmove == &chick_move_start_run)
	{
		self->monsterinfo.currentmove = &chick_move_run;
	}
	else
	{
		self->monsterinfo.currentmove = &chick_move_start_run;
	}
}

mframe_t chick_frames_pain1 [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t chick_move_pain1 = {FRAME_pain101, FRAME_pain105, chick_frames_pain1, chick_run};

mframe_t chick_frames_pain2 [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t chick_move_pain2 = {FRAME_pain201, FRAME_pain205, chick_frames_pain2, chick_run};

mframe_t chick_frames_pain3 [] =
{
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, -6,	NULL,
	ai_move, 3,		NULL,
	ai_move, 11,	NULL,
	ai_move, 3,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 4,		NULL,
	ai_move, 1,		NULL,
	ai_move, 0,		NULL,
	ai_move, -3,	NULL,
	ai_move, -4,	NULL,
	ai_move, 5,		NULL,
	ai_move, 7,		NULL,
	ai_move, -2,	NULL,
	ai_move, 3,		NULL,
	ai_move, -5,	NULL,
	ai_move, -2,	NULL,
	ai_move, -8,	NULL,
	ai_move, 2,		NULL
};
mmove_t chick_move_pain3 = {FRAME_pain301, FRAME_pain321, chick_frames_pain3, chick_run};

void chick_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	float	r;

	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;

	r = random();
	if (r < 0.33)
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else if (r < 0.66)
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_pain3, 1, ATTN_NORM, 0);

	if (skill->value > 1)  
		return;				// no pain anims in nightmare (CW: or hard)

	if (damage <= 10)		//CW: shrug off low damage
		return;

	if (damage <= 20)		//CW: increased damage resistance
		self->monsterinfo.currentmove = &chick_move_pain1;
	else if (damage <= 35)	//CW: increased damage resistance
		self->monsterinfo.currentmove = &chick_move_pain2;
	else
		self->monsterinfo.currentmove = &chick_move_pain3;
}

void chick_dead (edict_t *self)
{
//CW+++ Reset from backflip.
	self->s.angles[0] = 0.0;
	self->avelocity[0] = 0.0;
//CW---

	VectorSet (self->mins, -16, -16, 0);
	VectorSet (self->maxs, 16, 16, 16);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
	M_FlyCheck (self);

	// Lazarus monster fade
	if (world->effects & FX_WORLDSPAWN_CORPSEFADE)
	{
		self->think = FadeDieSink;
		self->nextthink = level.time + corpse_fadetime->value;
	}
}

mframe_t chick_frames_death2 [] =
{
	ai_move, -6, NULL,
	ai_move, 0,  NULL,
	ai_move, -1,  NULL,
	ai_move, -5, NULL,
	ai_move, 0, NULL,
	ai_move, -1,  NULL,
	ai_move, -2,  NULL,
	ai_move, 1,  NULL,
	ai_move, 10, NULL,
	ai_move, 2,  NULL,
	ai_move, 3,  NULL,
	ai_move, 1,  NULL,
	ai_move, 2, NULL,
	ai_move, 0,  NULL,
	ai_move, 3,  NULL,
	ai_move, 3,  NULL,
	ai_move, 1,  NULL,
	ai_move, -3,  NULL,
	ai_move, -5, NULL,
	ai_move, 4, NULL,
	ai_move, 15, NULL,
	ai_move, 14, NULL,
	ai_move, 1, NULL
};
mmove_t chick_move_death2 = {FRAME_death201, FRAME_death223, chick_frames_death2, chick_dead};

mframe_t chick_frames_death1 [] =
{
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, -7, NULL,
	ai_move, 4,  NULL,
	ai_move, 11, NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL
	
};
mmove_t chick_move_death1 = {FRAME_death101, FRAME_death112, chick_frames_death1, chick_dead};

void chick_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	n = rand() % 2;
	if (n == 0)
	{
		self->monsterinfo.currentmove = &chick_move_death1;
		gi.sound (self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
	}
	else
	{
		self->monsterinfo.currentmove = &chick_move_death2;
		gi.sound (self, CHAN_VOICE, sound_death2, 1, ATTN_NORM, 0);
	}
}

void chick_duck_down (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32;
	self->takedamage = DAMAGE_YES;
	gi.linkentity (self);
}

void chick_duck_hold (edict_t *self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

void chick_duck_up (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = DAMAGE_AIM;
	gi.linkentity (self);
}

mframe_t chick_frames_duck [] =
{
	ai_move, 0, chick_duck_down,
	ai_move, 1, NULL,
	ai_move, 4, chick_duck_hold,
	ai_move, -4,  NULL,
	ai_move, -5,  chick_duck_up,
	ai_move, 3, NULL,
	ai_move, 1,  NULL
};
mmove_t chick_move_duck = {FRAME_duck01, FRAME_duck07, chick_frames_duck, chick_run};

//CW+++ Dodge by flipping back and up.
void chick_start_backflip(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	self->bobframe = 0;
	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.pausetime = level.time + CHICK_FLIP_TIME + FRAMETIME;
	self->avelocity[0] = -360 / CHICK_FLIP_TIME;
	self->velocity[0] = (crandom() * 40) - (CHICK_FLIP_SPEED * cos(self->s.angles[1] * PI/180));
	self->velocity[1] = (crandom() * 40) - (CHICK_FLIP_SPEED * sin(self->s.angles[1] * PI/180));
	self->s.origin[2] += chick_flip_z[self->bobframe++];
	gi.linkentity(self);
}

void chick_backflip(edict_t *self)
{
	if (level.time > self->monsterinfo.pausetime)
	{
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		self->avelocity[0] = 0.0;
		gi.linkentity(self);
	}
	else
	{
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
		self->s.origin[2] += chick_flip_z[self->bobframe++];
		gi.linkentity(self);
	}
}

void chick_end_backflip(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = DAMAGE_AIM;
	self->s.angles[0] = 0.0;
	self->avelocity[0] = 0.0;
	gi.linkentity (self);
}

mframe_t chick_frames_backflip[] =
{
	ai_move, -10, chick_start_backflip,
	ai_move,  0, chick_backflip,
	ai_move,  0, NULL,
	ai_move,  0, NULL,
	ai_move,  2, chick_end_backflip
};
mmove_t chick_move_backflip = {FRAME_duck03, FRAME_duck07, chick_frames_backflip, chick_run};
//CW---

void chick_dodge(edict_t *self, edict_t *attacker, float eta)
{
	if (!self->enemy)
		self->enemy = attacker;

//CW+++
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;

	if (self->busy)		//don't dodge if currently firing
		return;
//CW---

	if (random() > (0.5 + 0.1*skill->value))	//CW: was 0.25; changed due to backflipping
		return;

//CW+++
//	Mostly backflip to dodge.

	if ((random() < 0.75) || (eta > 1.0))
		self->monsterinfo.currentmove = &chick_move_backflip;
	else
	{
		if (eta > 1.0)			// would be vulnerable for too long
			return;
		else
		{
			self->monsterinfo.pausetime = level.time + eta + 0.5;	//CW: was + 1, in duck_down()
//CW---
			self->monsterinfo.currentmove = &chick_move_duck;
		}
	}
}

void ChickSlash (edict_t *self)
{
	vec3_t	aim;

	VectorSet (aim, MELEE_DISTANCE, self->mins[0], 10);
	gi.sound (self, CHAN_WEAPON, sound_melee_swing, 1, ATTN_NORM, 0);
	fire_hit (self, aim, (10 + (rand() %6)), 100);
}


void ChickRocket (edict_t *self)
{
	// DWH: Added skill level-dependent rocket speed, leading target, suicide prevention,
	//      target elevation dependent target location, and homing rockets

	trace_t	trace;
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;
	int		rocketSpeed;

	if (!self->enemy || !self->enemy->inuse)	//CW
		return;

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_CHICK_ROCKET_1], forward, right, start);

	if ((self->spawnflags & SF_MONSTER_SPECIAL))
		rocketSpeed = 400; // DWH: Homing rockets are tougher if slow
	else
		rocketSpeed = 500 + (100 * skill->value);	// PGM rock & roll.... :)

	if (visible(self, self->enemy))
	{
		VectorCopy (self->enemy->s.origin, vec);
		if (random() < 0.66 || (start[2] < self->enemy->absmin[2]))
			vec[2] += self->enemy->viewheight;
		else
			vec[2] = self->enemy->absmin[2];

		VectorSubtract (vec, start, dir);

		// Lazarus fog reduction of accuracy
		if (self->monsterinfo.visibility < FOG_CANSEEGOOD)
		{
			vec[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
			vec[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
			vec[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		}
		
		// lead target, but not if using homers
		// 20, 35, 50, 65 chance of leading
		// DWH: Switched this around from Rogue code... it led target more often
		//      for Easy, which seemed backwards

		if ((random() < (0.2 + skill->value * 0.15) ) && !(self->spawnflags & SF_MONSTER_SPECIAL))
		{
			float	dist;
			float	time;

			dist = VectorLength (dir);
			time = dist/rocketSpeed;
			VectorMA(vec, time, self->enemy->velocity, vec);
			VectorSubtract(vec, start, dir);
		}
	}
	else
	{
		// Fire at feet of last known position
		VectorCopy(self->monsterinfo.last_sighting,vec);
		vec[2] += self->enemy->mins[2];
		VectorSubtract(vec, start, dir);
	}

	VectorNormalize(dir);

	// paranoia, make sure we're not shooting a target right next to us
	trace = gi.trace(start, vec3_origin, vec3_origin, vec, self, MASK_SHOT);
	if (trace.ent == self->enemy || trace.ent == world)
	{
		VectorSubtract(trace.endpos,start,vec);
		if (VectorLength(vec) > MELEE_DISTANCE)
		{
			if (trace.fraction > 0.5 || (trace.ent && trace.ent->client))
				monster_fire_rocket (self, start, dir, 50, rocketSpeed, MZ2_CHICK_ROCKET_1, (self->spawnflags & SF_MONSTER_SPECIAL ? self->enemy : NULL));
		}
	}
}	

void Chick_PreAttack1 (edict_t *self)
{
	self->busy = true;		//CW: ensure dodging routines don't get executed
	gi.sound (self, CHAN_VOICE, sound_missile_prelaunch, 1, ATTN_NORM, 0);
}

void ChickReload (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_missile_reload, 1, ATTN_NORM, 0);
}

void chick_skip_frames (edict_t *self)
{
	if (skill->value >= 1)
	{
		if (self->s.frame == FRAME_attak102)
			self->s.frame = FRAME_attak103;
		if (self->s.frame == FRAME_attak105)
			self->s.frame = FRAME_attak106;
	}
	if (skill->value > 1)
		if (self->s.frame == FRAME_attak109)
			self->s.frame = FRAME_attak112;
}

mframe_t chick_frames_start_attack1 [] =
{
	ai_charge, 0,	Chick_PreAttack1,
	ai_charge, 0,	chick_skip_frames,
	ai_charge, 0,	chick_skip_frames,
	ai_charge, 4,	chick_skip_frames,
	ai_charge, 0,	chick_skip_frames,
	ai_charge, -3,  chick_skip_frames,
	ai_charge, 3,	chick_skip_frames,
	ai_charge, 5,	chick_skip_frames,
	ai_charge, 7,	chick_skip_frames,
	ai_charge, 0,	chick_skip_frames,
	ai_charge, 0,	chick_skip_frames,
	ai_charge, 0,	chick_skip_frames,
	ai_charge, 0,	chick_attack1
};
mmove_t chick_move_start_attack1 = {FRAME_attak101, FRAME_attak113, chick_frames_start_attack1, NULL};


mframe_t chick_frames_attack1 [] =
{
	ai_charge, 19,	ChickRocket,
	ai_charge, -6,	NULL,
	ai_charge, -5,	NULL,
	ai_charge, -2,	NULL,
	ai_charge, -7,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 1,	NULL,
	ai_charge, 10,	ChickReload,
	ai_charge, 4,	NULL,
	ai_charge, 5,	NULL,
	ai_charge, 6,	NULL,
	ai_charge, 6,	NULL,
	ai_charge, 4,	NULL,
	ai_charge, 3,	chick_rerocket
};
mmove_t chick_move_attack1 = {FRAME_attak114, FRAME_attak127, chick_frames_attack1, NULL};

mframe_t chick_frames_end_attack1 [] =
{
	ai_charge, -3,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, -6,	NULL,
	ai_charge, -4,	NULL,
	ai_charge, -2,  NULL
};
mmove_t chick_move_end_attack1 = {FRAME_attak128, FRAME_attak132, chick_frames_end_attack1, chick_run};

void chick_rerocket(edict_t *self)
{
//CW++	Fix potential crashes
	if (!self->enemy)
		return;
//CW--

	if ((self->enemy->health > 0) && (range(self, self->enemy) > RANGE_MELEE) && (visible(self, self->enemy)))
	{
		if (random() <= (0.5 + 0.1*skill->value))	//CW: was just >= 0.6
		{
			self->monsterinfo.currentmove = &chick_move_attack1;
			return;
		}
	}
	self->busy = false;		//CW: reset so dodging routines can be executed as normal
	self->monsterinfo.currentmove = &chick_move_end_attack1;
}

void chick_attack1(edict_t *self)
{
	self->monsterinfo.currentmove = &chick_move_attack1;
}

mframe_t chick_frames_slash [] =
{
	ai_charge, 1,	NULL,
	ai_charge, 7,	ChickSlash,
	ai_charge, -7,	NULL,
	ai_charge, 1,	NULL,
	ai_charge, -1,	NULL,
	ai_charge, 1,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 1,	NULL,
	ai_charge, -2,	chick_reslash
};
mmove_t chick_move_slash = {FRAME_attak204, FRAME_attak212, chick_frames_slash, NULL};

mframe_t chick_frames_end_slash [] =
{
	ai_charge, -6,	NULL,
	ai_charge, -1,	NULL,
	ai_charge, -6,	NULL,
	ai_charge, 0,	NULL
};
mmove_t chick_move_end_slash = {FRAME_attak213, FRAME_attak216, chick_frames_end_slash, chick_run};


void chick_reslash(edict_t *self)
{
//CW++	Fix potential crashes
	if (!self->enemy)
		return;
//CW--

	if (self->enemy->health > 0)
	{
		if (range (self, self->enemy) == RANGE_MELEE)
			if (random() <= 0.9)
			{				
				self->monsterinfo.currentmove = &chick_move_slash;
				return;
			}
			else
			{
				self->monsterinfo.currentmove = &chick_move_end_slash;
				return;
			}
	}
	self->monsterinfo.currentmove = &chick_move_end_slash;
}

void chick_slash(edict_t *self)
{
	self->monsterinfo.currentmove = &chick_move_slash;
}


mframe_t chick_frames_start_slash [] =
{	
	ai_charge, 1,	NULL,
	ai_charge, 8,	NULL,
	ai_charge, 3,	NULL
};
mmove_t chick_move_start_slash = {FRAME_attak201, FRAME_attak203, chick_frames_start_slash, chick_slash};



void chick_melee(edict_t *self)
{
	self->monsterinfo.currentmove = &chick_move_start_slash;
}


void chick_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &chick_move_start_attack1;
}

void chick_sight(edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

/*QUAKED monster_chick (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_chick (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	sound_missile_prelaunch	= gi.soundindex ("chick/chkatck1.wav");	
	sound_missile_launch	= gi.soundindex ("chick/chkatck2.wav");	
	sound_melee_swing		= gi.soundindex ("chick/chkatck3.wav");	
	sound_melee_hit			= gi.soundindex ("chick/chkatck4.wav");	
	sound_missile_reload	= gi.soundindex ("chick/chkatck5.wav");	
	sound_death1			= gi.soundindex ("chick/chkdeth1.wav");	
	sound_death2			= gi.soundindex ("chick/chkdeth2.wav");	
	sound_fall_down			= gi.soundindex ("chick/chkfall1.wav");	
	sound_idle1				= gi.soundindex ("chick/chkidle1.wav");	
	sound_idle2				= gi.soundindex ("chick/chkidle2.wav");	
	sound_pain1				= gi.soundindex ("chick/chkpain1.wav");	
	sound_pain2				= gi.soundindex ("chick/chkpain2.wav");	
	sound_pain3				= gi.soundindex ("chick/chkpain3.wav");	
	sound_sight				= gi.soundindex ("chick/chksght1.wav");	
	sound_search			= gi.soundindex ("chick/chksrch1.wav");	

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	// Lazarus: special purpose skins
	if ( self->style )
	{
		PatchMonsterModel("models/monsters/bitch/tris.md2");
		self->s.skinnum = self->style * 2;
	}

	self->s.modelindex = gi.modelindex ("models/monsters/bitch/tris.md2");
	VectorSet (self->mins, -16, -16, 0);
	VectorSet (self->maxs, 16, 16, 56);

	// DWH: mapper-configurable health
	if(!self->health)
		self->health = 175;
	if(!self->gib_health)
		self->gib_health = -100;	//CW: was -70
	if(!self->mass)
		self->mass = 200;

	self->pain = chick_pain;
	self->die = chick_die;

	self->monsterinfo.stand = chick_stand;
	self->monsterinfo.walk = chick_walk;
	self->monsterinfo.run = chick_run;
	self->monsterinfo.dodge = chick_dodge;
	self->monsterinfo.attack = chick_attack;
	self->monsterinfo.melee = chick_melee;
	self->monsterinfo.sight = chick_sight;

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
	//	self->monsterinfo.flies = 0.40;

	self->common_name = "Iron Maiden";
	gi.linkentity (self);

	self->monsterinfo.currentmove = &chick_move_stand;
	if(self->health < 0)
	{
		mmove_t	*deathmoves[] = {&chick_move_death1,
			                     &chick_move_death2,
								 NULL};
		M_SetDeath(self,(mmove_t **)&deathmoves);
	}
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start (self);
}
