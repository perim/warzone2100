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

// bool is a real type in C++
#ifdef bool
# undef bool
#endif

using boost::array;
using std::for_each;
using std::string;
using std::vector;

#define BUFFER_SIZE 2048

#define NB_TRACKS 3

struct WZ_TRACK
{
	vector<string>  songs;
	bool		shuffle;

	void clear();
};

void WZ_TRACK::clear()
{
	songs.clear();
	shuffle = false;
}

static unsigned int current_track = 0;
static unsigned int current_song = 0;

static array<WZ_TRACK, NB_TRACKS> playlist;

void PlayList_Init()
{
	// Call clear on all WZ_TRACKs
	for_each(playlist.begin(), playlist.end(), boost::bind(&WZ_TRACK::clear, _1));
}

void PlayList_Quit()
{
	// Call clear on all WZ_TRACKs
	for_each(playlist.begin(), playlist.end(), boost::bind(&WZ_TRACK::clear, _1));
}

bool PlayList_Read(const char* path) 
{
	string path_to_music;

	char fileName[PATH_MAX];

	// Construct file name
	snprintf(fileName, sizeof(fileName), "%s/music.wpl", path);

	// Attempt to open the playlist file
	PhysFS::ifstream file(fileName);
	if (!file.is_open())
	{
		debug(LOG_NEVER, "sound_LoadTrackFromFile: PHYSFS_openRead(\"%s\") failed with error: %s\n", fileName, PHYSFS_getLastError());
		return false;
	}

	while (!file.eof())
	{
		// Read a line from the file
		string line;
		std::getline(file, line);

		if (line.substr(0, 6) == "[game]")
		{
			current_track = 1;
			path_to_music.clear();
			playlist[current_track].shuffle = false;
		}
		else if (line.substr(0, 6) == "[menu]")
		{
			current_track = 2;
			path_to_music.clear();
			playlist[current_track].shuffle = false;
		}
		else if (line.substr(0, 5) == "path=")
		{
			path_to_music = line.substr(5);
			if (path_to_music == ".")
			{
				path_to_music = path;
			}

			debug(LOG_SOUND, "playlist: path = %s", path_to_music.c_str());
		}
		else if (line.substr(0, 8) == "shuffle=")
		{
			if (line.substr(8) == "yes")
			{
				playlist[current_track].shuffle = true;
			}
			debug( LOG_SOUND, "playlist: shuffle = yes" );
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

			playlist[current_track].songs.push_back(filepath);
		}
	}

	return true;
}

static void PlayList_Shuffle()
{
	if (playlist[current_track].shuffle)
	{
		for (unsigned int i = playlist[current_track].songs.size() - 1; i > 0; --i)
		{
			unsigned int j = rand() % (i + 1);

			if (i != j)
				std::swap(playlist[current_track].songs[i], playlist[current_track].songs[j]);
		}
	}
}

void PlayList_SetTrack(unsigned int t)
{
	if (t < NB_TRACKS)
	{
		current_track = t;
	}
	else
	{
		current_track = 0;
	}
	PlayList_Shuffle();
	current_song = 0;
}

const char* PlayList_CurrentSong()
{
	if (current_song < playlist[current_track].songs.size())
	{
		return playlist[current_track].songs[current_song].c_str();
	}

	return NULL;
}

const char* PlayList_NextSong()
{
	// If we're at the end of the playlist
	if (++current_song >= playlist[current_track].songs.size())
	{
		// Shuffle the playlist (again)
		PlayList_Shuffle();

		// Jump to the start of the playlist
		current_song = 0;
	}

	if (playlist[current_track].songs.empty())
	{
		return NULL;
	}
	
	return playlist[current_track].songs[current_song].c_str();
}
