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
 * Multistruct.c
 *
 * Alex Lee 98, Pumpkin Studios.
 *
 * files to cope with multiplayer structure related stuff..
 */

#include <string.h>

#include "lib/framework/frame.h"
#include "droid.h"
#include "droiddef.h"
#include "basedef.h"
#include "power.h"
#include "geometry.h"								// for gettilestructure
#include "anim_id.h"
#include "stats.h"
#include "map.h"
#include "console.h"
#include "action.h"
#include "order.h"
#include "projectile.h"
#include "lib/netplay/netplay.h"								// the netplay library.
#include "multiplay.h"
#include "multigifts.h"
#include "multirecv.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/audio.h"
#include "research.h"

// ////////////////////////////////////////////////////////////////////////////
// structures

// ////////////////////////////////////////////////////////////////////////////
// INFORM others that a building has been completed.
BOOL SendBuildFinished(STRUCTURE *psStruct)
{
	uint8_t player = psStruct->player;
	ASSERT( player < MAX_PLAYERS, "invalid player %u", player);

	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_BUILDFINISHED);
		NETuint32_t(&psStruct->id);		// ID of building

		// Along with enough info to build it (if needed)
		NETuint32_t(&psStruct->pStructureType->ref);
		NETPosition(&psStruct->pos);
		NETuint8_t(&player);
	return NETend();
}

// ////////////////////////////////////////////////////////////////////////////
BOOL recvBuildFinished(NETQUEUE queue)
{
	uint32_t	structId;
	STRUCTURE	*psStruct;
	Position        pos;
	uint32_t	type,typeindex;
	uint8_t		player;

	NETbeginDecode(queue, GAME_BUILDFINISHED);
		NETuint32_t(&structId);	// get the struct id.
		NETuint32_t(&type); 	// Kind of building.
		NETPosition(&pos);      // pos
		NETuint8_t(&player);
	NETend();

	ASSERT( player < MAX_PLAYERS, "invalid player %u", player);

	psStruct = IdToStruct(structId,ANYPLAYER);

	if (psStruct)
	{												// make it complete.
		psStruct->currentBuildPts = psStruct->pStructureType->buildPoints+1;

		if (psStruct->status != SS_BUILT)
		{
			debug(LOG_SYNC, "Synch error, structure %u was not complete, and should have been.", structId);
			psStruct->status = SS_BUILT;
			buildingComplete(psStruct);
		}
		debug(LOG_SYNC, "Created normal building %u for player %u", psStruct->id, player);
		return true;
	}

	// The building wasn't started, so we'll have to just plonk it down in the map.

	// Find the structures stats
	for (typeindex = 0; typeindex < numStructureStats && asStructureStats[typeindex].ref != type; typeindex++) {}	// Find structure target

	// Check for similar buildings, to avoid overlaps
	if (TileHasStructure(mapTile(map_coord(pos.x), map_coord(pos.y))))
	{
		// Get the current structure
		psStruct = getTileStructure(map_coord(pos.x), map_coord(pos.y));
		if (asStructureStats[typeindex].type == psStruct->pStructureType->type)
		{
			// Correct type, correct location, just rename the id's to sync it.. (urgh)
			psStruct->id = structId;
			psStruct->status = SS_BUILT;
			buildingComplete(psStruct);
			debug(LOG_SYNC, "Created modified building %u for player %u", psStruct->id, player);
#if defined (DEBUG)
			NETlogEntry("structure id modified", SYNC_FLAG, player);
#endif
			return true;
		}
	}
	// Build the structure
	psStruct = buildStructure(&(asStructureStats[typeindex]), pos.x, pos.y, player, true);

	if (psStruct)
	{
		psStruct->id		= structId;
		psStruct->status	= SS_BUILT;
		buildingComplete(psStruct);
		debug(LOG_SYNC, "Huge synch error, forced to create building %u for player %u", psStruct->id, player);
#if defined (DEBUG)
		NETlogEntry("had to plonk down a building", SYNC_FLAG, player);
#endif
	}
	else
	{
		debug(LOG_SYNC, "Gigantic synch error, unable to create building for player %u", player);
		NETlogEntry("had to plonk down a building, BUT FAILED!", SYNC_FLAG, player);
	}

	return false;
}


// ////////////////////////////////////////////////////////////////////////////
// demolish message.
BOOL SendDemolishFinished(STRUCTURE *psStruct, DROID *psDroid)
{
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_DEMOLISH);

		// Send what is being demolish and who is doing it
		NETuint32_t(&psStruct->id);
		NETuint32_t(&psDroid->id);

	return NETend();
}

BOOL recvDemolishFinished(NETQUEUE queue)
{
	STRUCTURE	*psStruct;
	DROID		*psDroid;
	uint32_t	structID, droidID;

	NETbeginDecode(queue, GAME_DEMOLISH);
		NETuint32_t(&structID);
		NETuint32_t(&droidID);
	NETend();

	psStruct = IdToStruct(structID, ANYPLAYER);
	if (!IdToDroid(droidID, ANYPLAYER, &psDroid))
	{
		debug(LOG_ERROR, "recvDemolishFinished: Packet with bad droid ID received. Discarding!");
		return false;
	}

	if (psStruct)
	{
		// Demolish it
		// Should never get here, if in synch.
		removeStruct(psStruct, true);
		if (psDroid && psDroid->psTarStats)
		{
			// Update droid if reqd
			psDroid->psTarStats = NULL;
		}
	}

	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// Inform others that a structure has been destroyed
BOOL SendDestroyStructure(STRUCTURE *s)
{
	technologyGiveAway(s);
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_STRUCTDEST);

	// Struct to destroy
	NETuint32_t(&s->id);

	return NETend();
}

// ////////////////////////////////////////////////////////////////////////////
// acknowledge the destruction of a structure, from another player.
BOOL recvDestroyStructure(NETQUEUE queue)
{
	uint32_t structID;
	STRUCTURE *psStruct;

	NETbeginDecode(queue, GAME_STRUCTDEST);
		NETuint32_t(&structID);
	NETend();

	// Struct to destory
	psStruct = IdToStruct(structID,ANYPLAYER);

	if (psStruct)
	{
		turnOffMultiMsg(true);
		// Remove the struct from remote players machine
		destroyStruct(psStruct);
		turnOffMultiMsg(false);
		// NOTE: I do not think this should be here!
		technologyGiveAway(psStruct);
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
//lassat is firing

BOOL sendLasSat(UBYTE player, STRUCTURE *psStruct, BASE_OBJECT *psObj)
{
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_LASSAT);

		NETuint8_t(&player);
		NETuint32_t(&psStruct->id);
		NETuint32_t(&psObj->id);	// Target
		NETuint8_t(&psObj->player);	// Target player

	return NETend();
}

// recv lassat info on the receiving end.
BOOL recvLasSat(NETQUEUE queue)
{
	BASE_OBJECT	*psObj;
	UBYTE		player,targetplayer;
	STRUCTURE	*psStruct;
	uint32_t	id,targetid;

	// TODO Add some kind of checking, so that things don't get lasatted by bunkers.
	NETbeginDecode(queue, GAME_LASSAT);
		NETuint8_t(&player);
		NETuint32_t(&id);
		NETuint32_t(&targetid);
		NETuint8_t(&targetplayer);
	NETend();

	psStruct = IdToStruct (id, player);
	psObj	 = IdToPointer(targetid, targetplayer);

	if (psStruct && psObj)
	{
		// Give enemy no quarter, unleash the lasat
		proj_SendProjectile(&psStruct->asWeaps[0], NULL, player, psObj->pos, psObj, true, 0);
		psStruct->asWeaps[0].lastFired = gameTime;

		// Play 5 second countdown message
		audio_QueueTrackPos( ID_SOUND_LAS_SAT_COUNTDOWN, psObj->pos.x, psObj->pos.y, psObj->pos.z);
	}

	return true;
}

void sendStructureInfo(STRUCTURE *psStruct, STRUCTURE_INFO structureInfo_, DROID_TEMPLATE *psTempl)
{
	uint8_t  player = psStruct->player;
	uint32_t structId = psStruct->id;
	uint8_t  structureInfo = structureInfo_;

	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_STRUCTUREINFO);
		NETuint8_t(&player);
		NETuint32_t(&structId);
		NETuint8_t(&structureInfo);
		if (structureInfo_ == STRUCTUREINFO_MANUFACTURE)
		{
			uint32_t templateId = psTempl != NULL ? psTempl->multiPlayerID : 0;

			NETuint32_t(&templateId);
		}
	NETend();
}

void recvStructureInfo(NETQUEUE queue)
{
	uint8_t         player = 0;
	uint32_t        structId = 0;
	uint32_t        templateId = 0;
	uint8_t         structureInfo;
	STRUCTURE *     psStruct;
	DROID_TEMPLATE *psTempl = NULL;

	NETbeginDecode(queue, GAME_STRUCTUREINFO);
		NETuint8_t(&player);
		NETuint32_t(&structId);
		NETuint8_t(&structureInfo);
		if (structureInfo == STRUCTUREINFO_MANUFACTURE)
		{
			NETuint32_t(&templateId);
			if (templateId != 0)
			{
				psTempl = IdToTemplate(templateId, player);
				if (psTempl == NULL)
				{
					debug(LOG_SYNC, "Synch error, don't have tempate id %u, so can't change production of factory %u!", templateId, structId);
				}
			}
		}
	NETend();

	psStruct = IdToStruct(structId, player);

	syncDebug("player%d,structId%u%c,structureInfo%u,templateId%u%c", player, structId, psStruct == NULL? '^' : '*', structureInfo, templateId, psTempl == NULL? '^' : '*');

	if (psStruct == NULL)
	{
		debug(LOG_SYNC, "Couldn't find structure %u to change production.", structId);
		return;
	}

	CHECK_STRUCTURE(psStruct);

	if (StructIsFactory(psStruct))
	{
		if (psStruct->pFunctionality->factory.pendingCount == 0)
		{
			++psStruct->pFunctionality->factory.pendingCount;
		}
		if (--psStruct->pFunctionality->factory.pendingCount == 0)
		{
			// Subject is now synchronised, remove pending.
			psStruct->pFunctionality->factory.psSubjectPending = NULL;
			psStruct->pFunctionality->factory.statusPending = FACTORY_NOTHING_PENDING;
		}
	}

	syncDebugStructure(psStruct, '<');

	switch (structureInfo)
	{
		case STRUCTUREINFO_MANUFACTURE:       structSetManufacture(psStruct, psTempl, ModeImmediate); break;
		case STRUCTUREINFO_CANCELPRODUCTION:  cancelProduction(psStruct, ModeImmediate);              break;
		case STRUCTUREINFO_HOLDPRODUCTION:    holdProduction(psStruct, ModeImmediate);                break;
		case STRUCTUREINFO_RELEASEPRODUCTION: releaseProduction(psStruct, ModeImmediate);             break;
		case STRUCTUREINFO_HOLDRESEARCH:      holdResearch(psStruct, ModeImmediate);                  break;
		case STRUCTUREINFO_RELEASERESEARCH:   releaseResearch(psStruct, ModeImmediate);               break;
		default:
			debug(LOG_ERROR, "Invalid structureInfo %d", structureInfo);
	}

	syncDebugStructure(psStruct, '>');

	CHECK_STRUCTURE(psStruct);
}
