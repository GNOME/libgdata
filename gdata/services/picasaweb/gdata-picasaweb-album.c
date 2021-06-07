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
 * SECTION:gdata-picasaweb-album
 * @short_description: GData PicasaWeb album object
 * @stability: Stable
 * @include: gdata/services/picasaweb/gdata-picasaweb-album.h
 *
 * #GDataPicasaWebAlbum is a subclass of #GDataEntry to represent an album from Google PicasaWeb.
 *
 * For more details of Google PicasaWeb's GData API, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html">
 * online documentation</ulink>.
 *
 * <example>
 * 	<title>Getting Basic Album Data</title>
 * 	<programlisting>
 *	GDataFeed *album_feed;
 *	GList *album_entries;
 *
 *	/<!-- -->* Query for a feed of GDataPicasaWebAlbums owned by user "libgdata.picasaweb" *<!-- -->/
 *	album_feed = gdata_picasaweb_service_query_all_albums (service, NULL, "libgdata.picasaweb", NULL, NULL, NULL, NULL);
 *
 *	/<!-- -->* Get a list of GDataPicasaWebAlbums from the query's feed *<!-- -->/
 *	for (album_entries = gdata_feed_get_entries (album_feed); album_entries != NULL; album_entries = album_entries->next) {
 *		GDataPicasaWebAlbum *album;
 *		guint num_photos;
 *		const gchar *owner_nickname, *title, *summary;
 *		gint64 timestamp;
 *		GList *thumbnails;
 *
 *		album = GDATA_PICASAWEB_ALBUM (album_entries->data);
 *
 *		/<!-- -->* Get various bits of information about the album *<!-- -->/
 *		num_photos = gdata_picasaweb_album_get_num_photos (album);
 *		owner_nickname = gdata_picasaweb_album_get_nickname (album);
 *		title = gdata_entry_get_title (GDATA_ENTRY (album));
 *		summary = gdata_entry_get_summary (GDATA_ENTRY (album));
 *		/<!-- -->* Get the day the album was shot on or, if not set, when it was uploaded. This is in milliseconds since the epoch. *<!-- -->/
 *		timestamp = gdata_picasaweb_album_get_timestamp (album);
 *
 *		for (thumbnails = gdata_picasaweb_album_get_thumbnails (album); thumbnails != NULL; thumbnails = thumbnails->next) {
 *			GDataMediaThumbnail *thumbnail;
 *			GDataDownloadStream *download_stream;
 *			GdkPixbuf *pixbuf;
 *
 *			thumbnail = GDATA_MEDIA_THUMBNAIL (thumbnails->data);
 *			/<!-- -->* Do something fun with the thumbnails, like download and display them. We could just as easily download them into
 *			 * files using g_file_create() and g_output_stream_splice(), rather than create GdkPixbufs directly from them.
 *			 * Note that this is a blocking operation. *<!-- -->/
 *			download_stream = gdata_media_thumbnail_download (thumbnail, GDATA_SERVICE (service), NULL, NULL);
 *			pixbuf = gdk_pixbuf_new_from_stream (G_INPUT_STREAM (download_stream), NULL, NULL);
 *			g_object_unref (download_stream);
 *			/<!-- -->* ... *<!-- -->/
 *			g_object_unref (pixbuf);
 *		}
 *
 *		/<!-- -->* Do something worthwhile with your album data *<!-- -->/
 *	}
 *
 *	g_object_unref (album_feed);
 * 	</programlisting>
 * </example>
 *
 * Since: 0.4.0
 */

/* TODO: support the album cover/icon ? I think this is already done with the thumbnails, but we don't set it yet :( */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-picasaweb-album.h"
#include "gdata-private.h"
#include "gdata-service.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "media/gdata-media-group.h"
#include "gdata-picasaweb-enums.h"
#include "georss/gdata-georss-where.h"

static GObject *gdata_picasaweb_album_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params);
static void gdata_picasaweb_album_dispose (GObject *object);
static void gdata_picasaweb_album_finalize (GObject *object);
static void gdata_picasaweb_album_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_picasaweb_album_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataPicasaWebAlbumPrivate {
	gchar *album_id;
	gchar *user;
	gchar *nickname;
	gint64 edited;
	gchar *location;
	GDataPicasaWebVisibility visibility;
	gint64 timestamp; /* in milliseconds! */
	guint num_photos;
	guint num_photos_remaining;
	glong bytes_used;
	gboolean is_commenting_enabled;
	guint comment_count;

	/* media:group */
	GDataMediaGroup *media_group;
	/* georss:where */
	GDataGeoRSSWhere *georss_where;
};

enum {
	PROP_USER = 1,
	PROP_NICKNAME,
	PROP_EDITED,
	PROP_LOCATION,
	PROP_VISIBILITY,
	PROP_TIMESTAMP,
	PROP_NUM_PHOTOS,
	PROP_NUM_PHOTOS_REMAINING,
	PROP_BYTES_USED,
	PROP_IS_COMMENTING_ENABLED,
	PROP_COMMENT_COUNT,
	PROP_TAGS,
	PROP_LATITUDE,
	PROP_LONGITUDE,
	PROP_ALBUM_ID
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataPicasaWebAlbum, gdata_picasaweb_album, GDATA_TYPE_ENTRY)

static void
gdata_picasaweb_album_class_init (GDataPicasaWebAlbumClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	gobject_class->constructor = gdata_picasaweb_album_constructor;
	gobject_class->get_property = gdata_picasaweb_album_get_property;
	gobject_class->set_property = gdata_picasaweb_album_set_property;
	gobject_class->dispose = gdata_picasaweb_album_dispose;
	gobject_class->finalize = gdata_picasaweb_album_finalize;

	parsable_class->parse_xml = parse_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;

	entry_class->kind_term = "http://schemas.google.com/photos/2007#album";

	/**
	 * GDataPicasaWebAlbum:album-id:
	 *
	 * The ID of the album. This is a substring of the ID returned by gdata_entry_get_id() for #GDataPicasaWebAlbums; for example,
	 * if gdata_entry_get_id() returned "http://picasaweb.google.com/data/entry/user/libgdata.picasaweb/albumid/5328889949261497249" for a
	 * particular #GDataPicasaWebAlbum, the #GDataPicasaWebAlbum:album-id property would be "5328889949261497249".
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_id">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_ALBUM_ID,
	                                 g_param_spec_string ("album-id",
	                                                      "Album ID", "The ID of the album.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebAlbum:user:
	 *
	 * The username of the album owner.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_user">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_USER,
	                                 g_param_spec_string ("user",
	                                                      "User", "The username of the album owner.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebAlbum:nickname:
	 *
	 * The user's nickname. This is a user-specified value that should be used when referring to the user by name.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_nickname">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_NICKNAME,
	                                 g_param_spec_string ("nickname",
	                                                      "Nickname", "The user's nickname.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebAlbum:edited:
	 *
	 * The time this album was last edited. If the album has not been edited yet, the content indicates the time it was created.
	 *
	 * For more information, see the <ulink type="http" url="http://www.atomenabled.org/developers/protocol/#appEdited">
	 * Atom Publishing Protocol specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_EDITED,
	                                 g_param_spec_int64 ("edited",
	                                                     "Edited", "The time this album was last edited.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebAlbum:location:
	 *
	 * The user-specified location associated with the album. A place name.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_location">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_LOCATION,
	                                 g_param_spec_string ("location",
	                                                      "Location", "The user-specified location associated with the album.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebAlbum:visibility:
	 *
	 * The visibility (or access rights) of the album.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_access">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_VISIBILITY,
	                                 g_param_spec_enum ("visibility",
	                                                    "Visibility", "The visibility (or access rights) of the album.",
	                                                    GDATA_TYPE_PICASAWEB_VISIBILITY, GDATA_PICASAWEB_PRIVATE,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebAlbum:timestamp:
	 *
	 * The timestamp of when the album occurred, settable by the user. This a UNIX timestamp in milliseconds (not seconds) since the epoch.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_timestamp">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_TIMESTAMP,
	                                 g_param_spec_int64 ("timestamp",
	                                                     "Timestamp", "The timestamp of when the album occurred, settable by the user.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/* TODO: Change to photo-count? */
	/**
	 * GDataPicasaWebAlbum:num-photos:
	 *
	 * The number of photos and videos in the album.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_numphotos">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_NUM_PHOTOS,
	                                 g_param_spec_uint ("num-photos",
	                                                    "Number of photos", "The number of photos and videos in the album.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/* TODO: Change to remaining-photos-count? */
	/**
	 * GDataPicasaWebAlbum:num-photos-remaining:
	 *
	 * The number of photos and videos that can still be uploaded to this album.
	 * This doesn't account for quota, just a hardcoded maximum number per album set by Google.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_numphotosremaining">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_NUM_PHOTOS_REMAINING,
	                                 g_param_spec_uint ("num-photos-remaining",
	                                                    "Number of photo spaces remaining", "The number of files spaces still free for uploads.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebAlbum:bytes-used:
	 *
	 * The number of bytes consumed by this album and its contents. Note that this is only set if the authenticated user is the owner of the
	 * album; it's otherwise <code class="literal">-1</code>.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_bytesUsed">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_BYTES_USED,
	                                 g_param_spec_long ("bytes-used",
	                                                    "Number of bytes used", "The number of bytes consumed by this album and its contents.",
	                                                    -1, G_MAXLONG, -1,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebAlbum:commenting-enabled:
	 *
	 * Whether commenting is enabled for this album.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_IS_COMMENTING_ENABLED,
	                                 g_param_spec_boolean ("is-commenting-enabled",
	                                                       "Commenting enabled?", "Whether commenting is enabled for this album.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebAlbum:comment-count:
	 *
	 * The number of comments on the album.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#gphoto_commentCount">
	 * gphoto specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_COMMENT_COUNT,
	                                 g_param_spec_uint ("comment-count",
	                                                    "Comment count", "The number of comments on the album.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebAlbum:tags:
	 *
	 * A %NULL-terminated array of tags associated with the album; all the tags associated with the individual photos in the album.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/reference.html#media_keywords">
	 * Media RSS specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_TAGS,
	                                 g_param_spec_boxed ("tags",
	                                                     "Tags", "A NULL-terminated array of tags associated with the album",
	                                                     G_TYPE_STRV,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebAlbum:latitude:
	 *
	 * The location as a latitude coordinate associated with this album. Valid latitudes range from <code class="literal">-90.0</code>
	 * to <code class="literal">90.0</code> inclusive.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/docs/2.0/reference.html#georss_where">
	 * GeoRSS specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_LATITUDE,
	                                 g_param_spec_double ("latitude",
	                                                      "Latitude", "The location as a latitude coordinate associated with this album.",
	                                                      -90.0, 90.0, 0.0,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebAlbum:longitude:
	 *
	 * The location as a longitude coordinate associated with this album. Valid longitudes range from <code class="literal">-180.0</code>
	 * to <code class="literal">180.0</code> inclusive.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/picasaweb/docs/2.0/reference.html#georss_where">
	 * GeoRSS specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_LONGITUDE,
	                                 g_param_spec_double ("longitude",
	                                                      "Longitude", "The location as a longitude coordinate associated with this album.",
	                                                      -180.0, 180.0, 0.0,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
notify_title_cb (GDataPicasaWebAlbum *self, GParamSpec *pspec, gpointer user_data)
{
	/* Update our media:group title */
	if (self->priv->media_group != NULL)
		gdata_media_group_set_title (self->priv->media_group, gdata_entry_get_title (GDATA_ENTRY (self)));
}

static void
notify_summary_cb (GDataPicasaWebAlbum *self, GParamSpec *pspec, gpointer user_data)
{
	/* Update our media:group description */
	if (self->priv->media_group != NULL)
		gdata_media_group_set_description (self->priv->media_group, gdata_entry_get_summary (GDATA_ENTRY (self)));
}

static void notify_visibility_cb (GDataPicasaWebAlbum *self, GParamSpec *pspec, gpointer user_data);

static void
notify_rights_cb (GDataPicasaWebAlbum *self, GParamSpec *pspec, gpointer user_data)
{
	const gchar *rights = gdata_entry_get_rights (GDATA_ENTRY (self));

	/* Update our gphoto:visibility */
	g_signal_handlers_block_by_func (self, notify_visibility_cb, NULL);

	if (rights == NULL || strcmp (rights, "public") == 0) {
		gdata_picasaweb_album_set_visibility (self, GDATA_PICASAWEB_PUBLIC);
	} else if (strcmp (rights, "private") == 0) {
		gdata_picasaweb_album_set_visibility (self, GDATA_PICASAWEB_PRIVATE);
	} else {
		/* Print out a warning and leave the visibility as it is */
		g_warning ("Unknown <rights> or <gd:access> value: %s", rights);
	}

	g_signal_handlers_unblock_by_func (self, notify_visibility_cb, NULL);
}

static void
notify_visibility_cb (GDataPicasaWebAlbum *self, GParamSpec *pspec, gpointer user_data)
{
	/* Update our GDataEntry's atom:rights */
	g_signal_handlers_block_by_func (self, notify_rights_cb, NULL);

	switch (self->priv->visibility) {
		case GDATA_PICASAWEB_PUBLIC:
			gdata_entry_set_rights (GDATA_ENTRY (self), "public");
			break;
		case GDATA_PICASAWEB_PRIVATE:
			gdata_entry_set_rights (GDATA_ENTRY (self), "private");
			break;
		default:
			g_assert_not_reached ();
	}

	g_signal_handlers_unblock_by_func (self, notify_rights_cb, NULL);
}

static void
gdata_picasaweb_album_init (GDataPicasaWebAlbum *self)
{
	self->priv = gdata_picasaweb_album_get_instance_private (self);
	self->priv->media_group = g_object_new (GDATA_TYPE_MEDIA_GROUP, NULL);
	self->priv->georss_where = g_object_new (GDATA_TYPE_GEORSS_WHERE, NULL);
	self->priv->edited = -1;
	self->priv->timestamp = -1;

	/* Set the initial visibility */
	self->priv->visibility = GDATA_PICASAWEB_PRIVATE;
	gdata_entry_set_rights (GDATA_ENTRY (self), "private");

	/* Connect to the notify::title signal from GDataEntry so our media:group title can be kept in sync
	 * (the title of an album is duplicated in atom:title and media:group/media:title) */
	g_signal_connect (GDATA_ENTRY (self), "notify::title", G_CALLBACK (notify_title_cb), NULL);
	/* Connect to the notify::description signal from GDataEntry so our media:group description can be kept in sync
	 * (the description of an album is duplicated in atom:summary and media:group/media:description) */
	g_signal_connect (GDATA_ENTRY (self), "notify::summary", G_CALLBACK (notify_summary_cb), NULL);
	/* Connect to the notify::rights signal from GDataEntry so our gphoto:visibility can be kept in sync (and vice-versa)
	 * (visibility settings are duplicated in atom:rights and gphoto:visibility) */
	g_signal_connect (GDATA_ENTRY (self), "notify::rights", G_CALLBACK (notify_rights_cb), NULL);
	g_signal_connect (self, "notify::visibility", G_CALLBACK (notify_visibility_cb), NULL);
}

static GObject *
gdata_picasaweb_album_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
	GObject *object;

	/* Chain up to the parent class */
	object = G_OBJECT_CLASS (gdata_picasaweb_album_parent_class)->constructor (type, n_construct_params, construct_params);

	if (_gdata_parsable_is_constructed_from_xml (GDATA_PARSABLE (object)) == FALSE) {
		GDataPicasaWebAlbumPrivate *priv = GDATA_PICASAWEB_ALBUM (object)->priv;
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
gdata_picasaweb_album_dispose (GObject *object)
{
	GDataPicasaWebAlbumPrivate *priv = GDATA_PICASAWEB_ALBUM (object)->priv;

	if (priv->media_group != NULL)
		g_object_unref (priv->media_group);
	priv->media_group = NULL;

	if (priv->georss_where != NULL)
		g_object_unref (priv->georss_where);
	priv->georss_where = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_picasaweb_album_parent_class)->dispose (object);
}

static void
gdata_picasaweb_album_finalize (GObject *object)
{
	GDataPicasaWebAlbumPrivate *priv = GDATA_PICASAWEB_ALBUM (object)->priv;

	g_free (priv->album_id);
	g_free (priv->user);
	g_free (priv->nickname);
	g_free (priv->location);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_picasaweb_album_parent_class)->finalize (object);
}

static void
gdata_picasaweb_album_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataPicasaWebAlbumPrivate *priv = GDATA_PICASAWEB_ALBUM (object)->priv;

	switch (property_id) {
		case PROP_ALBUM_ID:
			g_value_set_string (value, priv->album_id);
			break;
		case PROP_USER:
			g_value_set_string (value, priv->user);
			break;
		case PROP_NICKNAME:
			g_value_set_string (value, priv->nickname);
			break;
		case PROP_EDITED:
			g_value_set_int64 (value, priv->edited);
			break;
		case PROP_LOCATION:
			g_value_set_string (value, priv->location);
			break;
		case PROP_VISIBILITY:
			g_value_set_enum (value, priv->visibility);
			break;
		case PROP_TIMESTAMP:
			g_value_set_int64 (value, priv->timestamp);
			break;
		case PROP_NUM_PHOTOS:
			g_value_set_uint (value, priv->num_photos);
			break;
		case PROP_NUM_PHOTOS_REMAINING:
			g_value_set_uint (value, priv->num_photos_remaining);
			break;
		case PROP_BYTES_USED:
			g_value_set_long (value, priv->bytes_used);
			break;
		case PROP_IS_COMMENTING_ENABLED:
			g_value_set_boolean (value, priv->is_commenting_enabled);
			break;
		case PROP_COMMENT_COUNT:
			g_value_set_uint (value, priv->comment_count);
			break;
		case PROP_TAGS:
			g_value_set_boxed (value, gdata_media_group_get_keywords (priv->media_group));
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
gdata_picasaweb_album_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataPicasaWebAlbum *self = GDATA_PICASAWEB_ALBUM (object);

	switch (property_id) {
		case PROP_ALBUM_ID:
			/* Construct only */
			g_free (self->priv->album_id);
			self->priv->album_id = g_value_dup_string (value);
			break;
		case PROP_LOCATION:
			gdata_picasaweb_album_set_location (self, g_value_get_string (value));
			break;
		case PROP_VISIBILITY:
			gdata_picasaweb_album_set_visibility (self, g_value_get_enum (value));
			break;
		case PROP_TIMESTAMP:
			gdata_picasaweb_album_set_timestamp (self, g_value_get_int64 (value));
			break;
		case PROP_IS_COMMENTING_ENABLED:
			gdata_picasaweb_album_set_is_commenting_enabled (self, g_value_get_boolean (value));
			break;
		case PROP_TAGS:
			gdata_picasaweb_album_set_tags (self, g_value_get_boxed (value));
			break;
		case PROP_LATITUDE:
			gdata_picasaweb_album_set_coordinates (self, g_value_get_double (value),
							       gdata_georss_where_get_longitude (self->priv->georss_where));
			break;
		case PROP_LONGITUDE:
			gdata_picasaweb_album_set_coordinates (self, gdata_georss_where_get_latitude (self->priv->georss_where),
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
	GDataPicasaWebAlbum *self = GDATA_PICASAWEB_ALBUM (parsable);

	/* TODO: media:group should also be P_NO_DUPES, but we can't, as priv->media_group has to be pre-populated
	 * in order for things like gdata_picasaweb_album_get_tags() to work. */
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
	} else if (gdata_parser_is_namespace (node, "http://schemas.google.com/photos/2007") == TRUE) {
		if (gdata_parser_string_from_element (node, "user", P_REQUIRED | P_NON_EMPTY, &(self->priv->user), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "nickname", P_REQUIRED | P_NON_EMPTY, &(self->priv->nickname), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "location", P_NONE, &(self->priv->location), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "id", P_REQUIRED | P_NON_EMPTY | P_NO_DUPES,
		                                      &(self->priv->album_id), &success, error) == TRUE) {
			return success;
		} else if (xmlStrcmp (node->name, (xmlChar*) "access") == 0) {
			/* gphoto:access */
			xmlChar *access_level = xmlNodeListGetString (doc, node->children, TRUE);
			if (xmlStrcmp (access_level, (xmlChar*) "public") == 0) {
				gdata_picasaweb_album_set_visibility (self, GDATA_PICASAWEB_PUBLIC);
			} else if (xmlStrcmp (access_level, (xmlChar*) "private") == 0 ||
			           xmlStrcmp (access_level, (xmlChar*) "protected") == 0) {
				gdata_picasaweb_album_set_visibility (self, GDATA_PICASAWEB_PRIVATE);
			} else {
				gdata_parser_error_unknown_content (node, (gchar*) access_level, error);
				xmlFree (access_level);
				return FALSE;
			}
			xmlFree (access_level);
		} else if (xmlStrcmp (node->name, (xmlChar*) "timestamp") == 0) {
			/* gphoto:timestamp */
			xmlChar *timestamp_str;
			guint64 milliseconds;

			timestamp_str = xmlNodeListGetString (doc, node->children, TRUE);
			milliseconds = g_ascii_strtoull ((gchar*) timestamp_str, NULL, 10);
			xmlFree (timestamp_str);

			gdata_picasaweb_album_set_timestamp (self, (gint64) milliseconds);
		} else if (xmlStrcmp (node->name, (xmlChar*) "numphotos") == 0) {
			/* gphoto:numphotos */
			xmlChar *num_photos = xmlNodeListGetString (doc, node->children, TRUE);
			if (num_photos == NULL || *num_photos == '\0') {
				xmlFree (num_photos);
				return gdata_parser_error_required_content_missing (node, error);
			}

			self->priv->num_photos = g_ascii_strtoull ((char*) num_photos, NULL, 10);
			xmlFree (num_photos);
		} else if (xmlStrcmp (node->name, (xmlChar*) "numphotosremaining") == 0) {
			/* gphoto:numphotosremaining */
			xmlChar *num_photos_remaining = xmlNodeListGetString (doc, node->children, TRUE);
			if (num_photos_remaining == NULL || *num_photos_remaining == '\0') {
				xmlFree (num_photos_remaining);
				return gdata_parser_error_required_content_missing (node, error);
			}

			self->priv->num_photos_remaining = g_ascii_strtoull ((char*) num_photos_remaining, NULL, 10);
			xmlFree (num_photos_remaining);
		} else if (xmlStrcmp (node->name, (xmlChar*) "bytesUsed") == 0) {
			/* gphoto:bytesUsed */
			xmlChar *bytes_used = xmlNodeListGetString (doc, node->children, TRUE);
			if (bytes_used == NULL || *bytes_used == '\0') {
				xmlFree (bytes_used);
				return gdata_parser_error_required_content_missing (node, error);
			}

			self->priv->bytes_used = g_ascii_strtoll ((char*) bytes_used, NULL, 10);
			xmlFree (bytes_used);
		} else if (xmlStrcmp (node->name, (xmlChar*) "commentingEnabled") == 0) {
			/* gphoto:commentingEnabled */
			xmlChar *commenting_enabled = xmlNodeListGetString (doc, node->children, TRUE);
			if (commenting_enabled == NULL || *commenting_enabled == '\0') {
				xmlFree (commenting_enabled);
				return gdata_parser_error_required_content_missing (node, error);
			}

			gdata_picasaweb_album_set_is_commenting_enabled (self,
			                                                 (xmlStrcmp (commenting_enabled, (xmlChar*) "true") == 0) ? TRUE : FALSE);
			xmlFree (commenting_enabled);
		} else if (xmlStrcmp (node->name, (xmlChar*) "commentCount") == 0) {
			/* gphoto:commentCount */
			xmlChar *comment_count = xmlNodeListGetString (doc, node->children, TRUE);
			if (comment_count == NULL || *comment_count == '\0') {
				xmlFree (comment_count);
				return gdata_parser_error_required_content_missing (node, error);
			}

			self->priv->comment_count = g_ascii_strtoull ((char*) comment_count, NULL, 10);
			xmlFree (comment_count);
		} else {
			return GDATA_PARSABLE_CLASS (gdata_picasaweb_album_parent_class)->parse_xml (parsable, doc, node, user_data, error);
		}
	} else {
		return GDATA_PARSABLE_CLASS (gdata_picasaweb_album_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}

	return TRUE;
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataPicasaWebAlbumPrivate *priv = GDATA_PICASAWEB_ALBUM (parsable)->priv;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_picasaweb_album_parent_class)->get_xml (parsable, xml_string);

	/* Add all the album-specific XML */
	if (priv->album_id != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gphoto:id>", priv->album_id, "</gphoto:id>");

	if (priv->location != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gphoto:location>", priv->location, "</gphoto:location>");

	switch (priv->visibility) {
		case GDATA_PICASAWEB_PUBLIC:
			g_string_append (xml_string, "<gphoto:access>public</gphoto:access>");
			break;
		case GDATA_PICASAWEB_PRIVATE:
			g_string_append (xml_string, "<gphoto:access>private</gphoto:access>");
			break;
		default:
			g_assert_not_reached ();
	}

	if (priv->timestamp != -1) {
		/* in milliseconds */
		g_string_append_printf (xml_string, "<gphoto:timestamp>%" G_GINT64_FORMAT "</gphoto:timestamp>", priv->timestamp);
	}

	if (priv->is_commenting_enabled == FALSE)
		g_string_append (xml_string, "<gphoto:commentingEnabled>false</gphoto:commentingEnabled>");
	else
		g_string_append (xml_string, "<gphoto:commentingEnabled>true</gphoto:commentingEnabled>");

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
	GDataPicasaWebAlbumPrivate *priv = GDATA_PICASAWEB_ALBUM (parsable)->priv;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_picasaweb_album_parent_class)->get_namespaces (parsable, namespaces);

	g_hash_table_insert (namespaces, (gchar*) "gphoto", (gchar*) "http://schemas.google.com/photos/2007");
	g_hash_table_insert (namespaces, (gchar*) "app", (gchar*) "http://www.w3.org/2007/app");

	/* Add the media:group namespaces */
	GDATA_PARSABLE_GET_CLASS (priv->media_group)->get_namespaces (GDATA_PARSABLE (priv->media_group), namespaces);
	/* Add the georss:where namespaces */
	GDATA_PARSABLE_GET_CLASS (priv->georss_where)->get_namespaces (GDATA_PARSABLE (priv->georss_where), namespaces);
}

/**
 * gdata_picasaweb_album_new:
 * @id: (allow-none): the album's entry ID, or %NULL
 *
 * Creates a new #GDataPicasaWebAlbum with the given ID and default properties. @id is the ID which would be returned by gdata_entry_get_id(),
 * not gdata_picasaweb_album_get_id().
 *
 * If @id is not %NULL and can't be parsed to extract an album ID, %NULL will be returned.
 *
 * Return value: a new #GDataPicasaWebAlbum, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 */
GDataPicasaWebAlbum *
gdata_picasaweb_album_new (const gchar *id)
{
	const gchar *album_id = NULL, *i;

	if (id != NULL) {
		album_id = g_strrstr (id, "/");
		if (album_id == NULL)
			return NULL;
		album_id++; /* skip the slash */

		/* Ensure the @album_id is entirely numeric */
		for (i = album_id; *i != '\0'; i = g_utf8_next_char (i)) {
			if (g_unichar_isdigit (g_utf8_get_char (i)) == FALSE)
				return NULL;
		}
	}

	return GDATA_PICASAWEB_ALBUM (g_object_new (GDATA_TYPE_PICASAWEB_ALBUM, "id", id, "album-id", album_id, NULL));
}

/**
 * gdata_picasaweb_album_get_id:
 * @self: a #GDataPicasaWebAlbum
 *
 * Gets the #GDataPicasaWebAlbum:album-id property.
 *
 * Return value: the album's ID
 *
 * Since: 0.7.0
 */
const gchar *
gdata_picasaweb_album_get_id (GDataPicasaWebAlbum *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_ALBUM (self), NULL);
	return self->priv->album_id;
}

/**
 * gdata_picasaweb_album_get_user:
 * @self: a #GDataPicasaWebAlbum
 *
 * Gets the #GDataPicasaWebAlbum:user property.
 *
 * Return value: the album owner's username
 *
 * Since: 0.4.0
 */
const gchar *
gdata_picasaweb_album_get_user (GDataPicasaWebAlbum *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_ALBUM (self), NULL);
	return self->priv->user;
}

/**
 * gdata_picasaweb_album_get_nickname:
 * @self: a #GDataPicasaWebAlbum
 *
 * Gets the #GDataPicasaWebAlbum:nickname property.
 *
 * Return value: the album owner's nickname
 *
 * Since: 0.4.0
 */
const gchar *
gdata_picasaweb_album_get_nickname (GDataPicasaWebAlbum *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_ALBUM (self), NULL);
	return self->priv->nickname;
}

/**
 * gdata_picasaweb_album_get_edited:
 * @self: a #GDataPicasaWebAlbum
 *
 * Gets the #GDataPicasaWebAlbum:edited property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the UNIX timestamp for the time the album was last edited, or <code class="literal">-1</code>
 *
 * Since: 0.4.0
 */
gint64
gdata_picasaweb_album_get_edited (GDataPicasaWebAlbum *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_ALBUM (self), -1);
	return self->priv->edited;
}

/**
 * gdata_picasaweb_album_get_location:
 * @self: a #GDataPicasaWebAlbum
 *
 * Gets the #GDataPicasaWebAlbum:location property.
 *
 * Return value: the album's location, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_picasaweb_album_get_location (GDataPicasaWebAlbum *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_ALBUM (self), NULL);
	return self->priv->location;
}

/**
 * gdata_picasaweb_album_set_location:
 * @self: a #GDataPicasaWebAlbum
 * @location: (allow-none): the new album location, or %NULL
 *
 * Sets the #GDataPicasaWebAlbum:location property to @location.
 *
 * Set @location to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_picasaweb_album_set_location (GDataPicasaWebAlbum *self, const gchar *location)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_ALBUM (self));

	g_free (self->priv->location);
	self->priv->location = g_strdup (location);
	g_object_notify (G_OBJECT (self), "location");
}

/**
 * gdata_picasaweb_album_get_visibility:
 * @self: a #GDataPicasaWebAlbum
 *
 * Gets the #GDataPicasaWebAlbum:visibility property.
 *
 * Return value: the album's visibility level
 *
 * Since: 0.4.0
 */
GDataPicasaWebVisibility
gdata_picasaweb_album_get_visibility (GDataPicasaWebAlbum *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_ALBUM (self), GDATA_PICASAWEB_PUBLIC);
	return self->priv->visibility;
}

/**
 * gdata_picasaweb_album_set_visibility:
 * @self: a #GDataPicasaWebAlbum
 * @visibility: the new album visibility level
 *
 * Sets the #GDataPicasaWebAlbum:visibility property to @visibility.
 *
 * Since: 0.4.0
 */
void
gdata_picasaweb_album_set_visibility (GDataPicasaWebAlbum *self, GDataPicasaWebVisibility visibility)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_ALBUM (self));

	self->priv->visibility = visibility;
	g_object_notify (G_OBJECT (self), "visibility");
}

/**
 * gdata_picasaweb_album_get_timestamp:
 * @self: a #GDataPicasaWebAlbum
 *
 * Gets the #GDataPicasaWebAlbum:timestamp property. This value usually holds either the date that best corresponds to the album of photos, or to the
 * day it was uploaded. It's a UNIX timestamp in milliseconds (not seconds) since the epoch. If the property is unset, <code class="literal">-1</code>
 * will be returned.
 *
 * Return value: the UNIX timestamp for the timestamp property in milliseconds, or <code class="literal">-1</code>
 *
 * Since: 0.4.0
 */
gint64
gdata_picasaweb_album_get_timestamp (GDataPicasaWebAlbum *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_ALBUM (self), -1);
	return self->priv->timestamp;
}

/**
 * gdata_picasaweb_album_set_timestamp:
 * @self: a #GDataPicasaWebAlbum
 * @timestamp: a UNIX timestamp, or <code class="literal">-1</code>
 *
 * Sets the #GDataPicasaWebAlbum:timestamp property from @timestamp. This should be a UNIX timestamp in milliseconds (not seconds) since the epoch.
 *
 * Set @timestamp to <code class="literal">-1</code> to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_picasaweb_album_set_timestamp (GDataPicasaWebAlbum *self, gint64 timestamp)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_ALBUM (self));
	g_return_if_fail (timestamp >= -1);

	self->priv->timestamp = timestamp;
	g_object_notify (G_OBJECT (self), "timestamp");
}

/**
 * gdata_picasaweb_album_get_num_photos:
 * @self: a #GDataPicasaWebAlbum
 *
 * Gets the #GDataPicasaWebAlbum:num-photos property.
 *
 * Return value: the number of photos currently in the album
 *
 * Since: 0.4.0
 */
guint
gdata_picasaweb_album_get_num_photos (GDataPicasaWebAlbum *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_ALBUM (self), 0);
	return self->priv->num_photos;
}

/**
 * gdata_picasaweb_album_get_num_photos_remaining:
 * @self: a #GDataPicasaWebAlbum
 *
 * Gets the #GDataPicasaWebAlbum:num-photos-remaining property.
 *
 * Return value: the number of photos that can still be uploaded to the album
 *
 * Since: 0.4.0
 */
guint
gdata_picasaweb_album_get_num_photos_remaining (GDataPicasaWebAlbum *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_ALBUM (self), 0);
	return self->priv->num_photos_remaining;
}

/**
 * gdata_picasaweb_album_get_bytes_used:
 * @self: a #GDataPicasaWebAlbum
 *
 * Gets the #GDataPicasaWebAlbum:bytes-used property. It will return <code class="literal">-1</code> if the current authenticated
 * user is not the owner of the album.
 *
 * Return value: the number of bytes used by the album and its contents, or <code class="literal">-1</code>
 *
 * Since: 0.4.0
 */
glong
gdata_picasaweb_album_get_bytes_used (GDataPicasaWebAlbum *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_ALBUM (self), -1);
	return self->priv->bytes_used;
}

/**
 * gdata_picasaweb_album_is_commenting_enabled:
 * @self: a #GDataPicasaWebAlbum
 *
 * Gets the #GDataPicasaWebAlbum:is-commenting-enabled property.
 *
 * Return value: %TRUE if commenting is enabled for the album, %FALSE otherwise
 *
 * Since: 0.4.0
 */
gboolean
gdata_picasaweb_album_is_commenting_enabled (GDataPicasaWebAlbum *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_ALBUM (self), FALSE);
	return self->priv->is_commenting_enabled;
}

/**
 * gdata_picasaweb_album_set_is_commenting_enabled:
 * @self: a #GDataPicasaWebAlbum
 * @is_commenting_enabled: %TRUE if commenting should be enabled for the album, %FALSE otherwise
 *
 * Sets the #GDataPicasaWebAlbum:is-commenting-enabled property to @is_commenting_enabled.
 *
 * Since: 0.4.0
 */
void
gdata_picasaweb_album_set_is_commenting_enabled (GDataPicasaWebAlbum *self, gboolean is_commenting_enabled)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_ALBUM (self));
	self->priv->is_commenting_enabled = is_commenting_enabled;
	g_object_notify (G_OBJECT (self), "is-commenting-enabled");
}

/**
 * gdata_picasaweb_album_get_comment_count:
 * @self: a #GDataPicasaWebAlbum
 *
 * Gets the #GDataPicasaWebAlbum:comment-count property.
 *
 * Return value: the number of comments on the album
 *
 * Since: 0.4.0
 */
guint
gdata_picasaweb_album_get_comment_count (GDataPicasaWebAlbum *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_ALBUM (self), 0);
	return self->priv->comment_count;
}

/**
 * gdata_picasaweb_album_get_tags:
 * @self: a #GDataPicasaWebAlbum
 *
 * Gets the #GDataPicasaWebAlbum:tags property.
 *
 * Return value: (array zero-terminated=1) (transfer none): a %NULL-terminated array of tags associated with all the photos in the album, or %NULL
 *
 * Since: 0.4.0
 */
const gchar * const *
gdata_picasaweb_album_get_tags (GDataPicasaWebAlbum *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_ALBUM (self), NULL);
	return gdata_media_group_get_keywords (self->priv->media_group);
}

/**
 * gdata_picasaweb_album_set_tags:
 * @self: a #GDataPicasaWebAlbum
 * @tags: (array zero-terminated=1) (allow-none): the new %NULL-terminated array of tags, or %NULL
 *
 * Sets the #GDataPicasaWebAlbum:tags property to @tags.
 *
 * Set @tags to %NULL to unset the album's tag list.
 *
 * Since: 0.4.0
 */
void
gdata_picasaweb_album_set_tags (GDataPicasaWebAlbum *self, const gchar * const *tags)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_ALBUM (self));

	gdata_media_group_set_keywords (self->priv->media_group, tags);
	g_object_notify (G_OBJECT (self), "tags");
}

/**
 * gdata_picasaweb_album_get_contents:
 * @self: a #GDataPicasaWebAlbum
 *
 * Returns a list of media content, such as the cover image for the album.
 *
 * Return value: (element-type GData.MediaContent) (transfer none): a #GList of #GDataMediaContent items
 *
 * Since: 0.4.0
 */
GList *
gdata_picasaweb_album_get_contents (GDataPicasaWebAlbum *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_ALBUM (self), NULL);
	return gdata_media_group_get_contents (self->priv->media_group);
}

/**
 * gdata_picasaweb_album_get_thumbnails:
 * @self: a #GDataPicasaWebAlbum
 *
 * Returns a list of thumbnails, often at different sizes, for this album.
 *
 * Return value: (element-type GData.MediaThumbnail) (transfer none): a #GList of #GDataMediaThumbnails, or %NULL
 *
 * Since: 0.4.0
 */
GList *
gdata_picasaweb_album_get_thumbnails (GDataPicasaWebAlbum *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_ALBUM (self), NULL);
	return gdata_media_group_get_thumbnails (self->priv->media_group);
}

/**
 * gdata_picasaweb_album_get_coordinates:
 * @self: a #GDataPicasaWebAlbum
 * @latitude: (out caller-allocates) (allow-none): return location for the latitude, or %NULL
 * @longitude: (out caller-allocates) (allow-none): return location for the longitude, or %NULL
 *
 * Gets the #GDataPicasaWebAlbum:latitude and #GDataPicasaWebAlbum:longitude properties,
 * setting the out parameters to them. If either latitude or longitude is %NULL, that parameter will not be set.
 * If the coordinates are unset, @latitude and @longitude will be set to %G_MAXDOUBLE.
 *
 * Since: 0.5.0
 */
void
gdata_picasaweb_album_get_coordinates (GDataPicasaWebAlbum *self, gdouble *latitude, gdouble *longitude)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_ALBUM (self));

	if (latitude != NULL)
		*latitude = gdata_georss_where_get_latitude (self->priv->georss_where);
	if (longitude != NULL)
		*longitude = gdata_georss_where_get_longitude (self->priv->georss_where);
}

/**
 * gdata_picasaweb_album_set_coordinates:
 * @self: a #GDataPicasaWebAlbum
 * @latitude: the album's new latitude coordinate, or %G_MAXDOUBLE
 * @longitude: the album's new longitude coordinate, or %G_MAXDOUBLE
 *
 * Sets the #GDataPicasaWebAlbum:latitude and #GDataPicasaWebAlbum:longitude properties to
 * @latitude and @longitude respectively.
 *
 * Since: 0.5.0
 */
void
gdata_picasaweb_album_set_coordinates (GDataPicasaWebAlbum *self, gdouble latitude, gdouble longitude)
{
	g_return_if_fail (GDATA_IS_PICASAWEB_ALBUM (self));

	gdata_georss_where_set_latitude (self->priv->georss_where, latitude);
	gdata_georss_where_set_longitude (self->priv->georss_where, longitude);

	g_object_freeze_notify (G_OBJECT (self));
	g_object_notify (G_OBJECT (self), "latitude");
	g_object_notify (G_OBJECT (self), "longitude");
	g_object_thaw_notify (G_OBJECT (self));
}
