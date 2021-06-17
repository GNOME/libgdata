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
 * SECTION:gdata-picasaweb-file
 * @short_description: GData PicasaWeb file object
 * @stability: Stable
 * @include: gdata/services/picasaweb/gdata-picasaweb-file.h
 *
 * #GDataPicasaWebFile is a subclass of #GDataEntry to represent a file (photo or video) in an album on Google PicasaWeb.
 *
 * #GDataPicasaWebFile implements #GDataCommentable, allowing comments on files to be queried using gdata_commentable_query_comments(), new
 * comments to be added to files using gdata_commentable_insert_comment() and existing comments to be deleted from files using
 * gdata_commentable_delete_comment().
 *
 * For more details of Google PicasaWeb's GData API, see the
 * <ulink type="http" url="http://code.google.com/apis/picasaweb/developers_guide_protocol.html">online documentation</ulink>.
 *
 * <example>
 *	<title>Getting Basic Photo Data</title>
 *	<programlisting>
 *	GDataFeed *photo_feed;
 *	GList *photo_entries;
 *
 *	/<!-- -->* Query for a feed of GDataPicasaWebFiles belonging to the given GDataPicasaWebAlbum album *<!-- -->/
 *	photo_feed = gdata_picasaweb_service_query_files (service, album, NULL, NULL, NULL, NULL, NULL);
 *
 *	/<!-- -->* Get a list of GDataPicasaWebFiles from the query's feed *<!-- -->/
 *	for (photo_entries = gdata_feed_get_entries (photo_feed); photo_entries != NULL; photo_entries = photo_entries->next) {
 *		GDataPicasaWebFile *photo;
 *		guint height, width;
 *		gsize file_size;
 *		gint64 timestamp;
 *		const gchar *title, *summary;
 *		GList *contents;
 *
 *		photo = GDATA_PICASAWEB_FILE (photo_entries->data);
 *
 *		/<!-- -->* Get various bits of information about the photo *<!-- -->/
 *		height = gdata_picasaweb_file_get_height (photo);
 *		width = gdata_picasaweb_file_get_width (photo);
 *		file_size = gdata_picasaweb_file_get_size (photo);
 *		timestamp = gdata_picasaweb_file_get_timestamp (photo);
 *		title = gdata_entry_get_title (GDATA_ENTRY (photo));
 *		summary = gdata_entry_get_summary (GDATA_ENTRY (photo));
 *
 *		/<!-- -->* Obtain the image data at various sizes *<!-- -->/
 *		for (contents = gdata_picasaweb_file_get_contents (photo); contents != NULL; contents = contents->next) {
 *			GDataMediaContent *content;
 *			GDataDownloadStream *download_stream;
 *			GFileOutputStream *file_stream;
 *			GFile *new_file;
 *
 *			content = GDATA_MEDIA_CONTENT (contents->data);
 *			/<!-- -->* Do something fun with the actual images, like download them to a file.
 *			 * Note that this is a blocking operation. *<!-- -->/
 *			download_stream = gdata_media_content_download (content, GDATA_SERVICE (service), NULL, NULL);
 *			new_file = g_file_new_for_path (file_path);
 *			file_stream = g_file_create (new_file, G_FILE_CREATE_NONE, NULL, NULL);
 *			g_output_stream_splice (G_OUTPUT_STREAM (file_stream), G_INPUT_STREAM (download_stream),
 *			                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, NULL);
 *			g_object_unref (file_stream);
 *			g_object_unref (download_stream);
 *			/<!-- -->* ... *<!-- -->/
 *			g_object_unref (new_file);
 *		}
 *
 *		/<!-- -->* Do something worthwhile with your image data *<!-- -->/
 *	}
 *
 *	g_object_unref (photo_feed);
 *	</programlisting>
 * </example>
 *
 * Since: 0.4.0
 */

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
#include "gd/gdata-gd-feed-link.h"
#include "gdata-commentable.h"
#include "gdata-picasaweb-comment.h"
#include "gdata-picasaweb-service.h"

static GObject *gdata_picasaweb_file_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params);
static void gdata_picasaweb_file_dispose (GObject *object);
static void gdata_picasaweb_file_finalize (GObject *object);
static void gdata_picasaweb_file_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_picasaweb_file_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);
static gchar *get_entry_uri (const gchar *id) G_GNUC_WARN_UNUSED_RESULT;
GDataAuthorizationDomain *get_authorization_domain (GDataCommentable *self) G_GNUC_CONST;
static gchar *get_query_comments_uri (GDataCommentable *self) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
static gchar *get_insert_comment_uri (GDataCommentable *self, GDataComment *comment_) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
static gboolean is_comment_deletable (GDataCommentable *self, GDataComment *comment_);
static void gdata_picasaweb_file_commentable_init (GDataCommentableInterface *iface);

struct _GDataPicasaWebFilePrivate {
	gchar *file_id;
	gint64 edited;
	gchar *version;
	gchar *album_id;
	guint width;
	guint height;
	gsize size;
	gchar *checksum;
	gint64 timestamp; /* in milliseconds! */
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
	PROP_ALBUM_ID,
	PROP_WIDTH,
	PROP_HEIGHT,
	PROP_SIZE,
	PROP_CHECKSUM,
	PROP_TIMESTAMP,
	PROP_IS_COMMENTING_ENABLED,
	PROP_COMMENT_COUNT,
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
	PROP_LONGITUDE,
	PROP_FILE_ID
};

G_DEFINE_TYPE_WITH_CODE (GDataPicasaWebFile, gdata_picasaweb_file, GDATA_TYPE_ENTRY,
                         G_ADD_PRIVATE (GDataPicasaWebFile)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMMENTABLE, gdata_picasaweb_file_commentable_init))

static void
gdata_picasaweb_file_class_init (GDataPicasaWebFileClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	gobject_class->constructor = gdata_picasaweb_file_constructor;
	gobject_class->get_property = gdata_picasaweb_file_get_property;
	gobject_class->set_property = gdata_picasaweb_file_set_property;
	gobject_class->dispose = gdata_picasaweb_file_dispose;
	gobject_class->finalize = gdata_picasaweb_file_finalize;

	parsable_class->get_xml = get_xml;
	parsable_class->parse_xml = parse_xml;
	parsable_class->get_namespaces = get_namespaces;

	entry_class->get_entry_uri = get_entry_uri;
	entry_class->kind_term = "http://schemas.google.com/photos/2007#photo";

	/**
	 * GDataPicasaWebFile:file-id:
	 *
	 * The ID of the file. This is a substring of the ID returned by gdata_entry_get_id() for #GDataPicasaWebFiles; for example,
	 * if gdata_entry_get_id() returned
	 * "http://picasaweb.google.com/data/entry/user/libgdata.picasaweb/albumid/5328889949261497249/photoid/5328890138794566386" for a
	 * particular #GDataPicasaWebFile, the #GDataPicasaWebFile:file-id property would be "5328890138794566386".
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_id">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_FILE_ID,
	                                 g_param_spec_string ("file-id",
	                                                      "File ID", "The ID of the file.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:version:
	 *
	 * The version number of the file. Version numbers are based on modification time, so they don't increment linearly.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_version">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_VERSION,
	                                 g_param_spec_string ("version",
	                                                      "Version", "The version number of the file.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:album-id:
	 *
	 * The ID for the file's album. This is in the same form as returned by gdata_picasaweb_album_get_id().
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_albumid">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_ALBUM_ID,
	                                 g_param_spec_string ("album-id",
	                                                      "Album ID", "The ID for the file's album.",
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
	 */
	g_object_class_install_property (gobject_class, PROP_CHECKSUM,
	                                 g_param_spec_string ("checksum",
	                                                      "Checksum", "A checksum of the file, useful for duplicate detection.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:video-status:
	 *
	 * The status of the file, if it is a video. For example: %GDATA_PICASAWEB_VIDEO_STATUS_PENDING or %GDATA_PICASAWEB_VIDEO_STATUS_FAILED.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_videostatus">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_VIDEO_STATUS,
	                                 g_param_spec_string ("video-status",
	                                                      "Video Status", "The status of the file, if it is a video.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:width:
	 *
	 * The width of the photo or video, in pixels.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_width">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
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
	 */
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
	 */
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
	 */
	g_object_class_install_property (gobject_class, PROP_EDITED,
	                                 g_param_spec_int64 ("edited",
	                                                     "Edited", "The time this file was last edited.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:timestamp:
	 *
	 * The time the file was purportedly taken. This a UNIX timestamp in milliseconds (not seconds) since the epoch.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_timestamp">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_TIMESTAMP,
	                                 g_param_spec_int64 ("timestamp",
	                                                     "Timestamp", "The time the file was purportedly taken.",
	                                                     -1, G_MAXINT64, -1,
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
	 */
	g_object_class_install_property (gobject_class, PROP_COMMENT_COUNT,
	                                 g_param_spec_uint ("comment-count",
	                                                    "Comment Count", "The number of comments on the file.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:rotation:
	 *
	 * The rotation of the photo, in degrees. This will only be non-zero for files which are pending rotation, and haven't yet been
	 * permanently modified. For files which have already been rotated, this will be <code class="literal">0</code>.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_rotation">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
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
	 */
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
	 */
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
	 */
	g_object_class_install_property (gobject_class, PROP_CAPTION,
	                                 g_param_spec_string ("caption",
	                                                      "Caption", "The file's descriptive caption.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:tags:
	 *
	 * A %NULL-terminated array of tags associated with the file.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#media_keywords">
	 * Media RSS specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_TAGS,
	                                 g_param_spec_boxed ("tags",
	                                                     "Tags", "A NULL-terminated array of tags associated with the file.",
	                                                     G_TYPE_STRV,
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
	 */
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
	 */
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
	 */
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
	 */
	g_object_class_install_property (gobject_class, PROP_FOCAL_LENGTH,
	                                 g_param_spec_double ("focal-length",
	                                                      "Focal Length", "The focal length used in the shot.",
	                                                      -1.0, G_MAXDOUBLE, -1.0,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/* TODO: Rename to f-stop */
	/**
	 * GDataPicasaWebFile:fstop:
	 *
	 * The F-stop value.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#exif_reference">
	 * EXIF element reference</ulink>.
	 *
	 * Since: 0.5.0
	 */
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
	 */
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
	 */
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
	 */
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
	 */
	g_object_class_install_property (gobject_class, PROP_MODEL,
	                                 g_param_spec_string ("model",
	                                                      "Model", "The model of the camera.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:latitude:
	 *
	 * The location as a latitude coordinate associated with this file. Valid latitudes range from <code class="literal">-90.0</code>
	 * to <code class="literal">90.0</code> inclusive.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/docs/2.0/reference.html#georss_where">
	 * GeoRSS specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_LATITUDE,
	                                 g_param_spec_double ("latitude",
	                                                      "Latitude", "The location as a latitude coordinate associated with this file.",
	                                                      -90.0, 90.0, 0.0,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebFile:longitude:
	 *
	 * The location as a longitude coordinate associated with this file. Valid longitudes range from <code class="literal">-180.0</code>
	 * to <code class="literal">180.0</code> inclusive.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/docs/2.0/reference.html#georss_where">
	 * GeoRSS specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_LONGITUDE,
	                                 g_param_spec_double ("longitude",
	                                                      "Longitude", "The location as a longitude coordinate associated with this file.",
	                                                      -180.0, 180.0, 0.0,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_picasaweb_file_commentable_init (GDataCommentableInterface *iface)
{
	iface->comment_type = GDATA_TYPE_PICASAWEB_COMMENT;
	iface->get_authorization_domain = get_authorization_domain;
	iface->get_query_comments_uri = get_query_comments_uri;
	iface->get_insert_comment_uri = get_insert_comment_uri;
	iface->is_comment_deletable = is_comment_deletable;
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
	self->priv = gdata_picasaweb_file_get_instance_private (self);
	self->priv->media_group = g_object_new (GDATA_TYPE_MEDIA_GROUP, NULL);
	self->priv->exif_tags = g_object_new (GDATA_TYPE_EXIF_TAGS, NULL);
	self->priv->georss_where = g_object_new (GDATA_TYPE_GEORSS_WHERE, NULL);
	self->priv->is_commenting_enabled = TRUE;
	self->priv->edited = -1;
	self->priv->timestamp = -1;

	/* We need to keep atom:title (the canonical title for the file) in sync with media:group/media:title */
	g_signal_connect (self, "notify::title", G_CALLBACK (notify_title_cb), NULL);
	/* atom:summary (the canonical summary/caption for the file) in sync with media:group/media:description */
	g_signal_connect (self, "notify::summary", G_CALLBACK (notify_summary_cb), NULL);
}

static GObject *
gdata_picasaweb_file_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
	GObject *object;

	/* Chain up to the parent class */
	object = G_OBJECT_CLASS (gdata_picasaweb_file_parent_class)->constructor (type, n_construct_params, construct_params);

	if (_gdata_parsable_is_constructed_from_xml (GDATA_PARSABLE (object)) == FALSE) {
		GDataPicasaWebFilePrivate *priv = GDATA_PICASAWEB_FILE (object)->priv;
		gint64 time_val;

		/* Set the edited and timestamp properties to the current time (creation time). bgo#599140
		 * We don't do this in *_init() since that would cause setting it from parse_xml() to fail (duplicate element). */
		time_val = g_get_real_time () / G_USEC_PER_SEC;
		priv->timestamp = time_val * 1000;
		priv->edited = time_val;
	}

	return object;
}

static void
gdata_picasaweb_file_dispose (GObject *object)
{
	GDataPicasaWebFilePrivate *priv = GDATA_PICASAWEB_FILE (object)->priv;

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
	GDataPicasaWebFilePrivate *priv = GDATA_PICASAWEB_FILE (object)->priv;

	g_free (priv->file_id);
	g_free (priv->version);
	g_free (priv->album_id);
	g_free (priv->checksum);
	g_free (priv->video_status);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_picasaweb_file_parent_class)->finalize (object);
}

static void
gdata_picasaweb_file_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataPicasaWebFilePrivate *priv = GDATA_PICASAWEB_FILE (object)->priv;

	switch (property_id) {
		case PROP_FILE_ID:
			g_value_set_string (value, priv->file_id);
			break;
		case PROP_EDITED:
			g_value_set_int64 (value, priv->edited);
			break;
		case PROP_VERSION:
			g_value_set_string (value, priv->version);
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
		case PROP_CHECKSUM:
			g_value_set_string (value, priv->checksum);
			break;
		case PROP_TIMESTAMP:
			g_value_set_int64 (value, priv->timestamp);
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
			g_value_set_boxed (value, gdata_media_group_get_keywords (priv->media_group));
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
		case PROP_FILE_ID:
			/* Construct only */
			g_free (self->priv->file_id);
			self->priv->file_id = g_value_dup_string (value);
			break;
		case PROP_VERSION:
			/* Construct only */
			g_free (self->priv->version);
			self->priv->version = g_value_dup_string (value);
			break;
		case PROP_ALBUM_ID:
			/* TODO: do we allow this to change albums? I think that's how pictures are moved. */
			gdata_picasaweb_file_set_album_id (self, g_value_get_string (value));
			break;
		case PROP_CHECKSUM:
			gdata_picasaweb_file_set_checksum (self, g_value_get_string (value));
			break;
		case PROP_TIMESTAMP:
			gdata_picasaweb_file_set_timestamp (self, g_value_get_int64 (value));
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
			gdata_picasaweb_file_set_tags (self, g_value_get_boxed (value));
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
	gboolean success;
	GDataPicasaWebFile *self = GDATA_PICASAWEB_FILE (parsable);

	/* TODO: media:group should also be P_NO_DUPES, but we can't, as priv->media_group has to be pre-populated
	 * in order for things like gdata_picasaweb_file_set_description() to work. */
	if (gdata_parser_is_namespace (node, "http://www.w3.org/2007/app") == TRUE &&
	    gdata_parser_int64_time_from_element (node, "edited", P_REQUIRED | P_NO_DUPES, &(self->priv->edited), &success, error) == TRUE) {
		return success;
	} else if (gdata_parser_is_namespace (node, "http://search.yahoo.com/mrss/") == TRUE &&
	           gdata_parser_object_from_element (node, "group", P_REQUIRED, GDATA_TYPE_MEDIA_GROUP,
	                                             &(self->priv->media_group), &success, error) == TRUE) {
		return success;
	} else if (gdata_parser_is_namespace (node, "http://www.georss.org/georss") == TRUE &&
	          gdata_parser_object_from_element (node, "where", P_REQUIRED, GDATA_TYPE_GEORSS_WHERE,
	                                            &(self->priv->georss_where), &success, error) == TRUE) {
		return success;
	} else if (gdata_parser_is_namespace (node, "http://schemas.google.com/photos/exif/2007") == TRUE &&
	           gdata_parser_object_from_element (node, "tags", P_REQUIRED, GDATA_TYPE_EXIF_TAGS,
	                                             &(self->priv->exif_tags), &success, error) == TRUE) {
		return success;
	} else if (gdata_parser_is_namespace (node, "http://schemas.google.com/photos/2007") == TRUE) {
		if (gdata_parser_string_from_element (node, "videostatus", P_NO_DUPES, &(self->priv->video_status), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "imageVersion", P_NONE, &(self->priv->version), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "albumid", P_NONE, &(self->priv->album_id), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "checksum", P_NONE, &(self->priv->checksum), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "id", P_REQUIRED | P_NON_EMPTY | P_NO_DUPES,
		                                      &(self->priv->file_id), &success, error) == TRUE) {
			return success;
		} else if (xmlStrcmp (node->name, (xmlChar*) "width") == 0) {
			/* gphoto:width */
			xmlChar *width = xmlNodeListGetString (doc, node->children, TRUE);
			self->priv->width = g_ascii_strtoull ((gchar*) width, NULL, 10);
			xmlFree (width);
		} else if (xmlStrcmp (node->name, (xmlChar*) "height") == 0) {
			/* gphoto:height */
			xmlChar *height = xmlNodeListGetString (doc, node->children, TRUE);
			self->priv->height = g_ascii_strtoull ((gchar*) height, NULL, 10);
			xmlFree (height);
		} else if (xmlStrcmp (node->name, (xmlChar*) "size") == 0) {
			/* gphoto:size */
			xmlChar *size = xmlNodeListGetString (doc, node->children, TRUE);
			self->priv->size = g_ascii_strtoull ((gchar*) size, NULL, 10);
			xmlFree (size);
		} else if (xmlStrcmp (node->name, (xmlChar*) "timestamp") == 0) {
			/* gphoto:timestamp */
			xmlChar *timestamp_str;
			guint64 milliseconds;

			timestamp_str = xmlNodeListGetString (doc, node->children, TRUE);
			milliseconds = g_ascii_strtoull ((gchar*) timestamp_str, NULL, 10);
			xmlFree (timestamp_str);

			gdata_picasaweb_file_set_timestamp (self, (gint64) milliseconds);
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
			self->priv->comment_count = g_ascii_strtoull ((gchar*) comment_count, NULL, 10);
			xmlFree (comment_count);
		} else if (xmlStrcmp (node->name, (xmlChar*) "access") == 0) {
			/* gphoto:access */
			/* Visibility is already obtained through the album. When PicasaWeb supports per-file access restrictions,
			 * we'll expose this property. Until then, we'll catch this to suppress the Unhandled XML warning.
			 * See https://bugzilla.gnome.org/show_bug.cgi?id=589858 */
		} else if (xmlStrcmp (node->name, (xmlChar*) "rotation") == 0) {
			/* gphoto:rotation */
			xmlChar *rotation = xmlNodeListGetString (doc, node->children, TRUE);
			gdata_picasaweb_file_set_rotation (self, g_ascii_strtoull ((gchar*) rotation, NULL, 10));
			xmlFree (rotation);
		} else {
			return GDATA_PARSABLE_CLASS (gdata_picasaweb_file_parent_class)->parse_xml (parsable, doc, node, user_data, error);
		}
	} else {
		return GDATA_PARSABLE_CLASS (gdata_picasaweb_file_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}

	return TRUE;
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataPicasaWebFilePrivate *priv = GDATA_PICASAWEB_FILE (parsable)->priv;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_picasaweb_file_parent_class)->get_xml (parsable, xml_string);

	/* Add all the PicasaWeb-specific XML */
	if (priv->file_id != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gphoto:id>", priv->file_id, "</gphoto:id>");

	if (priv->version != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gphoto:imageVersion>", priv->version, "</gphoto:imageVersion>");

	if (priv->album_id != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gphoto:albumid>", priv->album_id, "</gphoto:albumid>");

	if (priv->checksum != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gphoto:checksum>", priv->checksum, "</gphoto:checksum>");

	if (priv->timestamp != -1) {
		/* timestamp is in milliseconds */
		g_string_append_printf (xml_string, "<gphoto:timestamp>%" G_GINT64_FORMAT "</gphoto:timestamp>", priv->timestamp);
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

static gchar *
get_entry_uri (const gchar *id)
{
	/* For files, the ID is of the form: "http://picasaweb.google.com/data/entry/user/liz/albumid/albumID/photoid/photoID"
	 *   whereas the URI is of the form: "http://picasaweb.google.com/data/entry/api/user/liz/albumid/albumID/photoid/photoID" */
	gchar **parts, *uri;

	parts = g_strsplit (id, "/entry/user/", 2);
	g_assert (parts[0] != NULL && parts[1] != NULL && parts[2] == NULL);
	uri = g_strconcat (parts[0], "/entry/api/user/", parts[1], NULL);
	g_strfreev (parts);

	return uri;
}

GDataAuthorizationDomain *
get_authorization_domain (GDataCommentable *self)
{
	return gdata_picasaweb_service_get_primary_authorization_domain ();
}

static gchar *
get_query_comments_uri (GDataCommentable *self)
{
	GDataLink *_link;
	SoupURI *uri;
	GHashTable *query;
	gchar *output_uri;

	/* Get the feed link of the form: https://picasaweb.google.com/data/feed/api/user/[userID]/albumid/[albumID]/photoid/[photoID] */
	_link = gdata_entry_look_up_link (GDATA_ENTRY (self), "http://schemas.google.com/g/2005#feed");
	g_assert (_link != NULL);

	/* We're going to query the comments belonging to the photo, so add the comment kind. This link isn't available as a normal <link> on
	 * photos. It's of the form: https://picasaweb.google.com/data/feed/api/user/[userID]/albumid/[albumID]/photoid/[photoID]?kind=comment */
	uri = soup_uri_new (gdata_link_get_uri (_link));
	query = soup_form_decode (uri->query);

	g_hash_table_replace (query, g_strdup ("kind"), (gchar*) "comment"); /* libsoup only specifies a destruction function for the key */
	soup_uri_set_query_from_form (uri, query);
	output_uri = soup_uri_to_string (uri, FALSE);

	g_hash_table_destroy (query);
	soup_uri_free (uri);

	return output_uri;
}

static gchar *
get_insert_comment_uri (GDataCommentable *self, GDataComment *comment_)
{
	GDataLink *_link;

	_link = gdata_entry_look_up_link (GDATA_ENTRY (self), "http://schemas.google.com/g/2005#feed");
	g_assert (_link != NULL);

	return g_strdup (gdata_link_get_uri (_link));
}

static gboolean
is_comment_deletable (GDataCommentable *self, GDataComment *comment_)
{
	return (gdata_entry_look_up_link (GDATA_ENTRY (comment_), GDATA_LINK_EDIT) != NULL) ? TRUE : FALSE;
}

/**
 * gdata_picasaweb_file_new:
 * @id: (allow-none): the file's ID, or %NULL
 *
 * Creates a new #GDataPicasaWebFile with the given ID and default properties.
 *
 * Return value: a new #GDataPicasaWebFile; unref with g_object_unref()
 *
 * Since: 0.4.0
 */
GDataPicasaWebFile *
gdata_picasaweb_file_new (const gchar *id)
{
	const gchar *file_id = NULL, *i;

	if (id != NULL) {
		file_id = g_strrstr (id, "/");
		if (file_id == NULL)
			return NULL;
		file_id++; /* skip the slash */

		/* Ensure the @file_id is entirely numeric */
		for (i = file_id; *i != '\0'; i = g_utf8_next_char (i)) {
			if (g_unichar_isdigit (g_utf8_get_char (i)) == FALSE)
				return NULL;
		}
	}

	return GDATA_PICASAWEB_FILE (g_object_new (GDATA_TYPE_PICASAWEB_FILE, "id", id, "file-id", file_id, NULL));
}

/**
 * gdata_picasaweb_file_get_id:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:file-id property.
 *
 * Return value: the file's ID
 *
 * Since: 0.7.0
 */
const gchar *
gdata_picasaweb_file_get_id (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return self->priv->file_id;
}

/**
 * gdata_picasaweb_file_get_edited:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:edited property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the UNIX timestamp for the time the file was last edited, or <code class="literal">-1</code>
 *
 * Since: 0.4.0
 */
gint64
gdata_picasaweb_file_get_edited (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), -1);
	return self->priv->edited;
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
 */
const gchar *
gdata_picasaweb_file_get_version (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return self->priv->version;
}

/**
 * gdata_picasaweb_file_get_album_id:
 * @self: a #GDataPicasaWebFile
 *
 * Gets the #GDataPicasaWebFile:album-id property. This is in the same form as returned by gdata_picasaweb_album_get_id().
 *
 * Return value: the ID of the album containing the #GDataPicasaWebFile
 *
 * Since: 0.4.0
 */
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
 */
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
 */
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
 */
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
 */
gsize
gdata_picasaweb_file_get_size (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), 0);
	return self->priv->size;
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
 */
const gchar *
gdata_picasaweb_file_get_checksum (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return self->priv->checksum;
}

/**
 * gdata_picasaweb_file_set_checksum:
 * @self: a #GDataPicasaWebFile
 * @checksum: (allow-none): the new checksum for this file, or %NULL
 *
 * Sets the #GDataPicasaWebFile:checksum property to @checksum.
 *
 * Set @checksum to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
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
 *
 * Gets the #GDataPicasaWebFile:timestamp property. It's a UNIX timestamp in milliseconds (not seconds) since the epoch. If the property is unset,
 * <code class="literal">-1</code> will be returned.
 *
 * Return value: the UNIX timestamp for the timestamp property in milliseconds, or <code class="literal">-1</code>
 *
 * Since: 0.4.0
 */
gint64
gdata_picasaweb_file_get_timestamp (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), -1);
	return self->priv->timestamp;
}

/**
 * gdata_picasaweb_file_set_timestamp:
 * @self: a #GDataPicasaWebFile
 * @timestamp: a UNIX timestamp, or <code class="literal">-1</code>
 *
 * Sets the #GDataPicasaWebFile:timestamp property from @timestamp. This should be a UNIX timestamp in milliseconds (not seconds) since the epoch. If
 * @timestamp is <code class="literal">-1</code>, the property will be unset.
 *
 * Since: 0.4.0
 */
void
gdata_picasaweb_file_set_timestamp (GDataPicasaWebFile *self, gint64 timestamp)
{
	/* RHSTODO: I think the timestamp value is just being
	   over-ridden by the file's actual EXIF time value; unless
	   we're setting this incorrectly here or in get_xml(); test that */
	/* RHSTODO: improve testing of setters in tests/picasa.c */

	g_return_if_fail (GDATA_IS_PICASAWEB_FILE (self));
	g_return_if_fail (timestamp >= -1);

	self->priv->timestamp = timestamp;
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
 */
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
 */
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
 */
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
 */
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
 */
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
 */
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
 * Return value: (array zero-terminated=1) (transfer none): a %NULL-terminated array of tags associated with the file, or %NULL
 *
 * Since: 0.4.0
 */
const gchar * const *
gdata_picasaweb_file_get_tags (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return gdata_media_group_get_keywords (self->priv->media_group);
}

/**
 * gdata_picasaweb_file_set_tags:
 * @self: a #GDataPicasaWebFile
 * @tags: (array zero-terminated=1) (allow-none): a new %NULL-terminated array of tags, or %NULL
 *
 * Sets the #GDataPicasaWebFile:tags property to @tags.
 *
 * Set @tags to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_picasaweb_file_set_tags (GDataPicasaWebFile *self, const gchar * const *tags)
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
 */
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
 */
const gchar *
gdata_picasaweb_file_get_caption (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return gdata_entry_get_summary (GDATA_ENTRY (self));
}

/**
 * gdata_picasaweb_file_set_caption:
 * @self: a #GDataPicasaWebFile
 * @caption: (allow-none): the file's new caption, or %NULL
 *
 * Sets the #GDataPicasaWebFile:caption property to @caption.
 *
 * Set @caption to %NULL to unset the file's caption.
 *
 * Since: 0.4.0
 */
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
 * Return value: (element-type GData.MediaContent) (transfer none): a #GList of #GDataMediaContent items
 *
 * Since: 0.4.0
 */
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
 * Returns a list of thumbnails, often at different sizes, for this
 * file.  Currently, PicasaWeb usually returns three thumbnails, with
 * widths in pixels of 72, 144, and 288.  However, the thumbnail will
 * not be larger than the actual image, so thumbnails may be smaller
 * than the widths listed above.
 *
 * Return value: (element-type GData.MediaThumbnail) (transfer none): a #GList of #GDataMediaThumbnails, or %NULL
 *
 * Since: 0.4.0
 */
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
 * Return value: the distance recorded in the photo's EXIF, or <code class="literal">-1</code> if unknown
 *
 * Since: 0.5.0
 */
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
 * Return value: the exposure value, or <code class="literal">0</code> if unknown
 *
 * Since: 0.5.0
 */
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
 */
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
 * Return value: the focal-length value, or <code class="literal">-1</code> if unknown
 *
 * Since: 0.5.0
 */
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
 * Return value: the F-stop value, or <code class="literal">0</code> if unknown
 *
 * Since: 0.5.0
 */
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
 */
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
 * Return value: the ISO speed, or <code class="literal">-1</code> if unknown
 *
 * Since: 0.5.0
 */
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
 */
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
 */
const gchar *
gdata_picasaweb_file_get_model (GDataPicasaWebFile *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_FILE (self), NULL);
	return gdata_exif_tags_get_model (self->priv->exif_tags);
}

/**
 * gdata_picasaweb_file_get_coordinates:
 * @self: a #GDataPicasaWebFile
 * @latitude: (out caller-allocates) (allow-none): return location for the latitude, or %NULL
 * @longitude: (out caller-allocates) (allow-none): return location for the longitude, or %NULL
 *
 * Gets the #GDataPicasaWebFile:latitude and #GDataPicasaWebFile:longitude properties, setting the out parameters to them.
 * If either latitude or longitude is %NULL, that parameter will not be set. If the coordinates are unset,
 * @latitude and @longitude will be set to %G_MAXDOUBLE.
 *
 * Since: 0.5.0
 */
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
 */
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
