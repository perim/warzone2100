/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

/** \file
*  Definitions for the stats system.
*/
#ifndef __INCLUDED_STATSDEF_H__
#define __INCLUDED_STATSDEF_H__

#include "lib/ivis_common/ivisdef.h"

/*
if any types are added BEFORE 'COMP_BODY' - then Save/Load Game will have to be
altered since it loops through the components from 1->MAX_COMP
*/
typedef enum COMPONENT_TYPE
{
	COMP_UNKNOWN,
	COMP_BODY,
	COMP_BRAIN,
	COMP_PROPULSION,
	COMP_REPAIRUNIT,
	COMP_ECM,
	COMP_SENSOR,
	COMP_CONSTRUCT,
	COMP_WEAPON,
	COMP_NUMCOMPONENTS,			/** The number of enumerators in this enum.	 */
} COMPONENT_TYPE;

/**
 * LOC used for holding locations for Sensors and ECM's
 */
typedef enum LOC
{
	LOC_DEFAULT,
	LOC_TURRET,
} LOC;

/**
 * SIZE used for specifying body size
 */
typedef enum BODY_SIZE
{
	SIZE_LIGHT,
	SIZE_MEDIUM,
	SIZE_HEAVY,
	SIZE_SUPER_HEAVY,
} BODY_SIZE;

/**
 * only using KINETIC and HEAT for now
 */
typedef enum WEAPON_CLASS
{
	WC_KINETIC,					///< bullets etc
	//WC_EXPLOSIVE,				///< rockets etc - classed as WC_KINETIC now to save space in DROID
	WC_HEAT,					///< laser etc
	//WC_MISC					///< others we haven't thought of! - classed as WC_HEAT now to save space in DROID
	WC_NUM_WEAPON_CLASSES		/** The number of enumerators in this enum.	 */
} WEAPON_CLASS;

/**
 * weapon subclasses used to define which weapons are affected by weapon upgrade
 * functions
 *
 * Watermelon:added a new subclass to do some tests
 */
typedef enum WEAPON_SUBCLASS
{
	WSC_MGUN,
	WSC_CANNON,
	//WSC_ARTILLARY,
	WSC_MORTARS,
	WSC_MISSILE,
	WSC_ROCKET,
	WSC_ENERGY,
	WSC_GAUSS,
	WSC_FLAME,
	//WSC_CLOSECOMBAT,
	WSC_HOWITZERS,
	WSC_ELECTRONIC,
	WSC_AAGUN,
	WSC_SLOWMISSILE,
	WSC_SLOWROCKET,
	WSC_LAS_SAT,
	WSC_BOMB,
	WSC_COMMAND,
	WSC_EMP,
	WSC_COUNTER,				// Counter missile
	WSC_NUM_WEAPON_SUBCLASSES,	/** The number of enumerators in this enum.	 */
} WEAPON_SUBCLASS;

/**
 * Used to define which projectile model to use for the weapon.
 */
typedef enum MOVEMENT_MODEL
{
	MM_DIRECT,
	MM_INDIRECT,
	MM_HOMINGDIRECT,
	MM_HOMINGINDIRECT,
	MM_ERRATICDIRECT,
	MM_SWEEP,
	NUM_MOVEMENT_MODEL,			/**  The number of enumerators in this enum. */
} MOVEMENT_MODEL;

/**
 * Used to modify the damage to a propuslion type (or structure) based on
 * weapon.
 */
typedef enum WEAPON_EFFECT
{
	WE_ANTI_PERSONNEL,
	WE_ANTI_TANK,
	WE_BUNKER_BUSTER,
	WE_ARTILLERY_ROUND,
	WE_FLAMER,
	WE_ANTI_AIRCRAFT,
	WE_NUMEFFECTS,			/**  The number of enumerators in this enum. */
} WEAPON_EFFECT;

/**
 * Sides used for droid impact
 */
typedef enum HIT_SIDE
{
	HIT_SIDE_FRONT,
	HIT_SIDE_REAR,
	HIT_SIDE_LEFT,
	HIT_SIDE_RIGHT,
	HIT_SIDE_TOP,
	HIT_SIDE_BOTTOM,
	NUM_HIT_SIDES,			/**  The number of enumerators in this enum. */
} HIT_SIDE;

/**
 * Defines the left and right sides for propulsion IMDs
 */
typedef enum PROP_SIDE
{
	LEFT_PROP,
	RIGHT_PROP,
	NUM_PROP_SIDES,			/**  The number of enumerators in this enum. */
} PROP_SIDE;

typedef enum PROPULSION_TYPE
{
	PROPULSION_TYPE_WHEELED,
	PROPULSION_TYPE_TRACKED,
	PROPULSION_TYPE_LEGGED,
	PROPULSION_TYPE_HOVER,
	PROPULSION_TYPE_SKI,
	PROPULSION_TYPE_LIFT,
	PROPULSION_TYPE_PROPELLOR,
	PROPULSION_TYPE_HALF_TRACKED,
	PROPULSION_TYPE_JUMP,
	PROPULSION_TYPE_NUM,	/**  The number of enumerators in this enum. */
} PROPULSION_TYPE;

typedef enum SENSOR_TYPE
{
	STANDARD_SENSOR,
	INDIRECT_CB_SENSOR,
	VTOL_CB_SENSOR,
	VTOL_INTERCEPT_SENSOR,
	SUPER_SENSOR,			///< works as all of the above together! - new for updates
	RADAR_DETECTOR_SENSOR,
} SENSOR_TYPE;

typedef enum FIREONMOVE
{
	FOM_NO,			///< no capability - droid must stop
	FOM_PARTIAL,	///< partial capability - droid has 50% chance to hit
	FOM_YES,		///< full capability - droid fires normally on move
} FIREONMOVE;

typedef enum TRAVEL_MEDIUM
{
	GROUND,
	AIR,
} TRAVEL_MEDIUM;

/*
* Stats structures type definitions
*/

/* Elements common to all stats structures */

/* Stats common to all stats structs */
struct BASE_STATS
{
	UDWORD	ref;	/**< Unique ID of the item */
	char	*pName; /**< pointer to the text id name (i.e. short language-independant name) */
};

/* Stats common to all droid components */
struct COMPONENT_STATS : public BASE_STATS
{
	UDWORD		buildPower;			/**< Power required to build the component */
	UDWORD		buildPoints;		/**< Time required to build the component */
	UDWORD		weight;				/**< Component's weight */
	UDWORD		body;				/**< Component's body points */
	bool		designable;			/**< flag to indicate whether this component can be used in the design screen */
	iIMDShape	*pIMD;				/**< The IMD to draw for this component */
};

struct PROPULSION_STATS : public COMPONENT_STATS
{
	UDWORD			maxSpeed;		///< Max speed for the droid
	PROPULSION_TYPE propulsionType; ///< Type of propulsion used - index into PropulsionTable
};

struct SENSOR_STATS : public COMPONENT_STATS
{
	UDWORD		range;			///< Sensor range
	UDWORD		power;			///< Sensor power (put against ecm power)
	UDWORD		location;		///< specifies whether the Sensor is default or for the Turret
	SENSOR_TYPE type;			///< used for combat
	UDWORD		time;			///< time delay before associated weapon droids 'know' where the attack is from
	iIMDShape	*pMountGraphic; ///< The turret mount to use
};

struct ECM_STATS : public COMPONENT_STATS
{
	UDWORD		range;			///< ECM range
	UDWORD		power;			///< ECM power (put against sensor power)
	UDWORD		location;		///< specifies whether the ECM is default or for the Turret
	iIMDShape	*pMountGraphic; ///< The turret mount to use
};

struct REPAIR_STATS : public COMPONENT_STATS
{
	UDWORD		repairPoints;	///< How much damage is restored to Body Points and armour each Repair Cycle
	bool		repairArmour;	///< whether armour can be repaired or not
	UDWORD		location;		///< specifies whether the Repair is default or for the Turret
	UDWORD		time;			///< time delay for repair cycle
	iIMDShape	*pMountGraphic; ///< The turret mount to use
};

struct WEAPON_STATS : public COMPONENT_STATS
{
	UDWORD			shortRange;				///< Max distance to target for	short	range	shot
	UDWORD			longRange;				///< Max distance to target for	long range shot
	UDWORD			minRange;				///< Min distance to target for	shot
	UDWORD			shortHit;				///< Chance to hit at short range
	UDWORD			longHit;				///< Chance to hit at long range
	UDWORD			firePause;				///< Time between each weapon fire
	UDWORD			numExplosions;			///< The number of explosions per shot
	UBYTE			numRounds;				///< The number of rounds	per salvo(magazine)
	UDWORD			reloadTime;				///< Time to reload	the round of ammo	(salvo fire)
	UDWORD			damage;					///< How much	damage the weapon	causes
	UDWORD			radius;					///< Basic blast radius of weapon
	UDWORD			radiusHit;				///< Chance to hit in the	blast	radius
	UDWORD			radiusDamage;			///< Damage done in	the blast radius
	UDWORD			incenTime;				///< How long	the round burns
	UDWORD			incenDamage;			///< Damage done each burn cycle
	UDWORD			incenRadius;			///< Burn radius of	the round
	UDWORD			flightSpeed;			///< speed ammo travels at
	FIREONMOVE		fireOnMove;				///< indicates whether the droid has to stop before firing
	WEAPON_CLASS	weaponClass;			///< the class of weapon
	WEAPON_SUBCLASS weaponSubClass;			///< the subclass to which the weapon	belongs
	MOVEMENT_MODEL	movementModel;			///< which projectile model to use for the bullet
	WEAPON_EFFECT	weaponEffect;			///< which type of warhead is associated with the	weapon
	UDWORD			recoilValue;			///< used to compare with	weight to see if recoils or not
	UBYTE			rotate;					///< amount the weapon(turret) can rotate 0	= none
	UBYTE			maxElevation;			///< max amount the	turret can be elevated up
	SBYTE			minElevation;			///< min amount the	turret can be elevated down
	UBYTE			facePlayer;				///< flag to make the (explosion) effect face the	player when	drawn
	UBYTE			faceInFlight;			///< flag to make the inflight effect	face the player when drawn
	UBYTE			effectSize;				///< size of the effect 100 = normal,	50 = half etc
	bool			lightWorld;				///< flag to indicate whether the effect lights up the world
	UBYTE			surfaceToAir;			///< indicates how good in the air - SHOOT_ON_GROUND, SHOOT_IN_AIR or both
	UBYTE			vtolAttackRuns;			///< number of attack runs a VTOL droid can	do with this weapon
	bool			penetrate;				///< flag to indicate whether pentrate droid or not

	/* Graphics control stats */
	UDWORD			directLife;				///< How long a direct fire weapon is visible. Measured in 1/100 sec.
	UDWORD			radiusLife;				///< How long a blast radius is visible

	/* Graphics used for the weapon */
	iIMDShape		*pMountGraphic;			///< The turret mount to use
	iIMDShape		*pMuzzleGraphic;		///< The muzzle flash
	iIMDShape		*pInFlightGraphic;		///< The ammo in flight
	iIMDShape		*pTargetHitGraphic;		///< The ammo hitting a target
	iIMDShape		*pTargetMissGraphic;	///< The ammo missing a target
	iIMDShape		*pWaterHitGraphic;		///< The ammo hitting water
	iIMDShape		*pTrailGraphic;			///< The trail used for in flight

	/* Audio */
	SDWORD			iAudioFireID;
	SDWORD			iAudioImpactID;
};

struct CONSTRUCT_STATS : public COMPONENT_STATS
{
	UDWORD		constructPoints;	///< The number of points contributed each cycle
	iIMDShape	*pMountGraphic;		///< The turret mount to use
};

struct BRAIN_STATS : public COMPONENT_STATS
{
	UDWORD			progCap;		///< Program capacity
	WEAPON_STATS	*psWeaponStat;	///< weapon stats associated with this brain - for Command Droids
};


#define NULL_COMP	(-1)
/*
 * Stats structures type definitions
 */
#define SHOOT_ON_GROUND 0x01
#define SHOOT_IN_AIR	0x02

//Special angles representing top or bottom hit
#define HIT_ANGLE_TOP		361
#define HIT_ANGLE_BOTTOM	362

struct BODY_STATS : public COMPONENT_STATS
{
	UBYTE		size;			///< How big the body is - affects how hit
	UDWORD		weaponSlots;	///< The number of weapon slots on the body
	UDWORD		armourValue[NUM_HIT_SIDES][WC_NUM_WEAPON_CLASSES];	///< A measure of how much protection the armour provides. Cross referenced with the weapon types.

	// A measure of how much energy the power plant outputs
	UDWORD		powerOutput;	///< this is the engine output of the body
	iIMDShape	**ppIMDList;	///< list of IMDs to use for propulsion unit - up to numPropulsionStats
	iIMDShape	*pFlameIMD;		///< pointer to which flame graphic to use - for VTOLs only at the moment
};

/************************************************************************************
* Additional stats tables
************************************************************************************/
typedef struct _propulsion_types
{
	UWORD	powerRatioMult; ///< Multiplier for the calculated power ratio of the droid
	UDWORD	travel;			///< Which medium the propulsion travels in
	SWORD	startID;		///< sound to play when this prop type starts
	SWORD	idleID;			///< sound to play when this prop type is idle
	SWORD	moveOffID;		///< sound to link moveID and idleID
	SWORD	moveID;			///< sound to play when this prop type is moving
	SWORD	hissID;			///< sound to link moveID and idleID
	SWORD	shutDownID;		///< sound to play when this prop type shuts down
} PROPULSION_TYPES;

typedef struct _terrain_table
{
	UDWORD	speedFactor;	///< factor to multiply the speed by depending on the method of propulsion and the terrain type - to be divided by 100 before use
} TERRAIN_TABLE;

typedef struct _special_ability
{
	char	*pName;			///< Text name of the component
} SPECIAL_ABILITY;

typedef UWORD	WEAPON_MODIFIER;

/* weapon stats which can be upgraded by research*/
typedef struct _weapon_upgrade
{
	UBYTE	firePause;
	UWORD	shortHit;
	UWORD	longHit;
	UWORD	damage;
	UWORD	radiusDamage;
	UWORD	incenDamage;
	UWORD	radiusHit;
} WEAPON_UPGRADE;

/*sensor stats which can be upgraded by research*/
typedef struct _sensor_upgrade
{
	UWORD	power;
	UWORD	range;
} SENSOR_UPGRADE;

/*ECM stats which can be upgraded by research*/
typedef struct _ecm_upgrade
{
	UWORD	power;
	UDWORD	range;
} ECM_UPGRADE;

/*repair stats which can be upgraded by research*/
typedef struct _repair_upgrade
{
	UWORD	repairPoints;
} REPAIR_UPGRADE;

/*constructor stats which can be upgraded by research*/
typedef struct _constructor_upgrade
{
	UWORD	constructPoints;
} CONSTRUCTOR_UPGRADE;

/*body stats which can be upgraded by research*/
typedef struct _body_upgrade
{
	UWORD	powerOutput;
	UWORD	body;
	UWORD	armourValue[WC_NUM_WEAPON_CLASSES];
} BODY_UPGRADE;

#endif // __INCLUDED_STATSDEF_H__
