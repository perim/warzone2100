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
     *  \return TRUE when the library is initialized, FALSE otherwise
     */
    BOOL sound_InitLibrary(void);

    /** Shuts down and unloads the sound-library
     */
    void sound_ShutdownLibrary(void);

    /** Creates a 2D source and prepares for streaming specified audio from it
     *  \param path the directory path to the file to start playing
     *  \return true on success (stream is playing), false otherwise
     */
    sndStreamID sound_Create2DStream(char* path);

    /** Performs required update actions
     */
    void sound_Update(void);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif // _lib_sound_h_
