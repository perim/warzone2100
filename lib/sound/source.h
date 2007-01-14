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

#ifndef SOUND_SOURCE_H
#define SOUND_SOURCE_H

#include <AL/al.h>
#include "buffer.h"

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
        soundSource(soundBuffer* sndBuffer, bool b2D = false);

        ~soundSource();

        bool is2D();
        bool isStream();
        sourceState getState();

        void queueBuffer(soundBuffer* sndBuffer);

    private:
        ALuint source;
        const bool bIs2D;
        const bool bIsStream;
};

#endif // SOUND_SOURCE_H
