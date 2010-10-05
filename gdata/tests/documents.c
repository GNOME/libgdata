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

#include "gdata.h"
#include "common.h"

static void
test_authentication (void)
{
	gboolean retval;
	GDataDocumentsService *service;
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
test_remove_all_documents_and_folders (gconstpointer service)
{
	GDataDocumentsFeed *feed;
	GDataDocumentsQuery *query;
	GError *error = NULL;
	GList *i;

	g_assert (service != NULL);

	query = gdata_documents_query_new (NULL);
	gdata_documents_query_set_show_folders (query, FALSE);

	feed = gdata_documents_service_query_documents (GDATA_DOCUMENTS_SERVICE (service), query, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_FEED (feed));

	/* We delete the folders after all the files so we don't get ETag mismatches; deleting a folder changes the version
	 * of all the documents inside it. Conversely, deleting an entry inside a folder changes the version of the folder. */
	for (i = gdata_feed_get_entries (GDATA_FEED (feed)); i != NULL; i = i->next) {
		gdata_service_delete_entry (GDATA_SERVICE (service), GDATA_ENTRY (i->data), NULL, &error);
		g_assert_no_error (error);
		g_clear_error (&error);
	}

	g_object_unref (feed);

	/* Now delete the folders */
	gdata_documents_query_set_show_folders (query, TRUE);

	feed = gdata_documents_service_query_documents (GDATA_DOCUMENTS_SERVICE (service), query, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_FEED (feed));

	for (i = gdata_feed_get_entries (GDATA_FEED (feed)); i != NULL; i = i->next) {
		gdata_service_delete_entry (GDATA_SERVICE (service), GDATA_ENTRY (i->data), NULL, &error);
		g_assert_no_error (error);
		g_clear_error (&error);
	}

	g_object_unref (feed);
}

static void
test_query_all_documents_with_folder (gconstpointer service)
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
test_query_all_documents (gconstpointer service)
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
test_query_all_documents_async (gconstpointer service)
{
	GMainLoop *main_loop = g_main_loop_new (NULL, TRUE);

	g_assert (service != NULL);

	gdata_documents_service_query_documents_async (GDATA_DOCUMENTS_SERVICE (service), NULL, NULL, NULL,
						     NULL, (GAsyncReadyCallback) test_query_all_documents_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
}

static void
test_upload_metadata (gconstpointer service)
{
	GDataDocumentsEntry *document, *new_document;
	GError *error = NULL;

	g_assert (service != NULL);

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_spreadsheet_new (NULL));
	gdata_entry_set_title (GDATA_ENTRY (document), "myNewSpreadsheet");

	/* Insert the document */
	new_document = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), GDATA_DOCUMENTS_ENTRY (document), NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_SPREADSHEET (new_document));

	g_clear_error (&error);
	g_object_unref (document);
	g_object_unref (new_document);
}

static void
test_upload_metadata_file (gconstpointer service)
{
	GDataDocumentsEntry *document, *new_document;
	GFile *document_file;
	GError *error = NULL;

	g_assert (service != NULL);

	document_file = g_file_new_for_path (TEST_FILE_DIR "test.odt");

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_text_new (NULL));
	gdata_entry_set_title (GDATA_ENTRY (document), "upload_metadata_file");

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
test_upload_file_get_entry (gconstpointer service)
{
	GDataDocumentsEntry *new_document;
	GDataEntry *new_presentation;
	GFile *document_file;
	GError *error = NULL;

	g_assert (service != NULL);

	document_file = g_file_new_for_path (TEST_FILE_DIR "test.ppt");

	/* Insert the document */
	new_document = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), NULL, document_file, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_PRESENTATION (new_document));

	/* Get the entry on the server */
	new_presentation = gdata_service_query_single_entry (GDATA_SERVICE (service), gdata_entry_get_id (GDATA_ENTRY (new_document)), NULL,
	                                                     GDATA_TYPE_DOCUMENTS_PRESENTATION, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_PRESENTATION (new_presentation));

	g_clear_error (&error);
	g_object_unref (new_document);
	g_object_unref (new_presentation);
	g_object_unref (document_file);
}

static void
test_add_remove_file_from_folder (gconstpointer service)
{
	GDataDocumentsEntry *document, *new_document, *new_document2;
	GDataDocumentsFolder *folder, *new_folder;
	GFile *document_file;
	GError *error = NULL;

	g_assert (service != NULL);

	folder = gdata_documents_folder_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (folder), "add_remove_from_folder_folder");

	document_file = g_file_new_for_path (TEST_FILE_DIR "test.ppt");
	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_presentation_new (NULL));
	gdata_entry_set_title (GDATA_ENTRY (document), "add_remove_from_folder_presentation");

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
test_add_file_folder_and_move (gconstpointer service)
{
	GDataDocumentsEntry *document, *new_document, *new_document2;
	GDataDocumentsFolder *folder, *new_folder;
	GFile *document_file;
	GError *error = NULL;

	g_assert (service != NULL);

	document_file = g_file_new_for_path (TEST_FILE_DIR "test.odt");

	folder = gdata_documents_folder_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (folder), "add_file_folder_move_folder");

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_text_new (NULL));
	gdata_entry_set_title (GDATA_ENTRY (document), "add_file_folder_move_text");

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
test_upload_file_metadata_in_new_folder (gconstpointer service)
{
	GDataDocumentsEntry *document, *new_document;
	GDataDocumentsFolder *folder, *new_folder;
	GFile *document_file;
	GError *error = NULL;

	g_assert (service != NULL);

	document_file = g_file_new_for_path (TEST_FILE_DIR "test.odt");

	folder = gdata_documents_folder_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (folder), "upload_file_metadata_in_new_folder_folder");

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_text_new (NULL));
	gdata_entry_set_title (GDATA_ENTRY (document), "upload_file_metadata_in_new_folder_text");

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
test_update_metadata (gconstpointer service)
{
	GDataDocumentsEntry *document, *new_document, *updated_document;
	GError *error = NULL;

	g_assert (service != NULL);

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_text_new (NULL));
	gdata_entry_set_title (GDATA_ENTRY (document), "update_metadata_first_title");

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
test_update_metadata_file (gconstpointer service)
{
	GDataDocumentsEntry *document, *new_document, *updated_document;
	GFile *document_file, *updated_document_file;
	GError *error = NULL;

	g_assert (service != NULL);

	document_file = g_file_new_for_path (TEST_FILE_DIR "test.odt");
	updated_document_file = g_file_new_for_path (TEST_FILE_DIR "test_updated.odt");

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_text_new (NULL));
	gdata_entry_set_title (GDATA_ENTRY (document), "update_metadata_file_first_title");

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
test_update_file (gconstpointer service)
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
test_download_all_documents (gconstpointer service)
{
	GDataDocumentsFeed *feed;
	GError *error = NULL;
	gchar *content_type = NULL;
	GFile *destination_file;
	gchar *destination_file_name;
	GString *destination_display_name;
	GList *i;
	gint ods_nb = 0, odt_nb = 0;

	feed = gdata_documents_service_query_documents (GDATA_DOCUMENTS_SERVICE (service), NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));

	for (i = gdata_feed_get_entries (GDATA_FEED (feed)); i != NULL; i = i->next) {
		if (GDATA_IS_DOCUMENTS_PRESENTATION (i->data)) {
			destination_file = g_file_new_for_path ("/tmp");
			destination_file = gdata_documents_document_download (GDATA_DOCUMENTS_DOCUMENT (i->data), GDATA_DOCUMENTS_SERVICE (service),
			                                                      &content_type, GDATA_DOCUMENTS_PRESENTATION_PPT, destination_file,
			                                                      TRUE, NULL, &error);
		} else if (GDATA_IS_DOCUMENTS_SPREADSHEET (i->data)) {
			destination_file_name = g_strdup_printf ("/tmp/%s.%s", gdata_documents_entry_get_document_id (GDATA_DOCUMENTS_ENTRY (i->data)), "ods");
			destination_file = g_file_new_for_path (destination_file_name);
			g_free (destination_file_name);
			destination_file = gdata_documents_document_download (GDATA_DOCUMENTS_DOCUMENT (i->data), GDATA_DOCUMENTS_SERVICE (service),
			                                                      &content_type, GDATA_DOCUMENTS_SPREADSHEET_ODS, destination_file,
			                                                      TRUE, NULL, &error);
			g_assert_no_error (error);

			destination_display_name = g_string_new (gdata_entry_get_title (GDATA_ENTRY(i->data)));
			g_string_append (destination_display_name, ".ods");
			g_file_set_display_name (destination_file, destination_display_name->str, NULL, &error);
			while (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
				g_clear_error (&error);
				error = NULL;
				g_string_printf (destination_display_name,"%s%d.%s", gdata_entry_get_title (GDATA_ENTRY(i->data)), ods_nb, "ods");
				g_file_set_display_name (destination_file, destination_display_name->str, NULL, &error);
				ods_nb++;
			}
			g_string_free (destination_display_name, TRUE);
		} else if (GDATA_IS_DOCUMENTS_TEXT (i->data)) {
			destination_file_name = g_strdup_printf ("/tmp/%s.%s", gdata_documents_entry_get_document_id (GDATA_DOCUMENTS_ENTRY (i->data)), "odt");
			destination_file = g_file_new_for_path (destination_file_name);
			g_free (destination_file_name);
			destination_file = gdata_documents_document_download (GDATA_DOCUMENTS_DOCUMENT (i->data), GDATA_DOCUMENTS_SERVICE (service),
			                                                      &content_type, GDATA_DOCUMENTS_TEXT_ODT, destination_file, TRUE, NULL,
			                                                      &error);
			g_assert_no_error (error);

			destination_display_name = g_string_new (gdata_entry_get_title (GDATA_ENTRY(i->data)));
			g_string_append (destination_display_name, ".odt");
			g_file_set_display_name (destination_file, destination_display_name->str, NULL, &error);
			while (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS)) {
				g_clear_error (&error);
				g_string_printf (destination_display_name,"%s%d.%s", gdata_entry_get_title (GDATA_ENTRY(i->data)), odt_nb, "odt");
				g_file_set_display_name (destination_file, destination_display_name->str, NULL, &error);
				odt_nb++;
			}
			g_string_free (destination_display_name, TRUE);
		}

		g_assert_no_error (error);
		g_free (content_type);
	}

	g_object_unref (feed);
	g_clear_error (&error);
}

static void
test_new_document_with_collaborator (gconstpointer service)
{
	GDataDocumentsEntry *document, *new_document;
	GDataAccessRule *access_rule, *new_access_rule;
	GDataLink *_link;
	GError *error = NULL;

	g_assert (service != NULL);

	document = GDATA_DOCUMENTS_ENTRY (gdata_documents_spreadsheet_new (NULL));
	gdata_entry_set_title (GDATA_ENTRY (document), "new_with_collaborator");

	/* Insert the document */
	new_document = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), GDATA_DOCUMENTS_ENTRY (document), NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_SPREADSHEET (new_document));

	/* New access rule */
	access_rule = gdata_access_rule_new (NULL);
	gdata_access_rule_set_role (access_rule, GDATA_DOCUMENTS_ACCESS_ROLE_WRITER);
	gdata_access_rule_set_scope (access_rule, GDATA_ACCESS_SCOPE_USER, "libgdata.test@gmail.com");

	/* Set access rules */
	_link = gdata_entry_look_up_link (GDATA_ENTRY (new_document), GDATA_LINK_ACCESS_CONTROL_LIST);
	g_assert (_link != NULL);

	new_access_rule = GDATA_ACCESS_RULE (gdata_service_insert_entry (GDATA_SERVICE (service), gdata_link_get_uri (_link),
	                                                                 GDATA_ENTRY (access_rule), NULL, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_ACCESS_RULE (new_access_rule));

	/* Check if everything is as it should be */
	g_clear_error (&error);
	g_object_unref (document);
	g_object_unref (new_document);
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
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "https://docs.google.com/feeds/documents/private/full/batch");

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
	g_assert (gdata_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);
	g_object_unref (doc);

	/* Run another batch operation to insert another entry and query the previous one */
	doc2 = gdata_documents_text_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (doc2), "I'm a poet and I didn't know it");

	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "https://docs.google.com/feeds/documents/private/full/batch");
	op_id = gdata_test_batch_operation_insertion (operation, GDATA_ENTRY (doc2), &inserted_entry2, NULL);
	op_id2 = gdata_test_batch_operation_query (operation, gdata_entry_get_id (inserted_entry), GDATA_TYPE_DOCUMENTS_TEXT, inserted_entry, NULL,
	                                           NULL);
	g_assert_cmpuint (op_id, !=, op_id2);

	g_assert (gdata_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);
	g_object_unref (doc2);

	/* Run another batch operation to query one of the entries we just created, since it seems that the ETags for documents change for no
	 * apparent reason when you're not looking. */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "https://docs.google.com/feeds/documents/private/full/batch");
	gdata_test_batch_operation_query (operation, gdata_entry_get_id (inserted_entry), GDATA_TYPE_DOCUMENTS_TEXT, inserted_entry,
	                                  &inserted_entry_updated, NULL);

	g_assert (gdata_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);
	g_object_unref (inserted_entry);

	/* Run another batch operation to query the other entry we just created. It would be sensible to batch this query together with the previous
	 * one, seeing as we're testing _batch_ functionality. Funnily enough, the combination of two idempotent operations changes the ETags and
	 * makes the whole effort worthless. */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "https://docs.google.com/feeds/documents/private/full/batch");
	gdata_test_batch_operation_query (operation, gdata_entry_get_id (inserted_entry2), GDATA_TYPE_DOCUMENTS_TEXT, inserted_entry2,
	                                  &inserted_entry2_updated, NULL);

	g_assert (gdata_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);

	/* Run another batch operation to delete the first entry and a fictitious one to test error handling, and update the second entry */
	gdata_entry_set_title (inserted_entry2_updated, "War & Peace");
	doc3 = gdata_documents_text_new ("foobar");

	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "https://docs.google.com/feeds/documents/private/full/batch");
	op_id = gdata_test_batch_operation_deletion (operation, inserted_entry_updated, NULL);
	op_id2 = gdata_test_batch_operation_deletion (operation, GDATA_ENTRY (doc3), &entry_error);
	op_id3 = gdata_test_batch_operation_update (operation, inserted_entry2_updated, &inserted_entry3, NULL);
	g_assert_cmpuint (op_id, !=, op_id2);
	g_assert_cmpuint (op_id, !=, op_id3);
	g_assert_cmpuint (op_id2, !=, op_id3);

	g_assert (gdata_batch_operation_run (operation, NULL, &error) == TRUE);
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
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "https://docs.google.com/feeds/documents/private/full/batch");
	gdata_test_batch_operation_update (operation, inserted_entry2, NULL, &entry_error);
	g_assert (gdata_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_assert_error (entry_error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_CONFLICT);

	g_clear_error (&error);
	g_clear_error (&entry_error);
	g_object_unref (operation);
	g_object_unref (inserted_entry2);

	/* Run a final batch operation to delete the second entry */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "https://docs.google.com/feeds/documents/private/full/batch");
	gdata_test_batch_operation_deletion (operation, inserted_entry3, NULL);
	g_assert (gdata_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);
	g_object_unref (inserted_entry3);
}

typedef struct {
	GDataDocumentsEntry *new_doc;
} BatchAsyncData;

static void
setup_batch_async (BatchAsyncData *data, gconstpointer service)
{
	GDataDocumentsText *doc;
	GError *error = NULL;

	/* Insert a new document which we can query asyncly */
	doc = gdata_documents_text_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (doc), "A View from the Bridge");

	data->new_doc = gdata_documents_service_upload_document (GDATA_DOCUMENTS_SERVICE (service), GDATA_DOCUMENTS_ENTRY (doc), NULL, NULL, NULL,
	                                                         &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOCUMENTS_TEXT (data->new_doc));
	g_clear_error (&error);

	g_object_unref (doc);
}

static void
test_batch_async_cb (GDataBatchOperation *operation, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GError *error = NULL;

	g_assert (gdata_batch_operation_run_finish (operation, async_result, &error) == TRUE);
	g_assert_no_error (error);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
}

static void
test_batch_async (BatchAsyncData *data, gconstpointer service)
{
	GDataBatchOperation *operation;
	guint op_id;
	GMainLoop *main_loop;

	/* Run an async query operation on the document */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "https://docs.google.com/feeds/documents/private/full/batch");
	op_id = gdata_test_batch_operation_query (operation, gdata_entry_get_id (GDATA_ENTRY (data->new_doc)), GDATA_TYPE_DOCUMENTS_TEXT,
	                                          GDATA_ENTRY (data->new_doc), NULL, NULL);

	main_loop = g_main_loop_new (NULL, TRUE);

	gdata_batch_operation_run_async (operation, NULL, (GAsyncReadyCallback) test_batch_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
}

static void
test_batch_async_cancellation_cb (GDataBatchOperation *operation, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GError *error = NULL;

	g_assert (gdata_batch_operation_run_finish (operation, async_result, &error) == FALSE);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
}

static void
test_batch_async_cancellation (BatchAsyncData *data, gconstpointer service)
{
	GDataBatchOperation *operation;
	guint op_id;
	GMainLoop *main_loop;
	GCancellable *cancellable;

	/* Run an async query operation on the document */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "https://docs.google.com/feeds/documents/private/full/batch");
	op_id = gdata_test_batch_operation_query (operation, gdata_entry_get_id (GDATA_ENTRY (data->new_doc)), GDATA_TYPE_DOCUMENTS_TEXT,
	                                          GDATA_ENTRY (data->new_doc), NULL, NULL);

	main_loop = g_main_loop_new (NULL, TRUE);
	cancellable = g_cancellable_new ();

	gdata_batch_operation_run_async (operation, cancellable, (GAsyncReadyCallback) test_batch_async_cancellation_cb, main_loop);
	g_cancellable_cancel (cancellable); /* this should cancel the operation before it even starts, as we haven't run the main loop yet */

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
	g_object_unref (cancellable);
}

static void
teardown_batch_async (BatchAsyncData *data, gconstpointer service)
{
	GDataEntry *document;
	GError *error = NULL;

	/* Re-query the document in case its ETag has changed */
	document = gdata_service_query_single_entry (GDATA_SERVICE (service), gdata_entry_get_id (GDATA_ENTRY (data->new_doc)), NULL,
	                                             GDATA_TYPE_DOCUMENTS_TEXT, NULL, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	/* Delete the document */
	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), document, NULL, &error) == TRUE);
	g_assert_no_error (error);
	g_clear_error (&error);

	g_object_unref (data->new_doc);
	g_object_unref (document);
}

int
main (int argc, char *argv[])
{
	gint retval;
	GDataService *service = NULL;

	gdata_test_init (argc, argv);

	if (gdata_test_internet () == TRUE) {
		service = GDATA_SERVICE (gdata_documents_service_new (CLIENT_ID));
		gdata_service_authenticate (service, DOCUMENTS_USERNAME, PASSWORD, NULL, NULL);

		g_test_add_func ("/documents/authentication", test_authentication);

		g_test_add_data_func ("/documents/remove/all", service, test_remove_all_documents_and_folders);

		g_test_add_data_func ("/documents/upload/only_file_get_entry", service, test_upload_file_get_entry);
		g_test_add_data_func ("/documents/upload/metadata_file", service, test_upload_metadata_file);
		g_test_add_data_func ("/documents/upload/only_metadata", service, test_upload_metadata);
		g_test_add_data_func ("/documents/upload/metadata_file_in_new_folder", service, test_upload_file_metadata_in_new_folder);

		g_test_add_data_func ("/documents/download/download_all_documents", service, test_download_all_documents);

		g_test_add_data_func ("/documents/update/only_metadata", service, test_update_metadata);
		g_test_add_data_func ("/documents/update/only_file", service, test_update_file);
		g_test_add_data_func ("/documents/update/metadata_file", service, test_update_metadata_file);

		g_test_add_data_func ("/documents/access_rules/add_document_with_a_collaborator", service, test_new_document_with_collaborator);

		g_test_add_data_func ("/documents/query/all_documents_with_folder", service, test_query_all_documents_with_folder);
		g_test_add_data_func ("/documents/query/all_documents", service, test_query_all_documents);
		g_test_add_data_func ("/documents/query/all_documents_async", service, test_query_all_documents_async);

		g_test_add_data_func ("/documents/move/move_to_folder", service, test_add_file_folder_and_move);
		g_test_add_data_func ("/documents/move/remove_from_folder", service, test_add_remove_file_from_folder);
		/*g_test_add_data_func ("/documents/remove/all", service, test_remove_all_documents_and_folders);*/

		g_test_add_data_func ("/documents/batch", service, test_batch);
		g_test_add ("/documents/batch/async", BatchAsyncData, service, setup_batch_async, test_batch_async, teardown_batch_async);
		g_test_add ("/documents/batch/async/cancellation", BatchAsyncData, service, setup_batch_async, test_batch_async_cancellation,
		            teardown_batch_async);
	}

	g_test_add_func ("/documents/query/etag", test_query_etag);

	retval = g_test_run ();

	if (service != NULL)
		g_object_unref (service);

	return retval;
}
