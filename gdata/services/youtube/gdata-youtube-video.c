/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2008–2010 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-youtube-video
 * @short_description: GData YouTube video object
 * @stability: Unstable
 * @include: gdata/services/youtube/gdata-youtube-video.h
 *
 * #GDataYouTubeVideo is a subclass of #GDataEntry to represent a single video on YouTube, either when uploading or querying.
 *
 * For more details of YouTube's GData API, see the <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html">
 * online documentation</ulink>.
 *
 * <example>
 * 	<title>Getting Basic Video Data</title>
 * 	<programlisting>
 * 		GDataYouTubeVideo *video;
 * 		const gchar *video_id, *title, *player_uri, *description, *video_uri = NULL;
 * 		GTimeVal updated, published;
 * 		GDataMediaContent *content;
 * 		GList *thumbnails;
 *
 * 		video = gdata_youtube_service_query_single_video (service, NULL, "R-9gzmQHoe0", NULL, NULL);
 *
 * 		video_id = gdata_youtube_video_get_video_id (video); /<!-- -->* e.g. "R-9gzmQHoe0" *<!-- -->/
 * 		title = gdata_entry_get_title (GDATA_ENTRY (video)); /<!-- -->* e.g. "Korpiklaani Vodka (official video 2009)" *<!-- -->/
 * 		player_uri = gdata_youtube_video_get_player_uri (video); /<!-- -->* e.g. "http://www.youtube.com/watch?v=ZTUVgYoeN_b" *<!-- -->/
 * 		description = gdata_youtube_video_get_description (video); /<!-- -->* e.g. "Vodka is the first single from the album..." *<!-- -->/
 * 		gdata_entry_get_published (GDATA_ENTRY (video), &published); /<!-- -->* Date and time the video was originally published *<!-- -->/
 * 		gdata_entry_get_updated (GDATA_ENTRY (video), &updated); /<!-- -->* When the video was most recently updated by the author *<!-- -->/
 *
 * 		/<!-- -->* Retrieve a specific encoding of the video in #GDataMediaContent format *<!-- -->/
 * 		content = gdata_youtube_video_look_up_content (video, "video/3gpp");
 * 		if (content != NULL)
 * 			video_uri = gdata_media_content_get_uri (content); /<!-- -->* the URI for the direct 3GP version of the video *<!-- -->/
 * 		else
 * 			/<!-- -->* Fall back and try a different video encoding? SWF ("application/x-shockwave-flash") is always present. *<!-- -->/
 *
 * 		/<!-- -->* Get a list of #GDataMediaThumbnail<!-- -->s for the video *<!-- -->/
 * 		for (thumbnails = gdata_youtube_video_get_thumbnails (video); thumbnails != NULL; thumbnails = thumbnails->next)
 * 			download_and_do_something_with_thumbnail (gdata_media_thumbnail_get_uri (thumbnail));
 *
 * 		g_object_unref (video);
 * 	</programlisting>
 * </example>
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-youtube-video.h"
#include "gdata-private.h"
#include "gdata-service.h"
#include "gdata-parser.h"
#include "media/gdata-media-category.h"
#include "media/gdata-media-thumbnail.h"
#include "gdata-youtube-group.h"
#include "gdata-types.h"
#include "gdata-youtube-control.h"
#include "gdata-youtube-enums.h"

static void gdata_youtube_video_dispose (GObject *object);
static void gdata_youtube_video_finalize (GObject *object);
static void gdata_youtube_video_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_youtube_video_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static gboolean post_parse_xml (GDataParsable *parsable, gpointer user_data, GError **error);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataYouTubeVideoPrivate {
	guint view_count;
	guint favorite_count;
	gchar *location;
	GHashTable *access_controls;

	/* gd:rating attributes */
	struct {
		guint min;
		guint max;
		guint count;
		gdouble average;
	} rating;

	/* media:group */
	GDataMediaGroup *media_group; /* is actually a GDataYouTubeGroup */

	/* Other properties */
	GDataYouTubeControl *youtube_control;
	GTimeVal recorded;
};

enum {
	PROP_VIEW_COUNT = 1,
	PROP_FAVORITE_COUNT,
	PROP_LOCATION,
	PROP_MIN_RATING,
	PROP_MAX_RATING,
	PROP_RATING_COUNT,
	PROP_AVERAGE_RATING,
	PROP_KEYWORDS,
	PROP_PLAYER_URI,
	PROP_CATEGORY,
	PROP_CREDIT,
	PROP_DESCRIPTION,
	PROP_DURATION,
	PROP_IS_PRIVATE,
	PROP_UPLOADED,
	PROP_VIDEO_ID,
	PROP_IS_DRAFT,
	PROP_STATE,
	PROP_RECORDED,
	PROP_ASPECT_RATIO
};

G_DEFINE_TYPE (GDataYouTubeVideo, gdata_youtube_video, GDATA_TYPE_ENTRY)
#define GDATA_YOUTUBE_VIDEO_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDATA_TYPE_YOUTUBE_VIDEO, GDataYouTubeVideoPrivate))

static void
gdata_youtube_video_class_init (GDataYouTubeVideoClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataYouTubeVideoPrivate));

	gobject_class->get_property = gdata_youtube_video_get_property;
	gobject_class->set_property = gdata_youtube_video_set_property;
	gobject_class->dispose = gdata_youtube_video_dispose;
	gobject_class->finalize = gdata_youtube_video_finalize;

	parsable_class->parse_xml = parse_xml;
	parsable_class->post_parse_xml = post_parse_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;

	/**
	 * GDataYouTubeVideo:view-count:
	 *
	 * The number of times the video has been viewed.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:statistics">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_VIEW_COUNT,
				g_param_spec_uint ("view-count",
					"View count", "The number of times the video has been viewed.",
					0, G_MAXUINT, 0,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:favorite-count:
	 *
	 * The number of YouTube users who have added the video to their list of favorite videos.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:statistics">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_FAVORITE_COUNT,
				g_param_spec_uint ("favorite-count",
					"Favorite count", "The number of YouTube users who have added the video to their list of favorite videos.",
					0, G_MAXUINT, 0,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:location:
	 *
	 * Descriptive text about the location where the video was taken.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:location">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_LOCATION,
				g_param_spec_string ("location",
					"Location", "Descriptive text about the location where the video was taken.",
					NULL,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:min-rating:
	 *
	 * The minimum allowed rating for the video.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdRating">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_MIN_RATING,
				g_param_spec_uint ("min-rating",
					"Minimum rating", "The minimum allowed rating for the video.",
					0, G_MAXUINT, 1, /* defaults to 1 */
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:max-rating:
	 *
	 * The maximum allowed rating for the video.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdRating">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_MAX_RATING,
				g_param_spec_uint ("max-rating",
					"Maximum rating", "The maximum allowed rating for the video.",
					0, G_MAXUINT, 5, /* defaults to 5 */
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:rating-count:
	 *
	 * The number of times the video has been rated.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdRating">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_RATING_COUNT,
				g_param_spec_uint ("rating-count",
					"Rating count", "The number of times the video has been rated.",
					0, G_MAXUINT, 0,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:average-rating:
	 *
	 * The average rating of the video, over all the ratings it's received.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdRating">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_AVERAGE_RATING,
				g_param_spec_double ("average-rating",
					"Average rating", "The average rating of the video, over all the ratings it's received.",
					0.0, G_MAXDOUBLE, 0.0,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:keywords:
	 *
	 * A comma-separated list of words associated with the video.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_media:keywords">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_KEYWORDS,
				g_param_spec_string ("keywords",
					"Keywords", "A comma-separated list of words associated with the video.",
					NULL,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:player-uri:
	 *
	 * Specifies a URI where the full-length video is available through a media player that runs inside a web browser
	 * (i.e. the video's page on YouTube).
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_media:player">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_PLAYER_URI,
				g_param_spec_string ("player-uri",
					"Player URI", "Specifies a URI where the full-length video is available through a media player"
					"that runs inside a web browser.",
					NULL,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:category:
	 *
	 * Specifies a genre or developer tag that describes the video.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_media:category">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_CATEGORY,
				g_param_spec_object ("category",
					"Category", "Specifies a genre or developer tag that describes the video.",
					GDATA_TYPE_MEDIA_CATEGORY,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:credit:
	 *
	 * Identifies the owner of the video.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_media:credit">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_CREDIT,
				g_param_spec_object ("credit",
					"Credit", "Identifies the owner of the video.",
					GDATA_TYPE_YOUTUBE_CREDIT,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:description:
	 *
	 * A summary or description of the video.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_media:description">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_DESCRIPTION,
				g_param_spec_string ("description",
					"Description", "A summary or description of the video.",
					NULL,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:duration:
	 *
	 * The duration of the video in seconds.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:duration">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_DURATION,
				g_param_spec_uint ("duration",
					"Duration", "The duration of the video in seconds.",
					0, G_MAXINT, 0,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:private:
	 *
	 * Indicates whether the video is private, meaning that it will not be publicly visible on YouTube's website.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:private">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_IS_PRIVATE,
				g_param_spec_boolean ("is-private",
					"Private?", "Indicates whether the video is private.",
					FALSE,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:uploaded:
	 *
	 * Specifies the time the video was originally uploaded to YouTube.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:uploaded">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_UPLOADED,
				g_param_spec_boxed ("uploaded",
					"Uploaded", "Specifies the time the video was originally uploaded to YouTube.",
					GDATA_TYPE_G_TIME_VAL,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:video-id:
	 *
	 * Specifies a unique ID which YouTube uses to identify the video. For example: <literal>qz8EfkS4KK0</literal>.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:videoid">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_VIDEO_ID,
				g_param_spec_string ("video-id",
					"Video ID", "Specifies a unique ID which YouTube uses to identify the video.",
					NULL,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:is-draft:
	 *
	 * Indicates whether the video is in draft, or unpublished, status.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_app:draft">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_IS_DRAFT,
				g_param_spec_boolean ("is-draft",
					"Draft?", "Indicates whether the video is in draft, or unpublished, status.",
					FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:state:
	 *
	 * Information describing the state of the video. If this is non-%NULL, the video is not playable.
	 * It points to a #GDataYouTubeState.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:state">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_STATE,
				g_param_spec_object ("state",
					"State", "Information describing the state of the video.",
					GDATA_TYPE_YOUTUBE_STATE,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:recorded:
	 *
	 * Specifies the time the video was originally recorded.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:recorded">online documentation</ulink>.
	 *
	 * Since: 0.3.0
	 **/
	g_object_class_install_property (gobject_class, PROP_RECORDED,
				g_param_spec_boxed ("recorded",
					"Recorded", "Specifies the time the video was originally recorded.",
					GDATA_TYPE_G_TIME_VAL,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:aspect-ratio:
	 *
	 * The aspect ratio of the video.
	 *
	 * For more information see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:aspectratio">online documentation</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_ASPECT_RATIO,
				g_param_spec_enum ("aspect-ratio",
					"Aspect Ratio", "The aspect ratio of the video.",
					GDATA_TYPE_YOUTUBE_ASPECT_RATIO,
					GDATA_YOUTUBE_ASPECT_RATIO_UNKNOWN,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
notify_title_cb (GDataYouTubeVideo *self, GParamSpec *pspec, gpointer user_data)
{
	/* Update our media:group title */
	if (self->priv->media_group != NULL)
		gdata_media_group_set_title (self->priv->media_group, gdata_entry_get_title (GDATA_ENTRY (self)));
}

static void
gdata_youtube_video_init (GDataYouTubeVideo *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_YOUTUBE_VIDEO, GDataYouTubeVideoPrivate);
	self->priv->access_controls = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify) g_free, NULL);

	/* The video's title is duplicated between atom:title and media:group/media:title, so listen for change notifications on atom:title
	 * and propagate them to media:group/media:title accordingly. Since the media group isn't publically accessible, we don't need to
	 * listen for notifications from it. */
	g_signal_connect (GDATA_ENTRY (self), "notify::title", G_CALLBACK (notify_title_cb), NULL);
}

static void
gdata_youtube_video_dispose (GObject *object)
{
	GDataYouTubeVideoPrivate *priv = GDATA_YOUTUBE_VIDEO (object)->priv;

	if (priv->media_group != NULL)
		g_object_unref (priv->media_group);
	priv->media_group = NULL;

	if (priv->youtube_control != NULL)
		g_object_unref (priv->youtube_control);
	priv->youtube_control = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_youtube_video_parent_class)->dispose (object);
}

static void
gdata_youtube_video_finalize (GObject *object)
{
	GDataYouTubeVideoPrivate *priv = GDATA_YOUTUBE_VIDEO (object)->priv;

	g_free (priv->location);
	g_hash_table_destroy (priv->access_controls);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_youtube_video_parent_class)->finalize (object);
}

static void
gdata_youtube_video_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataYouTubeVideoPrivate *priv = GDATA_YOUTUBE_VIDEO (object)->priv;

	switch (property_id) {
		case PROP_VIEW_COUNT:
			g_value_set_uint (value, priv->view_count);
			break;
		case PROP_FAVORITE_COUNT:
			g_value_set_uint (value, priv->favorite_count);
			break;
		case PROP_LOCATION:
			g_value_set_string (value, priv->location);
			break;
		case PROP_MIN_RATING:
			g_value_set_uint (value, priv->rating.min);
			break;
		case PROP_MAX_RATING:
			g_value_set_uint (value, priv->rating.max);
			break;
		case PROP_RATING_COUNT:
			g_value_set_uint (value, priv->rating.count);
			break;
		case PROP_AVERAGE_RATING:
			g_value_set_double (value, priv->rating.average);
			break;
		case PROP_KEYWORDS:
			g_value_set_string (value, gdata_media_group_get_keywords (priv->media_group));
			break;
		case PROP_PLAYER_URI:
			g_value_set_string (value, gdata_media_group_get_player_uri (priv->media_group));
			break;
		case PROP_CATEGORY:
			g_value_set_object (value, gdata_media_group_get_category (priv->media_group));
			break;
		case PROP_CREDIT:
			g_value_set_object (value, gdata_media_group_get_credit (priv->media_group));
			break;
		case PROP_DESCRIPTION:
			g_value_set_string (value, gdata_media_group_get_description (priv->media_group));
			break;
		case PROP_DURATION:
			g_value_set_uint (value, gdata_youtube_group_get_duration (GDATA_YOUTUBE_GROUP (priv->media_group)));
			break;
		case PROP_IS_PRIVATE:
			g_value_set_boolean (value, gdata_youtube_group_is_private (GDATA_YOUTUBE_GROUP (priv->media_group)));
			break;
		case PROP_UPLOADED: {
			GTimeVal uploaded;
			gdata_youtube_group_get_uploaded (GDATA_YOUTUBE_GROUP (priv->media_group), &uploaded);
			g_value_set_boxed (value, &(uploaded));
			break; }
		case PROP_VIDEO_ID:
			g_value_set_string (value, gdata_youtube_group_get_video_id (GDATA_YOUTUBE_GROUP (priv->media_group)));
			break;
		case PROP_IS_DRAFT:
			g_value_set_boolean (value, gdata_youtube_control_is_draft (priv->youtube_control));
			break;
		case PROP_STATE:
			g_value_set_object (value, gdata_youtube_control_get_state (priv->youtube_control));
			break;
		case PROP_RECORDED:
			g_value_set_boxed (value, &(priv->recorded));
			break;
		case PROP_ASPECT_RATIO:
			g_value_set_enum (value, gdata_youtube_group_get_aspect_ratio (GDATA_YOUTUBE_GROUP (priv->media_group)));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_youtube_video_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataYouTubeVideo *self = GDATA_YOUTUBE_VIDEO (object);

	switch (property_id) {
		case PROP_LOCATION:
			gdata_youtube_video_set_location (self, g_value_get_string (value));
			break;
		case PROP_KEYWORDS:
			gdata_youtube_video_set_keywords (self, g_value_get_string (value));
			break;
		case PROP_CATEGORY:
			gdata_youtube_video_set_category (self, g_value_get_object (value));
			break;
		case PROP_DESCRIPTION:
			gdata_youtube_video_set_description (self, g_value_get_string (value));
			break;
		case PROP_IS_PRIVATE:
			gdata_youtube_video_set_is_private (self, g_value_get_boolean (value));
			break;
		case PROP_IS_DRAFT:
			gdata_youtube_video_set_is_draft (self, g_value_get_boolean (value));
			break;
		case PROP_RECORDED:
			gdata_youtube_video_set_recorded (self, g_value_get_boxed (value));
			break;
		case PROP_ASPECT_RATIO:
			gdata_youtube_video_set_aspect_ratio (self, g_value_get_enum (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

/**
 * gdata_youtube_video_new:
 * @id: the video's ID, or %NULL
 *
 * Creates a new #GDataYouTubeVideo with the given ID and default properties.
 *
 * Return value: a new #GDataYouTubeVideo; unref with g_object_unref()
 **/
GDataYouTubeVideo *
gdata_youtube_video_new (const gchar *id)
{
	GDataYouTubeVideo *video = g_object_new (GDATA_TYPE_YOUTUBE_VIDEO, "id", id, NULL);
	/* We can't create these in init, or they would collide with the group and control created when parsing the XML */
	video->priv->media_group = g_object_new (GDATA_TYPE_YOUTUBE_GROUP, NULL);
	video->priv->youtube_control = g_object_new (GDATA_TYPE_YOUTUBE_CONTROL, NULL);
	return video;
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	GDataYouTubeVideo *self = GDATA_YOUTUBE_VIDEO (parsable);

	if (xmlStrcmp (node->name, (xmlChar*) "group") == 0) {
		/* media:group */
		GDataMediaGroup *group = GDATA_MEDIA_GROUP (_gdata_parsable_new_from_xml_node (GDATA_TYPE_YOUTUBE_GROUP, doc, node, NULL, error));
		if (group == NULL)
			return FALSE;

		if (self->priv->media_group != NULL) {
			g_object_unref (group);
			return gdata_parser_error_duplicate_element (node, error);
		}

		self->priv->media_group = group;
	} else if (xmlStrcmp (node->name, (xmlChar*) "rating") == 0) {
		/* gd:rating */
		xmlChar *min, *max, *num_raters, *average;
		guint num_raters_uint;
		gdouble average_double;

		min = xmlGetProp (node, (xmlChar*) "min");
		if (min == NULL)
			return gdata_parser_error_required_property_missing (node, "min", error);

		max = xmlGetProp (node, (xmlChar*) "max");
		if (max == NULL) {
			gdata_parser_error_required_property_missing (node, "max", error);
			xmlFree (min);
			return FALSE;
		}

		num_raters = xmlGetProp (node, (xmlChar*) "numRaters");
		if (num_raters == NULL)
			num_raters_uint = 0;
		else
			num_raters_uint = strtoul ((gchar*) num_raters, NULL, 10);
		xmlFree (num_raters);

		average = xmlGetProp (node, (xmlChar*) "average");
		if (average == NULL)
			average_double = 0;
		else
			average_double = g_ascii_strtod ((gchar*) average, NULL);
		xmlFree (average);

		self->priv->rating.min = strtoul ((gchar*) min, NULL, 10);
		self->priv->rating.max = strtoul ((gchar*) max, NULL, 10);
		self->priv->rating.count = num_raters_uint;
		self->priv->rating.average = average_double;
	} else if (xmlStrcmp (node->name, (xmlChar*) "comments") == 0) {
		/* gd:comments */
		xmlChar *rel, *href, *count_hint, *read_only;
		xmlNode *child_node;
		guint count_hint_uint;

		/* This is actually the child of the <comments> element */
		child_node = node->children;

		count_hint = xmlGetProp (child_node, (xmlChar*) "countHint");
		if (count_hint == NULL)
			count_hint_uint = 0;
		else
			count_hint_uint = strtoul ((gchar*) count_hint, NULL, 10);
		xmlFree (count_hint);

		read_only = xmlGetProp (child_node, (xmlChar*) "readOnly");
		rel = xmlGetProp (child_node, (xmlChar*) "rel");
		href = xmlGetProp (child_node, (xmlChar*) "href");

		/* TODO */
		/*gdata_gd_feed_link_free (self->priv->comments_feed_link);
		self->priv->comments_feed_link = gdata_gd_feed_link_new ((gchar*) href, (gchar*) rel, count_hint_uint,
									 ((xmlStrcmp (read_only, (xmlChar*) "true") == 0) ? TRUE : FALSE));*/

		xmlFree (rel);
		xmlFree (href);
		xmlFree (read_only);
	} else if (xmlStrcmp (node->name, (xmlChar*) "statistics") == 0) {
		/* yt:statistics */
		xmlChar *view_count, *favorite_count;

		/* View count */
		view_count = xmlGetProp (node, (xmlChar*) "viewCount");
		if (view_count == NULL)
			return gdata_parser_error_required_property_missing (node, "viewCount", error);
		self->priv->view_count = strtoul ((gchar*) view_count, NULL, 10);
		xmlFree (view_count);

		/* Favourite count */
		favorite_count = xmlGetProp (node, (xmlChar*) "favoriteCount");
		self->priv->favorite_count = (favorite_count != NULL) ? strtoul ((gchar*) favorite_count, NULL, 10) : 0;
		xmlFree (favorite_count);
	} else if (xmlStrcmp (node->name, (xmlChar*) "location") == 0) {
		/* yt:location */
		g_free (self->priv->location);
		self->priv->location = (gchar*) xmlNodeListGetString (doc, node->children, TRUE);
	} else if (xmlStrcmp (node->name, (xmlChar*) "noembed") == 0) {
		/* yt:noembed */
		/* Ignore this now; it's been superceded by yt:accessControl.
		   See http://apiblog.youtube.com/2010/02/extended-access-controls-available-via.html */
	} else if (xmlStrcmp (node->name, (xmlChar*) "accessControl") == 0) {
		/* yt:accessControl */
		xmlChar *action, *permission;
		GDataYouTubePermission permission_enum;

		action = xmlGetProp (node, (xmlChar*) "action");
		if (action == NULL)
			return gdata_parser_error_required_property_missing (node, "action", error);
		permission = xmlGetProp (node, (xmlChar*) "permission");
		if (permission == NULL) {
			xmlFree (action);
			return gdata_parser_error_required_property_missing (node, "permission", error);
		}

		/* Work out what the permission is */
		if (xmlStrcmp (permission, (xmlChar*) "allowed") == 0) {
			permission_enum = GDATA_YOUTUBE_PERMISSION_ALLOWED;
		} else if (xmlStrcmp (permission, (xmlChar*) "denied") == 0) {
			permission_enum = GDATA_YOUTUBE_PERMISSION_DENIED;
		} else if (xmlStrcmp (permission, (xmlChar*) "moderated") == 0) {
			permission_enum = GDATA_YOUTUBE_PERMISSION_MODERATED;
		} else {
			xmlFree (action);
			xmlFree (permission);
			return gdata_parser_error_unknown_property_value (node, "permission", (gchar*) permission, error);
		}

		/* Store the access control */
		g_hash_table_insert (self->priv->access_controls, (gchar*) action, GINT_TO_POINTER (permission_enum));
	} else if (xmlStrcmp (node->name, (xmlChar*) "recorded") == 0) {
		/* yt:recorded */
		xmlChar *recorded;
		GTimeVal recorded_timeval;

		recorded = xmlNodeListGetString (doc, node->children, TRUE);
		if (gdata_parser_time_val_from_date ((gchar*) recorded, &recorded_timeval) == FALSE) {
			/* Error */
			gdata_parser_error_not_iso8601_format (node, (gchar*) recorded, error);
			xmlFree (recorded);
			return FALSE;
		}
		xmlFree (recorded);
		gdata_youtube_video_set_recorded (self, &recorded_timeval);
	} else if (xmlStrcmp (node->name, (xmlChar*) "control") == 0) {
		/* app:control */
		GDataYouTubeControl *control = GDATA_YOUTUBE_CONTROL (_gdata_parsable_new_from_xml_node (GDATA_TYPE_YOUTUBE_CONTROL, doc,
													 node, NULL, error));
		if (control == NULL)
			return FALSE;

		if (self->priv->youtube_control != NULL) {
			g_object_unref (control);
			return gdata_parser_error_duplicate_element (node, error);
		}

		self->priv->youtube_control = control;
	} else if (GDATA_PARSABLE_CLASS (gdata_youtube_video_parent_class)->parse_xml (parsable, doc, node, user_data, error) == FALSE) {
		/* Error! */
		return FALSE;
	}

	return TRUE;
}

static gboolean
post_parse_xml (GDataParsable *parsable, gpointer user_data, GError **error)
{
	GDataYouTubeVideoPrivate *priv = GDATA_YOUTUBE_VIDEO (parsable)->priv;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_youtube_video_parent_class)->post_parse_xml (parsable, user_data, error);

	/* This must always exist, so is_draft can be set on it */
	if (priv->youtube_control == NULL)
		priv->youtube_control = g_object_new (GDATA_TYPE_YOUTUBE_CONTROL, NULL);

	return TRUE;
}

static void
access_control_cb (const gchar *action, gpointer value, GString *xml_string)
{
	GDataYouTubePermission permission = GPOINTER_TO_INT (value);
	const gchar *permission_string;

	switch (permission) {
		case GDATA_YOUTUBE_PERMISSION_ALLOWED:
			permission_string = "allowed";
			break;
		case GDATA_YOUTUBE_PERMISSION_DENIED:
			permission_string = "denied";
			break;
		case GDATA_YOUTUBE_PERMISSION_MODERATED:
			permission_string = "moderated";
			break;
		default:
			g_assert_not_reached ();
	}

	g_string_append_printf (xml_string, "<yt:accessControl action='%s' permission='%s'/>", action, permission_string);
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataYouTubeVideoPrivate *priv = GDATA_YOUTUBE_VIDEO (parsable)->priv;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_youtube_video_parent_class)->get_xml (parsable, xml_string);

	/* media:group */
	_gdata_parsable_get_xml (GDATA_PARSABLE (priv->media_group), xml_string, FALSE);

	if (priv->location != NULL)
		gdata_parser_string_append_escaped (xml_string, "<yt:location>", priv->location, "</yt:location>");

	if (priv->recorded.tv_sec != 0 || priv->recorded.tv_usec != 0) {
		gchar *recorded = gdata_parser_date_from_time_val (&(priv->recorded));
		g_string_append_printf (xml_string, "<yt:recorded>%s</yt:recorded>", recorded);
		g_free (recorded);
	}

	/* yt:accessControl */
	g_hash_table_foreach (priv->access_controls, (GHFunc) access_control_cb, xml_string);

	/* app:control */
	_gdata_parsable_get_xml (GDATA_PARSABLE (priv->youtube_control), xml_string, FALSE);

	/* TODO:
	 * - georss:where
	 * - Check things are escaped (or not) as appropriate
	 */
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	GDataYouTubeVideoPrivate *priv = GDATA_YOUTUBE_VIDEO (parsable)->priv;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_youtube_video_parent_class)->get_namespaces (parsable, namespaces);

	g_hash_table_insert (namespaces, (gchar*) "yt", (gchar*) "http://gdata.youtube.com/schemas/2007");

	/* Add the media:group and app:control namespaces */
	GDATA_PARSABLE_GET_CLASS (priv->media_group)->get_namespaces (GDATA_PARSABLE (priv->media_group), namespaces);
	GDATA_PARSABLE_GET_CLASS (priv->youtube_control)->get_namespaces (GDATA_PARSABLE (priv->youtube_control), namespaces);
}

/**
 * gdata_youtube_video_get_view_count:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:view-count property.
 *
 * Return value: the number of times the video has been viewed
 **/
guint
gdata_youtube_video_get_view_count (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), 0);
	return self->priv->view_count;
}

/**
 * gdata_youtube_video_get_favorite_count:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:favorite-count property.
 *
 * Return value: the number of users who have added the video to their favorites list
 **/
guint
gdata_youtube_video_get_favorite_count (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), 0);
	return self->priv->favorite_count;
}

/**
 * gdata_youtube_video_get_location:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:location property.
 *
 * Return value: a string describing the video's location, or %NULL
 **/
const gchar *
gdata_youtube_video_get_location (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);
	return self->priv->location;
}

/**
 * gdata_youtube_video_set_location:
 * @self: a #GDataYouTubeVideo
 * @location: a new location, or %NULL
 *
 * Sets the #GDataYouTubeVideo:location property to the new location string, @location.
 *
 * Set @location to %NULL to unset the property in the video.
 **/
void
gdata_youtube_video_set_location (GDataYouTubeVideo *self, const gchar *location)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));

	g_free (self->priv->location);
	self->priv->location = g_strdup (location);
	g_object_notify (G_OBJECT (self), "location");
}

/* Convert a GDataYouTubeAction into its string representation.
 * See: http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:accessControl */
static const gchar *
action_to_string (GDataYouTubeAction action)
{
	switch (action) {
		case GDATA_YOUTUBE_ACTION_RATE:
			return "rate";
		case GDATA_YOUTUBE_ACTION_COMMENT:
			return "comment";
		case GDATA_YOUTUBE_ACTION_COMMENT_VOTE:
			return "commentVote";
		case GDATA_YOUTUBE_ACTION_VIDEO_RESPOND:
			return "videoRespond";
		case GDATA_YOUTUBE_ACTION_EMBED:
			return "embed";
		case GDATA_YOUTUBE_ACTION_SYNDICATE:
			return "syndicate";
		default:
			g_assert_not_reached ();
	}
}

/**
 * gdata_youtube_video_get_access_control:
 * @self: a #GDataYouTubeVideo
 * @action: the action whose permission should be returned
 *
 * Gets the permission associated with the given @action on the #GDataYouTubeVideo. If the given @action
 * doesn't have a permission set on the video, %GDATA_YOUTUBE_PERMISSION_DENIED is returned.
 *
 * Return value: the permission associated with @action, or %GDATA_YOUTUBE_PERMISSION_DENIED
 *
 * Since: 0.7.0
 **/
GDataYouTubePermission
gdata_youtube_video_get_access_control (GDataYouTubeVideo *self, GDataYouTubeAction action)
{
	gpointer value;

	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), GDATA_YOUTUBE_PERMISSION_DENIED);

	if (g_hash_table_lookup_extended (self->priv->access_controls, action_to_string (action), NULL, &value) == FALSE)
		return GDATA_YOUTUBE_PERMISSION_DENIED;
	return GPOINTER_TO_INT (value);
}

/**
 * gdata_youtube_video_set_access_control:
 * @self: a #GDataYouTubeVideo
 * @action: the action whose permission is being set
 * @permission: the permission to give to the action
 *
 * Sets the permission associated with @action on the #GDataYouTubeVideo, allowing restriction or derestriction of various
 * operations on YouTube videos.
 *
 * Note that only %GDATA_YOUTUBE_ACTION_RATE and %GDATA_YOUTUBE_ACTION_COMMENT actions can have the %GDATA_YOUTUBE_PERMISSION_MODERATED permission.
 *
 * Since: 0.7.0
 **/
void
gdata_youtube_video_set_access_control (GDataYouTubeVideo *self, GDataYouTubeAction action, GDataYouTubePermission permission)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));
	g_return_if_fail (action == GDATA_YOUTUBE_ACTION_RATE || action == GDATA_YOUTUBE_ACTION_COMMENT ||
	                  permission != GDATA_YOUTUBE_PERMISSION_MODERATED);

	g_hash_table_replace (self->priv->access_controls, g_strdup (action_to_string (action)), GINT_TO_POINTER (permission));
}

/**
 * gdata_youtube_video_get_rating:
 * @self: a #GDataYouTubeVideo
 * @min: return location for the minimum rating value, or %NULL
 * @max: return location for the maximum rating value, or %NULL
 * @count: return location for the number of ratings, or %NULL
 * @average: return location for the average rating value, or %NULL
 *
 * Gets various properties of the ratings on the video.
 **/
void
gdata_youtube_video_get_rating (GDataYouTubeVideo *self, guint *min, guint *max, guint *count, gdouble *average)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));
	if (min != NULL)
		*min = self->priv->rating.min;
	if (max != NULL)
		*max = self->priv->rating.max;
	if (count != NULL)
		*count = self->priv->rating.count;
	if (average != NULL)
		*average = self->priv->rating.average;
}

/**
 * gdata_youtube_video_get_keywords:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:keywords property.
 *
 * Return value: a comma-separated list of words associated with the video
 **/
const gchar *
gdata_youtube_video_get_keywords (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);
	return gdata_media_group_get_keywords (self->priv->media_group);
}

/**
 * gdata_youtube_video_set_keywords:
 * @self: a #GDataYouTubeVideo
 * @keywords: a new comma-separated list of keywords
 *
 * Sets the #GDataYouTubeVideo:keywords property to the new keyword list, @keywords.
 *
 * @keywords must not be %NULL. For more information, see the <ulink type="http"
 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_media:keywords">online documentation</ulink>.
 **/
void
gdata_youtube_video_set_keywords (GDataYouTubeVideo *self, const gchar *keywords)
{
	g_return_if_fail (keywords != NULL);
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));

	gdata_media_group_set_keywords (self->priv->media_group, keywords);
	g_object_notify (G_OBJECT (self), "keywords");
}

/**
 * gdata_youtube_video_get_player_uri:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:player-uri property.
 *
 * Return value: a URI where the video is playable in a web browser, or %NULL
 **/
const gchar *
gdata_youtube_video_get_player_uri (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);
	return gdata_media_group_get_player_uri (self->priv->media_group);
}

/**
 * gdata_youtube_video_is_restricted_in_country:
 * @self: a #GDataYouTubeVideo
 * @country: an ISO 3166 two-letter country code to check
 *
 * Checks whether viewing of the video is restricted in @country, either by its content rating, or by the request of the producer.
 * The return value from this function is purely informational, and no obligation is assumed.
 *
 * Return value: %TRUE if the video is restricted in @country, %FALSE otherwise
 **/
gboolean
gdata_youtube_video_is_restricted_in_country (GDataYouTubeVideo *self, const gchar *country)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), FALSE);
	g_return_val_if_fail (country != NULL && *country != '\0', FALSE);

	return gdata_media_group_is_restricted_in_country (self->priv->media_group, country);
}

/**
 * gdata_youtube_video_get_category:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:category property.
 *
 * Return value: a #GDataMediaCategory giving the video's single and mandatory category
 **/
GDataMediaCategory *
gdata_youtube_video_get_category (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);
	return gdata_media_group_get_category (self->priv->media_group);
}

/**
 * gdata_youtube_video_set_category:
 * @self: a #GDataYouTubeVideo
 * @category: a new #GDataMediaCategory
 *
 * Sets the #GDataYouTubeVideo:category property to the new category, @category, and increments its reference count.
 *
 * @category must not be %NULL. For more information, see the <ulink type="http"
 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_media:category">online documentation</ulink>.
 **/
void
gdata_youtube_video_set_category (GDataYouTubeVideo *self, GDataMediaCategory *category)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));
	g_return_if_fail (GDATA_IS_MEDIA_CATEGORY (category));

	gdata_media_group_set_category (self->priv->media_group, category);
	g_object_notify (G_OBJECT (self), "category");
}

/**
 * gdata_youtube_video_get_credit:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:credit property.
 *
 * Return value: a #GDataMediaCredit giving information on whom to credit for the video, or %NULL
 **/
GDataYouTubeCredit *
gdata_youtube_video_get_credit (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);
	return GDATA_YOUTUBE_CREDIT (gdata_media_group_get_credit (self->priv->media_group));
}

/**
 * gdata_youtube_video_get_description:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:description property.
 *
 * Return value: the video's long text description, or %NULL
 **/
const gchar *
gdata_youtube_video_get_description (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);
	return gdata_media_group_get_description (self->priv->media_group);
}

/**
 * gdata_youtube_video_set_description:
 * @self: a #GDataYouTubeVideo
 * @description: the video's new description
 *
 * Sets the #GDataYouTubeVideo:description property to the new description, @description.
 *
 * Set @description to %NULL to unset the video's description.
 **/
void
gdata_youtube_video_set_description (GDataYouTubeVideo *self, const gchar *description)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));

	gdata_media_group_set_description (self->priv->media_group, description);
	g_object_notify (G_OBJECT (self), "keywords");
}

/**
 * gdata_youtube_video_look_up_content:
 * @self: a #GDataYouTubeVideo
 * @type: the MIME type of the content desired
 *
 * Looks up a #GDataYouTubeContent from the video with the given MIME type. The video's list of contents is
 * a list of URIs to various formats of the video itself, such as its SWF URI or RTSP stream.
 *
 * Return value: a #GDataYouTubeContent matching @type, or %NULL
 **/
GDataYouTubeContent *
gdata_youtube_video_look_up_content (GDataYouTubeVideo *self, const gchar *type)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);
	g_return_val_if_fail (type != NULL, NULL);

	return GDATA_YOUTUBE_CONTENT (gdata_media_group_look_up_content (self->priv->media_group, type));
}

/**
 * gdata_youtube_video_get_thumbnails:
 * @self: a #GDataYouTubeVideo
 *
 * Gets a list of the thumbnails available for the video.
 *
 * Return value: a #GList of #GDataMediaThumbnail<!-- -->s, or %NULL
 **/
GList *
gdata_youtube_video_get_thumbnails (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);
	return gdata_media_group_get_thumbnails (self->priv->media_group);
}

/**
 * gdata_youtube_video_get_duration:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:duration property.
 *
 * Return value: the video duration in seconds, or %0 if unknown
 **/
guint
gdata_youtube_video_get_duration (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), 0);
	return gdata_youtube_group_get_duration (GDATA_YOUTUBE_GROUP (self->priv->media_group));
}

/**
 * gdata_youtube_video_is_private:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:is-private property.
 *
 * Return value: %TRUE if the video is private, %FALSE otherwise
 **/
gboolean
gdata_youtube_video_is_private (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), FALSE);
	return gdata_youtube_group_is_private (GDATA_YOUTUBE_GROUP (self->priv->media_group));
}

/**
 * gdata_youtube_video_set_is_private:
 * @self: a #GDataYouTubeVideo
 * @is_private: whether the video is private
 *
 * Sets the #GDataYouTubeVideo:is-private property to decide whether the video is publicly viewable.
 **/
void
gdata_youtube_video_set_is_private (GDataYouTubeVideo *self, gboolean is_private)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));
	gdata_youtube_group_set_is_private (GDATA_YOUTUBE_GROUP (self->priv->media_group), is_private);
	g_object_notify (G_OBJECT (self), "is-private");
}

/**
 * gdata_youtube_video_get_uploaded:
 * @self: a #GDataYouTubeVideo
 * @uploaded: a #GTimeVal
 *
 * Gets the #GDataYouTubeVideo:uploaded property and puts it in @uploaded. If the property is unset,
 * both fields in the #GTimeVal will be set to %0.
 **/
void
gdata_youtube_video_get_uploaded (GDataYouTubeVideo *self, GTimeVal *uploaded)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));
	gdata_youtube_group_get_uploaded (GDATA_YOUTUBE_GROUP (self->priv->media_group), uploaded);
}

/**
 * gdata_youtube_video_get_video_id:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:video-id property.
 *
 * Return value: the video's unique and permanent ID
 **/
const gchar *
gdata_youtube_video_get_video_id (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);
	return gdata_youtube_group_get_video_id (GDATA_YOUTUBE_GROUP (self->priv->media_group));
}

/**
 * gdata_youtube_video_is_draft:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:is-draft property.
 *
 * Return value: %TRUE if the video is a draft, %FALSE otherwise
 **/
gboolean
gdata_youtube_video_is_draft (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), FALSE);
	return gdata_youtube_control_is_draft (self->priv->youtube_control);
}

/**
 * gdata_youtube_video_set_is_draft:
 * @self: a #GDataYouTubeVideo
 * @is_draft: whether the video is a draft
 *
 * Sets the #GDataYouTubeVideo:is-draft property to decide whether the video is a draft.
 **/
void
gdata_youtube_video_set_is_draft (GDataYouTubeVideo *self, gboolean is_draft)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));
	gdata_youtube_control_set_is_draft (self->priv->youtube_control, is_draft);
	g_object_notify (G_OBJECT (self), "is-draft");
}

/**
 * gdata_youtube_video_get_state:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:state property.
 *
 * For more information, see the <ulink type="http"
 * url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:state">online documentation</ulink>.
 *
 * Return value: a #GDataYouTubeState showing the state of the video, or %NULL
 **/
GDataYouTubeState *
gdata_youtube_video_get_state (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);
	return gdata_youtube_control_get_state (self->priv->youtube_control);
}

/**
 * gdata_youtube_video_get_recorded:
 * @self: a #GDataYouTubeVideo
 * @recorded: a #GTimeVal
 *
 * Gets the #GDataYouTubeVideo:recorded property and puts it in @recorded. If the property is unset,
 * both fields in the #GTimeVal will be set to %0.
 *
 * Since: 0.3.0
 **/
void
gdata_youtube_video_get_recorded (GDataYouTubeVideo *self, GTimeVal *recorded)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));
	*recorded = self->priv->recorded;
}

/**
 * gdata_youtube_video_set_recorded:
 * @self: a #GDataYouTubeVideo
 * @recorded: the video's new recorded time
 *
 * Sets the #GDataYouTubeVideo:recorded property to the new recorded time, @recorded.
 *
 * Set @recorded to %NULL to unset the video's recorded time.
 *
 * Since: 0.3.0
 **/
void
gdata_youtube_video_set_recorded (GDataYouTubeVideo *self, GTimeVal *recorded)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));
	if (recorded == NULL) {
		self->priv->recorded.tv_sec = 0;
		self->priv->recorded.tv_usec = 0;
	} else {
		self->priv->recorded = *recorded;
	}
}

/**
 * gdata_youtube_video_get_video_id_from_uri:
 * @video_uri: a YouTube video player URI
 *
 * Extracts a video ID from a YouTube video player URI. The video ID is in the same form as returned by
 * gdata_youtube_video_get_video_id(), and the @video_uri should be in the same form as returned by
 * gdata_youtube_video_get_player_uri().
 *
 * The function will validate whether the URI actually points to a hostname containing <literal>youtube</literal>
 * (e.g. <literal>youtube.com</literal>), and will return %NULL if it doesn't.
 *
 * For example:
 * <informalexample><programlisting>
 * video_id = gdata_youtube_video_get_video_id_from_uri ("http://www.youtube.com/watch?v=BH_vwsyCrTc&feature=featured");
 * g_message ("Video ID: %s", video_id); /<!-- -->* Should print: BH_vwsyCrTc *<!-- -->/
 * g_free (video_id);
 * </programlisting></informalexample>
 *
 * Since: 0.4.0
 *
 * Return value: the video ID, or %NULL; free with g_free()
 **/
gchar *
gdata_youtube_video_get_video_id_from_uri (const gchar *video_uri)
{
	gchar *video_id = NULL;
	SoupURI *uri;

	g_return_val_if_fail (video_uri != NULL && *video_uri != '\0', NULL);

	/* Extract the query string from the URI */
	uri = soup_uri_new (video_uri);
	if (uri == NULL)
		return NULL;
	else if (uri->host == NULL || strstr (uri->host, "youtube") == NULL) {
		soup_uri_free (uri);
		return NULL;
	}

	/* Try the "v" parameter (e.g. format is: http://www.youtube.com/watch?v=ylLzyHk54Z0) */
	if (uri->query != NULL) {
		GHashTable *params = soup_form_decode (uri->query);
		video_id = g_strdup (g_hash_table_lookup (params, "v"));
		g_hash_table_destroy (params);
	}

	/* Try the "v" fragment component (e.g. format is: http://www.youtube.com/watch#!v=ylLzyHk54Z0).
	 * YouTube introduced this new URI format in March 2010:
	 * http://apiblog.youtube.com/2010/03/upcoming-change-to-youtube-video-page.html */
	if (video_id == NULL && uri->fragment != NULL) {
		gchar **components, **i;

		components = g_strsplit (uri->fragment, "!", -1);
		for (i = components; *i != NULL; i++) {
			if (**i == 'v' && *((*i) + 1) == '=') {
				video_id = g_strdup ((*i) + 2);
				break;
			}
		}
		g_strfreev (components);
	}

	soup_uri_free (uri);

	return video_id;
}

/**
 * gdata_youtube_video_get_aspect_ratio:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:aspect-ratio property.
 *
 * Return value: the aspect ratio property
 *
 * Since: 0.4.0
 **/
GDataYouTubeAspectRatio
gdata_youtube_video_get_aspect_ratio (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), FALSE);
	return gdata_youtube_group_get_aspect_ratio (GDATA_YOUTUBE_GROUP (self->priv->media_group));
}

/**
 * gdata_youtube_video_set_aspect_ratio:
 * @self: a #GDataYouTubeVideo
 * @aspect_ratio: the aspect ratio property
 *
 * Sets the #GDataYouTubeVideo:aspect-ratio property to specify the video's aspect ratio.
 *
 * Since: 0.4.0
 **/
void
gdata_youtube_video_set_aspect_ratio (GDataYouTubeVideo *self, GDataYouTubeAspectRatio aspect_ratio)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));
	gdata_youtube_group_set_aspect_ratio (GDATA_YOUTUBE_GROUP (self->priv->media_group), aspect_ratio);
	g_object_notify (G_OBJECT (self), "aspect-ratio");
}
