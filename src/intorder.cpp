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

#include "lib/framework/frame.h"
#include "lib/ivis_opengl/bitimage.h"//bitmap routines

#include "hci.h"
#include "keymap.h"
#include "intdisplay.h"
#include "intorder.h"
#include "objects.h"
#include "order.h"
#include "keybind.h"

#include <set>

#define ORDER_X			6
#define ORDER_Y			45
#define ORDER_WIDTH		RET_FORMWIDTH
#define ORDER_HEIGHT	273
#define ORDER_BUTX		8
#define ORDER_BUTY		16
#define ORDER_BUTGAP	4
#define ORDER_BOTTOMY	318	+ E_H

struct ORDER_BUTTON
{
	int x;
	int y;
	const char *tip;
	bool (*enabled)(int userdata);
	bool (*selected)(int userdata);
	unsigned ButImageID;
	void (*action)(void);
	int userdata;
};

bool OrderUp = false;

static SECONDARY_STATE getSecondaryStates(SECONDARY_ORDER secondary);

static bool enabled_holdfire(int userdata)
{
	for (const DROID *psDroid : selectedDroidList())
	{
		if (psDroid->numWeaps > 0 && psDroid->asWeaps[0].nStat != 0)
		{
			return true;
		}
	}
	return false;
}

static bool enabled_return(int userdata)
{
	bool hqExists = false;
	bool factoryExists = false;
	bool repairExists = false;

	for (STRUCTURE *psStruct = apsStructLists[selectedPlayer]; psStruct != NULL; psStruct = psStruct->psNext)
	{
		factoryExists = factoryExists | (psStruct->type == REF_FACTORY) | (psStruct->type == REF_VTOL_FACTORY) | (psStruct->type == REF_CYBORG_FACTORY);
		hqExists = hqExists | (psStruct->type == REF_HQ);
		repairExists = repairExists | (psStruct->type == REF_REPAIR_FACILITY);
		if (hqExists && factoryExists && repairExists)
		{
			break;
		}
	}

	const SECONDARY_STATE state = (SECONDARY_STATE)userdata;
	switch (state)
	{
	case DSS_RTL_REPAIR: return repairExists;
	case DSS_RTL_BASE: return hqExists;
	case DSS_RTL_TRANSPORT: return false; // FIXME
	case DSS_RECYCLE_SET: return factoryExists;
	default:
		ASSERT(false, "Unhandled order case");
	}
	return false;
}

static bool selected_secondary(int userdata)
{
	const SECONDARY_STATE state = (SECONDARY_STATE)userdata;
	switch (state)
	{
	case DSS_RTL_REPAIR:
	case DSS_RTL_BASE:
	case DSS_RTL_TRANSPORT:
	case DSS_RECYCLE_SET:
		return getSecondaryStates(DSO_RETURN_TO_LOC) == state;
	case DSS_REPLEV_LOW:
	case DSS_REPLEV_HIGH:
	case DSS_REPLEV_NEVER:
		return getSecondaryStates(DSO_REPAIR_LEVEL) == state;
	case DSS_ALEV_NEVER:
		return getSecondaryStates(DSO_ATTACK_LEVEL) == state;
	default:
		ASSERT(false, "Unhandled order case");
	}
	return false;
}

static bool selected_mode(int userdata)
{
	const SELECT_MODE state = (SELECT_MODE)userdata;
	return state == intSelectMode();
}

static bool selected_hold(int userdata)
{
	bool all_holding = true;
	for (DROID *psDroid : selectedDroidList(SELECTED_ALL))
	{
		all_holding = all_holding && (psDroid->order.type == DORDER_HOLD);
	}
	return selectedDroidList(SELECTED_ALL).size() > 0 && all_holding;
}

const ORDER_BUTTON buttons[] =
{
	// primary orders
	{  8,  16, "Attack", enabled_holdfire, selected_mode, IMAGE_ORD_RANGE3UP, kf_SetDroidOrderAttack, SELECT_ATTACK },
	{ 48,  16, "Guard", enabled_holdfire, selected_mode, IMAGE_ORD_GUARDUP, kf_SetDroidOrderGuard, SELECT_GUARD },
	{ 88,  16, "Hold Position", NULL, selected_hold, IMAGE_ORD_HALTUP, kf_SetDroidOrderHold, 0 },

	{  8,  44, "Stop", NULL, NULL, IMAGE_MULTI_OFFAL, kf_SetDroidOrderStop, 0 },
	{ 48,  44, "Patrol", NULL, selected_mode, IMAGE_ORD_PATROLUP, kf_SetDroidMovePatrol, SELECT_PATROL },
	{ 88,  44, "Circle", NULL, selected_mode, IMAGE_ORD_CIRCLEUP, kf_SetDroidMoveCircle, SELECT_CIRCLE },

	// secondary orders
	{  8,  82, "Do or Die!", NULL, selected_secondary, IMAGE_ORD_REPAIR3UP, kf_SetDroidRetreatNever, DSS_REPLEV_NEVER },
	{ 48,  82, "Retreat at Heavy Damage", NULL, selected_secondary, IMAGE_ORD_REPAIR2UP, kf_SetDroidRetreatHeavy, DSS_REPLEV_HIGH },
	{ 88,  82, "Retreat at Medium Damage", NULL, selected_secondary, IMAGE_ORD_REPAIR1UP, kf_SetDroidRetreatMedium, DSS_REPLEV_LOW },

	{  8, 110, "Return For Repair", enabled_return, selected_secondary, IMAGE_ORD_RTRUP, kf_SetDroidGoForRepair, DSS_RTL_REPAIR },
	{ 48, 110, "Return To HQ", enabled_return, selected_secondary, IMAGE_ORD_GOTOHQUP, kf_SetDroidReturnToBase, DSS_RTL_BASE },
	{ 88, 110, "Go to Transport", enabled_return, selected_secondary, IMAGE_ORD_EMBARKUP, kf_SetDroidGoToTransport, DSS_RTL_TRANSPORT },

	{ 28, 138, "Return for Recycling", enabled_return, selected_secondary, IMAGE_ORD_DESTRUCT1UP, kf_SetDroidRecycle, DSS_RECYCLE_SET },
	{ 68, 138, "Hold Fire", enabled_holdfire, selected_secondary, IMAGE_ORD_HOLDFIREUP, kf_SetDroidAttackCease, DSS_ALEV_NEVER },
};

// return the state for an order for all the units selected
// if there are multiple states then don't return a state
static SECONDARY_STATE getSecondaryStates(SECONDARY_ORDER secondary)
{
	SECONDARY_STATE state = DSS_NONE;
	bool bFirst = true;

	for (const DROID *psDroid : selectedDroidList())
	{
		SECONDARY_STATE currState = secondaryGetState(psDroid, secondary, ModeQueue);
		if (bFirst)
		{
			state = currState;
			bFirst = false;
		}
		else if (state != currState)
		{
			state = (SECONDARY_STATE)0;
		}
	}

	return state;
}

// Add the droid order screen.
// Returns true if the form was displayed ok.
bool intAddOrder(DROID *psDroid, bool Animate)
{
	// Is the form already up?
	if (widgGetFromID(psWScreen, IDORDER_FORM) != NULL)
	{
		intRemoveOrderNoAnim();
		Animate = false;
	}
	// Is the stats window up?
	if (widgGetFromID(psWScreen, IDSTAT_FORM) != NULL)
	{
		intRemoveStatsNoAnim();
		Animate = false;
	}

	setWidgetsStatus(true);
	WIDGET *parent = psWScreen->psForm;

	/* Create the basic form */
	IntFormAnimated *orderForm = new IntFormAnimated(parent, Animate);  // Do not animate the opening, if the window was already open.
	orderForm->id = IDORDER_FORM;
	orderForm->setGeometry(ORDER_X, ORDER_Y, ORDER_WIDTH, ORDER_HEIGHT);

	// Add the close button.
	W_BUTINIT sButInit;
	sButInit.formID = IDORDER_FORM;
	sButInit.id = IDORDER_CLOSE;
	sButInit.x = ORDER_WIDTH - CLOSE_WIDTH;
	sButInit.y = 0;
	sButInit.width = CLOSE_WIDTH;
	sButInit.height = CLOSE_HEIGHT;
	sButInit.pTip = _("Close");
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0, IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	sButInit = W_BUTINIT();
	sButInit.formID = IDORDER_FORM;
	sButInit.pDisplay = intDisplayButtonHilight;

	int height = 0;
	int id = IDORDER_CLOSE + 1;

	for (auto &button : buttons)
	{
		sButInit.id = id++;
		sButInit.pTip = keyExpandWithMapping(button.tip, (void *)button.action);
		sButInit.x = button.x;
		sButInit.y = button.y;
		sButInit.width = 36;
		sButInit.height = 24;
		// disabled/greyed out image is always +2 to original
		sButInit.UserData = PACKDWORD_TRI(button.ButImageID + 2, IMAGE_DES_HILIGHT, button.ButImageID);
		if (!widgAddButton(psWScreen, &sButInit))
		{
			debug(LOG_ERROR, "Failed to add order button!");
			return false;
		}

		if (button.enabled && !button.enabled(button.userdata))
		{
			widgSetButtonState(psWScreen, sButInit.id, WBUT_DISABLE);
		}

		if (button.selected && button.selected(button.userdata))
		{
			widgSetButtonState(psWScreen, sButInit.id, WBUT_CLICKLOCK);
		}

		height = MAX(height, button.y + sButInit.height);
	}

	// Now we know how many orders there are we can resize the form accordingly.
	int newHeight = height + CLOSE_HEIGHT + ORDER_BUTGAP;
	orderForm->setGeometry(orderForm->x(), ORDER_BOTTOMY - newHeight, orderForm->width(), newHeight);

	OrderUp = true;
	intMode = INT_ORDER;

	return true;
}

// Do any housekeeping for the droid order screen.
void intRunOrder()
{
	if (selectedDroidList().empty())
	{
		intRemoveOrder();
	}
}

/// Process the droid order screen.
void intProcessOrder(UDWORD id)
{
	if (id == IDORDER_CLOSE)
	{
		intRemoveOrder();
		if (intMode == INT_ORDER)
		{
			intMode = INT_NORMAL;
		}
		else
		{
			/* Unlock the stats button */
			widgSetButtonState(psWScreen, objStatID, 0);
			intMode = INT_OBJECT;
		}
		return;
	}
	// else clicked on button
	const int idx = id - (IDORDER_CLOSE + 1);
	ASSERT(idx < ARRAY_SIZE(buttons), "Out of bounds access to orders (id=%u, idx=%d)", id, idx);
	buttons[idx].action();

	intRefreshOrder();
}

/// Call to refresh the Order screen, ie when a droids boards it.
bool intRefreshOrder()
{
	// Is the Order screen up?
	if (intMode == INT_ORDER && widgGetFromID(psWScreen, IDORDER_FORM))
	{
		if (selectedDroidList().empty())
		{
			// no units selected - quit
			intRemoveOrder();
			return true;
		}
		if (!intAddOrder(NULL))	// Refresh it by re-adding it.
		{
			intMode = INT_NORMAL;
			return false;
		}
	}

	return true;
}

/// Remove the droids order screen with animation.
void intRemoveOrder()
{
	widgDelete(psWScreen, IDORDER_CLOSE);

	// Start the window close animation.
	IntFormAnimated *form = (IntFormAnimated *)widgGetFromID(psWScreen, IDORDER_FORM);
	if (form != nullptr)
	{
		form->closeAnimateDelete();
		OrderUp = false;
	}
}

/// Remove the droids order screen without animation.
void intRemoveOrderNoAnim()
{
	widgDelete(psWScreen, IDORDER_CLOSE);
	widgDelete(psWScreen, IDORDER_FORM);
	OrderUp = false;
}
