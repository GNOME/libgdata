# Enums
GDATA_DOCUMENTS_ENUM_FILES = \
	gdata-documents-enums.c	\
	gdata-documents-enums.h

gdata-documents-enums.h: $(gdata_documents_headers) Makefile
	(cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#ifndef GDATA_DOCUMENTS_ENUMS_H\n#define GDATA_DOCUMENTS_ENUMS_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define GDATA_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* !GDATA_DOCUMENTS_ENUMS_H */" $(gdata_documents_headers)) > gdata-documents-enums.h.tmp \
	&& sed "s/g_data/gdata/" gdata-documents-enums.h.tmp > gdata-documents-enums.h \
	&& rm -f gdata-documents-enums.h.tmp

gdata-documents-enums.c: $(gdata_documents_headers) Makefile gdata-documents-enums.h
	(cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#include \"gdata-documents-service.h\"\n#include \"gdata-documents-text.h\"\n#include \"gdata-documents-spreadsheet.h\"\n#include \"gdata-documents-presentation.h\"\n#include \"gdata-documents-enums.h\"" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(gdata_documents_headers)) > gdata-documents-enums.c.tmp \
	&& sed "s/g_data/gdata/" gdata-documents-enums.c.tmp > gdata-documents-enums.c \
	&& rm -f gdata-documents-enums.c.tmp

# Documents library
gdatadocumentsincludedir = $(pkgincludedir)/gdata/services/documents

gdata_documents_headers = \
	gdata-documents-service.h	\
	gdata-documents-feed.h		\
	gdata-documents-entry.h		\
	gdata-documents-query.h		\
	gdata-documents-text.h		\
	gdata-documents-presentation.h	\
	gdata-documents-folder.h	\
	gdata-documents-spreadsheet.h

gdatadocumentsinclude_HEADERS = \
	$(gdata_documents_headers)	\
	gdata-documents-enums.h

noinst_LTLIBRARIES = libgdatadocuments.la

libgdatadocuments_la_SOURCES = \
	$(GDATA_DOCUMENTS_ENUM_FILES)	\
	gdata-documents-service.c	\
	gdata-documents-feed.c	\
	gdata-documents-entry.c	\
	gdata-documents-text.c	\
	gdata-documents-presentation.c	\
	gdata-documents-spreadsheet.c	\
	gdata-documents-folder.c	\
	gdata-documents-query.c

libgdatadocuments_la_CPPFLAGS = \
	-I$(top_srcdir)				\
	-I$(top_srcdir)/gdata			\
	-I$(top_srcdir)/gdata/services/documents	\
	$(DISABLE_DEPRECATED)			\
	$(AM_CPPFLAGS)

libgdatadocuments_la_CFLAGS = \
	$(GDATA_CFLAGS)	\
	$(WARN_CFLAGS)	\
	$(AM_CFLAGS)	\
	-D_GNU_SOURCE

libgdatadocuments_la_LIBADD = \
	$(GDATA_LIBS)

libgdatadocuments_la_LDFLAGS = \
	-no-undefined	\
	$(AM_LDFLAGS)

# General cleanup
CLEANFILES = \
	$(GDATA_DOCUMENTS_ENUM_FILES)

-include $(top_srcdir)/git.mk
