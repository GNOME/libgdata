/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Richard Schwarting 2009 <aquarichy@gmail.com>
 * Copyright (C) Philip Withnall 2009–2010 <philip@tecnocode.co.uk>
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

/**
 * SECTION:gdata-picasaweb-service
 * @short_description: GData PicasaWeb service object
 * @stability: Unstable
 * @include: gdata/services/picasaweb/gdata-picasaweb-service.h
 *
 * #GDataPicasaWebService is a subclass of #GDataService for communicating with the GData API of Google PicasaWeb. It supports querying for files
 * and albums, and uploading files.
 *
 * For more details of PicasaWeb's GData API, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/developers_guide_protocol.html">
 * online documentation</ulink>.
 *
 * <example>
 * 	<title>Authenticating and Creating a New Album</title>
 * 	<programlisting>
 *	GDataPicasaWebService *service;
 *	GDataPicasaWebAlbum *album, *inserted_album;
 *
 *	/<!-- -->* Create a service object and authenticate with the PicasaWeb server *<!-- -->/
 *	service = gdata_picasaweb_service_new ("companyName-applicationName-versionID");
 *	gdata_service_authenticate (GDATA_SERVICE (service), username, password, NULL, NULL);
 *
 *	/<!-- -->* Create a GDataPicasaWebAlbum entry for the new album, setting some information about it *<!-- -->/
 *	album = gdata_picasaweb_album_new (NULL);
 *	gdata_entry_set_title (GDATA_ENTRY (album), "Photos from the Rhine");
 *	gdata_entry_set_summary (GDATA_ENTRY (album), "An album of our adventures on the great river.");
 *	gdata_picasaweb_album_set_location (album, "The Rhine, Germany");
 *
 *	/<!-- -->* Insert the new album on the server. Note that this is a blocking operation. *<!-- -->/
 *	inserted_album = gdata_picasaweb_service_insert_album (service, album, NULL, NULL);
 *
 *	g_object_unref (album);
 *	g_object_unref (inserted_album);
 *	g_object_unref (service);
 *	</programlisting>
 * </example>
 *
 * <example>
 * 	<title>Uploading a Photo or Video</title>
 * 	<programlisting>
 *	GDataPicasaWebFile *file_entry, *uploaded_file_entry;
 *	GFile *file_data;
 *
 *	/<!-- -->* Specify the GFile image on disk to upload *<!-- -->/
 *	file_data = g_file_new_for_path (path);
 *
 *	/<!-- -->* Create a GDataPicasaWebFile entry for the image, setting a title and caption/summary *<!-- -->/
 *	file_entry = gdata_picasaweb_file_new (NULL);
 *	gdata_entry_set_title (GDATA_ENTRY (file_entry), "Black Cat");
 *	gdata_entry_set_summary (GDATA_ENTRY (file_entry), "Photo of the world's most beautiful cat.");
 *
 *	/<!-- -->* Upload the file to the server. Note that this is a blocking operation. *<!-- -->/
 *	uploaded_file_entry = gdata_picasaweb_service_upload_file (service, album, file_entry, file_data, NULL, NULL);
 *
 *	g_object_unref (file_entry);
 *	g_object_unref (uploaded_file_entry);
 *	g_object_unref (file_data);
 * 	</programlisting>
 * </example>
 *
 *
 * Since: 0.4.0
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "gdata-service.h"
#include "gdata-picasaweb-service.h"
#include "gdata-private.h"
#include "gdata-parser.h"
#include "atom/gdata-link.h"
#include "gdata-upload-stream.h"
#include "gdata-picasaweb-feed.h"

G_DEFINE_TYPE (GDataPicasaWebService, gdata_picasaweb_service, GDATA_TYPE_SERVICE)

static void
gdata_picasaweb_service_class_init (GDataPicasaWebServiceClass *klass)
{
	GDataServiceClass *service_class = GDATA_SERVICE_CLASS (klass);
	service_class->service_name = "lh2";
	service_class->feed_type = GDATA_TYPE_PICASAWEB_FEED;
}

static void
gdata_picasaweb_service_init (GDataPicasaWebService *self)
{
	/* Nothing to see here */
}

/**
 * gdata_picasaweb_service_new:
 * @client_id: your application's client ID
 *
 * Creates a new #GDataPicasaWebService. The @client_id must be unique for your application, and as registered with Google.
 * The <ulink type="http" url="http://code.google.com/apis/accounts/docs/AuthForInstalledApps.html#Request">recommended
 * form</ulink> is "companyName-applicationName-versionID".
 *
 * Return value: a new #GDataPicasaWebService, or %NULL
 *
 * Since: 0.4.0
 **/
GDataPicasaWebService *
gdata_picasaweb_service_new (const gchar *client_id)
{
	g_return_val_if_fail (client_id != NULL, NULL);
	return g_object_new (GDATA_TYPE_PICASAWEB_SERVICE, "client-id", client_id, NULL);
}

/*
 * create_uri:
 * @self: a #GDataPicasaWebService
 * @username: the username to use, or %NULL to use the currently logged in user
 * @type: the type of object to access: "entry" for a user, or "feed" for an album
 *
 * Builds a URI to use when querying for albums or a user.
 *
 * Return value: a constructed URI; free with g_free()
 *
 * Since: 0.4.0
 */
static gchar *
create_uri (GDataPicasaWebService *self, const gchar *username, const gchar *type)
{
	if (username == NULL) {
		/* Ensure we're authenticated first */
		if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE)
			return NULL;

		/* Querying Picasa albums for the "default" user when logged in returns the albums for the authenticated user */
		username = "default";
	}

	return g_strdup_printf ("http://picasaweb.google.com/data/%s/api/user/%s", type, username);
}

/**
 * gdata_picasaweb_service_get_user
 * @self: a #GDataPicasaWebService
 * @username: the username of the user whose information you wish to retrieve, or %NULL for the currently authenticated user.
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Queries the service to return the user specified by @username.
 *
 * Return value: a #GDataPicasaWebUser; unref with g_object_unref()
 *
 * Since: 0.6.0
 **/
GDataPicasaWebUser *
gdata_picasaweb_service_get_user (GDataPicasaWebService *self, const gchar *username, GCancellable *cancellable, GError **error)
{
	gchar *uri;
	GDataParsable *user;
	SoupMessage *message;

	g_return_val_if_fail (GDATA_IS_PICASAWEB_SERVICE (self), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	uri = create_uri (self, username, "entry");
	if (uri == NULL) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must specify a username or be authenticated to query a user."));
		return NULL;
	}

	message = _gdata_service_query (GDATA_SERVICE (self), uri, NULL, cancellable, error);
	g_free (uri);

	if (message == NULL)
		return NULL;

	g_assert (message->response_body->data != NULL);
	user = gdata_parsable_new_from_xml (GDATA_TYPE_PICASAWEB_USER, message->response_body->data, message->response_body->length, error);
	g_object_unref (message);

	return GDATA_PICASAWEB_USER (user);
}

/**
 * gdata_picasaweb_service_query_all_albums:
 * @self: a #GDataPicasaWebService
 * @query: a #GDataQuery with the query parameters, or %NULL
 * @username: the username of the user whose albums you wish to retrieve, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service to return a list of all albums belonging to the specified @username which match the given
 * @query. If a user is authenticated with the service, @username can be set as %NULL to return a list of albums belonging
 * to the currently-authenticated user.
 *
 * Note that the #GDataQuery:q query parameter cannot be set on @query for album queries.
 *
 * For more details, see gdata_service_query().
 *
 * Return value: a #GDataFeed of query results; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataFeed *
gdata_picasaweb_service_query_all_albums (GDataPicasaWebService *self, GDataQuery *query, const gchar *username, GCancellable *cancellable,
                                          GDataQueryProgressCallback progress_callback, gpointer progress_user_data, GError **error)
{
	gchar *uri;
	GDataFeed *album_feed;

	g_return_val_if_fail (GDATA_IS_PICASAWEB_SERVICE (self), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (query != NULL && gdata_query_get_q (query) != NULL) {
		/* Bug #593336 — Query parameter "q=..." isn't valid for album kinds */
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_BAD_QUERY_PARAMETER,
		                     _("Query parameter not allowed for albums."));
		return NULL;
	}

	uri = create_uri (self, username, "feed");
	if (uri == NULL) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must specify a username or be authenticated to query all albums."));
		return NULL;
	}

	/* Execute the query */
	album_feed = gdata_service_query (GDATA_SERVICE (self), uri, query, GDATA_TYPE_PICASAWEB_ALBUM,
	                                  cancellable, progress_callback, progress_user_data, error);
	g_free (uri);

	return album_feed;
}

/**
 * gdata_picasaweb_service_query_all_albums_async:
 * @self: a #GDataPicasaWebService
 * @query: a #GDataQuery with the query parameters, or %NULL
 * @username: the username of the user whose albums you wish to retrieve, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: data to pass to the @progress_callback function
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: data to pass to the @callback function
 *
 * Queries the service to return a list of all albums belonging to the specified @username which match the given
 * @query. @self, @query and @username are all reffed/copied when this function is called, so can safely be unreffed/freed after
 * this function returns.
 *
 * For more details, see gdata_picasaweb_service_query_all_albums(), which is the synchronous version of
 * this function, and gdata_service_query_async(), which is the base asynchronous query function.
 *
 * Since: 0.4.0
 **/
void
gdata_picasaweb_service_query_all_albums_async (GDataPicasaWebService *self, GDataQuery *query, const gchar *username,
                                                GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *uri;

	g_return_if_fail (GDATA_IS_PICASAWEB_SERVICE (self));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	if (query != NULL && gdata_query_get_q (query) != NULL) {
		/* Bug #593336 — Query parameter "q=..." isn't valid for album kinds */
		g_simple_async_report_error_in_idle (G_OBJECT (self), callback, user_data,
		                                     GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_BAD_QUERY_PARAMETER,
		                                     _("Query parameter not allowed for albums."));
		return;
	}

	uri = create_uri (self, username, "feed");
	if (uri == NULL) {
		g_simple_async_report_error_in_idle (G_OBJECT (self), callback, user_data,
		                                     GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                                     _("You must specify a username or be authenticated to query all albums."));
		return;
	}

	/* Schedule the async query */
	gdata_service_query_async (GDATA_SERVICE (self), uri, query, GDATA_TYPE_PICASAWEB_ALBUM, cancellable, progress_callback, progress_user_data,
	                           callback, user_data);
	g_free (uri);
}

/**
 * gdata_picasaweb_service_query_files:
 * @self: a #GDataPicasaWebService
 * @album: a #GDataPicasaWebAlbum from which to retrieve the files, or %NULL
 * @query: a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the specified @album for a list of the files which match the given @query. If @album is %NULL and a user is
 * authenticated with the service, the user's default album will be queried.
 *
 * For more details, see gdata_service_query().
 *
 * Return value: a #GDataFeed of query results; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataFeed *
gdata_picasaweb_service_query_files (GDataPicasaWebService *self, GDataPicasaWebAlbum *album, GDataQuery *query, GCancellable *cancellable,
                                     GDataQueryProgressCallback progress_callback, gpointer progress_user_data, GError **error)
{
	/* TODO: Async variant */
	const gchar *uri;

	g_return_val_if_fail (GDATA_IS_PICASAWEB_SERVICE (self), NULL);
	g_return_val_if_fail (album == NULL || GDATA_IS_PICASAWEB_ALBUM (album), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (album != NULL) {
		GDataLink *link = gdata_entry_look_up_link (GDATA_ENTRY (album), "http://schemas.google.com/g/2005#feed");
		if (link == NULL) {
			/* Error */
			g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
			                     _("The album did not have a feed link."));
			return NULL;
		}
		uri = gdata_link_get_uri (link);
	} else {
		/* Default URI */
		uri = "http://picasaweb.google.com/data/feed/api/user/default/albumid/default";
	}

	/* Execute the query */
	return gdata_service_query (GDATA_SERVICE (self), uri, GDATA_QUERY (query), GDATA_TYPE_PICASAWEB_FILE, cancellable,
	                            progress_callback, progress_user_data, error);
}

static GOutputStream *
get_file_output_stream (GDataPicasaWebService *self, GDataPicasaWebAlbum *album, GDataPicasaWebFile *file_entry, GFile *file_data, GError **error)
{
	GFileInfo *file_info = NULL;
	const gchar *slug = NULL, *content_type = NULL, *user_id = NULL, *album_id = NULL;
	GOutputStream *output_stream;
	gchar *upload_uri;

	/* PicasaWeb allows you to post to a default Dropbox */
	album_id = (album != NULL) ? gdata_entry_get_id (GDATA_ENTRY (album)) : "default";
	user_id = gdata_service_get_username (GDATA_SERVICE (self));

	file_info = g_file_query_info (file_data, "standard::display-name,standard::content-type", G_FILE_QUERY_INFO_NONE, NULL, error);
	if (file_info == NULL)
		return NULL;

	slug = g_file_info_get_display_name (file_info);
	content_type = g_file_info_get_content_type (file_info);

	/* Build the upload URI and upload stream */
	upload_uri = g_strdup_printf ("http://picasaweb.google.com/data/feed/api/user/%s/albumid/%s", user_id, album_id);
	output_stream = gdata_upload_stream_new (GDATA_SERVICE (self), SOUP_METHOD_POST, upload_uri, GDATA_ENTRY (file_entry), slug, content_type);
	g_free (upload_uri);
	g_object_unref (file_info);

	return output_stream;
}

static GDataPicasaWebFile *
parse_spliced_stream (GOutputStream *output_stream, GError **error)
{
	const gchar *response_body;
	gssize response_length;
	GDataPicasaWebFile *new_entry;

	/* Get the response from the server */
	response_body = gdata_upload_stream_get_response (GDATA_UPLOAD_STREAM (output_stream), &response_length);
	g_assert (response_body != NULL && response_length > 0);

	/* Parse the response to produce a GDataPicasaWebFile */
	new_entry = GDATA_PICASAWEB_FILE (gdata_parsable_new_from_xml (GDATA_TYPE_PICASAWEB_FILE, response_body, (gint) response_length, error));

	return new_entry;
}

/**
 * gdata_picasaweb_service_upload_file:
 * @self: a #GDataPicasaWebService
 * @album: a #GDataPicasaWebAlbum into which to insert the file, or %NULL
 * @file_entry: a #GDataPicasaWebFile to insert
 * @file_data: the actual file to upload
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Uploads a file (photo or video) to the given PicasaWeb @album, using the @actual_file from disk and the metadata from @file. If @album is
 * %NULL, the file will be uploaded to the currently-authenticated user's "Drop Box" album. A user must be authenticated to use this function.
 *
 * If @file has already been inserted, a %GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED error will be returned. If no user is authenticated
 * with the service, %GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED will be returned.
 *
 * If there is a problem reading @file_data, an error from g_output_stream_splice() or g_file_query_info() will be returned. Other errors from
 * #GDataServiceError can be returned for other exceptional conditions, as determined by the server.
 *
 * Return value: the inserted #GDataPicasaWebFile with updated properties from @file_entry; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataPicasaWebFile *
gdata_picasaweb_service_upload_file (GDataPicasaWebService *self, GDataPicasaWebAlbum *album, GDataPicasaWebFile *file_entry, GFile *file_data,
                                     GCancellable *cancellable, GError **error)
{
	GOutputStream *output_stream;
	GInputStream *input_stream;
	GDataPicasaWebFile *new_entry;
	GError *child_error = NULL;

	g_return_val_if_fail (GDATA_IS_PICASAWEB_SERVICE (self), NULL);
	g_return_val_if_fail (album == NULL || GDATA_IS_PICASAWEB_ALBUM (album), NULL);
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (file_entry), NULL);
	g_return_val_if_fail (G_IS_FILE (file_data), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (gdata_entry_is_inserted (GDATA_ENTRY (file_entry)) == TRUE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED,
		                     _("The entry has already been inserted."));
		return NULL;
	}

	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to upload a file."));
		return NULL;
	}

	output_stream = get_file_output_stream (self, album, file_entry, file_data, error);
	if (output_stream == NULL)
		return NULL;

	/* Pipe the input file to the upload stream */
	input_stream = G_INPUT_STREAM (g_file_read (file_data, cancellable, error));
	if (input_stream == NULL) {
		g_object_unref (output_stream);
		return NULL;
	}

	g_output_stream_splice (output_stream, input_stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
	                        cancellable, &child_error);

	g_object_unref (input_stream);
	if (child_error != NULL) {
		g_object_unref (output_stream);
		g_propagate_error (error, child_error);
		return NULL;
	}

	new_entry = parse_spliced_stream (output_stream, error);
	g_object_unref (output_stream);

	return new_entry;
}

/**
 * gdata_picasaweb_service_upload_file_finish:
 * @self: a #GDataPicasaWebService
 * @result: a #GSimpleAsyncResult
 * @error: a #GError, or %NULL
 *
 * This should be called to obtain the result of a call to
 * gdata_picasaweb_service_upload_file_async() and to check for
 * errors.
 *
 * If there is a problem reading the subect file's data, an error
 * from g_output_stream_splice() or g_file_query_info() will be
 * returned. Other errors from #GDataServiceError can be returned for
 * other exceptional conditions, as determined by the server.
 *
 * If the file to upload has already been inserted, a
 * %GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED error will be set. If
 * no user is authenticated with the service when trying to upload it,
 * %GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED will be set.
 *
 * Return value: the inserted #GDataPicasaWebFile; unref with
 * g_object_unref()
 *
 * Since: 0.6.0
 */
GDataPicasaWebFile *
gdata_picasaweb_service_upload_file_finish (GDataPicasaWebService *self, GAsyncResult *result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;

	g_assert (gdata_picasaweb_service_upload_file_async == g_simple_async_result_get_source_tag (G_SIMPLE_ASYNC_RESULT (result)));

	return GDATA_PICASAWEB_FILE (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}

typedef struct {
	GDataPicasaWebService *service;
	GAsyncReadyCallback callback;
	gpointer user_data;
} UploadFileAsyncData;

static void
upload_file_async_data_free (UploadFileAsyncData *data)
{
	g_object_unref (data->service);
	g_slice_free (UploadFileAsyncData, data);
}

static void
upload_file_async_cb (GOutputStream *output_stream, GAsyncResult *result, UploadFileAsyncData *data)
{
	GError *error = NULL;
	GDataPicasaWebFile *file = NULL;
	GSimpleAsyncResult *async_result;

	g_output_stream_splice_finish (output_stream, result, &error);

	/* If we're error free, parse the file from the stream */
	if (error == NULL)
		file = parse_spliced_stream (output_stream, &error);

	if (error == NULL && file != NULL)
		async_result = g_simple_async_result_new (G_OBJECT (data->service), (GAsyncReadyCallback) data->callback,
		                                          data->user_data, gdata_picasaweb_service_upload_file_async);
	else
		async_result = g_simple_async_result_new_from_error (G_OBJECT (data->service), (GAsyncReadyCallback) data->callback,
		                                                     data->user_data, error);

	g_simple_async_result_set_op_res_gpointer (async_result, file, NULL);

	g_simple_async_result_complete (async_result);

	upload_file_async_data_free (data);
}

/**
 * gdata_picasaweb_service_upload_file_async:
 * @self: a #GDataPicasaWebService
 * @album: a #GDataPicasaWebAlbum into which to insert the file, or %NULL
 * @file_entry: a #GDataPicasaWebFile to insert
 * @file_data: the actual file to upload
 * @cancellable: optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: data to pass to the @callback function
 *
 * Uploads a file (photo or video) to the given PicasaWeb @album
 * asynchronously, using the @actual_file from disk and the metadata
 * from @file. If @album is %NULL, the file will be uploaded to the
 * currently-authenticated user's "Drop Box" album. A user must be
 * authenticated to use this function.
 *
 * @callback should call gdata_picasaweb_service_upload_file_finish()
 * to obtain a #GDataPicasaWebFile representing the uploaded file and
 * check for possible errors.
 *
 * Since: 0.6.0
 **/
void
gdata_picasaweb_service_upload_file_async (GDataPicasaWebService *self, GDataPicasaWebAlbum *album, GDataPicasaWebFile *file_entry,
                                           GFile *file_data, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	GOutputStream *output_stream;
	GInputStream *input_stream;
	UploadFileAsyncData *data;
	GSimpleAsyncResult *result;
	GError *error = NULL;

	g_return_if_fail (GDATA_IS_PICASAWEB_SERVICE (self));
	g_return_if_fail (album == NULL || GDATA_IS_PICASAWEB_ALBUM (album));
	g_return_if_fail (GDATA_IS_PICASAWEB_FILE (file_entry));
	g_return_if_fail (G_IS_FILE (file_data));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	if (gdata_entry_is_inserted (GDATA_ENTRY (file_entry)) == TRUE) {
		g_set_error_literal (&error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED,
		                     _("The entry has already been inserted."));
		goto error;
	}

	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (&error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to upload a file."));
		goto error;
	}

	/* Prepare and retrieve a #GDataOutputStream for the file and its data */
	output_stream = get_file_output_stream (self, album, file_entry, file_data, &error);
	if (output_stream == NULL)
		goto error;

	/* Pipe the input file to the upload stream */
	input_stream = G_INPUT_STREAM (g_file_read (file_data, cancellable, &error));
	if (input_stream == NULL) {
		g_object_unref (output_stream);
		goto error;
	}

	data = g_slice_new (UploadFileAsyncData);
	data->service = g_object_ref (self);
	data->callback = callback;
	data->user_data = user_data;

	/* Actually transfer the data */
	g_output_stream_splice_async (output_stream, input_stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
	                              0, cancellable, (GAsyncReadyCallback) upload_file_async_cb, data);

	g_object_unref (input_stream);
	g_object_unref (output_stream);

	return;

error:
	result = g_simple_async_result_new_from_error (G_OBJECT (self), callback, user_data, error);
	g_simple_async_result_complete (result);
}

/**
 * gdata_picasaweb_service_insert_album:
 * @self: a #GDataPicasaWebService
 * @album: a #GDataPicasaWebAlbum to create on the server
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Inserts a new album described by @album. A user must be
 * authenticated to use this function.
 *
 * Return value: the inserted #GDataPicasaWebAlbum; unref with
 * g_object_unref()
 *
 * Since: 0.6.0
 **/
GDataPicasaWebAlbum *
gdata_picasaweb_service_insert_album (GDataPicasaWebService *self, GDataPicasaWebAlbum *album, GCancellable *cancellable, GError **error)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_PICASAWEB_ALBUM (album), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (gdata_entry_is_inserted (GDATA_ENTRY (album)) == TRUE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED,
		                     _("The album has already been inserted."));
		return NULL;
	}

	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to insert an album."));
		return NULL;
	}

	return GDATA_PICASAWEB_ALBUM (gdata_service_insert_entry (GDATA_SERVICE (self), "http://picasaweb.google.com/data/feed/api/user/default",
	                                                          GDATA_ENTRY (album), cancellable, error));
}

