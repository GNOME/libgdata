# Library
gdatageorssincludedir = $(pkgincludedir)/gdata/georss
gdatageorssinclude_HEADERS = gdata-georss-where.h

noinst_LTLIBRARIES = libgdatageorss.la

libgdatageorss_la_SOURCES = gdata-georss-where.c

libgdatageorss_la_CPPFLAGS = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/gdata		\
	-I$(top_srcdir)/gdata/georss	\
	$(DISABLE_DEPRECATED)		\
	$(AM_CPPFLAGS)

libgdatageorss_la_CFLAGS = \
	$(GDATA_CFLAGS)	\
	$(WARN_CFLAGS)	\
	$(AM_CFLAGS)	\
	-D_GNU_SOURCE

libgdatageorss_la_LIBADD = \
	$(GDATA_LIBS)

libgdatageorss_la_LDFLAGS = \
	-no-undefined	\
	$(AM_LDFLAGS)

-include $(top_srcdir)/git.mk
