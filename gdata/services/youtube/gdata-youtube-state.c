/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009–2010, 2015 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-youtube-state
 * @short_description: YouTube state element
 * @stability: Stable
 * @include: gdata/services/youtube/gdata-youtube-state.h
 *
 * #GDataYouTubeState represents a "state" element from the
 * <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:state">YouTube namespace</ulink>.
 *
 * Since: 0.4.0
 */

#include <glib.h>

#include "gdata-youtube-state.h"
#include "gdata-parsable.h"

static void gdata_youtube_state_finalize (GObject *object);
static void gdata_youtube_state_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void
gdata_youtube_state_set_property (GObject *object,
                                  guint property_id,
                                  const GValue *value,
                                  GParamSpec *pspec);

struct _GDataYouTubeStatePrivate {
	gchar *name;
	gchar *reason_code;
	gchar *help_uri;
	gchar *message;
};

enum {
	PROP_NAME = 1,
	PROP_REASON_CODE,
	PROP_HELP_URI,
	PROP_MESSAGE
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataYouTubeState, gdata_youtube_state, GDATA_TYPE_PARSABLE)

static void
gdata_youtube_state_class_init (GDataYouTubeStateClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->get_property = gdata_youtube_state_get_property;
	gobject_class->set_property = gdata_youtube_state_set_property;
	gobject_class->finalize = gdata_youtube_state_finalize;

	/**
	 * GDataYouTubeState:name:
	 *
	 * The name of the status of the unpublished video. Valid values are: "processing", "restricted", "deleted", "rejected" and "failed".
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:state">online documentation</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_NAME,
	                                 g_param_spec_string ("name",
	                                                      "Name", "The name of the status of the unpublished video.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY |
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeState:reason-code:
	 *
	 * The reason code explaining why the video failed to upload.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:state">online documentation</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_REASON_CODE,
	                                 g_param_spec_string ("reason-code",
	                                                      "Reason code", "The reason code explaining why the video failed to upload.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY |
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeState:help-uri:
	 *
	 * A URI for a YouTube Help Center page that may help the developer or the video owner to diagnose
	 * the reason that an upload failed or was rejected.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:state">online documentation</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_HELP_URI,
	                                 g_param_spec_string ("help-uri",
	                                                      "Help URI", "A URI for a YouTube Help Center page.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY |
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeState:message:
	 *
	 * A human-readable description of why the video failed to upload.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_yt:state">online documentation</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_MESSAGE,
	                                 g_param_spec_string ("message",
	                                                      "Message", "A human-readable description of why the video failed to upload.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY |
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_STATIC_STRINGS));
}

static void
gdata_youtube_state_init (GDataYouTubeState *self)
{
	self->priv = gdata_youtube_state_get_instance_private (self);
}

static void
gdata_youtube_state_finalize (GObject *object)
{
	GDataYouTubeStatePrivate *priv = GDATA_YOUTUBE_STATE (object)->priv;

	g_free (priv->name);
	g_free (priv->reason_code);
	g_free (priv->help_uri);
	g_free (priv->message);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_youtube_state_parent_class)->finalize (object);
}

static void
gdata_youtube_state_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataYouTubeStatePrivate *priv = GDATA_YOUTUBE_STATE (object)->priv;

	switch (property_id) {
		case PROP_NAME:
			g_value_set_string (value, priv->name);
			break;
		case PROP_REASON_CODE:
			g_value_set_string (value, priv->reason_code);
			break;
		case PROP_HELP_URI:
			g_value_set_string (value, priv->help_uri);
			break;
		case PROP_MESSAGE:
			g_value_set_string (value, priv->message);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_youtube_state_set_property (GObject *object,
                                  guint property_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
	GDataYouTubeStatePrivate *priv = GDATA_YOUTUBE_STATE (object)->priv;

	switch (property_id) {
		case PROP_NAME:
			priv->name = g_value_dup_string (value);
			break;
		case PROP_REASON_CODE:
			priv->reason_code = g_value_dup_string (value);
			break;
		case PROP_HELP_URI:
			priv->help_uri = g_value_dup_string (value);
			break;
		case PROP_MESSAGE:
			priv->message = g_value_dup_string (value);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

/**
 * gdata_youtube_state_get_name:
 * @self: a #GDataYouTubeState
 *
 * Gets the #GDataYouTubeState:name property.
 *
 * Return value: the status name
 *
 * Since: 0.4.0
 */
const gchar *
gdata_youtube_state_get_name (GDataYouTubeState *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_STATE (self), NULL);
	return self->priv->name;
}

/**
 * gdata_youtube_state_get_reason_code:
 * @self: a #GDataYouTubeState
 *
 * Gets the #GDataYouTubeState:reason-code property.
 *
 * Return value: the status reason code, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_youtube_state_get_reason_code (GDataYouTubeState *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_STATE (self), NULL);
	return self->priv->reason_code;
}

/**
 * gdata_youtube_state_get_help_uri:
 * @self: a #GDataYouTubeState
 *
 * Gets the #GDataYouTubeState:help-uri property.
 *
 * Return value: the help URI, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_youtube_state_get_help_uri (GDataYouTubeState *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_STATE (self), NULL);
	return self->priv->help_uri;
}

/**
 * gdata_youtube_state_get_message:
 * @self: a #GDataYouTubeState
 *
 * Gets the #GDataYouTubeState:message property.
 *
 * Return value: the status message, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_youtube_state_get_message (GDataYouTubeState *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_STATE (self), NULL);
	return self->priv->message;
}
