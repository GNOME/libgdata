/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Richard Schwarting 2009 <aquarichy@gmail.com>
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
 * SECTION:gdata-picasaweb-query
 * @short_description: GData PicasaWeb query object
 * @stability: Stable
 * @include: gdata/services/picasaweb/gdata-picasaweb-query.h
 *
 * #GDataPicasaWebQuery represents a collection of query parameters specific to the Google PicasaWeb service, which go above and beyond
 * those catered for by #GDataQuery.
 *
 * For more information on the custom GData query parameters supported by #GDataPicasaWebQuery, see the <ulink type="http"
 * url="http://code.google.com/apis/picasaweb/reference.html#Parameters">online documentation</ulink>.
 *
 * Since: 0.4.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-picasaweb-query.h"
#include "gdata-query.h"
#include "gdata-picasaweb-enums.h"
#include "gdata-private.h"

static void gdata_picasaweb_query_finalize (GObject *object);
static void gdata_picasaweb_query_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_picasaweb_query_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void get_query_uri (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started);

struct _GDataPicasaWebQueryPrivate {
	GDataPicasaWebVisibility visibility;
	gchar *thumbnail_size;
	gchar *image_size;
	gchar *tag;
	gchar *location;

	struct {
		gdouble north;
		gdouble east;
		gdouble south;
		gdouble west;
	} bounding_box;
};

enum {
	PROP_VISIBILITY = 1,
	PROP_THUMBNAIL_SIZE,
	PROP_IMAGE_SIZE,
	PROP_TAG,
	PROP_LOCATION
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataPicasaWebQuery, gdata_picasaweb_query, GDATA_TYPE_QUERY)

static void
gdata_picasaweb_query_class_init (GDataPicasaWebQueryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataQueryClass *query_class = GDATA_QUERY_CLASS (klass);

	gobject_class->get_property = gdata_picasaweb_query_get_property;
	gobject_class->set_property = gdata_picasaweb_query_set_property;
	gobject_class->finalize = gdata_picasaweb_query_finalize;

	query_class->get_query_uri = get_query_uri;

	/**
	 * GDataPicasaWebQuery:visibility:
	 *
	 * Specifies which albums should be listed, in terms of their visibility (#GDataPicasaWebAlbum:visibility).
	 *
	 * Set the property to <code class="literal">0</code> to list all albums, regardless of their visibility. Otherwise, use values
	 * from #GDataPicasaWebVisibility.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#Visibility">
	 * online documentation</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_VISIBILITY,
	                                 g_param_spec_int ("visibility",
	                                                   "Visibility", "Specifies which albums should be listed, in terms of their visibility.",
	                                                   0, GDATA_PICASAWEB_PRIVATE, 0,
	                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebQuery:thumbnail-size:
	 *
	 * A comma-separated list of thumbnail widths (in pixels) to return. Only certain sizes are allowed, and whether the thumbnail should be
	 * cropped or scaled can be specified; for more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#Parameters">online documentation</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_THUMBNAIL_SIZE,
	                                 g_param_spec_string ("thumbnail-size",
	                                                      "Thumbnail size", "A comma-separated list of thumbnail width (in pixels) to return.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebQuery:image-size:
	 *
	 * A comma-separated list of image sizes (width in pixels) to return. Only certain sizes are allowed, and whether the image should be
	 * cropped or scaled can be specified; for more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#Parameters">online documentation</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_IMAGE_SIZE,
	                                 g_param_spec_string ("image-size",
	                                                      "Image size", "A comma-separated list of image sizes (width in pixels) to return.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebQuery:tag:
	 *
	 * A tag which returned results must contain.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_TAG,
	                                 g_param_spec_string ("tag",
	                                                      "Tag", "A tag which returned results must contain.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebQuery:location:
	 *
	 * A location to search for photos, e.g. "London".
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_LOCATION,
	                                 g_param_spec_string ("location",
	                                                      "Location", "A location to search for photos, e.g. \"London\".",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_picasaweb_query_init (GDataPicasaWebQuery *self)
{
	self->priv = gdata_picasaweb_query_get_instance_private (self);

	/* https://developers.google.com/picasa-web/docs/3.0/reference#Parameters */
	_gdata_query_set_pagination_type (GDATA_QUERY (self),
	                                  GDATA_QUERY_PAGINATION_INDEXED);
}

static void
gdata_picasaweb_query_finalize (GObject *object)
{
	GDataPicasaWebQueryPrivate *priv = GDATA_PICASAWEB_QUERY (object)->priv;

	g_free (priv->thumbnail_size);
	g_free (priv->image_size);
	g_free (priv->tag);
	g_free (priv->location);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_picasaweb_query_parent_class)->finalize (object);
}

static void
gdata_picasaweb_query_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataPicasaWebQueryPrivate *priv = GDATA_PICASAWEB_QUERY (object)->priv;

	switch (property_id) {
		case PROP_VISIBILITY:
			g_value_set_int (value, priv->visibility);
			break;
		case PROP_THUMBNAIL_SIZE:
			g_value_set_string (value, priv->thumbnail_size);
			break;
		case PROP_IMAGE_SIZE:
			g_value_set_string (value, priv->image_size);
			break;
		case PROP_TAG:
			g_value_set_string (value, priv->tag);
			break;
		case PROP_LOCATION:
			g_value_set_string (value, priv->location);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_picasaweb_query_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataPicasaWebQuery *self = GDATA_PICASAWEB_QUERY (object);

	switch (property_id) {
		case PROP_VISIBILITY:
			gdata_picasaweb_query_set_visibility (self, g_value_get_int (value));
			break;
		case PROP_THUMBNAIL_SIZE:
			gdata_picasaweb_query_set_thumbnail_size (self, g_value_get_string (value));
			break;
		case PROP_IMAGE_SIZE:
			gdata_picasaweb_query_set_image_size (self, g_value_get_string (value));
			break;
		case PROP_TAG:
			gdata_picasaweb_query_set_tag (self, g_value_get_string (value));
			break;
		case PROP_LOCATION:
			gdata_picasaweb_query_set_location (self, g_value_get_string (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
get_query_uri (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started)
{
	GDataPicasaWebQueryPrivate *priv = GDATA_PICASAWEB_QUERY (self)->priv;

	#define APPEND_SEP g_string_append_c (query_uri, (*params_started == FALSE) ? '?' : '&'); *params_started = TRUE;

	/* Chain up to the parent class */
	GDATA_QUERY_CLASS (gdata_picasaweb_query_parent_class)->get_query_uri (self, feed_uri, query_uri, params_started);

	APPEND_SEP
	if (priv->visibility == 0)
		; // Appending nothing to retrieve all for authenticated users and just public albums for unauthenticated
	else if (priv->visibility == GDATA_PICASAWEB_PUBLIC)
		g_string_append (query_uri, "access=public");
	else if (priv->visibility == GDATA_PICASAWEB_PRIVATE)
		g_string_append (query_uri, "access=private");
	else
		g_assert_not_reached ();

	if (priv->thumbnail_size != NULL) {
		APPEND_SEP
		g_string_append (query_uri, "thumbsize=");
		g_string_append_uri_escaped (query_uri, priv->thumbnail_size, NULL, FALSE);
	}

	if (priv->image_size != NULL) {
		APPEND_SEP
		g_string_append (query_uri, "imgmax=");
		g_string_append_uri_escaped (query_uri, priv->image_size, NULL, FALSE);
	}

	if (priv->tag != NULL) {
		APPEND_SEP
		g_string_append (query_uri, "tag=");
		g_string_append_uri_escaped (query_uri, priv->tag, NULL, FALSE);
	}

	if (priv->bounding_box.north != priv->bounding_box.south && priv->bounding_box.east != priv->bounding_box.west) {
		gchar west[G_ASCII_DTOSTR_BUF_SIZE], south[G_ASCII_DTOSTR_BUF_SIZE], east[G_ASCII_DTOSTR_BUF_SIZE], north[G_ASCII_DTOSTR_BUF_SIZE];

		APPEND_SEP
		g_string_append_printf (query_uri, "bbox=%s,%s,%s,%s",
		                        g_ascii_dtostr (west, sizeof (west), priv->bounding_box.west),
		                        g_ascii_dtostr (south, sizeof (south), priv->bounding_box.south),
		                        g_ascii_dtostr (east, sizeof (east), priv->bounding_box.east),
		                        g_ascii_dtostr (north, sizeof (north), priv->bounding_box.north));
	}

	if (priv->location != NULL) {
		APPEND_SEP
		g_string_append (query_uri, "l=");
		g_string_append_uri_escaped (query_uri, priv->location, NULL, FALSE);
	}
}

/**
 * gdata_picasaweb_query_new:
 * @q: (allow-none): a query string, or %NULL
 *
 * Creates a new #GDataPicasaWebQuery with its #GDataQuery:q property set to @q.
 *
 * Note that when querying for albums with gdata_picasaweb_service_query_all_albums(), the @q parameter cannot be used.
 *
 * Return value: a new #GDataPicasaWebQuery
 *
 * Since: 0.4.0
 */
GDataPicasaWebQuery *
gdata_picasaweb_query_new (const gchar *q)
{
	return g_object_new (GDATA_TYPE_PICASAWEB_QUERY, "q", q, NULL);
}

/**
 * gdata_picasaweb_query_new_with_limits:
 * @q: (allow-none): a query string, or %NULL
 * @start_index: the index of the first result to include, or <code class="literal">0</code>
 * @max_results: the maximum number of results to include, or <code class="literal">0</code>
 *
 * Creates a #GDataPicasaWebQuery with its #GDataQuery:q property set to @q, returning @max_results starting from the @start_index<!-- -->th result.
 *
 * Note that when querying for albums with gdata_picasaweb_service_query_all_albums(), the @q parameter cannot be used.
 *
 * This is useful for paging through results, but the result set between separate queries may change. So, if you use this to
 * request the next ten results after a previous query, it may include some of the previously returned results if their order changed, or
 * omit ones that would have otherwise been found in a earlier but larger query.
 *
 * Return value: a new #GDataPicasaWebQuery
 *
 * Since: 0.6.0
 */
GDataPicasaWebQuery *
gdata_picasaweb_query_new_with_limits (const gchar *q, guint start_index, guint max_results)
{
	return g_object_new (GDATA_TYPE_PICASAWEB_QUERY,
	                     "q", q,
	                     "start-index", start_index,
	                     "max-results", max_results,
	                     NULL);
}

/**
 * gdata_picasaweb_query_get_visibility:
 * @self: a #GDataPicasaWebQuery
 *
 * Gets the #GDataPicasaWebQuery:visibility property.
 *
 * Return value: the visibility of the objects to retrieve, or <code class="literal">0</code> to retrieve all objects
 *
 * Since: 0.4.0
 */
GDataPicasaWebVisibility
gdata_picasaweb_query_get_visibility (GDataPicasaWebQuery *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_QUERY (self), 0);
	return self->priv->visibility;
}

/**
 * gdata_picasaweb_query_set_visibility:
 * @self: a #GDataPicasaWebQuery
 * @visibility: the visibility of the objects to retrieve, or <code class="literal">0</code> to retrieve all objects
 *
 * Sets the #GDataPicasaWebQuery:visibility property to @visibility.
 *
 * Since: 0.4.0
 */
void
gdata_picasaweb_query_set_visibility (GDataPicasaWebQuery *self, GDataPicasaWebVisibility visibility)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_QUERY (self));
	self->priv->visibility = visibility;
	g_object_notify (G_OBJECT (self), "visibility");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

/**
 * gdata_picasaweb_query_get_thumbnail_size:
 * @self: a #GDataPicasaWebQuery
 *
 * Gets the #GDataPicasaWebQuery:thumbnail-size property.
 *
 * Return value: a comma-separated list of thumbnail sizes to retrieve, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_picasaweb_query_get_thumbnail_size (GDataPicasaWebQuery *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_QUERY (self), NULL);
	return self->priv->thumbnail_size;
}

/**
 * gdata_picasaweb_query_set_thumbnail_size:
 * @self: a #GDataPicasaWebQuery
 * @thumbnail_size: (allow-none): a comma-separated list of thumbnail sizes to retrieve, or %NULL
 *
 * Sets the #GDataPicasaWebQuery:thumbnail-size property to @thumbnail_size.
 *
 * Set @thumbnail_size to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_picasaweb_query_set_thumbnail_size (GDataPicasaWebQuery *self, const gchar *thumbnail_size)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_QUERY (self));

	g_free (self->priv->thumbnail_size);
	self->priv->thumbnail_size = g_strdup (thumbnail_size);
	g_object_notify (G_OBJECT (self), "thumbnail-size");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

/**
 * gdata_picasaweb_query_get_image_size:
 * @self: a #GDataPicasaWebQuery
 *
 * Gets the #GDataPicasaWebQuery:image-size property.
 *
 * Return value: the currently set desired image size for retrieval, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_picasaweb_query_get_image_size (GDataPicasaWebQuery *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_QUERY (self), NULL);
	return self->priv->image_size;
}

/**
 * gdata_picasaweb_query_set_image_size:
 * @self: a #GDataPicasaWebQuery
 * @image_size: (allow-none): the desired size of the image to be retrieved, or %NULL
 *
 * Sets the #GDataPicasaWebQuery:image-size property to @image_size.
 * Valid sizes are described in the
 * <ulink type="http" url="http://code.google.com/apis/picasaweb/docs/2.0/reference.html#Parameters">online documentation</ulink>.
 *
 * Set @image_size to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_picasaweb_query_set_image_size (GDataPicasaWebQuery *self, const gchar *image_size)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_QUERY (self));

	g_free (self->priv->image_size);
	self->priv->image_size = g_strdup (image_size);
	g_object_notify (G_OBJECT (self), "image-size");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

/**
 * gdata_picasaweb_query_get_tag:
 * @self: a #GDataPicasaWebQuery
 *
 * Gets the #GDataPicasaWebQuery:tag property.
 *
 * Return value: a tag which retrieved objects must have, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_picasaweb_query_get_tag (GDataPicasaWebQuery *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_QUERY (self), NULL);
	return self->priv->tag;
}

/**
 * gdata_picasaweb_query_set_tag:
 * @self: a #GDataPicasaWebQuery
 * @tag: (allow-none): a tag which retrieved objects must have, or %NULL
 *
 * Sets the #GDataPicasaWebQuery:tag property to @tag.
 *
 * Set @tag to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_picasaweb_query_set_tag (GDataPicasaWebQuery *self, const gchar *tag)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_QUERY (self));

	g_free (self->priv->tag);
	self->priv->tag = g_strdup (tag);
	g_object_notify (G_OBJECT (self), "tag");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

/**
 * gdata_picasaweb_query_get_bounding_box:
 * @self: a #GDataPicasaWebQuery
 * @north: (out caller-allocates) (allow-none): return location for the latitude of the top of the box, or %NULL
 * @east: (out caller-allocates) (allow-none): return location for the longitude of the right of the box, or %NULL
 * @south: (out caller-allocates) (allow-none): return location for the latitude of the south of the box, or %NULL
 * @west: (out caller-allocates) (allow-none): return location for the longitude of the left of the box, or %NULL
 *
 * Gets the latitudes and longitudes of a bounding box, inside which all the results must lie.
 *
 * Since: 0.4.0
 */
void
gdata_picasaweb_query_get_bounding_box (GDataPicasaWebQuery *self, gdouble *north, gdouble *east, gdouble *south, gdouble *west)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_QUERY (self));

	if (north != NULL)
		*north = self->priv->bounding_box.north;
	if (east != NULL)
		*east = self->priv->bounding_box.east;
	if (south != NULL)
		*south = self->priv->bounding_box.south;
	if (west != NULL)
		*west = self->priv->bounding_box.west;
}

/**
 * gdata_picasaweb_query_set_bounding_box:
 * @self: a #GDataPicasaWebQuery
 * @north: latitude of the top of the box
 * @east: longitude of the right of the box
 * @south: latitude of the bottom of the box
 * @west: longitude of the left of the box
 *
 * Sets a bounding box, inside which all the returned results must lie.
 *
 * Set @north, @east, @south and @west to <code class="literal">0</code> to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_picasaweb_query_set_bounding_box (GDataPicasaWebQuery *self, gdouble north, gdouble east, gdouble south, gdouble west)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_QUERY (self));
	g_return_if_fail (north >= -90.0 && north <= 90.0);
	g_return_if_fail (south >= -90.0 && south <= 90.0);
	g_return_if_fail (east >= -180.0 && east <= 180.0);
	g_return_if_fail (west >= -180.0 && west <= 180.0);

	self->priv->bounding_box.north = north;
	self->priv->bounding_box.east = east;
	self->priv->bounding_box.south = south;
	self->priv->bounding_box.west = west;

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

/**
 * gdata_picasaweb_query_get_location:
 * @self: a #GDataPicasaWebQuery
 *
 * Gets the #GDataPicasaWebQuery:location property.
 *
 * Return value: a location which returned objects must be near, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_picasaweb_query_get_location (GDataPicasaWebQuery *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_QUERY (self), NULL);
	return self->priv->location;
}

/**
 * gdata_picasaweb_query_set_location:
 * @self: a #GDataPicasaWebQuery
 * @location: (allow-none): a location which returned objects must be near, or %NULL
 *
 * Sets the #GDataPicasaWebQuery:location property to @location.
 *
 * Set @location to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_picasaweb_query_set_location (GDataPicasaWebQuery *self, const gchar *location)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_QUERY (self));

	g_free (self->priv->location);
	self->priv->location = g_strdup (location);
	g_object_notify (G_OBJECT (self), "location");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}
