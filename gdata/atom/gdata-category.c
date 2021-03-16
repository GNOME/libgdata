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
 * SECTION:gdata-category
 * @short_description: Atom category element
 * @stability: Stable
 * @include: gdata/atom/gdata-category.h
 *
 * #GDataCategory represents a "category" element from the
 * <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php">Atom specification</ulink>.
 *
 * Since: 0.4.0
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-category.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-comparable.h"

static void gdata_category_comparable_init (GDataComparableIface *iface);
static void gdata_category_finalize (GObject *object);
static void gdata_category_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_category_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);

struct _GDataCategoryPrivate {
	gchar *term;
	gchar *scheme;
	gchar *label;
};

enum {
	PROP_TERM = 1,
	PROP_SCHEME,
	PROP_LABEL
};

G_DEFINE_TYPE_WITH_CODE (GDataCategory, gdata_category, GDATA_TYPE_PARSABLE,
                         G_ADD_PRIVATE (GDataCategory)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMPARABLE, gdata_category_comparable_init))

static void
gdata_category_class_init (GDataCategoryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->set_property = gdata_category_set_property;
	gobject_class->get_property = gdata_category_get_property;
	gobject_class->finalize = gdata_category_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->pre_get_xml = pre_get_xml;
	parsable_class->element_name = "category";

	/**
	 * GDataCategory:term:
	 *
	 * Identifies the category to which the entry or feed belongs.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.category">
	 * Atom specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_TERM,
	                                 g_param_spec_string ("term",
	                                                      "Term", "Identifies the category to which the entry or feed belongs.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCategory:scheme:
	 *
	 * An IRI that identifies a categorization scheme.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.category">
	 * Atom specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_SCHEME,
	                                 g_param_spec_string ("scheme",
	                                                      "Scheme", "An IRI that identifies a categorization scheme.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCategory:label:
	 *
	 * A human-readable label for display in end-user applications.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.category">
	 * Atom specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_LABEL,
	                                 g_param_spec_string ("label",
	                                                      "Label", "A human-readable label for display in end-user applications.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static gint
compare_with (GDataComparable *self, GDataComparable *other)
{
	return g_strcmp0 (((GDataCategory*) self)->priv->term, ((GDataCategory*) other)->priv->term);
}

static void
gdata_category_comparable_init (GDataComparableIface *iface)
{
	iface->compare_with = compare_with;
}

static void
gdata_category_init (GDataCategory *self)
{
	self->priv = gdata_category_get_instance_private (self);
}

static void
gdata_category_finalize (GObject *object)
{
	GDataCategoryPrivate *priv = GDATA_CATEGORY (object)->priv;

	g_free (priv->term);
	g_free (priv->scheme);
	g_free (priv->label);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_category_parent_class)->finalize (object);
}

static void
gdata_category_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataCategoryPrivate *priv = GDATA_CATEGORY (object)->priv;

	switch (property_id) {
		case PROP_TERM:
			g_value_set_string (value, priv->term);
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
gdata_category_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataCategory *self = GDATA_CATEGORY (object);

	switch (property_id) {
		case PROP_TERM:
			gdata_category_set_term (self, g_value_get_string (value));
			break;
		case PROP_SCHEME:
			gdata_category_set_scheme (self, g_value_get_string (value));
			break;
		case PROP_LABEL:
			gdata_category_set_label (self, g_value_get_string (value));
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
	xmlChar *term, *scheme;
	GDataCategory *self = GDATA_CATEGORY (parsable);

	term = xmlGetProp (root_node, (xmlChar*) "term");
	if (term == NULL || *term == '\0') {
		xmlFree (term);
		return gdata_parser_error_required_property_missing (root_node, "term", error);
	}

	scheme = xmlGetProp (root_node, (xmlChar*) "scheme");
	if (scheme != NULL && *scheme == '\0') {
		xmlFree (term);
		xmlFree (scheme);
		return gdata_parser_error_required_property_missing (root_node, "scheme", error);
	}

	self->priv->term = (gchar*) term;
	self->priv->scheme = (gchar*) scheme;
	self->priv->label = (gchar*) xmlGetProp (root_node, (xmlChar*) "label");

	return TRUE;
}

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataCategoryPrivate *priv = GDATA_CATEGORY (parsable)->priv;

	gdata_parser_string_append_escaped (xml_string, " term='", priv->term, "'");

	if (priv->scheme != NULL)
		gdata_parser_string_append_escaped (xml_string, " scheme='", priv->scheme, "'");

	if (priv->label != NULL)
		gdata_parser_string_append_escaped (xml_string, " label='", priv->label, "'");
}

/**
 * gdata_category_new:
 * @term: a category identifier
 * @scheme: (allow-none): an IRI to define the categorisation scheme, or %NULL
 * @label: (allow-none): a human-readable label for the category, or %NULL
 *
 * Creates a new #GDataCategory. More information is available in the <ulink type="http"
 * url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.category">Atom specification</ulink>.
 *
 * @term must be non-%NULL and non-empty. @scheme must be %NULL or non-empty.
 *
 * Return value: a new #GDataCategory, or %NULL; unref with g_object_unref()
 */
GDataCategory *
gdata_category_new (const gchar *term, const gchar *scheme, const gchar *label)
{
	g_return_val_if_fail (term != NULL && *term != '\0', NULL);
	g_return_val_if_fail (scheme == NULL || *scheme != '\0', NULL);
	return g_object_new (GDATA_TYPE_CATEGORY, "term", term, "scheme", scheme, "label", label, NULL);
}

/**
 * gdata_category_get_term:
 * @self: a #GDataCategory
 *
 * Gets the #GDataCategory:term property. The term will always be a non-%NULL, non-empty string.
 *
 * Return value: the category's term
 *
 * Since: 0.4.0
 */
const gchar *
gdata_category_get_term (GDataCategory *self)
{
	g_return_val_if_fail (GDATA_IS_CATEGORY (self), NULL);
	return self->priv->term;
}

/**
 * gdata_category_set_term:
 * @self: a #GDataCategory
 * @term: the new term for the category
 *
 * Sets the #GDataCategory:term property to @term. @term must be non-%NULL and non-empty.
 *
 * Since: 0.4.0
 */
void
gdata_category_set_term (GDataCategory *self, const gchar *term)
{
	g_return_if_fail (GDATA_IS_CATEGORY (self));
	g_return_if_fail (term != NULL && *term != '\0');

	g_free (self->priv->term);
	self->priv->term = g_strdup (term);
	g_object_notify (G_OBJECT (self), "term");
}

/**
 * gdata_category_get_scheme:
 * @self: a #GDataCategory
 *
 * Gets the #GDataCategory:scheme property. If the scheme is non-%NULL, it will be non-empty.
 *
 * Return value: the category's scheme, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_category_get_scheme (GDataCategory *self)
{
	g_return_val_if_fail (GDATA_IS_CATEGORY (self), NULL);
	return self->priv->scheme;
}

/**
 * gdata_category_set_scheme:
 * @self: a #GDataCategory
 * @scheme: (allow-none): the new scheme for the category, or %NULL
 *
 * Sets the #GDataCategory:scheme property to @scheme. @scheme must be %NULL or non-empty.
 *
 * Set @scheme to %NULL to unset the property in the category.
 *
 * Since: 0.4.0
 */
void
gdata_category_set_scheme (GDataCategory *self, const gchar *scheme)
{
	g_return_if_fail (GDATA_IS_CATEGORY (self));

	g_free (self->priv->scheme);
	self->priv->scheme = g_strdup (scheme);
	g_object_notify (G_OBJECT (self), "scheme");
}

/**
 * gdata_category_get_label:
 * @self: a #GDataCategory
 *
 * Gets the #GDataCategory:label property.
 *
 * Return value: the category's label, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_category_get_label (GDataCategory *self)
{
	g_return_val_if_fail (GDATA_IS_CATEGORY (self), NULL);
	return self->priv->label;
}

/**
 * gdata_category_set_label:
 * @self: a #GDataCategory
 * @label: (allow-none): the new label for the category, or %NULL
 *
 * Sets the #GDataCategory:label property to @label.
 *
 * Set @label to %NULL to unset the property in the category.
 *
 * Since: 0.4.0
 */
void
gdata_category_set_label (GDataCategory *self, const gchar *label)
{
	g_return_if_fail (GDATA_IS_CATEGORY (self));

	g_free (self->priv->label);
	self->priv->label = g_strdup (label);
	g_object_notify (G_OBJECT (self), "label");
}
