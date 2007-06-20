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

namespace OpenAL
{
    soundBuffer::soundBuffer() : _duration(0)
    {
        createBuffer();
    }

    soundBuffer::soundBuffer(const soundDataBuffer& data) : _duration(0)
    {
        createBuffer();
        bufferData(data);
    }

    inline void soundBuffer::createBuffer()
    {
        // Clear error state
        alGetError();

        alGenBuffers(1, &buffer);

        if (alGetError() != AL_NO_ERROR)
            throw std::string("soundBuffer: alGenBuffers error, possibly out of memory");
    }

    soundBuffer::~soundBuffer()
    {
        alDeleteBuffers(1, &buffer);
    }

    void soundBuffer::bufferData(const soundDataBuffer& data)
    {
        // Clear error state
        alGetError();

        alBufferData(buffer, ((data.channelCount() == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16), data, data.size(), data.frequency());

        switch (alGetError())
        {
            case AL_INVALID_VALUE:
                throw std::string("soundBuffer: alBufferData error: invalid size parameter, buffer is in use or NULL pointer passed as data");
        }

        unsigned int samplecount = data.size() / data.channelCount() / data.bytesPerSample();
        _duration = samplecount / data.frequency();
    }

    inline float soundBuffer::duration() const
    {
        return _duration;
    }
}
