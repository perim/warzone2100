SUBDIRS = general interface openal

noinst_LIBRARIES = libsound.a
noinst_HEADERS = aud.h audio.h audio_id.h cdaudio.h mixer.h oggvorbis.h openal_error.h track.h tracklib.h

libsound_a_SOURCES = audio.cpp audio_id.c cdaudio.cpp oggvorbis.c openal_error.c openal_track.cpp playlist.c track.c

libsound_a_LIBADD = $(top_builddir)/lib/sound/general/libgeneral.a \
	$(top_builddir)/lib/sound/interface/libinterface.a \
	$(top_builddir)/lib/gamelib/libgamelib.a \
	$(top_builddir)/lib/framework/libframework.a
