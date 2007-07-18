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

#ifndef _INCLUDE_SOUND_OPENAL_EXCEPTION_HPP_
#define _INCLUDE_SOUND_OPENAL_EXCEPTION_HPP_

#include <stdexcept>
#include <string>

#ifndef WZ_NOSOUND
# ifdef WZ_OS_MAC
#  include <OpenAL/al.h>
# else
#  include <AL/al.h>
# endif
#endif

namespace OpenAL
{
    class exception : public std::exception
    {
        public:
            enum errorCodes
            {
                NoError,
                InvalidName,
                InvalidEnum,
                InvalidValue,
                InvalidOperation,
                OutOfMemory,

                undefined,
            };

        public:
            exception(const std::string& desc, ALenum errCode) throw();
            exception(ALenum errCode) throw();
            static exception exception_DescPrefixed(ALenum errCode, const std::string& descPrefix);

            virtual ~exception() throw();

            virtual const char* what() const throw();
            virtual errorCodes errorCode() const throw();

        private:
            std::string _description;
            errorCodes _errorCode;
    };

    class out_of_non_memory_resources : public exception
    {
        public:
            out_of_non_memory_resources(ALenum errCode) throw();
            out_of_non_memory_resources(const std::string& desc, ALenum errCode) throw();
    };
}

#endif // _INCLUDE_SOUND_OPENAL_EXCEPTION_HPP_
