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

#ifndef SOUND_OPENAL_DEVICE_HPP
#define SOUND_OPENAL_DEVICE_HPP

// Include the OpenAL libraries
#include <AL/alc.h>

#include <string>

class soundDevice
{
    public:
        /** Default constructor
         *  This function constructs the soundDevice class,
         *  and intializes the default sound device.
         */
        soundDevice();

        /** Constructs by opening specified device
         *  \param deviceName the name of the device to open with OpenAL
         *  \throw std::string object with error messsage on failure
         */
        soundDevice(const std::string deviceName);
        ~soundDevice();

        inline ALCdevice* getALCDeviceID()
        {
            return sndDevice;
        }

    private:
        // Private copy constructor and copy assignment operator ensures this class cannot be copied
        soundDevice( const soundDevice& );
        const soundDevice& operator=( const soundDevice& );

    private:
        // Identifier towards OpenAL
        ALCdevice* sndDevice;
};

#endif // SOUND_OPENAL_DEVICE_HPP
