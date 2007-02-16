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

#include "source.hpp"
#include <string>
#include "stringconv.hpp"

soundSource::soundSource(boost::shared_ptr<soundContext> sndContext, bool b2D) : context(sndContext), bIs2D(b2D), bIsStream(true)
{
    createSource();
}

soundSource::soundSource(boost::shared_ptr<soundContext> sndContext, boost::shared_ptr<soundBuffer> sndBuffer, bool b2D) : context(sndContext), bIs2D(b2D), bIsStream(false)
{
    createSource();
    setBuffer(sndBuffer);
}

inline void soundSource::createSource()
{
    context->makeCurrent();

    // Clear current error state
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

    // Default initialization
    alSource3f(source, AL_POSITION,        0.0, 0.0, 0.0);
    alSource3f(source, AL_VELOCITY,        0.0, 0.0, 0.0);
    alSource3f(source, AL_DIRECTION,       0.0, 0.0, 0.0);
    alSourcef (source, AL_ROLLOFF_FACTOR,  0.0          );
    alSourcei (source, AL_SOURCE_RELATIVE, AL_TRUE      );
}

void soundSource::setBuffer(boost::shared_ptr<soundBuffer> sndBuffer)
{
    if (bIsStream)
        throw std::string("soundSource: attempt to single-set buffer on stream source.");

    context->makeCurrent();

    // Clear current error state
    alGetError();

    alSourcei(source, AL_BUFFER, sndBuffer->getALBufferID());

    ALenum alErrNo = alGetError();
    if (alErrNo != AL_NO_ERROR)
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
    context->makeCurrent();

    alDeleteSources(1, &source);    // Should only fail if alGenSources() failed
}

soundSource::sourceState soundSource::getState()
{
    context->makeCurrent();

    // Clear error state
    alGetError();

    ALenum state = AL_SOURCE_STATE;
    alGetSourcei(source, AL_SOURCE_STATE, &state);

    ALenum error = alGetError();
    if (error != AL_NO_ERROR)
        throw std::string("alGetSourcei error: " + to_string(error));

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

void soundSource::queueBuffer(boost::shared_ptr<soundBuffer> sndBuffer)
{
    if (!bIsStream)
        throw std::string("soundSource: attempt to (un)queue buffer to/from non-stream source");

    context->makeCurrent();

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

    buffers.push_back(sndBuffer);
}

boost::shared_ptr<soundBuffer> soundSource::unqueueBuffer()
{
    if (!bIsStream)
        throw std::string("soundSource: attempt to (un)queue buffer to/from non-stream source");

    context->makeCurrent();

    alGetError();   // clear current error state

    ALuint bufferID;
    alSourceUnqueueBuffers(source, 1, &bufferID);

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

    for (std::vector< boost::shared_ptr<soundBuffer> >::iterator i = buffers.begin(); i != buffers.end(); ++i)
    {
        if ((*i)->getALBufferID() == bufferID)
        {
            return *i;
        }
    }

    throw std::string("alSourceUnqueueBuffers(): no buffers to unqueue");
}

void soundSource::play()
{
    context->makeCurrent();

    alSourcePlay(source);
}

void soundSource::stop()
{
    context->makeCurrent();

    alSourceStop(source);
}

unsigned int soundSource::numProcessedBuffers()
{
    context->makeCurrent();

    int count;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &count);

    return count;
}
