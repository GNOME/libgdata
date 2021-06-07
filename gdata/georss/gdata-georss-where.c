/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009 <philip@tecnocode.co.uk>
 * Copyright (C) Richard Schwarting 2009 <aquarichy@gmail.com>
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
 * SECTION:gdata-georss-where
 * @short_description: GeoRSS where element
 * @stability: Stable
 * @include: gdata/georss/gdata-georss-where.h
 *
 * #GDataGeoRSSWhere represents a "where" element from the
 * <ulink type="http" url="http://www.georss.org/georss">GeoRSS specification</ulink>.
 * with PicasaWeb usage defined at
 * <ulink type="http" url="http://code.google.com/apis/picasaweb/docs/2.0/reference.html#georss_reference">PicasaWeb API reference</ulink>.
 *
 * It is private API, since implementing classes are likely to proxy the properties and functions
 * of #GDataGeoRSSWhere as appropriate; most entry types which implement #GDataGeoRSSWhere have no use
 * for most of its properties, and it would be unnecessary and confusing to expose #GDataGeoRSSWhere itself.
 *
 * Since: 0.5.0
 */

#include <glib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-georss-where.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-private.h"

static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);
static void get_xml (GDataParsable *parsable, GString *xml_string);

struct _GDataGeoRSSWherePrivate {
	gdouble latitude;
	gdouble longitude;
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataGeoRSSWhere, gdata_georss_where, GDATA_TYPE_PARSABLE)

static void
gdata_georss_where_class_init (GDataGeoRSSWhereClass *klass)
{
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	parsable_class->get_xml = get_xml;
	parsable_class->parse_xml = parse_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "where";
	parsable_class->element_namespace = "georss";
}

static void
gdata_georss_where_init (GDataGeoRSSWhere *self)
{
	self->priv = gdata_georss_where_get_instance_private (self);

	self->priv->latitude = G_MAXDOUBLE;
	self->priv->longitude = G_MAXDOUBLE;
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	GDataGeoRSSWhere *self = GDATA_GEORSS_WHERE (parsable);

	if (gdata_parser_is_namespace (node, "http://www.opengis.net/gml") == TRUE &&
	    xmlStrcmp (node->name, (xmlChar*) "Point") == 0) {
		/* gml:Point */
		gboolean found_pos = FALSE;
		xmlNode *child;

		for (child = node->children; child != NULL; child = child->next) {
			if (xmlStrcmp (child->name, (xmlChar*) "pos") == 0) {
				xmlChar *pos = xmlNodeListGetString (doc, child->children, TRUE);
				gchar *endptr;

				self->priv->latitude = g_ascii_strtod ((gchar*) pos, &endptr);
				self->priv->longitude = g_ascii_strtod (endptr, NULL);

				xmlFree (pos);
				found_pos = TRUE;
			} else {
				/* TODO: this logic copied from gdata-parsable.c.  Re-evaluate this at some point in the future.
				   If GeoRSS and GML support were to be used more widely, it might due to implement GML objects. */
				xmlBuffer *buffer;

				/* Unhandled XML */
				buffer = xmlBufferCreate ();
				xmlNodeDump (buffer, doc, child, 0, 0);
				g_debug ("Unhandled XML in <gml:Point>: %s", (gchar*) xmlBufferContent (buffer));
				xmlBufferFree (buffer);
			}
		}

		if (found_pos == FALSE)
			return gdata_parser_error_required_element_missing ("pos", "gml:Point", error);
		return TRUE;
	}

	return GDATA_PARSABLE_CLASS (gdata_georss_where_parent_class)->parse_xml (parsable, doc, node, user_data, error);
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGeoRSSWherePrivate *priv = GDATA_GEORSS_WHERE (parsable)->priv;
	gchar latitude_str[G_ASCII_DTOSTR_BUF_SIZE];
	gchar longitude_str[G_ASCII_DTOSTR_BUF_SIZE];

	if (priv->latitude != G_MAXDOUBLE && priv->longitude != G_MAXDOUBLE) {
		g_string_append_printf (xml_string, "<gml:Point><gml:pos>%s %s</gml:pos></gml:Point>",
		                        g_ascii_dtostr (latitude_str, sizeof (latitude_str), priv->latitude),
		                        g_ascii_dtostr (longitude_str, sizeof (longitude_str), priv->longitude));
	}
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "georss", (gchar*) "http://www.georss.org/georss");
	g_hash_table_insert (namespaces, (gchar*) "gml", (gchar*) "http://www.opengis.net/gml");
}

/**
 * gdata_georss_where_get_latitude:
 * @self: a #GDataGeoRSSWhere
 *
 * Gets the #GDataGeoRSSWhere:latitude property.
 *
 * Return value: the latitude of this position, or %G_MAXDOUBLE if unknown
 *
 * Since: 0.5.0
 */
gdouble
gdata_georss_where_get_latitude (GDataGeoRSSWhere *self)
{
	g_return_val_if_fail (GDATA_IS_GEORSS_WHERE (self), G_MAXDOUBLE);
	return self->priv->latitude;
}

/**
 * gdata_georss_where_get_longitude:
 * @self: a #GDataGeoRSSWhere
 *
 * Gets the #GDataGeoRSSWhere:longitude property.
 *
 * Return value: the longitude of this position, or %G_MAXDOUBLE if unknown
 *
 * Since: 0.5.0
 */
gdouble
gdata_georss_where_get_longitude (GDataGeoRSSWhere *self)
{
	g_return_val_if_fail (GDATA_IS_GEORSS_WHERE (self), G_MAXDOUBLE);
	return self->priv->longitude;
}

/**
 * gdata_georss_where_set_latitude:
 * @self: a #GDataGeoRSSWhere
 * @latitude: the new latitude coordinate, or %G_MAXDOUBLE
 *
 * Sets the #GDataGeoRSSWhere:latitude property to @latitude.
 *
 * Valid values range from <code class="literal">-90.0</code> to <code class="literal">90.0</code> inclusive.
 * Set @latitude to %G_MAXDOUBLE to unset it.
 *
 * Since: 0.5.0
 */
void
gdata_georss_where_set_latitude (GDataGeoRSSWhere *self, gdouble latitude)
{
	g_return_if_fail (GDATA_IS_GEORSS_WHERE (self));

	if (latitude < -90.0 || latitude > 90.0)
		self->priv->latitude = G_MAXDOUBLE;
	else
		self->priv->latitude = latitude;
}

/**
 * gdata_georss_where_set_longitude:
 * @self: a #GDataGeoRSSWhere
 * @longitude: the new longitude coordinate, or %G_MAXDOUBLE
 *
 * Sets the #GDataGeoRSSWhere:longitude property to @longitude.
 *
 * Valid values range from <code class="literal">-180.0</code> to <code class="literal">180.0</code> inclusive.
 * Set @longitude to %G_MAXDOUBLE to unset it.
 *
 * Since: 0.5.0
 */
void
gdata_georss_where_set_longitude (GDataGeoRSSWhere *self, gdouble longitude)
{
	g_return_if_fail (GDATA_IS_GEORSS_WHERE (self));

	if (longitude < -180.0 || longitude > 180.0)
		self->priv->longitude = G_MAXDOUBLE;
	else
		self->priv->longitude = longitude;
}
