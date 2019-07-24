/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009–2010, 2015 <philip@tecnocode.co.uk>
 * Copyright (C) Red Hat, Inc. 2015
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
 * SECTION:gdata-access-handler
 * @short_description: GData access handler interface
 * @stability: Stable
 * @include: gdata/gdata-access-handler.h
 *
 * #GDataAccessHandler is an interface which can be implemented by #GDataEntrys which can have their permissions controlled by an
 * access control list (ACL). It has a set of methods which allow the #GDataAccessRules for the access handler/entry to be retrieved,
 * added, modified and deleted, with immediate effect.
 *
 * For an example of inserting an access rule into an ACL, see the documentation for #GDataAccessRule.
 *
 * When implementing the interface, classes must implement an <function>is_owner_rule</function> function. It's optional to implement a
 * <function>get_authorization_domain</function> function, but if it's not implemented, any operations on the access handler's
 * #GDataAccessRules will be performed unauthorized (i.e. as if by a non-logged-in user). This will not usually work.
 *
 * Since: 0.3.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-access-handler.h"
#include "gdata-private.h"
#include "gdata-access-rule.h"

static GDataFeed *
gdata_access_handler_real_get_rules (GDataAccessHandler *self,
                                     GDataService *service,
                                     GCancellable *cancellable,
                                     GDataQueryProgressCallback progress_callback,
                                     gpointer progress_user_data,
                                     GError **error);

typedef GDataAccessHandlerIface GDataAccessHandlerInterface;
G_DEFINE_INTERFACE (GDataAccessHandler, gdata_access_handler,
                    GDATA_TYPE_ENTRY);

static void
gdata_access_handler_default_init (GDataAccessHandlerInterface *iface)
{
	iface->get_rules = gdata_access_handler_real_get_rules;
}

typedef struct {
	GDataService *service;
	GDataQueryProgressCallback progress_callback;
	gpointer progress_user_data;
	GDestroyNotify destroy_progress_user_data;
} GetRulesAsyncData;

static void
get_rules_async_data_free (GetRulesAsyncData *self)
{
	if (self->service != NULL)
		g_object_unref (self->service);

	g_slice_free (GetRulesAsyncData, self);
}

static GDataFeed *
gdata_access_handler_real_get_rules (GDataAccessHandler *self,
                                     GDataService *service,
                                     GCancellable *cancellable,
                                     GDataQueryProgressCallback progress_callback,
                                     gpointer progress_user_data,
                                     GError **error)
{
	GDataAccessHandlerIface *iface;
	GDataAuthorizationDomain *domain = NULL;
	GDataFeed *feed;
	GDataLink *_link;
	SoupMessage *message;
	SoupMessageHeaders *headers;
	const gchar *content_type;

	_link = gdata_entry_look_up_link (GDATA_ENTRY (self), GDATA_LINK_ACCESS_CONTROL_LIST);
	g_assert (_link != NULL);

	iface = GDATA_ACCESS_HANDLER_GET_IFACE (self);
	if (iface->get_authorization_domain != NULL) {
		domain = iface->get_authorization_domain (self);
	}

	message = _gdata_service_query (service, domain, gdata_link_get_uri (_link), NULL, cancellable, error);
	if (message == NULL) {
		return NULL;
	}

	g_assert (message->response_body->data != NULL);

	headers = message->response_headers;
	content_type = soup_message_headers_get_content_type (headers, NULL);

	if (g_strcmp0 (content_type, "application/json") == 0) {
		/* Definitely JSON. */
		g_debug("JSON content type detected.");
		feed = _gdata_feed_new_from_json (GDATA_TYPE_FEED, message->response_body->data, message->response_body->length, GDATA_TYPE_ACCESS_RULE,
		                                  progress_callback, progress_user_data, error);
	} else {
		/* Potentially XML. Don't bother checking the Content-Type, since the parser
		 * will fail gracefully if the response body is not valid XML. */
		g_debug("XML content type detected.");
		feed = _gdata_feed_new_from_xml (GDATA_TYPE_FEED, message->response_body->data, message->response_body->length, GDATA_TYPE_ACCESS_RULE,
		                                 progress_callback, progress_user_data, error);
	}

	g_object_unref (message);

	return feed;
}

static void
get_rules_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	GDataAccessHandler *access_handler = GDATA_ACCESS_HANDLER (source_object);
	GDataAccessHandlerIface *iface;
	g_autoptr(GError) error = NULL;
	g_autoptr(GDataFeed) feed = NULL;
	GetRulesAsyncData *data = task_data;

	/* Execute the query and return */
	iface = GDATA_ACCESS_HANDLER_GET_IFACE (access_handler);
	g_assert (iface->get_rules != NULL);

	feed = iface->get_rules (access_handler, data->service,
	                         cancellable, data->progress_callback,
	                         data->progress_user_data, &error);

	if (feed == NULL && error != NULL)
		g_task_return_error (task, g_steal_pointer (&error));
	else
		g_task_return_pointer (task, g_steal_pointer (&feed), g_object_unref);

	if (data->destroy_progress_user_data != NULL) {
		data->destroy_progress_user_data (data->progress_user_data);
	}
}

/**
 * gdata_access_handler_get_rules_async:
 * @self: a #GDataAccessHandler
 * @service: a #GDataService
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (closure progress_user_data): a #GDataQueryProgressCallback to call when a rule is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @destroy_progress_user_data: (allow-none): the function to call when @progress_callback will not be called any more, or %NULL. This function will be
 * called with @progress_user_data as a parameter and can be used to free any memory allocated for it.
 * @callback: a #GAsyncReadyCallback to call when the query is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Retrieves a #GDataFeed containing all the access rules which apply to the given #GDataAccessHandler. Only the owner of a #GDataAccessHandler may
 * view its rule feed. @self and @service are both reffed when this function is called, so can safely be unreffed after this function returns.
 *
 * For more details, see gdata_access_handler_get_rules(), which is the synchronous version of this function, and gdata_service_query_async(), which
 * is the base asynchronous query function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_service_query_finish()
 * to get the results of the operation.
 *
 * Since: 0.9.1
 */
void
gdata_access_handler_get_rules_async (GDataAccessHandler *self, GDataService *service, GCancellable *cancellable,
                                      GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                      GDestroyNotify destroy_progress_user_data,
                                      GAsyncReadyCallback callback, gpointer user_data)
{
	g_autoptr(GTask) task = NULL;
	GetRulesAsyncData *data;

	g_return_if_fail (GDATA_IS_ACCESS_HANDLER (self));
	g_return_if_fail (GDATA_IS_SERVICE (service));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	data = g_slice_new (GetRulesAsyncData);
	data->service = g_object_ref (service);
	data->progress_callback = progress_callback;
	data->progress_user_data = progress_user_data;
	data->destroy_progress_user_data = destroy_progress_user_data;

	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_service_query_async);
	g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) get_rules_async_data_free);
	g_task_run_in_thread (task, get_rules_thread);
}

/**
 * gdata_access_handler_get_rules:
 * @self: a #GDataAccessHandler
 * @service: a #GDataService
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (scope call) (closure progress_user_data): a #GDataQueryProgressCallback to call when a rule is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Retrieves a #GDataFeed containing all the access rules which apply to the given #GDataAccessHandler. Only the owner of a #GDataAccessHandler may
 * view its rule feed.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * A %GDATA_SERVICE_ERROR_PROTOCOL_ERROR will be returned if the server indicates there is a problem with the query.
 *
 * For each rule in the response feed, @progress_callback will be called in the main thread. If there was an error parsing the XML response,
 * a #GDataParserError will be returned.
 *
 * Return value: (transfer full): a #GDataFeed of access control rules, or %NULL; unref with g_object_unref()
 *
 * Since: 0.3.0
 */
GDataFeed *
gdata_access_handler_get_rules (GDataAccessHandler *self, GDataService *service, GCancellable *cancellable,
                                GDataQueryProgressCallback progress_callback, gpointer progress_user_data, GError **error)
{
	GDataAccessHandlerIface *iface;

	g_return_val_if_fail (GDATA_IS_ACCESS_HANDLER (self), NULL);
	g_return_val_if_fail (GDATA_IS_SERVICE (service), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	iface = GDATA_ACCESS_HANDLER_GET_IFACE (self);
	g_assert (iface->get_rules != NULL);

	return iface->get_rules (self, service, cancellable, progress_callback,
	                         progress_user_data, error);
}
