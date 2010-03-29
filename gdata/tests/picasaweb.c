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

#include <glib.h>
#include <unistd.h>
#include <string.h>
#include <config.h>

/* For the thumbnail size tests in test_download_thumbnails() */
#ifdef HAVE_GDK
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

#include "gdata.h"
#include "common.h"

#define PW_USERNAME "libgdata.picasaweb@gmail.com"
/* the following two properties will change if a new album is added */
#define NUM_ALBUMS 3
#define TEST_ALBUM_INDEX 2

static void
delete_directory (GFile *directory, GError **error)
{
	GFileEnumerator *enumerator;

	enumerator = g_file_enumerate_children (directory, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, error);
	if (enumerator == NULL)
		return;

	/* Delete all the files in the directory */
	while (TRUE) {
		GFileInfo *file_info;
		GFile *file;

		file_info = g_file_enumerator_next_file (enumerator, NULL, NULL);
		if (file_info == NULL)
			break;

		file = g_file_get_child (directory, g_file_info_get_name (file_info));
		g_object_unref (file_info);

		g_file_delete (file, NULL, NULL);
		g_object_unref (file);
	}

	g_file_enumerator_close (enumerator, NULL, error);

	/* Delete the directory itself */
	g_file_delete (directory, NULL, error);
}

static void
test_authentication (void)
{
	gboolean retval;
	GDataService *service;
	GError *error = NULL;

	/* Create a service */
	service = GDATA_SERVICE (gdata_picasaweb_service_new (CLIENT_ID));

	g_assert (service != NULL);
	g_assert (GDATA_IS_SERVICE (service));
	g_assert_cmpstr (gdata_service_get_client_id (service), ==, CLIENT_ID);

	/* Log in */
	retval = gdata_service_authenticate (service, PW_USERNAME, PASSWORD, NULL, &error);
	g_assert_no_error (error);
	g_assert (retval == TRUE);
	g_clear_error (&error);

	/* Check all is as it should be */
	g_assert (gdata_service_is_authenticated (service) == TRUE);
	g_assert_cmpstr (gdata_service_get_username (service), ==, PW_USERNAME);
	g_assert_cmpstr (gdata_service_get_password (service), ==, PASSWORD);

	g_object_unref (service);
}

static void
test_authentication_async_cb (GDataService *service, GAsyncResult *async_result, GMainLoop *main_loop)
{
	gboolean retval;
	GError *error = NULL;

	retval = gdata_service_authenticate_finish (service, async_result, &error);
	g_assert_no_error (error);
	g_assert (retval == TRUE);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);

	/* Check all is as it should be */
	g_assert (gdata_service_is_authenticated (service) == TRUE);
	g_assert_cmpstr (gdata_service_get_username (service), ==, PW_USERNAME);
	g_assert_cmpstr (gdata_service_get_password (service), ==, PASSWORD);
}


static void
test_authentication_async (void)
{
	GDataService *service;
	GMainLoop *main_loop = g_main_loop_new (NULL, TRUE);

	/* Create a service */
	service = GDATA_SERVICE (gdata_picasaweb_service_new (CLIENT_ID));

	g_assert (service != NULL);
	g_assert (GDATA_IS_SERVICE (service));

	gdata_service_authenticate_async (service, PW_USERNAME, PASSWORD, NULL, (GAsyncReadyCallback) test_authentication_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);

	g_object_unref (service);
}

static void
test_upload_async_cb (GDataPicasaWebService *service, GAsyncResult *result, GMainLoop *main_loop)
{
	GDataPicasaWebFile *photo_new;
	GError *error = NULL;

	photo_new = gdata_picasaweb_service_upload_file_finish (service, result, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_PICASAWEB_FILE (photo_new));
	g_clear_error (&error);
	g_assert (gdata_entry_is_inserted (GDATA_ENTRY (photo_new)));

	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (photo_new)), ==, "Async Photo Entry Title");

	g_main_loop_quit (main_loop);

	g_object_unref (photo_new);
}

static void
test_upload_async (gconstpointer service)
{
	GDataPicasaWebFile *photo;
	GFile *photo_file;
	GTimeVal timeval;
	gchar *xml, *time_str, *summary, *expected_xml, *parsed_time_str;
	GRegex *regex;
	GMatchInfo *match_info;
	guint64 delta;
	const gchar * const tags[] = { "foo", "bar", ",,baz,baz", NULL };
	GMainLoop *main_loop = g_main_loop_new (NULL, TRUE);

	g_get_current_time (&timeval);
	time_str = g_time_val_to_iso8601 (&timeval);
	summary = g_strdup_printf ("Async Photo Summary (%s)", time_str);

	expected_xml = g_strdup_printf ("<entry "
						"xmlns='http://www.w3.org/2005/Atom' "
						"xmlns:gphoto='http://schemas.google.com/photos/2007' "
						"xmlns:media='http://search.yahoo.com/mrss/' "
						"xmlns:gd='http://schemas.google.com/g/2005' "
						"xmlns:exif='http://schemas.google.com/photos/exif/2007' "
						"xmlns:app='http://www.w3.org/2007/app' "
						"xmlns:georss='http://www.georss.org/georss' "
						"xmlns:gml='http://www.opengis.net/gml'>"
						"<title type='text'>Async Photo Entry Title</title>"
						"<summary type='text'>Async Photo Summary \\(%s\\)</summary>"
						"<gphoto:position>0</gphoto:position>"
						"<gphoto:timestamp>([0-9]+)</gphoto:timestamp>"
						"<gphoto:commentingEnabled>true</gphoto:commentingEnabled>"
						"<media:group>"
							"<media:title type='plain'>Async Photo Entry Title</media:title>"
							"<media:description type='plain'>Async Photo Summary \\(%s\\)</media:description>"
							"<media:keywords>foo,bar,%%2C%%2Cbaz%%2Cbaz</media:keywords>"
						"</media:group>"
					"</entry>", time_str, time_str);
	g_free (time_str);

	/* Build a regex to match the timestamp from the XML, since we can't definitely say what it'll be */
	regex = g_regex_new (expected_xml, 0, 0, NULL);
	g_free (expected_xml);

	/* Build the photo */
	photo = gdata_picasaweb_file_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (photo), "Async Photo Entry Title");
	gdata_picasaweb_file_set_caption (photo, summary);
	gdata_picasaweb_file_set_tags (photo, tags);

	/* Check the XML: match it against the regex built above, then check that the timestamp is within 100ms of the current time at the start of
	 * the test function. We can't check it exactly, as a few milliseconds may have passed inbetween building the expected_xml and building the XML
	 * for the photo. */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (photo));
	g_assert (g_regex_match (regex, xml, 0, &match_info) == TRUE);
	parsed_time_str = g_match_info_fetch (match_info, 1);
	delta = g_ascii_strtoull (parsed_time_str, NULL, 10) - (((guint64) timeval.tv_sec) * 1000 + ((guint64) timeval.tv_usec) / 1000);
	g_assert_cmpuint (abs (delta), <, 100);

	g_free (parsed_time_str);
	g_free (xml);
	g_regex_unref (regex);
	g_match_info_free (match_info);

	gdata_picasaweb_file_set_coordinates (photo, 17.127, -110.35);

	/* File is public domain: http://en.wikipedia.org/wiki/File:German_garden_gnome_cropped.jpg */
	photo_file = g_file_new_for_path (TEST_FILE_DIR "photo.jpg");

	/* Upload the photo */
	gdata_picasaweb_service_upload_file_async (GDATA_PICASAWEB_SERVICE (service), NULL, photo, photo_file, NULL,
						   (GAsyncReadyCallback) test_upload_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);

	g_free (summary);
	g_object_unref (photo);
	g_object_unref (photo_file);
}

static void
test_download_thumbnails (gconstpointer _service)
{
	GDataService *service = GDATA_SERVICE (_service);
	GDataFeed *album_feed, *photo_feed;
	GList *album_entries, *photo_entries, *thumbnails, *node;
	GDataPicasaWebAlbum *album;
	GDataPicasaWebFile *photo;
	GDataPicasaWebQuery *query;
	GFile *dest_dir, *dest_file, *actual_file;
	GDataMediaThumbnail *thumbnail;
	gchar *file_path, *basename;
	GError *error = NULL;

	/* Acquire album, photo to test */
	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (album_feed));

	album_entries = gdata_feed_get_entries (album_feed);
	g_assert (album_entries != NULL);

	album = GDATA_PICASAWEB_ALBUM (album_entries->data);

	query = gdata_picasaweb_query_new (NULL);
	gdata_picasaweb_query_set_image_size (query, "32"); /* we're querying for the smallest size, to save bandwidth here :D */
	photo_feed = gdata_picasaweb_service_query_files (GDATA_PICASAWEB_SERVICE (service), album, GDATA_QUERY (query), NULL, NULL, NULL, &error);
	g_object_unref (query);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (photo_feed));

	photo_entries = gdata_feed_get_entries (photo_feed);
	g_assert (photo_entries != NULL);

	photo = GDATA_PICASAWEB_FILE (photo_entries->data);

	dest_dir = g_file_new_for_path ("/tmp/gdata.picasaweb.test.dir/");
	dest_file = g_file_new_for_path ("/tmp/gdata.picasaweb.test.dir/test.jpg");

	/* clean up any pre-existing test output  */
	if (g_file_query_exists (dest_dir, NULL)) {
		delete_directory (dest_dir, &error);
		g_assert_no_error (error);
	}

	thumbnails = gdata_picasaweb_file_get_thumbnails (photo);
	thumbnail = GDATA_MEDIA_THUMBNAIL (thumbnails->data);

	/* to a directory, non-existent, should succeed, file with "directory"'s name */
	actual_file = gdata_media_thumbnail_download (thumbnail, service, "thumbnail.jpg", dest_dir, FALSE, NULL, &error);
	g_assert_no_error (error);
	g_assert (g_file_query_exists (actual_file, NULL));
	basename = g_file_get_basename (actual_file);
	g_assert_cmpstr (basename, ==, "gdata.picasaweb.test.dir");
	g_free (basename);
	g_object_unref (actual_file);

	/* to a "directory", which doesn't actually exist (as a directory), should fail */
	actual_file = gdata_media_thumbnail_download (thumbnail, service, "thumbnail.jpg", dest_file, FALSE, NULL, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_DIRECTORY);
	g_clear_error (&error);
	g_assert (actual_file == NULL);

	/* create the directory so we can test on it and in it */
	g_file_delete (dest_dir, NULL, &error);
	g_assert_no_error (error);
	g_file_make_directory (dest_dir, NULL, &error);
	g_assert_no_error (error);

	/* to a directory, existent, should succeed, making use of the default filename provided */
	actual_file = gdata_media_thumbnail_download (thumbnail, service, "thumbnail.jpg", dest_dir, FALSE, NULL, &error);
	g_assert_no_error (error);
	g_assert (actual_file != NULL);
	basename = g_file_get_basename (actual_file);
	g_assert_cmpstr (basename, ==, "thumbnail.jpg");
	g_free (basename);
	g_object_unref (actual_file);

	/* to a directory, existent, with inferred file destination already existent, without replace, should fail */
	actual_file = gdata_media_thumbnail_download (thumbnail, service, "thumbnail.jpg", dest_dir, FALSE, NULL, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_EXISTS);
	g_clear_error (&error);
	g_assert (actual_file == NULL);

	/* to a directory, existent, with inferred file destination already existent, with replace, should succeed */
	actual_file = gdata_media_thumbnail_download (thumbnail, service, "thumbnail.jpg", dest_dir, TRUE, NULL, &error);
	g_assert_no_error (error);
	g_assert (g_file_query_exists (actual_file, NULL));
	basename = g_file_get_basename (actual_file);
	g_assert_cmpstr (basename, ==, "thumbnail.jpg");
	g_free (basename);
	g_object_unref (actual_file);

	/* to a path, non-existent, should succeed */
	g_assert (g_file_query_exists (dest_file, NULL) == FALSE);
	actual_file = gdata_media_thumbnail_download (thumbnail, service, "thumbnail.jpg", dest_file, FALSE, NULL, &error);
	g_assert_no_error (error);
	g_assert (g_file_query_exists (actual_file, NULL));
	basename = g_file_get_basename (actual_file);
	g_assert_cmpstr (basename, ==, "test.jpg");
	g_free (basename);
	g_object_unref (actual_file);

	/* to a path, existent, without replace, should fail */
	actual_file = gdata_media_thumbnail_download (thumbnail, service, "thumbnail.jpg", dest_file, FALSE, NULL, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_EXISTS);
	g_clear_error (&error);
	g_assert (actual_file == NULL);

	/* to a path, existent, with replace, should succeed */
	actual_file = gdata_media_thumbnail_download (thumbnail, service, "thumbnail.jpg", dest_file, TRUE, NULL, &error);
	g_assert_no_error (error);
	g_assert (g_file_query_exists (actual_file, NULL));
	basename = g_file_get_basename (actual_file);
	g_assert_cmpstr (basename, ==, "test.jpg");
	g_free (basename);
	g_object_unref (actual_file);

	/* clean up test file and thumbnail*/
	g_file_delete (dest_file, NULL, &error);
	g_assert_no_error (error);

	/* test getting all thumbnails and that they're all the correct size */
	for (node = thumbnails; node != NULL; node = node->next) {
#ifdef HAVE_GDK
		GdkPixbuf *pixbuf;
#endif /* HAVE_GDK */

		thumbnail = GDATA_MEDIA_THUMBNAIL (node->data);
		actual_file = gdata_media_thumbnail_download (thumbnail, service, "thumbnail.jpg", dest_file, FALSE, NULL, &error);
		g_assert_no_error (error);
		g_assert (g_file_query_exists (actual_file, NULL));

#ifdef HAVE_GDK
		file_path = g_file_get_path (actual_file);
		pixbuf = gdk_pixbuf_new_from_file (file_path, &error);
		g_assert_no_error (error);
		g_free (file_path);

		/* PicasaWeb reported the height of a thumbnail as a pixel too large once, but otherwise correct */
		g_assert_cmpint (abs (gdk_pixbuf_get_width (pixbuf) - (gint)gdata_media_thumbnail_get_width (thumbnail)) , <=, 1);
		g_assert_cmpint (abs (gdk_pixbuf_get_height (pixbuf) - (gint)gdata_media_thumbnail_get_height (thumbnail)) , <=, 1);
		g_object_unref (pixbuf);
#endif /* HAVE_GDK */

		g_file_delete (actual_file, NULL, &error);
		g_assert (g_file_query_exists (actual_file, NULL) == FALSE);
		g_assert_no_error (error);
		g_object_unref (actual_file);
	}

	/* clean up test directory again */
	delete_directory (dest_dir, &error);
	g_assert_no_error (error);

	g_object_unref (photo_feed);
	g_object_unref (album_feed);
	g_object_unref (dest_dir);
	g_object_unref (dest_file);
}

static void
test_download (gconstpointer _service)
{
	GDataService *service = GDATA_SERVICE (_service);
	GDataFeed *album_feed, *photo_feed;
	GList *album_entries, *photo_entries, *media_contents;
	GDataPicasaWebAlbum *album;
	GDataPicasaWebFile *photo;
	GDataPicasaWebQuery *query;
	GDataMediaContent* content;
	GFile *dest_dir, *dest_file, *actual_file;
	gchar *basename;
	GError *error = NULL;

	/*** Acquire a photo to test ***/
	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (album_feed));

	album_entries = gdata_feed_get_entries (album_feed);
	g_assert (album_entries != NULL);

	album = GDATA_PICASAWEB_ALBUM (album_entries->data);

	query = gdata_picasaweb_query_new (NULL);
	gdata_picasaweb_query_set_image_size (query, "32"); /* we're querying for the smallest size, to save bandwidth here :D */
	photo_feed = gdata_picasaweb_service_query_files (GDATA_PICASAWEB_SERVICE (service), album, GDATA_QUERY (query), NULL, NULL, NULL, &error);
	g_object_unref (query);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (photo_feed));

	photo_entries = gdata_feed_get_entries (photo_feed);
	g_assert (photo_entries != NULL);

	photo = GDATA_PICASAWEB_FILE (photo_entries->data);

	dest_dir = g_file_new_for_path ("/tmp/gdata.picasaweb.test.dir/");
	dest_file = g_file_new_for_path ("/tmp/gdata.picasaweb.test.dir/test.jpg");

	/* clean up any pre-existing test output  */
	if (g_file_query_exists (dest_dir, NULL)) {
		delete_directory (dest_dir, &error);
		g_assert_no_error (error);
	}

	media_contents = gdata_picasaweb_file_get_contents (photo);
	g_assert_cmpint (g_list_length (media_contents), ==, 1);
	content = GDATA_MEDIA_CONTENT (media_contents->data);

	/* to a directory, non-existent, should succeed, file with "directory"'s name */
	actual_file = gdata_media_content_download (content, service, "default.jpg", dest_dir, FALSE, NULL, &error);
	g_assert_no_error (error);
	g_assert (g_file_query_exists (actual_file, NULL));
	basename = g_file_get_basename (actual_file);
	g_assert_cmpstr (basename, ==, "gdata.picasaweb.test.dir");
	g_free (basename);
	g_object_unref (actual_file);

	/* to a file in a "directory", which already exists as a file, should fail */
	actual_file = gdata_media_content_download (content, service, "default.jpg", dest_file, FALSE, NULL, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_DIRECTORY);
	g_clear_error (&error);
	g_assert (actual_file == NULL);

	/* create the directory so we can test on it and in it */
	g_file_delete (dest_dir, NULL, &error);
	g_assert_no_error (error);
	g_file_make_directory (dest_dir, NULL, &error);
	g_assert_no_error (error);

	/* to a directory, existent, should succeed, using default filename */
	actual_file = gdata_media_content_download (content, service, "default.jpg", dest_dir, FALSE, NULL, &error);
	g_assert_no_error (error);
	g_assert (actual_file != NULL);
	basename = g_file_get_basename (actual_file);
	g_assert_cmpstr (basename, ==, "default.jpg");
	g_free (basename);
	g_object_unref (actual_file);
	/* TODO: test that it exists with default filename? */

	/* to a directory, existent, should fail trying to use the default filename, which already exists */
	actual_file = gdata_media_content_download (content, service, "default.jpg", dest_dir, FALSE, NULL, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_EXISTS);
	g_clear_error (&error);
	g_assert (actual_file == NULL);

	/* to a directory, existent, should succeed with default filename, replacing what already exists */
	actual_file = gdata_media_content_download (content, service, "default.jpg", dest_dir, TRUE, NULL, &error);
	g_assert_no_error (error);
	g_assert (g_file_query_exists (actual_file, NULL));
	basename = g_file_get_basename (actual_file);
	g_assert_cmpstr (basename, ==, "default.jpg");
	g_free (basename);
	g_object_unref (actual_file);

	/* to a path, non-existent, should succeed */
	g_assert (g_file_query_exists (dest_file, NULL) == FALSE);
	actual_file = gdata_media_content_download (content, service, "default.jpg", dest_file, FALSE, NULL, &error);
	g_assert_no_error (error);
	g_assert (g_file_query_exists (actual_file, NULL));
	basename = g_file_get_basename (actual_file);
	g_assert_cmpstr (basename, ==, "test.jpg");
	g_free (basename);
	g_object_unref (actual_file);

	/* to a path, existent, without replace, should fail */
	actual_file = gdata_media_content_download (content, service, "default.jpg", dest_file, FALSE, NULL, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_EXISTS);
	g_clear_error (&error);
	g_assert (actual_file == NULL);

	/* to a path, existent, with replace, should succeed */
	actual_file = gdata_media_content_download (content, service, "default.jpg", dest_file, TRUE, NULL, &error);
	g_assert_no_error (error);
	g_assert (g_file_query_exists (actual_file, NULL));
	basename = g_file_get_basename (actual_file);
	g_assert_cmpstr (basename, ==, "test.jpg");
	g_free (basename);
	g_object_unref (actual_file);

	/* clean up test directory */
	delete_directory (dest_dir, &error);
	g_assert_no_error (error);

	g_object_unref (photo_feed);
	g_object_unref (album_feed);
	g_object_unref (dest_dir);
	g_object_unref (dest_file);
}

static void
test_upload_simple (gconstpointer service)
{
	GDataPicasaWebFile *photo, *photo_new;
	GFile *photo_file;
	GError *error = NULL;
	GTimeVal timeval;
	gchar *xml, *time_str, *summary, *expected_xml, *parsed_time_str;
	GRegex *regex;
	GMatchInfo *match_info;
	guint64 delta;
	const gchar * const tags[] = { "foo", "bar", ",,baz,baz", NULL };
	const gchar * const *tags2;

	g_get_current_time (&timeval);
	time_str = g_time_val_to_iso8601 (&timeval);
	summary = g_strdup_printf ("Photo Summary (%s)", time_str);

	expected_xml = g_strdup_printf ("<entry "
						"xmlns='http://www.w3.org/2005/Atom' "
						"xmlns:gphoto='http://schemas.google.com/photos/2007' "
						"xmlns:media='http://search.yahoo.com/mrss/' "
						"xmlns:gd='http://schemas.google.com/g/2005' "
						"xmlns:exif='http://schemas.google.com/photos/exif/2007' "
						"xmlns:app='http://www.w3.org/2007/app' "
						"xmlns:georss='http://www.georss.org/georss' "
						"xmlns:gml='http://www.opengis.net/gml'>"
						"<title type='text'>Photo Entry Title</title>"
						"<summary type='text'>Photo Summary \\(%s\\)</summary>"
						"<gphoto:position>0</gphoto:position>"
						"<gphoto:timestamp>([0-9]+)</gphoto:timestamp>"
						"<gphoto:commentingEnabled>true</gphoto:commentingEnabled>"
						"<media:group>"
							"<media:title type='plain'>Photo Entry Title</media:title>"
							"<media:description type='plain'>Photo Summary \\(%s\\)</media:description>"
							"<media:keywords>foo,bar,%%2C%%2Cbaz%%2Cbaz</media:keywords>"
						"</media:group>"
					"</entry>", time_str, time_str);
	g_free (time_str);

	/* Build a regex to match the timestamp from the XML, since we can't definitely say what it'll be */
	regex = g_regex_new (expected_xml, 0, 0, NULL);
	g_free (expected_xml);

	/* Build the photo */
	photo = gdata_picasaweb_file_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (photo), "Photo Entry Title");
	gdata_picasaweb_file_set_caption (photo, summary);
	gdata_picasaweb_file_set_tags (photo, tags);

	/* Check the XML: match it against the regex built above, then check that the timestamp is within 100ms of the current time at the start of
	 * the test function. We can't check it exactly, as a few milliseconds may have passed inbetween building the expected_xml and building the XML
	 * for the photo. */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (photo));
	g_assert (g_regex_match (regex, xml, 0, &match_info) == TRUE);
	parsed_time_str = g_match_info_fetch (match_info, 1);
	delta = g_ascii_strtoull (parsed_time_str, NULL, 10) - (((guint64) timeval.tv_sec) * 1000 + ((guint64) timeval.tv_usec) / 1000);
	g_assert_cmpuint (abs (delta), <, 100);

	g_free (parsed_time_str);
	g_free (xml);
	g_regex_unref (regex);
	g_match_info_free (match_info);

	gdata_picasaweb_file_set_coordinates (photo, 17.127, -110.35);

	/* File is public domain: http://en.wikipedia.org/wiki/File:German_garden_gnome_cropped.jpg */
	photo_file = g_file_new_for_path (TEST_FILE_DIR "photo.jpg");

	/* Upload the photo */
	/* TODO right now, it will just go to the default album, we want an uploading one :| */
	photo_new = gdata_picasaweb_service_upload_file (GDATA_PICASAWEB_SERVICE (service), NULL, photo, photo_file, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_PICASAWEB_FILE (photo_new));
	g_clear_error (&error);

	/* TODO: check entries and feed properties */
	tags2 = gdata_picasaweb_file_get_tags (photo_new);
	g_assert_cmpuint (g_strv_length ((gchar**) tags2), ==, 3);
	g_assert_cmpstr (tags2[0], ==, tags[0]);
	g_assert_cmpstr (tags2[1], ==, tags[1]);
	g_assert_cmpstr (tags2[2], ==, tags[2]);

	g_free (summary);
	g_object_unref (photo);
	g_object_unref (photo_new);
	g_object_unref (photo_file);
}

static void
test_photo (gconstpointer service)
{
	GError *error = NULL;
	GDataFeed *album_feed;
	GDataFeed *photo_feed;
	GList *albums;
	GList *files;
	GDataEntry *album_entry;
	GDataEntry *photo_entry;
	GDataPicasaWebAlbum *album;
	GDataPicasaWebFile *photo;
	GList *list;
	GDataMediaContent *content;
	GDataMediaThumbnail *thumbnail;
	GTimeVal _time;
	gchar *str;
	gchar *timestamp;
	const gchar * const *tags;
	gdouble latitude;
	gdouble longitude;
	gdouble original_latitude;
	gdouble original_longitude;

	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (album_feed));
	g_clear_error (&error);

	albums = gdata_feed_get_entries (album_feed);
	album_entry = GDATA_ENTRY (g_list_nth_data (albums, TEST_ALBUM_INDEX));
	album = GDATA_PICASAWEB_ALBUM (album_entry);

	photo_feed = gdata_picasaweb_service_query_files (GDATA_PICASAWEB_SERVICE (service), GDATA_PICASAWEB_ALBUM (album), NULL, NULL, NULL,
							  NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (photo_feed));
	g_clear_error (&error);

	files = gdata_feed_get_entries (photo_feed);
	photo_entry = GDATA_ENTRY (g_list_nth_data (files, 0));
	photo = GDATA_PICASAWEB_FILE (photo_entry);

	gdata_picasaweb_file_get_edited (photo, &_time);
	str = g_time_val_to_iso8601 (&_time);
	g_assert_cmpstr (str, ==, "2009-10-17T08:52:18.885000Z");
	g_free (str);

	/* tests */

	g_assert_cmpstr (gdata_picasaweb_file_get_caption (photo), ==, "Ginger cookie caption");
	g_assert_cmpstr (gdata_picasaweb_file_get_version (photo), ==, "29"); /* 1240729023474000"); */ /* TODO check how constant this even is */
	g_assert_cmpfloat (gdata_picasaweb_file_get_position (photo), ==, 0.0);
	g_assert_cmpstr (gdata_picasaweb_file_get_album_id (photo), ==, "5328889949261497249");
	g_assert_cmpuint (gdata_picasaweb_file_get_width (photo), ==, 2576);
	g_assert_cmpuint (gdata_picasaweb_file_get_height (photo), ==, 1932);
	g_assert_cmpuint (gdata_picasaweb_file_get_size (photo), ==, 1124730);
	/* TODO: file wasn't uploaded with client assigned; g_assert_cmpstr (gdata_picasaweb_file_get_client (photo), ==, ??); */
	/* TODO: file wasn't uploaded with checksum assigned; g_assert_cmpstr (gdata_picasaweb_file_get_checksum (photo), ==, ??); */

	gdata_picasaweb_file_get_timestamp (photo, &_time);
	timestamp = g_time_val_to_iso8601 (&_time);
	g_assert_cmpstr (timestamp, ==, "2008-12-06T18:32:10Z");

	g_assert_cmpstr (gdata_picasaweb_file_get_video_status (photo), ==, NULL);
	/* TODO: not a good test of video status; want to upload a video for it */
	g_assert_cmpuint (gdata_picasaweb_file_is_commenting_enabled (photo), ==, TRUE);
	g_assert_cmpuint (gdata_picasaweb_file_get_comment_count (photo), ==, 2);
	g_assert_cmpuint (gdata_picasaweb_file_get_rotation (photo), ==, 0);

	g_assert_cmpstr (gdata_picasaweb_file_get_caption (photo), ==, "Ginger cookie caption");
	tags = gdata_picasaweb_file_get_tags (photo);
	g_assert (tags != NULL);
	g_assert_cmpstr (tags[0], ==, "cookies");
	g_assert (tags[1] == NULL);
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (photo)), ==, "100_0269.jpg");

	g_assert_cmpstr (gdata_picasaweb_file_get_credit (photo), ==, "libgdata.picasaweb");

	/* Check EXIF values */
	g_assert_cmpfloat (gdata_picasaweb_file_get_distance (photo), ==, 0);
	g_assert_cmpfloat (gdata_picasaweb_file_get_exposure (photo), ==, 0.016666668);
	g_assert_cmpint (gdata_picasaweb_file_get_flash (photo), ==, TRUE);
	g_assert_cmpfloat (gdata_picasaweb_file_get_focal_length (photo), ==, 6.3);
	g_assert_cmpfloat (gdata_picasaweb_file_get_fstop (photo), ==, 2.8);
	g_assert_cmpstr (gdata_picasaweb_file_get_image_unique_id (photo), ==, "1c179e0ac4f6741c8c1cdda3516e69e5");
	g_assert_cmpint (gdata_picasaweb_file_get_iso (photo), ==, 80);
	g_assert_cmpstr (gdata_picasaweb_file_get_make (photo), ==, "EASTMAN KODAK COMPANY");
	g_assert_cmpstr (gdata_picasaweb_file_get_model (photo), ==, "KODAK Z740 ZOOM DIGITAL CAMERA");

	/* Check GeoRSS coordinates */
	gdata_picasaweb_file_get_coordinates (photo, &original_latitude, &original_longitude);
	g_assert_cmpfloat (original_latitude, ==, 45.4341173);
	g_assert_cmpfloat (original_longitude, ==, 12.1289062);

	gdata_picasaweb_file_get_coordinates (photo, NULL, &longitude);
	g_assert_cmpfloat (longitude, ==, 12.1289062);
	gdata_picasaweb_file_get_coordinates (photo, &latitude, NULL);
	g_assert_cmpfloat (latitude, ==, 45.4341173);
	gdata_picasaweb_file_get_coordinates (photo, NULL, NULL);

	gdata_picasaweb_file_set_coordinates (photo, original_longitude, original_latitude);
	gdata_picasaweb_file_get_coordinates (photo, &latitude, &longitude);
	g_assert_cmpfloat (latitude, ==, original_longitude);
	g_assert_cmpfloat (longitude, ==, original_latitude);
	gdata_picasaweb_file_set_coordinates (photo, original_latitude, original_longitude);
	gdata_picasaweb_file_get_coordinates (photo, &latitude, &longitude);
	g_assert_cmpfloat (latitude, ==, 45.4341173);
	g_assert_cmpfloat (longitude, ==, 12.1289062);

	/* Check Media */
	list = gdata_picasaweb_file_get_contents (photo);
	g_assert_cmpuint (g_list_length (list), ==, 1);

	content = GDATA_MEDIA_CONTENT (list->data);
	g_assert_cmpstr (gdata_media_content_get_uri (content), ==,
			 "http://lh3.ggpht.com/_1kdcGyvOb8c/SfQFWPnuovI/AAAAAAAAAB0/MI0L4Sd11Eg/100_0269.jpg");
	g_assert_cmpstr (gdata_media_content_get_content_type (content), ==, "image/jpeg");
	g_assert_cmpuint (gdata_media_content_get_width (content), ==, 1600);
	g_assert_cmpuint (gdata_media_content_get_height (content), ==, 1200);
	g_assert_cmpuint (gdata_media_content_get_medium (content), ==, GDATA_MEDIA_IMAGE);

	g_assert_cmpuint (gdata_media_content_is_default (content), ==, FALSE);
	g_assert_cmpint (gdata_media_content_get_duration (content), ==, 0); /* doesn't apply to photos */
	g_assert_cmpuint (gdata_media_content_get_filesize (content), ==, 0); /* PicasaWeb doesn't set anything better */
	g_assert_cmpuint (gdata_media_content_get_expression (content), ==, GDATA_MEDIA_EXPRESSION_FULL);
	/* TODO: really want to test these with a video clip */

	list = gdata_picasaweb_file_get_thumbnails (photo);
	g_assert_cmpuint (g_list_length (list), ==, 3);

	thumbnail = GDATA_MEDIA_THUMBNAIL (list->data);
	g_assert_cmpstr (gdata_media_thumbnail_get_uri (thumbnail), ==,
			 "http://lh3.ggpht.com/_1kdcGyvOb8c/SfQFWPnuovI/AAAAAAAAAB0/MI0L4Sd11Eg/s288/100_0269.jpg");
	g_assert_cmpuint (gdata_media_thumbnail_get_width (thumbnail), ==, 288);
	g_assert_cmpuint (gdata_media_thumbnail_get_height (thumbnail), ==, 216);
	g_assert_cmpint (gdata_media_thumbnail_get_time (thumbnail), ==, -1); /* PicasaWeb doesn't set anything better */

	g_free (timestamp);
}

static void
test_photo_feed_entry (gconstpointer service)
{
	GDataFeed *album_feed;
	GDataFeed *photo_feed;
	GError *error = NULL;
	GDataEntry *entry;
	GDataPicasaWebAlbum *album;
	GList *albums;
	GList *files;
	GDataEntry *photo_entry;
	gchar *str;
	GTimeVal _time;

	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (album_feed));
	g_clear_error (&error);

	albums = gdata_feed_get_entries (album_feed);
	entry = GDATA_ENTRY (g_list_nth_data (albums, TEST_ALBUM_INDEX));
	album = GDATA_PICASAWEB_ALBUM (entry);

	photo_feed = gdata_picasaweb_service_query_files (GDATA_PICASAWEB_SERVICE (service), GDATA_PICASAWEB_ALBUM (album), NULL, NULL, NULL,
							  NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (photo_feed));
	g_clear_error (&error);

	files = gdata_feed_get_entries (photo_feed);
	photo_entry = GDATA_ENTRY (g_list_nth_data (files, 0));

	/* tests */

	g_assert_cmpuint (g_list_length (files), ==, 1);

	g_assert_cmpstr (gdata_entry_get_title (photo_entry), ==, "100_0269.jpg");
	g_assert_cmpstr (gdata_picasaweb_file_get_id (GDATA_PICASAWEB_FILE (photo_entry)), ==, "5328890138794566386");
	g_assert_cmpstr (gdata_entry_get_id (photo_entry), ==, "http://picasaweb.google.com/data/entry/user/libgdata.picasaweb/albumid/5328889949261497249/photoid/5328890138794566386");
	g_assert_cmpstr (gdata_entry_get_etag (photo_entry), !=, NULL);

	gdata_entry_get_updated (photo_entry, &_time);
	str = g_time_val_to_iso8601 (&_time);
	g_assert_cmpstr (str, ==, "2009-10-17T08:52:18.885000Z");
	g_free (str);

	gdata_entry_get_published (photo_entry, &_time);
	str = g_time_val_to_iso8601 (&_time);
	g_assert_cmpstr (str, ==, "2009-04-26T06:55:20Z");
	g_free (str);

	g_assert_cmpstr (gdata_entry_get_content (photo_entry), ==,
			 "http://lh3.ggpht.com/_1kdcGyvOb8c/SfQFWPnuovI/AAAAAAAAAB0/MI0L4Sd11Eg/100_0269.jpg");
	g_assert_cmpstr (gdata_parsable_get_xml (GDATA_PARSABLE (photo_entry)), !=, NULL);
	g_assert_cmpuint (strlen (gdata_parsable_get_xml (GDATA_PARSABLE (photo_entry))), >, 0);
}

static void
test_photo_feed (gconstpointer service)
{
	GError *error = NULL;
	GDataFeed *album_feed;
	GDataFeed *photo_feed;
	GDataEntry *entry;
	GDataPicasaWebAlbum *album;
	GList *albums;

	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (album_feed));
	g_clear_error (&error);

	albums = gdata_feed_get_entries (album_feed);
	entry = GDATA_ENTRY (g_list_nth_data (albums, TEST_ALBUM_INDEX));
	album = GDATA_PICASAWEB_ALBUM (entry);

	/* tests */

	photo_feed = gdata_picasaweb_service_query_files (GDATA_PICASAWEB_SERVICE (service), GDATA_PICASAWEB_ALBUM (album), NULL, NULL, NULL,
							  NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (photo_feed));
	g_clear_error (&error);

	g_assert_cmpstr (gdata_feed_get_title (photo_feed), ==, "Test Album 1 - Venice - Public");
	g_assert_cmpstr (gdata_feed_get_id (photo_feed), ==,
			 "http://picasaweb.google.com/data/feed/user/libgdata.picasaweb/albumid/5328889949261497249");
	g_assert_cmpstr (gdata_feed_get_etag (photo_feed), !=, NULL);
	g_assert_cmpuint (gdata_feed_get_items_per_page (photo_feed), ==, 1000);
	g_assert_cmpuint (gdata_feed_get_start_index (photo_feed), ==, 1);
	g_assert_cmpuint (gdata_feed_get_total_results (photo_feed), ==, 1);
}

static void
test_album (gconstpointer service)
{
	GDataFeed *album_feed;
	GDataPicasaWebAlbum *album;
	GList *albums, *contents, *thumbnails;
	GTimeVal _time;
	gchar *str, *original_rights;
	gdouble latitude, longitude, original_latitude, original_longitude;
	GDataMediaContent *content;
	GDataMediaThumbnail *thumbnail;
	GError *error = NULL;

	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (album_feed));
	g_clear_error (&error);

	albums = gdata_feed_get_entries (album_feed);
	album = GDATA_PICASAWEB_ALBUM (g_list_nth_data (albums, TEST_ALBUM_INDEX));

	/* Tests */
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (album)), ==, "Test Album 1 - Venice - Public");
	g_assert_cmpstr (gdata_entry_get_summary (GDATA_ENTRY (album)), ==, "This is the test description.  This album should be in Venice.");

	/* Check album-specific API */
	g_assert_cmpstr (gdata_picasaweb_album_get_user (album), ==, "libgdata.picasaweb");
	g_assert_cmpstr (gdata_picasaweb_album_get_nickname (album), ==, "libgdata.picasaweb");

	gdata_picasaweb_album_get_edited (album, &_time);
	str = g_time_val_to_iso8601 (&_time);
	g_assert_cmpstr (str, ==, "2009-04-26T06:57:03.474000Z");
	g_free (str);

	g_assert_cmpint (gdata_picasaweb_album_get_visibility (album), ==, GDATA_PICASAWEB_PUBLIC);
	g_assert_cmpstr (gdata_picasaweb_album_get_location (album), ==, "Venice");

	gdata_picasaweb_album_get_timestamp (album, &_time);
	str = g_time_val_to_iso8601 (&_time);
	g_assert_cmpstr (str, ==, "2009-04-26T07:00:00Z");
	g_free (str);

	g_assert_cmpuint (gdata_picasaweb_album_get_num_photos (album), ==, 1);
	g_assert_cmpuint (gdata_picasaweb_album_get_num_photos_remaining (album), >, 0); /* about 999 remaining, testing weakly to avoid having to update regularly */
	g_assert_cmpuint (gdata_picasaweb_album_get_bytes_used (album), ==, 1124730);

	/* Check GeoRSS coordinates */
	gdata_picasaweb_album_get_coordinates (album, &latitude, &longitude);
	g_assert_cmpfloat (latitude, ==, 45.434336);
	gdata_picasaweb_album_get_coordinates (album, &original_latitude, &original_longitude);
	g_assert_cmpfloat (original_latitude, ==, 45.434336);
	g_assert_cmpfloat (original_longitude, ==, 12.338784);

	gdata_picasaweb_album_get_coordinates (album, NULL, &longitude);
	g_assert_cmpfloat (longitude, ==, 12.338784);
	gdata_picasaweb_album_get_coordinates (album, &latitude, NULL);
	g_assert_cmpfloat (latitude, ==, 45.434336);
	gdata_picasaweb_album_get_coordinates (album, NULL, NULL);

	gdata_picasaweb_album_set_coordinates (album, original_longitude, original_latitude);
	gdata_picasaweb_album_get_coordinates (album, &latitude, &longitude);
	g_assert_cmpfloat (latitude, ==, original_longitude);
	g_assert_cmpfloat (longitude, ==, original_latitude);
	gdata_picasaweb_album_set_coordinates (album, original_latitude, original_longitude);
	gdata_picasaweb_album_get_coordinates (album, &original_latitude, &original_longitude);
	g_assert_cmpfloat (original_latitude, ==, 45.434336);
	g_assert_cmpfloat (original_longitude, ==, 12.338784);

	/* Test visibility and its synchronisation with its GDataEntry's rights */
	original_rights = g_strdup (gdata_entry_get_rights (GDATA_ENTRY (album)));

	gdata_entry_set_rights (GDATA_ENTRY (album), "private");
	g_assert_cmpstr (gdata_entry_get_rights (GDATA_ENTRY (album)), ==, "private");
	g_assert_cmpint (gdata_picasaweb_album_get_visibility (album), ==, GDATA_PICASAWEB_PRIVATE);

	gdata_entry_set_rights (GDATA_ENTRY (album), "public");
	g_assert_cmpstr (gdata_entry_get_rights (GDATA_ENTRY (album)), ==, "public");
	g_assert_cmpint (gdata_picasaweb_album_get_visibility (album), ==, GDATA_PICASAWEB_PUBLIC);

	gdata_picasaweb_album_set_visibility (album, GDATA_PICASAWEB_PRIVATE);
	g_assert_cmpstr (gdata_entry_get_rights (GDATA_ENTRY (album)), ==, "private");
	g_assert_cmpint (gdata_picasaweb_album_get_visibility (album), ==, GDATA_PICASAWEB_PRIVATE);

	gdata_picasaweb_album_set_visibility (album, GDATA_PICASAWEB_PUBLIC);
	g_assert_cmpstr (gdata_entry_get_rights (GDATA_ENTRY (album)), ==, "public");
	g_assert_cmpint (gdata_picasaweb_album_get_visibility (album), ==, GDATA_PICASAWEB_PUBLIC);

	gdata_entry_set_rights (GDATA_ENTRY (album), original_rights);
	g_free (original_rights);

	/* Check Media */
	g_assert (gdata_picasaweb_album_get_tags (album) == NULL);
	/* TODO: they return a <media:keywords></...> but it's empty and the web interface can't set it;
	   try setting it programmatically; if we can't do that either, consider removing API */

	contents = gdata_picasaweb_album_get_contents (album);
	g_assert_cmpuint (g_list_length (contents), ==, 1);
	content = GDATA_MEDIA_CONTENT (contents->data);

	g_assert_cmpstr (gdata_media_content_get_uri (content), ==,
			 "http://lh5.ggpht.com/_1kdcGyvOb8c/SfQFLNjhg6E/AAAAAAAAAB8/2WtMjZCa71k/TestAlbum1VenicePublic.jpg");
	g_assert_cmpstr (gdata_media_content_get_content_type (content), ==, "image/jpeg");
	g_assert_cmpuint (gdata_media_content_get_medium (content), ==, GDATA_MEDIA_IMAGE);

	g_assert_cmpuint (gdata_media_content_is_default (content), ==, FALSE);
	g_assert_cmpint (gdata_media_content_get_duration (content), ==, 0); /* doesn't apply to photos */
	g_assert_cmpuint (gdata_media_content_get_width (content), ==, 0); /* PicasaWeb doesn't set anything better */
	g_assert_cmpuint (gdata_media_content_get_height (content), ==, 0); /* PicasaWeb doesn't set anything better */
	g_assert_cmpuint (gdata_media_content_get_filesize (content), ==, 0); /* PicasaWeb doesn't set anything better */
	g_assert_cmpuint (gdata_media_content_get_expression (content), ==, GDATA_MEDIA_EXPRESSION_FULL);

	thumbnails = gdata_picasaweb_album_get_thumbnails (album);
	g_assert_cmpuint (g_list_length (thumbnails), ==, 1);
	thumbnail = GDATA_MEDIA_THUMBNAIL (thumbnails->data);

	g_assert_cmpstr (gdata_media_thumbnail_get_uri (thumbnail), ==,
			 "http://lh5.ggpht.com/_1kdcGyvOb8c/SfQFLNjhg6E/AAAAAAAAAB8/2WtMjZCa71k/s160-c/TestAlbum1VenicePublic.jpg");
	g_assert_cmpint (gdata_media_thumbnail_get_time (thumbnail), ==, -1); /* PicasaWeb doesn't set anything better */
	g_assert_cmpint (gdata_media_thumbnail_get_width (thumbnail), ==, 160);
	g_assert_cmpint (gdata_media_thumbnail_get_height (thumbnail), ==, 160);

	g_object_unref (album_feed);
}

static void
test_album_feed_entry (gconstpointer service)
{
	GDataFeed *album_feed;
	GError *error = NULL;
	GDataEntry *entry;
	GList *albums;
	gchar *str, *xml;
	GTimeVal _time;

	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (album_feed));
	g_clear_error (&error);

	albums = gdata_feed_get_entries (album_feed);
	g_assert_cmpuint (g_list_length (albums), ==, NUM_ALBUMS);

	entry = GDATA_ENTRY (g_list_nth_data (albums, TEST_ALBUM_INDEX));
	g_assert (entry != NULL);

	g_object_ref (entry);
	g_object_unref (album_feed);

	/* Tests */
	g_assert_cmpstr (gdata_entry_get_title (entry), ==, "Test Album 1 - Venice - Public");
	g_assert_cmpstr (gdata_picasaweb_album_get_id (GDATA_PICASAWEB_ALBUM (entry)), ==, "5328889949261497249");
	g_assert_cmpstr (gdata_entry_get_id (entry), ==, "http://picasaweb.google.com/data/entry/user/libgdata.picasaweb/albumid/5328889949261497249");
	g_assert_cmpstr (gdata_entry_get_etag (entry), !=, NULL);
	g_assert_cmpstr (gdata_entry_get_rights (entry), ==, "public");

	gdata_entry_get_updated (entry, &_time);
	str = g_time_val_to_iso8601 (&_time);
	g_assert_cmpstr (str, ==, "2009-04-26T06:57:03.474000Z");
	g_free (str);

	gdata_entry_get_published (entry, &_time);
	str = g_time_val_to_iso8601 (&_time);
	g_assert_cmpstr (str, ==, "2009-04-26T07:00:00Z");
	g_free (str);

	xml = gdata_parsable_get_xml (GDATA_PARSABLE (entry));
	g_assert_cmpstr (xml, !=, NULL);
	g_assert_cmpuint (strlen (xml), >, 0);
	g_free (xml);

	g_object_unref (entry);
}

static void
test_album_feed (gconstpointer service)
{
	GDataFeed *album_feed;
	GError *error = NULL;

	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (album_feed));
	g_clear_error (&error);

	/* tests */

	g_assert_cmpstr (gdata_feed_get_title (album_feed), ==, "libgdata.picasaweb");
	/* TODO find out why subtitle == null when returned: no subtitle for feed? printf("feed subtitle: %s\n", gdata_feed_get_subtitle(feed)); */
	g_assert_cmpstr (gdata_feed_get_id (album_feed), ==, "http://picasaweb.google.com/data/feed/user/libgdata.picasaweb");
	g_assert_cmpstr (gdata_feed_get_etag (album_feed), !=, NULL); /* this varies as albums change, like when a new image is uploaded in our test! */
	g_assert_cmpstr (gdata_feed_get_icon (album_feed), ==, "http://lh6.ggpht.com/_1kdcGyvOb8c/AAAA9mDag3s/AAAAAAAAAAA/Jq-NWYWKFao/s64-c/libgdata.picasaweb.jpg");
	g_assert_cmpuint (gdata_feed_get_items_per_page (album_feed), ==, 1000);
	g_assert_cmpuint (gdata_feed_get_start_index (album_feed), ==, 1);
	g_assert_cmpuint (gdata_feed_get_total_results (album_feed), ==, NUM_ALBUMS);
}

static void
test_insert_album (gconstpointer service)
{
	GDataPicasaWebAlbum *album;
	GDataPicasaWebAlbum *inserted_album;
	GTimeVal timestamp;
	gchar *timestr;
	GError *error;

	GDataFeed *album_feed;
	GList *albums;
	gboolean album_found;
	GList *node;

	error = NULL;

	album = gdata_picasaweb_album_new (NULL);
	g_assert (GDATA_IS_PICASAWEB_ALBUM (album));

	gdata_entry_set_title (GDATA_ENTRY (album), "Thanksgiving photos");
	gdata_entry_set_summary (GDATA_ENTRY (album), "Family photos of the feast!");
	gdata_picasaweb_album_set_location (album, "Winnipeg, MN");

	g_time_val_from_iso8601 ("2002-10-14T09:58:59.643554Z", &timestamp);
	gdata_picasaweb_album_set_timestamp (album, &timestamp);

	inserted_album = gdata_picasaweb_service_insert_album (GDATA_PICASAWEB_SERVICE (service), album, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_PICASAWEB_ALBUM (inserted_album));

	/* Test that it returns what we gave */
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (inserted_album)), ==, "Thanksgiving photos");
	g_assert_cmpstr (gdata_entry_get_summary (GDATA_ENTRY (inserted_album)), ==, "Family photos of the feast!");
	g_assert_cmpstr (gdata_picasaweb_album_get_location (inserted_album), ==, "Winnipeg, MN");

	gdata_picasaweb_album_get_timestamp (inserted_album, &timestamp);
	timestr = g_time_val_to_iso8601 (&timestamp);
	g_assert_cmpstr (timestr, ==, "2002-10-14T09:58:59Z");
	g_free (timestr);

	/* Test that album is actually on server */
	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	albums = gdata_feed_get_entries (album_feed);

	album_found = FALSE;
	for (node = albums; node != NULL; node = node->next) {
		if (g_strcmp0 (gdata_entry_get_title (GDATA_ENTRY (node->data)), "Thanksgiving photos")) {
			album_found = TRUE;
		}
	}
	g_assert (album_found);

	/* Clean up the evidence */
	gdata_service_delete_entry (GDATA_SERVICE (service), GDATA_ENTRY (inserted_album), NULL, &error);
	g_assert_no_error (error);

	g_object_unref (album_feed);
	g_object_unref (album);
	g_object_unref (inserted_album);
}

static void
test_query_all_albums (gconstpointer service)
{
	GDataFeed *album_feed, *photo_feed;
	GDataQuery *query;
	GError *error = NULL;
	GList *albums;
	GDataEntry *entry;
	GDataPicasaWebAlbum *album;

	/* Test a query with a "q" parameter; it should fail */
	query = GDATA_QUERY (gdata_picasaweb_query_new ("foobar"));
	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), query, NULL, NULL, NULL, NULL, &error);
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_BAD_QUERY_PARAMETER);
	g_assert (album_feed == NULL);
	g_clear_error (&error);

	/* Now try a proper query */
	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (album_feed));
	g_clear_error (&error);

	albums = gdata_feed_get_entries (album_feed);
	entry = GDATA_ENTRY (g_list_nth_data (albums, TEST_ALBUM_INDEX));
	album = GDATA_PICASAWEB_ALBUM (entry);

	photo_feed = gdata_picasaweb_service_query_files (GDATA_PICASAWEB_SERVICE (service), GDATA_PICASAWEB_ALBUM (album), NULL, NULL, NULL,
							  NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (photo_feed));
	g_clear_error (&error);

	g_object_unref (photo_feed);
	g_object_unref (album_feed);
}

static void
test_query_user (gconstpointer service)
{
	GDataPicasaWebUser *user;
	GError *error = NULL;

	user = gdata_picasaweb_service_get_user (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_PICASAWEB_USER (user));
	g_clear_error (&error);

	g_assert_cmpstr (gdata_picasaweb_user_get_user (user), ==, "libgdata.picasaweb");
	g_assert_cmpstr (gdata_picasaweb_user_get_nickname (user), ==, "libgdata.picasaweb");
	g_assert_cmpint (gdata_picasaweb_user_get_quota_limit (user), ==, 1073741824); /* 1GiB: it'll be a beautiful day when this assert gets tripped */
	g_assert_cmpint (gdata_picasaweb_user_get_quota_current (user), >, 0);
	g_assert_cmpint (gdata_picasaweb_user_get_max_photos_per_album (user), >, 0); /* now it's 1000, testing this weakly to avoid having to regularly update it */
	g_assert_cmpstr (gdata_picasaweb_user_get_thumbnail_uri (user), ==, "http://lh6.ggpht.com/_1kdcGyvOb8c/AAAA9mDag3s/AAAAAAAAAAA/Jq-NWYWKFao/s64-c/libgdata.picasaweb.jpg");

	g_object_unref (user);
}

static void
test_query_new_with_limits (gconstpointer service)
{
	GDataQuery *query;
	GDataFeed *album_feed_1, *album_feed_2;
	GError *error;
	GList *albums_1, *albums_2;

	error = NULL;

	/* Test a query with a "q" parameter; it should fail */
	query = GDATA_QUERY (gdata_picasaweb_query_new_with_limits ("foobar", 1, 1));
	album_feed_1 = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), query, NULL, NULL, NULL, NULL, &error);
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_BAD_QUERY_PARAMETER);
	g_assert (album_feed_1 == NULL);
	g_clear_error (&error);
	g_object_unref (query);

	/* Test that two queries starting at different indices don't return the same content */
	query = GDATA_QUERY (gdata_picasaweb_query_new_with_limits (NULL, 1, 1));
	album_feed_1 = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), query, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (album_feed_1));
	albums_1 = gdata_feed_get_entries (album_feed_1);
	g_object_unref (query);

	query = GDATA_QUERY (gdata_picasaweb_query_new_with_limits (NULL, 2, 1));
	album_feed_2 = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), query, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (album_feed_2));
	albums_2 = gdata_feed_get_entries (album_feed_2);
	g_object_unref (query);

	g_assert_cmpint (g_list_length (albums_1), ==, 1);
	g_assert_cmpint (g_list_length (albums_2), ==, 1);
	g_assert (GDATA_IS_ENTRY (albums_1->data));
	g_assert (GDATA_IS_ENTRY (albums_2->data));
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (albums_1->data)), !=, gdata_entry_get_title (GDATA_ENTRY (albums_2->data)));

	g_object_unref (album_feed_1);
	g_object_unref (album_feed_2);

	/* Test that we get at most as many results as we requested */
	query = GDATA_QUERY (gdata_picasaweb_query_new_with_limits (NULL, 1, 3));
	album_feed_1 = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), query, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (album_feed_1));
	albums_1 = gdata_feed_get_entries (album_feed_1);
	g_object_unref (query);

	g_assert_cmpint (g_list_length (albums_1), ==, 3);

	g_object_unref (album_feed_1);
}

static void
test_query_all_albums_async_cb (GDataService *service, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GDataFeed *feed;
	GError *error = NULL;

	feed = gdata_service_query_finish (service, async_result, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* @TODO: Tests? */
	g_main_loop_quit (main_loop);

	g_object_unref (feed);
}

static void
test_query_all_albums_async (gconstpointer service)
{
	GMainLoop *main_loop = g_main_loop_new (NULL, TRUE);

	gdata_picasaweb_service_query_all_albums_async (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL,
							NULL, (GAsyncReadyCallback) test_query_all_albums_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
}

static void
test_album_new (gconstpointer service)
{
	GDataPicasaWebAlbum *album;
	gchar *xml, *parsed_time_str;
	GRegex *regex;
	GMatchInfo *match_info;
	guint64 delta;
	GTimeVal timeval;

	g_test_bug ("598893");

	/* Get the current time */
	g_get_current_time (&timeval);

	/* Build a regex to match the timestamp from the XML, since we can't definitely say what it'll be */
	regex = g_regex_new ("<entry xmlns='http://www.w3.org/2005/Atom' "
				    "xmlns:gphoto='http://schemas.google.com/photos/2007' "
				    "xmlns:media='http://search.yahoo.com/mrss/' "
				    "xmlns:gd='http://schemas.google.com/g/2005' "
				    "xmlns:gml='http://www.opengis.net/gml' "
				    "xmlns:app='http://www.w3.org/2007/app' "
				    "xmlns:georss='http://www.georss.org/georss'>"
					"<title type='text'></title>"
					"<id>http://picasaweb.google.com/data/entry/user/libgdata.picasaweb/albumid/5328889949261497249</id>"
					"<gphoto:id>5328889949261497249</gphoto:id>"
					"<gphoto:access>private</gphoto:access>"
					"<gphoto:timestamp>([0-9]+)</gphoto:timestamp>"
					"<gphoto:commentingEnabled>false</gphoto:commentingEnabled>"
					"<media:group/>"
			     "</entry>", 0, 0, NULL);

	/* Build the album */
	album = gdata_picasaweb_album_new ("http://picasaweb.google.com/data/entry/user/libgdata.picasaweb/albumid/5328889949261497249");
	g_assert (GDATA_IS_PICASAWEB_ALBUM (album));

	/* Check the XML: match it against the regex built above, then check that the timestamp is within 100ms of the current time at the start of
	 * the test function. We can't check it exactly, as a few milliseconds may have passed inbetween building the expected XML and building the XML
	 * for the photo. */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (album));
	g_assert (g_regex_match (regex, xml, 0, &match_info) == TRUE);
	parsed_time_str = g_match_info_fetch (match_info, 1);
	delta = g_ascii_strtoull (parsed_time_str, NULL, 10) - (((guint64) timeval.tv_sec) * 1000 + ((guint64) timeval.tv_usec) / 1000);
	g_assert_cmpuint (abs (delta), <, 100);

	g_free (parsed_time_str);
	g_free (xml);
	g_regex_unref (regex);
	g_match_info_free (match_info);
	g_object_unref (album);
}

static void
test_query_etag (void)
{
	GDataPicasaWebQuery *query = gdata_picasaweb_query_new (NULL);

	/* Test that setting any property will unset the ETag */
	g_test_bug ("613529");

#define CHECK_ETAG(C) \
	gdata_query_set_etag (GDATA_QUERY (query), "foobar");		\
	(C);								\
	g_assert (gdata_query_get_etag (GDATA_QUERY (query)) == NULL);

	CHECK_ETAG (gdata_picasaweb_query_set_visibility (query, GDATA_PICASAWEB_PUBLIC))
	CHECK_ETAG (gdata_picasaweb_query_set_thumbnail_size (query, "500x430"))
	CHECK_ETAG (gdata_picasaweb_query_set_image_size (query, "1024x768"))
	CHECK_ETAG (gdata_picasaweb_query_set_tag (query, "tag"))
	CHECK_ETAG (gdata_picasaweb_query_set_bounding_box (query, 0.0, 1.0, 20.0, 12.5))
	CHECK_ETAG (gdata_picasaweb_query_set_location (query, "Somewhere near here"))

#undef CHECK_ETAG

	g_object_unref (query);
}

/* TODO: test private, public albums, test uploading */
/* TODO: add queries to update albums, files on the server; test those */

int
main (int argc, char *argv[])
{
	GDataService *service;
	gint retval;

	gdata_test_init (&argc, &argv);

	service = GDATA_SERVICE (gdata_picasaweb_service_new (CLIENT_ID));
	gdata_service_authenticate (service, PW_USERNAME, PASSWORD, NULL, NULL);

	g_test_add_func ("/picasaweb/authentication", test_authentication);
	if (g_test_thorough () == TRUE)
		g_test_add_func ("/picasaweb/authentication_async", test_authentication_async);
	g_test_add_data_func ("/picasaweb/upload/photo", service, test_upload_simple);
	if (g_test_thorough () == TRUE)
		g_test_add_data_func ("/picasaweb/upload/photo_async", service, test_upload_async);
	g_test_add_data_func ("/picasaweb/query/all_albums", service, test_query_all_albums);
	g_test_add_data_func ("/picasaweb/query/user", service, test_query_user);
	if (g_test_thorough () == TRUE)
		g_test_add_data_func ("/picasaweb/query/all_albums_async", service, test_query_all_albums_async);
	g_test_add_data_func ("/picasaweb/query/new_with_limits", service, test_query_new_with_limits);
	g_test_add_data_func ("/picasaweb/query/album_feed", service, test_album_feed);
	g_test_add_data_func ("/picasaweb/query/album_feed_entry", service, test_album_feed_entry);
	g_test_add_data_func ("/picasaweb/query/album", service, test_album);
	g_test_add_data_func ("/picasaweb/insert/album", service, test_insert_album);
	g_test_add_data_func ("/picasaweb/query/photo_feed", service, test_photo_feed);
	g_test_add_data_func ("/picasaweb/query/photo_feed_entry", service, test_photo_feed_entry);
	g_test_add_data_func ("/picasaweb/query/photo", service, test_photo);
	g_test_add_data_func ("/picasaweb/upload/photo", service, test_upload_simple);
	g_test_add_data_func ("/picasaweb/download/photo", service, test_download);
	g_test_add_data_func ("/picasaweb/download/thumbnails", service, test_download_thumbnails);
	g_test_add_data_func ("/picasaweb/album/new", service, test_album_new);
	g_test_add_func ("/picasaweb/query/etag", test_query_etag);

	retval = g_test_run ();
	g_object_unref (service);

	return retval;
}
