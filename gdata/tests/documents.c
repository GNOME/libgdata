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

#include <glib.h>
#include <unistd.h>

#include "gdata.h"
#include "common.h"

/* TODO: probably a better way to do this; some kind of data associated with the test suite? */
static GDataDocumentsService *service = NULL;

static void
test_authentication (void)
{
	gboolean retval;
	GError *error = NULL;

	/* Create a service */
	service = gdata_documents_service_new (CLIENT_ID);

	g_assert (service != NULL);
	g_assert (GDATA_IS_SERVICE (service));
	g_assert_cmpstr (gdata_service_get_client_id (GDATA_SERVICE (service)), ==, CLIENT_ID);

	/* Log in */
	retval = gdata_service_authenticate (GDATA_SERVICE (service), DOCUMENTS_USERNAME, PASSWORD, NULL, &error);
	g_assert_no_error (error);
	g_assert (retval == TRUE);
	g_clear_error (&error);

	/* Check all is as it should be */
	g_assert (gdata_service_is_authenticated (GDATA_SERVICE (service)) == TRUE);
	g_assert_cmpstr (gdata_service_get_username (GDATA_SERVICE (service)), ==, DOCUMENTS_USERNAME);
	g_assert_cmpstr (gdata_service_get_password (GDATA_SERVICE (service)), ==, PASSWORD);
}

static void
test_remove_all_documents_and_folders (GDataService *service)
{
	GDataDocumentsFeed *feed;
	GDataDocumentsQuery *query;
	GError *error = NULL;
	GList *i;

	g_assert (service != NULL);

	query = gdata_documents_query_new (NULL);
	gdata_documents_query_set_show_folders (query, TRUE);

	feed = gdata_documents_service_query_documents (GDATA_DOCUMENTS_SERVICE (service), query, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_FEED (feed));

	for (i = gdata_feed_get_entries (GDATA_FEED (feed)); i != NULL; i = i->next) {
		gdata_service_delete_entry (GDATA_SERVICE (service), GDATA_ENTRY (i->data), NULL, &error);
		g_assert_no_error (error);
	}

	g_clear_error (&error);

	/* TODO: check entries and feed properties */
	g_object_unref (feed);
}

static void
test_query_all_documents_with_folder (GDataService *service)
{
	GDataDocumentsFeed *feed;
	GDataDocumentsQuery *query;
	GError *error = NULL;

	g_assert (service != NULL);

	query = gdata_documents_query_new (NULL);
	gdata_documents_query_set_show_folders (query, TRUE);

	feed = gdata_documents_service_query_documents (GDATA_DOCUMENTS_SERVICE (service), query, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));

	g_clear_error (&error);
	g_object_unref (feed);
}

static void
test_query_all_documents (GDataService *service)
{
	GDataDocumentsFeed *feed;
	GError *error = NULL;

	g_assert (service != NULL);

	feed = gdata_documents_service_query_documents (GDATA_DOCUMENTS_SERVICE (service), NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* TODO: check entries and feed properties */

	g_object_unref (feed);
}

static void
test_query_all_documents_async_cb (GDataService *service, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GDataDocumentsFeed *feed;
	GError *error = NULL;

	feed = GDATA_DOCUMENTS_FEED (gdata_service_query_finish (GDATA_SERVICE (service), async_result, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* TODO: Tests? */
	g_main_loop_quit (main_loop);
	g_object_unref (feed);
}

static void
test_query_all_documents_async (GDataService *service)
{
	GMainLoop *main_loop = g_main_loop_new (NULL, TRUE);

	g_assert (service != NULL);

	gdata_documents_service_query_documents_async (GDATA_DOCUMENTS_SERVICE (service), NULL, NULL, NULL,
						     NULL, (GAsyncReadyCallback) test_query_all_documents_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
}

static void
test_upload_metadata (GDataService *service)
{
	GDataDocumentsEntry *document, *new_document;
	GDataCategory *category;
	GError *error = NULL;

	g_assert (service != NULL);

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_spreadsheet_new (NULL));
	category = gdata_category_new ("http://schemas.google.com/docs/2007#spreadsheet", "http://schemas.google.com/g/2005#kind", "spreadsheet");

	gdata_entry_set_title (GDATA_ENTRY (document), "myNewSpreadsheet");
	gdata_entry_add_category (GDATA_ENTRY (document), category);

	/* Insert the document */
	new_document = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), GDATA_DOCUMENTS_ENTRY (document), NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_SPREADSHEET (new_document));

	g_clear_error (&error);
	g_object_unref (document);
	g_object_unref (new_document);
}

static void
test_upload_metadata_file (GDataService *service)
{
	GDataDocumentsEntry *document, *new_document;
	GFile *document_file;
	GDataCategory *category;
	GError *error = NULL;

	g_assert (service != NULL);

	document_file = g_file_new_for_path (TEST_FILE_DIR "test.odt");

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_text_new (NULL));
	category = gdata_category_new ("http://schemas.google.com/docs/2007#document", "http://schemas.google.com/g/2005#kind", "document");
	gdata_entry_set_title (GDATA_ENTRY (document), "upload_metadata_file");
	gdata_entry_add_category (GDATA_ENTRY (document), category);

	/* Insert the document */
	new_document = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), document, document_file, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (new_document));

	g_clear_error (&error);
	g_object_unref (document_file);
	g_object_unref (document);
	g_object_unref (new_document);
}

static void
test_upload_file (GDataService *service)
{
	GDataDocumentsEntry *new_document;
	GFile *document_file;
	GDataCategory *category;
	GError *error = NULL;

	g_assert (service != NULL);

	document_file = g_file_new_for_path (TEST_FILE_DIR "test.ppt");

	category = gdata_category_new ("http://schemas.google.com/docs/2007#presentation", "http://schemas.google.com/g/2005#kind", "presentation");

	/* Insert the document */
	new_document = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), NULL, document_file, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_PRESENTATION (new_document));

	g_clear_error (&error);
	g_object_unref (new_document);
	g_object_unref (document_file);
}

static void
test_add_remove_file_from_folder (GDataService *service)
{
	GDataDocumentsEntry *document, *new_document, *new_document2;
	GDataDocumentsFolder *folder, *new_folder;
	GFile *document_file;
	GDataCategory *folder_category, *document_category;
	GError *error = NULL;

	g_assert (service != NULL);

	folder = gdata_documents_folder_new (NULL);
	folder_category = gdata_category_new ("http://schemas.google.com/docs/2007#folder", "http://schemas.google.com/g/2005#kind", "folder");
	gdata_entry_set_title (GDATA_ENTRY (folder), "add_remove_from_folder_folder");
	gdata_entry_add_category (GDATA_ENTRY (folder), folder_category);

	document_file = g_file_new_for_path (TEST_FILE_DIR "test.ppt");
	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_presentation_new (NULL));
	document_category = gdata_category_new ("http://schemas.google.com/docs/2007#presentation", "http://schemas.google.com/g/2005#kind", "presentation");
	gdata_entry_set_title (GDATA_ENTRY (document), "add_remove_from_folder_presentation");
	gdata_entry_add_category (GDATA_ENTRY (document), document_category);

	/* Insert the folder */
	new_folder = GDATA_DOCUMENTS_FOLDER (gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), GDATA_DOCUMENTS_ENTRY (folder), NULL, NULL, NULL, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_FOLDER (new_folder));

	/* Insert the document in the new folder */
	new_document = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), document, document_file, new_folder, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_PRESENTATION (new_document));

	/*remove document from the folder*/
	new_document2 = gdata_documents_service_remove_document_from_folder (GDATA_DOCUMENTS_SERVICE (service), new_document, new_folder, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_PRESENTATION (new_document2));

	g_clear_error (&error);
	g_object_unref (document);
	g_object_unref (new_document);
	g_object_unref (new_document2);
	g_object_unref (folder);
	g_object_unref (new_folder);
	g_object_unref (document_file);
}

static void
test_add_file_folder_and_move (GDataService *service)
{
	GDataDocumentsEntry *document, *new_document, *new_document2;
	GDataDocumentsFolder *folder, *new_folder;
	GFile *document_file;
	GDataCategory *folder_category, *document_category;
	GError *error = NULL;

	g_assert (service != NULL);

	document_file = g_file_new_for_path (TEST_FILE_DIR "test.odt");

	folder = gdata_documents_folder_new (NULL);
	folder_category = gdata_category_new ("http://schemas.google.com/docs/2007#folder", "http://schemas.google.com/g/2005#kind", "folder");
	gdata_entry_set_title (GDATA_ENTRY (folder), "add_file_folder_move_folder");
	gdata_entry_add_category (GDATA_ENTRY (folder), folder_category);

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_text_new (NULL));
	document_category = gdata_category_new ("http://schemas.google.com/docs/2007#document", "http://schemas.google.com/g/2005#kind", "document");
	gdata_entry_set_title (GDATA_ENTRY (document), "add_file_folder_move_text");
	gdata_entry_add_category (GDATA_ENTRY (document), document_category);

	/* Insert the folder */
	new_folder = GDATA_DOCUMENTS_FOLDER (gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), GDATA_DOCUMENTS_ENTRY (folder), NULL, NULL, NULL, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_FOLDER (new_folder));

	/* Insert the document in the new folder */
	new_document = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), document, document_file, NULL, NULL, &error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (new_document));

	/* Remove the document from the folder */
	new_document2 = gdata_documents_service_move_document_to_folder (GDATA_DOCUMENTS_SERVICE (service), new_document, new_folder, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (new_document2));

	g_clear_error (&error);
	g_object_unref (document);
	g_object_unref (new_document);
	g_object_unref (new_document2);
	g_object_unref (folder);
	g_object_unref (new_folder);
	g_object_unref (document_file);
}

static void
test_upload_file_metadata_in_new_folder (GDataService *service)
{
	GDataDocumentsEntry *document, *new_document;
	GDataDocumentsFolder *folder, *new_folder;
	GFile *document_file;
	GDataCategory *folder_category, *document_category;
	GError *error = NULL;

	g_assert (service != NULL);

	document_file = g_file_new_for_path (TEST_FILE_DIR "test.odt");

	folder = gdata_documents_folder_new (NULL);
	folder_category = gdata_category_new ("http://schemas.google.com/docs/2007#folder", "http://schemas.google.com/g/2005#kind", "folder");
	gdata_entry_set_title (GDATA_ENTRY (folder), "upload_file_metadata_in_new_folder_folder");
	gdata_entry_add_category (GDATA_ENTRY (folder), folder_category);

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_text_new (NULL));
	document_category = gdata_category_new ("http://schemas.google.com/docs/2007#document", "http://schemas.google.com/g/2005#kind", "document");
	gdata_entry_set_title (GDATA_ENTRY (document), "upload_file_metadata_in_new_folder_text");
	gdata_entry_add_category (GDATA_ENTRY (document), document_category);

	/* Insert the folder */
	new_folder = GDATA_DOCUMENTS_FOLDER (gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), GDATA_DOCUMENTS_ENTRY (folder), NULL, NULL, NULL, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_FOLDER (new_folder));
	g_clear_error (&error);

	/* Insert the document in the new folder */
	new_document = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), document, document_file, new_folder, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (new_document));

	g_clear_error (&error);
	g_object_unref (document);
	g_object_unref (new_document);
	g_object_unref (folder);
	g_object_unref (new_folder);
	g_object_unref (document_file);
}

static void
test_update_metadata (GDataService *service)
{
	GDataDocumentsEntry *document, *new_document, *updated_document;
	GDataCategory *category;
	GError *error = NULL;

	g_assert (service != NULL);

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_text_new (NULL));
	category = gdata_category_new ("http://schemas.google.com/docs/2007#document", "http://schemas.google.com/g/2005#kind", "document");
	gdata_entry_set_title (GDATA_ENTRY (document), "update_metadata_first_title");
	gdata_entry_add_category (GDATA_ENTRY (document), category);

	/* Insert the document */
	new_document = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), document, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (new_document));

	/* Change the title */
	gdata_entry_set_title (GDATA_ENTRY (document), "update_metadata_updated_title");

	/* Update the document */
	updated_document = gdata_documents_service_update_document (GDATA_DOCUMENTS_SERVICE (service), new_document, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (updated_document));

	g_clear_error (&error);
	g_object_unref (document);
	g_object_unref (new_document);
	g_object_unref (updated_document);
}

static void
test_update_metadata_file (GDataService *service)
{
	GDataDocumentsEntry *document, *new_document, *updated_document;
	GFile *document_file, *updated_document_file;
	GDataCategory *category;
	GError *error = NULL;

	g_assert (service != NULL);

	document_file = g_file_new_for_path (TEST_FILE_DIR "test.odt");
	updated_document_file = g_file_new_for_path (TEST_FILE_DIR "test_updated.odt");

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_text_new (NULL));
	category = gdata_category_new ("http://schemas.google.com/docs/2007#document", "http://schemas.google.com/g/2005#kind", "document");
	gdata_entry_set_title (GDATA_ENTRY (document), "update_metadata_file_first_title");
	gdata_entry_add_category (GDATA_ENTRY (document), category);

	/* Insert the documents metadata*/
	new_document = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), document, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (new_document));

	/* Change the title of the document */
	gdata_entry_set_title (GDATA_ENTRY (new_document), "update_metadata_file_updated_title");

	/* Update the document */
	updated_document = gdata_documents_service_update_document (GDATA_DOCUMENTS_SERVICE (service), new_document, updated_document_file, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (updated_document));

	g_clear_error (&error);
	g_object_unref (document);
	g_object_unref (new_document);
	g_object_unref (updated_document);
	g_object_unref (document_file);
}

static void
test_update_file (GDataService *service)
{
	GDataDocumentsEntry *new_document, *updated_document;
	GFile *document_file, *updated_document_file;
	GError *error = NULL;

	g_assert (service != NULL);

	document_file = g_file_new_for_path (TEST_FILE_DIR "test.ppt");
	updated_document_file = g_file_new_for_path (TEST_FILE_DIR "test_updated_file.ppt");

	/* Insert the document */
	new_document = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), NULL, document_file, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_PRESENTATION (new_document));

	/* Update the document */
	updated_document = gdata_documents_service_update_document (GDATA_DOCUMENTS_SERVICE (service), new_document, document_file, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_PRESENTATION (updated_document));
	g_clear_error (&error);

	g_object_unref (document_file);
	g_object_unref (updated_document_file);
	g_object_unref (new_document);
	g_object_unref (updated_document);
}

static void
test_download_all_documents (GDataService *service)
{
	GDataDocumentsFeed *feed;
	GError *error = NULL;
	gchar *content_type = NULL;
	GFile *destination_directory;
	GList *i;

	destination_directory = g_file_new_for_path ("/tmp");
	feed = gdata_documents_service_query_documents (GDATA_DOCUMENTS_SERVICE (service), NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));

	for (i = gdata_feed_get_entries (GDATA_FEED (feed)); i != NULL; i = i->next) {
		GFile *destination_file = NULL;

		if (GDATA_IS_DOCUMENTS_PRESENTATION (i->data)) {
			destination_file = gdata_documents_presentation_download_document (GDATA_DOCUMENTS_PRESENTATION (i->data), GDATA_DOCUMENTS_SERVICE (service),
											   &content_type, GDATA_DOCUMENTS_PRESENTATION_PPT, destination_directory,
											   TRUE, NULL, &error);
		} else if (GDATA_IS_DOCUMENTS_SPREADSHEET (i->data)) {
			destination_file = gdata_documents_spreadsheet_download_document (i->data, GDATA_DOCUMENTS_SERVICE (service),
											  &content_type, GDATA_DOCUMENTS_SPREADSHEET_ODS, -1, destination_directory,
											  TRUE, NULL, &error);
		} else if (GDATA_IS_DOCUMENTS_TEXT (i->data)) {
			destination_file = gdata_documents_text_download_document (i->data, GDATA_DOCUMENTS_SERVICE (service), &content_type, GDATA_DOCUMENTS_TEXT_ODT,
										   destination_directory, TRUE, NULL, &error);
		}

		g_assert_no_error (error);
		if (destination_file != NULL)
			g_object_unref (destination_file);
		g_free (content_type);
	}

	g_object_unref (destination_directory);
	g_object_unref (feed);
	g_clear_error (&error);
}

static void
test_new_document_with_collaborator (GDataService *service)
{
	GDataDocumentsEntry *document, *new_document;
	GDataAccessRule *access_rule, *new_access_rule;
	GDataCategory *category;
	GError *error = NULL;

	g_assert (service != NULL);

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_spreadsheet_new (NULL));
	category = gdata_category_new ("http://schemas.google.com/docs/2007#spreadsheet", "http://schemas.google.com/g/2005#kind", "spreadsheet");

	gdata_entry_set_title (GDATA_ENTRY (document), "new_with_collaborator");
	gdata_entry_add_category (GDATA_ENTRY (document), category);

	/* Insert the document */
	new_document = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), GDATA_DOCUMENTS_ENTRY (document), NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_SPREADSHEET (new_document));

	/* New access rule */
	access_rule = gdata_access_rule_new (NULL);
	gdata_access_rule_set_role (access_rule, "writer");
	gdata_access_rule_set_scope (access_rule, "user", "libgdata.test@gmail.com");

	/* Set access rules */
	new_access_rule = gdata_access_handler_insert_rule (GDATA_ACCESS_HANDLER (new_document), GDATA_SERVICE (service), access_rule, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_ACCESS_RULE (new_access_rule));

	/* Check if everything is as it should be */
	g_clear_error (&error);
	g_object_unref (document);
	g_object_unref (new_document);
	g_object_unref (access_rule);
	g_object_unref (new_access_rule);
}

int
main (int argc, char *argv[])
{
	GDataService *service;
	gint retval;

	g_type_init ();
	g_thread_init (NULL);
	g_test_init (&argc, &argv, NULL);
	g_test_bug_base ("http://bugzilla.gnome.org/show_bug.cgi?id=");

	service = GDATA_SERVICE (gdata_documents_service_new (CLIENT_ID));
	gdata_service_authenticate (service, DOCUMENTS_USERNAME, PASSWORD, NULL, NULL);

	g_test_add_func ("/documents/authentication", test_authentication);

	g_test_add_data_func ("/documents/remove/all", service, test_remove_all_documents_and_folders);

	g_test_add_data_func ("/documents/upload/only_file", service, test_upload_file);
	g_test_add_data_func ("/documents/upload/metadata_file", service, test_upload_metadata_file);
	g_test_add_data_func ("/documents/upload/only_metadata", service, test_upload_metadata);
	g_test_add_data_func ("/documents/upload/metadata_file_in_new_folder", service, test_upload_file_metadata_in_new_folder);

	g_test_add_data_func ("/documents/download/download_all_documents", service, test_download_all_documents);

	g_test_add_data_func ("/documents/update/only_metadata", service, test_update_metadata);
	g_test_add_data_func ("/documents/update/only_file", service, test_update_file);
	g_test_add_data_func ("/documents/update/metadata_file", service, test_update_metadata_file);

	g_test_add_data_func ("/documents/access_rules/add_document_with_a_collaborator", service,
			      test_new_document_with_collaborator);

	g_test_add_data_func ("/documents/query/all_documents_with_folder", service, test_query_all_documents_with_folder);
	g_test_add_data_func ("/documents/query/all_documents", service, test_query_all_documents);
	if (g_test_thorough () == TRUE)
		g_test_add_data_func ("/documents/query/all_documents_async", service, test_query_all_documents_async);

	g_test_add_data_func ("/documents/move/move_to_folder", service, test_add_file_folder_and_move);

	g_test_add_data_func ("/documents/move/remove_from_folder", service, test_add_remove_file_from_folder);

	g_test_add_data_func ("/documents/remove/all", service, test_remove_all_documents_and_folders);
	retval = g_test_run ();

	g_object_unref (service);

	return retval;
}
