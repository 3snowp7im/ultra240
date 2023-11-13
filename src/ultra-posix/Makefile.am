AM_CXXFLAGS = -I$(srcdir)/../../include

lib_LTLIBRARIES = libultra-posix.la
libultra_posix_la_SOURCES = \
	dynamic_library.cc \
	file.cc \
	mkdirp.cc \
	path_manager.cc \
	settings_manager.cc

file.$(OBJEXT): $(BINRELOC_HEADER_FILES)
