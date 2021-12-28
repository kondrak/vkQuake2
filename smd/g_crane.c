#include "g_local.h"

void box_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);

// g_crane.c
//
// Utility functions for manipulating overhead crane
//

#define STEPSIZE 8
#define MAX_PICKUP_DISTANCE 64
#define BUFFER 1 // Bonk buffer
#define CABLE_SEGMENT 32.0
#define SPOT_SEGMENT 32.0
#define CARGO_BUFFER 0.125

qboolean box_movestep (edict_t *ent, vec3_t move, qboolean relink);
void Crane_Move_Begin (edict_t *);

void Moving_Speaker_Think(edict_t *speaker)
{
	qboolean moved;
	vec3_t  offset;
	edict_t *owner;

	owner = speaker->owner;
	if(!owner)
	{
		G_FreeEdict(speaker);
		return;
	}
	if(!owner->inuse)
	{
		G_FreeEdict(speaker);
		return;
	}
	if(speaker->spawnflags & 15)
	{
		if((speaker->spawnflags & 8) && (
			(!speaker->owner->groundentity) ||      // not on the ground
			(!speaker->owner->activator)    ||      // not "activated" by anything
			(!speaker->owner->activator->client)  ) // not activated by client
			) {
			moved = false;
		} else
		{
			moved = false;
			VectorSubtract(speaker->s.origin,owner->s.origin,offset);
			if((speaker->spawnflags & 1) && (fabs(offset[0] - speaker->offset[0]) > 0.125) )
				moved = true;
			if((speaker->spawnflags & 2) && (fabs(offset[1] - speaker->offset[1]) > 0.125) )
				moved = true;
			if((speaker->spawnflags & 4) && (fabs(offset[2] - speaker->offset[2]) > 0.125) )
				moved = true;
		}
		if(moved) {
			speaker->s.sound = speaker->owner->noise_index;
	#ifdef LOOP_SOUND_ATTENUATION
			speaker->s.attenuation = speaker->attenuation;
	#endif
		}
		else
			speaker->s.sound = 0;
	}
	else {
		speaker->s.sound = speaker->owner->noise_index;
#ifdef LOOP_SOUND_ATTENUATION
		speaker->s.attenuation = speaker->attenuation;
#endif
	}

	VectorAdd(owner->s.origin,speaker->offset,speaker->s.origin);
	speaker->nextthink = level.time + FRAMETIME;
	gi.linkentity(speaker);
}

edict_t *CrateOnTop (edict_t *from, edict_t *ent)
{
	float maxdist;

	if (!from)
		from = g_edicts;
	else
		from++;

	for ( ; from < &g_edicts[globals.num_edicts] ; from++)
	{
		if (from == ent)
			continue;
		if (!from->inuse)
			continue;
		if (from->movetype != MOVETYPE_PUSHABLE)
			continue;
		if (from->absmin[0] >=  ent->absmax[0]) continue;
		if (from->absmax[0] <=  ent->absmin[0]) continue;
		if (from->absmin[1] >=  ent->absmax[1]) continue;
		if (from->absmax[1] <=  ent->absmin[1]) continue;
		maxdist = VectorLength(ent->velocity)*FRAMETIME + 2.0;
		if (fabs(from->absmin[2] - ent->absmax[2]) > maxdist) continue;
		return from;
	}

	return NULL;
}

void Cargo_Stop (edict_t *ent)
{
	vec3_t v;

	VectorClear (ent->velocity);
	// Sanity check: force cargo to correct elevation
	ent->s.origin[2] += ent->crane_hook->absmin[2] - CARGO_BUFFER - ent->absmax[2];
	ent->think = NULL;
	ent->nextthink = 0;
	ent->gravity   = 0.0;
	VectorAdd(ent->absmax,ent->absmin,v);
	VectorScale(v,0.5,v);
	v[2] = ent->absmax[2];
	gi.positioned_sound (v, ent, CHAN_VOICE, gi.soundindex("tank/thud.wav"), 1, 1, 0);
	gi.linkentity(ent);
	ent->crane_control->busy = false;
}

void cargo_blocked (edict_t *cargo, edict_t *obstacle )
{
	vec3_t	origin;

	VectorAdd(obstacle->s.origin,obstacle->origin_offset,origin);
	cargo->gravity  = 1.0;
	cargo->movetype = MOVETYPE_PUSHABLE;
	cargo->velocity[2] = 0;
	box_movestep (cargo, vec3_origin, true);
	cargo->crane_control->busy = false;
	cargo->crane_hook->crane_cargo = NULL;
	cargo->blocked   = NULL;
	cargo->touch     = box_touch;
	cargo->nextthink = 0;
	gi.linkentity(cargo);
}

void Cargo_Float_Up (edict_t *cargo)
{
	cargo->velocity[2] += sv_gravity->value * FRAMETIME;
	cargo->velocity[0] = cargo->velocity[1] = 0;
	if(cargo->absmax[2] + cargo->velocity[2]*FRAMETIME >=
		cargo->crane_hook->absmin[2]-CARGO_BUFFER)
	{
		cargo->attracted = false;
		cargo->think = Cargo_Stop;
	}
	cargo->nextthink = level.time + FRAMETIME;
	gi.linkentity(cargo);
}

void SetCableLength(edict_t *cable)
{
	int frame;
	float length;

	length = cable->s.origin[2] - cable->crane_hook->absmax[2];
	frame = (int)(length/CABLE_SEGMENT);
	if((frame+1)*CABLE_SEGMENT < length) frame++;
	frame = max(0,min(frame,19));
	cable->s.frame = frame;
}

void SetSpotlightLength(edict_t *hook)
{
	trace_t tr;
	vec3_t start, end;

	start[0] = (hook->absmin[0] + hook->absmax[0])/2;
	start[1] = (hook->absmin[1] + hook->absmax[1])/2;
	start[2] = hook->absmin[2] + 1;
	end[0] = start[0];
	end[1] = start[1];
	end[2] = start[2] - 8192;
	tr = gi.trace(start,NULL,NULL,end,hook,MASK_SOLID);
	hook->crane_light->s.origin[2] = tr.endpos[2] + 1;
}

void Cable_Think(edict_t *cable)
{
	SetCableLength(cable);
	cable->nextthink = level.time + FRAMETIME;
	gi.linkentity(cable);
}

void crane_light_off(edict_t *light)
{
	light->svflags |= SVF_NOCLIENT;
}

void Crane_Move_Done (edict_t *ent)
{
	if(!Q_stricmp(ent->classname,"crane_hook"))
	{
		edict_t *cable;
		edict_t *light;
		// Sanity checks - force hook to correct relative location from hoist...
		ent->s.origin[0] = ent->crane_hoist->s.origin[0] + ent->offset[0];
		ent->s.origin[1] = ent->crane_hoist->s.origin[1] + ent->offset[1];
		// ... and force cargo to correct elevation ...
		if(ent->crane_cargo)
		{
			ent->crane_cargo->s.origin[2] +=
				ent->absmin[2] - CARGO_BUFFER - ent->crane_cargo->absmax[2];
			gi.linkentity(ent->crane_cargo);
		}
		// ... and finally, stop cable and move to correct position
		cable = ent->crane_cable;
		VectorClear(cable->velocity);
		cable->s.origin[0] = ent->s.origin[0] + cable->offset[0];
		cable->s.origin[1] = ent->s.origin[1] + cable->offset[1];
		SetCableLength(cable);
		gi.linkentity(cable);
		light = ent->crane_light;
		if(light)
		{
			VectorClear(light->velocity);
			light->think = crane_light_off;
			light->nextthink = level.time + 1.0;
			gi.linkentity(light);
		}
	}
// Lazarus: ACK! If crate is being carried, it's NOT a MOVETYPE_PUSHABLE!!!!
//	        if(ent->movetype == MOVETYPE_PUSHABLE)
	if(!strcmp(ent->classname,"func_pushable"))
	{
		edict_t *e;

		ent->s.origin[2] += ent->crane_hook->absmin[2] - CARGO_BUFFER - ent->absmax[2];
		// Check to see if any OTHER crates are stacked on this one. If so,
		// adjust their position and velocity as well.
		e = NULL;
		while ((e = CrateOnTop(e, ent)) != NULL)
		{
			VectorClear(e->velocity);
			e->s.origin[2] += ent->crane_hook->absmin[2] - e->absmin[2];
			gi.linkentity(e);
		}
	}
	VectorClear (ent->velocity);
	ent->busy      = false;
	ent->think     = NULL;
	ent->nextthink = 0;
	gi.linkentity(ent);
}

void Crane_Stop(edict_t *control)
{
	if(control->crane_beam->crane_onboard_control)
		Crane_Move_Done(control->crane_beam->crane_onboard_control);
	Crane_Move_Done(control->crane_beam);
	Crane_Move_Done(control->crane_hoist);
	Crane_Move_Done(control->crane_hook);
	if(control->crane_hook->crane_cargo) Crane_Move_Done(control->crane_hook->crane_cargo);
}

qboolean Crane_Hook_Bonk(edict_t *hook, int axis, int dir, vec3_t bonk)
{
	float  fraction, cargo_fraction;
	int    i1,i2;
	edict_t *cargo;
	vec3_t cargo_origin, cargo_bonk, origin, end, forward, start;
	vec3_t mins, maxs;
	trace_t tr;

	VectorClear(end);
	VectorClear(start);
	VectorClear(forward);
	forward[axis] = (float)dir;
	switch (axis)
	{
	case 0:
		// X
		i1 = 1;
		i2 = 2;
		break;
	case 1:
		// Y
		i1 = 0;
		i2 = 2;
		break;
	default:
		// Z
		i1 = 0;
		i2 = 1;
		break;
	}
	cargo = hook->crane_cargo;
	VectorAdd(hook->s.origin,hook->origin_offset,origin);
	VectorCopy(origin,start);
	if(dir > 0)
		start[axis] = origin[axis] + hook->size[axis]/2;
	else
		start[axis] = origin[axis] - hook->size[axis]/2;
	fraction = 1.0;
	mins[axis] =  0;
	mins[i1]   = -hook->size[i1]/2;
	mins[i2]   = -hook->size[i2]/2;
	maxs[axis] =  0;
	maxs[i1]   =  hook->size[i1]/2;
	maxs[i2]   =  hook->size[i2]/2;
	VectorMA(start,8192,forward,end);
	tr = gi.trace(start,mins,maxs,end,cargo,MASK_PLAYERSOLID);
	if(tr.fraction < fraction && tr.ent != hook->crane_beam &&
		tr.ent != hook->crane_hoist && tr.ent != cargo )
	{
		VectorCopy(tr.endpos,bonk);
		bonk[axis] -= dir*BUFFER;
		fraction = tr.fraction;
	} else {
		VectorCopy(end,bonk);
	}

	if(cargo)
	{
		VectorAdd(cargo->s.origin,cargo->origin_offset,cargo_origin);
		VectorCopy(cargo_origin,start);
		if(dir > 0)
			start[axis] = cargo_origin[axis] + cargo->size[axis]/2;
		else
			start[axis] = cargo_origin[axis] - cargo->size[axis]/2;
		cargo_fraction = 1.0;
		mins[axis] =  0;
		mins[i1]   = -cargo->size[i1]/2+1;
		mins[i2]   = -cargo->size[i2]/2+1;
		maxs[axis] =  0;
		maxs[i1]   =  cargo->size[i1]/2-1;
		maxs[i2]   =  cargo->size[i2]/2-1;
		VectorMA(start, 8192, forward, end);
		tr=gi.trace(start, mins, maxs, end, cargo, MASK_PLAYERSOLID );
		if(tr.fraction < cargo_fraction && tr.ent != hook->crane_beam &&
			tr.ent != hook->crane_hoist && tr.ent != hook )
		{
			VectorCopy(tr.endpos,cargo_bonk);
			cargo_bonk[axis] -= dir*BUFFER;
			cargo_fraction = tr.fraction;
		} else {
			VectorCopy(end,cargo_bonk);
		}
		if(cargo_fraction < 1)
		{
			fraction = cargo_fraction;
			if(dir > 0)
			{
				cargo_bonk[axis] += hook->absmax[axis] - cargo->absmax[axis];
				bonk[axis] = min(bonk[axis],cargo_bonk[axis]);
			}
			else
			{
				cargo_bonk[axis] += hook->absmin[axis] - cargo->absmin[axis];
				bonk[axis] = max(bonk[axis],cargo_bonk[axis]);
			}
		}
	}

	if(fraction < 1)
		return true;
	else
		return false;
}

void Crane_blocked (edict_t *self, edict_t *other)
{
	if ( (other->classname) && (other->movetype == MOVETYPE_PUSHABLE))
	{
		// treat func_pushable like a world brush - attempt to stop
		// crane
		// This *shouldn't* be necessary, but I'm a pessimist
		Crane_Stop(self->crane_control);
		return;

	}

	if (self->crane_control->crane_hook == other)
		return;

	if (!(other->svflags & SVF_MONSTER) && (!other->client) )
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, 0, MOD_CRUSH);
		// if it's still there, nuke it
		if (other)
		{
			// Lazarus: Some of our ents don't have origin near the model
			vec3_t save;
			VectorCopy(other->s.origin,save);
			VectorMA (other->absmin, 0.5, other->size, other->s.origin);
			BecomeExplosion1 (other);
		}
		return;
	}

	if (level.time < self->touch_debounce_time)
		return;

	if (!self->dmg)
		return;
	self->touch_debounce_time = level.time + 0.5;
	T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}

void Crane_Move_Final (edict_t *ent)
{
	float bonk_distance;

	if(ent->crane_control->activator->client->use)
	{
		// Use key is still pressed
		bonk_distance = ent->crane_control->crane_increment *
			(ent->crane_bonk - ent->absmin[ent->crane_dir]);
		if(ent->moveinfo.remaining_distance > 0)
			bonk_distance -= ent->moveinfo.remaining_distance;
		bonk_distance = min(bonk_distance,STEPSIZE);
		if(bonk_distance > 0)
		{
			ent->moveinfo.remaining_distance += bonk_distance;
			Crane_Move_Begin(ent);
			return;
		}
	}
	
	if (ent->moveinfo.remaining_distance == 0)
	{
		Crane_Move_Done (ent);
		return;
	}

	VectorScale (ent->moveinfo.dir, ent->moveinfo.remaining_distance / FRAMETIME, ent->velocity);
	if(!strcmp(ent->classname,"crane_hook"))
	{
		VectorCopy(ent->velocity,ent->crane_cable->velocity);
		ent->crane_cable->velocity[2] = 0;
		gi.linkentity(ent);
		if(ent->crane_light != NULL)
		{
			VectorCopy(ent->velocity,ent->crane_light->velocity);
			ent->crane_light->velocity[2] = 0;
			gi.linkentity(ent->crane_light);
		}
	}
	ent->think     = Crane_Move_Done;
	ent->nextthink = level.time + FRAMETIME;
	gi.linkentity(ent);
}

void Crane_Move_Begin (edict_t *ent)
{
	float	frames;

	if ((ent->moveinfo.speed * FRAMETIME) >= ent->moveinfo.remaining_distance)
	{
		Crane_Move_Final (ent);
		return;
	}
	VectorScale (ent->moveinfo.dir, ent->moveinfo.speed, ent->velocity);
	frames = floor((ent->moveinfo.remaining_distance / ent->moveinfo.speed) / FRAMETIME);
	ent->moveinfo.remaining_distance -= frames * ent->moveinfo.speed * FRAMETIME;
	if(!strcmp(ent->classname,"crane_hook"))
	{
		if((ent->crane_light) && (ent->crane_cargo==NULL))
		{
			SetSpotlightLength(ent);
			ent->crane_light->svflags &= ~SVF_NOCLIENT;
		}
		VectorCopy(ent->velocity,ent->crane_cable->velocity);
		ent->crane_cable->velocity[2] = 0;
		gi.linkentity(ent->crane_cable);
		if(ent->crane_light != NULL)
		{
			VectorCopy(ent->velocity,ent->crane_light->velocity);
			ent->crane_light->velocity[2] = 0;
			gi.linkentity(ent->crane_light);
		}
	}
	ent->nextthink = level.time + (frames * FRAMETIME);
	ent->think     = Crane_Move_Final;
	ent->blocked   = Crane_blocked;
	gi.linkentity(ent);
}

void G_FindCraneParts()
{
	vec3_t  dist;
	edict_t *cable=NULL;
	edict_t *control;
	edict_t *beam;
	edict_t *hoist;
	edict_t *hook;
	edict_t *light;
	edict_t *p1, *p2;

	edict_t	*e;
	int		direction;
	int		i;

	for (i=1, e=g_edicts+i ; i < globals.num_edicts ; i++,e++)
	{
		if (!e->inuse)
			continue;
		if (!e->classname)
			continue;
		if (strcmp(e->classname,"crane_control"))
			continue;

		control = e;
		beam = G_Find(NULL,FOFS(targetname),control->target);
		if(!beam)
		{
			gi.dprintf("Crane_control with no target\n");
			G_FreeEdict(control);
			return;
		}
		// get path_corner locations to determine movement direction
		p1 = G_Find(NULL,FOFS(targetname),beam->pathtarget);
		if(!p1->target)
		{
			gi.dprintf("Only 1 path_corner in pathtarget sequence for crane_beam\n"
				"(2 are required)\n");
			G_FreeEdict(control);
			G_FreeEdict(beam);
			G_FreeEdict(p1);
			return;
		}
		p2 = G_Find(NULL,FOFS(targetname),p1->target);
		VectorSubtract(p1->s.origin,p2->s.origin,dist);
		if(fabs(dist[0]) > fabs(dist[1]))
		{
			VectorSet(beam->movedir, 1.0,0.0,0.0);
			direction = 0;
			if(p1->s.origin[0] < p2->s.origin[0])
			{
				VectorCopy(p1->s.origin,beam->pos1);
				VectorCopy(p2->s.origin,beam->pos2);
			}
			else
			{
				VectorCopy(p2->s.origin,beam->pos1);
				VectorCopy(p1->s.origin,beam->pos2);
			}
		}
		else
		{
			VectorSet(beam->movedir, 0.0,1.0,0.0);
			direction = 1;
			if(p1->s.origin[1] < p2->s.origin[1])
			{
				VectorCopy(p1->s.origin,beam->pos1);
				VectorCopy(p2->s.origin,beam->pos2);
			}
			else
			{
				VectorCopy(p2->s.origin,beam->pos1);
				VectorCopy(p1->s.origin,beam->pos2);
			}
		}
		hoist = G_Find(NULL,FOFS(targetname),beam->target);
		if(!hoist)
		{
			gi.dprintf("Crane_beam with no target\n");
			G_FreeEdict(control);
			G_FreeEdict(beam);
			return;
		}
		// get path_corner locations to determine movement direction
		p1 = G_Find(NULL,FOFS(targetname),hoist->pathtarget);
		if(!p1->target)
		{
			gi.dprintf("Only 1 path_corner in pathtarget sequence for crane_hoist\n"
				"(2 are required)\n");
			G_FreeEdict(control);
			G_FreeEdict(beam);
			G_FreeEdict(hoist);
			G_FreeEdict(p1);
			return;
		}
		p2 = G_Find(NULL,FOFS(targetname),p1->target);
		VectorSubtract(p1->s.origin,p2->s.origin,dist);
		if(fabs(dist[0]) > fabs(dist[1]))
		{
			VectorSet(hoist->movedir, 1.0,0.0,0.0);
			if(p1->s.origin[0] < p2->s.origin[0])
			{
				VectorCopy(p1->s.origin,hoist->pos1);
				VectorCopy(p2->s.origin,hoist->pos2);
			}
			else
			{
				VectorCopy(p2->s.origin,hoist->pos1);
				VectorCopy(p1->s.origin,hoist->pos2);
			}
		}
		else
		{
			VectorSet(hoist->movedir, 0.0,1.0,0.0);
			if(p1->s.origin[1] < p2->s.origin[1])
			{
				VectorCopy(p1->s.origin,hoist->pos1);
				VectorCopy(p2->s.origin,hoist->pos2);
			}
			else
			{
				VectorCopy(p2->s.origin,hoist->pos1);
				VectorCopy(p1->s.origin,hoist->pos2);
			}
		}

		// correct spawnflags for beam and hoist speakers
		if(beam->speaker)
		{
			if(direction)
				beam->speaker->spawnflags = 2;
			else
				beam->speaker->spawnflags = 1;
		}
		if(hoist->speaker)
		{
			if(direction)
				hoist->speaker->spawnflags = 1;
			else
				hoist->speaker->spawnflags = 2;
		}

		hook = G_Find(NULL,FOFS(targetname),hoist->target);
		if(!hook)
		{
			gi.dprintf("Crane hoist with no target\n");
			G_FreeEdict(control);
			G_FreeEdict(beam);
			G_FreeEdict(hoist);
			return;
		}
		// Turn on hook ambient sound if control is on
		// We use a trick here... since hook by definition cannot
		// be moving unless control is on, then with control off
		// we set hook speaker spawnflag to require hook to be
		// moving to play
		if(hook->speaker)
			hook->speaker->spawnflags = 1 - (control->spawnflags & 1);
			
		// Get offset from hook origin to hoist origin, so we can
		// correct timing problems
		VectorSubtract(hook->s.origin,hoist->s.origin,hook->offset);

		// get path_corner locations to determine movement direction
		p1 = G_Find(NULL,FOFS(targetname),hook->pathtarget);
		if(!p1->target)
		{
			gi.dprintf("Only 1 path_corner in pathtarget sequence for crane_hook\n"
				"(2 are required)\n");
			G_FreeEdict(control);
			G_FreeEdict(beam);
			G_FreeEdict(hoist);
			G_FreeEdict(hook);
			G_FreeEdict(p1);
			return;
		}
		p2 = G_Find(NULL,FOFS(targetname),p1->target);
		VectorSet(hook->movedir,0.0,0.0,1.0);
		if(p1->s.origin[2] < p2->s.origin[2])
		{
			VectorCopy(p1->s.origin,hook->pos1);
			VectorCopy(p2->s.origin,hook->pos2);
		}
		else
		{
			VectorCopy(p2->s.origin,hook->pos1);
			VectorCopy(p1->s.origin,hook->pos2);
		}

		control->crane_control = control;
		control->crane_beam    = beam;
		control->crane_hoist   = hoist;
		control->crane_hook    = hook;
		if(!beam->crane_control) beam->crane_control = control;
		if(control->team)
		{
			beam->crane_control = control;
			if(beam->flags & FL_TEAMSLAVE)
				beam->crane_onboard_control = beam->teammaster;
			else
				beam->crane_onboard_control = beam->teamchain;
		}
		else
			beam->crane_onboard_control = NULL;
		beam->crane_hoist    = hoist;
		beam->crane_hook     = hook;
		hoist->crane_control = beam->crane_control;
		hoist->crane_beam    = beam;
		hoist->crane_hook    = hook;
		hook->crane_control  = beam->crane_control;
		hook->crane_beam     = beam;
		hook->crane_hoist    = hoist;
		if(control->spawnflags & 4)
		{
			beam->dmg  = 0;
			hoist->dmg = 0;
			hook->dmg  = 0;
		}

		if(hook->crane_cable == NULL)
		{
			int   frame;
			float length;

			cable = G_Spawn();
			cable->classname = "crane_cable";
			VectorAdd(hook->absmin,hook->absmax,cable->s.origin);
			VectorScale(cable->s.origin,0.5,cable->s.origin);
			VectorAdd(cable->s.origin,hook->move_origin,cable->s.origin);
			VectorSubtract(cable->s.origin,hook->s.origin,cable->offset);
			cable->s.origin[2] = hoist->absmax[2] - 2;
			cable->model = "models/cable/tris.md2";
			gi.setmodel(cable,cable->model);
			cable->s.skinnum = 0;
			length = hoist->absmax[2]-1 - hook->absmax[2];
			frame = (int)(length/CABLE_SEGMENT);
			if((frame+1)*CABLE_SEGMENT < length) frame++;
			frame = max(0,min(frame,19));
			cable->s.frame = frame;
			cable->solid = SOLID_NOT;
			cable->movetype = MOVETYPE_STOP;
			VectorSet(cable->mins,-2,-2,length);
			VectorSet(cable->maxs, 2, 2,0);
			gi.linkentity(cable);
			beam->crane_cable    = cable;
			hoist->crane_cable   = cable;
			hook->crane_cable    = cable;
			cable->crane_control = control;
			cable->crane_beam    = beam;
			cable->crane_hoist   = hoist;
			cable->crane_hook    = hook;
		}
		control->crane_cable = hook->crane_cable;

		if((hook->spawnflags & 1) && (hook->crane_light == NULL))
		{
			light = G_Spawn();
			light->s.origin[0] = (hook->absmin[0] + hook->absmax[0])/2;
			light->s.origin[1] = (hook->absmin[1] + hook->absmax[1])/2;
			light->s.origin[2] = hook->absmin[2] + 8;
			VectorSet(light->mins,-32,-32,-512);
			VectorSet(light->maxs, 32, 32,   0);
			light->solid      = SOLID_NOT;
			light->movetype   = MOVETYPE_NOCLIP;
			light->s.effects  = EF_SPHERETRANS;
			light->s.modelindex = gi.modelindex("sprites/point.sp2");
			light->s.effects = EF_HYPERBLASTER;
			light->svflags       = SVF_NOCLIENT;
			VectorSubtract(light->s.origin,hook->s.origin,light->offset);
			gi.linkentity(light);
			beam->crane_light    = light;
			hook->crane_light    = light;
			cable->crane_light   = light;
		}
		control->crane_light = hook->crane_light;

		// If control is NOT onboard, move beam speaker (if any) to end of
		// beam closest to control
		if(!beam->crane_onboard_control && beam->speaker) {
			if(beam->movedir[0] > 0) {
				if(control->absmin[1]+control->absmax[1] < beam->absmin[1]+beam->absmax[1])
					beam->speaker->s.origin[1] = beam->absmin[1];
				else
					beam->speaker->s.origin[1] = beam->absmax[1];
			} else {
				if(control->absmin[0]+control->absmax[0] < beam->absmin[0]+beam->absmax[0])
					beam->speaker->s.origin[0] = beam->absmin[0];
				else
					beam->speaker->s.origin[0] = beam->absmax[0];
			}
			beam->speaker->s.origin[2] = control->s.origin[2] + 32;
			VectorSubtract(beam->speaker->s.origin,beam->s.origin,beam->speaker->offset);
		}
	}
}

void Crane_AdjustSpeed(edict_t *ent)
{
	float frames;

	// Adjust speed so that travel time is an integral multiple
	// of FRAMETIME
	if(ent->moveinfo.remaining_distance > 0)
	{
		ent->moveinfo.speed = ent->speed;
		frames = floor((ent->moveinfo.remaining_distance / ent->moveinfo.speed) / FRAMETIME);
		if(frames < 1) frames = 1;
		ent->moveinfo.speed = ent->moveinfo.remaining_distance/(frames*FRAMETIME);
	}
}

void crane_control_action(edict_t *control, edict_t *activator, vec3_t point)
{
	float  Z;
	int    dir;
	int    row, column;
	int    content;
	edict_t *beam, *cable, *cargo, *hoist, *hook;
	trace_t tr;
	vec3_t  center, v;
	vec3_t  end, forward, start, pt;
	vec3_t  bonk, mins, maxs;

	if(!(control->spawnflags & 1))
	{
		if(control->message)
			gi.centerprintf(activator,"%s\n",control->message);
		else
			gi.centerprintf(activator,"No power\n");
		return;
	}

	if(control->busy) return;

	// First make sure player (activator) is on the panel side of the
	// control panel
	// Also get center point of panel side
	switch (control->style)
	{
	case 0:
		if(activator->s.origin[0] > control->absmax[0]) return;
		center[0] = control->absmin[0];
		center[1] = (control->absmin[1] + control->absmax[1])/2;
		center[2] = (control->absmin[2] + control->absmax[2])/2;
		break;
	case 1:
		if(activator->s.origin[1] > control->absmax[1]) return;
		center[0] = (control->absmin[0] + control->absmax[0])/2;
		center[1] = control->absmin[1];
		center[2] = (control->absmin[2] + control->absmax[2])/2;
		break;
	case 2:
		if(activator->s.origin[0] < control->absmin[0]) return;
		center[0] = control->absmax[0];
		center[1] = (control->absmin[1] + control->absmax[1])/2;
		center[2] = (control->absmin[2] + control->absmax[2])/2;
		break;
	case 3:
		if(activator->s.origin[1] < control->absmin[1]) return;
		center[0] = (control->absmin[0] + control->absmax[0])/2;
		center[1] = control->absmax[1];
		center[2] = (control->absmin[2] + control->absmax[2])/2;
		break;
	}
	// now check distance from player to panel
	VectorSubtract(activator->s.origin,center,v);
	if(VectorLength(v) > 64) return;

	beam  = control->crane_beam;
	cable = control->crane_cable;
	hoist = control->crane_hoist;
	hook  = control->crane_hook;
	cargo = hook->crane_cargo;
	if(cargo) cargo->gravity = 0.0;    // reset after making it float up,
									   // otherwise things get jammed up
	control->activator = activator;

	// if any part of crane is currently moving, do nothing.
	if(VectorLength(control->velocity) > 0.) return;
	if(VectorLength(beam->velocity)    > 0.) return;
	if(VectorLength(hoist->velocity)   > 0.) return;
	if(VectorLength(hook->velocity)    > 0.) return;

	// now find which row and column of buttons corresponds to "point"
	row = (2*(point[2] - control->absmin[2]))/(control->absmax[2]-control->absmin[2]);
	if(row < 0) row = 0;
	if(row > 1) row = 1;
	switch (control->style)
	{
	case 1:
		column = (4*(point[0]-control->absmin[0]))/(control->absmax[0]-control->absmin[0]);
		break;
	case 2:
		column = (4*(point[1]-control->absmin[1]))/(control->absmax[1]-control->absmin[1]);
		break;
	case 3:
		column = (4*(point[0]-control->absmax[0]))/(control->absmin[0]-control->absmax[0]);
		break;
	default:
		column = (4*(point[1]-control->absmax[1]))/(control->absmin[1]-control->absmax[1]);
		break;
	}
	if(column < 0) column = 0;
	if(column > 3) column = 3;

	// adjust for controller facing beam movement direction
	if( beam->movedir[0] > 0 && (control->style == 0 || control->style == 2)) {
		if(column == 0 || column == 1) {
			column = 1-column;
			row = 1-row;
		}
	}
	if( beam->movedir[1] > 0 && (control->style == 1 || control->style == 3)) {
		if(column == 0 || column == 1) {
			column = 1-column;
			row = 1-row;
		}
	}

	switch(column)
	{
	case 0:
		//==================
		// move hoist
		//==================
		if(row)
		{
			// hoist away
			if(control->style == 0 || control->style == 1)
				control->crane_increment = 1;
			else
				control->crane_increment = -1;
		}
		else
		{
			// hoist toward
			if(control->style == 0 || control->style == 1)
				control->crane_increment = -1;
			else
				control->crane_increment = 1;
		}
		if(hoist->movedir[0] > 0)
		{
			// hoist travels in X
			dir = 0;
			if(control->crane_increment > 0)
			{
				if(Crane_Hook_Bonk(hook,0,1,bonk))
				{
					bonk[0] += hoist->absmax[0] - hook->absmax[0];
					hoist->crane_bonk = min(bonk[0],hoist->pos2[0]);
				}
				else
					hoist->crane_bonk = hoist->pos2[0];
				hoist->crane_bonk += hoist->absmin[0] - hoist->absmax[0];
			}
			else
			{
				if(Crane_Hook_Bonk(hook,0,-1,bonk))
				{
					bonk[0] += hoist->absmin[0] - hook->absmin[0];
					hoist->crane_bonk = max(bonk[0],hoist->pos1[0]);
				}
				else
					hoist->crane_bonk = hoist->pos1[0];
			}
		}
		else
		{
			// travels in Y
			dir = 1;
			if(control->crane_increment > 0)
			{
				if(Crane_Hook_Bonk(hook,1,1,bonk))
				{
					bonk[1] += hoist->absmax[1] - hook->absmax[1];
					hoist->crane_bonk = min(bonk[1],hoist->pos2[1]);
				}
				else
					hoist->crane_bonk = hoist->pos2[1];
				hoist->crane_bonk += hoist->absmin[1] - hoist->absmax[1];

			}
			else
			{
				if(Crane_Hook_Bonk(hook,1,-1,bonk))
				{
					bonk[1] += hoist->absmin[1] - hook->absmin[1];
					hoist->crane_bonk = max(bonk[1],hoist->pos1[1]);
				}
				else
					hoist->crane_bonk = hoist->pos1[1];
			}
		}
		hoist->crane_dir = dir;
		hoist->moveinfo.remaining_distance = control->crane_increment *
			(hoist->crane_bonk - hoist->absmin[dir]);
		if(hoist->moveinfo.remaining_distance <= 0) return;

		hoist->moveinfo.remaining_distance = min(hoist->moveinfo.remaining_distance,STEPSIZE);
		Crane_AdjustSpeed(hoist);
		VectorSet(hoist->moveinfo.dir,
			hoist->movedir[0]*control->crane_increment,
			hoist->movedir[1]*control->crane_increment,
			0);
		hoist->crane_control = control;

		hook->crane_dir  = dir;
		hook->crane_bonk = hoist->crane_bonk + hook->absmin[dir] -
			hoist->absmin[dir];
		hook->crane_control = control;
		memcpy(&hook->moveinfo,&hoist->moveinfo,sizeof(moveinfo_t));

		cable->crane_dir  = dir;
		cable->crane_bonk = hoist->crane_bonk + cable->absmin[dir] -
			hoist->absmin[dir];
		cable->crane_control = control;
		memcpy(&cable->moveinfo,&hoist->moveinfo,sizeof(moveinfo_t));

		if(cargo)
		{
			cargo->movetype   = MOVETYPE_PUSH;
			cargo->crane_dir  = dir;
			cargo->crane_bonk = hoist->crane_bonk + cargo->absmin[dir] -
				hoist->absmin[dir];
			cargo->crane_control = control;
			memcpy(&cargo->moveinfo,&hoist->moveinfo,sizeof(moveinfo_t));
		}
		Crane_Move_Begin(hoist);
		Crane_Move_Begin(hook);
		if(cargo) Crane_Move_Begin(cargo);
		break;
	case 1:
		//==================
		// move beam
		//==================
		// first re-parent associated speaker, if any
		if(beam->speaker && control == beam->crane_onboard_control)
		{
			beam->speaker->owner = control;
			VectorAdd(control->absmin,control->absmax,beam->speaker->s.origin);
			VectorScale(beam->speaker->s.origin,0.5,beam->speaker->s.origin);
			VectorSubtract(beam->speaker->s.origin,control->s.origin,beam->speaker->offset);
			control->noise_index = beam->noise_index;
		}
		if(row)
		{
			// left arrow
			if(control->style == 0 || control->style == 3)
				control->crane_increment =  1;
			else
				control->crane_increment = -1;
		}
		else
		{
			// right arrow
			if(control->style == 0 || control->style == 3)
				control->crane_increment = -1;
			else
				control->crane_increment =  1;
		}
		if(beam->movedir[0] > 0)
		{
			// travels in X
			dir = 0;
			if(control->crane_increment > 0)
			{
				if(Crane_Hook_Bonk(hook,0,1,bonk))
				{
					bonk[0] += beam->absmax[0] - hook->absmax[0];
					beam->crane_bonk = min(bonk[0],beam->pos2[0]);
				}
				else
					beam->crane_bonk = beam->pos2[0];
				beam->crane_bonk += beam->absmin[0] - beam->absmax[0];
			}
			else
			{
				if(Crane_Hook_Bonk(hook,0,-1,bonk))
				{
					bonk[0] += beam->absmin[0] - hook->absmin[0];
					beam->crane_bonk = max(bonk[0],beam->pos1[0]);
				}
				else
					beam->crane_bonk = beam->pos1[0];
			}
		}
		else
		{
			// travels in Y
			dir = 1;
			if(control->crane_increment > 0)
			{
				if(Crane_Hook_Bonk(hook,1,1,bonk))
				{
					bonk[1] += beam->absmax[1] - hook->absmax[1];
					beam->crane_bonk = min(bonk[1],beam->pos2[1]);
				}
				else
					beam->crane_bonk = beam->pos2[1];
				beam->crane_bonk += beam->absmin[1] - beam->absmax[1];
			}
			else
			{
				if(Crane_Hook_Bonk(hook,1,-1,bonk))
				{
					bonk[1] += beam->absmin[1] - hook->absmin[1];
					beam->crane_bonk = max(bonk[1],beam->pos1[1]);
				}
				else
					beam->crane_bonk = beam->pos1[1];
			}
		}
		beam->crane_dir = dir;
		beam->moveinfo.remaining_distance = control->crane_increment *
			(beam->crane_bonk - beam->absmin[dir]);
//		gi.dprintf("remaining distance = %g\n",beam->moveinfo.remaining_distance);
		if(beam->moveinfo.remaining_distance <= 0) return;
		beam->moveinfo.remaining_distance = min(beam->moveinfo.remaining_distance,STEPSIZE);

		Crane_AdjustSpeed(beam);

		VectorSet(beam->moveinfo.dir,
			beam->movedir[0]*control->crane_increment,
			beam->movedir[1]*control->crane_increment,
			0);
		beam->crane_control = control;

		hoist->crane_dir  = dir;
		hoist->crane_bonk = beam->crane_bonk + hoist->absmin[dir] - beam->absmin[dir];
		hoist->crane_control = control;
		memcpy(&hoist->moveinfo,&beam->moveinfo,sizeof(moveinfo_t));

		hook->crane_dir   = dir;
		hook->crane_bonk  = beam->crane_bonk + hook->absmin[dir] - beam->absmin[dir];
		hook->crane_control = control;
		memcpy(&hook->moveinfo,&beam->moveinfo,sizeof(moveinfo_t));

		cable->crane_dir  = dir;
		cable->crane_bonk = beam->crane_bonk + cable->absmin[dir] -
			beam->absmin[dir];
		cable->crane_control = control;
		memcpy(&cable->moveinfo,&beam->moveinfo,sizeof(moveinfo_t));
		
		if(beam->crane_onboard_control)
		{
			beam->crane_onboard_control->crane_dir  = dir;
			beam->crane_onboard_control->crane_bonk = beam->crane_bonk +
				beam->crane_onboard_control->absmin[dir] -
				beam->absmin[dir];
			beam->crane_onboard_control->crane_control = control;
			memcpy(&beam->crane_onboard_control->moveinfo,&beam->moveinfo,sizeof(moveinfo_t));
		}

		if(cargo)
		{
			cargo->movetype   = MOVETYPE_PUSH;
			cargo->crane_dir  = dir;
			cargo->crane_bonk = beam->crane_bonk + cargo->absmin[dir] - beam->absmin[dir];
			cargo->crane_control = control;
			memcpy(&cargo->moveinfo,&beam->moveinfo,sizeof(moveinfo_t));
		}
		Crane_Move_Begin(beam);
		Crane_Move_Begin(hoist);
		Crane_Move_Begin(hook);
		if(beam->crane_onboard_control)
			Crane_Move_Begin(beam->crane_onboard_control);
		if(cargo) Crane_Move_Begin(cargo);
		break;
	case 2:
		//==================
		// hook up/down
		//==================
		hook->crane_dir = dir = 2;
		if(row)
		{
			// hook up
			control->crane_increment = 1;
			if(Crane_Hook_Bonk(hook,2,1,bonk))
				hook->crane_bonk = min(bonk[2],hook->pos2[2]);
			else
				hook->crane_bonk = hook->pos2[2];
			hook->crane_bonk += hook->absmin[2] - hook->absmax[2];
		}
		else
		{
			// hook down
			if(cargo)
			{
				pt[0] = (cargo->absmin[0] + cargo->absmax[0])/2;
				pt[1] = (cargo->absmin[1] + cargo->absmax[1])/2;
				pt[2] = cargo->absmin[2] - 0.125;
				content = gi.pointcontents(pt);
				if(content & MASK_SOLID)
				{
					BeepBeep(activator);
					return;
				}
			}
			control->crane_increment = -1;
			if(Crane_Hook_Bonk(hook,2,-1,bonk))
				hook->crane_bonk = max(bonk[2],hook->pos1[2]);
			else
				hook->crane_bonk = hook->pos1[2];
		}
		hook->moveinfo.remaining_distance = control->crane_increment *
			(hook->crane_bonk - hook->absmin[hook->crane_dir]);
		if(hook->moveinfo.remaining_distance <= 0)
		{
			BeepBeep(activator);
			return;
		}
		hook->moveinfo.remaining_distance = min(hook->moveinfo.remaining_distance,STEPSIZE);
		Crane_AdjustSpeed(hook);
		VectorSet(hook->moveinfo.dir,0.,0.,(float)(control->crane_increment));
		hook->crane_control = control;
		if(cargo)
		{
			cargo->movetype   = MOVETYPE_PUSH;
			cargo->crane_dir  = dir;
			cargo->crane_bonk = hook->crane_bonk + cargo->absmin[dir] - hook->absmin[dir];
			cargo->crane_control = control;
			VectorSubtract(cargo->s.origin,hook->s.origin,cargo->offset);
			memcpy(&cargo->moveinfo,&hook->moveinfo,sizeof(moveinfo_t));
		}
		cable->think = Cable_Think;
		cable->nextthink = level.time + FRAMETIME;

		Crane_Move_Begin(hook);
		if(cargo) Crane_Move_Begin(cargo);
		break;
	case 3:
		//==================
		// hook/unhook
		//==================
		if(row)
		{
			// pickup cargo

			if(hook->crane_cargo)
			{
				// already carrying something
				BeepBeep(activator);
				return;
			}
			VectorAdd(hook->absmin,hook->absmax,start);
			VectorScale(start,0.5,start);
			VectorSet(forward,0.,0.,-1.);
			VectorMA(start, 8192, forward, end);
			VectorSubtract(hook->absmin,start,mins);
			VectorSubtract(hook->absmax,start,maxs);
			// 06/03/00 change: Use 1/3 the bounding box to force a better hit
			VectorScale(mins,0.3333,mins);
			VectorScale(maxs,0.3333,maxs);
			// end 06/03/00 change
			tr=gi.trace(start, mins, maxs, end, hook, MASK_SOLID);
			if((tr.fraction < 1) && (tr.ent) && (tr.ent->classname) &&
				(tr.ent->movetype == MOVETYPE_PUSHABLE) )
			{
				Z = hook->absmin[2] - tr.ent->absmax[2];
				if(Z > MAX_PICKUP_DISTANCE)
				{
					gi.centerprintf(activator,"Too far\n");
					return;
				}
				if(CrateOnTop(NULL,tr.ent))
				{
					BeepBeep(activator);
					gi.dprintf("Too heavy\n");
					return;
				}
				// run a trace from top of cargo up... if first entity hit is NOT
				// the hook, we can't get there from here.
				if( Z > 0 )
				{
					trace_t tr2;

					VectorMA(tr.ent->mins,0.5,tr.ent->size,start);
					start[2] = tr.ent->maxs[2];
					VectorCopy(tr.ent->size,mins);
					VectorScale(mins,-0.5,mins);
					VectorCopy(tr.ent->size,maxs);
					VectorScale(maxs,0.5,maxs);
					mins[2] = maxs[2] = 0;
					mins[0] += 1; mins[1] += 1; maxs[0] -= 1; maxs[1] -= 1;
					VectorCopy(start,end);
					end[2] += Z + 1;
					tr2=gi.trace(start, mins, maxs, end, hook, MASK_SOLID);
					if((tr2.fraction < 1) && tr2.ent && (tr2.ent != hook))
					{
						gi.centerprintf(activator,"Blocked!\n");
						return;
					}
				}
				Z -= CARGO_BUFFER;   // leave a buffer between hook and cargo
				hook->crane_cargo    = cargo = tr.ent;
				cargo->groundentity  = NULL;
				cargo->crane_control = control;
				cargo->crane_hook    = hook;
				cargo->movetype      = MOVETYPE_PUSH;
				cargo->touch         = NULL;
				// Make cargo float up to the hook
				if(Z > 0)
				{
					control->busy      = true;
					cargo->attracted   = true;
					cargo->gravity     = 0.0;
					cargo->velocity[2] = 0.0;
					cargo->think       = Cargo_Float_Up;
					cargo->blocked     = cargo_blocked;
					cargo->goal_frame  = level.framenum;
					cargo->nextthink   = level.time + FRAMETIME;
					gi.linkentity(cargo);
				}
				else
				{
					gi.positioned_sound (start, cargo, CHAN_VOICE,
						gi.soundindex("tank/thud.wav"), 1, 1, 0);
				}
			}
			else
				BeepBeep(activator);
		}
		else
		{
			// drop cargo

			if(hook->crane_cargo)
			{
				hook->crane_cargo->gravity    = 1.0;
				hook->crane_cargo->movetype   = MOVETYPE_PUSHABLE;
				hook->crane_cargo->touch      = box_touch;
				hook->crane_cargo->crane_control = NULL;
				gi.linkentity(hook->crane_cargo);
				box_movestep (hook->crane_cargo, vec3_origin, true);
				hook->crane_cargo = NULL;
			}
			else
				BeepBeep(activator);
		}
	}
}

void Use_Crane_Control (edict_t *ent, edict_t *other, edict_t *activator)
{ 
	ent->spawnflags ^= 1;
	if(ent->crane_hook->speaker)
		ent->crane_hook->speaker->spawnflags = 1 - (ent->spawnflags & 1);
}

void SP_crane_control (edict_t *self)
{
	if (!self->target)
	{
		gi.dprintf ("crane_control with no target at %s\n", vtos(self->s.origin));
		G_FreeEdict (self);
		return;
	}
	self->classname = "crane_control";
	self->solid     = SOLID_BSP;
	self->movetype  = MOVETYPE_PUSH;
	gi.setmodel (self, self->model);
	self->use       = Use_Crane_Control;
	gi.linkentity (self);
}

void SP_crane_hook (edict_t *self)
{
	vec3_t origin;
	edict_t *speaker;

	gi.setmodel (self, self->model);
	VectorAdd(self->absmin,self->absmax,origin);
	VectorScale(origin,0.5,origin);

	if (!self->targetname)
	{
		gi.dprintf ("crane_hook with no targetname at %s\n", vtos(origin));
		G_FreeEdict (self);
		return;
	}
	self->classname = "crane_hook";
	self->solid     = SOLID_BSP;
	self->movetype  = MOVETYPE_PUSH;
	if(st.noise)
		self->noise_index = gi.soundindex(st.noise);
	else
		self->noise_index = 0;

#ifdef LOOP_SOUND_ATTENUATION
	if (self->attenuation <= 0)
		self->attenuation = ATTN_IDLE;
#endif

	if(!self->speed) self->speed = 160;
	self->moveinfo.speed = self->speed;
	gi.linkentity (self);

	if(self->noise_index && !VectorLength(self->s.origin) )
	{
		speaker = G_Spawn();
		speaker->classname   = "moving_speaker";
		speaker->s.sound     = 0;
		speaker->volume      = 1;
		speaker->attenuation = 1;
		speaker->owner       = self;
		speaker->think       = Moving_Speaker_Think;
		speaker->nextthink   = level.time + 2*FRAMETIME;
		speaker->spawnflags  = 0;       // plays constantly
		self->speaker        = speaker;
		VectorAdd(self->absmin,self->absmax,speaker->s.origin);
		VectorScale(speaker->s.origin,0.5,speaker->s.origin);
		VectorSubtract(speaker->s.origin,self->s.origin,speaker->offset);
	}

}


void SP_crane_hoist (edict_t *self)
{
	vec3_t origin;
	edict_t *speaker;

	gi.setmodel (self, self->model);
	VectorAdd(self->absmin,self->absmax,origin);
	VectorScale(origin,0.5,origin);

	if (!self->targetname)
	{
		gi.dprintf ("crane_hoist with no targetname at %s\n", vtos(origin));
		G_FreeEdict (self);
		return;
	}
	if (!self->target)
	{
		gi.dprintf ("crane_hoist with no target at %s\n", vtos(origin));
		G_FreeEdict (self);
		return;
	}
	self->classname = "crane_hoist";
	self->solid     = SOLID_BSP;
	self->movetype  = MOVETYPE_PUSH;
	if(!self->speed) self->speed = 160;
	self->moveinfo.speed = self->speed;
	if(st.noise)
		self->noise_index = gi.soundindex(st.noise);
	else
		self->noise_index = 0;

#ifdef LOOP_SOUND_ATTENUATION
	if (self->attenuation <= 0)
		self->attenuation = ATTN_IDLE;
#endif

	gi.linkentity (self);

	if(self->noise_index && !VectorLength(self->s.origin) )
	{
		speaker = G_Spawn();
		speaker->classname   = "moving_speaker";
		speaker->s.sound     = 0;
		speaker->volume      = 1;
		speaker->attenuation = 1;
		speaker->owner       = self;
		speaker->think       = Moving_Speaker_Think;
		speaker->nextthink   = level.time + 2*FRAMETIME;
		speaker->spawnflags  = 7;       // owner must be moving to play
		self->speaker        = speaker;
		VectorAdd(self->absmin,self->absmax,speaker->s.origin);
		VectorScale(speaker->s.origin,0.5,speaker->s.origin);
		VectorSubtract(speaker->s.origin,self->s.origin,speaker->offset);
	}
}


void SP_crane_beam (edict_t *self)
{
	vec3_t  origin;
	edict_t *speaker;

	gi.setmodel (self, self->model);
	VectorAdd(self->absmin,self->absmax,origin);
	VectorScale(origin,0.5,origin);

	if (!self->targetname)
	{
		gi.dprintf ("crane_beam with no targetname at %s\n", vtos(origin));
		G_FreeEdict (self);
		return;
	}
	if (!self->target)
	{
		gi.dprintf ("crane_beam with no target at %s\n", vtos(origin));
		G_FreeEdict (self);
		return;
	}
	if (!self->pathtarget)
	{
		gi.dprintf ("crane_beam with no pathtarget at %s\n", vtos(origin));
		G_FreeEdict (self);
		return;
	}
	self->classname = "crane_beam";
	self->solid     = SOLID_BSP;
	self->movetype  = MOVETYPE_PUSH;

	if(!self->speed) self->speed = 160;
	self->moveinfo.speed = self->speed;
	if(st.noise)
		self->noise_index = gi.soundindex(st.noise);
	else
		self->noise_index = 0;

#ifdef LOOP_SOUND_ATTENUATION
	if (self->attenuation <= 0)
		self->attenuation = ATTN_IDLE;
#endif

	gi.linkentity (self);

	if(self->noise_index && !VectorLength(self->s.origin) )
	{
		speaker = G_Spawn();
		speaker->classname   = "moving_speaker";
		speaker->s.sound     = 0;
		speaker->volume      = 1;
		speaker->attenuation = 1;
		speaker->think       = Moving_Speaker_Think;
		speaker->nextthink   = level.time + 2*FRAMETIME;
		speaker->spawnflags  = 7;       // owner must be moving to play
		speaker->owner       = self;    // this will be changed later when we know
		                                // controls are spawned
		self->speaker        = speaker;
		VectorAdd(self->absmin,self->absmax,speaker->s.origin);
		VectorScale(speaker->s.origin,0.5,speaker->s.origin);
		VectorSubtract(speaker->s.origin,self->s.origin,speaker->offset);
	}
}

//===================================================================================

//QUAKED crane_reset - special purpose trigger_relay that calls the associated crane
// to the beam extent closest to the crane_reset. Typically crane_reset is targeted
// by a func_button

void crane_reset_go (edict_t *temp)
{
	edict_t *control;
	control = temp->owner;
	Crane_Move_Begin(control->crane_beam);
	Crane_Move_Begin(control->crane_hoist);
	Crane_Move_Begin(control->crane_hook);
	if(control->crane_beam->crane_onboard_control)
		Crane_Move_Begin(control->crane_beam->crane_onboard_control);
	if(control->crane_hook->crane_cargo) Crane_Move_Begin(control->crane_hook->crane_cargo);
	G_FreeEdict(temp);
}

void crane_reset_use (edict_t *self, edict_t *other, edict_t *activator)
{
	float   d1, d2;
	int     dir;
	edict_t *delay;
	edict_t *crane;
	edict_t *control, *beam, *cable, *cargo, *hoist, *hook;
	vec3_t  bonk, v1, v2;
	

	crane = G_Find (NULL, FOFS(targetname), self->target);
	if(!crane)
	{
		gi.dprintf("Cannot find target of crane_reset at %s\n",vtos(self->s.origin));
		return;
	}
	control = crane->crane_control;
	control->activator = activator;

	if(!(control->spawnflags & 1))
	{
		if(control->message)
			gi.centerprintf(activator,"%s\n",control->message);
		else
			gi.centerprintf(activator,"No power\n");
		return;
	}
	
	beam    = control->crane_beam;
	hoist   = control->crane_hoist;
	hook    = control->crane_hook;
	cable   = control->crane_cable;
	cargo   = hook->crane_cargo;
	VectorSubtract(beam->pos1,self->s.origin,v1);
	VectorSubtract(beam->pos2,self->s.origin,v2);
	d1 = VectorLength(v1);
	d2 = VectorLength(v2);

	if(d2 < d1)
		control->crane_increment = 1;
	else
		control->crane_increment = -1;

	if(beam->movedir[0] > 0)
	{
		// travels in X
		dir = 0;
		if(control->crane_increment > 0)
		{
			if(Crane_Hook_Bonk(hook,0,1,bonk))
			{
				bonk[0] += beam->absmax[0] - hook->absmax[0];
				beam->crane_bonk = min(bonk[0],beam->pos2[0]);
			}
			else
				beam->crane_bonk = beam->pos2[0];
			beam->crane_bonk += beam->absmin[0] - beam->absmax[0];
		}
		else
		{
			if(Crane_Hook_Bonk(hook,0,-1,bonk))
			{
				bonk[0] += beam->absmin[0] - hook->absmin[0];
				beam->crane_bonk = max(bonk[0],beam->pos1[0]);
			}
			else
				beam->crane_bonk = beam->pos1[0];
		}
	}
	else
	{
		// travels in Y
		dir = 1;
		if(control->crane_increment > 0)
		{
			if(Crane_Hook_Bonk(hook,1,1,bonk))
			{
				bonk[1] += beam->absmax[1] - hook->absmax[1];
				beam->crane_bonk = min(bonk[1],beam->pos2[1]);
			}
			else
				beam->crane_bonk = beam->pos2[1];
			beam->crane_bonk += beam->absmin[1] - beam->absmax[1];
		}
		else
		{
			if(Crane_Hook_Bonk(hook,1,-1,bonk))
			{
				bonk[1] += beam->absmin[1] - hook->absmin[1];
				beam->crane_bonk = max(bonk[1],beam->pos1[1]);
			}
			else
				beam->crane_bonk = beam->pos1[1];
		}
	}

	if(beam->speaker && beam->crane_onboard_control)
	{
		beam->speaker->owner = beam->crane_onboard_control;
		VectorAdd(beam->crane_onboard_control->absmin,
			      beam->crane_onboard_control->absmax,
				  beam->speaker->s.origin);
		VectorScale(beam->speaker->s.origin,0.5,beam->speaker->s.origin);
		VectorSubtract(beam->speaker->s.origin,
			           beam->crane_onboard_control->s.origin,beam->speaker->offset);
		beam->speaker->owner->noise_index = beam->noise_index;
	}

	beam->crane_dir = dir;
	beam->moveinfo.remaining_distance = control->crane_increment *
		(beam->crane_bonk - beam->absmin[dir]);
	if(beam->moveinfo.remaining_distance <= 0) return;

	Crane_AdjustSpeed(beam);

	VectorSet(beam->moveinfo.dir,
		beam->movedir[0]*control->crane_increment,
		beam->movedir[1]*control->crane_increment,
		0);
	beam->crane_control = control;

	hoist->crane_dir  = dir;
	hoist->crane_bonk = beam->crane_bonk + hoist->absmin[dir] - beam->absmin[dir];
	hoist->crane_control = control;
	memcpy(&hoist->moveinfo,&beam->moveinfo,sizeof(moveinfo_t));

	hook->crane_dir   = dir;
	hook->crane_bonk  = beam->crane_bonk + hook->absmin[dir] - beam->absmin[dir];
	hook->crane_control = control;
	memcpy(&hook->moveinfo,&beam->moveinfo,sizeof(moveinfo_t));

	cable->crane_dir  = dir;
	cable->crane_bonk = beam->crane_bonk + cable->absmin[dir] - beam->absmin[dir];
	cable->crane_control = control;
	memcpy(&cable->moveinfo,&beam->moveinfo,sizeof(moveinfo_t));
		
	if(beam->crane_onboard_control)
	{
		beam->crane_onboard_control->crane_dir  = dir;
		beam->crane_onboard_control->crane_bonk = beam->crane_bonk +
			beam->crane_onboard_control->absmin[dir] -
			beam->absmin[dir];
		beam->crane_onboard_control->crane_control = control;
		memcpy(&beam->crane_onboard_control->moveinfo,&beam->moveinfo,sizeof(moveinfo_t));
	}

	if(cargo)
	{
		cargo->crane_dir  = dir;
		cargo->crane_bonk = beam->crane_bonk + cargo->absmin[dir] - beam->absmin[dir];
		cargo->crane_control = control;
		memcpy(&cargo->moveinfo,&beam->moveinfo,sizeof(moveinfo_t));
	}
	delay = G_Spawn();
	delay->owner     = control;
	delay->think     = crane_reset_go;
	delay->nextthink = level.time + FRAMETIME;
	gi.linkentity(delay);

	self->count--;
	if(!self->count) {
		self->think = G_FreeEdict;
		self->nextthink = level.time + 1;
	}
}

void SP_crane_reset (edict_t *self)
{
	if (!self->targetname)
	{
		gi.dprintf ("crane_reset with no targetname at %s\n", vtos(self->s.origin));
		G_FreeEdict (self);
		return;
	}
	if (!self->target)
	{
		gi.dprintf ("crane_reset with no target at %s\n", vtos(self->s.origin));
		G_FreeEdict (self);
		return;
	}
	self->use = crane_reset_use;
}

