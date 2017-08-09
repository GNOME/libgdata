/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2012 <philip@tecnocode.co.uk>
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

#ifndef GDATA_DOCUMENTS_UPLOAD_QUERY_H
#define GDATA_DOCUMENTS_UPLOAD_QUERY_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/services/documents/gdata-documents-folder.h>

G_BEGIN_DECLS

#define GDATA_TYPE_DOCUMENTS_UPLOAD_QUERY		(gdata_documents_upload_query_get_type ())
#define GDATA_DOCUMENTS_UPLOAD_QUERY(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_DOCUMENTS_UPLOAD_QUERY, GDataDocumentsUploadQuery))
#define GDATA_DOCUMENTS_UPLOAD_QUERY_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_DOCUMENTS_UPLOAD_QUERY, GDataDocumentsUploadQueryClass))
#define GDATA_IS_DOCUMENTS_UPLOAD_QUERY(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_DOCUMENTS_UPLOAD_QUERY))
#define GDATA_IS_DOCUMENTS_UPLOAD_QUERY_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_DOCUMENTS_UPLOAD_QUERY))
#define GDATA_DOCUMENTS_UPLOAD_QUERY_GET_CLASS(o) \
	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_DOCUMENTS_UPLOAD_QUERY, GDataDocumentsUploadQueryClass))

typedef struct _GDataDocumentsUploadQueryPrivate	GDataDocumentsUploadQueryPrivate;

/**
 * GDataDocumentsUploadQuery:
 *
 * All the fields in the #GDataDocumentsUploadQuery structure are private and should never be accessed directly.
 *
 * Since: 0.13.0
 */
typedef struct {
	GObject parent;
	GDataDocumentsUploadQueryPrivate *priv;
} GDataDocumentsUploadQuery;

/**
 * GDataDocumentsUploadQueryClass:
 *
 * All the fields in the #GDataDocumentsUploadQueryClass structure are private and should never be accessed directly.
 *
 * Since: 0.13.0
 */
typedef struct {
	/*< private >*/
	GObjectClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataDocumentsUploadQueryClass;

GType gdata_documents_upload_query_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataDocumentsUploadQuery, g_object_unref)

GDataDocumentsUploadQuery *gdata_documents_upload_query_new (void) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

gchar *gdata_documents_upload_query_build_uri (GDataDocumentsUploadQuery *self) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataDocumentsFolder *gdata_documents_upload_query_get_folder (GDataDocumentsUploadQuery *self) G_GNUC_PURE;
void gdata_documents_upload_query_set_folder (GDataDocumentsUploadQuery *self, GDataDocumentsFolder *folder);

gboolean gdata_documents_upload_query_get_convert (GDataDocumentsUploadQuery *self) G_GNUC_PURE;
void gdata_documents_upload_query_set_convert (GDataDocumentsUploadQuery *self, gboolean convert);

G_END_DECLS

#endif /* !GDATA_DOCUMENTS_UPLOAD_QUERY_H */
