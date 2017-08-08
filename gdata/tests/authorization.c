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

#include <string.h>
#include <glib.h>
#include <gdata/gdata.h>

#include "common.h"

/* Used as common "testing domains" to simplify test code. */
static GDataAuthorizationDomain *test_domain1 = NULL;
static GDataAuthorizationDomain *test_domain2 = NULL;

static void
test_authorization_domain_properties (void)
{
	GDataAuthorizationDomain *domain;
	gchar *service_name, *scope;

	/* NOTE: It's not expected that client code will normally get hold of GDataAuthorizationDomain instances this way.
	 * This is just for testing purposes. */
	domain = GDATA_AUTHORIZATION_DOMAIN (g_object_new (GDATA_TYPE_AUTHORIZATION_DOMAIN,
	                                                   "service-name", "service-name",
	                                                   "scope", "scope",
	                                                   NULL));

	g_assert_cmpstr (gdata_authorization_domain_get_service_name (domain), ==, "service-name");
	g_assert_cmpstr (gdata_authorization_domain_get_scope (domain), ==, "scope");

	g_object_get (domain,
	              "service-name", &service_name,
	              "scope", &scope,
	              NULL);

	g_assert_cmpstr (service_name, ==, "service-name");
	g_assert_cmpstr (scope, ==, "scope");

	g_free (service_name);
	g_free (scope);
}

/* Simple implementation of GDataAuthorizer for test purposes */
#define TYPE_SIMPLE_AUTHORIZER		(simple_authorizer_get_type ())
#define SIMPLE_AUTHORIZER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), SIMPLE_TYPE_AUTHORIZER, SimpleAuthorizer))
#define SIMPLE_AUTHORIZER_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), SIMPLE_TYPE_AUTHORIZER, SimpleAuthorizerClass))
#define IS_SIMPLE_AUTHORIZER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), SIMPLE_TYPE_AUTHORIZER))
#define IS_SIMPLE_AUTHORIZER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), SIMPLE_TYPE_AUTHORIZER))
#define SIMPLE_AUTHORIZER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), SIMPLE_TYPE_AUTHORIZER, SimpleAuthorizerClass))

typedef struct {
	GObject parent;
} SimpleAuthorizer;

typedef struct {
	GObjectClass parent;
} SimpleAuthorizerClass;

static GType simple_authorizer_get_type (void) G_GNUC_CONST;
static void simple_authorizer_authorizer_init (GDataAuthorizerInterface *iface);

G_DEFINE_TYPE_WITH_CODE (SimpleAuthorizer, simple_authorizer, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_AUTHORIZER, simple_authorizer_authorizer_init))

static void
simple_authorizer_class_init (SimpleAuthorizerClass *klass)
{
	/* Nothing to see here */
}

static void
simple_authorizer_init (SimpleAuthorizer *self)
{
	/* Nothing to see here */
}

static void
simple_authorizer_process_request (GDataAuthorizer *self, GDataAuthorizationDomain *domain, SoupMessage *message)
{
	SoupURI *test_uri;

	/* Check that the domain and message are as expected */
	g_assert (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain));
	if (domain != NULL) {
		g_assert_cmpstr (gdata_authorization_domain_get_scope (domain), ==, "scope1");
	}

	g_assert (message != NULL);
	g_assert (SOUP_IS_MESSAGE (message));
	test_uri = soup_uri_new ("http://example.com/");
	g_assert (soup_uri_equal (soup_message_get_uri (message), test_uri) == TRUE);
	soup_uri_free (test_uri);

	/* Check that this is the first time we've touched the message, and if so, flag the message as touched */
	if (domain != NULL) {
		g_assert (soup_message_headers_get_one (message->request_headers, "process_request") == NULL);
		soup_message_headers_append (message->request_headers, "process_request", "1");
	} else {
		soup_message_headers_append (message->request_headers, "process_request_null", "1");
	}
}

static gboolean
simple_authorizer_is_authorized_for_domain (GDataAuthorizer *self, GDataAuthorizationDomain *domain)
{
	gboolean is_test_domain1, is_test_domain2;

	/* Check that the domain is as expected */
	g_assert (domain != NULL);
	g_assert (GDATA_IS_AUTHORIZATION_DOMAIN (domain));

	is_test_domain1 = (strcmp (gdata_authorization_domain_get_scope (domain), "scope1") == 0) ? TRUE : FALSE;
	is_test_domain2 = (strcmp (gdata_authorization_domain_get_scope (domain), "scope2") == 0) ? TRUE : FALSE;

	g_assert (is_test_domain1 == TRUE || is_test_domain2 == TRUE);

	/* Increment the counter on the domain so we know if this function's been called more than once on each domain */
	g_object_set_data (G_OBJECT (domain), "counter", GUINT_TO_POINTER (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (domain), "counter")) + 1));

	/* Only authorise test_domain1 */
	return is_test_domain1;
}

static void
simple_authorizer_authorizer_init (GDataAuthorizerInterface *iface)
{
	iface->process_request = simple_authorizer_process_request;
	iface->is_authorized_for_domain = simple_authorizer_is_authorized_for_domain;
}

/* Normal implementation of GDataAuthorizer for test purposes */
#define TYPE_NORMAL_AUTHORIZER		(normal_authorizer_get_type ())
#define NORMAL_AUTHORIZER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), NORMAL_TYPE_AUTHORIZER, NormalAuthorizer))
#define NORMAL_AUTHORIZER_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), NORMAL_TYPE_AUTHORIZER, NormalAuthorizerClass))
#define IS_NORMAL_AUTHORIZER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), NORMAL_TYPE_AUTHORIZER))
#define IS_NORMAL_AUTHORIZER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), NORMAL_TYPE_AUTHORIZER))
#define NORMAL_AUTHORIZER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), NORMAL_TYPE_AUTHORIZER, NormalAuthorizerClass))

typedef struct {
	GObject parent;
} NormalAuthorizer;

typedef struct {
	GObjectClass parent;
} NormalAuthorizerClass;

static GType normal_authorizer_get_type (void) G_GNUC_CONST;
static void normal_authorizer_authorizer_init (GDataAuthorizerInterface *iface);

G_DEFINE_TYPE_WITH_CODE (NormalAuthorizer, normal_authorizer, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_AUTHORIZER, normal_authorizer_authorizer_init))

static void
normal_authorizer_class_init (NormalAuthorizerClass *klass)
{
	/* Nothing to see here */
}

static void
normal_authorizer_init (NormalAuthorizer *self)
{
	/* Nothing to see here */
}

static gboolean
normal_authorizer_refresh_authorization (GDataAuthorizer *self, GCancellable *cancellable, GError **error)
{
	/* Check the inputs */
	g_assert (GDATA_IS_AUTHORIZER (self));
	g_assert (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_assert (error == NULL || *error == NULL);

	/* Increment the counter on the authorizer so we know if this function's been called more than once */
	g_object_set_data (G_OBJECT (self), "counter", GUINT_TO_POINTER (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (self), "counter")) + 1));

	/* If we're instructed to set an error, do so (with an arbitrary error code) */
	if (g_object_get_data (G_OBJECT (self), "error") != NULL) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR, "Error message");
		return FALSE;
	}

	return TRUE;
}

static void
normal_authorizer_authorizer_init (GDataAuthorizerInterface *iface)
{
	/* Use the same implementation as SimpleAuthorizer for process_request() and is_authorized_for_domain(). */
	iface->process_request = simple_authorizer_process_request;
	iface->is_authorized_for_domain = simple_authorizer_is_authorized_for_domain;

	/* Unlike SimpleAuthorizer, also implement refresh_authorization() (but not the async versions). */
	iface->refresh_authorization = normal_authorizer_refresh_authorization;
}

/* Complex implementation of GDataAuthorizer for test purposes */
#define TYPE_COMPLEX_AUTHORIZER		(complex_authorizer_get_type ())
#define COMPLEX_AUTHORIZER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), COMPLEX_TYPE_AUTHORIZER, ComplexAuthorizer))
#define COMPLEX_AUTHORIZER_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), COMPLEX_TYPE_AUTHORIZER, ComplexAuthorizerClass))
#define IS_COMPLEX_AUTHORIZER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), COMPLEX_TYPE_AUTHORIZER))
#define IS_COMPLEX_AUTHORIZER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), COMPLEX_TYPE_AUTHORIZER))
#define COMPLEX_AUTHORIZER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), COMPLEX_TYPE_AUTHORIZER, ComplexAuthorizerClass))

typedef struct {
	GObject parent;
} ComplexAuthorizer;

typedef struct {
	GObjectClass parent;
} ComplexAuthorizerClass;

static GType complex_authorizer_get_type (void) G_GNUC_CONST;
static void complex_authorizer_authorizer_init (GDataAuthorizerInterface *iface);

G_DEFINE_TYPE_WITH_CODE (ComplexAuthorizer, complex_authorizer, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_AUTHORIZER, complex_authorizer_authorizer_init))

static void
complex_authorizer_class_init (ComplexAuthorizerClass *klass)
{
	/* Nothing to see here */
}

static void
complex_authorizer_init (ComplexAuthorizer *self)
{
	/* Nothing to see here */
}

static void
complex_authorizer_refresh_authorization_async (GDataAuthorizer *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	g_autoptr(GTask) task = NULL;
	g_autoptr(GError) error = NULL;

	/* Check the inputs */
	g_assert (GDATA_IS_AUTHORIZER (self));
	g_assert (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, complex_authorizer_refresh_authorization_async);

	/* Increment the async counter on the authorizer so we know if this function's been called more than once */
	g_object_set_data (G_OBJECT (self), "async-counter",
	                   GUINT_TO_POINTER (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (self), "async-counter")) + 1));

	if (g_cancellable_set_error_if_cancelled (cancellable, &error) == TRUE) {
		/* Handle cancellation */
		g_task_return_error (task, g_steal_pointer (&error));
	} else if (g_object_get_data (G_OBJECT (self), "error") != NULL) {
		/* If we're instructed to set an error, do so (with an arbitrary error code) */
		g_task_return_new_error (task, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_NETWORK_ERROR, "%s", "Error message");
	} else {
		g_task_return_boolean (task, TRUE);
	}
}

static gboolean
complex_authorizer_refresh_authorization_finish (GDataAuthorizer *self, GAsyncResult *async_result, GError **error)
{
	/* Check the inputs */
	g_assert (GDATA_IS_AUTHORIZER (self));
	g_assert (G_IS_ASYNC_RESULT (async_result));
	g_assert (error == NULL || *error == NULL);
	g_assert (g_task_is_valid (async_result, self));
	g_assert (g_async_result_is_tagged (async_result, complex_authorizer_refresh_authorization_async));

	/* Assert that the async function's already been called (once) */
	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (self), "async-counter")), ==, 1);

	/* Increment the finish counter on the authorizer so we know if this function's been called more than once */
	g_object_set_data (G_OBJECT (self), "finish-counter",
	                   GUINT_TO_POINTER (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (self), "finish-counter")) + 1));

	return g_task_propagate_boolean (G_TASK (async_result), error);
}

static void
complex_authorizer_authorizer_init (GDataAuthorizerInterface *iface)
{
	/* Use the same implementation as SimpleAuthorizer/NormalAuthorizer for process_request(), is_authorized_for_domain() and
	 * refresh_authorization(). */
	iface->process_request = simple_authorizer_process_request;
	iface->is_authorized_for_domain = simple_authorizer_is_authorized_for_domain;
	iface->refresh_authorization = normal_authorizer_refresh_authorization;

	/* Unlike NormalAuthorizer, also implement the async versions of refresh_authorization(). */
	iface->refresh_authorization_async = complex_authorizer_refresh_authorization_async;
	iface->refresh_authorization_finish = complex_authorizer_refresh_authorization_finish;
}

/* Testing data for generic GDataAuthorizer interface tests */
typedef struct {
	GDataAuthorizer *authorizer;
} AuthorizerData;

static void
set_up_simple_authorizer_data (AuthorizerData *data, gconstpointer user_data)
{
	data->authorizer = GDATA_AUTHORIZER (g_object_new (TYPE_SIMPLE_AUTHORIZER, NULL));
}

static void
set_up_normal_authorizer_data (AuthorizerData *data, gconstpointer user_data)
{
	data->authorizer = GDATA_AUTHORIZER (g_object_new (TYPE_NORMAL_AUTHORIZER, NULL));
}

static void
set_up_complex_authorizer_data (AuthorizerData *data, gconstpointer user_data)
{
	data->authorizer = GDATA_AUTHORIZER (g_object_new (TYPE_COMPLEX_AUTHORIZER, NULL));
}

static void
tear_down_authorizer_data (AuthorizerData *data, gconstpointer user_data)
{
	g_object_unref (data->authorizer);
}

/* Test that calling gdata_authorizer_process_request() happens correctly */
static void
test_authorizer_process_request (AuthorizerData *data, gconstpointer user_data)
{
	SoupMessage *message;

	message = soup_message_new (SOUP_METHOD_GET, "http://example.com/");

	gdata_authorizer_process_request (data->authorizer, test_domain1, message);
	g_assert_cmpstr (soup_message_headers_get_one (message->request_headers, "process_request"), ==, "1");
	g_assert (soup_message_headers_get_one (message->request_headers, "process_request_null") == NULL);

	g_object_unref (message);
}

/* Test that calling gdata_authorizer_process_request() happens correctly for a NULL domain */
static void
test_authorizer_process_request_null (AuthorizerData *data, gconstpointer user_data)
{
	SoupMessage *message;

	message = soup_message_new (SOUP_METHOD_GET, "http://example.com/");

	gdata_authorizer_process_request (data->authorizer, NULL, message);
	g_assert (soup_message_headers_get_one (message->request_headers, "process_request") == NULL);
	g_assert_cmpstr (soup_message_headers_get_one (message->request_headers, "process_request_null"), ==, "1");

	g_object_unref (message);
}

/* Test that calling gdata_authorizer_is_authorized_for_domain() happens correctly */
static void
test_authorizer_is_authorized_for_domain (AuthorizerData *data, gconstpointer user_data)
{
	/* Set some counters on the test domains to check that the interface implementation is only called once per domain */
	g_object_set_data (G_OBJECT (test_domain1), "counter", GUINT_TO_POINTER (0));
	g_object_set_data (G_OBJECT (test_domain2), "counter", GUINT_TO_POINTER (0));

	g_assert (gdata_authorizer_is_authorized_for_domain (data->authorizer, test_domain1) == TRUE);
	g_assert (gdata_authorizer_is_authorized_for_domain (data->authorizer, test_domain2) == FALSE);

	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (test_domain1), "counter")), ==, 1);
	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (test_domain2), "counter")), ==, 1);
}

/* Test that calling gdata_authorizer_is_authorized_for_domain() with a NULL authorizer always returns FALSE */
static void
test_authorizer_is_authorized_for_domain_null (AuthorizerData *data, gconstpointer user_data)
{
	g_assert (gdata_authorizer_is_authorized_for_domain (NULL, test_domain1) == FALSE);
	g_assert (gdata_authorizer_is_authorized_for_domain (NULL, test_domain2) == FALSE);
}

/* Test that calling refresh_authorization() on an authorizer which implements it returns TRUE without error, and only calls the implementation
 * once */
static void
test_authorizer_refresh_authorization (AuthorizerData *data, gconstpointer user_data)
{
	gboolean success;
	GError *error = NULL;

	/* Set a counter on the authoriser to check that the interface implementation is only called once */
	g_object_set_data (G_OBJECT (data->authorizer), "counter", GUINT_TO_POINTER (0));

	success = gdata_authorizer_refresh_authorization (data->authorizer, NULL, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);
	g_clear_error (&error);

	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->authorizer), "counter")), ==, 1);
}

/* Test that calling refresh_authorization() on an authorizer which implements it with errors returns FALSE with an error, and only calls the
 * implementation once */
static void
test_authorizer_refresh_authorization_error (AuthorizerData *data, gconstpointer user_data)
{
	gboolean success;
	GError *error = NULL;

	/* Set a counter on the authoriser to check that the interface implementation is only called once */
	g_object_set_data (G_OBJECT (data->authorizer), "counter", GUINT_TO_POINTER (0));

	/* Set a flag on the authoriser to make the NormalAuthorizer implementation return an error for refresh_authorization() */
	g_object_set_data (G_OBJECT (data->authorizer), "error", GUINT_TO_POINTER (TRUE));

	success = gdata_authorizer_refresh_authorization (data->authorizer, NULL, &error);
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR);
	g_assert (success == FALSE);
	g_clear_error (&error);

	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->authorizer), "counter")), ==, 1);
}

/* Test that calling refresh_authorization() on an authorizer which doesn't implement it returns FALSE without an error */
static void
test_authorizer_refresh_authorization_unimplemented (AuthorizerData *data, gconstpointer user_data)
{
	gboolean success;
	GError *error = NULL;

	success = gdata_authorizer_refresh_authorization (data->authorizer, NULL, &error);
	g_assert_no_error (error);
	g_assert (success == FALSE);
	g_clear_error (&error);
}

/* Test that calling refresh_authorization() on an authorizer which doesn't implement it, then cancelling the call returns FALSE without an error
 * (not even a cancellation error) */
static void
test_authorizer_refresh_authorization_cancellation_unimplemented (AuthorizerData *data, gconstpointer user_data)
{
	GCancellable *cancellable;
	gboolean success;
	GError *error = NULL;

	cancellable = g_cancellable_new ();
	g_cancellable_cancel (cancellable);

	success = gdata_authorizer_refresh_authorization (data->authorizer, cancellable, &error);
	g_assert_no_error (error);
	g_assert (success == FALSE);
	g_clear_error (&error);

	g_object_unref (cancellable);
}

/* Set of standard async callback functions for refresh_authorization_async() which check various combinations of success and error value */
static void
test_authorizer_refresh_authorization_async_success_no_error_cb (GDataAuthorizer *authorizer, GAsyncResult *async_result, GMainLoop *main_loop)
{
	gboolean success;
	GError *error = NULL;

	success = gdata_authorizer_refresh_authorization_finish (authorizer, async_result, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
}

static void
test_authorizer_refresh_authorization_async_failure_no_error_cb (GDataAuthorizer *authorizer, GAsyncResult *async_result, GMainLoop *main_loop)
{
	gboolean success;
	GError *error = NULL;

	success = gdata_authorizer_refresh_authorization_finish (authorizer, async_result, &error);
	g_assert_no_error (error);
	g_assert (success == FALSE);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
}

static void
test_authorizer_refresh_authorization_async_network_error_cb (GDataAuthorizer *authorizer, GAsyncResult *async_result, GMainLoop *main_loop)
{
	gboolean success;
	GError *error = NULL;

	success = gdata_authorizer_refresh_authorization_finish (authorizer, async_result, &error);
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_NETWORK_ERROR);
	g_assert (success == FALSE);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
}

static void
test_authorizer_refresh_authorization_async_protocol_error_cb (GDataAuthorizer *authorizer, GAsyncResult *async_result, GMainLoop *main_loop)
{
	gboolean success;
	GError *error = NULL;

	success = gdata_authorizer_refresh_authorization_finish (authorizer, async_result, &error);
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR);
	g_assert (success == FALSE);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
}

static void
test_authorizer_refresh_authorization_async_cancelled_error_cb (GDataAuthorizer *authorizer, GAsyncResult *async_result, GMainLoop *main_loop)
{
	gboolean success;
	GError *error = NULL;

	success = gdata_authorizer_refresh_authorization_finish (authorizer, async_result, &error);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_assert (success == FALSE);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
}

/* Test that calling refresh_authorization_async() on an authorizer which implements it returns TRUE without an error */
static void
test_authorizer_refresh_authorization_async (AuthorizerData *data, gconstpointer user_data)
{
	GMainLoop *main_loop;

	/* Set counters on the authoriser to check that the interface implementations are only called once */
	g_object_set_data (G_OBJECT (data->authorizer), "counter", GUINT_TO_POINTER (0));
	g_object_set_data (G_OBJECT (data->authorizer), "async-counter", GUINT_TO_POINTER (0));
	g_object_set_data (G_OBJECT (data->authorizer), "finish-counter", GUINT_TO_POINTER (0));

	main_loop = g_main_loop_new (NULL, FALSE);

	gdata_authorizer_refresh_authorization_async (data->authorizer, NULL,
	                                              (GAsyncReadyCallback) test_authorizer_refresh_authorization_async_success_no_error_cb,
	                                              main_loop);

	g_main_loop_run (main_loop);

	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->authorizer), "counter")), ==, 0);
	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->authorizer), "async-counter")), ==, 1);
	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->authorizer), "finish-counter")), ==, 1);

	g_main_loop_unref (main_loop);
}

/* Test that calling refresh_authorization_async() on an authorizer which implements it with an error returns FALSE with the appropriate error */
static void
test_authorizer_refresh_authorization_async_error (AuthorizerData *data, gconstpointer user_data)
{
	GMainLoop *main_loop;

	/* Set counters on the authoriser to check that the interface implementations are only called once */
	g_object_set_data (G_OBJECT (data->authorizer), "counter", GUINT_TO_POINTER (0));
	g_object_set_data (G_OBJECT (data->authorizer), "async-counter", GUINT_TO_POINTER (0));
	g_object_set_data (G_OBJECT (data->authorizer), "finish-counter", GUINT_TO_POINTER (0));

	/* Set a flag on the authoriser to make the ComplexAuthorizer implementation return an error for refresh_authorization_async() */
	g_object_set_data (G_OBJECT (data->authorizer), "error", GUINT_TO_POINTER (TRUE));

	main_loop = g_main_loop_new (NULL, FALSE);

	gdata_authorizer_refresh_authorization_async (data->authorizer, NULL,
	                                              (GAsyncReadyCallback) test_authorizer_refresh_authorization_async_network_error_cb, main_loop);

	g_main_loop_run (main_loop);

	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->authorizer), "counter")), ==, 0);
	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->authorizer), "async-counter")), ==, 1);
	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->authorizer), "finish-counter")), ==, 1);

	g_main_loop_unref (main_loop);
}

/* Test that calling refresh_authorization_async() on an authorizer which implements it, then cancelling the call returns FALSE with a cancellation
 * error */
static void
test_authorizer_refresh_authorization_async_cancellation (AuthorizerData *data, gconstpointer user_data)
{
	GCancellable *cancellable;
	GMainLoop *main_loop;

	/* Set counters on the authoriser to check that the interface implementations are only called once */
	g_object_set_data (G_OBJECT (data->authorizer), "counter", GUINT_TO_POINTER (0));
	g_object_set_data (G_OBJECT (data->authorizer), "async-counter", GUINT_TO_POINTER (0));
	g_object_set_data (G_OBJECT (data->authorizer), "finish-counter", GUINT_TO_POINTER (0));

	main_loop = g_main_loop_new (NULL, FALSE);

	cancellable = g_cancellable_new ();
	g_cancellable_cancel (cancellable);

	gdata_authorizer_refresh_authorization_async (data->authorizer, cancellable,
	                                              (GAsyncReadyCallback) test_authorizer_refresh_authorization_async_cancelled_error_cb,
	                                              main_loop);

	g_main_loop_run (main_loop);

	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->authorizer), "counter")), ==, 0);
	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->authorizer), "async-counter")), ==, 1);
	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->authorizer), "finish-counter")), ==, 1);

	g_object_unref (cancellable);
	g_main_loop_unref (main_loop);
}

/* Test that calling refresh_authorization_async() on an authorizer which doesn't implement it, but does implement refresh_authorization(), returns
 * TRUE without an error */
static void
test_authorizer_refresh_authorization_async_simulated (AuthorizerData *data, gconstpointer user_data)
{
	GMainLoop *main_loop;

	/* Set a counter on the authoriser to check that the interface implementation is only called once */
	g_object_set_data (G_OBJECT (data->authorizer), "counter", GUINT_TO_POINTER (0));

	main_loop = g_main_loop_new (NULL, FALSE);

	gdata_authorizer_refresh_authorization_async (data->authorizer, NULL,
	                                              (GAsyncReadyCallback) test_authorizer_refresh_authorization_async_success_no_error_cb,
	                                              main_loop);

	g_main_loop_run (main_loop);

	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->authorizer), "counter")), ==, 1);

	g_main_loop_unref (main_loop);
}

/* Test that calling refresh_authorization_async() on an authorizer which doesn't implement it, but does implement refresh_authorization() with an
 * error, returns FALSE with the appropriate error */
static void
test_authorizer_refresh_authorization_async_error_simulated (AuthorizerData *data, gconstpointer user_data)
{
	GMainLoop *main_loop;

	/* Set a counter on the authoriser to check that the interface implementation is only called once */
	g_object_set_data (G_OBJECT (data->authorizer), "counter", GUINT_TO_POINTER (0));

	/* Set a flag on the authoriser to make the NormalAuthorizer implementation return an error for refresh_authorization() */
	g_object_set_data (G_OBJECT (data->authorizer), "error", GUINT_TO_POINTER (TRUE));

	main_loop = g_main_loop_new (NULL, FALSE);

	gdata_authorizer_refresh_authorization_async (data->authorizer, NULL,
	                                              (GAsyncReadyCallback) test_authorizer_refresh_authorization_async_protocol_error_cb, main_loop);

	g_main_loop_run (main_loop);

	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->authorizer), "counter")), ==, 1);

	g_main_loop_unref (main_loop);
}

/* Test that calling refresh_authorization_async() on an authorizer which doesn't implement it, but does implement refresh_authorization(), then
 * cancelling the call returns FALSE with a cancellation error */
static void
test_authorizer_refresh_authorization_async_cancellation_simulated (AuthorizerData *data, gconstpointer user_data)
{
	GCancellable *cancellable;
	GMainLoop *main_loop;

	main_loop = g_main_loop_new (NULL, FALSE);

	cancellable = g_cancellable_new ();
	g_cancellable_cancel (cancellable);

	/* Note we don't count how many times the implementation of refresh_authorization() is called, since cancellation can legitimately be
	 * handled by the gdata_authorizer_refresh_authorization_async() code before refresh_authorization() is ever called. */
	gdata_authorizer_refresh_authorization_async (data->authorizer, cancellable,
	                                              (GAsyncReadyCallback) test_authorizer_refresh_authorization_async_cancelled_error_cb,
	                                              main_loop);

	g_main_loop_run (main_loop);

	g_object_unref (cancellable);
	g_main_loop_unref (main_loop);
}

/* Test that calling refresh_authorization_async() on an authorizer which doesn't implement it returns FALSE without an error */
static void
test_authorizer_refresh_authorization_async_unimplemented (AuthorizerData *data, gconstpointer user_data)
{
	GMainLoop *main_loop;

	main_loop = g_main_loop_new (NULL, FALSE);

	gdata_authorizer_refresh_authorization_async (data->authorizer, NULL,
	                                              (GAsyncReadyCallback) test_authorizer_refresh_authorization_async_failure_no_error_cb,
	                                              main_loop);

	g_main_loop_run (main_loop);

	g_main_loop_unref (main_loop);
}

/* Test that calling refresh_authorization_async() on an authorizer which doesn't implement it, then cancelling the call returns FALSE without an
 * error (not even a cancellation error) */
static void
test_authorizer_refresh_authorization_async_cancellation_unimplemented (AuthorizerData *data, gconstpointer user_data)
{
	GCancellable *cancellable;
	GMainLoop *main_loop;

	main_loop = g_main_loop_new (NULL, FALSE);

	cancellable = g_cancellable_new ();
	g_cancellable_cancel (cancellable);

	gdata_authorizer_refresh_authorization_async (data->authorizer, cancellable,
	                                              (GAsyncReadyCallback) test_authorizer_refresh_authorization_async_failure_no_error_cb,
	                                              main_loop);

	g_main_loop_run (main_loop);

	g_object_unref (cancellable);
	g_main_loop_unref (main_loop);
}

int
main (int argc, char *argv[])
{
	int retval;

	gdata_test_init (argc, argv);

	/* Note: This is not how GDataAuthorizationDomains are meant to be constructed. Client code is not expected to do this. */
	test_domain1 = g_object_new (GDATA_TYPE_AUTHORIZATION_DOMAIN,
	                             "service-name", "service-name1",
	                             "scope", "scope1",
	                             NULL);
	test_domain2 = g_object_new (GDATA_TYPE_AUTHORIZATION_DOMAIN,
	                             "service-name", "service-name2",
	                             "scope", "scope2",
	                             NULL);

	/* GDataAuthorizationDomain tests */
	g_test_add_func ("/authorization-domain/properties", test_authorization_domain_properties);

	/* GDataAuthorizer interface tests */
	g_test_add ("/authorizer/process-request", AuthorizerData, NULL, set_up_simple_authorizer_data, test_authorizer_process_request,
	            tear_down_authorizer_data);
	g_test_add ("/authorizer/process-request/null", AuthorizerData, NULL, set_up_simple_authorizer_data, test_authorizer_process_request_null,
	            tear_down_authorizer_data);
	g_test_add ("/authorizer/is-authorized-for-domain", AuthorizerData, NULL, set_up_simple_authorizer_data,
	            test_authorizer_is_authorized_for_domain, tear_down_authorizer_data);
	g_test_add ("/authorizer/is-authorized-for-domain/null", AuthorizerData, NULL, set_up_simple_authorizer_data,
	            test_authorizer_is_authorized_for_domain_null, tear_down_authorizer_data);
	g_test_add ("/authorizer/refresh-authorization", AuthorizerData, NULL, set_up_normal_authorizer_data,
	            test_authorizer_refresh_authorization, tear_down_authorizer_data);
	g_test_add ("/authorizer/refresh-authorization/error", AuthorizerData, NULL, set_up_normal_authorizer_data,
	            test_authorizer_refresh_authorization_error, tear_down_authorizer_data);
	g_test_add ("/authorizer/refresh-authorization/unimplemented", AuthorizerData, NULL, set_up_simple_authorizer_data,
	            test_authorizer_refresh_authorization_unimplemented, tear_down_authorizer_data);
	g_test_add ("/authorizer/refresh-authorization/cancellation/unimplemented", AuthorizerData, NULL, set_up_simple_authorizer_data,
	            test_authorizer_refresh_authorization_cancellation_unimplemented, tear_down_authorizer_data);
	g_test_add ("/authorizer/refresh-authorization/async", AuthorizerData, NULL, set_up_complex_authorizer_data,
	            test_authorizer_refresh_authorization_async, tear_down_authorizer_data);
	g_test_add ("/authorizer/refresh-authorization/async/error", AuthorizerData, NULL, set_up_complex_authorizer_data,
	            test_authorizer_refresh_authorization_async_error, tear_down_authorizer_data);
	g_test_add ("/authorizer/refresh-authorization/async/cancellation", AuthorizerData, NULL, set_up_complex_authorizer_data,
	            test_authorizer_refresh_authorization_async_cancellation, tear_down_authorizer_data);
	g_test_add ("/authorizer/refresh-authorization/async/simulated", AuthorizerData, NULL, set_up_normal_authorizer_data,
	            test_authorizer_refresh_authorization_async_simulated, tear_down_authorizer_data);
	g_test_add ("/authorizer/refresh-authorization/async/error/simulated", AuthorizerData, NULL, set_up_normal_authorizer_data,
	            test_authorizer_refresh_authorization_async_error_simulated, tear_down_authorizer_data);
	g_test_add ("/authorizer/refresh-authorization/async/cancellation/simulated", AuthorizerData, NULL, set_up_normal_authorizer_data,
	            test_authorizer_refresh_authorization_async_cancellation_simulated, tear_down_authorizer_data);
	g_test_add ("/authorizer/refresh-authorization/async/unimplemented", AuthorizerData, NULL, set_up_simple_authorizer_data,
	            test_authorizer_refresh_authorization_async_unimplemented, tear_down_authorizer_data);
	g_test_add ("/authorizer/refresh-authorization/async/cancellation/unimplemented", AuthorizerData, NULL, set_up_simple_authorizer_data,
	            test_authorizer_refresh_authorization_async_cancellation_unimplemented, tear_down_authorizer_data);

	retval = g_test_run ();

	g_object_unref (test_domain2);
	g_object_unref (test_domain1);

	return retval;
}
