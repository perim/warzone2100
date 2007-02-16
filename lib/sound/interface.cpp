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

// Declares library C-interface
#include "sound.h"

// Internal library includes
#include "openal/device.hpp"
#include "openal/context.hpp"
#include "stream.hpp"

#include <map>
#include <vector>
#include <string>
#include <exception>

#include <boost/smart_ptr.hpp>

// Needed by sound_DeviceList() for memcpy
#include <string.h>

// Library initialization status
static bool Initialized = false;

// OpenAL device specific stuff
static boost::shared_ptr<soundDevice> sndDevice;
static boost::shared_array<char*> DeviceList;

// OpenAL rendering contexts (can be rendered in parallel)
static boost::shared_ptr<soundContext> sndContext;

// Forward declarations of internal functions
static void clearDeviceList();

// Some containers of exported objects (through ID numbers
static std::map<sndStreamID, boost::shared_ptr<soundStream> > sndStreams;

// The ID numbers
static sndStreamID nextStreamID = 0;

BOOL sound_InitLibrary()
{
    return sound_InitLibraryWithDevice(0);
}

BOOL sound_InitLibraryWithDevice(unsigned int deviceNum)
{
    // Always clear this list to make sure we're not using memory only necessary once
    clearDeviceList();

    // check wether the library has already been previously initialized
    if (Initialized) return FALSE;

    try
    {
        // Open device (default dev (0) usually is "Generic Hardware")
        sndDevice = boost::shared_ptr<soundDevice>(new soundDevice(soundDevice::deviceList().at(deviceNum)));

        sndContext = boost::shared_ptr<soundContext>(new soundContext(sndDevice));

        Initialized = true;
    }
    catch (std::string &e)
    {
        fprintf(stderr, "soundLib: ERROR %s\n", e.c_str());
    }
    catch (std::exception &e)
    {
        fprintf(stderr, "soundLib: ERROR %s\n", e.what());
    }
    catch (...)
    {
        fprintf(stderr, "soundLib: ERROR An unhandled exception occurred!\n");
        throw;
    }

    return (Initialized ? TRUE : FALSE);
}

void sound_ShutdownLibrary()
{
    sndStreams.clear();

    sndContext.reset();
    sndDevice.reset();

    clearDeviceList();
}

const char** sound_DeviceList()
{
    if (DeviceList.get() == NULL)
    {
        // Allocate memory
        DeviceList = boost::shared_array<char*>(new char*[soundDevice::deviceList().size() + 1]);

        // Mark the end of the array
        DeviceList[soundDevice::deviceList().size()] = NULL;

        for (unsigned int i = 0; i != soundDevice::deviceList().size(); ++i)
        {
            // Allocate memory
            DeviceList[i] = new char[soundDevice::deviceList()[i].length() + 1];

            // Mark the end of the C-string
            DeviceList[i][soundDevice::deviceList()[i].length()] = 0;

            // Insert data
            memcpy(DeviceList[i], soundDevice::deviceList()[i].c_str(), soundDevice::deviceList()[i].length());
        }
    }

    return const_cast<const char**>(DeviceList.get());
}

/** Clears out and destroys the memory used by the enumeration array
 */
static void clearDeviceList()
{
    if (DeviceList.get() != NULL)
    {
        // Run through list and delete its contents (i.e. free memory of its contents)
        unsigned int i = 0;
        while (DeviceList[i] != NULL)
        {
            delete [] DeviceList[i];
            ++i;
        }

        // Destroy the list itself and automatically reset its pointer to NULL
        DeviceList.reset();
    }
}

inline bool validStream(sndStreamID stream)
{
    if (stream == 0)
    {
        fprintf(stderr, "soundLib: ERROR Invalid StreamID: %d\n", stream);
        return false;
    }

    if (stream > nextStreamID)
    {
        fprintf(stderr, "soundLib: ERROR Invalid StreamID: %d (out of bounds)\n", stream);
        return false;
    }

    if (sndStreams[stream - 1].get() == NULL)
    {
        fprintf(stderr, "soundLib: ERROR Invalid StreamID: %d (NULL: probably destroyed stream)\n", stream);
        return false;
    }

    return true;
}

sndStreamID sound_Create2DStream(char* path)
{
    if (!sndDevice || !Initialized)
        return 0;

    try
    {
        // Construct decoder object and OpenAL source object
        boost::shared_ptr<soundDecoding> decoder(new soundDecoding(path));
        boost::shared_ptr<soundSource> source(new soundSource(sndContext, true));

        // Construct streaming object
        boost::shared_ptr<soundStream> stream(new soundStream(source, decoder));

        // Insert the stream into the container for later reference/usage
        sndStreams.insert(std::pair<sndStreamID, boost::shared_ptr<soundStream> >(nextStreamID, stream));

        // First return current stream ID plus 1 (because we reserve 0 for an invalid/non-existent stream)
        return nextStreamID++ + 1;
    }
    catch (std::string &e)
    {
        fprintf(stderr, "soundLib: ERROR %s\n", e.c_str());
        return 0;
    }
    catch (std::exception &e)
    {
        fprintf(stderr, "soundLib: ERROR %s\n", e.what());
        return 0;
    }
    catch (...)
    {
        fprintf(stderr, "soundLib: ERROR An unhandled exception occurred!\n");
        throw;
    }
}

void sound_Destroy2DStream(sndStreamID stream)
{
    sndStreams[stream - 1].reset();
}

BOOL sound_Play2DStream(sndStreamID stream, BOOL reset)
{
    if (!Initialized || !stream) return FALSE;

    try
    {
        if (validStream(stream))
            return sndStreams[stream - 1]->play((reset == TRUE)) ? TRUE : FALSE;
        else
            return FALSE;
    }
    catch (std::string &e)
    {
        fprintf(stderr, "soundLib: ERROR %s\n", e.c_str());
        return FALSE;
    }
    catch (std::exception &e)
    {
        fprintf(stderr, "soundLib: ERROR %s\n", e.what());
        return FALSE;
    }
    catch (...)
    {
        fprintf(stderr, "soundLib: ERROR An unhandled exception occurred!\n");
        throw;
    }
}


BOOL sound_2DStreamIsPlaying(sndStreamID stream)
{
    if (!Initialized || !stream) return FALSE;

    try
    {
        if (validStream(stream))
            return sndStreams[stream - 1]->isPlaying() ? TRUE : FALSE;
        else
            return FALSE;
    }
    catch (std::string &e)
    {
        fprintf(stderr, "soundLib: ERROR %s\n", e.c_str());
        return FALSE;
    }
    catch (std::exception &e)
    {
        fprintf(stderr, "soundLib: ERROR %s\n", e.what());
        return FALSE;
    }
    catch (...)
    {
        fprintf(stderr, "soundLib: ERROR An unhandled exception occurred!\n");
        throw;
    }
}

void sound_Update(void)
{
    for (std::map<sndStreamID, boost::shared_ptr<soundStream> >::iterator i = sndStreams.begin(); i != sndStreams.end(); ++i)
    {
        i->second->update();
    }
}
