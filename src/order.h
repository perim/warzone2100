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
/** @file
 *  Function prototypes for giving droids orders
 */

#ifndef __INCLUDED_SRC_ORDER_H__
#define __INCLUDED_SRC_ORDER_H__

#include "orderdef.h"
#include "droiddef.h"
#include "structuredef.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

//call this *AFTER* every mission so it gets reset
extern void initRunData(void);

/* Update a droids order state */
extern void orderUpdateDroid(DROID *psDroid);

/* Give a droid an order */
extern void orderDroid(DROID *psDroid, DROID_ORDER order, QUEUE_MODE mode);

/* Check the order state of a droid */
extern BOOL orderState(DROID *psDroid, DROID_ORDER order);

/** Check if an order is valid for a location. */
BOOL validOrderForLoc(DROID_ORDER order);

/** Check if an order is valid for an object. */
BOOL validOrderForObj(DROID_ORDER order);

/* Give a droid an order with a location target */
void orderDroidLoc(DROID *psDroid, DROID_ORDER order, UDWORD x, UDWORD y, QUEUE_MODE mode);

/* Get the state of a droid order with a location */
extern BOOL orderStateLoc(DROID *psDroid, DROID_ORDER order, UDWORD *pX, UDWORD *pY);

/* Give a droid an order with an object target */
void orderDroidObj(DROID *psDroid, DROID_ORDER order, BASE_OBJECT *psObj, QUEUE_MODE mode);

/* Get the state of a droid order with an object */
extern BASE_OBJECT* orderStateObj(DROID *psDroid, DROID_ORDER order);

/* Give a droid an order with a location and a stat */
void orderDroidStatsLocDir(DROID *psDroid, DROID_ORDER order, BASE_STATS *psStats, UDWORD x, UDWORD y, uint16_t direction, QUEUE_MODE mode);

/* Get the state of a droid order with a location and a stat */
extern BOOL orderStateStatsLoc(DROID *psDroid, DROID_ORDER order,
						BASE_STATS **ppsStats, UDWORD *pX, UDWORD *pY);

/* Give a droid an order with a location and a stat */
void orderDroidStatsTwoLocDir(DROID *psDroid, DROID_ORDER order, BASE_STATS *psStats, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, uint16_t direction, QUEUE_MODE mode);

void orderDroidStatsTwoLocDirAdd(DROID *psDroid, DROID_ORDER order, BASE_STATS *psStats, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, uint16_t direction);

/* Give selected droids an order with a location target */
// Only called from UI.
extern void orderSelectedLoc(uint32_t player, uint32_t x, uint32_t y, bool add);

/* Give selected droids an order with an object target */
extern void orderSelectedObj(UDWORD player, BASE_OBJECT *psObj);
extern void orderSelectedObjAdd(UDWORD player, BASE_OBJECT *psObj, BOOL add);

// add an order to a droids order list
extern void orderDroidAdd(DROID *psDroid, struct _droid_order_data *psOrder);
// do the next order from a droids order list
extern BOOL orderDroidList(DROID *psDroid);

/* order all selected droids with a location and a stat */
void orderSelectedStatsLocDir(UDWORD player, DROID_ORDER order, BASE_STATS *psStats, UDWORD x, UDWORD y, uint16_t direction, BOOL add);

/* add an order with a location and a stat to the droids order list*/
void orderDroidStatsLocDirAdd(DROID *psDroid, DROID_ORDER order, BASE_STATS *psStats, UDWORD x, UDWORD y, uint16_t direction);

/* order all selected droids with two a locations and a stat */
void orderSelectedStatsTwoLocDir(UDWORD player, DROID_ORDER order, BASE_STATS *psStats, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, uint16_t direction, BOOL add);

// see if a droid supports a secondary order
extern BOOL secondarySupported(DROID *psDroid, SECONDARY_ORDER sec);

// get the state of a secondary order, return false if unsupported
extern SECONDARY_STATE secondaryGetState(DROID *psDroid, SECONDARY_ORDER sec);

// set the state of a secondary order, return false if failed.
extern BOOL secondarySetState(DROID *psDroid, SECONDARY_ORDER sec, SECONDARY_STATE State);

// check the damage level of a droid against it's secondary state
extern void secondaryCheckDamageLevel(DROID *psDroid);

// make all the members of a numeric group have the same secondary states
extern void secondarySetAverageGroupState(UDWORD player, UDWORD group);

// do a moral check for a player
extern void orderMoralCheck(UDWORD player);

// do a moral check for a group
extern void orderGroupMoralCheck(struct _droid_group *psGroup);

extern const char* getDroidOrderName(DROID_ORDER order);

extern DROID *FindATransporter(unsigned player);

/*For a given constructor droid, check if there are any damaged buildings within
a defined range*/
extern BASE_OBJECT * checkForDamagedStruct(DROID *psDroid, STRUCTURE *psTarget);


// do a health check for a droid
extern void orderHealthCheck(DROID *psDroid);

// set the state of a secondary order for a Factory, return false if failed.
extern BOOL setFactoryState(STRUCTURE *psStruct, SECONDARY_ORDER sec, SECONDARY_STATE State);
// get the state of a secondary order for a Factory, return false if unsupported
extern BOOL getFactoryState(STRUCTURE *psStruct, SECONDARY_ORDER sec, SECONDARY_STATE *pState);

//lasSat structure can select a target
extern void orderStructureObj(UDWORD player, BASE_OBJECT *psObj);

static inline void setDroidOrderTarget(DROID *psDroid, void *psNewObject, SDWORD idx)
{
	assert(idx >= 0 && idx < ORDER_LIST_MAX);
	psDroid->asOrderList[idx].psOrderTarget = psNewObject;
}

static inline void removeDroidOrderTarget(DROID *psDroid, SDWORD idx)
{
	assert(idx >= 0 && idx < ORDER_LIST_MAX);
	psDroid->asOrderList[idx].psOrderTarget = NULL;
}

extern DROID_ORDER chooseOrderLoc(DROID *psDroid, UDWORD x,UDWORD y, BOOL altOrder);
extern DROID_ORDER chooseOrderObj(DROID *psDroid, BASE_OBJECT *psObj, BOOL altOrder);

extern void orderDroidBase(DROID *psDroid, DROID_ORDER_DATA *psOrder);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_SRC_ORDER_H__
