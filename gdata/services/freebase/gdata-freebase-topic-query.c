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
 * SECTION:gdata-freebase-topic-query
 * @short_description: GData Freebase topic query object
 * @stability: Stable
 * @include: gdata/services/freebase/gdata-freebase-topic-query.h
 *
 * #GDataFreebaseTopicQuery represents a Freebase topic query. The topic query happens on a single Freebase ID,
 * given in gdata_freebase_topic_query_new(), the reply returns all known information in Freebase for that given ID.
 * For more documentation and examples, see the <ulink type="http" url="https://developers.google.com/freebase/v1/topic-response">
 * Topic response API documentation</ulink>
 *
 * This implementation of #GDataQuery respects the gdata_query_set_max_results() and gdata_query_set_updated_max() calls.
 *
 * For more details of Google Freebase API, see the <ulink type="http" url="https://developers.google.com/freebase/v1/">
 * online documentation</ulink>.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>
#include <json-glib/json-glib.h>

#include "gdata-freebase-topic-query.h"
#include "gdata-query.h"
#include "gdata-parser.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void gdata_freebase_topic_query_finalize (GObject *self);
static void gdata_freebase_topic_query_set_property (GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gdata_freebase_topic_query_get_property (GObject *self, guint prop_id, GValue *value, GParamSpec *pspec);
static void get_query_uri (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started);

struct _GDataFreebaseTopicQueryPrivate {
	gchar *lang;
	gchar **filter;
};

enum {
	PROP_LANGUAGE = 1,
	PROP_FILTER
};

G_DEFINE_TYPE (GDataFreebaseTopicQuery, gdata_freebase_topic_query, GDATA_TYPE_QUERY)

static void
gdata_freebase_topic_query_class_init (GDataFreebaseTopicQueryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataQueryClass *query_class = GDATA_QUERY_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataFreebaseTopicQueryPrivate));

	gobject_class->finalize = gdata_freebase_topic_query_finalize;
	gobject_class->set_property = gdata_freebase_topic_query_set_property;
	gobject_class->get_property = gdata_freebase_topic_query_get_property;

	query_class->get_query_uri = get_query_uri;

	/**
	 * GDataFreebaseTopicQuery:language:
	 *
	 * Language used for topic values in the result, in ISO-639-1 format.
	 *
	 * Since: 0.15.1
	 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
	 */
	g_object_class_install_property (gobject_class, PROP_LANGUAGE,
	                                 g_param_spec_string ("language",
							      "Language used for results",
							      "Language in ISO-639-1 format.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
							      G_PARAM_DEPRECATED));

	/**
	 * GDataFreebaseTopicQuery:filter:
	 *
	 * Array of properties (eg. "/common/topic/description", or "/computer/software/first_released"), or property
	 * domains (eg. "/common/topic", or "/computer") to be used as filter.
	 *
	 * Since: 0.15.1
	 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
	 */
	g_object_class_install_property (gobject_class, PROP_FILTER,
					 g_param_spec_boxed ("filter",
							     "Filter",
							     "Property domain to be used as filter",
							     G_TYPE_STRV,
							     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_freebase_topic_query_init (GDataFreebaseTopicQuery *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_FREEBASE_TOPIC_QUERY, GDataFreebaseTopicQueryPrivate);
}

static void
gdata_freebase_topic_query_finalize (GObject *self)
{
	GDataFreebaseTopicQueryPrivate *priv = GDATA_FREEBASE_TOPIC_QUERY (self)->priv;

	g_free (priv->lang);
	g_free (priv->filter);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_freebase_topic_query_parent_class)->finalize (self);
}

static void
gdata_freebase_topic_query_set_property (GObject *self, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GDataFreebaseTopicQuery *query = GDATA_FREEBASE_TOPIC_QUERY (self);

	switch (prop_id) {
	case PROP_LANGUAGE:
		gdata_freebase_topic_query_set_language (query, g_value_get_string (value));
		break;
	case PROP_FILTER:
		gdata_freebase_topic_query_set_filter (query, g_value_get_boxed (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
		break;
	}
}

static void
gdata_freebase_topic_query_get_property (GObject *self, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GDataFreebaseTopicQueryPrivate *priv = GDATA_FREEBASE_TOPIC_QUERY (self)->priv;

	switch (prop_id) {
	case PROP_LANGUAGE:
		g_value_set_string (value, priv->lang);
		break;
	case PROP_FILTER:
		g_value_set_boxed (value, priv->filter);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (self, prop_id, pspec);
		break;
	}
}

static void
get_query_uri (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started)
{
	GDataFreebaseTopicQueryPrivate *priv = GDATA_FREEBASE_TOPIC_QUERY (self)->priv;
	const gchar *lang = NULL;
	gint64 updated_max;
	guint limit;

	g_string_append (query_uri, gdata_query_get_q (self));

#define APPEND_SEP g_string_append_c (query_uri, (*params_started == FALSE) ? '?' : '&'); *params_started = TRUE;

	if (priv->lang != NULL) {
		lang = priv->lang;
	} else {
		const gchar * const *user_languages;
		guint i;

		user_languages = g_get_language_names ();

		/* Pick the first user language */
		for (i = 0; user_languages[i] != NULL; i++) {
			if (strlen (user_languages[i]) == 2) {
				lang = user_languages[i];
				break;
			}
		}
	}

	APPEND_SEP;
	g_string_append (query_uri, "lang=");
	g_string_append (query_uri, lang);

	if (priv->filter) {
		guint i;

		for (i = 0; priv->filter[i] != NULL; i++) {
			APPEND_SEP;
			g_string_append (query_uri, "filter=");
			g_string_append (query_uri, priv->filter[i]);
		}
	}

	updated_max = gdata_query_get_updated_max (self);

	if (updated_max > -1) {
		APPEND_SEP;
		g_string_append_printf (query_uri, "dateline=%" G_GINT64_FORMAT, updated_max);
	}

	limit = gdata_query_get_max_results (self);

	if (limit > 0) {
		APPEND_SEP;
		g_string_append_printf (query_uri, "limit=%d", limit);
	}

	/* We don't chain up with parent class get_query_uri because it uses
	 *  GData protocol parameters and they aren't compatible with newest API family
	 */
#undef APPEND_SEP
}

/**
 * gdata_freebase_topic_query_new:
 * @id: a Freebase ID or MID
 *
 * Creates a new #GDataFreebaseTopicQuery for the given Freebase ID. Those can be
 * obtained programmatically through gdata_freebase_search_result_item_get_id() or
 * embedded in the result of a gdata_freebase_service_query() call.
 *
 * Return value: (transfer full): a new #GDataFreebaseTopicQuery
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GDataFreebaseTopicQuery *
gdata_freebase_topic_query_new (const gchar *id)
{
	g_return_val_if_fail (id != NULL, NULL);
	return g_object_new (GDATA_TYPE_FREEBASE_TOPIC_QUERY, "q", id, NULL);
}

/**
 * gdata_freebase_topic_query_set_language:
 * @self: a #GDataFreebaseTopicQuery
 * @lang: (allow-none): language used on the topic query, in ISO-639-1 format, or %NULL to unset the language
 *
 * Sets the language used in the topic query. If unset,
 * the locale preferences will be respected.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
void
gdata_freebase_topic_query_set_language (GDataFreebaseTopicQuery *self,
					 const gchar             *lang)
{
	GDataFreebaseTopicQueryPrivate *priv;

	g_return_if_fail (GDATA_IS_FREEBASE_TOPIC_QUERY (self));
	g_return_if_fail (lang == NULL || strlen (lang) == 2);

	priv = self->priv;

	if (g_strcmp0 (priv->lang, lang) == 0)
		return;

	g_free (priv->lang);
	priv->lang = g_strdup (lang);
	g_object_notify (G_OBJECT (self), "language");
}

/**
 * gdata_freebase_topic_query_get_language:
 * @self: a #GDataFreebaseTopicQuery
 *
 * Gets the language set on the topic query, or %NULL if unset.
 *
 * Return value: (allow-none): The language used on the query.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const gchar *
gdata_freebase_topic_query_get_language (GDataFreebaseTopicQuery *self)
{
	g_return_val_if_fail (GDATA_IS_FREEBASE_TOPIC_QUERY (self), NULL);

	return self->priv->lang;
}

/**
 * gdata_freebase_topic_query_set_filter:
 * @self: a #GDataFreebaseTopicQuery
 * @filter:  (array zero-terminated=1) (allow-none): %NULL-terminated array of filter strings, or %NULL to unset
 *
 * Sets a filter on the properties to be returned by the #GDataFreebaseTopicQuery, a filter string usually contains either
 * a specific property (eg. "/common/topic/description", or "/computer/software/first_released"), or a property domain
 * (eg. "/common/topic", or "/computer"), all properties pertaining to the domain will be returned through the
 * #GDataFreebaseTopicResult in the latter case. Other special strings can be passed as filter strings, those are documented
 * in the <ulink type="http" url="https://developers.google.com/freebase/v1/topic-overview#filter">Topic API overview</ulink>
 *
 * If multiple filter strings are set, the result will contain all information necessary to satisfy each of those individually.
 * If no filter is set, the "commons" special value will be implicitly assumed, which returns a reasonably complete data set.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
void
gdata_freebase_topic_query_set_filter (GDataFreebaseTopicQuery *self, const gchar * const *filter)
{
	GDataFreebaseTopicQueryPrivate *priv;

	g_return_if_fail (GDATA_IS_FREEBASE_TOPIC_QUERY (self));
	priv = self->priv;

	g_strfreev (priv->filter);
	priv->filter = g_strdupv ((gchar **) filter);
	g_object_notify (G_OBJECT (self), "filter");
}

/**
 * gdata_freebase_topic_query_get_filter:
 * @self: a #GDataFreebaseTopicQuery
 *
 * Gets the filter set on the topic query, or %NULL if unset.
 *
 * Return value: (array zero-terminated=1) (transfer none) (allow-none): The filter used on the query.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const gchar * const *
gdata_freebase_topic_query_get_filter (GDataFreebaseTopicQuery *self)
{
	g_return_val_if_fail (GDATA_IS_FREEBASE_TOPIC_QUERY (self), NULL);

	return (const gchar * const *) self->priv->filter;
}

G_GNUC_END_IGNORE_DEPRECATIONS
