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
 * SECTION:gdata-generator
 * @short_description: Atom generator element
 * @stability: Stable
 * @include: gdata/atom/gdata-generator.h
 *
 * #GDataGenerator represents a "generator" element from the
 * <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php">Atom specification</ulink>.
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-generator.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-comparable.h"

static void gdata_generator_comparable_init (GDataComparableIface *iface);
static void gdata_generator_finalize (GObject *object);
static void gdata_generator_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);

struct _GDataGeneratorPrivate {
	gchar *name;
	gchar *uri;
	gchar *version;
};

enum {
	PROP_NAME = 1,
	PROP_URI,
	PROP_VERSION
};

G_DEFINE_TYPE_WITH_CODE (GDataGenerator, gdata_generator, GDATA_TYPE_PARSABLE,
                         G_ADD_PRIVATE (GDataGenerator)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMPARABLE, gdata_generator_comparable_init))

static void
gdata_generator_class_init (GDataGeneratorClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_generator_get_property;
	gobject_class->finalize = gdata_generator_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->parse_xml = parse_xml;
	parsable_class->element_name = "generator";

	/**
	 * GDataGenerator:name:
	 *
	 * A human-readable name for the generating agent.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.generator">
	 * Atom specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_NAME,
	                                 g_param_spec_string ("name",
	                                                      "Name", "A human-readable name for the generating agent.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGenerator:uri:
	 *
	 * An IRI reference that is relevant to the agent.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.generator">
	 * Atom specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_URI,
	                                 g_param_spec_string ("uri",
	                                                      "URI", "An IRI reference that is relevant to the agent.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGenerator:version:
	 *
	 * Indicates the version of the generating agent.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.generator">
	 * Atom specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_VERSION,
	                                 g_param_spec_string ("version",
	                                                      "Version", "Indicates the version of the generating agent.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static gint
compare_with (GDataComparable *self, GDataComparable *other)
{
	return g_strcmp0 (((GDataGenerator*) self)->priv->name, ((GDataGenerator*) other)->priv->name);
}

static void
gdata_generator_comparable_init (GDataComparableIface *iface)
{
	iface->compare_with = compare_with;
}

static void
gdata_generator_init (GDataGenerator *self)
{
	self->priv = gdata_generator_get_instance_private (self);
}

static void
gdata_generator_finalize (GObject *object)
{
	GDataGeneratorPrivate *priv = GDATA_GENERATOR (object)->priv;

	g_free (priv->name);
	g_free (priv->uri);
	g_free (priv->version);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_generator_parent_class)->finalize (object);
}

static void
gdata_generator_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataGeneratorPrivate *priv = GDATA_GENERATOR (object)->priv;

	switch (property_id) {
		case PROP_NAME:
			g_value_set_string (value, priv->name);
			break;
		case PROP_URI:
			g_value_set_string (value, priv->uri);
			break;
		case PROP_VERSION:
			g_value_set_string (value, priv->version);
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
	xmlChar *uri, *name;
	GDataGeneratorPrivate *priv = GDATA_GENERATOR (parsable)->priv;

	uri = xmlGetProp (root_node, (xmlChar*) "uri");
	if (uri != NULL && *uri == '\0') {
		xmlFree (uri);
		return gdata_parser_error_required_property_missing (root_node, "uri", error);
	}

	name = xmlNodeListGetString (doc, root_node->children, TRUE);
	if (name != NULL && *name == '\0') {
		xmlFree (uri);
		xmlFree (name);
		return gdata_parser_error_required_content_missing (root_node, error);
	}

	priv->uri = (gchar*) uri;
	priv->name = (gchar*) name;
	priv->version = (gchar*) xmlGetProp (root_node, (xmlChar*) "version");

	return TRUE;
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	/* Textual content's handled in pre_parse_xml */
	if (node->type != XML_ELEMENT_NODE)
		return TRUE;

	return GDATA_PARSABLE_CLASS (gdata_generator_parent_class)->parse_xml (parsable, doc, node, user_data, error);
}

/**
 * gdata_generator_get_name:
 * @self: a #GDataGenerator
 *
 * Gets the #GDataGenerator:name property. The name will be %NULL or non-empty.
 *
 * Return value: (nullable): the generator's name
 *
 * Since: 0.4.0
 */
const gchar *
gdata_generator_get_name (GDataGenerator *self)
{
	g_return_val_if_fail (GDATA_IS_GENERATOR (self), NULL);
	return self->priv->name;
}

/**
 * gdata_generator_get_uri:
 * @self: a #GDataGenerator
 *
 * Gets the #GDataGenerator:uri property. The URI will be %NULL or non-empty.
 *
 * Return value: (nullable): the generator's URI, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_generator_get_uri (GDataGenerator *self)
{
	g_return_val_if_fail (GDATA_IS_GENERATOR (self), NULL);
	return self->priv->uri;
}

/**
 * gdata_generator_get_version:
 * @self: a #GDataGenerator
 *
 * Gets the #GDataGenerator:version property.
 *
 * Return value: the generator's version, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_generator_get_version (GDataGenerator *self)
{
	g_return_val_if_fail (GDATA_IS_GENERATOR (self), NULL);
	return self->priv->version;
}
