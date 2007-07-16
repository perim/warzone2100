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

#include "context.hpp"
#include <string>
#include <stdexcept>

namespace OpenAL
{
    Context::Context(boost::shared_ptr<Device> sndDevice) :
        listener(*this),
        _device(sndDevice)
    {
        // Clear error condition
        alcGetError(_device->_device);

        // Create a new rendering context
        _context = alcCreateContext(_device->_device, NULL);

        switch (alcGetError(_device->_device))
        {
            case ALC_NO_ERROR:
                return;
            case ALC_INVALID_VALUE:
                throw std::runtime_error("OpenAL::Context: An additional context cannot be created for this device.");
            case ALC_INVALID_DEVICE:
                throw std::runtime_error("OpenAL::Context: The specified device is not a valid output device.");
        }
    }

    Context::~Context()
    {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(_context);
    }

    Context::Listener::Listener(Context& sndContext) :
        _context(sndContext)
    {
    }

    void Context::Listener::setRotation(float pitch, float yaw, float roll)
    {
        _context.makeCurrent();
        // TODO: implement some kind of conversion from pitch, yaw and roll to two "at" and "up" vectors
    }

    void Context::Listener::setRotation(int pitch, int yaw, int roll)
    {
        _context.makeCurrent();
        // TODO: implement some kind of conversion from pitch, yaw and roll to two "at" and "up" vectors
    }

    void Context::Listener::getRotation(float& pitch, float& yaw, float& roll)
    {
        _context.makeCurrent();
        // TODO: implement some kind of conversion from pitch, yaw and roll to two "at" and "up" vectors
    }

    void Context::Listener::getRotation(int& pitch, int& yaw, int& roll)
    {
        _context.makeCurrent();
        // TODO: implement some kind of conversion from pitch, yaw and roll to two "at" and "up" vectors
    }
}
