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

#include "buffer.hpp"
#include <string>
#include <stdexcept>

namespace OpenAL
{
    Buffer::Buffer() :
        _duration(0)
    {
        createBuffer();
    }

    Buffer::Buffer(const Sound::DataBuffer& data) :
        _duration(0)
    {
        createBuffer();
        bufferData(data);
    }

    inline void Buffer::createBuffer()
    {
        // Clear error state
        alGetError();

        alGenBuffers(1, &buffer);

        if (alGetError() != AL_NO_ERROR)
            throw std::runtime_error("OpenAL::Buffer.createBuffer: alGenBuffers error, possibly out of memory");
    }

    Buffer::~Buffer()
    {
        alDeleteBuffers(1, &buffer);
    }

    void Buffer::bufferData(const Sound::DataBuffer& data)
    {
        // Clear error state
        alGetError();

        alBufferData(buffer, ((data.channelCount() == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16), data, data.size(), data.frequency());

        switch (alGetError())
        {
            case AL_INVALID_VALUE:
                throw std::runtime_error("OpenAL::Buffer.bufferData: alBufferData error: invalid size parameter, buffer is in use or NULL pointer passed as data");
        }

        unsigned int samplecount = data.size() / data.channelCount() / data.bytesPerSample();
        _duration = samplecount / data.frequency();
    }

    inline float Buffer::duration() const
    {
        return _duration;
    }
}
