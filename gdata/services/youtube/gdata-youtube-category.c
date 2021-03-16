/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2010 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-youtube-category
 * @short_description: YouTube category element
 * @stability: Stable
 * @include: gdata/services/youtube/gdata-youtube-category.h
 *
 * #GDataYouTubeCategory represents the YouTube-specific customizations to #GDataCategory. For more information,
 * see the <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#YouTube_Category_List">
 * online documentation</ulink>.
 *
 * Since: 0.7.0
 */

#include <glib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-youtube-category.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"

static void gdata_youtube_category_finalize (GObject *object);
static void gdata_youtube_category_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataYouTubeCategoryPrivate {
	gboolean assignable;
	gchar **browsable_regions;
	/*gboolean deprecated; --- "Categories that are neither assignable or browsable are deprecated and are identified as such using the
	 * <yt:deprecated> tag." */
};

enum {
	PROP_IS_ASSIGNABLE = 1,
	PROP_IS_DEPRECATED
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataYouTubeCategory, gdata_youtube_category, GDATA_TYPE_CATEGORY)

static void
gdata_youtube_category_class_init (GDataYouTubeCategoryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_youtube_category_get_property;
	gobject_class->finalize = gdata_youtube_category_finalize;

	parsable_class->parse_xml = parse_xml;
	parsable_class->get_namespaces = get_namespaces;

	/**
	 * GDataYouTubeCategory:is-assignable:
	 *
	 * Whether new videos can be added to the category.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_IS_ASSIGNABLE,
	                                 g_param_spec_boolean ("is-assignable",
	                                                       "Assignable?", "Whether new videos can be added to the category.",
	                                                       FALSE,
	                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataYouTubeCategory:is-deprecated:
	 *
	 * Whether the category is deprecated.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_IS_DEPRECATED,
	                                 g_param_spec_boolean ("is-deprecated",
	                                                       "Deprecated?", "Whether the category is deprecated.",
	                                                       TRUE,
	                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_youtube_category_init (GDataYouTubeCategory *self)
{
	self->priv = gdata_youtube_category_get_instance_private (self);
}

static void
gdata_youtube_category_finalize (GObject *object)
{
	GDataYouTubeCategoryPrivate *priv = GDATA_YOUTUBE_CATEGORY (object)->priv;

	g_strfreev (priv->browsable_regions);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_youtube_category_parent_class)->finalize (object);
}

static void
gdata_youtube_category_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataYouTubeCategoryPrivate *priv = GDATA_YOUTUBE_CATEGORY (object)->priv;

	switch (property_id) {
		case PROP_IS_ASSIGNABLE:
			g_value_set_boolean (value, priv->assignable);
			break;
		case PROP_IS_DEPRECATED:
			g_value_set_boolean (value, (priv->assignable == FALSE && priv->browsable_regions == NULL) ? TRUE : FALSE);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	GDataYouTubeCategory *self = GDATA_YOUTUBE_CATEGORY (parsable);

	if (gdata_parser_is_namespace (node, "http://gdata.youtube.com/schemas/2007") == TRUE) {
		if (xmlStrcmp (node->name, (xmlChar*) "assignable") == 0) {
			/* yt:assignable */
			self->priv->assignable = TRUE;
		} else if (xmlStrcmp (node->name, (xmlChar*) "deprecated") == 0) {
			/* yt:deprecated */
			self->priv->assignable = FALSE;
			g_strfreev (self->priv->browsable_regions);
			self->priv->browsable_regions = NULL;
		} else if (xmlStrcmp (node->name, (xmlChar*) "browsable") == 0) {
			/* yt:browsable */
			xmlChar *regions;

			regions = xmlGetProp (node, (xmlChar*) "regions");
			if (regions == NULL)
				return gdata_parser_error_required_property_missing (node, "regions", error);

			self->priv->browsable_regions = g_strsplit ((gchar*) regions, " ", -1);
			xmlFree (regions);
		} else {
			return GDATA_PARSABLE_CLASS (gdata_youtube_category_parent_class)->parse_xml (parsable, doc, node, user_data, error);
		}
	} else {
		return GDATA_PARSABLE_CLASS (gdata_youtube_category_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}

	return TRUE;
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_youtube_category_parent_class)->get_namespaces (parsable, namespaces);

	g_hash_table_insert (namespaces, (gchar*) "youtube", (gchar*) "http://gdata.youtube.com/schemas/2007");
}

/**
 * gdata_youtube_category_is_assignable:
 * @self: a #GDataYouTubeCategory
 *
 * Gets the #GDataYouTubeCategory:is-assignable property.
 *
 * Return value: whether new videos can be assigned to the category
 *
 * Since: 0.7.0
 */
gboolean
gdata_youtube_category_is_assignable (GDataYouTubeCategory *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_CATEGORY (self), FALSE);
	return self->priv->assignable;
}

/**
 * gdata_youtube_category_is_browsable:
 * @self: a #GDataYouTubeCategory
 * @region: a two-letter region ID
 *
 * Returns whether the category is browsable in the given @region. The list of supported region IDs is
 * <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#Region_specific_feeds">available online</ulink>.
 *
 * Return value: whether the category is browsable in @region
 *
 * Since: 0.7.0
 */
gboolean
gdata_youtube_category_is_browsable (GDataYouTubeCategory *self, const gchar *region)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_CATEGORY (self), FALSE);
	g_return_val_if_fail (region != NULL && *region != '\0', FALSE);

	if (self->priv->browsable_regions != NULL) {
		guint i;

		for (i = 0; self->priv->browsable_regions[i] != NULL; i++) {
			if (strcmp (self->priv->browsable_regions[i], region) == 0)
				return TRUE;
		}
	}

	return FALSE;
}

/**
 * gdata_youtube_category_is_deprecated:
 * @self: a #GDataYouTubeCategory
 *
 * Gets the #GDataYouTubeCategory:is-deprecated property.
 *
 * Return value: whether the category is deprecated
 *
 * Since: 0.7.0
 */
gboolean
gdata_youtube_category_is_deprecated (GDataYouTubeCategory *self)
{
	g_return_val_if_fail (GDATA_IS_YOUTUBE_CATEGORY (self), TRUE);
	return (self->priv->assignable == FALSE && self->priv->browsable_regions == NULL) ? TRUE : FALSE;
}
