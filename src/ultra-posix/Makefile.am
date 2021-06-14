AM_CXXFLAGS = -I$(srcdir)/../../include

lib_LTLIBRARIES = libultra-posix.la
libultra_posix_la_SOURCES = \
	dynamic_library.cc \
	file.cc \
	mkdirp.cc \
	resource_loader.cc \
	settings_manager.cc \
	util.cc

file.$(OBJEXT): $(BINRELOC_HEADER_FILES)
