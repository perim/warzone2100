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

#include "stream.hpp"
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <string>

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
    if (source->is2D())
        throw std::string("soundStream: can't retrieve source if stream is 2D");
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
