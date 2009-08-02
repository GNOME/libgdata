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
					     SoupMessage *message, guint good_status_code,
					     GCancellable *cancellable, GError **error);

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

	return GDATA_DOCUMENTS_FEED (gdata_service_query (GDATA_SERVICE (self), "http://docs.google.com/feeds/documents/private/full", GDATA_QUERY (query),
				     GDATA_TYPE_DOCUMENTS_ENTRY, cancellable, progress_callback, progress_user_data, error));
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

/**
 * To upload spreasheet documents, another token is needed since the service for it is "wise" as apposed to "writely" for other operations.
 * This callback aims to authenticate to this service as a private property (@priv->spreadsheet_service) of #GDataDocumentsService.
 * */
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
upload_update_document (GDataDocumentsService *self, GDataDocumentsEntry *document, GFile *document_file, SoupMessage *message,
			guint good_status_code, GCancellable *cancellable, GError **error)
{
	/* TODO: Async variant */
	#define BOUNDARY_STRING "0003Z5W789RTE456KlemsnoZV"

	GDataServiceClass *klass;
	GType new_document_type = GDATA_TYPE_DOCUMENTS_ENTRY; /* If the document type is wrong we get an error from the server anyway */
	gchar *entry_xml, *second_chunk_header, *upload_data, *document_contents = NULL, *i;
	const gchar *first_chunk_header, *footer;
	GFileInfo *document_file_info = NULL;
	guint status;
	gsize content_length, first_chunk_header_length, second_chunk_header_length, entry_xml_length, document_length, footer_length;

	g_return_val_if_fail (document != NULL || document_file != NULL, NULL);
	g_return_val_if_fail (message != NULL, NULL);

	/* Make sure subclasses set their headers */
	klass = GDATA_SERVICE_GET_CLASS (self);
	if (klass->append_query_headers != NULL)
		klass->append_query_headers (GDATA_SERVICE (self), message);

	/* Gets document file information */
	if (document_file != NULL) {
		/* Get the data early so we can calculate the content length */
		if (g_file_load_contents (document_file, NULL, &document_contents, &document_length, NULL, error) == FALSE)
			return NULL;

		document_file_info = g_file_query_info (document_file, "standard::display-name,standard::content-type", G_FILE_QUERY_INFO_NONE, NULL, error);
		if (document_file_info == NULL) {
			g_free (document_contents);
			return NULL;
		}

		/* Check for cancellation */
		if (g_cancellable_set_error_if_cancelled (cancellable, error) == TRUE) {
			g_free (document_contents);
			g_object_unref (document_file_info);
			return NULL;
		}

		/* Add document-upload--specific headers */
		soup_message_headers_append (message->request_headers, "Slug", g_file_info_get_display_name (document_file_info));

		if (document == NULL) {
			const gchar *content_type = g_file_info_get_content_type (document_file_info);

			/* Corrects a bug on spreadsheet content types handling
			 * The content type for ODF spreasheets is "application/vnd.oasis.opendocument.spreadsheet" for my ODF spreadsheet;
			 * but Google Documents' spreadsheet service is waiting for "application/x-vnd.oasis.opendocument.spreadsheet"
			 * and nothing else.
			 * Bug filed with Google: http://code.google.com/p/gdata-issues/issues/detail?id=1127 */
			if (strcmp (content_type, "application/vnd.oasis.opendocument.spreadsheet") == 0)
				content_type = "application/x-vnd.oasis.opendocument.spreadsheet";
			soup_message_set_request (message, content_type, SOUP_MEMORY_TAKE, document_contents, document_length);

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
				g_free (document_contents);
				g_object_unref (document_file_info);
				return NULL;
			}
		}
	}

	/* Prepare XML file to upload metadata */
	if (document != NULL) {
		/* Test on the document entry */
		new_document_type = G_OBJECT_TYPE (document);

		/* Get the XML content */
		entry_xml = gdata_parsable_get_xml (GDATA_PARSABLE (document));

		/* Check for cancellation */
		if (g_cancellable_set_error_if_cancelled (cancellable, error) == TRUE) {
			g_free (entry_xml);
			g_free (document_contents);
			if (document_file_info != NULL)
				g_object_unref (document_file_info);
			return NULL;
		}

		/* Prepare the data to upload containg both metadata and document content */
		if (document_file != NULL) {
			const gchar *content_type = g_file_info_get_content_type (document_file_info);

			first_chunk_header = "--" BOUNDARY_STRING "\nContent-Type: application/atom+xml; charset=UTF-8\n\n<?xml version='1.0'?>";
			/* Corrects a bug on spreadsheet content types handling (see above) */
			if (strcmp (content_type, "application/vnd.oasis.opendocument.spreadsheet") == 0)
				content_type = "application/x-vnd.oasis.opendocument.spreadsheet";

			second_chunk_header = g_strdup_printf ("\n--" BOUNDARY_STRING "\nContent-Type: %s\n\n", content_type);
			footer = "\n--" BOUNDARY_STRING "--";

			first_chunk_header_length = strlen (first_chunk_header);
			second_chunk_header_length = strlen (second_chunk_header);
			entry_xml_length = strlen (entry_xml);
			footer_length = strlen (footer);
			content_length = first_chunk_header_length + entry_xml_length + second_chunk_header_length + document_length + footer_length;

			/* Build the upload data */
			upload_data = i = g_malloc (content_length);

			memcpy (upload_data, first_chunk_header, first_chunk_header_length);
			i += first_chunk_header_length;

			memcpy (i, entry_xml, entry_xml_length);
			i += entry_xml_length;
			g_free (entry_xml);

			memcpy (i, second_chunk_header, second_chunk_header_length);
			g_free (second_chunk_header);
			i += second_chunk_header_length;

			memcpy (i, document_contents, document_length);
			g_free (document_contents);
			i += document_length;

			memcpy (i, footer, footer_length);

			/* Append the data */
			soup_message_set_request (message, "multipart/related; boundary=" BOUNDARY_STRING, SOUP_MEMORY_TAKE, upload_data, content_length);
		} else {
			/* Send only metadata */
			upload_data = g_strconcat ("<?xml version='1.0' encoding='UTF-8'?>", entry_xml, NULL);
			g_free (entry_xml);
			soup_message_set_request (message, "application/atom+xml", SOUP_MEMORY_TAKE, upload_data, strlen (upload_data));
		}

		if (document_file_info != NULL)
			g_object_unref (document_file_info);
	}

	/* Send the message */
	status = _gdata_service_send_message (GDATA_SERVICE (self), message, error);
	if (status == SOUP_STATUS_NONE)
		return NULL;

	/* Check for cancellation */
	if (g_cancellable_set_error_if_cancelled (cancellable, error) == TRUE)
		return NULL;

	if (status != good_status_code) {
		/* Error */
		g_assert (klass->parse_error_response != NULL);
		klass->parse_error_response (GDATA_SERVICE (self), GDATA_SERVICE_ERROR_WITH_INSERTION, status, message->reason_phrase, message->response_body->data,
					     message->response_body->length, error);
		return NULL;
	}

	/* Build the updated entry */
	g_assert (message->response_body->data != NULL);

	/* Parse the XML; create and return a new GDataEntry of the same type as @entry */
	return GDATA_DOCUMENTS_ENTRY (gdata_parsable_new_from_xml (new_document_type, message->response_body->data, (gint) message->response_body->length, error));
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
	SoupMessage *message;
	gchar *upload_uri, *tmp_str = NULL;

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

	/* Get the upload URI */
	if (folder != NULL) {
		const gchar *folder_id = gdata_documents_entry_get_document_id (GDATA_DOCUMENTS_ENTRY (folder));
		g_assert (folder_id != NULL);
		upload_uri = tmp_str = g_strconcat ("http://docs.google.com/feeds/folders/private/full/folder%3A", folder_id, NULL);
	} else {
		upload_uri = "http://docs.google.com/feeds/documents/private/full";
	}

	message = soup_message_new (SOUP_METHOD_POST, upload_uri);
	g_free (tmp_str);

	new_document = upload_update_document (self, document, document_file, message, 201, cancellable, error);
	g_object_unref (message);

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
 * Update the document using the properties from @document and the document file pointed to by @document_file.
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
	GDataDocumentsEntry *updated_document;
	SoupMessage *message;
	GDataLink *update_uri;

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
		update_uri = gdata_entry_look_up_link (GDATA_ENTRY (document), GDATA_LINK_EDIT);
	else
		update_uri = gdata_entry_look_up_link (GDATA_ENTRY (document), GDATA_LINK_EDIT_MEDIA);
	g_assert (update_uri != NULL);

	message = soup_message_new (SOUP_METHOD_PUT, gdata_link_get_uri (update_uri));
	soup_message_headers_append (message->request_headers, "If-Match", gdata_entry_get_etag (GDATA_ENTRY (document)));

	updated_document = upload_update_document (self, document, document_file, message, 200, cancellable, error);
	g_object_unref (message);

	return updated_document;
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
	gchar *uri;
	const gchar *document_id, *folder_id;
	SoupMessage *message;
	GDataDocumentsEntry *new_document;
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

	/* Get the document ID */
	folder_id = gdata_documents_entry_get_document_id (GDATA_DOCUMENTS_ENTRY (folder));
	document_id = gdata_documents_entry_get_document_id (GDATA_DOCUMENTS_ENTRY (document));
	g_assert (folder_id != NULL);
	g_assert (document_id != NULL);

	if (GDATA_IS_DOCUMENTS_PRESENTATION (document))
		uri = g_strdup_printf ("http://docs.google.com/feeds/folders/private/full/folder%%3A%s/presentation%%3A%s", folder_id, document_id);
	else if (GDATA_IS_DOCUMENTS_SPREADSHEET (document))
		uri = g_strdup_printf ("http://docs.google.com/feeds/folders/private/full/folder%%3A%s/spreadsheet%%3A%s", folder_id, document_id);
	else if (GDATA_IS_DOCUMENTS_TEXT (document))
		uri = g_strdup_printf ("http://docs.google.com/feeds/folders/private/full/folder%%3A%s/document%%3A%s", folder_id, document_id);
	else
		g_assert_not_reached ();

	message = soup_message_new (SOUP_METHOD_DELETE, uri);
	g_free (uri);

	/* Make sure subclasses set their headers */
	klass = GDATA_SERVICE_GET_CLASS (self);
	if (klass->append_query_headers != NULL)
		klass->append_query_headers (GDATA_SERVICE (self), message);

	soup_message_headers_append (message->request_headers, "If-Match", gdata_entry_get_etag (GDATA_ENTRY (document)));

	/* Check for cancellation */
	if (g_cancellable_set_error_if_cancelled (cancellable, error) == TRUE) {
		g_object_unref (message);
		return NULL;
	}

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

	/* Build the updated entry */
	g_assert (message->response_body->data != NULL);

	/* Parse the XML; and update the document*/
	new_document = GDATA_DOCUMENTS_ENTRY (gdata_parsable_new_from_xml (G_OBJECT_TYPE (document), message->response_body->data,
									   message->response_body->length, error));
	g_object_unref (message);

	return new_document;
}

GDataService *
_gdata_documents_service_get_spreadsheet_service (GDataDocumentsService *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	return self->priv->spreadsheet_service;
}
