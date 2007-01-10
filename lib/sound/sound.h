/*
* This file is part of Warzone 2100, an open-source, cross-platform, real-time strategy game
* Copyright (C) 2007  Giel van Schijndel, Warzone Ressurection Project
*
* This program is distributed under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
*
* $Revision$
* $Id$
* $HeadURL$
*/

#ifndef _lib_sound_h_
#define _lib_sound_h_

#if defined(__cplusplus)
extern "C" {
#endif

#include "../framework/frame.h"

/** Initializes the sound-library
 *  \return TRUE when the library is initialized, FALSE otherwise
 */
BOOL sound_InitLibrary(void);

/** Shuts down and unloads the sound-library
 */
void sound_ShutdownLibrary(void);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif // _lib_sound_h_
