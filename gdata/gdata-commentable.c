/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2008–2010 <philip@tecnocode.co.uk>
 * Copyright (C) Richard Schwarting 2010 <aquarichy@gmail.com>
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
 * SECTION:gdata-commentable
 * @short_description: GData commentable interface
 * @stability: Stable
 * @include: gdata/gdata-commentable.h
 *
 * #GDataCommentable is an interface which can be implemented by commentable objects: objects which support having comments added to them by users,
 * such as videos and photos.
 *
 * Comments may be queried, added and deleted. Note that they may not be edited.
 *
 * #GDataCommentable objects may not support all operations on comments, on an instance-by-instance basis (i.e. it's an invalid assumption that if,
 * for example, one #GDataYouTubeVideo doesn't support adding comments all other #GDataYouTubeVideos don't support adding comments either).
 * Specific documentation for a particular type of #GDataCommentable may state otherwise, though.
 *
 * <example>
 * 	<title>Querying for Comments</title>
 * 	<programlisting>
 *	GDataService *service;
 *	GDataCommentable *commentable;
 *
 *	/<!-- -->* Create a service *<!-- -->/
 *	service = create_service ();
 *
 *	/<!-- -->* Retrieve the GDataCommentable which is going to be queried. This may be, for example, a GDataYouTubeVideo. *<!-- -->/
 *	commentable = get_commentable ();
 *
 *	/<!-- -->* Start the async. query for the comments. *<!-- -->/
 *	gdata_commentable_query_comments_async (commentable, service, NULL, NULL, NULL, NULL, NULL, (GAsyncReadyCallback) query_comments_cb, NULL);
 *
 *	g_object_unref (service);
 *	g_object_unref (commentable);
 *
 *	static void
 *	query_comments_cb (GDataCommentable *commentable, GAsyncResult *result, gpointer user_data)
 *	{
 *		GDataFeed *comment_feed;
 *		GList *comments, *i;
 *		GError *error = NULL;
 *
 *		comment_feed = gdata_commentable_query_comments_finish (commentable, result, &error);
 *
 *		if (error != NULL) {
 *			/<!-- -->* Error! *<!-- -->/
 *			g_error ("Error querying comments: %s", error->message);
 *			g_error_free (error);
 *			return;
 *		}
 *
 *		/<!-- -->* Examine the comments. *<!-- -->/
 *		comments = gdata_feed_get_entries (comment_feed);
 *		for (i = comments; i != NULL; i = i->next) {
 *			/<!-- -->* Note that this will actually be a subclass of GDataComment,
 *			 * such as GDataYouTubeComment or GDataPicasaWebComment. *<!-- -->/
 *			GDataComment *comment = GDATA_COMMENT (i->data);
 *			GDataAuthor *author;
 *
 *			/<!-- -->* Note that in practice it might not always be safe to assume that a comment always has an author. *<!-- -->/
 *			author = GDATA_AUTHOR (gdata_entry_get_authors (GDATA_ENTRY (comment))->data);
 *
 *			g_message ("Comment by %s (%s): %s",
 *			           gdata_author_get_name (author),
 *			           gdata_author_get_uri (author),
 *			           gdata_entry_get_content (GDATA_ENTRY (comment)));
 *		}
 *
 *		g_object_unref (comment_feed);
 *	}
 * 	</programlisting>
 * </example>
 *
 * Since: 0.10.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-commentable.h"
#include "gdata-service.h"

G_DEFINE_INTERFACE (GDataCommentable, gdata_commentable, GDATA_TYPE_ENTRY)

static void
gdata_commentable_default_init (GDataCommentableInterface *iface)
{
	iface->comment_type = G_TYPE_INVALID;
	iface->get_authorization_domain = NULL;
	iface->get_query_comments_uri = NULL;
	iface->get_insert_comment_uri = NULL;
	iface->is_comment_deletable = NULL;
}

static GType
get_comment_type (GDataCommentableInterface *iface)
{
	GType comment_type;

	comment_type = iface->comment_type;
	g_assert (g_type_is_a (comment_type, GDATA_TYPE_COMMENT) == TRUE);

	return comment_type;
}

/**
 * gdata_commentable_query_comments:
 * @self: a #GDataCommentable
 * @service: a #GDataService representing the service with which the object's comments will be manipulated
 * @query: (allow-none): a #GDataQuery with query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (scope call) (closure progress_user_data): a #GDataQueryProgressCallback to call when a comment is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Retrieves a #GDataFeed containing the #GDataComments representing the comments on the #GDataCommentable which match the given @query.
 *
 * If the #GDataCommentable doesn't support commenting, %NULL will be returned and @error will be set to %GDATA_SERVICE_ERROR_FORBIDDEN. This is in
 * contrast to if it does support commenting but hasn't had any comments added yet, in which case an empty #GDataFeed will be returned and no error
 * will be set.
 *
 * Return value: (transfer full) (allow-none): a #GDataFeed of #GDataComments, or %NULL; unref with g_object_unref()
 *
 * Since: 0.10.0
 */
GDataFeed *
gdata_commentable_query_comments (GDataCommentable *self, GDataService *service, GDataQuery *query,
                                  GCancellable *cancellable, GDataQueryProgressCallback progress_callback,
                                  gpointer progress_user_data, GError **error)
{
	GDataCommentableInterface *iface;
	gchar *uri;
	GDataFeed *feed;
	GDataAuthorizationDomain *domain = NULL;

	g_return_val_if_fail (GDATA_IS_COMMENTABLE (self), NULL);
	g_return_val_if_fail (GDATA_IS_SERVICE (service), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	iface = GDATA_COMMENTABLE_GET_IFACE (self);

	/* Get the comment feed URI. */
	g_assert (iface->get_query_comments_uri != NULL);
	uri = iface->get_query_comments_uri (self);

	/* The URI can be NULL when no comments and thus no feedLink is present in a GDataCommentable */
	if (uri == NULL) {
		/* Translators: This is an error message for if a user attempts to retrieve comments from an entry (such as a video) which doesn't
		 * support comments. */
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_FORBIDDEN, _("This entry does not support comments."));
		return NULL;
	}

	/* Get the authorisation domain. */
	if (iface->get_authorization_domain != NULL) {
		domain = iface->get_authorization_domain (self);
	}

	/* Get the comment feed. */
	feed = gdata_service_query (service, domain, uri, query, get_comment_type (iface), cancellable, progress_callback, progress_user_data, error);
	g_free (uri);

	return feed;
}

static void
query_comments_async_cb (GDataService *service, GAsyncResult *service_result, gpointer user_data)
{
	g_autoptr(GTask) commentable_task = G_TASK (user_data);
	g_autoptr(GDataFeed) feed = NULL;
	g_autoptr(GError) error = NULL;

	feed = gdata_service_query_finish (service, service_result, &error);

	if (error != NULL)
		g_task_return_error (commentable_task, g_steal_pointer (&error));
	else
		g_task_return_pointer (commentable_task, g_steal_pointer (&feed), g_object_unref);
}

/**
 * gdata_commentable_query_comments_async:
 * @self: a #GDataCommentable
 * @service: a #GDataService representing the service with which the object's comments will be manipulated
 * @query: (allow-none): a #GDataQuery with query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (scope notified) (closure progress_user_data): a #GDataQueryProgressCallback to call when a comment is loaded,
 * or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @destroy_progress_user_data: (allow-none): a function to call when @progress_callback will not be called any more, or %NULL; this function will be
 * called with @progress_user_data as a parameter and can be used to free any memory allocated for it
 * @callback: a #GAsyncReadyCallback to call when the query is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Retrieves a #GDataFeed containing the #GDataComments representing the comments on the #GDataCommentable which match the given @query.
 * @self, @service and @query are all reffed when this method is called, so can safely be freed after this method returns.
 *
 * For more details, see gdata_commentable_query_comments(), which is the synchronous version of this method.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_commentable_query_comments_finish() to get the results of the
 * operation.
 *
 * Since: 0.10.0
 */
void
gdata_commentable_query_comments_async (GDataCommentable *self, GDataService *service, GDataQuery *query, GCancellable *cancellable,
                                        GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                        GDestroyNotify destroy_progress_user_data, GAsyncReadyCallback callback, gpointer user_data)
{
	GDataCommentableInterface *iface;
	gchar *uri;
	g_autoptr(GTask) task = NULL;
	GDataAuthorizationDomain *domain = NULL;

	g_return_if_fail (GDATA_IS_COMMENTABLE (self));
	g_return_if_fail (GDATA_IS_SERVICE (service));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	iface = GDATA_COMMENTABLE_GET_IFACE (self);

	/* Get the comment feed URI. */
	g_assert (iface->get_query_comments_uri != NULL);
	uri = iface->get_query_comments_uri (self);

	/* Build the async result. */
	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_commentable_query_comments_async);

	/* The URI can be NULL when no comments and thus no feedLink is present in a GDataCommentable */
	if (uri == NULL) {
		g_task_return_new_error (task, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_FORBIDDEN,
		                         /* Translators: This is an error message for if a user attempts to retrieve comments from an entry
		                          * (such as a video) which doesn't support comments. */
		                         _("This entry does not support comments."));
		return;
	}

	/* Get the authorisation domain. */
	if (iface->get_authorization_domain != NULL) {
		domain = iface->get_authorization_domain (self);
	}

	/* Get the comment feed. */
	gdata_service_query_async (service, domain, uri, query, get_comment_type (iface), cancellable, progress_callback, progress_user_data,
	                           destroy_progress_user_data, (GAsyncReadyCallback) query_comments_async_cb, g_object_ref (task));
	g_free (uri);
}

/**
 * gdata_commentable_query_comments_finish:
 * @self: a #GDataCommentable
 * @result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous comment query operation started with gdata_commentable_query_comments_async().
 *
 * Return value: (transfer full) (allow-none): a #GDataFeed of #GDataComments, or %NULL; unref with g_object_unref()
 *
 * Since: 0.10.0
 */
GDataFeed *
gdata_commentable_query_comments_finish (GDataCommentable *self, GAsyncResult *result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_COMMENTABLE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);
	g_return_val_if_fail (g_task_is_valid (result, self), NULL);
	g_return_val_if_fail (g_async_result_is_tagged (result, gdata_commentable_query_comments_async), NULL);

	return g_task_propagate_pointer (G_TASK (result), error);
}

/**
 * gdata_commentable_insert_comment:
 * @self: a #GDataCommentable
 * @service: a #GDataService with which the comment will be added
 * @comment_: a new comment to be added to the #GDataCommentable
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Adds @comment to the #GDataCommentable.
 *
 * If the #GDataCommentable doesn't support commenting, %NULL will be returned and @error will be set to %GDATA_SERVICE_ERROR_FORBIDDEN.
 *
 * Return value: (transfer full) (allow-none): the added #GDataComment, or %NULL; unref with g_object_unref()
 *
 * Since: 0.10.0
 */
GDataComment *
gdata_commentable_insert_comment (GDataCommentable *self, GDataService *service, GDataComment *comment_, GCancellable *cancellable, GError **error)
{
	GDataCommentableInterface *iface;
	gchar *uri;
	GDataComment *new_comment;
	GDataAuthorizationDomain *domain = NULL;

	g_return_val_if_fail (GDATA_IS_COMMENTABLE (self), NULL);
	g_return_val_if_fail (GDATA_IS_SERVICE (service), NULL);
	g_return_val_if_fail (GDATA_IS_COMMENT (comment_), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	iface = GDATA_COMMENTABLE_GET_IFACE (self);

	g_return_val_if_fail (g_type_is_a (G_OBJECT_TYPE (comment_), get_comment_type (iface)) == TRUE, NULL);

	/* Get the upload URI. */
	g_assert (iface->get_insert_comment_uri != NULL);
	uri = iface->get_insert_comment_uri (self, comment_);

	if (uri == NULL) {
		/* Translators: This is an error message for if a user attempts to add a comment to an entry (such as a video) which doesn't support
		 * comments. */
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_FORBIDDEN, _("Comments may not be added to this entry."));
		return NULL;
	}

	/* Get the authorisation domain. */
	if (iface->get_authorization_domain != NULL) {
		domain = iface->get_authorization_domain (self);
	}

	/* Add the comment. */
	new_comment = GDATA_COMMENT (gdata_service_insert_entry (service, domain, uri, GDATA_ENTRY (comment_), cancellable, error));

	g_free (uri);

	return new_comment;
}

static void
insert_comment_async_cb (GDataService *service, GAsyncResult *service_result, gpointer user_data)
{
	g_autoptr(GTask) commentable_task = G_TASK (user_data);
	g_autoptr(GDataEntry) new_comment = NULL;
	g_autoptr(GError) error = NULL;

	new_comment = gdata_service_insert_entry_finish (service, service_result, &error);

	if (error != NULL)
		g_task_return_error (commentable_task, g_steal_pointer (&error));
	else
		g_task_return_pointer (commentable_task, g_steal_pointer (&new_comment), g_object_unref);
}

/**
 * gdata_commentable_insert_comment_async:
 * @self: a #GDataCommentable
 * @service: a #GDataService with which the comment will be added
 * @comment_: a new comment to be added to the #GDataCommentable
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the operation is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Adds @comment to the #GDataCommentable. @self, @service and @comment_ are all reffed when this method is called, so can safely be freed after this
 * method returns.
 *
 * For more details, see gdata_commentable_insert_comment(), which is the synchronous version of this method.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_commentable_insert_comment_finish() to get the results of the
 * operation.
 *
 * Since: 0.10.0
 */
void
gdata_commentable_insert_comment_async (GDataCommentable *self, GDataService *service, GDataComment *comment_, GCancellable *cancellable,
                                        GAsyncReadyCallback callback, gpointer user_data)
{
	GDataCommentableInterface *iface;
	g_autofree gchar *uri = NULL;
	g_autoptr(GTask) task = NULL;
	GDataAuthorizationDomain *domain = NULL;

	g_return_if_fail (GDATA_IS_COMMENTABLE (self));
	g_return_if_fail (GDATA_IS_SERVICE (service));
	g_return_if_fail (GDATA_IS_COMMENT (comment_));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	iface = GDATA_COMMENTABLE_GET_IFACE (self);

	g_return_if_fail (g_type_is_a (G_OBJECT_TYPE (comment_), get_comment_type (iface)) == TRUE);

	/* Get the upload URI. */
	g_assert (iface->get_insert_comment_uri != NULL);
	uri = iface->get_insert_comment_uri (self, comment_);

	/* Build the async result. */
	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_commentable_insert_comment_async);

	/* The URI can be NULL when no comments and thus no feedLink is present in a GDataCommentable */
	if (uri == NULL) {
		g_task_return_new_error (task, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_FORBIDDEN,
		                         /* Translators: This is an error message for if a user attempts to add a comment to an entry
		                          * (such as a video) which doesn't support comments. */
		                         _("Comments may not be added to this entry."));
		return;
	}

	/* Get the authorisation domain. */
	if (iface->get_authorization_domain != NULL) {
		domain = iface->get_authorization_domain (self);
	}

	/* Add the comment. */
	gdata_service_insert_entry_async (service, domain, uri, GDATA_ENTRY (comment_), cancellable, (GAsyncReadyCallback) insert_comment_async_cb,
	                                  g_object_ref (task));
}

/**
 * gdata_commentable_insert_comment_finish:
 * @self: a #GDataCommentable
 * @result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous comment insertion operation started with gdata_commentable_insert_comment_async().
 *
 * Return value: (transfer full) (allow-none): the added #GDataComment, or %NULL; unref with g_object_unref()
 *
 * Since: 0.10.0
 */
GDataComment *
gdata_commentable_insert_comment_finish (GDataCommentable *self, GAsyncResult *result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_COMMENTABLE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);
	g_return_val_if_fail (g_task_is_valid (result, self), NULL);
	g_return_val_if_fail (g_async_result_is_tagged (result, gdata_commentable_insert_comment_async), NULL);

	return g_task_propagate_pointer (G_TASK (result), error);
}

/**
 * gdata_commentable_delete_comment:
 * @self: a #GDataCommentable
 * @service: a #GDataService with which the comment will be deleted
 * @comment_: a comment to be deleted
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Deletes @comment from the #GDataCommentable.
 *
 * If the given @comment isn't deletable (either because the service doesn't support deleting comments at all, or because this particular comment
 * is not deletable due to having insufficient permissions), %GDATA_SERVICE_ERROR_FORBIDDEN will be set in @error and %FALSE will be returned.
 *
 * Return value: %TRUE if the comment was successfully deleted, %FALSE otherwise
 *
 * Since: 0.10.0
 */
gboolean
gdata_commentable_delete_comment (GDataCommentable *self, GDataService *service, GDataComment *comment_, GCancellable *cancellable, GError **error)
{
	GDataCommentableInterface *iface;
	GDataAuthorizationDomain *domain = NULL;

	g_return_val_if_fail (GDATA_IS_COMMENTABLE (self), FALSE);
	g_return_val_if_fail (GDATA_IS_SERVICE (service), FALSE);
	g_return_val_if_fail (GDATA_IS_COMMENT (comment_), FALSE);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	iface = GDATA_COMMENTABLE_GET_IFACE (self);

	g_return_val_if_fail (g_type_is_a (G_OBJECT_TYPE (comment_), get_comment_type (iface)) == TRUE, FALSE);

	g_assert (iface->is_comment_deletable != NULL);
	if (iface->is_comment_deletable (self, comment_) == FALSE) {
		/* Translators: This is an error message for if a user attempts to delete a comment they're not allowed to delete. */
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_FORBIDDEN, _("This comment may not be deleted."));
		return FALSE;
	}

	/* Get the authorisation domain. */
	if (iface->get_authorization_domain != NULL) {
		domain = iface->get_authorization_domain (self);
	}

	/* Delete the comment. */
	return gdata_service_delete_entry (service, domain, GDATA_ENTRY (comment_), cancellable, error);
}

static void
delete_comment_async_cb (GDataService *service, GAsyncResult *service_result, gpointer user_data)
{
	g_autoptr(GTask) commentable_task = G_TASK (user_data);
	g_autoptr(GError) error = NULL;

	if (!gdata_service_delete_entry_finish (service, service_result, &error))
		g_task_return_error (commentable_task, g_steal_pointer (&error));
	else
		g_task_return_boolean (commentable_task, TRUE);
}

/**
 * gdata_commentable_delete_comment_async:
 * @self: a #GDataCommentable
 * @service: a #GDataService with which the comment will be deleted
 * @comment_: a comment to be deleted
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the operation is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Deletes @comment from the #GDataCommentable. @self, @service and @comment_ are all reffed when this method is called, so can safely be freed after
 * this method returns.
 *
 * For more details, see gdata_commentable_delete_comment(), which is the synchronous version of this method.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_commentable_delete_comment_finish() to get the results of the
 * operation.
 *
 * Since: 0.10.0
 */
void
gdata_commentable_delete_comment_async (GDataCommentable *self, GDataService *service, GDataComment *comment_, GCancellable *cancellable,
                                        GAsyncReadyCallback callback, gpointer user_data)
{
	GDataCommentableInterface *iface;
	g_autoptr(GTask) task = NULL;
	GDataAuthorizationDomain *domain = NULL;

	g_return_if_fail (GDATA_IS_COMMENTABLE (self));
	g_return_if_fail (GDATA_IS_SERVICE (service));
	g_return_if_fail (GDATA_IS_COMMENT (comment_));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	iface = GDATA_COMMENTABLE_GET_IFACE (self);

	g_return_if_fail (g_type_is_a (G_OBJECT_TYPE (comment_), get_comment_type (iface)) == TRUE);

	/* Build the async result. */
	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_commentable_delete_comment_async);

	g_assert (iface->is_comment_deletable != NULL);
	if (iface->is_comment_deletable (self, comment_) == FALSE) {
		g_task_return_new_error (task, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_FORBIDDEN,
		                         /* Translators: This is an error message for if a user attempts to delete a comment they're not allowed to delete. */
		                         _("This comment may not be deleted."));
		return;
	}

	/* Get the authorisation domain. */
	if (iface->get_authorization_domain != NULL) {
		domain = iface->get_authorization_domain (self);
	}

	/* Delete the comment. */
	gdata_service_delete_entry_async (service, domain, GDATA_ENTRY (comment_), cancellable, (GAsyncReadyCallback) delete_comment_async_cb, g_object_ref (task));
}

/**
 * gdata_commentable_delete_comment_finish:
 * @self: a #GDataCommentable
 * @result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous comment deletion operation started with gdata_commentable_delete_comment_async().
 *
 * Return value: %TRUE if the comment was successfully deleted, %FALSE otherwise
 *
 * Since: 0.10.0
 */
gboolean
gdata_commentable_delete_comment_finish (GDataCommentable *self, GAsyncResult *result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_COMMENTABLE (self), FALSE);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	g_return_val_if_fail (g_task_is_valid (result, self), FALSE);
	g_return_val_if_fail (g_async_result_is_tagged (result, gdata_commentable_delete_comment_async), FALSE);

	return g_task_propagate_boolean (G_TASK (result), error);
}
