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

#include "playlist.hpp"
#include "general/physfs_stream.hpp"
#include <string>
#include <algorithm>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <stdio.h>
#include <physfs.h>
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/wzglobal.h"

#include "cdaudio.h"

// bool is a real type in C++
#ifdef bool
# undef bool
#endif

using boost::array;
using std::for_each;
using std::string;

#define BUFFER_SIZE 2048

namespace Sound
{
PlayList::PlayList() :
		_cur_song(_playlist.begin())
{
}

bool PlayList::read(const char* base_path)
{
	char* fileName;

	// Construct file name
	sasprintf(&fileName, "%s/music.wpl", base_path);

	// Attempt to open the playlist file
	PhysFS::ifstream file(fileName);
	if (!file.is_open())
	{
		debug(LOG_NEVER, "sound_LoadTrackFromFile: PHYSFS_openRead(\"%s\") failed with error: %s\n", fileName, PHYSFS_getLastError());
		return false;
	}

	return read(file, base_path);
}

bool PlayList::read(std::istream& file, const std::string& base_path)
{
	while (!file.eof())
	{
		// Read a line from the file
		string filename;
		std::getline(file, filename);

		// Don't add empty filenames to the playlist
		if (filename.empty())
		{
			continue;
		}

		_playlist.push_back(base_path + "/" + filename);
		debug(LOG_SOUND, "Added song %s to playlist", filename.c_str());
	}

	return true;
}

const char* PlayList::currentSong() const
{
	if (_cur_song == _playlist.end())
		return NULL;

	return _cur_song->c_str();
}

const char* PlayList::nextSong()
{
	if (_cur_song != _playlist.end())
	{
		++_cur_song;
	}

	if (_cur_song == _playlist.end())
	{
		_cur_song = _playlist.begin();
	}

	return currentSong();
}
}
