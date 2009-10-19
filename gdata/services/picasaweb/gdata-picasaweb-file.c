/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Richard Schwarting 2009 <aquarichy@gmail.com>
 * Copyright (C) Philip Withnall 2009 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-picasaweb-file
 * @short_description: GData PicasaWeb file object
 * @stability: Unstable
 * @include: gdata/services/picasaweb/gdata-picasaweb-file.h
 *
 * #GDataPicasaWebFile is a subclass of #GDataEntry to represent a file in an album on Google PicasaWeb.
 *
 * For more details of Google PicasaWeb's GData API, see the
 * <ulink type="http" url="http://code.google.com/apis/picasaweb/developers_guide_protocol.html">online documentation</ulink>.
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-picasaweb-file.h"
#include "gdata-private.h"
#include "gdata-service.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "media/gdata-media-group.h"
#include "exif/gdata-exif-tags.h"
#include "georss/gdata-georss-where.h"

static void gdata_picasaweb_file_dispose (GObject *object);
static void gdata_picasaweb_file_finalize (GObject *object);
static void gdata_picasaweb_file_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_picasaweb_file_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataPicasaWebFilePrivate {
	GTimeVal edited;
	gchar *version;
	gdouble position;
	gchar *album_id;
	guint width;
	guint height;
	gsize size;
	gchar *client;
	gchar *checksum;
	GTimeVal timestamp;
	gboolean is_commenting_enabled;
	guint comment_count;
	guint rotation;
	gchar *video_status;

	/* media:group */
	GDataMediaGroup *media_group;
	/* exif:tags */
	GDataExifTags *exif_tags;
	/* georss:where */
	GDataGeoRSSWhere *georss_where;
};

enum {
	PROP_EDITED = 1,
	PROP_VERSION,
	PROP_POSITION,
	PROP_ALBUM_ID,
	PROP_WIDTH,
	PROP_HEIGHT,
	PROP_SIZE,
	PROP_CLIENT,
	PROP_CHECKSUM,
	PROP_TIMESTAMP,
	PROP_IS_COMMENTING_ENABLED,
	PROP_COMMENT_COUNT, /* TODO support comments */
	PROP_ROTATION,
	PROP_VIDEO_STATUS,
	PROP_CREDIT,
	PROP_CAPTION,
	PROP_TAGS,
	PROP_DISTANCE,
	PROP_EXPOSURE,
	PROP_FLASH,
	PROP_FOCAL_LENGTH,
	PROP_FSTOP,
	PROP_IMAGE_UNIQUE_ID,
	PROP_ISO,
	PROP_MAKE,
	PROP_MODEL,
	PROP_LATITUDE,
	PROP_LONGITUDE
};

G_DEFINE_TYPE (GDataPicasaWebFile, gdata_picasaweb_file, GDATA_TYPE_ENTRY)
#define GDATA_PICASAWEB_FILE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDATA_TYPE_PICASAWEB_FILE, GDataPicasaWebFilePrivate))

static void
gdata_picasaweb_file_class_init (GDataPicasaWebFileClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataPicasaWebFilePrivate));

	gobject_class->get_property = gdata_picasaweb_file_get_property;
	gobject_class->set_property = gdata_picasaweb_file_set_property;
	gobject_class->dispose = gdata_picasaweb_file_dispose;
	gobject_class->finalize = gdata_picasaweb_file_finalize;

	parsable_class->get_xml = get_xml;
	parsable_class->parse_xml = parse_xml;
	parsable_class->get_namespaces = get_namespaces;

	/**
	 * GDataPicasaWebFile:version:
	 *
	 * The version number of the file. Version numbers are based on modification time, so they don't increment linearly.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_version">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_VERSION,
					 g_param_spec_string ("version",
							      "Version", "The version number of the file.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:album-id:
	 *
	 * The ID for the file's album.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_albumid">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_ALBUM_ID,
					 g_param_spec_string ("album-id",
							      "Album ID", "The ID for the file's album.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:client:
	 *
	 * The name of the software which created or last modified the file.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_client">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_CLIENT,
					 g_param_spec_string ("client",
							      "Client", "The name of the software which created or last modified the file.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	/**
	 * GDataPicasaWebFile:checksum:
	 *
	 * A checksum of the file, useful for duplicate detection.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_checksum">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_CHECKSUM,
					 g_param_spec_string ("checksum",
							      "Checksum", "A checksum of the file, useful for duplicate detection.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	/**
	 * GDataPicasaWebFile:video-status:
	 *
	 * The status of the file, if it is a video.
	 *
	 * Possible values include "pending", "ready", "final", and "failed".
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_videostatus">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_VIDEO_STATUS,
					 g_param_spec_string ("video-status",
							      "Video Status", "The status of the file, if it is a video.",
							      NULL,
							      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:position:
	 *
	 * The ordinal position of the file within the album. Lower values mean the file will be closer to the start of the album.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_position">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_POSITION,
					 g_param_spec_double ("position",
							      "Position", "The ordinal position of the file within the album.",
							      0.0, G_MAXFLOAT, 0.0,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	/**
	 * GDataPicasaWebFile:width:
	 *
	 * The width of the photo or video, in pixels.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_width">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_WIDTH,
					 g_param_spec_uint ("width",
							    "Width", "The width of the photo or video, in pixels.",
							    0, G_MAXUINT, 0,
							    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
	/**
	 * GDataPicasaWebFile:height:
	 *
	 * The height of the photo or video, in pixels.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_height">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_HEIGHT,
					 g_param_spec_uint ("height",
							    "Height", "The height of the photo or video, in pixels.",
							    0, G_MAXUINT, 0,
							    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
	/**
	 * GDataPicasaWebFile:size:
	 *
	 * The size of the file, in bytes.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_size">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_SIZE,
					 g_param_spec_ulong ("size",
							     "Size", "The size of the file, in bytes.",
							     0, G_MAXULONG, 0,
							     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:edited:
	 *
	 * The time this file was last edited. If the file has not been edited yet, the content indicates the time it was created.
	 *
	 * For more information, see the <ulink type="http" url="http://www.atomenabled.org/developers/protocol/#appEdited">
	 * Atom Publishing Protocol specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_EDITED,
					 g_param_spec_boxed ("edited",
							     "Edited", "The time this file was last edited.",
							     GDATA_TYPE_G_TIME_VAL,
							     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
	/**
	 * GDataPicasaWebFile:timestamp:
	 *
	 * The time the file was purportedly taken.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_timestamp">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_TIMESTAMP,
					 g_param_spec_boxed ("timestamp",
							     "Timestamp", "The time the file was purportedly taken.",
							     GDATA_TYPE_G_TIME_VAL,
							     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	/**
	 * GDataPicasaWebFile:comment-count:
	 *
	 * The number of comments on the file.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_commentCount">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_COMMENT_COUNT,
					 g_param_spec_uint ("comment-count",
							    "Comment Count", "The number of comments on the file.",
							    0, G_MAXUINT, 0,
							    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
	/**
	 * GDataPicasaWebFile:rotation:
	 *
	 * The rotation of the photo, in degrees. This will only be non-zero for files which are pending rotation, and haven't yet been
	 * permanently modified. For files which have already been rotated, this will be %0.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_rotation">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_ROTATION,
					 g_param_spec_uint ("rotation",
							    "Rotation", "The rotation of the photo, in degrees.",
							    0, 359, 0,
							    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	/**
	 * GDataPicasaWebFile:is-commenting-enabled:
	 *
	 * Whether commenting is enabled for this file.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_IS_COMMENTING_ENABLED,
					 g_param_spec_boolean ("is-commenting-enabled",
							       "Commenting enabled?", "Indicates whether comments are enabled.",
							       TRUE,
							       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:credit:
	 *
	 * The nickname of the user credited with this file.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#media_credit">Media RSS
	 * specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_CREDIT,
					 g_param_spec_string ("credit",
							      "Credit", "The nickname of the user credited with this file.",
							      NULL,
							      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:caption:
	 *
	 * The file's descriptive caption.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_CAPTION,
					 g_param_spec_string ("caption",
							      "Caption", "The file's descriptive caption.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:tags:
	 *
	 * A comma-separated list of tags associated with the file.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#media_keywords">
	 * Media RSS specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_TAGS,
					 g_param_spec_string ("tags",
							      "Tags", "A comma-separated list of tags associated with the file.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:distance:
	 *
	 * The distance to the subject reported in the image's EXIF.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#exif_reference">
	 * EXIF element reference</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_DISTANCE,
					 g_param_spec_double ("distance",
							      "Distance", "The distance to the subject.",
							      -1.0, G_MAXDOUBLE, -1.0,
							      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:exposure:
	 *
	 * The exposure time.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#exif_reference">
	 * EXIF element reference</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_EXPOSURE,
					 g_param_spec_double ("exposure",
							      "Exposure", "The exposure time.",
							      0.0, G_MAXDOUBLE, 0.0,
							      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:flash:
	 *
	 * Indicates whether the flash was used.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#exif_reference">
	 * EXIF element reference</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_FLASH,
					 g_param_spec_boolean ("flash",
							       "Flash", "Indicates whether the flash was used.",
							       FALSE,
							       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:focal-length:
	 *
	 * The focal length for the shot.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#exif_reference">
	 * EXIF element reference</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_FOCAL_LENGTH,
					 g_param_spec_double ("focal-length",
							      "Focal Length", "The focal length used in the shot.",
							      -1.0, G_MAXDOUBLE, -1.0,
							      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:fstop:
	 *
	 * The F-stop value.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#exif_reference">
	 * EXIF element reference</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_FSTOP,
					 g_param_spec_double ("fstop",
							      "F-stop", "The F-stop used.",
							      0.0, G_MAXDOUBLE, 0.0,
							      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:image-unique-id:
	 *
	 * An unique ID for the image found in the EXIF.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#exif_reference">
	 * EXIF element reference</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_IMAGE_UNIQUE_ID,
					 g_param_spec_string ("image-unique-id",
							      "Image Unique ID", "An unique ID for the image.",
							      NULL,
							      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:iso:
	 *
	 * The ISO speed.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#exif_reference">
	 * EXIF element reference</ulink> and ISO 5800:1987.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_ISO,
					 g_param_spec_long ("iso",
							    "ISO", "The ISO speed.",
							    -1, G_MAXLONG, -1,
							    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:make:
	 *
	 * The name of the manufacturer of the camera.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#exif_reference">
	 * EXIF element reference</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_MAKE,
					 g_param_spec_string ("make",
							      "Make", "The name of the manufacturer.",
							      NULL,
							      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:model:
	 *
	 * The model of the camera.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#exif_reference">
	 * EXIF element reference</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_MODEL,
					 g_param_spec_string ("model",
							      "Model", "The model of the camera.",
							      NULL,
							      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:latitude:
	 *
	 * The location as a latitude coordinate associated with this file. Valid latitudes range from %-90.0 to %90.0 inclusive.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/docs/2.0/reference.html#georss_where">
	 * GeoRSS specification</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_LATITUDE,
					 g_param_spec_double ("latitude",
							      "Latitude", "The location as a latitude coordinate associated with this file.",
							      -90.0, 90.0, 0.0,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:longitude:
	 *
	 * The location as a longitude coordinate associated with this file. Valid longitudes range from %-180.0 to %180.0 inclusive.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/docs/2.0/reference.html#georss_where">
	 * GeoRSS specification</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_LONGITUDE,
					 g_param_spec_double ("longitude",
							      "Longitude", "The location as a longitude coordinate associated with this file.",
							      -180.0, 180.0, 0.0,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
notify_title_cb (GDataPicasaWebFile *self, GParamSpec *pspec, gpointer user_data)
{
	/* Keep the atom:title and media:group/media:title in sync */
	if (self->priv->media_group != NULL)
		gdata_media_group_set_title (self->priv->media_group, gdata_entry_get_title (GDATA_ENTRY (self)));
}

static void
notify_summary_cb (GDataPicasaWebFile *self, GParamSpec *pspec, gpointer user_data)
{
	/* Keep the atom:summary and media:group/media:description in sync */
	if (self->priv->media_group != NULL)
		gdata_media_group_set_description (self->priv->media_group, gdata_entry_get_summary (GDATA_ENTRY (self)));
}

static void
gdata_picasaweb_file_init (GDataPicasaWebFile *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_PICASAWEB_FILE, GDataPicasaWebFilePrivate);
	self->priv->media_group = g_object_new (GDATA_TYPE_MEDIA_GROUP, NULL);
	self->priv->exif_tags = g_object_new (GDATA_TYPE_EXIF_TAGS, NULL);
	self->priv->georss_where = g_object_new (GDATA_TYPE_GEORSS_WHERE, NULL);
	self->priv->is_commenting_enabled = TRUE;

	/* We need to keep atom:title (the canonical title for the file) in sync with media:group/media:title */
	g_signal_connect (self, "notify::title", G_CALLBACK (notify_title_cb), NULL);
	/* atom:summary (the canonical summary/caption for the file) in sync with media:group/media:description */
	g_signal_connect (self, "notify::summary", G_CALLBACK (notify_summary_cb), NULL);
}

static void
gdata_picasaweb_file_dispose (GObject *object)
{
	GDataPicasaWebFilePrivate *priv = GDATA_PICASAWEB_FILE_GET_PRIVATE (object);

	if (priv->media_group != NULL)
		g_object_unref (priv->media_group);
	priv->media_group = NULL;

	if (priv->exif_tags != NULL)
		g_object_unref (priv->exif_tags);
	priv->exif_tags = NULL;

	if (priv->georss_where != NULL)
		g_object_unref (priv->georss_where);
	priv->georss_where = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_picasaweb_file_parent_class)->dispose (object);
}

static void
gdata_picasaweb_file_finalize (GObject *object)
{
	GDataPicasaWebFilePrivate *priv = GDATA_PICASAWEB_FILE_GET_PRIVATE (object);

	g_free (priv->version);
	g_free (priv->album_id);
	g_free (priv->client);
	g_free (priv->checksum);
	xmlFree (priv->video_status);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_picasaweb_file_parent_class)->finalize (object);
}

static void
gdata_picasaweb_file_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataPicasaWebFilePrivate *priv = GDATA_PICASAWEB_FILE_GET_PRIVATE (object);

	switch (property_id) {
		case PROP_EDITED:
			g_value_set_boxed (value, &(priv->edited));
			break;
		case PROP_VERSION:
			g_value_set_string (value, priv->version);
			break;
		case PROP_POSITION:
			g_value_set_double (value, priv->position);
			break;
		case PROP_ALBUM_ID:
			g_value_set_string (value, priv->album_id);
			break;
		case PROP_WIDTH:
			g_value_set_uint (value, priv->width);
			break;
		case PROP_HEIGHT:
			g_value_set_uint (value, priv->height);
			break;
		case PROP_SIZE:
			g_value_set_ulong (value, priv->size);
			break;
		case PROP_CLIENT:
			g_value_set_string (value, priv->client);
			break;
		case PROP_CHECKSUM:
			g_value_set_string (value, priv->checksum);
			break;
		case PROP_TIMESTAMP:
			g_value_set_boxed (value, &(priv->timestamp));
			break;
		case PROP_IS_COMMENTING_ENABLED:
			g_value_set_boolean (value, priv->is_commenting_enabled);
			break;
		case PROP_COMMENT_COUNT:
			g_value_set_uint (value, priv->comment_count);
			break;
		case PROP_ROTATION:
			g_value_set_uint (value, priv->rotation);
			break;
		case PROP_VIDEO_STATUS:
			g_value_set_string (value, priv->video_status);
			break;
		case PROP_CREDIT: {
			GDataMediaCredit *credit = gdata_media_group_get_credit (priv->media_group);
			g_value_set_string (value, gdata_media_credit_get_credit (credit));
			break; }
		case PROP_CAPTION:
			g_value_set_string (value, gdata_entry_get_summary (GDATA_ENTRY (object)));
			break;
		case PROP_TAGS:
			g_value_set_string (value, gdata_media_group_get_keywords (priv->media_group));
			break;
		case PROP_DISTANCE:
			g_value_set_double (value, gdata_exif_tags_get_distance (priv->exif_tags));
			break;
		case PROP_EXPOSURE:
			g_value_set_double (value, gdata_exif_tags_get_exposure (priv->exif_tags));
			break;
		case PROP_FLASH:
			g_value_set_boolean (value, gdata_exif_tags_get_flash (priv->exif_tags));
			break;
		case PROP_FOCAL_LENGTH:
			g_value_set_double (value, gdata_exif_tags_get_focal_length (priv->exif_tags));
			break;
		case PROP_FSTOP:
			g_value_set_double (value, gdata_exif_tags_get_fstop (priv->exif_tags));
			break;
		case PROP_IMAGE_UNIQUE_ID:
			g_value_set_string (value, gdata_exif_tags_get_image_unique_id (priv->exif_tags));
			break;
		case PROP_ISO:
			g_value_set_long (value, gdata_exif_tags_get_iso (priv->exif_tags));
			break;
		case PROP_MAKE:
			g_value_set_string (value, gdata_exif_tags_get_make (priv->exif_tags));
			break;
		case PROP_MODEL:
			g_value_set_string (value, gdata_exif_tags_get_model (priv->exif_tags));
			break;
		case PROP_LATITUDE:
			g_value_set_double (value, gdata_georss_where_get_latitude (priv->georss_where));
			break;
		case PROP_LONGITUDE:
			g_value_set_double (value, gdata_georss_where_get_longitude (priv->georss_where));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_picasaweb_file_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataPicasaWebFile *self = GDATA_PICASAWEB_FILE (object);

	switch (property_id) {
		case PROP_VERSION:
			/* Construct only */
			g_free (self->priv->version);
			self->priv->version = g_value_dup_string (value);
			break;
		case PROP_POSITION:
			gdata_picasaweb_file_set_position (self, g_value_get_double (value));
			break;
		case PROP_ALBUM_ID:
			/* TODO: do we allow this to change albums? I think that's how pictures are moved. */
			gdata_picasaweb_file_set_album_id (self, g_value_get_string (value));
			break;
		case PROP_CLIENT:
			gdata_picasaweb_file_set_client (self, g_value_get_string (value));
			break;
		case PROP_CHECKSUM:
			gdata_picasaweb_file_set_checksum (self, g_value_get_string (value));
			break;
		case PROP_TIMESTAMP:
			gdata_picasaweb_file_set_timestamp (self, g_value_get_boxed (value));
			break;
		case PROP_IS_COMMENTING_ENABLED: /* TODO I don't think we can change this on a per file basis */
			gdata_picasaweb_file_set_is_commenting_enabled (self, g_value_get_boolean (value));
			break;
		case PROP_ROTATION:
			gdata_picasaweb_file_set_rotation (self, g_value_get_uint (value));
			break;
		case PROP_CAPTION:
			gdata_picasaweb_file_set_caption (self, g_value_get_string (value));
			break;
		case PROP_TAGS:
			gdata_picasaweb_file_set_tags (self, g_value_get_string (value));
			break;
		case PROP_LATITUDE:
			gdata_picasaweb_file_set_coordinates (self, g_value_get_double (value),
							      gdata_georss_where_get_longitude (self->priv->georss_where));
			break;
		case PROP_LONGITUDE:
			gdata_picasaweb_file_set_coordinates (self, gdata_georss_where_get_latitude (self->priv->georss_where),
							      g_value_get_double (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	GDataPicasaWebFile *self = GDATA_PICASAWEB_FILE (parsable);

	if (xmlStrcmp (node->name, (xmlChar*) "group") == 0) {
		/* media:group */
		GDataMediaGroup *group = GDATA_MEDIA_GROUP (_gdata_parsable_new_from_xml_node (GDATA_TYPE_MEDIA_GROUP, doc, node, NULL, error));
		if (group == NULL)
			return FALSE;

		if (self->priv->media_group != NULL)
			/* We should really error here, but we can't, as priv->media_group has to be pre-populated
			 * in order for things like gdata_picasaweb_file_set_description() to work. */
			g_object_unref (self->priv->media_group);

		self->priv->media_group = group;
	} else if (xmlStrcmp (node->name, (xmlChar*) "where") == 0) {
		/* georss:where */
		GDataGeoRSSWhere *where = GDATA_GEORSS_WHERE (_gdata_parsable_new_from_xml_node (GDATA_TYPE_GEORSS_WHERE, doc, node, NULL, error));
		if (where == NULL)
			return FALSE;

		if (self->priv->georss_where != NULL)
			g_object_unref (self->priv->georss_where);

		self->priv->georss_where = where;
	} else if (xmlStrcmp (node->name, (xmlChar*) "tags") == 0) {
		/* exif:tags */
		GDataExifTags *tags = GDATA_EXIF_TAGS (_gdata_parsable_new_from_xml_node (GDATA_TYPE_EXIF_TAGS, doc, node, NULL, error));
		if (tags == NULL)
			return FALSE;

		if (self->priv->exif_tags != NULL)
			g_object_unref (self->priv->exif_tags);

		self->priv->exif_tags = tags;
	} else if (xmlStrcmp (node->name, (xmlChar*) "edited") == 0) {
		/* app:edited */
		xmlChar *edited = xmlNodeListGetString (doc, node->children, TRUE);
		if (g_time_val_from_iso8601 ((gchar*) edited, &(self->priv->edited)) == FALSE) {
			/* Error */
			gdata_parser_error_not_iso8601_format (node, (gchar*) edited, error);
			xmlFree (edited);
			return FALSE;
		}
		xmlFree (edited);
	} else if (xmlStrcmp (node->name, (xmlChar*) "imageVersion") == 0) {
		/* gphoto:imageVersion */
		xmlChar *version = xmlNodeListGetString (doc, node->children, TRUE);
		g_free (self->priv->version);
		self->priv->version = g_strdup ((gchar*) version);
		xmlFree (version);
	} else if (xmlStrcmp (node->name, (xmlChar*) "position") == 0) {
		/* gphoto:position */
		xmlChar *position_str = xmlNodeListGetString (doc, node->children, TRUE);
		gdata_picasaweb_file_set_position (self, g_ascii_strtod ((gchar*) position_str, NULL));
		xmlFree (position_str);
	} else if (xmlStrcmp (node->name, (xmlChar*) "albumid") == 0) {
		/* gphoto:album_id */
		xmlChar *album_id = xmlNodeListGetString (doc, node->children, TRUE);
		gdata_picasaweb_file_set_album_id (self, (gchar*) album_id);
		xmlFree (album_id);
	} else if (xmlStrcmp (node->name, (xmlChar*) "width") == 0) {
		/* gphoto:width */
		xmlChar *width = xmlNodeListGetString (doc, node->children, TRUE);
		self->priv->width = strtoul ((gchar*) width, NULL, 10);
		xmlFree (width);
	} else if (xmlStrcmp (node->name, (xmlChar*) "height") == 0) {
		/* gphoto:height */
		xmlChar *height = xmlNodeListGetString (doc, node->children, TRUE);
		self->priv->height = strtoul ((gchar*) height, NULL, 10);
		xmlFree (height);
	} else if (xmlStrcmp (node->name, (xmlChar*) "size") == 0) {
		/* gphoto:size */
		xmlChar *size = xmlNodeListGetString (doc, node->children, TRUE);
		self->priv->size = strtoul ((gchar*) size, NULL, 10);
		xmlFree (size);
	} else if (xmlStrcmp (node->name, (xmlChar*) "client") == 0) {
		/* gphoto:client */
		xmlChar *client = xmlNodeListGetString (doc, node->children, TRUE);
		gdata_picasaweb_file_set_client (self, (gchar*) client);
		xmlFree (client);
	} else if (xmlStrcmp (node->name, (xmlChar*) "checksum") == 0) {
		/* gphoto:checksum */
		xmlChar *checksum = xmlNodeListGetString (doc, node->children, TRUE);
		gdata_picasaweb_file_set_checksum (self, (gchar*) checksum);
		xmlFree (checksum);
	} else if (xmlStrcmp (node->name, (xmlChar*) "timestamp") == 0) {
		/* gphoto:timestamp */
		xmlChar *timestamp_str;
		guint64 milliseconds;
		GTimeVal timestamp;

		timestamp_str = xmlNodeListGetString (doc, node->children, TRUE);
		milliseconds = g_ascii_strtoull ((gchar*) timestamp_str, NULL, 10);
		xmlFree (timestamp_str);

		timestamp.tv_sec = (glong) (milliseconds / 1000);
		timestamp.tv_usec = (glong) ((milliseconds % 1000) * 1000);

		gdata_picasaweb_file_set_timestamp (self, &timestamp);
	} else if (xmlStrcmp (node->name, (xmlChar*) "commentingEnabled") == 0) {
		/* gphoto:commentingEnabled */
		xmlChar *is_commenting_enabled = xmlNodeListGetString (doc, node->children, TRUE);
		if (is_commenting_enabled == NULL)
			return gdata_parser_error_required_content_missing (node, error);
		self->priv->is_commenting_enabled = (xmlStrcmp (is_commenting_enabled, (xmlChar*) "true") == 0 ? TRUE : FALSE);
		xmlFree (is_commenting_enabled);
	} else if (xmlStrcmp (node->name, (xmlChar*) "commentCount") == 0) {
		/* gphoto:commentCount */
		xmlChar *comment_count = xmlNodeListGetString (doc, node->children, TRUE);
		self->priv->comment_count = strtoul ((gchar*) comment_count, NULL, 10);
		xmlFree (comment_count);
	} else if (xmlStrcmp (node->name, (xmlChar*) "videostatus") == 0) {
		/* gphoto:videostatus */
		xmlChar *video_status = xmlNodeListGetString (doc, node->children, TRUE);
		if (self->priv->video_status != NULL) {
			xmlFree (video_status);
			return gdata_parser_error_duplicate_element (node, error);
		}
		self->priv->video_status = (gchar*) video_status;
	} else if (xmlStrcmp (node->name, (xmlChar*) "rotation") == 0) {
		/* gphoto:rotation */
		xmlChar *rotation = xmlNodeListGetString (doc, node->children, TRUE);
		gdata_picasaweb_file_set_rotation (self, strtoul ((gchar*) rotation, NULL, 10));
		xmlFree (rotation);
	} else if (GDATA_PARSABLE_CLASS (gdata_picasaweb_file_parent_class)->parse_xml (parsable, doc, node, user_data, error) == FALSE) {
		/* Error! */
		return FALSE;
	}

	return TRUE;
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataPicasaWebFilePrivate *priv = GDATA_PICASAWEB_FILE (parsable)->priv;
	gchar ascii_double_str[G_ASCII_DTOSTR_BUF_SIZE];

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_picasaweb_file_parent_class)->get_xml (parsable, xml_string);

	/* Add all the PicasaWeb-specific XML */
	if (priv->version != NULL)
		g_string_append_printf (xml_string, "<gphoto:version>%s</gphoto:version>", priv->version);

	g_string_append_printf (xml_string, "<gphoto:position>%s</gphoto:position>",
				g_ascii_dtostr (ascii_double_str, sizeof (ascii_double_str), priv->position));

	if (priv->album_id != NULL)
		g_string_append_printf (xml_string, "<gphoto:albumid>%s</gphoto:albumid>", priv->album_id);

	if (priv->client != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gphoto:client>", priv->client, "</gphoto:client>");

	if (priv->checksum != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gphoto:checksum>", priv->checksum, "</gphoto:checksum>");

	if (priv->timestamp.tv_sec != 0 || priv->timestamp.tv_usec != 0) {
		/* timestamp is in milliseconds */
		g_string_append_printf (xml_string, "<gphoto:timestamp>%" G_GUINT64_FORMAT "</gphoto:timestamp>",
					((guint64) priv->timestamp.tv_sec) * 1000 + priv->timestamp.tv_usec / 1000);
	}

	if (priv->is_commenting_enabled == TRUE)
		g_string_append (xml_string, "<gphoto:commentingEnabled>true</gphoto:commentingEnabled>");
	else
		g_string_append (xml_string, "<gphoto:commentingEnabled>false</gphoto:commentingEnabled>");

	if (priv->rotation > 0)
		g_string_append_printf (xml_string, "<gphoto:rotation>%u</gphoto:rotation>", priv->rotation);

	/* media:group */
	_gdata_parsable_get_xml (GDATA_PARSABLE (priv->media_group), xml_string, FALSE);

	/* georss:where */
	if (priv->georss_where != NULL && gdata_georss_where_get_latitude (priv->georss_where) != G_MAXDOUBLE &&
	    gdata_georss_where_get_longitude (priv->georss_where) != G_MAXDOUBLE) {
		_gdata_parsable_get_xml (GDATA_PARSABLE (priv->georss_where), xml_string, FALSE);
	}

	/* TODO:
	 * - Finish supporting all tags
	 * - Check all tags here are valid for insertions and updates
	 * - Check things are escaped (or not) as appropriate
	 */
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	GDataPicasaWebFilePrivate *priv = GDATA_PICASAWEB_FILE (parsable)->priv;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_picasaweb_file_parent_class)->get_namespaces (parsable, namespaces);

	g_hash_table_insert (namespaces, (gchar*) "gphoto", (gchar*) "http://schemas.google.com/photos/2007");
	g_hash_table_insert (namespaces, (gchar*) "app", (gchar*) "http://www.w3.org/2007/app");

	/* Add the media:group namespaces */
	GDATA_PARSABLE_GET_CLASS (priv->media_group)->get_namespaces (GDATA_PARSABLE (priv->media_group), namespaces);
	/* Add the exif:tags namespaces */
	GDATA_PARSABLE_GET_CLASS (priv->exif_tags)->get_namespaces (GDATA_PARSABLE (priv->exif_tags), namespaces);
	/* Add the georss:where namespaces */
	GDATA_PARSABLE_GET_CLASS (priv->georss_where)->get_namespaces (GDATA_PARSABLE (priv->georss_where), namespaces);
}

/**
 * gdata_picasaweb_file_new:
 * @id: the file's ID, or %NULL
 *
 * Creates a new #GDataPicasaWebFile with the given ID and default properties.
 *
 * Return value: a new #GDataPicasaWebFile; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataPicasaWebFile *
gdata_picasaweb_file_new (const gchar *id)
{
	return g_object_new (GDATA_TYPE_PICASAWEB_FILE, "id", id, NULL);
}

/**
 * gdata_picasaweb_file_get_edited:
 * @self: a #GDataPicasaWebFile
 * @edited: a #GTimeVal
 *
 * Gets the #GDataPicasaWebFile:edited property and puts it in @edited. If the property is unset,
 * both fields in the #GTimeVal will be set to %0.
 *
 * Since: 0.4.0
 **/
void
gdata_picasaweb_file_get_edited (GDataPicasaWebFile *self, GTimeVal *edited)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_FILE (self));
	g_return_if_fail (edited != NULL);
	*edited = self->priv->edited;
}

/**
 * gdata_picasaweb_file_get_version:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:version property.
 *
 * Return value: the file's version number, or %NULL
 *
 * Since: 0.4.0
 **/
const gchar *
gdata_picasaweb_file_get_version (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return self->priv->version;
}

/**
 * gdata_picasaweb_file_get_position:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:position property.
 *
 * Return value: the file's ordinal position in the album
 *
 * Since: 0.4.0
 **/
gdouble
gdata_picasaweb_file_get_position (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), 0.0);
	return self->priv->position;
}

/**
 * gdata_picasaweb_file_set_position:
 * @self: a #GDataPicasaWebFile
 * @position: the file's new position in the album
 *
 * Sets the #GDataPicasaWebFile:position property.
 *
 * Since: 0.4.0
 **/
void
gdata_picasaweb_file_set_position (GDataPicasaWebFile *self, gdouble position)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_FILE (self));
	self->priv->position = position;
	g_object_notify (G_OBJECT (self), "position");
}

/**
 * gdata_picasaweb_file_get_album_id:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:album-id property.
 *
 * Return value: the ID of the album containing the #GDataPicasaWebFile
 *
 * Since: 0.4.0
 **/
const gchar *
gdata_picasaweb_file_get_album_id (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return self->priv->album_id;
}

/**
 * gdata_picasaweb_file_set_album_id:
 * @self: a #GDataPicasaWebFile
 * @album_id: the ID of the new album for this file
 *
 * Sets the #GDataPicasaWebFile:album-id property, effectively moving the file to the album.
 *
 * Since: 0.4.0
 **/
void
gdata_picasaweb_file_set_album_id (GDataPicasaWebFile *self, const gchar *album_id)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_FILE (self));
	g_return_if_fail (album_id != NULL && *album_id != '\0');

	g_free (self->priv->album_id);
	self->priv->album_id = g_strdup (album_id);
	g_object_notify (G_OBJECT (self), "album-id");
}

/**
 * gdata_picasaweb_file_get_width:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:width property.
 *
 * Return value: the width of the image or video, in pixels
 *
 * Since: 0.4.0
 **/
guint
gdata_picasaweb_file_get_width (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), 0);
	return self->priv->width;
}

/**
 * gdata_picasaweb_file_get_height:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:height property.
 *
 * Return value: the height of the image or video, in pixels
 *
 * Since: 0.4.0
 **/
guint
gdata_picasaweb_file_get_height (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), 0);
	return self->priv->height;
}

/**
 * gdata_picasaweb_file_get_size:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:size property.
 *
 * Return value: the size of the file, in bytes
 *
 * Since: 0.4.0
 **/
gsize
gdata_picasaweb_file_get_size (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), 0);
	return self->priv->size;
}

/**
 * gdata_picasaweb_file_get_client:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:client property.
 *
 * Return value: the name of the software which created the photo, or %NULL
 *
 * Since: 0.4.0
 **/
const gchar *
gdata_picasaweb_file_get_client (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return self->priv->client;
}

/**
 * gdata_picasaweb_file_set_client:
 * @self: a #GDataPicasaWebFile
 * @client: the name of the software which created or modified the photo, or %NULL
 *
 * Sets the #GDataPicasaWebFile:client property to @client.
 *
 * Set @client to %NULL to unset the property.
 *
 * Since: 0.4.0
 **/
void
gdata_picasaweb_file_set_client (GDataPicasaWebFile *self, const gchar *client)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_FILE (self));

	g_free (self->priv->client);
	self->priv->client = g_strdup (client);
	g_object_notify (G_OBJECT (self), "client");
}

/**
 * gdata_picasaweb_file_get_checksum:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:checksum property.
 *
 * Return value: the checksum assigned to this file, or %NULL
 *
 * Since: 0.4.0
 **/
const gchar *
gdata_picasaweb_file_get_checksum (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return self->priv->checksum;
}

/**
 * gdata_picasaweb_file_set_checksum:
 * @self: a #GDataPicasaWebFile
 * @checksum: the new checksum for this file, or %NULL
 *
 * Sets the #GDataPicasaWebFile:checksum property to @checksum.
 *
 * Set @checksum to %NULL to unset the property.
 *
 * Since: 0.4.0
 **/
void
gdata_picasaweb_file_set_checksum (GDataPicasaWebFile *self, const gchar *checksum)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_FILE (self));

	g_free (self->priv->checksum);
	self->priv->checksum = g_strdup (checksum);
	g_object_notify (G_OBJECT (self), "checksum");
}

/**
 * gdata_picasaweb_file_get_timestamp:
 * @self: a #GDataPicasaWebFile
 * @timestamp: a #GTimeVal
 *
 * Gets the #GDataPicasaWebFile:timestamp property and puts it in @timestamp. If the property is unset,
 * both fields in the #GTimeVal will be set to %0.
 *
 * Since: 0.4.0
 **/
void
gdata_picasaweb_file_get_timestamp (GDataPicasaWebFile *self, GTimeVal *timestamp)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_FILE (self));
	g_return_if_fail (timestamp != NULL);
	*timestamp = self->priv->timestamp;
}

/**
 * gdata_picasaweb_file_set_timestamp:
 * @self: a #GDataPicasaWebFile
 * @timestamp: a #GTimeVal, or %NULL
 *
 * Sets the #GDataPicasaWebFile:timestamp property from values supplied by @timestamp. If @timestamp is %NULL,
 * the property will be unset.
 *
 * Since: 0.4.0
 **/
void
gdata_picasaweb_file_set_timestamp (GDataPicasaWebFile *self, GTimeVal *timestamp)
{
	/* RHSTODO: I think the timestamp value is just being
	   over-ridden by the file's actual EXIF time value; unless
	   we're setting this incorrectly here or in get_xml(); test that */
	/* RHSTODO: improve testing of setters in tests/picasa.c */

	g_return_if_fail (GDATA_IS_PICASAWEB_FILE (self));
	if (timestamp == NULL)
		self->priv->timestamp.tv_sec = self->priv->timestamp.tv_usec = 0;
	else
		self->priv->timestamp = *timestamp;
	g_object_notify (G_OBJECT (self), "timestamp");
}

/**
 * gdata_picasaweb_file_is_commenting_enabled:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:is-commenting-enabled property.
 *
 * Return value: %TRUE if commenting is enabled, %FALSE otherwise
 *
 * Since: 0.4.0
 **/
gboolean
gdata_picasaweb_file_is_commenting_enabled (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), FALSE);
	return self->priv->is_commenting_enabled;
}

/**
 * gdata_picasaweb_file_set_is_commenting_enabled:
 * @self: a #GDataPicasaWebFile
 * @is_commenting_enabled: %TRUE if commenting should be enabled for the file, %FALSE otherwise
 *
 * Sets the #GDataPicasaWebFile:is-commenting-enabled property to @is_commenting_enabled.
 *
 * Since: 0.4.0
 **/
void
gdata_picasaweb_file_set_is_commenting_enabled (GDataPicasaWebFile *self, gboolean is_commenting_enabled)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_FILE (self));
	self->priv->is_commenting_enabled = is_commenting_enabled;
	g_object_notify (G_OBJECT (self), "is-commenting-enabled");
}

/**
 * gdata_picasaweb_file_get_comment_count:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:comment-count property.
 *
 * Return value: the number of comments on the file
 *
 * Since: 0.4.0
 **/
guint
gdata_picasaweb_file_get_comment_count (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), 0);
	return self->priv->comment_count;
}

/**
 * gdata_picasaweb_file_get_rotation:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:rotation property.
 *
 * Return value: the image's rotation, in degrees
 *
 * Since: 0.4.0
 **/
guint
gdata_picasaweb_file_get_rotation (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), 0);
	return self->priv->rotation;
}

/**
 * gdata_picasaweb_file_set_rotation:
 * @self: a #GDataPicasaWebFile
 * @rotation: the new rotation for the image, in degrees
 *
 * Sets the #GDataPicasaWebFile:rotation property to @rotation.
 *
 * The rotation is absolute, rather than cumulative, through successive calls to gdata_picasaweb_file_set_rotation(),
 * so calling it with 90° then 20° will result in a final rotation of 20°.
 *
 * Since: 0.4.0
 **/
void
gdata_picasaweb_file_set_rotation (GDataPicasaWebFile *self, guint rotation)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_FILE (self));
	self->priv->rotation = rotation % 360;
	g_object_notify (G_OBJECT (self), "rotation");
}


/**
 * gdata_picasaweb_file_get_video_status:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:video-status property.
 *
 * Return value: the status of this video ("pending", "ready", "final" or "failed"), or %NULL
 *
 * Since: 0.4.0
 **/
const gchar *
gdata_picasaweb_file_get_video_status (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return self->priv->video_status;
}

/**
 * gdata_picasaweb_file_get_tags:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:tags property.
 *
 * Return value: a comma-separated list of tags associated with the file, or %NULL
 *
 * Since: 0.4.0
 **/
const gchar *
gdata_picasaweb_file_get_tags (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return gdata_media_group_get_keywords (self->priv->media_group);
}

/**
 * gdata_picasaweb_file_set_tags:
 * @self: a #GDataPicasaWebFile
 * @tags: a new comma-separated list of tags, or %NULL
 *
 * Sets the #GDataPicasaWebFile:tags property to @tags.
 *
 * Set @tags to %NULL to unset the property.
 *
 * Since: 0.4.0
 **/
void
gdata_picasaweb_file_set_tags (GDataPicasaWebFile *self, const gchar *tags)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_FILE (self));

	gdata_media_group_set_keywords (self->priv->media_group, tags);
	g_object_notify (G_OBJECT (self), "tags");
}

/**
 * gdata_picasaweb_file_get_credit:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:credit property.
 *
 * Return value: the nickname of the user credited with this file
 *
 * Since: 0.4.0
 **/
const gchar *
gdata_picasaweb_file_get_credit (GDataPicasaWebFile *self)
{
	GDataMediaCredit *credit;

	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);

	credit = gdata_media_group_get_credit (self->priv->media_group);
	return (credit == NULL) ? NULL : gdata_media_credit_get_credit (credit);
}

/**
 * gdata_picasaweb_file_get_caption:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:caption property.
 *
 * Return value: the file's descriptive caption, or %NULL
 *
 * Since: 0.4.0
 **/
const gchar *
gdata_picasaweb_file_get_caption (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return gdata_entry_get_summary (GDATA_ENTRY (self));
}

/**
 * gdata_picasaweb_file_set_caption:
 * @self: a #GDataPicasaWebFile
 * @caption: the file's new caption, or %NULL
 *
 * Sets the #GDataPicasaWebFile:caption property to @caption.
 *
 * Set @caption to %NULL to unset the file's caption.
 *
 * Since: 0.4.0
 **/
void
gdata_picasaweb_file_set_caption (GDataPicasaWebFile *self, const gchar *caption)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_FILE (self));

	gdata_entry_set_summary (GDATA_ENTRY (self), caption);
	gdata_media_group_set_description (self->priv->media_group, caption);
	g_object_notify (G_OBJECT (self), "caption");
}

/**
 * gdata_picasaweb_file_get_contents:
 * @self: a #GDataPicasaWebFile
 *
 * Returns a list of media content, e.g. the actual photo or video.
 *
 * Return value: a #GList of #GDataMediaContent items
 *
 * Since: 0.4.0
 **/
GList *
gdata_picasaweb_file_get_contents (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return gdata_media_group_get_contents (self->priv->media_group);
}

/**
 * gdata_picasaweb_file_get_thumbnails:
 * @self: a #GDataPicasaWebFile
 *
 * Returns a list of thumbnails, often at different sizes, for this file.
 *
 * Return value: a #GList of #GDataMediaThumbnail<!-- -->s, or %NULL
 *
 * Since: 0.4.0
 **/
GList *
gdata_picasaweb_file_get_thumbnails (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return gdata_media_group_get_thumbnails (self->priv->media_group);
}

/**
 * gdata_picasaweb_file_get_distance:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:distance property.
 *
 * Return value: the distance recorded in the photo's EXIF, or %-1 if unknown
 *
 * Since: 0.5.0
 **/
gdouble
gdata_picasaweb_file_get_distance (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), -1);
	return gdata_exif_tags_get_distance (self->priv->exif_tags);
}

/**
 * gdata_picasaweb_file_get_exposure:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:exposure property.
 *
 * Return value: the exposure value, or %0 if unknown
 *
 * Since: 0.5.0
 **/
gdouble
gdata_picasaweb_file_get_exposure (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), 0);
	return gdata_exif_tags_get_exposure (self->priv->exif_tags);
}

/**
 * gdata_picasaweb_file_get_flash:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:flash property.
 *
 * Return value: %TRUE if flash was used, %FALSE otherwise
 *
 * Since: 0.5.0
 **/
gboolean
gdata_picasaweb_file_get_flash (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), FALSE);
	return gdata_exif_tags_get_flash (self->priv->exif_tags);
}

/**
 * gdata_picasaweb_file_get_focal_length:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:focal-length property.
 *
 * Return value: the focal-length value, or %-1 if unknown
 *
 * Since: 0.5.0
 **/
gdouble
gdata_picasaweb_file_get_focal_length (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), -1);
	return gdata_exif_tags_get_focal_length (self->priv->exif_tags);
}

/**
 * gdata_picasaweb_file_get_fstop:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:fstop property.
 *
 * Return value: the F-stop value, or %0 if unknown
 *
 * Since: 0.5.0
 **/
gdouble
gdata_picasaweb_file_get_fstop (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), 0);
	return gdata_exif_tags_get_fstop (self->priv->exif_tags);
}

/**
 * gdata_picasaweb_file_get_image_unique_id:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:image-unique-id property.
 *
 * Return value: the photo's unique EXIF identifier, or %NULL
 *
 * Since: 0.5.0
 **/
const gchar *
gdata_picasaweb_file_get_image_unique_id (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return gdata_exif_tags_get_image_unique_id (self->priv->exif_tags);
}

/**
 * gdata_picasaweb_file_get_iso:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:iso property.
 *
 * Return value: the ISO speed, or %-1 if unknown
 *
 * Since: 0.5.0
 **/
gint
gdata_picasaweb_file_get_iso (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), -1);
	return gdata_exif_tags_get_iso (self->priv->exif_tags);
}

/**
 * gdata_picasaweb_file_get_make:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:make property.
 *
 * Return value: the name of the manufacturer of the camera, or %NULL if unknown
 *
 * Since: 0.5.0
 **/
const gchar *
gdata_picasaweb_file_get_make (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return gdata_exif_tags_get_make (self->priv->exif_tags);
}

/**
 * gdata_picasaweb_file_get_model:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:model property.
 *
 * Return value: the model name of the camera, or %NULL if unknown
 *
 * Since: 0.5.0
 **/
const gchar *
gdata_picasaweb_file_get_model (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return gdata_exif_tags_get_model (self->priv->exif_tags);
}

/**
 * gdata_picasaweb_file_get_coordinates:
 * @self: a #GDataPicasaWebFile
 * @latitude: return location for the latitude, or %NULL
 * @longitude: return location for the longitude, or %NULL
 *
 * Gets the #GDataPicasaWebFile:latitude and #GDataPicasaWebFile:longitude properties, setting the out parameters to them.
 * If either latitude or longitude is %NULL, that parameter will not be set. If the coordinates are unset,
 * @latitude and @longitude will be set to %G_MAXDOUBLE.
 *
 * Since: 0.5.0
 **/
void
gdata_picasaweb_file_get_coordinates (GDataPicasaWebFile *self, gdouble *latitude, gdouble *longitude)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_FILE (self));

	if (latitude != NULL)
		*latitude = gdata_georss_where_get_latitude (self->priv->georss_where);
	if (longitude != NULL)
		*longitude = gdata_georss_where_get_longitude (self->priv->georss_where);
}

/**
 * gdata_picasaweb_file_set_coordinates:
 * @self: a #GDataPicasaWebFile
 * @latitude: the file's new latitude coordinate, or %G_MAXDOUBLE
 * @longitude: the file's new longitude coordinate, or %G_MAXDOUBLE
 *
 * Sets the #GDataPicasaWebFile:latitude and #GDataPicasaWebFile:longitude properties to
 * @latitude and @longitude respectively.
 *
 * Since: 0.5.0
 **/
void
gdata_picasaweb_file_set_coordinates (GDataPicasaWebFile *self, gdouble latitude, gdouble longitude)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_FILE (self));

	gdata_georss_where_set_latitude (self->priv->georss_where, latitude);
	gdata_georss_where_set_longitude (self->priv->georss_where, longitude);

	g_object_freeze_notify (G_OBJECT (self));
	g_object_notify (G_OBJECT (self), "latitude");
	g_object_notify (G_OBJECT (self), "longitude");
	g_object_thaw_notify (G_OBJECT (self));
}
