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
    bool buffersFull = true;

    for (unsigned int updated = source->numProcessedBuffers() ; updated != 0 ; --updated)
    {
        ALuint buffer;
        buffer = source->unqueueBuffer();

        for (size_t i = 0; i < 2 /* buffercount */; ++i)
        {
            if (buffers[i].getALBufferID() == buffer)
            {
                buffersFull = stream(&buffers[i]);

                if (buffersFull)
                    source->queueBuffer(&buffers[i]);

                break;
            }
        }
    }

    return buffersFull;
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
