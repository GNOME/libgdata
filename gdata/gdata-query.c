/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009–2010 <philip@tecnocode.co.uk>
 * Copyright (C) Red Hat, Inc. 2015
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
 * SECTION:gdata-query
 * @short_description: GData query object
 * @stability: Stable
 * @include: gdata/gdata-query.h
 *
 * #GDataQuery represents a collection of query parameters used in a series of queries on a #GDataService. It allows the query parameters to be
 * set, with the aim of building a query URI using gdata_query_get_query_uri(). Pagination is supported using gdata_query_next_page() and
 * gdata_query_previous_page().
 *
 * Each query can have an ETag associated with it, which is a unique identifier for the set of query results produced by the query.
 * Each time a query is made, gdata_service_query() will set the #GDataQuery:etag property of the accompanying query to a value returned by the
 * server. If the same query is made again (using the same #GDataQuery instance), the server can skip returning the resulting #GDataFeed if its
 * contents haven't changed (in this case, gdata_service_query() will return %NULL with an empty error).
 *
 * For this reason, code using #GDataQuery should be careful when reusing #GDataQuery instances: the code should either unset #GDataQuery:etag after
 * every query or (preferably) gracefully handle the case where gdata_service_query() returns %NULL to signify unchanged results.
 *
 * Every time a property of a #GDataQuery instance is changed, the instance's ETag will be unset.
 *
 * For more information on the standard GData query parameters supported by #GDataQuery, see the <ulink type="http"
 * url="http://code.google.com/apis/gdata/docs/2.0/reference.html#Queries">online documentation</ulink>.
 */

#include <glib.h>
#include <string.h>

#include "gdata-query.h"
#include "gdata-private.h"
#include "gdata-types.h"

static void gdata_query_finalize (GObject *object);
static void gdata_query_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_query_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void get_query_uri (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started);

struct _GDataQueryPrivate {
	/* Standard query parameters (see: http://code.google.com/apis/gdata/docs/2.0/reference.html#Queries) */
	gchar *q;
	gchar *q_internal;
	gchar *categories;
	gchar *author;
	gint64 updated_min;
	gint64 updated_max;
	gint64 published_min;
	gint64 published_max;
	guint start_index;
	gboolean is_strict;
	guint max_results;

	/* Pagination management. The type of pagination is set as
	 * pagination_type, and should be set in the init() vfunc implementation
	 * of any class derived from GDataQuery. It defaults to
	 * %GDATA_QUERY_PAGINATION_INDEXED, which most subclasses will not want.
	 *
	 * The next_uri, previous_uri or next_page_token are set by
	 * #GDataService if a query returns a new #GDataFeed containing them. If
	 * the user then calls next_page() or previous_page(), use_next_page or
	 * use_previous_page are set as appropriate, and the next call to
	 * get_uri() will return a URI for the next or previous page. This might
	 * be next_uri, previous_uri, or a constructed URI which appends the
	 * next_page_token.
	 *
	 * Note that %GDATA_QUERY_PAGINATION_TOKENS does not support returning
	 * to the previous page.
	 *
	 * It is not invalid to have use_next_page set and to not have a
	 * next_uri for %GDATA_QUERY_PAGINATION_URIS; or to not have a
	 * next_page_token for %GDATA_QUERY_PAGINATION_TOKENS: this signifies
	 * that the current set of results are the last page. There are no
	 * further pages. Similarly for use_previous_page and a %NULL
	 * previous_page.
	 */
	GDataQueryPaginationType pagination_type;

	gchar *next_uri;
	gchar *previous_uri;
	gchar *next_page_token;

	gboolean use_next_page;
	gboolean use_previous_page;

	gchar *etag;
};

enum {
	PROP_Q = 1,
	PROP_CATEGORIES,
	PROP_AUTHOR,
	PROP_UPDATED_MIN,
	PROP_UPDATED_MAX,
	PROP_PUBLISHED_MIN,
	PROP_PUBLISHED_MAX,
	PROP_START_INDEX,
	PROP_IS_STRICT,
	PROP_MAX_RESULTS,
	PROP_ETAG
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataQuery, gdata_query, G_TYPE_OBJECT)

static void
gdata_query_class_init (GDataQueryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->set_property = gdata_query_set_property;
	gobject_class->get_property = gdata_query_get_property;
	gobject_class->finalize = gdata_query_finalize;

	klass->get_query_uri = get_query_uri;

	/**
	 * GDataQuery:q:
	 *
	 * A full-text query string.
	 *
	 * When creating a query, list search terms separated by spaces, in the form <userinput>term1 term2 term3</userinput>.
	 * (As with all of the query parameter values, the spaces must be URL encoded.) The service returns all entries that match all of the
	 * search terms (like using AND between terms). Like Google's web search, a service searches on complete words (and related words with
	 * the same stem), not substrings.
	 *
	 * To search for an exact phrase, enclose the phrase in quotation marks: <userinput>"exact phrase"</userinput>.
	 *
	 * To exclude entries that match a given term, use the form <userinput>-term</userinput>.
	 *
	 * The search is case-insensitive.
	 *
	 * Example: to search for all entries that contain the exact phrase "Elizabeth Bennet" and the word "Darcy" but don't contain the
	 * word "Austen", use the following query: <userinput>"Elizabeth Bennet" Darcy -Austen</userinput>.
	 */
	g_object_class_install_property (gobject_class, PROP_Q,
	                                 g_param_spec_string ("q",
	                                                      "Query terms", "Query terms for which to search.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataQuery:categories:
	 *
	 * A category filter.
	 *
	 * You can query on multiple categories by listing multiple categories separated by slashes. The service returns all entries that match all
	 * of the categories (like using AND between terms). For example: <userinput>Fritz/Laurie</userinput> returns
	 * entries that match both categories ("Fritz" and "Laurie").
	 *
	 * To do an OR between terms, use a pipe character (<literal>|</literal>). For example: <userinput>Fritz\%7CLaurie</userinput> returns
	 * entries that match either category.
	 *
	 * An entry matches a specified category if the entry is in a category that has a matching term or label, as defined in the Atom
	 * specification. (Roughly, the "term" is the internal string used by the software to identify the category, while the "label" is the
	 * human-readable string presented to a user in a user interface.)
	 *
	 * To exclude entries that match a given category, use the form <userinput>-categoryname</userinput>.
	 *
	 * To query for a category that has a scheme – such as <literal>&lt;category scheme="urn:google.com" term="public"/&gt;</literal> – you must
	 * place the scheme in curly braces before the category name. For example: <userinput>{urn:google.com}public</userinput>. To match a category
	 * that has no scheme, use an empty pair of curly braces. If you don't specify curly braces, then categories in any scheme will match.
	 *
	 * The above features can be combined. For example: <userinput>A|-{urn:google.com}B/-C</userinput> means (A OR (NOT B)) AND (NOT C).
	 */
	g_object_class_install_property (gobject_class, PROP_CATEGORIES,
	                                 g_param_spec_string ("categories",
	                                                      "Category string", "Category search string.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataQuery:author:
	 *
	 * An entry author. The service returns entries where the author name and/or e-mail address match your query string.
	 */
	g_object_class_install_property (gobject_class, PROP_AUTHOR,
	                                 g_param_spec_string ("author",
	                                                      "Author", "Author search string.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataQuery:updated-min:
	 *
	 * Lower bound on the entry update date, inclusive.
	 */
	g_object_class_install_property (gobject_class, PROP_UPDATED_MIN,
	                                 g_param_spec_int64 ("updated-min",
	                                                     "Minimum update date", "Minimum date for updates on returned entries.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataQuery:updated-max:
	 *
	 * Upper bound on the entry update date, exclusive.
	 */
	g_object_class_install_property (gobject_class, PROP_UPDATED_MAX,
	                                 g_param_spec_int64 ("updated-max",
	                                                     "Maximum update date", "Maximum date for updates on returned entries.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataQuery:published-min:
	 *
	 * Lower bound on the entry publish date, inclusive.
	 */
	g_object_class_install_property (gobject_class, PROP_PUBLISHED_MIN,
	                                 g_param_spec_int64 ("published-min",
	                                                     "Minimum publish date", "Minimum date for returned entries to be published.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataQuery:published-max:
	 *
	 * Upper bound on the entry publish date, exclusive.
	 */
	g_object_class_install_property (gobject_class, PROP_PUBLISHED_MAX,
	                                 g_param_spec_int64 ("published-max",
	                                                     "Maximum publish date", "Maximum date for returned entries to be published.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataQuery:start-index:
	 *
	 * The one-based index of the first result to be retrieved. Use gdata_query_next_page() and gdata_query_previous_page() to
	 * implement pagination, rather than manually changing #GDataQuery:start-index.
	 *
	 * Use <code class="literal">0</code> to not specify a start index.
	 */
	g_object_class_install_property (gobject_class, PROP_START_INDEX,
	                                 g_param_spec_uint ("start-index",
	                                                    "Start index", "One-based result start index.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataQuery:is-strict:
	 *
	 * Strict query parameter checking. If this is enabled, an error will be returned by the online service if a parameter is
	 * not recognised.
	 *
	 * Since: 0.2.0
	 */
	g_object_class_install_property (gobject_class, PROP_IS_STRICT,
	                                 g_param_spec_boolean ("is-strict",
	                                                       "Strict?", "Should the server be strict about the query?",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataQuery:max-results:
	 *
	 * Maximum number of results to be retrieved. Most services have a default #GDataQuery:max-results size imposed by the server; if you wish
	 * to receive the entire feed, specify a large number such as %G_MAXUINT for this property.
	 *
	 * Use <code class="literal">0</code> to not specify a maximum number of results.
	 */
	g_object_class_install_property (gobject_class, PROP_MAX_RESULTS,
	                                 g_param_spec_uint ("max-results",
	                                                    "Maximum number of results", "The maximum number of entries to return.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataQuery:etag:
	 *
	 * The ETag against which to check for updates. If the server-side ETag matches this one, the requested feed hasn't changed, and is not
	 * returned unnecessarily.
	 *
	 * Setting any of the other query properties will unset the ETag, as ETags match against entire queries. If the ETag should be used in a
	 * query, it must be set again using gdata_query_set_etag() after setting any other properties.
	 *
	 * Since: 0.2.0
	 */
	g_object_class_install_property (gobject_class, PROP_ETAG,
	                                 g_param_spec_string ("etag",
	                                                      "ETag", "An ETag against which to check.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_query_init (GDataQuery *self)
{
	self->priv = gdata_query_get_instance_private (self);
	self->priv->updated_min = -1;
	self->priv->updated_max = -1;
	self->priv->published_min = -1;
	self->priv->published_max = -1;

	_gdata_query_set_pagination_type (self, GDATA_QUERY_PAGINATION_INDEXED);
}

static void
gdata_query_finalize (GObject *object)
{
	GDataQueryPrivate *priv = GDATA_QUERY (object)->priv;

	g_free (priv->q);
	g_free (priv->q_internal);
	g_free (priv->categories);
	g_free (priv->author);
	g_free (priv->next_uri);
	g_free (priv->previous_uri);
	g_free (priv->etag);
	g_free (priv->next_page_token);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_query_parent_class)->finalize (object);
}

static void
gdata_query_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataQueryPrivate *priv = GDATA_QUERY (object)->priv;

	switch (property_id) {
		case PROP_Q:
			g_value_set_string (value, priv->q);
			break;
		case PROP_CATEGORIES:
			g_value_set_string (value, priv->categories);
			break;
		case PROP_AUTHOR:
			g_value_set_string (value, priv->author);
			break;
		case PROP_UPDATED_MIN:
			g_value_set_int64 (value, priv->updated_min);
			break;
		case PROP_UPDATED_MAX:
			g_value_set_int64 (value, priv->updated_max);
			break;
		case PROP_PUBLISHED_MIN:
			g_value_set_int64 (value, priv->published_min);
			break;
		case PROP_PUBLISHED_MAX:
			g_value_set_int64 (value, priv->published_max);
			break;
		case PROP_START_INDEX:
			g_value_set_uint (value, priv->start_index);
			break;
		case PROP_IS_STRICT:
			g_value_set_boolean (value, priv->is_strict);
			break;
		case PROP_MAX_RESULTS:
			g_value_set_uint (value, priv->max_results);
			break;
		case PROP_ETAG:
			g_value_set_string (value, priv->etag);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_query_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataQuery *self = GDATA_QUERY (object);

	switch (property_id) {
		case PROP_Q:
			gdata_query_set_q (self, g_value_get_string (value));
			break;
		case PROP_CATEGORIES:
			gdata_query_set_categories (self, g_value_get_string (value));
			break;
		case PROP_AUTHOR:
			gdata_query_set_author (self, g_value_get_string (value));
			break;
		case PROP_UPDATED_MIN:
			gdata_query_set_updated_min (self, g_value_get_int64 (value));
			break;
		case PROP_UPDATED_MAX:
			gdata_query_set_updated_max (self, g_value_get_int64 (value));
			break;
		case PROP_PUBLISHED_MIN:
			gdata_query_set_published_min (self, g_value_get_int64 (value));
			break;
		case PROP_PUBLISHED_MAX:
			gdata_query_set_published_max (self, g_value_get_int64 (value));
			break;
		case PROP_START_INDEX:
			gdata_query_set_start_index (self, g_value_get_uint (value));
			break;
		case PROP_IS_STRICT:
			gdata_query_set_is_strict (self, g_value_get_boolean (value));
			break;
		case PROP_MAX_RESULTS:
			gdata_query_set_max_results (self, g_value_get_uint (value));
			break;
		case PROP_ETAG:
			gdata_query_set_etag (self, g_value_get_string (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
get_query_uri (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started)
{
	GDataQueryPrivate *priv = self->priv;

	#define APPEND_SEP g_string_append_c (query_uri, (*params_started == FALSE) ? '?' : '&'); *params_started = TRUE;

	/* Categories */
	if (priv->categories != NULL) {
		g_string_append (query_uri, "/-/");
		g_string_append_uri_escaped (query_uri, priv->categories, "/", FALSE);
	}

	/* q param */
	if (priv->q != NULL || priv->q_internal != NULL) {
		APPEND_SEP
		g_string_append (query_uri, "q=");

		if (priv->q != NULL) {
			g_string_append_uri_escaped (query_uri, priv->q, NULL, FALSE);
			if (priv->q_internal != NULL)
				g_string_append (query_uri, "%20and%20");
		}
		if (priv->q_internal != NULL)
			g_string_append_uri_escaped (query_uri, priv->q_internal, NULL, FALSE);
	}

	if (priv->author != NULL) {
		APPEND_SEP
		g_string_append (query_uri, "author=");
		g_string_append_uri_escaped (query_uri, priv->author, NULL, FALSE);
	}

	if (priv->updated_min != -1) {
		gchar *updated_min;

		APPEND_SEP
		g_string_append (query_uri, "updated-min=");
		updated_min = gdata_parser_int64_to_iso8601 (priv->updated_min);
		g_string_append (query_uri, updated_min);
		g_free (updated_min);
	}

	if (priv->updated_max != -1) {
		gchar *updated_max;

		APPEND_SEP
		g_string_append (query_uri, "updated-max=");
		updated_max = gdata_parser_int64_to_iso8601 (priv->updated_max);
		g_string_append (query_uri, updated_max);
		g_free (updated_max);
	}

	if (priv->published_min != -1) {
		gchar *published_min;

		APPEND_SEP
		g_string_append (query_uri, "published-min=");
		published_min = gdata_parser_int64_to_iso8601 (priv->published_min);
		g_string_append (query_uri, published_min);
		g_free (published_min);
	}

	if (priv->published_max != -1) {
		gchar *published_max;

		APPEND_SEP
		g_string_append (query_uri, "published-max=");
		published_max = gdata_parser_int64_to_iso8601 (priv->published_max);
		g_string_append (query_uri, published_max);
		g_free (published_max);
	}

	if (priv->start_index > 0) {
		APPEND_SEP
		g_string_append_printf (query_uri, "start-index=%u", priv->start_index);
	}

	if (priv->is_strict == TRUE) {
		APPEND_SEP
		g_string_append (query_uri, "strict=true");
	}

	if (priv->max_results > 0) {
		APPEND_SEP
		g_string_append_printf (query_uri, "max-results=%u", priv->max_results);
	}

	if (priv->pagination_type == GDATA_QUERY_PAGINATION_TOKENS && priv->use_next_page &&
	    priv->next_page_token != NULL && *priv->next_page_token != '\0') {
		APPEND_SEP
		g_string_append (query_uri, "pageToken=");
		g_string_append_uri_escaped (query_uri, priv->next_page_token, NULL, FALSE);
	}
}

/**
 * gdata_query_new:
 * @q: (allow-none): a query string, or %NULL
 *
 * Creates a new #GDataQuery with its #GDataQuery:q property set to @q.
 *
 * Return value: a new #GDataQuery
 */
GDataQuery *
gdata_query_new (const gchar *q)
{
	return g_object_new (GDATA_TYPE_QUERY, "q", q, NULL);
}

/**
 * gdata_query_new_with_limits:
 * @q: (allow-none): a query string, or %NULL
 * @start_index: a one-based start index for the results, or <code class="literal">0</code>
 * @max_results: the maximum number of results to return, or <code class="literal">0</code>
 *
 * Creates a new #GDataQuery with its #GDataQuery:q property set to @q, and the limits @start_index and @max_results
 * applied.
 *
 * Return value: a new #GDataQuery
 */
GDataQuery *
gdata_query_new_with_limits (const gchar *q, guint start_index, guint max_results)
{
	return g_object_new (GDATA_TYPE_QUERY,
	                     "q", q,
	                     "start-index", start_index,
	                     "max-results", max_results,
	                     NULL);
}

/**
 * gdata_query_get_query_uri:
 * @self: a #GDataQuery
 * @feed_uri: the feed URI on which to build the query URI
 *
 * Builds a query URI from the given base feed URI, using the properties of the #GDataQuery. This function will take care
 * of all necessary URI escaping, so it should <emphasis>not</emphasis> be done beforehand.
 *
 * The query URI is what functions like gdata_service_query() use to query the online service.
 *
 * Return value: a query URI; free with g_free()
 */
gchar *
gdata_query_get_query_uri (GDataQuery *self, const gchar *feed_uri)
{
	GDataQueryClass *klass;
	GString *query_uri;
	gboolean params_started;

	g_return_val_if_fail (GDATA_IS_QUERY (self), NULL);
	g_return_val_if_fail (feed_uri != NULL, NULL);

	/* Check to see if we're paginating first */
	if (self->priv->pagination_type == GDATA_QUERY_PAGINATION_URIS) {
		if (self->priv->use_next_page)
			return g_strdup (self->priv->next_uri);
		if (self->priv->use_previous_page)
			return g_strdup (self->priv->previous_uri);
	}

	klass = GDATA_QUERY_GET_CLASS (self);
	g_assert (klass->get_query_uri != NULL);

	/* Determine whether the first param has already been appended (e.g. it exists in the feed_uri) */
	params_started = (strstr (feed_uri, "?") != NULL) ? TRUE : FALSE;

	/* Build the query URI */
	query_uri = g_string_new (feed_uri);
	klass->get_query_uri (self, feed_uri, query_uri, &params_started);

	return g_string_free (query_uri, FALSE);
}

/* Used internally by child classes of GDataQuery to add search clauses that represent service-specific
 * query properties. For example, in the Drive v2 API, certain GDataDocumentsQuery properties like
 * show-deleted and show-folders no longer have their own parameters, but have to be specified as a search
 * clause in the query string. */
void
_gdata_query_add_q_internal (GDataQuery *self, const gchar *q)
{
	GDataQueryPrivate *priv = self->priv;
	GString *str;

	g_return_if_fail (GDATA_IS_QUERY (self));
	g_return_if_fail (q != NULL && q[0] != '\0');

	str = g_string_new (priv->q_internal);

	/* Search parameters: https://developers.google.com/drive/web/search-parameters */
	if (str->len > 0)
		g_string_append (str, " and ");

	g_string_append (str, q);

	g_free (priv->q_internal);
	priv->q_internal = g_string_free (str, FALSE);
}

/* Used internally by child classes of GDataQuery to clear the internal query string when building the
 * query URI in GDataQueryClass->get_query_uri */
void
_gdata_query_clear_q_internal (GDataQuery *self)
{
	GDataQueryPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_QUERY (self));

	g_free (priv->q_internal);
	priv->q_internal = NULL;
}

/**
 * gdata_query_get_q:
 * @self: a #GDataQuery
 *
 * Gets the #GDataQuery:q property.
 *
 * Return value: the q property, or %NULL if it is unset
 */
const gchar *
gdata_query_get_q (GDataQuery *self)
{
	g_return_val_if_fail (GDATA_IS_QUERY (self), NULL);
	return self->priv->q;
}

/**
 * gdata_query_set_q:
 * @self: a #GDataQuery
 * @q: (allow-none): a new query string, or %NULL
 *
 * Sets the #GDataQuery:q property of the #GDataQuery to the new query string, @q.
 *
 * Set @q to %NULL to unset the property in the query URI.
 */
void
gdata_query_set_q (GDataQuery *self, const gchar *q)
{
	g_return_if_fail (GDATA_IS_QUERY (self));

	g_free (self->priv->q);
	self->priv->q = g_strdup (q);
	g_object_notify (G_OBJECT (self), "q");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (self, NULL);
}

/**
 * gdata_query_get_categories:
 * @self: a #GDataQuery
 *
 * Gets the #GDataQuery:categories property.
 *
 * Return value: the categories property, or %NULL if it is unset
 */
const gchar *
gdata_query_get_categories (GDataQuery *self)
{
	g_return_val_if_fail (GDATA_IS_QUERY (self), NULL);
	return self->priv->categories;
}

/**
 * gdata_query_set_categories:
 * @self: a #GDataQuery
 * @categories: (allow-none): the new category string, or %NULL
 *
 * Sets the #GDataQuery:categories property of the #GDataQuery to the new category string, @categories.
 *
 * Set @categories to %NULL to unset the property in the query URI.
 */
void
gdata_query_set_categories (GDataQuery *self, const gchar *categories)
{
	g_return_if_fail (GDATA_IS_QUERY (self));

	g_free (self->priv->categories);
	self->priv->categories = g_strdup (categories);
	g_object_notify (G_OBJECT (self), "categories");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (self, NULL);
}

/**
 * gdata_query_get_author:
 * @self: a #GDataQuery
 *
 * Gets the #GDataQuery:author property.
 *
 * Return value: the author property, or %NULL if it is unset
 */
const gchar *
gdata_query_get_author (GDataQuery *self)
{
	g_return_val_if_fail (GDATA_IS_QUERY (self), NULL);
	return self->priv->author;
}

/**
 * gdata_query_set_author:
 * @self: a #GDataQuery
 * @author: (allow-none): the new author string, or %NULL
 *
 * Sets the #GDataQuery:author property of the #GDataQuery to the new author string, @author.
 *
 * Set @author to %NULL to unset the property in the query URI.
 */
void
gdata_query_set_author (GDataQuery *self, const gchar *author)
{
	g_return_if_fail (GDATA_IS_QUERY (self));

	g_free (self->priv->author);
	self->priv->author = g_strdup (author);
	g_object_notify (G_OBJECT (self), "author");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (self, NULL);
}

/**
 * gdata_query_get_updated_min:
 * @self: a #GDataQuery
 *
 * Gets the #GDataQuery:updated-min property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the updated-min property, or <code class="literal">-1</code>
 */
gint64
gdata_query_get_updated_min (GDataQuery *self)
{
	g_return_val_if_fail (GDATA_IS_QUERY (self), -1);
	return self->priv->updated_min;
}

/**
 * gdata_query_set_updated_min:
 * @self: a #GDataQuery
 * @updated_min: the new minimum update time, or <code class="literal">-1</code>
 *
 * Sets the #GDataQuery:updated-min property of the #GDataQuery to the new minimum update time, @updated_min.
 *
 * Set @updated_min to <code class="literal">-1</code> to unset the property in the query URI.
 */
void
gdata_query_set_updated_min (GDataQuery *self, gint64 updated_min)
{
	g_return_if_fail (GDATA_IS_QUERY (self));
	g_return_if_fail (updated_min >= -1);

	self->priv->updated_min = updated_min;
	g_object_notify (G_OBJECT (self), "updated-min");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (self, NULL);
}

/**
 * gdata_query_get_updated_max:
 * @self: a #GDataQuery
 *
 * Gets the #GDataQuery:updated-max property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the updated-max property, or <code class="literal">-1</code>
 */
gint64
gdata_query_get_updated_max (GDataQuery *self)
{
	g_return_val_if_fail (GDATA_IS_QUERY (self), -1);
	return self->priv->updated_max;
}

/**
 * gdata_query_set_updated_max:
 * @self: a #GDataQuery
 * @updated_max: the new maximum update time, or <code class="literal">-1</code>
 *
 * Sets the #GDataQuery:updated-max property of the #GDataQuery to the new maximum update time, @updated_max.
 *
 * Set @updated_max to <code class="literal">-1</code> to unset the property in the query URI.
 */
void
gdata_query_set_updated_max (GDataQuery *self, gint64 updated_max)
{
	g_return_if_fail (GDATA_IS_QUERY (self));
	g_return_if_fail (updated_max >= -1);

	self->priv->updated_max = updated_max;
	g_object_notify (G_OBJECT (self), "updated-max");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (self, NULL);
}

/**
 * gdata_query_get_published_min:
 * @self: a #GDataQuery
 *
 * Gets the #GDataQuery:published-min property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the published-min property, or <code class="literal">-1</code>
 */
gint64
gdata_query_get_published_min (GDataQuery *self)
{
	g_return_val_if_fail (GDATA_IS_QUERY (self), -1);
	return self->priv->published_min;
}

/**
 * gdata_query_set_published_min:
 * @self: a #GDataQuery
 * @published_min: the new minimum publish time, or <code class="literal">-1</code>
 *
 * Sets the #GDataQuery:published-min property of the #GDataQuery to the new minimum publish time, @published_min.
 *
 * Set @published_min to <code class="literal">-1</code> to unset the property in the query URI.
 */
void
gdata_query_set_published_min (GDataQuery *self, gint64 published_min)
{
	g_return_if_fail (GDATA_IS_QUERY (self));
	g_return_if_fail (published_min >= -1);

	self->priv->published_min = published_min;
	g_object_notify (G_OBJECT (self), "published-min");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (self, NULL);
}

/**
 * gdata_query_get_published_max:
 * @self: a #GDataQuery
 *
 * Gets the #GDataQuery:published-max property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the published-max property, or <code class="literal">-1</code>
 */
gint64
gdata_query_get_published_max (GDataQuery *self)
{
	g_return_val_if_fail (GDATA_IS_QUERY (self), -1);
	return self->priv->published_max;
}

/**
 * gdata_query_set_published_max:
 * @self: a #GDataQuery
 * @published_max: the new maximum publish time, or <code class="literal">-1</code>
 *
 * Sets the #GDataQuery:published-max property of the #GDataQuery to the new maximum publish time, @published_max.
 *
 * Set @published_max to <code class="literal">-1</code> to unset the property in the query URI.
 */
void
gdata_query_set_published_max (GDataQuery *self, gint64 published_max)
{
	g_return_if_fail (GDATA_IS_QUERY (self));
	g_return_if_fail (published_max >= -1);

	self->priv->published_max = published_max;
	g_object_notify (G_OBJECT (self), "published-max");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (self, NULL);
}

/**
 * gdata_query_get_start_index:
 * @self: a #GDataQuery
 *
 * Gets the #GDataQuery:start-index property.
 *
 * Return value: the start index property, or <code class="literal">0</code> if it is unset
 */
guint
gdata_query_get_start_index (GDataQuery *self)
{
	g_return_val_if_fail (GDATA_IS_QUERY (self), 0);
	return self->priv->start_index;
}

/**
 * gdata_query_set_start_index:
 * @self: a #GDataQuery
 * @start_index: the new start index, or <code class="literal">0</code>
 *
 * Sets the #GDataQuery:start-index property of the #GDataQuery to the new one-based start index, @start_index.
 *
 * Set @start_index to <code class="literal">0</code> to unset the property in the query URI.
 */
void
gdata_query_set_start_index (GDataQuery *self, guint start_index)
{
	g_return_if_fail (GDATA_IS_QUERY (self));

	self->priv->start_index = start_index;
	g_object_notify (G_OBJECT (self), "start-index");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (self, NULL);
}

/**
 * gdata_query_is_strict:
 * @self: a #GDataQuery
 *
 * Gets the #GDataQuery:is-strict property.
 *
 * Return value: the strict property
 *
 * Since: 0.2.0
 */
gboolean
gdata_query_is_strict (GDataQuery *self)
{
	g_return_val_if_fail (GDATA_IS_QUERY (self), FALSE);
	return self->priv->is_strict;
}

/**
 * gdata_query_set_is_strict:
 * @self: a #GDataQuery
 * @is_strict: the new strict value
 *
 * Sets the #GDataQuery:is-strict property of the #GDataQuery to the new strict value, @is_strict.
 *
 * Since: 0.2.0
 */
void
gdata_query_set_is_strict (GDataQuery *self, gboolean is_strict)
{
	g_return_if_fail (GDATA_IS_QUERY (self));

	self->priv->is_strict = is_strict;
	g_object_notify (G_OBJECT (self), "is-strict");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (self, NULL);
}

/**
 * gdata_query_get_max_results:
 * @self: a #GDataQuery
 *
 * Gets the #GDataQuery:max-results property.
 *
 * Return value: the maximum results property, or <code class="literal">0</code> if it is unset
 */
guint
gdata_query_get_max_results (GDataQuery *self)
{
	g_return_val_if_fail (GDATA_IS_QUERY (self), 0);
	return self->priv->max_results;
}

/**
 * gdata_query_set_max_results:
 * @self: a #GDataQuery
 * @max_results: the new maximum results value, or <code class="literal">0</code>
 *
 * Sets the #GDataQuery:max-results property of the #GDataQuery to the new maximum results value, @max_results.
 *
 * Set @max_results to <code class="literal">0</code> to unset the property in the query URI.
 */
void
gdata_query_set_max_results (GDataQuery *self, guint max_results)
{
	g_return_if_fail (GDATA_IS_QUERY (self));

	self->priv->max_results = max_results;
	g_object_notify (G_OBJECT (self), "max-results");

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (self, NULL);
}

/**
 * gdata_query_get_etag:
 * @self: a #GDataQuery
 *
 * Gets the #GDataQuery:etag property.
 *
 * Return value: the ETag property, or %NULL if it is unset
 *
 * Since: 0.2.0
 */
const gchar *
gdata_query_get_etag (GDataQuery *self)
{
	g_return_val_if_fail (GDATA_IS_QUERY (self), NULL);
	return self->priv->etag;
}

/**
 * gdata_query_set_etag:
 * @self: a #GDataQuery
 * @etag: (allow-none): the new ETag, or %NULL
 *
 * Sets the #GDataQuery:etag property of the #GDataQuery to the new ETag, @etag.
 *
 * Set @etag to %NULL to not check against the server-side ETag.
 *
 * Since: 0.2.0
 */
void
gdata_query_set_etag (GDataQuery *self, const gchar *etag)
{
	g_return_if_fail (GDATA_IS_QUERY (self));

	g_free (self->priv->etag);
	self->priv->etag = g_strdup (etag);
	g_object_notify (G_OBJECT (self), "etag");
}

void
_gdata_query_clear_pagination (GDataQuery *self)
{
	g_return_if_fail (GDATA_IS_QUERY (self));

	switch (self->priv->pagination_type) {
	case GDATA_QUERY_PAGINATION_INDEXED:
		/* Nothing to do here: indexes can always be incremented. */
		break;
	case GDATA_QUERY_PAGINATION_URIS:
		g_clear_pointer (&self->priv->next_uri, g_free);
		g_clear_pointer (&self->priv->previous_uri, g_free);
		break;
	case GDATA_QUERY_PAGINATION_TOKENS:
		g_clear_pointer (&self->priv->next_page_token, g_free);
		break;
	default:
		g_assert_not_reached ();
	}

	self->priv->use_next_page = FALSE;
	self->priv->use_previous_page = FALSE;
}

void
_gdata_query_set_pagination_type (GDataQuery               *self,
                                  GDataQueryPaginationType  type)
{
	g_debug ("%s: Pagination type set to %u", G_STRFUNC, type);

	_gdata_query_clear_pagination (self);
	self->priv->pagination_type = type;
}

void
_gdata_query_set_next_page_token (GDataQuery  *self,
                                  const gchar *next_page_token)
{
	g_return_if_fail (GDATA_IS_QUERY (self));
	g_return_if_fail (self->priv->pagination_type ==
	                  GDATA_QUERY_PAGINATION_TOKENS);

	g_free (self->priv->next_page_token);
	self->priv->next_page_token = g_strdup (next_page_token);
}

void
_gdata_query_set_next_uri (GDataQuery *self, const gchar *next_uri)
{
	g_return_if_fail (GDATA_IS_QUERY (self));
	g_return_if_fail (self->priv->pagination_type ==
	                  GDATA_QUERY_PAGINATION_URIS);

	g_free (self->priv->next_uri);
	self->priv->next_uri = g_strdup (next_uri);
}

gboolean
_gdata_query_is_finished (GDataQuery *self)
{
	g_return_val_if_fail (GDATA_IS_QUERY (self), FALSE);

	switch (self->priv->pagination_type) {
	case GDATA_QUERY_PAGINATION_INDEXED:
		return FALSE;
	case GDATA_QUERY_PAGINATION_URIS:
		return (self->priv->next_uri == NULL && self->priv->use_next_page);
	case GDATA_QUERY_PAGINATION_TOKENS:
		return (self->priv->next_page_token == NULL && self->priv->use_next_page);
	default:
		g_assert_not_reached ();
	}
}

void
_gdata_query_set_previous_uri (GDataQuery *self, const gchar *previous_uri)
{
	g_return_if_fail (GDATA_IS_QUERY (self));
	g_return_if_fail (self->priv->pagination_type ==
	                  GDATA_QUERY_PAGINATION_URIS);

	g_free (self->priv->previous_uri);
	self->priv->previous_uri = g_strdup (previous_uri);
}

/**
 * gdata_query_next_page:
 * @self: a #GDataQuery
 *
 * Changes the state of the #GDataQuery such that when gdata_query_get_query_uri() is next called, it will build the
 * query URI for the next page in the result set.
 *
 * Ideally, the URI of the next page is retrieved from a feed automatically when gdata_service_query() is called, but
 * gdata_query_next_page() will fall back to using #GDataQuery:start-index to emulate true pagination if this fails.
 *
 * You <emphasis>should not</emphasis> implement pagination manually using #GDataQuery:start-index.
 */
void
gdata_query_next_page (GDataQuery *self)
{
	GDataQueryPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_QUERY (self));

	switch (self->priv->pagination_type) {
	case GDATA_QUERY_PAGINATION_INDEXED:
		if (priv->start_index == 0)
			priv->start_index++;
		priv->start_index += priv->max_results;
		break;
	case GDATA_QUERY_PAGINATION_URIS:
	case GDATA_QUERY_PAGINATION_TOKENS:
		priv->use_next_page = TRUE;
		priv->use_previous_page = FALSE;
		break;
	default:
		g_assert_not_reached ();
	}

	/* Our current ETag will no longer be relevant */
	gdata_query_set_etag (self, NULL);
}

/**
 * gdata_query_previous_page:
 * @self: a #GDataQuery
 *
 * Changes the state of the #GDataQuery such that when gdata_query_get_query_uri() is next called, it will build the
 * query URI for the previous page in the result set.
 *
 * See the documentation for gdata_query_next_page() for an explanation of how query URIs from the feeds are used to this end.
 *
 * Return value: %TRUE if there is a previous page and it has been switched to, %FALSE otherwise
 */
gboolean
gdata_query_previous_page (GDataQuery *self)
{
	GDataQueryPrivate *priv = self->priv;
	gboolean retval;

	g_return_val_if_fail (GDATA_IS_QUERY (self), FALSE);

	switch (self->priv->pagination_type) {
	case GDATA_QUERY_PAGINATION_INDEXED:
		if (priv->start_index <= priv->max_results) {
			retval = FALSE;
		} else {
			priv->start_index -= priv->max_results;
			if (priv->start_index == 1)
				priv->start_index--;
			retval = TRUE;
		}
		break;
	case GDATA_QUERY_PAGINATION_URIS:
		if (priv->previous_uri != NULL) {
			priv->use_next_page = FALSE;
			priv->use_previous_page = TRUE;
			retval = TRUE;
		} else {
			retval = FALSE;
		}
		break;
	case GDATA_QUERY_PAGINATION_TOKENS:
		/* There are no previous page tokens, unfortunately. */
		retval = FALSE;
		break;
	default:
		g_assert_not_reached ();
	}

	if (retval) {
		/* Our current ETag will no longer be relevant */
		gdata_query_set_etag (self, NULL);
	}

	return retval;
}
