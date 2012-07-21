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

/* For the thumbnail size tests in test_download_thumbnail() */
#ifdef HAVE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

#include "gdata.h"
#include "common.h"

static gboolean
check_document_is_in_folder (GDataDocumentsDocument *document, GDataDocumentsFolder *folder)
{
	GList *links;
	gboolean found_folder_category = FALSE;
	GDataLink *folder_self_link;

	folder_self_link = gdata_entry_look_up_link (GDATA_ENTRY (folder), GDATA_LINK_SELF);
	g_assert (folder_self_link != NULL);

	for (links = gdata_entry_look_up_links (GDATA_ENTRY (document), "http://schemas.google.com/docs/2007#parent");
	     links != NULL; links = links->next) {
		GDataLink *_link = GDATA_LINK (links->data);

		if (strcmp (gdata_link_get_uri (_link), gdata_link_get_uri (folder_self_link)) == 0 &&
		    strcmp (gdata_link_get_title (_link), gdata_entry_get_title (GDATA_ENTRY (folder))) == 0) {
			g_assert (found_folder_category == FALSE);
			found_folder_category = TRUE;
		}
	}

	return found_folder_category;
}

static gboolean
check_document_is_in_root_folder (GDataDocumentsDocument *document)
{
	GList *links;
	gboolean is_in_root_folder;

	links = gdata_entry_look_up_links (GDATA_ENTRY (document), "http://schemas.google.com/docs/2007#parent");
	is_in_root_folder = (links == NULL) ? TRUE : FALSE;
	g_list_free (links);

	return is_in_root_folder;
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

	/* Delete the entry. Don't bother asserting that it succeeds, because it will often fail because Google keep giving us the wrong ETag above. */
	gdata_service_delete_entry (service, gdata_documents_service_get_primary_authorization_domain (), new_entry, NULL, NULL);
	g_object_unref (new_entry);
}

static GDataDocumentsFolder *
create_folder (GDataDocumentsService *service, const gchar *title)
{
	GDataDocumentsFolder *folder, *new_folder;
	gchar *upload_uri;

	folder = gdata_documents_folder_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (folder), title);

	/* Insert the folder */
	upload_uri = gdata_documents_service_get_upload_uri (NULL);
	new_folder = GDATA_DOCUMENTS_FOLDER (gdata_service_insert_entry (GDATA_SERVICE (service),
	                                                                 gdata_documents_service_get_primary_authorization_domain (),
	                                                                 upload_uri, GDATA_ENTRY (folder), NULL, NULL));
	g_assert (GDATA_IS_DOCUMENTS_FOLDER (new_folder));
	g_free (upload_uri);
	g_object_unref (folder);

	return new_folder;
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

GDATA_ASYNC_TEST_FUNCTIONS (authentication, void,
G_STMT_START {
	GDataClientLoginAuthorizer *authorizer;

	/* Create an authorizer */
	authorizer = gdata_client_login_authorizer_new (CLIENT_ID, GDATA_TYPE_DOCUMENTS_SERVICE);

	g_assert_cmpstr (gdata_client_login_authorizer_get_client_id (authorizer), ==, CLIENT_ID);

	gdata_client_login_authorizer_authenticate_async (authorizer, USERNAME, PASSWORD, cancellable, async_ready_callback, async_data);

	g_object_unref (authorizer);
} G_STMT_END,
G_STMT_START {
	gboolean retval;
	GDataClientLoginAuthorizer *authorizer = GDATA_CLIENT_LOGIN_AUTHORIZER (obj);

	retval = gdata_client_login_authorizer_authenticate_finish (authorizer, async_result, &error);

	if (error == NULL) {
		g_assert (retval == TRUE);

		/* Check all is as it should be */
		g_assert_cmpstr (gdata_client_login_authorizer_get_username (authorizer), ==, USERNAME);
		g_assert_cmpstr (gdata_client_login_authorizer_get_password (authorizer), ==, PASSWORD);

		g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
		                                                     gdata_documents_service_get_primary_authorization_domain ()) == TRUE);
		g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
		                                                     gdata_documents_service_get_spreadsheet_authorization_domain ()) == TRUE);
	} else {
		g_assert (retval == FALSE);

		/* Check nothing's changed */
		g_assert_cmpstr (gdata_client_login_authorizer_get_username (authorizer), ==, NULL);
		g_assert_cmpstr (gdata_client_login_authorizer_get_password (authorizer), ==, NULL);

		g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
		                                                     gdata_documents_service_get_primary_authorization_domain ()) == FALSE);
		g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
		                                                     gdata_documents_service_get_spreadsheet_authorization_domain ()) == FALSE);
	}
} G_STMT_END);

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
_set_up_temp_document (GDataDocumentsEntry *entry, GDataService *service, GFile *document_file)
{
	GDataDocumentsDocument *document, *new_document;
	GFileInfo *file_info;
	GFileInputStream *file_stream;
	GDataUploadStream *upload_stream;
	GError *error = NULL;

	/* Query for information on the file. */
	file_info = g_file_query_info (document_file,
	                               G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
	                               G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE, NULL, &error);
	g_assert_no_error (error);

	/* Prepare the upload stream */
	upload_stream = gdata_documents_service_upload_document_resumable (GDATA_DOCUMENTS_SERVICE (service), GDATA_DOCUMENTS_DOCUMENT (entry),
	                                                                   g_file_info_get_display_name (file_info),
	                                                                   g_file_info_get_content_type (file_info), g_file_info_get_size (file_info),
	                                                                   NULL, NULL, &error);
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
	document = gdata_documents_service_finish_upload (GDATA_DOCUMENTS_SERVICE (service), upload_stream, &error);
	g_assert_no_error (error);

	g_object_unref (upload_stream);

	/* HACK: Query for the new document, as Google's servers appear to modify it behind our back when creating the document:
	 * http://code.google.com/a/google.com/p/apps-api-issues/issues/detail?id=2337. We have to wait a few seconds before trying this to allow the
	 * various Google servers to catch up with each other. */
	g_usleep (5 * G_USEC_PER_SEC);
	new_document = GDATA_DOCUMENTS_DOCUMENT (gdata_service_query_single_entry (service,
	                                                                           gdata_documents_service_get_primary_authorization_domain (),
	                                                                           gdata_entry_get_id (GDATA_ENTRY (document)), NULL,
	                                                                           G_OBJECT_TYPE (document), NULL, NULL));
	g_assert (GDATA_IS_DOCUMENTS_DOCUMENT (new_document));

	g_object_unref (document);

	return new_document;
}

static void
set_up_temp_document_spreadsheet (TempDocumentData *data, gconstpointer service)
{
	GDataDocumentsSpreadsheet *document;
	gchar *document_file_path;
	GFile *document_file;

	/* Create a document */
	document = gdata_documents_spreadsheet_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (document), "Temporary Document (Spreadsheet)");

	document_file_path = g_strconcat (TEST_FILE_DIR, "test.ods", NULL);
	document_file = g_file_new_for_path (document_file_path);
	g_free (document_file_path);

	data->document = _set_up_temp_document (GDATA_DOCUMENTS_ENTRY (document), GDATA_SERVICE (service), document_file);

	g_object_unref (document_file);
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
	GDataDocumentsDocument *arbitrary_document;
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

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_document_new (NULL));
	gdata_entry_set_title (GDATA_ENTRY (document), "Temporary Arbitrary Document");
	data->arbitrary_document = GDATA_DOCUMENTS_DOCUMENT (
		gdata_service_insert_entry (GDATA_SERVICE (service), gdata_documents_service_get_primary_authorization_domain (),
		                            upload_uri, GDATA_ENTRY (document), NULL, NULL)
	);
	g_assert (GDATA_IS_DOCUMENTS_DOCUMENT (data->arbitrary_document));
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

	delete_entry (GDATA_DOCUMENTS_ENTRY (data->arbitrary_document), GDATA_SERVICE (service));
	g_object_unref (data->arbitrary_document);

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

GDATA_ASYNC_CLOSURE_FUNCTIONS (temp_documents, TempDocumentsData);

GDATA_ASYNC_TEST_FUNCTIONS (query_all_documents, TempDocumentsData,
G_STMT_START {
	gdata_documents_service_query_documents_async (GDATA_DOCUMENTS_SERVICE (service), NULL, cancellable, NULL, NULL,
	                                               NULL, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataDocumentsFeed *feed;

	feed = GDATA_DOCUMENTS_FEED (gdata_service_query_finish (GDATA_SERVICE (obj), async_result, &error));

	if (error == NULL) {
		g_assert (GDATA_IS_FEED (feed));
		/* TODO: Tests? */

		g_object_unref (feed);
	} else {
		g_assert (feed == NULL);
	}
} G_STMT_END);

static void
test_query_all_documents_async_progress_closure (TempDocumentsData *documents_data, gconstpointer service)
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

typedef enum {
	UPLOAD_METADATA_ONLY,
	UPLOAD_CONTENT_ONLY,
	UPLOAD_CONTENT_AND_METADATA,
} PayloadType;
#define UPLOAD_PAYLOAD_TYPE_MAX UPLOAD_CONTENT_AND_METADATA

const gchar *payload_type_names[] = {
	"metadata-only",
	"content-only",
	"content-and-metadata",
};

typedef enum {
	UPLOAD_IN_FOLDER,
	UPLOAD_ROOT_FOLDER,
} FolderType;
#define UPLOAD_FOLDER_TYPE_MAX UPLOAD_ROOT_FOLDER

const gchar *folder_type_names[] = {
	"in-folder",
	"root-folder",
};

typedef enum {
	UPLOAD_RESUMABLE,
	UPLOAD_NON_RESUMABLE,
} ResumableType;
#define UPLOAD_RESUMABLE_TYPE_MAX UPLOAD_NON_RESUMABLE

const gchar *resumable_type_names[] = {
	"resumable",
	"non-resumable",
};

typedef enum {
	UPLOAD_ODT_CONVERT,
	UPLOAD_ODT_NO_CONVERT,
	UPLOAD_BIN_NO_CONVERT,
} DocumentType;
#define UPLOAD_DOCUMENT_TYPE_MAX UPLOAD_BIN_NO_CONVERT

const gchar *document_type_names[] = {
	"odt-convert",
	"odt-no-convert",
	"bin-no-convert",
};

typedef struct {
	PayloadType payload_type;
	FolderType folder_type;
	ResumableType resumable_type;
	DocumentType document_type;
	gchar *test_name;

	GDataDocumentsService *service;
} UploadDocumentTestParams;

typedef struct {
	GDataDocumentsFolder *folder;
	GDataDocumentsDocument *new_document;
} UploadDocumentData;

static void
set_up_upload_document (UploadDocumentData *data, gconstpointer _test_params)
{
	const UploadDocumentTestParams *test_params = _test_params;

	data->new_document = NULL;

	switch (test_params->folder_type) {
		case UPLOAD_IN_FOLDER:
			data->folder = create_folder (test_params->service, "Temporary Folder for Uploading Documents");
			break;
		case UPLOAD_ROOT_FOLDER:
			data->folder = NULL;
			break;
		default:
			g_assert_not_reached ();
	}
}

static void
tear_down_upload_document (UploadDocumentData *data, gconstpointer _test_params)
{
	const UploadDocumentTestParams *test_params = _test_params;

	/* Delete the new file */
	if (data->new_document != NULL) {
		/* HACK: Query for the new document, as Google's servers appear to modify it behind our back if we don't upload both metadata and data
		 * when creating the document: http://code.google.com/a/google.com/p/apps-api-issues/issues/detail?id=2337. We have to wait a few
		 * seconds before trying this to allow the various Google servers to catch up with each other. */
		g_usleep (5 * G_USEC_PER_SEC);
		delete_entry (GDATA_DOCUMENTS_ENTRY (data->new_document), GDATA_SERVICE (test_params->service));
		g_object_unref (data->new_document);
	}

	/* Delete the folder */
	if (data->folder != NULL) {
		delete_entry (GDATA_DOCUMENTS_ENTRY (data->folder), GDATA_SERVICE (test_params->service));
		g_object_unref (data->folder);
	}
}

static void
test_upload (UploadDocumentData *data, gconstpointer _test_params)
{
	const UploadDocumentTestParams *test_params = _test_params;

	GDataDocumentsDocument *document = NULL;
	const gchar *document_filename = NULL;
	GFile *document_file = NULL;
	GFileInfo *file_info = NULL;
	GDataDocumentsUploadQuery *upload_query = NULL;
	GDataLink *edit_media_link;
	GError *error = NULL;

	upload_query = gdata_documents_upload_query_new ();

	/* File to upload. (Ignored if we're doing a metadata-only upload.) Also set the conversion type (ignored for non-resumable uploads). */
	switch (test_params->document_type) {
		case UPLOAD_ODT_CONVERT:
			/* ODT file. */
			document_filename = "test.odt";
			gdata_documents_upload_query_set_convert (upload_query, TRUE);
			break;
		case UPLOAD_ODT_NO_CONVERT:
			/* ODT file. */
			document_filename = "test.odt";
			gdata_documents_upload_query_set_convert (upload_query, FALSE);
			break;
		case UPLOAD_BIN_NO_CONVERT:
			/* Arbitrary binary file. */
			document_filename = "sample.ogg";
			gdata_documents_upload_query_set_convert (upload_query, FALSE);
			break;
		default:
			g_assert_not_reached ();
	}

	/* Upload content? */
	switch (test_params->payload_type) {
		case UPLOAD_METADATA_ONLY:
			document_filename = NULL;
			document_file = NULL;
			file_info = NULL;
			break;
		case UPLOAD_CONTENT_ONLY:
		case UPLOAD_CONTENT_AND_METADATA: {
			gchar *document_file_path = g_strconcat (TEST_FILE_DIR, document_filename, NULL);
			document_file = g_file_new_for_path (document_file_path);
			g_free (document_file_path);

			file_info = g_file_query_info (document_file,
			                               G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
			                               G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE, NULL, &error);
			g_assert_no_error (error);

			break;
		}
		default:
			g_assert_not_reached ();
	}

	/* Upload metadata? */
	switch (test_params->payload_type) {
		case UPLOAD_CONTENT_ONLY:
			document = NULL;
			break;
		case UPLOAD_METADATA_ONLY:
		case UPLOAD_CONTENT_AND_METADATA: {
			gchar *title;

			switch (test_params->document_type) {
				case UPLOAD_ODT_CONVERT:
					document = GDATA_DOCUMENTS_DOCUMENT (gdata_documents_text_new (NULL));
					break;
				case UPLOAD_ODT_NO_CONVERT:
				case UPLOAD_BIN_NO_CONVERT:
					document = GDATA_DOCUMENTS_DOCUMENT (gdata_documents_document_new (NULL));
					break;
				default:
					g_assert_not_reached ();
			}

			/* Build a title including the test details. */
			title = g_strdup_printf ("Test Upload file (%s)", test_params->test_name);
			gdata_entry_set_title (GDATA_ENTRY (document), title);
			g_free (title);

			break;
		}
		default:
			g_assert_not_reached ();
	}

	if (test_params->payload_type == UPLOAD_METADATA_ONLY) {
		gchar *upload_uri;

		/* Insert the document */
		upload_uri = gdata_documents_service_get_upload_uri (data->folder);
		data->new_document = GDATA_DOCUMENTS_DOCUMENT (gdata_service_insert_entry (GDATA_SERVICE (test_params->service),
		                                                                           gdata_documents_service_get_primary_authorization_domain (),
		                                                                           upload_uri, GDATA_ENTRY (document), NULL, &error));
		g_free (upload_uri);

		g_assert_no_error (error);
	} else {
		GDataUploadStream *upload_stream;
		GFileInputStream *file_stream;

		/* Prepare the upload stream */
		switch (test_params->resumable_type) {
			case UPLOAD_NON_RESUMABLE:
				upload_stream = gdata_documents_service_upload_document (test_params->service, document,
				                                                         g_file_info_get_display_name (file_info),
				                                                         g_file_info_get_content_type (file_info), data->folder,
				                                                         NULL, &error);
				break;
			case UPLOAD_RESUMABLE:
				gdata_documents_upload_query_set_folder (upload_query, data->folder);

				upload_stream = gdata_documents_service_upload_document_resumable (test_params->service, document,
				                                                                   g_file_info_get_display_name (file_info),
				                                                                   g_file_info_get_content_type (file_info),
				                                                                   g_file_info_get_size (file_info), upload_query,
				                                                                   NULL, &error);

				break;
			default:
				g_assert_not_reached ();
		}

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
		data->new_document = gdata_documents_service_finish_upload (test_params->service, upload_stream, &error);
		g_assert_no_error (error);

		g_object_unref (upload_stream);
		g_object_unref (file_stream);
	}

	g_assert (GDATA_IS_DOCUMENTS_DOCUMENT (data->new_document)); /* note that this isn't entirely specific */

	/* Verify the uploaded document is the same as the original */
	switch (test_params->payload_type) {
		case UPLOAD_METADATA_ONLY:
		case UPLOAD_CONTENT_AND_METADATA:
			g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (data->new_document)), ==, gdata_entry_get_title (GDATA_ENTRY (document)));
			break;
		case UPLOAD_CONTENT_ONLY:
			/* HACK: The title returned by the server varies depending on how we uploaded the document. */
			if (test_params->resumable_type == UPLOAD_NON_RESUMABLE) {
				g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (data->new_document)), ==, document_filename);
			} else {
				g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (data->new_document)), ==, "Untitled");
			}

			break;
		default:
			g_assert_not_reached ();
	}

	/* Check it's been correctly converted/not converted and is of the right document type. */
	edit_media_link = gdata_entry_look_up_link (GDATA_ENTRY (data->new_document), GDATA_LINK_EDIT_MEDIA);

	switch (test_params->document_type) {
		case UPLOAD_ODT_CONVERT:
			g_assert (GDATA_IS_DOCUMENTS_TEXT (data->new_document));
			g_assert (g_str_has_prefix (gdata_documents_entry_get_resource_id (GDATA_DOCUMENTS_ENTRY (data->new_document)), "document:"));
			g_assert_cmpstr (gdata_link_get_content_type (edit_media_link), ==, "text/html");
			break;
		case UPLOAD_ODT_NO_CONVERT:
			g_assert (GDATA_IS_DOCUMENTS_DOCUMENT (data->new_document));
			g_assert (g_str_has_prefix (gdata_documents_entry_get_resource_id (GDATA_DOCUMENTS_ENTRY (data->new_document)), "file:"));
			g_assert_cmpstr (gdata_link_get_content_type (edit_media_link), ==, "application/vnd.oasis.opendocument.text");
			break;
		case UPLOAD_BIN_NO_CONVERT:
			g_assert (GDATA_IS_DOCUMENTS_DOCUMENT (data->new_document));
			g_assert (g_str_has_prefix (gdata_documents_entry_get_resource_id (GDATA_DOCUMENTS_ENTRY (data->new_document)), "file:"));
			g_assert_cmpstr (gdata_link_get_content_type (edit_media_link), ==, "video/x-theora+ogg");
			break;
		default:
			g_assert_not_reached ();
	}

	/* Check it's in the right folder. */
	switch (test_params->folder_type) {
		case UPLOAD_IN_FOLDER:
			/* HACK: When uploading content-only to a folder using the folder's resumable-create-media link, Google decides that it's
			 * not useful to list the folder in the returned entry XML for the new document (i.e. the server pretends the document's
			 * not in the folder you've just uploaded it to). Joy. */
			g_assert (test_params->payload_type == UPLOAD_CONTENT_ONLY ||
			          check_document_is_in_folder (data->new_document, data->folder) == TRUE);
			break;
		case UPLOAD_ROOT_FOLDER:
			/* Check root folder. */
			g_assert (check_document_is_in_root_folder (data->new_document) == TRUE);
			break;
		default:
			g_assert_not_reached ();
	}

	g_clear_error (&error);
	g_clear_object (&upload_query);
	g_clear_object (&document_file);
	g_clear_object (&document);
}

typedef struct {
	PayloadType payload_type;
	ResumableType resumable_type;
	gchar *test_name;

	GDataDocumentsService *service;
} UpdateDocumentTestParams;

typedef struct {
	GDataDocumentsDocument *document;
} UpdateDocumentData;

static void
set_up_update_document (UpdateDocumentData *data, gconstpointer _test_params)
{
	const UpdateDocumentTestParams *test_params = _test_params;
	GDataDocumentsText *document;
	gchar *title, *document_file_path;
	GFile *document_file;

	/* Create a document */
	document = gdata_documents_text_new (NULL);
	title = g_strdup_printf ("Test Update file (%s)", test_params->test_name);
	gdata_entry_set_title (GDATA_ENTRY (document), title);
	g_free (title);

	document_file_path = g_strconcat (TEST_FILE_DIR, "test.odt", NULL);
	document_file = g_file_new_for_path (document_file_path);
	g_free (document_file_path);

	data->document = _set_up_temp_document (GDATA_DOCUMENTS_ENTRY (document), GDATA_SERVICE (test_params->service), document_file);

	g_object_unref (document_file);
	g_object_unref (document);
}

static void
tear_down_update_document (UpdateDocumentData *data, gconstpointer _test_params)
{
	const UpdateDocumentTestParams *test_params = _test_params;

	/* Delete the new file */
	if (data->document != NULL) {
		/* HACK: Query for the new document, as Google's servers appear to modify it behind our back if we don't update both metadata and data
		 * when creating the document: http://code.google.com/a/google.com/p/apps-api-issues/issues/detail?id=2337. We have to wait a few
		 * seconds before trying this to allow the various Google servers to catch up with each other. */
		g_usleep (5 * G_USEC_PER_SEC);
		delete_entry (GDATA_DOCUMENTS_ENTRY (data->document), GDATA_SERVICE (test_params->service));
		g_object_unref (data->document);
	}
}

static void
test_update (UpdateDocumentData *data, gconstpointer _test_params)
{
	const UpdateDocumentTestParams *test_params = _test_params;

	GDataDocumentsDocument *updated_document;
	gchar *original_title;
	GError *error = NULL;

	switch (test_params->payload_type) {
		case UPLOAD_METADATA_ONLY:
		case UPLOAD_CONTENT_AND_METADATA: {
			gchar *new_title;

			/* Change the title of the document */
			original_title = g_strdup (gdata_entry_get_title (GDATA_ENTRY (data->document)));
			new_title = g_strdup_printf ("Updated Test Update file (%s)", test_params->test_name);
			gdata_entry_set_title (GDATA_ENTRY (data->document), new_title);
			g_free (new_title);

			break;
		}
		case UPLOAD_CONTENT_ONLY:
			original_title = NULL;
			break;
		default:
			g_assert_not_reached ();
	}

	if (test_params->payload_type == UPLOAD_METADATA_ONLY) {
		/* Update the document */
		updated_document = GDATA_DOCUMENTS_DOCUMENT (gdata_service_update_entry (GDATA_SERVICE (test_params->service),
		                                                                         gdata_documents_service_get_primary_authorization_domain (),
		                                                                         GDATA_ENTRY (data->document), NULL, &error));
		g_assert_no_error (error);
	} else {
		GDataUploadStream *upload_stream;
		GFileInputStream *file_stream;
		GFile *updated_document_file;
		GFileInfo *file_info;

		/* Prepare the updated file */
		updated_document_file = g_file_new_for_path (TEST_FILE_DIR "test_updated.odt");

		file_info = g_file_query_info (updated_document_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME ","
		                               G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE "," G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE,
		                               NULL, &error);
		g_assert_no_error (error);

		/* Prepare the upload stream */
		switch (test_params->resumable_type) {
			case UPLOAD_NON_RESUMABLE:
				upload_stream = gdata_documents_service_update_document (test_params->service, data->document,
				                                                         g_file_info_get_display_name (file_info),
				                                                         g_file_info_get_content_type (file_info),
				                                                         NULL, &error);
				break;
			case UPLOAD_RESUMABLE:
				upload_stream = gdata_documents_service_update_document_resumable (test_params->service, data->document,
				                                                                   g_file_info_get_display_name (file_info),
				                                                                   g_file_info_get_content_type (file_info),
				                                                                   g_file_info_get_size (file_info), NULL, &error);
				break;
			default:
				g_assert_not_reached ();
		}

		g_assert_no_error (error);
		g_assert (GDATA_IS_UPLOAD_STREAM (upload_stream));

		g_object_unref (file_info);

		/* Open the file */
		file_stream = g_file_read (updated_document_file, NULL, &error);
		g_assert_no_error (error);

		g_object_unref (updated_document_file);

		/* Upload the updated document */
		g_output_stream_splice (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (file_stream),
		                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &error);
		g_assert_no_error (error);

		/* Finish the upload */
		updated_document = gdata_documents_service_finish_upload (test_params->service, upload_stream, &error);
		g_assert_no_error (error);

		g_object_unref (upload_stream);
		g_object_unref (file_stream);
	}

	g_assert (GDATA_IS_DOCUMENTS_TEXT (updated_document));

	/* Check for success */
	switch (test_params->payload_type) {
		case UPLOAD_METADATA_ONLY:
		case UPLOAD_CONTENT_AND_METADATA:
			g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (updated_document)), !=, original_title);
			/* Fall through */
		case UPLOAD_CONTENT_ONLY:
			g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (updated_document)), ==,
			                 gdata_entry_get_title (GDATA_ENTRY (data->document)));
			break;
		default:
			g_assert_not_reached ();
	}

	g_free (original_title);
	g_object_unref (updated_document);
}

typedef struct {
	TempDocumentData parent;
	GDataDocumentsDocument *new_document;
} TempCopyDocumentData;

static void
set_up_copy_document (TempCopyDocumentData *data, gconstpointer service)
{
	/* Create a temporary document. */
	set_up_temp_document_spreadsheet ((TempDocumentData*) data, service);

	data->new_document = NULL;
}

static void
tear_down_copy_document (TempCopyDocumentData *data, gconstpointer service)
{
	/* Delete the copied document */
	delete_entry (GDATA_DOCUMENTS_ENTRY (data->new_document), GDATA_SERVICE (service));
	g_object_unref (data->new_document);

	/* Delete the folder */
	tear_down_temp_document ((TempDocumentData*) data, service);
}

static void
test_copy_document (TempCopyDocumentData *data, gconstpointer service)
{
	GError *error = NULL;

	/* Copy the document */
	data->new_document = gdata_documents_service_copy_document (GDATA_DOCUMENTS_SERVICE (service), data->parent.document, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_SPREADSHEET (data->new_document));

	/* Check their IDs are different but that their other properties (e.g. title) are the same. */
	g_assert_cmpstr (gdata_entry_get_id (GDATA_ENTRY (data->parent.document)), !=, gdata_entry_get_id (GDATA_ENTRY (data->new_document)));
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (data->parent.document)), ==, gdata_entry_get_title (GDATA_ENTRY (data->new_document)));
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

GDATA_ASYNC_CLOSURE_FUNCTIONS (folders_add_to_folder, FoldersData);

GDATA_ASYNC_TEST_FUNCTIONS (folders_add_to_folder, FoldersData,
G_STMT_START {
	/* Add the document to the folder asynchronously */
	gdata_documents_service_add_entry_to_folder_async (GDATA_DOCUMENTS_SERVICE (service), GDATA_DOCUMENTS_ENTRY (data->document),
	                                                   data->folder, cancellable, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataDocumentsEntry *entry;

	entry = gdata_documents_service_add_entry_to_folder_finish (GDATA_DOCUMENTS_SERVICE (obj), async_result, &error);

	if (error == NULL) {
		g_assert (GDATA_IS_DOCUMENTS_ENTRY (entry));

		/* Check it's still the same document */
		g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (entry)), ==, gdata_entry_get_title (GDATA_ENTRY (data->document)));
		g_assert (check_document_is_in_folder (GDATA_DOCUMENTS_DOCUMENT (entry), data->folder) == TRUE);

		g_object_unref (entry);
	} else {
		g_assert (entry == NULL);
	}
} G_STMT_END);

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

GDATA_ASYNC_CLOSURE_FUNCTIONS (folders_remove_from_folder, FoldersData);

GDATA_ASYNC_TEST_FUNCTIONS (folders_remove_from_folder, FoldersData,
G_STMT_START {
	/* Remove the document from the folder asynchronously */
	gdata_documents_service_remove_entry_from_folder_async (GDATA_DOCUMENTS_SERVICE (service), GDATA_DOCUMENTS_ENTRY (data->document),
	                                                        data->folder, cancellable, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataDocumentsEntry *entry;

	entry = gdata_documents_service_remove_entry_from_folder_finish (GDATA_DOCUMENTS_SERVICE (obj), async_result, &error);

	if (error == NULL) {
		g_assert (GDATA_IS_DOCUMENTS_ENTRY (entry));

		/* Check it's still the same document */
		g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (entry)), ==, gdata_entry_get_title (GDATA_ENTRY (data->document)));
		g_assert (check_document_is_in_folder (GDATA_DOCUMENTS_DOCUMENT (entry), data->folder) == FALSE);

		g_object_unref (entry);
	} else {
		g_assert (entry == NULL);
	}
} G_STMT_END);

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
	destination_file_name = g_strdup_printf ("%s.%s", gdata_documents_entry_get_resource_id (GDATA_DOCUMENTS_ENTRY (document)),
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
	_test_download_document (GDATA_DOCUMENTS_DOCUMENT (data->arbitrary_document), GDATA_SERVICE (service));
}

static void
test_download_thumbnail (TempDocumentData *data, gconstpointer service)
{
	const gchar *thumbnail_uri;
	GDataDownloadStream *download_stream;
	gchar *destination_file_name, *destination_file_path;
	GFile *destination_file;
	GFileOutputStream *file_stream;
	gssize transfer_size;
	GError *error = NULL;

	thumbnail_uri = gdata_documents_document_get_thumbnail_uri (data->document);

	/* Google takes many minutes to generate thumbnails for uploaded documents, so with our current testing strategy of creating fresh documents
	 * for each test, this particular test is fairly useless. */
	if (thumbnail_uri == NULL) {
		g_test_message ("Skipping thumbnail download test because document ‘%s’ doesn’t have a thumbnail.",
		                gdata_documents_entry_get_resource_id (GDATA_DOCUMENTS_ENTRY (data->document)));
		return;
	}

	/* Download the thumbnail to a file for testing (in case we weren't compiled with GdkPixbuf support) */
	download_stream = GDATA_DOWNLOAD_STREAM (gdata_download_stream_new (GDATA_SERVICE (service), NULL, thumbnail_uri, NULL));
	g_assert (GDATA_IS_DOWNLOAD_STREAM (download_stream));

	/* Prepare a file to write the data to */
	destination_file_name = g_strdup_printf ("%s_thumbnail.jpg", gdata_documents_entry_get_resource_id (GDATA_DOCUMENTS_ENTRY (data->document)));
	destination_file_path = g_build_filename (g_get_tmp_dir (), destination_file_name, NULL);
	g_free (destination_file_name);
	destination_file = g_file_new_for_path (destination_file_path);
	g_free (destination_file_path);

	/* Download the file */
	file_stream = g_file_replace (destination_file, NULL, FALSE, G_FILE_CREATE_REPLACE_DESTINATION, NULL, &error);
	g_assert_no_error (error);
	g_assert (G_IS_FILE_OUTPUT_STREAM (file_stream));

	transfer_size = g_output_stream_splice (G_OUTPUT_STREAM (file_stream), G_INPUT_STREAM (download_stream),
	                                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &error);
	g_assert_no_error (error);
	g_assert_cmpint (transfer_size, >, 0);

	g_object_unref (file_stream);
	g_object_unref (download_stream);

	/* Delete the file (shouldn't cause the test to fail if this fails) */
	g_file_delete (destination_file, NULL, NULL);
	g_object_unref (destination_file);

#ifdef HAVE_GDK_PIXBUF
	/* Test downloading all thumbnails directly into GdkPixbufs, and check that they're all the correct size */
	{
		GdkPixbuf *pixbuf;

		/* Prepare a new download stream */
		download_stream = GDATA_DOWNLOAD_STREAM (gdata_download_stream_new (GDATA_SERVICE (service), NULL, thumbnail_uri, NULL));
		g_assert (GDATA_IS_DOWNLOAD_STREAM (download_stream));

		/* Download into a new GdkPixbuf */
		pixbuf = gdk_pixbuf_new_from_stream (G_INPUT_STREAM (download_stream), NULL, &error);
		g_assert_no_error (error);
		g_assert (GDK_IS_PIXBUF (pixbuf));

		g_object_unref (download_stream);

		/* Check the dimensions are as expected. */
		g_assert_cmpint (gdk_pixbuf_get_width (pixbuf), ==, 10);
		g_assert_cmpint (gdk_pixbuf_get_height (pixbuf), ==, 10);

		g_object_unref (pixbuf);
	}
#endif /* HAVE_GDK_PIXBUF */
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
test_folder_parser_normal (void)
{
	GDataDocumentsFolder *folder;
	gchar *path;
	GDataAuthor *author;
	GError *error = NULL;

	folder = GDATA_DOCUMENTS_FOLDER (gdata_parsable_new_from_xml (GDATA_TYPE_DOCUMENTS_FOLDER,
		"<?xml version='1.0' encoding='UTF-8'?>"
		"<entry xmlns='http://www.w3.org/2005/Atom' xmlns:docs='http://schemas.google.com/docs/2007' "
		       "xmlns:batch='http://schemas.google.com/gdata/batch' xmlns:gd='http://schemas.google.com/g/2005' "
		       "gd:etag='&quot;WBYEFh8LRCt7ImBk&quot;'>"
			"<id>https://docs.google.com/feeds/id/folder%3A0BzY2jgHHwMwYalFhbjhVT3dyams</id>"
			"<published>2012-04-14T09:12:19.418Z</published>"
			"<updated>2012-04-14T09:12:19.418Z</updated>"
			"<app:edited xmlns:app='http://www.w3.org/2007/app'>2012-04-14T09:12:20.055Z</app:edited>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://schemas.google.com/docs/2007#folder' label='folder'/>"
			"<title>Temporary Folder</title>"
			"<content type='application/atom+xml;type=feed' "
			         "src='https://docs.google.com/feeds/default/private/full/folder%3A0BzY2jgHHwMwYalFhbjhVT3dyams/contents'/>"
			"<link rel='alternate' type='text/html' href='https://docs.google.com/#folders/folder.0.0BzY2jgHHwMwYalFhbjhVT3dyams'/>"
			"<link rel='http://schemas.google.com/docs/2007#icon' type='image/png' href='https://ssl.gstatic.com/docs/doclist/images/icon_9_collection_list.png'/>"
			"<link rel='http://schemas.google.com/g/2005#resumable-create-media' type='application/atom+xml' href='https://docs.google.com/feeds/upload/create-session/default/private/full/folder%3A0BzY2jgHHwMwYalFhbjhVT3dyams/contents'/>"
			"<link rel='http://schemas.google.com/docs/2007#alt-post' type='application/atom+xml' href='https://docs.google.com/feeds/upload/file/default/private/full/folder%3A0BzY2jgHHwMwYalFhbjhVT3dyams/contents'/>"
			"<link rel='self' type='application/atom+xml' href='https://docs.google.com/feeds/default/private/full/folder%3A0BzY2jgHHwMwYalFhbjhVT3dyams'/>"
			"<link rel='edit' type='application/atom+xml' href='https://docs.google.com/feeds/default/private/full/folder%3A0BzY2jgHHwMwYalFhbjhVT3dyams'/>"
			"<author>"
				"<name>libgdata.documents</name>"
				"<email>libgdata.documents@gmail.com</email>"
			"</author>"
			"<gd:resourceId>folder:0BzY2jgHHwMwYalFhbjhVT3dyams</gd:resourceId>"
			"<gd:lastModifiedBy>"
				"<name>libgdata.documents</name>"
				"<email>libgdata.documents@gmail.com</email>"
			"</gd:lastModifiedBy>"
			"<gd:quotaBytesUsed>0</gd:quotaBytesUsed>"
			"<docs:writersCanInvite value='false'/>"
			"<gd:feedLink rel='http://schemas.google.com/acl/2007#accessControlList' href='https://docs.google.com/feeds/default/private/full/folder%3A0BzY2jgHHwMwYalFhbjhVT3dyams/acl'/>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_FOLDER (folder));
	gdata_test_compare_kind (GDATA_ENTRY (folder), "http://schemas.google.com/docs/2007#folder", NULL);

	/* Check IDs. */
	g_assert_cmpstr (gdata_documents_entry_get_resource_id (GDATA_DOCUMENTS_ENTRY (folder)), ==, "folder:0BzY2jgHHwMwYalFhbjhVT3dyams");

	path = gdata_documents_entry_get_path (GDATA_DOCUMENTS_ENTRY (folder));
	g_assert_cmpstr (path, ==, "/0BzY2jgHHwMwYalFhbjhVT3dyams");
	g_free (path);

	/* Check dates. */
	g_assert_cmpuint (gdata_documents_entry_get_edited (GDATA_DOCUMENTS_ENTRY (folder)), ==, 1334394740);
	g_assert_cmpuint (gdata_documents_entry_get_last_viewed (GDATA_DOCUMENTS_ENTRY (folder)), ==, -1);

	author = gdata_documents_entry_get_last_modified_by (GDATA_DOCUMENTS_ENTRY (folder));

	g_assert_cmpstr (gdata_author_get_name (author), ==, "libgdata.documents");
	g_assert_cmpstr (gdata_author_get_uri (author), ==, NULL);
	g_assert_cmpstr (gdata_author_get_email_address (author), ==, "libgdata.documents@gmail.com");

	/* Check permissions/quotas. */
	g_assert (gdata_documents_entry_writers_can_invite (GDATA_DOCUMENTS_ENTRY (folder)) == FALSE);
	g_assert_cmpuint (gdata_documents_entry_get_quota_used (GDATA_DOCUMENTS_ENTRY (folder)), ==, 0);

	/* Check miscellany. */
	g_assert (gdata_documents_entry_is_deleted (GDATA_DOCUMENTS_ENTRY (folder)) == FALSE);

	g_object_unref (folder);
}

static void
test_query_etag (void)
{
	GDataDocumentsQuery *query = gdata_documents_query_new (NULL);

	/* Test that setting any property will unset the ETag */
	g_test_bug ("613529");

#define CHECK_ETAG(C) \
	gdata_query_set_etag (GDATA_QUERY (query), "foobar"); \
	(C); \
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

/* Here we hardcode the feed URI, but it should really be extracted from a document feed, as the GDATA_LINK_BATCH link */
#define BATCH_URI "https://docs.google.com/feeds/default/private/full/batch"

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

	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_documents_service_get_primary_authorization_domain (),
	                                              BATCH_URI);

	/* Check the properties of the operation */
	g_assert (gdata_batch_operation_get_service (operation) == service);
	g_assert_cmpstr (gdata_batch_operation_get_feed_uri (operation), ==, BATCH_URI);

	g_object_get (operation,
	              "service", &service2,
	              "feed-uri", &feed_uri,
	              NULL);

	g_assert (service2 == service);
	g_assert_cmpstr (feed_uri, ==, BATCH_URI);

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
	                                              BATCH_URI);
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
	                                              BATCH_URI);
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
	                                              BATCH_URI);
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
	                                              BATCH_URI);
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
	                                              BATCH_URI);
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
	                                              BATCH_URI);
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
	                                              BATCH_URI);
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
	                                              BATCH_URI);
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
		g_test_add ("/documents/authentication/async", GDataAsyncTestData, NULL, gdata_set_up_async_test_data, test_authentication_async,
		            gdata_tear_down_async_test_data);
		g_test_add ("/documents/authentication/async/cancellation", GDataAsyncTestData, NULL, gdata_set_up_async_test_data,
		            test_authentication_async_cancellation, gdata_tear_down_async_test_data);

		g_test_add ("/documents/delete/document", TempDocumentData, service, set_up_temp_document_spreadsheet, test_delete_document,
		            tear_down_temp_document);
		g_test_add ("/documents/delete/folder", TempFolderData, service, set_up_temp_folder, test_delete_folder, tear_down_temp_folder);

		/* Test all possible combinations of conditions for resumable uploads. */
		{
			PayloadType i;
			FolderType j;
			ResumableType k;
			DocumentType l;

			for (i = 0; i < UPLOAD_PAYLOAD_TYPE_MAX + 1; i++) {
				for (j = 0; j < UPLOAD_FOLDER_TYPE_MAX + 1; j++) {
					for (k = 0; k < UPLOAD_RESUMABLE_TYPE_MAX + 1; k++) {
						for (l = 0; l < UPLOAD_DOCUMENT_TYPE_MAX + 1; l++) {
							UploadDocumentTestParams *test_params;
							gchar *test_name;

							/* Resumable metadata-only uploads don't make sense. */
							if (i == UPLOAD_METADATA_ONLY && k == UPLOAD_RESUMABLE) {
								continue;
							}
							/* Non-resumable non-conversion uploads don't make sense. */
							if (k == UPLOAD_NON_RESUMABLE && l != UPLOAD_ODT_CONVERT) {
								continue;
							}

							test_name = g_strdup_printf ("/documents/upload/%s/%s/%s/%s",
							                             payload_type_names[i], folder_type_names[j],
							                             resumable_type_names[k], document_type_names[l]);

							/* Allocate a new struct. We leak this. */
							test_params = g_slice_new0 (UploadDocumentTestParams);
							test_params->payload_type = i;
							test_params->folder_type = j;
							test_params->resumable_type = k;
							test_params->document_type = l;
							test_params->test_name = g_strdup (test_name);
							test_params->service = GDATA_DOCUMENTS_SERVICE (service);

							g_test_add (test_name, UploadDocumentData, test_params, set_up_upload_document, test_upload,
							            tear_down_upload_document);

							g_free (test_name);
						}
					}
				}
			}
		}

		g_test_add ("/documents/download/document", TempDocumentsData, service, set_up_temp_documents, test_download_document,
		            tear_down_temp_documents);
		g_test_add ("/documents/download/thumbnail", TempDocumentData, service, set_up_temp_document_spreadsheet, test_download_thumbnail,
		            tear_down_temp_document);

		/* Test all possible combinations of conditions for resumable updates. */
		{
			PayloadType i;
			ResumableType j;

			for (i = 0; i < UPLOAD_PAYLOAD_TYPE_MAX + 1; i++) {
				for (j = 0; j < UPLOAD_RESUMABLE_TYPE_MAX + 1; j++) {
					UpdateDocumentTestParams *test_params;
					gchar *test_name;

					/* Resumable metadata-only updates don't make sense. */
					if (i == UPLOAD_METADATA_ONLY && j == UPLOAD_RESUMABLE) {
						continue;
					}

					test_name = g_strdup_printf ("/documents/update/%s/%s", payload_type_names[i], resumable_type_names[j]);

					/* Allocate a new struct. We leak this. */
					test_params = g_slice_new0 (UpdateDocumentTestParams);
					test_params->payload_type = i;
					test_params->resumable_type = j;
					test_params->test_name = g_strdup (test_name);
					test_params->service = GDATA_DOCUMENTS_SERVICE (service);

					g_test_add (test_name, UpdateDocumentData, test_params, set_up_update_document, test_update,
					            tear_down_update_document);

					g_free (test_name);
				}
			}
		}

		g_test_add ("/documents/access-rule/insert", TempDocumentData, service, set_up_temp_document_spreadsheet, test_access_rule_insert,
		            tear_down_temp_document);

		g_test_add ("/documents/query/all_documents", TempDocumentsData, service, set_up_temp_documents, test_query_all_documents,
		            tear_down_temp_documents);
		g_test_add ("/documents/query/all_documents/with_folder", TempDocumentsData, service, set_up_temp_documents,
		            test_query_all_documents_with_folder, tear_down_temp_documents);
		g_test_add ("/documents/query/all_documents/async", GDataAsyncTestData, service, set_up_temp_documents_async,
		            test_query_all_documents_async, tear_down_temp_documents_async);
		g_test_add ("/documents/query/all_documents/async/progress_closure", TempDocumentsData, service, set_up_temp_documents,
		            test_query_all_documents_async_progress_closure, tear_down_temp_documents);
		g_test_add ("/documents/query/all_documents/async/cancellation", GDataAsyncTestData, service, set_up_temp_documents_async,
		            test_query_all_documents_async_cancellation, tear_down_temp_documents_async);

		g_test_add ("/documents/copy", TempCopyDocumentData, service, set_up_copy_document, test_copy_document, tear_down_copy_document);
		/*g_test_add ("/documents/copy/async", GDataAsyncTestData, service, set_up_folders_add_to_folder_async,
		            test_folders_add_to_folder_async, tear_down_folders_add_to_folder_async);
		g_test_add ("/documents/copy/async/cancellation", GDataAsyncTestData, service, set_up_folders_add_to_folder_async,
		            test_folders_add_to_folder_async_cancellation, tear_down_folders_add_to_folder_async);*/

		g_test_add ("/documents/folders/add_to_folder", FoldersData, service, set_up_folders_add_to_folder,
		            test_folders_add_to_folder, tear_down_folders_add_to_folder);
		g_test_add ("/documents/folders/add_to_folder/async", GDataAsyncTestData, service, set_up_folders_add_to_folder_async,
		            test_folders_add_to_folder_async, tear_down_folders_add_to_folder_async);
		g_test_add ("/documents/folders/add_to_folder/async/cancellation", GDataAsyncTestData, service, set_up_folders_add_to_folder_async,
		            test_folders_add_to_folder_async_cancellation, tear_down_folders_add_to_folder_async);

		g_test_add ("/documents/folders/remove_from_folder", FoldersData, service, set_up_folders_remove_from_folder,
		            test_folders_remove_from_folder, tear_down_folders_remove_from_folder);
		g_test_add ("/documents/folders/remove_from_folder/async", GDataAsyncTestData, service, set_up_folders_remove_from_folder_async,
		            test_folders_remove_from_folder_async, tear_down_folders_remove_from_folder_async);
		g_test_add ("/documents/folders/remove_from_folder/async/cancellation", GDataAsyncTestData, service,
		            set_up_folders_remove_from_folder_async, test_folders_remove_from_folder_async_cancellation,
		            tear_down_folders_remove_from_folder_async);

		g_test_add_data_func ("/documents/batch", service, test_batch);
		g_test_add ("/documents/batch/async", BatchAsyncData, service, set_up_batch_async, test_batch_async, tear_down_batch_async);
		g_test_add ("/documents/batch/async/cancellation", BatchAsyncData, service, set_up_batch_async, test_batch_async_cancellation,
		            tear_down_batch_async);
	}

	g_test_add_func ("/documents/folder/parser/normal", test_folder_parser_normal);

	g_test_add_func ("/documents/query/etag", test_query_etag);

	retval = g_test_run ();

	if (service != NULL)
		g_object_unref (service);

	return retval;
}
