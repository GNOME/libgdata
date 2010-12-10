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
 *
 * <example>
 * 	<title>Getting a Localized List of YouTube Categories</title>
 * 	<programlisting>
 *	GDataYouTubeService *service;
 *	GDataAPPCategories *app_categories;
 *	GList *categories, *i;
 *
 *	/<!-- -->* Create a service and set its locale to Italian, which localizes the categories to Italian *<!-- -->/
 *	service = create_youtube_service ();
 *	gdata_service_set_locale (GDATA_SERVICE (service), "it");
 *
 *	/<!-- -->* Query the server for the current list of YouTube categories (in Italian) *<!-- -->/
 *	app_categories = gdata_youtube_service_get_categories (service, NULL, NULL);
 *	categories = gdata_app_categories_get_categories (app_categories);
 *
 *	/<!-- -->* Iterate through the categories *<!-- -->/
 *	for (i = categories; i != NULL; i = i->next) {
 *		GDataYouTubeCategory *category = GDATA_YOUTUBE_CATEGORY (i->data);
 *
 *		if (gdata_youtube_category_is_deprecated (category) == FALSE && gdata_youtube_category_is_browsable (category, "IT") == TRUE) {
 *			/<!-- -->* Do something with the category here, as it's not deprecated, and is browsable in the given region *<!-- -->/
 *			add_to_ui (gdata_category_get_term (GDATA_CATEGORY (category)), gdata_category_get_label (GDATA_CATEGORY (category)));
 *		}
 *	}
 *
 *	g_object_unref (app_categories);
 *	g_object_unref (service);
 * 	</programlisting>
 * </example>
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
#include "gdata-youtube-category.h"
#include "gdata-batchable.h"

/* Standards reference here: http://code.google.com/apis/youtube/2.0/reference.html */

GQuark
gdata_youtube_service_error_quark (void)
{
	return g_quark_from_static_string ("gdata-youtube-service-error-quark");
}

static void gdata_youtube_service_finalize (GObject *object);
static void gdata_youtube_service_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_youtube_service_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void append_query_headers (GDataService *self, SoupMessage *message);
static void parse_error_response (GDataService *self, GDataOperationType operation_type, guint status, const gchar *reason_phrase,
                                  const gchar *response_body, gint length, GError **error);

struct _GDataYouTubeServicePrivate {
	gchar *developer_key;
};

enum {
	PROP_DEVELOPER_KEY = 1
};

G_DEFINE_TYPE_WITH_CODE (GDataYouTubeService, gdata_youtube_service, GDATA_TYPE_SERVICE, G_IMPLEMENT_INTERFACE (GDATA_TYPE_BATCHABLE, NULL))

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
		if (error == NULL || *error == NULL) {
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
					             /* Translators: the first parameter is an error code, which is a coded string.
					              * The second parameter is an error domain, which is another coded string.
					              * The third parameter is the location of the error, which is either a URI or an XPath. */
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
	if (error != NULL && *error == NULL)
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
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: (scope call): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service's standard @feed_type feed to build a #GDataFeed.
 *
 * Parameters and errors are as for gdata_service_query().
 *
 * Return value: (transfer full): a #GDataFeed of query results, or %NULL; unref with g_object_unref()
 **/
GDataFeed *
gdata_youtube_service_query_standard_feed (GDataYouTubeService *self, GDataYouTubeStandardFeedType feed_type, GDataQuery *query,
                                           GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                           GError **error)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* TODO: Support the "time" parameter, as well as category- and region-specific feeds */
	return gdata_service_query (GDATA_SERVICE (self), standard_feed_type_to_feed_uri (feed_type), query,
	                            GDATA_TYPE_YOUTUBE_VIDEO, cancellable, progress_callback, progress_user_data, error);
}

/**
 * gdata_youtube_service_query_standard_feed_async: (skip)
 * @self: a #GDataService
 * @feed_type: the feed type to query, from #GDataYouTubeStandardFeedType
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: (closure): data to pass to the @callback function
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
	g_return_if_fail (GDATA_IS_YOUTUBE_SERVICE (self));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	gdata_service_query_async (GDATA_SERVICE (self), standard_feed_type_to_feed_uri (feed_type), query,
	                           GDATA_TYPE_YOUTUBE_VIDEO, cancellable, progress_callback, progress_user_data, callback, user_data);
}

/**
 * gdata_youtube_service_query_videos:
 * @self: a #GDataYouTubeService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: (scope call): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service for videos matching the parameters set on the #GDataQuery. This searches site-wide, and imposes no other restrictions or
 * parameters on the query.
 *
 * Parameters and errors are as for gdata_service_query().
 *
 * Return value: (transfer full): a #GDataFeed of query results, or %NULL; unref with g_object_unref()
 **/
GDataFeed *
gdata_youtube_service_query_videos (GDataYouTubeService *self, GDataQuery *query,
                                    GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                    GError **error)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return gdata_service_query (GDATA_SERVICE (self), "http://gdata.youtube.com/feeds/api/videos", query,
	                            GDATA_TYPE_YOUTUBE_VIDEO, cancellable, progress_callback, progress_user_data, error);
}

/**
 * gdata_youtube_service_query_videos_async: (skip)
 * @self: a #GDataService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: (closure): data to pass to the @callback function
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
	g_return_if_fail (GDATA_IS_YOUTUBE_SERVICE (self));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	gdata_service_query_async (GDATA_SERVICE (self), "http://gdata.youtube.com/feeds/api/videos", query,
	                           GDATA_TYPE_YOUTUBE_VIDEO, cancellable, progress_callback, progress_user_data, callback, user_data);
}

/**
 * gdata_youtube_service_query_related:
 * @self: a #GDataYouTubeService
 * @video: a #GDataYouTubeVideo for which to find related videos
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: (scope call): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service for videos related to @video. The algorithm determining which videos are related is on the server side.
 *
 * If @video does not have a link with rel value <literal>http://gdata.youtube.com/schemas/2007#video.related</literal>, a
 * %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be thrown. Parameters and other errors are as for gdata_service_query().
 *
 * Return value: (transfer full): a #GDataFeed of query results; unref with g_object_unref()
 **/
GDataFeed *
gdata_youtube_service_query_related (GDataYouTubeService *self, GDataYouTubeVideo *video, GDataQuery *query,
                                     GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                     GError **error)
{
	GDataLink *related_link;

	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (video), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

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
 * gdata_youtube_service_query_related_async: (skip)
 * @self: a #GDataService
 * @video: a #GDataYouTubeVideo for which to find related videos
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: (closure): data to pass to the @callback function
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

	g_return_if_fail (GDATA_IS_YOUTUBE_SERVICE (self));
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (video));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

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

static GOutputStream *
get_file_output_stream (GDataYouTubeService *self, GDataYouTubeVideo *video_entry, GFile *video_data, GError **error)
{
	GFileInfo *file_info = NULL;
	const gchar *slug = NULL, *content_type = NULL;
	GOutputStream *output_stream;

	file_info = g_file_query_info (video_data, "standard::display-name,standard::content-type", G_FILE_QUERY_INFO_NONE, NULL, error);
	if (file_info == NULL)
		return NULL;

	slug = g_file_info_get_display_name (file_info);
	content_type = g_file_info_get_content_type (file_info);

	/* Streaming upload support using GDataUploadStream; automatically handles the XML and multipart stuff for us */
	output_stream = gdata_upload_stream_new (GDATA_SERVICE (self), SOUP_METHOD_POST,
	                                         "http://uploads.gdata.youtube.com/feeds/api/users/default/uploads",
	                                         GDATA_ENTRY (video_entry), slug, content_type);
	g_object_unref (file_info);

	return output_stream;
}

static GDataYouTubeVideo *
parse_spliced_stream (GOutputStream *output_stream, GError **error)
{
	const gchar *response_body;
	gssize response_length;
	GDataYouTubeVideo *new_entry;

	/* Get the response from the server */
	response_body = gdata_upload_stream_get_response (GDATA_UPLOAD_STREAM (output_stream), &response_length);
	g_assert (response_body != NULL && response_length > 0);

	/* Parse the response to produce a GDataPicasaWebFile */
	new_entry = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO, response_body, (gint) response_length, error));

	return new_entry;
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
 * Return value: (transfer full): the inserted #GDataYouTubeVideo with updated properties from @video; unref with g_object_unref()
 **/
GDataYouTubeVideo *
gdata_youtube_service_upload_video (GDataYouTubeService *self, GDataYouTubeVideo *video, GFile *video_file, GCancellable *cancellable, GError **error)
{
	GDataYouTubeVideo *new_entry;
	GOutputStream *output_stream;
	GInputStream *input_stream;
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

	output_stream = get_file_output_stream (self, video, video_file, error);
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

	new_entry = parse_spliced_stream (output_stream, error);
	g_object_unref (output_stream);

	return new_entry;
}

/**
 * gdata_youtube_service_upload_video_finish:
 * @self: a #GDataYouTubeService
 * @result: a #GSimpleAsyncResult
 * @error: a #GError, or %NULL
 *
 * This should be called to obtain the result of a call to gdata_youtube_service_upload_video_async() and to check for errors.
 *
 * If there is a problem reading the subect file's data, an error from g_output_stream_splice() or g_file_query_info() will be returned. Other errors
 * from #GDataServiceError can be returned for other exceptional conditions, as determined by the server.
 *
 * If the video to upload has already been inserted, a %GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED error will be set. If no user is authenticated with
 * the service when trying to upload it, %GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED will be set.
 *
 * Return value: (transfer full): the inserted #GDataYouTubeVideo; unref with g_object_unref()
 *
 * Since: 0.8.0
 */
GDataYouTubeVideo *
gdata_youtube_service_upload_video_finish (GDataYouTubeService *self, GAsyncResult *result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error))
		return NULL;

	g_assert (gdata_youtube_service_upload_video_async == g_simple_async_result_get_source_tag (G_SIMPLE_ASYNC_RESULT (result)));

	return GDATA_YOUTUBE_VIDEO (g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result)));
}

typedef struct {
	GDataYouTubeService *service;
	GAsyncReadyCallback callback;
	gpointer user_data;
} UploadVideoAsyncData;

static void
upload_video_async_data_free (UploadVideoAsyncData *data)
{
	g_object_unref (data->service);
	g_slice_free (UploadVideoAsyncData, data);
}

static void
upload_video_async_cb (GOutputStream *output_stream, GAsyncResult *result, UploadVideoAsyncData *data)
{
	GError *error = NULL;
	GDataYouTubeVideo *video = NULL;
	GSimpleAsyncResult *async_result;

	g_output_stream_splice_finish (output_stream, result, &error);

	/* If we're error free, parse the file from the stream */
	if (error == NULL)
		video = parse_spliced_stream (output_stream, &error);

	if (error == NULL && video != NULL) {
		async_result = g_simple_async_result_new (G_OBJECT (data->service), (GAsyncReadyCallback) data->callback,
		                                          data->user_data, gdata_youtube_service_upload_video_async);
	} else {
		async_result = g_simple_async_result_new_from_error (G_OBJECT (data->service), (GAsyncReadyCallback) data->callback,
		                                                     data->user_data, error);
	}

	g_simple_async_result_set_op_res_gpointer (async_result, video, NULL);

	g_simple_async_result_complete (async_result);

	upload_video_async_data_free (data);
}

/**
 * gdata_youtube_service_upload_video_async:
 * @self: a #GDataYouTubeService
 * @video_entry: a #GDataYouTubeVideo to insert
 * @video_data: the actual file to upload
 * @cancellable: optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the upload is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Uploads a video to YouTube asynchronously, using the @video_data from disk and the metadata from @video_entry. A user must be authenticated to use
 * this function. Note that uploaded videos aren't publicly visible on YouTube immediately; they need to go through a moderation queue first.
 *
 * @callback should call gdata_youtube_service_upload_video_finish() to obtain a #GDataYouTubeVideo representing the uploaded video and check for
 * possible errors.
 *
 * Since: 0.8.0
 **/
void
gdata_youtube_service_upload_video_async (GDataYouTubeService *self, GDataYouTubeVideo *video_entry, GFile *video_data, GCancellable *cancellable,
                                          GAsyncReadyCallback callback, gpointer user_data)
{
	GOutputStream *output_stream;
	GInputStream *input_stream;
	UploadVideoAsyncData *data;
	GSimpleAsyncResult *result;
	GError *error = NULL;

	g_return_if_fail (GDATA_IS_YOUTUBE_SERVICE (self));
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (video_entry));
	g_return_if_fail (G_IS_FILE (video_data));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	if (gdata_entry_is_inserted (GDATA_ENTRY (video_entry)) == TRUE) {
		g_set_error_literal (&error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED,
		                     _("The entry has already been inserted."));
		goto error;
	}

	if (gdata_service_is_authenticated (GDATA_SERVICE (self)) == FALSE) {
		g_set_error_literal (&error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to upload a video."));
		goto error;
	}

	/* Prepare and retrieve a #GDataOutputStream for the file and its data */
	output_stream = get_file_output_stream (self, video_entry, video_data, &error);
	if (output_stream == NULL)
		goto error;

	/* Pipe the input file to the upload stream */
	input_stream = G_INPUT_STREAM (g_file_read (video_data, cancellable, &error));
	if (input_stream == NULL) {
		g_object_unref (output_stream);
		goto error;
	}

	data = g_slice_new (UploadVideoAsyncData);
	data->service = g_object_ref (self);
	data->callback = callback;
	data->user_data = user_data;

	/* Actually transfer the data */
	g_output_stream_splice_async (output_stream, input_stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
	                              0, cancellable, (GAsyncReadyCallback) upload_video_async_cb, data);

	g_object_unref (input_stream);
	g_object_unref (output_stream);

	return;

error:
	result = g_simple_async_result_new_from_error (G_OBJECT (self), callback, user_data, error);
	g_simple_async_result_complete (result);
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
 * gdata_youtube_service_get_categories:
 * @self: a #GDataYouTubeService
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Gets a list of the categories currently in use on YouTube. The returned #GDataAPPCategories contains a list of #GDataYouTubeCategory<!-- -->s which
 * enumerate the current YouTube categories.
 *
 * The category labels (#GDataCategory:label) are localised based on the value of #GDataService:locale.
 *
 * Return value: (transfer full): a #GDataAPPCategories, or %NULL; unref with g_object_unref()
 *
 * Since: 0.7.0
 **/
GDataAPPCategories *
gdata_youtube_service_get_categories (GDataYouTubeService *self, GCancellable *cancellable, GError **error)
{
	SoupMessage *message;
	GDataAPPCategories *categories;

	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Download the category list. Note that this is (service) locale-dependent. */
	message = _gdata_service_query (GDATA_SERVICE (self), "http://gdata.youtube.com/schemas/2007/categories.cat", NULL, cancellable, error);
	if (message == NULL)
		return NULL;

	g_assert (message->response_body->data != NULL);
	categories = GDATA_APP_CATEGORIES (_gdata_parsable_new_from_xml (GDATA_TYPE_APP_CATEGORIES, message->response_body->data,
	                                                                 message->response_body->length,
	                                                                 GSIZE_TO_POINTER (GDATA_TYPE_YOUTUBE_CATEGORY), error));
	g_object_unref (message);

	return categories;
}

static void
get_categories_thread (GSimpleAsyncResult *result, GDataYouTubeService *service, GCancellable *cancellable)
{
	GDataAPPCategories *categories;
	GError *error = NULL;

	/* Get the categories and return */
	categories = gdata_youtube_service_get_categories (service, cancellable, &error);
	if (error != NULL) {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
	}

	g_simple_async_result_set_op_res_gpointer (result, categories, (GDestroyNotify) g_object_unref);
}

/**
 * gdata_youtube_service_get_categories_async:
 * @self: a #GDataYouTubeService
 * @cancellable: optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the request is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Gets a list of the categories currently in use on YouTube. @self is reffed when this function is called, so can safely be unreffed after this
 * function returns.
 *
 * For more details, see gdata_youtube_service_get_categories(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_youtube_service_get_categories_finish() to get the results of the
 * operation.
 *
 * Since: 0.7.0
 **/
void
gdata_youtube_service_get_categories_async (GDataYouTubeService *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	GSimpleAsyncResult *result;

	g_return_if_fail (GDATA_IS_YOUTUBE_SERVICE (self));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	result = g_simple_async_result_new (G_OBJECT (self), callback, user_data, gdata_youtube_service_get_categories_async);
	g_simple_async_result_run_in_thread (result, (GSimpleAsyncThreadFunc) get_categories_thread, G_PRIORITY_DEFAULT, cancellable);
	g_object_unref (result);
}

/**
 * gdata_youtube_service_get_categories_finish:
 * @self: a #GDataYouTubeService
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous request for a list of categories on YouTube, as started with gdata_youtube_service_get_categories_async().
 *
 * Return value: (transfer full): a #GDataAPPCategories, or %NULL; unref with g_object_unref()
 *
 * Since: 0.7.0
 **/
GDataAPPCategories *
gdata_youtube_service_get_categories_finish (GDataYouTubeService *self, GAsyncResult *async_result, GError **error)
{
	GSimpleAsyncResult *result = G_SIMPLE_ASYNC_RESULT (async_result);
	GDataAPPCategories *categories;

	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	g_warn_if_fail (g_simple_async_result_get_source_tag (result) == gdata_youtube_service_get_categories_async);

	if (g_simple_async_result_propagate_error (result, error) == TRUE)
		return NULL;

	categories = g_simple_async_result_get_op_res_gpointer (result);
	if (categories != NULL)
		return g_object_ref (categories);
	return NULL;
}
