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
/**
 * @file
 *
 * Warzone audio wrapper functions.
 */

#include "lib/framework/frame.h"
#include "lib/sound/aud.h"
#include "lib/sound/tracklib.h"

#include "display3d.h"
#include "map.h"

BOOL audio_ObjectDead(void * psObj)
{
	SIMPLE_OBJECT * const psSimpleObj = (SIMPLE_OBJECT *) psObj;

	/* check is valid simple object pointer */
	if (psSimpleObj == NULL)
	{
		debug( LOG_NEVER, "audio_ObjectDead: simple object pointer invalid" );
		return true;
	}

	/* check projectiles */
	if (psSimpleObj->type == OBJ_PROJECTILE)
	{
		PROJECTILE * const psProj = (PROJECTILE *) psSimpleObj;

		return (psProj->state == PROJ_POSTIMPACT);
	}
	else
	{
		/* check base object */
		BASE_OBJECT *psBaseObj  = (BASE_OBJECT *) psObj;

		return psBaseObj->died;
	}
}
// @FIXME we don't need to do this, since we are not using qsound.

Vector3f audio_GetPlayerPos(void)
{
	Vector3f pos;
	// Player's Y and Z interchanged
	// @NOTE Why?
	pos.x = player.p.x + world_coord(visibleTiles.x / 2);
	pos.y = player.p.z + world_coord(visibleTiles.x / 2);
	pos.z = player.p.y;

	// Invert Y to match QSOUND axes
	// @NOTE What is QSOUND? Why invert the Y axis?
	pos.y = world_coord(GetHeightOfMap()) - pos.y;

	return pos;
}

/**
 * get the angle, and convert it from fixed point PSX crap to a float and then convert that to radians
 */
void audio_Get3DPlayerRotAboutVerticalAxis(float *angle)
{
	*angle = ((float) player.r.y / DEG_1)* M_PI / 180.0f;
}

/**
 * Get QSound axial position from world (x,y)
@FIXME we don't need to do this, since we are not using qsound.
 */
void audio_GetStaticPos(SDWORD iWorldX, SDWORD iWorldY, SDWORD *piX, SDWORD *piY, SDWORD *piZ)
{
	*piX = iWorldX;
	*piZ = map_TileHeight(map_coord(iWorldX), map_coord(iWorldY));
	/* invert y to match QSOUND axes */
	*piY = world_coord(GetHeightOfMap()) - iWorldY;
}

// @FIXME we don't need to do this, since we are not using qsound.
void audio_GetObjectPos(void *psObj, SDWORD *piX, SDWORD *piY, SDWORD *piZ)
{
	BASE_OBJECT	*psBaseObj = (BASE_OBJECT *) psObj;

	/* check is valid pointer */
	ASSERT( psBaseObj != NULL,
			"audio_GetObjectPos: game object pointer invalid" );

	*piX = psBaseObj->pos.x;
	*piZ = map_TileHeight(map_coord(psBaseObj->pos.x), map_coord(psBaseObj->pos.y));

	/* invert y to match QSOUND axes */
	*piY = world_coord(GetHeightOfMap()) - psBaseObj->pos.y;
}

UDWORD sound_GetGameTime()
{
	return gameTime;
}
