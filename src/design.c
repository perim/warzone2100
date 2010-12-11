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
 * @file design.c
 *
 * Functions for design screen.
 *
 */
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/widget/widget.h"

#include "objects.h"
#include "loop.h"
#include "map.h"

/* Includes direct access to render library */
#include "lib/ivis_common/ivisdef.h"
#include "lib/ivis_common/bitimage.h"
#include "lib/ivis_common/pieblitfunc.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"//matrix code
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_common/piemode.h"

#include "display3d.h"
#include "edit3d.h"
#include "structure.h"
#include "research.h"
#include "function.h"
#include "lib/gamelib/gtime.h"
#include "hci.h"
#include "stats.h"
#include "game.h"
#include "power.h"
#include "lib/sound/audio.h"
#include "lib/widget/widgint.h"
#include "lib/widget/bar.h"
#include "lib/widget/form.h"
#include "lib/widget/label.h"
#include "lib/widget/button.h"
#include "lib/widget/editbox.h"
#include "lib/widget/slider.h"
#include "order.h"
#include "projectile.h"

#include "intimage.h"
#include "intdisplay.h"
#include "design.h"
#include "component.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "main.h"
#include "objects.h"
#include "display.h"
#include "console.h"
#include "cmddroid.h"
#include "scriptextern.h"
#include "mission.h"

#include "multiplay.h"
#include "multistat.h"

#define FLASH_BUTTONS		// Enable flashing body part buttons.

#define MAX_TABS	4

#define TAB_USEMAJOR 0
#define TAB_USEMINOR 1

//how many buttons can be put on the system component form
#define DES_BUTSPERFORM  8

#define MAX_DESIGN_COMPONENTS 40	// Max number of stats the design screen can cope with.
#define MAX_SYSTEM_COMPONENTS 32	// can only fit 8 buttons on a system component form


/***************************************************************************************/
/*                  Max values for the design bar graphs                               */

#define DBAR_TEMPLATEMAXPOINTS      8400            //maximum body points for a template
#define DBAR_TEMPLATEMAXPOWER       1000            //maximum power points for a template

/* The maximum number of characters on the component buttons */
#define DES_COMPBUTMAXCHAR			5


/* Which type of system is displayed on the design screen */
typedef enum _des_sysmode
{
	IDES_SENSOR,		// The sensor clickable is displayed
	IDES_ECM,			// The ECM clickable is displayed
	IDES_CONSTRUCT,		// The Constructor clickable is displayed
	IDES_REPAIR,		// The Repair clickable is displayed
	IDES_WEAPON,		// The Weapon clickable is displayed
	IDES_COMMAND,		// The command droid clickable is displayed
	IDES_NOSYSTEM,		// No system clickable has been displayed
} DES_SYSMODE;
static DES_SYSMODE desSysMode;

/* The major component tabs on the design screen */
#define IDES_MAINTAB	0
#define IDES_EXTRATAB	1
#define IDES_EXTRATAB2	2

/* Which component type is being selected on the design screen */
//added IDES_TURRET_A,IDES_TURRET_B,changing the name of IDES_TURRET might break exist codes
typedef enum _des_compmode
{
	IDES_SYSTEM,		// The main system for the droid (sensor, ECM, constructor)
	IDES_TURRET,		// The weapon for the droid
	IDES_BODY,			// The droid body
	IDES_PROPULSION,	// The propulsion system
	IDES_NOCOMPONENT,	// No system has been selected
	IDES_TURRET_A,		// The 2nd turret
	IDES_TURRET_B,		// The 3rd turret
} DES_COMPMODE;
static DES_COMPMODE desCompMode;

/* Which type of propulsion is being selected */
typedef enum _des_propmode
{
	IDES_GROUND,		// Ground propulsion (wheeled, tracked, etc).
	IDES_AIR,			// Air propulsion
	IDES_NOPROPULSION,	// No propulsion has been selected
} DES_PROPMODE;
static DES_PROPMODE desPropMode;


#define STRING_BUFFER_SIZE (32 * MAX_STR_LENGTH)
char StringBuffer[STRING_BUFFER_SIZE];

/* Design screen positions */
#define DESIGN_Y				(59 + D_H)	//the top left y value for all forms on the design screen

#define DES_TABTHICKNESS	0
#define DES_MAJORSIZE		40
#define DES_MINORSIZE		11
#define DES_TABBUTGAP		2
#define DES_TABBUTWIDTH		60
#define DES_TABBUTHEIGHT	46
#define DES_TITLEY			10
#define DES_TITLEHEIGHT		20
#define DES_NAMELABELX		10
#define DES_NAMELABELWIDTH	100
#define	DES_TAB_LEFTOFFSET 	OBJ_TABOFFSET
#define	DES_TAB_RIGHTOFFSET	OBJ_TABOFFSET
#define	DES_TAB_SYSOFFSET	0
#define DES_TAB_SYSWIDTH	12
#define DES_TAB_SYSHEIGHT	19
#define DES_TAB_WIDTH		OBJ_TABWIDTH
#define DES_TAB_HEIGHT		OBJ_TABHEIGHT
#define DES_TAB_SYSHORZOFFSET	OBJ_TABOFFSET
#define DES_TAB_SYSGAP		4

#define DES_LEFTFORMX		RET_X
#define DES_LEFTFORMY		DESIGN_Y
#define DES_LEFTFORMWIDTH	RET_FORMWIDTH
#define DES_LEFTFORMHEIGHT	258
#define	DES_LEFTFORMBUTX	2
#define	DES_LEFTFORMBUTY	2

#define DES_CENTERFORMWIDTH		315
#define DES_CENTERFORMHEIGHT	262
#define DES_CENTERFORMX			POW_X
#define DES_CENTERFORMY			DESIGN_Y

#define DES_3DVIEWX				8
#define DES_3DVIEWY				25
#define DES_3DVIEWWIDTH			236
#define DES_3DVIEWHEIGHT		192

#define	DES_STATSFORMX			POW_X
#define	DES_STATSFORMY			(DES_CENTERFORMY + DES_CENTERFORMHEIGHT + 3)
#define	DES_STATSFORMWIDTH		DES_CENTERFORMWIDTH
#define	DES_STATSFORMHEIGHT		100

#define	DES_PARTFORMX			DES_3DVIEWX + DES_3DVIEWWIDTH + 2
#define	DES_PARTFORMY			DES_3DVIEWY
#define	DES_PARTFORMHEIGHT		DES_3DVIEWHEIGHT

#define DES_POWERFORMX			DES_3DVIEWX
#define DES_POWERFORMY			(DES_3DVIEWY + DES_3DVIEWHEIGHT + 2)
#define DES_POWERFORMWIDTH		(DES_CENTERFORMWIDTH - 2*DES_POWERFORMX)
#define DES_POWERFORMHEIGHT		40

#define DES_RIGHTFORMX		RADTLX
#define DES_RIGHTFORMY		DESIGN_Y
#define DES_RIGHTFORMWIDTH	(RET_FORMWIDTH + 20)
#define DES_RIGHTFORMHEIGHT DES_LEFTFORMHEIGHT
#define	DES_RIGHTFORMBUTX	2
#define	DES_RIGHTFORMBUTY	2

#define DES_BARFORMX		6
#define DES_BARFORMY		6
#define	DES_BARFORMWIDTH	300
#define	DES_BARFORMHEIGHT	85

#define DES_NAMEBOXX		DES_3DVIEWX
#define DES_NAMEBOXY		6
#define	DES_NAMEBOXWIDTH	DES_CENTERFORMWIDTH - 2*DES_NAMEBOXX
#define	DES_NAMEBOXHEIGHT	14

/* The central boxes on the design screen */
#define DES_COMPBUTWIDTH	150
#define DES_COMPBUTHEIGHT	85

#define DES_MAINBUTWIDTH	36
#define DES_MAINBUTHEIGHT	24

#define DES_ICONX			5
#define DES_ICONY			22//28
#define DES_ICONWIDTH		76
#define DES_ICONHEIGHT		53

#define DES_POWERX				1
#define DES_POWERY				6
#define DES_POWERSEPARATIONX	4
#define DES_POWERSEPARATIONY	2

#define	DES_PARTSEPARATIONX		6
#define	DES_PARTSEPARATIONY		6

/* Positions of stuff on the clickable boxes (Design screen) */
#define DES_CLICKLABELHEIGHT	14
#define DES_CLICKBARX			154
#define DES_CLICKBARY			7
#define DES_CLICKBARWIDTH		140
#define DES_CLICKBARHEIGHT		11
#define DES_CLICKGAP			9
#define DES_CLICKBARNAMEX		126
#define DES_CLICKBARNAMEWIDTH	20
#define DES_CLICKBARNAMEHEIGHT	19

#define DES_CLICKBARMAJORRED	255		//0xcc
#define DES_CLICKBARMAJORGREEN	235		//0
#define DES_CLICKBARMAJORBLUE	19		//0
#define DES_CLICKBARMINORRED	0x55
#define DES_CLICKBARMINORGREEN	0
#define DES_CLICKBARMINORBLUE	0

#define DES_WEAPONBUTTON_X		26
#define DES_SYSTEMBUTTON_X		68
#define DES_SYSTEMBUTTON_Y		10

// Stat bar y positions.
#define	DES_STATBAR_Y1	(DES_CLICKBARY)
#define	DES_STATBAR_Y2	(DES_CLICKBARY+DES_CLICKBARHEIGHT + DES_CLICKGAP)
#define	DES_STATBAR_Y3	(DES_CLICKBARY+(DES_CLICKBARHEIGHT + DES_CLICKGAP)*2)
#define	DES_STATBAR_Y4	(DES_CLICKBARY+(DES_CLICKBARHEIGHT + DES_CLICKGAP)*3)

/* the widget screen */
extern W_SCREEN		*psWScreen;

/* default droid design template */
DROID_TEMPLATE sDefaultDesignTemplate;

extern void intDisplayPlainForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void desSetupDesignTemplates(void);

/* Set the current mode of the design screen, and display the appropriate component lists */
static void intSetDesignMode(DES_COMPMODE newCompMode);
/* Set all the design bar graphs from a design template */
static void intSetDesignStats(DROID_TEMPLATE *psTemplate);
/* Set up the system clickable form of the design screen given a set of stats */
static BOOL intSetSystemForm(COMPONENT_STATS *psStats);
/* Set up the propulsion clickable form of the design screen given a set of stats */
static BOOL intSetPropulsionForm(PROPULSION_STATS *psStats);
/* Add the component tab form to the design screen */
static BOOL intAddComponentForm(UDWORD numButtons);
/* Add the template tab form to the design screen */
static BOOL intAddTemplateForm(DROID_TEMPLATE *psSelected);
/* Add the Major system tab form to the design screen */
// count the number of available components
static UDWORD intNumAvailable(UBYTE *aAvailable, UDWORD numEntries,
							  COMPONENT_STATS *asStats, UDWORD size);
/* Add the system buttons (weapons, command droid, etc) to the design screen */
static BOOL intAddSystemButtons(DES_COMPMODE mode);
/* Add the component buttons to the main tab of the system or component form */
static BOOL intAddComponentButtons(COMPONENT_STATS *psStats, UDWORD size,
								   UBYTE *aAvailable,	UDWORD numEntries,
								   UDWORD compID,UDWORD WhichTab);
/* Add the component buttons to the main tab of the component form */
static BOOL intAddExtraSystemButtons(UDWORD sensorIndex, UDWORD ecmIndex,
									 UDWORD constIndex, UDWORD repairIndex, UDWORD brainIndex);
/* Set the bar graphs for the system clickable */
static void intSetSystemStats(COMPONENT_STATS *psStats);
/* Set the shadow bar graphs for the system clickable */
static void intSetSystemShadowStats(COMPONENT_STATS *psStats);
/* Set the bar graphs for the sensor stats */
static void intSetSensorStats(SENSOR_STATS *psStats);
/* Set the shadow bar graphs for the sensor stats */
static void intSetSensorShadowStats(SENSOR_STATS *psStats);
/* Set the bar graphs for the ECM stats */
static void intSetECMStats(ECM_STATS *psStats);
/* Set the shadow bar graphs for the ECM stats */
static void intSetECMShadowStats(ECM_STATS *psStats);
/* Set the bar graphs for the Repair stats */
static void intSetRepairStats(REPAIR_STATS *psStats);
/* Set the shadow bar graphs for the Repair stats */
static void intSetRepairShadowStats(REPAIR_STATS *psStats);
/* Set the bar graphs for the Constructor stats */
static void intSetConstructStats(CONSTRUCT_STATS *psStats);
/* Set the shadow bar graphs for the Constructor stats */
static void intSetConstructShadowStats(CONSTRUCT_STATS *psStats);
/* Set the bar graphs for the Weapon stats */
static void intSetWeaponStats(WEAPON_STATS *psStats);
/* Set the shadow bar graphs for the weapon stats */
static void intSetWeaponShadowStats(WEAPON_STATS *psStats);
/* Set the bar graphs for the Body stats */
static void intSetBodyStats(BODY_STATS *psStats);
/* Set the shadow bar graphs for the Body stats */
static void intSetBodyShadowStats(BODY_STATS *psStats);
/* Set the bar graphs for the Propulsion stats */
static void intSetPropulsionStats(PROPULSION_STATS *psStats);
/* Set the shadow bar graphs for the Propulsion stats */
static void intSetPropulsionShadowStats(PROPULSION_STATS *psStats);
/* Check whether a droid template is valid */
BOOL intValidTemplate(DROID_TEMPLATE *psTempl, const char *newName);
/* General display window for the design form */
void intDisplayDesignForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
/* Sets the Design Power Bar for a given Template */
static void intSetDesignPower(DROID_TEMPLATE *psTemplate);
/* Sets the Power shadow Bar for the current Template with new stat*/
static void intSetTemplatePowerShadowStats(COMPONENT_STATS *psStats);
/* Sets the Body Points Bar for a given Template */
static void intSetBodyPoints(DROID_TEMPLATE *psTemplate);
/* Sets the Body Points shadow Bar for the current Template with new stat*/
static void intSetTemplateBodyShadowStats(COMPONENT_STATS *psStats);
/* set flashing flag for button */
static void intSetButtonFlash( UDWORD id, BOOL bFlash );
/*Function to set the shadow bars for all the stats when the mouse is over
the Template buttons*/
static void runTemplateShadowStats(UDWORD id);

static BOOL intCheckValidWeaponForProp(void);

static BOOL checkTemplateIsVtol(DROID_TEMPLATE *psTemplate);

/* save the current Template if valid. Return true if stored */
static BOOL saveTemplate(void);

static void desCreateDefaultTemplate( void );

/* The current name of the design */
static char			aCurrName[WIDG_MAXSTR];

/* Store a list of component stats pointers for the design screen */
extern UDWORD			maxComponent;
extern UDWORD			numComponent;
extern COMPONENT_STATS	**apsComponentList;
extern UDWORD			maxExtraSys;
extern UDWORD			numExtraSys;
extern COMPONENT_STATS	**apsExtraSysList;

/* The button id of the component that is in the design */
static UDWORD			desCompID;

static UDWORD			droidTemplID;

/* The current design being edited on the design screen */
DROID_TEMPLATE			sCurrDesign;

/* Flag to indictate whether a 'spare' template button is required */
static BOOL				newTemplate = false;

static void intDisplayStatForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void intDisplayViewForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
void intDisplayTemplateButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);
static void intDisplayComponentButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

extern BOOL bRender3DOnly;


/* Add the design widgets to the widget screen */
static BOOL _intAddDesign( BOOL bShowCentreScreen )
{
	W_FORMINIT		sFormInit;
	W_LABINIT		sLabInit;
	W_EDBINIT		sEdInit;
	W_BUTINIT		sButInit;
	W_BARINIT		sBarInit;

	desSetupDesignTemplates();

	//set which states are to be paused while design screen is up
	setDesignPauseState();

	if((GetGameMode() == GS_NORMAL) && !bMultiPlayer)
	{	// Only do this in main game.
		BOOL radOnScreen = radarOnScreen;

		bRender3DOnly = true;
		radarOnScreen = false;

	// Just display the 3d, no interface
		displayWorld();
	// Upload the current display back buffer into system memory.
		pie_UploadDisplayBuffer();

		radarOnScreen = radOnScreen;
		bRender3DOnly = false;
	}

	//initialise flags
	newTemplate = false;

	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	memset(&sLabInit, 0, sizeof(W_LABINIT));
	memset(&sEdInit, 0, sizeof(W_EDBINIT));
	memset(&sButInit, 0, sizeof(W_BUTINIT));
	memset(&sBarInit, 0, sizeof(W_BARINIT));

	/* Add the main design form */
	sFormInit.formID = 0;
	sFormInit.id = IDDES_FORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = (SWORD)DES_CENTERFORMX;	//0;
	sFormInit.y = (SWORD)DES_CENTERFORMY;	//0;
	sFormInit.width = DES_CENTERFORMWIDTH;	//DISP_WIDTH-1;
	sFormInit.height = DES_CENTERFORMHEIGHT;	//DES_BASEHEIGHT;
	sFormInit.pDisplay = intDisplayPlainForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* add the edit name box */
	sEdInit.formID = IDDES_FORM;
	sEdInit.id = IDDES_NAMEBOX;
	sEdInit.style = WEDB_PLAIN;
	sEdInit.x = DES_NAMEBOXX;
	sEdInit.y = DES_NAMEBOXY;
	sEdInit.width = DES_NAMEBOXWIDTH;
	sEdInit.height = DES_NAMEBOXHEIGHT;
	sEdInit.pText = _("New Vehicle");
	sEdInit.FontID = font_regular;
	sEdInit.pBoxDisplay = intDisplayEditBox;
	if (!widgAddEditBox(psWScreen, &sEdInit))
	{
		return false;
	}

	CurrentStatsTemplate = NULL;

	/* Initialise the current design */
	if (psCurrTemplate != NULL)
	{
		memcpy(&sCurrDesign, psCurrTemplate, sizeof(DROID_TEMPLATE));
		sstrcpy(aCurrName, getStatName(psCurrTemplate));
		sstrcpy(sCurrDesign.aName, aCurrName);
	}
	else
	{
		memcpy(&sCurrDesign, &sDefaultDesignTemplate, sizeof(DROID_TEMPLATE));
		sCurrDesign.pName = NULL;
		sstrcpy(aCurrName, _("New Vehicle"));
		sstrcpy(sCurrDesign.aName, aCurrName);
	}

	/* Add the design templates form */
	if (!intAddTemplateForm(psCurrTemplate))
	{
		return false;
	}

	/* Add the 3D View form */
	sFormInit.formID = IDDES_FORM;
	sFormInit.id = IDDES_3DVIEW;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = DES_3DVIEWX;
	sFormInit.y = DES_3DVIEWY;
	sFormInit.width = DES_3DVIEWWIDTH;
	sFormInit.height = DES_3DVIEWHEIGHT;
	sFormInit.pDisplay = intDisplayViewForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Add the part button form */
	sFormInit.formID = IDDES_FORM;
	sFormInit.id = IDDES_PARTFORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = DES_PARTFORMX;
	sFormInit.y = DES_PARTFORMY;
	sFormInit.width = (UWORD)(iV_GetImageWidth(IntImages, IMAGE_DES_TURRET) +
						2*DES_PARTSEPARATIONX);
	sFormInit.height = DES_PARTFORMHEIGHT;
	sFormInit.pDisplay = intDisplayDesignForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	// add the body part button
	sButInit.formID = IDDES_PARTFORM;
	sButInit.id = IDDES_BODYBUTTON;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = DES_PARTSEPARATIONX;
	sButInit.y = DES_PARTSEPARATIONY;
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_BODY);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_BODY);
	sButInit.pTip = _("Vehicle Body");
	sButInit.FontID = font_regular;
#ifdef FLASH_BUTTONS
	sButInit.pDisplay = intDisplayButtonFlash;
#else
	sButInit.pDisplay = intDisplayButtonHilight;
#endif
	sButInit.UserData = PACKDWORD_TRI(1, IMAGE_DES_BODYH, IMAGE_DES_BODY);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	// add the propulsion part button
	sButInit.formID = IDDES_PARTFORM;
	sButInit.id = IDDES_PROPBUTTON;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = DES_PARTSEPARATIONX;
	sButInit.y = (UWORD)(iV_GetImageHeight(IntImages, IMAGE_DES_PROPULSION) +
					2 * DES_PARTSEPARATIONY);
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_PROPULSION);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_PROPULSION);
	sButInit.pTip = _("Vehicle Propulsion");
	sButInit.FontID = font_regular;
#ifdef FLASH_BUTTONS
	sButInit.pDisplay = intDisplayButtonFlash;
#else
	sButInit.pDisplay = intDisplayButtonHilight;
#endif
	sButInit.UserData = PACKDWORD_TRI(1, IMAGE_DES_PROPULSIONH, IMAGE_DES_PROPULSION);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	// add the turret part button
	sButInit.formID = IDDES_PARTFORM;
	sButInit.id = IDDES_SYSTEMBUTTON;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = DES_PARTSEPARATIONX;
	sButInit.y = (UWORD)(iV_GetImageHeight(IntImages, IMAGE_DES_PROPULSION) +
				 iV_GetImageHeight(IntImages, IMAGE_DES_BODY)   +
				 3*DES_PARTSEPARATIONY);
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_TURRET);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_TURRET);
	sButInit.pTip = _("Vehicle Turret");
	sButInit.FontID = font_regular;
#ifdef FLASH_BUTTONS
	sButInit.pDisplay = intDisplayButtonFlash;
#else
	sButInit.pDisplay = intDisplayButtonHilight;
#endif
	sButInit.UserData = PACKDWORD_TRI(1, IMAGE_DES_TURRETH, IMAGE_DES_TURRET);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	// add the turret_a button
	sButInit.formID = IDDES_PARTFORM;
	sButInit.id = IDDES_WPABUTTON;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = DES_PARTSEPARATIONX;
	// use BODY height for now
	sButInit.y = (UWORD)(iV_GetImageHeight(IntImages, IMAGE_DES_PROPULSION) +
				 iV_GetImageHeight(IntImages, IMAGE_DES_BODY)   +
				 iV_GetImageHeight(IntImages, IMAGE_DES_BODY)   +
				 4*DES_PARTSEPARATIONY);
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_TURRET);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_TURRET);
	sButInit.pTip = _("Vehicle Turret");
	sButInit.FontID = font_regular;
#ifdef FLASH_BUTTONS
	sButInit.pDisplay = intDisplayButtonFlash;
#else
	sButInit.pDisplay = intDisplayButtonHilight;
#endif
	sButInit.UserData = PACKDWORD_TRI(1, IMAGE_DES_TURRETH, IMAGE_DES_TURRET);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	// add the turret_b button
	sButInit.formID = IDDES_PARTFORM;
	sButInit.id = IDDES_WPBBUTTON;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = DES_PARTSEPARATIONX;
	//use body height for now
	sButInit.y = (UWORD)(iV_GetImageHeight(IntImages, IMAGE_DES_PROPULSION) +
				 iV_GetImageHeight(IntImages, IMAGE_DES_BODY)   +
				 iV_GetImageHeight(IntImages, IMAGE_DES_BODY)   +
				 iV_GetImageHeight(IntImages, IMAGE_DES_BODY)   +
				 5*DES_PARTSEPARATIONY);
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_TURRET);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_TURRET);
	sButInit.pTip = _("Vehicle Turret");
	sButInit.FontID = font_regular;
#ifdef FLASH_BUTTONS
	sButInit.pDisplay = intDisplayButtonFlash;
#else
	sButInit.pDisplay = intDisplayButtonHilight;
#endif
	sButInit.UserData = PACKDWORD_TRI(1, IMAGE_DES_TURRETH, IMAGE_DES_TURRET);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	/* add the delete button */
	sButInit.formID = IDDES_PARTFORM;
	sButInit.id = IDDES_BIN;
	sButInit.style = WBUT_PLAIN;
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_BIN);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_BIN);
	sButInit.x = DES_PARTSEPARATIONX;
	sButInit.y = (UWORD)(DES_PARTFORMHEIGHT - sButInit.height - DES_PARTSEPARATIONY);
	sButInit.pTip = _("Delete Design");
	sButInit.FontID = font_regular;
	sButInit.pDisplay = intDisplayButtonHilight;
	sButInit.UserData = PACKDWORD_TRI(0,IMAGE_DES_BINH, IMAGE_DES_BIN);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

	/* add central stats form */
	sFormInit.formID = 0;
	sFormInit.id = IDDES_STATSFORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = (SWORD)DES_STATSFORMX;
	sFormInit.y = (SWORD)DES_STATSFORMY;
	sFormInit.width = DES_STATSFORMWIDTH;
	sFormInit.height = DES_STATSFORMHEIGHT;
	sFormInit.pDisplay = intDisplayPlainForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Add the body form */
	sFormInit.formID = IDDES_STATSFORM;
	sFormInit.id = IDDES_BODYFORM;
	sFormInit.style = WFORM_CLICKABLE | WFORM_NOCLICKMOVE;
	sFormInit.width = DES_BARFORMWIDTH;
	sFormInit.height = DES_BARFORMHEIGHT;
	sFormInit.x = DES_BARFORMX;
	sFormInit.y = DES_BARFORMY;
	sFormInit.pDisplay = intDisplayStatForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Add the graphs for the Body */
	sBarInit.formID = IDDES_BODYFORM;
	sBarInit.id = IDDES_BODYARMOUR_K;
	sBarInit.style = WBAR_PLAIN;
	sBarInit.orientation = WBAR_LEFT;
	sBarInit.x = DES_CLICKBARX;
	sBarInit.y = DES_STATBAR_Y1;	//DES_CLICKBARY;
	sBarInit.width = DES_CLICKBARWIDTH;
	sBarInit.height = DES_CLICKBARHEIGHT;
	sBarInit.size = 50;
	sBarInit.sCol.byte.r = DES_CLICKBARMAJORRED;
	sBarInit.sCol.byte.g = DES_CLICKBARMAJORGREEN;
	sBarInit.sCol.byte.b = DES_CLICKBARMAJORBLUE;
	sBarInit.sMinorCol.byte.r = DES_CLICKBARMINORRED;
	sBarInit.sMinorCol.byte.g = DES_CLICKBARMINORGREEN;
	sBarInit.sMinorCol.byte.b = DES_CLICKBARMINORBLUE;
	sBarInit.pDisplay = intDisplayStatsBar;
	sBarInit.pTip = _("Kinetic Armour");
	sBarInit.iRange = (UWORD)getMaxBodyArmour();//DBAR_BODYMAXARMOUR;
	if (!widgAddBarGraph(psWScreen, &sBarInit))
	{
		return true;
	}

	sBarInit.id = IDDES_BODYARMOUR_H;
	sBarInit.y  = DES_STATBAR_Y2;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
	sBarInit.pTip = _("Thermal Armour");
	sBarInit.iRange = (UWORD)getMaxBodyArmour();//DBAR_BODYMAXARMOUR;
	if (!widgAddBarGraph(psWScreen, &sBarInit))
	{
		return true;
	}

	//body points added AB 3/9/97
	//sBarInit.id = IDDES_BODYPOINTS;
	//sBarInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
	//if (!widgAddBarGraph(psWScreen, &sBarInit))
	//{
	//	return true;
	//}
	sBarInit.id = IDDES_BODYPOWER;
	sBarInit.y = DES_STATBAR_Y3;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
	sBarInit.pTip = _("Engine Output");
	sBarInit.iRange = (UWORD)getMaxBodyPower();//DBAR_BODYMAXPOWER;
	if (!widgAddBarGraph(psWScreen, &sBarInit))
	{
		return true;
	}
	sBarInit.id = IDDES_BODYWEIGHT;
	sBarInit.y = DES_STATBAR_Y4;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
	sBarInit.pTip = _("Weight");
	sBarInit.iRange = (UWORD)getMaxComponentWeight();//DBAR_MAXWEIGHT;
	if (!widgAddBarGraph(psWScreen, &sBarInit))
	{
		return true;
	}

	/* Add the labels for the Body */
	sLabInit.formID = IDDES_BODYFORM;
	sLabInit.id = IDDES_BODYARMOURKLAB;
	sLabInit.style = WLAB_PLAIN;
	sLabInit.x = DES_CLICKBARNAMEX;
	sLabInit.y = DES_CLICKBARY - DES_CLICKBARHEIGHT/3;
	sLabInit.width = DES_CLICKBARNAMEWIDTH;
	sLabInit.height = DES_CLICKBARHEIGHT;
	sLabInit.pTip = _("Kinetic Armour");
	sLabInit.FontID = font_regular;
	sLabInit.pDisplay = intDisplayImage;
	//just to confuse things even more - the graphics were named incorrectly!
	sLabInit.UserData = IMAGE_DES_ARMOUR_EXPLOSIVE;//IMAGE_DES_ARMOUR_KINETIC;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return true;
	}
	sLabInit.id = IDDES_BODYARMOURHLAB;
	sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
	sLabInit.pTip = _("Thermal Armour");
	sLabInit.pDisplay = intDisplayImage;
	sLabInit.UserData = IMAGE_DES_ARMOUR_KINETIC;//IMAGE_DES_ARMOUR_EXPLOSIVE;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return true;
	}
	//body points added AB 3/9/97
	//sLabInit.id = IDDES_BODYPOINTSLAB;
	//sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
	//sLabInit.pText = "Body Points";
	//sLabInit.pTip = sLabInit.pText;
	//sLabInit.pDisplay = intDisplayImage;
	//sLabInit.pUserData = (void*)IMAGE_DES_BODYPOINTS;
	//if (!widgAddLabel(psWScreen, &sLabInit))
	//{
	//	return true;
	//}
	sLabInit.id = IDDES_BODYPOWERLAB;
	sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
	sLabInit.pTip = _("Engine Output");
	sLabInit.pDisplay = intDisplayImage;
	sLabInit.UserData = IMAGE_DES_POWER;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return true;
	}
	sLabInit.id = IDDES_BODYWEIGHTLAB;
	sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
	sLabInit.pTip = _("Weight");
	sLabInit.pDisplay = intDisplayImage;
	sLabInit.UserData = IMAGE_DES_WEIGHT;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return true;
	}

	/* add power/points bar subform */
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = IDDES_FORM;
	sFormInit.id = IDDES_POWERFORM;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = DES_POWERFORMX;
	sFormInit.y = DES_POWERFORMY;
	sFormInit.width = DES_POWERFORMWIDTH;
	sFormInit.height = DES_POWERFORMHEIGHT;
	sFormInit.pDisplay = intDisplayDesignForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Set the text colour for the form */
	widgSetColour(psWScreen, IDDES_POWERFORM, WCOL_TEXT, WZCOL_DESIGN_POWER_FORM_BACKGROUND);

	/* Add the design template power bar and label*/
	sLabInit.formID	= IDDES_POWERFORM;
	sLabInit.id = IDDES_TEMPPOWERLAB;
	sLabInit.x = DES_POWERX;
	sLabInit.y = DES_POWERY;
	sLabInit.pTip = _("Total Power Required");
	sLabInit.pDisplay = intDisplayImage;
	sLabInit.UserData = IMAGE_DES_POWER;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return true;
	}

	memset(&sBarInit, 0, sizeof(W_BARINIT));
	sBarInit.formID = IDDES_POWERFORM;
	sBarInit.id = IDDES_POWERBAR;
	sBarInit.style = WBAR_PLAIN;
	sBarInit.orientation = WBAR_LEFT;
	sBarInit.x = (SWORD)(DES_POWERX + DES_POWERSEPARATIONX +
					iV_GetImageWidth(IntImages,IMAGE_DES_BODYPOINTS));
	sBarInit.y = DES_POWERY;
	sBarInit.width = (SWORD)(DES_POWERFORMWIDTH - 15 -
					iV_GetImageWidth(IntImages,IMAGE_DES_BODYPOINTS));
	sBarInit.height = iV_GetImageHeight(IntImages,IMAGE_DES_POWERBACK);
	sBarInit.pDisplay = intDisplayDesignPowerBar;//intDisplayStatsBar;
	sBarInit.pTip = _("Total Power Required");
	sBarInit.iRange = DBAR_TEMPLATEMAXPOWER;//WBAR_SCALE;
	if (!widgAddBarGraph(psWScreen, &sBarInit))
	{
		return false;
	}

	/* Add the design template body points bar and label*/
	sLabInit.formID	= IDDES_POWERFORM;
	sLabInit.id = IDDES_TEMPBODYLAB;
	sLabInit.x = DES_POWERX;
	sLabInit.y = (SWORD)(DES_POWERY + DES_POWERSEPARATIONY +
						iV_GetImageHeight(IntImages,IMAGE_DES_BODYPOINTS));
	sLabInit.pTip = _("Total Body Points");
	sLabInit.pDisplay = intDisplayImage;
	sLabInit.UserData = IMAGE_DES_BODYPOINTS;
	if (!widgAddLabel(psWScreen, &sLabInit))
	{
		return true;
	}

	memset(&sBarInit, 0, sizeof(W_BARINIT));
	sBarInit.formID = IDDES_POWERFORM;
	sBarInit.id = IDDES_BODYPOINTS;
	sBarInit.style = WBAR_PLAIN;
	sBarInit.orientation = WBAR_LEFT;
	sBarInit.x = (SWORD)(DES_POWERX + DES_POWERSEPARATIONX +
					iV_GetImageWidth(IntImages,IMAGE_DES_BODYPOINTS));
	sBarInit.y = (SWORD)(DES_POWERY + DES_POWERSEPARATIONY + 4 +
							iV_GetImageHeight(IntImages,IMAGE_DES_BODYPOINTS));
	sBarInit.width = (SWORD)(DES_POWERFORMWIDTH - 15 -
					iV_GetImageWidth(IntImages,IMAGE_DES_BODYPOINTS));
	sBarInit.height = iV_GetImageHeight(IntImages,IMAGE_DES_POWERBACK);
	sBarInit.pDisplay = intDisplayDesignPowerBar;//intDisplayStatsBar;
	sBarInit.pTip = _("Total Body Points");
	sBarInit.iRange = DBAR_TEMPLATEMAXPOINTS;//(UWORD)getMaxBodyPoints();//DBAR_BODYMAXPOINTS;
	if (!widgAddBarGraph(psWScreen, &sBarInit))
	{
		return false;
	}

	/* Add the variable bits of the design screen and set the bar graphs */
	desCompMode = IDES_NOCOMPONENT;
	desSysMode = IDES_NOSYSTEM;
	desPropMode = IDES_NOPROPULSION;
	intSetDesignStats(&sCurrDesign);
	intSetBodyPoints(&sCurrDesign);
	intSetDesignPower(&sCurrDesign);
	intSetDesignMode(IDES_BODY);

	/* hide design and component forms until required */
	if ( bShowCentreScreen == false )
	{
		widgHide( psWScreen, IDDES_FORM );
	}
	widgHide( psWScreen, IDDES_STATSFORM );
	widgHide( psWScreen, IDDES_RIGHTBASE );

	return true;
}

/* set up droid templates before going into design screen */
void desSetupDesignTemplates(void)
{
	DROID_TEMPLATE	*psTempl;
	UDWORD			i;

	/* init template list */
	memset( apsTemplateList, 0, sizeof(DROID_TEMPLATE*) * MAXTEMPLATES );
	apsTemplateList[0] = &sDefaultDesignTemplate;
	i = 1;
	psTempl = apsDroidTemplates[selectedPlayer];
	while ((psTempl != NULL) && (i < MAXTEMPLATES))
	{
		/* add template to list if not a transporter,
		 * cyborg, person or command droid,
		 */
		if ( psTempl->droidType != DROID_TRANSPORTER        &&
			 psTempl->droidType != DROID_CYBORG             &&
			 psTempl->droidType != DROID_CYBORG_SUPER       &&
             psTempl->droidType != DROID_CYBORG_CONSTRUCT   &&
             psTempl->droidType != DROID_CYBORG_REPAIR      &&
			 psTempl->droidType != DROID_PERSON	)
		{
			apsTemplateList[i] = psTempl;
			i++;
		}

		/* next template */
		psTempl = psTempl->psNext;
	}
}

/* Add the design template form */
static BOOL _intAddTemplateForm(DROID_TEMPLATE *psSelected)
{
	W_FORMINIT	sFormInit;
	UDWORD		numButtons, butPerForm;
	UDWORD		i;


	/* Count the number of minor tabs needed for the template form */
	numButtons = 0;
	for( i=0; i<MAXTEMPLATES; i++ )
	{
		if ( apsTemplateList[i] != NULL )
		{
			numButtons++;
		}
	}

	/* Calculate how many buttons will go on a single form */
	butPerForm = ((DES_LEFTFORMWIDTH - DES_TABTHICKNESS - DES_TABBUTGAP) /
						(DES_TABBUTWIDTH + DES_TABBUTGAP)) *
				 ((DES_LEFTFORMHEIGHT - DES_TABTHICKNESS - DES_TABBUTGAP) /
						(DES_TABBUTHEIGHT + DES_TABBUTGAP));

	/* add a form to place the tabbed form on */
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = 0;
	sFormInit.id = IDDES_TEMPLBASE;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = (SWORD)RET_X;
	sFormInit.y = (SWORD)DESIGN_Y;
	sFormInit.width = RET_FORMWIDTH;
	sFormInit.height = DES_LEFTFORMHEIGHT + 4;
	sFormInit.pDisplay = intDisplayPlainForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Add the design templates form */
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = IDDES_TEMPLBASE;	//IDDES_FORM;
	sFormInit.id = IDDES_TEMPLFORM;
	sFormInit.style = WFORM_TABBED;
	sFormInit.x = 2;//DES_LEFTFORMX;	//DES_TEMPLX;
	sFormInit.y = 2;//DES_LEFTFORMY;		//DES_TEMPLY;
	sFormInit.width = DES_LEFTFORMWIDTH;	//DES_TEMPLWIDTH;
	sFormInit.height = DES_LEFTFORMHEIGHT;	//DES_TEMPLHEIGHT;
	sFormInit.numMajor = numForms(numButtons, butPerForm);
	sFormInit.majorPos = WFORM_TABTOP;
	sFormInit.minorPos = WFORM_TABNONE;
	sFormInit.majorSize = DES_TAB_WIDTH;
	sFormInit.majorOffset = DES_TAB_LEFTOFFSET;
	sFormInit.tabVertOffset = (DES_TAB_HEIGHT/2);			//(DES_TAB_HEIGHT/2)+2;
	sFormInit.tabMajorThickness = DES_TAB_HEIGHT;
	sFormInit.pUserData = &StandardTab;
	sFormInit.pTabDisplay = intDisplayTab;
	if (sFormInit.numMajor > MAX_TAB_STD_SHOWN)
	{
		// we do NOT want more than this amount of tabs on design screen.
		// 40 templates should be more than enough.
		sFormInit.numMajor = MAX_TAB_STD_SHOWN;
		// If we were to change this in future then :
		//Just switching from normal sized tabs to smaller ones to fit more in form.
		//		sFormInit.pUserData = &SmallTab;
		//		sFormInit.majorSize /= 2;
		// Change MAX_TAB_STD_SHOWN to ..SMALL_SHOWN, this will give us 80 templates max.
	}


	for (i=0; i< sFormInit.numMajor; i++)
	{
		sFormInit.aNumMinors[i] = 1;
	}
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Put the buttons on it */
	if (!intAddTemplateButtons(IDDES_TEMPLFORM, DES_LEFTFORMWIDTH - DES_TABTHICKNESS,
							   DES_LEFTFORMHEIGHT - DES_TABTHICKNESS,
							   DES_TABBUTWIDTH, DES_TABBUTHEIGHT, DES_TABBUTGAP,
							   psSelected ))
	{
		return false;
	}

	return true;
}



/* Add the droid template buttons to a form */
BOOL intAddTemplateButtons(UDWORD formID, UDWORD formWidth, UDWORD formHeight,
								  UDWORD butWidth, UDWORD butHeight, UDWORD gap,
								  DROID_TEMPLATE *psSelected)
{
	W_FORMINIT		sButInit;
	W_BARINIT		sBarInit;
	DROID_TEMPLATE	*psTempl = NULL;
	char			aButText[DES_COMPBUTMAXCHAR + 1];
	SDWORD			BufferID;
	UDWORD			i;
	char TempString[256];
	int BufferPos = 0;

	ClearStatBuffers();

	memset(aButText, 0, DES_COMPBUTMAXCHAR + 1);
	memset(&sButInit, 0, sizeof(W_BUTINIT));

	/* Set up the button struct */
	sButInit.formID = formID;
	sButInit.id = IDDES_TEMPLSTART;
	sButInit.style = WFORM_CLICKABLE;
	sButInit.x = DES_LEFTFORMBUTX;
	sButInit.y = DES_LEFTFORMBUTY;
	sButInit.width = OBJ_BUTWIDTH;			//DES_TABBUTWIDTH;
	sButInit.height = OBJ_BUTHEIGHT;		//DES_TABBUTHEIGHT;

	/* Add each button */
	memset(&sBarInit, 0, sizeof(W_BARINIT));
	sBarInit.id = IDDES_BARSTART;
	sBarInit.style = WBAR_PLAIN;
	sBarInit.orientation = WBAR_LEFT;
	sBarInit.x = STAT_TIMEBARX;
	sBarInit.y = STAT_TIMEBARY;
	sBarInit.width = STAT_PROGBARWIDTH;
	sBarInit.height = STAT_PROGBARHEIGHT;
	sBarInit.size = 50;
	sBarInit.sCol = WZCOL_ACTION_PROGRESS_BAR_MAJOR;
	sBarInit.sMinorCol = WZCOL_ACTION_PROGRESS_BAR_MINOR;
	sBarInit.pTip = _("Power Usage");

	droidTemplID = 0;
	for( i=0; i<MAXTEMPLATES; i++ )
	{
		if ( apsTemplateList[i] != NULL )
		{
			psTempl = apsTemplateList[i];

			/* Set the tip and add the button */


			// On the playstation the tips are additionaly setup when they are displayed ... because we only have one text name buffer
			sstrcpy(aButText, getTemplateName(psTempl));
			sButInit.pTip = getTemplateName(psTempl);

			BufferID = GetStatBuffer();
			ASSERT_OR_RETURN(false, BufferID >= 0,"Unable to aquire stat buffer." );
			RENDERBUTTON_INUSE(&StatBuffers[BufferID]);
			StatBuffers[BufferID].Data = (void*)psTempl;
			sButInit.pUserData = &StatBuffers[BufferID];
			sButInit.pDisplay = intDisplayTemplateButton;

			if (!widgAddForm(psWScreen, &sButInit))
			{
				return false;
			}

			sBarInit.iRange = POWERPOINTS_DROIDDIV;
			sBarInit.size = (UWORD)(psTempl->powerPoints  / POWERPOINTS_DROIDDIV);
			if(sBarInit.size > WBAR_SCALE) sBarInit.size = WBAR_SCALE;

			snprintf(TempString, sizeof(TempString), "%s - %d",_("Power Usage"), psTempl->powerPoints);

			ASSERT(BufferPos + strlen(TempString) + 1 < sizeof(StringBuffer), "String Buffer Overflow");
			strlcpy(&StringBuffer[BufferPos], TempString, sizeof(StringBuffer) - BufferPos);

			sBarInit.pTip = &StringBuffer[BufferPos];
			BufferPos += strlen(TempString) + 1;

			sBarInit.formID = sButInit.id;
			if (!widgAddBarGraph(psWScreen, &sBarInit))
			{
				return false;
			}

			/* if the current template matches psSelected lock the button */
			if (psTempl == psSelected)
			{
				droidTemplID = sButInit.id;
				widgSetButtonState(psWScreen, droidTemplID, WBUT_LOCK);
				widgSetTabs(psWScreen, IDDES_TEMPLFORM, sButInit.majorID, 0);
			}

			/* Update the init struct for the next button */
			sBarInit.id += 1;
			sButInit.id += 1;
			sButInit.x = (SWORD)(sButInit.x + butWidth + gap);
			if (sButInit.x + butWidth + gap > formWidth)
			{
				sButInit.x = DES_LEFTFORMBUTX;
				sButInit.y = (SWORD)(sButInit.y + butHeight + gap);
			}
			if (sButInit.y + butHeight + gap > formHeight)
			{
				sButInit.y = DES_LEFTFORMBUTY;
				sButInit.majorID += 1;
			}
			//check don't go over max templates that can fit on the form
			if (sButInit.id >= IDDES_TEMPLEND)
			{
				break;
			}
		}
	}

	return true;
}


/* Set the current mode of the design screen, and display the appropriate
 * component lists
 * added case IDES_TURRET_A,IDES_TURRET_B
 */
static void intSetDesignMode(DES_COMPMODE newCompMode)
{
	UDWORD	weaponIndex;

	if (newCompMode != desCompMode)
	{
		/* Have to change the component display - remove the old one */
		if (desCompMode != IDES_NOCOMPONENT)
		{
			widgDelete(psWScreen, IDDES_COMPFORM);
			widgDelete(psWScreen, IDDES_RIGHTBASE);

			widgSetButtonState(psWScreen, IDDES_BODYFORM, 0);
			widgSetButtonState(psWScreen, IDDES_PROPFORM, 0);
			widgSetButtonState(psWScreen, IDDES_SYSTEMFORM, 0);
			widgHide(psWScreen, IDDES_BODYFORM);
			widgHide(psWScreen, IDDES_PROPFORM);
			widgHide(psWScreen, IDDES_SYSTEMFORM);

			widgSetButtonState(psWScreen, IDDES_BODYBUTTON, 0);
			widgSetButtonState(psWScreen, IDDES_PROPBUTTON, 0);
			widgSetButtonState(psWScreen, IDDES_SYSTEMBUTTON, 0);
			widgSetButtonState(psWScreen, IDDES_WPABUTTON, 0);
			widgSetButtonState(psWScreen, IDDES_WPBBUTTON, 0);
		}

		/* Set up the display for the new mode */
		desCompMode = newCompMode;
		switch (desCompMode)
		{
		case IDES_NOCOMPONENT:
			/* Nothing to display */
			break;
		case IDES_SYSTEM:
			intAddComponentForm(
				intNumAvailable(apCompLists[selectedPlayer][COMP_SENSOR], numSensorStats,
								(COMPONENT_STATS *)asSensorStats, sizeof(SENSOR_STATS)) +
				intNumAvailable(apCompLists[selectedPlayer][COMP_ECM], numECMStats,
								(COMPONENT_STATS *)asECMStats, sizeof(ECM_STATS)) +
				intNumAvailable(apCompLists[selectedPlayer][COMP_BRAIN], numBrainStats,
								(COMPONENT_STATS *)asBrainStats, sizeof(BRAIN_STATS)) +
				intNumAvailable(apCompLists[selectedPlayer][COMP_CONSTRUCT], numConstructStats,
								(COMPONENT_STATS *)asConstructStats, sizeof(CONSTRUCT_STATS)) +
				intNumAvailable(apCompLists[selectedPlayer][COMP_REPAIRUNIT], numRepairStats,
								(COMPONENT_STATS *)asRepairStats, sizeof(REPAIR_STATS)));
			intAddExtraSystemButtons(sCurrDesign.asParts[COMP_SENSOR],
									 sCurrDesign.asParts[COMP_ECM],
									 sCurrDesign.asParts[COMP_CONSTRUCT],
									 sCurrDesign.asParts[COMP_REPAIRUNIT],
									 sCurrDesign.asParts[COMP_BRAIN]);
			intAddSystemButtons(IDES_SYSTEM);
			widgSetButtonState(psWScreen, IDDES_SYSTEMFORM, WBUT_LOCK);
			widgSetButtonState(psWScreen, IDDES_SYSTEMBUTTON, WBUT_CLICKLOCK);
			widgReveal(psWScreen, IDDES_SYSTEMFORM);
			break;
		case IDES_TURRET:
			intAddComponentForm(
				intNumAvailable(apCompLists[selectedPlayer][COMP_WEAPON], numWeaponStats,
								(COMPONENT_STATS *)asWeaponStats, sizeof(WEAPON_STATS)));
			weaponIndex = (sCurrDesign.numWeaps > 0) ? sCurrDesign.asWeaps[0] : 0;
			intAddComponentButtons((COMPONENT_STATS *)asWeaponStats,
								   sizeof(WEAPON_STATS),
								   apCompLists[selectedPlayer][COMP_WEAPON],
								   numWeaponStats, weaponIndex,TAB_USEMAJOR);
			intAddSystemButtons(IDES_TURRET);
			widgSetButtonState(psWScreen, IDDES_SYSTEMFORM, WBUT_LOCK);
			widgSetButtonState(psWScreen, IDDES_SYSTEMBUTTON, WBUT_CLICKLOCK);
			widgReveal(psWScreen, IDDES_SYSTEMFORM);
			intSetSystemForm((COMPONENT_STATS *)(asWeaponStats + sCurrDesign.asWeaps[0])); // in case previous was a different slot
			break;
		case IDES_BODY:
			intAddComponentForm(
				intNumAvailable(apCompLists[selectedPlayer][COMP_BODY], numBodyStats,
								(COMPONENT_STATS *)asBodyStats, sizeof(BODY_STATS)));
			intAddComponentButtons((COMPONENT_STATS *)asBodyStats,
								   sizeof(BODY_STATS),
								   apCompLists[selectedPlayer][COMP_BODY],
								   numBodyStats, sCurrDesign.asParts[COMP_BODY],TAB_USEMAJOR);
			widgSetButtonState(psWScreen, IDDES_BODYFORM, WBUT_LOCK);
			widgSetButtonState(psWScreen, IDDES_BODYBUTTON, WBUT_CLICKLOCK);
			widgReveal(psWScreen, IDDES_BODYFORM);
			break;
		case IDES_PROPULSION:
			intAddComponentForm(
				intNumAvailable(apCompLists[selectedPlayer][COMP_PROPULSION], numPropulsionStats,
								(COMPONENT_STATS *)asPropulsionStats, sizeof(PROPULSION_STATS)));
			intAddComponentButtons((COMPONENT_STATS *)asPropulsionStats,
								   sizeof(PROPULSION_STATS),
								   apCompLists[selectedPlayer][COMP_PROPULSION],
								   //NumComponents, sCurrDesign.asParts[COMP_PROPULSION],TAB_USEMAJOR);
								   numPropulsionStats, sCurrDesign.asParts[COMP_PROPULSION],
								   TAB_USEMAJOR);
			widgSetButtonState(psWScreen, IDDES_PROPFORM, WBUT_LOCK);
			widgSetButtonState(psWScreen, IDDES_PROPBUTTON, WBUT_CLICKLOCK);
			widgReveal(psWScreen, IDDES_PROPFORM);
			break;
		case IDES_TURRET_A:
			intAddComponentForm(
				intNumAvailable(apCompLists[selectedPlayer][COMP_WEAPON], numWeaponStats,
								(COMPONENT_STATS *)asWeaponStats, sizeof(WEAPON_STATS)));
			weaponIndex = (sCurrDesign.numWeaps > 1) ? sCurrDesign.asWeaps[1] : 0;
			intAddComponentButtons((COMPONENT_STATS *)asWeaponStats,
								   sizeof(WEAPON_STATS),
								   apCompLists[selectedPlayer][COMP_WEAPON],
								   numWeaponStats, weaponIndex,TAB_USEMAJOR);
			intAddSystemButtons(IDES_TURRET_A);
			widgSetButtonState(psWScreen, IDDES_SYSTEMFORM, WBUT_LOCK);
			widgSetButtonState(psWScreen, IDDES_WPABUTTON, WBUT_CLICKLOCK);
			widgReveal(psWScreen, IDDES_SYSTEMFORM);
			intSetSystemForm((COMPONENT_STATS *)(asWeaponStats + sCurrDesign.asWeaps[1])); // in case previous was a different slot
			// Stop the button flashing
			intSetButtonFlash( IDDES_WPABUTTON,   false );
			break;
		case IDES_TURRET_B:
			intAddComponentForm(
				intNumAvailable(apCompLists[selectedPlayer][COMP_WEAPON], numWeaponStats,
								(COMPONENT_STATS *)asWeaponStats, sizeof(WEAPON_STATS)));
			weaponIndex = (sCurrDesign.numWeaps > 2) ? sCurrDesign.asWeaps[2] : 0;
			intAddComponentButtons((COMPONENT_STATS *)asWeaponStats,
								   sizeof(WEAPON_STATS),
								   apCompLists[selectedPlayer][COMP_WEAPON],
								   numWeaponStats, weaponIndex,TAB_USEMAJOR);
			intAddSystemButtons(IDES_TURRET_B);
			widgSetButtonState(psWScreen, IDDES_SYSTEMFORM, WBUT_LOCK);
			widgSetButtonState(psWScreen, IDDES_WPBBUTTON, WBUT_CLICKLOCK);
			widgReveal(psWScreen, IDDES_SYSTEMFORM);
			intSetSystemForm((COMPONENT_STATS *)(asWeaponStats + sCurrDesign.asWeaps[2])); // in case previous was a different slot
			// Stop the button flashing
			intSetButtonFlash( IDDES_WPBBUTTON,   false );
			break;
		}
	}
}

static COMPONENT_STATS *
intChooseSystemStats( DROID_TEMPLATE *psTemplate )
{
	COMPONENT_STATS		*psStats = NULL;
	int compIndex;

	// Choose correct system stats
	switch (droidTemplateType(psTemplate))
	{
	case DROID_COMMAND:
		compIndex = psTemplate->asParts[COMP_BRAIN];
		ASSERT_OR_RETURN( NULL, compIndex < numBrainStats, "Invalid range referenced for numBrainStats, %d > %d", compIndex, numBrainStats);
		psStats = (COMPONENT_STATS *)(asBrainStats + compIndex);
		break;
	case DROID_SENSOR:
		compIndex = psTemplate->asParts[COMP_SENSOR];
		ASSERT_OR_RETURN( NULL, compIndex < numSensorStats, "Invalid range referenced for numSensorStats, %d > %d", compIndex, numSensorStats);
		psStats = (COMPONENT_STATS *)(asSensorStats + compIndex);
		break;
	case DROID_ECM:
		compIndex = psTemplate->asParts[COMP_ECM];
		ASSERT_OR_RETURN( NULL, compIndex < numECMStats, "Invalid range referenced for numECMStats, %d > %d", compIndex, numECMStats);
		psStats = (COMPONENT_STATS *)(asECMStats + compIndex);
		break;
	case DROID_CONSTRUCT:
	case DROID_CYBORG_CONSTRUCT:
		compIndex = psTemplate->asParts[COMP_CONSTRUCT];
		ASSERT_OR_RETURN( NULL, compIndex < numConstructStats, "Invalid range referenced for numConstructStats, %d > %d", compIndex, numConstructStats);
		psStats = (COMPONENT_STATS *)(asConstructStats + compIndex);
		break;
	case DROID_REPAIR:
	case DROID_CYBORG_REPAIR:
		compIndex = psTemplate->asParts[COMP_REPAIRUNIT];
		ASSERT_OR_RETURN( NULL, compIndex < numRepairStats, "Invalid range referenced for numRepairStats, %d > %d", compIndex, numRepairStats);
		psStats = (COMPONENT_STATS *)(asRepairStats + compIndex);
		break;
	case DROID_WEAPON:
	case DROID_PERSON:
	case DROID_CYBORG:
	case DROID_CYBORG_SUPER:
	case DROID_DEFAULT:
		compIndex = psTemplate->asWeaps[0];
		ASSERT_OR_RETURN( NULL, compIndex < numWeaponStats, "Invalid range referenced for numWeaponStats, %d > %d", compIndex, numWeaponStats);
		psStats = (COMPONENT_STATS *)(asWeaponStats + compIndex);
		break;
	default:
		debug( LOG_ERROR, "unrecognised droid type" );
		return NULL;
	}

	return psStats;
}

/* set SHOWTEMPLATENAME to 0 to show template components in edit box */
#define SHOWTEMPLATENAME	0

const char *GetDefaultTemplateName(DROID_TEMPLATE *psTemplate)
{
	// NOTE:	At this time, savegames can support a max of 60. We are using WIDG_MAXSTR (currently 80 )for display
	// We are also returning a truncated string, instead of NULL if the string is too long.
	COMPONENT_STATS *psStats = NULL;
	int compIndex;

	/*
		First we check for the special cases of the Transporter & Cyborgs
	*/
	if(psTemplate->droidType == DROID_TRANSPORTER)
	{
		return _("Transport");
	}

	/*
		Now get the normal default droid name based on its components
	*/
	aCurrName[0] = '\0'; // Reset string to null
	psStats = intChooseSystemStats( psTemplate );
	if ( psTemplate->asWeaps[0]					!= 0 ||
		 psTemplate->asParts[COMP_CONSTRUCT]	!= 0 ||
		 psTemplate->asParts[COMP_SENSOR]		!= 0 ||
		 psTemplate->asParts[COMP_ECM]			!= 0 ||
		 psTemplate->asParts[COMP_REPAIRUNIT]   != 0 ||
		 psTemplate->asParts[COMP_BRAIN]		!= 0    )
	{
		const char * pStr = getStatName( psStats );
		sstrcpy(aCurrName, pStr);
		sstrcat(aCurrName, " ");
	}

	if ( psTemplate->numWeaps > 1 )
	{
		sstrcat(aCurrName, _("Hydra "));
	}

	compIndex = psTemplate->asParts[COMP_BODY];
	ASSERT_OR_RETURN( NULL, compIndex < numBodyStats, "Invalid range referenced for numBodyStats, %d > %d", compIndex, numBodyStats);
	psStats = (COMPONENT_STATS *) (asBodyStats + compIndex);
	if ( psTemplate->asParts[COMP_BODY] != 0 )
	{
		const char * pStr = getStatName( psStats );

		if ( strlen( aCurrName ) + strlen( pStr ) > WIDG_MAXSTR )
		{
			debug(LOG_ERROR, "Name string too long %s+%s > %u", aCurrName, pStr, WIDG_MAXSTR);
			debug(LOG_ERROR, "Please report what language you are using in the bug report!");
		}

		sstrcat(aCurrName, pStr);
		sstrcat(aCurrName, " ");
	}

	compIndex = psTemplate->asParts[COMP_PROPULSION];
	ASSERT_OR_RETURN( NULL, compIndex < numPropulsionStats, "Invalid range referenced for numPropulsionStats, %d > %d", compIndex, numPropulsionStats);
	psStats = (COMPONENT_STATS *) (asPropulsionStats + compIndex);
	if ( psTemplate->asParts[COMP_PROPULSION] != 0 )
	{
		const char * pStr = getStatName( psStats );

		if ( strlen( aCurrName ) + strlen( pStr ) > WIDG_MAXSTR )
		{
			debug(LOG_ERROR, "Name string too long %s+%s", aCurrName, pStr);
			debug(LOG_ERROR, "Please report what language you are using in the bug report!");
		}

		sstrcat(aCurrName, pStr);
	}

	return aCurrName;
}


static void intSetEditBoxTextFromTemplate( DROID_TEMPLATE *psTemplate )
{
#if SHOWTEMPLATENAME
	widgSetString(psWScreen, IDDES_NAMEBOX, getStatName(psTemplate));
#else

	sstrcpy(aCurrName, "");

	/* show component names if default template else show stat name */
	if ( psTemplate->droidType != DROID_DEFAULT )
	{
		sstrcpy(aCurrName, getTemplateName(psTemplate));
	}
	else
	{
		sstrcpy(aCurrName, GetDefaultTemplateName(psTemplate));
	}

	widgSetString(psWScreen, IDDES_NAMEBOX, aCurrName);
#endif
}

/* Set all the design bar graphs from a design template */
static void intSetDesignStats( DROID_TEMPLATE *psTemplate )
{
	COMPONENT_STATS		*psStats = intChooseSystemStats( psTemplate );

	/* Set system stats */
	intSetSystemForm( psStats );

	/* Set the body stats */
	intSetBodyStats(asBodyStats + psTemplate->asParts[COMP_BODY]);

	/* Set the propulsion stats */
	intSetPropulsionForm(asPropulsionStats + psTemplate->asParts[COMP_PROPULSION]);

	/* Set the name in the edit box */
	intSetEditBoxTextFromTemplate( psTemplate );
}

/* Set up the system clickable form of the design screen given a set of stats */
static BOOL _intSetSystemForm(COMPONENT_STATS *psStats)
{
	W_FORMINIT		sFormInit;
	W_BARINIT		sBarInit;
	W_LABINIT		sLabInit;
	DES_SYSMODE		newSysMode=(DES_SYSMODE)0;

	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	memset(&sLabInit, 0, sizeof(W_LABINIT));
	memset(&sBarInit, 0, sizeof(W_BARINIT));

	/* Figure out what the new mode should be */
	switch (statType(psStats->ref))
	{
	case COMP_WEAPON:
		newSysMode = IDES_WEAPON;
		break;
	case COMP_SENSOR:
		newSysMode = IDES_SENSOR;
		break;
	case COMP_ECM:
		newSysMode = IDES_ECM;
		break;
	case COMP_CONSTRUCT:
		newSysMode = IDES_CONSTRUCT;
		break;
	case COMP_BRAIN:
		newSysMode = IDES_COMMAND;
		break;
	case COMP_REPAIRUNIT:
		newSysMode = IDES_REPAIR;
		break;
	}

	/* If the correct form is already displayed just set the stats */
	if (newSysMode == desSysMode)
	{
		intSetSystemStats(psStats);

		return true;
	}

	/* Remove the old form if necessary */
	if (desSysMode != IDES_NOSYSTEM)
	{
		widgDelete(psWScreen, IDDES_SYSTEMFORM);
	}

	/* Set the new mode */
	desSysMode = newSysMode;

	/* Add the system form */
	sFormInit.formID = IDDES_STATSFORM;
	sFormInit.id = IDDES_SYSTEMFORM;
	sFormInit.style = (WFORM_CLICKABLE | WFORM_NOCLICKMOVE);
	sFormInit.x = DES_BARFORMX;
	sFormInit.y = DES_BARFORMY;
	sFormInit.width = DES_BARFORMWIDTH;	//COMPBUTWIDTH;
	sFormInit.height = DES_BARFORMHEIGHT;	//COMPBUTHEIGHT;
	sFormInit.pTip = getStatName(psStats);	/* set form tip to stats string */
	sFormInit.pUserData = psStats;			/* store component stats */
	sFormInit.pDisplay = intDisplayStatForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Initialise the bargraph struct */
	sBarInit.formID = IDDES_SYSTEMFORM;
	sBarInit.style = WBAR_PLAIN;//WBAR_DOUBLE;
	sBarInit.orientation = WBAR_LEFT;
	sBarInit.x = DES_CLICKBARX;
	sBarInit.y = DES_STATBAR_Y1;	//DES_CLICKBARY;
	sBarInit.width = DES_CLICKBARWIDTH;
	sBarInit.height = DES_CLICKBARHEIGHT;
	sBarInit.sCol.byte.r = DES_CLICKBARMAJORRED;
	sBarInit.sCol.byte.g = DES_CLICKBARMAJORGREEN;
	sBarInit.sCol.byte.b = DES_CLICKBARMAJORBLUE;
	sBarInit.sMinorCol.byte.r = DES_CLICKBARMINORRED;
	sBarInit.sMinorCol.byte.g = DES_CLICKBARMINORGREEN;
	sBarInit.sMinorCol.byte.b = DES_CLICKBARMINORBLUE;
	sBarInit.pDisplay = intDisplayStatsBar;

	/* Initialise the label struct */
	sLabInit.formID = IDDES_SYSTEMFORM;
	sLabInit.style = WLAB_PLAIN;
	sLabInit.x = DES_CLICKBARNAMEX;
	sLabInit.y = DES_CLICKBARY - DES_CLICKBARHEIGHT/3;
	sLabInit.width = DES_CLICKBARNAMEWIDTH;
	sLabInit.height = DES_CLICKBARHEIGHT;
	sLabInit.FontID = font_regular;

	/* See what type of system stats we've got */
	if (psStats->ref >= REF_SENSOR_START &&
		psStats->ref < REF_SENSOR_START + REF_RANGE)
	{
		/* Add the bar graphs*/
		sBarInit.id = IDDES_SENSORRANGE;
		sBarInit.iRange = (UWORD)getMaxSensorRange();//DBAR_SENSORMAXRANGE;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_SENSORPOWER;
		sBarInit.y = DES_STATBAR_Y2;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.iRange = (UWORD)getMaxSensorPower();//DBAR_SENSORMAXPOWER;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_SENSORWEIGHT;
		sBarInit.y = DES_STATBAR_Y3;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.iRange = (UWORD)getMaxComponentWeight();//DBAR_MAXWEIGHT;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}

		/* Add the labels */
		sLabInit.id = IDDES_SENSORRANGELAB;
		sLabInit.pTip = _("Sensor Range");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_RANGE;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_SENSORPOWERLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Sensor Power");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_POWER;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_SENSORWEIGHTLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Weight");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_WEIGHT;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
	}
	else if (psStats->ref >= REF_ECM_START &&
			 psStats->ref < REF_ECM_START + REF_RANGE)
	{
		/* Add the bar graphs */
		sBarInit.id = IDDES_ECMPOWER;
		sBarInit.iRange = (UWORD)getMaxECMPower();//DBAR_ECMMAXPOWER;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_ECMWEIGHT;
		sBarInit.y = DES_STATBAR_Y2;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.iRange = (UWORD)getMaxComponentWeight();//DBAR_MAXWEIGHT;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}

		/* Add the labels */
		sLabInit.id = IDDES_ECMPOWERLAB;
		sLabInit.pTip = _("ECM Power");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_POWER;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_ECMWEIGHTLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Weight");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_WEIGHT;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
	}
	else if (psStats->ref >= REF_CONSTRUCT_START &&
			 psStats->ref < REF_CONSTRUCT_START + REF_RANGE)
	{
		/* Add the bar graphs */
		sBarInit.id = IDDES_CONSTPOINTS;
		sBarInit.pTip = _("Build Points");
		sBarInit.iRange = (UWORD)getMaxConstPoints();//DBAR_CONSTMAXPOINTS;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_CONSTWEIGHT;
		sBarInit.y = DES_STATBAR_Y2;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.pTip = _("Weight");
		sBarInit.iRange = (UWORD)getMaxComponentWeight();//DBAR_MAXWEIGHT;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}

		/* Add the labels */
		sLabInit.id = IDDES_CONSTPOINTSLAB;
		sLabInit.pTip = _("Build Points");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_BUILDRATE;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_CONSTWEIGHTLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Weight");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_WEIGHT;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
	}
	else if (psStats->ref >= REF_REPAIR_START &&
			 psStats->ref < REF_REPAIR_START + REF_RANGE)
	{
		/* Add the bar graphs */
		sBarInit.id = IDDES_REPAIRPOINTS;
		sBarInit.pTip = _("Build Points");
		sBarInit.iRange = (UWORD)getMaxRepairPoints();//DBAR_REPAIRMAXPOINTS;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_REPAIRWEIGHT;
		sBarInit.y = DES_STATBAR_Y2;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.pTip = _("Weight");
		sBarInit.iRange = (UWORD)getMaxComponentWeight();//DBAR_MAXWEIGHT;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}

		/* Add the labels */
		sLabInit.id = IDDES_REPAIRPTLAB;
		sLabInit.pTip = _("Build Points");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_BUILDRATE;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_REPAIRWGTLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Weight");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_WEIGHT;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
	}
	else if (psStats->ref >= REF_WEAPON_START &&
			 psStats->ref < REF_WEAPON_START + REF_RANGE)
	{
		/* Add the bar graphs */
		sBarInit.id = IDDES_WEAPRANGE;
		sBarInit.iRange = (UWORD)getMaxWeaponRange();//DBAR_WEAPMAXRANGE;
		sBarInit.pTip = _("Range");
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_WEAPDAMAGE;
		sBarInit.y = DES_STATBAR_Y2;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.iRange = (UWORD)getMaxWeaponDamage();//DBAR_WEAPMAXDAMAGE;
		sBarInit.pTip = _("Damage");
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_WEAPROF;
		sBarInit.y = DES_STATBAR_Y3;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.iRange = getMaxWeaponROF();
		sBarInit.pTip = _("Rate-of-Fire");
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_WEAPWEIGHT;
		sBarInit.y = DES_STATBAR_Y4;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.iRange = (UWORD)getMaxComponentWeight();//DBAR_MAXWEIGHT;
		sBarInit.pTip = _("Weight");
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}

		/* Add the labels */
		sLabInit.id = IDDES_WEAPRANGELAB;
		sLabInit.pTip = _("Range");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_RANGE;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_WEAPDAMAGELAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Damage");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_DAMAGE;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_WEAPROFLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Rate-of-Fire");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_FIRERATE;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_WEAPWEIGHTLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Weight");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_WEIGHT;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
	}

	// Add the correct component form
	switch (desSysMode)
	{
	case IDES_SENSOR:
	case IDES_CONSTRUCT:
	case IDES_ECM:
	case IDES_REPAIR:
	case IDES_COMMAND:
		intSetDesignMode(IDES_SYSTEM);
		break;
	case IDES_WEAPON:
		intSetDesignMode(IDES_TURRET);
		break;
	default:
		break;
	}

	/* Set the stats */
	intSetSystemStats(psStats);

	/* Lock the form down if necessary */
	if ( desCompMode == IDES_SYSTEM )
	{
		widgSetButtonState(psWScreen, IDDES_SYSTEMFORM, WBUT_LOCK);
	}

	return true;
}


/* Set up the propulsion clickable form of the design screen given a set of stats */
static BOOL intSetPropulsionForm(PROPULSION_STATS *psStats)
{
	W_FORMINIT		sFormInit;
	W_BARINIT		sBarInit;
	W_LABINIT		sLabInit;
	DES_PROPMODE	newPropMode=(DES_PROPMODE)0;

	ASSERT_OR_RETURN(false, psStats != NULL, "Invalid propulsion stats pointer");

	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	memset(&sLabInit, 0, sizeof(W_LABINIT));
	memset(&sBarInit, 0, sizeof(W_BARINIT));

	/* figure out what the new mode should be */
	switch (asPropulsionTypes[psStats->propulsionType].travel)
	{
	case GROUND:
		newPropMode = IDES_GROUND;
		break;
	case AIR:
		newPropMode = IDES_AIR;
		break;
	}

	/* If the mode hasn't changed, just set the stats */
	if (desPropMode == newPropMode)
	{
		intSetPropulsionStats(psStats);
		return true;
	}

	/* Remove the old form if necessary */
	if (desPropMode != IDES_NOPROPULSION)
	{
		widgDelete(psWScreen, IDDES_PROPFORM);
	}

	/* Set the new mode */
	desPropMode = newPropMode;

	/* Add the propulsion form */
	sFormInit.formID = IDDES_STATSFORM;
	sFormInit.id = IDDES_PROPFORM;
	sFormInit.style = WFORM_CLICKABLE | WFORM_NOCLICKMOVE;
	sFormInit.x = DES_BARFORMX;
	sFormInit.y = DES_BARFORMY;
	sFormInit.width = DES_BARFORMWIDTH;	//DES_COMPBUTWIDTH;
	sFormInit.height = DES_BARFORMHEIGHT;	//DES_COMPBUTHEIGHT;
	sFormInit.pTip = getStatName(psStats);	/* set form tip to stats string */
	sFormInit.pDisplay = intDisplayStatForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	/* Initialise the bargraph struct */
	sBarInit.formID = IDDES_PROPFORM;
	sBarInit.style = WBAR_PLAIN;//WBAR_DOUBLE;
	sBarInit.orientation = WBAR_LEFT;
	sBarInit.x = DES_CLICKBARX;
	sBarInit.y = DES_STATBAR_Y1;	//DES_CLICKBARY;
	sBarInit.width = DES_CLICKBARWIDTH;
	sBarInit.height = DES_CLICKBARHEIGHT;
	sBarInit.sCol.byte.r = DES_CLICKBARMAJORRED;
	sBarInit.sCol.byte.g = DES_CLICKBARMAJORGREEN;
	sBarInit.sCol.byte.b = DES_CLICKBARMAJORBLUE;
	sBarInit.sMinorCol.byte.r = DES_CLICKBARMINORRED;
	sBarInit.sMinorCol.byte.g = DES_CLICKBARMINORGREEN;
	sBarInit.sMinorCol.byte.b = DES_CLICKBARMINORBLUE;
	sBarInit.pDisplay = intDisplayStatsBar;

	/* Initialise the label struct */
	sLabInit.formID = IDDES_PROPFORM;
	sLabInit.style = WLAB_PLAIN;
	sLabInit.x = DES_CLICKBARNAMEX;
	sLabInit.y = DES_CLICKBARY - DES_CLICKBARHEIGHT/3;
	sLabInit.width = DES_CLICKBARNAMEWIDTH;
	sLabInit.height = DES_CLICKBARNAMEHEIGHT;	//DES_CLICKBARHEIGHT;
	sLabInit.FontID = font_regular;

	/* See what type of propulsion we've got */
	switch (desPropMode)
	{
	case IDES_AIR:
		/* Add the bar graphs */
		sBarInit.id = IDDES_PROPAIR;
		sBarInit.iRange = (UWORD)getMaxPropulsionSpeed();//DBAR_PROPMAXSPEED;
		sBarInit.pTip = _("Air Speed");
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_PROPWEIGHT;
		sBarInit.y = DES_STATBAR_Y2;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.iRange = (UWORD)getMaxComponentWeight();//DBAR_MAXWEIGHT;
		sBarInit.pTip = _("Weight");
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}

		/* Add the labels */
		sLabInit.id = IDDES_PROPAIRLAB;
		sLabInit.pTip = _("Air Speed");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_HOVER;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_PROPWEIGHTLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Weight");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_WEIGHT;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		break;
	case IDES_GROUND:
		/* Add the bar graphs */
		sBarInit.id = IDDES_PROPROAD;
		sBarInit.pTip = _("Road Speed");
		sBarInit.iRange = (UWORD)getMaxPropulsionSpeed();//DBAR_PROPMAXSPEED;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_PROPCOUNTRY;
		sBarInit.y = DES_STATBAR_Y2;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.pTip = _("Off-Road Speed");
		sBarInit.iRange = (UWORD)getMaxPropulsionSpeed();//DBAR_PROPMAXSPEED;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_PROPWATER;
		sBarInit.y = DES_STATBAR_Y3;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.pTip = _("Water Speed");
		sBarInit.iRange = (UWORD)getMaxPropulsionSpeed();//DBAR_PROPMAXSPEED;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}
		sBarInit.id = IDDES_PROPWEIGHT;
		sBarInit.y = DES_STATBAR_Y4;	//+= DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sBarInit.pTip = _("Weight");
		sBarInit.iRange = (UWORD)getMaxComponentWeight();//DBAR_MAXWEIGHT;
		if (!widgAddBarGraph(psWScreen, &sBarInit))
		{
			return false;
		}

		/* Add the labels */
		sLabInit.id = IDDES_PROPROADLAB;
		sLabInit.pTip = _("Road Speed");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_ROAD;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_PROPCOUNTRYLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Off-Road Speed");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_CROSSCOUNTRY;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_PROPWATERLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Water Speed");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_HOVER;	//WATER;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		sLabInit.id = IDDES_PROPWEIGHTLAB;
		sLabInit.y += DES_CLICKBARHEIGHT + DES_CLICKGAP;
		sLabInit.pTip = _("Weight");
		sLabInit.pDisplay = intDisplayImage;
		sLabInit.UserData = IMAGE_DES_WEIGHT;
		if (!widgAddLabel(psWScreen, &sLabInit))
		{
			return false;
		}
		break;
	default:
		break;
	}

	/* Set the stats */
	intSetPropulsionStats(psStats);

	/* Lock the form down if necessary */
	if (desCompMode == IDES_PROPULSION)
	{
		widgSetButtonState(psWScreen, IDDES_PROPFORM, WBUT_LOCK);
	}

	return true;
}


// count the number of available components
static UDWORD intNumAvailable(UBYTE *aAvailable, UDWORD numEntries,
							  COMPONENT_STATS *asStats, UDWORD size)
{
	UDWORD				numButtons, i;
	COMPONENT_STATS		*psCurrStats;

	numButtons = 0;
	psCurrStats = asStats;
	for(i=0; i < numEntries; i++)
	{
		if (psCurrStats->designable
                 && aAvailable[i] & AVAILABLE)
		{
			numButtons++;
		}

		psCurrStats = (COMPONENT_STATS *)( (UBYTE *)psCurrStats + size );
	}

	return numButtons;
}


/* Add the component tab form to the design screen */
static BOOL intAddComponentForm(UDWORD numButtons)
{
	W_FORMINIT		sFormInit;
	UDWORD			i, butPerForm, numFrm;

	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	butPerForm = DES_BUTSPERFORM;

	/* add a form to place the tabbed form on */
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = 0;
	sFormInit.id = IDDES_RIGHTBASE;
	sFormInit.style = WFORM_PLAIN;
	sFormInit.x = (SWORD)(RADTLX-2);
	sFormInit.y = (SWORD)DESIGN_Y;
	sFormInit.width = RET_FORMWIDTH;
	sFormInit.height = DES_RIGHTFORMHEIGHT + 4;
	sFormInit.pDisplay = intDisplayPlainForm;
	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	//now a single form
	memset(&sFormInit, 0, sizeof(W_FORMINIT));
	sFormInit.formID = IDDES_RIGHTBASE;
	sFormInit.id = IDDES_COMPFORM;
	sFormInit.style = WFORM_TABBED;
	sFormInit.x = 2;
	sFormInit.y = 40;
	sFormInit.width = DES_RIGHTFORMWIDTH;
	sFormInit.height = DES_RIGHTFORMHEIGHT;
	numFrm = numForms(numButtons, butPerForm);
	sFormInit.numMajor = (UWORD)(numFrm >= WFORM_MAXMAJOR ? WFORM_MAXMAJOR-1 : numFrm);
	sFormInit.majorPos = WFORM_TABTOP;
	sFormInit.minorPos = WFORM_TABNONE;
	sFormInit.majorSize = DES_TAB_WIDTH;
	sFormInit.majorOffset = DES_TAB_LEFTOFFSET;
	sFormInit.tabVertOffset = (DES_TAB_HEIGHT/2);
	sFormInit.tabMajorThickness = DES_TAB_HEIGHT;
	sFormInit.pUserData = &StandardTab;
	sFormInit.pTabDisplay = intDisplayTab;
	if (sFormInit.numMajor > MAX_TAB_STD_SHOWN)
	{	// StandardTab can't have more than 4 tabs.  Being extra safe here, since
		// we do NOT use scrolltabs & not smallTab icons either (which allow max 8)
		sFormInit.numMajor = MAX_TAB_STD_SHOWN;
	}
	for (i=0; i< sFormInit.numMajor; i++)
	{
		sFormInit.aNumMinors[i] = 1;
	}

	if (!widgAddForm(psWScreen, &sFormInit))
	{
		return false;
	}

	return true;
}

/* Add the system buttons (weapons, command droid, etc) to the design screen */
static BOOL intAddSystemButtons(DES_COMPMODE mode)
{
	W_BUTINIT	sButInit;

	memset(&sButInit, 0, sizeof(W_BUTINIT));

	// add the weapon button
	sButInit.formID = IDDES_RIGHTBASE;
	sButInit.id = IDDES_WEAPONS;
	sButInit.style = WBUT_PLAIN;
	sButInit.x = DES_WEAPONBUTTON_X;
	sButInit.y = DES_SYSTEMBUTTON_Y;
	sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_WEAPONS);
	sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_WEAPONS);
	sButInit.pTip = _("Weapons");
	sButInit.FontID = font_regular;
	sButInit.pDisplay = intDisplayButtonHilight;
	sButInit.UserData = PACKDWORD_TRI(0,IMAGE_DES_EXTRAHI , IMAGE_DES_WEAPONS);
	if (!widgAddButton(psWScreen, &sButInit))
	{
		return false;
	}

    //if currently got a VTOL proplusion attached then don't add the system buttons
	//dont add the system button if mode is IDES_TURRET_A or IDES_TURRET_B
    if (!checkTemplateIsVtol(&sCurrDesign) &&
	    mode != IDES_TURRET_A && mode != IDES_TURRET_B)
    {
	    // add the system button
	    sButInit.formID = IDDES_RIGHTBASE;
	    sButInit.id = IDDES_SYSTEMS;
	    sButInit.style = WBUT_PLAIN;
	    sButInit.x = DES_SYSTEMBUTTON_X;
	    sButInit.y = DES_SYSTEMBUTTON_Y;
	    sButInit.width = iV_GetImageWidth(IntImages, IMAGE_DES_SYSTEMS);
	    sButInit.height = iV_GetImageHeight(IntImages, IMAGE_DES_SYSTEMS);
	    sButInit.pTip = _("Systems");
	    sButInit.FontID = font_regular;
	    sButInit.pDisplay = intDisplayButtonHilight;
	    sButInit.UserData = PACKDWORD_TRI(0,IMAGE_DES_EXTRAHI , IMAGE_DES_SYSTEMS);
	    if (!widgAddButton(psWScreen, &sButInit))
	    {
		    return false;
	    }
	    if (mode == IDES_SYSTEM)
	    {
		widgSetButtonState(psWScreen, IDDES_SYSTEMS, WBUT_LOCK);
	    }
    }

	// lock down the correct button
	switch (mode)
	{
		case IDES_TURRET:
		case IDES_TURRET_A:
		case IDES_TURRET_B:
			widgSetButtonState(psWScreen, IDDES_WEAPONS, WBUT_LOCK);
			break;
		case IDES_SYSTEM:
			break;
		default:
			ASSERT(!"invalid/unexpected mode", "unexpected mode");
			break;
	}

	return true;
}


/* Add the component buttons to the main tab of the component form */
static BOOL intAddComponentButtons(COMPONENT_STATS *psStats, UDWORD size,
								   UBYTE *aAvailable,	UDWORD numEntries,
								   UDWORD compID,UDWORD WhichTab)
{
	W_FORMINIT			sButInit;
	W_TABFORM           *psTabForm;
	UDWORD				i, maxComponents;
	COMPONENT_STATS		*psCurrStats;
	char				aButText[DES_COMPBUTMAXCHAR + 1];
	SDWORD				BufferID;
	PROPULSION_STATS	*psPropStats;
	BOOL				bVTol, bWeapon, bVtolWeapon;
	UWORD               numTabs;

	ClearObjectBuffers();

	memset(aButText, 0, DES_COMPBUTMAXCHAR + 1);
	memset(&sButInit, 0, sizeof(W_BUTINIT));

	/* Set up the button struct */
	sButInit.formID = IDDES_COMPFORM;
	sButInit.majorID = IDES_MAINTAB;
	sButInit.minorID = 0;
	sButInit.id = IDDES_COMPSTART;
	sButInit.style = WFORM_CLICKABLE;
	sButInit.x = DES_RIGHTFORMBUTX;
	sButInit.y = DES_RIGHTFORMBUTY;
	sButInit.width = DES_TABBUTWIDTH;
	sButInit.height = DES_TABBUTHEIGHT;

	//need to set max number of buttons possible
	if (psStats->ref >=REF_WEAPON_START && psStats->ref < REF_WEAPON_START +
		REF_RANGE)
	{
		maxComponents = MAX_SYSTEM_COMPONENTS;
	}
	else
	{
		maxComponents = MAX_DESIGN_COMPONENTS;
	}

	/*if adding weapons - need to check if the propulsion is a VTOL*/
	bVTol = false;

	if ( (psStats->ref >= REF_WEAPON_START) &&
		 (psStats->ref < REF_WEAPON_START + REF_RANGE) )
	{
		bWeapon = true;
	}
	else
	{
		bWeapon = false;
	}

	if ( bWeapon )
	{
		//check if the current Template propulsion has been set
		if (sCurrDesign.asParts[COMP_PROPULSION])
		{
			psPropStats = asPropulsionStats + sCurrDesign.
				asParts[COMP_PROPULSION];
			ASSERT_OR_RETURN(false, psPropStats != NULL, "invalid propulsion stats pointer");

			if (asPropulsionTypes[psPropStats->propulsionType].travel == AIR)
			{
				bVTol = true;
			}
		}
	}

	/* Add each button */
	desCompID = 0;
	numComponent = 0;
	psCurrStats = psStats;
	for (i=0; i<numEntries; i++)
	{
		/* If we are out of space in the list - stop */
		if (numComponent >= maxComponents)
		{
			//ASSERT( false,
			//	"intAddComponentButtons: Too many components for the list" );
			break;
		}

		/* Skip unavailable entries and non-design ones*/
		if (!(aAvailable[i] & AVAILABLE)
				 || !psCurrStats->designable)
		{
			/* Update the stats pointer for the next button */
			psCurrStats = (COMPONENT_STATS *)(((UBYTE *)psCurrStats) + size);

			continue;
		}

		/*skip indirect weapons if VTOL propulsion or numVTOLattackRuns for the weapon is zero*/
		if ( bWeapon )
		{
			if ( ((WEAPON_STATS *)psCurrStats)->vtolAttackRuns )
			{
				bVtolWeapon = true;
			}
			else
			{
				bVtolWeapon = false;
			}

			if ( (bVTol && !bVtolWeapon) || (!bVTol && bVtolWeapon) )
			{
				/* Update the stats pointer for the next button */
				psCurrStats = (COMPONENT_STATS *)(((UBYTE *)psCurrStats) + size);
				continue;
			}
		}

		/* Set the tip and add the button */
		sstrcpy(aButText, getStatName(psCurrStats));
		sButInit.pTip = getStatName(psCurrStats);

		BufferID = GetObjectBuffer();
		ASSERT_OR_RETURN(false, BufferID >= 0,"Unable to acquire Topic buffer." );

		RENDERBUTTON_INUSE(&ObjectBuffers[BufferID]);
		ObjectBuffers[BufferID].Data = psCurrStats;
		sButInit.pUserData = &ObjectBuffers[BufferID];
		sButInit.pDisplay = intDisplayComponentButton;

		if (!widgAddForm(psWScreen, &sButInit))
		{
			return false;
		}

		/* Store the stat pointer in the list */
		apsComponentList[numComponent++] = psCurrStats;

		/* If this matches the component ID lock the button */
		if (i == compID)
		{
			desCompID = sButInit.id;
			widgSetButtonState(psWScreen, sButInit.id, WBUT_LOCK);
			widgSetTabs(psWScreen, IDDES_COMPFORM, sButInit.majorID, sButInit.minorID);
		}

		// if this is a command droid that is in use or dead - make it unavailable
		if (statType(psCurrStats->ref) == COMP_BRAIN)
		{
			if ( ( ((COMMAND_DROID *)psCurrStats)->psDroid != NULL ) ||
				 ((COMMAND_DROID *)psCurrStats)->died )
			{
				widgSetButtonState(psWScreen, sButInit.id, WBUT_DISABLE);
			}
		}

		if(WhichTab == TAB_USEMAJOR) {
			/* Update the init struct for the next button */
			sButInit.id += 1;
			sButInit.x += DES_TABBUTWIDTH + DES_TABBUTGAP;
			if (sButInit.x + DES_TABBUTWIDTH+DES_TABBUTGAP > DES_RIGHTFORMWIDTH - DES_TABTHICKNESS)
			{
				sButInit.x = DES_RIGHTFORMBUTX;
				sButInit.y += DES_TABBUTHEIGHT + DES_TABBUTGAP;
			}
			if (sButInit.y + DES_TABBUTHEIGHT+DES_TABBUTGAP > DES_RIGHTFORMHEIGHT - DES_MAJORSIZE)
			{
				sButInit.y = DES_RIGHTFORMBUTY;
				sButInit.majorID += 1;
				if (sButInit.majorID >= WFORM_MAXMAJOR)
				{
					debug( LOG_NEVER, "Too many buttons for component form" );
					return false;
				}
			}
		} else {
			/* Update the init struct for the next button */
			sButInit.id += 1;
			sButInit.x += DES_TABBUTWIDTH + DES_TABBUTGAP;
			if (sButInit.x + DES_TABBUTWIDTH+DES_TABBUTGAP > DES_RIGHTFORMWIDTH - DES_MINORSIZE)
			{
				sButInit.x = DES_RIGHTFORMBUTX;
				sButInit.y += DES_TABBUTHEIGHT + DES_TABBUTGAP;
			}
			if (sButInit.y + DES_TABBUTHEIGHT+DES_TABBUTGAP > DES_RIGHTFORMHEIGHT - DES_MAJORSIZE)
			{
				sButInit.y = DES_RIGHTFORMBUTY;
				sButInit.minorID += 1;
				if (sButInit.minorID >= WFORM_MAXMINOR)
				{
					debug( LOG_NEVER, "Too many buttons for component form" );
					return false;
				}
			}
		}

		/* Update the stats pointer for the next button */
		psCurrStats = (COMPONENT_STATS *)(((UBYTE *)psCurrStats) + size);
	}

    //hack to sort out the tabs on the weapon form
    //need to check how many buttons have been added to see if need all the tabs that are there
    psTabForm = (W_TABFORM *) widgGetFromID(psWScreen,IDDES_COMPFORM);
    if (psTabForm)
    {
        numTabs = psTabForm->numMajor;
        if (numComponent < (UDWORD)(numTabs * DES_BUTSPERFORM))
        {
            psTabForm->numMajor = numForms(numComponent, DES_BUTSPERFORM);
        }
    }

	return true;
}

/* Add the component buttons to the main tab of the component form */
static BOOL intAddExtraSystemButtons(UDWORD sensorIndex, UDWORD ecmIndex,
									 UDWORD constIndex, UDWORD repairIndex,
									 UDWORD brainIndex)
{
	W_FORMINIT		sButInit;
	UDWORD			i, buttonType, size=0;
	UDWORD			compIndex=0, numStats=0;
	COMPONENT_STATS	*psCurrStats=0;
	UBYTE			*aAvailable=0;
	char			aButText[DES_COMPBUTMAXCHAR + 1];
	SDWORD			BufferID;

	memset(aButText, 0, DES_COMPBUTMAXCHAR + 1);
	memset(&sButInit, 0, sizeof(W_BUTINIT));

	// Set up the button struct
	sButInit.formID = IDDES_COMPFORM;
	sButInit.majorID = 0;
	sButInit.minorID = 0;
	sButInit.id = IDDES_EXTRASYSSTART;
	sButInit.style = WFORM_CLICKABLE;
	sButInit.x = DES_RIGHTFORMBUTX;
	sButInit.y = DES_RIGHTFORMBUTY;
	sButInit.width = DES_TABBUTWIDTH;
	sButInit.height = DES_TABBUTHEIGHT;

	// Add the buttons :
	// buttonType == 0  -  Sensor Buttons
	// buttonType == 1  -  ECM Buttons
	// buttonType == 2  -  Constructor Buttons
	// buttonType == 3  -  Repair Buttons
	// buttonType == 4  -  Brain Buttons
	numExtraSys = 0;
	for(buttonType = 0; buttonType < 5; buttonType++)
	{
		switch (buttonType)
		{
		case 0:
			// Sensor Buttons
			psCurrStats = (COMPONENT_STATS *)asSensorStats;
			size = sizeof(SENSOR_STATS);
			aAvailable = apCompLists[selectedPlayer][COMP_SENSOR];
			numStats = numSensorStats;
			compIndex = sensorIndex;
			break;
		case 1:
			// ECM Buttons
			psCurrStats = (COMPONENT_STATS *)asECMStats;
			size = sizeof(ECM_STATS);
			aAvailable = apCompLists[selectedPlayer][COMP_ECM];
			numStats = numECMStats;
			compIndex = ecmIndex;
			break;
		case 2:
			// Constructor Buttons
			psCurrStats = (COMPONENT_STATS *)asConstructStats;
			size = sizeof(CONSTRUCT_STATS);
			aAvailable = apCompLists[selectedPlayer][COMP_CONSTRUCT];
			numStats = numConstructStats;
			compIndex = constIndex;
			break;
		case 3:
			// Repair Buttons
			psCurrStats = (COMPONENT_STATS *)asRepairStats;
			size = sizeof(REPAIR_STATS);
			aAvailable = apCompLists[selectedPlayer][COMP_REPAIRUNIT];
			numStats = numRepairStats;
			compIndex = repairIndex;
			break;
		case 4:
			// Brain Buttons
			psCurrStats = (COMPONENT_STATS *)asBrainStats;
			size = sizeof(BRAIN_STATS);
			aAvailable = apCompLists[selectedPlayer][COMP_BRAIN];
			numStats = numBrainStats;
			compIndex = brainIndex;
			break;
		}
		for (i=0; i<numStats; i++)
		{
			// If we are out of space in the list - stop
			if (numExtraSys >= MAXEXTRASYS)
			{
				ASSERT( false, "Too many components for the list");
				return false;
			}

			// Skip unavailable entries or non-design ones
			if (!(aAvailable[i] & AVAILABLE)
			 || !psCurrStats->designable)
			{
				// Update the stats pointer for the next button
				psCurrStats = (COMPONENT_STATS *)(((UBYTE *)psCurrStats) + size);

				continue;
			}

			// Set the tip and add the button
			sstrcpy(aButText, getStatName(psCurrStats));
			sButInit.pTip = getStatName(psCurrStats);

			BufferID = sButInit.id-IDDES_EXTRASYSSTART;
			ASSERT_OR_RETURN(false, BufferID < NUM_OBJECTBUFFERS, "BufferID > NUM_OBJECTBUFFERS");

			//just use one set of buffers for mixed system form
			RENDERBUTTON_INUSE(&System0Buffers[BufferID]);
			if (statType(psCurrStats->ref) == COMP_BRAIN)
			{
				System0Buffers[BufferID].Data = ((BRAIN_STATS *)psCurrStats)->psWeaponStat;
			}
			else
			{
				System0Buffers[BufferID].Data = psCurrStats;
			}
			sButInit.pUserData = &System0Buffers[BufferID];

			sButInit.pDisplay = intDisplayComponentButton;

			if (!widgAddForm(psWScreen, &sButInit))
			{
				return false;
			}

			// Store the stat pointer in the list
			apsExtraSysList[numExtraSys++] = psCurrStats;

			// If this matches the sensorIndex note the form and button
			if (i == compIndex)
			{
				desCompID = sButInit.id;
				widgSetButtonState(psWScreen, sButInit.id, WBUT_LOCK);
				widgSetTabs(psWScreen, IDDES_COMPFORM,
							sButInit.majorID, sButInit.minorID);
			}

			// Update the init struct for the next button
			sButInit.id += 1;
			sButInit.x += DES_TABBUTWIDTH + DES_TABBUTGAP;
			if (sButInit.x + DES_TABBUTWIDTH+DES_TABBUTGAP > DES_RIGHTFORMWIDTH - DES_MINORSIZE)
			{
				sButInit.x = DES_RIGHTFORMBUTX;
				sButInit.y += DES_TABBUTHEIGHT + DES_TABBUTGAP;
			}
			if (sButInit.y + DES_TABBUTHEIGHT+DES_TABBUTGAP > DES_RIGHTFORMHEIGHT - DES_MAJORSIZE)
			{
				sButInit.y = DES_RIGHTFORMBUTY;
				sButInit.majorID += 1;
			}

			// Update the stats pointer for the next button
			psCurrStats = (COMPONENT_STATS *)(((UBYTE *)psCurrStats) + size);
		}
	}

	return true;
}


/* Set the bar graphs for the system clickable */
static void intSetSystemStats(COMPONENT_STATS *psStats)
{
	W_FORM *psForm;

	ASSERT_OR_RETURN( , psStats != NULL, "Invalid stats pointer");

	/* set form tip to stats string */
	widgSetTip( psWScreen, IDDES_SYSTEMFORM, getStatName(psStats) );

	/* set form stats for later display in intDisplayStatForm */
	psForm = (W_FORM *) widgGetFromID( psWScreen, IDDES_SYSTEMFORM );
	if ( psForm != NULL )
	{
		psForm->pUserData = psStats;
	}

	/* Set the correct system stats */
	switch (statType(psStats->ref))
	{
	case COMP_SENSOR:
		intSetSensorStats((SENSOR_STATS *)psStats);
		break;
	case COMP_ECM:
		intSetECMStats((ECM_STATS *)psStats);
		break;
	case COMP_WEAPON:
		intSetWeaponStats((WEAPON_STATS *)psStats);
		break;
	case COMP_CONSTRUCT:
		intSetConstructStats((CONSTRUCT_STATS *)psStats);
		break;
	case COMP_REPAIRUNIT:
		intSetRepairStats((REPAIR_STATS *)psStats);
		break;
	}
}

/* Set the shadow bar graphs for the system clickable */
static void intSetSystemShadowStats(COMPONENT_STATS *psStats)
{
	/* Set the correct system stats - psStats can be set to NULL if
	 * desSysMode does not match the type of the stats.
	 */
	if (psStats)
	{
		switch (statType(psStats->ref))
		{
		case COMP_SENSOR:
			if (desSysMode == IDES_SENSOR)
			{
				intSetSensorShadowStats((SENSOR_STATS *)psStats);
			}
			else
			{
				psStats = NULL;
			}
			break;
		case COMP_ECM:
			if (desSysMode == IDES_ECM)
			{
				intSetECMShadowStats((ECM_STATS *)psStats);
			}
			else
			{
				psStats = NULL;
			}
			break;
		case COMP_WEAPON:
			if (desSysMode == IDES_WEAPON)
			{
				intSetWeaponShadowStats((WEAPON_STATS *)psStats);
			}
			else
			{
				psStats = NULL;
			}
			break;
		case COMP_CONSTRUCT:
			if (desSysMode == IDES_CONSTRUCT)
			{
				intSetConstructShadowStats((CONSTRUCT_STATS *)psStats);
			}
			else
			{
				psStats = NULL;
			}
			break;
		case COMP_BRAIN:
			psStats = NULL;
			break;
		case COMP_REPAIRUNIT:
			if (desSysMode == IDES_REPAIR)
			{
				intSetRepairShadowStats((REPAIR_STATS *)psStats);
			}
			else
			{
				psStats = NULL;
			}
			break;
		}
	}

	if (psStats == NULL)
	{
		switch (desSysMode)
		{
		case IDES_SENSOR:
			intSetSensorShadowStats(NULL);
			break;
		case IDES_ECM:
			intSetECMShadowStats(NULL);
			break;
		case IDES_WEAPON:
			intSetWeaponShadowStats(NULL);
			break;
		case IDES_CONSTRUCT:
			intSetConstructShadowStats(NULL);
			break;
		case IDES_REPAIR:
			intSetRepairShadowStats(NULL);
			break;
		default:
			break;
		}
	}
}

/* Set the bar graphs for the sensor stats */
static void intSetSensorStats(SENSOR_STATS *psStats)
{
	ASSERT_OR_RETURN( , psStats != NULL, "Invalid stats pointer");
	ASSERT_OR_RETURN( , (psStats->ref >= REF_SENSOR_START) &&
			(psStats->ref < REF_SENSOR_START + REF_RANGE), "stats ref is out of range");

	/* range */
	widgSetBarSize(psWScreen, IDDES_SENSORRANGE,
		sensorRange(psStats, (UBYTE)selectedPlayer));
	/* power */
	widgSetBarSize(psWScreen, IDDES_SENSORPOWER,
		sensorPower(psStats, (UBYTE)selectedPlayer));
	/* weight */
	widgSetBarSize(psWScreen, IDDES_SENSORWEIGHT, psStats->weight);
}

/* Set the shadow bar graphs for the sensor stats */
static void intSetSensorShadowStats(SENSOR_STATS *psStats)
{
	ASSERT( psStats == NULL ||
			((psStats->ref >= REF_SENSOR_START) &&
			 (psStats->ref < REF_SENSOR_START + REF_RANGE)),
		"stats ref is out of range" );

	if (psStats)
	{
		/* range */
		widgSetMinorBarSize(psWScreen, IDDES_SENSORRANGE,
			sensorRange(psStats, (UBYTE)selectedPlayer));
		/* power */
		widgSetMinorBarSize(psWScreen, IDDES_SENSORPOWER,
			sensorPower(psStats, (UBYTE)selectedPlayer));
		/* weight */
		widgSetMinorBarSize(psWScreen, IDDES_SENSORWEIGHT, psStats->weight);
	}
	else
	{
		/* Remove the shadow bars */
		widgSetMinorBarSize(psWScreen, IDDES_SENSORRANGE, 0);
		widgSetMinorBarSize(psWScreen, IDDES_SENSORPOWER, 0);
		widgSetMinorBarSize(psWScreen, IDDES_SENSORWEIGHT, 0);
	}
}


/* Set the bar graphs for the ECM stats */
static void intSetECMStats(ECM_STATS *psStats)
{
	ASSERT_OR_RETURN( , psStats != NULL, "Invalid stats pointer");
	ASSERT_OR_RETURN( , (psStats->ref >= REF_ECM_START) &&
			(psStats->ref < REF_ECM_START + REF_RANGE), "stats ref is out of range");

	/* power */
	widgSetBarSize(psWScreen, IDDES_ECMPOWER,
		ecmPower(psStats, (UBYTE)selectedPlayer));
	/* weight */
	widgSetBarSize(psWScreen, IDDES_ECMWEIGHT, psStats->weight);
}

/* Set the shadow bar graphs for the ECM stats */
static void intSetECMShadowStats(ECM_STATS *psStats)
{
	ASSERT( psStats == NULL ||
			((psStats->ref >= REF_ECM_START) &&
			 (psStats->ref < REF_ECM_START + REF_RANGE)),
		"stats ref is out of range" );

	if (psStats)
	{
		/* power */
		widgSetMinorBarSize(psWScreen, IDDES_ECMPOWER,
			ecmPower(psStats, (UBYTE)selectedPlayer));
		/* weight */
		widgSetMinorBarSize(psWScreen, IDDES_ECMWEIGHT, psStats->weight);
	}
	else
	{
		/* Remove the shadow bars */
		widgSetMinorBarSize(psWScreen, IDDES_ECMPOWER, 0);
		widgSetMinorBarSize(psWScreen, IDDES_ECMWEIGHT, 0);
	}
}


/* Set the bar graphs for the Constructor stats */
static void intSetConstructStats(CONSTRUCT_STATS *psStats)
{
	ASSERT_OR_RETURN( , psStats != NULL, "Invalid stats pointer");
	ASSERT_OR_RETURN( , (psStats->ref >= REF_CONSTRUCT_START) &&
		(psStats->ref < REF_CONSTRUCT_START + REF_RANGE), "stats ref is out of range");

	/* power */
	widgSetBarSize(psWScreen, IDDES_CONSTPOINTS,
		constructorPoints(psStats, (UBYTE)selectedPlayer));
	/* weight */
	widgSetBarSize(psWScreen, IDDES_CONSTWEIGHT, psStats->weight);
}


/* Set the shadow bar graphs for the Constructor stats */
static void intSetConstructShadowStats(CONSTRUCT_STATS *psStats)
{
	ASSERT( psStats == NULL ||
			((psStats->ref >= REF_CONSTRUCT_START) &&
			 (psStats->ref < REF_CONSTRUCT_START + REF_RANGE)),
		"stats ref is out of range" );

	if (psStats)
	{
		/* power */
		widgSetMinorBarSize(psWScreen, IDDES_CONSTPOINTS,
			constructorPoints(psStats, (UBYTE)selectedPlayer));
		/* weight */
		widgSetMinorBarSize(psWScreen, IDDES_CONSTWEIGHT, psStats->weight);
	}
	else
	{
		/* reset the shadow bars */
		widgSetMinorBarSize(psWScreen, IDDES_CONSTPOINTS, 0);
		widgSetMinorBarSize(psWScreen, IDDES_CONSTWEIGHT, 0);
	}
}

/* Set the bar graphs for the Repair stats */
static void intSetRepairStats(REPAIR_STATS *psStats)
{
	ASSERT_OR_RETURN( , psStats != NULL, "Invalid stats pointer");
	ASSERT_OR_RETURN( , (psStats->ref >= REF_REPAIR_START) &&
			(psStats->ref < REF_REPAIR_START + REF_RANGE), "stats ref is out of range");

	/* power */
	widgSetBarSize(psWScreen, IDDES_REPAIRPOINTS,
		repairPoints(psStats, (UBYTE)selectedPlayer));
	/* weight */
	widgSetBarSize(psWScreen, IDDES_REPAIRWEIGHT, psStats->weight);
}


/* Set the shadow bar graphs for the Repair stats */
static void intSetRepairShadowStats(REPAIR_STATS *psStats)
{
	ASSERT( psStats == NULL ||
			((psStats->ref >= REF_REPAIR_START) &&
			 (psStats->ref < REF_REPAIR_START + REF_RANGE)),
		"stats ref is out of range" );

	if (psStats)
	{
		/* power */
		widgSetMinorBarSize(psWScreen, IDDES_REPAIRPOINTS,
			repairPoints(psStats, (UBYTE)selectedPlayer));
		/* weight */
		widgSetMinorBarSize(psWScreen, IDDES_REPAIRWEIGHT, psStats->weight);
	}
	else
	{
		/* reset the shadow bars */
		widgSetMinorBarSize(psWScreen, IDDES_REPAIRPOINTS, 0);
		widgSetMinorBarSize(psWScreen, IDDES_REPAIRWEIGHT, 0);
	}
}


/* Set the bar graphs for the Weapon stats */
static void intSetWeaponStats(WEAPON_STATS *psStats)
{
	ASSERT_OR_RETURN( , psStats != NULL, "Invalid stats pointer");
	ASSERT_OR_RETURN( , (psStats->ref >= REF_WEAPON_START) &&
			(psStats->ref < REF_WEAPON_START + REF_RANGE), "stats ref is out of range");

	/* range */
	widgSetBarSize(psWScreen, IDDES_WEAPRANGE, proj_GetLongRange(psStats));
	/* rate of fire */
	widgSetBarSize(psWScreen, IDDES_WEAPROF, weaponROF(psStats, (SBYTE)selectedPlayer));
	/* damage */
	widgSetBarSize(psWScreen, IDDES_WEAPDAMAGE, (UWORD)weaponDamage(psStats,
		(UBYTE)selectedPlayer));
	/* weight */
	widgSetBarSize(psWScreen, IDDES_WEAPWEIGHT, psStats->weight);
}

/* Set the shadow bar graphs for the Weapon stats */
static void intSetWeaponShadowStats(WEAPON_STATS *psStats)
{
	ASSERT( psStats == NULL ||
			((psStats->ref >= REF_WEAPON_START) &&
			 (psStats->ref < REF_WEAPON_START + REF_RANGE)),
		"stats ref is out of range" );

	if (psStats)
	{
		/* range */
		widgSetMinorBarSize(psWScreen, IDDES_WEAPRANGE, proj_GetLongRange(psStats));
		/* rate of fire */
		widgSetMinorBarSize(psWScreen, IDDES_WEAPROF, weaponROF(psStats, (SBYTE)selectedPlayer));
		/* damage */
		widgSetMinorBarSize(psWScreen, IDDES_WEAPDAMAGE, (UWORD)weaponDamage(
			psStats, (UBYTE)selectedPlayer));
		/* weight */
		widgSetMinorBarSize(psWScreen, IDDES_WEAPWEIGHT, psStats->weight);
	}
	else
	{
		/* Reset the shadow bars */
		widgSetMinorBarSize(psWScreen, IDDES_WEAPRANGE, 0);
		widgSetMinorBarSize(psWScreen, IDDES_WEAPROF, 0);
		widgSetMinorBarSize(psWScreen, IDDES_WEAPDAMAGE, 0);
		widgSetMinorBarSize(psWScreen, IDDES_WEAPWEIGHT, 0);
	}
}

/* Set the bar graphs for the Body stats */
static void intSetBodyStats(BODY_STATS *psStats)
{
	W_FORM	*psForm;

	ASSERT_OR_RETURN( , psStats != NULL, "Invalid stats pointer");
	ASSERT_OR_RETURN( , (psStats->ref >= REF_BODY_START) &&
			(psStats->ref < REF_BODY_START + REF_RANGE),
		"stats ref is out of range");

	/* set form tip to stats string */
	widgSetTip( psWScreen, IDDES_BODYFORM, getStatName(psStats) );

	/* armour */
	//	size = WBAR_SCALE * psStats->armourValue/DBAR_BODYMAXARMOUR;
	//do kinetic armour
	widgSetBarSize(psWScreen, IDDES_BODYARMOUR_K, bodyArmour(psStats,
		(UBYTE)selectedPlayer, DROID_BODY_UPGRADE, WC_KINETIC, 0));
	//do heat armour
	widgSetBarSize(psWScreen, IDDES_BODYARMOUR_H, bodyArmour(psStats,
		(UBYTE)selectedPlayer, DROID_BODY_UPGRADE, WC_HEAT, 0));
	/* body points */
	/*size = WBAR_SCALE * psStats->body/DBAR_BODYMAXPOINTS;
	if (size > WBAR_SCALE)
	{
		size = WBAR_SCALE;
	}
	widgSetBarSize(psWScreen, IDDES_BODYPOINTS, size);*/
	/* power */
	//widgSetBarSize(psWScreen, IDDES_BODYPOWER, psStats->powerOutput);
	widgSetBarSize(psWScreen, IDDES_BODYPOWER, bodyPower(psStats,
		(UBYTE)selectedPlayer,DROID_BODY_UPGRADE));

	/* weight */
	widgSetBarSize(psWScreen, IDDES_BODYWEIGHT, psStats->weight);

	/* set form stats for later display in intDisplayStatForm */
	psForm = (W_FORM *) widgGetFromID( psWScreen, IDDES_BODYFORM );
	if ( psForm != NULL )
	{
		psForm->pUserData = psStats;
	}
}

/* Set the shadow bar graphs for the Body stats */
static void intSetBodyShadowStats(BODY_STATS *psStats)
{
	ASSERT( psStats == NULL ||
			((psStats->ref >= REF_BODY_START) &&
			 (psStats->ref < REF_BODY_START + REF_RANGE)),
		"stats ref is out of range" );

	if (psStats)
	{
		/* armour - kinetic*/
		//size = WBAR_SCALE * psStats->armourValue/DBAR_BODYMAXARMOUR;
		//widgSetMinorBarSize(psWScreen, IDDES_BODYARMOUR_K,psStats->armourValue[WC_KINETIC]);
		widgSetMinorBarSize(psWScreen, IDDES_BODYARMOUR_K, bodyArmour(psStats,
			(UBYTE)selectedPlayer, DROID_BODY_UPGRADE, WC_KINETIC, 0));
		//armour - heat
		//widgSetMinorBarSize(psWScreen, IDDES_BODYARMOUR_H,psStats->armourValue[WC_HEAT]);
		widgSetMinorBarSize(psWScreen, IDDES_BODYARMOUR_H,bodyArmour(psStats,
			(UBYTE)selectedPlayer, DROID_BODY_UPGRADE, WC_HEAT, 0));
		/* body points */
//			size = WBAR_SCALE * psStats->bodyPoints/DBAR_BODYMAXPOINTS;
//			if (size > WBAR_SCALE)
//			{
//				size = WBAR_SCALE;
//			}
//			widgSetMinorBarSize(psWScreen, IDDES_BODYPOINTS, size);
		/* power */
		//widgSetMinorBarSize(psWScreen, IDDES_BODYPOWER, psStats->powerOutput);
		widgSetMinorBarSize(psWScreen, IDDES_BODYPOWER, bodyPower(psStats,
			(UBYTE)selectedPlayer, DROID_BODY_UPGRADE));

		/* weight */
		widgSetMinorBarSize(psWScreen, IDDES_BODYWEIGHT, psStats->weight);
	}
	else
	{
		/* Reset the shadow bars */
		widgSetMinorBarSize(psWScreen, IDDES_BODYARMOUR_K, 0);
		widgSetMinorBarSize(psWScreen, IDDES_BODYARMOUR_H, 0);
//		widgSetMinorBarSize(psWScreen, IDDES_BODYPOINTS, 0);
		widgSetMinorBarSize(psWScreen, IDDES_BODYPOWER, 0);
		widgSetMinorBarSize(psWScreen, IDDES_BODYWEIGHT, 0);
	}
}

/* Sets the Design Power Bar for a given Template */
static void intSetDesignPower(DROID_TEMPLATE *psTemplate)
{
	/* use the same scale as PowerBar in main window so values are relative */
	widgSetBarSize(psWScreen, IDDES_POWERBAR, calcTemplatePower(psTemplate));
}

// work out current system component
static UDWORD getSystemType(DROID_TEMPLATE* droidTemplate)
{
	if (droidTemplate->asParts[COMP_ECM]) {
		return COMP_ECM;
	} else if (droidTemplate->asParts[COMP_SENSOR]) {
		return COMP_SENSOR;
	} else if (droidTemplate->asParts[COMP_CONSTRUCT]) {
		return COMP_CONSTRUCT;
	} else if (droidTemplate->asParts[COMP_REPAIRUNIT]) {
		return COMP_REPAIRUNIT;
	} else if (droidTemplate->asWeaps[0]) {
		return COMP_WEAPON;
	} else {
		// compare it with the current weapon
		return COMP_WEAPON;
	}
}

/* Set the shadow bar graphs for the template power points - psStats is new hilited stats*/
static void intSetTemplatePowerShadowStats(COMPONENT_STATS *psStats)
{
	UDWORD				type;
	UDWORD				power;

	if (psStats != NULL) {
		UDWORD bodyPower        = asBodyStats[sCurrDesign.asParts[COMP_BODY]].buildPower;
		UDWORD brainPower       = asBrainStats[sCurrDesign.asParts[COMP_BRAIN]].buildPower;
		UDWORD sensorPower      = asSensorStats[sCurrDesign.asParts[COMP_SENSOR]].buildPower;
		UDWORD ECMPower         = asECMStats[sCurrDesign.asParts[COMP_ECM]].buildPower;
		UDWORD repairPower      = asRepairStats[sCurrDesign.asParts[COMP_REPAIRUNIT]].buildPower;
		UDWORD constructPower   = asConstructStats[sCurrDesign.asParts[COMP_CONSTRUCT]].buildPower;
		UDWORD propulsionPower  = asPropulsionStats[sCurrDesign.asParts[COMP_PROPULSION]].buildPower;
		UDWORD weaponPower1     = asWeaponStats[sCurrDesign.numWeaps ? sCurrDesign.asWeaps[0] : 0].buildPower;
		UDWORD weaponPower2     = asWeaponStats[sCurrDesign.numWeaps>=2 ? sCurrDesign.asWeaps[1] : 0].buildPower;
		UDWORD weaponPower3     = asWeaponStats[sCurrDesign.numWeaps>=3 ? sCurrDesign.asWeaps[2] : 0].buildPower;
		UDWORD newComponentPower= psStats->buildPower;

		type = statType(psStats->ref);
		// Commanders receive the stats of their associated weapon.
		if (type == COMP_BRAIN)
		{
			newComponentPower += ((BRAIN_STATS *)psStats)->psWeaponStat->buildPower;
		}
		/*if type = BODY or PROPULSION can do a straight comparison but if the new stat is
		a 'system' stat then need to find out which 'system' is currently in place so the
		comparison is meaningful*/
		if (desCompMode == IDES_SYSTEM)
		{
			type = getSystemType(&sCurrDesign);
		}

		switch (type)
		{
		case COMP_BODY:
			bodyPower = newComponentPower;
			break;
		case COMP_PROPULSION:
			propulsionPower = newComponentPower;
			break;
		case COMP_ECM:
			ECMPower = newComponentPower;
			break;
		case COMP_SENSOR:
			sensorPower = newComponentPower;
			break;
		case COMP_CONSTRUCT:
			constructPower = newComponentPower;
			break;
		case COMP_REPAIRUNIT:
			repairPower = newComponentPower;
			break;
		case COMP_WEAPON:
			brainPower = 0;
			if (desCompMode == IDES_TURRET_A)
			{
				weaponPower2 = newComponentPower;
			}
			else if (desCompMode == IDES_TURRET_B)
			{
				weaponPower3 = newComponentPower;
			}
			else
			{
				weaponPower1 = newComponentPower;
			}
			break;
		//default:
			//don't want to draw for unknown comp
		}

		// this code is from calcTemplatePower

		//get the component power
		power = bodyPower + brainPower + sensorPower + ECMPower + repairPower + constructPower;

		/* propulsion power points are a percentage of the bodys' power points */
		power += (propulsionPower *
			bodyPower) / 100;

		//add weapon power
		power += weaponPower1 + weaponPower2 + weaponPower3;
		widgSetMinorBarSize( psWScreen, IDDES_POWERBAR,
								power);
	}
	else
	{
		/* Reset the shadow bar */
		widgSetMinorBarSize(psWScreen, IDDES_POWERBAR, 0);
	}
}

/* Sets the Body Points Bar for a given Template */
static void intSetBodyPoints(DROID_TEMPLATE *psTemplate)
{
	// If total greater than Body Bar size then scale values.
	widgSetBarSize( psWScreen, IDDES_BODYPOINTS, calcTemplateBody(psTemplate,
		(UBYTE)selectedPlayer) );
}

/* Set the shadow bar graphs for the template Body points - psStats is new hilited stats*/
static void intSetTemplateBodyShadowStats(COMPONENT_STATS *psStats)
{
	UDWORD				type;
	UDWORD				body;

	if (psStats != NULL) {
		UDWORD bodyBody        = asBodyStats[sCurrDesign.asParts[COMP_BODY]].body;
		UDWORD brainBody       = asBrainStats[sCurrDesign.asParts[COMP_BRAIN]].body;
		UDWORD sensorBody      = asSensorStats[sCurrDesign.asParts[COMP_SENSOR]].body;
		UDWORD ECMBody         = asECMStats[sCurrDesign.asParts[COMP_ECM]].body;
		UDWORD repairBody      = asRepairStats[sCurrDesign.asParts[COMP_REPAIRUNIT]].body;
		UDWORD constructBody   = asConstructStats[sCurrDesign.asParts[COMP_CONSTRUCT]].body;
		UDWORD propulsionBody  = asPropulsionStats[sCurrDesign.asParts[COMP_PROPULSION]].body;
		UDWORD weaponBody1     = asWeaponStats[sCurrDesign.numWeaps ? sCurrDesign.asWeaps[0] : 0].body;
		UDWORD weaponBody2     = asWeaponStats[sCurrDesign.numWeaps>=2 ? sCurrDesign.asWeaps[1] : 0].body;
		UDWORD weaponBody3     = asWeaponStats[sCurrDesign.numWeaps>=3 ? sCurrDesign.asWeaps[2] : 0].body;
		UDWORD newComponentBody= psStats->body;

		type = statType(psStats->ref);
		// Commanders receive the stats of their associated weapon.
		if (type == COMP_BRAIN)
		{
			newComponentBody += ((BRAIN_STATS *)psStats)->psWeaponStat->body;
		}
		/*if type = BODY or PROPULSION can do a straight comparison but if the new stat is
		a 'system' stat then need to find out which 'system' is currently in place so the
		comparison is meaningful*/
		if ( desCompMode == IDES_SYSTEM )
		{
			type = getSystemType( &sCurrDesign );
		}

		switch (type)
		{
		case COMP_BODY:
			bodyBody = newComponentBody;
			break;
		case COMP_PROPULSION:
			propulsionBody = newComponentBody;
			break;
		case COMP_ECM:
			ECMBody = newComponentBody;
			break;
		case COMP_SENSOR:
			sensorBody = newComponentBody;
			break;
		case COMP_CONSTRUCT:
			constructBody = newComponentBody;
			break;
		case COMP_REPAIRUNIT:
			repairBody = newComponentBody;
			break;
		case COMP_WEAPON:
			brainBody = 0;
			if (desCompMode == IDES_TURRET_A)
			{
				weaponBody2 = newComponentBody;
			}
			else if (desCompMode == IDES_TURRET_B)
			{
				weaponBody3 = newComponentBody;
			}
			else
			{
				weaponBody1 = newComponentBody;
			}
			break;
		//default:
			//don't want to draw for unknown comp
		}
		// this code is from calcTemplateBody

    	//get the component HP
    	body = bodyBody + brainBody + sensorBody + ECMBody + repairBody + constructBody;

    	/* propulsion HP are a percentage of the body's HP */
    	body += (propulsionBody *
    		bodyBody) / 100;

     	//add weapon HP
        body += weaponBody1 + weaponBody2 + weaponBody3;
    	body += (body * asBodyUpgrade[selectedPlayer]->body / 100);
   		widgSetMinorBarSize( psWScreen, IDDES_BODYPOINTS,
								body);
	}
	else
	{
		/* Reset the shadow bar */
		widgSetMinorBarSize(psWScreen, IDDES_BODYPOINTS, 0);
	}
}


/* Calculate the speed of a droid over a type of terrain */
static UDWORD intCalcSpeed(TYPE_OF_TERRAIN type, PROPULSION_STATS *psProp)
{
	UDWORD		weight;

	/* Calculate the weight */
	weight = calcDroidWeight(&sCurrDesign);
	if (weight == 0)
	{
		return 0;
	}
	//we want the design screen to show zero speed over water for all prop types except Hover and Vtol
	if (type == TER_WATER)
	{
		if (!(psProp->propulsionType == PROPULSION_TYPE_HOVER || psProp->propulsionType == PROPULSION_TYPE_LIFT))
		{
			return 0;
		}
	}


	return calcDroidSpeed(calcDroidBaseSpeed(&sCurrDesign, weight,
		(UBYTE)selectedPlayer), type, psProp - asPropulsionStats, 0);
}


/* Set the bar graphs for the Propulsion stats */
static void intSetPropulsionStats(PROPULSION_STATS *psStats)
{
	W_FORM	    *psForm;
	UDWORD      weight;

	ASSERT_OR_RETURN( , psStats != NULL, "Invalid stats pointer");
	ASSERT_OR_RETURN( , (psStats->ref >= REF_PROPULSION_START) &&
			(psStats->ref < REF_PROPULSION_START + REF_RANGE), "stats ref is out of range");

	/* set form tip to stats string */
	widgSetTip( psWScreen, IDDES_PROPFORM, getStatName(psStats) );

	/* set form stats for later display in intDisplayStatForm */
	psForm = (W_FORM *) widgGetFromID( psWScreen, IDDES_PROPFORM );
	if ( psForm != NULL )
	{
		psForm->pUserData = psStats;
	}

	switch (desPropMode)
	{
	case IDES_GROUND:
		/* Road speed */
		widgSetBarSize(psWScreen, IDDES_PROPROAD, intCalcSpeed(TER_ROAD, psStats));
		/* Cross country speed - grass */
		widgSetBarSize(psWScreen, IDDES_PROPCOUNTRY, intCalcSpeed(TER_SANDYBRUSH, psStats));
		/* Water speed */
		widgSetBarSize(psWScreen, IDDES_PROPWATER, intCalcSpeed(TER_WATER, psStats));
		break;
	case IDES_AIR:
		/* Air speed - terrain type doesn't matter, use road */
		widgSetBarSize(psWScreen, IDDES_PROPAIR, intCalcSpeed(TER_ROAD, psStats));
		break;
	default:
		break;
	}

	/* weight */
	//widgSetBarSize(psWScreen, IDDES_PROPWEIGHT, psStats->weight);

	/* propulsion weight is a percentage of the body weight */
	if (sCurrDesign.asParts[COMP_BODY] != 0)
	{
		weight = psStats->weight * asBodyStats[sCurrDesign.asParts[COMP_BODY]].weight / 100;
	}
	else
	{
		//if haven't got a body - can't calculate a value
		weight = 0;
	}
	widgSetBarSize(psWScreen, IDDES_PROPWEIGHT, weight);
}


/* Set the shadow bar graphs for the Propulsion stats */
static void intSetPropulsionShadowStats(PROPULSION_STATS *psStats)
{
	UDWORD      weight;


	ASSERT( psStats == NULL ||
			((psStats->ref >= REF_PROPULSION_START) &&
			 (psStats->ref < REF_PROPULSION_START + REF_RANGE)),
		"stats ref is out of range" );

	/* Only set the shadow stats if they are the right type */
	if (psStats &&
		((asPropulsionTypes[psStats->propulsionType].travel == GROUND &&
		  desPropMode != IDES_GROUND) ||
		 (asPropulsionTypes[psStats->propulsionType].travel == AIR &&
		  desPropMode != IDES_AIR)))
	{
		return;
	}

	switch (desPropMode)
	{
	case IDES_GROUND:
		if (psStats)
		{
			/* Road speed */
			widgSetMinorBarSize( psWScreen, IDDES_PROPROAD,
									intCalcSpeed(TER_ROAD, psStats) );
			/* Cross country speed - grass */
			widgSetMinorBarSize( psWScreen, IDDES_PROPCOUNTRY,
									intCalcSpeed(TER_SANDYBRUSH, psStats) );
			/* Water speed */
			widgSetMinorBarSize(psWScreen, IDDES_PROPWATER,
									intCalcSpeed(TER_WATER, psStats));
		}
		else
		{
			/* Reset the shadow bars */
			widgSetMinorBarSize(psWScreen, IDDES_PROPROAD, 0);
			widgSetMinorBarSize(psWScreen, IDDES_PROPCOUNTRY, 0);
			widgSetMinorBarSize(psWScreen, IDDES_PROPWATER, 0);
		}
		break;
	case IDES_AIR:
		if (psStats)
		{
			/* Air speed - terrain type doesn't matter, use ROAD */
			widgSetMinorBarSize( psWScreen, IDDES_PROPAIR,
									intCalcSpeed(TER_ROAD, psStats) );
		}
		else
		{
			/* Reset the shadow bar */
			widgSetMinorBarSize(psWScreen, IDDES_PROPAIR, 0);
		}
		break;
	default:
		break;
	}

	if (psStats)
	{
		/* weight */
		//widgSetMinorBarSize(psWScreen, IDDES_PROPWEIGHT, psStats->weight);

		/* propulsion weight is a percentage of the body weight */
		if (sCurrDesign.asParts[COMP_BODY] != 0)
		{
			weight = psStats->weight * asBodyStats[sCurrDesign.asParts[COMP_BODY]].weight / 100;
		}
		else
		{
			//if haven't got a body - can't calculate a value
			weight = 0;
		}
		widgSetMinorBarSize(psWScreen, IDDES_PROPWEIGHT, weight);
	}
	else
	{
		/* Reset the shadow bar */
		widgSetMinorBarSize(psWScreen, IDDES_PROPWEIGHT, 0);
	}
}


/* Check whether a droid template is valid */
BOOL intValidTemplate(DROID_TEMPLATE *psTempl, const char *newName)
{
	UDWORD i;

	// set the weapon for a command droid
	if (psTempl->asParts[COMP_BRAIN] != 0)
	{
		psTempl->numWeaps = 1;
		psTempl->asWeaps[0] = asBrainStats[psTempl->asParts[COMP_BRAIN]].psWeaponStat - asWeaponStats;
	}

	/* Check all the components have been set */
	if (psTempl->asParts[COMP_BODY] == 0)
	{
		return false;
	}
	else if (psTempl->asParts[COMP_PROPULSION] == 0)
	{
		return false;
	}

	// Check a turret has been installed
	if (psTempl->numWeaps == 0 &&
		psTempl->asParts[COMP_SENSOR] == 0 &&
		psTempl->asParts[COMP_ECM] == 0 &&
		psTempl->asParts[COMP_BRAIN] == 0 &&
		psTempl->asParts[COMP_REPAIRUNIT] == 0 &&
		psTempl->asParts[COMP_CONSTRUCT] == 0 )
	{
		return false;
	}

	/* Check the weapons */
	for(i=0; i<psTempl->numWeaps; i++)
	{
		if (psTempl->asWeaps[i] == 0)
		{
			return false;
		}
	}

	// Check no mixing of systems and weapons
	if (psTempl->numWeaps != 0 &&
	    (psTempl->asParts[COMP_SENSOR] ||
	     psTempl->asParts[COMP_ECM] ||
	     (psTempl->asParts[COMP_REPAIRUNIT] && psTempl->asParts[COMP_REPAIRUNIT] != aDefaultRepair[selectedPlayer]) ||
	     psTempl->asParts[COMP_CONSTRUCT]))
	{
		return false;
	}
	if (psTempl->numWeaps != 1 &&
	    psTempl->asParts[COMP_BRAIN])
	{
		return false;
	}
	
	//can only have a weapon on a VTOL propulsion
	if (checkTemplateIsVtol(psTempl))
	{
		if (psTempl->numWeaps == 0)
		{
			return false;
		}
	}

	if (psTempl->asParts[COMP_SENSOR] == 0)
	{
		/* Set the default Sensor */
		psTempl->asParts[COMP_SENSOR] = aDefaultSensor[selectedPlayer];
	}

	if (psTempl->asParts[COMP_ECM] == 0)
	{
		/* Set the default ECM */
		psTempl->asParts[COMP_ECM] = aDefaultECM[selectedPlayer];
	}

	if (psTempl->asParts[COMP_REPAIRUNIT] == 0)
	{
		/* Set the default Repair */
		psTempl->asParts[COMP_REPAIRUNIT] = aDefaultRepair[selectedPlayer];
	}

	psTempl->ref = REF_TEMPLATE_START;

	/* Calculate build points */
	psTempl->buildPoints = calcTemplateBuild(psTempl);
	psTempl->powerPoints = calcTemplatePower(psTempl);

	//set the droidtype
	psTempl->droidType = droidTemplateType(psTempl);

	/* copy name into template */
	sstrcpy(psTempl->aName, newName);

	return true;
}

static void desCreateDefaultTemplate( void )
{
	/* set current design to default */
	memcpy( &sCurrDesign, &sDefaultDesignTemplate, sizeof(DROID_TEMPLATE) );
	sCurrDesign.pName = NULL;

	/* reset stats */
	intSetDesignStats(&sCurrDesign);
	widgDelete(psWScreen, IDDES_SYSTEMFORM);
	desSysMode = IDES_NOSYSTEM;
	CurrentStatsTemplate = (BASE_STATS *) &sCurrDesign;
}

/* Remove the design widgets from the widget screen */
void intRemoveDesign(void)
{
	//save the current design on exit if it is valid
	saveTemplate();

	newTemplate = false;

	widgDelete(psWScreen, IDDES_POWERFORM);
	widgDelete(psWScreen, IDDES_NAMEBOX);
	widgDelete(psWScreen, IDDES_TEMPLFORM);
	widgDelete(psWScreen, IDDES_TEMPLBASE);
	widgDelete(psWScreen, IDDES_COMPFORM);
	widgDelete(psWScreen, IDDES_RIGHTBASE);

	widgDelete(psWScreen, IDDES_BODYFORM);
	widgDelete(psWScreen, IDDES_PROPFORM);
	widgDelete(psWScreen, IDDES_SYSTEMFORM);

	widgDelete(psWScreen, IDDES_FORM);
	widgDelete( psWScreen, IDDES_STATSFORM );

	resetDesignPauseState();
}

/* set flashing flag for button */
static void intSetButtonFlash( UDWORD id, BOOL bFlash )
{
#ifdef FLASH_BUTTONS
	WIDGET	*psWidget = widgGetFromID( psWScreen, id );

	ASSERT_OR_RETURN( , psWidget->type == WIDG_BUTTON, "Not a button");

	if ( bFlash == true )
	{
		psWidget->display = intDisplayButtonFlash;
	}
	else
	{
		psWidget->display = intDisplayButtonHilight;
	}
#endif
}

/*
 * desTemplateNameCustomised
 *
 * Checks whether user has customised template name : template not
 * customised if not complete or if generated name same as current.
 */
static BOOL desTemplateNameCustomised( DROID_TEMPLATE *psTemplate )
{
	if ( (psTemplate->droidType == DROID_DEFAULT) ||
		 (strcmp( getTemplateName(psTemplate),
				  GetDefaultTemplateName(psTemplate) ) == 0) )
	{
		return false;
	}
	else
	{
		return true;
	}
}

/* Process return codes from the design screen */
void intProcessDesign(UDWORD id)
{
	DROID_TEMPLATE	*psTempl = NULL, *psCurr, *psPrev;
	//DROID_TEMPLATE	*psTempPrev;
	UDWORD			currID;
	UDWORD			i;
	BOOL			bTemplateNameCustomised;

	/* check template button pressed */
	if (id >= IDDES_TEMPLSTART && id <= IDDES_TEMPLEND)
	{
		/* if first template create blank design */
		if ( id == IDDES_TEMPLSTART )
		{
			desCreateDefaultTemplate();

			sstrcpy(aCurrName, _("New Vehicle"));
			sstrcpy(sCurrDesign.aName, aCurrName);

			/* reveal body button */
			widgReveal( psWScreen, IDDES_BODYBUTTON );
			/* hide other component buttons */
			widgHide( psWScreen, IDDES_SYSTEMBUTTON );
			widgHide( psWScreen, IDDES_PROPBUTTON );
			widgHide( psWScreen, IDDES_WPABUTTON );
			widgHide( psWScreen, IDDES_WPBBUTTON );

			/* set button render routines to flash */
			intSetButtonFlash( IDDES_BODYBUTTON,   true );
			intSetButtonFlash( IDDES_SYSTEMBUTTON, true );
			intSetButtonFlash( IDDES_PROPBUTTON,   true );
			intSetButtonFlash( IDDES_WPABUTTON,   true );
			intSetButtonFlash( IDDES_WPBBUTTON,   true );
		}
		else
		{
			/* Find the template for the new button */
			currID = IDDES_TEMPLSTART;
			for( i=0; i<MAXTEMPLATES; i++ )
			{
				psTempl = apsTemplateList[i];

				if (currID == id)
				{
					break;
				}
				currID ++;
			}

			ASSERT_OR_RETURN(, psTempl != NULL, "template not found!");

			if ( psTempl != NULL )
			{
				/* Set the new template */
				memcpy(&sCurrDesign, psTempl, sizeof(DROID_TEMPLATE));
				sstrcpy(aCurrName, getTemplateName(psTempl));

				/* reveal body/propulsion/turret component buttons */
				widgReveal( psWScreen, IDDES_BODYBUTTON );
				widgReveal( psWScreen, IDDES_PROPBUTTON );
				widgReveal( psWScreen, IDDES_SYSTEMBUTTON );
				/* hide extra turrets */
				widgHide( psWScreen, IDDES_WPABUTTON );
				widgHide( psWScreen, IDDES_WPBBUTTON );
				
				/* turn off button flashes */
				intSetButtonFlash( IDDES_BODYBUTTON,   false );
				intSetButtonFlash( IDDES_SYSTEMBUTTON, false );
				intSetButtonFlash( IDDES_PROPBUTTON,   false );
				intSetButtonFlash( IDDES_WPABUTTON,   false );
				intSetButtonFlash( IDDES_WPBBUTTON,   false );
				
				// reveal additional buttons
				if (psTempl->numWeaps >= 2)
				{
					widgReveal( psWScreen, IDDES_WPABUTTON );
				}
				else
				{
					intSetButtonFlash( IDDES_WPABUTTON,   true );
				}
				if (psTempl->numWeaps == 3)
				{
					widgReveal( psWScreen, IDDES_WPBBUTTON );
				}
				else
				{
					intSetButtonFlash( IDDES_WPBBUTTON,   true );
				}
			}
		}

		/* reveal design form if not already on-screen */
		widgReveal( psWScreen, IDDES_FORM );

		/* Droid template button has been pressed - clear the old button */
		if (droidTemplID != 0)
		{
			widgSetButtonState(psWScreen, droidTemplID, 0);
		}

		intSetDesignStats(&sCurrDesign);

		/* show body stats only */
		widgReveal( psWScreen, IDDES_STATSFORM );
		widgReveal( psWScreen, IDDES_BODYFORM );
		widgHide(   psWScreen, IDDES_PROPFORM );
		widgHide(   psWScreen, IDDES_SYSTEMFORM );

		/*Update the Power bar stats as the power to build will have changed */
		intSetDesignPower(&sCurrDesign);
		/*Update the body points */
		intSetBodyPoints(&sCurrDesign);

		/* Lock the button */
		widgSetButtonState(psWScreen, id, WBUT_LOCK);
		droidTemplID = id;

		/* Update the component form */
		widgDelete(psWScreen, IDDES_COMPFORM);
		widgDelete(psWScreen, IDDES_RIGHTBASE);
		/* reset button states */
		widgSetButtonState(psWScreen, IDDES_SYSTEMBUTTON, 0);
		widgSetButtonState(psWScreen, IDDES_BODYBUTTON,   0);
		widgSetButtonState(psWScreen, IDDES_PROPBUTTON,   0);
		widgSetButtonState(psWScreen, IDDES_WPABUTTON,   0);
		widgSetButtonState(psWScreen, IDDES_WPBBUTTON,   0);
		desCompMode = IDES_NOCOMPONENT;
		intSetDesignMode(IDES_BODY);
	}
	else if (id >= IDDES_COMPSTART && id <= IDDES_COMPEND)
	{
		/* check whether can change template name */
		bTemplateNameCustomised = desTemplateNameCustomised( &sCurrDesign );

		/* Component stats button has been pressed - clear the old button */
		if (desCompID != 0)
		{
			widgSetButtonState(psWScreen, desCompID, 0);
		}

		/* Set the stats in the template */
		switch (desCompMode)
		{
		case IDES_SYSTEM:
			//0 weapon for utility droid
			sCurrDesign.numWeaps = 0;
			break;
		case IDES_TURRET:
			/* Calculate the index of the component */
			sCurrDesign.asWeaps[0] =
				((WEAPON_STATS *)apsComponentList[id - IDDES_COMPSTART]) -
				asWeaponStats;
			if (sCurrDesign.numWeaps < 1)
			{
				sCurrDesign.numWeaps = 1;
			}
			/* Reset the sensor, ECM and constructor and repair
				- defaults will be set when OK is hit */
			sCurrDesign.asParts[COMP_SENSOR] = 0;
			sCurrDesign.asParts[COMP_ECM] = 0;
			sCurrDesign.asParts[COMP_CONSTRUCT] = 0;
			sCurrDesign.asParts[COMP_REPAIRUNIT] = 0;
			sCurrDesign.asParts[COMP_BRAIN] = 0;
			//Watemelon:weaponslots >= 2
			if( (asBodyStats + sCurrDesign.asParts[COMP_BODY])->weaponSlots >= 2 )
			{
				/* reveal turret_a button if hidden */
				widgReveal( psWScreen, IDDES_WPABUTTON );
			}
			/* Set the new stats on the display */
			intSetSystemForm(apsComponentList[id - IDDES_COMPSTART]);
			// Stop the button flashing
			intSetButtonFlash( IDDES_SYSTEMBUTTON, false );
			// do the callback if in the tutorial
			if (bInTutorial)
			{
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DESIGN_WEAPON);
			}
			break;
		//Added cases for 2nd/3rd turret
		case IDES_TURRET_A:
			/* Calculate the index of the component */
			sCurrDesign.asWeaps[1] =
				((WEAPON_STATS *)apsComponentList[id - IDDES_COMPSTART]) -
				asWeaponStats;
			if (sCurrDesign.numWeaps < 2)
			{
				sCurrDesign.numWeaps = 2;
			}
			/* Reset the sensor, ECM and constructor and repair
				- defaults will be set when OK is hit */
			sCurrDesign.asParts[COMP_SENSOR] = 0;
			sCurrDesign.asParts[COMP_ECM] = 0;
			sCurrDesign.asParts[COMP_CONSTRUCT] = 0;
			sCurrDesign.asParts[COMP_REPAIRUNIT] = 0;
			sCurrDesign.asParts[COMP_BRAIN] = 0;
			//Watemelon:weaponSlots > 2
			if( (asBodyStats + sCurrDesign.asParts[COMP_BODY])->weaponSlots > 2 )
			{
				/* reveal turret_b button if hidden */
				widgReveal( psWScreen, IDDES_WPBBUTTON );
			}
			/* Set the new stats on the display */
			intSetSystemForm(apsComponentList[id - IDDES_COMPSTART]);
			// Stop the button flashing
			intSetButtonFlash( IDDES_WPABUTTON,   false );
			// do the callback if in the tutorial
			if (bInTutorial)
			{
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DESIGN_WEAPON);
			}
			break;
		case IDES_TURRET_B:
			/* Calculate the index of the component */
			sCurrDesign.asWeaps[2] =
				((WEAPON_STATS *)apsComponentList[id - IDDES_COMPSTART]) -
				asWeaponStats;
			sCurrDesign.numWeaps = 3;
			/* Reset the sensor, ECM and constructor and repair
				- defaults will be set when OK is hit */
			sCurrDesign.asParts[COMP_SENSOR] = 0;
			sCurrDesign.asParts[COMP_ECM] = 0;
			sCurrDesign.asParts[COMP_CONSTRUCT] = 0;
			sCurrDesign.asParts[COMP_REPAIRUNIT] = 0;
			sCurrDesign.asParts[COMP_BRAIN] = 0;
			/* Set the new stats on the display */
			intSetSystemForm(apsComponentList[id - IDDES_COMPSTART]);
			// Stop the button flashing
			intSetButtonFlash( IDDES_WPBBUTTON,   false );
			// do the callback if in the tutorial
			if (bInTutorial)
			{
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DESIGN_WEAPON);
			}
			break;
		case IDES_BODY:
			/* reveal propulsion button if hidden */
			widgReveal( psWScreen, IDDES_PROPBUTTON );

			/* Calculate the index of the component */
			sCurrDesign.asParts[COMP_BODY] =
				((BODY_STATS *)apsComponentList[id - IDDES_COMPSTART]) -
				asBodyStats;
			/* Set the new stats on the display */
			intSetBodyStats((BODY_STATS *)apsComponentList[id - IDDES_COMPSTART]);

			if ((sCurrDesign.asParts[COMP_BODY] + asBodyStats)->weaponSlots == 1)
			{
				if (sCurrDesign.numWeaps > 1)
				{
					sCurrDesign.numWeaps = 1;
					sCurrDesign.asWeaps[1] = 0;
					sCurrDesign.asWeaps[2] = 0;
				}
				widgHide( psWScreen, IDDES_WPABUTTON );
				widgHide( psWScreen, IDDES_WPBBUTTON );
			}
			else if ((sCurrDesign.asParts[COMP_BODY] + asBodyStats)->weaponSlots >= 2)
			{
				if (sCurrDesign.numWeaps > 2)
				{
					sCurrDesign.numWeaps = 2;
					sCurrDesign.asWeaps[2] = 0;
				}
				else if (sCurrDesign.numWeaps == 1 && sCurrDesign.asWeaps[0] && sCurrDesign.asParts[COMP_BRAIN] == 0)
				{
					widgReveal( psWScreen, IDDES_WPABUTTON );
					widgSetButtonState(psWScreen, IDDES_WPABUTTON,   0);
					intSetButtonFlash( IDDES_WPABUTTON,   false );
				}
				else
				{
					widgHide( psWScreen, IDDES_WPBBUTTON );
				}
			}
			if ((sCurrDesign.asParts[COMP_BODY] + asBodyStats)->weaponSlots == 3)
			{
				if (sCurrDesign.numWeaps == 2)
				{
					widgReveal( psWScreen, IDDES_WPBBUTTON );
					widgSetButtonState(psWScreen, IDDES_WPBBUTTON,   0);
					intSetButtonFlash( IDDES_WPABUTTON,   false );
				}
				else if (sCurrDesign.numWeaps == 1 && sCurrDesign.asParts[COMP_BRAIN] == 0)
				{
					widgReveal( psWScreen, IDDES_WPABUTTON );
					widgSetButtonState(psWScreen, IDDES_WPABUTTON,   0);
				}
			}
			// Stop the button flashing
			intSetButtonFlash( IDDES_BODYBUTTON,   false );
			// do the callback if in the tutorial
			if (bInTutorial)
			{
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DESIGN_BODY);
			}
			break;
		case IDES_PROPULSION:
			/* Calculate the index of the component */
			sCurrDesign.asParts[COMP_PROPULSION] = ((PROPULSION_STATS *)apsComponentList[id - IDDES_COMPSTART]) - asPropulsionStats;

			/* Set the new stats on the display */
			intSetPropulsionStats((PROPULSION_STATS *)apsComponentList[id - IDDES_COMPSTART]);

			// Check that the weapon (if any) is valid for this propulsion
			if (!intCheckValidWeaponForProp())
			{
				// Not valid weapon so initialise the weapon stat
				sCurrDesign.asWeaps[0] = 0;
				sCurrDesign.asWeaps[1] = 0;
				sCurrDesign.asWeaps[2] = 0;
				sCurrDesign.numWeaps = 0;
				widgHide( psWScreen, IDDES_WPABUTTON );
				widgHide( psWScreen, IDDES_WPBBUTTON );

				// Init all other stats as well!
				sCurrDesign.asParts[COMP_SENSOR] = 0;
				sCurrDesign.asParts[COMP_BRAIN] = 0;
				sCurrDesign.asParts[COMP_REPAIRUNIT] = 0;
				sCurrDesign.asParts[COMP_CONSTRUCT] = 0;
				sCurrDesign.asParts[COMP_ECM] = 0;

				// We need a turret again
				intSetButtonFlash( IDDES_SYSTEMBUTTON, true );
			}

			// Stop the button flashing
			intSetButtonFlash( IDDES_PROPBUTTON,   false );
			// do the callback if in the tutorial
			if (bInTutorial)
			{
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DESIGN_PROPULSION);
			}
			break;
		default:
			break;
		}

		/* Lock the new button */
		widgSetButtonState(psWScreen, id, WBUT_LOCK);
		desCompID = id;

		/* Update the propulsion stats as the droid weight will have changed */
		intSetPropulsionStats(asPropulsionStats + sCurrDesign.asParts[COMP_PROPULSION]);

		/*Update the Power bar stats as the power to build will have changed */
		intSetDesignPower(&sCurrDesign);
		/*Update the body points */
		intSetBodyPoints(&sCurrDesign);

		/* update name if not customised */
		if ( bTemplateNameCustomised == false )
		{
			sstrcpy(sCurrDesign.aName, GetDefaultTemplateName(&sCurrDesign));
		}

		/* Update the name in the edit box */
		intSetEditBoxTextFromTemplate( &sCurrDesign );
	}
	else if (id >= IDDES_EXTRASYSSTART && id <= IDDES_EXTRASYSEND)
	{
		/* check whether can change template name */
		bTemplateNameCustomised = desTemplateNameCustomised( &sCurrDesign );

		// Extra component stats button has been pressed - clear the old button
		if (desCompID != 0)
		{
			widgSetButtonState(psWScreen, desCompID, 0);
		}

		// Now store the new stats
		switch (statType(apsExtraSysList[id - IDDES_EXTRASYSSTART]->ref))
		{
		case COMP_SENSOR:
			// Calculate the index of the component
			sCurrDesign.asParts[COMP_SENSOR] =
				((SENSOR_STATS *)apsExtraSysList[id - IDDES_EXTRASYSSTART]) -
				asSensorStats;
			// Reset the ECM, constructor and weapon and repair
			//	- defaults will be set when OK is hit
			sCurrDesign.numWeaps = 0;
			sCurrDesign.asWeaps[0] = 0;
			sCurrDesign.asParts[COMP_ECM] = 0;
			sCurrDesign.asParts[COMP_CONSTRUCT] = 0;
			sCurrDesign.asParts[COMP_REPAIRUNIT] = 0;
			sCurrDesign.asParts[COMP_BRAIN] = 0;
			widgHide( psWScreen, IDDES_WPABUTTON );
			widgHide( psWScreen, IDDES_WPBBUTTON );
			// Set the new stats on the display
			intSetSystemForm(apsExtraSysList[id - IDDES_EXTRASYSSTART]);
			break;
		case COMP_ECM:
			// Calculate the index of the component
			sCurrDesign.asParts[COMP_ECM] =
				((ECM_STATS *)apsExtraSysList[id - IDDES_EXTRASYSSTART]) -
				asECMStats;
			// Reset the Sensor, constructor and weapon and repair
			//	- defaults will be set when OK is hit
			sCurrDesign.numWeaps = 0;
			sCurrDesign.asWeaps[0] = 0;
			sCurrDesign.asParts[COMP_SENSOR] = 0;
			sCurrDesign.asParts[COMP_CONSTRUCT] = 0;
			sCurrDesign.asParts[COMP_REPAIRUNIT] = 0;
			sCurrDesign.asParts[COMP_BRAIN] = 0;
			widgHide( psWScreen, IDDES_WPABUTTON );
			widgHide( psWScreen, IDDES_WPBBUTTON );
			// Set the new stats on the display
			intSetSystemForm(apsExtraSysList[id - IDDES_EXTRASYSSTART]);
			break;
		case COMP_CONSTRUCT:
			// Calculate the index of the component and repair
			sCurrDesign.asParts[COMP_CONSTRUCT] =
				((CONSTRUCT_STATS *)apsExtraSysList[id - IDDES_EXTRASYSSTART]) -
				asConstructStats;
			// Reset the Sensor, ECM and weapon
			//	- defaults will be set when OK is hit
			sCurrDesign.numWeaps = 0;
			sCurrDesign.asWeaps[0] = 0;
			sCurrDesign.asParts[COMP_ECM] = 0;
			sCurrDesign.asParts[COMP_SENSOR] = 0;
			sCurrDesign.asParts[COMP_REPAIRUNIT] = 0;
			sCurrDesign.asParts[COMP_BRAIN] = 0;
			widgHide( psWScreen, IDDES_WPABUTTON );
			widgHide( psWScreen, IDDES_WPBBUTTON );
			// Set the new stats on the display
			intSetSystemForm(apsExtraSysList[id - IDDES_EXTRASYSSTART]);
			break;
		case COMP_REPAIRUNIT:
			// Calculate the index of the component
			sCurrDesign.asParts[COMP_REPAIRUNIT] =
				((REPAIR_STATS *)apsExtraSysList[id - IDDES_EXTRASYSSTART]) -
				asRepairStats;
			// Reset the Sensor, ECM and weapon and construct
			//	- defaults will be set when OK is hit
			sCurrDesign.numWeaps = 0;
			sCurrDesign.asWeaps[0] = 0;
			sCurrDesign.asParts[COMP_ECM] = 0;
			sCurrDesign.asParts[COMP_SENSOR] = 0;
			sCurrDesign.asParts[COMP_CONSTRUCT] = 0;
			sCurrDesign.asParts[COMP_BRAIN] = 0;
			widgHide( psWScreen, IDDES_WPABUTTON );
			widgHide( psWScreen, IDDES_WPBBUTTON );
			// Set the new stats on the display
			intSetSystemForm(apsExtraSysList[id - IDDES_EXTRASYSSTART]);
			break;
		case COMP_BRAIN:
			/* Calculate the index of the brain */
			sCurrDesign.asParts[COMP_BRAIN] =
				((BRAIN_STATS *)apsExtraSysList[id - IDDES_EXTRASYSSTART]) -
					asBrainStats;
			/* Reset the sensor, ECM and constructor and repair
				- defaults will be set when OK is hit */
			sCurrDesign.asParts[COMP_SENSOR] = 0;
			sCurrDesign.asParts[COMP_ECM] = 0;
			sCurrDesign.asParts[COMP_CONSTRUCT] = 0;
			sCurrDesign.asParts[COMP_REPAIRUNIT] = 0;
			sCurrDesign.numWeaps = 1;
			sCurrDesign.asWeaps[0] =
				(((BRAIN_STATS *)apsExtraSysList[id - IDDES_EXTRASYSSTART])->psWeaponStat) -
					asWeaponStats;
			widgHide( psWScreen, IDDES_WPABUTTON );
			widgHide( psWScreen, IDDES_WPBBUTTON );
			/* Set the new stats on the display */
			intSetSystemForm(apsExtraSysList[id - IDDES_EXTRASYSSTART]);
			break;
		}
		// Stop the button flashing
		intSetButtonFlash( IDDES_SYSTEMBUTTON, false );
		// Lock the new button
		widgSetButtonState(psWScreen, id, WBUT_LOCK);
		desCompID = id;

		// Update the propulsion stats as the droid weight will have changed
		intSetPropulsionStats(asPropulsionStats + sCurrDesign.asParts[COMP_PROPULSION]);

		// Update the Power bar stats as the power to build will have changed
		intSetDesignPower(&sCurrDesign);
		// Update the body points
		intSetBodyPoints(&sCurrDesign);

		/* update name if not customised */
		if ( bTemplateNameCustomised == false )
		{
			sstrcpy(sCurrDesign.aName, GetDefaultTemplateName(&sCurrDesign));
		}

		/* Update the name in the edit box */
		intSetEditBoxTextFromTemplate( &sCurrDesign );

		// do the callback if in the tutorial
		if (bInTutorial)
		{
			if (statType(apsExtraSysList[id - IDDES_EXTRASYSSTART]->ref) == COMP_BRAIN)
			{
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DESIGN_COMMAND);
			}
			else
			{
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DESIGN_SYSTEM);
			}
		}
	}
	else
	{
		switch (id)
		{
			/* The four component clickable forms */
			/* the six component clickable forms... */
		case IDDES_WEAPONS:
			desCompID = 0;
			intSetDesignMode(IDES_TURRET);
			break;
		case IDDES_WEAPONS_A:
			desCompID = 0;
			intSetDesignMode(IDES_TURRET_A);
			break;
		case IDDES_WEAPONS_B:
			desCompID = 0;
			intSetDesignMode(IDES_TURRET_B);
			break;
		case IDDES_COMMAND:
			desCompID = 0;
			break;
		case IDDES_SYSTEMS:
			desCompID = 0;
			intSetDesignMode(IDES_SYSTEM);
			break;
			/* The name edit box */
		case IDDES_NAMEBOX:
			sstrcpy(sCurrDesign.aName, widgGetString(psWScreen, IDDES_NAMEBOX));
			sstrcpy(aCurrName, sCurrDesign.aName);
			break;
		case IDDES_BIN:
			/* Find the template for the current button */
			currID = IDDES_TEMPLSTART+1;
			//psTempPrev = NULL;
			for( i=1; i<MAXTEMPLATES; i++ )
			{
				psTempl = apsTemplateList[i];
				if (currID == droidTemplID && psTempl != &sCurrDesign)
				{
					break;
				}
				currID ++;
				//psTempPrev = psTempl;
			}



			/* remove template if found */
			if ( psTempl )
			{
				SendDestroyTemplate(psTempl);

				//update player template list.
				{
					for (psCurr = apsDroidTemplates[selectedPlayer], psPrev = NULL;
						psCurr != NULL; psCurr = psCurr->psNext)
					{
						if (psCurr == psTempl)
						{
							if (psPrev)
							{
								psPrev->psNext = psCurr->psNext;
							}
							else
							{
								apsDroidTemplates[selectedPlayer] = psCurr->psNext;
							}

							//quit looking cos found
							break;
						}
						psPrev = psCurr;
					}
				}

				// Delete the template.
				//before deleting the template, need to make sure not being used in production
				deleteTemplateFromProduction(psTempl, selectedPlayer, ModeQueue);
				free(psTempl);

				/* get previous template and set as current */
				psTempl = apsTemplateList[i-1];

				/* update local list */
				desSetupDesignTemplates();

				/* Now update the droid template form */
				newTemplate = false;
				widgDelete(psWScreen, IDDES_TEMPLFORM);
				widgDelete(psWScreen, IDDES_TEMPLBASE);
				intAddTemplateForm( psTempl );

				/* Set the new template */
				memcpy(&sCurrDesign, psTempl, sizeof(DROID_TEMPLATE));
				sstrcpy(aCurrName, getTemplateName(psTempl));

				intSetEditBoxTextFromTemplate( psTempl );

				intSetDesignStats(&sCurrDesign);

				/* show body stats only */
				widgReveal( psWScreen, IDDES_STATSFORM );
				widgReveal( psWScreen, IDDES_BODYFORM );
				widgHide(   psWScreen, IDDES_PROPFORM );
				widgHide(   psWScreen, IDDES_SYSTEMFORM );

				/*Update the Power bar stats as the power to build will have changed */
				intSetDesignPower(&sCurrDesign);
				/*Update the body points */
				intSetBodyPoints(&sCurrDesign);

				/* show correct body component highlight */
				widgDelete(psWScreen, IDDES_COMPFORM);
				widgDelete(psWScreen, IDDES_RIGHTBASE);
				/* reset button states */
				widgSetButtonState(psWScreen, IDDES_SYSTEMBUTTON, 0);
				widgSetButtonState(psWScreen, IDDES_BODYBUTTON,   0);
				widgSetButtonState(psWScreen, IDDES_PROPBUTTON,   0);
				widgSetButtonState(psWScreen, IDDES_WPABUTTON,   0);
				widgSetButtonState(psWScreen, IDDES_WPBBUTTON,   0);
				desCompMode = IDES_NOCOMPONENT;
				intSetDesignMode(IDES_BODY);
			}
			break;
		case IDDES_SYSTEMBUTTON:
			// Add the correct component form
			switch (droidTemplateType(&sCurrDesign))
			{
			case DROID_COMMAND:
			case DROID_SENSOR:
			case DROID_CONSTRUCT:
			case DROID_ECM:
			case DROID_REPAIR:
				intSetDesignMode(IDES_SYSTEM);
				break;
			default:
				intSetDesignMode(IDES_TURRET);
				break;
			}
			/* reveal components if not already onscreen */
			widgReveal( psWScreen, IDDES_STATSFORM );
			widgReveal( psWScreen, IDDES_RIGHTBASE );
			widgReveal( psWScreen, IDDES_SYSTEMFORM );
			widgHide(   psWScreen, IDDES_BODYFORM );
			widgHide(   psWScreen, IDDES_PROPFORM );

			break;
		// WPABUTTON
		case IDDES_WPABUTTON:
			// Add the correct component form
			switch (droidTemplateType(&sCurrDesign))
			{
			case DROID_COMMAND:
			case DROID_SENSOR:
			case DROID_CONSTRUCT:
			case DROID_ECM:
			case DROID_REPAIR:
				break;
			default:
				intSetDesignMode(IDES_TURRET_A);
				break;
			}
			/* reveal components if not already onscreen */
			widgReveal( psWScreen, IDDES_STATSFORM );
			widgReveal( psWScreen, IDDES_RIGHTBASE );
			widgReveal( psWScreen, IDDES_SYSTEMFORM );
			widgHide(   psWScreen, IDDES_BODYFORM );
			widgHide(   psWScreen, IDDES_PROPFORM );

			break;
		// WPBBUTTON
		case IDDES_WPBBUTTON:
			// Add the correct component form
			switch (droidTemplateType(&sCurrDesign))
			{
			case DROID_COMMAND:
			case DROID_SENSOR:
			case DROID_CONSTRUCT:
			case DROID_ECM:
			case DROID_REPAIR:
				break;
			default:
				intSetDesignMode(IDES_TURRET_B);
				break;
			}
			/* reveal components if not already onscreen */
			widgReveal( psWScreen, IDDES_STATSFORM );
			widgReveal( psWScreen, IDDES_RIGHTBASE );
			widgReveal( psWScreen, IDDES_SYSTEMFORM );
			widgHide(   psWScreen, IDDES_BODYFORM );
			widgHide(   psWScreen, IDDES_PROPFORM );

			break;
		case IDDES_BODYBUTTON:
			/* reveal components if not already onscreen */
			widgReveal( psWScreen, IDDES_RIGHTBASE );
			intSetDesignMode(IDES_BODY);

			widgReveal( psWScreen, IDDES_STATSFORM );
			widgHide(   psWScreen, IDDES_SYSTEMFORM );
			widgReveal( psWScreen, IDDES_BODYFORM );
			widgHide(   psWScreen, IDDES_PROPFORM );

			break;
		case IDDES_PROPBUTTON:
			/* reveal components if not already onscreen */
			widgReveal( psWScreen, IDDES_RIGHTBASE );
			intSetDesignMode(IDES_PROPULSION);
			widgReveal( psWScreen, IDDES_STATSFORM );
			widgHide(   psWScreen, IDDES_SYSTEMFORM );
			widgHide(   psWScreen, IDDES_BODYFORM );
			widgReveal( psWScreen, IDDES_PROPFORM );

			break;
		}
	}

	/* show body button if component button pressed and
	 * save template if valid
	 */
	if ( ( id >= IDDES_COMPSTART && id <= IDDES_COMPEND ) ||
		 ( id >= IDDES_EXTRASYSSTART && id <= IDDES_EXTRASYSEND ) )
	{
		/* reveal body button if hidden */
		widgReveal( psWScreen, IDDES_BODYBUTTON );

		/* save template if valid */
		if (saveTemplate())
		{
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DROIDDESIGNED);
		}

		switch ( desCompMode )
		{
			case IDES_BODY:
				widgReveal( psWScreen, IDDES_BODYFORM );
				widgHide(   psWScreen, IDDES_PROPFORM );
				widgHide(   psWScreen, IDDES_SYSTEMFORM );
				break;

			case IDES_PROPULSION:
				widgHide(   psWScreen, IDDES_BODYFORM );
				widgReveal( psWScreen, IDDES_PROPFORM );
				widgHide(   psWScreen, IDDES_SYSTEMFORM );
				break;

			case IDES_SYSTEM:
			case IDES_TURRET:
			// reveals SYSTEMFORM
			case IDES_TURRET_A:
			case IDES_TURRET_B:
				widgHide(   psWScreen, IDDES_BODYFORM );
				widgHide(   psWScreen, IDDES_PROPFORM );
				widgReveal( psWScreen, IDDES_SYSTEMFORM );
				break;
			default:
				break;
		}

		widgReveal( psWScreen, IDDES_STATSFORM );

		/* switch automatically to next component type if initial design */
		if ( !intValidTemplate( &sCurrDesign, aCurrName ) )
		{
			/* show next component design screen */
			switch ( desCompMode )
			{
				case IDES_BODY:
					intSetDesignMode( IDES_PROPULSION );
					widgReveal(psWScreen, IDDES_PROPBUTTON);
					break;

				case IDES_PROPULSION:
					intSetDesignMode( IDES_TURRET );
					widgReveal(psWScreen, IDDES_SYSTEMBUTTON);
					break;

				case IDES_SYSTEM:
				case IDES_TURRET:
					if ((asBodyStats + sCurrDesign.asParts[COMP_BODY])->weaponSlots > 1 &&
					    sCurrDesign.numWeaps == 1 && sCurrDesign.asParts[COMP_BRAIN] == 0)
					{
						debug(LOG_GUI, "intProcessDesign: First weapon selected, doing next.");
						intSetDesignMode( IDES_TURRET_A );
						widgReveal(psWScreen, IDDES_WPABUTTON);
					}
					else
					{
						debug(LOG_GUI, "intProcessDesign: First weapon selected, is final.");
					}
					break;
				case IDES_TURRET_A:
					if ( (asBodyStats + sCurrDesign.asParts[COMP_BODY])->weaponSlots > 2 )
					{
						debug(LOG_GUI, "intProcessDesign: Second weapon selected, doing next.");
						intSetDesignMode( IDES_TURRET_B );
						widgReveal(psWScreen, IDDES_WPBBUTTON);
					}
					else
					{
						debug(LOG_GUI, "intProcessDesign: Second weapon selected, is final.");
					}
					break;
				case IDES_TURRET_B:
					debug(LOG_GUI, "intProcessDesign: Third weapon selected, is final.");
					break;
				default:
					break;
			}
		}
	}
	//save the template if the name gets edited
	if (id == IDDES_NAMEBOX)
	{
		saveTemplate();
	}

}


/* Set the shadow bar graphs for the design screen */
void intRunDesign(void)
{
	UDWORD				statID;
	COMPONENT_STATS		*psStats;
	BOOL				templateButton;
	int compIndex;

	/* Find out which button was hilited */
	templateButton = false;
	statID = widgGetMouseOver(psWScreen);

	// Somut around here is casuing a nasty crash.....
	/* If a component button is hilited get the stats for it */
	if (statID == desCompID)
	{
		/* The mouse is over the selected component - no shadow stats */
		psStats = NULL;
	}
	else if (statID >= IDDES_COMPSTART && statID <= IDDES_COMPEND)
	{
		compIndex = statID - IDDES_COMPSTART;
		ASSERT_OR_RETURN( , compIndex < numComponent, "Invalid range referenced for numComponent, %d > %d", compIndex, numComponent);
		psStats = apsComponentList[compIndex];
	}
	else if (statID >= IDDES_EXTRASYSSTART && statID <= IDDES_EXTRASYSEND)
	{
		compIndex = statID - IDDES_EXTRASYSSTART;
		ASSERT_OR_RETURN( , compIndex < numExtraSys, "Invalid range referenced for numExtraSys, %d > %d", compIndex, numExtraSys);
		psStats = apsExtraSysList[compIndex];
	}
	else if (statID >= IDDES_TEMPLSTART && statID <= IDDES_TEMPLEND)
	{
		runTemplateShadowStats(statID);
		templateButton = true;
		psStats = NULL;
	}
	else
	{
		/* No component button so reset the stats to nothing */
		psStats = NULL;
	}

	/* Now set the bar graphs for the stats - don't bother if over template
	since setting them all!*/
	if (!templateButton)
	{
		switch (desCompMode)
		{
		case IDES_SYSTEM:
		case IDES_TURRET:
			intSetSystemShadowStats(psStats);
			intSetBodyShadowStats(NULL);
			intSetPropulsionShadowStats(NULL);
			break;
		case IDES_BODY:
			intSetSystemShadowStats(NULL);
			intSetBodyShadowStats((BODY_STATS *)psStats);
			intSetPropulsionShadowStats(NULL);
			break;
		case IDES_PROPULSION:
			intSetSystemShadowStats(NULL);
			intSetBodyShadowStats(NULL);
			intSetPropulsionShadowStats((PROPULSION_STATS *)psStats);
			break;
		default:
			break;
		}

		//set the template shadow stats
		intSetTemplateBodyShadowStats(psStats);
		intSetTemplatePowerShadowStats(psStats);
	}
}

static void intDisplayStatForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	static UDWORD	iRY = 45;

	W_CLICKFORM		*Form = (W_CLICKFORM*)psWidget;
	UWORD			x0 = xOffset+Form->x, y0 = yOffset+Form->y;

	/* get stats from userdata pointer in widget stored in
	 * intSetSystemStats, intSetBodyStats, intSetPropulsionStats
	 */
	BASE_STATS *psStats = (BASE_STATS *) Form->pUserData;

	SWORD templateRadius = getComponentRadius(psStats);

	Vector3i Rotation = {-30, iRY, 0}, Position = {0, -templateRadius / 4, BUTTON_DEPTH /* templateRadius * 12 */};

	//scale the object around the BUTTON_RADIUS so that half size objects are draw are draw 75% the size of normal objects
	SDWORD falseScale = (DESIGN_COMPONENT_SCALE * COMPONENT_RADIUS) / templateRadius / 2 + (DESIGN_COMPONENT_SCALE / 2);

	iV_DrawImage(IntImages,(UWORD)(IMAGE_DES_STATBACKLEFT),x0,y0);
	iV_DrawImageRect(IntImages,IMAGE_DES_STATBACKMID,
				x0+iV_GetImageWidth(IntImages,IMAGE_DES_STATBACKLEFT),y0,
				Form->width-iV_GetImageWidth(IntImages,IMAGE_DES_STATBACKLEFT)-iV_GetImageWidth(IntImages,IMAGE_DES_STATBACKRIGHT),
				iV_GetImageHeight(IntImages,IMAGE_DES_STATBACKMID) );
	iV_DrawImage(IntImages,IMAGE_DES_STATBACKRIGHT,
				x0+Form->width-iV_GetImageWidth(IntImages,(UWORD)(IMAGE_DES_STATBACKRIGHT)),y0);

	/* display current component */
	pie_SetGeometricOffset( (xOffset+psWidget->width/4),
							(yOffset+psWidget->height/2) );

	/* inc rotation if highlighted */
	if ( Form->state & WCLICK_HILITE )
	{
		iRY += realTimeAdjustedIncrement(BUTTONOBJ_ROTSPEED);
		iRY %= 360;
	}

	//display component in bottom design screen window
	displayComponentButton( psStats, &Rotation, &Position, true, falseScale);
}

/* Displays the 3D view of the droid in a window on the design form */
static void intDisplayViewForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_FORM			*Form = (W_FORM*)psWidget;
	UDWORD			x0,y0,x1,y1;
	static UDWORD	iRY = 45;
	Vector3i			Rotation, Position;
	SWORD			templateRadius;
	SDWORD			falseScale;

	x0 = xOffset+Form->x;
	y0 = yOffset+Form->y;
	x1 = x0 + Form->width;
	y1 = y0 + Form->height;


	RenderWindowFrame(FRAME_NORMAL, x0, y0, x1 - x0, y1 - y0);

	if(CurrentStatsTemplate) {

		pie_SetGeometricOffset(  (DES_CENTERFORMX+DES_3DVIEWX) + (DES_3DVIEWWIDTH/2),
								(DES_CENTERFORMY+DES_3DVIEWY) + (DES_3DVIEWHEIGHT/4) + 32);

		Rotation.x = -30;
		Rotation.y = iRY;
		Rotation.z = 0;

		/* inc rotation */
		iRY += realTimeAdjustedIncrement(BUTTONOBJ_ROTSPEED);
		iRY %= 360;

		//fixed depth scale the pie
		Position.x = 0;
		Position.y = -100;
		Position.z = BUTTON_DEPTH;

		templateRadius = (SWORD)(getComponentDroidTemplateRadius((DROID_TEMPLATE*)
			CurrentStatsTemplate));
		//scale the object around the OBJECT_RADIUS so that half size objects are draw are draw 75% the size of normal objects
		falseScale = (DESIGN_DROID_SCALE * OBJECT_RADIUS) / templateRadius;

		//display large droid view in the design screen
		displayComponentButtonTemplate((DROID_TEMPLATE*)&sCurrDesign,&Rotation,&Position,true, falseScale);
	}
}


void intDisplayTemplateButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
	intDisplayStatsButton(psWidget, xOffset, yOffset, pColours);
}


static void intDisplayComponentButton(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours)
{
//	iIMDShape *OldCurShape = CurrentStatsShape;
//	SWORD OldCurIndex = CurrentStatsIndex;
	BASE_STATS *OldCurStatsTemplate = CurrentStatsTemplate;

	intDisplayStatsButton(psWidget, xOffset, yOffset, pColours);

	CurrentStatsTemplate = OldCurStatsTemplate;
//	CurrentStatsShape = OldCurShape;
//	CurrentStatsIndex = OldCurIndex;
}

/* General display window for the design form  SOLID BACKGROUND - NOT TRANSPARENT*/
void intDisplayDesignForm(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, WZ_DECL_UNUSED PIELIGHT *pColours)
{
	W_TABFORM *Form = (W_TABFORM*)psWidget;
	UDWORD x0,y0,x1,y1;

	x0 = xOffset+Form->x;
	y0 = yOffset+Form->y;
	x1 = x0 + Form->width;
	y1 = y0 + Form->height;

	//AdjustTabFormSize(Form,&x0,&y0,&x1,&y1);

	RenderWindowFrame(FRAME_NORMAL, x0, y0, x1 - x0, y1 - y0);
}


/* save the current Template if valid. Return true if stored */
static BOOL saveTemplate(void)
{
	DROID_TEMPLATE	*psTempl = NULL, *psPlayerTempl, *psPrevTempl;
	BOOL			stored = false, bTemplateFound = false;
	UDWORD			i, iCurrID;

	/* if first (New Design) button selected find empty template
	 * else find current button template
	 */
	if ( droidTemplID == IDDES_TEMPLSTART )
	{
		/* find empty template and point to that */
		for( i=1; i<MAXTEMPLATES; i++ )
		{
			psTempl = apsTemplateList[i];

			if ( psTempl == NULL )
			{
				bTemplateFound = true;
				break;
			}
		}
	}
	else
	{
		/* Find the template for the current button */
		iCurrID = IDDES_TEMPLSTART + 1;
		for( i=1; i<MAXTEMPLATES; i++ )
		{
			psTempl = apsTemplateList[i];

			if ( iCurrID == droidTemplID )
			{
				bTemplateFound = true;
				break;
			}
			iCurrID++;
		}
	}

	if ( bTemplateFound == true && intValidTemplate( &sCurrDesign, aCurrName ) )
	{
		/* create new template if button is NULL,
		 * else store changes to existing template */
		if ( psTempl == NULL )
		{
			/* The design needs a new template in the list */
			psTempl = (DROID_TEMPLATE *)malloc(sizeof(DROID_TEMPLATE));
			if (psTempl == NULL)
			{
				debug(LOG_ERROR, "saveTemplate: Out of memory");
				return false;
			}

			psTempl->ref = REF_TEMPLATE_START;
			newTemplate = true;
			/* Add it to temp array */
			apsTemplateList[i] = psTempl;

			/* update player template list */
			psPlayerTempl = apsDroidTemplates[selectedPlayer];
			psPrevTempl = NULL;
			while ( psPlayerTempl != NULL )
			{
				psPrevTempl = psPlayerTempl;
				psPlayerTempl = psPlayerTempl->psNext;
			}
			if ( psPrevTempl == NULL )
			{
				apsDroidTemplates[selectedPlayer] = psTempl;
			}
			else
			{
				psPrevTempl->psNext = psTempl;
			}

			/* set button render routines to highlight, not flash */
			intSetButtonFlash( IDDES_SYSTEMBUTTON, false );
			intSetButtonFlash( IDDES_BODYBUTTON,   false );
			intSetButtonFlash( IDDES_PROPBUTTON,   false );
		}
		else
		{
			/* Get existing template */
			psTempl = apsTemplateList[i];
			newTemplate = false;
			/*ANY change to the template affect the production - even if the
			template is changed and then changed back again!*/
			deleteTemplateFromProduction(psTempl, selectedPlayer, ModeQueue);
			SendDestroyTemplate(psTempl);
			sCurrDesign.multiPlayerID = generateNewObjectId();
		}

		/* Copy the template */
		memcpy(psTempl, &sCurrDesign, sizeof(DROID_TEMPLATE));
		sstrcpy(psTempl->aName, aCurrName);

		/* Now update the droid template form */
		widgDelete(psWScreen, IDDES_TEMPLFORM);
		widgDelete(psWScreen, IDDES_TEMPLBASE);
		intAddTemplateForm(psTempl);
		stored = true;
	}

	if (stored)
	{
		ASSERT_OR_RETURN( false, psTempl != NULL, "Template is NULL in saveTemplate()!");
		psTempl->multiPlayerID = generateNewObjectId();
		if (bMultiMessages)
		{
			sendTemplate(psTempl);
		}
	}

	return stored;
}


/*Function to set the shadow bars for all the stats when the mouse is over
the Template buttons*/
void runTemplateShadowStats(UDWORD id)
{
	UDWORD			currID;
	DROID_TEMPLATE	*psTempl = NULL;
	COMPONENT_STATS	*psStats;
	DROID_TYPE		templType;
	UDWORD			i;
	int				compIndex;

	/* Find the template for the new button */
	//currID = IDDES_TEMPLSTART;
	//we're ignoring the Blank Design so start at the second button
	currID = IDDES_TEMPLSTART + 1;
	for( i=1; i<MAXTEMPLATES; i++ )
	{
		psTempl = apsTemplateList[i];
		if (currID == id)
		{
			break;
		}
		currID ++;
	}

	//if we're over a different template
	if (psTempl && psTempl != &sCurrDesign)
	{
		/* Now set the bar graphs for the stats */
		intSetBodyShadowStats(asBodyStats + psTempl->asParts[COMP_BODY]);
		intSetPropulsionShadowStats(asPropulsionStats + psTempl->asParts[COMP_PROPULSION]);
		//only set the system shadow bar if the same type of droid
		psStats = NULL;
		templType = droidTemplateType(psTempl);
		if (templType == droidTemplateType(&sCurrDesign))
		{
			switch (templType)
			{
			case DROID_WEAPON:
				compIndex = psTempl->asWeaps[0];
				ASSERT_OR_RETURN( , compIndex < numWeaponStats, "Invalid range referenced for numWeaponStats, %d > %d", compIndex, numWeaponStats);
				psStats = (COMPONENT_STATS *)(asWeaponStats + compIndex);
				break;
			case DROID_SENSOR:
				compIndex = psTempl->asParts[COMP_SENSOR];
				ASSERT_OR_RETURN( , compIndex < numSensorStats, "Invalid range referenced for numSensorStats, %d > %d", compIndex, numSensorStats);
				psStats = (COMPONENT_STATS *)(asSensorStats + compIndex);
				break;
			case DROID_ECM:
				compIndex = psTempl->asParts[COMP_ECM];
				ASSERT_OR_RETURN( , compIndex < numECMStats, "Invalid range referenced for numECMStats, %d > %d", compIndex, numECMStats);
				psStats = (COMPONENT_STATS *)(asECMStats + compIndex);
				break;
			case DROID_CONSTRUCT:
				compIndex = psTempl->asParts[COMP_CONSTRUCT];
				ASSERT_OR_RETURN( , compIndex < numConstructStats, "Invalid range referenced for numConstructStats, %d > %d", compIndex, numConstructStats);
				psStats = (COMPONENT_STATS *)(asConstructStats + compIndex);
				break;
			case DROID_REPAIR:
				compIndex = psTempl->asParts[COMP_REPAIRUNIT];
				ASSERT_OR_RETURN( , compIndex < numRepairStats, "Invalid range referenced for numRepairStats, %d > %d", compIndex, numRepairStats);
				psStats = (COMPONENT_STATS *)(asRepairStats + compIndex);
				break;
			default:
				break;
			}
		}

		if (psStats)
		{
			intSetSystemShadowStats(psStats);
		}
		//set the template shadow stats
		//intSetTemplateBodyShadowStats(psStats);
		//haven't got a stat so just do the code required here...
		widgSetMinorBarSize( psWScreen, IDDES_BODYPOINTS,
								calcTemplateBody(psTempl, (UBYTE)selectedPlayer) );

		//intSetTemplatePowerShadowStats(psStats);
		widgSetMinorBarSize( psWScreen, IDDES_POWERBAR,
								calcTemplatePower(psTempl) );
	}
}

/*sets which states need to be paused when the design screen is up*/
void setDesignPauseState(void)
{
	if (!bMultiPlayer)
	{
		//need to clear mission widgets from being shown on design screen
		clearMissionWidgets();
		gameTimeStop();
		setGameUpdatePause(true);
		setScrollPause(true);
		screen_RestartBackDrop();
	}
}

/*resets the pause states */
void resetDesignPauseState(void)
{
	if (!bMultiPlayer)
	{
		//put any widgets back on for the missions
		resetMissionWidgets();
		setGameUpdatePause(false);
		setScrollPause(false);
		gameTimeStart();
		screen_StopBackDrop();
		pie_ScreenFlip(CLEAR_BLACK);
	}
}

/*this is called when a new propulsion type is added to the current design
to check the weapon is 'allowed'. Check if VTOL, the weapon is direct fire.
Also check numVTOLattackRuns for the weapon is not zero - return true if valid weapon*/
static BOOL intCheckValidWeaponForProp(void)
{
	if (asPropulsionTypes[((PROPULSION_STATS *)(asPropulsionStats + sCurrDesign.asParts[COMP_PROPULSION]))->propulsionType].travel != AIR)
	{
		if (sCurrDesign.numWeaps == 0 &&
		    (sCurrDesign.asParts[COMP_SENSOR] ||
		     sCurrDesign.asParts[COMP_REPAIRUNIT] ||
		     sCurrDesign.asParts[COMP_CONSTRUCT] ||
		     sCurrDesign.asParts[COMP_ECM]))
		{
			// non-AIR propulsions can have systems, too.
			return true;
		}
	}
	return checkValidWeaponForProp(&sCurrDesign);
}

BOOL intAddDesign( BOOL bShowCentreScreen )
{
	return _intAddDesign(bShowCentreScreen);
}


/* Set up the system clickable form of the design screen given a set of stats */
static BOOL intSetSystemForm(COMPONENT_STATS *psStats)
{
	return _intSetSystemForm(psStats);
}


static BOOL intAddTemplateForm(DROID_TEMPLATE *psSelected)
{
	return _intAddTemplateForm(psSelected);
}

//checks if the template has PROPULSION_TYPE_LIFT propulsion attached - returns true if it does
BOOL checkTemplateIsVtol(DROID_TEMPLATE *psTemplate)
{
	if (asPropulsionStats[psTemplate->asParts[COMP_PROPULSION]].
		propulsionType == PROPULSION_TYPE_LIFT)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*goes thru' the list passed in reversing the order so the first entry becomes
the last and the last entry becomes the first!*/
void reverseTemplateList(DROID_TEMPLATE **ppsList)
{
	DROID_TEMPLATE     *psPrev, *psNext, *psCurrent, *psObjList;

	//initialise the pointers
	psObjList = *ppsList;
	psPrev = psNext = NULL;
	psCurrent = psObjList;

	while(psCurrent != NULL)
	{
		psNext = psCurrent->psNext;
		psCurrent->psNext = psPrev;
		psPrev = psCurrent;
		psCurrent = psNext;
	}
	//set the list passed in to point to the new top
	*ppsList = psPrev;
}

