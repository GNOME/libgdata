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
 * SECTION:gdata-gcontact-external-id
 * @short_description: gContact externalId element
 * @stability: Stable
 * @include: gdata/gcontact/gdata-gcontact-external-id.h
 *
 * #GDataGContactExternalID represents an "externalId" element from the
 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcExternalId">gContact specification</ulink>.
 *
 * Since: 0.7.0
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-gcontact-external-id.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-comparable.h"

static void gdata_gcontact_external_id_comparable_init (GDataComparableIface *iface);
static void gdata_gcontact_external_id_finalize (GObject *object);
static void gdata_gcontact_external_id_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_gcontact_external_id_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataGContactExternalIDPrivate {
	gchar *value;
	gchar *relation_type;
	gchar *label;
};

enum {
	PROP_VALUE = 1,
	PROP_RELATION_TYPE,
	PROP_LABEL
};

G_DEFINE_TYPE_WITH_CODE (GDataGContactExternalID, gdata_gcontact_external_id, GDATA_TYPE_PARSABLE,
                         G_ADD_PRIVATE (GDataGContactExternalID)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMPARABLE, gdata_gcontact_external_id_comparable_init))

static void
gdata_gcontact_external_id_class_init (GDataGContactExternalIDClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_gcontact_external_id_get_property;
	gobject_class->set_property = gdata_gcontact_external_id_set_property;
	gobject_class->finalize = gdata_gcontact_external_id_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->pre_get_xml = pre_get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "externalId";
	parsable_class->element_namespace = "gContact";

	/**
	 * GDataGContactExternalID:value:
	 *
	 * The value of the external ID.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcExternalId">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_VALUE,
	                                 g_param_spec_string ("value",
	                                                      "Value", "The value of the external ID.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGContactExternalID:relation-type:
	 *
	 * A programmatic value that identifies the type of external ID. It is mutually exclusive with #GDataGContactExternalID:label.
	 * Examples are %GDATA_GCONTACT_EXTERNAL_ID_NETWORK or %GDATA_GCONTACT_EXTERNAL_ID_ACCOUNT.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcExternalId">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_RELATION_TYPE,
	                                 g_param_spec_string ("relation-type",
	                                                      "Relation type", "A programmatic value that identifies the type of external ID.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGContactExternalID:label:
	 *
	 * A free-form string that identifies the type of external ID. It is mutually exclusive with #GDataGContactExternalID:relation-type.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcExternalId">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_LABEL,
	                                 g_param_spec_string ("label",
	                                                      "Label", "A free-form string that identifies the type of external ID.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static gint
compare_with (GDataComparable *self, GDataComparable *other)
{
	GDataGContactExternalIDPrivate *a = ((GDataGContactExternalID*) self)->priv, *b = ((GDataGContactExternalID*) other)->priv;

	if (g_strcmp0 (a->value, b->value) == 0 && g_strcmp0 (a->relation_type, b->relation_type) == 0 && g_strcmp0 (a->label, b->label) == 0)
		return 0;
	return 1;
}

static void
gdata_gcontact_external_id_comparable_init (GDataComparableIface *iface)
{
	iface->compare_with = compare_with;
}

static void
gdata_gcontact_external_id_init (GDataGContactExternalID *self)
{
	self->priv = gdata_gcontact_external_id_get_instance_private (self);
}

static void
gdata_gcontact_external_id_finalize (GObject *object)
{
	GDataGContactExternalIDPrivate *priv = GDATA_GCONTACT_EXTERNAL_ID (object)->priv;

	g_free (priv->value);
	g_free (priv->relation_type);
	g_free (priv->label);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gcontact_external_id_parent_class)->finalize (object);
}

static void
gdata_gcontact_external_id_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataGContactExternalIDPrivate *priv = GDATA_GCONTACT_EXTERNAL_ID (object)->priv;

	switch (property_id) {
		case PROP_VALUE:
			g_value_set_string (value, priv->value);
			break;
		case PROP_RELATION_TYPE:
			g_value_set_string (value, priv->relation_type);
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
gdata_gcontact_external_id_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataGContactExternalID *self = GDATA_GCONTACT_EXTERNAL_ID (object);

	switch (property_id) {
		case PROP_VALUE:
			gdata_gcontact_external_id_set_value (self, g_value_get_string (value));
			break;
		case PROP_RELATION_TYPE:
			gdata_gcontact_external_id_set_relation_type (self, g_value_get_string (value));
			break;
		case PROP_LABEL:
			gdata_gcontact_external_id_set_label (self, g_value_get_string (value));
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
	xmlChar *value, *rel, *label;
	GDataGContactExternalIDPrivate *priv = GDATA_GCONTACT_EXTERNAL_ID (parsable)->priv;

	value = xmlGetProp (root_node, (xmlChar*) "value");
	if (value == NULL) {
		xmlFree (value);
		return gdata_parser_error_required_property_missing (root_node, "value", error);
	}

	rel = xmlGetProp (root_node, (xmlChar*) "rel");
	label = xmlGetProp (root_node, (xmlChar*) "label");
	if ((rel == NULL || *rel == '\0') && (label == NULL || *label == '\0')) {
		xmlFree (value);
		xmlFree (rel);
		xmlFree (label);
		return gdata_parser_error_required_property_missing (root_node, "rel", error);
	} else if (rel != NULL && label != NULL) {
		/* Can't have both set at once */
		xmlFree (value);
		xmlFree (rel);
		xmlFree (label);
		return gdata_parser_error_mutexed_properties (root_node, "rel", "label", error);
	}

	priv->value = (gchar*) value;
	priv->relation_type = (gchar*) rel;
	priv->label = (gchar*) label;

	return TRUE;
}

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGContactExternalIDPrivate *priv = GDATA_GCONTACT_EXTERNAL_ID (parsable)->priv;

	gdata_parser_string_append_escaped (xml_string, " value='", priv->value, "'");

	if (priv->relation_type != NULL)
		gdata_parser_string_append_escaped (xml_string, " rel='", priv->relation_type, "'");
	else if (priv->label != NULL)
		gdata_parser_string_append_escaped (xml_string, " label='", priv->label, "'");
	else
		g_assert_not_reached ();
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "gContact", (gchar*) "http://schemas.google.com/contact/2008");
}

/**
 * gdata_gcontact_external_id_new:
 * @value: the value of the external ID
 * @relation_type: (allow-none): the type of external ID, or %NULL
 * @label: (allow-none): a free-form label for the external ID, or %NULL
 *
 * Creates a new #GDataGContactExternalID. More information is available in the <ulink type="http"
 * url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcExternalId">gContact specification</ulink>.
 *
 * Exactly one of @relation_type and @label should be provided; the other must be %NULL.
 *
 * Return value: a new #GDataGContactExternalID; unref with g_object_unref()
 *
 * Since: 0.7.0
 */
GDataGContactExternalID *
gdata_gcontact_external_id_new (const gchar *value, const gchar *relation_type, const gchar *label)
{
	g_return_val_if_fail (value != NULL, NULL);
	g_return_val_if_fail ((relation_type != NULL && *relation_type != '\0' && label == NULL) ||
	                      (relation_type == NULL && label != NULL && *label != '\0'), NULL);

	return g_object_new (GDATA_TYPE_GCONTACT_EXTERNAL_ID, "value", value, "relation-type", relation_type, "label", label, NULL);
}

/**
 * gdata_gcontact_external_id_get_value:
 * @self: a #GDataGContactExternalID
 *
 * Gets the #GDataGContactExternalID:value property.
 *
 * Return value: the external ID's value
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_external_id_get_value (GDataGContactExternalID *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_EXTERNAL_ID (self), NULL);
	return self->priv->value;
}

/**
 * gdata_gcontact_external_id_set_value:
 * @self: a #GDataGContactExternalID
 * @value: the new value for the external ID
 *
 * Sets the #GDataGContactExternalID:value property to @value.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_external_id_set_value (GDataGContactExternalID *self, const gchar *value)
{
	g_return_if_fail (GDATA_IS_GCONTACT_EXTERNAL_ID (self));
	g_return_if_fail (value != NULL);

	g_free (self->priv->value);
	self->priv->value = g_strdup (value);
	g_object_notify (G_OBJECT (self), "value");
}

/**
 * gdata_gcontact_external_id_get_relation_type:
 * @self: a #GDataGContactExternalID
 *
 * Gets the #GDataGContactExternalID:relation-type property.
 *
 * Return value: the type of the relation, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_external_id_get_relation_type (GDataGContactExternalID *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_EXTERNAL_ID (self), NULL);
	return self->priv->relation_type;
}

/**
 * gdata_gcontact_external_id_set_relation_type:
 * @self: a #GDataGContactExternalID
 * @relation_type: (allow-none): the new type for the external ID, or %NULL
 *
 * Sets the #GDataGContactExternalID:relation-type property to @relation_type,
 * such as %GDATA_GCONTACT_EXTERNAL_ID_NETWORK or %GDATA_GCONTACT_EXTERNAL_ID_ACCOUNT.
 *
 * If @relation_type is %NULL, the relation type will be unset. When the #GDataGContactExternalID is used in a query, however,
 * exactly one of #GDataGContactExternalID:relation-type and #GDataGContactExternalID:label must be %NULL.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_external_id_set_relation_type (GDataGContactExternalID *self, const gchar *relation_type)
{
	g_return_if_fail (GDATA_IS_GCONTACT_EXTERNAL_ID (self));
	g_return_if_fail (relation_type == NULL || *relation_type != '\0');

	g_free (self->priv->relation_type);
	self->priv->relation_type = g_strdup (relation_type);
	g_object_notify (G_OBJECT (self), "relation-type");
}

/**
 * gdata_gcontact_external_id_get_label:
 * @self: a #GDataGContactExternalID
 *
 * Gets the #GDataGContactExternalID:label property.
 *
 * Return value: a free-form label for the external ID, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_external_id_get_label (GDataGContactExternalID *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_EXTERNAL_ID (self), NULL);
	return self->priv->label;
}

/**
 * gdata_gcontact_external_id_set_label:
 * @self: a #GDataGContactExternalID
 * @label: (allow-none): the new free-form label for the external ID, or %NULL
 *
 * Sets the #GDataGContactExternalID:label property to @label.
 *
 * If @label is %NULL, the label will be unset. When the #GDataGContactExternalID is used in a query, however,
 * exactly one of #GDataGContactExternalID:relation-type and #GDataGContactExternalID:label must be %NULL.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_external_id_set_label (GDataGContactExternalID *self, const gchar *label)
{
	g_return_if_fail (GDATA_IS_GCONTACT_EXTERNAL_ID (self));
	g_return_if_fail (label == NULL || *label != '\0');

	g_free (self->priv->label);
	self->priv->label = g_strdup (label);
	g_object_notify (G_OBJECT (self), "label");
}
