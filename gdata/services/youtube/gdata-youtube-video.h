/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2008-2009, 2015 <philip@tecnocode.co.uk>
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

#ifndef GDATA_YOUTUBE_VIDEO_H
#define GDATA_YOUTUBE_VIDEO_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-entry.h>
#include <gdata/media/gdata-media-category.h>
#include <gdata/services/youtube/gdata-youtube-state.h>

G_BEGIN_DECLS

/**
 * GDATA_YOUTUBE_ASPECT_RATIO_WIDESCREEN:
 *
 * The aspect ratio for widescreen (16:9) videos.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:aspectratio">
 * online documentation</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_YOUTUBE_ASPECT_RATIO_WIDESCREEN "widescreen"

/**
 * GDATA_YOUTUBE_ACTION_RATE:
 *
 * An action to rate a video, for use with gdata_youtube_video_set_access_control().
 *
 * Since: 0.7.0
 */
#define GDATA_YOUTUBE_ACTION_RATE "rate"

/**
 * GDATA_YOUTUBE_ACTION_COMMENT:
 *
 * An action to comment on a video, for use with gdata_youtube_video_set_access_control().
 *
 * Since: 0.7.0
 */
#define GDATA_YOUTUBE_ACTION_COMMENT "comment"

/**
 * GDATA_YOUTUBE_ACTION_COMMENT_VOTE:
 *
 * An action to rate other users' comments on a video, for use with gdata_youtube_video_set_access_control().
 *
 * Since: 0.7.0
 */
#define GDATA_YOUTUBE_ACTION_COMMENT_VOTE "commentVote"

/**
 * GDATA_YOUTUBE_ACTION_VIDEO_RESPOND:
 *
 * An action to add a video response to a video, for use with gdata_youtube_video_set_access_control().
 *
 * Since: 0.7.0
 */
#define GDATA_YOUTUBE_ACTION_VIDEO_RESPOND "videoRespond"

/**
 * GDATA_YOUTUBE_ACTION_EMBED:
 *
 * An action to embed a video on third-party websites, for use with gdata_youtube_video_set_access_control().
 *
 * Since: 0.7.0
 */
#define GDATA_YOUTUBE_ACTION_EMBED "embed"

/**
 * GDATA_YOUTUBE_ACTION_SYNDICATE:
 *
 * An action allowing YouTube to show the video on mobile phones and televisions, for use with gdata_youtube_video_set_access_control().
 *
 * Since: 0.7.0
 */
#define GDATA_YOUTUBE_ACTION_SYNDICATE "syndicate"

/**
 * GDATA_YOUTUBE_RATING_TYPE_MPAA:
 *
 * A rating type to pass to gdata_youtube_video_get_media_rating() for ratings by the <ulink type="http" url="http://www.mpaa.org/">MPAA</ulink>. The
 * values which can be returned for such ratings are: <code class="literal">g</code>, <code class="literal">pg</code>,
 * <code class="literal">pg-13</code>, <code class="literal">r</code> and <code class="literal">nc-17</code>.
 *
 * Since: 0.10.0
 */
#define GDATA_YOUTUBE_RATING_TYPE_MPAA "mpaa"

/**
 * GDATA_YOUTUBE_RATING_TYPE_V_CHIP:
 *
 * A rating type to pass to gdata_youtube_video_get_media_rating() for ratings following the FCC
 * <ulink type="http" url="http://www.fcc.gov/vchip/">V-Chip</ulink> system. The values which can be returned for such ratings are:
 * <code class="literal">tv-y</code>, <code class="literal">tv-y7</code>, <code class="literal">tv-y7-fv</code>, <code class="literal">tv-g</code>,
 * <code class="literal">tv-pg</code>, <code class="literal">tv-14</code> and <code class="literal">tv-ma</code>.
 *
 * Since: 0.10.0
 */
#define GDATA_YOUTUBE_RATING_TYPE_V_CHIP "v-chip"

/**
 * GDataYouTubePermission:
 * @GDATA_YOUTUBE_PERMISSION_ALLOWED: the action is allowed for everyone
 * @GDATA_YOUTUBE_PERMISSION_DENIED: the action is denied for everyone
 * @GDATA_YOUTUBE_PERMISSION_MODERATED: the action is moderated by the video owner
 *
 * Permissions for actions which can be set on a #GDataYouTubeVideo using gdata_youtube_video_set_access_control().
 *
 * The only actions which can have the %GDATA_YOUTUBE_PERMISSION_MODERATED permission are
 * %GDATA_YOUTUBE_ACTION_RATE and %GDATA_YOUTUBE_ACTION_COMMENT.
 *
 * Since: 0.7.0
 */
typedef enum {
	GDATA_YOUTUBE_PERMISSION_ALLOWED,
	GDATA_YOUTUBE_PERMISSION_DENIED,
	GDATA_YOUTUBE_PERMISSION_MODERATED
} GDataYouTubePermission;

#define GDATA_TYPE_YOUTUBE_VIDEO		(gdata_youtube_video_get_type ())
#define GDATA_YOUTUBE_VIDEO(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_YOUTUBE_VIDEO, GDataYouTubeVideo))
#define GDATA_YOUTUBE_VIDEO_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_YOUTUBE_VIDEO, GDataYouTubeVideoClass))
#define GDATA_IS_YOUTUBE_VIDEO(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_YOUTUBE_VIDEO))
#define GDATA_IS_YOUTUBE_VIDEO_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_YOUTUBE_VIDEO))
#define GDATA_YOUTUBE_VIDEO_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_YOUTUBE_VIDEO, GDataYouTubeVideoClass))

typedef struct _GDataYouTubeVideoPrivate	GDataYouTubeVideoPrivate;

/**
 * GDataYouTubeVideo:
 *
 * All the fields in the #GDataYouTubeVideo structure are private and should never be accessed directly.
 */
typedef struct {
	GDataEntry parent;
	GDataYouTubeVideoPrivate *priv;
} GDataYouTubeVideo;

/**
 * GDataYouTubeVideoClass:
 *
 * All the fields in the #GDataYouTubeVideoClass structure are private and should never be accessed directly.
 */
typedef struct {
	/*< private >*/
	GDataEntryClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataYouTubeVideoClass;

GType gdata_youtube_video_get_type (void) G_GNUC_CONST;

GDataYouTubeVideo *gdata_youtube_video_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

guint gdata_youtube_video_get_view_count (GDataYouTubeVideo *self) G_GNUC_PURE;
guint gdata_youtube_video_get_favorite_count (GDataYouTubeVideo *self) G_GNUC_PURE;
const gchar *gdata_youtube_video_get_location (GDataYouTubeVideo *self) G_GNUC_PURE;
void gdata_youtube_video_set_location (GDataYouTubeVideo *self, const gchar *location);
GDataYouTubePermission gdata_youtube_video_get_access_control (GDataYouTubeVideo *self, const gchar *action) G_GNUC_PURE;
void gdata_youtube_video_set_access_control (GDataYouTubeVideo *self, const gchar *action, GDataYouTubePermission permission);
void gdata_youtube_video_get_rating (GDataYouTubeVideo *self, guint *min, guint *max, guint *count, gdouble *average);
const gchar * const *gdata_youtube_video_get_keywords (GDataYouTubeVideo *self) G_GNUC_PURE;
void gdata_youtube_video_set_keywords (GDataYouTubeVideo *self, const gchar * const *keywords);
const gchar *gdata_youtube_video_get_player_uri (GDataYouTubeVideo *self) G_GNUC_PURE;
gboolean gdata_youtube_video_is_restricted_in_country (GDataYouTubeVideo *self, const gchar *country) G_GNUC_PURE;
const gchar *gdata_youtube_video_get_media_rating (GDataYouTubeVideo *self, const gchar *rating_type) G_GNUC_PURE;
GDataMediaCategory *gdata_youtube_video_get_category (GDataYouTubeVideo *self) G_GNUC_PURE;
void gdata_youtube_video_set_category (GDataYouTubeVideo *self, GDataMediaCategory *category);
const gchar *gdata_youtube_video_get_description (GDataYouTubeVideo *self) G_GNUC_PURE;
void gdata_youtube_video_set_description (GDataYouTubeVideo *self, const gchar *description);
GList *gdata_youtube_video_get_thumbnails (GDataYouTubeVideo *self) G_GNUC_PURE;
guint gdata_youtube_video_get_duration (GDataYouTubeVideo *self) G_GNUC_PURE;
gboolean gdata_youtube_video_is_private (GDataYouTubeVideo *self) G_GNUC_PURE;
void gdata_youtube_video_set_is_private (GDataYouTubeVideo *self, gboolean is_private);
gint64 gdata_youtube_video_get_uploaded (GDataYouTubeVideo *self);
GDataYouTubeState *gdata_youtube_video_get_state (GDataYouTubeVideo *self) G_GNUC_PURE;
gint64 gdata_youtube_video_get_recorded (GDataYouTubeVideo *self);
void gdata_youtube_video_set_recorded (GDataYouTubeVideo *self, gint64 recorded);
const gchar *gdata_youtube_video_get_aspect_ratio (GDataYouTubeVideo *self) G_GNUC_PURE;
void gdata_youtube_video_set_aspect_ratio (GDataYouTubeVideo *self, const gchar *aspect_ratio);
void gdata_youtube_video_get_coordinates (GDataYouTubeVideo *self, gdouble *latitude, gdouble *longitude);
void gdata_youtube_video_set_coordinates (GDataYouTubeVideo *self, gdouble latitude, gdouble longitude);

gchar *gdata_youtube_video_get_video_id_from_uri (const gchar *video_uri) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_YOUTUBE_VIDEO_H */
