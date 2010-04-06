SUBDIRS = . gdata/tests po docs

DISTCHECK_CONFIGURE_FLAGS =
MAINTAINERCLEANFILES =

# Marshalling
GDATA_MARSHAL_FILES = \
	gdata/gdata-marshal.c	\
	gdata/gdata-marshal.h

gdata/gdata-marshal.h: $(top_srcdir)/gdata/gdata-marshal.list Makefile
	$(AM_V_GEN)($(GLIB_GENMARSHAL) --prefix=gdata_marshal $(srcdir)/gdata/gdata-marshal.list --header > $@)
gdata/gdata-marshal.c: $(top_srcdir)/gdata/gdata-marshal.h Makefile
	$(AM_V_GEN)($(GLIB_GENMARSHAL) --prefix=gdata_marshal $(srcdir)/gdata/gdata-marshal.list --header --body > $@)

EXTRA_DIST = gdata/gdata-marshal.list
CLEANFILES = $(GDATA_MARSHAL_FILES)

# Enums
GDATA_ENUM_FILES = \
	gdata/gdata-enums.c					\
	gdata/gdata-enums.h					\
	gdata/media/gdata-media-enums.c				\
	gdata/media/gdata-media-enums.h				\
	gdata/services/documents/gdata-documents-enums.c	\
	gdata/services/documents/gdata-documents-enums.h	\
	gdata/services/picasaweb/gdata-picasaweb-enums.c	\
	gdata/services/picasaweb/gdata-picasaweb-enums.h	\
	gdata/services/youtube/gdata-youtube-enums.c		\
	gdata/services/youtube/gdata-youtube-enums.h

gdata/gdata-enums.h: $(gdata_headers) Makefile
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#ifndef GDATA_ENUMS_H\n#define GDATA_ENUMS_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define GDATA_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* !GDATA_ENUMS_H */" $(addprefix $(srcdir)/,$(gdata_headers)) > $@.tmp \
	&& sed "s/g_data/gdata/" $@.tmp > $@ \
	&& rm -f $@.tmp)

gdata/gdata-enums.c: $(gdata_headers) Makefile gdata/gdata-enums.h
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#include \"gdata-service.h\"\n#include \"gdata-parsable.h\"\n#include \"gdata-enums.h\"" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(addprefix $(srcdir)/,$(gdata_headers)) > $@.tmp \
	&& sed "s/g_data/gdata/" $@.tmp > $@ \
	&& rm -f $@.tmp)

gdata/media/gdata-media-enums.h: $(gdata_media_headers) Makefile
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#ifndef GDATA_MEDIA_ENUMS_H\n#define GDATA_MEDIA_ENUMS_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define GDATA_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* !GDATA_MEDIA_ENUMS_H */" \
		$(addprefix $(srcdir)/,$(gdata_media_headers)) > $@.tmp \
	&& sed "s/g_data/gdata/" $@.tmp > $@.tmp2 \
	&& sed "s/GDATA_TYPE_DATA/GDATA_TYPE/" $@.tmp2 > $@ \
	&& rm -f $@.tmp \
	&& rm -f $@.tmp2)

gdata/media/gdata-media-enums.c: $(gdata_media_headers) Makefile gdata/media/gdata-media-enums.h
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#include \"gdata-media-content.h\"\n#include \"gdata-media-enums.h\"" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(addprefix $(srcdir)/,$(gdata_media_headers)) > $@.tmp \
	&& sed "s/g_data/gdata/" $@.tmp > $@ \
	&& rm -f $@.tmp)

gdata/services/documents/gdata-documents-enums.h: $(gdata_documents_headers) Makefile
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#ifndef GDATA_DOCUMENTS_ENUMS_H\n#define GDATA_DOCUMENTS_ENUMS_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define GDATA_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* !GDATA_DOCUMENTS_ENUMS_H */" \
		$(addprefix $(srcdir)/,$(gdata_documents_headers)) > $@.tmp \
	&& sed "s/g_data/gdata/" $@.tmp > $@ \
	&& rm -f $@.tmp)

gdata/services/documents/gdata-documents-enums.c: $(gdata_documents_headers) Makefile gdata/services/documents/gdata-documents-enums.h
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#include \"gdata-documents-service.h\"\n#include \"gdata-documents-text.h\"\n#include \"gdata-documents-spreadsheet.h\"\n#include \"gdata-documents-presentation.h\"\n#include \"gdata-documents-enums.h\"" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(addprefix $(srcdir)/,$(gdata_documents_headers)) > $@.tmp \
	&& sed "s/g_data/gdata/" $@.tmp > $@ \
	&& rm -f $@.tmp)

gdata/services/picasaweb/gdata-picasaweb-enums.h: $(gdata_picasaweb_headers) Makefile
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#ifndef GDATA_PICASAWEB_ENUMS_H\n#define GDATA_PICASAWEB_ENUMS_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define GDATA_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* !GDATA_PICASAWEB_ENUMS_H */" \
		$(addprefix $(srcdir)/,$(gdata_picasaweb_headers)) > $@.tmp \
	&& sed "s/g_data_picasa_web/gdata_picasaweb/" $@.tmp > $@.tmp2 \
	&& sed "s/GDATA_TYPE_DATA_PICASA_WEB/GDATA_TYPE_PICASAWEB/" $@.tmp2 > $@ \
	&& rm -f $@.tmp \
	&& rm -f $@.tmp2)

gdata/services/picasaweb/gdata-picasaweb-enums.c: $(gdata_picasaweb_headers) Makefile gdata/services/picasaweb/gdata-picasaweb-enums.h
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#include \"gdata-picasaweb-album.h\"\n#include \"gdata-picasaweb-enums.h\"" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(addprefix $(srcdir)/,$(gdata_picasaweb_headers)) > $@.tmp \
	&& sed "s/g_data_picasa_web/gdata_picasaweb/" $@.tmp > $@ \
	&& rm -f $@.tmp)

gdata/services/youtube/gdata-youtube-enums.h: $(gdata_youtube_headers) Makefile
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#ifndef GDATA_YOUTUBE_ENUMS_H\n#define GDATA_YOUTUBE_ENUMS_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define GDATA_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* !GDATA_YOUTUBE_ENUMS_H */" \
		$(addprefix $(srcdir)/,$(gdata_youtube_headers)) > $@.tmp \
	&& sed "s/g_data_you_tube/gdata_youtube/" $@.tmp > $@.tmp2 \
	&& sed "s/GDATA_TYPE_DATA_YOU_TUBE/GDATA_TYPE_YOUTUBE/" $@.tmp2 > $@ \
	&& rm -f $@.tmp \
	&& rm -f $@.tmp2)

gdata/services/youtube/gdata-youtube-enums.c: $(gdata_youtube_headers) Makefile gdata/services/youtube/gdata-youtube-enums.h
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#include \"gdata-youtube-service.h\"\n#include \"gdata-youtube-query.h\"\n#include \"gdata-youtube-enums.h\"" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(addprefix $(srcdir)/,$(gdata_youtube_headers)) > $@.tmp \
	&& sed "s/g_data_you_tube/gdata_youtube/" $@.tmp > $@ \
	&& rm -f $@.tmp)

CLEANFILES += $(GDATA_ENUM_FILES)

# libgdata library
lib_LTLIBRARIES = gdata/libgdata.la

gdataincludedir = $(pkgincludedir)/gdata
gdata_headers = \
	gdata/gdata.h			\
	gdata/gdata-entry.h		\
	gdata/gdata-feed.h		\
	gdata/gdata-service.h		\
	gdata/gdata-query.h		\
	gdata/gdata-access-handler.h	\
	gdata/gdata-access-rule.h	\
	gdata/gdata-parsable.h		\
	gdata/gdata-download-stream.h	\
	gdata/gdata-upload-stream.h
# The following headers are private, and shouldn't be installed:
private_headers = \
	gdata/gdata-private.h		\
	gdata/gdata-parser.h		\
	gdata/gdata-buffer.h		\
	gdata/exif/gdata-exif-tags.h	\
	gdata/georss/gdata-georss-where.h
gdatainclude_HEADERS = \
	$(gdata_headers)	\
	gdata/gdata-enums.h	\
	gdata/gdata-types.h

gdataatomincludedir = $(gdataincludedir)/atom
gdataatominclude_HEADERS = \
	gdata/atom/gdata-author.h	\
	gdata/atom/gdata-category.h	\
	gdata/atom/gdata-generator.h	\
	gdata/atom/gdata-link.h

gdatagdincludedir = $(gdataincludedir)/gd
gdatagdinclude_HEADERS = \
	gdata/gd/gdata-gd-email-address.h	\
	gdata/gd/gdata-gd-im-address.h		\
	gdata/gd/gdata-gd-organization.h	\
	gdata/gd/gdata-gd-phone-number.h	\
	gdata/gd/gdata-gd-postal-address.h	\
	gdata/gd/gdata-gd-reminder.h		\
	gdata/gd/gdata-gd-when.h		\
	gdata/gd/gdata-gd-where.h		\
	gdata/gd/gdata-gd-who.h			\
	gdata/gd/gdata-gd-name.h

gdatagcontactincludedir = $(gdataincludedir)/gcontact
gdatagcontactinclude_HEADERS = \
	gdata/gcontact/gdata-gcontact-calendar.h	\
	gdata/gcontact/gdata-gcontact-event.h		\
	gdata/gcontact/gdata-gcontact-external-id.h	\
	gdata/gcontact/gdata-gcontact-jot.h		\
	gdata/gcontact/gdata-gcontact-relation.h	\
	gdata/gcontact/gdata-gcontact-website.h

gdatamediaincludedir = $(gdataincludedir)/media
gdata_media_headers = \
	gdata/media/gdata-media-category.h	\
	gdata/media/gdata-media-content.h	\
	gdata/media/gdata-media-credit.h	\
	gdata/media/gdata-media-thumbnail.h
private_headers += gdata/media/gdata-media-group.h
gdatamediainclude_HEADERS = \
	$(gdata_media_headers)		\
	gdata/media/gdata-media-enums.h

gdatacalendarincludedir = $(gdataincludedir)/services/calendar
gdatacalendarinclude_HEADERS = \
	gdata/services/calendar/gdata-calendar-service.h	\
	gdata/services/calendar/gdata-calendar-calendar.h	\
	gdata/services/calendar/gdata-calendar-event.h		\
	gdata/services/calendar/gdata-calendar-query.h		\
	gdata/services/calendar/gdata-calendar-feed.h

gdatacontactsincludedir = $(gdataincludedir)/services/contacts
gdatacontactsinclude_HEADERS = \
	gdata/services/contacts/gdata-contacts-service.h	\
	gdata/services/contacts/gdata-contacts-contact.h	\
	gdata/services/contacts/gdata-contacts-query.h

gdatadocumentsincludedir = $(gdataincludedir)/services/documents
gdata_documents_headers = \
	gdata/services/documents/gdata-documents-service.h	\
	gdata/services/documents/gdata-documents-feed.h		\
	gdata/services/documents/gdata-documents-entry.h	\
	gdata/services/documents/gdata-documents-query.h	\
	gdata/services/documents/gdata-documents-text.h		\
	gdata/services/documents/gdata-documents-presentation.h	\
	gdata/services/documents/gdata-documents-folder.h	\
	gdata/services/documents/gdata-documents-spreadsheet.h
gdatadocumentsinclude_HEADERS = \
	$(gdata_documents_headers)				\
	gdata/services/documents/gdata-documents-enums.h

gdatapicasawebincludedir = $(gdataincludedir)/services/picasaweb
gdata_picasaweb_headers = \
	gdata/services/picasaweb/gdata-picasaweb-service.h	\
	gdata/services/picasaweb/gdata-picasaweb-query.h	\
	gdata/services/picasaweb/gdata-picasaweb-file.h		\
	gdata/services/picasaweb/gdata-picasaweb-album.h	\
	gdata/services/picasaweb/gdata-picasaweb-user.h		\
	gdata/services/picasaweb/gdata-picasaweb-feed.h
gdatapicasawebinclude_HEADERS = \
	$(gdata_picasaweb_headers)				\
	gdata/services/picasaweb/gdata-picasaweb-enums.h

gdatayoutubeincludedir = $(gdataincludedir)/services/youtube
gdata_youtube_headers = \
	gdata/services/youtube/gdata-youtube-service.h	\
	gdata/services/youtube/gdata-youtube-video.h	\
	gdata/services/youtube/gdata-youtube-content.h	\
	gdata/services/youtube/gdata-youtube-credit.h	\
	gdata/services/youtube/gdata-youtube-query.h	\
	gdata/services/youtube/gdata-youtube-state.h
private_headers += \
	gdata/services/youtube/gdata-youtube-group.h	\
	gdata/services/youtube/gdata-youtube-control.h
gdatayoutubeinclude_HEADERS = \
	$(gdata_youtube_headers)			\
	gdata/services/youtube/gdata-youtube-enums.h

gdata_libgdata_la_SOURCES = \
	$(GDATA_MARSHAL_FILES)		\
	$(GDATA_ENUM_FILES)		\
	$(private_headers)		\
	gdata/gdata-entry.c		\
	gdata/gdata-feed.c		\
	gdata/gdata-service.c		\
	gdata/gdata-types.c		\
	gdata/gdata-query.c		\
	gdata/gdata-parser.c		\
	gdata/gdata-access-handler.c	\
	gdata/gdata-access-rule.c	\
	gdata/gdata-parsable.c		\
	gdata/gdata-download-stream.c	\
	gdata/gdata-upload-stream.c	\
	gdata/gdata-buffer.c		\
	\
	gdata/atom/gdata-author.c	\
	gdata/atom/gdata-category.c	\
	gdata/atom/gdata-generator.c	\
	gdata/atom/gdata-link.c		\
	\
	gdata/exif/gdata-exif-tags.c	\
	\
	gdata/gd/gdata-gd-email-address.c	\
	gdata/gd/gdata-gd-im-address.c		\
	gdata/gd/gdata-gd-organization.c	\
	gdata/gd/gdata-gd-phone-number.c	\
	gdata/gd/gdata-gd-postal-address.c	\
	gdata/gd/gdata-gd-reminder.c		\
	gdata/gd/gdata-gd-when.c		\
	gdata/gd/gdata-gd-where.c		\
	gdata/gd/gdata-gd-who.c			\
	gdata/gd/gdata-gd-name.c		\
	\
	gdata/gcontact/gdata-gcontact-calendar.c	\
	gdata/gcontact/gdata-gcontact-event.c		\
	gdata/gcontact/gdata-gcontact-external-id.c	\
	gdata/gcontact/gdata-gcontact-jot.c		\
	gdata/gcontact/gdata-gcontact-relation.c	\
	gdata/gcontact/gdata-gcontact-website.c		\
	\
	gdata/georss/gdata-georss-where.c	\
	\
	gdata/media/gdata-media-category.c	\
	gdata/media/gdata-media-content.c	\
	gdata/media/gdata-media-credit.c	\
	gdata/media/gdata-media-thumbnail.c	\
	gdata/media/gdata-media-group.c		\
	\
	gdata/services/calendar/gdata-calendar-service.c	\
	gdata/services/calendar/gdata-calendar-calendar.c	\
	gdata/services/calendar/gdata-calendar-event.c		\
	gdata/services/calendar/gdata-calendar-query.c		\
	gdata/services/calendar/gdata-calendar-feed.c		\
	\
	gdata/services/contacts/gdata-contacts-service.c	\
	gdata/services/contacts/gdata-contacts-contact.c	\
	gdata/services/contacts/gdata-contacts-query.c		\
	\
	gdata/services/documents/gdata-documents-service.c	\
	gdata/services/documents/gdata-documents-feed.c		\
	gdata/services/documents/gdata-documents-entry.c	\
	gdata/services/documents/gdata-documents-text.c		\
	gdata/services/documents/gdata-documents-presentation.c	\
	gdata/services/documents/gdata-documents-spreadsheet.c	\
	gdata/services/documents/gdata-documents-folder.c	\
	gdata/services/documents/gdata-documents-query.c	\
	\
	gdata/services/picasaweb/gdata-picasaweb-album.c	\
	gdata/services/picasaweb/gdata-picasaweb-query.c	\
	gdata/services/picasaweb/gdata-picasaweb-file.c		\
	gdata/services/picasaweb/gdata-picasaweb-service.c	\
	gdata/services/picasaweb/gdata-picasaweb-user.c		\
	gdata/services/picasaweb/gdata-picasaweb-feed.c		\
	\
	gdata/services/youtube/gdata-youtube-service.c		\
	gdata/services/youtube/gdata-youtube-video.c		\
	gdata/services/youtube/gdata-youtube-content.c		\
	gdata/services/youtube/gdata-youtube-credit.c		\
	gdata/services/youtube/gdata-youtube-query.c		\
	gdata/services/youtube/gdata-youtube-group.c		\
	gdata/services/youtube/gdata-youtube-state.c		\
	gdata/services/youtube/gdata-youtube-control.c

# All but the first two -I flags here are to cope with compiling the enum files in a srcdir!=builddir environment
gdata_libgdata_la_CPPFLAGS = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/gdata		\
	-I$(top_srcdir)/gdata/media	\
	-I$(top_srcdir)/gdata/services/documents	\
	-I$(top_srcdir)/gdata/services/picasaweb	\
	-I$(top_srcdir)/gdata/services/youtube		\
	-DG_LOG_DOMAIN=\"libgdata\"	\
	$(DISABLE_DEPRECATED)		\
	$(AM_CPPFLAGS)

gdata_libgdata_la_CFLAGS = \
	$(GDATA_CFLAGS)	\
	$(GNOME_CFLAGS)	\
	$(GCOV_CFLAGS)	\
	$(WARN_CFLAGS)	\
	$(AM_CFLAGS)

gdata_libgdata_la_LIBADD = \
	$(GDATA_LIBS)	\
	$(GNOME_LIBS)	\
	$(GCOV_LIBS)	\
	$(AM_LIBADD)

gdata_libgdata_la_LDFLAGS = \
	-version-info $(GDATA_LT_VERSION)	\
	-export-symbols $(srcdir)/gdata/gdata.symbols	\
	-no-undefined				\
	$(AM_LDFLAGS)

EXTRA_DIST += gdata/gdata.symbols

# Introspection
if HAVE_INTROSPECTION
_gir_libtool = $(if $(LIBTOOL),--libtool="$(LIBTOOL)") # bgo#610273
gdata/GData-$(GDATA_API_VERSION).gir: Makefile gdata/libgdata.la
	$(AM_V_GEN)($(INTROSPECTION_SCANNER)						\
	--namespace GData --nsversion=$(GDATA_API_VERSION)				\
	--c-include="gdata/gdata.h" --include=libxml2-2.0 --include=Soup-2.4		\
	--library=gdata/libgdata.la $(_gir_libtool) --output $@				\
	-I$(top_srcdir) -I$(top_srcdir)/gdata						\
	$(addprefix $(srcdir)/,$(gdatainclude_HEADERS) $(gdataatominclude_HEADERS)	\
	$(gdatagdinclude_HEADERS) $(gdatagcontactinclude_HEADERS)			\
	$(gdatamediainclude_HEADERS) $(gdatacalendarinclude_HEADERS)			\
	$(gdatacontactsinclude_HEADERS) $(gdatadocumentsinclude_HEADERS)		\
	$(gdatapicasawebinclude_HEADERS) $(gdatayoutubeinclude_HEADERS)))
BUILT_GIRSOURCES = gdata/GData-$(GDATA_API_VERSION).gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(BUILT_GIRSOURCES)

typelibsdir = $(libdir)/girepository-1.0/
typelibs_DATA = $(BUILT_GIRSOURCES:.gir=.typelib)

%.typelib: %.gir
	$(AM_V_GEN)($(INTROSPECTION_COMPILER) --includedir=$(top_srcdir)/gdata --includedir=. $(INTROSPECTION_COMPILER_ARGS) $< -o $@)

CLEANFILES += \
	$(gir_DATA)	\
	$(typelibs_DATA)

DISTCHECK_CONFIGURE_FLAGS += --enable-introspection
endif

# gcov
if GCOV_ENABLED
gcov-report.txt: gcov-clean all check
	$(AM_V_GEN)(rm -f $@; \
	echo -e "Test coverage for libgdata:\n" >> $@; \
	total_covered=0; total_actual=0; \
	for file in $(filter %.c,$(gdata_libgdata_la_SOURCES)); do \
	  file2=$${file##*/}; \
	  cd $(abs_srcdir)/$${file%/*}; \
	  gcov -o `find -newer $$file2 -name "*-$${file2/.c/.gcda}" -print0 | sed -e 's/\.gcda/\.o/'` $$file2 > /dev/null; \
	  if test -f $$file2.gcov; then \
	    actual=`grep -v '        -:' $$file2.gcov  | wc -l`; \
	    uncovered=`grep '#####:' $$file2.gcov | wc -l`; \
	    covered=$$((actual - uncovered)); \
	    total_covered=$$((total_covered + covered)); \
	    total_actual=$$((total_actual + actual)); \
	    echo -e "$$file:\t$$covered / $$actual\t($$((($$covered * 100) / $$actual))%)"; \
	  fi \
	done >> $@; \
	cd $(abs_srcdir); \
	echo -e "\nSource lines: $$total_actual\nCovered statements: $$total_covered\nTotal coverage: $$((($$total_covered * 100) / $$total_actual))%" >> $@)

gcov: gcov-report.txt
	@cat gcov-report.txt

clean: gcov-clean
gcov-clean:
	@find . -name "*.gcda" -o -name "*.gcov" -delete

MAINTAINERCLEANFILES += gcov-report.txt
else
gcov:
	@echo "Need to reconfigure with --enable-gcov"
endif

# Cleaning
EXTRA_DIST += \
	autogen.sh		\
	HACKING			\
	gtk-doc.make		\
	libgdata.doap

CLEANFILES += \
	`find "$(srcdir)" -type f -name .dirstamp -print`

DISTCLEANFILES = \
	gnome-doc-utils.make	\
	gtk-doc.make

MAINTAINERCLEANFILES += \
	INSTALL		\
	aclocal.m4	\
	compile		\
	config.guess	\
	config.h.in	\
	config.sub	\
	depcomp		\
	install-sh	\
	ltmain.sh	\
	missing		\
	mkinstalldirs	\
	omf.make	\
	gtk-doc.make	\
	`find "$(srcdir)" -type f -name Makefile.in -print`

DISTCHECK_CONFIGURE_FLAGS += --enable-gtk-doc

# pkgconfig data
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgdata.pc

CLEANFILES += $(pkgconfig_DATA)
EXTRA_DIST += libgdata.pc.in

# ChangeLog
ChangeLog: $(srcdir)/ChangeLog
$(srcdir)/ChangeLog:
	@echo Creating $@
	@if test -d "$(srcdir)/.git"; then \
	  (GIT_DIR=$(top_srcdir)/.git ./missing --run \
	   git log --stat) | fmt --split-only > $@.tmp \
	  && mv -f $@.tmp $@ \
	  || ($(RM) $@.tmp; \
	      echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
	      (test -f $@ || echo git-log is required to generate this file >> $@)); \
	else \
	  test -f $@ || \
	  (echo A git checkout and git-log is required to generate ChangeLog >&2 && \
	  echo A git checkout and git-log is required to generate this file >> $@); \
	fi
.PHONY: $(srcdir)/ChangeLog gcov gcov-clean gcov-report.txt

MAINTAINERCLEANFILES += ChangeLog

-include $(top_srcdir)/git.mk
