/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2011, 2014 <philip@tecnocode.co.uk>
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
static UhmServer *mock_server = NULL;

#undef CLIENT_ID  /* from common.h */

#define CLIENT_ID "352818697630-nqu2cmt5quqd6lr17ouoqmb684u84l1f.apps.googleusercontent.com"
#define CLIENT_SECRET "-fA4pHQJxR3zJ-FyAMPQsikg"
#define REDIRECT_URI "urn:ietf:wg:oauth:2.0:oob"

static void
test_oauth2_authorizer_constructor (void)
{
	GDataOAuth2Authorizer *authorizer;

	authorizer = gdata_oauth2_authorizer_new (CLIENT_ID, CLIENT_SECRET,
	                                          REDIRECT_URI,
	                                          GDATA_TYPE_TASKS_SERVICE);

	g_assert (authorizer != NULL);
	g_assert (GDATA_IS_OAUTH2_AUTHORIZER (authorizer));
	g_assert (GDATA_IS_AUTHORIZER (authorizer));

	g_object_unref (authorizer);
}

static void
test_oauth2_authorizer_constructor_for_domains (void)
{
	GDataOAuth2Authorizer *authorizer;
	GDataAuthorizationDomain *domain;
	GList *domains;

	/* Try with standard domains first */
	domains = gdata_service_get_authorization_domains (GDATA_TYPE_TASKS_SERVICE);
	authorizer = gdata_oauth2_authorizer_new_for_authorization_domains (CLIENT_ID, CLIENT_SECRET,
	                                                                    REDIRECT_URI, domains);
	g_list_free (domains);

	g_assert (authorizer != NULL);
	g_assert (GDATA_IS_OAUTH2_AUTHORIZER (authorizer));
	g_assert (GDATA_IS_AUTHORIZER (authorizer));

	g_object_unref (authorizer);

	/* Try again with a custom domain. Note that, as in test_authorization_domain_properties() this should not normally happen in client code. */
	domain = GDATA_AUTHORIZATION_DOMAIN (g_object_new (GDATA_TYPE_AUTHORIZATION_DOMAIN,
	                                                   "service-name", "test",
	                                                   "scope", "test",
	                                                   NULL));

	domains = g_list_prepend (NULL, domain);
	authorizer = gdata_oauth2_authorizer_new_for_authorization_domains (CLIENT_ID, CLIENT_SECRET,
	                                                                    REDIRECT_URI, domains);
	g_list_free (domains);

	g_assert (authorizer != NULL);
	g_assert (GDATA_IS_OAUTH2_AUTHORIZER (authorizer));
	g_assert (GDATA_IS_AUTHORIZER (authorizer));

	g_object_unref (authorizer);
	g_object_unref (domain);
}

typedef struct {
	GDataOAuth2Authorizer *authorizer;

	guint locale_notification_count;
	gulong locale_signal_handler;
	guint proxy_resolver_notification_count;
	gulong proxy_resolver_signal_handler;
	guint timeout_notification_count;
	gulong timeout_signal_handler;
} OAuth2AuthorizerData;

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
connect_to_oauth2_authorizer (OAuth2AuthorizerData *data)
{
	/* Connect to notifications from the object to verify they're only emitted the correct number of times */
	data->locale_signal_handler = g_signal_connect (data->authorizer, "notify::locale", (GCallback) notify_cb,
	                                                &(data->locale_notification_count));
	data->proxy_resolver_signal_handler = g_signal_connect (data->authorizer, "notify::proxy-resolver", (GCallback) notify_cb,
	                                                        &(data->proxy_resolver_notification_count));
	data->timeout_signal_handler = g_signal_connect (data->authorizer, "notify::timeout", (GCallback) notify_cb,
	                                                 &(data->timeout_notification_count));
}

static void
set_up_oauth2_authorizer_data (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	data->authorizer = gdata_oauth2_authorizer_new (CLIENT_ID,
	                                                CLIENT_SECRET,
	                                                REDIRECT_URI,
	                                                GDATA_TYPE_TASKS_SERVICE);
	connect_to_oauth2_authorizer (data);
}

static void
set_up_oauth2_authorizer_data_multiple_domains (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	GList *authorization_domains = NULL;

	authorization_domains = g_list_prepend (authorization_domains, gdata_tasks_service_get_primary_authorization_domain ());
	authorization_domains = g_list_prepend (authorization_domains, gdata_picasaweb_service_get_primary_authorization_domain ());
	data->authorizer = gdata_oauth2_authorizer_new_for_authorization_domains (CLIENT_ID, CLIENT_SECRET,
	                                                                          REDIRECT_URI, authorization_domains);
	g_list_free (authorization_domains);

	connect_to_oauth2_authorizer (data);
}

static void
set_up_oauth2_authorizer_data_locale (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	data->authorizer = gdata_oauth2_authorizer_new (CLIENT_ID,
	                                                CLIENT_SECRET,
	                                                REDIRECT_URI,
	                                                GDATA_TYPE_TASKS_SERVICE);
	gdata_oauth2_authorizer_set_locale (data->authorizer, "en_GB");
	connect_to_oauth2_authorizer (data);
}

static void
set_up_oauth2_authorizer_data_authenticated (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	gboolean *skip_test = (gboolean*) user_data;
	gchar *authentication_uri, *authorisation_code;

	gdata_test_mock_server_start_trace (mock_server, "setup-oauth2-authorizer-data-authenticated");

	/* Chain up */
	set_up_oauth2_authorizer_data (data, NULL);

	/* Get an authentication URI. */
	authentication_uri = gdata_oauth2_authorizer_build_authentication_uri (data->authorizer, NULL, FALSE);
	g_assert (authentication_uri != NULL);

	/* Get the authorisation code off the user. */
	if (uhm_server_get_enable_online (mock_server)) {
		authorisation_code = gdata_test_query_user_for_verifier (authentication_uri);

		g_free (authentication_uri);

		if (authorisation_code == NULL) {
			*skip_test = TRUE;
			goto skip_test;
		}
	} else {
		/* Hard-coded default to match the trace file. */
		authorisation_code = g_strdup ("4/GeYb_3HkYh4vyephp-lbvzQs1GAb.YtXAxmx-uJ0eoiIBeO6P2m9iH6kvkQI");
		g_free (authentication_uri);
	}

	/* Authorise the token. */
	g_assert (gdata_oauth2_authorizer_request_authorization (data->authorizer, authorisation_code, NULL, NULL) == TRUE);

skip_test:
	g_free (authorisation_code);

	uhm_server_end_trace (mock_server);
}

static void
tear_down_oauth2_authorizer_data (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	/* Clean up signal handlers */
	g_signal_handler_disconnect (data->authorizer, data->timeout_signal_handler);
	g_signal_handler_disconnect (data->authorizer, data->proxy_resolver_signal_handler);
	g_signal_handler_disconnect (data->authorizer, data->locale_signal_handler);

	g_object_unref (data->authorizer);
}

/* Test getting and setting the client-id property */
static void
test_oauth2_authorizer_properties_client_id (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	gchar *client_id;

	/* Verifying the normal state of the property in a newly-constructed instance of GDataOAuth2Authorizer */
	g_assert_cmpstr (gdata_oauth2_authorizer_get_client_id (data->authorizer), ==, CLIENT_ID);

	g_object_get (data->authorizer, "client-id", &client_id, NULL);
	g_assert_cmpstr (client_id, ==, CLIENT_ID);
	g_free (client_id);
}

/* Test getting and setting the client-id property */
static void
test_oauth2_authorizer_properties_client_secret (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	gchar *client_secret;

	/* Verifying the normal state of the property in a newly-constructed instance of GDataOAuth2Authorizer */
	g_assert_cmpstr (gdata_oauth2_authorizer_get_client_secret (data->authorizer), ==, CLIENT_SECRET);

	g_object_get (data->authorizer, "client-secret", &client_secret, NULL);
	g_assert_cmpstr (client_secret, ==, CLIENT_SECRET);
	g_free (client_secret);
}

/* Test getting and setting the client-id property */
static void
test_oauth2_authorizer_properties_redirect_uri (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	gchar *redirect_uri;

	/* Verifying the normal state of the property in a newly-constructed instance of GDataOAuth2Authorizer */
	g_assert_cmpstr (gdata_oauth2_authorizer_get_redirect_uri (data->authorizer), ==, REDIRECT_URI);

	g_object_get (data->authorizer, "redirect-uri", &redirect_uri, NULL);
	g_assert_cmpstr (redirect_uri, ==, REDIRECT_URI);
	g_free (redirect_uri);
}

/* Test getting and setting the locale property */
static void
test_oauth2_authorizer_properties_locale (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	gchar *locale;

	/* Verifying the normal state of the property in a newly-constructed instance of GDataOAuth2Authorizer */
	g_assert_cmpstr (gdata_oauth2_authorizer_get_locale (data->authorizer), ==, NULL);

	g_object_get (data->authorizer, "locale", &locale, NULL);
	g_assert_cmpstr (locale, ==, NULL);
	g_free (locale);

	g_assert_cmpuint (data->locale_notification_count, ==, 0);

	/* Check setting it works and emits a notification */
	gdata_oauth2_authorizer_set_locale (data->authorizer, "en");

	g_assert_cmpuint (data->locale_notification_count, ==, 1);

	g_assert_cmpstr (gdata_oauth2_authorizer_get_locale (data->authorizer), ==, "en");

	g_object_get (data->authorizer, "locale", &locale, NULL);
	g_assert_cmpstr (locale, ==, "en");
	g_free (locale);

	/* Check setting it to the same value is a no-op */
	gdata_oauth2_authorizer_set_locale (data->authorizer, "en");
	g_assert_cmpuint (data->locale_notification_count, ==, 1);

	/* Check setting it back to NULL works */
	gdata_oauth2_authorizer_set_locale (data->authorizer, NULL);

	g_assert_cmpuint (data->locale_notification_count, ==, 2);

	g_assert_cmpstr (gdata_oauth2_authorizer_get_locale (data->authorizer), ==, NULL);

	g_object_get (data->authorizer, "locale", &locale, NULL);
	g_assert_cmpstr (locale, ==, NULL);
	g_free (locale);

	/* Test that setting it using g_object_set() works */
	g_object_set (data->authorizer, "locale", "de", NULL);
	g_assert_cmpstr (gdata_oauth2_authorizer_get_locale (data->authorizer), ==, "de");
}

/* Test getting and setting the timeout property */
static void
test_oauth2_authorizer_properties_timeout (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	guint timeout;

	/* Verifying the normal state of the property in a newly-constructed instance of GDataOAuth2Authorizer */
	g_assert_cmpuint (gdata_oauth2_authorizer_get_timeout (data->authorizer), ==, 0);

	g_object_get (data->authorizer, "timeout", &timeout, NULL);
	g_assert_cmpuint (timeout, ==, 0);

	g_assert_cmpuint (data->timeout_notification_count, ==, 0);

	/* Check setting it works and emits a notification */
	gdata_oauth2_authorizer_set_timeout (data->authorizer, 30);

	g_assert_cmpuint (data->timeout_notification_count, ==, 1);

	g_assert_cmpuint (gdata_oauth2_authorizer_get_timeout (data->authorizer), ==, 30);

	g_object_get (data->authorizer, "timeout", &timeout, NULL);
	g_assert_cmpuint (timeout, ==, 30);

	/* Check setting it to the same value is a no-op */
	gdata_oauth2_authorizer_set_timeout (data->authorizer, 30);
	g_assert_cmpuint (data->timeout_notification_count, ==, 1);

	/* Check setting it back to 0 works */
	gdata_oauth2_authorizer_set_timeout (data->authorizer, 0);

	g_assert_cmpuint (data->timeout_notification_count, ==, 2);

	g_assert_cmpuint (gdata_oauth2_authorizer_get_timeout (data->authorizer), ==, 0);

	g_object_get (data->authorizer, "timeout", &timeout, NULL);
	g_assert_cmpuint (timeout, ==, 0);

	/* Test that setting it using g_object_set() works */
	g_object_set (data->authorizer, "timeout", 15, NULL);
	g_assert_cmpuint (gdata_oauth2_authorizer_get_timeout (data->authorizer), ==, 15);
}

/* Test getting and setting the proxy-resolver property */
static void
test_oauth2_authorizer_properties_proxy_resolver (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	GProxyResolver *old_proxy_resolver, *proxy_resolver, *new_proxy_resolver;

	/* Verifying the normal state of the property in a newly-constructed instance of GDataOAuth2Authorizer.
	 * Since the resolver comes from the SoupSession, we don’t know whether it’s initially NULL. */
	old_proxy_resolver = gdata_oauth2_authorizer_get_proxy_resolver (data->authorizer);

	g_object_get (data->authorizer, "proxy-resolver", &proxy_resolver, NULL);
	g_assert (proxy_resolver == old_proxy_resolver);

	g_assert_cmpuint (data->proxy_resolver_notification_count, ==, 0);

	/* Check setting it works and emits a notification */
	new_proxy_resolver = g_object_ref (g_proxy_resolver_get_default ());
	gdata_oauth2_authorizer_set_proxy_resolver (data->authorizer, new_proxy_resolver);

	g_assert_cmpuint (data->proxy_resolver_notification_count, ==, 1);

	g_assert (gdata_oauth2_authorizer_get_proxy_resolver (data->authorizer) != NULL);
	g_assert (gdata_oauth2_authorizer_get_proxy_resolver (data->authorizer) == new_proxy_resolver);

	g_object_get (data->authorizer, "proxy-resolver", &proxy_resolver, NULL);
	g_assert (proxy_resolver != NULL);
	g_assert (gdata_oauth2_authorizer_get_proxy_resolver (data->authorizer) == new_proxy_resolver);
	g_object_unref (proxy_resolver);

	g_object_unref (new_proxy_resolver);

	/* Check setting it back to NULL works */
	gdata_oauth2_authorizer_set_proxy_resolver (data->authorizer, NULL);

	g_assert_cmpuint (data->proxy_resolver_notification_count, ==, 2);

	g_assert (gdata_oauth2_authorizer_get_proxy_resolver (data->authorizer) == NULL);

	g_object_get (data->authorizer, "proxy-resolver", &proxy_resolver, NULL);
	g_assert (proxy_resolver == NULL);

	/* Test that setting it using g_object_set() works */
	new_proxy_resolver = g_object_ref (g_proxy_resolver_get_default ());
	g_object_set (data->authorizer, "proxy-resolver", new_proxy_resolver, NULL);
	g_object_unref (new_proxy_resolver);

	g_assert (gdata_oauth2_authorizer_get_proxy_resolver (data->authorizer) != NULL);
}

/* Test that gdata_authorizer_refresh_authorization() is a no-op when
 * unauthenticated. */
static void
test_oauth2_authorizer_refresh_authorization_unauthenticated (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	gboolean *skip_test = (gboolean*) user_data;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "oauth2-authorizer-refresh-authorization-unauthorized");

	/* Skip the test if the user's requested */
	if (skip_test != NULL && *skip_test == TRUE) {
		return;
	}

	g_assert (gdata_authorizer_refresh_authorization (GDATA_AUTHORIZER (data->authorizer), NULL, &error) == FALSE);
	g_assert_no_error (error);
	g_clear_error (&error);

	uhm_server_end_trace (mock_server);
}

/* Test that gdata_authorizer_refresh_authorization() works when authenticated. */
static void
test_oauth2_authorizer_refresh_authorization_authenticated (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	gboolean *skip_test = (gboolean*) user_data;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "oauth2-authorizer-refresh-authorization-authorized");

	/* Skip the test if the user's requested */
	if (skip_test != NULL && *skip_test == TRUE) {
		return;
	}

	g_assert (gdata_authorizer_refresh_authorization (GDATA_AUTHORIZER (data->authorizer), NULL, &error) == TRUE);
	g_assert_no_error (error);
	g_clear_error (&error);

	uhm_server_end_trace (mock_server);
}

/* Test that processing a request with a NULL domain will not change the request. */
static void
test_oauth2_authorizer_process_request_null (OAuth2AuthorizerData *data, gconstpointer user_data)
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
test_oauth2_authorizer_process_request_unauthenticated (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	SoupMessage *message;
	SoupMessageHeadersIter iter;
	guint header_count = 0;
	const gchar *name, *value;

	/* Create a new message with an empty set of request headers */
	message = soup_message_new (SOUP_METHOD_GET, "https://example.com/");

	/* Process the message */
	gdata_authorizer_process_request (GDATA_AUTHORIZER (data->authorizer), gdata_tasks_service_get_primary_authorization_domain (), message);

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
test_oauth2_authorizer_process_request_authenticated (OAuth2AuthorizerData *data, gconstpointer user_data)
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
	message = soup_message_new (SOUP_METHOD_GET, "https://example.com/");

	/* Process the message */
	gdata_authorizer_process_request (GDATA_AUTHORIZER (data->authorizer), gdata_tasks_service_get_primary_authorization_domain (), message);

	/* Check that at least one new header has been set */
	soup_message_headers_iter_init (&iter, message->request_headers);

	while (soup_message_headers_iter_next (&iter, &name, &value) == TRUE) {
		header_count++;
	}

	g_assert_cmpuint (header_count, >, 0);

	g_object_unref (message);
}

/* Test that building an authentication URI works correctly */
static void
test_oauth2_authorizer_build_authentication_uri_default (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	gchar *authentication_uri;

	authentication_uri = gdata_oauth2_authorizer_build_authentication_uri (data->authorizer, NULL, FALSE);
	g_assert (authentication_uri != NULL && *authentication_uri != '\0');

	g_test_message ("Building an authentication URI gave “%s”.",
	                authentication_uri);

	g_free (authentication_uri);
}

/* Test that building an authentication URI with a login hint works
 * correctly. */
static void
test_oauth2_authorizer_build_authentication_uri_hint (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	gchar *authentication_uri;

	authentication_uri = gdata_oauth2_authorizer_build_authentication_uri (data->authorizer, "test.user@gmail.com", FALSE);
	g_assert (authentication_uri != NULL && *authentication_uri != '\0');

	g_test_message ("Building an authentication URI gave “%s”.",
	                authentication_uri);

	g_free (authentication_uri);
}

/* Test that buildig an authentication URI with a login hint and incremental
 * authentication works correctly. */
static void
test_oauth2_authorizer_build_authentication_uri_incremental (OAuth2AuthorizerData *data, gconstpointer user_data)
{
	gchar *authentication_uri;

	authentication_uri = gdata_oauth2_authorizer_build_authentication_uri (data->authorizer, "test.user@gmail.com", TRUE);
	g_assert (authentication_uri != NULL && *authentication_uri != '\0');

	g_test_message ("Building an authentication URI gave “%s”.",
	                authentication_uri);

	g_free (authentication_uri);
}

typedef struct {
	OAuth2AuthorizerData parent;
	gchar *authorisation_code;
} OAuth2AuthorizerInteractiveData;

/* NOTE: Any consumer of this data has to check for (data->authorisation_code == NULL) and skip the test in that case */
static void
set_up_oauth2_authorizer_interactive_data (OAuth2AuthorizerInteractiveData *data, gconstpointer user_data)
{
	gchar *authentication_uri;

	/* Chain up */
	set_up_oauth2_authorizer_data ((OAuth2AuthorizerData*) data, user_data);

	/* Get an authentication URI */
	authentication_uri = gdata_oauth2_authorizer_build_authentication_uri (data->parent.authorizer, NULL, FALSE);
	g_assert (authentication_uri != NULL);

	/* Wait for the user to retrieve and enter the authorisation code. */
	if (uhm_server_get_enable_online (mock_server)) {
		data->authorisation_code = gdata_test_query_user_for_verifier (authentication_uri);
	} else {
		/* Hard-coded default to match the trace file. */
		data->authorisation_code = g_strdup ((const gchar *) user_data);
	}

	g_free (authentication_uri);
}

static void
set_up_oauth2_authorizer_interactive_data_bad_credentials (OAuth2AuthorizerInteractiveData *data, gconstpointer user_data)
{
	gchar *authentication_uri;

	/* Chain up */
	set_up_oauth2_authorizer_data ((OAuth2AuthorizerData*) data, user_data);

	/* Get an authentication URI */
	authentication_uri = gdata_oauth2_authorizer_build_authentication_uri (data->parent.authorizer, NULL, FALSE);
	g_assert (authentication_uri != NULL);

	/* Give a bogus authorisation code. */
	data->authorisation_code = g_strdup ("test");

	g_free (authentication_uri);
}

static void
tear_down_oauth2_authorizer_interactive_data (OAuth2AuthorizerInteractiveData *data, gconstpointer user_data)
{
	g_free (data->authorisation_code);

	/* Chain up */
	tear_down_oauth2_authorizer_data ((OAuth2AuthorizerData*) data, user_data);
}

/* Test that synchronously authorizing an authorisation code is successful. Note
 * that this test has to be interactive, as the user has to visit the
 * authentication URI to retrieve an authorisation code. */
static void
test_oauth2_authorizer_request_authorization_sync (OAuth2AuthorizerInteractiveData *data, gconstpointer user_data)
{
	gboolean success;
	GError *error = NULL;

	/* Skip the test if the user's requested */
	if (data->authorisation_code == NULL) {
		return;
	}

	gdata_test_mock_server_start_trace (mock_server, "oauth2-authorizer-request-authorization-sync");

	/* Check we're not authorised beforehand */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.authorizer),
	          gdata_tasks_service_get_primary_authorization_domain ()) == FALSE);

	/* Test that authorising the token retrieved previously is successful */
	success = gdata_oauth2_authorizer_request_authorization (data->parent.authorizer, data->authorisation_code, NULL,
	                                                         &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);
	g_clear_error (&error);

	/* Are we authorised now? */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.authorizer),
	          gdata_tasks_service_get_primary_authorization_domain ()) == TRUE);

	uhm_server_end_trace (mock_server);
}

/* Test that synchronously authorizing fails if an invalid authorisation code is
 * provided. */
static void
test_oauth2_authorizer_request_authorization_sync_bad_credentials (OAuth2AuthorizerInteractiveData *data, gconstpointer user_data)
{
	gboolean success;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "oauth2-authorizer-request-authorization-sync-bad-credentials");

	/* Check we're not authorised beforehand */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.authorizer),
	          gdata_tasks_service_get_primary_authorization_domain ()) == FALSE);

	/* Test that authorising the token retrieved above fails */
	success = gdata_oauth2_authorizer_request_authorization (data->parent.authorizer, data->authorisation_code, NULL,
	                                                         &error);
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_FORBIDDEN);
	g_assert (success == FALSE);
	g_clear_error (&error);

	/* Are we authorised now? */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.authorizer),
	          gdata_tasks_service_get_primary_authorization_domain ()) == FALSE);

	uhm_server_end_trace (mock_server);
}

/* Test that cancellation of synchronously authorizing works. Note that this
 * test has to be interactive, as the user has to visit the authentication URI
 * to retrieve an authorisation code. */
static void
test_oauth2_authorizer_request_authorization_sync_cancellation (OAuth2AuthorizerInteractiveData *data, gconstpointer user_data)
{
	gboolean success;
	GCancellable *cancellable;
	GError *error = NULL;

	/* Skip the test if the user's requested */
	if (data->authorisation_code == NULL) {
		return;
	}

	gdata_test_mock_server_start_trace (mock_server, "oauth2-authorizer-request-authorization-sync-cancellation");

	/* Check we're not authorised beforehand */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.authorizer),
	          gdata_tasks_service_get_primary_authorization_domain ()) == FALSE);

	/* Create the cancellable */
	cancellable = g_cancellable_new ();

	/* Test that authorising the code retrieved above is unsuccessful */
	g_cancellable_cancel (cancellable);
	success = gdata_oauth2_authorizer_request_authorization (data->parent.authorizer, data->authorisation_code,
	                                                         cancellable, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_assert (success == FALSE);
	g_clear_error (&error);

	/* Are we authorised now? */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.authorizer),
	          gdata_tasks_service_get_primary_authorization_domain ()) == FALSE);

	g_object_unref (cancellable);

	uhm_server_end_trace (mock_server);
}

typedef struct {
	OAuth2AuthorizerInteractiveData parent;
	GMainLoop *main_loop;
} OAuth2AuthorizerInteractiveAsyncData;

/* NOTE: Any consumer of this data has to check for
 * (data->authorisation_code == NULL) and skip the test in that case */
static void
set_up_oauth2_authorizer_interactive_async_data (OAuth2AuthorizerInteractiveAsyncData *data, gconstpointer user_data)
{
	/* Chain up */
	set_up_oauth2_authorizer_interactive_data ((OAuth2AuthorizerInteractiveData*) data, user_data);

	/* Set up the main loop */
	data->main_loop = g_main_loop_new (NULL, FALSE);
}

static void
set_up_oauth2_authorizer_interactive_async_data_bad_credentials (OAuth2AuthorizerInteractiveAsyncData *data, gconstpointer user_data)
{
	/* Chain up */
	set_up_oauth2_authorizer_interactive_data_bad_credentials ((OAuth2AuthorizerInteractiveData*) data, user_data);

	/* Set up the main loop */
	data->main_loop = g_main_loop_new (NULL, FALSE);
}

static void
tear_down_oauth2_authorizer_interactive_async_data (OAuth2AuthorizerInteractiveAsyncData *data, gconstpointer user_data)
{
	g_main_loop_unref (data->main_loop);

	/* Chain up */
	tear_down_oauth2_authorizer_interactive_data ((OAuth2AuthorizerInteractiveData*) data, user_data);
}

static void
test_oauth2_authorizer_request_authorization_async_cb (GDataOAuth2Authorizer *authorizer, GAsyncResult *async_result,
                                                       OAuth2AuthorizerInteractiveAsyncData *data)
{
	gboolean success;
	GError *error = NULL;

	success = gdata_oauth2_authorizer_request_authorization_finish (authorizer, async_result, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);
	g_clear_error (&error);

	/* Are we authorised now? */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	          gdata_tasks_service_get_primary_authorization_domain ()) == TRUE);

	g_main_loop_quit (data->main_loop);
}

/* Test that asynchronously authorizing works. Note that this test has to be
 * interactive, as the user has to visit the authentication URI to retrieve an
 * authorisation code. */
static void
test_oauth2_authorizer_request_authorization_async (OAuth2AuthorizerInteractiveAsyncData *data, gconstpointer user_data)
{
	/* Skip the test if the user's requested */
	if (data->parent.authorisation_code == NULL) {
		return;
	}

	gdata_test_mock_server_start_trace (mock_server, "oauth2-authorizer-request-authorization-async");

	/* Check we're not authorised beforehand */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.parent.authorizer),
	          gdata_tasks_service_get_primary_authorization_domain ()) == FALSE);

	/* Create a main loop and request authorization */
	gdata_oauth2_authorizer_request_authorization_async (data->parent.parent.authorizer,
	                                                     data->parent.authorisation_code,
	                                                     NULL,
	                                                     (GAsyncReadyCallback) test_oauth2_authorizer_request_authorization_async_cb, data);

	g_main_loop_run (data->main_loop);

	uhm_server_end_trace (mock_server);
}

static void
test_oauth2_authorizer_request_authorization_async_bad_credentials_cb (GDataOAuth2Authorizer *authorizer, GAsyncResult *async_result,
                                                                       OAuth2AuthorizerInteractiveAsyncData *data)
{
	gboolean success;
	GError *error = NULL;

	success = gdata_oauth2_authorizer_request_authorization_finish (authorizer, async_result, &error);
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_FORBIDDEN);
	g_assert (success == FALSE);
	g_clear_error (&error);

	/* Are we authorised now? */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	          gdata_tasks_service_get_primary_authorization_domain ()) == FALSE);

	g_main_loop_quit (data->main_loop);
}

/* Test that asynchronously authorizing fails if an invalid authorisation code
 * is provided. */
static void
test_oauth2_authorizer_request_authorization_async_bad_credentials (OAuth2AuthorizerInteractiveAsyncData *data, gconstpointer user_data)
{
	gdata_test_mock_server_start_trace (mock_server, "oauth2-authorizer-request-authorization-async-bad-credentials");

	/* Check we're not authorised beforehand */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.parent.authorizer),
	          gdata_tasks_service_get_primary_authorization_domain ()) == FALSE);

	/* Create a main loop and request authorization */
	gdata_oauth2_authorizer_request_authorization_async (data->parent.parent.authorizer,
	                                                     data->parent.authorisation_code,
	                                                     NULL,
	                                                     (GAsyncReadyCallback) test_oauth2_authorizer_request_authorization_async_bad_credentials_cb,
	                                                     data);

	g_main_loop_run (data->main_loop);

	uhm_server_end_trace (mock_server);
}

static void
test_oauth2_authorizer_request_authorization_async_cancellation_cb (GDataOAuth2Authorizer *authorizer, GAsyncResult *async_result,
                                                                    OAuth2AuthorizerInteractiveAsyncData *data)
{
	gboolean success;
	GError *error = NULL;

	success = gdata_oauth2_authorizer_request_authorization_finish (authorizer, async_result, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_assert (success == FALSE);
	g_clear_error (&error);

	/* Are we authorised now? */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	          gdata_tasks_service_get_primary_authorization_domain ()) == FALSE);

	g_main_loop_quit (data->main_loop);
}

/* Test that cancelling asynchronously authorizing works. Note that this test
 * has to be interactive, as the user has to visit the authentication URI to
 * retrieve an authorisation code. */
static void
test_oauth2_authorizer_request_authorization_async_cancellation (OAuth2AuthorizerInteractiveAsyncData *data, gconstpointer user_data)
{
	GCancellable *cancellable;

	/* Skip the test if the user's requested */
	if (data->parent.authorisation_code == NULL) {
		return;
	}

	gdata_test_mock_server_start_trace (mock_server, "oauth2-authorizer-request-authorization-async-cancellation");

	/* Check we're not authorised beforehand */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (data->parent.parent.authorizer),
	          gdata_tasks_service_get_primary_authorization_domain ()) == FALSE);

	/* Create the cancellable */
	cancellable = g_cancellable_new ();

	/* Create a main loop and request authorization */
	gdata_oauth2_authorizer_request_authorization_async (data->parent.parent.authorizer,
	                                                     data->parent.authorisation_code,
	                                                     cancellable,
	                                                     (GAsyncReadyCallback) test_oauth2_authorizer_request_authorization_async_cancellation_cb,
	                                                     data);
	g_cancellable_cancel (cancellable);

	g_main_loop_run (data->main_loop);

	g_object_unref (cancellable);

	uhm_server_end_trace (mock_server);
}

static void
mock_server_notify_resolver_cb (GObject *object, GParamSpec *pspec,
                                gpointer user_data)
{
	UhmServer *server;
	UhmResolver *resolver;

	server = UHM_SERVER (object);

	/* Set up the expected domain names here. This should technically be
	 * split up between the different unit test suites, but that’s too much
	 * effort. */
	resolver = uhm_server_get_resolver (server);

	if (resolver != NULL) {
		const gchar *ip_address = uhm_server_get_address (server);

		uhm_resolver_add_A (resolver, "www.google.com", ip_address);
		uhm_resolver_add_A (resolver,
		                    "accounts.google.com", ip_address);
	}
}

int
main (int argc, char *argv[])
{
	GFile *trace_directory;
	gchar *path = NULL;

	gdata_test_init (argc, argv);

	mock_server = gdata_test_get_mock_server ();
	g_signal_connect (G_OBJECT (mock_server), "notify::resolver",
	                  (GCallback) mock_server_notify_resolver_cb, NULL);
	path = g_test_build_filename (G_TEST_DIST, "traces/oauth2-authorizer", NULL);
	trace_directory = g_file_new_for_path (path);
	g_free (path);
	uhm_server_set_trace_directory (mock_server, trace_directory);
	g_object_unref (trace_directory);

	main_thread = g_thread_self ();

	g_test_add_func ("/oauth2-authorizer/constructor", test_oauth2_authorizer_constructor);
	g_test_add_func ("/oauth2-authorizer/constructor/for-domains", test_oauth2_authorizer_constructor_for_domains);

	g_test_add ("/oauth2-authorizer/properties/client-id", OAuth2AuthorizerData, NULL, set_up_oauth2_authorizer_data,
	            test_oauth2_authorizer_properties_client_id, tear_down_oauth2_authorizer_data);
	g_test_add ("/oauth2-authorizer/properties/client-secret", OAuth2AuthorizerData, NULL, set_up_oauth2_authorizer_data,
	            test_oauth2_authorizer_properties_client_secret, tear_down_oauth2_authorizer_data);
	g_test_add ("/oauth2-authorizer/properties/redirect-uri", OAuth2AuthorizerData, NULL, set_up_oauth2_authorizer_data,
	            test_oauth2_authorizer_properties_redirect_uri, tear_down_oauth2_authorizer_data);
	g_test_add ("/oauth2-authorizer/properties/locale", OAuth2AuthorizerData, NULL, set_up_oauth2_authorizer_data,
	            test_oauth2_authorizer_properties_locale, tear_down_oauth2_authorizer_data);
	g_test_add ("/oauth2-authorizer/properties/timeout", OAuth2AuthorizerData, NULL, set_up_oauth2_authorizer_data,
	            test_oauth2_authorizer_properties_timeout, tear_down_oauth2_authorizer_data);
	g_test_add ("/oauth2-authorizer/properties/proxy-resolver", OAuth2AuthorizerData, NULL, set_up_oauth2_authorizer_data,
	            test_oauth2_authorizer_properties_proxy_resolver, tear_down_oauth2_authorizer_data);

	g_test_add ("/oauth2-authorizer/refresh-authorization/unauthenticated", OAuth2AuthorizerData, NULL,
	            set_up_oauth2_authorizer_data, test_oauth2_authorizer_refresh_authorization_unauthenticated, tear_down_oauth2_authorizer_data);

	g_test_add ("/oauth2-authorizer/process-request/null", OAuth2AuthorizerData, NULL,
	            set_up_oauth2_authorizer_data, test_oauth2_authorizer_process_request_null, tear_down_oauth2_authorizer_data);
	g_test_add ("/oauth2-authorizer/process-request/unauthenticated", OAuth2AuthorizerData, NULL,
	            set_up_oauth2_authorizer_data, test_oauth2_authorizer_process_request_unauthenticated, tear_down_oauth2_authorizer_data);

	/* build-authentication-uri tests */
	g_test_add ("/oauth2-authorizer/build-authentication-uri", OAuth2AuthorizerData, NULL, set_up_oauth2_authorizer_data,
	            test_oauth2_authorizer_build_authentication_uri_default, tear_down_oauth2_authorizer_data);
	g_test_add ("/oauth2-authorizer/build-authentication-uri/multiple-domains", OAuth2AuthorizerData, NULL,
	            set_up_oauth2_authorizer_data_multiple_domains, test_oauth2_authorizer_build_authentication_uri_default,
	            tear_down_oauth2_authorizer_data);
	g_test_add ("/oauth2-authorizer/build-authentication-uri/locale", OAuth2AuthorizerData, NULL,
	            set_up_oauth2_authorizer_data_locale, test_oauth2_authorizer_build_authentication_uri_default,
	            tear_down_oauth2_authorizer_data);
	g_test_add ("/oauth2-authorizer/build-authentication-uri/hint", OAuth2AuthorizerData, NULL,
	            set_up_oauth2_authorizer_data, test_oauth2_authorizer_build_authentication_uri_hint,
	            tear_down_oauth2_authorizer_data);
	g_test_add ("/oauth2-authorizer/build-authentication-uri/incremental", OAuth2AuthorizerData, NULL,
	            set_up_oauth2_authorizer_data, test_oauth2_authorizer_build_authentication_uri_incremental,
	            tear_down_oauth2_authorizer_data);

	/* Sync request-authorization tests */
	if (gdata_test_interactive () == TRUE) {
		g_test_add ("/oauth2-authorizer/request-authorization/sync", OAuth2AuthorizerInteractiveData,
		            "4/P-pwMETnCh47w20wexdnflDFhXum.4qZ2A1pkUGsSoiIBeO6P2m8OUKkvkQI",
		            set_up_oauth2_authorizer_interactive_data, test_oauth2_authorizer_request_authorization_sync,
		            tear_down_oauth2_authorizer_interactive_data);
		g_test_add ("/oauth2-authorizer/request-authorization/sync/cancellation", OAuth2AuthorizerInteractiveData,
		            "4/P-pwMETnCh47w20wexdnflDFhXum.4qZ2A1pkUGsSoiIBeO6P2m8OUKkvkQI",
		            set_up_oauth2_authorizer_interactive_data, test_oauth2_authorizer_request_authorization_sync_cancellation,
		            tear_down_oauth2_authorizer_interactive_data);
	}

	g_test_add ("/oauth2-authorizer/request-authorization/sync/bad-credentials", OAuth2AuthorizerInteractiveData, "",
	            set_up_oauth2_authorizer_interactive_data_bad_credentials,
	            test_oauth2_authorizer_request_authorization_sync_bad_credentials, tear_down_oauth2_authorizer_interactive_data);

	/* Async request-authorization tests */
	if (gdata_test_interactive () == TRUE) {
		g_test_add ("/oauth2-authorizer/request-authorization/async", OAuth2AuthorizerInteractiveAsyncData,
		            "4/Gfha9-4IeN09ibTR2Sa2MtQrG9qz.ks8v0zlKR9ceoiIBeO6P2m92f6kvkQI",
		            set_up_oauth2_authorizer_interactive_async_data, test_oauth2_authorizer_request_authorization_async,
		            tear_down_oauth2_authorizer_interactive_async_data);
		g_test_add ("/oauth2-authorizer/request-authorization/async/cancellation", OAuth2AuthorizerInteractiveAsyncData,
		            "4/Gfha9-4IeN09ibTR2Sa2MtQrG9qz.ks8v0zlKR9ceoiIBeO6P2m92f6kvkQI",
		            set_up_oauth2_authorizer_interactive_async_data, test_oauth2_authorizer_request_authorization_async_cancellation,
		            tear_down_oauth2_authorizer_interactive_async_data);
	}

	g_test_add ("/oauth2-authorizer/request-authorization/async/bad-credentials", OAuth2AuthorizerInteractiveAsyncData, "",
	            set_up_oauth2_authorizer_interactive_async_data_bad_credentials,
	            test_oauth2_authorizer_request_authorization_async_bad_credentials, tear_down_oauth2_authorizer_interactive_async_data);

	/* Miscellaneous tests */
	if (gdata_test_interactive () == TRUE) {
		gboolean skip_test = FALSE;

		g_test_add ("/oauth2-authorizer/refresh-authorization/authenticated", OAuth2AuthorizerData, &skip_test,
		            set_up_oauth2_authorizer_data_authenticated, test_oauth2_authorizer_refresh_authorization_authenticated,
		            tear_down_oauth2_authorizer_data);

		g_test_add ("/oauth2-authorizer/process-request/authenticated", OAuth2AuthorizerData, &skip_test,
		            set_up_oauth2_authorizer_data_authenticated, test_oauth2_authorizer_process_request_authenticated,
		            tear_down_oauth2_authorizer_data);
	}

	return g_test_run ();
}
