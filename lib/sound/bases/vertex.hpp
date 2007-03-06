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

#ifndef LIB_BASECLASS_VERTEX_HPP
#define LIB_BASECLASS_VERTEX_HPP

#include <boost/smart_ptr.hpp>

class Vertex
{
    public:
        // Virtual, empty definition of destructor because this automatically
	// turns all derived classes' destructors into virtuals as well
	// wether the virtual keyword is specified or not.
        virtual ~Vertex() {}

        virtual void setPos(float x, float y, float z) = 0;
        virtual void setPos(int x, int y, int z) = 0;

        virtual void getPos(float& x, float& y, float& z) = 0;
        virtual void getPos(int& x, int& y, int& z) = 0;
};

#endif // LIB_BASECLASS_VERTEX_HPP
