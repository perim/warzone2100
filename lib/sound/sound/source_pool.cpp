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

#include "source_pool.hpp"
#include "source.hpp"
#include "../openal/exception.hpp"

extern "C"
{
#include "../../framework/frame.h"
#include "../../framework/debug.h"
#undef bool
#undef true
#undef false
#undef __bool_true_false_are_defined
}

namespace Sound
{
    SourcePool::SourcePool(boost::shared_ptr<OpenAL::Context> context)
    {
        try
        {
            while (1)
            {
                boost::shared_ptr<Source> newSource(new Source(context));
                _freePool.push_back(newSource);
            }
        }
        catch (OpenAL::out_of_non_memory_resources& e)
        {
            debug(LOG_SOUND, "Sound::SourcePool::SourcePool: caught expected exception: \"%s\"", e.what());
        }

        debug(LOG_SOUND, "Sound::SourcePool::SourcePool: succesfully registered %u Sound::Source objects", _freePool.size());
    }

    boost::shared_ptr<Source> SourcePool::requestFreeSource()
    {
        if (_freePool.empty())
            updatePools();

        // If the pool is still empty then return a "null" pointer
        if (_freePool.empty())
            return boost::shared_ptr<Source>();

        // Extract a source from the free pool
        boost::shared_ptr<Source> soundSource(_freePool.back());
        _freePool.pop_back();

        // Add this source to the volatile pool
        _volatilePool.push_back(soundSource);

        return soundSource;
    }

    boost::shared_ptr<Source> SourcePool::requestSource()
    {
        // First try grabbing a free source
        boost::shared_ptr<Source> soundSource(requestFreeSource());

        // If we got a free source then return it
        if (soundSource)
            return soundSource;

        // Otherwise grab the first source from the volatile pool
        while (!_volatilePool.empty())
        {
            // Remove "null" pointers from the front
            if (!_volatilePool.front())
                _volatilePool.pop_front();

            // Apparently the first pointer is no null pointer
            // So move it to the back of the pool
            soundSource = _volatilePool.front();
            _volatilePool.pop_front();
            _volatilePool.push_back(soundSource);

            // Unlock the source for usage
            soundSource->unlock();

            // Return the source for usage by the next client
            return soundSource;
        }

        return soundSource;
    }

    boost::shared_ptr<Source> SourcePool::requestStaticSource()
    {
        if (_freePool.empty())
            updatePools();

        // If there now are souces in the free pool
        if (!_freePool.empty())
        {
            // Extract a source from the free pool
            boost::shared_ptr<Source> soundSource(_freePool.back());
            _freePool.pop_back();

            // Add this source to the static pool
            _staticPool.push_back(soundSource);

            return soundSource;
        }

        // Otherwise grab the first source from the volatile pool
        while (!_volatilePool.empty())
        {
            // Remove "null" pointers from the front
            if (!_volatilePool.front())
                _volatilePool.pop_front();

            // Apparently the first pointer is no null pointer
            // So move it to the back of the pool
            boost::shared_ptr<Source> soundSource(_volatilePool.front());
            _volatilePool.pop_front();
            _volatilePool.push_back(soundSource);

            // Unlock the source for usage
            soundSource->unlock();

            // Return the source for usage by the next client
            return soundSource;
        }

        // Apparently all sources are currently located in the static pool (where we're not allowed to take them from)
        // So return a NULL pointer
        return boost::shared_ptr<Source> ();
    }

    void SourcePool::updatePools()
    {
        // Update volatile pool
        for (std::deque<boost::shared_ptr<Source> >::iterator curSource = _volatilePool.begin(); curSource != _volatilePool.end();)
        {
            if (*curSource
             && (*curSource)->locked())
            {
                // Skip this source; it is apparently being used
                ++curSource;
                continue;
            }

            if (*curSource
             && !(*curSource)->locked())
            {
                // Move this source to the free pool
                _freePool.push_back(*curSource);
                curSource->reset();
            }

            // Remove "null" pointers (but only from the front)
            if (curSource == _volatilePool.begin()
             && !*curSource)
            {
                _volatilePool.erase(curSource);
                curSource = _volatilePool.begin();
            }

            ++curSource;
        }

        // Update static pool
        for (std::list<boost::shared_ptr<Source> >::iterator curSource = _staticPool.begin(); curSource != _staticPool.end();)
        {
            // Remove "null" pointers and unused sources
            if (!*curSource
             || !(*curSource)->locked())
            {
                // Move unused sources back to the free pool
                if (*curSource)
                    _freePool.push_back(*curSource);

                // Remove this source from the static pool
                std::list<boost::shared_ptr<Source> >::iterator eraseSource = curSource++;
                _staticPool.erase(eraseSource);
                continue;
            }

            ++curSource;
        }
    }
}
