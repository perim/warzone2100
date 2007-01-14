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

#include "source.h"
#include <string>

soundSource::soundSource(bool b2D) : bIs2D(b2D), bIsStream(true)
{
    alGetError();
    alGenSources(1, &source);

    ALenum alErrNo = alGetError();
    if (alErrNo != AL_NO_ERROR)
    {
        switch (alErrNo)
        {
        case AL_OUT_OF_MEMORY:
            throw std::string("alGenSources(): Out of memory");
        case AL_INVALID_VALUE:
            throw std::string("alGenSources(): not enough non-memory resources, or invalid pointer");
        case AL_INVALID_OPERATION:
            throw std::string("alGenSources(): no context available to create sources in");
        }
    }
}

soundSource::soundSource(soundBuffer* sndBuffer, bool b2D) : bIs2D(b2D), bIsStream(false)
{
    alGetError();
    alGenSources(1, &source);

    ALenum alErrNo = alGetError();
    if (alErrNo != AL_NO_ERROR)
    {
        switch (alErrNo)
        {
        case AL_OUT_OF_MEMORY:
            throw std::string("alGenSources(): Out of memory");
        case AL_INVALID_VALUE:
            throw std::string("alGenSources(): not enough non-memory resources, or invalid pointer");
        case AL_INVALID_OPERATION:
            throw std::string("alGenSources(): no *current* context available to create sources in");
        }
    }

    alSourcei(source, AL_BUFFER, sndBuffer->getALBufferID());
    if ((alErrNo = alGetError()) != AL_NO_ERROR)
    {
        switch (alErrNo)
        {
        case AL_OUT_OF_MEMORY:
            throw std::string("alSource(): Out of memory");
        case AL_INVALID_VALUE:
            throw std::string("alSource(): not enough non-memory resources, or invalid pointer");
        }
    }
}

soundSource::~soundSource()
{
    alDeleteSources(1, &source);    // Should only fail if alGenSources() failed
}

bool soundSource::is2D()
{
    return bIs2D;
}

soundSource::sourceState soundSource::getState()
{
    ALenum state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    switch (state)
    {
    case AL_INITIAL:
        return initial;
    case AL_PLAYING:
        return playing;
    case AL_PAUSED:
        return paused;
    case AL_STOPPED:
        return stopped;
    }

    return undefined;
}

void soundSource::queueBuffer(soundBuffer* sndBuffer)
{
    if (bIsStream)
    {
        ALuint tmpBuffer = sndBuffer->getALBufferID();
        alSourceQueueBuffers(source, 1, &tmpBuffer);
    }
    else
        throw std::string("soundSource: attempt to queue buffer to non-stream source");
}
