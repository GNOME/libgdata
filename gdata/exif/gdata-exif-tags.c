/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Richard Schwarting 2009 <aquarichy@gmail.com>
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

/*
 * SECTION:gdata-exif-tags
 * @short_description: EXIF tags element
 * @stability: Stable
 * @include: gdata/exif/gdata-exif-tags.h
 *
 * #GDataExifTags represents a "tags" element from the
 * <ulink type="http" url="http://schemas.google.com/photos/exif/2007">EXIF specification</ulink>.
 *
 * It is private API, since implementing classes are likely to proxy the properties and functions
 * of #GDataExifTags as appropriate; most entry types which implement #GDataExifTags have no use
 * for most of its properties, and it would be unnecessary and confusing to expose #GDataExifTags itself.
 *
 * Also note that modified EXIF values submitted back to the Google (in an update or on the original
 * upload) appear to be ignored.  Google's EXIF values for the uploaded image will be set to the EXIF
 * metadata found in the image itself.
 *
 * For these reasons, properties have not been implemented on #GDataExifTags (yet).
 *
 * Since: 0.5.0
 */

#include <glib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-exif-tags.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-private.h"

static void gdata_exif_tags_finalize (GObject *object);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataExifTagsPrivate {
	gdouble distance;
	gdouble exposure;
	gboolean flash;
	gdouble focal_length;
	gdouble fstop;
	gchar *image_unique_id;
	gint iso;
	gchar *make;
	gchar *model;
	gint64 _time; /* in milliseconds! */
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataExifTags, gdata_exif_tags, GDATA_TYPE_PARSABLE)

static void
gdata_exif_tags_class_init (GDataExifTagsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->finalize = gdata_exif_tags_finalize;

	parsable_class->parse_xml = parse_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "tags";
	parsable_class->element_namespace = "exif";
}

static void
gdata_exif_tags_init (GDataExifTags *self)
{
	self->priv = gdata_exif_tags_get_instance_private (self);
	self->priv->_time = -1;
}

static void
gdata_exif_tags_finalize (GObject *object)
{
	GDataExifTagsPrivate *priv = GDATA_EXIF_TAGS (object)->priv;

	g_free (priv->make);
	g_free (priv->model);
	g_free (priv->image_unique_id);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_exif_tags_parent_class)->finalize (object);
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	gboolean success;
	GDataExifTags *self = GDATA_EXIF_TAGS (parsable);

	if (gdata_parser_is_namespace (node, "http://schemas.google.com/photos/exif/2007") == FALSE)
		return GDATA_PARSABLE_CLASS (gdata_exif_tags_parent_class)->parse_xml (parsable, doc, node, user_data, error);

	if (xmlStrcmp (node->name, (xmlChar*) "distance") == 0 ) {
		/* exif:distance */
		xmlChar *distance = xmlNodeListGetString (doc, node->children, TRUE);
		self->priv->distance = g_ascii_strtod ((gchar*) distance, NULL);
		xmlFree (distance);
	} else if (xmlStrcmp (node->name, (xmlChar*) "fstop") == 0) {
		/* exif:fstop */
		xmlChar *fstop = xmlNodeListGetString (doc, node->children, TRUE);
		self->priv->fstop = g_ascii_strtod ((gchar*) fstop, NULL);
		xmlFree (fstop);
	} else if (gdata_parser_string_from_element (node, "make", P_NONE, &(self->priv->make), &success, error) == TRUE ||
	           gdata_parser_string_from_element (node, "model", P_NONE, &(self->priv->model), &success, error) == TRUE ||
	           gdata_parser_string_from_element (node, "imageUniqueID", P_NONE, &(self->priv->image_unique_id), &success, error) == TRUE) {
		return success;
	} else if (xmlStrcmp (node->name, (xmlChar*) "exposure") == 0) {
		/* exif:exposure */
		xmlChar *exposure = xmlNodeListGetString (doc, node->children, TRUE);
		self->priv->exposure = g_ascii_strtod ((gchar*) exposure, NULL);
		xmlFree (exposure);
	} else if (xmlStrcmp (node->name, (xmlChar*) "flash") == 0) {
		/* exif:flash */
		xmlChar *flash = xmlNodeListGetString (doc, node->children, TRUE);
		if (flash == NULL)
			return gdata_parser_error_required_content_missing (node, error);
		self->priv->flash = (xmlStrcmp (flash, (xmlChar*) "true") == 0 ? TRUE : FALSE);
		xmlFree (flash);
	} else if (xmlStrcmp (node->name, (xmlChar*) "focallength") == 0) {
		/* exif:focal-length */
		xmlChar *focal_length = xmlNodeListGetString (doc, node->children, TRUE);
		self->priv->focal_length = g_ascii_strtod ((gchar*) focal_length, NULL);
		xmlFree (focal_length);
	} else if (xmlStrcmp (node->name, (xmlChar*) "iso") == 0) {
		/* exif:iso */
		xmlChar *iso = xmlNodeListGetString (doc, node->children, TRUE);
		self->priv->iso = g_ascii_strtoll ((gchar*) iso, NULL, 10);
		xmlFree (iso);
	} else if (xmlStrcmp (node->name, (xmlChar*) "time") == 0) {
		/* exif:time */
		xmlChar *time_str;
		guint64 milliseconds;

		time_str = xmlNodeListGetString (doc, node->children, TRUE);
		milliseconds = g_ascii_strtoull ((gchar*) time_str, NULL, 10);
		xmlFree (time_str);

		self->priv->_time = (gint64) milliseconds;
	} else {
		return GDATA_PARSABLE_CLASS (gdata_exif_tags_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}

	return TRUE;
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "exif", (gchar*) "http://schemas.google.com/photos/exif/2007");
}

/**
 * gdata_exif_tags_get_distance:
 * @self: a #GDataExifTags
 *
 * Gets the #GDataExifTags:distance property.
 *
 * Return value: the distance value, or <code class="literal">-1</code> if unknown
 *
 * Since: 0.5.0
 */
gdouble
gdata_exif_tags_get_distance (GDataExifTags *self)
{
	g_return_val_if_fail (GDATA_IS_EXIF_TAGS (self), -1);
	return self->priv->distance;
}

/**
 * gdata_exif_tags_get_exposure:
 * @self: a #GDataExifTags
 *
 * Gets the #GDataExifTags:exposure property.
 *
 * Return value: the exposure value, or <code class="literal">0</code> if unknown
 *
 * Since: 0.5.0
 */
gdouble
gdata_exif_tags_get_exposure (GDataExifTags *self)
{
	g_return_val_if_fail (GDATA_IS_EXIF_TAGS (self), 0);
	return self->priv->exposure;
}

/**
 * gdata_exif_tags_get_flash:
 * @self: a #GDataExifTags
 *
 * Gets the #GDataExifTags:flash property.
 *
 * Return value: %TRUE if flash was used, %FALSE otherwise
 *
 * Since: 0.5.0
 */
gboolean
gdata_exif_tags_get_flash (GDataExifTags *self)
{
	g_return_val_if_fail (GDATA_IS_EXIF_TAGS (self), FALSE);
	return self->priv->flash;
}

/**
 * gdata_exif_tags_get_focal_length:
 * @self: a #GDataExifTags
 *
 * Gets the #GDataExifTags:focal-length property.
 *
 * Return value: the focal-length value, or <code class="literal">-1</code> if unknown
 *
 * Since: 0.5.0
 */
gdouble
gdata_exif_tags_get_focal_length (GDataExifTags *self)
{
	g_return_val_if_fail (GDATA_IS_EXIF_TAGS (self), -1);
	return self->priv->focal_length;
}

/**
 * gdata_exif_tags_get_fstop:
 * @self: a #GDataExifTags
 *
 * Gets the #GDataExifTags:fstop property.
 *
 * Return value: the F-stop value, or <code class="literal">0</code> if unknown
 *
 * Since: 0.5.0
 */
gdouble
gdata_exif_tags_get_fstop (GDataExifTags *self)
{
	g_return_val_if_fail (GDATA_IS_EXIF_TAGS (self), 0);
	return self->priv->fstop;
}

/**
 * gdata_exif_tags_get_image_unique_id:
 * @self: a #GDataExifTags
 *
 * Gets the #GDataExifTags:image-unique-id property.
 *
 * Return value: the photo's unique EXIF identifier, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_exif_tags_get_image_unique_id (GDataExifTags *self)
{
	g_return_val_if_fail (GDATA_IS_EXIF_TAGS (self), NULL);
	return self->priv->image_unique_id;
}

/**
 * gdata_exif_tags_get_iso:
 * @self: a #GDataExifTags
 *
 * Gets the #GDataExifTags:iso property.
 *
 * Return value: the ISO speed, or <code class="literal">-1</code> if unknown
 *
 * Since: 0.5.0
 */
gint
gdata_exif_tags_get_iso (GDataExifTags *self)
{
	g_return_val_if_fail (GDATA_IS_EXIF_TAGS (self), -1);
	return self->priv->iso;
}

/**
 * gdata_exif_tags_get_make:
 * @self: a #GDataExifTags
 *
 * Gets the #GDataExifTags:make property.
 *
 * Return value: the name of the manufacturer of the camera, or %NULL if unknown
 *
 * Since: 0.5.0
 */
const gchar *
gdata_exif_tags_get_make (GDataExifTags *self)
{
	g_return_val_if_fail (GDATA_IS_EXIF_TAGS (self), NULL);
	return self->priv->make;
}

/**
 * gdata_exif_tags_get_model:
 * @self: a #GDataExifTags
 *
 * Gets the #GDataExifTags:model property.
 *
 * Return value: the model name of the camera, or %NULL if unknown
 *
 * Since: 0.5.0
 */
const gchar *
gdata_exif_tags_get_model (GDataExifTags *self)
{
	g_return_val_if_fail (GDATA_IS_EXIF_TAGS (self), NULL);
	return self->priv->model;
}

/**
 * gdata_exif_tags_get_time:
 * @self: a #GDataExifTags
 *
 * Gets the #GDataExifTags:time property as a number of milliseconds since the epoch. If the property is unset, <code class="literal">-1</code> will
 * be returned.
 *
 * Return value: the UNIX timestamp for the time property in milliseconds, or <code class="literal">-1</code>
 *
 * Since: 0.5.0
 */
gint64
gdata_exif_tags_get_time (GDataExifTags *self)
{
	g_return_val_if_fail (GDATA_IS_EXIF_TAGS (self), -1);
	return self->priv->_time;
}
