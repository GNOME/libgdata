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
 * SECTION:gdata-property
 * @short_description: GData property object class
 * @stability: Unstable
 * @include: gdata/gdata-property.h
 *
 * #GDataProperty is a subclass of #GDataParsable and represents a Google Drive Property Resource on a file object.
 *
 * It allows applications to store additional metadata on a file, such as tags, IDs from other data stores, viewing preferences etc. Properties can be used to share metadata between applications, for example, in a workflow application.
 *
 * Each #GDataProperty is characterized by a key-value pair (where value is optional, and takes empty string "" by default) and a visibility parameter. The visibility can take values %TRUE for "PUBLIC" properties and %FALSE for "PRIVATE" properties (default). This allows for a property to be visible to all apps, or restricted to the app that creates the property.
 *
 * Since: 0.18.0
 */

#include <glib.h>
#include <json-glib/json-glib.h>
#include "gdata-property.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-private.h"

#include "gdata-comparable.h"

static void gdata_property_comparable_init (GDataComparableIface *iface);
static void gdata_property_finalize (GObject *object);
static void gdata_property_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_property_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void _gdata_property_set_key (GDataProperty *self, const gchar *key);
static void _gdata_property_set_etag (GDataProperty *self, const gchar *etag);

static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);
static gboolean post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error);
static void get_json (GDataParsable *parsable, JsonBuilder *builder);

struct _GDataPropertyPrivate {
	gchar *key;
	gchar *etag;
	gchar *value;		/* default - %NULL */
	gboolean visibility;	/* default - %FALSE */
};

enum {
	PROP_KEY = 1,
	PROP_ETAG,
	PROP_VALUE,
	PROP_VISIBILITY
};

G_DEFINE_TYPE_WITH_CODE (GDataProperty, gdata_property, GDATA_TYPE_PARSABLE,
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMPARABLE, gdata_property_comparable_init))

static void
gdata_property_class_init (GDataPropertyClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataPropertyPrivate));

	gobject_class->set_property = gdata_property_set_property;
	gobject_class->get_property = gdata_property_get_property;
	gobject_class->finalize = gdata_property_finalize;

	parsable_class->parse_json = parse_json;
	parsable_class->post_parse_json = post_parse_json;
	parsable_class->get_json = get_json;
	parsable_class->element_name = "property";


	/**
	 * GDataProperty:key:
	 *
	 * The key of this property.
	 *
	 * For more information, see the <ulink type="http" url="https://developers.google.com/drive/api/v2/reference/properties">Properties Resource</ulink>
	 *
	 * Since: 0.18.0
	 */
	g_object_class_install_property (gobject_class, PROP_KEY,
					 g_param_spec_string ("key",
							      "Key", "The key of this property.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataProperty:etag:
	 *
	 * ETag of the property. TODO: What else should I write here?
	 *
	 * For more information, see the <ulink type="http" url="https://developers.google.com/drive/api/v2/reference/properties">Properties Resource</ulink>
	 *
	 * Since: 0.18.0
	 */
	g_object_class_install_property (gobject_class, PROP_ETAG,
					 g_param_spec_string ("etag",
							      "ETag", "ETag of the property.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataProperty:value:
	 *
	 * The value of this property. By default, it assumes a %NULL value in
	 * #GDataProperty which corresponds to an empty string for the Drive
	 * Properties Resource.
	 *
	 * For more information, see the <ulink type="http" url="https://developers.google.com/drive/api/v2/reference/properties">Properties Resource</ulink>
	 *
	 * Since: 0.18.0
	 */
	g_object_class_install_property (gobject_class, PROP_VALUE,
					 g_param_spec_string ("value",
							      "Value", "The value of this property.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataProperty:visibility:
	 *
	 * The visibility status of this property. The default value of
	 * visibility is PRIVATE on a Drive Properties Resource object,
	 * hence #GDataProperty:visibility is %FALSE by default. A private property
	 * restricts its visibility to only the app which created it. A
	 * %TRUE value corresponds to a PUBLIC property which is visible to all the apps.
	 *
	 * For more information, see the <ulink type="http" url="https://developers.google.com/drive/api/v2/reference/properties">Properties Resource</ulink>
	 *
	 * Since: 0.18.0
	 */
	g_object_class_install_property (gobject_class, PROP_VISIBILITY,
					 g_param_spec_boolean ("visibility",
							       "Visibility", "The visibility of this property.",
							       FALSE,
							       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static gint
compare_with (GDataComparable *self, GDataComparable *other)
{
	// TODO: What other things may we need to compare?
	return g_strcmp0 (((GDataProperty*) self)->priv->key, ((GDataProperty*) other)->priv->key);
}

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	gchar *output_val = NULL;
	gboolean success = TRUE;

	if (gdata_parser_string_from_json_member (reader, "key", P_DEFAULT, &output_val, &success, error) == TRUE) {
		if (success && output_val != NULL && output_val[0] != '\0') {
			_gdata_property_set_key (GDATA_PROPERTY (parsable), output_val);
		}

		g_free (output_val);
		return success;
	} else if (gdata_parser_string_from_json_member (reader, "etag", P_DEFAULT, &output_val, &success, error) == TRUE) {
		if (success && output_val != NULL && output_val[0] != '\0') {
			_gdata_property_set_etag (GDATA_PROPERTY (parsable), output_val);
		}

		g_free (output_val);
		return success;
	} else if (gdata_parser_string_from_json_member (reader, "value", P_DEFAULT, &output_val, &success, error) == TRUE) {

		/* A Property can have a value field to be an empty string */
		// TODO: Test if this is always present in the response body
		if (success && output_val != NULL) {
			gdata_property_set_value (GDATA_PROPERTY (parsable), output_val);
		}

		g_free (output_val);
		return success;
	} else if (gdata_parser_string_from_json_member (reader, "visibility", P_REQUIRED | P_NON_EMPTY, &output_val, &success, error) == TRUE) {
		gdata_property_set_visibility (GDATA_PROPERTY (parsable),
							    (g_strcmp0 (output_val, GDATA_PROPERTY_VISIBILITY_PUBLIC) == 0));
		g_free (output_val);
		return success;
	}

	return GDATA_PARSABLE_CLASS (gdata_property_parent_class)->parse_json (parsable, reader, user_data, error);
}

static gboolean
post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error)
{
	/*
	 * FIXME: post_parse_json ()
	 * a function called after parsing a JSON object, to allow the parsable to validate the parsed properties
	 **/
	return TRUE;
}

static void
get_json (GDataParsable *parsable, JsonBuilder *builder)
{

	GDataPropertyPrivate *priv = GDATA_PROPERTY (parsable)->priv;


	/* Add all the App Property specific JSON members */
	g_assert (priv->key != NULL);
	json_builder_set_member_name (builder, "key");
	json_builder_add_string_value (builder, priv->key);

	if (gdata_property_get_etag (GDATA_PROPERTY (parsable)) != NULL) {
		json_builder_set_member_name (builder, "etag");
		json_builder_add_string_value (builder, priv->etag);
	}

	/* Property object's value may be an empty string */
	json_builder_set_member_name (builder, "value");
	if (priv->value == NULL) {
		json_builder_add_string_value (builder, "");
	} else {
		json_builder_add_string_value (builder, priv->value);
	}

	json_builder_set_member_name (builder, "visibility");
	json_builder_add_string_value (builder,
				       (priv->visibility) ? GDATA_PROPERTY_VISIBILITY_PUBLIC : GDATA_PROPERTY_VISIBILITY_PRIVATE);

	/* Chain up to the parent class */
	/* GDATA_PARSABLE_CLASS (gdata_property_parent_class)->get_json (parsable, builder); */
}

static void
gdata_property_comparable_init (GDataComparableIface *iface)
{
	iface->compare_with = compare_with;
}

static void
gdata_property_init (GDataProperty *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_PROPERTY, GDataPropertyPrivate);
}

static void
gdata_property_finalize (GObject *object)
{
	GDataPropertyPrivate *priv = GDATA_PROPERTY (object)->priv;

	g_free (priv->key);
	g_free (priv->etag);
	g_free (priv->value);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_property_parent_class)->finalize (object);
}

static void
gdata_property_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataPropertyPrivate *priv = GDATA_PROPERTY (object)->priv;

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
			g_value_set_boolean (value, priv->visibility);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_property_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataProperty *self = GDATA_PROPERTY (object);

	switch (property_id) {
		case PROP_KEY:
			_gdata_property_set_key (self, g_value_get_string (value));
			break;
		case PROP_ETAG:
			_gdata_property_set_etag (self, g_value_get_string (value));
			break;
		case PROP_VALUE:
			gdata_property_set_value (self, g_value_get_string (value));
			break;
		case PROP_VISIBILITY:
			gdata_property_set_visibility (self, g_value_get_boolean (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

/**
 * gdata_property_new:
 * @key: the property's key
 *
 * Creates a new #GDataEntry with the given ID and default properties.
 *
 * Return value: (transfer full): a new #GDataProperty; unref with g_object_unref()
 */
GDataProperty *gdata_property_new (const gchar *key)
{
	/* App Property must have a non NULL key at initilization time,
	 * rest of the properties can be NULL or take DEFAULT values */
	g_return_val_if_fail (key != NULL && *key != '\0', NULL);

	/* Google Drive allows value to be an empty string,
	 * and visibility is PRIVATE by default */
	return g_object_new (GDATA_TYPE_PROPERTY,
			     "key", key,
			     "value", NULL,
			     "visibility", FALSE,
			     NULL);
}

/**
 * gdata_property_get_key:
 * @self: a #GDataProperty
 *
 * Returns the key of the property. This will never be %NULL or an empty string ("").
 *
 * Return value: (transfer none): the property's key
 */
const gchar *
gdata_property_get_key (GDataProperty *self)
{
	g_return_val_if_fail (GDATA_IS_PROPERTY (self), NULL);
	return self->priv->key;
}

static void
_gdata_property_set_key (GDataProperty *self, const gchar *key)
{
	/* This is a READ-ONLY PROPERTY */
	g_return_if_fail (GDATA_IS_PROPERTY (self));
	g_return_if_fail (key != NULL && *key != '\0');

	g_free (self->priv->key);
	self->priv->key = g_strdup (key);

	g_object_notify (G_OBJECT (self), "key");
}

/**
 * gdata_property_get_etag:
 * @self: a #GDataProperty
 *
 * Returns the ETag of the property.
 *
 * Return value: (transfer none): the property's ETag. The ETag will never be empty; it's either %NULL or a valid ETag.
 */
const gchar *
gdata_property_get_etag (GDataProperty *self)
{
	g_return_val_if_fail (GDATA_IS_PROPERTY (self), NULL);
	return self->priv->etag;
}

static void
_gdata_property_set_etag (GDataProperty *self, const gchar *etag)
{
	g_return_if_fail (GDATA_IS_PROPERTY (self));

	g_free (self->priv->etag);
	self->priv->etag = g_strdup (etag);
	g_object_notify (G_OBJECT (self), "etag");
}

/**
 * gdata_property_get_value:
 * @self: a #GDataProperty
 *
 * Returns the value of the property.
 *
 * Return value: (nullable): the property's value. This can be %NULL or empty,
 * both of which correspond to an empty string on the Drive Property Resource.
 */
const gchar *
gdata_property_get_value (GDataProperty *self)
{
	g_return_val_if_fail (GDATA_IS_PROPERTY (self), NULL);
	return self->priv->value;
}

/**
 * gdata_property_set_value:
 * @self: a #GDataProperty
 * @value: (allow-none): the new value of the property
 *
 * Sets #GDataProperty:value to @value, corresponding to the key.
 */
void
gdata_property_set_value (GDataProperty *self, const gchar *value)
{
	g_return_if_fail (GDATA_IS_PROPERTY (self));

	g_free (self->priv->value);
	self->priv->value = g_strdup (value);
	g_object_notify (G_OBJECT (self), "value");
}

/**
 * gdata_property_get_visibility:
 * @self: a #GDataProperty
 *
 * Returns the visibility status of the property.
 *
 * Return value: %TRUE if the #GDataProperty is publicly visible to other
 * apps, %FALSE if the #GDataProperty is restricted to the application which
 * created it.
 */
gboolean
gdata_property_get_visibility (GDataProperty *self)
{
	g_return_val_if_fail (GDATA_IS_PROPERTY (self), FALSE);
	return self->priv->visibility;
}

/**
 * gdata_property_set_visibility:
 * @self: a #GDataProperty
 * @visibility: the new visibility status of the property
 *
 * Sets #GDataEntry:visibility to %TRUE for PUBLIC and %FALSE for PRIVATE
 * (default).
 */
void
gdata_property_set_visibility (GDataProperty *self, const gboolean visibility)
{
	g_return_if_fail (GDATA_IS_PROPERTY (self));
	self->priv->visibility = visibility;
	g_object_notify (G_OBJECT (self), "visibility");
}
