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

#ifndef __INCLUDED_LIB_SOUND_GENERAL_GEOMETRY_VERTEX_HPP__
#define __INCLUDED_LIB_SOUND_GENERAL_GEOMETRY_VERTEX_HPP__

class Geometry
{
    public:
        // Virtual, empty definition of destructor because this automatically
        // turns all derived classes' destructors into virtuals as well
        // wether the virtual keyword is specified or not.
        virtual ~Geometry() {}

        // Functions for setting/getting position
        virtual void setPosition(float x, float y, float z) = 0;
        virtual void setPosition(int x, int y, int z) = 0;

        virtual void getPosition(float& x, float& y, float& z) const = 0;
        virtual void getPosition(int& x, int& y, int& z) const = 0;

        // Functions for setting/getting rotation
        virtual void setRotation(float pitch, float yaw, float roll) = 0;
        virtual void setRotation(int pitch, int yaw, int roll) = 0;

        virtual void getRotation(float& pitch, float& yaw, float& roll) const = 0;
        virtual void getRotation(int& pitch, int& yaw, int& roll) const = 0;

        // Functions for setting/getting velocity
        virtual void setVelocity(float x, float y, float z) = 0;
        virtual void setVelocity(int x, int y, int z) = 0;

        virtual void getVelocity(float& x, float& y, float& z) const = 0;
        virtual void getVelocity(int& x, int& y, int& z) const = 0;
};

#endif // __INCLUDED_LIB_SOUND_GENERAL_GEOMETRY_VERTEX_HPP__
