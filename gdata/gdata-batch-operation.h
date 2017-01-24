/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2010 <philip@tecnocode.co.uk>
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

#ifndef GDATA_BATCH_OPERATION_H
#define GDATA_BATCH_OPERATION_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-service.h>
#include <gdata/gdata-entry.h>
#include <gdata/gdata-authorization-domain.h>

G_BEGIN_DECLS

/**
 * GDATA_LINK_BATCH:
 *
 * The relation type URI for the batch operation URI for a given #GDataFeed.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/gdata/docs/batch.html#Submit_HTTP">GData specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_LINK_BATCH "http://schemas.google.com/g/2005#batch"

/**
 * GDataBatchOperationType:
 * @GDATA_BATCH_OPERATION_QUERY: a query operation
 * @GDATA_BATCH_OPERATION_INSERTION: an insertion operation
 * @GDATA_BATCH_OPERATION_UPDATE: an update operation
 * @GDATA_BATCH_OPERATION_DELETION: a deletion operation
 *
 * Indicates which type of batch operation caused the current #GDataBatchOperationCallback to be called.
 *
 * Since: 0.7.0
 */
typedef enum {
	GDATA_BATCH_OPERATION_QUERY = 0,
	GDATA_BATCH_OPERATION_INSERTION,
	GDATA_BATCH_OPERATION_UPDATE,
	GDATA_BATCH_OPERATION_DELETION
} GDataBatchOperationType;

/**
 * GDataBatchOperationCallback:
 * @operation_id: the operation ID returned from gdata_batch_operation_add_*()
 * @operation_type: the type of operation which was requested
 * @entry: the result of the operation, or %NULL
 * @error: a #GError describing any error which occurred, or %NULL
 * @user_data: user data passed to the callback
 *
 * Callback function called once for each operation in a batch operation run. The operation is identified by @operation_id and @operation_type (where
 * @operation_id is the ID returned by the relevant call to gdata_batch_operation_add_query(), gdata_batch_operation_add_insertion(),
 * gdata_batch_operation_add_update() or gdata_batch_operation_add_deletion(), and @operation_type shows which one of the above was called).
 *
 * If the operation was successful, the resulting #GDataEntry will be passed in as @entry, and @error will be %NULL. Otherwise, @entry will be %NULL
 * and a descriptive error will be in @error. If @operation_type is %GDATA_BATCH_OPERATION_DELETION, @entry will always be %NULL, and @error will be
 * %NULL or non-%NULL as appropriate.
 *
 * If the callback code needs to retain a copy of @entry, it must be referenced (with g_object_ref()). Similarly, @error is owned by the calling code,
 * and must not be freed.
 *
 * The callback is called in the main thread, and there is no guarantee on the order in which the callbacks for the operations in a run are executed,
 * or whether they will be called in a timely manner. It is, however, guaranteed that they will all be called before the #GAsyncReadyCallback which
 * signals the completion of the run (if initiated with gdata_batch_operation_run_async()) is called; or gdata_batch_operation_run() returns (if
 * initiated synchronously).
 *
 * Since: 0.7.0
 */
typedef void (*GDataBatchOperationCallback) (guint operation_id, GDataBatchOperationType operation_type, GDataEntry *entry,
                                             GError *error, gpointer user_data);

#define GDATA_TYPE_BATCH_OPERATION		(gdata_batch_operation_get_type ())
#define GDATA_BATCH_OPERATION(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_BATCH_OPERATION, GDataBatchOperation))
#define GDATA_BATCH_OPERATION_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_BATCH_OPERATION, GDataBatchOperationClass))
#define GDATA_IS_BATCH_OPERATION(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_BATCH_OPERATION))
#define GDATA_IS_BATCH_OPERATION_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_BATCH_OPERATION))
#define GDATA_BATCH_OPERATION_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_BATCH_OPERATION, GDataBatchOperationClass))

typedef struct _GDataBatchOperationPrivate	GDataBatchOperationPrivate;

/**
 * GDataBatchOperation:
 *
 * All the fields in the #GDataBatchOperation structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	GObject parent;
	GDataBatchOperationPrivate *priv;
} GDataBatchOperation;

/**
 * GDataBatchOperationClass:
 *
 * All the fields in the #GDataBatchOperationClass structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	/*< private >*/
	GObjectClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
	void (*_g_reserved2) (void);
	void (*_g_reserved3) (void);
	void (*_g_reserved4) (void);
	void (*_g_reserved5) (void);
} GDataBatchOperationClass;

GType gdata_batch_operation_get_type (void) G_GNUC_CONST;

GDataService *gdata_batch_operation_get_service (GDataBatchOperation *self) G_GNUC_PURE;
GDataAuthorizationDomain *gdata_batch_operation_get_authorization_domain (GDataBatchOperation *self) G_GNUC_PURE;
const gchar *gdata_batch_operation_get_feed_uri (GDataBatchOperation *self) G_GNUC_PURE;

guint gdata_batch_operation_add_query (GDataBatchOperation *self, const gchar *id, GType entry_type,
                                       GDataBatchOperationCallback callback, gpointer user_data);
guint gdata_batch_operation_add_insertion (GDataBatchOperation *self, GDataEntry *entry, GDataBatchOperationCallback callback, gpointer user_data);
guint gdata_batch_operation_add_update (GDataBatchOperation *self, GDataEntry *entry, GDataBatchOperationCallback callback, gpointer user_data);
guint gdata_batch_operation_add_deletion (GDataBatchOperation *self, GDataEntry *entry, GDataBatchOperationCallback callback, gpointer user_data);

gboolean gdata_batch_operation_run (GDataBatchOperation *self, GCancellable *cancellable, GError **error);
void gdata_batch_operation_run_async (GDataBatchOperation *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
gboolean gdata_batch_operation_run_finish (GDataBatchOperation *self, GAsyncResult *async_result, GError **error);

G_END_DECLS

#endif /* !GDATA_BATCH_OPERATION_H */
