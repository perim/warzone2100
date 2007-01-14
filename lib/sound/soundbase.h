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

#ifndef SOUNDBASE_H
#define SOUNDBASE_H

// Include the OpenAL libraries
#include <AL/al.h>
#include <AL/alc.h>
#include "types.h"

// we're using the vector class from the standard template lib
#include <vector>

class soundBase
{
    public:
        /** Default constructor
         *  This function constructs the soundBase class,
         *  and optionally intializes the sound device already.
         *  \param init whether the default (system/os default) sound device should be initialized at construction.
         */
        soundBase(bool init = false);
        ~soundBase();

        void setListenerPos(float x, float y, float z);
        void setListenerVel(float x, float y, float z);

        /** Sets the listener orientation.
         *  Sets the orientation of the listener to "look" at a certain direction,
         *  see http://en.wikipedia.org/wiki/Flight_dynamics for more information.
         *  \param pitch the "height" the listener is pointed at (i.e. in on screen terms)
         *  \param yaw the orientation to the "left and right" (rotation about vertical axis)
         *  \param roll just see the wikipedia article ;-) it has a nice pic explaining this way better
         */
        void setListenerRot(float pitch, float yaw, float roll);

        void updateStreams();

    private:
        ALCdevice* sndDevice;
        ALCcontext* sndContext;

        bool sndExtEAX;
};

#endif // SOUNDBASE_H
