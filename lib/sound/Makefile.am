noinst_LIBRARIES = libsound.a
noinst_HEADERS = sound/track.hpp sound/channel.hpp sound/source.hpp \
	sound/track_sample.hpp sound/source_pool.hpp sound/source_lock.hpp
	sound/sample.hpp templates.hpp openal/source.hpp openal/exception.hpp \
	openal/device.hpp openal/context.hpp openal/buffer.hpp openal/devicelist.hpp \
	openal_error.h constants.hpp interface/stringarray.hpp interface/devicelist.hpp \
	types.h audio_id.hpp audio_id.h stream.hpp decoding.hpp general/databuffer.hpp \
	general/geometry.hpp general/physfs_stream.hpp general/raii_counter.hpp sound.h

libsound_a_SOURCES = audio.c cdaudio.c openal_track.c playlist.c track.c

libsound_a_LIBADD = $(top_builddir)/lib/gamelib/libgamelib.a \
	$(top_builddir)/lib/framework/libframework.a
