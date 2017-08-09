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
 * @stability: Stable
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
 * The Contacts service can be manipulated using batch operations, too. See the
 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/developers_guide_protocol.html#Batch">online documentation on batch
 * operations</ulink> for more information.
 *
 * <example>
 * 	<title>Performing a Batch Operation on Contacts</title>
 * 	<programlisting>
 *	GDataContactsService *service;
 *	GDataBatchOperation *operation;
 *	GDataFeed *feed;
 *	GDataLink *batch_link;
 *	GList *i;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service *<!-- -->/
 *	service = create_contacts_service ();
 *
 *	/<!-- -->* Create the batch operation; this requires that we have done a query first so that we can get the batch link *<!-- -->/
 *	feed = do_some_query (service);
 *	batch_link = gdata_feed_look_up_link (feed, GDATA_LINK_BATCH);
 *	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_link_get_uri (batch_link));
 *	g_object_unref (feed);
 *
 *	gdata_batch_operation_add_query (operation, contact_entry_id_to_query, GDATA_TYPE_CONTACTS_CONTACT,
 *	                                 (GDataBatchOperationCallback) batch_query_cb, user_data);
 *	gdata_batch_operation_add_insertion (operation, new_entry, (GDataBatchOperationCallback) batch_insertion_cb, user_data);
 *	gdata_batch_operation_add_update (operation, old_entry, (GDataBatchOperationCallback) batch_update_cb, user_data);
 *	gdata_batch_operation_add_deletion (operation, entry_to_delete, (GDataBatchOperationCallback) batch_deletion_cb, user_data);
 *
 *	/<!-- -->* Run the batch operation and handle the results in the various callbacks *<!-- -->/
 *	gdata_test_batch_operation_run (operation, NULL, &error);
 *
 *	g_object_unref (operation);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error running batch operation: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	static void
 *	batch_query_cb (guint operation_id, GDataBatchOperationType operation_type, GDataEntry *entry, GError *error, gpointer user_data)
 *	{
 *		/<!-- -->* operation_type == GDATA_BATCH_OPERATION_QUERY *<!-- -->/
 *		/<!-- -->* Reference and do something with the returned entry. *<!-- -->/
 *	}
 *
 *	static void
 *	batch_insertion_cb (guint operation_id, GDataBatchOperationType operation_type, GDataEntry *entry, GError *error, gpointer user_data)
 *	{
 *		/<!-- -->* operation_type == GDATA_BATCH_OPERATION_INSERTION *<!-- -->/
 *		/<!-- -->* Reference and do something with the returned entry. *<!-- -->/
 *	}
 *
 *	static void
 *	batch_update_cb (guint operation_id, GDataBatchOperationType operation_type, GDataEntry *entry, GError *error, gpointer user_data)
 *	{
 *		/<!-- -->* operation_type == GDATA_BATCH_OPERATION_UPDATE *<!-- -->/
 *		/<!-- -->* Reference and do something with the returned entry. *<!-- -->/
 *	}
 *
 *	static void
 *	batch_deletion_cb (guint operation_id, GDataBatchOperationType operation_type, GDataEntry *entry, GError *error, gpointer user_data)
 *	{
 *		/<!-- -->* operation_type == GDATA_BATCH_OPERATION_DELETION, entry == NULL *<!-- -->/
 *	}
 * 	</programlisting>
 * </example>
 *
 * Since: 0.2.0
 */

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

static GList *get_authorization_domains (void);

_GDATA_DEFINE_AUTHORIZATION_DOMAIN (contacts, "cp", "https://www.google.com/m8/feeds/")
G_DEFINE_TYPE_WITH_CODE (GDataContactsService, gdata_contacts_service, GDATA_TYPE_SERVICE,
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_BATCHABLE, NULL))

static void
gdata_contacts_service_class_init (GDataContactsServiceClass *klass)
{
	GDataServiceClass *service_class = GDATA_SERVICE_CLASS (klass);
	service_class->api_version = "3";
	service_class->get_authorization_domains = get_authorization_domains;
}

static void
gdata_contacts_service_init (GDataContactsService *self)
{
	/* Nothing to see here */
}

static GList *
get_authorization_domains (void)
{
	return g_list_prepend (NULL, get_contacts_authorization_domain ());
}

/**
 * gdata_contacts_service_new:
 * @authorizer: (allow-none): a #GDataAuthorizer to authorize the service's requests, or %NULL
 *
 * Creates a new #GDataContactsService using the given #GDataAuthorizer. If @authorizer is %NULL, all requests are made as an unauthenticated user.
 *
 * Return value: a new #GDataContactsService, or %NULL; unref with g_object_unref()
 *
 * Since: 0.9.0
 */
GDataContactsService *
gdata_contacts_service_new (GDataAuthorizer *authorizer)
{
	g_return_val_if_fail (authorizer == NULL || GDATA_IS_AUTHORIZER (authorizer), NULL);

	return g_object_new (GDATA_TYPE_CONTACTS_SERVICE,
	                     "authorizer", authorizer,
	                     NULL);
}

/**
 * gdata_contacts_service_get_primary_authorization_domain:
 *
 * The primary #GDataAuthorizationDomain for interacting with Google Contacts. This will not normally need to be used, as it's used internally
 * by the #GDataContactsService methods. However, if using the plain #GDataService methods to implement custom queries or requests which libgdata
 * does not support natively, then this domain may be needed to authorize the requests.
 *
 * The domain never changes, and is interned so that pointer comparison can be used to differentiate it from other authorization domains.
 *
 * Return value: (transfer none): the service's authorization domain
 *
 * Since: 0.9.0
 */
GDataAuthorizationDomain *
gdata_contacts_service_get_primary_authorization_domain (void)
{
	return get_contacts_authorization_domain ();
}

/**
 * gdata_contacts_service_query_contacts:
 * @self: a #GDataContactsService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (scope call) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
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
 */
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
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_contacts_authorization_domain ()) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to query contacts."));
		return NULL;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.google.com/m8/feeds/contacts/default/full", NULL);
	feed = gdata_service_query (GDATA_SERVICE (self), get_contacts_authorization_domain (), request_uri, GDATA_QUERY (query),
	                            GDATA_TYPE_CONTACTS_CONTACT, cancellable, progress_callback, progress_user_data, error);
	g_free (request_uri);

	return feed;
}

/**
 * gdata_contacts_service_query_contacts_async:
 * @self: a #GDataContactsService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @destroy_progress_user_data: (allow-none): the function to call when @progress_callback will not be called any more, or %NULL. This function will be
 * called with @progress_user_data as a parameter and can be used to free any memory allocated for it.
 * @callback: a #GAsyncReadyCallback to call when the query is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Queries the service to return a list of contacts matching the given @query. @self and
 * @query are all reffed when this function is called, so can safely be unreffed after this function returns.
 *
 * For more details, see gdata_contacts_service_query_contacts(), which is the synchronous version of this function,
 * and gdata_service_query_async(), which is the base asynchronous query function.
 *
 * Since: 0.9.1
 */
void
gdata_contacts_service_query_contacts_async (GDataContactsService *self, GDataQuery *query, GCancellable *cancellable,
                                             GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                             GDestroyNotify destroy_progress_user_data,
                                             GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *request_uri;

	g_return_if_fail (GDATA_IS_CONTACTS_SERVICE (self));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	/* Ensure we're authenticated first */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_contacts_authorization_domain ()) == FALSE) {
		g_autoptr(GTask) task = NULL;

		task = g_task_new (self, cancellable, callback, user_data);
		g_task_set_source_tag (task, gdata_service_query_async);
		g_task_return_new_error (task, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED, "%s",
		                         _("You must be authenticated to query contacts."));

		return;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.google.com/m8/feeds/contacts/default/full", NULL);
	gdata_service_query_async (GDATA_SERVICE (self), get_contacts_authorization_domain (), request_uri, GDATA_QUERY (query),
	                           GDATA_TYPE_CONTACTS_CONTACT, cancellable, progress_callback, progress_user_data,
	                           destroy_progress_user_data, callback, user_data);
	g_free (request_uri);
}

/**
 * gdata_contacts_service_insert_contact:
 * @self: a #GDataContactsService
 * @contact: the #GDataContactsContact to insert
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Inserts @contact by uploading it to the online contacts service.
 *
 * For more details, see gdata_service_insert_entry().
 *
 * Return value: (transfer full): an updated #GDataContactsContact, or %NULL; unref with g_object_unref()
 *
 * Since: 0.2.0
 */
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
	entry = gdata_service_insert_entry (GDATA_SERVICE (self), get_contacts_authorization_domain (), uri, GDATA_ENTRY (contact), cancellable,
	                                    error);
	g_free (uri);

	return GDATA_CONTACTS_CONTACT (entry);
}

/**
 * gdata_contacts_service_insert_contact_async:
 * @self: a #GDataContactsService
 * @contact: the #GDataContactsContact to insert
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
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
 */
void
gdata_contacts_service_insert_contact_async (GDataContactsService *self, GDataContactsContact *contact, GCancellable *cancellable,
                                             GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *uri;

	g_return_if_fail (GDATA_IS_CONTACTS_SERVICE (self));
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (contact));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	uri = g_strconcat (_gdata_service_get_scheme (), "://www.google.com/m8/feeds/contacts/default/full", NULL);
	gdata_service_insert_entry_async (GDATA_SERVICE (self), get_contacts_authorization_domain (), uri, GDATA_ENTRY (contact), cancellable,
	                                  callback, user_data);
	g_free (uri);
}

/**
 * gdata_contacts_service_query_groups:
 * @self: a #GDataContactsService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (scope call) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
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
 */
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
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_contacts_authorization_domain ()) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to query contact groups."));
		return NULL;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.google.com/m8/feeds/groups/default/full", NULL);
	feed = gdata_service_query (GDATA_SERVICE (self), get_contacts_authorization_domain (), request_uri, GDATA_QUERY (query),
	                            GDATA_TYPE_CONTACTS_GROUP, cancellable, progress_callback, progress_user_data, error);
	g_free (request_uri);

	return feed;
}

/**
 * gdata_contacts_service_query_groups_async:
 * @self: a #GDataContactsService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @destroy_progress_user_data: (allow-none): the function to call when @progress_callback will not be called any more, or %NULL. This function will be
 * called with @progress_user_data as a parameter and can be used to free any memory allocated for it.
 * @callback: a #GAsyncReadyCallback to call when the query is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Queries the service to return a list of groups matching the given @query. @self and @query are all reffed when this function is called, so can
 * safely be unreffed after this function returns.
 *
 * For more details, see gdata_contacts_service_query_groups(), which is the synchronous version of this function, and gdata_service_query_async(),
 * which is the base asynchronous query function.
 *
 * Since: 0.9.1
 */
void
gdata_contacts_service_query_groups_async (GDataContactsService *self, GDataQuery *query, GCancellable *cancellable,
                                           GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                           GDestroyNotify destroy_progress_user_data,
                                           GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *request_uri;

	g_return_if_fail (GDATA_IS_CONTACTS_SERVICE (self));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	/* Ensure we're authenticated first */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_contacts_authorization_domain ()) == FALSE) {
		g_autoptr(GTask) task = NULL;

		task = g_task_new (self, cancellable, callback, user_data);
		g_task_set_source_tag (task, gdata_service_query_async);
		g_task_return_new_error (task, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED, "%s",
		                         _("You must be authenticated to query contact groups."));

		return;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.google.com/m8/feeds/groups/default/full", NULL);
	gdata_service_query_async (GDATA_SERVICE (self), get_contacts_authorization_domain (), request_uri, GDATA_QUERY (query),
	                           GDATA_TYPE_CONTACTS_GROUP, cancellable, progress_callback, progress_user_data,
	                           destroy_progress_user_data, callback, user_data);
	g_free (request_uri);
}

/**
 * gdata_contacts_service_insert_group:
 * @self: a #GDataContactsService
 * @group: a #GDataContactsGroup to create on the server
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Inserts a new contact group described by @group. The user must be authenticated to use this function.
 *
 * Return value: (transfer full): the inserted #GDataContactsGroup; unref with g_object_unref()
 *
 * Since: 0.7.0
 */
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

	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_contacts_authorization_domain ()) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to insert a group."));
		return NULL;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.google.com/m8/feeds/groups/default/full", NULL);
	new_group = gdata_service_insert_entry (GDATA_SERVICE (self), get_contacts_authorization_domain (), request_uri, GDATA_ENTRY (group),
	                                        cancellable, error);
	g_free (request_uri);

	return GDATA_CONTACTS_GROUP (new_group);
}

/**
 * gdata_contacts_service_insert_group_async:
 * @self: a #GDataContactsService
 * @group: the #GDataContactsGroup to insert
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
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
 */
void
gdata_contacts_service_insert_group_async (GDataContactsService *self, GDataContactsGroup *group, GCancellable *cancellable,
                                           GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *request_uri;

	g_return_if_fail (GDATA_IS_CONTACTS_SERVICE (self));
	g_return_if_fail (GDATA_IS_CONTACTS_GROUP (group));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.google.com/m8/feeds/groups/default/full", NULL);
	gdata_service_insert_entry_async (GDATA_SERVICE (self), get_contacts_authorization_domain (), request_uri, GDATA_ENTRY (group), cancellable,
	                                  callback, user_data);
	g_free (request_uri);
}
