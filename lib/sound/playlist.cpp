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
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <physfs.h>
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/wzglobal.h"

// bool is a real type in C++
#ifdef bool
# undef bool
#endif

using std::vector;

#define BUFFER_SIZE 2048

#define NB_TRACKS 3

struct WZ_TRACK
{
	vector<char*>   songs;
	bool		shuffle;
};

static unsigned int current_track = 0;
static unsigned int current_song = 0;

static WZ_TRACK playlist[NB_TRACKS];

void PlayList_Init()
{
	unsigned int i;

	for (i = 0; i < NB_TRACKS; ++i)
	{
		playlist[i].songs.clear();
	}
}

void PlayList_Quit()
{
	unsigned int i;

	for(i = 0; i < NB_TRACKS; ++i)
	{
		playlist[i].songs.clear();
	}
}

bool PlayList_Read(const char* path) 
{
	PHYSFS_file* fileHandle;
	char* path_to_music = NULL;

	char fileName[PATH_MAX];

	// Construct file name
	snprintf(fileName, sizeof(fileName), "%s/music.wpl", path);

	// Attempt to open the playlist file
	fileHandle = PHYSFS_openRead(fileName);
	if (fileHandle == NULL)
	{
		debug(LOG_NEVER, "sound_LoadTrackFromFile: PHYSFS_openRead(\"%s\") failed with error: %s\n", fileName, PHYSFS_getLastError());
		return false;
	}

	while (!PHYSFS_eof(fileHandle))
	{
		char line_buf[BUFFER_SIZE];
		size_t buf_pos = 0;
		char* filename;

		while (buf_pos < sizeof(line_buf) - 1
		    && PHYSFS_read(fileHandle, &line_buf[buf_pos], 1, 1)
		    && line_buf[buf_pos] != '\n'
		    && line_buf[buf_pos] != '\r')
		{
			++buf_pos;
		}

		// Nul-terminate string
		line_buf[buf_pos] = '\0';
		buf_pos = 0;

		if (strncmp(line_buf, "[game]", 6) == 0)
		{
			current_track = 1;
			free(path_to_music);
			path_to_music = NULL;
			playlist[current_track].shuffle = false;
		}
		else if (strncmp(line_buf, "[menu]", 6) == 0)
		{
			current_track = 2;
			free(path_to_music);
			path_to_music = NULL;
			playlist[current_track].shuffle = false;
		}
		else if (strncmp(line_buf, "path=", 5) == 0)
		{
			free(path_to_music);
			path_to_music = strtok(line_buf+5, "\n");
			if (strcmp(path_to_music, ".") == 0)
			{
				path_to_music = strdup(path);
			}
			else
			{
				path_to_music = strdup(path_to_music);
			}
			debug(LOG_SOUND, "playlist: path = %s", path_to_music );
		}
		else if (strncmp(line_buf, "shuffle=", 8) == 0)
		{
			if (strcmp(strtok(line_buf+8, "\n"), "yes") == 0)
			{
				playlist[current_track].shuffle = true;
			}
			debug( LOG_SOUND, "playlist: shuffle = yes" );
		}
		else if (line_buf[0] != '\0'
		      && (filename = strtok(line_buf, "\n")) != NULL
		      && strlen(filename) != 0)
		{
			char* filepath;

			if (path_to_music == NULL)
			{
				filepath = strdup(filename);
				if (filename == NULL)
				{
					debug(LOG_ERROR, "PlayList_Read: Out of memory!");
					PHYSFS_close(fileHandle);
					abort();
					return false;
				}
			}
			else
			{
				// Determine the length of the string we're about to construct
				size_t path_length = strlen(path_to_music) + 1 + strlen(filename) + 2;

				// Allocate memory for our string
				filepath = (char*)malloc(path_length);
				if (filepath == NULL)
				{
					debug(LOG_ERROR, "PlayList_Read: Out of memory!");
					free(path_to_music);
					PHYSFS_close(fileHandle);
					abort();
					return false;
				}

				snprintf(filepath, path_length, "%s/%s", path_to_music, filename);
			}
			debug(LOG_SOUND, "playlist: adding song %s", filepath );

			playlist[current_track].songs.push_back(filepath);
		}
	}

	free(path_to_music);

	PHYSFS_close(fileHandle);

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
		return playlist[current_track].songs[current_song];
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
	
	return playlist[current_track].songs[current_song];
}
