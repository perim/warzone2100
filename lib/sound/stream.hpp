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

#ifndef SOUND_STREAM_H
#define SOUND_STREAM_H

#include "openal/buffer.hpp"
#include "openal/source.hpp"

class soundStream
{
    public:

        /** Creates and sets up all resources required for a sound stream
         *  \param b2D wether to play as a 2D sound (without distance attenuation, doppler effect, etc.)
         */
        soundStream(bool b2D = false);
        ~soundStream();

        /** Returns a handle to the OpenAL source which is being streamed from
         *  This should only be used to set positional, directional and doppler data with
         *  \return a pointer to the soundSource used by this stream if the stream isn't 2D, throws an exception otherwise
         */
        soundSource* getSource();

        /** keep all required buffers filled
         *  needs to be called more often if buffer size is smaller, and less often if larger
         *  \return true if there is still enough data left to fill all buffers, false otherwise
         */
        bool update();

        /** sets the buffersize for the streaming buffers
         *  \param size size of the buffers in bytes, minimum is 4096, which is still not recommended, 4096 is very likely to introduce clipping
         */
        void setBufferSize(unsigned int size);

        /** returns the currently used buffer size
         *  \return the currently used buffer size in bytes
         */
        unsigned int getBufferSize();

    protected:

        /** fill buffer with the next data
         *  \param buffer buffer to fill
         *  \return       true if the buffer is filled, false if there was no data left to fill a buffer with
         */
        bool stream(soundBuffer* buffer);

        soundSource* source;        // sound source (i.e. in-game)
        soundBuffer  buffers[2];    // front and back buffers (i.e. in the buffer-queue)

        unsigned int bufferSize;    // size of the buffers in bytes/octets, default is 16384 bytes or 16kB
};

#endif // SOUND_STREAM_H
