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

#include <sstream>

template <class T>
inline std::string to_string (const T& t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

/** Dual iterating for_each implementation
 *  This implementation maintains two separate iterators and increments them simultaniously.
 *  Further it passes both to the provided function
 *  \return object of type Function (can be a function pointer, or in case of a functor the object itself)
 */
template<class InputIterator, class OutputIterator, class Function>
Function for_each2(InputIterator inFirst, InputIterator inLast, OutputIterator outFirst, OutputIterator outLast, Function f)
{
    while ( inFirst != inLast && outFirst != outLast)
        f(*inFirst++, *outFirst++);
    return f;
}
