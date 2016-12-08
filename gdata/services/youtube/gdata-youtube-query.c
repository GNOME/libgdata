/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
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
 * SECTION:gdata-youtube-query
 * @short_description: GData YouTube query object
 * @stability: Stable
 * @include: gdata/services/youtube/gdata-youtube-query.h
 *
 * #GDataYouTubeQuery represents a collection of query parameters specific to the YouTube service, which go above and beyond
 * those catered for by #GDataQuery.
 *
 * With the transition to version 3 of the YouTube, the #GDataQuery:author and
 * #GDataQuery:start-index properties are no longer supported, and their values
 * will be ignored. Use gdata_query_next_page() instead of the
 * #GDataQuery:start-index API.
 *
 * For more information on the custom GData query parameters supported by #GDataYouTubeQuery, see the <ulink type="http"
 * url="https://developers.google.com/youtube/v3/docs/search/list#parameters">online documentation</ulink>.
 *
 * Since: 0.3.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-youtube-query.h"
#include "gdata-query.h"
#include "gdata-youtube-content.h"
#include "gdata-private.h"

static void gdata_youtube_query_finalize (GObject *object);
static void gdata_youtube_query_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_youtube_query_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void get_query_uri (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started);

struct _GDataYouTubeQueryPrivate {
	G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	GDataYouTubeFormat format;
	GDataYouTubeSortOrder sort_order;
	GDataYouTubeUploader uploader;
	G_GNUC_END_IGNORE_DEPRECATIONS

	gdouble latitude;
	gdouble longitude;
	gdouble location_radius;
	gboolean has_location;
	gchar *language;
	gchar *order_by;
	gchar *restriction;
	GDataYouTubeSafeSearch safe_search;
	GDataYouTubeAge age;
	gchar *license;
};

enum {
	PROP_FORMAT = 1,
	PROP_LATITUDE,
	PROP_LONGITUDE,
	PROP_LOCATION_RADIUS,
	PROP_HAS_LOCATION,
	PROP_LANGUAGE,
	PROP_ORDER_BY,
	PROP_RESTRICTION,
	PROP_SAFE_SEARCH,
	PROP_SORT_ORDER,
	PROP_AGE,
	PROP_UPLOADER,
	PROP_LICENSE,
};

G_DEFINE_TYPE (GDataYouTubeQuery, gdata_youtube_query, GDATA_TYPE_QUERY)

static void
gdata_youtube_query_class_init (GDataYouTubeQueryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataQueryClass *query_class = GDATA_QUERY_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataYouTubeQueryPrivate));

	gobject_class->set_property = gdata_youtube_query_set_property;
	gobject_class->get_property = gdata_youtube_query_get_property;
	gobject_class->finalize = gdata_youtube_query_finalize;

	query_class->get_query_uri = get_query_uri;

	/**
	 * GDataYouTubeQuery:format:
	 *
	 * Specifies that videos must be available in a particular video format. Use %GDATA_YOUTUBE_FORMAT_UNKNOWN to
	 * retrieve videos irrespective of their format availability.
	 *
	 * Since: 0.3.0
	 * Deprecated: 0.17.0: No longer supported by Google. The value of
	 *   this property will be unused in queries. There is no replacement.
	 */
	g_object_class_install_property (gobject_class, PROP_FORMAT,
	                                 g_param_spec_enum ("format",
	                                                    "Format", "Specifies that videos must be available in a particular video format.",
	                                                    GDATA_TYPE_YOUTUBE_FORMAT, GDATA_YOUTUBE_FORMAT_UNKNOWN,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
	                                                    G_PARAM_DEPRECATED));

	/**
	 * GDataYouTubeQuery:latitude:
	 *
	 * The latitude of a particular location of which videos should be found. This should be used in conjunction with
	 * #GDataYouTubeQuery:longitude; if either property is outside the valid range, neither will be used. Valid latitudes
	 * are between <code class="literal">-90</code> and <code class="literal">90</code>0 degrees; any values of this property outside that range
	 * will unset the property in the query URI.
	 *
	 * If #GDataYouTubeQuery:location-radius is a non-<code class="literal">0</code> value, this will define a circle from which videos should be
	 * found.
	 *
	 * As it is deprecated, the value of #GDataYouTubeQuery:has-location is
	 * ignored.
	 *
	 * For more information, see the <ulink type="http"
	 * url="https://developers.google.com/youtube/v3/docs/search/list#location">online documentation</ulink>.
	 *
	 * Since: 0.3.0
	 */
	g_object_class_install_property (gobject_class, PROP_LATITUDE,
	                                 g_param_spec_double ("latitude",
	                                                      "Latitude", "The latitude of a particular location of which videos should be found.",
	                                                      -G_MAXDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeQuery:longitude:
	 *
	 * The longitude of a particular location of which videos should be found. This should be used in conjunction with
	 * #GDataYouTubeQuery:latitude; if either property is outside the valid range, neither will be used. Valid longitudes
	 * are between <code class="literal">-180</code> and <code class="literal">180</code> degrees; any values of this property outside that
	 * range will unset the property in the query URI.
	 *
	 * For more information, see the documentation for #GDataYouTubeQuery:latitude.
	 *
	 * Since: 0.3.0
	 */
	g_object_class_install_property (gobject_class, PROP_LONGITUDE,
	                                 g_param_spec_double ("longitude",
	                                                      "Longitude", "The longitude of a particular location of which videos should be found.",
	                                                      -G_MAXDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeQuery:location-radius:
	 *
	 * The radius, in metres, of a circle from within which videos should be returned. The circle is centred on the latitude and
	 * longitude given in #GDataYouTubeQuery:latitude and #GDataYouTubeQuery:longitude.
	 *
	 * Set this property to <code class="literal">0</code> to search for specific coordinates, rather than within a given radius.
	 *
	 * For more information, see the documentation for #GDataYouTubeQuery:latitude.
	 *
	 * Since: 0.3.0
	 */
	g_object_class_install_property (gobject_class, PROP_LOCATION_RADIUS,
	                                 g_param_spec_double ("location-radius",
	                                                      "Location radius", "The radius, in metres, of a circle to search within.",
	                                                      0.0, G_MAXDOUBLE, 0.0,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeQuery:has-location:
	 *
	 * Whether to restrict search results to videos with specific coordinates associated with them. If used with a given
	 * #GDataYouTubeQuery:latitude and #GDataYouTubeQuery:longitude, only videos with specific coordinates (not those with merely
	 * a descriptive address) will be returned. If used without a latitude and longitude set, only videos with specific coordinates
	 * (regardless of those coordinates) will be returned.
	 *
	 * For more information, see the documentation for #GDataYouTubeQuery:latitude.
	 *
	 * Since: 0.3.0
	 * Deprecated: 0.17.0: No longer supported by Google. The value of
	 *   this property will be unused in queries.
	 */
	g_object_class_install_property (gobject_class, PROP_HAS_LOCATION,
	                                 g_param_spec_boolean ("has-location",
	                                                       "Has location?", "Whether to restrict results to videos with specific coordinates.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
	                                                       G_PARAM_DEPRECATED));

	/**
	 * GDataYouTubeQuery:language:
	 *
	 * Restricts the search to videos that have a title, description or keywords in a specified language. The language code should
	 * be a two-letter ISO 639-1 code; or you can use <literal>zh-Hans</literal> for simplified Chinese and <literal>zh-Hant</literal>
	 * for traditional Chinese.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://code.google.com/apis/youtube/2.0/reference.html#lrsp">online documentation</ulink>.
	 *
	 * Since: 0.3.0
	 * Deprecated: 0.17.0: No longer supported by Google. The value of
	 *   this property will be unused in queries. There is no replacement.
	 */
	g_object_class_install_property (gobject_class, PROP_LANGUAGE,
	                                 g_param_spec_string ("language",
	                                                      "Language", "Restricts the search to videos described in the given language.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
	                                                      G_PARAM_DEPRECATED));

	/**
	 * GDataYouTubeQuery:order-by:
	 *
	 * Specifies the order of entries in a feed. Supported values are <literal>relevance</literal>,
	 * <literal>published</literal>, <literal>viewCount</literal> and <literal>rating</literal>.
	 *
	 * Additionally, results most relevant to a specific language can be returned by setting the property
	 * to <literal>relevance_lang_<replaceable>languageCode</replaceable></literal>, where
	 * <replaceable>languageCode</replaceable> is an ISO 639-1 language code, as used in #GDataYouTubeQuery:language.
	 *
	 * For more information, see the <ulink type="http"
	 * url="https://developers.google.com/youtube/v3/docs/search/list#order">online documentation</ulink>.
	 *
	 * Since: 0.3.0
	 */
	g_object_class_install_property (gobject_class, PROP_ORDER_BY,
	                                 g_param_spec_string ("order-by",
	                                                      "Order by", "Specifies the order of entries in a feed.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeQuery:restriction:
	 *
	 * An ISO 3166 two-letter country code that should be used to filter
	 * videos playable only in specific countries.
	 *
	 * Previously, this property could also accept the client’s IP address
	 * for country lookup. This feature is no longer supported by Google,
	 * and will result in an error from the server if used. Use a country
	 * code instead.
	 *
	 * For more information, see the <ulink type="http"
	 * url="https://developers.google.com/youtube/v3/docs/search/list#regionCode">online documentation</ulink>.
	 *
	 * Since: 0.3.0
	 */
	g_object_class_install_property (gobject_class, PROP_RESTRICTION,
	                                 g_param_spec_string ("restriction",
	                                                      "Restriction", "The country code to filter videos playable only in specific countries.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeQuery:safe-search:
	 *
	 * Whether the search results should include restricted content as well as standard content.
	 *
	 * For more information, see the <ulink type="http"
	 * url="https://developers.google.com/youtube/v3/docs/search/list#safeSearch">online documentation</ulink>.
	 *
	 * Since: 0.3.0
	 */
	g_object_class_install_property (gobject_class, PROP_SAFE_SEARCH,
	                                 g_param_spec_enum ("safe-search",
	                                                    "Safe search", "Whether the search results should include restricted content.",
	                                                    GDATA_TYPE_YOUTUBE_SAFE_SEARCH, GDATA_YOUTUBE_SAFE_SEARCH_MODERATE,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeQuery:sort-order:
	 *
	 * Specifies the direction of sorting. To use the default sort order, set the property to %GDATA_YOUTUBE_SORT_NONE.
	 *
	 * Since: 0.3.0
	 * Deprecated: 0.17.0: No longer supported by Google. The value of
	 *   this property will be unused in queries. There is no replacement.
	 */
	g_object_class_install_property (gobject_class, PROP_SORT_ORDER,
	                                 g_param_spec_enum ("sort-order",
	                                                    "Sort order", "Specifies the direction of sorting.",
	                                                    GDATA_TYPE_YOUTUBE_SORT_ORDER, GDATA_YOUTUBE_SORT_NONE,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
	                                                    G_PARAM_DEPRECATED));

	/**
	 * GDataYouTubeQuery:age:
	 *
	 * Restricts the search to videos uploaded within the specified time period. To retrieve videos irrespective of their
	 * age, set the property to %GDATA_YOUTUBE_AGE_ALL_TIME.
	 *
	 * Since: 0.3.0
	 */
	g_object_class_install_property (gobject_class, PROP_AGE,
	                                 g_param_spec_enum ("age",
	                                                    "Age", "Restricts the search to videos uploaded within the specified time period.",
	                                                    GDATA_TYPE_YOUTUBE_AGE, GDATA_YOUTUBE_AGE_ALL_TIME,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeQuery:uploader:
	 *
	 * Restricts the search to videos from the specified type of uploader. Currently, this can only be used to restrict
	 * searches to videos from YouTube partners.
	 *
	 * Since: 0.3.0
	 * Deprecated: 0.17.0: No longer supported by Google. The value of
	 *   this property will be unused in queries. There is no replacement.
	 */
	g_object_class_install_property (gobject_class, PROP_UPLOADER,
	                                 g_param_spec_enum ("uploader",
	                                                    "Uploader", "Restricts the search to videos from the specified type of uploader.",
	                                                    GDATA_TYPE_YOUTUBE_UPLOADER, GDATA_YOUTUBE_UPLOADER_ALL,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
	                                                    G_PARAM_DEPRECATED));

	/**
	 * GDataYouTubeQuery:license:
	 *
	 * The content license which should be used to filter search results. If set to, for example, %GDATA_YOUTUBE_LICENSE_CC, only videos which
	 * are Creative Commons licensed will be returned in search results. Set this to %NULL to return videos under any license.
	 *
	 * For more information, see the <ulink type="http"
	 * url="https://developers.google.com/youtube/v3/docs/search/list#videoLicense">online documentation</ulink>.
	 *
	 * Since: 0.11.0
	 */
	g_object_class_install_property (gobject_class, PROP_LICENSE,
	                                 g_param_spec_string ("license",
	                                                      "License", "The content license which should be used to filter search results.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}

static void
gdata_youtube_query_init (GDataYouTubeQuery *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_YOUTUBE_QUERY, GDataYouTubeQueryPrivate);

	self->priv->latitude = G_MAXDOUBLE;
	self->priv->longitude = G_MAXDOUBLE;

	/* https://developers.google.com/youtube/v3/docs/search/list#pageToken */
	_gdata_query_set_pagination_type (GDATA_QUERY (self),
	                                  GDATA_QUERY_PAGINATION_TOKENS);
}

static void
gdata_youtube_query_finalize (GObject *object)
{
	GDataYouTubeQueryPrivate *priv = GDATA_YOUTUBE_QUERY (object)->priv;

	g_free (priv->language);
	g_free (priv->order_by);
	g_free (priv->restriction);
	g_free (priv->license);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_youtube_query_parent_class)->finalize (object);
}

static void
gdata_youtube_query_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataYouTubeQueryPrivate *priv = GDATA_YOUTUBE_QUERY (object)->priv;

	switch (property_id) {
		case PROP_FORMAT:
			g_value_set_enum (value, priv->format);
			break;
		case PROP_LATITUDE:
			g_value_set_double (value, priv->latitude);
			break;
		case PROP_LONGITUDE:
			g_value_set_double (value, priv->longitude);
			break;
		case PROP_LOCATION_RADIUS:
			g_value_set_double (value, priv->location_radius);
			break;
		case PROP_HAS_LOCATION:
			g_value_set_boolean (value, priv->has_location);
			break;
		case PROP_LANGUAGE:
			g_value_set_string (value, priv->language);
			break;
		case PROP_ORDER_BY:
			g_value_set_string (value, priv->order_by);
			break;
		case PROP_RESTRICTION:
			g_value_set_string (value, priv->restriction);
			break;
		case PROP_SAFE_SEARCH:
			g_value_set_enum (value, priv->safe_search);
			break;
		case PROP_SORT_ORDER:
			g_value_set_enum (value, priv->sort_order);
			break;
		case PROP_AGE:
			g_value_set_enum (value, priv->age);
			break;
		case PROP_UPLOADER:
			g_value_set_enum (value, priv->uploader);
			break;
		case PROP_LICENSE:
			g_value_set_string (value, priv->license);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_youtube_query_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataYouTubeQuery *self = GDATA_YOUTUBE_QUERY (object);

	switch (property_id) {
		case PROP_FORMAT:
			G_GNUC_BEGIN_IGNORE_DEPRECATIONS
			gdata_youtube_query_set_format (self, g_value_get_enum (value));
			G_GNUC_END_IGNORE_DEPRECATIONS
			break;
		case PROP_LATITUDE:
			self->priv->latitude = g_value_get_double (value);
			break;
		case PROP_LONGITUDE:
			self->priv->longitude = g_value_get_double (value);
			break;
		case PROP_LOCATION_RADIUS:
			self->priv->location_radius = g_value_get_double (value);
			break;
		case PROP_HAS_LOCATION:
			self->priv->has_location = g_value_get_boolean (value);
			break;
		case PROP_LANGUAGE:
			G_GNUC_BEGIN_IGNORE_DEPRECATIONS
			gdata_youtube_query_set_language (self, g_value_get_string (value));
			G_GNUC_END_IGNORE_DEPRECATIONS
			break;
		case PROP_ORDER_BY:
			gdata_youtube_query_set_order_by (self, g_value_get_string (value));
			break;
		case PROP_RESTRICTION:
			gdata_youtube_query_set_restriction (self, g_value_get_string (value));
			break;
		case PROP_SAFE_SEARCH:
			gdata_youtube_query_set_safe_search (self, g_value_get_enum (value));
			break;
		case PROP_SORT_ORDER:
			G_GNUC_BEGIN_IGNORE_DEPRECATIONS
			gdata_youtube_query_set_sort_order (self, g_value_get_enum (value));
			G_GNUC_END_IGNORE_DEPRECATIONS
			break;
		case PROP_AGE:
			gdata_youtube_query_set_age (self, g_value_get_enum (value));
			break;
		case PROP_UPLOADER:
			G_GNUC_BEGIN_IGNORE_DEPRECATIONS
			gdata_youtube_query_set_uploader (self, g_value_get_enum (value));
			G_GNUC_END_IGNORE_DEPRECATIONS
			break;
		case PROP_LICENSE:
			gdata_youtube_query_set_license (self, g_value_get_string (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

/* Convert from a v2 order-by parameter value to a v3 order parameter value.
 * Reference:
 * v2: https://developers.google.com/youtube/2.0/developers_guide_protocol_api_query_parameters#orderbysp
 * v3: https://developers.google.com/youtube/v3/docs/search/list#order
 */
static const gchar *
get_v3_order (const gchar *v2_order_by)
{
	const struct {
		const gchar *v2_order_by;
		const gchar *v3_order;
	} mapping[] = {
		{ "relevance", "relevance" },
		{ "published", "date" },
		{ "viewCount", "viewCount" },
		{ "rating", "rating" },
	};
	guint i;

	for (i = 0; i < G_N_ELEMENTS (mapping); i++) {
		if (g_strcmp0 (v2_order_by, mapping[i].v2_order_by) == 0) {
			return mapping[i].v3_order;
		}
	}

	/* Special case for ‘relevance_lang_*’. */
	if (g_str_has_prefix (v2_order_by, "relevance_lang_")) {
		return "relevance";
	}

	return NULL;
}

/* Convert from a v2 license parameter value to a v3 videoLicense parameter
 * value. Reference:
 * v2: https://developers.google.com/youtube/2.0/developers_guide_protocol_api_query_parameters#licensesp
 * v3: https://developers.google.com/youtube/v3/docs/search/list#videoLicense
 */
static const gchar *
get_v3_video_license (const gchar *v2_license)
{
	if (g_strcmp0 (v2_license, "cc") == 0) {
		return "creativeCommon";
	} else if (g_strcmp0 (v2_license, "youtube") == 0) {
		return "youtube";
	} else {
		return NULL;
	}
}

static void
get_query_uri (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started)
{
	GDataYouTubeQueryPrivate *priv = GDATA_YOUTUBE_QUERY (self)->priv;

	#define APPEND_SEP g_string_append_c (query_uri, (*params_started == FALSE) ? '?' : '&'); *params_started = TRUE;

	/* NOTE: We do not chain up because the parent class implements a lot
	 * of deprecated API. */

	/* Categories */
	if (gdata_query_get_categories (self) != NULL) {
		APPEND_SEP
		g_string_append (query_uri, "videoCategoryId=");
		g_string_append_uri_escaped (query_uri,
		                             gdata_query_get_categories (self),
		                             NULL,
		                             FALSE);
	}

	/* q param */
	if (gdata_query_get_q (self) != NULL) {
		APPEND_SEP
		g_string_append (query_uri, "q=");
		g_string_append_uri_escaped (query_uri,
		                             gdata_query_get_q (self), NULL,
		                             FALSE);
	}

	if (gdata_query_get_max_results (self) > 0) {
		APPEND_SEP
		g_string_append_printf (query_uri, "maxResults=%u",
		                        gdata_query_get_max_results (self));
	}

	if (priv->age != GDATA_YOUTUBE_AGE_ALL_TIME) {
		gchar *after;
		GTimeVal tv = { 0, };

		g_get_current_time (&tv);

		/* Squash the microseconds; they’re not useful. */
		tv.tv_usec = 0;

		switch (priv->age) {
		case GDATA_YOUTUBE_AGE_TODAY:
			tv.tv_sec -= 24 * 60 * 60;
			break;
		case GDATA_YOUTUBE_AGE_THIS_WEEK:
			tv.tv_sec -= 7 * 24 * 60 * 60;
			break;
		case GDATA_YOUTUBE_AGE_THIS_MONTH:
			tv.tv_sec -= 31 * 24 * 60 * 60;
			break;
		case GDATA_YOUTUBE_AGE_ALL_TIME:
		default:
			g_assert_not_reached ();
		}

		APPEND_SEP

		after = g_time_val_to_iso8601 (&tv);
		g_string_append_printf (query_uri, "publishedAfter=%s", after);
		g_free (after);
	}

	/* We don’t need to use APPEND_SEP below here, as this parameter is
	 * always included */
	APPEND_SEP
	switch (priv->safe_search) {
		case GDATA_YOUTUBE_SAFE_SEARCH_NONE:
			g_string_append (query_uri, "safeSearch=none");
			break;
		case GDATA_YOUTUBE_SAFE_SEARCH_MODERATE:
			g_string_append (query_uri, "safeSearch=moderate");
			break;
		case GDATA_YOUTUBE_SAFE_SEARCH_STRICT:
			g_string_append (query_uri, "safeSearch=strict");
			break;
		default:
			g_assert_not_reached ();
	}

	if (priv->latitude >= -90.0 && priv->latitude <= 90.0 &&
	    priv->longitude >= -180.0 && priv->longitude <= 180.0) {
		gchar latitude[G_ASCII_DTOSTR_BUF_SIZE];
		gchar longitude[G_ASCII_DTOSTR_BUF_SIZE];

		g_string_append_printf (query_uri, "&location=%s,%s",
		                        g_ascii_dtostr (latitude,
		                                        sizeof (latitude),
		                                        priv->latitude),
		                        g_ascii_dtostr (longitude,
		                                        sizeof (longitude),
		                                        priv->longitude));

		if (priv->location_radius >= 0.0) {
			gchar radius[G_ASCII_DTOSTR_BUF_SIZE];
			g_string_append_printf (query_uri, "&locationRadius=%sm",
			                        g_ascii_dtostr (radius,
			                                        sizeof (radius),
			                                        priv->location_radius));
		}
	}

	if (priv->order_by != NULL) {
		const gchar *v3_order_by = get_v3_order (priv->order_by);

		if (v3_order_by != NULL) {
			g_string_append (query_uri, "&order=");
			g_string_append_uri_escaped (query_uri, v3_order_by,
			                             NULL, FALSE);
		}
	}

	if (priv->restriction != NULL) {
		g_string_append (query_uri, "&regionCode=");
		g_string_append_uri_escaped (query_uri, priv->restriction, NULL, FALSE);
	}

	if (priv->license != NULL) {
		const gchar *v3_video_license;

		v3_video_license = get_v3_video_license (priv->license);

		if (v3_video_license != NULL) {
			g_string_append (query_uri, "&videoLicense=");
			g_string_append_uri_escaped (query_uri,
			                             v3_video_license, NULL,
			                             FALSE);
		}
	}
}

/**
 * gdata_youtube_query_new:
 * @q: (allow-none): a query string, or %NULL
 *
 * Creates a new #GDataYouTubeQuery with its #GDataQuery:q property set to @q.
 *
 * Return value: a new #GDataYouTubeQuery
 *
 * Since: 0.3.0
 */
GDataYouTubeQuery *
gdata_youtube_query_new (const gchar *q)
{
	return g_object_new (GDATA_TYPE_YOUTUBE_QUERY, "q", q, NULL);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * gdata_youtube_query_get_format:
 * @self: a #GDataYouTubeQuery
 *
 * Gets the #GDataYouTubeQuery:format property.
 *
 * Return value: the format property
 *
 * Since: 0.3.0
 * Deprecated: 0.17.0: No longer supported by Google. The value of
 *   this property will be unused in queries. There is no replacement.
 */
GDataYouTubeFormat
gdata_youtube_query_get_format (GDataYouTubeQuery *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_QUERY (self), GDATA_YOUTUBE_FORMAT_UNKNOWN);
	return self->priv->format;
}

/**
 * gdata_youtube_query_set_format:
 * @self: a #GDataYouTubeQuery
 * @format: the requested video format
 *
 * Sets the #GDataYouTubeQuery:format property of the #GDataYouTubeQuery to @format.
 *
 * Since: 0.3.0
 * Deprecated: 0.17.0: No longer supported by Google. The value of
 *   this property will be unused in queries. There is no replacement.
 */
void
gdata_youtube_query_set_format (GDataYouTubeQuery *self, GDataYouTubeFormat format)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_QUERY (self));
	self->priv->format = format;
	g_object_notify (G_OBJECT (self), "format");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * gdata_youtube_query_get_location:
 * @self: a #GDataYouTubeQuery
 * @latitude: (out caller-allocates) (allow-none): a location in which to return the latitude, or %NULL
 * @longitude: (out caller-allocates) (allow-none): a location in which to return the longitude, or %NULL
 * @radius: (out caller-allocates) (allow-none): a location in which to return the location radius, or %NULL
 * @has_location: (out caller-allocates) (allow-none): a location in which to return %TRUE if the query is searching for videos with a specific
 * location, %FALSE otherwise, or %NULL
 *
 * Gets the location-based properties of the #GDataYouTubeQuery<!-- -->: #GDataYouTubeQuery:latitude, #GDataYouTubeQuery:longitude,
 * #GDataYouTubeQuery:location-radius and #GDataYouTubeQuery:has-location.
 *
 * Since: 0.3.0
 */
void
gdata_youtube_query_get_location (GDataYouTubeQuery *self, gdouble *latitude, gdouble *longitude, gdouble *radius, gboolean *has_location)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_QUERY (self));

	if (latitude != NULL)
		*latitude = self->priv->latitude;
	if (longitude != NULL)
		*longitude = self->priv->longitude;
	if (radius != NULL)
		*radius = self->priv->location_radius;
	if (has_location != NULL)
		*has_location = self->priv->has_location;
}

/**
 * gdata_youtube_query_set_location:
 * @self: a #GDataYouTubeQuery
 * @latitude: the new latitude, or %G_MAXDOUBLE
 * @longitude: the new longitude, or %G_MAXDOUBLE
 * @radius: the new location radius, or <code class="literal">0</code>
 * @has_location: %TRUE if the query is for videos with a specific location, %FALSE otherwise
 *
 * Sets the location-based properties of the #GDataYouTubeQuery<!-- -->: #GDataYouTubeQuery:latitude, #GDataYouTubeQuery:longitude,
 * #GDataYouTubeQuery:location-radius and #GDataYouTubeQuery:has-location.
 *
 * Since: 0.3.0
 */
void
gdata_youtube_query_set_location (GDataYouTubeQuery *self, gdouble latitude, gdouble longitude, gdouble radius, gboolean has_location)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_QUERY (self));

	self->priv->latitude = latitude;
	self->priv->longitude = longitude;
	self->priv->location_radius = radius;
	self->priv->has_location = has_location;

	g_object_freeze_notify (G_OBJECT (self));
	g_object_notify (G_OBJECT (self), "latitude");
	g_object_notify (G_OBJECT (self), "longitude");
	g_object_notify (G_OBJECT (self), "location-radius");
	g_object_notify (G_OBJECT (self), "has-location");
	g_object_thaw_notify (G_OBJECT (self));

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

/**
 * gdata_youtube_query_get_language:
 * @self: a #GDataYouTubeQuery
 *
 * Gets the #GDataYouTubeQuery:language property.
 *
 * Return value: the language property, or %NULL if it is unset
 *
 * Since: 0.3.0
 * Deprecated: 0.17.0: No longer supported by Google. The value of
 *   this property will be unused in queries. There is no replacement.
 */
const gchar *
gdata_youtube_query_get_language (GDataYouTubeQuery *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_QUERY (self), NULL);
	return self->priv->language;
}

/**
 * gdata_youtube_query_set_language:
 * @self: a #GDataYouTubeQuery
 * @language: (allow-none): a new language name, or %NULL
 *
 * Sets the #GDataYouTubeQuery:language property of the #GDataYouTubeQuery to the new language, @language.
 *
 * Set @language to %NULL to unset the property in the query URI.
 *
 * Since: 0.3.0
 * Deprecated: 0.17.0: No longer supported by Google. The value of
 *   this property will be unused in queries. There is no replacement.
 */
void
gdata_youtube_query_set_language (GDataYouTubeQuery *self, const gchar *language)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_QUERY (self));
	g_free (self->priv->language);
	self->priv->language = g_strdup (language);
	g_object_notify (G_OBJECT (self), "language");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

/**
 * gdata_youtube_query_get_order_by:
 * @self: a #GDataYouTubeQuery
 *
 * Gets the #GDataYouTubeQuery:order-by property.
 *
 * Return value: the order by property, or %NULL if it is unset
 *
 * Since: 0.3.0
 */
const gchar *
gdata_youtube_query_get_order_by (GDataYouTubeQuery *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_QUERY (self), NULL);
	return self->priv->order_by;
}

/**
 * gdata_youtube_query_set_order_by:
 * @self: a #GDataYouTubeQuery
 * @order_by: (allow-none): a new order by string, or %NULL
 *
 * Sets the #GDataYouTubeQuery:order-by property of the #GDataYouTubeQuery to the new order by string, @order_by.
 *
 * Set @order_by to %NULL to unset the property in the query URI.
 *
 * Since: 0.3.0
 */
void
gdata_youtube_query_set_order_by (GDataYouTubeQuery *self, const gchar *order_by)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_QUERY (self));

	g_free (self->priv->order_by);
	self->priv->order_by = g_strdup (order_by);
	g_object_notify (G_OBJECT (self), "order-by");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

/**
 * gdata_youtube_query_get_restriction:
 * @self: a #GDataYouTubeQuery
 *
 * Gets the #GDataYouTubeQuery:restriction property.
 *
 * Return value: the restriction property, or %NULL if it is unset
 *
 * Since: 0.3.0
 */
const gchar *
gdata_youtube_query_get_restriction (GDataYouTubeQuery *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_QUERY (self), NULL);
	return self->priv->restriction;
}

/**
 * gdata_youtube_query_set_restriction:
 * @self: a #GDataYouTubeQuery
 * @restriction: (allow-none): a new restriction string, or %NULL
 *
 * Sets the #GDataYouTubeQuery:restriction property of the #GDataYouTubeQuery to the new restriction string, @restriction.
 *
 * Set @restriction to %NULL to unset the property in the query URI.
 *
 * Since: 0.3.0
 */
void
gdata_youtube_query_set_restriction (GDataYouTubeQuery *self, const gchar *restriction)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_QUERY (self));
	g_free (self->priv->restriction);
	self->priv->restriction = g_strdup (restriction);
	g_object_notify (G_OBJECT (self), "restriction");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

/**
 * gdata_youtube_query_get_safe_search:
 * @self: a #GDataYouTubeQuery
 *
 * Gets the #GDataYouTubeQuery:safe-search property.
 *
 * Return value: the safe search property
 *
 * Since: 0.3.0
 */
GDataYouTubeSafeSearch
gdata_youtube_query_get_safe_search (GDataYouTubeQuery *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_QUERY (self), GDATA_YOUTUBE_SAFE_SEARCH_MODERATE);
	return self->priv->safe_search;
}

/**
 * gdata_youtube_query_set_safe_search:
 * @self: a #GDataYouTubeQuery
 * @safe_search: a new safe search level
 *
 * Sets the #GDataYouTubeQuery:safe-search property of the #GDataYouTubeQuery to @safe_search.
 *
 * Since: 0.3.0
 */
void
gdata_youtube_query_set_safe_search (GDataYouTubeQuery *self, GDataYouTubeSafeSearch safe_search)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_QUERY (self));
	self->priv->safe_search = safe_search;
	g_object_notify (G_OBJECT (self), "safe-search");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * gdata_youtube_query_get_sort_order:
 * @self: a #GDataYouTubeQuery
 *
 * Gets the #GDataYouTubeQuery:sort-order property.
 *
 * Return value: the sort order property
 *
 * Since: 0.3.0
 * Deprecated: 0.17.0: No longer supported by Google. The value of
 *   this property will be unused in queries. There is no replacement.
 */
GDataYouTubeSortOrder
gdata_youtube_query_get_sort_order (GDataYouTubeQuery *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_QUERY (self), GDATA_YOUTUBE_SORT_NONE);
	return self->priv->sort_order;
}

/**
 * gdata_youtube_query_set_sort_order:
 * @self: a #GDataYouTubeQuery
 * @sort_order: the new sort order
 *
 * Sets the #GDataYouTubeQuery:sort-order property of the #GDataYouTubeQuery to @sort_order.
 *
 * Set @sort_order to %GDATA_YOUTUBE_SORT_NONE to unset the property in the query URI.
 *
 * Since: 0.3.0
 * Deprecated: 0.17.0: No longer supported by Google. The value of
 *   this property will be unused in queries. There is no replacement.
 */
void
gdata_youtube_query_set_sort_order (GDataYouTubeQuery *self, GDataYouTubeSortOrder sort_order)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_QUERY (self));
	self->priv->sort_order = sort_order;
	g_object_notify (G_OBJECT (self), "sort-order");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * gdata_youtube_query_get_age:
 * @self: a #GDataYouTubeQuery
 *
 * Gets the #GDataYouTubeQuery:age property.
 *
 * Return value: the age property
 *
 * Since: 0.3.0
 */
GDataYouTubeAge
gdata_youtube_query_get_age (GDataYouTubeQuery *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_QUERY (self), GDATA_YOUTUBE_AGE_ALL_TIME);
	return self->priv->age;
}

/**
 * gdata_youtube_query_set_age:
 * @self: a #GDataYouTubeQuery
 * @age: the new age
 *
 * Sets the #GDataYouTubeQuery:age property of the #GDataYouTubeQuery to @age.
 *
 * Since: 0.3.0
 */
void
gdata_youtube_query_set_age (GDataYouTubeQuery *self, GDataYouTubeAge age)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_QUERY (self));
	self->priv->age = age;
	g_object_notify (G_OBJECT (self), "age");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * gdata_youtube_query_get_uploader:
 * @self: a #GDataYouTubeQuery
 *
 * Gets the #GDataYouTubeQuery:uploader property.
 *
 * Return value: the uploader property
 *
 * Since: 0.3.0
 * Deprecated: 0.17.0: No longer supported by Google. The value of
 *   this property will be unused in queries. There is no replacement.
 */
GDataYouTubeUploader
gdata_youtube_query_get_uploader (GDataYouTubeQuery *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_QUERY (self), GDATA_YOUTUBE_UPLOADER_ALL);
	return self->priv->uploader;
}

/**
 * gdata_youtube_query_set_uploader:
 * @self: a #GDataYouTubeQuery
 * @uploader: the new uploader
 *
 * Sets the #GDataYouTubeQuery:uploader property of the #GDataYouTubeQuery to @uploader.
 *
 * Since: 0.3.0
 * Deprecated: 0.17.0: No longer supported by Google. The value of
 *   this property will be unused in queries. There is no replacement.
 */
void
gdata_youtube_query_set_uploader (GDataYouTubeQuery *self, GDataYouTubeUploader uploader)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_QUERY (self));
	self->priv->uploader = uploader;
	g_object_notify (G_OBJECT (self), "uploader");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}

G_GNUC_END_IGNORE_DEPRECATIONS

/**
 * gdata_youtube_query_get_license:
 * @self: a #GDataYouTubeQuery
 *
 * Gets the #GDataYouTubeQuery:license property.
 *
 * Return value: the license property, or %NULL if it is unset
 *
 * Since: 0.11.0
 */
const gchar *
gdata_youtube_query_get_license (GDataYouTubeQuery *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_QUERY (self), NULL);
	return self->priv->license;
}

/**
 * gdata_youtube_query_set_license:
 * @self: a #GDataYouTubeQuery
 * @license: (allow-none): a new license value, or %NULL
 *
 * Sets the #GDataYouTubeQuery:license property of the #GDataYouTubeQuery to the new license value, @license.
 *
 * Set @license to %NULL to unset the property in the query URI.
 *
 * Since: 0.11.0
 */
void
gdata_youtube_query_set_license (GDataYouTubeQuery *self, const gchar *license)
{
	g_return_if_fail (GDATA_IS_YOUTUBE_QUERY (self));
	g_free (self->priv->license);
	self->priv->license = g_strdup (license);
	g_object_notify (G_OBJECT (self), "license");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (GDATA_QUERY (self), NULL);
}
