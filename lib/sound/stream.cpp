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
#include "constants.hpp"

soundStream::soundStream(boost::shared_ptr<soundContext> sndContext, boost::shared_ptr<soundDecoding> PCM) : soundSource(sndContext), decoder(PCM), bufferSize(OpenAL_BufferSize)
{
}

soundStream::~soundStream()
{
}

bool soundStream::update()
{
    if (!isPlaying()) return false;

    bool buffersFull = true;

    for (unsigned int update = numProcessedBuffers() ; update != 0 ; --update)
    {
        boost::shared_ptr<soundBuffer> buffer(unqueueBuffer());

        buffersFull = stream(buffer);

        if (buffersFull)
            queueBuffer(buffer);
    }

    return buffersFull;
}

bool soundStream::stream(boost::shared_ptr<soundBuffer> buffer)
{
    // Fill the buffer with decoded PCM data
    unsigned int size = bufferSize;
    boost::shared_array<char> pcm(decoder->decode(size));

    if (size == 0)
        return false;

    buffer->bufferData(decoder->getChannelCount(), decoder->frequency(), pcm.get(), size);

    return true;
}

bool soundStream::play()
{
    if (isPlaying())
        return true;

    // Create two streaming buffers
    boost::shared_ptr<soundBuffer> buf1(new soundBuffer);
    boost::shared_ptr<soundBuffer> buf2(new soundBuffer);

    // Fill the buffers with sounddata
    if (!stream(buf1))
        return false;
    bool buffer2Filled = stream(buf2);

    queueBuffer(buf1);
    // Only queue the second buffer if it is filled (i.e. there was enough data to fill it with)
    if (buffer2Filled)
        queueBuffer(buf2);

    soundSource::play();

    return isPlaying();
}

void soundStream::stop()
{
    // First stop playback
    soundSource::stop();

    // Now destroy all buffers
    for (unsigned int i = numProcessedBuffers(); i != 0; --i)
        unqueueBuffer();

    // Reset decoder
    decoder->reset();
}

void soundStream::setBufferSize(unsigned int size)
{
    bufferSize = size;
}

unsigned int soundStream::getBufferSize()
{
    return bufferSize;
}
