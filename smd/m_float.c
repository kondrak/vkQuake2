/*
==============================================================================

floater

==============================================================================
*/

#include "g_local.h"
#include "m_float.h"

//CW+++
#define BOLT_BASESPEED		700
#define BOLT_SKILLSPEED		100
#define AIM_R_BASE			1300
#define AIM_R_SKILL			250
//CW---

static int	sound_attack2;
static int	sound_attack3;
static int	sound_death1;
static int	sound_idle;
static int	sound_pain1;
static int	sound_pain2;
static int	sound_sight;


void floater_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void floater_idle (edict_t *self)
{
	if (!(self->spawnflags & SF_MONSTER_AMBUSH))
		gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}


//void floater_stand1 (edict_t *self);
void floater_dead (edict_t *self);
void floater_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);
void floater_run (edict_t *self);
void floater_wham (edict_t *self);
void floater_zap (edict_t *self);


void floater_fire_blaster (edict_t *self)
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
	int		damage;

	if (!self->enemy || !self->enemy->inuse)	//CW
		return;

	if ((self->s.frame == FRAME_attak104) || (self->s.frame == FRAME_attak107))
		effect = EF_HYPERBLASTER;
	else
		effect = 0;

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_FLOAT_BLASTER_1], forward, right, start);
	VectorCopy (self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;

//	Lazarus: fog reduction of accuracy.

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
	VectorMA(end, time, self->enemy->velocity, end);
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

//	Beefier blaster bolts if monster is flagged as special.

	if (self->spawnflags & SF_MONSTER_SPECIAL)
	{
		effect |= EF_PENT;
		damage = 6;
	}
	else
		damage = 3;
//CW---
	monster_fire_blaster(self, start, aim, damage, bolt_speed, MZ2_FLOAT_BLASTER_1, effect, BLASTER_ORANGE);	//CW: dmg was 1, speed was 1000
}


mframe_t floater_frames_stand1 [] =
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
mmove_t	floater_move_stand1 = {FRAME_stand101, FRAME_stand152, floater_frames_stand1, NULL};

mframe_t floater_frames_stand2 [] =
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
mmove_t	floater_move_stand2 = {FRAME_stand201, FRAME_stand252, floater_frames_stand2, NULL};

void floater_stand (edict_t *self)
{
	if (random() <= 0.5)		
		self->monsterinfo.currentmove = &floater_move_stand1;
	else
		self->monsterinfo.currentmove = &floater_move_stand2;
}

mframe_t floater_frames_activate [] =
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
	ai_move,	0,	NULL
};
mmove_t floater_move_activate = {FRAME_actvat01, FRAME_actvat31, floater_frames_activate, NULL};

mframe_t floater_frames_attack1 [] =
{
	ai_charge,	0,	NULL,			// Blaster attack
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	floater_fire_blaster,			// BOOM (0, -25.8, 32.5)	-- LOOP Starts
	ai_charge,	0,	floater_fire_blaster,
	ai_charge,	0,	floater_fire_blaster,
	ai_charge,	0,	floater_fire_blaster,
	ai_charge,	0,	floater_fire_blaster,
	ai_charge,	0,	floater_fire_blaster,
	ai_charge,	0,	floater_fire_blaster,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL			//							-- LOOP Ends
};
mmove_t floater_move_attack1 = {FRAME_attak101, FRAME_attak114, floater_frames_attack1, floater_run};

mframe_t floater_frames_attack2 [] =
{
	ai_charge,	0,	NULL,			// Claws
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	floater_wham,			// WHAM (0, -45, 29.6)		-- LOOP Starts
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,			//							-- LOOP Ends
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL
};
mmove_t floater_move_attack2 = {FRAME_attak201, FRAME_attak225, floater_frames_attack2, floater_run};

mframe_t floater_frames_attack3 [] =
{
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	floater_zap,		//								-- LOOP Starts
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,		//								-- LOOP Ends
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL
};
mmove_t floater_move_attack3 = {FRAME_attak301, FRAME_attak334, floater_frames_attack3, floater_run};

mframe_t floater_frames_death [] =
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
	ai_move,	0,	NULL
};
mmove_t floater_move_death = {FRAME_death01, FRAME_death13, floater_frames_death, floater_dead};

mframe_t floater_frames_pain1 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t floater_move_pain1 = {FRAME_pain101, FRAME_pain107, floater_frames_pain1, floater_run};

mframe_t floater_frames_pain2 [] =
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
mmove_t floater_move_pain2 = {FRAME_pain201, FRAME_pain208, floater_frames_pain2, floater_run};

mframe_t floater_frames_pain3 [] =
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
mmove_t floater_move_pain3 = {FRAME_pain301, FRAME_pain312, floater_frames_pain3, floater_run};

mframe_t floater_frames_walk [] =
{
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL,
	ai_walk, 5, NULL
};
mmove_t	floater_move_walk = {FRAME_stand101, FRAME_stand152, floater_frames_walk, NULL};

mframe_t floater_frames_run [] =
{
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL,
	ai_run, 13, NULL
};
mmove_t	floater_move_run = {FRAME_stand101, FRAME_stand152, floater_frames_run, NULL};

void floater_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &floater_move_stand1;
	else
		self->monsterinfo.currentmove = &floater_move_run;
}

void floater_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &floater_move_walk;
}

void floater_wham (edict_t *self)
{
	static	vec3_t	aim = {MELEE_DISTANCE, 0, 0};
	gi.sound (self, CHAN_WEAPON, sound_attack3, 1, ATTN_NORM, 0);
	fire_hit (self, aim, 5 + rand() % 6, -50);
}

void floater_zap (edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	origin;
	vec3_t	dir;
	vec3_t	offset;

	if (!self->enemy || !self->enemy->inuse)	//CW
		return;

	VectorSubtract (self->enemy->s.origin, self->s.origin, dir);

	AngleVectors (self->s.angles, forward, right, NULL);
	//FIXME use a flash and replace these two lines with the commented one
	VectorSet (offset, 18.5, -0.9, 10);
	G_ProjectSource (self->s.origin, offset, forward, right, origin);
//	G_ProjectSource (self->s.origin, monster_flash_offset[flash_number], forward, right, origin);

	gi.sound (self, CHAN_WEAPON, sound_attack2, 1, ATTN_NORM, 0);

	//FIXME use the flash, Luke
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_SPLASH);
	gi.WriteByte (32);
	gi.WritePosition (origin);
	gi.WriteDir (dir);
	gi.WriteByte (1);	//sparks
	gi.multicast (origin, MULTICAST_PVS);

	T_Damage (self->enemy, self, self, dir, self->enemy->s.origin, vec3_origin, 5 + rand() % 6, -10, DAMAGE_ENERGY, MOD_UNKNOWN);
}

void floater_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &floater_move_attack1;
}


void floater_melee(edict_t *self)
{
	if (random() < 0.5)		
		self->monsterinfo.currentmove = &floater_move_attack3;
	else
		self->monsterinfo.currentmove = &floater_move_attack2;
}

//CW+++
void floater_dodge(edict_t *self, edict_t *attacker, float eta)
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

void floater_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	int		n;

	if (self->health < (self->max_health / 2))
	{
		self->s.skinnum |= 1;
		if (!(self->fogclip & 2)) //custom bloodtype flag check
			self->blood_type = 3; //sparks and blood
	}

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;

	if (skill->value > 1)  
		return;		// no pain anims in nightmare (CW: or hard)

	if (damage <= 10)	//CW: shrug off low damage
		return;

	n = (rand() + 1) % 3;
	if (n == 0)
	{
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &floater_move_pain1;
	}
	else
	{
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &floater_move_pain2;
	}
}

void floater_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
}

void floater_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int	n;
	// Knightmare- gibs!
	for (n= 0; n < 4; n++)
		ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
	for (n= 0; n < 10; n++)
		ThrowGib (self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);
	for (n= 0; n < 2; n++)
		ThrowGib (self, "models/objects/gibs/gear/tris.md2", damage, GIB_METALLIC);
	gi.sound (self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
	BecomeExplosion1(self);
}

/*QUAKED monster_floater (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_floater (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	sound_attack2 = gi.soundindex ("floater/fltatck2.wav");
	sound_attack3 = gi.soundindex ("floater/fltatck3.wav");
	sound_death1 = gi.soundindex ("floater/fltdeth1.wav");
	sound_idle = gi.soundindex ("floater/fltidle1.wav");
	sound_pain1 = gi.soundindex ("floater/fltpain1.wav");
	sound_pain2 = gi.soundindex ("floater/fltpain2.wav");
	sound_sight = gi.soundindex ("floater/fltsght1.wav");

	gi.soundindex ("floater/fltatck1.wav");

	self->s.sound = gi.soundindex ("floater/fltsrch1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	// Lazarus: special purpose skins
	if ( self->style )
	{
		PatchMonsterModel("models/monsters/float/tris.md2");
		self->s.skinnum = self->style * 2;
	}
	
	self->s.modelindex = gi.modelindex ("models/monsters/float/tris.md2");
	VectorSet (self->mins, -24, -24, -24);
	VectorSet (self->maxs, 24, 24, 32);

	// Lazarus: mapper-configurable health
	if (!self->health)
		self->health = 200;
	if (!self->gib_health)
		self->gib_health = -80;
	if (!self->mass)
		self->mass = 300;

	self->pain = floater_pain;
	self->die = floater_die;

	self->monsterinfo.stand = floater_stand;
	self->monsterinfo.walk = floater_walk;
	self->monsterinfo.run = floater_run;
	self->monsterinfo.dodge = floater_dodge;		//CW:
	self->monsterinfo.attack = floater_attack;
	self->monsterinfo.melee = floater_melee;
	self->monsterinfo.sight = floater_sight;
	self->monsterinfo.idle = floater_idle;

	// Knightmare- added sparks and blood type
	if (!self->blood_type)
		self->blood_type = 2; //sparks
	else
		self->fogclip |= 2; //custom bloodtype flag

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

	self->common_name = "Technician";

	gi.linkentity (self);

	if (self->health < 0)
	{
		mmove_t	*deathmoves[] = {&floater_move_death,
								 NULL};
		if (!M_SetDeath(self,(mmove_t **)&deathmoves))
			self->monsterinfo.currentmove = &floater_move_stand1;
	}
	else
	{
		if (random() <= 0.5)		
			self->monsterinfo.currentmove = &floater_move_stand1;	
		else
			self->monsterinfo.currentmove = &floater_move_stand2;	
	}
	
	self->monsterinfo.scale = MODEL_SCALE;

	flymonster_start (self);
}
