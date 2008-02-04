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
#include <vector>
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
using std::vector;

#define BUFFER_SIZE 2048

namespace Sound
{
struct PlayList::Track
{
	/** Default constructor.
	 *  Initialises all values to their defaults.
	 */
	Track() :
		shuffle(false)
	{
	}

	vector<string>  songs;
	bool            shuffle;
};

PlayList::PlayList() :
		_cur_track(0),
		_cur_song(0),
		_playlist(new Track[playlist_last])
{
}

PlayList::PlayList(const char* path) :
		_cur_track(0),
		_cur_song(0),
		_playlist(new Track[playlist_last])
{
}

PlayList::PlayList(std::istream& file) :
		_cur_track(0),
		_cur_song(0),
		_playlist(new Track[playlist_last])
{
}

PlayList::~PlayList()
{
	delete [] _playlist;
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

bool PlayList::read(std::istream& file, const char* base_path)
{
	string path_to_music;

	while (!file.eof())
	{
		// Read a line from the file
		string line;
		std::getline(file, line);

		if (line.substr(0, 6) == "[game]")
		{
			_cur_track = 1;
			path_to_music.clear();
			_playlist[_cur_track].shuffle = false;
		}
		else if (line.substr(0, 6) == "[menu]")
		{
			_cur_track = 2;
			path_to_music.clear();
			_playlist[_cur_track].shuffle = false;
		}
		else if (line.substr(0, 5) == "path=")
		{
			path_to_music = line.substr(5);
			if (path_to_music == ".")
			{
				path_to_music = base_path;
			}

			debug(LOG_SOUND, "playlist: path = %s", path_to_music.c_str());
		}
		else if (line.substr(0, 8) == "shuffle=")
		{
			if (line.substr(8) == "yes")
			{
				_playlist[_cur_track].shuffle = true;
			}
			debug(LOG_SOUND, "playlist: shuffle = yes");
		}
		else if (!line.empty())
		{
			string filepath;

			if (path_to_music.empty())
			{
				filepath = line;
			}
			else
			{
				filepath = path_to_music + "/" + line;
			}

			debug(LOG_SOUND, "playlist: adding song %s", filepath.c_str());

			_playlist[_cur_track].songs.push_back(filepath);
		}
	}

	return true;
}

void PlayList::shuffle()
{
	if (!_playlist[_cur_track].shuffle)
		return;

	for (unsigned int i = _playlist[_cur_track].songs.size() - 1; i > 0; --i)
	{
		unsigned int j = rand() % (i + 1);

		if (i != j)
			std::swap(_playlist[_cur_track].songs[i], _playlist[_cur_track].songs[j]);
	}
}

void PlayList::track(unsigned int t)
{
	if (t < playlist_last)
	{
		_cur_track = t;
	}
	else
	{
		_cur_track = 0;
	}
	shuffle();
	_cur_song = 0;
}

const char* PlayList::currentSong() const
{
	if (_cur_song < _playlist[_cur_track].songs.size())
	{
		return _playlist[_cur_track].songs[_cur_song].c_str();
	}

	return NULL;
}

const char* PlayList::nextSong()
{
	// If we're at the end of the playlist
	if (++_cur_song >= _playlist[_cur_track].songs.size())
	{
		// Shuffle the playlist (again)
		shuffle();

		// Jump to the start of the playlist
		_cur_song = 0;
	}

	if (_playlist[_cur_track].songs.empty())
	{
		return NULL;
	}

	return _playlist[_cur_track].songs[_cur_song].c_str();
}
}
