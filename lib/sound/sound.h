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

#ifndef _lib_sound_h_
#define _lib_sound_h_

#include "types.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#include "../framework/frame.h"

    /** Initializes the sound-library
     *  This also cleans out the memory acquired by sound_DeviceList, but doesn't prevent reallocation of it.
     *  \return TRUE when the library is initialized, FALSE otherwise
     */
    BOOL sound_InitLibrary(void);

    /** Initializes the sound-library
     *  This version of the initialization function selects which sound Device should be used
     *  This also cleans out the memory acquired by sound_DeviceList, but doesn't prevent reallocation of it.
     *  \param soundDevice the sound-device to use for rendering and output of sound, 0 selects system default
     *  \return TRUE when the library is initialized, FALSE otherwise (which can be due to double intialization)
     */
    BOOL sound_InitLibraryWithDevice(unsigned int soundDevice);

    /** Shuts down and unloads the sound-library
     *  This also cleans out the memory acquired by sound_DeviceList, but doesn't prevent reallocation of it.
     */
    void sound_ShutdownLibrary(void);

    /** Returns an array of C-strings with devicenames
     *  Allocates memory for an array of C-strings, then fills it up with available devices.
     *  \return an array of pointers to character arrays terminated by a NULL pointer
     */
    const char** sound_DeviceList(void);

    /** Creates a 2D source and prepares for streaming specified audio from it
     *  \param fileName the file to play from
     *  \return the id number of the stream as used internally by the library
     */
    sndStreamID sound_Create2DStream(const char* fileName);

    /** Initiates playing of a stream
     *  \param stream the stream to play
     *  \param reset if true the stream will start playing from the beginning, if false it will start from the last known position
     *  \return true on succes, false otherwise
     */
    BOOL sound_Play2DStream(sndStreamID stream, BOOL reset);

    /** Destroys the stream
     *  Deallocates all resources used by the stream.
     *  \param stream the stream to destroy
     */
    void sound_Destroy2DStream(sndStreamID stream);

    /** Determines wether a stream is playing
     *  \param stream the stream to check
     *  \return TRUE if the stream is currently playing, FALSE if it is not or does not exist
     */
    BOOL sound_2DStreamIsPlaying(sndStreamID stream);

    /** Creates a trackbuffer by loading it from a file
     *  \param fileName the file to load
     *  \return the id number of the track as used internally by the library
     */
    TrackHandle sound_LoadTrackFromFile(const char* fileName);

    /** Destructs the referred track object
     *  matches the RES_FREE signature of the resource management framework
     *  \param trackHandle a handle which (indirectly!!) refers to the track object
     */
    void sound_ReleaseTrack(TrackHandle handle);

    /** Finds an audio file previously loaded with sound_LoadTrackFromFile and initializes it
     *  \param fileName the filename of the track
     *  \param loop whether the track should be looped until explictitly stopped
     *  \param volume the volume this track should be played on (range is 0-100)
     *  \param AudibleRadius the radius from the source of sound where it can be heard
     *  \return a non-zero ID number if successfull, zero on failure
     */
    sndTrackID sound_SetTrackVals(const char* fileName, BOOL loop, unsigned int volume, unsigned int AudibleRadius);

    /** Immediately starts playing a track
     *  \param track the track to play
     */
    void sound_PlayTrack(sndTrackID track);

    /** Performs required update actions
     */
    void sound_Update(void);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif // _lib_sound_h_
