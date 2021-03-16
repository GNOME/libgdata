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
 * SECTION:gdata-gcontact-language
 * @short_description: gContact language element
 * @stability: Stable
 * @include: gdata/gcontact/gdata-gcontact-language.h
 *
 * #GDataGContactLanguage represents a "language" element from the
 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcLanguage">gContact specification</ulink>.
 *
 * Since: 0.7.0
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-gcontact-language.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-comparable.h"

static void gdata_gcontact_language_comparable_init (GDataComparableIface *iface);
static void gdata_gcontact_language_finalize (GObject *object);
static void gdata_gcontact_language_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_gcontact_language_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataGContactLanguagePrivate {
	gchar *code;
	gchar *label;
};

enum {
	PROP_CODE = 1,
	PROP_LABEL
};

G_DEFINE_TYPE_WITH_CODE (GDataGContactLanguage, gdata_gcontact_language, GDATA_TYPE_PARSABLE,
                         G_ADD_PRIVATE (GDataGContactLanguage)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMPARABLE, gdata_gcontact_language_comparable_init))

static void
gdata_gcontact_language_class_init (GDataGContactLanguageClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_gcontact_language_get_property;
	gobject_class->set_property = gdata_gcontact_language_set_property;
	gobject_class->finalize = gdata_gcontact_language_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->pre_get_xml = pre_get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "language";
	parsable_class->element_namespace = "gContact";

	/**
	 * GDataGContactLanguage:code:
	 *
	 * A code identifying the language, conforming to the IETF BCP 47 specification. It is mutually exclusive with #GDataGContactLanguage:label.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcLanguage">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_CODE,
	                                 g_param_spec_string ("code",
	                                                      "Code", "A code identifying the language, conforming to the IETF BCP 47 specification.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGContactLanguage:label:
	 *
	 * A free-form string that identifies the language. It is mutually exclusive with #GDataGContactLanguage:code.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcLanguage">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_LABEL,
	                                 g_param_spec_string ("label",
	                                                      "Label", "A free-form string that identifies the language.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static gint
compare_with (GDataComparable *self, GDataComparable *other)
{
	GDataGContactLanguagePrivate *a = ((GDataGContactLanguage*) self)->priv, *b = ((GDataGContactLanguage*) other)->priv;

	if (g_strcmp0 (a->code, b->code) == 0 && g_strcmp0 (a->label, b->label) == 0)
		return 0;
	return 1;
}

static void
gdata_gcontact_language_comparable_init (GDataComparableIface *iface)
{
	iface->compare_with = compare_with;
}

static void
gdata_gcontact_language_init (GDataGContactLanguage *self)
{
	self->priv = gdata_gcontact_language_get_instance_private (self);
}

static void
gdata_gcontact_language_finalize (GObject *object)
{
	GDataGContactLanguagePrivate *priv = GDATA_GCONTACT_LANGUAGE (object)->priv;

	g_free (priv->code);
	g_free (priv->label);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gcontact_language_parent_class)->finalize (object);
}

static void
gdata_gcontact_language_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataGContactLanguagePrivate *priv = GDATA_GCONTACT_LANGUAGE (object)->priv;

	switch (property_id) {
		case PROP_CODE:
			g_value_set_string (value, priv->code);
			break;
		case PROP_LABEL:
			g_value_set_string (value, priv->label);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_gcontact_language_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataGContactLanguage *self = GDATA_GCONTACT_LANGUAGE (object);

	switch (property_id) {
		case PROP_CODE:
			gdata_gcontact_language_set_code (self, g_value_get_string (value));
			break;
		case PROP_LABEL:
			gdata_gcontact_language_set_label (self, g_value_get_string (value));
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
	xmlChar *code, *label;
	GDataGContactLanguagePrivate *priv = GDATA_GCONTACT_LANGUAGE (parsable)->priv;

	code = xmlGetProp (root_node, (xmlChar*) "code");
	label = xmlGetProp (root_node, (xmlChar*) "label");
	if ((code == NULL || *code == '\0') && (label == NULL || *label == '\0')) {
		xmlFree (code);
		xmlFree (label);
		return gdata_parser_error_required_property_missing (root_node, "code", error);
	} else if (code != NULL && label != NULL) {
		/* Can't have both set at once */
		xmlFree (code);
		xmlFree (label);
		return gdata_parser_error_mutexed_properties (root_node, "code", "label", error);
	}

	priv->code = (gchar*) code;
	priv->label = (gchar*) label;

	return TRUE;
}

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGContactLanguagePrivate *priv = GDATA_GCONTACT_LANGUAGE (parsable)->priv;

	if (priv->code != NULL)
		gdata_parser_string_append_escaped (xml_string, " code='", priv->code, "'");
	else if (priv->label != NULL)
		gdata_parser_string_append_escaped (xml_string, " label='", priv->label, "'");
	else
		g_assert_not_reached ();
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "gContact", (gchar*) "http://schemas.google.com/contact/2008");
}

/**
 * gdata_gcontact_language_new:
 * @code: (allow-none): the language code, or %NULL
 * @label: (allow-none): a free-form label for the language, or %NULL
 *
 * Creates a new #GDataGContactLanguage. More information is available in the <ulink type="http"
 * url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcLanguage">gContact specification</ulink>.
 *
 * Exactly one of @code and @label should be provided; the other must be %NULL.
 *
 * Return value: a new #GDataGContactLanguage; unref with g_object_unref()
 *
 * Since: 0.7.0
 */
GDataGContactLanguage *
gdata_gcontact_language_new (const gchar *code, const gchar *label)
{
	g_return_val_if_fail ((code != NULL && *code != '\0' && label == NULL) || (code == NULL && label != NULL && *label != '\0'), NULL);
	return g_object_new (GDATA_TYPE_GCONTACT_LANGUAGE, "code", code, "label", label, NULL);
}

/**
 * gdata_gcontact_language_get_code:
 * @self: a #GDataGContactLanguage
 *
 * Gets the #GDataGContactLanguage:code property.
 *
 * Return value: the language's code, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_language_get_code (GDataGContactLanguage *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_LANGUAGE (self), NULL);
	return self->priv->code;
}

/**
 * gdata_gcontact_language_set_code:
 * @self: a #GDataGContactLanguage
 * @code: (allow-none): the new code for the language, or %NULL
 *
 * Sets the #GDataGContactLanguage:code property to @code.
 *
 * If @code is %NULL, the code will be unset. When the #GDataGContactLanguage is used in a query, however,
 * exactly one of #GDataGContactLanguage:code and #GDataGContactLanguage:label must be %NULL.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_language_set_code (GDataGContactLanguage *self, const gchar *code)
{
	g_return_if_fail (GDATA_IS_GCONTACT_LANGUAGE (self));
	g_return_if_fail (code == NULL || *code != '\0');

	g_free (self->priv->code);
	self->priv->code = g_strdup (code);
	g_object_notify (G_OBJECT (self), "code");
}

/**
 * gdata_gcontact_language_get_label:
 * @self: a #GDataGContactLanguage
 *
 * Gets the #GDataGContactLanguage:label property.
 *
 * Return value: a free-form label for the language, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_language_get_label (GDataGContactLanguage *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_LANGUAGE (self), NULL);
	return self->priv->label;
}

/**
 * gdata_gcontact_language_set_label:
 * @self: a #GDataGContactLanguage
 * @label: (allow-none): the new free-form label for the language, or %NULL
 *
 * Sets the #GDataGContactLanguage:label property to @label.
 *
 * If @label is %NULL, the label will be unset. When the #GDataGContactLanguage is used in a query, however,
 * exactly one of #GDataGContactLanguage:code and #GDataGContactLanguage:label must be %NULL.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_language_set_label (GDataGContactLanguage *self, const gchar *label)
{
	g_return_if_fail (GDATA_IS_GCONTACT_LANGUAGE (self));
	g_return_if_fail (label == NULL || *label != '\0');

	g_free (self->priv->label);
	self->priv->label = g_strdup (label);
	g_object_notify (G_OBJECT (self), "label");
}
