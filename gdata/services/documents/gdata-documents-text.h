/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Thibault Saunier 2009 <saunierthibault@gmail.com>
 *
 * GData Client is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * GData Client is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GData Client.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GDATA_DOCUMENTS_TEXT_H
#define GDATA_DOCUMENTS_TEXT_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/services/documents/gdata-documents-entry.h>
#include <gdata/gdata-types.h>

G_BEGIN_DECLS

#define GDATA_TYPE_DOCUMENTS_TEXT		(gdata_documents_text_get_type ())
#define GDATA_DOCUMENTS_TEXT(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_DOCUMENTS_TEXT, GDataDocumentsText))
#define GDATA_DOCUMENTS_TEXT_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_DOCUMENTS_TEXT, GDataDocumentsTextClass))
#define GDATA_IS_DOCUMENTS_TEXT(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_DOCUMENTS_TEXT))
#define GDATA_IS_DOCUMENTS_TEXT_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_DOCUMENTS_TEXT))
#define GDATA_DOCUMENTS_TEXT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_DOCUMENTS_TEXT, GDataDocumentsTextClass))

typedef struct _GDataDocumentsTextPrivate	GDataDocumentsTextPrivate;

/**
 * GDataDocumentsTextFormat:
 * @GDATA_DOCUMENTS_TEXT_DOC: Microsoft Word (DOC) format
 * @GDATA_DOCUMENTS_TEXT_HTML: HyperText Markup Language (HTML) format
 * @GDATA_DOCUMENTS_TEXT_ODT: OpenDocument Text (ODT) format
 * @GDATA_DOCUMENTS_TEXT_PDF: Portable Document Format (PDF)
 * @GDATA_DOCUMENTS_TEXT_PNG: Portable Network Graphics (PNG) image format
 * @GDATA_DOCUMENTS_TEXT_RTF: Rich Text Format (RTF)
 * @GDATA_DOCUMENTS_TEXT_TXT: plain text format
 * @GDATA_DOCUMENTS_TEXT_ZIP: ZIP archive containing images and exported HTML
 *
 * The different available download formats for text documents.
 *
 * Since: 0.4.0
 **/
typedef enum {
	GDATA_DOCUMENTS_TEXT_DOC = 0,
	GDATA_DOCUMENTS_TEXT_HTML,
	GDATA_DOCUMENTS_TEXT_ODT,
	GDATA_DOCUMENTS_TEXT_PDF,
	GDATA_DOCUMENTS_TEXT_PNG,
	GDATA_DOCUMENTS_TEXT_RTF,
	GDATA_DOCUMENTS_TEXT_TXT,
	GDATA_DOCUMENTS_TEXT_ZIP,
} GDataDocumentsTextFormat;

/**
 * GDataDocumentsText:
 *
 * All the fields in the #GDataDocumentsText structure are private and should never be accessed directly.
 *
 * Since: 0.4.0
 **/
typedef struct {
	GDataDocumentsEntry parent;
	GDataDocumentsTextPrivate *priv;
} GDataDocumentsText;

/**
 * GDataDocumentsTextClass:
 *
 * All the fields in the #GDataDocumentsTextClass structure are private and should never be accessed directly.
 *
 * Since: 0.4.0
 **/
typedef struct {
	/*< private >*/
	GDataDocumentsEntryClass parent;
} GDataDocumentsTextClass;

GType gdata_documents_text_get_type (void) G_GNUC_CONST;

GDataDocumentsText *gdata_documents_text_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

#include <gdata/services/documents/gdata-documents-service.h>

GFile *gdata_documents_text_download_document (GDataDocumentsText *self, GDataDocumentsService *service, gchar **content_type,
                                               GDataDocumentsTextFormat export_format, GFile *destination_file,
                                               gboolean replace_file_if_exists, GCancellable *cancellable,
                                               GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
gchar *gdata_documents_text_get_download_uri (GDataDocumentsText *self,
                                              GDataDocumentsTextFormat export_format) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_DOCUMENTS_TEXT_H */
