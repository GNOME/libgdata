/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
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

/**
 * SECTION:gdata-access-handler
 * @short_description: GData access handler interface
 * @stability: Unstable
 * @include: gdata/gdata-access-handler.h
 *
 * #GDataAccessHandler is an interface which can be implemented by #GDataEntry<!-- -->s which can have their permissions controlled by an
 * access control list (ACL). It has a set of methods which allow the #GDataAccessRule<!-- -->s for the access handler/entry to be retrieved,
 * added, modified and deleted, with immediate effect.
 *
 * For an example of inserting an access rule into an ACL, see the documentation for #GDataAccessRule.
 *
 * When implementing the interface, classes must implement an <function>is_owner_rule</function> function. It's optional to implement a
 * <function>get_authorization_domain</function> function, but if it's not implemented, any operations on the access handler's
 * #GDataAccessRule<!-- -->s will be performed unauthorized (i.e. as if by a non-logged-in user). This will not usually work.
 *
 * Since: 0.3.0
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-access-handler.h"
#include "gdata-private.h"
#include "gdata-access-rule.h"

GType
gdata_access_handler_get_type (void)
{
	static GType access_handler_type = 0;

	if (!access_handler_type) {
		access_handler_type = g_type_register_static_simple (G_TYPE_INTERFACE, "GDataAccessHandler",
		                                                     sizeof (GDataAccessHandlerIface),
		                                                     NULL, 0, NULL, 0);
		g_type_interface_add_prerequisite (access_handler_type, GDATA_TYPE_ENTRY);
	}

	return access_handler_type;
}

typedef struct {
	GDataService *service;
	GDataQueryProgressCallback progress_callback;
	gpointer progress_user_data;
	GDestroyNotify destroy_progress_user_data;
	GDataFeed *feed;
} GetRulesAsyncData;

static void
get_rules_async_data_free (GetRulesAsyncData *self)
{
	if (self->service != NULL)
		g_object_unref (self->service);
	if (self->feed != NULL)
		g_object_unref (self->feed);

	g_slice_free (GetRulesAsyncData, self);
}

static GDataFeed *
_gdata_access_handler_get_rules (GDataAccessHandler *self, GDataService *service, GCancellable *cancellable,
                                 GDataQueryProgressCallback progress_callback, gpointer progress_user_data, gboolean is_async, GError **error)
{
	GDataAccessHandlerIface *iface;
	GDataAuthorizationDomain *domain = NULL;
	GDataFeed *feed;
	GDataLink *_link;
	SoupMessage *message;

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
	feed = _gdata_feed_new_from_xml (GDATA_TYPE_FEED, message->response_body->data, message->response_body->length, GDATA_TYPE_ACCESS_RULE,
	                                 progress_callback, progress_user_data, is_async, error);
	g_object_unref (message);

	return feed;
}

static void
get_rules_thread (GSimpleAsyncResult *result, GDataAccessHandler *access_handler, GCancellable *cancellable)
{
	GError *error = NULL;
	GetRulesAsyncData *data = g_simple_async_result_get_op_res_gpointer (result);

	/* Execute the query and return */
	data->feed = _gdata_access_handler_get_rules (access_handler, data->service, cancellable, data->progress_callback, data->progress_user_data,
	                                              TRUE, &error);
	if (data->feed == NULL && error != NULL) {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
	}

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
 **/
void
gdata_access_handler_get_rules_async (GDataAccessHandler *self, GDataService *service, GCancellable *cancellable,
                                      GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                      GDestroyNotify destroy_progress_user_data,
                                      GAsyncReadyCallback callback, gpointer user_data)
{
	GSimpleAsyncResult *result;
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

	result = g_simple_async_result_new (G_OBJECT (self), callback, user_data, gdata_service_query_async);
	g_simple_async_result_set_op_res_gpointer (result, data, (GDestroyNotify) get_rules_async_data_free);
	g_simple_async_result_run_in_thread (result, (GSimpleAsyncThreadFunc) get_rules_thread, G_PRIORITY_DEFAULT, cancellable);
	g_object_unref (result);
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
 **/
GDataFeed *
gdata_access_handler_get_rules (GDataAccessHandler *self, GDataService *service, GCancellable *cancellable,
                                GDataQueryProgressCallback progress_callback, gpointer progress_user_data, GError **error)
{
	g_return_val_if_fail (GDATA_IS_ACCESS_HANDLER (self), NULL);
	g_return_val_if_fail (GDATA_IS_SERVICE (service), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return _gdata_access_handler_get_rules (self, service, cancellable, progress_callback, progress_user_data, FALSE, error);
}
