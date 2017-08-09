/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Richard Schwarting 2009 <aquarichy@gmail.com>
 * Copyright (C) Philip Withnall 2009 <philip@tecnocode.co.uk>
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

#ifndef GDATA_PICASAWEB_SERVICE_H
#define GDATA_PICASAWEB_SERVICE_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-service.h>
#include <gdata/gdata-upload-stream.h>
#include <gdata/services/picasaweb/gdata-picasaweb-album.h>
#include <gdata/services/picasaweb/gdata-picasaweb-user.h>

G_BEGIN_DECLS

#define GDATA_TYPE_PICASAWEB_SERVICE		(gdata_picasaweb_service_get_type ())
#define GDATA_PICASAWEB_SERVICE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_PICASAWEB_SERVICE, GDataPicasaWebService))
#define GDATA_PICASAWEB_SERVICE_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_PICASAWEB_SERVICE, GDataPicasaWebServiceClass))
#define GDATA_IS_PICASAWEB_SERVICE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_PICASAWEB_SERVICE))
#define GDATA_IS_PICASAWEB_SERVICE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_PICASAWEB_SERVICE))
#define GDATA_PICASAWEB_SERVICE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_PICASAWEB_SERVICE, GDataPicasaWebServiceClass))

/**
 * GDataPicasaWebService:
 *
 * All the fields in the #GDataPicasaWebService structure are private and should never be accessed directly.
 *
 * Since: 0.4.0
 */
typedef struct {
	GDataService parent;
} GDataPicasaWebService;

/**
 * GDataPicasaWebServiceClass:
 *
 * All the fields in the #GDataPicasaWebServiceClass structure are private and should never be accessed directly.
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
} GDataPicasaWebServiceClass;

GType gdata_picasaweb_service_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataPicasaWebService, g_object_unref)

GDataPicasaWebService *gdata_picasaweb_service_new (GDataAuthorizer *authorizer) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataAuthorizationDomain *gdata_picasaweb_service_get_primary_authorization_domain (void) G_GNUC_CONST;

#include <gdata/services/picasaweb/gdata-picasaweb-query.h>

GDataPicasaWebUser *gdata_picasaweb_service_get_user (GDataPicasaWebService *self, const gchar *username,
                                                      GCancellable *cancellable, GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_picasaweb_service_get_user_async (GDataPicasaWebService *self, const gchar *username, GCancellable *cancellable,
                                             GAsyncReadyCallback callback, gpointer user_data);
GDataPicasaWebUser *gdata_picasaweb_service_get_user_finish (GDataPicasaWebService *self, GAsyncResult *result,
                                                             GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataFeed *gdata_picasaweb_service_query_all_albums (GDataPicasaWebService *self, GDataQuery *query, const gchar *username, GCancellable *cancellable,
                                                     GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                     GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_picasaweb_service_query_all_albums_async (GDataPicasaWebService *self, GDataQuery *query, const gchar *username, GCancellable *cancellable,
                                                     GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                     GDestroyNotify destroy_progress_user_data,
                                                     GAsyncReadyCallback callback, gpointer user_data);

GDataFeed *gdata_picasaweb_service_query_files (GDataPicasaWebService *self, GDataPicasaWebAlbum *album, GDataQuery *query,
                                                GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_picasaweb_service_query_files_async (GDataPicasaWebService *self, GDataPicasaWebAlbum *album, GDataQuery *query, GCancellable *cancellable,
                                                GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                GDestroyNotify destroy_progress_user_data,
                                                GAsyncReadyCallback callback, gpointer user_data);

#include <gdata/services/picasaweb/gdata-picasaweb-file.h>

GDataUploadStream *gdata_picasaweb_service_upload_file (GDataPicasaWebService *self, GDataPicasaWebAlbum *album, GDataPicasaWebFile *file_entry,
                                                        const gchar *slug, const gchar *content_type, GCancellable *cancellable,
                                                        GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
GDataPicasaWebFile *gdata_picasaweb_service_finish_file_upload (GDataPicasaWebService *self, GDataUploadStream *upload_stream,
                                                                GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataPicasaWebAlbum *gdata_picasaweb_service_insert_album (GDataPicasaWebService *self, GDataPicasaWebAlbum *album, GCancellable *cancellable,
                                                           GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_picasaweb_service_insert_album_async (GDataPicasaWebService *self, GDataPicasaWebAlbum *album, GCancellable *cancellable,
                                                 GAsyncReadyCallback callback, gpointer user_data);

G_END_DECLS

#endif /* !GDATA_PICASAWEB_SERVICE_H */
