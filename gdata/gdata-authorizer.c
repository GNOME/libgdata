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

/**
 * SECTION:gdata-authorizer
 * @short_description: GData authorization interface
 * @stability: Stable
 * @include: gdata/gdata-authorizer.h
 *
 * The #GDataAuthorizer interface provides a uniform way to implement authentication and authorization processes for use by #GDataServices.
 * Client code will construct a new #GDataAuthorizer instance of their choosing, such as #GDataOAuth2Authorizer, for
 * the #GDataServices which will be used by the client, then authenticates and authorizes with the #GDataAuthorizer instead of the
 * #GDataService. The #GDataService then uses the #GDataAuthorizer to authorize individual network requests using whatever authorization token was
 * returned to the #GDataAuthorizer by the Google Accounts service.
 *
 * All #GDataAuthorizer implementations are expected to operate against a set of #GDataAuthorizationDomains which are provided to the
 * authorizer at construction time. These domains specify which data domains the client expects to access using the #GDataServices they
 * have using the #GDataAuthorizer instance. Following the principle of least privilege, the set of domains should be the minimum such set of domains
 * which still allows the client to operate normally. Note that implementations of #GDataAuthorizationDomain may display the list of requested
 * authorization domains to the user for verification before authorization is granted.
 *
 * #GDataAuthorizer implementations are provided for some of the standard authorization processes supported by Google for installed applications, as
 * listed in their <ulink type="http" url="http://code.google.com/apis/accounts/docs/GettingStarted.html">online documentation</ulink>:
 * <itemizedlist>
 *  <listitem>#GDataOAuth2Authorizer for
 *    <ulink type="http" url="https://developers.google.com/accounts/docs/OAuth2InstalledApp">OAuth 2.0</ulink> (preferred)</listitem>
 * </itemizedlist>
 *
 * It is quite possible for clients to write their own #GDataAuthorizer implementation. For example, if a client already uses OAuth 2.0 and handles
 * authentication itself, it may want to use its own #GDataAuthorizer implementation which simply exposes the client's existing access token to
 * libgdata and does nothing more.
 *
 * It must be noted that all #GDataAuthorizer implementations must be thread safe, as methods such as gdata_authorizer_refresh_authorization() may be
 * called from any thread (such as the thread performing an asynchronous query operation) at any time.
 *
 * Examples of code using #GDataAuthorizer can be found in the documentation for the various implementations of the #GDataAuthorizer interface.
 *
 * Since: 0.9.0
 */

#include <glib.h>

#include "gdata-authorizer.h"

G_DEFINE_INTERFACE (GDataAuthorizer, gdata_authorizer, G_TYPE_OBJECT)

static void
gdata_authorizer_default_init (GDataAuthorizerInterface *iface)
{
	/* Nothing to see here */
}

/**
 * gdata_authorizer_process_request:
 * @self: a #GDataAuthorizer
 * @domain: (allow-none): the #GDataAuthorizationDomain the query falls under, or %NULL
 * @message: the query to process
 *
 * Processes @message, adding all the necessary extra headers and parameters to ensure that it's correctly authenticated and authorized under the
 * given @domain for the online service. Basically, if a query is not processed by calling this method on it, it will be sent to the online service as
 * if it's a query from a non-logged-in user. Similarly, if the #GDataAuthorizer isn't authenticated or authorized (for @domain), no changes will
 * be made to the @message.
 *
 * @domain may be %NULL if the request doesn't require authorization.
 *
 * This modifies @message in place.
 *
 * This method is thread safe.
 *
 * Since: 0.9.0
 */
void
gdata_authorizer_process_request (GDataAuthorizer *self, GDataAuthorizationDomain *domain, SoupMessage *message)
{
	GDataAuthorizerInterface *iface;

	g_return_if_fail (GDATA_IS_AUTHORIZER (self));
	g_return_if_fail (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain));
	g_return_if_fail (SOUP_IS_MESSAGE (message));

	iface = GDATA_AUTHORIZER_GET_IFACE (self);
	g_assert (iface->process_request != NULL);

	iface->process_request (self, domain, message);
}

/**
 * gdata_authorizer_is_authorized_for_domain:
 * @self: (allow-none): a #GDataAuthorizer, or %NULL
 * @domain: the #GDataAuthorizationDomain to check against
 *
 * Returns whether the #GDataAuthorizer instance believes it's currently authorized to access the given @domain. Note that this will not perform any
 * network requests, and will just look up the result in the #GDataAuthorizer's local cache of authorizations. This means that the result may be out
 * of date, as the server may have since invalidated the authorization. If the #GDataAuthorizer class supports timeouts and TTLs on authorizations,
 * they will not be taken into account; this method effectively returns whether the last successful authorization operation performed on the
 * #GDataAuthorizer included @domain in the list of requested authorization domains.
 *
 * Note that %NULL may be passed as the #GDataAuthorizer, in which case %FALSE will always be returned, regardless of the @domain. This is for
 * convenience of checking whether a domain is authorized by the #GDataAuthorizer returned by gdata_service_get_authorizer(), which may be %NULL.
 * For example:
 * |[
 * if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (my_service), my_domain) == TRUE) {
 * 	/<!-- -->* Code to execute only if we're authorized for the given domain *<!-- -->/
 * }
 * ]|
 *
 * This method is thread safe.
 *
 * Return value: %TRUE if the #GDataAuthorizer has been authorized to access @domain, %FALSE otherwise
 *
 * Since: 0.9.0
 */
gboolean
gdata_authorizer_is_authorized_for_domain (GDataAuthorizer *self, GDataAuthorizationDomain *domain)
{
	GDataAuthorizerInterface *iface;

	g_return_val_if_fail (self == NULL || GDATA_IS_AUTHORIZER (self), FALSE);
	g_return_val_if_fail (GDATA_IS_AUTHORIZATION_DOMAIN (domain), FALSE);

	if (self == NULL) {
		return FALSE;
	}

	iface = GDATA_AUTHORIZER_GET_IFACE (self);
	g_assert (iface->is_authorized_for_domain != NULL);

	return iface->is_authorized_for_domain (self, domain);
}

/**
 * gdata_authorizer_refresh_authorization:
 * @self: a #GDataAuthorizer
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Forces the #GDataAuthorizer to refresh any authorization tokens it holds with the online service. This should typically be called when a
 * #GDataService query returns %GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED, and is already called transparently by methods such as
 * gdata_service_query() and gdata_service_insert_entry() (see their documentation for more details).
 *
 * If re-authorization is successful, it's guaranteed that by the time this method returns, the properties containing the relevant authorization
 * tokens on the #GDataAuthorizer instance will have been updated.
 *
 * If %FALSE is returned, @error will be set if (and only if) it's due to a refresh being attempted and failing. If a refresh is not attempted, %FALSE
 * will be returned but @error will not be set.
 *
 * If the #GDataAuthorizer has not been previously authenticated or authorized (using the class' specific methods), no authorization will be
 * attempted, %FALSE will be returned immediately and @error will not be set.
 *
 * Some #GDataAuthorizer implementations may not support refreshing authorization tokens at all; for example if doing so requires user interaction.
 * %FALSE will be returned immediately in that case and @error will not be set.
 *
 * This method is thread safe.
 *
 * Return value: %TRUE if an authorization refresh was attempted and was successful, %FALSE if a refresh wasn't attempted or was unsuccessful
 *
 * Since: 0.9.0
 */
gboolean
gdata_authorizer_refresh_authorization (GDataAuthorizer *self, GCancellable *cancellable, GError **error)
{
	GDataAuthorizerInterface *iface;

	g_return_val_if_fail (GDATA_IS_AUTHORIZER (self), FALSE);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	iface = GDATA_AUTHORIZER_GET_IFACE (self);

	/* Return FALSE with no error if the method isn't implemented */
	if (iface->refresh_authorization == NULL) {
		return FALSE;
	}

	return iface->refresh_authorization (self, cancellable, error);
}

static void
refresh_authorization_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	GDataAuthorizer *authorizer = GDATA_AUTHORIZER (source_object);
	g_autoptr(GError) error = NULL;

	/* Refresh the authorisation and return */
	gdata_authorizer_refresh_authorization (authorizer, cancellable, &error);

	if (error != NULL)
		g_task_return_error (task, g_steal_pointer (&error));
	else
		g_task_return_boolean (task, TRUE);
}

/**
 * gdata_authorizer_refresh_authorization_async:
 * @self: a #GDataAuthorizer
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: (allow-none) (scope async): a #GAsyncReadyCallback to call when the authorization refresh operation is finished, or %NULL
 * @user_data: (closure): data to pass to the @callback function
 *
 * Forces the #GDataAuthorizer to refresh any authorization tokens it holds with the online service. @self and @cancellable are reffed when this
 * method is called, so can safely be freed after this method returns.
 *
 * For more details, see gdata_authorizer_refresh_authorization(), which is the synchronous version of this method. If the #GDataAuthorizer class
 * doesn't implement #GDataAuthorizerInterface.refresh_authorization_async but does implement #GDataAuthorizerInterface.refresh_authorization, the
 * latter will be called from a new thread to make it asynchronous.
 *
 * When the authorization refresh operation is finished, @callback will be called. You can then call gdata_authorizer_refresh_authorization_finish()
 * to get the results of the operation.
 *
 * This method is thread safe.
 *
 * Since: 0.9.0
 */
void
gdata_authorizer_refresh_authorization_async (GDataAuthorizer *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	GDataAuthorizerInterface *iface;

	g_return_if_fail (GDATA_IS_AUTHORIZER (self));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	iface = GDATA_AUTHORIZER_GET_IFACE (self);

	/* Either both _async() and _finish() must be defined, or they must both be undefined. */
	g_assert ((iface->refresh_authorization_async == NULL && iface->refresh_authorization_finish == NULL) ||
	          (iface->refresh_authorization_async != NULL && iface->refresh_authorization_finish != NULL));

	if (iface->refresh_authorization_async != NULL) {
		/* Call the method */
		iface->refresh_authorization_async (self, cancellable, callback, user_data);
	} else {
		g_autoptr(GTask) task = NULL;

		task = g_task_new (self, cancellable, callback, user_data);
		g_task_set_source_tag (task, gdata_authorizer_refresh_authorization_async);


		if (iface->refresh_authorization != NULL) {
			/* If the _async() method isn't implemented, fall back to running the sync method in a thread */
			g_task_run_in_thread (task, refresh_authorization_thread);
		} else {
			/* If neither are implemented, immediately return FALSE with no error in a callback */
			g_task_return_boolean (task, FALSE);
		}

		return;
	}
}

/**
 * gdata_authorizer_refresh_authorization_finish:
 * @self: a #GDataAuthorizer
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous authorization refresh operation for the #GDataAuthorizer, as started with gdata_authorizer_refresh_authorization_async().
 *
 * This method is thread safe.
 *
 * Return value: %TRUE if an authorization refresh was attempted and was successful, %FALSE if a refresh wasn't attempted or was unsuccessful
 *
 * Since: 0.9.0
 */
gboolean
gdata_authorizer_refresh_authorization_finish (GDataAuthorizer *self, GAsyncResult *async_result, GError **error)
{
	GDataAuthorizerInterface *iface;

	g_return_val_if_fail (GDATA_IS_AUTHORIZER (self), FALSE);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	iface = GDATA_AUTHORIZER_GET_IFACE (self);

	/* Either both _async() and _finish() must be defined, or they must both be undefined. */
	g_assert ((iface->refresh_authorization_async == NULL && iface->refresh_authorization_finish == NULL) ||
	          (iface->refresh_authorization_async != NULL && iface->refresh_authorization_finish != NULL));

	if (iface->refresh_authorization_finish != NULL) {
		/* Call the method */
		return iface->refresh_authorization_finish (self, async_result, error);
	} else if (iface->refresh_authorization != NULL) {
		/* If the _async() method isn't implemented, fall back to finishing off running the sync method in a thread */
		g_return_val_if_fail (g_task_is_valid (async_result, self), FALSE);
		g_return_val_if_fail (g_async_result_is_tagged (async_result, gdata_authorizer_refresh_authorization_async), FALSE);

		return g_task_propagate_boolean (G_TASK (async_result), error);
	}

	/* Fall back to just returning FALSE if none of the methods are implemented */
	return FALSE;
}
