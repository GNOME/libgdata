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
 * @stability: Unstable
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
 * Since: UNRELEASED
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-freebase-service.h"
#include "gdata-freebase-result.h"
#include "gdata-service.h"
#include "gdata-private.h"
#include "gdata-query.h"

/* Standards reference at https://developers.google.com/freebase/v1/ */

#define URLBASE "://www.googleapis.com/freebase/v1"

enum {
	PROP_DEVELOPER_KEY = 1
};

struct _GDataFreebaseServicePrivate {
	gchar *developer_key;
};

static void gdata_freebase_service_set_property (GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gdata_freebase_service_get_property (GObject *self, guint prop_id, GValue *value, GParamSpec *pspec);
static void gdata_freebase_service_finalize (GObject *self);
static GList *get_authorization_domains (void);

_GDATA_DEFINE_AUTHORIZATION_DOMAIN (freebase, "freebase", "https" URLBASE)

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

	service_class->get_authorization_domains = get_authorization_domains;

	/**
	 * GDataFreebaseService:developer-key:
	 *
	 * The developer key your application has registered with the Freebase API. For more information, see the <ulink type="http"
	 * url="https://developers.google.com/freebase/v1/how-tos/authorizing">online documentation</ulink>.
	 *
	 * Since: UNRELEASED
	 **/
	g_object_class_install_property (gobject_class, PROP_DEVELOPER_KEY,
	                                 g_param_spec_string ("developer-key",
	                                                      "Developer key", "Your Freebase developer API key.",
	                                                      NULL,
							      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_freebase_service_init (GDataFreebaseService *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_FREEBASE_SERVICE, GDataFreebaseServicePrivate);
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
 * Since: UNRELEASED
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
 * Since: 0.9.0
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
 * Since: UNRELEASED
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
 * Since: UNRELEASED
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
 * Since: UNRELEASED
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
 * Since: UNRELEASED
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
						GDATA_QUERY (query), GDATA_TYPE_FREEBASE_RESULT, cancellable, callback, user_data);
}
