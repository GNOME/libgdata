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
 * SECTION:gdata-freebase-query
 * @short_description: GData Freebase query object
 * @stability: Stable
 * @include: gdata/services/freebase/gdata-freebase-query.h
 *
 * #GDataFreebaseQuery represents a MQL query specific to the Google Freebase service.
 *
 * This implementation of #GDataQuery respects the gdata_query_set_max_results() call.
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

#include "gdata-freebase-query.h"
#include "gdata-query.h"
#include "gdata-parser.h"
#include "gdata-private.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void gdata_freebase_query_finalize (GObject *self);
static void gdata_freebase_query_set_property (GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gdata_freebase_query_get_property (GObject *self, guint prop_id, GValue *value, GParamSpec *pspec);
static void get_query_uri (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started);

struct _GDataFreebaseQueryPrivate {
	/* These params are here not in GDataQuery due of differently named query params for JSON protocols therefore need for different parse_uri */
	GVariant *variant;
	JsonNode *query_node;
};

enum {
	PROP_VARIANT = 1,
};

G_DEFINE_TYPE (GDataFreebaseQuery, gdata_freebase_query, GDATA_TYPE_QUERY)

static void
gdata_freebase_query_class_init (GDataFreebaseQueryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataQueryClass *query_class = GDATA_QUERY_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataFreebaseQueryPrivate));

	gobject_class->finalize = gdata_freebase_query_finalize;
	gobject_class->set_property = gdata_freebase_query_set_property;
	gobject_class->get_property = gdata_freebase_query_get_property;

	query_class->get_query_uri = get_query_uri;

	/**
	 * GDataFreebaseQuery:variant:
	 *
	 * Variant containing the MQL query. The variant is a very generic container of type "a{smv}",
	 * containing (possibly nested) Freebase schema types and values.
	 *
	 * Since: 0.15.1
	 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
	 */
	g_object_class_install_property (gobject_class, PROP_VARIANT,
	                                 g_param_spec_variant ("variant",
							       "Variant",
							       "Variant to construct the query from.",
							       G_VARIANT_TYPE ("a{smv}"), NULL,
							       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
							       G_PARAM_DEPRECATED));
}

static void
gdata_freebase_query_init (GDataFreebaseQuery *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_FREEBASE_QUERY, GDataFreebaseQueryPrivate);

	/* https://developers.google.com/freebase/v1/search#cursor */
	_gdata_query_set_pagination_type (GDATA_QUERY (self),
	                                  GDATA_QUERY_PAGINATION_INDEXED);
}

static void
gdata_freebase_query_finalize (GObject *self)
{
	GDataFreebaseQueryPrivate *priv = GDATA_FREEBASE_QUERY (self)->priv;

	if (priv->variant != NULL)
		g_variant_unref (priv->variant);
	if (priv->query_node != NULL)
		json_node_free (priv->query_node);
	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_freebase_query_parent_class)->finalize (self);
}

static void
gdata_freebase_query_set_property (GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GDataFreebaseQueryPrivate *priv = GDATA_FREEBASE_QUERY (self)->priv;

	switch (prop_id) {
	case PROP_VARIANT:
		priv->variant = g_value_get_variant (value);
		if (priv->variant)
			priv->query_node = json_gvariant_serialize (priv->variant);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
		break;
	}
}

static void
gdata_freebase_query_get_property (GObject *self, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GDataFreebaseQueryPrivate *priv = GDATA_FREEBASE_QUERY (self)->priv;

	switch (prop_id) {
	case PROP_VARIANT:
		g_value_set_variant (value, priv->variant);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
		break;
	}
}

static void
get_query_uri (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started)
{
	GDataFreebaseQueryPrivate *priv = GDATA_FREEBASE_QUERY (self)->priv;
	const gchar *query;

#define APPEND_SEP g_string_append_c (query_uri, (*params_started == FALSE) ? '?' : '&'); *params_started = TRUE;

	query = gdata_query_get_q (self);

	if (query != NULL) {
		APPEND_SEP;
		g_string_append (query_uri, "query=");
		g_string_append (query_uri, query);
	} else if (priv->query_node != NULL) {
		JsonGenerator *generator;
		JsonNode *copy;
		gchar *json;
		guint limit;

		copy = json_node_copy (priv->query_node);

		limit = gdata_query_get_max_results (self);

		if (limit > 0) {
			JsonNode *limit_node;
			JsonObject *object;

			limit_node = json_node_new (JSON_NODE_VALUE);
			json_node_set_int (limit_node, limit);

			object = json_node_get_object (copy);
			json_object_set_member (object, "limit", limit_node);
		}

		generator = json_generator_new ();
		json_generator_set_root (generator, copy);
		json = json_generator_to_data (generator, NULL);
		g_object_unref (generator);

		APPEND_SEP;
		g_string_append (query_uri, "query=");
		g_string_append (query_uri, json);
		g_free (json);
	}

	/* We don't chain up with parent class get_query_uri because it uses
	 *  GData protocol parameters and they aren't compatible with newest API family
	 */
#undef APPEND_SEP
}

/**
 * gdata_freebase_query_new:
 * @mql: a MQL query string
 *
 * Creates a new #GDataFreebaseQuery with the MQL query provided in @mql. MQL
 * is a JSON-based query language, analogous to SPARQL. To learn more about MQL,
 * see the <ulink type="http" url="https://developers.google.com/freebase/v1/mql-overview">
 * MQL overview</ulink> and <ulink type="http" url="https://developers.google.com/freebase/v1/mql-cookbook">
 * cookbook</ulink>.
 *
 * For detailed information on Freebase schemas, The <ulink type="http" url="http://www.freebase.com/schema">"Schema"
 * section</ulink> on the main site allows for natural search and navigation through the multiple data properties and domains.
 *
 * Return value: (transfer full): a new #GDataFreebaseQuery
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GDataFreebaseQuery *
gdata_freebase_query_new (const gchar *mql)
{
	g_return_val_if_fail (mql != NULL, NULL);

	return g_object_new (GDATA_TYPE_FREEBASE_QUERY, "q", mql, NULL);
}

/**
 * gdata_freebase_query_new_from_variant:
 * @variant: a variant containing the MQL query structure
 *
 * Creates a new #GDataFreebaseQuery with the MQL query provided in a serialized form as @variant
 * of type "a{smv}" containing the JSON data tree of a MQL query. One convenient way
 * to build the variant is using json_gvariant_serialize() from a #JsonNode. For more information
 * about MQL, see gdata_freebase_query_new().
 *
 * #GDataFreebaseQuery takes ownership on @variant, if it has a floating reference, it will be sunk.
 * Otherwise an extra reference will be added.
 *
 * Return value: (transfer full): a new #GDataFreebaseQuery
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GDataFreebaseQuery *
gdata_freebase_query_new_from_variant (GVariant *variant)
{
	g_return_val_if_fail (variant != NULL, NULL);

	return g_object_new (GDATA_TYPE_FREEBASE_QUERY,
			     "variant", g_variant_ref_sink (variant),
			     NULL);
}

G_GNUC_END_IGNORE_DEPRECATIONS
