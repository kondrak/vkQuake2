/*
==============================================================================

boss2

==============================================================================
*/

#include "g_local.h"
#include "m_boss2.h"

void BossExplode (edict_t *self);

qboolean infront (edict_t *self, edict_t *other);

static int	sound_pain1;
static int	sound_pain2;
static int	sound_pain3;
static int	sound_death;
static int	sound_search1;

void boss2_search (edict_t *self)
{
	if (random() < 0.5)
		gi.sound (self, CHAN_VOICE, sound_search1, 1, ATTN_NONE, 0);
}

void boss2_run (edict_t *self);
void boss2_stand (edict_t *self);
void boss2_dead (edict_t *self);
void boss2_attack (edict_t *self);
void boss2_attack_mg (edict_t *self);
void boss2_reattack_mg (edict_t *self);
void boss2_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

void Boss2Rocket (edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;
	int		rocketSpeed;

//CW++	Fix potential crashes
	if (!self->enemy)
		return;
//CW--

	if((self->spawnflags & SF_MONSTER_SPECIAL))
		rocketSpeed = 400; // Lazarus: Homing rockets are tougher when slower
	else
		rocketSpeed = 500 + (100 * skill->value);	// PGM rock & roll.... :)

	AngleVectors (self->s.angles, forward, right, NULL);

	VectorCopy (self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;

	// Lazarus fog reduction of accuracy
	if(self->monsterinfo.visibility < FOG_CANSEEGOOD)
	{
		vec[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		vec[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		vec[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
	}

//1
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_1], forward, right, start);
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	monster_fire_rocket (self, start, dir, 50, rocketSpeed, MZ2_BOSS2_ROCKET_1,
		(self->spawnflags & SF_MONSTER_SPECIAL ? self->enemy : NULL) );

//2
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_2], forward, right, start);
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	monster_fire_rocket (self, start, dir, 50, rocketSpeed, MZ2_BOSS2_ROCKET_2,
		(self->spawnflags & SF_MONSTER_SPECIAL ? self->enemy : NULL) );

//3
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_3], forward, right, start);
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	monster_fire_rocket (self, start, dir, 50, rocketSpeed, MZ2_BOSS2_ROCKET_3,
		(self->spawnflags & SF_MONSTER_SPECIAL ? self->enemy : NULL) );

//4
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_4], forward, right, start);
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	monster_fire_rocket (self, start, dir, 50, rocketSpeed, MZ2_BOSS2_ROCKET_4,
		(self->spawnflags & SF_MONSTER_SPECIAL ? self->enemy : NULL) );
}	

void boss2_firebullet_right (edict_t *self)
{
	vec3_t	forward, right, target;
	vec3_t	start;

//CW++	Fix potential crashes
	if (!self->enemy)
		return;
//CW--

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_MACHINEGUN_R1], forward, right, start);
	VectorMA (self->enemy->s.origin, -0.05*(3-skill->value), self->enemy->velocity, target);	//CW vary projection scale factor based on skill
	target[2] += self->enemy->viewheight;

	// Lazarus fog reduction of accuracy
	if(self->monsterinfo.visibility < FOG_CANSEEGOOD)
	{
		target[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		target[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		target[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
	}

	VectorSubtract (target, start, forward);
	VectorNormalize (forward);
	monster_fire_bullet (self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_BOSS2_MACHINEGUN_R1);
}	

void boss2_firebullet_left (edict_t *self)
{
	vec3_t	forward, right, target;
	vec3_t	start;
	
//CW++	Fix potential crashes
	if (!self->enemy)
		return;
//CW--

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_MACHINEGUN_L1], forward, right, start);
	VectorMA (self->enemy->s.origin, -0.05*(3-skill->value), self->enemy->velocity, target);	//CW vary projection scale factor based on skill
	target[2] += self->enemy->viewheight;

	// Lazarus fog reduction of accuracy
	if(self->monsterinfo.visibility < FOG_CANSEEGOOD)
	{
		target[0] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		target[1] += crandom() * 640 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
		target[2] += crandom() * 320 * (FOG_CANSEEGOOD - self->monsterinfo.visibility);
	}

	VectorSubtract (target, start, forward);
	VectorNormalize (forward);

	monster_fire_bullet (self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_BOSS2_MACHINEGUN_L1);
}	

void Boss2MachineGun (edict_t *self)
{
	boss2_firebullet_left(self);
	boss2_firebullet_right(self);
}	


mframe_t boss2_frames_stand [] =
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
	ai_stand, 0, NULL
};
mmove_t	boss2_move_stand = {FRAME_stand30, FRAME_stand50, boss2_frames_stand, NULL};

mframe_t boss2_frames_fidget [] =
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
mmove_t boss2_move_fidget = {FRAME_stand1, FRAME_stand30, boss2_frames_fidget, NULL};

mframe_t boss2_frames_walk [] =
{
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL,
	ai_walk,	8,	NULL
};
mmove_t boss2_move_walk = {FRAME_walk1, FRAME_walk20, boss2_frames_walk, NULL};


mframe_t boss2_frames_run [] =
{
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	8,	NULL
};
mmove_t boss2_move_run = {FRAME_walk1, FRAME_walk20, boss2_frames_run, NULL};

mframe_t boss2_frames_attack_pre_mg [] =
{
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	boss2_attack_mg
};
mmove_t boss2_move_attack_pre_mg = {FRAME_attack1, FRAME_attack9, boss2_frames_attack_pre_mg, NULL};


// Loop this
mframe_t boss2_frames_attack_mg [] =
{
	ai_charge,	1,	Boss2MachineGun,
	ai_charge,	1,	Boss2MachineGun,
	ai_charge,	1,	Boss2MachineGun,
	ai_charge,	1,	Boss2MachineGun,
	ai_charge,	1,	Boss2MachineGun,
	ai_charge,	1,	boss2_reattack_mg
};
mmove_t boss2_move_attack_mg = {FRAME_attack10, FRAME_attack15, boss2_frames_attack_mg, NULL};

mframe_t boss2_frames_attack_post_mg [] =
{
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL
};
mmove_t boss2_move_attack_post_mg = {FRAME_attack16, FRAME_attack19, boss2_frames_attack_post_mg, boss2_run};

mframe_t boss2_frames_attack_rocket [] =
{
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_move,	-20,	Boss2Rocket,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL
};
mmove_t boss2_move_attack_rocket = {FRAME_attack20, FRAME_attack40, boss2_frames_attack_rocket, boss2_run};

mframe_t boss2_frames_pain_heavy [] =
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
	ai_move,	0,	NULL
};
mmove_t boss2_move_pain_heavy = {FRAME_pain2, FRAME_pain19, boss2_frames_pain_heavy, boss2_run};

mframe_t boss2_frames_pain_light [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t boss2_move_pain_light = {FRAME_pain20, FRAME_pain23, boss2_frames_pain_light, boss2_run};

mframe_t boss2_frames_death [] =
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
	ai_move,	0,	BossExplode
};
mmove_t boss2_move_death = {FRAME_death2, FRAME_death50, boss2_frames_death, boss2_dead};

void boss2_stand (edict_t *self)
{
		self->monsterinfo.currentmove = &boss2_move_stand;
}

void boss2_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &boss2_move_stand;
	else
		self->monsterinfo.currentmove = &boss2_move_run;
}

void boss2_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &boss2_move_walk;
}

void boss2_attack (edict_t *self)
{
	vec3_t	vec;
	float	range;

//CW++	Fix potential crashes
	if (!self->enemy)
		return;
//CW--

	VectorSubtract (self->enemy->s.origin, self->s.origin, vec);
	range = VectorLength (vec);
	
	if (range <= 128)
	{
		self->monsterinfo.currentmove = &boss2_move_attack_pre_mg;
	}
	else 
	{
		if (random() <= ((self->spawnflags & SF_MONSTER_SPECIAL) ? (0.5+0.1*skill->value) : 0.4)) //CW: was <= 0.6 for mg
			self->monsterinfo.currentmove = &boss2_move_attack_rocket;
		else
			self->monsterinfo.currentmove = &boss2_move_attack_pre_mg;
	}
}

void boss2_attack_mg (edict_t *self)
{
	self->monsterinfo.currentmove = &boss2_move_attack_mg;
}

void boss2_reattack_mg (edict_t *self)
{
//CW++	Fix potential crashes
	if (!self->enemy)
		return;
//CW--

	if ((infront(self, self->enemy)) && (random() <= (0.6 + 0.1*skill->value)))	//CW: was <= 0.7
		self->monsterinfo.currentmove = &boss2_move_attack_mg;
	else
		self->monsterinfo.currentmove = &boss2_move_attack_post_mg;
}

void boss2_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health * 0.5))
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

	if (damage <= 30)	//CW: shrug off low damage
		return;

// American wanted these at no attenuation
	if (damage < 45)		//CW: was 10
	{
		gi.sound (self, CHAN_VOICE, sound_pain3, 1, ATTN_NONE, 0);
		self->monsterinfo.currentmove = &boss2_move_pain_light;
	}
	else if (damage < 60)	//CW: was 30
	{
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NONE, 0);
		self->monsterinfo.currentmove = &boss2_move_pain_light;
	}
	else 
	{
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NONE, 0);
		self->monsterinfo.currentmove = &boss2_move_pain_heavy;
	}
}

void boss2_dead (edict_t *self)
{
	VectorSet (self->mins, -56, -56, 0);
	VectorSet (self->maxs, 56, 56, 80);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
}

void boss2_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->s.skinnum |= 1;
	if (!(self->fogclip & 2)) //custom bloodtype flag check
		self->blood_type = 3; //sparks and blood

	self->monsterinfo.power_armor_type = POWER_ARMOR_NONE;
	gi.sound (self, CHAN_VOICE, sound_death, 1, ATTN_NONE, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_NO;
	self->count = 0;
	self->monsterinfo.currentmove = &boss2_move_death;
#if 0
	int		n;

	self->s.sound = 0;
	// check for gib
	if (self->health <= self->gib_health && !(self->spawnflags & MONSTER_NOGIB))
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

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &boss2_move_death;
#endif
}

qboolean Boss2_CheckAttack (edict_t *self)
{
	vec3_t	spot1, spot2;
	vec3_t	temp;
	float	chance;
	trace_t	tr;
	qboolean	enemy_infront;
	int			enemy_range;
	float		enemy_yaw;

	if (!self->enemy)		//CW: paranoia check
		return false;

	if (self->enemy->health > 0)
	{
	// see if any entities are in the way of the shot
		VectorCopy (self->s.origin, spot1);
		spot1[2] += self->viewheight;
		VectorCopy (self->enemy->s.origin, spot2);
		spot2[2] += self->enemy->viewheight;

		tr = gi.trace (spot1, NULL, NULL, spot2, self, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_SLIME|CONTENTS_LAVA);

		// do we have a clear shot?
		if (tr.ent != self->enemy)
			return false;
	}
	
	enemy_infront = infront(self, self->enemy);
	enemy_range = range(self, self->enemy);
	VectorSubtract (self->enemy->s.origin, self->s.origin, temp);
	enemy_yaw = vectoyaw(temp);

	self->ideal_yaw = enemy_yaw;


	// melee attack
	if (enemy_range == RANGE_MELEE)
	{
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
		chance = 0.8;
	}
	else if (enemy_range == RANGE_NEAR)
	{
		chance = 0.8;
	}
	else if (enemy_range == RANGE_MID)
	{
		chance = 0.8;
	}
	else
	{
		return false;
	}

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



/*QUAKED monster_boss2 (1 .5 0) (-56 -56 0) (56 56 80) Ambush Trigger_Spawn Sight
*/
void SP_monster_boss2 (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	// Lazarus: special purpose skins
	if ( (self->spawnflags & SF_MONSTER_SPECIAL) && self->style )
	{
		PatchMonsterModel("models/monsters/boss2/tris.md2");
		self->s.skinnum = self->style * 2;
	}

	sound_pain1 = gi.soundindex ("bosshovr/bhvpain1.wav");
	sound_pain2 = gi.soundindex ("bosshovr/bhvpain2.wav");
	sound_pain3 = gi.soundindex ("bosshovr/bhvpain3.wav");
	sound_death = gi.soundindex ("bosshovr/bhvdeth1.wav");
	sound_search1 = gi.soundindex ("bosshovr/bhvunqv1.wav");

	self->s.sound = gi.soundindex ("bosshovr/bhvengn1.wav");
#ifdef LOOP_SOUND_ATTENUATION
	self->s.attenuation = ATTN_IDLE;
#endif

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	// Lazarus: special purpose skins
	if ( self->style )
	{
		PatchMonsterModel("models/monsters/boss2/tris.md2");
		self->s.skinnum = self->style * 2;
	}
	
	self->s.modelindex = gi.modelindex ("models/monsters/boss2/tris.md2");
	VectorSet (self->mins, -56, -56, 0);
	VectorSet (self->maxs, 56, 56, 80);

	// Lazarus: mapper-configurable health
	if(!self->health)
		self->health = 2000;
	if(!self->gib_health)
		self->gib_health = -200;
	if(!self->mass)
		self->mass = 1000;

	self->flags |= FL_IMMUNE_LASER;

	self->pain = boss2_pain;
	self->die = boss2_die;

	self->monsterinfo.stand = boss2_stand;
	self->monsterinfo.walk = boss2_walk;
	self->monsterinfo.run = boss2_run;
	self->monsterinfo.attack = boss2_attack;
	self->monsterinfo.search = boss2_search;
	self->monsterinfo.checkattack = Boss2_CheckAttack;

	// Knightmare- added sparks and blood type
	if (!self->blood_type)
		self->blood_type = 2; //sparks
	else
		self->fogclip |= 2; //custom bloodtype flag

	gi.linkentity (self);

	self->monsterinfo.currentmove = &boss2_move_stand;	
	if(self->health < 0)
	{
		mmove_t	*deathmoves[] = {&boss2_move_death,
								 NULL};
		M_SetDeath(self,(mmove_t **)&deathmoves);
	}
	self->monsterinfo.scale = MODEL_SCALE;

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

	self->common_name = "Flying Boss";

	flymonster_start (self);
}
