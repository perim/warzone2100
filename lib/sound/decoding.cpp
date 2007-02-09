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

#include "decoding.hpp"
#include "constants.hpp"
#include "stringconv.hpp"
#include <vorbis/vorbisfile.h>

size_t ovB_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    return PHYSFS_read(reinterpret_cast<PHYSFS_file*>(datasource), ptr, 1, size*nmemb);
}

int ovB_seek(void *datasource, ogg_int64_t offset, int whence) {
    return -1;
}

int ovB_close(void *datasource) {
    return 0;
}

long ovB_tell(void *datasource) {
    return -1;
}

static ov_callbacks oggVorbis_callbacks = {
	ovB_read,
	ovB_seek,
	ovB_close,
	ovB_tell
};

soundDecoding::soundDecoding(std::string fileName)
{
    if (!PHYSFS_exists(fileName.c_str()))
        throw std::string("decoding: PhysFS: File Not Found");

    fileHandle = PHYSFS_openRead(fileName.c_str());
    if (fileHandle == NULL)
        throw std::string("decoding: PhysFS returned NULL upon PHYSFS_openRead call");

    int error = ov_open_callbacks(fileHandle, &oggVorbisStream, NULL, 0, oggVorbis_callbacks);
    if (error < 0)
        throw std::string("decoding: VorbisFile returned an error while opening; errorcode: " + to_string(error));

    VorbisInfo = ov_info(&oggVorbisStream, -1);
}

soundDecoding::~soundDecoding()
{
    PHYSFS_close(fileHandle);
}

unsigned int soundDecoding::decode(boost::shared_array<char> buffer, unsigned int bufferSize)
{
    unsigned int size = 0;
    while(size < bufferSize)
    {
        int section;
        int result = ov_read(&oggVorbisStream, buffer.get() + size, bufferSize - size, 0, 2, 1, &section);

        if(result > 0)
            size += result;
        else
            if(result < 0)
                //throw errorString(result);
                throw std::string("error decoding OggVorbis stream; errorcode: " + to_string(result) + " with buffersize: " + to_string(bufferSize));
            else
                break;
    }

    return size;
}

unsigned int soundDecoding::numChannels()
{
    return VorbisInfo->channels;
}

unsigned int soundDecoding::frequency()
{
    return VorbisInfo->rate;
}
