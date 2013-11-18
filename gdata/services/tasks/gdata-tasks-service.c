/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Peteris Krisjanis 2013 <pecisk@gmail.com>
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
 * SECTION:gdata-tasks-service
 * @short_description: GData Tasks service object
 * @stability: Unstable
 * @include: gdata/services/tasks/gdata-tasks-service.h
 *
 * #GDataTasksService is a subclass of #GDataService for communicating with the API of Google Tasks. It supports querying
 * for, inserting, editing and deleting tasks from tasklists, as well as operations on the tasklists themselves.
 *
 * For more details of Google Tasks API, see the <ulink type="http" url="https://developers.google.com/google-apps/tasks/v1/reference/">
 * online documentation</ulink>.
 *
 * Since: UNRELEASED
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-tasks-service.h"
#include "gdata-service.h"
#include "gdata-private.h"
#include "gdata-query.h"
#include "gdata-feed.h"

/* Standards reference here: https://developers.google.com/google-apps/tasks/v1/reference/ */

static GList *get_authorization_domains (void);
_GDATA_DEFINE_AUTHORIZATION_DOMAIN (tasks, "tasks", "https://www.googleapis.com/auth/tasks")
G_DEFINE_TYPE (GDataTasksService, gdata_tasks_service, GDATA_TYPE_SERVICE)

static void
gdata_tasks_service_class_init (GDataTasksServiceClass *klass)
{
	GDataServiceClass *service_class = GDATA_SERVICE_CLASS (klass);
	service_class->feed_type = GDATA_TYPE_FEED;
	service_class->get_authorization_domains = get_authorization_domains;
}

static void
gdata_tasks_service_init (GDataTasksService *self)
{
	/* Nothing to see here */
}

static GList *
get_authorization_domains (void)
{
	return g_list_prepend (NULL, get_tasks_authorization_domain ());
}

/**
 * gdata_tasks_service_new:
 * @authorizer: (allow-none): a #GDataAuthorizer to authorize the service's requests, or %NULL
 *
 * Creates a new #GDataTasksService using the given #GDataAuthorizer. If @authorizer is %NULL, all requests are made as an unauthenticated user.
 *
 * Return value: a new #GDataTasksService, or %NULL; unref with g_object_unref()
 *
 * Since: UNRELEASED
 */
GDataTasksService *
gdata_tasks_service_new (GDataAuthorizer *authorizer)
{
	g_return_val_if_fail (authorizer == NULL || GDATA_IS_AUTHORIZER (authorizer), NULL);

	return g_object_new (GDATA_TYPE_TASKS_SERVICE,
	                     "authorizer", authorizer,
	                     NULL);
}

/**
 * gdata_tasks_service_get_primary_authorization_domain:
 *
 * The primary #GDataAuthorizationDomain for interacting with Google Tasks. This will not normally need to be used, as it's used internally
 * by the #GDataTasksService methods. However, if using the plain #GDataService methods to implement custom queries or requests which libgdata
 * does not support natively, then this domain may be needed to authorize the requests.
 *
 * The domain never changes, and is interned so that pointer comparison can be used to differentiate it from other authorization domains.
 *
 * Return value: (transfer none): the service's authorization domain
 *
 * Since: UNRELEASED
 */
GDataAuthorizationDomain *
gdata_tasks_service_get_primary_authorization_domain (void)
{
	return get_tasks_authorization_domain ();
}

/**
 * gdata_tasks_service_query_all_tasklists:
 * @self: a #GDataTasksService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (scope call) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: (allow-none): a #GError, or %NULL
 *
 * Queries the service to return a list of all tasklists from the authenticated account which match the given
 * @query. It will return all tasklists the user has read access to.
 *
 * For more details, see gdata_service_query().
 *
 * Return value: (transfer full): a #GDataFeed of query results; unref with g_object_unref()
 *
 * Since: UNRELEASED
 */
GDataFeed *
gdata_tasks_service_query_all_tasklists (GDataTasksService *self, GDataQuery *query, GCancellable *cancellable,
                                         GDataQueryProgressCallback progress_callback, gpointer progress_user_data, GError **error)
{
	GDataFeed *feed;
	gchar *request_uri;

	g_return_val_if_fail (GDATA_IS_TASKS_SERVICE (self), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Ensure we're authenticated first */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_tasks_authorization_domain ()) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to query all tasklists."));
		return NULL;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.googleapis.com/tasks/v1/users/@me/lists", NULL);
	feed = gdata_service_query (GDATA_SERVICE (self), get_tasks_authorization_domain (), request_uri, query, GDATA_TYPE_TASKS_TASKLIST,
	                            cancellable, progress_callback, progress_user_data, error);
	g_free (request_uri);

	return feed;
}

/**
 * gdata_tasks_service_query_all_tasklists_async:
 * @self: a #GDataTasksService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @destroy_progress_user_data: (allow-none): the function to call when @progress_callback will not be called any more, or %NULL. This function will be
 * called with @progress_user_data as a parameter and can be used to free any memory allocated for it.
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Queries the service to return a list of all tasklists from the authenticated account which match the given
 * @query. @self and @query are all reffed when this function is called, so can safely be unreffed after
 * this function returns.
 *
 * For more details, see gdata_tasks_service_query_all_tasklists(), which is the synchronous version of
 * this function, and gdata_service_query_async(), which is the base asynchronous query function.
 *
 * Since: UNRELEASED
 */
void
gdata_tasks_service_query_all_tasklists_async (GDataTasksService *self, GDataQuery *query, GCancellable *cancellable,
                                               GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                               GDestroyNotify destroy_progress_user_data,
                                               GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *request_uri;

	g_return_if_fail (GDATA_IS_TASKS_SERVICE (self));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	/* Ensure we're authenticated first */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_tasks_authorization_domain ()) == FALSE) {
		GSimpleAsyncResult *result = g_simple_async_result_new (G_OBJECT (self), callback, user_data, gdata_service_query_async);
		g_simple_async_result_set_error (result, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED, "%s",
		                                 _("You must be authenticated to query all tasklists."));
		g_simple_async_result_complete_in_idle (result);
		g_object_unref (result);

		return;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.googleapis.com/tasks/v1/users/@me/lists", NULL);
	gdata_service_query_async (GDATA_SERVICE (self), get_tasks_authorization_domain (), request_uri, query, GDATA_TYPE_TASKS_TASKLIST,
	                           cancellable, progress_callback, progress_user_data, destroy_progress_user_data, callback, user_data);
	g_free (request_uri);
}

/**
 * gdata_tasks_service_query_tasks:
 * @self: a #GDataTasksService
 * @tasklist: a #GDataTasksTasklist
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (scope call) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: (allow-none): a #GError, or %NULL
 *
 * Queries the service to return a list of tasks in the given @tasklist, which match @query.
 *
 * For more details, see gdata_service_query().
 *
 * Return value: (transfer full): a #GDataFeed of query results; unref with g_object_unref()
 *
 * Since: UNRELEASED
 */
GDataFeed *
gdata_tasks_service_query_tasks (GDataTasksService *self, GDataTasksTasklist *tasklist, GDataQuery *query, GCancellable *cancellable,
                                 GDataQueryProgressCallback progress_callback, gpointer progress_user_data, GError **error)
{
	gchar* request_uri;
	GDataFeed *feed;

	g_return_val_if_fail (GDATA_IS_TASKS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_TASKS_TASKLIST (tasklist), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Ensure we're authenticated first */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_tasks_authorization_domain ()) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to query your own tasks."));
		return NULL;
	}

	/* Should add /tasks as requested by API */
	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.googleapis.com/tasks/v1/lists/", gdata_entry_get_id (GDATA_ENTRY (tasklist)), "/tasks", NULL);
	/* Execute the query */
	feed = gdata_service_query (GDATA_SERVICE (self), get_tasks_authorization_domain (), request_uri, query, GDATA_TYPE_TASKS_TASK, cancellable,
	                            progress_callback, progress_user_data, error);
	g_free (request_uri);

	return feed;
}

/**
 * gdata_tasks_service_query_tasks_async:
 * @self: a #GDataTasksService
 * @tasklist: a #GDataTasksTasklist
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @destroy_progress_user_data: (allow-none): the function to call when @progress_callback will not be called any more, or %NULL. This function will be
 * called with @progress_user_data as a parameter and can be used to free any memory allocated for it.
 * @callback: a #GAsyncReadyCallback to call when the query is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Queries the service to return a list of tasks in the given @tasklist, which match @query. @self, @tasklist and @query are all reffed when this
 * function is called, so can safely be unreffed after this function returns.
 *
 * Get the results of the query using gdata_service_query_finish() in the @callback.
 *
 * For more details, see gdata_tasks_service_query_tasks(), which is the synchronous version of this function, and gdata_service_query_async(),
 * which is the base asynchronous query function.
 *
 * Since: UNRELEASED
 */
void
gdata_tasks_service_query_tasks_async (GDataTasksService *self, GDataTasksTasklist *tasklist, GDataQuery *query, GCancellable *cancellable,
                                       GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                       GDestroyNotify destroy_progress_user_data,
                                       GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *request_uri;

	g_return_if_fail (GDATA_IS_TASKS_SERVICE (self));
	g_return_if_fail (GDATA_IS_TASKS_TASKLIST (tasklist));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	/* Ensure we're authenticated first */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_tasks_authorization_domain ()) == FALSE) {
		GSimpleAsyncResult *result = g_simple_async_result_new (G_OBJECT (self), callback, user_data, gdata_service_query_async);
		g_simple_async_result_set_error (result, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED, "%s",
		                                 _("You must be authenticated to query your own tasks."));
		g_simple_async_result_complete_in_idle (result);
		g_object_unref (result);

		return;
	}

	/* Should add /tasks as requested by API */
	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.googleapis.com/tasks/v1/lists/", gdata_entry_get_id (GDATA_ENTRY (tasklist)), "/tasks", NULL);
	/* Execute the query */
	gdata_service_query_async (GDATA_SERVICE (self), get_tasks_authorization_domain (), request_uri, query, GDATA_TYPE_TASKS_TASK, cancellable,
	                           progress_callback, progress_user_data, destroy_progress_user_data, callback, user_data);
	g_free (request_uri);
}

/**
 * gdata_tasks_service_insert_task:
 * @self: a #GDataTasksService
 * @task: the #GDataTasksTask to insert
 * @tasklist: #GDataTasksTasklist to insert into
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: (allow-none): a #GError, or %NULL
 *
 * Inserts @task by uploading it to the online tasks service into tasklist @tasklist. It is safe to unref @tasklist after function returns.
 *
 * For more details, see gdata_service_insert_entry().
 *
 * Return value: (transfer full): an updated #GDataTasksTask, or %NULL; unref with g_object_unref()
 *
 * Since: UNRELEASED
 */
GDataTasksTask *
gdata_tasks_service_insert_task (GDataTasksService *self, GDataTasksTask *task, GDataTasksTasklist *tasklist, GCancellable *cancellable, GError **error)
{
	gchar *request_uri;
	GDataEntry *entry;

	g_return_val_if_fail (GDATA_IS_TASKS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_TASKS_TASK (task), NULL);
	g_return_val_if_fail (GDATA_IS_TASKS_TASKLIST (tasklist), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.googleapis.com/tasks/v1/lists/", gdata_entry_get_id (GDATA_ENTRY (tasklist)), "/tasks", NULL);
	entry = gdata_service_insert_entry (GDATA_SERVICE (self), get_tasks_authorization_domain (), request_uri, GDATA_ENTRY (task), cancellable, error);
	g_free (request_uri);

	return GDATA_TASKS_TASK (entry);
}

/**
 * gdata_tasks_service_insert_task_async:
 * @self: a #GDataTasksService
 * @task: the #GDataTasksTask to insert
 * @tasklist: #GDataTasksTasklist to insert into
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when insertion is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Inserts @task by uploading it to the online tasks service into tasklist @tasklist. @self, @task and @tasklist are all reffed when this function is called, so can safely be
 * unreffed after this function returns.
 *
 * @callback should call gdata_service_insert_entry_finish() to obtain a #GDataTasksTask representing the inserted task and to check for possible
 * errors.
 *
 * For more details, see gdata_tasks_service_insert_task(), which is the synchronous version of this function, and
 * gdata_service_insert_entry_async(), which is the base asynchronous insertion function.
 *
 * Since: UNRELEASED
 */
void
gdata_tasks_service_insert_task_async (GDataTasksService *self, GDataTasksTask *task, GDataTasksTasklist *tasklist, GCancellable *cancellable,
                                       GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *request_uri;

	g_return_if_fail (GDATA_IS_TASKS_SERVICE (self));
	g_return_if_fail (GDATA_IS_TASKS_TASK (task));
	g_return_if_fail (GDATA_IS_TASKS_TASKLIST (tasklist));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.googleapis.com/tasks/v1/lists/", gdata_entry_get_id (GDATA_ENTRY (tasklist)), "/tasks", NULL);
	gdata_service_insert_entry_async (GDATA_SERVICE (self), get_tasks_authorization_domain (), request_uri, GDATA_ENTRY (task), cancellable,
                                      callback, user_data);
	g_free (request_uri);
}

/**
 * gdata_tasks_service_insert_tasklist:
 * @self: a #GDataTasksService
 * @tasklist: #GDataTasksTasklist to insert
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: (allow-none): a #GError, or %NULL
 *
 * Inserts @tasklist by uploading it to the online tasks service.
 *
 * For more details, see gdata_service_insert_entry().
 *
 * Return value: (transfer full): an updated #GDataTasksTasklist, or %NULL; unref with g_object_unref()
 *
 * Since: UNRELEASED
 */
GDataTasksTasklist *
gdata_tasks_service_insert_tasklist (GDataTasksService *self, GDataTasksTasklist *tasklist, GCancellable *cancellable, GError **error)
{
	gchar *request_uri;
	GDataEntry *entry;

	g_return_val_if_fail (GDATA_IS_TASKS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_TASKS_TASKLIST (tasklist), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.googleapis.com/tasks/v1/users/@me/lists", NULL);
	entry = gdata_service_insert_entry (GDATA_SERVICE (self), get_tasks_authorization_domain (), request_uri, GDATA_ENTRY (tasklist), cancellable, error);
	g_free (request_uri);

	return GDATA_TASKS_TASKLIST (entry);
}

/**
 * gdata_tasks_service_insert_tasklist_async:
 * @self: a #GDataTasksService
 * @tasklist: #GDataTasksTasklist to insert
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when insertion is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Inserts @tasklist by uploading it to the online tasks service. @self and @tasklist are both reffed when this function is called, so can safely be
 * unreffed after this function returns.
 *
 * @callback should call gdata_service_insert_entry_finish() to obtain a #GDataTasksTasklist representing the inserted tasklist and to check for possible
 * errors.
 *
 * For more details, see gdata_tasks_service_insert_tasklist(), which is the synchronous version of this function, and
 * gdata_service_insert_entry_async(), which is the base asynchronous insertion function.
 *
 * Since: UNRELEASED
 */
void
gdata_tasks_service_insert_tasklist_async (GDataTasksService *self, GDataTasksTasklist *tasklist, GCancellable *cancellable,
                                           GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *request_uri;

	g_return_if_fail (GDATA_IS_TASKS_SERVICE (self));
	g_return_if_fail (GDATA_IS_TASKS_TASKLIST (tasklist));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.googleapis.com/tasks/v1/users/@me/lists", NULL);
	gdata_service_insert_entry_async (GDATA_SERVICE (self), get_tasks_authorization_domain (), request_uri, GDATA_ENTRY (tasklist), cancellable,
	                                  callback, user_data);
	g_free (request_uri);
}

/**
 * gdata_tasks_service_delete_task:
 * @self: a #GDataTasksService
 * @task: the #GDataTasksTask to delete
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: (allow-none): a #GError, or %NULL
 *
 * Delete @task from online tasks service.
 *
 * For more details, see gdata_service_delete_entry().
 *
 * Return value: %TRUE on success, %FALSE otherwise
 *
 * Since: UNRELEASED
 */
gboolean
gdata_tasks_service_delete_task (GDataTasksService *self, GDataTasksTask *task, GCancellable *cancellable, GError **error)
{
	g_return_val_if_fail (GDATA_IS_TASKS_SERVICE (self), FALSE);
	g_return_val_if_fail (GDATA_IS_TASKS_TASK (task), FALSE);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	return gdata_service_delete_entry (GDATA_SERVICE (self), get_tasks_authorization_domain (), GDATA_ENTRY (task), cancellable, error);
}

/**
 * gdata_tasks_service_delete_task_async:
 * @self: a #GDataTasksService
 * @task: #GDataTasksTask to delete
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when insertion is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Deletes @task from online tasks service. @self and @task are both reffed when this function is called, so can safely be
 * unreffed after this function returns.
 *
 * @callback should call gdata_service_delete_entry_finish() to finish deleting task and to check for possible
 * errors.
 *
 * For more details, see gdata_tasks_service_delete_task(), which is the synchronous version of this function, and
 * gdata_service_delete_entry_async(), which is the base asynchronous insertion function.
 *
 * Since: UNRELEASED
 */
void
gdata_tasks_service_delete_task_async (GDataTasksService *self, GDataTasksTask *task, GCancellable *cancellable,
                                       GAsyncReadyCallback callback, gpointer user_data)
{
	g_return_if_fail (GDATA_IS_TASKS_SERVICE (self));
	g_return_if_fail (GDATA_IS_TASKS_TASK (task));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	gdata_service_delete_entry_async (GDATA_SERVICE (self), get_tasks_authorization_domain (), GDATA_ENTRY (task), cancellable,
	                                  callback, user_data);
}

/**
 * gdata_tasks_service_delete_tasklist:
 * @self: a #GDataTasksService
 * @tasklist: the #GDataTasksTasklist to delete
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: (allow-none): a #GError, or %NULL
 *
 * Delete @tasklist from online tasks service.
 *
 * For more details, see gdata_service_delete_entry().
 *
 * Return value: %TRUE on success, %FALSE otherwise
 *
 * Since: UNRELEASED
 */
gboolean
gdata_tasks_service_delete_tasklist (GDataTasksService *self, GDataTasksTasklist *tasklist, GCancellable *cancellable, GError **error)
{
	g_return_val_if_fail (GDATA_IS_TASKS_SERVICE (self), FALSE);
	g_return_val_if_fail (GDATA_IS_TASKS_TASKLIST (tasklist), FALSE);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	return gdata_service_delete_entry (GDATA_SERVICE (self), get_tasks_authorization_domain (), GDATA_ENTRY (tasklist), cancellable, error);
}

/**
 * gdata_tasks_service_delete_tasklist_async:
 * @self: a #GDataTasksService
 * @tasklist: #GDataTasksTasklist to delete
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when insertion is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Deletes @tasklist from online tasks service. @self and @tasklist are both reffed when this function is called, so can safely be
 * unreffed after this function returns.
 *
 * @callback should call gdata_service_delete_entry_finish() to finish deleting tasklist and to check for possible
 * errors.
 *
 * For more details, see gdata_tasks_service_delete_tasklist(), which is the synchronous version of this function, and
 * gdata_service_delete_entry_async(), which is the base asynchronous insertion function.
 *
 * Since: UNRELEASED
 */
void
gdata_tasks_service_delete_tasklist_async (GDataTasksService *self, GDataTasksTasklist *tasklist, GCancellable *cancellable,
                                           GAsyncReadyCallback callback, gpointer user_data)
{
	g_return_if_fail (GDATA_IS_TASKS_SERVICE (self));
	g_return_if_fail (GDATA_IS_TASKS_TASKLIST (tasklist));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	gdata_service_delete_entry_async (GDATA_SERVICE (self), get_tasks_authorization_domain (), GDATA_ENTRY (tasklist), cancellable,
	                                  callback, user_data);
}

/**
 * gdata_tasks_service_update_task:
 * @self: a #GDataTasksService
 * @task: the #GDataTasksTask to update
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: (allow-none): a #GError, or %NULL
 *
 * Update @task in online tasks service.
 *
 * For more details, see gdata_service_update_entry().
 *
 * Return value: (transfer full): an updated #GDataTasksTask, or %NULL; unref with g_object_unref()
 *
 * Since: UNRELEASED
 */
GDataTasksTask *
gdata_tasks_service_update_task (GDataTasksService *self, GDataTasksTask *task, GCancellable *cancellable, GError **error)
{
	g_return_val_if_fail (GDATA_IS_TASKS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_TASKS_TASK (task), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GDATA_TASKS_TASK (gdata_service_update_entry (GDATA_SERVICE (self), get_tasks_authorization_domain (), GDATA_ENTRY (task), cancellable, error));
}

/**
 * gdata_tasks_service_update_task_async:
 * @self: a #GDataTasksService
 * @task: #GDataTasksTask to update
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when insertion is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Updates @task to online tasks service. @self and @task are both reffed when this function is called, so can safely be
 * unreffed after this function returns.
 *
 * @callback should call gdata_service_update_entry_finish() to obtain a #GDataTasksTask representing the updated task and to check for possible
 * errors.
 *
 * For more details, see gdata_tasks_service_update_task(), which is the synchronous version of this function, and
 * gdata_service_update_entry_async(), which is the base asynchronous insertion function.
 *
 * Since: UNRELEASED
 */
void
gdata_tasks_service_update_task_async (GDataTasksService *self, GDataTasksTask *task, GCancellable *cancellable,
                                       GAsyncReadyCallback callback, gpointer user_data)
{
	g_return_if_fail (GDATA_IS_TASKS_SERVICE (self));
	g_return_if_fail (GDATA_IS_TASKS_TASK (task));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	gdata_service_update_entry_async (GDATA_SERVICE (self), get_tasks_authorization_domain (), GDATA_ENTRY (task), cancellable,
	                                  callback, user_data);
}

/**
 * gdata_tasks_service_update_tasklist:
 * @self: a #GDataTasksService
 * @tasklist: the #GDataTasksTasklist to update
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: (allow-none): a #GError, or %NULL
 *
 * Update @tasklist in online tasks service.
 *
 * For more details, see gdata_service_update_entry().
 *
 * Return value: (transfer full): an updated #GDataTasksTasklist, or %NULL; unref with g_object_unref()
 *
 * Since: UNRELEASED
 */
GDataTasksTasklist *
gdata_tasks_service_update_tasklist (GDataTasksService *self, GDataTasksTasklist *tasklist, GCancellable *cancellable, GError **error)
{
	g_return_val_if_fail (GDATA_IS_TASKS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_TASKS_TASKLIST (tasklist), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return GDATA_TASKS_TASKLIST (gdata_service_update_entry (GDATA_SERVICE (self), get_tasks_authorization_domain (), GDATA_ENTRY (tasklist), cancellable, error));
}

/**
 * gdata_tasks_service_update_tasklist_async:
 * @self: a #GDataTasksService
 * @tasklist: #GDataTasksTasklist to update
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when insertion is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Updates @tasklist from online tasks service. @self and @tasklist are both reffed when this function is called, so can safely be
 * unreffed after this function returns.
 *
 * @callback should call gdata_service_update_entry_finish() to obtain a #GDataTasksTasklist representing the updated task and to check for possible
 * errors.
 *
 * For more details, see gdata_tasks_service_update_tasklist(), which is the synchronous version of this function, and
 * gdata_service_update_entry_async(), which is the base asynchronous insertion function.
 *
 * Since: UNRELEASED
 */
void
gdata_tasks_service_update_tasklist_async (GDataTasksService *self, GDataTasksTasklist *tasklist, GCancellable *cancellable,
                                           GAsyncReadyCallback callback, gpointer user_data)
{
	g_return_if_fail (GDATA_IS_TASKS_SERVICE (self));
	g_return_if_fail (GDATA_IS_TASKS_TASKLIST (tasklist));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	gdata_service_update_entry_async (GDATA_SERVICE (self), get_tasks_authorization_domain (), GDATA_ENTRY (tasklist), cancellable,
	                                  callback, user_data);
}
