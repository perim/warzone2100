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

#ifndef _INCLUDE_SOUND_SOUND_TRACK_SAMPLE_HPP_
#define _INCLUDE_SOUND_SOUND_TRACK_SAMPLE_HPP_

#include "track.hpp"
#include "sample.hpp"
#include "source_lock.hpp"
#include "../general/raii_counter.hpp"

namespace Sound
{
    class Track::Sample : public Sound::Sample
    {
        public:
            Sample(boost::shared_ptr<Track> track);

        protected:
            virtual void SoundSample_Play();
            virtual void SoundSample_Stop();
            virtual bool SoundSample_Playing() const;

            virtual void SoundSample_Pause();
            virtual void SoundSample_Unpause();
            virtual bool SoundSample_Paused() const;

            virtual void SoundSample_Rewind();

            virtual void SoundSample_Volume(float newVol);
            virtual float SoundSample_Volume() const;

            virtual void SoundSample_Mute();
            virtual void SoundSample_Unmute();
            virtual bool SoundSample_Muted() const;

            virtual void SoundSample_PlayOn(boost::shared_ptr<Source> source);

        private:
            boost::shared_ptr<Track> _track;
            RAIICounter::scope_counted _trackCounterLock;

            bool _muted;
            float _volume;

            Source::Lock _source;
    };
}

#endif // _INCLUDE_SOUND_SOUND_TRACK_SAMPLE_HPP_
