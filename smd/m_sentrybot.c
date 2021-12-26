/*
==============================================================================

SENTRYBOT

==============================================================================
*/

//CW: Added as a new monster.

#include "g_local.h"
#include "m_actor.h"

#define BOLT_BASESPEED		500
#define BOLT_SKILLSPEED		100
#define AIM_R_BASE			1400
#define AIM_R_SKILL			250

static int	sound_pain1;
static int	sound_die1;
static int	sound_sight;
static int	sound_search;
static int	sound_idle;
static int	sound_stand;
static int	sound_move;
static int	sound_jump;
static int	sound_weapon;

// Standing behaviour and animations.

void sentrybot_sound_stand(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_stand, 1, ATTN_IDLE, 0);
}

mframe_t sentrybot_frames_stand [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, sentrybot_sound_stand,
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
	ai_stand, 0, sentrybot_sound_stand,
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
mmove_t sentrybot_move_stand = {FRAME_stand01, FRAME_stand40, sentrybot_frames_stand, NULL};

void sentrybot_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &sentrybot_move_stand;
}

// Idling behaviour.

void sentrybot_idle(edict_t *self)
{
	if (!(self->spawnflags & SF_MONSTER_AMBUSH))
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

// Searching behaviour.

void sentrybot_search (edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

// Enemy sighting behaviour.

void sentrybot_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_BODY, sound_sight, 1, ATTN_NORM, 0);
}

// Walking behaviour and animations.

void sentrybot_sound_move(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_move, 1, ATTN_IDLE, 0);
}

mframe_t sentrybot_frames_walk [] =
{
	ai_walk, 11, NULL,
	ai_walk, 11, NULL,
	ai_walk, 11, NULL,
	ai_walk, 11, NULL,
	ai_walk, 11, NULL,
	ai_walk, 11, sentrybot_sound_move,
};
mmove_t sentrybot_move_walk = {FRAME_run1, FRAME_run6, sentrybot_frames_walk, NULL};

void sentrybot_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &sentrybot_move_walk;
}

// Running behaviour and animations.

mframe_t sentrybot_frames_run [] =
{
	ai_run, 21, NULL,
	ai_run, 21, NULL,
	ai_run, 21, NULL,
	ai_run, 21, NULL,
	ai_run, 21, NULL,
	ai_run, 21, sentrybot_sound_move
};
mmove_t sentrybot_move_run = {FRAME_run1, FRAME_run6, sentrybot_frames_run, NULL};

void sentrybot_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &sentrybot_move_stand;
	else
		self->monsterinfo.currentmove = &sentrybot_move_run;
}

// Jumping behaviour and animations.

mframe_t sentrybot_frames_jump [] =
{
	ai_move,  0, NULL,
	ai_move,  0, NULL,
	ai_move,  0, NULL,
	ai_move,  0, NULL,
	ai_move,  0, NULL,
	ai_move,  0, NULL
};
mmove_t sentrybot_move_jump = {FRAME_jump1, FRAME_jump6, sentrybot_frames_jump, sentrybot_run};

void sentrybot_jump(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_jump, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &sentrybot_move_jump;
}

// Pain behaviour and animations.

mframe_t sentrybot_frames_pain1 [] =
{
	ai_move, -4, NULL,
	ai_move, 4,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL
};
mmove_t sentrybot_move_pain1 = {FRAME_pain201, FRAME_pain204, sentrybot_frames_pain1, sentrybot_run};

mframe_t sentrybot_frames_pain2 [] =
{
	ai_move, -1, NULL,
	ai_move, 1,  NULL,
	ai_move, 0,  NULL,
	ai_move, 1,  NULL
};
mmove_t sentrybot_move_pain2 = {FRAME_pain301, FRAME_pain304, sentrybot_frames_pain2, sentrybot_run};

void sentrybot_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	self->s.sound = 0;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0;
	
	if (skill->value > 2)
		return;

	if (damage <= 8)
		return;

	if (random() < 0.5)
		self->monsterinfo.currentmove = &sentrybot_move_pain1;
	else
		self->monsterinfo.currentmove = &sentrybot_move_pain2;
	
	gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
}

// Death behaviour.

void sentrybot_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

	self->s.sound = 0;
	gi.sound(self, CHAN_VOICE, sound_die1, 1, ATTN_NORM, 0);
	if (!(self->spawnflags & SF_MONSTER_NOGIB))
	{
		for (n = 0; n < 5; ++n)
			ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_METALLIC);
		ThrowGib(self, "models/objects/gibs/gear/tris.md2", damage, GIB_METALLIC);
		ThrowHead(self, "models/objects/gibs/gear/tris.md2", damage, GIB_METALLIC);
		self->deadflag = DEAD_DEAD;
	}
	T_RadiusDamage(self, self, 10, self, 100, MOD_UNKNOWN, -0.5);
	BecomeExplosion1(self);
}

// Combat behaviour and animations.

void sentrybot_HB(edict_t *self)
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

	if (!self->enemy || !self->enemy->inuse)
		return;

//	Determine muzzle effects.

	flash_number = (self->moreflags) ? MZ2_SENTRYBOT_GUN_1 : MZ2_SENTRYBOT_GUN_2;
	self->moreflags ^= 1;

	if (((self->count++) % 4) == 0)
		effect = EF_HYPERBLASTER;
	else
		effect = 0;

	if (self->count >= 3)
		self->count = 0;

//	Determine initial start and end vectors.

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);
	VectorCopy(self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight - 4;

//	Fog reduction of accuracy.

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

//	Fire!

	gi.sound(self, CHAN_WEAPON, sound_weapon, 1, ATTN_NORM, 0);
	fire_blaster(self, start, aim, 1, bolt_speed, effect, false, BLASTER_ORANGE);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_HEATBEAM_STEAM);
	gi.WritePosition(start);
	gi.WriteDir(forward);
	gi.multicast(start, MULTICAST_PVS);
}

void sentrybot_MG(edict_t *self)
{
	vec3_t	forward;
	vec3_t	right;
	vec3_t	start;
	vec3_t	end;
	vec3_t	aim;
	int		flash_number;

	if (!self->enemy || !self->enemy->inuse)
		return;

//	Determine muzzle effects.

	flash_number = (self->moreflags) ? MZ2_SENTRYBOT_GUN_1 : MZ2_SENTRYBOT_GUN_2;
	self->moreflags ^= 1;

//	Determine initial start and end vectors; lag the target slightly (hitscan weapon with spread).

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);
	VectorMA(self->enemy->s.origin, -0.2, self->enemy->velocity, end);
	end[2] += self->enemy->viewheight;

//	Fog reduction of accuracy.

	if (self->monsterinfo.visibility < FOG_CANSEEGOOD)
	{
		end[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		end[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		end[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
	}
	VectorSubtract(end, start, aim);
	VectorNormalize(aim);

//	Fire!

	gi.sound(self, CHAN_WEAPON, sound_weapon, 1, ATTN_NORM, 0);
	fire_bullet(self, start, aim, 1, 5, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_UNKNOWN);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_GUNSHOT);
	gi.WritePosition(start);
	gi.WriteDir(forward);
	gi.multicast(start, MULTICAST_PVS);
}

void sentrybot_fire(edict_t *self)
{
//	If flagged as 'special', fire hyperblasters; otherwise, fire machineguns.

	if (self->spawnflags & SF_MONSTER_SPECIAL)
		sentrybot_HB(self);
	else
		sentrybot_MG(self);

	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;

	if ((self->enemy) && !(visible(self, self->enemy) && infront(self, self->enemy)))
		self->monsterinfo.pausetime = level.time;
}

mframe_t sentrybot_frames_attack [] =
{
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  sentrybot_fire,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL
};
mmove_t sentrybot_move_attack = {FRAME_attack1, FRAME_attack8, sentrybot_frames_attack, sentrybot_run};

void sentrybot_attack(edict_t *self)
{
	int n;

	n = (rand() & 15) + 5 + (5 * skill->value);
	self->monsterinfo.pausetime = level.time + (n * FRAMETIME);
	self->monsterinfo.currentmove = &sentrybot_move_attack;
}

/*QUAKED monster_sentrybot (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight Special
*/
void SP_monster_sentrybot (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

//	Model information.

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("players/walker/tris.md2");
	self->s.modelindex2 = gi.modelindex("players/walker/weapon.md2");
	VectorSet(self->mins, -24, -24, -24);
	VectorSet(self->maxs,  24,  24,  30);
	self->s.skinnum = self->style;

//	Behaviour and animation functions.

	self->pain = sentrybot_pain;
	self->die = sentrybot_die;

	self->monsterinfo.stand = sentrybot_stand;
	self->monsterinfo.walk = sentrybot_walk;
	self->monsterinfo.run = sentrybot_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = sentrybot_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = sentrybot_sight;
	self->monsterinfo.idle = sentrybot_idle;
	
	if (monsterjump->value)
	{
		self->monsterinfo.jump = sentrybot_jump;
		self->monsterinfo.jumpup = 48;
		self->monsterinfo.jumpdn = 160;
	}
	
//	Pre-cache sounds.

	sound_pain1 = gi.soundindex("sentrybot/sbpain1.wav");
	sound_die1 = gi.soundindex("sentrybot/sbdeth1.wav");
	sound_sight = gi.soundindex("sentrybot/sbsght1.wav");
	sound_search = gi.soundindex("sentrybot/sbsrch1.wav");
	sound_idle = gi.soundindex("sentrybot/sbidle1.wav");
	sound_stand = gi.soundindex("sentrybot/sbstand1.wav");
	sound_move = gi.soundindex("sentrybot/sbmove1.wav");
	sound_jump = gi.soundindex("sentrybot/sbjump1.wav");
	if (self->spawnflags & SF_MONSTER_SPECIAL)
		sound_weapon = gi.soundindex("hover/hovatck1.wav");
	else
		sound_weapon = gi.soundindex("sentrybot/sbatck1.wav");


//	Mapper-configurable health

	if (self->health <= 0)
		self->health = 150;
	if (!self->gib_health)
		self->gib_health = -120;
	if (!self->mass)
		self->mass = 250;

//	Set blood_type to sparks by default. If the mapper wants blood, they need to set it to -1.

	if (!self->blood_type)
		self->blood_type = 2;

//	if (self->blood_type == -1)
//		self->blood_type = 0;
	// Knightmare- added sparks and blood type
	if (!self->blood_type)
		self->blood_type = 2; //sparks
	else
		self->fogclip |= 2; //custom bloodtype flag

//	Powerarmor or powerscreen use.

	if (self->powerarmor < 0)
	{
		self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
		self->monsterinfo.power_armor_power = -self->powerarmor;
	}
	else if (self->powerarmor > 0)
	{
		self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
		self->monsterinfo.power_armor_power = self->powerarmor;
	}

//	Miscellaneous setup information.

	if (!self->monsterinfo.flies)
		self->monsterinfo.flies = 0.0;

	gi.linkentity(self);
	self->monsterinfo.currentmove = &sentrybot_move_stand;

	self->common_name = "Sentrybot";
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start(self);
}

