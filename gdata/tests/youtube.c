/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2008–2010, 2015 <philip@tecnocode.co.uk>
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
#include <string.h>
#include <unistd.h>

#include "gdata.h"
#include "common.h"
#include "gdata-dummy-authorizer.h"

static UhmServer *mock_server = NULL;

#undef CLIENT_ID  /* from common.h */

#define DEVELOPER_KEY "AI39si7Me3Q7zYs6hmkFvpRBD2nrkVjYYsUO5lh_3HdOkGRc9g6Z4nzxZatk_aAo2EsA21k7vrda0OO6oFg2rnhMedZXPyXoEw"
#define CLIENT_ID "352818697630-nqu2cmt5quqd6lr17ouoqmb684u84l1f.apps.googleusercontent.com"
#define CLIENT_SECRET "-fA4pHQJxR3zJ-FyAMPQsikg"
#define REDIRECT_URI "urn:ietf:wg:oauth:2.0:oob"

/* Effectively gdata_test_mock_server_start_trace() but calling uhm_server_run() instead of uhm_server_start_trace(). */
static void
gdata_test_mock_server_run (UhmServer *server)
{
	const gchar *ip_address;
	UhmResolver *resolver;

	uhm_server_run (server);
	gdata_test_set_https_port (server);

	if (uhm_server_get_enable_online (server) == FALSE) {
		/* Set up the expected domain names here. This should technically be split up between
		 * the different unit test suites, but that's too much effort. */
		ip_address = uhm_server_get_address (server);
		resolver = uhm_server_get_resolver (server);

		uhm_resolver_add_A (resolver, "www.google.com", ip_address);
		uhm_resolver_add_A (resolver, "gdata.youtube.com", ip_address);
		uhm_resolver_add_A (resolver, "uploads.gdata.youtube.com", ip_address);
	}
}

static void
test_authentication (void)
{
	GDataOAuth2Authorizer *authorizer = NULL;  /* owned */
	gchar *authentication_uri, *authorisation_code;

	gdata_test_mock_server_start_trace (mock_server, "authentication");

	authorizer = gdata_oauth2_authorizer_new (CLIENT_ID, CLIENT_SECRET,
	                                          REDIRECT_URI,
	                                          GDATA_TYPE_YOUTUBE_SERVICE);

	/* Get an authentication URI. */
	authentication_uri = gdata_oauth2_authorizer_build_authentication_uri (authorizer, NULL, FALSE);
	g_assert (authentication_uri != NULL);

	/* Get the authorisation code off the user. */
	if (uhm_server_get_enable_online (mock_server)) {
		authorisation_code = gdata_test_query_user_for_verifier (authentication_uri);
	} else {
		/* Hard coded, extracted from the trace file. */
		authorisation_code = g_strdup ("4/9qV_LanNEOUOL4sftMwUp4cfa_yeF"
		                               "assB6-ys5EkA5o.4rgOzrZMXgcboiIB"
		                               "eO6P2m-GWLMXmgI");
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
	                                                     gdata_youtube_service_get_primary_authorization_domain ()) == TRUE);

skip_test:
	g_free (authorisation_code);
	g_object_unref (authorizer);

	uhm_server_end_trace (mock_server);
}

static void
test_service_properties (void)
{
	GDataService *service;

	/* Create a service */
	service = GDATA_SERVICE (gdata_youtube_service_new (DEVELOPER_KEY, NULL));

	g_assert (service != NULL);
	g_assert (GDATA_IS_SERVICE (service));
	g_assert_cmpstr (gdata_youtube_service_get_developer_key (GDATA_YOUTUBE_SERVICE (service)), ==, DEVELOPER_KEY);

	g_object_unref (service);
}

static void
test_query_standard_feeds (gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;
	guint i;
	gulong filter_id;
	GDataYouTubeStandardFeedType feeds[] = {
		/* This must be kept up-to-date with GDataYouTubeStandardFeedType. */
		GDATA_YOUTUBE_TOP_RATED_FEED,
		GDATA_YOUTUBE_TOP_FAVORITES_FEED,
		GDATA_YOUTUBE_MOST_VIEWED_FEED,
		GDATA_YOUTUBE_MOST_POPULAR_FEED,
		GDATA_YOUTUBE_MOST_RECENT_FEED,
		GDATA_YOUTUBE_MOST_DISCUSSED_FEED,
		GDATA_YOUTUBE_MOST_LINKED_FEED,
		GDATA_YOUTUBE_MOST_RESPONDED_FEED,
		GDATA_YOUTUBE_RECENTLY_FEATURED_FEED,
		GDATA_YOUTUBE_WATCH_ON_MOBILE_FEED,
	};
	const gchar *ignore_query_param_values[] = {
		"publishedAfter",
		NULL,
	};

	filter_id = uhm_server_filter_ignore_parameter_values (mock_server,
	                                                       ignore_query_param_values);
	gdata_test_mock_server_start_trace (mock_server, "query-standard-feeds");

	for (i = 0; i < G_N_ELEMENTS (feeds); i++) {
		feed = gdata_youtube_service_query_standard_feed (GDATA_YOUTUBE_SERVICE (service), feeds[i], NULL, NULL, NULL, NULL, &error);
		g_assert_no_error (error);
		g_assert (GDATA_IS_FEED (feed));
		g_clear_error (&error);

		g_assert_cmpuint (g_list_length (gdata_feed_get_entries (feed)), >, 0);

		g_object_unref (feed);
	}

	uhm_server_end_trace (mock_server);
	uhm_server_compare_messages_remove_filter (mock_server, filter_id);
}

static void
test_query_standard_feed (gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;
	gulong filter_id;
	const gchar *ignore_query_param_values[] = {
		"publishedAfter",
		NULL,
	};

	filter_id = uhm_server_filter_ignore_parameter_values (mock_server,
	                                                       ignore_query_param_values);
	gdata_test_mock_server_start_trace (mock_server, "query-standard-feed");

	feed = gdata_youtube_service_query_standard_feed (GDATA_YOUTUBE_SERVICE (service), GDATA_YOUTUBE_TOP_RATED_FEED, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	g_assert_cmpuint (g_list_length (gdata_feed_get_entries (feed)), >, 0);

	g_object_unref (feed);

	uhm_server_end_trace (mock_server);
	uhm_server_compare_messages_remove_filter (mock_server, filter_id);
}

static void
test_query_standard_feed_with_query (gconstpointer service)
{
	GDataYouTubeQuery *query;
	GDataFeed *feed;
	GError *error = NULL;
	gulong filter_id;
	const gchar *ignore_query_param_values[] = {
		"publishedAfter",
		NULL,
	};

	filter_id = uhm_server_filter_ignore_parameter_values (mock_server,
	                                                       ignore_query_param_values);

	G_GNUC_BEGIN_IGNORE_DEPRECATIONS

	gdata_test_mock_server_start_trace (mock_server, "query-standard-feed-with-query");

	query = gdata_youtube_query_new (NULL);
	gdata_youtube_query_set_language (query, "fr");

	feed = gdata_youtube_service_query_standard_feed (GDATA_YOUTUBE_SERVICE (service), GDATA_YOUTUBE_TOP_RATED_FEED, GDATA_QUERY (query), NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	g_assert_cmpuint (g_list_length (gdata_feed_get_entries (feed)), >, 0);

	g_object_unref (query);
	g_object_unref (feed);

	uhm_server_end_trace (mock_server);
	uhm_server_compare_messages_remove_filter (mock_server, filter_id);

	G_GNUC_END_IGNORE_DEPRECATIONS
}

/* HTTP message responses and the expected associated GData error domain/code. */
static const GDataTestRequestErrorData query_standard_feed_errors[] = {
	/* Generic network errors. */
	{ SOUP_STATUS_BAD_REQUEST, "Bad Request", "Invalid parameter ‘foobar’.",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_PROTOCOL_ERROR },
	{ SOUP_STATUS_NOT_FOUND, "Not Found", "Login page wasn't found for no good reason at all.",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_NOT_FOUND },
	{ SOUP_STATUS_PRECONDITION_FAILED, "Precondition Failed", "Not allowed to log in at this time, possibly.",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_CONFLICT },
	{ SOUP_STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error", "Whoops.",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_PROTOCOL_ERROR },
#if 0
FIXME
	/* Specific query errors. */
	{ SOUP_STATUS_FORBIDDEN, "Too Many Calls",
	  "<?xml version='1.0' encoding='UTF-8'?><errors><error><domain>yt:quota</domain><code>too_many_recent_calls</code></error></errors>",
	  gdata_youtube_service_error_quark, GDATA_YOUTUBE_SERVICE_ERROR_API_QUOTA_EXCEEDED },
	{ SOUP_STATUS_FORBIDDEN, "Too Many Entries",
	  "<?xml version='1.0' encoding='UTF-8'?><errors><error><domain>yt:quota</domain><code>too_many_entries</code></error></errors>",
	  gdata_youtube_service_error_quark, GDATA_YOUTUBE_SERVICE_ERROR_ENTRY_QUOTA_EXCEEDED },
	{ SOUP_STATUS_SERVICE_UNAVAILABLE, "Maintenance",
	  "<?xml version='1.0' encoding='UTF-8'?><errors><error><domain>yt:service</domain><code>disabled_in_maintenance_mode</code></error></errors>",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_UNAVAILABLE },
	{ SOUP_STATUS_FORBIDDEN, "YouTube Signup Required",
	  "<?xml version='1.0' encoding='UTF-8'?><errors><error><domain>yt:service</domain><code>youtube_signup_required</code></error></errors>",
	  gdata_youtube_service_error_quark, GDATA_YOUTUBE_SERVICE_ERROR_CHANNEL_REQUIRED },
	{ SOUP_STATUS_FORBIDDEN, "Forbidden",
	  "<?xml version='1.0' encoding='UTF-8'?><errors><error><domain>yt:authentication</domain><code>TokenExpired</code>"
	  "<location type='header'>Authorization: GoogleLogin</location></error></errors>",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED },
	/* Malformed YouTube errors to test parser error handling. */
	{ SOUP_STATUS_INTERNAL_SERVER_ERROR, "Malformed XML",
	  "<?xml version='1.0' encoding='UTF-8'?><errors>",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_PROTOCOL_ERROR },
	{ SOUP_STATUS_FORBIDDEN, "Empty Response", "",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED },
	{ SOUP_STATUS_INTERNAL_SERVER_ERROR, "Unknown Element",
	  "<?xml version='1.0' encoding='UTF-8'?><errors> <error> <foobar /> </error> </errors>",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_PROTOCOL_ERROR },
	{ SOUP_STATUS_INTERNAL_SERVER_ERROR, "Wrong Top-Level Element",
	  "<?xml version='1.0' encoding='UTF-8'?><nonerrors></nonerrors>",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_PROTOCOL_ERROR },
	{ SOUP_STATUS_FORBIDDEN, "Unknown Error Code (Service)",
	  "<?xml version='1.0' encoding='UTF-8'?><errors><error><domain>yt:service</domain><code>UnknownCode</code></error></errors>",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_PROTOCOL_ERROR },
	{ SOUP_STATUS_FORBIDDEN, "Unknown Error Code (Quota)",
	  "<?xml version='1.0' encoding='UTF-8'?><errors><error><domain>yt:quota</domain><code>UnknownCode</code></error></errors>",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_PROTOCOL_ERROR },
	{ SOUP_STATUS_FORBIDDEN, "Unknown Error Domain",
	  "<?xml version='1.0' encoding='UTF-8'?><errors><error><domain>yt:foobaz</domain><code>TokenExpired</code></error></errors>",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_PROTOCOL_ERROR },
#endif
};

static void
test_query_standard_feed_error (gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;
	gulong handler_id, filter_id;
	guint i;
	const gchar *ignore_query_param_values[] = {
		"publishedAfter",
		NULL,
	};

	if (uhm_server_get_enable_logging (mock_server) == TRUE) {
		g_test_message ("Ignoring test due to logging being enabled.");
		return;
	} else if (uhm_server_get_enable_online (mock_server) == TRUE) {
		g_test_message ("Ignoring test due to running online and test not being reproducible.");
		return;
	}

	filter_id = uhm_server_filter_ignore_parameter_values (mock_server,
	                                                       ignore_query_param_values);

	for (i = 0; i < G_N_ELEMENTS (query_standard_feed_errors); i++) {
		const GDataTestRequestErrorData *data = &query_standard_feed_errors[i];

		handler_id = g_signal_connect (mock_server, "handle-message", (GCallback) gdata_test_mock_server_handle_message_error, (gpointer) data);
		gdata_test_mock_server_run (mock_server);

		/* Query the feed. */
		feed = gdata_youtube_service_query_standard_feed (GDATA_YOUTUBE_SERVICE (service), GDATA_YOUTUBE_TOP_RATED_FEED, NULL, NULL, NULL, NULL, &error);
		g_assert_error (error, data->error_domain_func (), data->error_code);
		g_assert (feed == NULL);
		g_clear_error (&error);

		uhm_server_stop (mock_server);
		g_signal_handler_disconnect (mock_server, handler_id);
	}

	uhm_server_compare_messages_remove_filter (mock_server, filter_id);
}

static void
test_query_standard_feed_timeout (gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;
	gulong handler_id, filter_id;
	const gchar *ignore_query_param_values[] = {
		"publishedAfter",
		NULL,
	};

	if (uhm_server_get_enable_logging (mock_server) == TRUE) {
		g_test_message ("Ignoring test due to logging being enabled.");
		return;
	} else if (uhm_server_get_enable_online (mock_server) == TRUE) {
		g_test_message ("Ignoring test due to running online and test not being reproducible.");
		return;
	}

	filter_id = uhm_server_filter_ignore_parameter_values (mock_server,
	                                                       ignore_query_param_values);
	handler_id = g_signal_connect (mock_server, "handle-message", (GCallback) gdata_test_mock_server_handle_message_timeout, NULL);
	gdata_test_mock_server_run (mock_server);

	/* Set the service's timeout as low as possible (1 second). */
	gdata_service_set_timeout (GDATA_SERVICE (service), 1);

	/* Query the feed. */
	feed = gdata_youtube_service_query_standard_feed (GDATA_YOUTUBE_SERVICE (service), GDATA_YOUTUBE_TOP_RATED_FEED, NULL, NULL, NULL, NULL, &error);
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_NETWORK_ERROR);
	g_assert (feed == NULL);
	g_clear_error (&error);

	uhm_server_stop (mock_server);
	g_signal_handler_disconnect (mock_server, handler_id);
	uhm_server_compare_messages_remove_filter (mock_server, filter_id);
}

typedef struct {
	GDataAsyncTestData async_data;
	gulong filter_id;
} StandardFeedData;

static void
set_up_standard_feed_async (StandardFeedData *standard_feed_data,
                            gconstpointer test_data)
{
	const gchar *ignore_query_param_values[] = {
		"publishedAfter",
		NULL,
	};

	gdata_set_up_async_test_data (&standard_feed_data->async_data,
	                              test_data);
	standard_feed_data->filter_id =
		uhm_server_filter_ignore_parameter_values (mock_server,
		                                           ignore_query_param_values);
}

static void
tear_down_standard_feed_async (StandardFeedData *standard_feed_data,
                               gconstpointer test_data)
{
	uhm_server_compare_messages_remove_filter (mock_server,
	                                           standard_feed_data->filter_id);
	gdata_tear_down_async_test_data (&standard_feed_data->async_data,
	                                 test_data);
}

GDATA_ASYNC_TEST_FUNCTIONS (query_standard_feed, void,
G_STMT_START {
	gdata_youtube_service_query_standard_feed_async (GDATA_YOUTUBE_SERVICE (service), GDATA_YOUTUBE_TOP_RATED_FEED, NULL, cancellable,
	                                                 NULL, NULL, NULL, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataFeed *feed;

	feed = gdata_service_query_finish (GDATA_SERVICE (obj), async_result, &error);

	if (error == NULL) {
		g_assert (GDATA_IS_FEED (feed));

		g_assert_cmpuint (g_list_length (gdata_feed_get_entries (feed)), >, 0);

		g_object_unref (feed);
	} else {
		g_assert (feed == NULL);
	}
} G_STMT_END);

static void
test_query_standard_feed_async_progress_closure (gconstpointer service)
{
	GDataAsyncProgressClosure *data = g_slice_new0 (GDataAsyncProgressClosure);
	gulong filter_id;
	const gchar *ignore_query_param_values[] = {
		"publishedAfter",
		NULL,
	};

	g_assert (service != NULL);

	filter_id = uhm_server_filter_ignore_parameter_values (mock_server,
	                                                       ignore_query_param_values);
	gdata_test_mock_server_start_trace (mock_server, "query-standard-feed-async-progress-closure");

	data->main_loop = g_main_loop_new (NULL, TRUE);

	gdata_youtube_service_query_standard_feed_async (GDATA_YOUTUBE_SERVICE (service), GDATA_YOUTUBE_TOP_RATED_FEED, NULL, NULL,
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
	uhm_server_compare_messages_remove_filter (mock_server, filter_id);
}

static GDataYouTubeVideo *
get_video_for_related (void)
{
	GDataYouTubeVideo *video;
	GError *error = NULL;

	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_json (GDATA_TYPE_YOUTUBE_VIDEO,
		"{"
			"'kind': 'youtube#video',"
			"'id': 'q1UPMEmCqZo'"
		"}", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	return video;
}

static void
test_query_related (gconstpointer service)
{
	GDataFeed *feed;
	GDataYouTubeVideo *video;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "query-related");

	video = get_video_for_related ();
	feed = gdata_youtube_service_query_related (GDATA_YOUTUBE_SERVICE (service), video, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* TODO: check entries and feed properties */

	g_object_unref (video);
	g_object_unref (feed);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_TEST_FUNCTIONS (query_related, void,
G_STMT_START {
	GDataYouTubeVideo *video;

	video = get_video_for_related ();
	gdata_youtube_service_query_related_async (GDATA_YOUTUBE_SERVICE (service), video, NULL, cancellable, NULL,
	                                           NULL, NULL, async_ready_callback, async_data);
	g_object_unref (video);
} G_STMT_END,
G_STMT_START {
	GDataFeed *feed;

	feed = gdata_service_query_finish (GDATA_SERVICE (obj), async_result, &error);

	if (error == NULL) {
		g_assert (GDATA_IS_FEED (feed));
		/* TODO: Tests? */

		g_object_unref (feed);
	} else {
		g_assert (feed == NULL);
	}
} G_STMT_END);

static void
test_query_related_async_progress_closure (gconstpointer service)
{
	GDataAsyncProgressClosure *data = g_slice_new0 (GDataAsyncProgressClosure);
	GDataYouTubeVideo *video;

	g_assert (service != NULL);

	gdata_test_mock_server_start_trace (mock_server, "query-related-async-progress-closure");

	data->main_loop = g_main_loop_new (NULL, TRUE);
	video = get_video_for_related ();

	gdata_youtube_service_query_related_async (GDATA_YOUTUBE_SERVICE (service), video, NULL, NULL,
	                                           (GDataQueryProgressCallback) gdata_test_async_progress_callback,
	                                           data, (GDestroyNotify) gdata_test_async_progress_closure_free,
	                                           (GAsyncReadyCallback) gdata_test_async_progress_finish_callback, data);
	g_object_unref (video);

	g_main_loop_run (data->main_loop);
	g_main_loop_unref (data->main_loop);

	/* Check that both callbacks were called exactly once */
	g_assert_cmpuint (data->progress_destroy_notify_count, ==, 1);
	g_assert_cmpuint (data->async_ready_notify_count, ==, 1);

	g_slice_free (GDataAsyncProgressClosure, data);

	uhm_server_end_trace (mock_server);
}

typedef struct {
	GDataYouTubeService *service;
	GDataYouTubeVideo *video;
	GDataYouTubeVideo *updated_video;
	GFile *video_file;
	gchar *slug;
	gchar *content_type;
} UploadData;

static void
set_up_upload (UploadData *data, gconstpointer service)
{
	GDataMediaCategory *category;
	GFileInfo *file_info;
	const gchar * const tags[] = { "toast", "wedding", NULL };
	gchar *path = NULL;
	GError *error = NULL;

	data->service = g_object_ref ((gpointer) service);

	/* Create the metadata for the video being uploaded */
	data->video = gdata_youtube_video_new (NULL);

	gdata_entry_set_title (GDATA_ENTRY (data->video), "Bad Wedding Toast");
	gdata_youtube_video_set_description (data->video, "I gave a bad toast at my friend's wedding.");
	category = gdata_media_category_new ("22", NULL, NULL);
	gdata_youtube_video_set_category (data->video, category);
	g_object_unref (category);
	gdata_youtube_video_set_keywords (data->video, tags);

	/* Get a file to upload */
	/* TODO: fix the path */
	path = g_test_build_filename (G_TEST_DIST, "sample.ogg", NULL);
	data->video_file = g_file_new_for_path (path);
	g_free (path);

	/* Get the file's info */
	file_info = g_file_query_info (data->video_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
	                               G_FILE_QUERY_INFO_NONE, NULL, &error);
	g_assert_no_error (error);
	g_assert (G_IS_FILE_INFO (file_info));

	data->slug = g_strdup (g_file_info_get_display_name (file_info));
	data->content_type = g_strdup (g_file_info_get_content_type (file_info));

	g_object_unref (file_info);
}

static void
tear_down_upload (UploadData *data, gconstpointer service)
{
	gdata_test_mock_server_start_trace (mock_server, "teardown-upload");

	/* Delete the uploaded video, if possible */
	if (data->updated_video != NULL) {
		gdata_service_delete_entry (GDATA_SERVICE (service), gdata_youtube_service_get_primary_authorization_domain (),
		                            GDATA_ENTRY (data->updated_video), NULL, NULL);
		g_object_unref (data->updated_video);
	}

	g_object_unref (data->video);
	g_object_unref (data->video_file);
	g_free (data->slug);
	g_free (data->content_type);
	g_object_unref (data->service);

	uhm_server_end_trace (mock_server);
}

static void
test_upload_simple (UploadData *data, gconstpointer service)
{
	GDataUploadStream *upload_stream;
	GFileInputStream *file_stream;
	const gchar * const *tags, * const *tags2;
	gssize transfer_size;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "upload-simple");

	/* Prepare the upload stream */
	upload_stream = gdata_youtube_service_upload_video (GDATA_YOUTUBE_SERVICE (service), data->video, data->slug, data->content_type, NULL,
	                                                    &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_UPLOAD_STREAM (upload_stream));

	/* Get an input stream for the file */
	file_stream = g_file_read (data->video_file, NULL, &error);
	g_assert_no_error (error);
	g_assert (G_IS_FILE_INPUT_STREAM (file_stream));

	/* Upload the video */
	transfer_size = g_output_stream_splice (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (file_stream),
	                                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &error);
	g_assert_no_error (error);
	g_assert_cmpint (transfer_size, >, 0);

	/* Finish off the upload */
	data->updated_video = gdata_youtube_service_finish_video_upload (GDATA_YOUTUBE_SERVICE (service), upload_stream, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (data->updated_video));

	g_object_unref (file_stream);
	g_object_unref (upload_stream);

	/* Check the video's properties */
	g_assert (gdata_entry_is_inserted (GDATA_ENTRY (data->updated_video)));
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (data->updated_video)), ==, gdata_entry_get_title (GDATA_ENTRY (data->video)));
	g_assert_cmpstr (gdata_youtube_video_get_description (data->updated_video), ==, gdata_youtube_video_get_description (data->video));
	g_assert_cmpstr (gdata_media_category_get_category (gdata_youtube_video_get_category (data->updated_video)), ==,
	                 gdata_media_category_get_category (gdata_youtube_video_get_category (data->video)));

	tags = gdata_youtube_video_get_keywords (data->video);
	tags2 = gdata_youtube_video_get_keywords (data->updated_video);
	g_assert_cmpuint (g_strv_length ((gchar**) tags2), ==, g_strv_length ((gchar**) tags));
	g_assert_cmpstr (tags2[0], ==, tags[0]);
	g_assert_cmpstr (tags2[1], ==, tags[1]);
	g_assert_cmpstr (tags2[2], ==, tags[2]);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_CLOSURE_FUNCTIONS (upload, UploadData);

GDATA_ASYNC_TEST_FUNCTIONS (upload, UploadData,
G_STMT_START {
	GDataUploadStream *upload_stream;
	GFileInputStream *file_stream;
	GError *error = NULL;

	/* Prepare the upload stream */
	upload_stream = gdata_youtube_service_upload_video (GDATA_YOUTUBE_SERVICE (service), data->video, data->slug,
	                                                    data->content_type, cancellable, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_UPLOAD_STREAM (upload_stream));

	/* Get an input stream for the file */
	file_stream = g_file_read (data->video_file, NULL, &error);
	g_assert_no_error (error);
	g_assert (G_IS_FILE_INPUT_STREAM (file_stream));

	/* Upload the video asynchronously */
	g_output_stream_splice_async (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (file_stream),
	                              G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, G_PRIORITY_DEFAULT, NULL,
	                              async_ready_callback, async_data);

	g_object_unref (file_stream);
	g_object_unref (upload_stream);
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
		data->updated_video = gdata_youtube_service_finish_video_upload (data->service, GDATA_UPLOAD_STREAM (stream), &upload_error);
		g_assert_no_error (upload_error);
		g_assert (GDATA_IS_YOUTUBE_VIDEO (data->updated_video));

		/* Check the video's properties */
		g_assert (gdata_entry_is_inserted (GDATA_ENTRY (data->updated_video)));
		g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (data->updated_video)), ==, gdata_entry_get_title (GDATA_ENTRY (data->video)));
		g_assert_cmpstr (gdata_youtube_video_get_description (data->updated_video), ==, gdata_youtube_video_get_description (data->video));
		g_assert_cmpstr (gdata_media_category_get_category (gdata_youtube_video_get_category (data->updated_video)), ==,
		                 gdata_media_category_get_category (gdata_youtube_video_get_category (data->video)));

		tags = gdata_youtube_video_get_keywords (data->video);
		tags2 = gdata_youtube_video_get_keywords (data->updated_video);
		g_assert_cmpuint (g_strv_length ((gchar**) tags2), ==, g_strv_length ((gchar**) tags));
		g_assert_cmpstr (tags2[0], ==, tags[0]);
		g_assert_cmpstr (tags2[1], ==, tags[1]);
		g_assert_cmpstr (tags2[2], ==, tags[2]);
	} else {
		g_assert_cmpint (transfer_size, ==, -1);

		/* Finish off the upload */
		data->updated_video = gdata_youtube_service_finish_video_upload (data->service, GDATA_UPLOAD_STREAM (stream), &upload_error);
		g_assert_no_error (upload_error);
		g_assert (data->updated_video == NULL);
	}

	g_clear_error (&upload_error);
} G_STMT_END);

static void
test_parsing_app_control (void)
{
	GDataYouTubeVideo *video;
	GDataYouTubeState *state;
	GError *error = NULL;

	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_json (GDATA_TYPE_YOUTUBE_VIDEO,
		"{"
			"'kind': 'youtube#video',"
			"'etag': '\"tbWC5XrSXxe1WOAx6MK9z4hHSU8/X_byq2BdOVgHzCA-ScpZbTWmgfQ\"',"
			"'id': 'JAagedeKdcQ',"
			"'snippet': {"
				"'publishedAt': '2006-05-16T14:06:37.000Z',"
				"'channelId': 'UCCS6UQvicRHyn1whEUDEMUQ',"
				"'title': 'Judas Priest - Painkiller',"
				"'description': 'Videoclip de Judas Priest',"
				"'thumbnails': {"
					"'default': {"
						"'url': 'https://i.ytimg.com/vi/JAagedeKdcQ/default.jpg',"
						"'width': 120,"
						"'height': 90"
					"},"
					"'medium': {"
						"'url': 'https://i.ytimg.com/vi/JAagedeKdcQ/mqdefault.jpg',"
						"'width': 320,"
						"'height': 180"
					"},"
					"'high': {"
						"'url': 'https://i.ytimg.com/vi/JAagedeKdcQ/hqdefault.jpg',"
						"'width': 480,"
						"'height': 360"
					"}"
				"},"
				"'channelTitle': 'eluves',"
				"'categoryId': '10',"
				"'liveBroadcastContent': 'none',"
				"'localized': {"
					"'title': 'Judas Priest - Painkiller',"
					"'description': 'Videoclip de Judas Priest'"
				"}"
			"},"
			"'contentDetails': {"
				"'duration': 'PT6M',"
				"'dimension': '2d',"
				"'definition': 'sd',"
				"'caption': 'false',"
				"'licensedContent': false,"
				"'regionRestriction': {"
					"'blocked': ["
						"'RU',"
						"'RW',"
						"'RS',"
						"'RO',"
						"'RE',"
						"'BL',"
						"'BM',"
						"'BN',"
						"'BO',"
						"'JP',"
						"'BI',"
						"'BJ',"
						"'BD',"
						"'BE',"
						"'BF',"
						"'BG',"
						"'YT',"
						"'BB',"
						"'CX',"
						"'JE',"
						"'BY',"
						"'BZ',"
						"'BT',"
						"'JM',"
						"'BV',"
						"'BW',"
						"'YE',"
						"'BQ',"
						"'BR',"
						"'BS',"
						"'IM',"
						"'IL',"
						"'IO',"
						"'IN',"
						"'IE',"
						"'ID',"
						"'QA',"
						"'TM',"
						"'IQ',"
						"'IS',"
						"'IR',"
						"'IT',"
						"'TK',"
						"'AE',"
						"'AD',"
						"'AG',"
						"'AF',"
						"'AI',"
						"'AM',"
						"'AL',"
						"'AO',"
						"'AQ',"
						"'AS',"
						"'AR',"
						"'AU',"
						"'AT',"
						"'AW',"
						"'TG',"
						"'AX',"
						"'AZ',"
						"'PR',"
						"'HK',"
						"'HN',"
						"'PW',"
						"'PT',"
						"'HM',"
						"'PY',"
						"'PA',"
						"'PF',"
						"'PG',"
						"'PE',"
						"'HR',"
						"'PK',"
						"'PH',"
						"'PN',"
						"'HT',"
						"'HU',"
						"'OM',"
						"'WS',"
						"'WF',"
						"'BH',"
						"'KP',"
						"'TT',"
						"'GG',"
						"'GF',"
						"'GE',"
						"'GD',"
						"'GB',"
						"'VN',"
						"'VA',"
						"'GM',"
						"'VC',"
						"'VE',"
						"'GI',"
						"'VG',"
						"'GW',"
						"'GU',"
						"'GT',"
						"'GS',"
						"'GR',"
						"'GQ',"
						"'GP',"
						"'VU',"
						"'GY',"
						"'NA',"
						"'NC',"
						"'NE',"
						"'NF',"
						"'NG',"
						"'NI',"
						"'NL',"
						"'BA',"
						"'NO',"
						"'NP',"
						"'NR',"
						"'NU',"
						"'NZ',"
						"'PM',"
						"'UM',"
						"'TV',"
						"'UG',"
						"'UA',"
						"'FI',"
						"'FJ',"
						"'FK',"
						"'UY',"
						"'FM',"
						"'CN',"
						"'UZ',"
						"'US',"
						"'ME',"
						"'MD',"
						"'MG',"
						"'MF',"
						"'MA',"
						"'MC',"
						"'VI',"
						"'MM',"
						"'ML',"
						"'MO',"
						"'FO',"
						"'MH',"
						"'MK',"
						"'MU',"
						"'MT',"
						"'MW',"
						"'MV',"
						"'MQ',"
						"'MP',"
						"'MS',"
						"'MR',"
						"'CO',"
						"'CV',"
						"'MY',"
						"'MX',"
						"'MZ',"
						"'TN',"
						"'TO',"
						"'TL',"
						"'JO',"
						"'TJ',"
						"'GA',"
						"'TH',"
						"'TF',"
						"'ET',"
						"'TD',"
						"'TC',"
						"'ES',"
						"'ER',"
						"'TZ',"
						"'EH',"
						"'GN',"
						"'EE',"
						"'TW',"
						"'EG',"
						"'TR',"
						"'CA',"
						"'EC',"
						"'GL',"
						"'LB',"
						"'LC',"
						"'LA',"
						"'MN',"
						"'LK',"
						"'LI',"
						"'LV',"
						"'LT',"
						"'LU',"
						"'LR',"
						"'LS',"
						"'PS',"
						"'KZ',"
						"'GH',"
						"'LY',"
						"'DZ',"
						"'DO',"
						"'DM',"
						"'DJ',"
						"'PL',"
						"'DK',"
						"'DE',"
						"'SZ',"
						"'SY',"
						"'SX',"
						"'SS',"
						"'SR',"
						"'SV',"
						"'ST',"
						"'SK',"
						"'SJ',"
						"'SI',"
						"'SH',"
						"'SO',"
						"'SN',"
						"'SM',"
						"'SL',"
						"'SC',"
						"'SB',"
						"'SA',"
						"'FR',"
						"'SG',"
						"'SE',"
						"'SD',"
						"'CK',"
						"'KR',"
						"'CI',"
						"'CH',"
						"'KW',"
						"'ZA',"
						"'CM',"
						"'CL',"
						"'CC',"
						"'ZM',"
						"'KY',"
						"'CG',"
						"'CF',"
						"'CD',"
						"'CZ',"
						"'CY',"
						"'ZW',"
						"'KG',"
						"'CU',"
						"'KE',"
						"'CR',"
						"'KI',"
						"'KH',"
						"'CW',"
						"'KN',"
						"'KM'"
					"]"
				"}"
			"},"
			"'status': {"
				"'uploadStatus': 'processed',"
				"'privacyStatus': 'private',"
				"'license': 'youtube',"
				"'embeddable': true,"
				"'publicStatsViewable': true"
			"},"
			"'statistics': {"
				"'viewCount': '4369107',"
				"'likeCount': '13619',"
				"'dislikeCount': '440',"
				"'favoriteCount': '0',"
				"'commentCount': '11181'"
			"}"
		"}", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	/* Test the app:control values */
	G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	g_assert (gdata_youtube_video_is_draft (video) == TRUE);
	G_GNUC_END_IGNORE_DEPRECATIONS

	state = gdata_youtube_video_get_state (video);
	g_assert_cmpstr (gdata_youtube_state_get_name (state), ==, NULL);
	g_assert_cmpstr (gdata_youtube_state_get_message (state), ==, NULL);
	g_assert (gdata_youtube_state_get_reason_code (state) == NULL);
	g_assert (gdata_youtube_state_get_help_uri (state) == NULL);

	/* TODO: more tests on entry properties */

	g_object_unref (video);
}

static void
test_parsing_yt_recorded (void)
{
	GDataYouTubeVideo *video;
	gint64 recorded;
	GError *error = NULL;

	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_json (GDATA_TYPE_YOUTUBE_VIDEO,
		"{"
			"'kind': 'youtube#video',"
			"'etag': '\"tbWC5XrSXxe1WOAx6MK9z4hHSU8/X_byq2BdOVgHzCA-ScpZbTWmgfQ\"',"
			"'id': 'JAagedeKdcQ',"
			"'snippet': {"
				"'publishedAt': '2006-05-16T14:06:37.000Z',"
				"'channelId': 'UCCS6UQvicRHyn1whEUDEMUQ',"
				"'title': 'Judas Priest - Painkiller',"
				"'description': 'Videoclip de Judas Priest',"
				"'channelTitle': 'eluves',"
				"'categoryId': '10',"
				"'liveBroadcastContent': 'none',"
				"'localized': {"
					"'title': 'Judas Priest - Painkiller',"
					"'description': 'Videoclip de Judas Priest'"
				"}"
			"},"
			"'recordingDetails': {"
				"'recordingDate': '2003-08-03'"
			"}"
		"}", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	/* Test the recorded date */
	recorded = gdata_youtube_video_get_recorded (video);
	g_assert_cmpint (recorded, ==, 1059868800);

	/* Update the state and see if the XML's written out OK */
	recorded = 1128229200;
	gdata_youtube_video_set_recorded (video, recorded);

	/* Check the XML */
	gdata_test_assert_json (video,
		"{"
			"'kind': 'youtube#video',"
			"'etag': '\"tbWC5XrSXxe1WOAx6MK9z4hHSU8/X_byq2BdOVgHzCA-ScpZbTWmgfQ\"',"
			"'id': 'JAagedeKdcQ',"
			"'selfLink': 'https://www.googleapis.com/youtube/v3/videos?id=JAagedeKdcQ',"
			"'title': 'Judas Priest - Painkiller',"
			"'description': 'Videoclip de Judas Priest',"
			"'snippet': {"
				"'title': 'Judas Priest - Painkiller',"
				"'description': 'Videoclip de Judas Priest',"
				"'categoryId': '10'"
			"},"
			"'status': {"
				"'privacyStatus': 'public'"
			"},"
			"'recordingDetails': {"
				"'recordingDate': '2005-10-02'"
			"}"
		"}");

	/* TODO: more tests on entry properties */

	g_object_unref (video);
}

static void
test_parsing_yt_access_control (void)
{
	GDataYouTubeVideo *video;
	GError *error = NULL;

	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_json (GDATA_TYPE_YOUTUBE_VIDEO,
		"{"
			"'kind': 'youtube#video',"
			"'id': 'JAagedeKdcQ',"
			"'status': {"
				"'privacyStatus': 'public',"
				"'embeddable': false"
			"}"
		"}", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	/* Test the access controls */
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_RATE), ==, GDATA_YOUTUBE_PERMISSION_DENIED);
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_COMMENT), ==, GDATA_YOUTUBE_PERMISSION_DENIED);
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_COMMENT_VOTE), ==, GDATA_YOUTUBE_PERMISSION_DENIED);
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_VIDEO_RESPOND), ==, GDATA_YOUTUBE_PERMISSION_DENIED);
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_EMBED), ==, GDATA_YOUTUBE_PERMISSION_DENIED);
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_SYNDICATE), ==, GDATA_YOUTUBE_PERMISSION_DENIED);
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, "list"), ==, GDATA_YOUTUBE_PERMISSION_ALLOWED);

	/* Update some of them and see if the JSON’s written out OK */
	gdata_youtube_video_set_access_control (video, "list", GDATA_YOUTUBE_PERMISSION_DENIED);
	gdata_youtube_video_set_access_control (video, GDATA_YOUTUBE_ACTION_EMBED, GDATA_YOUTUBE_PERMISSION_ALLOWED);

	/* Check the JSON */
	gdata_test_assert_json (video,
		"{"
			"'kind': 'youtube#video',"
			"'id': 'JAagedeKdcQ',"
			"'selfLink': 'https://www.googleapis.com/youtube/v3/videos?id=JAagedeKdcQ',"
			"'title': null,"
			"'snippet': {},"
			"'status': {"
				"'privacyStatus': 'unlisted',"
				"'embeddable': true"
			"},"
			"'recordingDetails': {}"
		"}");

	g_object_unref (video);
}

static void
test_parsing_yt_category (void)
{
	GDataYouTubeCategory *category;
	gboolean assignable, deprecated;
	GError *error = NULL;

	/* Test a non-deprecated category */
	category = GDATA_YOUTUBE_CATEGORY (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_CATEGORY,
		"<category xmlns='http://www.w3.org/2005/Atom' xmlns:yt='http://gdata.youtube.com/schemas/2007' "
			"scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'>"
			"<yt:assignable/>"
			"<yt:browsable regions='CZ AU HK'/>"
		"</category>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_CATEGORY (category));
	g_clear_error (&error);

	/* Test the category's properties */
	g_assert (gdata_youtube_category_is_assignable (category) == TRUE);
	g_assert (gdata_youtube_category_is_browsable (category, "CZ") == TRUE);
	g_assert (gdata_youtube_category_is_browsable (category, "AU") == TRUE);
	g_assert (gdata_youtube_category_is_browsable (category, "HK") == TRUE);
	g_assert (gdata_youtube_category_is_browsable (category, "GB") == FALSE);
	g_assert (gdata_youtube_category_is_deprecated (category) == FALSE);

	/* Test the properties the other way */
	g_object_get (category, "is-assignable", &assignable, "is-deprecated", &deprecated, NULL);
	g_assert (assignable == TRUE);
	g_assert (deprecated == FALSE);

	g_object_unref (category);

	/* Test a deprecated category */
	category = GDATA_YOUTUBE_CATEGORY (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_CATEGORY,
		"<category xmlns='http://www.w3.org/2005/Atom' xmlns:yt='http://gdata.youtube.com/schemas/2007' "
			"scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'>"
			"<yt:deprecated/>"
		"</category>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_CATEGORY (category));
	g_clear_error (&error);

	/* Test the category's properties */
	g_assert (gdata_youtube_category_is_assignable (category) == FALSE);
	g_assert (gdata_youtube_category_is_browsable (category, "CZ") == FALSE);
	g_assert (gdata_youtube_category_is_browsable (category, "AU") == FALSE);
	g_assert (gdata_youtube_category_is_browsable (category, "HK") == FALSE);
	g_assert (gdata_youtube_category_is_browsable (category, "GB") == FALSE);
	g_assert (gdata_youtube_category_is_deprecated (category) == TRUE);

	g_object_unref (category);
}

static void
test_parsing_georss_where (void)
{
	GDataYouTubeVideo *video;
	gdouble latitude, longitude;
	GError *error = NULL;

	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_json (GDATA_TYPE_YOUTUBE_VIDEO,
		"{"
			"'kind': 'youtube#video',"
			"'id': 'JAagedeKdcQ',"
			"'recordingDetails': {"
				"'location': {"
					"'latitude': 41.14556884765625,"
					"'longitude': -8.63525390625"
				"}"
			"}"
		"}", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	/* Test the coordinates */
	gdata_youtube_video_get_coordinates (video, &latitude, &longitude);
	g_assert_cmpfloat (latitude, ==, 41.14556884765625);
	g_assert_cmpfloat (longitude, ==, -8.63525390625);

	/* Update them and see if they're set OK and the JSON’s written out OK */
	gdata_youtube_video_set_coordinates (video, 5.5, 6.5);

	g_object_get (G_OBJECT (video),
	              "latitude", &latitude,
	              "longitude", &longitude,
	              NULL);

	g_assert_cmpfloat (latitude, ==, 5.5);
	g_assert_cmpfloat (longitude, ==, 6.5);

	/* Check the JSON */
	gdata_test_assert_json (video,
		"{"
			"'kind': 'youtube#video',"
			"'id': 'JAagedeKdcQ',"
			"'selfLink': 'https://www.googleapis.com/youtube/v3/videos?id=JAagedeKdcQ',"
			"'title': null,"
			"'snippet': {},"
			"'status': {"
				"'privacyStatus': 'public'"
			"},"
			"'recordingDetails': {"
				"'location': {"
					"'latitude': 5.5,"
					"'longitude': 6.5"
				"}"
			"}"
		"}");

	/* Unset the properties and ensure they’re removed from the JSON */
	gdata_youtube_video_set_coordinates (video, G_MAXDOUBLE, G_MAXDOUBLE);

	gdata_youtube_video_get_coordinates (video, &latitude, &longitude);
	g_assert_cmpfloat (latitude, ==, G_MAXDOUBLE);
	g_assert_cmpfloat (longitude, ==, G_MAXDOUBLE);

	/* Check the JSON */
	gdata_test_assert_json (video,
		"{"
			"'kind': 'youtube#video',"
			"'id': 'JAagedeKdcQ',"
			"'selfLink': 'https://www.googleapis.com/youtube/v3/videos?id=JAagedeKdcQ',"
			"'title': null,"
			"'snippet': {},"
			"'status': {"
				"'privacyStatus': 'public'"
			"},"
			"'recordingDetails': {}"
		"}");

	g_object_unref (video);
}

static void
test_parsing_ratings (void)
{
	GDataYouTubeVideo *video;
	GError *error = NULL;

	/* Parse all ratings */
	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_json (GDATA_TYPE_YOUTUBE_VIDEO,
		"{"
			"'kind': 'youtube#video',"
			"'id': 'JAagedeKdcQ',"
			"'contentDetails': {"
				"'contentRating': {"
					"'mpaaRating': 'mpaaPg',"
					"'tvpgRating': 'tvpgPg'"
				"}"
			"}"
		"}", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	/* Check the ratings, and check that we haven't ended up with a country restriction */
	g_assert_cmpstr (gdata_youtube_video_get_media_rating (video, GDATA_YOUTUBE_RATING_TYPE_MPAA), ==, "pg");
	g_assert_cmpstr (gdata_youtube_video_get_media_rating (video, GDATA_YOUTUBE_RATING_TYPE_V_CHIP), ==, "tv-pg");

	g_assert (gdata_youtube_video_is_restricted_in_country (video, "US") == FALSE);

	g_object_unref (video);

	/* Parse a video with one rating missing and see what happens */
	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_json (GDATA_TYPE_YOUTUBE_VIDEO,
		"{"
			"'kind': 'youtube#video',"
			"'id': 'JAagedeKdcQ',"
			"'contentDetails': {"
				"'contentRating': {"
					"'tvpgRating': 'tvpgY7Fv'"
				"}"
			"}"
		"}", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	/* Check the ratings again */
	g_assert_cmpstr (gdata_youtube_video_get_media_rating (video, GDATA_YOUTUBE_RATING_TYPE_MPAA), ==, NULL);
	g_assert_cmpstr (gdata_youtube_video_get_media_rating (video, GDATA_YOUTUBE_RATING_TYPE_V_CHIP), ==, "tv-y7-fv");

	/* Check that calling with an arbitrary rating type returns NULL.
	 * %GDATA_YOUTUBE_RATING_TYPE_SIMPLE is no longer supported. */
	g_assert_cmpstr (gdata_youtube_video_get_media_rating (video, "fooish bar"), ==, NULL);
	g_assert_cmpstr (gdata_youtube_video_get_media_rating (video, GDATA_YOUTUBE_RATING_TYPE_SIMPLE), ==, NULL);

	g_object_unref (video);
}

static void
test_video_escaping (void)
{
	GDataYouTubeVideo *video;
	const gchar * const keywords[] = { "<keyword1>", "keyword2 & stuff, things", NULL };

	video = gdata_youtube_video_new (NULL);
	gdata_youtube_video_set_location (video, "\"Here\" & 'there'");
	gdata_youtube_video_set_access_control (video, "<action>", GDATA_YOUTUBE_PERMISSION_ALLOWED);
	gdata_youtube_video_set_keywords (video, keywords);
	gdata_youtube_video_set_description (video, "Description & 'stuff'.");
	gdata_youtube_video_set_aspect_ratio (video, "4 & 3");

	/* Check the outputted JSON is escaped properly */
	gdata_test_assert_json (video,
		"{"
			"'kind': 'youtube#video',"
			"'title': null,"
			"'description': \"Description & 'stuff'.\","
			"'snippet': {"
				"'description': \"Description & 'stuff'.\","
				"'tags': ["
					"'<keyword1>',"
					"'keyword2 & stuff, things'"
				"]"
			"},"
			"'status': {"
				"'privacyStatus': 'public'"
			"},"
			"'recordingDetails': {"
				"'locationDescription': \"\\\"Here\\\" & 'there'\""
			"}"
		"}");
	g_object_unref (video);
}

/* Check that a newly-constructed video does not output a location in its
 * JSON or properties. */
static void
test_video_location (void)
{
	GDataYouTubeVideo *video;
	gdouble latitude, longitude;

	video = gdata_youtube_video_new (NULL);

	g_assert_null (gdata_youtube_video_get_location (video));

	/* Latitude and longitude should be outside the valid ranges. */
	gdata_youtube_video_get_coordinates (video, &latitude, &longitude);
	g_assert (latitude < -90.0 || latitude > 90.0);
	g_assert (longitude < -180.0 || longitude > 180.0);

	/* Check the outputted JSON is escaped properly */
	gdata_test_assert_json (video,
		"{"
			"'title': null,"
			"'kind': 'youtube#video',"
			"'snippet': {},"
			"'status': {"
				"'privacyStatus': 'public'"
			"},"
			"'recordingDetails': {}"
		"}");

	g_object_unref (video);
}

static void
test_comment_get_json (void)
{
	GDataYouTubeComment *comment_;

	comment_ = gdata_youtube_comment_new (NULL);
	gdata_entry_set_content (GDATA_ENTRY (comment_),
	                         "This is a comment with <markup> & 'stüff'.");
	gdata_youtube_comment_set_parent_comment_uri (comment_,
	                                              "http://example.com/?foo=bar&baz=shizzle");

	/* Check the outputted JSON is OK */
	gdata_test_assert_json (comment_,
		"{"
			"'kind': 'youtube#commentThread',"
			"'snippet' : {"
				"'topLevelComment': {"
					"'kind': 'youtube#comment',"
					"'snippet': {"
						"'textOriginal': \"This is a comment with <markup> & 'stüff'.\","
						"'parentId': 'http://example.com/?foo=bar&baz=shizzle'"
					"}"
				"}"
			"}"
		"}");

	g_object_unref (comment_);
}

static void
notify_cb (GDataYouTubeComment *comment_, GParamSpec *pspec, guint *notification_count)
{
	*notification_count = *notification_count + 1;
}

static void
test_comment_properties_parent_comment_uri (void)
{
	GDataYouTubeComment *comment_;
	guint notification_count = 0;
	gchar *parent_comment_uri;

	comment_ = gdata_youtube_comment_new (NULL);
	g_signal_connect (comment_, "notify::parent-comment-uri", (GCallback) notify_cb, &notification_count);

	/* Default. */
	g_assert (gdata_youtube_comment_get_parent_comment_uri (comment_) == NULL);

	/* Set the property. */
	gdata_youtube_comment_set_parent_comment_uri (comment_, "foo");
	g_assert_cmpuint (notification_count, ==, 1);

	g_assert_cmpstr (gdata_youtube_comment_get_parent_comment_uri (comment_), ==, "foo");

	/* Get the property a different way. */
	g_object_get (G_OBJECT (comment_),
	              "parent-comment-uri", &parent_comment_uri,
	              NULL);

	g_assert_cmpstr (parent_comment_uri, ==, "foo");

	g_free (parent_comment_uri);

	/* Set the property a different way. */
	g_object_set (G_OBJECT (comment_),
	              "parent-comment-uri", "bar",
	              NULL);
	g_assert_cmpuint (notification_count, ==, 2);

	/* Set the property to the same value. */
	gdata_youtube_comment_set_parent_comment_uri (comment_, "bar");
	g_assert_cmpuint (notification_count, ==, 2);

	/* Set the property back to NULL. */
	gdata_youtube_comment_set_parent_comment_uri (comment_, NULL);
	g_assert_cmpuint (notification_count, ==, 3);

	g_assert (gdata_youtube_comment_get_parent_comment_uri (comment_) == NULL);

	g_object_unref (comment_);
}

static gchar *
build_this_week_date_str (void)
{
	GTimeVal tv;

	g_get_current_time (&tv);
	tv.tv_sec -= 7 * 24 * 60 * 60;  /* this week */
	tv.tv_usec = 0;  /* pointless accuracy */

	return g_time_val_to_iso8601 (&tv);
}

static void
test_query_uri (void)
{
	gdouble latitude, longitude, radius;
	gboolean has_location;
	gchar *query_uri;
	GDataYouTubeQuery *query = gdata_youtube_query_new ("q");
	gchar *this_week_date_str, *expected_uri;

	G_GNUC_BEGIN_IGNORE_DEPRECATIONS

	/* This should not appear in the query because it is deprecated. */
	gdata_youtube_query_set_format (query, GDATA_YOUTUBE_FORMAT_RTSP_H263_AMR);
	g_assert_cmpuint (gdata_youtube_query_get_format (query), ==, 1);

	/* Location */
	gdata_youtube_query_set_location (query, 45.01364, -97.12356, 112.5, TRUE);
	gdata_youtube_query_get_location (query, &latitude, &longitude, &radius, &has_location);

	g_assert_cmpfloat (latitude, ==, 45.01364);
	g_assert_cmpfloat (longitude, ==, -97.12356);
	g_assert_cmpfloat (radius, ==, 112.5);
	g_assert (has_location == TRUE);

	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&safeSearch=none&location=45.013640000000002,-97.123559999999998&locationRadius=112.5m");
	g_free (query_uri);

	/* This used to set the has-location parameter in the query, but that’s
	 * no longer supported by Google, so it should be the same as the
	 * following query. */
	gdata_youtube_query_set_location (query, G_MAXDOUBLE, 0.6672, 52.8, TRUE);

	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&safeSearch=none");
	g_free (query_uri);

	gdata_youtube_query_set_location (query, G_MAXDOUBLE, G_MAXDOUBLE, 0.0, FALSE);

	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&safeSearch=none");
	g_free (query_uri);

	/* Language; this should not appear in the query as it is no longer
	 * supported. */
	gdata_youtube_query_set_language (query, "fr");
	g_assert_cmpstr (gdata_youtube_query_get_language (query), ==, "fr");

	gdata_youtube_query_set_order_by (query, "relevance_lang_fr");
	g_assert_cmpstr (gdata_youtube_query_get_order_by (query), ==, "relevance_lang_fr");

	gdata_youtube_query_set_restriction (query, "192.168.0.1");
	g_assert_cmpstr (gdata_youtube_query_get_restriction (query), ==, "192.168.0.1");

	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&safeSearch=none&order=relevance&regionCode=192.168.0.1");
	g_free (query_uri);

	gdata_youtube_query_set_safe_search (query, GDATA_YOUTUBE_SAFE_SEARCH_STRICT);
	g_assert_cmpuint (gdata_youtube_query_get_safe_search (query), ==, GDATA_YOUTUBE_SAFE_SEARCH_STRICT);

	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&safeSearch=strict&order=relevance&regionCode=192.168.0.1");
	g_free (query_uri);

	/* Deprecated and unused: */
	gdata_youtube_query_set_sort_order (query, GDATA_YOUTUBE_SORT_ASCENDING);
	g_assert_cmpuint (gdata_youtube_query_get_sort_order (query), ==, GDATA_YOUTUBE_SORT_ASCENDING);

	gdata_youtube_query_set_age (query, GDATA_YOUTUBE_AGE_THIS_WEEK);
	g_assert_cmpuint (gdata_youtube_query_get_age (query), ==, GDATA_YOUTUBE_AGE_THIS_WEEK);

	/* Deprecated and unused: */
	gdata_youtube_query_set_uploader (query, GDATA_YOUTUBE_UPLOADER_PARTNER);
	g_assert_cmpuint (gdata_youtube_query_get_uploader (query), ==, GDATA_YOUTUBE_UPLOADER_PARTNER);

	gdata_youtube_query_set_license (query, GDATA_YOUTUBE_LICENSE_CC);
	g_assert_cmpstr (gdata_youtube_query_get_license (query), ==, GDATA_YOUTUBE_LICENSE_CC);

	/* Check the built URI with a normal feed URI */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	this_week_date_str = build_this_week_date_str ();
	expected_uri = g_strdup_printf ("http://example.com?q=q&publishedAfter=%s&safeSearch=strict&order=relevance&regionCode=192.168.0.1&videoLicense=creativeCommon", this_week_date_str);
	g_assert_cmpstr (query_uri, ==, expected_uri);

	g_free (this_week_date_str);
	g_free (expected_uri);
	g_free (query_uri);

	/* …and with a feed URI with pre-existing arguments */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com?foobar=shizzle");
	this_week_date_str = build_this_week_date_str ();
	expected_uri = g_strdup_printf ("http://example.com?foobar=shizzle&q=q&publishedAfter=%s&safeSearch=strict&order=relevance&regionCode=192.168.0.1&videoLicense=creativeCommon", this_week_date_str);
	g_assert_cmpstr (query_uri, ==, expected_uri);

	g_free (this_week_date_str);
	g_free (expected_uri);
	g_free (query_uri);

	g_object_unref (query);

	G_GNUC_END_IGNORE_DEPRECATIONS
}

static void
test_query_etag (void)
{
	GDataYouTubeQuery *query = gdata_youtube_query_new (NULL);

	/* Test that setting any property will unset the ETag */
	g_test_bug ("613529");

	G_GNUC_BEGIN_IGNORE_DEPRECATIONS

#define CHECK_ETAG(C) \
	gdata_query_set_etag (GDATA_QUERY (query), "foobar");		\
	(C);								\
	g_assert (gdata_query_get_etag (GDATA_QUERY (query)) == NULL);

	CHECK_ETAG (gdata_youtube_query_set_format (query, GDATA_YOUTUBE_FORMAT_RTSP_H263_AMR))
	CHECK_ETAG (gdata_youtube_query_set_location (query, 0.0, 65.0, 15.0, TRUE))
	CHECK_ETAG (gdata_youtube_query_set_language (query, "British English"))
	CHECK_ETAG (gdata_youtube_query_set_order_by (query, "shizzle"))
	CHECK_ETAG (gdata_youtube_query_set_restriction (query, "restriction"))
	CHECK_ETAG (gdata_youtube_query_set_safe_search (query, GDATA_YOUTUBE_SAFE_SEARCH_MODERATE))
	CHECK_ETAG (gdata_youtube_query_set_sort_order (query, GDATA_YOUTUBE_SORT_DESCENDING))
	CHECK_ETAG (gdata_youtube_query_set_age (query, GDATA_YOUTUBE_AGE_THIS_WEEK))
	CHECK_ETAG (gdata_youtube_query_set_uploader (query, GDATA_YOUTUBE_UPLOADER_PARTNER))
	CHECK_ETAG (gdata_youtube_query_set_license (query, GDATA_YOUTUBE_LICENSE_STANDARD))

#undef CHECK_ETAG

	g_object_unref (query);

	G_GNUC_END_IGNORE_DEPRECATIONS
}

static void
test_query_single (gconstpointer service)
{
	GDataYouTubeVideo *video;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "query-single");

	video = GDATA_YOUTUBE_VIDEO (gdata_service_query_single_entry (GDATA_SERVICE (service),
	                                                               gdata_youtube_service_get_primary_authorization_domain (),
	                                                               "tag:youtube.com,2008:video:_LeQuMpwbW4", NULL,
	                                                               GDATA_TYPE_YOUTUBE_VIDEO, NULL, &error));

	g_assert_no_error (error);
	g_assert (video != NULL);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));

	G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	g_assert_cmpstr (gdata_youtube_video_get_video_id (video), ==, "_LeQuMpwbW4");
	g_assert_cmpstr (gdata_entry_get_id (GDATA_ENTRY (video)), ==, "_LeQuMpwbW4");
	G_GNUC_END_IGNORE_DEPRECATIONS

	g_clear_error (&error);

	g_object_unref (video);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_TEST_FUNCTIONS (query_single, void,
G_STMT_START {
	gdata_service_query_single_entry_async (GDATA_SERVICE (service), gdata_youtube_service_get_primary_authorization_domain (),
	                                        "tag:youtube.com,2008:video:_LeQuMpwbW4", NULL, GDATA_TYPE_YOUTUBE_VIDEO,
	                                        cancellable, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataYouTubeVideo *video;

	video = GDATA_YOUTUBE_VIDEO (gdata_service_query_single_entry_finish (GDATA_SERVICE (obj), async_result, &error));

	if (error == NULL) {
		G_GNUC_BEGIN_IGNORE_DEPRECATIONS
		g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
		g_assert_cmpstr (gdata_youtube_video_get_video_id (video), ==, "_LeQuMpwbW4");
		g_assert_cmpstr (gdata_entry_get_id (GDATA_ENTRY (video)), ==, "_LeQuMpwbW4");
		G_GNUC_END_IGNORE_DEPRECATIONS

		g_object_unref (video);
	} else {
		g_assert (video == NULL);
	}
} G_STMT_END);

typedef struct {
	GDataYouTubeVideo *video;
} CommentData;

static void
set_up_comment (CommentData *data, gconstpointer service)
{
	gdata_test_mock_server_start_trace (mock_server, "setup-comment");

	/* Get a video known to have comments on it. */
	data->video = GDATA_YOUTUBE_VIDEO (gdata_service_query_single_entry (GDATA_SERVICE (service),
	                                                                     gdata_youtube_service_get_primary_authorization_domain (),
	                                                                     "tag:youtube.com,2008:video:RzR2k8yo4NY", NULL,
	                                                                     GDATA_TYPE_YOUTUBE_VIDEO, NULL, NULL));
	g_assert (GDATA_IS_YOUTUBE_VIDEO (data->video));

	uhm_server_end_trace (mock_server);
}

static void
tear_down_comment (CommentData *data, gconstpointer service)
{
	g_object_unref (data->video);
}

static void
assert_comments_feed (GDataFeed *comments_feed)
{
	GList *comments;

	g_assert (GDATA_IS_FEED (comments_feed));

	for (comments = gdata_feed_get_entries (comments_feed); comments != NULL; comments = comments->next) {
		GList *authors;
		GDataYouTubeComment *comment_ = GDATA_YOUTUBE_COMMENT (comments->data);

		/* We can't do much more than this, since we can't reasonably add test comments to public videos, and can't upload a new video
		 * for each test since it has to go through moderation. */
		g_assert_cmpstr (gdata_entry_get_content (GDATA_ENTRY (comment_)), !=, NULL);

		g_assert_cmpuint (g_list_length (gdata_entry_get_authors (GDATA_ENTRY (comment_))), >, 0);

		for (authors = gdata_entry_get_authors (GDATA_ENTRY (comment_)); authors != NULL; authors = authors->next) {
			GDataAuthor *author = GDATA_AUTHOR (authors->data);

			/* Again, we can't test these much. */
			g_assert_cmpstr (gdata_author_get_name (author), !=, NULL);
			g_assert_cmpstr (gdata_author_get_uri (author), !=, NULL);
		}
	}
}

static void
test_comment_query (CommentData *data, gconstpointer service)
{
	GDataFeed *comments_feed;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "comment-query");

	/* Get the comments feed for the video */
	comments_feed = gdata_commentable_query_comments (GDATA_COMMENTABLE (data->video), GDATA_SERVICE (service), NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	assert_comments_feed (comments_feed);

	g_object_unref (comments_feed);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_CLOSURE_FUNCTIONS (comment, CommentData);

GDATA_ASYNC_TEST_FUNCTIONS (comment_query, CommentData,
G_STMT_START {
	gdata_commentable_query_comments_async (GDATA_COMMENTABLE (data->video), GDATA_SERVICE (service), NULL, cancellable, NULL, NULL, NULL,
	                                        async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataFeed *comments_feed;

	comments_feed = gdata_commentable_query_comments_finish (GDATA_COMMENTABLE (obj), async_result, &error);

	if (error == NULL) {
		assert_comments_feed (comments_feed);

		g_object_unref (comments_feed);
	} else {
		g_assert (comments_feed == NULL);
	}
} G_STMT_END);

/* Test that the progress callbacks from gdata_commentable_query_comments_async() are called correctly.
 * We take a CommentData so that we can guarantee the video exists, but we don't use it much as we don't actually care about the specific
 * video. */
static void
test_comment_query_async_progress_closure (CommentData *query_data, gconstpointer service)
{
	GDataAsyncProgressClosure *data = g_slice_new0 (GDataAsyncProgressClosure);

	gdata_test_mock_server_start_trace (mock_server, "comment-query-async-progress-closure");

	data->main_loop = g_main_loop_new (NULL, TRUE);

	gdata_commentable_query_comments_async (GDATA_COMMENTABLE (query_data->video), GDATA_SERVICE (service), NULL, NULL,
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
	CommentData parent;
	GDataYouTubeComment *comment;
} InsertCommentData;

static void
set_up_insert_comment (InsertCommentData *data, gconstpointer service)
{
	set_up_comment ((CommentData*) data, service);

	gdata_test_mock_server_start_trace (mock_server, "setup-insert-comment");

	/* Create a test comment to be inserted. */
	data->comment = gdata_youtube_comment_new (NULL);
	g_assert (GDATA_IS_YOUTUBE_COMMENT (data->comment));

	gdata_entry_set_content (GDATA_ENTRY (data->comment), "This is a test comment.");

	uhm_server_end_trace (mock_server);
}

static void
tear_down_insert_comment (InsertCommentData *data, gconstpointer service)
{
	gdata_test_mock_server_start_trace (mock_server, "teardown-insert-comment");

	if (data->comment != NULL) {
		g_object_unref (data->comment);
	}

	tear_down_comment ((CommentData*) data, service);

	uhm_server_end_trace (mock_server);
}

static void
assert_comments_equal (GDataComment *new_comment,
                       GDataYouTubeComment *original_comment,
                       gboolean allow_empty)
{
	GList *authors;
	GDataAuthor *author;

	g_assert (GDATA_IS_YOUTUBE_COMMENT (new_comment));
	g_assert (GDATA_IS_YOUTUBE_COMMENT (original_comment));
	g_assert (GDATA_YOUTUBE_COMMENT (new_comment) != original_comment);

	/* Comments can be "" if they’ve just been inserted and are pending
	 * moderation. Not much we can do about that without waiting a few
	 * minutes, which would suck in a unit test. */
	if (g_strcmp0 (gdata_entry_get_content (GDATA_ENTRY (new_comment)), "") != 0) {
		g_assert_cmpstr (gdata_entry_get_content (GDATA_ENTRY (new_comment)), ==,
		                 gdata_entry_get_content (GDATA_ENTRY (original_comment)));
	} else {
		g_assert (allow_empty);
	}

	g_assert_cmpstr (gdata_youtube_comment_get_parent_comment_uri (GDATA_YOUTUBE_COMMENT (new_comment)), ==,
	                 gdata_youtube_comment_get_parent_comment_uri (original_comment));

	/* Check the author of the new comment. */
	authors = gdata_entry_get_authors (GDATA_ENTRY (new_comment));
	g_assert_cmpuint (g_list_length (authors), ==, 1);

	author = GDATA_AUTHOR (authors->data);

	g_assert_cmpstr (gdata_author_get_name (author), ==, "GDataTest");
	g_assert_cmpstr (gdata_author_get_uri (author), ==, "http://www.youtube.com/user/GDataTest");
}

static void
test_comment_insert (InsertCommentData *data, gconstpointer service)
{
	GDataComment *new_comment;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "comment-insert");

	new_comment = gdata_commentable_insert_comment (GDATA_COMMENTABLE (data->parent.video), GDATA_SERVICE (service), GDATA_COMMENT (data->comment),
	                                                NULL, &error);
	g_assert_no_error (error);
	g_clear_error (&error);

	assert_comments_equal (new_comment, data->comment, TRUE);

	g_object_unref (new_comment);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_CLOSURE_FUNCTIONS (insert_comment, InsertCommentData);

GDATA_ASYNC_TEST_FUNCTIONS (comment_insert, InsertCommentData,
G_STMT_START {
	gdata_commentable_insert_comment_async (GDATA_COMMENTABLE (data->parent.video), GDATA_SERVICE (service),
	                                        GDATA_COMMENT (data->comment), cancellable, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataComment *new_comment;

	new_comment = gdata_commentable_insert_comment_finish (GDATA_COMMENTABLE (obj), async_result, &error);

	if (error == NULL) {
		assert_comments_equal (new_comment, data->comment, TRUE);

		g_object_unref (new_comment);
	} else {
		g_assert (new_comment == NULL);
	}
} G_STMT_END);

static void
test_comment_delete (InsertCommentData *data, gconstpointer service)
{
	gboolean success;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "comment-delete");

	/* We attempt to delete a comment which hasn't been inserted here, but that doesn't matter as the function should always immediately
	 * return an error because deleting YouTube comments isn't allowed. */
	success = gdata_commentable_delete_comment (GDATA_COMMENTABLE (data->parent.video), GDATA_SERVICE (service), GDATA_COMMENT (data->comment),
	                                            NULL, &error);
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_FORBIDDEN);
	g_assert (success == FALSE);
	g_clear_error (&error);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_TEST_FUNCTIONS (comment_delete, InsertCommentData,
G_STMT_START {
	gdata_commentable_delete_comment_async (GDATA_COMMENTABLE (data->parent.video), GDATA_SERVICE (service),
	                                        GDATA_COMMENT (data->comment), cancellable, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	gboolean success;

	success = gdata_commentable_delete_comment_finish (GDATA_COMMENTABLE (obj), async_result, &error);

	g_assert (error != NULL);
	g_assert (success == FALSE);

	/* See the note above in test_comment_delete(). */
	if (g_error_matches (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_FORBIDDEN) == TRUE) {
		/* Pretend no error happened so that the test succeeds. */
		g_clear_error (&error);
		async_data->cancellation_timeout = 13;
	}
} G_STMT_END);

static void
test_parsing_video_id_from_uri (void)
{
	gchar *video_id;

	video_id = gdata_youtube_video_get_video_id_from_uri ("http://www.youtube.com/watch?v=BH_vwsyCrTc&feature=featured");
	g_assert_cmpstr (video_id, ==, "BH_vwsyCrTc");
	g_free (video_id);

	video_id = gdata_youtube_video_get_video_id_from_uri ("http://www.youtube.es/watch?v=foo");
	g_assert_cmpstr (video_id, ==, "foo");
	g_free (video_id);

	video_id = gdata_youtube_video_get_video_id_from_uri ("http://foobar.com/watch?v=foo");
	g_assert (video_id == NULL);

	video_id = gdata_youtube_video_get_video_id_from_uri ("http://foobar.com/not/real");
	g_assert (video_id == NULL);

	video_id = gdata_youtube_video_get_video_id_from_uri ("http://www.youtube.com/watch#!v=ylLzyHk54Z0");
	g_assert_cmpstr (video_id, ==, "ylLzyHk54Z0");
	g_free (video_id);

	video_id = gdata_youtube_video_get_video_id_from_uri ("http://www.youtube.com/watch#!foo=bar!v=ylLzyHk54Z0");
	g_assert_cmpstr (video_id, ==, "ylLzyHk54Z0");
	g_free (video_id);

	video_id = gdata_youtube_video_get_video_id_from_uri ("http://www.youtube.com/watch#!foo=bar");
	g_assert (video_id == NULL);

	video_id = gdata_youtube_video_get_video_id_from_uri ("http://www.youtube.com/watch#random-fragment");
	g_assert (video_id == NULL);
}

static void
test_categories (gconstpointer service)
{
	GDataAPPCategories *app_categories;
	GList *categories;
	GError *error = NULL;
	gchar *old_locale;
	guint old_n_results;

	gdata_test_mock_server_start_trace (mock_server, "categories");

	app_categories = gdata_youtube_service_get_categories (GDATA_YOUTUBE_SERVICE (service), NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_APP_CATEGORIES (app_categories));
	g_clear_error (&error);

	categories = gdata_app_categories_get_categories (app_categories);
	g_assert_cmpint (g_list_length (categories), >, 0);
	g_assert (GDATA_IS_YOUTUBE_CATEGORY (categories->data));

	/* Save the number of results for comparison against a different locale */
	old_n_results = g_list_length (categories);

	g_object_unref (app_categories);

	/* Test with a different locale */
	old_locale = g_strdup (gdata_service_get_locale (GDATA_SERVICE (service)));
	gdata_service_set_locale (GDATA_SERVICE (service), "TR");

	app_categories = gdata_youtube_service_get_categories (GDATA_YOUTUBE_SERVICE (service), NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_APP_CATEGORIES (app_categories));
	g_clear_error (&error);

	categories = gdata_app_categories_get_categories (app_categories);
	g_assert_cmpint (g_list_length (categories), >, 0);
	g_assert (GDATA_IS_YOUTUBE_CATEGORY (categories->data));

	/* Compare the number of results */
	g_assert_cmpuint (old_n_results, !=, g_list_length (categories));

	g_object_unref (app_categories);

	/* Reset the locale */
	gdata_service_set_locale (GDATA_SERVICE (service), old_locale);
	g_free (old_locale);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_TEST_FUNCTIONS (categories, void,
G_STMT_START {
	gdata_youtube_service_get_categories_async (GDATA_YOUTUBE_SERVICE (service), cancellable, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataAPPCategories *app_categories;
	GList *categories;

	app_categories = gdata_youtube_service_get_categories_finish (GDATA_YOUTUBE_SERVICE (obj), async_result, &error);

	if (error == NULL) {
		g_assert (GDATA_IS_APP_CATEGORIES (app_categories));

		categories = gdata_app_categories_get_categories (app_categories);
		g_assert_cmpint (g_list_length (categories), >, 0);
		g_assert (GDATA_IS_YOUTUBE_CATEGORY (categories->data));

		g_object_unref (app_categories);
	} else {
		g_assert (app_categories == NULL);
	}
} G_STMT_END);

typedef struct {
	GDataEntry *new_video;
	GDataEntry *new_video2;
} BatchData;

static void
setup_batch (BatchData *data, gconstpointer service)
{
	GDataEntry *video;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "setup-batch");

	/* We can't insert new videos as they'd just hit the moderation queue and cause tests to fail. Instead, we rely on two videos already existing
	 * on the server with the given IDs. */
	video = gdata_service_query_single_entry (GDATA_SERVICE (service), gdata_youtube_service_get_primary_authorization_domain (),
	                                          "tag:youtube.com,2008:video:RzR2k8yo4NY", NULL, GDATA_TYPE_YOUTUBE_VIDEO,
	                                          NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));

	data->new_video = video;

	video = gdata_service_query_single_entry (GDATA_SERVICE (service), gdata_youtube_service_get_primary_authorization_domain (),
	                                          "tag:youtube.com,2008:video:VppEcVz8qaI", NULL, GDATA_TYPE_YOUTUBE_VIDEO,
	                                          NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));

	data->new_video2 = video;

	uhm_server_end_trace (mock_server);
}

static void
test_batch (BatchData *data, gconstpointer service)
{
	GDataBatchOperation *operation;
	GDataService *service2;
	gchar *feed_uri;
	guint op_id, op_id2;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "batch");

	/* Here we hardcode the feed URI, but it should really be extracted from a video feed, as the GDATA_LINK_BATCH link.
	 * It looks like this feed is read-only, so we can only test querying. */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_youtube_service_get_primary_authorization_domain (),
	                                              "https://gdata.youtube.com/feeds/api/videos/batch");

	/* Check the properties of the operation */
	g_assert (gdata_batch_operation_get_service (operation) == service);
	g_assert_cmpstr (gdata_batch_operation_get_feed_uri (operation), ==, "https://gdata.youtube.com/feeds/api/videos/batch");

	g_object_get (operation,
	              "service", &service2,
	              "feed-uri", &feed_uri,
	              NULL);

	g_assert (service2 == service);
	g_assert_cmpstr (feed_uri, ==, "https://gdata.youtube.com/feeds/api/videos/batch");

	g_object_unref (service2);
	g_free (feed_uri);

	/* Run a singleton batch operation to query one of the entries. This
	 * should now always fail, as batch operations were deprecated by v3
	 * of the YouTube API. */
	gdata_test_batch_operation_query (operation, gdata_entry_get_id (data->new_video), GDATA_TYPE_YOUTUBE_VIDEO, data->new_video, NULL, NULL);

	g_assert (!gdata_batch_operation_run (operation, NULL, &error));
	g_assert_error (error, GDATA_SERVICE_ERROR,
	                GDATA_SERVICE_ERROR_WITH_BATCH_OPERATION);

	g_clear_error (&error);
	g_object_unref (operation);

	/* Run another batch operation to query the two entries */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_youtube_service_get_primary_authorization_domain (),
	                                              "https://gdata.youtube.com/feeds/api/videos/batch");
	op_id = gdata_test_batch_operation_query (operation, gdata_entry_get_id (data->new_video), GDATA_TYPE_YOUTUBE_VIDEO, data->new_video, NULL,
	                                          NULL);
	op_id2 = gdata_test_batch_operation_query (operation, gdata_entry_get_id (data->new_video2), GDATA_TYPE_YOUTUBE_VIDEO, data->new_video2, NULL,
	                                           NULL);
	g_assert_cmpuint (op_id, !=, op_id2);

	g_assert (!gdata_batch_operation_run (operation, NULL, &error));
	g_assert_error (error, GDATA_SERVICE_ERROR,
	                GDATA_SERVICE_ERROR_WITH_BATCH_OPERATION);

	g_clear_error (&error);
	g_object_unref (operation);

	uhm_server_end_trace (mock_server);
}

static void
test_batch_async_cb (GDataBatchOperation *operation, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GError *error = NULL;

	g_assert (!gdata_batch_operation_run_finish (operation, async_result, &error));
	g_assert_error (error, GDATA_SERVICE_ERROR,
	                GDATA_SERVICE_ERROR_WITH_BATCH_OPERATION);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
}

static void
test_batch_async (BatchData *data, gconstpointer service)
{
	GDataBatchOperation *operation;
	GMainLoop *main_loop;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "batch-async");

	/* Run an async query operation on the video */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_youtube_service_get_primary_authorization_domain (),
	                                              "https://gdata.youtube.com/feeds/api/videos/batch");
	gdata_test_batch_operation_query (operation, gdata_entry_get_id (data->new_video), GDATA_TYPE_YOUTUBE_VIDEO, data->new_video, NULL, &error);

	main_loop = g_main_loop_new (NULL, TRUE);

	gdata_batch_operation_run_async (operation, NULL, (GAsyncReadyCallback) test_batch_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);

	g_assert_error (error, GDATA_SERVICE_ERROR,
	                GDATA_SERVICE_ERROR_WITH_BATCH_OPERATION);
	g_clear_error (&error);

	uhm_server_end_trace (mock_server);
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
test_batch_async_cancellation (BatchData *data, gconstpointer service)
{
	GDataBatchOperation *operation;
	GMainLoop *main_loop;
	GCancellable *cancellable;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "batch-async-cancellation");

	/* Run an async query operation on the video */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_youtube_service_get_primary_authorization_domain (),
	                                              "https://gdata.youtube.com/feeds/api/videos/batch");
	gdata_test_batch_operation_query (operation, gdata_entry_get_id (data->new_video), GDATA_TYPE_YOUTUBE_VIDEO, data->new_video, NULL, &error);

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

	uhm_server_end_trace (mock_server);
}

static void
teardown_batch (BatchData *data, gconstpointer service)
{
	g_object_unref (data->new_video);
	g_object_unref (data->new_video2);
}

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
		uhm_resolver_add_A (resolver, "www.googleapis.com", ip_address);
		uhm_resolver_add_A (resolver, "accounts.google.com",
		                    ip_address);
	}
}

/* Set up a global GDataAuthorizer to be used for all the tests. Unfortunately,
 * the YouTube API is limited to OAuth2 authorisation, so this requires user
 * interaction when online.
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
		return GDATA_AUTHORIZER (gdata_dummy_authorizer_new (GDATA_TYPE_YOUTUBE_SERVICE));
	}

	/* Otherwise, go through the interactive OAuth dance. */
	gdata_test_mock_server_start_trace (mock_server, "global-authentication");
	authorizer = gdata_oauth2_authorizer_new (CLIENT_ID, CLIENT_SECRET,
	                                          REDIRECT_URI,
	                                          GDATA_TYPE_YOUTUBE_SERVICE);

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
	g_assert (gdata_oauth2_authorizer_request_authorization (authorizer,
	                                                         authorisation_code,
	                                                         NULL, &error));
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
	GDataAuthorizer *authorizer = NULL;  /* owned */
	GDataService *service = NULL;  /* owned */
	GFile *trace_directory = NULL;  /* owned */
	gchar *path = NULL;

	gdata_test_init (argc, argv);

	mock_server = gdata_test_get_mock_server ();
	g_signal_connect (G_OBJECT (mock_server), "notify::resolver",
	                  (GCallback) mock_server_notify_resolver_cb, NULL);
	path = g_test_build_filename (G_TEST_DIST, "traces/youtube", NULL);
	trace_directory = g_file_new_for_path (path);
	g_free (path);
	uhm_server_set_trace_directory (mock_server, trace_directory);
	g_object_unref (trace_directory);

	authorizer = create_global_authorizer ();
	service = GDATA_SERVICE (gdata_youtube_service_new (DEVELOPER_KEY,
	                                                    authorizer));

	g_test_add_func ("/youtube/authentication", test_authentication);

	g_test_add_data_func ("/youtube/query/standard_feeds", service, test_query_standard_feeds);
	g_test_add_data_func ("/youtube/query/standard_feed", service, test_query_standard_feed);
	g_test_add_data_func ("/youtube/query/standard_feed/with_query", service, test_query_standard_feed_with_query);
	g_test_add_data_func ("/youtube/query/standard_feed/error", service, test_query_standard_feed_error);
	g_test_add_data_func ("/youtube/query/standard_feed/timeout", service, test_query_standard_feed_timeout);
	g_test_add ("/youtube/query/standard_feed/async", StandardFeedData,
	            service, set_up_standard_feed_async,
	            (void (*)(StandardFeedData *, const void *)) test_query_standard_feed_async,
	            tear_down_standard_feed_async);
	g_test_add_data_func ("/youtube/query/standard_feed/async/progress_closure", service, test_query_standard_feed_async_progress_closure);
	g_test_add ("/youtube/query/standard_feed/async/cancellation",
	            StandardFeedData, service, set_up_standard_feed_async,
	            (void (*)(StandardFeedData *, const void *)) test_query_standard_feed_async_cancellation,
	            tear_down_standard_feed_async);

	g_test_add_data_func ("/youtube/query/related", service, test_query_related);
	g_test_add ("/youtube/query/related/async", GDataAsyncTestData, service, gdata_set_up_async_test_data, test_query_related_async,
	            gdata_tear_down_async_test_data);
	g_test_add_data_func ("/youtube/query/related/async/progress_closure", service, test_query_related_async_progress_closure);
	g_test_add ("/youtube/query/related/async/cancellation", GDataAsyncTestData, service, gdata_set_up_async_test_data,
	            test_query_related_async_cancellation, gdata_tear_down_async_test_data);

	g_test_add ("/youtube/upload/simple", UploadData, service, set_up_upload, test_upload_simple, tear_down_upload);
	g_test_add ("/youtube/upload/async", GDataAsyncTestData, service, set_up_upload_async, test_upload_async, tear_down_upload_async);
	g_test_add ("/youtube/upload/async/cancellation", GDataAsyncTestData, service, set_up_upload_async, test_upload_async_cancellation,
	            tear_down_upload_async);

	g_test_add_data_func ("/youtube/query/single", service, test_query_single);
	g_test_add ("/youtube/query/single/async", GDataAsyncTestData, service, gdata_set_up_async_test_data, test_query_single_async,
	            gdata_tear_down_async_test_data);
	g_test_add ("/youtube/query/single/async/cancellation", GDataAsyncTestData, service, gdata_set_up_async_test_data,
	            test_query_single_async_cancellation, gdata_tear_down_async_test_data);

	g_test_add ("/youtube/comment/query", CommentData, service, set_up_comment, test_comment_query, tear_down_comment);
	g_test_add ("/youtube/comment/query/async", GDataAsyncTestData, service, set_up_comment_async, test_comment_query_async,
	            tear_down_comment_async);
	g_test_add ("/youtube/comment/query/async/cancellation", GDataAsyncTestData, service, set_up_comment_async,
	            test_comment_query_async_cancellation, tear_down_comment_async);
	g_test_add ("/youtube/comment/query/async/progress_closure", CommentData, service, set_up_comment,
	            test_comment_query_async_progress_closure, tear_down_comment);

	g_test_add ("/youtube/comment/insert", InsertCommentData, service, set_up_insert_comment, test_comment_insert,
	            tear_down_insert_comment);
	g_test_add ("/youtube/comment/insert/async", GDataAsyncTestData, service, set_up_insert_comment_async, test_comment_insert_async,
	            tear_down_insert_comment_async);
	g_test_add ("/youtube/comment/insert/async/cancellation", GDataAsyncTestData, service, set_up_insert_comment_async,
	            test_comment_insert_async_cancellation, tear_down_insert_comment_async);

	g_test_add ("/youtube/comment/delete", InsertCommentData, service, set_up_insert_comment, test_comment_delete,
	            tear_down_insert_comment);
	g_test_add ("/youtube/comment/delete/async", GDataAsyncTestData, service, set_up_insert_comment_async, test_comment_delete_async,
	            tear_down_insert_comment_async);
	g_test_add ("/youtube/comment/delete/async/cancellation", GDataAsyncTestData, service, set_up_insert_comment_async,
	            test_comment_delete_async_cancellation, tear_down_insert_comment_async);

	g_test_add_data_func ("/youtube/categories", service, test_categories);
	g_test_add ("/youtube/categories/async", GDataAsyncTestData, service, gdata_set_up_async_test_data, test_categories_async,
	            gdata_tear_down_async_test_data);
	g_test_add ("/youtube/categories/async/cancellation", GDataAsyncTestData, service, gdata_set_up_async_test_data,
	            test_categories_async_cancellation, gdata_tear_down_async_test_data);

	g_test_add ("/youtube/batch", BatchData, service, setup_batch, test_batch, teardown_batch);
	g_test_add ("/youtube/batch/async", BatchData, service, setup_batch, test_batch_async, teardown_batch);
	g_test_add ("/youtube/batch/async/cancellation", BatchData, service, setup_batch, test_batch_async_cancellation, teardown_batch);

	g_test_add_func ("/youtube/service/properties", test_service_properties);

	g_test_add_func ("/youtube/parsing/app:control", test_parsing_app_control);
	g_test_add_func ("/youtube/parsing/yt:recorded", test_parsing_yt_recorded);
	g_test_add_func ("/youtube/parsing/yt:accessControl", test_parsing_yt_access_control);
	g_test_add_func ("/youtube/parsing/yt:category", test_parsing_yt_category);
	g_test_add_func ("/youtube/parsing/video_id_from_uri", test_parsing_video_id_from_uri);
	g_test_add_func ("/youtube/parsing/georss:where", test_parsing_georss_where);
	g_test_add_func ("/youtube/parsing/ratings", test_parsing_ratings);

	g_test_add_func ("/youtube/video/escaping", test_video_escaping);
	g_test_add_func ("/youtube/video/location", test_video_location);

	g_test_add_func ("/youtube/comment/get_json", test_comment_get_json);
	g_test_add_func ("/youtube/comment/properties/parent-comment-id", test_comment_properties_parent_comment_uri);

	g_test_add_func ("/youtube/query/uri", test_query_uri);
	g_test_add_func ("/youtube/query/etag", test_query_etag);

	retval = g_test_run ();

	g_clear_object (&service);
	g_clear_object (&authorizer);

	return retval;
}
