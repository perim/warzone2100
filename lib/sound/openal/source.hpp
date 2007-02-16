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

#ifndef SOUND_OPENAL_SOURCE_HPP
#define SOUND_OPENAL_SOURCE_HPP

#include <AL/al.h>
#include <vector>
#include <boost/smart_ptr.hpp>
#include "context.hpp"
#include "buffer.hpp"

class soundSource
{
    public:

        enum sourceState {
            initial,
            playing,
            paused,
            stopped,

            undefined
        };

    public:

        // Constructors

        /** For the creation of a multibuffer, streaming, buffer
         *  \param sndContext the context in which sound from this source should be rendered
         *  \param b2D wether to play as a 2D sound (without distance attenuation, doppler effect, etc.)
         */
        soundSource(boost::shared_ptr<soundContext> sndContext, bool b2D = false);

        /** For the creation of a single-buffer source (mostly SFX, or other non-streaming sounds)
         *  \param sndContext the context in which sound from this source should be rendered
         *  \param sndBuffer the buffer to play from
         *  \param b2D       wether to play as a 2D sound (without distance attenuation, doppler effect, etc.)
         */
        soundSource(boost::shared_ptr<soundContext> sndContext, boost::shared_ptr<soundBuffer> sndBuffer, bool b2D = false);

        ~soundSource();

        /** Sets the source's buffer.
         *  In case of a non streaming source this function sets the buffer for the source.
         *  \throw std::string containing error message on failure
         */
        void setBuffer(boost::shared_ptr<soundBuffer> sndBuffer);

        /** Wether all sound routed through this source will be rendered as 2D.
         *  \return true if all routed sound is displayed 2D, false otherwise
         */
        inline bool is2D()
        {
            return bIs2D;
        }

        /** Wether this source is used for streaming sound
         *  \return true if this source uses buffer queues to stream from, false otherwise
         */
        inline bool isStream()
        {
            return bIsStream;
        }

        /** Retrieve the currect state of the source
         *  \return an enum representing the current play-state of the source
         */
        sourceState getState();

        /** Append a buffer to the end of the queue
         *  \param sndBuffer buffer to append to the end of the queue
         */
        void queueBuffer(boost::shared_ptr<soundBuffer> sndBuffer);

        /** Remove a finished buffer from the top of the queue
         *  \return the buffer that was unqueued
         */
        boost::shared_ptr<soundBuffer> unqueueBuffer();

        /** Tells OpenAL to start playing
         */
        void play();

        /** Tells OpenAL to stop playing
         */
        void stop();

        unsigned int numProcessedBuffers();

    private:
        /** Handles the creation of the source
         *  Makes sure an OpenAL source is created and related errors are dealt with
         */
        inline void createSource();

    private:
        // Identifier towards OpenAL
        ALuint source;

        // Internal data
        boost::shared_ptr<soundContext> context;
        std::vector< boost::shared_ptr<soundBuffer> > buffers;

        // Internal state
        const bool bIs2D;
        const bool bIsStream;
};

#endif // SOUND_OPENAL_SOURCE_HPP
