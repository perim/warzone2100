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

#include "context.hpp"
#include <string>

static const float Pi = 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679;

soundContext::soundContext(ALCdevice* sndDevice, bool set2D) : is2D(set2D)
{
    alcGetError(sndDevice);

    // Create a new rendering context
    sndContext = alcCreateContext(sndDevice, NULL);
    makeCurrent();

    switch (alcGetError(sndDevice))
    {
        case ALC_NO_ERROR:
            return;
        case ALC_INVALID_VALUE:
            throw std::string("An additional context cannot be created for this device.");
        case ALC_INVALID_DEVICE:
            throw std::string("The specified device is not a valid output device.");
    }
}

soundContext::~soundContext()
{
    if ( sndContext != NULL )
    {
        makeCurrent();
        alcDestroyContext(sndContext);
        sndContext = NULL;
    }
}

void soundContext::setListenerPos(float x, float y, float z)
{
    if (sndContext)
    {
        makeCurrent();
        alListener3f( AL_POSITION, x, y, z );
    }
    // else throw an exception
}

void soundContext::setListenerVel(float x, float y, float z)
{
    if (sndContext)
    {
        makeCurrent();
        alListener3f( AL_VELOCITY, x, y, z );
    }
    // else throw an exception
}

void soundContext::setListenerRot(float pitch, float yaw, float roll)
{
    if (sndContext)
    {
        makeCurrent();
        // TODO: implement some kind of conversion from pitch, yaw and roll to two "at" and "up" vectors
    }
    // else throw an exception
}

void soundContext::updateStreams()
{
    for ( std::map<sndStreamID, boost::shared_ptr<soundStream> >::iterator i = sndStreams.begin(); i != sndStreams.end(); ++i )
    {
        makeCurrent();
        i->second->update();
    }
}
