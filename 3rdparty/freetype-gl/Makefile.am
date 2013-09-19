M_CPPFLAGS = -DYY_NO_INPUT $(WZ_CPPFLAGS) 
AM_CFLAGS = $(WZ_CFLAGS) $(OPENGL_CFLAGS)
AM_CXXFLAGS = $(WZ_CXXFLAGS) $(OPENGL_CFLAGS)

noinst_HEADERS = texture-font.h vec234.h vector.c vector.h texture-atlas.h freetype-gl.h opengl.h platform.h 

noinst_LIBRARIES = libfreetypegl.a
nodist_libfreetypegl_a_SOURCES = texture-atlas.c texture-font.c vector.c platform.c
