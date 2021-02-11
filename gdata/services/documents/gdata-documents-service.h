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
#include <gdata/gdata-upload-stream.h>
#include <gdata/services/documents/gdata-documents-query.h>
#include <gdata/services/documents/gdata-documents-feed.h>
#include <gdata/services/documents/gdata-documents-metadata.h>
#include <gdata/services/documents/gdata-documents-drive-query.h>

G_BEGIN_DECLS

/**
 * GDataDocumentsServiceError:
 * @GDATA_DOCUMENTS_SERVICE_ERROR_INVALID_CONTENT_TYPE: the content type of a provided file was invalid
 *
 * Error codes for #GDataDocumentsService operations.
 *
 * Since: 0.4.0
 */
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
 */
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
 */
typedef struct {
	/*< private >*/
	GDataServiceClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
	void (*_g_reserved2) (void);
	void (*_g_reserved3) (void);
	void (*_g_reserved4) (void);
	void (*_g_reserved5) (void);
} GDataDocumentsServiceClass;

GType gdata_documents_service_get_type (void) G_GNUC_CONST;
GQuark gdata_documents_service_error_quark (void) G_GNUC_CONST;

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataDocumentsService, g_object_unref)

GDataDocumentsService *gdata_documents_service_new (GDataAuthorizer *authorizer) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataAuthorizationDomain *gdata_documents_service_get_primary_authorization_domain (void) G_GNUC_CONST;
GDataAuthorizationDomain *gdata_documents_service_get_spreadsheet_authorization_domain (void) G_GNUC_CONST;

GDataDocumentsMetadata *gdata_documents_service_get_metadata (GDataDocumentsService *self, GCancellable *cancellable,
                                                              GError **error)  G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_documents_service_get_metadata_async (GDataDocumentsService *self, GCancellable *cancellable,
                                                 GAsyncReadyCallback callback, gpointer user_data);
GDataDocumentsMetadata *gdata_documents_service_get_metadata_finish (GDataDocumentsService *self, GAsyncResult *async_result,
                                                                     GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataDocumentsFeed *gdata_documents_service_query_documents (GDataDocumentsService *self, GDataDocumentsQuery *query, GCancellable *cancellable,
                                                             GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                             GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_documents_service_query_documents_async (GDataDocumentsService *self, GDataDocumentsQuery *query, GCancellable *cancellable,
                                                    GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                    GDestroyNotify destroy_progress_user_data,
                                                    GAsyncReadyCallback callback, gpointer user_data);

GDataDocumentsFeed *gdata_documents_service_query_drives (GDataDocumentsService *self, GDataDocumentsDriveQuery *query, GCancellable *cancellable,
                                                          GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                          GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_documents_service_query_drives_async (GDataDocumentsService *self, GDataDocumentsDriveQuery *query, GCancellable *cancellable,
                                                 GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                 GDestroyNotify destroy_progress_user_data,
                                                 GAsyncReadyCallback callback, gpointer user_data);

#include <gdata/services/documents/gdata-documents-document.h>
#include <gdata/services/documents/gdata-documents-folder.h>
#include <gdata/services/documents/gdata-documents-upload-query.h>

GDataUploadStream *gdata_documents_service_upload_document (GDataDocumentsService *self, GDataDocumentsDocument *document, const gchar *slug,
                                                            const gchar *content_type, GDataDocumentsFolder *folder,
                                                            GCancellable *cancellable, GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
GDataUploadStream *gdata_documents_service_upload_document_resumable (GDataDocumentsService *self, GDataDocumentsDocument *document, const gchar *slug,
                                                                      const gchar *content_type, goffset content_length,
                                                                      GDataDocumentsUploadQuery *query,
                                                                      GCancellable *cancellable, GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataUploadStream *gdata_documents_service_update_document (GDataDocumentsService *self, GDataDocumentsDocument *document, const gchar *slug,
                                                            const gchar *content_type, GCancellable *cancellable,
                                                            GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
GDataUploadStream *gdata_documents_service_update_document_resumable (GDataDocumentsService *self, GDataDocumentsDocument *document, const gchar *slug,
                                                                      const gchar *content_type, goffset content_length, GCancellable *cancellable,
                                                                      GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataDocumentsDocument *gdata_documents_service_finish_upload (GDataDocumentsService *self, GDataUploadStream *upload_stream,
                                                               GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataDocumentsDocument *gdata_documents_service_copy_document (GDataDocumentsService *self, GDataDocumentsDocument *document,
                                                               GCancellable *cancellable, GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_documents_service_copy_document_async (GDataDocumentsService *self, GDataDocumentsDocument *document, GCancellable *cancellable,
                                                  GAsyncReadyCallback callback, gpointer user_data);
GDataDocumentsDocument *gdata_documents_service_copy_document_finish (GDataDocumentsService *self, GAsyncResult *async_result,
                                                                      GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataDocumentsEntry *gdata_documents_service_add_entry_to_folder (GDataDocumentsService *self, GDataDocumentsEntry *entry,
                                                                  GDataDocumentsFolder *folder, GCancellable *cancellable,
                                                                  GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_documents_service_add_entry_to_folder_async (GDataDocumentsService *self, GDataDocumentsEntry *entry, GDataDocumentsFolder *folder,
                                                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
GDataDocumentsEntry *gdata_documents_service_add_entry_to_folder_finish (GDataDocumentsService *self, GAsyncResult *async_result,
                                                                         GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataDocumentsEntry *gdata_documents_service_remove_entry_from_folder (GDataDocumentsService *self, GDataDocumentsEntry *entry,
                                                                       GDataDocumentsFolder *folder, GCancellable *cancellable,
                                                                       GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_documents_service_remove_entry_from_folder_async (GDataDocumentsService *self, GDataDocumentsEntry *entry, GDataDocumentsFolder *folder,
                                                             GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
GDataDocumentsEntry *gdata_documents_service_remove_entry_from_folder_finish (GDataDocumentsService *self, GAsyncResult *async_result,
                                                                              GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

gchar *gdata_documents_service_get_upload_uri (GDataDocumentsFolder *folder) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS
#endif /* !GDATA_DOCUMENTS_SERVICE_H */
