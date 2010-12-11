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
 *  Functions for the button widget
 */

#include "lib/framework/frame.h"
#include "lib/framework/frameint.h"
#include "widget.h"
#include "widgint.h"
#include "button.h"
#include "form.h"
#include "tip.h"
#include "lib/ivis_common/pieblitfunc.h"
#include "lib/gamelib/gtime.h"


/* Initialise the button module */
BOOL buttonStartUp(void)
{
	return true;
}


/* Create a button widget data structure */
W_BUTTON* buttonCreate(const W_BUTINIT* psInit)
{
	W_BUTTON* psWidget;

	if (psInit->style & ~(WBUT_PLAIN | WIDG_HIDDEN | WFORM_NOCLICKMOVE
	                    | WBUT_NOPRIMARY | WBUT_SECONDARY | WBUT_TXTCENTRE))
	{
		ASSERT(!"unknown button style", "buttonCreate: unknown button style");
		return NULL;
	}

	/* Allocate the required memory */
	psWidget = (W_BUTTON *)malloc(sizeof(W_BUTTON));
	if (psWidget == NULL)
	{
		debug(LOG_FATAL, "buttonCreate: Out of memory" );
		abort();
		return NULL;
	}
	/* Allocate memory for the text and copy it if necessary */
	if (psInit->pText)
	{
		psWidget->pText = psInit->pText;
	}
	else
	{
		psWidget->pText = NULL;
	}
	/* Allocate the memory for the tip and copy it if necessary */
	if (psInit->pTip)
	{
		psWidget->pTip = psInit->pTip;
	}
	else
	{
		psWidget->pTip = NULL;
	}

	/* Initialise the structure */
	psWidget->type = WIDG_BUTTON;
	psWidget->id = psInit->id;
	psWidget->formID = psInit->formID;
	psWidget->style = psInit->style;
	psWidget->x = psInit->x;
	psWidget->y = psInit->y;
	psWidget->width = psInit->width;
	psWidget->height = psInit->height;
	psWidget->callback = psInit->pCallback;
	psWidget->pUserData = psInit->pUserData;
	psWidget->UserData = psInit->UserData;
	psWidget->AudioCallback = WidgGetAudioCallback();
	psWidget->HilightAudioID = WidgGetHilightAudioID();
	psWidget->ClickedAudioID = WidgGetClickedAudioID();

	if (psInit->pDisplay)
	{
		psWidget->display = psInit->pDisplay;
	}
	else
	{
		psWidget->display = buttonDisplay;
	}
	psWidget->FontID = psInit->FontID;

	buttonInitialise(psWidget);

	return psWidget;
}


/* Free the memory used by a button */
void buttonFree(W_BUTTON *psWidget)
{
	ASSERT( psWidget != NULL,
		"buttonFree: invalid button pointer" );

	free(psWidget);
}


/* Initialise a button widget before it is run */
void buttonInitialise(W_BUTTON *psWidget)
{
	ASSERT( psWidget != NULL,
		"buttonDisplay: Invalid widget pointer" );

	psWidget->state = WBUTS_NORMAL;
}


/* Get a button's state */
UDWORD buttonGetState(W_BUTTON *psButton)
{
	UDWORD State = 0;

	if (psButton->state & WBUTS_GREY)
	{
		State |= WBUT_DISABLE;
	}

	if (psButton->state & WBUTS_LOCKED)
	{
		State |= WBUT_LOCK;
	}

	if (psButton->state & WBUTS_CLICKLOCK)
	{
		State |= WBUT_CLICKLOCK;
	}

	if (psButton->state & WBUTS_FLASH)
	{
		State |= WBUT_FLASH;
	}

	return State;
}


void buttonSetFlash(W_BUTTON *psButton)
{
	psButton->state |= WBUTS_FLASH;
}


void buttonClearFlash(W_BUTTON *psButton)
{
	psButton->state &= ~WBUTS_FLASH;
	psButton->state &= ~WBUTS_FLASHON;
}


/* Set a button's state */
void buttonSetState(W_BUTTON *psButton, UDWORD state)
{
	ASSERT(!((state & WBUT_LOCK) && (state & WBUT_CLICKLOCK)), "Cannot have both WBUT_LOCK and WBUT_CLICKLOCK");

	if (state & WBUT_DISABLE)
	{
		psButton->state |= WBUTS_GREY;
	}
	else
	{
		psButton->state &= ~WBUTS_GREY;
	}
	if (state & WBUT_LOCK)
	{
		psButton->state |= WBUTS_LOCKED;
	}
	else
	{
		psButton->state &= ~WBUTS_LOCKED;
	}
	if (state & WBUT_CLICKLOCK)
	{
		psButton->state |= WBUTS_CLICKLOCK;
	}
	else
	{
		psButton->state &= ~WBUTS_CLICKLOCK;
	}
}


/* Run a button widget */
void buttonRun(W_BUTTON *psButton)
{
	if (psButton->state & WBUTS_FLASH)
	{
		if (((gameTime2/250) % 2) == 0) {
			psButton->state &= ~WBUTS_FLASHON;
		} else {
			psButton->state |= WBUTS_FLASHON;
		}
	}
}


/* Respond to a mouse click */
void buttonClicked(W_BUTTON *psWidget, UDWORD key)
{
	/* Can't click a button if it is disabled or locked down */
	if (!(psWidget->state & (WBUTS_GREY | WBUTS_LOCKED)))
	{
		// Check this is the correct key
		if ((!(psWidget->style & WBUT_NOPRIMARY) && key == WKEY_PRIMARY) ||
			((psWidget->style & WBUT_SECONDARY) && key == WKEY_SECONDARY))
		{
			if(psWidget->AudioCallback) {
				psWidget->AudioCallback(psWidget->ClickedAudioID);
			}
			psWidget->state &= ~WBUTS_FLASH;	// Stop it flashing
			psWidget->state &= ~WBUTS_FLASHON;
			psWidget->state |= WBUTS_DOWN;
		}
	}

	/* Kill the tip if there is one */
	if (psWidget->pTip)
	{
		tipStop((WIDGET *)psWidget);
	}
}

/* Respond to a mouse button up */
void buttonReleased(W_SCREEN* psScreen, W_BUTTON* psWidget, UDWORD key)
{
	if (psWidget->state & WBUTS_DOWN)
	{
		// Check this is the correct key
		if ((!(psWidget->style & WBUT_NOPRIMARY) && key == WKEY_PRIMARY) ||
			((psWidget->style & WBUT_SECONDARY) && key == WKEY_SECONDARY))
		{
			widgSetReturn(psScreen, (WIDGET *)psWidget);
			psWidget->state &= ~WBUTS_DOWN;
		}
	}
}


/* Respond to a mouse moving over a button */
void buttonHiLite(W_BUTTON *psWidget, W_CONTEXT *psContext)
{
	psWidget->state |= WBUTS_HILITE;

	if(psWidget->AudioCallback) {
		psWidget->AudioCallback(psWidget->HilightAudioID);
	}

	/* If there is a tip string start the tool tip */
	if (psWidget->pTip)
	{
		tipStart((WIDGET *)psWidget, psWidget->pTip, psContext->psScreen->TipFontID,
				 psContext->psForm->aColours,
				 psWidget->x + psContext->xOffset, psWidget->y + psContext->yOffset,
				 psWidget->width, psWidget->height);
	}
}


/* Respond to the mouse moving off a button */
void buttonHiLiteLost(W_BUTTON *psWidget)
{
	psWidget->state &= ~(WBUTS_DOWN | WBUTS_HILITE);
	if (psWidget->pTip)
	{
		tipStop((WIDGET *)psWidget);
	}
}


/* Display a button */
void buttonDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	W_BUTTON	*psButton;
	SDWORD		x0,y0,x1,y1, fx,fy,fw;
	int			CurrFontID;

	ASSERT(psWidget != NULL && pColours != NULL, "Invalid pointers");
	if (!psWidget || !pColours)
	{
		return;
	}

	psButton = (W_BUTTON *)psWidget;
	CurrFontID = psButton->FontID;

	x0=psButton->x + xOffset;
	y0=psButton->y + yOffset;
	x1=x0 + psButton->width;
	y1=y0 + psButton->height;

	if (psButton->state & (WBUTS_DOWN | WBUTS_LOCKED | WBUTS_CLICKLOCK))
	{
		/* Display the button down */
		pie_BoxFill(x0, y0, x1, y1, pColours[WCOL_BKGRND]);
		iV_Line(x0,y0, x1,y0, pColours[WCOL_DARK]);
		iV_Line(x0,y0, x0,y1, pColours[WCOL_DARK]);
		iV_Line(x0,y1, x1,y1, pColours[WCOL_LIGHT]);
		iV_Line(x1,y1, x1,y0, pColours[WCOL_LIGHT]);

		if (psButton->pText)
		{
			iV_SetFont(psButton->FontID);
			iV_SetTextColour(pColours[WCOL_TEXT]);
			fw = iV_GetTextWidth(psButton->pText);
			if(psButton->style & WBUT_NOCLICKMOVE) {
				fx = x0 + (psButton->width - fw) / 2 + 1;
				fy = y0 + 1 + (psButton->height - iV_GetTextLineSize())/2 - iV_GetTextAboveBase();
			} else {
				fx = x0 + (psButton->width - fw) / 2;
				fy = y0 + (psButton->height - iV_GetTextLineSize())/2 - iV_GetTextAboveBase();
			}
			iV_DrawText(psButton->pText,fx,fy);
		}

		if (psButton->state & WBUTS_HILITE)
		{
			/* Display the button hilite */
			iV_Line(x0+3,y0+3, x1-2,y0+3, pColours[WCOL_HILITE]);
			iV_Line(x0+3,y0+3, x0+3,y1-2, pColours[WCOL_HILITE]);
			iV_Line(x0+3,y1-2, x1-2,y1-2, pColours[WCOL_HILITE]);
			iV_Line(x1-2,y1-2, x1-2,y0+3, pColours[WCOL_HILITE]);
		}
	}
	else if (psButton->state & WBUTS_GREY)
	{
		/* Display the disabled button */
		pie_BoxFill(x0, y0, x1, y1, pColours[WCOL_BKGRND]);
		iV_Line(x0,y0, x1,y0, pColours[WCOL_LIGHT]);
		iV_Line(x0,y0, x0,y1, pColours[WCOL_LIGHT]);
		iV_Line(x0,y1, x1,y1, pColours[WCOL_DARK]);
		iV_Line(x1,y1, x1,y0, pColours[WCOL_DARK]);

		if (psButton->pText)
		{
			iV_SetFont(psButton->FontID);
			fw = iV_GetTextWidth(psButton->pText);
			fx = x0 + (psButton->width - fw) / 2;
			fy = y0 + (psButton->height - iV_GetTextLineSize())/2 - iV_GetTextAboveBase();
			iV_SetTextColour(pColours[WCOL_LIGHT]);
			iV_DrawText(psButton->pText, fx+1, fy+1);
			iV_SetTextColour(pColours[WCOL_DISABLE]);
			iV_DrawText(psButton->pText, fx, fy);
		}

		if (psButton->state & WBUTS_HILITE)
		{
			/* Display the button hilite */
			iV_Line(x0+2,y0+2, x1-3,y0+2, pColours[WCOL_HILITE]);
			iV_Line(x0+2,y0+2, x0+2,y1-3, pColours[WCOL_HILITE]);
			iV_Line(x0+2,y1-3, x1-3,y1-3, pColours[WCOL_HILITE]);
			iV_Line(x1-3,y1-3, x1-3,y0+2, pColours[WCOL_HILITE]);
		}
	}
	else
	{
		/* Display the button up */
		pie_BoxFill(x0, y0, x1, y1, pColours[WCOL_BKGRND]);
		iV_Line(x0,y0, x1,y0, pColours[WCOL_LIGHT]);
		iV_Line(x0,y0, x0,y1, pColours[WCOL_LIGHT]);
		iV_Line(x0,y1, x1,y1, pColours[WCOL_DARK]);
		iV_Line(x1,y1, x1,y0, pColours[WCOL_DARK]);

		if (psButton->pText)
		{
			iV_SetFont(psButton->FontID);
			iV_SetTextColour(pColours[WCOL_TEXT]);
			fw = iV_GetTextWidth(psButton->pText);
			fx = x0 + (psButton->width - fw) / 2;
			fy = y0 + (psButton->height - iV_GetTextLineSize())/2 - iV_GetTextAboveBase();
			iV_DrawText(psButton->pText, fx, fy);
		}

		if (psButton->state & WBUTS_HILITE)
		{
			/* Display the button hilite */
			iV_Line(x0+2,y0+2, x1-3,y0+2, pColours[WCOL_HILITE]);
			iV_Line(x0+2,y0+2, x0+2,y1-3, pColours[WCOL_HILITE]);
			iV_Line(x0+2,y1-3, x1-3,y1-3, pColours[WCOL_HILITE]);
			iV_Line(x1-3,y1-3, x1-3,y0+2, pColours[WCOL_HILITE]);
		}
	}
}
