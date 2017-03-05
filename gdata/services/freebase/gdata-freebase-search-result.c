/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) 2014 Carlos Garnacho <carlosg@gnome.org>
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
 * SECTION:gdata-freebase-search-result
 * @short_description: GData Freebase search result object
 * @stability: Stable
 * @include: gdata/services/freebase/gdata-freebase-result.h
 *
 * #GDataFreebaseSearchResult is a subclass of #GDataEntry to represent the result of a Freebase search query.
 *
 * For more details of Google Freebase API, see the <ulink type="http" url="https://developers.google.com/freebase/v1/">
 * online documentation</ulink>.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-freebase-search-result.h"
#include "gdata-private.h"
#include "gdata-types.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

#define URLBASE "https://www.googleapis.com/freebase/v1"

struct _GDataFreebaseSearchResultItem {
	gchar *mid;
	gchar *id;
	gchar *name;
	gchar *lang;
	gchar *notable_id;
	gchar *notable_name;
	gdouble score;
};

struct _GDataFreebaseSearchResultPrivate {
	GPtrArray *items; /* contains owned GDataFreebaseSearchResultItem structs */
	guint total_hits;
};

static void gdata_freebase_search_result_finalize (GObject *self);
static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);
static GDataFreebaseSearchResultItem * item_copy (const GDataFreebaseSearchResultItem *item);
static void item_free (GDataFreebaseSearchResultItem *item);

G_DEFINE_BOXED_TYPE (GDataFreebaseSearchResultItem, gdata_freebase_search_result_item, item_copy, item_free)

G_DEFINE_TYPE (GDataFreebaseSearchResult, gdata_freebase_search_result, GDATA_TYPE_FREEBASE_RESULT)

static void
gdata_freebase_search_result_class_init (GDataFreebaseSearchResultClass *klass)
{
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataFreebaseSearchResultPrivate));

	gobject_class->finalize = gdata_freebase_search_result_finalize;
	parsable_class->parse_json = parse_json;
}

static GDataFreebaseSearchResultItem *
item_new (void)
{
	return g_slice_new0 (GDataFreebaseSearchResultItem);
}

static void
item_free (GDataFreebaseSearchResultItem *item)
{
	g_free (item->mid);
	g_free (item->id);
	g_free (item->name);
	g_free (item->lang);
	g_free (item->notable_id);
	g_free (item->notable_name);
	g_slice_free (GDataFreebaseSearchResultItem, item);
}

static GDataFreebaseSearchResultItem *
item_copy (const GDataFreebaseSearchResultItem *item)
{
	GDataFreebaseSearchResultItem *copy;

	copy = item_new ();
	copy->mid = g_strdup (item->mid);
	copy->id = g_strdup (item->id);
	copy->name = g_strdup (item->name);
	copy->lang = g_strdup (item->lang);
	copy->notable_id = g_strdup (item->notable_id);
	copy->notable_name = g_strdup (item->notable_name);
	copy->score = item->score;

	return copy;
}

static void
gdata_freebase_search_result_init (GDataFreebaseSearchResult *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_FREEBASE_SEARCH_RESULT, GDataFreebaseSearchResultPrivate);
	self->priv->items = g_ptr_array_new_with_free_func ((GDestroyNotify) item_free);
}

static void
gdata_freebase_search_result_finalize (GObject *self)
{
	GDataFreebaseSearchResultPrivate *priv = GDATA_FREEBASE_SEARCH_RESULT (self)->priv;

	g_ptr_array_unref (priv->items);

	G_OBJECT_CLASS (gdata_freebase_search_result_parent_class)->finalize (self);
}

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	GDataFreebaseSearchResultPrivate *priv = GDATA_FREEBASE_SEARCH_RESULT (parsable)->priv;
	const GError *inner_error = NULL;
	const gchar *member_name;
	gint count, i;

	GDATA_PARSABLE_CLASS (gdata_freebase_search_result_parent_class)->parse_json (parsable, reader, user_data, error);

#define ITEM_SET_STRING(id,field,mandatory)				\
	json_reader_read_member (reader, #id);				\
	item->field = g_strdup (json_reader_get_string_value (reader)); \
	if (mandatory) {						\
		inner_error = json_reader_get_error (reader);		\
		if (inner_error != NULL) goto item_error;		\
	}								\
	json_reader_end_member (reader);

#define ITEM_SET_DOUBLE(id,field)					\
	json_reader_read_member (reader, #id);				\
	item->field = json_reader_get_double_value (reader);		\
	inner_error = json_reader_get_error (reader);			\
	if (inner_error != NULL) goto item_error;			\
	json_reader_end_member (reader);

	member_name = json_reader_get_member_name (reader);

	if (member_name == NULL)
		return FALSE;

	if (strcmp (member_name, "hits") == 0) {
		priv->total_hits = json_reader_get_int_value (reader);
		return TRUE;
	} else if (strcmp (member_name, "result") != 0) {
		/* Avoid anything else besides hits/result */
		return TRUE;
	}

	if (!json_reader_is_array (reader))
		return FALSE;

	count = json_reader_count_elements (reader);

	for (i = 0; i < count; i++) {
		GDataFreebaseSearchResultItem *item;

		item = item_new ();
		json_reader_read_element (reader, i);

		ITEM_SET_STRING (mid, mid, TRUE);
		ITEM_SET_STRING (id, id, FALSE);
		ITEM_SET_STRING (name, name, TRUE);
		ITEM_SET_STRING (lang, lang, FALSE);
		ITEM_SET_DOUBLE (score, score);

		/* Read "notable" */
		json_reader_read_member (reader, "notable");

		if (json_reader_get_error (reader) == NULL) {
			ITEM_SET_STRING (id, notable_id, TRUE);
			ITEM_SET_STRING (name, notable_name, TRUE);
		}

		json_reader_end_member (reader);
		json_reader_end_element (reader);

		g_ptr_array_add (priv->items, item);
		continue;

	item_error:
		item_free (item);
		gdata_parser_error_required_json_content_missing (reader, error);
		return FALSE;
	}

#undef ITEM_SET_DOUBLE
#undef ITEM_SET_STRING

	return TRUE;
}

/**
 * gdata_freebase_search_result_new:
 *
 * Creates a new #GDataFreebaseSearchResult with the given ID and default properties.
 *
 * Return value: (transfer full): a new #GDataFreebaseSearchResult; unref with g_object_unref()
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GDataFreebaseSearchResult *
gdata_freebase_search_result_new (void)
{
	return g_object_new (GDATA_TYPE_FREEBASE_SEARCH_RESULT, NULL);
}

/**
 * gdata_freebase_search_result_get_num_items:
 * @self: a #GDataFreebaseSearchResult
 *
 * Returns the number of items contained in this result.
 *
 * Returns: The number of items
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
guint
gdata_freebase_search_result_get_num_items (GDataFreebaseSearchResult *self)
{
	g_return_val_if_fail (GDATA_IS_FREEBASE_SEARCH_RESULT (self), 0);

	return self->priv->items->len;
}

/**
 * gdata_freebase_search_result_get_total_hits:
 * @self: a #GDataFreebaseSearchResult
 *
 * Returns the total number of hits found for the search query.
 *
 * Returns: the total number of hits.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
guint
gdata_freebase_search_result_get_total_hits (GDataFreebaseSearchResult *self)
{
	g_return_val_if_fail (GDATA_IS_FREEBASE_SEARCH_RESULT (self), 0);

	return self->priv->total_hits;
}

/**
 * gdata_freebase_search_result_get_item:
 * @self: a #GDataFreebaseSearchResult
 * @i: number of item to retrieve
 *
 * Gets an item from the search result.
 *
 * Returns: (transfer none) (allow-none): a search result item, or %NULL on invalid item.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const GDataFreebaseSearchResultItem *
gdata_freebase_search_result_get_item (GDataFreebaseSearchResult *self, guint i)
{
	GDataFreebaseSearchResultPrivate *priv;

	g_return_val_if_fail (GDATA_IS_FREEBASE_SEARCH_RESULT (self), NULL);

	priv = self->priv;
	g_return_val_if_fail (i < priv->items->len, NULL);

	return g_ptr_array_index (priv->items, i);
}

/**
 * gdata_freebase_search_result_item_get_mid:
 * @item: a #GDataFreebaseSearchResultItem
 *
 * Returns the machine-encoded ID (MID) of the search result item. Elements may
 * have a single MID, as opposed to the potentially multiple Freebase IDs that
 * may point to it. MIDs are usable interchangeably with Freebase IDs.
 *
 * Returns: (transfer none): The result item MID.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const gchar *
gdata_freebase_search_result_item_get_mid (const GDataFreebaseSearchResultItem *item)
{
	g_return_val_if_fail (item != NULL, NULL);
	return item->mid;
}

/**
 * gdata_freebase_search_result_item_get_id:
 * @item: a #GDataFreebaseSearchResultItem
 *
 * Returns the Freebase ID of the search result item.
 *
 * Returns: (transfer none): The search result item Freebase ID.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const gchar *
gdata_freebase_search_result_item_get_id (const GDataFreebaseSearchResultItem *item)
{
	g_return_val_if_fail (item != NULL, NULL);

	if (item->id != NULL)
		return item->id;

	return item->mid;
}

/**
 * gdata_freebase_search_result_item_get_name:
 * @item: a #GDataFreebaseSearchResultItem
 *
 * Returns the human readable name of the search result item.
 *
 * Returns: (transfer none): The human readable name of the item.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const gchar *
gdata_freebase_search_result_item_get_name (const GDataFreebaseSearchResultItem *item)
{
	g_return_val_if_fail (item != NULL, NULL);
	return item->name;
}

/**
 * gdata_freebase_search_result_item_get_language:
 * @item: a #GDataFreebaseSearchResultItem
 *
 * Gets the language of this search result item, in ISO-639-1 format.
 *
 * Returns: (transfer none): The language of the search result item.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const gchar *
gdata_freebase_search_result_item_get_language (const GDataFreebaseSearchResultItem *item)
{
	g_return_val_if_fail (item != NULL, NULL);
	return item->lang;
}

/**
 * gdata_freebase_search_result_item_get_notable_id:
 * @item: a #GDataFreebaseSearchResultItem
 *
 * If this search result item is notable in an specific topic, this function
 * returns the Freebase ID of this topic.
 *
 * Returns: (transfer none) (allow-none): The topic the result item is most notable of, or %NULL.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const gchar *
gdata_freebase_search_result_item_get_notable_id (const GDataFreebaseSearchResultItem *item)
{
	g_return_val_if_fail (item != NULL, NULL);
	return item->notable_id;
}

/**
 * gdata_freebase_search_result_item_get_notable_name:
 * @item: a #GDataFreebaseSearchResultItem
 *
 * If this search result item is notable in an specific topic, this function
 * returns the human readable name of this topic.
 *
 * Returns: (transfer none) (allow-none): The human readable topic name, or %NULL
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const gchar *
gdata_freebase_search_result_item_get_notable_name (const GDataFreebaseSearchResultItem *item)
{
	g_return_val_if_fail (item != NULL, NULL);
	return item->notable_name;
}

/**
 * gdata_freebase_search_result_item_get_score:
 * @item: a #GDataFreebaseSearchResultItem
 *
 * Returns the score of this search result item. The higher, the more relevant this
 * item seems, given the search terms.
 *
 * Returns: the result item score.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
gdouble
gdata_freebase_search_result_item_get_score (const GDataFreebaseSearchResultItem *item)
{
	g_return_val_if_fail (item != NULL, 0.0);
	return item->score;
}

G_GNUC_END_IGNORE_DEPRECATIONS
