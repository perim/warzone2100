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
 *  MultiMenu.c
 *  Handles the In Game MultiPlayer Screen, alliances etc...
 *  Also the selection of disk files..
 */
#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/widget/button.h"
#include "lib/widget/widget.h"

#include "display3d.h"
#include "intdisplay.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_common/pieblitfunc.h"
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/piematrix.h"
#include "levels.h"
#include "objmem.h"		 	//for droid lists.
#include "component.h"		// for disaplycomponentobj.
#include "hci.h"			// for wFont def.& intmode.
#include "init.h"
#include "power.h"
#include "loadsave.h"		// for drawbluebox
#include "console.h"
#include "ai.h"
#include "frend.h"
#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "multistat.h"
#include "multimenu.h"
#include "multiint.h"
#include "multigifts.h"
#include "multijoin.h"
#include "mission.h"
#include "scores.h"
#include "keymap.h"
#include "loop.h"
#include "lib/framework/frameint.h"

// ////////////////////////////////////////////////////////////////////////////
// defines

W_SCREEN  *psRScreen;			// requester stuff.

extern char	MultiCustomMapsPath[PATH_MAX];
extern void	displayMultiBut(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

BOOL	MultiMenuUp			= false;
BOOL	ClosingMultiMenu	= false;
BOOL	DebugMenuUp		= false;
static UDWORD	context = 0;
UDWORD	current_tech = 1;
UDWORD	current_numplayers = 4;

#define DEBUGMENU_FORM_W		200
#define DEBUGMENU_FORM_H		300
#define DEBUGMENU_FORM_X		(screenWidth - DEBUGMENU_FORM_W)		//pie_GetVideoBufferWidth() ?
#define DEBUGMENU_FORM_Y		110 + D_H

#define DEBUGMENU_ENTRY_H		20

#define MULTIMENU_FORM_X		10 + D_W
#define MULTIMENU_FORM_Y		23 + D_H
#define MULTIMENU_FORM_W		620
#define MULTIMENU_FORM_H		295

#define MULTIMENU_PLAYER_H		32
#define MULTIMENU_FONT_OSET		20

#define MULTIMENU_C0			(MULTIMENU_C2+95)
#define MULTIMENU_C1			30
#define MULTIMENU_C2			(MULTIMENU_C1+30)
#define MULTIMENU_C3			(MULTIMENU_C0+36)
#define MULTIMENU_C4			(MULTIMENU_C3+36)
#define MULTIMENU_C5			(MULTIMENU_C4+32)
#define MULTIMENU_C6			(MULTIMENU_C5+32)
#define MULTIMENU_C7			(MULTIMENU_C6+32)
#define MULTIMENU_C8			(MULTIMENU_C7+45)
#define MULTIMENU_C9			(MULTIMENU_C8+95)
#define MULTIMENU_C10			(MULTIMENU_C9+50)
#define MULTIMENU_C11			(MULTIMENU_C10+45)

#define MULTIMENU_CLOSE			(MULTIMENU+1)
#define MULTIMENU_PLAYER		(MULTIMENU+2)

#define	MULTIMENU_ALLIANCE_BASE (MULTIMENU_PLAYER        +MAX_PLAYERS)
#define	MULTIMENU_GIFT_RAD		(MULTIMENU_ALLIANCE_BASE +MAX_PLAYERS)
#define	MULTIMENU_GIFT_RES		(MULTIMENU_GIFT_RAD		 +MAX_PLAYERS)
#define	MULTIMENU_GIFT_DRO		(MULTIMENU_GIFT_RES		 +MAX_PLAYERS)
#define	MULTIMENU_GIFT_POW		(MULTIMENU_GIFT_DRO		 +MAX_PLAYERS)
#define MULTIMENU_CHANNEL		(MULTIMENU_GIFT_POW		 +MAX_PLAYERS)

#define MULTIMENU_STOPS			50
#define MULTIMENU_MIDPOS		(MULTIMENU_STOPS/2)
#define MULTIMENU_MULTIPLIER	((100/MULTIMENU_STOPS)*2)

/// requester stuff.
#define M_REQUEST_CLOSE (MULTIMENU+49)
#define M_REQUEST		(MULTIMENU+50)
#define M_REQUEST_TAB	(MULTIMENU+51)

#define M_REQUEST_C1	(MULTIMENU+61)
#define M_REQUEST_C2	(MULTIMENU+62)
#define M_REQUEST_C3	(MULTIMENU+63)

#define M_REQUEST_AP	(MULTIMENU+70)
#define M_REQUEST_2P	(MULTIMENU+71)
#define M_REQUEST_3P	(MULTIMENU+72)
#define M_REQUEST_4P	(MULTIMENU+73)
#define M_REQUEST_5P	(MULTIMENU+74)
#define M_REQUEST_6P	(MULTIMENU+75)
#define M_REQUEST_7P	(MULTIMENU+76)
#define M_REQUEST_8P	(MULTIMENU+77)

#define M_REQUEST_BUT	(MULTIMENU+100)		// allow loads of buttons.
#define M_REQUEST_BUTM	(MULTIMENU+1100)

#define M_REQUEST_X		MULTIOP_PLAYERSX
#define M_REQUEST_Y		MULTIOP_PLAYERSY
#define M_REQUEST_W		MULTIOP_PLAYERSW
#define M_REQUEST_H		MULTIOP_PLAYERSH

#define	R_BUT_W			105//112
#define R_BUT_H			30

BOOL			multiRequestUp = false;				//multimenu is up.
static BOOL		giftsUp[MAX_PLAYERS] = {true};		//gift buttons for player are up.

char		debugMenuEntry[DEBUGMENU_MAX_ENTRIES][MAX_STR_LENGTH];

// ////////////////////////////////////////////////////////////////////////////
// Map / force / name load save stuff.

/* Sets the player's text color depending on if alliance formed, or if dead.
 * \param mode  the specified alliance
 * \param player the specified player
 */
static void SetPlayerTextColor( int mode, UDWORD player )
{
	// override color if they are dead...
	if (!apsDroidLists[player] && !apsStructLists[player])
	{
		iV_SetTextColour(WZCOL_GREY);			// dead text color
	}
	// the colors were chosen to match the FRIEND/FOE radar map colors.
	else if (mode == ALLIANCE_FORMED)
	{
		iV_SetTextColour(WZCOL_YELLOW);			// Human alliance text color
	}
	else if (isHumanPlayer(player))				// Human player, no alliance
	{
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);	// Normal text color
	}
	else
	{
		iV_SetTextColour(WZCOL_RED);			// Enemy color
	}
}
// ////////////////////////////////////////////////////////////////////////////
// enumerates maps in the gamedesc file.
// returns only maps that are valid the right 'type'
static char* enumerateMultiMaps(UDWORD* players, bool first, unsigned int camToUse, unsigned int numPlayers)
{
	static LEVEL_DATASET *lev;
	unsigned int cam;

	if(first)
	{
		lev = psLevels;
	}
	while(lev)
	{
		if(game.type == SKIRMISH)
		{
			if(lev->type == MULTI_SKIRMISH2)
			{
				cam = 2;
			}
			else if(lev->type == MULTI_SKIRMISH3)
			{
				cam = 3;
			}
			else
			{
				cam = 1;
			}

			if((lev->type == SKIRMISH || lev->type == MULTI_SKIRMISH2 || lev->type == MULTI_SKIRMISH3)
				&& (numPlayers == 0 || numPlayers == lev->players)
				&& cam == camToUse )
			{
				char* const found = strdup(lev->pName);
				if (found == NULL)
				{
					debug(LOG_FATAL, "Out of memory");
					// No way to indicate out-of-memory failure by return value
					abort();
					return NULL;
				}

				*players = lev->players;
				lev = lev->psNext;
				return found;
			}
		}
		else	//  campaign
		{
// 'service pack 1'
			if(lev->type == MULTI_CAMPAIGN2)
			{
				cam = 2;
			}
			else if(lev->type == MULTI_CAMPAIGN3)
			{
				cam = 3;
			}
			else
			{
				cam = 1;
			}
//	end of service pack

			if ((lev->type == CAMPAIGN || lev->type == MULTI_CAMPAIGN2 || lev->type == MULTI_CAMPAIGN3)
			    && (numPlayers == 0 || numPlayers == lev->players)
			    && cam == camToUse )
			{
				char* const found = strdup(lev->pName);
				if (found == NULL)
				{
					debug(LOG_FATAL, "Out of memory");
					// No way to indicate out-of-memory failure by return value
					abort();
					return NULL;
				}

				*players = lev->players;
				lev = lev->psNext;
				return found;
			}
		}
		lev = lev->psNext;
	}

	return NULL;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
void displayRequestOption(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{

	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;
	UDWORD	count;
	char  butString[255];

	strcpy(butString,((W_BUTTON *)psWidget)->pTip);

	drawBlueBox(x,y,psWidget->width,psWidget->height);	//draw box

	iV_SetFont(font_regular);					// font
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);

	while(iV_GetTextWidth(butString) > psWidget->width -10 )
	{
		butString[strlen(butString)-1]='\0';
	}

	iV_DrawText(butString, x + 6, y + 12);	//draw text

	// if map, then draw no. of players.
	for(count=0;count<psWidget->UserData;count++)
	{
		iV_DrawImage(FrontImages,IMAGE_WEE_GUY,(x+(6*count)+6),y+16);
	}
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////

static void displayCamTypeBut(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;
	char buffer[8];

	drawBlueBox(x,y,psWidget->width,psWidget->height);	//draw box
	sprintf(buffer, "T%i", (int)(psWidget->UserData));
	if ((unsigned int)(psWidget->UserData) == current_tech) {
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	} else {
		iV_SetTextColour(WZCOL_TEXT_MEDIUM);
	}
	iV_DrawText(buffer, x+2, y+12);
}

static void displayNumPlayersBut(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD	x = xOffset+psWidget->x;
	UDWORD	y = yOffset+psWidget->y;
	char buffer[8];

	drawBlueBox(x,y,psWidget->width,psWidget->height);	//draw box
	if ((unsigned int)(psWidget->UserData) == current_numplayers) {
		iV_SetTextColour(WZCOL_TEXT_BRIGHT);
	} else {
		iV_SetTextColour(WZCOL_TEXT_MEDIUM);
	}
	if ((unsigned int)(psWidget->UserData) == 0) {
		sprintf(buffer, " *");
	} else {
		sprintf(buffer, "%iP", (int)(psWidget->UserData));
	}
	iV_DrawText(buffer, x+2, y+12);

}

#define NBTIPS 512

static unsigned int check_tip_index(unsigned int i) {
	if (i < NBTIPS) {
		return i;
	} else {
		debug(LOG_MAIN, "Tip window index too high (%ud)", i);
		return NBTIPS-1;
	}
}

/** Searches in the given search directory for files ending with the
 *  given extension. Then will create a window with buttons for each
 *  found file.
 *  \param searchDir the directory to search in
 *  \param fileExtension the extension files should end with, if the
 *         extension has a dot (.) then this dot _must_ be present as
 *         the first char in this parameter
 *  \param mode (purpose unknown)
 *  \param numPlayers (purpose unknown)
 */
void addMultiRequest(const char* searchDir, const char* fileExtension, UDWORD mode, UBYTE mapCam, UBYTE numPlayers)
{
	W_FORMINIT         sFormInit;
	W_BUTINIT          sButInit;
	UDWORD             players;
	char**             fileList;
	char**             currFile;
	const unsigned int extensionLength = strlen(fileExtension);
	const unsigned int buttonsX = (mode == MULTIOP_MAP) ? 22 : 17;

	unsigned int       numButtons, count, butPerForm, i;

	static char		tips[NBTIPS][MAX_STR_LENGTH];


	context = mode;
	if(mode == MULTIOP_MAP)
	{	// only save these when they select MAP button
		current_tech = mapCam;
		current_numplayers = numPlayers;
	}
	fileList = PHYSFS_enumerateFiles(searchDir);
	if (!fileList)
	{
		debug(LOG_FATAL, "addMultiRequest: Out of memory");
		abort();
		return;
	}

	// Count number of required buttons
	numButtons = 0;
	for (currFile = fileList; *currFile != NULL; ++currFile)
	{
		const unsigned int fileNameLength = strlen(*currFile);

		// Check to see if this file matches the given extension
		if (fileNameLength > extensionLength
		 && strcmp(&(*currFile)[fileNameLength - extensionLength], fileExtension) == 0)
			++numButtons;
	}

	if(mode == MULTIOP_MAP)									// if its a map, also look in the predone stuff.
	{
		char* map;
		if ((map = enumerateMultiMaps(&players, true, mapCam, numPlayers)))
		{
			free(map);
			numButtons++;
			while ((map = enumerateMultiMaps(&players, false, mapCam, numPlayers)))
			{
				free(map);
				numButtons++;
			}
		}
	}

	psRScreen = widgCreateScreen(); ///< move this to intinit or somewhere like that.. (close too.)
	widgSetTipFont(psRScreen,font_regular);

	/* Calculate how many buttons will go on a single form */
	butPerForm = ((M_REQUEST_W - 0 - 4) /
						(R_BUT_W +4)) *
				 ((M_REQUEST_H - 0- 4) /
						(R_BUT_H+ 4));

	/* add a form to place the tabbed form on */
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = 0;
	sFormInit.id = M_REQUEST;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = (SWORD)(M_REQUEST_X+D_W);
	sFormInit.y = (SWORD)(M_REQUEST_Y+D_H);
	sFormInit.width = M_REQUEST_W;
	sFormInit.height = M_REQUEST_H;
	sFormInit.disableChildren = true;
	sFormInit.pDisplay = intOpenPlainForm;
	widgAddForm(psRScreen, &sFormInit);

	/* Add the tabs */
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = M_REQUEST;
	sFormInit.id = M_REQUEST_TAB;
	sFormInit.style = WFORM_TABBED;
	sFormInit.x = 2;
	sFormInit.y = 2;
	sFormInit.width = M_REQUEST_W;
	sFormInit.height = M_REQUEST_H-4;

	sFormInit.numMajor = numForms(numButtons, butPerForm);

	sFormInit.majorPos = WFORM_TABTOP;
	sFormInit.minorPos = WFORM_TABNONE;
	sFormInit.majorSize = OBJ_TABWIDTH+2;
	sFormInit.majorOffset = OBJ_TABOFFSET;
	sFormInit.tabVertOffset = (OBJ_TABHEIGHT/2);
	sFormInit.tabMajorThickness = OBJ_TABHEIGHT;
	sFormInit.pUserData = &StandardTab;
	sFormInit.pTabDisplay = intDisplayTab;

	// TABFIXME: 
	// This appears to be the map pick screen, when we have lots of maps
	// this will need to change.
	if (sFormInit.numMajor > MAX_TAB_STD_SHOWN)
	{
		ASSERT(sFormInit.numMajor < MAX_TAB_SMALL_SHOWN,"Too many maps! Need scroll tabs here.");
		sFormInit.pUserData = &SmallTab;
		sFormInit.majorSize /= 2;
	}

	for (i = 0; i < sFormInit.numMajor; ++i)
	{
		sFormInit.aNumMinors[i] = 2;
	}
	widgAddForm(psRScreen, &sFormInit);

	// Add the close button.
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID = M_REQUEST;
	sButInit.id = M_REQUEST_CLOSE;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = M_REQUEST_W - CLOSE_WIDTH - 3;
	sButInit.y = 0;
	sButInit.width = CLOSE_WIDTH;
	sButInit.height = CLOSE_HEIGHT;
	sButInit.pTip = _("Close");
	sButInit.FontID = font_regular;
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0,IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	widgAddButton(psRScreen, &sButInit);

	/* Put the buttons on it *//* Set up the button struct */
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID		= M_REQUEST_TAB;
	sButInit.id		= M_REQUEST_BUT;
	sButInit.style		= WBUT_PLAIN;
	sButInit.x		= buttonsX;
	sButInit.y		= 4;
	sButInit.width		= R_BUT_W;
	sButInit.height		= R_BUT_H;
	sButInit.pUserData	= NULL;
	sButInit.pDisplay	= displayRequestOption;
	sButInit.FontID		= font_regular;

	for (currFile = fileList, count = 0; *currFile != NULL && count < (butPerForm * 4); ++currFile)
	{
		const unsigned int tip_index = check_tip_index(sButInit.id - M_REQUEST_BUT);
		const unsigned int fileNameLength = strlen(*currFile);
		const unsigned int tipStringLength = fileNameLength - extensionLength;

		// Check to see if this file matches the given extension
		if (!(fileNameLength > extensionLength)
		 || strcmp(&(*currFile)[fileNameLength - extensionLength], fileExtension) != 0)
			continue;

		// Set the tip and add the button

		// Copy all of the filename except for the extension into the tiptext string
		strlcpy(tips[tip_index], *currFile, MIN(tipStringLength + 1, sizeof(tips[tip_index])));

		sButInit.pTip		= tips[tip_index];
		sButInit.pText		= tips[tip_index];

		if(mode == MULTIOP_MAP)											// if its a map, set player flag.
		{
			const char* mapText;
			unsigned int mapTextLength;

			sButInit.UserData = (*currFile)[0] - '0';

			if( (*currFile)[1] != 'c')
			{
				continue;
			}

			// Chop off description
			mapText = strrchr(*currFile, '-') + 1;
			if (mapText - 1 == NULL)
				continue;

			mapTextLength = tipStringLength - (mapText - *currFile);
			strlcpy(tips[tip_index], mapText, MIN(mapTextLength + 1, sizeof(tips[tip_index])));
		}

		++count;
		widgAddButton(psRScreen, &sButInit);

		/* Update the init struct for the next button */
		sButInit.id += 1;
		sButInit.x = (SWORD)(sButInit.x + (R_BUT_W+ 4));
		if (sButInit.x + R_BUT_W+ 2 > M_REQUEST_W)
		{
			sButInit.x = buttonsX;
			sButInit.y = (SWORD)(sButInit.y +R_BUT_H + 4);
		}
		if (sButInit.y +R_BUT_H + 4 > M_REQUEST_H)
		{
			sButInit.y = 4;
			sButInit.majorID += 1;
		}
	}

	// Make sure to return memory back to PhyscisFS
	PHYSFS_freeList(fileList);

	if(mode == MULTIOP_MAP)
	{
		char* mapName;
		if ((mapName = enumerateMultiMaps(&players, true, mapCam, numPlayers)))
		{
			do
			{
				unsigned int tip_index = check_tip_index(sButInit.id-M_REQUEST_BUT);

				// add number of players to string.
				sstrcpy(tips[tip_index], mapName);
				free(mapName);

				sButInit.pTip = tips[tip_index];
				sButInit.pText = tips[tip_index];
				sButInit.UserData	= players;

				widgAddButton(psRScreen, &sButInit);

				sButInit.id += 1;
				sButInit.x = (SWORD)(sButInit.x + (R_BUT_W+ 4));
				if (sButInit.x + R_BUT_W+ 2 > M_REQUEST_W)
				{
					sButInit.x = buttonsX;
					sButInit.y = (SWORD)(sButInit.y +R_BUT_H + 4);
				}
				if (sButInit.y +R_BUT_H + 4 > M_REQUEST_H)
				{
					sButInit.y = 4;
					sButInit.majorID += 1;
				}
			} while ((mapName = enumerateMultiMaps(&players, false, mapCam, numPlayers)));
		}
	}
	multiRequestUp = true;


	// if it's map select then add the cam style buttons.
	if(mode == MULTIOP_MAP)
	{
		memset(&sButInit, 0, sizeof(W_BUTINIT));
		sButInit.formID		= M_REQUEST_TAB;
		sButInit.id		= M_REQUEST_C1;
		sButInit.style		= WBUT_PLAIN;
		sButInit.x		= 1;
		sButInit.y		= 252;
		sButInit.width		= 17;
		sButInit.height		= 17;
		sButInit.UserData	= 1;
		sButInit.FontID		= font_regular;
		sButInit.pTip		= _("Technology level 1");
		sButInit.pDisplay	= displayCamTypeBut;

		widgAddButton(psRScreen, &sButInit);

		sButInit.id		= M_REQUEST_C2;
		sButInit.y		+= 22;
		sButInit.UserData	= 2;
		sButInit.pTip		= _("Technology level 2");
		widgAddButton(psRScreen, &sButInit);

		sButInit.id		= M_REQUEST_C3;
		sButInit.y		+= 22;
		sButInit.UserData	= 3;
		sButInit.pTip		= _("Technology level 3");
		widgAddButton(psRScreen, &sButInit);

		sButInit.id		= M_REQUEST_AP;
		sButInit.y		= 17;
		sButInit.UserData	= 0;
		sButInit.pTip		= _("Any number of players");
		sButInit.pDisplay	= displayNumPlayersBut;
		widgAddButton(psRScreen, &sButInit);

		sButInit.id		= M_REQUEST_2P;
		sButInit.y		+= 22;
		sButInit.UserData	= 2;
		sButInit.pTip		= _("2 players");
		widgAddButton(psRScreen, &sButInit);

		sButInit.id		= M_REQUEST_3P;
		sButInit.y		+= 22;
		sButInit.UserData	= 3;
		sButInit.pTip		= _("3 players");
		widgAddButton(psRScreen, &sButInit);

		sButInit.id		= M_REQUEST_4P;
		sButInit.y		+= 22;
		sButInit.UserData	= 4;
		sButInit.pTip		= _("4 players");
		widgAddButton(psRScreen, &sButInit);

		sButInit.id		= M_REQUEST_5P;
		sButInit.y		+= 22;
		sButInit.UserData	= 5;
		sButInit.pTip		= _("5 players");
		widgAddButton(psRScreen, &sButInit);

		sButInit.id		= M_REQUEST_6P;
		sButInit.y		+= 22;
		sButInit.UserData	= 6;
		sButInit.pTip		= _("6 players");
		widgAddButton(psRScreen, &sButInit);

		sButInit.id		= M_REQUEST_7P;
		sButInit.y		+= 22;
		sButInit.UserData	= 7;
		sButInit.pTip		= _("7 players");
		widgAddButton(psRScreen, &sButInit);

		sButInit.id		= M_REQUEST_8P;
		sButInit.y		+= 22;
		sButInit.UserData	= 8;
		sButInit.pTip		= _("8 players");
		widgAddButton(psRScreen, &sButInit);
	}

}

static void closeMultiRequester(void)
{
	widgDelete(psRScreen,M_REQUEST);
	multiRequestUp = false;

	widgReleaseScreen(psRScreen);		// move this to the frontend shutdown...
	return;
}

BOOL runMultiRequester(UDWORD id,UDWORD *mode, char *chosen,UDWORD *chosenValue)
{
	if( id==M_REQUEST_CLOSE)							// close
	{
		closeMultiRequester();
		return true;
	}

	if( id>=M_REQUEST_BUT && id<=M_REQUEST_BUTM)		// chose a file.
	{
		strcpy(chosen,((W_BUTTON *)widgGetFromID(psRScreen,id))->pText );

		*chosenValue = ((W_BUTTON *)widgGetFromID(psRScreen,id))->UserData ;
		closeMultiRequester();
		*mode = context;

		return true;
	}

	switch (id)
	{
		case M_REQUEST_C1:
			closeMultiRequester();
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, 1, current_numplayers);
			break;
		case M_REQUEST_C2:
			closeMultiRequester();
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, 2, current_numplayers);
			break;
		case M_REQUEST_C3:
			closeMultiRequester();
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, 3, current_numplayers);
			break;
		case M_REQUEST_AP:
			closeMultiRequester();
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, current_tech, 0);
			break;
		case M_REQUEST_2P:
			closeMultiRequester();
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, current_tech, 2);
			break;
		case M_REQUEST_3P:
			closeMultiRequester();
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, current_tech, 3);
			break;
		case M_REQUEST_4P:
			closeMultiRequester();
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, current_tech, 4);
			break;
		case M_REQUEST_5P:
			closeMultiRequester();
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, current_tech, 5);
			break;
		case M_REQUEST_6P:
			closeMultiRequester();
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, current_tech, 6);
			break;
		case M_REQUEST_7P:
			closeMultiRequester();
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, current_tech, 7);
			break;
		case M_REQUEST_8P:
			closeMultiRequester();
			addMultiRequest(MultiCustomMapsPath, ".wrf", MULTIOP_MAP, current_tech, 8);
			break;
	}

	return false;
}


// ////////////////////////////////////////////////////////////////////////////
// Multiplayer in game menu stuff

// ////////////////////////////////////////////////////////////////////////////
// Display Functions


static void displayExtraGubbins(UDWORD height)
{
	char	str[128];

	//draw grid
	iV_Line(MULTIMENU_FORM_X+MULTIMENU_C0 -6 , MULTIMENU_FORM_Y,
			MULTIMENU_FORM_X+MULTIMENU_C0 -6 , MULTIMENU_FORM_Y+height, WZCOL_BLACK);

	iV_Line(MULTIMENU_FORM_X+MULTIMENU_C8 -6 , MULTIMENU_FORM_Y,
			MULTIMENU_FORM_X+MULTIMENU_C8 -6 , MULTIMENU_FORM_Y+height, WZCOL_BLACK);

	iV_Line(MULTIMENU_FORM_X+MULTIMENU_C9 -6 , MULTIMENU_FORM_Y,
			MULTIMENU_FORM_X+MULTIMENU_C9 -6 , MULTIMENU_FORM_Y+height, WZCOL_BLACK);

	iV_Line(MULTIMENU_FORM_X+MULTIMENU_C10 -6 , MULTIMENU_FORM_Y,
			MULTIMENU_FORM_X+MULTIMENU_C10 -6 , MULTIMENU_FORM_Y+height, WZCOL_BLACK);

	iV_Line(MULTIMENU_FORM_X+MULTIMENU_C11 -6 , MULTIMENU_FORM_Y,
			MULTIMENU_FORM_X+MULTIMENU_C11 -6 , MULTIMENU_FORM_Y+height, WZCOL_BLACK);

	iV_Line(MULTIMENU_FORM_X				, MULTIMENU_FORM_Y+MULTIMENU_PLAYER_H,
			MULTIMENU_FORM_X+MULTIMENU_FORM_W, MULTIMENU_FORM_Y+MULTIMENU_PLAYER_H, WZCOL_BLACK);

	iV_SetFont(font_regular);						// font
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);			// main wz text color

	// draw timer
	getAsciiTime(str, gameTime);
	iV_DrawText(str, MULTIMENU_FORM_X+MULTIMENU_C2, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET) ;

	// draw titles.
	iV_DrawText(_("Alliances"), MULTIMENU_FORM_X+MULTIMENU_C0, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET);
	iV_DrawText(_("Score"), MULTIMENU_FORM_X+MULTIMENU_C8, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET);
	iV_DrawText(_("Kills"), MULTIMENU_FORM_X+MULTIMENU_C9, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET);

	if(getDebugMappingStatus())
	{	// shows # units for *all* players in debug mode ONLY!
		iV_DrawText(_("Units"), MULTIMENU_FORM_X+MULTIMENU_C10, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET);
		iV_DrawText(_("Power"), MULTIMENU_FORM_X+MULTIMENU_C11, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET);
	}
	else
	{	// shows # units for *yourself* (+ team member?) only.
		iV_DrawText(_("Units"), MULTIMENU_FORM_X+MULTIMENU_C10, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET);

		// ping is useless for non MP games, so display something useful depending on mode.
		if (runningMultiplayer())
		{
			iV_DrawText(_("Ping"), MULTIMENU_FORM_X+MULTIMENU_C11, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET);
		}
		else
		{
			iV_DrawText(_("Structs"), MULTIMENU_FORM_X+MULTIMENU_C11, MULTIMENU_FORM_Y+MULTIMENU_FONT_OSET);
		}
	}

#ifdef DEBUG
	{
		unsigned int width;

		sprintf(str,"Traf: %u/%u", NETgetBytesSent(), NETgetBytesRecvd());
		width = iV_GetTextWidth(str);
		iV_DrawText(str, MULTIMENU_FORM_X, MULTIMENU_FORM_Y + MULTIMENU_FORM_H);

		sprintf(str,"Pack: %u/%u", NETgetPacketsSent(), NETgetPacketsRecvd());
		iV_DrawText(str, MULTIMENU_FORM_X + 20 + width, MULTIMENU_FORM_Y + MULTIMENU_FORM_H);
	}
#endif
	return;
}


static void displayMultiPlayer(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	char			str[128];
	UDWORD			x					= xOffset+psWidget->x;
	UDWORD			y					= yOffset+psWidget->y;
	UDWORD			player = psWidget->UserData; //get the in game player number.
	Position		position;
	Vector3i 		rotation;

	if( responsibleFor(player,0) )
	{
		displayExtraGubbins(widgGetFromID(psWScreen,MULTIMENU_FORM)->height);
	}

	iV_SetFont(font_regular);											// font
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);

	if(isHumanPlayer(player) || (game.type == SKIRMISH && player<game.maxPlayers) )
	{
		ssprintf(str, "%d: %s", NetPlay.players[player].position, getPlayerName(player));
		if (isHumanPlayer(player))
		{
			SetPlayerTextColor(alliances[selectedPlayer][player], player);
		}
		else
		{
			SetPlayerTextColor(alliances[selectedPlayer][player], player);
		}

		while(iV_GetTextWidth(str) >= (MULTIMENU_C0-MULTIMENU_C2-10) )
		{
			str[strlen(str)-1]='\0';
		}
		iV_DrawText(str, x+MULTIMENU_C2, y+MULTIMENU_FONT_OSET);

		//c3-7 alliance
		//manage buttons by showing or hiding them. gifts only in campaign,
		{
			if(game.alliance != NO_ALLIANCES)
			{
				if(alliances[selectedPlayer][player] == ALLIANCE_FORMED)
				{
					if(player != selectedPlayer &&  !giftsUp[player] )
					{
						if (game.alliance != ALLIANCES_TEAMS)
						{
							widgReveal(psWScreen,MULTIMENU_GIFT_RAD+ player);
							widgReveal(psWScreen,MULTIMENU_GIFT_RES+ player);
						}
						widgReveal(psWScreen,MULTIMENU_GIFT_DRO+ player);
						widgReveal(psWScreen,MULTIMENU_GIFT_POW+ player);
						giftsUp[player] = true;
					}
				}
				else
				{
					if(player != selectedPlayer && giftsUp[player])
					{
						if (game.alliance != ALLIANCES_TEAMS)
						{
							widgHide(psWScreen,MULTIMENU_GIFT_RAD+ player);
							widgHide(psWScreen,MULTIMENU_GIFT_RES+ player);
						}
						widgHide(psWScreen,MULTIMENU_GIFT_DRO+ player);
						widgHide(psWScreen,MULTIMENU_GIFT_POW+ player);
						giftsUp[player] = false;
					}
				}
			}
		}
	}
	if(isHumanPlayer(player))
	{
		SetPlayerTextColor(alliances[selectedPlayer][player], player);

		// Let's use the real score for MP games
		if (NetPlay.bComms)
		{
			//c8:score,
			if (Cheated)
			{
				sprintf(str,"(cheated)");
			}
			else
			{
				sprintf(str,"%d",getMultiStats(player).recentScore);
			}
			iV_DrawText(str, x+MULTIMENU_C8, y+MULTIMENU_FONT_OSET);

			//c9:kills,
			sprintf(str,"%d",getMultiStats(player).recentKills);
			iV_DrawText(str, x+MULTIMENU_C9, y+MULTIMENU_FONT_OSET);
		}
		else
		{
			// estimate of score for skirmish games
			sprintf(str,"%d",ingame.skScores[player][0]);
			iV_DrawText(str, x+MULTIMENU_C8, y+MULTIMENU_FONT_OSET);
			// estimated kills
			sprintf(str,"%d",ingame.skScores[player][1]);
			iV_DrawText(str, x+MULTIMENU_C9, y+MULTIMENU_FONT_OSET);
		}

		if(!getDebugMappingStatus())
		{
			//only show player's units, and nobody elses.
			//c10:units
			if (myResponsibility(player))
			{
				SetPlayerTextColor(alliances[selectedPlayer][player], player);
				sprintf(str, "%d", getNumDroids(player) + getNumTransporterDroids(player));
				iV_DrawText(str, x+MULTIMENU_C10, y+MULTIMENU_FONT_OSET);
			}

			if (runningMultiplayer())
			{
				//c11:ping
				if (player != selectedPlayer)
				{
					if (ingame.PingTimes[player] >= 2000)
					{
						sprintf(str,"???");
					}
					else
					{
						sprintf(str, "%d", ingame.PingTimes[player]);
					}
					iV_DrawText(str, x+MULTIMENU_C11, y+MULTIMENU_FONT_OSET);
				}
			}
			else
			{
				int num;
				STRUCTURE *temp;
				// NOTE, This tallys up *all* the structures you have. Test out via 'start with no base'.
				for (num = 0, temp = apsStructLists[player]; temp != NULL;num++,temp = temp->psNext);
				//c11: Structures
				sprintf(str, "%d", num);
				iV_DrawText(str, x+MULTIMENU_C11, y+MULTIMENU_FONT_OSET);
			}
		}
	}
	else
	{
		SetPlayerTextColor(alliances[selectedPlayer][player], player);

		// Let's use the real score for MP games
		if (NetPlay.bComms)
		{
			//c8:score,
			if (Cheated)
			{
				sprintf(str,"(cheated)");
			}
			else
			{
				sprintf(str,"%d",getMultiStats(player).recentScore);
			}
			iV_DrawText(str, x+MULTIMENU_C8, y+MULTIMENU_FONT_OSET);

			//c9:kills,
			sprintf(str,"%d",getMultiStats(player).recentKills);
			iV_DrawText(str, x+MULTIMENU_C9, y+MULTIMENU_FONT_OSET);
		}
		else
		{
			// estimate of score for skirmish games
			sprintf(str,"%d",ingame.skScores[player][0]);
			iV_DrawText(str, x+MULTIMENU_C8, y+MULTIMENU_FONT_OSET);
			// estimated kills
			sprintf(str,"%d",ingame.skScores[player][1]);
			iV_DrawText(str, x+MULTIMENU_C9, y+MULTIMENU_FONT_OSET);
		}
	}

	/* Display player power instead of number of played games
	  * and number of units instead of ping when in debug mode
	  */
	if(getDebugMappingStatus())			//Won't pass this when in both release and multiplayer modes
	{
		//c10: Total number of player units in possession
		sprintf(str,"%d",getNumDroids(player) + getNumTransporterDroids(player));
		iV_DrawText(str, x+MULTIMENU_C10, y+MULTIMENU_FONT_OSET);

		//c11: Player power
		sprintf(str, "%u", (int)getPower(player));
		iV_DrawText(str, MULTIMENU_FORM_X+MULTIMENU_C11, y+MULTIMENU_FONT_OSET);
	}

	// a droid of theirs.
	if(apsDroidLists[player])
	{
		pie_SetGeometricOffset( MULTIMENU_FORM_X+MULTIMENU_C1 ,y+MULTIMENU_PLAYER_H);
		rotation.x = -15;
		rotation.y = 45;
		rotation.z = 0;
		position.x = 0;
		position.y = 0;
		position.z = 2000;		//scale them!

		displayComponentButtonObject(apsDroidLists[player],&rotation,&position,false, 100);
	}

	// clean up widgets if player leaves while menu is up.
	if(!isHumanPlayer(player) && !(game.type == SKIRMISH && player<game.maxPlayers))
	{
		if(widgGetFromID(psWScreen,MULTIMENU_CHANNEL+player))
		{
			widgDelete(psWScreen,MULTIMENU_CHANNEL+ player);
		}

		if(widgGetFromID(psWScreen,MULTIMENU_ALLIANCE_BASE+player) )
		{
			widgDelete(psWScreen,MULTIMENU_ALLIANCE_BASE+ player);
			widgDelete(psWScreen,MULTIMENU_GIFT_RAD+ player);
			widgDelete(psWScreen,MULTIMENU_GIFT_RES+ player);
			widgDelete(psWScreen,MULTIMENU_GIFT_DRO+ player);
			widgDelete(psWScreen,MULTIMENU_GIFT_POW+ player);
			giftsUp[player] = false;
		}
	}
}

static void displayDebugMenu(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	char			str[128];
	UDWORD			x					= xOffset+psWidget->x;
	UDWORD			y					= yOffset+psWidget->y;
	UDWORD			index = psWidget->UserData;

	iV_SetFont(font_regular);											// font
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);

	if(strcmp(debugMenuEntry[index],""))
	{
		sprintf(str,"%s", debugMenuEntry[index]);
		iV_DrawText(str, x, y+MULTIMENU_FONT_OSET);
	}
}


// ////////////////////////////////////////////////////////////////////////////
// alliance display funcs

static void displayAllianceState(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD a, b, c, player = psWidget->UserData;
	switch(alliances[selectedPlayer][player])
	{
	case ALLIANCE_BROKEN:
		a = 0;
		b = IMAGE_MULTI_NOAL_HI;
		c = IMAGE_MULTI_NOAL;					// replace with real gfx
		break;
	case ALLIANCE_FORMED:
		a = 0;
		b = IMAGE_MULTI_AL_HI;
		c = IMAGE_MULTI_AL;						// replace with real gfx
		break;
	case ALLIANCE_REQUESTED:
	case ALLIANCE_INVITATION:
		a = 0;
		b = IMAGE_MULTI_OFFAL_HI;
		c = IMAGE_MULTI_OFFAL;						// replace with real gfx
		break;
	default:
		a = 0;
		b = IMAGE_MULTI_NOAL_HI;
		c = IMAGE_MULTI_NOAL;
		break;
	}

	psWidget->UserData = PACKDWORD_TRI(a,b,c);
	intDisplayImageHilight(psWidget,  xOffset,  yOffset, pColours);
	psWidget->UserData = player;
}


static void displayChannelState(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	UDWORD a, b, c, player = psWidget->UserData;
	switch(openchannels[player])
	{
	case 1:
		a = 0;
		b = IMAGE_MULTI_CHAN;
		c = IMAGE_MULTI_CHAN;
		break;
	case 0:
	default:
		a = 0;
		b = IMAGE_MULTI_NOCHAN;
		c = IMAGE_MULTI_NOCHAN;
		break;
	}

	psWidget->UserData = PACKDWORD_TRI(a,b,c);
	intDisplayImageHilight(psWidget,  xOffset,  yOffset, pColours);
	psWidget->UserData = player;
}


// ////////////////////////////////////////////////////////////////////////////

static void addMultiPlayer(UDWORD player,UDWORD pos)
{
	UDWORD			y,id;
	W_BUTINIT		sButInit;
	W_FORMINIT		sFormInit;
	y	= MULTIMENU_PLAYER_H*(pos+1);					// this is the top of the pos.
	id	= MULTIMENU_PLAYER+player;

	// add the whole thing.
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID		  = MULTIMENU_FORM;
	sFormInit.id			  = id;
	sFormInit.style			  = WFORM_PLAIN;
	sFormInit.x				  = 2;
	sFormInit.y				  = (short)y;
	sFormInit.width			  = MULTIMENU_FORM_W -4;
	sFormInit.height		  = MULTIMENU_PLAYER_H;
	sFormInit.pDisplay		  = displayMultiPlayer;
	sFormInit.UserData		  = player;
	widgAddForm(psWScreen, &sFormInit);

	//name,
	//score,
	//kills,
	//ping
	//ALL DONE IN THE DISPLAY FUNC.

	// add channel opener.
	if(player != selectedPlayer)
	{
		memset(&sButInit, 0, sizeof(W_BUTINIT));
		sButInit.formID = id;
		sButInit.style	= WBUT_PLAIN;
		sButInit.x		= MULTIMENU_C0;
		sButInit.y		= 5;
		sButInit.width	= 35;
		sButInit.height = 24;
		sButInit.FontID = font_regular;
		sButInit.id		= MULTIMENU_CHANNEL + player;
		sButInit.pTip	= _("Channel");
		sButInit.pDisplay = displayChannelState;
		sButInit.UserData = player;
		widgAddButton(psWScreen, &sButInit);
	}

	if(game.alliance!=NO_ALLIANCES && player!=selectedPlayer)
	{
		//alliance
		sButInit.x		= MULTIMENU_C3;
		sButInit.y		= 5;
		sButInit.width	= 35;
		sButInit.height = 24;
		sButInit.FontID = font_regular;
		sButInit.id		= MULTIMENU_ALLIANCE_BASE + player;
		sButInit.pTip	= _("Toggle Alliance State");
		sButInit.pDisplay = displayAllianceState;
		sButInit.UserData = player;

		//can't break alliances in 'Locked Teams' mode
		if(game.alliance != ALLIANCES_TEAMS)
		{
			widgAddButton(psWScreen, &sButInit);
		}

		sButInit.pDisplay = intDisplayImageHilight;

			// add the gift buttons.
			sButInit.y		+= 1;	// move down a wee bit.

		if (game.alliance != ALLIANCES_TEAMS)
		{
			sButInit.id		= MULTIMENU_GIFT_RAD+ player;
			sButInit.x		= MULTIMENU_C4;
			sButInit.pTip	= _("Give Visibility Report");
			sButInit.UserData = PACKDWORD_TRI(0,IMAGE_MULTI_VIS_HI, IMAGE_MULTI_VIS);
			widgAddButton(psWScreen, &sButInit);

			sButInit.id		= MULTIMENU_GIFT_RES + player;
			sButInit.x		= MULTIMENU_C5;
			sButInit.pTip	= _("Leak Technology Documents");
			sButInit.UserData = PACKDWORD_TRI(0,IMAGE_MULTI_TEK_HI , IMAGE_MULTI_TEK);
			widgAddButton(psWScreen, &sButInit);
		}

			sButInit.id		= MULTIMENU_GIFT_DRO + player;
			sButInit.x		= MULTIMENU_C6;
			sButInit.pTip	= _("Hand Over Selected Units");
			sButInit.UserData = PACKDWORD_TRI(0,IMAGE_MULTI_DRO_HI , IMAGE_MULTI_DRO);
			widgAddButton(psWScreen, &sButInit);

			sButInit.id		= MULTIMENU_GIFT_POW + player;
			sButInit.x		= MULTIMENU_C7;
			sButInit.pTip	= _("Give Power To Player");
			sButInit.UserData = PACKDWORD_TRI(0,IMAGE_MULTI_POW_HI , IMAGE_MULTI_POW);
			widgAddButton(psWScreen, &sButInit);

			giftsUp[player] = true;				// note buttons are up!
	}
}

/* Output some text to the debug menu */
void setDebugMenuEntry(char *entry, SDWORD index)
{
	BOOL		bAddingNew = false;

	/* New one? */
	if(!strcmp(debugMenuEntry[index],""))
	{
		bAddingNew = true;
	}

	/* Set */
	strcpy(debugMenuEntry[index], entry);

	/* Re-open it if already open to recalculate height */
	if(DebugMenuUp && bAddingNew)
	{
		intCloseDebugMenuNoAnim();
		(void)addDebugMenu(true);
	}
}

void intCloseDebugMenuNoAnim(void)
{
	//widgDelete(psWScreen, DEBUGMENU_CLOSE);
	widgDelete(psWScreen, DEBUGMENU);
	DebugMenuUp = false;
	//intMode		= INT_NORMAL;
}

/* Opens/closes a 'watch' window (Default key combo: Alt+Space),
 * only available in debug mode
 */
BOOL addDebugMenu(BOOL bAdd)
{
	W_FORMINIT		sFormInit;
	UDWORD			i,pos = 0,formHeight=0;

	/* Close */
	if(!bAdd)	//|| widgGetFromID(psWScreen,DEBUGMENU)
	{
		intCloseDebugMenuNoAnim();
		return true;
	}

	intResetScreen(false);

	// calculate required height.
	formHeight = 12;		//DEBUGMENU_ENTRY_H
	for(i=0;i<DEBUGMENU_MAX_ENTRIES;i++)
	{
		if(strcmp(debugMenuEntry[i],""))
			formHeight += DEBUGMENU_ENTRY_H;
	}

	// add form
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID		  = 0;
	sFormInit.id			  = DEBUGMENU;
	sFormInit.style			  = WFORM_PLAIN;
	sFormInit.x				  =(SWORD)(DEBUGMENU_FORM_X);
	sFormInit.y				  =(SWORD)(DEBUGMENU_FORM_Y);
	sFormInit.width			  = DEBUGMENU_FORM_W;
	sFormInit.height		  = (UWORD)formHeight;			//MULTIMENU_FORM_H;
	sFormInit.pDisplay		  = intOpenPlainForm;
	sFormInit.disableChildren = true;

	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	// add debug info
	pos = 0;
	for(i=0;i<DEBUGMENU_MAX_ENTRIES;i++)
	{
		if(strcmp(debugMenuEntry[i],""))
		{
			// add form
			memset(&sFormInit, 0, sizeof(W_FORMINIT));
			sFormInit.formID		  = DEBUGMENU;
			sFormInit.id			  = DEBUGMENU_CLOSE + pos + 1;
			sFormInit.style			  = WFORM_PLAIN;
			sFormInit.x				  = 5;
			sFormInit.y				  = 5 + DEBUGMENU_ENTRY_H * pos;
			sFormInit.width			  = DEBUGMENU_FORM_W;
			sFormInit.height		  = DEBUGMENU_ENTRY_H;
			sFormInit.pDisplay		  = displayDebugMenu;
			sFormInit.UserData		  = i;
			widgAddForm(psWScreen, &sFormInit);

			pos++;
		}
	}

	// Add the close button.
	/* memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID = DEBUGMENU;
	sButInit.id = DEBUGMENU_CLOSE;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = DEBUGMENU_FORM_W - CLOSE_WIDTH;
	sButInit.y = 0;
	sButInit.width = CLOSE_WIDTH;
	sButInit.height = CLOSE_HEIGHT;
	sButInit.pTip = _("Close");
	sButInit.FontID = font_regular;
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.pUserData = (void*)PACKDWORD_TRI(0,IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	} */

	DebugMenuUp = true;

	return true;
}

BOOL intAddMultiMenu(void)
{
	W_FORMINIT		sFormInit;
	W_BUTINIT		sButInit;
	UDWORD			i;

	//check for already open.
	if(widgGetFromID(psWScreen,MULTIMENU_FORM))
	{
		intCloseMultiMenu();
		return true;
	}

	if (intMode != INT_INTELMAP)
	{
		intResetScreen(false);
	}

	// add form
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID		  = 0;
	sFormInit.id			  = MULTIMENU_FORM;
	sFormInit.style			  = WFORM_PLAIN;
	sFormInit.x				  =(SWORD)(MULTIMENU_FORM_X);
	sFormInit.y				  =(SWORD)(MULTIMENU_FORM_Y);
	sFormInit.width			  = MULTIMENU_FORM_W;
	sFormInit.height		  = (UWORD)(MULTIMENU_PLAYER_H*game.maxPlayers + MULTIMENU_PLAYER_H+7);	//MULTIMENU_FORM_H;
	sFormInit.pDisplay		  = intOpenPlainForm;
	sFormInit.disableChildren = true;

	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	// add any players
	for(i=0;i<MAX_PLAYERS;i++)
	{
		if(isHumanPlayer(i) || (game.type == SKIRMISH && i<game.maxPlayers && game.skDiff[i] ) )
		{
			addMultiPlayer(i,NetPlay.players[i].position);
		}
	}

	// Add the close button.
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	sButInit.formID = MULTIMENU_FORM;
	sButInit.id = MULTIMENU_CLOSE;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = MULTIMENU_FORM_W - CLOSE_WIDTH;
	sButInit.y = 0;
	sButInit.width = CLOSE_WIDTH;
	sButInit.height = CLOSE_HEIGHT;
	sButInit.pTip = _("Close");
	sButInit.FontID = font_regular;
	sButInit.pDisplay = intDisplayImageHilight;
	sButInit.UserData = PACKDWORD_TRI(0,IMAGE_CLOSEHILIGHT , IMAGE_CLOSE);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	intShowPowerBar();						// add power bar

	if (intMode != INT_INTELMAP)
	{
		intMode		= INT_MULTIMENU;			// change interface mode.
	}
	MultiMenuUp = true;
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
void intCloseMultiMenuNoAnim(void)
{
	if (!MultiMenuUp && !ClosingMultiMenu)
	{
		return;
	}
	widgDelete(psWScreen, MULTIMENU_CLOSE);
	widgDelete(psWScreen, MULTIMENU_FORM);
	MultiMenuUp = false;
	if (intMode != INT_INTELMAP)
	{
		intMode		= INT_NORMAL;
	}
}


// ////////////////////////////////////////////////////////////////////////////
BOOL intCloseMultiMenu(void)
{
	W_TABFORM *Form;

	if (!MultiMenuUp)
	{
		return true;
	}

	widgDelete(psWScreen, MULTIMENU_CLOSE);

	// Start the window close animation.
	Form = (W_TABFORM*)widgGetFromID(psWScreen,MULTIMENU_FORM);
	if(Form) {
		Form->display = intClosePlainForm;
		Form->pUserData = NULL;	// Used to signal when the close anim has finished.
		Form->disableChildren = true;
		ClosingMultiMenu = true;
		MultiMenuUp  = false;
	}

	if (intMode != INT_INTELMAP)
	{
		intMode		= INT_NORMAL;
	}
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// In Game Options house keeping stuff.
BOOL intRunMultiMenu(void)
{
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// process clicks made by user.
void intProcessMultiMenu(UDWORD id)
{
	UBYTE	i;

	//close
	if (id == MULTIMENU_CLOSE)
	{
		intCloseMultiMenu();
	}

	//alliance button
	if(id >=MULTIMENU_ALLIANCE_BASE  &&  id<MULTIMENU_ALLIANCE_BASE+MAX_PLAYERS)
	{
		i =(UBYTE)( id - MULTIMENU_ALLIANCE_BASE);

		switch(alliances[selectedPlayer][i])
		{
		case ALLIANCE_BROKEN:
			requestAlliance((UBYTE)selectedPlayer,i,true,true);			// request an alliance
			break;
		case ALLIANCE_INVITATION:
			formAlliance((UBYTE)selectedPlayer,i,true,true,true);			// form an alliance
			break;
		case ALLIANCE_REQUESTED:
			breakAlliance((UBYTE)selectedPlayer,i,true,true);		// break an alliance
			break;

		case ALLIANCE_FORMED:
			breakAlliance((UBYTE)selectedPlayer,i,true,true);		// break an alliance
			break;
		default:
			break;
		}
	}


	//channel opens.
	if(id >=MULTIMENU_CHANNEL &&  id<MULTIMENU_CHANNEL+MAX_PLAYERS)
	{
		i =(UBYTE)( id - MULTIMENU_CHANNEL);
		if(openchannels[i])
		{
			openchannels[i] = false;// close channel
		}
		else
		{
			openchannels[i] = true;// open channel
		}
	}

	//radar gifts
	if(id >=  MULTIMENU_GIFT_RAD && id< MULTIMENU_GIFT_RAD +MAX_PLAYERS)
	{
		sendGift(RADAR_GIFT, id - MULTIMENU_GIFT_RAD);
	}

	// research gift
	if(id >= MULTIMENU_GIFT_RES && id<MULTIMENU_GIFT_RES  +MAX_PLAYERS)
	{
		sendGift(RESEARCH_GIFT, id - MULTIMENU_GIFT_RES);
	}

	//droid gift
	if(id >=  MULTIMENU_GIFT_DRO && id<  MULTIMENU_GIFT_DRO +MAX_PLAYERS)
	{
		sendGift(DROID_GIFT, id - MULTIMENU_GIFT_DRO);
	}

	//power gift
	if(id >=  MULTIMENU_GIFT_POW && id<  MULTIMENU_GIFT_POW +MAX_PLAYERS)
	{
		sendGift(POWER_GIFT, id - MULTIMENU_GIFT_POW);
	}
}
