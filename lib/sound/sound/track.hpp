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

#ifndef _INCLUDE_SOUND_SOUND_TRACK_HPP_
#define _INCLUDE_SOUND_SOUND_TRACK_HPP_

#include <string>
#include "../openal/buffer.hpp"
#include "../general/raii_counter.hpp"

namespace Sound
{
    class Track : public OpenAL::Buffer
    {
        public:
            Track(const DataBuffer& data);

            void loop(bool bLoop);
            bool loop() const;

            void volume(float vol);
            float volume() const;

            void fileName(const std::string& fName);
            std::string fileName() const;

            unsigned int lastFinished() const;
            void lastFinished(unsigned int finishTime);

            unsigned int audibleRadius() const;
            void audibleRadius(unsigned int radius);

            unsigned int numPlaying() const;

            class Sample;

        private:
            bool         _loop;
            float        _volume;
            unsigned int _lastFinished; // timeframe this track last finished playing in user defined units
            unsigned int _audibleRadius;
            std::string  _fileName;

            RAIICounter _playingTracks;
    };
}

#endif // _INCLUDE_SOUND_SOUND_TRACK_HPP_
