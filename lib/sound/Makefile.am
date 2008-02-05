SUBDIRS = general openal

noinst_LIBRARIES = libsound.a
noinst_HEADERS = aud.h audio.h audio_id.h cdaudio.h constants.hpp decoding.hpp mixer.h openal_error.h playlist.hpp stream.hpp track.h tracklib.h

libsound_a_SOURCES = audio.cpp audio_id.c cdaudio.cpp constants.cpp decoding.cpp openal_error.c openal_track.cpp playlist.cpp stream.cpp track.c

libsound_a_LIBADD = $(top_builddir)/lib/sound/general/libgeneral.a \
	$(top_builddir)/lib/sound/openal/libopenal.a \
	$(top_builddir)/lib/gamelib/libgamelib.a \
	$(top_builddir)/lib/framework/libframework.a
