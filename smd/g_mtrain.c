#include "g_local.h"

void func_train_find (edict_t *self);
void SP_model_spawn (edict_t *ent);
void train_blocked (edict_t *self, edict_t *other);
void train_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);
void train_use (edict_t *self, edict_t *other, edict_t *activator);

#define TRAIN_START_ON		     1
#define TRAIN_TOGGLE		     2
#define TRAIN_BLOCK_STOPS	     4
#define	PLAYER_MODEL             8
#define	NO_MODEL		        16
#define MTRAIN_ROTATE           32
#define MTRAIN_ROTATE_CONSTANT  64
#define TRAIN_SMOOTH           128

void model_train_animator(edict_t *animator)
{
	edict_t	*train;

	train = animator->owner;
	if(!train || !train->inuse)
	{
		G_FreeEdict(animator);
		return;
	}
	if(Q_stricmp(train->classname,"model_train"))
	{
		G_FreeEdict(animator);
		return;
	}
	animator->nextthink = level.time + FRAMETIME;
	if(VectorLength(train->velocity) == 0)
		return;
	train->s.frame++;
	if (train->s.frame >= train->framenumbers)
		train->s.frame = train->startframe;
	gi.linkentity(train);
}

void SP_model_train (edict_t *self)
{
	SP_model_spawn (self);
	// Reset s.sound, which SP_model_spawn may have turned on
	self->moveinfo.sound_middle = self->s.sound;
	self->s.sound = 0;

	if(!self->inuse) return;

	// Reset some things from SP_model_spawn
	self->delay = 0;
	self->think = NULL;
	self->nextthink = 0;
	if (self->health)
	{
		self->die = train_die;
		self->takedamage = DAMAGE_YES;
	}

	if(self->framenumbers > self->startframe+1)
	{
		edict_t	*animator;
		animator = G_Spawn();
		animator->owner = self;
		animator->think = model_train_animator;
		animator->nextthink = level.time + FRAMETIME;
	}
	self->s.frame = self->startframe;
	self->movetype = MOVETYPE_PUSH;

	// Really gross stuff here... translate model_spawn spawnflags
	// to func_train spawnflags. PLAYER_MODEL and NO_MODEL have
	// already been checked in SP_model_spawn and are never re-used,
	// so it's OK to overwrite those.
	if(self->spawnflags & MTRAIN_ROTATE)
	{
		self->spawnflags &= ~MTRAIN_ROTATE;
		self->spawnflags |= TRAIN_ROTATE;
	}
	if(self->spawnflags & MTRAIN_ROTATE_CONSTANT)
	{
		self->spawnflags &= ~MTRAIN_ROTATE_CONSTANT;
		self->spawnflags |= TRAIN_ROTATE_CONSTANT;
	}
	if( (self->spawnflags & (TRAIN_ROTATE | TRAIN_ROTATE_CONSTANT)) == (TRAIN_ROTATE | TRAIN_ROTATE_CONSTANT))
	{
		self->spawnflags &= ~(TRAIN_ROTATE | TRAIN_ROTATE_CONSTANT);
		self->spawnflags |= TRAIN_SPLINE;
	}
	if(self->style == 3)
		self->spawnflags |= TRAIN_ANIMATE;		// 32
	if(self->style == 4)
		self->spawnflags |= TRAIN_ANIMATE_FAST;	// 64

	// TRAIN_SMOOTH forces trains to go directly to Move_Done from
	// Move_Final rather than slowing down (if necessary) for one
	// frame.
	if (self->spawnflags & TRAIN_SMOOTH)
		self->smooth_movement = true;
	else
		self->smooth_movement = false;

	self->blocked = train_blocked;
	if (self->spawnflags & TRAIN_BLOCK_STOPS)
		self->dmg = 0;
	else
	{
		if (!self->dmg)
			self->dmg = 100;
	}

	if (!self->speed)
		self->speed = 100;

	self->moveinfo.speed = self->speed;
	self->moveinfo.accel = self->moveinfo.decel = self->moveinfo.speed;

	self->use = train_use;

	gi.linkentity (self);

	if (self->target)
	{
		// start trains on the second frame, to make sure their targets have had
		// a chance to spawn
		self->nextthink = level.time + FRAMETIME;
		self->think = func_train_find;
	}
	else
	{
		gi.dprintf ("model_train without a target at %s\n", vtos(self->absmin));
	}

}
