// g_local.h -- local definitions for game module


#include "q_shared.h"

// define GAME_INCLUDE so that game.h does not define the
// short, server-visible gclient_t and edict_t structures,
// because we define the full size ones in this file
#define	GAME_INCLUDE
#include <stdint.h>
#include "game.h"
#include "p_menu.h"
#include "p_text.h"
#include "km_cvar.h"
#define JETPACK_MOD

#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif

// the "gameversion" client command will print this plus compile date
#define	GAMEVERSION	"lazarus_smd"

#define PI		3.14159265359

// protocol bytes that can be directly added to messages
#define	svc_muzzleflash		1
#define	svc_muzzleflash2	2
#define	svc_temp_entity		3
#define	svc_layout			4
#define	svc_inventory		5
#define	svc_stufftext		11
#define	svc_fog				21

//==================================================================

// Lazarus: When visibility is reduced below this level, aiming accuracy suffers:
#define FOG_CANSEEGOOD 0.12

// view pitching times
#define DAMAGE_TIME		0.5
#define	FALL_TIME		0.3


// edict->spawnflags
// these are set with checkboxes on each entity in the map editor
#define	SPAWNFLAG_NOT_EASY			0x00000100
#define	SPAWNFLAG_NOT_MEDIUM		0x00000200
#define	SPAWNFLAG_NOT_HARD			0x00000400
#define	SPAWNFLAG_NOT_DEATHMATCH	0x00000800

// edict->flags
#define	FL_FLY					0x00000001
#define	FL_SWIM					0x00000002	// implied immunity to drowining
#define FL_IMMUNE_LASER			0x00000004
#define	FL_INWATER				0x00000008
#define	FL_GODMODE				0x00000010
#define	FL_NOTARGET				0x00000020
#define FL_IMMUNE_SLIME			0x00000040
#define FL_IMMUNE_LAVA			0x00000080
#define	FL_PARTIALGROUND		0x00000100	// not all corners are valid
#define	FL_WATERJUMP			0x00000200	// player jumping out of water
#define	FL_TEAMSLAVE			0x00000400	// not the first on the team
#define FL_NO_KNOCKBACK			0x00000800
//#define FL_POWER_ARMOR			0x00001000	// power armor (if any) is active
#define FL_POWER_SHIELD			0x00001000	// power armor (if any) is active
#define FL_POWER_SCREEN			0x00002000

#define FL_BOB                  0x00004000  // Lazarus: Used for bobbing water
#define	FL_TURRET_OWNER			0x00008000  // Lazarus: player on turret and controlling it
#define FL_TRACKTRAIN			0x00010000	
#define FL_DISGUISED			0x00020000	// entity is in disguise, monsters will not recognize.
#define	FL_NOGIB				0x00040000	// player has been vaporized by a nuke, drop no gibs

#define FL_REVERSIBLE           0x00080000	// Lazarus: used for reversible func_door_rotating
#define FL_REVOLVING            0x00100000	// Lazarus revolving door
#define FL_ROBOT				0x00200000	// Player-controlled robot or monster. Relax yaw constraints
#define FL_REFLECT              0x00400000	// Reflection entity

#define FL_RESPAWN				0x80000000	// used for item respawning


#define	FRAMETIME		0.1

// memory tags to allow dynamic memory to be cleaned up
#define	TAG_GAME	765		// clear when unloading the dll
#define	TAG_LEVEL	766		// clear when loading a new level


#define MELEE_DISTANCE	80

#define BODY_QUEUE_SIZE		8

typedef enum
{
	DAMAGE_NO,
	DAMAGE_YES,			// will take damage if hit
	DAMAGE_AIM			// auto targeting recognizes this
} damage_t;

typedef enum 
{
	WEAPON_READY, 
	WEAPON_ACTIVATING,
	WEAPON_DROPPING,
	WEAPON_FIRING
} weaponstate_t;

typedef enum
{
	AMMO_BULLETS,
	AMMO_SHELLS,
	AMMO_ROCKETS,
	AMMO_GRENADES,
	AMMO_CELLS,
	AMMO_SLUGS,
	AMMO_FUEL,
	AMMO_HOMING_ROCKETS
} ammo_t;

//deadflag
#define DEAD_NO					0
#define DEAD_DYING				1
#define DEAD_DEAD				2
#define DEAD_RESPAWNABLE		3
#define DEAD_FROZEN             4  // Lazarus: Don't shift angles, just freeze him

//range
#define RANGE_MELEE				0
#define RANGE_NEAR				1
#define RANGE_MID				2
#define RANGE_FAR				3

//gib types
#define GIB_ORGANIC				0
#define GIB_METALLIC			1

//monster ai flags
#define AI_STAND_GROUND			0x00000001
#define AI_TEMP_STAND_GROUND	0x00000002
#define AI_SOUND_TARGET			0x00000004
#define AI_LOST_SIGHT			0x00000008
#define AI_PURSUIT_LAST_SEEN	0x00000010
#define AI_PURSUE_NEXT			0x00000020
#define AI_PURSUE_TEMP			0x00000040
#define AI_HOLD_FRAME			0x00000080
#define AI_GOOD_GUY				0x00000100
#define AI_BRUTAL				0x00000200
#define AI_NOSTEP				0x00000400
#define AI_DUCKED				0x00000800
#define AI_COMBAT_POINT			0x00001000
#define AI_MEDIC				0x00002000
#define AI_RESURRECTING			0x00004000
//ROGUE (Lazarus: Eliminate many inapplicable Rogue AI flags to make room for more)
#define AI_TARGET_ANGER			0x00008000
#define AI_HINT_PATH			0x00010000
#define	AI_BLOCKED				0x00020000	// used by blocked_checkattack: set to say I'm attacking while blocked 
											// (prevents run-attacks)
//ROGUE
// Lazarus:
#define AI_ACTOR                0x00040000  // Is this a misc_actor?
#define AI_FOLLOW_LEADER        0x00080000  // misc_actor only
#define AI_TWO_GUNS             0x00100000  // misc_actor only - nothing to do with AI really,
                                            // but we're out of spawnflags
#define AI_RESPAWN_FINDPLAYER   0x00200000  // used for monsters that change maps with
                                            // a trigger_transition... tells 'em to find SP
                                            // player right away
#define AI_FREEFORALL           0x00400000  // Set by target_monsterbattle, lets dmgteam monsters
                                            // attack monsters on opposion dmgteam
#define AI_RANGE_PAUSE          0x00800000
#define AI_CHASE_THING          0x01000000
#define AI_SEEK_COVER           0x02000000
#define AI_CHICKEN              0x04000000
#define AI_MEDIC_PATROL         0x08000000
#define AI_HINT_TEST            0x10000000
#define AI_CROUCH               0x20000000
#define AI_EVADE_GRENADE		0x40000000

//monster attack state
#define AS_STRAIGHT				1
#define AS_SLIDING				2
#define	AS_MELEE				3
#define	AS_MISSILE				4
#define	AS_BLIND				5	// PMM - used by boss code to do nasty things even if it can't see you

// armor types
#define ARMOR_NONE				0
#define ARMOR_JACKET			1
#define ARMOR_COMBAT			2
#define ARMOR_BODY				3
#define ARMOR_SHARD				4

// power armor types
#define POWER_ARMOR_NONE		0
#define POWER_ARMOR_SCREEN		1
#define POWER_ARMOR_SHIELD		2

// handedness values
#define RIGHT_HANDED			0
#define LEFT_HANDED				1
#define CENTER_HANDED			2


// game.serverflags values
#define SFL_CROSS_TRIGGER_1		0x00000001
#define SFL_CROSS_TRIGGER_2		0x00000002
#define SFL_CROSS_TRIGGER_3		0x00000004
#define SFL_CROSS_TRIGGER_4		0x00000008
#define SFL_CROSS_TRIGGER_5		0x00000010
#define SFL_CROSS_TRIGGER_6		0x00000020
#define SFL_CROSS_TRIGGER_7		0x00000040
#define SFL_CROSS_TRIGGER_8		0x00000080
#define SFL_CROSS_TRIGGER_MASK	0x000000ff


// noise types for PlayerNoise
#define PNOISE_SELF				0
#define PNOISE_WEAPON			1
#define PNOISE_IMPACT			2

// actor follow parms
#define ACTOR_FOLLOW_RUN_RANGE   256    // AI_FOLLOW_LEADER actors run if farther away than this
#define ACTOR_FOLLOW_STAND_RANGE 128    //       ..          ..    stand if closer than this

// edict->movetype values
typedef enum
{
MOVETYPE_NONE,			// never moves
MOVETYPE_NOCLIP,		// origin and angles change with no interaction
MOVETYPE_PUSH,			// no clip to world, push on box contact
MOVETYPE_STOP,			// no clip to world, stops on box contact

MOVETYPE_WALK,			// gravity
MOVETYPE_STEP,			// gravity, special edge handling
MOVETYPE_FLY,
MOVETYPE_TOSS,			// gravity
MOVETYPE_FLYMISSILE,	// extra size to monsters
MOVETYPE_BOUNCE,
MOVETYPE_VEHICLE,
MOVETYPE_PUSHABLE,
MOVETYPE_DEBRIS,		// non-solid debris that can still hurt you
MOVETYPE_RAIN,			// identical to MOVETYPE_FLYMISSILE, but doesn't cause splash noises
						//   when touching water.
MOVETYPE_PENDULUM,		// same as MOVETYPE_PUSH, but used only for pendulums to grab special-case
						//   problems
MOVETYPE_CONVEYOR		// func_conveyor
} movetype_t;



typedef struct
{
	int		base_count;
	int		max_count;
	float	normal_protection;
	float	energy_protection;
	int		armor;
} gitem_armor_t;


// gitem_t->flags
#define	IT_WEAPON			1		// use makes active weapon
#define	IT_AMMO				2
#define IT_ARMOR			4
#define IT_STAY_COOP		8
#define IT_KEY				16
#define IT_POWERUP			32
#define IT_INDESTRUCTABLE	64		//CW++

// gitem_t->weapmodel for weapons indicates model index
#define WEAP_BLASTER			1 
#define WEAP_SHOTGUN			2 
#define WEAP_SUPERSHOTGUN		3 
#define WEAP_MACHINEGUN			4 
#define WEAP_CHAINGUN			5 
#define WEAP_GRENADES			6 
#define WEAP_GRENADELAUNCHER	7 
#define WEAP_ROCKETLAUNCHER		8 
#define WEAP_HYPERBLASTER		9 
#define WEAP_RAILGUN			10
#define WEAP_BFG				11
#define WEAP_NONE               12

typedef struct gitem_s
{
	char		*classname;	// spawning name
	qboolean	(*pickup)(struct edict_s *ent, struct edict_s *other);
	void		(*use)(struct edict_s *ent, struct gitem_s *item);
	void		(*drop)(struct edict_s *ent, struct gitem_s *item);
	void		(*weaponthink)(struct edict_s *ent);
	char		*pickup_sound;
	char		*world_model;
	int			world_model_skinnum; //Knightmare- added skinnum here so items can share models
	int			world_model_flags;
	char		*view_model;

	// client side info
	char		*icon;
	char		*pickup_name;	// for printing on pickup
	int			count_width;		// number of digits to display by icon

	int			quantity;		// for ammo how much, for weapons how much is used per shot
	char		*ammo;			// for weapons
	int			flags;			// IT_* flags

	int			weapmodel;		// weapon model index (for weapons)

	void		*info;
	int			tag;

	char		*precaches;		// string of all models, sounds, and images this item will use
} gitem_t;



//
// this structure is left intact through an entire game
// it should be initialized at dll load time, and read/written to
// the server.ssv file for savegames
//
typedef struct
{
	char		helpmessage1[512];
	char		helpmessage2[512];
	int			helpchanged;	// flash F1 icon if non 0, play sound
								// and increment only if 1, 2, or 3

	gclient_t	*clients;		// [maxclients]

	// can't store spawnpoint in level, because
	// it would get overwritten by the savegame restore
	char		spawnpoint[512];	// needed for coop respawns

	// store latched cvars here that we want to get at often
	int			maxclients;
	int			maxentities;

	// cross level triggers
	int			serverflags;

	// Lazarus: target_lock combination
	char		lock_code[9];
	int			lock_revealed;
	qboolean	lock_hud;
	// Lazarus: number of entities moved between maps (not counting players)
	int			transition_ents;

	// items
	int			num_items;

//CW++
	int			clock_count;
	int			clock_ticking;
//CW--

	qboolean	autosaved;
} game_locals_t;

struct fog_s
{
	qboolean	Trigger;
	int			Model;
	float		Near;
	float		Far;
	float		Density;
	float		Density1;
	float		Density2;
	vec3_t		Dir;
	int			GL_Model;
	vec3_t		Color;
	vec3_t		GlideColor;
	edict_t		*ent;
};
typedef struct fog_s fog_t;

//
// this structure is cleared as each map is entered
// it is read/written to the level.sav file for savegames
//
typedef struct
{
	int			framenum;
	float		time;

	char		level_name[MAX_QPATH];	// the descriptive name (Outer Base, etc)
	char		mapname[MAX_QPATH];		// the server name (base1, etc)
	char		nextmap[MAX_QPATH];		// go here when fraglimit is hit

	// intermission state
	float		intermissiontime;		// time the intermission was started
	char		*changemap;
	int			exitintermission;
	vec3_t		intermission_origin;
	vec3_t		intermission_angle;

	edict_t		*sight_client;	// changed once each frame for coop games

	edict_t		*sight_entity;
	int			sight_entity_framenum;
	edict_t		*sound_entity;
	int			sound_entity_framenum;
	edict_t		*sound2_entity;
	int			sound2_entity_framenum;

	int			pic_health;

	int			total_secrets;
	int			found_secrets;

	int			total_goals;
	int			found_goals;

	int			total_monsters;
	int			killed_monsters;

	edict_t		*current_entity;	// entity running from G_RunFrame
	int			body_que;			// dead bodies

	int			power_cubes;		// ugly necessity for coop

	// ROGUE
	edict_t		*disguise_violator;
	int			disguise_violation_framenum;
	// ROGUE

	// Lazarus
	int			fogs;
	int			trigger_fogs;
	int			active_target_fog;
	int			active_fog;
	int			last_active_fog;
	fog_t		fog;
	int			flashlight_cost;	// cost/10 seconds for flashlight
	int			mud_puddles;
	int			num_3D_sounds;
	qboolean	restart_for_actor_models;
	qboolean	freeze;
	int			freezeframes;
	int			next_skill;
	int			num_reflectors;
	qboolean	intermission_letterbox;		// Knightmare- letterboxing

} level_locals_t;


// spawn_temp_t is only used to hold entity field values that
// can be set from the editor, but aren't actually present
// in edict_t during gameplay
typedef struct
{
	// world vars
	char		*sky;
	float		skyrotate;
	vec3_t		skyaxis;
	char		*nextmap;

	int			lip;
	int			distance;
	int			height;
	char		*noise;
	float		pausetime;
	char		*item;
	char		*gravity;

	float		minyaw;
	float		maxyaw;
	float		minpitch;
	float		maxpitch;
	float		phase;

	float		shift;
} spawn_temp_t;


typedef struct
{
	// fixed data
	vec3_t		start_origin;
	vec3_t		start_angles;
	vec3_t		end_origin;
	vec3_t		end_angles;

	int			sound_start;
	int			sound_middle;
	int			sound_end;

	float		accel;
	float		speed;
	float		decel;
	float		distance;

	float		wait;

	// state data
	int			state;
	int			prevstate;
	vec3_t		dir;
	float		current_speed;
	float		move_speed;
	float		next_speed;
	float		remaining_distance;
	float		decel_distance;
	float		ratio;
	void		(*endfunc)(edict_t *);
	qboolean	is_blocked;
} moveinfo_t;


typedef struct
{
	void	(*aifunc)(edict_t *self, float dist);
	float	dist;
	void	(*thinkfunc)(edict_t *self);
} mframe_t;

typedef struct
{
	int			firstframe;
	int			lastframe;
	mframe_t	*frame;
	void		(*endfunc)(edict_t *self);
} mmove_t;

typedef struct
{
	mmove_t		*currentmove;
	mmove_t		*savemove;
	int			aiflags;
	int			nextframe;
	float		scale;

	void		(*stand)(edict_t *self);
	void		(*idle)(edict_t *self);
	void		(*search)(edict_t *self);
	void		(*walk)(edict_t *self);
	void		(*run)(edict_t *self);
	void		(*dodge)(edict_t *self, edict_t *other, float eta);
	void		(*attack)(edict_t *self);
	void		(*melee)(edict_t *self);
	void		(*sight)(edict_t *self, edict_t *other);
	qboolean	(*checkattack)(edict_t *self);
	void		(*jump)(edict_t *self);

	float		pausetime;
	float		attack_finished;

	vec3_t		saved_goal;
	float		search_time;
	float		trail_time;
	vec3_t		last_sighting;
	int			attack_state;
	int			lefty;
	float		idle_time;
	int			linkcount;

	int			power_armor_type;
	int			power_armor_power;

//ROGUE
	qboolean	(*blocked)(edict_t *self, float dist);
	float		last_hint_time;		// last time the monster checked for hintpaths.
	edict_t		*goal_hint;			// which hint_path we're trying to get to
	int			medicTries;
	edict_t		*badMedic1, *badMedic2;	// these medics have declared this monster "unhealable"
	edict_t		*healer;	// this is who is healing this monster
	void		(*duck)(edict_t *self, float eta);
	void		(*unduck)(edict_t *self);
	void		(*sidestep)(edict_t *self);
	//  while abort_duck would be nice, only monsters which duck but don't sidestep would use it .. only the brain
	//  not really worth it.  sidestep is an implied abort_duck
//	void		(*abort_duck)(edict_t *self);
	float		base_height;
	float		next_duck_time;
	float		duck_wait_time;
	edict_t		*last_player_enemy;
	// blindfire stuff .. the boolean says whether the monster will do it, and blind_fire_time is the timing
	// (set in the monster) of the next shot
	qboolean	blindfire;		// will the monster blindfire?
	float		blind_fire_delay;
	vec3_t		blind_fire_target;
	// used by the spawners to not spawn too much and keep track of #s of monsters spawned
	int			monster_slots;
	int			monster_used;
	edict_t		*commander;
	// powerup timers, used by widow, our friend
	float		quad_framenum;
	float		invincible_framenum;
	float		double_framenum;
	edict_t		*leader;
	edict_t		*old_leader;
//ROGUE
//Lazarus
	float		min_range;		// Monsters stop chasing enemy at this distance
	float		max_range;		// Monsters won't notice or attack targets farther than this
	float		ideal_range[2];	// Ideal low and high range from target, weapon-specific
	float		flies;			// Probability of dead monster generating flies
	float		jumpup;
	float		jumpdn;
	float		rangetime;
	int			chicken_framenum;
	int			pathdir;		// Up/down a hint_path chain flag for medic
	float		visibility;		// Ratio of visibility (it's a fog thang)

//end Lazarus
} monsterinfo_t;

// ROGUE
// this determines how long to wait after a duck to duck again.  this needs to be longer than
// the time after the monster_duck_up in all of the animation sequences
#define	DUCK_INTERVAL	0.5
// ROGUE

extern	game_locals_t	game;
extern	level_locals_t	level;
extern	game_import_t	gi;
extern	game_export_t	globals;
extern	spawn_temp_t	st;

extern	int	sm_meat_index;
extern	int	snd_fry;

extern	int	noweapon_index;
extern	int	jacket_armor_index;
extern	int	combat_armor_index;
extern	int	body_armor_index;
extern	int	shells_index;
extern	int	bullets_index;
extern	int	grenades_index;
extern	int	rockets_index;
extern	int	cells_index;
extern	int	slugs_index;
extern	int fuel_index;
extern	int	homing_index;
extern	int	rl_index;
extern	int	hml_index;

// means of death
#define MOD_UNKNOWN			0
#define MOD_BLASTER			1
#define MOD_SHOTGUN			2
#define MOD_SSHOTGUN		3
#define MOD_MACHINEGUN		4
#define MOD_CHAINGUN		5
#define MOD_GRENADE			6
#define MOD_G_SPLASH		7
#define MOD_ROCKET			8
#define MOD_R_SPLASH		9
#define MOD_HYPERBLASTER	10
#define MOD_RAILGUN			11
#define MOD_BFG_LASER		12
#define MOD_BFG_BLAST		13
#define MOD_BFG_EFFECT		14
#define MOD_HANDGRENADE		15
#define MOD_HG_SPLASH		16
#define MOD_WATER			17
#define MOD_SLIME			18
#define MOD_LAVA			19
#define MOD_CRUSH			20
#define MOD_TELEFRAG		21
#define MOD_FALLING			22
#define MOD_SUICIDE			23
#define MOD_HELD_GRENADE	24
#define MOD_EXPLOSIVE		25
#define MOD_BARREL			26
#define MOD_BOMB			27
#define MOD_EXIT			28
#define MOD_SPLASH			29
#define MOD_TARGET_LASER	30
#define MOD_TRIGGER_HURT	31
#define MOD_HIT				32
#define MOD_TARGET_BLASTER	33
#define MOD_VEHICLE         34
#define MOD_KICK            35
#define	MOD_PLASMA			36			//CW
#define MOD_FRIENDLY_FIRE	0x8000000

extern	int	meansOfDeath;

extern	edict_t			*g_edicts;

#define	FOFS(x) (intptr_t)&(((edict_t *)0)->x)
#define	STOFS(x) (intptr_t)&(((spawn_temp_t *)0)->x)
#define	LLOFS(x) (intptr_t)&(((level_locals_t *)0)->x)
#define	CLOFS(x) (intptr_t)&(((gclient_t *)0)->x)

#define random()	((rand () & 0x7fff) / ((float)0x7fff))
#define crandom()	(2.0 * (random() - 0.5))

extern	cvar_t	*maxentities;
extern	cvar_t	*deathmatch;
extern	cvar_t	*coop;
extern	cvar_t	*dmflags;
extern	cvar_t	*skill;
extern	cvar_t	*fraglimit;
extern	cvar_t	*timelimit;
extern	cvar_t	*password;
extern	cvar_t	*spectator_password;
extern	cvar_t	*needpass;
extern	cvar_t	*g_select_empty;
extern	cvar_t	*dedicated;

extern	cvar_t	*filterban;

extern	cvar_t	*sv_gravity;
extern	cvar_t	*sv_maxvelocity;

extern	cvar_t	*gun_x, *gun_y, *gun_z;
extern	cvar_t	*sv_rollspeed;
extern	cvar_t	*sv_rollangle;

extern	cvar_t	*run_pitch;
extern	cvar_t	*run_roll;
extern	cvar_t	*bob_up;
extern	cvar_t	*bob_pitch;
extern	cvar_t	*bob_roll;

extern	cvar_t	*sv_cheats;
extern	cvar_t	*maxclients;
extern	cvar_t	*maxspectators;

extern	cvar_t	*flood_msgs;
extern	cvar_t	*flood_persecond;
extern	cvar_t	*flood_waitdelay;

extern	cvar_t	*sv_maplist;

extern	cvar_t	*sv_stopspeed;		// PGM - this was a define in g_phys.c
extern	cvar_t	*sv_step_fraction;	// Knightmare- this was a define in p_view.c

extern	cvar_t	*actorchicken;
extern	cvar_t	*actorjump;
extern	cvar_t	*actorscram;
extern	cvar_t	*alert_sounds;
extern	cvar_t	*allow_download;
extern  cvar_t	*allow_fog;       // Set to 0 for no fog
extern	cvar_t	*bounce_bounce;
extern	cvar_t	*bounce_minv;
extern	cvar_t	*cd_loopcount;
extern	cvar_t	*cl_gun;
extern	cvar_t	*cl_thirdperson; // Knightmare added
extern	cvar_t	*corpse_fade;
extern	cvar_t	*corpse_fadetime;
extern	cvar_t	*crosshair;
extern	cvar_t	*crossh;
extern	cvar_t	*developer;
extern	cvar_t	*fmod_nomusic;
extern	cvar_t	*footstep_sounds;
extern	cvar_t	*fov;
extern	cvar_t	*gl_clear;
extern  cvar_t  *gl_driver;
extern	cvar_t	*gl_driver_fog;   // Name of dll to load for Default OpenGL mode
extern	cvar_t	*hand;
extern	cvar_t	*jetpack_weenie;
extern	cvar_t	*joy_pitchsensitivity;
extern	cvar_t	*joy_yawsensitivity;
extern	cvar_t	*jump_kick;
extern	cvar_t	*lazarus_cd_loop;
extern	cvar_t	*lazarus_cl_gun;
extern	cvar_t	*lazarus_crosshair;
extern	cvar_t	*lazarus_gl_clear;
extern	cvar_t	*lazarus_joyp;
extern	cvar_t	*lazarus_joyy;
extern	cvar_t	*lazarus_pitch;
extern	cvar_t	*lazarus_yaw;
extern	cvar_t	*lights;
extern	cvar_t	*lightsmin;
extern	cvar_t	*m_pitch;
extern	cvar_t	*m_yaw;
extern	cvar_t	*monsterjump;
extern	cvar_t	*packet_fmod_playback;
extern	cvar_t	*readout;
extern	cvar_t	*rocket_strafe;
extern	cvar_t	*rotate_distance;
extern	cvar_t	*s_primary;
extern	cvar_t	*shift_distance;
extern	cvar_t	*sv_maxgibs;
extern  cvar_t  *tpp;			  // third person perspective
extern	cvar_t	*tpp_auto;
extern	cvar_t	*turn_rider;
extern	cvar_t	*vid_ref;
extern	cvar_t	*zoomrate;
extern	cvar_t	*zoomsnap;

extern	int		max_modelindex;
extern	int		max_soundindex;


#define world	(&g_edicts[0])

// item spawnflags
#define ITEM_TRIGGER_SPAWN		0x00000001
#define ITEM_NO_TOUCH			0x00000002
// 6 bits reserved for editor flags
// 8 bits used as power cube id bits for coop games
#define DROPPED_ITEM			0x00010000
#define	DROPPED_PLAYER_ITEM		0x00020000
#define ITEM_TARGETS_USED		0x00040000


//CW++
//func_force_wall spawnflags
#define FWALL_START_ON		1
#define FWALL_DOUBLE		2
#define FWALL_REFLECT		4
#define FWALL_IMPACTSOUND	8
//CW--


//
// fields are needed for spawning from the entity string
// and saving / loading games
//
#define FFL_SPAWNTEMP		1
#define FFL_NOSPAWN			2

typedef enum {
	F_INT, 
	F_FLOAT,
	F_LSTRING,			// string on disk, pointer in memory, TAG_LEVEL
	F_GSTRING,			// string on disk, pointer in memory, TAG_GAME
	F_VECTOR,
	F_ANGLEHACK,
	F_EDICT,			// index on disk, pointer in memory
	F_ITEM,				// index on disk, pointer in memory
	F_CLIENT,			// index on disk, pointer in memory
	F_FUNCTION,
	F_MMOVE,
	F_IGNORE
} fieldtype_t;

typedef struct
{
	char	*name;
	int		ofs;
	fieldtype_t	type;
	int		flags;
} field_t;

typedef struct
{
	char	*name;
	void	(*spawn)(edict_t *ent);
} spawn_t;

// Lazarus: worldspawn effects
#define FX_WORLDSPAWN_NOHELP       1
#define FX_WORLDSPAWN_STEPSOUNDS   2
#define FX_WORLDSPAWN_WHATSIT      4
#define FX_WORLDSPAWN_ALERTSOUNDS  8
#define FX_WORLDSPAWN_CORPSEFADE  16
#define FX_WORLDSPAWN_JUMPKICK    32


extern	field_t fields[];
extern	gitem_t	itemlist[];
extern	spawn_t	spawns[];

//
// g_ai.c
//
void AI_SetSightClient (void);
void ai_stand (edict_t *self, float dist);
void ai_move (edict_t *self, float dist);
void ai_strafe(edict_t *self, float dist);	//CW
void ai_walk (edict_t *self, float dist);
void ai_turn (edict_t *self, float dist);
void ai_run (edict_t *self, float dist);
void ai_charge (edict_t *self, float dist);
qboolean canReach (edict_t *ent, edict_t *other);
qboolean FacingIdeal(edict_t *self);
qboolean FindTarget (edict_t *self);
void FoundTarget (edict_t *self);
void HuntTarget (edict_t *self);
qboolean infront (edict_t *self, edict_t *other);
int range (edict_t *self, edict_t *other);
qboolean visible (edict_t *self, edict_t *other);
qboolean ai_chicken (edict_t *ent, edict_t *badguy);

//
// km_cvar.c
//
void InitLithiumVars (void);	// init lithium cvars

//
// g_camera.c
//
void use_camera(edict_t *ent, edict_t *other, edict_t *activator);
void camera_on(edict_t *ent);
void camera_off(edict_t *ent);
void faker_animate(edict_t *self);
edict_t *G_FindNextCamera (edict_t *camera, edict_t *monitor);
edict_t *G_FindPrevCamera (edict_t *camera, edict_t *monitor);

//
// g_chase.c
//
void UpdateChaseCam(edict_t *ent);
void ChaseNext(edict_t *ent);
void ChasePrev(edict_t *ent);
void GetChaseTarget(edict_t *ent);
//
// g_combat.c
//
qboolean OnSameTeam (edict_t *ent1, edict_t *ent2);
qboolean CanDamage (edict_t *targ, edict_t *inflictor);
void T_Damage (edict_t *targ, edict_t *inflictor, edict_t *attacker, vec3_t dir, vec3_t point, vec3_t normal, int damage, int knockback, int dflags, int mod);
void T_RadiusDamage (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod, double dmg_slope);
void Killed (edict_t *targ, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);
//ROGUE
//void T_RadiusNukeDamage (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod);
//void T_RadiusClassDamage (edict_t *inflictor, edict_t *attacker, float damage, char *ignoreClass, float radius, int mod);
void cleanupHealTarget (edict_t *ent);
//ROGUE

// damage flags
#define DAMAGE_RADIUS			0x00000001	// damage was indirect
#define DAMAGE_NO_ARMOR			0x00000002	// armour does not protect from this damage
#define DAMAGE_ENERGY			0x00000004	// damage is from an energy based weapon
#define DAMAGE_NO_KNOCKBACK		0x00000008	// do not affect velocity, just view angles
#define DAMAGE_BULLET			0x00000010  // damage is from a bullet (used for ricochets)
#define DAMAGE_NO_PROTECTION	0x00000020  // armor, shields, invulnerability, and godmode have no effect

#define DEFAULT_BULLET_HSPREAD	300
#define DEFAULT_BULLET_VSPREAD	500
#define DEFAULT_SHOTGUN_HSPREAD	1000
#define DEFAULT_SHOTGUN_VSPREAD	500
#define DEFAULT_DEATHMATCH_SHOTGUN_COUNT	12
#define DEFAULT_SHOTGUN_COUNT	12
#define DEFAULT_SSHOTGUN_COUNT	20
//
// g_cmds.c
//
void Cmd_Help_f (edict_t *ent);
void Cmd_Score_f (edict_t *ent);
void Use_Flashlight(edict_t *ent,gitem_t *item);
void SetLazarusCrosshair (edict_t *ent);
void SetSensitivities(edict_t *ent,qboolean reset);
void ShiftItem(edict_t *ent, int direction);
//
// g_crane.c
//
void G_FindCraneParts(void);
void crane_control_action(edict_t *crane, edict_t *activator, vec3_t point);
void Moving_Speaker_Think(edict_t *ent);
//
// g_fog.c
//
#define MAX_FOGS 16
extern fog_t gfogs[MAX_FOGS];
void Cmd_Fog_f(edict_t *ent);
void Fog_Init(void);
void Fog(vec3_t viewpoint);
void Fog_Off(void);
void Fog_SetFogParms(void);
//
// g_func.c
//
#define TRAIN_START_ON		   1
#define TRAIN_TOGGLE		   2
#define TRAIN_BLOCK_STOPS	   4
#define TRAIN_ROTATE           8
#define TRAIN_ROTATE_CONSTANT 16
#define TRAIN_ROTATE_MASK     (TRAIN_ROTATE | TRAIN_ROTATE_CONSTANT)
#define TRAIN_ANIMATE         32
#define TRAIN_ANIMATE_FAST    64
#define TRAIN_SMOOTH         128
#define TRAIN_SPLINE        4096

qboolean box_walkmove (edict_t *ent, float yaw, float dist);
void button_use (edict_t *self, edict_t *other, edict_t *activator);
void trainbutton_use (edict_t *self, edict_t *other, edict_t *activator);
void movewith_init (edict_t *self);
void set_child_movement(edict_t *self);
//
// g_items.c
//
void PrecacheItem (gitem_t *it);
void InitItems (void);
void SetItemNames (void);
void SetAmmoPickupValues (void);
gitem_t	*FindItem (char *pickup_name);
gitem_t	*FindItemByClassname (char *classname);
#define	ITEM_INDEX(x) ((x)-itemlist)
edict_t *Drop_Item (edict_t *ent, gitem_t *item);
void SetRespawn (edict_t *ent, float delay);
void ChangeWeapon (edict_t *ent);
void SpawnItem (edict_t *ent, gitem_t *item);
void Think_Weapon (edict_t *ent);
int ArmorIndex (edict_t *ent);
int PowerArmorType (edict_t *ent);
gitem_t	*GetItemByIndex (int index);
int GetMaxAmmoByIndex (gclient_t *client, int item_index); // Knightmare added
int GetMaxArmorByIndex (int item_index); // Knightmare added
qboolean Add_Ammo (edict_t *ent, gitem_t *item, int count);
void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);

#ifdef JETPACK_MOD
void Use_Jet (edict_t *ent, gitem_t *item);

//
// g_jetpack.c
//
void Jet_ApplyJet( edict_t *ent, usercmd_t *ucmd );
qboolean Jet_AvoidGround( edict_t *ent );
void Jet_BecomeExplosion( edict_t *ent, int damage );
#endif
//
// g_lights.c
//
void Lights(void);
void ToggleLights(void);
//
// g_lock.c
//
void lock_digit_increment (edict_t *digit, edict_t *activator);
//
// g_main.c
//
void SaveClientData (void);
void FetchClientEntData (edict_t *ent);
//
// g_misc.c
//
void ThrowHead (edict_t *self, char *gibname, int damage, int type);
void ThrowClientHead (edict_t *self, int damage);
void ThrowGib (edict_t *self, char *gibname, int damage, int type);
void BecomeExplosion1(edict_t *self);
void barrel_delay (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);
void barrel_explode (edict_t *self);
void func_explosive_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);
void PrecacheDebris (int style);

//
// g_monster.c
//
#define SF_MONSTER_AMBUSH          1
#define SF_MONSTER_TRIGGER_SPAWN   2
#define SF_MONSTER_SIGHT           4
#define SF_MONSTER_GOODGUY         8
#define SF_MONSTER_NOGIB          16
#define SF_MONSTER_SPECIAL        32
#define SF_MONSTER_NOHINT		  64	//CW: if set, don't look for path_hints
#define SF_ACTOR_BAD_GUY          64
#define SF_MONSTER_FLIES         128	// only used for monster_commander_body
#define SF_MONSTER_IGNORESHOTS   128

void FadeSink (edict_t *ent);
void FadeDieSink (edict_t *ent);
void monster_fire_bullet (edict_t *self, vec3_t start, vec3_t dir, int damage, int kick, int hspread, int vspread, int flashtype);
void monster_fire_shotgun (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int flashtype);
void monster_fire_blaster (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype, int effect, int color);
void monster_fire_plasma (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype, int effect);	//CW
void monster_fire_grenade (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int flashtype);
void monster_fire_rocket (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype, edict_t *homing_target);
void monster_fire_railgun (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int flashtype);
void monster_fire_bfg (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int kick, float damage_radius, int flashtype);
void HintTestNext (edict_t *self, edict_t *hint);
int  HintTestStart (edict_t *self);
void M_droptofloor (edict_t *ent);
void monster_think (edict_t *self);
void walkmonster_start (edict_t *self);
void swimmonster_start (edict_t *self);
void flymonster_start (edict_t *self);
void AttackFinished (edict_t *self, float time);
void monster_death_use (edict_t *self);
void M_CatagorizePosition (edict_t *ent);
qboolean M_CheckAttack (edict_t *self);
void M_FlyCheck (edict_t *self);
void M_FliesOff (edict_t *self);
void M_FliesOn (edict_t *self);
void M_CheckGround (edict_t *ent);
qboolean M_SetDeath (edict_t *ent,mmove_t **moves);
int  PatchMonsterModel (char *model);

//
// g_patchplayermodels.c
//
int PatchPlayerModels (char *modelname);
//
// g_phys.c
//
void SV_AddGravity (edict_t *ent);
void G_RunEntity (edict_t *ent);
//
// g_reflect.c
//
void AddReflection (edict_t *ent);
void DeleteReflection (edict_t *ent, int index);
void ReflectExplosion (int type, vec3_t origin);
void ReflectSparks (int type, vec3_t origin, vec3_t movedir);
void ReflectSteam (vec3_t origin,vec3_t movedir,int count,int sounds,int speed, int wait, int nextid);
void ReflectTrail (int type, vec3_t start, vec3_t end);
//
// g_sound.c (interface to FMOD)
//
qboolean FMOD_IsPlaying(edict_t *ent);
void FMOD_Shutdown(void);
void FMOD_Stop(void);
void FMOD_StopSound(edict_t *ent, qboolean free);
int FMOD_PlaySound(edict_t *ent);
void FMOD_UpdateListenerPos(void);
void FMOD_UpdateSpeakerPos(edict_t *speaker);
qboolean FMOD_Init(void);
//Knightmare- this is now handled client-side
#ifdef FMOD_FOOTSTEPS
void FootStep(edict_t *ent);
void PlayFootstep(edict_t *ent, footstep_t index);
extern qboolean qFMOD_Footsteps;
#endif
void target_playback_delayed_restart (edict_t *ent);
void target_playback_delayed_start (edict_t *ent);
//
// g_spawn.c
//
void ED_CallSpawn (edict_t *ent);
void G_FindTeams(void);
void Cmd_ToggleHud (void);
void Hud_On(void);
void Hud_Off(void);
//
// g_svcmds.c
//
void	ServerCommand (void);
qboolean SV_FilterPacket (char *from);
//
// g_thing.c
//
edict_t *SpawnThing(void);
//
// g_tracktrain.c
//
void tracktrain_disengage (edict_t *train);
//
// g_turret.c
//
void turret_breach_fire(edict_t *ent);
void turret_disengage (edict_t *ent);
//
// g_trigger.c
//
typedef struct
{
	char	*name;
} entlist_t;
qboolean HasSpawnFunction(edict_t *ent);
int trigger_transition_ents (edict_t *changelevel, edict_t *self);
//
// g_utils.c
//
qboolean	KillBox (edict_t *ent);
void	G_ProjectSource (vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);
edict_t *G_Find (edict_t *from, int fieldofs, char *match);
edict_t *findradius (edict_t *from, vec3_t org, float rad);
edict_t *G_PickTarget (char *targetname);
void	G_UseTargets (edict_t *ent, edict_t *activator);
void	G_SetMovedir (vec3_t angles, vec3_t movedir);
void	G_InitEdict (edict_t *e);
edict_t	*G_Spawn (void);
void	G_FreeEdict (edict_t *e);
void	G_TouchTriggers (edict_t *ent);
void	G_TouchSolids (edict_t *ent);
char	*G_CopyString (char *in);
void	stuffcmd(edict_t *ent,char *command);
float	*tv (float x, float y, float z);
char	*vtos (vec3_t v);
char	*vtosf (vec3_t v);		//CW++
float vectoyaw (vec3_t vec);
void vectoangles (vec3_t vec, vec3_t angles);
qboolean point_infront (edict_t *self, vec3_t point);
void AnglesNormalize(vec3_t vec);
float SnapToEights(float x);
// Lazarus
float AtLeast(float x, float dx);
edict_t	*LookingAt(edict_t *ent, int filter, vec3_t endpos, float *range);
void	GameDirRelativePath(char *filename, char *output);
void	G_UseTarget (edict_t *ent, edict_t *activator, edict_t *target);
//ROGUE
void	G_ProjectSource2 (vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t up, vec3_t result);
float	vectoyaw2 (vec3_t vec);
void	vectoangles2 (vec3_t vec, vec3_t angles);
edict_t *findradius2 (edict_t *from, vec3_t org, float rad);
//ROGUE

//
// g_weapon.c
//
#define BLASTER_ORANGE	1
#define BLASTER_GREEN	2
#define BLASTER_BLUE	3
#define BLASTER_RED		4
void ThrowDebris (edict_t *self, char *modelname, float speed, vec3_t origin, int skin, int effects);
qboolean fire_hit (edict_t *self, vec3_t aim, int damage, int kick);
void fire_bullet (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int mod);
void fire_shotgun (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int mod);
void fire_blaster (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int effect, qboolean hyper, int color);
void fire_plasma (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int effect);	//CW
void fire_grenade (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius, qboolean contact);
void fire_grenade2 (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius, qboolean held);
void fire_rocket (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage, edict_t *home_target);
void fire_rail (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick);
void fire_bfg (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius);
qboolean AimGrenade (edict_t *launcher, vec3_t start, vec3_t target, vec_t speed, vec3_t aim);
void Grenade_Evade (edict_t *monster);
//
// m_actor.c
//
void actor_attack (edict_t *actor);
void actor_files (void);
void actor_fire (edict_t *actor);
void actor_jump (edict_t *actor);
void actor_moveit (edict_t *player, edict_t *actor);
void actor_run (edict_t *actor);
void actor_run_back (edict_t *actor);
void actor_salute (edict_t *actor);
void actor_stand (edict_t *actor);
void actor_walk (edict_t *actor);
void actor_walk_back (edict_t *actor);
extern mmove_t actor_move_crouch;
extern mmove_t actor_move_crouchwalk;
extern mmove_t actor_move_crouchwalk_back;
extern mmove_t	actor_move_run;
extern mmove_t	actor_move_run_back;
extern mmove_t	actor_move_run_bad;
extern mmove_t actor_move_stand;
extern mmove_t actor_move_walk;
extern mmove_t	actor_move_walk_back;
//
// m_medic.c
//
#define	MEDIC_MIN_DISTANCE	32
#define MEDIC_MAX_HEAL_DISTANCE	400
#define	MEDIC_TRY_TIME		10.0

//CW++
// m_chick.c
void chick_dodge(edict_t *self, edict_t *attacker, float eta);

// m_infantry.c
void infantry_dodge(edict_t *self, edict_t *attacker, float eta);
//CW--

void abortHeal (edict_t *ent,qboolean mark);
void medic_NextPatrolPoint(edict_t *ent,edict_t *hintpath);
edict_t *medic_FindDeadMonster (edict_t *ent);
void medic_StopPatrolling(edict_t *ent);
//
// m_move.c
//
qboolean M_CheckBottom (edict_t *ent);
qboolean M_walkmove (edict_t *ent, float yaw, float dist);
void M_MoveToGoal (edict_t *ent, float dist);
void M_ChangeYaw (edict_t *ent);
// tpp
//
// p_chase.c
//
#define OPTION_OFF        0
#define OPTION_BACKGROUND 1
void CheckChasecam_Viewent (edict_t *ent);
void Cmd_Chasecam_Toggle (edict_t *ent);
void ChasecamRemove (edict_t *ent, int opt);
void ChasecamStart (edict_t *ent);
// end tpp
//
// p_client.c
//
void player_pain (edict_t *self, edict_t *other, float kick, int damage);
void player_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);
void respawn (edict_t *ent);
void BeginIntermission (edict_t *targ);
void PutClientInServer (edict_t *ent);
void InitClientPersistant (gclient_t *client,int style);
void InitClientResp (gclient_t *client);
void InitBodyQue (void);
void ClientBeginServerFrame (edict_t *ent);
//
// p_hud.c
//
void MoveClientToIntermission (edict_t *client);
void G_SetStats (edict_t *ent);
void G_SetSpectatorStats (edict_t *ent);
void G_CheckChaseStats (edict_t *ent);
void ValidateSelectedItem (edict_t *ent);
void DeathmatchScoreboardMessage (edict_t *client, edict_t *killer);
//
// p_text.c
//
void Do_Text_Display(edict_t *activator, int flags, char *message);
//
// p_trail.c
//
void PlayerTrail_Init (void);
void PlayerTrail_Add (vec3_t spot);
void PlayerTrail_New (vec3_t spot);
edict_t *PlayerTrail_PickFirst (edict_t *self);
edict_t *PlayerTrail_PickNext (edict_t *self);
edict_t	*PlayerTrail_LastSpot (void);
//
// p_view.c
//
#define FALL_HURTBAD		55				//CWL was harcoded as 55 in P_FallingDamage()
#define MAX_SAFE_FALLDIST	30				//CW: was hardcoded as 30
#define FALLDAMAGE_FACTOR	1				//CW: was hardcoded as 0.5
void ClientEndServerFrame (edict_t *ent);
//
// p_weapon.c
//
void PlayerNoise(edict_t *who, vec3_t where, int type);
void P_ProjectSource (gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);
void kick_attack (edict_t *ent);

// ROGUE
//
// g_newai.c
//
#define	MAX_HINT_CHAINS		100
extern int	hint_paths_present;
extern edict_t *hint_path_start[MAX_HINT_CHAINS];
extern int	num_hint_paths;

qboolean blocked_checkshot (edict_t *self, float shotChance);
qboolean blocked_checkplat (edict_t *self, float dist);
qboolean blocked_checkjump (edict_t *self, float dist, float maxDown, float maxUp);
qboolean blocked_checknewenemy (edict_t *self);
qboolean monsterlost_checkhint (edict_t *self);
qboolean inback (edict_t *self, edict_t *other);
float realrange (edict_t *self, edict_t *other);
edict_t *SpawnBadArea(vec3_t mins, vec3_t maxs, float lifespan, edict_t *owner);
edict_t *CheckForBadArea(edict_t *ent);
void InitHintPaths (void);
void PredictAim (edict_t *target, vec3_t start, float bolt_speed, qboolean eye_height, float offset, vec3_t aimdir, vec3_t aimpoint);
qboolean below (edict_t *self, edict_t *other);
void drawbbox (edict_t *self);
qboolean has_valid_enemy (edict_t *self);
void hintpath_stop (edict_t *self);
edict_t * PickCoopTarget (edict_t *self);
int CountPlayers (void);
void monster_jump_start (edict_t *self);
qboolean monster_jump_finished (edict_t *self);
// END ROGUE

//============================================================================

// client_t->anim_priority
#define	ANIM_BASIC		0		// stand / run
#define	ANIM_WAVE		1
#define	ANIM_JUMP		2
#define	ANIM_PAIN		3
#define	ANIM_ATTACK		4
#define	ANIM_DEATH		5
#define	ANIM_REVERSE	6


// client data that stays across multiple level loads
typedef struct
{
	char		userinfo[MAX_INFO_STRING];
	char		netname[16];
	int			hand;

	qboolean	connected;			// a loadgame will leave valid entities that
									// just don't have a connection yet

	// values saved and restored from edicts when changing levels
	int			health;
	int			max_health;
	int			savedFlags;

	int			selected_item;
	int			inventory[MAX_ITEMS];

	// ammo capacities
	int			max_bullets;
	int			max_shells;
	int			max_rockets;
	int			max_grenades;
	int			max_cells;
	int			max_slugs;
	int			max_fuel;
	int			max_homing_rockets;

	gitem_t		*weapon;
	gitem_t		*lastweapon;

	qboolean    fire_mode;              // Lazarus - alternate firing mode

	int			power_cubes;	// used for tracking the cubes in coop games
	int			score;			// for calculating total unit score in coop games

	int			game_helpchanged;
	int			helpchanged;

	qboolean	spectator;			// client is a spectator
	// tpp
	int			chasetoggle;
	// end tpp
	qboolean	spawn_landmark;
	qboolean	spawn_levelchange;
	vec3_t		spawn_offset;
	vec3_t		spawn_velocity;
	vec3_t		spawn_angles;
	vec3_t		spawn_viewangles;
	int			spawn_pm_flags;
	int			spawn_gunframe;
	int			spawn_modelframe;
	int			spawn_anim_end;
	gitem_t		*newweapon;
} client_persistant_t;

// client data that stays across deathmatch respawns
typedef struct
{
	client_persistant_t	coop_respawn;	// what to set client->pers to on a respawn
	int			enterframe;			// level.framenum the client entered the game
	int			score;				// frags, etc
	vec3_t		cmd_angles;			// angles sent over in the last command

	qboolean	spectator;			// client is a spectator
	// tpp
	int			chasetoggle;
	// end tpp
} client_respawn_t;

// this structure is cleared on each PutClientInServer(),
// except for 'client->pers'
struct gclient_s
{
	// known to server
	player_state_t	ps;				// communicated by server to clients
	int				ping;

	// private to game
	client_persistant_t	pers;
	client_respawn_t	resp;
	pmove_state_t		old_pmove;	// for detecting out-of-pmove changes

	qboolean	showscores;			// set layout stat
	qboolean	showinventory;		// set layout stat
	qboolean	showhelp;
	qboolean	showhelpicon;

	int			ammo_index;

	int			buttons;
	int			oldbuttons;
	int			latched_buttons;

	int			nNewLatch;

	qboolean	weapon_thunk;

	gitem_t		*newweapon;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int			damage_armor;		// damage absorbed by armor
	int			damage_parmor;		// damage absorbed by power armor
	int			damage_blood;		// damage taken out of health
	int			damage_knockback;	// impact damage
	vec3_t		damage_from;		// origin for vector calculation

	float		killer_yaw;			// when dead, look at killer

	weaponstate_t	weaponstate;
	vec3_t		kick_angles;	// weapon kicks
	vec3_t		kick_origin;
	float		v_dmg_roll, v_dmg_pitch, v_dmg_time;	// damage kicks
	float		fall_time, fall_value;		// for view drop on fall
	float		damage_alpha;
	float		bonus_alpha;
	vec3_t		damage_blend;
	vec3_t		v_angle;			// aiming direction
	float		bobtime;			// so off-ground doesn't change it
	vec3_t		oldviewangles;
	vec3_t		oldvelocity;

	float		next_drown_time;
	int			old_waterlevel;
	int			breather_sound;

	int			machinegun_shots;	// for weapon raising

	qboolean    backpedaling;  // <- CDawg added this

	// animation vars
	int			anim_end;
	int			anim_priority;
	qboolean	anim_duck;
	qboolean	anim_run;

	// powerup timers
	float		quad_framenum;
	float		invincible_framenum;
	float		breather_framenum;
	float		enviro_framenum;

	qboolean	grenade_blew_up;
	float		grenade_time;
	int			silencer_shots;
	int			weapon_sound;

	float		pickup_msg_time;

	float		flood_locktill;		// locked from talking
	float		flood_when[10];		// when messages were said
	int			flood_whenhead;		// head pointer for when said

	float		respawn_time;		// can respawn when time > this

	edict_t		*chase_target;		// player we are chasing
	qboolean	update_chase;		// need to update chase info?

	usercmd_t	ucmd;				// Lazarus: Copied for convenience in ClientThink

	// tpp
	int			chasetoggle;
	edict_t		*chasecam;
	edict_t		*oldplayer;
	int			use;				// indicates whether +use key is pressed
	int			zoom;
	int			delayedstart;
	// end tpp

	// TREMOR func_pushable stuff
	float       maxvelocity;        // Used when pushing func_pushable
	edict_t     *push;

	// flashlight
	qboolean    flashlight;
	float       flashlight_time;

	// menu stuff ala CTF
	qboolean	inmenu;				// in menu
	int         menutimer;
	pmenuhnd_t	*menu;				// current menu
	texthnd_t	*textdisplay;		// currently displayed text
	char		*whatsit;

	// security camera
	edict_t     *spycam;
	edict_t		*monitor;
	edict_t		*camplayer;
	vec3_t		org_viewangles;
	short		old_owner_angles[2];

	// laser sight
	edict_t		*laser_sight;

	int			vehicle_framenum;	// last time player engaged or disengaged vehicle
	int			zooming;
	float		joy_pitchsensitivity;
	float		joy_yawsensitivity;
	float		m_pitch;
	float		m_yaw;
	qboolean	sensitivities_init;
	qboolean	zoomed;
	float		original_fov;
	float		fps_time_start;
	int			fps_frames;
	float		secs_per_frame;
	float		frame_zoomrate;

	int			shift_dir;			// direction code for debugging/moving an item
	int			startframe;			// time at which ClientBegin is called

	float		fadestart;
	float		fadein;				// for fading screen to black at mission failure
	float		fadehold;
	float		fadeout;
	vec3_t		fadecolor;
	float		fadealpha;

	int			leftfoot;			// 0 or 1, used for footstep sounds
	int			jumping;			// 0 or 1, used for jumpkick

	edict_t		*homing_rocket;		// used to limit firing frequency

	qboolean	in_func_air;		//CW: inside a func_air
	qboolean	entered_func_air;	//CW: just entered a func_air
	int			func_air_frame;		//CW: time at which func_air was entered
	qboolean	airy_sound;			//CW: alternate between func_air breathing sounds

#ifdef JETPACK_MOD
	qboolean	jetpack;
	float		jetpack_framenum;
	float		jetpack_nextthink;
	qboolean	jetpack_thrusting;
	qboolean	jetpack_infinite;
	float		jetpack_start_thrust;
	float		jetpack_last_thrust;
	float		jetpack_activation;
	float		jetpack_roll;
#endif
};

#define NUM_ACTOR_SOUNDS   13

struct edict_s
{
	entity_state_t	s;
	struct gclient_s	*client;	// NULL if not a player
									// the server expects the first part
									// of gclient_s to be a player_state_t
									// but the rest of it is opaque

	qboolean	inuse;
	int			linkcount;

	// FIXME: move these fields to a server private sv_entity_t
	link_t		area;				// linked to a division node or leaf
	
	int			num_clusters;		// if -1, use headnode instead
	int			clusternums[MAX_ENT_CLUSTERS];
	int			headnode;			// unused if num_clusters != -1
	int			areanum, areanum2;

	//================================

	int			svflags;
	vec3_t		mins, maxs;
	vec3_t		absmin, absmax, size;
	solid_t		solid;
	int			clipmask;
	edict_t		*owner;


	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!

	//================================
	int			movetype;
	int			flags;

	char		*model;
	float		freetime;			// sv.time when the object was freed
	
	//
	// only used locally in game, not by server
	//
	char		*message;
	char        *key_message;   // Lazarus: used from tremor_trigger_key
	char		*classname;
	int			spawnflags;

	float		timestamp;

	float		angle;			// set in qe3, -1 = up, -2 = down
	char		*target;
	char		*targetname;
	char		*killtarget;
	char		*team;
	char		*pathtarget;
	char		*deathtarget;
	char		*combattarget;
	edict_t		*target_ent;

	float		speed, accel, decel;
	vec3_t		movedir;
	vec3_t		pos1, pos2;

	vec3_t		velocity;
	vec3_t		avelocity;
	int			mass;
	float		air_finished;
	float		gravity;		// per entity gravity multiplier (1.0 is normal)
								// use for lowgrav artifact, flares

	edict_t		*goalentity;
	edict_t		*movetarget;
	float		yaw_speed;
	float		ideal_yaw;

	char		*common_name;

	// Lazarus: for rotating brush models:
	float		pitch_speed;
	float		roll_speed;
	float		ideal_pitch;
	float		ideal_roll;
	float		roll;

	float		nextthink;
	void		(*prethink) (edict_t *ent);
	void		(*think)(edict_t *self);
	void		(*blocked)(edict_t *self, edict_t *other);	//move to moveinfo?
	void		(*touch)(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
	void		(*use)(edict_t *self, edict_t *other, edict_t *activator);
	void		(*pain)(edict_t *self, edict_t *other, float kick, int damage);
	void		(*die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

	void		(*play)(edict_t *self, edict_t *activator);

	float		touch_debounce_time;		// are all these legit?  do we need more/less of them?
	float		pain_debounce_time;
	float		damage_debounce_time;
	float		gravity_debounce_time;		// used by item_ movement commands to prevent
											// monsters from dropping to floor
	float		fly_sound_debounce_time;	//move to clientinfo
	float		last_move_time;

	int			health;
	int			max_health;
	int			gib_health;
	int			deadflag;
	qboolean	show_hostile;

	// Lazarus: health2 and mass2 are passed from jorg to makron health and mass
	int			health2;
	int			mass2;

	float		powerarmor_time;

	char		*map;			// target_changelevel

	int			viewheight;		// height above origin where eyesight is determined
	int			takedamage;
	int			dmg;
	int			radius_dmg;
	float		dmg_radius;
	int			sounds;			//make this a spawntemp var?
	int			count;

	edict_t		*chain;
	edict_t		*enemy;
	edict_t		*oldenemy;
	edict_t		*activator;
	edict_t		*groundentity;
	int			groundentity_linkcount;
	edict_t		*teamchain;
	edict_t		*teammaster;

	edict_t		*mynoise;		// can go in client only
	edict_t		*mynoise2;

	int			noise_index;
	int			noise_index2;
	float		volume;
	float		attenuation;

	// timing variables
	float		wait;
	float		delay;			// before firing targets
	float		random;
	// Lazarus: laser timing
	float		starttime;
	float		endtime;

	float		teleport_time;

	int			watertype;
	int			waterlevel;
	int			old_watertype;

	vec3_t		move_origin;
	vec3_t		move_angles;

	// move this to clientinfo?
	int			light_level;

	int			style;			// also used as areaportal number

	gitem_t		*item;			// for bonus items

	// common data blocks
	moveinfo_t		moveinfo;
	monsterinfo_t	monsterinfo;

	float		goal_frame;
	
	// various Lazarus additions follow:

	edict_t		*turret;		// player-controlled turret
	edict_t		*child;			// "real" infantry guy, child of remote turret_driver
	vec_t		base_radius;	// Lazarus: used to project "viewpoint" of TRACK turret
								// out past base

	//ed - for the sprite/model spawner
	char		*usermodel;
	int			startframe;
	int			framenumbers;
	int			solidstate;
	// Lazarus: changed from rendereffect to renderfx and effects, and now uses
	//          real constants which can be combined.
//	int			rendereffect;
	int			renderfx;
	int         effects;
	vec3_t		bleft;
	vec3_t		tright;

	// tpp
	int			chasedist1;
    int			chasedist2;
    edict_t		*crosshair;
	// end tpp

	// item identification
	char		*datafile;

	// func_pushable
	vec3_t      oldvelocity;    // Added for TREMOR to figure falling damage
	vec3_t      offset;         // Added for TREMOR - offset from func_pushable to pusher
	float       density;
	float		bob;            // bobbing in water amplitude
	float		duration;
	int			bobframe;
	int			bounce_me;		// 0 for no bounce, 1 to bounce, 2 if velocity should not be clipped
								// this is solely used by func_pushable for now
	vec3_t      origin_offset;  // used to help locate brush models w/o origin brush
	vec3_t		org_mins,org_maxs;
	vec3_t		org_angles;
	int			org_movetype;
	int			axis;

	// crane
	qboolean    busy;
	qboolean	attracted;
	int         crane_increment;
	int         crane_dir;
	edict_t     *crane_control;
	edict_t     *crane_onboard_control;
	edict_t     *crane_beam;
	edict_t     *crane_hoist;
	edict_t     *crane_hook;
	edict_t     *crane_cargo;
	edict_t		*crane_cable;
	edict_t     *crane_light;
	vec_t       crane_bonk;

	edict_t     *speaker;       // moving speaker that eliminates the need
	                            // for origin brushes with brush models
	edict_t     *vehicle;       // generic drivable vehicle
	char		*idle_noise;
	float		radius;
	vec3_t		org_size;		// Initial size of the vehicle bounding box,

	vec3_t		fog_color;
	int			fog_model;
	float		fog_near;
	float		fog_far;
	float		fog_density;
	int			fog_index;
	int			fogclip;		// only used by worldspawn to indicate whether gl_clear
								// should be forced to a good value for fog obscuration
								// of HOM

	edict_t		*movewith_next;
	char		*movewith;
	edict_t		*movewith_ent;
	vec3_t		movewith_offset;
	vec3_t		parent_attach_angles;
	qboolean	do_not_rotate;

	// monster AI
	char		*dmgteam;

	// turret
	char		*destroytarget;
	char		*viewmessage;
	char		*followtarget;

	// spycam
	edict_t		*viewer;

	// monster power armor
	int			powerarmor;

	// MOVETYPE_PUSH rider angles
	int			turn_rider;

	// selected brush models will move their origin to 
	// the origin of this entity:
	char		*move_to;

	// newtargetname used ONLY by target_change and target_bmodel_spawner.
	//CW ...and trigger_relay, trigger_multiple, func_button and func_door toggles
	char		*newtargetname;

	// source of target_clone's model
	char		*source;

	char		*musictrack;	// Knightmare- for specifying OGG or CD track

	// if true, brush models will move directly to Move_Done
	// at Move_Final rather than slowing down.
	qboolean	smooth_movement;

	int			in_mud;

	int			actor_sound_index[NUM_ACTOR_SOUNDS];
	int			actor_gunframe;
	int			actor_current_weapon;		// Index into weapon[]
	int			actor_weapon[2];
	int			actor_model_index[2];
	float		actor_crouch_time;
	qboolean	actor_id_model;
	vec3_t		muzzle;						// Offset from origin to gun muzzle
	vec3_t		muzzle2;					// Offset to left weapon (must have SF | 128)

	vec3_t		color;						// target_fade
	float		alpha;
	float		fadein;
	float		holdtime;
	float		fadeout;

	int			owner_id;					// These are used ONLY for ents that
	int			id;							// change maps via trigger_transition
	int			last_attacked_framenum;		// Used to turn off chicken mode

	// tracktrain
	char		*target2;
	edict_t		*prevpath;

	// spline train
	edict_t		*from;
	edict_t		*to;

	edict_t		*next_grenade;				// Used to build a list of active grenades
	edict_t		*prev_grenade;

	// FMOD
	int			*stream;	// Actually a FSOUND_STREAM * or FMUSIC_MODULE *
	int			channel;

	// gib type - specifies folder where gib models are found.
	int			gib_type;
	int			blood_type;

	int			moreflags;

	// actor muzzle flash
	edict_t		*flash;

	// Psychospaz reflections
	edict_t		*reflection[6];

//=========
//ROGUE
	int			plat2flags;
//	vec3_t		offset;  used by Lazarus
	vec3_t		gravityVector;
	edict_t		*bad_area;
	edict_t		*hint_chain;
	edict_t		*monster_hint_chain;
	edict_t		*target_hint_chain;
	int			hint_chain_id;
	// FIXME - debug help!
	float		lastMoveTime;
//ROGUE
//=========

};

#define	LOOKAT_NOBRUSHMODELS  1
#define LOOKAT_NOWORLD        2
#define LOOKAT_MD2			  (LOOKAT_NOBRUSHMODELS | LOOKAT_NOWORLD)

#define BeepBeep(ent) (gi.sound (ent, CHAN_AUTO, gi.soundindex ("misc/talk1.wav"), 1, ATTN_NORM, 0))

#define POWERUP_REPLACE_ENT 0
#define POWERUP_NEW_ENT     1
#define POWERUP_USE_ITEM    2

#define FLASHLIGHT_MOD
#define FLASHLIGHT_USE POWERUP_NEW_ENT
#define FLASHLIGHT_DRAIN     60
#define FLASHLIGHT_ITEM      "Cells"

