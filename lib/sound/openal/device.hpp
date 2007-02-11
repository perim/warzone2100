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
#include <AL/al.h>
#include <AL/alc.h>

#include <map>
#include <vector>
#include <string>
#include <boost/smart_ptr.hpp>

#include "../types.h"
#include "context.hpp"

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

        /** Creates a sound rendering context
         *  \return a unique ID number which is to be used as a handle to the soundContext class
         */
        sndContextID createContext();

        /** Return a pointer to the context
         */
        const boost::shared_ptr<soundContext> getContext(sndContextID context);

    public:
        /** Returns a reference to a vector containing a list of devices available for opening
         */
        static std::vector<std::string>& deviceList();

    private:
        // Identifier towards OpenAL
        ALCdevice* sndDevice;

        // Internal data
        std::map<sndContextID, boost::shared_ptr<soundContext> > sndContexts;

        // Internal state
        sndContextID nextContextID;
};

#endif // SOUND_OPENAL_DEVICE_HPP
