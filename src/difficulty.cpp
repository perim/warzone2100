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
/* Simple short file - only because there was nowhere else for it logically to go */
/**
 * @file difficulty.c
 * Handes the difficulty level effects on gameplay.
 */


/*
	Changed to allow seperate modifiers for enemy and player damage.
*/

#include "lib/framework/frame.h"
#include "difficulty.h"
#include "lib/framework/math_ext.h"

// ------------------------------------------------------------------------------------

static DIFFICULTY_LEVEL	presDifLevel = DL_NORMAL;
static float		fDifPlayerModifier;
static float		fDifEnemyModifier;


// ------------------------------------------------------------------------------------
/* Sets the game difficulty level */
void	setDifficultyLevel(DIFFICULTY_LEVEL lev)
{

	switch(lev)
	{
	case	DL_EASY:
		fDifPlayerModifier = 120.f / 100.f;
		fDifEnemyModifier = 100.f / 100.f;
		break;
	case	DL_NORMAL:
		fDifPlayerModifier = 100.f / 100.f;
		fDifEnemyModifier = 100.f / 100.f;
		break;
	case	DL_HARD:
		fDifPlayerModifier = 80.f / 100.f;
		fDifEnemyModifier = 100.f / 100.f;
		break;
	case	DL_KILLER:
		fDifPlayerModifier = 999.f / 100.f;	// 10 times
		fDifEnemyModifier = 1.f / 100.f;		// almost nothing
		break;
	case	DL_TOUGH:
		fDifPlayerModifier = 100.f / 100.f;
		fDifEnemyModifier = 50.f / 100.f;	// they do less damage!
		break;
	default:
		debug( LOG_ERROR, "Invalid difficulty level selected - forcing NORMAL" );
		fDifPlayerModifier = 100.f / 100.f;
		fDifEnemyModifier = 100.f / 100.f;
		lev = DL_NORMAL;
		break;
	}

	presDifLevel = lev;
}

// ------------------------------------------------------------------------------------
/* Returns the difficulty level */
DIFFICULTY_LEVEL	getDifficultyLevel( void )
{
	return(presDifLevel);
}

// ------------------------------------------------------------------------------------
int modifyForDifficultyLevel(int basicVal, bool IsPlayer)
{
	if (IsPlayer)
		return roundf(basicVal * fDifPlayerModifier);
	else
		return roundf(basicVal * fDifEnemyModifier);
}
// ------------------------------------------------------------------------------------
