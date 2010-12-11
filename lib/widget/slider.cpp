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
 *  Slide bar widget definitions.
 */

#include "widget.h"
#include "widgint.h"
#include "slider.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_common/pieblitfunc.h"

static BOOL DragEnabled = true;

void sliderEnableDrag(BOOL Enable)
{
	DragEnabled = Enable;
}

/* Create a slider widget data structure */
W_SLIDER* sliderCreate(const W_SLDINIT* psInit)
{
	W_SLIDER* psWidget;

	if (psInit->style & ~(WBAR_PLAIN | WIDG_HIDDEN))
	{
		ASSERT(false, "sliderCreate: Unknown style");
		return NULL;
	}

	if (psInit->orientation < WSLD_LEFT
	 || psInit->orientation > WSLD_BOTTOM)
	{
		ASSERT(false, "sliderCreate: Unknown orientation");
		return NULL;
	}

	if (((psInit->orientation == WSLD_LEFT
	   || psInit->orientation == WSLD_RIGHT)
	  && psInit->numStops > (psInit->width - psInit->barSize))
	 || ((psInit->orientation == WSLD_TOP
	   || psInit->orientation == WSLD_BOTTOM)
	  && psInit->numStops > (psInit->height - psInit->barSize)))
	{
		ASSERT(false, "sliderCreate: Too many stops for slider length");
		return NULL;
	}

	if (psInit->pos > psInit->numStops)
	{
		ASSERT(false, "sliderCreate: slider position greater than stops (%d/%d)", psInit->pos,  psInit->numStops);
		return NULL;
	}

	if (((psInit->orientation == WSLD_LEFT
	   || psInit->orientation == WSLD_RIGHT)
	  && psInit->barSize > psInit->width)
	 || ((psInit->orientation == WSLD_TOP
	   || psInit->orientation == WSLD_BOTTOM)
	  && psInit->barSize > psInit->height))
	{
		ASSERT(false, "sliderCreate: slider bar is larger than slider width");
		return NULL;
	}

	/* Allocate the required memory */
	psWidget = (W_SLIDER *)malloc(sizeof(W_SLIDER));
	if (psWidget == NULL)
	{
		debug(LOG_FATAL, "sliderCreate: Out of memory");
		abort();
		return NULL;
	}
	/* Allocate the memory for the tip and copy it if necessary */
	psWidget->pTip = psInit->pTip;

	/* Initialise the structure */
	psWidget->type = WIDG_SLIDER;
	psWidget->id = psInit->id;
	psWidget->formID = psInit->formID;
	psWidget->style = psInit->style;
	psWidget->x = psInit->x;
	psWidget->y = psInit->y;
	psWidget->width = psInit->width;
	psWidget->height = psInit->height;

	if (psInit->pDisplay)
	{
		psWidget->display = psInit->pDisplay;
	}
	else
	{
		psWidget->display = sliderDisplay;
	}
	psWidget->callback = psInit->pCallback;
	psWidget->pUserData = psInit->pUserData;
	psWidget->UserData = psInit->UserData;
	psWidget->orientation = psInit->orientation;
	psWidget->numStops = psInit->numStops;
	psWidget->barSize = psInit->barSize;

	sliderInitialise(psWidget);

	psWidget->pos = psInit->pos;

	return psWidget;
}


/* Free the memory used by a slider */
void sliderFree(W_SLIDER *psWidget)
{
	ASSERT( psWidget != NULL,
		"sliderFree: Invalid widget pointer" );

	free(psWidget);
}


/* Initialise a slider widget before running it */
void sliderInitialise(W_SLIDER *psWidget)
{
	ASSERT( psWidget != NULL,
		"sliderInitialise: Invalid slider pointer" );

	psWidget->state = 0;
	psWidget->pos = 0;
}


/* Get the current position of a slider bar */
UDWORD widgGetSliderPos(W_SCREEN *psScreen, UDWORD id)
{
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	ASSERT( psWidget != NULL,
		"widgGetSliderPos: couldn't find widget from id" );
	if (psWidget)
	{
		return ((W_SLIDER *)psWidget)->pos;
	}

	return 0;
}

/* Set the current position of a slider bar */
void widgSetSliderPos(W_SCREEN *psScreen, UDWORD id, UWORD pos)
{
	WIDGET	*psWidget;

	psWidget = widgGetFromID(psScreen, id);
	ASSERT( psWidget != NULL,
		"widgGetSliderPos: couldn't find widget from id" );
	if (psWidget)
	{
		if (pos > ((W_SLIDER *)psWidget)->numStops)
		{
			((W_SLIDER *)psWidget)->pos = ((W_SLIDER *)psWidget)->numStops;
		}
		else
		{
			((W_SLIDER *)psWidget)->pos = pos;
		}
	}
}

/* Return the current position of the slider bar on the widget */
static void sliderGetBarBox(W_SLIDER *psSlider, SWORD *pX, SWORD *pY,
							UWORD *pWidth, UWORD *pHeight)
{
	switch (psSlider->orientation)
	{
	case WSLD_LEFT:
		*pX = (SWORD)((psSlider->width - psSlider->barSize)
					 * psSlider->pos / psSlider->numStops);
		*pY = 0;
		*pWidth = psSlider->barSize;
		*pHeight = psSlider->height;
		break;
	case WSLD_RIGHT:
		*pX = (SWORD)(psSlider->width - psSlider->barSize
				- (psSlider->width - psSlider->barSize)
				* psSlider->pos / psSlider->numStops);
		*pY = 0;
		*pWidth = psSlider->barSize;
		*pHeight = psSlider->height;
		break;
	case WSLD_TOP:
		*pX = 0;
		*pY = (SWORD)((psSlider->height - psSlider->barSize)
				* psSlider->pos / psSlider->numStops);
		*pWidth = psSlider->width;
		*pHeight = psSlider->barSize;
		break;
	case WSLD_BOTTOM:
		*pX = 0;
		*pY = (SWORD)(psSlider->height - psSlider->barSize
					- (psSlider->height - psSlider->barSize)
					* psSlider->pos / psSlider->numStops);
		*pWidth = psSlider->width;
		*pHeight = psSlider->barSize;
		break;
	}
}


/* Run a slider widget */
void sliderRun(W_SLIDER *psWidget, W_CONTEXT *psContext)
{
	SDWORD  mx,my;
	UDWORD	stopSize;

	if ((psWidget->state & SLD_DRAG) && !mouseDown(MOUSE_LMB))
	{
		psWidget->state &= ~SLD_DRAG;
		widgSetReturn(psContext->psScreen, (WIDGET *)psWidget);
	}
	else if (!(psWidget->state & SLD_DRAG) && mouseDown(MOUSE_LMB))
	{
		sliderClicked(psWidget, psContext);
	}
	if (psWidget->state & SLD_DRAG)
	{
		/* Figure out where the drag box should be */
		mx = psContext->mx - psWidget->x;
		my = psContext->my - psWidget->y;
		switch (psWidget->orientation)
		{
		case WSLD_LEFT:
			if (mx <= psWidget->barSize/2)
			{
				psWidget->pos = 0;
			}
			else if (mx >= psWidget->width - psWidget->barSize/2)
			{
				psWidget->pos = psWidget->numStops;
			}
			else
			{
				/* Mouse is in the middle of the slider, calculate which stop */
				stopSize = (psWidget->width - psWidget->barSize) / psWidget->numStops;
				psWidget->pos = (UWORD)((mx + stopSize/2 - psWidget->barSize/2)
											* psWidget->numStops
											/ (psWidget->width - psWidget->barSize));
			}
			break;
		case WSLD_RIGHT:
			if (mx <= psWidget->barSize/2)
			{
				psWidget->pos = psWidget->numStops;
			}
			else if (mx >= psWidget->width - psWidget->barSize/2)
			{
				psWidget->pos = 0;
			}
			else
			{
				/* Mouse is in the middle of the slider, calculate which stop */
				stopSize = (psWidget->width - psWidget->barSize) / psWidget->numStops;
				psWidget->pos = (UWORD)(psWidget->numStops
											- (mx + stopSize/2 - psWidget->barSize/2)
												* psWidget->numStops
												/ (psWidget->width - psWidget->barSize));
			}
			break;
		case WSLD_TOP:
			if (my <= psWidget->barSize/2)
			{
				psWidget->pos = 0;
			}
			else if (my >= psWidget->height - psWidget->barSize/2)
			{
				psWidget->pos = psWidget->numStops;
			}
			else
			{
				/* Mouse is in the middle of the slider, calculate which stop */
				stopSize = (psWidget->height - psWidget->barSize) / psWidget->numStops;
				psWidget->pos = (UWORD)((my + stopSize/2 - psWidget->barSize/2)
											* psWidget->numStops
											/ (psWidget->height - psWidget->barSize));
			}
			break;
		case WSLD_BOTTOM:
			if (my <= psWidget->barSize/2)
			{
				psWidget->pos = psWidget->numStops;
			}
			else if (my >= psWidget->height - psWidget->barSize/2)
			{
				psWidget->pos = 0;
			}
			else
			{
				/* Mouse is in the middle of the slider, calculate which stop */
				stopSize = (psWidget->height - psWidget->barSize) / psWidget->numStops;
				psWidget->pos = (UWORD)(psWidget->numStops
											- (my + stopSize/2 - psWidget->barSize/2)
												* psWidget->numStops
												/ (psWidget->height - psWidget->barSize));
			}
			break;
		}
	}
}


/* Respond to a mouse click */
void sliderClicked(W_SLIDER *psWidget, W_CONTEXT *psContext)
{
#if 0
	SWORD	x,y;
	UWORD	width,height;
	SDWORD	mx,my;

	/* Get the slider position */
	sliderGetBarBox(psWidget, &x,&y, &width,&height);

	/* Did the mouse click on the slider ? */
	mx = psContext->mx - psWidget->x;
	my = psContext->my - psWidget->y;
#endif
	if(DragEnabled) {
		if (psContext->mx >= psWidget->x &&
			psContext->mx <= psWidget->x + psWidget->width &&
			psContext->my >= psWidget->y &&
			psContext->my <= psWidget->y + psWidget->height)
		{
			psWidget->state |= SLD_DRAG;
		}
	}
}


/* Respond to a mouse up */
void sliderReleased(W_SLIDER *psWidget)
{
	(void)psWidget;
}


/* Respond to a mouse moving over a slider */
void sliderHiLite(W_SLIDER *psWidget)
{
	psWidget->state |= SLD_HILITE;
}


/* Respond to the mouse moving off a slider */
void sliderHiLiteLost(W_SLIDER *psWidget)
{
	psWidget->state &= ~SLD_HILITE;
}

/* The slider display function */
void sliderDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	W_SLIDER	*psSlider;
	SWORD		x0,y0, x1,y1;
	UWORD		width = 0, height = 0;

	psSlider = (W_SLIDER *)psWidget;

	switch (psSlider->orientation)
	{
	case WSLD_LEFT:
	case WSLD_RIGHT:
		/* Draw the line */
		x0 = (SWORD)(psSlider->x + xOffset + psSlider->barSize/(SWORD)2);
		y0 = (SWORD)(psSlider->y + yOffset + psSlider->height/(SWORD)2);
		x1 = (SWORD)(x0 + psSlider->width - psSlider->barSize);
		iV_Line(x0,y0, x1,y0, pColours[WCOL_DARK]);
		iV_Line(x0,y0+1, x1,y0+1, pColours[WCOL_LIGHT]);

		/* Now Draw the bar */
		sliderGetBarBox(psSlider, &x0,&y0, &width,&height);
		x0 = (SWORD)(x0 + psSlider->x + xOffset);
		y0 = (SWORD)(y0 + psSlider->y + yOffset);
		x1 = (SWORD)(x0 + width);
		y1 = (SWORD)(y0 + height);
		pie_BoxFill(x0, y0, x1, y1, pColours[WCOL_BKGRND]);
		iV_Line(x0,y0, x1,y0, pColours[WCOL_LIGHT]);
		iV_Line(x0,y0, x0,y1, pColours[WCOL_LIGHT]);
		iV_Line(x1,y0, x1,y1, pColours[WCOL_DARK]);
		iV_Line(x0,y1, x1,y1, pColours[WCOL_DARK]);
		break;
	case WSLD_TOP:
	case WSLD_BOTTOM:
		/* Draw the line */
		x0 = (SWORD)(psSlider->x + xOffset + psSlider->width/(SWORD)2);
		y0 = (SWORD)(psSlider->y + yOffset + psSlider->barSize/(SWORD)2);
		y1 = (SWORD)(y0 + psSlider->height - psSlider->barSize);
		iV_Line(x0,y0, x0,y1, pColours[WCOL_DARK]);
		iV_Line(x0+1,y0, x0+1,y1, pColours[WCOL_LIGHT]);

		/* Now Draw the bar */
		sliderGetBarBox(psSlider, &x0,&y0, &width,&height);
		x0 = (SWORD)(x0 + psSlider->x + xOffset);
		y0 = (SWORD)(y0 + psSlider->y + yOffset);
		x1 = (SWORD)(x0 + width);
		y1 = (SWORD)(y0 + height);
		pie_BoxFill(x0, y0, x1, y1, pColours[WCOL_BKGRND]);
		iV_Line(x0,y0, x1,y0, pColours[WCOL_LIGHT]);
		iV_Line(x0,y0, x0,y1, pColours[WCOL_LIGHT]);
		iV_Line(x1,y0, x1,y1, pColours[WCOL_DARK]);
		iV_Line(x0,y1, x1,y1, pColours[WCOL_DARK]);
		break;
	}

	if (psSlider->state & SLD_HILITE)
	{
		x0 = (SWORD)(psWidget->x + xOffset - 2);
		y0 = (SWORD)(psWidget->y + yOffset - 2);
		x1 = (SWORD)(x0 + psWidget->width + 4);
		y1 = (SWORD)(y0 + psWidget->height + 4);
		iV_Line(x0,y0, x1,y0, pColours[WCOL_HILITE]);
		iV_Line(x1,y0, x1,y1, pColours[WCOL_HILITE]);
		iV_Line(x0,y1, x1,y1, pColours[WCOL_HILITE]);
		iV_Line(x0,y0, x0,y1, pColours[WCOL_HILITE]);
	}
}
