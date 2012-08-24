/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009 <philip@tecnocode.co.uk>
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

#ifndef GDATA_COMMENTABLE_H
#define GDATA_COMMENTABLE_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-feed.h>
#include <gdata/gdata-service.h>
#include <gdata/gdata-comment.h>

G_BEGIN_DECLS

#define GDATA_TYPE_COMMENTABLE			(gdata_commentable_get_type ())
#define GDATA_COMMENTABLE(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_COMMENTABLE, GDataCommentable))
#define GDATA_COMMENTABLE_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_COMMENTABLE, GDataCommentableInterface))
#define GDATA_IS_COMMENTABLE(o)			(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_COMMENTABLE))
#define GDATA_COMMENTABLE_GET_IFACE(o)		(G_TYPE_INSTANCE_GET_INTERFACE ((o), GDATA_TYPE_COMMENTABLE, GDataCommentableInterface))

/**
 * GDataCommentable:
 *
 * All the fields in the #GDataCommentable structure are private and should never be accessed directly
 *
 * Since: 0.10.0
 */
typedef struct _GDataCommentable		GDataCommentable; /* dummy typedef */

/**
 * GDataCommentableInterface:
 * @parent: the parent type
 * @comment_type: the #GType of the comment class (subclass of #GDataComment) to use for query results from this commentable object
 * @get_authorization_domain: (allow-none): a function to return the #GDataAuthorizationDomain to be used for all operations on the comments
 * belonging to this commentable object; not implementing this function is equivalent to returning %NULL from it, which signifies that operations on the
 * comments don't require authorization
 * @get_query_comments_uri: a function that returns the URI of a #GDataFeed of comments from a commentable object, or %NULL if the given commentable
 * object doesn't support commenting; free with g_free()
 * @get_insert_comment_uri: a function that returns the URI to add new comments to the commentable object, or %NULL if the given commentable object
 * doesn't support adding comments; free with g_free()
 * @is_comment_deletable: a function that returns %TRUE if the given comment may be deleted, %FALSE otherwise
 *
 * The interface structure for the #GDataCommentable interface.
 *
 * Since: 0.10.0
 */
typedef struct {
	GTypeInterface parent;

	GType comment_type;

	GDataAuthorizationDomain *(*get_authorization_domain) (GDataCommentable *self);

	gchar *(*get_query_comments_uri) (GDataCommentable *self);
	gchar *(*get_insert_comment_uri) (GDataCommentable *self, GDataComment *comment);
	gboolean (*is_comment_deletable) (GDataCommentable *self, GDataComment *comment);
} GDataCommentableInterface;

GType gdata_commentable_get_type (void) G_GNUC_CONST;

GDataFeed *gdata_commentable_query_comments (GDataCommentable *self, GDataService *service, GDataQuery *query, GCancellable *cancellable,
                                             GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                             GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_commentable_query_comments_async (GDataCommentable *self, GDataService *service, GDataQuery *query, GCancellable *cancellable,
                                             GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                             GDestroyNotify destroy_progress_user_data, GAsyncReadyCallback callback, gpointer user_data);
GDataFeed *gdata_commentable_query_comments_finish (GDataCommentable *self, GAsyncResult *result,
                                                    GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataComment *gdata_commentable_insert_comment (GDataCommentable *self, GDataService *service, GDataComment *comment_, GCancellable *cancellable,
                                                GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_commentable_insert_comment_async (GDataCommentable *self, GDataService *service, GDataComment *comment_, GCancellable *cancellable,
                                             GAsyncReadyCallback callback, gpointer user_data);
GDataComment *gdata_commentable_insert_comment_finish (GDataCommentable *self, GAsyncResult *result,
                                                       GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

gboolean gdata_commentable_delete_comment (GDataCommentable *self, GDataService *service, GDataComment *comment_, GCancellable *cancellable,
                                           GError **error);
void gdata_commentable_delete_comment_async (GDataCommentable *self, GDataService *service, GDataComment *comment_, GCancellable *cancellable,
                                             GAsyncReadyCallback callback, gpointer user_data);
gboolean gdata_commentable_delete_comment_finish (GDataCommentable *self, GAsyncResult *result, GError **error);

G_END_DECLS

#endif /* !GDATA_COMMENTABLE_H */
