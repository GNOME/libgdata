SUBDIRS = . gdata/tests po docs

ACLOCAL_AMFLAGS = -I m4

DISTCHECK_CONFIGURE_FLAGS =
MAINTAINERCLEANFILES =

# Marshalling
GDATA_MARSHAL_FILES = \
	gdata/gdata-marshal.c	\
	gdata/gdata-marshal.h

gdata/gdata-marshal.h: $(top_srcdir)/gdata/gdata-marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=gdata_marshal --header $< > $@
gdata/gdata-marshal.c: $(top_srcdir)/gdata/gdata-marshal.list gdata/gdata-marshal.h
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=gdata_marshal --header --body $< > $@

EXTRA_DIST = $(top_srcdir)/gdata/gdata-marshal.list
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
			--ftail "G_END_DECLS\n\n#endif /* !GDATA_ENUMS_H */" $(addprefix $(srcdir)/,$(gdata_headers)) > gdata/gdata-enums.h.tmp \
	&& sed "s/g_data/gdata/" gdata/gdata-enums.h.tmp > gdata/gdata-enums.h \
	&& rm -f gdata/gdata-enums.h.tmp)

gdata/gdata-enums.c: $(gdata_headers) Makefile gdata/gdata-enums.h
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#include \"gdata-service.h\"\n#include \"gdata-parsable.h\"\n#include \"gdata-batch-operation.h\"\n#include \"gdata-enums.h\"\n#include \"gdata-client-login-authorizer.h\"" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(addprefix $(srcdir)/,$(gdata_headers)) > gdata/gdata-enums.c.tmp \
	&& sed "s/g_data/gdata/" gdata/gdata-enums.c.tmp > gdata/gdata-enums.c \
	&& rm -f gdata/gdata-enums.c.tmp)

gdata/media/gdata-media-enums.h: $(gdata_media_headers) Makefile
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#ifndef GDATA_MEDIA_ENUMS_H\n#define GDATA_MEDIA_ENUMS_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define GDATA_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* !GDATA_MEDIA_ENUMS_H */" \
		$(addprefix $(srcdir)/,$(gdata_media_headers)) > gdata/media/gdata-media-enums.h.tmp \
	&& sed "s/g_data/gdata/" gdata/media/gdata-media-enums.h.tmp > gdata/media/gdata-media-enums.h.tmp2 \
	&& sed "s/GDATA_TYPE_DATA/GDATA_TYPE/" gdata/media/gdata-media-enums.h.tmp2 > gdata/media/gdata-media-enums.h \
	&& rm -f gdata/media/gdata-media-enums.h.tmp \
	&& rm -f gdata/media/gdata-media-enums.h.tmp2)

gdata/media/gdata-media-enums.c: $(gdata_media_headers) Makefile gdata/media/gdata-media-enums.h
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#include \"gdata-media-content.h\"\n#include \"gdata-media-enums.h\"" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(addprefix $(srcdir)/,$(gdata_media_headers)) > gdata/media/gdata-media-enums.c.tmp \
	&& sed "s/g_data/gdata/" gdata/media/gdata-media-enums.c.tmp > gdata/media/gdata-media-enums.c \
	&& rm -f gdata/media/gdata-media-enums.c.tmp)

gdata/services/documents/gdata-documents-enums.h: $(gdata_documents_headers) Makefile
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#ifndef GDATA_DOCUMENTS_ENUMS_H\n#define GDATA_DOCUMENTS_ENUMS_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define GDATA_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* !GDATA_DOCUMENTS_ENUMS_H */" \
		$(addprefix $(srcdir)/,$(gdata_documents_headers)) > gdata/services/documents/gdata-documents-enums.h.tmp \
	&& sed "s/g_data/gdata/" gdata/services/documents/gdata-documents-enums.h.tmp > gdata/services/documents/gdata-documents-enums.h \
	&& rm -f gdata/services/documents/gdata-documents-enums.h.tmp)

gdata/services/documents/gdata-documents-enums.c: $(gdata_documents_headers) Makefile gdata/services/documents/gdata-documents-enums.h
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#include \"gdata-documents-service.h\"\n#include \"gdata-documents-text.h\"\n#include \"gdata-documents-spreadsheet.h\"\n#include \"gdata-documents-drawing.h\"\n#include \"gdata-documents-pdf.h\"\n#include \"gdata-documents-presentation.h\"\n#include \"gdata-documents-enums.h\"" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(addprefix $(srcdir)/,$(gdata_documents_headers)) > gdata/services/documents/gdata-documents-enums.c.tmp \
	&& sed "s/g_data/gdata/" gdata/services/documents/gdata-documents-enums.c.tmp > gdata/services/documents/gdata-documents-enums.c \
	&& rm -f gdata/services/documents/gdata-documents-enums.c.tmp)

gdata/services/picasaweb/gdata-picasaweb-enums.h: $(gdata_picasaweb_headers) Makefile
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#ifndef GDATA_PICASAWEB_ENUMS_H\n#define GDATA_PICASAWEB_ENUMS_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define GDATA_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* !GDATA_PICASAWEB_ENUMS_H */" \
		$(addprefix $(srcdir)/,$(gdata_picasaweb_headers)) > gdata/services/picasaweb/gdata-picasaweb-enums.h.tmp \
	&& sed "s/g_data_picasa_web/gdata_picasaweb/" gdata/services/picasaweb/gdata-picasaweb-enums.h.tmp > gdata/services/picasaweb/gdata-picasaweb-enums.h.tmp2 \
	&& sed "s/GDATA_TYPE_DATA_PICASA_WEB/GDATA_TYPE_PICASAWEB/" gdata/services/picasaweb/gdata-picasaweb-enums.h.tmp2 > gdata/services/picasaweb/gdata-picasaweb-enums.h \
	&& rm -f gdata/services/picasaweb/gdata-picasaweb-enums.h.tmp \
	&& rm -f gdata/services/picasaweb/gdata-picasaweb-enums.h.tmp2)

gdata/services/picasaweb/gdata-picasaweb-enums.c: $(gdata_picasaweb_headers) Makefile gdata/services/picasaweb/gdata-picasaweb-enums.h
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#include \"gdata-picasaweb-album.h\"\n#include \"gdata-picasaweb-enums.h\"" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(addprefix $(srcdir)/,$(gdata_picasaweb_headers)) > gdata/services/picasaweb/gdata-picasaweb-enums.c.tmp \
	&& sed "s/g_data_picasa_web/gdata_picasaweb/" gdata/services/picasaweb/gdata-picasaweb-enums.c.tmp > gdata/services/picasaweb/gdata-picasaweb-enums.c \
	&& rm -f gdata/services/picasaweb/gdata-picasaweb-enums.c.tmp)

gdata/services/youtube/gdata-youtube-enums.h: $(gdata_youtube_headers) Makefile
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#ifndef GDATA_YOUTUBE_ENUMS_H\n#define GDATA_YOUTUBE_ENUMS_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define GDATA_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* !GDATA_YOUTUBE_ENUMS_H */" \
		$(addprefix $(srcdir)/,$(gdata_youtube_headers)) > gdata/services/youtube/gdata-youtube-enums.h.tmp \
	&& sed "s/g_data_you_tube/gdata_youtube/" gdata/services/youtube/gdata-youtube-enums.h.tmp > gdata/services/youtube/gdata-youtube-enums.h.tmp2 \
	&& sed "s/GDATA_TYPE_DATA_YOU_TUBE/GDATA_TYPE_YOUTUBE/" gdata/services/youtube/gdata-youtube-enums.h.tmp2 > gdata/services/youtube/gdata-youtube-enums.h \
	&& rm -f gdata/services/youtube/gdata-youtube-enums.h.tmp \
	&& rm -f gdata/services/youtube/gdata-youtube-enums.h.tmp2)

gdata/services/youtube/gdata-youtube-enums.c: $(gdata_youtube_headers) Makefile gdata/services/youtube/gdata-youtube-enums.h
	$(AM_V_GEN)($(GLIB_MKENUMS) \
			--fhead "#include \"gdata-youtube-service.h\"\n#include \"gdata-youtube-query.h\"\n#include \"gdata-youtube-enums.h\"" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(addprefix $(srcdir)/,$(gdata_youtube_headers)) > gdata/services/youtube/gdata-youtube-enums.c.tmp \
	&& sed "s/g_data_you_tube/gdata_youtube/" gdata/services/youtube/gdata-youtube-enums.c.tmp > gdata/services/youtube/gdata-youtube-enums.c \
	&& rm -f gdata/services/youtube/gdata-youtube-enums.c.tmp)

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
	gdata/gdata-commentable.h	\
	gdata/gdata-comment.h		\
	gdata/gdata-access-handler.h	\
	gdata/gdata-access-rule.h	\
	gdata/gdata-parsable.h		\
	gdata/gdata-download-stream.h	\
	gdata/gdata-upload-stream.h	\
	gdata/gdata-comparable.h	\
	gdata/gdata-batch-operation.h	\
	gdata/gdata-batchable.h		\
	gdata/gdata-authorizer.h	\
	gdata/gdata-authorization-domain.h	\
	gdata/gdata-client-login-authorizer.h	\
	gdata/gdata-oauth1-authorizer.h

if ENABLE_GOA
gdata_headers += \
	gdata/gdata-goa-authorizer.h \
	$(NULL)
endif

# The following headers are private, and shouldn't be installed:
private_headers = \
	gdata/gdata-private.h		\
	gdata/gdata-batch-private.h	\
	gdata/gdata-batch-feed.h	\
	gdata/gdata-parser.h		\
	gdata/gdata-buffer.h		\
	gdata/gd/gdata-gd-feed-link.h	\
	gdata/exif/gdata-exif-tags.h	\
	gdata/georss/gdata-georss-where.h
gdatainclude_HEADERS = \
	$(gdata_headers)	\
	gdata/gdata-enums.h	\
	gdata/gdata-types.h \
	gdata/gdata-version.h

gdataatomincludedir = $(gdataincludedir)/atom
gdataatominclude_HEADERS = \
	gdata/atom/gdata-author.h	\
	gdata/atom/gdata-category.h	\
	gdata/atom/gdata-generator.h	\
	gdata/atom/gdata-link.h

gdataappincludedir = $(gdataincludedir)/app
gdataappinclude_HEADERS = \
	gdata/app/gdata-app-categories.h

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
	gdata/gcontact/gdata-gcontact-language.h	\
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
	gdata/services/contacts/gdata-contacts-group.h		\
	gdata/services/contacts/gdata-contacts-query.h

gdatadocumentsincludedir = $(gdataincludedir)/services/documents
gdata_documents_headers = \
	gdata/services/documents/gdata-documents-service.h	\
	gdata/services/documents/gdata-documents-feed.h		\
	gdata/services/documents/gdata-documents-entry.h	\
	gdata/services/documents/gdata-documents-query.h	\
	gdata/services/documents/gdata-documents-document.h	\
	gdata/services/documents/gdata-documents-text.h		\
	gdata/services/documents/gdata-documents-presentation.h	\
	gdata/services/documents/gdata-documents-folder.h	\
	gdata/services/documents/gdata-documents-drawing.h	\
	gdata/services/documents/gdata-documents-pdf.h		\
	gdata/services/documents/gdata-documents-spreadsheet.h	\
	gdata/services/documents/gdata-documents-upload-query.h
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
	gdata/services/picasaweb/gdata-picasaweb-feed.h		\
	gdata/services/picasaweb/gdata-picasaweb-comment.h
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
	gdata/services/youtube/gdata-youtube-state.h	\
	gdata/services/youtube/gdata-youtube-category.h	\
	gdata/services/youtube/gdata-youtube-comment.h
private_headers += \
	gdata/services/youtube/gdata-youtube-group.h	\
	gdata/services/youtube/gdata-youtube-control.h
gdatayoutubeinclude_HEADERS = \
	$(gdata_youtube_headers)			\
	gdata/services/youtube/gdata-youtube-enums.h

gdata_sources = \
	gdata/gdata-entry.c		\
	gdata/gdata-feed.c		\
	gdata/gdata-service.c		\
	gdata/gdata-types.c		\
	gdata/gdata-query.c		\
	gdata/gdata-parser.c		\
	gdata/gdata-commentable.c	\
	gdata/gdata-comment.c		\
	gdata/gdata-access-handler.c	\
	gdata/gdata-access-rule.c	\
	gdata/gdata-parsable.c		\
	gdata/gdata-download-stream.c	\
	gdata/gdata-upload-stream.c	\
	gdata/gdata-buffer.c		\
	gdata/gdata-comparable.c	\
	gdata/gdata-batch-operation.c	\
	gdata/gdata-batchable.c		\
	gdata/gdata-batch-feed.c	\
	gdata/gdata-authorizer.c	\
	gdata/gdata-authorization-domain.c	\
	gdata/gdata-client-login-authorizer.c	\
	gdata/gdata-oauth1-authorizer.c		\
	\
	gdata/atom/gdata-author.c	\
	gdata/atom/gdata-category.c	\
	gdata/atom/gdata-generator.c	\
	gdata/atom/gdata-link.c		\
	\
	gdata/app/gdata-app-categories.c	\
	\
	gdata/exif/gdata-exif-tags.c	\
	\
	gdata/gd/gdata-gd-email-address.c	\
	gdata/gd/gdata-gd-feed-link.c		\
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
	gdata/gcontact/gdata-gcontact-language.c	\
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
	gdata/services/contacts/gdata-contacts-group.c		\
	gdata/services/contacts/gdata-contacts-query.c		\
	\
	gdata/services/documents/gdata-documents-service.c	\
	gdata/services/documents/gdata-documents-feed.c		\
	gdata/services/documents/gdata-documents-entry.c	\
	gdata/services/documents/gdata-documents-document.c	\
	gdata/services/documents/gdata-documents-text.c		\
	gdata/services/documents/gdata-documents-presentation.c	\
	gdata/services/documents/gdata-documents-spreadsheet.c	\
	gdata/services/documents/gdata-documents-folder.c	\
	gdata/services/documents/gdata-documents-drawing.c	\
	gdata/services/documents/gdata-documents-pdf.c	\
	gdata/services/documents/gdata-documents-query.c	\
	gdata/services/documents/gdata-documents-upload-query.c	\
	\
	gdata/services/picasaweb/gdata-picasaweb-album.c	\
	gdata/services/picasaweb/gdata-picasaweb-query.c	\
	gdata/services/picasaweb/gdata-picasaweb-file.c		\
	gdata/services/picasaweb/gdata-picasaweb-service.c	\
	gdata/services/picasaweb/gdata-picasaweb-user.c		\
	gdata/services/picasaweb/gdata-picasaweb-feed.c		\
	gdata/services/picasaweb/gdata-picasaweb-comment.c	\
	\
	gdata/services/youtube/gdata-youtube-service.c		\
	gdata/services/youtube/gdata-youtube-video.c		\
	gdata/services/youtube/gdata-youtube-content.c		\
	gdata/services/youtube/gdata-youtube-credit.c		\
	gdata/services/youtube/gdata-youtube-query.c		\
	gdata/services/youtube/gdata-youtube-group.c		\
	gdata/services/youtube/gdata-youtube-state.c		\
	gdata/services/youtube/gdata-youtube-control.c		\
	gdata/services/youtube/gdata-youtube-category.c		\
	gdata/services/youtube/gdata-youtube-comment.c

if ENABLE_GOA
gdata_sources += \
	gdata/gdata-goa-authorizer.c \
	$(NULL)
endif

main_header = gdata/gdata.h
public_headers = \
	$(gdatainclude_HEADERS)			\
	$(gdataatominclude_HEADERS)		\
	$(gdataappinclude_HEADERS)		\
	$(gdatagdinclude_HEADERS)		\
	$(gdatagcontactinclude_HEADERS)		\
	$(gdatamediainclude_HEADERS)		\
	$(gdatacalendarinclude_HEADERS)		\
	$(gdatacontactsinclude_HEADERS)		\
	$(gdatadocumentsinclude_HEADERS)	\
	$(gdatapicasawebinclude_HEADERS)	\
	$(gdatayoutubeinclude_HEADERS)

gdata_libgdata_la_SOURCES = \
	$(GDATA_MARSHAL_FILES)		\
	$(GDATA_ENUM_FILES)		\
	$(private_headers)		\
	$(gdata_sources)

# All but the first two -I flags here are to cope with compiling the enum files in a srcdir!=builddir environment
gdata_libgdata_la_CPPFLAGS = \
	-I$(top_srcdir)					\
	-I$(top_srcdir)/gdata				\
	-I$(top_srcdir)/gdata/media			\
	-I$(top_srcdir)/gdata/services/documents	\
	-I$(top_srcdir)/gdata/services/picasaweb	\
	-I$(top_srcdir)/gdata/services/youtube		\
	-I$(top_builddir)/gdata				\
	-I$(top_builddir)/gdata/media			\
	-I$(top_builddir)/gdata/services/picasaweb	\
	-I$(top_builddir)/gdata/services/youtube	\
	-DG_LOG_DOMAIN=\"libgdata\"			\
	$(DISABLE_DEPRECATED)				\
	$(AM_CPPFLAGS)

gdata_libgdata_la_CFLAGS = \
	$(GDATA_CFLAGS)	\
	$(GNOME_CFLAGS)	\
	$(CODE_COVERAGE_CFLAGS)	\
	$(WARN_CFLAGS)	\
	$(AM_CFLAGS)

gdata_libgdata_la_LIBADD = \
	$(GDATA_LIBS)	\
	$(GNOME_LIBS)	\
	$(CODE_COVERAGE_LIBS)	\
	$(AM_LIBADD)

gdata_libgdata_la_LDFLAGS = \
	-version-info $(GDATA_LT_VERSION)	\
	-export-symbols $(srcdir)/gdata/gdata.symbols	\
	-no-undefined				\
	$(AM_LDFLAGS)

EXTRA_DIST += gdata/gdata.symbols

# Check if gdata.h includes all the public headers
check-local: check-headers
check-headers:
	@any_missing=0; for header_file in $(public_headers); do \
		if test "x$$header_file" != "x$(main_header)"; then \
			if ! grep "#include <$$header_file>" $(top_srcdir)/$(main_header) >/dev/null; then \
				echo "$(main_header) doesn't appear to include \"$$header_file\""; \
				any_missing=1; \
			fi; \
		fi; \
	done; exit "$$any_missing"

# Introspection
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all
INTROSPECTION_COMPILER_ARGS =
DISTCHECK_CONFIGURE_FLAGS += --enable-introspection
EXTRA_DIST += m4/introspection.m4

if HAVE_INTROSPECTION
gdata/GData-@GDATA_API_VERSION_MAJOR@.@GDATA_API_VERSION_MINOR@.gir: gdata/libgdata.la
gdata_GData_@GDATA_API_VERSION_MAJOR@_@GDATA_API_VERSION_MINOR@_gir_INCLUDES = GObject-2.0 libxml2-2.0 Soup-2.4
if ENABLE_GOA
gdata_GData_@GDATA_API_VERSION_MAJOR@_@GDATA_API_VERSION_MINOR@_gir_INCLUDES += Goa-1.0
endif
gdata_GData_@GDATA_API_VERSION_MAJOR@_@GDATA_API_VERSION_MINOR@_gir_CFLAGS = $(GDATA_CFLAGS) $(GNOME_CFLAGS) $(gdata_libgdata_la_CPPFLAGS)
gdata_GData_@GDATA_API_VERSION_MAJOR@_@GDATA_API_VERSION_MINOR@_gir_LIBS = gdata/libgdata.la
gdata_GData_@GDATA_API_VERSION_MAJOR@_@GDATA_API_VERSION_MINOR@_gir_FILES = $(gdata_sources) $(public_headers)
gdata_GData_@GDATA_API_VERSION_MAJOR@_@GDATA_API_VERSION_MINOR@_gir_NAMESPACE = GData
gdata_GData_@GDATA_API_VERSION_MAJOR@_@GDATA_API_VERSION_MINOR@_gir_EXPORT_PACKAGES = libgdata
gdata_GData_@GDATA_API_VERSION_MAJOR@_@GDATA_API_VERSION_MINOR@_gir_SCANNERFLAGS = \
	--nsversion=$(GDATA_API_VERSION) \
	--symbol-prefix=gdata \
	--c-include="gdata/gdata.h"

INTROSPECTION_GIRS += gdata/GData-$(GDATA_API_VERSION).gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)

if ENABLE_VAPIGEN
-include $(VAPIGEN_MAKEFILE)

# Note: The VAPI file name has to match the pkg-config file name.
# If we break API and add a version number to the pkg-config file name, the VAPI can be changed to match.
# See: https://bugzilla.gnome.org/show_bug.cgi?id=693865
gdata/libgdata.vapi: gdata/GData-@GDATA_API_VERSION_MAJOR@.@GDATA_API_VERSION_MINOR@.gir gdata/GData-@GDATA_API_VERSION_MAJOR@.@GDATA_API_VERSION_MINOR@.metadata

VAPIGEN_VAPIS = gdata/libgdata.vapi

gdata_libgdata_vapi_DEPS = libxml-2.0 libsoup-2.4
gdata_libgdata_vapi_METADATADIRS = $(srcdir)/gdata
gdata_libgdata_vapi_FILES = gdata/GData-@GDATA_API_VERSION_MAJOR@.@GDATA_API_VERSION_MINOR@.gir

gdata/libgdata.deps:
	$(AM_V_GEN) for pkg in $(gdata_libgdata_vapi_DEPS); do \
		echo $$pkg >> $@; \
	done

vapidir = $(datadir)/vala/vapi
vapi_DATA = $(VAPIGEN_VAPIS) $(VAPIGEN_VAPIS:.vapi=.deps)

CLEANFILES += $(vapi_DATA)

EXTRA_DIST += gdata/GData-@GDATA_API_VERSION_MAJOR@.@GDATA_API_VERSION_MINOR@.metadata
endif
endif

# Code coverage
@GNOME_CODE_COVERAGE_RULES@
CODE_COVERAGE_DIRECTORY = $(top_builddir)/gdata
CODE_COVERAGE_LCOV_OPTIONS = --base-directory $(abs_top_srcdir)

# Demo programs
noinst_PROGRAMS =

if HAVE_GTK
noinst_PROGRAMS += demos/scrapbook/scrapbook
endif

demos_scrapbook_scrapbook_SOURCES = \
	demos/scrapbook/scrapbook.c \
	demos/scrapbook/scrapbook.h \
	$(NULL)

demos_scrapbook_scrapbook_CPPFLAGS = \
	-I$(top_srcdir)/ \
	-I$(top_srcdir)/gdata \
	-DG_LOG_DOMAIN=\"scrapbook\" \
	-DLIBGDATA_DISABLE_DEPRECATED \
	$(DISABLE_DEPRECATED) \
	$(AM_CPPFLAGS) \
	$(NULL)

demos_scrapbook_scrapbook_CFLAGS = \
	$(WARN_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GDATA_CFLAGS) \
	$(GNOME_CFLAGS) \
	$(AM_CFLAGS) \
	$(NULL)

demos_scrapbook_scrapbook_LDADD = \
	$(top_builddir)/gdata/libgdata.la \
	$(GTK_LIBS) \
	$(GDATA_LIBS) \
	$(GNOME_LIBS) \
	$(AM_LDADD) \
	$(NULL)

# Cleaning
EXTRA_DIST += \
	autogen.sh		\
	HACKING			\
	gtk-doc.make		\
	libgdata.doap

CLEANFILES += \
	`find "$(srcdir)" -type f -name .dirstamp -print`

DISTCLEANFILES = \
	gtk-doc.make

MAINTAINERCLEANFILES += \
	$(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
	$(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
	$(GITIGNORE_MAINTAINERCLEANFILES_M4_LIBTOOL) \
	build-aux/ \
	INSTALL \
	omf.make \
	gtk-doc.make \
	ABOUT-NLS \
	config.rpath \
	po/Makevars.template \
	po/Rules-quot \
	po/boldquot.sed \
	po/en@boldquot.header \
	po/en@quot.header \
	po/insert-header.sin \
	po/quot.sed \
	po/remove-potcdate.sin \
	m4/gtk-doc.m4 \
	m4/intltool.m4 \
	$(NULL)

DISTCHECK_CONFIGURE_FLAGS += --enable-gtk-doc

# pkgconfig data
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgdata.pc

CLEANFILES += $(pkgconfig_DATA)
EXTRA_DIST += libgdata.pc.in \
	gdata/gdata-version.h.in

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
.PHONY: $(srcdir)/ChangeLog

MAINTAINERCLEANFILES += ChangeLog

-include $(top_srcdir)/git.mk
