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
 * SECTION:gdata-youtube-service
 * @short_description: GData YouTube service object
 * @stability: Stable
 * @include: gdata/services/youtube/gdata-youtube-service.h
 *
 * #GDataYouTubeService is a subclass of #GDataService for communicating with the GData API of YouTube. It supports querying for and
 * uploading videos using version 3 of the API.
 *
 * The YouTube API supports returning different sets of properties for
 * #GDataYouTubeVideos depending on the specific query. For search results, only
 * ‘snippet’ properties are returned (including #GDataEntry:title,
 * #GDataEntry:summary and the set of thumbnails). For querying single videos,
 * a more complete set of properties are returned — so use
 * gdata_service_query_single_entry_async() to get further details on a video.
 *
 * For more details of YouTube's GData API, see the <ulink type="http" url="https://developers.google.com/youtube/v3/docs/">
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
 *
 * <example>
 * 	<title>Uploading a Video from Disk</title>
 * 	<programlisting>
 *	GDataYouTubeService *service;
 *	GDataYouTubeVideo *video, *uploaded_video;
 *	GDataMediaCategory *category;
 *	const gchar * const tags[] = { "tag1", "tag2", NULL };
 *	GFile *video_file;
 *	GFileInfo *file_info;
 *	const gchar *slug, *content_type;
 *	GFileInputStream *file_stream;
 *	GDataUploadStream *upload_stream;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service *<!-- -->/
 *	service = create_youtube_service ();
 *
 *	/<!-- -->* Get the video file to upload *<!-- -->/
 *	video_file = g_file_new_for_path ("sample.ogg");
 *
 *	/<!-- -->* Get the file's display name and content type *<!-- -->/
 *	file_info = g_file_query_info (video_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
 *	                               G_FILE_QUERY_INFO_NONE, NULL, &error);
 *
 *	if (error != NULL) {
 *		g_error ("Error getting video file information: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (video_file);
 *		g_object_unref (service);
 *		return;
 *	}
 *
 *	slug = g_file_info_get_display_name (file_info);
 *	content_type = g_file_info_get_content_type (file_info);
 *
 *	/<!-- -->* Get an input stream for the file *<!-- -->/
 *	file_stream = g_file_read (video_file, NULL, &error);
 *
 *	g_object_unref (video_file);
 *
 *	if (error != NULL) {
 *		g_error ("Error getting video file stream: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (file_info);
 *		g_object_unref (service);
 *		return;
 *	}
 *
 *	/<!-- -->* Create the video to upload *<!-- -->/
 *	video = gdata_youtube_video_new (NULL);
 *
 *	gdata_entry_set_title (GDATA_ENTRY (video), "Video Title");
 *	gdata_youtube_video_set_description (video, "Video description.");
 *	gdata_youtube_video_set_keywords (video, video_tags);
 *
 *	category = gdata_media_category_new ("People", "http://gdata.youtube.com/schemas/2007/categories.cat", NULL);
 *	gdata_youtube_video_set_category (video, category);
 *	g_object_unref (category);
 *
 *	/<!-- -->* Get an upload stream for the video *<!-- -->/
 *	upload_stream = gdata_youtube_service_upload_video (service, video, slug, content_type, NULL, &error);
 *
 *	g_object_unref (video);
 *	g_object_unref (file_info);
 *
 *	if (error != NULL) {
 *		g_error ("Error getting upload stream: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (file_stream);
 *		g_object_unref (service);
 *		return;
 *	}
 *
 *	/<!-- -->* Upload the video. This is a blocking operation, and should normally be done asynchronously. *<!-- -->/
 *	g_output_stream_splice (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (file_stream),
 *	                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &error);
 *
 *	g_object_unref (file_stream);
 *
 *	if (error != NULL) {
 *		g_error ("Error splicing streams: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (upload_stream);
 *		g_object_unref (service);
 *		return;
 *	}
 *
 *	/<!-- -->* Finish off the upload by parsing the returned updated video entry *<!-- -->/
 *	uploaded_video = gdata_youtube_service_finish_video_upload (service, upload_stream, &error);
 *
 *	g_object_unref (upload_stream);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error uploading video: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	/<!-- -->* Do something with the uploaded video *<!-- -->/
 *
 *	g_object_unref (uploaded_video);
 * 	</programlisting>
 * </example>
 *
 * <example>
 * 	<title>Querying for Videos from a Standard Feed</title>
 * 	<programlisting>
 *	GDataYouTubeService *service;
 *	GDataFeed *feed;
 *	GList *i;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service *<!-- -->/
 *	service = create_youtube_service ();
 *
 *	/<!-- -->* Query for the top page of videos in the most popular feed *<!-- -->/
 *	feed = gdata_youtube_service_query_standard_feed (service, GDATA_YOUTUBE_MOST_POPULAR_FEED, NULL, NULL, NULL, NULL, &error);
 *
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error querying for most popular videos: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	/<!-- -->* Iterate through the videos *<!-- -->/
 *	for (i = gdata_feed_get_entries (feed); i != NULL; i = i->next) {
 *		GDataYouTubeVideo *video = GDATA_YOUTUBE_VIDEO (i->data);
 *
 *		/<!-- -->* Do something with the video, like insert it into the UI *<!-- -->/
 *	}
 *
 *	g_object_unref (feed);
 * 	</programlisting>
 * </example>
 *
 * <example>
 * 	<title>Querying for Videos using Search Terms</title>
 * 	<programlisting>
 *	GDataYouTubeService *service;
 *	GDataYouTubeQuery *query;
 *	GDataFeed *feed;
 *	GList *i;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service *<!-- -->/
 *	service = create_youtube_service ();
 *
 *	/<!-- -->* Build a query with the given search terms, also matching only videos which are CC-licensed *<!-- -->/
 *	query = gdata_youtube_query_new (my_space_separated_search_terms);
 *	gdata_youtube_query_set_license (query, GDATA_YOUTUBE_LICENSE_CC);
 *
 *	/<!-- -->* Query for the videos matching the query parameters *<!-- -->/
 *	feed = gdata_youtube_service_query_videos (service, query, NULL, NULL, NULL, &error);
 *
 *	g_object_unref (query);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error querying for videos matching search terms ‘%s’: %s", my_space_separated_search_terms, error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	/<!-- -->* Iterate through the videos *<!-- -->/
 *	for (i = gdata_feed_get_entries (feed); i != NULL; i = i->next) {
 *		GDataYouTubeVideo *video = GDATA_YOUTUBE_VIDEO (i->data);
 *
 *		/<!-- -->* Do something with the video, like insert it into the UI *<!-- -->/
 *	}
 *
 *	g_object_unref (feed);
 * 	</programlisting>
 * </example>
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libsoup/soup.h>
#include <string.h>

#include "gdata-youtube-service.h"
#include "gdata-youtube-feed.h"
#include "gdata-service.h"
#include "gdata-private.h"
#include "gdata-parser.h"
#include "atom/gdata-link.h"
#include "gdata-upload-stream.h"
#include "gdata-youtube-category.h"
#include "gdata-batchable.h"

/* Standards reference here: https://developers.google.com/youtube/v3/docs/ */

GQuark
gdata_youtube_service_error_quark (void)
{
	return g_quark_from_static_string ("gdata-youtube-service-error-quark");
}

static void gdata_youtube_service_batchable_init (GDataBatchableIface *iface);
static void gdata_youtube_service_finalize (GObject *object);
static void gdata_youtube_service_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_youtube_service_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void append_query_headers (GDataService *self, GDataAuthorizationDomain *domain, SoupMessage *message);
static void parse_error_response (GDataService *self, GDataOperationType operation_type, guint status, const gchar *reason_phrase,
                                  const gchar *response_body, gint length, GError **error);

static GList *get_authorization_domains (void);

struct _GDataYouTubeServicePrivate {
	gchar *developer_key;
};

enum {
	PROP_DEVELOPER_KEY = 1
};

/* Reference: https://developers.google.com/youtube/v3/guides/authentication */
_GDATA_DEFINE_AUTHORIZATION_DOMAIN (youtube, "youtube",
                                    "https://www.googleapis.com/auth/youtube")
_GDATA_DEFINE_AUTHORIZATION_DOMAIN (youtube_force_ssl, "youtube-force-ssl",
                                    "https://www.googleapis.com/auth/youtube.force-ssl")
G_DEFINE_TYPE_WITH_CODE (GDataYouTubeService, gdata_youtube_service, GDATA_TYPE_SERVICE,
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_BATCHABLE,
                                                gdata_youtube_service_batchable_init))

static void
gdata_youtube_service_class_init (GDataYouTubeServiceClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataServiceClass *service_class = GDATA_SERVICE_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataYouTubeServicePrivate));

	gobject_class->set_property = gdata_youtube_service_set_property;
	gobject_class->get_property = gdata_youtube_service_get_property;
	gobject_class->finalize = gdata_youtube_service_finalize;

	service_class->feed_type = GDATA_TYPE_YOUTUBE_FEED;
	service_class->append_query_headers = append_query_headers;
	service_class->parse_error_response = parse_error_response;
	service_class->get_authorization_domains = get_authorization_domains;

	/**
	 * GDataYouTubeService:developer-key:
	 *
	 * The developer key your application has registered with the YouTube API. For more information, see the <ulink type="http"
	 * url="https://developers.google.com/youtube/registering_an_application">online documentation</ulink>.
	 *
	 * With the port from v2 to v3 of the YouTube API in libgdata
	 * 0.17.0, it might be necessary to update your application’s
	 * developer key.
	 */
	g_object_class_install_property (gobject_class, PROP_DEVELOPER_KEY,
	                                 g_param_spec_string ("developer-key",
	                                                      "Developer key", "Your YouTube developer API key.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static gboolean
gdata_youtube_service_batchable_is_supported (GDataBatchOperationType operation_type)
{
	/* Batch operation support was removed with v3 of the API:
	 * https://developers.google.com/youtube/v3/guides/implementation/deprecated#Batch_Processing */
	return FALSE;
}

static void
gdata_youtube_service_batchable_init (GDataBatchableIface *iface)
{
	iface->is_supported = gdata_youtube_service_batchable_is_supported;
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
append_query_headers (GDataService *self, GDataAuthorizationDomain *domain,
                      SoupMessage *message)
{
	GDataYouTubeServicePrivate *priv = GDATA_YOUTUBE_SERVICE (self)->priv;

	g_assert (message != NULL);

	if (priv->developer_key != NULL &&
	    !gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                                get_youtube_authorization_domain ())) {
		const gchar *query;
		SoupURI *uri;

		uri = soup_message_get_uri (message);
		query = soup_uri_get_query (uri);

		/* Set the key on every unauthorised request:
		 * https://developers.google.com/youtube/v3/docs/standard_parameters#key */
		if (query != NULL) {
			GString *new_query;

			new_query = g_string_new (query);

			g_string_append (new_query, "&key=");
			g_string_append_uri_escaped (new_query,
			                             priv->developer_key, NULL,
			                             FALSE);

			soup_uri_set_query (uri, new_query->str);
			g_string_free (new_query, TRUE);
		}
	}

	/* Chain up to the parent class */
	GDATA_SERVICE_CLASS (gdata_youtube_service_parent_class)->append_query_headers (self, domain, message);
}

/* Reference: https://developers.google.com/youtube/v3/docs/errors
 *
 * Example response:
 *     {
 *      "error": {
 *       "errors": [
 *        {
 *         "domain": "youtube.parameter",
 *         "reason": "missingRequiredParameter",
 *         "message": "No filter selected.",
 *         "locationType": "parameter",
 *         "location": ""
 *        }
 *       ],
 *       "code": 400,
 *       "message": "No filter selected."
 *      }
 *     }
 */
/* FIXME: Factor this out into a common JSON error parser helper which simply
 * takes a map of expected error codes. */
static void
parse_error_response (GDataService *self, GDataOperationType operation_type,
                      guint status, const gchar *reason_phrase,
                      const gchar *response_body, gint length, GError **error)
{
	JsonParser *parser = NULL;  /* owned */
	JsonReader *reader = NULL;  /* owned */
	gint i;
	GError *child_error = NULL;

	if (response_body == NULL) {
		goto parent;
	}

	if (length == -1) {
		length = strlen (response_body);
	}

	parser = json_parser_new ();
	if (!json_parser_load_from_data (parser, response_body, length,
	                                 &child_error)) {
		goto parent;
	}

	reader = json_reader_new (json_parser_get_root (parser));

	/* Check that the outermost node is an object. */
	if (!json_reader_is_object (reader)) {
		goto parent;
	}

	/* Grab the ‘error’ member, then its ‘errors’ member. */
	if (!json_reader_read_member (reader, "error") ||
	    !json_reader_is_object (reader) ||
	    !json_reader_read_member (reader, "errors") ||
	    !json_reader_is_array (reader)) {
		goto parent;
	}

	/* Parse each of the errors. Return the first one, and print out any
	 * others. */
	for (i = 0; i < json_reader_count_elements (reader); i++) {
		const gchar *domain, *reason, *message, *extended_help;
		const gchar *location_type, *location;

		/* Parse the error. */
		if (!json_reader_read_element (reader, i) ||
		    !json_reader_is_object (reader)) {
			goto parent;
		}

		json_reader_read_member (reader, "domain");
		domain = json_reader_get_string_value (reader);
		json_reader_end_member (reader);

		json_reader_read_member (reader, "reason");
		reason = json_reader_get_string_value (reader);
		json_reader_end_member (reader);

		json_reader_read_member (reader, "message");
		message = json_reader_get_string_value (reader);
		json_reader_end_member (reader);

		json_reader_read_member (reader, "extendedHelp");
		extended_help = json_reader_get_string_value (reader);
		json_reader_end_member (reader);

		json_reader_read_member (reader, "locationType");
		location_type = json_reader_get_string_value (reader);
		json_reader_end_member (reader);

		json_reader_read_member (reader, "location");
		location = json_reader_get_string_value (reader);
		json_reader_end_member (reader);

		/* End the error element. */
		json_reader_end_element (reader);

		/* Create an error message, but only for the first error */
		if (error == NULL || *error == NULL) {
			if (g_strcmp0 (domain, "usageLimits") == 0 &&
			    g_strcmp0 (reason,
			               "dailyLimitExceededUnreg") == 0) {
				/* Daily Limit for Unauthenticated Use
				 * Exceeded. */
				g_set_error (error, GDATA_SERVICE_ERROR,
				             GDATA_SERVICE_ERROR_API_QUOTA_EXCEEDED,
				             _("You have made too many API "
				               "calls recently. Please wait a "
				               "few minutes and try again."));
			} else if (g_strcmp0 (reason,
			                      "rateLimitExceeded") == 0) {
				g_set_error (error, GDATA_YOUTUBE_SERVICE_ERROR,
				             GDATA_YOUTUBE_SERVICE_ERROR_ENTRY_QUOTA_EXCEEDED,
				             _("You have exceeded your entry "
				               "quota. Please delete some "
				               "entries and try again."));
			} else if (g_strcmp0 (domain, "global") == 0 &&
			           (g_strcmp0 (reason, "authError") == 0 ||
			            g_strcmp0 (reason, "required") == 0)) {
				/* Authentication problem */
				g_set_error (error, GDATA_SERVICE_ERROR,
				             GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
				             _("You must be authenticated to "
				               "do this."));
			} else if (g_strcmp0 (reason,
			                      "youtubeSignupRequired") == 0) {
				/* Tried to authenticate with a Google Account which hasn't yet had a YouTube channel created for it. */
				g_set_error (error, GDATA_YOUTUBE_SERVICE_ERROR,
				             GDATA_YOUTUBE_SERVICE_ERROR_CHANNEL_REQUIRED,
				             /* Translators: the parameter is a URI. */
				             _("Your Google Account must be "
				               "associated with a YouTube "
				               "channel to do this. Visit %s "
				               "to create one."),
				             "https://www.youtube.com/create_channel");
			} else {
				/* Unknown or validation (protocol) error. Fall
				 * back to working off the HTTP status code. */
				g_warning ("Unknown error code ‘%s’ in domain "
				           "‘%s’ received with location type "
				           "‘%s’, location ‘%s’, extended help "
				           "‘%s’ and message ‘%s’.",
				           reason, domain, location_type,
				           location, extended_help, message);

				goto parent;
			}
		} else {
			/* For all errors after the first, log the error in the
			 * terminal. */
			g_debug ("Error message received in response: domain "
			         "‘%s’, reason ‘%s’, extended help ‘%s’, "
			         "message ‘%s’, location type ‘%s’, location "
			         "‘%s’.",
			         domain, reason, extended_help, message,
			         location_type, location);
		}
	}

	/* End the ‘errors’ and ‘error’ members. */
	json_reader_end_element (reader);
	json_reader_end_element (reader);

	g_clear_object (&reader);
	g_clear_object (&parser);

	/* Ensure we’ve actually set an error message. */
	g_assert (error == NULL || *error != NULL);

	return;

parent:
	g_clear_object (&reader);
	g_clear_object (&parser);

	/* Chain up to the parent class */
	GDATA_SERVICE_CLASS (gdata_youtube_service_parent_class)->parse_error_response (self, operation_type, status, reason_phrase,
	                                                                                response_body, length, error);
}

static GList *
get_authorization_domains (void)
{
	GList *authorization_domains = NULL;

	authorization_domains = g_list_prepend (authorization_domains, get_youtube_authorization_domain ());
	authorization_domains = g_list_prepend (authorization_domains, get_youtube_force_ssl_authorization_domain ());

	return authorization_domains;
}

/**
 * gdata_youtube_service_new:
 * @developer_key: your application's developer API key
 * @authorizer: (allow-none): a #GDataAuthorizer to authorize the service's requests, or %NULL
 *
 * Creates a new #GDataYouTubeService using the given #GDataAuthorizer. If @authorizer is %NULL, all requests are made as an unauthenticated user.
 * The @developer_key must be unique for your application, and as
 * <ulink type="http" url="https://developers.google.com/youtube/registering_an_application">registered with Google</ulink>.
 *
 * Return value: a new #GDataYouTubeService, or %NULL; unref with g_object_unref()
 *
 * Since: 0.9.0
 */
GDataYouTubeService *
gdata_youtube_service_new (const gchar *developer_key, GDataAuthorizer *authorizer)
{
	g_return_val_if_fail (developer_key != NULL, NULL);
	g_return_val_if_fail (authorizer == NULL || GDATA_IS_AUTHORIZER (authorizer), NULL);

	return g_object_new (GDATA_TYPE_YOUTUBE_SERVICE,
	                     "developer-key", developer_key,
	                     "authorizer", authorizer,
	                     NULL);
}

/**
 * gdata_youtube_service_get_primary_authorization_domain:
 *
 * The primary #GDataAuthorizationDomain for interacting with YouTube. This will not normally need to be used, as it's used internally
 * by the #GDataYouTubeService methods. However, if using the plain #GDataService methods to implement custom queries or requests which libgdata
 * does not support natively, then this domain may be needed to authorize the requests.
 *
 * The domain never changes, and is interned so that pointer comparison can be used to differentiate it from other authorization domains.
 *
 * Return value: (transfer none): the service's authorization domain
 *
 * Since: 0.9.0
 */
GDataAuthorizationDomain *
gdata_youtube_service_get_primary_authorization_domain (void)
{
	return get_youtube_authorization_domain ();
}

static gchar *
standard_feed_type_to_feed_uri (GDataYouTubeStandardFeedType feed_type)
{
	switch (feed_type) {
	case GDATA_YOUTUBE_MOST_POPULAR_FEED:
		return _gdata_service_build_uri ("https://www.googleapis.com/youtube/v3/videos"
		                                 "?part=snippet"
		                                 "&chart=mostPopular");
	case GDATA_YOUTUBE_TOP_RATED_FEED:
	case GDATA_YOUTUBE_TOP_FAVORITES_FEED:
	case GDATA_YOUTUBE_MOST_VIEWED_FEED:
	case GDATA_YOUTUBE_MOST_RECENT_FEED:
	case GDATA_YOUTUBE_MOST_DISCUSSED_FEED:
	case GDATA_YOUTUBE_MOST_LINKED_FEED:
	case GDATA_YOUTUBE_MOST_RESPONDED_FEED:
	case GDATA_YOUTUBE_RECENTLY_FEATURED_FEED:
	case GDATA_YOUTUBE_WATCH_ON_MOBILE_FEED: {
		gchar *date, *out;
		GTimeVal tv;

		/* All feed types except MOST_POPULAR have been deprecated for
		 * a while, and fall back to MOST_POPULAR on the server anyway.
		 * See: https://developers.google.com/youtube/2.0/developers_guide_protocol_video_feeds#Standard_feeds */
		g_get_current_time (&tv);
		tv.tv_sec -= 24 * 60 * 60;  /* 1 day ago */
		date = g_time_val_to_iso8601 (&tv);
		out = _gdata_service_build_uri ("https://www.googleapis.com/youtube/v3/videos"
		                                "?part=snippet"
		                                "&chart=mostPopular"
		                                "&publishedAfter=%s", date);
		g_free (date);

		return out;
	}
	default:
		g_assert_not_reached ();
	}
}

/**
 * gdata_youtube_service_query_standard_feed:
 * @self: a #GDataYouTubeService
 * @feed_type: the feed type to query, from #GDataYouTubeStandardFeedType
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (scope call) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service's standard @feed_type feed to build a #GDataFeed.
 *
 * Note that with the port from v2 to v3 of the YouTube API in libgdata
 * 0.17.0, all feed types except %GDATA_YOUTUBE_MOST_POPULAR_FEED have been
 * deprecated. Other feed types will now transparently return
 * %GDATA_YOUTUBE_MOST_POPULAR_FEED, limited to the past 24 hours.
 *
 * Parameters and errors are as for gdata_service_query().
 *
 * Return value: (transfer full): a #GDataFeed of query results, or %NULL; unref with g_object_unref()
 */
GDataFeed *
gdata_youtube_service_query_standard_feed (GDataYouTubeService *self, GDataYouTubeStandardFeedType feed_type, GDataQuery *query,
                                           GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                           GError **error)
{
	gchar *query_uri;
	GDataFeed *feed;

	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* TODO: Support the "time" parameter, as well as category- and region-specific feeds */
	query_uri = standard_feed_type_to_feed_uri (feed_type);
	feed = gdata_service_query (GDATA_SERVICE (self),
	                            get_youtube_authorization_domain (),
	                            query_uri, query, GDATA_TYPE_YOUTUBE_VIDEO,
	                            cancellable, progress_callback,
	                            progress_user_data, error);
	g_free (query_uri);

	return feed;
}

/**
 * gdata_youtube_service_query_standard_feed_async:
 * @self: a #GDataService
 * @feed_type: the feed type to query, from #GDataYouTubeStandardFeedType
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @destroy_progress_user_data: (allow-none): the function to call when @progress_callback will not be called any more, or %NULL. This function will be
 * called with @progress_user_data as a parameter and can be used to free any memory allocated for it.
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
 *
 * Since: 0.9.1
 */
void
gdata_youtube_service_query_standard_feed_async (GDataYouTubeService *self, GDataYouTubeStandardFeedType feed_type, GDataQuery *query,
                                                 GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                 GDestroyNotify destroy_progress_user_data,
                                                 GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *query_uri;

	g_return_if_fail (GDATA_IS_YOUTUBE_SERVICE (self));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	query_uri = standard_feed_type_to_feed_uri (feed_type);
	gdata_service_query_async (GDATA_SERVICE (self),
	                           get_youtube_authorization_domain (),
	                           query_uri, query, GDATA_TYPE_YOUTUBE_VIDEO,
	                           cancellable, progress_callback,
	                           progress_user_data,
	                           destroy_progress_user_data, callback,
	                           user_data);
	g_free (query_uri);
}

/**
 * gdata_youtube_service_query_videos:
 * @self: a #GDataYouTubeService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (scope call) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service for videos matching the parameters set on the #GDataQuery. This searches site-wide, and imposes no other restrictions or
 * parameters on the query.
 *
 * Parameters and errors are as for gdata_service_query().
 *
 * Return value: (transfer full): a #GDataFeed of query results, or %NULL; unref with g_object_unref()
 */
GDataFeed *
gdata_youtube_service_query_videos (GDataYouTubeService *self, GDataQuery *query,
                                    GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                    GError **error)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return gdata_service_query (GDATA_SERVICE (self),
	                            get_youtube_authorization_domain (),
	                            "https://www.googleapis.com/youtube/v3/search"
	                            "?part=snippet"
	                            "&type=video",
	                            query, GDATA_TYPE_YOUTUBE_VIDEO,
	                            cancellable, progress_callback,
	                            progress_user_data, error);
}

/**
 * gdata_youtube_service_query_videos_async:
 * @self: a #GDataService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @destroy_progress_user_data: (allow-none): the function to call when @progress_callback will not be called any more, or %NULL. This function will be
 * called with @progress_user_data as a parameter and can be used to free any memory allocated for it.
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
 *
 * Since: 0.9.1
 */
void
gdata_youtube_service_query_videos_async (GDataYouTubeService *self, GDataQuery *query,
                                          GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                          GDestroyNotify destroy_progress_user_data,
                                          GAsyncReadyCallback callback, gpointer user_data)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_SERVICE (self));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	gdata_service_query_async (GDATA_SERVICE (self),
	                           get_youtube_authorization_domain (),
	                           "https://www.googleapis.com/youtube/v3/search"
	                           "?part=snippet"
	                           "&type=video",
	                           query, GDATA_TYPE_YOUTUBE_VIDEO, cancellable,
	                           progress_callback, progress_user_data,
	                           destroy_progress_user_data, callback,
	                           user_data);
}

/**
 * gdata_youtube_service_query_related:
 * @self: a #GDataYouTubeService
 * @video: a #GDataYouTubeVideo for which to find related videos
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (scope call) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service for videos related to @video. The algorithm determining which videos are related is on the server side.
 *
 * Parameters and other errors are as for gdata_service_query().
 *
 * Return value: (transfer full): a #GDataFeed of query results; unref with g_object_unref()
 */
GDataFeed *
gdata_youtube_service_query_related (GDataYouTubeService *self, GDataYouTubeVideo *video, GDataQuery *query,
                                     GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                     GError **error)
{
	GDataFeed *feed;
	gchar *uri;

	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (video), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Execute the query */
	uri = _gdata_service_build_uri ("https://www.googleapis.com/youtube/v3/search"
	                                "?part=snippet"
	                                "&type=video"
	                                "&relatedToVideoId=%s",
	                                gdata_entry_get_id (GDATA_ENTRY (video)));
	feed = gdata_service_query (GDATA_SERVICE (self),
	                            get_youtube_authorization_domain (), uri,
	                            query, GDATA_TYPE_YOUTUBE_VIDEO,
	                            cancellable, progress_callback,
	                            progress_user_data, error);
	g_free (uri);

	return feed;
}

/**
 * gdata_youtube_service_query_related_async:
 * @self: a #GDataService
 * @video: a #GDataYouTubeVideo for which to find related videos
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @destroy_progress_user_data: (allow-none): the function to call when @progress_callback will not be called any more, or %NULL. This function will be
 * called with @progress_user_data as a parameter and can be used to free any memory allocated for it.
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
 *
 * Since: 0.9.1
 */
void
gdata_youtube_service_query_related_async (GDataYouTubeService *self, GDataYouTubeVideo *video, GDataQuery *query,
                                           GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                           GDestroyNotify destroy_progress_user_data,
                                           GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *uri;

	g_return_if_fail (GDATA_IS_YOUTUBE_SERVICE (self));
	g_return_if_fail (GDATA_IS_YOUTUBE_VIDEO (video));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	uri = _gdata_service_build_uri ("https://www.googleapis.com/youtube/v3/search"
	                                "?part=snippet"
	                                "&type=video"
	                                "&relatedToVideoId=%s",
	                                gdata_entry_get_id (GDATA_ENTRY (video)));
	gdata_service_query_async (GDATA_SERVICE (self),
	                           get_youtube_authorization_domain (), uri,
	                           query, GDATA_TYPE_YOUTUBE_VIDEO, cancellable,
	                           progress_callback, progress_user_data,
	                           destroy_progress_user_data, callback,
	                           user_data);
	g_free (uri);
}

/**
 * gdata_youtube_service_upload_video:
 * @self: a #GDataYouTubeService
 * @video: a #GDataYouTubeVideo to insert
 * @slug: the filename to give to the uploaded file
 * @content_type: the content type of the uploaded data
 * @cancellable: (allow-none): a #GCancellable for the entire upload stream, or %NULL
 * @error: a #GError, or %NULL
 *
 * Uploads a video to YouTube, using the properties from @video and the file data written to the resulting #GDataUploadStream.
 *
 * If @video has already been inserted, a %GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED error will be returned. If no user is authenticated
 * with the service, %GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED will be returned.
 *
 * The stream returned by this function should be written to using the standard #GOutputStream methods, asynchronously or synchronously. Once the stream
 * is closed (using g_output_stream_close()), gdata_youtube_service_finish_video_upload() should be called on it to parse and return the updated
 * #GDataYouTubeVideo for the uploaded video. This must be done, as @video isn't updated in-place.
 *
 * In order to cancel the upload, a #GCancellable passed in to @cancellable must be cancelled using g_cancellable_cancel(). Cancelling the individual
 * #GOutputStream operations on the #GDataUploadStream will not cancel the entire upload; merely the write or close operation in question. See the
 * #GDataUploadStream:cancellable for more details.
 *
 * Any upload errors will be thrown by the stream methods, and may come from the #GDataServiceError domain.
 *
 * Return value: (transfer full): a #GDataUploadStream to write the video data to, or %NULL; unref with g_object_unref()
 *
 * Since: 0.8.0
 */
GDataUploadStream *
gdata_youtube_service_upload_video (GDataYouTubeService *self, GDataYouTubeVideo *video, const gchar *slug, const gchar *content_type,
                                    GCancellable *cancellable, GError **error)
{
	GOutputStream *stream = NULL;  /* owned */

	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_YOUTUBE_VIDEO (video), NULL);
	g_return_val_if_fail (slug != NULL && *slug != '\0', NULL);
	g_return_val_if_fail (content_type != NULL && *content_type != '\0', NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (gdata_entry_is_inserted (GDATA_ENTRY (video)) == TRUE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED,
		                     _("The entry has already been inserted."));
		return NULL;
	}

	/* FIXME: Could be more cunning about domains here; see scope on
	 * https://developers.google.com/youtube/v3/guides/authentication */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_youtube_authorization_domain ()) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to upload a video."));
		return NULL;
	}

	/* FIXME: Add support for resumable uploads. That means a new
	 * gdata_youtube_service_upload_video_resumable() method a la
	 * Documents. */
	stream = gdata_upload_stream_new (GDATA_SERVICE (self),
	                                            get_youtube_authorization_domain (),
	                                            SOUP_METHOD_POST,
	                                            "https://www.googleapis.com/upload/youtube/v3/videos"
	                                            "?part=snippet,status,"
	                                                  "recordingDetails",
	                                            GDATA_ENTRY (video), slug,
	                                            content_type,
	                                            cancellable);

	return GDATA_UPLOAD_STREAM (stream);
}

/**
 * gdata_youtube_service_finish_video_upload:
 * @self: a #GDataYouTubeService
 * @upload_stream: the #GDataUploadStream from the operation
 * @error: a #GError, or %NULL
 *
 * Finish off a video upload operation started by gdata_youtube_service_upload_video(), parsing the result and returning the new #GDataYouTubeVideo.
 *
 * If an error occurred during the upload operation, it will have been returned during the operation (e.g. by g_output_stream_splice() or one
 * of the other stream methods). In such a case, %NULL will be returned but @error will remain unset. @error is only set in the case that the server
 * indicates that the operation was successful, but an error is encountered in parsing the result sent by the server.
 *
 * Return value: (transfer full): the new #GDataYouTubeVideo, or %NULL; unref with g_object_unref()
 *
 * Since: 0.8.0
 */
GDataYouTubeVideo *
gdata_youtube_service_finish_video_upload (GDataYouTubeService *self, GDataUploadStream *upload_stream, GError **error)
{
	const gchar *response_body;
	gssize response_length;

	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_UPLOAD_STREAM (upload_stream), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Get the response from the server */
	response_body = gdata_upload_stream_get_response (upload_stream, &response_length);
	if (response_body == NULL || response_length == 0)
		return NULL;

	/* Parse the response to produce a GDataYouTubeVideo */
	return GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_json (GDATA_TYPE_YOUTUBE_VIDEO,
	                                                          response_body,
	                                                          (gint) response_length,
	                                                          error));
}

/**
 * gdata_youtube_service_get_developer_key:
 * @self: a #GDataYouTubeService
 *
 * Gets the #GDataYouTubeService:developer-key property from the #GDataYouTubeService.
 *
 * Return value: the developer key property
 */
const gchar *
gdata_youtube_service_get_developer_key (GDataYouTubeService *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	return self->priv->developer_key;
}

/**
 * gdata_youtube_service_get_categories:
 * @self: a #GDataYouTubeService
 * @cancellable: (allow-none): a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Gets a list of the categories currently in use on YouTube. The returned #GDataAPPCategories contains a list of #GDataYouTubeCategorys which
 * enumerate the current YouTube categories.
 *
 * The category labels (#GDataCategory:label) are localised based on the value of #GDataService:locale.
 *
 * Return value: (transfer full): a #GDataAPPCategories, or %NULL; unref with g_object_unref()
 *
 * Since: 0.7.0
 */
GDataAPPCategories *
gdata_youtube_service_get_categories (GDataYouTubeService *self, GCancellable *cancellable, GError **error)
{
	const gchar *locale;
	gchar *uri;
	SoupMessage *message;
	GDataAPPCategories *categories;

	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Download the category list. Note that this is (service)
	 * locale-dependent, and a locale must always be specified. */
	locale = gdata_service_get_locale (GDATA_SERVICE (self));
	if (locale == NULL) {
		locale = "US";
	}

	uri = _gdata_service_build_uri ("https://www.googleapis.com/youtube/v3/videoCategories"
	                                "?part=snippet"
	                                "&regionCode=%s",
	                                locale);
	message = _gdata_service_query (GDATA_SERVICE (self),
	                                get_youtube_authorization_domain (),
	                                uri, NULL, cancellable, error);
	g_free (uri);

	if (message == NULL)
		return NULL;

	g_assert (message->response_body->data != NULL);
	categories = GDATA_APP_CATEGORIES (_gdata_parsable_new_from_json (GDATA_TYPE_APP_CATEGORIES,
	                                                                  message->response_body->data,
	                                                                  message->response_body->length,
	                                                                  GSIZE_TO_POINTER (GDATA_TYPE_YOUTUBE_CATEGORY),
	                                                                  error));
	g_object_unref (message);

	return categories;
}

static void
get_categories_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	GDataYouTubeService *service = GDATA_YOUTUBE_SERVICE (source_object);
	g_autoptr(GDataAPPCategories) categories = NULL;
	g_autoptr(GError) error = NULL;

	/* Get the categories and return */
	categories = gdata_youtube_service_get_categories (service, cancellable, &error);
	if (error != NULL)
		g_task_return_error (task, g_steal_pointer (&error));
	else
		g_task_return_pointer (task, g_steal_pointer (&categories), g_object_unref);
}

/**
 * gdata_youtube_service_get_categories_async:
 * @self: a #GDataYouTubeService
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
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
 */
void
gdata_youtube_service_get_categories_async (GDataYouTubeService *self, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	g_autoptr(GTask) task = NULL;

	g_return_if_fail (GDATA_IS_YOUTUBE_SERVICE (self));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_youtube_service_get_categories_async);
	g_task_run_in_thread (task, get_categories_thread);
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
 */
GDataAPPCategories *
gdata_youtube_service_get_categories_finish (GDataYouTubeService *self, GAsyncResult *async_result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);
	g_return_val_if_fail (g_task_is_valid (async_result, self), NULL);
	g_return_val_if_fail (g_async_result_is_tagged (async_result, gdata_youtube_service_get_categories_async), NULL);

	return g_task_propagate_pointer (G_TASK (async_result), error);
}
