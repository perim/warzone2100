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

#include "stream.hpp"
#include "constants.hpp"
#include "openal/buffer.hpp"
#include "general/databuffer.hpp"

namespace Sound
{
    Stream::Stream(boost::shared_ptr<Decoding> PCM) :
        decoder(PCM),
        bufferSize(OpenAL_BufferSize)
    {
    }

    Stream::~Stream()
    {
    }

    bool Stream::update()
    {
        sourceState state = getState();
        if (state != playing && state != paused)
            return false;

        for (unsigned int update = numProcessedBuffers() ; update != 0 ; --update)
        {
            boost::shared_ptr<OpenAL::Buffer> buffer(unqueueBuffer());

            bool bufferFull = stream(buffer);

            if (bufferFull)
                queueBuffer(buffer);
        }

        return true;
    }

    bool Stream::stream(boost::shared_ptr<OpenAL::Buffer> buffer)
    {
        // Fill the buffer with decoded PCM data
        DataBuffer pcm = decoder->decode(bufferSize);

        if (pcm.empty())
            return false;

        buffer->bufferData(pcm);

        return true;
    }

    bool Stream::play()
    {
        if (isPlaying())
            return true;

        // Create two streaming buffers
        boost::shared_ptr<OpenAL::Buffer> buf1(new OpenAL::Buffer);
        boost::shared_ptr<OpenAL::Buffer> buf2(new OpenAL::Buffer);

        // Fill the buffers with sounddata
        if (!stream(buf1))
            return false;
        bool buffer2Filled = stream(buf2);

        queueBuffer(buf1);
        // Only queue the second buffer if it is filled (i.e. there was enough data to fill it with)
        if (buffer2Filled)
            queueBuffer(buf2);

        Source::play();

        return isPlaying();
    }

    void Stream::stop()
    {
        // First stop playback
        Source::stop();

        // Now destroy all buffers
        for (unsigned int i = numProcessedBuffers(); i != 0; --i)
            unqueueBuffer();

        // Reset decoder
        decoder->reset();
    }

    void Stream::setBufferSize(unsigned int size)
    {
        bufferSize = size;
    }

    unsigned int Stream::getBufferSize()
    {
        return bufferSize;
    }
}
