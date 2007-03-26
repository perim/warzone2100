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

#include "stringarray.hpp"

// FIXME: global, get rid of it (probably a functor/function-object will do best)
static const char** targetIter;
static inline void _Append(const std::string& source)
{
    // Allocate memory
    *targetIter = new char[source.length() + 1];

    // Mark the end of the C-string
    const_cast<char*>(*targetIter)[source.length()] = 0;

    // Insert data
    memcpy(const_cast<char*>(*targetIter), source.c_str(), source.length());

    ++targetIter;
}

interfaceUtil::CArray::CArray(const std::vector<std::string>& _arr) : _cArray(new const char*[_arr.size() + 1])
{
    // Mark the end of the array
    _cArray[_arr.size()] = NULL;

    targetIter = _cArray;
    for_each(_arr.begin(), _arr.end(), _Append);
}

interfaceUtil::CArray::~CArray()
{
    // Run through list and delete its contents (i.e. free memory of its contents)
    for (const char** iter = _cArray; *iter != NULL; ++iter)
        delete [] *iter;

    delete [] _cArray;
    _cArray = 0;
}
