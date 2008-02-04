/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
*/

#ifndef __INCLUDED_LIB_SOUND_PLAYLIST_HPP__
#define __INCLUDED_LIB_SOUND_PLAYLIST_HPP__

#if !defined(__cplusplus)
# error "This header file is meant to be compiled as C++ only (i.e. not C)."
#endif

#include <istream>

namespace Sound
{
    class PlayList
    {
        public:
            PlayList();
            PlayList(const char* path);
            PlayList(std::istream& file);
            ~PlayList();

            bool read(const char* base_path);
            bool read(std::istream& file, const char* base_path);

            void track(unsigned int t);

            const char* currentSong() const;
            const char* nextSong();

        private:
            void shuffle();

        private:
            unsigned int _cur_track;
            unsigned int _cur_song;
            
            struct Track;
            Track* _playlist;
    };
}

#endif // __INCLUDED_LIB_SOUND_PLAYLIST_HPP__
