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
/*
 * Function.c
 *
 * Store function stats for the Structures etc.
 *
 */
#include "lib/framework/frame.h"
#include "lib/framework/strres.h"

#include "function.h"
#include "stats.h"
#include "structure.h"
#include "text.h"
#include "droid.h"
#include "group.h"

#include "multiplay.h"


//holder for all functions
FUNCTION **asFunctions;
UDWORD numFunctions;


typedef bool (*LoadFunction)(const char *pData);


/*Returns the Function type based on the string - used for reading in data */
static FUNCTION_TYPE functionType(const char *pType)
{
	if (!strcmp(pType, "Repair Droid"))
	{
		return REPAIR_DROID_TYPE;
	}
	if (!strcmp(pType, "Weapon Upgrade"))
	{
		return WEAPON_UPGRADE_TYPE;
	}
	if (!strcmp(pType, "Wall Function"))
	{
		return WALL_TYPE;
	}
	if (!strcmp(pType, "VehicleRepair Upgrade"))
	{
		return DROIDREPAIR_UPGRADE_TYPE;
	}
	if (!strcmp(pType, "VehicleConst Upgrade"))
	{
		return DROIDCONST_UPGRADE_TYPE;
	}
	ASSERT(false, "Unknown Function Type: %s", pType);
	return NUMFUNCTIONS;
}

// Allocate storage for the name
static bool storeName(FUNCTION *pFunction, const char *pNameToStore)
{
	pFunction->pName = strdup(pNameToStore);
	return true;
}

//generic load function for upgrade type
static bool loadUpgradeFunction(const char *pData, FUNCTION_TYPE type)
{
	char functionName[MAX_STR_LENGTH];
	UDWORD modifier;
	UPGRADE_FUNCTION *psFunction;

	//allocate storage
	psFunction = (UPGRADE_FUNCTION *)malloc(sizeof(UPGRADE_FUNCTION));
	memset(psFunction, 0, sizeof(UPGRADE_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION *)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = type;

	//read the data in
	functionName[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%d", functionName, &modifier);

	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	if (modifier > UWORD_MAX)
	{
		ASSERT(false, "Modifier too great for %s", functionName);
		return false;
	}

	//store the % upgrade
	psFunction->upgradePoints = (UWORD)modifier;

	return true;
}

static bool loadDroidRepairUpgradeFunction(const char *pData)
{
	return loadUpgradeFunction(pData, DROIDREPAIR_UPGRADE_TYPE);
}

static bool loadDroidConstUpgradeFunction(const char *pData)
{
	return loadUpgradeFunction(pData, DROIDCONST_UPGRADE_TYPE);
}

static bool loadWeaponUpgradeFunction(const char *pData)
{
	WEAPON_UPGRADE_FUNCTION	*psFunction;
	char functionName[MAX_STR_LENGTH], weaponSubClass[MAX_STR_LENGTH];
	UDWORD firePause, dummyVal, longHit, damage, radiusDamage, periodicalDamage, radiusHit;

	//allocate storage
	psFunction = (WEAPON_UPGRADE_FUNCTION *)malloc(sizeof(WEAPON_UPGRADE_FUNCTION));
	memset(psFunction, 0, sizeof(WEAPON_UPGRADE_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION *)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = WEAPON_UPGRADE_TYPE;

	//read the data in
	functionName[0] = '\0';
	weaponSubClass[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%255[^,'\r\n],%d,%d,%d,%d,%d,%d,%d", functionName,
	       weaponSubClass, &firePause, &dummyVal, &longHit, &damage, &radiusDamage,
	       &periodicalDamage, &radiusHit);

	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	if (!getWeaponSubClass(weaponSubClass, &psFunction->subClass))
	{
		return false;
	}

	//check none of the %increases are over UBYTE max
	if (firePause > UBYTE_MAX ||
	    longHit > UWORD_MAX ||
	    damage > UWORD_MAX ||
	    radiusDamage > UWORD_MAX ||
	    periodicalDamage > UWORD_MAX ||
	    radiusHit > UWORD_MAX)
	{
		debug(LOG_ERROR, "A percentage increase for Weapon Upgrade function is too large");
		return false;
	}

	//copy the data across
	psFunction->firePause = (UBYTE)firePause;
	psFunction->longHit = (UWORD)longHit;
	psFunction->damage = (UWORD)damage;
	psFunction->radiusDamage = (UWORD)radiusDamage;
	psFunction->periodicalDamage = (UWORD)periodicalDamage;
	psFunction->radiusHit = (UWORD)radiusHit;

	return true;
}

static bool loadRepairDroidFunction(const char *pData)
{
	REPAIR_DROID_FUNCTION		*psFunction;
	char						functionName[MAX_STR_LENGTH];

	//allocate storage
	psFunction = (REPAIR_DROID_FUNCTION *)malloc(sizeof(REPAIR_DROID_FUNCTION));
	memset(psFunction, 0, sizeof(REPAIR_DROID_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION *)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = REPAIR_DROID_TYPE;

	//read the data in
	functionName[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%d", functionName, &psFunction->repairPoints);

	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	return true;
}


/*loads the corner stat to use for a particular wall stat */
static bool loadWallFunction(const char *pData)
{
	WALL_FUNCTION			*psFunction;
	char					functionName[MAX_STR_LENGTH];
	char					structureName[MAX_STR_LENGTH];

	//allocate storage
	psFunction = (WALL_FUNCTION *)malloc(sizeof(WALL_FUNCTION));
	memset(psFunction, 0, sizeof(WALL_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION *)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = WALL_TYPE;

	//read the data in
	functionName[0] = '\0';
	structureName[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%255[^,'\r\n],%*d", functionName, structureName);

	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	//store the structure name - cannot set the stat pointer here because structures
	//haven't been loaded in yet!
	psFunction->pStructName = allocateName(structureName);
	if (!psFunction->pStructName)
	{
		debug(LOG_ERROR, "Structure Stats Invalid for function - %s", functionName);
		return false;
	}
	psFunction->pCornerStat = NULL;

	return true;
}

void droidBodyUpgrade(DROID *psDroid)
{
	BODY_STATS *psStats = getBodyStats(psDroid);
	int increase = psStats->upgrade[psDroid->player].body;
	int prevBaseBody = psDroid->originalBody;
	int base = calcDroidBaseBody(psDroid);
	int newBaseBody =  base + (base * increase) / 100;

	if (newBaseBody > prevBaseBody)
	{
		psDroid->body = (psDroid->body * newBaseBody) / prevBaseBody;
		psDroid->originalBody = newBaseBody;
	}
	//if a transporter droid then need to upgrade the contents
	if (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)
	{
		for (DROID *psCurr = psDroid->psGroup->psList; psCurr != NULL; psCurr = psCurr->psGrpNext)
		{
			if (psCurr != psDroid)
			{
				droidBodyUpgrade(psCurr);
			}
		}
	}
}

//upgrade the weapon stats for the correct subclass
void weaponUpgrade(FUNCTION *pFunction, UBYTE player)
{
	WEAPON_UPGRADE_FUNCTION		*pUpgrade;

	pUpgrade = (WEAPON_UPGRADE_FUNCTION *)pFunction;

	//check upgrades increase all values!
	if (asWeaponUpgrade[player][pUpgrade->subClass].firePause < pUpgrade->firePause)
	{
		//make sure don't go less than 100%
		if (pUpgrade->firePause > 100)
		{
			pUpgrade->firePause = 100;
		}
		asWeaponUpgrade[player][pUpgrade->subClass].firePause = pUpgrade->firePause;
	}
	if (asWeaponUpgrade[player][pUpgrade->subClass].longHit < pUpgrade->longHit)
	{
		asWeaponUpgrade[player][pUpgrade->subClass].longHit = pUpgrade->longHit;
	}
	if (asWeaponUpgrade[player][pUpgrade->subClass].damage < pUpgrade->damage)
	{
		asWeaponUpgrade[player][pUpgrade->subClass].damage = pUpgrade->damage;
	}
	if (asWeaponUpgrade[player][pUpgrade->subClass].radiusDamage < pUpgrade->radiusDamage)
	{
		asWeaponUpgrade[player][pUpgrade->subClass].radiusDamage = pUpgrade->radiusDamage;
	}
	if (asWeaponUpgrade[player][pUpgrade->subClass].periodicalDamage < pUpgrade->periodicalDamage)
	{
		asWeaponUpgrade[player][pUpgrade->subClass].periodicalDamage = pUpgrade->periodicalDamage;
	}
	if (asWeaponUpgrade[player][pUpgrade->subClass].radiusHit < pUpgrade->radiusHit)
	{
		asWeaponUpgrade[player][pUpgrade->subClass].radiusHit = pUpgrade->radiusHit;
	}
}

//upgrade the repair stats
void repairUpgrade(FUNCTION *pFunction, UBYTE player)
{
	DROIDREPAIR_UPGRADE_FUNCTION		*pUpgrade;

	pUpgrade = (DROIDREPAIR_UPGRADE_FUNCTION *)pFunction;

	//check upgrades increase all values!
	if (asRepairUpgrade[player].repairPoints < pUpgrade->upgradePoints)
	{
		asRepairUpgrade[player].repairPoints = pUpgrade->upgradePoints;
	}
}

//upgrade the repair stats
void constructorUpgrade(FUNCTION *pFunction, UBYTE player)
{
	DROIDCONSTR_UPGRADE_FUNCTION		*pUpgrade;

	pUpgrade = (DROIDCONSTR_UPGRADE_FUNCTION *)pFunction;

	//check upgrades increase all values!
	if (asConstUpgrade[player].constructPoints < pUpgrade->upgradePoints)
	{
		asConstUpgrade[player].constructPoints = pUpgrade->upgradePoints;
	}
}

/*upgrades the droids inside a Transporter uwith the appropriate upgrade function*/
void upgradeTransporterDroids(DROID *psTransporter, void(*pUpgradeFunction)(DROID *psDroid))
{
	ASSERT(psTransporter->droidType == DROID_TRANSPORTER || psTransporter->droidType == DROID_SUPERTRANSPORTER, "upgradeTransporterUnits: invalid unit type");

	//loop thru' each unit in the Transporter
	for (DROID *psCurr = psTransporter->psGroup->psList; psCurr != NULL; psCurr = psCurr->psGrpNext)
	{
		if (psCurr != psTransporter)
		{
			//apply upgrade if not the transporter itself
			pUpgradeFunction(psCurr);
		}
	}
}

bool FunctionShutDown(void)
{
	UDWORD		inc;
	FUNCTION	*pFunction, **pStartList = asFunctions;

	for (inc = 0; inc < numFunctions; inc++)
	{
		pFunction = *asFunctions;
		free(pFunction->pName);
		free(pFunction);
		asFunctions++;
	}
	free(pStartList);

	return true;
}

bool loadFunctionStats(const char *pFunctionData, UDWORD bufferSize)
{
	//array of functions pointers for each load function
	static const LoadFunction pLoadFunction[NUMFUNCTIONS] =
	{
		loadRepairDroidFunction,
		loadWeaponUpgradeFunction,
		loadWallFunction,
		loadDroidRepairUpgradeFunction,
		loadDroidConstUpgradeFunction,
	};

	const unsigned int totalFunctions = numCR(pFunctionData, bufferSize);
	UDWORD		i;
	FUNCTION_TYPE type;
	char		FunctionType[MAX_STR_LENGTH];
	FUNCTION	**pStartList;

	//allocate storage for the Function pointer array
	asFunctions = (FUNCTION **) malloc(totalFunctions * sizeof(FUNCTION *));
	pStartList = asFunctions;
	//initialise the storage
	memset(asFunctions, 0, totalFunctions * sizeof(FUNCTION *));
	numFunctions = 0;

	for (i = 0; i < totalFunctions; i++)
	{
		//read the data into the storage - the data is delimeted using comma's
		FunctionType[0] = '\0';
		sscanf(pFunctionData, "%255[^,'\r\n]", FunctionType);
		type = functionType(FunctionType);
		ASSERT_OR_RETURN(false, type != NUMFUNCTIONS, "Function type not found");
		pFunctionData += (strlen(FunctionType) + 1);

		if (!(pLoadFunction[type](pFunctionData)))
		{
			return false;
		}
		//increment the pointer to the start of the next record
		pFunctionData = strchr(pFunctionData, '\n') + 1;
	}
	//set the function list pointer to the start
	asFunctions = pStartList;

	return true;
}
