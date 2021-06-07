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
 * SECTION:gdata-gcontact-relation
 * @short_description: gContact relation element
 * @stability: Stable
 * @include: gdata/gcontact/gdata-gcontact-relation.h
 *
 * #GDataGContactRelation represents a "relation" element from the
 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcJot">gContact specification</ulink>.
 *
 * Since: 0.7.0
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-gcontact-relation.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"

static void gdata_gcontact_relation_finalize (GObject *object);
static void gdata_gcontact_relation_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_gcontact_relation_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataGContactRelationPrivate {
	gchar *name;
	gchar *relation_type;
	gchar *label;
};

enum {
	PROP_NAME = 1,
	PROP_RELATION_TYPE,
	PROP_LABEL
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataGContactRelation, gdata_gcontact_relation, GDATA_TYPE_PARSABLE)

static void
gdata_gcontact_relation_class_init (GDataGContactRelationClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_gcontact_relation_get_property;
	gobject_class->set_property = gdata_gcontact_relation_set_property;
	gobject_class->finalize = gdata_gcontact_relation_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->parse_xml = parse_xml;
	parsable_class->pre_get_xml = pre_get_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "relation";
	parsable_class->element_namespace = "gContact";

	/**
	 * GDataGContactRelation:name:
	 *
	 * The name of the relation. It need not be a full name, and there need not be a contact representing the name.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_NAME,
	                                 g_param_spec_string ("name",
	                                                      "Name", "The name of the relation.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGContactRelation:relation-type:
	 *
	 * A programmatic value that identifies the type of relation. It is mutually exclusive with #GDataGContactRelation:label.
	 * Examples are %GDATA_GCONTACT_RELATION_MANAGER or %GDATA_GCONTACT_RELATION_CHILD.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_RELATION_TYPE,
	                                 g_param_spec_string ("relation-type",
	                                                      "Relation type", "A programmatic value that identifies the type of relation.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGContactRelation:label:
	 *
	 * A free-form string that identifies the type of relation. It is mutually exclusive with #GDataGContactRelation:relation-type.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_LABEL,
	                                 g_param_spec_string ("label",
	                                                      "Label", "A free-form string that identifies the type of relation.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_gcontact_relation_init (GDataGContactRelation *self)
{
	self->priv = gdata_gcontact_relation_get_instance_private (self);
}

static void
gdata_gcontact_relation_finalize (GObject *object)
{
	GDataGContactRelationPrivate *priv = GDATA_GCONTACT_RELATION (object)->priv;

	g_free (priv->name);
	g_free (priv->relation_type);
	g_free (priv->label);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gcontact_relation_parent_class)->finalize (object);
}

static void
gdata_gcontact_relation_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataGContactRelationPrivate *priv = GDATA_GCONTACT_RELATION (object)->priv;

	switch (property_id) {
		case PROP_NAME:
			g_value_set_string (value, priv->name);
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
gdata_gcontact_relation_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataGContactRelation *self = GDATA_GCONTACT_RELATION (object);

	switch (property_id) {
		case PROP_NAME:
			gdata_gcontact_relation_set_name (self, g_value_get_string (value));
			break;
		case PROP_RELATION_TYPE:
			gdata_gcontact_relation_set_relation_type (self, g_value_get_string (value));
			break;
		case PROP_LABEL:
			gdata_gcontact_relation_set_label (self, g_value_get_string (value));
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
	xmlChar *rel, *label, *name;
	GDataGContactRelationPrivate *priv = GDATA_GCONTACT_RELATION (parsable)->priv;

	rel = xmlGetProp (root_node, (xmlChar*) "rel");
	label = xmlGetProp (root_node, (xmlChar*) "label");
	if ((rel == NULL || *rel == '\0') && (label == NULL || *label == '\0')) {
		xmlFree (rel);
		xmlFree (label);
		return gdata_parser_error_required_property_missing (root_node, "rel", error);
	} else if (rel != NULL && label != NULL) {
		/* Can't have both set at once */
		xmlFree (rel);
		xmlFree (label);
		return gdata_parser_error_mutexed_properties (root_node, "rel", "label", error);
	}

	/* Get the name */
	name = xmlNodeListGetString (doc, root_node->children, TRUE);
	if (name == NULL || *name == '\0') {
		xmlFree (rel);
		xmlFree (label);
		xmlFree (name);
		return gdata_parser_error_required_content_missing (root_node, error);
	}

	priv->name = (gchar*) name;
	priv->relation_type = (gchar*) rel;
	priv->label = (gchar*) label;

	return TRUE;
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	/* Textual content's handled in pre_parse_xml */
	if (node->type != XML_ELEMENT_NODE)
		return TRUE;

	return GDATA_PARSABLE_CLASS (gdata_gcontact_relation_parent_class)->parse_xml (parsable, doc, node, user_data, error);
}

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGContactRelationPrivate *priv = GDATA_GCONTACT_RELATION (parsable)->priv;

	if (priv->relation_type != NULL)
		gdata_parser_string_append_escaped (xml_string, " rel='", priv->relation_type, "'");
	else if (priv->label != NULL)
		gdata_parser_string_append_escaped (xml_string, " label='", priv->label, "'");
	else
		g_assert_not_reached ();
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	gdata_parser_string_append_escaped (xml_string, NULL, GDATA_GCONTACT_RELATION (parsable)->priv->name, NULL);
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "gContact", (gchar*) "http://schemas.google.com/contact/2008");
}

/**
 * gdata_gcontact_relation_new:
 * @name: the name of the relation
 * @relation_type: (allow-none): the type of relation, or %NULL
 * @label: (allow-none): a free-form label for the type of relation, or %NULL
 *
 * Creates a new #GDataGContactRelation. More information is available in the <ulink type="http"
 * url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">gContact specification</ulink>.
 *
 * Exactly one of @relation_type and @label should be provided; the other must be %NULL.
 *
 * Return value: a new #GDataGContactRelation; unref with g_object_unref()
 *
 * Since: 0.7.0
 */
GDataGContactRelation *
gdata_gcontact_relation_new (const gchar *name, const gchar *relation_type, const gchar *label)
{
	g_return_val_if_fail (name != NULL && *name != '\0', NULL);
	g_return_val_if_fail ((relation_type != NULL && *relation_type != '\0' && label == NULL) ||
	                      (relation_type == NULL && label != NULL && *label != '\0'), NULL);

	return g_object_new (GDATA_TYPE_GCONTACT_RELATION, "name", name, "relation-type", relation_type, "label", label, NULL);
}

/**
 * gdata_gcontact_relation_get_name:
 * @self: a #GDataGContactRelation
 *
 * Gets the #GDataGContactRelation:name property.
 *
 * Return value: the relation's name
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_relation_get_name (GDataGContactRelation *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_RELATION (self), NULL);
	return self->priv->name;
}

/**
 * gdata_gcontact_relation_set_name:
 * @self: a #GDataGContactRelation
 * @name: (allow-none): the new name for the relation
 *
 * Sets the #GDataGContactRelation:name property to @name.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_relation_set_name (GDataGContactRelation *self, const gchar *name)
{
	g_return_if_fail (GDATA_IS_GCONTACT_RELATION (self));
	g_return_if_fail (name != NULL && *name != '\0');

	g_free (self->priv->name);
	self->priv->name = g_strdup (name);
	g_object_notify (G_OBJECT (self), "name");
}

/**
 * gdata_gcontact_relation_get_relation_type:
 * @self: a #GDataGContactRelation
 *
 * Gets the #GDataGContactRelation:relation-type property.
 *
 * Return value: the type of the relation, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_relation_get_relation_type (GDataGContactRelation *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_RELATION (self), NULL);
	return self->priv->relation_type;
}

/**
 * gdata_gcontact_relation_set_relation_type:
 * @self: a #GDataGContactRelation
 * @relation_type: (allow-none): the new type for the relation, or %NULL
 *
 * Sets the #GDataGContactRelation:relation-type property to @relation_type,
 * such as %GDATA_GCONTACT_RELATION_MANAGER or %GDATA_GCONTACT_RELATION_CHILD.
 *
 * If @relation_type is %NULL, the relation type will be unset. When the #GDataGContactRelation is used in a query, however,
 * exactly one of #GDataGContactRelation:relation-type and #GDataGContactRelation:label must be %NULL.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_relation_set_relation_type (GDataGContactRelation *self, const gchar *relation_type)
{
	g_return_if_fail (GDATA_IS_GCONTACT_RELATION (self));
	g_return_if_fail (relation_type == NULL || *relation_type != '\0');

	g_free (self->priv->relation_type);
	self->priv->relation_type = g_strdup (relation_type);
	g_object_notify (G_OBJECT (self), "relation-type");
}

/**
 * gdata_gcontact_relation_get_label:
 * @self: a #GDataGContactRelation
 *
 * Gets the #GDataGContactRelation:label property.
 *
 * Return value: a free-form label for the type of the relation, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_relation_get_label (GDataGContactRelation *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_RELATION (self), NULL);
	return self->priv->label;
}

/**
 * gdata_gcontact_relation_set_label:
 * @self: a #GDataGContactRelation
 * @label: (allow-none): the new free-form type for the relation, or %NULL
 *
 * Sets the #GDataGContactRelation:label property to @label.
 *
 * If @label is %NULL, the label will be unset. When the #GDataGContactRelation is used in a query, however,
 * exactly one of #GDataGContactRelation:relation-type and #GDataGContactRelation:label must be %NULL.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_relation_set_label (GDataGContactRelation *self, const gchar *label)
{
	g_return_if_fail (GDATA_IS_GCONTACT_RELATION (self));
	g_return_if_fail (label == NULL || *label != '\0');

	g_free (self->priv->label);
	self->priv->label = g_strdup (label);
	g_object_notify (G_OBJECT (self), "label");
}
