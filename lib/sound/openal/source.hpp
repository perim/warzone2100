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
#include "context.hpp"
#include "../bases/geometry.hpp"

// Needed to declare pointers for soundBuffer
class soundBuffer;

class soundSource : public Geometry
{
    public:

        enum sourceState {
            initial,
            playing,
            paused,
            stopped,

            undefined
        };

    public:

        // Constructors

        /** For the creation of a multibuffer, streaming, buffer
         *  \param sndContext the context in which sound from this source should be rendered
         *  \param b2D wether to play as a 2D sound (without distance attenuation, doppler effect, etc.)
         */
        soundSource(boost::shared_ptr<soundContext> sndContext);

        /** For the creation of a single-buffer source (mostly SFX, or other non-streaming sounds)
         *  \param sndContext the context in which sound from this source should be rendered
         *  \param sndBuffer the buffer to play from
         *  \param b2D       wether to play as a 2D sound (without distance attenuation, doppler effect, etc.)
         */
        soundSource(boost::shared_ptr<soundContext> sndContext, boost::shared_ptr<soundBuffer> sndBuffer);

        ~soundSource();

        /** Sets the source's buffer.
         *  In case of a non streaming source this function sets the buffer for the source.
         *  \throw std::string containing error message on failure
         */
        void setBuffer(boost::shared_ptr<soundBuffer> sndBuffer);

        /** Wether all sound routed through this source will be rendered as 2D.
         *  \return true if all routed sound is displayed 2D, false otherwise
         */
        inline bool is2D()
        {
            return bIs2D;
        }

        /** Wether this source is used for streaming sound
         *  \return true if this source uses buffer queues to stream from, false otherwise
         */
        inline bool isStream()
        {
            return bIsStream;
        }

        /** Retrieve the currect state of the source
         *  \return an enum representing the current play-state of the source
         */
        sourceState getState();

        /** Append a buffer to the end of the queue
         *  \param sndBuffer buffer to append to the end of the queue
         */
        void queueBuffer(boost::shared_ptr<soundBuffer> sndBuffer);

        /** Remove a finished buffer from the top of the queue
         *  \return the buffer that was unqueued
         */
        boost::shared_ptr<soundBuffer> unqueueBuffer();

        /** Tells OpenAL to start playing
         */
        inline virtual bool play()
        {
            context->makeCurrent();

            // Clear current error state
            alGetError();

            alSourcePlay(source);

            if(alGetError() != AL_NO_ERROR)
                return false;

            return true;
        }

        /** Tells OpenAL to stop playing
         */
        inline virtual void stop()
        {
            context->makeCurrent();
            alSourceStop(source);
        }

        inline unsigned int numProcessedBuffers()
        {
            context->makeCurrent();

            int count;
            alGetSourcei(source, AL_BUFFERS_PROCESSED, &count);

            return count;
        }

        /** Sets the position of the source
         *  \param x X-coordinate of source
         *  \param y Y-coordinate of source
         *  \param z Z-coordinate of source
         */
        inline virtual void setPosition(float x, float y, float z)
        {
            context->makeCurrent();
            alSource3f(source, AL_POSITION, x, y, z);
        }

        inline virtual void setPosition(int x, int y, int z)
        {
            context->makeCurrent();
            alSource3i(source, AL_POSITION, x, y, z);
        }

        /** Retrieves the position of the source
         *  \param x this will be used to return the X-coordinate in
         *  \param y this will be used to return the Y-coordinate in
         *  \param z this will be used to return the Z-coordinate in
         */
        inline virtual void getPosition(float& x, float& y, float& z)
        {
            context->makeCurrent();
            alGetSource3f(source, AL_POSITION, &x, &y, &z);
        }
        inline virtual void getPosition(int& x, int& y, int& z)
        {
            context->makeCurrent();
            alGetSource3i(source, AL_POSITION, &x, &y, &z);
        }

        // Functions for setting/getting rotation
        // * Is this implementation of setting/getting AL_DIRECTION correct?
        // * Honestly I can't tell, because the OpenAL specification neglects
        // * to mention how the vector of 3 values is used for specifying direction
        inline virtual void setRotation(float pitch, float yaw, float roll)
        {
            context->makeCurrent();
            alSource3f(source, AL_DIRECTION, pitch, yaw, roll);
        }
        inline virtual void setRotation(int pitch, int yaw, int roll)
        {
            context->makeCurrent();
            alSource3i(source, AL_DIRECTION, pitch, yaw, roll);
        }

        inline virtual void getRotation(float& pitch, float& yaw, float& roll)
        {
            context->makeCurrent();
            alGetSource3f(source, AL_DIRECTION, &pitch, &yaw, &roll);
        }
        inline virtual void getRotation(int& pitch, int& yaw, int& roll)
        {
            context->makeCurrent();
            alGetSource3i(source, AL_DIRECTION, &pitch, &yaw, &roll);
        }

        // Functions for setting/getting velocity
        inline virtual void setVelocity(float x, float y, float z)
        {
            context->makeCurrent();
            alSource3f(source, AL_VELOCITY, x, y, z);
        }
        inline virtual void setVelocity(int x, int y, int z)
        {
            context->makeCurrent();
            alSource3i(source, AL_VELOCITY, x, y, z);
        }

        inline virtual void getVelocity(float& x, float& y, float& z)
        {
            context->makeCurrent();
            alGetSource3f(source, AL_VELOCITY, &x, &y, &z);
        }
        inline virtual void getVelocity(int& x, int& y, int& z)
        {
            context->makeCurrent();
            alGetSource3i(source, AL_VELOCITY, &x, &y, &z);
        }

    private:
        /** Handles the creation of the source
         *  Makes sure an OpenAL source is created and related errors are dealt with
         */
        inline void createSource();

    private:
        // Private copy constructor and copy assignment operator ensures this class cannot be copied
        soundSource( const soundSource& );
        const soundSource& operator=( const soundSource& );

    private:
        // Identifier towards OpenAL
        ALuint source;

        // Internal data
        boost::shared_ptr<soundContext> context;
        boost::shared_ptr<soundBuffer>  buffer;
        std::vector< boost::shared_ptr<soundBuffer> > buffers;

        // Internal state
        const bool bIs2D;
        const bool bIsStream;
};

#endif // SOUND_OPENAL_SOURCE_HPP
