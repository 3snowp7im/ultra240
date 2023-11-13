AC_INIT([ultra240], [0.0.1])
PKG_CHECK_MODULES([SDL2], [sdl2])
PKG_CHECK_MODULES([YAML], [yaml-cpp])
AC_CHECK_PROG([HAVE_GLAD], [glad], [yes])
AS_IF([test x"$HAVE_GLAD" != x"yes"], [AC_MSG_ERROR([Missing glad])])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_FILES([
  Makefile
  src/ultra/Makefile
  src/ultra-posix/Makefile
  src/ultra-sdl/Makefile
  src/ultra-yaml/Makefile
  ultra240.pc
])
AC_PROG_CC
AC_PROG_CXX
LT_INIT
AC_OUTPUT
