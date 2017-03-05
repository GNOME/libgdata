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
 * SECTION:gdata-freebase-search-query
 * @short_description: GData Freebase query object
 * @stability: Stable
 * @include: gdata/services/freebase/gdata-freebase-query.h
 *
 * #GDataFreebaseQuery represents a collection of query parameters specific to the Google Freebase service.
 * a #GDataFreebaseQuery is built on top of a search term, further filters can be set on the search query
 * through gdata_freebase_search_query_add_filter() or gdata_freebase_search_query_add_location(). The filters
 * can be nested in sublevels, created through gdata_freebase_search_query_open_filter()
 * and gdata_freebase_search_query_close_filter().
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
#include <json-glib/json-glib.h>

#include "gdata-freebase-search-query.h"
#include "gdata-query.h"
#include "gdata-parser.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void gdata_freebase_search_query_finalize (GObject *self);
static void gdata_freebase_search_query_set_property (GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gdata_freebase_search_query_get_property (GObject *self, guint prop_id, GValue *value, GParamSpec *pspec);
static void get_query_uri (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started);

typedef enum {
	NODE_CONTAINER,
	NODE_VALUE,
	NODE_LOCATION
} FilterNodeType;

typedef union {
	FilterNodeType type;

	struct {
		FilterNodeType type;
		GDataFreebaseSearchFilterType filter_type;
		GPtrArray *child_nodes; /* Contains owned FilterNode structs */
	} container;

	struct {
		FilterNodeType type;
		gchar *property;
		gchar *value;
	} value;

	struct {
		FilterNodeType type;
		guint64 radius;
		gdouble lat;
		gdouble lon;
	} location;
} FilterNode;

struct _GDataFreebaseSearchQueryPrivate {
	FilterNode *filter;
	GList *filter_stack; /* Contains unowned FilterNode structs */

	gchar *lang;
	guint stemmed : 1;
};

enum {
	PROP_LANGUAGE = 1,
	PROP_STEMMED
};

G_DEFINE_TYPE (GDataFreebaseSearchQuery, gdata_freebase_search_query, GDATA_TYPE_QUERY)

static void
gdata_freebase_search_query_class_init (GDataFreebaseSearchQueryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataQueryClass *query_class = GDATA_QUERY_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataFreebaseSearchQueryPrivate));

	gobject_class->finalize = gdata_freebase_search_query_finalize;
	gobject_class->set_property = gdata_freebase_search_query_set_property;
	gobject_class->get_property = gdata_freebase_search_query_get_property;

	query_class->get_query_uri = get_query_uri;

	/**
	 * GDataFreebaseSearchQuery:language:
	 *
	 * Language used for search results, in ISO-639-1 format.
	 *
	 * Since: 0.15.1
	 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
	 */
	g_object_class_install_property (gobject_class, PROP_LANGUAGE,
	                                 g_param_spec_string ("language",
							      "Language used for results",
							      "Language in ISO-639-1 format.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
							      G_PARAM_DEPRECATED));
	/**
	 * GDataFreebaseSearchQuery:stemmed:
	 *
	 * Whether word stemming should happen on the search terms. If this property is enabled,
	 * words like eg. "natural", "naturally" or "nature" would be all reduced to the root "natur"
	 * for search purposes.
	 *
	 * Since: 0.15.1
	 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
	 */
	g_object_class_install_property (gobject_class, PROP_STEMMED,
	                                 g_param_spec_boolean ("stemmed",
							       "Stem search terms",
							       "Whether the search terms should be stemmed",
							       FALSE,
							       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
							       G_PARAM_DEPRECATED));
}

static void
gdata_freebase_search_query_init (GDataFreebaseSearchQuery *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_FREEBASE_SEARCH_QUERY, GDataFreebaseSearchQueryPrivate);
}

static void
_free_filter_node (FilterNode *node)
{
	switch (node->type) {
	case NODE_CONTAINER:
		g_ptr_array_unref (node->container.child_nodes);
		break;
	case NODE_VALUE:
		g_free (node->value.property);
		g_free (node->value.value);
		break;
	case NODE_LOCATION:
	default:
		break;
	}

	g_slice_free (FilterNode, node);
}

static void
gdata_freebase_search_query_finalize (GObject *self)
{
	GDataFreebaseSearchQueryPrivate *priv = GDATA_FREEBASE_SEARCH_QUERY (self)->priv;

	g_free (priv->lang);
	g_list_free (priv->filter_stack);

	if (priv->filter != NULL)
		_free_filter_node (priv->filter);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_freebase_search_query_parent_class)->finalize (self);
}

static void
gdata_freebase_search_query_set_property (GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GDataFreebaseSearchQuery *query = GDATA_FREEBASE_SEARCH_QUERY (self);

	switch (prop_id) {
	case PROP_LANGUAGE:
		gdata_freebase_search_query_set_language (query, g_value_get_string (value));
		break;
	case PROP_STEMMED:
		gdata_freebase_search_query_set_stemmed (query, g_value_get_boolean (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
		break;
	}
}

static void
gdata_freebase_search_query_get_property (GObject *self, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GDataFreebaseSearchQueryPrivate *priv = GDATA_FREEBASE_SEARCH_QUERY (self)->priv;

	switch (prop_id) {
	case PROP_LANGUAGE:
		g_value_set_string (value, priv->lang);
		break;
	case PROP_STEMMED:
		g_value_set_boolean (value, priv->stemmed);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
		break;
	}
}

static void
build_filter_string (FilterNode *node,
		     GString    *str)
{
	switch (node->type) {
	case NODE_CONTAINER:
	{
		/* Array matches GDataFreebaseSearchFilterType */
		const gchar *type_str[] = { "all", "any", "not" };
		guint i;

		g_assert (/* node->container.filter_type >= 0 && */
		          node->container.filter_type < G_N_ELEMENTS (type_str));

		g_string_append_printf (str, "(%s", type_str[node->container.filter_type]);

		for (i = 0; i < node->container.child_nodes->len; i++)
			build_filter_string (g_ptr_array_index (node->container.child_nodes, i), str);

		g_string_append (str, ")");
		break;
	}
	case NODE_VALUE:
	{
		gchar *escaped;

		escaped = g_strescape (node->value.value, NULL);
		g_string_append_printf (str, " %s:\"%s\"", node->value.property, escaped);
		g_free (escaped);
		break;
	}
	case NODE_LOCATION:
	{
		gchar lon_str[G_ASCII_DTOSTR_BUF_SIZE], lat_str[G_ASCII_DTOSTR_BUF_SIZE];

		g_ascii_formatd (lon_str, G_ASCII_DTOSTR_BUF_SIZE, "%.4f", node->location.lon);
		g_ascii_formatd (lat_str, G_ASCII_DTOSTR_BUF_SIZE, "%.4f", node->location.lat);
		g_string_append_printf (str, "(within radius:%" G_GUINT64_FORMAT "m lon:%s lat:%s)",
					node->location.radius, lon_str, lat_str);
		break;
	}
	default:
		g_assert_not_reached ();
		break;
	}
}

static void
get_query_uri (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started)
{
	GDataFreebaseSearchQueryPrivate *priv = GDATA_FREEBASE_SEARCH_QUERY (self)->priv;
	const gchar *query, *lang = NULL;
	gint64 updated_max;
	guint cur, limit;

#define APPEND_SEP g_string_append_c (query_uri, (*params_started == FALSE) ? '?' : '&'); *params_started = TRUE;

	query = gdata_query_get_q (self);

	if (query != NULL) {
		APPEND_SEP;
		g_string_append (query_uri, "query=");
		g_string_append (query_uri, query);
	}

	if (priv->filter != NULL) {
		GString *str = g_string_new (NULL);

		build_filter_string (priv->filter, str);

		APPEND_SEP;
		g_string_append (query_uri, "filter=");
		g_string_append (query_uri, str->str);
		g_string_free (str, TRUE);
	}

	updated_max = gdata_query_get_updated_max (self);

	if (updated_max != -1) {
		gchar *date_str;

		date_str = gdata_parser_int64_to_iso8601 (updated_max);

		APPEND_SEP;
		g_string_append (query_uri, "as_of_time=");
		g_string_append (query_uri, date_str);
		g_free (date_str);
	}

	if (priv->lang != NULL) {
		lang = priv->lang;
	} else {
		const gchar * const *user_languages;
		GString *lang_str = NULL;
		gint i;

		user_languages = g_get_language_names ();

		for (i = 0; user_languages[i] != NULL; i++) {
			if (strlen (user_languages[i]) != 2)
				continue;

			if (!lang_str)
				lang_str = g_string_new (user_languages[i]);
			else
				g_string_append_printf (lang_str, ",%s", user_languages[i]);
		}

		lang = g_string_free (lang_str, FALSE);
	}

	APPEND_SEP;
	g_string_append (query_uri, "lang=");
	g_string_append (query_uri, lang);

	if (priv->stemmed) {
		APPEND_SEP;
		g_string_append (query_uri, "stemmed=true");
	}

	cur = gdata_query_get_start_index (self);

	if (cur > 0) {
		APPEND_SEP;
		g_string_append_printf (query_uri, "cursor=%d", cur);
	}

	limit = gdata_query_get_max_results (self);

	if (limit > 0) {
		APPEND_SEP;
		g_string_append_printf (query_uri, "limit=%d", limit);
	}

	/* We don't chain up with parent class get_query_uri because it uses
	 *  GData protocol parameters and they aren't compatible with newest API family
	 */
#undef APPEND_SEP
}

/**
 * gdata_freebase_search_query_new:
 * @search_terms: string to search for
 *
 * Creates a new #GDataFreebaseSearchQuery prepared to search for Freebase elements that
 * match the given @search_terms. Further filters on the query can be set through
 * gdata_freebase_search_query_add_filter() or gdata_freebase_search_query_add_location().
 *
 * Return value: (transfer full): a new #GDataFreebaseSearchQuery; unref with g_object_unref()
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GDataFreebaseSearchQuery *
gdata_freebase_search_query_new (const gchar *search_terms)
{
	g_return_val_if_fail (search_terms != NULL, NULL);
	return g_object_new (GDATA_TYPE_FREEBASE_SEARCH_QUERY, "q", search_terms, NULL);
}

/**
 * gdata_freebase_search_query_open_filter:
 * @self: a #GDataFreebaseSearchQuery
 * @filter_type: filter type
 *
 * Opens a container of filter rules, those are applied according to the behavior specified by @filter_type.
 * Every call to this function must be paired by a call to gdata_freebase_search_query_close_filter().
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
void
gdata_freebase_search_query_open_filter (GDataFreebaseSearchQuery *self, GDataFreebaseSearchFilterType filter_type)
{
	GDataFreebaseSearchQueryPrivate *priv;
	FilterNode *current_node, *node;

	g_return_if_fail (GDATA_IS_FREEBASE_SEARCH_QUERY (self));

	priv = GDATA_FREEBASE_SEARCH_QUERY (self)->priv;

	node = g_slice_new0 (FilterNode);
	node->type = NODE_CONTAINER;
	node->container.filter_type = filter_type;
	node->container.child_nodes = g_ptr_array_new_with_free_func ((GDestroyNotify) _free_filter_node);

	if (priv->filter_stack != NULL) {
		current_node = priv->filter_stack->data;
		g_ptr_array_add (current_node->container.child_nodes, node);
	} else if (priv->filter == NULL) {
		priv->filter = node;
	} else {
		g_assert_not_reached ();
	}

	priv->filter_stack = g_list_prepend (priv->filter_stack, node);
}

/**
 * gdata_freebase_search_query_close_filter:
 * @self: a #GDataFreebaseSearchQuery
 *
 * Closes a filter level.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
void
gdata_freebase_search_query_close_filter (GDataFreebaseSearchQuery *self)
{
	GDataFreebaseSearchQueryPrivate *priv;

	g_return_if_fail (GDATA_IS_FREEBASE_SEARCH_QUERY (self));

	priv = GDATA_FREEBASE_SEARCH_QUERY (self)->priv;

	if (priv->filter_stack == NULL)
		g_assert_not_reached ();

	priv->filter_stack = g_list_delete_link (priv->filter_stack, priv->filter_stack);
}

/**
 * gdata_freebase_search_query_add_filter:
 * @self: a #GDataFreebaseSearchQuery
 * @property: Freebase property ID
 * @value: match string
 *
 * Adds a property filter to the query. property filters are always nested in
 * containers, opened and closed through gdata_freebase_search_query_open_filter()
 * and gdata_freebase_search_query_close_filter().
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
void
gdata_freebase_search_query_add_filter (GDataFreebaseSearchQuery *self, const gchar *property, const gchar *value)
{
	GDataFreebaseSearchQueryPrivate *priv;
	FilterNode *current_node, *node;

	g_return_if_fail (GDATA_IS_FREEBASE_SEARCH_QUERY (self));
	g_return_if_fail (property != NULL && value != NULL);

	priv = GDATA_FREEBASE_SEARCH_QUERY (self)->priv;

	if (priv->filter_stack == NULL) {
		g_critical ("A filter container must be opened before through "
			    "gdata_freebase_search_query_open_filter()");
		g_assert_not_reached ();
	}

	node = g_slice_new0 (FilterNode);
	node->type = NODE_VALUE;
	node->value.property = g_strdup (property);
	node->value.value = g_strdup (value);

	current_node = priv->filter_stack->data;
	g_ptr_array_add (current_node->container.child_nodes, node);
}

/**
 * gdata_freebase_search_query_add_location:
 * @self: a #GDataFreebaseSearchQuery
 * @radius: radius in meters
 * @lat: latitude
 * @lon: longitude
 *
 * Adds a geolocation filter to the query. location filters are always nested in
 * containers, opened and closed through gdata_freebase_search_query_open_filter()
 * and gdata_freebase_search_query_close_filter().
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
void
gdata_freebase_search_query_add_location (GDataFreebaseSearchQuery *self, guint64 radius, gdouble lat, gdouble lon)
{
	GDataFreebaseSearchQueryPrivate *priv = GDATA_FREEBASE_SEARCH_QUERY (self)->priv;
	FilterNode *current_node, *node;

	g_return_if_fail (GDATA_IS_FREEBASE_SEARCH_QUERY (self));

	if (priv->filter_stack == NULL) {
		g_critical ("A filter container must be opened before through "
			    "gdata_freebase_search_query_open_filter()");
		g_assert_not_reached ();
	}

	node = g_slice_new0 (FilterNode);
	node->type = NODE_LOCATION;
	node->location.radius = radius;
	node->location.lat = lat;
	node->location.lon = lon;

	current_node = priv->filter_stack->data;
	g_ptr_array_add (current_node->container.child_nodes, node);
}

/**
 * gdata_freebase_search_query_set_language:
 * @self: a #GDataFreebaseSearchQuery
 * @lang: (allow-none): Language used on the search terms and results, in ISO-639-1 format, or %NULL to unset.
 *
 * Sets the language used, both on the search terms and the results. If unset,
 * the locale preferences will be respected.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
void
gdata_freebase_search_query_set_language (GDataFreebaseSearchQuery *self,
					  const gchar              *lang)
{
	GDataFreebaseSearchQueryPrivate *priv;

	g_return_if_fail (GDATA_IS_FREEBASE_SEARCH_QUERY (self));
	g_return_if_fail (!lang || strlen (lang) == 2);

	priv = self->priv;

	if (g_strcmp0 (priv->lang, lang) == 0)
		return;

	g_free (priv->lang);
	priv->lang = g_strdup (lang);
	g_object_notify (G_OBJECT (self), "language");
}

/**
 * gdata_freebase_search_query_get_language:
 * @self: a #GDataFreebaseSearchQuery
 *
 * Gets the language set on the search query, or %NULL if unset.
 *
 * Return value: (allow-none): The language used on the query.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const gchar *
gdata_freebase_search_query_get_language (GDataFreebaseSearchQuery *self)
{
	GDataFreebaseSearchQueryPrivate *priv;

	g_return_val_if_fail (GDATA_IS_FREEBASE_SEARCH_QUERY (self), NULL);

	priv = self->priv;
	return priv->lang;
}

/**
 * gdata_freebase_search_query_set_stemmed:
 * @self: a #GDataFreebaseSearchQuery
 * @stemmed: %TRUE to perform stemming on the search results
 *
 * Sets whether stemming is performed on the provided search terms. If @stemmed is %TRUE,
 * words like eg. "natural", "naturally" or "nature" would be all reduced to the root "natur"
 * for search purposes.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
void
gdata_freebase_search_query_set_stemmed (GDataFreebaseSearchQuery *self,
					 gboolean                  stemmed)
{
	GDataFreebaseSearchQueryPrivate *priv;

	g_return_if_fail (GDATA_IS_FREEBASE_SEARCH_QUERY (self));

	priv = self->priv;

	if (priv->stemmed == stemmed)
		return;

	priv->stemmed = stemmed;
	g_object_notify (G_OBJECT (self), "stemmed");
}

/**
 * gdata_freebase_search_query_get_stemmed:
 * @self: a #GDataFreebaseSearchQuery
 *
 * Returns whether the #GDataFreebaseSearchQuery will perform stemming on the search terms.
 *
 * Return value: %TRUE if the #GDataFreebaseSearchQuery performs stemming
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
gboolean
gdata_freebase_search_query_get_stemmed (GDataFreebaseSearchQuery *self)
{
	GDataFreebaseSearchQueryPrivate *priv;

	g_return_val_if_fail (GDATA_IS_FREEBASE_SEARCH_QUERY (self), FALSE);

	priv = self->priv;
	return priv->stemmed;
}

G_GNUC_END_IGNORE_DEPRECATIONS
