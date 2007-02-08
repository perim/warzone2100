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
         *  \param b2D wether to play as a 2D sound (without distance attenuation, doppler effect, etc.)
         */
        soundSource(bool b2D = false);

        /** For the creation of a single-buffer source (mostly SFX, or other non-streaming sounds)
         *  \param sndBuffer the buffer to play from
         *  \param b2D       wether to play as a 2D sound (without distance attenuation, doppler effect, etc.)
         */
        soundSource(boost::shared_ptr<soundBuffer> sndBuffer, bool b2D = false);

        ~soundSource();

        bool is2D();
        bool isStream();
        sourceState getState();

        void queueBuffer(boost::shared_ptr<soundBuffer> sndBuffer);
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
         *  Makes sure an OpenAL source is created and connected errors are dealt with
         */
        inline void createSource();

    private:
        // Identifier towards OpenAL
        ALuint source;

        // Internal data
        std::vector< boost::shared_ptr<soundBuffer> > buffers;

        // Internal state
        const bool bIs2D;
        const bool bIsStream;
};

#endif // SOUND_OPENAL_SOURCE_HPP
