/*
* This file is part of Warzone 2100, an open-source, cross-platform, real-time strategy game
* Copyright (C) 2007  Giel van Schijndel, Warzone Ressurection Project
*
* This program is distributed under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
*
* $Revision$
* $Id$
* $HeadURL$
*/

#ifndef SOUND_STREAM_H
#define SOUND_STREAM_H

#include "buffer.h"
#include "source.h"

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
         *  \return a pointer to the soundSource used by this stream
         */
        soundSource* getSource();

        /** keep all required buffers filled
         *  needs to be called more often if buffer size is smaller, and less often if larger
         *  \return true if there is still enough data left to fill all buffers, false otherwise
         */
        bool update();

        void setBufferSize(unsigned int size);
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
