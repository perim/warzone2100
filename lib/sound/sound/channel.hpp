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

#ifndef _INCLUDE_LIB_SOUND_SOUND_CHANNEL_HPP_
#define _INCLUDE_LIB_SOUND_SOUND_CHANNEL_HPP_

#include <boost/weak_ptr.hpp>
#include <boost/function.hpp>
#include <list>
#include "sample.hpp"

namespace Sound
{
    class Channel
    {
        public:
            Channel();

            /** Adds a sound to this channel to manage
             */
            void addSound(boost::weak_ptr<Sample> snd);

        protected:
            void pause();
            void unpause();
            bool paused() const;

            void volume(float newVol);
            float volume() const;

            void mute();
            void unmute();
            bool muted() const;

        private:
            void operateOn(boost::function<void (Sample*)> functionToCall);

        private:
            std::list<boost::weak_ptr<Sample> > _sounds;
            bool _muted, _paused;
            float _volume;
    };
}

#endif // _INCLUDE_LIB_SOUND_SOUND_CHANNEL_HPP_
