/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-contacts-service
 * @short_description: GData Contacts service object
 * @stability: Unstable
 * @include: gdata/services/contacts/gdata-contacts-service.h
 *
 * #GDataContactsService is a subclass of #GDataService for communicating with the GData API of Google Contacts. It supports querying
 * for, inserting, editing and deleting contacts from a Google address book.
 *
 * For more details of Google Contacts' GData API, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/2.0/reference.html">
 * online documentation</ulink>.
 *
 * <example>
 * 	<title>Querying for Groups</title>
 * 	<programlisting>
 *	GDataContactsService *service;
 *	GDataFeed *feed;
 *	GList *i;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service *<!-- -->/
 *	service = create_contacts_service ();
 *
 *	/<!-- -->* Query for groups *<!-- -->/
 *	feed = gdata_contacts_service_query_groups (service, NULL, NULL, NULL, NULL, &error);
 *
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error querying for groups: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	/<!-- -->* Iterate through the returned groups and do something with them *<!-- -->/
 *	for (i = gdata_feed_get_entries (feed); i != NULL; i = i->next) {
 *		const gchar *system_group_id, *group_name;
 *		gboolean is_system_group;
 *		GDataContactsGroup *group = GDATA_CONTACTS_GROUP (i->data);
 *
 *		/<!-- -->* Determine whether the group's a system group. If so, you should use the system group ID to provide your application's own
 *		 * translations of the group name, as it's not translated. *<!-- -->/
 *		system_group_id = gdata_contacts_group_get_system_group_id (group);
 *		is_system_group = (system_group_id != NULL) ? TRUE : FALSE;
 *		group_name = (is_system_group == TRUE) ? get_group_name_for_system_group_id (system_group_id)
 *		                                       : gdata_entry_get_title (GDATA_ENTRY (group));
 *
 *		/<!-- -->* Do something with the group here, such as insert it into a UI. Note that system groups are not allowed to be deleted,
 *		 * so you may want to make certain parts of your UI insensitive accordingly if the group is a system group. *<!-- -->/
 *	}
 *
 *	g_object_unref (feed);
 * 	</programlisting>
 * </example>
 *
 * Since: 0.2.0
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libsoup/soup.h>
#include <string.h>

#include "gdata-contacts-service.h"
#include "gdata-batchable.h"
#include "gdata-service.h"
#include "gdata-private.h"
#include "gdata-query.h"

G_DEFINE_TYPE_WITH_CODE (GDataContactsService, gdata_contacts_service, GDATA_TYPE_SERVICE,
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_BATCHABLE, NULL))

static void
gdata_contacts_service_class_init (GDataContactsServiceClass *klass)
{
	GDataServiceClass *service_class = GDATA_SERVICE_CLASS (klass);
	service_class->service_name = "cp";
	service_class->api_version = "3";
}

static void
gdata_contacts_service_init (GDataContactsService *self)
{
	/* Nothing to see here */
}

/**
 * gdata_contacts_service_new:
 * @client_id: your application's client ID
 *
 * Creates a new #GDataContactsService. The @client_id must be unique for your application, and as registered with Google.
 *
 * Return value: a new #GDataContactsService, or %NULL
 *
 * Since: 0.2.0
 **/
GDataContactsService *
gdata_contacts_service_new (const gchar *client_id)
{
	g_return_val_if_fail (client_id != NULL, NULL);

	return g_object_new (GDATA_TYPE_CONTACTS_SERVICE,
	                     "client-id", client_id,
	                     NULL);
}

/**
 * gdata_contacts_service_query_contacts:
 * @self: a #GDataContactsService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: (scope call): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service to return a list of contacts matching the given @query.
 *
 * For more details, see gdata_service_query().
 *
 * Return value: (transfer full): a #GDataFeed of query results; unref with g_object_unref()
 *
 * Since: 0.2.0
 **/
GDataFeed *
gdata_contacts_service_query_contacts (GDataContactsService *self, GDataQuery *query, GCancellable *cancellable,
                                       GDataQueryProgressCallback progress_callback, gpointer progress_user_data, GError **error)
{
	GDataFeed *feed;
	gchar *request_uri;

	g_return_val_if_fail (GDATA_IS_CONTACTS_SERVICE (self), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Ensure we're authenticated first */
	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to query contacts."));
		return NULL;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.google.com/m8/feeds/contacts/default/full", NULL);
	feed = gdata_service_query (GDATA_SERVICE (self), request_uri, GDATA_QUERY (query),
	                            GDATA_TYPE_CONTACTS_CONTACT, cancellable, progress_callback, progress_user_data, error);
	g_free (request_uri);

	return feed;
}

/**
 * gdata_contacts_service_query_contacts_async: (skip)
 * @self: a #GDataContactsService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @callback: a #GAsyncReadyCallback to call when the query is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Queries the service to return a list of contacts matching the given @query. @self and
 * @query are all reffed when this function is called, so can safely be unreffed after this function returns.
 *
 * For more details, see gdata_contacts_service_query_contacts(), which is the synchronous version of this function,
 * and gdata_service_query_async(), which is the base asynchronous query function.
 *
 * Since: 0.2.0
 **/
void
gdata_contacts_service_query_contacts_async (GDataContactsService *self, GDataQuery *query, GCancellable *cancellable,
                                             GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                             GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *request_uri;

	g_return_if_fail (GDATA_IS_CONTACTS_SERVICE (self));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	/* Ensure we're authenticated first */
	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_simple_async_report_error_in_idle (G_OBJECT (self), callback, user_data,
		                                     GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                                     _("You must be authenticated to query contacts."));
		return;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.google.com/m8/feeds/contacts/default/full", NULL);
	gdata_service_query_async (GDATA_SERVICE (self), request_uri, GDATA_QUERY (query),
	                           GDATA_TYPE_CONTACTS_CONTACT, cancellable, progress_callback, progress_user_data, callback, user_data);
	g_free (request_uri);
}

/**
 * gdata_contacts_service_insert_contact:
 * @self: a #GDataContactsService
 * @contact: the #GDataContactsContact to insert
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Inserts @contact by uploading it to the online contacts service.
 *
 * For more details, see gdata_service_insert_entry().
 *
 * Return value: (transfer full): an updated #GDataContactsContact, or %NULL; unref with g_object_unref()
 *
 * Since: 0.2.0
 **/
GDataContactsContact *
gdata_contacts_service_insert_contact (GDataContactsService *self, GDataContactsContact *contact, GCancellable *cancellable, GError **error)
{
	gchar *uri;
	GDataEntry *entry;

	g_return_val_if_fail (GDATA_IS_CONTACTS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (contact), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	uri = g_strconcat (_gdata_service_get_scheme (), "://www.google.com/m8/feeds/contacts/default/full", NULL);
	entry = gdata_service_insert_entry (GDATA_SERVICE (self), uri, GDATA_ENTRY (contact), cancellable, error);
	g_free (uri);

	return GDATA_CONTACTS_CONTACT (entry);
}

/**
 * gdata_contacts_service_insert_contact_async:
 * @self: a #GDataContactsService
 * @contact: the #GDataContactsContact to insert
 * @cancellable: optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when insertion is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Inserts @contact by uploading it to the online contacts service. @self and @contact are both reffed when this function is called, so can safely be
 * unreffed after this function returns.
 *
 * @callback should call gdata_service_insert_entry_finish() to obtain a #GDataContactsContact representing the inserted contact and to check for
 * possible errors.
 *
 * For more details, see gdata_contacts_service_insert_contact(), which is the synchronous version of this function,
 * and gdata_service_insert_entry_async(), which is the base asynchronous insertion function.
 *
 * Since: 0.7.0
 **/
void
gdata_contacts_service_insert_contact_async (GDataContactsService *self, GDataContactsContact *contact, GCancellable *cancellable,
                                             GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *uri;

	g_return_if_fail (GDATA_IS_CONTACTS_SERVICE (self));
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (contact));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	uri = g_strconcat (_gdata_service_get_scheme (), "://www.google.com/m8/feeds/contacts/default/full", NULL);
	gdata_service_insert_entry_async (GDATA_SERVICE (self), uri, GDATA_ENTRY (contact), cancellable, callback, user_data);
	g_free (uri);
}

/**
 * gdata_contacts_service_query_groups:
 * @self: a #GDataContactsService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: (scope call): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service to return a list of groups matching the given @query.
 *
 * For more details, see gdata_service_query().
 *
 * Return value: (transfer full): a #GDataFeed of query results; unref with g_object_unref()
 *
 * Since: 0.7.0
 **/
GDataFeed *
gdata_contacts_service_query_groups (GDataContactsService *self, GDataQuery *query, GCancellable *cancellable,
                                     GDataQueryProgressCallback progress_callback, gpointer progress_user_data, GError **error)
{
	GDataFeed *feed;
	gchar *request_uri;

	g_return_val_if_fail (GDATA_IS_CONTACTS_SERVICE (self), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Ensure we're authenticated first */
	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to query contact groups."));
		return NULL;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.google.com/m8/feeds/groups/default/full", NULL);
	feed = gdata_service_query (GDATA_SERVICE (self), request_uri, GDATA_QUERY (query),
	                            GDATA_TYPE_CONTACTS_GROUP, cancellable, progress_callback, progress_user_data, error);
	g_free (request_uri);

	return feed;
}

/**
 * gdata_contacts_service_query_groups_async: (skip)
 * @self: a #GDataContactsService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @callback: a #GAsyncReadyCallback to call when the query is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Queries the service to return a list of groups matching the given @query. @self and @query are all reffed when this function is called, so can
 * safely be unreffed after this function returns.
 *
 * For more details, see gdata_contacts_service_query_groups(), which is the synchronous version of this function, and gdata_service_query_async(),
 * which is the base asynchronous query function.
 *
 * Since: 0.7.0
 **/
void
gdata_contacts_service_query_groups_async (GDataContactsService *self, GDataQuery *query, GCancellable *cancellable,
                                           GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                           GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *request_uri;

	g_return_if_fail (GDATA_IS_CONTACTS_SERVICE (self));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	/* Ensure we're authenticated first */
	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_simple_async_report_error_in_idle (G_OBJECT (self), callback, user_data,
		                                     GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                                     _("You must be authenticated to query contact groups."));
		return;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.google.com/m8/feeds/groups/default/full", NULL);
	gdata_service_query_async (GDATA_SERVICE (self), request_uri, GDATA_QUERY (query),
	                           GDATA_TYPE_CONTACTS_GROUP, cancellable, progress_callback, progress_user_data, callback, user_data);
	g_free (request_uri);
}

/**
 * gdata_contacts_service_insert_group:
 * @self: a #GDataContactsService
 * @group: a #GDataContactsGroup to create on the server
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Inserts a new contact group described by @group. The user must be authenticated to use this function.
 *
 * Return value: (transfer full): the inserted #GDataContactsGroup; unref with g_object_unref()
 *
 * Since: 0.7.0
 **/
GDataContactsGroup *
gdata_contacts_service_insert_group (GDataContactsService *self, GDataContactsGroup *group, GCancellable *cancellable, GError **error)
{
	gchar *request_uri;
	GDataEntry *new_group;

	g_return_val_if_fail (GDATA_IS_CONTACTS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_CONTACTS_GROUP (group), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (gdata_entry_is_inserted (GDATA_ENTRY (group)) == TRUE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED,
		                     _("The group has already been inserted."));
		return NULL;
	}

	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to insert a group."));
		return NULL;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.google.com/m8/feeds/groups/default/full", NULL);
	new_group = gdata_service_insert_entry (GDATA_SERVICE (self), request_uri, GDATA_ENTRY (group), cancellable, error);
	g_free (request_uri);

	return GDATA_CONTACTS_GROUP (new_group);
}

/**
 * gdata_contacts_service_insert_group_async:
 * @self: a #GDataContactsService
 * @group: the #GDataContactsGroup to insert
 * @cancellable: optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when insertion is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Inserts a new contact group described by @group. The user must be authenticated to use this function. @self and @group are both reffed when this
 * function is called, so can safely be unreffed after this function returns.
 *
 * @callback should call gdata_service_insert_entry_finish() to obtain a #GDataContactsGroup representing the inserted group and to check for possible
 * errors.
 *
 * For more details, see gdata_contacts_service_insert_group(), which is the synchronous version of this function, and
 * gdata_service_insert_entry_async(), which is the base asynchronous insertion function.
 *
 * Since: 0.7.0
 **/
void
gdata_contacts_service_insert_group_async (GDataContactsService *self, GDataContactsGroup *group, GCancellable *cancellable,
                                           GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *request_uri;

	g_return_if_fail (GDATA_IS_CONTACTS_SERVICE (self));
	g_return_if_fail (GDATA_IS_CONTACTS_GROUP (group));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.google.com/m8/feeds/groups/default/full", NULL);
	gdata_service_insert_entry_async (GDATA_SERVICE (self), request_uri, GDATA_ENTRY (group), cancellable, callback, user_data);
	g_free (request_uri);
}
