/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2010, 2015 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-app-categories
 * @short_description: GData APP categories object
 * @stability: Stable
 * @include: gdata/app/gdata-app-categories.h
 *
 * #GDataAPPCategories is a list of categories (#GDataCategory) returned as the result of querying an
 * <ulink type="http" url="http://www.atomenabled.org/developers/protocol/#category">Atom Publishing Protocol Category Document</ulink>.
 *
 * Since: 0.7.0
 */

#include <config.h>
#include <glib.h>

#include "gdata-app-categories.h"
#include "atom/gdata-category.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"

static void gdata_app_categories_dispose (GObject *object);
static void gdata_app_categories_finalize (GObject *object);
static void gdata_app_categories_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data,
            GError **error);
static gboolean
post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error);
static const gchar *
get_content_type (void);

struct _GDataAPPCategoriesPrivate {
	GList *categories;
	gchar *scheme;
	gboolean fixed;
};

enum {
	PROP_IS_FIXED = 1
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataAPPCategories, gdata_app_categories, GDATA_TYPE_PARSABLE)

static void
gdata_app_categories_class_init (GDataAPPCategoriesClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_app_categories_get_property;
	gobject_class->dispose = gdata_app_categories_dispose;
	gobject_class->finalize = gdata_app_categories_finalize;

	parsable_class->parse_json = parse_json;
	parsable_class->post_parse_json = post_parse_json;
	parsable_class->get_content_type = get_content_type;

	parsable_class->element_name = "categories";
	parsable_class->element_namespace = "app";

	/**
	 * GDataAPPCategories:is-fixed:
	 *
	 * Whether entries may use categories not in this category list.
	 *
	 * API reference: <ulink type="http" url="http://www.atomenabled.org/developers/protocol/#appCategories2">app:categories</ulink>
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_IS_FIXED,
	                                 g_param_spec_boolean ("is-fixed",
	                                                       "Fixed?", "Whether entries may use categories not in this category list.",
	                                                       FALSE,
	                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_app_categories_init (GDataAPPCategories *self)
{
	self->priv = gdata_app_categories_get_instance_private (self);
}

static void
gdata_app_categories_dispose (GObject *object)
{
	GDataAPPCategoriesPrivate *priv = GDATA_APP_CATEGORIES (object)->priv;

	g_list_free_full (priv->categories, g_object_unref);
	priv->categories = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_app_categories_parent_class)->dispose (object);
}

static void
gdata_app_categories_finalize (GObject *object)
{
	GDataAPPCategoriesPrivate *priv = GDATA_APP_CATEGORIES (object)->priv;

	g_free (priv->scheme);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_app_categories_parent_class)->finalize (object);
}

static void
gdata_app_categories_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataAPPCategoriesPrivate *priv = GDATA_APP_CATEGORIES (object)->priv;

	switch (property_id) {
		case PROP_IS_FIXED:
			g_value_set_boolean (value, priv->fixed);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

/* Reference: https://developers.google.com/youtube/v3/docs/videoCategories/list */
static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	GDataAPPCategories *self = GDATA_APP_CATEGORIES (parsable);
	GDataAPPCategoriesPrivate *priv = self->priv;
	GType category_type;

	category_type = (user_data == NULL) ?
		GDATA_TYPE_CATEGORY : GPOINTER_TO_SIZE (user_data);

	if (g_strcmp0 (json_reader_get_member_name (reader), "items") == 0) {
		guint i, elements;

		/* Loop through the elements array. */
		for (i = 0, elements = (guint) json_reader_count_elements (reader); i < elements; i++) {
			GDataCategory *category = NULL;
			const gchar *id, *title;
			const GError *child_error = NULL;

			json_reader_read_element (reader, i);

			json_reader_read_member (reader, "id");
			id = json_reader_get_string_value (reader);
			json_reader_end_member (reader);

			json_reader_read_member (reader, "snippet");

			json_reader_read_member (reader, "title");
			title = json_reader_get_string_value (reader);
			json_reader_end_member (reader);

			child_error = json_reader_get_error (reader);

			if (child_error != NULL) {
				return gdata_parser_error_from_json_error (reader,
				                                           child_error,
				                                           error);
			}

			/* Create the category. */
			category = g_object_new (category_type,
			                         "term", id,
			                         "label", title,
			                         NULL);
			priv->categories = g_list_prepend (priv->categories,
			                                   category);

			json_reader_end_member (reader);  /* snippet */
			json_reader_end_element (reader);  /* category */
		}

		return TRUE;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "kind") == 0 ||
	           g_strcmp0 (json_reader_get_member_name (reader), "etag") == 0 ||
	           g_strcmp0 (json_reader_get_member_name (reader), "id") == 0) {
		/* Ignore. */
		return TRUE;
	} else {
		return GDATA_PARSABLE_CLASS (gdata_app_categories_parent_class)->parse_json (parsable, reader, user_data, error);
	}
}

static gboolean
post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error)
{
	GDataAPPCategoriesPrivate *priv = GDATA_APP_CATEGORIES (parsable)->priv;

	/* Reverse our lists of stuff. */
	priv->categories = g_list_reverse (priv->categories);

	return TRUE;
}

static const gchar *
get_content_type (void)
{
	return "application/json";
}

/**
 * gdata_app_categories_get_categories:
 * @self: a #GDataAPPCategories
 *
 * Returns a list of the categories in this category list.
 *
 * Return value: (element-type GData.Category) (transfer none): a #GList of #GDataCategorys
 *
 * Since: 0.7.0
 */
GList *
gdata_app_categories_get_categories (GDataAPPCategories *self)
{
	g_return_val_if_fail (GDATA_IS_APP_CATEGORIES (self), NULL);
	return self->priv->categories;
}

/**
 * gdata_app_categories_is_fixed:
 * @self: a #GDataAPPCategories
 *
 * Returns #GDataAPPCategories:is-fixed.
 *
 * Return value: whether entries may use categories not in this category list
 *
 * Since: 0.7.0
 */
gboolean
gdata_app_categories_is_fixed (GDataAPPCategories *self)
{
	g_return_val_if_fail (GDATA_IS_APP_CATEGORIES (self), FALSE);
	return self->priv->fixed;
}
