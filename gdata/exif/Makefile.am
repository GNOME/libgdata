# Library
gdataexifincludedir = $(pkgincludedir)/gdata/exif
gdataexifinclude_HEADERS = gdata-exif-tags.h

noinst_LTLIBRARIES = libgdataexif.la

libgdataexif_la_SOURCES = gdata-exif-tags.c

libgdataexif_la_CPPFLAGS = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/gdata		\
	-I$(top_srcdir)/gdata/exif	\
	$(DISABLE_DEPRECATED)		\
	$(AM_CPPFLAGS)

libgdataexif_la_CFLAGS = \
	$(GDATA_CFLAGS)	\
	$(WARN_CFLAGS)	\
	$(AM_CFLAGS)	\
	-D_GNU_SOURCE

libgdataexif_la_LIBADD = \
	$(GDATA_LIBS)

libgdataexif_la_LDFLAGS = \
	-no-undefined	\
	$(AM_LDFLAGS)

-include $(top_srcdir)/git.mk
