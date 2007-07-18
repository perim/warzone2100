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

#ifndef _INCLUDE_LIB_SOUND_SOUND_HPP_
#define _INCLUDE_LIB_SOUND_SOUND_HPP_

#include <boost/weak_ptr.hpp>
#include <list>

namespace Sound
{
    class Source;

    class Sample
    {
        public:
            virtual ~Sample() {}

            inline void play()
            {
                SoundSample_Play();
            }

            inline void stop()
            {
                SoundSample_Play();
            }

            inline bool playing() const
            {
                return SoundSample_Playing();
            }

            inline void pause()
            {
                SoundSample_Pause();
            }

            inline void unpause()
            {
                SoundSample_Unpause();
            }

            inline bool paused() const
            {
                return SoundSample_Paused();
            }

            inline void rewind()
            {
                return SoundSample_Rewind();
            }

            inline void volume(float newVol)
            {
                SoundSample_Volume(newVol);
            }

            inline float volume() const
            {
                return SoundSample_Volume();
            }

            inline void mute()
            {
                SoundSample_Mute();
            }

            inline void unmute()
            {
                SoundSample_Unmute();
            }

            inline bool muted() const
            {
                return SoundSample_Muted();
            }

            inline void playOn(boost::shared_ptr<Source> source)
            {
                SoundSample_PlayOn(source);
            }

        protected:
            virtual void SoundSample_Play() = 0;
            virtual void SoundSample_Stop() = 0;
            virtual bool SoundSample_Playing() const = 0;

            virtual void SoundSample_Pause() = 0;
            virtual void SoundSample_Unpause() = 0;
            virtual bool SoundSample_Paused() const = 0;

            virtual void SoundSample_Rewind() = 0;

            virtual void SoundSample_Volume(float newVol) = 0;
            virtual float SoundSample_Volume() const = 0;

            virtual void SoundSample_Mute() = 0;
            virtual void SoundSample_Unmute() = 0;
            virtual bool SoundSample_Muted() const = 0;

            virtual void SoundSample_PlayOn(boost::shared_ptr<Source> source) = 0;
    };
}

#endif // _INCLUDE_LIB_SOUND_SOUND_HPP_
