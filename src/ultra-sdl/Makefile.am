AM_CFLAGS = -I../../include
AM_CXXFLAGS = -I$(srcdir)/../../include -I../../include

GLAD_HEADER_FILES = \
	../../include/glad/glad.h \
	../../include/KHR/khrplatform.h

GLAD_SOURCE_FILES = \
	../glad.c

GLAD_FILES = $(GLAD_HEADER_FILES) $(GLAD_SOURCE_FILES)

SHADER_SOURCES = \
	shader/tile_vert.c \
	shader/sprite_vert.c \
	shader/tex_geom.c \
	shader/tex_frag.c \
	shader/box_vert.c \
	shader/line_geom.c \
	shader/line_frag.c \
	shader/passthrough.c \
	shader/quad.c

BUILT_SOURCES = $(SHADER_SOURCES)

CLEANFILES = $(GLAD_FILES) $(SHADER_SOURCES)

lib_LTLIBRARIES = libultra-sdl.la
libultra_sdl_la_SOURCES = window.cc $(GLAD_SOURCE_FILES) $(SHADER_SOURCES)

window.lo: $(GLAD_HEADER_FILES)
window.lo: CXXFLAGS += $(SDL2_CFLAGS)

$(GLAD_FILES):
	glad --api gl=4.2 --generator c --out-path ../../

SUFFIXES: .glsl
.glsl.c:
	xxd -i $^ | sed 's/ [^ ]*shader_\(.*\) / shader_\1 /g' > $@
