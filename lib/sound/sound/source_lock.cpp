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

#include "source_lock.hpp"
#include <stdexcept>

namespace Sound
{
    Source::Lock::Lock() :
        _playing(false),
        _paused(false),
        _xPos(0.0), _yPos(0.0), _zPos(0.0),
        _pitch(0.0), _yaw(0.0), _roll(0.0),
        _xVel(0.0), _yVel(0.0), _zVel(0.0),
        _volume(1.0),
        _looping(false)
    {
    }

    Source::Lock::~Lock()
    {
        unlock();
    }

    bool Source::Lock::lock(boost::shared_ptr<Sound::Source> sndSource)
    {
        // If we already have a lock on a source, we don't need to acquire a second
        if (_source)
            return true;

        // If the requested source is locked already, don't try re-locking it. Return failure.
        if (sndSource->locked())
            return false;

        // Lock the source
        sndSource->_lock = this;
        _source = sndSource;

        OpenAL::Source& source = _source->_source;

        // Copy our data into OpenAL
        source.rewind();

        if (_buffer)
            source.setBuffer(_buffer);
        else
            source.unbuffer();

        source.setPosition(_xPos, _yPos, _zPos);
        source.setRotation(_pitch, _yaw, _roll);
        source.setVelocity(_xVel, _yVel, _zVel);

        source.volume(_volume);
        source.loop(_looping);

        if (_playing)
        {
            source.play();
            if (_paused)
                source.pause();
        }

        return true;
    }

    void Source::Lock::unlock()
    {
        if (!_source)
            return;

        // Call these functions to store our playing and paused state
        playing();
        paused();

        _source->_lock = 0;
        _source.reset();
    }

    bool Source::Lock::locked() const
    {
        return _source;
    }

    OpenAL::Source::sourceState Source::Lock::getState() const
    {
        checkLock();

        return _source->_source.getState();
    }

    void Source::Lock::queueBuffer(boost::shared_ptr<OpenAL::Buffer> sndBuffer)
    {
        checkLock();

        _source->_source.queueBuffer(sndBuffer);
    }

    boost::shared_ptr<OpenAL::Buffer> Source::Lock::unqueueBuffer()
    {
        checkLock();

        return _source->_source.unqueueBuffer();
    }

    unsigned int Source::Lock::numProcessedBuffers() const
    {
        checkLock();

        return _source->_source.numProcessedBuffers();
    }

    void Source::Lock::setBuffer(boost::shared_ptr<OpenAL::Buffer> sndBuffer)
    {
        _buffer = sndBuffer;

        if (_source)
            _source->_source.setBuffer(_buffer);
    }

    void Source::Lock::unbuffer()
    {
        _buffer.reset();

        if (_source)
            _source->_source.unbuffer();
    }

    void Source::Lock::play()
    {
        _playing = true;

        if (_source)
            _source->_source.play();
    }

    void Source::Lock::stop()
    {
        _playing = false;

        if (_source)
            _source->_source.stop();
    }

    bool Source::Lock::playing() const
    {
        if (!_source)
            return _playing;

        OpenAL::Source::sourceState state = getState();
        _playing = (state == OpenAL::Source::playing
                 || state == OpenAL::Source::paused);

        return _playing;
    }

    void Source::Lock::pause()
    {
        _paused = true;
    }

    void Source::Lock::unpause()
    {
        _paused = false;
    }

    bool Source::Lock::paused() const
    {
        if (!_source)
            return _paused;

        OpenAL::Source::sourceState state = getState();
        _paused = (state == OpenAL::Source::paused);

        return _paused;
    }

    void Source::Lock::setPosition(float x, float y, float z)
    {
        _xPos = x;
        _yPos = y;
        _zPos = z;

        if (_source)
            _source->_source.setPosition(x, y, z);
    }

    void Source::Lock::setPosition(int x, int y, int z)
    {
        _xPos = x;
        _yPos = y;
        _zPos = z;

        if (_source)
            _source->_source.setPosition(x, y, z);
    }

    void Source::Lock::getPosition(float& x, float& y, float& z) const
    {
        if (_source)
        {
            _source->_source.getPosition(x, y, z);
            return;
        }
        else
        {
            x = _xPos;
            y = _yPos;
            z = _zPos;
        }
    }

    void Source::Lock::getPosition(int& x, int& y, int& z) const
    {
        if (_source)
        {
            _source->_source.getPosition(x, y, z);
            return;
        }
        else
        {
            x = int(_xPos);
            y = int(_yPos);
            z = int(_zPos);
        }
    }

    void Source::Lock::setRotation(float pitch, float yaw, float roll)
    {
        _pitch = pitch;
        _yaw = yaw;
        _roll = roll;

        if (_source)
            _source->_source.setRotation(pitch, yaw, roll);
    }

    void Source::Lock::setRotation(int pitch, int yaw, int roll)
    {
        _pitch = pitch;
        _yaw = yaw;
        _roll = roll;

        if (_source)
            _source->_source.setRotation(pitch, yaw, roll);
    }

    void Source::Lock::getRotation(float& pitch, float& yaw, float& roll) const
    {
        if (_source)
        {
            _source->_source.getRotation(pitch, yaw, roll);
            return;
        }
        else
        {
            pitch = _pitch;
            yaw = _yaw;
            roll = _roll;
        }
    }

    void Source::Lock::getRotation(int& pitch, int& yaw, int& roll) const
    {
        if (_source)
        {
            _source->_source.getRotation(pitch, yaw, roll);
            return;
        }
        else
        {
            pitch = int(_pitch);
            yaw = int(_yaw);
            roll = int(_roll);
        }
    }

    void Source::Lock::setVelocity(float x, float y, float z)
    {
        _xVel = x;
        _yVel = y;
        _zVel = z;

        if (_source)
            _source->_source.setVelocity(x, y, z);
    }

    void Source::Lock::setVelocity(int x, int y, int z)
    {
        _xVel = x;
        _yVel = y;
        _zVel = z;

        if (_source)
            _source->_source.setVelocity(x, y, z);
    }

    void Source::Lock::getVelocity(float& x, float& y, float& z) const
    {
        if (_source)
        {
            _source->_source.getVelocity(x, y, z);
            return;
        }
        else
        {
            x = _xVel;
            y = _yVel;
            z = _zVel;
        }
    }

    void Source::Lock::getVelocity(int& x, int& y, int& z) const
    {
        if (_source)
        {
            _source->_source.getVelocity(x, y, z);
            return;
        }
        else
        {
            x = int(_xVel);
            y = int(_yVel);
            z = int(_zVel);
        }
    }

    void Source::Lock::checkLock() const
    {
        if (!locked())
            throw std::runtime_error("Source::Lock::getState(): we don't have a locked source to work with");
    }
}
