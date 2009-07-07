#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([darts-clone], [0.32f], [syata@acm.org])
AC_CONFIG_SRCDIR([src/darts-clone.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

AC_LANG_CPLUSPLUS

# Checks for libraries.

# Checks for C++ header files.
AC_CHECK_HEADERS([dawgdic/dawg.h], ,
	[AC_MSG_ERROR(dawgdic 0.30 or later: not found)])

# Checks for header files.
AC_CHECK_HEADERS([sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([strchr clock_gettime gettimeofday])

AC_CONFIG_FILES([Makefile src/Makefile test/Makefile])
AC_OUTPUT
