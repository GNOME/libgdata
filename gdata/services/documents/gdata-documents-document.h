/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2010 <philip@tecnocode.co.uk>
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

#ifndef GDATA_DOCUMENTS_DOCUMENT_H
#define GDATA_DOCUMENTS_DOCUMENT_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-download-stream.h>
#include <gdata/services/documents/gdata-documents-entry.h>

G_BEGIN_DECLS

#define GDATA_TYPE_DOCUMENTS_DOCUMENT		(gdata_documents_document_get_type ())
#define GDATA_DOCUMENTS_DOCUMENT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_DOCUMENTS_DOCUMENT, GDataDocumentsDocument))
#define GDATA_DOCUMENTS_DOCUMENT_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_DOCUMENTS_DOCUMENT, GDataDocumentsDocumentClass))
#define GDATA_IS_DOCUMENTS_DOCUMENT(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_DOCUMENTS_DOCUMENT))
#define GDATA_IS_DOCUMENTS_DOCUMENT_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_DOCUMENTS_DOCUMENT))
#define GDATA_DOCUMENTS_DOCUMENT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_DOCUMENTS_DOCUMENT, GDataDocumentsDocumentClass))

typedef struct _GDataDocumentsDocumentPrivate	GDataDocumentsDocumentPrivate;

/**
 * GDataDocumentsDocument:
 *
 * All the fields in the #GDataDocumentsDocument structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	GDataDocumentsEntry parent;
	GDataDocumentsDocumentPrivate *priv;
} GDataDocumentsDocument;

/**
 * GDataDocumentsDocumentClass:
 *
 * All the fields in the #GDataDocumentsDocumentClass structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	/*< private >*/
	GDataDocumentsEntryClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataDocumentsDocumentClass;

GType gdata_documents_document_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataDocumentsDocument, g_object_unref)

GDataDocumentsDocument *gdata_documents_document_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

#include <gdata/services/documents/gdata-documents-service.h>

GDataDownloadStream *gdata_documents_document_download (GDataDocumentsDocument *self, GDataDocumentsService *service, const gchar *export_format,
                                                        GCancellable *cancellable, GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
gchar *gdata_documents_document_get_download_uri (GDataDocumentsDocument *self, const gchar *export_format) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_documents_document_get_thumbnail_uri (GDataDocumentsDocument *self) G_GNUC_PURE;

G_END_DECLS

#endif /* !GDATA_DOCUMENTS_DOCUMENT_H */
