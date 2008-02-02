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

#include "exception.hpp"

namespace OpenAL
{
    exception::exception(ALenum errCode) throw()
    {
        switch (errCode)
        {
            case AL_NO_ERROR:
                _errorCode = NoError;
                _description = "There is no current error.";
                break;

            case AL_INVALID_NAME:
                _errorCode = InvalidName;
                _description = "Invalid name parameter.";
                break;

            case AL_INVALID_ENUM:
                _errorCode = InvalidEnum;
                _description = "Invalid parameter.";
                break;

            case AL_INVALID_VALUE:
                _errorCode = InvalidValue;
                _description = "Invalid enum parameter value.";
                break;

            case AL_INVALID_OPERATION:
                _errorCode = InvalidOperation;
                _description = "Illegal call";
                break;

            case AL_OUT_OF_MEMORY:
                _errorCode = OutOfMemory;
                _description = "Unable to allocate memory.";
                break;

            default:
                _errorCode = undefined;
                break;
        }
    }

    exception::exception(const std::string& desc, ALenum errCode) throw() :
        _description(desc)
    {
        switch (errCode)
        {
            case AL_NO_ERROR:
                _errorCode = NoError;
                break;

            case AL_INVALID_NAME:
                _errorCode = InvalidName;
                break;

            case AL_INVALID_ENUM:
                _errorCode = InvalidEnum;
                break;

            case AL_INVALID_VALUE:
                _errorCode = InvalidValue;
                break;

            case AL_INVALID_OPERATION:
                _errorCode = InvalidOperation;
                break;

            case AL_OUT_OF_MEMORY:
                _errorCode = OutOfMemory;
                break;

            default:
                _errorCode = undefined;
                break;
        }
    }

    exception exception::exception_DescPrefixed(ALenum errCode, const std::string& descPrefix)
    {
        exception newException(errCode);
        newException._description = descPrefix + newException._description;
        return newException;
    }

    exception::~exception() throw()
    {
    }

    const char* exception::what() const throw()
    {
        return _description.c_str();
    }

    exception::errorCodes exception::errorCode() const throw()
    {
        return _errorCode;
    }

    out_of_non_memory_resources::out_of_non_memory_resources(ALenum errCode) throw() :
        exception("Out of non-memory, possibly OpenAL (implementation) specific, resources.", errCode)
    {
    }

    out_of_non_memory_resources::out_of_non_memory_resources(const std::string& desc, ALenum errCode) throw() :
        exception(desc, errCode)
    {
    }
}
