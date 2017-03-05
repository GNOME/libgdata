/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) 2014 Carlos Garnacho <carlosg@gnome.org>
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
 * SECTION:gdata-freebase-service
 * @short_description: GData Freebase service object
 * @stability: Stable
 * @include: gdata/services/freebase/gdata-freebase-service.h
 *
 * #GDataFreebaseService is a subclass of #GDataService for communicating with the Google Freebase API. It supports queries
 * in MQL format, that allows highly flexible queries on any topic. MQL is a JSON based query language, MQL requests consist
 * of a mix of defined and empty values for types in the Freebase schema, those "placeholder" values will be filled in on the
 * reply. For more information and examples, see the <ulink type="http" url="https://developers.google.com/freebase/v1/mql-overview">
 * MQL overview page</ulink>.
 *
 * For more details of Google Freebase API, see the <ulink type="http" url="https://developers.google.com/freebase/v1/">
 * online documentation</ulink>.
 *
 * Since August 2016, [Google has retired Freebase](https://developers.google.com/freebase/),
 * so all of these APIs will return an error if used; and should be considered
 * deprecated.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-freebase-service.h"
#include "gdata-freebase-result.h"
#include "gdata-freebase-search-result.h"
#include "gdata-service.h"
#include "gdata-private.h"
#include "gdata-query.h"
#include "gdata-feed.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/* Standards reference at https://developers.google.com/freebase/v1/ */

#define URLBASE "://www.googleapis.com/freebase/v1"
#define IMAGE_URI_PREFIX "https://usercontent.googleapis.com/freebase/v1/image"

enum {
	PROP_DEVELOPER_KEY = 1
};

struct _GDataFreebaseServicePrivate {
	gchar *developer_key;
};

static void gdata_freebase_service_set_property (GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gdata_freebase_service_get_property (GObject *self, guint prop_id, GValue *value, GParamSpec *pspec);
static void gdata_freebase_service_finalize (GObject *self);
static void append_query_headers (GDataService *self, GDataAuthorizationDomain *domain, SoupMessage *message);
static GList *get_authorization_domains (void);

_GDATA_DEFINE_AUTHORIZATION_DOMAIN (freebase, "freebase", "https://www.googleapis.com/auth/freebase.readonly")

G_DEFINE_TYPE (GDataFreebaseService, gdata_freebase_service, GDATA_TYPE_SERVICE)

static void
gdata_freebase_service_class_init (GDataFreebaseServiceClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataServiceClass *service_class = GDATA_SERVICE_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataFreebaseServicePrivate));

	gobject_class->set_property = gdata_freebase_service_set_property;
	gobject_class->get_property = gdata_freebase_service_get_property;
	gobject_class->finalize = gdata_freebase_service_finalize;

	service_class->append_query_headers = append_query_headers;
	service_class->get_authorization_domains = get_authorization_domains;

	/**
	 * GDataFreebaseService:developer-key:
	 *
	 * The developer key your application has registered with the Freebase API. For more information, see the <ulink type="http"
	 * url="https://developers.google.com/freebase/v1/how-tos/authorizing">online documentation</ulink>.
	 *
	 * Since: 0.15.1
	 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
	 */
	g_object_class_install_property (gobject_class, PROP_DEVELOPER_KEY,
	                                 g_param_spec_string ("developer-key",
	                                                      "Developer key", "Your Freebase developer API key.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
	                                                      G_PARAM_DEPRECATED));
}

static void
gdata_freebase_service_init (GDataFreebaseService *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_FREEBASE_SERVICE, GDataFreebaseServicePrivate);
}

static void
append_query_headers (GDataService *self, GDataAuthorizationDomain *domain, SoupMessage *message)
{
	GDataFreebaseServicePrivate *priv = GDATA_FREEBASE_SERVICE (self)->priv;
	const gchar *query;
	GString *new_query;
	SoupURI *uri;

	g_assert (message != NULL);

	if (priv->developer_key) {
		uri = soup_message_get_uri (message);
		query = soup_uri_get_query (uri);

		/* Set the key on every request, as per
		 * https://developers.google.com/freebase/v1/parameters
		 */
		if (query) {
			new_query = g_string_new (query);

			g_string_append (new_query, "&key=");
			g_string_append_uri_escaped (new_query, priv->developer_key, NULL, FALSE);

			soup_uri_set_query (uri, new_query->str);
			g_string_free (new_query, TRUE);
		}
	}

	/* Chain up to the parent class */
	GDATA_SERVICE_CLASS (gdata_freebase_service_parent_class)->append_query_headers (self, domain, message);
}

static GList *
get_authorization_domains (void)
{
	return g_list_prepend (NULL, get_freebase_authorization_domain ());
}

static void
gdata_freebase_service_set_property (GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GDataFreebaseServicePrivate *priv = GDATA_FREEBASE_SERVICE (self)->priv;

	switch (prop_id) {
	case PROP_DEVELOPER_KEY:
		priv->developer_key = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
		break;
	}
}

static void
gdata_freebase_service_get_property (GObject *self, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GDataFreebaseServicePrivate *priv = GDATA_FREEBASE_SERVICE (self)->priv;

	switch (prop_id) {
	case PROP_DEVELOPER_KEY:
		g_value_set_string (value, priv->developer_key);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
		break;
	}
}

static void
gdata_freebase_service_finalize (GObject *self)
{
	GDataFreebaseServicePrivate *priv = GDATA_FREEBASE_SERVICE (self)->priv;

	g_free (priv->developer_key);

	G_OBJECT_CLASS (gdata_freebase_service_parent_class)->finalize (self);
}

/**
 * gdata_freebase_service_new:
 * @developer_key: (allow-none): developer key to use the API, or %NULL
 * @authorizer: (allow-none): a #GDataAuthorizer to authorize the service's requests, or %NULL
 *
 * Creates a new #GDataFreebaseService using the given #GDataAuthorizer. If @authorizer is %NULL, all requests are made as an unauthenticated user.
 * Having both @developer_key and @authorizer set to %NULL is allowed, but this should be reserved for debugging situations, as there is a certain
 * key-less quota for those purposes. If this service is used on any code intended to be deployed, one or both of @developer_key and @authorizer
 * should be non-%NULL and valid.
 *
 * Return value: (transfer full): a new #GDataFreebaseService; unref with g_object_unref()
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GDataFreebaseService *
gdata_freebase_service_new (const gchar *developer_key, GDataAuthorizer *authorizer)
{
	g_return_val_if_fail (authorizer == NULL || GDATA_IS_AUTHORIZER (authorizer), NULL);

	return g_object_new (GDATA_TYPE_FREEBASE_SERVICE,
	                     "developer-key", developer_key,
	                     "authorizer", authorizer,
	                     NULL);
}

/**
 * gdata_freebase_service_get_primary_authorization_domain:
 *
 * The primary #GDataAuthorizationDomain for interacting with Freebase. This will not normally need to be used, as it's used internally
 * by the #GDataFreebaseService methods. However, if using the plain #GDataService methods to implement custom queries or requests which libgdata
 * does not support natively, then this domain may be needed to authorize the requests.
 *
 * The domain never changes, and is interned so that pointer comparison can be used to differentiate it from other authorization domains.
 *
 * Return value: (transfer none): the service's authorization domain
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GDataAuthorizationDomain *
gdata_freebase_service_get_primary_authorization_domain (void)
{
	return get_freebase_authorization_domain ();
}

/**
 * gdata_freebase_service_query:
 * @self: a #GDataFreebaseService
 * @query: a #GDataFreebaseQuery with the MQL query
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: (allow-none): a #GError, or %NULL
 *
 * Performs a MQL query on the service, you can find out more about MQL in the <ulink type="http" url="http://mql.freebaseapps.com/index.html">online MQL documentation</ulink>.
 *
 * Return value: (transfer full): a #GDataFreebaseResult containing the query result; unref with g_object_unref()
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GDataFreebaseResult *
gdata_freebase_service_query (GDataFreebaseService *self, GDataFreebaseQuery *query,
			      GCancellable *cancellable, GError **error)
{
	GDataEntry *entry;

	g_return_val_if_fail (GDATA_IS_FREEBASE_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_FREEBASE_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	entry = gdata_service_query_single_entry (GDATA_SERVICE (self), get_freebase_authorization_domain (), "mqlread",
						  GDATA_QUERY (query), GDATA_TYPE_FREEBASE_RESULT, cancellable, error);
	if (entry == NULL)
		return NULL;

	return GDATA_FREEBASE_RESULT (entry);
}

/**
 * gdata_freebase_service_query_async:
 * @self: a #GDataFreebaseService
 * @query: a #GDataFreebaseQuery with the MQL query
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Performs a MQL query on the service. @self and @query are all reffed when this function is called, so can safely
 * be unreffed after this function returns. When the query is replied, or fails, @callback will be executed, and
 * the result can be obtained through gdata_service_query_single_entry_finish().
 *
 * For more details, see gdata_freebase_service_query(), which is the synchronous version of
 * this function.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
void
gdata_freebase_service_query_async (GDataFreebaseService *self, GDataFreebaseQuery *query, GCancellable *cancellable,
				    GAsyncReadyCallback callback, gpointer user_data)
{
	g_return_if_fail (GDATA_IS_FREEBASE_SERVICE (self));
	g_return_if_fail (GDATA_IS_FREEBASE_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	gdata_service_query_single_entry_async (GDATA_SERVICE (self), get_freebase_authorization_domain (), "mqlread",
						GDATA_QUERY (query), GDATA_TYPE_FREEBASE_RESULT, cancellable, callback, user_data);
}

/**
 * gdata_freebase_service_get_topic:
 * @self: a #GDataFreebaseService
 * @query: a #GDataFreebaseTopicQuery containing the topic ID
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: (allow-none): a #GError, or %NULL
 *
 * Queries information about a topic, identified through a Freebase ID. You can find out more about topic queries in the
 * <ulink type="http" url="https://developers.google.com/freebase/v1/topic-response">online documentation</ulink>.
 *
 * Return value: (transfer full): a #GDataFreebaseTopicResult containing information about the topic; unref with g_object_unref()
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GDataFreebaseTopicResult *
gdata_freebase_service_get_topic (GDataFreebaseService *self, GDataFreebaseTopicQuery *query, GCancellable *cancellable, GError **error)
{
	GDataEntry *entry;

	g_return_val_if_fail (GDATA_IS_FREEBASE_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_FREEBASE_TOPIC_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	entry = gdata_service_query_single_entry (GDATA_SERVICE (self), get_freebase_authorization_domain (), "topic",
						  GDATA_QUERY (query), GDATA_TYPE_FREEBASE_TOPIC_RESULT, cancellable, error);
	if (entry == NULL)
		return NULL;

	return GDATA_FREEBASE_TOPIC_RESULT (entry);
}

/**
 * gdata_freebase_service_get_topic_async:
 * @self: a #GDataFreebaseService
 * @query: a #GDataFreebaseQuery with the MQL query
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Queries information about a topic, identified through a Freebase ID. @self and @query are all reffed when this
 * function is called, so can safely be unreffed after this function returns. When the query is replied, or fails,
 * @callback will be executed, and the result can be obtained through gdata_service_query_single_entry_finish().
 *
 * For more details, see gdata_freebase_service_get_topic(), which is the synchronous version of
 * this function.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
void
gdata_freebase_service_get_topic_async (GDataFreebaseService *self, GDataFreebaseTopicQuery *query,
					GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	g_return_if_fail (GDATA_IS_FREEBASE_SERVICE (self));
	g_return_if_fail (GDATA_IS_FREEBASE_TOPIC_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	gdata_service_query_single_entry_async (GDATA_SERVICE (self), get_freebase_authorization_domain (), "topic",
						GDATA_QUERY (query), GDATA_TYPE_FREEBASE_TOPIC_RESULT, cancellable, callback, user_data);
}

/**
 * gdata_freebase_service_search:
 * @self: a #GDataFreebaseService
 * @query: a #GDataFreebaseSearchQuery containing the topic ID
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: (allow-none): a #GError, or %NULL
 *
 * Performs a search for any given search term, filters can be set on @query to narrow down the results. The results returned
 * are ordered by relevance. You can find out more about topic queries in the
 * <ulink type="http" url="https://developers.google.com/freebase/v1/search-cookbook">online documentation</ulink>.
 *
 * Return value: (transfer full): a #GDataFreebaseSearchResult containing the results for the given search query; unref with g_object_unref()
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GDataFreebaseSearchResult *
gdata_freebase_service_search (GDataFreebaseService *self, GDataFreebaseSearchQuery *query, GCancellable *cancellable, GError **error)
{
	GDataEntry *entry;

	g_return_val_if_fail (GDATA_IS_FREEBASE_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_FREEBASE_SEARCH_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	entry = gdata_service_query_single_entry (GDATA_SERVICE (self), get_freebase_authorization_domain (), "search",
						  GDATA_QUERY (query), GDATA_TYPE_FREEBASE_SEARCH_RESULT, cancellable, error);
	if (entry == NULL)
		return NULL;

	return GDATA_FREEBASE_SEARCH_RESULT (entry);
}

/**
 * gdata_freebase_service_search_async:
 * @self: a #GDataFreebaseService
 * @query: a #GDataFreebaseQuery with the MQL query
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Performs a search for any given search term. @self and @query are all reffed when this
 * function is called, so can safely be unreffed after this function returns.
 *
 * For more details, see gdata_freebase_service_search(), which is the synchronous version of
 * this function.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
void
gdata_freebase_service_search_async (GDataFreebaseService *self, GDataFreebaseSearchQuery *query,
				     GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	g_return_if_fail (GDATA_IS_FREEBASE_SERVICE (self));
	g_return_if_fail (GDATA_IS_FREEBASE_SEARCH_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	gdata_service_query_single_entry_async (GDATA_SERVICE (self), get_freebase_authorization_domain (), "search",
						GDATA_QUERY (query), GDATA_TYPE_FREEBASE_SEARCH_RESULT, cancellable, callback, user_data);
}

static gchar *
compose_image_uri (GDataFreebaseTopicValue *value, guint max_width, guint max_height)
{
	GString *uri = g_string_new (IMAGE_URI_PREFIX);
	const GDataFreebaseTopicObject *object;
	gboolean first = TRUE;

	object = gdata_freebase_topic_value_get_object (value);
	g_assert (object != NULL);

	g_string_append (uri, gdata_freebase_topic_object_get_id (object));

#define APPEND_SEP g_string_append_c (uri, first ? '?' : '&'); first = FALSE;

	if (max_width > 0) {
		APPEND_SEP;
		g_string_append_printf (uri, "maxwidth=%d", max_width);
	}

	if (max_height > 0) {
		APPEND_SEP;
		g_string_append_printf (uri, "maxheight=%d", max_height);
	}
#undef APPEND_SEP

	return g_string_free (uri, FALSE);
}

/**
 * gdata_freebase_service_get_image:
 * @self: a #GDataFreebaseService
 * @value: a #GDataFreebaseTopicValue from a topic result
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @max_width: maximum width of the image returned, or 0
 * @max_height: maximum height of the image returned, or 0
 * @error: (allow-none): a #GError, or %NULL
 *
 * Creates an input stream to an image object returned in a topic query. If @max_width and @max_height
 * are unspecified (i.e. set to 0), the image returned will be the smallest available.
 *
 * Return value: (transfer full): a #GInputStream opened to the image; unref with g_object_unref()
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GInputStream *
gdata_freebase_service_get_image (GDataFreebaseService *self, GDataFreebaseTopicValue *value,
				  GCancellable *cancellable, guint max_width, guint max_height, GError **error)
{
	GInputStream *stream;
	gchar *uri;

	g_return_val_if_fail (GDATA_IS_FREEBASE_SERVICE (self), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (value != NULL, NULL);
	g_return_val_if_fail (!error || !*error, NULL);
	g_return_val_if_fail (max_width < 4096 && max_height < 4096, NULL);

	if (!gdata_freebase_topic_value_is_image (value)) {
		g_set_error (error,
			     GDATA_SERVICE_ERROR,
			     GDATA_SERVICE_ERROR_BAD_QUERY_PARAMETER,
			     _("Property ‘%s’ does not hold an image"),
			     gdata_freebase_topic_value_get_property (value));
		return NULL;
	}

	uri = compose_image_uri (value, max_width, max_height);
	stream = gdata_download_stream_new (GDATA_SERVICE (self), get_freebase_authorization_domain (), uri, cancellable);
	g_free (uri);

	return stream;
}

G_GNUC_END_IGNORE_DEPRECATIONS
