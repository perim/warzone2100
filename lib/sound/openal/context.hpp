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

#ifndef SOUND_OPENAL_CONTEXT_HPP
#define SOUND_OPENAL_CONTEXT_HPP

// Include the OpenAL libraries
#include <AL/al.h>
#include <AL/alc.h>

#include "device.hpp"

#include <boost/smart_ptr.hpp>
#include "../general/geometry.hpp"

namespace OpenAL
{
    class Context
    {
        public:
            /** Default constructor
             *  This function constructs the Context class,
             *  and optionally intializes the sound device already.
             *  \param init whether the default (system/os default) sound device should be initialized at construction.
             */
            Context(boost::shared_ptr<Device> sndDevice);
            ~Context();


            /** makes this soundContext current
             *  Uses OpenAL function alcMakeContextCurrent to make this function current.
             *  This function and it's effect should be used as few as possible (if at all).
             */
            inline void makeCurrent()
            {
                alcMakeContextCurrent(sndContext);
            }

        public:
            class Listener : public Geometry
            {
                public:
                    Listener(Context* sndContext);

                    /** Sets the position of the listener
                     *  \param x X-coordinate of listener
                     *  \param y Y-coordinate of listener
                     *  \param z Z-coordinate of listener
                     */
                    inline virtual void setPosition(float x, float y, float z)
                    {
                        context->makeCurrent();
                        alListener3f(AL_POSITION, x, y, z);
                    }
                    inline virtual void setPosition(int x, int y, int z)
                    {
                        context->makeCurrent();
                        alListener3i(AL_POSITION, x, y, z);
                    }

                    /** Retrieves the position of the listener
                     *  \param x this will be used to return the X-coordinate in
                     *  \param y this will be used to return the Y-coordinate in
                     *  \param z this will be used to return the Z-coordinate in
                     */
                    inline virtual void getPosition(float& x, float& y, float& z)
                    {
                        context->makeCurrent();
                        alGetListener3f(AL_POSITION, &x, &y, &z);
                    }
                    inline virtual void getPosition(int& x, int& y, int& z)
                    {
                        context->makeCurrent();
                        alGetListener3i(AL_POSITION, &x, &y, &z);
                    }

                    /** Sets the listener orientation.
                     *  Sets the orientation of the listener to "look" at a certain direction,
                     *  see http://en.wikipedia.org/wiki/Flight_dynamics for more information.
                     *  \param pitch the "height" the listener is pointed at (i.e. in on screen terms)
                     *  \param yaw the orientation to the "left and right" (rotation about vertical axis)
                     *  \param roll just see the wikipedia article ;-) it has a nice pic explaining this way better
                     */
                    virtual void setRotation(float pitch, float yaw, float roll);
                    virtual void setRotation(int pitch, int yaw, int roll);

                    virtual void getRotation(float& pitch, float& yaw, float& roll);
                    virtual void getRotation(int& pitch, int& yaw, int& roll);


                    inline virtual void setVelocity(float x, float y, float z)
                    {
                        context->makeCurrent();
                        alListener3f(AL_VELOCITY, x, y, z);
                    }
                    inline virtual void setVelocity(int x, int y, int z)
                    {
                        context->makeCurrent();
                        alListener3i(AL_VELOCITY, x, y, z);
                    }

                    inline virtual void getVelocity(float& x, float& y, float& z)
                    {
                        context->makeCurrent();
                        alGetListener3f(AL_VELOCITY, &x, &y, &z);
                    }
                    inline virtual void getVelocity(int& x, int& y, int& z)
                    {
                        context->makeCurrent();
                        alGetListener3i(AL_VELOCITY, &x, &y, &z);
                    }

                private:
                    Context* context;
            };

            Listener listener;

        private:
            // Private copy constructor and copy assignment operator ensures this class cannot be copied
            Context( const Context& );
            const Context& operator=( const Context& );

        private:
            // Identifier towards OpenAL
            ALCcontext* sndContext;

            // Parent. This smart pointer is here only to keep the device alive while the context is.
            boost::shared_ptr<Device> device;
    };
}

#endif // SOUND_OPENAL_CONTEXT_HPP
