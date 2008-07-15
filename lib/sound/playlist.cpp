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
#include "playlist.h"
#include "general/physfs_stream.hpp"
#include <string>
#include <sstream>
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

using std::string;
using std::istringstream;

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

	/* Guarantee that (if the playlist contains any tracks) we can retrieve
	 * them.
	 */
	if (_cur_song == _playlist.end())
	{
		_cur_song = _playlist.begin();
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

std::size_t PlayList::numSongs() const
{
	return _playlist.size();
}

void playListTest()
{
	for (unsigned int i = 0; i < 100; ++i)
	{
		static const string first_file ("track1.ogg");
		static const string second_file("track2.ogg");
		static const string playlist(first_file  + "\n" +
		                             second_file + "\n");
		istringstream plist(playlist);

		PlayList list;
		list.read(plist, "music");

		ASSERT(list.numSongs() == 2, "Error occurred while parsing the playlist as it should contain 2 tracks but contains %zu tracks.", list.numSongs());

		const char * const first = list.currentSong();
		const char *       second = list.nextSong();

		assert(first  == first_file);
		assert(second == second_file);

		// Loop around
		second = list.nextSong();
		assert(second == first_file);
		assert(list.numSongs() == 2);
	}

	puts("\tPlaylist self-test: PASSED");
}
}
