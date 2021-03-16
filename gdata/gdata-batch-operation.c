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

/**
 * SECTION:gdata-batch-operation
 * @short_description: GData batch operation object
 * @stability: Stable
 * @include: gdata/gdata-batch-operation.h
 *
 * #GDataBatchOperation is a transient standalone class which represents and handles a single batch operation request to a service. To make a batch
 * operation request: create a new #GDataBatchOperation; add the required queries, insertions, updates and deletions to the operation using
 * gdata_batch_operation_add_query(), gdata_batch_operation_add_insertion(), gdata_batch_operation_add_update() and
 * gdata_batch_operation_add_deletion(), respectively; run the request with gdata_batch_operation_run() or gdata_batch_operation_run_async(); and
 * handle the results in the callback functions which are invoked by the operation as the results are received and parsed.
 *
 * If authorization is required for any of the requests in the batch operation, the #GDataService set in #GDataBatchOperation:service must have
 * a #GDataAuthorizer set as its #GDataService:authorizer property, and that authorizer must be authorized for the #GDataAuthorizationDomain set
 * in #GDataBatchOperation:authorization-domain. It's not possible for requests in a single batch operation to be authorized under multiple domains;
 * in that case, the requests must be split up across several batch operations using different authorization domains.
 *
 * If all of the requests in the batch operation don't require authorization (i.e. they all operate on public data; see the documentation for the
 * #GDataService subclass in question's operations for details of which require authorization), #GDataBatchOperation:authorization-domain can be set
 * to %NULL to save the overhead of sending authorization data to the online service.
 *
 * <example>
 * 	<title>Running a Synchronous Operation</title>
 * 	<programlisting>
 *	guint op_id, op_id2;
 *	GDataBatchOperation *operation;
 *	GDataContactsContact *contact;
 *	GDataService *service;
 *	GDataAuthorizationDomain *domain;
 *
 *	service = create_contacts_service ();
 *	domain = get_authorization_domain_from_service (service);
 *	contact = create_new_contact ();
 *	batch_link = gdata_feed_look_up_link (contacts_feed, GDATA_LINK_BATCH);
 *
 *	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), domain, gdata_link_get_uri (batch_link));
 *
 *	/<!-- -->* Add to the operation to insert a new contact and query for another one *<!-- -->/
 *	op_id = gdata_batch_operation_add_insertion (operation, GDATA_ENTRY (contact), insertion_cb, user_data);
 *	op_id2 = gdata_batch_operation_add_query (operation, gdata_entry_get_id (other_contact), GDATA_TYPE_CONTACTS_CONTACT, query_cb, user_data);
 *
 *	g_object_unref (contact);
 *	g_object_unref (domain);
 *	g_object_unref (service);
 *
 *	/<!-- -->* Run the operations in a blocking fashion. Ideally, check and free the error as appropriate after running the operation. *<!-- -->/
 *	gdata_batch_operation_run (operation, NULL, &error);
 *
 *	g_object_unref (operation);
 *
 *	static void
 *	insertion_cb (guint operation_id, GDataBatchOperationType operation_type, GDataEntry *entry, GError *error, gpointer user_data)
 *	{
 *		/<!-- -->* operation_id == op_id, operation_type == GDATA_BATCH_OPERATION_INSERTION *<!-- -->/
 *
 *		/<!-- -->* Process the new inserted entry, ideally after checking for errors. Note that the entry should be reffed if it needs to stay
 *		 * alive after execution of the callback finishes. *<!-- -->/
 *		process_inserted_entry (entry, user_data);
 *	}
 *
 *	static void
 *	query_cb (guint operation_id, GDataBatchOperationType operation_type, GDataEntry *entry, GError *error, gpointer user_data)
 *	{
 *		/<!-- -->* operation_id == op_id2, operation_type == GDATA_BATCH_OPERATION_QUERY *<!-- -->/
 *
 *		/<!-- -->* Process the results of the query, ideally after checking for errors. Note that the entry should be reffed if it needs to
 *		 * stay alive after execution of the callback finishes. *<!-- -->/
 *		process_queried_entry (entry, user_data);
 *	}
 * 	</programlisting>
 * </example>
 *
 * Since: 0.7.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-batch-operation.h"
#include "gdata-batch-feed.h"
#include "gdata-batchable.h"
#include "gdata-private.h"
#include "gdata-batch-private.h"

static void operation_free (BatchOperation *op);

static void gdata_batch_operation_dispose (GObject *object);
static void gdata_batch_operation_finalize (GObject *object);
static void gdata_batch_operation_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_batch_operation_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

struct _GDataBatchOperationPrivate {
	GDataService *service;
	GDataAuthorizationDomain *authorization_domain;
	gchar *feed_uri;
	GHashTable *operations;
	guint next_id; /* next available operation ID */
	gboolean has_run; /* TRUE if the operation has been run already (though it does not necessarily have to have finished running) */
	gboolean is_async; /* TRUE if the operation was run with *_run_async(); FALSE if run with *_run() */
};

enum {
	PROP_SERVICE = 1,
	PROP_FEED_URI,
	PROP_AUTHORIZATION_DOMAIN,
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataBatchOperation, gdata_batch_operation, G_TYPE_OBJECT)

static void
gdata_batch_operation_class_init (GDataBatchOperationClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose = gdata_batch_operation_dispose;
	gobject_class->finalize = gdata_batch_operation_finalize;
	gobject_class->get_property = gdata_batch_operation_get_property;
	gobject_class->set_property = gdata_batch_operation_set_property;

	/**
	 * GDataBatchOperation:service:
	 *
	 * The service this batch operation is attached to.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_SERVICE,
	                                 g_param_spec_object ("service",
	                                                      "Service", "The service this batch operation is attached to.",
	                                                      GDATA_TYPE_SERVICE,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataBatchOperation:authorization-domain:
	 *
	 * The authorization domain for the batch operation, against which the #GDataService:authorizer for the #GDataBatchOperation:service should be
	 * authorized. This may be %NULL if authorization is not needed for any of the requests in the batch operation.
	 *
	 * All requests in the batch operation must be authorizable under this single authorization domain. If requests need different authorization
	 * domains, they must be performed in different batch operations.
	 *
	 * Since: 0.9.0
	 */
	g_object_class_install_property (gobject_class, PROP_AUTHORIZATION_DOMAIN,
	                                 g_param_spec_object ("authorization-domain",
	                                                      "Authorization domain", "The authorization domain for the batch operation.",
	                                                      GDATA_TYPE_AUTHORIZATION_DOMAIN,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataBatchOperation:feed-uri:
	 *
	 * The feed URI that this batch operation will be sent to.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_FEED_URI,
	                                 g_param_spec_string ("feed-uri",
	                                                      "Feed URI", "The feed URI that this batch operation will be sent to.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_batch_operation_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataBatchOperationPrivate *priv = gdata_batch_operation_get_instance_private (GDATA_BATCH_OPERATION (object));

	switch (property_id) {
		case PROP_SERVICE:
			g_value_set_object (value, priv->service);
			break;
		case PROP_AUTHORIZATION_DOMAIN:
			g_value_set_object (value, priv->authorization_domain);
			break;
		case PROP_FEED_URI:
			g_value_set_string (value, priv->feed_uri);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_batch_operation_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataBatchOperationPrivate *priv = gdata_batch_operation_get_instance_private (GDATA_BATCH_OPERATION (object));

	switch (property_id) {
		case PROP_SERVICE:
			priv->service = g_value_dup_object (value);
			break;
		/* Construct only */
		case PROP_AUTHORIZATION_DOMAIN:
			priv->authorization_domain = g_value_dup_object (value);
			break;
		case PROP_FEED_URI:
			priv->feed_uri = g_value_dup_string (value);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_batch_operation_init (GDataBatchOperation *self)
{
	self->priv = gdata_batch_operation_get_instance_private (self);
	self->priv->next_id = 1; /* reserve ID 0 for error conditions */
	self->priv->operations = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) operation_free);
}

static void
gdata_batch_operation_dispose (GObject *object)
{
	GDataBatchOperationPrivate *priv = gdata_batch_operation_get_instance_private (GDATA_BATCH_OPERATION (object));

	if (priv->authorization_domain != NULL)
		g_object_unref (priv->authorization_domain);
	priv->authorization_domain = NULL;

	if (priv->service != NULL)
		g_object_unref (priv->service);
	priv->service = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_batch_operation_parent_class)->dispose (object);
}

static void
gdata_batch_operation_finalize (GObject *object)
{
	GDataBatchOperationPrivate *priv = gdata_batch_operation_get_instance_private (GDATA_BATCH_OPERATION (object));

	g_free (priv->feed_uri);
	g_hash_table_destroy (priv->operations);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_batch_operation_parent_class)->finalize (object);
}

/**
 * gdata_batch_operation_get_service:
 * @self: a #GDataBatchOperation
 *
 * Gets the #GDataBatchOperation:service property.
 *
 * Return value: (transfer none): the batch operation's attached service
 *
 * Since: 0.7.0
 */
GDataService *
gdata_batch_operation_get_service (GDataBatchOperation *self)
{
	g_return_val_if_fail (GDATA_IS_BATCH_OPERATION (self), NULL);
	return self->priv->service;
}

/**
 * gdata_batch_operation_get_authorization_domain:
 * @self: a #GDataBatchOperation
 *
 * Gets the #GDataBatchOperation:authorization-domain property.
 *
 * Return value: (transfer none) (allow-none): the #GDataAuthorizationDomain used to authorize the batch operation, or %NULL
 *
 * Since: 0.9.0
 */
GDataAuthorizationDomain *
gdata_batch_operation_get_authorization_domain (GDataBatchOperation *self)
{
	g_return_val_if_fail (GDATA_IS_BATCH_OPERATION (self), NULL);

	return self->priv->authorization_domain;
}

/**
 * gdata_batch_operation_get_feed_uri:
 * @self: a #GDataBatchOperation
 *
 * Gets the #GDataBatchOperation:feed-uri property.
 *
 * Return value: the batch operation's feed URI
 *
 * Since: 0.7.0
 */
const gchar *
gdata_batch_operation_get_feed_uri (GDataBatchOperation *self)
{
	g_return_val_if_fail (GDATA_IS_BATCH_OPERATION (self), NULL);
	return self->priv->feed_uri;
}

/* Add an operation to the list of operations to be executed when the #GDataBatchOperation is run, and return its operation ID */
static guint
add_operation (GDataBatchOperation *self, GDataBatchOperationType type, GDataEntry *entry, GDataBatchOperationCallback callback, gpointer user_data)
{
	BatchOperation *op;

	/* Create the operation */
	op = g_slice_new0 (BatchOperation);
	op->id = (self->priv->next_id++);
	op->type = type;
	op->callback = callback;
	op->user_data = user_data;
	op->entry = g_object_ref (entry);

	/* Add the operation to the table */
	g_hash_table_insert (self->priv->operations, GUINT_TO_POINTER (op->id), op);

	return op->id;
}

/**
 * _gdata_batch_operation_get_operation:
 * @self: a #GDataBatchOperation
 * @id: the operation ID
 *
 * Return the #BatchOperation for the given operation ID.
 *
 * Return value: the relevant #BatchOperation, or %NULL
 *
 * Since: 0.7.0
 */
BatchOperation *
_gdata_batch_operation_get_operation (GDataBatchOperation *self, guint id)
{
	g_return_val_if_fail (GDATA_IS_BATCH_OPERATION (self), NULL);
	g_return_val_if_fail (id > 0, NULL);

	return g_hash_table_lookup (self->priv->operations, GUINT_TO_POINTER (id));
}

/* Run a user-supplied callback for a #BatchOperation whose return value we've just processed. This is designed to be used in an idle handler, so
 * that the callback is run in the main thread. It can be called if the user-supplied callback is %NULL (e.g. in the case that the callback's been
 * called before). */
static gboolean
run_callback_cb (BatchOperation *op)
{
	if (op->callback != NULL)
		op->callback (op->id, op->type, op->entry, op->error, op->user_data);

	/* Unset the callback so that it can't be called again */
	op->callback = NULL;

	return FALSE;
}

/**
 * _gdata_batch_operation_run_callback:
 * @self: a #GDataBatchOperation
 * @op: the #BatchOperation which has been finished
 * @entry: (allow-none): the entry representing the operation's result, or %NULL
 * @error: the error from the operation, or %NULL
 *
 * Run the callback for @op to notify the user code that the operation's result has been received and processed. Either @entry or @error should be
 * set (and the other should be %NULL), signifying a successful operation or a failed operation, respectively.
 *
 * The function will call @op's user-supplied callback, if available, in either the current or the main thread, depending on whether the
 * #GDataBatchOperation was run with gdata_batch_operation_run() or gdata_batch_operation_run_async().
 *
 * Since: 0.7.0
 */
void
_gdata_batch_operation_run_callback (GDataBatchOperation *self, BatchOperation *op, GDataEntry *entry, GError *error)
{
	g_return_if_fail (GDATA_IS_BATCH_OPERATION (self));
	g_return_if_fail (op != NULL);
	g_return_if_fail (entry == NULL || GDATA_IS_ENTRY (entry));
	g_return_if_fail (entry == NULL || error == NULL);

	/* We can free the request data, and replace it with the response data */
	g_free (op->query_id);
	op->query_id = NULL;
	if (op->entry != NULL)
		g_object_unref (op->entry);
	if (entry != NULL)
		g_object_ref (entry);
	op->entry = entry;
	op->error = error;

	/* Don't bother scheduling run_callback_cb() if there is no callback to run */
	if (op->callback == NULL)
		return;

	/* Only dispatch it in the main thread if the request was run with *_run_async(). This allows applications to run batch operations entirely in
	 * application-owned threads if desired. */
	if (self->priv->is_async == TRUE) {
		/* Send the callback; use G_PRIORITY_DEFAULT rather than G_PRIORITY_DEFAULT_IDLE
		 * to contend with the priorities used by the callback functions in GAsyncResult */
		g_idle_add_full (G_PRIORITY_DEFAULT, (GSourceFunc) run_callback_cb, op, NULL);
	} else {
		run_callback_cb (op);
	}
}

/* Free a #BatchOperation */
static void
operation_free (BatchOperation *op)
{
	g_free (op->query_id);
	if (op->entry != NULL)
		g_object_unref (op->entry);
	if (op->error != NULL)
		g_error_free (op->error);

	g_slice_free (BatchOperation, op);
}

/**
 * gdata_batch_operation_add_query:
 * @self: a #GDataBatchOperation
 * @id: the ID of the entry being queried for
 * @entry_type: the type of the entry which will be returned
 * @callback: (scope async): a #GDataBatchOperationCallback to call when the query is finished, or %NULL
 * @user_data: (closure): data to pass to the @callback function
 *
 * Add a query to the #GDataBatchOperation, to be executed when the operation is run. The query will return a #GDataEntry (of subclass type
 * @entry_type) representing the given entry @id. The ID is of the same format as that returned by gdata_entry_get_id().
 *
 * Note that a single batch operation should not operate on a given #GDataEntry more than once, as there's no guarantee about the order in which the
 * batch operation's operations will be performed.
 *
 * @callback will be called when the #GDataBatchOperation is run with gdata_batch_operation_run() (in which case it will be called in the thread which
 * ran the batch operation), or with gdata_batch_operation_run_async() (in which case it will be called in an idle handler in the main thread). The
 * @operation_id passed to the callback will match the return value of gdata_batch_operation_add_query(), and the @operation_type will be
 * %GDATA_BATCH_OPERATION_QUERY. If the query was successful, the resulting entry will be passed to the callback function as @entry, and @error will
 * be %NULL. If, however, the query was unsuccessful, @entry will be %NULL and @error will contain a #GError detailing what went wrong.
 *
 * Return value: operation ID for the added query, or <code class="literal">0</code>
 *
 * Since: 0.7.0
 */
guint
gdata_batch_operation_add_query (GDataBatchOperation *self, const gchar *id, GType entry_type,
                                 GDataBatchOperationCallback callback, gpointer user_data)
{
	BatchOperation *op;

	g_return_val_if_fail (GDATA_IS_BATCH_OPERATION (self), 0);
	g_return_val_if_fail (id != NULL, 0);
	g_return_val_if_fail (g_type_is_a (entry_type, GDATA_TYPE_ENTRY), 0);
	g_return_val_if_fail (self->priv->has_run == FALSE, 0);

	/* Create the operation manually, since it would be messy to special-case add_operation() to do this */
	op = g_slice_new0 (BatchOperation);
	op->id = (self->priv->next_id++);
	op->type = GDATA_BATCH_OPERATION_QUERY;
	op->callback = callback;
	op->user_data = user_data;
	op->query_id = g_strdup (id);
	op->entry_type = entry_type;

	/* Add the operation to the table */
	g_hash_table_insert (self->priv->operations, GUINT_TO_POINTER (op->id), op);

	return op->id;
}

/**
 * gdata_batch_operation_add_insertion:
 * @self: a #GDataBatchOperation
 * @entry: the #GDataEntry to insert
 * @callback: (scope async): a #GDataBatchOperationCallback to call when the insertion is finished, or %NULL
 * @user_data: (closure): data to pass to the @callback function
 *
 * Add an entry to the #GDataBatchOperation, to be inserted on the server when the operation is run. The insertion will return the inserted version
 * of @entry. @entry is reffed by the function, so may be freed after it returns.
 *
 * @callback will be called as specified in the documentation for gdata_batch_operation_add_query(), with an @operation_type of
 * %GDATA_BATCH_OPERATION_INSERTION.
 *
 * Return value: operation ID for the added insertion, or <code class="literal">0</code>
 *
 * Since: 0.7.0
 */
guint
gdata_batch_operation_add_insertion (GDataBatchOperation *self, GDataEntry *entry, GDataBatchOperationCallback callback, gpointer user_data)
{
	g_return_val_if_fail (GDATA_IS_BATCH_OPERATION (self), 0);
	g_return_val_if_fail (GDATA_IS_ENTRY (entry), 0);
	g_return_val_if_fail (self->priv->has_run == FALSE, 0);

	return add_operation (self, GDATA_BATCH_OPERATION_INSERTION, entry, callback, user_data);
}

/**
 * gdata_batch_operation_add_update:
 * @self: a #GDataBatchOperation
 * @entry: the #GDataEntry to update
 * @callback: (scope async): a #GDataBatchOperationCallback to call when the update is finished, or %NULL
 * @user_data: (closure): data to pass to the @callback function
 *
 * Add an entry to the #GDataBatchOperation, to be updated on the server when the operation is run. The update will return the updated version of
 * @entry. @entry is reffed by the function, so may be freed after it returns.
 *
 * Note that a single batch operation should not operate on a given #GDataEntry more than once, as there's no guarantee about the order in which the
 * batch operation's operations will be performed.
 *
 * @callback will be called as specified in the documentation for gdata_batch_operation_add_query(), with an @operation_type of
 * %GDATA_BATCH_OPERATION_UPDATE.
 *
 * Return value: operation ID for the added update, or <code class="literal">0</code>
 *
 * Since: 0.7.0
 */
guint
gdata_batch_operation_add_update (GDataBatchOperation *self, GDataEntry *entry, GDataBatchOperationCallback callback, gpointer user_data)
{
	g_return_val_if_fail (GDATA_IS_BATCH_OPERATION (self), 0);
	g_return_val_if_fail (GDATA_IS_ENTRY (entry), 0);
	g_return_val_if_fail (self->priv->has_run == FALSE, 0);

	return add_operation (self, GDATA_BATCH_OPERATION_UPDATE, entry, callback, user_data);
}

/**
 * gdata_batch_operation_add_deletion:
 * @self: a #GDataBatchOperation
 * @entry: the #GDataEntry to delete
 * @callback: (scope async): a #GDataBatchOperationCallback to call when the deletion is finished, or %NULL
 * @user_data: (closure): data to pass to the @callback function
 *
 * Add an entry to the #GDataBatchOperation, to be deleted on the server when the operation is run. @entry is reffed by the function, so may be freed
 * after it returns.
 *
 * Note that a single batch operation should not operate on a given #GDataEntry more than once, as there's no guarantee about the order in which the
 * batch operation's operations will be performed.
 *
 * @callback will be called as specified in the documentation for gdata_batch_operation_add_query(), with an @operation_type of
 * %GDATA_BATCH_OPERATION_DELETION.
 *
 * Return value: operation ID for the added deletion, or <code class="literal">0</code>
 *
 * Since: 0.7.0
 */
guint
gdata_batch_operation_add_deletion (GDataBatchOperation *self, GDataEntry *entry, GDataBatchOperationCallback callback, gpointer user_data)
{
	g_return_val_if_fail (GDATA_IS_BATCH_OPERATION (self), 0);
	g_return_val_if_fail (GDATA_IS_ENTRY (entry), 0);
	g_return_val_if_fail (self->priv->has_run == FALSE, 0);

	return add_operation (self, GDATA_BATCH_OPERATION_DELETION, entry, callback, user_data);
}

/**
 * gdata_batch_operation_run:
 * @self: a #GDataBatchOperation
 * @cancellable: (allow-none): a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Run the #GDataBatchOperation synchronously. This will send all the operations in the batch operation to the server, and call their respective
 * callbacks synchronously (i.e. before gdata_batch_operation_run() returns, and in the same thread that called gdata_batch_operation_run()) as the
 * server returns results for each operation.
 *
 * The callbacks for all of the operations in the batch operation are always guaranteed to be called, even if the batch operation as a whole fails.
 * Each callback will be called exactly once for each time gdata_batch_operation_run() is called.
 *
 * The return value of the function indicates whether the overall batch operation was successful, and doesn't indicate the status of any of the
 * operations it comprises. gdata_batch_operation_run() could return %TRUE even if all of its operations failed.
 *
 * @cancellable can be used to cancel the entire batch operation any time before or during the network activity. If @cancellable is cancelled
 * after network activity has finished, gdata_batch_operation_run() will continue and finish as normal.
 *
 * Return value: %TRUE on success, %FALSE otherwise
 *
 * Since: 0.7.0
 */
gboolean
gdata_batch_operation_run (GDataBatchOperation *self, GCancellable *cancellable, GError **error)
{
	GDataBatchOperationPrivate *priv = self->priv;
	SoupMessage *message;
	GDataFeed *feed;
	gint64 updated;
	gchar *upload_data;
	guint status;
	GHashTableIter iter;
	gpointer op_id;
	BatchOperation *op;
	GError *child_error = NULL;

	g_return_val_if_fail (GDATA_IS_BATCH_OPERATION (self), FALSE);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	g_return_val_if_fail (priv->has_run == FALSE, FALSE);

	/* Check for early cancellation. */
	if (g_cancellable_set_error_if_cancelled (cancellable, error)) {
		return FALSE;
	}

	/* Check whether the service actually supports these kinds of
	 * operations. */
	g_hash_table_iter_init (&iter, priv->operations);
	while (g_hash_table_iter_next (&iter, &op_id, (gpointer*) &op) == TRUE) {
		GDataBatchable *batchable = GDATA_BATCHABLE (priv->service);
		GDataBatchableIface *batchable_iface;

		batchable_iface = GDATA_BATCHABLE_GET_IFACE (batchable);

		if (batchable_iface->is_supported != NULL &&
		    !batchable_iface->is_supported (op->type)) {
			g_set_error (error, GDATA_SERVICE_ERROR,
			             GDATA_SERVICE_ERROR_WITH_BATCH_OPERATION,
			             _("Batch operations are unsupported by "
			               "this service."));
			return FALSE;
		}
	}

	message = _gdata_service_build_message (priv->service, priv->authorization_domain, SOUP_METHOD_POST, priv->feed_uri, NULL, TRUE);

	/* Build the request */
	updated = g_get_real_time () / G_USEC_PER_SEC;
	feed = _gdata_feed_new (GDATA_TYPE_FEED, "Batch operation feed",
	                        "batch1", updated);

	g_hash_table_iter_init (&iter, priv->operations);
	while (g_hash_table_iter_next (&iter, &op_id, (gpointer*) &op) == TRUE) {
		if (op->type == GDATA_BATCH_OPERATION_QUERY) {
			/* Queries are weird; build a new throwaway entry, and add it to the feed */
			GDataEntry *entry;
			GDataEntryClass *klass;
			gchar *entry_uri;

			klass = g_type_class_ref (op->entry_type);
			g_assert (klass->get_entry_uri != NULL);

			entry_uri = klass->get_entry_uri (op->query_id);
			entry = gdata_entry_new (entry_uri);
			g_free (entry_uri);

			gdata_entry_set_title (entry, "Batch operation query");
			_gdata_entry_set_updated (entry, updated);

			_gdata_entry_set_batch_data (entry, op->id, op->type);
			_gdata_feed_add_entry (feed, entry);

			g_type_class_unref (klass);
			g_object_unref (entry);
		} else {
			/* Everything else just dumps the entry's XML in the request */
			_gdata_entry_set_batch_data (op->entry, op->id, op->type);
			_gdata_feed_add_entry (feed, op->entry);
		}
	}

	upload_data = gdata_parsable_get_xml (GDATA_PARSABLE (feed));
	soup_message_set_request (message, "application/atom+xml", SOUP_MEMORY_TAKE, upload_data, strlen (upload_data));

	g_object_unref (feed);

	/* Ensure that this GDataBatchOperation can't be run again */
	priv->has_run = TRUE;

	/* Send the message */
	status = _gdata_service_send_message (priv->service, message, cancellable, &child_error);

	if (status != SOUP_STATUS_OK) {
		/* Iff status is SOUP_STATUS_NONE or SOUP_STATUS_CANCELLED, child_error has already been set */
		if (status != SOUP_STATUS_NONE && status != SOUP_STATUS_CANCELLED) {
			/* Error */
			GDataServiceClass *klass = GDATA_SERVICE_GET_CLASS (priv->service);
			g_assert (klass->parse_error_response != NULL);
			klass->parse_error_response (priv->service, GDATA_OPERATION_BATCH, status, message->reason_phrase, message->response_body->data,
			                             message->response_body->length, &child_error);
		}
		g_object_unref (message);

		goto error;
	}

	/* Parse the XML; GDataBatchFeed will fire off the relevant callbacks */
	g_assert (message->response_body->data != NULL);
	feed = GDATA_FEED (_gdata_parsable_new_from_xml (GDATA_TYPE_BATCH_FEED, message->response_body->data, message->response_body->length,
	                                                 self, &child_error));
	g_object_unref (message);

	if (feed == NULL)
		goto error;
	g_object_unref (feed);

	return TRUE;

error:
	/* Call the callbacks for each of our operations to notify them of the error */
	g_hash_table_iter_init (&iter, priv->operations);
	while (g_hash_table_iter_next (&iter, &op_id, (gpointer*) &op) == TRUE)
		_gdata_batch_operation_run_callback (self, op, NULL, g_error_copy (child_error));

	g_propagate_error (error, child_error);

	return FALSE;
}

static void
run_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	GDataBatchOperation *operation = GDATA_BATCH_OPERATION (source_object);
	g_autoptr(GError) error = NULL;

	/* Run the batch operation and return */
	if (!gdata_batch_operation_run (operation, cancellable, &error))
		g_task_return_error (task, g_steal_pointer (&error));
	else
		g_task_return_boolean (task, TRUE);
}

/**
 * gdata_batch_operation_run_async:
 * @self: a #GDataBatchOperation
 * @cancellable: (allow-none): a #GCancellable, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the batch operation is finished, or %NULL
 * @user_data: (closure): data to pass to the @callback function
 *
 * Run the #GDataBatchOperation asynchronously. This will send all the operations in the batch operation to the server, and call their respective
 * callbacks asynchronously (i.e. in idle functions in the main thread, usually after gdata_batch_operation_run_async() has returned) as the
 * server returns results for each operation. @self is reffed when this function is called, so can safely be unreffed after this function returns.
 *
 * For more details, see gdata_batch_operation_run(), which is the synchronous version of this function.
 *
 * When the entire batch operation is finished, @callback will be called. You can then call gdata_batch_operation_run_finish() to get the results of
 * the batch operation.
 *
 * Since: 0.7.0
 */
void
gdata_batch_operation_run_async (GDataBatchOperation *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	g_autoptr(GTask) task = NULL;

	g_return_if_fail (GDATA_IS_BATCH_OPERATION (self));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (self->priv->has_run == FALSE);

	/* Mark the operation as async for the purposes of deciding where to call the callbacks */
	self->priv->is_async = TRUE;

	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_batch_operation_run_async);
	g_task_run_in_thread (task, run_thread);
}

/**
 * gdata_batch_operation_run_finish:
 * @self: a #GDataBatchOperation
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous batch operation run with gdata_batch_operation_run_async().
 *
 * Return values are as for gdata_batch_operation_run().
 *
 * Return value: %TRUE on success, %FALSE otherwise
 *
 * Since: 0.7.0
 */
gboolean
gdata_batch_operation_run_finish (GDataBatchOperation *self, GAsyncResult *async_result, GError **error)
{
	GDataBatchOperationPrivate *priv = self->priv;
	g_autoptr(GError) child_error = NULL;

	g_return_val_if_fail (GDATA_IS_BATCH_OPERATION (self), FALSE);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	g_return_val_if_fail (g_task_is_valid (async_result, self), FALSE);
	g_return_val_if_fail (g_async_result_is_tagged (async_result, gdata_batch_operation_run_async), FALSE);

	if (!g_task_propagate_boolean (G_TASK (async_result), &child_error)) {
		if (priv->has_run == FALSE) {
			GHashTableIter iter;
			gpointer op_id;
			BatchOperation *op;

			/* Temporarily mark the operation as synchronous so that the callbacks get dispatched in this thread */
			priv->is_async = FALSE;

			/* If has_run hasn't been set, the call to gdata_batch_operation_run() was never made in the thread, and so none of the
			 * operations' callbacks have been called. Call the callbacks for each of our operations to notify them of the error.
			 * If has_run has been set, gdata_batch_operation_run() has already done this for us. */
			g_hash_table_iter_init (&iter, priv->operations);
			while (g_hash_table_iter_next (&iter, &op_id, (gpointer*) &op) == TRUE)
				_gdata_batch_operation_run_callback (self, op, NULL, g_error_copy (child_error));

			priv->is_async = TRUE;
		}

		g_propagate_error (error, g_steal_pointer (&child_error));

		return FALSE;
	}

	return TRUE;
}
