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

#include "devicelist.hpp"

// Initialize singleton pointer to zero
soundDeviceList* soundDeviceList::_instance = 0;

// Device enumeration stuff
soundDeviceList::soundDeviceList() : _cArray(0)
{
    // This function call should never fail according to the OpenAL specs (not with these params)
    // So as long as std::vector doesn't throw any exceptions there should occur no problems
    const char* DeviceList = static_cast<const char*>(alcGetString(NULL, ALC_DEVICE_SPECIFIER));

    // Default NULL device (which might very well be /dev/null)
    push_back(std::string());

    if (DeviceList)
    {
        // The returned C-string DeviceList has its entries separated by one NUL char (0x00),
        // and the list itself is terminated by two NUL chars.
        while (*DeviceList != 0x00)
        {
            // Append the current list entry to the list
            push_back(std::string(DeviceList));

            // Move to the next entry in the list
            // strlen really only detects the position of the first NUL char in the array
            DeviceList += strlen(DeviceList) + 1;
        }
    }
    else
    {
        //fprintf(stderr, "soundLib: alcGetString returning NULL; ALC_ERROR: %d\n", alcGetError(NULL));
    }

    _cArray = new _CArray(*this);
}

soundDeviceList::~soundDeviceList()
{
    if (_cArray)
        delete _cArray;
}

const soundDeviceList& soundDeviceList::Instance()
{
    if (_instance == 0)
    {
        _instance = new soundDeviceList;
    }

    return *_instance;
}

void soundDeviceList::DestroyInstance()
{
    if (_instance)
    {
        delete _instance;
        _instance = 0;
    }
}

soundDeviceList::_CArray::_CArray(std::vector<std::string>& _arr) : _cArray(new const char*[_arr.size() + 1])
{
    // Allocate memory
    //char** tmpArray = new char*[_arr.size() + 1];

    // Mark the end of the array
    _cArray[_arr.size()] = NULL;

    std::vector<std::string>::iterator sourceIter;
    const char** targetIter;
    for (sourceIter = _arr.begin(), targetIter = _cArray; sourceIter != _arr.end() && *targetIter != NULL; ++sourceIter, ++targetIter)
    {
        // Allocate memory
        *targetIter = new char[sourceIter->length() + 1];

        // Mark the end of the C-string
        const_cast<char*>(*targetIter)[sourceIter->length()] = 0;

        // Insert data
        memcpy(const_cast<char*>(*targetIter), sourceIter->c_str(), sourceIter->length());
    }
}

soundDeviceList::_CArray::~_CArray()
{
    // Run through list and delete its contents (i.e. free memory of its contents)
    for (const char** iter = _cArray; *iter != NULL; ++iter)
        delete [] *iter;

    delete [] _cArray;
    _cArray = 0;
}
