/*
* This file is part of Warzone 2100, an open-source, cross-platform, real-time strategy game
* Copyright (C) 2007  Giel van Schijndel, Warzone Ressurection Project
*
* This program is distributed under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
*
* $Revision$
* $Id$
* $HeadURL$
*/

/** C-interface
 *  This file provides an interface so that C-code is capable of using the soundlibrary.
 */

#include "soundbase.h"
#include "sound.h"

static soundBase* sndBase = NULL;

BOOL sound_InitLibrary()
{
    // check wether the library has already been initialized
    if (sndBase)
    {
        sndBase = new soundBase(true);
    }
    else
    {
        return TRUE;
    }

    sndBase->setListenerPos( 0.0, 0.0, 0.0 );
    sndBase->setListenerVel( 0.0, 0.0, 0.0 );
    //sndBase->setListenerRot( x, y, z ); // TODO: first implement sndBase::setListenerRot,
    //                                    // then calculate values for this function call

    ALfloat listenerOri[6] = { 0.0, 0.0, 1.0, 0.0, 1.0, 0.0 }; // Will replace this with
    alListenerfv( AL_ORIENTATION, listenerOri );               // soundBase::setListenerRot.
    return TRUE;
}

void sound_ShutdownLibrary()
{
    if (sndBase)
        delete sndBase;
}

sndStreamID sound_Create2DStream(char* path)
{
    if (sndBase)
    {
        // do something here
    }
    else
        return 0;
}

void sound_Update(void)
{
    sndBase->updateStreams();
}
