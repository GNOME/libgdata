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
test_oauth1_authorizer_constructor (void)
{
	GDataOAuth1Authorizer *authorizer;

	authorizer = gdata_oauth1_authorizer_new ("Application name", GDATA_TYPE_CONTACTS_SERVICE);

	g_assert (authorizer != NULL);
	g_assert (GDATA_IS_OAUTH1_AUTHORIZER (authorizer));
	g_assert (GDATA_IS_AUTHORIZER (authorizer));

	g_object_unref (authorizer);
}

static void
test_oauth1_authorizer_constructor_for_domains (void)
{
	GDataOAuth1Authorizer *authorizer;
	GDataAuthorizationDomain *domain;
	GList *domains;

	/* Try with standard domains first */
	domains = gdata_service_get_authorization_domains (GDATA_TYPE_CONTACTS_SERVICE);
	authorizer = gdata_oauth1_authorizer_new_for_authorization_domains ("Application name", domains);
	g_list_free (domains);

	g_assert (authorizer != NULL);
	g_assert (GDATA_IS_OAUTH1_AUTHORIZER (authorizer));
	g_assert (GDATA_IS_AUTHORIZER (authorizer));

	g_object_unref (authorizer);

	/* Try again with a custom domain. Note that, as in test_authorization_domain_properties() this should not normally happen in client code. */
	domain = GDATA_AUTHORIZATION_DOMAIN (g_object_new (GDATA_TYPE_AUTHORIZATION_DOMAIN,
	                                                   "service-name", "test",
	                                                   "scope", "test",
	                                                   NULL));

	domains = g_list_prepend (NULL, domain);
	authorizer = gdata_oauth1_authorizer_new_for_authorization_domains ("Application name", domains);
	g_list_free (domains);

	g_assert (authorizer != NULL);
	g_assert (GDATA_IS_OAUTH1_AUTHORIZER (authorizer));
	g_assert (GDATA_IS_AUTHORIZER (authorizer));

	g_object_unref (authorizer);
	g_object_unref (domain);
}

typedef struct {
	GDataOAuth1Authorizer *authorizer;

	guint locale_notification_count;
	gulong locale_signal_handler;
	guint proxy_uri_notification_count;
	gulong proxy_uri_signal_handler;
	guint timeout_notification_count;
	gulong timeout_signal_handler;
} OAuth1AuthorizerData;

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
connect_to_oauth1_authorizer (OAuth1AuthorizerData *data)
{
	/* Connect to notifications from the object to verify they're only emitted the correct number of times */
	data->locale_signal_handler = g_signal_connect (data->authorizer, "notify::locale", (GCallback) notify_cb,
	                                                &(data->locale_notification_count));
	data->proxy_uri_signal_handler = g_signal_connect (data->authorizer, "notify::proxy-uri", (GCallback) notify_cb,
	                                                   &(data->proxy_uri_notification_count));
	data->timeout_signal_handler = g_signal_connect (data->authorizer, "notify::timeout", (GCallback) notify_cb,
	                                                 &(data->timeout_notification_count));
}

static void
set_up_oauth1_authorizer_data (OAuth1AuthorizerData *data, gconstpointer user_data)
{
	data->authorizer = gdata_oauth1_authorizer_new ("Application name", GDATA_TYPE_CONTACTS_SERVICE);
	connect_to_oauth1_authorizer (data);
}

static void
set_up_oauth1_authorizer_data_fallback_application_name (OAuth1AuthorizerData *data, gconstpointer user_data)
{
	g_set_application_name ("Fallback name");
	data->authorizer = gdata_oauth1_authorizer_new (NULL, GDATA_TYPE_CONTACTS_SERVICE);
	connect_to_oauth1_authorizer (data);
}

static void
set_up_oauth1_authorizer_data_multiple_domains (OAuth1AuthorizerData *data, gconstpointer user_data)
{
	GList *authorization_domains = NULL;

	authorization_domains = g_list_prepend (authorization_domains, gdata_contacts_service_get_primary_authorization_domain ());
	authorization_domains = g_list_prepend (authorization_domains, gdata_picasaweb_service_get_primary_authorization_domain ());
	data->authorizer = gdata_oauth1_authorizer_new_for_authorization_domains ("Application name", authorization_domains);
	g_list_free (authorization_domains);

	connect_to_oauth1_authorizer (data);
}

static void
set_up_oauth1_authorizer_data_locale (OAuth1AuthorizerData *data, gconstpointer user_data)
{
	data->authorizer = gdata_oauth1_authorizer_new ("Application name", GDATA_TYPE_CONTACTS_SERVICE);
	gdata_oauth1_authorizer_set_locale (data->authorizer, "en_GB");
	connect_to_oauth1_authorizer (data);
}

/* Given an authentication URI, prompt the user to go to that URI, grant access to the test application and enter the resulting verifier */
static gchar *
query_user_for_verifier (const gchar *authentication_uri)
{
	char verifier[100];

	/* Wait for the user to retrieve and enter the verifier */
	g_print ("Please navigate to the following URI and grant access: %s\n", authentication_uri);
	g_print ("Enter verifier (EOF to skip test): ");
	if (scanf ("%100s", verifier) != 1) {
		/* Skip the test */
		g_test_message ("Skipping test on user request.");
		return NULL;
	}

	g_test_message ("Proceeding with user-provided verifier “%s”.", verifier);

	return g_strdup (verifier);
}

static void
set_up_oauth1_authorizer_data_authenticated (OAuth1AuthorizerData *data, gconstpointer user_data)
{
	gboolean *skip_test = (gboolean*) user_data;
	gchar *authentication_uri, *token, *token_secret, *verifier;

	gdata_test_mock_server_start_trace (mock_server, "setup-oauth1-authorizer-data-authenticated");

	/* Chain up */
	set_up_oauth1_authorizer_data (data, NULL);

	/* Get an authentication URI */
	authentication_uri = gdata_oauth1_authorizer_request_authentication_uri (data->authorizer, &token, &token_secret, NULL, NULL);
	g_assert (authentication_uri != NULL);

	/* Get the verifier off the user */
	verifier = query_user_for_verifier (authentication_uri);

	g_free (authentication_uri);

	if (verifier == NULL) {
		*skip_test = TRUE;
		goto skip_test;
	}

	/* Authorise the token */
	g_assert (gdata_oauth1_authorizer_request_authorization (data->authorizer, token, token_secret, verifier, NULL, NULL) == TRUE);

skip_test:
	g_free (token);
	g_free (token_secret);
	g_free (verifier);

	gdata_mock_server_end_trace (mock_server);
}

static void
tear_down_oauth1_authorizer_data (OAuth1AuthorizerData *data, gconstpointer user_data)
{
	/* Clean up signal handlers */
	g_signal_handler_disconnect (data->authorizer, data->timeout_signal_handler);
	g_signal_handler_disconnect (data->authorizer, data->proxy_uri_signal_handler);
	g_signal_handler_disconnect (data->authorizer, data->locale_signal_handler);

	g_object_unref (data->authorizer);
}

/* Test getting and setting the application-name property */
static void
test_oauth1_authorizer_properties_application_name (OAuth1AuthorizerData *data, gconstpointer user_data)
{
	gchar *application_name;

	/* Verifying the normal state of the property in a newly-constructed instance of GDataOAuth1Authorizer */
	g_assert_cmpstr (gdata_oauth1_authorizer_get_application_name (data->authorizer), ==, "Application name");

	g_object_get (data->authorizer, "application-name", &application_name, NULL);
	g_assert_cmpstr (application_name, ==, "Application name");
	g_free (application_name);
}

/* Test the fallback for the application-name property */
static void
test_oauth1_authorizer_properties_application_name_fallback (OAuth1AuthorizerData *data, gconstpointer user_data)
{
	gchar *application_name;

	/* Verifying the normal state of the property in a newly-constructed instance of GDataOAuth1Authorizer */
	g_assert_cmpstr (gdata_oauth1_authorizer_get_application_name (data->authorizer), ==, "Fallback name");

	g_object_get (data->authorizer, "application-name", &application_name, NULL);
	g_assert_cmpstr (application_name, ==, "Fallback name");
	g_free (application_name);
}

/* Test getting and setting the locale property */
static void
test_oauth1_authorizer_properties_locale (OAuth1AuthorizerData *data, gconstpointer user_data)
{
	gchar *locale;

	/* Verifying the normal state of the property in a newly-constructed instance of GDataOAuth1Authorizer */
	g_assert_cmpstr (gdata_oauth1_authorizer_get_locale (data->authorizer), ==, NULL);

	g_object_get (data->authorizer, "locale", &locale, NULL);
	g_assert_cmpstr (locale, ==, NULL);
	g_free (locale);

	g_assert_cmpuint (data->locale_notification_count, ==, 0);

	/* Check setting it works and emits a notification */
	gdata_oauth1_authorizer_set_locale (data->authorizer, "en");

	g_assert_cmpuint (data->locale_notification_count, ==, 1);

	g_assert_cmpstr (gdata_oauth1_authorizer_get_locale (data->authorizer), ==, "en");

	g_object_get (data->authorizer, "locale", &locale, NULL);
	g_assert_cmpstr (locale, ==, "en");
	g_free (locale);

	/* Check setting it to the same value is a no-op */
	gdata_oauth1_authorizer_set_locale (data->authorizer, "en");
	g_assert_cmpuint (data->locale_notification_count, ==, 1);

	/* Check setting it back to NULL works */
	gdata_oauth1_authorizer_set_locale (data->authorizer, NULL);

	g_assert_cmpuint (data->locale_notification_count, ==, 2);

	g_assert_cmpstr (gdata_oauth1_authorizer_get_locale (data->authorizer), ==, NULL);

	g_object_get (data->authorizer, "locale", &locale, NULL);
	g_assert_cmpstr (locale, ==, NULL);
	g_free (locale);

	/* Test that setting it using g_object_set() works */
	g_object_set (data->authorizer, "locale", "de", NULL);
	g_assert_cmpstr (gdata_oauth1_authorizer_get_locale (data->authorizer), ==, "de");
}

/* Test getting and setting the proxy-uri property */
static void
test_oauth1_authorizer_properties_proxy_uri (OAuth1AuthorizerData *data, gconstpointer user_data)
{
	SoupURI *proxy_uri, *new_proxy_uri;

	/* Verifying the normal state of the property in a newly-constructed instance of GDataOAuth1Authorizer */
	g_assert (gdata_oauth1_authorizer_get_proxy_uri (data->authorizer) == NULL);

	g_object_get (data->authorizer, "proxy-uri", &proxy_uri, NULL);
	g_assert (proxy_uri == NULL);

	g_assert_cmpuint (data->proxy_uri_notification_count, ==, 0);

	/* Check setting it works and emits a notification */
	new_proxy_uri = soup_uri_new ("http://example.com/");
	gdata_oauth1_authorizer_set_proxy_uri (data->authorizer, new_proxy_uri);

	g_assert_cmpuint (data->proxy_uri_notification_count, ==, 1);

	g_assert (gdata_oauth1_authorizer_get_proxy_uri (data->authorizer) != NULL);
	g_assert (soup_uri_equal (gdata_oauth1_authorizer_get_proxy_uri (data->authorizer), new_proxy_uri) == TRUE);

	g_object_get (data->authorizer, "proxy-uri", &proxy_uri, NULL);
	g_assert (proxy_uri != NULL);
	g_assert (soup_uri_equal (gdata_oauth1_authorizer_get_proxy_uri (data->authorizer), new_proxy_uri) == TRUE);
	soup_uri_free (proxy_uri);

	soup_uri_free (new_proxy_uri);

	/* Check setting it back to NULL works */
	gdata_oauth1_authorizer_set_proxy_uri (data->authorizer, NULL);

	g_assert_cmpuint (data->proxy_uri_notification_count, ==, 2);

	g_assert (gdata_oauth1_authorizer_get_proxy_uri (data->authorizer) == NULL);

	g_object_get (data->authorizer, "proxy-uri", &proxy_uri, NULL);
	g_assert (proxy_uri == NULL);

	/* Test that setting it using g_object_set() works */
	new_proxy_uri = soup_uri_new ("http://example.com/");
	g_object_set (data->authorizer, "proxy-uri", new_proxy_uri, NULL);
	soup_uri_free (new_proxy_uri);

	g_assert (gdata_oauth1_authorizer_get_proxy_uri (data->authorizer) != NULL);
}

/* Test getting and setting the timeout property */
static void
test_oauth1_authorizer_properties_timeout (OAuth1AuthorizerData *data, gconstpointer user_data)
{
	guint timeout;

	/* Verifying the normal state of the property in a newly-constructed instance of GDataOAuth1Authorizer */
	g_assert_cmpuint (gdata_oauth1_authorizer_get_timeout (data->authorizer), ==, 0);

	g_object_get (data->authorizer, "timeout", &timeout, NULL);
	g_assert_cmpuint (timeout, ==, 0);

	g_assert_cmpuint (data->timeout_notification_count, ==, 0);

	/* Check setting it works and emits a notification */
	gdata_oauth1_authorizer_set_timeout (data->authorizer, 30);

	g_assert_cmpuint (data->timeout_notification_count, ==, 1);

	g_assert_cmpuint (gdata_oauth1_authorizer_get_timeout (data->authorizer), ==, 30);

	g_object_get (data->authorizer, "timeout", &timeout, NULL);
	g_assert_cmpuint (timeout, ==, 30);

	/* Check setting it to the same value is a no-op */
	gdata_oauth1_authorizer_set_timeout (data->authorizer, 30);
	g_assert_cmpuint (data->timeout_notification_count, ==, 1);

	/* Check setting it back to 0 works */
	gdata_oauth1_authorizer_set_timeout (data->authorizer, 0);

	g_assert_cmpuint (data->timeout_notification_count, ==, 2);

	g_assert_cmpuint (gdata_oauth1_authorizer_get_timeout (data->authorizer), ==, 0);

	g_object_get (data->authorizer, "timeout", &timeout, NULL);
	g_assert_cmpuint (timeout, ==, 0);

	/* Test that setting it using g_object_set() works */
	g_object_set (data->authorizer, "timeout", 15, NULL);
	g_assert_cmpuint (gdata_oauth1_authorizer_get_timeout (data->authorizer), ==, 15);
}

/* Test that gdata_authorizer_refresh_authorization() is a no-op (whether authorised or not) */
static void
test_oauth1_authorizer_refresh_authorization (OAuth1AuthorizerData *data, gconstpointer user_data)
{
	gboolean *skip_test = (gboolean*) user_data;
	GError *error = NULL;

	/* Skip the test if the user's requested */
	if (skip_test != NULL && *skip_test == TRUE) {
		return;
	}

	gdata_test_mock_server_start_trace (mock_server, "oauth1-authorizer-refresh-authorization");

	g_assert (gdata_authorizer_refresh_authorization (GDATA_AUTHORIZER (data->authorizer), NULL, &error) == FALSE);
	g_assert_no_error (error);
	g_clear_error (&error);

	gdata_mock_server_end_trace (mock_server);
}

/* Test that processing a request with a NULL domain will not change the request. */
static void
test_oauth1_authorizer_process_request_null (OAuth1AuthorizerData *data, gconstpointer user_data)
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
test_oauth1_authorizer_process_request_unauthenticated (OAuth1AuthorizerData *data, gconstpointer user_data)
{
	SoupMessage *message;
	SoupMessageHeadersIter iter;
	guint header_count = 0;
	const gchar *name, *value;

	/* Create a new message with an empty set of request headers */
	message = soup_message_new (SOUP_METHOD_GET, "https://example.com/");

	/* Process the message */
	gdata_authorizer_process_request (GDATA_AUTHORIZER (data->authorizer), gdata_contacts_service_get_primary_authorization_domain (), message);

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
test_oauth1_authorizer_process_request_authenticated (OAuth1AuthorizerData *data, gconstpointer user_data)
{
	gboolean *skip_test = (gboolean*) user_data;
	SoupMessage *message;
	SoupMessageHeadersIter iter;
	guint header_count = 0;
	const gchar *name, *value;

	/* Skip the test if the user's requested */
	if (skip_test != NULL && *skip_test == TRUE) {
		return;
	}

	/* Create a new message with an empty set of request headers */
	message = soup_message_new (SOUP_METHOD_GET, "http://example.com/");

	/* Process the message */
	gdata_authorizer_process_request (GDATA_AUTHORIZER (data->authorizer), gdata_contacts_service_get_primary_authorization_domain (), message);

	/* Check that at least one new header has been set */
	soup_message_headers_iter_init (&iter, message->request_headers);

	while (soup_message_headers_iter_next (&iter, &name, &value) == TRUE) {
		header_count++;
	}

	g_assert_cmpuint (header_count, >, 0);

	g_object_unref (message);
}

/* Test that requesting an authentication URI synchronously works correctly */
static void
test_oauth1_authorizer_request_authentication_uri_sync (OAuth1AuthorizerData *data, gconstpointer user_data)
{
	gchar *authentication_uri, *token, *token_secret;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "oauth1-authorizer-request-authentication-uri-sync");

	authentication_uri = gdata_oauth1_authorizer_request_authentication_uri (data->authorizer, &token, &token_secret, NULL, &error);
	g_assert_no_error (error);
	g_assert (authentication_uri != NULL && *authentication_uri != '\0');
	g_assert (token != NULL && *token != '\0');
	g_assert (token_secret != NULL && *token != '\0');
	g_clear_error (&error);

	g_test_message ("Requesting an authentication URI gave “%s” with request token “%s” and request token secret “%s”.",
	                authentication_uri, token, token_secret);

	g_free (authentication_uri);
	g_free (token);
	g_free (token_secret);

	gdata_mock_server_end_trace (mock_server);
}

/* Test that requesting an authentication URI synchronously can be cancelled */
static void
test_oauth1_authorizer_request_authentication_uri_sync_cancellation (OAuth1AuthorizerData *data, gconstpointer user_data)
{
	/* Initialise token and token_secret so we check that gdata_oauth1_authorizer_request_authentication_uri() NULLifies them on error */
	gchar *authentication_uri, *token = (gchar*) "error", *token_secret = (gchar*) "error";
	GCancellable *cancellable;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "oauth1-authorizer-request-authentication-uri-sync-cancellation");

	/* Set up the cancellable */
	cancellable = g_cancellable_new ();

	/* Get a request token. This should return immediately as the cancellable was cancelled beforehand. */
	g_cancellable_cancel (cancellable);
	authentication_uri = gdata_oauth1_authorizer_request_authentication_uri (data->authorizer, &token, &token_secret, cancellable, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_assert (authentication_uri == NULL);
	g_assert (token == NULL);
	g_assert (token_secret == NULL);
	g_clear_error (&error);

	g_free (authentication_uri);
	g_free (token);
	g_free (token_secret);

	g_object_unref (cancellable);

	gdata_mock_server_end_trace (mock_server);
}

typedef struct {
	OAuth1AuthorizerData parent;
	GMainLoop *main_loop;
} OAuth1AuthorizerAsyncData;

static void
set_up_oauth1_authorizer_async_data (OAuth1AuthorizerAsyncData *data, gconstpointer user_data)
{
	/* Chain up */
	set_up_oauth1_authorizer_data ((OAuth1AuthorizerData*) data, user_data);

	/* Set up the main loop */
	data->main_loop = g_main_loop_new (NULL, FALSE);
}

static void
set_up_oauth1_authorizer_async_data_multiple_domains (OAuth1AuthorizerAsyncData *data, gconstpointer user_data)
{
	/* Chain up */
	set_up_oauth1_authorizer_data_multiple_domains ((OAuth1AuthorizerData*) data, user_data);

	/* Set up the main loop */
	data->main_loop = g_main_loop_new (NULL, FALSE);
}

static void
tear_down_oauth1_authorizer_async_data (OAuth1AuthorizerAsyncData *data, gconstpointer user_data)
{
	g_main_loop_unref (data->main_loop);

	/* Chain up */
	tear_down_oauth1_authorizer_data ((OAuth1AuthorizerData*) data, user_data);
}

static void
test_oauth1_authorizer_request_authentication_uri_async_cb (GDataOAuth1Authorizer *authorizer, GAsyncResult *async_result,
                                                            OAuth1AuthorizerAsyncData *data)
{
	gchar *authentication_uri, *token, *token_secret;
	GError *error = NULL;

	authentication_uri = gdata_oauth1_authorizer_request_authentication_uri_finish (authorizer, async_result, &token, &token_secret, &error);
	g_assert_no_error (error);
	g_assert (authentication_uri != NULL && *authentication_uri != '\0');
	g_assert (token != NULL && *token != '\0');
	g_assert (token_secret != NULL && *token != '\0');
	g_clear_error (&error);

	g_test_message ("Requesting an authentication URI asynchronously gave “%s” with request token “%s” and request token secret “%s”.",
	                authentication_uri, token, token_secret);

	g_free (authentication_uri);
	g_free (token);
	g_free (token_secret);

	g_main_loop_quit (data->main_loop);
}

/* Test that asynchronously requesting an authentication URI for a single authorization domain works */
static void
test_oauth1_authorizer_request_authentication_uri_async (OAuth1AuthorizerAsyncData *data, gconstpointer user_data)
{
	gdata_test_mock_server_start_trace (mock_server, "oauth1-authorizer-request-authentication-uri-async");

	/* Create a main loop and request an authentication URI */
	gdata_oauth1_authorizer_request_authentication_uri_async (data->parent.authorizer, NULL,
	                                                          (GAsyncReadyCallback) test_oauth1_authorizer_request_authentication_uri_async_cb,
	                                                          data);

	g_main_loop_run (data->main_loop);

	gdata_mock_server_end_trace (mock_server);
}

static void
test_oauth1_authorizer_request_authentication_uri_async_cancellation_cb (GDataOAuth1Authorizer *authorizer, GAsyncResult *async_result,
                                                                         OAuth1AuthorizerAsyncData *data)
{
	/* Initialise token and token_secret so we check that gdata_oauth1_authorizer_request_authentication_uri_finish() NULLifies them on error */
	gchar *authentication_uri, *token = (gchar*) "error", *token_secret = (gchar*) "error";
	GError *error = NULL;

	authentication_uri = gdata_oauth1_authorizer_request_authentication_uri_finish (authorizer, async_result, &token, &token_secret, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_assert (authentication_uri == NULL);
	g_assert (token == NULL);
	g_assert (token_secret == NULL);
	g_clear_error (&error);

	g_free (authentication_uri);
	g_free (token);
	g_free (token_secret);

	g_main_loop_quit (data->main_loop);
}

/* Test that cancellation of asynchronous authentication URI requests work */
static void
test_oauth1_authorizer_request_authentication_uri_async_cancellation (OAuth1AuthorizerAsyncData *data, gconstpointer user_data)
{
	GCancellable *cancellable;

	gdata_test_mock_server_start_trace (mock_server, "oauth1-authorizer-request-authentication-uri-async-cancellation");

	/* Set up the cancellable */
	cancellable = g_cancellable_new ();

	/* Create a main loop and request an authentication URI */
	gdata_oauth1_authorizer_request_authentication_uri_async (data->parent.authorizer, cancellable,
	                                                          (GAsyncReadyCallback)
	                                                              test_oauth1_authorizer_request_authentication_uri_async_cancellation_cb,
	                                                          data);
	g_cancellable_cancel (cancellable);

	g_main_loop_run (data->main_loop);

	g_object_unref (cancellable);

	gdata_mock_server_end_trace (mock_server);
}

typedef struct {
	OAuth1AuthorizerData parent;
	gchar *token;
	gchar *token_secret;
	gchar *verifier;
} OAuth1AuthorizerInteractiveData;

/* NOTE: Any consumer of this data has to check for (data->verifier == NULL) and skip the test in that case */
static void
set_up_oauth1_authorizer_interactive_data (OAuth1AuthorizerInteractiveData *data, gconstpointer user_data)
{
	gchar *authentication_uri;

	/* Chain up */
	set_up_oauth1_authorizer_data ((OAuth1AuthorizerData*) data, user_data);

	gdata_test_mock_server_start_trace (mock_server, "setup-oauth1-authorizer-interactive-data");

	/* Get an authentication URI */
	authentication_uri = gdata_oauth1_authorizer_request_authentication_uri (data->parent.authorizer, &(data->token), &(data->token_secret),
	                                                                         NULL, NULL);
	g_assert (authentication_uri != NULL);

	/* Wait for the user to retrieve and enter the verifier */
	data->verifier = query_user_for_verifier (authentication_uri);

	g_free (authentication_uri);

	gdata_mock_server_end_trace (mock_server);
}

static void
set_up_oauth1_authorizer_interactive_data_bad_credentials (OAuth1AuthorizerInteractiveData *data, gconstpointer user_data)
{
	gchar *authentication_uri;

	/* Chain up */
	set_up_oauth1_authorizer_data ((OAuth1AuthorizerData*) data, user_data);

	gdata_test_mock_server_start_trace (mock_server, "oauth1-authorizer-interactive-data-bad-credentials");

	/* Get an authentication URI */
	authentication_uri = gdata_oauth1_authorizer_request_authentication_uri (data->parent.authorizer, &(data->token), &(data->token_secret),
	                                                                         NULL, NULL);
	g_assert (authentication_uri != NULL);

	/* Give a bogus verifier */
	data->verifier = g_strdup ("test");

	g_free (authentication_uri);

	gdata_mock_server_end_trace (mock_server);
}

static void
tear_down_oauth1_authorizer_interactive_data (OAuth1AuthorizerInteractiveData *data, gconstpointer user_data)
{
	g_free (data->token);
	g_free (data->token_secret);
	g_free (data->verifier);

	/* Chain up */
	tear_down_oauth1_authorizer_data ((OAuth1AuthorizerData*) data, user_data);
}

/* Test that synchronously authorizing a request token is successful. Note that this test has to be interactive, as the user has to visit the
 * authentication URI to retrieve a verifier for the request token. */
static void
test_oauth1_authorizer_request_authorization_sync (OAuth1AuthorizerInteractiveData *data, gconstpointer user_data)
{
	gboolean success;
	GError *error = NULL;

	/* Skip the test if the user's requested */
	if (data->verifier == NULL) {
		return;
	}

	gdata_test_mock_server_start_trace (mock_server, "oauth1-authorizer-request-authorization-sync");

	/* Check we're not authorised beforehand */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.authorizer),
	          gdata_contacts_service_get_primary_authorization_domain ()) == FALSE);

	/* Test that authorising the token retrieved previously is successful */
	success = gdata_oauth1_authorizer_request_authorization (data->parent.authorizer, data->token, data->token_secret, data->verifier, NULL,
	                                                         &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);
	g_clear_error (&error);

	/* Are we authorised now? */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.authorizer),
	          gdata_contacts_service_get_primary_authorization_domain ()) == TRUE);

	gdata_mock_server_end_trace (mock_server);
}

/* Test that synchronously authorizing a request token fails if an invalid verifier is provided. */
static void
test_oauth1_authorizer_request_authorization_sync_bad_credentials (OAuth1AuthorizerInteractiveData *data, gconstpointer user_data)
{
	gboolean success;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "oauth1-authorizer-request-authorization-sync-bad-credentials");

	/* Check we're not authorised beforehand */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.authorizer),
	          gdata_contacts_service_get_primary_authorization_domain ()) == FALSE);

	/* Test that authorising the token retrieved above fails */
	success = gdata_oauth1_authorizer_request_authorization (data->parent.authorizer, data->token, data->token_secret, data->verifier, NULL,
	                                                         &error);
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_FORBIDDEN);
	g_assert (success == FALSE);
	g_clear_error (&error);

	/* Are we authorised now? */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.authorizer),
	          gdata_contacts_service_get_primary_authorization_domain ()) == FALSE);

	gdata_mock_server_end_trace (mock_server);
}

/* Test that cancellation of synchronously authorizing a request token works. Note that this test has to be interactive, as the user has to visit the
 * authentication URI to retrieve a verifier for the request token. */
static void
test_oauth1_authorizer_request_authorization_sync_cancellation (OAuth1AuthorizerInteractiveData *data, gconstpointer user_data)
{
	gboolean success;
	GCancellable *cancellable;
	GError *error = NULL;

	/* Skip the test if the user's requested */
	if (data->verifier == NULL) {
		return;
	}

	gdata_test_mock_server_start_trace (mock_server, "oauth1-authorizer-request-authorization-sync-cancellation");

	/* Check we're not authorised beforehand */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.authorizer),
	          gdata_contacts_service_get_primary_authorization_domain ()) == FALSE);

	/* Create the cancellable */
	cancellable = g_cancellable_new ();

	/* Test that authorising the token retrieved above is successful */
	g_cancellable_cancel (cancellable);
	success = gdata_oauth1_authorizer_request_authorization (data->parent.authorizer, data->token, data->token_secret, data->verifier,
	                                                         cancellable, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_assert (success == FALSE);
	g_clear_error (&error);

	/* Are we authorised now? */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.authorizer),
	          gdata_contacts_service_get_primary_authorization_domain ()) == FALSE);

	g_object_unref (cancellable);

	gdata_mock_server_end_trace (mock_server);
}

typedef struct {
	OAuth1AuthorizerInteractiveData parent;
	GMainLoop *main_loop;
} OAuth1AuthorizerInteractiveAsyncData;

/* NOTE: Any consumer of this data has to check for (data->verifier == NULL) and skip the test in that case */
static void
set_up_oauth1_authorizer_interactive_async_data (OAuth1AuthorizerInteractiveAsyncData *data, gconstpointer user_data)
{
	/* Chain up */
	set_up_oauth1_authorizer_interactive_data ((OAuth1AuthorizerInteractiveData*) data, user_data);

	/* Set up the main loop */
	data->main_loop = g_main_loop_new (NULL, FALSE);
}

static void
set_up_oauth1_authorizer_interactive_async_data_bad_credentials (OAuth1AuthorizerInteractiveAsyncData *data, gconstpointer user_data)
{
	/* Chain up */
	set_up_oauth1_authorizer_interactive_data_bad_credentials ((OAuth1AuthorizerInteractiveData*) data, user_data);

	/* Set up the main loop */
	data->main_loop = g_main_loop_new (NULL, FALSE);
}

static void
tear_down_oauth1_authorizer_interactive_async_data (OAuth1AuthorizerInteractiveAsyncData *data, gconstpointer user_data)
{
	g_main_loop_unref (data->main_loop);

	/* Chain up */
	tear_down_oauth1_authorizer_interactive_data ((OAuth1AuthorizerInteractiveData*) data, user_data);
}

static void
test_oauth1_authorizer_request_authorization_async_cb (GDataOAuth1Authorizer *authorizer, GAsyncResult *async_result,
                                                       OAuth1AuthorizerInteractiveAsyncData *data)
{
	gboolean success;
	GError *error = NULL;

	success = gdata_oauth1_authorizer_request_authorization_finish (authorizer, async_result, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);
	g_clear_error (&error);

	/* Are we authorised now? */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	          gdata_contacts_service_get_primary_authorization_domain ()) == TRUE);

	g_main_loop_quit (data->main_loop);
}

/* Test that asynchronously authorizing a request token works. Note that this test has to be interactive, as the user has to visit the
 * authentication URI to retrieve a verifier for the request token. */
static void
test_oauth1_authorizer_request_authorization_async (OAuth1AuthorizerInteractiveAsyncData *data, gconstpointer user_data)
{
	/* Skip the test if the user's requested */
	if (data->parent.verifier == NULL) {
		return;
	}

	gdata_test_mock_server_start_trace (mock_server, "oauth1-authorizer-request-authorization-async");

	/* Check we're not authorised beforehand */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.parent.authorizer),
	          gdata_contacts_service_get_primary_authorization_domain ()) == FALSE);

	/* Create a main loop and request authorization */
	gdata_oauth1_authorizer_request_authorization_async (data->parent.parent.authorizer, data->parent.token, data->parent.token_secret,
	                                                     data->parent.verifier, NULL,
	                                                     (GAsyncReadyCallback) test_oauth1_authorizer_request_authorization_async_cb, data);

	g_main_loop_run (data->main_loop);

	gdata_mock_server_end_trace (mock_server);
}

static void
test_oauth1_authorizer_request_authorization_async_bad_credentials_cb (GDataOAuth1Authorizer *authorizer, GAsyncResult *async_result,
                                                                       OAuth1AuthorizerInteractiveAsyncData *data)
{
	gboolean success;
	GError *error = NULL;

	success = gdata_oauth1_authorizer_request_authorization_finish (authorizer, async_result, &error);
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_FORBIDDEN);
	g_assert (success == FALSE);
	g_clear_error (&error);

	/* Are we authorised now? */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	          gdata_contacts_service_get_primary_authorization_domain ()) == FALSE);

	g_main_loop_quit (data->main_loop);
}

/* Test that asynchronously authorizing a request token fails if an invalid verifier is provided. */
static void
test_oauth1_authorizer_request_authorization_async_bad_credentials (OAuth1AuthorizerInteractiveAsyncData *data, gconstpointer user_data)
{
	gdata_test_mock_server_start_trace (mock_server, "oauth1-authorizer-request-authorization-async-bad-credentials");

	/* Check we're not authorised beforehand */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.parent.authorizer),
	          gdata_contacts_service_get_primary_authorization_domain ()) == FALSE);

	/* Create a main loop and request authorization */
	gdata_oauth1_authorizer_request_authorization_async (data->parent.parent.authorizer, data->parent.token, data->parent.token_secret,
	                                                     data->parent.verifier, NULL,
	                                                     (GAsyncReadyCallback)
	                                                         test_oauth1_authorizer_request_authorization_async_bad_credentials_cb,
	                                                     data);

	g_main_loop_run (data->main_loop);

	gdata_mock_server_end_trace (mock_server);
}

static void
test_oauth1_authorizer_request_authorization_async_cancellation_cb (GDataOAuth1Authorizer *authorizer, GAsyncResult *async_result,
                                                                    OAuth1AuthorizerInteractiveAsyncData *data)
{
	gboolean success;
	GError *error = NULL;

	success = gdata_oauth1_authorizer_request_authorization_finish (authorizer, async_result, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_assert (success == FALSE);
	g_clear_error (&error);

	/* Are we authorised now? */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	          gdata_contacts_service_get_primary_authorization_domain ()) == FALSE);

	g_main_loop_quit (data->main_loop);
}

/* Test that cancelling asynchronously authorizing a request token works. Note that this test has to be interactive, as the user has to visit the
 * authentication URI to retrieve a verifier for the request token. */
static void
test_oauth1_authorizer_request_authorization_async_cancellation (OAuth1AuthorizerInteractiveAsyncData *data, gconstpointer user_data)
{
	GCancellable *cancellable;

	/* Skip the test if the user's requested */
	if (data->parent.verifier == NULL) {
		return;
	}

	gdata_test_mock_server_start_trace (mock_server, "oauth1-authorizer-request-authorization-async-cancellation");

	/* Check we're not authorised beforehand */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.parent.authorizer),
	          gdata_contacts_service_get_primary_authorization_domain ()) == FALSE);

	/* Create the cancellable */
	cancellable = g_cancellable_new ();

	/* Create a main loop and request authorization */
	gdata_oauth1_authorizer_request_authorization_async (data->parent.parent.authorizer, data->parent.token, data->parent.token_secret,
	                                                     data->parent.verifier, cancellable,
	                                                     (GAsyncReadyCallback) test_oauth1_authorizer_request_authorization_async_cancellation_cb,
	                                                     data);
	g_cancellable_cancel (cancellable);

	g_main_loop_run (data->main_loop);

	g_object_unref (cancellable);

	gdata_mock_server_end_trace (mock_server);
}

int
main (int argc, char *argv[])
{
	GFile *trace_directory;

	gdata_test_init (argc, argv);

	mock_server = gdata_test_get_mock_server ();
	trace_directory = g_file_new_for_path ("traces/oauth1-authorizer");
	gdata_mock_server_set_trace_directory (mock_server, trace_directory);
	g_object_unref (trace_directory);

	main_thread = g_thread_self ();

	g_test_add_func ("/oauth1-authorizer/constructor", test_oauth1_authorizer_constructor);
	g_test_add_func ("/oauth1-authorizer/constructor/for-domains", test_oauth1_authorizer_constructor_for_domains);

	g_test_add ("/oauth1-authorizer/properties/application-name", OAuth1AuthorizerData, NULL, set_up_oauth1_authorizer_data,
	            test_oauth1_authorizer_properties_application_name, tear_down_oauth1_authorizer_data);
	g_test_add ("/oauth1-authorizer/properties/application-name/fallback", OAuth1AuthorizerData, NULL,
	            set_up_oauth1_authorizer_data_fallback_application_name, test_oauth1_authorizer_properties_application_name_fallback,
	            tear_down_oauth1_authorizer_data);
	g_test_add ("/oauth1-authorizer/properties/locale", OAuth1AuthorizerData, NULL, set_up_oauth1_authorizer_data,
	            test_oauth1_authorizer_properties_locale, tear_down_oauth1_authorizer_data);
	g_test_add ("/oauth1-authorizer/properties/proxy-uri", OAuth1AuthorizerData, NULL, set_up_oauth1_authorizer_data,
	            test_oauth1_authorizer_properties_proxy_uri, tear_down_oauth1_authorizer_data);
	g_test_add ("/oauth1-authorizer/properties/timeout", OAuth1AuthorizerData, NULL, set_up_oauth1_authorizer_data,
	            test_oauth1_authorizer_properties_timeout, tear_down_oauth1_authorizer_data);

	g_test_add ("/oauth1-authorizer/refresh-authorization/unauthenticated", OAuth1AuthorizerData, NULL,
	            set_up_oauth1_authorizer_data, test_oauth1_authorizer_refresh_authorization, tear_down_oauth1_authorizer_data);

	g_test_add ("/oauth1-authorizer/process-request/null", OAuth1AuthorizerData, NULL,
	            set_up_oauth1_authorizer_data, test_oauth1_authorizer_process_request_null, tear_down_oauth1_authorizer_data);
	g_test_add ("/oauth1-authorizer/process-request/unauthenticated", OAuth1AuthorizerData, NULL,
	            set_up_oauth1_authorizer_data, test_oauth1_authorizer_process_request_unauthenticated, tear_down_oauth1_authorizer_data);

	/* Sync request-authentication-uri tests */
	g_test_add ("/oauth1-authorizer/request-authentication-uri/sync", OAuth1AuthorizerData, NULL, set_up_oauth1_authorizer_data,
	            test_oauth1_authorizer_request_authentication_uri_sync, tear_down_oauth1_authorizer_data);
	g_test_add ("/oauth1-authorizer/request-authentication-uri/sync/multiple-domains", OAuth1AuthorizerData, NULL,
	            set_up_oauth1_authorizer_data_multiple_domains, test_oauth1_authorizer_request_authentication_uri_sync,
	            tear_down_oauth1_authorizer_data);
	g_test_add ("/oauth1-authorizer/request-authentication-uri/sync/multiple-domains", OAuth1AuthorizerData, NULL,
	            set_up_oauth1_authorizer_data_locale, test_oauth1_authorizer_request_authentication_uri_sync,
	            tear_down_oauth1_authorizer_data);
	g_test_add ("/oauth1-authorizer/request-authentication-uri/sync/cancellation", OAuth1AuthorizerData, NULL,
	            set_up_oauth1_authorizer_data, test_oauth1_authorizer_request_authentication_uri_sync_cancellation,
	            tear_down_oauth1_authorizer_data);

	/* Async request-authentication-uri tests */
	g_test_add ("/oauth1-authorizer/request-authentication-uri/async", OAuth1AuthorizerAsyncData, NULL,
	            set_up_oauth1_authorizer_async_data, test_oauth1_authorizer_request_authentication_uri_async,
	            tear_down_oauth1_authorizer_async_data);
	g_test_add ("/oauth1-authorizer/request-authentication-uri/async/multiple-domains", OAuth1AuthorizerAsyncData, NULL,
	            set_up_oauth1_authorizer_async_data_multiple_domains, test_oauth1_authorizer_request_authentication_uri_async,
	            tear_down_oauth1_authorizer_async_data);
	g_test_add ("/oauth1-authorizer/request-authentication-uri/async/cancellation", OAuth1AuthorizerAsyncData, NULL,
	            set_up_oauth1_authorizer_async_data, test_oauth1_authorizer_request_authentication_uri_async_cancellation,
	            tear_down_oauth1_authorizer_async_data);

	/* Sync request-authorization tests */
	if (gdata_test_interactive () == TRUE) {
		g_test_add ("/oauth1-authorizer/request-authorization/sync", OAuth1AuthorizerInteractiveData, NULL,
		            set_up_oauth1_authorizer_interactive_data, test_oauth1_authorizer_request_authorization_sync,
		            tear_down_oauth1_authorizer_interactive_data);
		g_test_add ("/oauth1-authorizer/request-authorization/sync/cancellation", OAuth1AuthorizerInteractiveData, NULL,
		            set_up_oauth1_authorizer_interactive_data, test_oauth1_authorizer_request_authorization_sync_cancellation,
		            tear_down_oauth1_authorizer_interactive_data);
	}

	g_test_add ("/oauth1-authorizer/request-authorization/sync/bad-credentials", OAuth1AuthorizerInteractiveData, NULL,
	            set_up_oauth1_authorizer_interactive_data_bad_credentials,
	            test_oauth1_authorizer_request_authorization_sync_bad_credentials, tear_down_oauth1_authorizer_interactive_data);

	/* Async request-authorization tests */
	if (gdata_test_interactive () == TRUE) {
		g_test_add ("/oauth1-authorizer/request-authorization/async", OAuth1AuthorizerInteractiveAsyncData, NULL,
		            set_up_oauth1_authorizer_interactive_async_data, test_oauth1_authorizer_request_authorization_async,
		            tear_down_oauth1_authorizer_interactive_async_data);
		g_test_add ("/oauth1-authorizer/request-authorization/async/cancellation", OAuth1AuthorizerInteractiveAsyncData, NULL,
		            set_up_oauth1_authorizer_interactive_async_data, test_oauth1_authorizer_request_authorization_async_cancellation,
		            tear_down_oauth1_authorizer_interactive_async_data);
	}

	g_test_add ("/oauth1-authorizer/request-authorization/async/bad-credentials", OAuth1AuthorizerInteractiveAsyncData, NULL,
	            set_up_oauth1_authorizer_interactive_async_data_bad_credentials,
	            test_oauth1_authorizer_request_authorization_async_bad_credentials, tear_down_oauth1_authorizer_interactive_async_data);

	/* Miscellaneous tests */
	if (gdata_test_interactive () == TRUE) {
		gboolean skip_test = FALSE;

		g_test_add ("/oauth1-authorizer/refresh-authorization/authenticated", OAuth1AuthorizerData, &skip_test,
		            set_up_oauth1_authorizer_data_authenticated, test_oauth1_authorizer_refresh_authorization,
		            tear_down_oauth1_authorizer_data);

		g_test_add ("/oauth1-authorizer/process-request/authenticated", OAuth1AuthorizerData, &skip_test,
		            set_up_oauth1_authorizer_data_authenticated, test_oauth1_authorizer_process_request_authenticated,
		            tear_down_oauth1_authorizer_data);
	}

	return g_test_run ();
}
