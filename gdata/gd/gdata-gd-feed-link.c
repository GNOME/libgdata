/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2010 <philip@tecnocode.co.uk>
 * Copyright (C) Richard Schwarting 2010 <aquarichy@gmail.com>
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

/*
 * SECTION:gdata-gd-feed-link
 * @short_description: GD feed link element
 * @stability: Stable
 * @include: gdata/gd/gdata-gd-feed-link.h
 *
 * #GDataGDFeedLink represents a "feedLink" element from the
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdFeedLink">GData specification</ulink>.
 *
 * It is private API, since implementing classes are likely to proxy the properties and functions
 * of #GDataGDFeedLink as appropriate; most entry types which implement #GDataGDFeedLink have no use
 * for most of its properties, and it would be unnecessary and confusing to expose #GDataGDFeedLink itself.
 *
 * In its current state, #GDataGDFeedLink supports the <code class="literal">href</code> attribute, but doesn't support inline
 * <code class="literal">feed</code> elements, since they don't seem to appear in the wild.
 *
 * Since: 0.10.0
 */

#include <glib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-gd-feed-link.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-private.h"

static void gdata_gd_feed_link_finalize (GObject *object);
static void gdata_gd_feed_link_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_gd_feed_link_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataGDFeedLinkPrivate {
	gchar *uri;
	gchar *relation_type;
	gint count_hint;
	gboolean is_read_only;
};

enum {
	PROP_RELATION_TYPE = 1,
	PROP_URI,
	PROP_COUNT_HINT,
	PROP_IS_READ_ONLY,
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataGDFeedLink, gdata_gd_feed_link, GDATA_TYPE_PARSABLE)

static void
gdata_gd_feed_link_class_init (GDataGDFeedLinkClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->set_property = gdata_gd_feed_link_set_property;
	gobject_class->get_property = gdata_gd_feed_link_get_property;
	gobject_class->finalize = gdata_gd_feed_link_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->pre_get_xml = pre_get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "feedLink";
	parsable_class->element_namespace = "gd";

	/**
	 * GDataGDFeedLink:relation-type:
	 *
	 * Describes the relation type of the given feed to its parent feed.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdFeedLink">GData specification</ulink>.
	 *
	 * Since: 0.10.0
	 */
	g_object_class_install_property (gobject_class, PROP_RELATION_TYPE,
	                                 g_param_spec_string ("relation-type",
	                                                      "Relation type", "Describes the relation type of the given feed to its parent feed.",
	                                                      GDATA_LINK_ALTERNATE,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDFeedLink:uri:
	 *
	 * A URI describing the location of the designated feed.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdFeedLink">GData specification</ulink>.
	 *
	 * Since: 0.10.0
	 */
	g_object_class_install_property (gobject_class, PROP_URI,
	                                 g_param_spec_string ("uri",
	                                                      "URI", "A URI describing the location of the designated feed.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDFeedLink:count-hint:
	 *
	 * Hints at the number of entries contained in the designated feed, and may not be reliable.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdFeedLink">GData specification</ulink>.
	 *
	 * Since: 0.10.0
	 */
	g_object_class_install_property (gobject_class, PROP_COUNT_HINT,
	                                 g_param_spec_int ("count-hint",
	                                                   "Count hint", "Hints at the number of entries contained in the designated feed.",
	                                                   -1, G_MAXINT, -1,
	                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDFeedLink:is-read-only:
	 *
	 * Indicates whether the feed is read only.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdFeedLink">GData specification</ulink>.
	 *
	 * Since: 0.10.0
	 */
	g_object_class_install_property (gobject_class, PROP_IS_READ_ONLY,
	                                 g_param_spec_boolean ("is-read-only",
	                                                       "Read only?", "Indicates whether the feed is read only.",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_gd_feed_link_init (GDataGDFeedLink *self)
{
	self->priv = gdata_gd_feed_link_get_instance_private (self);
	self->priv->count_hint = -1;
	self->priv->relation_type = g_strdup (GDATA_LINK_ALTERNATE);
}

static void
gdata_gd_feed_link_finalize (GObject *object)
{
	GDataGDFeedLinkPrivate *priv = GDATA_GD_FEED_LINK (object)->priv;

	g_free (priv->uri);
	g_free (priv->relation_type);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gd_feed_link_parent_class)->finalize (object);
}

static void
gdata_gd_feed_link_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataGDFeedLinkPrivate *priv = GDATA_GD_FEED_LINK (object)->priv;

	switch (property_id) {
		case PROP_RELATION_TYPE:
			g_value_set_string (value, priv->relation_type);
			break;
		case PROP_URI:
			g_value_set_string (value, priv->uri);
			break;
		case PROP_COUNT_HINT:
			g_value_set_int (value, priv->count_hint);
			break;
		case PROP_IS_READ_ONLY:
			g_value_set_boolean (value, priv->is_read_only);
			break;
		default:
			/* We don't have any other properties... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_gd_feed_link_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataGDFeedLink *self = GDATA_GD_FEED_LINK (object);

	switch (property_id) {
		case PROP_RELATION_TYPE:
			gdata_gd_feed_link_set_relation_type (self, g_value_get_string (value));
			break;
		case PROP_URI:
			gdata_gd_feed_link_set_uri (self, g_value_get_string (value));
			break;
		case PROP_COUNT_HINT:
			gdata_gd_feed_link_set_count_hint (self, g_value_get_int (value));
			break;
		case PROP_IS_READ_ONLY:
			gdata_gd_feed_link_set_is_read_only (self, g_value_get_boolean (value));
			break;
		default:
			/* We don't have any other properties... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static gboolean
pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error)
{
	xmlChar *rel, *href, *count_hint;
	GDataGDFeedLink *self = GDATA_GD_FEED_LINK (parsable);

	rel = xmlGetProp (root_node, (xmlChar*) "rel");
	if (rel != NULL && *rel == '\0') {
		xmlFree (rel);
		return gdata_parser_error_required_property_missing (root_node, "rel", error);
	}

	gdata_gd_feed_link_set_relation_type (self, (const gchar*) rel);
	xmlFree (rel);

	href = xmlGetProp (root_node, (xmlChar*) "href");
	if (href == NULL || *href == '\0') {
		xmlFree (href);
		return gdata_parser_error_required_property_missing (root_node, "href", error);
	}
	self->priv->uri = (gchar*) href;

	count_hint = xmlGetProp (root_node, (xmlChar*) "countHint");
	self->priv->count_hint = (count_hint != NULL) ? g_ascii_strtoll ((char*) count_hint, NULL, 10) : -1;
	xmlFree (count_hint);

	return gdata_parser_boolean_from_property (root_node, "readOnly", &(self->priv->is_read_only), 0, error);
}

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGDFeedLinkPrivate *priv = GDATA_GD_FEED_LINK (parsable)->priv;

	if (priv->relation_type != NULL) {
		gdata_parser_string_append_escaped (xml_string, " rel='", priv->relation_type, "'");
	}

	if (priv->uri != NULL) {
		gdata_parser_string_append_escaped (xml_string, " href='", priv->uri, "'");
	}

	if (priv->count_hint != -1) {
		g_string_append_printf (xml_string, " countHint='%i'", priv->count_hint);
	}

	if (priv->is_read_only == TRUE) {
		g_string_append_printf (xml_string, " readOnly='true'");
	} else {
		g_string_append_printf (xml_string, " readOnly='false'");
	}
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "gd", (gchar*) "http://schemas.google.com/g/2005");
}

/**
 * gdata_gd_feed_link_get_relation_type:
 * @self: a #GDataGDFeedLink
 *
 * Gets the #GDataGDFeedLink:relation-type property.
 *
 * Return value: the feed's relation to its owner, or %NULL
 *
 * Since: 0.10.0
 */
const gchar *
gdata_gd_feed_link_get_relation_type (GDataGDFeedLink *self)
{
	g_return_val_if_fail (GDATA_IS_GD_FEED_LINK (self), NULL);

	return self->priv->relation_type;
}

/**
 * gdata_gd_feed_link_get_uri:
 * @self: a #GDataGDFeedLink
 *
 * Gets the #GDataGDFeedLink:uri property.
 *
 * Return value: the related feed's URI
 *
 * Since: 0.10.0
 */
const gchar *
gdata_gd_feed_link_get_uri (GDataGDFeedLink *self)
{
	g_return_val_if_fail (GDATA_IS_GD_FEED_LINK (self), NULL);

	return self->priv->uri;
}

/**
 * gdata_gd_feed_link_get_count_hint:
 * @self: a #GDataGDFeedLink
 *
 * Gets the #GDataGDFeedLink:count-hint property.
 *
 * Return value: the potential number of entries in the related feed, or <code class="literal">-1</code> if not set
 *
 * Since: 0.10.0
 */
gint
gdata_gd_feed_link_get_count_hint (GDataGDFeedLink *self)
{
	g_return_val_if_fail (GDATA_IS_GD_FEED_LINK (self), -1);

	return self->priv->count_hint;
}

/**
 * gdata_gd_feed_link_get_read_only:
 * @self: a #GDataGDFeedLink
 *
 * Gets the #GDataGDFeedLink:read-only property.
 *
 * Return value: %TRUE if the feed is read only, %FALSE otherwise
 *
 * Since: 0.10.0
 */
gboolean
gdata_gd_feed_link_is_read_only (GDataGDFeedLink *self)
{
	g_return_val_if_fail (GDATA_IS_GD_FEED_LINK (self), FALSE);

	return self->priv->is_read_only;
}

/**
 * gdata_gd_feed_link_set_relation_type:
 * @self: a #GDataGDFeedLink
 * @relation_type: (allow-none): the new relation type between the feed and its owner, or %NULL
 *
 * Sets the relation type of the #GDataGDFeedLink's related feed to @relation_type. If @relation_type is one of the standard Atom relation types,
 * use one of the defined relation type values, instead of a static string. e.g. %GDATA_LINK_EDIT or %GDATA_LINK_SELF.
 *
 * Since: 0.10.0
 */
void
gdata_gd_feed_link_set_relation_type (GDataGDFeedLink *self, const gchar *relation_type)
{
	g_return_if_fail (GDATA_IS_GD_FEED_LINK (self));
	g_return_if_fail (relation_type == NULL || *relation_type != '\0');

	/* If the relation type is unset, use the default "alternate" relation type. If it's set, and isn't an IRI, turn it into an IRI
	 * by appending it to "http://www.iana.org/assignments/relation/". If it's set and is an IRI, just use the IRI.
	 * See: http://www.atomenabled.org/developers/syndication/atom-format-spec.php#rel_attribute
	 */
	g_free (self->priv->relation_type);
	if (relation_type == NULL) {
		self->priv->relation_type = g_strdup (GDATA_LINK_ALTERNATE);
	} else if (strchr ((char*) relation_type, ':') == NULL) {
		self->priv->relation_type = g_strconcat ("http://www.iana.org/assignments/relation/", (const gchar*) relation_type, NULL);
	} else {
		self->priv->relation_type = g_strdup ((gchar*) relation_type);
	}

	g_object_notify (G_OBJECT (self), "relation-type");
}

/**
 * gdata_gd_feed_link_set_uri:
 * @self: a #GDataGDFeedLink
 * @uri: the new URI for the related feed
 *
 * Sets the URI of the #GDataGDFeedLink's related feed to @uri.
 *
 * Since: 0.10.0
 */
void
gdata_gd_feed_link_set_uri (GDataGDFeedLink *self, const gchar *uri)
{
	g_return_if_fail (GDATA_IS_GD_FEED_LINK (self));
	g_return_if_fail (uri != NULL && *uri != '\0');

	g_free (self->priv->uri);
	self->priv->uri = g_strdup (uri);

	g_object_notify (G_OBJECT (self), "uri");
}

/**
 * gdata_gd_feed_link_set_count_hint:
 * @self: a #GDataGDFeedLink
 * @count_hint: the new number of entries in the related feed, or <code class="literal">-1</code> if unknown
 *
 * Sets the number of entries in the #GDataGDFeedLink's related feed to @count_hint. This number may be an imprecise estimate.
 *
 * Since: 0.10.0
 */
void
gdata_gd_feed_link_set_count_hint (GDataGDFeedLink *self, gint count_hint)
{
	g_return_if_fail (GDATA_IS_GD_FEED_LINK (self));
	g_return_if_fail (count_hint >= -1);

	self->priv->count_hint = count_hint;

	g_object_notify (G_OBJECT (self), "count-hint");
}

/**
 * gdata_gd_feed_link_set_is_read_only:
 * @self: a #GDataGDFeedLink
 * @is_read_only: %TRUE if the #GDataGDFeedLink's related feed is read only, %FALSE otherwise
 *
 * Sets the read only status of the #GDataGDFeedLink's related feed to @is_read_only.
 *
 * Since: 0.10.0
 */
void
gdata_gd_feed_link_set_is_read_only (GDataGDFeedLink *self, gboolean is_read_only)
{
	g_return_if_fail (GDATA_IS_GD_FEED_LINK (self));

	self->priv->is_read_only = is_read_only;

	g_object_notify (G_OBJECT (self), "is-read-only");
}

