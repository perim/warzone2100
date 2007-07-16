/*
	This file is part of Warzone 2100.
	Copyright (C) 2007  Warzone Resurrection Project

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
#ifndef WZ_NOSOUND
# ifdef WZ_OS_MAC
#  include <OpenAL/alc.h>
# else
#  include <AL/alc.h>
# endif
#endif

#include <string>

namespace OpenAL
{
    // Forward declaration so that we can declare this class a friend
    class Context;

    class Device
    {
        public:
            /** Default constructor
             *  Constructs by opening specified device
             *  \param deviceName the name of the device to open with OpenAL,
             *         use NULL to open the default device (or give no parameter
             *         to select the default parameter which is NULL)
             *  \throw std::runtime_error object with error messsage on failure
             */
            Device(const char* deviceName = 0);
            ~Device();

        private:
            // Private copy constructor and copy assignment operator ensures this class cannot be copied
            Device( const Device& );
            const Device& operator=( const Device& );

        private:
            // Identifier towards OpenAL
            ALCdevice* _device;

            friend class Context;
    };
}

#endif // SOUND_OPENAL_DEVICE_HPP
