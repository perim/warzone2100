/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Warzone Resurrection Project

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
#include "buffer.hpp"
#include <string>
#include "../templates.hpp"
#include <stdexcept>

namespace OpenAL
{
    Source::Source(boost::shared_ptr<Context> sndContext) :
        context(sndContext),
        bIs2D(false),
        bIsStream(true)
    {
        createSource();
    }

    Source::Source(boost::shared_ptr<Context> sndContext, boost::shared_ptr<Buffer> sndBuffer) :
        context(sndContext),
        bIs2D(false),
        bIsStream(false)
    {
        createSource();
        setBuffer(sndBuffer);
    }

    Source::~Source()
    {
        context->makeCurrent();

        alDeleteSources(1, &source);    // Should only fail if alGenSources() failed
    }

    inline void Source::createSource()
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
                    throw std::runtime_error("OpenAL::Source: alGenSources(): Out of memory");
                case AL_INVALID_VALUE:
                    throw std::runtime_error("OpenAL::Source: alGenSources(): not enough non-memory resources, or invalid pointer");
                case AL_INVALID_OPERATION:
                    throw std::runtime_error("OpenAL::Source: alGenSources(): no context available to create sources in");
            }
        }
    }

    void Source::setBuffer(boost::shared_ptr<Buffer> sndBuffer)
    {
        if (bIsStream)
            throw std::runtime_error("OpenAL::Source: attempt to single-set buffer on stream source.");

        context->makeCurrent();

        // Clear current error state
        alGetError();

        alSourcei(source, AL_BUFFER, sndBuffer->buffer);

        ALenum alErrNo = alGetError();
        if (alErrNo != AL_NO_ERROR)
        {
            switch (alErrNo)
            {
                case AL_OUT_OF_MEMORY:
                    throw std::runtime_error("OpenAL::Source.setBuffer: alSource(): Out of memory");
                case AL_INVALID_VALUE:
                    throw std::runtime_error("OpenAL::Source.setBuffer: alSource(): not enough non-memory resources, or invalid pointer");
            }
        }

        buffer = sndBuffer;
    }

    bool Source::is2D() const
    {
        return bIs2D;
    }

    bool Source::isStream() const
    {
        return bIsStream;
    }

    Source::sourceState Source::getState() const
    {
        context->makeCurrent();

        // Clear error state
        alGetError();

        ALenum state = AL_SOURCE_STATE;
        alGetSourcei(source, AL_SOURCE_STATE, &state);

        ALenum error = alGetError();
        if (error != AL_NO_ERROR)
            throw std::runtime_error("OpenAL::Source.getState: alGetSourcei error: " + to_string(error));

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

    void Source::queueBuffer(boost::shared_ptr<Buffer> sndBuffer)
    {
        if (!bIsStream)
            throw std::runtime_error("OpenAL::Source.queueBuffer: attempt to queue buffer on non-stream source");

        context->makeCurrent();

        alGetError();   // clear current error state

        ALuint tmpBuffer = sndBuffer->buffer;
        alSourceQueueBuffers(source, 1, &tmpBuffer);

        ALenum alErrNo = alGetError();
        if (alErrNo != AL_NO_ERROR)
        {
            switch (alErrNo)
            {
                case AL_INVALID_VALUE:
                    throw std::runtime_error("OpenAL::Source.queueBuffer: alSourceQueueBuffers(): OpenAL: specified buffer is invalid or does not exist.");
                case AL_INVALID_OPERATION:
                    throw std::runtime_error("OpenAL::Source.queueBuffer: alSourceQueueBuffers(): buffer format mismatched with the rest of queue (i.e. mono8/mono16/stereo8/stereo16 or bad samplerate)");
            }
        }

        buffers.push_back(sndBuffer);
    }

    boost::shared_ptr<Buffer> Source::unqueueBuffer()
    {
        if (!bIsStream)
            throw std::runtime_error("OpenAL::Source.unqueueBuffer: attempt to unqueue buffer from non-stream source");

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
                    throw std::runtime_error("OpenAL::Source.unqueueBuffer: alSourceUnqueueBuffers(): not enough non-memory resources, or invalid pointer");
                default:
                    throw std::runtime_error("OpenAL::Source.unqueueBuffer: alSourceUnqueueBuffers(): unknown OpenAL error");
            }
        }

        for (std::vector< boost::shared_ptr<Buffer> >::iterator i = buffers.begin(); i != buffers.end(); ++i)
        {
            if ((*i)->buffer == bufferID)
            {
                return *i;
            }
        }

        throw std::runtime_error("OpenAL::Source.unqueueBuffer: alSourceUnqueueBuffers(): no buffers to unqueue");
    }

    bool Source::play()
    {
        context->makeCurrent();

        // Clear current error state
        alGetError();

        alSourcePlay(source);

        if(alGetError() != AL_NO_ERROR)
            return false;

        return true;
    }

    void Source::stop()
    {
        context->makeCurrent();
        alSourceStop(source);
    }

    unsigned int Source::numProcessedBuffers()
    {
        context->makeCurrent();

        int count;
        alGetSourcei(source, AL_BUFFERS_PROCESSED, &count);

        return count;
    }

    void Source::setPosition(float x, float y, float z)
    {
        context->makeCurrent();
        alSource3f(source, AL_POSITION, x, y, z);
    }

    void Source::setPosition(int x, int y, int z)
    {
        context->makeCurrent();
        alSource3i(source, AL_POSITION, x, y, z);
    }

    void Source::getPosition(float& x, float& y, float& z)
    {
        context->makeCurrent();
        alGetSource3f(source, AL_POSITION, &x, &y, &z);
    }

    void Source::getPosition(int& x, int& y, int& z)
    {
        context->makeCurrent();
        alGetSource3i(source, AL_POSITION, &x, &y, &z);
    }

    // * Is this implementation of setting/getting AL_DIRECTION correct?
    // * Honestly I can't tell, because the OpenAL specification neglects
    // * to mention how the vector of 3 values is used for specifying direction
    void Source::setRotation(float pitch, float yaw, float roll)
    {
        context->makeCurrent();
        alSource3f(source, AL_DIRECTION, pitch, yaw, roll);
    }

    void Source::setRotation(int pitch, int yaw, int roll)
    {
        context->makeCurrent();
        alSource3i(source, AL_DIRECTION, pitch, yaw, roll);
    }

    void Source::getRotation(float& pitch, float& yaw, float& roll)
    {
        context->makeCurrent();
        alGetSource3f(source, AL_DIRECTION, &pitch, &yaw, &roll);
    }

    void Source::getRotation(int& pitch, int& yaw, int& roll)
    {
        context->makeCurrent();
        alGetSource3i(source, AL_DIRECTION, &pitch, &yaw, &roll);
    }

    void Source::setVelocity(float x, float y, float z)
    {
        context->makeCurrent();
        alSource3f(source, AL_VELOCITY, x, y, z);
    }

    void Source::setVelocity(int x, int y, int z)
    {
        context->makeCurrent();
        alSource3i(source, AL_VELOCITY, x, y, z);
    }

    void Source::getVelocity(float& x, float& y, float& z)
    {
        context->makeCurrent();
        alGetSource3f(source, AL_VELOCITY, &x, &y, &z);
    }

    void Source::getVelocity(int& x, int& y, int& z)
    {
        context->makeCurrent();
        alGetSource3i(source, AL_VELOCITY, &x, &y, &z);
    }
}
