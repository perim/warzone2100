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
 * @file challenge.c
 * Run challenges dialog.
 *
 */

#include <ctype.h>
#include <physfs.h>
#include <time.h>
#ifndef WIN32
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/iniparser/iniparser.h"
#include "lib/ivis_common/bitimage.h"
#include "lib/ivis_common/pieblitfunc.h"
#include "lib/widget/button.h"

#include "challenge.h"
#include "frontend.h"
#include "hci.h"
#include "intdisplay.h"
#include "loadsave.h"
#include "multiplay.h"
#include "scores.h"

#define totalslots 36			// challenge slots
#define slotsInColumn 12		// # of slots in a column
#define totalslotspace 256		// max chars for slot strings.

#define CHALLENGE_X				D_W + 16
#define CHALLENGE_Y				D_H + 5
#define CHALLENGE_W				610
#define CHALLENGE_H				220

#define CHALLENGE_HGAP			9
#define CHALLENGE_VGAP			9
#define CHALLENGE_BANNER_DEPTH	40 		//top banner which displays either load or save

#define CHALLENGE_ENTRY_W				((CHALLENGE_W / 3 )-(3 * CHALLENGE_HGAP))
#define CHALLENGE_ENTRY_H				(CHALLENGE_H -(5 * CHALLENGE_VGAP )- (CHALLENGE_BANNER_DEPTH+CHALLENGE_VGAP) ) /5

#define ID_LOADSAVE				21000
#define CHALLENGE_FORM			ID_LOADSAVE+1		// back form.
#define CHALLENGE_CANCEL			ID_LOADSAVE+2		// cancel but.
#define CHALLENGE_LABEL			ID_LOADSAVE+3		// load/save
#define CHALLENGE_BANNER			ID_LOADSAVE+4		// banner.

#define CHALLENGE_ENTRY_START			ID_LOADSAVE+10		// each of the buttons.
#define CHALLENGE_ENTRY_END			ID_LOADSAVE+10 +totalslots  // must have unique ID hmm -Q

static	W_SCREEN	*psRequestScreen;					// Widget screen for requester

bool		challengesUp = false;		///< True when interface is up and should be run.
bool		challengeActive = false;	///< Whether we are running a challenge

static void displayLoadBanner(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	PIELIGHT col = WZCOL_GREEN;
	UDWORD	x = xOffset + psWidget->x;
	UDWORD	y = yOffset + psWidget->y;

	pie_BoxFill(x, y, x + psWidget->width, y + psWidget->height, col);
	pie_BoxFill(x + 2, y + 2, x + psWidget->width - 2, y + psWidget->height - 2, WZCOL_MENU_BACKGROUND);
}

// ////////////////////////////////////////////////////////////////////////////
static void displayLoadSlot(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{

	UDWORD	x = xOffset + psWidget->x;
	UDWORD	y = yOffset + psWidget->y;
	char  butString[64];

	drawBlueBox(x, y, psWidget->width, psWidget->height);	//draw box

	if (((W_BUTTON *)psWidget)->pText)
	{
		sstrcpy(butString, ((W_BUTTON *)psWidget)->pText);

		iV_SetFont(font_regular);									// font
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);

		while (iV_GetTextWidth(butString) > psWidget->width)
		{
			butString[strlen(butString)-1] = '\0';
		}

		//draw text
		iV_DrawText(butString, x + 4, y + 17);
	}
}

//****************************************************************************************
// Challenge menu
//*****************************************************************************************
bool addChallenges()
{
	char			sPath[PATH_MAX];
	const char *sSearchPath	= "challenges";
	W_FORMINIT		sFormInit;
	W_BUTINIT		sButInit;
	W_LABINIT		sLabInit;
	UDWORD			slotCount;
	static char		sSlotCaps[totalslots][totalslotspace];
	static char		sSlotTips[totalslots][totalslotspace];
	static char		sSlotFile[totalslots][totalslotspace];
	char **i, **files;

	(void) PHYSFS_mkdir(sSearchPath); // just in case

	psRequestScreen = widgCreateScreen(); // init the screen
	widgSetTipFont(psRequestScreen, font_regular);

	/* add a form to place the tabbed form on */
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = 0;				//this adds the blue background, and the "box" behind the buttons -Q
	sFormInit.id = CHALLENGE_FORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = (SWORD) CHALLENGE_X;
	sFormInit.y = (SWORD) CHALLENGE_Y;
	sFormInit.width = CHALLENGE_W;
	// we need the form to be long enough for all resolutions, so we take the total number of items * height
	// and * the gaps, add the banner, and finally, the fudge factor ;)
	sFormInit.height = (slotsInColumn * CHALLENGE_ENTRY_H + CHALLENGE_HGAP * slotsInColumn) + CHALLENGE_BANNER_DEPTH + 20;
	sFormInit.disableChildren = true;
	sFormInit.pDisplay = intOpenPlainForm;
	widgAddForm(psRequestScreen, &sFormInit);

	// Add Banner
	sFormInit.formID = CHALLENGE_FORM;
	sFormInit.id = CHALLENGE_BANNER;
	sFormInit.x = CHALLENGE_HGAP;
	sFormInit.y = CHALLENGE_VGAP;
	sFormInit.width = CHALLENGE_W - (2 * CHALLENGE_HGAP);
	sFormInit.height = CHALLENGE_BANNER_DEPTH;
	sFormInit.disableChildren = false;
	sFormInit.pDisplay = displayLoadBanner;
	sFormInit.UserData = 0;
	widgAddForm(psRequestScreen, &sFormInit);

	// Add Banner Label
	memset(&sLabInit, 0, sizeof(W_LABINIT));
	sLabInit.formID		= CHALLENGE_BANNER;
	sLabInit.id		= CHALLENGE_LABEL;
	sLabInit.style		= WLAB_ALIGNCENTRE;
	sLabInit.x		= 0;
	sLabInit.y		= 3;
	sLabInit.width		= CHALLENGE_W - (2 * CHALLENGE_HGAP);	//CHALLENGE_W;
	sLabInit.height		= CHALLENGE_BANNER_DEPTH;		//This looks right -Q
	sLabInit.pText		= "Challenge";
	sLabInit.FontID		= font_regular;
	widgAddLabel(psRequestScreen, &sLabInit);

	// add cancel.
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID = CHALLENGE_BANNER;
	sButInit.x = 8;
	sButInit.y = 8;
	sButInit.width		= iV_GetImageWidth(IntImages, IMAGE_NRUTER);
	sButInit.height		= iV_GetImageHeight(IntImages, IMAGE_NRUTER);
	sButInit.UserData	= PACKDWORD_TRI(0, IMAGE_NRUTER , IMAGE_NRUTER);

	sButInit.id = CHALLENGE_CANCEL;
	sButInit.style = WBUT_PLAIN;
	sButInit.pTip = _("Close");
	sButInit.FontID = font_regular;
	sButInit.pDisplay = intDisplayImageHilight;
	widgAddButton(psRequestScreen, &sButInit);

	// add slots
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID		= CHALLENGE_FORM;
	sButInit.style		= WBUT_PLAIN;
	sButInit.width		= CHALLENGE_ENTRY_W;
	sButInit.height		= CHALLENGE_ENTRY_H;
	sButInit.pDisplay	= displayLoadSlot;
	sButInit.FontID		= font_regular;

	for (slotCount = 0; slotCount < totalslots; slotCount++)
	{
		sButInit.id		= slotCount + CHALLENGE_ENTRY_START;

		if (slotCount < slotsInColumn)
		{
			sButInit.x	= 22 + CHALLENGE_HGAP;
			sButInit.y	= (SWORD)((CHALLENGE_BANNER_DEPTH + (2 * CHALLENGE_VGAP)) + (
			                         slotCount * (CHALLENGE_VGAP + CHALLENGE_ENTRY_H)));
		}
		else if (slotCount >= slotsInColumn && (slotCount < (slotsInColumn *2)))
		{
			sButInit.x	= 22 + (2 * CHALLENGE_HGAP + CHALLENGE_ENTRY_W);
			sButInit.y	= (SWORD)((CHALLENGE_BANNER_DEPTH + (2 * CHALLENGE_VGAP)) + (
			                         (slotCount % slotsInColumn) * (CHALLENGE_VGAP + CHALLENGE_ENTRY_H)));
		}
		else
		{
			sButInit.x	= 22 + (3 * CHALLENGE_HGAP + (2 * CHALLENGE_ENTRY_W));
			sButInit.y	= (SWORD)((CHALLENGE_BANNER_DEPTH + (2 * CHALLENGE_VGAP)) + (
			                         (slotCount % slotsInColumn) * (CHALLENGE_VGAP + CHALLENGE_ENTRY_H)));
		}
		widgAddButton(psRequestScreen, &sButInit);
	}

	// fill slots.
	slotCount = 0;

	sstrcpy(sPath, sSearchPath);
	sstrcat(sPath, "/*.ini");

	debug(LOG_SAVE, "Searching \"%s\" for challenges", sPath);

	// add challenges to buttons
	files = PHYSFS_enumerateFiles(sSearchPath);
	for (i = files; *i != NULL; ++i)
	{
		W_BUTTON *button;
		char description[totalslotspace];
		char highscore[totalslotspace];
		const char *name, *difficulty, *map, *givendescription;
		dictionary *dict;

		// See if this filename contains the extension we're looking for
		if (!strstr(*i, ".ini"))
		{
			// If it doesn't, move on to the next filename
			continue;
		}

		/* First grab any high score associated with this challenge */
		dict = iniparser_load(CHALLENGE_SCORES);
		sstrcpy(sPath, *i);
		sPath[strlen(sPath) - 4] = '\0';	// remove .ini
		sstrcpy(highscore, "no score");
		if (dict)
		{
			char key[64];
			bool victory;
			int seconds;

			ssprintf(key, "%s:Player", sPath);
			name = iniparser_getstring(dict, key, "NO NAME");
			ssprintf(key, "%s:Victory", sPath);
			victory = iniparser_getboolean(dict, key, false);
			ssprintf(key, "%s:Seconds", sPath);
			seconds = iniparser_getint(dict, key, -1);
			if (seconds > 0)
			{
				getAsciiTime(key, seconds * GAME_TICKS_PER_SEC);
				ssprintf(highscore, "%s by %s (%s)", key, name, victory ? "Victory" : "Survived");
			}
			iniparser_freedict(dict);
		}

		ssprintf(sPath, "%s/%s", sSearchPath, *i);
		dict = iniparser_load(sPath);
		if (!dict)
		{
			debug(LOG_ERROR, "Could not open \"%s\"", sPath);
			continue;
		}
		name = iniparser_getstring(dict, "challenge:Name", "BAD NAME");
		map = iniparser_getstring(dict, "challenge:Map", "BAD MAP");
		difficulty = iniparser_getstring(dict, "challenge:difficulty", "BAD DIFFICULTY");
		givendescription = iniparser_getstring(dict, "challenge:description", "");
		ssprintf(description, "%s, %s, %s. %s", map, difficulty, highscore, givendescription);

		button = (W_BUTTON*)widgGetFromID(psRequestScreen, CHALLENGE_ENTRY_START + slotCount);

		debug(LOG_SAVE, "We found [%s]", *i);

		/* Set the button-text */
		sstrcpy(sSlotCaps[slotCount], name);		// store it!
		sstrcpy(sSlotTips[slotCount], description);	// store it, too!
		sstrcpy(sSlotFile[slotCount], sPath);		// store filename
		iniparser_freedict(dict);

		/* Add button */
		button->pTip = sSlotTips[slotCount];
		button->pText = sSlotCaps[slotCount];
		button->pUserData = (void *)sSlotFile[slotCount];
		slotCount++;		// go to next button...
		if (slotCount == totalslots)
		{
			break;
		}
	}
	PHYSFS_freeList(files);

	challengesUp = true;

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
bool closeChallenges()
{
	widgDelete(psRequestScreen, CHALLENGE_FORM);
	widgReleaseScreen(psRequestScreen);
	// need to "eat" up the return key so it don't pass back to game.
	inputLooseFocus();
	challengesUp = false;
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Returns true if cancel pressed or a valid game slot was selected.
// if when returning true strlen(sRequestResult) != 0 then a valid game
// slot was selected otherwise cancel was selected..
bool runChallenges(void)
{
	UDWORD		id = 0;

	id = widgRunScreen(psRequestScreen);

	sstrcpy(sRequestResult, "");					// set returned filename to null;

	// cancel this operation...
	if (id == CHALLENGE_CANCEL || CancelPressed())
	{
		goto failure;
	}

	// clicked a load entry
	if (id >= CHALLENGE_ENTRY_START  &&  id <= CHALLENGE_ENTRY_END)
	{
		if (((W_BUTTON *)widgGetFromID(psRequestScreen, id))->pText)
		{
			sstrcpy(sRequestResult, (const char *)((W_BUTTON *)widgGetFromID(psRequestScreen, id))->pUserData);
		}
		else
		{
			goto failure;				// clicked on an empty box
		}
		goto success;
	}

	return false;

// failed and/or cancelled..
failure:
	closeChallenges();
	challengeActive = false;
	return false;

// success on load.
success:
	closeChallenges();
	challengeActive = true;
	ingame.bHostSetup = true;
	changeTitleMode(MULTIOPTION);
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// should be done when drawing the other widgets.
bool displayChallenges()
{
	widgDisplayScreen(psRequestScreen);	// display widgets.
	return true;
}

