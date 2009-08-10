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
GDataDocumentsEntry *upload_update_document (GDataDocumentsService *self, GDataDocumentsEntry *document, GFile *document_file,
					     const gchar *method, const gchar *upload_uri, GCancellable *cancellable, GError **error);

struct _GDataDocumentsServicePrivate {
	GDataService *spreadsheet_service;
};

enum {
	PROP_SPREADSHEET_SERVICE = 1
};

G_DEFINE_TYPE (GDataDocumentsService, gdata_documents_service, GDATA_TYPE_SERVICE)
#define GDATA_DOCUMENTS_SERVICE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDATA_TYPE_DOCUMENTS_SERVICE, GDataDocumentsServicePrivate))

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

static void
gdata_documents_service_dispose (GObject *object)
{
	GDataDocumentsServicePrivate *priv = GDATA_DOCUMENTS_SERVICE_GET_PRIVATE (object);

	if (priv->spreadsheet_service != NULL)
		g_object_unref (priv->spreadsheet_service);
	priv->spreadsheet_service = NULL;

	G_OBJECT_CLASS (gdata_documents_service_parent_class)->dispose (object);
}

static void
gdata_documents_service_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataDocumentsServicePrivate *priv = GDATA_DOCUMENTS_SERVICE_GET_PRIVATE (object);

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
 * gdata_documents_service_query_documents:
 * @self: a #GDataDocumentsService
 * @query: a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service to return a list of documents matching the given @query.
 *
 * For more details, see gdata_service_query().
 *
 * Return value: a #GDataDocumentsFeed of query results; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataDocumentsFeed *
gdata_documents_service_query_documents (GDataDocumentsService *self, GDataDocumentsQuery *query, GCancellable *cancellable,
					 GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
					 GError **error)
{
	/* Ensure we're authenticated first */
	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
				     _("You must be authenticated to query documents."));
		return NULL;
	}

	/* If we want to query for documents contained in a folder, the URI is different */
	if (query != NULL && gdata_documents_query_get_folder_id (query) != NULL) {
		return GDATA_DOCUMENTS_FEED (gdata_service_query (GDATA_SERVICE (self), "http://docs.google.com/feeds/folders/private/full",
								  GDATA_QUERY (query), GDATA_TYPE_DOCUMENTS_ENTRY, cancellable, progress_callback,
								  progress_user_data, error));
	}

	return GDATA_DOCUMENTS_FEED (gdata_service_query (GDATA_SERVICE (self), "http://docs.google.com/feeds/documents/private/full",
							  GDATA_QUERY (query), GDATA_TYPE_DOCUMENTS_ENTRY, cancellable, progress_callback,
							  progress_user_data, error));
}

/**
 * gdata_documents_service_query_single_document:
 * @self: a #GDataDocumentsService
 * @document_type: the expected #GType of the queried entry
 * @document_id: the document ID of the queried document
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Retrieves information about a single document with the given document ID.
 *
 * @document_type should be the expected type of the document to be returned. e.g. %GDATA_TYPE_DOCUMENTS_SPREADSHEET if you're querying
 * for a spreadsheet.
 *
 * @document_id should be the ID of the document as returned by gdata_document_entry_get_document_id().
 *
 * Parameters and errors are as for gdata_service_query().
 *
 * Return value: a #GDataDocumentsEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.5.0
 **/
GDataDocumentsEntry *
gdata_documents_service_query_single_document (GDataDocumentsService *self, GType document_type, const gchar *document_id,
					       GCancellable *cancellable, GError **error)
{
	GDataDocumentsEntry *document;
	SoupMessage *message;
	GDataDocumentsQuery *query;
	gchar *resource_id;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (document_id != NULL, NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (document_type == GDATA_TYPE_DOCUMENTS_FOLDER)
		resource_id = g_strconcat ("folder:", document_id, NULL);
	else if (document_type == GDATA_TYPE_DOCUMENTS_SPREADSHEET)
		resource_id = g_strconcat ("spreasheet:", document_id, NULL);
	else if (document_type == GDATA_TYPE_DOCUMENTS_TEXT)
		resource_id = g_strconcat ("document:", document_id, NULL);
	else if (document_type == GDATA_TYPE_DOCUMENTS_PRESENTATION)
		resource_id = g_strconcat ("presentation:", document_id, NULL);
	else
		g_assert_not_reached ();

	query = gdata_documents_query_new (NULL);
	gdata_query_set_entry_id (GDATA_QUERY (query), resource_id);
	g_free (resource_id);

	message = _gdata_service_query (GDATA_SERVICE (self), "http://docs.google.com/feeds/documents/private/full", GDATA_QUERY (query),
					cancellable, NULL, NULL, error);
	g_object_unref (query);

	if (message == NULL)
		return NULL;

	g_assert (message->response_body->data != NULL);
	document = GDATA_DOCUMENTS_ENTRY (gdata_parsable_new_from_xml (document_type, message->response_body->data,
								       message->response_body->length, error));
	g_object_unref (message);

	return document;
}

/**
 * gdata_documents_service_query_documents_async:
 * @self: a #GDataDocumentsService
 * @query: a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: data to pass to the @progress_callback function
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: data to pass to the @callback function
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
	/* Ensure we're authenticated first */
	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_simple_async_report_error_in_idle (G_OBJECT (self), callback, user_data,
						     GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
						     _("You must be authenticated to query documents."));
		return;
	}

	gdata_service_query_async (GDATA_SERVICE (self), "http://docs.google.com/feeds/documents/private/full", GDATA_QUERY (query),
				   GDATA_TYPE_DOCUMENTS_ENTRY, cancellable, progress_callback, progress_user_data, callback, user_data);
}

/*
 * To upload spreasheet documents, another token is needed since the service for it is "wise" as apposed to "writely" for other operations.
 * This callback aims to authenticate to this service as a private property (@priv->spreadsheet_service) of #GDataDocumentsService.
 */
static void
notify_authenticated_cb (GObject *service, GParamSpec *pspec, GObject *self)
{
	GDataService *spreadsheet_service;
	GDataDocumentsServicePrivate *priv = GDATA_DOCUMENTS_SERVICE_GET_PRIVATE (GDATA_DOCUMENTS_SERVICE (service));

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

GDataDocumentsEntry *
upload_update_document (GDataDocumentsService *self, GDataDocumentsEntry *document, GFile *document_file, const gchar *method,
			const gchar *upload_uri, GCancellable *cancellable, GError **error)
{
	GDataDocumentsEntry *new_entry;
	GOutputStream *output_stream;
	GInputStream *input_stream;
	const gchar *slug = NULL, *content_type = NULL, *response_body;
	gssize response_length;
	GFileInfo *file_info = NULL;
	GType new_document_type;

	/* Get some information about the file we're uploading */
	if (document_file != NULL) {
		/* Get the slug and content type */
		file_info = g_file_query_info (document_file, "standard::display-name,standard::content-type", G_FILE_QUERY_INFO_NONE, NULL, error);
		if (file_info == NULL)
			return NULL;

		slug = g_file_info_get_display_name (file_info);
		content_type = g_file_info_get_content_type (file_info);

		/* Corrects a bug on spreadsheet content types handling
		 * The content type for ODF spreasheets is "application/vnd.oasis.opendocument.spreadsheet" for my ODF spreadsheet;
		 * but Google Documents' spreadsheet service is waiting for "application/x-vnd.oasis.opendocument.spreadsheet"
		 * and nothing else.
		 * Bug filed with Google: http://code.google.com/p/gdata-issues/issues/detail?id=1127 */
		if (strcmp (content_type, "application/vnd.oasis.opendocument.spreadsheet") == 0)
			content_type = "application/x-vnd.oasis.opendocument.spreadsheet";
	}

	/* Determine the type of the document we're uploading */
	if (document == NULL) {
		/* We get the document type of the document which is being uploaded */
		if (strcmp (content_type, "application/x-vnd.oasis.opendocument.spreadsheet") == 0 ||
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
		} else {
			g_set_error_literal (error, GDATA_DOCUMENTS_SERVICE_ERROR, GDATA_DOCUMENTS_SERVICE_ERROR_INVALID_CONTENT_TYPE,
					     _("The supplied document had an invalid content type."));
			if (file_info != NULL)
				g_object_unref (file_info);
			return NULL;
		}
	} else {
		new_document_type = G_OBJECT_TYPE (document);
	}

	/* We need streaming file I/O: GDataUploadStream */
	output_stream = gdata_upload_stream_new (GDATA_SERVICE (self), method, upload_uri, GDATA_ENTRY (document), slug, content_type);

	if (file_info != NULL)
		g_object_unref (file_info);
	if (output_stream == NULL)
		return NULL;

	/* Open the document file for reading and pipe it to the upload stream */
	input_stream = G_INPUT_STREAM (g_file_read (document_file, cancellable, error));
	if (input_stream == NULL) {
		g_object_unref (output_stream);
		return NULL;
	}

	g_output_stream_splice (output_stream, input_stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
				cancellable, error);

	g_object_unref (input_stream);
	if (error != NULL && *error != NULL) {
		g_object_unref (output_stream);
		return NULL;
	}

	/* Get and parse the response from the server */
	response_body = gdata_upload_stream_get_response (GDATA_UPLOAD_STREAM (output_stream), &response_length);
	g_assert (response_body != NULL && response_length > 0);

	new_entry = GDATA_DOCUMENTS_ENTRY (gdata_parsable_new_from_xml (new_document_type, response_body, (gint) response_length, error));
	g_object_unref (output_stream);

	return new_entry;
}

/**
 * gdata_documents_service_upload_document:
 * @self: an authenticated #GDataDocumentsService
 * @document: the #GDataDocumentsEntry to insert, or %NULL
 * @document_file: the document to upload, or %NULL
 * @folder: the folder to which the document should be uploaded, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Uploads a document to Google Documents, using the properties from @document and the document file pointed to by @document_file.
 *
 * If @document is %NULL, only the document file will be uploaded. The new document entry will be named after the document file's name,
 * and will have default metadata.
 *
 * If @document_file is %NULL, only the document metadata will be uploaded. A blank document file will be created with the name
 * <literal>new document</literal> and the specified metadata. @document and @document_file cannot both be %NULL, but can both have values.
 *
 * The updated @document_entry will be returned on success, containing updated metadata.
 *
 * If there is a problem reading @document_file, an error from g_file_load_contents() or g_file_query_info() will be returned. Other errors from
 * #GDataServiceError can be returned for other exceptional conditions, as determined by the server.
 *
 * Return value: an updated #GDataDocumentsEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataDocumentsEntry *
gdata_documents_service_upload_document (GDataDocumentsService *self, GDataDocumentsEntry *document, GFile *document_file, GDataDocumentsFolder *folder,
					 GCancellable *cancellable, GError **error)
{
	GDataDocumentsEntry *new_document;
	gchar *upload_uri;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (document == NULL || GDATA_IS_DOCUMENTS_ENTRY (document), NULL);
	g_return_val_if_fail (document_file == NULL || G_IS_FILE (document_file), NULL);
	g_return_val_if_fail (document != NULL || document_file != NULL, NULL);
	g_return_val_if_fail (folder == NULL || GDATA_IS_DOCUMENTS_FOLDER (folder), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);

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

	if (document_file == NULL) {
		new_document = GDATA_DOCUMENTS_ENTRY (gdata_service_insert_entry (GDATA_SERVICE (self), upload_uri, GDATA_ENTRY (document),
										  cancellable, error));
	} else {
		new_document = upload_update_document (self, document, document_file, SOUP_METHOD_POST, upload_uri, cancellable, error);
	}
	g_free (upload_uri);

	return new_document;
}

/**
 * gdata_documents_service_update_document:
 * @self: a #GDataDocumentsService
 * @document: the #GDataDocumentsEntry to update
 * @document_file: the local document file containing the new data, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Update the document using the properties from @document and the document file pointed to by @document_file. If the document file does not
 * need to be changed, @document_file can be %NULL.
 *
 * If there is a problem reading @document_file, an error from g_file_load_contents() or g_file_query_info() will be returned. Other errors from
 * #GDataServiceError can be returned for other exceptional conditions, as determined by the server.
 *
 * For more details, see gdata_service_insert_entry().
 *
 * Return value: an updated #GDataDocumentsEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataDocumentsEntry *
gdata_documents_service_update_document (GDataDocumentsService *self, GDataDocumentsEntry *document, GFile *document_file,
					 GCancellable *cancellable, GError **error)
{
	GDataLink *update_link;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (document), NULL);
	g_return_val_if_fail (document_file == NULL || G_IS_FILE (document_file), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);

	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
				     _("You must be authenticated to update documents."));
		return NULL;
	}

	if (document_file == NULL)
		return GDATA_DOCUMENTS_ENTRY (gdata_service_update_entry (GDATA_SERVICE (self), GDATA_ENTRY (document), cancellable, error));

	update_link = gdata_entry_look_up_link (GDATA_ENTRY (document), GDATA_LINK_EDIT_MEDIA);
	g_assert (update_link != NULL);

	return upload_update_document (self, document, document_file, SOUP_METHOD_PUT, gdata_link_get_uri (update_link), cancellable, error);
}

/**
 * gdata_documents_service_move_document_to_folder:
 * @self: an authenticated #GDataDocumentsService
 * @document: the #GDataDocumentsEntry to move
 * @folder: the #GDataDocumentsFolder to move @document into
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Move the given @document to the specified @folder. If the document is already in another folder, it will be added to
 * the new folder, but will also remain in any previous folders.
 *
 * Errors from #GDataServiceError can be returned for other exceptional conditions, as determined by the server.
 *
 * Return value: an updated #GDataDocumentsEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataDocumentsEntry *
gdata_documents_service_move_document_to_folder (GDataDocumentsService *self, GDataDocumentsEntry *document, GDataDocumentsFolder *folder,
						 GCancellable *cancellable, GError **error)
{
	GDataServiceClass *klass;
	GDataDocumentsEntry *new_document;
	gchar *uri, *entry_xml, *upload_data;
	const gchar *folder_id;
	SoupMessage *message;
	guint status;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (document), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_FOLDER (folder), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);

	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
				     _("You must be authenticated to move documents."));
		return NULL;
	}

	folder_id = gdata_documents_entry_get_document_id (GDATA_DOCUMENTS_ENTRY (folder));
	g_assert (folder_id != NULL);
	uri = g_strconcat ("http://docs.google.com/feeds/folders/private/full/folder%3A", folder_id, NULL);

	message = soup_message_new (SOUP_METHOD_POST, uri);
	g_free (uri);

	/* Make sure subclasses set their headers */
	klass = GDATA_SERVICE_GET_CLASS (self);
	if (klass->append_query_headers != NULL)
		klass->append_query_headers (GDATA_SERVICE (self), message);

	/* Get the XML content */
	entry_xml = gdata_parsable_get_xml (GDATA_PARSABLE (document));

	/* Check for cancellation */
	if (g_cancellable_set_error_if_cancelled (cancellable, error) == TRUE) {
		g_object_unref (message);
		g_free (entry_xml);
		return NULL;
	}

	upload_data = g_strconcat ("<?xml version='1.0' encoding='UTF-8'?>", entry_xml, NULL);
	g_free (entry_xml);
	soup_message_set_request (message, "application/atom+xml", SOUP_MEMORY_TAKE, upload_data, strlen (upload_data));

	/* Send the message */
	status = _gdata_service_send_message (GDATA_SERVICE (self), message, error);
	if (status == SOUP_STATUS_NONE) {
		g_object_unref (message);
		return NULL;
	}

	/* Check for cancellation */
	if (g_cancellable_set_error_if_cancelled (cancellable, error) == TRUE) {
		g_object_unref (message);
		return NULL;
	}

	if (status != 201) {
		/* Error */
		g_assert (klass->parse_error_response != NULL);
		klass->parse_error_response (GDATA_SERVICE (self), GDATA_SERVICE_ERROR_WITH_INSERTION, status, message->reason_phrase, message->response_body->data,
					     message->response_body->length, error);
		g_object_unref (message);
		return NULL;
	}

	/* Build the updated entry */
	g_assert (message->response_body->data != NULL);

	/* Parse the XML; and update the document*/
	new_document = GDATA_DOCUMENTS_ENTRY (gdata_parsable_new_from_xml (G_OBJECT_TYPE (document), message->response_body->data,
									   message->response_body->length, error));
	g_object_unref (message);

	return new_document;
}

/**
 * gdata_documents_service_remove_document_from_folder:
 * @self: a #GDataDocumentsService
 * @document : the #GDataDocumentsEntry to remove
 * @folder : the #GDataDocumentsFolder from wich we should remove @document
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Remove the #GDataDocumentsEntry @document from the GDataDocumentsFolder @folder, and updates the document entry @document.
 *
 * Errors from #GDataServiceError can be returned for other exceptional conditions, as determined by the server.
 *
 * Return value: an updated #GDataDocumentsEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataDocumentsEntry *
gdata_documents_service_remove_document_from_folder (GDataDocumentsService *self, GDataDocumentsEntry *document, GDataDocumentsFolder *folder,
						     GCancellable *cancellable, GError **error)
{
	GDataServiceClass *klass;
	SoupMessage *message;
	guint status;
	GDataLink *link;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (document), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_FOLDER (folder), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);

	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
				     _("You must be authenticated to move documents."));
		return NULL;
	}

	/* Build the message */
	link = gdata_entry_look_up_link (GDATA_ENTRY (document), GDATA_LINK_EDIT);
	message = soup_message_new (SOUP_METHOD_DELETE, gdata_link_get_uri (link));

	/* Make sure subclasses set their headers */
	klass = GDATA_SERVICE_GET_CLASS (self);
	if (klass->append_query_headers != NULL)
		klass->append_query_headers (GDATA_SERVICE (self), message);

	soup_message_headers_append (message->request_headers, "If-Match", gdata_entry_get_etag (GDATA_ENTRY (document)));

	/* Send the message */
	status = _gdata_service_send_message (GDATA_SERVICE (self), message, error);
	if (status == SOUP_STATUS_NONE) {
		g_object_unref (message);
		return NULL;
	}

	/* Check for cancellation */
	if (g_cancellable_set_error_if_cancelled (cancellable, error) == TRUE) {
		g_object_unref (message);
		return NULL;
	}

	if (status != 200) {
		/* Error */
		g_assert (klass->parse_error_response != NULL);
		klass->parse_error_response (GDATA_SERVICE (self), GDATA_SERVICE_ERROR_WITH_INSERTION, status, message->reason_phrase, message->response_body->data,
					     message->response_body->length, error);
		g_object_unref (message);
		return NULL;
	}

	g_object_unref (message);

	/* Google's servers don't return an updated copy of the entry, so we have to query for it again.
	 * See: http://code.google.com/p/gdata-issues/issues/detail?id=1380 */
	return gdata_documents_service_query_single_document (self, G_OBJECT_TYPE (document), gdata_documents_entry_get_document_id (document),
							      cancellable, error);
}

/**
 * gdata_documents_service_get_upload_uri:
 * @folder: the #GDataDocumentsFolder into which to upload the document, or %NULL
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
		return g_strconcat ("http://docs.google.com/feeds/folders/private/full/folder%3A", folder_id, NULL);
	}

	/* Otherwise return the default upload URI */
	return g_strdup ("http://docs.google.com/feeds/documents/private/full");
}

GDataService *
_gdata_documents_service_get_spreadsheet_service (GDataDocumentsService *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	return self->priv->spreadsheet_service;
}
