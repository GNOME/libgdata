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
 * SECTION:gdata-freebase-result
 * @short_description: GData Freebase result object
 * @stability: Stable
 * @include: gdata/services/freebase/gdata-freebase-result.h
 *
 * #GDataFreebaseResult is a subclass of #GDataEntry to represent the result of a Google Freebase MQL query.
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

#include "gdata-freebase-result.h"
#include "gdata-private.h"
#include "gdata-types.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

#define URLBASE "https://www.googleapis.com/freebase/v1/"

enum {
	PROP_VARIANT = 1
};

struct _GDataFreebaseResultPrivate {
	GVariant *result;
};

static void gdata_freebase_result_finalize (GObject *self);
static void gdata_freebase_result_get_property (GObject *self, guint prop_id, GValue *value, GParamSpec *pspec);
static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);
static const gchar *get_content_type (void);
static gchar *get_entry_uri (const gchar *id);

G_DEFINE_TYPE (GDataFreebaseResult, gdata_freebase_result, GDATA_TYPE_ENTRY)

static void
gdata_freebase_result_class_init (GDataFreebaseResultClass *klass)
{
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataFreebaseResultPrivate));

	gobject_class->finalize = gdata_freebase_result_finalize;
	gobject_class->get_property = gdata_freebase_result_get_property;

	parsable_class->parse_json = parse_json;
	parsable_class->get_content_type = get_content_type;
	entry_class->get_entry_uri = get_entry_uri;

	/**
	 * GDataFreebaseResult:variant:
	 *
	 * Variant containing the MQL result. The variant is a very generic container of type "a{smv}",
	 * containing (possibly nested) Freebase schema types and values.
	 *
	 * Since: 0.15.1
	 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
	 */
	g_object_class_install_property (gobject_class, PROP_VARIANT,
	                                 g_param_spec_variant ("variant",
							       "Variant", "Variant holding the raw result.",
							       G_VARIANT_TYPE ("a{smv}"), NULL,
							       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS |
							       G_PARAM_DEPRECATED));
}

static void
gdata_freebase_result_init (GDataFreebaseResult *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_FREEBASE_RESULT, GDataFreebaseResultPrivate);
}

static void
gdata_freebase_result_finalize (GObject *self)
{
	GDataFreebaseResultPrivate *priv = GDATA_FREEBASE_RESULT (self)->priv;

	if (priv->result != NULL)
		g_variant_unref (priv->result);

	G_OBJECT_CLASS (gdata_freebase_result_parent_class)->finalize (self);
}

static void
gdata_freebase_result_get_property (GObject *self, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GDataFreebaseResultPrivate *priv = GDATA_FREEBASE_RESULT (self)->priv;

	switch (prop_id) {
	case PROP_VARIANT:
		g_value_set_variant (value, priv->result);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
		break;
	}
}

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	GDataFreebaseResultPrivate *priv = GDATA_FREEBASE_RESULT (parsable)->priv;
	JsonNode *root, *node;
	JsonObject *object;

	if (g_strcmp0 (json_reader_get_member_name (reader), "result") != 0)
		return TRUE;

	g_object_get (reader, "root", &root, NULL);
	object = json_node_get_object (root);
	node = json_object_get_member (object, "result");

	priv->result = g_variant_ref_sink (json_gvariant_deserialize (node, NULL, error));
	json_node_free (root);

	return (priv->result != NULL);
}

static const gchar *
get_content_type (void)
{
	return "application/json";
}

static gchar *
get_entry_uri (const gchar *id)
{
	/* https://www.googleapis.com/freebase/v1/mqlread interface */
	return g_strconcat (URLBASE, id, NULL);
}

/**
 * gdata_freebase_result_new:
 *
 * Creates a new #GDataFreebaseResult.
 *
 * Return value: (transfer full): a new #GDataFreebaseResult; unref with g_object_unref()
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GDataFreebaseResult *
gdata_freebase_result_new (void)
{
	return g_object_new (GDATA_TYPE_FREEBASE_RESULT, NULL);
}

/**
 * gdata_freebase_result_dup_variant:
 * @self: a #GDataFreebaseResult
 *
 * Gets the result serialized as a #GVariant of type "a{smv}", containing the JSON
 * data tree. This variant can be alternatively processed through json_gvariant_serialize().
 *
 * Returns: (allow-none) (transfer full): the serialized result, or %NULL; unref with g_variant_unref()
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GVariant *
gdata_freebase_result_dup_variant (GDataFreebaseResult *self)
{
	GDataFreebaseResultPrivate *priv;

	g_return_val_if_fail (GDATA_IS_FREEBASE_RESULT (self), NULL);

	priv = self->priv;

	if (priv->result == NULL)
		return NULL;

	return g_variant_ref (priv->result);
}

G_GNUC_END_IGNORE_DEPRECATIONS
