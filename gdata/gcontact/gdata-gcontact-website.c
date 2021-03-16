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
 * SECTION:gdata-gcontact-website
 * @short_description: gContact website element
 * @stability: Stable
 * @include: gdata/gcontact/gdata-gcontact-website.h
 *
 * #GDataGContactWebsite represents a "website" element from the
 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcWebsite">gContact specification</ulink>.
 *
 * Since: 0.7.0
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-gcontact-website.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-comparable.h"

static void gdata_gcontact_website_comparable_init (GDataComparableIface *iface);
static void gdata_gcontact_website_finalize (GObject *object);
static void gdata_gcontact_website_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_gcontact_website_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataGContactWebsitePrivate {
	gchar *uri;
	gchar *relation_type;
	gchar *label;
	gboolean is_primary;
};

enum {
	PROP_URI = 1,
	PROP_RELATION_TYPE,
	PROP_LABEL,
	PROP_IS_PRIMARY
};

G_DEFINE_TYPE_WITH_CODE (GDataGContactWebsite, gdata_gcontact_website, GDATA_TYPE_PARSABLE,
                         G_ADD_PRIVATE (GDataGContactWebsite)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMPARABLE, gdata_gcontact_website_comparable_init))

static void
gdata_gcontact_website_class_init (GDataGContactWebsiteClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_gcontact_website_get_property;
	gobject_class->set_property = gdata_gcontact_website_set_property;
	gobject_class->finalize = gdata_gcontact_website_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->pre_get_xml = pre_get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "website";
	parsable_class->element_namespace = "gContact";

	/**
	 * GDataGContactWebsite:uri:
	 *
	 * The URI of the website.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcWebsite">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_URI,
	                                 g_param_spec_string ("uri",
	                                                      "URI", "The URI of the website.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGContactWebsite:relation-type:
	 *
	 * A programmatic value that identifies the type of website. Examples are %GDATA_GCONTACT_WEBSITE_HOME_PAGE or %GDATA_GCONTACT_WEBSITE_FTP.
	 * It is mutually exclusive with #GDataGContactWebsite:label.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcWebsite">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_RELATION_TYPE,
	                                 g_param_spec_string ("relation-type",
	                                                      "Relation type", "A programmatic value that identifies the type of website.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGContactWebsite:label:
	 *
	 * A simple string value used to name this website. It allows UIs to display a label such as "Work", "Travel blog", "Personal blog", etc.
	 * It is mutually exclusive with #GDataGContactWebsite:relation-type.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcWebsite">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_LABEL,
	                                 g_param_spec_string ("label",
	                                                      "Label", "A simple string value used to name this website.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGContactWebsite:is-primary:
	 *
	 * Indicates which website out of a group is primary.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcWebsite">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_IS_PRIMARY,
	                                 g_param_spec_boolean ("is-primary",
	                                                       "Primary?", "Indicates which website out of a group is primary.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static gint
compare_with (GDataComparable *self, GDataComparable *other)
{
	GDataGContactWebsitePrivate *a = ((GDataGContactWebsite*) self)->priv, *b = ((GDataGContactWebsite*) other)->priv;

	if (g_strcmp0 (a->uri, b->uri) == 0 && g_strcmp0 (a->relation_type, b->relation_type) == 0 && g_strcmp0 (a->label, b->label) == 0)
		return 0;
	return 1;
}

static void
gdata_gcontact_website_comparable_init (GDataComparableIface *iface)
{
	iface->compare_with = compare_with;
}

static void
gdata_gcontact_website_init (GDataGContactWebsite *self)
{
	self->priv = gdata_gcontact_website_get_instance_private (self);
}

static void
gdata_gcontact_website_finalize (GObject *object)
{
	GDataGContactWebsitePrivate *priv = GDATA_GCONTACT_WEBSITE (object)->priv;

	g_free (priv->uri);
	g_free (priv->relation_type);
	g_free (priv->label);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gcontact_website_parent_class)->finalize (object);
}

static void
gdata_gcontact_website_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataGContactWebsitePrivate *priv = GDATA_GCONTACT_WEBSITE (object)->priv;

	switch (property_id) {
		case PROP_URI:
			g_value_set_string (value, priv->uri);
			break;
		case PROP_RELATION_TYPE:
			g_value_set_string (value, priv->relation_type);
			break;
		case PROP_LABEL:
			g_value_set_string (value, priv->label);
			break;
		case PROP_IS_PRIMARY:
			g_value_set_boolean (value, priv->is_primary);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_gcontact_website_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataGContactWebsite *self = GDATA_GCONTACT_WEBSITE (object);

	switch (property_id) {
		case PROP_URI:
			gdata_gcontact_website_set_uri (self, g_value_get_string (value));
			break;
		case PROP_RELATION_TYPE:
			gdata_gcontact_website_set_relation_type (self, g_value_get_string (value));
			break;
		case PROP_LABEL:
			gdata_gcontact_website_set_label (self, g_value_get_string (value));
			break;
		case PROP_IS_PRIMARY:
			gdata_gcontact_website_set_is_primary (self, g_value_get_boolean (value));
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
	xmlChar *uri, *rel, *label;
	gboolean primary_bool;
	GDataGContactWebsitePrivate *priv = GDATA_GCONTACT_WEBSITE (parsable)->priv;

	/* Is it the primary website? */
	if (gdata_parser_boolean_from_property (root_node, "primary", &primary_bool, 0, error) == FALSE)
		return FALSE;

	uri = xmlGetProp (root_node, (xmlChar*) "href");
	if (uri == NULL || *uri == '\0') {
		xmlFree (uri);
		return gdata_parser_error_required_property_missing (root_node, "href", error);
	}

	/* NOTE: We allow both rel and label to be present when we should probably be asserting that they're mutually exclusive. See the comment in
	 * pre_get_xml() for details. */
	rel = xmlGetProp (root_node, (xmlChar*) "rel");
	label = xmlGetProp (root_node, (xmlChar*) "label");
	if ((rel == NULL || *rel == '\0') && (label == NULL || *label == '\0')) {
		xmlFree (rel);
		xmlFree (label);
		xmlFree (uri);
		return gdata_parser_error_required_property_missing (root_node, "rel", error);
	}

	priv->relation_type = (gchar*) rel;
	priv->label = (gchar*) label;
	priv->uri = (gchar*) uri;
	priv->is_primary = primary_bool;

	return TRUE;
}

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGContactWebsitePrivate *priv = GDATA_GCONTACT_WEBSITE (parsable)->priv;

	gdata_parser_string_append_escaped (xml_string, " href='", priv->uri, "'");

	/* NOTE: We previously allowed both rel and label to be set, making rel mandatory. Since bgo#659016, we treat rel and label as mutually
	 * exclusive attributes when parsing. We should treat them as mutually exclusive here as well, and g_assert_not_reached() if neither or
	 * both are set (as in gdata-gcontact-event.c:pre_get_xml()), but in order to maintain backwards compatibility, we don't.
	 * Sigh, Google. */
	if (priv->relation_type != NULL) {
		gdata_parser_string_append_escaped (xml_string, " rel='", priv->relation_type, "'");
	}

	if (priv->label != NULL) {
		gdata_parser_string_append_escaped (xml_string, " label='", priv->label, "'");
	}

	if (priv->is_primary == TRUE)
		g_string_append (xml_string, " primary='true'");
	else
		g_string_append (xml_string, " primary='false'");
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "gContact", (gchar*) "http://schemas.google.com/contact/2008");
}

/**
 * gdata_gcontact_website_new:
 * @uri: the website URI
 * @relation_type: the relationship between the website and its owner, or %NULL
 * @label: (allow-none): a human-readable label for the website, or %NULL
 * @is_primary: %TRUE if this website is its owner's primary website, %FALSE otherwise
 *
 * Creates a new #GDataGContactWebsite. More information is available in the <ulink type="http"
 * url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcWebsite">gContact specification</ulink>.
 *
 * Exactly one of @relation_type and @label should be provided; the other must be %NULL.
 *
 * Return value: a new #GDataGContactWebsite; unref with g_object_unref()
 *
 * Since: 0.7.0
 */
GDataGContactWebsite *
gdata_gcontact_website_new (const gchar *uri, const gchar *relation_type, const gchar *label, gboolean is_primary)
{
	g_return_val_if_fail (uri != NULL && *uri != '\0', NULL);
	g_return_val_if_fail ((relation_type != NULL && *relation_type != '\0'/* && label == NULL*/) ||
	                      (relation_type == NULL && label != NULL && *label != '\0'), NULL);
	/* NOTE: As in pre_get_xml(), we should treat rel and label as mutually exclusive here, but we can't for backwards compatibility reasons. */

	return g_object_new (GDATA_TYPE_GCONTACT_WEBSITE, "uri", uri, "relation-type", relation_type, "label", label, "is-primary", is_primary, NULL);
}

/**
 * gdata_gcontact_website_get_uri:
 * @self: a #GDataGContactWebsite
 *
 * Gets the #GDataGContactWebsite:uri property.
 *
 * Return value: the URI of the website
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_website_get_uri (GDataGContactWebsite *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_WEBSITE (self), NULL);
	return self->priv->uri;
}

/**
 * gdata_gcontact_website_set_uri:
 * @self: a #GDataGContactWebsite
 * @uri: the new website URI
 *
 * Sets the #GDataGContactWebsite:uri property to @uri.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_website_set_uri (GDataGContactWebsite *self, const gchar *uri)
{
	g_return_if_fail (GDATA_IS_GCONTACT_WEBSITE (self));
	g_return_if_fail (uri != NULL && *uri != '\0');

	g_free (self->priv->uri);
	self->priv->uri = g_strdup (uri);
	g_object_notify (G_OBJECT (self), "uri");
}

/**
 * gdata_gcontact_website_get_relation_type:
 * @self: a #GDataGContactWebsite
 *
 * Gets the #GDataGContactWebsite:relation-type property.
 *
 * Return value: the website's relation type
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_website_get_relation_type (GDataGContactWebsite *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_WEBSITE (self), NULL);
	return self->priv->relation_type;
}

/**
 * gdata_gcontact_website_set_relation_type:
 * @self: a #GDataGContactWebsite
 * @relation_type: the new relation type for the website
 *
 * Sets the #GDataGContactWebsite:relation-type property to @relation_type
 * such as %GDATA_GCONTACT_WEBSITE_HOME_PAGE or %GDATA_GCONTACT_WEBSITE_FTP.
 *
 * If @relation_type is %NULL, the relation type will be unset. When the #GDataGContactWebsite is used in a query, however,
 * exactly one of #GDataGContactWebsite:relation-type and #GDataGContactWebsite:label must be %NULL.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_website_set_relation_type (GDataGContactWebsite *self, const gchar *relation_type)
{
	g_return_if_fail (GDATA_IS_GCONTACT_WEBSITE (self));
	g_return_if_fail (relation_type == NULL || *relation_type != '\0');

	g_free (self->priv->relation_type);
	self->priv->relation_type = g_strdup (relation_type);
	g_object_notify (G_OBJECT (self), "relation-type");
}

/**
 * gdata_gcontact_website_get_label:
 * @self: a #GDataGContactWebsite
 *
 * Gets the #GDataGContactWebsite:label property.
 *
 * Return value: the website's label, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_website_get_label (GDataGContactWebsite *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_WEBSITE (self), NULL);
	return self->priv->label;
}

/**
 * gdata_gcontact_website_set_label:
 * @self: a #GDataGContactWebsite
 * @label: (allow-none): the new label for the website, or %NULL
 *
 * Sets the #GDataGContactWebsite:label property to @label.
 *
 * If @label is %NULL, the label will be unset. When the #GDataGContactWebsite is used in a query, however,
 * exactly one of #GDataGContactWebsite:relation-type and #GDataGContactWebsite:label must be %NULL.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_website_set_label (GDataGContactWebsite *self, const gchar *label)
{
	g_return_if_fail (GDATA_IS_GCONTACT_WEBSITE (self));
	/* NOTE: We should be validating using the code below, but we can't. See pre_get_xml() for details. */
	g_return_if_fail (label == NULL || *label != '\0');

	g_free (self->priv->label);
	self->priv->label = g_strdup (label);
	g_object_notify (G_OBJECT (self), "label");
}

/**
 * gdata_gcontact_website_is_primary:
 * @self: a #GDataGContactWebsite
 *
 * Gets the #GDataGContactWebsite:is-primary property.
 *
 * Return value: %TRUE if this is the primary website, %FALSE otherwise
 *
 * Since: 0.7.0
 */
gboolean
gdata_gcontact_website_is_primary (GDataGContactWebsite *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_WEBSITE (self), FALSE);
	return self->priv->is_primary;
}

/**
 * gdata_gcontact_website_set_is_primary:
 * @self: a #GDataGContactWebsite
 * @is_primary: %TRUE if this is the primary website, %FALSE otherwise
 *
 * Sets the #GDataGContactWebsite:is-primary property to @is_primary.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_website_set_is_primary (GDataGContactWebsite *self, gboolean is_primary)
{
	g_return_if_fail (GDATA_IS_GCONTACT_WEBSITE (self));

	self->priv->is_primary = is_primary;
	g_object_notify (G_OBJECT (self), "is-primary");
}
