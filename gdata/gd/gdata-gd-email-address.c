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
 * SECTION:gdata-gd-email-address
 * @short_description: GData e-mail address element
 * @stability: Stable
 * @include: gdata/gd/gdata-gd-email-address.h
 *
 * #GDataGDEmailAddress represents an "email" element from the
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdEmail">GData specification</ulink>.
 *
 * Since: 0.4.0
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-gd-email-address.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-comparable.h"

static void gdata_gd_email_address_comparable_init (GDataComparableIface *iface);
static void gdata_gd_email_address_finalize (GObject *object);
static void gdata_gd_email_address_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_gd_email_address_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataGDEmailAddressPrivate {
	gchar *address;
	gchar *relation_type;
	gchar *label;
	gboolean is_primary;
	gchar *display_name;
};

enum {
	PROP_ADDRESS = 1,
	PROP_RELATION_TYPE,
	PROP_LABEL,
	PROP_IS_PRIMARY,
	PROP_DISPLAY_NAME
};

G_DEFINE_TYPE_WITH_CODE (GDataGDEmailAddress, gdata_gd_email_address, GDATA_TYPE_PARSABLE,
                         G_ADD_PRIVATE (GDataGDEmailAddress)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMPARABLE, gdata_gd_email_address_comparable_init))

static void
gdata_gd_email_address_class_init (GDataGDEmailAddressClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_gd_email_address_get_property;
	gobject_class->set_property = gdata_gd_email_address_set_property;
	gobject_class->finalize = gdata_gd_email_address_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->pre_get_xml = pre_get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "email";
	parsable_class->element_namespace = "gd";

	/**
	 * GDataGDEmailAddress:address:
	 *
	 * The e-mail address itself.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdEmail">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_ADDRESS,
	                                 g_param_spec_string ("address",
	                                                      "Address", "The e-mail address itself.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDEmailAddress:relation-type:
	 *
	 * A programmatic value that identifies the type of e-mail address. For example: %GDATA_GD_EMAIL_ADDRESS_HOME or %GDATA_GD_EMAIL_ADDRESS_WORK.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdEmail">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_RELATION_TYPE,
	                                 g_param_spec_string ("relation-type",
	                                                      "Relation type", "A programmatic value that identifies the type of e-mail address.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDEmailAddress:label:
	 *
	 * A simple string value used to name this e-mail address. It allows UIs to display a label such as "Work", "Personal", "Preferred", etc.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdEmail">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_LABEL,
	                                 g_param_spec_string ("label",
	                                                      "Label", "A simple string value used to name this e-mail address.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDEmailAddress:is-primary:
	 *
	 * Indicates which e-mail address out of a group is primary.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdEmail">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_IS_PRIMARY,
	                                 g_param_spec_boolean ("is-primary",
	                                                       "Primary?", "Indicates which e-mail address out of a group is primary.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDEmailAddress:display-name:
	 *
	 * A display name of the entity (e.g. a person) the e-mail address belongs to.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdEmail">GData specification</ulink>.
	 *
	 * Since: 0.6.0
	 */
	g_object_class_install_property (gobject_class, PROP_DISPLAY_NAME,
	                                 g_param_spec_string ("display-name",
	                                                      "Display name", "A display name of the entity the e-mail address belongs to.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static gint
compare_with (GDataComparable *self, GDataComparable *other)
{
	return g_strcmp0 (((GDataGDEmailAddress*) self)->priv->address, ((GDataGDEmailAddress*) other)->priv->address);
}

static void
gdata_gd_email_address_comparable_init (GDataComparableIface *iface)
{
	iface->compare_with = compare_with;
}

static void
gdata_gd_email_address_init (GDataGDEmailAddress *self)
{
	self->priv = gdata_gd_email_address_get_instance_private (self);
}

static void
gdata_gd_email_address_finalize (GObject *object)
{
	GDataGDEmailAddressPrivate *priv = GDATA_GD_EMAIL_ADDRESS (object)->priv;

	g_free (priv->address);
	g_free (priv->relation_type);
	g_free (priv->label);
	g_free (priv->display_name);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gd_email_address_parent_class)->finalize (object);
}

static void
gdata_gd_email_address_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataGDEmailAddressPrivate *priv = GDATA_GD_EMAIL_ADDRESS (object)->priv;

	switch (property_id) {
		case PROP_ADDRESS:
			g_value_set_string (value, priv->address);
			break;
		case PROP_RELATION_TYPE:
			g_value_set_string (value, priv->relation_type);
			break;
		case PROP_LABEL:
			g_value_set_string (value, priv->label);
			break;
		case PROP_IS_PRIMARY:
			g_value_set_boolean (value, priv->is_primary);
			break;
		case PROP_DISPLAY_NAME:
			g_value_set_string (value, priv->display_name);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_gd_email_address_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataGDEmailAddress *self = GDATA_GD_EMAIL_ADDRESS (object);

	switch (property_id) {
		case PROP_ADDRESS:
			gdata_gd_email_address_set_address (self, g_value_get_string (value));
			break;
		case PROP_RELATION_TYPE:
			gdata_gd_email_address_set_relation_type (self, g_value_get_string (value));
			break;
		case PROP_LABEL:
			gdata_gd_email_address_set_label (self, g_value_get_string (value));
			break;
		case PROP_IS_PRIMARY:
			gdata_gd_email_address_set_is_primary (self, g_value_get_boolean (value));
			break;
		case PROP_DISPLAY_NAME:
			gdata_gd_email_address_set_display_name (self, g_value_get_string (value));
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
	xmlChar *address, *rel;
	gboolean primary_bool;
	GDataGDEmailAddressPrivate *priv = GDATA_GD_EMAIL_ADDRESS (parsable)->priv;

	/* Is it the primary e-mail address? */
	if (gdata_parser_boolean_from_property (root_node, "primary", &primary_bool, 0, error) == FALSE)
		return FALSE;

	address = xmlGetProp (root_node, (xmlChar*) "address");
	if (address == NULL || *address == '\0') {
		xmlFree (address);
		return gdata_parser_error_required_property_missing (root_node, "address", error);
	}

	rel = xmlGetProp (root_node, (xmlChar*) "rel");
	if (rel != NULL && *rel == '\0') {
		xmlFree (address);
		xmlFree (rel);
		return gdata_parser_error_required_property_missing (root_node, "rel", error);
	}

	priv->address = (gchar*) address;
	priv->relation_type = (gchar*) rel;
	priv->label = (gchar*) xmlGetProp (root_node, (xmlChar*) "label");
	priv->is_primary = primary_bool;
	priv->display_name = (gchar*) xmlGetProp (root_node, (xmlChar*) "displayName");

	return TRUE;
}

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGDEmailAddressPrivate *priv = GDATA_GD_EMAIL_ADDRESS (parsable)->priv;

	gdata_parser_string_append_escaped (xml_string, " address='", priv->address, "'");
	if (priv->relation_type != NULL)
		gdata_parser_string_append_escaped (xml_string, " rel='", priv->relation_type, "'");
	if (priv->label != NULL)
		gdata_parser_string_append_escaped (xml_string, " label='", priv->label, "'");
	if (priv->display_name != NULL)
		gdata_parser_string_append_escaped (xml_string, " displayName='", priv->display_name, "'");

	if (priv->is_primary == TRUE)
		g_string_append (xml_string, " primary='true'");
	else
		g_string_append (xml_string, " primary='false'");
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "gd", (gchar*) "http://schemas.google.com/g/2005");
}

/**
 * gdata_gd_email_address_new:
 * @address: the e-mail address
 * @relation_type: (allow-none): the relationship between the e-mail address and its owner, or %NULL
 * @label: (allow-none): a human-readable label for the e-mail address, or %NULL
 * @is_primary: %TRUE if this e-mail address is its owner's primary address, %FALSE otherwise
 *
 * Creates a new #GDataGDEmailAddress. More information is available in the <ulink type="http"
 * url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdEmail">GData specification</ulink>.
 *
 * Return value: a new #GDataGDEmailAddress, or %NULL; unref with g_object_unref()
 *
 * Since: 0.2.0
 */
GDataGDEmailAddress *
gdata_gd_email_address_new (const gchar *address, const gchar *relation_type, const gchar *label, gboolean is_primary)
{
	g_return_val_if_fail (address != NULL && *address != '\0', NULL);
	g_return_val_if_fail (relation_type == NULL || *relation_type != '\0', NULL);
	return g_object_new (GDATA_TYPE_GD_EMAIL_ADDRESS, "address", address, "relation-type", relation_type,
	                     "label", label, "is-primary", is_primary, NULL);
}

/**
 * gdata_gd_email_address_get_address:
 * @self: a #GDataGDEmailAddress
 *
 * Gets the #GDataGDEmailAddress:address property.
 *
 * Return value: the e-mail address itself, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_gd_email_address_get_address (GDataGDEmailAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_EMAIL_ADDRESS (self), NULL);
	return self->priv->address;
}

/**
 * gdata_gd_email_address_set_address:
 * @self: a #GDataGDEmailAddress
 * @address: the new e-mail address
 *
 * Sets the #GDataGDEmailAddress:address property to @address.
 *
 * Since: 0.4.0
 */
void
gdata_gd_email_address_set_address (GDataGDEmailAddress *self, const gchar *address)
{
	g_return_if_fail (GDATA_IS_GD_EMAIL_ADDRESS (self));
	g_return_if_fail (address != NULL && *address != '\0');

	g_free (self->priv->address);
	self->priv->address = g_strdup (address);
	g_object_notify (G_OBJECT (self), "address");
}

/**
 * gdata_gd_email_address_get_relation_type:
 * @self: a #GDataGDEmailAddress
 *
 * Gets the #GDataGDEmailAddress:relation-type property.
 *
 * Return value: the e-mail address' relation type, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_gd_email_address_get_relation_type (GDataGDEmailAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_EMAIL_ADDRESS (self), NULL);
	return self->priv->relation_type;
}

/**
 * gdata_gd_email_address_set_relation_type:
 * @self: a #GDataGDEmailAddress
 * @relation_type: (allow-none): the new relation type for the email_address, or %NULL
 *
 * Sets the #GDataGDEmailAddress:relation-type property to @relation_type.
 *
 * Set @relation_type to %NULL to unset the property in the e-mail address.
 *
 * Since: 0.4.0
 */
void
gdata_gd_email_address_set_relation_type (GDataGDEmailAddress *self, const gchar *relation_type)
{
	g_return_if_fail (GDATA_IS_GD_EMAIL_ADDRESS (self));
	g_return_if_fail (relation_type == NULL || *relation_type != '\0');

	g_free (self->priv->relation_type);
	self->priv->relation_type = g_strdup (relation_type);
	g_object_notify (G_OBJECT (self), "relation-type");
}

/**
 * gdata_gd_email_address_get_label:
 * @self: a #GDataGDEmailAddress
 *
 * Gets the #GDataGDEmailAddress:label property.
 *
 * Return value: the e-mail address' label, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_gd_email_address_get_label (GDataGDEmailAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_EMAIL_ADDRESS (self), NULL);
	return self->priv->label;
}

/**
 * gdata_gd_email_address_set_label:
 * @self: a #GDataGDEmailAddress
 * @label: (allow-none): the new label for the e-mail address, or %NULL
 *
 * Sets the #GDataGDEmailAddress:label property to @label.
 *
 * Set @label to %NULL to unset the property in the e-mail address.
 *
 * Since: 0.4.0
 */
void
gdata_gd_email_address_set_label (GDataGDEmailAddress *self, const gchar *label)
{
	g_return_if_fail (GDATA_IS_GD_EMAIL_ADDRESS (self));

	g_free (self->priv->label);
	self->priv->label = g_strdup (label);
	g_object_notify (G_OBJECT (self), "label");
}

/**
 * gdata_gd_email_address_is_primary:
 * @self: a #GDataGDEmailAddress
 *
 * Gets the #GDataGDEmailAddress:is-primary property.
 *
 * Return value: %TRUE if this is the primary e-mail address, %FALSE otherwise
 *
 * Since: 0.4.0
 */
gboolean
gdata_gd_email_address_is_primary (GDataGDEmailAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_EMAIL_ADDRESS (self), FALSE);
	return self->priv->is_primary;
}

/**
 * gdata_gd_email_address_set_is_primary:
 * @self: a #GDataGDEmailAddress
 * @is_primary: %TRUE if this is the primary e-mail address, %FALSE otherwise
 *
 * Sets the #GDataGDEmailAddress:is-primary property to @is_primary.
 *
 * Since: 0.4.0
 */
void
gdata_gd_email_address_set_is_primary (GDataGDEmailAddress *self, gboolean is_primary)
{
	g_return_if_fail (GDATA_IS_GD_EMAIL_ADDRESS (self));

	self->priv->is_primary = is_primary;
	g_object_notify (G_OBJECT (self), "is-primary");
}

/**
 * gdata_gd_email_address_get_display_name:
 * @self: a #GDataGDEmailAddress
 *
 * Gets the #GDataGDEmailAddress:display-name property.
 *
 * Return value: a display name for the e-mail address, or %NULL
 *
 * Since: 0.6.0
 */
const gchar *
gdata_gd_email_address_get_display_name (GDataGDEmailAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_EMAIL_ADDRESS (self), NULL);
	return self->priv->display_name;
}

/**
 * gdata_gd_email_address_set_display_name:
 * @self: a #GDataGDEmailAddress
 * @display_name: (allow-none): the new display name, or %NULL
 *
 * Sets the #GDataGDEmailAddress:display-name property to @display_name.
 *
 * Set @display_name to %NULL to unset the property in the e-mail address.
 *
 * Since: 0.6.0
 */
void
gdata_gd_email_address_set_display_name (GDataGDEmailAddress *self, const gchar *display_name)
{
	g_return_if_fail (GDATA_IS_GD_EMAIL_ADDRESS (self));

	g_free (self->priv->display_name);
	self->priv->display_name = g_strdup (display_name);
	g_object_notify (G_OBJECT (self), "display-name");
}
