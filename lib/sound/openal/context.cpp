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

soundContext::soundContext(boost::shared_ptr<soundDevice> sndDevice) : listener(this), device(sndDevice)
{
    alcGetError(device->getALCDeviceID());

    // Create a new rendering context
    sndContext = alcCreateContext(device->getALCDeviceID(), NULL);

    switch (alcGetError(device->getALCDeviceID()))
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
        alcMakeContextCurrent(NULL);
        alcDestroyContext(sndContext);
        sndContext = NULL;
    }
}

soundContext::soundListener::soundListener(soundContext* sndContext) : context(sndContext)
{
}

void soundContext::soundListener::setRotation(float pitch, float yaw, float roll)
{
    context->makeCurrent();
    // TODO: implement some kind of conversion from pitch, yaw and roll to two "at" and "up" vectors
}

void soundContext::soundListener::setRotation(int pitch, int yaw, int roll)
{
    context->makeCurrent();
    // TODO: implement some kind of conversion from pitch, yaw and roll to two "at" and "up" vectors
}

void soundContext::soundListener::getRotation(float& pitch, float& yaw, float& roll)
{
    context->makeCurrent();
    // TODO: implement some kind of conversion from pitch, yaw and roll to two "at" and "up" vectors
}

void soundContext::soundListener::getRotation(int& pitch, int& yaw, int& roll)
{
    context->makeCurrent();
    // TODO: implement some kind of conversion from pitch, yaw and roll to two "at" and "up" vectors
}
