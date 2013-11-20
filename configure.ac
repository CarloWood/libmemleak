dnl Run autogen.sh to produce a configure script.

dnl CW_VERSION_MAJOR               : Increment when the whole library was rewritten; or some VERY MAJOR change was made.
dnl CW_VERSION_MINOR               : Increment when major changes have occured, like removing interfaces.
dnl CW_VERSION_REVISION            : Increment every public release; or set to 0 when CW_VERSION_MINOR was incremented.

define(CW_VERSION_MAJOR, 0)
define(CW_VERSION_MINOR, 0)
define(CW_VERSION_REVISION, 0)

dnl Version info, see http://www.gnu.org/software/libtool/manual.html#Updating-version-info
dnl
dnl CW_INTERFACE_VERSION           : Current interface version, increment whenever the interface is changed.
dnl CW_INTERFACE_VERSION_REVISION  : Revision, increment every public release; or set to 0 when the interface is changed.
dnl CW_INTERFACE_AGE               : Increment if interfaces have been added; or set to 0 when interfaces have been removed.

define(CW_INTERFACE_VERSION, 0)
define(CW_INTERFACE_VERSION_REVISION, 0)
define(CW_INTERFACE_AGE, 0)

define(CW_PACKAGE_NAME, [libmemleak])
define(CW_BUGREPORT, [aleric.inglewood@gmail.com])

dnl Package name and version
AC_INIT(CW_PACKAGE_NAME, CW_VERSION_MAJOR.CW_VERSION_MINOR.CW_VERSION_REVISION, CW_BUGREPORT)

dnl Put resulting configuration defines in this header file.
AC_CONFIG_HEADERS([config.h])

dnl Put revision of configure.ac in configure script.
AC_REVISION($Revision: 20 $)dnl

dnl Ensure that the directory specified with --srcdir was correct
dnl by demanding that it contains `libmemleak.pc.in'.
AC_CONFIG_SRCDIR([libmemleak.pc.in])

dnl Automake options.
AM_INIT_AUTOMAKE

dnl Include maintainer mode targets
AM_MAINTAINER_MODE

dnl Check if we are the real maintainer
AM_CONDITIONAL(REAL_MAINTAINER, test -z "$MAINTAINER_MODE_TRUE" -a -n "$TOPPROJECT")

dnl Because we use cwautomacros.
CW_AUTOMACROS([20091119])

dnl We use doxygen for documentation.
CW_DOXYGEN

dnl Doxygen output directory.
if test -z "$REAL_MAINTAINER_FALSE"; then
  OUTPUT_DIRECTORY=""
fi
AC_SUBST(OUTPUT_DIRECTORY)

dnl Demand at least version 2.57 of autoconf.
AC_PREREQ(2.57)

dnl Check for compiler and preprocessor
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CXXCPP

dnl Add --enable-debug (DEBUG, DOXYGEN_DEBUG), --enable-optimise and --enable-profile options.
dnl Update CFLAGS accordingly.
CW_OPG_CFLAGS([-W -Wall -Wundef -Wpointer-arith -Wwrite-strings -Winline])

dnl Add --enable-pch (USE_PCH (macro)) option.
dnl Update USE_PCH (automake conditional) and PCHFLAGS accordingly.
CW_PCHFLAGS

dnl Each Makefile.am should use DEFS = @DEFS@. Set DEFS here.
DEFS="-DHAVE_CONFIG_H"
AC_SUBST(DEFS)

dnl These CFLAGS are used in the .pc file.
PCCFLAGS=
AC_SUBST(PCCFLAGS)

dnl Checks for other programs.
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl Libtool version info
VERSIONINFO="CW_INTERFACE_VERSION:CW_INTERFACE_VERSION_REVISION:CW_INTERFACE_AGE"
AC_SUBST(VERSIONINFO)

dnl Detect boost.
#CW_BOOST([$cw_enable_static], [no], [])

dnl Detect libbfd
AC_CHECK_LIB(bfd, bfd_init, [LIBBFD=-lbfd])
AC_CHECK_LIB(readline, readline, [LIBREADLINE=-lreadline])

AC_SUBST(LIBBFD)
AC_SUBST(LIBREADLINE)

dnl Output the other configuration files.
AC_CONFIG_FILES(
        [Makefile]
        [src/Makefile]
	[src/rb_tree/Makefile]
	[src/include/Makefile]
	[libmemleak.pc]
        [doc/Makefile]
        [doc/doxygen.config]
	[doc/html.header]
	[doc/html.footer])

AC_OUTPUT
