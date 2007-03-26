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
#include "templates.hpp"
#include <vorbis/vorbisfile.h>

struct fileInfo
{
    // Internal identifier towards PhysFS
    PHYSFS_file* fileHandle;

    // Wether to allow seeking or not
    bool         allowSeeking;
};

static size_t ovB_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    PHYSFS_file* fileHandle = reinterpret_cast<fileInfo*>(datasource)->fileHandle;
    return PHYSFS_read(fileHandle, ptr, 1, size*nmemb);
}

static int ovB_seek(void *datasource, ogg_int64_t offset, int whence) {
    // check to see if seeking is allowed
    if (!reinterpret_cast<fileInfo*>(datasource)->allowSeeking)
        return -1;

    PHYSFS_file* fileHandle = reinterpret_cast<fileInfo*>(datasource)->fileHandle;

    int curPos, fileSize, newPos;

    switch (whence)
    {
        // Seek to absolute position
        case SEEK_SET:
            newPos = offset;
            break;

        // Seek `offset` ahead
        case SEEK_CUR:
            curPos = PHYSFS_tell(fileHandle);
            if (curPos == -1)
                return -1;

            newPos = curPos + offset;
            break;

        // Seek backwards from the end of the file
        case SEEK_END:
            fileSize = PHYSFS_fileLength(fileHandle);
            if (fileSize == -1)
                return -1;

            newPos = fileSize - 1 - offset;
            break;
    }

    // PHYSFS_seek return value of non-zero means success
    if (PHYSFS_seek(fileHandle, newPos) != 0)
        return newPos;   // success
    else
        return -1;  // failure
}

static int ovB_close(void *datasource) {
    return 0;
}

static long ovB_tell(void *datasource) {
    PHYSFS_file* fileHandle = reinterpret_cast<fileInfo*>(datasource)->fileHandle;
    return PHYSFS_tell(fileHandle);
}

static ov_callbacks oggVorbis_callbacks = {
        ovB_read,
        ovB_seek,
        ovB_close,
        ovB_tell
};

soundDecoding::soundDecoding(std::string fileName, bool Seekable) : fileHandle(new fileInfo)
{
    fileHandle->allowSeeking = Seekable;

    if (!PHYSFS_exists(fileName.c_str()))
        throw std::string("decoding: PhysFS: File Not Found");

    fileHandle->fileHandle = PHYSFS_openRead(fileName.c_str());
    if (fileHandle->fileHandle == NULL)
        throw std::string("decoding: PhysFS returned NULL upon PHYSFS_openRead call");

    int error = ov_open_callbacks(fileHandle.get(), &oggVorbisStream, NULL, 0, oggVorbis_callbacks);
    if (error < 0)
        throw std::string("decoding: VorbisFile returned an error while opening; errorcode: " + to_string(error));

    VorbisInfo = ov_info(&oggVorbisStream, -1);
}

soundDecoding::~soundDecoding()
{
    PHYSFS_close(fileHandle->fileHandle);
}

void soundDecoding::reset()
{
    PHYSFS_seek(fileHandle->fileHandle, 0);

    int error = ov_open_callbacks(fileHandle.get(), &oggVorbisStream, NULL, 0, oggVorbis_callbacks);
    if (error < 0)
        throw std::string("decoding: VorbisFile returned an error while opening; errorcode: " + to_string(error));

    VorbisInfo = ov_info(&oggVorbisStream, -1);
}

boost::shared_array<char> soundDecoding::decode(unsigned int& bufferSize)
{
    unsigned int sizeEstimate = getSampleCount() * getChannelCount() * 2; // The 2 is for the assumption that samples are 16 bit large

    // If a maximum buffer size is specified check wether it isn't larger than the needed size
    if (((bufferSize == 0) || (bufferSize > sizeEstimate)) && (sizeEstimate != 0) )
        bufferSize = (getSampleCount() - getCurrentSample()) * getChannelCount() * 2;

    boost::shared_array<char> buffer(new char[bufferSize]);

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

    bufferSize = size;
    return buffer;
}

unsigned int soundDecoding::frequency()
{
    return VorbisInfo->rate;
}

unsigned int soundDecoding::getSampleCount()
{
    int numSamples = ov_pcm_total(&oggVorbisStream, -1);

    if (numSamples == OV_EINVAL)
        return 0;
    else
        return numSamples;
}

unsigned int soundDecoding::getCurrentSample()
{
    int samplePos = ov_pcm_tell(&oggVorbisStream);

    if (samplePos == OV_EINVAL)
        throw std::string("soundDecoding::getCurrentSample: ov_pcm_tell: invalid argument");
    else
        return samplePos;
}
