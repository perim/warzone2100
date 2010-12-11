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
 *  Functions for the bar graph widget
 */

#include "widget.h"
#include "widgint.h"
#include "tip.h"
#include "form.h"
#include "bar.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_common/pieblitfunc.h"
#include "lib/ivis_common/piepalette.h"

/* Create a barGraph widget data structure */
W_BARGRAPH* barGraphCreate(const W_BARINIT* psInit)
{
	W_BARGRAPH* psWidget;

	if (psInit->style & ~(WBAR_PLAIN | WBAR_TROUGH | WBAR_DOUBLE | WIDG_HIDDEN))
	{
		ASSERT(false, "Unknown bar graph style");
		return NULL;
	}

	if (psInit->orientation < WBAR_LEFT
	 || psInit->orientation > WBAR_BOTTOM)
	{
		ASSERT(false, "barGraphCreate: Unknown orientation");
		return NULL;
	}

	if (psInit->size > WBAR_SCALE)
	{
		ASSERT(false, "barGraphCreate: Bar size out of range");
		return NULL;
	}
	if ((psInit->style & WBAR_DOUBLE)
	 && (psInit->minorSize > WBAR_SCALE))
	{
		ASSERT(false, "barGraphCreate: Minor bar size out of range");
		return NULL;
	}

	/* Allocate the required memory */
	psWidget = (W_BARGRAPH *)malloc(sizeof(W_BARGRAPH));
	if (psWidget == NULL)
	{
		debug(LOG_FATAL, "barGraphCreate: Out of memory");
		abort();
		return NULL;
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
	psWidget->type = WIDG_BARGRAPH;
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
	psWidget->barPos = psInit->orientation;
	psWidget->majorSize = psInit->size;
	psWidget->minorSize = psInit->minorSize;
	psWidget->iRange = psInit->iRange;

	/* Set the display function */
	if (psInit->pDisplay)
	{
		psWidget->display = psInit->pDisplay;
	}
	else if (psInit->style & WBAR_TROUGH)
	{
		psWidget->display = barGraphDisplayTrough;
	}
	else if (psInit->style & WBAR_DOUBLE)
	{
		psWidget->display = barGraphDisplayDouble;
	}
	else
	{
		psWidget->display = barGraphDisplay;
	}
	/* Set the major colour */
	psWidget->majorCol = psInit->sCol;

	/* Set the minor colour if necessary */
	if (psInit->style & WBAR_DOUBLE)
	{
		psWidget->majorCol = psInit->sMinorCol;
	}

	barGraphInitialise(psWidget);

	return psWidget;
}


/* Free the memory used by a barGraph */
void barGraphFree(W_BARGRAPH *psWidget)
{
	ASSERT(psWidget != NULL,"Invalid widget pointer");

	free(psWidget);
}

/* Initialise a barGraph widget before running it */
void barGraphInitialise(W_BARGRAPH *psWidget)
{
	(void)psWidget;
}


/* Set the current size of a bar graph */
void widgSetBarSize(W_SCREEN *psScreen, UDWORD id, UDWORD iValue)
{
	W_BARGRAPH		*psBGraph;
	UDWORD			size;

	ASSERT(psScreen != NULL, "Invalid screen pointer");

	psBGraph = (W_BARGRAPH *)widgGetFromID(psScreen, id);
	if (psBGraph == NULL || psBGraph->type != WIDG_BARGRAPH)
	{
		ASSERT( false, "widgSetBarSize: Couldn't find widget from id" );
		return;
	}

	psBGraph->iOriginal = iValue;
	if ( iValue < psBGraph->iRange )
	{
		psBGraph->iValue = (UWORD) iValue;
	}
	else
	{
		psBGraph->iValue = psBGraph->iRange;
	}

	size = WBAR_SCALE * psBGraph->iValue / MAX(psBGraph->iRange, 1);

	psBGraph->majorSize = (UWORD)size;
}


/* Set the current size of a minor bar on a double graph */
void widgSetMinorBarSize(W_SCREEN *psScreen, UDWORD id, UDWORD iValue )
{
	W_BARGRAPH		*psBGraph;
	UDWORD			size;

	ASSERT(psScreen != NULL, "Invalid screen pointer");

	psBGraph = (W_BARGRAPH *)widgGetFromID(psScreen, id);
	if (psBGraph == NULL || psBGraph->type != WIDG_BARGRAPH)
	{
		ASSERT(false, "Couldn't find widget from id");
		return;
	}

	size = WBAR_SCALE * iValue / MAX(psBGraph->iRange, 1);
	if (size > WBAR_SCALE)
	{
		size = WBAR_SCALE;
	}

	psBGraph->minorSize = (UWORD)size;
}


/* Respond to a mouse moving over a barGraph */
void barGraphHiLite(W_BARGRAPH *psWidget, W_CONTEXT *psContext)
{
	if (psWidget->pTip)
	{
		tipStart((WIDGET *)psWidget, psWidget->pTip, psContext->psScreen->TipFontID,
				 psContext->psForm->aColours,
				 psWidget->x + psContext->xOffset, psWidget->y + psContext->yOffset,
				 psWidget->width, psWidget->height);
	}
}


/* Respond to the mouse moving off a barGraph */
void barGraphHiLiteLost(W_BARGRAPH *psWidget)
{
	tipStop((WIDGET *)psWidget);
}


/* The simple bar graph display function */
void barGraphDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	SDWORD		x0 = 0, y0 = 0, x1 = 0, y1 = 0;
	W_BARGRAPH	*psBGraph;

	psBGraph = (W_BARGRAPH *)psWidget;

	/* figure out which way the bar graph fills */
	switch (psBGraph->barPos)
	{
	case WBAR_LEFT:
		x0 = xOffset + psWidget->x;
		y0 = yOffset + psWidget->y;
		x1 = x0 + psWidget->width * psBGraph->majorSize / WBAR_SCALE;
		y1 = y0 + psWidget->height;
		break;
	case WBAR_RIGHT:
		y0 = yOffset + psWidget->y;
		x1 = xOffset + psWidget->x + psWidget->width;
		x0 = x1 - psWidget->width * psBGraph->majorSize / WBAR_SCALE;
		y1 = y0 + psWidget->height;
		break;
	case WBAR_TOP:
		x0 = xOffset + psWidget->x;
		y0 = yOffset + psWidget->y;
		x1 = x0 + psWidget->width;
		y1 = y0 + psWidget->height * psBGraph->majorSize / WBAR_SCALE;
		break;
	case WBAR_BOTTOM:
		x0 = xOffset + psWidget->x;
		x1 = x0 + psWidget->width;
		y1 = yOffset + psWidget->y + psWidget->height;
		y0 = y1 - psWidget->height * psBGraph->majorSize / WBAR_SCALE;
		break;
	}

	/* Now draw the graph */
	pie_BoxFill(x0,y0, x1,y1,psBGraph->majorCol);
	iV_Line(x0,y1, x0,y0, pColours[WCOL_LIGHT]);
	iV_Line(x0,y0, x1,y0, pColours[WCOL_LIGHT]);
	iV_Line(x1,y0, x1,y1, pColours[WCOL_DARK]);
	iV_Line(x0,y1, x1,y1, pColours[WCOL_DARK]);
}


/* The double bar graph display function */
void barGraphDisplayDouble(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	SDWORD		x0 = 0, y0 = 0, x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0;
	W_BARGRAPH	*psBGraph = (W_BARGRAPH *)psWidget;

	/* figure out which way the bar graph fills */
	switch (psBGraph->barPos)
	{
	case WBAR_LEFT:
		/* Calculate the major bar */
		x0 = xOffset + psWidget->x;
		y0 = yOffset + psWidget->y;
		x1 = x0 + psWidget->width * psBGraph->majorSize / WBAR_SCALE;
		y1 = y0 + 2*psWidget->height/3;

		/* Calculate the minor bar */
		x2 = x0;
		y2 = y0 + psWidget->height/3;
		x3 = x2 + psWidget->width * psBGraph->minorSize / WBAR_SCALE;
		y3 = y0 + psWidget->height;
		break;
	case WBAR_RIGHT:
		/* Calculate the major bar */
		y0 = yOffset + psWidget->y;
		x1 = xOffset + psWidget->x + psWidget->width;
		x0 = x1 - psWidget->width * psBGraph->majorSize / WBAR_SCALE;
		y1 = y0 + 2*psWidget->height/3;

		/* Calculate the minor bar */
		x3 = x1;
		y2 = y0 + psWidget->height/3;
		x2 = x3 - psWidget->width * psBGraph->minorSize / WBAR_SCALE;
		y3 = y0 + psWidget->height;
		break;
	case WBAR_TOP:
		/* Calculate the major bar */
		x0 = xOffset + psWidget->x;
		y0 = yOffset + psWidget->y;
		x1 = x0 + 2*psWidget->width/3;
		y1 = y0 + psWidget->height * psBGraph->majorSize / WBAR_SCALE;

		/* Calculate the minor bar */
		x2 = x0 + psWidget->width/3;
		y2 = y0;
		x3 = x0 + psWidget->width;
		y3 = y2 + psWidget->height * psBGraph->minorSize / WBAR_SCALE;
		break;
	case WBAR_BOTTOM:
		/* Calculate the major bar */
		x0 = xOffset + psWidget->x;
		x1 = x0 + 2*psWidget->width/3;
		y1 = yOffset + psWidget->y + psWidget->height;
		y0 = y1 - psWidget->height * psBGraph->majorSize / WBAR_SCALE;

		/* Calculate the minor bar */
		x2 = x0 + psWidget->width/3;
		x3 = x0 + psWidget->width;
		y3 = y1;
		y2 = y3 - psWidget->height * psBGraph->minorSize / WBAR_SCALE;
		break;
	}

	/* Draw the minor bar graph */
	if (psBGraph->minorSize > 0)
	{
		pie_BoxFill(x2,y2, x3,y3,psBGraph->minorCol);
		iV_Line(x2,y3, x2,y2, pColours[WCOL_LIGHT]);
		iV_Line(x2,y2, x3,y2, pColours[WCOL_LIGHT]);
		iV_Line(x3,y2, x3,y3, pColours[WCOL_DARK]);
		iV_Line(x2,y3, x3,y3, pColours[WCOL_DARK]);
	}

	/* Draw the major bar graph */
	pie_BoxFill(x0,y0, x1,y1,psBGraph->majorCol);
	iV_Line(x0,y1, x0,y0, pColours[WCOL_LIGHT]);
	iV_Line(x0,y0, x1,y0, pColours[WCOL_LIGHT]);
	iV_Line(x1,y0, x1,y1, pColours[WCOL_DARK]);
	iV_Line(x0,y1, x1,y1, pColours[WCOL_DARK]);
}


/* The trough bar graph display function */
void barGraphDisplayTrough(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	SDWORD		x0 = 0, y0 = 0, x1 = 0, y1 = 0;		// Position of the bar
	SDWORD		tx0 = 0, ty0 = 0, tx1 = 0, ty1 = 0;	// Position of the trough
	BOOL		showBar=true, showTrough=true;
	W_BARGRAPH	*psBGraph = (W_BARGRAPH *)psWidget;

	/* figure out which way the bar graph fills */
	switch (psBGraph->barPos)
	{
	case WBAR_LEFT:
		x0 = xOffset + psWidget->x;
		y0 = yOffset + psWidget->y;
		x1 = x0 + psWidget->width * psBGraph->majorSize / WBAR_SCALE;
		y1 = y0 + psWidget->height;
		if (x0 == x1)
		{
			showBar = false;
		}
		tx0 = x1+1;
		ty0 = y0;
		tx1 = x0 + psWidget->width;
		ty1 = y1;
		if (tx0 >= tx1)
		{
			showTrough = false;
		}
		break;
	case WBAR_RIGHT:
		y0 = yOffset + psWidget->y;
		x1 = xOffset + psWidget->x + psWidget->width;
		x0 = x1 - psWidget->width * psBGraph->majorSize / WBAR_SCALE;
		y1 = y0 + psWidget->height;
		if (x0 == x1)
		{
			showBar = false;
		}
		tx0 = xOffset + psWidget->x;
		ty0 = y0;
		tx1 = x0-1;
		ty1 = y1;
		if (tx0 >= tx1)
		{
			showTrough = false;
		}
		break;
	case WBAR_TOP:
		x0 = xOffset + psWidget->x;
		y0 = yOffset + psWidget->y;
		x1 = x0 + psWidget->width;
		y1 = y0 + psWidget->height * psBGraph->majorSize / WBAR_SCALE;
		if (y0 == y1)
		{
			showBar = false;
		}
		tx0 = x0;
		ty0 = y1+1;
		tx1 = x1;
		ty1 = y0 + psWidget->height;
		if (ty0 >= ty1)
		{
			showTrough = false;
		}
		break;
	case WBAR_BOTTOM:
		x0 = xOffset + psWidget->x;
		x1 = x0 + psWidget->width;
		y1 = yOffset + psWidget->y + psWidget->height;
		y0 = y1 - psWidget->height * psBGraph->majorSize / WBAR_SCALE;
		if (y0 == y1)
		{
			showBar = false;
		}
		tx0 = x0;
		ty0 = yOffset + psWidget->y;
		tx1 = x1;
		ty1 = y0-1;
		if (ty0 >= ty1)
		{
			showTrough = false;
		}
		break;
	}

	/* Now draw the graph */
	if (showBar)
	{
		pie_BoxFill(x0, y0, x1, y1, psBGraph->majorCol);
	}
	if (showTrough)
	{
		pie_BoxFill(tx0, ty0, tx1, ty1, pColours[WCOL_BKGRND]);
		iV_Line(tx0,ty1, tx0,ty0, pColours[WCOL_DARK]);
		iV_Line(tx0,ty0, tx1,ty0, pColours[WCOL_DARK]);
		iV_Line(tx1,ty0, tx1,ty1, pColours[WCOL_LIGHT]);
		iV_Line(tx0,ty1, tx1,ty1, pColours[WCOL_LIGHT]);
	}
}
