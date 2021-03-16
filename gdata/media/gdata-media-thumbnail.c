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
 * SECTION:gdata-media-thumbnail
 * @short_description: Media RSS thumbnail element
 * @stability: Stable
 * @include: gdata/media/gdata-media-thumbnail.h
 *
 * #GDataMediaThumbnail represents a "thumbnail" element from the
 * <ulink type="http" url="http://video.search.yahoo.com/mrss">Media RSS specification</ulink>.
 *
 * The class only implements parsing, not XML output, at the moment.
 */

#include <glib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-media-thumbnail.h"
#include "gdata-download-stream.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-private.h"

static void gdata_media_thumbnail_finalize (GObject *object);
static void gdata_media_thumbnail_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);
static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data,
            GError **error);

struct _GDataMediaThumbnailPrivate {
	gchar *uri;
	guint height;
	guint width;
	gint64 time;
};

enum {
	PROP_URI = 1,
	PROP_HEIGHT,
	PROP_WIDTH,
	PROP_TIME
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataMediaThumbnail, gdata_media_thumbnail, GDATA_TYPE_PARSABLE)

static void
gdata_media_thumbnail_class_init (GDataMediaThumbnailClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_media_thumbnail_get_property;
	gobject_class->finalize = gdata_media_thumbnail_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->parse_json = parse_json;
	parsable_class->element_name = "thumbnail";
	parsable_class->element_namespace = "media";

	/**
	 * GDataMediaThumbnail:uri:
	 *
	 * The URI of the thumbnail.
	 *
	 * For more information, see the <ulink type="http" url="http://video.search.yahoo.com/mrss">Media RSS specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_URI,
	                                 g_param_spec_string ("uri",
	                                                      "URI", "The URI of the thumbnail.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataMediaThumbnail:height:
	 *
	 * The height of the thumbnail, in pixels.
	 *
	 * For more information, see the <ulink type="http" url="http://video.search.yahoo.com/mrss">Media RSS specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_HEIGHT,
	                                 g_param_spec_uint ("height",
	                                                    "Height", "The height of the thumbnail, in pixels.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataMediaThumbnail:width:
	 *
	 * The width of the thumbnail, in pixels.
	 *
	 * For more information, see the <ulink type="http" url="http://video.search.yahoo.com/mrss">Media RSS specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_WIDTH,
	                                 g_param_spec_uint ("width",
	                                                    "Width", "The width of the thumbnail, in pixels.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataMediaThumbnail:time:
	 *
	 * The time offset of the thumbnail in relation to the media object, in milliseconds.
	 *
	 * For more information, see the <ulink type="http" url="http://video.search.yahoo.com/mrss">Media RSS specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_TIME,
	                                 g_param_spec_int64 ("time",
	                                                     "Time", "The time offset of the thumbnail in relation to the media object, in ms.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_media_thumbnail_init (GDataMediaThumbnail *self)
{
	self->priv = gdata_media_thumbnail_get_instance_private (self);
}

static void
gdata_media_thumbnail_finalize (GObject *object)
{
	GDataMediaThumbnailPrivate *priv = GDATA_MEDIA_THUMBNAIL (object)->priv;

	g_free (priv->uri);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_media_thumbnail_parent_class)->finalize (object);
}

static void
gdata_media_thumbnail_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataMediaThumbnailPrivate *priv = GDATA_MEDIA_THUMBNAIL (object)->priv;

	switch (property_id) {
		case PROP_URI:
			g_value_set_string (value, priv->uri);
			break;
		case PROP_HEIGHT:
			g_value_set_uint (value, priv->height);
			break;
		case PROP_WIDTH:
			g_value_set_uint (value, priv->width);
			break;
		case PROP_TIME:
			g_value_set_int64 (value, priv->time);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

/*
 * gdata_media_thumbnail_parse_time:
 * @time_string: a time string to parse
 *
 * Parses a time string in (a subset of) NTP format into a number of milliseconds since the start of a media stream.
 *
 * For more information about NTP format, see <ulink type="http" url="http://www.ietf.org/rfc/rfc2326.txt">RFC 2326 3.6 Normal Play Time</ulink>.
 *
 * To build an NTP-format string, see gdata_media_thumbnail_build_time().
 *
 * Return value: number of milliseconds since the start of a media stream
 */
static gint64
parse_time (const gchar *time_string)
{
	guint hours, minutes;
	gdouble seconds;
	gchar *end_pointer;

	g_return_val_if_fail (time_string != NULL, 0);

	hours = g_ascii_strtoull (time_string, &end_pointer, 10);
	if (end_pointer != time_string + 2)
		return -1;

	minutes = g_ascii_strtoull (time_string + 3, &end_pointer, 10);
	if (end_pointer != time_string + 5)
		return -1;

	seconds = g_ascii_strtod (time_string + 6, &end_pointer);
	if (end_pointer != time_string + strlen (time_string))
		return -1;

	return (gint64) ((seconds + minutes * 60 + hours * 3600) * 1000);
}

/*
 * gdata_media_thumbnail_build_time:
 * @_time: a number of milliseconds since the start of a media stream
 *
 * Builds an NTP-format time string describing @_time milliseconds since the start
 * of a media stream.
 *
 * Return value: an NTP-format string describing @_time; free with g_free()
 */
/*static gchar *
build_time (gint64 _time)
{
	guint hours, minutes;
	gfloat seconds;

	hours = _time % 3600000;
	_time -= hours * 3600000;

	minutes = _time % 60000;
	_time -= minutes * 60000;

	seconds = _time / 1000.0;

	return g_strdup_printf ("%02u:%02u:%02f", hours, minutes, seconds);
}*/

static gboolean
pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error)
{
	GDataMediaThumbnailPrivate *priv = GDATA_MEDIA_THUMBNAIL (parsable)->priv;
	xmlChar *uri, *width, *height, *_time;
	guint width_uint, height_uint;
	gint64 time_int64;

	/* Get the width and height */
	width = xmlGetProp (root_node, (xmlChar*) "width");
	width_uint = (width == NULL) ? 0 : g_ascii_strtoull ((gchar*) width, NULL, 10);
	xmlFree (width);

	height = xmlGetProp (root_node, (xmlChar*) "height");
	height_uint = (height == NULL) ? 0 : g_ascii_strtoull ((gchar*) height, NULL, 10);
	xmlFree (height);

	/* Get and parse the time */
	_time = xmlGetProp (root_node, (xmlChar*) "time");
	if (_time == NULL) {
		time_int64 = -1;
	} else {
		time_int64 = parse_time ((gchar*) _time);
		if (time_int64 == -1) {
			gdata_parser_error_unknown_property_value (root_node, "time", (gchar*) _time, error);
			xmlFree (_time);
			return FALSE;
		}
		xmlFree (_time);
	}

	/* Get the URI */
	uri = xmlGetProp (root_node, (xmlChar*) "url");
	if (uri == NULL || *uri == '\0') {
		xmlFree (uri);
		return gdata_parser_error_required_property_missing (root_node, "url", error);
	}

	priv->uri = (gchar*) uri;
	priv->height = height_uint;
	priv->width = width_uint;
	priv->time = time_int64;

	return TRUE;
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "media", (gchar*) "http://search.yahoo.com/mrss/");
}

/* Reference:
 * https://developers.google.com/youtube/v3/docs/videos#snippet.thumbnails */
static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data,
            GError **error)
{
	gboolean success;
	gint64 width, height;
	GDataMediaThumbnail *self = GDATA_MEDIA_THUMBNAIL (parsable);
	GDataMediaThumbnailPrivate *priv = self->priv;

	if (gdata_parser_string_from_json_member (reader, "url", P_DEFAULT,
	                                          &priv->uri, &success,
	                                          error)) {
		return success;
	} else if (gdata_parser_int_from_json_member (reader, "width",
	                                              P_DEFAULT, &width,
	                                              &success, error)) {
		priv->width = width;
		return success;
	} else if (gdata_parser_int_from_json_member (reader, "height",
	                                              P_DEFAULT, &height,
	                                              &success, error)) {
		priv->height = height;
		return success;
	} else {
		return GDATA_PARSABLE_CLASS (gdata_media_thumbnail_parent_class)->parse_json (parsable, reader, user_data, error);
	}
}

/**
 * gdata_media_thumbnail_get_uri:
 * @self: a #GDataMediaThumbnail
 *
 * Gets the #GDataMediaThumbnail:uri property.
 *
 * Return value: the thumbnail's URI
 *
 * Since: 0.4.0
 */
const gchar *
gdata_media_thumbnail_get_uri (GDataMediaThumbnail *self)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_THUMBNAIL (self), NULL);
	return self->priv->uri;
}

/**
 * gdata_media_thumbnail_get_height:
 * @self: a #GDataMediaThumbnail
 *
 * Gets the #GDataMediaThumbnail:height property.
 *
 * Return value: the thumbnail's height in pixels, or <code class="literal">0</code>
 *
 * Since: 0.4.0
 */
guint
gdata_media_thumbnail_get_height (GDataMediaThumbnail *self)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_THUMBNAIL (self), 0);
	return self->priv->height;
}

/**
 * gdata_media_thumbnail_get_width:
 * @self: a #GDataMediaThumbnail
 *
 * Gets the #GDataMediaThumbnail:width property.
 *
 * Return value: the thumbnail's width in pixels, or <code class="literal">0</code>
 *
 * Since: 0.4.0
 */
guint
gdata_media_thumbnail_get_width (GDataMediaThumbnail *self)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_THUMBNAIL (self), 0);
	return self->priv->width;
}

/**
 * gdata_media_thumbnail_get_time:
 * @self: a #GDataMediaThumbnail
 *
 * Gets the #GDataMediaThumbnail:time property.
 *
 * Return value: the thumbnail's time offset in the media, or <code class="literal">-1</code>
 *
 * Since: 0.4.0
 */
gint64
gdata_media_thumbnail_get_time (GDataMediaThumbnail *self)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_THUMBNAIL (self), -1);
	return self->priv->time;
}

/**
 * gdata_media_thumbnail_download:
 * @self: a #GDataMediaThumbnail
 * @service: the #GDataService
 * @cancellable: (allow-none): a #GCancellable for the entire download stream, or %NULL
 * @error: a #GError, or %NULL
 *
 * Downloads and returns a #GDataDownloadStream allowing the thumbnail data represented by @self to be read.
 *
 * To get the content type of the downloaded data, gdata_download_stream_get_content_type() can be called on the returned #GDataDownloadStream.
 * Calling gdata_download_stream_get_content_length() on the stream will not return a meaningful result, however, as the stream is encoded in chunks,
 * rather than by content length.
 *
 * In order to cancel the download, a #GCancellable passed in to @cancellable must be cancelled using g_cancellable_cancel(). Cancelling the individual
 * #GInputStream operations on the #GDataDownloadStream will not cancel the entire download; merely the read or close operation in question. See the
 * #GDataDownloadStream:cancellable for more details.
 *
 * Return value: (transfer full): a #GDataDownloadStream to download the thumbnail with, or %NULL; unref with g_object_unref()
 *
 * Since: 0.8.0
 */
GDataDownloadStream *
gdata_media_thumbnail_download (GDataMediaThumbnail *self, GDataService *service, GCancellable *cancellable, GError **error)
{
	const gchar *src_uri;

	g_return_val_if_fail (GDATA_IS_MEDIA_THUMBNAIL (self), NULL);
	g_return_val_if_fail (GDATA_IS_SERVICE (service), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* We keep a GError in the argument list so that we can add authentication errors, etc., in future if necessary */

	/* Get the download URI and create a stream for it */
	src_uri = gdata_media_thumbnail_get_uri (self);
	return GDATA_DOWNLOAD_STREAM (gdata_download_stream_new (service, NULL, src_uri, cancellable));
}
