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
#include "../general/databuffer.hpp"

namespace OpenAL
{
    // Necessary to be able to declare this class a friend of soundBuffer
    class Source;

    class Buffer
    {
        public:

            /** Creates an OpenAL buffer
             */
            Buffer();
            Buffer(const Sound::DataBuffer& data);
            virtual ~Buffer();

            /** Fills the buffer with the provided data
             *  \param data a buffer containing the sounddata
             */
            void bufferData(const Sound::DataBuffer& data);

            /** Retrieves the duration of this buffer
             *  \return the duration of the sounddata in this buffer, expressed in seconds
             */
            inline float duration() const;

        private:
            // Private copy constructor and copy assignment operator ensures this class cannot be copied
            Buffer( const Buffer& );
            const Buffer& operator=( const Buffer& );

            /** Handles the creation of the buffer
             *  Makes sure an OpenAL buffer is created and related errors are dealt with
             */
            inline void createBuffer();

        private:
            // Internal identifier towards OpenAL
            ALuint buffer;

            // Needed so that Source can attach (or queue/unqueue) Buffer's OpenAL buffer to its OpenAL source
            friend class Source;

        protected:
            float _duration; // duration of sounddata in this buffer in seconds
    };
}

#endif // SOUND_OPENAL_BUFFER_HPP
