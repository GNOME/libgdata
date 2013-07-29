/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2011 <philip@tecnocode.co.uk>
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
#include <gdata/gdata.h>

#include "common.h"

static GThread *main_thread = NULL;
static GDataMockServer *mock_server = NULL;

static void
test_client_login_authorizer_constructor (void)
{
	GDataClientLoginAuthorizer *authorizer;

	authorizer = gdata_client_login_authorizer_new ("client-id", GDATA_TYPE_YOUTUBE_SERVICE);

	g_assert (authorizer != NULL);
	g_assert (GDATA_IS_CLIENT_LOGIN_AUTHORIZER (authorizer));
	g_assert (GDATA_IS_AUTHORIZER (authorizer));

	g_object_unref (authorizer);
}

static void
test_client_login_authorizer_constructor_for_domains (void)
{
	GDataClientLoginAuthorizer *authorizer;
	GDataAuthorizationDomain *domain;
	GList *domains;

	/* Try with standard domains first */
	domains = gdata_service_get_authorization_domains (GDATA_TYPE_YOUTUBE_SERVICE);
	authorizer = gdata_client_login_authorizer_new_for_authorization_domains ("client-id", domains);
	g_list_free (domains);

	g_assert (authorizer != NULL);
	g_assert (GDATA_IS_CLIENT_LOGIN_AUTHORIZER (authorizer));
	g_assert (GDATA_IS_AUTHORIZER (authorizer));

	g_object_unref (authorizer);

	/* Try again with a custom domain. Note that, as in test_authorization_domain_properties() this should not normally happen in client code. */
	domain = GDATA_AUTHORIZATION_DOMAIN (g_object_new (GDATA_TYPE_AUTHORIZATION_DOMAIN,
	                                                   "service-name", "test",
	                                                   "scope", "test",
	                                                   NULL));

	domains = g_list_prepend (NULL, domain);
	authorizer = gdata_client_login_authorizer_new_for_authorization_domains ("client-id", domains);
	g_list_free (domains);

	g_assert (authorizer != NULL);
	g_assert (GDATA_IS_CLIENT_LOGIN_AUTHORIZER (authorizer));
	g_assert (GDATA_IS_AUTHORIZER (authorizer));

	g_object_unref (authorizer);
	g_object_unref (domain);
}

typedef struct {
	GDataClientLoginAuthorizer *authorizer;

	guint proxy_uri_notification_count;
	gulong proxy_uri_signal_handler;
	guint timeout_notification_count;
	gulong timeout_signal_handler;
	guint username_notification_count;
	gulong username_signal_handler;
	guint password_notification_count;
	gulong password_signal_handler;
} ClientLoginAuthorizerData;

/* Used to count that exactly the right number of notify signals are emitted when setting properties */
static void
notify_cb (GObject *object, GParamSpec *pspec, guint *notification_count)
{
	/* Check we're running in the main thread */
	g_assert (g_thread_self () == main_thread);

	/* Increment the notification count */
	*notification_count = *notification_count + 1;
}

static void
connect_to_client_login_authorizer (ClientLoginAuthorizerData *data)
{
	/* Connect to notifications from the object to verify they're only emitted the correct number of times */
	data->proxy_uri_signal_handler = g_signal_connect (data->authorizer, "notify::proxy-uri", (GCallback) notify_cb,
	                                                   &(data->proxy_uri_notification_count));
	data->timeout_signal_handler = g_signal_connect (data->authorizer, "notify::timeout", (GCallback) notify_cb,
	                                                 &(data->timeout_notification_count));
	data->username_signal_handler = g_signal_connect (data->authorizer, "notify::username", (GCallback) notify_cb,
	                                                  &(data->username_notification_count));
	data->password_signal_handler = g_signal_connect (data->authorizer, "notify::password", (GCallback) notify_cb,
	                                                  &(data->password_notification_count));
}

static void
set_up_client_login_authorizer_data (ClientLoginAuthorizerData *data, gconstpointer user_data)
{
	data->authorizer = gdata_client_login_authorizer_new ("client-id", GDATA_TYPE_YOUTUBE_SERVICE);
	connect_to_client_login_authorizer (data);
}

static void
set_up_client_login_authorizer_data_multiple_domains (ClientLoginAuthorizerData *data, gconstpointer user_data)
{
	GList *authorization_domains = NULL;

	authorization_domains = g_list_prepend (authorization_domains, gdata_youtube_service_get_primary_authorization_domain ());
	authorization_domains = g_list_prepend (authorization_domains, gdata_picasaweb_service_get_primary_authorization_domain ());
	data->authorizer = gdata_client_login_authorizer_new_for_authorization_domains ("client-id", authorization_domains);
	g_list_free (authorization_domains);

	connect_to_client_login_authorizer (data);
}

static void
set_up_client_login_authorizer_data_authenticated (ClientLoginAuthorizerData *data, gconstpointer user_data)
{
	gdata_test_mock_server_start_trace (mock_server, "setup-client-login-authorizer-data-authenticated");

	data->authorizer = gdata_client_login_authorizer_new ("client-id", GDATA_TYPE_YOUTUBE_SERVICE);
	g_assert (gdata_client_login_authorizer_authenticate (data->authorizer, USERNAME, PASSWORD, NULL, NULL) == TRUE);
	connect_to_client_login_authorizer (data);

	gdata_mock_server_end_trace (mock_server);
}

static void
tear_down_client_login_authorizer_data (ClientLoginAuthorizerData *data, gconstpointer user_data)
{
	/* Clean up signal handlers */
	g_signal_handler_disconnect (data->authorizer, data->password_signal_handler);
	g_signal_handler_disconnect (data->authorizer, data->username_signal_handler);
	g_signal_handler_disconnect (data->authorizer, data->timeout_signal_handler);
	g_signal_handler_disconnect (data->authorizer, data->proxy_uri_signal_handler);

	g_object_unref (data->authorizer);
}

/* Test getting and setting the client-id property */
static void
test_client_login_authorizer_properties_client_id (ClientLoginAuthorizerData *data, gconstpointer user_data)
{
	gchar *client_id;

	/* Verifying the normal state of the property in a newly-constructed instance of GDataClientLoginAuthorizer */
	g_assert_cmpstr (gdata_client_login_authorizer_get_client_id (data->authorizer), ==, "client-id");

	g_object_get (data->authorizer, "client-id", &client_id, NULL);
	g_assert_cmpstr (client_id, ==, "client-id");
	g_free (client_id);
}

/* Test getting and setting the username property */
static void
test_client_login_authorizer_properties_username (ClientLoginAuthorizerData *data, gconstpointer user_data)
{
	gchar *username;

	/* Verifying the normal state of the property in a newly-constructed instance of GDataClientLoginAuthorizer */
	g_assert (gdata_client_login_authorizer_get_username (data->authorizer) == NULL);

	g_object_get (data->authorizer, "username", &username, NULL);
	g_assert (username == NULL);
	g_free (username);
}

/* Test getting and setting the password property */
static void
test_client_login_authorizer_properties_password (ClientLoginAuthorizerData *data, gconstpointer user_data)
{
	gchar *password;

	/* Verifying the normal state of the property in a newly-constructed instance of GDataClientLoginAuthorizer */
	g_assert (gdata_client_login_authorizer_get_password (data->authorizer) == NULL);

	g_object_get (data->authorizer, "password", &password, NULL);
	g_assert (password == NULL);
	g_free (password);
}

/* Test getting and setting the proxy-uri property */
static void
test_client_login_authorizer_properties_proxy_uri (ClientLoginAuthorizerData *data, gconstpointer user_data)
{
	SoupURI *proxy_uri, *new_proxy_uri;

	/* Verifying the normal state of the property in a newly-constructed instance of GDataClientLoginAuthorizer */
	g_assert (gdata_client_login_authorizer_get_proxy_uri (data->authorizer) == NULL);

	g_object_get (data->authorizer, "proxy-uri", &proxy_uri, NULL);
	g_assert (proxy_uri == NULL);

	g_assert_cmpuint (data->proxy_uri_notification_count, ==, 0);

	/* Check setting it works and emits a notification */
	new_proxy_uri = soup_uri_new ("http://example.com/");
	gdata_client_login_authorizer_set_proxy_uri (data->authorizer, new_proxy_uri);

	g_assert_cmpuint (data->proxy_uri_notification_count, ==, 1);

	g_assert (gdata_client_login_authorizer_get_proxy_uri (data->authorizer) != NULL);
	g_assert (soup_uri_equal (gdata_client_login_authorizer_get_proxy_uri (data->authorizer), new_proxy_uri) == TRUE);

	g_object_get (data->authorizer, "proxy-uri", &proxy_uri, NULL);
	g_assert (proxy_uri != NULL);
	g_assert (soup_uri_equal (gdata_client_login_authorizer_get_proxy_uri (data->authorizer), new_proxy_uri) == TRUE);
	soup_uri_free (proxy_uri);

	soup_uri_free (new_proxy_uri);

	/* Check setting it back to NULL works */
	gdata_client_login_authorizer_set_proxy_uri (data->authorizer, NULL);

	g_assert_cmpuint (data->proxy_uri_notification_count, ==, 2);

	g_assert (gdata_client_login_authorizer_get_proxy_uri (data->authorizer) == NULL);

	g_object_get (data->authorizer, "proxy-uri", &proxy_uri, NULL);
	g_assert (proxy_uri == NULL);

	/* Test that setting it using g_object_set() works */
	new_proxy_uri = soup_uri_new ("http://example.com/");
	g_object_set (data->authorizer, "proxy-uri", new_proxy_uri, NULL);
	soup_uri_free (new_proxy_uri);

	g_assert (gdata_client_login_authorizer_get_proxy_uri (data->authorizer) != NULL);
}

/* Test getting and setting the timeout property */
static void
test_client_login_authorizer_properties_timeout (ClientLoginAuthorizerData *data, gconstpointer user_data)
{
	guint timeout;

	/* Verifying the normal state of the property in a newly-constructed instance of GDataClientLoginAuthorizer */
	g_assert_cmpuint (gdata_client_login_authorizer_get_timeout (data->authorizer), ==, 0);

	g_object_get (data->authorizer, "timeout", &timeout, NULL);
	g_assert_cmpuint (timeout, ==, 0);

	g_assert_cmpuint (data->timeout_notification_count, ==, 0);

	/* Check setting it works and emits a notification */
	gdata_client_login_authorizer_set_timeout (data->authorizer, 30);

	g_assert_cmpuint (data->timeout_notification_count, ==, 1);

	g_assert_cmpuint (gdata_client_login_authorizer_get_timeout (data->authorizer), ==, 30);

	g_object_get (data->authorizer, "timeout", &timeout, NULL);
	g_assert_cmpuint (timeout, ==, 30);

	/* Check setting it back to 0 works */
	gdata_client_login_authorizer_set_timeout (data->authorizer, 0);

	g_assert_cmpuint (data->timeout_notification_count, ==, 2);

	g_assert_cmpuint (gdata_client_login_authorizer_get_timeout (data->authorizer), ==, 0);

	g_object_get (data->authorizer, "timeout", &timeout, NULL);
	g_assert_cmpuint (timeout, ==, 0);

	/* Test that setting it using g_object_set() works */
	g_object_set (data->authorizer, "timeout", 15, NULL);
	g_assert_cmpuint (gdata_client_login_authorizer_get_timeout (data->authorizer), ==, 15);
}

/* Standard tests for pre-authentication in sync and async tests with single or multiple domains */
static void
pre_test_authentication (ClientLoginAuthorizerData *data)
{
	/* Check we're not already authorised any domains */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->authorizer),
	          gdata_youtube_service_get_primary_authorization_domain ()) == FALSE);
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->authorizer),
	          gdata_picasaweb_service_get_primary_authorization_domain ()) == FALSE);

	g_assert_cmpuint (data->username_notification_count, ==, 0);
	g_assert_cmpuint (data->password_notification_count, ==, 0);
}

/* Standard tests for post-authentication (successful or not controlled by @authorized) in sync tests with single domains */
static void
post_test_authentication (ClientLoginAuthorizerData *data, gboolean authorized)
{
	/* Are we authorised now? */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->authorizer),
	          gdata_youtube_service_get_primary_authorization_domain ()) == authorized);

	g_assert_cmpuint (data->username_notification_count, ==, 1);
	g_assert_cmpuint (data->password_notification_count, ==, 1);

	if (authorized == TRUE) {
		/* Check the username and password were set correctly. Note that we always assert that the domain name is present in the username. */
		g_assert_cmpstr (gdata_client_login_authorizer_get_username (data->authorizer), ==, USERNAME);
		g_assert_cmpstr (gdata_client_login_authorizer_get_password (data->authorizer), ==, PASSWORD);
	} else {
		/* Check the username and password are *not* set. */
		g_assert (gdata_client_login_authorizer_get_username (data->authorizer) == NULL);
		g_assert (gdata_client_login_authorizer_get_password (data->authorizer) == NULL);
	}
}

/* Test that synchronous authentication against a single authorization domains succeeds */
static void
test_client_login_authorizer_authenticate_sync (ClientLoginAuthorizerData *data, gconstpointer _username)
{
	const gchar *username = (const gchar*) _username;
	gboolean success;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "client-login-authorizer-authenticate-sync");

	pre_test_authentication (data);

	/* Authenticate! */
	success = gdata_client_login_authorizer_authenticate (data->authorizer, username, PASSWORD, NULL, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);
	g_clear_error (&error);

	post_test_authentication (data, TRUE);

	gdata_mock_server_end_trace (mock_server);
}

/* Test that authentication using an incorrect password fails */
static void
test_client_login_authorizer_authenticate_sync_bad_password (ClientLoginAuthorizerData *data, gconstpointer _username)
{
	const gchar *username = (const gchar*) _username;
	gboolean success;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "client-login-authorizer-authenticate-sync-bad-password");

	pre_test_authentication (data);

	/* Authenticate! */
	success = gdata_client_login_authorizer_authenticate (data->authorizer, username, INCORRECT_PASSWORD, NULL, &error);
	g_assert_error (error, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_BAD_AUTHENTICATION);
	g_assert (success == FALSE);
	g_clear_error (&error);

	post_test_authentication (data, FALSE);

	gdata_mock_server_end_trace (mock_server);
}

/* Test that authentication against multiple authorization domains simultaneously and synchronously works */
static void
test_client_login_authorizer_authenticate_sync_multiple_domains (ClientLoginAuthorizerData *data, gconstpointer user_data)
{
	gboolean success;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "client-login-authorizer-authenticate-sync-multiple-domains");

	pre_test_authentication (data);

	/* Authenticate! */
	success = gdata_client_login_authorizer_authenticate (data->authorizer, USERNAME, PASSWORD, NULL, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);
	g_clear_error (&error);

	/* Are we authorised in the second domain now? */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->authorizer),
	          gdata_picasaweb_service_get_primary_authorization_domain ()) == TRUE);

	post_test_authentication (data, TRUE);

	gdata_mock_server_end_trace (mock_server);
}

/* Test that synchronous authentication can be cancelled */
static void
test_client_login_authorizer_authenticate_sync_cancellation (ClientLoginAuthorizerData *data, gconstpointer user_data)
{
	gboolean success;
	GCancellable *cancellable;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "client-login-authorizer-authenticate-sync-cancellation");

	pre_test_authentication (data);

	/* Set up the cancellable */
	cancellable = g_cancellable_new ();

	/* Authenticate! This should return immediately as the cancellable was cancelled beforehand. */
	g_cancellable_cancel (cancellable);
	success = gdata_client_login_authorizer_authenticate (data->authorizer, USERNAME, PASSWORD, cancellable, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_assert (success == FALSE);
	g_clear_error (&error);

	post_test_authentication (data, FALSE);

	g_object_unref (cancellable);

	gdata_mock_server_end_trace (mock_server);
}

typedef struct {
	ClientLoginAuthorizerData parent;
	GMainLoop *main_loop;
} ClientLoginAuthorizerAsyncData;

static void
set_up_client_login_authorizer_async_data (ClientLoginAuthorizerAsyncData *data, gconstpointer user_data)
{
	/* Chain up */
	set_up_client_login_authorizer_data ((ClientLoginAuthorizerData*) data, user_data);

	/* Set up the main loop */
	data->main_loop = g_main_loop_new (NULL, FALSE);
}

static void
set_up_client_login_authorizer_async_data_multiple_domains (ClientLoginAuthorizerAsyncData *data, gconstpointer user_data)
{
	/* Chain up */
	set_up_client_login_authorizer_data_multiple_domains ((ClientLoginAuthorizerData*) data, user_data);

	/* Set up the main loop */
	data->main_loop = g_main_loop_new (NULL, FALSE);
}

static void
tear_down_client_login_authorizer_async_data (ClientLoginAuthorizerAsyncData *data, gconstpointer user_data)
{
	g_main_loop_unref (data->main_loop);

	/* Chain up */
	tear_down_client_login_authorizer_data ((ClientLoginAuthorizerData*) data, user_data);
}

/* Standard tests for post-authentication (successful or not controlled by @authorized) in async tests with single domains */
static void
post_test_authentication_async (ClientLoginAuthorizerAsyncData *data, gboolean authorized)
{
	/* Spin on the notification counts being incremented */
	while (data->parent.username_notification_count == 0 || data->parent.password_notification_count == 0) {
		g_main_context_iteration (g_main_loop_get_context (data->main_loop), FALSE);
	}

	post_test_authentication ((ClientLoginAuthorizerData*) data, authorized);
}

static void
test_client_login_authorizer_authenticate_async_cb (GDataClientLoginAuthorizer *authorizer, GAsyncResult *async_result,
                                                    ClientLoginAuthorizerAsyncData *data)
{
	gboolean success;
	GError *error = NULL;

	success = gdata_client_login_authorizer_authenticate_finish (authorizer, async_result, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);
	g_clear_error (&error);

	post_test_authentication_async (data, TRUE);

	g_main_loop_quit (data->main_loop);
}

/* Test that asynchronous authentication against a single authorization domain works */
static void
test_client_login_authorizer_authenticate_async (ClientLoginAuthorizerAsyncData *data, gconstpointer user_data)
{
	gdata_test_mock_server_start_trace (mock_server, "client-login-authorizer-authenticate-async");

	pre_test_authentication ((ClientLoginAuthorizerData*) data);

	/* Create a main loop and authenticate */
	gdata_client_login_authorizer_authenticate_async (data->parent.authorizer, USERNAME, PASSWORD, NULL,
	                                                  (GAsyncReadyCallback) test_client_login_authorizer_authenticate_async_cb, data);

	g_main_loop_run (data->main_loop);

	gdata_mock_server_end_trace (mock_server);
}

static void
test_client_login_authorizer_authenticate_async_multiple_domains_cb (GDataClientLoginAuthorizer *authorizer, GAsyncResult *async_result,
                                                                     ClientLoginAuthorizerAsyncData *data)
{
	gboolean success;
	GError *error = NULL;

	success = gdata_client_login_authorizer_authenticate_finish (authorizer, async_result, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);
	g_clear_error (&error);

	/* Assert that we're now authorised in the second domain */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	          gdata_picasaweb_service_get_primary_authorization_domain ()) == TRUE);

	post_test_authentication_async (data, TRUE);

	g_main_loop_quit (data->main_loop);
}

/* Test that authentication against multiple authorization domains simultaneously and asynchronously works */
static void
test_client_login_authorizer_authenticate_async_multiple_domains (ClientLoginAuthorizerAsyncData *data, gconstpointer user_data)
{
	gdata_test_mock_server_start_trace (mock_server, "client-login-authorizer-authenticate-async-multiple-domains");

	pre_test_authentication ((ClientLoginAuthorizerData*) data);

	/* Create a main loop and authenticate */
	gdata_client_login_authorizer_authenticate_async (data->parent.authorizer, USERNAME, PASSWORD, NULL,
	                                                  (GAsyncReadyCallback) test_client_login_authorizer_authenticate_async_multiple_domains_cb,
	                                                  data);

	g_main_loop_run (data->main_loop);

	gdata_mock_server_end_trace (mock_server);
}

static void
test_client_login_authorizer_authenticate_async_cancellation_cb (GDataClientLoginAuthorizer *authorizer, GAsyncResult *async_result,
                                                                 ClientLoginAuthorizerAsyncData *data)
{
	gboolean success;
	GError *error = NULL;

	success = gdata_client_login_authorizer_authenticate_finish (authorizer, async_result, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_assert (success == FALSE);
	g_clear_error (&error);

	post_test_authentication_async (data, FALSE);

	g_main_loop_quit (data->main_loop);
}

/* Test that cancellation of asynchronous authentication works */
static void
test_client_login_authorizer_authenticate_async_cancellation (ClientLoginAuthorizerAsyncData *data, gconstpointer user_data)
{
	GCancellable *cancellable;

	gdata_test_mock_server_start_trace (mock_server, "client-login-authorizer-authenticate-async-cancellation");

	pre_test_authentication ((ClientLoginAuthorizerData*) data);

	/* Set up the cancellable */
	cancellable = g_cancellable_new ();

	/* Create a main loop and authenticate */
	gdata_client_login_authorizer_authenticate_async (data->parent.authorizer, USERNAME, PASSWORD, cancellable,
	                                                  (GAsyncReadyCallback) test_client_login_authorizer_authenticate_async_cancellation_cb,
	                                                  data);
	g_cancellable_cancel (cancellable);

	g_main_loop_run (data->main_loop);

	g_object_unref (cancellable);

	gdata_mock_server_end_trace (mock_server);
}

/* Test that gdata_authorizer_refresh_authorization() is a no-op (when authorised or not) */
static void
test_client_login_authorizer_refresh_authorization (ClientLoginAuthorizerData *data, gconstpointer user_data)
{
	GError *error = NULL;

	g_assert (gdata_authorizer_refresh_authorization (GDATA_AUTHORIZER (data->authorizer), NULL, &error) == FALSE);
	g_assert_no_error (error);
	g_clear_error (&error);
}

/* Test that processing a request with a NULL domain will not change the request. */
static void
test_client_login_authorizer_process_request_null (ClientLoginAuthorizerData *data, gconstpointer user_data)
{
	SoupMessage *message;
	SoupMessageHeadersIter iter;
	guint header_count = 0;
	const gchar *name, *value;

	/* Create a new message with an empty set of request headers */
	message = soup_message_new (SOUP_METHOD_GET, "https://example.com/");

	/* Process the message */
	gdata_authorizer_process_request (GDATA_AUTHORIZER (data->authorizer), NULL, message);

	/* Check that the set of request headers is still empty */
	soup_message_headers_iter_init (&iter, message->request_headers);

	while (soup_message_headers_iter_next (&iter, &name, &value) == TRUE) {
		header_count++;
	}

	g_assert_cmpuint (header_count, ==, 0);

	g_object_unref (message);
}

/* Test that processing a request with an authorizer which hasn't been authenticated yet will not change the request. */
static void
test_client_login_authorizer_process_request_unauthenticated (ClientLoginAuthorizerData *data, gconstpointer user_data)
{
	SoupMessage *message;
	SoupMessageHeadersIter iter;
	guint header_count = 0;
	const gchar *name, *value;

	/* Create a new message with an empty set of request headers */
	message = soup_message_new (SOUP_METHOD_GET, "https://example.com/");

	/* Process the message */
	gdata_authorizer_process_request (GDATA_AUTHORIZER (data->authorizer), gdata_youtube_service_get_primary_authorization_domain (), message);

	/* Check that the set of request headers is still empty */
	soup_message_headers_iter_init (&iter, message->request_headers);

	while (soup_message_headers_iter_next (&iter, &name, &value) == TRUE) {
		header_count++;
	}

	g_assert_cmpuint (header_count, ==, 0);

	g_object_unref (message);
}

/* Test that processing a request with an authorizer which has been authenticated will change the request. */
static void
test_client_login_authorizer_process_request_authenticated (ClientLoginAuthorizerData *data, gconstpointer user_data)
{
	SoupMessage *message;
	SoupMessageHeadersIter iter;
	guint header_count = 0;
	const gchar *name, *value;

	/* Create a new message with an empty set of request headers */
	message = soup_message_new (SOUP_METHOD_GET, "https://example.com/");

	/* Process the message */
	gdata_authorizer_process_request (GDATA_AUTHORIZER (data->authorizer), gdata_youtube_service_get_primary_authorization_domain (), message);

	/* Check that at least one new header has been set */
	soup_message_headers_iter_init (&iter, message->request_headers);

	while (soup_message_headers_iter_next (&iter, &name, &value) == TRUE) {
		header_count++;
	}

	g_assert_cmpuint (header_count, >, 0);

	g_object_unref (message);
}

/* Test that processing a HTTP request (as opposed to the more normal HTTPS request) with an authenticated authorizer will abort rather than
 * transmitting the user's private auth token over an insecure HTTP connection. */
static void
test_client_login_authorizer_process_request_insecure (ClientLoginAuthorizerData *data, gconstpointer user_data)
{
	SoupMessage *message;

	/* Create a new message which uses HTTP instead of HTTPS */
	message = soup_message_new (SOUP_METHOD_GET, "http://example.com/");

	/* Process the message */
	if (g_test_trap_fork (0, 0) == TRUE) {
		gdata_authorizer_process_request (GDATA_AUTHORIZER (data->authorizer), gdata_youtube_service_get_primary_authorization_domain (),
		                                message);
		exit (0);
	}

	/* Assert that it aborted */
	g_test_trap_assert_failed ();
	g_test_trap_assert_stderr_unmatched ("Not authorizing a non-HTTPS message with the user's ClientLogin "
	                                     "auth token as the connection isn't secure.");

	g_object_unref (message);
}

int
main (int argc, char *argv[])
{
	GFile *trace_directory;

	gdata_test_init (argc, argv);

	mock_server = gdata_test_get_mock_server ();
	trace_directory = g_file_new_for_path ("traces/client-login-authorizer");
	gdata_mock_server_set_trace_directory (mock_server, trace_directory);
	g_object_unref (trace_directory);

	main_thread = g_thread_self ();

	g_test_add_func ("/client-login-authorizer/constructor", test_client_login_authorizer_constructor);
	g_test_add_func ("/client-login-authorizer/constructor/for-domains", test_client_login_authorizer_constructor_for_domains);

	g_test_add ("/client-login-authorizer/properties/client-id", ClientLoginAuthorizerData, NULL, set_up_client_login_authorizer_data,
	            test_client_login_authorizer_properties_client_id, tear_down_client_login_authorizer_data);
	g_test_add ("/client-login-authorizer/properties/username", ClientLoginAuthorizerData, NULL, set_up_client_login_authorizer_data,
	            test_client_login_authorizer_properties_username, tear_down_client_login_authorizer_data);
	g_test_add ("/client-login-authorizer/properties/password", ClientLoginAuthorizerData, NULL, set_up_client_login_authorizer_data,
	            test_client_login_authorizer_properties_password, tear_down_client_login_authorizer_data);
	g_test_add ("/client-login-authorizer/properties/proxy-uri", ClientLoginAuthorizerData, NULL, set_up_client_login_authorizer_data,
	            test_client_login_authorizer_properties_proxy_uri, tear_down_client_login_authorizer_data);
	g_test_add ("/client-login-authorizer/properties/timeout", ClientLoginAuthorizerData, NULL, set_up_client_login_authorizer_data,
	            test_client_login_authorizer_properties_timeout, tear_down_client_login_authorizer_data);

	g_test_add ("/client-login-authorizer/refresh-authorization/unauthenticated", ClientLoginAuthorizerData, NULL,
	            set_up_client_login_authorizer_data, test_client_login_authorizer_refresh_authorization,
	            tear_down_client_login_authorizer_data);

	g_test_add ("/client-login-authorizer/process-request/null", ClientLoginAuthorizerData, NULL,
	            set_up_client_login_authorizer_data, test_client_login_authorizer_process_request_null, tear_down_client_login_authorizer_data);
	g_test_add ("/client-login-authorizer/process-request/unauthenticated", ClientLoginAuthorizerData, NULL,
	            set_up_client_login_authorizer_data, test_client_login_authorizer_process_request_unauthenticated,
	            tear_down_client_login_authorizer_data);

	/* Test once with the domain attached and once without */
	g_test_add ("/client-login-authorizer/authenticate/sync", ClientLoginAuthorizerData, USERNAME, set_up_client_login_authorizer_data,
	            test_client_login_authorizer_authenticate_sync, tear_down_client_login_authorizer_data);
	g_test_add ("/client-login-authorizer/authenticate/sync/no-domain", ClientLoginAuthorizerData, USERNAME_NO_DOMAIN,
	            set_up_client_login_authorizer_data, test_client_login_authorizer_authenticate_sync,
	            tear_down_client_login_authorizer_data);
	g_test_add ("/client-login-authorizer/authenticate/sync/bad-password", ClientLoginAuthorizerData, USERNAME,
	            set_up_client_login_authorizer_data, test_client_login_authorizer_authenticate_sync_bad_password,
	            tear_down_client_login_authorizer_data);
	g_test_add ("/client-login-authorizer/authenticate/sync/multiple-domains", ClientLoginAuthorizerData, NULL,
	            set_up_client_login_authorizer_data_multiple_domains, test_client_login_authorizer_authenticate_sync_multiple_domains,
	            tear_down_client_login_authorizer_data);
	g_test_add ("/client-login-authorizer/authenticate/sync/cancellation", ClientLoginAuthorizerData, NULL,
	            set_up_client_login_authorizer_data, test_client_login_authorizer_authenticate_sync_cancellation,
	            tear_down_client_login_authorizer_data);

	/* Async tests */
	g_test_add ("/client-login-authorizer/authenticate/async", ClientLoginAuthorizerAsyncData, NULL,
	            set_up_client_login_authorizer_async_data, test_client_login_authorizer_authenticate_async,
	            tear_down_client_login_authorizer_async_data);
	g_test_add ("/client-login-authorizer/authenticate/async/multiple-domains", ClientLoginAuthorizerAsyncData, NULL,
	            set_up_client_login_authorizer_async_data_multiple_domains,
	            test_client_login_authorizer_authenticate_async_multiple_domains, tear_down_client_login_authorizer_async_data);
	g_test_add ("/client-login-authorizer/authenticate/async/cancellation", ClientLoginAuthorizerAsyncData, NULL,
	            set_up_client_login_authorizer_async_data, test_client_login_authorizer_authenticate_async_cancellation,
	            tear_down_client_login_authorizer_async_data);

	/* Miscellaneous other tests which require authentication */
	g_test_add ("/client-login-authorizer/refresh-authorization/authenticated", ClientLoginAuthorizerData, NULL,
	            set_up_client_login_authorizer_data_authenticated, test_client_login_authorizer_refresh_authorization,
	            tear_down_client_login_authorizer_data);

	g_test_add ("/client-login-authorizer/process-request/authenticated", ClientLoginAuthorizerData, NULL,
	            set_up_client_login_authorizer_data_authenticated, test_client_login_authorizer_process_request_authenticated,
	            tear_down_client_login_authorizer_data);
	g_test_add ("/client-login-authorizer/process-request/insecure", ClientLoginAuthorizerData, NULL,
	            set_up_client_login_authorizer_data_authenticated, test_client_login_authorizer_process_request_insecure,
	            tear_down_client_login_authorizer_data);

	return g_test_run ();
}
