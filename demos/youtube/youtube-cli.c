/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) 2015 Philip Withnall <philip@tecnocode.co.uk>
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

#include <gdata/gdata.h>
#include <locale.h>
#include <string.h>

#define CLIENT_ID "1074795795536-necvslvs0pchk65nf6ju4i6mniogg8fr.apps.googleusercontent.com"
#define CLIENT_SECRET "8totRi50eo2Zfr3SD2DeNAzo"
#define REDIRECT_URI "urn:ietf:wg:oauth:2.0:oob"
#define DEVELOPER_KEY "AIzaSyCENhl8yDxDZbyhTF6p-ok-RefK07xdXUg"

static int
print_usage (char *argv[])
{
	g_printerr ("%s: Usage — %s <subcommand>\n"
	            "Subcommands:\n"
	            "   search <query string>\n"
	            "   info <video ID>\n"
	            "   standard-feed <feed name>\n"
	            "   categories\n"
	            "   related <video ID>\n"
	            "   upload <filename> <title> [description]\n",
	            argv[0], argv[0]);
	return -1;
}

static void
print_video (GDataYouTubeVideo *video)
{
	const gchar *title, *player_uri, *id, *description;
	GList/*<unowned GDataMediaThumbnail>*/ *thumbnails;
	GDateTime *tmp;
	gint64 date_published_tv;
	gchar *date_published = NULL;  /* owned */
	guint duration;  /* seconds */
	guint rating_min = 0, rating_max = 0, rating_count = 0;
	gdouble rating_average = 0.0;

	title = gdata_entry_get_title (GDATA_ENTRY (video));
	player_uri = gdata_youtube_video_get_player_uri (video);
	id = gdata_entry_get_id (GDATA_ENTRY (video));
	description = gdata_youtube_video_get_description (video);
	thumbnails = gdata_youtube_video_get_thumbnails (video);
	date_published_tv = gdata_entry_get_published (GDATA_ENTRY (video));
	tmp = g_date_time_new_from_unix_utc (date_published_tv);
	date_published = g_date_time_format_iso8601 (tmp);
	g_date_time_unref (tmp);
	duration = gdata_youtube_video_get_duration (video);
	gdata_youtube_video_get_rating (video, &rating_min, &rating_max,
	                                &rating_count, &rating_average);

	g_print ("%s — %s\n", player_uri, title);
	g_print ("   ID: %s\n", id);
	g_print ("   Published: %s\n", date_published);
	g_print ("   Duration: %us\n", duration);
	g_print ("   Rating: %.2f (min: %u, max: %u, count: %u)\n",
	         rating_average, rating_min, rating_max, rating_count);
	g_print ("   Description:\n      %s\n", description);
	g_print ("   Thumbnails:\n");

	for (; thumbnails != NULL; thumbnails = thumbnails->next) {
		GDataMediaThumbnail *thumbnail;

		thumbnail = GDATA_MEDIA_THUMBNAIL (thumbnails->data);
		g_print ("    • %s\n",
		         gdata_media_thumbnail_get_uri (thumbnail));
	}

	g_print ("\n");

	g_free (date_published);
}

static void
print_category (GDataCategory *category)
{
	const gchar *term, *label;

	term = gdata_category_get_term (category);
	label = gdata_category_get_label (category);

	g_print ("%s — %s\n", term, label);
}

static GDataAuthorizer *
create_authorizer (GError **error)
{
	GDataOAuth2Authorizer *authorizer = NULL;  /* owned */
	gchar *uri = NULL;
	gchar code[100];
	GError *child_error = NULL;

	/* Go through the interactive OAuth dance. */
	authorizer = gdata_oauth2_authorizer_new (CLIENT_ID, CLIENT_SECRET,
	                                          REDIRECT_URI,
	                                          GDATA_TYPE_YOUTUBE_SERVICE);

	/* Get an authentication URI */
	uri = gdata_oauth2_authorizer_build_authentication_uri (authorizer,
	                                                        NULL, FALSE);

	/* Wait for the user to retrieve and enter the verifier. */
	g_print ("Please navigate to the following URI and grant access:\n"
	         "   %s\n", uri);
	g_print ("Enter verifier (EOF to abort): ");

	g_free (uri);

	if (scanf ("%100s", code) != 1) {
		/* User chose to abort. */
		g_print ("\n");
		g_clear_object (&authorizer);
		return NULL;
	}

	/* Authorise the token. */
	gdata_oauth2_authorizer_request_authorization (authorizer, code, NULL,
	                                               &child_error);

	if (child_error != NULL) {
		g_propagate_error (error, child_error);
		g_clear_object (&authorizer);
		return NULL;
	}

	return GDATA_AUTHORIZER (authorizer);
}

/* Search for videos given a simple query string. */
static int
command_search (int argc, char *argv[])
{
	GDataYouTubeService *service = NULL;
	GDataYouTubeQuery *query = NULL;
	GDataFeed *feed = NULL;
	GList/*<unowned GDataYouTubeVideo>*/ *entries;
	GError *error = NULL;
	gint retval = 0;
	const gchar *query_string;

	if (argc < 3) {
		return print_usage (argv);
	}

	query_string = argv[2];

	service = gdata_youtube_service_new (DEVELOPER_KEY, NULL);
	query = gdata_youtube_query_new (query_string);
	feed = gdata_youtube_service_query_videos (service, GDATA_QUERY (query),
	                                           NULL, NULL, NULL, &error);

	if (error != NULL) {
		g_printerr ("%s: Error querying YouTube: %s\n",
		            argv[0], error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	}

	/* Print results. */
	for (entries = gdata_feed_get_entries (feed); entries != NULL;
	     entries = entries->next) {
		GDataYouTubeVideo *video;

		video = GDATA_YOUTUBE_VIDEO (entries->data);
		print_video (video);
	}

	g_print ("Total of %u results.\n", gdata_feed_get_total_results (feed));

done:
	g_clear_object (&feed);
	g_clear_object (&query);
	g_clear_object (&service);

	return retval;
}

/* Display information about a single video. */
static int
command_info (int argc, char *argv[])
{
	GDataYouTubeService *service = NULL;
	GDataEntry *result = NULL;
	GDataYouTubeVideo *video;  /* unowned */
	GError *error = NULL;
	gint retval = 0;
	const gchar *entry_id;

	if (argc < 3) {
		return print_usage (argv);
	}

	entry_id = argv[2];

	service = gdata_youtube_service_new (DEVELOPER_KEY, NULL);
	result = gdata_service_query_single_entry (GDATA_SERVICE (service),
	                                           NULL, entry_id, NULL,
	                                           GDATA_TYPE_YOUTUBE_VIDEO,
	                                           NULL, &error);

	if (error != NULL) {
		g_printerr ("%s: Error querying YouTube: %s\n",
		            argv[0], error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	}

	/* Print results. */
	video = GDATA_YOUTUBE_VIDEO (result);
	print_video (video);

done:
	g_clear_object (&result);
	g_clear_object (&service);

	return retval;
}

static gboolean
standard_feed_type_from_name (const gchar *name,
                              GDataYouTubeStandardFeedType *out)
{
	/* Indexed by GDataYouTubeStandardFeedType. */
	const gchar *feed_type_names[] = {
		"most-popular",
	};
	guint i;

	for (i = 0; i < G_N_ELEMENTS (feed_type_names); i++) {
		if (g_strcmp0 (feed_type_names[i], name) == 0) {
			*out = (GDataYouTubeStandardFeedType) i;
			return TRUE;
		}
	}

	return FALSE;
}

/* List all videos in a standard feed. */
static int
command_standard_feed (int argc, char *argv[])
{
	GDataYouTubeService *service = NULL;
	GDataFeed *feed = NULL;
	GList/*<unowned GDataYouTubeVideo>*/ *entries;
	GError *error = NULL;
	gint retval = 0;
	GDataYouTubeStandardFeedType feed_type;

	if (argc < 3) {
		return print_usage (argv);
	}

	if (!standard_feed_type_from_name (argv[2], &feed_type)) {
		g_printerr ("%s: Invalid feed type ‘%s’.\n", argv[0], argv[2]);
		retval = 1;
		goto done;
	}

	service = gdata_youtube_service_new (DEVELOPER_KEY, NULL);
	feed = gdata_youtube_service_query_standard_feed (service, feed_type,
	                                                  NULL, NULL, NULL,
	                                                  NULL, &error);

	if (error != NULL) {
		g_printerr ("%s: Error querying YouTube: %s\n",
		            argv[0], error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	}

	/* Print results. */
	for (entries = gdata_feed_get_entries (feed); entries != NULL;
	     entries = entries->next) {
		GDataYouTubeVideo *video;

		video = GDATA_YOUTUBE_VIDEO (entries->data);
		print_video (video);
	}

	g_print ("Total of %u results.\n", gdata_feed_get_total_results (feed));

done:
	g_clear_object (&feed);
	g_clear_object (&service);

	return retval;
}

/* List videos related to a given one. */
static int
command_related (int argc, char *argv[])
{
	GDataYouTubeService *service = NULL;
	GDataFeed *feed = NULL;
	GList/*<unowned GDataYouTubeVideo>*/ *entries;
	GError *error = NULL;
	gint retval = 0;
	const gchar *entry_id;
	GDataYouTubeVideo *query_video = NULL;

	if (argc < 3) {
		return print_usage (argv);
	}

	entry_id = argv[2];
	query_video = gdata_youtube_video_new (entry_id);

	service = gdata_youtube_service_new (DEVELOPER_KEY, NULL);
	feed = gdata_youtube_service_query_related (service, query_video, NULL,
	                                            NULL, NULL, NULL, &error);

	if (error != NULL) {
		g_printerr ("%s: Error querying YouTube: %s\n",
		            argv[0], error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	}

	/* Print results. */
	for (entries = gdata_feed_get_entries (feed); entries != NULL;
	     entries = entries->next) {
		GDataYouTubeVideo *video;

		video = GDATA_YOUTUBE_VIDEO (entries->data);
		print_video (video);
	}

	g_print ("Total of %u results.\n", gdata_feed_get_total_results (feed));

done:
	g_clear_object (&query_video);
	g_clear_object (&feed);
	g_clear_object (&service);

	return retval;
}

/* List all available video categories. */
static int
command_categories (int argc, char *argv[])
{
	GDataYouTubeService *service = NULL;
	GDataAPPCategories *app_categories = NULL;
	GList/*<unowned GDataCategory>*/ *categories;
	GError *error = NULL;
	gint retval = 0;

	service = gdata_youtube_service_new (DEVELOPER_KEY, NULL);
	app_categories = gdata_youtube_service_get_categories (service, NULL,
	                                                       &error);

	if (error != NULL) {
		g_printerr ("%s: Error querying YouTube: %s\n",
		            argv[0], error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	}

	/* Print results. */
	for (categories = gdata_app_categories_get_categories (app_categories);
	     categories != NULL;
	     categories = categories->next) {
		GDataCategory *category;

		category = GDATA_CATEGORY (categories->data);
		print_category (category);
	}

	g_print ("Total of %u results.\n",
	         g_list_length (gdata_app_categories_get_categories (app_categories)));

done:
	g_clear_object (&app_categories);
	g_clear_object (&service);

	return retval;
}

/* Upload a video. */
static int
command_upload (int argc, char *argv[])
{
	GDataYouTubeService *service = NULL;
	GDataUploadStream *upload_stream = NULL;
	GError *error = NULL;
	gint retval = 0;
	const gchar *filename;
	GFile *video_file = NULL;
	GFileInputStream *video_file_stream = NULL;
	GFileInfo *video_file_info = NULL;
	GDataYouTubeVideo *video = NULL;
	GDataYouTubeVideo *uploaded_video = NULL;
	gssize transfer_size;
	const gchar *content_type, *slug;
	GDataAuthorizer *authorizer = NULL;
	const gchar *title, *description;

	if (argc < 3) {
		return print_usage (argv);
	}

	filename = argv[2];
	title = (argc > 3) ? argv[3] : NULL;
	description = (argc > 4) ? argv[4] : NULL;

	/* Load the file and query its details. */
	video_file = g_file_new_for_commandline_arg (filename);

	video_file_info = g_file_query_info (video_file,
	                                     G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
	                                     G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
	                                     G_FILE_QUERY_INFO_NONE, NULL,
	                                     &error);

	if (error != NULL) {
		g_printerr ("%s: Error loading video information ‘%s’: %s\n",
		            argv[0], filename, error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	}

	content_type = g_file_info_get_content_type (video_file_info);
	slug = g_file_info_get_display_name (video_file_info);

	video_file_stream = g_file_read (video_file, NULL, &error);

	if (error != NULL) {
		g_printerr ("%s: Error loading video ‘%s’: %s\n",
		            argv[0], filename, error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	}

	/* Build the video. */
	video = gdata_youtube_video_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (video), title);
	gdata_entry_set_summary (GDATA_ENTRY (video), description);

	/* Authenticate and create a service. */
	authorizer = create_authorizer (&error);

	if (error != NULL) {
		g_printerr ("%s: Error authenticating: %s\n",
		            argv[0], error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	} else if (authorizer == NULL) {
		g_printerr ("%s: User chose to abort authentication.\n",
		            argv[0]);
		retval = 1;
		goto done;
	}

	service = gdata_youtube_service_new (DEVELOPER_KEY,
	                                     GDATA_AUTHORIZER (authorizer));

	/* Start the upload. */
	upload_stream = gdata_youtube_service_upload_video (service, video,
	                                                    slug, content_type,
	                                                    NULL, &error);

	if (error != NULL) {
		g_printerr ("%s: Error initializing upload with YouTube: %s\n",
		            argv[0], error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	}

	/* Upload the video */
	transfer_size = g_output_stream_splice (G_OUTPUT_STREAM (upload_stream),
	                                        G_INPUT_STREAM (video_file_stream),
	                                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE |
	                                        G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
	                                        NULL, &error);

	if (error != NULL) {
		g_printerr ("%s: Error transferring file: %s\n",
		            argv[0], error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	}

	/* Finish off the upload */
	uploaded_video = gdata_youtube_service_finish_video_upload (service,
	                                                            upload_stream,
	                                                            &error);

	if (error != NULL) {
		g_printerr ("%s: Error finishing upload with YouTube: %s\n",
		            argv[0], error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	}

	/* Print the uploaded video as confirmation. */
	g_print ("Uploaded %" G_GSSIZE_FORMAT " bytes.\n", transfer_size);
	print_video (uploaded_video);

done:
	g_clear_object (&authorizer);
	g_clear_object (&uploaded_video);
	g_clear_object (&video);
	g_clear_object (&video_file_info);
	g_clear_object (&video_file_stream);
	g_clear_object (&video_file);
	g_clear_object (&upload_stream);
	g_clear_object (&service);

	return retval;
}

static const struct {
	const gchar *command;
	int (*handler_fn) (int argc, char **argv);
} command_handlers[] = {
	{ "search", command_search },
	{ "info", command_info },
	{ "standard-feed", command_standard_feed },
	{ "categories", command_categories },
	{ "related", command_related },
	{ "upload", command_upload },
};

int
main (int argc, char *argv[])
{
	guint i;
	gint retval = -1;

	setlocale (LC_ALL, "");

	if (argc < 2) {
		return print_usage (argv);
	}

	for (i = 0; i < G_N_ELEMENTS (command_handlers); i++) {
		if (strcmp (argv[1], command_handlers[i].command) == 0) {
			retval = command_handlers[i].handler_fn (argc, argv);
		}
	}

	if (retval == -1) {
		retval = print_usage (argv);
	}

	return retval;
}
