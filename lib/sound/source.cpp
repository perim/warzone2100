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
    if (!bIsStream)
        throw std::string("soundSource: attempt to (un)queue buffer to/from non-stream source");

    alGetError();   // clear current error state

    ALuint tmpBuffer = sndBuffer->getALBufferID();
    alSourceQueueBuffers(source, 1, &tmpBuffer);

    ALenum alErrNo = alGetError();
    if (alErrNo != AL_NO_ERROR)
    {
        switch (alErrNo)
        {
        case AL_INVALID_VALUE:
            throw std::string("alSourceQueueBuffers(): OpenAL: specified buffer is invalid or does not exist.");
        case AL_INVALID_OPERATION:
            throw std::string("alSourceQueueBuffers(): no current context or buffer format mismatched with the rest of queue (i.e. mono8/mono16/stereo8/stereo16)");
        }
    }
}

ALuint soundSource::unqueueBuffer()
{
    if (!bIsStream)
        throw std::string("soundSource: attempt to (un)queue buffer to/from non-stream source");

    alGetError();   // clear current error state

    ALuint buffer;
    alSourceUnqueueBuffers(source, 1, &buffer);

    ALenum alErrNo = alGetError();
    if (alErrNo != AL_NO_ERROR)
    {
        switch (alErrNo)
        {
        case AL_INVALID_VALUE:
            throw std::string("alSourceUnqueueBuffers(): not enough non-memory resources, or invalid pointer");
        default:
            throw std::string("alSourceUnqueueBuffers(): unknown OpenAL error");
        }
    }

    return buffer;
}

unsigned int soundSource::numProcessedBuffers()
{
    int count;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &count);

    return count;
}
