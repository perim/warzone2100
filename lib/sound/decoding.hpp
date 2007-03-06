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

#ifndef SOUND_DECODING_HPP
#define SOUND_DECODING_HPP

#include <boost/smart_ptr.hpp>
#include <physfs.h>
#include <string>
#include <vorbis/vorbisfile.h>

struct fileInfo;

class soundDecoding
{
    public:
        /** Constructor
         *  \param fileName file to decode from
         *  \param Seekable if false this disables seeking, this is faster and adviseable for streams
         */
        soundDecoding(std::string fileName, bool Seekable);

        /** Destructor
         */
        ~soundDecoding();

        /** Decode some audio
         *  decodes audio until the output buffer has reached the length bufferSize
         *  \param[in,out] bufferSize the maximum output size of the returned buffer, this is modified to the actual buffer size
         *  \return an output buffer and a smart pointer to the output buffer
         */
        boost::shared_array<char> decode(unsigned int& bufferSize);

        /** Retrieve the channel count
         *  \return number of channels in the decoded stream
         */
        inline unsigned int getChannelCount()
        {
            return VorbisInfo->channels;
        }

        /** Retrieve the sample frequency
         *  \return sample frequency of the decoded stream in Hertz (samples/second)
         */
        unsigned int frequency();

        /** Retrieve the amount of samples
         *  \return the amount of samples (samples/channel), or zero if it can't be determined
         */
        unsigned int getSampleCount();

        /** Retrieve the current sample
         *  \return the next sample to be decoded
         */
        unsigned int getCurrentSample();

    private:
        // Info used by the internal file reading callback mechanism
        boost::shared_ptr<fileInfo> fileHandle;

        // Internal identifier towards VorbisFile
        OggVorbis_File oggVorbisStream;

        // Internal data
        vorbis_info* VorbisInfo;
};

#endif // SOUND_DECODING_HPP
