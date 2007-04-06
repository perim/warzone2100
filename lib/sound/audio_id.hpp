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

#ifndef _AUDIO_ID_HPP_
#define _AUDIO_ID_HPP_

#include "types.h"
#include <string>
#include <map>

// Associative array which maps filenames (of audio files) to track id numbers
class AudioIDmap : public std::map<std::string, sndTrackID>
{
    public:
        /** Provides a reference to an instance of AudioIDmap
         *  If there currently exists no instance of AudioIDmap it creates one
         *  \return a const (read-only) reference to a singleton instance of AudioIDmap&
         */
        static const AudioIDmap& Instance();

        /** Destroys the singleton instance of AudioIDmap, provided that it exists of course.
         */
        static void DestroyInstance();

    protected:
        /** Default constructor
         *  This constructor is protected to ensure that only one instance (singleton) can
         *  ever exist of AudioIDmap. It also enables derived classes to use AudioIDmap.
         */
        AudioIDmap();

    private:
        // Private copy constructor and copy assignment operator ensures this class cannot be copied
        AudioIDmap( const AudioIDmap& );
        const AudioIDmap& operator=( const AudioIDmap& );

    private:
        // Singleton instance pointer
        static AudioIDmap* _instance;

    private:
        // Proxy iterator to aid in inserting AUDIO_ID_MAP in an std::map for construction of associative arrays
        template <typename iterator>
        class pair_iterator
        {
            private:
                iterator _iter;
            public:
                // We only provide a const reference since otherwise we'd need a proxy class for references as well
                typedef const std::pair<std::string, sndTrackID>  const_reference;
                typedef pair_iterator<iterator>                   this_type;

            public:
                pair_iterator(const iterator& _Iterator) : _iter(_Iterator) {}

            public:
                // Iterator interface operators (==, !=, ++prefix, *dereference)
                inline bool operator==(const this_type& i) { return _iter == i._iter; }
                inline bool operator!=(const this_type& i) { return _iter != i._iter; }
                inline void operator++() { ++_iter; }

                // The dereference operator constructs the pair
                inline const_reference operator*() const
                {
                    return make_pair(_iter->key(), _iter->value());
                }
        };
};

#endif  // _AUDIO_ID_HPP_
