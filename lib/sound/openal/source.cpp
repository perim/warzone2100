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
#include "exception.hpp"

namespace OpenAL
{
    Source::Source(boost::shared_ptr<Context> sndContext) :
        _context(sndContext)
    {
        createSource();
    }

    Source::Source(boost::shared_ptr<Context> sndContext, boost::shared_ptr<Buffer> sndBuffer) :
        _context(sndContext)
    {
        createSource();
        setBuffer(sndBuffer);
    }

    Source::~Source()
    {
        _context->makeCurrent();

        alDeleteSources(1, &_source);    // Should only fail if alGenSources() failed
    }

    inline void Source::createSource()
    {
        _context->makeCurrent();

        // Clear current error state
        alGetError();

        // Produce the source
        alGenSources(1, &_source);

        ALenum alErrNo = alGetError();
        if (alErrNo != AL_NO_ERROR)
        {
            switch (alErrNo)
            {
                case AL_INVALID_VALUE:
                    throw OpenAL::out_of_non_memory_resources("OpenAL::Source: alGenSources(): not enough non-memory resources", alErrNo);
                default:
                    throw OpenAL::exception::exception_DescPrefixed(alErrNo, "OpenAL::Source: alGenSources(): ");
            }
        }
    }

    void Source::setBuffer(boost::shared_ptr<Buffer> sndBuffer)
    {
        _context->makeCurrent();

        // Clear current error state
        alGetError();

        alSourcei(_source, AL_BUFFER, sndBuffer->buffer);

        ALenum alErrNo = alGetError();
        if (alErrNo != AL_NO_ERROR)
        {
            switch (alErrNo)
            {
                case AL_INVALID_VALUE:
                    throw OpenAL::out_of_non_memory_resources("OpenAL::Source::setBuffer: alSourcei(): not enough non-memory resources, or invalid pointer", alErrNo);
                default:
                    throw OpenAL::exception::exception_DescPrefixed(alErrNo, "OpenAL::Source.setBuffer: alSource(): ");
            }
        }

        _buffer = sndBuffer;
        _buffers.clear();
    }

    Source::sourceType Source::getType() const
    {
        ALint source_type;
        alGetSourcei(_source, AL_SOURCE_TYPE, &source_type);

        switch (source_type)
        {
            case AL_STATIC:
                return Static;
            case AL_STREAMING:
                return Streaming;
            default:
                return Undetermined;
        }
    }

    bool Source::isStream() const
    {
        return (getType() == Streaming);
    }

    bool Source::isStatic() const
    {
        return (getType() == Static);
    }

    Source::sourceState Source::getState() const
    {
        _context->makeCurrent();

        // Clear error state
        alGetError();

        ALenum state = AL_SOURCE_STATE;
        alGetSourcei(_source, AL_SOURCE_STATE, &state);

        ALenum error = alGetError();
        if (error != AL_NO_ERROR)
            throw OpenAL::exception::exception_DescPrefixed(error, "OpenAL::Source.getState: alGetSourcei error: ");

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
        if (isStatic())
            throw std::runtime_error("OpenAL::Source.queueBuffer: attempt to queue buffer on non-stream source");

        _context->makeCurrent();

        alGetError();   // clear current error state

        ALuint tmpBuffer = sndBuffer->buffer;
        alSourceQueueBuffers(_source, 1, &tmpBuffer);

        ALenum alErrNo = alGetError();
        if (alErrNo != AL_NO_ERROR)
        {
            switch (alErrNo)
            {
                case AL_INVALID_VALUE:
                    throw OpenAL::exception("OpenAL::Source.queueBuffer: alSourceQueueBuffers(): OpenAL: specified buffer is invalid or does not exist.", alErrNo);
                case AL_INVALID_OPERATION:
                    throw OpenAL::exception("OpenAL::Source.queueBuffer: alSourceQueueBuffers(): buffer format mismatched with the rest of queue (i.e. mono8/mono16/stereo8/stereo16 or bad samplerate)", alErrNo);
                default:
                    throw OpenAL::exception::exception_DescPrefixed(alErrNo, "OpenAL::Source.queueBuffer: alSourceQueueBuffers(): ");
            }
        }

        _buffers.push_back(sndBuffer);
    }

    boost::shared_ptr<Buffer> Source::unqueueBuffer()
    {
        if (!isStream())
            throw std::runtime_error("OpenAL::Source.unqueueBuffer: attempt to unqueue buffer from non-stream source");

        _context->makeCurrent();

        alGetError();   // clear current error state

        ALuint bufferID;
        alSourceUnqueueBuffers(_source, 1, &bufferID);

        ALenum alErrNo = alGetError();
        if (alErrNo != AL_NO_ERROR)
        {
            switch (alErrNo)
            {
                case AL_INVALID_VALUE:
                    throw OpenAL::out_of_non_memory_resources("OpenAL::Source::unqueueBuffer: alSourceUnqueueBuffers(): not enough non-memory resources, or invalid pointer", alErrNo);
                default:
                    throw OpenAL::exception::exception_DescPrefixed(alErrNo, "OpenAL::Source.unqueueBuffer: alSourceUnqueueBuffers(): ");
            }
        }

        for (std::vector< boost::shared_ptr<Buffer> >::iterator i = _buffers.begin(); i != _buffers.end(); ++i)
        {
            if ((*i)->buffer == bufferID)
            {
                return *i;
            }
        }

        throw std::runtime_error("OpenAL::Source.unqueueBuffer: alSourceUnqueueBuffers(): no buffers to unqueue");
    }

    void Source::unbuffer()
    {
        sourceState alSourceState = getState();
        if (alSourceState != initial
         && alSourceState != stopped)
            throw std::runtime_error("OpenAL::Source::unbuffer: can't remove a buffer from a source that isn't stopped or \"initial\"");

        // Detach all buffers
        alSourcei(_source, AL_BUFFER, AL_NONE);

        // Remove all our shared pointers to these buffers
        _buffer.reset();
        _buffers.clear();
    }

    bool Source::play()
    {
        _context->makeCurrent();

        // Clear current error state
        alGetError();

        alSourcePlay(_source);

        if(alGetError() != AL_NO_ERROR)
            return false;

        return true;
    }

    void Source::stop()
    {
        _context->makeCurrent();
        alSourceStop(_source);
    }

    void Source::pause()
    {
        _context->makeCurrent();
        alSourcePause(_source);
    }

    void Source::rewind()
    {
        _context->makeCurrent();
        alSourceRewind(_source);
    }

    unsigned int Source::numProcessedBuffers() const
    {
        _context->makeCurrent();

        int count;
        alGetSourcei(_source, AL_BUFFERS_PROCESSED, &count);

        return count;
    }

    void Source::setPosition(float x, float y, float z)
    {
        _context->makeCurrent();
        alSource3f(_source, AL_POSITION, x, y, z);
    }

    void Source::setPosition(int x, int y, int z)
    {
        _context->makeCurrent();
        alSource3i(_source, AL_POSITION, x, y, z);
    }

    void Source::getPosition(float& x, float& y, float& z) const
    {
        _context->makeCurrent();
        alGetSource3f(_source, AL_POSITION, &x, &y, &z);
    }

    void Source::getPosition(int& x, int& y, int& z) const
    {
        _context->makeCurrent();
        alGetSource3i(_source, AL_POSITION, &x, &y, &z);
    }

    // * Is this implementation of setting/getting AL_DIRECTION correct?
    // * Honestly I can't tell, because the OpenAL specification neglects
    // * to mention how the vector of 3 values is used for specifying direction
    void Source::setRotation(float pitch, float yaw, float roll)
    {
        _context->makeCurrent();
        alSource3f(_source, AL_DIRECTION, pitch, yaw, roll);
    }

    void Source::setRotation(int pitch, int yaw, int roll)
    {
        _context->makeCurrent();
        alSource3i(_source, AL_DIRECTION, pitch, yaw, roll);
    }

    void Source::getRotation(float& pitch, float& yaw, float& roll) const
    {
        _context->makeCurrent();
        alGetSource3f(_source, AL_DIRECTION, &pitch, &yaw, &roll);
    }

    void Source::getRotation(int& pitch, int& yaw, int& roll) const
    {
        _context->makeCurrent();
        alGetSource3i(_source, AL_DIRECTION, &pitch, &yaw, &roll);
    }

    void Source::setVelocity(float x, float y, float z)
    {
        _context->makeCurrent();
        alSource3f(_source, AL_VELOCITY, x, y, z);
    }

    void Source::setVelocity(int x, int y, int z)
    {
        _context->makeCurrent();
        alSource3i(_source, AL_VELOCITY, x, y, z);
    }

    void Source::getVelocity(float& x, float& y, float& z) const
    {
        _context->makeCurrent();
        alGetSource3f(_source, AL_VELOCITY, &x, &y, &z);
    }

    void Source::getVelocity(int& x, int& y, int& z) const
    {
        _context->makeCurrent();
        alGetSource3i(_source, AL_VELOCITY, &x, &y, &z);
    }

    void Source::volume(float gain)
    {
        _context->makeCurrent();
        alSourcef(_source, AL_GAIN, gain);
    }

    float Source::volume() const
    {
        _context->makeCurrent();
        float gain;
        alGetSourcef(_source, AL_GAIN, &gain);

        return gain;
    }

    void Source::loop(bool looping)
    {
        _context->makeCurrent();
        alSourcei(_source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
    }

    bool Source::loop() const
    {
        _context->makeCurrent();
        ALint looping;
        alGetSourcei(_source, AL_LOOPING, &looping);
        return (looping == AL_TRUE);
    }
}
