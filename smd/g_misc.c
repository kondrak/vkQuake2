// g_misc.c

#include "g_local.h"

int	gibsthisframe=0;
int lastgibframe=0;

#define GIB_METAL   1
#define GIB_GLASS   2
#define GIB_BARREL  3
#define GIB_CRATE   4
#define GIB_ROCK    5
#define GIB_CRYSTAL 6
#define GIB_MECH    7
#define GIB_WOOD    8
#define GIB_TECH    9

void FadeThink(edict_t *ent)
{
	ent->count++;
	if (ent->count==2)
	{
		G_FreeEdict(ent);
		return;
	}
	ent->s.effects |= EF_SPHERETRANS;
	ent->nextthink=level.time+0.5;
	gi.linkentity(ent);
}
void FadeDieThink (edict_t *ent)
{
	ent->s.renderfx   = RF_TRANSLUCENT;
	ent->think        = FadeThink;
	ent->nextthink    = level.time+0.5;
	ent->count        = 0;
	gi.linkentity(ent);
}

/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.
*/

//=====================================================

void Use_Areaportal (edict_t *ent, edict_t *other, edict_t *activator)
{
	ent->count ^= 1;		// toggle state
//	gi.dprintf ("portalstate: %i = %i\n", ent->style, ent->count);
	gi.SetAreaPortalState (ent->style, ent->count);
}

/*QUAKED func_areaportal (0 0 0) ?

This is a non-visible object that divides the world into
areas that are seperated when this portal is not activated.
Usually enclosed in the middle of a door.
*/
void SP_func_areaportal (edict_t *ent)
{
	ent->use = Use_Areaportal;
	ent->count = 0;		// always start closed;
}

//=====================================================


/*
=================
Misc functions
=================
*/
void VelocityForDamage (int damage, vec3_t v)
{
	v[0] = 100.0 * crandom();
	v[1] = 100.0 * crandom();
	v[2] = 200.0 + 100.0 * random();

	if (damage < 50)
		VectorScale (v, 0.7, v);
	else 
		VectorScale (v, 1.2, v);
}

void ClipGibVelocity (edict_t *ent)
{
	if (ent->velocity[0] < -300)
		ent->velocity[0] = -300;
	else if (ent->velocity[0] > 300)
		ent->velocity[0] = 300;
	if (ent->velocity[1] < -300)
		ent->velocity[1] = -300;
	else if (ent->velocity[1] > 300)
		ent->velocity[1] = 300;
	if (ent->velocity[2] < 200)
		ent->velocity[2] = 200;	// always some upwards
	else if (ent->velocity[2] > 500)
		ent->velocity[2] = 500;
}


/*
=================
gibs
=================
*/

//Knightmare- gib fade
#ifdef KMQUAKE2_ENGINE_MOD
void gib_fade2 (edict_t *self);

void gib_fade (edict_t *self)
{
	if (self->s.effects & EF_BLASTER) //Remove glow from gekk gibs
	{
		self->s.effects &= ~EF_BLASTER;
		self->s.renderfx &= ~RF_NOSHADOW;
	}
	if (self->s.renderfx & RF_TRANSLUCENT)
		self->s.alpha = 0.70F;
	else if (self->s.effects & EF_SPHERETRANS)
		self->s.alpha = 0.30F;
	else if (!(self->s.alpha) || self->s.alpha <= 0.0F || self->s.alpha > 1.0F)
		self->s.alpha = 1.00F;
	gib_fade2 (self);
}

void gib_fade2 (edict_t *self)
{
	self->s.alpha -= 0.05F;
	self->s.alpha = max(self->s.alpha, 1/255);
	if (self->s.alpha <= 1/255)
	{
		G_FreeEdict (self);
		return;
	}
	self->nextthink = level.time + 0.2;
	self->think = gib_fade2;
	gi.linkentity (self);
}

#else
void gib_fade2 (edict_t *self);

void gib_fade (edict_t *self)
{
	if (self->s.effects & EF_BLASTER)  //Remove glow from gekk gibs
		self->s.effects &= ~EF_BLASTER;
	self->s.renderfx = RF_TRANSLUCENT;
	self->nextthink = level.time + 2;
	self->think = gib_fade2;
	gi.linkentity(self);
}

void gib_fade2 (edict_t *self)
{
	self->s.effects |= EF_SPHERETRANS;
	self->s.renderfx &= ~RF_TRANSLUCENT;
	self->nextthink = level.time + 2;
	self->think = G_FreeEdict;
	gi.linkentity(self);
}
#endif

void gib_think (edict_t *self)
{
	self->s.frame++;
	self->nextthink = level.time + FRAMETIME;

	if (self->s.frame == 10)
	{
		self->think = G_FreeEdict;
		self->nextthink = level.time + 8 + random()*10;
	}
}

void gib_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t	normal_angles, right;

	if (!self->groundentity)
		return;

	self->touch = NULL;

	if (plane)
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/fhit3.wav"), 1, ATTN_NORM, 0);

		vectoangles (plane->normal, normal_angles);
		AngleVectors (normal_angles, NULL, right, NULL);
		vectoangles (right, self->s.angles);

		if (self->s.modelindex == sm_meat_index)
		{
			self->s.frame++;
			self->think = gib_think;
			self->nextthink = level.time + FRAMETIME;
		}
	}
}

void gib_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	G_FreeEdict (self);
}

void ThrowGib (edict_t *self, char *gibname, int damage, int type)
{
	edict_t *gib;
	vec3_t	vd;
	vec3_t	origin;
	vec3_t	size;
	float	vscale;
	char	modelname[256];
	char	*p;

	// Lazarus: Prevent gib showers (generally due to firing BFG in a crowd) from
	// causing SZ_GetSpace: overflow
	if (level.framenum > lastgibframe)
	{
		gibsthisframe = 0;
		lastgibframe = level.framenum;
	}
	gibsthisframe++;
	if (gibsthisframe > sv_maxgibs->value)
		return;

	gib = G_Spawn();

	gib->classname = gi.TagMalloc (4,TAG_LEVEL);
	strcpy(gib->classname,"gib");

	// Lazarus: mapper-definable gib class
	strcpy(modelname,gibname);
	p = strstr(modelname,"models/objects/gibs/");
	if (p && self->gib_type)
	{
		p += 18;
		p[0] = (char)((int)'0' + (self->gib_type % 10));
	}

	// Save gibname and type for level transition gibs
	gib->key_message = gi.TagMalloc ((int)strlen(modelname)+1,TAG_LEVEL);
	strcpy(gib->key_message, modelname);
	gib->style = type;

	VectorScale (self->size, 0.5, size);
	VectorAdd (self->absmin, size, origin);
	gib->s.origin[0] = origin[0] + crandom() * size[0];
	gib->s.origin[1] = origin[1] + crandom() * size[1];
	gib->s.origin[2] = origin[2] + crandom() * size[2];

	gi.setmodel (gib, modelname);
	gib->solid = SOLID_NOT;
	if (self->blood_type == 1)
	{
		gib->s.effects |= EF_GREENGIB|EF_BLASTER;
		gib->s.renderfx |= RF_NOSHADOW;
	}
	else if (self->blood_type == 2)
		gib->s.effects |= EF_GRENADE;
	else
		gib->s.effects |= EF_GIB;
#ifdef KMQUAKE2_ENGINE_MOD
	//Knightmare- transparent monsters throw transparent gibs
	if ((self->s.alpha) && self->s.alpha > 0.0F)
		gib->s.alpha = self->s.alpha;
#endif
	gib->flags |= FL_NO_KNOCKBACK;
	gib->svflags |= SVF_GIB; //Knightmare- gib flag
	gib->takedamage = DAMAGE_YES;
	gib->die = gib_die;

	if (type == GIB_ORGANIC)
	{
		gib->movetype = MOVETYPE_TOSS;
		gib->touch = gib_touch;
		vscale = 0.5;
	}
	else
	{
		gib->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	VelocityForDamage (damage, vd);
	VectorMA (self->velocity, vscale, vd, gib->velocity);
	ClipGibVelocity (gib);
	gib->avelocity[0] = random()*600;
	gib->avelocity[1] = random()*600;
	gib->avelocity[2] = random()*600;

	gib->think = gib_fade; //Knightmare- gib fade, was FadeDieThink
	gib->nextthink = level.time + 8 + random()*10;

	gib->s.renderfx |= RF_IR_VISIBLE;

	gi.linkentity (gib);
}

// NOTE: SP_gib is ONLY intended to be used for gibs that change maps
//       via trigger_transition. It should NOT be used for map entities

void gib_delayed_start (edict_t *gib)
{
	if (g_edicts[1].linkcount)
	{
		if (gib->count > 0)
		{
			gib->think = gib_fade; // was FadeThink
			gib->nextthink = level.time + FRAMETIME;
		}
		else
		{
			gib->think = gib_fade; // was FadeDieThink
			gib->nextthink = level.time + 8 + random()*10;
		}
	}
	else
	{
		gib->nextthink = level.time + FRAMETIME;
	}
}

void SP_gib (edict_t *gib)
{
	if (gib->key_message)
		gi.setmodel (gib, gib->key_message);
	else
		gi.setmodel (gib, "models/objects/gibs/sm_meat/tris.md2");
	gib->die = gib_die;
	if (gib->style == GIB_ORGANIC)
		gib->touch = gib_touch;
	gib->think = gib_delayed_start;
	gib->nextthink = level.time + FRAMETIME;
	gi.linkentity (gib);
}

void ThrowHead (edict_t *self, char *gibname, int damage, int type)
{
	vec3_t	vd;
	float	vscale;
	char	modelname[256];
	char	*p;

	self->s.skinnum = 0;
	self->s.frame = 0;
	VectorClear (self->mins);
	VectorClear (self->maxs);

	DeleteReflection(self,-1);

	strcpy(modelname,gibname);
	p = strstr(modelname,"models/objects/gibs/");
	if (p && self->gib_type)
	{
		p += 18;
		p[0] = (char)((int)'0' + (self->gib_type % 10));
	}

	// Save gibname and type for level transition gibs
	self->key_message = gi.TagMalloc ((int)strlen(modelname)+1,TAG_LEVEL);
	strcpy(self->key_message, modelname);

	self->style = type;
	self->s.modelindex2 = 0;
	gi.setmodel (self, modelname);
	self->solid = SOLID_NOT;
	if (self->blood_type == 1)
	{
		self->s.effects |= EF_GREENGIB|EF_BLASTER;
		self->s.renderfx |= RF_NOSHADOW;
	}
	else if (self->blood_type == 2)
		self->s.effects |= EF_GRENADE;
	else
		self->s.effects |= EF_GIB;
	self->s.effects &= ~EF_FLIES;
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;
	self->svflags &= ~SVF_MONSTER;
	self->svflags |= SVF_GIB; //Knightmare- gib flag
	self->takedamage = DAMAGE_YES;
	// Lazarus: Disassociate this head with its monster
	self->targetname = NULL;
	self->die = gib_die;
	self->dmgteam = NULL; // Prevent gibs from becoming angry if their buddies are hurt

	if (type == GIB_ORGANIC)
	{
		self->movetype = MOVETYPE_TOSS;
		self->touch = gib_touch;
		vscale = 0.5;
	}
	else
	{
		self->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	VelocityForDamage (damage, vd);
	VectorMA (self->velocity, vscale, vd, self->velocity);
	ClipGibVelocity (self);

	self->avelocity[YAW] = crandom()*600;

	self->think = gib_fade; //Knightmare- gib fade, was G_FreeEdict
	self->nextthink = level.time + 8 + random()*10;

	// Lazarus: If head owner was part of a movewith chain,
	//          remove from the chain and repair the chain
	//          if necessary
	if (self->movewith) {
		edict_t	*e;
		edict_t	*parent=NULL;
		int		i;
		for(i=1; i<globals.num_edicts && !parent; i++) {
			e = g_edicts + i;
			if (e->movewith_next == self) parent=e;
		}
		if (parent) parent->movewith_next = self->movewith_next;
	}

	self->s.renderfx |= RF_IR_VISIBLE;

	gi.linkentity (self);
}

void SP_gibhead (edict_t *gib)
{
	if (gib->key_message)
		gi.setmodel (gib, gib->key_message);
	else
		gi.setmodel (gib, "models/objects/gibs/head2/tris.md2");

	if (gib->style == GIB_ORGANIC)
		gib->touch = gib_touch;

	gib->think = gib_delayed_start;
	gib->nextthink = level.time + FRAMETIME;
	gi.linkentity (gib);

}

void ThrowClientHead (edict_t *self, int damage)
{
	vec3_t	vd;
	char	*gibname;

	if (rand()&1)
	{
		gibname = "models/objects/gibs/head2/tris.md2";
		self->s.skinnum = 1;		// second skin is player
	}
	else
	{
		gibname = "models/objects/gibs/skull/tris.md2";
		self->s.skinnum = 0;
	}

	self->s.origin[2] += 32;
	self->s.frame = 0;
	gi.setmodel (self, gibname);
	VectorSet (self->mins, -16, -16, 0);
	VectorSet (self->maxs, 16, 16, 16);

	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->s.effects = EF_GIB;
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;
	self->svflags |= SVF_GIB; //Knightmare- gib flag

	self->movetype = MOVETYPE_BOUNCE;
	VelocityForDamage (damage, vd);
	VectorAdd (self->velocity, vd, self->velocity);

	if (self->client)	// bodies in the queue don't have a client anymore
	{
		self->client->anim_priority = ANIM_DEATH;
		self->client->anim_end = self->s.frame;
	}
	else
	{
		self->think = NULL;
		self->nextthink = 0;
	}

	gi.linkentity (self);
}


/*
=================
debris
=================
*/
void debris_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	G_FreeEdict (self);
}

void ThrowDebris (edict_t *self, char *modelname, float speed, vec3_t origin, int skin, int effects)
{
	edict_t	*chunk;
	vec3_t	v;

	chunk = G_Spawn();
	VectorCopy (origin, chunk->s.origin);
	gi.setmodel (chunk, modelname);
#ifdef KMQUAKE2_ENGINE_MOD
	//Knightmare- transparent entities throw transparent debris
	if ((self->s.alpha) && self->s.alpha > 0.0F && self->s.alpha < 1.0F)
		chunk->s.alpha = self->s.alpha;
#endif
	v[0] = 100 * crandom();
	v[1] = 100 * crandom();
	v[2] = 100 + 100 * crandom();
	VectorMA (self->velocity, speed, v, chunk->velocity);
	chunk->movetype = MOVETYPE_BOUNCE;
	chunk->solid = SOLID_NOT;
	chunk->avelocity[0] = random()*600;
	chunk->avelocity[1] = random()*600;
	chunk->avelocity[2] = random()*600;
	chunk->think = gib_fade; //Knightmare- gib fade, was G_FreeEdict
	chunk->nextthink = level.time + 5 + random()*5;
	chunk->s.frame = 0;
	chunk->flags = 0;
	chunk->classname = "debris";
	chunk->takedamage = DAMAGE_YES;
	chunk->die = debris_die;

	// Lazarus: Preserve model name for level changes:
	chunk->message = gi.TagMalloc ((int)strlen(modelname)+1,TAG_LEVEL);
	strcpy(chunk->message, modelname);

	// Lazarus: skin number and effects
	chunk->s.skinnum = skin;
	chunk->s.effects |= effects;

	gi.linkentity (chunk);
}

// NOTE: SP_debris is ONLY intended to be used for debris chunks that change maps
//       via trigger_transition. It should NOT be used for map entities

void debris_delayed_start (edict_t *debris)
{
	if (g_edicts[1].linkcount)
	{
		debris->think = G_FreeEdict;
		debris->nextthink = level.time + 5 + random()*5;
	}
	else
	{
		debris->nextthink = level.time + FRAMETIME;
	}
}

void SP_debris (edict_t *debris)
{
	if (debris->message)
		gi.setmodel (debris, debris->message);
	else
		gi.setmodel (debris, "models/objects/debris2/tris.md2");
	debris->think = debris_delayed_start;
	debris->nextthink = level.time + FRAMETIME;
	debris->die = debris_die;
	gi.linkentity (debris);
}

void BecomeExplosion1 (edict_t *self)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	if (level.num_reflectors)
		ReflectExplosion (TE_EXPLOSION1, self->s.origin);

	G_FreeEdict (self);
}


void BecomeExplosion2 (edict_t *self)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION2);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	if (level.num_reflectors)
		ReflectExplosion (TE_EXPLOSION2, self->s.origin);

	G_FreeEdict (self);
}


/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8) TELEPORT
Target: next path corner
Pathtarget: gets used when an entity that has
	this path_corner targeted touches it
*/

void path_corner_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t		v;
	edict_t		*next;

	if (other->movetarget != self)
		return;
	
	if (other->enemy)
		return;

	if (self->pathtarget)
	{
		char *savetarget;
		savetarget = self->target;
		self->target = self->pathtarget;
		G_UseTargets(self, other);
		self->target = savetarget;
	}

	if (self->target)
		next = G_PickTarget(self->target);
	else
		next = NULL;

	// Lazarus: Distinguish between path_corner and target_actor, or target_actor
	//          with JUMP spawnflag will result in teleport. Ack!
	if ((next) && (next->spawnflags & 1) && !Q_stricmp(next->classname,"path_corner"))
	{
		VectorCopy (next->s.origin, v);
		v[2] += next->mins[2];
		v[2] -= other->mins[2];
		VectorCopy (v, other->s.origin);
		next = G_PickTarget(next->target);
		other->s.event = EV_OTHER_TELEPORT;
	}

	other->goalentity = other->movetarget = next;

	if (self->wait > 0)
	{
		other->monsterinfo.pausetime = level.time + self->wait;
		other->monsterinfo.stand (other);
	}
	else if (self->wait < 0)
	{
		other->monsterinfo.pausetime = level.time + 100000000;
		other->monsterinfo.stand (other);
	}
	else
	{
		
		if (!other->movetarget)
		{
			other->monsterinfo.pausetime = level.time + 100000000;
			other->monsterinfo.stand (other);
		}
		else
		{
			VectorSubtract (other->goalentity->s.origin, other->s.origin, v);
			other->ideal_yaw = vectoyaw (v);
		}
	}

	self->count--;
	if (!self->count)
	{
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}

}

void SP_path_corner (edict_t *self)
{
	if (!self->targetname)
	{
		gi.dprintf ("path_corner with no targetname at %s\n", vtos(self->s.origin));
		G_FreeEdict (self);
		return;
	}

	self->solid = SOLID_TRIGGER;
	self->touch = path_corner_touch;
	VectorSet (self->mins, -8, -8, -8);
	VectorSet (self->maxs, 8, 8, 8);
	self->svflags |= SVF_NOCLIENT;
	if (!self->count) self->count = -1;

	gi.linkentity (self);
}


/*QUAKED point_combat (0.5 0.3 0) (-8 -8 -8) (8 8 8) Hold
Makes this the target of a monster and it will head here
when first activated before going after the activator.  If
hold is selected, it will stay here.
*/
void tracktrain_drive(edict_t *train, edict_t *other);
void point_combat_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t	*activator;

	if (other->movetarget != self)
		return;

	self->count--;
	if (!self->count) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + self->delay + 1;
	}

	if (self->target)
	{
		other->target = self->target;
		other->goalentity = other->movetarget = G_PickTarget(other->target);
		if (!other->goalentity)
		{
			gi.dprintf("%s at %s target %s does not exist\n", self->classname, vtos(self->s.origin), self->target);
			other->movetarget = self;
		}
		// Lazarus: NO NO NO! This makes the pc's target accessible once and only once
		//self->target = NULL;
	}
	else if ((self->spawnflags & 1) && !(other->flags & (FL_SWIM|FL_FLY)))
	{
		other->monsterinfo.pausetime = level.time + 100000000;
		other->monsterinfo.aiflags |= AI_STAND_GROUND;
		other->monsterinfo.stand (other);
	}

	if (other->movetarget == self)
	{
		other->target = NULL;
		other->movetarget = NULL;
		other->goalentity = other->enemy;
		other->monsterinfo.aiflags &= ~AI_COMBAT_POINT;
	}

	if (self->pathtarget)
	{
		if (self->spawnflags & 2)
		{
			edict_t	*train;
			train = G_PickTarget(self->pathtarget);
			if (train)
				tracktrain_drive(train,other);
		}
		else
		{
			char *savetarget;
			
			savetarget = self->target;
			self->target = self->pathtarget;
			if (other->enemy && other->enemy->client)
				activator = other->enemy;
			else if (other->oldenemy && other->oldenemy->client)
				activator = other->oldenemy;
			else if (other->activator && other->activator->client)
				activator = other->activator;
			else
				activator = other;
			G_UseTargets (self, activator);
			self->target = savetarget;
		}
	}
}

void SP_point_combat (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}
	self->solid = SOLID_TRIGGER;
	self->touch = point_combat_touch;
	VectorSet (self->mins, -8, -8, -16);
	VectorSet (self->maxs, 8, 8, 16);
	self->svflags = SVF_NOCLIENT;

	// Point_combat usage count is handled a bit differently than other
	// entities. Since the standard Q2 point_combat can only be used once,
	// the default behavior should be to delete the point_combat after
	// one use.
	if (!self->count) self->count = 1;
	gi.linkentity (self);
};


/*QUAKED viewthing (0 .5 .8) (-8 -8 -8) (8 8 8)
Just for the debugging level.  Don't use
*/
void TH_viewthing(edict_t *ent)
{
	ent->s.frame = (ent->s.frame + 1) % 7;
	ent->nextthink = level.time + FRAMETIME;
}

void SP_viewthing(edict_t *ent)
{
	gi.dprintf ("viewthing spawned\n");

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->s.renderfx = RF_FRAMELERP;
	VectorSet (ent->mins, -16, -16, -24);
	VectorSet (ent->maxs, 16, 16, 32);
	ent->s.modelindex = gi.modelindex ("models/objects/banner/tris.md2");
	gi.linkentity (ent);
	ent->nextthink = level.time + 0.5;
	ent->think = TH_viewthing;
	return;
}


/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for spotlights, etc.
*/
void SP_info_null (edict_t *self)
{
	G_FreeEdict (self);
};


/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for lightning.
*/
void SP_info_notnull (edict_t *self)
{
	VectorCopy (self->s.origin, self->absmin);
	VectorCopy (self->s.origin, self->absmax);
};


/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Non-displayed light.
Default light value is 300.
Default style is 0.
If targeted, will toggle between on and off.
Default _cone value is 10 (used to set size of light for spotlights)
*/

#define START_OFF	1

/*static*/ void light_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->spawnflags & START_OFF)
	{
		gi.configstring (CS_LIGHTS+self->style, "m");
		self->spawnflags &= ~START_OFF;
	}
	else
	{
		gi.configstring (CS_LIGHTS+self->style, "a");
		self->spawnflags |= START_OFF;

		self->count--;
		if (!self->count) {
			self->think = G_FreeEdict;
			self->nextthink = level.time + 1;
		}
	}

}

void SP_light (edict_t *self)
{
	// no targeted lights in deathmatch, because they cause global messages
	if (!self->targetname || deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	if (self->style >= 32)
	{
		self->use = light_use;
		if (self->spawnflags & START_OFF)
			gi.configstring (CS_LIGHTS+self->style, "a");
		else
			gi.configstring (CS_LIGHTS+self->style, "m");
	}
}


/*QUAKED func_wall (0 .5 .8) ? TRIGGER_SPAWN TOGGLE START_ON ANIMATED ANIMATED_FAST
This is just a solid wall if not inhibited

TRIGGER_SPAWN	the wall will not be present until triggered
				it will then blink in to existance; it will
				kill anything that was in it's way

TOGGLE			only valid for TRIGGER_SPAWN walls
				this allows the wall to be turned on and off

START_ON		only valid for TRIGGER_SPAWN walls
				the wall will initially be present
*/

void func_wall_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->solid == SOLID_NOT)
	{
		self->solid = SOLID_BSP;
		self->svflags &= ~SVF_NOCLIENT;
		KillBox (self);
	}
	else
	{
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;

		self->count--;
		if (!self->count) {
			self->think = G_FreeEdict;
			self->nextthink = level.time + 1;
			return;
		}
	}
	gi.linkentity (self);

	if (!(self->spawnflags & 2))
		self->use = NULL;
}

void SP_func_wall (edict_t *self)
{
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel (self, self->model);

	if (self->spawnflags & 8)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & 16)
		self->s.effects |= EF_ANIM_ALLFAST;

	// just a wall
	if ((self->spawnflags & 7) == 0)
	{
		self->solid = SOLID_BSP;
		gi.linkentity (self);
		return;
	}

	// it must be TRIGGER_SPAWN
	if (!(self->spawnflags & 1))
	{
//		gi.dprintf("func_wall missing TRIGGER_SPAWN\n");
		self->spawnflags |= 1;
	}

	// yell if the spawnflags are odd
	if (self->spawnflags & 4)
	{
		if (!(self->spawnflags & 2))
		{
			gi.dprintf("func_wall START_ON without TOGGLE\n");
			self->spawnflags |= 2;
		}
	}

	self->use = func_wall_use;
	if (self->spawnflags & 4)
	{
		self->solid = SOLID_BSP;
	}
	else
	{
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
	}
	gi.linkentity (self);
}


/*QUAKED func_object (0 .5 .8) ? TRIGGER_SPAWN ANIMATED ANIMATED_FAST
This is solid bmodel that will fall if it's support it removed.
*/

void func_object_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	// only squash thing we fall on top of
	if (!plane)
		return;
	if (plane->normal[2] < 1.0)
		return;
	if (other->takedamage == DAMAGE_NO)
		return;
	T_Damage (other, self, self, vec3_origin, self->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}

void func_object_release (edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->touch = func_object_touch;
}

void func_object_use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->solid = SOLID_BSP;
	self->svflags &= ~SVF_NOCLIENT;
	self->use = NULL;
	KillBox (self);
	func_object_release (self);
}

void SP_func_object (edict_t *self)
{
	gi.setmodel (self, self->model);

	self->mins[0] += 1;
	self->mins[1] += 1;
	self->mins[2] += 1;
	self->maxs[0] -= 1;
	self->maxs[1] -= 1;
	self->maxs[2] -= 1;

	if (!self->dmg)
		self->dmg = 100;

	if (self->spawnflags == 0)
	{
		self->solid = SOLID_BSP;
		self->movetype = MOVETYPE_PUSH;
		self->think = func_object_release;
		self->nextthink = level.time + 2 * FRAMETIME;
	}
	else
	{
		self->solid = SOLID_NOT;
		self->movetype = MOVETYPE_PUSH;
		self->use = func_object_use;
		self->svflags |= SVF_NOCLIENT;
	}

	if (self->spawnflags & 2)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & 4)
		self->s.effects |= EF_ANIM_ALLFAST;

	self->clipmask = MASK_MONSTERSOLID;

	gi.linkentity (self);
}


/*QUAKED func_explosive (0 .5 .8) ? Trigger_Spawn ANIMATED ANIMATED_FAST
Any brush that you want to explode or break apart.  If you want an
ex0plosion, set dmg and it will do a radius explosion of that amount
at the center of the bursh.

If targeted it will not be shootable.

health defaults to 100.

mass defaults to 75.  This determines how much debris is emitted when
it explodes.  You get one large chunk per 100 of mass (up to 8) and
one small chunk per 25 of mass (up to 16).  So 800 gives the most.

Lazarus:
SF 8 EXPLOSIVES_ONLY  Func_explosive may ONLY be damaged by explosions

*/
void func_explosive_use(edict_t *self, edict_t *other, edict_t *activator);
void func_explosive_respawn(edict_t *self)
{
    KillBox(self);
    self->solid = SOLID_BSP;
    self->svflags &= ~SVF_NOCLIENT;
    self->health = self->max_health;
    self->takedamage = DAMAGE_YES;
	self->use = func_explosive_use;
    gi.linkentity(self);
}

void func_explosive_explode (edict_t *self)
{
	vec3_t	origin;
	vec3_t	chunkorigin;
	vec3_t	size;
	int		count;
	int		mass;

	// bmodel origins are (0 0 0), we need to adjust that here
	VectorScale (self->size, 0.5, size);
	VectorAdd (self->absmin, size, origin);
	VectorCopy (origin, self->s.origin);

	self->takedamage = DAMAGE_NO;

	if (self->dmg)
		T_RadiusDamage (self, self->activator, self->dmg, NULL, self->dmg+40, MOD_EXPLOSIVE, -0.5);

	VectorSubtract (self->s.origin, self->enemy->s.origin, self->velocity);
	VectorNormalize (self->velocity);
	VectorScale (self->velocity, 150, self->velocity);

	// start chunks towards the center
	VectorScale (size, 0.5, size);

	mass = self->mass;
	if (!mass)
		mass = 75;

	// Lazarus: Use traditional debris for gib_type=0, but non-zero gib_type gives equal 
	// weight to all models.

	if ( (self->gib_type > 0) && (self->gib_type < 10))
	{
		int	r;

		count = mass/25;
		if (count > 16)
			count = 16;
		while(count--)
		{
			r = (rand() % 5) + 1;
			chunkorigin[0] = origin[0] + crandom() * size[0];
			chunkorigin[1] = origin[1] + crandom() * size[1];
			chunkorigin[2] = origin[2] + crandom() * size[2];
			switch(self->gib_type) {
			case GIB_METAL:
				ThrowDebris (self, va("models/objects/metal_gibs/gib%i.md2",r),   2, chunkorigin, self->s.skinnum, 0); break;
			case GIB_GLASS:
				ThrowDebris (self, va("models/objects/glass_gibs/gib%i.md2",r),   2, chunkorigin, self->s.skinnum, EF_SPHERETRANS); break;
			case GIB_BARREL:
				ThrowDebris (self, va("models/objects/barrel_gibs/gib%i.md2",r),  2, chunkorigin, self->s.skinnum, 0); break;
			case GIB_CRATE:
				ThrowDebris (self, va("models/objects/crate_gibs/gib%i.md2",r),   2, chunkorigin, self->s.skinnum, 0); break;
			case GIB_ROCK:
				ThrowDebris (self, va("models/objects/rock_gibs/gib%i.md2",r),    2, chunkorigin, self->s.skinnum, 0); break;
			case GIB_CRYSTAL:
				ThrowDebris (self, va("models/objects/crystal_gibs/gib%i.md2",r), 2, chunkorigin, self->s.skinnum, 0); break;
			case GIB_MECH:
				ThrowDebris (self, va("models/objects/mech_gibs/gib%i.md2",r),    2, chunkorigin, self->s.skinnum, 0); break;
			case GIB_WOOD:
				ThrowDebris (self, va("models/objects/wood_gibs/gib%i.md2",r),    2, chunkorigin, self->s.skinnum, 0); break;
			case GIB_TECH:
				ThrowDebris (self, va("models/objects/tech_gibs/gib%i.md2",r),    2, chunkorigin, self->s.skinnum, 0); break;
			}
		}
	}
	else
	{
		// big chunks
		if (mass >= 100)
		{
			count = mass / 100;
			if (count > 8)
				count = 8;
			while(count--)
			{
				chunkorigin[0] = origin[0] + crandom() * size[0];
				chunkorigin[1] = origin[1] + crandom() * size[1];
				chunkorigin[2] = origin[2] + crandom() * size[2];
				ThrowDebris (self, "models/objects/debris1/tris.md2", 1, chunkorigin, 0, 0);
			}
		}
		
		// small chunks
		count = mass / 25;
		if (count > 16)
			count = 16;
		while(count--)
		{
			chunkorigin[0] = origin[0] + crandom() * size[0];
			chunkorigin[1] = origin[1] + crandom() * size[1];
			chunkorigin[2] = origin[2] + crandom() * size[2];
			ThrowDebris (self, "models/objects/debris2/tris.md2", 2, chunkorigin, 0, 0);
		}
	}

	G_UseTargets (self, self->activator);

// Respawnable func_explosive. If desired, replace these lines....

	if (self->dmg)
		BecomeExplosion1 (self);
	else
		G_FreeEdict (self);

/*  with these:
    if (self->dmg)
    {
        gi.WriteByte (svc_temp_entity);
        gi.WriteByte (TE_EXPLOSION1);
        gi.WritePosition (self->s.origin);
        gi.multicast (self->s.origin, MULTICAST_PVS);
    }
	VectorClear(self->s.origin);
	VectorClear(self->velocity);
    self->solid = SOLID_NOT;
    self->svflags |= SVF_NOCLIENT;
    self->think = func_explosive_respawn;
    self->nextthink = level.time + 10; // substitute whatever value you want here
	self->use = NULL;
	gi.linkentity(self); */
}

void func_explosive_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->enemy = inflictor;
	self->activator = attacker;
	if (self->delay > 0)
	{
		self->think = func_explosive_explode;
		self->nextthink = level.time + self->delay;
	}
	else
		func_explosive_explode(self);
}

void func_explosive_use(edict_t *self, edict_t *other, edict_t *activator)
{
	func_explosive_die (self, self, other, self->health, vec3_origin);
}

void func_explosive_spawn (edict_t *self, edict_t *other, edict_t *activator)
{
	self->solid = SOLID_BSP;
	self->svflags &= ~SVF_NOCLIENT;
	self->use = NULL;
	KillBox (self);
	gi.linkentity (self);
}

// Lazarus func_explosive can be damaged by impact
void func_explosive_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int		damage;
	float	delta;
	float	mass;
	vec3_t	dir, impact_v;

	if (!self->health) return;
	if (other->mass <= 200) return;
	if (VectorLength(other->velocity)==0) return;
	// Check for impact damage
	if (self->health > 0)
	{
		VectorSubtract(other->velocity,self->velocity,impact_v);
		delta = VectorLength(impact_v);
		delta = delta*delta*0.0001;
		mass = self->mass;
		if (!mass) mass = 200;
		delta *= (float)(other->mass)/mass;
		if (delta > 30)
		{
			damage = (delta-30)/2;
			if (damage > 0)
			{
				VectorSubtract(self->s.origin,other->s.origin,dir);
				VectorNormalize(dir);
				T_Damage (self, other, other, dir, self->s.origin, vec3_origin, damage, 0, 0, MOD_FALLING);
			}
		}
	}
}

void SP_func_explosive (edict_t *self)
{
	if (deathmatch->value)
	{	// auto-remove for deathmatch
		G_FreeEdict (self);
		return;
	}

	self->movetype = MOVETYPE_PUSH;

	PrecacheDebris(self->gib_type);

	gi.setmodel (self, self->model);

	if (self->spawnflags & 1)
	{
		self->svflags |= SVF_NOCLIENT;
		self->solid = SOLID_NOT;
		self->use = func_explosive_spawn;
	}
	else
	{
		self->solid = SOLID_BSP;
		if (self->targetname)
			self->use = func_explosive_use;
	}

	if (self->spawnflags & 2)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & 4)
		self->s.effects |= EF_ANIM_ALLFAST;

	if (self->use != func_explosive_use)
	{
		if (!self->health)
			self->health = 100;
		self->die = func_explosive_die;
		self->takedamage = DAMAGE_YES;
	}

	// Lazarus: added touch function for impact damage
	self->touch = func_explosive_touch;

	// Lazarus: for respawning func_explosives
	self->max_health = self->health;

	gi.linkentity (self);
}


void barrel_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)

{
	float	ratio;
	vec3_t	v;

	if ((!other->groundentity) || (other->groundentity == self))
		return;

	ratio = (float)other->mass / (float)self->mass;
	VectorSubtract (self->s.origin, other->s.origin, v);
	M_walkmove (self, vectoyaw(v), 20 * ratio * FRAMETIME);
}

void barrel_explode (edict_t *self)
{
	vec3_t	org;
	float	spd;
	vec3_t	save;
	vec3_t	size;
	int		i;

	/*if (self->gib_type == GIB_BARREL)
	{
		func_explosive_explode(self);
		return;
	}*/

	VectorCopy (self->s.origin, save);
	VectorMA (self->absmin, 0.5, self->size, self->s.origin);

	T_RadiusDamage (self, self->activator, self->dmg, NULL, self->dmg+40, MOD_BARREL, -0.5);

	// Lazarus: Nice catch by Argh! For years now, explobox chunks have
	//          gone through the floor. This is very noticeable if the floor
	//          is transparent. Like func_explosive, shrink size box so that
	//          debris has an initial standoff from the floor.
	VectorScale(self->size,0.5,size);

	// Knightmare- new gib code
	if (self->gib_type && self->gib_type == GIB_BARREL)
	{
		// top
		spd = 1.5 * (float)self->dmg / 200.0;
		VectorCopy (self->s.origin, org);
		org[2] = self->absmax[2];
		ThrowDebris (self, "models/objects/barrel_gibs/gib2.md2", spd, org, 0, 0);
		
		// side pieces
		for (i = 0; i < 8; i++)
		{
			org[0] = self->s.origin[0] + crandom() * size[0];
			org[1] = self->s.origin[1] + crandom() * size[1];
			org[2] = self->s.origin[2] + crandom() * size[2];
			ThrowDebris (self, "models/objects/barrel_gibs/gib4.md2",  spd, org, 0, 0);
		}

		// bottom corners
		spd = 1.75 * (float)self->dmg / 200.0;
		VectorCopy (self->absmin, org);
		org[1] += self->size[1];
		ThrowDebris (self, "models/objects/barrel_gibs/gib1.md2",  spd, org, 0, 0);
		org[0] += self->size[0] * 2;
		ThrowDebris (self, "models/objects/barrel_gibs/gib1.md2",  spd, org, 0, 0);

		// top corners
		VectorCopy (self->absmax, org);
		org[1] += self->size[1];
		ThrowDebris (self, "models/objects/barrel_gibs/gib3.md2",  spd, org, 0, 0);
		org[0] += self->size[0] * 2;
		ThrowDebris (self, "models/objects/barrel_gibs/gib3.md2",  spd, org, 0, 0);

		// a bunch of little chunks
		spd = 2 * self->dmg / 200;
		for (i = 0; i < 8; i++)
		{
			org[0] = self->s.origin[0] + crandom() * size[0];
			org[1] = self->s.origin[1] + crandom() * size[1];
			org[2] = self->s.origin[2] + crandom() * size[2];
			ThrowDebris (self, "models/objects/barrel_gibs/gib5.md2",  spd, org, 0, 0);
		}
	}
	else
	{
		// a few big chunks
		spd = 1.5 * (float)self->dmg / 200.0;
		org[0] = self->s.origin[0] + crandom() * size[0];
		org[1] = self->s.origin[1] + crandom() * size[1];
		org[2] = self->s.origin[2] + crandom() * size[2];
		ThrowDebris (self, "models/objects/debris1/tris.md2", spd, org, 0, 0);
		org[0] = self->s.origin[0] + crandom() * size[0];
		org[1] = self->s.origin[1] + crandom() * size[1];
		org[2] = self->s.origin[2] + crandom() * size[2];
		ThrowDebris (self, "models/objects/debris1/tris.md2", spd, org, 0, 0);

		// bottom corners
		spd = 1.75 * (float)self->dmg / 200.0;
		VectorCopy (self->absmin, org);
		org[2] += 2;
		ThrowDebris (self, "models/objects/debris3/tris.md2", spd, org, 0, 0);
		VectorCopy (self->absmin, org);
		org[0] += self->size[0];
		org[2] += 2;
		ThrowDebris (self, "models/objects/debris3/tris.md2", spd, org, 0, 0);
		VectorCopy (self->absmin, org);
		org[1] += self->size[1];
		org[2] += 2;
		ThrowDebris (self, "models/objects/debris3/tris.md2", spd, org, 0, 0);
		VectorCopy (self->absmin, org);
		org[0] += self->size[0];
		org[1] += self->size[1];
		org[2] += 2;
		ThrowDebris (self, "models/objects/debris3/tris.md2", spd, org, 0, 0);

		// a bunch of little chunks
		spd = 2 * self->dmg / 200;
		org[0] = self->s.origin[0] + crandom() * size[0];
		org[1] = self->s.origin[1] + crandom() * size[1];
		org[2] = self->s.origin[2] + crandom() * size[2];
		ThrowDebris (self, "models/objects/debris2/tris.md2", spd, org, 0, 0);
		org[0] = self->s.origin[0] + crandom() * size[0];
		org[1] = self->s.origin[1] + crandom() * size[1];
		org[2] = self->s.origin[2] + crandom() * size[2];
		ThrowDebris (self, "models/objects/debris2/tris.md2", spd, org, 0, 0);
		org[0] = self->s.origin[0] + crandom() * size[0];
		org[1] = self->s.origin[1] + crandom() * size[1];
		org[2] = self->s.origin[2] + crandom() * size[2];
		ThrowDebris (self, "models/objects/debris2/tris.md2", spd, org, 0, 0);
		org[0] = self->s.origin[0] + crandom() * size[0];
		org[1] = self->s.origin[1] + crandom() * size[1];
		org[2] = self->s.origin[2] + crandom() * size[2];
		ThrowDebris (self, "models/objects/debris2/tris.md2", spd, org, 0, 0);
		org[0] = self->s.origin[0] + crandom() * size[0];
		org[1] = self->s.origin[1] + crandom() * size[1];
		org[2] = self->s.origin[2] + crandom() * size[2];
		ThrowDebris (self, "models/objects/debris2/tris.md2", spd, org, 0, 0);
		org[0] = self->s.origin[0] + crandom() * size[0];
		org[1] = self->s.origin[1] + crandom() * size[1];
		org[2] = self->s.origin[2] + crandom() * size[2];
		ThrowDebris (self, "models/objects/debris2/tris.md2", spd, org, 0, 0);
		org[0] = self->s.origin[0] + crandom() * size[0];
		org[1] = self->s.origin[1] + crandom() * size[1];
		org[2] = self->s.origin[2] + crandom() * size[2];
		ThrowDebris (self, "models/objects/debris2/tris.md2", spd, org, 0, 0);
		org[0] = self->s.origin[0] + crandom() * size[0];
		org[1] = self->s.origin[1] + crandom() * size[1];
		org[2] = self->s.origin[2] + crandom() * size[2];
		ThrowDebris (self, "models/objects/debris2/tris.md2", spd, org, 0, 0);
	}

	// Lazarus: use targets
	G_UseTargets (self, self->activator);

	// Lazarus: added case for no damage
	if (self->dmg) {
		if (self->groundentity) {
			self->s.origin[2] = self->absmin[2] + 2;
			BecomeExplosion2 (self);
		} else
			BecomeExplosion1 (self);
	} else {
		gi.sound (self, CHAN_VOICE, gi.soundindex ("tank/thud.wav"), 1, ATTN_NORM, 0);
		G_FreeEdict (self);
	}
}

void barrel_delay (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->takedamage = DAMAGE_NO;
	self->nextthink = level.time + 2 * FRAMETIME;
	self->think = barrel_explode;
	self->activator = attacker;
}

void SP_misc_explobox (edict_t *self)
{
	int	i;

	if (deathmatch->value)
	{	// auto-remove for deathmatch
		G_FreeEdict (self);
		return;
	}

	// Knightmare- barrel-specific gibs, since we have the higher limit
#ifdef KMQUAKE2_ENGINE_MOD
	self->gib_type = GIB_BARREL;
	for (i=1; i<=5; i++)
		gi.modelindex(va("models/objects/barrel_gibs/gib%i.md2",i));
#else
	// Lazarus: can use actual barrel parts for debris:
	if ((self->spawnflags & 1) || (self->gib_type == GIB_BARREL))
	{
		self->gib_type = GIB_BARREL;
		for (i=1; i<=5; i++)
			gi.modelindex(va("models/objects/barrel_gibs/gib%i.md2",i));
	}
	else
	{
		gi.modelindex ("models/objects/debris1/tris.md2");
		gi.modelindex ("models/objects/debris2/tris.md2");
		gi.modelindex ("models/objects/debris3/tris.md2");
	}
#endif

	self->solid = SOLID_BBOX;
	self->clipmask = MASK_MONSTERSOLID | MASK_PLAYERSOLID;
	self->movetype = MOVETYPE_STEP;

	self->model = "models/objects/barrels/tris.md2";
	self->s.modelindex = gi.modelindex (self->model);
	VectorSet (self->mins, -16, -16, 0);
	VectorSet (self->maxs, 16, 16, 40);

	if (!self->mass)
		self->mass = 400;
	if (!self->health)
		self->health = 10;
	if (!self->dmg)
		self->dmg = 150;

	self->die = barrel_delay;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.aiflags = AI_NOSTEP;

	self->touch = barrel_touch;

	// Lazarus: this isn't necessary, as MOVETYPE_STEP is already
	// effected by gravity anyway. Adding this in has the effect
	// of doubling gravity, which is OK for normally placed
	// misc_explobox but sucks if it is spawned from above the floor
	// by a target_spawner or target_clone.
	//self->think = M_droptofloor;
	//self->nextthink = level.time + 2 * FRAMETIME;

	self->common_name = "Exploding Box";
	gi.linkentity (self);
}


//
// miscellaneous specialty items
//

/*QUAKED misc_blackhole (1 .5 0) (-8 -8 -8) (8 8 8)
*/

void misc_blackhole_use (edict_t *ent, edict_t *other, edict_t *activator)
{
	/*
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BOSSTPORT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);
	*/
	G_FreeEdict (ent);
}

void misc_blackhole_think (edict_t *self)
{
	if (++self->s.frame < 19)
		self->nextthink = level.time + FRAMETIME;
	else
	{		
		self->s.frame = 0;
		self->nextthink = level.time + FRAMETIME;
	}
}

void misc_blackhole_transparent (edict_t *ent)
{
	// Lazarus: This avoids the problem mentioned below.
	ent->s.renderfx = RF_TRANSLUCENT;
	ent->prethink = NULL;
	gi.linkentity(ent);
}

void SP_misc_blackhole (edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	VectorSet (ent->mins, -64, -64, 0);
	VectorSet (ent->maxs, 64, 64, 8);
	ent->s.modelindex = gi.modelindex ("models/objects/black/tris.md2");
//	ent->s.renderfx = RF_TRANSLUCENT;     // Lazarus: For some oddball reason if this is set
	                                      //          here, blackhole generator will be 
	                                      //          invisible. Rogue MP has the same problem.
	ent->use = misc_blackhole_use;
	ent->think = misc_blackhole_think;
	ent->prethink = misc_blackhole_transparent;
	ent->nextthink = level.time + 2 * FRAMETIME;
	gi.linkentity (ent);
}

/*QUAKED misc_eastertank (1 .5 0) (-32 -32 -16) (32 32 32)
*/

void misc_eastertank_think (edict_t *self)
{
	if (++self->s.frame < 293)
		self->nextthink = level.time + FRAMETIME;
	else
	{		
		self->s.frame = 254;
		self->nextthink = level.time + FRAMETIME;
	}
}

void SP_misc_eastertank (edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	// Lazarus - nonsolid
	if (ent->spawnflags & 1)
		ent->solid = SOLID_NOT;
	else
	{
		ent->solid = SOLID_BBOX;
		VectorSet (ent->mins, -32, -32, -16);
		VectorSet (ent->maxs, 32, 32, 32);
	}
	ent->s.modelindex = gi.modelindex ("models/monsters/tank/tris.md2");
	ent->s.frame = 254;
	ent->think = misc_eastertank_think;
	ent->nextthink = level.time + 2 * FRAMETIME;
	gi.linkentity (ent);
}

/*QUAKED misc_easterchick (1 .5 0) (-32 -32 0) (32 32 32)
*/


void misc_easterchick_think (edict_t *self)
{
	if (++self->s.frame < 247)
		self->nextthink = level.time + FRAMETIME;
	else
	{		
		self->s.frame = 208;
		self->nextthink = level.time + FRAMETIME;
	}
}

void SP_misc_easterchick (edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	// Lazarus - nonsolid
	if (ent->spawnflags & 1)
		ent->solid = SOLID_NOT;
	else
	{
		ent->solid = SOLID_BBOX;
		VectorSet (ent->mins, -32, -32, 0);
		VectorSet (ent->maxs, 32, 32, 32);
	}
	ent->s.modelindex = gi.modelindex ("models/monsters/bitch/tris.md2");
	ent->s.frame = 208;
	ent->think = misc_easterchick_think;
	ent->nextthink = level.time + 2 * FRAMETIME;
	gi.linkentity (ent);
}

/*QUAKED misc_easterchick2 (1 .5 0) (-32 -32 0) (32 32 32)
*/


void misc_easterchick2_think (edict_t *self)
{
	if (++self->s.frame < 287)
		self->nextthink = level.time + FRAMETIME;
	else
	{		
		self->s.frame = 248;
		self->nextthink = level.time + FRAMETIME;
	}
}

void SP_misc_easterchick2 (edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	// Lazarus - nonsolid
	if (ent->spawnflags & 1)
		ent->solid = SOLID_NOT;
	else
	{
		ent->solid = SOLID_BBOX;
		VectorSet (ent->mins, -32, -32, 0);
		VectorSet (ent->maxs, 32, 32, 32);
	}
	ent->s.modelindex = gi.modelindex ("models/monsters/bitch/tris.md2");
	ent->s.frame = 248;
	ent->think = misc_easterchick2_think;
	ent->nextthink = level.time + 2 * FRAMETIME;
	gi.linkentity (ent);
}


/*QUAKED monster_commander_body (1 .5 0) (-32 -32 0) (32 32 48)
Not really a monster, this is the Tank Commander's decapitated body.
There should be a item_commander_head that has this as it's target.
*/

void commander_body_think (edict_t *self)
{
	if (++self->s.frame < 24)
		self->nextthink = level.time + FRAMETIME;
	else
		self->nextthink = 0;

	if (self->s.frame == 22)
		gi.sound (self, CHAN_BODY, gi.soundindex ("tank/thud.wav"), 1, ATTN_NORM, 0);
}

void commander_body_use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->think = commander_body_think;
	self->nextthink = level.time + FRAMETIME;
	gi.sound (self, CHAN_BODY, gi.soundindex ("tank/pain.wav"), 1, ATTN_NORM, 0);
}

void commander_body_drop (edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->s.origin[2] += 2;

	if (!(self->spawnflags & SF_MONSTER_FLIES))
		return;
	if (self->waterlevel)
		return;
	self->s.effects |= EF_FLIES;
	self->s.sound = gi.soundindex ("infantry/inflies1.wav");
}

void SP_monster_commander_body (edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	self->model = "models/monsters/commandr/tris.md2";
	self->s.modelindex = gi.modelindex (self->model);
	VectorSet (self->mins, -32, -32, 0);
	VectorSet (self->maxs, 32, 32, 48);
	self->use = commander_body_use;
	self->takedamage = DAMAGE_YES;
	self->flags = FL_GODMODE;
	self->s.renderfx |= RF_FRAMELERP;
	gi.linkentity (self);

	gi.soundindex ("tank/thud.wav");
	gi.soundindex ("tank/pain.wav");

	self->think = commander_body_drop;
	self->nextthink = level.time + 5 * FRAMETIME;
}


/*QUAKED misc_banner (1 .5 0) (-4 -4 -4) (4 4 4)
The origin is the bottom of the banner.
The banner is 128 tall.
*/
void misc_banner_think (edict_t *ent)
{
	ent->s.frame = (ent->s.frame + 1) % 16;
	ent->nextthink = level.time + FRAMETIME;
}

void SP_misc_banner (edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex ("models/objects/banner/tris.md2");
	ent->s.frame = rand() % 16;
	gi.linkentity (ent);

	ent->think = misc_banner_think;
	ent->nextthink = level.time + FRAMETIME;
}

/*QUAKED misc_deadsoldier (1 .5 0) (-16 -16 0) (16 16 16) ON_BACK ON_STOMACH BACK_DECAP FETAL_POS SIT_DECAP IMPALED
This is the dead player model. Comes in 6 exciting different poses!
*/
void misc_deadsoldier_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

	if (self->health > -80)
		return;

	// Lazarus
	self->s.effects &= ~EF_FLIES;
	self->s.sound = 0;

	gi.sound (self, CHAN_BODY, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
	for (n= 0; n < 4; n++)
		ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
	ThrowHead (self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
}

void misc_deadsoldier_flieson(edict_t *self)
{
	if (!(self->spawnflags & 64))
		return;
	if (self->waterlevel)
		return;
	self->s.effects |= EF_FLIES;
	self->s.sound = gi.soundindex ("infantry/inflies1.wav");
}

int PatchDeadSoldier (void);
void SP_misc_deadsoldier (edict_t *ent)
{
	if (deathmatch->value)
	{	// auto-remove for deathmatch
		G_FreeEdict (ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->s.modelindex=gi.modelindex ("models/deadbods/dude/tris.md2");

	// Defaults to frame 0
	if (ent->spawnflags & 2)
		ent->s.frame = 1;
	else if (ent->spawnflags & 4)
		ent->s.frame = 2;
	else if (ent->spawnflags & 8)
		ent->s.frame = 3;
	else if (ent->spawnflags & 16)
		ent->s.frame = 4;
	else if (ent->spawnflags & 32)
		ent->s.frame = 5;
	else
		ent->s.frame = 0;

	// Lazarus
	if (ent->spawnflags & 64) {
		// postpone turning on flies till we figure out whether dude is submerged
		ent->think = misc_deadsoldier_flieson;
		ent->nextthink = level.time + FRAMETIME;
	}
	VectorSet (ent->mins, -16, -16, 0);
	VectorSet (ent->maxs, 16, 16, 16);
	ent->deadflag = DEAD_DEAD;
	ent->takedamage = DAMAGE_YES;
	ent->svflags |= SVF_MONSTER|SVF_DEADMONSTER;
	ent->die = misc_deadsoldier_die;
	ent->monsterinfo.aiflags |= AI_GOOD_GUY;

	if (ent->style)
	{
		PatchDeadSoldier();
		ent->s.skinnum = ent->style;
	}
	ent->common_name = "Dead Marine";
	gi.linkentity (ent);
}

/*QUAKED misc_viper (1 .5 0) (-16 -16 0) (16 16 32)
This is the Viper for the flyby bombing.
It is trigger_spawned, so you must have something use it for it to show up.
There must be a path for it to follow once it is activated.

"speed"		How fast the Viper should fly
*/

extern void train_use (edict_t *self, edict_t *other, edict_t *activator);
extern void func_train_find (edict_t *self);

void viper_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point )
{
	edict_t	*e, *next;

	if (self->deathtarget)
	{
		self->target = self->deathtarget;
		G_UseTargets (self, attacker);
	}

	e = self->movewith_next;
	while(e) {
		next = e->movewith_next;
		if (e->solid == SOLID_NOT) {
			e->nextthink = 0;
			G_FreeEdict(e);
		} else
			BecomeExplosion1 (e);
		e = next;
	}

	self->enemy = inflictor;
	self->activator = attacker;
	func_explosive_explode (self);
}

void misc_viper_use  (edict_t *self, edict_t *other, edict_t *activator)
{
	self->svflags &= ~SVF_NOCLIENT;
	self->use = train_use;
	train_use (self, other, activator);
}

void SP_misc_viper (edict_t *ent)
{
	if (!ent->target)
	{
		gi.dprintf ("misc_viper without a target at %s\n", vtos(ent->absmin));
		G_FreeEdict (ent);
		return;
	}

	if (!ent->speed)
		ent->speed = 300;

	ent->movetype = MOVETYPE_PUSH;
	ent->s.modelindex = gi.modelindex ("models/ships/viper/tris.md2");

	// Lazarus - allow ship to be destroyed if positive health value
	if (ent->health > 0)
	{
		ent->solid = SOLID_BBOX;
		VectorSet (ent->mins, -32, -24, -12);
		VectorSet (ent->maxs,  32,  24,  16);
		ent->takedamage = DAMAGE_YES;
		ent->die = viper_die;
		if (!ent->dmg)
			ent->dmg = 200;
		if (!ent->mass)
			ent->mass = 800;
	}
	else
	{
		ent->solid = SOLID_NOT;
		VectorSet (ent->mins, -16, -16, 0);
		VectorSet (ent->maxs, 16, 16, 32);
	}

	// Lazarus - TRAIN_SMOOTH forces trains to go directly to Move_Done from
	//           Move_Final rather than slowing down (if necessary) for one
	//           frame.
	if (ent->spawnflags & TRAIN_SMOOTH)
		ent->smooth_movement = true;
	else
		ent->smooth_movement = false;

	ent->think = func_train_find;
	ent->nextthink = level.time + FRAMETIME;

	if (ent->spawnflags & TRAIN_START_ON)
		ent->use = train_use;
	else
	{
		ent->use = misc_viper_use;
		ent->svflags |= SVF_NOCLIENT;
	}
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;

	ent->common_name = "Viper";
	gi.linkentity (ent);
}


/*QUAKED misc_bigviper (1 .5 0) (-176 -120 -24) (176 120 72) 
This is a large stationary viper as seen in Paul's intro
*/
void SP_misc_bigviper (edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	VectorSet (ent->mins, -176, -120, -24);
	VectorSet (ent->maxs, 176, 120, 72);
	ent->s.modelindex = gi.modelindex ("models/ships/bigviper/tris.md2");
	ent->common_name = "Viper";
	gi.linkentity (ent);
}


/*QUAKED misc_viper_bomb (1 0 0) (-8 -8 -8) (8 8 8)
"dmg"	how much boom should the bomb make?
*/
void misc_viper_bomb_use (edict_t *, edict_t *, edict_t *);
void misc_viper_bomb_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	G_UseTargets (self, self->activator);

	self->s.origin[2] = self->absmin[2] + 1;
	T_RadiusDamage (self, self, self->dmg, NULL, self->dmg+40, MOD_BOMB, -0.5);

	self->count--;
	if (!self->count)
		self->spawnflags &= ~1;

	if (self->spawnflags & 1)
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_EXPLOSION2);
		gi.WritePosition (self->s.origin);
		gi.multicast (self->s.origin, MULTICAST_PVS);

		if (level.num_reflectors)
			ReflectExplosion (TE_EXPLOSION2, self->s.origin);
		
		self->svflags   |= SVF_NOCLIENT;
		self->solid      = SOLID_NOT;
		self->use        = misc_viper_bomb_use;
		self->s.effects  = 0;
		self->movetype   = MOVETYPE_NONE;
		self->nextthink  = 0;
		self->think      = NULL;
		VectorCopy(self->pos1,self->s.origin);
		VectorCopy(self->pos2,self->s.angles);
		VectorClear(self->velocity);
		gi.linkentity(self);
	}
	else
		BecomeExplosion2 (self);
}

void misc_viper_bomb_prethink (edict_t *self)
{
	vec3_t	v;
	float	diff;

	self->groundentity = NULL;

	diff = self->timestamp - level.time;
	if (diff < -1.0)
		diff = -1.0;

	VectorScale (self->moveinfo.dir, 1.0 + diff, v);
	v[2] = diff;

	diff = self->s.angles[2];
	vectoangles (v, self->s.angles);
	self->s.angles[2] = diff + 10;
}

void viper_bomb_think(edict_t *self)
{
	if (readout->value) {
		gi.dprintf("bomb velocity=%g,%g,%g\n",self->velocity[0],self->velocity[1],self->velocity[2]);
		gi.dprintf("bomb position=%g,%g,%g\n",self->s.origin[0],self->s.origin[1],self->s.origin[2]);
	}
	self->nextthink = level.time + FRAMETIME;
}
void misc_viper_bomb_use (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t	*viper;

	if (self->solid != SOLID_NOT)  // Already in use
		return;

	self->solid = SOLID_BBOX;
	self->svflags &= ~SVF_NOCLIENT;
	self->s.effects |= EF_ROCKET;
	self->use = NULL;
	self->movetype = MOVETYPE_TOSS;
	self->prethink = misc_viper_bomb_prethink;
	self->touch = misc_viper_bomb_touch;
	self->activator = activator;

	VectorCopy(self->pos1,self->s.origin);
	VectorCopy(self->pos2,self->s.angles);

	if (self->pathtarget)
	{
		if (!Q_stricmp(self->pathtarget,self->targetname))
		{
			VectorScale(self->movedir,self->speed,self->velocity);
			VectorCopy(self->movedir,self->moveinfo.dir);
		}
		else
		{
			viper = G_Find(NULL, FOFS(targetname), self->pathtarget);
			if (!viper)
				return;
			VectorScale (viper->moveinfo.dir, viper->moveinfo.speed, self->velocity);
			VectorCopy (viper->moveinfo.dir, self->moveinfo.dir);
		}
	}
	else
	{
		viper = G_Find (NULL, FOFS(classname), "misc_viper");
		if (!viper)
			return;
		VectorScale (viper->moveinfo.dir, viper->moveinfo.speed, self->velocity);
		VectorCopy (viper->moveinfo.dir, self->moveinfo.dir);
	}

	self->timestamp = level.time;

	self->think = viper_bomb_think;
	self->nextthink = level.time + FRAMETIME;
}

void SP_misc_viper_bomb (edict_t *self)
{
	vec3_t	angles;

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	VectorSet (self->mins, -8, -8, -8);
	VectorSet (self->maxs, 8, 8, 8);

	self->s.modelindex = gi.modelindex ("models/objects/bomb/tris.md2");

	// Lazarus: Preserve angles in movedir and initial placement
	//          for case of targeting self
	VectorCopy(self->s.angles,angles);
	G_SetMovedir (angles,self->movedir);
	VectorCopy(self->s.origin,self->pos1);
	VectorCopy(self->s.angles,self->pos2);

	if (!self->dmg)
		self->dmg = 1000;

	self->use = misc_viper_bomb_use;
	self->svflags |= SVF_NOCLIENT;
	self->common_name = "Bomb";
	gi.linkentity (self);
}


/*QUAKED misc_strogg_ship (1 .5 0) (-16 -16 0) (16 16 32)
This is a Storgg ship for the flybys.
It is trigger_spawned, so you must have something use it for it to show up.
There must be a path for it to follow once it is activated.

"speed"		How fast it should fly
*/

extern void train_use (edict_t *self, edict_t *other, edict_t *activator);
extern void func_train_find (edict_t *self);

void misc_strogg_ship_use  (edict_t *self, edict_t *other, edict_t *activator)
{
	self->svflags &= ~SVF_NOCLIENT;
	self->use = train_use;
	train_use (self, other, activator);
}

void SP_misc_strogg_ship (edict_t *ent)
{
	if (!ent->target)
	{
		gi.dprintf ("%s without a target at %s\n", ent->classname, vtos(ent->absmin));
		G_FreeEdict (ent);
		return;
	}

	if (!ent->speed)
		ent->speed = 300;

	ent->movetype = MOVETYPE_PUSH;
	ent->s.modelindex = gi.modelindex ("models/ships/strogg1/tris.md2");
	// Lazarus - allow ship to be destroyed if positive health value
	if (ent->health > 0)
	{
		ent->solid = SOLID_BBOX;
		VectorSet (ent->mins, -58, -60, -40);
		VectorSet (ent->maxs,  72,  60,  38);
		ent->takedamage = DAMAGE_YES;
		ent->die = viper_die;
		if (!ent->dmg)
			ent->dmg = 200;
		if (!ent->mass)
			ent->mass = 1200;
	}
	else
	{
		ent->solid = SOLID_NOT;
		VectorSet (ent->mins, -16, -16, 0);
		VectorSet (ent->maxs, 16, 16, 32);
	}

	// Lazarus - TRAIN_SMOOTH forces trains to go directly to Move_Done from
	//           Move_Final rather than slowing down (if necessary) for one
	//           frame.
	if (ent->spawnflags & TRAIN_SMOOTH)
		ent->smooth_movement = true;
	else
		ent->smooth_movement = false;

	ent->think = func_train_find;
	ent->nextthink = level.time + FRAMETIME;

	if (ent->spawnflags & TRAIN_START_ON)
		ent->use = train_use;
	else
	{
		ent->use = misc_strogg_ship_use;
		ent->svflags |= SVF_NOCLIENT;
	}
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;

	ent->common_name = "Strogg Ship";
	gi.linkentity (ent);
}


/*QUAKED misc_satellite_dish (1 .5 0) (-64 -64 0) (64 64 128)
*/
void misc_satellite_dish_think (edict_t *self)
{
	self->s.frame++;
	if (self->s.frame < 38)
		self->nextthink = level.time + FRAMETIME;
}

void misc_satellite_dish_use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->s.frame = 0;
	self->think = misc_satellite_dish_think;
	self->nextthink = level.time + FRAMETIME;
}

void SP_misc_satellite_dish (edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	VectorSet (ent->mins, -64, -64, 0);
	VectorSet (ent->maxs, 64, 64, 128);
	ent->s.modelindex = gi.modelindex ("models/objects/satellite/tris.md2");
	ent->use = misc_satellite_dish_use;
	ent->common_name = "Satellite Dish";
	gi.linkentity (ent);
}


/*QUAKED light_mine1 (0 1 0) (-2 -2 -12) (2 2 12)
*/
void SP_light_mine1 (edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->s.modelindex = gi.modelindex ("models/objects/minelite/light1/tris.md2");
	gi.linkentity (ent);
}


/*QUAKED light_mine2 (0 1 0) (-2 -2 -12) (2 2 12)
*/
void SP_light_mine2 (edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->s.modelindex = gi.modelindex ("models/objects/minelite/light2/tris.md2");
	gi.linkentity (ent);
}


/*QUAKED misc_gib_arm (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_arm (edict_t *ent)
{
	gi.setmodel (ent, "models/objects/gibs/arm/tris.md2");
	ent->solid = SOLID_NOT;
	ent->s.effects |= EF_GIB;
	ent->takedamage = DAMAGE_YES;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->svflags |= SVF_MONSTER;
	ent->deadflag = DEAD_DEAD;
	ent->avelocity[0] = random()*200;
	ent->avelocity[1] = random()*200;
	ent->avelocity[2] = random()*200;
	ent->think = G_FreeEdict;
	ent->nextthink = level.time + 30;
	gi.linkentity (ent);
}

/*QUAKED misc_gib_leg (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_leg (edict_t *ent)
{
	gi.setmodel (ent, "models/objects/gibs/leg/tris.md2");
	ent->solid = SOLID_NOT;
	ent->s.effects |= EF_GIB;
	ent->takedamage = DAMAGE_YES;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->svflags |= SVF_MONSTER;
	ent->deadflag = DEAD_DEAD;
	ent->avelocity[0] = random()*200;
	ent->avelocity[1] = random()*200;
	ent->avelocity[2] = random()*200;
	ent->think = G_FreeEdict;
	ent->nextthink = level.time + 30;
	gi.linkentity (ent);
}

/*QUAKED misc_gib_head (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_head (edict_t *ent)
{
	gi.setmodel (ent, "models/objects/gibs/head/tris.md2");
	ent->solid = SOLID_NOT;
	ent->s.effects |= EF_GIB;
	ent->takedamage = DAMAGE_YES;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->svflags |= SVF_MONSTER;
	ent->deadflag = DEAD_DEAD;
	ent->avelocity[0] = random()*200;
	ent->avelocity[1] = random()*200;
	ent->avelocity[2] = random()*200;
	ent->think = G_FreeEdict;
	ent->nextthink = level.time + 30;
	gi.linkentity (ent);
}

//=====================================================

/*QUAKED target_character (0 0 1) ?
used with target_string (must be on same "team")
"count" is position in the string (starts at 1)
*/

void SP_target_character (edict_t *self)
{
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel (self, self->model);
	self->solid = SOLID_BSP;
	self->s.frame = 12;
	gi.linkentity (self);
	return;
}


/*QUAKED target_string (0 0 1) (-8 -8 -8) (8 8 8)
*/

void target_string_use (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t *e;
	int		n, l;
	char	c;

	l = (int)strlen(self->message);
	for (e = self->teammaster; e; e = e->teamchain)
	{
		if (!e->count)
			continue;
		n = e->count - 1;
		if (n > l)
		{
			e->s.frame = 12;
			continue;
		}

		c = self->message[n];
		if (c >= '0' && c <= '9')
			e->s.frame = c - '0';
		else if (c == '-')
			e->s.frame = 10;
		else if (c == ':')
			e->s.frame = 11;
		else
			e->s.frame = 12;
	}
}

void SP_target_string (edict_t *self)
{
	if (!self->message)
		self->message = "";
	self->use = target_string_use;
}


/*QUAKED func_clock (0 0 1) (-8 -8 -8) (8 8 8) TIMER_UP TIMER_DOWN START_OFF MULTI_USE
target a target_string with this

The default is to be a time of day clock

TIMER_UP and TIMER_DOWN run for "count" seconds and the fire "pathtarget"
If START_OFF, this entity must be used before it starts

"style"		0 "xx"
			1 "xx:xx"
			2 "xx:xx:xx"
*/

#define	CLOCK_MESSAGE_SIZE	16

// don't let field width of any clock messages change, or it
// could cause an overwrite after a game load

/*static*/ void func_clock_reset (edict_t *self)
{
	self->activator = NULL;
	if (self->spawnflags & 1)
	{
		self->health = 0;
		self->wait = self->count;
	}
	else if (self->spawnflags & 2)
	{
		self->health = self->count;
		self->wait = 0;
	}
}

// Skuller's hack to fix crash on exiting biggun
typedef struct zhead_s {
   struct zhead_s   *prev, *next;
   short   magic;
   short   tag;         // for group free
   int      size;
} zhead_t; 

/*static*/ void func_clock_format_countdown (edict_t *self)
{
	zhead_t *z = ( zhead_t * )self->message - 1;
	int size = z->size - sizeof (zhead_t);

	if (size < CLOCK_MESSAGE_SIZE) {
		gi.TagFree (self->message);
		self->message = gi.TagMalloc (CLOCK_MESSAGE_SIZE, TAG_LEVEL);
		//gi.dprintf ("WARNING: func_clock_format_countdown: self->message is too small: %i\n", size);
	} 
	// end Skuller's hack

	if (self->style == 0)
	{
		Com_sprintf (self->message, CLOCK_MESSAGE_SIZE, "%2i", self->health);
		return;
	}

	if (self->style == 1)
	{
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i", self->health / 60, self->health % 60);
		if (self->message[3] == ' ')
			self->message[3] = '0';
		return;
	}

	if (self->style == 2)
	{
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i:%2i", self->health / 3600, (self->health - (self->health / 3600) * 3600) / 60, self->health % 60);
		if (self->message[3] == ' ')
			self->message[3] = '0';
		if (self->message[6] == ' ')
			self->message[6] = '0';
		return;
	}
}

void func_clock_think (edict_t *self)
{
	if (!self->enemy)
	{
		self->enemy = G_Find (NULL, FOFS(targetname), self->target);
		if (!self->enemy)
			return;
	}

	if (self->spawnflags & 1)
	{
		func_clock_format_countdown (self);
		self->health++;
	}
	else if (self->spawnflags & 2)
	{
		func_clock_format_countdown (self);
		self->health--;
	}
	else
	{
		struct tm	*ltime;
		time_t		gmtime;

		time(&gmtime);
		ltime = localtime(&gmtime);
		Com_sprintf (self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i:%2i", ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
		if (self->message[3] == ' ')
			self->message[3] = '0';
		if (self->message[6] == ' ')
			self->message[6] = '0';
	}

	self->enemy->message = self->message;
	self->enemy->use (self->enemy, self, self);

	if (((self->spawnflags & 1) && (self->health > self->wait)) ||
		((self->spawnflags & 2) && (self->health < self->wait)))
	{
		if (self->pathtarget)
		{
			char *savetarget;
			char *savemessage;

			savetarget = self->target;
			savemessage = self->message;
			self->target = self->pathtarget;
			self->message = NULL;
			G_UseTargets (self, self->activator);
			self->target = savetarget;
			self->message = savemessage;
		}

		if (!(self->spawnflags & 8)) {
			// Lazarus: Can't be used again, so get rid of it
			self->think = G_FreeEdict;
			self->nextthink = level.time + 1;
			return;
		}

		func_clock_reset (self);

		if (self->spawnflags & 4)
			return;
	}

	self->nextthink = level.time + 1;
}

void func_clock_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (!(self->spawnflags & 8))
		self->use = NULL;

	if (self->activator)
		return;

	self->activator = activator;
	self->think (self);
}

void SP_func_clock (edict_t *self)
{
	if (!self->target)
	{
		gi.dprintf("%s with no target at %s\n", self->classname, vtos(self->s.origin));
		G_FreeEdict (self);
		return;
	}

	if ((self->spawnflags & 2) && (!self->count))
	{
		gi.dprintf("%s with no count at %s\n", self->classname, vtos(self->s.origin));
		G_FreeEdict (self);
		return;
	}

	if ((self->spawnflags & 1) && (!self->count))
		self->count = 60*60;;

	func_clock_reset (self);

	self->message = gi.TagMalloc (CLOCK_MESSAGE_SIZE, TAG_LEVEL);

	self->think = func_clock_think;

	if (self->spawnflags & 4)
		self->use = func_clock_use;
	else
		self->nextthink = level.time + 1;
}

//CW++
/*QUAKED func_clock_screen (0 0 1) (-8 -8 -8) (8 8 8) TIMER_UP TIMER_DOWN START_OFF MULTI_USE
Displays clock readout on the screen.

The default is to be a time of day clock.

TIMER_UP and TIMER_DOWN run for "count" seconds and the fire "pathtarget"
If START_OFF, this entity must be used before it starts

"style"		0 "xx"
			1 "xx:xx"
			2 "xx:xx:xx"
*/

void func_clock_screen_think(edict_t *self)
{
	char text[CLOCK_MESSAGE_SIZE];

	if (self->spawnflags & 16)
	{
		if (!self->activator)
		{
			self->activator = g_edicts + 1;							//dirty hack!
			self->nextthink = level.time + 1;
			return;
		}
		else if ( self->activator->classname && strlen(self->activator->classname) )	// Knightmare- catch null/bad classname
		{
			if (Q_stricmp(self->activator->classname, "player"))	//map not yet finished loading
			{
				self->nextthink = level.time + 1;
				return;
			}
		}
	}

	if (self->spawnflags & 1)
	{
		self->health = ++game.clock_count;
		func_clock_format_countdown(self);
	}
	else if (self->spawnflags & 2)
	{
		self->health = --game.clock_count;
		func_clock_format_countdown(self);
	}
	else
	{
		struct tm	*ltime;
		time_t		gmtime;

		time(&gmtime);
		ltime = localtime(&gmtime);
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i:%2i", ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
		if (self->message[3] == ' ')
			self->message[3] = '0';
		if (self->message[6] == ' ')
			self->message[6] = '0';
	}

	sprintf(text, "%s", self->message);
	gi.configstring(CS_CLOCK_TIMER, text);

	if (((self->spawnflags & 1) && (self->health >= self->wait)) || ((self->spawnflags & 2) && (self->health <= self->wait)))
	{
		if (self->pathtarget)
		{
			char *savetarget;
			char *savemessage;

			savetarget = self->target;
			savemessage = self->message;
			self->target = self->pathtarget;
			self->message = NULL;
			G_UseTargets (self, self->activator);
			self->target = savetarget;
			self->message = savemessage;
		}

		if (!(self->spawnflags & 8))
		{
			// Lazarus: Can't be used again, so get rid of it
			self->think = G_FreeEdict;
			self->nextthink = level.time + 1;
			return;
		}

		func_clock_reset(self);
		game.clock_count = 0;
		game.clock_ticking = 0;

		if (self->spawnflags & 4)
			return;
	}

	self->nextthink = level.time + 1;
}

void func_clock_screen_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (!(self->spawnflags & 8))
		self->use = NULL;

	if (self->activator)
		return;

	game.clock_ticking = 1;
	game.clock_count = self->health;

	self->activator = activator;
	self->think(self);
}

void SP_func_clock_screen(edict_t *self)
{
	if (game.clock_ticking)
	{
		self->spawnflags |= 16;
		self->health = game.clock_count + 5;
		self->message = gi.TagMalloc(CLOCK_MESSAGE_SIZE, TAG_LEVEL);
		self->think = func_clock_screen_think;
		self->nextthink = level.time + 1;
		self->think(self);
		return;
	}

	if ((self->spawnflags & 2) && !self->count)
	{
		gi.dprintf("%s with no count at %s\n", self->classname, vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	if ((self->spawnflags & 1) && !self->count)
		self->count = 60 * 60;

	func_clock_reset(self);
	self->message = gi.TagMalloc(CLOCK_MESSAGE_SIZE, TAG_LEVEL);
	self->think = func_clock_screen_think;

	if (self->spawnflags & 4)
		self->use = func_clock_screen_use;
	else
	{
		self->nextthink = level.time + 1;
		game.clock_ticking = 1;
		game.clock_count = self->health;
	}
}
//CW--

/*==================================================================
 Lazarus changes to misc_teleporter:

 START_OFF (=1)  If set, misc_teleporter must be targeted to work
 TOGGLE    (=2)  If set, misc_teleporter is toggled on/off each time
                 it is targeted
 NO_MODEL  (=4)  If set, teleporter does not use the normal teleporter
                 base or splash effect
 LANDMARK  (=64) If set, player angles and speed are preserved

 misc_teleporter selects a random pick from up to 8 targets for a 
 destination

 trigger_transition with same name as teleporter can be used to 
 teleport multiple non-player entities
================================================================== */
void teleport_transition_ents (edict_t *transition, edict_t *teleporter, edict_t *destination)
{
	extern entlist_t DoNotMove;
	int			i, j;
	int			total=0;
	qboolean	nogo=false;
	edict_t		*ent;
	entlist_t	*p;
	vec3_t		angles, forward, right, v;
	vec3_t		start;

	angles[YAW] = destination->s.angles[YAW] - teleporter->s.angles[YAW];
	angles[PITCH] = angles[ROLL] = 0;
	AngleVectors(angles,forward,right,NULL);
	VectorNegate(right,right);
	if (teleporter->solid == SOLID_TRIGGER)
		VectorMA(teleporter->absmin,0.5,teleporter->size,start);
	else
		VectorCopy(teleporter->s.origin,start);

	for(i=game.maxclients+1; i<globals.num_edicts; i++)
	{
		ent = &g_edicts[i];
		ent->id = 0;
		if (!ent->inuse) continue;
		// Pass up owned entities not owned by the player on this pass...
		// get 'em next pass so we'll know whether owner is in our list
		if (ent->owner && !ent->owner->client) continue;
		if (ent->movewith) continue;
		if (ent->solid == SOLID_BSP) continue;
		if ((ent->solid == SOLID_TRIGGER) && !FindItemByClassname(ent->classname)) continue;
		// Do not under any circumstances move these entities:
		for(p=&DoNotMove, nogo=false; p->name && !nogo; p++)
			if (!Q_stricmp(ent->classname,p->name))
				nogo = true;
		if (nogo) continue;
		if (!HasSpawnFunction(ent)) continue;
		if (ent->s.origin[0] > transition->maxs[0]) continue;
		if (ent->s.origin[1] > transition->maxs[1]) continue;
		if (ent->s.origin[2] > transition->maxs[2]) continue;
		if (ent->s.origin[0] < transition->mins[0]) continue;
		if (ent->s.origin[1] < transition->mins[1]) continue;
		if (ent->s.origin[2] < transition->mins[2]) continue;
		total++;
		ent->id = total;
		if (ent->owner)
			ent->owner_id = -(ent->owner - g_edicts);
		else
			ent->owner_id = 0;
		if (angles[YAW])
		{
			vec3_t	spawn_offset;
		
			VectorSubtract(ent->s.origin,start,spawn_offset);
			VectorCopy(spawn_offset,v);
			G_ProjectSource (vec3_origin, v, forward, right, spawn_offset);
			VectorCopy(spawn_offset,ent->s.origin);
			VectorCopy(ent->velocity,v);
			G_ProjectSource (vec3_origin, v, forward, right, ent->velocity);
			ent->s.angles[YAW] += angles[YAW];
		}
		else
		{
			VectorSubtract(ent->s.origin,teleporter->s.origin,ent->s.origin);
		}
		VectorAdd(ent->s.origin,destination->s.origin,ent->s.origin);
		gi.linkentity(ent);
	}
	// Repeat, ONLY for ents owned by non-players
	for(i=game.maxclients+1; i<globals.num_edicts; i++)
	{
		ent = &g_edicts[i];
		if (!ent->inuse) continue;
		if (!ent->owner) continue;
		if (ent->owner->client) continue;
		if (ent->movewith) continue;
		if (ent->solid == SOLID_BSP) continue;
		if ((ent->solid == SOLID_TRIGGER) && !FindItemByClassname(ent->classname)) continue;
		// Do not under any circumstances move these entities:
		for(p=&DoNotMove, nogo=false; p->name && !nogo; p++)
			if (!Q_stricmp(ent->classname,p->name))
				nogo = true;
		if (nogo) continue;
		if (!HasSpawnFunction(ent)) continue;
		if (ent->s.origin[0] > transition->maxs[0]) continue;
		if (ent->s.origin[1] > transition->maxs[1]) continue;
		if (ent->s.origin[2] > transition->maxs[2]) continue;
		if (ent->s.origin[0] < transition->mins[0]) continue;
		if (ent->s.origin[1] < transition->mins[1]) continue;
		if (ent->s.origin[2] < transition->mins[2]) continue;
		ent->owner_id = 0;
		for(j=game.maxclients+1; j<globals.num_edicts && !ent->owner_id; j++)
		{
			if (ent->owner == &g_edicts[j])
				ent->owner_id = g_edicts[j].id;
		}
		if (!ent->owner_id) continue;
		total++;
		ent->id = total;
		if (angles[YAW])
		{
			vec3_t	spawn_offset;
		
			VectorSubtract(ent->s.origin,start,spawn_offset);
			VectorCopy(spawn_offset,v);
			G_ProjectSource (vec3_origin, v, forward, right, spawn_offset);
			VectorCopy(spawn_offset,ent->s.origin);
			VectorCopy(ent->velocity,v);
			G_ProjectSource (vec3_origin, v, forward, right, ent->velocity);
			ent->s.angles[YAW] += angles[YAW];
		}
		else
		{
			VectorSubtract(ent->s.origin,teleporter->s.origin,ent->s.origin);
		}
		VectorAdd(ent->s.origin,destination->s.origin,ent->s.origin);
		gi.linkentity(ent);
	}
}

// G_PickDestination is identical to G_PickTarget, but w/o the 
// obnoxious error message.

#define MAXCHOICES 8
static edict_t *G_PickDestination (char *targetname)
{
	edict_t	*ent = NULL;
	int		num_choices = 0;
	edict_t	*choice[MAXCHOICES];

	if (!targetname)
	{
		gi.dprintf("G_PickDestination called with NULL targetname\n");
		return NULL;
	}

	while(1)
	{
		ent = G_Find (ent, FOFS(targetname), targetname);
		if (!ent)
			break;
		choice[num_choices++] = ent;
		if (num_choices == MAXCHOICES)
			break;
	}

	if (!num_choices)
		return NULL;

	return choice[rand() % num_choices];
}
//=================================================================================

void teleporter_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t		*dest;
	edict_t		*teleporter;
	qboolean	landmark;
	int			i;

	// Lazarus: teleporters with MONSTER spawnflag can be used by monsters
	if (!other->client && (!(self->spawnflags & 8) || !(other->svflags & SVF_MONSTER)))
		return;
	// Lazarus: Use G_PickTarget instead so we can have multiple destinations
	//dest = G_Find (NULL, FOFS(targetname), self->target);
	dest = G_PickDestination(self->target);
	if (!dest)
	{
		gi.dprintf ("Couldn't find destination\n");
		return;
	}

	if (self->owner)
		teleporter = self->owner;
	else
		teleporter = self;

	// unlink to make sure it can't possibly interfere with KillBox
	gi.unlinkentity (other);

	VectorCopy (dest->s.origin, other->s.origin);
	VectorCopy (dest->s.origin, other->s.old_origin);
	other->s.origin[2] += 10;

	// clear the velocity and hold them in place briefly
	landmark = (teleporter->spawnflags & 64 ? true : false);

	if (landmark)
	{
		vec3_t	angles, forward, right, v;

		VectorSubtract(dest->s.angles,teleporter->s.angles,angles);
		AngleVectors(angles,forward,right,NULL);
		VectorNegate(right,right);
		VectorCopy(other->velocity,v);
		G_ProjectSource (vec3_origin,
					     v, forward, right,
						 other->velocity);
	}
	else
		VectorClear (other->velocity);

	if (other->client) {
		other->client->ps.pmove.pm_time = 160>>3;		// hold time
		other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;
	}

	if (self->owner)
	{
		self->owner->s.event = EV_PLAYER_TELEPORT;
		other->s.event = EV_PLAYER_TELEPORT;		//CW: moved here to prevent effect if not flagged
	}
	else
	{
		// This must be a trigger_teleporter
		vec3_t	origin;
		VectorMA(self->mins,0.5,self->size,origin);

		if (!(self->spawnflags & 16))
		{
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_TELEPORT_EFFECT);
			gi.WritePosition(origin);
			gi.multicast(origin, MULTICAST_PHS);

			other->s.event = EV_PLAYER_TELEPORT;	//CW: moved here to prevent effect if not flagged
		}
		gi.positioned_sound(origin,self,CHAN_AUTO,self->noise_index,1,1,0);
	}

	// set angles
	if (landmark)
	{
		edict_t	*transition;

		// LANDMARK
		if (other->client)
		{
			other->s.angles[ROLL] = 0;
			for (i=0 ; i<3 ; i++)
				other->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(dest->s.angles[i] - teleporter->s.angles[i] + other->s.angles[i] - other->client->resp.cmd_angles[i]);
			VectorClear (other->client->ps.viewangles);
			VectorClear (other->client->v_angle);
			VectorClear (other->s.angles);
		}
		else
		{
			VectorSubtract(other->s.angles,teleporter->s.angles,other->s.angles);
			VectorAdd(other->s.angles,dest->s.angles,other->s.angles);
		}

		transition = G_Find(NULL,FOFS(classname),"trigger_transition");
		while(transition)
		{
			if (!Q_stricmp(transition->targetname,teleporter->targetname))
				teleport_transition_ents(transition,teleporter,dest);
			transition = G_Find(transition,FOFS(classname),"trigger_transition");
		}

	}
	else
	{
		if (other->client)
		{
			for (i=0 ; i<3 ; i++)
				other->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(dest->s.angles[i] - other->client->resp.cmd_angles[i]);
			VectorClear (other->client->ps.viewangles);
			VectorClear (other->client->v_angle);
			VectorClear (other->s.angles);
		} else
			VectorCopy (dest->s.angles,other->s.angles);
	}


	// kill anything at the destination
	KillBox (other);

	if (!self->owner)
	{
		// this is a trigger_teleporter
		self->count--;
		if (!self->count)
			G_FreeEdict(self);
	}
	gi.linkentity (other);
}

void use_teleporter (edict_t *self, edict_t *other, edict_t *activator) {

	if (!(self->spawnflags & 1))
	{
		if (!(self->spawnflags & 2))
			return;

		self->spawnflags |= 1;
		self->s.effects &= ~EF_TELEPORTER;
		self->target_ent->solid = SOLID_NOT;
		self->s.sound = 0;
	}
	else
	{
		self->spawnflags &= ~1;
		if (!(self->spawnflags & 32))				//CSW: Was 4; added 'no effects' instead
			self->s.effects |= EF_TELEPORTER;
		self->target_ent->solid = SOLID_TRIGGER;
		self->s.sound = gi.soundindex ("world/amb10.wav");
	}
	gi.linkentity(self);
	gi.linkentity(self->target_ent);
}

/*QUAKED misc_teleporter (1 0 0) (-32 -32 -24) (32 32 -16)
Stepping onto this disc will teleport players to the targeted misc_teleporter_dest object.
*/
void SP_misc_teleporter (edict_t *ent)
{
	edict_t		*trig;

	if (!ent->target)
	{
		gi.dprintf ("teleporter without a target.\n");
		G_FreeEdict (ent);
		return;
	}

	if (!(ent->spawnflags & 1) && !(ent->spawnflags & 32))	//CSW: Added s/f 32 check
	{
		ent->s.effects = EF_TELEPORTER;
		ent->s.sound = gi.soundindex ("world/amb10.wav");
	}

	if (!(ent->spawnflags & 4))
	{
		gi.setmodel (ent, "models/objects/dmspot/tris.md2");
		ent->s.skinnum = 1;
	}

	if (ent->spawnflags & 3)
		ent->use = use_teleporter;

	if (ent->spawnflags & 4)
	{
		ent->solid = SOLID_NOT;
	}
	else
	{
		ent->solid = SOLID_BBOX;
		VectorSet (ent->mins, -32, -32, -24);
		VectorSet (ent->maxs, 32, 32, -16);
	}
	ent->common_name = "Teleporter";
	gi.linkentity (ent);

	trig = G_Spawn ();
	ent->target_ent = trig;
	trig->touch = teleporter_touch;
	trig->spawnflags = ent->spawnflags & 8; // Lazarus: Monster-usable
	if (ent->spawnflags & 1)
		trig->solid = SOLID_NOT;
	else
		trig->solid = SOLID_TRIGGER;

	trig->target = ent->target;
	trig->owner = ent;

	// Lazarus
	trig->movewith = ent->movewith;
	VectorCopy (ent->s.origin, trig->s.origin);
	VectorSet (trig->mins, -8, -8, 8);
	VectorSet (trig->maxs, 8, 8, 24);
	gi.linkentity (trig);
	
}

void trigger_teleporter_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->solid == SOLID_NOT)
		self->solid = SOLID_TRIGGER;
	else
		self->solid = SOLID_NOT;
	gi.linkentity(self);
}

// trigger_teleporter
// spawnflags
//  2 = TRIGGERED
//  8 = MONSTER
// 16 = NO EFFECT
// 32 = CUSTOM
void SP_trigger_teleporter (edict_t *self)
{
	if (!self->target)
	{
		gi.dprintf ("trigger_teleporter without a target.\n");
		G_FreeEdict (self);
		return;
	}
	if (!(self->spawnflags & 48))
		self->noise_index = gi.soundindex("misc/tele1.wav");
	else if (self->spawnflags & 32)
	{
		if (st.noise)
			self->noise_index = gi.soundindex(st.noise);
		else
			self->noise_index = 0;
	}
	else
		self->noise_index = 0;

	gi.setmodel (self, self->model);
	self->touch   = teleporter_touch;
	self->svflags = SVF_NOCLIENT;
	if (self->spawnflags & 2)
		self->solid = SOLID_NOT;
	else
		self->solid = SOLID_TRIGGER;
	gi.linkentity (self);
}

/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
*/
void SP_misc_teleporter_dest (edict_t *ent)
{
	if (!(ent->spawnflags & 1))	//CW++	Spawnflag = 1 => no model used
	{
		gi.setmodel (ent, "models/objects/dmspot/tris.md2");
		ent->s.skinnum = 0;
		ent->solid = SOLID_BBOX;
		VectorSet (ent->mins, -32, -32, -24);
		VectorSet (ent->maxs, 32, 32, -16);
	}

	ent->common_name = "Teleporter Destination";
	gi.linkentity (ent);
}

// Lazarus new entities

void misc_light_think (edict_t *self)
{
	if (self->spawnflags & START_OFF) return;
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_FLASHLIGHT);
	gi.WritePosition (self->s.origin);
	gi.WriteShort (self - g_edicts);
	gi.multicast (self->s.origin, MULTICAST_PVS);
	self->nextthink = level.time + FRAMETIME;
}

void misc_light_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->spawnflags & START_OFF) {
		self->spawnflags &= ~START_OFF;
		self->nextthink = level.time + FRAMETIME;
	} else
		self->spawnflags |= START_OFF;
}

void SP_misc_light (edict_t *self)
{
	self->use = misc_light_use;
	if (self->movewith)
		self->movetype = MOVETYPE_PUSH;
	else
		self->movetype = MOVETYPE_NONE;
	self->think = misc_light_think;
	if (!(self->spawnflags & START_OFF))
		self->nextthink = level.time + 2*FRAMETIME;
}

/*=============================================================

TARGET_PRECIPITATION

==============================================================*/

#define SF_WEATHER_STARTON			1
#define SF_WEATHER_SPLASH			2
#define SF_WEATHER_GRAVITY_BOUNCE	4
#define SF_WEATHER_FIRE_ONCE		8
#define SF_WEATHER_START_FADE		16
#define SF_FOUNTAIN_ANIM			32	//CW

#define STYLE_WEATHER_RAIN			0
#define STYLE_WEATHER_BIGRAIN		1
#define STYLE_WEATHER_SNOW			2
#define STYLE_WEATHER_LEAF			3
#define STYLE_WEATHER_USER			4

void drop_add_to_chain(edict_t *drop)
{
	edict_t	*owner = drop->owner;
	edict_t *parent;

	if (!owner || !owner->inuse || !(owner->spawnflags & SF_WEATHER_STARTON))
	{
		G_FreeEdict(drop);
		return;
	}

	parent = owner;
	while (parent->child)
		parent = parent->child;

	parent->child = drop;
	drop->child = NULL;
	drop->svflags |= SVF_NOCLIENT;
	drop->s.effects &= ~EF_SPHERETRANS;
	drop->s.renderfx &= ~RF_TRANSLUCENT;
	VectorClear(drop->velocity);
	VectorClear(drop->avelocity);
	gi.linkentity(drop);
}

void drop_splash(edict_t *drop)
{
	vec3_t	up = {0,0,1};

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_LASER_SPARKS);
	gi.WriteByte (drop->owner->mass2);
	gi.WritePosition (drop->s.origin);
	gi.WriteDir (up);
	gi.WriteByte (drop->owner->sounds);
	gi.multicast (drop->s.origin, MULTICAST_PVS);

	drop_add_to_chain(drop);
}

//CW+++
void leaf_fade(edict_t *self);

void fountain_animate(edict_t *self)
{
//	Cycle through frame numbers 'startframe' to 'framenumbers'.

	self->s.frame++;
	if (self->s.frame >= self->framenumbers)
		self->s.frame = self->startframe;

//	Check if it's time to fade yet (self->wait would have been set in 
//	spawn_precipitation or drop_touch if so).

	if (self->wait && (level.time >= self->wait))
		self->think = leaf_fade;

	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}
//CW---

void leaf_fade2(edict_t *self)
{
	if (!self->wait)	//CW: added if-statement for animated target_fountains
	{
		self->count++;
		if (self->count == 1)
		{
			self->s.effects |= EF_SPHERETRANS;
			self->nextthink = level.time + 0.5;
			gi.linkentity(self);
		}
		else
			drop_add_to_chain(self);
	}

//CW+++ Make sure animated target_fountains do their thing in the meantime.
	if (self->spawnflags & SF_FOUNTAIN_ANIM)
	{
		if (!self->wait)
			self->wait = self->nextthink;		//abuse 'wait' to store initial 'nexttime'

		fountain_animate(self);
		self->think = leaf_fade2;	//reset from fountain_animate

		if (level.time >= self->wait)
			self->wait = 0;		//reset for next use with leaf_fade2 (remember, self->count is 1)
	}
//CW---	
}

void leaf_fade(edict_t *self)
{
	if (!self->wait)	//CW: added if-statement for animated target_fountains
	{
		self->s.renderfx = RF_TRANSLUCENT;
		self->think = leaf_fade2;
		self->nextthink = level.time + 0.5;
		self->count = 0;
	}

//CW+++ Make sure animated target_fountains do their thing in the meantime.
	if (self->spawnflags & SF_FOUNTAIN_ANIM)
	{
		if (!self->wait)
			self->wait = self->nextthink;	//abuse 'wait' to store initial 'nexttime'

		fountain_animate(self);

		if (level.time >= self->wait)
		{
			self->wait = 0;		//reset for use with leaf_fade2
			self->think = leaf_fade2;
		}
		else
			self->think = leaf_fade;	//reset from fountain_animate
	}
//CW---	
	
	gi.linkentity(self);
}

void drop_touch(edict_t *drop, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (drop->owner->spawnflags & SF_WEATHER_START_FADE)
		return;
	else if (drop->fadeout > 0)
	{
		if ( (drop->spawnflags & SF_WEATHER_GRAVITY_BOUNCE) && (drop->owner->gravity > 0))
		{
			drop->movetype = MOVETYPE_DEBRIS;
			drop->gravity  = drop->owner->gravity;
		}

//CW+++ Make sure animated target_fountains do their thing in the meantime.
		if (drop->spawnflags & SF_FOUNTAIN_ANIM)
		{
			drop->think = fountain_animate;
			drop->wait = level.time + drop->fadeout;
			drop->nextthink = level.time + FRAMETIME;
		}
		else
//CW---
		{
			drop->think = leaf_fade;
			drop->nextthink = level.time + drop->fadeout;
		}
	}
	else if (drop->spawnflags & SF_WEATHER_SPLASH)
		drop_splash(drop);
	else
		drop_add_to_chain(drop);
}

void spawn_precipitation(edict_t *self, vec3_t org, vec3_t dir, float speed)
{
	edict_t *drop;

	if (self->child)
	{
		// Then we already have a currently unused, invisible drop available
		drop = self->child;
		self->child = drop->child;
		drop->child = NULL;
		drop->svflags &= ~SVF_NOCLIENT;
		drop->groundentity = NULL;
	}
	else
	{
		drop = G_Spawn();
		if (self->style == STYLE_WEATHER_BIGRAIN)
			drop->s.modelindex = gi.modelindex ("models/objects/drop/heavy.md2");
		else if (self->style == STYLE_WEATHER_SNOW)
			drop->s.modelindex = gi.modelindex ("models/objects/snow/tris.md2");
		else if (self->style == STYLE_WEATHER_LEAF)
		{
			float	r=random();
			if (r < 0.33)
				drop->s.modelindex = gi.modelindex ("models/objects/leaf1/tris.md2");
			else if (r < 0.66)
				drop->s.modelindex = gi.modelindex ("models/objects/leaf2/tris.md2");
			else
				drop->s.modelindex = gi.modelindex ("models/objects/leaf3/tris.md2");
			VectorSet(drop->mins,-1,-1,-1);
			VectorSet(drop->maxs, 1, 1, 1);
		}
		else if (self->style == STYLE_WEATHER_USER)
		{

//CW+++ Copy animation info from parent entity.
			if (self->spawnflags & SF_FOUNTAIN_ANIM)
			{
				drop->framenumbers = self->framenumbers;
				drop->startframe = self->startframe;
				drop->s.frame = self->startframe;
				drop->think = fountain_animate;
				drop->nextthink = level.time + FRAMETIME;
			}
//CW---

			drop->s.modelindex = gi.modelindex(self->usermodel);
		}
		else
			drop->s.modelindex = gi.modelindex ("models/objects/drop/tris.md2");
		drop->classname = "rain drop";
	}
	if (self->gravity > 0. || self->attenuation > 0 )
		drop->movetype = MOVETYPE_DEBRIS;
	else
		drop->movetype = MOVETYPE_RAIN;

	drop->touch = drop_touch;
	if (self->style == STYLE_WEATHER_USER)
		drop->clipmask = MASK_MONSTERSOLID;
	else if ((self->fadeout > 0) && (self->gravity == 0.))
		drop->clipmask = MASK_SOLID | CONTENTS_WATER;
	else
		drop->clipmask = MASK_MONSTERSOLID | CONTENTS_WATER;

	drop->solid = SOLID_BBOX;
	drop->svflags = SVF_DEADMONSTER;
	VectorSet(drop->mins,-1,-1,-1);
	VectorSet(drop->maxs, 1, 1, 1);

	if (self->spawnflags & SF_WEATHER_GRAVITY_BOUNCE)
		drop->gravity = self->gravity;
	else
		drop->gravity = 0.;

	drop->attenuation = self->attenuation;
	drop->mass        = self->mass;
	drop->spawnflags  = self->spawnflags;
	drop->fadeout     = self->fadeout;
	drop->owner       = self;

	VectorCopy (org, drop->s.origin);
	vectoangles(dir, drop->s.angles);
	drop->s.angles[PITCH] -= 90;
	VectorScale (dir, speed, drop->velocity);
	if (self->style == STYLE_WEATHER_LEAF)
	{
		drop->avelocity[PITCH] = crandom() * 360;
		drop->avelocity[YAW] = crandom() * 360;
		drop->avelocity[ROLL] = crandom() * 360;
	}
	else if (self->style == STYLE_WEATHER_USER)
	{
		drop->s.effects = self->effects;
		drop->s.renderfx = self->renderfx;
		drop->avelocity[PITCH] = crandom() * self->pitch_speed;
		drop->avelocity[YAW]   = crandom() * self->yaw_speed;
		drop->avelocity[ROLL]  = crandom() * self->roll_speed;
	}
	else
	{
		drop->s.effects |= EF_SPHERETRANS;
		drop->avelocity[YAW] = self->yaw_speed;
	}
	if (self->spawnflags & SF_WEATHER_START_FADE)
	{

//CW+++ Make sure animated target_fountains do their thing in the meantime.
		if (drop->spawnflags & SF_FOUNTAIN_ANIM)
		{
			drop->think = fountain_animate;
			drop->wait = level.time + self->fadeout;
			drop->nextthink = level.time + FRAMETIME;
		}
		else
//CW---

		{
			drop->think = leaf_fade;
			drop->nextthink = level.time + self->fadeout;
		}
	}
	gi.linkentity(drop);
}

void target_precipitation_think (edict_t *self)
{
	vec3_t	center;
	vec3_t	org;
	int		r, i;
	float	u, v, z;
	float	temp;
	qboolean	can_see_me;

	self->nextthink = level.time + FRAMETIME;

	// Don't start raining until player is in the game. The following 
	// takes care of both initial map load conditions and restored saved games.
	// This is a gross abuse of groundentity_linkcount. Sue me.
	if (g_edicts[1].linkcount == self->groundentity_linkcount)
		return;
	else
		self->groundentity_linkcount = g_edicts[1].linkcount;
	// Don't spawn drops if player can't see us. This SEEMS like an obvious
	// thing to do, but can cause visual problems if mapper isn't careful.
	// For example, placing target_precipitation where it isn't in the PVS
	// of the player's current position, but the result (rain) IS in the
	// PVS. In any case, this step is necessary to prevent overflows when
	// player suddenly encounters rain.
	can_see_me = false;
	for(i=1; i<=game.maxclients && !can_see_me; i++)
	{
		if (!g_edicts[i].inuse) continue;
		if (gi.inPVS(g_edicts[i].s.origin,self->s.origin))
			can_see_me = true;
	}
	if (!can_see_me) return;

	// Count is models/second. We accumulate a probability of a model
	// falling this frame in ->density. Yeah its a misnomer but density isn't 	// used for anything else so it works fine.

	temp = 0.1*(self->density + crandom()*self->random);
	r = (int)(temp);
	if (r > 0)
		self->density = self->count + (temp-(float)r)*10;
	else
		self->density += (temp*10);
	if (r < 1) return;

	VectorAdd(self->bleft,self->tright,center);
	VectorMA(self->s.origin,0.5,center,center);

	for(i=0; i<r; i++)
	{
		u = crandom() * (self->tright[0] - self->bleft[0])/2;
		v = crandom() * (self->tright[1] - self->bleft[1])/2;
		z = crandom() * (self->tright[2] - self->bleft[2])/2;
		
		VectorCopy(center, org);
		
		org[0] += u;
		org[1] += v;
		org[2] += z;

		spawn_precipitation(self, org, self->movedir, self->speed);
	}
}

void target_precipitation_use (edict_t *ent, edict_t *other, edict_t *activator)
{
	if (ent->spawnflags & SF_WEATHER_STARTON)
	{
		// already on; turn it off
		ent->nextthink = 0;
		ent->spawnflags &= ~SF_WEATHER_STARTON;
		if (ent->child)
		{
			edict_t	*child, *parent;
			child = ent->child;
			ent->child = NULL;
			while(child)
			{
				parent = child;
				child = parent->child;
				G_FreeEdict(parent);
			}
		}
	}
	else
	{
		ent->density = ent->count;
		ent->think = target_precipitation_think;
		ent->spawnflags |= SF_WEATHER_STARTON;
		ent->think(ent);
	}
}

void target_precipitation_delayed_use (edict_t *self)
{
	// Since target_precipitation tends to be a processor hog,
	// for START_ON we wait until the player has spawned into the
	// game to ease the startup burden somewhat
	if (g_edicts[1].linkcount)
	{
		self->think = target_precipitation_think;
		self->think(self);
	}
	else
		self->nextthink = level.time + FRAMETIME;
}
void SP_target_precipitation (edict_t *ent)
{
	if (deathmatch->value || coop->value)
	{
		G_FreeEdict(ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;

	if (ent->spawnflags & SF_WEATHER_STARTON)
	{
		ent->think = target_precipitation_delayed_use;
		ent->nextthink = level.time + 1;
	}

	if (ent->style == STYLE_WEATHER_USER)
	{
		char	*buffer;
		if (!ent->usermodel)
		{
			gi.dprintf("target_precipitation style=user\nwith no usermodel.\n");
			G_FreeEdict(ent);
			return;
		}
		buffer = gi.TagMalloc((int)strlen(ent->usermodel)+10,TAG_LEVEL);
		if (strstr(ent->usermodel,".sp2"))
			sprintf(buffer, "sprites/%s", ent->usermodel);
		else
			sprintf(buffer, "models/%s", ent->usermodel);
		ent->usermodel = buffer;

		if (st.gravity)
			ent->gravity = atof(st.gravity);
		else
			ent->gravity = 0.;
	}
	else
	{
		ent->gravity = 0.;
		ent->attenuation = 0.;
	}

	// If not rain or "user", turn off splash. Yeah I know goofy mapper
	// might WANT splash, but we're enforcing good taste here :)
	if (ent->style > STYLE_WEATHER_BIGRAIN && ent->style != STYLE_WEATHER_USER)
		ent->spawnflags &= ~SF_WEATHER_SPLASH;

	ent->use = target_precipitation_use;
	
	if (!ent->count)
		ent->count = 1;

	if (!ent->sounds)
		ent->sounds = 2;	// blue splash

	if (!ent->mass2)
		ent->mass2 = 8;		// 8 particles in splash

	if ((ent->style < STYLE_WEATHER_RAIN) || (ent->style > STYLE_WEATHER_USER))
		ent->style = STYLE_WEATHER_RAIN;		// single rain drop model

	if (ent->speed <= 0)
	{
		switch(ent->style)
		{
		case STYLE_WEATHER_SNOW: ent->speed = 50; break;
		case STYLE_WEATHER_LEAF: ent->speed = 50; break;
		default: ent->speed = 300;
		}
	}

	if ((VectorLength(ent->bleft) == 0.) && (VectorLength(ent->tright) == 0.))
	{
		// Default distribution places raindrops vertically for
		// full coverage, to help avoid "lumps"
		VectorSet(ent->bleft,-512,-512, -ent->speed*0.05);
		VectorSet(ent->tright,512, 512,  ent->speed*0.05);
	}

	if (VectorLength(ent->s.angles) > 0)
		G_SetMovedir(ent->s.angles,ent->movedir);
	else
		VectorSet(ent->movedir,0,0,-1);

	ent->density = ent->count;

	gi.linkentity (ent);
}

//=============================================================================
// TARGET_FOUNTAIN is identical to TARGET_PRECIPITATION, with these exceptions:
// ALL styles are "user-defined" (no predefined rain, snow, etc.)
// Models are spawned from a point source, and bleft/tright form a box within
// which the target point is found.
//=============================================================================
void target_fountain_think (edict_t *self)
{
	vec3_t	center;
	vec3_t	org;
	vec3_t	dir;
	int		r, i;
	float	u, v, z;
	float	temp;
	qboolean	can_see_me;

	if (!(self->spawnflags & SF_WEATHER_FIRE_ONCE))
		self->nextthink = level.time + FRAMETIME;

	// Don't start raining until player is in the game. The following 
	// takes care of both initial map load conditions and restored saved games.
	// This is a gross abuse of groundentity_linkcount. Sue me.
	if (g_edicts[1].linkcount == self->groundentity_linkcount)
		return;
	else
		self->groundentity_linkcount = g_edicts[1].linkcount;

	// Don't spawn drops if player can't see us. This SEEMS like an obvious
	// thing to do, but can cause visual problems if mapper isn't careful.
	// For example, placing target_precipitation where it isn't in the PVS
	// of the player's current position, but the result (rain) IS in the
	// PVS. In any case, this step is necessary to prevent overflows when
	// player suddenly encounters rain.

	can_see_me = false;
	for (i=1; i<=game.maxclients && !can_see_me; i++)
	{
		if (!g_edicts[i].inuse) continue;
		if (gi.inPVS(g_edicts[i].s.origin, self->s.origin))
			can_see_me = true;
	}
	if (!can_see_me) return;

	// Count is models/second. We accumulate a probability of a model
	// falling this frame in ->density. Yeah its a misnomer but density isn't 	// used for anything else so it works fine.

	temp = 0.1 * (self->density + crandom()*self->random);
	r = (int)(temp);
	if (r > 0)
		self->density = self->count;
	else
		self->density += (temp * 10);
	if (r < 1) return;

	VectorAdd(self->bleft, self->tright, center);
	VectorMA(self->s.origin, 0.5, center, center);

	for (i=0; i<r; i++)
	{
		u = crandom() * (self->tright[0] - self->bleft[0])/2;
		v = crandom() * (self->tright[1] - self->bleft[1])/2;
		z = crandom() * (self->tright[2] - self->bleft[2])/2;
		
		VectorCopy(center, org);
		
		org[0] += u;
		org[1] += v;
		org[2] += z;
		VectorSubtract(org,self->s.origin,dir);
		VectorNormalize(dir);

		spawn_precipitation(self, self->s.origin, dir, self->speed);
	}
}

void target_fountain_use (edict_t *ent, edict_t *other, edict_t *activator)
{
	if ((ent->spawnflags & SF_WEATHER_STARTON) && !(ent->spawnflags & SF_WEATHER_FIRE_ONCE))
	{
		// already on; turn it off
		ent->nextthink = 0;
		ent->spawnflags &= ~SF_WEATHER_STARTON;
		if (ent->child)
		{
			edict_t	*child, *parent;
			child = ent->child;
			ent->child = NULL;
			while (child)
			{
				parent = child;
				child = parent->child;
				G_FreeEdict(parent);
			}
		}
	}
	else
	{
		ent->density = ent->count;
		ent->think = target_fountain_think;
		ent->spawnflags |= SF_WEATHER_STARTON;
		ent->think(ent);
	}
}

void target_fountain_delayed_use (edict_t *self)
{
	// Since target_fountain tends to be a processor hog,
	// for START_ON we wait until the player has spawned into the
	// game to ease the startup burden somewhat
	if (g_edicts[1].linkcount)
	{
		self->think = target_fountain_think;
		self->think(self);
	}
	else
		self->nextthink = level.time + FRAMETIME;
}
void SP_target_fountain (edict_t *ent)
{
	char	*buffer;

	if (deathmatch->value || coop->value)
	{
		G_FreeEdict(ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;

	if (ent->spawnflags & SF_WEATHER_STARTON)
	{
		ent->think = target_fountain_delayed_use;
		ent->nextthink = level.time + 1;
	}

	ent->style = STYLE_WEATHER_USER;
	if (!ent->usermodel)
	{
		gi.dprintf("target_fountain with no usermodel.\n");
		G_FreeEdict(ent);
		return;
	}
	buffer = gi.TagMalloc((int)strlen(ent->usermodel)+10, TAG_LEVEL);
	if (strstr(ent->usermodel,".sp2"))
		sprintf(buffer, "sprites/%s", ent->usermodel);
	else
		sprintf(buffer, "models/%s", ent->usermodel);
	ent->usermodel = buffer;

	if (st.gravity)
		ent->gravity = atof(st.gravity);
	else
		ent->gravity = 0.;

//CW+++ Setup animation info if flagged to do so.
	if (ent->spawnflags & SF_FOUNTAIN_ANIM)
	{
		ent->s.modelindex = gi.modelindex(buffer);

		if (!ent->framenumbers)
			ent->framenumbers = 1;

		ent->framenumbers += ent->startframe;
		ent->s.frame = ent->startframe;
	}
//CW---

	ent->use = target_fountain_use;
	
	if (!ent->count)
		ent->count = 1;

	if (!ent->sounds)
		ent->sounds = 2;	// blue splash

	if (!ent->mass2)
		ent->mass2 = 8;		// 8 particles in splash

	if (ent->speed <= 0)
		ent->speed = 300;

	if ((VectorLength(ent->bleft) == 0.) && (VectorLength(ent->tright) == 0.))
	{
		// Default distribution places raindrops vertically for
		// full coverage, to help avoid "lumps"
		VectorSet(ent->bleft,-32, -32, 64);
		VectorSet(ent->tright,32,  32,128);
	}

	ent->density = ent->count;

	gi.linkentity(ent);
}
//
/*=============================================================================

MISC_DEADSOLDIER MODEL PATCH

==============================================================================*/

#define NUM_SKINS		16
#define MAX_SKINNAME	64
#define DEADSOLDIER_MODEL "models/deadbods/dude/tris.md2"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#ifndef _mkdir
#define _mkdir(s) mkdir(s, 0777)
#endif
#endif
#include "pak.h"

int PatchDeadSoldier (void)
{
	cvar_t		*gamedir;
	char		skins[NUM_SKINS][MAX_SKINNAME];	// skin entries
	char		infilename[MAX_OSPATH];
	char		outfilename[MAX_OSPATH];
	int			j;
	char		*p;
	FILE		*infile;
	FILE		*outfile;
	dmdl_t		model;				// model header
	byte		*data;				// model data
	int			datasize = 0;		// model data size (bytes)
	int			newoffset;			// model data offset (after skins)

	// get game (moddir) name
	gamedir = gi.cvar("game", "", 0);
	if (!*gamedir->string)
		return 0;	// we're in baseq2

	sprintf (outfilename, "%s/%s", gamedir->string,DEADSOLDIER_MODEL);
	if ((outfile = fopen (outfilename, "rb")))
	{
		// output file already exists, move along
		fclose (outfile);
		return 0;
	}

	for (j = 0; j < NUM_SKINS; j++)
		memset (skins[j], 0, MAX_SKINNAME);

	sprintf (skins[0],  "models/deadbods/dude/dead1.pcx");
	sprintf (skins[1],	"players/male/cipher.pcx");
	sprintf (skins[2],	"players/male/claymore.pcx");
	sprintf (skins[3],	"players/male/flak.pcx");
	sprintf (skins[4],	"players/male/grunt.pcx");
	sprintf (skins[5],	"players/male/howitzer.pcx");
	sprintf (skins[6],	"players/male/major.pcx");
	sprintf (skins[7],	"players/male/nightops.pcx");
	sprintf (skins[8],	"players/male/pointman.pcx");
	sprintf (skins[9],	"players/male/psycho.pcx");
	sprintf (skins[10],	"players/male/rampage.pcx");
	sprintf (skins[11], "players/male/razor.pcx");
	sprintf (skins[12], "players/male/recon.pcx");
	sprintf (skins[13], "players/male/scout.pcx");
	sprintf (skins[14], "players/male/sniper.pcx");
	sprintf (skins[15], "players/male/viper.pcx");


	// load original model
	sprintf (infilename, "baseq2/%s", DEADSOLDIER_MODEL);
	if ( !(infile = fopen (infilename, "rb")) )
	{
		// If file doesn't exist on user's hard disk, it must be in 
		// pak0.pak

		pak_header_t	pakheader;
		pak_item_t		pakitem;
		FILE			*fpak;
		int				k, numitems;

		fpak = fopen("baseq2/pak0.pak","rb");
		if (!fpak)
		{
			cvar_t	*cddir;
			char	pakfile[MAX_OSPATH];

			cddir = gi.cvar("cddir", "", 0);
			sprintf(pakfile,"%s/baseq2/pak0.pak",cddir->string);
			fpak = fopen(pakfile,"rb");
			if (!fpak)
			{
				gi.dprintf("PatchDeadSoldier: Cannot find pak0.pak\n");
				return 0;
			}
		}
		fread(&pakheader,1,sizeof(pak_header_t),fpak);
		numitems = pakheader.dsize/sizeof(pak_item_t);
		fseek(fpak,pakheader.dstart,SEEK_SET);
		data = NULL;
		for(k=0; k<numitems && !data; k++)
		{
			fread(&pakitem,1,sizeof(pak_item_t),fpak);
			if (!strcmp(pakitem.name,DEADSOLDIER_MODEL))
			{
				fseek(fpak,pakitem.start,SEEK_SET);
				fread(&model, sizeof(dmdl_t), 1, fpak);
				datasize = model.ofs_end - model.ofs_skins;
				if ( !(data = malloc (datasize)) )	// make sure freed locally
				{
					fclose(fpak);
					gi.dprintf ("PatchDeadSoldier: Could not allocate memory for model\n");
					return 0;
				}
				fread (data, sizeof (byte), datasize, fpak);
			}
		}
		fclose(fpak);
		if (!data)
		{
			gi.dprintf("PatchDeadSoldier: Could not find %s in baseq2/pak0.pak\n",DEADSOLDIER_MODEL);
			return 0;
		}
	}
	else
	{
		fread (&model, sizeof (dmdl_t), 1, infile);
	
		datasize = model.ofs_end - model.ofs_skins;
		if ( !(data = malloc (datasize)) )	// make sure freed locally
		{
			gi.dprintf ("PatchMonsterModel: Could not allocate memory for model\n");
			return 0;
		}
		fread (data, sizeof (byte), datasize, infile);
	
		fclose (infile);
	}
	
	// update model info
	model.num_skins = NUM_SKINS;
	
	// Already had 1 skin, so new offset doesn't include that one
	newoffset = (model.num_skins-1) * MAX_SKINNAME;
	model.ofs_st     += newoffset;
	model.ofs_tris   += newoffset;
	model.ofs_frames += newoffset;
	model.ofs_glcmds += newoffset;
	model.ofs_end    += newoffset;
	
	// save new model
	sprintf (outfilename, "%s/models", gamedir->string);	// make some dirs if needed
	_mkdir (outfilename);
	strcat (outfilename,"/deadbods");
	_mkdir (outfilename);
	strcat (outfilename,"/dude");
	_mkdir (outfilename);
	sprintf (outfilename, "%s/%s", gamedir->string, DEADSOLDIER_MODEL);
	p = strstr(outfilename,"/tris.md2");
	*p = 0;
	_mkdir (outfilename);

	sprintf (outfilename, "%s/%s", gamedir->string, DEADSOLDIER_MODEL);
	
	if ( !(outfile = fopen (outfilename, "wb")) )
	{
		// file couldn't be created for some other reason
		gi.dprintf ("PatchDeadSoldier: Could not save %s\n", outfilename);
		free (data);
		return 0;
	}
	
	fwrite (&model, sizeof (dmdl_t), 1, outfile);
	fwrite (skins, sizeof (char), model.num_skins*MAX_SKINNAME, outfile);
	data += MAX_SKINNAME;
	fwrite (data, sizeof (byte), datasize, outfile);
	
	fclose (outfile);
	gi.dprintf ("PatchDeadSoldier: Saved %s\n", outfilename);
	free (data);
	return 1;
}

void PrecacheDebris(int type)
{
	int	i;

	switch(type)
	{
	case 0:
		gi.modelindex ("models/objects/debris1/tris.md2");
		gi.modelindex ("models/objects/debris2/tris.md2");
		gi.modelindex ("models/objects/debris3/tris.md2");
		break;
	case GIB_METAL:
		for(i=1;i<=5;i++)
			gi.modelindex(va("models/objects/metal_gibs/gib%i.md2",i));
		break;
	case GIB_GLASS:
		for(i=1;i<=5;i++)
			gi.modelindex(va("models/objects/glass_gibs/gib%i.md2",i));
		break;
	case GIB_BARREL:
		for(i=1;i<=5;i++)
			gi.modelindex(va("models/objects/barrel_gibs/gib%i.md2",i));
		break;
	case GIB_CRATE:
		for(i=1;i<=5;i++)
			gi.modelindex(va("models/objects/crate_gibs/gib%i.md2",i));
		break;
	case GIB_ROCK:
		for(i=1;i<=5;i++)
			gi.modelindex(va("models/objects/rock_gibs/gib%i.md2",i));
		break;
	case GIB_CRYSTAL:
		for(i=1;i<=5;i++)
			gi.modelindex(va("models/objects/crystal_gibs/gib%i.md2",i));
		break;
	case GIB_MECH:
		for(i=1;i<=5;i++)
			gi.modelindex(va("models/objects/mech_gibs/gib%i.md2",i));
		break;
	case GIB_WOOD:
		for(i=1;i<=5;i++)
			gi.modelindex(va("models/objects/wood_gibs/gib%i.md2",i));
		break;
	case GIB_TECH:
		for(i=1;i<=5;i++)
			gi.modelindex(va("models/objects/tech_gibs/gib%i.md2",i));
		break;
	}
}
