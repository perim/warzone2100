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

#include "device.hpp"
#include <string>

soundDevice::soundDevice() : nextContextID(0)
{
    sndDevice = alcOpenDevice(NULL);

    if (sndDevice == NULL)
        throw std::string("Unable to open audio device.");
}

soundDevice::soundDevice(const std::string deviceName) : nextContextID(0)
{
    if (deviceName == std::string())
        sndDevice = alcOpenDevice(NULL);
    else
        sndDevice = alcOpenDevice(deviceName.c_str());

    if (sndDevice == NULL)
        throw std::string("Unable to open audio device.");
}

soundDevice::~soundDevice()
{
    alcCloseDevice(sndDevice);
}

sndContextID soundDevice::createContext()
{
    sndContexts.insert(std::pair<sndContextID, boost::shared_ptr<soundContext> >(nextContextID, boost::shared_ptr<soundContext>(new soundContext(sndDevice))));
    return nextContextID++;
}

boost::shared_ptr<soundContext> soundDevice::getContext(sndContextID context)
{
    return sndContexts[context];
}

// Device enumeration stuff
static std::vector<std::string> Devices;
std::vector<std::string>& soundDevice::deviceList()
{
    if (Devices.empty())
    {
        // This function call should never fail according to the OpenAL specs (not with these params)
        // So as long as std::vector doesn't throw any exceptions there should occur no problems
        const char* DeviceList = static_cast<const char*>(alcGetString(NULL, ALC_DEVICE_SPECIFIER));

        // Default NULL device
        Devices.push_back(std::string());

        if (DeviceList == NULL)
        {
            fprintf(stderr, "soundLib: alcGetString returning NULL; ALC_ERROR: %d\n", alcGetError(NULL));
        }
        else
        {
            // The returned C-string DeviceList has its entries separated by one NUL char (0x00),
            // and the list itself is terminated by two NUL chars.
            while (*DeviceList != 0x00)
            {
                // Append the current list entry to the list
                Devices.push_back(std::string(DeviceList));

                // Move to the next entry in the list
                // strlen really only detects the position of the first NUL char in the array
                DeviceList += strlen(DeviceList) + 1;
            }
        }
    }

    return Devices;
}
