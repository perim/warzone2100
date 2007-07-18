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

#include "channel.hpp"
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

namespace Sound
{
    Channel::Channel() :
        _muted(false),
        _paused(false),
        _volume(1.0)
    {
    }

    void Channel::addSound(boost::weak_ptr<Sample> snd)
    {
        if (snd.expired())
            return;

        boost::shared_ptr<Sample> sound(snd);

        _sounds.push_back(snd);

        // Set mute state on SoundSample
        if (_muted)
            sound->mute();
        else
            sound->unpause();

        // Set paused state on SoundSample
        if (_paused)
            sound->pause();
        else
            sound->unpause();

        // Set volume on SoundSample
        sound->volume(_volume);
    }

    void Channel::pause()
    {
        _paused = true;

        operateOn(&Sample::pause);
    }

    void Channel::unpause()
    {
        _paused = false;

        operateOn(&Sample::unpause);
    }

    bool Channel::paused() const
    {
        return _paused;
    }

    void Channel::volume(float newVol)
    {
        _volume = newVol;

        operateOn(boost::bind<void>(&Sample::volume, _1, newVol));
    }

    float Channel::volume() const
    {
        return _volume;
    }

    void Channel::mute()
    {
        _muted = true;

        operateOn(&Sample::mute);
    }

    void Channel::unmute()
    {
        _muted = false;

        operateOn(&Sample::unmute);
    }

    bool Channel::muted() const
    {
        return _muted;
    }

    void Channel::operateOn(boost::function<void (Sample*)> functionToCall)
    {
        for (std::list<boost::weak_ptr<Sample> >::iterator curSound = _sounds.begin(); curSound != _sounds.end();)
        {
            // If this sound has been destroyed remove it from the list
            if (curSound->expired())
            {
                std::list<boost::weak_ptr<Sample> >::iterator toErase = curSound++;

                _sounds.erase(toErase);

                continue;
            }

            boost::shared_ptr<Sample> snd(*curSound);

            // Call the requested function on this SoundSample object
            functionToCall(snd.get());

            // Continue to the next sound (we do this here instead of in the for-declaration so that
            // we can use "continue" above sine std::list::erase invalidates the current iterator
            ++curSound;
        }
    }
}
