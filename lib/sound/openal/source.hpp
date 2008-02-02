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

#ifndef SOUND_OPENAL_SOURCE_HPP
#define SOUND_OPENAL_SOURCE_HPP

#include <AL/al.h>
#include <vector>
#include <boost/smart_ptr.hpp>
#include "../general/geometry.hpp"

namespace OpenAL
{
    // Necessary to declare pointers for Buffer
    class Buffer;

    class Source : public Geometry
    {
        public:

            enum sourceState
            {
                initial,
                playing,
                paused,
                stopped,

                undefined,
            };

            enum sourceType
            {
                Static,
                Streaming,

                Undetermined,
            };

        public:

            // Constructors

            /** For the creation of a multibuffer, streaming, source
             *  \param b2D wether to play as a 2D sound (without distance attenuation, doppler effect, etc.)
             */
            Source();

            /** For the creation of a single-buffer source (mostly SFX, or other non-streaming sounds)
             *  \param sndBuffer the buffer to play from
             */
            Source(boost::shared_ptr<Buffer> sndBuffer);

            ~Source();

            /** Sets the source's buffer.
             *  In case of a non streaming source this function sets the buffer for the source.
             *  \throw std::runtime_error containing error message on failure
             */
            void setBuffer(boost::shared_ptr<Buffer> sndBuffer);

            sourceType getType() const;

            /** Wether this source is used for streaming sound
             *  \return true if this source uses buffer queues to stream from, false otherwise
             */
            bool isStream() const;

            bool isStatic() const;

            /** Retrieve the currect state of the source
             *  \return an enum representing the current play-state of the source
             */
            sourceState getState() const;

            /** Append a buffer to the end of the queue
             *  \param sndBuffer buffer to append to the end of the queue
             */
            void queueBuffer(boost::shared_ptr<Buffer> sndBuffer);

            /** Remove a finished buffer from the top of the queue
             *  \return the buffer that was unqueued
             */
            boost::shared_ptr<Buffer> unqueueBuffer();

            /** Remove all buffers from this source
             */
            void unbuffer();

            /** Tells OpenAL to start playing
             */
            virtual bool play();

            /** Tells OpenAL to stop playing
             */
            virtual void stop();

            /** Tells OpenAL to pause playing
             */
            virtual void pause();

            virtual void rewind();

            unsigned int numProcessedBuffers() const;

            /** Sets the position of the source
             *  \param x X-coordinate of source
             *  \param y Y-coordinate of source
             *  \param z Z-coordinate of source
             */
            virtual void setPosition(float x, float y, float z);
            virtual void setPosition(int x, int y, int z);

            /** Retrieves the position of the source
             *  \param x this will be used to return the X-coordinate in
             *  \param y this will be used to return the Y-coordinate in
             *  \param z this will be used to return the Z-coordinate in
             */
            virtual void getPosition(float& x, float& y, float& z) const;
            virtual void getPosition(int& x, int& y, int& z) const;

            // Functions for setting/getting rotation
            virtual void setRotation(float pitch, float yaw, float roll);
            virtual void setRotation(int pitch, int yaw, int roll);

            virtual void getRotation(float& pitch, float& yaw, float& roll) const;
            virtual void getRotation(int& pitch, int& yaw, int& roll) const;

            // Functions for setting/getting velocity
            virtual void setVelocity(float x, float y, float z);
            virtual void setVelocity(int x, int y, int z);

            virtual void getVelocity(float& x, float& y, float& z) const;
            virtual void getVelocity(int& x, int& y, int& z) const;

            void volume(float gain);
            float volume() const;

            void loop(bool looping);
            bool loop() const;

	    void pitch(float pitch_);
	    float pitch() const;

        private:
            /** Handles the creation of the source
             *  Makes sure an OpenAL source is created and related errors are dealt with
             */
            inline void createSource();

        private:
            // Private copy constructor and copy assignment operator ensures this class cannot be copied
            Source( const Source& );
            const Source& operator=( const Source& );

        private:
            // Identifier towards OpenAL
            ALuint _source;

            // Internal data
            boost::shared_ptr<Buffer>  _buffer;
            std::vector< boost::shared_ptr<Buffer> > _buffers;
    };
}

#endif // SOUND_OPENAL_SOURCE_HPP
