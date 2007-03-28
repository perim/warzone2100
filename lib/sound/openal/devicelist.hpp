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

#ifndef SOUND_OPENAL_DEVICELIST_HPP
#define SOUND_OPENAL_DEVICELIST_HPP

// Include the OpenAL libraries
#include <AL/alc.h>

#include <vector>
#include <string>

// Singleton class
class soundDeviceList : public std::vector<std::string>
{
    public:
        /** Provides a reference to an instance of soundDeviceList
         *  If there currently exists no instance of soundDeviceList it creates one
         *  \return a reference to a singleton instance of soundDeviceList
         */
        static const soundDeviceList& Instance();

        /** Destroys the singleton instance of soundDeviceList if it exists
         */
        static void DestroyInstance();

    protected:
        soundDeviceList();

    private:
        // Private copy constructor and copy assignment operator ensures this class cannot be copied
        soundDeviceList( const soundDeviceList& );
        const soundDeviceList& operator=( const soundDeviceList& );

    private:
        // Singleton instance pointer
        static soundDeviceList* _instance;
};

#endif // SOUND_OPENAL_DEVICELIST_HPP
