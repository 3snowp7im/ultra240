AM_CXXFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/include

SHADER_SOURCES = \
	shader/tile_vert.c \
	shader/sprite_vert.c \
	shader/tex_geom.c \
	shader/tex_frag.c

BUILT_SOURCES = $(SHADER_SOURCES)

CLEANFILES = $(SHADER_SOURCES)

lib_LTLIBRARIES = libultra-gl.la
libultra_gl_la_SOURCES = \
	renderer.cc \
	$(SHADER_SOURCES)

SUFFIXES: .glsl
.glsl.c:
	xxd -i $^ | sed 's/ [^ ]*shader_\(.*\) / shader_\1 /g' > $@
