#include "g_local.h"

/* Func_reflect is adopted from psychospaz' original floor reflection code. See
   psychospaz' stuff at http://modscape.telefragged.com

   Chief differences:
   1) The obvious - reflections work in 6 directions, not just the floor.
   2) Uses a new entity rather than automatically doing reflections based on surface
      or content properties of the floor.
   3) Most TE_ effects are reflected, in addition to entities. This requires calls to
      the appropriate ReflectXXXXX routine in several places scattered around the code.
   4) Roll angle is correct.

  You can have up to 16 func_reflects in one map. To increase that number change
  MAX_MIRRORS below. The only reason to use a static limit is that the func_reflect
  entity addresses get copied to g_mirror, which makes searching for func_reflects
  much easier.
*/

#define MAX_MIRRORS 16
edict_t	*g_mirror[MAX_MIRRORS];

#define SF_REFLECT_OFF    1
#define SF_REFLECT_TOGGLE 2

void ReflectExplosion (int type, vec3_t origin)
{
	int		m;
	edict_t	*mirror;
	vec3_t	org;

	if(!level.num_reflectors)
		return;

	for(m=0; m<level.num_reflectors; m++)
	{
		mirror = g_mirror[m];

		if(!mirror->inuse)
			continue;
		if(mirror->spawnflags & SF_REFLECT_OFF)
			continue;

		// Don't reflect explosions (other than BFG) from floor or ceiling, 
		// 'cuz there's no way to do it right
		if((mirror->style <= 1) && (type != TE_BFG_EXPLOSION) && (type != TE_BFG_BIGEXPLOSION))
			continue;

		VectorCopy(origin,org);
		switch(mirror->style)
		{
			case 0: org[2] = 2*mirror->absmax[2] - origin[2] + mirror->moveinfo.distance - 2; break;
			case 1: org[2] = 2*mirror->absmin[2] - origin[2] - mirror->moveinfo.distance + 2; break;
			case 2: org[0] = 2*mirror->absmin[0] - origin[0] + mirror->moveinfo.distance + 2; break;
			case 3: org[0] = 2*mirror->absmax[0] - origin[0] - mirror->moveinfo.distance - 2; break;
			case 4: org[1] = 2*mirror->absmin[1] - origin[1] + mirror->moveinfo.distance + 2; break;
			case 5: org[1] = 2*mirror->absmax[1] - origin[1] - mirror->moveinfo.distance - 2; break;
		}
		if(org[0] < mirror->absmin[0]) continue;
		if(org[0] > mirror->absmax[0]) continue;
		if(org[1] < mirror->absmin[1]) continue;
		if(org[1] > mirror->absmax[1]) continue;
		if(org[2] < mirror->absmin[2]) continue;
		if(org[2] > mirror->absmax[2]) continue;

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (type);
		gi.WritePosition (org);
		gi.multicast (org, MULTICAST_PVS);
	}
}

void ReflectTrail (int type, vec3_t start, vec3_t end)
{
	int		m;
	edict_t	*mirror;
	vec3_t	p1, p2;

	if(!level.num_reflectors)
		return;

	for(m=0; m<level.num_reflectors; m++)
	{
		mirror = g_mirror[m];

		if(!mirror->inuse)
			continue;
		if(mirror->spawnflags & SF_REFLECT_OFF)
			continue;

		VectorCopy(start,p1);
		VectorCopy(end,  p2);
		switch(mirror->style)
		{
			case 0:
				p1[2] = 2*mirror->absmax[2] - start[2] + mirror->moveinfo.distance + 2;
				p2[2] = 2*mirror->absmax[2] -   end[2] + mirror->moveinfo.distance + 2;
				break;
			case 1:
				p1[2] = 2*mirror->absmin[2] - start[2] - mirror->moveinfo.distance - 2;
				p2[2] = 2*mirror->absmin[2] -   end[2] - mirror->moveinfo.distance - 2;
				break;
			case 2:
				p1[0] = 2*mirror->absmin[0] - start[0] + mirror->moveinfo.distance + 2;
				p2[0] = 2*mirror->absmin[0] -   end[0] + mirror->moveinfo.distance + 2;
				break;
			case 3:
				p1[0] = 2*mirror->absmax[0] - start[0] - mirror->moveinfo.distance - 2;
				p2[0] = 2*mirror->absmax[0] -   end[0] - mirror->moveinfo.distance - 2;
				break;
			case 4:
				p1[1] = 2*mirror->absmin[1] - start[1] + mirror->moveinfo.distance + 2;
				p2[1] = 2*mirror->absmin[1] -   end[1] + mirror->moveinfo.distance + 2;
				break;
			case 5:
				p1[1] = 2*mirror->absmax[1] - start[1] - mirror->moveinfo.distance - 2;
				p2[1] = 2*mirror->absmax[1] -   end[1] - mirror->moveinfo.distance - 2;
				break;
		}
		if(p1[0] < mirror->absmin[0]) continue;
		if(p1[0] > mirror->absmax[0]) continue;
		if(p1[1] < mirror->absmin[1]) continue;
		if(p1[1] > mirror->absmax[1]) continue;
		if(p1[2] < mirror->absmin[2]) continue;
		if(p1[2] > mirror->absmax[2]) continue;

		// If p1 is within func_reflect, we assume p2 is also. If map is constructed 
		// properly this should always be true.

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (type);
		gi.WritePosition (p1);
		gi.WritePosition (p2);
		gi.multicast (p1, MULTICAST_PVS);
	}
}

void ReflectSteam (vec3_t origin,vec3_t movedir,int count,int sounds,int speed, int wait, int nextid)
{
	int		m;
	edict_t	*mirror;
	vec3_t	org, dir;

	if(!level.num_reflectors)
		return;

	for(m=0; m<level.num_reflectors; m++)
	{
		mirror = g_mirror[m];

		if(!mirror->inuse)
			continue;
		if(mirror->spawnflags & SF_REFLECT_OFF)
			continue;

		VectorCopy(origin,org);
		VectorCopy(movedir,dir);
		switch(mirror->style)
		{
			case 0:
				org[2] = 2*mirror->absmax[2] - origin[2] + mirror->moveinfo.distance + 2;
				dir[2] = -dir[2];
				break;
			case 1:
				org[2] = 2*mirror->absmin[2] - origin[2] - mirror->moveinfo.distance - 2;
				dir[2] = -dir[2];
				break;
			case 2:
				org[0] = 2*mirror->absmin[0] - origin[0] + mirror->moveinfo.distance + 2;
				dir[0] = -dir[0];
				break;
			case 3:
				org[0] = 2*mirror->absmax[0] - origin[0] - mirror->moveinfo.distance - 2;
				dir[0] = -dir[0];
				break;
			case 4:
				org[1] = 2*mirror->absmin[1] - origin[1] + mirror->moveinfo.distance + 2;
				dir[1] = -dir[1];
				break;
			case 5:
				org[1] = 2*mirror->absmax[1] - origin[1] - mirror->moveinfo.distance - 2;
				dir[1] = -dir[1];
				break;
		}
		if(org[0] < mirror->absmin[0]) continue;
		if(org[0] > mirror->absmax[0]) continue;
		if(org[1] < mirror->absmin[1]) continue;
		if(org[1] > mirror->absmax[1]) continue;
		if(org[2] < mirror->absmin[2]) continue;
		if(org[2] > mirror->absmax[2]) continue;

		// If p1 is within func_reflect, we assume p2 is also. If map is constructed 
		// properly this should always be true.

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_STEAM);
		gi.WriteShort (nextid);
		gi.WriteByte (count);
		gi.WritePosition (org);
		gi.WriteDir (dir);
		gi.WriteByte (sounds&0xff);
		gi.WriteShort (speed);
		gi.WriteLong (wait);
		gi.multicast (org, MULTICAST_PVS);
	}
}

void ReflectSparks (int type,vec3_t origin,vec3_t movedir)
{
	int		m;
	edict_t	*mirror;
	vec3_t	org, dir;

	if(!level.num_reflectors)
		return;

	for(m=0; m<level.num_reflectors; m++)
	{
		mirror = g_mirror[m];

		if(!mirror->inuse)
			continue;
		if(mirror->spawnflags & SF_REFLECT_OFF)
			continue;
		// Don't reflect in floor or ceiling because sparks are affected by
		// gravity
		if(mirror->style <= 1)
			continue;

		VectorCopy(origin,org);
		VectorCopy(movedir,dir);
		switch(mirror->style)
		{
			case 0:
				org[2] = 2*mirror->absmax[2] - origin[2] + mirror->moveinfo.distance + 2;
				dir[2] = -dir[2];
				break;
			case 1:
				org[2] = 2*mirror->absmin[2] - origin[2] - mirror->moveinfo.distance - 2;
				dir[2] = -dir[2];
				break;
			case 2:
				org[0] = 2*mirror->absmin[0] - origin[0] + mirror->moveinfo.distance + 2;
				dir[0] = -dir[0];
				break;
			case 3:
				org[0] = 2*mirror->absmax[0] - origin[0] - mirror->moveinfo.distance - 2;
				dir[0] = -dir[0];
				break;
			case 4:
				org[1] = 2*mirror->absmin[1] - origin[1] + mirror->moveinfo.distance + 2;
				dir[1] = -dir[1];
				break;
			case 5:
				org[1] = 2*mirror->absmax[1] - origin[1] - mirror->moveinfo.distance - 2;
				dir[1] = -dir[1];
				break;
		}
		if(org[0] < mirror->absmin[0]) continue;
		if(org[0] > mirror->absmax[0]) continue;
		if(org[1] < mirror->absmin[1]) continue;
		if(org[1] > mirror->absmax[1]) continue;
		if(org[2] < mirror->absmin[2]) continue;
		if(org[2] > mirror->absmax[2]) continue;

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(type);
		gi.WritePosition(org);
		if(type != TE_CHAINFIST_SMOKE) 
			gi.WriteDir(dir);
		gi.multicast(org, MULTICAST_PVS);

	}
}

void DeleteReflection (edict_t *ent, int index)
{
	edict_t	*r;

	if(index < 0)
	{
		int	i;
		for(i=0; i<6; i++)
		{
			r = ent->reflection[i];
			if(r)
			{
				if(r->client)
			//		free(r->client);
					gi.TagFree(r->client);
				memset (r, 0, sizeof(*r));
				r->classname = "freed";
				r->freetime = level.time;
				r->inuse = false;
			}
			ent->reflection[i] = NULL;
		}
	}
	else
	{
		r = ent->reflection[index];
		if(r)
		{
			if(r->client)
		//		free(r->client);
				gi.TagFree(r->client);
			memset (r, 0, sizeof(*r));
			r->classname = "freed";
			r->freetime = level.time;
			r->inuse = false;
			ent->reflection[index] = NULL;
		}
	}
}

void AddReflection (edict_t *ent)
{
	gclient_t	*cl;
	edict_t		*mirror;
	float		roll;
	int			i, m;
	qboolean	is_reflected;
	vec3_t		forward;
	vec3_t		org;

	for(i=0; i<6; i++)
	{
		is_reflected = false;
		for(m=0; m<level.num_reflectors && !is_reflected; m++)
		{
			mirror = g_mirror[m];
			if(!mirror->inuse)
				continue;
			if(mirror->spawnflags & SF_REFLECT_OFF)
				continue;
			if(mirror->style != i)
				continue;
			VectorCopy(ent->s.origin,org);
			switch(i)
			{
			case 0: org[2] = 2*mirror->absmax[2] - ent->s.origin[2] - mirror->moveinfo.distance - 2; break;
			case 1: org[2] = 2*mirror->absmin[2] - ent->s.origin[2] + mirror->moveinfo.distance + 2; break;
			case 2: org[0] = 2*mirror->absmin[0] - ent->s.origin[0] + mirror->moveinfo.distance + 2; break;
			case 3: org[0] = 2*mirror->absmax[0] - ent->s.origin[0] - mirror->moveinfo.distance - 2; break;
			case 4: org[1] = 2*mirror->absmin[1] - ent->s.origin[1] + mirror->moveinfo.distance + 2; break;
			case 5: org[1] = 2*mirror->absmax[1] - ent->s.origin[1] - mirror->moveinfo.distance - 2; break;
			}
			if(org[0] < mirror->absmin[0]) continue;
			if(org[0] > mirror->absmax[0]) continue;
			if(org[1] < mirror->absmin[1]) continue;
			if(org[1] > mirror->absmax[1]) continue;
			if(org[2] < mirror->absmin[2]) continue;
			if(org[2] > mirror->absmax[2]) continue;
			is_reflected = true;
		}
		if(is_reflected)
		{
			if (!ent->reflection[i])
			{			
				ent->reflection[i] = G_Spawn();
			
				if(ent->s.effects & EF_ROTATE)
				{
					ent->s.effects &= ~EF_ROTATE;
					gi.linkentity(ent);
				}
				ent->reflection[i]->movetype = MOVETYPE_NONE;
				ent->reflection[i]->solid = SOLID_NOT;
				ent->reflection[i]->classname = "reflection";
				ent->reflection[i]->flags = FL_REFLECT;
				ent->reflection[i]->takedamage = DAMAGE_NO;
			}
			if (ent->client && !ent->reflection[i]->client)
			{
			//	cl = (gclient_t *)malloc(sizeof(gclient_t)); 
				cl = (gclient_t *)gi.TagMalloc(sizeof(gclient_t), TAG_LEVEL); 
				ent->reflection[i]->client = cl; 
			}
			if (ent->client && ent->reflection[i]->client)
			{
//				Lazarus: Hmm.. this crashes when loading saved game.
//				         Not sure what use pers is anyhow?
//				ent->reflection[i]->client->pers = ent->client->pers;
				ent->reflection[i]->s = ent->s;
			}
			ent->reflection[i]->s.number     = ent->reflection[i] - g_edicts;
			ent->reflection[i]->s.modelindex = ent->s.modelindex;
			ent->reflection[i]->s.modelindex2 = ent->s.modelindex2;
			ent->reflection[i]->s.modelindex3 = ent->s.modelindex3;
			ent->reflection[i]->s.modelindex4 = ent->s.modelindex4;
		#ifdef KMQUAKE2_ENGINE_MOD
			ent->reflection[i]->s.modelindex5 = ent->s.modelindex5;
			ent->reflection[i]->s.modelindex6 = ent->s.modelindex6;
			ent->reflection[i]->s.alpha = ent->s.alpha;
		#endif
			ent->reflection[i]->s.skinnum = ent->s.skinnum;
			ent->reflection[i]->s.frame = ent->s.frame;
			ent->reflection[i]->s.renderfx = ent->s.renderfx;
			ent->reflection[i]->s.effects = ent->s.effects;
			ent->reflection[i]->s.renderfx &= ~RF_IR_VISIBLE;		
		#ifdef KMQUAKE2_ENGINE_MOD
			// Don't flip if left handed player model
			if ( !(ent->s.modelindex == (MAX_MODELS-1) && hand->value == 1) )
				ent->reflection[i]->s.renderfx |= RF_MIRRORMODEL; // Knightmare added- flip reflected models
		#endif

			VectorCopy (ent->s.angles, ent->reflection[i]->s.angles);
			switch(i)
			{
			case 0:
			case 1:
				ent->reflection[i]->s.angles[0]+=180;
				ent->reflection[i]->s.angles[1]+=180;
				ent->reflection[i]->s.angles[2]=360-ent->reflection[i]->s.angles[2];
				break;
			case 2:
			case 3:
				AngleVectors(ent->reflection[i]->s.angles,forward,NULL,NULL);
				roll = ent->reflection[i]->s.angles[2];
				forward[0] = -forward[0];
				vectoangles(forward,ent->reflection[i]->s.angles);
				ent->reflection[i]->s.angles[2] = 360-roll;
				break;
			case 4:
			case 5:
				AngleVectors(ent->reflection[i]->s.angles,forward,NULL,NULL);
				roll = ent->reflection[i]->s.angles[2];
				forward[1] = -forward[1];
				vectoangles(forward,ent->reflection[i]->s.angles);
				ent->reflection[i]->s.angles[2] = 360-roll;
			}

			VectorCopy (org, ent->reflection[i]->s.origin);
			if(ent->s.renderfx & RF_BEAM)
			{
				vec3_t	delta;

				VectorSubtract(ent->reflection[i]->s.origin,ent->s.origin,delta);
				VectorAdd(ent->s.old_origin,delta,ent->reflection[i]->s.old_origin);
			}
			else
				VectorCopy(ent->reflection[i]->s.origin, ent->reflection[i]->s.old_origin);
			gi.linkentity (ent->reflection[i]);
		}
		else if (ent->reflection[i])
		{
			DeleteReflection(ent,i);
		}
	}
}

void use_func_reflect (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->spawnflags & SF_REFLECT_OFF)
	{
		// was off, turn it on
		self->spawnflags &= ~SF_REFLECT_OFF;
	}
	else
	{
		// was on, turn it off
		self->spawnflags |= SF_REFLECT_OFF;
	}
	if (!(self->spawnflags & SF_REFLECT_TOGGLE))
		self->use = NULL;
}

void SP_func_reflect (edict_t *self)
{
	if(level.num_reflectors >= MAX_MIRRORS)
	{
		gi.dprintf("Max number of func_reflect's (%d) exceeded.\n",MAX_MIRRORS);
		G_FreeEdict(self);
		return;
	}
	gi.setmodel (self, self->model);
	self->svflags = SVF_NOCLIENT;
	g_mirror[level.num_reflectors] = self;
	if(!st.lip)
		st.lip = 2;
	self->moveinfo.distance = st.lip;
	self->use = use_func_reflect;
	level.num_reflectors++;
}
