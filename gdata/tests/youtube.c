/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2008–2010 <philip@tecnocode.co.uk>
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

#define DEVELOPER_KEY "AI39si7Me3Q7zYs6hmkFvpRBD2nrkVjYYsUO5lh_3HdOkGRc9g6Z4nzxZatk_aAo2EsA21k7vrda0OO6oFg2rnhMedZXPyXoEw"

static GDataMockServer *mock_server = NULL;

/* Effectively gdata_test_mock_server_start_trace() but calling gdata_mock_server_run() instead of gdata_mock_server_start_trace(). */
static void
gdata_test_mock_server_run (GDataMockServer *server)
{
	const gchar *ip_address;
	GDataMockResolver *resolver;

	gdata_mock_server_run (server);
	gdata_test_set_https_port (server);

	if (gdata_mock_server_get_enable_online (server) == FALSE) {
		/* Set up the expected domain names here. This should technically be split up between
		 * the different unit test suites, but that's too much effort. */
		ip_address = soup_address_get_physical (gdata_mock_server_get_address (server));
		resolver = gdata_mock_server_get_resolver (server);

		gdata_mock_resolver_add_A (resolver, "www.google.com", ip_address);
		gdata_mock_resolver_add_A (resolver, "gdata.youtube.com", ip_address);
		gdata_mock_resolver_add_A (resolver, "uploads.gdata.youtube.com", ip_address);
	}
}

static void
test_authentication (void)
{
	gboolean retval;
	GDataClientLoginAuthorizer *authorizer;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "authentication");

	/* Create an authorizer */
	authorizer = gdata_client_login_authorizer_new (CLIENT_ID, GDATA_TYPE_YOUTUBE_SERVICE);

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
	                                                     gdata_youtube_service_get_primary_authorization_domain ()) == TRUE);

	g_object_unref (authorizer);

	gdata_mock_server_end_trace (mock_server);
}

/* HTTP message responses and the expected associated GData error domain/code. */
static const GDataTestRequestErrorData authentication_errors[] = {
	/* Generic network errors. */
	{ SOUP_STATUS_BAD_REQUEST, "Bad Request", "Invalid parameter ‘foobar’.",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_PROTOCOL_ERROR },
	{ SOUP_STATUS_NOT_FOUND, "Not Found", "Login page wasn't found for no good reason at all.",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_NOT_FOUND },
	{ SOUP_STATUS_PRECONDITION_FAILED, "Precondition Failed", "Not allowed to log in at this time, possibly.",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_CONFLICT },
	{ SOUP_STATUS_INTERNAL_SERVER_ERROR, "Internal Server Error", "Whoops.",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_PROTOCOL_ERROR },
	/* Specific authentication errors. */
	{ SOUP_STATUS_FORBIDDEN, "Access Forbidden", "Error=BadAuthentication\n",
	  gdata_client_login_authorizer_error_quark, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_BAD_AUTHENTICATION },
	{ SOUP_STATUS_FORBIDDEN, "Access Forbidden", "Error=BadAuthentication\nInfo=InvalidSecondFactor\n",
	  gdata_client_login_authorizer_error_quark, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_INVALID_SECOND_FACTOR },
	{ SOUP_STATUS_FORBIDDEN, "Access Forbidden", "Error=NotVerified\nUrl=http://example.com/\n",
	  gdata_client_login_authorizer_error_quark, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_NOT_VERIFIED },
	{ SOUP_STATUS_FORBIDDEN, "Access Forbidden", "Error=TermsNotAgreed\nUrl=http://example.com/\n",
	  gdata_client_login_authorizer_error_quark, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_TERMS_NOT_AGREED },
	{ SOUP_STATUS_FORBIDDEN, "Access Forbidden", "Error=Unknown\nUrl=http://example.com/\n",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED },
	{ SOUP_STATUS_FORBIDDEN, "Access Forbidden", "Error=AccountDeleted\nUrl=http://example.com/\n",
	  gdata_client_login_authorizer_error_quark, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_ACCOUNT_DELETED },
	{ SOUP_STATUS_FORBIDDEN, "Access Forbidden", "Error=AccountDisabled\nUrl=http://example.com/\n",
	  gdata_client_login_authorizer_error_quark, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_ACCOUNT_DISABLED },
	{ SOUP_STATUS_FORBIDDEN, "Access Forbidden", "Error=AccountMigrated\nUrl=http://example.com/\n",
	  gdata_client_login_authorizer_error_quark, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_ACCOUNT_MIGRATED },
	{ SOUP_STATUS_FORBIDDEN, "Access Forbidden", "Error=ServiceDisabled\nUrl=http://example.com/\n",
	  gdata_client_login_authorizer_error_quark, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_SERVICE_DISABLED },
	{ SOUP_STATUS_FORBIDDEN, "Access Forbidden", "Error=ServiceUnavailable\nUrl=http://example.com/\n",
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_UNAVAILABLE },
	/* Malformed authentication errors to test parser error handling. */
	{ SOUP_STATUS_INTERNAL_SERVER_ERROR, "Access Forbidden", "Error=BadAuthentication", /* missing Error delimiter */
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_PROTOCOL_ERROR },
	{ SOUP_STATUS_INTERNAL_SERVER_ERROR, "Access Forbidden", "Error=AccountDeleted\n", /* missing Url */
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_PROTOCOL_ERROR },
	{ SOUP_STATUS_INTERNAL_SERVER_ERROR, "Access Forbidden", "Error=AccountDeleted\nUrl=http://example.com/", /* missing Url delimiter */
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_PROTOCOL_ERROR },
	{ SOUP_STATUS_INTERNAL_SERVER_ERROR, "Access Forbidden", "", /* missing Error */
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_PROTOCOL_ERROR },
	{ SOUP_STATUS_INTERNAL_SERVER_ERROR, "Access Forbidden", "Error=", /* missing Error */
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_PROTOCOL_ERROR },
	{ SOUP_STATUS_INTERNAL_SERVER_ERROR, "Access Forbidden", "Error=Foobar\nUrl=http://example.com/\n", /* unknown Error */
	  gdata_service_error_quark, GDATA_SERVICE_ERROR_PROTOCOL_ERROR },
};

static void
test_authentication_error (void)
{
	gboolean retval;
	GDataClientLoginAuthorizer *authorizer;
	GError *error = NULL;
	gulong handler_id;
	guint i;

	if (gdata_mock_server_get_enable_logging (mock_server) == TRUE) {
		g_test_message ("Ignoring test due to logging being enabled.");
		return;
	} else if (gdata_mock_server_get_enable_online (mock_server) == TRUE) {
		g_test_message ("Ignoring test due to running online and test not being reproducible.");
		return;
	}

	for (i = 0; i < G_N_ELEMENTS (authentication_errors); i++) {
		const GDataTestRequestErrorData *data = &authentication_errors[i];

		handler_id = g_signal_connect (mock_server, "handle-message", (GCallback) gdata_test_mock_server_handle_message_error, (gpointer) data);
		gdata_test_mock_server_run (mock_server);

		/* Create an authorizer */
		authorizer = gdata_client_login_authorizer_new (CLIENT_ID, GDATA_TYPE_YOUTUBE_SERVICE);

		g_assert_cmpstr (gdata_client_login_authorizer_get_client_id (authorizer), ==, CLIENT_ID);

		/* Log in */
		retval = gdata_client_login_authorizer_authenticate (authorizer, USERNAME, PASSWORD, NULL, &error);
		g_assert_error (error, data->error_domain_func (), data->error_code);
		g_assert (retval == FALSE);
		g_clear_error (&error);

		/* Check nothing's changed in the authoriser. */
		g_assert_cmpstr (gdata_client_login_authorizer_get_username (authorizer), ==, NULL);
		g_assert_cmpstr (gdata_client_login_authorizer_get_password (authorizer), ==, NULL);

		g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
		                                                     gdata_youtube_service_get_primary_authorization_domain ()) == FALSE);

		g_object_unref (authorizer);

		gdata_mock_server_stop (mock_server);
		g_signal_handler_disconnect (mock_server, handler_id);
	}
}

static void
test_authentication_timeout (void)
{
	gboolean retval;
	GDataClientLoginAuthorizer *authorizer;
	GError *error = NULL;
	gulong handler_id;

	if (gdata_mock_server_get_enable_logging (mock_server) == TRUE) {
		g_test_message ("Ignoring test due to logging being enabled.");
		return;
	} else if (gdata_mock_server_get_enable_online (mock_server) == TRUE) {
		g_test_message ("Ignoring test due to running online and test not being reproducible.");
		return;
	}

	handler_id = g_signal_connect (mock_server, "handle-message", (GCallback) gdata_test_mock_server_handle_message_timeout, NULL);
	gdata_mock_server_run (mock_server);
	gdata_test_set_https_port (mock_server);

	/* Create an authorizer and set its timeout as low as possible (1 second). */
	authorizer = gdata_client_login_authorizer_new (CLIENT_ID, GDATA_TYPE_YOUTUBE_SERVICE);
	gdata_client_login_authorizer_set_timeout (authorizer, 1);

	g_assert_cmpstr (gdata_client_login_authorizer_get_client_id (authorizer), ==, CLIENT_ID);

	/* Log in */
	retval = gdata_client_login_authorizer_authenticate (authorizer, USERNAME, PASSWORD, NULL, &error);
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_NETWORK_ERROR);
	g_assert (retval == FALSE);
	g_clear_error (&error);

	/* Check nothing's changed in the authoriser. */
	g_assert_cmpstr (gdata_client_login_authorizer_get_username (authorizer), ==, NULL);
	g_assert_cmpstr (gdata_client_login_authorizer_get_password (authorizer), ==, NULL);

	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	                                                     gdata_youtube_service_get_primary_authorization_domain ()) == FALSE);

	g_object_unref (authorizer);

	gdata_mock_server_stop (mock_server);
	g_signal_handler_disconnect (mock_server, handler_id);
}

GDATA_ASYNC_TEST_FUNCTIONS (authentication, void,
G_STMT_START {
	GDataClientLoginAuthorizer *authorizer;

	/* Create an authorizer */
	authorizer = gdata_client_login_authorizer_new (CLIENT_ID, GDATA_TYPE_YOUTUBE_SERVICE);

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
		                                                     gdata_youtube_service_get_primary_authorization_domain ()) == TRUE);
	} else {
		g_assert (retval == FALSE);

		/* Check nothing's changed */
		g_assert_cmpstr (gdata_client_login_authorizer_get_username (authorizer), ==, NULL);
		g_assert_cmpstr (gdata_client_login_authorizer_get_password (authorizer), ==, NULL);

		g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
		                                                     gdata_youtube_service_get_primary_authorization_domain ()) == FALSE);
	}
} G_STMT_END);

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
	struct {
		GDataYouTubeStandardFeedType feed_type;
		const gchar *expected_title;
	} feeds[] = {
		/* This must be kept up-to-date with GDataYouTubeStandardFeedType. */
		{ GDATA_YOUTUBE_TOP_RATED_FEED, "Top Rated" },
		{ GDATA_YOUTUBE_TOP_FAVORITES_FEED, "Top Favorites" },
		{ GDATA_YOUTUBE_MOST_VIEWED_FEED, "Most Popular" },
		{ GDATA_YOUTUBE_MOST_POPULAR_FEED, "Most Popular" },
		{ GDATA_YOUTUBE_MOST_RECENT_FEED, "Most Recent" },
		{ GDATA_YOUTUBE_MOST_DISCUSSED_FEED, "Most Discussed" },
		{ GDATA_YOUTUBE_MOST_LINKED_FEED, NULL },
		{ GDATA_YOUTUBE_MOST_RESPONDED_FEED, "Most Responded" },
		{ GDATA_YOUTUBE_RECENTLY_FEATURED_FEED, "Spotlight Videos" },
		{ GDATA_YOUTUBE_WATCH_ON_MOBILE_FEED, NULL },
	};

	gdata_test_mock_server_start_trace (mock_server, "query-standard-feeds");

	for (i = 0; i < G_N_ELEMENTS (feeds); i++) {
		feed = gdata_youtube_service_query_standard_feed (GDATA_YOUTUBE_SERVICE (service), feeds[i].feed_type, NULL, NULL, NULL, NULL, &error);
		g_assert_no_error (error);
		g_assert (GDATA_IS_FEED (feed));
		g_clear_error (&error);

		g_assert_cmpstr (gdata_feed_get_title (feed), ==, feeds[i].expected_title);

		g_object_unref (feed);
	}

	gdata_mock_server_end_trace (mock_server);
}

static void
test_query_standard_feed (gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "query-standard-feed");

	feed = gdata_youtube_service_query_standard_feed (GDATA_YOUTUBE_SERVICE (service), GDATA_YOUTUBE_TOP_RATED_FEED, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	g_assert_cmpstr (gdata_feed_get_title (feed), ==, "Top Rated");

	g_object_unref (feed);

	gdata_mock_server_end_trace (mock_server);
}

static void
test_query_standard_feed_with_query (gconstpointer service)
{
	GDataYouTubeQuery *query;
	GDataFeed *feed;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "query-standard-feed-with-query");

	query = gdata_youtube_query_new (NULL);
	gdata_youtube_query_set_language (query, "fr");

	feed = gdata_youtube_service_query_standard_feed (GDATA_YOUTUBE_SERVICE (service), GDATA_YOUTUBE_TOP_RATED_FEED, GDATA_QUERY (query), NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	g_assert_cmpstr (gdata_feed_get_title (feed), ==, "Top Rated");

	g_object_unref (query);
	g_object_unref (feed);

	gdata_mock_server_end_trace (mock_server);
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
};

static void
test_query_standard_feed_error (gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;
	gulong handler_id;
	guint i;

	if (gdata_mock_server_get_enable_logging (mock_server) == TRUE) {
		g_test_message ("Ignoring test due to logging being enabled.");
		return;
	} else if (gdata_mock_server_get_enable_online (mock_server) == TRUE) {
		g_test_message ("Ignoring test due to running online and test not being reproducible.");
		return;
	}

	for (i = 0; i < G_N_ELEMENTS (query_standard_feed_errors); i++) {
		const GDataTestRequestErrorData *data = &query_standard_feed_errors[i];

		handler_id = g_signal_connect (mock_server, "handle-message", (GCallback) gdata_test_mock_server_handle_message_error, (gpointer) data);
		gdata_test_mock_server_run (mock_server);

		/* Query the feed. */
		feed = gdata_youtube_service_query_standard_feed (GDATA_YOUTUBE_SERVICE (service), GDATA_YOUTUBE_TOP_RATED_FEED, NULL, NULL, NULL, NULL, &error);
		g_assert_error (error, data->error_domain_func (), data->error_code);
		g_assert (feed == NULL);
		g_clear_error (&error);

		gdata_mock_server_stop (mock_server);
		g_signal_handler_disconnect (mock_server, handler_id);
	}
}

static void
test_query_standard_feed_timeout (gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;
	gulong handler_id;

	if (gdata_mock_server_get_enable_logging (mock_server) == TRUE) {
		g_test_message ("Ignoring test due to logging being enabled.");
		return;
	} else if (gdata_mock_server_get_enable_online (mock_server) == TRUE) {
		g_test_message ("Ignoring test due to running online and test not being reproducible.");
		return;
	}

	handler_id = g_signal_connect (mock_server, "handle-message", (GCallback) gdata_test_mock_server_handle_message_timeout, NULL);
	gdata_test_mock_server_run (mock_server);

	/* Set the service's timeout as low as possible (1 second). */
	gdata_service_set_timeout (GDATA_SERVICE (service), 1);

	/* Query the feed. */
	feed = gdata_youtube_service_query_standard_feed (GDATA_YOUTUBE_SERVICE (service), GDATA_YOUTUBE_TOP_RATED_FEED, NULL, NULL, NULL, NULL, &error);
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_NETWORK_ERROR);
	g_assert (feed == NULL);
	g_clear_error (&error);

	gdata_mock_server_stop (mock_server);
	g_signal_handler_disconnect (mock_server, handler_id);
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
		/* TODO: Tests? */

		g_object_unref (feed);
	} else {
		g_assert (feed == NULL);
	}
} G_STMT_END);

static void
test_query_standard_feed_async_progress_closure (gconstpointer service)
{
	GDataAsyncProgressClosure *data = g_slice_new0 (GDataAsyncProgressClosure);

	g_assert (service != NULL);

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

	gdata_mock_server_end_trace (mock_server);
}

static GDataYouTubeVideo *
get_video_for_related (void)
{
	GDataYouTubeVideo *video;
	GError *error = NULL;

	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
			"xmlns:media='http://search.yahoo.com/mrss/' "
			"xmlns:yt='http://gdata.youtube.com/schemas/2007' "
			"xmlns:georss='http://www.georss.org/georss' "
			"xmlns:gd='http://schemas.google.com/g/2005' "
			"xmlns:gml='http://www.opengis.net/gml'>"
			"<id>http://gdata.youtube.com/feeds/api/videos/q1UPMEmCqZo</id>"
			"<published>2009-02-12T20:34:08.000Z</published>"
			"<updated>2009-02-21T13:00:13.000Z</updated>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/keywords.cat' term='part one'/>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/categories.cat' term='Film' label='Film &amp; Animation'/>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'/>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/keywords.cat' term='ian purchase'/>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/keywords.cat' term='purchase brothers'/>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/keywords.cat' term='half life 2'/>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/keywords.cat' term='escape from city 17'/>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/keywords.cat' term='Half Life'/>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/keywords.cat' term='david purchase'/>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/keywords.cat' term='half-life'/>"
			"<title type='text'>Escape From City 17 - Part One</title>"
			"<content type='text'>Directed by The Purchase Brothers. *snip*</content>"
			"<link rel='http://www.iana.org/assignments/relation/alternate' type='text/html' href='http://www.youtube.com/watch?v=q1UPMEmCqZo'/>"
			"<link rel='http://gdata.youtube.com/schemas/2007#video.related' type='application/atom+xml' href='http://gdata.youtube.com/feeds/api/videos/q1UPMEmCqZo/related'/>"
			"<link rel='http://gdata.youtube.com/schemas/2007#mobile' type='text/html' href='http://m.youtube.com/details?v=q1UPMEmCqZo'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' href='http://gdata.youtube.com/feeds/api/standardfeeds/top_rated/v/q1UPMEmCqZo'/>"
			"<author>"
				"<name>PurchaseBrothers</name>"
				"<uri>http://gdata.youtube.com/feeds/api/users/purchasebrothers</uri>"
			"</author>"
			"<media:group>"
				"<media:title type='plain'>Escape From City 17 - Part One</media:title>"
				"<media:description type='plain'>Directed by The Purchase Brothers. *snip*</media:description>"
				"<media:keywords>Half Life, escape from city 17, half-life, half life 2, part one, purchase brothers, david purchase, ian purchase</media:keywords>"
				"<yt:duration seconds='330'/>"
				"<media:category label='Film &amp; Animation' scheme='http://gdata.youtube.com/schemas/2007/categories.cat'>Film</media:category>"
				"<media:content url='http://www.youtube.com/v/q1UPMEmCqZo&amp;f=standard&amp;app=youtube_gdata' type='application/x-shockwave-flash' medium='video' isDefault='true' expression='full' duration='330' yt:format='5'/>"
				"<media:content url='rtsp://rtsp2.youtube.com/CiQLENy73wIaGwmaqYJJMA9VqxMYDSANFEgGUghzdGFuZGFyZAw=/0/0/0/video.3gp' type='video/3gpp' medium='video' expression='full' duration='330' yt:format='1'/>"
				"<media:content url='rtsp://rtsp2.youtube.com/CiQLENy73wIaGwmaqYJJMA9VqxMYESARFEgGUghzdGFuZGFyZAw=/0/0/0/video.3gp' type='video/3gpp' medium='video' expression='full' duration='330' yt:format='6'/>"
				"<media:thumbnail url='http://i.ytimg.com/vi/q1UPMEmCqZo/2.jpg' height='97' width='130' time='00:02:45'/>"
				"<media:thumbnail url='http://i.ytimg.com/vi/q1UPMEmCqZo/1.jpg' height='97' width='130' time='00:01:22.500'/>"
				"<media:thumbnail url='http://i.ytimg.com/vi/q1UPMEmCqZo/3.jpg' height='97' width='130' time='00:04:07.500'/>"
				"<media:thumbnail url='http://i.ytimg.com/vi/q1UPMEmCqZo/0.jpg' height='240' width='320' time='00:02:45'/>"
				"<media:player url='http://www.youtube.com/watch?v=q1UPMEmCqZo'/>"
			"</media:group>"
			"<yt:statistics viewCount='1683289' favoriteCount='29963'/>"
			"<gd:rating min='1' max='5' numRaters='24550' average='4.95'/>"
			"<georss:where>"
				"<gml:Point>"
					"<gml:pos>43.661911057260674 -79.37759399414062</gml:pos>"
				"</gml:Point>"
			"</georss:where>"
			"<gd:comments>"
				"<gd:feedLink href='http://gdata.youtube.com/feeds/api/videos/q1UPMEmCqZo/comments' countHint='13021'/>"
			"</gd:comments>"
		"</entry>", -1, &error));
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

	gdata_mock_server_end_trace (mock_server);
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

	gdata_mock_server_end_trace (mock_server);
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
	GError *error = NULL;

	data->service = g_object_ref ((gpointer) service);

	/* Create the metadata for the video being uploaded */
	data->video = gdata_youtube_video_new (NULL);

	gdata_entry_set_title (GDATA_ENTRY (data->video), "Bad Wedding Toast");
	gdata_youtube_video_set_description (data->video, "I gave a bad toast at my friend's wedding.");
	category = gdata_media_category_new ("People", "http://gdata.youtube.com/schemas/2007/categories.cat", NULL);
	gdata_youtube_video_set_category (data->video, category);
	g_object_unref (category);
	gdata_youtube_video_set_keywords (data->video, tags);

	/* Get a file to upload */
	/* TODO: fix the path */
	data->video_file = g_file_new_for_path (TEST_FILE_DIR "sample.ogg");

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

	gdata_mock_server_end_trace (mock_server);
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

	gdata_mock_server_end_trace (mock_server);
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

	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
			"xmlns:media='http://search.yahoo.com/mrss/' "
			"xmlns:yt='http://gdata.youtube.com/schemas/2007' "
			"xmlns:gd='http://schemas.google.com/g/2005' "
			"gd:etag='W/\"CEMFSX47eCp7ImA9WxVUGEw.\"'>"
			"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
			"<published>2006-05-16T14:06:37.000Z</published>"
			"<updated>2009-03-23T12:46:58.000Z</updated>"
			"<app:control xmlns:app='http://www.w3.org/2007/app'>"
				"<app:draft>yes</app:draft>"
				"<yt:state name='blacklisted'>This video is not available in your country</yt:state>"
			"</app:control>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'/>"
			"<title>Judas Priest - Painkiller</title>"
			"<link rel='http://www.iana.org/assignments/relation/alternate' type='text/html' href='http://www.youtube.com/watch?v=JAagedeKdcQ'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ?client=ytapi-google-jsdemo'/>"
			"<author>"
				"<name>eluves</name>"
				"<uri>http://gdata.youtube.com/feeds/api/users/eluves</uri>"
			"</author>"
			"<media:group>"
				"<media:title type='plain'>Judas Priest - Painkiller</media:title>"
				"<media:credit role='uploader' scheme='urn:youtube'>eluves</media:credit>"
				"<media:category label='Music' scheme='http://gdata.youtube.com/schemas/2007/categories.cat'>Music</media:category>"
			"</media:group>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	/* Test the app:control values */
	g_assert (gdata_youtube_video_is_draft (video) == TRUE);

	state = gdata_youtube_video_get_state (video);
	g_assert_cmpstr (gdata_youtube_state_get_name (state), ==, "blacklisted");
	g_assert_cmpstr (gdata_youtube_state_get_message (state), ==, "This video is not available in your country");
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

	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
			"xmlns:media='http://search.yahoo.com/mrss/' "
			"xmlns:yt='http://gdata.youtube.com/schemas/2007' "
			"xmlns:gd='http://schemas.google.com/g/2005' "
			"gd:etag='W/\"CEMFSX47eCp7ImA9WxVUGEw.\"'>"
			"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
			"<published>2006-05-16T14:06:37.000Z</published>"
			"<updated>2009-03-23T12:46:58.000Z</updated>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'/>"
			"<title>Judas Priest - Painkiller</title>"
			"<link rel='http://www.iana.org/assignments/relation/alternate' type='text/html' href='http://www.youtube.com/watch?v=JAagedeKdcQ'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ?client=ytapi-google-jsdemo'/>"
			"<author>"
				"<name>eluves</name>"
				"<uri>http://gdata.youtube.com/feeds/api/users/eluves</uri>"
			"</author>"
			"<media:group>"
				"<media:title type='plain'>Judas Priest - Painkiller</media:title>"
				"<media:credit role='uploader' scheme='urn:youtube'>eluves</media:credit>"
				"<media:category label='Music' scheme='http://gdata.youtube.com/schemas/2007/categories.cat'>Music</media:category>"
			"</media:group>"
			"<yt:recorded>2003-08-03</yt:recorded>"
		"</entry>", -1, &error));
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
	gdata_test_assert_xml (video,
			 "<?xml version='1.0' encoding='UTF-8'?>"
			 "<entry xmlns='http://www.w3.org/2005/Atom' "
				"xmlns:media='http://search.yahoo.com/mrss/' "
				"xmlns:gd='http://schemas.google.com/g/2005' "
				"xmlns:yt='http://gdata.youtube.com/schemas/2007' "
				"xmlns:app='http://www.w3.org/2007/app' "
				"xmlns:georss='http://www.georss.org/georss' "
				"xmlns:gml='http://www.opengis.net/gml' "
				"gd:etag='W/\"CEMFSX47eCp7ImA9WxVUGEw.\"'>"
				"<title type='text'>Judas Priest - Painkiller</title>"
				"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
				"<updated>2009-03-23T12:46:58Z</updated>"
				"<published>2006-05-16T14:06:37Z</published>"
				"<category term='http://gdata.youtube.com/schemas/2007#video' scheme='http://schemas.google.com/g/2005#kind'/>"
				"<link href='http://www.youtube.com/watch?v=JAagedeKdcQ' rel='http://www.iana.org/assignments/relation/alternate' type='text/html'/>"
				"<link href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ?client=ytapi-google-jsdemo' rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml'/>"
				"<author>"
					"<name>eluves</name>"
					"<uri>http://gdata.youtube.com/feeds/api/users/eluves</uri>"
				"</author>"
				"<media:group>"
					"<media:category scheme='http://gdata.youtube.com/schemas/2007/categories.cat' label='Music'>Music</media:category>"
					"<media:title type='plain'>Judas Priest - Painkiller</media:title>"
				"</media:group>"
				"<yt:recorded>2005-10-02</yt:recorded>"
				"<app:control>"
					"<app:draft>no</app:draft>"
				"</app:control>"
			 "</entry>");

	/* TODO: more tests on entry properties */

	g_object_unref (video);
}

static void
test_parsing_yt_access_control (void)
{
	GDataYouTubeVideo *video;
	GError *error = NULL;

	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
			"xmlns:media='http://search.yahoo.com/mrss/' "
			"xmlns:yt='http://gdata.youtube.com/schemas/2007' "
			"xmlns:gd='http://schemas.google.com/g/2005' "
			"gd:etag='W/\"CEMFSX47eCp7ImA9WxVUGEw.\"'>"
			"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
			"<published>2006-05-16T14:06:37.000Z</published>"
			"<updated>2009-03-23T12:46:58.000Z</updated>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'/>"
			"<title>Judas Priest - Painkiller</title>"
			"<link rel='http://www.iana.org/assignments/relation/alternate' type='text/html' href='http://www.youtube.com/watch?v=JAagedeKdcQ'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ?client=ytapi-google-jsdemo'/>"
			"<author>"
				"<name>eluves</name>"
				"<uri>http://gdata.youtube.com/feeds/api/users/eluves</uri>"
			"</author>"
			"<media:group>"
				"<media:title type='plain'>Judas Priest - Painkiller</media:title>"
				"<media:credit role='uploader' scheme='urn:youtube'>eluves</media:credit>"
				"<media:category label='Music' scheme='http://gdata.youtube.com/schemas/2007/categories.cat'>Music</media:category>"
			"</media:group>"
			"<yt:accessControl action='rate' permission='allowed'/>"
			"<yt:accessControl action='comment' permission='moderated'/>"
			"<yt:accessControl action='commentVote' permission='denied'/>"
			"<yt:accessControl action='videoRespond' permission='allowed'/>"
			"<yt:accessControl action='syndicate' permission='denied'/>"
			"<yt:accessControl action='random' permission='moderated'/>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	/* Test the access controls */
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_RATE), ==, GDATA_YOUTUBE_PERMISSION_ALLOWED);
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_COMMENT), ==, GDATA_YOUTUBE_PERMISSION_MODERATED);
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_COMMENT_VOTE), ==, GDATA_YOUTUBE_PERMISSION_DENIED);
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_VIDEO_RESPOND), ==, GDATA_YOUTUBE_PERMISSION_ALLOWED);
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_EMBED), ==, GDATA_YOUTUBE_PERMISSION_DENIED);
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_SYNDICATE), ==, GDATA_YOUTUBE_PERMISSION_DENIED);

	/* Update some of them and see if the XML's written out OK */
	gdata_youtube_video_set_access_control (video, GDATA_YOUTUBE_ACTION_RATE, GDATA_YOUTUBE_PERMISSION_MODERATED);
	gdata_youtube_video_set_access_control (video, GDATA_YOUTUBE_ACTION_EMBED, GDATA_YOUTUBE_PERMISSION_DENIED);

	/* Check the XML */
	gdata_test_assert_xml (video,
			 "<?xml version='1.0' encoding='UTF-8'?>"
			 "<entry xmlns='http://www.w3.org/2005/Atom' "
				"xmlns:media='http://search.yahoo.com/mrss/' "
				"xmlns:gd='http://schemas.google.com/g/2005' "
				"xmlns:yt='http://gdata.youtube.com/schemas/2007' "
				"xmlns:app='http://www.w3.org/2007/app' "
				"xmlns:georss='http://www.georss.org/georss' "
				"xmlns:gml='http://www.opengis.net/gml' "
				"gd:etag='W/\"CEMFSX47eCp7ImA9WxVUGEw.\"'>"
				"<title type='text'>Judas Priest - Painkiller</title>"
				"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
				"<updated>2009-03-23T12:46:58Z</updated>"
				"<published>2006-05-16T14:06:37Z</published>"
				"<category term='http://gdata.youtube.com/schemas/2007#video' scheme='http://schemas.google.com/g/2005#kind'/>"
				"<link href='http://www.youtube.com/watch?v=JAagedeKdcQ' rel='http://www.iana.org/assignments/relation/alternate' type='text/html'/>"
				"<link href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ?client=ytapi-google-jsdemo' rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml'/>"
				"<author>"
					"<name>eluves</name>"
					"<uri>http://gdata.youtube.com/feeds/api/users/eluves</uri>"
				"</author>"
				"<media:group>"
					"<media:category scheme='http://gdata.youtube.com/schemas/2007/categories.cat' label='Music'>Music</media:category>"
					"<media:title type='plain'>Judas Priest - Painkiller</media:title>"
				"</media:group>"
				"<yt:accessControl action='embed' permission='denied'/>"
				"<yt:accessControl action='random' permission='moderated'/>"
				"<yt:accessControl action='commentVote' permission='denied'/>"
				"<yt:accessControl action='rate' permission='moderated'/>"
				"<yt:accessControl action='comment' permission='moderated'/>"
				"<yt:accessControl action='syndicate' permission='denied'/>"
				"<yt:accessControl action='videoRespond' permission='allowed'/>"
				"<app:control>"
					"<app:draft>no</app:draft>"
				"</app:control>"
			 "</entry>");

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

/*static void
test_parsing_comments_feed_link (void)
{
	GDataYouTubeVideo *video;
	GDataGDFeedLink *feed_link;
	GError *error = NULL;

	video = gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
			"xmlns:media='http://search.yahoo.com/mrss/' "
			"xmlns:yt='http://gdata.youtube.com/schemas/2007' "
			"xmlns:gd='http://schemas.google.com/g/2005' "
			"gd:etag='W/\"CEMFSX47eCp7ImA9WxVUGEw.\"'>"
			"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
			"<published>2006-05-16T14:06:37.000Z</published>"
			"<updated>2009-03-23T12:46:58.000Z</updated>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'/>"
			"<title>Judas Priest - Painkiller</title>"
			"<link rel='http://www.iana.org/assignments/relation/alternate' type='text/html' href='http://www.youtube.com/watch?v=JAagedeKdcQ'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ?client=ytapi-google-jsdemo'/>"
			"<author>"
				"<name>eluves</name>"
				"<uri>http://gdata.youtube.com/feeds/api/users/eluves</uri>"
			"</author>"
			"<media:group>"
				"<media:title type='plain'>Judas Priest - Painkiller</media:title>"
				"<media:credit role='uploader' scheme='urn:youtube'>eluves</media:credit>"
				"<media:category label='Music' scheme='http://gdata.youtube.com/schemas/2007/categories.cat'>Music</media:category>"
			"</media:group>"
			"<gd:comments>"
				"<gd:feedLink href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ/comments' countHint='13021'/>"
			"</gd:comments>"
		"</entry>", -1, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	* Test the feed link *
	feed_link = gdata_youtube_video_get_comments_feed_link (video);
	g_assert (feed_link != NULL);
	g_assert (feed_link->rel == NULL);
	g_assert_cmpstr (feed_link->href, ==, "http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ/comments");
	g_assert_cmpuint (feed_link->count_hint, ==, 13021);
	g_assert (feed_link->read_only == FALSE);

	* TODO: more tests on entry properties *

	g_object_unref (video);
}*/

static void
test_parsing_georss_where (void)
{
	GDataYouTubeVideo *video;
	gdouble latitude, longitude;
	GError *error = NULL;

	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
		       "xmlns:media='http://search.yahoo.com/mrss/' "
		       "xmlns:yt='http://gdata.youtube.com/schemas/2007' "
		       "xmlns:gd='http://schemas.google.com/g/2005' "
		       "xmlns:georss='http://www.georss.org/georss' "
		       "xmlns:gml='http://www.opengis.net/gml'>"
			"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
			"<published>2006-05-16T14:06:37.000Z</published>"
			"<updated>2009-03-23T12:46:58.000Z</updated>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'/>"
			"<title>Some video somewhere</title>"
			"<link rel='http://www.iana.org/assignments/relation/alternate' type='text/html' href='http://www.youtube.com/watch?v=JAagedeKdcQ'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ?client=ytapi-google-jsdemo'/>"
			"<author>"
				"<name>Foo</name>"
				"<uri>http://gdata.youtube.com/feeds/api/users/Foo</uri>"
			"</author>"
			"<media:group>"
				"<media:title type='plain'>Some video somewhere</media:title>"
				"<media:credit role='uploader' scheme='urn:youtube'>Foo</media:credit>"
				"<media:category label='Music' scheme='http://gdata.youtube.com/schemas/2007/categories.cat'>Music</media:category>"
			"</media:group>"
			"<georss:where>"
				"<gml:Point>"
					"<gml:pos>41.14556884765625 -8.63525390625</gml:pos>"
				"</gml:Point>"
			"</georss:where>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	/* Test the coordinates */
	gdata_youtube_video_get_coordinates (video, &latitude, &longitude);
	g_assert_cmpfloat (latitude, ==, 41.14556884765625);
	g_assert_cmpfloat (longitude, ==, -8.63525390625);

	/* Update them and see if they're set OK and the XML's written out OK */
	gdata_youtube_video_set_coordinates (video, 5.5, 6.5);

	g_object_get (G_OBJECT (video),
	              "latitude", &latitude,
	              "longitude", &longitude,
	              NULL);

	g_assert_cmpfloat (latitude, ==, 5.5);
	g_assert_cmpfloat (longitude, ==, 6.5);

	/* Check the XML */
	gdata_test_assert_xml (video,
		"<?xml version='1.0' encoding='UTF-8'?>"
		"<entry xmlns='http://www.w3.org/2005/Atom' "
		       "xmlns:app='http://www.w3.org/2007/app' "
		       "xmlns:media='http://search.yahoo.com/mrss/' "
		       "xmlns:yt='http://gdata.youtube.com/schemas/2007' "
		       "xmlns:gd='http://schemas.google.com/g/2005' "
		       "xmlns:georss='http://www.georss.org/georss' "
		       "xmlns:gml='http://www.opengis.net/gml'>"
			"<title type='text'>Some video somewhere</title>"
			"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
			"<updated>2009-03-23T12:46:58Z</updated>"
			"<published>2006-05-16T14:06:37Z</published>"
			"<category term='http://gdata.youtube.com/schemas/2007#video' scheme='http://schemas.google.com/g/2005#kind'/>"
			"<link href='http://www.youtube.com/watch?v=JAagedeKdcQ' rel='http://www.iana.org/assignments/relation/alternate' type='text/html'/>"
			"<link href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ?client=ytapi-google-jsdemo' rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml'/>"
			"<author>"
				"<name>Foo</name>"
				"<uri>http://gdata.youtube.com/feeds/api/users/Foo</uri>"
			"</author>"
			"<media:group>"
				"<media:category scheme='http://gdata.youtube.com/schemas/2007/categories.cat' label='Music'>Music</media:category>"
				"<media:title type='plain'>Some video somewhere</media:title>"
			"</media:group>"
			"<app:control><app:draft>no</app:draft></app:control>"
			"<georss:where>"
				"<gml:Point>"
					"<gml:pos>5.5 6.5</gml:pos>"
				"</gml:Point>"
			"</georss:where>"
		"</entry>");

	/* Unset the properties and ensure they're removed from the XML */
	gdata_youtube_video_set_coordinates (video, G_MAXDOUBLE, G_MAXDOUBLE);

	gdata_youtube_video_get_coordinates (video, &latitude, &longitude);
	g_assert_cmpfloat (latitude, ==, G_MAXDOUBLE);
	g_assert_cmpfloat (longitude, ==, G_MAXDOUBLE);

	/* Check the XML */
	gdata_test_assert_xml (video,
		"<?xml version='1.0' encoding='UTF-8'?>"
		"<entry xmlns='http://www.w3.org/2005/Atom' "
		       "xmlns:app='http://www.w3.org/2007/app' "
		       "xmlns:media='http://search.yahoo.com/mrss/' "
		       "xmlns:yt='http://gdata.youtube.com/schemas/2007' "
		       "xmlns:gd='http://schemas.google.com/g/2005' "
		       "xmlns:georss='http://www.georss.org/georss' "
		       "xmlns:gml='http://www.opengis.net/gml'>"
			"<title type='text'>Some video somewhere</title>"
			"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
			"<updated>2009-03-23T12:46:58Z</updated>"
			"<published>2006-05-16T14:06:37Z</published>"
			"<category term='http://gdata.youtube.com/schemas/2007#video' scheme='http://schemas.google.com/g/2005#kind'/>"
			"<link href='http://www.youtube.com/watch?v=JAagedeKdcQ' rel='http://www.iana.org/assignments/relation/alternate' type='text/html'/>"
			"<link href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ?client=ytapi-google-jsdemo' rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml'/>"
			"<author>"
				"<name>Foo</name>"
				"<uri>http://gdata.youtube.com/feeds/api/users/Foo</uri>"
			"</author>"
			"<media:group>"
				"<media:category scheme='http://gdata.youtube.com/schemas/2007/categories.cat' label='Music'>Music</media:category>"
				"<media:title type='plain'>Some video somewhere</media:title>"
			"</media:group>"
			"<app:control><app:draft>no</app:draft></app:control>"
		"</entry>");

	g_object_unref (video);
}

static void
test_parsing_media_group (void)
{
	GDataYouTubeVideo *video;
	GError *error = NULL;

	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
		       "xmlns:media='http://search.yahoo.com/mrss/' "
		       "xmlns:yt='http://gdata.youtube.com/schemas/2007' "
		       "xmlns:gd='http://schemas.google.com/g/2005'>"
			"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
			"<published>2006-05-16T14:06:37.000Z</published>"
			"<updated>2009-03-23T12:46:58.000Z</updated>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'/>"
			"<title>Some video somewhere</title>"
			"<link rel='http://www.iana.org/assignments/relation/alternate' type='text/html' href='http://www.youtube.com/watch?v=JAagedeKdcQ'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ?client=ytapi-google-jsdemo'/>"
			"<author>"
				"<name>Foo</name>"
				"<uri>http://gdata.youtube.com/feeds/api/users/Foo</uri>"
			"</author>"
			"<media:group>"
				"<media:category label='Shows' scheme='http://gdata.youtube.com/schemas/2007/categories.cat'>Shows</media:category>"
				"<media:category scheme='http://gdata.youtube.com/schemas/2007/releasemediums.cat'>6</media:category>"
				"<media:category scheme='http://gdata.youtube.com/schemas/2007/mediatypes.cat'>3</media:category>"
				"<media:content url='http://www.youtube.com/v/aklRlKH4R94?f=related&amp;d=ARK7_SyB_5iKQvGvwsk-0D4O88HsQjpE1a8d1GxQnGDm&amp;app=youtube_gdata' type='application/x-shockwave-flash' medium='video' isDefault='true' expression='full' duration='163' yt:format='5'/>"
				"<media:content url='rtsp://v3.cache6.c.youtube.com/CkYLENy73wIaPQneR_ihlFFJahMYDSANFEgGUgdyZWxhdGVkciEBErv9LIH_mIpC8a_CyT7QPg7zwexCOkTVrx3UbFCcYOYM/0/0/0/video.3gp' type='video/3gpp' medium='video' expression='full' duration='163' yt:format='1'/>"
				"<media:content url='rtsp://v3.cache3.c.youtube.com/CkYLENy73wIaPQneR_ihlFFJahMYESARFEgGUgdyZWxhdGVkciEBErv9LIH_mIpC8a_CyT7QPg7zwexCOkTVrx3UbFCcYOYM/0/0/0/video.3gp' type='video/3gpp' medium='video' expression='full' duration='163' yt:format='6'/>"
				"<media:credit role='uploader' scheme='urn:youtube' yt:type='partner'>machinima</media:credit>"
				"<media:credit role='Producer' scheme='urn:ebu'>Machinima</media:credit>"
				"<media:credit role='info' scheme='urn:ebu'>season 1 episode 4 air date 08/22/10</media:credit>"
				"<media:credit role='Producer' scheme='urn:ebu'>Machinima</media:credit>"
				"<media:credit role='info' scheme='urn:ebu'>season 1 episode 4 air date 08/22/10</media:credit>"
				"<media:description type='plain'>www.youtube.com Click here to watch If It Were Realistic: Melee If It Were Realistic: Gravity Gun (Half Life 2 Machinima) What if gravity guns were realistic? Created by Renaldoxx from Massive X Productions Directors Channel: www.youtube.com www.youtube.com - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - Follow Machinima on Twitter! Machinima twitter.com Inside Gaming twitter.com Machinima Respawn twitter.com Machinima Entertainment, Technology, Culture twitter.com FOR MORE MACHINIMA, GO TO: www.youtube.com FOR MORE GAMEPLAY, GO TO: www.youtube.com FOR MORE SPORTS GAMEPLAY, GO TO: www.youtube.com FOR MORE TRAILERS, GO TO: www.youtube.com</media:description>"
				"<media:keywords>Half, Life, If, It, Were, Realistic, Gravity, Gun, Renaldoxx, Sniper, Game, Machinima, Action, Gordon, Freeman, drift0r, Euphorian, Films, Combine, Rebel, Dark, Citizen, Diary, massivex, Productions, Massive, yt:quality=high, Half-Life, [2], HL2, fortress, gmod, left dead, tf2</media:keywords>"
				"<media:player url='http://www.youtube.com/watch?v=aklRlKH4R94&amp;feature=youtube_gdata_player'/>"
				"<media:rating scheme='urn:mpaa'>pg</media:rating>"
				"<media:thumbnail url='http://i.ytimg.com/vi/aklRlKH4R94/default.jpg' height='90' width='120' time='00:01:21.500' yt:name='default'/>"
				"<media:thumbnail url='http://i.ytimg.com/vi/aklRlKH4R94/hqdefault.jpg' height='360' width='480' yt:name='hqdefault'/>"
				"<media:thumbnail url='http://i.ytimg.com/vi/aklRlKH4R94/1.jpg' height='90' width='120' time='00:00:40.750' yt:name='start'/>"
				"<media:thumbnail url='http://i.ytimg.com/vi/aklRlKH4R94/2.jpg' height='90' width='120' time='00:01:21.500' yt:name='middle'/>"
				"<media:thumbnail url='http://i.ytimg.com/vi/aklRlKH4R94/3.jpg' height='90' width='120' time='00:02:02.250' yt:name='end'/>"
				"<media:title type='plain'>If It Were Realistic - Gravity Gun (Half Life 2 Machinima)</media:title>"
				"<yt:aspectRatio>widescreen</yt:aspectRatio>"
				"<yt:duration seconds='163'/>"
				"<yt:uploaded>2010-08-22T14:04:18.000Z</yt:uploaded>"
				"<yt:videoid>aklRlKH4R94</yt:videoid>"
			"</media:group>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	/* TODO: For the moment, we just check that parsing the XML didn't fail. Later, we might actually support outputting the XML again. */

	g_object_unref (video);
}

static void
test_parsing_media_group_ratings (void)
{
	GDataYouTubeVideo *video;
	GError *error = NULL;

	/* Parse all ratings */
	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
		       "xmlns:media='http://search.yahoo.com/mrss/' "
		       "xmlns:yt='http://gdata.youtube.com/schemas/2007' "
		       "xmlns:gd='http://schemas.google.com/g/2005'>"
			"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
			"<published>2006-05-16T14:06:37.000Z</published>"
			"<updated>2009-03-23T12:46:58.000Z</updated>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'/>"
			"<title>Some video somewhere</title>"
			"<media:group>"
				"<media:rating scheme='urn:simple'>nonadult</media:rating>"
				"<media:rating scheme='urn:mpaa'>pg</media:rating>"
				"<media:rating scheme='urn:v-chip'>tv-pg</media:rating>"
			"</media:group>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	/* Check the ratings, and check that we haven't ended up with a country restriction */
	g_assert_cmpstr (gdata_youtube_video_get_media_rating (video, GDATA_YOUTUBE_RATING_TYPE_SIMPLE), ==, "nonadult");
	g_assert_cmpstr (gdata_youtube_video_get_media_rating (video, GDATA_YOUTUBE_RATING_TYPE_MPAA), ==, "pg");
	g_assert_cmpstr (gdata_youtube_video_get_media_rating (video, GDATA_YOUTUBE_RATING_TYPE_V_CHIP), ==, "tv-pg");

	g_assert (gdata_youtube_video_is_restricted_in_country (video, "US") == FALSE);

	g_object_unref (video);

	/* Parse a video with one rating missing and see what happens */
	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
		       "xmlns:media='http://search.yahoo.com/mrss/' "
		       "xmlns:yt='http://gdata.youtube.com/schemas/2007' "
		       "xmlns:gd='http://schemas.google.com/g/2005'>"
			"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
			"<published>2006-05-16T14:06:37.000Z</published>"
			"<updated>2009-03-23T12:46:58.000Z</updated>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'/>"
			"<title>Some video somewhere</title>"
			"<media:group>"
				"<media:rating scheme='urn:v-chip'>tv-y7-fv</media:rating>"
				"<media:rating>adult</media:rating>"
			"</media:group>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	/* Check the ratings again */
	g_assert_cmpstr (gdata_youtube_video_get_media_rating (video, GDATA_YOUTUBE_RATING_TYPE_SIMPLE), ==, "adult");
	g_assert_cmpstr (gdata_youtube_video_get_media_rating (video, GDATA_YOUTUBE_RATING_TYPE_MPAA), ==, NULL);
	g_assert_cmpstr (gdata_youtube_video_get_media_rating (video, GDATA_YOUTUBE_RATING_TYPE_V_CHIP), ==, "tv-y7-fv");

	/* Check that calling with an arbitrary rating type returns NULL */
	g_assert_cmpstr (gdata_youtube_video_get_media_rating (video, "fooish bar"), ==, NULL);

	g_object_unref (video);
}

static void
test_parsing_media_group_ratings_error_handling (void)
{
	GDataYouTubeVideo *video;
	GError *error = NULL;

#define TEST_XML_ERROR_HANDLING(x) \
	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO,\
		"<entry xmlns='http://www.w3.org/2005/Atom' "\
		       "xmlns:media='http://search.yahoo.com/mrss/' "\
		       "xmlns:yt='http://gdata.youtube.com/schemas/2007' "\
		       "xmlns:gd='http://schemas.google.com/g/2005'>"\
			"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"\
			"<published>2006-05-16T14:06:37.000Z</published>"\
			"<updated>2009-03-23T12:46:58.000Z</updated>"\
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'/>"\
			"<title>Some video somewhere</title>"\
			"<media:group>"\
				x\
			"</media:group>"\
		"</entry>", -1, &error));\
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR);\
	g_assert (video == NULL);\
	g_clear_error (&error)

	/* Missing content */
	TEST_XML_ERROR_HANDLING ("<media:rating scheme='urn:simple'/>");
	TEST_XML_ERROR_HANDLING ("<media:rating scheme='urn:mpaa'/>");
	TEST_XML_ERROR_HANDLING ("<media:rating scheme='urn:v-chip'/>");

	/* Empty content */
	TEST_XML_ERROR_HANDLING ("<media:rating scheme='urn:simple'></media:rating>");
	TEST_XML_ERROR_HANDLING ("<media:rating scheme='urn:mpaa'></media:rating>");
	TEST_XML_ERROR_HANDLING ("<media:rating scheme='urn:v-chip'></media:rating>");

	/* Unknown/Empty scheme */
	TEST_XML_ERROR_HANDLING ("<media:rating scheme=''>foo</media:rating>");
	TEST_XML_ERROR_HANDLING ("<media:rating scheme='urn:baz'>bob</media:rating>");

#undef TEST_XML_ERROR_HANDLING
}

static void
test_video_escaping (void)
{
	GDataYouTubeVideo *video;
	const gchar * const keywords[] = { "<keyword1>", "keyword2 & stuff, things", NULL };

	video = gdata_youtube_video_new (NULL);
	gdata_youtube_video_set_location (video, "Here & there");
	gdata_youtube_video_set_access_control (video, "<action>", GDATA_YOUTUBE_PERMISSION_ALLOWED);
	gdata_youtube_video_set_keywords (video, keywords);
	gdata_youtube_video_set_description (video, "Description & stuff.");
	gdata_youtube_video_set_aspect_ratio (video, "4 & 3");

	/* Check the outputted XML is escaped properly */
	gdata_test_assert_xml (video,
	                 "<?xml version='1.0' encoding='UTF-8'?>"
	                 "<entry xmlns='http://www.w3.org/2005/Atom' xmlns:media='http://search.yahoo.com/mrss/' "
	                        "xmlns:gd='http://schemas.google.com/g/2005' "
	                        "xmlns:yt='http://gdata.youtube.com/schemas/2007' xmlns:app='http://www.w3.org/2007/app' "
	                        "xmlns:georss='http://www.georss.org/georss' xmlns:gml='http://www.opengis.net/gml'>"
				"<title type='text'></title>"
				"<category term='http://gdata.youtube.com/schemas/2007#video' scheme='http://schemas.google.com/g/2005#kind'/>"
				"<media:group>"
					"<media:description type='plain'>Description &amp; stuff.</media:description>"
					"<media:keywords>&lt;keyword1&gt;,keyword2 &amp; stuff%2C things</media:keywords>"
					"<yt:aspectratio>4 &amp; 3</yt:aspectratio>"
				"</media:group>"
				"<yt:location>Here &amp; there</yt:location>"
				"<yt:accessControl action='&lt;action&gt;' permission='allowed'/>"
				"<app:control><app:draft>no</app:draft></app:control>"
	                 "</entry>");
	g_object_unref (video);
}

static void
test_comment_get_xml (void)
{
	GDataYouTubeComment *comment_;

	comment_ = gdata_youtube_comment_new (NULL);
	gdata_entry_set_content (GDATA_ENTRY (comment_), "This is a comment with <markup> & stüff.");
	gdata_youtube_comment_set_parent_comment_uri (comment_, "http://example.com/?foo=bar&baz=shizzle");

	/* Check the outputted XML is OK */
	gdata_test_assert_xml (comment_,
		"<?xml version='1.0' encoding='UTF-8'?>"
		"<entry xmlns='http://www.w3.org/2005/Atom' xmlns:gd='http://schemas.google.com/g/2005'>"
			"<title type='text'></title>"
			"<content type='text'>This is a comment with &lt;markup&gt; &amp; stüff.</content>"
			"<category term='http://gdata.youtube.com/schemas/2007#comment' scheme='http://schemas.google.com/g/2005#kind'/>"
			"<link href='http://example.com/?foo=bar&amp;baz=shizzle' rel='http://gdata.youtube.com/schemas/2007#in-reply-to'/>"
		"</entry>");

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

static void
test_query_uri (void)
{
	gdouble latitude, longitude, radius;
	gboolean has_location;
	gchar *query_uri;
	GDataYouTubeQuery *query = gdata_youtube_query_new ("q");

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
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&time=all_time&safeSearch=none&format=1&location=45.013640000000002,-97.123559999999998!&location-radius=112.5m");
	g_free (query_uri);

	gdata_youtube_query_set_location (query, G_MAXDOUBLE, 0.6672, 52.8, TRUE);

	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&time=all_time&safeSearch=none&format=1&location=!");
	g_free (query_uri);

	gdata_youtube_query_set_location (query, G_MAXDOUBLE, G_MAXDOUBLE, 0.0, FALSE);

	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&time=all_time&safeSearch=none&format=1");
	g_free (query_uri);

	/* Language */
	gdata_youtube_query_set_language (query, "fr");
	g_assert_cmpstr (gdata_youtube_query_get_language (query), ==, "fr");

	gdata_youtube_query_set_order_by (query, "relevance_lang_fr");
	g_assert_cmpstr (gdata_youtube_query_get_order_by (query), ==, "relevance_lang_fr");

	gdata_youtube_query_set_restriction (query, "192.168.0.1");
	g_assert_cmpstr (gdata_youtube_query_get_restriction (query), ==, "192.168.0.1");

	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&time=all_time&safeSearch=none&format=1&lr=fr&orderby=relevance_lang_fr&restriction=192.168.0.1");
	g_free (query_uri);

	gdata_youtube_query_set_safe_search (query, GDATA_YOUTUBE_SAFE_SEARCH_STRICT);
	g_assert_cmpuint (gdata_youtube_query_get_safe_search (query), ==, GDATA_YOUTUBE_SAFE_SEARCH_STRICT);

	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&time=all_time&safeSearch=strict&format=1&lr=fr&orderby=relevance_lang_fr&restriction=192.168.0.1");
	g_free (query_uri);

	gdata_youtube_query_set_sort_order (query, GDATA_YOUTUBE_SORT_ASCENDING);
	g_assert_cmpuint (gdata_youtube_query_get_sort_order (query), ==, GDATA_YOUTUBE_SORT_ASCENDING);

	gdata_youtube_query_set_age (query, GDATA_YOUTUBE_AGE_THIS_WEEK);
	g_assert_cmpuint (gdata_youtube_query_get_age (query), ==, GDATA_YOUTUBE_AGE_THIS_WEEK);

	gdata_youtube_query_set_uploader (query, GDATA_YOUTUBE_UPLOADER_PARTNER);
	g_assert_cmpuint (gdata_youtube_query_get_uploader (query), ==, GDATA_YOUTUBE_UPLOADER_PARTNER);

	gdata_youtube_query_set_license (query, GDATA_YOUTUBE_LICENSE_CC);
	g_assert_cmpstr (gdata_youtube_query_get_license (query), ==, GDATA_YOUTUBE_LICENSE_CC);

	/* Check the built URI with a normal feed URI */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&time=this_week&safeSearch=strict&format=1&lr=fr&orderby=relevance_lang_fr&restriction=192.168.0.1&sortorder=ascending&uploader=partner&license=cc");
	g_free (query_uri);

	/* …and with a feed URI with pre-existing arguments */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com?foobar=shizzle");
	g_assert_cmpstr (query_uri, ==, "http://example.com?foobar=shizzle&q=q&time=this_week&safeSearch=strict&format=1&lr=fr&orderby=relevance_lang_fr&restriction=192.168.0.1&sortorder=ascending&uploader=partner&license=cc");
	g_free (query_uri);

	g_object_unref (query);
}

static void
test_query_etag (void)
{
	GDataYouTubeQuery *query = gdata_youtube_query_new (NULL);

	/* Test that setting any property will unset the ETag */
	g_test_bug ("613529");

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
	g_assert_cmpstr (gdata_youtube_video_get_video_id (video), ==, "_LeQuMpwbW4");
	g_assert_cmpstr (gdata_entry_get_id (GDATA_ENTRY (video)), ==, "tag:youtube.com,2008:video:_LeQuMpwbW4");
	g_clear_error (&error);

	g_object_unref (video);

	gdata_mock_server_end_trace (mock_server);
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
		g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
		g_assert_cmpstr (gdata_youtube_video_get_video_id (video), ==, "_LeQuMpwbW4");
		g_assert_cmpstr (gdata_entry_get_id (GDATA_ENTRY (video)), ==, "tag:youtube.com,2008:video:_LeQuMpwbW4");

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

	gdata_mock_server_end_trace (mock_server);
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
		g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (comment_)), !=, NULL);
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

	gdata_mock_server_end_trace (mock_server);
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

	gdata_mock_server_end_trace (mock_server);
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

	gdata_mock_server_end_trace (mock_server);
}

static void
tear_down_insert_comment (InsertCommentData *data, gconstpointer service)
{
	gdata_test_mock_server_start_trace (mock_server, "teardown-insert-comment");

	if (data->comment != NULL) {
		g_object_unref (data->comment);
	}

	tear_down_comment ((CommentData*) data, service);

	gdata_mock_server_end_trace (mock_server);
}

static void
assert_comments_equal (GDataComment *new_comment, GDataYouTubeComment *original_comment)
{
	GList *authors;
	GDataAuthor *author;

	g_assert (GDATA_IS_YOUTUBE_COMMENT (new_comment));
	g_assert (GDATA_IS_YOUTUBE_COMMENT (original_comment));
	g_assert (GDATA_YOUTUBE_COMMENT (new_comment) != original_comment);

	g_assert_cmpstr (gdata_entry_get_content (GDATA_ENTRY (new_comment)), ==, gdata_entry_get_content (GDATA_ENTRY (original_comment)));
	g_assert_cmpstr (gdata_youtube_comment_get_parent_comment_uri (GDATA_YOUTUBE_COMMENT (new_comment)), ==,
	                 gdata_youtube_comment_get_parent_comment_uri (original_comment));

	/* Check the author of the new comment. */
	authors = gdata_entry_get_authors (GDATA_ENTRY (new_comment));
	g_assert_cmpuint (g_list_length (authors), ==, 1);

	author = GDATA_AUTHOR (authors->data);

	g_assert_cmpstr (gdata_author_get_name (author), ==, "GDataTest");
	g_assert_cmpstr (gdata_author_get_uri (author), ==, "https://gdata.youtube.com/feeds/api/users/GDataTest");
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

	assert_comments_equal (new_comment, data->comment);

	g_object_unref (new_comment);

	gdata_mock_server_end_trace (mock_server);
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
		assert_comments_equal (new_comment, data->comment);

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

	gdata_mock_server_end_trace (mock_server);
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
	gchar *category_label, *old_locale;

	gdata_test_mock_server_start_trace (mock_server, "categories");

	app_categories = gdata_youtube_service_get_categories (GDATA_YOUTUBE_SERVICE (service), NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_APP_CATEGORIES (app_categories));
	g_clear_error (&error);

	categories = gdata_app_categories_get_categories (app_categories);
	g_assert_cmpint (g_list_length (categories), >, 0);
	g_assert (GDATA_IS_YOUTUBE_CATEGORY (categories->data));

	/* Save a label for comparison against a different locale */
	category_label = g_strdup (gdata_category_get_label (GDATA_CATEGORY (categories->data)));

	g_object_unref (app_categories);

	/* Test with a different locale */
	old_locale = g_strdup (gdata_service_get_locale (GDATA_SERVICE (service)));
	gdata_service_set_locale (GDATA_SERVICE (service), "it");

	app_categories = gdata_youtube_service_get_categories (GDATA_YOUTUBE_SERVICE (service), NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_APP_CATEGORIES (app_categories));
	g_clear_error (&error);

	categories = gdata_app_categories_get_categories (app_categories);
	g_assert_cmpint (g_list_length (categories), >, 0);
	g_assert (GDATA_IS_YOUTUBE_CATEGORY (categories->data));

	/* Compare the labels */
	g_assert_cmpstr (category_label, !=, gdata_category_get_label (GDATA_CATEGORY (categories->data)));

	g_object_unref (app_categories);
	g_free (category_label);

	/* Reset the locale */
	gdata_service_set_locale (GDATA_SERVICE (service), old_locale);
	g_free (old_locale);

	gdata_mock_server_end_trace (mock_server);
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

	gdata_mock_server_end_trace (mock_server);
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

	/* Run a singleton batch operation to query one of the entries */
	gdata_test_batch_operation_query (operation, gdata_entry_get_id (data->new_video), GDATA_TYPE_YOUTUBE_VIDEO, data->new_video, NULL, NULL);

	g_assert (gdata_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

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

	g_assert (gdata_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);

	gdata_mock_server_end_trace (mock_server);
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
test_batch_async (BatchData *data, gconstpointer service)
{
	GDataBatchOperation *operation;
	GMainLoop *main_loop;

	gdata_test_mock_server_start_trace (mock_server, "batch-async");

	/* Run an async query operation on the video */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_youtube_service_get_primary_authorization_domain (),
	                                              "https://gdata.youtube.com/feeds/api/videos/batch");
	gdata_test_batch_operation_query (operation, gdata_entry_get_id (data->new_video), GDATA_TYPE_YOUTUBE_VIDEO, data->new_video, NULL, NULL);

	main_loop = g_main_loop_new (NULL, TRUE);

	gdata_batch_operation_run_async (operation, NULL, (GAsyncReadyCallback) test_batch_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);

	gdata_mock_server_end_trace (mock_server);
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

	gdata_mock_server_end_trace (mock_server);
}

static void
teardown_batch (BatchData *data, gconstpointer service)
{
	g_object_unref (data->new_video);
	g_object_unref (data->new_video2);
}

int
main (int argc, char *argv[])
{
	gint retval;
	GDataAuthorizer *authorizer = NULL;
	GDataService *service = NULL;
	GFile *trace_directory;

	gdata_test_init (argc, argv);

	mock_server = gdata_test_get_mock_server ();
	trace_directory = g_file_new_for_path ("traces/youtube");
	gdata_mock_server_set_trace_directory (mock_server, trace_directory);
	g_object_unref (trace_directory);

	gdata_test_mock_server_start_trace (mock_server, "global-authentication");
	authorizer = GDATA_AUTHORIZER (gdata_client_login_authorizer_new (CLIENT_ID, GDATA_TYPE_YOUTUBE_SERVICE));
	gdata_client_login_authorizer_authenticate (GDATA_CLIENT_LOGIN_AUTHORIZER (authorizer), USERNAME, PASSWORD, NULL, NULL);
	gdata_mock_server_end_trace (mock_server);

	service = GDATA_SERVICE (gdata_youtube_service_new (DEVELOPER_KEY, authorizer));

	g_test_add_func ("/youtube/authentication", test_authentication);
	g_test_add_func ("/youtube/authentication/error", test_authentication_error);
	g_test_add_func ("/youtube/authentication/timeout", test_authentication_timeout);
	g_test_add ("/youtube/authentication/async", GDataAsyncTestData, NULL, gdata_set_up_async_test_data, test_authentication_async,
	            gdata_tear_down_async_test_data);
	g_test_add ("/youtube/authentication/async/cancellation", GDataAsyncTestData, NULL, gdata_set_up_async_test_data,
	            test_authentication_async_cancellation, gdata_tear_down_async_test_data);

	g_test_add_data_func ("/youtube/query/standard_feeds", service, test_query_standard_feeds);
	g_test_add_data_func ("/youtube/query/standard_feed", service, test_query_standard_feed);
	g_test_add_data_func ("/youtube/query/standard_feed/with_query", service, test_query_standard_feed_with_query);
	g_test_add_data_func ("/youtube/query/standard_feed/error", service, test_query_standard_feed_error);
	g_test_add_data_func ("/youtube/query/standard_feed/timeout", service, test_query_standard_feed_timeout);
	g_test_add ("/youtube/query/standard_feed/async", GDataAsyncTestData, service, gdata_set_up_async_test_data,
	            test_query_standard_feed_async, gdata_tear_down_async_test_data);
	g_test_add_data_func ("/youtube/query/standard_feed/async/progress_closure", service, test_query_standard_feed_async_progress_closure);
	g_test_add ("/youtube/query/standard_feed/async/cancellation", GDataAsyncTestData, service, gdata_set_up_async_test_data,
	            test_query_standard_feed_async_cancellation, gdata_tear_down_async_test_data);
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
	/*g_test_add_func ("/youtube/parsing/comments/feedLink", test_parsing_comments_feed_link);*/
	g_test_add_func ("/youtube/parsing/yt:recorded", test_parsing_yt_recorded);
	g_test_add_func ("/youtube/parsing/yt:accessControl", test_parsing_yt_access_control);
	g_test_add_func ("/youtube/parsing/yt:category", test_parsing_yt_category);
	g_test_add_func ("/youtube/parsing/video_id_from_uri", test_parsing_video_id_from_uri);
	g_test_add_func ("/youtube/parsing/georss:where", test_parsing_georss_where);
	g_test_add_func ("/youtube/parsing/media:group", test_parsing_media_group);
	g_test_add_func ("/youtube/parsing/media:group/ratings", test_parsing_media_group_ratings);
	g_test_add_func ("/youtube/parsing/media:group/ratings/error_handling", test_parsing_media_group_ratings_error_handling);

	g_test_add_func ("/youtube/video/escaping", test_video_escaping);

	g_test_add_func ("/youtube/comment/get_xml", test_comment_get_xml);
	g_test_add_func ("/youtube/comment/properties/parent-comment-id", test_comment_properties_parent_comment_uri);

	g_test_add_func ("/youtube/query/uri", test_query_uri);
	g_test_add_func ("/youtube/query/etag", test_query_etag);

	retval = g_test_run ();

	if (service != NULL)
		g_object_unref (service);

	return retval;
}
