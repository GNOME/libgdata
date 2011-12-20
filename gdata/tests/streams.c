/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
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
#include <locale.h>
#include <string.h>

#include "gdata.h"
#include "common.h"

static gpointer
run_server_thread (SoupServer *server)
{
	soup_server_run (server);

	return NULL;
}

static GThread *
run_server (SoupServer *server)
{
	GThread *thread;
	GError *error = NULL;

#if GLIB_CHECK_VERSION (2, 31, 0)
	thread = g_thread_try_new ("server-thread", (GThreadFunc) run_server_thread, server, &error);
#else
	thread = g_thread_create ((GThreadFunc) run_server_thread, server, TRUE, &error);
#endif
	g_assert_no_error (error);
	g_assert (thread != NULL);

	return thread;
}

static gboolean
quit_server_cb (SoupServer *server)
{
	soup_server_quit (server);

	return FALSE;
}

static gchar *
get_test_string (guint start_num, guint end_num)
{
	GString *test_string;
	guint i;

	test_string = g_string_new (NULL);

	for (i = start_num; i <= end_num; i++)
		g_string_append_printf (test_string, "%u\n", i);

	return g_string_free (test_string, FALSE);
}

static void
test_download_stream_download_server_content_length_handler_cb (SoupServer *server, SoupMessage *message, const char *path, GHashTable *query,
                                                                SoupClientContext *client, gpointer user_data)
{
	gchar *test_string;
	goffset test_string_length;

	test_string = get_test_string (1, 1000);
	test_string_length = strlen (test_string) + 1;

	/* Add some response headers */
	soup_message_set_status (message, SOUP_STATUS_OK);
	soup_message_headers_set_content_type (message->response_headers, "text/plain", NULL);
	soup_message_headers_set_content_length (message->response_headers, test_string_length);

	/* Set the response body */
	soup_message_body_append (message->response_body, SOUP_MEMORY_TAKE, test_string, test_string_length);
}

static void
test_download_stream_download_content_length (void)
{
	SoupServer *server;
	GMainContext *async_context;
	SoupAddress *addr;
	GThread *thread;
	gchar *download_uri, *test_string;
	GDataService *service;
	GInputStream *download_stream;
	gssize length_read;
	GString *contents;
	guint8 buffer[20];
	gboolean success;
	GError *error = NULL;

	/* Create the server */
	async_context = g_main_context_new ();
	addr = soup_address_new ("127.0.0.1", SOUP_ADDRESS_ANY_PORT);
	soup_address_resolve_sync (addr, NULL);

	server = soup_server_new (SOUP_SERVER_INTERFACE, addr,
	                          SOUP_SERVER_ASYNC_CONTEXT, async_context,
	                          NULL);
	soup_server_add_handler (server, NULL, (SoupServerCallback) test_download_stream_download_server_content_length_handler_cb, NULL, NULL);

	g_object_unref (addr);

	g_assert (server != NULL);

	/* Create a thread for the server */
	thread = run_server (server);

	/* Create a new download stream connected to the server */
	download_uri = g_strdup_printf ("http://127.0.0.1:%u/", soup_server_get_port (server));
	service = GDATA_SERVICE (gdata_youtube_service_new ("developer-key", NULL));
	download_stream = gdata_download_stream_new (service, NULL, download_uri, NULL);
	g_object_unref (service);
	g_free (download_uri);

	/* Read the entire stream into a string which we can later compare with what we expect. */
	contents = g_string_new (NULL);

	while ((length_read = g_input_stream_read (download_stream, buffer, sizeof (buffer), NULL, &error)) > 0) {
		g_assert_cmpint (length_read, <=, sizeof (buffer));

		g_string_append_len (contents, (const gchar*) buffer, length_read);
	}

	/* Check we've reached EOF successfully */
	g_assert_no_error (error);
	g_assert_cmpint (length_read, ==, 0);

	/* Close the stream */
	success = g_input_stream_close (download_stream, NULL, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);

	/* Compare the downloaded string to the original */
	test_string = get_test_string (1, 1000);

	g_assert_cmpint (contents->len, ==, strlen (test_string) + 1);
	g_assert_cmpstr (contents->str, ==, test_string);

	g_free (test_string);
	g_string_free (contents, TRUE);

	/* Kill the server and wait for it to die */
	soup_add_completion (async_context, (GSourceFunc) quit_server_cb, server);
	g_thread_join (thread);

	g_object_unref (download_stream);
	g_object_unref (server);
	g_main_context_unref (async_context);
}

static void
test_download_stream_download_server_seek_handler_cb (SoupServer *server, SoupMessage *message, const char *path, GHashTable *query,
                                                      SoupClientContext *client, gpointer user_data)
{
	gchar *test_string;
	goffset test_string_length;

	test_string = get_test_string (1, 1000);
	test_string_length = strlen (test_string) + 1;

	/* Add some response headers */
	soup_message_set_status (message, SOUP_STATUS_OK);
	soup_message_body_append (message->response_body, SOUP_MEMORY_TAKE, test_string, test_string_length);
}

/* Test seeking before the first read */
static void
test_download_stream_download_seek_before_start (void)
{
	SoupServer *server;
	GMainContext *async_context;
	SoupAddress *addr;
	GThread *thread;
	gchar *download_uri, *test_string;
	goffset test_string_offset = 0;
	guint test_string_length;
	GDataService *service;
	GInputStream *download_stream;
	gssize length_read;
	guint8 buffer[20];
	gboolean success;
	GError *error = NULL;

	/* Create the server */
	async_context = g_main_context_new ();
	addr = soup_address_new ("127.0.0.1", SOUP_ADDRESS_ANY_PORT);
	soup_address_resolve_sync (addr, NULL);

	server = soup_server_new (SOUP_SERVER_INTERFACE, addr,
	                          SOUP_SERVER_ASYNC_CONTEXT, async_context,
	                          NULL);
	soup_server_add_handler (server, NULL, (SoupServerCallback) test_download_stream_download_server_seek_handler_cb, NULL, NULL);

	g_object_unref (addr);

	g_assert (server != NULL);

	/* Create a thread for the server */
	thread = run_server (server);

	/* Create a new download stream connected to the server */
	download_uri = g_strdup_printf ("http://127.0.0.1:%u/", soup_server_get_port (server));
	service = GDATA_SERVICE (gdata_youtube_service_new ("developer-key", NULL));
	download_stream = gdata_download_stream_new (service, NULL, download_uri, NULL);
	g_object_unref (service);
	g_free (download_uri);

	/* Read alternating blocks into a string and compare with what we expect as we go. i.e. Skip 20 bytes, then read 20 bytes, etc. */
	test_string = get_test_string (1, 1000);
	test_string_length = strlen (test_string) + 1;

	while (TRUE) {
		/* Check the seek offset */
		g_assert_cmpint (g_seekable_tell (G_SEEKABLE (download_stream)), ==, test_string_offset);

		/* Seek forward a buffer length */
		if (g_seekable_seek (G_SEEKABLE (download_stream), sizeof (buffer), G_SEEK_CUR, NULL, &error) == FALSE) {
			/* Tried to seek past the end of the stream */
			g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
			g_clear_error (&error);
			break;
		}

		test_string_offset += sizeof (buffer);
		g_assert_no_error (error);

		/* Check the seek offset again */
		g_assert_cmpint (g_seekable_tell (G_SEEKABLE (download_stream)), ==, test_string_offset);

		/* Read a buffer-load */
		length_read = g_input_stream_read (download_stream, buffer, sizeof (buffer), NULL, &error);

		g_assert_no_error (error);
		g_assert_cmpint (length_read, <=, sizeof (buffer));

		/* Check the buffer-load against the test string */
		g_assert (memcmp (buffer, test_string + test_string_offset, length_read) == 0);
		test_string_offset += length_read;

		/* Check the seek offset again */
		g_assert_cmpint (g_seekable_tell (G_SEEKABLE (download_stream)), ==, test_string_offset);

		if (length_read < (gssize) sizeof (buffer)) {
			/* Done */
			break;
		}
	}

	g_free (test_string);

	/* Check the seek offset is within one buffer-load of the end */
	g_assert_cmpint (g_seekable_tell (G_SEEKABLE (download_stream)), >, test_string_length - sizeof (buffer));
	g_assert_cmpint (g_seekable_tell (G_SEEKABLE (download_stream)), <=, test_string_length);

	/* Close the stream */
	success = g_input_stream_close (download_stream, NULL, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);

	/* Kill the server and wait for it to die */
	soup_add_completion (async_context, (GSourceFunc) quit_server_cb, server);
	g_thread_join (thread);

	g_object_unref (download_stream);
	g_object_unref (server);
	g_main_context_unref (async_context);
}

/* Test seeking forwards after the first read */
static void
test_download_stream_download_seek_after_start_forwards (void)
{
	SoupServer *server;
	GMainContext *async_context;
	SoupAddress *addr;
	GThread *thread;
	gchar *download_uri, *test_string;
	goffset test_string_offset = 0;
	guint test_string_length;
	GDataService *service;
	GInputStream *download_stream;
	gssize length_read;
	guint8 buffer[20];
	gboolean success;
	GError *error = NULL;

	/* Create the server */
	async_context = g_main_context_new ();
	addr = soup_address_new ("127.0.0.1", SOUP_ADDRESS_ANY_PORT);
	soup_address_resolve_sync (addr, NULL);

	server = soup_server_new (SOUP_SERVER_INTERFACE, addr,
	                          SOUP_SERVER_ASYNC_CONTEXT, async_context,
	                          NULL);
	soup_server_add_handler (server, NULL, (SoupServerCallback) test_download_stream_download_server_seek_handler_cb, NULL, NULL);

	g_object_unref (addr);

	g_assert (server != NULL);

	/* Create a thread for the server */
	thread = run_server (server);

	/* Create a new download stream connected to the server */
	download_uri = g_strdup_printf ("http://127.0.0.1:%u/", soup_server_get_port (server));
	service = GDATA_SERVICE (gdata_youtube_service_new ("developer-key", NULL));
	download_stream = gdata_download_stream_new (service, NULL, download_uri, NULL);
	g_object_unref (service);
	g_free (download_uri);

	/* Read alternating blocks into a string and compare with what we expect as we go. i.e. Read 20 bytes, then skip 20 bytes, etc. */
	test_string = get_test_string (1, 1000);
	test_string_length = strlen (test_string) + 1;

	while (TRUE) {
		/* Check the seek offset */
		g_assert_cmpint (g_seekable_tell (G_SEEKABLE (download_stream)), ==, test_string_offset);

		/* Read a buffer-load */
		length_read = g_input_stream_read (download_stream, buffer, sizeof (buffer), NULL, &error);

		g_assert_no_error (error);
		g_assert_cmpint (length_read, <=, sizeof (buffer));

		/* Check the buffer-load against the test string */
		g_assert (memcmp (buffer, test_string + test_string_offset, length_read) == 0);
		test_string_offset += length_read;

		/* Check the seek offset again */
		g_assert_cmpint (g_seekable_tell (G_SEEKABLE (download_stream)), ==, test_string_offset);

		if (length_read < (gssize) sizeof (buffer)) {
			/* Done */
			break;
		}

		/* Seek forward a buffer length */
		if (g_seekable_seek (G_SEEKABLE (download_stream), sizeof (buffer), G_SEEK_CUR, NULL, &error) == FALSE) {
			/* Tried to seek past the end of the stream */
			g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT);
			g_clear_error (&error);
			break;
		}

		test_string_offset += sizeof (buffer);
		g_assert_no_error (error);

		/* Check the seek offset again */
		g_assert_cmpint (g_seekable_tell (G_SEEKABLE (download_stream)), ==, test_string_offset);
	}

	g_free (test_string);

	/* Check the seek offset is within one buffer-load of the end */
	g_assert_cmpint (g_seekable_tell (G_SEEKABLE (download_stream)), >, test_string_length - sizeof (buffer));
	g_assert_cmpint (g_seekable_tell (G_SEEKABLE (download_stream)), <=, test_string_length);

	/* Close the stream */
	success = g_input_stream_close (download_stream, NULL, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);

	/* Kill the server and wait for it to die */
	soup_add_completion (async_context, (GSourceFunc) quit_server_cb, server);
	g_thread_join (thread);

	g_object_unref (download_stream);
	g_object_unref (server);
	g_main_context_unref (async_context);
}

/* Test seeking backwards after the first read */
static void
test_download_stream_download_seek_after_start_backwards (void)
{
	SoupServer *server;
	GMainContext *async_context;
	SoupAddress *addr;
	GThread *thread;
	gchar *download_uri, *test_string;
	goffset test_string_offset = 0;
	guint repeat_count;
	GDataService *service;
	GInputStream *download_stream;
	gssize length_read;
	guint8 buffer[20];
	gboolean success;
	GError *error = NULL;

	/* Create the server */
	async_context = g_main_context_new ();
	addr = soup_address_new ("127.0.0.1", SOUP_ADDRESS_ANY_PORT);
	soup_address_resolve_sync (addr, NULL);

	server = soup_server_new (SOUP_SERVER_INTERFACE, addr,
	                          SOUP_SERVER_ASYNC_CONTEXT, async_context,
	                          NULL);
	soup_server_add_handler (server, NULL, (SoupServerCallback) test_download_stream_download_server_seek_handler_cb, NULL, NULL);

	g_object_unref (addr);

	g_assert (server != NULL);

	/* Create a thread for the server */
	thread = run_server (server);

	/* Create a new download stream connected to the server */
	download_uri = g_strdup_printf ("http://127.0.0.1:%u/", soup_server_get_port (server));
	service = GDATA_SERVICE (gdata_youtube_service_new ("developer-key", NULL));
	download_stream = gdata_download_stream_new (service, NULL, download_uri, NULL);
	g_object_unref (service);
	g_free (download_uri);

	/* Read a block in, then skip back over the block again. i.e. Read the first block, read the second block, skip back over the second block,
	 * read the second block again, skip back over it, etc. Close the stream after doing this several times. */
	test_string = get_test_string (1, 1000);

	/* Read a buffer-load to begin with */
	length_read = g_input_stream_read (download_stream, buffer, sizeof (buffer), NULL, &error);

	g_assert_no_error (error);
	test_string_offset += length_read;

	for (repeat_count = 6; repeat_count > 0; repeat_count--) {
		/* Check the seek offset */
		g_assert_cmpint (g_seekable_tell (G_SEEKABLE (download_stream)), ==, test_string_offset);

		/* Read a buffer-load */
		length_read = g_input_stream_read (download_stream, buffer, sizeof (buffer), NULL, &error);

		g_assert_no_error (error);
		g_assert_cmpint (length_read, <=, sizeof (buffer));

		/* Check the buffer-load against the test string */
		g_assert (memcmp (buffer, test_string + test_string_offset, length_read) == 0);
		test_string_offset += length_read;

		/* Check the seek offset again */
		g_assert_cmpint (g_seekable_tell (G_SEEKABLE (download_stream)), ==, test_string_offset);

		/* Seek backwards a buffer length */
		success = g_seekable_seek (G_SEEKABLE (download_stream), -length_read, G_SEEK_CUR, NULL, &error);
		g_assert_no_error (error);
		g_assert (success == TRUE);
		test_string_offset -= length_read;

		/* Check the seek offset again */
		g_assert_cmpint (g_seekable_tell (G_SEEKABLE (download_stream)), ==, test_string_offset);
	}

	g_free (test_string);

	/* Check the seek offset is at the end of the first buffer-load */
	g_assert_cmpint (g_seekable_tell (G_SEEKABLE (download_stream)), ==, sizeof (buffer));

	/* Close the stream */
	success = g_input_stream_close (download_stream, NULL, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);

	/* Kill the server and wait for it to die */
	soup_add_completion (async_context, (GSourceFunc) quit_server_cb, server);
	g_thread_join (thread);

	g_object_unref (download_stream);
	g_object_unref (server);
	g_main_context_unref (async_context);
}

static void
test_upload_stream_upload_no_entry_content_length_server_handler_cb (SoupServer *server, SoupMessage *message, const char *path, GHashTable *query,
                                                                     SoupClientContext *client, gpointer user_data)
{
	gchar *test_string;
	goffset test_string_length;

	/* Check the Slug and Content-Type headers have been correctly set by the client */
	g_assert_cmpstr (soup_message_headers_get_content_type (message->request_headers, NULL), ==, "text/plain");
	g_assert_cmpstr (soup_message_headers_get_one (message->request_headers, "Slug"), ==, "slug");

	/* Check the client sent the right data */
	test_string = get_test_string (1, 1000);
	test_string_length = strlen (test_string) + 1;

	g_assert_cmpint (message->request_body->length, ==, test_string_length);
	g_assert_cmpstr (message->request_body->data, ==, test_string);

	g_free (test_string);

	/* Add some response headers */
	soup_message_set_status (message, SOUP_STATUS_OK);
	soup_message_headers_set_content_type (message->response_headers, "text/plain", NULL);

	/* Set the response body */
	soup_message_body_append (message->response_body, SOUP_MEMORY_STATIC, "Test passed!", 13);
}

static void
test_upload_stream_upload_no_entry_content_length (void)
{
	SoupServer *server;
	GMainContext *async_context;
	SoupAddress *addr;
	GThread *thread;
	gchar *upload_uri, *test_string;
	GDataService *service;
	GOutputStream *upload_stream;
	gssize length_written;
	gsize total_length_written = 0;
	goffset test_string_length;
	gboolean success;
	GError *error = NULL;

	/* Create the server */
	async_context = g_main_context_new ();
	addr = soup_address_new ("127.0.0.1", SOUP_ADDRESS_ANY_PORT);
	soup_address_resolve_sync (addr, NULL);

	server = soup_server_new (SOUP_SERVER_INTERFACE, addr,
	                          SOUP_SERVER_ASYNC_CONTEXT, async_context,
	                          NULL);
	soup_server_add_handler (server, NULL, (SoupServerCallback) test_upload_stream_upload_no_entry_content_length_server_handler_cb, NULL, NULL);

	g_object_unref (addr);

	g_assert (server != NULL);

	/* Create a thread for the server */
	thread = run_server (server);

	/* Create a new upload stream uploading to the server */
	upload_uri = g_strdup_printf ("http://127.0.0.1:%u/", soup_server_get_port (server));
	service = GDATA_SERVICE (gdata_youtube_service_new ("developer-key", NULL));
	upload_stream = gdata_upload_stream_new (service, NULL, SOUP_METHOD_POST, upload_uri, NULL, "slug", "text/plain", NULL);
	g_object_unref (service);
	g_free (upload_uri);

	/* Write the entire test string to the stream */
	test_string = get_test_string (1, 1000);
	test_string_length = strlen (test_string) + 1;

	while ((length_written = g_output_stream_write (upload_stream, test_string + total_length_written,
	                                                test_string_length - total_length_written, NULL, &error)) > 0) {
		g_assert_cmpint (length_written, <=, test_string_length - total_length_written);

		total_length_written += length_written;
	}

	g_free (test_string);

	/* Check we've had a successful return value */
	g_assert_no_error (error);
	g_assert_cmpint (length_written, ==, 0);
	g_assert_cmpint (total_length_written, ==, test_string_length);

	/* Close the stream */
	success = g_output_stream_close (upload_stream, NULL, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);

	/* Kill the server and wait for it to die */
	soup_add_completion (async_context, (GSourceFunc) quit_server_cb, server);
	g_thread_join (thread);

	g_object_unref (upload_stream);
	g_object_unref (server);
	g_main_context_unref (async_context);
}

int
main (int argc, char *argv[])
{
	gdata_test_init (argc, argv);

	g_test_add_func ("/download-stream/download_content_length", test_download_stream_download_content_length);
	g_test_add_func ("/download-stream/download_seek/before_start", test_download_stream_download_seek_before_start);
	g_test_add_func ("/download-stream/download_seek/after_start_forwards", test_download_stream_download_seek_after_start_forwards);
	g_test_add_func ("/download-stream/download_seek/after_start_backwards", test_download_stream_download_seek_after_start_backwards);

	g_test_add_func ("/upload-stream/upload_no_entry_content_length", test_upload_stream_upload_no_entry_content_length);

	return g_test_run ();
}
