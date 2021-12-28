#include "g_local.h"


qboolean	Pickup_Weapon (edict_t *ent, edict_t *other);
void		Use_Weapon (edict_t *ent, gitem_t *inv);
void		Drop_Weapon (edict_t *ent, gitem_t *inv);

void Weapon_Blaster (edict_t *ent);
void Weapon_Shotgun (edict_t *ent);
void Weapon_SuperShotgun (edict_t *ent);
void Weapon_Machinegun (edict_t *ent);
void Weapon_Chaingun (edict_t *ent);
void Weapon_HyperBlaster (edict_t *ent);
void Weapon_RocketLauncher (edict_t *ent);
void Weapon_Grenade (edict_t *ent);
void Weapon_GrenadeLauncher (edict_t *ent);
void Weapon_Railgun (edict_t *ent);
void Weapon_BFG (edict_t *ent);
void Weapon_HomingMissileLauncher (edict_t *ent);
void Weapon_Null(edict_t *ent);

gitem_armor_t jacketarmor_info	= { 25,  50, .30, .00, ARMOR_JACKET};
gitem_armor_t combatarmor_info	= { 50, 100, .60, .30, ARMOR_COMBAT};
gitem_armor_t bodyarmor_info	= {100, 200, .80, .60, ARMOR_BODY};

int	noweapon_index;
int	jacket_armor_index;
int	combat_armor_index;
int	body_armor_index;
int	power_screen_index;
int	power_shield_index;
int	shells_index;
int	bullets_index;
int	grenades_index;
int	rockets_index;
int	cells_index;
int	slugs_index;
int fuel_index;
int	homing_index;
int rl_index;
int	hml_index;

#define HEALTH_IGNORE_MAX	1
#define HEALTH_TIMED		2

#define NO_STUPID_SPINNING  4
#define NO_DROPTOFLOOR      8
#define SHOOTABLE           16

void Use_Quad (edict_t *ent, gitem_t *item);
void Use_Stasis (edict_t *ent, gitem_t *item);
static int	quad_drop_timeout_hack;

//======================================================================

// Lazarus: damageable pickups
void item_die(edict_t *self,edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	if (level.num_reflectors)
		ReflectExplosion(TE_EXPLOSION1, self->s.origin);

//CW+++ Fire killtarget, if set.
	if (self->killtarget)
		G_UseTargets(self, attacker);
//CW---

	if (!(self->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn(self, 30);
	else
		G_FreeEdict(self);
}

/*
===============
GetItemByIndex
===============
*/
gitem_t	*GetItemByIndex (int index)
{
	if (index == 0 || index >= game.num_items)
		return NULL;

	return &itemlist[index];
}

/*
===============
GetMaxAmmoByIndex
===============
*/
int GetMaxAmmoByIndex (gclient_t *client, int item_index)
{
	int value;

	if (!client)
		return 0;

	if (item_index == shells_index)
		value = client->pers.max_shells;
	else if (item_index == bullets_index)
		value = client->pers.max_bullets;
	else if (item_index == grenades_index)
		value = client->pers.max_grenades;
	else if (item_index == rockets_index)
		value = client->pers.max_rockets;
	else if (item_index == cells_index)
		value = client->pers.max_cells;
	else if (item_index == slugs_index)
		value = client->pers.max_slugs;
	else if (item_index == fuel_index)
		value = client->pers.max_fuel;
	else if (item_index == homing_index)
		value = client->pers.max_homing_rockets;
	else
		value = 0;

	return value;
}

/*
===============
GetMaxArmorByIndex
===============
*/
int GetMaxArmorByIndex (int item_index)
{
	int value;

	if (item_index == jacket_armor_index)
		value = sk_max_armor_jacket->value;
	else if (item_index == combat_armor_index)
		value = sk_max_armor_combat->value;
	else if (item_index == body_armor_index)
		value = sk_max_armor_body->value;
	else
		value = 0;

	return value;
}

/*
===============
FindItemByClassname

===============
*/
gitem_t	*FindItemByClassname (char *classname)
{
	int		i;
	gitem_t	*it;

//CW++
	if (NULL == classname)
	{
		gi.dprintf("NULL pointer passed to FindItemByClassname()\n");
		return NULL;
	}
//CW--

	it = itemlist;
	for (i=0 ; i<game.num_items ; i++, it++)
	{
		if (!it->classname)
			continue;

		if (!Q_stricmp(it->classname, classname))
			return it;
	}

	return NULL;
}

/*
===============
FindItem

===============
*/
gitem_t	*FindItem (char *pickup_name)
{
	int		i;
	gitem_t	*it;

	it = itemlist;
	for (i=0 ; i<game.num_items ; i++, it++)
	{
		if (!it->pickup_name)
			continue;
		if (!Q_stricmp(it->pickup_name, pickup_name))
			return it;
	}

	return NULL;
}

//======================================================================

void DoRespawn (edict_t *ent)
{
	if (ent->team)
	{
		edict_t	*master;
		int	count;
		int choice;

		master = ent->teammaster;

		for (count = 0, ent = master; ent; ent = ent->chain, count++)
			;

		choice = rand() % count;

		for (count = 0, ent = master; count < choice; ent = ent->chain, count++)
			;
	}

	ent->svflags &= ~SVF_NOCLIENT;
	if(ent->spawnflags & SHOOTABLE) {
		ent->solid = SOLID_BBOX;
		ent->clipmask |= MASK_MONSTERSOLID;
		if(!ent->health)
			ent->health = 20;
		ent->takedamage = DAMAGE_YES;
		ent->die = item_die;
	} else
		ent->solid = SOLID_TRIGGER;
	gi.linkentity (ent);

	// send an effect
	ent->s.event = EV_ITEM_RESPAWN;
}

void SetRespawn (edict_t *ent, float delay)
{
	ent->flags |= FL_RESPAWN;
	ent->svflags |= SVF_NOCLIENT;
	ent->solid = SOLID_NOT;
	ent->nextthink = level.time + delay;
	ent->think = DoRespawn;
	gi.linkentity (ent);
}


//======================================================================

qboolean Pickup_Powerup (edict_t *ent, edict_t *other)
{
	int		quantity;

	quantity = other->client->pers.inventory[ITEM_INDEX(ent->item)];
//	if ((skill->value == 1 && quantity >= 2) || (skill->value >= 2 && quantity >= 1))
	if ((skill->value == 1 && quantity >= sk_powerup_max->value) || (skill->value >= 2 && quantity >= (sk_powerup_max->value - 1)))
		return false;

	if ((coop->value) && (ent->item->flags & IT_STAY_COOP) && (quantity > 0))
		return false;

	// Lazarus: Don't allow more than one of some items
#ifdef FLASHLIGHT_MOD
	if( !Q_stricmp(ent->classname,"item_flashlight")   && quantity >= 1 ) return false;
#endif
#ifdef JETPACK_MOD
	if( !Q_stricmp(ent->classname,"item_jetpack") )
	{
		gitem_t *fuel;

		if( quantity >= 1 )
			return false;

		fuel = FindItem("fuel");
		if(ent->count < 0)
		{
			other->client->jetpack_infinite = true;
			Add_Ammo(other,fuel,10000);
		}
		else
		{
			other->client->jetpack_infinite = false;
			Add_Ammo(other,fuel,ent->count);
		}
	}
#endif

	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;

#ifdef FLASHLIGHT_MOD
		// DON'T Instant-use flashlight
		if (ent->item->use == Use_Flashlight) return true;
#endif

	if (deathmatch->value)
	{
		if (!(ent->spawnflags & DROPPED_ITEM) )
			SetRespawn (ent, ent->item->quantity);

#ifdef JETPACK_MOD
		// DON'T Instant-use Jetpack
		if(ent->item->use == Use_Jet) return true;
#endif

		if (((int)dmflags->value & DF_INSTANT_ITEMS) || ((ent->item->use == Use_Quad) && (ent->spawnflags & DROPPED_PLAYER_ITEM)))
		{
			if ((ent->item->use == Use_Quad) && (ent->spawnflags & DROPPED_PLAYER_ITEM))
				quad_drop_timeout_hack = (ent->nextthink - level.time) / FRAMETIME;
			ent->item->use (other, ent->item);
		}
	}

	return true;
}

void Drop_General (edict_t *ent, gitem_t *item)
{
	Drop_Item (ent, item);
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);
}

#ifdef JETPACK_MOD
void Drop_Jetpack (edict_t *ent, gitem_t *item)
{
	if(ent->client->jetpack)
		gi.cprintf(ent,PRINT_HIGH,"Cannot drop jetpack in use\n");
	else
	{
		edict_t	*dropped;

		dropped = Drop_Item (ent, item);
		if(ent->client->jetpack_infinite)
		{
			dropped->count = -1;
			ent->client->pers.inventory[fuel_index] = 0;
			ent->client->jetpack_infinite = false;
		}
		else
		{
			dropped->count = ent->client->pers.inventory[fuel_index];
			if(dropped->count > 500)
				dropped->count = 500;
			ent->client->pers.inventory[fuel_index] -= dropped->count;
		}
		ent->client->pers.inventory[ITEM_INDEX(item)]--;
		ValidateSelectedItem (ent);
	}
}
#endif

//======================================================================

qboolean Pickup_Adrenaline (edict_t *ent, edict_t *other)
{
	if (!deathmatch->value)
		other->max_health += 1;

	if (other->health < other->max_health)
		other->health = other->max_health;

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn (ent, ent->item->quantity);

	return true;
}

qboolean Pickup_AncientHead (edict_t *ent, edict_t *other)
{
	other->max_health += 2;

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn (ent, ent->item->quantity);

	return true;
}

qboolean Pickup_Bandolier (edict_t *ent, edict_t *other)
{
	gitem_t	*item;
	int		index;

	//Knightmare- override ammo pickup values with cvars
	SetAmmoPickupValues ();

	if (other->client->pers.max_bullets < sk_bando_bullets->value)
		other->client->pers.max_bullets = sk_bando_bullets->value;
	if (other->client->pers.max_shells < sk_bando_shells->value)
		other->client->pers.max_shells = sk_bando_shells->value;
	if (other->client->pers.max_cells < sk_bando_cells->value)
		other->client->pers.max_cells = sk_bando_cells->value;
	if (other->client->pers.max_slugs < sk_bando_slugs->value)
		other->client->pers.max_slugs = sk_bando_slugs->value;
	if (other->client->pers.max_fuel  < sk_bando_fuel->value)
		other->client->pers.max_fuel  = sk_bando_fuel->value;

	item = FindItem("Bullets");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_bullets)
			other->client->pers.inventory[index] = other->client->pers.max_bullets;
	}

	item = FindItem("Shells");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_shells)
			other->client->pers.inventory[index] = other->client->pers.max_shells;
	}

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn (ent, ent->item->quantity);

	return true;
}

qboolean Pickup_Pack (edict_t *ent, edict_t *other)
{
	gitem_t	*item;
	int		index;

	//Knightmare- override ammo pickup values with cvars
	SetAmmoPickupValues ();

	if (other->client->pers.max_bullets < sk_pack_bullets->value)
		other->client->pers.max_bullets = sk_pack_bullets->value;
	if (other->client->pers.max_shells < sk_pack_shells->value)
		other->client->pers.max_shells = sk_pack_shells->value;
	if (other->client->pers.max_rockets < sk_pack_rockets->value)
		other->client->pers.max_rockets = sk_pack_rockets->value;
	if (other->client->pers.max_grenades < sk_pack_grenades->value)
		other->client->pers.max_grenades = sk_pack_grenades->value;
	if (other->client->pers.max_cells < sk_pack_cells->value)
		other->client->pers.max_cells = sk_pack_cells->value;
	if (other->client->pers.max_slugs < sk_pack_slugs->value)
		other->client->pers.max_slugs = sk_pack_slugs->value;
	if (other->client->pers.max_homing_rockets < sk_pack_rockets->value)
		other->client->pers.max_homing_rockets = sk_pack_rockets->value;
	if (other->client->pers.max_fuel  < sk_pack_fuel->value)
		other->client->pers.max_fuel  = sk_pack_fuel->value;

	item = FindItem("Bullets");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_bullets)
			other->client->pers.inventory[index] = other->client->pers.max_bullets;
	}

	item = FindItem("Shells");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_shells)
			other->client->pers.inventory[index] = other->client->pers.max_shells;
	}

	item = FindItem("Cells");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_cells)
			other->client->pers.inventory[index] = other->client->pers.max_cells;
	}

	item = FindItem("Grenades");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_grenades)
			other->client->pers.inventory[index] = other->client->pers.max_grenades;
	}

	item = FindItem("Rockets");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_rockets)
			other->client->pers.inventory[index] = other->client->pers.max_rockets;
	}

	item = FindItem("Slugs");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_slugs)
			other->client->pers.inventory[index] = other->client->pers.max_slugs;
	}

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn (ent, ent->item->quantity);

	return true;
}

//======================================================================

void Use_Quad (edict_t *ent, gitem_t *item)
{
	int		timeout;

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (quad_drop_timeout_hack)
	{
		timeout = quad_drop_timeout_hack;
		quad_drop_timeout_hack = 0;
	}
	else
	{
		timeout = (sk_quad_time->value * 10);
	}

	if (ent->client->quad_framenum > level.framenum)
		ent->client->quad_framenum += timeout;
	else
		ent->client->quad_framenum = level.framenum + timeout;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void Use_Breather (edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (ent->client->breather_framenum > level.framenum)
		ent->client->breather_framenum += (sk_breather_time->value * 10);
	else
		ent->client->breather_framenum = level.framenum + (sk_breather_time->value * 10);

//	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void Use_Envirosuit (edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (ent->client->enviro_framenum > level.framenum)
		ent->client->enviro_framenum += (sk_enviro_time->value * 10);
	else
		ent->client->enviro_framenum = level.framenum + (sk_enviro_time->value * 10);

//	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void	Use_Invulnerability (edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (ent->client->invincible_framenum > level.framenum)
		ent->client->invincible_framenum += (sk_inv_time->value * 10);
	else
		ent->client->invincible_framenum = level.framenum + (sk_inv_time->value * 10);

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void	Use_Silencer (edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);
	ent->client->silencer_shots += sk_silencer_shots->value;

//	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

qboolean Pickup_Key (edict_t *ent, edict_t *other)
{
	if (coop->value)
	{
		if (strcmp(ent->classname, "key_power_cube") == 0)
		{
			if (other->client->pers.power_cubes & ((ent->spawnflags & 0x0000ff00)>> 8))
				return false;
			other->client->pers.inventory[ITEM_INDEX(ent->item)]++;
			other->client->pers.power_cubes |= ((ent->spawnflags & 0x0000ff00) >> 8);
		}
		else
		{
			if (other->client->pers.inventory[ITEM_INDEX(ent->item)])
				return false;
			other->client->pers.inventory[ITEM_INDEX(ent->item)] = 1;
		}
		return true;
	}
	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;
	return true;
}

//======================================================================

qboolean Add_Ammo (edict_t *ent, gitem_t *item, int count)
{
	int			index;
	int			max;

	if (!ent->client)
		return false;

	if (item->tag == AMMO_BULLETS)
		max = ent->client->pers.max_bullets;
	else if (item->tag == AMMO_SHELLS)
		max = ent->client->pers.max_shells;
	else if (item->tag == AMMO_ROCKETS)
		max = ent->client->pers.max_rockets;
	else if (item->tag == AMMO_GRENADES)
		max = ent->client->pers.max_grenades;
	else if (item->tag == AMMO_CELLS)
		max = ent->client->pers.max_cells;
	else if (item->tag == AMMO_SLUGS)
		max = ent->client->pers.max_slugs;
	else if (item->tag == AMMO_FUEL)
		max = ent->client->pers.max_fuel;
	else if (item->tag == AMMO_HOMING_ROCKETS)
		max = ent->client->pers.max_homing_rockets;
	else
		return false;

	index = ITEM_INDEX(item);

	if (ent->client->pers.inventory[index] == max)
		return false;

	ent->client->pers.inventory[index] += count;

	if (ent->client->pers.inventory[index] > max)
		ent->client->pers.inventory[index] = max;

	return true;
}

//Knightmare- this function overrides ammo pickup values with cvars
void SetAmmoPickupValues (void)
{
	gitem_t		*item;

	item = FindItem("Shells");
	if (item)
		item->quantity = sk_box_shells->value;

	item = FindItem("Bullets");
	if (item)
		item->quantity = sk_box_bullets->value;

	item = FindItem("Grenades");
	if (item)
		item->quantity = sk_box_grenades->value;

	item = FindItem("Rockets");
	if (item)
		item->quantity = sk_box_rockets->value;

	item = FindItem("Homing Rockets");
	if (item)
		item->quantity = sk_box_rockets->value;

	item = FindItem("Cells");
	if (item)
		item->quantity = sk_box_cells->value;

	item = FindItem("Slugs");
	if (item)
		item->quantity = sk_box_slugs->value;

	item = FindItem("Fuel");
	if (item)
		item->quantity = sk_box_fuel->value;

	item = FindItemByClassname("item_health_small");
	if (item)
		item->quantity = sk_health_bonus_value->value;
}

qboolean Pickup_Ammo (edict_t *ent, edict_t *other)
{
	int			oldcount;
	int			count;
	qboolean	weapon;

	//Knightmare- override ammo pickup values with cvars
	SetAmmoPickupValues ();

	weapon = (ent->item->flags & IT_WEAPON);
	if ( (weapon) && ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		count = 1000;
	else if (ent->count)
		count = ent->count;
	else
		count = ent->item->quantity;

	oldcount = other->client->pers.inventory[ITEM_INDEX(ent->item)];

	if (!Add_Ammo (other, ent->item, count))
		return false;

	if (weapon && !oldcount)
	{
		if (other->client->pers.weapon != ent->item && ( !deathmatch->value || other->client->pers.weapon == FindItem("blaster") || other->client->pers.weapon == FindItem("No weapon") ) )
			other->client->newweapon = ent->item;
	}

	if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)) && (deathmatch->value))
		SetRespawn (ent, 30);
	return true;
}

void Drop_Ammo (edict_t *ent, gitem_t *item)
{
	edict_t	*dropped;
	int		index;

	index = ITEM_INDEX(item);
	dropped = Drop_Item (ent, item);
	if (ent->client->pers.inventory[index] >= item->quantity)
		dropped->count = item->quantity;
	else
		dropped->count = ent->client->pers.inventory[index];

	if (ent->client->pers.weapon && 
		ent->client->pers.weapon->tag == AMMO_GRENADES &&
		item->tag == AMMO_GRENADES &&
		ent->client->pers.inventory[index] - dropped->count <= 0) {
		gi.cprintf (ent, PRINT_HIGH, "Can't drop current weapon\n");
		G_FreeEdict(dropped);
		return;
	}

	ent->client->pers.inventory[index] -= dropped->count;
	ValidateSelectedItem (ent);
}


//======================================================================

void MegaHealth_think (edict_t *self)
{
	if (self->owner->health > self->owner->max_health)
	{
		self->nextthink = level.time + 1;
		self->owner->health -= 1;
		return;
	}

	if (!(self->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn (self, 20);
	else
		G_FreeEdict (self);
}

qboolean Pickup_Health (edict_t *ent, edict_t *other)
{
	if (!(ent->style & HEALTH_IGNORE_MAX))
		if (other->health >= other->max_health)
			return false;

	other->health += ent->count;

	if (!(ent->style & HEALTH_IGNORE_MAX))
	{
		if (other->health > other->max_health)
			other->health = other->max_health;
	}

	if (ent->style & HEALTH_TIMED)
	{
		ent->think = MegaHealth_think;
		ent->nextthink = level.time + 5;
		ent->owner = other;
		ent->flags |= FL_RESPAWN;
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
	}
	else
	{
		if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
			SetRespawn (ent, 30);
	}

	return true;
}

//======================================================================

int ArmorIndex (edict_t *ent)
{
	if (!ent->client)
		return 0;

	if (ent->client->pers.inventory[jacket_armor_index] > 0)
		return jacket_armor_index;

	if (ent->client->pers.inventory[combat_armor_index] > 0)
		return combat_armor_index;

	if (ent->client->pers.inventory[body_armor_index] > 0)
		return body_armor_index;

	return 0;
}

qboolean Pickup_Armor (edict_t *ent, edict_t *other)
{
	int				old_armor_index;
	gitem_armor_t	*oldinfo;
	gitem_armor_t	*newinfo;
	int				newcount;
	float			salvage;
	int				salvagecount;
	int				armor_maximum;

	// set armor cap
	if (ent->item->tag == ARMOR_JACKET)
		armor_maximum = sk_max_armor_jacket->value;
	else if (ent->item->tag == ARMOR_COMBAT)
		armor_maximum = sk_max_armor_combat->value;
	else	// ARMOR_BODY
		armor_maximum = sk_max_armor_body->value;

	// get info on new armor
	newinfo = (gitem_armor_t *)ent->item->info;

	old_armor_index = ArmorIndex (other);

	// handle armor shards specially
	if (ent->item->tag == ARMOR_SHARD)
	{
		if (!old_armor_index)
			other->client->pers.inventory[jacket_armor_index] = sk_armor_bonus_value->value;
		else
			other->client->pers.inventory[old_armor_index] += sk_armor_bonus_value->value;
	}

	// if player has no armor, just use it
	else if (!old_armor_index)
	{
		other->client->pers.inventory[ITEM_INDEX(ent->item)] = newinfo->base_count;
	}

	// use the better armor
	else
	{
		// get info on old armor
		if (old_armor_index == jacket_armor_index)
			oldinfo = &jacketarmor_info;
		else if (old_armor_index == combat_armor_index)
			oldinfo = &combatarmor_info;
		else // (old_armor_index == body_armor_index)
			oldinfo = &bodyarmor_info;

		if (newinfo->normal_protection > oldinfo->normal_protection)
		{
			// calc new armor values
			salvage = oldinfo->normal_protection / newinfo->normal_protection;
			salvagecount = salvage * other->client->pers.inventory[old_armor_index];
			newcount = newinfo->base_count + salvagecount;
			if (newcount > armor_maximum)
				newcount = armor_maximum;

			// zero count of old armor so it goes away
			other->client->pers.inventory[old_armor_index] = 0;

			// change armor to new item with computed value
			other->client->pers.inventory[ITEM_INDEX(ent->item)] = newcount;
		}
		else
		{
			// calc new armor values
			salvage = newinfo->normal_protection / oldinfo->normal_protection;
			salvagecount = salvage * newinfo->base_count;
			newcount = other->client->pers.inventory[old_armor_index] + salvagecount;
			if (newcount > armor_maximum)
				newcount = armor_maximum;

			// if we're already maxed out then we don't need the new armor
			if (other->client->pers.inventory[old_armor_index] >= newcount)
				return false;

			// update current armor value
			other->client->pers.inventory[old_armor_index] = newcount;
		}
	}

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn (ent, 20);

	return true;
}

//======================================================================

int PowerArmorType (edict_t *ent)
{
	if (!ent->client)
		return POWER_ARMOR_NONE;

//	if (!(ent->flags & FL_POWER_ARMOR))
//		return POWER_ARMOR_NONE;

//	if (ent->client->pers.inventory[power_shield_index] > 0)
	if (ent->flags & FL_POWER_SHIELD)
		return POWER_ARMOR_SHIELD;

//	if (ent->client->pers.inventory[power_screen_index] > 0)
	if (ent->flags & FL_POWER_SCREEN)
		return POWER_ARMOR_SCREEN;

	return POWER_ARMOR_NONE;
}

void Use_PowerArmor (edict_t *ent, gitem_t *item)
{
	int		index;

		if (item == FindItemByClassname("item_power_screen"))
	{	//if player has an active power shield, deacivate that and activate power screen
		if (ent->flags & FL_POWER_SHIELD)
		{
			index = ITEM_INDEX(FindItem("cells"));
			if (!ent->client->pers.inventory[index])
			{
				gi.cprintf (ent, PRINT_HIGH, "No cells for power screen.\n");
				return;
			}
			ent->flags &= ~FL_POWER_SHIELD;
			ent->flags |= FL_POWER_SCREEN;
			gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
			gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power1.wav"), 1, ATTN_NORM, 0);
		}
		//if they have an active power screen, deactivate that
		else if (ent->flags & FL_POWER_SCREEN)
		{
			ent->flags &= ~FL_POWER_SCREEN;
			gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
		}
		else //activate power screen
		{
			index = ITEM_INDEX(FindItem("cells"));
			if (!ent->client->pers.inventory[index])
			{
				gi.cprintf (ent, PRINT_HIGH, "No cells for power screen.\n");
				return;
			}
			ent->flags |= FL_POWER_SCREEN;
			gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power1.wav"), 1, ATTN_NORM, 0);

		}
	}
	else if (item == FindItemByClassname("item_power_shield"))
	{	//if player has an active power screen, deacivate that and activate power shield
		if (ent->flags & FL_POWER_SCREEN)
		{
			index = ITEM_INDEX(FindItem("cells"));
			if (!ent->client->pers.inventory[index])
			{
				gi.cprintf (ent, PRINT_HIGH, "No cells for power shield.\n");
				return;
			}
			ent->flags &= ~FL_POWER_SCREEN;
			ent->flags |= FL_POWER_SHIELD;
			gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
			gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power1.wav"), 1, ATTN_NORM, 0);
		}
		//if they have an active power shield, deactivate it
		else if (ent->flags & FL_POWER_SHIELD)
		{
			ent->flags &= ~FL_POWER_SHIELD;
			gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
		}
		else //activate power shield
		{
			index = ITEM_INDEX(FindItem("cells"));
			if (!ent->client->pers.inventory[index])
			{
				gi.cprintf (ent, PRINT_HIGH, "No cells for power shield.\n");
				return;
			}
			ent->flags |= FL_POWER_SHIELD;
			gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power1.wav"), 1, ATTN_NORM, 0);
		}
	}
/*	if (ent->flags & FL_POWER_ARMOR)
	{
		ent->flags &= ~FL_POWER_ARMOR;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
	}
	else
	{
		index = cells_index;
		if (!ent->client->pers.inventory[index])
		{
			gi.cprintf (ent, PRINT_HIGH, "No cells for power armor.\n");
			return;
		}
		ent->flags |= FL_POWER_ARMOR;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power1.wav"), 1, ATTN_NORM, 0);
	}*/
}

qboolean Pickup_PowerArmor (edict_t *ent, edict_t *other)
{
	int		quantity;

	quantity = other->client->pers.inventory[ITEM_INDEX(ent->item)];

	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;

	if (deathmatch->value)
	{
		if (!(ent->spawnflags & DROPPED_ITEM) )
			SetRespawn (ent, ent->item->quantity);
		// auto-use for DM only if we didn't already have one
		if (!quantity)
			ent->item->use (other, ent->item);
	}

	return true;
}

void Drop_PowerArmor (edict_t *ent, gitem_t *item)
{
//	if ((ent->flags & FL_POWER_ARMOR) && (ent->client->pers.inventory[ITEM_INDEX(item)] == 1))
//		Use_PowerArmor (ent, item);
	if (item == FindItemByClassname("item_power_shield"))
	{
		if ((ent->flags & FL_POWER_SHIELD) && (ent->client->pers.inventory[ITEM_INDEX(item)] == 1))
			Use_PowerArmor (ent, item);
	}
	else
		if ((ent->flags & FL_POWER_SCREEN) && (ent->client->pers.inventory[ITEM_INDEX(item)] == 1))
			Use_PowerArmor (ent, item);
	Drop_General (ent, item);
}

//======================================================================

/*
===============
Touch_Item
===============
*/
void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	qboolean	taken;

	if (!other->client)
		return;
	if (other->health < 1)
		return;		// dead people can't pickup
	if (!ent->item->pickup)
		return;		// not a grabbable item?

	taken = ent->item->pickup(ent, other);

	if (taken)
	{
		// flash the screen
		other->client->bonus_alpha = 0.25;	

		// show icon and name on status bar
		other->client->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(ent->item->icon);
		other->client->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS+ITEM_INDEX(ent->item);
		other->client->pickup_msg_time = level.time + 3.0;

		// change selected item
		if (ent->item->use)
			other->client->pers.selected_item = other->client->ps.stats[STAT_SELECTED_ITEM] = ITEM_INDEX(ent->item);

		if (ent->item->pickup == Pickup_Health)
		{
			if (ent->count == sk_health_bonus_value->value)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/s_health.wav"), 1, ATTN_NORM, 0);
			else if (ent->count == 10)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/n_health.wav"), 1, ATTN_NORM, 0);
			else if (ent->count == 25)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/l_health.wav"), 1, ATTN_NORM, 0);
			else // (ent->count == 100)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/m_health.wav"), 1, ATTN_NORM, 0);
		}
		else if (ent->item->pickup_sound)
		{
			gi.sound(other, CHAN_ITEM, gi.soundindex(ent->item->pickup_sound), 1, ATTN_NORM, 0);
		}
	}

	if (!taken)		//CW: this check was originally after the if-statement below; moved it here so that
		return;		//    target only fires if the item is actually picked up (not just touched).

	if (!(ent->spawnflags & ITEM_TARGETS_USED))
	{
		G_UseTargets (ent, other);
		ent->spawnflags |= ITEM_TARGETS_USED;
	}

	DeleteReflection(ent,-1);

	if (!((coop->value) &&  (ent->item->flags & IT_STAY_COOP)) || (ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
	{
		if (ent->flags & FL_RESPAWN)
			ent->flags &= ~FL_RESPAWN;
		else
			G_FreeEdict (ent);
	}
}

//======================================================================

/*static*/ void drop_temp_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;

	Touch_Item (ent, other, plane, surf);
}

/*static*/ void drop_make_touchable (edict_t *ent)
{
	ent->touch = Touch_Item;
	if (deathmatch->value)
	{
		ent->nextthink = level.time + 29;
		ent->think = G_FreeEdict;
	}
}

edict_t *Drop_Item(edict_t *ent, gitem_t *item)
{
	edict_t	*dropped;
	vec3_t	forward, right;
	vec3_t	offset;

	dropped = G_Spawn();
	
	dropped->classname = item->classname;
	dropped->clipmask |= CONTENTS_SOLID | CONTENTS_GRILL;	//CW: so that item doesn't fall through grills
	dropped->item = item;
	dropped->spawnflags = DROPPED_ITEM;
	dropped->s.skinnum = item->world_model_skinnum; // Knightmare- skinnum specified in item table
	dropped->s.effects = item->world_model_flags;
	dropped->s.renderfx = RF_GLOW | RF_IR_VISIBLE;
	dropped->s.angles[1] = ent->s.angles[1];	// Knightmare- preserve yaw from dropping entity
	if (rand() > 0.5)							// ranomize it a bit
		dropped->s.angles[1] += rand()*45;
	else
		dropped->s.angles[1] -= rand()*45;

	VectorSet (dropped->mins, -16, -16, -16);
	VectorSet (dropped->maxs, 16, 16, 16);

//CW+++ Don't floaty-spin certain items when they're dropped...lay them gently, statically on the ground.
	if (!Q_stricmp(dropped->classname, "key_commander_head") || !Q_stricmp(dropped->classname, "key_captain_head"))
	{
		dropped->s.angles[1] = 150;
		dropped->s.effects &= ~EF_ROTATE;
		VectorSet(dropped->mins, -16, -16, -16);
		VectorSet(dropped->maxs, 16, 16, 16);
	}
	else if (!Q_stricmp(dropped->classname, "weapon_railgun"))
	{
		dropped->s.effects &= ~EF_ROTATE;
		VectorSet(dropped->mins, -16, -16, 8);
		VectorSet(dropped->maxs, 16, 16, 40);
	}
	else if (!Q_stricmp(dropped->classname, "weapon_rocketlauncher"))
	{
		dropped->s.effects &= ~EF_ROTATE;
		VectorSet(dropped->mins, -16, -16, 8);
		VectorSet(dropped->maxs, 16, 16, 40);
	}
	else if (dropped->item->pickup == Pickup_Armor)
	{
		dropped->s.angles[0] = -90;
		dropped->s.effects &= ~EF_ROTATE;
		VectorSet(dropped->mins, -16, -16, -8);
		VectorSet(dropped->maxs, 16, 16, 24);
	}

//CW---
	
	gi.setmodel (dropped, dropped->item->world_model);
	dropped->solid = SOLID_TRIGGER;
	dropped->movetype = MOVETYPE_TOSS;  
	dropped->touch = drop_temp_touch;
	dropped->owner = ent;

	// Lazarus: for monster-dropped health
	if (item->pickup == Pickup_Health)
	{
		dropped->count = item->quantity;
		if (item->quantity == sk_health_bonus_value->value)
			dropped->style |= HEALTH_IGNORE_MAX;
		if (item->quantity == 100)
//			dropped->style |= HEALTH_IGNORE_MAX | HEALTH_TIMED;		//CW: commented out so that...
			dropped->style |= HEALTH_IGNORE_MAX;					//CW: ...MH doesn't count down
	}

	if (ent->client)
	{
		trace_t	trace;

		AngleVectors (ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 0, -16);
		G_ProjectSource (ent->s.origin, offset, forward, right, dropped->s.origin);
		trace = gi.trace (ent->s.origin, dropped->mins, dropped->maxs, dropped->s.origin, ent, CONTENTS_SOLID | CONTENTS_GRILL);	//CW: so that item doesn't fall through grills
		VectorCopy (trace.endpos, dropped->s.origin);
	}
	else
	{
// Lazarus: throw the dropped item a bit farther than the default
		trace_t	trace;

		AngleVectors (ent->s.angles, forward, right, NULL);
//		VectorCopy (ent->s.origin, dropped->s.origin);
		VectorSet(offset, 24, 0, -16);

//CW++
		if (!Q_stricmp(dropped->classname, "key_commander_head") || !Q_stricmp(dropped->classname, "key_captain_head"))
		{
			VectorNegate(forward, forward);
			VectorNegate(right, right);
			VectorSet(offset, 16, 0, 16);
		}
//CW--

		G_ProjectSource (ent->s.origin, offset, forward, right, dropped->s.origin);
		trace = gi.trace (ent->s.origin, dropped->mins, dropped->maxs, dropped->s.origin, ent, CONTENTS_SOLID | CONTENTS_GRILL);	//CW: so that item doesn't fall through grills
		VectorCopy (trace.endpos, dropped->s.origin);
	}

	VectorScale (forward, 100, dropped->velocity);
	dropped->velocity[2] = 300;

	dropped->think = drop_make_touchable;
	dropped->nextthink = level.time + 1;

	gi.linkentity (dropped);

	return dropped;
}

void Use_Item (edict_t *ent, edict_t *other, edict_t *activator)
{
	ent->svflags &= ~SVF_NOCLIENT;
	ent->use = NULL;

	if (ent->spawnflags & ITEM_NO_TOUCH)
	{
		ent->solid = SOLID_BBOX;
		ent->touch = NULL;
	}
	else
	{
		// Lazarus:
		if(ent->spawnflags & SHOOTABLE) {
			ent->solid = SOLID_BBOX;
			ent->clipmask |= MASK_MONSTERSOLID;
			if(!ent->health)
				ent->health = 20;
			ent->takedamage = DAMAGE_YES;
			ent->die = item_die;
		} else
			ent->solid = SOLID_TRIGGER;
		ent->touch = Touch_Item;
	}

	gi.linkentity (ent);
}

//======================================================================

/*
================
droptofloor
================
*/
void droptofloor (edict_t *ent)
{
	trace_t		tr;
	vec3_t		dest;
	float		*v;

	v = tv(-15,-15,-15);
	VectorCopy (v, ent->mins);
	v = tv(15,15,15);
	VectorCopy (v, ent->maxs);

	if (ent->model)
		gi.setmodel (ent, ent->model);
	else
		gi.setmodel (ent, ent->item->world_model);

	// Lazarus:
	// origin_offset is wrong - absmin and absmax weren't set soon enough.
	// Fortunately we KNOW what the "offset" is - nada.
	VectorClear(ent->origin_offset);

	if (ent->spawnflags & SHOOTABLE)
	{
		ent->solid = SOLID_BBOX;
		ent->clipmask |= MASK_MONSTERSOLID;
		ent->clipmask |= CONTENTS_SOLID | CONTENTS_GRILL;	//CW: so that item doesn't fall through grills
		if(!ent->health)
			ent->health = 20;
		ent->takedamage = DAMAGE_YES;
		ent->die = item_die;
	}
	else
	{
		ent->solid = SOLID_TRIGGER;
		ent->clipmask |= CONTENTS_SOLID | CONTENTS_GRILL;	//CW: so that item doesn't fall through grills
	}

	// Lazarus:
	if(ent->movewith)
		ent->movetype = MOVETYPE_PUSH;
	else if(ent->spawnflags & NO_DROPTOFLOOR)
		ent->movetype = MOVETYPE_NONE;
	else
		ent->movetype = MOVETYPE_TOSS;  
	ent->touch = Touch_Item;

	// Lazarus:
	if(!(ent->spawnflags & NO_DROPTOFLOOR)) {
		v = tv(0,0,-128);
		VectorAdd (ent->s.origin, v, dest);

		tr = gi.trace (ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID | CONTENTS_GRILL);	//CW: so that item doesn't fall through grills
		if (tr.startsolid)
		{
			gi.dprintf ("droptofloor: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
			G_FreeEdict (ent);
			return;
		}
		tr.endpos[2] += 1;
		ent->mins[2] -= 1;
		VectorCopy (tr.endpos, ent->s.origin);
	}

	if (ent->team)
	{
		ent->flags &= ~FL_TEAMSLAVE;
		ent->chain = ent->teamchain;
		ent->teamchain = NULL;

		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		if (ent == ent->teammaster)
		{
			ent->nextthink = level.time + FRAMETIME;
			ent->think = DoRespawn;
		}
	}

	if (ent->spawnflags & ITEM_NO_TOUCH)
	{
		ent->solid = SOLID_BBOX;
		ent->touch = NULL;
		ent->s.effects &= ~EF_ROTATE;
		ent->s.renderfx &= ~RF_GLOW;
	}

	if (ent->spawnflags & ITEM_TRIGGER_SPAWN)
	{
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		ent->use = Use_Item;
	}

	gi.linkentity (ent);
}


/*
===============
PrecacheItem

Precaches all data needed for a given item.
This will be called for each item spawned in a level,
and for each item in each client's inventory.
===============
*/
void PrecacheItem (gitem_t *it)
{
	char	*s, *start;
	char	data[MAX_QPATH];
	int		len;
	gitem_t	*ammo;

	if (!it)
		return;

	if (it->pickup_sound)
		gi.soundindex (it->pickup_sound);
	if (it->world_model)
		gi.modelindex (it->world_model);
	if (it->view_model)
		gi.modelindex (it->view_model);
	if (it->icon)
		gi.imageindex (it->icon);

	// parse everything for its ammo
	if (it->ammo && it->ammo[0])
	{
		ammo = FindItem (it->ammo);
		if (ammo != it)
			PrecacheItem (ammo);
	}

	// parse the space seperated precache string for other items
	s = it->precaches;
	if (!s || !s[0])
		return;

	while (*s)
	{
		start = s;
		while (*s && *s != ' ')
			s++;

		len = s-start;
		if (len >= MAX_QPATH || len < 5)
			gi.error ("PrecacheItem: %s has bad precache string", it->classname);
		memcpy (data, start, len);
		data[len] = 0;
		if (*s)
			s++;

		// determine type based on extension
		if (!strcmp(data+len-3, "md2"))
			gi.modelindex (data);
		else if (!strcmp(data+len-3, "sp2"))
			gi.modelindex (data);
		else if (!strcmp(data+len-3, "wav"))
			gi.soundindex (data);
		if (!strcmp(data+len-3, "pcx"))
			gi.imageindex (data);
	}
}

/*
============
SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void SpawnItem (edict_t *ent, gitem_t *item)
{
	PrecacheItem (item);

	// Lazarus: added several spawnflags, plus gave ALL keys trigger_spawn and no_touch
	// capabilities
	if ( ( (item->flags & IT_KEY) && (ent->spawnflags & ~31) ) ||
		 (!(item->flags & IT_KEY) && (ent->spawnflags & ~28) )    ) 
//	if (ent->spawnflags)
	{
//		if (strcmp(ent->classname, "key_power_cube") != 0)
		{
			gi.dprintf("%s at %s has invalid spawnflags set (%d)\n", ent->classname, vtos(ent->s.origin), ent->spawnflags);
			if (item->flags & IT_KEY)
				ent->spawnflags &= 31;
			else
				ent->spawnflags &= 28;
		}
	}

	// some items will be prevented in deathmatch
	if (deathmatch->value)
	{
		if ( (int)dmflags->value & DF_NO_ARMOR )
		{
			if (item->pickup == Pickup_Armor || item->pickup == Pickup_PowerArmor)
			{
				G_FreeEdict (ent);
				return;
			}
		}
		if ( (int)dmflags->value & DF_NO_ITEMS )
		{
			if (item->pickup == Pickup_Powerup)
			{
				G_FreeEdict (ent);
				return;
			}
		}
		if ( (int)dmflags->value & DF_NO_HEALTH )
		{
			if (item->pickup == Pickup_Health || item->pickup == Pickup_Adrenaline || item->pickup == Pickup_AncientHead)
			{
				G_FreeEdict (ent);
				return;
			}
		}
		if ( (int)dmflags->value & DF_INFINITE_AMMO )
		{
			if ( (item->flags == IT_AMMO) || (strcmp(ent->classname, "weapon_bfg") == 0) )
			{
				G_FreeEdict (ent);
				return;
			}
		}
	}

	if (coop->value && (strcmp(ent->classname, "key_power_cube") == 0))
	{
		ent->spawnflags |= (1 << (8 + level.power_cubes));
		level.power_cubes++;
	}

	// don't let them drop items that stay in a coop game
	if ((coop->value) && (item->flags & IT_STAY_COOP))
	{
		item->drop = NULL;
	}

	// Lazarus: flashlight - get level-wide cost for use
	if(strcmp(ent->classname, "item_flashlight") == 0)
		level.flashlight_cost = ent->count;

	ent->item = item;
	ent->nextthink = level.time + 2 * FRAMETIME;    // items start after other solids
	ent->think = droptofloor;
	ent->s.skinnum = item->world_model_skinnum; //Knightmare- skinnum specified in item table
	ent->s.effects = item->world_model_flags;
	ent->s.renderfx = RF_GLOW;

	// Lazarus:
	if (item->pickup == Pickup_Health)
	{
		if (strcmp(ent->classname, "item_health_small") == 0)
			ent->count = sk_health_bonus_value->value;
		else
			ent->count = item->quantity;
		ent->style = item->tag;
	}
	if (ent->spawnflags & NO_STUPID_SPINNING)
	{
		ent->s.effects &= ~EF_ROTATE;
		//ent->s.renderfx &= ~RF_GLOW;		//CW want glowing
	}

	if (ent->model)
		gi.modelindex (ent->model);
}

//======================================================================

gitem_t	itemlist[] = 
{
	{
		NULL
	},	// leave index 0 alone

	//
	// ARMOR
	//

/*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_armor_body", 
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/body/tris.md2", 0, EF_ROTATE,
		NULL,
/* icon */		"i_bodyarmor",
/* pickup */	"Body Armor",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&bodyarmor_info,
		ARMOR_BODY,
/* precache */ ""
	},

/*QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_armor_combat", 
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/combat/tris.md2", 0, EF_ROTATE,
		NULL,
/* icon */		"i_combatarmor",
/* pickup */	"Combat Armor",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&combatarmor_info,
		ARMOR_COMBAT,
/* precache */ ""
	},

/*QUAKED item_armor_jacket (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_armor_jacket", 
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/jacket/tris.md2", 0, EF_ROTATE,
		NULL,
/* icon */		"i_jacketarmor",
/* pickup */	"Jacket Armor",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&jacketarmor_info,
		ARMOR_JACKET,
/* precache */ ""
	},

/*QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_armor_shard", 
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar2_pkup.wav",
		"models/items/armor/shard/tris.md2", 0, EF_ROTATE,
		NULL,
/* icon */		"i_jacketarmor",
/* pickup */	"Armor Shard",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		NULL,
		ARMOR_SHARD,
/* precache */ ""
	},


/*QUAKED item_power_screen (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_power_screen", 
		Pickup_PowerArmor,
		Use_PowerArmor,
		Drop_PowerArmor,
		NULL,
		"misc/ar3_pkup.wav",
		"models/items/armor/screen/tris.md2", 0, EF_ROTATE,
		NULL,
/* icon */		"i_powerscreen",
/* pickup */	"Power Screen",
/* width */		0,
		60,
		NULL,
		IT_ARMOR,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_power_shield (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_power_shield",
		Pickup_PowerArmor,
		Use_PowerArmor,
		Drop_PowerArmor,
		NULL,
		"misc/ar3_pkup.wav",
		"models/items/armor/shield/tris.md2", 0, EF_ROTATE,
		NULL,
/* icon */		"i_powershield",
/* pickup */	"Power Shield",
/* width */		0,
		60,
		NULL,
		IT_ARMOR,
		0,
		NULL,
		0,
/* precache */ "misc/power2.wav misc/power1.wav"
	},

#ifdef FLASHLIGHT_MOD
#if FLASHLIGHT_USE != POWERUP_USE_ITEM
	{
		"item_flashlight",
		Pickup_Powerup,
		Use_Flashlight,
	    Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/f_light/tris.md2", 0, EF_ROTATE,
		NULL,
		"p_flash",
		"Flashlight",
		2,
		60,
	    NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
		""
  },
#endif
#endif

	//
	// WEAPONS 
	//

/* weapon_blaster (.3 .3 1) (-16 -16 -16) (16 16 16)
always owned, never in the world
*/
/*	{
		"weapon_blaster", 
		NULL,
		Use_Weapon,
		NULL,
		Weapon_Blaster,
		"misc/w_pkup.wav",
		NULL, 0,
		"models/weapons/v_blast/tris.md2",
		"w_blaster",
		"Blaster",
		0,
		0,
		NULL,
		IT_WEAPON|IT_STAY_COOP,
		WEAP_BLASTER,
		NULL,
		0,
		"weapons/blastf1a.wav misc/lasfly.wav"
	}, */

// DWH

/*QUAKED weapon_blaster (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_blaster",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Blaster,
		"misc/w_pkup.wav",
		"models/weapons/g_blast/tris.md2", 0, EF_ROTATE,
		"models/weapons/v_blast/tris.md2",
		"w_blaster",
		"Blaster",
		0,
		0,
		NULL,
		IT_WEAPON|IT_STAY_COOP,
		WEAP_BLASTER,
		NULL,
		0,
		"weapons/blastf1a.wav misc/lasfly.wav"
	},

/*QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_shotgun", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Shotgun,
		"misc/w_pkup.wav",
		"models/weapons/g_shotg/tris.md2", 0, EF_ROTATE,
		"models/weapons/v_shotg/tris.md2",
/* icon */		"w_shotgun",
/* pickup */	"Shotgun",
		0,
		1,
		"Shells",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_SHOTGUN,
		NULL,
		0,
/* precache */ "weapons/shotgf1b.wav weapons/shotgr1b.wav"
	},

/*QUAKED weapon_supershotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_supershotgun", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_SuperShotgun,
		"misc/w_pkup.wav",
		"models/weapons/g_shotg2/tris.md2", 0, EF_ROTATE,
		"models/weapons/v_shotg2/tris.md2",
/* icon */		"w_sshotgun",
/* pickup */	"Super Shotgun",
		0,
		2,
		"Shells",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_SUPERSHOTGUN,
		NULL,
		0,
/* precache */ "weapons/sshotf1b.wav"
	},

/*QUAKED weapon_machinegun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_machinegun", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Machinegun,
		"misc/w_pkup.wav",
		"models/weapons/g_machn/tris.md2", 0, EF_ROTATE,
		"models/weapons/v_machn/tris.md2",
/* icon */		"w_machinegun",
/* pickup */	"Machinegun",
		0,
		1,
		"Bullets",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_MACHINEGUN,
		NULL,
		0,
/* precache */ "weapons/machgf1b.wav weapons/machgf2b.wav weapons/machgf3b.wav weapons/machgf4b.wav weapons/machgf5b.wav"
	},

/*QUAKED weapon_chaingun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_chaingun", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Chaingun,
		"misc/w_pkup.wav",
		"models/weapons/g_chain/tris.md2", 0, EF_ROTATE,
		"models/weapons/v_chain/tris.md2",
/* icon */		"w_chaingun",
/* pickup */	"Chaingun",
		0,
		1,
		"Bullets",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_CHAINGUN,
		NULL,
		0,
/* precache */ "weapons/chngnu1a.wav weapons/chngnl1a.wav weapons/machgf3b.wav` weapons/chngnd1a.wav"
	},

/*QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_grenades",
		Pickup_Ammo,
		Use_Weapon,
		Drop_Ammo,
		Weapon_Grenade,
		"misc/am_pkup.wav",
		"models/items/ammo/grenades/medium/tris.md2", 0, 0,
		"models/weapons/v_handgr/tris.md2",
/* icon */		"a_grenades",
/* pickup */	"Grenades",
/* width */		3,
		5,
		"grenades",
		IT_AMMO|IT_WEAPON,
		WEAP_GRENADES,
		NULL,
		AMMO_GRENADES,
/* precache */ "weapons/hgrent1a.wav weapons/hgrena1b.wav weapons/hgrenc1b.wav weapons/hgrenb1a.wav weapons/hgrenb2a.wav "
	},

/*QUAKED weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_grenadelauncher",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_GrenadeLauncher,
		"misc/w_pkup.wav",
		"models/weapons/g_launch/tris.md2", 0, EF_ROTATE,
		"models/weapons/v_launch/tris.md2",
/* icon */		"w_glauncher",
/* pickup */	"Grenade Launcher",
		0,
		1,
		"Grenades",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_GRENADELAUNCHER,
		NULL,
		0,
/* precache */ "models/objects/grenade/tris.md2 weapons/grenlf1a.wav weapons/grenlr1b.wav weapons/grenlb1b.wav"
	},

/*QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_rocketlauncher",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_RocketLauncher,
		"misc/w_pkup.wav",
		"models/weapons/g_rocket/tris.md2", 0, EF_ROTATE,
		"models/weapons/v_rocket/tris.md2",
/* icon */		"w_rlauncher",
/* pickup */	"Rocket Launcher",
		0,
		1,
		"Rockets",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_ROCKETLAUNCHER,
		NULL,
		0,
/* precache */ "models/objects/rocket/tris.md2 weapons/rockfly.wav weapons/rocklf1a.wav weapons/rocklr1b.wav models/objects/debris2/tris.md2"
	},

/*QUAKED weapon_hyperblaster (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_hyperblaster", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_HyperBlaster,
		"misc/w_pkup.wav",
		"models/weapons/g_hyperb/tris.md2", 0, EF_ROTATE,
		"models/weapons/v_hyperb/tris.md2",
/* icon */		"w_hyperblaster",
/* pickup */	"HyperBlaster",
		0,
		1,
		"Cells",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_HYPERBLASTER,
		NULL,
		0,
/* precache */ "weapons/hyprbu1a.wav weapons/hyprbl1a.wav weapons/hyprbf1a.wav weapons/hyprbd1a.wav misc/lasfly.wav"
	},

/*QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_railgun", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Railgun,
		"misc/w_pkup.wav",
		"models/weapons/g_rail/tris.md2", 0, EF_ROTATE,
		"models/weapons/v_rail/tris.md2",
/* icon */		"w_railgun",
/* pickup */	"Railgun",
		0,
		1,
		"Slugs",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_RAILGUN,
		NULL,
		0,
/* precache */ "weapons/rg_hum.wav"
	},

/*QUAKED weapon_bfg (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_bfg",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_BFG,
		"misc/w_pkup.wav",
		"models/weapons/g_bfg/tris.md2", 0, EF_ROTATE,
		"models/weapons/v_bfg/tris.md2",
/* icon */		"w_bfg",
/* pickup */	"BFG10K",
		0,
		50,
		"Cells",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_BFG,
		NULL,
		0,
/* precache */ "sprites/s_bfg1.sp2 sprites/s_bfg2.sp2 sprites/s_bfg3.sp2 weapons/bfg__f1y.wav weapons/bfg__l1a.wav weapons/bfg__x1b.wav weapons/bfg_hum.wav"
	},

/*QUAKED weapon_hml (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_hml",
		NULL,
		Use_Weapon,
		NULL,
		Weapon_HomingMissileLauncher,
		NULL,
		NULL, 0, EF_ROTATE,
		"models/weapons/v_homing/tris.md2",
/* icon */		NULL,
/* pickup */	"Homing Rocket Launcher",
		0,
		1,
		"homing rockets",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_ROCKETLAUNCHER,
		NULL,
		0,
/* precache */ "models/objects/rocket/tris.md2 weapons/rockfly.wav weapons/rocklf1a.wav weapons/rocklr1b.wav models/objects/debris2/tris.md2"
	},

	// Lazarus: No weapon - we HAVE to have a weapon
	{
		"weapon_null",
		NULL,
		Use_Weapon,
		NULL,
		Weapon_Null,
		"misc/w_pkup.wav",
		NULL, 0, 0,
		NULL,
		NULL,
		"No Weapon",
	  	0,
		0,
		NULL,
		IT_WEAPON|IT_STAY_COOP,
		WEAP_NONE,
		NULL,
		0,
		""
	},

	//
	// AMMO ITEMS
	//

/*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_shells",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/shells/medium/tris.md2", 0, 0,
		NULL,
/* icon */		"a_shells",
/* pickup */	"Shells",
/* width */		3,
		10,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_SHELLS,
/* precache */ ""
	},

/*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_bullets",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/bullets/medium/tris.md2", 0, 0,
		NULL,
/* icon */		"a_bullets",
/* pickup */	"Bullets",
/* width */		3,
		50,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_BULLETS,
/* precache */ ""
	},

/*QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_cells",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/cells/medium/tris.md2", 0, 0,
		NULL,
/* icon */		"a_cells",
/* pickup */	"Cells",
/* width */		3,
		50,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_CELLS,
/* precache */ ""
	},

/*QUAKED ammo_fuel (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_fuel",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/fuel/medium/tris.md2", 0, 0,
		NULL,
/* icon */		"a_fuel",
/* pickup */	"fuel",
/* width */		4,
		500,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_FUEL,
/* precache */ ""
	},


/*QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_rockets",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/rockets/medium/tris.md2", 0, 0,
		NULL,
/* icon */		"a_rockets",
/* pickup */	"Rockets",
/* width */		3,
		5,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_ROCKETS,
/* precache */ ""
	},

/*QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_slugs",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/slugs/medium/tris.md2", 0, 0,
		NULL,
/* icon */		"a_slugs",
/* pickup */	"Slugs",
/* width */		3,
		10,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_SLUGS,
/* precache */ ""
	},

/*QUAKED ammo_homing_missiles (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_homing_missiles",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/homing/medium/tris.md2", 0, 0,
		NULL,
/* icon */		"a_homing",
/* pickup */	"Homing Rockets",
/* width */		3,
		5,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_HOMING_ROCKETS,
/* precache */ ""
	},

	//
	// POWERUP ITEMS
	//
/*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_quad", 
		Pickup_Powerup,
		Use_Quad,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/quaddama/tris.md2", 0, EF_ROTATE,
		NULL,
/* icon */		"p_quad",
/* pickup */	"Quad Damage",
/* width */		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/damage.wav items/damage2.wav items/damage3.wav"
	},

/*QUAKED item_invulnerability (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_invulnerability",
		Pickup_Powerup,
		Use_Invulnerability,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/invulner/tris.md2", 0, EF_ROTATE,
		NULL,
/* icon */		"p_invulnerability",
/* pickup */	"Invulnerability",
/* width */		2,
		300,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/protect.wav items/protect2.wav items/protect4.wav"
	},

/*QUAKED item_silencer (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_silencer",
		Pickup_Powerup,
		Use_Silencer,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/silencer/tris.md2", 0, EF_ROTATE,
		NULL,
/* icon */		"p_silencer",
/* pickup */	"Silencer",
/* width */		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_breather (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_breather",
		Pickup_Powerup,
		Use_Breather,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/breather/tris.md2", 0, EF_ROTATE,
		NULL,
/* icon */		"p_rebreather",
/* pickup */	"Rebreather",
/* width */		2,
		60,
		NULL,
		IT_STAY_COOP|IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/airout.wav"
	},

/*QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_enviro",
		Pickup_Powerup,
		Use_Envirosuit,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/enviro/tris.md2", 0, EF_ROTATE,
		NULL,
/* icon */		"p_envirosuit",
/* pickup */	"Environment Suit",
/* width */		2,
		60,
		NULL,
		IT_STAY_COOP|IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/airout.wav"
	},

/*QUAKED item_ancient_head (.3 .3 1) (-16 -16 -16) (16 16 16)
Special item that gives +2 to maximum health
*/
	{
		"item_ancient_head",
		Pickup_AncientHead,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/c_head/tris.md2", 0, EF_ROTATE,
		NULL,
/* icon */		"i_fixme",
/* pickup */	"Ancient Head",
/* width */		2,
		60,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_adrenaline (.3 .3 1) (-16 -16 -16) (16 16 16)
gives +1 to maximum health
*/
	{
		"item_adrenaline",
		Pickup_Adrenaline,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/adrenal/tris.md2", 0, EF_ROTATE,
		NULL,
/* icon */		"p_adrenaline",
/* pickup */	"Adrenaline",
/* width */		2,
		60,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_bandolier (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_bandolier",
		Pickup_Bandolier,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/band/tris.md2", 0, EF_ROTATE,
		NULL,
/* icon */		"p_bandolier",
/* pickup */	"Bandolier",
/* width */		2,
		60,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_pack (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_pack",
		Pickup_Pack,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/pack/tris.md2", 0, EF_ROTATE,
		NULL,
/* icon */		"i_pack",
/* pickup */	"Ammo Pack",
/* width */		2,
		180,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ ""
	},

#ifdef JETPACK_MOD
	{
		"item_jetpack",
		Pickup_Powerup,
		Use_Jet,
	    Drop_Jetpack,
		NULL,
		"items/pkup.wav",
		"models/items/jet/tris.md2", 0, EF_ROTATE,
		NULL,
		"p_jet",
		"Jetpack",
		2,
		600,
	    "fuel",
		IT_POWERUP,
		0,
		NULL,
		0,
		"jetpack/activate.wav jetpack/rev1.wav jetpack/revrun.wav jetpack/running.wav jetpack/shutdown.wav jetpack/stutter.wav"
  },
#endif

	{
		"item_freeze",
		Pickup_Powerup,
		Use_Stasis,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/stasis/tris.md2", 0, EF_ROTATE,
		NULL,
		"p_freeze",
		"Stasis Generator",
		2,
		30,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
		"items/stasis_start.wav items/stasis.wav items/stasis_stop.wav"
	},
	//
	// KEYS
	//
/*QUAKED key_data_cd (0 .5 .8) (-16 -16 -16) (16 16 16)
key for computer centers
*/
	{
		"key_data_cd",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/data_cd/tris.md2", 0, EF_ROTATE,
		NULL,
		"k_datacd",
		"Data CD",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_power_cube (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN NO_TOUCH
warehouse circuits
*/
	{
		"key_power_cube",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/power/tris.md2", 0, EF_ROTATE,
		NULL,
		"k_powercube",
		"Power Cube",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

//CW+++
/*QUAKED key_hipower_cube (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN NO_TOUCH
Over-charged power cube - handle with extreme care!
*/
	{
		"key_hipower_cube",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/power/tris.md2", 0, EF_ROTATE,
		NULL,
		"k_powercube",
		"Hi-Power Cube",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},
//CW---

/*QUAKED key_pyramid (0 .5 .8) (-16 -16 -16) (16 16 16)
key for the entrance of jail3
*/
	{
		"key_pyramid",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/pyramid/tris.md2", 0, EF_ROTATE,
		NULL,
		"k_pyramid",
		"Pyramid Key",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_data_spinner (0 .5 .8) (-16 -16 -16) (16 16 16)
key for the city computer
*/
	{
		"key_data_spinner",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/spinner/tris.md2", 0, EF_ROTATE,
		NULL,
		"k_dataspin",
		"Data Spinner",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_pass (0 .5 .8) (-16 -16 -16) (16 16 16)
security pass for the security level
*/
	{
		"key_pass",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/pass/tris.md2", 0, EF_ROTATE,
		NULL,
		"k_security",
		"Security Pass",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

//CW+++
/*QUAKED key_hackedpass (0 .5 .8) (-16 -16 -16) (16 16 16)
hacked security pass 
*/
	{
		"key_hackedpass",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/pass/tris.md2", 0, EF_ROTATE,
		NULL,
		"k_security",
		"Security Pass L2",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},
//CW---

/*QUAKED key_blue_key (0 .5 .8) (-16 -16 -16) (16 16 16)
normal door key - blue
*/
	{
		"key_blue_key",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/key/tris.md2", 0, EF_ROTATE,
		NULL,
		"k_bluekey",
		"Blue Key",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_red_key (0 .5 .8) (-16 -16 -16) (16 16 16)
normal door key - red
*/
	{
		"key_red_key",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/red_key/tris.md2", 0, EF_ROTATE,
		NULL,
		"k_redkey",
		"Red Key",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_commander_head (0 .5 .8) (-16 -16 -16) (16 16 16)
tank commander's head
*/
	{
		"key_commander_head",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/monsters/commandr/head/tris.md2", 0, EF_GIB,
		NULL,
/* icon */		"k_comhead",
/* pickup */	"Commander's Head",
/* width */		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY|IT_INDESTRUCTABLE,			//CW
		0,
		NULL,
		0,
/* precache */ ""
	},

//CW++
/*QUAKED key_captain_head (0 .5 .8) (-16 -16 -16) (16 16 16)
tank commander's head
*/
	{
		"key_captain_head",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/monsters/commandr/head/tris.md2", 0, EF_GIB,
		NULL,
/* icon */		"k_comhead",
/* pickup */	"Captain's Head",
/* width */		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY|IT_INDESTRUCTABLE,
		0,
		NULL,
		0,
/* precache */ ""
	},
//CW--

/*QUAKED key_airstrike_target (0 .5 .8) (-16 -16 -16) (16 16 16)
tank commander's head
*/
	{
		"key_airstrike_target",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/target/tris.md2", 0, EF_ROTATE,
		NULL,
/* icon */		"i_airstrike",
/* pickup */	"Airstrike Marker",
/* width */		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},
/*
	{
		NULL,
		Pickup_Health,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		NULL, 0, 0,
		NULL,
		"i_health",
		"Health",
		3,
		0,
		NULL,
		0,
		0,
		NULL,
		0,
		"items/s_health.wav items/n_health.wav items/l_health.wav items/m_health.wav"
	},
*/
	// Lazarus: Dunno what adding actual health models might end up fouling up, but
	// we'll give it a try for now. This will allow monsters to give up health.

	{
		"item_health_small",
		Pickup_Health,
		NULL,
		NULL,
		NULL,
		"items/s_health.wav",
		"models/items/healing/stimpack/tris.md2", 0, 0,
		NULL,
		"i_health",
		"Health",
		3,
		2,
		NULL,
		0,
		0,
		NULL,
		HEALTH_IGNORE_MAX,
		"items/s_health.wav"
	},

	{
		"item_health",
		Pickup_Health,
		NULL,
		NULL,
		NULL,
		"items/n_health.wav",
		"models/items/healing/medium/tris.md2", 0, 0,
		NULL,
		"i_health",
		"Health",
		3,
		10,
		NULL,
		0,
		0,
		NULL,
		0,
		"items/n_health.wav"
	},

	{
		"item_health_large",
		Pickup_Health,
		NULL,
		NULL,
		NULL,
		"items/l_health.wav",
		"models/items/healing/large/tris.md2", 0, 0,
		NULL,
		"i_health",
		"Health",
		3,
		25,
		NULL,
		0,
		0,
		NULL,
		0,
		"items/l_health.wav"
	},

	{
		"item_health_mega",
		Pickup_Health,
		NULL,
		NULL,
		NULL,
		"items/m_health.wav",
		"models/items/mega_h/tris.md2", 0, 0,
		NULL,
		"i_health",
		"Health",
		3,
		100,
		NULL,
		0,
		0,
		NULL,
		//HEALTH_IGNORE_MAX | HEALTH_TIMED,		//CW: commented out so that...
		HEALTH_IGNORE_MAX,						//CW: ...MH doesn't count down
		"items/m_health.wav"
	},

	// end of list marker
	{NULL}
};


// QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16)

void SP_item_health (edict_t *self)
{
	if ( deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/healing/medium/tris.md2";
	self->count = 10;
//	SpawnItem (self, FindItem ("Health"));
	SpawnItem (self, FindItemByClassname ("item_health"));
	gi.soundindex ("items/n_health.wav");
}

// QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16)

void SP_item_health_small (edict_t *self)
{
	if ( deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	gi.dprintf("HEY - SP_item_health_small\n");

	self->model = "models/items/healing/stimpack/tris.md2";
	self->count = sk_health_bonus_value->value;
//	SpawnItem (self, FindItem ("Health"));
	SpawnItem (self, FindItemByClassname ("item_health_small"));
	self->style = HEALTH_IGNORE_MAX;
	gi.soundindex ("items/s_health.wav");
}

// QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16)

void SP_item_health_large (edict_t *self)
{
	if ( deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/healing/large/tris.md2";
	self->count = 25;
//	SpawnItem (self, FindItem ("Health"));
	SpawnItem (self, FindItemByClassname ("item_health_large"));
	gi.soundindex ("items/l_health.wav");
}

// QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16)

void SP_item_health_mega (edict_t *self)
{
	if ( deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/mega_h/tris.md2";
	self->count = 100;
//	SpawnItem (self, FindItem ("Health"));
	SpawnItem (self, FindItemByClassname ("item_health_mega"));
	gi.soundindex ("items/m_health.wav");

//	self->style = HEALTH_IGNORE_MAX|HEALTH_TIMED;	//CW: commented out so that...
	self->style = HEALTH_IGNORE_MAX;				//CW: ...MH doesn't count down
}


void InitItems (void)
{
	game.num_items = sizeof(itemlist)/sizeof(itemlist[0]) - 1;
}



/*
===============
SetItemNames

Called by worldspawn
===============
*/
void SetItemNames (void)
{
	int		i;
	gitem_t	*it;

	for (i=0 ; i<game.num_items ; i++)
	{
		it = &itemlist[i];
		gi.configstring (CS_ITEMS+i, it->pickup_name);
	}

	noweapon_index     = ITEM_INDEX(FindItem("No Weapon"));
	jacket_armor_index = ITEM_INDEX(FindItem("Jacket Armor"));
	combat_armor_index = ITEM_INDEX(FindItem("Combat Armor"));
	body_armor_index   = ITEM_INDEX(FindItem("Body Armor"));
	power_screen_index = ITEM_INDEX(FindItem("Power Screen"));
	power_shield_index = ITEM_INDEX(FindItem("Power Shield"));
	shells_index       = ITEM_INDEX(FindItem("shells"));
	bullets_index      = ITEM_INDEX(FindItem("bullets"));
	grenades_index     = ITEM_INDEX(FindItem("Grenades"));
	rockets_index      = ITEM_INDEX(FindItem("rockets"));
	cells_index        = ITEM_INDEX(FindItem("cells"));
	slugs_index        = ITEM_INDEX(FindItem("slugs"));
	fuel_index         = ITEM_INDEX(FindItem("fuel"));
	homing_index       = ITEM_INDEX(FindItem("homing rockets"));
	rl_index           = ITEM_INDEX(FindItem("rocket launcher"));
	hml_index          = ITEM_INDEX(FindItem("Homing Rocket Launcher"));
}

/*
==================
Use_Flashlight
==================
*/
void Use_Flashlight ( edict_t *ent, gitem_t *item )
{
	if(!ent->client->flashlight)
	{
		if(ent->client->pers.inventory[ITEM_INDEX(FindItem(FLASHLIGHT_ITEM))] < level.flashlight_cost)
		{
			gi.cprintf(ent,PRINT_HIGH,"Flashlight requires %s\n",FLASHLIGHT_ITEM);
			return;
		}
#if FLASHLIGHT_USE != POWERUP_USE_ITEM
	/*  Lazarus: We never "use up" the flashlight
		ent->client->pers.inventory[ITEM_INDEX(item)]--; */
		ValidateSelectedItem (ent);
#endif
	}
	if(ent->client->flashlight ^= 1)
		ent->client->flashlight_time = level.time + FLASHLIGHT_DRAIN;
}

#ifdef JETPACK_MOD
//==============================================================================
void Use_Jet ( edict_t *ent, gitem_t *item )
{
	if(ent->client->jetpack)
	{
		// Currently on... turn it off and store remaining time
		ent->client->jetpack = false;
		ent->client->jetpack_framenum  = 0;
		// Force frame. While using the jetpack ClientThink forces the frame to
		// stand20 when it really SHOULD be jump2. This is fine, but if we leave
		// it at that then the player cycles through the wrong frames to complete
		// his "jump" when the jetpack is turned off. The same thing is done in 		// ClientThink when jetpack timer expires.
		ent->s.frame = 67;
		gi.sound(ent,CHAN_GIZMO,gi.soundindex("jetpack/shutdown.wav"), 1, ATTN_NORM, 0);
	}
	else
	{
		// Currently off. Turn it on, and add time, if any, remaining
		// from last jetpack.
		if( ent->client->pers.inventory[ITEM_INDEX(item)] )
		{
			ent->client->jetpack = true;
			// Lazarus: Never remove jetpack from inventory (unless dropped)
			// ent->client->pers.inventory[ITEM_INDEX(item)]--;
			ValidateSelectedItem (ent);
			ent->client->jetpack_framenum = level.framenum;
			ent->client->jetpack_activation = level.framenum;
		}
		else if(ent->client->pers.inventory[fuel_index] > 0)
		{
			ent->client->jetpack = true;
			ent->client->jetpack_framenum = level.framenum;
			ent->client->jetpack_activation = level.framenum;
		}
		else
			return;  // Shouldn't have been able to get here, but I'm a pessimist
		gi.sound( ent, CHAN_GIZMO, gi.soundindex("jetpack/activate.wav"), 1, ATTN_NORM, 0);
	}
}
#endif

// Lazarus: Stasis field generator
void Use_Stasis ( edict_t *ent, gitem_t *item )
{
	if(ent->client->jetpack)
	{
		gi.dprintf("Cannot use stasis generator while using jetpack\n");
		return;
	}
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);
	level.freeze = true;
	level.freezeframes = 0;
	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/stasis_start.wav"), 1, ATTN_NORM, 0);
}
