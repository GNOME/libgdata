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
#include "gdata-dummy-authorizer.h"

#undef CLIENT_ID  /* from common.h */

#define CLIENT_ID "352818697630-nqu2cmt5quqd6lr17ouoqmb684u84l1f.apps.googleusercontent.com"
#define CLIENT_SECRET "-fA4pHQJxR3zJ-FyAMPQsikg"
#define REDIRECT_URI "urn:ietf:wg:oauth:2.0:oob"

static UhmServer *mock_server = NULL;

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
	/* Apparently Google will arbitrarily change content URIs for things at various times. */
	/*g_assert_cmpstr (gdata_entry_get_content_uri (GDATA_ENTRY (file1)), ==, gdata_entry_get_content_uri (GDATA_ENTRY (file2)));*/
	g_assert (strstr (gdata_entry_get_content_uri (GDATA_ENTRY (file1)), "googleusercontent.com") != NULL);
	g_assert_cmpstr (gdata_entry_get_rights (GDATA_ENTRY (file1)), ==, gdata_entry_get_rights (GDATA_ENTRY (file2)));

	if (compare_inserted_data == TRUE) {
		g_assert_cmpstr (gdata_entry_get_id (GDATA_ENTRY (file1)), ==, gdata_entry_get_id (GDATA_ENTRY (file2)));
		g_assert_cmpstr (gdata_entry_get_id (GDATA_ENTRY (file1)), !=, NULL);
		/* Note: We don't check the ETags are equal, because Google like to randomly change ETags without warning. */
		g_assert_cmpstr (gdata_entry_get_etag (GDATA_ENTRY (file1)), !=, NULL);
		g_assert_cmpstr (gdata_entry_get_etag (GDATA_ENTRY (file2)), !=, NULL);
		/* Same for the updated times. */
		g_assert_cmpint (gdata_entry_get_updated (GDATA_ENTRY (file1)), >, 0);
		g_assert_cmpint (gdata_entry_get_updated (GDATA_ENTRY (file2)), >, 0);
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

		/* Same as above; don't compare the edited times. */
		g_assert_cmpint (gdata_picasaweb_file_get_edited (file1), >, 0);
		g_assert_cmpint (gdata_picasaweb_file_get_edited (file2), >, 0);
		/* See ETags and content URIs above. */
		/*g_assert_cmpstr (gdata_picasaweb_file_get_version (file1), ==, gdata_picasaweb_file_get_version (file2));*/
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

			/* And here: Google can arbitrarily change content URIs. */
			/*g_assert_cmpstr (gdata_media_content_get_uri (content1), ==, gdata_media_content_get_uri (content2));*/
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

			/* And here: Google can arbitrarily change thumbnail URIs. */
			/*g_assert_cmpstr (gdata_media_thumbnail_get_uri (thumbnail1), ==, gdata_media_thumbnail_get_uri (thumbnail2));*/
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
	GDataOAuth2Authorizer *authorizer = NULL;  /* owned */
	gchar *authentication_uri, *authorisation_code;

	gdata_test_mock_server_start_trace (mock_server, "authentication");

	authorizer = gdata_oauth2_authorizer_new (CLIENT_ID, CLIENT_SECRET,
	                                          REDIRECT_URI,
	                                          GDATA_TYPE_PICASAWEB_SERVICE);

	/* Get an authentication URI. */
	authentication_uri = gdata_oauth2_authorizer_build_authentication_uri (authorizer, NULL, FALSE);
	g_assert (authentication_uri != NULL);

	/* Get the authorisation code off the user. */
	if (uhm_server_get_enable_online (mock_server)) {
		authorisation_code = gdata_test_query_user_for_verifier (authentication_uri);
	} else {
		/* Hard coded, extracted from the trace file. */
		authorisation_code = g_strdup ("4/OEX-S1iMbOA_dOnNgUlSYmGWh3TK.QrR73axcNMkWoiIBeO6P2m_su7cwkQI");
	}

	g_free (authentication_uri);

	if (authorisation_code == NULL) {
		/* Skip tests. */
		goto skip_test;
	}

	/* Authorise the token */
	g_assert (gdata_oauth2_authorizer_request_authorization (authorizer, authorisation_code, NULL, NULL) == TRUE);

	/* Check all is as it should be */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	                                                     gdata_picasaweb_service_get_primary_authorization_domain ()) == TRUE);

skip_test:
	g_free (authorisation_code);
	g_object_unref (authorizer);

	uhm_server_end_trace (mock_server);
}

typedef struct {
	GDataPicasaWebAlbum *album;
	GDataPicasaWebFile *file1;
	GDataPicasaWebFile *file2;
	GDataPicasaWebFile *file3;
	GDataPicasaWebFile *file4;
} QueryFilesData;

static GDataPicasaWebFile *
upload_file (GDataPicasaWebService *service, const gchar *title, GDataPicasaWebAlbum *album)
{
	GDataPicasaWebFile *file, *uploaded_file;
	GFile *photo_file;
	GFileInfo *file_info;
	GFileInputStream *input_stream;
	GDataUploadStream *upload_stream;
	gchar *path = NULL;

	file = gdata_picasaweb_file_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (file), title);

	/* File is public domain: http://en.wikipedia.org/wiki/File:German_garden_gnome_cropped.jpg */
	path = g_test_build_filename (G_TEST_DIST, "photo.jpg", NULL);
	photo_file = g_file_new_for_path (path);
	g_free (path);

	/* Get the file's info */
	file_info = g_file_query_info (photo_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
	                               G_FILE_QUERY_INFO_NONE, NULL, NULL);
	g_assert (G_IS_FILE_INFO (file_info));

	/* Get an input stream for the file */
	input_stream = g_file_read (photo_file, NULL, NULL);
	g_assert (G_IS_FILE_INPUT_STREAM (input_stream));

	g_object_unref (photo_file);

	/* Prepare the upload stream */
	upload_stream = gdata_picasaweb_service_upload_file (GDATA_PICASAWEB_SERVICE (service), album, file, g_file_info_get_display_name (file_info),
	                                                     g_file_info_get_content_type (file_info), NULL, NULL);
	g_assert (GDATA_IS_UPLOAD_STREAM (upload_stream));

	g_object_unref (file_info);
	g_object_unref (file);

	/* Upload the photo */
	g_assert_cmpint (g_output_stream_splice (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (input_stream),
	                                         G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, NULL), >, 0);

	g_object_unref (input_stream);

	/* Finish off the upload */
	uploaded_file = gdata_picasaweb_service_finish_file_upload (GDATA_PICASAWEB_SERVICE (service), upload_stream, NULL);

	g_object_unref (upload_stream);

	g_assert (GDATA_IS_PICASAWEB_FILE (uploaded_file));

	return uploaded_file;
}

static void
set_up_query_files (QueryFilesData *data, gconstpointer service)
{
	GDataPicasaWebAlbum *album;

	gdata_test_mock_server_start_trace (mock_server, "setup-query-files");

	/* Album */
	album = gdata_picasaweb_album_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (album), "Test album for QueryFiles");

	data->album = gdata_picasaweb_service_insert_album (GDATA_PICASAWEB_SERVICE (service), album, NULL, NULL);
	g_assert (data->album != NULL);

	g_object_unref (album);

	/* Upload the files */
	data->file1 = upload_file (GDATA_PICASAWEB_SERVICE (service), "Test file 1", data->album);
	data->file2 = upload_file (GDATA_PICASAWEB_SERVICE (service), "Test file 2", data->album);
	data->file3 = upload_file (GDATA_PICASAWEB_SERVICE (service), "Test file 3", data->album);
	data->file4 = upload_file (GDATA_PICASAWEB_SERVICE (service), "Test file 4", data->album);

	uhm_server_end_trace (mock_server);
}

static void
tear_down_query_files (QueryFilesData *data, gconstpointer service)
{
	GDataFeed *album_feed;
	GDataEntry *album;

	g_object_unref (data->file4);
	g_object_unref (data->file3);
	g_object_unref (data->file2);
	g_object_unref (data->file1);

	/* HACK! Wait for the distributed Google servers to synchronise. */
	if (uhm_server_get_enable_online (mock_server) == TRUE) {
		sleep (10);
	}

	gdata_test_mock_server_start_trace (mock_server, "teardown-query-files");

	/* We have to re-query for the album, since its ETag will be out of date */
	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL, NULL, NULL);
	album = gdata_feed_look_up_entry (GDATA_FEED (album_feed), gdata_entry_get_id (GDATA_ENTRY (data->album)));
	g_assert (GDATA_IS_PICASAWEB_ALBUM (album));

	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_picasaweb_service_get_primary_authorization_domain (),
	                                      album, NULL, NULL) == TRUE);

	g_object_unref (album_feed);
	g_object_unref (data->album);

	uhm_server_end_trace (mock_server);
}

/* Checks to perform on a photo feed from test_query_files() or test_query_files_async(). */
static void
_test_query_files (GDataFeed *photo_feed, QueryFilesData *data)
{
	GDataEntry *entry;
	gchar *xml;

	g_assert (GDATA_IS_FEED (photo_feed));

	/* Check properties of the feed */
	g_assert_cmpint (g_list_length (gdata_feed_get_entries (photo_feed)), ==, 4);

	g_assert_cmpstr (gdata_feed_get_title (photo_feed), ==, "Test album for QueryFiles");
	g_assert_cmpstr (gdata_feed_get_subtitle (photo_feed), ==, NULL);
	g_assert_cmpstr (gdata_feed_get_id (photo_feed), !=, NULL);
	g_assert_cmpstr (gdata_feed_get_etag (photo_feed), !=, NULL); /* this varies as the album changes, e.g. when new images are uploaded */
	g_assert_cmpstr (gdata_feed_get_icon (photo_feed), !=, NULL); /* tested weakly because it changes fairly regularly */
	g_assert_cmpuint (gdata_feed_get_items_per_page (photo_feed), ==, 1000);
	g_assert_cmpuint (gdata_feed_get_start_index (photo_feed), ==, 1);
	g_assert_cmpuint (gdata_feed_get_total_results (photo_feed), ==, 4);

	/* Test the first file */
	entry = gdata_feed_look_up_entry (photo_feed, gdata_entry_get_id (GDATA_ENTRY (data->file1)));
	g_assert (entry != NULL);
	g_assert (GDATA_IS_PICASAWEB_FILE (entry));

	assert_files_equal (GDATA_PICASAWEB_FILE (entry), data->file1, TRUE);

	xml = gdata_parsable_get_xml (GDATA_PARSABLE (entry));
	g_assert_cmpstr (xml, !=, NULL);
	g_assert_cmpuint (strlen (xml), >, 0);
	g_free (xml);
}

static void
test_query_files (QueryFilesData *data, gconstpointer service)
{
	GDataFeed *photo_feed;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "query-files");

	photo_feed = gdata_picasaweb_service_query_files (GDATA_PICASAWEB_SERVICE (service), data->album, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (photo_feed));
	g_clear_error (&error);

	_test_query_files (photo_feed, data);

	g_object_unref (photo_feed);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_CLOSURE_FUNCTIONS (query_files, QueryFilesData);

/* Test that asynchronously querying for all photos in an album lists them correctly. */
GDATA_ASYNC_TEST_FUNCTIONS (query_files, QueryFilesData,
G_STMT_START {
	gdata_picasaweb_service_query_files_async (GDATA_PICASAWEB_SERVICE (service), data->album, NULL, cancellable, NULL, NULL, NULL,
	                                           async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataFeed *photo_feed;

	/* Get the photo feed */
	photo_feed = gdata_service_query_finish (GDATA_SERVICE (obj), async_result, &error);

	if (error == NULL) {
		_test_query_files (photo_feed, data);
		g_object_unref (photo_feed);
	} else {
		g_assert (photo_feed == NULL);
	}
} G_STMT_END);

/* Test that the progress callbacks from gdata_picasaweb_service_query_files_async() are called correctly.
 * We take a QueryFilesData so that we can guarantee the album and at least one file exists (since it's created in the setup function for
 * QueryFilesData), but we don't use it much as we don't actually care about the specific files. */
static void
test_query_files_async_progress_closure (QueryFilesData *query_data, gconstpointer service)
{
	GDataAsyncProgressClosure *data = g_slice_new0 (GDataAsyncProgressClosure);

	gdata_test_mock_server_start_trace (mock_server, "query-files-async-progress-closure");

	data->main_loop = g_main_loop_new (NULL, TRUE);

	gdata_picasaweb_service_query_files_async (GDATA_PICASAWEB_SERVICE (service), query_data->album, NULL, NULL,
	                                           (GDataQueryProgressCallback) gdata_test_async_progress_callback,
	                                           data, (GDestroyNotify) gdata_test_async_progress_closure_free,
	                                           (GAsyncReadyCallback) gdata_test_async_progress_finish_callback, data);

	g_main_loop_run (data->main_loop);
	g_main_loop_unref (data->main_loop);

	/* Check that both callbacks were called exactly once */
	g_assert_cmpuint (data->progress_destroy_notify_count, ==, 1);
	g_assert_cmpuint (data->async_ready_notify_count, ==, 1);

	g_slice_free (GDataAsyncProgressClosure, data);

	uhm_server_end_trace (mock_server);
}

static void
test_query_files_single (QueryFilesData *data, gconstpointer service)
{
	GDataEntry *file;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "query-files-single");

	file = gdata_service_query_single_entry (GDATA_SERVICE (service), gdata_picasaweb_service_get_primary_authorization_domain (),
	                                         gdata_entry_get_id (GDATA_ENTRY (data->file1)), NULL, GDATA_TYPE_PICASAWEB_FILE, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_PICASAWEB_FILE (file));
	g_clear_error (&error);

	assert_files_equal (GDATA_PICASAWEB_FILE (file), data->file1, TRUE);

	g_object_unref (file);

	uhm_server_end_trace (mock_server);
}

static void
test_download_thumbnails (QueryFilesData *data, gconstpointer service)
{
	GList *thumbnails, *node;
	GDataPicasaWebFile *photo;
	GDataMediaThumbnail *thumbnail;
	GDataDownloadStream *download_stream;
	gchar *destination_file_name, *destination_file_path;
	GFile *destination_file;
	GFileOutputStream *file_stream;
	gssize transfer_size;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "download-thumbnails");

	photo = GDATA_PICASAWEB_FILE (data->file3);

	thumbnails = gdata_picasaweb_file_get_thumbnails (photo);
	thumbnail = GDATA_MEDIA_THUMBNAIL (thumbnails->data);

	/* Download a single thumbnail to a file for testing (in case we weren't compiled with GdkPixbuf support) */
	download_stream = gdata_media_thumbnail_download (thumbnail, GDATA_SERVICE (service), NULL, &error);
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

		/* FIXME. The mock server currently doesn't support binary data, so we can't get JPEG files
		 * from it. Hence, only perform the GdkPixbuf tests when running tests online. */
		if (uhm_server_get_enable_online (mock_server) == FALSE) {
			break;
		}

		/* Prepare a download stream */
		download_stream = gdata_media_thumbnail_download (thumbnail, GDATA_SERVICE (service), NULL, &error);
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

	uhm_server_end_trace (mock_server);
}

static void
test_download_photo (QueryFilesData *data, gconstpointer service)
{
	GList *media_contents;
	GDataPicasaWebFile *photo;
	GDataMediaContent *content;
	GDataDownloadStream *download_stream;
	gchar *destination_file_name, *destination_file_path;
	GFile *destination_file;
	GFileOutputStream *file_stream;
	gssize transfer_size;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "download-photo");

	photo = GDATA_PICASAWEB_FILE (data->file3);

	media_contents = gdata_picasaweb_file_get_contents (photo);
	g_assert_cmpint (g_list_length (media_contents), ==, 1);
	content = GDATA_MEDIA_CONTENT (media_contents->data);

	/* Prepare a download stream */
	download_stream = gdata_media_content_download (content, GDATA_SERVICE (service), NULL, &error);
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

	uhm_server_end_trace (mock_server);
}

typedef struct {
	GDataPicasaWebAlbum *album;
	GDataPicasaWebAlbum *inserted_album;
} InsertAlbumData;

static void
set_up_insert_album (InsertAlbumData *data, gconstpointer service)
{
	GDateTime *timestamp;

	data->album = gdata_picasaweb_album_new (NULL);
	g_assert (GDATA_IS_PICASAWEB_ALBUM (data->album));

	gdata_entry_set_title (GDATA_ENTRY (data->album), "Thanksgiving photos");
	gdata_entry_set_summary (GDATA_ENTRY (data->album), "Family photos of the feast!");
	gdata_picasaweb_album_set_location (data->album, "Winnipeg, MN");

	timestamp = g_date_time_new_from_iso8601 ("2002-10-14T09:58:59.643554Z", NULL);
	gdata_picasaweb_album_set_timestamp (data->album, g_date_time_to_unix (timestamp) * 1000);
	g_date_time_unref (timestamp);
}

static void
tear_down_insert_album (InsertAlbumData *data, gconstpointer service)
{
	gdata_test_mock_server_start_trace (mock_server, "teardown-insert-album");

	/* Clean up the evidence */
	gdata_service_delete_entry (GDATA_SERVICE (service), gdata_picasaweb_service_get_primary_authorization_domain (),
	                            GDATA_ENTRY (data->inserted_album), NULL, NULL);

	g_object_unref (data->album);
	g_object_unref (data->inserted_album);

	uhm_server_end_trace (mock_server);
}

static void
test_insert_album (InsertAlbumData *data, gconstpointer service)
{
	GDataPicasaWebAlbum *inserted_album;
	GDataFeed *album_feed;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "insert-album");

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

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_CLOSURE_FUNCTIONS (insert_album, InsertAlbumData);

GDATA_ASYNC_TEST_FUNCTIONS (insert_album, InsertAlbumData,
G_STMT_START {
	gdata_picasaweb_service_insert_album_async (GDATA_PICASAWEB_SERVICE (service), data->album, cancellable,
	                                            async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataEntry *entry;

	entry = gdata_service_insert_entry_finish (GDATA_SERVICE (obj), async_result, &error);

	if (error == NULL) {
		g_assert (GDATA_IS_PICASAWEB_ALBUM (entry));

		/* Test the album was uploaded correctly */
		assert_albums_equal (GDATA_PICASAWEB_ALBUM (entry), data->album, FALSE);

		data->inserted_album = GDATA_PICASAWEB_ALBUM (entry);
	} else {
		g_assert (entry == NULL);
	}
} G_STMT_END);

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

	gdata_test_mock_server_start_trace (mock_server, "setup-query-all-albums");

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

	uhm_server_end_trace (mock_server);
}

static void
tear_down_query_all_albums (QueryAllAlbumsData *data, gconstpointer service)
{
	gdata_test_mock_server_start_trace (mock_server, "teardown-query-all-albums");

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

	uhm_server_end_trace (mock_server);
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

	gdata_test_mock_server_start_trace (mock_server, "query-all-albums");

	/* Try a proper query */
	album_feed = gdata_picasaweb_service_query_all_albums (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	_test_query_all_albums (album_feed, data);

	g_object_unref (album_feed);

	uhm_server_end_trace (mock_server);
}

static void
test_query_all_albums_with_limits (QueryAllAlbumsData *data, gconstpointer service)
{
	GDataQuery *query;
	GDataFeed *album_feed_1, *album_feed_2;
	GError *error = NULL;
	GList *albums_1, *albums_2;

	gdata_test_mock_server_start_trace (mock_server, "query-all-albums-with-limits");

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

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_CLOSURE_FUNCTIONS (query_all_albums, QueryAllAlbumsData);

/* Test that asynchronously querying for all albums lists them correctly. */
GDATA_ASYNC_TEST_FUNCTIONS (query_all_albums, QueryAllAlbumsData,
G_STMT_START {
	gdata_picasaweb_service_query_all_albums_async (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, cancellable, NULL,
	                                                NULL, NULL, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataFeed *album_feed;

	/* Get the album feed */
	album_feed = gdata_service_query_finish (GDATA_SERVICE (obj), async_result, &error);

	if (error == NULL) {
		_test_query_all_albums (album_feed, (QueryAllAlbumsData*) data);

		g_object_unref (album_feed);
	} else {
		g_assert (album_feed == NULL);
	}
} G_STMT_END);

/* Test that the progress callbacks from gdata_picasaweb_service_query_all_albums_async() are called correctly.
 * We take a QueryAllAlbumsData so that we can guarantee at least one album exists (since it's created in the setup function for
 * QueryAllAlbumsData), but we don't use it as we don't actually care about the specific album. */
static void
test_query_all_albums_async_progress_closure (QueryAllAlbumsData *unused_data, gconstpointer service)
{
	GDataAsyncProgressClosure *data = g_slice_new0 (GDataAsyncProgressClosure);

	gdata_test_mock_server_start_trace (mock_server, "query-all-albums-async-progress-closure");

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

	uhm_server_end_trace (mock_server);
}

static void
check_authenticated_user_details (GDataPicasaWebUser *user)
{
	g_assert (GDATA_IS_PICASAWEB_USER (user));

	g_assert_cmpstr (gdata_picasaweb_user_get_user (user), ==, "libgdata.picasaweb");
	g_assert_cmpstr (gdata_picasaweb_user_get_nickname (user), ==, "libgdata.picasaweb");
	/* 15GiB: it'll be a beautiful day when this assert gets tripped */
	g_assert_cmpint (gdata_picasaweb_user_get_quota_limit (user), ==, 16106127360);
	g_assert_cmpint (gdata_picasaweb_user_get_quota_current (user), >=, 0);
	/* now it's 1000, testing this weakly to avoid having to regularly update it */
	g_assert_cmpint (gdata_picasaweb_user_get_max_photos_per_album (user), >, 0);
	/* tested weakly to avoid having to update it regularly */
	g_assert_cmpstr (gdata_picasaweb_user_get_thumbnail_uri (user), !=, NULL);
}

typedef struct {
	QueryFilesData parent;
	GDataPicasaWebComment *comment1;
	GDataPicasaWebComment *comment2;
	GDataPicasaWebComment *comment3;
} QueryCommentsData;

static void
set_up_query_comments (QueryCommentsData *data, gconstpointer service)
{
	GDataPicasaWebComment *comment_;

	/* Set up some test albums and files. */
	set_up_query_files ((QueryFilesData*) data, service);

	gdata_test_mock_server_start_trace (mock_server, "setup-query-comments");

	/* Insert four test comments on the first test file. */
	comment_ = gdata_picasaweb_comment_new (NULL);
	gdata_entry_set_content (GDATA_ENTRY (comment_), "Test comment 1.");
	data->comment1 = GDATA_PICASAWEB_COMMENT (gdata_commentable_insert_comment (GDATA_COMMENTABLE (data->parent.file1), GDATA_SERVICE (service),
	                                                                            GDATA_COMMENT (comment_), NULL, NULL));
	g_assert (GDATA_IS_PICASAWEB_COMMENT (data->comment1));
	g_object_unref (comment_);

	comment_ = gdata_picasaweb_comment_new (NULL);
	gdata_entry_set_content (GDATA_ENTRY (comment_), "Test comment 2.");
	data->comment2 = GDATA_PICASAWEB_COMMENT (gdata_commentable_insert_comment (GDATA_COMMENTABLE (data->parent.file1), GDATA_SERVICE (service),
	                                                                            GDATA_COMMENT (comment_), NULL, NULL));
	g_assert (GDATA_IS_PICASAWEB_COMMENT (data->comment1));
	g_object_unref (comment_);

	comment_ = gdata_picasaweb_comment_new (NULL);
	gdata_entry_set_content (GDATA_ENTRY (comment_), "Test comment 3.");
	data->comment3 = GDATA_PICASAWEB_COMMENT (gdata_commentable_insert_comment (GDATA_COMMENTABLE (data->parent.file1), GDATA_SERVICE (service),
	                                                                            GDATA_COMMENT (comment_), NULL, NULL));
	g_assert (GDATA_IS_PICASAWEB_COMMENT (data->comment1));
	g_object_unref (comment_);

	uhm_server_end_trace (mock_server);
}

static void
tear_down_query_comments (QueryCommentsData *data, gconstpointer service)
{
	gdata_test_mock_server_start_trace (mock_server, "teardown-query-comments");

	/* Delete the test comments. */
	if (data->comment1 != NULL) {
		gdata_commentable_delete_comment (GDATA_COMMENTABLE (data->parent.file1), GDATA_SERVICE (service),
		                                  GDATA_COMMENT (data->comment1), NULL, NULL);
		g_object_unref (data->comment1);
	}

	if (data->comment2 != NULL) {
		gdata_commentable_delete_comment (GDATA_COMMENTABLE (data->parent.file1), GDATA_SERVICE (service),
		                                  GDATA_COMMENT (data->comment2), NULL, NULL);
		g_object_unref (data->comment2);
	}

	if (data->comment3 != NULL) {
		gdata_commentable_delete_comment (GDATA_COMMENTABLE (data->parent.file1), GDATA_SERVICE (service),
		                                  GDATA_COMMENT (data->comment3), NULL, NULL);
		g_object_unref (data->comment3);
	}

	uhm_server_end_trace (mock_server);

	/* Delete the test files and albums. */
	tear_down_query_files ((QueryFilesData*) data, service);
}

static void
assert_comments_feed (QueryCommentsData *data, GDataFeed *comments_feed)
{
	gboolean comment1_seen = FALSE, comment2_seen = FALSE, comment3_seen = FALSE;
	GList *comments;

	g_assert (GDATA_IS_FEED (comments_feed));
	g_assert_cmpuint (g_list_length (gdata_feed_get_entries (comments_feed)), >=, 3);

	for (comments = gdata_feed_get_entries (comments_feed); comments != NULL; comments = comments->next) {
		GList *authors;
		GDataPicasaWebComment *expected_comment, *actual_comment;

		actual_comment = GDATA_PICASAWEB_COMMENT (comments->data);

		if (strcmp (gdata_entry_get_id (GDATA_ENTRY (data->comment1)), gdata_entry_get_id (GDATA_ENTRY (actual_comment))) == 0) {
			g_assert (comment1_seen == FALSE);
			comment1_seen = TRUE;
			expected_comment = data->comment1;
		} else if (strcmp (gdata_entry_get_id (GDATA_ENTRY (data->comment2)), gdata_entry_get_id (GDATA_ENTRY (actual_comment))) == 0) {
			g_assert (comment2_seen == FALSE);
			comment2_seen = TRUE;
			expected_comment = data->comment2;
		} else if (strcmp (gdata_entry_get_id (GDATA_ENTRY (data->comment3)), gdata_entry_get_id (GDATA_ENTRY (actual_comment))) == 0) {
			g_assert (comment3_seen == FALSE);
			comment3_seen = TRUE;
			expected_comment = data->comment3;
		} else {
			/* Unknown comment; we'll assume it's been added externally to the test suite. */
			continue;
		}

		g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (actual_comment)), ==, gdata_entry_get_title (GDATA_ENTRY (expected_comment)));
		g_assert_cmpstr (gdata_entry_get_content (GDATA_ENTRY (actual_comment)), ==, gdata_entry_get_content (GDATA_ENTRY (expected_comment)));

		g_assert_cmpuint (g_list_length (gdata_entry_get_authors (GDATA_ENTRY (actual_comment))), >, 0);

		for (authors = gdata_entry_get_authors (GDATA_ENTRY (actual_comment)); authors != NULL; authors = authors->next) {
			GDataAuthor *author = GDATA_AUTHOR (authors->data);

			/* We can't test these much. */
			g_assert_cmpstr (gdata_author_get_name (author), !=, NULL);
			g_assert_cmpstr (gdata_author_get_uri (author), !=, NULL);
		}
	}
}

static void
test_comment_query (QueryCommentsData *data, gconstpointer service)
{
	GDataFeed *comments_feed;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "comment-query");

	comments_feed = gdata_commentable_query_comments (GDATA_COMMENTABLE (data->parent.file1), GDATA_SERVICE (service), NULL, NULL, NULL, NULL,
	                                                  &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	assert_comments_feed (data, comments_feed);

	g_object_unref (comments_feed);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_CLOSURE_FUNCTIONS (query_comments, QueryCommentsData);

/* Test that asynchronously querying for all albums lists them correctly. */
GDATA_ASYNC_TEST_FUNCTIONS (comment_query, QueryCommentsData,
G_STMT_START {
	gdata_commentable_query_comments_async (GDATA_COMMENTABLE (data->parent.file1), GDATA_SERVICE (service), NULL, cancellable, NULL, NULL, NULL,
	                                        async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataFeed *comments_feed;

	/* Get the comments feed */
	comments_feed = gdata_commentable_query_comments_finish (GDATA_COMMENTABLE (obj), async_result, &error);

	if (error == NULL) {
		assert_comments_feed (data, comments_feed);

		g_object_unref (comments_feed);
	} else {
		g_assert (comments_feed == NULL);
	}
} G_STMT_END);

/* Test that the progress callbacks from gdata_commentable_query_comments_async() are called correctly.
 * We take a QueryCommentsData so that we can guarantee the file exists, but we don't use it much as we don't actually care about the specific
 * file. */
static void
test_comment_query_async_progress_closure (QueryCommentsData *query_data, gconstpointer service)
{
	GDataAsyncProgressClosure *data = g_slice_new0 (GDataAsyncProgressClosure);

	gdata_test_mock_server_start_trace (mock_server, "comment-query-async-progress-closure");

	data->main_loop = g_main_loop_new (NULL, TRUE);

	gdata_commentable_query_comments_async (GDATA_COMMENTABLE (query_data->parent.file1), GDATA_SERVICE (service), NULL, NULL,
	                                        (GDataQueryProgressCallback) gdata_test_async_progress_callback,
	                                        data, (GDestroyNotify) gdata_test_async_progress_closure_free,
	                                        (GAsyncReadyCallback) gdata_test_async_progress_finish_callback, data);

	g_main_loop_run (data->main_loop);
	g_main_loop_unref (data->main_loop);

	/* Check that both callbacks were called exactly once */
	g_assert_cmpuint (data->progress_destroy_notify_count, ==, 1);
	g_assert_cmpuint (data->async_ready_notify_count, ==, 1);

	g_slice_free (GDataAsyncProgressClosure, data);

	uhm_server_end_trace (mock_server);
}

typedef struct {
	QueryFilesData parent;
	GDataPicasaWebComment *comment;
	GDataPicasaWebComment *new_comment;
} InsertCommentData;

static void
set_up_insert_comment (InsertCommentData *data, gconstpointer service)
{
	set_up_query_files ((QueryFilesData*) data, service);

	/* Create a test comment to be inserted. */
	data->comment = gdata_picasaweb_comment_new (NULL);
	g_assert (GDATA_IS_PICASAWEB_COMMENT (data->comment));

	gdata_entry_set_content (GDATA_ENTRY (data->comment), "This is a test comment.");

	data->new_comment = NULL;
}

static void
tear_down_insert_comment (InsertCommentData *data, gconstpointer service)
{
	gdata_test_mock_server_start_trace (mock_server, "teardown-insert-comment");

	/* Delete the inserted comment. */
	if (data->new_comment != NULL) {
		g_assert (gdata_commentable_delete_comment (GDATA_COMMENTABLE (data->parent.file1), GDATA_SERVICE (service),
		                                            GDATA_COMMENT (data->new_comment), NULL, NULL) == TRUE);
		g_object_unref (data->new_comment);
	}

	if (data->comment != NULL) {
		g_object_unref (data->comment);
	}

	uhm_server_end_trace (mock_server);

	tear_down_query_files ((QueryFilesData*) data, service);
}

static void
assert_comments_equal (GDataComment *new_comment, GDataPicasaWebComment *original_comment)
{
	GList *authors;
	GDataAuthor *author;

	g_assert (GDATA_IS_PICASAWEB_COMMENT (new_comment));
	g_assert (GDATA_IS_PICASAWEB_COMMENT (original_comment));
	g_assert (GDATA_PICASAWEB_COMMENT (new_comment) != original_comment);

	g_assert_cmpstr (gdata_entry_get_content (GDATA_ENTRY (new_comment)), ==, gdata_entry_get_content (GDATA_ENTRY (original_comment)));

	/* Check the author of the new comment. */
	authors = gdata_entry_get_authors (GDATA_ENTRY (new_comment));
	g_assert_cmpuint (g_list_length (authors), ==, 1);

	author = GDATA_AUTHOR (authors->data);

	g_assert_cmpstr (gdata_author_get_name (author), ==, "libgdata.picasaweb");
	g_assert_cmpstr (gdata_author_get_uri (author), ==, "https://picasaweb.google.com/libgdata.picasaweb");
}

static void
test_comment_insert (InsertCommentData *data, gconstpointer service)
{
	GDataComment *new_comment;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "comment-insert");

	new_comment = gdata_commentable_insert_comment (GDATA_COMMENTABLE (data->parent.file1), GDATA_SERVICE (service), GDATA_COMMENT (data->comment),
	                                                NULL, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	assert_comments_equal (new_comment, data->comment);

	data->new_comment = GDATA_PICASAWEB_COMMENT (new_comment);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_CLOSURE_FUNCTIONS (insert_comment, InsertCommentData);

GDATA_ASYNC_TEST_FUNCTIONS (comment_insert, InsertCommentData,
G_STMT_START {
	gdata_commentable_insert_comment_async (GDATA_COMMENTABLE (data->parent.file1), GDATA_SERVICE (service),
	                                        GDATA_COMMENT (data->comment), cancellable, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataComment *new_comment;

	new_comment = gdata_commentable_insert_comment_finish (GDATA_COMMENTABLE (obj), async_result, &error);

	if (error == NULL) {
		assert_comments_equal (new_comment, data->comment);

		data->new_comment = GDATA_PICASAWEB_COMMENT (new_comment);
	} else {
		g_assert (new_comment == NULL);
	}
} G_STMT_END);

static void
test_comment_delete (QueryCommentsData *data, gconstpointer service)
{
	gboolean success;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "comment-delete");

	success = gdata_commentable_delete_comment (GDATA_COMMENTABLE (data->parent.file1), GDATA_SERVICE (service), GDATA_COMMENT (data->comment1),
	                                            NULL, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);
	g_clear_error (&error);

	g_object_unref (data->comment1);
	data->comment1 = NULL;

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_TEST_FUNCTIONS (comment_delete, QueryCommentsData,
G_STMT_START {
	gdata_commentable_delete_comment_async (GDATA_COMMENTABLE (data->parent.file1), GDATA_SERVICE (service),
	                                        GDATA_COMMENT (data->comment1), cancellable, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	gboolean success;

	success = gdata_commentable_delete_comment_finish (GDATA_COMMENTABLE (obj), async_result, &error);

	if (error == NULL) {
		g_assert (success == TRUE);

		/* Prevent the closure tear down function from trying to delete the comment again */
		g_object_unref (data->comment1);
		data->comment1 = NULL;
	} else {
		g_assert (success == FALSE);

		/* The server's naughty and often deletes comments even if the connection's closed prematurely (when we cancel the operation). In
		 * this case, it returns an error 400, which we sneakily hide. */
		if (g_error_matches (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR) == TRUE &&
		    async_data->cancellation_timeout > 0) {
			async_data->cancellation_successful = FALSE;
			g_clear_error (&error);
		}
	}
} G_STMT_END);

static void
test_query_user (gconstpointer service)
{
	GDataPicasaWebUser *user;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "query-user");

	user = gdata_picasaweb_service_get_user (GDATA_PICASAWEB_SERVICE (service), NULL, NULL, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	check_authenticated_user_details (user);

	g_object_unref (user);

	uhm_server_end_trace (mock_server);
}

/* Check that asynchronously querying for the currently authenticated user's details works and returns the correct details. */
GDATA_ASYNC_TEST_FUNCTIONS (query_user, void,
G_STMT_START {
	gdata_picasaweb_service_get_user_async (GDATA_PICASAWEB_SERVICE (service), NULL, cancellable, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataPicasaWebUser *user;

	user = gdata_picasaweb_service_get_user_finish (GDATA_PICASAWEB_SERVICE (obj), async_result, &error);

	if (error == NULL) {
		check_authenticated_user_details (user);

		g_object_unref (user);
	} else {
		g_assert (user == NULL);
	}
} G_STMT_END);

/* Check that querying for a user other than the currently authenticated user, asynchronously, gives us an appropriate result. This result should,
 * for example, not contain any private information about the queried user. (That's a server-side consideration, but libgdata has to handle the
 * lack of information correctly.) */
GDATA_ASYNC_TEST_FUNCTIONS (query_user_by_username, void,
G_STMT_START {
	gdata_picasaweb_service_get_user_async (GDATA_PICASAWEB_SERVICE (service), "philip.withnall", cancellable, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataPicasaWebUser *user;

	user = gdata_picasaweb_service_get_user_finish (GDATA_PICASAWEB_SERVICE (obj), async_result, &error);

	if (error == NULL) {
		g_assert (GDATA_IS_PICASAWEB_USER (user));

		g_assert_cmpstr (gdata_picasaweb_user_get_user (user), ==, "104200312198892774147");
		g_assert_cmpstr (gdata_picasaweb_user_get_nickname (user), ==, "Philip Withnall");
		g_assert_cmpint (gdata_picasaweb_user_get_quota_limit (user), ==, -1); /* not the logged in user */
		g_assert_cmpint (gdata_picasaweb_user_get_quota_current (user), ==, -1); /* not the logged in user */
		g_assert_cmpint (gdata_picasaweb_user_get_max_photos_per_album (user), ==, -1); /* not the logged in user */
		/* tested weakly to avoid having to update it regularly */
		g_assert_cmpstr (gdata_picasaweb_user_get_thumbnail_uri (user), !=, NULL);

		g_object_unref (user);
	} else {
		g_assert (user == NULL);
	}
} G_STMT_END);

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
set_up_upload (UploadData *data, gconstpointer service)
{
	GFileInfo *file_info;
	const gchar * const tags[] = { "foo", "bar", ",,baz,baz", NULL };
	gchar *path = NULL;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "setup-upload");

	data->service = g_object_ref ((gpointer) service);

	/* Build the photo */
	data->photo = gdata_picasaweb_file_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (data->photo), "Photo Entry Title");
	gdata_picasaweb_file_set_caption (data->photo, "Photo Summary");
	gdata_picasaweb_file_set_tags (data->photo, tags);
	gdata_picasaweb_file_set_coordinates (data->photo, 17.127, -110.35);

	/* File is public domain: http://en.wikipedia.org/wiki/File:German_garden_gnome_cropped.jpg */
	path = g_test_build_filename (G_TEST_DIST, "photo.jpg", NULL);
	data->photo_file = g_file_new_for_path (path);
	g_free (path);

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

	uhm_server_end_trace (mock_server);
}

static void
tear_down_upload (UploadData *data, gconstpointer service)
{
	gdata_test_mock_server_start_trace (mock_server, "teardown-upload");

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

	uhm_server_end_trace (mock_server);
}

static void
test_upload_default_album (UploadData *data, gconstpointer service)
{
	GDataUploadStream *upload_stream;
	const gchar * const *tags, * const *tags2;
	gssize transfer_size;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "upload-default-album");

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

	uhm_server_end_trace (mock_server);
}

#if 0
FIXME: Port to v3 API and re-enable.
GDATA_ASYNC_CLOSURE_FUNCTIONS (upload, UploadData);

GDATA_ASYNC_TEST_FUNCTIONS (upload_default_album, UploadData,
G_STMT_START {
	GDataUploadStream *upload_stream;
	GError *error = NULL;

	/* Prepare the upload stream */
	upload_stream = gdata_picasaweb_service_upload_file (GDATA_PICASAWEB_SERVICE (service), NULL, data->photo, data->slug,
	                                                     data->content_type, cancellable, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_UPLOAD_STREAM (upload_stream));

	/* Upload the photo asynchronously */
	g_output_stream_splice_async (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (data->file_stream),
	                              G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, G_PRIORITY_DEFAULT, NULL,
	                              async_ready_callback, async_data);

	g_object_unref (upload_stream);

	/* Reset the input stream to the beginning. */
	g_assert (g_seekable_seek (G_SEEKABLE (data->file_stream), 0, G_SEEK_SET, NULL, NULL) == TRUE);
} G_STMT_END,
G_STMT_START {
	GOutputStream *stream = G_OUTPUT_STREAM (obj);
	const gchar * const *tags;
	const gchar * const *tags2;
	gssize transfer_size;
	GError *upload_error = NULL;

	/* Finish off the transfer */
	transfer_size = g_output_stream_splice_finish (stream, async_result, &error);

	if (error == NULL) {
		g_assert_cmpint (transfer_size, >, 0);

		/* Finish off the upload */
		data->updated_photo = gdata_picasaweb_service_finish_file_upload (data->service, GDATA_UPLOAD_STREAM (stream), &upload_error);
		g_assert_no_error (upload_error);
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
	} else {
		g_assert_cmpint (transfer_size, ==, -1);

		/* Finish off the upload */
		data->updated_photo = gdata_picasaweb_service_finish_file_upload (data->service, GDATA_UPLOAD_STREAM (stream), &upload_error);
		g_assert_no_error (upload_error);
		g_assert (data->updated_photo == NULL);
	}

	g_clear_error (&upload_error);
} G_STMT_END);
#endif

static void
test_album_new (void)
{
	GDataPicasaWebAlbum *album;
	gchar *xml, *parsed_time_str;
	GRegex *regex;
	GMatchInfo *match_info;
	gint64 delta;
	GDateTime *timeval;

	g_test_bug ("598893");

	/* Get the current time */
	timeval = g_date_time_new_now_utc ();

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
	 * the test function. We can't check it exactly, as a few milliseconds may have passed between building the expected XML and building the XML
	 * for the photo. */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (album));
	g_assert (g_regex_match (regex, xml, 0, &match_info) == TRUE);
	parsed_time_str = g_match_info_fetch (match_info, 2);
	delta = g_ascii_strtoull (parsed_time_str, NULL, 10) - (g_date_time_to_unix (timeval) * 1000 + ((guint64) g_date_time_get_microsecond (timeval)) / 1000);
	g_assert_cmpuint (ABS (delta), <, 1000);

	g_date_time_unref (timeval);
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
test_comment_get_xml (void)
{
	GDataPicasaWebComment *comment_;

	comment_ = gdata_picasaweb_comment_new (NULL);
	gdata_entry_set_content (GDATA_ENTRY (comment_), "This is a comment with <markup> & stüff.");

	/* Check the outputted XML is OK */
	gdata_test_assert_xml (comment_,
		"<?xml version='1.0' encoding='UTF-8'?>"
		"<entry xmlns='http://www.w3.org/2005/Atom' xmlns:gd='http://schemas.google.com/g/2005'>"
			"<title type='text'></title>"
			"<content type='text'>This is a comment with &lt;markup&gt; &amp; stüff.</content>"
			"<category term='http://schemas.google.com/photos/2007#comment' scheme='http://schemas.google.com/g/2005#kind'/>"
		"</entry>");

	g_object_unref (comment_);
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

static void
mock_server_notify_resolver_cb (GObject *object, GParamSpec *pspec, gpointer user_data)
{
	UhmServer *server;
	UhmResolver *resolver;

	server = UHM_SERVER (object);

	/* Set up the expected domain names here. This should technically be split up between
	 * the different unit test suites, but that's too much effort. */
	resolver = uhm_server_get_resolver (server);

	if (resolver != NULL) {
		const gchar *ip_address = uhm_server_get_address (server);

		uhm_resolver_add_A (resolver, "www.google.com", ip_address);
		uhm_resolver_add_A (resolver, "picasaweb.google.com", ip_address);
		uhm_resolver_add_A (resolver, "lh3.googleusercontent.com", ip_address);
		uhm_resolver_add_A (resolver, "lh5.googleusercontent.com", ip_address);
		uhm_resolver_add_A (resolver, "lh6.googleusercontent.com", ip_address);
	}
}

/* Set up a global GDataAuthorizer to be used for all the tests. Unfortunately,
 * the Google PicasaWeb API is limited to OAuth2 authorisation, so
 * this requires user interaction when online.
 *
 * If not online, use a dummy authoriser. */
static GDataAuthorizer *
create_global_authorizer (void)
{
	GDataOAuth2Authorizer *authorizer = NULL;  /* owned */
	gchar *authentication_uri, *authorisation_code;
	GError *error = NULL;

	/* If not online, just return a dummy authoriser. */
	if (!uhm_server_get_enable_online (mock_server)) {
		return GDATA_AUTHORIZER (gdata_dummy_authorizer_new (GDATA_TYPE_PICASAWEB_SERVICE));
	}

	/* Otherwise, go through the interactive OAuth dance. */
	gdata_test_mock_server_start_trace (mock_server, "global-authentication");
	authorizer = gdata_oauth2_authorizer_new (CLIENT_ID, CLIENT_SECRET,
	                                          REDIRECT_URI,
	                                          GDATA_TYPE_PICASAWEB_SERVICE);

	/* Get an authentication URI */
	authentication_uri = gdata_oauth2_authorizer_build_authentication_uri (authorizer, NULL, FALSE);
	g_assert (authentication_uri != NULL);

	/* Get the authorisation code off the user. */
	authorisation_code = gdata_test_query_user_for_verifier (authentication_uri);

	g_free (authentication_uri);

	if (authorisation_code == NULL) {
		/* Skip tests. */
		g_object_unref (authorizer);
		authorizer = NULL;
		goto skip_test;
	}

	/* Authorise the token */
	g_assert (gdata_oauth2_authorizer_request_authorization (authorizer, authorisation_code, NULL, &error));
	g_assert_no_error (error);

skip_test:
	g_free (authorisation_code);

	uhm_server_end_trace (mock_server);

	return GDATA_AUTHORIZER (authorizer);
}

int
main (int argc, char *argv[])
{
	gint retval;
	GDataAuthorizer *authorizer = NULL;
	GDataService *service = NULL;
	GFile *trace_directory;
	gchar *path = NULL;

	gdata_test_init (argc, argv);

	mock_server = gdata_test_get_mock_server ();
	g_signal_connect (G_OBJECT (mock_server), "notify::resolver", (GCallback) mock_server_notify_resolver_cb, NULL);
	path = g_test_build_filename (G_TEST_DIST, "traces/picasaweb", NULL);
	trace_directory = g_file_new_for_path (path);
	g_free (path);
	uhm_server_set_trace_directory (mock_server, trace_directory);
	g_object_unref (trace_directory);

	authorizer = create_global_authorizer ();

	service = GDATA_SERVICE (gdata_picasaweb_service_new (authorizer));

	g_test_add_func ("/picasaweb/authentication", test_authentication);

	g_test_add ("/picasaweb/query/all_albums", QueryAllAlbumsData, service, set_up_query_all_albums, test_query_all_albums,
	            tear_down_query_all_albums);
	g_test_add ("/picasaweb/query/all_albums/with_limits", QueryAllAlbumsData, service, set_up_query_all_albums,
	            test_query_all_albums_with_limits, tear_down_query_all_albums);
	g_test_add ("/picasaweb/query/all_albums/async", GDataAsyncTestData, service, set_up_query_all_albums_async,
	            test_query_all_albums_async, tear_down_query_all_albums_async);
	g_test_add ("/picasaweb/query/all_albums/async/progress_closure", QueryAllAlbumsData, service, set_up_query_all_albums,
	            test_query_all_albums_async_progress_closure, tear_down_query_all_albums);
	g_test_add ("/picasaweb/query/all_albums/async/cancellation", GDataAsyncTestData, service, set_up_query_all_albums_async,
	            test_query_all_albums_async_cancellation, tear_down_query_all_albums_async);
	g_test_add_data_func ("/picasaweb/query/all_albums/bad_query", service, test_query_all_albums_bad_query);
	g_test_add_data_func ("/picasaweb/query/all_albums/bad_query/with_limits", service, test_query_all_albums_bad_query_with_limits);
	g_test_add_data_func ("/picasaweb/query/user", service, test_query_user);
	g_test_add ("/picasaweb/query/user/async", GDataAsyncTestData, service, gdata_set_up_async_test_data, test_query_user_async,
	            gdata_tear_down_async_test_data);
	g_test_add ("/picasaweb/query/user/async/cancellation", GDataAsyncTestData, service, gdata_set_up_async_test_data,
	            test_query_user_async_cancellation, gdata_tear_down_async_test_data);
	g_test_add ("/picasaweb/query/user/by-username/async", GDataAsyncTestData, service, gdata_set_up_async_test_data,
	            test_query_user_by_username_async, gdata_tear_down_async_test_data);
	g_test_add ("/picasaweb/query/user/by-username/async/cancellation", GDataAsyncTestData, service, gdata_set_up_async_test_data,
	            test_query_user_by_username_async_cancellation, gdata_tear_down_async_test_data);

	g_test_add ("/picasaweb/insert/album", InsertAlbumData, service, set_up_insert_album, test_insert_album, tear_down_insert_album);
	g_test_add ("/picasaweb/insert/album/async", GDataAsyncTestData, service, set_up_insert_album_async, test_insert_album_async,
	            tear_down_insert_album_async);
	g_test_add ("/picasaweb/insert/album/async/cancellation", GDataAsyncTestData, service, set_up_insert_album_async,
	            test_insert_album_async_cancellation, tear_down_insert_album_async);

	g_test_add ("/picasaweb/query/files", QueryFilesData, service, set_up_query_files, test_query_files, tear_down_query_files);
	g_test_add ("/picasaweb/query/files/async", GDataAsyncTestData, service, set_up_query_files_async, test_query_files_async,
	            tear_down_query_files_async);
	g_test_add ("/picasaweb/query/files/async/progress_closure", QueryFilesData, service, set_up_query_files,
	            test_query_files_async_progress_closure, tear_down_query_files);
	g_test_add ("/picasaweb/query/files/async/cancellation", GDataAsyncTestData, service, set_up_query_files_async,
	            test_query_files_async_cancellation, tear_down_query_files_async);
	g_test_add ("/picasaweb/query/files/single", QueryFilesData, service, set_up_query_files, test_query_files_single,
	            tear_down_query_files);

	g_test_add ("/picasaweb/comment/query", QueryCommentsData, service, set_up_query_comments, test_comment_query,
	            tear_down_query_comments);
	g_test_add ("/picasaweb/comment/query/async", GDataAsyncTestData, service, set_up_query_comments_async, test_comment_query_async,
	            tear_down_query_comments_async);
	g_test_add ("/picasaweb/comment/query/async/cancellation", GDataAsyncTestData, service, set_up_query_comments_async,
	            test_comment_query_async_cancellation, tear_down_query_comments_async);
	g_test_add ("/picasaweb/comment/query/progress_closure", QueryCommentsData, service, set_up_query_comments,
	            test_comment_query_async_progress_closure, tear_down_query_comments);

	g_test_add ("/picasaweb/comment/insert", InsertCommentData, service, set_up_insert_comment, test_comment_insert,
	            tear_down_insert_comment);
	g_test_add ("/picasaweb/comment/insert/async", GDataAsyncTestData, service, set_up_insert_comment_async, test_comment_insert_async,
	            tear_down_insert_comment_async);
	g_test_add ("/picasaweb/comment/insert/async/cancellation", GDataAsyncTestData, service, set_up_insert_comment_async,
	            test_comment_insert_async_cancellation, tear_down_insert_comment_async);

	g_test_add ("/picasaweb/comment/delete", QueryCommentsData, service, set_up_query_comments, test_comment_delete,
	            tear_down_query_comments);
	g_test_add ("/picasaweb/comment/delete/async", GDataAsyncTestData, service, set_up_query_comments_async, test_comment_delete_async,
	            tear_down_query_comments_async);
	g_test_add ("/picasaweb/comment/delete/async/cancellation", GDataAsyncTestData, service, set_up_query_comments_async,
	            test_comment_delete_async_cancellation, tear_down_query_comments_async);

	g_test_add ("/picasaweb/upload/default_album", UploadData, service, set_up_upload, test_upload_default_album, tear_down_upload);
	/*g_test_add ("/picasaweb/upload/default_album/async", GDataAsyncTestData, service, set_up_upload_async, test_upload_default_album_async,
	            tear_down_upload_async);
	g_test_add ("/picasaweb/upload/default_album/async/cancellation", GDataAsyncTestData, service, set_up_upload_async,
	            test_upload_default_album_async_cancellation, tear_down_upload_async);*/

	g_test_add ("/picasaweb/download/photo", QueryFilesData, service, set_up_query_files, test_download_photo, tear_down_query_files);
	g_test_add ("/picasaweb/download/thumbnails", QueryFilesData, service, set_up_query_files, test_download_thumbnails,
	            tear_down_query_files);

	g_test_add_func ("/picasaweb/album/new", test_album_new);
	g_test_add_func ("/picasaweb/album/escaping", test_album_escaping);
	g_test_add_func ("/picasaweb/album/properties/coordinates", test_album_properties_coordinates);
	g_test_add_func ("/picasaweb/album/properties/visibility", test_album_properties_visibility);

	g_test_add_func ("/picasaweb/file/escaping", test_file_escaping);
	g_test_add_func ("/picasaweb/file/properties/coordinates", test_file_properties_coordinates);

	g_test_add_func ("/picasaweb/comment/get_xml", test_comment_get_xml);

	g_test_add_func ("/picasaweb/query/etag", test_query_etag);

	retval = g_test_run ();

	if (service != NULL)
		g_object_unref (service);

	return retval;
}
