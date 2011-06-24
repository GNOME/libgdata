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
#ifdef HAVE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

#include "gdata.h"
#include "common.h"

#define PW_USERNAME "libgdata.picasaweb@gmail.com"
/* the following two properties will change if a new album is added */
#define NUM_ALBUMS 5
#define TEST_ALBUM_INDEX 2

/* Assert that two albums have equal properties, but aren't the same object instance. For use in, e.g., comparing an inserted album from the server
 * to the original instance which was inserted. */
static void
assert_albums_equal (GDataPicasaWebAlbum *album1, GDataPicasaWebAlbum *album2, gboolean compare_inserted_data)
{
	gdouble latitude1, longitude1, latitude2, longitude2;
	const gchar * const *tags1, * const *tags2;

	g_assert (GDATA_IS_PICASAWEB_ALBUM (album1));
	g_assert (GDATA_IS_PICASAWEB_ALBUM (album2));

	g_assert (album1 != album2);

	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (album1)), ==, gdata_entry_get_title (GDATA_ENTRY (album2)));
	g_assert_cmpstr (gdata_entry_get_summary (GDATA_ENTRY (album1)), ==, gdata_entry_get_summary (GDATA_ENTRY (album2)));
	g_assert_cmpstr (gdata_entry_get_content (GDATA_ENTRY (album1)), ==, gdata_entry_get_content (GDATA_ENTRY (album2)));
	g_assert_cmpstr (gdata_entry_get_content_uri (GDATA_ENTRY (album1)), ==, gdata_entry_get_content_uri (GDATA_ENTRY (album2)));
	g_assert_cmpstr (gdata_entry_get_rights (GDATA_ENTRY (album1)), ==, gdata_entry_get_rights (GDATA_ENTRY (album2)));

	if (compare_inserted_data == TRUE) {
		g_assert_cmpstr (gdata_entry_get_id (GDATA_ENTRY (album1)), ==, gdata_entry_get_id (GDATA_ENTRY (album2)));
		g_assert_cmpstr (gdata_entry_get_etag (GDATA_ENTRY (album1)), ==, gdata_entry_get_etag (GDATA_ENTRY (album2)));
		g_assert_cmpint (gdata_entry_get_updated (GDATA_ENTRY (album1)), ==, gdata_entry_get_updated (GDATA_ENTRY (album2)));
		g_assert_cmpint (gdata_entry_get_published (GDATA_ENTRY (album1)), ==, gdata_entry_get_published (GDATA_ENTRY (album2)));
	}

	g_assert_cmpstr (gdata_picasaweb_album_get_location (album1), ==, gdata_picasaweb_album_get_location (album2));
	g_assert_cmpint (gdata_picasaweb_album_get_visibility (album1), ==, gdata_picasaweb_album_get_visibility (album2));
	g_assert_cmpint (gdata_picasaweb_album_get_timestamp (album1), ==, gdata_picasaweb_album_get_timestamp (album2));
	g_assert (gdata_picasaweb_album_is_commenting_enabled (album1) == gdata_picasaweb_album_is_commenting_enabled (album2));

	/* Compare coordinates */
	gdata_picasaweb_album_get_coordinates (album1, &latitude1, &longitude1);
	gdata_picasaweb_album_get_coordinates (album2, &latitude2, &longitude2);
	g_assert_cmpfloat (latitude1, ==, latitude2);
	g_assert_cmpfloat (longitude1, ==, longitude2);

	/* Compare tags */
	tags1 = gdata_picasaweb_album_get_tags (album1);
	tags2 = gdata_picasaweb_album_get_tags (album2);

	g_assert ((tags1 == NULL) == (tags2 == NULL));

	if (tags1 != NULL && tags2 != NULL) {
		guint i;

		for (i = 0; tags1[i] != NULL; i++) {
			g_assert_cmpstr (tags1[i], ==, tags2[i]);
		}

		g_assert (tags2[i] == NULL);
	}

	if (compare_inserted_data == TRUE) {
		GList *contents1, *contents2, *thumbnails1, *thumbnails2, *i1, *i2;

		g_assert_cmpstr (gdata_picasaweb_album_get_id (album1), ==, gdata_picasaweb_album_get_id (album2));
		g_assert_cmpstr (gdata_picasaweb_album_get_user (album1), ==, gdata_picasaweb_album_get_user (album2));
		g_assert_cmpstr (gdata_picasaweb_album_get_nickname (album1), ==, gdata_picasaweb_album_get_nickname (album2));
		g_assert_cmpint (gdata_picasaweb_album_get_edited (album1), ==, gdata_picasaweb_album_get_edited (album2));
		g_assert_cmpuint (gdata_picasaweb_album_get_num_photos (album1), ==, gdata_picasaweb_album_get_num_photos (album2));
		g_assert_cmpuint (gdata_picasaweb_album_get_num_photos_remaining (album1), ==,
		                  gdata_picasaweb_album_get_num_photos_remaining (album2));
		g_assert_cmpuint (gdata_picasaweb_album_get_bytes_used (album1), ==, gdata_picasaweb_album_get_bytes_used (album2));
		g_assert_cmpuint (gdata_picasaweb_album_get_comment_count (album1), ==, gdata_picasaweb_album_get_comment_count (album2));

		/* Compare contents */
		contents1 = gdata_picasaweb_album_get_contents (album1);
		contents2 = gdata_picasaweb_album_get_contents (album2);

		g_assert_cmpuint (g_list_length (contents1), ==, g_list_length (contents2));
		g_assert_cmpuint (g_list_length (contents1), >=, 1);

		for (i1 = contents1, i2 = contents2; i1 != NULL && i2 != NULL; i1 = i1->next, i2 = i2->next) {
			GDataMediaContent *content1, *content2;

			content1 = GDATA_MEDIA_CONTENT (i1->data);
			content2 = GDATA_MEDIA_CONTENT (i2->data);

			g_assert_cmpstr (gdata_media_content_get_uri (content1), ==, gdata_media_content_get_uri (content2));
			g_assert (strstr (gdata_media_content_get_uri (content1), "googleusercontent.com") != NULL);
			g_assert_cmpstr (gdata_media_content_get_content_type (content1), ==, gdata_media_content_get_content_type (content2));
			g_assert_cmpstr (gdata_media_content_get_content_type (content1), ==, "image/jpeg");
			g_assert_cmpuint (gdata_media_content_get_medium (content1), ==, gdata_media_content_get_medium (content2));
			g_assert_cmpuint (gdata_media_content_get_medium (content1), ==, GDATA_MEDIA_IMAGE);

			g_assert (gdata_media_content_is_default (content1) == gdata_media_content_is_default (content2));
			g_assert (gdata_media_content_is_default (content1) == FALSE);
			g_assert_cmpint (gdata_media_content_get_duration (content1), ==, gdata_media_content_get_duration (content2));
			g_assert_cmpint (gdata_media_content_get_duration (content1), ==, 0); /* doesn't apply to photos */
			g_assert_cmpuint (gdata_media_content_get_width (content1), ==, gdata_media_content_get_width (content2));
			g_assert_cmpuint (gdata_media_content_get_width (content1), ==, 0); /* PicasaWeb doesn't set anything better */
			g_assert_cmpuint (gdata_media_content_get_height (content1), ==, gdata_media_content_get_height (content2));
			g_assert_cmpuint (gdata_media_content_get_height (content1), ==, 0); /* PicasaWeb doesn't set anything better */
			g_assert_cmpuint (gdata_media_content_get_filesize (content1), ==, gdata_media_content_get_filesize (content2));
			g_assert_cmpuint (gdata_media_content_get_filesize (content1), ==, 0); /* PicasaWeb doesn't set anything better */
			g_assert_cmpuint (gdata_media_content_get_expression (content1), ==, gdata_media_content_get_expression (content2));
			g_assert_cmpuint (gdata_media_content_get_expression (content1), ==, GDATA_MEDIA_EXPRESSION_FULL);
		}

		g_assert (i1 == NULL && i2 == NULL);

		/* Compare thumbnails */
		thumbnails1 = gdata_picasaweb_album_get_thumbnails (album1);
		thumbnails2 = gdata_picasaweb_album_get_thumbnails (album2);

		g_assert_cmpuint (g_list_length (thumbnails1), ==, g_list_length (thumbnails2));
		g_assert_cmpuint (g_list_length (thumbnails1), >=, 1);

		for (i1 = thumbnails1, i2 = thumbnails2; i1 != NULL && i2 != NULL; i1 = i1->next, i2 = i2->next) {
			GDataMediaThumbnail *thumbnail1, *thumbnail2;

			thumbnail1 = GDATA_MEDIA_THUMBNAIL (i1->data);
			thumbnail2 = GDATA_MEDIA_THUMBNAIL (i2->data);

			g_assert_cmpstr (gdata_media_thumbnail_get_uri (thumbnail1), ==, gdata_media_thumbnail_get_uri (thumbnail2));
			g_assert (strstr (gdata_media_thumbnail_get_uri (thumbnail1), "googleusercontent.com") != NULL);
			g_assert_cmpint (gdata_media_thumbnail_get_time (thumbnail1), ==, gdata_media_thumbnail_get_time (thumbnail2));
			g_assert_cmpint (gdata_media_thumbnail_get_time (thumbnail1), ==, -1); /* PicasaWeb doesn't set anything better */
			g_assert_cmpint (gdata_media_thumbnail_get_width (thumbnail1), ==, gdata_media_thumbnail_get_width (thumbnail2));
			g_assert_cmpint (gdata_media_thumbnail_get_width (thumbnail1), ==, 160);
			g_assert_cmpint (gdata_media_thumbnail_get_height (thumbnail1), ==, gdata_media_thumbnail_get_height (thumbnail2));
			g_assert_cmpint (gdata_media_thumbnail_get_height (thumbnail1), ==, 160);
		}

		g_assert (i1 == NULL && i2 == NULL);
	}

	/* TODO: We don't compare categories or authors yet. */
}

/* Assert that two files have equal properties, but aren't the same object instance. For use in, e.g., comparing an inserted file from the server
 * to the original instance which was inserted. */
static void
assert_files_equal (GDataPicasaWebFile *file1, GDataPicasaWebFile *file2, gboolean compare_inserted_data)
{
	gdouble latitude1, longitude1, latitude2, longitude2;
	const gchar * const *tags1, * const *tags2;

	g_assert (GDATA_IS_PICASAWEB_FILE (file1));
	g_assert (GDATA_IS_PICASAWEB_FILE (file2));

	g_assert (file1 != file2);

	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (file1)), ==, gdata_entry_get_title (GDATA_ENTRY (file2)));
	g_assert_cmpstr (gdata_entry_get_summary (GDATA_ENTRY (file1)), ==, gdata_entry_get_summary (GDATA_ENTRY (file2)));
	g_assert_cmpstr (gdata_entry_get_content (GDATA_ENTRY (file1)), ==, gdata_entry_get_content (GDATA_ENTRY (file2)));
	g_assert_cmpstr (gdata_entry_get_content (GDATA_ENTRY (file1)), ==, NULL);
	g_assert_cmpstr (gdata_entry_get_content_uri (GDATA_ENTRY (file1)), ==, gdata_entry_get_content_uri (GDATA_ENTRY (file2)));
	g_assert (strstr (gdata_entry_get_content_uri (GDATA_ENTRY (file1)), "googleusercontent.com") != NULL);
	g_assert_cmpstr (gdata_entry_get_rights (GDATA_ENTRY (file1)), ==, gdata_entry_get_rights (GDATA_ENTRY (file2)));

	if (compare_inserted_data == TRUE) {
		g_assert_cmpstr (gdata_entry_get_id (GDATA_ENTRY (file1)), ==, gdata_entry_get_id (GDATA_ENTRY (file2)));
		g_assert_cmpstr (gdata_entry_get_id (GDATA_ENTRY (file1)), !=, NULL);
		g_assert_cmpstr (gdata_entry_get_etag (GDATA_ENTRY (file1)), ==, gdata_entry_get_etag (GDATA_ENTRY (file2)));
		g_assert_cmpstr (gdata_entry_get_etag (GDATA_ENTRY (file1)), !=, NULL);
		g_assert_cmpint (gdata_entry_get_updated (GDATA_ENTRY (file1)), ==, gdata_entry_get_updated (GDATA_ENTRY (file2)));
		g_assert_cmpint (gdata_entry_get_updated (GDATA_ENTRY (file1)), >, 0);
		g_assert_cmpint (gdata_entry_get_published (GDATA_ENTRY (file1)), ==, gdata_entry_get_published (GDATA_ENTRY (file2)));
		g_assert_cmpint (gdata_entry_get_published (GDATA_ENTRY (file1)), >, 0);
	}

	g_assert_cmpstr (gdata_picasaweb_file_get_id (file1), ==, gdata_picasaweb_file_get_id (file2));
	g_assert_cmpint (strlen (gdata_picasaweb_file_get_id (file1)), >, 0);
	g_assert_cmpstr (gdata_picasaweb_file_get_checksum (file1), ==, gdata_picasaweb_file_get_checksum (file2));
	g_assert (gdata_picasaweb_file_is_commenting_enabled (file1) == gdata_picasaweb_file_is_commenting_enabled (file2));
	g_assert_cmpstr (gdata_picasaweb_file_get_credit (file1), ==, gdata_picasaweb_file_get_credit (file2));
	g_assert_cmpstr (gdata_picasaweb_file_get_caption (file1), ==, gdata_picasaweb_file_get_caption (file2));

	/* Compare coordinates */
	gdata_picasaweb_file_get_coordinates (file1, &latitude1, &longitude1);
	gdata_picasaweb_file_get_coordinates (file2, &latitude2, &longitude2);
	g_assert_cmpfloat (latitude1, ==, latitude2);
	g_assert_cmpfloat (longitude1, ==, longitude2);

	/* Compare tags */
	tags1 = gdata_picasaweb_file_get_tags (file1);
	tags2 = gdata_picasaweb_file_get_tags (file2);

	g_assert ((tags1 == NULL) == (tags2 == NULL));

	if (tags1 != NULL && tags2 != NULL) {
		guint i;

		for (i = 0; tags1[i] != NULL; i++) {
			g_assert_cmpstr (tags1[i], ==, tags2[i]);
		}

		g_assert (tags2[i] == NULL);
	}

	if (compare_inserted_data == TRUE) {
		GList *contents1, *contents2, *thumbnails1, *thumbnails2, *i1, *i2;

		g_assert_cmpint (gdata_picasaweb_file_get_edited (file1), ==, gdata_picasaweb_file_get_edited (file2));
		g_assert_cmpint (gdata_picasaweb_file_get_edited (file1), >, 0);
		g_assert_cmpstr (gdata_picasaweb_file_get_version (file1), ==, gdata_picasaweb_file_get_version (file2));
		g_assert_cmpuint (strlen (gdata_picasaweb_file_get_version (file1)), >, 0);
		g_assert_cmpstr (gdata_picasaweb_file_get_album_id (file1), ==, gdata_picasaweb_file_get_album_id (file2));
		g_assert_cmpuint (strlen (gdata_picasaweb_file_get_album_id (file1)), >, 0);
		g_assert_cmpuint (gdata_picasaweb_file_get_width (file1), ==, gdata_picasaweb_file_get_width (file2));
		g_assert_cmpuint (gdata_picasaweb_file_get_width (file1), >, 0);
		g_assert_cmpuint (gdata_picasaweb_file_get_height (file1), ==, gdata_picasaweb_file_get_height (file2));
		g_assert_cmpuint (gdata_picasaweb_file_get_height (file1), >, 0);
		g_assert_cmpuint (gdata_picasaweb_file_get_size (file1), ==, gdata_picasaweb_file_get_size (file2));
		g_assert_cmpuint (gdata_picasaweb_file_get_size (file1), >, 0);
		g_assert_cmpint (gdata_picasaweb_file_get_timestamp (file1), ==, gdata_picasaweb_file_get_timestamp (file2));
		g_assert_cmpint (gdata_picasaweb_file_get_timestamp (file1), >, 0);
		g_assert_cmpuint (gdata_picasaweb_file_get_comment_count (file1), ==, gdata_picasaweb_file_get_comment_count (file2));
		g_assert_cmpuint (gdata_picasaweb_file_get_rotation (file1), ==, gdata_picasaweb_file_get_rotation (file2));
		g_assert_cmpstr (gdata_picasaweb_file_get_video_status (file1), ==, gdata_picasaweb_file_get_video_status (file2));

		/* Compare contents */
		contents1 = gdata_picasaweb_file_get_contents (file1);
		contents2 = gdata_picasaweb_file_get_contents (file2);

		g_assert_cmpuint (g_list_length (contents1), ==, g_list_length (contents2));
		g_assert_cmpuint (g_list_length (contents1), >=, 1);

		for (i1 = contents1, i2 = contents2; i1 != NULL && i2 != NULL; i1 = i1->next, i2 = i2->next) {
			GDataMediaContent *content1, *content2;

			content1 = GDATA_MEDIA_CONTENT (i1->data);
			content2 = GDATA_MEDIA_CONTENT (i2->data);

			g_assert_cmpstr (gdata_media_content_get_uri (content1), ==, gdata_media_content_get_uri (content2));
			g_assert (strstr (gdata_media_content_get_uri (content1), "googleusercontent.com") != NULL);
			g_assert_cmpstr (gdata_media_content_get_content_type (content1), ==, gdata_media_content_get_content_type (content2));
			g_assert_cmpstr (gdata_media_content_get_content_type (content1), ==, "image/jpeg");
			g_assert_cmpuint (gdata_media_content_get_medium (content1), ==, gdata_media_content_get_medium (content2));
			g_assert_cmpuint (gdata_media_content_get_medium (content1), ==, GDATA_MEDIA_IMAGE);

			g_assert (gdata_media_content_is_default (content1) == gdata_media_content_is_default (content2));
			g_assert (gdata_media_content_is_default (content1) == FALSE);
			g_assert_cmpint (gdata_media_content_get_duration (content1), ==, gdata_media_content_get_duration (content2));
			g_assert_cmpint (gdata_media_content_get_duration (content1), ==, 0); /* doesn't apply to photos */
			g_assert_cmpuint (gdata_media_content_get_width (content1), ==, gdata_media_content_get_width (content2));
			g_assert_cmpuint (gdata_media_content_get_width (content1), >, 0);
			g_assert_cmpuint (gdata_media_content_get_height (content1), ==, gdata_media_content_get_height (content2));
			g_assert_cmpuint (gdata_media_content_get_height (content1), >, 0);
			g_assert_cmpuint (gdata_media_content_get_filesize (content1), ==, gdata_media_content_get_filesize (content2));
			g_assert_cmpuint (gdata_media_content_get_filesize (content1), ==, 0); /* PicasaWeb doesn't set anything better */
			g_assert_cmpuint (gdata_media_content_get_expression (content1), ==, gdata_media_content_get_expression (content2));
			g_assert_cmpuint (gdata_media_content_get_expression (content1), ==, GDATA_MEDIA_EXPRESSION_FULL);

			/* TODO: really want to test these with a video clip */
		}

		g_assert (i1 == NULL && i2 == NULL);

		/* Compare thumbnails */
		thumbnails1 = gdata_picasaweb_file_get_thumbnails (file1);
		thumbnails2 = gdata_picasaweb_file_get_thumbnails (file2);

		g_assert_cmpuint (g_list_length (thumbnails1), ==, g_list_length (thumbnails2));
		g_assert_cmpuint (g_list_length (thumbnails1), >=, 1);

		for (i1 = thumbnails1, i2 = thumbnails2; i1 != NULL && i2 != NULL; i1 = i1->next, i2 = i2->next) {
			GDataMediaThumbnail *thumbnail1, *thumbnail2;

			thumbnail1 = GDATA_MEDIA_THUMBNAIL (i1->data);
			thumbnail2 = GDATA_MEDIA_THUMBNAIL (i2->data);

			g_assert_cmpstr (gdata_media_thumbnail_get_uri (thumbnail1), ==, gdata_media_thumbnail_get_uri (thumbnail2));
			g_assert (strstr (gdata_media_thumbnail_get_uri (thumbnail1), "googleusercontent.com") != NULL);
			g_assert_cmpint (gdata_media_thumbnail_get_time (thumbnail1), ==, gdata_media_thumbnail_get_time (thumbnail2));
			g_assert_cmpint (gdata_media_thumbnail_get_time (thumbnail1), ==, -1); /* PicasaWeb doesn't set anything better */
			g_assert_cmpint (gdata_media_thumbnail_get_width (thumbnail1), ==, gdata_media_thumbnail_get_width (thumbnail2));
			g_assert_cmpint (gdata_media_thumbnail_get_width (thumbnail1), >, 0);
			g_assert_cmpint (gdata_media_thumbnail_get_height (thumbnail1), ==, gdata_media_thumbnail_get_height (thumbnail2));
			g_assert_cmpint (gdata_media_thumbnail_get_height (thumbnail1), >, 0);
		}

		g_assert (i1 == NULL && i2 == NULL);

		/* Check EXIF values */
		g_assert_cmpfloat (gdata_picasaweb_file_get_distance (file1), ==, gdata_picasaweb_file_get_distance (file2));
		g_assert_cmpfloat (gdata_picasaweb_file_get_exposure (file1), ==, gdata_picasaweb_file_get_exposure (file2));
		g_assert_cmpfloat (gdata_picasaweb_file_get_exposure (file1), >, 0.0);
		g_assert (gdata_picasaweb_file_get_flash (file1) == gdata_picasaweb_file_get_flash (file2));
		g_assert_cmpfloat (gdata_picasaweb_file_get_focal_length (file1), ==, gdata_picasaweb_file_get_focal_length (file2));
		g_assert_cmpfloat (gdata_picasaweb_file_get_focal_length (file1), >, 0.0);
		g_assert_cmpfloat (gdata_picasaweb_file_get_fstop (file1), ==, gdata_picasaweb_file_get_fstop (file2));
		g_assert_cmpfloat (gdata_picasaweb_file_get_fstop (file1), >, 0.0);
		g_assert_cmpstr (gdata_picasaweb_file_get_image_unique_id (file1), ==, gdata_picasaweb_file_get_image_unique_id (file2));
		g_assert_cmpuint (strlen (gdata_picasaweb_file_get_image_unique_id (file1)), >, 0);
		g_assert_cmpint (gdata_picasaweb_file_get_iso (file1), ==, gdata_picasaweb_file_get_iso (file2));
		g_assert_cmpint (gdata_picasaweb_file_get_iso (file1), >, 0);
		g_assert_cmpstr (gdata_picasaweb_file_get_make (file1), ==, gdata_picasaweb_file_get_make (file2));
		g_assert_cmpuint (strlen (gdata_picasaweb_file_get_make (file1)), >, 0);
		g_assert_cmpstr (gdata_picasaweb_file_get_model (file1), ==, gdata_picasaweb_file_get_model (file2));
		g_assert_cmpuint (strlen (gdata_picasaweb_file_get_model (file1)), >, 0);
	}

	/* TODO: file wasn't uploaded with checksum assigned; g_assert_cmpstr (gdata_picasaweb_file_get_checksum (photo), ==, ??); */
	/* TODO: not a good test of video status; want to upload a video for it */
}

static void
test_authentication (void)
{
	gboolean retval;
	GDataClientLoginAuthorizer *authorizer;
	GError *error = NULL;

	/* Create an authorizer */
	authorizer = gdata_client_login_authorizer_new (CLIENT_ID, GDATA_TYPE_PICASAWEB_SERVICE);

	g_assert_cmpstr (gdata_client_login_authorizer_get_client_id (authorizer), ==, CLIENT_ID);

	/* Log in */
	retval = gdata_client_login_authorizer_authenticate (authorizer, PW_USERNAME, PASSWORD, NULL, &error);
	g_assert_no_error (error);
	g_assert (retval == TRUE);
	g_clear_error (&error);

	/* Check all is as it should be */
	g_assert_cmpstr (gdata_client_login_authorizer_get_username (authorizer), ==, PW_USERNAME);
	g_assert_cmpstr (gdata_client_login_authorizer_get_password (authorizer), ==, PASSWORD);

	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	                                                     gdata_picasaweb_service_get_primary_authorization_domain ()) == TRUE);

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
	g_assert_cmpstr (gdata_client_login_authorizer_get_username (authorizer), ==, PW_USERNAME);
	g_assert_cmpstr (gdata_client_login_authorizer_get_password (authorizer), ==, PASSWORD);

	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	                                                     gdata_picasaweb_service_get_primary_authorization_domain ()) == TRUE);
}

static void
test_authentication_async (void)
{
	GMainLoop *main_loop;
	GDataClientLoginAuthorizer *authorizer;

	/* Create an authorizer */
	authorizer = gdata_client_login_authorizer_new (CLIENT_ID, GDATA_TYPE_PICASAWEB_SERVICE);

	g_assert_cmpstr (gdata_client_login_authorizer_get_client_id (authorizer), ==, CLIENT_ID);

	main_loop = g_main_loop_new (NULL, TRUE);
	gdata_client_login_authorizer_authenticate_async (authorizer, PW_USERNAME, PASSWORD, NULL,
	                                                  (GAsyncReadyCallback) test_authentication_async_cb, main_loop);

	g_main_loop_run (main_loop);

	g_main_loop_unref (main_loop);
	g_object_unref (authorizer);
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
	GDataMediaThumbnail *thumbnail;
	GDataDownloadStream *download_stream;
	gchar *destination_file_name, *destination_file_path;
	GFile *destination_file;
	GFileOutputStream *file_stream;
	gssize transfer_size;
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
	thumbnails = gdata_picasaweb_file_get_thumbnails (photo);
	thumbnail = GDATA_MEDIA_THUMBNAIL (thumbnails->data);

	/* Download a single thumbnail to a file for testing (in case we weren't compiled with GdkPixbuf support) */
	download_stream = gdata_media_thumbnail_download (thumbnail, service, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOWNLOAD_STREAM (download_stream));

	/* Prepare a file to write the data to */
	destination_file_name = g_strdup_printf ("%s_thumbnail_%ux%u.jpg", gdata_picasaweb_file_get_id (photo),
	                                         gdata_media_thumbnail_get_width (thumbnail), gdata_media_thumbnail_get_height (thumbnail));
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
	for (node = thumbnails; node != NULL; node = node->next) {
		GdkPixbuf *pixbuf;

		thumbnail = GDATA_MEDIA_THUMBNAIL (node->data);

		/* Prepare a download stream */
		download_stream = gdata_media_thumbnail_download (thumbnail, service, NULL, &error);
		g_assert_no_error (error);
		g_assert (GDATA_IS_DOWNLOAD_STREAM (download_stream));

		/* Download into a new GdkPixbuf */
		pixbuf = gdk_pixbuf_new_from_stream (G_INPUT_STREAM (download_stream), NULL, &error);
		g_assert_no_error (error);
		g_assert (GDK_IS_PIXBUF (pixbuf));

		g_object_unref (download_stream);

		/* PicasaWeb reported the height of a thumbnail as a pixel too large once, but otherwise correct */
		g_assert_cmpint (abs (gdk_pixbuf_get_width (pixbuf) - (gint) gdata_media_thumbnail_get_width (thumbnail)) , <=, 1);
		g_assert_cmpint (abs (gdk_pixbuf_get_height (pixbuf) - (gint) gdata_media_thumbnail_get_height (thumbnail)) , <=, 1);

		g_object_unref (pixbuf);
	}
#endif /* HAVE_GDK_PIXBUF */

	g_object_unref (photo_feed);
	g_object_unref (album_feed);
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
	GDataMediaContent *content;
	GDataDownloadStream *download_stream;
	gchar *destination_file_name, *destination_file_path;
	GFile *destination_file;
	GFileOutputStream *file_stream;
	gssize transfer_size;
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
	media_contents = gdata_picasaweb_file_get_contents (photo);
	g_assert_cmpint (g_list_length (media_contents), ==, 1);
	content = GDATA_MEDIA_CONTENT (media_contents->data);

	/* Prepare a download stream */
	download_stream = gdata_media_content_download (content, service, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_DOWNLOAD_STREAM (download_stream));

	/* Prepare a file to write the data to */
	destination_file_name = g_strdup_printf ("%s.jpg", gdata_picasaweb_file_get_id (photo));
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

	g_object_unref (photo_feed);
	g_object_unref (album_feed);
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

	g_assert_cmpint (gdata_picasaweb_file_get_edited (photo), ==, 1273783513);

	/* tests */

	g_assert_cmpstr (gdata_picasaweb_file_get_caption (photo), ==, "Ginger cookie caption");
	g_assert_cmpstr (gdata_picasaweb_file_get_version (photo), ==, "29"); /* 1240729023474000"); */ /* TODO check how constant this even is */
	g_assert_cmpstr (gdata_picasaweb_file_get_album_id (photo), ==, "5328889949261497249");
	g_assert_cmpuint (gdata_picasaweb_file_get_width (photo), ==, 2576);
	g_assert_cmpuint (gdata_picasaweb_file_get_height (photo), ==, 1932);
	g_assert_cmpuint (gdata_picasaweb_file_get_size (photo), ==, 1124730);
	/* TODO: file wasn't uploaded with checksum assigned; g_assert_cmpstr (gdata_picasaweb_file_get_checksum (photo), ==, ??); */
	g_assert_cmpint (gdata_picasaweb_file_get_timestamp (photo), ==, 1228588330000);
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
	                 "https://lh3.googleusercontent.com/--1R6jzZZ1oI/SfQFWPnuovI/AAAAAAAAAB0/WdINsvmFPf8/100_0269.jpg");
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
	                 "https://lh3.googleusercontent.com/--1R6jzZZ1oI/SfQFWPnuovI/AAAAAAAAAB0/WdINsvmFPf8/s288/100_0269.jpg");
	g_assert_cmpuint (gdata_media_thumbnail_get_width (thumbnail), ==, 288);
	g_assert_cmpuint (gdata_media_thumbnail_get_height (thumbnail), ==, 216);
	g_assert_cmpint (gdata_media_thumbnail_get_time (thumbnail), ==, -1); /* PicasaWeb doesn't set anything better */

	g_object_unref (album_feed);
	g_object_unref (photo_feed);
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
	gchar *xml;
	GDataEntry *photo_entry;

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
	g_assert_cmpstr (gdata_entry_get_id (photo_entry), ==,
	                 "https://picasaweb.google.com/data/entry/user/libgdata.picasaweb/albumid/5328889949261497249/photoid/5328890138794566386");
	g_assert_cmpstr (gdata_entry_get_etag (photo_entry), !=, NULL);
	g_assert_cmpint (gdata_entry_get_updated (photo_entry), ==, 1273783513);
	g_assert_cmpint (gdata_entry_get_published (photo_entry), ==, 1240728920);
	g_assert (gdata_entry_get_content (photo_entry) == NULL);
	g_assert_cmpstr (gdata_entry_get_content_uri (photo_entry), ==,
	                 "https://lh3.googleusercontent.com/--1R6jzZZ1oI/SfQFWPnuovI/AAAAAAAAAB0/WdINsvmFPf8/100_0269.jpg");

	xml = gdata_parsable_get_xml (GDATA_PARSABLE (photo_entry));
	g_assert_cmpstr (xml, !=, NULL);
	g_assert_cmpuint (strlen (xml), >, 0);
	g_free (xml);

	g_object_unref (album_feed);
	g_object_unref (photo_feed);
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
			 "https://picasaweb.google.com/data/feed/user/libgdata.picasaweb/albumid/5328889949261497249");
	g_assert_cmpstr (gdata_feed_get_etag (photo_feed), !=, NULL);
	g_assert_cmpuint (gdata_feed_get_items_per_page (photo_feed), ==, 1000);
	g_assert_cmpuint (gdata_feed_get_start_index (photo_feed), ==, 1);
	g_assert_cmpuint (gdata_feed_get_total_results (photo_feed), ==, 1);

	g_object_unref (album_feed);
	g_object_unref (photo_feed);
}

static void
test_photo_single (gconstpointer service)
{
	GDataEntry *photo;
	GError *error = NULL;

	const gchar *entry_id =
		"https://picasaweb.google.com/data/entry/user/libgdata.picasaweb/albumid/5328889949261497249/photoid/5328890138794566386";
	photo = gdata_service_query_single_entry (GDATA_SERVICE (service), gdata_picasaweb_service_get_primary_authorization_domain (),
	                                          entry_id, NULL, GDATA_TYPE_PICASAWEB_FILE, NULL, &error);

	g_assert_no_error (error);
	g_assert (photo != NULL);
	g_assert (GDATA_IS_PICASAWEB_FILE (photo));
	g_assert_cmpstr (gdata_picasaweb_file_get_id (GDATA_PICASAWEB_FILE (photo)), ==, "5328890138794566386");
	g_assert_cmpstr (gdata_entry_get_id (photo), ==,
	                 "https://picasaweb.google.com/data/entry/user/libgdata.picasaweb/albumid/5328889949261497249/photoid/5328890138794566386");
	g_clear_error (&error);

	g_object_unref (photo);
}

static void
test_photo_async_cb (GDataService *service, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GDataFeed *feed;
	GError *error = NULL;

	feed = gdata_service_query_finish (service, async_result, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* Tests */
	g_assert_cmpstr (gdata_feed_get_title (feed), ==, "Test Album 1 - Venice - Public");
	g_assert_cmpstr (gdata_feed_get_id (feed), ==, "https://picasaweb.google.com/data/feed/user/libgdata.picasaweb/albumid/5328889949261497249");
	g_assert_cmpstr (gdata_feed_get_etag (feed), !=, NULL);
	g_assert_cmpuint (gdata_feed_get_items_per_page (feed), ==, 1000);
	g_assert_cmpuint (gdata_feed_get_start_index (feed), ==, 1);
	g_assert_cmpuint (gdata_feed_get_total_results (feed), ==, 1);

	g_main_loop_quit (main_loop);

	g_object_unref (feed);
}

static void
test_photo_async (gconstpointer service)
{
	GMainLoop *main_loop;
	GDataFeed *album_feed;
	GDataEntry *entry;
	GDataPicasaWebAlbum *album;
	GList *albums;
	GError *error = NULL;

	/* Find an album */
	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (album_feed));
	g_clear_error (&error);

	albums = gdata_feed_get_entries (album_feed);
	entry = GDATA_ENTRY (g_list_nth_data (albums, TEST_ALBUM_INDEX));
	album = GDATA_PICASAWEB_ALBUM (entry);

	main_loop = g_main_loop_new (NULL, TRUE);

	gdata_picasaweb_service_query_files_async (GDATA_PICASAWEB_SERVICE (service), album, NULL, NULL, NULL, NULL, NULL,
	                                           (GAsyncReadyCallback) test_photo_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
	g_object_unref (album_feed);
}

static void
test_photo_async_progress_closure (gconstpointer service)
{
	GDataAsyncProgressClosure *data = g_slice_new0 (GDataAsyncProgressClosure);
	GDataFeed *album_feed;
	GDataEntry *entry;
	GDataPicasaWebAlbum *album;
	GList *albums;
	GError *error = NULL;

	g_assert (service != NULL);

	/* Find an album */
	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (album_feed));
	g_clear_error (&error);

	albums = gdata_feed_get_entries (album_feed);
	entry = GDATA_ENTRY (g_list_nth_data (albums, TEST_ALBUM_INDEX));
	album = GDATA_PICASAWEB_ALBUM (entry);

	data->main_loop = g_main_loop_new (NULL, TRUE);

	gdata_picasaweb_service_query_files_async (GDATA_PICASAWEB_SERVICE (service), album, NULL, NULL,
	                                           (GDataQueryProgressCallback) gdata_test_async_progress_callback,
	                                           data, (GDestroyNotify) gdata_test_async_progress_closure_free,
	                                           (GAsyncReadyCallback) gdata_test_async_progress_finish_callback, data);
	g_main_loop_run (data->main_loop);
	g_main_loop_unref (data->main_loop);
	g_object_unref (album_feed);

	/* Check that both callbacks were called exactly once */
	g_assert_cmpuint (data->progress_destroy_notify_count, ==, 1);
	g_assert_cmpuint (data->async_ready_notify_count, ==, 1);

	g_slice_free (GDataAsyncProgressClosure, data);
}

typedef struct {
	GDataPicasaWebAlbum *album;
	GDataPicasaWebAlbum *inserted_album;
} InsertAlbumData;

static void
set_up_insert_album (InsertAlbumData *data, gconstpointer service)
{
	GTimeVal timestamp;

	data->album = gdata_picasaweb_album_new (NULL);
	g_assert (GDATA_IS_PICASAWEB_ALBUM (data->album));

	gdata_entry_set_title (GDATA_ENTRY (data->album), "Thanksgiving photos");
	gdata_entry_set_summary (GDATA_ENTRY (data->album), "Family photos of the feast!");
	gdata_picasaweb_album_set_location (data->album, "Winnipeg, MN");

	g_time_val_from_iso8601 ("2002-10-14T09:58:59.643554Z", &timestamp);
	gdata_picasaweb_album_set_timestamp (data->album, timestamp.tv_sec * 1000);
}

static void
tear_down_insert_album (InsertAlbumData *data, gconstpointer service)
{
	g_object_unref (data->album);

	/* Clean up the evidence */
	gdata_service_delete_entry (GDATA_SERVICE (service), gdata_picasaweb_service_get_primary_authorization_domain (),
	                            GDATA_ENTRY (data->inserted_album), NULL, NULL);
}

static void
test_insert_album (InsertAlbumData *data, gconstpointer service)
{
	GDataPicasaWebAlbum *inserted_album;
	GDataFeed *album_feed;
	GError *error = NULL;

	/* Insert the album synchronously */
	inserted_album = gdata_picasaweb_service_insert_album (GDATA_PICASAWEB_SERVICE (service), data->album, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_PICASAWEB_ALBUM (inserted_album));
	g_clear_error (&error);

	data->inserted_album = g_object_ref (inserted_album);

	/* Test that it returns what we gave */
	assert_albums_equal (inserted_album, data->album, FALSE);

	/* Test that the album is actually on the server */
	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);

	assert_albums_equal (GDATA_PICASAWEB_ALBUM (gdata_feed_look_up_entry (album_feed, gdata_entry_get_id (GDATA_ENTRY (inserted_album)))),
	                     inserted_album, TRUE);

	g_object_unref (album_feed);

	g_object_unref (inserted_album);
}

typedef struct {
	InsertAlbumData data;
	GMainLoop *main_loop;
} InsertAlbumAsyncData;

static void
set_up_insert_album_async (InsertAlbumAsyncData *data, gconstpointer service)
{
	set_up_insert_album ((InsertAlbumData*) data, service);
	data->main_loop = g_main_loop_new (NULL, TRUE);
}

static void
tear_down_insert_album_async (InsertAlbumAsyncData *data, gconstpointer service)
{
	g_main_loop_unref (data->main_loop);
	tear_down_insert_album ((InsertAlbumData*) data, service);
}

static void
test_insert_album_async_cb (GDataService *service, GAsyncResult *async_result, InsertAlbumAsyncData *data)
{
	GDataEntry *entry;
	GError *error = NULL;

	entry = gdata_service_insert_entry_finish (service, async_result, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_PICASAWEB_ALBUM (entry));
	g_clear_error (&error);

	data->data.inserted_album = g_object_ref (entry);

	/* Test the album was uploaded correctly */
	assert_albums_equal (GDATA_PICASAWEB_ALBUM (entry), data->data.album, FALSE);

	g_object_unref (entry);

	g_main_loop_quit (data->main_loop);
}

static void
test_insert_album_async (InsertAlbumAsyncData *data, gconstpointer service)
{
	gdata_picasaweb_service_insert_album_async (GDATA_PICASAWEB_SERVICE (service), data->data.album, NULL,
	                                            (GAsyncReadyCallback) test_insert_album_async_cb, data);

	g_main_loop_run (data->main_loop);
}

typedef struct {
	GDataPicasaWebAlbum *album1;
	GDataPicasaWebAlbum *album2;
	GDataPicasaWebAlbum *album3;
	GDataPicasaWebAlbum *album4;
} QueryAllAlbumsData;

static void
set_up_query_all_albums (QueryAllAlbumsData *data, gconstpointer service)
{
	GDataPicasaWebAlbum *album;

	/* First album */
	album = gdata_picasaweb_album_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (album), "Test album 1 for QueryAllAlbums");

	data->album1 = gdata_picasaweb_service_insert_album (GDATA_PICASAWEB_SERVICE (service), album, NULL, NULL);
	g_assert (data->album1 != NULL);

	g_object_unref (album);

	/* Second album */
	album = gdata_picasaweb_album_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (album), "Test album 2 for QueryAllAlbums");

	data->album2 = gdata_picasaweb_service_insert_album (GDATA_PICASAWEB_SERVICE (service), album, NULL, NULL);
	g_assert (data->album2 != NULL);

	g_object_unref (album);

	/* Third album */
	album = gdata_picasaweb_album_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (album), "Test album 3 for QueryAllAlbums");

	data->album3 = gdata_picasaweb_service_insert_album (GDATA_PICASAWEB_SERVICE (service), album, NULL, NULL);
	g_assert (data->album3 != NULL);

	g_object_unref (album);

	/* Fourth album */
	album = gdata_picasaweb_album_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (album), "Test album 4 for QueryAllAlbums");

	data->album4 = gdata_picasaweb_service_insert_album (GDATA_PICASAWEB_SERVICE (service), album, NULL, NULL);
	g_assert (data->album4 != NULL);

	g_object_unref (album);
}

static void
tear_down_query_all_albums (QueryAllAlbumsData *data, gconstpointer service)
{
	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_picasaweb_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->album1), NULL, NULL) == TRUE);
	g_object_unref (data->album1);

	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_picasaweb_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->album2), NULL, NULL) == TRUE);
	g_object_unref (data->album2);

	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_picasaweb_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->album3), NULL, NULL) == TRUE);
	g_object_unref (data->album3);

	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_picasaweb_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->album4), NULL, NULL) == TRUE);
	g_object_unref (data->album4);
}

static void
test_query_all_albums_bad_query (gconstpointer service)
{
	GDataQuery *query;
	GDataFeed *album_feed;
	GError *error = NULL;

	/* Test a query with a "q" parameter; it should fail */
	query = GDATA_QUERY (gdata_picasaweb_query_new ("foobar"));

	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), query, NULL, NULL, NULL, NULL, &error);
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_BAD_QUERY_PARAMETER);
	g_assert (album_feed == NULL);
	g_clear_error (&error);

	g_object_unref (query);
}

static void
test_query_all_albums_bad_query_with_limits (gconstpointer service)
{
	GDataQuery *query;
	GDataFeed *album_feed;
	GError *error = NULL;

	/* Test a query with a "q" parameter; it should fail */
	query = GDATA_QUERY (gdata_picasaweb_query_new_with_limits ("foobar", 1, 1));

	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), query, NULL, NULL, NULL, NULL, &error);
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_BAD_QUERY_PARAMETER);
	g_assert (album_feed == NULL);
	g_clear_error (&error);

	g_object_unref (query);
}

/* Checks to perform on an album feed from test_query_all_albums() or test_query_all_albums_async(). */
static void
_test_query_all_albums (GDataFeed *album_feed, QueryAllAlbumsData *data)
{
	GDataEntry *entry;
	gchar *xml;

	g_assert (GDATA_IS_FEED (album_feed));

	/* Check properties of the feed */
	g_assert_cmpint (g_list_length (gdata_feed_get_entries (album_feed)), >=, 4);

	g_assert_cmpstr (gdata_feed_get_title (album_feed), ==, "libgdata.picasaweb");
	g_assert_cmpstr (gdata_feed_get_subtitle (album_feed), ==, NULL);
	g_assert_cmpstr (gdata_feed_get_id (album_feed), ==, "https://picasaweb.google.com/data/feed/user/libgdata.picasaweb");
	g_assert_cmpstr (gdata_feed_get_etag (album_feed), !=, NULL); /* this varies as albums change, e.g. when new images are uploaded */
	g_assert_cmpstr (gdata_feed_get_icon (album_feed), !=, NULL); /* tested weakly because it changes fairly regularly */
	g_assert_cmpuint (gdata_feed_get_items_per_page (album_feed), ==, 1000);
	g_assert_cmpuint (gdata_feed_get_start_index (album_feed), ==, 1);
	g_assert_cmpuint (gdata_feed_get_total_results (album_feed), >=, 4);

	/* Test the first album */
	entry = gdata_feed_look_up_entry (album_feed, gdata_entry_get_id (GDATA_ENTRY (data->album1)));
	g_assert (entry != NULL);
	g_assert (GDATA_IS_PICASAWEB_ALBUM (entry));

	assert_albums_equal (GDATA_PICASAWEB_ALBUM (entry), data->album1, TRUE);

	xml = gdata_parsable_get_xml (GDATA_PARSABLE (entry));
	g_assert_cmpstr (xml, !=, NULL);
	g_assert_cmpuint (strlen (xml), >, 0);
	g_free (xml);
}

/* Test that synchronously querying for all albums lists them correctly. */
static void
test_query_all_albums (QueryAllAlbumsData *data, gconstpointer service)
{
	GDataFeed *album_feed;
	GError *error = NULL;

	/* Try a proper query */
	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	_test_query_all_albums (album_feed, data);

	g_object_unref (album_feed);
}

static void
test_query_all_albums_with_limits (QueryAllAlbumsData *data, gconstpointer service)
{
	GDataQuery *query;
	GDataFeed *album_feed_1, *album_feed_2;
	GError *error = NULL;
	GList *albums_1, *albums_2;

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

typedef struct {
	QueryAllAlbumsData data;
	GMainLoop *main_loop;
} QueryAllAlbumsAsyncData;

static void
set_up_query_all_albums_async (QueryAllAlbumsAsyncData *data, gconstpointer service)
{
	set_up_query_all_albums ((QueryAllAlbumsData*) data, service);
	data->main_loop = g_main_loop_new (NULL, TRUE);
}

static void
tear_down_query_all_albums_async (QueryAllAlbumsAsyncData *data, gconstpointer service)
{
	g_main_loop_unref (data->main_loop);
	tear_down_query_all_albums ((QueryAllAlbumsData*) data, service);
}

static void
test_query_all_albums_async_cb (GDataService *service, GAsyncResult *async_result, QueryAllAlbumsAsyncData *data)
{
	GDataFeed *album_feed;
	GError *error = NULL;

	/* Get the album feed */
	album_feed = gdata_service_query_finish (service, async_result, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	_test_query_all_albums (album_feed, (QueryAllAlbumsData*) data);

	g_object_unref (album_feed);

	g_main_loop_quit (data->main_loop);
}

/* Test that asynchronously querying for all albums lists them correctly. */
static void
test_query_all_albums_async (QueryAllAlbumsAsyncData *data, gconstpointer service)
{
	gdata_picasaweb_service_query_all_albums_async (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL,
	                                                NULL, NULL, (GAsyncReadyCallback) test_query_all_albums_async_cb, data);

	g_main_loop_run (data->main_loop);
}

/* Test that the progress callbacks from gdata_picasaweb_service_query_all_albums_async() are called correctly.
 * We take a QueryAllAlbumsAsyncData so that we can guarantee at least one album exists (since it's created in the setup function for
 * QueryAllAlbumsAsyncData), but we don't use it as we don't actually care about the specific album. */
static void
test_query_all_albums_async_progress_closure (QueryAllAlbumsAsyncData *unused_data, gconstpointer service)
{
	GDataAsyncProgressClosure *data = g_slice_new0 (GDataAsyncProgressClosure);

	data->main_loop = g_main_loop_new (NULL, TRUE);

	gdata_picasaweb_service_query_all_albums_async (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL,
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
	g_assert_cmpstr (gdata_picasaweb_user_get_thumbnail_uri (user), !=, NULL); /* tested weakly to avoid having to update it regularly */

	g_object_unref (user);
}

typedef struct {
	GDataPicasaWebService *service;
	GDataPicasaWebFile *photo;
	GDataPicasaWebFile *updated_photo;
	GFile *photo_file;
	gchar *slug;
	gchar *content_type;
	GFileInputStream *file_stream;
} UploadData;

static void
setup_upload (UploadData *data, gconstpointer service)
{
	GFileInfo *file_info;
	const gchar * const tags[] = { "foo", "bar", ",,baz,baz", NULL };
	GError *error = NULL;

	data->service = g_object_ref ((gpointer) service);

	/* Build the photo */
	data->photo = gdata_picasaweb_file_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (data->photo), "Photo Entry Title");
	gdata_picasaweb_file_set_caption (data->photo, "Photo Summary");
	gdata_picasaweb_file_set_tags (data->photo, tags);
	gdata_picasaweb_file_set_coordinates (data->photo, 17.127, -110.35);

	/* File is public domain: http://en.wikipedia.org/wiki/File:German_garden_gnome_cropped.jpg */
	data->photo_file = g_file_new_for_path (TEST_FILE_DIR "photo.jpg");

	/* Get the file's info */
	file_info = g_file_query_info (data->photo_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
	                               G_FILE_QUERY_INFO_NONE, NULL, &error);
	g_assert_no_error (error);
	g_assert (G_IS_FILE_INFO (file_info));

	data->slug = g_strdup (g_file_info_get_display_name (file_info));
	data->content_type = g_strdup (g_file_info_get_content_type (file_info));

	g_object_unref (file_info);

	/* Get an input stream for the file */
	data->file_stream = g_file_read (data->photo_file, NULL, &error);
	g_assert_no_error (error);
	g_assert (G_IS_FILE_INPUT_STREAM (data->file_stream));
}

static void
teardown_upload (UploadData *data, gconstpointer service)
{
	/* Delete the uploaded photo (don't worry if this fails) */
	if (data->updated_photo != NULL) {
		gdata_service_delete_entry (GDATA_SERVICE (service), gdata_picasaweb_service_get_primary_authorization_domain (),
		                            GDATA_ENTRY (data->updated_photo), NULL, NULL);
		g_object_unref (data->updated_photo);
	}

	g_object_unref (data->photo);
	g_object_unref (data->photo_file);
	g_free (data->slug);
	g_free (data->content_type);
	g_object_unref (data->file_stream);
	g_object_unref (data->service);
}

static void
test_upload_default_album (UploadData *data, gconstpointer service)
{
	GDataUploadStream *upload_stream;
	const gchar * const *tags, * const *tags2;
	gssize transfer_size;
	GError *error = NULL;

	/* Prepare the upload stream */
	/* TODO right now, it will just go to the default album, we want an uploading one :| */
	upload_stream = gdata_picasaweb_service_upload_file (GDATA_PICASAWEB_SERVICE (service), NULL, data->photo, data->slug, data->content_type,
	                                                     NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_UPLOAD_STREAM (upload_stream));

	/* Upload the photo */
	transfer_size = g_output_stream_splice (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (data->file_stream),
	                                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &error);
	g_assert_no_error (error);
	g_assert_cmpint (transfer_size, >, 0);

	/* Finish off the upload */
	data->updated_photo = gdata_picasaweb_service_finish_file_upload (GDATA_PICASAWEB_SERVICE (service), upload_stream, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_PICASAWEB_FILE (data->updated_photo));

	/* Check the photo's properties */
	g_assert (gdata_entry_is_inserted (GDATA_ENTRY (data->updated_photo)));
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (data->updated_photo)), ==, gdata_entry_get_title (GDATA_ENTRY (data->photo)));
	g_assert_cmpstr (gdata_picasaweb_file_get_caption (data->updated_photo), ==, gdata_picasaweb_file_get_caption (data->photo));

	tags = gdata_picasaweb_file_get_tags (data->photo);
	tags2 = gdata_picasaweb_file_get_tags (data->updated_photo);
	g_assert_cmpuint (g_strv_length ((gchar**) tags2), ==, g_strv_length ((gchar**) tags));
	g_assert_cmpstr (tags2[0], ==, tags[0]);
	g_assert_cmpstr (tags2[1], ==, tags[1]);
	g_assert_cmpstr (tags2[2], ==, tags[2]);
}

typedef struct {
	UploadData data;
	GMainLoop *main_loop;
} UploadAsyncData;

static void
setup_upload_async (UploadAsyncData *data, gconstpointer service)
{
	setup_upload ((UploadData*) data, service);
	data->main_loop = g_main_loop_new (NULL, TRUE);
}

static void
teardown_upload_async (UploadAsyncData *data, gconstpointer service)
{
	g_main_loop_unref (data->main_loop);
	teardown_upload ((UploadData*) data, service);
}

static void
test_upload_default_album_async_cb (GOutputStream *stream, GAsyncResult *result, UploadAsyncData *data)
{
	const gchar * const *tags, * const *tags2;
	gssize transfer_size;
	GError *error = NULL;

	/* Finish off the transfer */
	transfer_size = g_output_stream_splice_finish (stream, result, &error);
	g_assert_no_error (error);
	g_assert_cmpint (transfer_size, >, 0);

	/* Finish off the upload */
	data->data.updated_photo = gdata_picasaweb_service_finish_file_upload (data->data.service, GDATA_UPLOAD_STREAM (stream), &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_PICASAWEB_FILE (data->data.updated_photo));

	/* Check the photo's properties */
	g_assert (gdata_entry_is_inserted (GDATA_ENTRY (data->data.updated_photo)));
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (data->data.updated_photo)), ==, gdata_entry_get_title (GDATA_ENTRY (data->data.photo)));
	g_assert_cmpstr (gdata_picasaweb_file_get_caption (data->data.updated_photo), ==, gdata_picasaweb_file_get_caption (data->data.photo));

	tags = gdata_picasaweb_file_get_tags (data->data.photo);
	tags2 = gdata_picasaweb_file_get_tags (data->data.updated_photo);
	g_assert_cmpuint (g_strv_length ((gchar**) tags2), ==, g_strv_length ((gchar**) tags));
	g_assert_cmpstr (tags2[0], ==, tags[0]);
	g_assert_cmpstr (tags2[1], ==, tags[1]);
	g_assert_cmpstr (tags2[2], ==, tags[2]);

	g_main_loop_quit (data->main_loop);
}

static void
test_upload_default_album_async (UploadAsyncData *data, gconstpointer service)
{
	GDataUploadStream *upload_stream;
	GError *error = NULL;

	/* Prepare the upload stream */
	upload_stream = gdata_picasaweb_service_upload_file (GDATA_PICASAWEB_SERVICE (service), NULL, data->data.photo, data->data.slug,
	                                                     data->data.content_type, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_UPLOAD_STREAM (upload_stream));

	/* Upload the photo asynchronously */
	g_output_stream_splice_async (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (data->data.file_stream),
	                              G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, G_PRIORITY_DEFAULT, NULL,
	                              (GAsyncReadyCallback) test_upload_default_album_async_cb, data);
	g_main_loop_run (data->main_loop);

	g_object_unref (upload_stream);
}

static void
test_upload_default_album_cancellation_cb (GOutputStream *stream, GAsyncResult *result, UploadAsyncData *data)
{
	gssize transfer_size;
	GError *error = NULL;

	/* Finish off the transfer */
	transfer_size = g_output_stream_splice_finish (stream, result, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_assert_cmpint (transfer_size, ==, -1);
	g_clear_error (&error);

	/* Finish off the upload */
	data->data.updated_photo = gdata_picasaweb_service_finish_file_upload (data->data.service, GDATA_UPLOAD_STREAM (stream), &error);
	g_assert_no_error (error);
	g_assert (data->data.updated_photo == NULL);

	g_main_loop_quit (data->main_loop);
}

static gboolean
test_upload_default_album_cancellation_cancel_cb (GCancellable *cancellable)
{
	g_cancellable_cancel (cancellable);
	return FALSE;
}

static void
test_upload_default_album_cancellation (UploadAsyncData *data, gconstpointer service)
{
	GDataUploadStream *upload_stream;
	GCancellable *cancellable;
	GError *error = NULL;

	/* Create an idle function which will cancel the upload */
	cancellable = g_cancellable_new ();
	g_idle_add ((GSourceFunc) test_upload_default_album_cancellation_cancel_cb, cancellable);

	/* Prepare the upload stream */
	upload_stream = gdata_picasaweb_service_upload_file (GDATA_PICASAWEB_SERVICE (service), NULL, data->data.photo, data->data.slug,
	                                                     data->data.content_type, cancellable, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_UPLOAD_STREAM (upload_stream));

	/* Upload the photo asynchronously */
	g_output_stream_splice_async (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (data->data.file_stream),
	                              G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, G_PRIORITY_DEFAULT, NULL,
	                              (GAsyncReadyCallback) test_upload_default_album_cancellation_cb, data);
	g_main_loop_run (data->main_loop);

	g_object_unref (upload_stream);
	g_object_unref (cancellable);
}

static void
test_upload_default_album_cancellation2 (UploadAsyncData *data, gconstpointer service)
{
	GDataUploadStream *upload_stream;
	GCancellable *cancellable;
	GError *error = NULL;

	/* Create a timeout function which will cancel the upload after 1ms */
	cancellable = g_cancellable_new ();
	g_timeout_add (1, (GSourceFunc) test_upload_default_album_cancellation_cancel_cb, cancellable);

	/* Prepare the upload stream */
	upload_stream = gdata_picasaweb_service_upload_file (GDATA_PICASAWEB_SERVICE (service), NULL, data->data.photo, data->data.slug,
	                                                     data->data.content_type, cancellable, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_UPLOAD_STREAM (upload_stream));

	/* Upload the photo asynchronously */
	g_output_stream_splice_async (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (data->data.file_stream),
	                              G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, G_PRIORITY_DEFAULT, NULL,
	                              (GAsyncReadyCallback) test_upload_default_album_cancellation_cb, data);
	g_main_loop_run (data->main_loop);

	g_object_unref (upload_stream);
	g_object_unref (cancellable);
}

static void
test_album_new (void)
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

	/* Build a regex to match the timestamp from the XML, since we can't definitely say what it'll be. Note that we also assign any order to the
	 * namespace definitions, since due to a change in GLib's hashing algorithm, they could be in different orders with different GLib versions. */
	regex = g_regex_new ("<entry (xmlns='http://www.w3.org/2005/Atom' ?|"
				     "xmlns:gphoto='http://schemas.google.com/photos/2007' ?|"
				     "xmlns:media='http://search.yahoo.com/mrss/' ?|"
				     "xmlns:gd='http://schemas.google.com/g/2005' ?|"
				     "xmlns:gml='http://www.opengis.net/gml' ?|"
				     "xmlns:app='http://www.w3.org/2007/app' ?|"
				     "xmlns:georss='http://www.georss.org/georss' ?){7}>"
					"<title type='text'></title>"
					"<id>http://picasaweb.google.com/data/entry/user/libgdata.picasaweb/albumid/5328889949261497249</id>"
					"<rights>private</rights>"
					"<category term='http://schemas.google.com/photos/2007#album' "
						"scheme='http://schemas.google.com/g/2005#kind'/>"
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
	parsed_time_str = g_match_info_fetch (match_info, 2);
	delta = g_ascii_strtoull (parsed_time_str, NULL, 10) - (((guint64) timeval.tv_sec) * 1000 + ((guint64) timeval.tv_usec) / 1000);
	g_assert_cmpuint (abs (delta), <, 1000);

	g_free (parsed_time_str);
	g_free (xml);
	g_regex_unref (regex);
	g_match_info_free (match_info);
	g_object_unref (album);
}

static void
test_album_escaping (void)
{
	GDataPicasaWebAlbum *album;
	GError *error = NULL;
	const gchar * const tags[] = { "<tag1>", "tag2 & stuff, things", NULL };

	/* We have to create the album this way so that the album ID is set */
	album = GDATA_PICASAWEB_ALBUM (gdata_parsable_new_from_xml (GDATA_TYPE_PICASAWEB_ALBUM,
		"<entry xmlns='http://www.w3.org/2005/Atom' xmlns:gphoto='http://schemas.google.com/photos/2007'>"
			"<title type='text'></title>"
			"<category term='http://schemas.google.com/photos/2007#album' scheme='http://schemas.google.com/g/2005#kind'/>"
			"<gphoto:id>&lt;id&gt;</gphoto:id>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_PICASAWEB_ALBUM (album));
	g_clear_error (&error);

	/* Set other properties */
	gdata_picasaweb_album_set_location (album, "Everywhere & nowhere");
	gdata_picasaweb_album_set_tags (album, tags);

	/* Check the outputted XML is escaped properly */
	gdata_test_assert_xml (album,
	                 "<?xml version='1.0' encoding='UTF-8'?>"
	                 "<entry xmlns='http://www.w3.org/2005/Atom' xmlns:gphoto='http://schemas.google.com/photos/2007' "
	                        "xmlns:media='http://search.yahoo.com/mrss/' xmlns:gd='http://schemas.google.com/g/2005' "
	                        "xmlns:gml='http://www.opengis.net/gml' xmlns:app='http://www.w3.org/2007/app' "
	                        "xmlns:georss='http://www.georss.org/georss'>"
				"<title type='text'></title>"
				"<rights>private</rights>"
				"<category term='http://schemas.google.com/photos/2007#album' scheme='http://schemas.google.com/g/2005#kind'/>"
				"<gphoto:id>&lt;id&gt;</gphoto:id>"
				"<gphoto:location>Everywhere &amp; nowhere</gphoto:location>"
				"<gphoto:access>private</gphoto:access>"
				"<gphoto:commentingEnabled>false</gphoto:commentingEnabled>"
				"<media:group><media:keywords>&lt;tag1&gt;,tag2 &amp; stuff%2C things</media:keywords></media:group>"
	                 "</entry>");
	g_object_unref (album);
}

static void
test_album_properties_coordinates (void)
{
	GDataPicasaWebAlbum *album;
	gdouble latitude, longitude, original_latitude, original_longitude;

	/* Create a new album to test against */
	album = gdata_picasaweb_album_new (NULL);
	gdata_picasaweb_album_set_coordinates (album, 45.434336, 12.338784);

	/* Getting the coordinates */
	gdata_picasaweb_album_get_coordinates (album, &latitude, &longitude);
	g_assert_cmpfloat (latitude, ==, 45.434336);
	gdata_picasaweb_album_get_coordinates (album, &original_latitude, &original_longitude);
	g_assert_cmpfloat (original_latitude, ==, 45.434336);
	g_assert_cmpfloat (original_longitude, ==, 12.338784);

	/* Providing NULL to either or both parameters */
	gdata_picasaweb_album_get_coordinates (album, NULL, &longitude);
	g_assert_cmpfloat (longitude, ==, 12.338784);
	gdata_picasaweb_album_get_coordinates (album, &latitude, NULL);
	g_assert_cmpfloat (latitude, ==, 45.434336);
	gdata_picasaweb_album_get_coordinates (album, NULL, NULL);

	/* Setting the coordinates */
	gdata_picasaweb_album_set_coordinates (album, original_longitude, original_latitude);
	gdata_picasaweb_album_get_coordinates (album, &latitude, &longitude);
	g_assert_cmpfloat (latitude, ==, original_longitude);
	g_assert_cmpfloat (longitude, ==, original_latitude);
	gdata_picasaweb_album_set_coordinates (album, original_latitude, original_longitude);
	gdata_picasaweb_album_get_coordinates (album, &original_latitude, &original_longitude);
	g_assert_cmpfloat (original_latitude, ==, 45.434336);
	g_assert_cmpfloat (original_longitude, ==, 12.338784);

	g_object_unref (album);
}

static void
test_album_properties_visibility (void)
{
	GDataPicasaWebAlbum *album;
	gchar *original_rights;

	/* Create a test album */
	album = gdata_picasaweb_album_new (NULL);

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

	g_object_unref (album);
}

static void
test_file_escaping (void)
{
	GDataPicasaWebFile *file;
	GError *error = NULL;
	const gchar * const tags[] = { "<tag1>", "tag2 & stuff, things", NULL };

	/* We have to create the file this way so that the photo ID and version are set */
	file = GDATA_PICASAWEB_FILE (gdata_parsable_new_from_xml (GDATA_TYPE_PICASAWEB_FILE,
		"<entry xmlns='http://www.w3.org/2005/Atom' xmlns:gphoto='http://schemas.google.com/photos/2007'>"
			"<title type='text'></title>"
			"<category term='http://schemas.google.com/photos/2007#photo' scheme='http://schemas.google.com/g/2005#kind'/>"
			"<gphoto:id>&lt;id&gt;</gphoto:id>"
			"<gphoto:imageVersion>&lt;version&gt;</gphoto:imageVersion>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_PICASAWEB_FILE (file));
	g_clear_error (&error);

	/* Set other properties */
	gdata_picasaweb_file_set_album_id (file, "http://foo.com?foo&bar");
	gdata_picasaweb_file_set_checksum (file, "<checksum>");
	gdata_picasaweb_file_set_tags (file, tags);
	gdata_picasaweb_file_set_caption (file, "Caption & stuff.");

	/* Check the outputted XML is escaped properly */
	gdata_test_assert_xml (file,
	                 "<?xml version='1.0' encoding='UTF-8'?>"
	                 "<entry xmlns='http://www.w3.org/2005/Atom' xmlns:gphoto='http://schemas.google.com/photos/2007' "
	                        "xmlns:media='http://search.yahoo.com/mrss/' xmlns:gd='http://schemas.google.com/g/2005' "
	                        "xmlns:exif='http://schemas.google.com/photos/exif/2007' xmlns:app='http://www.w3.org/2007/app' "
	                        "xmlns:georss='http://www.georss.org/georss' xmlns:gml='http://www.opengis.net/gml'>"
				"<title type='text'></title>"
				"<summary type='text'>Caption &amp; stuff.</summary>"
				"<category term='http://schemas.google.com/photos/2007#photo' scheme='http://schemas.google.com/g/2005#kind'/>"
				"<gphoto:id>&lt;id&gt;</gphoto:id>"
				"<gphoto:imageVersion>&lt;version&gt;</gphoto:imageVersion>"
				"<gphoto:albumid>http://foo.com?foo&amp;bar</gphoto:albumid>"
				"<gphoto:checksum>&lt;checksum&gt;</gphoto:checksum>"
				"<gphoto:commentingEnabled>true</gphoto:commentingEnabled>"
				"<media:group>"
					"<media:description type='plain'>Caption &amp; stuff.</media:description>"
					"<media:keywords>&lt;tag1&gt;,tag2 &amp; stuff%2C things</media:keywords>"
				"</media:group>"
	                 "</entry>");
	g_object_unref (file);
}

static void
test_file_properties_coordinates (void)
{
	GDataPicasaWebFile *file;
	gdouble latitude, longitude, original_latitude, original_longitude;

	/* Create a new file to test against */
	file = gdata_picasaweb_file_new (NULL);
	gdata_picasaweb_file_set_coordinates (file, 45.4341173, 12.1289062);

	/* Getting the coordinates */
	gdata_picasaweb_file_get_coordinates (file, &original_latitude, &original_longitude);
	g_assert_cmpfloat (original_latitude, ==, 45.4341173);
	g_assert_cmpfloat (original_longitude, ==, 12.1289062);

	/* Providing NULL to either or both parameters */
	gdata_picasaweb_file_get_coordinates (file, NULL, &longitude);
	g_assert_cmpfloat (longitude, ==, 12.1289062);
	gdata_picasaweb_file_get_coordinates (file, &latitude, NULL);
	g_assert_cmpfloat (latitude, ==, 45.4341173);
	gdata_picasaweb_file_get_coordinates (file, NULL, NULL);

	/* Setting the coordinates */
	gdata_picasaweb_file_set_coordinates (file, original_longitude, original_latitude);
	gdata_picasaweb_file_get_coordinates (file, &latitude, &longitude);
	g_assert_cmpfloat (latitude, ==, original_longitude);
	g_assert_cmpfloat (longitude, ==, original_latitude);
	gdata_picasaweb_file_set_coordinates (file, original_latitude, original_longitude);
	gdata_picasaweb_file_get_coordinates (file, &latitude, &longitude);
	g_assert_cmpfloat (latitude, ==, 45.4341173);
	g_assert_cmpfloat (longitude, ==, 12.1289062);

	g_object_unref (file);
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
	gint retval;
	GDataAuthorizer *authorizer = NULL;
	GDataService *service = NULL;

	gdata_test_init (argc, argv);

	if (gdata_test_internet () == TRUE) {
		authorizer = GDATA_AUTHORIZER (gdata_client_login_authorizer_new (CLIENT_ID, GDATA_TYPE_PICASAWEB_SERVICE));
		gdata_client_login_authorizer_authenticate (GDATA_CLIENT_LOGIN_AUTHORIZER (authorizer), PW_USERNAME, PASSWORD, NULL, NULL);

		service = GDATA_SERVICE (gdata_picasaweb_service_new (authorizer));

		g_test_add_func ("/picasaweb/authentication", test_authentication);
		g_test_add_func ("/picasaweb/authentication_async", test_authentication_async);

		g_test_add ("/picasaweb/query/all_albums", QueryAllAlbumsData, service, set_up_query_all_albums, test_query_all_albums,
		            tear_down_query_all_albums);
		g_test_add ("/picasaweb/query/all_albums/with_limits", QueryAllAlbumsData, service, set_up_query_all_albums,
		            test_query_all_albums_with_limits, tear_down_query_all_albums);
		g_test_add ("/picasaweb/query/all_albums/async", QueryAllAlbumsAsyncData, service, set_up_query_all_albums_async,
		            test_query_all_albums_async, tear_down_query_all_albums_async);
		g_test_add ("/picasaweb/query/all_albums/async/progress_closure", QueryAllAlbumsAsyncData, service, set_up_query_all_albums_async,
		            test_query_all_albums_async_progress_closure, tear_down_query_all_albums_async);
		g_test_add_data_func ("/picasaweb/query/all_albums/bad_query", service, test_query_all_albums_bad_query);
		g_test_add_data_func ("/picasaweb/query/all_albums/bad_query/with_limits", service, test_query_all_albums_bad_query_with_limits);

		g_test_add_data_func ("/picasaweb/query/user", service, test_query_user);

		g_test_add ("/picasaweb/insert/album", InsertAlbumData, service, set_up_insert_album, test_insert_album, tear_down_insert_album);
		g_test_add ("/picasaweb/insert/album/async", InsertAlbumAsyncData, service, set_up_insert_album_async, test_insert_album_async,
		            tear_down_insert_album_async);

		g_test_add_data_func ("/picasaweb/query/photo_feed", service, test_photo_feed);
		g_test_add_data_func ("/picasaweb/query/photo_feed_entry", service, test_photo_feed_entry);
		g_test_add_data_func ("/picasaweb/query/photo", service, test_photo);
		g_test_add_data_func ("/picasaweb/query/photo_single", service, test_photo_single);
		g_test_add_data_func ("/picasaweb/query/photo/async", service, test_photo_async);
		g_test_add_data_func ("/picasaweb/query/photo/async_progress_closure", service, test_photo_async_progress_closure);

		g_test_add ("/picasaweb/upload/default_album", UploadData, service, setup_upload, test_upload_default_album, teardown_upload);
		g_test_add ("/picasaweb/upload/default_album/async", UploadAsyncData, service, setup_upload_async, test_upload_default_album_async,
		            teardown_upload_async);
		g_test_add ("/picasaweb/upload/default_album/cancellation", UploadAsyncData, service, setup_upload_async,
		            test_upload_default_album_cancellation, teardown_upload_async);
		g_test_add ("/picasaweb/upload/default_album/cancellation2", UploadAsyncData, service, setup_upload_async,
		            test_upload_default_album_cancellation2, teardown_upload_async);

		g_test_add_data_func ("/picasaweb/download/photo", service, test_download);
		g_test_add_data_func ("/picasaweb/download/thumbnails", service, test_download_thumbnails);
	}

	g_test_add_func ("/picasaweb/album/new", test_album_new);
	g_test_add_func ("/picasaweb/album/escaping", test_album_escaping);
	g_test_add_func ("/picasaweb/album/properties/coordinates", test_album_properties_coordinates);
	g_test_add_func ("/picasaweb/album/properties/visibility", test_album_properties_visibility);

	g_test_add_func ("/picasaweb/file/escaping", test_file_escaping);
	g_test_add_func ("/picasaweb/file/properties/coordinates", test_file_properties_coordinates);

	g_test_add_func ("/picasaweb/query/etag", test_query_etag);

	retval = g_test_run ();

	if (service != NULL)
		g_object_unref (service);

	return retval;
}
