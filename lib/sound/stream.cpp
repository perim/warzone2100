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

#include "stream.h"
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>

soundStream::soundStream(bool b2D) : bufferSize(16384)
{
    source = new soundSource(b2D);
}

soundStream::~soundStream()
{
    delete source;
}

soundSource* soundStream::getSource()
{
    return source;
}

bool soundStream::update()
{
}

bool soundStream::stream(soundBuffer* buffer)
{

}

void soundStream::setBufferSize(unsigned int size)
{
    bufferSize = size;
}

unsigned int soundStream::getBufferSize()
{
    return bufferSize;
}
