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
/*
 * Feature.c
 *
 * Load feature stats
 */
#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/strres.h"

#include "lib/gamelib/gtime.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lib/netplay/netplay.h"

#include "feature.h"
#include "map.h"
#include "hci.h"
#include "power.h"
#include "objects.h"
#include "display.h"
#include "console.h"
#include "order.h"
#include "structure.h"
#include "miscimd.h"
#include "anim_id.h"
#include "visibility.h"
#include "text.h"
#include "effects.h"
#include "geometry.h"
#include "scores.h"
#include "combat.h"
#include "multiplay.h"
#include "advvis.h"

#include "mapgrid.h"
#include "display3d.h"
#include "random.h"

/* The statistics for the features */
FEATURE_STATS	*asFeatureStats;
UDWORD			numFeatureStats;

//Value is stored for easy access to this feature in destroyDroid()/destroyStruct()
FEATURE_STATS* oilResFeature = NULL;

/* other house droid to add */
#define DROID_TEMPLINDEX	0
#define DROID_X				(TILE_UNITS * 37 + TILE_UNITS/2)
#define DROID_Y				(TILE_UNITS + TILE_UNITS/2)
#define DROID_TARX			37
#define DROID_TARY			42

//specifies how far round (in tiles) a constructor droid sound look for more wreckage
#define WRECK_SEARCH 3

struct featureTypeMap
{
	const char *typeStr;
	FEATURE_TYPE type;
};

static const struct featureTypeMap map[] =
{
	{ "PROPULSION_TYPE_HOVER WRECK", FEAT_HOVER },
	{ "TANK WRECK", FEAT_TANK },
	{ "GENERIC ARTEFACT", FEAT_GEN_ARTE },
	{ "OIL RESOURCE", FEAT_OIL_RESOURCE },
	{ "BOULDER", FEAT_BOULDER },
	{ "VEHICLE", FEAT_VEHICLE },
	{ "DROID WRECK", FEAT_DROID },
	{ "BUILDING WRECK", FEAT_BUILD_WRECK },
	{ "BUILDING", FEAT_BUILDING },
	{ "OIL DRUM", FEAT_OIL_DRUM },
	{ "TREE", FEAT_TREE },
	{ "SKYSCRAPER", FEAT_SKYSCRAPER }
};


void featureInitVars(void)
{
	asFeatureStats = NULL;
	numFeatureStats = 0;
	oilResFeature = NULL;
}

static void featureType(FEATURE_STATS* psFeature, const char *pType)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(map); i++)
	{
		if (strcmp(pType, map[i].typeStr) == 0)
		{
			psFeature->subType = map[i].type;
			return;
		}
	}

	ASSERT(false, "featureType: Unknown feature type");
}

/* Load the feature stats */
BOOL loadFeatureStats(const char *pFeatureData, UDWORD bufferSize)
{
	FEATURE_STATS		*psFeature;
	unsigned int		i;
	char				featureName[MAX_STR_LENGTH], GfxFile[MAX_STR_LENGTH],
						type[MAX_STR_LENGTH];

	numFeatureStats = numCR(pFeatureData, bufferSize);

	// Skip descriptive header
	if (strncmp(pFeatureData,"Feature ",8)==0)
	{
		pFeatureData = strchr(pFeatureData,'\n') + 1;
		numFeatureStats--;
	}
	
	asFeatureStats = (FEATURE_STATS*)malloc(sizeof(FEATURE_STATS) * numFeatureStats);

	if (asFeatureStats == NULL)
	{
		debug( LOG_FATAL, "Feature Stats - Out of memory" );
		abort();
		return false;
	}

	psFeature = asFeatureStats;

	for (i = 0; i < numFeatureStats; i++)
	{
		UDWORD Width, Breadth;
		int damageable = 0, tileDraw = 0, allowLOS = 0, visibleAtStart = 0;

		memset(psFeature, 0, sizeof(FEATURE_STATS));

		featureName[0] = '\0';
		GfxFile[0] = '\0';
		type[0] = '\0';

		//read the data into the storage - the data is delimeted using comma's
		sscanf(pFeatureData, "%[^','],%d,%d,%d,%d,%d,%[^','],%[^','],%d,%d,%d",
			featureName, &Width, &Breadth,
			&damageable, &psFeature->armourValue, &psFeature->body,
			GfxFile, type, &tileDraw, &allowLOS,
			&visibleAtStart);

		psFeature->damageable = damageable;
		psFeature->tileDraw = tileDraw;
		psFeature->allowLOS = allowLOS;
		psFeature->visibleAtStart = visibleAtStart;

		// These are now only 16 bits wide - so we need to copy them
		psFeature->baseWidth = Width;
		psFeature->baseBreadth = Breadth;

		psFeature->pName = allocateName(featureName);
		if (!psFeature->pName)
		{
			return false;
		}

		if (psFeature->damageable && psFeature->body == 0)
		{
			debug(LOG_ERROR, "The feature %s, ref %d, is damageable, but has no body points!  The files need to be updated / fixed.  " \
							 "Assigning 1 body point to feature.", psFeature->pName, psFeature->ref);
			psFeature->body = 1;
		}
		//determine the feature type
		featureType(psFeature, type);

		//and the oil resource - assumes only one!
		if (psFeature->subType == FEAT_OIL_RESOURCE)
		{
			oilResFeature = psFeature;
		}

		//get the IMD for the feature
		psFeature->psImd = (iIMDShape *) resGetData("IMD", GfxFile);
		if (psFeature->psImd == NULL)
		{
			debug( LOG_ERROR, "Cannot find the feature PIE for record %s",  getName( psFeature->pName ) );
			return false;
		}

		psFeature->ref = REF_FEATURE_START + i;

		//increment the pointer to the start of the next record
		pFeatureData = strchr(pFeatureData,'\n') + 1;
		//increment the list to the start of the next storage block
		psFeature++;
	}

	return true;
}

/* Release the feature stats memory */
void featureStatsShutDown(void)
{
	if(numFeatureStats)
	{
		free(asFeatureStats);
		asFeatureStats = NULL;
	}
}

/** Deals with damage to a feature
 *  \param psFeature feature to deal damage to
 *  \param damage amount of damage to deal
 *  \param weaponClass,weaponSubClass the class and subclass of the weapon that deals the damage
 *  \param impactSide the side/directon on which the feature is hit
 *  \return < 0 never, >= 0 always
 */
int32_t featureDamage(FEATURE *psFeature, UDWORD damage, WEAPON_CLASS weaponClass, WEAPON_SUBCLASS weaponSubClass, HIT_SIDE impactSide)
{
	int32_t relativeDamage;

	ASSERT_OR_RETURN(0, psFeature != NULL, "Invalid feature pointer");

	debug(LOG_ATTACK, "feature (id %d): body %d armour %d damage: %d",
	      psFeature->id, psFeature->body, psFeature->armour[impactSide][weaponClass], damage);

	relativeDamage = objDamage((BASE_OBJECT *)psFeature, damage, psFeature->psStats->body, weaponClass, weaponSubClass, impactSide);

	// If the shell did sufficient damage to destroy the feature
	if (relativeDamage < 0)
	{
		debug(LOG_ATTACK, "feature (id %d) DESTROYED", psFeature->id);
		destroyFeature(psFeature);
		return relativeDamage * -1;
	}
	else
	{
		return relativeDamage;
	}
}


/* Create a feature on the map */
FEATURE * buildFeature(FEATURE_STATS *psStats, UDWORD x, UDWORD y,BOOL FromSave)
{
	UDWORD		mapX, mapY;
	UDWORD		width,breadth, foundationMin,foundationMax, height;
	UDWORD		startX,startY,max,min;
	SDWORD		i;
	UBYTE		vis;
	//try and create the Feature
	FEATURE *psFeature = new FEATURE(generateSynchronisedObjectId(), psStats);

	if (psFeature == NULL)
	{
		debug(LOG_WARNING, "Feature couldn't be built.");
		return NULL;
	}
	// features are not in the cluster system
	// this will cause an assert when they still end up there
	psFeature->cluster = ~0;
	//add the feature to the list - this enables it to be drawn whilst being built
	addFeature(psFeature);

	// get the terrain average height
	startX = map_coord(x);
	startY = map_coord(y);
	foundationMin = TILE_MAX_HEIGHT;
	foundationMax = TILE_MIN_HEIGHT;
	for (breadth = 0; breadth < psStats->baseBreadth; breadth++)
	{
		for (width = 0; width < psStats->baseWidth; width++)
		{
			getTileMaxMin(startX + width, startY + breadth, &max, &min);
			if (foundationMin > min)
			{
				foundationMin = min;
			}
			if (foundationMax < max)
			{
				foundationMax = max;
			}
		}
	}
	//return the average of max/min height
	height = (foundationMin + foundationMax) / 2;

	// snap the coords to a tile
	if (!FromSave)
	{
		x = (x & ~TILE_MASK) + psStats->baseWidth  %2 * TILE_UNITS/2;
		y = (y & ~TILE_MASK) + psStats->baseBreadth%2 * TILE_UNITS/2;
	}
	else
	{
		if ((x & TILE_MASK) != psStats->baseWidth  %2 * TILE_UNITS/2 ||
		    (y & TILE_MASK) != psStats->baseBreadth%2 * TILE_UNITS/2)
		{
			debug(LOG_WARNING, "Feature not aligned. position (%d,%d), size (%d,%d)", x, y, psStats->baseWidth, psStats->baseBreadth);
		}
	}

	psFeature->pos.x = x;
	psFeature->pos.y = y;

	/* Dump down the building wrecks at random angles - still looks shit though */
	if(psStats->subType == FEAT_BUILD_WRECK || psStats->subType == FEAT_TREE)
	{
		psFeature->rot.direction = gameRand(DEG_360);
	}
	else
	{
		psFeature->rot.direction = 0;
	}
	psFeature->selected = false;
	psFeature->body = psStats->body;
	objSensorCache((BASE_OBJECT *)psFeature, NULL);
	objEcmCache((BASE_OBJECT *)psFeature, NULL);
	psFeature->bTargetted = false;
	psFeature->timeLastHit = 0;
	psFeature->lastHitWeapon = WSC_NUM_WEAPON_SUBCLASSES;  // no such weapon

	// it has never been drawn
	psFeature->sDisplay.frameNumber = 0;

	if(getRevealStatus())
	{
		vis = 0;
	}
	else
	{
		if(psStats->visibleAtStart)
		{
  			vis = UBYTE_MAX;
		}
		else
		{
			vis = 0;
		}
	}

	// note that the advanced armour system current unused for features
	for (i = 0; i < NUM_HIT_SIDES; i++)
	{
		int j;

		for (j = 0; j < WC_NUM_WEAPON_CLASSES; j++)
		{
			psFeature->armour[i][j] = psFeature->psStats->armourValue;
		}
	}

	memset(psFeature->seenThisTick, 0, sizeof(psFeature->seenThisTick));
	memset(psFeature->visible, 0, sizeof(psFeature->visible));

	//load into the map data
	mapX = map_coord(x) - psStats->baseWidth/2;
	mapY = map_coord(y) - psStats->baseBreadth/2;

	// set up the imd for the feature
	if(psFeature->psStats->subType==FEAT_BUILD_WRECK)
	{
		psFeature->sDisplay.imd = getRandomWreckageImd();
	}
	else
	{
		psFeature->sDisplay.imd = psStats->psImd;
  	}

	ASSERT_OR_RETURN(NULL, psFeature->sDisplay.imd, "No IMD for feature");		// make sure we have an imd.

	for (width = 0; width < psStats->baseWidth; width++)
	{
		for (breadth = 0; breadth < psStats->baseBreadth; breadth++)
		{
			MAPTILE *psTile = mapTile(mapX + width, mapY + breadth);

			//check not outside of map - for load save game
			ASSERT_OR_RETURN(NULL, mapX + width < mapWidth, "x coord bigger than map width - %s, id = %d", getName(psFeature->psStats->pName), psFeature->id);
			ASSERT_OR_RETURN(NULL, mapY + breadth < mapHeight, "y coord bigger than map height - %s, id = %d", getName(psFeature->psStats->pName), psFeature->id);

			if (width != psStats->baseWidth && breadth != psStats->baseBreadth)
			{
				if (TileHasFeature(psTile))
				{
					FEATURE *psBlock = (FEATURE *)psTile->psObject;

					debug(LOG_ERROR, "%s(%d) already placed at (%d+%d, %d+%d) when trying to place %s(%d) at (%d+%d, %d+%d) - removing it",
					      getName(psBlock->psStats->pName), psBlock->id, map_coord(psBlock->pos.x), psBlock->psStats->baseWidth, map_coord(psBlock->pos.y),
					      psBlock->psStats->baseBreadth, getName(psFeature->psStats->pName), psFeature->id, mapX, psStats->baseWidth, mapY, psStats->baseBreadth);

					removeFeature(psBlock);
				}

				psTile->psObject = (BASE_OBJECT*)psFeature;

				// if it's a tall feature then flag it in the map.
				if (psFeature->sDisplay.imd->max.y > TALLOBJECT_YMAX)
				{
					auxSetBlocking(mapX + width, mapY + breadth, AIR_BLOCKED);
				}

				if (psStats->subType != FEAT_GEN_ARTE && psStats->subType != FEAT_OIL_DRUM && psStats->subType != FEAT_BUILD_WRECK)
				{
					auxSetBlocking(mapX + width, mapY + breadth, FEATURE_BLOCKED);
				}
			}

			if( (!psStats->tileDraw) && (FromSave == false) )
			{
				psTile->height = height;
			}
		}
	}
	psFeature->pos.z = map_TileHeight(mapX,mapY);//jps 18july97

//	// set up the imd for the feature
//	if(psFeature->psStats->subType==FEAT_BUILD_WRECK)
//	{
//		psFeature->sDisplay.imd = wreckageImds[rand()%MAX_WRECKAGE];
//	}
//	else
//	{
//		psFeature->sDisplay.imd = psStats->psImd;
// 	}

	return psFeature;
}


FEATURE::FEATURE(uint32_t id, FEATURE_STATS const *psStats)
	: BASE_OBJECT(OBJ_FEATURE, id, MAX_PLAYERS + 1)  // Set the default player out of range to avoid targeting confusions
	, psStats(psStats)
{}

/* Release the resources associated with a feature */
FEATURE::~FEATURE()
{
}

void _syncDebugFeature(const char *function, FEATURE *psFeature, char ch)
{
	_syncDebug(function, "%c feature%d = p%d;pos(%d,%d,%d),subtype%d,dam%d,bp%d", ch,
	          psFeature->id,

	          psFeature->player,
	          psFeature->pos.x, psFeature->pos.y, psFeature->pos.z,
	          psFeature->psStats->subType,
	          psFeature->psStats->damageable,
	          psFeature->body

	          );
}

/* Update routine for features */
void featureUpdate(FEATURE *psFeat)
{
	syncDebugFeature(psFeat, '<');

   //	if(getRevealStatus())
   //	{
		// update the visibility for the feature
		processVisibilityLevel((BASE_OBJECT *)psFeat);
   //	}

	switch (psFeat->psStats->subType)
	{
	case FEAT_DROID:
	case FEAT_BUILD_WRECK:
//		//kill off wrecked droids and structures after 'so' long
//		if ((gameTime - psFeat->born) > WRECK_LIFETIME)
//		{
			destroyFeature(psFeat); // get rid of the now!!!
//		}
		return;
	default:
		break;
	}

	syncDebugFeature(psFeat, '>');
}


// free up a feature with no visual effects
bool removeFeature(FEATURE *psDel)
{
	int		mapX, mapY, width, breadth, player;
	MESSAGE		*psMessage;
	Vector3i	pos;

	ASSERT_OR_RETURN(false, psDel != NULL, "Invalid feature pointer");
	ASSERT_OR_RETURN(false, !psDel->died, "Feature already dead");

	if(bMultiMessages && !ingame.localJoiningInProgress)
	{
		SendDestroyFeature(psDel);	// inform other players of destruction
		return true;  // Wait for our message before really destroying the feature.
	}

	//remove from the map data
	mapX = map_coord(psDel->pos.x) - psDel->psStats->baseWidth/2;
	mapY = map_coord(psDel->pos.y) - psDel->psStats->baseBreadth/2;
	for (width = 0; width < psDel->psStats->baseWidth; width++)
	{
		for (breadth = 0; breadth < psDel->psStats->baseBreadth; breadth++)
		{
			if (tileOnMap(mapX + width, mapY + breadth))
			{
				MAPTILE *psTile = mapTile(mapX + width, mapY + breadth);
	 
				if (psTile->psObject == (BASE_OBJECT *)psDel)
				{
					psTile->psObject = NULL;
					auxClearBlocking(mapX + width, mapY + breadth, FEATURE_BLOCKED | AIR_BLOCKED);
				}
			}
		}
	}

	if (psDel->psStats->subType == FEAT_GEN_ARTE || psDel->psStats->subType == FEAT_OIL_DRUM)
	{
		pos.x = psDel->pos.x;
		pos.z = psDel->pos.y;
		pos.y = map_Height(pos.x, pos.z) + 30;
		addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_DISCOVERY,false,NULL,0);
		if (psDel->psStats->subType == FEAT_GEN_ARTE)
		{
			scoreUpdateVar(WD_ARTEFACTS_FOUND);
			intRefreshScreen();
		}
	}

	if (psDel->psStats->subType == FEAT_GEN_ARTE || psDel->psStats->subType == FEAT_OIL_RESOURCE)
	{
		for (player = 0; player < MAX_PLAYERS; player++)
		{
			psMessage = findMessage((MSG_VIEWDATA *)psDel, MSG_PROXIMITY, player);
			while (psMessage)
			{
				removeMessage(psMessage, player);
				psMessage = findMessage((MSG_VIEWDATA *)psDel, MSG_PROXIMITY, player);
			}
		}
	}

	killFeature(psDel);

	return true;
}

/* Remove a Feature and free it's memory */
bool destroyFeature(FEATURE *psDel)
{
	UDWORD			widthScatter,breadthScatter,heightScatter, i;
	EFFECT_TYPE		explosionSize;
	Vector3i pos;
	UDWORD			width,breadth;
	UDWORD			mapX,mapY;

	ASSERT_OR_RETURN(false, psDel != NULL, "Invalid feature pointer");

 	/* Only add if visible and damageable*/
	if(psDel->visible[selectedPlayer] && psDel->psStats->damageable)
	{
		/* Set off a destruction effect */
		/* First Explosions */
		widthScatter = TILE_UNITS/2;
		breadthScatter = TILE_UNITS/2;
		heightScatter = TILE_UNITS/4;
		//set which explosion to use based on size of feature
		if (psDel->psStats->baseWidth < 2 && psDel->psStats->baseBreadth < 2)
		{
			explosionSize = EXPLOSION_TYPE_SMALL;
		}
		else if (psDel->psStats->baseWidth < 3 && psDel->psStats->baseBreadth < 3)
		{
			explosionSize = EXPLOSION_TYPE_MEDIUM;
		}
		else
		{
			explosionSize = EXPLOSION_TYPE_LARGE;
		}
		for(i=0; i<4; i++)
		{
			pos.x = psDel->pos.x + widthScatter - rand()%(2*widthScatter);
			pos.z = psDel->pos.y + breadthScatter - rand()%(2*breadthScatter);
			pos.y = psDel->pos.z + 32 + rand()%heightScatter;
			addEffect(&pos,EFFECT_EXPLOSION,explosionSize,false,NULL,0);
		}

		if(psDel->psStats->subType == FEAT_SKYSCRAPER)
		{
			pos.x = psDel->pos.x;
			pos.z = psDel->pos.y;
			pos.y = psDel->pos.z;
			addEffect(&pos,EFFECT_DESTRUCTION,DESTRUCTION_TYPE_SKYSCRAPER,true,psDel->sDisplay.imd,0);
			initPerimeterSmoke(psDel->sDisplay.imd, pos);

			shakeStart();
		}

		/* Then a sequence of effects */
		pos.x = psDel->pos.x;
		pos.z = psDel->pos.y;
		pos.y = map_Height(pos.x,pos.z);
		addEffect(&pos,EFFECT_DESTRUCTION,DESTRUCTION_TYPE_FEATURE,false,NULL,0);

		//play sound
		// ffs gj
		if(psDel->psStats->subType == FEAT_SKYSCRAPER)
		{
			audio_PlayStaticTrack( psDel->pos.x, psDel->pos.y, ID_SOUND_BUILDING_FALL );
		}
		else
		{
			audio_PlayStaticTrack( psDel->pos.x, psDel->pos.y, ID_SOUND_EXPLOSION );
		}
	}

	if (psDel->psStats->subType == FEAT_SKYSCRAPER)
	{
		// ----- Flip all the tiles under the skyscraper to a rubble tile
		// smoke effect should disguise this happening
		mapX = map_coord(psDel->pos.x) - psDel->psStats->baseWidth/2;
		mapY = map_coord(psDel->pos.y) - psDel->psStats->baseBreadth/2;
		for (width = 0; width < psDel->psStats->baseWidth; width++)
		{
			for (breadth = 0; breadth < psDel->psStats->baseBreadth; breadth++)
			{
				MAPTILE *psTile = mapTile(mapX+width,mapY+breadth);
				// stops water texture chnaging for underwateer festures
				if (terrainType(psTile) != TER_WATER)
				{
					if (terrainType(psTile) != TER_CLIFFFACE)
					{
						/* Clear feature bits */
						psTile->texture = TileNumber_texture(psTile->texture) | RUBBLE_TILE;
					}
					else
					{
						/* This remains a blocking tile */
						psTile->psObject = NULL;
						psTile->texture = TileNumber_texture(psTile->texture) | BLOCKING_RUBBLE_TILE;
					}
				}
			}
		}
	}

	removeFeature(psDel);
	return true;
}


SDWORD getFeatureStatFromName( const char *pName )
{
	unsigned int inc;
	FEATURE_STATS *psStat;

	for (inc = 0; inc < numFeatureStats; inc++)
	{
		psStat = &asFeatureStats[inc];
		if (!strcmp(psStat->pName, pName))
		{
			return inc;
		}
	}
	return -1;
}


/*looks around the given droid to see if there is any building
wreckage to clear*/
FEATURE	* checkForWreckage(DROID *psDroid)
{
	FEATURE		*psFeature;
	UDWORD		startX, startY, incX, incY;
	SDWORD		x=0, y=0;

	startX = map_coord(psDroid->pos.x);
	startY = map_coord(psDroid->pos.y);

	//look around the droid - max 2 tiles distance
	for (incX = 1, incY = 1; incX < WRECK_SEARCH; incX++, incY++)
	{
		/* across the top */
		y = startY - incY;
		for(x = startX - incX; x < (SDWORD)(startX + incX); x++)
		{
			if(TileHasFeature(mapTile(x,y)))
			{
				psFeature = getTileFeature(x, y);
				if(psFeature && psFeature->psStats->subType == FEAT_BUILD_WRECK)
				{
					return psFeature;
				}
			}
		}
		/* the right */
		x = startX + incX;
		for(y = startY - incY; y < (SDWORD)(startY + incY); y++)
		{
			if(TileHasFeature(mapTile(x,y)))
			{
				psFeature = getTileFeature(x, y);
				if(psFeature && psFeature->psStats->subType == FEAT_BUILD_WRECK)
				{
					return psFeature;
				}
			}
		}
		/* across the bottom*/
		y = startY + incY;
		for(x = startX + incX; x > (SDWORD)(startX - incX); x--)
		{
			if(TileHasFeature(mapTile(x,y)))
			{
				psFeature = getTileFeature(x, y);
				if(psFeature && psFeature->psStats->subType == FEAT_BUILD_WRECK)
				{
					return psFeature;
				}
			}
		}

		/* the left */
		x = startX - incX;
		for(y = startY + incY; y > (SDWORD)(startY - incY); y--)
		{
			if(TileHasFeature(mapTile(x,y)))
			{
				psFeature = getTileFeature(x, y);
				if(psFeature && psFeature->psStats->subType == FEAT_BUILD_WRECK)
				{
					return psFeature;
				}
			}
		}
	}
	return NULL;
}
