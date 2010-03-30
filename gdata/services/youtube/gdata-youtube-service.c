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
 * SECTION:gdata-youtube-service
 * @short_description: GData YouTube service object
 * @stability: Unstable
 * @include: gdata/services/youtube/gdata-youtube-service.h
 *
 * #GDataYouTubeService is a subclass of #GDataService for communicating with the GData API of YouTube. It supports querying for and
 * uploading videos.
 *
 * For more details of YouTube's GData API, see the <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html">
 * online documentation</ulink>.
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libsoup/soup.h>
#include <string.h>

#include "gdata-youtube-service.h"
#include "gdata-service.h"
#include "gdata-private.h"
#include "gdata-parser.h"
#include "atom/gdata-link.h"
#include "gdata-upload-stream.h"

/* Standards reference here: http://code.google.com/apis/youtube/2.0/reference.html */

GQuark
gdata_youtube_service_error_quark (void)
{
	return g_quark_from_static_string ("gdata-youtube-service-error-quark");
}

static void gdata_youtube_service_finalize (GObject *object);
static void gdata_youtube_service_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_youtube_service_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean parse_authentication_response (GDataService *self, guint status, const gchar *response_body, gint length, GError **error);
static void append_query_headers (GDataService *self, SoupMessage *message);
static void parse_error_response (GDataService *self, GDataOperationType operation_type, guint status, const gchar *reason_phrase,
                                  const gchar *response_body, gint length, GError **error);

struct _GDataYouTubeServicePrivate {
	gchar *youtube_user;
	gchar *developer_key;
};

enum {
	PROP_DEVELOPER_KEY = 1,
	PROP_YOUTUBE_USER
};

G_DEFINE_TYPE (GDataYouTubeService, gdata_youtube_service, GDATA_TYPE_SERVICE)

static void
gdata_youtube_service_class_init (GDataYouTubeServiceClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataServiceClass *service_class = GDATA_SERVICE_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataYouTubeServicePrivate));

	gobject_class->set_property = gdata_youtube_service_set_property;
	gobject_class->get_property = gdata_youtube_service_get_property;
	gobject_class->finalize = gdata_youtube_service_finalize;

	service_class->service_name = "youtube";
	service_class->authentication_uri = "https://www.google.com/youtube/accounts/ClientLogin";
	service_class->parse_authentication_response = parse_authentication_response;
	service_class->append_query_headers = append_query_headers;
	service_class->parse_error_response = parse_error_response;

	/**
	 * GDataYouTubeService:developer-key:
	 *
	 * The developer key your application has registered with the YouTube API. For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/developers_guide_protocol.html#Developer_Key">online documentation</ulink>.
	 *
	 * The matching #GDataService:client-id property belongs to #GDataService.
	 **/
	g_object_class_install_property (gobject_class, PROP_DEVELOPER_KEY,
				g_param_spec_string ("developer-key",
					"Developer key", "Your YouTube developer API key.",
					NULL,
					G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeService:youtube-user:
	 *
	 * The YouTube username of the authenticated user, or %NULL. This may differ from #GDataService:username, due to the work done when
	 * YouTube was converted to use Google's centralised login system.
	 **/
	g_object_class_install_property (gobject_class, PROP_YOUTUBE_USER,
				g_param_spec_string ("youtube-user",
					"YouTube username", "The YouTube account username.",
					NULL,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_youtube_service_init (GDataYouTubeService *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_YOUTUBE_SERVICE, GDataYouTubeServicePrivate);
}

static void
gdata_youtube_service_finalize (GObject *object)
{
	GDataYouTubeServicePrivate *priv = GDATA_YOUTUBE_SERVICE (object)->priv;

	g_free (priv->youtube_user);
	g_free (priv->developer_key);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_youtube_service_parent_class)->finalize (object);
}

static void
gdata_youtube_service_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataYouTubeServicePrivate *priv = GDATA_YOUTUBE_SERVICE (object)->priv;

	switch (property_id) {
		case PROP_DEVELOPER_KEY:
			g_value_set_string (value, priv->developer_key);
			break;
		case PROP_YOUTUBE_USER:
			g_value_set_string (value, priv->youtube_user);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_youtube_service_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataYouTubeServicePrivate *priv = GDATA_YOUTUBE_SERVICE (object)->priv;

	switch (property_id) {
		case PROP_DEVELOPER_KEY:
			priv->developer_key = g_value_dup_string (value);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static gboolean
parse_authentication_response (GDataService *self, guint status, const gchar *response_body, gint length, GError **error)
{
	GDataYouTubeServicePrivate *priv = GDATA_YOUTUBE_SERVICE (self)->priv;
	gchar *user_start, *user_end;

	/* Chain up to the parent method first */
	if (GDATA_SERVICE_CLASS (gdata_youtube_service_parent_class)->parse_authentication_response (self, status,
												     response_body, length, error) == FALSE) {
		return FALSE;
	}

	/* Parse the response */
	user_start = strstr (response_body, "YouTubeUser=");
	if (user_start == NULL)
		goto protocol_error;
	user_start += strlen ("YouTubeUser=");

	user_end = strstr (user_start, "\n");
	if (user_end == NULL)
		goto protocol_error;

	priv->youtube_user = g_strndup (user_start, user_end - user_start);
	if (priv->youtube_user == NULL || strlen (priv->youtube_user) == 0)
		goto protocol_error;

	return TRUE;

protocol_error:
	g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
			     _("The server returned a malformed response."));
	return FALSE;
}

static void
append_query_headers (GDataService *self, SoupMessage *message)
{
	GDataYouTubeServicePrivate *priv = GDATA_YOUTUBE_SERVICE (self)->priv;
	gchar *key_header;

	g_assert (message != NULL);

	/* Dev key and client headers */
	key_header = g_strdup_printf ("key=%s", priv->developer_key);
	soup_message_headers_append (message->request_headers, "X-GData-Key", key_header);
	g_free (key_header);

	soup_message_headers_append (message->request_headers, "X-GData-Client", gdata_service_get_client_id (self));

	/* Chain up to the parent class */
	GDATA_SERVICE_CLASS (gdata_youtube_service_parent_class)->append_query_headers (self, message);
}

static void
parse_error_response (GDataService *self, GDataOperationType operation_type, guint status, const gchar *reason_phrase, const gchar *response_body,
                      gint length, GError **error)
{
	xmlDoc *doc;
	xmlNode *node;

	if (response_body == NULL)
		goto parent;

	if (length == -1)
		length = strlen (response_body);

	/* Parse the XML */
	doc = xmlReadMemory (response_body, length, "/dev/null", NULL, 0);
	if (doc == NULL)
		goto parent;

	/* Get the root element */
	node = xmlDocGetRootElement (doc);
	if (node == NULL) {
		/* XML document's empty; chain up to the parent class */
		xmlFreeDoc (doc);
		goto parent;
	}

	if (xmlStrcmp (node->name, (xmlChar*) "errors") != 0) {
		/* No <errors> element (required); chain up to the parent class */
		xmlFreeDoc (doc);
		goto parent;
	}

	/* Parse the actual errors */
	node = node->children;
	while (node != NULL) {
		xmlChar *domain = NULL, *code = NULL, *location = NULL;
		xmlNode *child_node = node->children;

		if (node->type == XML_TEXT_NODE) {
			/* Skip text nodes; they're all whitespace */
			node = node->next;
			continue;
		}

		/* Get the error data */
		while (child_node != NULL) {
			if (child_node->type == XML_TEXT_NODE) {
				/* Skip text nodes; they're all whitespace */
				child_node = child_node->next;
				continue;
			}

			if (xmlStrcmp (child_node->name, (xmlChar*) "domain") == 0)
				domain = xmlNodeListGetString (doc, child_node->children, TRUE);
			else if (xmlStrcmp (child_node->name, (xmlChar*) "code") == 0)
				code = xmlNodeListGetString (doc, child_node->children, TRUE);
			else if (xmlStrcmp (child_node->name, (xmlChar*) "location") == 0)
				location = xmlNodeListGetString (doc, child_node->children, TRUE);
			else if (xmlStrcmp (child_node->name, (xmlChar*) "internalReason") != 0) {
				/* Unknown element (ignore internalReason) */
				g_message ("Unhandled <error/%s> element.", child_node->name);

				xmlFree (domain);
				xmlFree (code);
				xmlFree (location);
				xmlFreeDoc (doc);
				goto check_error;
			}

			child_node = child_node->next;
		}

		/* Create an error message, but only for the first error */
		if (*error == NULL) {
			/* See http://code.google.com/apis/youtube/2.0/developers_guide_protocol.html#Error_responses */
			if (xmlStrcmp (domain, (xmlChar*) "yt:service") == 0 && xmlStrcmp (code, (xmlChar*) "disabled_in_maintenance_mode") == 0) {
				/* Service disabled */
				g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_UNAVAILABLE,
					     _("This service is not available at the moment."));
			} else if (xmlStrcmp (domain, (xmlChar*) "yt:authentication") == 0) {
				/* Authentication problem; make sure to set our status as unauthenticated */
				_gdata_service_set_authenticated (GDATA_SERVICE (self), FALSE);
				g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
					     _("You must be authenticated to do this."));
			} else if (xmlStrcmp (domain, (xmlChar*) "yt:quota") == 0) {
				/* Quota errors */
				if (xmlStrcmp (code, (xmlChar*) "too_many_recent_calls") == 0) {
					g_set_error (error, GDATA_YOUTUBE_SERVICE_ERROR, GDATA_YOUTUBE_SERVICE_ERROR_API_QUOTA_EXCEEDED,
						     _("You have made too many API calls recently. Please wait a few minutes and try again."));
				} else if (xmlStrcmp (code, (xmlChar*) "too_many_entries") == 0) {
					g_set_error (error, GDATA_YOUTUBE_SERVICE_ERROR, GDATA_YOUTUBE_SERVICE_ERROR_ENTRY_QUOTA_EXCEEDED,
						     _("You have exceeded your entry quota. Please delete some entries and try again."));
				} else {
					/* Protocol error */
					g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
						     /* Translators: the first parameter is an error code, which is a coded string. The second parameter
						      * is an error domain, which is another coded string. The third parameter is the location of the
						      * error, which is either a URI or an XPath. */
						     _("Unknown error code \"%s\" in domain \"%s\" received with location \"%s\"."),
						     code, domain, location);
				}
			} else {
				/* Unknown or validation (protocol) error */
				g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
					     _("Unknown error code \"%s\" in domain \"%s\" received with location \"%s\"."),
					     code, domain, location);
			}
		} else {
			/* For all errors after the first, log the error in the terminal */
			g_debug ("Error message received in response: code \"%s\", domain \"%s\", location \"%s\".", code, domain, location);
		}

		xmlFree (domain);
		xmlFree (code);
		xmlFree (location);

		node = node->next;
	}

check_error:
	/* Ensure we're actually set an error message */
	if (*error == NULL)
		g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR, _("Unknown and unparsable error received."));

	return;

parent:
	/* Chain up to the parent class */
	GDATA_SERVICE_CLASS (gdata_youtube_service_parent_class)->parse_error_response (self, operation_type, status, reason_phrase,
	                                                                                response_body, length, error);
}

/**
 * gdata_youtube_service_new:
 * @developer_key: your application's developer API key
 * @client_id: your application's client ID
 *
 * Creates a new #GDataYouTubeService. The @developer_key and @client_id must be unique for your application, and as
 * <ulink type="http" url="http://code.google.com/apis/youtube/2.0/developers_guide_protocol.html#Developer_Key">registered with Google</ulink>.
 *
 * Return value: a new #GDataYouTubeService, or %NULL
 **/
GDataYouTubeService *
gdata_youtube_service_new (const gchar *developer_key, const gchar *client_id)
{
	g_return_val_if_fail (developer_key != NULL, NULL);
	g_return_val_if_fail (client_id != NULL, NULL);

	return g_object_new (GDATA_TYPE_YOUTUBE_SERVICE,
			     "developer-key", developer_key,
			     "client-id", client_id,
			     NULL);
}

static const gchar *
standard_feed_type_to_feed_uri (GDataYouTubeStandardFeedType feed_type)
{
	switch (feed_type) {
	case GDATA_YOUTUBE_TOP_RATED_FEED:
		return "http://gdata.youtube.com/feeds/api/standardfeeds/top_rated";
	case GDATA_YOUTUBE_TOP_FAVORITES_FEED:
		return "http://gdata.youtube.com/feeds/api/standardfeeds/top_favorites";
	case GDATA_YOUTUBE_MOST_VIEWED_FEED:
		return "http://gdata.youtube.com/feeds/api/standardfeeds/most_viewed";
	case GDATA_YOUTUBE_MOST_POPULAR_FEED:
		return "http://gdata.youtube.com/feeds/api/standardfeeds/most_popular";
	case GDATA_YOUTUBE_MOST_RECENT_FEED:
		return "http://gdata.youtube.com/feeds/api/standardfeeds/most_recent";
	case GDATA_YOUTUBE_MOST_DISCUSSED_FEED:
		return "http://gdata.youtube.com/feeds/api/standardfeeds/most_discussed";
	case GDATA_YOUTUBE_MOST_LINKED_FEED:
		return "http://gdata.youtube.com/feeds/api/standardfeeds/most_linked";
	case GDATA_YOUTUBE_MOST_RESPONDED_FEED:
		return "http://gdata.youtube.com/feeds/api/standardfeeds/most_responded";
	case GDATA_YOUTUBE_RECENTLY_FEATURED_FEED:
		return "http://gdata.youtube.com/feeds/api/standardfeeds/recently_featured";
	case GDATA_YOUTUBE_WATCH_ON_MOBILE_FEED:
		return "http://gdata.youtube.com/feeds/api/standardfeeds/watch_on_mobile";
	default:
		g_assert_not_reached ();
	}
}

/**
 * gdata_youtube_service_query_standard_feed:
 * @self: a #GDataYouTubeService
 * @feed_type: the feed type to query, from #GDataYouTubeStandardFeedType
 * @query: a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service's standard @feed_type feed to build a #GDataFeed.
 *
 * Parameters and errors are as for gdata_service_query().
 *
 * Return value: a #GDataFeed of query results, or %NULL; unref with g_object_unref()
 **/
GDataFeed *
gdata_youtube_service_query_standard_feed (GDataYouTubeService *self, GDataYouTubeStandardFeedType feed_type, GDataQuery *query,
					   GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
					   GError **error)
{
	/* TODO: Support the "time" parameter, as well as category- and region-specific feeds */
	return gdata_service_query (GDATA_SERVICE (self), standard_feed_type_to_feed_uri (feed_type), query,
				    GDATA_TYPE_YOUTUBE_VIDEO, cancellable, progress_callback, progress_user_data, error);
}

/**
 * gdata_youtube_service_query_standard_feed_async:
 * @self: a #GDataService
 * @feed_type: the feed type to query, from #GDataYouTubeStandardFeedType
 * @query: a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: data to pass to the @progress_callback function
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: data to pass to the @callback function
 *
 * Queries the service's standard @feed_type feed to build a #GDataFeed. @self and
 * @query are both reffed when this function is called, so can safely be freed after this function returns.
 *
 * For more details, see gdata_youtube_service_query_standard_feed(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_service_query_finish()
 * to get the results of the operation.
 **/
void
gdata_youtube_service_query_standard_feed_async (GDataYouTubeService *self, GDataYouTubeStandardFeedType feed_type, GDataQuery *query,
						 GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
						 GAsyncReadyCallback callback, gpointer user_data)
{
	gdata_service_query_async (GDATA_SERVICE (self), standard_feed_type_to_feed_uri (feed_type), query,
				   GDATA_TYPE_YOUTUBE_VIDEO, cancellable, progress_callback, progress_user_data, callback, user_data);
}

/**
 * gdata_youtube_service_query_videos:
 * @self: a #GDataYouTubeService
 * @query: a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service for videos matching the parameters set on the #GDataQuery. This searches site-wide, and imposes no other restrictions or
 * parameters on the query.
 *
 * Parameters and errors are as for gdata_service_query().
 *
 * Return value: a #GDataFeed of query results, or %NULL; unref with g_object_unref()
 **/
GDataFeed *
gdata_youtube_service_query_videos (GDataYouTubeService *self, GDataQuery *query,
				    GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
				    GError **error)
{
	return gdata_service_query (GDATA_SERVICE (self), "http://gdata.youtube.com/feeds/api/videos", query,
				    GDATA_TYPE_YOUTUBE_VIDEO, cancellable, progress_callback, progress_user_data, error);
}

/**
 * gdata_youtube_service_query_videos_async:
 * @self: a #GDataService
 * @query: a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: data to pass to the @progress_callback function
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: data to pass to the @callback function
 *
 * Queries the service for videos matching the parameters set on the #GDataQuery. This searches site-wide, and imposes no other restrictions or
 * parameters on the query. @self and @query are both reffed when this function is called, so can safely be freed after this function returns.
 *
 * For more details, see gdata_youtube_service_query_videos(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_service_query_finish()
 * to get the results of the operation.
 **/
void
gdata_youtube_service_query_videos_async (GDataYouTubeService *self, GDataQuery *query,
					  GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
					  GAsyncReadyCallback callback, gpointer user_data)
{
	gdata_service_query_async (GDATA_SERVICE (self), "http://gdata.youtube.com/feeds/api/videos", query,
				   GDATA_TYPE_YOUTUBE_VIDEO, cancellable, progress_callback, progress_user_data, callback, user_data);
}

/**
 * gdata_youtube_service_query_related:
 * @self: a #GDataYouTubeService
 * @video: a #GDataYouTubeVideo for which to find related videos
 * @query: a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service for videos related to @video. The algorithm determining which videos are related is on the server side.
 *
 * If @video does not have a link with rel value <literal>http://gdata.youtube.com/schemas/2007#video.related</literal>, a
 * %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be thrown. Parameters and other errors are as for gdata_service_query().
 *
 * Return value: a #GDataFeed of query results; unref with g_object_unref()
 **/
GDataFeed *
gdata_youtube_service_query_related (GDataYouTubeService *self, GDataYouTubeVideo *video, GDataQuery *query,
				     GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
				     GError **error)
{
	GDataLink *related_link;

	/* See if the video already has a rel="http://gdata.youtube.com/schemas/2007#video.related" link */
	related_link = gdata_entry_look_up_link (GDATA_ENTRY (video), "http://gdata.youtube.com/schemas/2007#video.related");
	if (related_link == NULL) {
		/* Erroring out is probably the safest thing to do */
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
				     _("The video did not have a related videos <link>."));
		return NULL;
	}

	/* Execute the query */
	return gdata_service_query (GDATA_SERVICE (self), gdata_link_get_uri (related_link), query,
				    GDATA_TYPE_YOUTUBE_VIDEO, cancellable, progress_callback, progress_user_data, error);
}

/**
 * gdata_youtube_service_query_related_async:
 * @self: a #GDataService
 * @video: a #GDataYouTubeVideo for which to find related videos
 * @query: a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: data to pass to the @progress_callback function
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: data to pass to the @callback function
 *
 * Queries the service for videos related to @video. The algorithm determining which videos are related is on the server side.
 * @self and @query are both reffed when this function is called, so can safely be freed after this function returns.
 *
 * For more details, see gdata_youtube_service_query_related(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_service_query_finish()
 * to get the results of the operation.
 **/
void
gdata_youtube_service_query_related_async (GDataYouTubeService *self, GDataYouTubeVideo *video, GDataQuery *query,
					   GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
					   GAsyncReadyCallback callback, gpointer user_data)
{
	GDataLink *related_link;

	/* See if the video already has a rel="http://gdata.youtube.com/schemas/2007#video.related" link */
	related_link = gdata_entry_look_up_link (GDATA_ENTRY (video), "http://gdata.youtube.com/schemas/2007#video.related");
	if (related_link == NULL) {
		/* Erroring out is probably the safest thing to do */
		g_simple_async_report_error_in_idle (G_OBJECT (self), callback, user_data,
						     GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
						     _("The video did not have a related videos <link>."));
		return;
	}

	gdata_service_query_async (GDATA_SERVICE (self), gdata_link_get_uri (related_link), query,
				   GDATA_TYPE_YOUTUBE_VIDEO, cancellable, progress_callback, progress_user_data, callback, user_data);
}

/**
 * gdata_youtube_service_upload_video:
 * @self: a #GDataYouTubeService
 * @video: a #GDataYouTubeVideo to insert
 * @video_file: the video file to upload
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Uploads a video to YouTube, using the properties from @video and the video file pointed to by @video_file.
 *
 * If @video has already been inserted, a %GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED error will be returned. If no user is authenticated
 * with the service, %GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED will be returned.
 *
 * If there is a problem reading @video_file, an error from g_file_load_contents() or g_file_query_info() will be returned. Other errors from
 * #GDataServiceError can be returned for other exceptional conditions, as determined by the server.
 *
 * Return value: the inserted #GDataYouTubeVideo with updated properties from @video; unref with g_object_unref()
 **/
GDataYouTubeVideo *
gdata_youtube_service_upload_video (GDataYouTubeService *self, GDataYouTubeVideo *video, GFile *video_file, GCancellable *cancellable, GError **error)
{
	/* TODO: Async variant */
	GDataYouTubeVideo *new_entry;
	GDataCategory *category;
	GOutputStream *output_stream;
	GInputStream *input_stream;
	const gchar *slug, *content_type, *response_body;
	gssize response_length;
	GFileInfo *file_info;
	GError *child_error = NULL;

	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (video), NULL);
	g_return_val_if_fail (G_IS_FILE (video_file), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (gdata_entry_is_inserted (GDATA_ENTRY (video)) == TRUE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED,
		                     _("The entry has already been inserted."));
		return NULL;
	}

	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to upload a video."));
		return NULL;
	}

	/* Add the "video" kind if the entry is missing it. If it already has the kind category, no duplicate is added. */
	category = gdata_category_new ("http://gdata.youtube.com/schemas/2007#video", "http://schemas.google.com/g/2005#kind", NULL);
	gdata_entry_add_category (GDATA_ENTRY (video), category);
	g_object_unref (category);

	file_info = g_file_query_info (video_file, "standard::display-name,standard::content-type", G_FILE_QUERY_INFO_NONE, NULL, error);
	if (file_info == NULL)
		return NULL;

	slug = g_file_info_get_display_name (file_info);
	content_type = g_file_info_get_content_type (file_info);

	/* Streaming upload support using GDataUploadStream; automatically handles the XML and multipart stuff for us */
	output_stream = gdata_upload_stream_new (GDATA_SERVICE (self), SOUP_METHOD_POST,
	                                         "http://uploads.gdata.youtube.com/feeds/api/users/default/uploads",
	                                         GDATA_ENTRY (video), slug, content_type);

	g_object_unref (file_info);
	if (output_stream == NULL)
		return NULL;

	/* Open the video file for reading */
	input_stream = G_INPUT_STREAM (g_file_read (video_file, cancellable, error));
	if (input_stream == NULL) {
		g_object_unref (output_stream);
		return NULL;
	}

	/* Splice the streams to upload the file and metadata */
	g_output_stream_splice (output_stream, input_stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
	                        cancellable, &child_error);

	g_object_unref (input_stream);
	if (child_error != NULL) {
		g_object_unref (output_stream);
		g_propagate_error (error, child_error);
		return NULL;
	}

	/* Get and parse the response from the server */
	response_body = gdata_upload_stream_get_response (GDATA_UPLOAD_STREAM (output_stream), &response_length);
	g_assert (response_body != NULL && response_length > 0);

	new_entry = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO, response_body, (gint) response_length, error));
	g_object_unref (output_stream);

	return new_entry;
}

/**
 * gdata_youtube_service_get_developer_key:
 * @self: a #GDataYouTubeService
 *
 * Gets the #GDataYouTubeService:developer-key property from the #GDataYouTubeService.
 *
 * Return value: the developer key property
 **/
const gchar *
gdata_youtube_service_get_developer_key (GDataYouTubeService *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	return self->priv->developer_key;
}

/**
 * gdata_youtube_service_get_youtube_user:
 * @self: a #GDataYouTubeService
 *
 * Gets the #GDataYouTubeService:youtube-user property from the #GDataYouTubeService.
 *
 * Return value: the YouTube username property
 **/
const gchar *
gdata_youtube_service_get_youtube_user (GDataYouTubeService *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	return self->priv->youtube_user;
}
