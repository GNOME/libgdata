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
static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);
static gboolean post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error);
static void get_json (GDataParsable *parsable, JsonBuilder *builder);

struct _GDataPropertyPrivate {
	gchar *key;
	gchar *etag;
	gchar *value;
	gboolean is_publicly_visible;
};

enum {
	PROP_KEY = 1,
	PROP_ETAG,
	PROP_VALUE,
	PROP_IS_PUBLICLY_VISIBLE
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

	g_object_class_install_property (gobject_class, PROP_KEY,
					 g_param_spec_string ("key",
							      "Key", "The Key for the App-Property.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, PROP_ETAG,
					 g_param_spec_string ("etag",
							      "ETag", "",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (gobject_class, PROP_VALUE,
					 g_param_spec_string ("value",
							      "Value", "The value corresponding to the key in property.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/* The default value of visibility is PRIVATE on a properties object,
	 * hence is_publicly_visible is FALSE by default */
	g_object_class_install_property (gobject_class, PROP_IS_PUBLICLY_VISIBLE,
					 g_param_spec_boolean ("is-publicly-visible",
							       "Public?", "Indicates whether property visible publicly, or private to the app?",
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
			gdata_property_set_etag (GDATA_PROPERTY (parsable), output_val);
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
		gdata_property_set_is_publicly_visible (GDATA_PROPERTY (parsable),
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

	/* TODO: What is meant by the below statement? Remove the below statements
	 * Chain up to the parent class
	 * GDATA_PARSABLE_CLASS (gdata_property_parent_class)->get_json (parsable, builder); */

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
				       (priv->is_publicly_visible) ? GDATA_PROPERTY_VISIBILITY_PUBLIC : GDATA_PROPERTY_VISIBILITY_PRIVATE);
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
		case PROP_IS_PUBLICLY_VISIBLE:
			g_value_set_boolean (value, priv->is_publicly_visible);
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
			gdata_property_set_etag (self, g_value_get_string (value));
			break;
		case PROP_VALUE:
			gdata_property_set_value (self, g_value_get_string (value));
			break;
		case PROP_IS_PUBLICLY_VISIBLE:
			gdata_property_set_is_publicly_visible (self, g_value_get_boolean (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

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
			     "is_publicly_visible", FALSE,
			     NULL);
}

const gchar *
gdata_property_get_key (GDataProperty *self)
{
	g_return_val_if_fail (GDATA_IS_PROPERTY (self), NULL);
	return self->priv->key;
}

void
_gdata_property_set_key (GDataProperty *self, const gchar *key)
{
	/* This is a READ-ONLY PROPERTY */
	g_return_if_fail (GDATA_IS_PROPERTY (self));
	g_return_if_fail (key != NULL && *key != '\0');

	g_free (self->priv->key);
	self->priv->key = g_strdup (key);

	g_object_notify (G_OBJECT (self), "key");
}

const gchar *
gdata_property_get_etag (GDataProperty *self)
{
	g_return_val_if_fail (GDATA_IS_PROPERTY (self), NULL);
	return self->priv->etag;
}

// TODO: Ask how to convert this to a private function
void
gdata_property_set_etag (GDataProperty *self, const gchar *etag)
{
	// TODO: Look at how this is done in gdata_entry_get_etag.
	// TODO: Ask if this property needs to be read-only and private
	g_return_if_fail (GDATA_IS_PROPERTY (self));
	/* g_return_if_fail (etag != NULL && *etag != '\0'); */

	g_free (self->priv->etag);
	self->priv->etag = g_strdup (etag);
	g_object_notify (G_OBJECT (self), "etag");
}

const gchar *
gdata_property_get_value (GDataProperty *self)
{
	g_return_val_if_fail (GDATA_IS_PROPERTY (self), NULL);
	return self->priv->value;
}

void
gdata_property_set_value (GDataProperty *self, const gchar *value)
{
	g_return_if_fail (GDATA_IS_PROPERTY (self));

	g_free (self->priv->value);
	self->priv->value = g_strdup (value);
	g_object_notify (G_OBJECT (self), "value");
}

gboolean
gdata_property_get_is_publicly_visible (GDataProperty *self)
{
	g_return_val_if_fail (GDATA_IS_PROPERTY (self), FALSE);
	return self->priv->is_publicly_visible;
}

void
gdata_property_set_is_publicly_visible (GDataProperty *self, const gboolean visibility)
{
	g_return_if_fail (GDATA_IS_PROPERTY (self));
	self->priv->is_publicly_visible = visibility;
	g_object_notify (G_OBJECT (self), "is-publicly-visible");
}
