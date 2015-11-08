/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
/**
 *
 * @file
 * Order releated structures.
 *
 */

#ifndef __INCLUDED_SRC_ORDERDEF_H__
#define __INCLUDED_SRC_ORDERDEF_H__

#include "lib/framework/vector.h"

#include "basedef.h"

/** All the possible droid orders.
 * @todo DORDER_CIRCLE = 40 which is not consistent with rest of the enum.
 */
enum DroidOrderType
{
	DORDER_NONE,            /**< no order set. */

	DORDER_STOP,            /**< stop the current order. */
	DORDER_MOVE,            /**< move to a location. */
	DORDER_ATTACK,          /**< attack an enemy. */
	DORDER_BUILD,           /**< build a structure. */
	DORDER_HELPBUILD,       /**< help to build a structure. */
	DORDER_LINEBUILD,       /**< build a number of structures in a row (walls + bridges). */
	DORDER_DEMOLISH,        /**< demolish a structure. */
	DORDER_REPAIR,          /**< repair a structure. */
	DORDER_OBSERVE,         /**< keep a target in sensor view. */
	DORDER_FIRESUPPORT,     /**< attack whatever the linked sensor droid attacks. */
	DORDER_RETREAT,         /**< return to the players retreat position. */
	DORDER_RTB,             /**< return to base. */
	DORDER_RTR,             /**< return to repair at any repair facility. */
	DORDER_RUN,             /**< run away after moral failure. */
	DORDER_EMBARK,          /**< board a transporter. */
	DORDER_DISEMBARK,       /**< get off a transporter. */
	DORDER_ATTACKTARGET,    /**< a suggestion to attack something i.e. the target was chosen because the droid could see it. */
	DORDER_BUILDMODULE,     /**< build a module (power, research or factory). */
	DORDER_RECYCLE,         /**< return to factory to be recycled. */
	DORDER_TRANSPORTOUT,    /**< offworld transporter order. */
	DORDER_TRANSPORTIN,     /**< onworld transporter order. */
	DORDER_TRANSPORTRETURN, /**< transporter return after unloading. */
	DORDER_GUARD,           /**< guard a structure. */
	DORDER_RESTORE,         /**< restore resistance points for a structure. */
	DORDER_SCOUT,           /**< same as move, but stop if an enemy is seen. */
	DORDER_RUNBURN,         /**< run away on fire. */
	DORDER_PATROL,          /**< move between two way points. */
	DORDER_REARM,           /**< order a vtol to rearming pad. */
	DORDER_RECOVER,         /**< pick up an artifact. */
	DORDER_LEAVEMAP,        /**< vtol flying off the map. */
	DORDER_RTR_SPECIFIED,   /**< return to repair at a specified repair center. */
	DORDER_CIRCLE,          /**< circles target location and engage. */
	DORDER_HOLD,            /**< hold position until given next order. */
};
typedef DroidOrderType DROID_ORDER;

/** All the possible secondary orders for droids. */
enum SECONDARY_ORDER
{
	DSO_REPAIR_LEVEL,               /**< The repair level at which the droid falls back to repair: can be low, high or never. Used with DSS_REPLEV_LOW, DSS_REPLEV_HIGH, DSS_REPLEV_NEVER. */
	DSO_ATTACK_LEVEL,               /**< The attack level at which a droid can attack: can be always or never. Used with DSS_ALEV_ALWAYS or DSS_ALEV_NEVER. */
	DSO_RECYCLE,                    /**< If can be recycled or not. */
	DSO_RETURN_TO_LOC,              /**< Generic secondary order to return to a location. Will depend on the secondary state DSS_RTL* to be specific. */
};

/** All associated secondary states of the secondary orders. */
enum SECONDARY_STATE
{
	DSS_NONE            = 0x000000,	/**< no state. */
	DSS_REPLEV_LOW      = 0x000004,	/**< state referred to secondary order DSO_REPAIR_LEVEL. Droid falls back if its health decrease below 25%. */
	DSS_REPLEV_HIGH     = 0x000008,	/**< state referred to secondary order DSO_REPAIR_LEVEL. Droid falls back if its health decrease below 50%. */
	DSS_REPLEV_NEVER    = 0x00000c,	/**< state referred to secondary order DSO_REPAIR_LEVEL. Droid never falls back. */
	DSS_ALEV_ALWAYS     = 0x000010,	/**< state referred to secondary order DSO_ATTACK_LEVEL. Droid attacks by its free will everytime. */
	DSS_ALEV_NEVER      = 0x000030,	/**< state referred to secondary order DSO_ATTACK_LEVEL. Droid never attacks. */
	DSS_RECYCLE_SET     = 0x000100,	/**< state referred to secondary order DSO_RECYCLE. If set, the droid can be recycled. */
	DSS_RTL_REPAIR      = 0x080000,	/**< state set to send order DORDER_RTR to droid. */
	DSS_RTL_BASE        = 0x100000,	/**< state set to send order DORDER_RTB to droid. */
	DSS_RTL_TRANSPORT   = 0x200000,	/**< state set to send order DORDER_EMBARK to droid. */
};

/** masks for the secondary order state. */
#define DSS_REPLEV_MASK             0x00000c
#define DSS_ALEV_MASK               0x000030
#define DSS_RECYCLE_MASK            0x000100
#define DSS_RTL_MASK                0x380000

/** struct used to store the data for retreating. */
struct RUN_DATA
{
	Vector2i    sPos;       /**< position to where units should flee to. */
	uint8_t     forceLevel; /**< number of units below which others might flee. */
	uint8_t     healthLevel;/**< health percentage value below which it might flee. This value is used for groups only. */
	uint8_t     leadership; /**< basic value that will be used on calculations of the flee probability. */
};

struct STRUCTURE_STATS;

/** Struct that stores data of an order.
 * This struct is needed to send orders that comes with information, such as position, target, etc.
 * This struct is used to issue orders to droids.
 */
struct DroidOrder
{
	explicit DroidOrder(DroidOrderType type = DORDER_NONE)
		: type(type), pos(0, 0), pos2(0, 0), direction(0),         index(0),     psObj(NULL),  psStats(NULL)    {}
	DroidOrder(DroidOrderType type, Vector2i pos)
		: type(type), pos(pos),  pos2(0, 0), direction(0),         index(0),     psObj(NULL),  psStats(NULL)    {}
	DroidOrder(DroidOrderType type, STRUCTURE_STATS *psStats, Vector2i pos, uint16_t direction)
		: type(type), pos(pos),  pos2(0, 0), direction(direction), index(0),     psObj(NULL),  psStats(psStats) {}
	DroidOrder(DroidOrderType type, STRUCTURE_STATS *psStats, Vector2i pos, Vector2i pos2, uint16_t direction)
		: type(type), pos(pos),  pos2(pos2), direction(direction), index(0),     psObj(NULL),  psStats(psStats) {}
	DroidOrder(DroidOrderType type, BASE_OBJECT *psObj)
		: type(type), pos(0, 0), pos2(0, 0), direction(0),         index(0),     psObj(psObj), psStats(NULL)    {}
	DroidOrder(DroidOrderType type, BASE_OBJECT *psObj, uint32_t index)
		: type(type), pos(0, 0), pos2(0, 0), direction(0),         index(index), psObj(psObj), psStats(NULL)    {}

	DroidOrderType   type;       /**< the actual order. */
	Vector2i         pos;        /**< the order's position. */
	Vector2i         pos2;       /**< the order's second position, in case those exist. */
	uint16_t         direction;  /**< the order's direction, in case it exist. */
	uint32_t         index;      ///< Module index, with DORDER_BUILDMODULE.
	BASE_OBJECT     *psObj;      /**< the order's target, in case it exist. */
	STRUCTURE_STATS *psStats;    /**< order structure stats. */
};

typedef DroidOrder DROID_ORDER_DATA;

#endif // __INCLUDED_SRC_ORDERDEF_H__
