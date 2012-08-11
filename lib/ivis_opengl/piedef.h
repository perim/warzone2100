/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
/***************************************************************************/
/*
 * piedef.h
 *
 * type defines for all pumpkin image library functions.
 *
 */
/***************************************************************************/

#ifndef _piedef_h
#define _piedef_h

/***************************************************************************/

#include "lib/framework/frame.h"
#include "lib/framework/vector.h"
#include "ivisdef.h"
#include "pietypes.h"

/***************************************************************************/
/*
 *	Global Definitions (STRUCTURES)
 */
/***************************************************************************/

struct PIELIGHTBYTES
{
	uint8_t r, g, b, a;
};

/** Our basic colour type. Use whenever you want to define a colour.
 *  Set bytes separetely, and do not assume a byte order between the components. */
union PIELIGHT
{
	PIELIGHTBYTES byte;
	UDWORD rgba;
	UBYTE vector[4];
};
struct PIERECT  ///< Screen rectangle.
{
	SWORD x, y, w, h;
};
struct PIEIMAGE  ///< An area of texture.
{
	SDWORD texPage;
	SWORD tu, tv, tw, th;
	float invTextureSize;
};

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
extern void pie_Draw3DShape(iIMDShape *shape, int frame, int team, PIELIGHT colour, int pieFlag, int pieFlagData);
extern void pie_DrawImage(const PIEIMAGE *image, const PIERECT *dest);
void pie_DrawImage(const PIEIMAGE *image, const PIERECT *dest, PIELIGHT colour);

extern void pie_GetResetCounts(unsigned int* pPieCount, unsigned int* pPolyCount, unsigned int* pStateCount);

/** Setup stencil shadows and OpenGL lighting. */
void pie_BeginLighting(const Vector3f * light, bool drawshadows);

/* Stop using stencil shadows and OpenGL lighting (if enabled). */
void pie_EndLighting(void);

void pie_Lighting0(LIGHTING_TYPE entry, float value[4]);

void pie_RemainingPasses(void);

void pie_SetUp(void);
void pie_CleanUp(void);

#endif // _piedef_h
