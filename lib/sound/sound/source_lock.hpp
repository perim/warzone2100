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

#ifndef _INCLUDE_LIB_SOUND_SOUND_SOURCE_LOCK_HPP_
#define _INCLUDE_LIB_SOUND_SOUND_SOURCE_LOCK_HPP_

#include <boost/shared_ptr.hpp>
#include "../openal/source.hpp"
#include "source.hpp"

namespace Sound
{
    class Source::Lock
    {
        public:
            Lock();
            ~Lock();

            bool lock(boost::shared_ptr<Sound::Source> sndSource);
            void unlock();

            bool locked() const;

            // These functions only work when a source is locked
            OpenAL::Source::sourceState getState() const;

            void queueBuffer(boost::shared_ptr<OpenAL::Buffer> sndBuffer);
            boost::shared_ptr<OpenAL::Buffer> unqueueBuffer();
            unsigned int numProcessedBuffers() const;

            // These functions work independently from whether a source is locked or not
            void setBuffer(boost::shared_ptr<OpenAL::Buffer> sndBuffer);
            void unbuffer();

            void play();
            void stop();
            bool playing() const;

            void pause();
            void unpause();
            bool paused() const;

            void setPosition(float x, float y, float z);
            void setPosition(int x, int y, int z);
            void getPosition(float& x, float& y, float& z) const;
            void getPosition(int& x, int& y, int& z) const;

            void setRotation(float pitch, float yaw, float roll);
            void setRotation(int pitch, int yaw, int roll);
            void getRotation(float& pitch, float& yaw, float& roll) const;
            void getRotation(int& pitch, int& yaw, int& roll) const;

            void setVelocity(float x, float y, float z);
            void setVelocity(int x, int y, int z);
            void getVelocity(float& x, float& y, float& z) const;
            void getVelocity(int& x, int& y, int& z) const;

            void volume(float gain);
            float volume() const;

            void loop(bool looping);
            bool loop() const;

        private:
            void checkLock() const;

        private:
            boost::shared_ptr<Sound::Source> _source;
            boost::shared_ptr<OpenAL::Buffer> _buffer;

            mutable bool _playing, _paused;

            float _xPos, _yPos, _zPos;
            float _pitch, _yaw, _roll;
            float _xVel, _yVel, _zVel;

            float _volume;
            bool _looping;
    };
}

#endif // _INCLUDE_LIB_SOUND_SOUND_SOURCE_LOCK_HPP_
