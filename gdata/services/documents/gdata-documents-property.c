/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Mayank Sharma <mayank8019@gmail.com>
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
 * SECTION:gdata-documents-property
 * @short_description: GData Documents property object
 * @stability: Unstable
 * @include: gdata/gdata-property.h
 *
 * #GDataDocumentsProperty is a subclass of #GDataParsable and represents a Google Drive Property Resource on a file object.
 *
 * It allows applications to store additional metadata on a file, such as tags, IDs from other data stores, viewing preferences etc. Properties can be used to share metadata between applications, for example, in a workflow application.
 *
 * Each #GDataDocumentsProperty is characterized by a key-value pair (where value is optional, and takes empty string "" by default) and a visibility parameter. The visibility can take values "PUBLIC" for public properties and "PRIVATE" for private properties (default). Private properties are accessible only by the application which set them, but public properties can be read/written by other applications as well.
 *
 * Since: 0.17.11
 */

#include <glib.h>
#include <gdata/gdata-parsable.h>
#include <json-glib/json-glib.h>

#include "gdata-documents-property.h"
#include "gdata-parser.h"
#include "gdata-comparable.h"

static void gdata_documents_property_comparable_init (GDataComparableIface *iface);
static void gdata_documents_property_finalize (GObject *object);
static void gdata_documents_property_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_documents_property_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void _gdata_documents_property_set_key (GDataDocumentsProperty *self, const gchar *key);
static void _gdata_documents_property_set_etag (GDataDocumentsProperty *self, const gchar *etag);

static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);
static void get_json (GDataParsable *parsable, JsonBuilder *builder);

struct _GDataDocumentsPropertyPrivate {
	gchar *key;
	gchar *etag;
	gchar *value;		/* default - empty string ("") */
	gchar *visibility;	/* default - "PRIVATE" */
};

enum {
	PROP_KEY = 1,
	PROP_ETAG,
	PROP_VALUE,
	PROP_VISIBILITY
};

G_DEFINE_TYPE_WITH_CODE (GDataDocumentsProperty, gdata_documents_property, GDATA_TYPE_PARSABLE,
                         G_ADD_PRIVATE (GDataDocumentsProperty)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMPARABLE, gdata_documents_property_comparable_init))

static void
gdata_documents_property_class_init (GDataDocumentsPropertyClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->set_property = gdata_documents_property_set_property;
	gobject_class->get_property = gdata_documents_property_get_property;
	gobject_class->finalize = gdata_documents_property_finalize;

	parsable_class->parse_json = parse_json;
	parsable_class->get_json = get_json;
	parsable_class->element_name = "property";

	/**
	 * GDataDocumentsProperty:key:
	 *
	 * The key of this property.
	 *
	 * For more information, see the <ulink type="http" url="https://developers.google.com/drive/api/v2/reference/properties">Properties Resource</ulink>
	 *
	 * Since: 0.17.11
	 */
	g_object_class_install_property (gobject_class, PROP_KEY,
					 g_param_spec_string ("key",
							      "Key", "The key of this property.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsProperty:etag:
	 *
	 * ETag of the property.
	 *
	 * For more information, see the <ulink type="http" url="https://developers.google.com/drive/api/v2/reference/properties">Properties Resource</ulink>
	 *
	 * Since: 0.17.11
	 */
	g_object_class_install_property (gobject_class, PROP_ETAG,
					 g_param_spec_string ("etag",
							      "ETag", "ETag of the property.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsProperty:value:
	 *
	 * The value of this property. By default, it takes the an empty string ("").
	 *
	 * For more information, see the <ulink type="http" url="https://developers.google.com/drive/api/v2/reference/properties">Properties Resource</ulink>
	 *
	 * Since: 0.17.11
	 */
	g_object_class_install_property (gobject_class, PROP_VALUE,
					 g_param_spec_string ("value",
							      "Value", "The value of this property.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsProperty:visibility:
	 *
	 * The visibility status of this property. The default value of
	 * visibility is PRIVATE on a Drive Properties Resource object,
	 * hence #GDataDocumentsProperty:visibility is %GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE
	 * by default. A private property restricts its visibility to only the app which created it.
	 *
	 * For more information, see the <ulink type="http" url="https://developers.google.com/drive/api/v2/reference/properties">Properties Resource</ulink>
	 *
	 * Since: 0.17.11
	 */
	g_object_class_install_property (gobject_class, PROP_VISIBILITY,
					 g_param_spec_string ("visibility",
							      "Visibility", "The visibility of this property.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static gint
compare_with (GDataComparable *self, GDataComparable *other)
{

	GDataDocumentsPropertyPrivate *a = ((GDataDocumentsProperty*) self)->priv, *b = ((GDataDocumentsProperty*) other)->priv;

	if (g_strcmp0 (a->key, b->key) == 0 && g_strcmp0 (a->visibility, b->visibility) == 0)
		return 0;
	return 1;
}

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	gchar *output_val = NULL;
	gboolean success = TRUE, is_key_parsed = FALSE;

	if (gdata_parser_string_from_json_member (reader, "key", P_DEFAULT, &output_val, &success, error) == TRUE) {
		if (success && output_val != NULL && output_val[0] != '\0') {
			_gdata_documents_property_set_key (GDATA_DOCUMENTS_PROPERTY (parsable), output_val);
		}

		is_key_parsed = TRUE;
	} else if (gdata_parser_string_from_json_member (reader, "etag", P_DEFAULT, &output_val, &success, error) == TRUE) {
		if (success && output_val != NULL && output_val[0] != '\0') {
			_gdata_documents_property_set_etag (GDATA_DOCUMENTS_PROPERTY (parsable), output_val);
		}

		is_key_parsed = TRUE;
	} else if (gdata_parser_string_from_json_member (reader, "value", P_DEFAULT, &output_val, &success, error) == TRUE) {

		/* A Property can have a value field to be an empty string, but
		 * never NULL */
		if (success && output_val != NULL) {
			gdata_documents_property_set_value (GDATA_DOCUMENTS_PROPERTY (parsable), output_val);
		}

		is_key_parsed = TRUE;
	} else if (gdata_parser_string_from_json_member (reader, "visibility", P_REQUIRED | P_NON_EMPTY, &output_val, &success, error) == TRUE) {
		gdata_documents_property_set_visibility (GDATA_DOCUMENTS_PROPERTY (parsable), output_val);

		is_key_parsed = TRUE;
	}

	if (is_key_parsed) {
		g_free (output_val);
		return success;
	}

	/* Chain up to the parent class */
	return GDATA_PARSABLE_CLASS (gdata_documents_property_parent_class)->parse_json (parsable, reader, user_data, error);
}

static void
get_json (GDataParsable *parsable, JsonBuilder *builder)
{

	GDataDocumentsPropertyPrivate *priv = GDATA_DOCUMENTS_PROPERTY (parsable)->priv;

	/* Add all the Property specific JSON members */
	g_assert (priv->key != NULL);
	json_builder_set_member_name (builder, "key");
	json_builder_add_string_value (builder, priv->key);

	if (gdata_documents_property_get_etag (GDATA_DOCUMENTS_PROPERTY (parsable)) != NULL) {
		json_builder_set_member_name (builder, "etag");
		json_builder_add_string_value (builder, priv->etag);
	}

	/* Setting the "value" field of a Property Resource deletes that Property Resource */
	json_builder_set_member_name (builder, "value");
	json_builder_add_string_value (builder, priv->value);

	json_builder_set_member_name (builder, "visibility");
	json_builder_add_string_value (builder, priv->visibility);
}

static void
gdata_documents_property_comparable_init (GDataComparableIface *iface)
{
	iface->compare_with = compare_with;
}

static void
gdata_documents_property_init (GDataDocumentsProperty *self)
{
	self->priv = gdata_documents_property_get_instance_private (self);

	/* Google Drive sets the default value of a Property Resource to be an empty string (""),
	 * and visibility is %GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE by default */
	self->priv->value = g_strdup ("");
	self->priv->visibility = g_strdup (GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE);
}

static void
gdata_documents_property_finalize (GObject *object)
{
	GDataDocumentsPropertyPrivate *priv = GDATA_DOCUMENTS_PROPERTY (object)->priv;

	g_free (priv->key);
	g_free (priv->etag);
	g_free (priv->value);
	g_free (priv->visibility);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_documents_property_parent_class)->finalize (object);
}

static void
gdata_documents_property_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataDocumentsPropertyPrivate *priv = GDATA_DOCUMENTS_PROPERTY (object)->priv;

	switch (property_id) {
		case PROP_KEY:
			g_value_set_string (value, priv->key);
			break;
		case PROP_ETAG:
			g_value_set_string (value, priv->etag);
			break;
		case PROP_VALUE:
			g_value_set_string (value, priv->value);
			break;
		case PROP_VISIBILITY:
			g_value_set_string (value, priv->visibility);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_documents_property_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataDocumentsProperty *self = GDATA_DOCUMENTS_PROPERTY (object);

	switch (property_id) {
		case PROP_KEY:
			_gdata_documents_property_set_key (self, g_value_get_string (value));
			break;
		case PROP_ETAG:
			_gdata_documents_property_set_etag (self, g_value_get_string (value));
			break;
		case PROP_VALUE:
			gdata_documents_property_set_value (self, g_value_get_string (value));
			break;
		case PROP_VISIBILITY:
			gdata_documents_property_set_visibility (self, g_value_get_string (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

/**
 * gdata_documents_property_new:
 * @key: the property's key
 *
 * Creates a new #GDataEntry with the given ID and default properties.
 *
 * Return value: (transfer full): a new #GDataDocumentsProperty; unref with g_object_unref()
 *
 * Since: 0.17.11
 */
GDataDocumentsProperty *
gdata_documents_property_new (const gchar *key)
{
	/* GDataDocumentsProperty must have a non NULL key at initialization time,
	 * rest of the properties can be NULL or take their default values. */
	g_return_val_if_fail (key != NULL && key[0] != '\0', NULL);

	return GDATA_DOCUMENTS_PROPERTY (g_object_new (GDATA_TYPE_DOCUMENTS_PROPERTY, "key", key, NULL));
}

/**
 * gdata_documents_property_get_key:
 * @self: a #GDataDocumentsProperty
 *
 * Returns the key of the property. This will never be %NULL or an empty string ("").
 *
 * Return value: (transfer none): the property's key
 *
 * Since: 0.17.11
 */
const gchar *
gdata_documents_property_get_key (GDataDocumentsProperty *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_PROPERTY (self), NULL);
	return self->priv->key;
}

static void
_gdata_documents_property_set_key (GDataDocumentsProperty *self, const gchar *key)
{
	/* This is a READ-ONLY PROPERTY */
	g_return_if_fail (GDATA_IS_DOCUMENTS_PROPERTY (self));
	g_return_if_fail (key != NULL && *key != '\0');

	g_free (self->priv->key);
	self->priv->key = g_strdup (key);

	g_object_notify (G_OBJECT (self), "key");
}

/**
 * gdata_documents_property_get_etag:
 * @self: a #GDataDocumentsProperty
 *
 * Returns the ETag of the property.
 *
 * Return value: (transfer none): the property's ETag. The ETag will never be empty; it's either %NULL or a valid ETag.
 *
 * Since: 0.17.11
 */
const gchar *
gdata_documents_property_get_etag (GDataDocumentsProperty *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_PROPERTY (self), NULL);
	return self->priv->etag;
}

static void
_gdata_documents_property_set_etag (GDataDocumentsProperty *self, const gchar *etag)
{
	g_return_if_fail (GDATA_IS_DOCUMENTS_PROPERTY (self));

	if (g_strcmp0 (self->priv->etag, etag) != 0) {
		g_free (self->priv->etag);
		self->priv->etag = g_strdup (etag);
		g_object_notify (G_OBJECT (self), "etag");
	}
}

/**
 * gdata_documents_property_get_value:
 * @self: a #GDataDocumentsProperty
 *
 * Returns the value of the property.
 *
 * In the case that this value is %NULL, the Property Resource corresponding to @self will be deleted from the properties array on a file's metadata, whereas in the case that it's empty string (""), it will be set as it is.
 *
 * Return value: (nullable): the property's value. This can be %NULL or empty.
 *
 * Since: 0.17.11
 */
const gchar *
gdata_documents_property_get_value (GDataDocumentsProperty *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_PROPERTY (self), NULL);
	return self->priv->value;
}

/**
 * gdata_documents_property_set_value:
 * @self: a #GDataDocumentsProperty
 * @value: (allow-none): the new value of the property
 *
 * Sets #GDataDocumentsProperty:value to @value, corresponding to the key.
 *
 * In the case that @value is %NULL, the Property Resource corresponding to @self will be deleted from the properties array on a file's metadata, whereas in the case that it's empty string (""), it will be set as it is.
 *
 * Since: 0.17.11
 */
void
gdata_documents_property_set_value (GDataDocumentsProperty *self, const gchar *value)
{
	g_return_if_fail (GDATA_IS_DOCUMENTS_PROPERTY (self));

	if (g_strcmp0 (self->priv->value, value) != 0) {
		g_free (self->priv->value);
		self->priv->value = g_strdup (value);
		g_object_notify (G_OBJECT (self), "value");
	}
}

/**
 * gdata_documents_property_get_visibility:
 * @self: a #GDataDocumentsProperty
 *
 * Returns the visibility status of the property.
 *
 * Return value: %GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC if the #GDataDocumentsProperty is publicly visible to other
 * apps, %GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE if the #GDataDocumentsProperty is restricted to the application which
 * created it.
 *
 * Since: 0.17.11
 */
const gchar *
gdata_documents_property_get_visibility (GDataDocumentsProperty *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_PROPERTY (self), NULL);
	return self->priv->visibility;
}

/**
 * gdata_documents_property_set_visibility:
 * @self: a #GDataDocumentsProperty
 * @visibility: the new visibility status of the property
 *
 * Sets #GDataDocumentsProperty:visibility to %GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC for
 * public properties and %GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE for
 * private properties (default).
 *
 * Since: 0.17.11
 */
void
gdata_documents_property_set_visibility (GDataDocumentsProperty *self, const gchar *visibility)
{
	g_return_if_fail (GDATA_IS_DOCUMENTS_PROPERTY (self));
	g_return_if_fail (g_strcmp0 (visibility, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC) == 0 ||
			  g_strcmp0 (visibility, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE) == 0);

	if (g_strcmp0 (self->priv->visibility, visibility) != 0) {
		g_free (self->priv->visibility);
		self->priv->visibility = g_strdup (visibility);
		g_object_notify (G_OBJECT (self), "visibility");
	}
}
