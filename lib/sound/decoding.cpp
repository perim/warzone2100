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

#include "decoding.hpp"
#include "constants.hpp"
#include "templates.hpp"
#include <vorbis/vorbisfile.h>
#include <stdexcept>
#include <string>

namespace Sound
{
    size_t Decoding::ovB_read(void *ptr, size_t size, size_t nmemb, void *datasource)
    {
        Decoding& decoder = *static_cast<Decoding*> (datasource);

        return decoder._input->readsome(static_cast<std::istream::char_type*>(ptr), size * nmemb);
    }

    int Decoding::ovB_seek(void *datasource, ogg_int64_t offset, int whence)
    {
        Decoding& decoder = *static_cast<Decoding*> (datasource);

        // check to see if seeking is allowed
        if (!decoder._allowSeeking)
            return -1;

        std::ios_base::seekdir seekDir;

        switch (whence)
        {
            // Seek to absolute position
            case SEEK_SET:
                seekDir = std::ios::beg;
                break;

            // Seek `offset` ahead
            case SEEK_CUR:
                seekDir = std::ios::cur;
                break;

            // Seek backwards from the end of the file
            case SEEK_END:
                seekDir = std::ios::end;
                break;

            // Unknown seek direction: return failure
            default:
                return -1;
        }

        decoder._input->seekg(offset, seekDir);

        return decoder._input->tellg();
    }

    int Decoding::ovB_close(void *datasource)
    {
        return 0;
    }

    long Decoding::ovB_tell(void *datasource)
    {
        Decoding& decoder = *static_cast<Decoding*> (datasource);

        return decoder._input->tellg();
    }

    const ov_callbacks Decoding::oggVorbis_callbacks =
    {
        ovB_read,
        ovB_seek,
        ovB_close,
        ovB_tell
    };

    Decoding::Decoding(boost::shared_ptr<std::istream> input, bool Seekable) :
        _input(input)
    {
        int error = ov_open_callbacks(this, &_oggVorbisStream, NULL, 0, oggVorbis_callbacks);
        if (error < 0)
            throw std::runtime_error(std::string("Sound::Decoding: VorbisFile returned an error while opening; errorcode: " + to_string(error)));

        _VorbisInfo = ov_info(&_oggVorbisStream, -1);
    }

    Decoding::~Decoding()
    {
        ov_clear(&_oggVorbisStream);
    }

    void Decoding::reset()
    {
        ov_clear(&_oggVorbisStream);
        _input->seekg(0);

        int error = ov_open_callbacks(this, &_oggVorbisStream, NULL, 0, oggVorbis_callbacks);
        if (error < 0)
            throw std::runtime_error(std::string("Sound::Decoding.reset: VorbisFile returned an error while opening; errorcode: " + to_string(error)));

        _VorbisInfo = ov_info(&_oggVorbisStream, -1);
    }

    bool Decoding::allowsSeeking()
    {
        return _allowSeeking;
    }

    void Decoding::allowsSeeking(const bool& bSeek)
    {
        if (allowsSeeking() == bSeek)
            return;

        // If this is a state change, then reset the decoder
        reset();

        _allowSeeking = bSeek;
    }

    const DataBuffer Decoding::decode(std::size_t bufferSize)
    {
        unsigned int sizeEstimate = getSampleCount() * getChannelCount() * 2; // The 2 is for the assumption that samples are 16 bit large

        // If a maximum buffer size is specified check wether it isn't larger than the needed size
        if (((bufferSize == 0) || (bufferSize > sizeEstimate)) && (sizeEstimate != 0) )
            bufferSize = (getSampleCount() - getCurrentSample()) * getChannelCount() * 2;

        DataBuffer buffer(bufferSize, 16, getChannelCount(), frequency());

        std::size_t size = 0;
        while(size < bufferSize)
        {
            int section;
            int result = ov_read(&_oggVorbisStream, &buffer[size], bufferSize - size, 0, 2, 1, &section);

            if(result > 0)
                size += result;
            else
                if(result < 0)
                    //throw errorString(result);
                    throw std::runtime_error("Sound::Decoding.decode: error decoding OggVorbis stream; errorcode: " + to_string(result) + " with buffersize: " + to_string(bufferSize));
                else
                    break;
        }

        buffer.resize(size);

        return buffer;
    }

    const DataBuffer Decoding::decode()
    {
        allowsSeeking(true);
        return decode(0);
    }

    unsigned int Decoding::frequency()
    {
        return _VorbisInfo->rate;
    }

    unsigned int Decoding::getSampleCount()
    {
        int numSamples = ov_pcm_total(&_oggVorbisStream, -1);

        if (numSamples == OV_EINVAL)
            return 0;
        else
            return numSamples;
    }

    unsigned int Decoding::getCurrentSample()
    {
        int samplePos = ov_pcm_tell(&_oggVorbisStream);

        if (samplePos == OV_EINVAL)
            throw std::runtime_error(std::string("Sound::Decoding.getCurrentSample: ov_pcm_tell: invalid argument"));
        else
            return samplePos;
    }
}
