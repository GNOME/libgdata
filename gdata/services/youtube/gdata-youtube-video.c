/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2008–2010, 2015 <philip@tecnocode.co.uk>
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
 * @stability: Stable
 * @include: gdata/services/youtube/gdata-youtube-video.h
 *
 * #GDataYouTubeVideo is a subclass of #GDataEntry to represent a single video on YouTube, either when uploading or querying.
 *
 * #GDataYouTubeVideo implements #GDataCommentable, allowing comments on videos
 * to be queried and added.
 *
 * For more details of YouTube’s GData API, see the
 * <ulink type="http" url="https://developers.google.com/youtube/v3/docs/">
 * online documentation</ulink>.
 *
 * <example>
 * 	<title>Getting Basic Video Data</title>
 * 	<programlisting>
 * 	GDataYouTubeVideo *video;
 * 	const gchar *video_id, *title, *player_uri, *description, *video_uri = NULL;
 * 	gint64 updated, published;
 * 	GDataMediaContent *content;
 * 	GList *thumbnails;
 *
 * 	video = gdata_youtube_service_query_single_video (service, NULL, "R-9gzmQHoe0", NULL, NULL);
 *
 * 	video_id = gdata_entry_get_id (GDATA_ENTRY (video)); /<!-- -->* e.g. "R-9gzmQHoe0" *<!-- -->/
 * 	title = gdata_entry_get_title (GDATA_ENTRY (video)); /<!-- -->* e.g. "Korpiklaani Vodka (official video 2009)" *<!-- -->/
 * 	player_uri = gdata_youtube_video_get_player_uri (video); /<!-- -->* e.g. "http://www.youtube.com/watch?v=ZTUVgYoeN_b" *<!-- -->/
 * 	description = gdata_youtube_video_get_description (video); /<!-- -->* e.g. "Vodka is the first single from the album..." *<!-- -->/
 * 	published = gdata_entry_get_published (GDATA_ENTRY (video)); /<!-- -->* Date and time the video was originally published *<!-- -->/
 * 	updated = gdata_entry_get_updated (GDATA_ENTRY (video)); /<!-- -->* When the video was most recently updated by the author *<!-- -->/
 *
 * 	/<!-- -->* Get a list of GDataMediaThumbnails for the video *<!-- -->/
 * 	for (thumbnails = gdata_youtube_video_get_thumbnails (video); thumbnails != NULL; thumbnails = thumbnails->next)
 * 		download_and_do_something_with_thumbnail (gdata_media_thumbnail_get_uri (thumbnail));
 *
 * 	g_object_unref (video);
 * 	</programlisting>
 * </example>
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <json-glib/json-glib.h>
#include <string.h>

#include "gdata-youtube-video.h"
#include "gdata-private.h"
#include "gdata-service.h"
#include "gdata-parser.h"
#include "media/gdata-media-category.h"
#include "media/gdata-media-thumbnail.h"
#include "gdata-types.h"
#include "gdata-youtube-enums.h"
#include "gdata-commentable.h"
#include "gdata-youtube-comment.h"
#include "gdata-youtube-service.h"

static void gdata_youtube_video_dispose (GObject *object);
static void gdata_youtube_video_finalize (GObject *object);
static void gdata_youtube_video_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_youtube_video_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);
static gboolean post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error);
static void get_json (GDataParsable *parsable, JsonBuilder *builder);
static const gchar *get_content_type (void);
static gchar *get_entry_uri (const gchar *id) G_GNUC_WARN_UNUSED_RESULT;
static void gdata_youtube_video_commentable_init (GDataCommentableInterface *iface);
static GDataAuthorizationDomain *get_authorization_domain (GDataCommentable *self) G_GNUC_CONST;
static gchar *get_query_comments_uri (GDataCommentable *self) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
static gchar *get_insert_comment_uri (GDataCommentable *self, GDataComment *comment_) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
static gboolean is_comment_deletable (GDataCommentable *self, GDataComment *comment_);

struct _GDataYouTubeVideoPrivate {
	guint view_count;
	guint favorite_count;
	gchar *location;
	GHashTable/*<owned utf8, GDataYouTubePermission>*/ *access_controls;

	/* gd:rating attributes */
	struct {
		guint min;
		guint max;
		guint count;
		gdouble average;
	} rating;

	gchar **keywords;
	gchar *player_uri;
	gchar **region_restriction_allowed;
	gchar **region_restriction_blocked;
	GHashTable *content_ratings;  /* owned string → owned string */
	GList *thumbnails; /* GDataMediaThumbnail */
	GDataMediaCategory *category;
	guint duration;
	gboolean is_private;
	gchar *channel_id;  /* owned */

	/* Location. */
	gdouble latitude;
	gdouble longitude;

	/* Other properties */
	gchar *rejection_reason;
	gchar *processing_status;
	gchar *upload_status;
	gchar *failure_reason;
	GDataYouTubeState *upload_state;  /* owned */

	gint64 recorded;

	/* State for parse_json(). */
	gboolean parsing_in_video_list_response;
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
	PROP_DESCRIPTION,
	PROP_DURATION,
	PROP_IS_PRIVATE,
	PROP_UPLOADED,
	PROP_STATE,
	PROP_RECORDED,
	PROP_ASPECT_RATIO,
	PROP_LATITUDE,
	PROP_LONGITUDE
};

G_DEFINE_TYPE_WITH_CODE (GDataYouTubeVideo, gdata_youtube_video, GDATA_TYPE_ENTRY,
                         G_ADD_PRIVATE (GDataYouTubeVideo)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMMENTABLE, gdata_youtube_video_commentable_init))

static void
gdata_youtube_video_class_init (GDataYouTubeVideoClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	gobject_class->get_property = gdata_youtube_video_get_property;
	gobject_class->set_property = gdata_youtube_video_set_property;
	gobject_class->dispose = gdata_youtube_video_dispose;
	gobject_class->finalize = gdata_youtube_video_finalize;

	parsable_class->parse_json = parse_json;
	parsable_class->post_parse_json = post_parse_json;
	parsable_class->get_json = get_json;
	parsable_class->get_content_type = get_content_type;

	entry_class->get_entry_uri = get_entry_uri;
	entry_class->kind_term = "youtube#video";  /* also: youtube#searchResult */

	/**
	 * GDataYouTubeVideo:view-count:
	 *
	 * The number of times the video has been viewed.
	 *
	 * For more information, see the <ulink type="http"
	 * url="https://developers.google.com/youtube/v3/docs/videos#statistics.viewCount">online documentation</ulink>.
	 */
	g_object_class_install_property (gobject_class, PROP_VIEW_COUNT,
	                                 g_param_spec_uint ("view-count",
	                                                    "View count", "The number of times the video has been viewed.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:favorite-count:
	 *
	 * The number of users who have added the video to their favorites.
	 *
	 * For more information, see the <ulink type="http"
	 * url="https://developers.google.com/youtube/v3/docs/videos#statistics.favoriteCount">online documentation</ulink>.
	 */
	g_object_class_install_property (gobject_class, PROP_FAVORITE_COUNT,
	                                 g_param_spec_uint ("favorite-count",
	                                                    "Favorite count", "The number of users who have added the video to their favorites.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:location:
	 *
	 * Descriptive text about the location where the video was taken.
	 *
	 * For more information, see the <ulink type="http"
	 * url="https://developers.google.com/youtube/v3/docs/videos#recordingDetails.locationDescription">online documentation</ulink>.
	 */
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
	 * url="https://developers.google.com/youtube/v3/docs/videos#statistics.likeCount">online documentation</ulink>.
	 */
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
	 * url="https://developers.google.com/youtube/v3/docs/videos#statistics.likeCount">online documentation</ulink>.
	 */
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
	 * url="https://developers.google.com/youtube/v3/docs/videos#statistics.likeCount">online documentation</ulink>.
	 */
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
	 * url="https://developers.google.com/youtube/v3/docs/videos#statistics.likeCount">online documentation</ulink>.
	 */
	g_object_class_install_property (gobject_class, PROP_AVERAGE_RATING,
	                                 g_param_spec_double ("average-rating",
	                                                      "Average rating", "The average rating of the video.",
	                                                      0.0, G_MAXDOUBLE, 0.0,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:keywords:
	 *
	 * A %NULL-terminated array of words associated with the video.
	 *
	 * For more information, see the <ulink type="http"
	 * url="https://developers.google.com/youtube/v3/docs/videos#snippet.tags[]">online documentation</ulink>.
	 */
	g_object_class_install_property (gobject_class, PROP_KEYWORDS,
	                                 g_param_spec_boxed ("keywords",
	                                                     "Keywords", "A NULL-terminated array of words associated with the video.",
	                                                     G_TYPE_STRV,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:player-uri:
	 *
	 * A URI for a browser-based media player for the full-length video (i.e. the video's page on YouTube).
	 */
	g_object_class_install_property (gobject_class, PROP_PLAYER_URI,
	                                 g_param_spec_string ("player-uri",
	                                                      "Player URI", "A URI for a browser-based media player for the full-length video.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:category:
	 *
	 * Specifies a genre or developer tag that describes the video.
	 *
	 * For more information, see the <ulink type="http"
	 * url="https://developers.google.com/youtube/v3/docs/videos#snippet.categoryId">online documentation</ulink>.
	 */
	g_object_class_install_property (gobject_class, PROP_CATEGORY,
	                                 g_param_spec_object ("category",
	                                                      "Category", "Specifies a genre or developer tag that describes the video.",
	                                                      GDATA_TYPE_MEDIA_CATEGORY,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:description:
	 *
	 * A summary or description of the video.
	 *
	 * For more information, see the <ulink type="http"
	 * url="https://developers.google.com/youtube/v3/docs/videos#snippet.description">online documentation</ulink>.
	 */
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
	 * url="https://developers.google.com/youtube/v3/docs/videos#contentDetails.duration">online documentation</ulink>.
	 */
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
	 * url="https://developers.google.com/youtube/v3/docs/videos#status.privacyStatus">online documentation</ulink>.
	 */
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
	 * url="https://developers.google.com/youtube/v3/docs/videos#snippet.publishedAt">online documentation</ulink>.
	 */
	g_object_class_install_property (gobject_class, PROP_UPLOADED,
	                                 g_param_spec_int64 ("uploaded",
	                                                     "Uploaded", "Specifies the time the video was originally uploaded to YouTube.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:state:
	 *
	 * Information describing the state of the video. If this is non-%NULL, the video is not playable.
	 * It points to a #GDataYouTubeState.
	 *
	 * For more information, see the <ulink type="http"
	 * url="https://developers.google.com/youtube/v3/docs/videos#status.uploadStatus">online documentation</ulink>.
	 */
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
	 * url="https://developers.google.com/youtube/v3/docs/videos#recordingDetails.recordingDate">online documentation</ulink>.
	 *
	 * Since: 0.3.0
	 */
	g_object_class_install_property (gobject_class, PROP_RECORDED,
	                                 g_param_spec_int64 ("recorded",
	                                                     "Recorded", "Specifies the time the video was originally recorded.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:aspect-ratio:
	 *
	 * The aspect ratio of the video. A %NULL value means the aspect ratio is unknown (it could still be a widescreen video). A value of
	 * %GDATA_YOUTUBE_ASPECT_RATIO_WIDESCREEN means the video is definitely widescreen.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_ASPECT_RATIO,
	                                 g_param_spec_string ("aspect-ratio",
	                                                      "Aspect Ratio", "The aspect ratio of the video.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:latitude:
	 *
	 * The location as a latitude coordinate associated with this video. Valid latitudes range from <code class="literal">-90.0</code>
	 * to <code class="literal">90.0</code> inclusive. Set to a value
	 * outside this range to unset the location.
	 *
	 * For more information, see the
	 * <ulink type="http" url="https://developers.google.com/youtube/v3/docs/videos#recordingDetails.location.latitude">
	 * online documentation</ulink>.
	 *
	 * Since: 0.8.0
	 */
	g_object_class_install_property (gobject_class, PROP_LATITUDE,
	                                 g_param_spec_double ("latitude",
	                                                      "Latitude", "The location as a latitude coordinate associated with this video.",
	                                                      G_MINDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeVideo:longitude:
	 *
	 * The location as a longitude coordinate associated with this video. Valid longitudes range from <code class="literal">-180.0</code>
	 * to <code class="literal">180.0</code> inclusive. Set to a value
	 * outside this range to unset the location.
	 *
	 * For more information, see the
	 * <ulink type="http" url="https://developers.google.com/youtube/v3/docs/videos#recordingDetails.location.longitude">
	 * online documentation</ulink>.
	 *
	 * Since: 0.8.0
	 */
	g_object_class_install_property (gobject_class, PROP_LONGITUDE,
	                                 g_param_spec_double ("longitude",
	                                                      "Longitude", "The location as a longitude coordinate associated with this video.",
	                                                      G_MINDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_youtube_video_commentable_init (GDataCommentableInterface *iface)
{
	iface->comment_type = GDATA_TYPE_YOUTUBE_COMMENT;
	iface->get_authorization_domain = get_authorization_domain;
	iface->get_query_comments_uri = get_query_comments_uri;
	iface->get_insert_comment_uri = get_insert_comment_uri;
	iface->is_comment_deletable = is_comment_deletable;
}

static void
gdata_youtube_video_init (GDataYouTubeVideo *self)
{
	self->priv = gdata_youtube_video_get_instance_private (self);
	self->priv->recorded = -1;
	self->priv->access_controls = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify) g_free, NULL);
	self->priv->latitude = G_MAXDOUBLE;
	self->priv->longitude = G_MAXDOUBLE;
}

static void
gdata_youtube_video_dispose (GObject *object)
{
	GDataYouTubeVideoPrivate *priv = GDATA_YOUTUBE_VIDEO (object)->priv;

	g_clear_object (&priv->category);
	g_list_free_full (priv->thumbnails, (GDestroyNotify) g_object_unref);
	g_clear_object (&priv->upload_state);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_youtube_video_parent_class)->dispose (object);
}

static void
gdata_youtube_video_finalize (GObject *object)
{
	GDataYouTubeVideoPrivate *priv = GDATA_YOUTUBE_VIDEO (object)->priv;

	g_free (priv->location);
	g_hash_table_destroy (priv->access_controls);
	g_strfreev (priv->keywords);
	g_free (priv->channel_id);
	g_free (priv->player_uri);
	g_strfreev (priv->region_restriction_allowed);
	g_strfreev (priv->region_restriction_blocked);
	g_clear_pointer (&priv->content_ratings, g_hash_table_unref);
	g_free (priv->rejection_reason);
	g_free (priv->processing_status);
	g_free (priv->upload_status);
	g_free (priv->failure_reason);

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
			g_value_set_boxed (value, priv->keywords);
			break;
		case PROP_PLAYER_URI:
			g_value_set_string (value, gdata_youtube_video_get_player_uri (GDATA_YOUTUBE_VIDEO (object)));
			break;
		case PROP_CATEGORY:
			g_value_set_object (value, priv->category);
			break;
		case PROP_DESCRIPTION:
			g_value_set_string (value, gdata_entry_get_summary (GDATA_ENTRY (object)));
			break;
		case PROP_DURATION:
			g_value_set_uint (value, priv->duration);
			break;
		case PROP_IS_PRIVATE:
			g_value_set_boolean (value, priv->is_private);
			break;
		case PROP_UPLOADED:
			g_value_set_int64 (value, gdata_entry_get_published (GDATA_ENTRY (object)));
			break;
		case PROP_STATE:
			g_value_set_object (value, gdata_youtube_video_get_state (GDATA_YOUTUBE_VIDEO (object)));
			break;
		case PROP_RECORDED:
			g_value_set_int64 (value, priv->recorded);
			break;
		case PROP_ASPECT_RATIO:
			g_value_set_string (value, gdata_youtube_video_get_aspect_ratio (GDATA_YOUTUBE_VIDEO (object)));
			break;
		case PROP_LATITUDE:
			g_value_set_double (value, priv->latitude);
			break;
		case PROP_LONGITUDE:
			g_value_set_double (value, priv->longitude);
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
			gdata_youtube_video_set_keywords (self, g_value_get_boxed (value));
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
		case PROP_RECORDED:
			gdata_youtube_video_set_recorded (self, g_value_get_int64 (value));
			break;
		case PROP_ASPECT_RATIO:
			gdata_youtube_video_set_aspect_ratio (self, g_value_get_string (value));
			break;
		case PROP_LATITUDE:
			gdata_youtube_video_set_coordinates (self,
			                                     g_value_get_double (value),
			                                     self->priv->longitude);
			break;
		case PROP_LONGITUDE:
			gdata_youtube_video_set_coordinates (self,
			                                     self->priv->latitude,
			                                     g_value_get_double (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

/* https://developers.google.com/youtube/v3/docs/videos#contentDetails.duration
 *
 * Note that it can also include an ‘hours’ component, as specified in ISO 8601,
 * but not in the Google documentation. */
static gboolean
duration_from_json_member (JsonReader *reader, const gchar *member_name,
                           GDataParserOptions options, guint *output,
                           gboolean *success, GError **error)
{
	gchar *duration_str = NULL, *i = NULL, *new_i = NULL;
	guint64 seconds;
	gboolean child_success = FALSE;

	if (!gdata_parser_string_from_json_member (reader, member_name, options,
	                                           &duration_str,
	                                           &child_success, error)) {
		return FALSE;
	}

	*success = child_success;
	*output = 0;

	if (!child_success) {
		return TRUE;
	}

	/* Parse the string. Format: ‘PT(hH)?(mM)?(sS)?’, where ‘h’, ‘m’ and ‘s’
	 * are integer numbers of hours, minutes and seconds. Each element may
	 * not be present. */
	i = duration_str;
	if (strncmp (duration_str, "PT", 2) != 0) {
		goto error;
	}

	i += 2;  /* PT */

	seconds = 0;

	while (*i != '\0') {
		guint64 element;
		gchar designator;

		element = g_ascii_strtoull (i, &new_i, 10);
		if (new_i == i) {
			goto error;
		}

		i = new_i;

		designator = i[0];
		if (designator == 'H') {
			seconds += 60 * 60 * element;
		} else if (designator == 'M') {
			seconds += 60 * element;
		} else if (designator == 'S') {
			seconds += element;
		} else {
			goto error;
		}

		i += 1;
	}

	*output = seconds;
	*success = child_success;

	g_free (duration_str);

	return TRUE;

error:
	gdata_parser_error_not_iso8601_format_json (reader, duration_str,
	                                            error);
	g_free (duration_str);

	return TRUE;
}

/* https://developers.google.com/youtube/v3/docs/videos#snippet.thumbnails */
static gboolean
thumbnails_from_json_member (JsonReader *reader, const gchar *member_name,
                             GDataParserOptions options, GList **output,
                             gboolean *success, GError **error)
{
	guint i, len;
	GList *thumbnails = NULL;
	const GError *child_error = NULL;

	/* Check if there's such element */
	if (g_strcmp0 (json_reader_get_member_name (reader),
	               member_name) != 0) {
		return FALSE;
	}

	/* Check if the output string has already been set. The JSON parser
	 * guarantees this can't happen. */
	g_assert (!(options & P_NO_DUPES) || *output == NULL);

	len = json_reader_count_members (reader);
	child_error = json_reader_get_error (reader);

	if (child_error != NULL) {
		*success = gdata_parser_error_from_json_error (reader,
		                                               child_error,
		                                               error);
		goto done;
	}

	for (i = 0; i < len; i++) {
		GDataParsable *thumbnail = NULL;  /* GDataMediaThumbnail */

		json_reader_read_element (reader, i);
		thumbnail = _gdata_parsable_new_from_json_node (GDATA_TYPE_MEDIA_THUMBNAIL,
		                                                reader, NULL,
		                                                error);
		json_reader_end_element (reader);

		if (thumbnail == NULL) {
			*success = FALSE;
			goto done;
		}

		thumbnails = g_list_prepend (thumbnails, thumbnail);
	}

	/* Success! */
	*output = thumbnails;
	thumbnails = NULL;
	*success = TRUE;

done:
	g_list_free_full (thumbnails, (GDestroyNotify) g_object_unref);

	return TRUE;
}

/* https://developers.google.com/youtube/v3/docs/videos#contentDetails.regionRestriction */
static gboolean
restricted_countries_from_json_member (JsonReader *reader,
                                       const gchar *member_name,
                                       GDataParserOptions options,
                                       gchar ***output_allowed,
                                       gchar ***output_blocked,
                                       gboolean *success, GError **error)
{
	guint i, len;
	const GError *child_error = NULL;

	/* Check if there's such element */
	if (g_strcmp0 (json_reader_get_member_name (reader),
	               member_name) != 0) {
		return FALSE;
	}

	/* Check if the output string has already been set. The JSON parser guarantees this can't happen. */
	g_assert (!(options & P_NO_DUPES) ||
	          (*output_allowed == NULL && *output_blocked == NULL));

	len = json_reader_count_members (reader);
	child_error = json_reader_get_error (reader);

	if (child_error != NULL) {
		*success = gdata_parser_error_from_json_error (reader,
		                                               child_error,
		                                               error);
		return TRUE;
	}

	for (i = 0; i < len; i++) {
		json_reader_read_element (reader, i);

		if (gdata_parser_strv_from_json_member (reader, "allowed",
		                                        P_DEFAULT,
		                                        output_allowed, success,
		                                        error) ||
		    gdata_parser_strv_from_json_member (reader, "blocked",
		                                        P_DEFAULT,
		                                        output_blocked, success,
		                                        error)) {
			/* Nothing to do. */
		}

		json_reader_end_element (reader);
	}

	/* Success! */
	*success = TRUE;

	return TRUE;
}

/* https://developers.google.com/youtube/v3/docs/videos#contentDetails.contentRating */
static gboolean
content_rating_from_json_member (JsonReader *reader,
                                 const gchar *member_name,
                                 GDataParserOptions options,
                                 GHashTable **output,
                                 gboolean *success, GError **error)
{
	guint i, len;
	const GError *child_error = NULL;

	/* Check if there's such element */
	if (g_strcmp0 (json_reader_get_member_name (reader),
	               member_name) != 0) {
		return FALSE;
	}

	/* Check if the output string has already been set. The JSON parser
	 * guarantees this can't happen. */
	g_assert (!(options & P_NO_DUPES) || *output == NULL);

	len = json_reader_count_members (reader);
	child_error = json_reader_get_error (reader);

	if (child_error != NULL) {
		*success = gdata_parser_error_from_json_error (reader,
		                                               child_error,
		                                               error);
		return TRUE;
	}

	*output = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                 g_free, g_free);

	for (i = 0; i < len; i++) {
		const gchar *scheme, *rating;

		json_reader_read_element (reader, i);

		scheme = json_reader_get_member_name (reader);
		rating = json_reader_get_string_value (reader);

		/* Ignore errors. */
		if (rating != NULL) {
			g_hash_table_insert (*output, g_strdup (scheme),
			                     g_strdup (rating));
		}

		json_reader_end_element (reader);
	}

	/* Success! */
	*success = TRUE;

	return TRUE;
}

static guint64
parse_uint64_from_json_string_member (JsonReader *reader,
                                      const gchar *member_name,
                                      GError **error)
{
	const gchar *str_val, *end_ptr;
	guint64 out;
	const GError *child_error = NULL;

	/* Grab the string. */
	json_reader_read_member (reader, member_name);

	str_val = json_reader_get_string_value (reader);
	child_error = json_reader_get_error (reader);

	if (child_error != NULL) {
		gdata_parser_error_from_json_error (reader, child_error, error);
		out = 0;
		goto done;
	}

	/* Try and parse it as an integer. */
	out = g_ascii_strtoull (str_val, (gchar **) &end_ptr, 10);

	if (*end_ptr != '\0') {
		gdata_parser_error_required_json_content_missing (reader,
		                                                  error);
		out = 0;
		goto done;
	}

done:
	json_reader_end_member (reader);

	return out;
}

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	gboolean success;
	GDataYouTubeVideo *self = GDATA_YOUTUBE_VIDEO (parsable);
	GDataYouTubeVideoPrivate *priv = self->priv;

	/* HACK: When called with gdata_service_query_single_entry(), the video
	 * list endpoint returns a 0–1 item list of results as a normal feed.
	 * (See: https://developers.google.com/youtube/v3/docs/videos/list)
	 * This differs from the v2 API, which returned just the entry.
	 *
	 * So, we need a hack to extract the single entry from the feed without
	 * being able to invoke the parsing machinery in GDataFeed, because
	 * gdata_service_query_single_entry() can’t do that. Do that by checking
	 * the kind, and then ignoring all subsequent members until we reach the
	 * items member. Recursively parse in there, then break out again.
	 * This all assumes that we see the kind member before items. */
	if (g_strcmp0 (json_reader_get_member_name (reader), "kind") == 0 &&
	    g_strcmp0 (json_reader_get_string_value (reader),
	               "youtube#videoListResponse") == 0) {
		priv->parsing_in_video_list_response = TRUE;
		return TRUE;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "items") == 0 &&
	           priv->parsing_in_video_list_response) {
		guint i;

		/* Instead of a 404 when searching for an invalid ID, the server
		 * returns an empty results list. */
		if (json_reader_count_elements (reader) != 1) {
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_NOT_FOUND,
			             /* Translators: the parameter is an error message returned by the server. */
			             _("The requested resource was not found: %s"),
			             "items");
			return TRUE;
		}

		/* Parse the first (and only) array element. */
		json_reader_read_element (reader, 0);
		priv->parsing_in_video_list_response = FALSE;

		/* Parse all its properties. */
		for (i = 0; i < (guint) json_reader_count_members (reader); i++) {
			g_return_val_if_fail (json_reader_read_element (reader, i), FALSE);

			if (GDATA_PARSABLE_GET_CLASS (self)->parse_json (GDATA_PARSABLE (self), reader, user_data, error) == FALSE) {
				json_reader_end_element (reader);
				break;
			}

			json_reader_end_element (reader);
		}

		priv->parsing_in_video_list_response = TRUE;
		json_reader_end_element (reader);

		return TRUE;  /* handled */
	} else if (priv->parsing_in_video_list_response) {
		/* Ignore the member. */
		return TRUE;
	}

	/* Actual video property parsing. */
	if (g_strcmp0 (json_reader_get_member_name (reader), "id") == 0) {
		const gchar *id = NULL;

		/* If this is a youtube#searchResult, the id will be an object:
		 * https://developers.google.com/youtube/v3/docs/search#resource
		 * If it is a youtube#video, the id will be a string:
		 * https://developers.google.com/youtube/v3/docs/videos#resource
		 */

		if (json_reader_is_value (reader)) {
			id = json_reader_get_string_value (reader);
		} else if (json_reader_is_object (reader)) {
			json_reader_read_member (reader, "videoId");
			id = json_reader_get_string_value (reader);
			json_reader_end_member (reader);
		}

		/* Empty ID? */
		if (id == NULL || *id == '\0') {
			return gdata_parser_error_required_json_content_missing (reader, error);
		}

		_gdata_entry_set_id (GDATA_ENTRY (parsable), id);

		return TRUE;
	} else if (g_strcmp0 (json_reader_get_member_name (reader),
	                      "snippet") == 0) {
		guint i;

		/* Check this is an object. */
		if (!json_reader_is_object (reader)) {
			return gdata_parser_error_required_json_content_missing (reader, error);
		}

		for (i = 0; i < (guint) json_reader_count_members (reader); i++) {
			gint64 published_at;
			gchar *title = NULL, *description = NULL;
			gchar *category_id = NULL;

			json_reader_read_element (reader, i);

			if (gdata_parser_int64_time_from_json_member (reader, "publishedAt", P_DEFAULT, &published_at, &success, error)) {
				if (success) {
					_gdata_entry_set_published (GDATA_ENTRY (parsable),
					                            published_at);
				}
			} else if (gdata_parser_string_from_json_member (reader, "title", P_DEFAULT, &title, &success, error)) {
				if (success) {
					gdata_entry_set_title (GDATA_ENTRY (parsable),
					                       title);
				}

				g_free (title);
			} else if (gdata_parser_string_from_json_member (reader, "description", P_DEFAULT, &description, &success, error)) {
				if (success) {
					gdata_entry_set_summary (GDATA_ENTRY (parsable),
					                         description);
				}

				g_free (description);
			} else if (gdata_parser_strv_from_json_member (reader, "tags", P_DEFAULT, &priv->keywords, &success, error) ||
			           thumbnails_from_json_member (reader, "thumbnails", P_DEFAULT, &priv->thumbnails, &success, error) ||
			           gdata_parser_string_from_json_member (reader, "channelId", P_DEFAULT, &priv->channel_id, &success, error)) {
				/* Fall through. */
			} else if (gdata_parser_string_from_json_member (reader, "categoryId", P_DEFAULT, &category_id, &success, error)) {
				if (success) {
					priv->category = gdata_media_category_new (category_id,
					                                           NULL,
					                                           NULL);
				}

				g_free (category_id);
			}

			json_reader_end_element (reader);

			if (!success) {
				return FALSE;
			}
		}

		return TRUE;
	} else if (g_strcmp0 (json_reader_get_member_name (reader),
	                      "contentDetails") == 0) {
		guint i;

		/* Check this is an object. */
		if (!json_reader_is_object (reader)) {
			return gdata_parser_error_required_json_content_missing (reader, error);
		}

		for (i = 0; i < (guint) json_reader_count_members (reader); i++) {
			json_reader_read_element (reader, i);

			if (duration_from_json_member (reader, "duration", P_DEFAULT, &priv->duration, &success, error) ||
			    restricted_countries_from_json_member (reader, "regionRestriction", P_DEFAULT, &priv->region_restriction_allowed, &priv->region_restriction_blocked, &success, error) ||
			    content_rating_from_json_member (reader, "contentRating", P_DEFAULT, &priv->content_ratings, &success, error)) {
				/* Fall through. */
			}

			json_reader_end_element (reader);

			if (!success) {
				return FALSE;
			}
		}

		return TRUE;
	} else if (g_strcmp0 (json_reader_get_member_name (reader),
	                      "status") == 0) {
		const gchar *privacy_status;

		/* Check this is an object. */
		if (!json_reader_is_object (reader)) {
			return gdata_parser_error_required_json_content_missing (reader, error);
		}

		json_reader_read_member (reader, "privacyStatus");
		privacy_status = json_reader_get_string_value (reader);
		json_reader_end_member (reader);

		if (g_strcmp0 (privacy_status, "private") == 0) {
			priv->is_private = TRUE;
		} else if (g_strcmp0 (privacy_status, "public") == 0) {
			priv->is_private = FALSE;
			g_hash_table_insert (priv->access_controls,
			                     g_strdup ("list"),
			                     GINT_TO_POINTER (GDATA_YOUTUBE_PERMISSION_ALLOWED));
		} else if (g_strcmp0 (privacy_status, "unlisted") == 0) {
			/* See: ‘list’ on
			 * https://developers.google.com/youtube/2.0/reference?csw=1#youtube_data_api_tag_yt:accessControl */
			priv->is_private = FALSE;
			g_hash_table_insert (priv->access_controls,
			                     g_strdup ("list"),
			                     GINT_TO_POINTER (GDATA_YOUTUBE_PERMISSION_DENIED));
		}

		json_reader_read_member (reader, "embeddable");
		g_hash_table_insert (priv->access_controls,
		                     g_strdup (GDATA_YOUTUBE_ACTION_EMBED),
		                     GINT_TO_POINTER (json_reader_get_boolean_value (reader) ?
		                                      GDATA_YOUTUBE_PERMISSION_ALLOWED :
		                                      GDATA_YOUTUBE_PERMISSION_DENIED));
		json_reader_end_member (reader);

		json_reader_read_member (reader, "uploadStatus");
		priv->upload_status = g_strdup (json_reader_get_string_value (reader));
		json_reader_end_member (reader);

		json_reader_read_member (reader, "failureReason");
		priv->rejection_reason = g_strdup (json_reader_get_string_value (reader));
		json_reader_end_member (reader);

		json_reader_read_member (reader, "rejectionReason");
		priv->rejection_reason = g_strdup (json_reader_get_string_value (reader));
		json_reader_end_member (reader);

		return TRUE;
	} else if (g_strcmp0 (json_reader_get_member_name (reader),
	                      "statistics") == 0) {
		gint64 likes, dislikes;
		GError *child_error = NULL;

		/* Check this is an object. */
		if (!json_reader_is_object (reader)) {
			return gdata_parser_error_required_json_content_missing (reader, error);
		}

		/* Views and favourites. For some unknown reason, the feed
		 * returns them as a string, even though they’re documented as
		 * being unsigned longs.
		 *
		 * Reference: https://developers.google.com/youtube/v3/docs/videos#statistics */
		priv->view_count = parse_uint64_from_json_string_member (reader, "viewCount", &child_error);
		if (child_error != NULL) {
			g_propagate_error (error, child_error);
			return FALSE;
		}

		priv->favorite_count = parse_uint64_from_json_string_member (reader, "favoriteCount", &child_error);
		if (child_error != NULL) {
			g_propagate_error (error, child_error);
			return FALSE;
		}

		/* The new ratings API (total likes, total dislikes) doesn’t
		 * really match with the old API (collection of integer ratings
		 * between 1 and 5). Try and return something appropriate. */
		likes = parse_uint64_from_json_string_member (reader, "likeCount", &child_error);
		if (child_error != NULL) {
			g_propagate_error (error, child_error);
			return FALSE;
		}

		dislikes = parse_uint64_from_json_string_member (reader, "dislikeCount", &child_error);
		if (child_error != NULL) {
			g_propagate_error (error, child_error);
			return FALSE;
		}

		priv->rating.min = 0;
		priv->rating.max = 1;
		priv->rating.count = likes + dislikes;
		if (likes + dislikes == 0) {
			priv->rating.average = 0.0;  /* basically undefined */
		} else {
			priv->rating.average = (gdouble) likes / (gdouble) (likes + dislikes);
		}

		return TRUE;
	} else if (g_strcmp0 (json_reader_get_member_name (reader),
	                      "processingDetails") == 0) {
		/* Check this is an object. */
		if (!json_reader_is_object (reader)) {
			return gdata_parser_error_required_json_content_missing (reader, error);
		}

		json_reader_read_member (reader, "processingStatus");
		priv->processing_status = g_strdup (json_reader_get_string_value (reader));
		json_reader_end_member (reader);

		return TRUE;
	} else if (g_strcmp0 (json_reader_get_member_name (reader),
	                      "recordingDetails") == 0) {
		const gchar *recording_date;
		const GError *child_error = NULL;

		/* Check this is an object. */
		if (!json_reader_is_object (reader)) {
			return gdata_parser_error_required_json_content_missing (reader, error);
		}

		json_reader_read_member (reader, "recordingDate");
		recording_date = json_reader_get_string_value (reader);
		json_reader_end_member (reader);

		if (recording_date != NULL &&
		    !gdata_parser_int64_from_date (recording_date,
		                                   &priv->recorded)) {
			/* Error */
			gdata_parser_error_not_iso8601_format_json (reader,
			                                            recording_date,
			                                            error);
			return FALSE;
		}

		json_reader_read_member (reader, "locationDescription");
		priv->location = g_strdup (json_reader_get_string_value (reader));
		json_reader_end_member (reader);

		if (json_reader_read_member (reader, "location")) {
			json_reader_read_member (reader, "latitude");
			priv->latitude = json_reader_get_double_value (reader);
			json_reader_end_member (reader);

			json_reader_read_member (reader, "longitude");
			priv->longitude = json_reader_get_double_value (reader);
			json_reader_end_member (reader);
		}

		json_reader_end_member (reader);

		child_error = json_reader_get_error (reader);
		if (child_error != NULL) {
			return gdata_parser_error_from_json_error (reader,
			                                           child_error,
			                                           error);
		}

		return TRUE;
	} else {
		return GDATA_PARSABLE_CLASS (gdata_youtube_video_parent_class)->parse_json (parsable, reader, user_data, error);
	}

	return TRUE;
}

static gboolean
post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error)
{
	GDataLink *_link = NULL;  /* owned */
	const gchar *id;
	gchar *uri = NULL;  /* owned */

	/* Set the self link, which is needed for
	 * gdata_service_delete_entry(). */
	id = gdata_entry_get_id (GDATA_ENTRY (parsable));

	if (id == NULL) {
		return TRUE;
	}

	uri = _gdata_service_build_uri ("https://www.googleapis.com"
	                                "/youtube/v3/videos"
	                                "?id=%s", id);
	_link = gdata_link_new (uri, GDATA_LINK_SELF);
	gdata_entry_add_link (GDATA_ENTRY (parsable), _link);
	g_object_unref (_link);
	g_free (uri);

	return TRUE;
}

static void
get_json (GDataParsable *parsable, JsonBuilder *builder)
{
	GDataEntry *entry = GDATA_ENTRY (parsable);
	GDataYouTubeVideoPrivate *priv = GDATA_YOUTUBE_VIDEO (parsable)->priv;
	guint i;
	gpointer permission_ptr;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_youtube_video_parent_class)->get_json (parsable, builder);

	/* Add the video-specific JSON.
	 * Reference:
	 * https://developers.google.com/youtube/v3/docs/videos/insert#request_body */
	/* snippet object. */
	json_builder_set_member_name (builder, "snippet");
	json_builder_begin_object (builder);

	if (gdata_entry_get_title (entry) != NULL) {
		json_builder_set_member_name (builder, "title");
		json_builder_add_string_value (builder,
		                               gdata_entry_get_title (entry));
	}

	if (gdata_entry_get_summary (entry) != NULL) {
		json_builder_set_member_name (builder, "description");
		json_builder_add_string_value (builder,
		                               gdata_entry_get_summary (entry));
	}

	if (priv->keywords != NULL) {
		json_builder_set_member_name (builder, "tags");
		json_builder_begin_array (builder);

		for (i = 0; priv->keywords[i] != NULL; i++) {
			json_builder_add_string_value (builder,
			                               priv->keywords[i]);
		}

		json_builder_end_array (builder);
	}

	if (priv->category != NULL) {
		json_builder_set_member_name (builder, "categoryId");
		json_builder_add_string_value (builder,
		                               gdata_media_category_get_category (priv->category));
	}

	json_builder_end_object (builder);

	/* status object. */
	json_builder_set_member_name (builder, "status");
	json_builder_begin_object (builder);

	json_builder_set_member_name (builder, "privacyStatus");

	if (!priv->is_private &&
	    g_hash_table_lookup_extended (priv->access_controls,
	                                  "list", NULL,
	                                  &permission_ptr)) {
		GDataYouTubePermission perm;

		perm = GPOINTER_TO_INT (permission_ptr);

		/* See the ‘list’ documentation on:
		 * https://developers.google.com/youtube/2.0/reference?csw=1#youtube_data_api_tag_yt:accessControl */
		json_builder_add_string_value (builder,
		                               (perm == GDATA_YOUTUBE_PERMISSION_ALLOWED) ? "public" : "unlisted");
	} else {
		json_builder_add_string_value (builder,
		                               priv->is_private ? "private" : "public");
	}

	if (g_hash_table_lookup_extended (priv->access_controls,
	                                  GDATA_YOUTUBE_ACTION_EMBED, NULL,
	                                  &permission_ptr)) {
		GDataYouTubePermission perm;

		perm = GPOINTER_TO_INT (permission_ptr);

		json_builder_set_member_name (builder, "embeddable");
		json_builder_add_boolean_value (builder,
		                                perm == GDATA_YOUTUBE_PERMISSION_ALLOWED);
	}

	/* FIXME: add support for:
	 * publicStatsViewable
	 * publishAt
	 * license
	 */

	json_builder_end_object (builder);

	/* recordingDetails object. */
	json_builder_set_member_name (builder, "recordingDetails");
	json_builder_begin_object (builder);

	if (priv->location != NULL) {
		json_builder_set_member_name (builder, "locationDescription");
		json_builder_add_string_value (builder, priv->location);
	}

	if (priv->latitude >= -90.0 && priv->latitude <= 90.0 &&
	    priv->longitude >= -180.0 && priv->longitude <= 180.0) {
		json_builder_set_member_name (builder, "location");
		json_builder_begin_object (builder);

		json_builder_set_member_name (builder, "latitude");
		json_builder_add_double_value (builder, priv->latitude);

		json_builder_set_member_name (builder, "longitude");
		json_builder_add_double_value (builder, priv->longitude);

		json_builder_end_object (builder);
	}

	if (priv->recorded != -1) {
		gchar *recorded = gdata_parser_date_from_int64 (priv->recorded);
		json_builder_set_member_name (builder, "recordingDate");
		json_builder_add_string_value (builder, recorded);
		g_free (recorded);
	}

	json_builder_end_object (builder);
}

static const gchar *
get_content_type (void)
{
	return "application/json";
}

static gchar *
get_entry_uri (const gchar *id)
{
	const gchar *old_prefix = "tag:youtube.com,2008:video:";

	/* For compatibility with previous video ID formats, strip off the v2
	 * ID prefix. */
	if (g_str_has_prefix (id, old_prefix)) {
		id += strlen (old_prefix);
	}

	/* Build the query URI for a single video. This is a bit of a pain,
	 * because it actually returns a list containing a single video, but
	 * there seems no other way to do it. See parsing_in_video_list_response
	 * in parse_json() for the fallout.
	 *
	 * Reference: https://developers.google.com/youtube/v3/docs/videos/list#part */
	return _gdata_service_build_uri ("https://www.googleapis.com/youtube/v3/videos"
	                                 "?part=contentDetails,id,"
	                                       "recordingDetails,snippet,"
	                                       "status,statistics"
	                                 "&id=%s", id);
}

static GDataAuthorizationDomain *
get_authorization_domain (GDataCommentable *self)
{
	return gdata_youtube_service_get_primary_authorization_domain ();
}

static gchar *
get_query_comments_uri (GDataCommentable *self)
{
	const gchar *video_id;

	video_id = gdata_entry_get_id (GDATA_ENTRY (self));

	/* https://developers.google.com/youtube/v3/docs/commentThreads/list */
	return _gdata_service_build_uri ("https://www.googleapis.com"
	                                 "/youtube/v3/commentThreads"
	                                 "?part=snippet"
	                                 "&videoId=%s", video_id);
}

G_GNUC_INTERNAL void
_gdata_youtube_comment_set_video_id (GDataYouTubeComment *self,
                                     const gchar *video_id);
G_GNUC_INTERNAL void
_gdata_youtube_comment_set_channel_id (GDataYouTubeComment *self,
                                       const gchar *channel_id);

static gchar *
get_insert_comment_uri (GDataCommentable *self, GDataComment *comment_)
{
	const gchar *video_id, *channel_id;
	GDataYouTubeVideoPrivate *priv = GDATA_YOUTUBE_VIDEO (self)->priv;

	video_id = gdata_entry_get_id (GDATA_ENTRY (self));
	channel_id = priv->channel_id;

	/* https://developers.google.com/youtube/v3/docs/commentThreads/insert
	 *
	 * We have to set the video ID on the @comment_. */
	_gdata_youtube_comment_set_video_id (GDATA_YOUTUBE_COMMENT (comment_),
	                                     video_id);
	_gdata_youtube_comment_set_channel_id (GDATA_YOUTUBE_COMMENT (comment_),
	                                       channel_id);

	return _gdata_service_build_uri ("https://www.googleapis.com"
	                                 "/youtube/v3/commentThreads"
	                                 "?part=snippet"
	                                 "&shareOnGooglePlus=false");
}

static gboolean
is_comment_deletable (GDataCommentable *self, GDataComment *comment_)
{
	/* FIXME: Currently unsupported:
	 * https://developers.google.com/youtube/v3/migration-guide#to_be_migrated
	 * https://developers.google.com/youtube/v3/guides/implementation/comments#comments-delete */
	return FALSE;
}

/**
 * gdata_youtube_video_new:
 * @id: (allow-none): the video's ID, or %NULL
 *
 * Creates a new #GDataYouTubeVideo with the given ID and default properties.
 *
 * Return value: a new #GDataYouTubeVideo; unref with g_object_unref()
 */
GDataYouTubeVideo *
gdata_youtube_video_new (const gchar *id)
{
	return g_object_new (GDATA_TYPE_YOUTUBE_VIDEO, "id", id, NULL);
}

/**
 * gdata_youtube_video_get_view_count:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:view-count property.
 *
 * Return value: the number of times the video has been viewed
 */
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
 */
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
 */
const gchar *
gdata_youtube_video_get_location (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);
	return self->priv->location;
}

/**
 * gdata_youtube_video_set_location:
 * @self: a #GDataYouTubeVideo
 * @location: (allow-none): a new location, or %NULL
 *
 * Sets the #GDataYouTubeVideo:location property to the new location string, @location.
 *
 * Set @location to %NULL to unset the property in the video.
 */
void
gdata_youtube_video_set_location (GDataYouTubeVideo *self, const gchar *location)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));

	g_free (self->priv->location);
	self->priv->location = g_strdup (location);
	g_object_notify (G_OBJECT (self), "location");
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
 */
GDataYouTubePermission
gdata_youtube_video_get_access_control (GDataYouTubeVideo *self, const gchar *action)
{
	gpointer value;

	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), GDATA_YOUTUBE_PERMISSION_DENIED);
	g_return_val_if_fail (action != NULL, GDATA_YOUTUBE_PERMISSION_DENIED);

	if (g_hash_table_lookup_extended (self->priv->access_controls, action, NULL, &value) == FALSE)
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
 */
void
gdata_youtube_video_set_access_control (GDataYouTubeVideo *self, const gchar *action, GDataYouTubePermission permission)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));
	g_return_if_fail (action != NULL);

	g_hash_table_replace (self->priv->access_controls, g_strdup (action), GINT_TO_POINTER (permission));
}

/**
 * gdata_youtube_video_get_rating:
 * @self: a #GDataYouTubeVideo
 * @min: (out caller-allocates) (allow-none): return location for the minimum rating value, or %NULL
 * @max: (out caller-allocates) (allow-none): return location for the maximum rating value, or %NULL
 * @count: (out caller-allocates) (allow-none): return location for the number of ratings, or %NULL
 * @average: (out caller-allocates) (allow-none): return location for the average rating value, or %NULL
 *
 * Gets various properties of the ratings on the video.
 *
 * Note that this property may not be retrieved when querying for multiple
 * videos at once, but is guaranteed to be retrieved when querying with
 * gdata_service_query_single_entry_async().
 */
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
 * Return value: (array zero-terminated=1) (transfer none): a %NULL-terminated array of words associated with the video
 */
const gchar * const *
gdata_youtube_video_get_keywords (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);
	return (const gchar * const *) self->priv->keywords;
}

/**
 * gdata_youtube_video_set_keywords:
 * @self: a #GDataYouTubeVideo
 * @keywords: (array zero-terminated=1): a new %NULL-terminated array of keywords
 *
 * Sets the #GDataYouTubeVideo:keywords property to the new keyword list, @keywords.
 *
 * @keywords must not be %NULL. For more information, see the <ulink type="http"
 * url="https://developers.google.com/youtube/v3/docs/videos#snippet.tags[]">online documentation</ulink>.
 */
void
gdata_youtube_video_set_keywords (GDataYouTubeVideo *self, const gchar * const *keywords)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));
	g_return_if_fail (keywords != NULL);

	g_strfreev (self->priv->keywords);
	self->priv->keywords = g_strdupv ((gchar **) keywords);
	g_object_notify (G_OBJECT (self), "keywords");
}

/**
 * gdata_youtube_video_get_player_uri:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:player-uri property.
 *
 * Return value: a URI where the video is playable in a web browser, or %NULL
 */
const gchar *
gdata_youtube_video_get_player_uri (GDataYouTubeVideo *self)
{
	GDataYouTubeVideoPrivate *priv;

	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);

	priv = self->priv;

	/* Generate and cache the player URI. */
	if (priv->player_uri == NULL) {
		priv->player_uri = _gdata_service_build_uri ("https://www.youtube.com/watch?v=%s",
		                                             gdata_entry_get_id (GDATA_ENTRY (self)));
	}

	return priv->player_uri;
}

static gboolean
strv_contains (const gchar * const *strv, const gchar *key)
{
	guint i;

	for (i = 0; strv != NULL && strv[i] != NULL; i++) {
		if (g_strcmp0 (strv[i], key) == 0) {
			return TRUE;
		}
	}

	return FALSE;
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
 *
 * Since: 0.4.0
 */
gboolean
gdata_youtube_video_is_restricted_in_country (GDataYouTubeVideo *self, const gchar *country)
{
	GDataYouTubeVideoPrivate *priv;
	gboolean allowed_present, allowed_empty;
	gboolean in_allowed, in_blocked;

	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), FALSE);
	g_return_val_if_fail (country != NULL && *country != '\0', FALSE);

	priv = self->priv;

	allowed_present = (priv->region_restriction_allowed != NULL);
	allowed_empty = (allowed_present &&
	                 priv->region_restriction_allowed[0] == NULL);

	in_allowed = strv_contains ((const gchar * const *) priv->region_restriction_allowed, country);
	in_blocked = strv_contains ((const gchar * const *) priv->region_restriction_blocked, country);

	return ((allowed_present && !in_allowed) ||
	        allowed_empty ||
	        (in_blocked && !in_allowed));
}

/* References:
 * v2: https://developers.google.com/youtube/2.0/reference#youtube_data_api_tag_media:rating
 * v3: https://developers.google.com/youtube/v3/docs/videos#contentDetails.contentRating.mpaaRating
 */
static const gchar *
convert_mpaa_rating (const gchar *v3_rating)
{
	if (g_strcmp0 (v3_rating, "mpaaG") == 0) {
		return "g";
	} else if (g_strcmp0 (v3_rating, "mpaaNc17") == 0) {
		return "nc-17";
	} else if (g_strcmp0 (v3_rating, "mpaaPg") == 0) {
		return "pg";
	} else if (g_strcmp0 (v3_rating, "mpaaPg13") == 0) {
		return "pg-13";
	} else if (g_strcmp0 (v3_rating, "mpaaR") == 0) {
		return "r";
	} else {
		return NULL;
	}
}

/* References:
 * v2: https://developers.google.com/youtube/2.0/reference#youtube_data_api_tag_media:rating
 * v3: https://developers.google.com/youtube/v3/docs/videos#contentDetails.contentRating.tvpgRating
 */
static const gchar *
convert_tvpg_rating (const gchar *v3_rating)
{
	if (g_strcmp0 (v3_rating, "pg14") == 0) {
		return "tv-14";
	} else if (g_strcmp0 (v3_rating, "tvpgG") == 0) {
		return "tv-g";
	} else if (g_strcmp0 (v3_rating, "tvpgMa") == 0) {
		return "tv-ma";
	} else if (g_strcmp0 (v3_rating, "tvpgPg") == 0) {
		return "tv-pg";
	} else if (g_strcmp0 (v3_rating, "tvpgY") == 0) {
		return "tv-y";
	} else if (g_strcmp0 (v3_rating, "tvpgY7") == 0) {
		return "tv-y7";
	} else if (g_strcmp0 (v3_rating, "tvpgY7Fv") == 0) {
		return "tv-y7-fv";
	} else {
		return NULL;
	}
}

/**
 * gdata_youtube_video_get_media_rating:
 * @self: a #GDataYouTubeVideo
 * @rating_type: the type of rating to retrieve
 *
 * Returns the rating of the given type for the video, if one exists. For example, this could be a film rating awarded by the MPAA; or a simple
 * rating specifying whether the video contains adult content.
 *
 * The valid values for @rating_type are: %GDATA_YOUTUBE_RATING_TYPE_MPAA and %GDATA_YOUTUBE_RATING_TYPE_V_CHIP.
 * Further values may be added in future; if an unknown rating type is passed to the function, %NULL will be returned.
 *
 * The possible return values depend on what's passed to @rating_type. Valid values for each rating type are listed in the documentation for the
 * rating types.
 *
 * Return value: the rating of the video for the given @rating_type, or %NULL if the video isn't rated with that type (or the type is unknown)
 *
 * Since: 0.10.0
 */
const gchar *
gdata_youtube_video_get_media_rating (GDataYouTubeVideo *self, const gchar *rating_type)
{
	const gchar *rating;

	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);
	g_return_val_if_fail (rating_type != NULL && *rating_type != '\0', NULL);

	/* All ratings are unknown. */
	if (self->priv->content_ratings == NULL) {
		return NULL;
	}

	/* Compatibility with the old API. */
	if (g_strcmp0 (rating_type, "simple") == 0) {
		/* Not supported any more. */
		return NULL;
	} else if (g_strcmp0 (rating_type, "mpaa") == 0) {
		rating = g_hash_table_lookup (self->priv->content_ratings,
		                              "mpaaRating");
		return convert_mpaa_rating (rating);
	} else if (g_strcmp0 (rating_type, "v-chip") == 0) {
		rating = g_hash_table_lookup (self->priv->content_ratings,
		                              "tvpgRating");
		return convert_tvpg_rating (rating);
	}

	return g_hash_table_lookup (self->priv->content_ratings, rating_type);
}

/**
 * gdata_youtube_video_get_category:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:category property.
 *
 * Return value: (transfer none): a #GDataMediaCategory giving the video's single and mandatory category
 */
GDataMediaCategory *
gdata_youtube_video_get_category (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);
	return self->priv->category;
}

/**
 * gdata_youtube_video_set_category:
 * @self: a #GDataYouTubeVideo
 * @category: a new #GDataMediaCategory
 *
 * Sets the #GDataYouTubeVideo:category property to the new category, @category, and increments its reference count.
 *
 * @category must not be %NULL. For more information, see the <ulink type="http"
 * url="https://developers.google.com/youtube/v3/docs/videos#snippet.categoryId">online documentation</ulink>.
 */
void
gdata_youtube_video_set_category (GDataYouTubeVideo *self, GDataMediaCategory *category)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));
	g_return_if_fail (GDATA_IS_MEDIA_CATEGORY (category));

	g_object_ref (category);
	g_clear_object (&self->priv->category);
	self->priv->category = category;
	g_object_notify (G_OBJECT (self), "category");
}

/**
 * gdata_youtube_video_get_description:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:description property.
 *
 * Return value: the video's long text description, or %NULL
 */
const gchar *
gdata_youtube_video_get_description (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);
	return gdata_entry_get_summary (GDATA_ENTRY (self));
}

/**
 * gdata_youtube_video_set_description:
 * @self: a #GDataYouTubeVideo
 * @description: (allow-none): the video's new description, or %NULL
 *
 * Sets the #GDataYouTubeVideo:description property to the new description, @description.
 *
 * Set @description to %NULL to unset the video's description.
 */
void
gdata_youtube_video_set_description (GDataYouTubeVideo *self, const gchar *description)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));
	gdata_entry_set_summary (GDATA_ENTRY (self), description);
	g_object_notify (G_OBJECT (self), "description");
}

/**
 * gdata_youtube_video_get_thumbnails:
 * @self: a #GDataYouTubeVideo
 *
 * Gets a list of the thumbnails available for the video.
 *
 * Return value: (element-type GData.MediaThumbnail) (transfer none): a #GList of #GDataMediaThumbnails, or %NULL
 */
GList *
gdata_youtube_video_get_thumbnails (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);
	return self->priv->thumbnails;
}

/**
 * gdata_youtube_video_get_duration:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:duration property.
 *
 * Return value: the video duration in seconds, or <code class="literal">0</code> if unknown
 */
guint
gdata_youtube_video_get_duration (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), 0);
	return self->priv->duration;
}

/**
 * gdata_youtube_video_is_private:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:is-private property.
 *
 * Return value: %TRUE if the video is private, %FALSE otherwise
 */
gboolean
gdata_youtube_video_is_private (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), FALSE);
	return self->priv->is_private;
}

/**
 * gdata_youtube_video_set_is_private:
 * @self: a #GDataYouTubeVideo
 * @is_private: whether the video is private
 *
 * Sets the #GDataYouTubeVideo:is-private property to decide whether the video is publicly viewable.
 */
void
gdata_youtube_video_set_is_private (GDataYouTubeVideo *self, gboolean is_private)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));
	self->priv->is_private = is_private;
	g_object_notify (G_OBJECT (self), "is-private");
}

/**
 * gdata_youtube_video_get_uploaded:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:uploaded property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the UNIX timestamp for the time the video was uploaded, or <code class="literal">-1</code>
 */
gint64
gdata_youtube_video_get_uploaded (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), -1);
	return gdata_entry_get_published (GDATA_ENTRY (self));
}

/* Convert from v3 to v2 API video upload state. References:
 * v2: https://developers.google.com/youtube/2.0/reference?csw=1#youtube_data_api_tag_yt:state
 * v3: https://developers.google.com/youtube/v3/docs/videos#processingDetails.processingStatus
 *     https://developers.google.com/youtube/v3/docs/videos#status.uploadStatus
 */
static const gchar *
convert_state_name (const gchar *v3_processing_status,
                    const gchar *v3_upload_status)
{
	if (g_strcmp0 (v3_upload_status, "deleted") == 0 ||
	    g_strcmp0 (v3_upload_status, "failed") == 0 ||
	    g_strcmp0 (v3_upload_status, "rejected") == 0) {
		return v3_upload_status;
	} else if (g_strcmp0 (v3_processing_status, "processing") == 0) {
		return v3_processing_status;
	}

	return NULL;
}

/* References:
 * v2: https://developers.google.com/youtube/2.0/reference?csw=1#youtube_data_api_tag_yt:state
 * v3: https://developers.google.com/youtube/v3/docs/videos#status.failureReason
 *     https://developers.google.com/youtube/v3/docs/videos#status.rejectionReason
 */
static const gchar *
convert_state_reason_code (const gchar *v2_name,
                           const gchar *v3_failure_reason,
                           const gchar *v3_rejection_reason)
{
	if (v2_name == NULL ||
	    g_strcmp0 (v2_name, "processing") == 0 ||
	    g_strcmp0 (v2_name, "deleted") == 0) {
		/* Explicitly unset if unknown, processing or deleted. */
		return NULL;
	} else if (g_strcmp0 (v2_name, "restricted") == 0) {
		/* Unsupported conversion; convert_state_name() can never return
		 * ‘restricted’ anyway. */
		return NULL;
	} else if (g_strcmp0 (v2_name, "rejected") == 0) {
		if (g_strcmp0 (v3_rejection_reason, "claim") == 0 ||
		    g_strcmp0 (v3_rejection_reason, "copyright") == 0 ||
		    g_strcmp0 (v3_rejection_reason, "trademark") == 0) {
			return "copyright";
		} else if (g_strcmp0 (v3_rejection_reason, "duplicate") == 0) {
			return "duplicate";
		} else if (g_strcmp0 (v3_rejection_reason,
		                      "inappropriate") == 0) {
			return "inappropriate";
		} else if (g_strcmp0 (v3_rejection_reason, "length") == 0) {
			return "tooLong";
		} else if (g_strcmp0 (v3_rejection_reason, "termsOfUse") == 0) {
			return "termsOfUse";
		} else if (g_strcmp0 (v3_rejection_reason,
		                      "uploaderAccountClosed") == 0 ||
		           g_strcmp0 (v3_rejection_reason,
		                      "uploaderAccountSuspended") == 0) {
			return "duplicate";
		} else {
			/* Generic fallback. */
			return "termsOfUse";
		}
	} else if (g_strcmp0 (v2_name, "failed") == 0) {
		if (g_strcmp0 (v3_failure_reason, "codec") == 0) {
			return "unsupportedCodec";
		} else if (g_strcmp0 (v3_failure_reason, "conversion") == 0) {
			return "invalidFormat";
		} else if (g_strcmp0 (v3_failure_reason, "emptyFile") == 0) {
			return "empty";
		} else if (g_strcmp0 (v3_failure_reason, "tooSmall") == 0) {
			return "tooSmall";
		} else {
			return "cantProcess";
		}
	}

	return NULL;
}

/**
 * gdata_youtube_video_get_state:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:state property.
 *
 * For more information, see the <ulink type="http"
 * url="https://developers.google.com/youtube/v3/docs/videos#status.uploadStatus">online documentation</ulink>.
 *
 * Return value: (transfer none): a #GDataYouTubeState showing the state of the video, or %NULL
 */
GDataYouTubeState *
gdata_youtube_video_get_state (GDataYouTubeVideo *self)
{
	GDataYouTubeVideoPrivate *priv;

	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);

	priv = self->priv;

	/* Lazily create the state object. */
	if (priv->upload_state == NULL) {
		const gchar *name, *reason_code;

		name = convert_state_name (priv->processing_status,
		                           priv->upload_status);
		reason_code = convert_state_reason_code (name,
		                                         priv->failure_reason,
		                                         priv->rejection_reason);

		priv->upload_state = g_object_new (GDATA_TYPE_YOUTUBE_STATE,
		                                   "name", name,
		                                   "reason-code", reason_code,
		                                   "help-uri", NULL,
		                                   "message", NULL,
		                                   NULL);
	}

	return priv->upload_state;
}

/**
 * gdata_youtube_video_get_recorded:
 * @self: a #GDataYouTubeVideo
 *
 * Gets the #GDataYouTubeVideo:recorded property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the UNIX timestamp for the time the video was recorded, or <code class="literal">-1</code>
 *
 * Since: 0.3.0
 */
gint64
gdata_youtube_video_get_recorded (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), -1);
	return self->priv->recorded;
}

/**
 * gdata_youtube_video_set_recorded:
 * @self: a #GDataYouTubeVideo
 * @recorded: the video's new recorded time, or <code class="literal">-1</code>
 *
 * Sets the #GDataYouTubeVideo:recorded property to the new recorded time, @recorded.
 *
 * Set @recorded to <code class="literal">-1</code> to unset the video's recorded time.
 *
 * Since: 0.3.0
 */
void
gdata_youtube_video_set_recorded (GDataYouTubeVideo *self, gint64 recorded)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));
	g_return_if_fail (recorded >= -1);

	self->priv->recorded = recorded;
	g_object_notify (G_OBJECT (self), "recorded");
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
 */
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
 * Return value: the aspect ratio property, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_youtube_video_get_aspect_ratio (GDataYouTubeVideo *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (self), NULL);

	/* Permanently NULL for the moment, but let’s not deprecate the property
	 * because it looks like it might come in useful in future. */
	return NULL;
}

/**
 * gdata_youtube_video_set_aspect_ratio:
 * @self: a #GDataYouTubeVideo
 * @aspect_ratio: (allow-none): the aspect ratio property, or %NULL
 *
 * Sets the #GDataYouTubeVideo:aspect-ratio property to specify the video's aspect ratio.
 * If @aspect_ratio is %NULL, the property will be unset.
 *
 * Since: 0.4.0
 */
void
gdata_youtube_video_set_aspect_ratio (GDataYouTubeVideo *self, const gchar *aspect_ratio)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));

	/* Ignore it. See note in gdata_youtube_video_get_aspect_ratio(),
	 * above. */
}

/**
 * gdata_youtube_video_get_coordinates:
 * @self: a #GDataYouTubeVideo
 * @latitude: (out caller-allocates) (allow-none): return location for the latitude, or %NULL
 * @longitude: (out caller-allocates) (allow-none): return location for the longitude, or %NULL
 *
 * Gets the #GDataYouTubeVideo:latitude and #GDataYouTubeVideo:longitude properties, setting the out parameters to them. If either latitude or
 * longitude is %NULL, that parameter will not be set. If the coordinates are unset, @latitude and @longitude will be set to %G_MAXDOUBLE.
 *
 * Since: 0.8.0
 */
void
gdata_youtube_video_get_coordinates (GDataYouTubeVideo *self, gdouble *latitude, gdouble *longitude)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));

	if (latitude != NULL) {
		*latitude = self->priv->latitude;
	}
	if (longitude != NULL) {
		*longitude = self->priv->longitude;
	}
}

/**
 * gdata_youtube_video_set_coordinates:
 * @self: a #GDataYouTubeVideo
 * @latitude: the video's new latitude coordinate, or %G_MAXDOUBLE
 * @longitude: the video's new longitude coordinate, or %G_MAXDOUBLE
 *
 * Sets the #GDataYouTubeVideo:latitude and #GDataYouTubeVideo:longitude properties to @latitude and @longitude respectively.
 *
 * Since: 0.8.0
 */
void
gdata_youtube_video_set_coordinates (GDataYouTubeVideo *self, gdouble latitude, gdouble longitude)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (self));

	self->priv->latitude = latitude;
	self->priv->longitude = longitude;

	g_object_freeze_notify (G_OBJECT (self));
	g_object_notify (G_OBJECT (self), "latitude");
	g_object_notify (G_OBJECT (self), "longitude");
	g_object_thaw_notify (G_OBJECT (self));
}
