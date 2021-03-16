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
 * SECTION:gdata-gcontact-jot
 * @short_description: gContact jot element
 * @stability: Stable
 * @include: gdata/gcontact/gdata-gcontact-jot.h
 *
 * #GDataGContactJot represents a "jot" element from the
 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcJot">gContact specification</ulink>.
 *
 * Since: 0.7.0
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-gcontact-jot.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"

static void gdata_gcontact_jot_finalize (GObject *object);
static void gdata_gcontact_jot_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_gcontact_jot_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataGContactJotPrivate {
	gchar *content;
	gchar *relation_type;
};

enum {
	PROP_CONTENT = 1,
	PROP_RELATION_TYPE
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataGContactJot, gdata_gcontact_jot, GDATA_TYPE_PARSABLE)

static void
gdata_gcontact_jot_class_init (GDataGContactJotClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_gcontact_jot_get_property;
	gobject_class->set_property = gdata_gcontact_jot_set_property;
	gobject_class->finalize = gdata_gcontact_jot_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->parse_xml = parse_xml;
	parsable_class->pre_get_xml = pre_get_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "jot";
	parsable_class->element_namespace = "gContact";

	/**
	 * GDataGContactJot:content:
	 *
	 * The content of the jot.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcJot">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_CONTENT,
	                                 g_param_spec_string ("content",
	                                                      "Content", "The content of the jot.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGContactJot:relation-type:
	 *
	 * A programmatic value that identifies the type of jot. Examples are %GDATA_GCONTACT_JOT_HOME or %GDATA_GCONTACT_JOT_OTHER.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcJot">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_RELATION_TYPE,
	                                 g_param_spec_string ("relation-type",
	                                                      "Relation type", "A programmatic value that identifies the type of jot.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_gcontact_jot_init (GDataGContactJot *self)
{
	self->priv = gdata_gcontact_jot_get_instance_private (self);
}

static void
gdata_gcontact_jot_finalize (GObject *object)
{
	GDataGContactJotPrivate *priv = GDATA_GCONTACT_JOT (object)->priv;

	g_free (priv->content);
	g_free (priv->relation_type);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gcontact_jot_parent_class)->finalize (object);
}

static void
gdata_gcontact_jot_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataGContactJotPrivate *priv = GDATA_GCONTACT_JOT (object)->priv;

	switch (property_id) {
		case PROP_CONTENT:
			g_value_set_string (value, priv->content);
			break;
		case PROP_RELATION_TYPE:
			g_value_set_string (value, priv->relation_type);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_gcontact_jot_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataGContactJot *self = GDATA_GCONTACT_JOT (object);

	switch (property_id) {
		case PROP_CONTENT:
			gdata_gcontact_jot_set_content (self, g_value_get_string (value));
			break;
		case PROP_RELATION_TYPE:
			gdata_gcontact_jot_set_relation_type (self, g_value_get_string (value));
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
	GDataGContactJotPrivate *priv = GDATA_GCONTACT_JOT (parsable)->priv;

	rel = xmlGetProp (root_node, (xmlChar*) "rel");
	if (rel == NULL || *rel == '\0') {
		xmlFree (rel);
		return gdata_parser_error_required_property_missing (root_node, "rel", error);
	}

	priv->content = (gchar*) xmlNodeListGetString (doc, root_node->children, TRUE);
	priv->relation_type = (gchar*) rel;

	return TRUE;
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	/* Textual content's handled in pre_parse_xml */
	if (node->type != XML_ELEMENT_NODE)
		return TRUE;

	return GDATA_PARSABLE_CLASS (gdata_gcontact_jot_parent_class)->parse_xml (parsable, doc, node, user_data, error);
}

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	gdata_parser_string_append_escaped (xml_string, " rel='", GDATA_GCONTACT_JOT (parsable)->priv->relation_type, "'");
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	gdata_parser_string_append_escaped (xml_string, NULL, GDATA_GCONTACT_JOT (parsable)->priv->content, NULL);
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "gContact", (gchar*) "http://schemas.google.com/contact/2008");
}

/**
 * gdata_gcontact_jot_new:
 * @content: the content of the jot
 * @relation_type: the relationship between the jot and its owner
 *
 * Creates a new #GDataGContactJot. More information is available in the <ulink type="http"
 * url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcJot">gContact specification</ulink>.
 *
 * Return value: a new #GDataGContactJot; unref with g_object_unref()
 *
 * Since: 0.7.0
 */
GDataGContactJot *
gdata_gcontact_jot_new (const gchar *content, const gchar *relation_type)
{
	g_return_val_if_fail (content != NULL, NULL);
	g_return_val_if_fail (relation_type != NULL && *relation_type != '\0', NULL);
	return g_object_new (GDATA_TYPE_GCONTACT_JOT, "content", content, "relation-type", relation_type, NULL);
}

/**
 * gdata_gcontact_jot_get_content:
 * @self: a #GDataGContactJot
 *
 * Gets the #GDataGContactJot:content property.
 *
 * Return value: the jot's content
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_jot_get_content (GDataGContactJot *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_JOT (self), NULL);
	return self->priv->content;
}

/**
 * gdata_gcontact_jot_set_content:
 * @self: a #GDataGContactJot
 * @content: the new content
 *
 * Sets the #GDataGContactJot:content property to @content.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_jot_set_content (GDataGContactJot *self, const gchar *content)
{
	g_return_if_fail (GDATA_IS_GCONTACT_JOT (self));
	g_return_if_fail (content != NULL);

	g_free (self->priv->content);
	self->priv->content = g_strdup (content);
	g_object_notify (G_OBJECT (self), "content");
}

/**
 * gdata_gcontact_jot_get_relation_type:
 * @self: a #GDataGContactJot
 *
 * Gets the #GDataGContactJot:relation-type property.
 *
 * Return value: the jot's relation type
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_jot_get_relation_type (GDataGContactJot *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_JOT (self), NULL);
	return self->priv->relation_type;
}

/**
 * gdata_gcontact_jot_set_relation_type:
 * @self: a #GDataGContactJot
 * @relation_type: the new relation type for the jot, or %NULL
 *
 * Sets the #GDataGContactJot:relation-type property to @relation_type
 * such as %GDATA_GCONTACT_JOT_HOME or %GDATA_GCONTACT_JOT_OTHER.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_jot_set_relation_type (GDataGContactJot *self, const gchar *relation_type)
{
	g_return_if_fail (GDATA_IS_GCONTACT_JOT (self));
	g_return_if_fail (relation_type != NULL && *relation_type != '\0');

	g_free (self->priv->relation_type);
	self->priv->relation_type = g_strdup (relation_type);
	g_object_notify (G_OBJECT (self), "relation-type");
}
