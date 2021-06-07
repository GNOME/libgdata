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
 * SECTION:gdata-media-category
 * @short_description: Media RSS category element
 * @stability: Stable
 * @include: gdata/media/gdata-media-category.h
 *
 * #GDataMediaCategory represents a "category" element from the
 * <ulink type="http" url="http://video.search.yahoo.com/mrss">Media RSS specification</ulink>.
 *
 * Since: 0.4.0
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-media-category.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-types.h"

static void gdata_media_category_finalize (GObject *object);
static void gdata_media_category_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_media_category_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataMediaCategoryPrivate {
	gchar *category;
	gchar *scheme;
	gchar *label;
};

enum {
	PROP_CATEGORY = 1,
	PROP_SCHEME,
	PROP_LABEL
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataMediaCategory, gdata_media_category, GDATA_TYPE_PARSABLE)

static void
gdata_media_category_class_init (GDataMediaCategoryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_media_category_get_property;
	gobject_class->set_property = gdata_media_category_set_property;
	gobject_class->finalize = gdata_media_category_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->parse_xml = parse_xml;
	parsable_class->pre_get_xml = pre_get_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "category";
	parsable_class->element_namespace = "media";

	/**
	 * GDataMediaCategory:category:
	 *
	 * The category name.
	 *
	 * For more information, see the <ulink type="http" url="http://video.search.yahoo.com/mrss">Media RSS specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_CATEGORY,
	                                 g_param_spec_string ("category",
	                                                      "Category", "The category name.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataMediaCategory:scheme:
	 *
	 * A URI that identifies the categorization scheme.
	 *
	 * For more information, see the <ulink type="http" url="http://video.search.yahoo.com/mrss">Media RSS specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_SCHEME,
	                                 g_param_spec_string ("scheme",
	                                                      "Scheme", "A URI that identifies the categorization scheme.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataMediaCategory:label:
	 *
	 * A human-readable label that can be displayed in end-user applications.
	 *
	 * For more information, see the <ulink type="http" url="http://video.search.yahoo.com/mrss">Media RSS specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_LABEL,
	                                 g_param_spec_string ("label",
	                                                      "Label", "A human-readable label that can be displayed in end-user applications.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_media_category_init (GDataMediaCategory *self)
{
	self->priv = gdata_media_category_get_instance_private (self);
}

static void
gdata_media_category_finalize (GObject *object)
{
	GDataMediaCategoryPrivate *priv = GDATA_MEDIA_CATEGORY (object)->priv;

	g_free (priv->category);
	g_free (priv->scheme);
	g_free (priv->label);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_media_category_parent_class)->finalize (object);
}

static void
gdata_media_category_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataMediaCategoryPrivate *priv = GDATA_MEDIA_CATEGORY (object)->priv;

	switch (property_id) {
		case PROP_CATEGORY:
			g_value_set_string (value, priv->category);
			break;
		case PROP_SCHEME:
			g_value_set_string (value, priv->scheme);
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
gdata_media_category_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataMediaCategory *self = GDATA_MEDIA_CATEGORY (object);

	switch (property_id) {
		case PROP_CATEGORY:
			gdata_media_category_set_category (self, g_value_get_string (value));
			break;
		case PROP_SCHEME:
			gdata_media_category_set_scheme (self, g_value_get_string (value));
			break;
		case PROP_LABEL:
			gdata_media_category_set_label (self, g_value_get_string (value));
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
	GDataMediaCategoryPrivate *priv = GDATA_MEDIA_CATEGORY (parsable)->priv;
	xmlChar *category, *scheme;

	category = xmlNodeListGetString (doc, root_node->children, TRUE);
	if (category == NULL || *category == '\0') {
		xmlFree (category);
		return gdata_parser_error_required_content_missing (root_node, error);
	}

	scheme = xmlGetProp (root_node, (xmlChar*) "scheme");
	if (scheme != NULL && *scheme == '\0') {
		xmlFree (scheme);
		xmlFree (category);
		return gdata_parser_error_required_property_missing (root_node, "scheme", error);
	} else if (scheme == NULL) {
		/* Default */
		scheme = xmlStrdup ((xmlChar*) "http://video.search.yahoo.com/mrss/category_schema");
	}

	priv->category = (gchar*) category;
	priv->scheme = (gchar*) scheme;
	priv->label = (gchar*) xmlGetProp (root_node, (xmlChar*) "label");

	return TRUE;
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	/* Textual content's handled in pre_parse_xml */
	if (node->type != XML_ELEMENT_NODE)
		return TRUE;

	return GDATA_PARSABLE_CLASS (gdata_media_category_parent_class)->parse_xml (parsable, doc, node, user_data, error);
}

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataMediaCategoryPrivate *priv = GDATA_MEDIA_CATEGORY (parsable)->priv;

	if (priv->scheme != NULL)
		gdata_parser_string_append_escaped (xml_string, " scheme='", priv->scheme, "'");
	if (priv->label != NULL)
		gdata_parser_string_append_escaped (xml_string, " label='", priv->label, "'");
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	gdata_parser_string_append_escaped (xml_string, NULL, GDATA_MEDIA_CATEGORY (parsable)->priv->category, NULL);
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "media", (gchar*) "http://search.yahoo.com/mrss/");
}

/**
 * gdata_media_category_new:
 * @category: a category describing the content
 * @scheme: (allow-none): a URI identifying the categorisation scheme, or %NULL
 * @label: (allow-none): a human-readable name for the category, or %NULL
 *
 * Creates a new #GDataMediaCategory. More information is available in the <ulink type="http"
 * url="http://search.yahoo.com/mrss/">Media RSS specification</ulink>.
 *
 * Return value: a new #GDataMediaCategory, or %NULL; unref with g_object_unref()
 */
GDataMediaCategory *
gdata_media_category_new (const gchar *category, const gchar *scheme, const gchar *label)
{
	g_return_val_if_fail (category != NULL && *category != '\0', NULL);
	g_return_val_if_fail (scheme == NULL || *scheme != '\0', NULL);
	return g_object_new (GDATA_TYPE_MEDIA_CATEGORY, "category", category, "scheme", scheme, "label", label, NULL);
}

/**
 * gdata_media_category_get_category:
 * @self: a #GDataMediaCategory
 *
 * Gets the #GDataMediaCategory:category property.
 *
 * Return value: the actual category
 *
 * Since: 0.4.0
 */
const gchar *
gdata_media_category_get_category (GDataMediaCategory *self)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_CATEGORY (self), NULL);
	return self->priv->category;
}

/**
 * gdata_media_category_set_category:
 * @self: a #GDataMediaCategory
 * @category: the new category
 *
 * Sets the #GDataMediaCategory:category property to @category.
 *
 * Since: 0.4.0
 */
void
gdata_media_category_set_category (GDataMediaCategory *self, const gchar *category)
{
	g_return_if_fail (GDATA_IS_MEDIA_CATEGORY (self));
	g_return_if_fail (category != NULL && *category != '\0');

	g_free (self->priv->category);
	self->priv->category = g_strdup (category);
	g_object_notify (G_OBJECT (self), "category");
}

/**
 * gdata_media_category_get_scheme:
 * @self: a #GDataMediaCategory
 *
 * Gets the #GDataMediaCategory:scheme property.
 *
 * Return value: the category's scheme, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_media_category_get_scheme (GDataMediaCategory *self)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_CATEGORY (self), NULL);
	return self->priv->scheme;
}

/**
 * gdata_media_category_set_scheme:
 * @self: a #GDataMediaCategory
 * @scheme: (allow-none): the category's new scheme, or %NULL
 *
 * Sets the #GDataMediaCategory:scheme property to @scheme.
 *
 * Set @scheme to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_media_category_set_scheme (GDataMediaCategory *self, const gchar *scheme)
{
	g_return_if_fail (GDATA_IS_MEDIA_CATEGORY (self));
	g_return_if_fail (scheme == NULL || *scheme != '\0');

	if (scheme == NULL)
		scheme = "http://video.search.yahoo.com/mrss/category_schema";

	g_free (self->priv->scheme);
	self->priv->scheme = g_strdup (scheme);
	g_object_notify (G_OBJECT (self), "scheme");
}

/**
 * gdata_media_category_get_label:
 * @self: a #GDataMediaCategory
 *
 * Gets the #GDataMediaCategory:label property.
 *
 * Return value: the category's label, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_media_category_get_label (GDataMediaCategory *self)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_CATEGORY (self), NULL);
	return self->priv->label;
}

/**
 * gdata_media_category_set_label:
 * @self: a #GDataMediaCategory
 * @label: (allow-none): the category's new label, or %NULL
 *
 * Sets the #GDataMediaCategory:label property to @label.
 *
 * Set @label to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_media_category_set_label (GDataMediaCategory *self, const gchar *label)
{
	g_return_if_fail (GDATA_IS_MEDIA_CATEGORY (self));

	g_free (self->priv->label);
	self->priv->label = g_strdup (label);
	g_object_notify (G_OBJECT (self), "label");
}
