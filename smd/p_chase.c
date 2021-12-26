#include "g_local.h"

// Lazarus: removed all waterlevel-dependent stuff, which I don't get, and
//          the fake crosshair, which looked... really bad

cvar_t *tpp;
cvar_t *tpp_auto;
cvar_t *crossh;

void ChasecamTrack (edict_t *ent);

/*	The ent is the owner of the chasecam  */
void ChasecamStart (edict_t *ent)
{
/* This creates a tempory entity we can manipulate within this
	* function */
	edict_t 	 *chasecam;
	/* Don't work on a spectator! */
	if (ent->client->resp.spectator)
		return;
	//Don't turn back on during intermission!
	if (level.intermissiontime)
		return;
		/* Tell everything that looks at the toggle that our chasecam is on
	* and working */
	ent->client->chasetoggle = 1;
	/* Make our gun model "non-existent" so it's more realistic to the
	* player using the chasecam */
	ent->client->ps.gunindex = 0;		 
	chasecam = G_Spawn ();
	chasecam->owner = ent;
	chasecam->solid = SOLID_NOT;
	chasecam->movetype = MOVETYPE_FLYMISSILE;
	ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION; // this turns off Quake2's inclination to predict where the camera is going,
	// making a much smoother ride
	ent->svflags |= SVF_NOCLIENT; // this line tells Quake2 not to send the unnecessary info about the camera to other players
								  /* Now, make the angles of the player model, (!NOT THE HUMAN VIEW!) be
	* copied to the same angle of the chasecam entity */
	VectorCopy (ent->s.angles, chasecam->s.angles); 	   
	/* Clear the size of the entity, so it DOES technically have a size,
	* but that of '0 0 0'-'0 0 0'. (xyz, xyz). mins = Minimum size,
	* maxs = Maximum size */
	VectorClear (chasecam->mins);
	VectorClear (chasecam->maxs);		 
	/* Make the chasecam's origin (position) be the same as the player
	* entity's because as the camera starts, it will force itself out
	* slowly backwards from the player model */
	VectorCopy (ent->s.origin, chasecam->s.origin); 	   
	chasecam->classname = "chasecam";
	chasecam->prethink = ChasecamTrack;
	// Lazarus: Need think???
	chasecam->think = ChasecamTrack;
	ent->client->chasecam = chasecam;	  
	ent->client->oldplayer = G_Spawn();
	CheckChasecam_Viewent(ent);
	//MakeFakeCrosshair(ent);

	// remove reflection of real player, if any
	DeleteReflection(ent,-1);
}

/* ent = chasecam */
void ChasecamRestart (edict_t *ent)
{
/* Keep thinking this function to check all the time whether the
	* player is out of the water */
	/* If the player is dead, the camera is not wanted... Kill me and stop
	* the function. (return;) */
	if (ent->owner->health <= 0)
	{
		G_FreeEdict(ent);
		return;
	}
	/* If the player is still completly underwater, break the routine
	unless tpp has changed!*/
//	if (ent->owner->waterlevel && !tpp->value)
//		return;
	//Put camera back
	ChasecamStart (ent->owner);
	//Remove this temporary ent
	G_FreeEdict (ent);
}

/* Here, the "ent" is referring to the client, the player that owns the
* chasecam, and the "opt" integer is telling the function whether to
* totally get rid of the camera, or to put it into the background while
* it checks if the player is out of the water or not. */
void ChasecamRemove (edict_t *ent, int opt)
{
	/* Stop the chasecam from moving */
	VectorClear (ent->client->chasecam->velocity);		  
	/* Make the weapon model of the player appear on screen for 1st
	* person reality and aiming */
	//Don't turn back on during intermission!
	if (!level.intermissiontime)
		ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
	/* Make our invisible appearance the same model as the display entity
	* that mimics us while in chasecam mode */
	ent->s.modelindex = ent->client->oldplayer->s.modelindex;
	ent->svflags &= ~SVF_NOCLIENT;
	
	//DestroyFakeCrosshair (ent);
	
	if (opt == OPTION_BACKGROUND)
	{
		ent->client->chasetoggle = 0;
		G_FreeEdict (ent->client->chasecam);
		G_FreeEdict (ent->client->oldplayer);
		ent->client->oldplayer = NULL;
		ent->client->chasecam = G_Spawn ();
		ent->client->chasecam->owner = ent;
		ent->client->chasecam->solid = SOLID_NOT;
		ent->client->chasecam->movetype = MOVETYPE_FLYMISSILE;
		VectorClear (ent->client->chasecam->mins);
		VectorClear (ent->client->chasecam->maxs);
		ent->client->chasecam->classname = "chasecam";
		ent->client->chasecam->prethink = ChasecamRestart; // begin checking for emergence from the water
		// Lazarus: Need think don't we???
		ent->client->chasecam->think = ChasecamRestart;
	}
	else if (opt == OPTION_OFF)
	{
		G_FreeEdict (ent->client->oldplayer);
		ent->client->oldplayer = NULL;
		ent->client->chasetoggle = 0;
		G_FreeEdict (ent->client->chasecam);
		ent->client->chasecam = NULL;
	}
}

/* The "ent" is the chasecam */
void ChasecamTrack (edict_t *ent)
{
	/* Create tempory vectors and trace variables */
	trace_t 	 tr;
	vec3_t		 spot1, spot2, dir;
	vec3_t		 forward, right, up,angles;
	int 		 distance;
	int 		 tot;		 
	ent->nextthink = level.time + 0.100;
	/* if our owner is under water, run the remove routine to repeatedly
	* check for emergment from water */
//	if (ent->owner->waterlevel && !tpp->value)
//	{
//		ChasecamRemove (ent->owner, OPTION_BACKGROUND);
//		return;
//	}		 
	/* get the CLIENT's angle, and break it down into direction vectors,
	* of forward, right, and up. VERY useful */
	VectorCopy(ent->owner->client->v_angle,angles);
	if (angles[PITCH] > 56)
		angles[PITCH] = 56;
	AngleVectors (angles, forward, right, up);
	VectorNormalize(forward);
	/* go starting at the player's origin, forward, ent->chasedist1
	* distance, and save the location in vector spot2 */
	VectorMA (ent->owner->s.origin, -ent->chasedist1, forward, spot2);
	/* make spot2 a bit higher, by adding viewheight to the Z coordinate */
	spot2[2] += (ent->owner->viewheight + 16);
	// jump animation lifts
	if (!ent->owner->groundentity)
		spot2[2] += 16;
		/* make the tr traceline trace from the player model's position, to spot2,
	* ignoring the player, with a mask. */
	tr = gi.trace (ent->owner->s.origin, vec3_origin, vec3_origin, spot2, ent->owner, MASK_SOLID);
	/* subtract the endpoint from the start point for length and
	* direction manipulation */
	VectorSubtract (tr.endpos, ent->owner->s.origin, spot1);
	/* in this case, length */
	ent->chasedist1 = VectorLength (spot1); 	   
	/* go, starting from the end of the trace, 2 points forward (client
	* angles) and save the location in spot2 */
	VectorMA (tr.endpos, 2, forward, spot2);
	/* make spot1 the same for tempory vector modification and make spot1
	* a bit higher than spot2 */
	VectorCopy (spot2, spot1);
	spot1[2] += 32;
	/* another trace from spot2 to spot1, ignoring player, no masks */
	tr = gi.trace (spot2, vec3_origin, vec3_origin, spot1, ent->owner, MASK_SOLID);
	/* if we hit something, copy the trace end to spot2 and lower spot2 */
	if (tr.fraction < 1.000)
	{
		VectorCopy (tr.endpos, spot2);
		spot2[2] -= 32;
	}
	/* subtract endpos spot2 from startpos the camera origin, saving it to
	* the dir vector, and normalize dir for a direction from the camera
	* origin, to the spot2 */
	VectorSubtract (spot2, ent->s.origin, dir);
	distance = VectorLength (dir);		  
	VectorNormalize (dir);		  
	/* another traceline */
	tr = gi.trace (ent->s.origin, vec3_origin, vec3_origin, spot2, ent->owner, MASK_SOLID);
	/* if we DON'T hit anyting, do some freaky stuff  */
	if (tr.fraction == 1.000)
	{
	/* subtract the endpos camera position, from the startpos, the
	* player, and save in spot1. Normalize spot1 for a direction, and
	* make that direction the angles of the chasecam for copying to the
		* clients view angle which is displayed to the client. (human) */
		VectorSubtract (ent->s.origin, ent->owner->s.origin, spot1);
		VectorNormalize (spot1);
		VectorCopy (spot1, ent->s.angles);
		/* calculate the percentages of the distances, and make sure we're
		* not going too far, or too short, in relation to our panning
		* speed of the chasecam entity */
		tot = (distance * 0.400);
		/* if we're going too fast, make us top speed */
		if (tot > 5.200)
		{
			ent->velocity[0] = ((dir[0] * distance) * 5.2);
			ent->velocity[1] = ((dir[1] * distance) * 5.2);
			ent->velocity[2] = ((dir[2] * distance) * 5.2);
		}
		else
		{
		/* if we're NOT going top speed, but we're going faster than
			* 1, relative to the total, make us as fast as we're going */
			if (tot > 1.000)
			{
				ent->velocity[0] = ((dir[0] * distance) * tot);
				ent->velocity[1] = ((dir[1] * distance) * tot);
				ent->velocity[2] = ((dir[2] * distance) * tot);
			}
			else
			{
			/* if we're not going faster than one, don't accelerate our
				* speed at all, make us go slow to our destination */
				ent->velocity[0] = (dir[0] * distance);
				ent->velocity[1] = (dir[1] * distance);
				ent->velocity[2] = (dir[2] * distance);
			}
		}				 
		/* subtract endpos,player position, from chasecam position to get
		* a length to determine whether we should accelerate faster from
		* the player or not */
		VectorSubtract (ent->owner->s.origin, ent->s.origin, spot1);
		if (VectorLength(spot1) < 20)
		{
			ent->velocity[0] *= 2;
			ent->velocity[1] *= 2; 
			ent->velocity[2] *= 2;
		}
	}
	/* if we DID hit something in the tr.fraction call ages back, then
	* make the spot2 we created, the position for the chasecamera. */
	else
		VectorCopy (spot2, ent->s.origin);
	/* add to the distance between the player and the camera */
	ent->chasedist1 += 2;
	/* if we're too far away, give us a maximum distance */
	if (ent->chasedist1 > (60.00 + ent->owner->client->zoom))
		ent->chasedist1 = (60.00 + ent->owner->client->zoom);
		/* if we haven't gone anywhere since the last think routine, and we
		* are greater than 20 points in the distance calculated, add one to
		* the second chasedistance variable
		* The "ent->movedir" is a vector which is not used in this entity, so
		* we can use this a tempory vector belonging to the chasecam, which
	* can be carried through think routines. */
	if (VectorCompare(ent->movedir, ent->s.origin))
	{
		if (distance > 20)
			ent->chasedist2++;
	}
	/* if we've buggered up more than 3 times, there must be some mistake,
	* so restart the camera so we re-create a chasecam, destroy the old one,
	* slowly go outwards from the player, and keep thinking this routing in
	* the new camera entity */
	if (ent->chasedist2 > 3)
	{
		G_FreeEdict (ent->owner->client->oldplayer);
		ChasecamStart (ent->owner);
		G_FreeEdict(ent);
		return;
	}
	/* Copy the position of the chasecam now, and stick it to the movedir
	* variable, for position checking when we rethink this function */
	VectorCopy (ent->s.origin, ent->movedir);
	/* MUST LINK SINCE WE CHANGED THE ORIGIN! */
	gi.linkentity (ent);
	//UpdateFakeCrosshair (ent->owner);
}

void Cmd_Chasecam_Toggle (edict_t *ent)
{
	// Lazarus: Don't allow thirdperson when using spycam
	if (!ent->deadflag && !ent->client->spycam)
	{
		if (ent->client->chasetoggle)
			ChasecamRemove (ent, OPTION_OFF);
		// Knightmare- don't use server chasecam if client chasecam is on
#ifdef KMQUAKE2_ENGINE_MOD
		else if (!cl_thirdperson->value || deathmatch->value || coop->value)
#else
		else
#endif
			ChasecamStart (ent);
	}
}

void CheckChasecam_Viewent (edict_t *ent)
{
	vec3_t angles;
	/*
	Oldplayer is the fake player that everyone else sees.
	Assign the same client as the ent we're following so the game
	can read any vars it wants from there
	*/
	if (!ent->client->oldplayer->client)
		ent->client->oldplayer->client = ent->client;
		/* Copy the angle and model from ourselves to the old player.
	Even though people can't see us we still have all this stuff */
	if ((ent->client->chasetoggle == 1) && (ent->client->oldplayer))
	{
// Lazarus
//		if (ent->client->use)
//			VectorCopy (ent->client->oldplayer->s.angles, angles);
//		ent->client->oldplayer->s = ent->s; //Copy player related info
//		if (ent->client->use)
//			VectorCopy (angles, ent->client->oldplayer->s.angles);

		if (ent->client->use && !ent->client->push)
			VectorCopy (ent->client->oldplayer->s.angles, angles);
		ent->client->oldplayer->s = ent->s; //Copy player related info
		// Lazarus: s.numbers shouldn't be the same
		ent->client->oldplayer->s.number = ent->client->oldplayer - g_edicts;
		if (ent->client->use && !ent->client->push)
			VectorCopy (angles, ent->client->oldplayer->s.angles);
		ent->client->oldplayer->flags = ent->flags;
// end Lazarus
		gi.linkentity (ent->client->oldplayer);
	}
}

