/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009–2010 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-gd-name
 * @short_description: GData name element
 * @stability: Unstable
 * @include: gdata/gd/gdata-gd-name.h
 *
 * #GDataGDName represents a "name" element from the
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdName">GData specification</ulink>.
 *
 * Given a name such as <literal>Sir Winston Leonard Spencer-Churchill, KG</literal>, the properties of the #GDataGDName should be
 * set as follows:
 * <variablelist>
 * 	<varlistentry><term>#GDataGDName:given-name</term><listitem><para>Winston</para></listitem></varlistentry>
 * 	<varlistentry><term>#GDataGDName:additional-name</term><listitem><para>Leonard</para></listitem></varlistentry>
 * 	<varlistentry><term>#GDataGDName:family-name</term><listitem><para>Spencer-Churchill</para></listitem></varlistentry>
 * 	<varlistentry><term>#GDataGDName:prefix</term><listitem><para>Sir</para></listitem></varlistentry>
 * 	<varlistentry><term>#GDataGDName:suffix</term><listitem><para>KG</para></listitem></varlistentry>
 * </variablelist>
 *
 * Since: 0.5.0
 **/

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-gd-name.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"

static void gdata_gd_name_finalize (GObject *object);
static void gdata_gd_name_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_gd_name_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataGDNamePrivate {
	gchar *given_name;
	gchar *additional_name;
	gchar *family_name;
	gchar *prefix;
	gchar *suffix;
	gchar *full_name;
};

enum {
	PROP_GIVEN_NAME = 1,
	PROP_ADDITIONAL_NAME,
	PROP_FAMILY_NAME,
	PROP_PREFIX,
	PROP_SUFFIX,
	PROP_FULL_NAME
};

G_DEFINE_TYPE (GDataGDName, gdata_gd_name, GDATA_TYPE_PARSABLE)
#define GDATA_GD_NAME_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDATA_TYPE_GD_NAME, GDataGDNamePrivate))

static void
gdata_gd_name_class_init (GDataGDNameClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataGDNamePrivate));

	gobject_class->get_property = gdata_gd_name_get_property;
	gobject_class->set_property = gdata_gd_name_set_property;
	gobject_class->finalize = gdata_gd_name_finalize;

	parsable_class->parse_xml = parse_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "name";
	parsable_class->element_namespace = "gd";

	/**
	 * GDataGDName:given-name:
	 *
	 * The person's given name.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdName">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_GIVEN_NAME,
				g_param_spec_string ("given-name",
					"Given name", "The person's given name.",
					NULL,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDName:additional-name:
	 *
	 * An additional name for the person (e.g. a middle name).
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdName">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_ADDITIONAL_NAME,
				g_param_spec_string ("additional-name",
					"Additional name", "An additional name for the person (e.g. a middle name).",
					NULL,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDName:family-name:
	 *
	 * The person's family name.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdName">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_FAMILY_NAME,
				g_param_spec_string ("family-name",
					"Family name", "The person's family name.",
					NULL,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDName:prefix:
	 *
	 * An honorific prefix (e.g. <literal>Mr</literal> or <literal>Mrs</literal>).
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdName">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_PREFIX,
				g_param_spec_string ("prefix",
					"Prefix", "An honorific prefix.",
					NULL,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDName:suffix:
	 *
	 * An honorific suffix (e.g. <literal>san</literal> or <literal>III</literal>).
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdName">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_SUFFIX,
				g_param_spec_string ("suffix",
					"Suffix", "An honorific suffix.",
					NULL,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDName:full-name:
	 *
	 * An unstructured representation of the person's full name. It's generally advised to use the other individual properties in preference
	 * to this one, which can fall out of synchronisation with the other properties.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdName">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_FULL_NAME,
				g_param_spec_string ("full-name",
					"Full name", "An unstructured representation of the person's full name.",
					NULL,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_gd_name_init (GDataGDName *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_GD_NAME, GDataGDNamePrivate);
}

static void
gdata_gd_name_finalize (GObject *object)
{
	GDataGDNamePrivate *priv = GDATA_GD_NAME (object)->priv;

	g_free (priv->given_name);
	g_free (priv->additional_name);
	g_free (priv->family_name);
	g_free (priv->prefix);
	g_free (priv->suffix);
	g_free (priv->full_name);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gd_name_parent_class)->finalize (object);
}

static void
gdata_gd_name_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataGDNamePrivate *priv = GDATA_GD_NAME (object)->priv;

	switch (property_id) {
		case PROP_GIVEN_NAME:
			g_value_set_string (value, priv->given_name);
			break;
		case PROP_ADDITIONAL_NAME:
			g_value_set_string (value, priv->additional_name);
			break;
		case PROP_FAMILY_NAME:
			g_value_set_string (value, priv->family_name);
			break;
		case PROP_PREFIX:
			g_value_set_string (value, priv->prefix);
			break;
		case PROP_SUFFIX:
			g_value_set_string (value, priv->suffix);
			break;
		case PROP_FULL_NAME:
			g_value_set_string (value, priv->full_name);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_gd_name_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataGDName *self = GDATA_GD_NAME (object);

	switch (property_id) {
		case PROP_GIVEN_NAME:
			gdata_gd_name_set_given_name (self, g_value_get_string (value));
			break;
		case PROP_ADDITIONAL_NAME:
			gdata_gd_name_set_additional_name (self, g_value_get_string (value));
			break;
		case PROP_FAMILY_NAME:
			gdata_gd_name_set_family_name (self, g_value_get_string (value));
			break;
		case PROP_PREFIX:
			gdata_gd_name_set_prefix (self, g_value_get_string (value));
			break;
		case PROP_SUFFIX:
			gdata_gd_name_set_suffix (self, g_value_get_string (value));
			break;
		case PROP_FULL_NAME:
			gdata_gd_name_set_full_name (self, g_value_get_string (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

#define PARSE_STRING_ELEMENT(E,F)							\
	if (xmlStrcmp (node->name, (xmlChar*) (E)) == 0) {				\
		/* gd:##E */								\
		if (priv->F != NULL)							\
			return gdata_parser_error_duplicate_element (node, error);	\
		priv->F = (gchar*) xmlNodeListGetString (doc, node->children, TRUE);	\
	}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	GDataGDNamePrivate *priv = GDATA_GD_NAME (parsable)->priv;

	if (gdata_parser_is_namespace (node, "http://schemas.google.com/g/2005") == TRUE) {
		PARSE_STRING_ELEMENT ("givenName", given_name)
		else PARSE_STRING_ELEMENT ("additionalName", additional_name)
		else PARSE_STRING_ELEMENT ("familyName", family_name)
		else PARSE_STRING_ELEMENT ("namePrefix", prefix)
		else PARSE_STRING_ELEMENT ("nameSuffix", suffix)
		else if (xmlStrcmp (node->name, (xmlChar*) "fullName") == 0) {
			/* gd:fullName */
			if (priv->full_name != NULL)
				return gdata_parser_error_duplicate_element (node, error);
			priv->full_name = (gchar*) xmlNodeListGetString (doc, node->children, TRUE);
		} else {
			return GDATA_PARSABLE_CLASS (gdata_gd_name_parent_class)->parse_xml (parsable, doc, node, user_data, error);
		}
	} else {
		return GDATA_PARSABLE_CLASS (gdata_gd_name_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}

	return TRUE;
}

#define OUTPUT_STRING_ELEMENT(E,F)									\
	if (priv->F != NULL)										\
		gdata_parser_string_append_escaped (xml_string, "<gd:" E ">", priv->F, "</gd:" E ">");

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGDNamePrivate *priv = GDATA_GD_NAME (parsable)->priv;

	OUTPUT_STRING_ELEMENT ("givenName", given_name)
	OUTPUT_STRING_ELEMENT ("additionalName", additional_name)
	OUTPUT_STRING_ELEMENT ("familyName", family_name)
	OUTPUT_STRING_ELEMENT ("namePrefix", prefix)
	OUTPUT_STRING_ELEMENT ("nameSuffix", suffix)
	OUTPUT_STRING_ELEMENT ("fullName", full_name)
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "gd", (gchar*) "http://schemas.google.com/g/2005");
}

/**
 * gdata_gd_name_new:
 * @given_name: the person's given name, or %NULL
 * @family_name: the person's family name, or %NULL
 *
 * Creates a new #GDataGDName. More information is available in the <ulink type="http"
 * url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdName">GData specification</ulink>.
 *
 * Return value: a new #GDataGDName, or %NULL; unref with g_object_unref()
 *
 * Since: 0.5.0
 **/
GDataGDName *
gdata_gd_name_new (const gchar *given_name, const gchar *family_name)
{
	return g_object_new (GDATA_TYPE_GD_NAME, "given-name", given_name, "family-name", family_name, NULL);
}

/**
 * gdata_gd_name_compare:
 * @a: a #GDataGDName, or %NULL
 * @b: another #GDataGDName, or %NULL
 *
 * Compares the two names in a strcmp() fashion. %NULL values are handled gracefully, with
 * %0 returned if both @a and @b are %NULL, %-1 if @a is %NULL and %1 if @b is %NULL.
 *
 * The comparison of non-%NULL values is done on the basis of the @given_name, @additional_name and @family_name properties of the
 * #GDataGDName<!-- -->s.
 *
 * Return value: %0 if @a equals @b, %-1 or %1 as appropriate otherwise
 *
 * Since: 0.5.0
 **/
gint
gdata_gd_name_compare (const GDataGDName *a, const GDataGDName *b)
{
	if (a == NULL && b != NULL)
		return -1;
	else if (a != NULL && b == NULL)
		return 1;

	if (a == b)
		return 0;
	if (g_strcmp0 (a->priv->given_name, b->priv->given_name) == 0 && g_strcmp0 (a->priv->additional_name, b->priv->additional_name) == 0 &&
	    g_strcmp0 (a->priv->family_name, b->priv->family_name) == 0 && g_strcmp0 (a->priv->prefix, b->priv->prefix) == 0)
		return 0;
	return 1;
}

/**
 * gdata_gd_name_get_given_name:
 * @self: a #GDataGDName
 *
 * Gets the #GDataGDName:given-name property.
 *
 * Return value: the person's given name, or %NULL
 *
 * Since: 0.5.0
 **/
const gchar *
gdata_gd_name_get_given_name (GDataGDName *self)
{
	g_return_val_if_fail (GDATA_IS_GD_NAME (self), NULL);
	return self->priv->given_name;
}

/**
 * gdata_gd_name_set_given_name:
 * @self: a #GDataGDName
 * @given_name: the new given name, or %NULL
 *
 * Sets the #GDataGDName:given-name property to @given_name.
 *
 * Since: 0.5.0
 **/
void
gdata_gd_name_set_given_name (GDataGDName *self, const gchar *given_name)
{
	g_return_if_fail (GDATA_IS_GD_NAME (self));
	g_return_if_fail (given_name == NULL || *given_name != '\0');

	g_free (self->priv->given_name);
	self->priv->given_name = g_strdup (given_name);
	g_object_notify (G_OBJECT (self), "given-name");
}

/**
 * gdata_gd_name_get_additional_name:
 * @self: a #GDataGDName
 *
 * Gets the #GDataGDName:additional-name property.
 *
 * Return value: the person's additional name, or %NULL
 *
 * Since: 0.5.0
 **/
const gchar *
gdata_gd_name_get_additional_name (GDataGDName *self)
{
	g_return_val_if_fail (GDATA_IS_GD_NAME (self), NULL);
	return self->priv->additional_name;
}

/**
 * gdata_gd_name_set_additional_name:
 * @self: a #GDataGDName
 * @additional_name: the new additional name, or %NULL
 *
 * Sets the #GDataGDName:additional-name property to @additional_name.
 *
 * Since: 0.5.0
 **/
void
gdata_gd_name_set_additional_name (GDataGDName *self, const gchar *additional_name)
{
	g_return_if_fail (GDATA_IS_GD_NAME (self));
	g_return_if_fail (additional_name == NULL || *additional_name != '\0');

	g_free (self->priv->additional_name);
	self->priv->additional_name = g_strdup (additional_name);
	g_object_notify (G_OBJECT (self), "additional-name");
}

/**
 * gdata_gd_name_get_family_name:
 * @self: a #GDataGDName
 *
 * Gets the #GDataGDName:family-name property.
 *
 * Return value: the person's family name, or %NULL
 *
 * Since: 0.5.0
 **/
const gchar *
gdata_gd_name_get_family_name (GDataGDName *self)
{
	g_return_val_if_fail (GDATA_IS_GD_NAME (self), NULL);
	return self->priv->family_name;
}

/**
 * gdata_gd_name_set_family_name:
 * @self: a #GDataGDName
 * @family_name: the new family name, or %NULL
 *
 * Sets the #GDataGDName:family-name property to @family_name.
 *
 * Since: 0.5.0
 **/
void
gdata_gd_name_set_family_name (GDataGDName *self, const gchar *family_name)
{
	g_return_if_fail (GDATA_IS_GD_NAME (self));
	g_return_if_fail (family_name == NULL || *family_name != '\0');

	g_free (self->priv->family_name);
	self->priv->family_name = g_strdup (family_name);
	g_object_notify (G_OBJECT (self), "family-name");
}

/**
 * gdata_gd_name_get_prefix:
 * @self: a #GDataGDName
 *
 * Gets the #GDataGDName:prefix property.
 *
 * Return value: the person's name prefix, or %NULL
 *
 * Since: 0.5.0
 **/
const gchar *
gdata_gd_name_get_prefix (GDataGDName *self)
{
	g_return_val_if_fail (GDATA_IS_GD_NAME (self), NULL);
	return self->priv->prefix;
}

/**
 * gdata_gd_name_set_prefix:
 * @self: a #GDataGDName
 * @prefix: the new prefix, or %NULL
 *
 * Sets the #GDataGDName:prefix property to @prefix.
 *
 * Since: 0.5.0
 **/
void
gdata_gd_name_set_prefix (GDataGDName *self, const gchar *prefix)
{
	g_return_if_fail (GDATA_IS_GD_NAME (self));
	g_return_if_fail (prefix == NULL || *prefix != '\0');

	g_free (self->priv->prefix);
	self->priv->prefix = g_strdup (prefix);
	g_object_notify (G_OBJECT (self), "prefix");
}

/**
 * gdata_gd_name_get_suffix:
 * @self: a #GDataGDName
 *
 * Gets the #GDataGDName:suffix property.
 *
 * Return value: the person's name suffix, or %NULL
 *
 * Since: 0.5.0
 **/
const gchar *
gdata_gd_name_get_suffix (GDataGDName *self)
{
	g_return_val_if_fail (GDATA_IS_GD_NAME (self), NULL);
	return self->priv->suffix;
}

/**
 * gdata_gd_name_set_suffix:
 * @self: a #GDataGDName
 * @suffix: the new suffix, or %NULL
 *
 * Sets the #GDataGDName:suffix property to @suffix.
 *
 * Since: 0.5.0
 **/
void
gdata_gd_name_set_suffix (GDataGDName *self, const gchar *suffix)
{
	g_return_if_fail (GDATA_IS_GD_NAME (self));
	g_return_if_fail (suffix == NULL || *suffix != '\0');

	g_free (self->priv->suffix);
	self->priv->suffix = g_strdup (suffix);
	g_object_notify (G_OBJECT (self), "suffix");
}

/**
 * gdata_gd_name_get_full_name:
 * @self: a #GDataGDName
 *
 * Gets the #GDataGDName:full-name property.
 *
 * Return value: the person's full name, or %NULL
 *
 * Since: 0.5.0
 **/
const gchar *
gdata_gd_name_get_full_name (GDataGDName *self)
{
	g_return_val_if_fail (GDATA_IS_GD_NAME (self), NULL);
	return self->priv->full_name;
}

/**
 * gdata_gd_name_set_full_name:
 * @self: a #GDataGDName
 * @full_name: the new full name, or %NULL
 *
 * Sets the #GDataGDName:full-name property to @full_name.
 *
 * Since: 0.5.0
 **/
void
gdata_gd_name_set_full_name (GDataGDName *self, const gchar *full_name)
{
	g_return_if_fail (GDATA_IS_GD_NAME (self));

	g_free (self->priv->full_name);
	self->priv->full_name = g_strdup (full_name);
	g_object_notify (G_OBJECT (self), "full-name");
}
