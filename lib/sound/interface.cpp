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

// C-interface helper utility classes
#include "interface/stringarray.hpp"
#include "interface/devicelist.hpp"

#include <map>
#include <list>
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

// Some containers of exported objects (through ID numbers)
static std::map<sndStreamID, boost::shared_ptr<soundStream> > sndStreams;
static std::map<sndTrackID, boost::shared_ptr<soundBuffer> > sndTracks;

// Makes sure sound sources are kept alive until they're not needed anymore
static std::list<boost::shared_ptr<soundSource> > sndSamples;

// The ID numbers
static sndStreamID nextStreamID = 0;
static sndTrackID nextTrackID = 0;

BOOL sound_InitLibrary()
{
    return sound_InitLibraryWithDevice(0);
}

BOOL sound_InitLibraryWithDevice(unsigned int deviceNum)
{
    // check wether the library has already been previously initialized
    if (Initialized) return FALSE;

    try
    {
        // Open device (default dev (0) usually is "Generic Hardware")
        sndDevice = boost::shared_ptr<soundDevice>(new soundDevice(interfaceUtil::DeviceList::Instance().at(deviceNum)));

        sndContext = boost::shared_ptr<soundContext>(new soundContext(sndDevice));

        // Always clear this list to make sure we're not using memory only necessary once
        interfaceUtil::DeviceList::DestroyInstance();

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
    sndSamples.clear();
    sndTracks.clear();
    sndStreams.clear();

    sndContext.reset();
    sndDevice.reset();

    interfaceUtil::DeviceList::DestroyInstance();
}

const char** sound_DeviceList()
{
    return interfaceUtil::DeviceList::Instance();
}

template <class TypeID, class TypeObject>
inline bool validID(TypeID& ID, const TypeID& nextID, std::map<TypeID, boost::shared_ptr<TypeObject> >& MapCont)
{
    if (ID == 0)
    {
        fprintf(stderr, "soundLib: ERROR Invalid ID: 0\n");
        return false;
    }

    if (ID > nextID)
    {
        fprintf(stderr, "soundLib: ERROR Invalid ID: %d (out of bounds)\n", ID);
        return false;
    }

    ID -= 1;

    if (MapCont[ID].get() == NULL)
    {
        fprintf(stderr, "soundLib: ERROR Invalid ID: %d (NULL: probably destroyed object)\n", ID);
        MapCont.erase(ID);
        return false;
    }

    return true;
}

sndStreamID sound_Create2DStream(char* fileName)
{
    if (!Initialized)
        return 0;

    try
    {
        // Construct decoder object
        boost::shared_ptr<soundDecoding> decoder(new soundDecoding(fileName, false));

        // Construct streaming object
        boost::shared_ptr<soundStream> stream(new soundStream(sndContext, decoder));

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
    if (!Initialized || !validID(stream, nextStreamID, sndStreams)) return;

    sndStreams.erase(stream);
}

BOOL sound_Play2DStream(sndStreamID stream, BOOL reset)
{
    if (!Initialized || !validID(stream, nextStreamID, sndStreams)) return FALSE;

    try
    {
        if (reset)
            sndStreams[stream]->stop();

        return sndStreams[stream]->play() ? TRUE : FALSE;
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
    if (!Initialized || !validID(stream, nextStreamID, sndStreams)) return FALSE;

    try
    {
        return sndStreams[stream]->isPlaying() ? TRUE : FALSE;
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
    for (std::map<sndStreamID, boost::shared_ptr<soundStream> >::iterator stream = sndStreams.begin(); stream != sndStreams.end(); ++stream)
    {
        stream->second->update();
    }

    // We're not using a for loop here because we need to have control over the iterator's increment operation.
    // This is because as soon as we erase an iterator it becomes invalid that includes its increment operator
    std::list<boost::shared_ptr<soundSource> >::iterator sample = sndSamples.begin();
    while (sample != sndSamples.end())
    {
        if ((*sample)->getState() == soundSource::stopped)
        {
            std::list<boost::shared_ptr<soundSource> >::iterator tmp = sample;
            ++sample;
            sndSamples.erase(tmp);
        }
        else
            ++sample;
    }
}

sndTrackID sound_LoadTrackFromFile(char* fileName)
{
    // Construct decoder object
    boost::shared_ptr<soundDecoding> decoder(new soundDecoding(fileName, true));

    // Construct track/OpenAL buffer object
    boost::shared_ptr<soundBuffer> track(new soundBuffer);

    // Decode the track
    unsigned int size = 0;
    boost::shared_array<char> buffer(decoder->decode(size));

    if (size == 0)
        return 0;

    // Insert the buffer into the OpenAL buffer
    track->bufferData(decoder->getChannelCount(), decoder->frequency(), buffer.get(), size);

    // Insert the track into the container for later reference/usage
    sndTracks.insert(std::pair<sndTrackID, boost::shared_ptr<soundBuffer> >(nextTrackID, track));

    // First return current track ID plus 1 (because we reserve 0 for an invalid/non-existent stream)
    return nextTrackID++ + 1;
}

void sound_PlayTrack(sndTrackID track)
{
    if (!Initialized || !validID(track, nextTrackID, sndTracks)) return;

    boost::shared_ptr<soundSource> source(new soundSource(sndContext, sndTracks[track]));

    source->play();

    sndSamples.push_back(source);
}
