/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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

	$Revision$
	$Id$
	$HeadURL$
*/

/** C-interface
 *  This file provides an interface so that C-code is capable of using the soundlibrary.
 */

#include "sound.h"
#include "openal/context.hpp"
#include <AL/al.h>
#include <AL/alc.h>
#include <vector>
#include <string>
#include <iostream>
#include <exception>

// Library initialization status
static bool Initialized = false;

// OpenAL device specific stuff
static ALCdevice* sndDevice = NULL;
static std::vector<std::string> sndDeviceList;

// OpenAL rendering contexts (can be rendered in parallel)
static soundContext* snd3DContext = NULL;
static soundContext* snd2DContext = NULL;

/** Enumerates OpenAL devices available to open for output
 *  The output values are put into std::vector<std::string> sndDevicelist
 *  \throw std::exception
 */
void enumerateDevices()
{
    // This function call should never fail according to the OpenAL specs (not with these params)
    // So as long as std::vector doesn't throw any exceptions there should occur no problems
    const char* DeviceList = static_cast<const char*>(alcGetString(NULL, ALC_DEVICE_SPECIFIER));

    sndDeviceList.clear();

    if (DeviceList == NULL)
    {
        fprintf(stderr, "soundLib: alcGetString returning NULL; ALC_ERROR: %d; filling DeviceList with default NULL device.\n", alcGetError(NULL));
        sndDeviceList.push_back(std::string(""));
        return;
    }

    // The returned C-string DeviceList has its entries separated by one NUL char (0x00),
    // and the list itself is terminated by two NUL chars.
    while (*DeviceList != 0x00)
    {
        // Append the current list entry to the list
        sndDeviceList.push_back(std::string(DeviceList));

        std::cout << "sound Device: " << DeviceList << std::endl;

        // Move to the next entry in the list
        // strlen really only detects the position of the first NUL char in the array
        DeviceList += strlen(DeviceList) + 1;
    }
}

BOOL sound_InitLibrary()
{
    return sound_InitLibraryWithDevice(0);
}

BOOL sound_InitLibraryWithDevice(unsigned int soundDevice)
{
    // check wether the library has already been previously initialized
    if (Initialized) return FALSE;

    try
    {
        enumerateDevices();

        // Open device (default dev (0) usually is "Generic Hardware")
        if (sndDeviceList.at(soundDevice) == std::string(""))
            sndDevice = alcOpenDevice(NULL);
        else
            sndDevice = alcOpenDevice(sndDeviceList.at(soundDevice).c_str());

        if (sndDevice == NULL) return FALSE;

        snd3DContext = new soundContext(sndDevice);
        snd2DContext = new soundContext(sndDevice, true);

        snd3DContext->setListenerPos( 0.0, 0.0, 0.0 );
        snd3DContext->setListenerVel( 0.0, 0.0, 0.0 );
        //sndBase->setListenerRot( x, y, z ); // TODO: first implement sndBase::setListenerRot,
        //                                    // then calculate values for this function call

        snd3DContext->makeCurrent();

        ALfloat listenerOri[6] = { 0.0, 0.0, 1.0, 0.0, 1.0, 0.0 }; // Will replace this with
        alListenerfv( AL_ORIENTATION, listenerOri );               // soundContext::setListenerRot.

        Initialized = true;
    }
    catch (std::string &e)
    {
        std::cerr << "soundLib: ERROR " << e << std::endl;
    }
    catch (std::exception &e)
    {
        std::cerr << "soundLib: ERROR " << e.what() << std::endl;
    }

    return (Initialized ? TRUE : FALSE);
}

void sound_ShutdownLibrary()
{
    if (!Initialized) return;

    if (sndDevice)
    {
        alcCloseDevice(sndDevice);
        sndDevice = NULL;
    }

    delete snd3DContext;
    delete snd2DContext;
}

sndStreamID sound_Create2DStream(char* path)
{
    if (sndDevice)
    {
        // do something here
    }
    else
        return 0;
}

void sound_Update(void)
{
    snd3DContext->updateStreams();
    snd2DContext->updateStreams();
}
