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
 * When implementing the interface, classes must implement an <function>is_owner_rule</function> function.
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

/**
 * gdata_access_handler_get_rules:
 * @self: a #GDataAccessHandler
 * @service: a #GDataService
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: (scope call): a #GDataQueryProgressCallback to call when a rule is loaded, or %NULL
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
	GDataFeed *feed;
	GDataLink *_link;
	SoupMessage *message;
	guint status;

	/* TODO: async version */
	g_return_val_if_fail (GDATA_IS_ENTRY (self), NULL);
	g_return_val_if_fail (GDATA_IS_SERVICE (service), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Get the ACL URI */
	/* TODO: ETag support */
	_link = gdata_entry_look_up_link (GDATA_ENTRY (self), GDATA_LINK_ACCESS_CONTROL_LIST);
	g_assert (_link != NULL);
	message = _gdata_service_build_message (service, SOUP_METHOD_GET, gdata_link_get_uri (_link), NULL, FALSE);

	/* Send the message */
	status = _gdata_service_send_message (service, message, cancellable, error);

	if (status == SOUP_STATUS_NONE || status == SOUP_STATUS_CANCELLED) {
		/* Redirect error or cancelled */
		g_object_unref (message);
		return NULL;
	} else if (status != SOUP_STATUS_OK) {
		/* Error */
		GDataServiceClass *klass = GDATA_SERVICE_GET_CLASS (service);
		g_assert (klass->parse_error_response != NULL);
		klass->parse_error_response (service, GDATA_OPERATION_QUERY, status, message->reason_phrase, message->response_body->data,
		                             message->response_body->length, error);
		g_object_unref (message);
		return NULL;
	}

	g_assert (message->response_body->data != NULL);
	feed = _gdata_feed_new_from_xml (GDATA_TYPE_FEED, message->response_body->data, message->response_body->length, GDATA_TYPE_ACCESS_RULE,
	                                 progress_callback, progress_user_data, error);
	g_object_unref (message);

	return feed;
}
