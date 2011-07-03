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

#include <glib.h>
#include <unistd.h>
#include <string.h>

#include "gdata.h"
#include "common.h"

static gboolean
check_document_is_in_folder (GDataDocumentsDocument *document, GDataDocumentsFolder *folder)
{
	GList *categories;
	gboolean found_folder_category = FALSE;

	for (categories = gdata_entry_get_categories (GDATA_ENTRY (document)); categories != NULL; categories = categories->next) {
		GDataCategory *category = GDATA_CATEGORY (categories->data);

		if (strcmp (gdata_category_get_scheme (category), "http://schemas.google.com/docs/2007/folders/" DOCUMENTS_USERNAME) == 0 &&
		    strcmp (gdata_category_get_term (category), gdata_entry_get_title (GDATA_ENTRY (folder))) == 0) {
			g_assert (found_folder_category == FALSE);
			found_folder_category = TRUE;
		}
	}

	return found_folder_category;
}

static void
delete_entry (GDataDocumentsEntry *entry, GDataService *service)
{
	GDataEntry *new_entry;

	/* Re-query for the entry because its ETag may have changed over the course of the tests (or because the Documents servers like to
	 * arbitrarily change ETag values). */
	new_entry = gdata_service_query_single_entry (service, gdata_documents_service_get_primary_authorization_domain (),
	                                              gdata_entry_get_id (GDATA_ENTRY (entry)), NULL, G_OBJECT_TYPE (entry), NULL, NULL);
	g_assert (GDATA_IS_DOCUMENTS_ENTRY (new_entry));

	/* Delete the entry */
	g_assert (gdata_service_delete_entry (service, gdata_documents_service_get_primary_authorization_domain (), new_entry, NULL, NULL) == TRUE);
	g_object_unref (new_entry);
}

static void
test_authentication (void)
{
	gboolean retval;
	GDataClientLoginAuthorizer *authorizer;
	GError *error = NULL;

	/* Create an authorizer */
	authorizer = gdata_client_login_authorizer_new (CLIENT_ID, GDATA_TYPE_DOCUMENTS_SERVICE);

	g_assert_cmpstr (gdata_client_login_authorizer_get_client_id (authorizer), ==, CLIENT_ID);

	/* Log in */
	retval = gdata_client_login_authorizer_authenticate (authorizer, USERNAME, PASSWORD, NULL, &error);
	g_assert_no_error (error);
	g_assert (retval == TRUE);
	g_clear_error (&error);

	/* Check all is as it should be */
	g_assert_cmpstr (gdata_client_login_authorizer_get_username (authorizer), ==, USERNAME);
	g_assert_cmpstr (gdata_client_login_authorizer_get_password (authorizer), ==, PASSWORD);

	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	                                                     gdata_documents_service_get_primary_authorization_domain ()) == TRUE);
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	                                                     gdata_documents_service_get_spreadsheet_authorization_domain ()) == TRUE);

	g_object_unref (authorizer);
}

static void
test_authentication_async_cb (GDataClientLoginAuthorizer *authorizer, GAsyncResult *async_result, GMainLoop *main_loop)
{
	gboolean retval;
	GError *error = NULL;

	retval = gdata_client_login_authorizer_authenticate_finish (authorizer, async_result, &error);
	g_assert_no_error (error);
	g_assert (retval == TRUE);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);

	/* Check all is as it should be */
	g_assert_cmpstr (gdata_client_login_authorizer_get_username (authorizer), ==, USERNAME);
	g_assert_cmpstr (gdata_client_login_authorizer_get_password (authorizer), ==, PASSWORD);

	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	                                                     gdata_documents_service_get_primary_authorization_domain ()) == TRUE);
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	                                                     gdata_documents_service_get_spreadsheet_authorization_domain ()) == TRUE);
}

static void
test_authentication_async (void)
{
	GMainLoop *main_loop;
	GDataClientLoginAuthorizer *authorizer;

	/* Create an authorizer */
	authorizer = gdata_client_login_authorizer_new (CLIENT_ID, GDATA_TYPE_DOCUMENTS_SERVICE);

	g_assert_cmpstr (gdata_client_login_authorizer_get_client_id (authorizer), ==, CLIENT_ID);

	main_loop = g_main_loop_new (NULL, TRUE);
	gdata_client_login_authorizer_authenticate_async (authorizer, USERNAME, PASSWORD, NULL,
	                                                  (GAsyncReadyCallback) test_authentication_async_cb, main_loop);

	g_main_loop_run (main_loop);

	g_main_loop_unref (main_loop);
	g_object_unref (authorizer);
}

typedef struct {
	GDataDocumentsFolder *folder;
} TempFolderData;

static void
set_up_temp_folder (TempFolderData *data, gconstpointer service)
{
	GDataDocumentsFolder *folder;
	gchar *upload_uri;

	/* Create a folder */
	folder = gdata_documents_folder_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (folder), "Temporary Folder");

	/* Insert the folder */
	upload_uri = gdata_documents_service_get_upload_uri (NULL);
	data->folder = GDATA_DOCUMENTS_FOLDER (gdata_service_insert_entry (GDATA_SERVICE (service),
	                                                                   gdata_documents_service_get_primary_authorization_domain (),
	                                                                   upload_uri, GDATA_ENTRY (folder), NULL, NULL));
	g_assert (GDATA_IS_DOCUMENTS_FOLDER (data->folder));
	g_free (upload_uri);
	g_object_unref (folder);
}

static void
tear_down_temp_folder (TempFolderData *data, gconstpointer service)
{
	if (data->folder != NULL) {
		delete_entry (GDATA_DOCUMENTS_ENTRY (data->folder), GDATA_SERVICE (service));
		g_object_unref (data->folder);
	}
}

typedef struct {
	GDataDocumentsDocument *document;
} TempDocumentData;

static GDataDocumentsDocument *
_set_up_temp_document (GDataDocumentsEntry *entry, GDataService *service)
{
	GDataDocumentsDocument *document, *new_document;
	gchar *upload_uri;

	/* Insert the document */
	upload_uri = gdata_documents_service_get_upload_uri (NULL);
	document = GDATA_DOCUMENTS_DOCUMENT (gdata_service_insert_entry (service, gdata_documents_service_get_primary_authorization_domain (),
	                                                                 upload_uri, GDATA_ENTRY (entry), NULL, NULL));
	g_assert (GDATA_IS_DOCUMENTS_DOCUMENT (document));
	g_free (upload_uri);

	/* HACK: Query for the new document, as Google's servers appear to modify it behind our back when creating the document:
	 * http://code.google.com/a/google.com/p/apps-api-issues/issues/detail?id=2337. We have to wait a few seconds before trying this to allow the
	 * various Google servers to catch up with each other. */
	g_usleep (5 * G_USEC_PER_SEC);
	new_document = GDATA_DOCUMENTS_DOCUMENT (gdata_service_query_single_entry (service,
	                                                                           gdata_documents_service_get_primary_authorization_domain (),
	                                                                           gdata_entry_get_id (GDATA_ENTRY (document)), NULL,
	                                                                           G_OBJECT_TYPE (document), NULL, NULL));
	g_assert (GDATA_IS_DOCUMENTS_DOCUMENT (new_document));

	return new_document;
}

static void
set_up_temp_document_spreadsheet (TempDocumentData *data, gconstpointer service)
{
	GDataDocumentsSpreadsheet *document;

	/* Create a document */
	document = gdata_documents_spreadsheet_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (document), "Temporary Document (Spreadsheet)");

	data->document = _set_up_temp_document (GDATA_DOCUMENTS_ENTRY (document), GDATA_SERVICE (service));

	g_object_unref (document);
}

static void
set_up_temp_document_text (TempDocumentData *data, gconstpointer service)
{
	GDataDocumentsText *document;

	/* Create a document */
	document = gdata_documents_text_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (document), "Temporary Document (Text)");

	data->document = _set_up_temp_document (GDATA_DOCUMENTS_ENTRY (document), GDATA_SERVICE (service));

	g_object_unref (document);
}

static void
set_up_temp_document_presentation (TempDocumentData *data, gconstpointer service)
{
	GDataDocumentsPresentation *document;

	/* Create a document */
	document = gdata_documents_presentation_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (document), "Temporary Document (Presentation)");

	data->document = _set_up_temp_document (GDATA_DOCUMENTS_ENTRY (document), GDATA_SERVICE (service));

	g_object_unref (document);
}

static void
tear_down_temp_document (TempDocumentData *data, gconstpointer service)
{
	if (data->document != NULL) {
		delete_entry (GDATA_DOCUMENTS_ENTRY (data->document), GDATA_SERVICE (service));
		g_object_unref (data->document);
	}
}

static void
test_delete_folder (TempFolderData *data, gconstpointer service)
{
	gboolean success;
	GDataEntry *updated_folder;
	GError *error = NULL;

	g_assert (gdata_documents_entry_is_deleted (GDATA_DOCUMENTS_ENTRY (data->folder)) == FALSE);

	/* Delete the folder */
	success = gdata_service_delete_entry (GDATA_SERVICE (service), gdata_documents_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->folder), NULL, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);
	g_clear_error (&error);

	/* Re-query for the folder to ensure it's been deleted */
	updated_folder = gdata_service_query_single_entry (GDATA_SERVICE (service), gdata_documents_service_get_primary_authorization_domain (),
	                                                   gdata_entry_get_id (GDATA_ENTRY (data->folder)), NULL,
	                                                   GDATA_TYPE_DOCUMENTS_FOLDER, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_FOLDER (updated_folder));
	g_clear_error (&error);

	g_assert (gdata_documents_entry_is_deleted (GDATA_DOCUMENTS_ENTRY (updated_folder)) == TRUE);

	g_object_unref (updated_folder);
	g_object_unref (data->folder);
	data->folder = NULL;
}

static void
test_delete_document (TempDocumentData *data, gconstpointer service)
{
	gboolean success;
	GDataEntry *updated_document;
	GError *error = NULL;

	g_assert (gdata_documents_entry_is_deleted (GDATA_DOCUMENTS_ENTRY (data->document)) == FALSE);

	/* Delete the document */
	success = gdata_service_delete_entry (GDATA_SERVICE (service), gdata_documents_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->document), NULL, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);
	g_clear_error (&error);

	/* Re-query for the document to ensure it's been deleted */
	updated_document = gdata_service_query_single_entry (GDATA_SERVICE (service), gdata_documents_service_get_primary_authorization_domain (),
	                                                     gdata_entry_get_id (GDATA_ENTRY (data->document)), NULL,
	                                                     G_OBJECT_TYPE (data->document), NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_DOCUMENT (updated_document));
	g_clear_error (&error);

	g_assert (gdata_documents_entry_is_deleted (GDATA_DOCUMENTS_ENTRY (updated_document)) == TRUE);

	g_object_unref (updated_document);
	g_object_unref (data->document);
	data->document = NULL;
}

typedef struct {
	TempFolderData parent;
	GDataDocumentsSpreadsheet *spreadsheet_document;
	GDataDocumentsPresentation *presentation_document;
	GDataDocumentsText *text_document;
} TempDocumentsData;

static void
set_up_temp_documents (TempDocumentsData *data, gconstpointer service)
{
	GDataDocumentsEntry *document;
	gchar *upload_uri;

	upload_uri = gdata_documents_service_get_upload_uri (NULL);

	/* Create a temporary folder */
	set_up_temp_folder ((TempFolderData*) data, service);

	/* Create some temporary documents of different types */
	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_spreadsheet_new (NULL));
	gdata_entry_set_title (GDATA_ENTRY (document), "Temporary Spreadsheet");
	data->spreadsheet_document = GDATA_DOCUMENTS_SPREADSHEET (
		gdata_service_insert_entry (GDATA_SERVICE (service), gdata_documents_service_get_primary_authorization_domain (),
		                            upload_uri, GDATA_ENTRY (document), NULL, NULL)
	);
	g_assert (GDATA_IS_DOCUMENTS_SPREADSHEET (data->spreadsheet_document));
	g_object_unref (document);

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_presentation_new (NULL));
	gdata_entry_set_title (GDATA_ENTRY (document), "Temporary Presentation");
	data->presentation_document = GDATA_DOCUMENTS_PRESENTATION (
		gdata_service_insert_entry (GDATA_SERVICE (service), gdata_documents_service_get_primary_authorization_domain (),
		                            upload_uri, GDATA_ENTRY (document), NULL, NULL)
	);
	g_assert (GDATA_IS_DOCUMENTS_PRESENTATION (data->presentation_document));
	g_object_unref (document);

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_text_new (NULL));
	gdata_entry_set_title (GDATA_ENTRY (document), "Temporary Text Document");
	data->text_document = GDATA_DOCUMENTS_TEXT (
		gdata_service_insert_entry (GDATA_SERVICE (service), gdata_documents_service_get_primary_authorization_domain (),
		                            upload_uri, GDATA_ENTRY (document), NULL, NULL)
	);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (data->text_document));
	g_object_unref (document);

	g_free (upload_uri);
}

static void
tear_down_temp_documents (TempDocumentsData *data, gconstpointer service)
{
	/* Delete the documents */
	delete_entry (GDATA_DOCUMENTS_ENTRY (data->spreadsheet_document), GDATA_SERVICE (service));
	g_object_unref (data->spreadsheet_document);

	delete_entry (GDATA_DOCUMENTS_ENTRY (data->presentation_document), GDATA_SERVICE (service));
	g_object_unref (data->presentation_document);

	delete_entry (GDATA_DOCUMENTS_ENTRY (data->text_document), GDATA_SERVICE (service));
	g_object_unref (data->text_document);

	/* Delete the folder */
	tear_down_temp_folder ((TempFolderData*) data, service);
}

static void
test_query_all_documents_with_folder (TempDocumentsData *data, gconstpointer service)
{
	GDataDocumentsFeed *feed;
	GDataDocumentsQuery *query;
	GError *error = NULL;

	query = gdata_documents_query_new (NULL);
	gdata_documents_query_set_show_folders (query, TRUE);

	feed = gdata_documents_service_query_documents (GDATA_DOCUMENTS_SERVICE (service), query, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));

	/* TODO: Test the feed */

	g_clear_error (&error);
	g_object_unref (feed);
	g_object_unref (query);
}

static void
test_query_all_documents (TempDocumentsData *data, gconstpointer service)
{
	GDataDocumentsFeed *feed;
	GError *error = NULL;

	feed = gdata_documents_service_query_documents (GDATA_DOCUMENTS_SERVICE (service), NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* TODO: check entries and feed properties */

	g_object_unref (feed);
}

typedef struct {
	TempDocumentsData parent;
	GMainLoop *main_loop;
} TempDocumentsAsyncData;

static void
set_up_temp_documents_async (TempDocumentsAsyncData *data, gconstpointer service)
{
	set_up_temp_documents ((TempDocumentsData*) data, service);
	data->main_loop = g_main_loop_new (NULL, FALSE);
}

static void
tear_down_temp_documents_async (TempDocumentsAsyncData *data, gconstpointer service)
{
	g_main_loop_unref (data->main_loop);
	tear_down_temp_documents ((TempDocumentsData*) data, service);
}

static void
test_query_all_documents_async_cb (GDataService *service, GAsyncResult *async_result, TempDocumentsAsyncData *data)
{
	GDataDocumentsFeed *feed;
	GError *error = NULL;

	feed = GDATA_DOCUMENTS_FEED (gdata_service_query_finish (GDATA_SERVICE (service), async_result, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* TODO: Tests? */

	g_main_loop_quit (data->main_loop);
	g_object_unref (feed);
}

static void
test_query_all_documents_async (TempDocumentsAsyncData *data, gconstpointer service)
{
	gdata_documents_service_query_documents_async (GDATA_DOCUMENTS_SERVICE (service), NULL, NULL, NULL, NULL,
	                                               NULL, (GAsyncReadyCallback) test_query_all_documents_async_cb, data);

	g_main_loop_run (data->main_loop);
}

static void
test_query_all_documents_async_progress_closure (TempDocumentsAsyncData *documents_data, gconstpointer service)
{
	GDataAsyncProgressClosure *data = g_slice_new0 (GDataAsyncProgressClosure);

	data->main_loop = g_main_loop_new (NULL, TRUE);

	gdata_documents_service_query_documents_async (GDATA_DOCUMENTS_SERVICE (service), NULL, NULL,
	                                               (GDataQueryProgressCallback) gdata_test_async_progress_callback,
	                                               data, (GDestroyNotify) gdata_test_async_progress_closure_free,
	                                               (GAsyncReadyCallback) gdata_test_async_progress_finish_callback, data);

	g_main_loop_run (data->main_loop);
	g_main_loop_unref (data->main_loop);

	/* Check that both callbacks were called exactly once */
	g_assert_cmpuint (data->progress_destroy_notify_count, ==, 1);
	g_assert_cmpuint (data->async_ready_notify_count, ==, 1);

	g_slice_free (GDataAsyncProgressClosure, data);
}

typedef struct {
	GDataDocumentsFolder *folder;
	GDataDocumentsDocument *new_document;
} UploadDocumentData;

static void
set_up_upload_document (UploadDocumentData *data, gconstpointer service)
{
	data->folder = NULL;
	data->new_document = NULL;
}

static void
set_up_upload_document_with_folder (UploadDocumentData *data, gconstpointer service)
{
	GDataDocumentsFolder *folder;
	gchar *upload_uri;

	/* Set up the structure */
	set_up_upload_document (data, service);

	/* Create a folder */
	folder = gdata_documents_folder_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (folder), "Temporary Folder for Uploading Documents");

	/* Insert the folder */
	upload_uri = gdata_documents_service_get_upload_uri (NULL);
	data->folder = GDATA_DOCUMENTS_FOLDER (gdata_service_insert_entry (GDATA_SERVICE (service),
	                                                                   gdata_documents_service_get_primary_authorization_domain (),
	                                                                   upload_uri, GDATA_ENTRY (folder), NULL, NULL));
	g_assert (GDATA_IS_DOCUMENTS_FOLDER (data->folder));
	g_free (upload_uri);
	g_object_unref (folder);
}

static void
tear_down_upload_document (UploadDocumentData *data, gconstpointer service)
{
	/* Delete the new file */
	if (data->new_document != NULL) {
		/* HACK: Query for the new document, as Google's servers appear to modify it behind our back if we don't upload both metadata and data
		 * when creating the document: http://code.google.com/a/google.com/p/apps-api-issues/issues/detail?id=2337. We have to wait a few
		 * seconds before trying this to allow the various Google servers to catch up with each other. */
		g_usleep (5 * G_USEC_PER_SEC);
		delete_entry (GDATA_DOCUMENTS_ENTRY (data->new_document), GDATA_SERVICE (service));
		g_object_unref (data->new_document);
	}

	/* Delete the folder */
	if (data->folder != NULL) {
		delete_entry (GDATA_DOCUMENTS_ENTRY (data->folder), GDATA_SERVICE (service));
		g_object_unref (data->folder);
	}
}

static void
test_upload_metadata (UploadDocumentData *data, gconstpointer service)
{
	GDataDocumentsEntry *document;
	GError *error = NULL;
	gchar *upload_uri;

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_spreadsheet_new (NULL));
	gdata_entry_set_title (GDATA_ENTRY (document), "myNewSpreadsheet");

	/* Insert the document */
	upload_uri = gdata_documents_service_get_upload_uri (NULL);
	data->new_document = GDATA_DOCUMENTS_DOCUMENT (gdata_service_insert_entry (GDATA_SERVICE (service),
	                                                                           gdata_documents_service_get_primary_authorization_domain (),
	                                                                           upload_uri, GDATA_ENTRY (document), NULL, &error));
	g_free (upload_uri);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_SPREADSHEET (data->new_document));

	g_clear_error (&error);
	g_object_unref (document);
}

static void
test_upload_metadata_file (UploadDocumentData *data, gconstpointer service)
{
	GDataDocumentsDocument *document;
	GFile *document_file;
	GFileInfo *file_info;
	GDataUploadStream *upload_stream;
	GFileInputStream *file_stream;
	GError *error = NULL;

	document_file = g_file_new_for_path (TEST_FILE_DIR "test.odt");
	file_info = g_file_query_info (document_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
	                               G_FILE_QUERY_INFO_NONE, NULL, &error);
	g_assert_no_error (error);

	document = GDATA_DOCUMENTS_DOCUMENT (gdata_documents_text_new (NULL));
	gdata_entry_set_title (GDATA_ENTRY (document), "upload_metadata_file");

	/* Prepare the upload stream */
	upload_stream = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), document, g_file_info_get_display_name (file_info),
	                                                         g_file_info_get_content_type (file_info), NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_UPLOAD_STREAM (upload_stream));

	g_object_unref (file_info);

	/* Open the file */
	file_stream = g_file_read (document_file, NULL, &error);
	g_assert_no_error (error);

	/* Upload the document */
	g_output_stream_splice (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (file_stream),
	                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &error);
	g_assert_no_error (error);

	/* Finish the upload */
	data->new_document = gdata_documents_service_finish_upload (GDATA_DOCUMENTS_SERVICE (service), upload_stream, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (data->new_document));

	/* Verify the uploaded document is the same as the original */
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (document)), ==, gdata_entry_get_title (GDATA_ENTRY (data->new_document)));

	g_clear_error (&error);
	g_object_unref (upload_stream);
	g_object_unref (file_stream);
	g_object_unref (document_file);
	g_object_unref (document);
}

static void
test_upload_file_get_entry (UploadDocumentData *data, gconstpointer service)
{
	GDataEntry *new_presentation;
	GDataUploadStream *upload_stream;
	GFileInputStream *file_stream;
	GFile *document_file;
	GFileInfo *file_info;
	GError *error = NULL;

	g_assert (service != NULL);

	document_file = g_file_new_for_path (TEST_FILE_DIR "test.ppt");
	file_info = g_file_query_info (document_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
	                               G_FILE_QUERY_INFO_NONE, NULL, &error);
	g_assert_no_error (error);

	/* Prepare the upload stream */
	upload_stream = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), NULL, g_file_info_get_display_name (file_info),
	                                                         g_file_info_get_content_type (file_info), NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_UPLOAD_STREAM (upload_stream));

	g_object_unref (file_info);

	/* Open the file */
	file_stream = g_file_read (document_file, NULL, &error);
	g_assert_no_error (error);

	g_object_unref (document_file);

	/* Upload the document */
	g_output_stream_splice (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (file_stream),
	                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &error);
	g_assert_no_error (error);

	/* Finish the upload */
	data->new_document = gdata_documents_service_finish_upload (GDATA_DOCUMENTS_SERVICE (service), upload_stream, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_PRESENTATION (data->new_document));

	g_object_unref (file_stream);
	g_object_unref (upload_stream);

	/* Get the entry on the server */
	new_presentation = gdata_service_query_single_entry (GDATA_SERVICE (service), gdata_documents_service_get_primary_authorization_domain (),
	                                                     gdata_entry_get_id (GDATA_ENTRY (data->new_document)), NULL,
	                                                     GDATA_TYPE_DOCUMENTS_PRESENTATION, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_PRESENTATION (new_presentation));

	/* Verify that the entry is correct (mangled version of the file's display name) */
	g_assert_cmpstr (gdata_entry_get_title (new_presentation), ==, "test");

	g_clear_error (&error);
	g_object_unref (new_presentation);
}

typedef struct {
	GDataDocumentsFolder *folder;
	GDataDocumentsDocument *document;
} FoldersData;

static void
set_up_folders (FoldersData *data, GDataDocumentsService *service, gboolean initially_in_folder)
{
	GDataDocumentsFolder *folder;
	GDataDocumentsDocument *document, *new_document;
	GDataUploadStream *upload_stream;
	GFileInputStream *file_stream;
	GFile *document_file;
	GFileInfo *file_info;
	gchar *upload_uri;
	GError *error = NULL;

	/* Create a new folder for the tests */
	folder = gdata_documents_folder_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (folder), "add_file_folder_move_folder");

	/* Insert the folder */
	upload_uri = gdata_documents_service_get_upload_uri (NULL);
	data->folder = GDATA_DOCUMENTS_FOLDER (gdata_service_insert_entry (GDATA_SERVICE (service),
	                                                                   gdata_documents_service_get_primary_authorization_domain (),
	                                                                   upload_uri, GDATA_ENTRY (folder), NULL, &error));
	g_free (upload_uri);

	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_FOLDER (data->folder));

	g_object_unref (folder);

	/* Create a new file for the tests */
	document_file = g_file_new_for_path (TEST_FILE_DIR "test.odt");
	document = GDATA_DOCUMENTS_DOCUMENT (gdata_documents_text_new (NULL));
	gdata_entry_set_title (GDATA_ENTRY (document), "add_file_folder_move_text");

	file_info = g_file_query_info (document_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
	                               G_FILE_QUERY_INFO_NONE, NULL, &error);
	g_assert_no_error (error);

	/* Prepare the upload stream */
	upload_stream = gdata_documents_service_upload_document (service, document, g_file_info_get_display_name (file_info),
	                                                         g_file_info_get_content_type (file_info),
	                                                         (initially_in_folder == TRUE) ? data->folder : NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_UPLOAD_STREAM (upload_stream));

	g_object_unref (file_info);
	g_object_unref (document);

	/* Open the file */
	file_stream = g_file_read (document_file, NULL, &error);
	g_assert_no_error (error);

	g_object_unref (document_file);

	/* Upload the document */
	g_output_stream_splice (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (file_stream),
	                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &error);
	g_assert_no_error (error);

	/* Finish the upload */
	new_document = gdata_documents_service_finish_upload (service, upload_stream, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (new_document));

	g_object_unref (upload_stream);
	g_object_unref (file_stream);

	/* HACK: Query for the new document, as Google's servers appear to modify it behind our back when creating the document:
	 * http://code.google.com/a/google.com/p/apps-api-issues/issues/detail?id=2337. We have to wait a few seconds before trying this to allow the
	 * various Google servers to catch up with each other. */
	g_usleep (5 * G_USEC_PER_SEC);
	data->document = GDATA_DOCUMENTS_DOCUMENT (gdata_service_query_single_entry (GDATA_SERVICE (service),
	                                                                             gdata_documents_service_get_primary_authorization_domain (),
	                                                                             gdata_entry_get_id (GDATA_ENTRY (new_document)), NULL,
	                                                                             G_OBJECT_TYPE (new_document), NULL, NULL));
	g_assert (GDATA_IS_DOCUMENTS_TEXT (data->document));
}

static void
set_up_folders_add_to_folder (FoldersData *data, gconstpointer service)
{
	set_up_folders (data, GDATA_DOCUMENTS_SERVICE (service), FALSE);
}

static void
tear_down_folders_add_to_folder (FoldersData *data, gconstpointer service)
{
	delete_entry (GDATA_DOCUMENTS_ENTRY (data->document), GDATA_SERVICE (service));
	g_object_unref (data->document);

	delete_entry (GDATA_DOCUMENTS_ENTRY (data->folder), GDATA_SERVICE (service));
	g_object_unref (data->folder);
}

static void
test_folders_add_to_folder (FoldersData *data, gconstpointer service)
{
	GDataDocumentsDocument *new_document;
	GError *error = NULL;

	g_assert (service != NULL);

	/* Add the document to the folder */
	new_document = GDATA_DOCUMENTS_DOCUMENT (gdata_documents_service_add_entry_to_folder (GDATA_DOCUMENTS_SERVICE (service),
	                                                                                      GDATA_DOCUMENTS_ENTRY (data->document),
	                                                                                      data->folder, NULL, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (new_document));

	/* Check it's still the same document */
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (new_document)), ==, gdata_entry_get_title (GDATA_ENTRY (data->document)));
	g_assert (check_document_is_in_folder (new_document, data->folder) == TRUE);

	g_object_unref (new_document);
}

typedef struct {
	FoldersData data;
	GMainLoop *main_loop;
} FoldersAsyncData;

static void
set_up_folders_add_to_folder_async (FoldersAsyncData *data, gconstpointer service)
{
	set_up_folders_add_to_folder ((FoldersData*) data, service);
	data->main_loop = g_main_loop_new (NULL, TRUE);
}

static void
tear_down_folders_add_to_folder_async (FoldersAsyncData *data, gconstpointer service)
{
	g_main_loop_unref (data->main_loop);
	tear_down_folders_add_to_folder ((FoldersData*) data, service);
}

static void
test_folders_add_to_folder_async_cb (GDataDocumentsService *service, GAsyncResult *async_result, FoldersAsyncData *data)
{
	GDataDocumentsEntry *entry;
	GError *error = NULL;

	entry = gdata_documents_service_add_entry_to_folder_finish (service, async_result, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_ENTRY (entry));
	g_clear_error (&error);

	/* Check it's still the same document */
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (entry)), ==, gdata_entry_get_title (GDATA_ENTRY (data->data.document)));
	g_assert (check_document_is_in_folder (GDATA_DOCUMENTS_DOCUMENT (entry), data->data.folder) == TRUE);

	g_object_unref (entry);

	g_main_loop_quit (data->main_loop);
}

static void
test_folders_add_to_folder_async (FoldersAsyncData *data, gconstpointer service)
{
	/* Add the document to the folder asynchronously */
	gdata_documents_service_add_entry_to_folder_async (GDATA_DOCUMENTS_SERVICE (service), GDATA_DOCUMENTS_ENTRY (data->data.document),
	                                                   data->data.folder, NULL, (GAsyncReadyCallback) test_folders_add_to_folder_async_cb, data);
	g_main_loop_run (data->main_loop);
}

static void
test_folders_add_to_folder_cancellation_cb (GDataDocumentsService *service, GAsyncResult *async_result, FoldersAsyncData *data)
{
	GDataDocumentsEntry *entry;
	GError *error = NULL;

	entry = gdata_documents_service_add_entry_to_folder_finish (service, async_result, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_assert (entry == NULL);
	g_clear_error (&error);

	g_main_loop_quit (data->main_loop);
}

static gboolean
test_folders_add_to_folder_cancellation_cancel_cb (GCancellable *cancellable)
{
	g_cancellable_cancel (cancellable);
	return FALSE;
}

static void
test_folders_add_to_folder_cancellation (FoldersAsyncData *data, gconstpointer service)
{
	GCancellable *cancellable = g_cancellable_new ();
	g_timeout_add (1, (GSourceFunc) test_folders_add_to_folder_cancellation_cancel_cb, cancellable);

	/* Add the document to the folder asynchronously and cancel the operation after a few milliseconds */
	gdata_documents_service_add_entry_to_folder_async (GDATA_DOCUMENTS_SERVICE (service), GDATA_DOCUMENTS_ENTRY (data->data.document),
	                                                   data->data.folder, cancellable,
	                                                   (GAsyncReadyCallback) test_folders_add_to_folder_cancellation_cb, data);
	g_main_loop_run (data->main_loop);

	g_object_unref (cancellable);
}

static void
set_up_folders_remove_from_folder (FoldersData *data, gconstpointer service)
{
	set_up_folders (data, GDATA_DOCUMENTS_SERVICE (service), TRUE);
}

static void
tear_down_folders_remove_from_folder (FoldersData *data, gconstpointer service)
{
	tear_down_folders_add_to_folder (data, service);
}

static void
test_folders_remove_from_folder (FoldersData *data, gconstpointer service)
{
	GDataDocumentsDocument *new_document;
	GError *error = NULL;

	g_assert (service != NULL);

	/* Remove the document from the folder */
	new_document = GDATA_DOCUMENTS_DOCUMENT (gdata_documents_service_remove_entry_from_folder (GDATA_DOCUMENTS_SERVICE (service),
	                                                                                           GDATA_DOCUMENTS_ENTRY (data->document),
	                                                                                           data->folder, NULL, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (new_document));

	/* Check it's still the same document */
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (new_document)), ==, gdata_entry_get_title (GDATA_ENTRY (data->document)));
	g_assert (check_document_is_in_folder (new_document, data->folder) == FALSE);

	g_object_unref (new_document);
}

static void
set_up_folders_remove_from_folder_async (FoldersAsyncData *data, gconstpointer service)
{
	set_up_folders_remove_from_folder ((FoldersData*) data, service);
	data->main_loop = g_main_loop_new (NULL, TRUE);
}

static void
tear_down_folders_remove_from_folder_async (FoldersAsyncData *data, gconstpointer service)
{
	g_main_loop_unref (data->main_loop);
	tear_down_folders_remove_from_folder ((FoldersData*) data, service);
}

static void
test_folders_remove_from_folder_async_cb (GDataDocumentsService *service, GAsyncResult *async_result, FoldersAsyncData *data)
{
	GDataDocumentsEntry *entry;
	GError *error = NULL;

	entry = gdata_documents_service_remove_entry_from_folder_finish (service, async_result, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_ENTRY (entry));
	g_clear_error (&error);

	/* Check it's still the same document */
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (entry)), ==, gdata_entry_get_title (GDATA_ENTRY (data->data.document)));
	g_assert (check_document_is_in_folder (GDATA_DOCUMENTS_DOCUMENT (entry), data->data.folder) == FALSE);

	g_object_unref (entry);

	g_main_loop_quit (data->main_loop);
}

static void
test_folders_remove_from_folder_async (FoldersAsyncData *data, gconstpointer service)
{
	/* Remove the document from the folder asynchronously */
	gdata_documents_service_remove_entry_from_folder_async (GDATA_DOCUMENTS_SERVICE (service), GDATA_DOCUMENTS_ENTRY (data->data.document),
	                                                        data->data.folder, NULL,
	                                                        (GAsyncReadyCallback) test_folders_remove_from_folder_async_cb, data);
	g_main_loop_run (data->main_loop);
}

static void
test_folders_remove_from_folder_cancellation_cb (GDataDocumentsService *service, GAsyncResult *async_result, FoldersAsyncData *data)
{
	GDataDocumentsEntry *entry;
	GError *error = NULL;

	entry = gdata_documents_service_remove_entry_from_folder_finish (service, async_result, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_assert (entry == NULL);
	g_clear_error (&error);

	g_main_loop_quit (data->main_loop);
}

static gboolean
test_folders_remove_from_folder_cancellation_cancel_cb (GCancellable *cancellable)
{
	g_cancellable_cancel (cancellable);
	return FALSE;
}

static void
test_folders_remove_from_folder_cancellation (FoldersAsyncData *data, gconstpointer service)
{
	GCancellable *cancellable = g_cancellable_new ();
	g_timeout_add (1, (GSourceFunc) test_folders_remove_from_folder_cancellation_cancel_cb, cancellable);

	/* Remove the document from the folder asynchronously and cancel the operation after a few milliseconds */
	gdata_documents_service_remove_entry_from_folder_async (GDATA_DOCUMENTS_SERVICE (service), GDATA_DOCUMENTS_ENTRY (data->data.document),
	                                                        data->data.folder, cancellable,
	                                                        (GAsyncReadyCallback) test_folders_remove_from_folder_cancellation_cb, data);
	g_main_loop_run (data->main_loop);

	g_object_unref (cancellable);
}

static void
test_upload_file_metadata_in_new_folder (UploadDocumentData *data, gconstpointer service)
{
	GDataDocumentsDocument *document;
	GDataUploadStream *upload_stream;
	GFileInputStream *file_stream;
	GFile *document_file;
	GFileInfo *file_info;
	GError *error = NULL;

	/* Prepare the file */
	document_file = g_file_new_for_path (TEST_FILE_DIR "test.odt");
	document = GDATA_DOCUMENTS_DOCUMENT (gdata_documents_text_new (NULL));
	gdata_entry_set_title (GDATA_ENTRY (document), "upload_file_metadata_in_new_folder_text");

	file_info = g_file_query_info (document_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
	                               G_FILE_QUERY_INFO_NONE, NULL, &error);
	g_assert_no_error (error);

	/* Prepare the upload stream */
	upload_stream = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), document, g_file_info_get_display_name (file_info),
	                                                         g_file_info_get_content_type (file_info), data->folder, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_UPLOAD_STREAM (upload_stream));

	g_object_unref (file_info);

	/* Open the file */
	file_stream = g_file_read (document_file, NULL, &error);
	g_assert_no_error (error);

	g_object_unref (document_file);

	/* Upload the document into the new folder */
	g_output_stream_splice (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (file_stream),
	                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &error);
	g_assert_no_error (error);

	/* Finish the upload */
	data->new_document = gdata_documents_service_finish_upload (GDATA_DOCUMENTS_SERVICE (service), upload_stream, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (data->new_document));

	g_object_unref (upload_stream);
	g_object_unref (file_stream);

	/* Check for success */
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (data->new_document)), ==, gdata_entry_get_title (GDATA_ENTRY (document)));
	g_assert (check_document_is_in_folder (data->new_document, data->folder) == TRUE);

	g_clear_error (&error);
	g_object_unref (document);
}

static void
test_update_metadata (TempDocumentData *data, gconstpointer service)
{
	GDataDocumentsEntry *updated_document;
	gchar *original_title;
	GError *error = NULL;

	/* Change the document title */
	original_title = g_strdup (gdata_entry_get_title (GDATA_ENTRY (data->document)));
	gdata_entry_set_title (GDATA_ENTRY (data->document), "Updated Title for Metadata Only");

	/* Update the document */
	updated_document = GDATA_DOCUMENTS_ENTRY (gdata_service_update_entry (GDATA_SERVICE (service),
	                                                                      gdata_documents_service_get_primary_authorization_domain (),
	                                                                      GDATA_ENTRY (data->document), NULL, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (updated_document));
	g_clear_error (&error);

	/* Check for success */
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (updated_document)), ==, gdata_entry_get_title (GDATA_ENTRY (data->document)));
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (updated_document)), !=, original_title);

	g_free (original_title);
	g_object_unref (updated_document);
}

static void
test_update_metadata_file (TempDocumentData *data, gconstpointer service)
{
	GDataDocumentsDocument *updated_document;
	GDataUploadStream *upload_stream;
	GFileInputStream *file_stream;
	GFile *updated_document_file;
	GFileInfo *file_info;
	gchar *original_title;
	GError *error = NULL;

	/* Change the title of the document */
	original_title = g_strdup (gdata_entry_get_title (GDATA_ENTRY (data->document)));
	gdata_entry_set_title (GDATA_ENTRY (data->document), "Updated Title for Metadata and File");

	/* Prepare the updated file */
	updated_document_file = g_file_new_for_path (TEST_FILE_DIR "test_updated.odt");

	file_info = g_file_query_info (updated_document_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
	                               G_FILE_QUERY_INFO_NONE, NULL, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	/* Prepare the upload stream */
	upload_stream = gdata_documents_service_update_document (GDATA_DOCUMENTS_SERVICE (service), data->document,
	                                                         g_file_info_get_display_name (file_info), g_file_info_get_content_type (file_info),
	                                                         NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_UPLOAD_STREAM (upload_stream));
	g_clear_error (&error);

	g_object_unref (file_info);

	/* Open the file */
	file_stream = g_file_read (updated_document_file, NULL, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	g_object_unref (updated_document_file);

	/* Upload the updated document */
	g_output_stream_splice (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (file_stream),
	                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	/* Finish the upload */
	updated_document = gdata_documents_service_finish_upload (GDATA_DOCUMENTS_SERVICE (service), upload_stream, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (updated_document));
	g_clear_error (&error);

	g_object_unref (upload_stream);
	g_object_unref (file_stream);

	/* Check for success */
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (updated_document)), ==, gdata_entry_get_title (GDATA_ENTRY (data->document)));
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (updated_document)), !=, original_title);

	g_free (original_title);
	g_object_unref (updated_document);
}

static void
test_update_file (TempDocumentData *data, gconstpointer service)
{
	GDataDocumentsDocument *updated_document;
	GDataUploadStream *upload_stream;
	GFileInputStream *file_stream;
	GFile *document_file;
	GFileInfo *file_info;
	GError *error = NULL;

	/* Get the file info for the updated document */
	document_file = g_file_new_for_path (TEST_FILE_DIR "test_updated_file.ppt");
	file_info = g_file_query_info (document_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
	                               G_FILE_QUERY_INFO_NONE, NULL, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	/* Prepare the upload stream */
	upload_stream = gdata_documents_service_update_document (GDATA_DOCUMENTS_SERVICE (service), data->document,
	                                                         g_file_info_get_display_name (file_info), g_file_info_get_content_type (file_info),
	                                                         NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_UPLOAD_STREAM (upload_stream));
	g_clear_error (&error);

	g_object_unref (file_info);

	/* Open the file */
	file_stream = g_file_read (document_file, NULL, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	g_object_unref (document_file);

	/* Upload the document */
	g_output_stream_splice (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (file_stream),
	                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	g_object_unref (file_stream);

	/* Finish the upload */
	updated_document = gdata_documents_service_finish_upload (GDATA_DOCUMENTS_SERVICE (service), upload_stream, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_PRESENTATION (updated_document));
	g_clear_error (&error);

	g_object_unref (upload_stream);

	/* Check for success */
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (updated_document)), ==, gdata_entry_get_title (GDATA_ENTRY (data->document)));

	g_object_unref (updated_document);
}

static void
_test_download_document (GDataDocumentsDocument *document, GDataService *service)
{
	GDataDownloadStream *download_stream;
	GFileOutputStream *output_stream;
	GFile *destination_file;
	GFileInfo *file_info;
	const gchar *destination_file_extension;
	gchar *destination_file_name, *destination_file_path;
	GError *error = NULL;

	if (GDATA_IS_DOCUMENTS_PRESENTATION (document)) {
		/* Presentation */
		destination_file_extension = "odp";
		download_stream = gdata_documents_document_download (document, GDATA_DOCUMENTS_SERVICE (service),
		                                                     GDATA_DOCUMENTS_PRESENTATION_PPT, NULL, &error);
	} else if (GDATA_IS_DOCUMENTS_SPREADSHEET (document)) {
		/* Spreadsheet */
		destination_file_extension = "ods";
		download_stream = gdata_documents_document_download (document, GDATA_DOCUMENTS_SERVICE (service),
		                                                     GDATA_DOCUMENTS_SPREADSHEET_ODS, NULL, &error);
	} else if (GDATA_IS_DOCUMENTS_TEXT (document)) {
		/* Text document */
		destination_file_extension = "odt";
		download_stream = gdata_documents_document_download (document, GDATA_DOCUMENTS_SERVICE (service),
		                                                     GDATA_DOCUMENTS_TEXT_ODT, NULL, &error);
	} else {
		/* Error! */
		g_assert_not_reached ();
	}

	g_assert_no_error (error);
	g_clear_error (&error);

	/* Find a destination file */
	destination_file_name = g_strdup_printf ("%s.%s", gdata_documents_entry_get_document_id (GDATA_DOCUMENTS_ENTRY (document)),
	                                         destination_file_extension);
	destination_file_path = g_build_filename (g_get_tmp_dir (), destination_file_name, NULL);
	g_free (destination_file_name);

	destination_file = g_file_new_for_path (destination_file_path);
	g_free (destination_file_path);

	/* Download the file */
	output_stream = g_file_replace (destination_file, NULL, FALSE, G_FILE_CREATE_REPLACE_DESTINATION, NULL, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	g_output_stream_splice (G_OUTPUT_STREAM (output_stream), G_INPUT_STREAM (download_stream),
	                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	g_object_unref (output_stream);
	g_object_unref (download_stream);

	/* Check the file size.
	 * Checking the content types turns out to be quite involved, and not worth doing, as it depends on the local user's content type DB. */
	file_info = g_file_query_info (destination_file, G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE, NULL, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	g_assert_cmpint (g_file_info_get_size (file_info), >, 0);

	g_object_unref (file_info);

	/* Delete the file (shouldn't cause the test to fail if this fails) */
	g_file_delete (destination_file, NULL, NULL);
	g_object_unref (destination_file);
}

static void
test_download_document (TempDocumentsData *data, gconstpointer service)
{
	_test_download_document (GDATA_DOCUMENTS_DOCUMENT (data->spreadsheet_document), GDATA_SERVICE (service));
	_test_download_document (GDATA_DOCUMENTS_DOCUMENT (data->presentation_document), GDATA_SERVICE (service));
	_test_download_document (GDATA_DOCUMENTS_DOCUMENT (data->text_document), GDATA_SERVICE (service));
}

static void
test_access_rule_insert (TempDocumentData *data, gconstpointer service)
{
	GDataAccessRule *access_rule, *new_access_rule;
	GDataLink *_link;
	GError *error = NULL;

	/* New access rule */
	access_rule = gdata_access_rule_new (NULL);
	gdata_access_rule_set_role (access_rule, GDATA_DOCUMENTS_ACCESS_ROLE_WRITER);
	gdata_access_rule_set_scope (access_rule, GDATA_ACCESS_SCOPE_USER, "libgdata.test@gmail.com");

	/* Set access rules */
	_link = gdata_entry_look_up_link (GDATA_ENTRY (data->document), GDATA_LINK_ACCESS_CONTROL_LIST);
	g_assert (_link != NULL);

	new_access_rule = GDATA_ACCESS_RULE (gdata_service_insert_entry (GDATA_SERVICE (service),
	                                                                 gdata_documents_service_get_primary_authorization_domain (),
	                                                                 gdata_link_get_uri (_link),
	                                                                 GDATA_ENTRY (access_rule), NULL, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_ACCESS_RULE (new_access_rule));
	g_clear_error (&error);

	/* TODO: Check if everything is as it should be */

	g_object_unref (access_rule);
	g_object_unref (new_access_rule);
}

static void
test_query_etag (void)
{
	GDataDocumentsQuery *query = gdata_documents_query_new (NULL);

	/* Test that setting any property will unset the ETag */
	g_test_bug ("613529");

#define CHECK_ETAG(C) \
	gdata_query_set_etag (GDATA_QUERY (query), "foobar");		\
	(C);								\
	g_assert (gdata_query_get_etag (GDATA_QUERY (query)) == NULL);

	CHECK_ETAG (gdata_documents_query_set_show_deleted (query, FALSE))
	CHECK_ETAG (gdata_documents_query_set_show_folders (query, TRUE))
	CHECK_ETAG (gdata_documents_query_set_folder_id (query, "this-is-an-id"))
	CHECK_ETAG (gdata_documents_query_set_title (query, "Title", FALSE))
	CHECK_ETAG (gdata_documents_query_add_reader (query, "foo@example.com"))
	CHECK_ETAG (gdata_documents_query_add_collaborator (query, "foo@example.com"))

#undef CHECK_ETAG

	g_object_unref (query);
}

static void
test_batch (gconstpointer service)
{
	GDataBatchOperation *operation;
	GDataService *service2;
	GDataDocumentsText *doc, *doc2, *doc3;
	GDataEntry *inserted_entry, *inserted_entry_updated, *inserted_entry2, *inserted_entry2_updated, *inserted_entry3;
	gchar *feed_uri;
	guint op_id, op_id2, op_id3;
	GError *error = NULL, *entry_error = NULL;

	/* Here we hardcode the feed URI, but it should really be extracted from a document feed, as the GDATA_LINK_BATCH link */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_documents_service_get_primary_authorization_domain (),
	                                              "https://docs.google.com/feeds/documents/private/full/batch");

	/* Check the properties of the operation */
	g_assert (gdata_batch_operation_get_service (operation) == service);
	g_assert_cmpstr (gdata_batch_operation_get_feed_uri (operation), ==, "https://docs.google.com/feeds/documents/private/full/batch");

	g_object_get (operation,
	              "service", &service2,
	              "feed-uri", &feed_uri,
	              NULL);

	g_assert (service2 == service);
	g_assert_cmpstr (feed_uri, ==, "https://docs.google.com/feeds/documents/private/full/batch");

	g_object_unref (service2);
	g_free (feed_uri);

	/* Run a singleton batch operation to insert a new entry */
	doc = gdata_documents_text_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (doc), "My First Document");

	gdata_test_batch_operation_insertion (operation, GDATA_ENTRY (doc), &inserted_entry, NULL);
	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);
	g_object_unref (doc);

	/* Run another batch operation to insert another entry and query the previous one */
	doc2 = gdata_documents_text_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (doc2), "I'm a poet and I didn't know it");

	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_documents_service_get_primary_authorization_domain (),
	                                              "https://docs.google.com/feeds/documents/private/full/batch");
	op_id = gdata_test_batch_operation_insertion (operation, GDATA_ENTRY (doc2), &inserted_entry2, NULL);
	op_id2 = gdata_test_batch_operation_query (operation, gdata_entry_get_id (inserted_entry), GDATA_TYPE_DOCUMENTS_TEXT, inserted_entry, NULL,
	                                           NULL);
	g_assert_cmpuint (op_id, !=, op_id2);

	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);
	g_object_unref (doc2);

	/* Run another batch operation to query one of the entries we just created, since it seems that the ETags for documents change for no
	 * apparent reason when you're not looking. */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_documents_service_get_primary_authorization_domain (),
	                                              "https://docs.google.com/feeds/documents/private/full/batch");
	gdata_test_batch_operation_query (operation, gdata_entry_get_id (inserted_entry), GDATA_TYPE_DOCUMENTS_TEXT, inserted_entry,
	                                  &inserted_entry_updated, NULL);

	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);
	g_object_unref (inserted_entry);

	/* Run another batch operation to query the other entry we just created. It would be sensible to batch this query together with the previous
	 * one, seeing as we're testing _batch_ functionality. Funnily enough, the combination of two idempotent operations changes the ETags and
	 * makes the whole effort worthless. */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_documents_service_get_primary_authorization_domain (),
	                                              "https://docs.google.com/feeds/documents/private/full/batch");
	gdata_test_batch_operation_query (operation, gdata_entry_get_id (inserted_entry2), GDATA_TYPE_DOCUMENTS_TEXT, inserted_entry2,
	                                  &inserted_entry2_updated, NULL);

	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);

	/* Run another batch operation to delete the first entry and a fictitious one to test error handling, and update the second entry */
	gdata_entry_set_title (inserted_entry2_updated, "War & Peace");
	doc3 = gdata_documents_text_new ("foobar");

	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_documents_service_get_primary_authorization_domain (),
	                                              "https://docs.google.com/feeds/documents/private/full/batch");
	op_id = gdata_test_batch_operation_deletion (operation, inserted_entry_updated, NULL);
	op_id2 = gdata_test_batch_operation_deletion (operation, GDATA_ENTRY (doc3), &entry_error);
	op_id3 = gdata_test_batch_operation_update (operation, inserted_entry2_updated, &inserted_entry3, NULL);
	g_assert_cmpuint (op_id, !=, op_id2);
	g_assert_cmpuint (op_id, !=, op_id3);
	g_assert_cmpuint (op_id2, !=, op_id3);

	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_assert_error (entry_error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR);

	g_clear_error (&error);
	g_clear_error (&entry_error);
	g_object_unref (operation);
	g_object_unref (inserted_entry_updated);
	g_object_unref (inserted_entry2_updated);
	g_object_unref (doc3);

	/* Run another batch operation to update the second entry with the wrong ETag (i.e. pass the old version of the entry to the batch operation
	 * to test error handling */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_documents_service_get_primary_authorization_domain (),
	                                              "https://docs.google.com/feeds/documents/private/full/batch");
	gdata_test_batch_operation_update (operation, inserted_entry2, NULL, &entry_error);
	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_assert_error (entry_error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_CONFLICT);

	g_clear_error (&error);
	g_clear_error (&entry_error);
	g_object_unref (operation);
	g_object_unref (inserted_entry2);

	/* Run a final batch operation to delete the second entry */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_documents_service_get_primary_authorization_domain (),
	                                              "https://docs.google.com/feeds/documents/private/full/batch");
	gdata_test_batch_operation_deletion (operation, inserted_entry3, NULL);
	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);
	g_object_unref (inserted_entry3);
}

typedef struct {
	GDataDocumentsEntry *new_doc;
} BatchAsyncData;

static void
set_up_batch_async (BatchAsyncData *data, gconstpointer service)
{
	GDataDocumentsText *doc;
	gchar *upload_uri;
	GError *error = NULL;

	/* Insert a new document which we can query asyncly */
	doc = gdata_documents_text_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (doc), "A View from the Bridge");

	upload_uri = gdata_documents_service_get_upload_uri (NULL);
	data->new_doc = GDATA_DOCUMENTS_ENTRY (gdata_service_insert_entry (GDATA_SERVICE (service),
	                                                                   gdata_documents_service_get_primary_authorization_domain (),
	                                                                   upload_uri, GDATA_ENTRY (doc), NULL, &error));
	g_free (upload_uri);

	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (data->new_doc));
	g_clear_error (&error);

	g_object_unref (doc);
}

static void
test_batch_async_cb (GDataBatchOperation *operation, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GError *error = NULL;

	/* Clear all pending events (such as callbacks for the operations) */
	while (g_main_context_iteration (NULL, FALSE) == TRUE);

	g_assert (gdata_test_batch_operation_run_finish (operation, async_result, &error) == TRUE);
	g_assert_no_error (error);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
}

static void
test_batch_async (BatchAsyncData *data, gconstpointer service)
{
	GDataBatchOperation *operation;
	GMainLoop *main_loop;

	/* Run an async query operation on the document */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_documents_service_get_primary_authorization_domain (),
	                                              "https://docs.google.com/feeds/documents/private/full/batch");
	gdata_test_batch_operation_query (operation, gdata_entry_get_id (GDATA_ENTRY (data->new_doc)), GDATA_TYPE_DOCUMENTS_TEXT,
	                                  GDATA_ENTRY (data->new_doc), NULL, NULL);

	main_loop = g_main_loop_new (NULL, TRUE);

	gdata_batch_operation_run_async (operation, NULL, (GAsyncReadyCallback) test_batch_async_cb, main_loop);
	g_main_loop_run (main_loop);

	g_main_loop_unref (main_loop);
	g_object_unref (operation);
}

static void
test_batch_async_cancellation_cb (GDataBatchOperation *operation, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GError *error = NULL;

	/* Clear all pending events (such as callbacks for the operations) */
	while (g_main_context_iteration (NULL, FALSE) == TRUE);

	g_assert (gdata_test_batch_operation_run_finish (operation, async_result, &error) == FALSE);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
}

static void
test_batch_async_cancellation (BatchAsyncData *data, gconstpointer service)
{
	GDataBatchOperation *operation;
	GMainLoop *main_loop;
	GCancellable *cancellable;
	GError *error = NULL;

	/* Run an async query operation on the document */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_documents_service_get_primary_authorization_domain (),
	                                              "https://docs.google.com/feeds/documents/private/full/batch");
	gdata_test_batch_operation_query (operation, gdata_entry_get_id (GDATA_ENTRY (data->new_doc)), GDATA_TYPE_DOCUMENTS_TEXT,
	                                  GDATA_ENTRY (data->new_doc), NULL, &error);

	main_loop = g_main_loop_new (NULL, TRUE);
	cancellable = g_cancellable_new ();

	gdata_batch_operation_run_async (operation, cancellable, (GAsyncReadyCallback) test_batch_async_cancellation_cb, main_loop);
	g_cancellable_cancel (cancellable); /* this should cancel the operation before it even starts, as we haven't run the main loop yet */

	g_main_loop_run (main_loop);

	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_clear_error (&error);

	g_main_loop_unref (main_loop);
	g_object_unref (cancellable);
	g_object_unref (operation);
}

static void
tear_down_batch_async (BatchAsyncData *data, gconstpointer service)
{
	delete_entry (GDATA_DOCUMENTS_ENTRY (data->new_doc), GDATA_SERVICE (service));
	g_object_unref (data->new_doc);
}

int
main (int argc, char *argv[])
{
	gint retval;
	GDataAuthorizer *authorizer = NULL;
	GDataService *service = NULL;

	gdata_test_init (argc, argv);

	if (gdata_test_internet () == TRUE) {
		authorizer = GDATA_AUTHORIZER (gdata_client_login_authorizer_new (CLIENT_ID, GDATA_TYPE_DOCUMENTS_SERVICE));
		gdata_client_login_authorizer_authenticate (GDATA_CLIENT_LOGIN_AUTHORIZER (authorizer), DOCUMENTS_USERNAME, PASSWORD, NULL, NULL);

		service = GDATA_SERVICE (gdata_documents_service_new (authorizer));

		g_test_add_func ("/documents/authentication", test_authentication);
		g_test_add_func ("/documents/authentication/async", test_authentication_async);

		g_test_add ("/documents/delete/document", TempDocumentData, service, set_up_temp_document_spreadsheet, test_delete_document,
		            tear_down_temp_document);
		g_test_add ("/documents/delete/folder", TempFolderData, service, set_up_temp_folder, test_delete_folder, tear_down_temp_folder);

		g_test_add ("/documents/upload/only_file_get_entry", UploadDocumentData, service, set_up_upload_document, test_upload_file_get_entry,
		            tear_down_upload_document);
		g_test_add ("/documents/upload/metadata_file", UploadDocumentData, service, set_up_upload_document, test_upload_metadata_file,
		            tear_down_upload_document);
		g_test_add ("/documents/upload/only_metadata", UploadDocumentData, service, set_up_upload_document, test_upload_metadata,
		            tear_down_upload_document);
		g_test_add ("/documents/upload/metadata_file_in_new_folder", UploadDocumentData, service, set_up_upload_document_with_folder,
		            test_upload_file_metadata_in_new_folder, tear_down_upload_document);

		g_test_add ("/documents/download/document", TempDocumentsData, service, set_up_temp_documents, test_download_document,
		            tear_down_temp_documents);

		g_test_add ("/documents/update/only_metadata", TempDocumentData, service, set_up_temp_document_text, test_update_metadata,
		            tear_down_temp_document);
		g_test_add ("/documents/update/only_file", TempDocumentData, service, set_up_temp_document_presentation, test_update_file,
		            tear_down_temp_document);
		g_test_add ("/documents/update/metadata_file", TempDocumentData, service, set_up_temp_document_text, test_update_metadata_file,
		            tear_down_temp_document);

		g_test_add ("/documents/access-rule/insert", TempDocumentData, service, set_up_temp_document_spreadsheet, test_access_rule_insert,
		            tear_down_temp_document);

		g_test_add ("/documents/query/all_documents", TempDocumentsData, service, set_up_temp_documents, test_query_all_documents,
		            tear_down_temp_documents);
		g_test_add ("/documents/query/all_documents/with_folder", TempDocumentsData, service, set_up_temp_documents,
		            test_query_all_documents_with_folder, tear_down_temp_documents);
		g_test_add ("/documents/query/all_documents/async", TempDocumentsAsyncData, service, set_up_temp_documents_async,
		            test_query_all_documents_async, tear_down_temp_documents_async);
		g_test_add ("/documents/query/all_documents/async/progress_closure", TempDocumentsAsyncData, service, set_up_temp_documents_async,
		            test_query_all_documents_async_progress_closure, tear_down_temp_documents_async);

		g_test_add ("/documents/folders/add_to_folder", FoldersData, service, set_up_folders_add_to_folder,
		            test_folders_add_to_folder, tear_down_folders_add_to_folder);
		g_test_add ("/documents/folders/add_to_folder/async", FoldersAsyncData, service, set_up_folders_add_to_folder_async,
		            test_folders_add_to_folder_async, tear_down_folders_add_to_folder_async);
		g_test_add ("/documents/folders/add_to_folder/cancellation", FoldersAsyncData, service, set_up_folders_add_to_folder_async,
		            test_folders_add_to_folder_cancellation, tear_down_folders_add_to_folder_async);

		g_test_add ("/documents/folders/remove_from_folder", FoldersData, service, set_up_folders_remove_from_folder,
		            test_folders_remove_from_folder, tear_down_folders_remove_from_folder);
		g_test_add ("/documents/folders/remove_from_folder/async", FoldersAsyncData, service, set_up_folders_remove_from_folder_async,
		            test_folders_remove_from_folder_async, tear_down_folders_remove_from_folder_async);
		g_test_add ("/documents/folders/remove_from_folder/cancellation", FoldersAsyncData, service, set_up_folders_remove_from_folder_async,
		            test_folders_remove_from_folder_cancellation, tear_down_folders_remove_from_folder_async);

		g_test_add_data_func ("/documents/batch", service, test_batch);
		g_test_add ("/documents/batch/async", BatchAsyncData, service, set_up_batch_async, test_batch_async, tear_down_batch_async);
		g_test_add ("/documents/batch/async/cancellation", BatchAsyncData, service, set_up_batch_async, test_batch_async_cancellation,
		            tear_down_batch_async);
	}

	g_test_add_func ("/documents/query/etag", test_query_etag);

	retval = g_test_run ();

	if (service != NULL)
		g_object_unref (service);

	return retval;
}
