AC_PREREQ(2.63)

# Release version
m4_define(gdata_version_major, 0)
m4_define(gdata_version_minor, 6)
m4_define(gdata_version_micro, 1)

# API version
# TODO: This currently isn't used in the library name; once we hit gdata_version_major=1, that should be changed
m4_define(gdata_api_version_major, 0)
m4_define(gdata_api_version_minor, 0)

AC_INIT([libgdata],[gdata_version_major.gdata_version_minor.gdata_version_micro],[https://bugzilla.gnome.org/enter_bug.cgi?product=libgdata],
        [libgdata],[http://live.gnome.org/libgdata])

AC_CONFIG_SRCDIR([gdata/gdata.h])
AC_CONFIG_HEADERS([config.h])
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([1.9 dist-bzip2 no-dist-gzip check-news subdir-objects])

# Enable silent build when available (Automake 1.11)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PROG_CXX
AM_PROG_CC_C_O
LT_INIT([])
PKG_PROG_PKG_CONFIG

AC_PATH_PROG([GLIB_GENMARSHAL],[glib-genmarshal])
AC_PATH_PROG([GLIB_MKENUMS],[glib-mkenums])

# Requirements
GLIB_REQS=2.19.0
GIO_REQS=2.17.3
SOUP_REQS=2.26.1

# Before making a release, the GDATA_LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A
# Note that versioning started at 2:0:0 to ensure no conflicts with e-d-s' libgdata
# library, whose maximum version was 1:0:0
GDATA_LT_VERSION=7:1:0
AC_SUBST(GDATA_LT_VERSION)

GDATA_VERSION_MAJOR=gdata_version_major
GDATA_VERSION_MINOR=gdata_version_minor
GDATA_VERSION_MICRO=gdata_version_micro
GDATA_API_VERSION=gdata_api_version_major.gdata_api_version_minor
AC_SUBST(GDATA_VERSION_MAJOR)
AC_SUBST(GDATA_VERSION_MINOR)
AC_SUBST(GDATA_VERSION_MICRO)
AC_SUBST(GDATA_API_VERSION)

pkg_modules="glib-2.0 >= $GLIB_REQS libxml-2.0 gthread-2.0 gio-2.0 >= $GIO_REQS libsoup-2.4 >= $SOUP_REQS"
PKG_CHECK_MODULES(GDATA, [$pkg_modules])
AC_SUBST(GDATA_CFLAGS)
AC_SUBST(GDATA_LIBS)

# Optional dependencies
PKG_CHECK_MODULES(GDK, gdk-2.0, have_gdk=yes, have_gdk=no)
if test "x$have_gdk" = "xyes"; then
	AC_DEFINE(HAVE_GDK, 1, [Defined if GDK+ is installed])
fi

AC_SUBST(GDK_CFLAGS)
AC_SUBST(GDK_LIBS)

# GNOME support, which pulls in libsoup-gnome-2.4 to provide transparent proxy support
AC_MSG_CHECKING(whether to build with GNOME support)
AC_ARG_ENABLE(gnome, AS_HELP_STRING([--enable-gnome], [Whether to enable GNOME support]),, enable_gnome=yes)
AC_MSG_RESULT($enable_gnome)

if test "x$enable_gnome" = "xyes"; then
	AC_DEFINE(HAVE_GNOME, 1, [Defined if GNOME support is enabled])
	PKG_CHECK_MODULES(GNOME, [libsoup-gnome-2.4])
fi

AC_SUBST(GNOME_CFLAGS)
AC_SUBST(GNOME_LIBS)

# Internationalisation support
GETTEXT_PACKAGE=gdata
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, ["$GETTEXT_PACKAGE"], [Define to the Gettext package name])
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.40.0])

# Code testing coverage with gcov
AC_MSG_CHECKING(whether to build with gcov testing)
AC_ARG_ENABLE(gcov, AS_HELP_STRING([--enable-gcov], [Whether to enable gcov testing]),, enable_gcov=no)
			
if test "x$enable_gcov" = "xyes"; then
	if test "$GCC" = "no"; then
		AC_MSG_ERROR(not compiling with gcc, which is required for gcov testing)
	fi

	GCOV_CFLAGS="-fprofile-arcs -ftest-coverage"
	GCOV_LIBS="-lgcov"

	AC_SUBST(GCOV_CFLAGS)
	AC_SUBST(GCOV_LIBS)
fi

AM_CONDITIONAL(GCOV_ENABLED, test x$enable_gcov = xyes)
AC_MSG_RESULT($enable_gcov)

# General GNOME macros
GNOME_COMMON_INIT
GNOME_DEBUG_CHECK
GNOME_COMPILE_WARNINGS([maximum])
GNOME_CXX_WARNINGS
GNOME_MAINTAINER_MODE_DEFINES

GOBJECT_INTROSPECTION_CHECK([0.6.7])

AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])

GTK_DOC_CHECK(1.0)

AC_CONFIG_FILES([
Makefile
libgdata.pc
gdata/tests/Makefile
po/Makefile.in
docs/Makefile
docs/reference/Makefile
docs/reference/version.xml
])
AC_OUTPUT
