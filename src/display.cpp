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
 * @file display.c
 *
 * Display routines.
 *
 */

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/strres.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/framework/fixedpoint.h"
#include "lib/framework/wzapp.h"

#include "action.h"
#include "display.h"
#include "map.h"
#include "loop.h"
#include "component.h"
#include "display3d.h"
#include "lib/framework/cursors.h"
#include "hci.h"
#include "text.h"
#include "edit3d.h"
#include "geometry.h"
#include "lib/gamelib/gtime.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "radar.h"
#include "miscimd.h"
#include "lib/framework/math_ext.h"
#include "console.h"
#include "order.h"
#include "wrappers.h"
#include "power.h"
#include "map.h"
#include "keymap.h"
#include "intimage.h"
#include "mechanics.h"
#include "lighting.h"
#include "ingameop.h"
#include "oprint.h"
#include "warcam.h"
#include "keybind.h"
#include "keymap.h"
#include "projectile.h"
#include "message.h"
#include "effects.h"
#include "selection.h"
#include "transporter.h"
#include "intorder.h"
#include "multiplay.h"
#include "qtscript.h"
#include "warzoneconfig.h"

struct	_dragBox dragBox3D, wallDrag;

extern char DROIDDOING[512];		// holds the string on what the droid is doing

/// acceleration on scrolling. Game Option.
UDWORD	scroll_speed_accel;

/// Control zoom. Add an amount to zoom this much each second.
static float zoom_speed = 0.0f;
static float zoom_target = 0.0f;

static bool bInvertMouse = true;
static bool bMiddleClickRotate = false;
static bool bDrawShadows = true;
static void dealWithLMB(BASE_OBJECT *psClickedOn, CURSOR cursor, std::list<DROID *> droids);
static void dealWithLMBDClick(BASE_OBJECT *psClickedOn, CURSOR cursor, std::list<DROID *> droids);
static void dealWithRMB(BASE_OBJECT *psClickedOn, CURSOR cursor, std::list<DROID *> droids);
static bool mouseInBox(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1);
static OBJECT_POSITION *checkMouseLoc(void);

void finishDeliveryPosition(void);

static bool	bInstantRadarJump = false;
static SDWORD	desiredPitch = 340;
static UDWORD	currentFrame;
static UDWORD StartOfLastFrame;
static SDWORD	rotX;
static SDWORD	rotY;
static UDWORD	rotInitial;
static UDWORD	rotInitialUp;
static UDWORD	xMoved, yMoved;
static uint32_t scrollRefTime;
static float	scrollSpeedLeftRight; //use two directions and add them because its simple
static float	scrollStepLeftRight;
static float	scrollSpeedUpDown;
static float	scrollStepUpDown;
static bool	mouseOverRadar = false;
static bool	mouseOverConsole = false;
static bool	ignoreRMBC	= true;
static bool bRadarDragging = false;
static bool mouseScroll = true;

bool	rotActive = false;
bool	gameStats = false;

// Local prototypes
static BASE_OBJECT *itemUnderMouse();

float getZoom()
{
	return zoom_target;
}

float getZoomSpeed()
{
	return fabsf(zoom_speed);
}

void setZoom(float zoomSpeed, float zoomTarget)
{
	float zoom_origin = getViewDistance();
	zoom_speed = zoomSpeed;
	zoom_target = zoomTarget;
	zoom_speed *= zoom_target > zoom_origin ? 1 : -1; // get direction
}

void zoom()
{
	if (zoom_speed != 0.0f)
	{
		float distance = getViewDistance();
		distance += graphicsTimeAdjustedIncrement(zoom_speed);
		if ((zoom_speed > 0.0f && distance > zoom_target) || (zoom_speed <= 0.0f && distance < zoom_target))
		{
			distance = zoom_target; // reached target
			zoom_speed = 0.0f;
		}
		setViewDistance(distance);
		UpdateFogDistance(distance);
	}
}

bool isMouseOverRadar()
{
	return mouseOverRadar;
}

void setMouseScroll(bool scroll)
{
	mouseScroll = scroll;
}

void	setRadarJump(bool	val)
{
	bInstantRadarJump = val;
}

bool	getRadarJumpStatus(void)
{
	return (bInstantRadarJump);
}

bool	getInvertMouseStatus(void)
{
	return (bInvertMouse);
}

void	setInvertMouseStatus(bool val)
{
	bInvertMouse = val;
}

#define MOUSE_ORDER (MOUSE_LMB)
#define MOUSE_SELECT (MOUSE_RMB)
#define MOUSE_ROTATE (bMiddleClickRotate?MOUSE_MMB:MOUSE_RMB)

bool	getRightClickOrders(void)
{
	return false; // TBD
}

void	setRightClickOrders(bool val)
{
	// TBD
}

bool	getMiddleClickRotate(void)
{
	return bMiddleClickRotate;
}

void	setMiddleClickRotate(bool val)
{
	bMiddleClickRotate = val;
}

bool	getDrawShadows(void)
{
	return bDrawShadows;
}

void	setDrawShadows(bool val)
{
	bDrawShadows = val;
	pie_setShadows(val);
}

void ProcessRadarInput()
{
	int PosX, PosY;
	int x = mouseX();
	int y = mouseY();
	UDWORD	temp1, temp2;

	/* Only allow jump-to-area-of-map if radar is on-screen */
	mouseOverRadar = false;
	if (radarOnScreen && radarPermitted)
	{
		if (CoordInRadar(x, y))
		{
			mouseOverRadar = true;

			if (mousePressed(MOUSE_ORDER) || (mousePressed(MOUSE_MMB) && keyDown(KEY_LALT)))
			{
				if (mousePressed(MOUSE_ORDER))
				{
					x = mousePressPos_DEPRECATED(MOUSE_ORDER).x;
					y = mousePressPos_DEPRECATED(MOUSE_ORDER).y;
				}
				else
				{
					x = mousePressPos_DEPRECATED(MOUSE_MMB).x;
					y = mousePressPos_DEPRECATED(MOUSE_MMB).y;
				}

				/* If we're tracking a droid, then cancel that */
				CalcRadarPosition(x, y, &PosX, &PosY);
				if (mouseOverRadar)	// Send all droids to that location
				{
					for (DROID *psDroid : selectedDroidList())
					{
						if (psDroid->droidType != DROID_TRANSPORTER || game.type == CAMPAIGN)
						{
							const int x = (PosX * TILE_UNITS) + TILE_UNITS / 2;
							const int y = (PosY * TILE_UNITS) + TILE_UNITS / 2;
							orderDroidLoc(psDroid, DORDER_MOVE, x, y, ModeQueue, ctrlShiftDown());
						}
					}
				}
				CheckScrollLimits();
				audio_PlayTrack(ID_SOUND_MESSAGEEND);
			}


			if (mouseDrag(MOUSE_SELECT, &temp1, &temp2) && !rotActive)
			{
				CalcRadarPosition(x, y, &PosX, &PosY);
				setViewPos(PosX, PosY, true);
				bRadarDragging = true;
				if (ctrlShiftDown())
				{
					player.r.y = 0;
				}
			}
			else if (mousePressed(MOUSE_SELECT))
			{
				x = mousePressPos_DEPRECATED(MOUSE_SELECT).x;
				y = mousePressPos_DEPRECATED(MOUSE_SELECT).y;

				CalcRadarPosition(x, y, &PosX, &PosY);

				if (bInstantRadarJump)
				{
					/* Go instantly */
					setViewPos(PosX, PosY, true);
				}
				else
				{
					/* Pan to it */
					requestRadarTrack(PosX * TILE_UNITS, PosY * TILE_UNITS);
				}
			}
			// ctrl-alt-scroll changes game speed
			if (!keyDown(KEY_LCTRL) || !keyDown(KEY_LALT))
			{
				if (mousePressed(MOUSE_WUP))
				{
					kf_RadarZoomIn();
				}
				else if (mousePressed(MOUSE_WDN))
				{
					kf_RadarZoomOut();
				}
			}
		}
	}
}

// reset the input state
void resetInput(void)
{
	rotActive = false;
	dragBox3D.status = DRAG_INACTIVE;
	wallDrag.status = DRAG_INACTIVE;
}

/* Process the user input. This just processes the key input and jumping around the radar*/
void processInput(void)
{
	bool mOverConstruction = false;

	if (InGameOpUp || isInGamePopupUp)
	{
		dragBox3D.status = DRAG_RELEASED;	// disengage the dragging since it stops menu input
	}

	if (CoordInBuild(mouseX(), mouseY()))
	{
		mOverConstruction = true;
	}

	StartOfLastFrame = currentFrame;
	currentFrame = frameGetFrameNumber();
	ignoreRMBC = false;

	/* Process all of our key mappings */
	mouseOverConsole = mouseOverHistoryConsoleBox();
	if (mousePressed(MOUSE_WUP) && !isMouseOverRadar())
	{
		/* Ctrl+Alt+WheelUp makes game speed up */
		if (keyDown(KEY_LCTRL) && keyDown(KEY_LALT))
		{
			kf_SpeedUp();
		}
		else if (mOverConstruction)
		{
			kf_BuildPrevPage();
		}
		else if (!mouseOverConsole)
		{
			kf_ZoomInStep();
		}
	}
	else if (mousePressed(MOUSE_WDN) && !isMouseOverRadar())
	{
		/* Ctrl+Alt+WheelDown makes game slow down */
		if (keyDown(KEY_LCTRL) && keyDown(KEY_LALT))
		{
			kf_SlowDown();
		}
		else if (mOverConstruction)
		{
			kf_BuildNextPage();
		}
		else if (!mouseOverConsole)
		{
			kf_ZoomOutStep();
		}
	}

	keyProcessMappings();

	/* Allow the user to clear the (Active) console if need be */
	if (mouseOverConsoleBox() && mousePressed(MOUSE_LMB))
	{
		clearActiveConsole();
	}
}

static bool OverRadarAndNotDragging(void)
{
	return mouseOverRadar && dragBox3D.status != DRAG_DRAGGING && radarPermitted && wallDrag.status != DRAG_DRAGGING;
}

static void CheckFinishedDrag(void)
{
	if (mouseReleased(MOUSE_LMB) || mouseDown(MOUSE_RMB))
	{
		selectAttempt = false;
		if (dragBox3D.status == DRAG_DRAGGING)
		{
			if (wallDrag.status == DRAG_DRAGGING)
			{
				//if invalid location keep looking for a valid one
				if ((buildState == BUILD3D_VALID || buildState == BUILD3D_FINISHED)
				    && sBuildDetails.psStats->ref >= REF_STRUCTURE_START
				    && sBuildDetails.psStats->ref < (REF_STRUCTURE_START + REF_RANGE))
				{
					if ((((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_WALL
					     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_DEFENSE
					     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_GATE
					     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_REARM_PAD)
					    && !isLasSat((STRUCTURE_STATS *)sBuildDetails.psStats))
					{
						wallDrag.x2 = mouseTileX;
						wallDrag.y2 = mouseTileY;
						wallDrag.status = DRAG_RELEASED;
					}
				}
			}

			/* Only clear if shift isn't down - this is for the drag selection box for units*/
			if (!ctrlShiftDown() && wallDrag.status == DRAG_INACTIVE)
			{
				clearSelection();
			}
			dragBox3D.status = DRAG_RELEASED;
			dragBox3D.x2 = mouseX();
			dragBox3D.y2 = mouseY();
		}
		else
		{
			dragBox3D.status = DRAG_INACTIVE;
			wallDrag.status = DRAG_INACTIVE;
		}
	}
}

static void CheckStartWallDrag(void)
{
	if (mousePressed(MOUSE_LMB))
	{
		/* Store away the details if we're building */
		// You can start dragging walls from invalid locations so check for
		// BUILD3D_POS or BUILD3D_VALID, used tojust check for BUILD3D_VALID.
		if ((buildState == BUILD3D_POS || buildState == BUILD3D_VALID)
		    && sBuildDetails.psStats->ref >= REF_STRUCTURE_START
		    && sBuildDetails.psStats->ref < (REF_STRUCTURE_START + REF_RANGE))
		{
			if ((((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_WALL
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_DEFENSE
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_GATE
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_REARM_PAD)
			    && !isLasSat((STRUCTURE_STATS *)sBuildDetails.psStats))
			{
				wallDrag.x1 = wallDrag.x2 = mouseTileX;
				wallDrag.y1 = wallDrag.y2 = mouseTileY;
				wallDrag.status = DRAG_PLACING;
				debug(LOG_NEVER, "Start Wall Drag\n");
			}
		}
		else if (intBuildSelectMode())//if we were in build select mode
		{
			//uhoh no place to build here
			audio_PlayTrack(ID_SOUND_BUILD_FAIL);
		}
	}
}

//this function is called when a location has been chosen to place a structure or a DP
static bool CheckFinishedFindPosition(void)
{
	bool OverRadar = OverRadarAndNotDragging();

	/* Do not let the player position buildings 'under' the radar */
	if (mouseReleased(MOUSE_LMB) && !OverRadar)
	{

		if (deliveryReposValid())
		{
			finishDeliveryPosition();
			return true;
		}
		else if (buildState == BUILD3D_VALID)
		{
			if ((((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_WALL
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_GATE
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_REARM_PAD
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_DEFENSE)
			    && sBuildDetails.psStats->ref >= REF_STRUCTURE_START
			    && sBuildDetails.psStats->ref < (REF_STRUCTURE_START + REF_RANGE)
			    && !isLasSat((STRUCTURE_STATS *)sBuildDetails.psStats))
			{
				int dx, dy;

				wallDrag.x2 = mouseTileX;
				wallDrag.y2 = mouseTileY;

				dx = abs(mouseTileX - wallDrag.x1);
				dy = abs(mouseTileY - wallDrag.y1);

				if (dx >= dy)
				{
					wallDrag.y2 = wallDrag.y1;
				}
				else if (dx < dy)
				{
					wallDrag.x2 = wallDrag.x1;
				}

				wallDrag.status = DRAG_RELEASED;
			}
			debug(LOG_NEVER, "BUILD3D_FINISHED\n");
			buildState = BUILD3D_FINISHED;
			return true;
		}
	}

	return false;
}

static void HandleDrag(void)
{
	UDWORD dragX = 0, dragY = 0;

	if (mouseDrag(MOUSE_LMB, &dragX, &dragY) && !mouseOverRadar && !mouseDown(MOUSE_RMB))
	{
		dragBox3D.x1 = dragX;
		dragBox3D.x2 = mouseX();
		dragBox3D.y1 = dragY;
		dragBox3D.y2 = mouseY();
		dragBox3D.status = DRAG_DRAGGING;

		if (buildState == BUILD3D_VALID)
		{
			if ((((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_WALL
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_GATE
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_DEFENSE
			     || ((STRUCTURE_STATS *)sBuildDetails.psStats)->type == REF_REARM_PAD)
			    && !isLasSat((STRUCTURE_STATS *)sBuildDetails.psStats))
			{
				int dx, dy;

				wallDrag.x2 = mouseTileX;
				wallDrag.y2 = mouseTileY;

				dx = abs(mouseTileX - wallDrag.x1);
				dy = abs(mouseTileY - wallDrag.y1);

				if (dx >= dy)
				{
					wallDrag.y2 = wallDrag.y1;
				}
				else if (dx < dy)
				{
					wallDrag.x2 = wallDrag.x1;
				}

				wallDrag.status = DRAG_DRAGGING;
			}
		}
	}
}

static CURSOR getCursorState(std::list<DROID *> &droids, BASE_OBJECT **ppObjUnderMouse)
{
	if (gamePaused())
	{
		return CURSOR_DEFAULT;
	}
	if (buildState == BUILD3D_VALID)
	{
		// special casing for building
		return CURSOR_BUILD;
	}
	else if (buildState == BUILD3D_POS)
	{
		// special casing for building - can't build here
		return CURSOR_NOTPOSSIBLE;
	}
	else if (dragBox3D.status == DRAG_DRAGGING)
	{
		return CURSOR_DEFAULT;
	}

	BASE_OBJECT *objUnderMouse = *ppObjUnderMouse = itemUnderMouse();

	if (selectedDroidList().size() == 0) // no unit selected
	{
		// when one of the arrow key gets pressed, set cursor appropriately
		if (keyDown(KEY_UPARROW))
		{
			return CURSOR_UARROW;
		}
		else if (keyDown(KEY_DOWNARROW))
		{
			return CURSOR_DARROW;
		}
		else if (keyDown(KEY_LEFTARROW))
		{
			return CURSOR_LARROW;
		}
		else if (keyDown(KEY_RIGHTARROW))
		{
			return CURSOR_RARROW;
		}

		STRUCTURE *psStruct = getLasSat(selectedPlayer);
		if (bMultiPlayer && psStruct)
		{
			if (objUnderMouse && !aiCheckAlliances(selectedPlayer, objUnderMouse->player))
			{
				return CURSOR_ATTACK;
			}
			else if (objUnderMouse) // ally
			{
				return CURSOR_DEFAULT;
			}
			return CURSOR_NOTPOSSIBLE;
		}

		if (objUnderMouse && objUnderMouse->type == OBJ_STRUCTURE && objUnderMouse->player == selectedPlayer)
		{
			return CURSOR_DEFAULT;
		}
	}

	if (mouseTileX < 0 || mouseTileY < 0 || mouseTileX > (int)(mapWidth - 1) || mouseTileY > (int)(mapHeight - 1))
	{
		return CURSOR_EDGEOFMAP;
	}

	// Exceptions

	if (terrainType(mapTile(mouseTileX, mouseTileY)) == TER_CLIFFFACE)
	{
		return CURSOR_NOTPOSSIBLE;
	}

	if (objUnderMouse && objUnderMouse->type == OBJ_STRUCTURE && aiCheckAlliances(selectedPlayer, objUnderMouse->player)
	    && castStructure(objUnderMouse)->pStructureType->type == REF_REPAIR_FACILITY)
	{
		droids = selectedDroidList(SELECTED_ALL);
		return CURSOR_SEEKREPAIR;
	}

	if (bMultiPlayer && selectedDroidList(SELECTED_TRANSPORT).size() > 0)
	{
		droids = selectedDroidList(SELECTED_TRANSPORT);
		return CURSOR_MOVE;
	}

	if (selectedDroidList(SELECTED_CONSTRUCT).size() > 0)
	{
		if (objUnderMouse && objUnderMouse->type == OBJ_FEATURE)
		{
			FEATURE *psFeat = (FEATURE *)objUnderMouse;
			if (psFeat->psStats->subType == FEAT_OIL_RESOURCE)
			{
				// We don't allow the build cursor under certain circumstances ....
				// can't build if res extractors arent available.
				int i;
				for (i = 0; i < numStructureStats && asStructureStats[i].type != REF_RESOURCE_EXTRACTOR; i++) {} // find resource stat
				if (i < numStructureStats && apStructTypeLists[selectedPlayer][i] != AVAILABLE)		// check if you can build it!
				{
					return CURSOR_NOTPOSSIBLE;
				}
				droids = selectedDroidList(SELECTED_CONSTRUCT);
				return CURSOR_BUILD;
			}
		}
		if (!objUnderMouse && (intSelectMode() == SELECT_PATROL || intSelectMode() == SELECT_CIRCLE))
		{
			droids = selectedDroidList(SELECTED_ALL);
			return CURSOR_SIGHT;
		}
	}

	if (objUnderMouse)
	{
		int numWeapDroids = selectedDroidList(SELECTED_WEAPON).size();

		if (objUnderMouse->type == OBJ_STRUCTURE && aiCheckAlliances(selectedPlayer, objUnderMouse->player)
		    && selectedDroidList(SELECTED_CONSTRUCT).size() > 0)
		{
			STRUCTURE *psStruct = castStructure(objUnderMouse);
			if (psStruct->status == SS_BEING_BUILT && psStruct->player == selectedPlayer)
			{
				droids = selectedDroidList(SELECTED_CONSTRUCT);
				return CURSOR_BUILD;
			}
			else if ((psStruct->status == SS_BUILT && intSelectMode() == SELECT_DEMOLISH) || checkDroidsDemolishing(psStruct))
			{
				droids = selectedDroidList(SELECTED_CONSTRUCT);
				return CURSOR_DEST;
			}
			else if (psStruct->status == SS_BUILT && psStruct->body < structureBody(psStruct))
			{
				droids = selectedDroidList(SELECTED_CONSTRUCT);
				return CURSOR_FIX;
			}
			else if (nextModuleToBuild(psStruct, -1) > 0) // module available
			{
				droids = selectedDroidList(SELECTED_CONSTRUCT);
				return CURSOR_BUILD;
			}
			return CURSOR_NOTPOSSIBLE;
		}
		else if (objUnderMouse->type == OBJ_STRUCTURE && aiCheckAlliances(selectedPlayer, objUnderMouse->player) && numWeapDroids > 0)
		{
			// TODO - this is slow, make it faster when we have a good check if structure is a sensor or not
			STRUCTURE *psStruct = castStructure(objUnderMouse);
			droids = selectedDroidList(SELECTED_WEAPON);
			for (auto *psDroid : selectedDroidList(SELECTED_WEAPON))
			{
				if (!structSensorDroidWeapon(psStruct, psDroid))
				{
					droids.remove(psDroid);
				}
			}
			if (droids.size() > 0)
			{
				return CURSOR_ATTACH;
			}
		}
		if (intSelectMode() == SELECT_GUARD && numWeapDroids > 0)
		{
			droids = selectedDroidList(SELECTED_WEAPON);
			if (objUnderMouse->type == OBJ_DROID)
			{
				droids.remove(castDroid(objUnderMouse));
			}
			if (droids.size() == 0)
			{
				return CURSOR_NOTPOSSIBLE;
			}
			return CURSOR_GUARD;
		}
		if (objUnderMouse->type == OBJ_DROID && ((DROID *)objUnderMouse)->droidType == DROID_SENSOR && numWeapDroids > 0)
		{
			droids = selectedDroidList(SELECTED_WEAPON);
			for (auto *psDroid : selectedDroidList(SELECTED_WEAPON))
			{
				if (!droidSensorDroidWeapon(objUnderMouse, psDroid))
				{
					droids.remove(psDroid);
				}
			}
			if (droids.size() > 0)
			{
				return CURSOR_ATTACH;
			}
		}
		if (selectedDroidList(SELECTED_SENSOR).size() > 0 && !aiCheckAlliances(selectedPlayer, objUnderMouse->player)
		    && objUnderMouse->type != OBJ_FEATURE) // features handled specially later
		{
			droids = selectedDroidList(SELECTED_SENSOR);
			return CURSOR_LOCKON;
		}
		else if (objUnderMouse->player == selectedPlayer && (specialOrderKeyDown() || intSelectMode() == SELECT_ATTACK) && numWeapDroids > 0)
		{
			droids = selectedDroidList(SELECTED_WEAPON);
			if (objUnderMouse->type == OBJ_DROID)
			{
				droids.remove(castDroid(objUnderMouse));
			}
			if (droids.size() == 0)
			{
				return CURSOR_NOTPOSSIBLE;
			}
			return CURSOR_ATTACK;
		}
		else if (!aiCheckAlliances(selectedPlayer, objUnderMouse->player) && numWeapDroids > 0)
		{
			droids = selectedDroidList(SELECTED_WEAPON);
			return CURSOR_ATTACK;
		}
		else if (numWeapDroids > 0 && aiCheckAlliances(selectedPlayer, objUnderMouse->player) && objUnderMouse->type == OBJ_STRUCTURE)
		{
			droids = selectedDroidList(SELECTED_ALL);
			return CURSOR_GUARD;
		}
		else if (objUnderMouse->type == OBJ_DROID)
		{
			DROID *psDroid = castDroid(objUnderMouse);
			if (aiCheckAlliances(selectedPlayer, psDroid->player) && selectedDroidList(SELECTED_REPAIR).size() > 0)
			{
				droids = selectedDroidList(SELECTED_REPAIR);
				if (objUnderMouse->type == OBJ_DROID)
				{
					droids.remove(castDroid(objUnderMouse));
				}
				if (droids.size() > 0)
				{
					return CURSOR_FIX;
				}
			}
			if (isTransporter(psDroid) && psDroid->player == selectedPlayer && selectedDroidList(SELECTED_ALL).size() > 0)
			{
				droids = selectedDroidList(SELECTED_ALL);
				for (DROID *psCurr : droids)
				{
					if (isVtolDroid(psCurr) || psCurr->id == psDroid->id) // FIXME - other limits?
					{
						droids.remove(psCurr);
					}
				}
				return CURSOR_EMBARK;
			}
			if (objUnderMouse->player == selectedPlayer)
			{
				if ((keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT)) && selectedDroidList(SELECTED_ALL).size() > 0)
				{
					return CURSOR_SELECT_ADD;
				}
				return CURSOR_SELECT;
			}
		}
		else if (objUnderMouse->type == OBJ_FEATURE)
		{
			FEATURE *psFeat = castFeature(objUnderMouse);
			droids = selectedDroidList(SELECTED_ALL);
			if (psFeat->type == FEAT_GEN_ARTE && droids.size() > 0)
			{
				for (DROID *psCurr : droids)
				{
					if (isVtolDroid(psCurr))
					{
						droids.remove(psCurr);
					}
				}
				if (droids.size() == 0) // only vtols!
				{
					return CURSOR_NOTPOSSIBLE;
				}
				return CURSOR_PICKUP;
			}
			else if (psFeat->psStats->damageable && numWeapDroids > 0)
			{
				droids = selectedDroidList(SELECTED_WEAPON);
				return CURSOR_ATTACK;
			}
			else if (psFeat->psStats->damageable && selectedDroidList(SELECTED_SENSOR).size() > 0)
			{
				droids = selectedDroidList(SELECTED_SENSOR);
				return CURSOR_LOCKON;
			}
		}
		return CURSOR_NOTPOSSIBLE;
	}

	if (!objUnderMouse)
	{
		if ((specialOrderKeyDown() || intSelectMode() == SELECT_DISEMBARK) && bMultiPlayer && selectedDroidList(SELECTED_TRANSPORT).size() > 0)
		{
			droids = selectedDroidList(SELECTED_TRANSPORT);
			return CURSOR_DISEMBARK;
		}
		if ((specialOrderKeyDown() || intSelectMode() == SELECT_ATTACK) && selectedDroidList(SELECTED_WEAPON).size() > 0)
		{
			droids = selectedDroidList(SELECTED_WEAPON);
			return CURSOR_SCOUT;
		}
		// do some continent test here?
		if (selectedDroidList(SELECTED_ALL).size() > 0)
		{
			droids = selectedDroidList(SELECTED_ALL);
			return CURSOR_MOVE;
		}
	}

	return CURSOR_DEFAULT;
}

//don't want to do any of these whilst in the Intelligence Screen
void processMouseClickInput()
{
	bool OverRadar = OverRadarAndNotDragging();
	bool ignoreOrder = CheckFinishedFindPosition();

	CheckStartWallDrag();
	HandleDrag();
	CheckFinishedDrag();

	std::list<DROID *> droids;
	BASE_OBJECT *psTarget = NULL;
	CURSOR cursor = getCursorState(droids, &psTarget);
	wzSetCursor(cursor);

	if ((mouseReleased(MOUSE_LMB) || (mouseReleased(MOUSE_MMB) && (keyDown(KEY_LALT) || keyDown(KEY_RALT)))) && !OverRadar &&
	    dragBox3D.status != DRAG_RELEASED && !ignoreOrder && !mouseOverConsole && !bDisplayMultiJoiningStatus)
	{
		dealWithLMB(psTarget, cursor, droids);
	}

	if (mouseDClicked(MOUSE_LMB))
	{
		dealWithLMBDClick(psTarget, cursor, droids);
	}

	if (mouseReleased(MOUSE_RMB) && !rotActive && !ignoreRMBC)
	{
		dragBox3D.status = DRAG_INACTIVE;
		// Pretty sure we wan't set walldrag status here aswell.
		wallDrag.status = DRAG_INACTIVE;
		bRadarDragging = false;
		dealWithRMB(psTarget, cursor, droids);
		// Why?
		if (getWarCamStatus())
		{
			camToggleStatus();
		}
	}

	if (!mouseDrag(MOUSE_SELECT, (UDWORD *)&rotX, (UDWORD *)&rotY) && bRadarDragging)
	{
		bRadarDragging = false;
	}

	/* Right mouse click kills a building placement */
	if (mouseReleased(MOUSE_RMB) && (buildState == BUILD3D_POS || buildState == BUILD3D_VALID))
	{
		/* Stop the placement */
		kill3DBuilding();
		bRadarDragging = false;
	}
	if (mouseReleased(MOUSE_RMB))
	{
		cancelDeliveryRepos();
	}
	if (mouseDrag(MOUSE_ROTATE, (UDWORD *)&rotX, (UDWORD *)&rotY) && !rotActive && !bRadarDragging)
	{
		rotInitial = player.r.y;
		rotInitialUp = player.r.x;
		xMoved = 0;
		yMoved = 0;
		rotActive = true;
	}
}

static void calcScroll(float *y, float *dydt, float accel, float decel, float targetVelocity, float dt)
{
	double tMid;

	// Stop instantly, if trying to change direction.
	if (targetVelocity * *dydt < -1e-8f)
	{
		*dydt = 0;
	}

	if (targetVelocity < *dydt)
	{
		accel = -accel;
		decel = -decel;
	}

	// Decelerate if needed.
	tMid = (0 - *dydt) / decel;
	CLIP(tMid, 0, dt);
	*y += *dydt * tMid + decel / 2 * tMid * tMid;
	*dydt += decel * tMid;
	dt -= tMid;

	// Accelerate if needed.
	tMid = (targetVelocity - *dydt) / accel;
	CLIP(tMid, 0, dt);
	*y += *dydt * tMid + accel / 2 * tMid * tMid;
	*dydt += accel * tMid;
	dt -= tMid;

	// Continue at target velocity.
	*y += *dydt * dt;
}

void scroll(void)
{
	SDWORD	xDif, yDif;
	uint32_t timeDiff;
	int scrollDirLeftRight = 0, scrollDirUpDown = 0;
	float scroll_zoom_factor = 1 + 2 * ((getViewDistance() - MINDISTANCE) / ((float)(MAXDISTANCE - MINDISTANCE)));
	float scaled_max_scroll_speed = scroll_zoom_factor * MAX_SCROLL_SPEED;
	float scaled_accel;

	static float xDiffFrac = 0, yDiffFrac = 0;

	if (InGameOpUp || bDisplayMultiJoiningStatus || isInGamePopupUp)		// cant scroll when menu up. or when over radar
	{
		return;
	}

	if (mouseScroll && wzMouseInWindow())
	{
		// Scroll left or right
		scrollDirLeftRight += (mouseX() > (pie_GetVideoBufferWidth() - BOUNDARY_X)) - (mouseX() < BOUNDARY_X);

		// Scroll down or up
		scrollDirUpDown += (mouseY() < BOUNDARY_Y) - (mouseY() > (pie_GetVideoBufferHeight() - BOUNDARY_Y));
		// when mouse cursor goes to an edge, set cursor appropriately
		if (scrollDirUpDown > 0)
		{
			wzSetCursor(CURSOR_UARROW);
		}
		else if (scrollDirUpDown < 0)
		{
			wzSetCursor(CURSOR_DARROW);
		}
		else if (scrollDirLeftRight < 0)
		{
			wzSetCursor(CURSOR_LARROW);
		}
		else if (scrollDirLeftRight > 0)
		{
			wzSetCursor(CURSOR_RARROW);
		}
		else
		{
			wzSetCursor(CURSOR_DEFAULT);
		}
	}
	if (!keyDown(KEY_LCTRL) && !keyDown(KEY_RCTRL))
	{
		// Scroll left or right
		scrollDirLeftRight += keyDown(KEY_RIGHTARROW) - keyDown(KEY_LEFTARROW);

		// Scroll down or up
		scrollDirUpDown += keyDown(KEY_UPARROW) - keyDown(KEY_DOWNARROW);

	}
	CLIP(scrollDirLeftRight, -1, 1);
	CLIP(scrollDirUpDown,    -1, 1);

	if (scrollDirLeftRight != 0 || scrollDirUpDown != 0)
	{
		setWarCamActive(false);  // Don't let this thing override the user trying to scroll.
	}

	scaled_accel = scroll_zoom_factor * scroll_speed_accel;

	// Apparently there's stutter if using deltaRealTime, so we have our very own delta time here, just for us.
	timeDiff = wzGetTicks() - scrollRefTime;
	scrollRefTime += timeDiff;
	timeDiff = std::min<unsigned>(timeDiff, 500);  // Since we're using our own time variable, which isn't updated when dragging a box, clamp the time here so releasing the box doesn't scroll to the edge of the map suddenly.

	scrollStepLeftRight = 0;
	scrollStepUpDown = 0;
	calcScroll(&scrollStepLeftRight, &scrollSpeedLeftRight, scaled_accel, 2 * scaled_accel, scrollDirLeftRight * scaled_max_scroll_speed, (float)timeDiff / GAME_TICKS_PER_SEC);
	calcScroll(&scrollStepUpDown,    &scrollSpeedUpDown,    scaled_accel, 2 * scaled_accel, scrollDirUpDown    * scaled_max_scroll_speed, (float)timeDiff / GAME_TICKS_PER_SEC);

	/* Get x component of movement */
	xDiffFrac += cos(-player.r.y * (M_PI / 32768)) * scrollStepLeftRight + sin(-player.r.y * (M_PI / 32768)) * scrollStepUpDown;
	/* Get y component of movement */
	yDiffFrac += sin(-player.r.y * (M_PI / 32768)) * scrollStepLeftRight - cos(-player.r.y * (M_PI / 32768)) * scrollStepUpDown;

	xDif = (int)xDiffFrac;
	yDif = (int)yDiffFrac;

	xDiffFrac -= xDif;
	yDiffFrac -= yDif;

	/* Adjust player's position by these components */
	player.p.x += xDif;
	player.p.z += yDif;

	CheckScrollLimits();
}

/*
 * Reset scrolling, so we don't jump around after unpausing.
 */
void resetScroll(void)
{
	scrollRefTime = wzGetTicks();
	scrollSpeedUpDown = 0.0f;
	scrollSpeedLeftRight = 0.0f;
}

// Check a coordinate is within the scroll limits, SDWORD version.
// Returns true if edge hit.
//
bool CheckInScrollLimits(SDWORD *xPos, SDWORD *zPos)
{
	bool EdgeHit = false;
	SDWORD	minX, minY, maxX, maxY;

	minX = world_coord(scrollMinX);
	maxX = world_coord(scrollMaxX - 1);
	minY = world_coord(scrollMinY);
	maxY = world_coord(scrollMaxY - 1);

	//scroll is limited to what can be seen for current campaign
	if (*xPos < minX)
	{
		*xPos = minX;
		EdgeHit = true;
	}
	else if (*xPos >= maxX)
	{
		*xPos = maxX;
		EdgeHit = true;
	}

	if (*zPos < minY)
	{
		*zPos = minY;
		EdgeHit = true;
	}
	else if (*zPos >= maxY)
	{
		*zPos = maxY;
		EdgeHit = true;
	}


	return EdgeHit;
}

// Check the view is within the scroll limits,
// Returns true if edge hit.
//
bool CheckScrollLimits(void)
{
	SDWORD xp = player.p.x;
	SDWORD zp = player.p.z;
	bool ret = CheckInScrollLimits(&xp, &zp);

	player.p.x = xp;
	player.p.z = zp;

	return ret;
}

/* Do the 3D display */
void displayWorld(void)
{
	Vector3i pos;

	if (mouseDown(MOUSE_ROTATE) && rotActive)
	{
		if (abs(mouseX() - rotX) > 2 || xMoved > 2 || abs(mouseY() - rotY) > 2 || yMoved > 2)
		{
			xMoved += abs(mouseX() - rotX);
			if (mouseX() < rotX)
			{
				player.r.y = rotInitial + (rotX - mouseX()) * DEG(1) / 2;
			}
			else
			{
				player.r.y = rotInitial - (mouseX() - rotX) * DEG(1) / 2;
			}
			yMoved += abs(mouseY() - rotY);
			if (bInvertMouse)
			{
				if (mouseY() < rotY)
				{
					player.r.x = rotInitialUp + (rotY - mouseY()) * DEG(1) / 3;
				}
				else
				{
					player.r.x = rotInitialUp - (mouseY() - rotY) * DEG(1) / 3;
				}
			}
			else
			{
				if (mouseY() < rotY)
				{
					player.r.x = rotInitialUp - (rotY - mouseY()) * DEG(1) / 3;
				}
				else
				{
					player.r.x = rotInitialUp + (mouseY() - rotY) * DEG(1) / 3;
				}
			}
			if (player.r.x > DEG(360 + MAX_PLAYER_X_ANGLE))
			{
				player.r.x = DEG(360 + MAX_PLAYER_X_ANGLE);
			}
			if (player.r.x < DEG(360 + MIN_PLAYER_X_ANGLE))
			{
				player.r.x = DEG(360 + MIN_PLAYER_X_ANGLE);
			}

			setDesiredPitch(player.r.x / DEG_1);
		}
	}

	if (!mouseDown(MOUSE_ROTATE) && rotActive)
	{
		rotActive = false;
		xMoved = yMoved = 0;
		ignoreRMBC = true;
		pos.x = player.r.x;
		pos.y = player.r.y;
		pos.z = player.r.z;
		camInformOfRotation(&pos);
		bRadarDragging = false;
	}

	draw3DScene();
}

static bool mouseInBox(SDWORD x0, SDWORD y0, SDWORD x1, SDWORD y1)
{
	return mouseX() > x0 && mouseX() < x1 && mouseY() > y0 && mouseY() < y1;
}

bool DrawnInLastFrame(int32_t frame)
{
	return frame >= (int32_t)StartOfLastFrame;
}


/*
	Returns what the mouse was clicked on. Only called if there was a mouse pressed message
	on MOUSE_LMB. We aren't concerned here with setting selection flags - just what it
	actually was
*/
BASE_OBJECT *mouseTarget()
{
	BASE_OBJECT *psReturn = NULL;
	int dispX, dispY, dispR;

	if (mouseTileX < 0 || mouseTileY < 0 || mouseTileX > mapWidth - 1 || mouseTileY > mapHeight - 1)
	{
		return (NULL);
	}

	/* First have a look through the droid lists */
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		/* Note the !psObject check isn't really necessary as the goto will jump out */
		for (DROID *psDroid = apsDroidLists[i]; psDroid && !psReturn; psDroid = psDroid->psNext)
		{
			dispX = psDroid->sDisplay.screenX;
			dispY = psDroid->sDisplay.screenY;
			dispR = psDroid->sDisplay.screenR;

			// Has the droid been drawn since the start of the last frame
			if (psDroid->visible[selectedPlayer] && DrawnInLastFrame(psDroid->sDisplay.frameNumber))
			{
				if (mouseInBox(dispX - dispR, dispY - dispR, dispX + dispR, dispY + dispR))
				{
					/* We HAVE clicked on droid! */
					psReturn = (BASE_OBJECT *) psDroid;
					/* There's no point in checking other object types */
					return psReturn;
				}
			}
		}
	} // end of checking for droids

	/*	Not a droid, so maybe a structure or feature?
		If still NULL after this then nothing */
	psReturn = getTileOccupier(mouseTileX, mouseTileY);

	if (psReturn == NULL)
	{
		psReturn = getTileBlueprintStructure(mouseTileX, mouseTileY);
	}

	/* Send the result back - if it's null then we clicked on an area of terrain */
	return psReturn;
}

// Start repositioning a delivery point.
//
static FLAG_POSITION flagPos;
static int flagStructId;
static bool flagReposVarsValid;
static bool flagReposFinished;

void startDeliveryPosition(FLAG_POSITION *psFlag)
{
	FLAG_POSITION	*psFlagPos;

	if (tryingToGetLocation()) // if we're placing a building don't place
	{
		return;
	}

	//clear the selected delivery point
	for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos; psFlagPos = psFlagPos->psNext)
	{
		psFlagPos->selected = false;
	}

	//set this object position to be highlighted
	psFlag->selected = true;
	flagPos = *psFlag;

	STRUCTURE *psStruct = findDeliveryFactory(psFlag);
	if (!psStruct)
	{
		flagStructId = 0; // not a struct, just a flag.
	}
	else
	{
		flagStructId = psStruct->id;
	}
	flagReposVarsValid = true;
	flagReposFinished = false;
}

// Finished repositioning a delivery point.
//
void finishDeliveryPosition()
{
	FLAG_POSITION *psFlagPos;
	if (flagStructId)
	{
		flagReposVarsValid = false;
		STRUCTURE *psStruct = IdToStruct(flagStructId, selectedPlayer);
		if (StructIsFactory(psStruct) && psStruct->pFunctionality
		    && psStruct->pFunctionality->factory.psAssemblyPoint)
		{
			setAssemblyPoint(psStruct->pFunctionality->factory.psAssemblyPoint,
			                 flagPos.coords.x, flagPos.coords.y, selectedPlayer, true);
		}
		else if (psStruct->pStructureType->type == REF_REPAIR_FACILITY && psStruct->pFunctionality != NULL)
		{
			setAssemblyPoint(psStruct->pFunctionality->repairFacility.psDeliveryPoint,
			                 flagPos.coords.x, flagPos.coords.y, selectedPlayer, true);
		}
		//deselect once moved
		for (psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos; psFlagPos = psFlagPos->psNext)
		{
			psFlagPos->selected = false;
		}
	}
	flagReposFinished = true;
}

// Is there a valid delivery point repositioning going on.
bool deliveryReposValid(void)
{
	if (!flagReposVarsValid)
	{
		return false;
	}

	Vector2i map = map_coord(removeZ(flagPos.coords));

	//make sure we are not too near map edge
	if (map.x < scrollMinX + TOO_NEAR_EDGE || map.x + 1 > scrollMaxX - TOO_NEAR_EDGE ||
	    map.y < scrollMinY + TOO_NEAR_EDGE || map.y + 1 > scrollMaxY - TOO_NEAR_EDGE)
	{
		return false;
	}

	// cant place on top of a delivery point...
	for (FLAG_POSITION const *psCurrFlag = apsFlagPosLists[selectedPlayer]; psCurrFlag; psCurrFlag = psCurrFlag->psNext)
	{
		Vector2i flagTile = map_coord(removeZ(psCurrFlag->coords));
		if (flagTile == map)
		{
			return false;
		}
	}

	if (fpathBlockingTile(map.x, map.y, PROPULSION_TYPE_WHEELED))
	{
		return false;
	}

	return true;
}

bool deliveryReposFinished(FLAG_POSITION *psFlag)
{
	if (!flagReposVarsValid)
	{
		return false;
	}

	if (psFlag)
	{
		*psFlag = flagPos;
	}
	return flagReposFinished;
}

void processDeliveryRepos(void)
{
	if (!flagReposVarsValid)
	{
		return;
	}

	int bX = clip(mouseTileX, 2, mapWidth - 3);
	int bY = clip(mouseTileY, 2, mapHeight - 3);

	flagPos.coords = Vector3i(world_coord(Vector2i(bX, bY)) + Vector2i(TILE_UNITS / 2, TILE_UNITS / 2), map_TileHeight(bX, bY) + 2 * ASSEMBLY_POINT_Z_PADDING);
}

// Cancel repositioning of the delivery point without moving it.
//
void cancelDeliveryRepos(void)
{
	flagReposVarsValid = false;
}

void renderDeliveryRepos(void)
{
	if (flagReposVarsValid)
	{
		renderDeliveryPoint(&flagPos, true);
	}
}

// check whether a clicked on droid is assigned to a sensor
static bool droidHasLeader(DROID *psDroid)
{
	BASE_OBJECT		*psLeader;

	if (psDroid->droidType == DROID_SENSOR)
	{
		return false;
	}

	// psLeader can be either a droid or a structure
	psLeader = orderStateObj(psDroid, DORDER_FIRESUPPORT);

	if (psLeader != NULL)
	{
		if (psLeader->type == OBJ_DROID)
		{
			SelectDroid((DROID *)psLeader);
		}
		assignSensorTarget(psLeader);
		return true;
	}

	return false;
}

// deal with selecting a droid
void dealWithDroidSelect(DROID *psDroid, bool bDragBox)
{
	/*	Toggle selection on and off - allows you drag around a big
		area of droids and then exclude certain individuals */
	if (!bDragBox && psDroid->selected == true)
	{
		DeSelectDroid(psDroid);
	}
	else if (ctrlShiftDown() || !droidHasLeader(psDroid))
	{
		if (specialOrderKeyDown())
		{
			/* We only want to select weapon units if ALT is down on a drag */
			if (psDroid->asWeaps[0].nStat > 0)
			{
				SelectDroid(psDroid);
			}
		}
		else
		{
			SelectDroid(psDroid);
		}
	}
}

static void FeedbackOrderGiven(void)
{
	static UDWORD LastFrame = 0;
	UDWORD ThisFrame = frameGetFrameNumber();

	// Ensure only played once per game cycle.
	if (ThisFrame != LastFrame)
	{
		audio_PlayTrack(ID_SOUND_SELECT);
		LastFrame = ThisFrame;
	}
}

// check whether the queue order keys are pressed
bool ctrlShiftDown(void)
{
	return keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL) || keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT);
}

void AddDerrickBurningMessage(void)
{
	addConsoleMessage(_("Cannot Build. Oil Resource Burning."), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	audio_PlayTrack(ID_SOUND_BUILD_FAIL);
}

static void printDroidClickInfo(DROID *psDroid)
{
	if (getDebugMappingStatus()) // cheating on, so output debug info
	{
		console("%s - Hitpoints %d/%d - ID %d - experience %f, %s - order %s - action %s - sensor range %hu - ECM %u - pitch %.0f - frust %u",
		        droidGetName(psDroid), psDroid->body, psDroid->originalBody, psDroid->id,
		        psDroid->experience / 65536.f, getDroidLevelName(psDroid), getDroidOrderName(psDroid->order.type), getDroidActionName(psDroid->action),
		        droidSensorRange(psDroid), droidJammerPower(psDroid), UNDEG(psDroid->rot.pitch), psDroid->lastFrustratedTime);
		FeedbackOrderGiven();
	}
	else if (!psDroid->selected)
	{
		console(_("%s - Hitpoints %d/%d - Experience %.1f, %s"), droidGetName(psDroid), psDroid->body, psDroid->originalBody,
		        psDroid->experience / 65536.f, _(getDroidLevelName(psDroid)));
		FeedbackOrderGiven();
	}
	clearSelection();
	dealWithDroidSelect(psDroid, false);
}

static void dealWithLMBDroid(DROID *psDroid, CURSOR cursor, std::list<DROID *> droids)
{
	if (!aiCheckAlliances(selectedPlayer, psDroid->player))
	{
		memset(DROIDDOING, 0x0 , sizeof(DROIDDOING)); // take over the other players droid by debug menu.
		/* We've clicked on enemy droid */
		if (getDebugMappingStatus())
		{
			console("(Enemy!) %s - Hitpoints %d/%d - ID %d - experience %f, %s - order %s - action %s - sensor range %d - ECM %d - pitch %.0f",
			        droidGetName(psDroid),  psDroid->body, psDroid->originalBody, psDroid->id,
			        psDroid->experience / 65536.f, getDroidLevelName(psDroid), getDroidOrderName(psDroid->order.type),
			        getDroidActionName(psDroid->action), droidSensorRange(psDroid), droidJammerPower(psDroid), UNDEG(psDroid->rot.pitch));
			FeedbackOrderGiven();
		}
	}

	if (cursor == CURSOR_SELECT || cursor == CURSOR_SELECT_ADD)
	{
		ASSERT(psDroid->player == selectedPlayer, "Bad selection");
		if (cursor == CURSOR_SELECT)
		{
			ClearSelectedDroids(false); // remove old selections
		}
		dealWithDroidSelect(psDroid, false);
		if (isTransporter(psDroid))
		{
			if (bMultiPlayer)
			{
				triggerEventSelected();
			}
		}
		FeedbackOrderGiven();
		return;
	}

	for (DROID *psCurr : droids)
	{
		switch (cursor)
		{
		case CURSOR_ATTACK:
			break;
		case CURSOR_EMBARK:
			ASSERT(isTransporter(psDroid), "Bad target for embark");
			orderDroidObj(psCurr, DORDER_EMBARK, psDroid);
			FeedbackOrderGiven();
			break;
		case CURSOR_FIX:
			orderDroidObj(psDroid, DORDER_REPAIR, psDroid);
			FeedbackOrderGiven();
			break;
		case CURSOR_DEFAULT:
			// Just plain clicked on
			if (selectedPlayer == psDroid->player)
			{
				printDroidClickInfo(psDroid);
			}
			else // Clicked on allied unit with no other possible actions
			{
				console(_("%s - Allied - Hitpoints %d/%d - Experience %d, %s"), droidGetName(psDroid), psDroid->body, psDroid->originalBody,
				        psDroid->experience / 65536, getDroidLevelName(psDroid));
				FeedbackOrderGiven();
			}
			break;
		default:
			break;
		}
	}
}

static void dealWithLMBCommon(BASE_OBJECT *psObj, CURSOR cursor, std::list<DROID *> droids)
{
	for (DROID *psCurr : droids)
	{
		switch (cursor)
		{
		case CURSOR_GUARD:
			orderDroidObj(psCurr, DORDER_GUARD, psObj);
			intSetSelectMode(SELECT_NONE);
			FeedbackOrderGiven();
			break;
		case CURSOR_ATTACH:
			orderDroidObj(psCurr, DORDER_FIRESUPPORT, psObj);
			clearSelection();
			assignSensorTarget(psObj);
			FeedbackOrderGiven();
			break;
		case CURSOR_LOCKON:
			orderDroidObj(psCurr, DORDER_OBSERVE, psObj);
			FeedbackOrderGiven();
			break;
		case CURSOR_ATTACK:
			orderDroidObj(psCurr, DORDER_ATTACK, psObj);
			for (auto d : droids)
			{
				if (isVtolDroid(d))
				{
					audio_QueueTrack(ID_SOUND_ON_OUR_WAY2);
					break; // only once
				}
			}
			intSetSelectMode(SELECT_NONE);
			FeedbackOrderGiven();
			break;
		case CURSOR_SEEKREPAIR:
			orderDroidObj(psCurr, DORDER_ATTACK, psObj);
			FeedbackOrderGiven();
			break;
		case CURSOR_FIX:
			orderDroidObj(psCurr, DORDER_REPAIR, psObj);
			FeedbackOrderGiven();
			break;
		default:
			break;
		}
	}
	// eventually we should loop over structures here as well
	if (cursor == CURSOR_ATTACK) // also check for lassat
	{
		STRUCTURE *psLasSat = getLasSat(selectedPlayer);
		if (bMultiPlayer && psLasSat && psLasSat->selected)
		{
			orderStructureObj(selectedPlayer, psObj);
		}
		intSetSelectMode(SELECT_NONE);
		FeedbackOrderGiven();
	}
}

static void dealWithLMBStructure(STRUCTURE *psStructure, CURSOR cursor, std::list<DROID *> droids)
{
	if (aiCheckAlliances(psStructure->player, selectedPlayer) || getDebugMappingStatus())
	{
		printStructureInfo(psStructure);
	}

	switch (cursor)
	{
	case CURSOR_DEST:
		for (DROID *psCurr : droids)
		{
			orderDroidObj(psCurr, DORDER_DEMOLISH, psStructure);
			FeedbackOrderGiven();
		}
		if (ctrlShiftDown())
		{
			quickQueueMode = true;
		}
		else
		{
			intDemolishCancel();
		}
		break;
	case CURSOR_BUILD:
		if (psStructure->status == SS_BEING_BUILT)
		{
			for (DROID *psCurr : droids)
			{
				orderDroidObj(psCurr, DORDER_HELPBUILD, psStructure);
				FeedbackOrderGiven();
			}
		}
		else // upgrade module
		{
			for (DROID *psCurr : droids)
			{
				buildModule(psCurr, psStructure, ctrlShiftDown());
				FeedbackOrderGiven();
			}
		}
		break;
	case CURSOR_SELECT:
	case CURSOR_SELECT_ADD:
		/* Clear old building selection(s) - should only be one */
		for (STRUCTURE *psCurr = apsStructLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
		{
			psCurr->selected = false;
		}
		/* Establish new one */
		psStructure->selected = true;
		triggerEventSelected();
		break;
	case CURSOR_DEFAULT:
		if (selectedDroidList().size() == 0)
		{
			intObjectSelected((BASE_OBJECT *)psStructure);
			FeedbackOrderGiven();
		}
	default:
		break;
	}
}

static void dealWithLMBFeature(FEATURE *psFeature, CURSOR cursor, std::list<DROID *> droids)
{
	if (cursor == CURSOR_BUILD)
	{
		if (fireOnLocation(psFeature->pos.x, psFeature->pos.y))
		{
			// Can't build because it's burning
			AddDerrickBurningMessage();
			return;
		}
		// first find the derrick.
		unsigned int i;
		for (i = 0; (i < numStructureStats) && (asStructureStats[i].type != REF_RESOURCE_EXTRACTOR); ++i) {}
		for (DROID *psCurr : droids)
		{
			sendDroidInfo(psCurr, DroidOrder(DORDER_BUILD, &asStructureStats[i], removeZ(psFeature->pos), player.r.y), ctrlShiftDown());
		}
		FeedbackOrderGiven();
	}
	else if (cursor == CURSOR_PICKUP)
	{
		DROID *psNearestUnit = getNearestDroid(mouseTileX * TILE_UNITS + TILE_UNITS / 2,
		                                       mouseTileY * TILE_UNITS + TILE_UNITS / 2, true);
		ASSERT_OR_RETURN(, psNearestUnit, "No nearest unit for pickup found!?");
		sendDroidInfo(psNearestUnit, DroidOrder(DORDER_RECOVER, psFeature), ctrlShiftDown());
		FeedbackOrderGiven();
	}
	if (getDebugMappingStatus())
	{
		console("(Feature) %s ID: %d ref: %d Hipoints: %d/%d", getID(psFeature->psStats), psFeature->id, psFeature->psStats->ref, psFeature->psStats->body, psFeature->body);
	}
}

void dealWithLMB(BASE_OBJECT *psClickedOn, CURSOR cursor, std::list<DROID *> droids)
{
	OBJECT_POSITION *psLocation;

	/* Don't process if in game options are on screen */
	// FIXME, move to cursor setting?
	if (mouseOverRadar || InGameOpUp == true || widgGetFromID(psWScreen, INTINGAMEOP))
	{
		return;
	}

	/* What have we clicked on? */
	if (psClickedOn)
	{
		dealWithLMBCommon(psClickedOn, cursor, droids);

		switch (psClickedOn->type)
		{
		case OBJ_DROID:
			dealWithLMBDroid((DROID *)psClickedOn, cursor, droids);
			break;
		case OBJ_STRUCTURE:
			dealWithLMBStructure((STRUCTURE *)psClickedOn, cursor, droids);
			break;
		case OBJ_FEATURE:
			dealWithLMBFeature((FEATURE *)psClickedOn, cursor, droids);
			break;
		default:
			// assert only when the value is outside of the valid range
			ASSERT(psClickedOn->type < OBJ_NUM_TYPES, "Weird selection from LMB - type of clicked object is %d", (int)psClickedOn->type);
			break;
		}
		return;
	}

	if (cursor == CURSOR_MOVE || cursor == CURSOR_SCOUT || cursor == CURSOR_SIGHT)
	{
		for (DROID *psDroid : droids)
		{
			switch (cursor)
			{
			case CURSOR_MOVE:
				sendDroidInfo(psDroid, DroidOrder(DORDER_MOVE, mousePos), ctrlShiftDown());
				break;
			case CURSOR_SCOUT:
				sendDroidInfo(psDroid, DroidOrder(DORDER_SCOUT, mousePos), ctrlShiftDown());
				break;
			case CURSOR_SIGHT:
				switch (intSelectMode())
				{
				case SELECT_PATROL:
					sendDroidInfo(psDroid, DroidOrder(DORDER_PATROL, mousePos), false);
					break;
				case SELECT_CIRCLE:
					sendDroidInfo(psDroid, DroidOrder(DORDER_CIRCLE, mousePos), false);
					break;
				default:
					ASSERT(false, "unhandled case");
					break;
				}
				break;
			default:
				ASSERT(false, "unhandled case");
				break;
			}
		}
		assignDestTarget();
		audio_PlayTrack(ID_SOUND_SELECT);
	}
	/*Check for a Delivery Point or a Proximity Message*/
	psLocation = checkMouseLoc();
	if (psLocation == NULL)
	{
		if (getDebugMappingStatus() && tileOnMap(mouseTileX, mouseTileY))
		{
			MAPTILE *psTile = mapTile(mouseTileX, mouseTileY);
			uint8_t aux = auxTile(mouseTileX, mouseTileY, selectedPlayer);

			console("%s tile %d, %d [%d, %d] continent(l%d, h%d) level %g illum %d %s %s w=%d s=%d j=%d",
			        tileIsExplored(psTile) ? "Explored" : "Unexplored",
			        mouseTileX, mouseTileY, world_coord(mouseTileX), world_coord(mouseTileY),
			        (int)psTile->limitedContinent, (int)psTile->hoverContinent, psTile->level, (int)psTile->illumination,
			        aux & AUXBITS_DANGER ? "danger" : "", aux & AUXBITS_THREAT ? "threat" : "",
			        (int)psTile->watchers[selectedPlayer], (int)psTile->sensors[selectedPlayer], (int)psTile->jammers[selectedPlayer]);
		}
		return;
	}
	switch (psLocation->type)
	{
	case POS_DELIVERY:
		if (psLocation->player == selectedPlayer)
		{
			startDeliveryPosition((FLAG_POSITION *)psLocation);
		}
		break;
	default:
		ASSERT(!"unknown object position type", "Unknown type from checkMouseLoc");
	}
}

bool	getRotActive(void)
{
	return (rotActive);
}

SDWORD	getDesiredPitch(void)
{
	return (desiredPitch);
}

void	setDesiredPitch(SDWORD pitch)
{
	desiredPitch = pitch;
}

// process LMB double clicks
static void dealWithLMBDClick(BASE_OBJECT *psClickedOn, CURSOR cursor, std::list<DROID *> droids)
{
	if (psClickedOn)
	{
		if (psClickedOn->type == OBJ_DROID)
		{
			/* We clicked on droid */
			DROID *psDroid = (DROID *) psClickedOn;
			if (psDroid->player == selectedPlayer)
			{
				// Now selects all of same type on screen
				selDroidSelection(selectedPlayer, DS_BY_TYPE, DST_ALL_SAME, true);
			}
		}
		else if (psClickedOn->type == OBJ_STRUCTURE)
		{
			/* We clicked on structure */
			STRUCTURE *psStructure = (STRUCTURE *) psClickedOn;
			if (psStructure->player == selectedPlayer && !structureIsBlueprint(psStructure))
			{
				if (StructIsFactory(psStructure))
				{
					setViewPos(map_coord(psStructure->pFunctionality->factory.psAssemblyPoint->coords.x),
					           map_coord(psStructure->pFunctionality->factory.psAssemblyPoint->coords.y),
					           true);
				}
				else if (psStructure->pStructureType->type == REF_REPAIR_FACILITY)
				{
					setViewPos(map_coord(psStructure->pFunctionality->repairFacility.psDeliveryPoint->coords.x),
					           map_coord(psStructure->pFunctionality->repairFacility.psDeliveryPoint->coords.y),
					           true);
				}
			}
		}
	}
}

/*This checks to see if the mouse was over a delivery point or a proximity message
when the mouse button was pressed */
static OBJECT_POSITION 	*checkMouseLoc(void)
{
	FLAG_POSITION		*psPoint;
	UDWORD				i;
	UDWORD				dispX, dispY, dispR;

	// First have a look through the DeliveryPoint lists
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		//new way, handles multiple points.
		for (psPoint = apsFlagPosLists[i]; psPoint; psPoint = psPoint->psNext)
		{
			dispX = psPoint->screenX;
			dispY = psPoint->screenY;
			dispR = psPoint->screenR;
			if (DrawnInLastFrame(psPoint->frameNumber) == true)	// Only check DP's that are on screen
			{
				if (mouseInBox(dispX - dispR, dispY - dispR, dispX + dispR, dispY + dispR))
				{
					// We HAVE clicked on DP!
					return psPoint;
				}
			}
		}
	}
	return NULL;
}

static void dealWithRMB(BASE_OBJECT *psClickedOn, CURSOR cursor, std::list<DROID *> droids)
{
	if (mouseOverRadar || InGameOpUp == true || widgGetFromID(psWScreen, INTINGAMEOP))
	{
		return;
	}

	if (!psClickedOn)
	{
		/*Check for a Delivery Point*/
		OBJECT_POSITION *psLocation = checkMouseLoc();
		if (psLocation)
		{
			ASSERT(psLocation->type == POS_DELIVERY, "Strange location type");
			if (psLocation->player == selectedPlayer)
			{
				//centre the view on the owning Factory
				STRUCTURE *psStructure = findDeliveryFactory((FLAG_POSITION *)psLocation);
				if (psStructure)
				{
					setViewPos(map_coord(psStructure->pos.x), map_coord(psStructure->pos.y), true);
				}
			}
		}
		else
		{
			intObjectSelected(NULL);
			memset(DROIDDOING, 0x0 , sizeof(DROIDDOING));	// clear string when deselected
			clearSelection();
		}
	}
	else
	{
		if (psClickedOn->type == OBJ_DROID)
		{
			DROID *psDroid = (DROID *) psClickedOn;
			if (psDroid->player == selectedPlayer)
			{
				// Not a transporter
				if (!isTransporter(psDroid))
				{
					if (psDroid->selected != true)
					{
						clearSelection();
						SelectDroid(psDroid);
					}
					intObjectSelected((BASE_OBJECT *)psDroid);
				}
				// Transporter
				else
				{
					if (bMultiPlayer)
					{
						intResetScreen(false);
						if (!getWidgetsStatus())
						{
							setWidgetsStatus(true);
						}
						addTransporterInterface(psDroid, false);
					}
				}
			}
			else if (bMultiPlayer && isHumanPlayer(psDroid->player))
			{
				console("%s", droidGetName(psDroid));
				FeedbackOrderGiven();
			}
		}	// end if its a droid
		else if (psClickedOn->type == OBJ_STRUCTURE)
		{
			STRUCTURE *psStructure = (STRUCTURE *) psClickedOn;
			if (psStructure->player == selectedPlayer)
			{
				if (psStructure->selected == true)
				{
					psStructure->selected = false;
					intObjectSelected(NULL);
					triggerEventSelected();
				}
				else if (!structureIsBlueprint(psStructure))
				{
					clearSelection();
					intObjectSelected((BASE_OBJECT *)psStructure);
				}
			}
		}	// end if its a structure
	}
}

static BASE_OBJECT *itemUnderMouse()
{
	if (mouseTileX < 0 || mouseTileY < 0 || mouseTileX > (int)(mapWidth - 1) || mouseTileY > (int)(mapHeight - 1))
	{
		return NULL;
	}

	/* First have a look through the droid lists */
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		/* Note the !psObject check isn't really necessary as the goto will jump out */
		for (DROID *psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			unsigned dispX = psDroid->sDisplay.screenX;
			unsigned dispY = psDroid->sDisplay.screenY;
			unsigned dispR = psDroid->sDisplay.screenR;
			/* Only check droids that're on screen */
			if (psDroid->sDisplay.frameNumber + 1 == currentFrame && psDroid->visible[selectedPlayer])
			{
				if (mouseInBox(dispX - dispR, dispY - dispR, dispX + dispR, dispY + dispR))
				{
					return psDroid;
				}
			}
		}
	} // end of checking for droids

	BASE_OBJECT *psNotDroid = getTileOccupier(mouseTileX, mouseTileY);
	if (!psNotDroid)
	{
		psNotDroid = getTileBlueprintStructure(mouseTileX, mouseTileY);
	}
	return psNotDroid;
}

/* Clear the selection flag for a player */
void clearSelection()
{
	ClearSelectedDroids(false);
	for (STRUCTURE *psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
	{
		psStruct->selected = false;
	}
	//clear the Deliv Point if one
	for (FLAG_POSITION *psFlagPos = apsFlagPosLists[selectedPlayer]; psFlagPos; psFlagPos = psFlagPos->psNext)
	{
		psFlagPos->selected = false;
	}
	intRefreshScreen();
	triggerEventSelected();
}

/* Initialise the display system */
bool dispInitialise(void)
{
	flagReposVarsValid = false;
	return true;
}
