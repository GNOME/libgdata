/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Thibault Saunier 2009 <saunierthibault@gmail.com>
 * Copyright (C) Philip Withnall 2010 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-documents-query
 * @short_description: GData Documents query object
 * @stability: Stable
 * @include: gdata/services/documents/gdata-documents-query.h
 *
 * #GDataDocumentsQuery represents a collection of query parameters specific to the Google Documents service, which go above and beyond
 * those catered for by #GDataQuery.
 *
 * For more information on the custom GData query parameters supported by #GDataDocumentsQuery, see the <ulink type="http"
 * url="https://developers.google.com/google-apps/documents-list/#searching_for_documents_and_files">online documentation</ulink>.
 *
 * <example>
 * 	<title>Querying for Documents</title>
 * 	<programlisting>
 *	GDataDocumentsService *service;
 *	GDataDocumentsQuery *query;
 *	GDataFeed *feed;
 *	gint64 current_time;
 *	GList *i;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service *<!-- -->/
 *	service = create_documents_service ();
 *
 *	/<!-- -->* Create the query to use. We're going to query for the last 10 documents modified by example@gmail.com in the past week, including
 *	 * deleted documents. *<!-- -->/
 *	query = gdata_documents_query_new_with_limits (NULL, 0, 10);
 *
 *	gdata_documents_query_add_collaborator (query, "example@gmail.com");
 *	gdata_documents_query_set_show_deleted (query, TRUE);
 *
 *	current_time = g_get_real_time () / G_USEC_PER_SEC;
 *	gdata_query_set_updated_min (GDATA_QUERY (query), current_time - 7 * 24 * 60 * 60);
 *	gdata_query_set_updated_max (GDATA_QUERY (query), current_time);
 *
 *	/<!-- -->* Execute the query *<!-- -->/
 *	feed = gdata_documents_service_query_documents (service, query, NULL, NULL, NULL, &error);
 *
 *	g_object_unref (query);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error querying for documents: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	/<!-- -->* Iterate through the returned documents and do something with them *<!-- -->/
 *	for (i = gdata_feed_get_entries (feed); i != NULL; i = i->next) {
 *		GDataDocumentsDocument *document = GDATA_DOCUMENTS_DOCUMENT (i->data);
 *
 *		/<!-- -->* Do something with the document here, such as insert it into a UI *<!-- -->/
 *	}
 *
 *	g_object_unref (feed);
 * 	</programlisting>
 * </example>
 *
 * Since: 0.4.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gd/gdata-gd-email-address.h"
#include "gdata-documents-query.h"
#include "gdata-private.h"
#include "gdata-query.h"

#include <gdata/services/documents/gdata-documents-spreadsheet.h>
#include <gdata/services/documents/gdata-documents-presentation.h>
#include <gdata/services/documents/gdata-documents-text.h>
#include <gdata/services/documents/gdata-documents-folder.h>

static void gdata_documents_query_dispose (GObject *object);
static void gdata_documents_query_finalize (GObject *object);
static void gdata_documents_query_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_documents_query_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void get_query_uri (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started);

struct _GDataDocumentsQueryPrivate {
	gboolean show_deleted;
	gboolean show_folders;
	gboolean exact_title;
	gchar *folder_id;
	gchar *title;
	GList *collaborator_addresses; /* GDataGDEmailAddress */
	GList *reader_addresses; /* GDataGDEmailAddress */
};

enum {
	PROP_SHOW_DELETED = 1,
	PROP_SHOW_FOLDERS,
	PROP_EXACT_TITLE,
	PROP_FOLDER_ID,
	PROP_TITLE
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataDocumentsQuery, gdata_documents_query, GDATA_TYPE_QUERY)

static void
gdata_documents_query_class_init (GDataDocumentsQueryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataQueryClass *query_class = GDATA_QUERY_CLASS (klass);

	gobject_class->get_property = gdata_documents_query_get_property;
	gobject_class->set_property = gdata_documents_query_set_property;
	gobject_class->dispose = gdata_documents_query_dispose;
	gobject_class->finalize = gdata_documents_query_finalize;

	query_class->get_query_uri = get_query_uri;

	/**
	 * GDataDocumentsQuery:show-deleted:
	 *
	 * A shortcut to request all documents that have been deleted.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_SHOW_DELETED,
	                                 g_param_spec_boolean ("show-deleted",
	                                                       "Show deleted?", "A shortcut to request all documents that have been deleted.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsQuery:show-folders:
	 *
	 * Specifies if the request also returns folders.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_SHOW_FOLDERS,
	                                 g_param_spec_boolean ("show-folders",
	                                                       "Show folders?", "Specifies if the request also returns folders.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsQuery:exact-title:
	 *
	 * Specifies whether the query should search for an exact title match for the #GDataDocumentsQuery:title parameter.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_EXACT_TITLE,
	                                 g_param_spec_boolean ("exact-title",
	                                                       "Exact title?", "Specifies whether the query should search for an exact title match.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsQuery:folder-id:
	 *
	 * Specifies the ID of the folder in which to search.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_FOLDER_ID,
	                                 g_param_spec_string ("folder-id",
	                                                      "Folder ID", "Specifies the ID of the folder in which to search.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsQuery:title:
	 *
	 * A title (or title fragment) to be searched for. If #GDataDocumentsQuery:exact-title is %TRUE, an exact
	 * title match will be searched for, otherwise substring matches will also be returned.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_TITLE,
	                                 g_param_spec_string ("title",
	                                                      "Title", "A title (or title fragment) to be searched for.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_documents_query_init (GDataDocumentsQuery *self)
{
	self->priv = gdata_documents_query_get_instance_private (self);

	/* https://developers.google.com/drive/v3/reference/files/list#q */
	_gdata_query_set_pagination_type (GDATA_QUERY (self),
	                                  GDATA_QUERY_PAGINATION_TOKENS);
}

static void
gdata_documents_query_dispose (GObject *object)
{
	GList *i;
	GDataDocumentsQueryPrivate *priv = GDATA_DOCUMENTS_QUERY (object)->priv;

	for (i = priv->collaborator_addresses; i != NULL; i = i->next)
		g_object_unref (i->data);
	g_list_free (priv->collaborator_addresses);
	priv->collaborator_addresses = NULL;

	for (i = priv->reader_addresses; i != NULL; i = i->next)
		g_object_unref (i->data);
	g_list_free (priv->reader_addresses);
	priv->reader_addresses = NULL;

	G_OBJECT_CLASS (gdata_documents_query_parent_class)->dispose (object);
}

static void
gdata_documents_query_finalize (GObject *object)
{
	GDataDocumentsQueryPrivate *priv = GDATA_DOCUMENTS_QUERY (object)->priv;

	g_free (priv->folder_id);
	g_free (priv->title);

	G_OBJECT_CLASS (gdata_documents_query_parent_class)->finalize (object);
}

static void
gdata_documents_query_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataDocumentsQueryPrivate *priv = GDATA_DOCUMENTS_QUERY (object)->priv;

	switch (property_id) {
		case PROP_SHOW_DELETED:
			g_value_set_boolean (value, priv->show_deleted);
			break;
		case PROP_SHOW_FOLDERS:
			g_value_set_boolean (value, priv->show_folders);
			break;
		case PROP_FOLDER_ID:
			g_value_set_string (value, priv->folder_id);
			break;
		case PROP_EXACT_TITLE:
			g_value_set_boolean (value, priv->exact_title);
			break;
		case PROP_TITLE:
			g_value_set_string (value, priv->title);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_documents_query_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataDocumentsQuery *self = GDATA_DOCUMENTS_QUERY (object);

	switch (property_id) {
		case PROP_SHOW_DELETED:
			gdata_documents_query_set_show_deleted (self, g_value_get_boolean (value));
			break;
		case PROP_SHOW_FOLDERS:
			gdata_documents_query_set_show_folders (self, g_value_get_boolean (value));
			break;
		case PROP_FOLDER_ID:
			gdata_documents_query_set_folder_id (self, g_value_get_string (value));
			break;
		case PROP_EXACT_TITLE:
			self->priv->exact_title = g_value_get_boolean (value);
			break;
		case PROP_TITLE:
			gdata_documents_query_set_title (self, g_value_get_string (value), TRUE);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
get_query_uri (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started)
{
	GDataDocumentsQueryPrivate *priv = GDATA_DOCUMENTS_QUERY (self)->priv;
	guint max_results;

	#define APPEND_SEP g_string_append_c (query_uri, (*params_started == FALSE) ? '?' : '&'); *params_started = TRUE;

	if (priv->folder_id != NULL) {
		g_string_append (query_uri, "/folder%3A");
		g_string_append_uri_escaped (query_uri, priv->folder_id, NULL, FALSE);
	}

	/* Search parameters: https://developers.google.com/drive/web/search-parameters */

	_gdata_query_clear_q_internal (self);

	if  (priv->collaborator_addresses != NULL) {
		GList *i;
		GString *str;

		str = g_string_new (NULL);

		for (i = priv->collaborator_addresses; i != NULL; i = i->next) {
			GDataGDEmailAddress *email_address = GDATA_GD_EMAIL_ADDRESS (i->data);
			const gchar *address;

			address = gdata_gd_email_address_get_address (email_address);
			g_string_append_printf (str, "'%s' in writers", address);
			if (i->next != NULL)
				g_string_append (str, " or ");
		}

		_gdata_query_add_q_internal (self, str->str);
		g_string_free (str, TRUE);
	}

	if  (priv->reader_addresses != NULL) {
		GList *i;
		GString *str;

		str = g_string_new (NULL);

		for (i = priv->reader_addresses; i != NULL; i = i->next) {
			GDataGDEmailAddress *email_address = GDATA_GD_EMAIL_ADDRESS (i->data);
			const gchar *address;

			address = gdata_gd_email_address_get_address (email_address);
			g_string_append_printf (str, "'%s' in readers", address);
			if (i->next != NULL)
				g_string_append (str, " or ");
		}

		_gdata_query_add_q_internal (self, str->str);
		g_string_free (str, TRUE);
	}

	if (priv->show_deleted == TRUE)
		_gdata_query_add_q_internal (self, "trashed=true");
	else
		_gdata_query_add_q_internal (self, "trashed=false");

	if (priv->show_folders == FALSE)
		_gdata_query_add_q_internal (self, "mimeType!='application/vnd.google-apps.folder'");

	if (priv->title != NULL) {
		GString *title_query;
		const gchar *ptr, *ptr_end;

		title_query = g_string_new ("title");
		if (priv->exact_title) {
			g_string_append_c (title_query, '=');
		} else {
			g_string_append (title_query, " contains ");
		}
		g_string_append_c (title_query, '\'');

		for (ptr = priv->title; ptr != NULL; ptr = ptr_end) {
			/* Escape any "'" and "\" found in the title with a "\" */
			ptr_end = strpbrk (ptr, "\'\\");
			if (ptr_end == NULL) {
				g_string_append (title_query, ptr);
			} else {
				g_string_append_len (title_query, ptr, ptr_end - ptr);
				g_string_append_c (title_query, '\\');
				g_string_append_c (title_query, *ptr_end);
			}
		}

		g_string_append_c (title_query, '\'');
		_gdata_query_add_q_internal (self, title_query->str);
		g_string_free (title_query, TRUE);
	}

	/* Chain up to the parent class */
	GDATA_QUERY_CLASS (gdata_documents_query_parent_class)->get_query_uri (self, feed_uri, query_uri, params_started);

	/* https://developers.google.com/drive/v2/reference/files/list */
	max_results = gdata_query_get_max_results (self);
	if (max_results > 0) {
		APPEND_SEP
		max_results = max_results > 1000 ? 1000 : max_results;
		g_string_append_printf (query_uri, "maxResults=%u", max_results);
	}

	APPEND_SEP
	g_string_append_printf (query_uri, "includeItemsFromAllDrives=true");
	APPEND_SEP
	g_string_append_printf (query_uri, "supportsAllDrives=true");
}

/**
 * gdata_documents_query_new:
 * @q: (allow-none): a query string, or %NULL
 *
 * Creates a new #GDataDocumentsQuery with its #GDataQuery:q property set to @q.
 *
 * Return value: a new #GDataDocumentsQuery
 *
 * Since: 0.4.0
 */
GDataDocumentsQuery *
gdata_documents_query_new (const gchar *q)
{
	return g_object_new (GDATA_TYPE_DOCUMENTS_QUERY, "q", q, NULL);
}

/**
 * gdata_documents_query_new_with_limits:
 * @q: (allow-none): a query string, or %NULL
 * @start_index: a one-based start index for the results, or <code class="literal">0</code>
 * @max_results: the maximum number of results to return, or <code class="literal">0</code>
 *
 * Creates a new #GDataDocumentsQuery with its #GDataQuery:q property set to @q, and the limits @start_index and @max_results
 * applied.
 *
 * Return value: a new #GDataDocumentsQuery
 *
 * Since: 0.4.0
 */
GDataDocumentsQuery *
gdata_documents_query_new_with_limits (const gchar *q, guint start_index, guint max_results)
{
	return g_object_new (GDATA_TYPE_DOCUMENTS_QUERY,
	                     "q", q,
	                     "start-index", start_index,
	                     "max-results", max_results,
	                     NULL);
}

/**
 * gdata_documents_query_show_deleted:
 * @self: a #GDataDocumentsQuery
 *
 * Gets the #GDataDocumentsQuery:show_deleted property.
 *
 * Return value: %TRUE if the request should return deleted entries, %FALSE otherwise
 *
 * Since: 0.4.0
 */
gboolean
gdata_documents_query_show_deleted (GDataDocumentsQuery *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_QUERY (self), FALSE);
	return self->priv->show_deleted;
}

/**
 * gdata_documents_query_set_show_deleted:
 * @self: a #GDataDocumentsQuery
 * @show_deleted: %TRUE if the request should return deleted entries, %FALSE otherwise
 *
 * Sets the #GDataDocumentsQuery:show_deleted property to @show_deleted.
 *
 * Since: 0.4.0
 */
void
gdata_documents_query_set_show_deleted (GDataDocumentsQuery *self, gboolean show_deleted)
{
	g_return_if_fail (GDATA_IS_DOCUMENTS_QUERY (self));
	self->priv->show_deleted = show_deleted;
	g_object_notify (G_OBJECT (self), "show-deleted");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

/**
 * gdata_documents_query_show_folders:
 * @self: a #GDataDocumentsQuery
 *
 * Gets the #GDataDocumentsQuery:show-folders property.
 *
 * Return value: %TRUE if the request should return folders, %FALSE otherwise
 *
 * Since: 0.4.0
 */
gboolean
gdata_documents_query_show_folders (GDataDocumentsQuery *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_QUERY (self), FALSE);
	return self->priv->show_folders;
}

/**
 * gdata_documents_query_set_show_folders:
 * @self: a #GDataDocumentsQuery
 * @show_folders: %TRUE if the request should return folders, %FALSE otherwise
 *
 * Sets the #GDataDocumentsQuery:show-folders property to show_folders.
 *
 * Since: 0.4.0
 */
void
gdata_documents_query_set_show_folders (GDataDocumentsQuery *self, gboolean show_folders)
{
	g_return_if_fail (GDATA_IS_DOCUMENTS_QUERY (self));
	self->priv->show_folders = show_folders;
	g_object_notify (G_OBJECT (self), "show-folders");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

/**
 * gdata_documents_query_get_folder_id:
 * @self: a #GDataDocumentsQuery
 *
 * Gets the #GDataDocumentsQuery:folder-id property.
 *
 * Return value: the ID of the folder to be queried, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_documents_query_get_folder_id (GDataDocumentsQuery *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_QUERY (self), NULL);
	return self->priv->folder_id;
}

/**
 * gdata_documents_query_set_folder_id:
 * @self: a #GDataDocumentsQuery
 * @folder_id: (allow-none): the ID of the folder to be queried, or %NULL
 *
 * Sets the #GDataDocumentsQuery:folder-id property to @folder_id.
 *
 * Set @folder_id to %NULL to unset the property in the query URI.
 *
 * Since: 0.4.0
 */
void
gdata_documents_query_set_folder_id (GDataDocumentsQuery *self, const gchar *folder_id)
{
	g_return_if_fail (GDATA_IS_DOCUMENTS_QUERY (self));

	g_free (self->priv->folder_id);
	self->priv->folder_id = g_strdup (folder_id);
	g_object_notify (G_OBJECT (self), "folder-id");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

/**
 * gdata_documents_query_get_title:
 * @self: a #GDataDocumentsQuery
 *
 * Gets the #GDataDocumentsQuery:title property.
 *
 * Return value: the title (or title fragment) being queried for, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_documents_query_get_title (GDataDocumentsQuery *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_QUERY (self), NULL);
	return self->priv->title;
}

/**
 * gdata_documents_query_get_exact_title:
 * @self: a #GDataDocumentsQuery
 *
 * Gets the #GDataDocumentsQuery:exact-title property.
 *
 * Return value: %TRUE if the query matches the exact title of documents with #GDataDocumentsQuery:title, %FALSE otherwise
 *
 * Since: 0.4.0
 */
gboolean
gdata_documents_query_get_exact_title (GDataDocumentsQuery *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_QUERY (self), FALSE);
	return self->priv->exact_title;
}

/**
 * gdata_documents_query_set_title:
 * @self: a #GDataDocumentsQuery
 * @title: (allow-none): the title (or title fragment) to query for, or %NULL
 * @exact_title: %TRUE if the query should match the exact @title, %FALSE otherwise
 *
 * Sets the #GDataDocumentsQuery:title property to @title.
 *
 * Set @title to %NULL to unset the property in the query URI.
 *
 * Since: 0.4.0
 */
void
gdata_documents_query_set_title (GDataDocumentsQuery *self, const gchar *title, gboolean exact_title)
{
	g_return_if_fail (GDATA_IS_DOCUMENTS_QUERY (self));

	g_free (self->priv->title);
	self->priv->title = g_strdup (title);
	self->priv->exact_title = exact_title;

	g_object_freeze_notify (G_OBJECT (self));
	g_object_notify (G_OBJECT (self), "exact-title");
	g_object_notify (G_OBJECT (self), "title");
	g_object_thaw_notify (G_OBJECT (self));

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

/**
 * gdata_documents_query_get_collaborator_addresses:
 * @self: a #GDataDocumentsQuery
 *
 * Gets a list of #GDataGDEmailAddress<!-- -->es of the document collaborators whose documents will be queried.
 *
 * Return value: (element-type GData.GDEmailAddress) (transfer none): a list of #GDataGDEmailAddress<!-- -->es of the collaborators concerned by the
 * query, or %NULL
 *
 * Since: 0.4.0
 */
GList *
gdata_documents_query_get_collaborator_addresses (GDataDocumentsQuery *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_QUERY (self), NULL);
	return self->priv->collaborator_addresses;
}

/**
 * gdata_documents_query_get_reader_addresses:
 * @self: a #GDataDocumentsQuery
 *
 * Gets a list of #GDataGDEmailAddress<!-- -->es of the document readers whose documents will be queried.
 *
 * Return value: (element-type GData.GDEmailAddress) (transfer none): a list of #GDataGDEmailAddress<!-- -->es of the readers concerned by the query,
 * or %NULL
 *
 * Since: 0.4.0
 */
GList *
gdata_documents_query_get_reader_addresses (GDataDocumentsQuery *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_QUERY (self), NULL);
	return self->priv->reader_addresses;
}

/**
 * gdata_documents_query_add_reader:
 * @self: a #GDataDocumentsQuery
 * @email_address: the e-mail address of the reader to add
 *
 * Add @email_address as a #GDataGDEmailAddress to the list of readers, the documents readable by whom will be queried.
 *
 * Since: 0.4.0
 */
void
gdata_documents_query_add_reader (GDataDocumentsQuery *self, const gchar *email_address)
{
	GDataGDEmailAddress *address;

	g_return_if_fail (GDATA_IS_DOCUMENTS_QUERY (self));
	g_return_if_fail (email_address != NULL && *email_address != '\0');

	address = gdata_gd_email_address_new (email_address, "reader", NULL, FALSE);
	self->priv->reader_addresses = g_list_append (self->priv->reader_addresses, address);

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

/**
 * gdata_documents_query_add_collaborator:
 * @self: a #GDataDocumentsQuery
 * @email_address: the e-mail address of the collaborator to add
 *
 * Add @email_address as a #GDataGDEmailAddress to the list of collaborators whose edited documents will be queried.
 *
 * Since: 0.4.0
 */
void
gdata_documents_query_add_collaborator (GDataDocumentsQuery *self, const gchar *email_address)
{
	GDataGDEmailAddress *address;

	g_return_if_fail (GDATA_IS_DOCUMENTS_QUERY (self));
	g_return_if_fail (email_address != NULL && *email_address != '\0');

	address = gdata_gd_email_address_new (email_address, "collaborator", NULL, FALSE);
	self->priv->collaborator_addresses = g_list_append (self->priv->collaborator_addresses, address);

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}
