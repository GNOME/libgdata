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

#ifndef GDATA_DOCUMENTS_SERVICE_H
#define GDATA_DOCUMENTS_SERVICE_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gdata/gdata-service.h>
#include <gdata/services/documents/gdata-documents-query.h>
#include <gdata/services/documents/gdata-documents-feed.h>

G_BEGIN_DECLS

/**
 * GDataDocumentsServiceError:
 * @GDATA_DOCUMENTS_SERVICE_ERROR_INVALID_CONTENT_TYPE: the content type of a provided file was invalid
 *
 * Error codes for #GDataDocumentsService operations.
 *
 * Since: 0.4.0
 **/
typedef enum {
	GDATA_DOCUMENTS_SERVICE_ERROR_INVALID_CONTENT_TYPE
} GDataDocumentsServiceError;

#define GDATA_TYPE_DOCUMENTS_SERVICE		(gdata_documents_service_get_type ())
#define GDATA_DOCUMENTS_SERVICE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_DOCUMENTS_SERVICE, GDataDocumentsService))
#define GDATA_DOCUMENTS_SERVICE_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_DOCUMENTS_SERVICE, GDataDocumentsServiceClass))
#define GDATA_IS_DOCUMENTS_SERVICE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_DOCUMENTS_SERVICE))
#define GDATA_IS_DOCUMENTS_SERVICE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_DOCUMENTS_SERVICE))
#define GDATA_DOCUMENTS_SERVICE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_DOCUMENTS_SERVICE, GDataDocumentsServiceClass))

#define GDATA_DOCUMENTS_SERVICE_ERROR		gdata_documents_service_error_quark ()

typedef struct _GDataDocumentsServicePrivate	GDataDocumentsServicePrivate;

/**
 * GDataDocumentsService:
 *
 * All the fields in the #GDataDocumentsService structure are private and should never be accessed directly.
 *
 * Since: 0.4.0
 **/
typedef struct {
	GDataService parent;
	GDataDocumentsServicePrivate *priv;
} GDataDocumentsService;

/**
 * GDataDocumentsServiceClass:
 *
 * All the fields in the #GDataDocumentsServiceClass structure are private and should never be accessed directly.
 *
 * Since: 0.4.0
 **/
typedef struct {
	/*< private >*/
	GDataServiceClass parent;
} GDataDocumentsServiceClass;

GType gdata_documents_service_get_type (void) G_GNUC_CONST;
GQuark gdata_documents_service_error_quark (void) G_GNUC_CONST;

GDataDocumentsService *gdata_documents_service_new (const gchar *client_id) G_GNUC_WARN_UNUSED_RESULT;
GDataDocumentsFeed *gdata_documents_service_query_documents (GDataDocumentsService *self, GDataDocumentsQuery *query, GCancellable *cancellable,
							     GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
							     GError **error) G_GNUC_WARN_UNUSED_RESULT;
void gdata_documents_service_query_documents_async (GDataDocumentsService *self, GDataDocumentsQuery *query, GCancellable *cancellable,
						    GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
						    GAsyncReadyCallback callback, gpointer user_data);

#include <gdata/services/documents/gdata-documents-folder.h>

GDataDocumentsEntry *gdata_documents_service_upload_document (GDataDocumentsService *self, GDataDocumentsEntry *document, GFile *document_file,
							      GDataDocumentsFolder *folder, GCancellable *cancellable,
							      GError **error) G_GNUC_WARN_UNUSED_RESULT;
GDataDocumentsEntry *gdata_documents_service_update_document (GDataDocumentsService *self, GDataDocumentsEntry *document, GFile *document_file,
							      GCancellable *cancellable, GError **error) G_GNUC_WARN_UNUSED_RESULT;
GDataDocumentsEntry *gdata_documents_service_move_document_to_folder (GDataDocumentsService *self, GDataDocumentsEntry *document,
								      GDataDocumentsFolder *folder, GCancellable *cancellable,
								      GError **error) G_GNUC_WARN_UNUSED_RESULT;
GDataDocumentsEntry *gdata_documents_service_remove_document_from_folder (GDataDocumentsService *self, GDataDocumentsEntry *document,
									  GDataDocumentsFolder *folder,
									  GCancellable *cancellable, GError **error) G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS
#endif /* !GDATA_DOCUMENTS_SERVICE_H */
