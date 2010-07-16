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
 * SECTION:gdata-app-categories
 * @short_description: GData APP categories object
 * @stability: Unstable
 * @include: gdata/app/gdata-app-categories.h
 *
 * #GDataAPPCategories is a list of categories (#GDataCategory) returned as the result of querying an
 * <ulink type="http" url="http://www.atomenabled.org/developers/protocol/#category">Atom Publishing Protocol Category Document</ulink>.
 *
 * Since: 0.7.0
 **/

#include <config.h>
#include <glib.h>
#include <libxml/parser.h>

#include "gdata-app-categories.h"
#include "atom/gdata-category.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"

static void gdata_app_categories_dispose (GObject *object);
static void gdata_app_categories_finalize (GObject *object);
static void gdata_app_categories_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static gboolean post_parse_xml (GDataParsable *parsable, gpointer user_data, GError **error);

struct _GDataAPPCategoriesPrivate {
	GList *categories;
	gchar *scheme;
	gboolean fixed;
};

enum {
	PROP_IS_FIXED = 1
};

G_DEFINE_TYPE (GDataAPPCategories, gdata_app_categories, GDATA_TYPE_PARSABLE)

static void
gdata_app_categories_class_init (GDataAPPCategoriesClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataAPPCategoriesPrivate));

	gobject_class->get_property = gdata_app_categories_get_property;
	gobject_class->dispose = gdata_app_categories_dispose;
	gobject_class->finalize = gdata_app_categories_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->parse_xml = parse_xml;
	parsable_class->post_parse_xml = post_parse_xml;
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
	 **/
	g_object_class_install_property (gobject_class, PROP_IS_FIXED,
	                                 g_param_spec_boolean ("is-fixed",
	                                                       "Fixed?", "Whether entries may use categories not in this category list.",
	                                                       FALSE,
	                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_app_categories_init (GDataAPPCategories *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_APP_CATEGORIES, GDataAPPCategoriesPrivate);
}

static void
gdata_app_categories_dispose (GObject *object)
{
	GDataAPPCategoriesPrivate *priv = GDATA_APP_CATEGORIES (object)->priv;

	if (priv->categories != NULL) {
		g_list_foreach (priv->categories, (GFunc) g_object_unref, NULL);
		g_list_free (priv->categories);
	}
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

static gboolean
pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error)
{
	GDataAPPCategoriesPrivate *priv = GDATA_APP_CATEGORIES (parsable)->priv;
	xmlChar *fixed;

	/* Extract fixed and scheme */
	priv->scheme = (gchar*) xmlGetProp (root_node, (xmlChar*) "scheme");

	fixed = xmlGetProp (root_node, (xmlChar*) "fixed");
	if (xmlStrcmp (fixed, (xmlChar*) "yes") == 0)
		priv->fixed = TRUE;
	xmlFree (fixed);

	return TRUE;
}

static void
_gdata_app_categories_add_category (GDataAPPCategories *self, GDataCategory *category)
{
	g_return_if_fail (GDATA_IS_APP_CATEGORIES (self));
	g_return_if_fail (GDATA_IS_CATEGORY (category));

	/* If the category doesn't have a scheme, make it inherit ours */
	if (gdata_category_get_scheme (category) == NULL)
		gdata_category_set_scheme (category, self->priv->scheme);

	self->priv->categories = g_list_prepend (self->priv->categories, g_object_ref (category));
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	gboolean success;

	if (gdata_parser_is_namespace (node, "http://www.w3.org/2005/Atom") == TRUE &&
	    gdata_parser_object_from_element_setter (node, "category", P_REQUIRED,
	                                             (user_data == NULL) ? GDATA_TYPE_CATEGORY : GPOINTER_TO_SIZE (user_data),
		                                     _gdata_app_categories_add_category, GDATA_APP_CATEGORIES (parsable), &success, error) == TRUE) {
		return success;
	}

	return GDATA_PARSABLE_CLASS (gdata_app_categories_parent_class)->parse_xml (parsable, doc, node, user_data, error);
}

static gboolean
post_parse_xml (GDataParsable *parsable, gpointer user_data, GError **error)
{
	GDataAPPCategoriesPrivate *priv = GDATA_APP_CATEGORIES (parsable)->priv;

	/* Reverse our lists of stuff */
	priv->categories = g_list_reverse (priv->categories);

	return TRUE;
}

/**
 * gdata_app_categories_get_categories:
 * @self: a #GDataAPPCategories
 *
 * Returns a list of the categories in this category list.
 *
 * Return value: (element-type GData.Category) (transfer none): a #GList of #GDataCategory<!-- -->s
 *
 * Since: 0.7.0
 **/
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
 **/
gboolean
gdata_app_categories_is_fixed (GDataAPPCategories *self)
{
	g_return_val_if_fail (GDATA_IS_APP_CATEGORIES (self), FALSE);
	return self->priv->fixed;
}
