/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009–2010 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-gd-where
 * @short_description: GData where element
 * @stability: Stable
 * @include: gdata/gd/gdata-gd-where.h
 *
 * #GDataGDWhere represents a "where" element from the
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdWhere">GData specification</ulink>.
 *
 * Since: 0.4.0
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-gd-where.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-comparable.h"

static void gdata_gd_where_comparable_init (GDataComparableIface *iface);
static void gdata_gd_where_finalize (GObject *object);
static void gdata_gd_where_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_gd_where_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
/*static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);*/
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);
/*static void get_xml (GDataParsable *parsable, GString *xml_string);*/
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataGDWherePrivate {
	gchar *relation_type;
	gchar *value_string;
	gchar *label;
};

enum {
	PROP_RELATION_TYPE = 1,
	PROP_VALUE_STRING,
	PROP_LABEL
};

G_DEFINE_TYPE_WITH_CODE (GDataGDWhere, gdata_gd_where, GDATA_TYPE_PARSABLE,
                         G_ADD_PRIVATE (GDataGDWhere)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMPARABLE, gdata_gd_where_comparable_init))

static void
gdata_gd_where_class_init (GDataGDWhereClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_gd_where_get_property;
	gobject_class->set_property = gdata_gd_where_set_property;
	gobject_class->finalize = gdata_gd_where_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	/*parsable_class->parse_xml = parse_xml;*/
	parsable_class->pre_get_xml = pre_get_xml;
	/*parsable_class->get_xml = get_xml;*/
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "where";
	parsable_class->element_namespace = "gd";

	/**
	 * GDataGDWhere:relation-type:
	 *
	 * Specifies the relationship between the containing entity and the contained location. For example: %GDATA_GD_WHERE_EVENT or
	 * %GDATA_GD_WHERE_EVENT_PARKING.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdWhere">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_RELATION_TYPE,
	                                 g_param_spec_string ("relation-type",
	                                                      "Relation type", "Specifies the relationship between the container and the containee.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDWhere:value-string:
	 *
	 * A simple string representation of this location.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdWhere">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_VALUE_STRING,
	                                 g_param_spec_string ("value-string",
	                                                      "Value string", "A simple string representation of this location.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDWhere:label:
	 *
	 * Specifies a user-readable label to distinguish this location from others.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdWhere">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_LABEL,
	                                 g_param_spec_string ("label",
	                                                      "Label", "Specifies a user-readable label to distinguish this location from others.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static gint
compare_with (GDataComparable *self, GDataComparable *other)
{
	GDataGDWherePrivate *a = ((GDataGDWhere*) self)->priv, *b = ((GDataGDWhere*) other)->priv;

	if (g_strcmp0 (a->value_string, b->value_string) == 0 && g_strcmp0 (a->label, b->label) == 0)
		return 0;
	return 1;
}

static void
gdata_gd_where_comparable_init (GDataComparableIface *iface)
{
	iface->compare_with = compare_with;
}

static void
gdata_gd_where_init (GDataGDWhere *self)
{
	self->priv = gdata_gd_where_get_instance_private (self);
}

static void
gdata_gd_where_finalize (GObject *object)
{
	GDataGDWherePrivate *priv = GDATA_GD_WHERE (object)->priv;

	g_free (priv->relation_type);
	g_free (priv->value_string);
	g_free (priv->label);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gd_where_parent_class)->finalize (object);
}

static void
gdata_gd_where_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataGDWherePrivate *priv = GDATA_GD_WHERE (object)->priv;

	switch (property_id) {
		case PROP_RELATION_TYPE:
			g_value_set_string (value, priv->relation_type);
			break;
		case PROP_VALUE_STRING:
			g_value_set_string (value, priv->value_string);
			break;
		case PROP_LABEL:
			g_value_set_string (value, priv->label);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_gd_where_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataGDWhere *self = GDATA_GD_WHERE (object);

	switch (property_id) {
		case PROP_RELATION_TYPE:
			gdata_gd_where_set_relation_type (self, g_value_get_string (value));
			break;
		case PROP_VALUE_STRING:
			gdata_gd_where_set_value_string (self, g_value_get_string (value));
			break;
		case PROP_LABEL:
			gdata_gd_where_set_label (self, g_value_get_string (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static gboolean
pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error)
{
	xmlChar *rel;
	GDataGDWherePrivate *priv = GDATA_GD_WHERE (parsable)->priv;

	rel = xmlGetProp (root_node, (xmlChar*) "rel");
	if (rel != NULL && *rel == '\0') {
		xmlFree (rel);
		return gdata_parser_error_required_property_missing (root_node, "rel", error);
	}

	priv->relation_type = (gchar*) rel;
	priv->value_string = (gchar*) xmlGetProp (root_node, (xmlChar*) "valueString");
	priv->label = (gchar*) xmlGetProp (root_node, (xmlChar*) "label");

	return TRUE;
}

/*static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	GDataGDWherePrivate *priv = GDATA_GD_WHERE (parsable)->priv;

	TODO: deal with the entryLink

	return TRUE;
}*/

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGDWherePrivate *priv = GDATA_GD_WHERE (parsable)->priv;

	if (priv->label != NULL)
		gdata_parser_string_append_escaped (xml_string, " label='", priv->label, "'");

	if (priv->relation_type != NULL)
		gdata_parser_string_append_escaped (xml_string, " rel='", priv->relation_type, "'");

	if (priv->value_string != NULL)
		gdata_parser_string_append_escaped (xml_string, " valueString='", priv->value_string, "'");
}

/*static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGDWherePrivate *priv = GDATA_GD_WHERE (parsable)->priv;

	TODO: deal with the entryLink
}*/

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "gd", (gchar*) "http://schemas.google.com/g/2005");
}

/**
 * gdata_gd_where_new:
 * @relation_type: (allow-none): the relationship between the item and this place, or %NULL
 * @value_string: (allow-none): a string to represent the place, or %NULL
 * @label: (allow-none): a human-readable label for the place, or %NULL
 *
 * Creates a new #GDataGDWhere. More information is available in the <ulink type="http"
 * url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdWhere">GData specification</ulink>.
 *
 * Currently, entryLink functionality is not implemented in #GDataGDWhere.
 *
 * Return value: a new #GDataGDWhere; unref with g_object_unref()
 *
 * Since: 0.2.0
 */
GDataGDWhere *
gdata_gd_where_new (const gchar *relation_type, const gchar *value_string, const gchar *label)
{
	g_return_val_if_fail (relation_type == NULL || *relation_type != '\0', NULL);
	return g_object_new (GDATA_TYPE_GD_WHERE, "relation-type", relation_type, "value-string", value_string, "label", label, NULL);
}

/**
 * gdata_gd_where_get_relation_type:
 * @self: a #GDataGDWhere
 *
 * Gets the #GDataGDWhere:relation-type property.
 *
 * Return value: the relation type, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_gd_where_get_relation_type (GDataGDWhere *self)
{
	g_return_val_if_fail (GDATA_IS_GD_WHERE (self), NULL);
	return self->priv->relation_type;
}

/**
 * gdata_gd_where_set_relation_type:
 * @self: a #GDataGDWhere
 * @relation_type: (allow-none): the new relation type, or %NULL
 *
 * Sets the #GDataGDWhere:relation-type property to @relation_type.
 *
 * Set @relation_type to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_gd_where_set_relation_type (GDataGDWhere *self, const gchar *relation_type)
{
	g_return_if_fail (GDATA_IS_GD_WHERE (self));
	g_return_if_fail (relation_type == NULL || *relation_type != '\0');

	g_free (self->priv->relation_type);
	self->priv->relation_type = g_strdup (relation_type);
	g_object_notify (G_OBJECT (self), "relation-type");
}

/**
 * gdata_gd_where_get_value_string:
 * @self: a #GDataGDWhere
 *
 * Gets the #GDataGDWhere:value-string property.
 *
 * Return value: the value string, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_gd_where_get_value_string (GDataGDWhere *self)
{
	g_return_val_if_fail (GDATA_IS_GD_WHERE (self), NULL);
	return self->priv->value_string;
}

/**
 * gdata_gd_where_set_value_string:
 * @self: a #GDataGDWhere
 * @value_string: (allow-none): the new value string, or %NULL
 *
 * Sets the #GDataGDWhere:value-string property to @value_string.
 *
 * Set @value_string to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_gd_where_set_value_string (GDataGDWhere *self, const gchar *value_string)
{
	g_return_if_fail (GDATA_IS_GD_WHERE (self));

	g_free (self->priv->value_string);
	self->priv->value_string = g_strdup (value_string);
	g_object_notify (G_OBJECT (self), "value-string");
}

/**
 * gdata_gd_where_get_label:
 * @self: a #GDataGDWhere
 *
 * Gets the #GDataGDWhere:label property.
 *
 * Return value: the label, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_gd_where_get_label (GDataGDWhere *self)
{
	g_return_val_if_fail (GDATA_IS_GD_WHERE (self), NULL);
	return self->priv->label;
}

/**
 * gdata_gd_where_set_label:
 * @self: a #GDataGDWhere
 * @label: (allow-none): the new label, or %NULL
 *
 * Sets the #GDataGDWhere:label property to @label.
 *
 * Set @label to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_gd_where_set_label (GDataGDWhere *self, const gchar *label)
{
	g_return_if_fail (GDATA_IS_GD_WHERE (self));

	g_free (self->priv->label);
	self->priv->label = g_strdup (label);
	g_object_notify (G_OBJECT (self), "label");
}
