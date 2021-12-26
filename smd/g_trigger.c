#include "g_local.h"

#define TRIGGER_MONSTER      1
#define TRIGGER_NOT_PLAYER   2
#define TRIGGER_START_OFF    4
#define TRIGGER_NEED_USE     8
#define TRIGGER_CAMOWNER    16
#define TRIGGER_LOOKTARGET	32

//CW+++ For trigger toggles.
#define	TOGGLE_DELAY		16
#define TOGGLE_MESSAGE		32
#define TOGGLE_TARGET		64
//CW---

void InitTrigger(edict_t *self)
{
	if (!VectorCompare(self->s.angles, vec3_origin))
		G_SetMovedir(self->s.angles, self->movedir);

	self->solid = SOLID_TRIGGER;
	self->movetype = MOVETYPE_NONE;
	gi.setmodel(self, self->model);
	self->svflags = SVF_NOCLIENT;
}


// the wait time has passed, so set back up for another activation
void multi_wait (edict_t *ent)
{
	ent->nextthink = 0;
}


// the trigger was just activated
// ent->activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void multi_trigger (edict_t *ent)
{
	float	fTemp;		//CW
	char	*sTemp;		//CW

	if (ent->nextthink)
		return;		// already been triggered

	G_UseTargets (ent, ent->activator);

//CW+++ Toggle between two delay/message/target values.
	if (ent->spawnflags & TOGGLE_DELAY)
	{
		fTemp = ent->delay;
		ent->delay = ent->roll;
		ent->roll = fTemp;
	}

	if (ent->spawnflags & TOGGLE_MESSAGE)
	{
		sTemp = ent->message;
		ent->message = ent->viewmessage;
		ent->viewmessage = sTemp;
	}

	if (ent->spawnflags & TOGGLE_TARGET)
	{
		sTemp = ent->newtargetname;
		ent->target = ent->newtargetname;
		ent->newtargetname = sTemp;
	}
//CW---

	if (ent->wait > 0)	
	{
		ent->think = multi_wait;
		ent->nextthink = level.time + ent->wait;
	}
	else
	{	// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		ent->touch = NULL;
		ent->nextthink = level.time + FRAMETIME;
		ent->think = G_FreeEdict;
	}
}

void Use_Multi (edict_t *ent, edict_t *other, edict_t *activator)
{
	ent->activator = activator;
	multi_trigger (ent);
}

void Touch_Multi (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if(other->client || (other->flags & FL_ROBOT))
	{
		if (self->spawnflags & TRIGGER_NOT_PLAYER)
			return;
	}
	else if (other->svflags & SVF_MONSTER)
	{
		if (!(self->spawnflags & TRIGGER_MONSTER))
			return;
	}
	else
		return;

	if( (self->spawnflags & TRIGGER_CAMOWNER) && (!other->client || !other->client->spycam))
		return;

	if (!VectorCompare(self->movedir, vec3_origin))
	{
		vec3_t	forward;

		AngleVectors(other->s.angles, forward, NULL, NULL);
		if (_DotProduct(forward, self->movedir) < 0)
			return;
	}

	self->activator = other;
	multi_trigger (self);
}

/*QUAKED trigger_multiple (.5 .5 .5) ? MONSTER NOT_PLAYER TRIGGERED
Variable sized repeatable trigger.  Must be targeted at one or more entities.

If "delay" is set, the trigger waits some time after activating before firing.
"wait" : seconds between triggerings. (.2 default)

//CW+++
It can be set to toggle between two different values for the following spawnflags:
16 = message (with viewmessage)
32 = delay   (with roll)
64 = target  (with newtargetname)

"noise : play a user-defined WAV file (NB: overrides the 'sounds' setting)
"sounds" :
0)  beep beep
1)	secret
2)	F1 talk
3)	large switch
4)  key try
5)  key use
6)  silent
//CW---

set "message" to text string
*/
void trigger_enable (edict_t *self, edict_t *other, edict_t *activator)
{
	self->solid = SOLID_TRIGGER;
	self->use = Use_Multi;
	gi.linkentity (self);
}

void SP_trigger_multiple (edict_t *ent)
{
//CW+++ Extended preset sound list; allow user to specify their own sounds if desired.
	if (st.noise)
		ent->noise_index = gi.soundindex(st.noise);
	else
	{
		if (ent->sounds == 1)
			ent->noise_index = gi.soundindex ("misc/secret.wav");
		else if (ent->sounds == 2)
			ent->noise_index = gi.soundindex ("misc/talk.wav");
		else if (ent->sounds == 3)
			ent->noise_index = gi.soundindex ("switches/butn2.wav");
		else if (ent->sounds == 4)
			ent->noise_index = gi.soundindex ("misc/keytry.wav");
		else if (ent->sounds == 5)
			ent->noise_index = gi.soundindex ("misc/keyuse.wav");
		else if (ent->sounds == 6)
			ent->noise_index = -1;
	}
//CW---
	
	if (!ent->wait)
		ent->wait = 0.2;
	ent->touch = Touch_Multi;
	ent->movetype = MOVETYPE_NONE;
	ent->svflags |= SVF_NOCLIENT;

	if (ent->spawnflags & TRIGGER_CAMOWNER)
		ent->svflags |= SVF_TRIGGER_CAMOWNER;

	if (ent->spawnflags & TRIGGER_START_OFF)
	{
		ent->solid = SOLID_NOT;
		ent->use = trigger_enable;
	}
	else
	{
		ent->solid = SOLID_TRIGGER;
		ent->use = Use_Multi;
	}

	if (!VectorCompare(ent->s.angles, vec3_origin))
		G_SetMovedir (ent->s.angles, ent->movedir);

	gi.setmodel (ent, ent->model);
	gi.linkentity (ent);
}


/*QUAKED trigger_once (.5 .5 .5) ? x x TRIGGERED
Triggers once, then removes itself.
You must set the key "target" to the name of another object in the level that has a matching "targetname".
If TRIGGERED, this trigger must be triggered before it is live.
"sounds" :
0)  beep beep
1)	secret
2)	F1 talk
3)	large switch
4)  key try
5)  key use
6)  silent
"message"	string to be displayed when triggered
*/
void SP_trigger_once(edict_t *ent)
{
	// make old maps work because I messed up on flag assignments here
	// triggered was on bit 1 when it should have been on bit 4
	if (ent->spawnflags & 1)
	{
		vec3_t	v;

		VectorMA (ent->mins, 0.5, ent->size, v);
		ent->spawnflags &= ~1;
		ent->spawnflags |= TRIGGER_START_OFF;
		gi.dprintf("fixed TRIGGERED flag on %s at %s\n", ent->classname, vtos(v));
	}

	ent->wait = -1;
	SP_trigger_multiple (ent);
}

/*QUAKED trigger_relay (.5 .5 .5) (-8 -8 -8) (8 8 8)
This fixed size trigger cannot be touched, it can only be fired by other events.

//CW+++
It can be set to toggle between two different values for the following spawnflags:
16 = message (with viewmessage)
32 = delay   (with roll)
64 = target  (with newtargetname)

"noise : play a user-defined WAV file (NB: overrides the 'sounds' setting)
"sounds" :
0)  beep beep
1)	secret
2)	F1 talk
3)	large switch
4)  key try
5)  key use
6)  silent
//CW---
*/
void trigger_relay_use (edict_t *self, edict_t *other, edict_t *activator)
{
	float	fTemp;		//CW
	char	*sTemp;		//CW

	self->count--;
	if (!self->count)
	{
		self->think = G_FreeEdict;
		self->nextthink = level.time + FRAMETIME;
	}
	G_UseTargets(self, activator);

//CW+++ Toggle between two delay/message/target values.
	if (self->spawnflags & TOGGLE_DELAY)
	{
		fTemp = self->delay;
		self->delay = self->roll;
		self->roll = fTemp;
	}

	if (self->spawnflags & TOGGLE_MESSAGE)
	{
		sTemp = self->message;
		self->message = self->viewmessage;
		self->viewmessage = sTemp;
	}

	if (self->spawnflags & TOGGLE_TARGET)
	{
		sTemp = self->newtargetname;
		self->target = self->newtargetname;
		self->newtargetname = sTemp;
	}
//CW---
}

void SP_trigger_relay (edict_t *self)
{
// DWH - gives trigger_relay same message-displaying, sound-playing capabilities
//       as trigger_multiple and trigger_once

//CW+++ Extended preset sound list; allow user to specify their own sounds if desired.
	if (st.noise)
		self->noise_index = gi.soundindex(st.noise);
	else
	{
		if (self->sounds < 1)
			self->noise_index = 0;
		else if (self->sounds == 1)
			self->noise_index = gi.soundindex ("misc/secret.wav");
		else if (self->sounds == 2)
			self->noise_index = gi.soundindex ("misc/talk.wav");
		else if (self->sounds == 3)
			self->noise_index = gi.soundindex ("switches/butn2.wav");
		else if (self->sounds == 4)
			self->noise_index = gi.soundindex ("misc/keytry.wav");
		else if (self->sounds == 5)
			self->noise_index = gi.soundindex ("misc/keyuse.wav");
		else if (self->sounds == 6)
			self->noise_index = -1;
	}
//CW---
	
	if(!self->count) self->count = -1;
// end DWH

	self->use = trigger_relay_use;
}

/*
==============================================================================

trigger_key

Lazarus editions:
Spawnflags
1 = Multi-use. If set, item is required EVERY time trigger_key is targeted
2 = Keep key. If set, player doesn't give up the key when trigger_key is used.
4 = Silent. If set, neither the "You need" message and sound nor keyuse.wav
    are played. This is useful for trigger_keys used only to remove items from
	the player.

Lazarus also removes non-multi-use trigger_keys once used, to free up space in
the edicts array.

==============================================================================
*/

/*QUAKED trigger_key (.5 .5 .5) (-8 -8 -8) (8 8 8)
A relay trigger that only fires it's targets if player has the proper key.
Use "item" to specify the required key, for example "key_data_cd"
*/
void trigger_key_use (edict_t *self, edict_t *other, edict_t *activator)
{
	int			index;

	if (!self->item)
		return;
	if (!activator->client)
		return;

	index = ITEM_INDEX(self->item);
	if (!activator->client->pers.inventory[index])
	{
		if (level.time < self->touch_debounce_time)
			return;
		self->touch_debounce_time = level.time + 5.0;
		if(!(self->spawnflags & 4))
		{
			if (self->key_message)
				gi.centerprintf (activator, self->key_message);		//CW: display customised key_message if set
			else
				gi.centerprintf (activator, "You need the %s", self->item->pickup_name);
			gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/keytry.wav"), 1, ATTN_NORM, 0);
		}
		return;
	}

	if(!(self->spawnflags & 4))
		gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/keyuse.wav"), 1, ATTN_NORM, 0);
	if (coop->value)
	{
		int		player;
		edict_t	*ent;

		if (strcmp(self->item->classname, "key_power_cube") == 0)
		{
			int	cube;

			for (cube = 0; cube < 8; cube++)
				if (activator->client->pers.power_cubes & (1 << cube))
					break;
			for (player = 1; player <= game.maxclients; player++)
			{
				ent = &g_edicts[player];
				if (!ent->inuse)
					continue;
				if (!ent->client)
					continue;
				// DWH: keep key
				if (!(self->spawnflags & 2)) {
					if (ent->client->pers.power_cubes & (1 << cube))
					{
						ent->client->pers.inventory[index]--;
						ent->client->pers.power_cubes &= ~(1 << cube);
					}
				}
			}
		}
		else
		{
			for (player = 1; player <= game.maxclients; player++)
			{
				ent = &g_edicts[player];
				if (!ent->inuse)
					continue;
				if (!ent->client)
					continue;
				// DWH: keep key
				if(!(self->spawnflags & 2))
					ent->client->pers.inventory[index] = 0;
			}
		}
	}
	// DWH: keep key
	else if(!(self->spawnflags & 2))
	{
		activator->client->pers.inventory[index]--;
	}

	G_UseTargets (self, activator);

	// DWH - multi-use
	if(!(self->spawnflags & 1)) {
		self->use       = NULL;
		self->think     = G_FreeEdict;
		self->nextthink = level.time + FRAMETIME;
		gi.linkentity(self);
	}
}

void SP_trigger_key (edict_t *self)
{
	if (!st.item)
	{
		gi.dprintf("no key item for trigger_key at %s\n", vtos(self->s.origin));
		return;
	}
	self->item = FindItemByClassname (st.item);

	if (!self->item)
	{
		gi.dprintf("item %s not found for trigger_key at %s\n", st.item, vtos(self->s.origin));
		return;
	}

	if (!self->target)
	{
		gi.dprintf("%s at %s has no target\n", self->classname, vtos(self->s.origin));
		return;
	}

	gi.soundindex ("misc/keytry.wav");
	gi.soundindex ("misc/keyuse.wav");

	self->use = trigger_key_use;
}


/*
==============================================================================

trigger_counter

==============================================================================
*/

/*QUAKED trigger_counter (.5 .5 .5) ? nomessage
Acts as an intermediary for an action that takes multiple inputs.

If nomessage is not set, t will print "1 more.. " etc when triggered and "sequence complete" when finished.

After the counter has been triggered "count" times (default 2), it will fire all of it's targets and remove itself.
*/

void trigger_counter_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->count == 0) {
		G_FreeEdict(self);  // DWH
		return;
	}
	
	self->count--;

	if (self->count)
	{
		if (! (self->spawnflags & 1))
		{
			gi.centerprintf(activator, "%i more to go...", self->count);
			gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
		}
		return;
	}
	
	if (! (self->spawnflags & 1))
	{
		gi.centerprintf(activator, "Sequence completed!");
		gi.sound (activator, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}
	self->activator = activator;
	multi_trigger (self);
	// DWH
	if (self->count == 0) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}
}

void SP_trigger_counter (edict_t *self)
{
	self->wait = -1;
	if (!self->count)
		self->count = 2;

	self->use = trigger_counter_use;
}


/*
==============================================================================

trigger_always

==============================================================================
*/

/*QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
This trigger will always fire.  It is activated by the world.
*/
void SP_trigger_always (edict_t *ent)
{
	// we must have some delay to make sure our use targets are present
	if (ent->delay < 0.2)
		ent->delay = 0.2;
	G_UseTargets(ent, ent);
}


/*
==============================================================================

trigger_push

==============================================================================
*/

#define PUSH_ONCE		1

static int windsound;

void trigger_push_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (strcmp(other->classname, "grenade") == 0)
	{
		VectorScale (self->movedir, self->speed * 10, other->velocity);
	}
// Lazarus
	else if (other->movetype == MOVETYPE_PUSHABLE)
	{
		vec3_t v;
		VectorScale (self->movedir, self->speed * 2000 / (float)(other->mass), v); 
		VectorAdd(other->velocity,v,other->velocity);
	}
	else if (other->health > 0)
	{
		VectorScale (self->movedir, self->speed * 10, other->velocity);
		if (other->client)
		{
			// don't take falling damage immediately from this
			VectorCopy (other->velocity, other->client->oldvelocity);
			if (other->fly_sound_debounce_time < level.time)
			{
				other->fly_sound_debounce_time = level.time + 1.5;
				if(self->spawnflags & 2) {
					if(self->noise_index)
						gi.sound(other, CHAN_AUTO, self->noise_index, 1, ATTN_NORM, 0);
				} else
					gi.sound (other, CHAN_AUTO, windsound, 1, ATTN_NORM, 0);
			}
		}
	}
	if (self->spawnflags & PUSH_ONCE)
		G_FreeEdict (self);
}


/*QUAKED trigger_push (.5 .5 .5) ? PUSH_ONCE CUSTOM_SOUND
Pushes the player
"speed"		defaults to 1000
*/
void SP_trigger_push (edict_t *self)
{
	InitTrigger (self);
	// DWH: Custom (or no) sound
	if(self->spawnflags & 2) {
		if(st.noise)
			self->noise_index = gi.soundindex(st.noise);
		else
			self->noise_index = 0;
	} else {
		windsound = gi.soundindex ("misc/windfly.wav");
	}
	self->touch = trigger_push_touch;
	if (!self->speed)
		self->speed = 1000;

	gi.linkentity (self);
}

//CW+++
/*
==============================================================================

trigger_medibot

==============================================================================
*/

/*QUAKED trigger_medibot (.5 .5 .5) ? START_OFF TOGGLE SILENT SLOW
Any entity that touches this will be healed, until the charge runs out.

It does 'health' points of healing each server frame
"health"	default 1 (whole numbers only)
"count"		charge
*/

#define SF_MEDI_START_OFF		1
#define SF_MEDI_TOGGLE          2	// can be toggled on/off
#define SF_MEDI_SILENT			4	// supresses playing the sound
#define SF_MEDI_SLOW			8	// changes the healing rate to once per second
#define SF_MEDI_SUPERHEAL		16	// heals beyond the player's max_health

void medibot_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);

void medibot_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->solid == SOLID_NOT) self->solid = SOLID_TRIGGER;
	else self->solid = SOLID_NOT;

	gi.linkentity (self);
	if (!(self->spawnflags & SF_MEDI_TOGGLE)) self->use = NULL;
}

void medibot_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (self->message)
	{
		gi.centerprintf (other, "%s", self->message);
		gi.sound (other, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0);
		self->message = NULL;
	}
	
	if (self->target)
	{
		G_UseTargets(self, other);
		self->target = NULL;
		if (self->killtarget) self->killtarget = NULL;
		gi.linkentity(self);
	}

	if (self->timestamp > level.time) return;

	if (self->count <= 0) return;

	if (self->spawnflags & SF_MEDI_SLOW) self->timestamp = level.time + 1;
	else self->timestamp = level.time + FRAMETIME;

	if ((other->health >= other->max_health) && !(self->spawnflags & SF_MEDI_SUPERHEAL)) return;

	other->health += self->health;
	if ((other->health > other->max_health) && !(self->spawnflags & SF_MEDI_SUPERHEAL))
		other->health = other->max_health;

	if (!(self->spawnflags & SF_MEDI_SILENT))
	{
		if (((level.framenum % 10) == 0 ) || (self->spawnflags & SF_MEDI_SLOW))
			gi.sound(other, CHAN_AUTO, self->noise_index, 1, ATTN_NORM, 0);
	}

	if (--self->count == 0)
	{
		self->activator = other;
		if (self->deathtarget) self->target = self->deathtarget;
		multi_trigger(self);
	}
}

void SP_trigger_medibot(edict_t *self)
{
	InitTrigger(self);
	self->touch = medibot_touch;
	self->noise_index = gi.soundindex("items/s_health.wav");
	if (!self->health) self->health = 1;
	
	if (!self->count) self->count = 50;

	if (self->spawnflags & SF_MEDI_START_OFF)
	{
		if (self->targetname == NULL) 
		{
			gi.dprintf("trigger_medibot at %s flagged to start off but has no targetname (will start on)\n", vtos(self->s.origin));
			self->solid = SOLID_TRIGGER;
		}
		else self->solid = SOLID_NOT;
	}
	else self->solid = SOLID_TRIGGER;

	if (self->spawnflags & SF_MEDI_TOGGLE)
	{
		if (self->targetname == NULL) gi.dprintf("switchable trigger_medibot at %s has no targetname\n", vtos(self->s.origin));
		self->use = medibot_use;
	}
	gi.linkentity(self);
}


/*
==============================================================================
trigger_command
==============================================================================
*/
void trigger_cmd_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (self->touch_debounce_time > level.time)
		return;

	stuffcmd(other, self->viewmessage);
	self->touch_debounce_time = level.time + self->wait;
}

void SP_trigger_command(edict_t *self)
{
	InitTrigger(self);
	self->touch = trigger_cmd_touch;
	if (self->wait < FRAMETIME)
		self->wait = FRAMETIME;

	gi.linkentity(self);
}
//CW---

/*
==============================================================================

trigger_hurt

==============================================================================
*/

/*QUAKED trigger_hurt (.5 .5 .5) ? START_OFF TOGGLE SILENT NO_PROTECTION SLOW
Any entity that touches this will be hurt.

It does dmg points of damage each server frame
"dmg"			default 5 (whole numbers only) */

#define SF_HURT_START_OFF       1
#define SF_HURT_TOGGLE          2
#define SF_HURT_SILENT          4  // supresses playing the sound
#define SF_HURT_NO_PROTECTION   8  // *nothing* stops the damage
#define SF_HURT_SLOW           16  // changes the damage rate to once per second
#define SF_HURT_NOGIB          32  // Lazarus: won't gib entity
#define SF_HURT_ENVIRONMENT    64  // Lazarus: environment suit protects from damage

void hurt_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
void hurt_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->solid == SOLID_NOT)
	{
		int		i, num;
		edict_t	*touch[MAX_EDICTS], *hurtme;

		self->solid = SOLID_TRIGGER;
		// Lazaurs: Add check for non-moving (i.e. idle monsters) within trigger_hurt
		//          at first activation
		num = gi.BoxEdicts (self->absmin, self->absmax, touch, MAX_EDICTS, AREA_SOLID);
		for (i=0 ; i<num ; i++)
		{
			hurtme = touch[i];
			hurt_touch (self, hurtme, NULL, NULL);
		}
	}
	else
		self->solid = SOLID_NOT;
	gi.linkentity (self);

	if (!(self->spawnflags & SF_HURT_TOGGLE))
		self->use = NULL;
}


void hurt_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int		dflags;

	if (!other->takedamage)
		return;

	if (self->timestamp > level.time)
		return;

	// DWH - don't "heal" other if he's at max_health
	if ( (self->dmg < 0) && (other->health >= other->max_health))
		return;

	if (self->spawnflags & SF_HURT_SLOW)
		self->timestamp = level.time + 1;
	else
		self->timestamp = level.time + FRAMETIME;

	if (!(self->spawnflags & SF_HURT_SILENT))
	{
		// DWH - Original code would fail to play a sound for
		//       SF=16 unless player just HAPPENED to hit
		//       trigger_hurt at framenum = an integral number of 
		//       full seconds.
		if ( ((level.framenum % 10) == 0 ) || (self->spawnflags & SF_HURT_SLOW) )
			gi.sound (other, CHAN_AUTO, self->noise_index, 1, ATTN_NORM, 0);
	}

	if (self->spawnflags & SF_HURT_NO_PROTECTION)
		dflags = DAMAGE_NO_PROTECTION;
	else
		dflags = 0;
	// Lazarus: healing, no gib, and environment suit protection
	if (self->dmg > 0)
	{
		int	damage = self->dmg;

		if(self->spawnflags & SF_HURT_NOGIB)
		{
			if(skill->value > 0)
				damage = min(damage, other->health - other->gib_health - 1);
			else
				damage = min(damage, 2*(other->health - other->gib_health - 1));

			if(damage < 0)
				damage = 0;
		}

		if (other->client && (self->spawnflags & SF_HURT_ENVIRONMENT) && (other->client->enviro_framenum > level.framenum))
			damage = 0;

		if(damage > 0)
			T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, damage, self->dmg, dflags, MOD_TRIGGER_HURT);
	}
	else
	{
		other->health -= self->dmg;
		if(other->health > other->max_health)
			other->health = other->max_health;
	}
}

void SP_trigger_hurt (edict_t *self)
{
	InitTrigger (self);

	self->touch = hurt_touch;

	if (!self->dmg)
		self->dmg = 5;

	// DWH - play different sound for healing
	if (self->dmg > 0)
		self->noise_index = gi.soundindex ("world/electro.wav");
	else
		self->noise_index = gi.soundindex ("items/s_health.wav");

	if (self->spawnflags & SF_HURT_START_OFF)
		self->solid = SOLID_NOT;
	else
		self->solid = SOLID_TRIGGER;

	if (self->spawnflags & SF_HURT_TOGGLE)
		self->use = hurt_use;

	gi.linkentity (self);
}


/*
==============================================================================

trigger_gravity

==============================================================================
*/

/*QUAKED trigger_gravity (.5 .5 .5) ?
Changes the touching entites gravity to
the value of "gravity".  1.0 is standard
gravity for the level.
*/

void trigger_gravity_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	other->gravity = self->gravity;
}

void SP_trigger_gravity (edict_t *self)
{
	if (st.gravity == 0)
	{
		gi.dprintf("trigger_gravity without gravity set at %s\n", vtos(self->s.origin));
		G_FreeEdict  (self);
		return;
	}

	InitTrigger (self);
	self->gravity = atoi(st.gravity);
	self->touch = trigger_gravity_touch;
}


/*
==============================================================================

trigger_monsterjump

==============================================================================
*/

/*QUAKED trigger_monsterjump (.5 .5 .5) ?
Walking monsters that touch this will jump in the direction of the trigger's angle
"speed" default to 200, the speed thrown forward
"height" default to 200, the speed thrown upwards
*/

void trigger_monsterjump_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->flags & (FL_FLY | FL_SWIM) )
		return;
	if (other->svflags & SVF_DEADMONSTER)
		return;
	if ( !(other->svflags & SVF_MONSTER))
		return;

// set XY even if not on ground, so the jump will clear lips
	other->velocity[0] = self->movedir[0] * self->speed;
	other->velocity[1] = self->movedir[1] * self->speed;
	
	if (!other->groundentity)
		return;
	
	other->groundentity = NULL;
	other->velocity[2] = self->movedir[2];
}

void SP_trigger_monsterjump (edict_t *self)
{
	if (!self->speed)
		self->speed = 200;
	if (!st.height)
		st.height = 200;
	if (self->s.angles[YAW] == 0)
		self->s.angles[YAW] = 360;
	InitTrigger (self);
	self->touch = trigger_monsterjump_touch;
	self->movedir[2] = st.height;
}

//===============================================================
// DWH additions
//===============================================================

/*QUAKED tremor_trigger_multiple (.5 .5 .5) ? MONSTER NOT_PLAYER TRIGGERED
Variable sized repeatable trigger.  Must be targeted at one or more entities.
If "delay" is set, the trigger waits some time after activating before firing.
"wait" : Seconds between triggerings. (.2 default)
Same as trigger_multiple, but toggles on/off when targeted.

sounds
1)	secret
2)	beep beep
3)	large switch
4)
set "message" to text string
*/

void tremor_trigger_enable (edict_t *self, edict_t *other, edict_t *activator);
void Use_tremor_Multi (edict_t *self, edict_t *other, edict_t *activator)
{
	self->count--;
	if(!self->count) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}
	else
	{
		self->use = tremor_trigger_enable;
		self->solid = SOLID_NOT;
		gi.linkentity(self);
	}
}

void tremor_trigger_enable (edict_t *self, edict_t *other, edict_t *activator)
{
	self->solid = SOLID_TRIGGER;
	self->use = Use_tremor_Multi;
	gi.linkentity (self);
}

void SP_tremor_trigger_multiple (edict_t *ent)
{
	if (ent->sounds == 1)
		ent->noise_index = gi.soundindex ("misc/secret.wav");
	else if (ent->sounds == 2)
		ent->noise_index = gi.soundindex ("misc/talk.wav");
	else if (ent->sounds == 3)
// DWH - should be silent
//		ent->noise_index = gi.soundindex ("misc/trigger1.wav");
		ent->noise_index = -1;
	
	if (!ent->wait)
		ent->wait = 0.2;
	ent->touch = Touch_Multi;
	ent->movetype = MOVETYPE_NONE;
	ent->svflags |= SVF_NOCLIENT;

	if (ent->spawnflags & TRIGGER_CAMOWNER)
		ent->svflags |= SVF_TRIGGER_CAMOWNER;

	if (ent->spawnflags & TRIGGER_START_OFF)
	{
		ent->solid = SOLID_NOT;
		ent->use = tremor_trigger_enable;
	}
	else
	{
		ent->solid = SOLID_TRIGGER;
		ent->use = Use_tremor_Multi;
	}

	if (!VectorCompare(ent->s.angles, vec3_origin))
		G_SetMovedir (ent->s.angles, ent->movedir);

	gi.setmodel (ent, ent->model);
	gi.linkentity (ent);
}

//=========================================================================================
// TRIGGER_MASS - triggers its targets when touched by any entity with mass >= mass value
//                of trigger
//=========================================================================================
void trigger_mass_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if(other->mass < self->mass) return;
	self->activator = other;
	multi_trigger (self);
}

void SP_trigger_mass (edict_t *self)
{
	// Fires its target if touched by an entity weighing at least
	// self->mass
	if (self->sounds == 1)
		self->noise_index = gi.soundindex ("misc/secret.wav");
	else if (self->sounds == 2)
		self->noise_index = gi.soundindex ("misc/talk.wav");
	else if (self->sounds == 3)
// DWH - should be silent
//		self->noise_index = gi.soundindex ("misc/trigger1.wav");
		self->noise_index = -1;

	if(!self->wait) self->wait = 0.2;
	self->touch = trigger_mass_touch;
	self->movetype = MOVETYPE_NONE;
	self->svflags |= SVF_NOCLIENT;
	if(self->spawnflags & TRIGGER_START_OFF)
	{
		self->solid = SOLID_NOT;
		self->use = trigger_enable;
	}
	else
	{
		self->solid = SOLID_TRIGGER;
		self->use = Use_Multi;
	}
	if(!self->mass)
		self->mass = 100;
	gi.setmodel (self, self->model);
	gi.linkentity (self);
}
//=======================================================================================
// TRIGGER_INSIDE - triggers its targets when the bounding box for its pathtarget is
//                  completely inside the trigger field
//=======================================================================================
void trigger_inside_think (edict_t *self)
{
	int		i, num;
	edict_t	*touch[MAX_EDICTS], *hit;

	num = gi.BoxEdicts (self->absmin, self->absmax, touch, MAX_EDICTS, AREA_SOLID);
	for (i=0 ; i<num ; i++)
	{
		hit = touch[i];
		if (!hit->inuse) continue;
		if (!hit->targetname) continue;
		if (stricmp(self->pathtarget, hit->targetname)) continue;
		// must be COMPLETELY inside
		if (hit->absmin[0] < self->absmin[0]) continue;
		if (hit->absmin[1] < self->absmin[1]) continue;
		if (hit->absmin[2] < self->absmin[2]) continue;
		if (hit->absmax[0] > self->absmax[0]) continue;
		if (hit->absmax[1] > self->absmax[1]) continue;
		if (hit->absmax[2] > self->absmax[2]) continue;
		G_UseTargets (self, hit);
		if (self->wait > 0)	
			self->nextthink = level.time + self->wait;
		else
		{
			self->nextthink = level.time + FRAMETIME;
			self->think = G_FreeEdict;
		}
		gi.linkentity(self);
		return;
	}
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}
void SP_trigger_inside (edict_t *self)
{
	vec3_t v;

	VectorMA (self->mins, 0.5, self->size, v);
	if(!self->target)
	{
		gi.dprintf("trigger_inside with no target at %s.\n",vtos(v));
		G_FreeEdict(self);
		return;
	}
	if(!self->pathtarget)
	{
		gi.dprintf("trigger_inside with no pathtarget at %s.\n",vtos(v));
		G_FreeEdict(self);
		return;
	}
	self->movetype = MOVETYPE_NONE;
	self->svflags  |= SVF_NOCLIENT;
	self->solid    = SOLID_TRIGGER;
	if(!self->wait) self->wait = 0.2;
	gi.setmodel (self,self->model);
	self->think     = trigger_inside_think;
	self->nextthink = level.time + 1.0;
	gi.linkentity(self);
}
//==================================================================================
// TRIGGER_SCALES - coupled with target_characters, displays the weight of all
//                  entities that are "standing on" the trigger.
//==================================================================================
float weight_on_top(edict_t *ent)
{
	float weight;
	int i;
	edict_t *e;
	weight = 0.0;
	for(i=1, e=g_edicts+i; i<globals.num_edicts; i++, e++)
	{
		if(!e->inuse) continue;
		if(e->groundentity == ent)
		{
			weight += e->mass;
			weight += weight_on_top(e);
		}
	}
	return weight;
}

void trigger_scales_think (edict_t *self)
{
	float	f, fx, fy;
	int		i, num;
	int		weight;
	edict_t	*e, *touch[MAX_EDICTS], *hit;

	num = gi.BoxEdicts (self->absmin, self->absmax, touch, MAX_EDICTS, AREA_SOLID);
	weight = 0;
	for (i=0 ; i<num ; i++)
	{
		hit = touch[i];
		if (!hit->inuse) continue;
		if (!hit->mass) continue;
		fx = fy = 0.0;
		if(hit->absmin[0] < self->absmin[0])
			fx += (self->absmin[0] - hit->absmin[0])/hit->size[0];
		if(hit->absmax[0] > self->absmax[0])
			fx += (hit->absmax[0] - self->absmax[0])/hit->size[0];
		if(hit->absmin[1] < self->absmin[1])
			fy += (self->absmin[1] - hit->absmin[1])/hit->size[1];
		if(hit->absmax[1] > self->absmax[1])
			fy += (hit->absmax[1] - self->absmax[1])/hit->size[1];
		f = (1.0 - fx - fy + fx*fy);
		if(f > 0) weight += f * hit->mass;
		weight += f*weight_on_top(hit);
	}
	if(weight != self->mass)
	{
		self->mass = weight;
		for (e = self->teammaster; e; e = e->teamchain)
		{
			if (!e->count)
				continue;
			num = e->count;
			if(weight < pow(10,num-1))
				e->s.frame = 12;
			else
				e->s.frame = ( weight % (int)pow(10,num) ) / ( pow(10,num-1) );
		}
	}
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}

void SP_trigger_scales (edict_t *self)
{
	vec3_t v;

	VectorMA (self->mins, 0.5, self->size, v);
	if(!self->team)
	{
		gi.dprintf("trigger_scales with no team at %s.\n",vtos(v));
		G_FreeEdict(self);
		return;
	}
	self->movetype = MOVETYPE_NONE;
	self->svflags  |= SVF_NOCLIENT;
	self->solid    = SOLID_TRIGGER;
	gi.setmodel (self,self->model);
	self->think     = trigger_scales_think;
	self->nextthink = level.time + 1.0;
	self->mass = 0;
	gi.linkentity(self);
}
//======================================================================================
// TRIGGER_BBOX - Exactly like a tremor_trigger_multiple, but uses bleft, tright fields
//                to define extents of trigger field rather than a brush model. This 
//                helps lower the total brush model count, which in turn helps head off
//                Index Overflow errors.
//======================================================================================
void trigger_bbox_reset (edict_t *self)
{
	self->takedamage = DAMAGE_YES;
	self->health     = self->max_health;
	gi.linkentity(self);
}

void trigger_bbox_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->activator = attacker;
	self->takedamage = DAMAGE_NO;
	G_UseTargets (self, self->activator);
	self->count--;
	if(!self->count) {
		self->think     = G_FreeEdict;
		self->nextthink = level.time + self->delay + FRAMETIME;
		return;
	}
	if (self->wait >= 0)
	{
		self->nextthink = level.time + self->wait;
		self->think = trigger_bbox_reset;
	}
	gi.linkentity(self);
}

void trigger_bbox_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if(self->nextthink)
		return;					// already "touched" and waiting
	if((other->client) && (self->spawnflags & 2))
		return;
	if((other->svflags & SVF_MONSTER) && !(self->spawnflags & 1))
		return;
	if(!other->client && !(other->svflags & SVF_MONSTER))
		return;
	if(other->client && other->client->spycam && !(self->svflags & SVF_TRIGGER_CAMOWNER))
		return;
	if((self->svflags & SVF_TRIGGER_CAMOWNER) && (!other->client || !other->client->spycam))
		return;
	self->activator = other;
	G_UseTargets(self,self->activator);
	if(self->wait > 0)
	{
		self->count--;
		if(!self->count)
		{
			self->think = G_FreeEdict;
			self->nextthink = level.time + FRAMETIME;
		}
		else
		{
			self->think = multi_wait;
			self->nextthink = level.time + self->wait;
		}
	}
	else
	{
		self->think = G_FreeEdict;
		self->nextthink = level.time + FRAMETIME;
	}
}
void trigger_bbox_use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->count--;
	if(!self->count) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}
	else
	{
		if(self->solid == SOLID_NOT)
		{
			if(self->max_health > 0)
			{
				self->solid = SOLID_BBOX;
				self->touch = NULL;
			}
			else
			{
				self->solid = SOLID_TRIGGER;
				self->touch = trigger_bbox_touch;
			}
		}
		else
			self->solid = SOLID_NOT;
		gi.linkentity(self);
	}
}

void SP_trigger_bbox (edict_t *ent)
{
	if (ent->sounds == 1)
		ent->noise_index = gi.soundindex ("misc/secret.wav");
	else if (ent->sounds == 2)
		ent->noise_index = gi.soundindex ("misc/talk.wav");
	else if (ent->sounds == 3)
		ent->noise_index = -1;
	
	if (!ent->wait)
		ent->wait = 0.2;

	ent->movetype = MOVETYPE_NONE;
	if (ent->spawnflags & TRIGGER_CAMOWNER)
		ent->svflags |= SVF_TRIGGER_CAMOWNER;

	if ( (!VectorLength(ent->bleft)) && (!VectorLength(ent->tright)) ) {
		VectorSet(ent->bleft,-16,-16,-16);
		VectorSet(ent->tright,16, 16, 16);
	}
	VectorCopy(ent->bleft,ent->mins);
	VectorCopy(ent->tright,ent->maxs);

	ent->max_health = ent->health;
	if (ent->health > 0)
	{
		ent->svflags   |= SVF_DEADMONSTER;
		ent->die        = trigger_bbox_die;
		ent->takedamage = DAMAGE_YES;
	}
	else
		ent->svflags |= SVF_NOCLIENT;

	if (ent->spawnflags & TRIGGER_START_OFF)
		ent->solid = SOLID_NOT;
	else
	{
		if (ent->health)
		{
			ent->solid = SOLID_BBOX;
			ent->touch = NULL;
		}
		else
		{
			ent->solid = SOLID_TRIGGER;
			ent->touch = trigger_bbox_touch;
		}
	}
	ent->use = trigger_bbox_use;
	gi.linkentity (ent);
}

//============================================================================================
// TRIGGER_LOOK
// Serves the same function as trigger_multiple, but:
// 1) Is only usable by players
// 2) Player must be looking at a point within bleft-tright of the origin of the trigger_look
// 3) If USE spawnflag (=8) is set, player must be pressing +use to trigger

void trigger_look_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	trace_t	tr;
	vec_t	dist;
	vec3_t	dir, forward, left, up, end, start;

	if(!other->client)
		return;

	if (self->nextthink)
		return;		// already been triggered

	if( (self->spawnflags & TRIGGER_NEED_USE) && !(other->client->use))
		return;

	if( (self->spawnflags & TRIGGER_CAMOWNER) && !other->client->spycam)
		return;

	if( self->spawnflags & 32 )
	{
		// Then trigger only fires if looking at TARGET, not trigger bbox
		edict_t	*target;
		int		num_triggered=0;
		edict_t	*what;
		vec3_t	endpos;

		target = G_Find(NULL,FOFS(targetname),self->target);
		while(target && !num_triggered)
		{
			what = LookingAt(other,0,endpos,NULL);
			if(target->inuse && (LookingAt(other,0,NULL,NULL) == target))
			{
				num_triggered++;
				self->activator = other;
				G_UseTarget (self, other, target);
			}
			else
				target = G_Find(target,FOFS(targetname),self->target);
		}
		if(!num_triggered)
			return;
	}
	else
	{
		if(other->client->spycam)
		{
			vec3_t	f1, l1, u1;

			AngleVectors(other->client->spycam->s.angles, forward, left, up);
			VectorScale(forward, other->client->spycam->move_origin[0],f1);
			VectorScale(left,   -other->client->spycam->move_origin[1],l1);
			VectorScale(up,      other->client->spycam->move_origin[2],u1);
			VectorAdd(other->client->spycam->s.origin,f1,start);
			VectorAdd(start,l1,start);
			VectorAdd(start,u1,start);
		}
		else
		{
			AngleVectors(other->client->v_angle, forward, NULL, NULL);
			VectorCopy(other->s.origin,start);
			start[2] += other->viewheight;
		}
		VectorSubtract(self->s.origin,start,dir);
		dist = VectorLength(dir);
		VectorMA(start,dist,forward,end);
		
		tr = gi.trace(start,vec3_origin,vec3_origin,end,other,MASK_OPAQUE);
		
		// See if we're looking at origin, within bleft, tright
		// FIXME: The following is more or less accurate if the
		// bleft-tright box is roughly a cube. If it's considerably
		// longer in one direction we'll get false misses.
		
		if(end[0] < self->s.origin[0] + self->bleft[0])
			return;
		if(end[1] < self->s.origin[1] + self->bleft[1])
			return;
		if(end[2] < self->s.origin[2] + self->bleft[2])
			return;
		if(end[0] > self->s.origin[0] + self->tright[0])
			return;
		if(end[1] > self->s.origin[1] + self->tright[1])
			return;
		if(end[2] > self->s.origin[2] + self->tright[2])
			return;
		
		self->activator = other;
		G_UseTargets (self, other);
	}

	if (self->wait > 0)	
	{
		self->think = multi_wait;
		self->nextthink = level.time + self->wait;
	}
	else
	{	// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		self->touch = NULL;
		self->nextthink = level.time + FRAMETIME;
		self->think = G_FreeEdict;
	}

}

void trigger_look_enable (edict_t *self, edict_t *other, edict_t *activator);
void trigger_look_disable (edict_t *self, edict_t *other, edict_t *activator)
{
	self->count--;
	if(!self->count) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}
	else
	{
		self->solid = SOLID_NOT;
		self->use = trigger_look_enable;
		gi.linkentity (self);
	}
}

void trigger_look_enable (edict_t *self, edict_t *other, edict_t *activator)
{
	self->solid = SOLID_TRIGGER;
	self->use = trigger_look_disable;
	gi.linkentity (self);
}

void SP_trigger_look (edict_t *self)
{
	if (self->sounds == 1)
		self->noise_index = gi.soundindex ("misc/secret.wav");
	else if (self->sounds == 2)
		self->noise_index = gi.soundindex ("misc/talk.wav");
	else if (self->sounds == 3)
		self->noise_index = -1;
	
	if (!self->wait)
		self->wait = 0.2;

	if (self->spawnflags & TRIGGER_START_OFF)
	{
		self->solid = SOLID_NOT;
		self->use = trigger_look_enable;
	}
	else
	{
		self->solid = SOLID_TRIGGER;
		self->use = trigger_look_disable;
	}

	self->movetype = MOVETYPE_NONE;
	gi.setmodel (self, self->model);
	self->svflags = SVF_NOCLIENT;

	if (self->spawnflags & TRIGGER_CAMOWNER)
		self->svflags |= SVF_TRIGGER_CAMOWNER;

	if( (VectorLength(self->bleft) == 0) && (VectorLength(self->tright) == 0)) {
		VectorSet(self->bleft,-16,-16,-16);
		VectorSet(self->tright,16,16,16);
	}
	self->touch = trigger_look_touch;
}

void trigger_speaker_think (edict_t *self)
{
	int			i;
	edict_t		*touching;
	edict_t		*player;

	touching = NULL;
	for (i = 1; i <= maxclients->value && !touching; i++) {
		player = &g_edicts[i];
		if(!player->inuse) continue;
		if(player->s.origin[0] < self->s.origin[0] + self->bleft[0]) continue;
		if(player->s.origin[1] < self->s.origin[1] + self->bleft[1]) continue;
		if(player->s.origin[2] < self->s.origin[2] + self->bleft[2]) continue;
		if(player->s.origin[0] > self->s.origin[0] + self->tright[0]) continue;
		if(player->s.origin[1] > self->s.origin[1] + self->tright[1]) continue;
		if(player->s.origin[2] > self->s.origin[2] + self->tright[2]) continue;
		touching = player;
	}
	if(touching)
		gi.sound (touching, CHAN_VOICE, self->noise_index, 1, ATTN_NORM, 0);
	self->nextthink = level.time + FRAMETIME;
}

void trigger_speaker_enable (edict_t *self, edict_t *other, edict_t *activator);
void trigger_speaker_disable (edict_t *self, edict_t *other, edict_t *activator)
{
	self->use   = trigger_speaker_enable;
	self->think = NULL;
	self->nextthink = 0;
}

void trigger_speaker_enable (edict_t *self, edict_t *other, edict_t *activator)
{
	self->use   = trigger_speaker_disable;
	self->think = trigger_speaker_think;
	self->think(self);
}

void SP_trigger_speaker (edict_t *self)
{
	char	buffer[MAX_QPATH];

	if(!st.noise)
	{
		gi.dprintf("trigger_speaker with no noise set at %s\n", vtos(self->s.origin));
		return;
	}
	if (!strstr (st.noise, ".wav"))
		Com_sprintf (buffer, sizeof(buffer), "%s.wav", st.noise);
	else
		strncpy (buffer, st.noise, sizeof(buffer));
	self->noise_index = gi.soundindex (buffer);

	if(self->spawnflags & 1) {
		self->use       = trigger_speaker_disable;
		self->think     = trigger_speaker_think;
		self->nextthink = level.time + FRAMETIME;
	} else {
		self->use       = trigger_speaker_enable;
	}

	if ( (!VectorLength(self->bleft)) && (!VectorLength(self->tright)) ) {
		VectorSet(self->bleft,-16,-16,-16);
		VectorSet(self->tright,16, 16, 16);
	}


}

//==============================================================================
// trigger_transition is a HL-like box that defines what entities will be
// moved from one map to another when a target_changelevel with the same
// targetname is fired. Brush models may NOT be moved.
//==============================================================================
void WriteEdict (FILE *f, edict_t *ent);

qboolean HasSpawnFunction(edict_t *ent)
{
	spawn_t	*s;
	gitem_t	*item;
	int		i;

	if(!ent->classname)
		return false;

	// check item spawn functions
	for (i=0,item=itemlist ; i<game.num_items ; i++,item++)
	{
		if (!item->classname)
			continue;
		if (!strcmp(item->classname, ent->classname))
			return true;
	}
	// check normal spawn functions
	for (s=spawns ; s->name ; s++)
	{
		if (!strcmp(s->name, ent->classname))
			return true;
	}
	return false;
}
void WriteTransitionEdict (FILE *f, edict_t *changelevel, edict_t *ent)
{
	byte		*temp;
	edict_t		e;
	field_t		*field;
	void		*p;

	memcpy(&e,ent,sizeof(edict_t));
	if (!Q_stricmp(e.classname,"target_laser") ||
		!Q_stricmp(e.classname,"target_blaster")  )
		vectoangles(e.movedir,e.s.angles);

	if (!Q_stricmp(e.classname,"target_speaker"))
		e.spawnflags |= 8;  // indicates that "message" contains noise

	if(changelevel->s.angles[YAW])
	{
		vec3_t	angles;
		vec3_t	forward, right, v;
		vec3_t	spawn_offset;
		
		VectorSubtract(e.s.origin,changelevel->s.origin,spawn_offset);
		angles[PITCH] = angles[ROLL] = 0.;
		angles[YAW] = changelevel->s.angles[YAW];
		AngleVectors(angles,forward,right,NULL);
		VectorNegate(right,right);
		VectorCopy(spawn_offset,v);
		G_ProjectSource (vec3_origin, v, forward, right, spawn_offset);
		VectorCopy(spawn_offset,e.s.origin);
		VectorCopy(e.velocity,v);
		G_ProjectSource (vec3_origin, v, forward, right, e.velocity);
		e.s.angles[YAW] += angles[YAW];
	}
	else
	{
		VectorSubtract(e.s.origin,changelevel->s.origin,e.s.origin);
	}
	// wipe out all edict_t and function members, since
	// they won't be valid in the next map and might otherwise
	// cause.... umm... big crash
	temp = (byte *)&e;
	for (field=fields ; field->name ; field++)
	{
		if((field->type == F_EDICT) || (field->type == F_FUNCTION))
		{
			p = (void *)(temp + field->ofs);
			*(edict_t **)p = NULL;
		}
	}
	// Clean out a few more things
	e.s.number = 0;
	memset (&e.moveinfo,   0,sizeof(moveinfo_t));
	memset (&e.area, 0, sizeof(e.area));
	e.linkcount = 0;
	e.nextthink = 0;
	e.groundentity_linkcount = 0;
	e.s.modelindex  = 0;
	e.s.modelindex2 = 0;
	e.s.modelindex3 = 0;
	e.s.modelindex4 = 0;
	e.noise_index = 0;
	// If the ent is a live bad guy monster, remove him from the total
	// monster count. He'll be added back in in the new map.
	if((e.svflags & SVF_MONSTER) && !(e.monsterinfo.aiflags & AI_GOOD_GUY))
	{
		if(e.health > 0)
			level.total_monsters--;
		else
			e.max_health = -1;
	}
	// Enemy isn't preserved... let's try a new flag for
	// single-player only that tells monster to find
	// the player again at startup
	if(!coop->value && !deathmatch->value)
	{
		if(ent->enemy == &g_edicts[1] && ent->health > 0)
			e.monsterinfo.aiflags = AI_RESPAWN_FINDPLAYER;
	}
	if(e.classname &&
	   ( !Q_stricmp(e.classname,"misc_actor") || strstr(e.classname,"monster_") ) &&
	   (e.health <= e.gib_health) )
		e.classname = "gibhead";
	WriteEdict(f,&e);
}

entlist_t DoNotMove[] = {
	{"crane_reset"},
	{"func_clock"},
	{"func_timer"},
	{"hint_path"},
	{"info_player_coop"},
	{"info_player_deathmatch"},
	{"info_player_intermission"},
	{"info_player_start"},
	{"light"},
	{"light_mine1"},
	{"light_mine2"},
	{"misc_strogg_ship"},
	{"misc_viper"},
	{"misc_viper_bomb"},
	{"model_train"},
	{"path_corner"},
	{"path_track"},
	{"point_combat"},
	{"target_actor"},
	{"target_changelevel"},
	{"target_character"},
	{"target_crosslevel_target"},
	{"target_crosslevel_trigger"},
	{"target_goal"},
	{"target_help"},
	{"target_lightramp"},
	{"target_locator"},
	{"target_lock"},
	{"target_lock_clue"},
	{"target_lock_code"},
	{"target_lock_digit"},
	{"target_rotation"},
	{"target_secret"},
	{"target_string"},
	{"trigger_always"},
	{"trigger_counter"},
	{"trigger_elevator"},
	{"trigger_key"},
	{"trigger_relay"},
	{"turret_driver"},
//CW++
	{"target_speaker"},
	{"target_effect"},
	{"trigger_medibot"},
	{"target_bmodel_spawner"},
//CW--
	{NULL}};

void trans_ent_filename (char *filename)
{
	GameDirRelativePath("save/trans.ent",filename);
}

int trigger_transition_ents (edict_t *changelevel, edict_t *self)
{
	char		t_file[_MAX_PATH];
	int			i, j;
	int			total=0;
	qboolean	nogo;
	edict_t		*ent;
	entlist_t	*p;
	FILE		*f;

	trans_ent_filename(t_file);
	f = fopen(t_file,"wb");
	if(!f)
	{
		gi.dprintf("Error opening %s for writing\n",t_file);
		return 0;
	}
	// First scan entities for brush models that SHOULD change levels, e.g. func_tracktrain,
	// which had better have a partner train in the next map... or we'll bitch loudly
	for(i=game.maxclients+1; i<globals.num_edicts; i++)
	{
		ent = &g_edicts[i];
		if(!ent->inuse) continue;
		if(ent->solid != SOLID_BSP) continue;
		if(ent->s.origin[0] > self->maxs[0]) continue;
		if(ent->s.origin[1] > self->maxs[1]) continue;
		if(ent->s.origin[2] > self->maxs[2]) continue;
		if(ent->s.origin[0] < self->mins[0]) continue;
		if(ent->s.origin[1] < self->mins[1]) continue;
		if(ent->s.origin[2] < self->mins[2]) continue;
		if(!Q_stricmp(ent->classname,"func_tracktrain") && !(ent->spawnflags & 8) && ent->targetname)
		{
			edict_t	*e;

			e = G_Spawn();
			e->classname = gi.TagMalloc(17,TAG_LEVEL);
			strcpy(e->classname,"info_train_start");
			e->targetname = gi.TagMalloc(strlen(ent->targetname)+1,TAG_LEVEL);
			strcpy(e->targetname,ent->targetname);
			e->target = gi.TagMalloc(strlen(ent->target)+1,TAG_LEVEL);
			strcpy(e->target,ent->target);
			e->spawnflags = ent->spawnflags;
			VectorCopy(ent->s.origin,e->s.origin);
			VectorCopy(ent->s.angles,e->s.angles);
			VectorCopy(ent->offset,  e->offset);
			VectorCopy(ent->bleft,   e->bleft);
			VectorCopy(ent->tright,  e->tright);
			e->sounds = ent->sounds;
			e->viewheight = ent->viewheight;
			e->speed = ent->moveinfo.speed;
			// misuse/abuse a couple of entries to copy moveinfo stuff:
			e->count = ent->moveinfo.state;
			e->radius = ent->moveinfo.distance;
			e->solid = SOLID_NOT;
			e->svflags |= SVF_NOCLIENT;
			if(ent->owner)
				e->style = ent->owner - g_edicts;
			else
				e->style = 0;
			gi.linkentity(e);

			ent->owner = NULL;
			ent->spawnflags |= 24;	// SF_TRACKTRAIN_OTHERMAP | SF_TRACKTRAIN_DISABLED
			VectorClear(ent->velocity);
			VectorClear(ent->avelocity);
			ent->moveinfo.state = ent->moveinfo.prevstate = 0;	// STOP
			gi.linkentity(ent);
		}
	}

	for(i=game.maxclients+1; i<globals.num_edicts; i++)
	{
		ent = &g_edicts[i];
		ent->id = 0;
		if(!ent->inuse) continue;
		// Pass up owned entities not owned by the player on this pass...
		// get 'em next pass so we'll know whether owner is in our list
		if(ent->owner && !ent->owner->client) continue;
		if(ent->movewith) continue;
		if(ent->s.origin[0] > self->maxs[0]) continue;
		if(ent->s.origin[1] > self->maxs[1]) continue;
		if(ent->s.origin[2] > self->maxs[2]) continue;
		if(ent->s.origin[0] < self->mins[0]) continue;
		if(ent->s.origin[1] < self->mins[1]) continue;
		if(ent->s.origin[2] < self->mins[2]) continue;
		if(ent->solid == SOLID_BSP) continue;
		if((ent->solid == SOLID_TRIGGER) && !FindItemByClassname(ent->classname)) continue;
		// Do not under any circumstances move these entities:
		for(p=DoNotMove, nogo=false; p->name && !nogo; p++)
			if(!Q_stricmp(ent->classname,p->name))
				nogo = true;
		if(nogo) continue;
		if(!HasSpawnFunction(ent)) continue;
		total++;
		ent->id = total;
		if(ent->owner)
			ent->owner_id = -(ent->owner - g_edicts);
		else
			ent->owner_id = 0;

		WriteTransitionEdict(f,changelevel,ent);
		gi.unlinkentity(ent);
		ent->inuse = false;
	}
	// Repeat, ONLY for ents owned by non-players
	for(i=game.maxclients+1; i<globals.num_edicts; i++)
	{
		ent = &g_edicts[i];
		if(!ent->inuse) continue;
		if(!ent->owner) continue;
		if(ent->owner->client) continue;
		if(ent->movewith) continue;
		if(ent->solid == SOLID_BSP) continue;
		if((ent->solid == SOLID_TRIGGER) && !FindItemByClassname(ent->classname)) continue;
		// Do not under any circumstances move these entities:
		for(p=DoNotMove, nogo=false; p->name && !nogo; p++)
			if(!Q_stricmp(ent->classname,p->name))
				nogo = true;
		if(nogo) continue;
		if(!HasSpawnFunction(ent)) continue;
		if(ent->s.origin[0] > self->maxs[0]) continue;
		if(ent->s.origin[1] > self->maxs[1]) continue;
		if(ent->s.origin[2] > self->maxs[2]) continue;
		if(ent->s.origin[0] < self->mins[0]) continue;
		if(ent->s.origin[1] < self->mins[1]) continue;
		if(ent->s.origin[2] < self->mins[2]) continue;
		ent->owner_id = 0;
		for(j=game.maxclients+1; j<globals.num_edicts && !ent->owner_id; j++)
		{
			if(ent->owner == &g_edicts[j])
				ent->owner_id = g_edicts[j].id;
		}
		if(!ent->owner_id) continue;
		total++;
		ent->id = total;

		WriteTransitionEdict(f,changelevel,ent);
		gi.unlinkentity(ent);
		ent->inuse = false;
	}

	fflush(f);
	fclose(f);
	return total;
}

void SP_trigger_transition (edict_t *self)
{
	if(!self->targetname)
	{
		gi.dprintf("trigger_transition w/o a targetname\n");
		G_FreeEdict(self);
	}
	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;
	gi.setmodel (self, self->model);
	self->svflags = SVF_NOCLIENT;
}

// ***************************
// TRIGGER_DISGUISE - straight from Rogue MP
// ***************************

/*QUAKED trigger_disguise (.5 .5 .5) ? TOGGLE START_ON REMOVE
Anything passing through this trigger when it is active will
be marked as disguised.

TOGGLE - field is turned off and on when used.
START_ON - field is active when spawned.
REMOVE - field removes the disguise
*/

void trigger_disguise_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->client)
	{
		if(self->spawnflags & 4)
			other->flags &= ~FL_DISGUISED;
		else
			other->flags |= FL_DISGUISED;

		self->count--;
		if(!self->count) {
			self->think = G_FreeEdict;
			self->nextthink = level.time + FRAMETIME;
		}
	}
}

void trigger_disguise_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if(self->solid == SOLID_NOT)
		self->solid = SOLID_TRIGGER;
	else
		self->solid = SOLID_NOT;

	gi.linkentity(self);
}

void SP_trigger_disguise (edict_t *self)
{
	if(self->spawnflags & 2)
		self->solid = SOLID_TRIGGER;
	else
		self->solid = SOLID_NOT;

	self->touch = trigger_disguise_touch;
	self->use = trigger_disguise_use;
	self->movetype = MOVETYPE_NONE;
	self->svflags = SVF_NOCLIENT;

	gi.setmodel (self, self->model);
	gi.linkentity(self);

}

// end DWH
