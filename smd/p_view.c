#include "g_local.h"
#include "m_player.h"

static	edict_t		*current_player;
static	gclient_t	*current_client;

static	vec3_t	forward, right, up;
float	xyspeed;

float	bobmove;
int		bobcycle;		// odd cycles are right foot going forward
float	bobfracsin;		// sin(bobfrac*M_PI)

/*
===============
SV_CalcRoll

===============
*/
float SV_CalcRoll (vec3_t angles, vec3_t velocity)
{
	float	sign;
	float	side;
	float	value;
	
	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabs(side);
	
	value = sv_rollangle->value;

	if (side < sv_rollspeed->value)
		side = side * value / sv_rollspeed->value;
	else
		side = value;
	
	return side*sign;
	
}


/*
===============
P_DamageFeedback

Handles color blends and view kicks
===============
*/
void P_DamageFeedback (edict_t *player)
{
	gclient_t	*client;
	float	side;
	float	realcount, count, kick;
	vec3_t	v;
	int		r, l;
	static	vec3_t	power_color = {0.0, 1.0, 0.0};
	static	vec3_t	acolor = {1.0, 1.0, 1.0};
	static	vec3_t	bcolor = {1.0, 0.0, 0.0};

	client = player->client;

	// flash the backgrounds behind the status numbers
	client->ps.stats[STAT_FLASHES] = 0;
	if (client->damage_blood)
		client->ps.stats[STAT_FLASHES] |= 1;
	if (client->damage_armor && !(player->flags & FL_GODMODE) && (client->invincible_framenum <= level.framenum))
		client->ps.stats[STAT_FLASHES] |= 2;

	// total points of damage shot at the player this frame
	count = (client->damage_blood + client->damage_armor + client->damage_parmor);
	if (count == 0)
		return;		// didn't take any damage

	// start a pain animation if still in the player model
	if (client->anim_priority < ANIM_PAIN && player->s.modelindex == (MAX_MODELS-1)) //was 255
	{
		static int		i;

		client->anim_priority = ANIM_PAIN;
		if (client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			player->s.frame = FRAME_crpain1-1;
			client->anim_end = FRAME_crpain4;
		}
		else
		{
			i = (i+1)%3;
			switch (i)
			{
			case 0:
				player->s.frame = FRAME_pain101-1;
				client->anim_end = FRAME_pain104;
				break;
			case 1:
				player->s.frame = FRAME_pain201-1;
				client->anim_end = FRAME_pain204;
				break;
			case 2:
				player->s.frame = FRAME_pain301-1;
				client->anim_end = FRAME_pain304;
				break;
			}
		}
	}

	realcount = count;
	if (count < 10)
		count = 10;	// always make a visible effect

	// play an apropriate pain sound
	if ((level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) && (client->invincible_framenum <= level.framenum))
	{
		r = 1 + (rand()&1);
		player->pain_debounce_time = level.time + 0.7;
		if (player->health < 25)
			l = 25;
		else if (player->health < 50)
			l = 50;
		else if (player->health < 75)
			l = 75;
		else
			l = 100;
		gi.sound (player, CHAN_VOICE, gi.soundindex(va("*pain%i_%i.wav", l, r)), 1, ATTN_NORM, 0);
	}

	// the total alpha of the blend is always proportional to count
	if (client->damage_alpha < 0)
		client->damage_alpha = 0;
	client->damage_alpha += count*0.01;
	if (client->damage_alpha < 0.2)
		client->damage_alpha = 0.2;
	if (client->damage_alpha > 0.6)
		client->damage_alpha = 0.6;		// don't go too saturated

	// the color of the blend will vary based on how much was absorbed
	// by different armors
	VectorClear (v);
	if (client->damage_parmor)
		VectorMA (v, (float)client->damage_parmor/realcount, power_color, v);
	if (client->damage_armor)
		VectorMA (v, (float)client->damage_armor/realcount,  acolor, v);
	if (client->damage_blood)
		VectorMA (v, (float)client->damage_blood/realcount,  bcolor, v);
	VectorCopy (v, client->damage_blend);


	//
	// calculate view angle kicks
	//
	kick = abs(client->damage_knockback);
	if (kick && player->health > 0)	// kick of 0 means no view adjust at all
	{
		kick = kick * 100 / player->health;

		if (kick < count*0.5)
			kick = count*0.5;
		if (kick > 50)
			kick = 50;

		VectorSubtract (client->damage_from, player->s.origin, v);
		VectorNormalize (v);
		
		side = DotProduct (v, right);
		client->v_dmg_roll = kick*side*0.3;
		
		side = -DotProduct (v, forward);
		client->v_dmg_pitch = kick*side*0.3;

		client->v_dmg_time = level.time + DAMAGE_TIME;
	}

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_parmor = 0;
	client->damage_knockback = 0;
}




/*
===============
SV_CalcViewOffset

Auto pitching on slopes?

  fall from 128: 400 = 160000
  fall from 256: 580 = 336400
  fall from 384: 720 = 518400
  fall from 512: 800 = 640000
  fall from 640: 960 = 

  damage = deltavelocity*deltavelocity  * 0.0001

===============
*/
void SV_CalcViewOffset (edict_t *ent)
{
	float		*angles;
	float		bob;
	float		ratio;
	float		delta;
	vec3_t		v;


//===================================

	// base angles
	angles = ent->client->ps.kick_angles;

	// if dead, fix the angle and don't add any kick
	if (ent->deadflag)
	{
		if(ent->deadflag != DEAD_FROZEN)
		{
			VectorClear (angles);

			ent->client->ps.viewangles[ROLL] = 40;
			ent->client->ps.viewangles[PITCH] = -15;
			ent->client->ps.viewangles[YAW] = ent->client->killer_yaw;
		}
	}
	else
	{
		// add angles based on weapon kick

		VectorCopy (ent->client->kick_angles, angles);

		// add angles based on damage kick

		ratio = (ent->client->v_dmg_time - level.time) / DAMAGE_TIME;
		if (ratio < 0)
		{
			ratio = 0;
			ent->client->v_dmg_pitch = 0;
			ent->client->v_dmg_roll = 0;
		}
		angles[PITCH] += ratio * ent->client->v_dmg_pitch;
		angles[ROLL] += ratio * ent->client->v_dmg_roll;

		// add pitch based on fall kick

		ratio = (ent->client->fall_time - level.time) / FALL_TIME;
		if (ratio < 0)
			ratio = 0;
		angles[PITCH] += ratio * ent->client->fall_value;

		// add angles based on velocity

		delta = DotProduct (ent->velocity, forward);
		angles[PITCH] += delta*run_pitch->value;
		
		delta = DotProduct (ent->velocity, right);
		angles[ROLL] += delta*run_roll->value;

		// add angles based on bob
	
		delta = bobfracsin * bob_pitch->value * xyspeed;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			delta *= 6;		// crouching
		angles[PITCH] += delta;
		delta = bobfracsin * bob_roll->value * xyspeed;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			delta *= 6;		// crouching
		if (bobcycle & 1)
			delta = -delta;
		angles[ROLL] += delta;
	}

//===================================

	// base origin

	VectorClear (v);

	// add view height

	v[2] += ent->viewheight;

	// add fall height

	ratio = (ent->client->fall_time - level.time) / FALL_TIME;
	if (ratio < 0)
		ratio = 0;
	v[2] -= ratio * ent->client->fall_value * 0.4;

	// add bob height

	bob = bobfracsin * xyspeed * bob_up->value;
	if (bob > 6)
		bob = 6;
//	gi.DebugGraph (bob *2, 255);
	v[2] += bob;

	// add kick offset

	VectorAdd (v, ent->client->kick_origin, v);

	// absolutely bound offsets
	// so the view can never be outside the player box

	if(ent->client->chasetoggle) {
		VectorSet (v, 0, 0, 0);
		if(ent->client->chasecam != NULL) {
			ent->client->ps.pmove.origin[0] = ent->client->chasecam->s.origin[0]*8;
			ent->client->ps.pmove.origin[1] = ent->client->chasecam->s.origin[1]*8;
			ent->client->ps.pmove.origin[2] = ent->client->chasecam->s.origin[2]*8;
		}
	} else if(ent->client->spycam) {
		VectorSet (v, 0, 0, 0);
        VectorCopy (ent->client->spycam->s.angles, ent->client->ps.viewangles); 
		if(ent->client->spycam->svflags & SVF_MONSTER)
			ent->client->ps.viewangles[PITCH] = ent->client->spycam->move_angles[PITCH];
	} else {
		if (v[0] < -14)
			v[0] = -14;
		else if (v[0] > 14)
			v[0] = 14;
		if (v[1] < -14)
			v[1] = -14;
		else if (v[1] > 14)
			v[1] = 14;
		if (v[2] < -22)
			v[2] = -22;
		else if (v[2] > 30)
			v[2] = 30;
	}

	VectorCopy (v, ent->client->ps.viewoffset);
}

/*
==============
SV_CalcGunOffset
==============
*/
void SV_CalcGunOffset (edict_t *ent)
{
	int		i;
	float	delta;

	// gun angles from bobbing
	ent->client->ps.gunangles[ROLL] = xyspeed * bobfracsin * 0.005;
	ent->client->ps.gunangles[YAW] = xyspeed * bobfracsin * 0.01;
	if (bobcycle & 1)
	{
		ent->client->ps.gunangles[ROLL] = -ent->client->ps.gunangles[ROLL];
		ent->client->ps.gunangles[YAW] = -ent->client->ps.gunangles[YAW];
	}

	ent->client->ps.gunangles[PITCH] = xyspeed * bobfracsin * 0.005;

	// gun angles from delta movement
	for (i=0 ; i<3 ; i++)
	{
		delta = ent->client->oldviewangles[i] - ent->client->ps.viewangles[i];
		if (delta > 180)
			delta -= 360;
		if (delta < -180)
			delta += 360;
		if (delta > 45)
			delta = 45;
		if (delta < -45)
			delta = -45;
		if (i == YAW)
			ent->client->ps.gunangles[ROLL] += 0.1*delta;
		ent->client->ps.gunangles[i] += 0.2 * delta;
	}

	// gun height
	VectorClear (ent->client->ps.gunoffset);
//	ent->ps->gunorigin[2] += bob;

	// gun_x / gun_y / gun_z are development tools
	for (i=0 ; i<3 ; i++)
	{
		ent->client->ps.gunoffset[i] += forward[i]*(gun_y->value);
		ent->client->ps.gunoffset[i] += right[i]*gun_x->value;
		ent->client->ps.gunoffset[i] += up[i]* (-gun_z->value);
	}
}


/*
=============
SV_AddBlend
=============
*/
void SV_AddBlend (float r, float g, float b, float a, float *v_blend)
{
	float	a2, a3;

	if (a <= 0)
		return;
	a2 = v_blend[3] + (1-v_blend[3])*a;	// new total alpha
	a3 = v_blend[3]/a2;		// fraction of color from old

	v_blend[0] = v_blend[0]*a3 + r*(1-a3);
	v_blend[1] = v_blend[1]*a3 + g*(1-a3);
	v_blend[2] = v_blend[2]*a3 + b*(1-a3);
	v_blend[3] = a2;
}


/*
=============
SV_CalcBlend
=============
*/
void SV_CalcBlend (edict_t *ent)
{
	int		contents;
	vec3_t	vieworg;
	int		remaining;

	ent->client->ps.blend[0] = ent->client->ps.blend[1] = 
		ent->client->ps.blend[2] = ent->client->ps.blend[3] = 0;

	// add for contents
	if (ent->client->chasetoggle)
		VectorCopy (ent->client->chasecam->s.origin, vieworg);
	else
		VectorAdd (ent->s.origin, ent->client->ps.viewoffset, vieworg);

	contents = gi.pointcontents (vieworg);
	if (contents & (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER) )
		ent->client->ps.rdflags |= RDF_UNDERWATER;
	else
		ent->client->ps.rdflags &= ~RDF_UNDERWATER;

		if (contents & CONTENTS_LAVA) //(CONTENTS_SOLID|CONTENTS_LAVA))
		SV_AddBlend (1.0, 0.3, 0.0, 0.6, ent->client->ps.blend);
	else if (contents & CONTENTS_SLIME)
		SV_AddBlend (0.0, 0.1, 0.05, 0.6, ent->client->ps.blend);
	else if (contents & CONTENTS_WATER)
	{
		if(ent->in_mud == 3)
			SV_AddBlend (0.4, 0.3, 0.2, 0.9, ent->client->ps.blend);
		else
			SV_AddBlend (0.5, 0.3, 0.2, 0.4, ent->client->ps.blend);
	}

	// add for powerups

#ifdef JETPACK_MOD
	if ( ent->client->jetpack )
	{
		remaining = ent->client->pers.inventory[fuel_index];
		// beginning to fade if 4 secs or less
		if (remaining > 40)
		{
			if ( ((level.framenum % 6) == 0) && ( level.framenum - ent->client->jetpack_activation > 30 ) )
			{
				if (ent->client->jetpack_thrusting && (level.framenum - ent->client->jetpack_start_thrust > 10))
					gi.sound (ent, CHAN_AUTO, gi.soundindex("jetpack/revrun.wav"), 1, ATTN_NORM, 0);
				gi.sound (ent, CHAN_GIZMO, gi.soundindex("jetpack/running.wav"), 1, ATTN_NORM, 0);
			}
		}
	}
#endif // #ifdef JETPACK_MOD

	if (ent->client->quad_framenum > level.framenum)
	{
		remaining = ent->client->quad_framenum - level.framenum;
		if (remaining == 30)	// beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage2.wav"), 1, ATTN_NORM, 0);
		if (remaining > 30 || (remaining & 4) )
			SV_AddBlend (0, 0, 1, 0.08, ent->client->ps.blend);
	}
	else if (ent->client->invincible_framenum > level.framenum)
	{
		remaining = ent->client->invincible_framenum - level.framenum;
		if (remaining == 30)	// beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect2.wav"), 1, ATTN_NORM, 0);
		if (remaining > 30 || (remaining & 4) )
			SV_AddBlend (1, 1, 0, 0.08, ent->client->ps.blend);
	}
	else if (ent->client->enviro_framenum > level.framenum)
	{
		remaining = ent->client->enviro_framenum - level.framenum;
		if (remaining == 30)	// beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/airout.wav"), 1, ATTN_NORM, 0);
		if (remaining > 30 || (remaining & 4) )
			SV_AddBlend (0, 1, 0, 0.08, ent->client->ps.blend);
	}
	else if (ent->client->breather_framenum > level.framenum)
	{
		remaining = ent->client->breather_framenum - level.framenum;
		if (remaining == 30)	// beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/airout.wav"), 1, ATTN_NORM, 0);
		if (remaining > 30 || (remaining & 4) )
			SV_AddBlend (0.4, 1, 0.4, 0.04, ent->client->ps.blend);
	}

	if (level.freeze && (level.freezeframes % 30 == 0))
	{
		if (level.freezeframes == (sk_stasis_time->value*10 - 30)) // was 270
			gi.sound(ent,CHAN_ITEM,gi.soundindex("items/stasis_stop.wav"), 1, ATTN_NORM, 0);
		else
			gi.sound(ent,CHAN_ITEM,gi.soundindex("items/stasis.wav"), 1, ATTN_NORM, 0);
	}

	// add for damage
	if (ent->client->damage_alpha > 0)
		SV_AddBlend (ent->client->damage_blend[0],ent->client->damage_blend[1]
		,ent->client->damage_blend[2], ent->client->damage_alpha, ent->client->ps.blend);

	if (ent->client->bonus_alpha > 0)
		SV_AddBlend (0.85, 0.7, 0.3, ent->client->bonus_alpha, ent->client->ps.blend);

	// drop the damage value
	ent->client->damage_alpha -= 0.06;
	if (ent->client->damage_alpha < 0)
		ent->client->damage_alpha = 0;

	// drop the bonus value
	ent->client->bonus_alpha -= 0.1;
	if (ent->client->bonus_alpha < 0)
		ent->client->bonus_alpha = 0;

	// DWH: Screen fade from target_failure or target_fade
	if (ent->client->fadein > 0)
	{
		float alpha;

		// Turn off fade for dead software players or they won't see menu
		if((ent->health <= 0) && (strcmp(vid_ref->string,"gl")))
			ent->client->fadein = 0;

		if(ent->client->fadein > level.framenum)
		{
			alpha = ent->client->fadealpha*(1.0 - (ent->client->fadein-level.framenum)/(ent->client->fadein-ent->client->fadestart));
			SV_AddBlend (ent->client->fadecolor[0],
				         ent->client->fadecolor[1],
						 ent->client->fadecolor[2],
						 alpha, ent->client->ps.blend);
		}
		else if(ent->client->fadehold > level.framenum)
		{
			SV_AddBlend (ent->client->fadecolor[0],
			             ent->client->fadecolor[1],
						 ent->client->fadecolor[2],
						 ent->client->fadealpha, ent->client->ps.blend);
		}
		else if(ent->client->fadeout > level.framenum)
		{
			alpha = ent->client->fadealpha*((ent->client->fadeout-level.framenum)/(ent->client->fadeout-ent->client->fadehold));
			SV_AddBlend (ent->client->fadecolor[0],
				         ent->client->fadecolor[1],
						 ent->client->fadecolor[2],
						 alpha, ent->client->ps.blend);
		}
		else
			ent->client->fadein = 0;
	}
}

/*
=================
P_SlamDamage
Serves same purpose as P_FallingDamage, but detects wall impacts
=================
*/
void P_SlamDamage (edict_t *ent)
{
	float	delta;
	int		damage;
	vec3_t	dir;
	vec3_t	deltav;

	if (ent->s.modelindex != (MAX_MODELS-1)) //was 255
		return;		// not in the player model

	if (ent->movetype == MOVETYPE_NOCLIP)
		return;

	deltav[0] = ent->velocity[0] - ent->client->oldvelocity[0];
	deltav[1] = ent->velocity[1] - ent->client->oldvelocity[1];
	deltav[2] = 0;
	delta = VectorLength(deltav);
	delta = delta*delta * 0.0001;
	if (delta > 40*(player_max_speed->value/300))
	{
		if (ent->health > 0)
		{
		/*	if(delta > 65)
				ent->s.event = EV_FALLFAR;
			else
				ent->s.event = EV_FALL;*/
			//play correct PPM sounds while in third person mode
			if (delta >= 65*(player_max_speed->value/300)) // Knightmare changed
				gi.sound(ent,CHAN_VOICE,gi.soundindex("*fall1.wav"),1.0,ATTN_NORM,0);
			else
				gi.sound(ent,CHAN_VOICE,gi.soundindex("*fall2.wav"),1.0,ATTN_NORM,0);
		}
		ent->pain_debounce_time = level.time;	// no normal pain sound
		damage = (delta-40*(player_max_speed->value/300))/2; // Knightmare changed
		if (damage < 1)
			damage = 1;
		VectorCopy(deltav,dir);
		VectorNegate(dir,dir);
		VectorNormalize(dir);
		if (!deathmatch->value || !((int)dmflags->value & DF_NO_FALLING) )
			T_Damage (ent, world, world, dir, ent->s.origin, vec3_origin, damage, 0, 0, MOD_FALLING);
	}
}


/*
=================
P_FallingDamage
=================
*/
void P_FallingDamage (edict_t *ent)
{
	float	delta;
	int		damage;
	vec3_t	dir;

	if (ent->s.modelindex != (MAX_MODELS-1)) //was 255
		return;		// not in the player model

	if (ent->movetype == MOVETYPE_NOCLIP)
		return;

	if (ent->client->jetpack && ent->client->ucmd.upmove > 0)
		return;

	if ((ent->client->oldvelocity[2] < 0) && (ent->velocity[2] > ent->client->oldvelocity[2]) && (!ent->groundentity))
	{
		delta = ent->client->oldvelocity[2];
	}
	else
	{
		if (!ent->groundentity)
			return;
		delta = ent->velocity[2] - ent->client->oldvelocity[2];
		ent->client->jumping = 0;
	}
	delta = delta*delta * 0.0001;

	// never take falling damage if completely underwater
	if (ent->waterlevel == 3)
		return;
	if (ent->waterlevel == 2)
		delta *= 0.25;
	if (ent->waterlevel == 1)
		delta *= 0.5;

	if (delta < 1)
		return;

	// Lazarus: Changed here to NOT play footstep sounds if ent isn't on the ground.
	//          So player will no longer play footstep sounds when descending a ladder.
	if (delta < 15 )
	{
		if (!(ent->watertype & CONTENTS_MUD) && !ent->vehicle && !ent->turret && ent->groundentity)
#ifndef FMOD_FOOTSTEPS
			ent->s.event = EV_FOOTSTEP; //Knightmare- move Lazarus footsteps client-side
#else
			FootStep(ent);
#endif
		return;
	}

	ent->client->fall_value = delta * 0.5;
	if (ent->client->fall_value > 40) ent->client->fall_value = 40;

	ent->client->fall_time = level.time + FALL_TIME;

	if (delta > MAX_SAFE_FALLDIST)	//CW: reduced safe falling distance, and replaced hardcoded value
	{
		if (ent->health > 0)
		{

/*			This change plays the correct sexed sounds while in
			third person view

			if (delta >= 55)
				ent->s.event = EV_FALLFAR;
			else
				ent->s.event = EV_FALL;
*/
			if (delta >= FALL_HURTBAD) gi.sound(ent, CHAN_VOICE, gi.soundindex("*fall1.wav"), 1.0, ATTN_NORM, 0);
			else gi.sound(ent, CHAN_VOICE, gi.soundindex("*fall2.wav"), 1.0, ATTN_NORM, 0);

			if (world->effects & FX_WORLDSPAWN_ALERTSOUNDS) PlayerNoise(ent, ent->s.origin, PNOISE_SELF);

		}

		ent->pain_debounce_time = level.time;	// no normal pain sound
		damage = (delta - MAX_SAFE_FALLDIST) * FALLDAMAGE_FACTOR;	//CW: increased falling damage, and replaced hardcoded values
		if (damage < 1)	damage = 1;

		VectorSet (dir, 0, 0, 1);

		if (!deathmatch->value || !((int)dmflags->value & DF_NO_FALLING) )
			T_Damage (ent, world, world, dir, ent->s.origin, vec3_origin, damage, 0, 0, MOD_FALLING);
	}
	else if (delta > 15)
	{
		ent->s.event = EV_FALLSHORT;
		if (world->effects & FX_WORLDSPAWN_ALERTSOUNDS)
			PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
		return;
	}
	else // if delta > 7
#ifndef FMOD_FOOTSTEPS
		ent->s.event = EV_LOUDSTEP; //Knightmare- loud footstep for softer landing
#else
		FootStep(ent);
#endif

}



/*
=============
P_WorldEffects
=============
*/
void P_WorldEffects (void)
{
	qboolean	breather;
	qboolean	envirosuit;
	qboolean	airy;		//CW
	int			waterlevel, old_waterlevel, old_watertype;

	if (current_player->movetype == MOVETYPE_NOCLIP)
	{
		current_player->air_finished = level.time + 12;	// don't need air
		return;
	}

	waterlevel = current_player->waterlevel;
	old_waterlevel = current_client->old_waterlevel;
	old_watertype  = current_player->old_watertype;
	current_client->old_waterlevel = waterlevel;
	current_player->old_watertype  = current_player->watertype;

	breather = current_client->breather_framenum > level.framenum;
	envirosuit = current_client->enviro_framenum > level.framenum;
	airy = current_client->in_func_air;	//CW

	//
	// if just entered a water volume, play a sound
	//
	if (!old_waterlevel && waterlevel)
	{
		PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
		if (current_player->watertype & CONTENTS_LAVA)
			gi.sound (current_player, CHAN_BODY, gi.soundindex("player/lava_in.wav"), 1, ATTN_NORM, 0);
		else if (current_player->watertype & CONTENTS_SLIME)
			gi.sound (current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		else if (current_player->watertype & CONTENTS_MUD)
			gi.sound (current_player, CHAN_BODY, gi.soundindex("mud/mud_in2.wav"), 1, ATTN_NORM, 0);
		else if (current_player->watertype & CONTENTS_WATER)
			gi.sound (current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		current_player->flags |= FL_INWATER;

		// clear damage_debounce, so the pain sound will play immediately
		current_player->damage_debounce_time = level.time - 1;
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (old_waterlevel && ! waterlevel)
	{
		PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
		if (old_watertype & CONTENTS_MUD)
			gi.sound (current_player, CHAN_BODY, gi.soundindex("mud/mud_out1.wav"), 1, ATTN_NORM, 0);
		else
			gi.sound (current_player, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
		current_player->flags &= ~FL_INWATER;
	}

	//
	// check for head just going under water
	//
	if (old_waterlevel != 3 && waterlevel == 3)
	{
		if (current_player->in_mud)
			gi.sound (current_player, CHAN_BODY, gi.soundindex("mud/mud_un1.wav"), 1, ATTN_NORM, 0);
		else
			gi.sound (current_player, CHAN_BODY, gi.soundindex("player/watr_un.wav"), 1, ATTN_NORM, 0);
	}

	//
	// check for head just coming out of water
	//
	if (old_waterlevel == 3 && waterlevel != 3)
	{
		if (current_player->air_finished < level.time)
		{	// gasp for air
			gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/gasp1.wav"), 1, ATTN_NORM, 0);
			PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
		}
		else  if (current_player->air_finished < level.time + 11)
		{	// just break surface
			gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/gasp2.wav"), 1, ATTN_NORM, 0);
		}
	}

	//
	// check for drowning
	//
	if (waterlevel == 3)
	{
#ifdef JETPACK_MOD
		if ( current_player->client->jetpack )
		{
			if ( (current_player->watertype & (CONTENTS_LAVA|CONTENTS_SLIME)) && !(current_player->flags & FL_GODMODE ) ) // blow up in lava/slime
				T_Damage (current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, current_player->health+1, 0, DAMAGE_NO_ARMOR, 0);
			else
			{
				gitem_t	*jetpack = FindItem("jetpack");
				Use_Jet (current_player, jetpack);			// shut down in water
			}
		}
#endif
		// breather or envirosuit give air
		if (breather || envirosuit || airy)	//CW: or in a func_air
		{
			current_player->air_finished = level.time + 10;

			if (!airy)  //CW
			{
				if (((int)(current_client->breather_framenum - level.framenum) % 25) == 0)
				{
					if (!current_client->breather_sound)
						gi.sound (current_player, CHAN_AUTO, gi.soundindex("player/u_breath1.wav"), 1, ATTN_NORM, 0);
					else
						gi.sound (current_player, CHAN_AUTO, gi.soundindex("player/u_breath2.wav"), 1, ATTN_NORM, 0);
					current_client->breather_sound ^= 1;
					PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
					//FIXME: release a bubble?
				}
			}
//CW+++
			else
			{
				if (!current_client->entered_func_air)
				{
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/gasp1.wav"), 1, ATTN_NORM, 0);
					PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
					current_client->entered_func_air = true;
					current_client->func_air_frame = level.framenum;
				}
				else
				{
					if (((int)(level.framenum - current_client->func_air_frame) % 20) == 0)
					{
						if (!current_client->airy_sound)
							gi.sound(current_player, CHAN_AUTO, gi.soundindex("player/gasp2.wav"), 1, ATTN_NORM, 0);
						else
							gi.sound (current_player, CHAN_AUTO, gi.soundindex("player/gasp1.wav"), 1, ATTN_NORM, 0);
						current_client->airy_sound = !current_client->airy_sound;
						PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
					}
				}
			}
//CW---
		}

		// if out of air, start drowning
		if (current_player->air_finished < level.time)
		{	// drown!
			if (current_player->client->next_drown_time < level.time 
				&& current_player->health > 0)
			{
				current_player->client->next_drown_time = level.time + 1;

				// take more damage the longer underwater
				current_player->dmg += 2;
				if (current_player->dmg > 15)
					current_player->dmg = 15;

				// play a gurp sound instead of a normal pain sound
				if (current_player->health <= current_player->dmg)
					gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/drown1.wav"), 1, ATTN_NORM, 0);
				else if (rand()&1)
					gi.sound (current_player, CHAN_VOICE, gi.soundindex("*gurp1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound (current_player, CHAN_VOICE, gi.soundindex("*gurp2.wav"), 1, ATTN_NORM, 0);

				current_player->pain_debounce_time = level.time;

				T_Damage (current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, current_player->dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	}
	else
	{
		current_player->air_finished = level.time + 12;
		current_player->dmg = 2;
	}

//CW+++
	if (!airy)
		current_client->entered_func_air = false;

	current_client->in_func_air = false;			//reset in case player moves out of field
//CW--

	//
	// check for sizzle damage
	//
	if (waterlevel && (current_player->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) )
	{
		if (current_player->watertype & CONTENTS_LAVA)
		{
			if (current_player->health > 0
				&& current_player->pain_debounce_time <= level.time
				&& current_client->invincible_framenum < level.framenum)
			{
				if (rand()&1)
					gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/burn1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/burn2.wav"), 1, ATTN_NORM, 0);
				current_player->pain_debounce_time = level.time + 1;
			}

			if (envirosuit)	// take 1/3 damage with envirosuit
				T_Damage (current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, 1*waterlevel, 0, 0, MOD_LAVA);
			else
				T_Damage (current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, 3*waterlevel, 0, 0, MOD_LAVA);
		}

		if (current_player->watertype & CONTENTS_SLIME)
		{
			if (!envirosuit)
			{	// no damage from slime with envirosuit
				T_Damage (current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, 1*waterlevel, 0, 0, MOD_SLIME);
			}
		}
	}
}


/*
===============
G_SetClientEffects
===============
*/
void G_SetClientEffects (edict_t *ent)
{
	int		pa_type;
	int		remaining;

	ent->s.effects = 0;
//	ent->s.renderfx = 0;
	// PGM - player is always ir visible, even dead.
	ent->s.renderfx = RF_IR_VISIBLE;

	if (ent->health <= 0 || level.intermissiontime)
		return;

	if(ent->flags & FL_DISGUISED)
		ent->s.renderfx |= RF_USE_DISGUISE;

	if (ent->powerarmor_time > level.time)
	{
		pa_type = PowerArmorType (ent);
		if (pa_type == POWER_ARMOR_SCREEN)
		{
			ent->s.effects |= EF_POWERSCREEN;
		}
		else if (pa_type == POWER_ARMOR_SHIELD)
		{
			ent->s.effects |= EF_COLOR_SHELL;
			ent->s.renderfx |= RF_SHELL_GREEN;
		}
	}

	if (ent->client->quad_framenum > level.framenum)
	{
		remaining = ent->client->quad_framenum - level.framenum;
		if (remaining > 30 || (remaining & 4) )
			ent->s.effects |= EF_QUAD;
	}

	if (ent->client->invincible_framenum > level.framenum)
	{
		remaining = ent->client->invincible_framenum - level.framenum;
		if (remaining > 30 || (remaining & 4) )
			ent->s.effects |= EF_PENT;
	}

	// show cheaters!!!
	if (ent->flags & FL_GODMODE)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= (RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE);
	}

	if (ent->client->flashlight)
	{
		vec3_t end, forward, offset, right, start, up;
		trace_t tr;

		if(level.flashlight_cost > 0) {
			if(!Q_stricmp(FLASHLIGHT_ITEM,"health") || 
					(ent->client->pers.inventory[ITEM_INDEX(FindItem(FLASHLIGHT_ITEM))]>=level.flashlight_cost) ) {
				// Player has items remaining
				if(ent->client->flashlight_time <= level.time) {
					ent->client->pers.inventory[ITEM_INDEX(FindItem(FLASHLIGHT_ITEM))]-=level.flashlight_cost;
					ent->client->flashlight_time = level.time + FLASHLIGHT_DRAIN;
				}
			} else {
				// Out of item
				ent->client->flashlight = false;
			}
		}
		if(ent->client->flashlight) {
			AngleVectors (ent->s.angles, forward, right, up);
			VectorSet(offset, 0, 0, ent->viewheight-8);
			G_ProjectSource (ent->s.origin, offset, forward, right, start);
			VectorMA(start,384,forward,end);	// was 128		
			tr = gi.trace (start,NULL,NULL, end, ent, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);
			if (tr.fraction != 1)
				VectorMA(tr.endpos,-4,forward,end);
			VectorCopy(tr.endpos,end);
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_FLASHLIGHT);
			gi.WritePosition (end);
			gi.WriteShort (ent - g_edicts);
			gi.multicast (end, MULTICAST_PVS);

		}
	}
}


/*
===============
G_SetClientEvent
===============
*/
void G_SetClientEvent (edict_t *ent)
{
	if (ent->s.event)
		return;

	if ( ent->groundentity )
	{
		if (!ent->waterlevel && ( xyspeed > 225) && !ent->vehicle)
		{
			if ( (int)(current_client->bobtime+bobmove) != bobcycle )
		#ifndef FMOD_FOOTSTEPS
				ent->s.event = EV_FOOTSTEP;	 // Knightmare- move Lazarus footsteps client-side
		#else
				FootStep(ent);
		#endif
		}
		else if( ent->in_mud && (ent->waterlevel == 1) && (xyspeed > 40))
		{
			if ( (level.framenum % 10) == 0 )
			{
			#ifndef FMOD_FOOTSTEPS
					ent->s.event = EV_WADE_MUD; // Knightmare- move this client-side
			#else
				if( rand() & 1 )
					gi.sound(ent, CHAN_BODY, gi.soundindex("mud/wade_mud1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound(ent, CHAN_BODY, gi.soundindex("mud/wade_mud2.wav"), 1, ATTN_NORM, 0);
			#endif
			}
		}
//		else if(  ( (ent->waterlevel == 1) || (ent->waterlevel == 2) ) && ( xyspeed > 100 ) )
		else if ( ((ent->waterlevel == 1) || (ent->waterlevel == 2)) && (xyspeed > 100) && !(ent->in_mud) )
		{
			if ( (int)(current_client->bobtime+bobmove) != bobcycle )
			{
			#ifndef FMOD_FOOTSTEPS
				if (ent->waterlevel == 1)
					ent->s.event = EV_SLOSH;	 // Knightmare- move Lazarus footsteps client-side
				else if (ent->waterlevel == 2)
					ent->s.event = EV_WADE;	 // Knightmare- move Lazarus footsteps client-side
			#else
				FootStep(ent);
			#endif
			}
		}
	}
	// Knightmare- swimming sounds
	else if ((ent->waterlevel == 2) && (xyspeed > 60) && !(ent->in_mud) && (world->effects & FX_WORLDSPAWN_STEPSOUNDS))
	{
		if ( (int)(current_client->bobtime+bobmove) != bobcycle )
	#ifndef FMOD_FOOTSTEPS
			ent->s.event = EV_WADE;	 // Knightmare- move Lazarus footsteps client-side
	#else
			FootStep(ent);
	#endif
	}
	// Ladder sounds
	else if( (level.framenum % 4) == 0)
	{
		if(!ent->waterlevel && (ent->movetype != MOVETYPE_NOCLIP) && (fabs(ent->velocity[2]) > 50))
		{
			vec3_t	end, forward;
			trace_t	tr;
			AngleVectors(ent->s.angles,forward,NULL,NULL);
			VectorMA(ent->s.origin,2,forward,end);
			tr = gi.trace(ent->s.origin,ent->mins,ent->maxs,end,ent,CONTENTS_LADDER);
			if(tr.fraction < 1.0)
		#ifndef FMOD_FOOTSTEPS
				ent->s.event = EV_CLIMB_LADDER;	 // Knightmare- move Lazarus footsteps client-side
		#else
			{
				int	r;
				r = rand() & 1 + ent->client->leftfoot*2;
				ent->client->leftfoot = 1 - ent->client->leftfoot;
				if(qFMOD_Footsteps)
				{
					switch (r){
					case 0: PlayFootstep(ent,FOOTSTEP_LADDER1); break;
					case 1: PlayFootstep(ent,FOOTSTEP_LADDER3); break;
					case 2: PlayFootstep(ent,FOOTSTEP_LADDER2); break;
					case 3: PlayFootstep(ent,FOOTSTEP_LADDER4); break;
					}
				}
				else
				{
					switch (r){
					case 0:	gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_ladder1.wav"),1.0,ATTN_NORM,0); break;
					case 1: gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_ladder3.wav"),1.0,ATTN_NORM,0); break;
					case 2:	gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_ladder2.wav"),1.0,ATTN_NORM,0); break;
					case 3:	gi.sound(ent,CHAN_VOICE,gi.soundindex("player/pl_ladder4.wav"),1.0,ATTN_NORM,0); break;
					}
				}
			}
		#endif //FMOD_FOOTSTEPS
		}
	}
}

/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound (edict_t *ent)
{
	char	*weap;

	if (ent->client->pers.game_helpchanged != game.helpchanged)
	{
		ent->client->pers.game_helpchanged = game.helpchanged;
		ent->client->pers.helpchanged = 1;
	}

	// help beep (no more than three times)
	if (ent->client->pers.helpchanged && ent->client->pers.helpchanged <= 3 && !(level.framenum&63) )
	{
		ent->client->pers.helpchanged++;
		gi.sound (ent, CHAN_VOICE, gi.soundindex ("misc/pc_up.wav"), 1, ATTN_STATIC, 0);
	}


	if (ent->client->pers.weapon)
		weap = ent->client->pers.weapon->classname;
	else
		weap = "";

	if (ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) )
		ent->s.sound = snd_fry;
	else if ( ent->client->jetpack && (ent->client->pers.inventory[fuel_index] < 40 ))
		ent->s.sound = gi.soundindex("jetpack/stutter.wav");
	else if (strcmp(weap, "weapon_railgun") == 0)
		ent->s.sound = gi.soundindex("weapons/rg_hum.wav");
	else if (strcmp(weap, "weapon_bfg") == 0)
		ent->s.sound = gi.soundindex("weapons/bfg_hum.wav");
	else if (ent->client->weapon_sound)
		ent->s.sound = ent->client->weapon_sound;
	else
		ent->s.sound = 0;
}

//#define MAX_STEP_FRACTION 0.80
qboolean PlayerOnFloor (edict_t *player)
{
	trace_t		tr;
	vec3_t		end = {0, 0, -2};

	if (!player->client)
		return false;

	VectorMA (player->s.origin, 50, end, end);
    tr = gi.trace (player->s.origin, NULL, NULL, end, player, MASK_ALL);
    //Com_Printf("%f\n", tr.fraction);
	if (tr.fraction >= sv_step_fraction->value)
		return false;
	else if (player->client->oldvelocity[2] > 0 || player->velocity[2] > 0)
		return false;

	return true;
}

/*
===============
G_SetClientFrame
===============
*/
void G_SetClientFrame (edict_t *ent)
{
	gclient_t	*client;
	qboolean	duck, run;
	qboolean	floor;

	if (ent->s.modelindex != (MAX_MODELS-1)) //was 255
		return;		// not in the player model

	client = ent->client;

	if (client->ps.pmove.pm_flags & PMF_DUCKED)
		duck = true;
	else
		duck = false;
	// Knightmare- don't always play running animation when in mud
	if (ent->in_mud && xyspeed > 40)
		run = true;
	else if (!ent->in_mud && xyspeed)
		run = true;
	else
		run = false;

	// Knightmare- do the check here, to be sure not to skip over the frame increment
	floor = PlayerOnFloor(ent);

	// Lazarus: override run animations for vehicle drivers
	if (ent->vehicle)
		run = false;

	// check for stand/duck and stop/go transitions
	if (duck != client->anim_duck && client->anim_priority < ANIM_DEATH)
		goto newanim;
	if (run != client->anim_run && client->anim_priority == ANIM_BASIC)
		goto newanim;
	// Knightmare- only skip increment if greater than step or swimming
	if (!ent->groundentity && client->anim_priority <= ANIM_WAVE && (!floor || ent->waterlevel > 2))
		goto newanim;

	if(client->anim_priority == ANIM_REVERSE)
	{
		if(ent->s.frame > client->anim_end)
		{
			ent->s.frame--;
			return;
		}
	}
	else if (ent->s.frame < client->anim_end)
	{	// continue an animation
		ent->s.frame++;
		return;
	}

	if (client->anim_priority == ANIM_DEATH)
		return;		// stay there
	if (client->anim_priority == ANIM_JUMP)
	{
		if (!ent->groundentity)
			return;		// stay there
		ent->client->anim_priority = ANIM_WAVE;
		ent->s.frame = FRAME_jump3;
		ent->client->anim_end = FRAME_jump6;
		return;
	}

newanim:
	// return to either a running or standing frame
	client->anim_priority = ANIM_BASIC;
	client->anim_duck = duck;
	client->anim_run = run;

	// Knightmare- added swimming check
	if (!ent->groundentity && (!floor || ent->waterlevel > 2)) //CDawg modify this
	{
		client->anim_priority = ANIM_JUMP;
		if (ent->s.frame != FRAME_jump2)
			ent->s.frame = FRAME_jump1;
		client->anim_end = FRAME_jump2;
	}
	else if (run)
	{	// running
		if (duck)
		{
			ent->s.frame = FRAME_crwalk1;
			client->anim_end = FRAME_crwalk6;
		}
		else
		{	// CDawg - add here!
			if (client->backpedaling)
			{
				client->anim_priority = ANIM_REVERSE;
				ent->s.frame = FRAME_run6;
				client->anim_end = FRAME_run1; 
			}
			else
			{
				ent->s.frame = FRAME_run1;
				client->anim_end = FRAME_run6;
			} // CDawg end here!
		}
	}
	else
	{	// standing
		if (duck)
		{
			ent->s.frame = FRAME_crstnd01;
			client->anim_end = FRAME_crstnd19;
		}
		else
		{
			ent->s.frame = FRAME_stand01;
			client->anim_end = FRAME_stand40;
		}
	}
}


/*
=================
ClientEndServerFrame

Called for each player at the end of the server frame
and right after spawning
=================
*/
// Lazarus: "whatsit" display

void WhatsIt(edict_t *ent)
{
	char string[128];

	if(!ent->client->whatsit)
		return;

	sprintf(string, "xv 0 yb -68 cstring2 \"%s\" ", ent->client->whatsit);
	gi.WriteByte (svc_layout);
	gi.WriteString (string);
	gi.unicast(ent,true);
}

void ClientEndServerFrame (edict_t *ent)
{
	float	bobtime;
	int		i;

	current_player = ent;
	current_client = ent->client;

	//
	// If the origin or velocity have changed since ClientThink(),
	// update the pmove values.  This will happen when the client
	// is pushed by a bmodel or kicked by an explosion.
	// 
	// If it wasn't updated here, the view position would lag a frame
	// behind the body position when pushed -- "sinking into plats"
	//
	for (i=0 ; i<3 ; i++)
	{
		current_client->ps.pmove.origin[i] = ent->s.origin[i]*8.0;
		current_client->ps.pmove.velocity[i] = ent->velocity[i]*8.0;
	}

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if (level.intermissiontime)
	{
		// FIXME: add view drifting here?
		current_client->ps.blend[3] = 0;
		current_client->ps.fov = 90;
		G_SetStats (ent);
		return;
	}

	AngleVectors (ent->client->v_angle, forward, right, up);

	// burn from lava, etc
	P_WorldEffects ();

	//
	// set model angles from view angles so other things in
	// the world can tell which direction you are looking
	//
	if (ent->client->v_angle[PITCH] > 180)
		ent->s.angles[PITCH] = (-360 + ent->client->v_angle[PITCH])/3;
	else
		ent->s.angles[PITCH] = ent->client->v_angle[PITCH]/3;
	ent->s.angles[YAW] = ent->client->v_angle[YAW];
	ent->s.angles[ROLL] = 0;
	ent->s.angles[ROLL] = SV_CalcRoll (ent->s.angles, ent->velocity)*4;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	xyspeed = sqrt(ent->velocity[0]*ent->velocity[0] + ent->velocity[1]*ent->velocity[1]);

	if (xyspeed < 5)
	{
		bobmove = 0;
		current_client->bobtime = 0;	// start at beginning of cycle again
	}
	else if (ent->groundentity)
	{	// so bobbing only cycles when on ground
		if (xyspeed > 210)
			bobmove = 0.25;
		else if (xyspeed > 100)
			bobmove = 0.125;
		else
			bobmove = 0.0625;
	}
	
	bobtime = (current_client->bobtime += bobmove);

	if (current_client->ps.pmove.pm_flags & PMF_DUCKED)
		bobtime *= 4;

	bobcycle = (int)bobtime;

	// Lazarus: vehicle drivers don't bob
	if(ent->vehicle)
		bobfracsin = 0.;
	else
		bobfracsin = fabs(sin(bobtime*M_PI));

	// detect hitting the floor
	P_FallingDamage (ent);

	// Lazarus: detect hitting walls
	P_SlamDamage (ent);

	// apply all the damage taken this frame
	P_DamageFeedback (ent);

	// determine the view offsets
	SV_CalcViewOffset (ent);

	// determine the gun offsets
	SV_CalcGunOffset (ent);

	// determine the full screen color blend
	// must be after viewoffset, so eye contents can be
	// accurately determined
	// FIXME: with client prediction, the contents
	// should be determined by the client
	SV_CalcBlend (ent);

	// chase cam stuff
	if (ent->client->resp.spectator)
		G_SetSpectatorStats(ent);
	else
		G_SetStats (ent);
	G_CheckChaseStats(ent);

	G_SetClientEvent (ent);

	G_SetClientEffects (ent);

	G_SetClientSound (ent);

	G_SetClientFrame (ent);

	VectorCopy (ent->velocity, ent->client->oldvelocity);
	VectorCopy (ent->client->ps.viewangles, ent->client->oldviewangles);

	// clear weapon kicks
	VectorClear (ent->client->kick_origin);
	VectorClear (ent->client->kick_angles);

	// if the scoreboard is up, update it
	if (!(level.framenum & 31))
	{
		if (ent->client->showscores)
		{
			if (ent->client->menu)
				PMenu_Update(ent);
			else if (ent->client->textdisplay)
				Text_Update(ent);
			else
				DeathmatchScoreboardMessage (ent, ent->enemy);
			gi.unicast (ent, false);
		}
		else if(ent->client->whatsit)
			WhatsIt(ent);
	}

	// tpp
	if (ent->client->chasetoggle == 1)
		CheckChasecam_Viewent(ent);
	// end tpp

}

