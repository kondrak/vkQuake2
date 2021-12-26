/*
==============================================================================

BERSERK

==============================================================================
*/

#include "g_local.h"
#include "m_berserk.h"


static int sound_pain;
static int sound_die;
static int sound_idle;
static int sound_punch;
static int sound_sight;
static int sound_search;

void berserk_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void berserk_search (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}


void berserk_fidget (edict_t *self);
mframe_t berserk_frames_stand [] =
{
	ai_stand, 0, berserk_fidget,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL
};
mmove_t berserk_move_stand = {FRAME_stand1, FRAME_stand5, berserk_frames_stand, NULL};

void berserk_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &berserk_move_stand;
}

mframe_t berserk_frames_stand_fidget [] =
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
	ai_stand, 0, NULL
};
mmove_t berserk_move_stand_fidget = {FRAME_standb1, FRAME_standb20, berserk_frames_stand_fidget, berserk_stand};

void berserk_fidget (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return;
	if (random() > 0.15)
		return;

	self->monsterinfo.currentmove = &berserk_move_stand_fidget;

	if(!(self->spawnflags & SF_MONSTER_AMBUSH))
		gi.sound (self, CHAN_WEAPON, sound_idle, 1, ATTN_IDLE, 0);
}


mframe_t berserk_frames_walk [] =
{
	ai_walk, 9.1, NULL,
	ai_walk, 6.3, NULL,
	ai_walk, 4.9, NULL,
	ai_walk, 6.7, NULL,
	ai_walk, 6.0, NULL,
	ai_walk, 8.2, NULL,
	ai_walk, 7.2, NULL,
	ai_walk, 6.1, NULL,
	ai_walk, 4.9, NULL,
	ai_walk, 4.7, NULL,
	ai_walk, 4.7, NULL,
	ai_walk, 4.8, NULL
};
mmove_t berserk_move_walk = {FRAME_walkc1, FRAME_walkc11, berserk_frames_walk, NULL};

void berserk_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &berserk_move_walk;
}

/*

  *****************************
  SKIPPED THIS FOR NOW!
  *****************************

   Running -> Arm raised in air

void()	berserk_runb1	=[	$r_att1 ,	berserk_runb2	] {ai_run(21);};
void()	berserk_runb2	=[	$r_att2 ,	berserk_runb3	] {ai_run(11);};
void()	berserk_runb3	=[	$r_att3 ,	berserk_runb4	] {ai_run(21);};
void()	berserk_runb4	=[	$r_att4 ,	berserk_runb5	] {ai_run(25);};
void()	berserk_runb5	=[	$r_att5 ,	berserk_runb6	] {ai_run(18);};
void()	berserk_runb6	=[	$r_att6 ,	berserk_runb7	] {ai_run(19);};
// running with arm in air : start loop
void()	berserk_runb7	=[	$r_att7 ,	berserk_runb8	] {ai_run(21);};
void()	berserk_runb8	=[	$r_att8 ,	berserk_runb9	] {ai_run(11);};
void()	berserk_runb9	=[	$r_att9 ,	berserk_runb10	] {ai_run(21);};
void()	berserk_runb10	=[	$r_att10 ,	berserk_runb11	] {ai_run(25);};
void()	berserk_runb11	=[	$r_att11 ,	berserk_runb12	] {ai_run(18);};
void()	berserk_runb12	=[	$r_att12 ,	berserk_runb7	] {ai_run(19);};
// running with arm in air : end loop
*/


mframe_t berserk_frames_run1 [] =
{
	ai_run, 21, NULL,
	ai_run, 11, NULL,
	ai_run, 21, NULL,
	ai_run, 25, NULL,
	ai_run, 18, NULL,
	ai_run, 19, NULL
};
mmove_t berserk_move_run1 = {FRAME_run1, FRAME_run6, berserk_frames_run1, NULL};

void berserk_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &berserk_move_stand;
	else
		self->monsterinfo.currentmove = &berserk_move_run1;
}


void berserk_attack_spike (edict_t *self)
{
	static	vec3_t	aim = {MELEE_DISTANCE, 0, -24};
	fire_hit (self, aim, (20 + (rand() % 6)), 400);		//Faster attack (upwards and backwards)
														//CW: increased damage from 10
}


void berserk_swing (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_punch, 1, ATTN_NORM, 0);
}

mframe_t berserk_frames_attack_spike [] =
{
//		ai_charge, 0, NULL,		//CW: don't pause before attacking
//		ai_charge, 0, NULL,		//CW
		ai_charge, 0, berserk_swing,
		ai_charge, 0, berserk_attack_spike,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL,
		ai_charge, 0, NULL
};
mmove_t berserk_move_attack_spike = {FRAME_att_c3, FRAME_att_c8, berserk_frames_attack_spike, berserk_run};	//CW: initial frame was att_c1


void berserk_attack_club (edict_t *self)
{
	vec3_t	aim;

	VectorSet (aim, MELEE_DISTANCE, self->mins[0], -4);
	fire_hit (self, aim, (10 + (rand() % 6)), 400);		//CW: increased damage from 5
}

mframe_t berserk_frames_attack_club [] =
{	
//	ai_charge, 0, NULL,		//CW: don't pause before attacking
//	ai_charge, 0, NULL,		//CW
//	ai_charge, 0, NULL,		//CW
//	ai_charge, 0, NULL,		//CW
	ai_charge, 0, berserk_swing,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, berserk_attack_club,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t berserk_move_attack_club = {FRAME_att_c13, FRAME_att_c20, berserk_frames_attack_club, berserk_run};	//CW: initial frame was att_c9


void berserk_strike (edict_t *self)
{
	//FIXME play impact sound
}


mframe_t berserk_frames_attack_strike [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, berserk_swing,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, berserk_strike,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 9.7, NULL,
	ai_move, 13.6, NULL
};
	
mmove_t berserk_move_attack_strike = {FRAME_att_c21, FRAME_att_c34, berserk_frames_attack_strike, berserk_run};


void berserk_melee (edict_t *self)		//CW: wot no _attack_strike ??
{
	if ((rand() % 2) == 0)
		self->monsterinfo.currentmove = &berserk_move_attack_spike;
	else
		self->monsterinfo.currentmove = &berserk_move_attack_club;
}


/*
void() 	berserk_atke1	=[	$r_attb1,	berserk_atke2	] {ai_run(9);};
void() 	berserk_atke2	=[	$r_attb2,	berserk_atke3	] {ai_run(6);};
void() 	berserk_atke3	=[	$r_attb3,	berserk_atke4	] {ai_run(18.4);};
void() 	berserk_atke4	=[	$r_attb4,	berserk_atke5	] {ai_run(25);};
void() 	berserk_atke5	=[	$r_attb5,	berserk_atke6	] {ai_run(14);};
void() 	berserk_atke6	=[	$r_attb6,	berserk_atke7	] {ai_run(20);};
void() 	berserk_atke7	=[	$r_attb7,	berserk_atke8	] {ai_run(8.5);};
void() 	berserk_atke8	=[	$r_attb8,	berserk_atke9	] {ai_run(3);};
void() 	berserk_atke9	=[	$r_attb9,	berserk_atke10	] {ai_run(17.5);};
void() 	berserk_atke10	=[	$r_attb10,	berserk_atke11	] {ai_run(17);};
void() 	berserk_atke11	=[	$r_attb11,	berserk_atke12	] {ai_run(9);};
void() 	berserk_atke12	=[	$r_attb12,	berserk_atke13	] {ai_run(25);};
void() 	berserk_atke13	=[	$r_attb13,	berserk_atke14	] {ai_run(3.7);};
void() 	berserk_atke14	=[	$r_attb14,	berserk_atke15	] {ai_run(2.6);};
void() 	berserk_atke15	=[	$r_attb15,	berserk_atke16	] {ai_run(19);};
void() 	berserk_atke16	=[	$r_attb16,	berserk_atke17	] {ai_run(25);};
void() 	berserk_atke17	=[	$r_attb17,	berserk_atke18	] {ai_run(19.6);};
void() 	berserk_atke18	=[	$r_attb18,	berserk_run1	] {ai_run(7.8);};
*/


mframe_t berserk_frames_pain1 [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t berserk_move_pain1 = {FRAME_painc1, FRAME_painc4, berserk_frames_pain1, berserk_run};


mframe_t berserk_frames_pain2 [] =
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
	ai_move, 0, NULL
};
mmove_t berserk_move_pain2 = {FRAME_painb1, FRAME_painb20, berserk_frames_pain2, berserk_run};

void berserk_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;
	gi.sound (self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	if (skill->value > 1)  
		return;		// no pain anims in nightmare (CW: or hard)

	if (damage <= 10)	//CW: shrug off low damage
		return;

	if ((damage < 20) || (random() < 0.5))
		self->monsterinfo.currentmove = &berserk_move_pain1;
	else
		self->monsterinfo.currentmove = &berserk_move_pain2;
}


void berserk_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
	M_FlyCheck (self);

	// Lazarus monster fade
	if(world->effects & FX_WORLDSPAWN_CORPSEFADE)
	{
		self->think=FadeDieSink;
		self->nextthink=level.time+corpse_fadetime->value;
	}
}


mframe_t berserk_frames_death1 [] =
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
	ai_move, 0, NULL
	
};
mmove_t berserk_move_death1 = {FRAME_death1, FRAME_death13, berserk_frames_death1, berserk_dead};


mframe_t berserk_frames_death2 [] =
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
mmove_t berserk_move_death2 = {FRAME_deathc1, FRAME_deathc8, berserk_frames_death2, berserk_dead};


void berserk_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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

	gi.sound (self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	if (damage >= 50)
		self->monsterinfo.currentmove = &berserk_move_death1;
	else
		self->monsterinfo.currentmove = &berserk_move_death2;
}

//===========
//Jump
mframe_t berserk_frames_jump [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t berserk_move_jump = { FRAME_run1, FRAME_run6, berserk_frames_jump, berserk_run };

void berserk_jump (edict_t *self)
{
	self->monsterinfo.currentmove = &berserk_move_jump;
}


/*QUAKED monster_berserk (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_berserk (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	// pre-caches
	sound_pain  = gi.soundindex ("berserk/berpain2.wav");
	sound_die   = gi.soundindex ("berserk/berdeth2.wav");
	sound_idle  = gi.soundindex ("berserk/beridle1.wav");
	sound_punch = gi.soundindex ("berserk/attack.wav");
	sound_search = gi.soundindex ("berserk/bersrch1.wav");
	sound_sight = gi.soundindex ("berserk/sight.wav");

	// Lazarus: special purpose skins
	if ( self->style )
	{
		PatchMonsterModel("models/monsters/berserk/tris.md2");
		self->s.skinnum = self->style * 2;
	}

	self->s.modelindex = gi.modelindex("models/monsters/berserk/tris.md2");
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 32);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	// Lazarus: mapper-configurable health
	if(!self->health)
		self->health = 240;
	if(!self->gib_health)
		self->gib_health = -150;  //CW: was -60
	if(!self->mass)
		self->mass = 250;

	self->pain = berserk_pain;
	self->die = berserk_die;

	self->monsterinfo.stand = berserk_stand;
	self->monsterinfo.walk = berserk_walk;
	self->monsterinfo.run = berserk_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = NULL;
	self->monsterinfo.melee = berserk_melee;
	self->monsterinfo.sight = berserk_sight;
	self->monsterinfo.search = berserk_search;
	if(monsterjump->value) 
	{
		self->monsterinfo.jump = berserk_jump;
		self->monsterinfo.jumpup = 72;		//CW: was 48
		self->monsterinfo.jumpdn = 250;		//CW: was 160
	}

	self->monsterinfo.currentmove = &berserk_move_stand;
	if(self->health < 0)
	{
		mmove_t	*deathmoves[] = {&berserk_move_death1,
			                     &berserk_move_death2,
								 NULL};
		M_SetDeath(self,(mmove_t **)&deathmoves);
	}
	self->monsterinfo.scale = MODEL_SCALE;

	// Knightmare- added sparks and blood type
	if (!self->blood_type)
		self->blood_type = 3; //sparks and blood

//CW+++	Use negative powerarmor values to give the monster a Power Screen.
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
	//	self->monsterinfo.flies = 0.10;	//CW: was 0.20

	self->common_name = "Berserker";

	gi.linkentity (self);
	walkmonster_start (self);
}
