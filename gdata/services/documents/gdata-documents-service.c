/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Thibault Saunier 2009 <saunierthibault@gmail.com>
 * Copyright (C) Philip Withnall 2010, 2014 <philip@tecnocode.co.uk>
 * Copyright (C) Red Hat, Inc. 2015, 2016
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
 * @stability: Stable
 * @include: gdata/services/documents/gdata-documents-service.h
 *
 * #GDataDocumentsService is a subclass of #GDataService for communicating with the GData API of Google Drive. It supports querying
 * for, inserting, editing and deleting documents, as well as a folder hierarchy.
 * The API is named ‘documents’ rather than ‘drive’ as it used to use the Google
 * Documents API, which has since been deprecated.
 *
 * For more details of Google Drive's GData API, see the
 * <ulink type="http" url="https://developers.google.com/drive/v2/web/about-sdk">
 * online documentation</ulink>.
 *
 * Fore more details about the spreadsheet downloads handling, see the
 * <ulink type="http" url="http://groups.google.com/group/Google-Docs-Data-APIs/browse_thread/thread/bfc50e94e303a29a?pli=1">
 * online explanation about the problem</ulink>.
 *
 * <example>
 * 	<title>Uploading a Document from Disk</title>
 * 	<programlisting>
 *	GDataDocumentsService *service;
 *	GDataDocumentsDocument *document, *uploaded_document;
 *	GFile *document_file;
 *	GDataDocumentsFolder *destination_folder;
 *	GFileInfo *file_info;
 *	const gchar *slug, *content_type;
 *	GFileInputStream *file_stream;
 *	GDataUploadStream *upload_stream;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service *<!-- -->/
 *	service = create_documents_service ();
 *
 *	/<!-- -->* Get the document file to upload and the folder to upload it into *<!-- -->/
 *	document_file = g_file_new_for_path ("document.odt");
 *	destination_folder = query_user_for_destination_folder (service);
 *
 *	/<!-- -->* Get the file's display name and content type *<!-- -->/
 *	file_info = g_file_query_info (document_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
 *	                               G_FILE_QUERY_INFO_NONE, NULL, &error);
 *
 *	if (error != NULL) {
 *		g_error ("Error getting document file information: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (destination_folder);
 *		g_object_unref (document_file);
 *		g_object_unref (service);
 *		return;
 *	}
 *
 *	slug = g_file_info_get_display_name (file_info);
 *	content_type = g_file_info_get_content_type (file_info);
 *
 *	/<!-- -->* Get an input stream for the file *<!-- -->/
 *	file_stream = g_file_read (document_file, NULL, &error);
 *
 *	g_object_unref (document_file);
 *
 *	if (error != NULL) {
 *		g_error ("Error getting document file stream: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (file_info);
 *		g_object_unref (destination_folder);
 *		g_object_unref (service);
 *		return;
 *	}
 *
 *	/<!-- -->* Create the document metadata to upload *<!-- -->/
 *	document = gdata_documents_text_new (NULL);
 *	gdata_entry_set_title (GDATA_ENTRY (document), "Document Title");
 *
 *	/<!-- -->* Get an upload stream for the document *<!-- -->/
 *	upload_stream = gdata_documents_service_upload_document (service, document, slug, content_type, destination_folder, NULL, &error);
 *
 *	g_object_unref (document);
 *	g_object_unref (file_info);
 *	g_object_unref (destination_folder);
 *
 *	if (error != NULL) {
 *		g_error ("Error getting upload stream: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (file_stream);
 *		g_object_unref (service);
 *		return;
 *	}
 *
 *	/<!-- -->* Upload the document. This is a blocking operation, and should normally be done asynchronously. *<!-- -->/
 *	g_output_stream_splice (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (file_stream),
 *	                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &error);
 *
 *	g_object_unref (file_stream);
 *
 *	if (error != NULL) {
 *		g_error ("Error splicing streams: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (upload_stream);
 *		g_object_unref (service);
 *		return;
 *	}
 *
 *	/<!-- -->* Finish off the upload by parsing the returned updated document metadata entry *<!-- -->/
 *	uploaded_document = gdata_documents_service_finish_upload (service, upload_stream, &error);
 *
 *	g_object_unref (upload_stream);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error uploading document: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	/<!-- -->* Do something with the uploaded document *<!-- -->/
 *
 *	g_object_unref (uploaded_document);
 * 	</programlisting>
 * </example>
 *
 * The Drive service can be manipulated using batch operations, too. See the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#batching_acl_requests">online documentation on batch
 * operations</ulink> for more information.
 *
 * <example>
 * 	<title>Performing a Batch Operation on Documents</title>
 * 	<programlisting>
 *	GDataDocumentsService *service;
 *	GDataBatchOperation *operation;
 *	GDataFeed *feed;
 *	GDataLink *batch_link;
 *	GList *i;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service *<!-- -->/
 *	service = create_documents_service ();
 *
 *	/<!-- -->* Create the batch operation; this requires that we have done a query first so that we can get the batch link *<!-- -->/
 *	feed = do_some_query (service);
 *	batch_link = gdata_feed_look_up_link (feed, GDATA_LINK_BATCH);
 *	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_link_get_uri (batch_link));
 *	g_object_unref (feed);
 *
 *	gdata_batch_operation_add_query (operation, presentation_entry_id_to_query, GDATA_TYPE_DOCUMENTS_PRESENTATION,
 *	                                 (GDataBatchOperationCallback) batch_query_cb, user_data);
 *	gdata_batch_operation_add_insertion (operation, new_entry, (GDataBatchOperationCallback) batch_insertion_cb, user_data);
 *	gdata_batch_operation_add_update (operation, old_entry, (GDataBatchOperationCallback) batch_update_cb, user_data);
 *	gdata_batch_operation_add_deletion (operation, entry_to_delete, (GDataBatchOperationCallback) batch_deletion_cb, user_data);
 *
 *	/<!-- -->* Run the batch operation and handle the results in the various callbacks *<!-- -->/
 *	gdata_test_batch_operation_run (operation, NULL, &error);
 *
 *	g_object_unref (operation);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error running batch operation: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	static void
 *	batch_query_cb (guint operation_id, GDataBatchOperationType operation_type, GDataEntry *entry, GError *error, gpointer user_data)
 *	{
 *		/<!-- -->* operation_type == GDATA_BATCH_OPERATION_QUERY *<!-- -->/
 *		/<!-- -->* Reference and do something with the returned entry. *<!-- -->/
 *	}
 *
 *	static void
 *	batch_insertion_cb (guint operation_id, GDataBatchOperationType operation_type, GDataEntry *entry, GError *error, gpointer user_data)
 *	{
 *		/<!-- -->* operation_type == GDATA_BATCH_OPERATION_INSERTION *<!-- -->/
 *		/<!-- -->* Reference and do something with the returned entry. *<!-- -->/
 *	}
 *
 *	static void
 *	batch_update_cb (guint operation_id, GDataBatchOperationType operation_type, GDataEntry *entry, GError *error, gpointer user_data)
 *	{
 *		/<!-- -->* operation_type == GDATA_BATCH_OPERATION_UPDATE *<!-- -->/
 *		/<!-- -->* Reference and do something with the returned entry. *<!-- -->/
 *	}
 *
 *	static void
 *	batch_deletion_cb (guint operation_id, GDataBatchOperationType operation_type, GDataEntry *entry, GError *error, gpointer user_data)
 *	{
 *		/<!-- -->* operation_type == GDATA_BATCH_OPERATION_DELETION, entry == NULL *<!-- -->/
 *	}
 * 	</programlisting>
 * </example>
 *
 * Starred documents are denoted by being in the %GDATA_CATEGORY_SCHEMA_LABELS_STARRED category of the %GDATA_CATEGORY_SCHEMA_LABELS schema. Documents
 * can be starred or unstarred simply by adding or removing this category from them and updating the document:
 *
 * <example>
 * 	<title>Starring a Document</title>
 * 	<programlisting>
 *	GDataDocumentsService *service;
 *	GDataDocumentsEntry *document, *updated_document;
 *	GDataCategory *starred_category;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service and retrieve the document to be starred *<!-- -->/
 *	service = create_documents_service ();
 *	document = get_document_to_be_starred (service);
 *
 *	/<!-- -->* Add the “starred” category to the document *<!-- -->/
 *	starred_category = gdata_category_new (GDATA_CATEGORY_SCHEMA_LABELS_STARRED, GDATA_CATEGORY_SCHEMA_LABELS, "starred");
 *	gdata_entry_add_category (GDATA_ENTRY (document), starred_category);
 *	g_object_unref (starred_category);
 *
 *	/<!-- -->* Propagate the updated document to the server *<!-- -->/
 *	updated_document = GDATA_DOCUMENTS_ENTRY (gdata_service_update_entry (GDATA_SERVICE (service),
 *	                                                                      gdata_documents_service_get_primary_authorization_domain (),
 *	                                                                      GDATA_ENTRY (document), NULL, &error));
 *
 *	g_object_unref (document);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error starring document: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	/<!-- -->* Do something with the newly-starred document, like update it in the UI *<!-- -->/
 *
 *	g_object_unref (updated_document);
 * 	</programlisting>
 * </example>
 *
 * Since: 0.4.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libsoup/soup.h>
#include <string.h>

#include "gdata-documents-property.h"
#include "gdata-documents-service.h"
#include "gdata-documents-utils.h"
#include "gdata-documents-drive.h"
#include "gdata-batchable.h"
#include "gdata-service.h"
#include "gdata-private.h"
#include "gdata-upload-stream.h"

GQuark
gdata_documents_service_error_quark (void)
{
	return g_quark_from_static_string ("gdata-documents-service-error-quark");
}

static void append_query_headers (GDataService *self, GDataAuthorizationDomain *domain, SoupMessage *message);
static GList *get_authorization_domains (void);
static gchar *_get_upload_uri_for_query_and_folder (GDataDocumentsUploadQuery *query,
                                                    GDataDocumentsFolder *folder) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

_GDATA_DEFINE_AUTHORIZATION_DOMAIN (documents, "writely", "https://www.googleapis.com/auth/drive")
_GDATA_DEFINE_AUTHORIZATION_DOMAIN (spreadsheets, "wise", "https://spreadsheets.google.com/feeds/")
G_DEFINE_TYPE_WITH_CODE (GDataDocumentsService, gdata_documents_service, GDATA_TYPE_SERVICE, G_IMPLEMENT_INTERFACE (GDATA_TYPE_BATCHABLE, NULL))

static void
gdata_documents_service_class_init (GDataDocumentsServiceClass *klass)
{
	GDataServiceClass *service_class = GDATA_SERVICE_CLASS (klass);
	service_class->feed_type = GDATA_TYPE_DOCUMENTS_FEED;

	service_class->append_query_headers = append_query_headers;
	service_class->get_authorization_domains = get_authorization_domains;

	service_class->api_version = "3";
}

static void
gdata_documents_service_init (GDataDocumentsService *self)
{
	/* Nothing to see here */
}

static void
append_query_headers (GDataService *self, GDataAuthorizationDomain *domain, SoupMessage *message)
{
	g_assert (message != NULL);

	if (message->method == SOUP_METHOD_POST && soup_message_headers_get_one (message->request_headers, "X-Upload-Content-Length") == NULL) {
		gchar *upload_uri;
		const gchar *v3_pos;

		upload_uri = soup_uri_to_string (soup_message_get_uri (message), FALSE);
		v3_pos = strstr (upload_uri, "://docs.google.com/feeds/upload/create-session/default/private/full");

		if (v3_pos != NULL) {
			gchar *v2_upload_uri;
			SoupURI *_v2_upload_uri;

			/* Content length header for resumable uploads. Only set it if this looks like the initial request of a resumable upload, and
			 * if no content length has been set previously.
			 * This allows methods like gdata_service_insert_entry() (which aren't resumable-upload-aware) to continue working for creating
			 * documents with metadata only, by simulating the initial request of a resumable upload as described here:
			 * https://developers.google.com/google-apps/documents-list/#creating_a_new_document_or_file_with_metadata_only */
			soup_message_headers_replace (message->request_headers, "X-Upload-Content-Length", "0");

			/* Also set the encoding to be content length encoding. */
			soup_message_headers_set_encoding (message->request_headers, SOUP_ENCODING_CONTENT_LENGTH);

			/* HACK: Work around http://code.google.com/a/google.com/p/apps-api-issues/issues/detail?id=3033 by changing the upload URI
			 * to the v2 API's upload URI. Grrr. */
			v2_upload_uri = g_strconcat (_gdata_service_get_scheme (), "://docs.google.com/feeds/default/private/full",
			                             v3_pos + strlen ("://docs.google.com/feeds/upload/create-session/default/private/full"), NULL);
			_v2_upload_uri = soup_uri_new (v2_upload_uri);
			soup_message_set_uri (message, _v2_upload_uri);
			soup_uri_free (_v2_upload_uri);
			g_free (v2_upload_uri);
		}

		g_free (upload_uri);
	}

	/* Chain up to the parent class */
	GDATA_SERVICE_CLASS (gdata_documents_service_parent_class)->append_query_headers (self, domain, message);
}

static GList *
get_authorization_domains (void)
{
	GList *authorization_domains = NULL;

	authorization_domains = g_list_prepend (authorization_domains, get_documents_authorization_domain ());
	authorization_domains = g_list_prepend (authorization_domains, get_spreadsheets_authorization_domain ());

	return authorization_domains;
}

/**
 * gdata_documents_service_new:
 * @authorizer: (allow-none): a #GDataAuthorizer to authorize the service's requests, or %NULL
 *
 * Creates a new #GDataDocumentsService using the given #GDataAuthorizer. If @authorizer is %NULL, all requests are made as an unauthenticated user.
 *
 * Return value: a new #GDataDocumentsService, or %NULL; unref with g_object_unref()
 *
 * Since: 0.9.0
 */
GDataDocumentsService *
gdata_documents_service_new (GDataAuthorizer *authorizer)
{
	g_return_val_if_fail (authorizer == NULL || GDATA_IS_AUTHORIZER (authorizer), NULL);

	return g_object_new (GDATA_TYPE_DOCUMENTS_SERVICE,
	                     "authorizer", authorizer,
	                     NULL);
}

/**
 * gdata_documents_service_get_primary_authorization_domain:
 *
 * The primary #GDataAuthorizationDomain for interacting with Google Documents. This will not normally need to be used, as it's used internally
 * by the #GDataDocumentsService methods. However, if using the plain #GDataService methods to implement custom queries or requests which libgdata
 * does not support natively, then this domain may be needed to authorize the requests.
 *
 * The domain never changes, and is interned so that pointer comparison can be used to differentiate it from other authorization domains.
 *
 * Return value: (transfer none): the service's authorization domain
 *
 * Since: 0.9.0
 */
GDataAuthorizationDomain *
gdata_documents_service_get_primary_authorization_domain (void)
{
	return get_documents_authorization_domain ();
}

/**
 * gdata_documents_service_get_spreadsheet_authorization_domain:
 *
 * The #GDataAuthorizationDomain for interacting with spreadsheet data. This will not normally need to be used, as it's automatically used internally
 * by the #GDataDocumentsService methods. However, if using the plain #GDataService methods to implement custom queries or requests which libgdata
 * does not support natively, then this domain may be needed to authorize the requests which pertain to the Google Spreadsheets Data API, such as
 * requests to download or upload spreadsheet documents.
 *
 * The domain never changes, and is interned so that pointer comparison can be used to differentiate it from other authorization domains.
 *
 * Return value: (transfer none): the service's authorization domain
 *
 * Since: 0.9.0
 */
GDataAuthorizationDomain *
gdata_documents_service_get_spreadsheet_authorization_domain (void)
{
	return get_spreadsheets_authorization_domain ();
}

/**
 * gdata_documents_service_get_metadata:
 * @self: a #GDataDocumentsService
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Gets a #GDataDocumentsMetadata object containing metadata about the documents
 * service itself, like how large the user quota is.
 *
 * Return value: (transfer full): the service's metadata object; unref with g_object_unref()
 *
 * Since: 0.17.9
 */
GDataDocumentsMetadata *
gdata_documents_service_get_metadata (GDataDocumentsService *self, GCancellable *cancellable, GError **error)
{
	GDataDocumentsMetadata *metadata;
	const gchar *uri = "https://www.googleapis.com/drive/v2/about";
	SoupMessage *message;
	guint status;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	message = _gdata_service_build_message (GDATA_SERVICE (self), get_documents_authorization_domain (), SOUP_METHOD_GET, uri, NULL, FALSE);

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
		klass->parse_error_response (GDATA_SERVICE (self), GDATA_OPERATION_QUERY, status, message->reason_phrase, message->response_body->data,
					     message->response_body->length, error);
		g_object_unref (message);
		return NULL;
	}

	/* Parse the JSON; and update the entry */
	g_assert (message->response_body->data != NULL);
	metadata = GDATA_DOCUMENTS_METADATA (gdata_parsable_new_from_json (GDATA_TYPE_DOCUMENTS_METADATA, message->response_body->data, message->response_body->length,
	                                                                    error));
	g_object_unref (message);

	return metadata;
}

static void
get_metadata_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	GDataDocumentsService *service = GDATA_DOCUMENTS_SERVICE (source_object);
	g_autoptr(GDataDocumentsMetadata) metadata = NULL;
	g_autoptr(GError) error = NULL;

	/* Copy the metadata and return */
	metadata = gdata_documents_service_get_metadata (service, cancellable, &error);
	if (error != NULL)
		g_task_return_error (task, g_steal_pointer (&error));
	else
		g_task_return_pointer (task, g_steal_pointer (&metadata), g_object_unref);
}

/**
 * gdata_documents_service_get_metadata_async:
 * @self: a #GDataDocumentsService
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the operation is finished, or %NULL
 * @user_data: (closure): data to pass to the @callback function
 *
 * Gets a #GDataDocumentsMetadata object containing metadata about the documents
 * service itself, like how large the user quota is.
 *
 * For more details, see gdata_documents_service_get_metadata(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_documents_service_get_metadata_finish() to get the results
 * of the operation.
 *
 * Since: 0.17.9
 */
void
gdata_documents_service_get_metadata_async (GDataDocumentsService *self, GCancellable *cancellable,
                                            GAsyncReadyCallback callback, gpointer user_data)
{
	g_autoptr(GTask) task = NULL;

	g_return_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_documents_service_get_metadata_async);
	g_task_run_in_thread (task, get_metadata_thread);
}

/**
 * gdata_documents_service_get_metadata_finish:
 * @self: a #GDataDocumentsService
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finish an asynchronous operation to get a #GDataDocumentsMetadata started with gdata_documents_service_get_metadata_async().
 *
 * Return value: (transfer full): the service's metadata object; unref with g_object_unref()
 *
 * Since: 0.17.9
 */
GDataDocumentsMetadata *
gdata_documents_service_get_metadata_finish (GDataDocumentsService *self, GAsyncResult *async_result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);
	g_return_val_if_fail (g_task_is_valid (async_result, self), NULL);
	g_return_val_if_fail (g_async_result_is_tagged (async_result, gdata_documents_service_get_metadata_async), NULL);

	return g_task_propagate_pointer (G_TASK (async_result), error);
}

static gchar *
_query_documents_build_request_uri (GDataDocumentsQuery *query)
{
	/* If we want to query for documents contained in a folder, the URI is different.
	 * The "/[folder:id]" suffix is added by the GDataQuery later. */
	return g_strdup ("https://www.googleapis.com/drive/v2/files");
}

/**
 * gdata_documents_service_query_documents:
 * @self: a #GDataDocumentsService
 * @query: (allow-none): a #GDataDocumentsQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (scope call) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
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
 */
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
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_documents_authorization_domain ()) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to query documents."));
		return NULL;
	}

	request_uri = _query_documents_build_request_uri (query);
	feed = gdata_service_query (GDATA_SERVICE (self), get_documents_authorization_domain (), request_uri, GDATA_QUERY (query),
	                            GDATA_TYPE_DOCUMENTS_ENTRY, cancellable, progress_callback, progress_user_data, error);
	g_free (request_uri);

	return GDATA_DOCUMENTS_FEED (feed);
}

/**
 * gdata_documents_service_query_documents_async:
 * @self: a #GDataDocumentsService
 * @query: (allow-none): a #GDataDocumentsQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @destroy_progress_user_data: (allow-none): the function to call when @progress_callback will not be called any more, or %NULL. This function will be
 * called with @progress_user_data as a parameter and can be used to free any memory allocated for it.
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Queries the service to return a list of documents matching the given @query. @self and
 * @query are both reffed when this function is called, so can safely be unreffed after this function returns.
 *
 * For more details, see gdata_documents_service_query_documents(), which is the synchronous version of this function,
 * and gdata_service_query_async(), which is the base asynchronous query function.
 *
 * Since: 0.9.1
 */
void
gdata_documents_service_query_documents_async (GDataDocumentsService *self, GDataDocumentsQuery *query, GCancellable *cancellable,
                                               GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                               GDestroyNotify destroy_progress_user_data,
                                               GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *request_uri;

	g_return_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self));
	g_return_if_fail (query == NULL || GDATA_IS_DOCUMENTS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	/* Ensure we're authenticated first */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_documents_authorization_domain ()) == FALSE) {
		g_autoptr(GTask) task = NULL;

		task = g_task_new (self, cancellable, callback, user_data);
		g_task_set_source_tag (task, gdata_service_query_async);
		g_task_return_new_error (task, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED, "%s",
		                         _("You must be authenticated to query documents."));

		return;
	}

	request_uri = _query_documents_build_request_uri (query);
	gdata_service_query_async (GDATA_SERVICE (self), get_documents_authorization_domain (), request_uri, GDATA_QUERY (query),
	                           GDATA_TYPE_DOCUMENTS_ENTRY, cancellable, progress_callback, progress_user_data,
	                           destroy_progress_user_data, callback, user_data);
	g_free (request_uri);
}

/**
 * gdata_documents_service_query_drives:
 * @self: a #GDataDocumentsService
 * @query: (nullable): a #GDataDocumentsDriveQuery with the query parameters, or %NULL
 * @cancellable: (nullable): optional #GCancellable object, or %NULL
 * @progress_callback: (nullable) (scope call) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service to return a list of shared drives matching the given @query.
 *
 * For more details, see gdata_service_query().
 *
 * Return value: (transfer full): a #GDataDocumentsFeed of query results; unref with g_object_unref()
 *
 * Since: 0.18.0
 */
GDataDocumentsFeed *
gdata_documents_service_query_drives (GDataDocumentsService *self, GDataDocumentsDriveQuery *query, GCancellable *cancellable,
                                      GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                      GError **error)
{
	GDataFeed *feed;
	const gchar *request_uri = "https://www.googleapis.com/drive/v2/drives";

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_DOCUMENTS_DRIVE_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Ensure we're authenticated first */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_documents_authorization_domain ()) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to query drives."));
		return NULL;
	}

	feed = gdata_service_query (GDATA_SERVICE (self), get_documents_authorization_domain (), request_uri, GDATA_QUERY (query),
	                            GDATA_TYPE_DOCUMENTS_DRIVE, cancellable, progress_callback, progress_user_data, error);

	return GDATA_DOCUMENTS_FEED (feed);
}

/**
 * gdata_documents_service_query_drives_async:
 * @self: a #GDataDocumentsService
 * @query: (nullable): a #GDataDocumentsDriveQuery with the query parameters, or %NULL
 * @cancellable: (nullable): optional #GCancellable object, or %NULL
 * @progress_callback: (nullable) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @destroy_progress_user_data: (nullable): the function to call when @progress_callback will not be called any more, or %NULL. This function will be
 * called with @progress_user_data as a parameter and can be used to free any memory allocated for it.
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Queries the service to return a list of shared drives matching the given @query. @self and
 * @query are both reffed when this function is called, so can safely be unreffed after this function returns.
 *
 * For more details, see gdata_documents_service_query_drives(), which is the synchronous version of this function,
 * and gdata_service_query_async(), which is the base asynchronous query function.
 *
 * Since: 0.18.0
 */
void
gdata_documents_service_query_drives_async (GDataDocumentsService *self, GDataDocumentsDriveQuery *query, GCancellable *cancellable,
                                            GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                            GDestroyNotify destroy_progress_user_data,
                                            GAsyncReadyCallback callback, gpointer user_data)
{
	const gchar *request_uri = "https://www.googleapis.com/drive/v2/drives";

	g_return_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self));
	g_return_if_fail (query == NULL || GDATA_IS_DOCUMENTS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	/* Ensure we're authenticated first */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_documents_authorization_domain ()) == FALSE) {
		g_autoptr(GTask) task = NULL;

		task = g_task_new (self, cancellable, callback, user_data);
		g_task_set_source_tag (task, gdata_service_query_async);
		g_task_return_new_error (task, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED, "%s",
		                         _("You must be authenticated to query drives."));

		return;
	}

	gdata_service_query_async (GDATA_SERVICE (self), get_documents_authorization_domain (), request_uri, GDATA_QUERY (query),
	                           GDATA_TYPE_DOCUMENTS_DRIVE, cancellable, progress_callback, progress_user_data,
	                           destroy_progress_user_data, callback, user_data);
}

static void
add_folder_link_to_entry (GDataDocumentsEntry *entry, GDataDocumentsFolder *folder)
{
	GDataLink *_link;
	const gchar *id;
	gchar *uri;

	/* HACK: Build the GDataLink:uri from the ID by adding the prefix. */
	id = gdata_entry_get_id (GDATA_ENTRY (folder));
	uri = g_strconcat (GDATA_DOCUMENTS_URI_PREFIX, id, NULL);
	_link = gdata_link_new (uri, GDATA_LINK_PARENT);
	gdata_entry_add_link (GDATA_ENTRY (entry), _link);
	g_object_unref (_link);
	g_free (uri);
}

static GDataUploadStream *
upload_update_document (GDataDocumentsService *self, GDataDocumentsDocument *document, const gchar *slug, const gchar *content_type,
                        GDataDocumentsFolder *folder, goffset content_length, const gchar *method, const gchar *upload_uri,
			GCancellable *cancellable)
{
	/* HACK: Corrects a bug on spreadsheet content types handling
	 * The content type for ODF spreadsheets is "application/vnd.oasis.opendocument.spreadsheet" for my ODF spreadsheet;
	 * but Google Documents' spreadsheet service is waiting for "application/x-vnd.oasis.opendocument.spreadsheet"
	 * and nothing else.
	 * Bug filed with Google: http://code.google.com/p/gdata-issues/issues/detail?id=1127 */
	if (strcmp (content_type, "application/vnd.oasis.opendocument.spreadsheet") == 0)
		content_type = "application/x-vnd.oasis.opendocument.spreadsheet";

	if (folder != NULL)
		add_folder_link_to_entry (GDATA_DOCUMENTS_ENTRY (document), folder);

	/* We need streaming file I/O: GDataUploadStream */
	if (content_length == -1) {
		/* Non-resumable upload. */
		return GDATA_UPLOAD_STREAM (gdata_upload_stream_new (GDATA_SERVICE (self), get_documents_authorization_domain (), method, upload_uri,
	                                                             GDATA_ENTRY (document), slug, content_type, cancellable));
	} else {
		/* Resumable upload. */
		return GDATA_UPLOAD_STREAM (gdata_upload_stream_new_resumable (GDATA_SERVICE (self), get_documents_authorization_domain (), method,
		                                                               upload_uri, GDATA_ENTRY (document), slug, content_type, content_length,
		                                                               cancellable));
	}
}

static gboolean
_upload_checks (GDataDocumentsService *self, GDataDocumentsDocument *document, GError **error)
{
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_documents_authorization_domain ()) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to upload documents."));
		return FALSE;
	}

	if (document != NULL && gdata_entry_is_inserted (GDATA_ENTRY (document)) == TRUE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED,
		                     _("The document has already been uploaded."));
		return FALSE;
	}

	return TRUE;
}

/**
 * gdata_documents_service_upload_document:
 * @self: an authenticated #GDataDocumentsService
 * @document: (allow-none): the #GDataDocumentsDocument to insert, or %NULL
 * @slug: the filename to give to the uploaded document
 * @content_type: the content type of the uploaded data
 * @folder: (allow-none): the folder to which the document should be uploaded, or %NULL
 * @cancellable: (allow-none): a #GCancellable for the entire upload stream, or %NULL
 * @error: a #GError, or %NULL
 *
 * Uploads a document to Google Documents, using the properties from @document and the document data written to the resulting #GDataUploadStream. If
 * the document data does not need to be provided at the moment, just the metadata, use gdata_service_insert_entry() instead (e.g. in the case of
 * creating a new, empty file to be edited at a later date).
 *
 * This performs a non-resumable upload, unlike gdata_documents_service_upload_document(). This means that errors during transmission will cause the
 * upload to fail, and the entire document will have to be re-uploaded. It is recommended that gdata_documents_service_upload_document_resumable()
 * be used instead.
 *
 * If @document is %NULL, only the document data will be uploaded. The new document entry will be named using @slug, and will have default metadata.
 *
 * The stream returned by this function should be written to using the standard #GOutputStream methods, asynchronously or synchronously. Once the stream
 * is closed (using g_output_stream_close()), gdata_documents_service_finish_upload() should be called on it to parse and return the updated
 * #GDataDocumentsDocument for the document. This must be done, as @document isn't updated in-place.
 *
 * In order to cancel the upload, a #GCancellable passed in to @cancellable must be cancelled using g_cancellable_cancel(). Cancelling the individual
 * #GOutputStream operations on the #GDataUploadStream will not cancel the entire upload; merely the write or close operation in question. See the
 * #GDataUploadStream:cancellable for more details.
 *
 * Any upload errors will be thrown by the stream methods, and may come from the #GDataServiceError domain.
 *
 * Return value: (transfer full): a #GDataUploadStream to write the document data to, or %NULL; unref with g_object_unref()
 *
 * Since: 0.8.0
 */
GDataUploadStream *
gdata_documents_service_upload_document (GDataDocumentsService *self, GDataDocumentsDocument *document, const gchar *slug, const gchar *content_type,
                                         GDataDocumentsFolder *folder, GCancellable *cancellable, GError **error)
{
	GDataUploadStream *upload_stream;
	gchar *upload_uri;
	gchar *upload_uri_prefix;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (document == NULL || GDATA_IS_DOCUMENTS_DOCUMENT (document), NULL);
	g_return_val_if_fail (slug != NULL && *slug != '\0', NULL);
	g_return_val_if_fail (content_type != NULL && *content_type != '\0', NULL);
	g_return_val_if_fail (folder == NULL || GDATA_IS_DOCUMENTS_FOLDER (folder), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (_upload_checks (self, document, error) == FALSE) {
		return NULL;
	}

	upload_uri_prefix = gdata_documents_service_get_upload_uri (folder);
	upload_uri = g_strconcat (upload_uri_prefix, "?uploadType=multipart", NULL);
	upload_stream = upload_update_document (self, document, slug, content_type, folder, -1, SOUP_METHOD_POST, upload_uri, cancellable);
	g_free (upload_uri);
	g_free (upload_uri_prefix);

	return upload_stream;
}

/**
 * gdata_documents_service_upload_document_resumable:
 * @self: an authenticated #GDataDocumentsService
 * @document: (allow-none): the #GDataDocumentsDocument to insert, or %NULL
 * @slug: the filename to give to the uploaded document
 * @content_type: the content type of the uploaded data
 * @content_length: the size (in bytes) of the file being uploaded
 * @query: (allow-none): a query specifying parameters for the upload, or %NULL
 * @cancellable: (allow-none): a #GCancellable for the entire upload stream, or %NULL
 * @error: a #GError, or %NULL
 *
 * Uploads a document to Google Documents, using the properties from @document and the document data written to the resulting #GDataUploadStream. If
 * the document data does not need to be provided at the moment, just the metadata, use gdata_service_insert_entry() instead (e.g. in the case of
 * creating a new, empty file to be edited at a later date).
 *
 * Unlike gdata_documents_service_upload_document(), this method performs a
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/resumable_upload.html">resumable upload</ulink> which allows for correction of
 * transmission errors without re-uploading the entire file. Use of this method is preferred over gdata_documents_service_upload_document().
 *
 * If @document is %NULL, only the document data will be uploaded. The new document entry will be named using @slug, and will have default metadata.
 *
 * If non-%NULL, the @query specifies parameters for the upload, such as a #GDataDocumentsFolder to upload the document into; and whether to treat
 * the document as an opaque file, or convert it to a standard format. If @query is %NULL, the document will be uploaded into the root folder, and
 * automatically converted to a standard format. No OCR or automatic language translation will be performed by default.
 *
 * If @query is non-%NULL and #GDataDocumentsUploadQuery:convert is %FALSE, @document must be an instance of #GDataDocumentsDocument. Otherwise,
 * @document must be a subclass of it, such as #GDataDocumentsPresentation.
 *
 * The stream returned by this function should be written to using the standard #GOutputStream methods, asynchronously or synchronously. Once the stream
 * is closed (using g_output_stream_close()), gdata_documents_service_finish_upload() should be called on it to parse and return the updated
 * #GDataDocumentsDocument for the document. This must be done, as @document isn't updated in-place.
 *
 * In order to cancel the upload, a #GCancellable passed in to @cancellable must be cancelled using g_cancellable_cancel(). Cancelling the individual
 * #GOutputStream operations on the #GDataUploadStream will not cancel the entire upload; merely the write or close operation in question. See the
 * #GDataUploadStream:cancellable for more details.
 *
 * Any upload errors will be thrown by the stream methods, and may come from the #GDataServiceError domain.
 *
 * Return value: (transfer full): a #GDataUploadStream to write the document data to, or %NULL; unref with g_object_unref()
 *
 * Since: 0.13.0
 */
GDataUploadStream *
gdata_documents_service_upload_document_resumable (GDataDocumentsService *self, GDataDocumentsDocument *document, const gchar *slug,
                                                   const gchar *content_type, goffset content_length, GDataDocumentsUploadQuery *query,
                                                   GCancellable *cancellable, GError **error)
{
	GDataUploadStream *upload_stream;
	gchar *upload_uri;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (document == NULL || GDATA_IS_DOCUMENTS_DOCUMENT (document), NULL);
	g_return_val_if_fail (slug != NULL && *slug != '\0', NULL);
	g_return_val_if_fail (content_type != NULL && *content_type != '\0', NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_DOCUMENTS_UPLOAD_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (_upload_checks (self, document, error) == FALSE) {
		return NULL;
	}

	upload_uri = _get_upload_uri_for_query_and_folder (query, NULL);
	upload_stream = upload_update_document (self, document, slug, content_type, NULL, content_length, SOUP_METHOD_POST, upload_uri, cancellable);
	g_free (upload_uri);

	return upload_stream;
}

static gboolean
_update_checks (GDataDocumentsService *self, GError **error)
{
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_documents_authorization_domain ()) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to update documents."));
		return FALSE;
	}

	return TRUE;
}

/**
 * gdata_documents_service_update_document:
 * @self: a #GDataDocumentsService
 * @document: the #GDataDocumentsDocument to update
 * @slug: the filename to give to the uploaded document
 * @content_type: the content type of the uploaded data
 * @cancellable: (allow-none): a #GCancellable for the entire upload stream, or %NULL
 * @error: a #GError, or %NULL
 *
 * Update the document using the properties from @document and the document data written to the resulting #GDataUploadStream. If the document data does
 * not need to be changed, just the metadata, use gdata_service_update_entry() instead.
 *
 * This performs a non-resumable upload, unlike gdata_documents_service_update_document(). This means that errors during transmission will cause the
 * upload to fail, and the entire document will have to be re-uploaded. It is recommended that gdata_documents_service_update_document_resumable()
 * be used instead.
 *
 * The stream returned by this function should be written to using the standard #GOutputStream methods, asynchronously or synchronously. Once the stream
 * is closed (using g_output_stream_close()), gdata_documents_service_finish_upload() should be called on it to parse and return the updated
 * #GDataDocumentsDocument for the document. This must be done, as @document isn't updated in-place.
 *
 * In order to cancel the update, a #GCancellable passed in to @cancellable must be cancelled using g_cancellable_cancel(). Cancelling the individual
 * #GOutputStream operations on the #GDataUploadStream will not cancel the entire update; merely the write or close operation in question. See the
 * #GDataUploadStream:cancellable for more details.
 *
 * Any upload errors will be thrown by the stream methods, and may come from the #GDataServiceError domain.
 *
 * For more information, see gdata_service_update_entry().
 *
 * Return value: (transfer full): a #GDataUploadStream to write the document data to; unref with g_object_unref()
 *
 * Since: 0.8.0
 */
GDataUploadStream *
gdata_documents_service_update_document (GDataDocumentsService *self, GDataDocumentsDocument *document, const gchar *slug, const gchar *content_type,
                                         GCancellable *cancellable, GError **error)
{
	GDataUploadStream *update_stream;
	const gchar *id;
	gchar *update_uri;
	gchar *update_uri_prefix;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_DOCUMENT (document), NULL);
	g_return_val_if_fail (slug != NULL && *slug != '\0', NULL);
	g_return_val_if_fail (content_type != NULL && *content_type != '\0', NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (_update_checks (self, error) == FALSE) {
		return NULL;
	}

	update_uri_prefix = gdata_documents_service_get_upload_uri (NULL);
	id = gdata_entry_get_id (GDATA_ENTRY (document));
	update_uri = g_strconcat (update_uri_prefix, "/", id, "?uploadType=multipart", NULL);
	update_stream = upload_update_document (self, document, slug, content_type, NULL, -1, SOUP_METHOD_PUT, update_uri, cancellable);
	g_free (update_uri);
	g_free (update_uri_prefix);

	return update_stream;
}

/**
 * gdata_documents_service_update_document_resumable:
 * @self: a #GDataDocumentsService
 * @document: the #GDataDocumentsDocument to update
 * @slug: the filename to give to the uploaded document
 * @content_type: the content type of the uploaded data
 * @content_length: the size (in bytes) of the file being uploaded
 * @cancellable: (allow-none): a #GCancellable for the entire upload stream, or %NULL
 * @error: a #GError, or %NULL
 *
 * Update the document using the properties from @document and the document data written to the resulting #GDataUploadStream. If the document data does
 * not need to be changed, just the metadata, use gdata_service_update_entry() instead.
 *
 * Unlike gdata_documents_service_update_document(), this method performs a
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/resumable_upload.html">resumable upload</ulink> which allows for correction of
 * transmission errors without re-uploading the entire file. Use of this method is preferred over gdata_documents_service_update_document().
 *
 * The stream returned by this function should be written to using the standard #GOutputStream methods, asynchronously or synchronously. Once the stream
 * is closed (using g_output_stream_close()), gdata_documents_service_finish_upload() should be called on it to parse and return the updated
 * #GDataDocumentsDocument for the document. This must be done, as @document isn't updated in-place.
 *
 * In order to cancel the update, a #GCancellable passed in to @cancellable must be cancelled using g_cancellable_cancel(). Cancelling the individual
 * #GOutputStream operations on the #GDataUploadStream will not cancel the entire update; merely the write or close operation in question. See the
 * #GDataUploadStream:cancellable for more details.
 *
 * Any upload errors will be thrown by the stream methods, and may come from the #GDataServiceError domain.
 *
 * For more information, see gdata_service_update_entry().
 *
 * Return value: (transfer full): a #GDataUploadStream to write the document data to; unref with g_object_unref()
 *
 * Since: 0.13.0
 */
GDataUploadStream *
gdata_documents_service_update_document_resumable (GDataDocumentsService *self, GDataDocumentsDocument *document, const gchar *slug,
                                                   const gchar *content_type, goffset content_length, GCancellable *cancellable, GError **error)
{
	GDataLink *update_link;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_DOCUMENT (document), NULL);
	g_return_val_if_fail (slug != NULL && *slug != '\0', NULL);
	g_return_val_if_fail (content_type != NULL && *content_type != '\0', NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (_update_checks (self, error) == FALSE) {
		return NULL;
	}

	update_link = gdata_entry_look_up_link (GDATA_ENTRY (document), GDATA_LINK_RESUMABLE_EDIT_MEDIA);
	g_assert (update_link != NULL);

	return upload_update_document (self, document, slug, content_type, NULL, content_length, SOUP_METHOD_PUT, gdata_link_get_uri (update_link),
	                               cancellable);
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
	const gchar *content_type;
	const gchar *response_body;
	gssize response_length;
	GType new_document_type = G_TYPE_INVALID;

	/* Get and parse the response from the server */
	response_body = gdata_upload_stream_get_response (upload_stream, &response_length);
	if (response_body == NULL || response_length == 0) {
		/* Error will have been set by the upload stream. */
		return NULL;
	}

	content_type = gdata_upload_stream_get_content_type (upload_stream);
	new_document_type = gdata_documents_utils_get_type_from_content_type (content_type);

	if (g_type_is_a (new_document_type, GDATA_TYPE_DOCUMENTS_DOCUMENT) == FALSE) {
		g_set_error (error, GDATA_DOCUMENTS_SERVICE_ERROR, GDATA_DOCUMENTS_SERVICE_ERROR_INVALID_CONTENT_TYPE,
		             _("The content type of the supplied document (‘%s’) could not be recognized."),
		             content_type);
		return NULL;
	}

	return GDATA_DOCUMENTS_DOCUMENT (gdata_parsable_new_from_json (new_document_type, response_body, (gint) response_length, error));
}

/**
 * gdata_documents_service_copy_document:
 * @self: an authenticated #GDataDocumentsService
 * @document: the #GDataDocumentsDocument to copy
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Copy the given @document, producing a duplicate document in the same folder and returning its #GDataDocumentsDocument.
 *
 * Errors from #GDataServiceError can be returned for exceptional conditions, as determined by the server.
 *
 * Return value: (transfer full): the duplicate #GDataDocumentsDocument, or %NULL; unref with g_object_unref()
 *
 * Since: 0.13.1
 */
GDataDocumentsDocument *
gdata_documents_service_copy_document (GDataDocumentsService *self, GDataDocumentsDocument *document, GCancellable *cancellable, GError **error)
{
	GDataDocumentsDocument *new_document;
	GDataEntry *parent = NULL;
	GList *i;
	GList *parent_folders_list;
	const gchar *parent_id = NULL;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_DOCUMENT (document), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_documents_authorization_domain ()) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to copy documents."));
		return NULL;
	}

	parent_folders_list = gdata_entry_look_up_links (GDATA_ENTRY (document), GDATA_LINK_PARENT);
	for (i = parent_folders_list; i != NULL; i = i->next) {
		GDataLink *_link = GDATA_LINK (i->data);
		const gchar *id;

		id = gdata_documents_utils_get_id_from_link (_link);
		if (id != NULL) {
			parent_id = id;
			break;
		}
	}

	g_list_free (parent_folders_list);

	if (parent_id == NULL) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_NOT_FOUND, _("Parent folder not found"));
		return NULL;
	}

	parent = gdata_service_query_single_entry (GDATA_SERVICE (self), get_documents_authorization_domain (), parent_id, NULL, GDATA_TYPE_DOCUMENTS_FOLDER, cancellable, error);
	if (parent == NULL)
		return NULL;

	new_document = GDATA_DOCUMENTS_DOCUMENT (gdata_documents_service_add_entry_to_folder (self, GDATA_DOCUMENTS_ENTRY (document), GDATA_DOCUMENTS_FOLDER (parent), cancellable, error));
	g_object_unref (parent);

	return new_document;
}

static void
copy_document_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	GDataDocumentsService *service = GDATA_DOCUMENTS_SERVICE (source_object);
	GDataDocumentsDocument *document = task_data;
	g_autoptr(GDataDocumentsDocument) new_document = NULL;
	g_autoptr(GError) error = NULL;

	/* Copy the document and return */
	new_document = gdata_documents_service_copy_document (service, document, cancellable, &error);
	if (error != NULL)
		g_task_return_error (task, g_steal_pointer (&error));
	else
		g_task_return_pointer (task, g_steal_pointer (&new_document), g_object_unref);
}

/**
 * gdata_documents_service_copy_document_async:
 * @self: a #GDataDocumentsService
 * @document: the #GDataDocumentsDocument to copy
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the operation is finished, or %NULL
 * @user_data: (closure): data to pass to the @callback function
 *
 * Copy the given @document, producing a duplicate document in the same folder and returning its #GDataDocumentsDocument. @self and @document are
 * both reffed when this function is called, so can safely be unreffed after this function returns.
 *
 * For more details, see gdata_documents_service_copy_document(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_documents_service_copy_document_finish() to get the results
 * of the operation.
 *
 * Since: 0.13.1
 */
void
gdata_documents_service_copy_document_async (GDataDocumentsService *self, GDataDocumentsDocument *document, GCancellable *cancellable,
                                             GAsyncReadyCallback callback, gpointer user_data)
{
	g_autoptr(GTask) task = NULL;

	g_return_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self));
	g_return_if_fail (GDATA_IS_DOCUMENTS_DOCUMENT (document));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_documents_service_copy_document_async);
	g_task_set_task_data (task, g_object_ref (document), (GDestroyNotify) g_object_unref);
	g_task_run_in_thread (task, copy_document_thread);
}

/**
 * gdata_documents_service_copy_document_finish:
 * @self: a #GDataDocumentsService
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finish an asynchronous operation to copy a #GDataDocumentsDocument started with gdata_documents_service_copy_document_async().
 *
 * Return value: (transfer full): the duplicate #GDataDocumentsDocument, or %NULL; unref with g_object_unref()
 *
 * Since: 0.13.1
 */
GDataDocumentsDocument *
gdata_documents_service_copy_document_finish (GDataDocumentsService *self, GAsyncResult *async_result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);
	g_return_val_if_fail (g_task_is_valid (async_result, self), NULL);
	g_return_val_if_fail (g_async_result_is_tagged (async_result, gdata_documents_service_copy_document_async), NULL);

	return g_task_propagate_pointer (G_TASK (async_result), error);
}

/**
 * gdata_documents_service_add_entry_to_folder:
 * @self: an authenticated #GDataDocumentsService
 * @entry: the #GDataDocumentsEntry to copy
 * @folder: the #GDataDocumentsFolder to copy @entry into
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Add the given @entry to the specified @folder, and return an updated #GDataDocumentsEntry for @entry. If the @entry is already in another folder,
 * a copy will be added to the new folder. The copy and original will have different IDs. Note that @entry can't be a #GDataDocumentsFolder that
 * already exists on the server. It can be a new #GDataDocumentsFolder, or a #GDataDocumentsDocument that is either new or already present on the
 * server.
 *
 * Errors from #GDataServiceError can be returned for exceptional conditions, as determined by the server.
 *
 * Return value: (transfer full): an updated #GDataDocumentsEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.8.0
 */
GDataDocumentsEntry *
gdata_documents_service_add_entry_to_folder (GDataDocumentsService *self, GDataDocumentsEntry *entry, GDataDocumentsFolder *folder,
                                             GCancellable *cancellable, GError **error)
{
	GDataDocumentsEntry *new_entry;
	GDataDocumentsEntry *local_entry;
	GDataOperationType operation_type;
	GType entry_type;
	const gchar *content_type;
	const gchar *etag;
	const gchar *title;
	const gchar *uri_prefix = "https://www.googleapis.com/drive/v2/files";
	gchar *upload_data;
	gchar *uri;
	SoupMessage *message;
	guint status;
	GList *l;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (entry), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_FOLDER (folder), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_documents_authorization_domain ()) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to insert or move documents and folders."));
		return NULL;
	}

	if (gdata_entry_is_inserted (GDATA_ENTRY (entry)) == TRUE) {
		const gchar *id;

		id = gdata_entry_get_id (GDATA_ENTRY (entry));
		uri = g_strconcat (uri_prefix, "/", id, "/copy", NULL);
		operation_type = GDATA_OPERATION_UPDATE;
	} else {
		uri = g_strdup (uri_prefix);
		operation_type = GDATA_OPERATION_INSERTION;
	}

	entry_type = G_OBJECT_TYPE (entry);
	content_type = gdata_documents_utils_get_content_type (entry);
	etag = gdata_entry_get_etag (GDATA_ENTRY (entry));
	title = gdata_entry_get_title (GDATA_ENTRY (entry));
	local_entry = g_object_new (entry_type, "etag", etag, "title", title, NULL);
	gdata_documents_utils_add_content_type (local_entry, content_type);
	add_folder_link_to_entry (local_entry, folder);

	for (l = gdata_documents_entry_get_document_properties (entry); l != NULL; l = l->next) {
		GDataDocumentsProperty *old_prop;
		g_autoptr(GDataDocumentsProperty) new_prop = NULL;

		old_prop = GDATA_DOCUMENTS_PROPERTY (l->data);

		new_prop = gdata_documents_property_new (gdata_documents_property_get_key (old_prop));
		gdata_documents_property_set_value (new_prop, gdata_documents_property_get_value (old_prop));
		gdata_documents_property_set_visibility (new_prop, gdata_documents_property_get_visibility (old_prop));
		gdata_documents_entry_add_documents_property (local_entry, new_prop);
	}

	message = _gdata_service_build_message (GDATA_SERVICE (self), get_documents_authorization_domain (), SOUP_METHOD_POST, uri, NULL, FALSE);
	g_free (uri);

	/* Append the data */
	upload_data = gdata_parsable_get_json (GDATA_PARSABLE (local_entry));
	soup_message_set_request (message, "application/json", SOUP_MEMORY_TAKE, upload_data, strlen (upload_data));
	g_object_unref (local_entry);

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
		klass->parse_error_response (GDATA_SERVICE (self), operation_type, status, message->reason_phrase, message->response_body->data,
					     message->response_body->length, error);
		g_object_unref (message);
		return NULL;
	}

	/* Parse the JSON; and update the entry */
	g_assert (message->response_body->data != NULL);
	new_entry = GDATA_DOCUMENTS_ENTRY (gdata_parsable_new_from_json (entry_type, message->response_body->data, message->response_body->length,
									 error));
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
add_entry_to_folder_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	GDataDocumentsService *service = GDATA_DOCUMENTS_SERVICE (source_object);
	g_autoptr(GDataDocumentsEntry) updated_entry = NULL;
	AddEntryToFolderData *data = task_data;
	g_autoptr(GError) error = NULL;

	/* Add the entry to the folder and return */
	updated_entry = gdata_documents_service_add_entry_to_folder (service, data->entry, data->folder, cancellable, &error);
	if (error != NULL)
		g_task_return_error (task, g_steal_pointer (&error));
	else
		g_task_return_pointer (task, g_steal_pointer (&updated_entry), (GDestroyNotify) g_object_unref);
}

/**
 * gdata_documents_service_add_entry_to_folder_async:
 * @self: a #GDataDocumentsService
 * @entry: the #GDataDocumentsEntry to add to @folder
 * @folder: the #GDataDocumentsFolder to add @entry to
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
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
 */
void
gdata_documents_service_add_entry_to_folder_async (GDataDocumentsService *self, GDataDocumentsEntry *entry, GDataDocumentsFolder *folder,
                                                   GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	g_autoptr(GTask) task = NULL;
	AddEntryToFolderData *data;

	g_return_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self));
	g_return_if_fail (GDATA_IS_DOCUMENTS_ENTRY (entry));
	g_return_if_fail (GDATA_IS_DOCUMENTS_FOLDER (folder));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	data = g_slice_new (AddEntryToFolderData);
	data->entry = g_object_ref (entry);
	data->folder = g_object_ref (folder);

	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_documents_service_add_entry_to_folder_async);
	g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) add_entry_to_folder_data_free);
	g_task_run_in_thread (task, add_entry_to_folder_thread);
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
 */
GDataDocumentsEntry *
gdata_documents_service_add_entry_to_folder_finish (GDataDocumentsService *self, GAsyncResult *async_result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);
	g_return_val_if_fail (g_task_is_valid (async_result, self), NULL);
	g_return_val_if_fail (g_async_result_is_tagged (async_result, gdata_documents_service_add_entry_to_folder_async), NULL);

	return g_task_propagate_pointer (G_TASK (async_result), error);
}

/**
 * gdata_documents_service_remove_entry_from_folder:
 * @self: a #GDataDocumentsService
 * @entry: the #GDataDocumentsEntry to remove
 * @folder: the #GDataDocumentsFolder from which we should remove @entry
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
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
 */
GDataDocumentsEntry *
gdata_documents_service_remove_entry_from_folder (GDataDocumentsService *self, GDataDocumentsEntry *entry, GDataDocumentsFolder *folder,
                                                  GCancellable *cancellable, GError **error)
{
	const gchar *folder_id;
	GList *i;
	GList *parent_folders_list;
	GDataLink *folder_link = NULL, *file_link = NULL;
	GDataParsableClass *klass;
	GDataAuthorizationDomain *domain;
	gchar *fixed_uri, *modified_uri;
	guint status;
	gboolean req_status = TRUE;
	SoupMessage *message;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (entry), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_FOLDER (folder), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_documents_authorization_domain ()) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to move documents and folders."));
		return NULL;
	}

	domain = gdata_documents_service_get_primary_authorization_domain();

	folder_id = gdata_entry_get_id (GDATA_ENTRY (folder));
	g_assert (folder_id != NULL);

	parent_folders_list = gdata_entry_look_up_links (GDATA_ENTRY (entry), GDATA_LINK_PARENT);
	for (i = parent_folders_list; i != NULL; i = i->next) {
		GDataLink *_link = GDATA_LINK (i->data);
		const gchar *id;

		id = gdata_documents_utils_get_id_from_link (_link);
		if (g_strcmp0 (folder_id, id) == 0) {
			folder_link = _link;
			break;
		}
	}

	g_list_free (parent_folders_list);

	if (folder_link == NULL) {
		g_set_error_literal (error,
				     GDATA_SERVICE_ERROR,
				     GDATA_SERVICE_ERROR_NOT_FOUND,
				     _("Parent folder not found"));
		return NULL;
	}

	klass = GDATA_PARSABLE_GET_CLASS (entry);

	g_assert (klass->get_content_type != NULL);
	if (g_strcmp0 (klass->get_content_type (), "application/json") == 0) {
		file_link = gdata_entry_look_up_link (GDATA_ENTRY (entry), GDATA_LINK_SELF);
	} else {
		file_link = gdata_entry_look_up_link (GDATA_ENTRY (entry), GDATA_LINK_EDIT);
	}
	g_debug ("Link = %s", gdata_link_get_uri(file_link));
	g_assert (file_link != NULL);

	fixed_uri = _gdata_service_fix_uri_scheme (gdata_link_get_uri (file_link));
	modified_uri = g_strconcat (fixed_uri, "/parents/", folder_id, NULL);

	message = _gdata_service_build_message (GDATA_SERVICE (self),
						domain,
						SOUP_METHOD_DELETE,
						modified_uri,
						gdata_entry_get_etag (GDATA_ENTRY (entry)),
						TRUE);
	g_free (fixed_uri);
	g_free (modified_uri);

	/* Send the message */
	status = _gdata_service_send_message (GDATA_SERVICE (self), message, cancellable, error);

	if (status == SOUP_STATUS_NONE || status == SOUP_STATUS_CANCELLED) {
		/* Redirect error or cancelled */
		g_object_unref (message);
		return NULL;
	} else if (status != SOUP_STATUS_OK && status != SOUP_STATUS_NO_CONTENT) {
		/* Error */
		GDataServiceClass *service_klass = GDATA_SERVICE_GET_CLASS (self);
		g_assert (service_klass->parse_error_response != NULL);
		service_klass->parse_error_response (GDATA_SERVICE (self),
						     GDATA_OPERATION_DELETION,
						     status,
						     message->reason_phrase,
						     message->response_body->data,
						     message->response_body->length,
						     error);
		req_status = FALSE;
	}

	g_object_unref (message);

	if (req_status) {
		/* Remove parent link from File's Data Entry */
		gdata_entry_remove_link (GDATA_ENTRY (entry), folder_link);
		g_object_ref (entry);
		return entry;
	} else {
		return NULL;
	}
}

typedef struct {
	GDataDocumentsEntry *entry;
	GDataDocumentsFolder *folder;
} RemoveEntryFromFolderData;

static void
remove_entry_from_folder_data_free (RemoveEntryFromFolderData *data)
{
	g_object_unref (data->entry);
	g_object_unref (data->folder);
	g_slice_free (RemoveEntryFromFolderData, data);
}

static void
remove_entry_from_folder_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	GDataDocumentsService *service = GDATA_DOCUMENTS_SERVICE (source_object);
	g_autoptr(GDataDocumentsEntry) updated_entry = NULL;
	RemoveEntryFromFolderData *data = task_data;
	g_autoptr(GError) error = NULL;

	/* Remove the entry from the folder and return */
	updated_entry = gdata_documents_service_remove_entry_from_folder (service, data->entry, data->folder, cancellable, &error);
	if (error != NULL)
		g_task_return_error (task, g_steal_pointer (&error));
	else
		g_task_return_pointer (task, g_steal_pointer (&updated_entry), g_object_unref);
}

/**
 * gdata_documents_service_remove_entry_from_folder_async:
 * @self: a #GDataDocumentsService
 * @entry: the #GDataDocumentsEntry to remove from @folder
 * @folder: the #GDataDocumentsFolder to remove @entry from
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the operation is finished, or %NULL
 * @user_data: (closure): data to pass to the @callback function
 *
 * Remove the given @entry from the specified @folder. @self, @entry and @folder are all reffed when this function is called, so can safely be unreffed
 * after this function returns.
 *
 * For more details, see gdata_documents_service_remove_entry_from_folder(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_documents_service_remove_entry_from_folder_finish() to get the
 * results of the operation.
 *
 * Since: 0.8.0
 */
void
gdata_documents_service_remove_entry_from_folder_async (GDataDocumentsService *self, GDataDocumentsEntry *entry, GDataDocumentsFolder *folder,
                                                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	g_autoptr(GTask) task = NULL;
	RemoveEntryFromFolderData *data;

	g_return_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self));
	g_return_if_fail (GDATA_IS_DOCUMENTS_ENTRY (entry));
	g_return_if_fail (GDATA_IS_DOCUMENTS_FOLDER (folder));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	data = g_slice_new (RemoveEntryFromFolderData);
	data->entry = g_object_ref (entry);
	data->folder = g_object_ref (folder);

	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_documents_service_remove_entry_from_folder_async);
	g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) remove_entry_from_folder_data_free);
	g_task_run_in_thread (task, remove_entry_from_folder_thread);
}

/**
 * gdata_documents_service_remove_entry_from_folder_finish:
 * @self: a #GDataDocumentsService
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finish an asynchronous operation to remove a #GDataDocumentsEntry from a folder started with
 * gdata_documents_service_remove_entry_from_folder_async().
 *
 * Return value: (transfer full): an updated #GDataDocumentsEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.8.0
 */
GDataDocumentsEntry *
gdata_documents_service_remove_entry_from_folder_finish (GDataDocumentsService *self, GAsyncResult *async_result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);
	g_return_val_if_fail (g_task_is_valid (async_result, self), NULL);
	g_return_val_if_fail (g_async_result_is_tagged (async_result, gdata_documents_service_remove_entry_from_folder_async), NULL);

	return g_task_propagate_pointer (G_TASK (async_result), error);
}

/* NOTE: query may be NULL. */
static gchar *
_get_upload_uri_for_query_and_folder (GDataDocumentsUploadQuery *query, GDataDocumentsFolder *folder)
{
	if (query == NULL) {
		query = gdata_documents_upload_query_new ();
	}

	if (folder != NULL) {
		gdata_documents_upload_query_set_folder (query, folder);
	}

	return gdata_documents_upload_query_build_uri (query);
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
 */
gchar *
gdata_documents_service_get_upload_uri (GDataDocumentsFolder *folder)
{
	g_return_val_if_fail (folder == NULL || GDATA_IS_DOCUMENTS_FOLDER (folder), NULL);

	/* Upload URI: https://developers.google.com/drive/web/manage-uploads */
	return g_strdup ("https://www.googleapis.com/upload/drive/v2/files");
}
