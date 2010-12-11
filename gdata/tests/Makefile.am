include $(top_srcdir)/gtester.mk

INCLUDES = \
	-I$(top_srcdir)/				\
	-I$(top_srcdir)/gdata				\
	$(GDK_CFLAGS)					\
	-DTEST_FILE_DIR="\"$(top_srcdir)/gdata/tests/\""\
	$(DISABLE_DEPRECATED)				\
	$(WARN_CFLAGS)					\
	$(GDATA_CFLAGS)

LIBS = \
	$(top_builddir)/gdata/libgdata.la	\
	$(GDK_LIBS)				\
	$(GDATA_LIBS)

noinst_PROGRAMS = $(TEST_PROGS)

TEST_SRCS = common.c common.h

TEST_PROGS			+= general
general_SOURCES			 = general.c $(TEST_SRCS)

TEST_PROGS			+= youtube
youtube_SOURCES			 = youtube.c $(TEST_SRCS)

TEST_PROGS			+= calendar
calendar_SOURCES		 = calendar.c $(TEST_SRCS)

TEST_PROGS			+= contacts
contacts_SOURCES		 = contacts.c $(TEST_SRCS)

TEST_PROGS			+= picasaweb
picasaweb_SOURCES		 = picasaweb.c $(TEST_SRCS)

TEST_PROGS			+= documents
documents_SOURCES		 = documents.c $(TEST_SRCS)

TEST_PROGS			+= memory
memory_SOURCES			 = memory.c $(TEST_SRCS)

TEST_PROGS			+= perf
perf_SOURCES			 = perf.c $(TEST_SRCS)

EXTRA_DIST += \
	photo.jpg		\
	sample.ogg		\
	test.doc		\
	test.ods		\
	test.odt		\
	test.ppt		\
	test.xls		\
	test_updated.odt	\
	test_updated_file.ppt

-include $(top_srcdir)/git.mk
