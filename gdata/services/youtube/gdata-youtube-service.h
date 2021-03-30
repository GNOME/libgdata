/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2008-2009, 2015 <philip@tecnocode.co.uk>
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

#ifndef GDATA_YOUTUBE_SERVICE_H
#define GDATA_YOUTUBE_SERVICE_H

#include <glib.h>
#include <glib-object.h>
#include <libsoup/soup.h>

#include <gdata/gdata-service.h>
#include <gdata/gdata-upload-stream.h>
#include <gdata/services/youtube/gdata-youtube-video.h>
#include <gdata/app/gdata-app-categories.h>

G_BEGIN_DECLS

/**
 * GDataYouTubeStandardFeedType:
 * @GDATA_YOUTUBE_MOST_POPULAR_FEED: This feed contains the most popular YouTube
 *   videos, selected using an algorithm that combines many different signals to
 *   determine overall popularity. As of version 0.17.0, this is the only
 *   supported feed type.
 *
 * Standard feed types for standard feed queries with
 * gdata_youtube_service_query_standard_feed(). For more information, see the
 * <ulink type="http" url="https://developers.google.com/youtube/2.0/developers_guide_protocol_video_feeds#Standard_feeds">online
 * documentation</ulink>.
 */
typedef enum {
	GDATA_YOUTUBE_MOST_POPULAR_FEED
} GDataYouTubeStandardFeedType;

/**
 * GDataYouTubeServiceError:
 * @GDATA_YOUTUBE_SERVICE_ERROR_API_QUOTA_EXCEEDED: the API request quota for this developer account has been exceeded
 * @GDATA_YOUTUBE_SERVICE_ERROR_ENTRY_QUOTA_EXCEEDED: the entry (e.g. video) quota for this user account has been exceeded
 * @GDATA_YOUTUBE_SERVICE_ERROR_CHANNEL_REQUIRED: the currently authenticated user doesn't have a YouTube channel, but the current action requires one;
 * if this error is received, inform the user that they need a YouTube channel, and provide a link to
 * <ulink type="http" url="https://www.youtube.com/create_channel">https://www.youtube.com/create_channel</ulink>
 *
 * Error codes for #GDataYouTubeService operations.
 */
typedef enum {
	GDATA_YOUTUBE_SERVICE_ERROR_API_QUOTA_EXCEEDED,
	GDATA_YOUTUBE_SERVICE_ERROR_ENTRY_QUOTA_EXCEEDED,
	GDATA_YOUTUBE_SERVICE_ERROR_CHANNEL_REQUIRED,
} GDataYouTubeServiceError;

#define GDATA_TYPE_YOUTUBE_SERVICE		(gdata_youtube_service_get_type ())
#define GDATA_YOUTUBE_SERVICE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_YOUTUBE_SERVICE, GDataYouTubeService))
#define GDATA_YOUTUBE_SERVICE_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_YOUTUBE_SERVICE, GDataYouTubeServiceClass))
#define GDATA_IS_YOUTUBE_SERVICE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_YOUTUBE_SERVICE))
#define GDATA_IS_YOUTUBE_SERVICE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_YOUTUBE_SERVICE))
#define GDATA_YOUTUBE_SERVICE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_YOUTUBE_SERVICE, GDataYouTubeServiceClass))

#define GDATA_YOUTUBE_SERVICE_ERROR		gdata_youtube_service_error_quark ()

typedef struct _GDataYouTubeServicePrivate	GDataYouTubeServicePrivate;

/**
 * GDataYouTubeService:
 *
 * All the fields in the #GDataYouTubeService structure are private and should never be accessed directly.
 */
typedef struct {
	GDataService parent;
	GDataYouTubeServicePrivate *priv;
} GDataYouTubeService;

/**
 * GDataYouTubeServiceClass:
 *
 * All the fields in the #GDataYouTubeServiceClass structure are private and should never be accessed directly.
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
} GDataYouTubeServiceClass;

GType gdata_youtube_service_get_type (void) G_GNUC_CONST;
GQuark gdata_youtube_service_error_quark (void) G_GNUC_CONST;

GDataYouTubeService *gdata_youtube_service_new (const gchar *developer_key, GDataAuthorizer *authorizer) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataAuthorizationDomain *gdata_youtube_service_get_primary_authorization_domain (void) G_GNUC_CONST;

GDataFeed *gdata_youtube_service_query_standard_feed (GDataYouTubeService *self, GDataYouTubeStandardFeedType feed_type, GDataQuery *query,
                                                      GCancellable *cancellable,
                                                      GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                      GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_youtube_service_query_standard_feed_async (GDataYouTubeService *self, GDataYouTubeStandardFeedType feed_type, GDataQuery *query,
                                                      GCancellable *cancellable,
                                                      GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                      GDestroyNotify destroy_progress_user_data,
                                                      GAsyncReadyCallback callback, gpointer user_data);

GDataFeed *gdata_youtube_service_query_videos (GDataYouTubeService *self, GDataQuery *query,
                                               GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                               GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_youtube_service_query_videos_async (GDataYouTubeService *self, GDataQuery *query,
                                               GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                               GDestroyNotify destroy_progress_user_data,
                                               GAsyncReadyCallback callback, gpointer user_data);

GDataFeed *gdata_youtube_service_query_related (GDataYouTubeService *self, GDataYouTubeVideo *video, GDataQuery *query,
                                                GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_youtube_service_query_related_async (GDataYouTubeService *self, GDataYouTubeVideo *video, GDataQuery *query,
                                                GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                GDestroyNotify destroy_progress_user_data,
                                                GAsyncReadyCallback callback, gpointer user_data);

GDataUploadStream *gdata_youtube_service_upload_video (GDataYouTubeService *self, GDataYouTubeVideo *video, const gchar *slug,
                                                       const gchar *content_type, GCancellable *cancellable,
                                                       GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
GDataYouTubeVideo *gdata_youtube_service_finish_video_upload (GDataYouTubeService *self, GDataUploadStream *upload_stream,
                                                              GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_youtube_service_get_developer_key (GDataYouTubeService *self) G_GNUC_PURE;

GDataAPPCategories *gdata_youtube_service_get_categories (GDataYouTubeService *self, GCancellable *cancellable,
                                                          GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_youtube_service_get_categories_async (GDataYouTubeService *self, GCancellable *cancellable, GAsyncReadyCallback callback,
                                                 gpointer user_data);
GDataAPPCategories *gdata_youtube_service_get_categories_finish (GDataYouTubeService *self, GAsyncResult *async_result,
                                                                 GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_YOUTUBE_SERVICE_H */
