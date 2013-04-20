/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
 *  Definitions for functions.
 */

#ifndef __INCLUDED_FUNCTIONDEF_H__
#define __INCLUDED_FUNCTIONDEF_H__

#include "statsdef.h"

enum FUNCTION_TYPE
{
	REPAIR_DROID_TYPE,
	WEAPON_UPGRADE_TYPE,
	WALL_TYPE,
	DROIDREPAIR_UPGRADE_TYPE,
	DROIDCONST_UPGRADE_TYPE,

	/* The number of function types */
	NUMFUNCTIONS,
};

/*Common stats for all Structure Functions*/

/*Common struct for all functions*/
struct FUNCTION : public BASE_STATS
{
	FUNCTION_TYPE           type;                   ///< The type of Function
};


/*To repair droids that enter the repair facility*/
struct REPAIR_DROID_FUNCTION : public FUNCTION
{
	UDWORD			repairPoints;	/*The number of repair points used to reduce
									  damage to the droid. These repair points can
									  restore even destroyed droid components*/
};

/*To generate and supply power to other structures*/
struct POWER_GEN_FUNCTION : public FUNCTION
{
};

/*function used by walls to define which corner to use*/
struct WALL_FUNCTION : public FUNCTION
{
	char						*pStructName;		//storage space for the name so can work out
													//which stat when structs are loaded in
	struct STRUCTURE_STATS *                        pCornerStat;            ///< pointer to which stat to use as a corner wall
};

/*function used by Resource Extractor to indicate how much resource is available*/
struct RESOURCE_FUNCTION : public FUNCTION
{
};

/*To increase a production facilities output*/
struct PRODUCTION_UPGRADE_FUNCTION : public FUNCTION
{
};

/*To manufacture droids designed previously*/
struct PRODUCTION_FUNCTION : public FUNCTION
{
};

/*To research topics available*/
struct RESEARCH_FUNCTION : public FUNCTION
{
};

/*To rearm VTOLs*/
struct REARM_FUNCTION : public FUNCTION
{
};

struct UPGRADE_FUNCTION : public FUNCTION
{
	uint16_t                upgradePoints;  ///< The % to add to the action points
};

typedef UPGRADE_FUNCTION	RESEARCH_UPGRADE_FUNCTION;
typedef UPGRADE_FUNCTION	REPAIR_UPGRADE_FUNCTION;
typedef UPGRADE_FUNCTION	POWER_UPGRADE_FUNCTION;
typedef UPGRADE_FUNCTION	REARM_UPGRADE_FUNCTION;

/*Upgrade the weapon ROF and accuracy for the weapons of a particular class*/
struct WEAPON_UPGRADE_FUNCTION : public FUNCTION
{
	WEAPON_SUBCLASS		subClass;			/*which weapons are affected */
	UBYTE			firePause;			/*The % to decrease the fire pause or reload time */
	UWORD			longHit;			/*The % to increase the long range accuracy */
	UWORD			damage;				/*The % to increase the damage*/
	UWORD			radiusDamage;		/*The % to increase the radius damage*/
	UWORD			periodicalDamage;		/*The % to increase the periodical damage*/
	UWORD			radiusHit;			/*The % to increase the chance to hit in blast radius*/

};

/*Upgrade the structure stats for all non wall and defence structures*/
struct STRUCTURE_UPGRADE_FUNCTION : public FUNCTION
{
};

/*Upgrade the structure stats for all wall and defence structures*/
struct WALLDEFENCE_UPGRADE_FUNCTION : public FUNCTION
{
};

typedef UPGRADE_FUNCTION	DROIDREPAIR_UPGRADE_FUNCTION;
typedef UPGRADE_FUNCTION	DROIDECM_UPGRADE_FUNCTION;
typedef UPGRADE_FUNCTION	DROIDCONSTR_UPGRADE_FUNCTION;

struct DROIDSENSOR_UPGRADE_FUNCTION : public UPGRADE_FUNCTION
{
};

#endif // __INCLUDED_FUNCTIONDEF_H__
