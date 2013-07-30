AC_PREREQ(2.65)

# Release version
m4_define(gdata_version_major, 0)
m4_define(gdata_version_minor, 13)
m4_define(gdata_version_micro, 4)

# API version
# TODO: This currently isn't used in the library name; once we hit gdata_version_major=1, that should be changed
m4_define(gdata_api_version_major, 0)
m4_define(gdata_api_version_minor, 0)

AC_INIT([libgdata],[gdata_version_major.gdata_version_minor.gdata_version_micro],[https://bugzilla.gnome.org/enter_bug.cgi?product=libgdata],[libgdata],[http://live.gnome.org/libgdata])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
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
GLIB_REQS=2.31.0
GIO_REQS=2.17.3
SOUP_REQS=2.37.91
OAUTH_REQS=0.9.4
GTK_REQS=2.91.2
GOA_REQS=3.2

# Before making a release, the GDATA_LT_VERSION string should be modified. The string is of the form c:r:a. Follow these instructions sequentially:
#
#  1. If the library source code has changed at all since the last update, then increment revision (‘c:r:a’ becomes ‘c:r+1:a’).
#  2. If any interfaces have been added, removed, or changed since the last update, increment current, and set revision to 0.
#  3. If any interfaces have been added since the last public release, then increment age.
#  4. If any interfaces have been removed or changed since the last public release, then set age to 0.
#
# Note that versioning started at 2:0:0 to ensure no conflicts with e-d-s' libgdata library, whose maximum version was 1:0:0
GDATA_LT_VERSION=18:0:5
AC_SUBST(GDATA_LT_VERSION)

GDATA_VERSION_MAJOR=gdata_version_major
GDATA_VERSION_MINOR=gdata_version_minor
GDATA_VERSION_MICRO=gdata_version_micro
GDATA_API_VERSION=gdata_api_version_major.gdata_api_version_minor
GDATA_API_VERSION_MAJOR=gdata_api_version_major
GDATA_API_VERSION_MINOR=gdata_api_version_minor
AC_SUBST(GDATA_VERSION_MAJOR)
AC_SUBST(GDATA_VERSION_MINOR)
AC_SUBST(GDATA_VERSION_MICRO)
AC_SUBST(GDATA_API_VERSION)
AC_SUBST(GDATA_API_VERSION_MAJOR)
AC_SUBST(GDATA_API_VERSION_MINOR)

GDATA_PACKAGES_PUBLIC="gobject-2.0 glib-2.0 >= $GLIB_REQS gio-2.0 >= $GIO_REQS libxml-2.0 libsoup-2.4 >= $SOUP_REQS"
GDATA_PACKAGES_PRIVATE="gthread-2.0 oauth >= $OAUTH_REQS"
GDATA_PACKAGES="$GDATA_PACKAGES_PUBLIC $GDATA_PACKAGES_PRIVATE"
AC_SUBST([GDATA_PACKAGES_PUBLIC])
AC_SUBST([GDATA_PACKAGES_PRIVATE])
AC_SUBST([GDATA_PACKAGES])

PKG_CHECK_MODULES(GDATA, [$GDATA_PACKAGES])
AC_SUBST(GDATA_CFLAGS)
AC_SUBST(GDATA_LIBS)

# Optional dependencies
PKG_CHECK_MODULES(GDK_PIXBUF, gdk-pixbuf-2.0 >= 2.14, have_gdk_pixbuf=yes, have_gdk_pixbuf=no)
if test "x$have_gdk_pixbuf" = "xyes"; then
	AC_DEFINE(HAVE_GDK_PIXBUF, 1, [Defined if gdk-pixbuf is installed])
fi

AC_SUBST(GDK_PIXBUF_CFLAGS)
AC_SUBST(GDK_PIXBUF_LIBS)

PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= $GTK_REQS], [have_gtk=yes], [have_gtk=no])
AM_CONDITIONAL([HAVE_GTK], [test "x$have_gtk" = "xyes"])

# GNOME support, which pulls in libsoup-gnome-2.4 to provide transparent proxy support and gcr-base-3 to provide non-pageable memory
AC_MSG_CHECKING(whether to build with GNOME support)
AC_ARG_ENABLE(gnome, AS_HELP_STRING([--enable-gnome], [Whether to enable GNOME support]),, enable_gnome=yes)
AC_MSG_RESULT($enable_gnome)
AM_CONDITIONAL([ENABLE_GNOME], [test "x$enable_gnome" = "xyes"])

# GNOME Online Accounts support -- only used if GNOME support is also enabled
# Pulls in gdata/gdata-goa-authorizer.[ch]
AC_MSG_CHECKING(whether to build with GOA support)
AC_ARG_ENABLE(goa, AS_HELP_STRING([--enable-goa], [Whether to enable GOA support]),, enable_goa=yes)
AC_MSG_RESULT($enable_goa)
AM_CONDITIONAL([ENABLE_GOA], [test "x$enable_goa" = "xyes" -a "x$enable_gnome" = "xyes"])

if test "x$enable_gnome" = "xyes"; then
	GNOME_PACKAGES_PUBLIC=""
	GNOME_PACKAGES_PRIVATE="libsoup-gnome-2.4 gcr-base-3 libxml-2.0"
	if test "x$enable_goa" = "xyes"; then
		GNOME_PACKAGES_PUBLIC="$GNOME_PACKAGES_PUBLIC"
		GNOME_PACKAGES_PRIVATE="$GNOME_PACKAGES_PRIVATE goa-1.0 >= $GOA_REQS"
		GOA_ENABLED="true"
		AC_DEFINE(HAVE_GOA, 1, [Define if GOA support is enabled])
	fi
	GNOME_PACKAGES="$GNOME_PACKAGES_PUBLIC $GNOME_PACKAGES_PRIVATE"
	AC_DEFINE(HAVE_GNOME, 1, [Defined if GNOME support is enabled])
	PKG_CHECK_MODULES([GNOME], [$GNOME_PACKAGES])
else
	GNOME_PACKAGES_PUBLIC=""
	GNOME_PACKAGES_PRIVATE=""
	GNOME_PACKAGES=""
fi
AC_SUBST([GNOME_PACKAGES_PUBLIC])
AC_SUBST([GNOME_PACKAGES_PRIVATE])
AC_SUBST([GNOME_PACKAGES])
AC_SUBST([GOA_ENABLED])

# Various necessary functions and headers
AC_CHECK_FUNCS([strchr])
AC_CHECK_FUNCS([strstr])
AC_CHECK_FUNCS([strtol])
AC_CHECK_FUNCS([strtoul])
AC_CHECK_HEADERS([sys/time.h])

# Internationalisation support
GETTEXT_PACKAGE=gdata
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, ["$GETTEXT_PACKAGE"], [Define to the Gettext package name])
AC_SUBST(GETTEXT_PACKAGE)
IT_PROG_INTLTOOL([0.40.0])

# Code coverage
GNOME_CODE_COVERAGE

# General GNOME macros
GNOME_DEBUG_CHECK
GNOME_COMPILE_WARNINGS([maximum])
GNOME_MAINTAINER_MODE_DEFINES

GOBJECT_INTROSPECTION_CHECK([0.9.7])

VAPIGEN_CHECK()

AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])

GTK_DOC_CHECK([1.14],[--flavour no-tmpl])

AC_CONFIG_FILES([
Makefile
libgdata.pc
gdata/gdata-version.h
gdata/tests/Makefile
po/Makefile.in
docs/Makefile
docs/reference/Makefile
docs/reference/version.xml
])
AC_OUTPUT
