#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([gldemo], [0.1], [keithw@cs.stanford.edu])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src/frontend/example.cc])
AC_CONFIG_HEADERS([config.h])

# Add picky CXXFLAGS
CXX17_FLAGS="-std=c++17 -pthread"
PICKY_CXXFLAGS="-Wpedantic -Wall -Wextra -Weffc++ -Werror"
AC_SUBST([CXX17_FLAGS])
AC_SUBST([PICKY_CXXFLAGS])

# Change default CXXflags
: ${CXXFLAGS="-g -Ofast -march=native"}

# Checks for programs.
AC_PROG_CXX
AC_PROG_RANLIB
AC_PATH_PROG([CLANG_FORMAT], [clang-format], [])
AS_IF([test x"$CLANG_FORMAT" = x],
  [AC_MSG_ERROR([cannot find clang-format])])

# Checks for libraries.
PKG_CHECK_MODULES([GL], [gl])
PKG_CHECK_MODULES([GLU], [glu])
PKG_CHECK_MODULES([GLFW3], [glfw3])
PKG_CHECK_MODULES([GLEW], [glew])

# Checks for header files.
AC_LANG_PUSH(C++)
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CXX17_FLAGS"

CPPFLAGS="$save_CPPFLAGS"
AC_LANG_POP(C++)

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    src/util/Makefile
    src/frontend/Makefile
])
AC_OUTPUT
