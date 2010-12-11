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
 * @file ingameop.c
 * Ingame options screen.
 * Pumpkin Studios. 98
 */

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/widget/widget.h"
#include "lib/netplay/netplay.h"
#include "lib/ivis_common/piestate.h"		// for getrendertype
#include "lib/sound/audio.h"					// for sound.
#include "lib/sound/cdaudio.h"
#include "lib/sound/mixer.h"

#include "display3d.h"
#include "intdisplay.h"
#include "hci.h"			// for wFont def.& intmode.
#include "loop.h"
#include "lib/framework/cursors.h"
#include "frontend.h"		// for textdisplay function
#include "loadsave.h"		// for textdisplay function
#include "console.h"		// to add console message
#include "scriptextern.h"	// for tutorial
#include "keybind.h"
#include "multiplay.h"
#include "ingameop.h"
#include "mission.h"
#include "transporter.h"
#include "main.h"
#include "warzoneconfig.h"

//status bools.(for hci.h)
BOOL	ClosingInGameOp	= false;
BOOL	InGameOpUp		= false;
bool 	isInGamePopupUp = false;
// ////////////////////////////////////////////////////////////////////////////
// functions

// ////////////////////////////////////////////////////////////////////////////

static BOOL addIGTextButton(UDWORD id, UWORD y, UWORD width, const char *string, UDWORD Style)
{
	W_BUTINIT sButInit;

	memset( &sButInit, 0, sizeof(W_BUTINIT) );

	//resume
	sButInit.formID		= INTINGAMEOP;
	sButInit.id			= id;
	sButInit.style		= Style;


	sButInit.x			= INTINGAMEOP_1_X;
	sButInit.y			= y;
	sButInit.width		= width;
	sButInit.height		= INTINGAMEOP_OP_H;

	sButInit.FontID		= font_regular;
	sButInit.pDisplay	= displayTextOption;
	sButInit.pText		= string;
	widgAddButton(psWScreen, &sButInit);

	return true;
}

static BOOL addQuitOptions(void)
{
	W_FORMINIT		sFormInit;
	W_BUTINIT	sButInit;

	if (widgGetFromID(psWScreen,INTINGAMEOP))
	{
		widgDelete(psWScreen, INTINGAMEOP);		// get rid of the old stuff.
	}

	if (widgGetFromID(psWScreen,INTINGAMEPOPUP))
	{
		widgDelete(psWScreen, INTINGAMEPOPUP);		// get rid of the old stuff.
	}

	memset(&sFormInit,0, sizeof(W_FORMINIT));
	// add form
	sFormInit.formID	= 0;
	sFormInit.id		= INTINGAMEOP;
	sFormInit.style		= WFORM_PLAIN;
	sFormInit.width		= INTINGAMEOP3_W;
	sFormInit.height	= INTINGAMEOP3_H;;
	sFormInit.x		= (SWORD)INTINGAMEOP3_X;
	sFormInit.y		= (SWORD)INTINGAMEOP3_Y;
	sFormInit.pDisplay	= intOpenPlainForm;
	sFormInit.disableChildren= true;

	widgAddForm(psWScreen, &sFormInit);

	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_Y, INTINGAMEOP_OP_W, _("Resume Game"), OPALIGN);
	addIGTextButton(INTINGAMEOP_QUIT_CONFIRM, INTINGAMEOP_2_Y, INTINGAMEOP_OP_W, _("Quit"), OPALIGN);

	if (NetPlay.isHost && bMultiPlayer && NetPlay.bComms)		// only show for real MP games
	{
		sFormInit.id		= INTINGAMEPOPUP;
		sFormInit.width		= 600;
		sFormInit.height	= 26;
		sFormInit.x			= (SWORD)(20+D_W);	// center it
		sFormInit.y			= (SWORD) 130;

		widgAddForm(psWScreen, &sFormInit);

		memset( &sButInit, 0, sizeof(W_BUTINIT) );

		sButInit.formID		= INTINGAMEPOPUP;
		sButInit.FontID		= font_regular;
		sButInit.style		= OPALIGN;
		sButInit.width		= 600;
		sButInit.height		= 10;
		sButInit.x			= 0;
		sButInit.y			= 8;
		sButInit.pDisplay	= displayTextOption;
		sButInit.id			= INTINGAMEOP_POPUP_MSG3;
		sButInit.pText		= _("WARNING: You're the host. If you quit, the game ends for everyone!");

		widgAddButton(psWScreen, &sButInit);
	}

	return true;
}


static BOOL addSlideOptions(void)
{
	W_FORMINIT		sFormInit;

	if (widgGetFromID(psWScreen,INTINGAMEOP))
	{
		widgDelete(psWScreen, INTINGAMEOP);		// get rid of the old stuff.
	}

	memset(&sFormInit,0, sizeof(W_FORMINIT));

	// add form
	sFormInit.formID	= 0;
	sFormInit.id		= INTINGAMEOP;
	sFormInit.style		= WFORM_PLAIN;
	sFormInit.x		= (SWORD)INTINGAMEOP2_X;
	sFormInit.y		= (SWORD)INTINGAMEOP2_Y;
	sFormInit.width		= INTINGAMEOP2_W;
	sFormInit.height	= INTINGAMEOP2_H;

	sFormInit.pDisplay	= intOpenPlainForm;
	sFormInit.disableChildren= true;

	widgAddForm(psWScreen, &sFormInit);

	// fx vol
	addIGTextButton(INTINGAMEOP_FXVOL, INTINGAMEOP_1_Y, INTINGAMEOP_OP_W, _("Voice Volume"), WBUT_PLAIN);
	addFESlider(INTINGAMEOP_FXVOL_S, INTINGAMEOP, INTINGAMEOP_MID, INTINGAMEOP_1_Y-5,
				AUDIO_VOL_MAX, (int)(sound_GetUIVolume() * 100.0));

	// fx vol
	addIGTextButton(INTINGAMEOP_3DFXVOL, INTINGAMEOP_2_Y, INTINGAMEOP_OP_W, _("FX Volume"), WBUT_PLAIN);
	addFESlider(INTINGAMEOP_3DFXVOL_S, INTINGAMEOP, INTINGAMEOP_MID, INTINGAMEOP_2_Y-5,
				AUDIO_VOL_MAX, (int)(sound_GetEffectsVolume() * 100.0));

	// cd vol
	addIGTextButton(INTINGAMEOP_CDVOL, INTINGAMEOP_3_Y, INTINGAMEOP_OP_W, _("Music Volume"), WBUT_PLAIN);
	addFESlider(INTINGAMEOP_CDVOL_S, INTINGAMEOP, INTINGAMEOP_MID, INTINGAMEOP_3_Y-5,
				AUDIO_VOL_MAX, (int)(sound_GetMusicVolume() * 100));

	// Tactical UI: Target Origin
	if(tuiTargetOrigin)
	{
		addIGTextButton(INTINGAMEOP_TUI_TARGET_ORIGIN_SW, INTINGAMEOP_4_Y, INTINGAMEOP_SW_W,
			_("Tactical UI (Target Origin Icon): Show"), WBUT_PLAIN);
	}
	else
	{
		addIGTextButton(INTINGAMEOP_TUI_TARGET_ORIGIN_SW, INTINGAMEOP_4_Y, INTINGAMEOP_SW_W,
			_("Tactical UI (Target Origin Icon): Hide"), WBUT_PLAIN);
	}

	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_5_Y, INTINGAMEOP_SW_W, _("Resume Game"), OPALIGN);

	return true;
}


// ////////////////////////////////////////////////////////////////////////////

static BOOL _intAddInGameOptions(void)
{
//	UWORD WindowWidth;
	W_FORMINIT		sFormInit;


	audio_StopAll();

    //clear out any mission widgets - timers etc that may be on the screen
    clearMissionWidgets();


	setWidgetsStatus(true);

	//if already open, then close!
	if (widgGetFromID(psWScreen,INTINGAMEOP))
	{
		intCloseInGameOptions(false, true);
		return true;
	}

	intResetScreen(false);


	// Pause the game.
	if(!gamePaused())
	{
		kf_TogglePauseMode();
	}


	memset(&sFormInit,0, sizeof(W_FORMINIT));



	sFormInit.width		= INTINGAMEOP_W;

	// add form
	sFormInit.formID	= 0;
	sFormInit.id		= INTINGAMEOP;
	sFormInit.style		= WFORM_PLAIN;
	sFormInit.x			= (SWORD)INTINGAMEOP_X;
	sFormInit.y			= (SWORD)INTINGAMEOP_Y;
	sFormInit.height	= INTINGAMEOP_H;


    if ( (!bMultiPlayer || (NetPlay.bComms==0) )  && !bInTutorial)
	{
	}
	else
	{
		sFormInit.height	= INTINGAMEOP_HS;
	}



	sFormInit.pDisplay	= intOpenPlainForm;
	sFormInit.disableChildren= true;

	widgAddForm(psWScreen, &sFormInit);

	// add 'quit' text

    if ( (!bMultiPlayer || (NetPlay.bComms==0) )  && !bInTutorial)
	{
		addIGTextButton(INTINGAMEOP_QUIT, INTINGAMEOP_5_Y, INTINGAMEOP_OP_W, _("Quit"), OPALIGN);

	}
	else
	{
		addIGTextButton(INTINGAMEOP_QUIT, INTINGAMEOP_3_Y, INTINGAMEOP_OP_W, _("Quit"), OPALIGN);
	}

	// add 'resume'
	addIGTextButton(INTINGAMEOP_RESUME, INTINGAMEOP_1_Y, INTINGAMEOP_OP_W, _("Resume Game"), OPALIGN);

	// add 'options'
	addIGTextButton(INTINGAMEOP_OPTIONS, INTINGAMEOP_2_Y, INTINGAMEOP_OP_W, _("Audio Options"), OPALIGN);


	if ( (!bMultiPlayer || (NetPlay.bComms==0) )  && !bInTutorial)
	{		// add 'load'
		addIGTextButton(INTINGAMEOP_LOAD, INTINGAMEOP_3_Y, INTINGAMEOP_OP_W, _("Load Game"), OPALIGN);
		// add 'save'
		addIGTextButton(INTINGAMEOP_SAVE, INTINGAMEOP_4_Y, INTINGAMEOP_OP_W, _("Save Game"), OPALIGN);
	}


	intMode		= INT_INGAMEOP;			// change interface mode.
	InGameOpUp	= true;					// inform interface.

	// Using software cursors (when on) for these menus due to a bug in SDL's SDL_ShowCursor()
	pie_SetMouse(CURSOR_DEFAULT, war_GetColouredCursor());

	return true;
}


BOOL intAddInGameOptions(void)
{
	sliderEnableDrag(true);
	return _intAddInGameOptions();
}

// 
// Quick hack to throw up a ingame 'popup' for when the host drops connection.
//
void intAddInGamePopup(void)
{
	W_FORMINIT	sFormInit;
	W_BUTINIT	sButInit;

	//clear out any mission widgets - timers etc that may be on the screen
	clearMissionWidgets();
	setWidgetsStatus(true);
	intResetScreen(false);

	if (isInGamePopupUp) return;

	audio_StopAll();

	if(!gamePaused())
	{
		kf_TogglePauseMode();	// Pause the game.
	}

	memset(&sFormInit,0, sizeof(W_FORMINIT));

	sFormInit.formID	= 0;
	sFormInit.id		= INTINGAMEPOPUP;
	sFormInit.style		= WFORM_PLAIN;
	sFormInit.width		= 600;
	sFormInit.height	= 160;
	sFormInit.x			= (SWORD)(20+D_W);
	sFormInit.y			= (SWORD)((240-(160/2))+D_H);
	sFormInit.pDisplay	= intOpenPlainForm;
	sFormInit.disableChildren= true;

	widgAddForm(psWScreen, &sFormInit);

	// add the text "buttons" now
	memset( &sButInit, 0, sizeof(W_BUTINIT) );

	sButInit.formID		= INTINGAMEPOPUP;
	sButInit.style		= OPALIGN;
	sButInit.width		= 600;
	sButInit.FontID		= font_large;
	sButInit.x			= 0;
	sButInit.height		= 10;
	sButInit.pDisplay	= displayTextOption;

	sButInit.id			= INTINGAMEOP_POPUP_MSG2;
	sButInit.y			= 20;
	sButInit.pText		= _("Host has quit the game!");

	widgAddButton(psWScreen, &sButInit);

	sButInit.id			= INTINGAMEOP_POPUP_MSG1;
	sButInit.y			= 60;
	sButInit.pText		= _("The game can't continue without the host.");

	widgAddButton(psWScreen, &sButInit);

	sButInit.id			= INTINGAMEOP_POPUP_QUIT;
	sButInit.y			= 124;
	sButInit.pText		= _("-->  QUIT  <--");

	widgAddButton(psWScreen, &sButInit);

	intMode		= INT_POPUPMSG;			// change interface mode.
	isInGamePopupUp = true;
}

// ////////////////////////////////////////////////////////////////////////////

static void ProcessOptionFinished(void)
{
	intMode		= INT_NORMAL;



	//unpause.
	if(gamePaused())
	{
		kf_TogglePauseMode();
	}


}

void intCloseInGameOptionsNoAnim(BOOL bResetMissionWidgets)
{
	if (NetPlay.isHost)
	{
		widgDelete(psWScreen, INTINGAMEPOPUP);
	}
	widgDelete(psWScreen, INTINGAMEOP);
	InGameOpUp = false;

	ProcessOptionFinished();

    //don't add the widgets if the load/save screen is put up or exiting to front end
    if (bResetMissionWidgets)
    {
        //put any widgets back on for the missions
        resetMissionWidgets();
    }
}


// ////////////////////////////////////////////////////////////////////////////
BOOL intCloseInGameOptions(BOOL bPutUpLoadSave, BOOL bResetMissionWidgets)
{

	W_TABFORM	*Form;
	WIDGET		*widg;

	if (NetPlay.isHost)
	{
		widgDelete(psWScreen, INTINGAMEPOPUP);
	}

	if(bPutUpLoadSave)
	{
		widg = widgGetFromID(psWScreen,INTINGAMEOP);
		if(widg)
		{
			widgDelete(psWScreen,INTINGAMEOP);
		}

		InGameOpUp = false;
		ClosingInGameOp = true;
	}
	else
	{
		// close the form.
		// Start the window close animation.
		if (isInGamePopupUp)	// FIXME: we hijack this routine for the popup close.
		{
			Form = (W_TABFORM*)widgGetFromID(psWScreen,INTINGAMEPOPUP);
			isInGamePopupUp = false;
		}
		else
		{
		Form = (W_TABFORM*)widgGetFromID(psWScreen,INTINGAMEOP);
		}

		if(Form)
		{
			Form->display		 = intClosePlainForm;
			Form->pUserData		 = NULL; // Used to signal when the close anim has finished.
			Form->disableChildren= true;
			ClosingInGameOp		 = true;		// like orderup/closingorder
			InGameOpUp			 = false;
		}
	}

	ProcessOptionFinished();

    //don't add the widgets if the load/save screen is put up or exiting to front end
    if (bResetMissionWidgets)
    {
        //put any widgets back on for the missions
        resetMissionWidgets();
    }

    return true;
}


// ////////////////////////////////////////////////////////////////////////////
// process clicks made by user.
void intProcessInGameOptions(UDWORD id)
{


	switch(id)
	{
	// NORMAL KEYS
	case INTINGAMEOP_QUIT:				//quit was pressed
		addQuitOptions();
		break;

	case INTINGAMEOP_POPUP_QUIT:
	case INTINGAMEOP_QUIT_CONFIRM:		//quit was confirmed.
		intCloseInGameOptions(false, false);
		break;

	case INTINGAMEOP_OPTIONS:			//game options  was pressed
		addSlideOptions();
		break;

	case INTINGAMEOP_RESUME:			//resume was pressed.
		intCloseInGameOptions(false, true);
		break;


//	case INTINGAMEOP_REPLAY:
//		intCloseInGameOptions(true, false);
//		if(0!=strcmp(getLevelName(),"CAM_1A"))
//		{
//			loopMissionState = LMS_LOADGAME;
//			strcpy(saveGameName, "replay/replay.gam");
//			addConsoleMessage(_("GAME SAVED!"), LEFT_JUSTIFY, SYSTEM_MESSAGE);
//		}
//		break;
	case INTINGAMEOP_LOAD:
		intCloseInGameOptions(true, false);
		addLoadSave(LOAD_INGAME,SaveGamePath,"gam",_("Load Saved Game"));	// change mode when loadsave returns//		if(runLoadSave())// check for file name.
		break;
	case INTINGAMEOP_SAVE:
		intCloseInGameOptions(true, false);
		addLoadSave(SAVE_INGAME,SaveGamePath,"gam", _("Save Game") );
		break;


	// GAME OPTIONS KEYS
	case INTINGAMEOP_FXVOL:
	case INTINGAMEOP_3DFXVOL:
	case INTINGAMEOP_CDVOL:
		break;


	case INTINGAMEOP_FXVOL_S:
		sound_SetUIVolume((float)widgGetSliderPos(psWScreen, INTINGAMEOP_FXVOL_S) / 100.0);
		break;
	case INTINGAMEOP_3DFXVOL_S:
		sound_SetEffectsVolume((float)widgGetSliderPos(psWScreen, INTINGAMEOP_3DFXVOL_S) / 100.0);
		break;
	case INTINGAMEOP_CDVOL_S:
		sound_SetMusicVolume((float)widgGetSliderPos(psWScreen, INTINGAMEOP_CDVOL_S) / 100.0);
		break;

	case INTINGAMEOP_TUI_TARGET_ORIGIN_SW:
		tuiTargetOrigin = !tuiTargetOrigin;
		if(tuiTargetOrigin)
		{
			widgSetString(psWScreen, INTINGAMEOP_TUI_TARGET_ORIGIN_SW, _("Tactical UI (Target Origin Icon): Show"));
		}
		else
		{
			widgSetString(psWScreen, INTINGAMEOP_TUI_TARGET_ORIGIN_SW, _("Tactical UI (Target Origin Icon): Hide"));
		}
		
		break;
		
	default:
		break;
	}


}


