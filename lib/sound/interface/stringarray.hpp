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

#ifndef SOUND_INTERFACE_STRINGARRAY_HPP
#define SOUND_INTERFACE_STRINGARRAY_HPP

#include <cassert>
#include <algorithm>
#include <stdexcept>

namespace Sound
{
    namespace Interface
    {
        class StringArray
        {
            public:
                /** Constructs a C-style array of C strings
                 *  \param first,last a pair of input iterators which point to
                 *         a range of strings. The strings will have to be
                 *         container-like objects of char's with at least the
                 *         member functions size(), begin() and end(). E.g.
                 *         std::string will suffice.
                 */
                template <typename InputIterator>
                StringArray(InputIterator first, InputIterator last)
                {
                    // Determine the amount of strings to store
                    std::size_t count = std::distance(first, last);

                    // Allocate memory to hold pointers to all strings plus a
                    // terminating NULL pointer.
                    _cArray = new const char*[count + 1];

                    // Set an "iterator" to the first C string
                    const char** curString = _cArray;

                    try
                    {
                        // Copy all strings from the given iterator range into our
                        // string array
                        for (; first != last; ++first, ++curString)
                        {
                            assert(count-- > 0);

                            char*& target = const_cast<char*&>(*curString);
                            // Initialise this pointer to zero. This is to make
                            // sure that if an exception is thrown we can still
                            // delete [] curString without having to worry about
                            // deleting a possibly uninitialised pointer.
                            target = 0;

                            // Determine the length of the string
                            const std::size_t length = first->size();

                            // Allocate memory for the string
                            target = new char[length + 1];

                            // Copy over the string
                            std::copy(first->begin(), first->end(), target);

                            // Mark the end of the C string
                            target[length] = '\0';
                        }
                    }
                    // Don't leak memory when exceptions are thrown
                    catch (...)
                    {
                        // Clean up the memory we've allocated already
                        for (const char* i = _cArray[0]; i != *curString; ++i)
                            delete [] i;

                        delete [] curString;
                        delete [] _cArray;

                        // Rethrow the exception
                        throw;
                    }

                    // Mark the end of the array
                    // @note Not using NULL here because that's not portable C++.
                    *curString = 0;
                }

                ~StringArray();

                /** Syntactic sugar to allow instances of this class to be
                 *  used directly as C string arrays.
                 */
                inline operator const char**() const
                {
                    return _cArray;
                }

                /** Retrieve the array of C strings.
                 */
                inline const char** get() const
                {
                    return _cArray;
                }

            private:
                const char** _cArray;
        };
    }
}

#endif // SOUND_INTERFACE_STRINGARRAY_HPP
