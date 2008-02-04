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

#ifndef SOUND_DECODING_HPP
#define SOUND_DECODING_HPP

#include <istream>
#include <boost/shared_ptr.hpp>
#include <vorbis/vorbisfile.h>
#include "general/databuffer.hpp"

namespace Sound
{
    class Decoding
    {
        public:
            /** Constructor
             *  \param input istream to decode from
             *  \param Seekable if false this disables seeking, this is faster and adviseable for streams
             */
            Decoding(boost::shared_ptr<std::istream> input, bool Seekable);

            /** Destructor
             */
            ~Decoding();

            /** Decode some audio
             *  decodes audio into an output buffer
             *  \param bufferSize the maximum output size of the returned buffer, 0
             *         is interpreted as no maximum, don't use this when streaming
             *  \return an output buffer
             */
            const DataBuffer decode(std::size_t bufferSize);

            /** Decode some audio
             *  decodes audio into an output buffer
             *  \return an output buffer
             */
            const DataBuffer decode();

            /** Retrieve the channel count
             *  \return number of channels in the decoded stream
             */
            inline unsigned int getChannelCount()
            {
                return _VorbisInfo->channels;
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

            /** Resets the decoder to starting position
             */
            void reset();

            /** Whether seeking is enabled
             *  \return true if seeking is enabled, false if disabled
             */
            bool allowsSeeking();

            /** Enable or disable seeking
             *  \param bSeek true to enable seeking, false to disable it
             */
            void allowsSeeking(const bool& bSeek);

        private:
            // Private and undefined copy constructor and assignment operator,
            // to prevent them from being used.
            Decoding(const Decoding&);
            Decoding& operator=(const Decoding&);

        private:
            // Input stream to read input data from
            boost::shared_ptr<std::istream> _input;

            // Wether to allow seeking or not
            bool         _allowSeeking;

            // Internal identifier towards VorbisFile
            OggVorbis_File _oggVorbisStream;

            // Internal data
            vorbis_info* _VorbisInfo;

            static const ov_callbacks oggVorbis_callbacks;
            static size_t ovB_read(void *ptr, size_t size, size_t nmemb, void *datasource);
            static int ovB_seek(void *datasource, ogg_int64_t offset, int whence);
            static int ovB_close(void *datasource);
            static long ovB_tell(void *datasource);
    };
}

#endif // SOUND_DECODING_HPP
