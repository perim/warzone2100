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

#include "databuffer.hpp"

namespace Sound
{
    DataBuffer::DataBuffer(const std::size_t size,
                           const unsigned int bits,
                           const unsigned int channels,
                           const unsigned int sampleRate) :
        std::vector<char>(size),
        _bitsPerSample(bits),
        _channels(channels),
        _frequency(sampleRate)
    {
    }

    DataBuffer::operator const char* () const
    {
        return &*begin();
    }

    unsigned int DataBuffer::bitsPerSample() const
    {
        return _bitsPerSample;
    }

    unsigned int DataBuffer::bytesPerSample() const
    {
        return _bitsPerSample / 8 + ((_bitsPerSample % 8) ? 1 : 0);
    }

    unsigned int DataBuffer::channelCount() const
    {
        return _channels;
    }

    unsigned int DataBuffer::frequency() const
    {
        return _frequency;
    }
}
