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

#ifndef SOUND_OPENAL_BUFFER_HPP
#define SOUND_OPENAL_BUFFER_HPP

#include <AL/al.h>

// Needed to be able to declare this class a friend of soundBuffer
class soundSource;

class soundBuffer
{
    public:

        /** Creates an OpenAL buffer
         */
        soundBuffer();
        ~soundBuffer();

        /** Fills the buffer with the provided data
         *  \param channels the amount of channels provided in the data stream
         *  \param frequency the sample frequency of the provided data
         *  \param data a pointer to an array containing the sound data
         *  \param size the size of the data array in bytes
         */
        void bufferData(unsigned int channels, unsigned int frequency, const char* data, unsigned int size);

    private:
        // Private copy constructor and copy assignment operator ensures this class cannot be copied
        soundBuffer( const soundBuffer& );
        const soundBuffer& operator=( const soundBuffer& );

    private:
        // Internal identifier towards OpenAL
        ALuint buffer;

        // Needed so that soundSource can attach (or queue/unqueue) soundBuffer's OpenAL buffer to its OpenAL source
        friend class soundSource;
};

#endif // SOUND_OPENAL_BUFFER_HPP
