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

class soundDecoding
{
    public:
        /** Constructor
         *  \param fileName file to decode from
         */
        soundDecoding(std::string fileName);

        /** Destructor
         */
        ~soundDecoding();

        /** Decode some audio
         *  decodes audio until the output buffer has reached the length bufferSize
         *  \param bufferSize the maximum output size of the returned buffer
         *  \return a smart pointer to the output buffer (so that destruction is automatically dealt with)
         */
        unsigned int decode(boost::shared_array<char> buffer, unsigned int bufferSize);

        /** Retrieve the channel count
         *  \return number of channels in the decoded stream
         */
        unsigned int numChannels();

        /** Retrieve the sample frequency
         *  \return sample frequency of the decoded stream
         */
        unsigned int frequency();

    private:
        // Internal identifier towards PhysFS
        PHYSFS_file* fileHandle;
        OggVorbis_File oggVorbisStream;
        vorbis_info* VorbisInfo;
};

#endif // SOUND_DECODING_HPP
