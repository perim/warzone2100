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

#ifndef __INCLUDED_LIB_SOUND_GENERAL_SOUNDDATABUFFER_HPP__
#define __INCLUDED_LIB_SOUND_GENERAL_SOUNDDATABUFFER_HPP__

#include <vector>

namespace Sound
{
    class DataBuffer : public std::vector<char>
    {
        public:
            /** Constructor
             *  \param size the initial size of the buffer (can be changed using vector's member functions)
             *  \param bits sample size in bits
             *  \param channels the amount of channels contained
             *  \param sampleRate the frequency of samples per second
             */
            DataBuffer(const std::size_t size, const unsigned int bits, const unsigned int channels, const unsigned int sampleRate);

            /** Conversion operator
             *  Enables this class to be used directly as if it where an array of chars: char*
             */
            operator const char* () const;

            /** \return the amount of bits per sample
             */
            unsigned int bitsPerSample() const;

            /** \return the amount of bytes per sample
             */
            unsigned int bytesPerSample() const;

            /** \return the amount of channels contained
             */
            unsigned int channelCount() const;

            /** \return the sample frequency
             */
            unsigned int frequency() const;

        private:
            unsigned int _bitsPerSample;
            unsigned int _channels;
            unsigned int _frequency;
    };
}

#endif // __INCLUDED_LIB_SOUND_GENERAL_SOUNDDATABUFFER_HPP__
