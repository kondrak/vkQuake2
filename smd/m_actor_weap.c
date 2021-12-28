#include "g_local.h"
#include "m_actor.h"

void muzzleflash_think(edict_t *flash)
{
	if(level.time >= flash->wait)
	{
		flash->svflags |= SVF_NOCLIENT;
		flash->s.effects &= ~EF_HYPERBLASTER;
	}
	else
	{
		flash->svflags &= ~SVF_NOCLIENT;
		if(flash->s.frame ^= 1)
			flash->s.effects |= EF_HYPERBLASTER;
		else
			flash->s.effects &= ~EF_HYPERBLASTER;
		flash->nextthink = level.time + FRAMETIME;
	}
	gi.linkentity(flash);
}

void TraceAimPoint(vec3_t start,vec3_t target)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_DEBUGTRAIL);
	gi.WritePosition (start);
	gi.WritePosition (target);
	gi.multicast (start, MULTICAST_ALL);
}
void ActorTarget(edict_t *self, vec3_t target)
{
	float	accuracy;
	float	dist;
	float	tf;
	vec3_t	v;

	if(!self->enemy) {
		VectorClear(target);
		return;
	}
	if(self->monsterinfo.aiflags & AI_GOOD_GUY)
		accuracy = 5.0 - skill->value;
	else
		accuracy = skill->value + 2.0;

	if (self->enemy->health > 0)
	{
		int		weapon;
		trace_t	tr;
		vec3_t	start;
		qboolean	can_see=false;

		VectorCopy(self->s.origin,start);
		start[2] += self->viewheight;
		VectorMA (self->enemy->s.origin, -0.2, self->enemy->velocity, target);
		weapon = self->actor_weapon[self->actor_current_weapon];

		if(weapon == 7 && (rand() & 1))
		{
			// Fire rockets at feet half the time
			target[2] += self->enemy->mins[2] + 1;
			tr = gi.trace(start,NULL,NULL,target,self,MASK_SHOT);
			if(tr.ent == self->enemy)
				can_see=true;
			else
				target[2] -= self->enemy->mins[2] + 1;
		}
		if(!can_see)
		{
			// Fire at origin if origin can be seen
			tr = gi.trace(start,NULL,NULL,target,self,MASK_SHOT);
			if(tr.ent == self->enemy)
				can_see = true;
		}
		// Otherwise fire at eyeballs
		if(!can_see)
			target[2] += self->enemy->viewheight;
	}
	else
	{
		// For dead targets, fire at center of bounding box (point entities)
		// or origin (brush models)
		if(self->enemy->solid == SOLID_BBOX)
			VectorMA(self->enemy->absmin,0.5,self->enemy->size,target);
		else
			VectorAdd(self->enemy->s.origin,self->enemy->origin_offset,target);
	}

	if(accuracy == 5.0)
		return;

	VectorSubtract(target,self->s.origin,v);
	dist = VectorLength(v);

	tf = (dist < 256) ? dist/2 : 256;
	tf *= (float) (5.0 - accuracy) / 25.0;
	VectorAdd(target, tv(crandom() * tf, crandom() * tf, crandom() * tf * 0.2), target);
}

//Blaster
void actorBlaster (edict_t *self)
{
	vec3_t	start, target;
	vec3_t	forward, right, up;
	int		damage;
	int		effect, color;

	if(!self->enemy || !self->enemy->inuse)
		return;
	// Knightmare- select color and effect
	if (sk_blaster_color->value == 2) { //green
		color = BLASTER_GREEN;
		effect = (EF_BLASTER|EF_TRACKER);
	}
	else if (sk_blaster_color->value == 3) { //blue
		color = BLASTER_BLUE;
#ifdef KMQUAKE2_ENGINE_MOD
		effect = EF_BLASTER|EF_BLUEHYPERBLASTER;
#else
		effect = EF_BLUEHYPERBLASTER;
#endif
	}
#ifdef KMQUAKE2_ENGINE_MOD
	else if (sk_blaster_color->value == 4) {//red
		color = BLASTER_RED;
		effect = EF_BLASTER|EF_IONRIPPER;
	}
#endif
	else { //standard yellow
		color = BLASTER_ORANGE;
		effect = EF_BLASTER;
	}

	AngleVectors (self->s.angles, forward, right, up);
	G_ProjectSource2 (self->s.origin, self->muzzle, forward, right, up, start);
	ActorTarget(self,target);
	VectorSubtract (target, start, forward);
	VectorNormalize (forward);

	if(self->monsterinfo.aiflags & AI_TWO_GUNS)
		damage = 5;
	else
		damage = 10;

	monster_fire_blaster (self, start, forward, damage, 600, MZ2_SOLDIER_BLASTER_2, effect, color);

	if(developer->value)
		TraceAimPoint(start,target);

	if(self->monsterinfo.aiflags & AI_TWO_GUNS)
	{
		G_ProjectSource2(self->s.origin,self->muzzle2,forward,right,up,start);
		VectorSubtract (target, start, forward);
		VectorNormalize (forward);
		monster_fire_blaster (self, start, forward, damage, 600, MZ2_SOLDIER_BLASTER_2, effect, color);
	}

}

//Shotgun
void actorShotgun (edict_t *self)
{
	vec3_t	start, target;
	vec3_t	forward, right, up;

	if(!self->enemy || !self->enemy->inuse)
		return;

	AngleVectors (self->s.angles, forward, right, up);
	if(self->monsterinfo.aiflags & AI_TWO_GUNS)
	{
		if(self->framenumbers % 2)
			G_ProjectSource2(self->s.origin, self->muzzle2, forward, right, up, start);
		else
			G_ProjectSource2(self->s.origin, self->muzzle, forward, right, up, start);
		self->framenumbers++;
	}
	else
		G_ProjectSource2 (self->s.origin, self->muzzle, forward, right, up, start);

	ActorTarget(self,target);
	VectorSubtract (target, start, forward);
	VectorNormalize (forward);
	fire_shotgun (self, start, forward, 4, 8, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SHOTGUN_COUNT, MOD_SHOTGUN);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_CHAINFIST_SMOKE);
	gi.WritePosition(start);
	gi.multicast(start, MULTICAST_PVS);
	gi.positioned_sound(start,self,CHAN_WEAPON,gi.soundindex("weapons/shotgf1b.wav"),1,ATTN_NORM,0);

	if(self->flash)
	{
		VectorCopy(start,self->flash->s.origin);
		self->flash->s.frame = 0;
		self->flash->think   = muzzleflash_think;
		self->flash->wait    = level.time + FRAMETIME;
		self->flash->think(self->flash);
	}

	if(developer->value)
	{
		if (!(self->monsterinfo.aiflags & AI_TWO_GUNS) || (self->framenumbers % 2))
			TraceAimPoint(start,target);
	}
}

//Super Shotgun
void actorSuperShotgun (edict_t *self)
{
	vec3_t	start, target;
	vec3_t	forward, right, up;
	vec3_t	angles;

	if(!self->enemy || !self->enemy->inuse)
		return;

	AngleVectors (self->s.angles, forward, right, up);

	if(self->monsterinfo.aiflags & AI_TWO_GUNS)
	{
		if(self->framenumbers % 2)
			G_ProjectSource2 (self->s.origin, self->muzzle2, forward, right, up, start);
		else
			G_ProjectSource2 (self->s.origin, self->muzzle, forward, right, up, start);
		self->framenumbers++;
	}
	else
		G_ProjectSource2 (self->s.origin, self->muzzle, forward, right, up, start);

	ActorTarget(self,target);
	VectorSubtract (target, start, forward);
	VectorNormalize (forward);
	vectoangles(forward,angles);
	angles[YAW] -= 5;
	AngleVectors(angles,forward,NULL,NULL);
	fire_shotgun (self, start, forward, 6, 12, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT/2, MOD_SSHOTGUN);
	angles[YAW] += 10;
	AngleVectors(angles,forward,NULL,NULL);
	fire_shotgun (self, start, forward, 6, 12, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT/2, MOD_SSHOTGUN);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_CHAINFIST_SMOKE);
	gi.WritePosition(start);
	gi.multicast(start, MULTICAST_PVS);
	gi.positioned_sound(start,self,CHAN_WEAPON,gi.soundindex("weapons/sshotf1b.wav"),1,ATTN_NORM,0);

	if(self->flash)
	{
		VectorCopy(start,self->flash->s.origin);
		self->flash->s.frame = 0;
		self->flash->think   = muzzleflash_think;
		self->flash->wait    = level.time + FRAMETIME;
		self->flash->think(self->flash);
	}

	if(developer->value)
	{
		if (!(self->monsterinfo.aiflags & AI_TWO_GUNS) || (self->framenumbers % 2))
			TraceAimPoint(start,target);
	}
}

// Machinegun
void actorMachineGun (edict_t *self)
{
	vec3_t	start, target;
	vec3_t	forward, right, up;
	int		damage;

	if(!self->enemy || !self->enemy->inuse) {
		self->monsterinfo.pausetime = 0;
		return;
	}

	AngleVectors (self->s.angles, forward, right, up);
	G_ProjectSource2 (self->s.origin, self->muzzle, forward, right, up, start);
	ActorTarget(self,target);
	VectorSubtract (target, start, forward);
	VectorNormalize (forward);

	if(self->monsterinfo.aiflags & AI_TWO_GUNS)
		damage = 2;
	else
		damage = 4;

	fire_bullet (self, start, forward, damage, 2, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_MACHINEGUN);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_CHAINFIST_SMOKE);
	gi.WritePosition(start);
	gi.multicast(start, MULTICAST_PVS);
	gi.positioned_sound(start,self,CHAN_WEAPON,gi.soundindex(va("weapons/machgf%db.wav",self->actor_gunframe % 5 + 1)),1,ATTN_NORM,0);

	if(self->flash)
	{
		VectorCopy(start,self->flash->s.origin);
		self->flash->think = muzzleflash_think;
		self->flash->wait  = level.time + FRAMETIME;
		self->flash->think(self->flash);
	}

	if(developer->value)
		TraceAimPoint(start,target);

	if(self->monsterinfo.aiflags & AI_TWO_GUNS)
	{
		G_ProjectSource2 (self->s.origin, self->muzzle2, forward, right, up, start);
		ActorTarget(self,target);
		VectorSubtract (target, start, forward);
		VectorNormalize (forward);
		fire_bullet (self, start, forward, damage, 2, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_MACHINEGUN);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_CHAINFIST_SMOKE);
		gi.WritePosition(start);
		gi.multicast(start, MULTICAST_PVS);
	}
}

// Chaingun
void actorChaingun (edict_t *self)
{
	vec3_t	start, target;
	vec3_t	forward, right, up;
	int		i;
	int		shots;
	int		damage;

	if(!self->enemy || !self->enemy->inuse)
		self->monsterinfo.pausetime = 0;

	if(level.time >= self->monsterinfo.pausetime) {
		self->s.sound = 0;
		gi.sound(self,CHAN_AUTO,gi.soundindex("weapons/chngnd1a.wav"),1,ATTN_IDLE,0);
		return;
	}

	if(self->actor_gunframe == 0)
		gi.sound(self, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);

	if(self->actor_gunframe == 21 && level.time < self->monsterinfo.pausetime)
		self->actor_gunframe = 15;
	else
		self->actor_gunframe++;

	self->s.sound = gi.soundindex("weapons/chngnl1a.wav");
#ifdef LOOP_SOUND_ATTENUATION
	self->s.attenuation = ATTN_IDLE;
#endif

	if(self->actor_gunframe <= 9)
		shots = 1;
	else if(self->actor_gunframe <= 14)
		shots = 2;
	else
		shots = 3;

	AngleVectors (self->s.angles, forward, right, up);
	G_ProjectSource2 (self->s.origin, self->muzzle, forward, right, up, start);
	ActorTarget(self,target);
	VectorSubtract (target, start, forward);
	VectorNormalize (forward);
	if(self->monsterinfo.aiflags & AI_TWO_GUNS)
		damage = 2;
	else
		damage = 4;

	for(i=0; i<shots; i++)
		fire_bullet (self, start, forward, damage, 2, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_CHAINGUN);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_CHAINFIST_SMOKE);
	gi.WritePosition(start);
	gi.multicast(start, MULTICAST_PVS);
	gi.positioned_sound(start,self,CHAN_WEAPON,gi.soundindex(va("weapons/machgf%db.wav",self->actor_gunframe % 5 + 1)),1,ATTN_NORM,0);

	if(self->flash)
	{
		VectorCopy(start,self->flash->s.origin);
		self->flash->think = muzzleflash_think;
		self->flash->wait  = level.time + FRAMETIME;
		self->flash->think(self->flash);
	}

	if(developer->value)
		TraceAimPoint(start,target);

	if(self->monsterinfo.aiflags & AI_TWO_GUNS)
	{
		G_ProjectSource2 (self->s.origin, self->muzzle2, forward, right, up, start);
		ActorTarget(self,target);
		VectorSubtract (target, start, forward);
		VectorNormalize (forward);
		for(i=0; i<shots; i++)
			fire_bullet (self, start, forward, damage, 2, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_CHAINGUN);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_CHAINFIST_SMOKE);
		gi.WritePosition(start);
		gi.multicast(start, MULTICAST_PVS);
	}
}

// Grenade Launcher
// DWH: Pretty much a straight copy from Lazarus gunner code
#define GRENADE_VELOCITY 632.4555320337
#define GRENADE_VELOCITY_SQUARED 400000
void actorGrenadeLauncher (edict_t *self)
{
	vec3_t	start,target;
	vec3_t	forward, right, up;
	vec3_t	aim;
	vec3_t	dist;
	vec_t	monster_speed;

	if(!self->enemy || !self->enemy->inuse)
		return;

	AngleVectors (self->s.angles, forward, right, up);

	if(self->monsterinfo.aiflags & AI_TWO_GUNS)
	{
		if(self->framenumbers % 2)
			G_ProjectSource2 (self->s.origin, self->muzzle2, forward, right, up, start);
		else
			G_ProjectSource2 (self->s.origin, self->muzzle, forward, right, up, start);
		self->framenumbers++;
	}
	else
		G_ProjectSource2 (self->s.origin, self->muzzle, forward, right, up, start);

	ActorTarget(self,target);
	if(self->enemy->absmin[2] <= self->absmax[2])
		target[2] += self->enemy->mins[2] - self->enemy->viewheight;
	VectorSubtract(target, start, aim);

	// lead target... 20, 35, 50, 65 chance of leading
	if( random() < (0.2 + skill->value * 0.15) )
	{
		float	dist;
		float	time;

		dist = VectorLength (aim);
		time = dist/GRENADE_VELOCITY;  // Not correct, but better than nothin'
		VectorMA(target, time, self->enemy->velocity, target);
		VectorSubtract(target, start, aim);
	}
	
	VectorCopy(aim,forward);
	VectorNormalize (aim);
	if(aim[2] < 1.0) {
		float	cosa, t, x, vx, y;
		float	drop;
		float	last_error, last_up=0.f, v_error;
		int		i;
		VectorCopy(forward,target);	// save target point
		// horizontal distance to target
		x = sqrt( forward[0]*forward[0] + forward[1]*forward[1]);
		cosa = sqrt(aim[0]*aim[0] + aim[1]*aim[1]);
		// constant horizontal velocity (since grenades don't have drag)
		vx = GRENADE_VELOCITY * cosa;
		// time to reach target x
		t = x/vx;
		// in that time, grenade will drop this much:
		drop = 0.5*sv_gravity->value*t*(t+FRAMETIME);
		forward[2] = target[2] + drop;
		// this is a good first cut, but incorrect since angle now changes, so
		// horizontal speed changes
		VectorCopy(forward,aim);
		VectorNormalize(aim);
		cosa = sqrt(aim[0]*aim[0] + aim[1]*aim[1]);
		vx = GRENADE_VELOCITY * cosa;
		t = x/vx;
		y = GRENADE_VELOCITY*aim[2]*t - 0.5*sv_gravity->value*t*(t+FRAMETIME);
		v_error = target[2]-y;
		last_error = 2*v_error;
		for(i=0; i<10 && fabs(v_error) > 4 && fabs(v_error) < fabs(last_error); i++) {
			drop = 0.5*sv_gravity->value*t*(t+FRAMETIME);
			forward[2] = target[2] + drop;
			VectorCopy(forward,aim);
			VectorNormalize(aim);
			cosa = sqrt(aim[0]*aim[0] + aim[1]*aim[1]);
			vx = GRENADE_VELOCITY * cosa;
			t = x/vx;
			y = GRENADE_VELOCITY*aim[2]*t - 0.5*sv_gravity->value*t*(t+FRAMETIME);
			v_error = target[2]-y;
			// If error is increasing... we can't get there from here and
			// probably shouldn't be here in the first place. Too late now...
			// use last aim vector and shoot.
			if(fabs(v_error) < fabs(last_error))
				last_up = forward[2];
		}
		if(fabs(v_error) > fabs(last_error)) {
			forward[2] = last_up;
			VectorCopy(forward,aim);
			VectorNormalize(aim);
		}
		// Sanity check... if gunner is at the same elevation or a bit above the 
		// target entity, check to make sure he won't bounce grenades off the 
		// top of a doorway. If he WOULD do that, then figure out the max elevation
		// angle that will get the grenade through the door, and hope we get a 
		// good bounce.
		if( (self->s.origin[2] - self->enemy->s.origin[2] < 160) &&
			(self->s.origin[2] - self->enemy->s.origin[2] > -16)   ) {
			trace_t	tr;

			VectorAdd(start,forward,target);
			tr = gi.trace(start,vec3_origin,vec3_origin,target,self,MASK_SOLID);
			if(tr.fraction < 1.0) {
				// OK... the aim vector hit a solid, but would the grenade actually hit?
				int		contents;

				cosa = sqrt(aim[0]*aim[0] + aim[1]*aim[1]);
				vx = GRENADE_VELOCITY * cosa;
				VectorSubtract(tr.endpos,start,dist);
				dist[2] = 0;
				x = VectorLength(dist);
				t = x/vx;
				drop = 0.5*sv_gravity->value*t*(t+FRAMETIME);
				tr.endpos[2] -= drop;
				// move just a bit in the aim direction
				tr.endpos[0] += aim[0];
				tr.endpos[1] += aim[1];
				contents = gi.pointcontents(tr.endpos);
				while((contents & MASK_SOLID) && (target[2] > self->enemy->s.origin[2])) {
					target[2] -= 8.0;
					VectorSubtract(target,start,forward);
					VectorCopy(forward,aim);
					VectorNormalize(aim);
					tr = gi.trace(start,vec3_origin,vec3_origin,target,self,MASK_SOLID);
					if(tr.fraction < 1.0) {
						cosa = sqrt(aim[0]*aim[0] + aim[1]*aim[1]);
						vx = GRENADE_VELOCITY * cosa;
						VectorSubtract(tr.endpos,start,dist);
						dist[2] = 0;
						x = VectorLength(dist);
						t = x/vx;
						drop = 0.5*sv_gravity->value*t*(t+FRAMETIME);
						tr.endpos[2] -= drop;
						tr.endpos[0] += aim[0];
						tr.endpos[1] += aim[1];
						contents = gi.pointcontents(tr.endpos);
					}
					// drop aim point another bit for insurance
					target[2] -= 8;
					VectorSubtract(target,start,forward);
					VectorCopy(forward,aim);
					VectorNormalize(aim);
				}
			}
		}
	}
	// DWH - take into account (sort of) Lazarus feature of adding shooter's velocity to
	// grenade velocity
	monster_speed = VectorLength(self->velocity);
	if(monster_speed > 0) {
		vec3_t	v1;
		vec_t	delta;

		VectorCopy(self->velocity,v1);
		VectorNormalize(v1);
		delta = -monster_speed/GRENADE_VELOCITY;
		VectorMA(aim,delta,v1,aim);
		VectorNormalize(aim);
	}
	fire_grenade (self, start, aim, 50, (int)GRENADE_VELOCITY, 2.5, 90, false);

	gi.positioned_sound(start,self,CHAN_WEAPON,gi.soundindex("weapons/grenlf1a.wav"),1,ATTN_NORM,0);

	if(developer->value)
	{
		if (!(self->monsterinfo.aiflags & AI_TWO_GUNS) || (self->framenumbers % 2))
			TraceAimPoint(start,target);
	}
}


// Rocket Launcher
void actorRocket (edict_t *self)
{
	vec3_t	start, target;
	vec3_t	forward, right, up;
	int		damage = 80;

	if(!self->enemy || !self->enemy->inuse)
		return;

	AngleVectors (self->s.angles, forward, right, up);
	if(self->monsterinfo.aiflags & AI_TWO_GUNS)
	{
		if(self->framenumbers % 2)
			G_ProjectSource2 (self->s.origin, self->muzzle2, forward, right, up, start);
		else
			G_ProjectSource2 (self->s.origin, self->muzzle, forward, right, up, start);
		self->framenumbers++;
	}
	else
		G_ProjectSource2 (self->s.origin, self->muzzle, forward, right, up, start);
	ActorTarget(self,target);
	VectorSubtract (target, start, forward);
	VectorNormalize (forward);
	fire_rocket (self, start, forward, damage, 550, damage+20, damage, 
		( (self->spawnflags & SF_MONSTER_SPECIAL) ? self->enemy : NULL) );

	gi.positioned_sound(start,self,CHAN_WEAPON,gi.soundindex("weapons/rocklf1a.wav"),1,ATTN_NORM,0);

	if(developer->value)
	{
		if (!(self->monsterinfo.aiflags & AI_TWO_GUNS) || (self->framenumbers % 2))
			TraceAimPoint(start,target);
	}
}

void actorHyperblaster (edict_t *self)
{
	vec3_t	start, target;
	vec3_t	forward, right, up;
	int		damage;
	int		effect;
	int		color;

	if(!self->enemy || !self->enemy->inuse) {
		self->monsterinfo.pausetime = 0;
		self->s.sound = 0;
		return;
	}

	self->s.sound = gi.soundindex("weapons/hyprbl1a.wav");
#ifdef LOOP_SOUND_ATTENUATION
	self->s.attenuation = ATTN_IDLE;
#endif

	if (level.time >= self->monsterinfo.pausetime)
	{
		self->actor_gunframe++;
	}
	else
	{
		// Knightmare- select color
		if (sk_hyperblaster_color->value == 2) //green
			color = BLASTER_GREEN;
		else if (sk_hyperblaster_color->value == 3) //blue
			color = BLASTER_BLUE;
	#ifdef KMQUAKE2_ENGINE_MOD
		else if (sk_hyperblaster_color->value == 4) //red
			color = BLASTER_RED;
	#endif
		else //standard yellow
			color = BLASTER_ORANGE;

		AngleVectors (self->s.angles, forward, right, up);
		G_ProjectSource2 (self->s.origin, self->muzzle, forward, right, up, start);
		ActorTarget(self,target);
		VectorSubtract (target, start, forward);
		VectorNormalize (forward);
		if ((random() * 3) < 1)
			effect = EF_HYPERBLASTER;
		else
			effect = 0;

		gi.positioned_sound(start,self,CHAN_WEAPON,gi.soundindex("weapons/hyprbf1a.wav"),1,ATTN_NORM,0);

		if(self->monsterinfo.aiflags & AI_TWO_GUNS)
			damage = 8;
		else
			damage = 15;

		fire_blaster (self, start, forward, damage, 1000, effect, true, color);

		if(developer->value)
			TraceAimPoint(start,target);

		if(self->monsterinfo.aiflags & AI_TWO_GUNS)
		{
			G_ProjectSource2 (self->s.origin, self->muzzle2, forward, right, up, start);
			ActorTarget(self,target);
			VectorSubtract (target, start, forward);
			VectorNormalize (forward);
			if ((random() * 3) < 1)
				effect = EF_HYPERBLASTER;
			else
				effect = 0;
			fire_blaster (self, start, forward, damage, 1000, effect, true, color);
		}

		self->actor_gunframe++;
		if (self->actor_gunframe == 12)
			self->actor_gunframe = 6;
	}

	if (self->actor_gunframe == 12)
	{
		gi.sound(self, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);
		self->s.sound = 0;
	}

}

// Railgun
void actorRailGun (edict_t *self)
{
	vec3_t	start, target;
	vec3_t	forward, right, up;

	if(!self->enemy || !self->enemy->inuse)
		return;

	AngleVectors (self->s.angles, forward, right, up);
	if(self->monsterinfo.aiflags & AI_TWO_GUNS)
	{
		if(self->framenumbers % 2)
			G_ProjectSource2 (self->s.origin, self->muzzle2, forward, right, up, start);
		else
			G_ProjectSource2 (self->s.origin, self->muzzle, forward, right, up, start);
		self->framenumbers++;
	}
	else
		G_ProjectSource2 (self->s.origin, self->muzzle, forward, right, up, start);
	ActorTarget(self,target);
	VectorSubtract (target, start, forward);
	VectorNormalize (forward);
	fire_rail (self, start, forward, 80, 100);  // Do slightly less damage

	gi.positioned_sound(start,self,CHAN_WEAPON,gi.soundindex("weapons/railgf1a.wav"),1,ATTN_NORM,0);

	if(developer->value)
	{
		if (!(self->monsterinfo.aiflags & AI_TWO_GUNS) || (self->framenumbers % 2))
			TraceAimPoint(start,target);
	}
}

void actorBFG (edict_t *self)
{
	vec3_t	start, target;
	vec3_t	forward, right, up;

	if(!self->enemy || !self->enemy->inuse) {
		self->monsterinfo.pausetime = 0;
		return;
	}

	if(self->actor_gunframe == 0)
		gi.positioned_sound(self->s.origin,self,CHAN_WEAPON,gi.soundindex("weapons/bfg__f1y.wav"),1,ATTN_NORM,0);

	if(self->actor_gunframe == 10)
	{
		AngleVectors (self->s.angles, forward, right, up);
		if(self->monsterinfo.aiflags & AI_TWO_GUNS)
		{
			if(self->framenumbers % 2)
				G_ProjectSource2 (self->s.origin, self->muzzle2, forward, right, up, start);
			else
				G_ProjectSource2 (self->s.origin, self->muzzle, forward, right, up, start);
			self->framenumbers++;
		}
		else
			G_ProjectSource2 (self->s.origin, self->muzzle, forward, right, up, start);

		ActorTarget(self,target);
		VectorSubtract (target, start, forward);
		VectorNormalize (forward);
		fire_bfg (self, start, forward, 500, 300, 1000);
		self->endtime = level.time + 1;
		if(developer->value)
		{
			if (!(self->monsterinfo.aiflags & AI_TWO_GUNS) || (self->framenumbers % 2))
				TraceAimPoint(start,target);
		}
	}
	self->actor_gunframe++;
}
