/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Thibault Saunier 2009 <saunierthibault@gmail.com>
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

/**
 * SECTION:gdata-documents-service
 * @short_description: GData Documents service object
 * @stability: Unstable
 * @include: gdata/services/documents/gdata-documents-service.h
 *
 * #GDataDocumentsService is a subclass of #GDataService for communicating with the GData API of Google Documents. It supports querying
 * for, inserting, editing and deleting documents, as well as a folder hierarchy.
 *
 * For more details of Google Documents' GData API, see the <ulink type="http" url="http://code.google.com/apis/document/docs/2.0/reference.html">
 * online documentation</ulink>.
 *
 * Fore more details about the spreadsheet downloads handling, see the
 * <ulink type="http" url="http://groups.google.com/group/Google-Docs-Data-APIs/browse_thread/thread/bfc50e94e303a29a?pli=1">
 * online explanation about the problem</ulink>.
 *
 * Since: 0.4.0
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libsoup/soup.h>
#include <string.h>

#include "gdata-documents-service.h"
#include "gdata-documents-spreadsheet.h"
#include "gdata-documents-text.h"
#include "gdata-documents-presentation.h"
#include "gdata-batchable.h"
#include "gdata-service.h"
#include "gdata-private.h"
#include "gdata-upload-stream.h"

GQuark
gdata_documents_service_error_quark (void)
{
	return g_quark_from_static_string ("gdata-documents-service-error-quark");
}

static void gdata_documents_service_dispose (GObject *object);
static void gdata_documents_service_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void notify_authenticated_cb (GObject *service, GParamSpec *pspec, GObject *self);
static void notify_proxy_uri_cb (GObject *service, GParamSpec *pspec, GObject *self);

struct _GDataDocumentsServicePrivate {
	GDataService *spreadsheet_service;
};

enum {
	PROP_SPREADSHEET_SERVICE = 1
};

G_DEFINE_TYPE_WITH_CODE (GDataDocumentsService, gdata_documents_service, GDATA_TYPE_SERVICE, G_IMPLEMENT_INTERFACE (GDATA_TYPE_BATCHABLE, NULL))

static void
gdata_documents_service_class_init (GDataDocumentsServiceClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataServiceClass *service_class = GDATA_SERVICE_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataDocumentsServicePrivate));

	gobject_class->get_property = gdata_documents_service_get_property;
	gobject_class->dispose = gdata_documents_service_dispose;

	service_class->service_name = "writely";
	service_class->feed_type = GDATA_TYPE_DOCUMENTS_FEED;

	/**
	 * GDataService:spreadsheet-service:
	 *
	 * Another service for spreadsheets, required to be able to handle downloads.
	 *
	 * For more details about the spreadsheet downloads handling, see the
	 * <ulink type="http" url="http://groups.google.com/group/Google-Docs-Data-APIs/browse_thread/thread/bfc50e94e303a29a?pli=1">
	 * online explanation about the problem</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_SPREADSHEET_SERVICE,
	                                 g_param_spec_object ("spreadsheet-service",
	                                                      "Spreadsheet service", "Another service for spreadsheets.",
	                                                      GDATA_TYPE_SERVICE,
	                                                      G_PARAM_READABLE));
}

static void
gdata_documents_service_init (GDataDocumentsService *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_DOCUMENTS_SERVICE, GDataDocumentsServicePrivate);
	g_signal_connect (self, "notify::authenticated", G_CALLBACK (notify_authenticated_cb), NULL);
	g_signal_connect (self, "notify::proxy-uri", G_CALLBACK (notify_proxy_uri_cb), NULL);
}

static void
gdata_documents_service_dispose (GObject *object)
{
	GDataDocumentsServicePrivate *priv = GDATA_DOCUMENTS_SERVICE (object)->priv;

	if (priv->spreadsheet_service != NULL)
		g_object_unref (priv->spreadsheet_service);
	priv->spreadsheet_service = NULL;

	G_OBJECT_CLASS (gdata_documents_service_parent_class)->dispose (object);
}

static void
gdata_documents_service_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataDocumentsServicePrivate *priv = GDATA_DOCUMENTS_SERVICE (object)->priv;

	switch (property_id) {
		case PROP_SPREADSHEET_SERVICE:
			g_value_set_object (value, priv->spreadsheet_service);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

/**
 * gdata_documents_service_new:
 * @client_id: your application's client ID
 *
 * Creates a new #GDataDocumentsService. The @client_id must be unique for your application, and as registered with Google.
 *
 * Return value: a new #GDataDocumentsService, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataDocumentsService *
gdata_documents_service_new (const gchar *client_id)
{
	g_return_val_if_fail (client_id != NULL, NULL);

	return g_object_new (GDATA_TYPE_DOCUMENTS_SERVICE,
	                     "client-id", client_id,
	                     NULL);
}

/**
 * gdata_documents_service_query_documents:
 * @self: a #GDataDocumentsService
 * @query: (allow-none): a #GDataDocumentsQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: (scope call): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service to return a list of documents matching the given @query. Note that @query has to be a #GDataDocumentsQuery, rather than just
 * a #GDataQuery, as it uses the folder ID specified in #GDataDocumentsQuery:folder-id.
 *
 * For more details, see gdata_service_query().
 *
 * Return value: (transfer full): a #GDataDocumentsFeed of query results; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataDocumentsFeed *
gdata_documents_service_query_documents (GDataDocumentsService *self, GDataDocumentsQuery *query, GCancellable *cancellable,
                                         GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                         GError **error)
{
	GDataFeed *feed;
	gchar *request_uri;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_DOCUMENTS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Ensure we're authenticated first */
	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to query documents."));
		return NULL;
	}

	/* If we want to query for documents contained in a folder, the URI is different */
	if (query != NULL && gdata_documents_query_get_folder_id (query) != NULL)
		request_uri = g_strconcat (_gdata_service_get_scheme (), "://docs.google.com/feeds/folders/private/full", NULL);
	else
		request_uri = g_strconcat (_gdata_service_get_scheme (), "://docs.google.com/feeds/documents/private/full", NULL);

	feed = gdata_service_query (GDATA_SERVICE (self), request_uri, GDATA_QUERY (query), GDATA_TYPE_DOCUMENTS_ENTRY, cancellable,
	                            progress_callback, progress_user_data, error);
	g_free (request_uri);

	return GDATA_DOCUMENTS_FEED (feed);
}

/**
 * gdata_documents_service_query_documents_async: (skip)
 * @self: a #GDataDocumentsService
 * @query: (allow-none): a #GDataDocumentsQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: data to pass to the @progress_callback function
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Queries the service to return a list of documents matching the given @query. @self and
 * @query are both reffed when this function is called, so can safely be unreffed after this function returns.
 *
 * For more details, see gdata_documents_service_query_documents(), which is the synchronous version of this function,
 * and gdata_service_query_async(), which is the base asynchronous query function.
 *
 * Since: 0.4.0
 **/
void
gdata_documents_service_query_documents_async (GDataDocumentsService *self, GDataDocumentsQuery *query, GCancellable *cancellable,
                                               GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                               GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *request_uri;

	g_return_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self));
	g_return_if_fail (query == NULL || GDATA_IS_DOCUMENTS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	/* Ensure we're authenticated first */
	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_simple_async_report_error_in_idle (G_OBJECT (self), callback, user_data,
		                                     GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                                     _("You must be authenticated to query documents."));
		return;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://docs.google.com/feeds/documents/private/full", NULL);
	gdata_service_query_async (GDATA_SERVICE (self), request_uri, GDATA_QUERY (query), GDATA_TYPE_DOCUMENTS_ENTRY,
	                           cancellable, progress_callback, progress_user_data, callback, user_data);
	g_free (request_uri);
}

/*
 * To upload spreadsheet documents, another token is needed since the service for it is "wise" as apposed to "writely" for other operations.
 * This callback aims to authenticate to this service as a private property (@priv->spreadsheet_service) of #GDataDocumentsService.
 */
static void
notify_authenticated_cb (GObject *service, GParamSpec *pspec, GObject *self)
{
	GDataService *spreadsheet_service;
	GDataDocumentsServicePrivate *priv = GDATA_DOCUMENTS_SERVICE (service)->priv;

	if (priv->spreadsheet_service != NULL)
		g_object_unref (priv->spreadsheet_service);

	spreadsheet_service = g_object_new (GDATA_TYPE_SERVICE, "client-id", gdata_service_get_client_id (GDATA_SERVICE (service)), NULL);
	GDATA_SERVICE_GET_CLASS (spreadsheet_service)->service_name = "wise";
	gdata_service_authenticate (spreadsheet_service, gdata_service_get_username (GDATA_SERVICE (service)),
	                            gdata_service_get_password (GDATA_SERVICE (service)), NULL, NULL);
	priv->spreadsheet_service = spreadsheet_service;
}

/* Sets the proxy on @spreadsheet_service when it is set on the service */
static void
notify_proxy_uri_cb (GObject *service, GParamSpec *pspec, GObject *self)
{
	SoupURI *proxy_uri;

	if (GDATA_DOCUMENTS_SERVICE (self)->priv->spreadsheet_service == NULL)
		return;

	proxy_uri = gdata_service_get_proxy_uri (GDATA_SERVICE (service));
	gdata_service_set_proxy_uri (GDATA_DOCUMENTS_SERVICE (self)->priv->spreadsheet_service, proxy_uri);
}

static GDataUploadStream *
upload_update_document (GDataDocumentsService *self, GDataDocumentsDocument *document, const gchar *slug, const gchar *content_type,
                        const gchar *method, const gchar *upload_uri)
{
	/* Corrects a bug on spreadsheet content types handling
	 * The content type for ODF spreadsheets is "application/vnd.oasis.opendocument.spreadsheet" for my ODF spreadsheet;
	 * but Google Documents' spreadsheet service is waiting for "application/x-vnd.oasis.opendocument.spreadsheet"
	 * and nothing else.
	 * Bug filed with Google: http://code.google.com/p/gdata-issues/issues/detail?id=1127 */
	if (strcmp (content_type, "application/vnd.oasis.opendocument.spreadsheet") == 0)
		content_type = "application/x-vnd.oasis.opendocument.spreadsheet";

	/* We need streaming file I/O: GDataUploadStream */
	return GDATA_UPLOAD_STREAM (gdata_upload_stream_new (GDATA_SERVICE (self), method, upload_uri, GDATA_ENTRY (document), slug, content_type));
}

/**
 * gdata_documents_service_upload_document:
 * @self: an authenticated #GDataDocumentsService
 * @document: (allow-none): the #GDataDocumentsDocument to insert, or %NULL
 * @slug: the filename to give to the uploaded document
 * @content_type: the content type of the uploaded data
 * @folder: (allow-none): the folder to which the document should be uploaded, or %NULL
 * @error: a #GError, or %NULL
 *
 * Uploads a document to Google Documents, using the properties from @document and the document data written to the resulting #GDataUploadStream. If
 * the document data does not need to be provided at the moment, just the metadata, use gdata_service_insert_entry() instead (e.g. in the case of
 * creating a new, empty file to be edited at a later date).
 *
 * If @document is %NULL, only the document data will be uploaded. The new document entry will be named using @slug, and will have default metadata.
 *
 * The stream returned by this function should be written to using the standard #GOutputStream methods, asychronously or synchronously. Once the stream
 * is closed (using g_output_stream_close()), gdata_documents_service_finish_upload() should be called on it to parse and return the updated
 * #GDataDocumentsDocument for the document. This must be done, as @document isn't updated in-place.
 *
 * Any upload errors will be thrown by the stream methods, and may come from the #GDataServiceError domain.
 *
 * Return value: (transfer full): a #GDataUploadStream to write the document data to, or %NULL; unref with g_object_unref()
 *
 * Since: 0.8.0
 **/
GDataUploadStream *
gdata_documents_service_upload_document (GDataDocumentsService *self, GDataDocumentsDocument *document, const gchar *slug, const gchar *content_type,
                                         GDataDocumentsFolder *folder, GError **error)
{
	GDataUploadStream *upload_stream;
	gchar *upload_uri;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (document == NULL || GDATA_IS_DOCUMENTS_DOCUMENT (document), NULL);
	g_return_val_if_fail (slug != NULL && *slug != '\0', NULL);
	g_return_val_if_fail (content_type != NULL && *content_type != '\0', NULL);
	g_return_val_if_fail (folder == NULL || GDATA_IS_DOCUMENTS_FOLDER (folder), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to upload documents."));
		return NULL;
	}

	if (document != NULL && gdata_entry_is_inserted (GDATA_ENTRY (document)) == TRUE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED,
		                     _("The document has already been uploaded."));
		return NULL;
	}

	upload_uri = gdata_documents_service_get_upload_uri (folder);
	upload_stream = upload_update_document (self, document, slug, content_type, SOUP_METHOD_POST, upload_uri);
	g_free (upload_uri);

	return upload_stream;
}

/**
 * gdata_documents_service_update_document:
 * @self: a #GDataDocumentsService
 * @document: the #GDataDocumentsDocument to update
 * @slug: the filename to give to the uploaded document
 * @content_type: the content type of the uploaded data
 * @error: a #GError, or %NULL
 *
 * Update the document using the properties from @document and the document data written to the resulting #GDataUploadStream. If the document data does
 * not need to be changed, just the metadata, use gdata_service_update_entry() instead.
 *
 * The stream returned by this function should be written to using the standard #GOutputStream methods, asychronously or synchronously. Once the stream
 * is closed (using g_output_stream_close()), gdata_documents_service_finish_upload() should be called on it to parse and return the updated
 * #GDataDocumentsDocument for the document. This must be done, as @document isn't updated in-place.
 *
 * Any upload errors will be thrown by the stream methods, and may come from the #GDataServiceError domain.
 *
 * For more information, see gdata_service_update_entry().
 *
 * Return value: (transfer full): a #GDataUploadStream to write the document data to; unref with g_object_unref()
 *
 * Since: 0.8.0
 **/
GDataUploadStream *
gdata_documents_service_update_document (GDataDocumentsService *self, GDataDocumentsDocument *document, const gchar *slug, const gchar *content_type,
                                         GError **error)
{
	GDataLink *update_link;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_DOCUMENT (document), NULL);
	g_return_val_if_fail (slug != NULL && *slug != '\0', NULL);
	g_return_val_if_fail (content_type != NULL && *content_type != '\0', NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to update documents."));
		return NULL;
	}

	update_link = gdata_entry_look_up_link (GDATA_ENTRY (document), GDATA_LINK_EDIT_MEDIA);
	g_assert (update_link != NULL);

	return upload_update_document (self, document, slug, content_type, SOUP_METHOD_PUT, gdata_link_get_uri (update_link));
}

/**
 * gdata_documents_service_finish_upload:
 * @self: a #GDataDocumentsService
 * @upload_stream: the #GDataUploadStream from the operation
 * @error: a #GError, or %NULL
 *
 * Finish off a document upload or update operation started by gdata_documents_service_upload_document() or gdata_documents_service_update_document(),
 * parsing the result and returning the new or updated #GDataDocumentsDocument.
 *
 * If an error occurred during the upload or update operation, it will have been returned during the operation (e.g. by g_output_stream_splice() or one
 * of the other stream methods). In such a case, %NULL will be returned but @error will remain unset. @error is only set in the case that the server
 * indicates that the operation was successful, but an error is encountered in parsing the result sent by the server.
 *
 * In the case that no #GDataDocumentsDocument was passed (to gdata_documents_service_upload_document() or gdata_documents_service_update_document())
 * when starting the operation, %GDATA_DOCUMENTS_SERVICE_ERROR_INVALID_CONTENT_TYPE will be thrown in @error if the content type of the uploaded data
 * could not be mapped to a document type with which to interpret the response from the server.
 *
 * Return value: (transfer full): the new or updated #GDataDocumentsDocument, or %NULL; unref with g_object_unref()
 *
 * Since: 0.8.0
 */
GDataDocumentsDocument *
gdata_documents_service_finish_upload (GDataDocumentsService *self, GDataUploadStream *upload_stream, GError **error)
{
	const gchar *response_body, *content_type;
	gssize response_length;
	GDataEntry *entry;
	GType new_document_type = G_TYPE_INVALID;

	/* Determine the type of the document we've uploaded */
	entry = gdata_upload_stream_get_entry (upload_stream);
	content_type = gdata_upload_stream_get_content_type (upload_stream);

	if (entry != NULL) {
		new_document_type = G_OBJECT_TYPE (entry);
	} else if (strcmp (content_type, "application/x-vnd.oasis.opendocument.spreadsheet") == 0 ||
	           strcmp (content_type, "text/tab-separated-values") == 0 ||
	           strcmp (content_type, "application/x-vnd.oasis.opendocument.spreadsheet") == 0 ||
	           strcmp (content_type, "application/vnd.ms-excel") == 0) {
		new_document_type = GDATA_TYPE_DOCUMENTS_SPREADSHEET;
	} else if (strcmp (content_type, "application/msword") == 0 ||
	           strcmp (content_type, "application/vnd.oasis.opendocument.text") == 0 ||
	           strcmp (content_type, "application/rtf") == 0 ||
	           strcmp (content_type, "text/html") == 0 ||
	           strcmp (content_type, "application/vnd.sun.xml.writer") == 0 ||
	           strcmp (content_type, "text/plain") == 0) {
		new_document_type = GDATA_TYPE_DOCUMENTS_TEXT;
	} else if (strcmp (content_type, "application/vnd.ms-powerpoint") == 0) {
		new_document_type = GDATA_TYPE_DOCUMENTS_PRESENTATION;
	}

	if (g_type_is_a (new_document_type, GDATA_TYPE_DOCUMENTS_DOCUMENT) == FALSE) {
		g_set_error (error, GDATA_DOCUMENTS_SERVICE_ERROR, GDATA_DOCUMENTS_SERVICE_ERROR_INVALID_CONTENT_TYPE,
		             _("The content type of the supplied document ('%s') could not be recognized."), content_type);
		return NULL;
	}

	/* Get and parse the response from the server */
	response_body = gdata_upload_stream_get_response (upload_stream, &response_length);
	if (response_body == NULL || response_length == 0)
		return NULL;

	return GDATA_DOCUMENTS_DOCUMENT (gdata_parsable_new_from_xml (new_document_type, response_body, (gint) response_length, error));
}

/**
 * gdata_documents_service_add_entry_to_folder:
 * @self: an authenticated #GDataDocumentsService
 * @entry: the #GDataDocumentsEntry to move
 * @folder: the #GDataDocumentsFolder to move @entry into
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Add the given @entry to the specified @folder, and return an updated #GDataDocumentsEntry for @entry. If the @entry is already in another folder, it
 * will be added to the new folder, but will also remain  in its other folders. Note that @entry can be either a #GDataDocumentsDocument or a
 * #GDataDocumentsFolder.
 *
 * Errors from #GDataServiceError can be returned for exceptional conditions, as determined by the server.
 *
 * Return value: (transfer full): an updated #GDataDocumentsEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.8.0
 **/
GDataDocumentsEntry *
gdata_documents_service_add_entry_to_folder (GDataDocumentsService *self, GDataDocumentsEntry *entry, GDataDocumentsFolder *folder,
                                             GCancellable *cancellable, GError **error)
{
	GDataDocumentsEntry *new_entry;
	gchar *uri, *upload_data;
	const gchar *folder_id;
	SoupMessage *message;
	guint status;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (entry), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_FOLDER (folder), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to move documents and folders."));
		return NULL;
	}

	/* NOTE: adding a document to a folder doesn't have server-side ETag support (throws "noPostConcurrency" error) */
	folder_id = gdata_documents_entry_get_document_id (GDATA_DOCUMENTS_ENTRY (folder));
	g_assert (folder_id != NULL);
	uri = g_strconcat (_gdata_service_get_scheme (), "://docs.google.com/feeds/folders/private/full/folder%3A", folder_id, NULL);
	message = _gdata_service_build_message (GDATA_SERVICE (self), SOUP_METHOD_POST, uri, NULL, TRUE);
	g_free (uri);

	/* Append the data */
	upload_data = gdata_parsable_get_xml (GDATA_PARSABLE (entry));
	soup_message_set_request (message, "application/atom+xml", SOUP_MEMORY_TAKE, upload_data, strlen (upload_data));

	/* Send the message */
	status = _gdata_service_send_message (GDATA_SERVICE (self), message, cancellable, error);

	if (status == SOUP_STATUS_NONE || status == SOUP_STATUS_CANCELLED) {
		/* Redirect error or cancelled */
		g_object_unref (message);
		return NULL;
	} else if (status != SOUP_STATUS_CREATED) {
		/* Error */
		GDataServiceClass *klass = GDATA_SERVICE_GET_CLASS (self);
		g_assert (klass->parse_error_response != NULL);
		klass->parse_error_response (GDATA_SERVICE (self), GDATA_OPERATION_UPDATE, status, message->reason_phrase,
		                             message->response_body->data, message->response_body->length, error);
		g_object_unref (message);
		return NULL;
	}

	/* Parse the XML; and update the entry */
	g_assert (message->response_body->data != NULL);
	new_entry = GDATA_DOCUMENTS_ENTRY (gdata_parsable_new_from_xml (G_OBJECT_TYPE (entry), message->response_body->data,
	                                                                message->response_body->length, error));
	g_object_unref (message);

	return new_entry;
}

typedef struct {
	GDataDocumentsEntry *entry;
	GDataDocumentsFolder *folder;
} AddEntryToFolderData;

static void
add_entry_to_folder_data_free (AddEntryToFolderData *data)
{
	g_object_unref (data->entry);
	g_object_unref (data->folder);
	g_slice_free (AddEntryToFolderData, data);
}

static void
add_entry_to_folder_thread (GSimpleAsyncResult *result, GDataDocumentsService *service, GCancellable *cancellable)
{
	GDataDocumentsEntry *updated_entry;
	AddEntryToFolderData *data;
	GError *error = NULL;

	data = g_simple_async_result_get_op_res_gpointer (result);

	/* Add the entry to the folder and return */
	updated_entry = gdata_documents_service_add_entry_to_folder (service, data->entry, data->folder, cancellable, &error);
	if (error != NULL) {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
		return;
	}

	/* Return the updated entry */
	g_simple_async_result_set_op_res_gpointer (result, updated_entry, (GDestroyNotify) g_object_unref);
}

/**
 * gdata_documents_service_add_entry_to_folder_async:
 * @self: a #GDataDocumentsService
 * @entry: the #GDataDocumentsEntry to add to @folder
 * @folder: the #GDataDocumentsFolder to add @entry to
 * @cancellable: optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the operation is finished, or %NULL
 * @user_data: (closure): data to pass to the @callback function
 *
 * Add the given @entry to the specified @folder. @self, @entry and @folder are all reffed when this function is called, so can safely be unreffed
 * after this function returns.
 *
 * For more details, see gdata_documents_service_add_entry_to_folder(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_documents_service_add_entry_to_folder_finish() to get the results
 * of the operation.
 *
 * Since: 0.8.0
 **/
void
gdata_documents_service_add_entry_to_folder_async (GDataDocumentsService *self, GDataDocumentsEntry *entry, GDataDocumentsFolder *folder,
                                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	GSimpleAsyncResult *result;
	AddEntryToFolderData *data;

	g_return_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self));
	g_return_if_fail (GDATA_IS_DOCUMENTS_ENTRY (entry));
	g_return_if_fail (GDATA_IS_DOCUMENTS_FOLDER (folder));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	data = g_slice_new (AddEntryToFolderData);
	data->entry = g_object_ref (entry);
	data->folder = g_object_ref (folder);

	result = g_simple_async_result_new (G_OBJECT (self), callback, user_data, gdata_documents_service_add_entry_to_folder_async);
	g_simple_async_result_set_op_res_gpointer (result, data, (GDestroyNotify) add_entry_to_folder_data_free);
	g_simple_async_result_run_in_thread (result, (GSimpleAsyncThreadFunc) add_entry_to_folder_thread, G_PRIORITY_DEFAULT, cancellable);
	g_object_unref (result);
}

/**
 * gdata_documents_service_add_entry_to_folder_finish:
 * @self: a #GDataDocumentsService
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finish an asynchronous operation to add a #GDataDocumentsEntry to a folder started with gdata_documents_service_add_entry_to_folder_async().
 *
 * Return value: (transfer full): an updated #GDataDocumentsEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.8.0
 **/
GDataDocumentsEntry *
gdata_documents_service_add_entry_to_folder_finish (GDataDocumentsService *self, GAsyncResult *async_result, GError **error)
{
	GSimpleAsyncResult *result = G_SIMPLE_ASYNC_RESULT (async_result);
	GDataDocumentsEntry *entry;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_warn_if_fail (g_simple_async_result_get_source_tag (result) == gdata_documents_service_add_entry_to_folder_async);

	if (g_simple_async_result_propagate_error (result, error) == TRUE)
		return NULL;

	entry = g_simple_async_result_get_op_res_gpointer (result);
	if (entry != NULL)
		return g_object_ref (entry);

	g_assert_not_reached ();
}

/**
 * gdata_documents_service_remove_entry_from_folder:
 * @self: a #GDataDocumentsService
 * @entry: the #GDataDocumentsEntry to remove
 * @folder: the #GDataDocumentsFolder from which we should remove @entry
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Remove the given @entry from @folder, and return an updated #GDataDocumentsEntry for @entry. @entry will remain a member of any other folders it's
 * currently in. Note that @entry can be either a #GDataDocumentsDocument or a #GDataDocumentsFolder.
 *
 * Errors from #GDataServiceError can be returned for exceptional conditions, as determined by the server.
 *
 * Return value: (transfer full): an updated #GDataDocumentsEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.8.0
 **/
GDataDocumentsEntry *
gdata_documents_service_remove_entry_from_folder (GDataDocumentsService *self, GDataDocumentsEntry *entry, GDataDocumentsFolder *folder,
                                                  GCancellable *cancellable, GError **error)
{
	const gchar *folder_id, *entry_id;
	SoupMessage *message;
	guint status;
	gchar *uri;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (entry), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_FOLDER (folder), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to move documents and folders."));
		return NULL;
	}

	/* Get the document ID */
	folder_id = gdata_documents_entry_get_document_id (GDATA_DOCUMENTS_ENTRY (folder));
	entry_id = gdata_documents_entry_get_document_id (entry);
	g_assert (folder_id != NULL);
	g_assert (entry_id != NULL);

	if (GDATA_IS_DOCUMENTS_PRESENTATION (entry)) {
		uri = _gdata_service_build_uri (FALSE, "http://docs.google.com/feeds/folders/private/full/folder%%3A%s/presentation%%3A%s",
		                                folder_id, entry_id);
	} else if (GDATA_IS_DOCUMENTS_SPREADSHEET (entry)) {
		uri = _gdata_service_build_uri (FALSE, "http://docs.google.com/feeds/folders/private/full/folder%%3A%s/spreadsheet%%3A%s",
		                                folder_id, entry_id);
	} else if (GDATA_IS_DOCUMENTS_TEXT (entry)) {
		uri = _gdata_service_build_uri (FALSE, "http://docs.google.com/feeds/folders/private/full/folder%%3A%s/document%%3A%s",
		                                folder_id, entry_id);
	} else if (GDATA_IS_DOCUMENTS_FOLDER (entry)) {
		uri = _gdata_service_build_uri (FALSE, "http://docs.google.com/feeds/folders/private/full/folder%%3A%s/folder%%3A%s",
		                                folder_id, entry_id);
	} else {
		g_assert_not_reached ();
	}

	message = _gdata_service_build_message (GDATA_SERVICE (self), SOUP_METHOD_DELETE, uri, gdata_entry_get_etag (GDATA_ENTRY (entry)), TRUE);
	g_free (uri);

	/* Send the message */
	status = _gdata_service_send_message (GDATA_SERVICE (self), message, cancellable, error);

	if (status == SOUP_STATUS_NONE || status == SOUP_STATUS_CANCELLED) {
		/* Redirect error or cancelled */
		g_object_unref (message);
		return NULL;
	} else if (status != SOUP_STATUS_OK) {
		/* Error */
		GDataServiceClass *klass = GDATA_SERVICE_GET_CLASS (self);
		g_assert (klass->parse_error_response != NULL);
		klass->parse_error_response (GDATA_SERVICE (self), GDATA_OPERATION_UPDATE, status, message->reason_phrase,
		                             message->response_body->data, message->response_body->length, error);
		g_object_unref (message);
		return NULL;
	}

	g_object_unref (message);

	/* Google's servers don't return an updated copy of the entry, so we have to query for it again.
	 * See: http://code.google.com/p/gdata-issues/issues/detail?id=1380 */
	return GDATA_DOCUMENTS_ENTRY (gdata_service_query_single_entry (GDATA_SERVICE (self), gdata_entry_get_id (GDATA_ENTRY (entry)), NULL,
	                                                                G_OBJECT_TYPE (entry), cancellable, error));
}

/**
 * gdata_documents_service_get_upload_uri:
 * @folder: (allow-none): the #GDataDocumentsFolder into which to upload the document, or %NULL
 *
 * Gets the upload URI for documents for the service.
 *
 * If @folder is %NULL, the URI will be the one to upload documents to the "root" folder.
 *
 * Return value: the URI permitting the upload of documents to @folder, or %NULL; free with g_free()
 *
 * Since: 0.5.0
 **/
gchar *
gdata_documents_service_get_upload_uri (GDataDocumentsFolder *folder)
{
	g_return_val_if_fail (folder == NULL || GDATA_IS_DOCUMENTS_FOLDER (folder), NULL);

	/* If we have a folder, return the folder's upload URI */
	if (folder != NULL) {
		const gchar *folder_id = gdata_documents_entry_get_document_id (GDATA_DOCUMENTS_ENTRY (folder));
		g_assert (folder_id != NULL);
		return g_strconcat (_gdata_service_get_scheme (), "://docs.google.com/feeds/folders/private/full/folder%3A", folder_id, NULL);
	}

	/* Otherwise return the default upload URI */
	return g_strconcat (_gdata_service_get_scheme (), "://docs.google.com/feeds/documents/private/full", NULL);
}

GDataService *
_gdata_documents_service_get_spreadsheet_service (GDataDocumentsService *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	return self->priv->spreadsheet_service;
}
