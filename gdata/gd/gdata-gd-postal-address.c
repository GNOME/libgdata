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
 * SECTION:gdata-gd-postal-address
 * @short_description: GData postal address element
 * @stability: Stable
 * @include: gdata/gd/gdata-gd-postal-address.h
 *
 * #GDataGDPostalAddress represents a "structuredPostalAddress" element from the
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
 * Note that it does not represent a simple "postalAddress" element, as "structuredPostalAddress" is now used wherever possible in the GData API.
 *
 * Since: 0.4.0
 */

#include <glib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-gd-postal-address.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-comparable.h"

static void gdata_gd_postal_address_comparable_init (GDataComparableIface *iface);
static void gdata_gd_postal_address_finalize (GObject *object);
static void gdata_gd_postal_address_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_gd_postal_address_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataGDPostalAddressPrivate {
	gchar *formatted_address;
	gchar *relation_type;
	gchar *label;
	gboolean is_primary;
	gchar *mail_class;
	gchar *usage;
	gchar *agent;
	gchar *house_name;
	gchar *street;
	gchar *po_box;
	gchar *neighborhood;
	gchar *city;
	gchar *subregion;
	gchar *region;
	gchar *postcode;
	gchar *country;
	gchar *country_code;
};

enum {
	PROP_FORMATTED_ADDRESS = 1,
	PROP_RELATION_TYPE,
	PROP_LABEL,
	PROP_IS_PRIMARY,
	PROP_MAIL_CLASS,
	PROP_USAGE,
	PROP_AGENT,
	PROP_HOUSE_NAME,
	PROP_STREET,
	PROP_PO_BOX,
	PROP_NEIGHBORHOOD,
	PROP_CITY,
	PROP_SUBREGION,
	PROP_REGION,
	PROP_POSTCODE,
	PROP_COUNTRY,
	PROP_COUNTRY_CODE
};

G_DEFINE_TYPE_WITH_CODE (GDataGDPostalAddress, gdata_gd_postal_address, GDATA_TYPE_PARSABLE,
                         G_ADD_PRIVATE (GDataGDPostalAddress)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMPARABLE, gdata_gd_postal_address_comparable_init))

static void
gdata_gd_postal_address_class_init (GDataGDPostalAddressClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_gd_postal_address_get_property;
	gobject_class->set_property = gdata_gd_postal_address_set_property;
	gobject_class->finalize = gdata_gd_postal_address_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->parse_xml = parse_xml;
	parsable_class->pre_get_xml = pre_get_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "structuredPostalAddress";
	parsable_class->element_namespace = "gd";

	/**
	 * GDataGDPostalAddress:address:
	 *
	 * The postal address itself, formatted and unstructured. It is preferred to use the other, structured properties rather than this one.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_FORMATTED_ADDRESS,
	                                 g_param_spec_string ("address",
	                                                      "Address", "The postal address itself.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDPostalAddress:relation-type:
	 *
	 * A programmatic value that identifies the type of postal address. For example: %GDATA_GD_POSTAL_ADDRESS_WORK or
	 * %GDATA_GD_POSTAL_ADDRESS_OTHER.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_RELATION_TYPE,
	                                 g_param_spec_string ("relation-type",
	                                                      "Relation type", "A programmatic value that identifies the type of postal address.",
	                                                      GDATA_GD_POSTAL_ADDRESS_WORK,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDPostalAddress:label:
	 *
	 * A simple string value used to name this postal address. It allows UIs to display a label such as "Work", "Personal", "Preferred", etc.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_LABEL,
	                                 g_param_spec_string ("label",
	                                                      "Label", "A simple string value used to name this postal address.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDPostalAddress:is-primary:
	 *
	 * Indicates which postal address out of a group is primary.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_IS_PRIMARY,
	                                 g_param_spec_boolean ("is-primary",
	                                                       "Primary?", "Indicates which postal address out of a group is primary.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDPostalAddress:mail-class:
	 *
	 * Classes of mail accepted at this address. For example: %GDATA_GD_MAIL_CLASS_LETTERS or %GDATA_GD_MAIL_CLASS_BOTH.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_MAIL_CLASS,
	                                 g_param_spec_string ("mail-class",
	                                                      "Mail class", "Classes of mail accepted at this address.",
	                                                      GDATA_GD_MAIL_CLASS_BOTH,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDPostalAddress:usage:
	 *
	 * The context in which this address can be used. For example: %GDATA_GD_ADDRESS_USAGE_GENERAL or %GDATA_GD_ADDRESS_USAGE_LOCAL.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_USAGE,
	                                 g_param_spec_string ("usage",
	                                                      "Usage", "The context in which this address can be used.",
	                                                      GDATA_GD_ADDRESS_USAGE_GENERAL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDPostalAddress:agent:
	 *
	 * The agent who actually receives the mail. Used in work addresses. Also for "in care of" or "c/o".
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_AGENT,
	                                 g_param_spec_string ("agent",
	                                                      "Agent", "The agent who actually receives the mail.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDPostalAddress:house-name:
	 *
	 * Used in places where houses or buildings have names (and not necessarily numbers).
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_HOUSE_NAME,
	                                 g_param_spec_string ("house-name",
	                                                      "House name", "Used in places where houses or buildings have names (and not numbers).",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDPostalAddress:street:
	 *
	 * Can be street, avenue, road, etc. This element also includes the house number and room/apartment/flat/floor number.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_STREET,
	                                 g_param_spec_string ("street",
	                                                      "Street", "Can be street, avenue, road, etc.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDPostalAddress:po-box:
	 *
	 * Covers actual P.O. boxes, drawers, locked bags, etc. This is usually but not always mutually exclusive with #GDataGDPostalAddress:street.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_PO_BOX,
	                                 g_param_spec_string ("po-box",
	                                                      "PO box", "Covers actual P.O. boxes, drawers, locked bags, etc.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDPostalAddress:neighborhood:
	 *
	 * This is used to disambiguate a street address when a city contains more than one street with the same name, or to specify a small place
	 * whose mail is routed through a larger postal town. In China it could be a county or a minor city.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_NEIGHBORHOOD,
	                                 g_param_spec_string ("neighborhood",
	                                                      "Neighborhood", "This is used to disambiguate a street address.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDPostalAddress:city:
	 *
	 * Can be city, village, town, borough, etc. This is the postal town and not necessarily the place of residence or place of business.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_CITY,
	                                 g_param_spec_string ("city",
	                                                      "City", "Can be city, village, town, borough, etc.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDPostalAddress:subregion:
	 *
	 * Handles administrative districts such as U.S. or U.K. counties that are not used for mail addressing purposes.
	 * Subregion is not intended for delivery addresses.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_SUBREGION,
	                                 g_param_spec_string ("subregion",
	                                                      "Subregion", "Handles administrative districts such as U.S. or U.K. counties.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDPostalAddress:region:
	 *
	 * A state, province, county (in Ireland), Land (in Germany), departement (in France), etc.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_REGION,
	                                 g_param_spec_string ("region",
	                                                      "Region", "A state, province, county, Land, departement, etc.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDPostalAddress:postcode:
	 *
	 * Postal code. Usually country-wide, but sometimes specific to the city (e.g. "2" in "Dublin 2, Ireland" addresses).
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_POSTCODE,
	                                 g_param_spec_string ("postcode",
	                                                      "Postcode", "Postal code.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDPostalAddress:country:
	 *
	 * The name of the country. Since this is paired with #GDataGDPostalAddress:country-code, they must both be set with
	 * gdata_gd_postal_address_set_country().
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_COUNTRY,
	                                 g_param_spec_string ("country",
	                                                      "Country", "The name of the country.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDPostalAddress:country-code:
	 *
	 * The ISO 3166-1 alpha-2 country code for the country in #GDataGDPostalAddress:country. Since this is paired with
	 * #GDataGDPostalAddress:country, they must both be set with gdata_gd_postal_address_set_country().
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>
	 * or <ulink type="http" url="http://www.iso.org/iso/iso-3166-1_decoding_table">ISO 3166-1 alpha-2</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_COUNTRY_CODE,
	                                 g_param_spec_string ("country-code",
	                                                      "Country code", "The ISO 3166-1 alpha-2 country code for the country.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static gint
compare_with (GDataComparable *self, GDataComparable *other)
{
	GDataGDPostalAddressPrivate *a = ((GDataGDPostalAddress*) self)->priv, *b = ((GDataGDPostalAddress*) other)->priv;

	if (g_strcmp0 (a->street, b->street) == 0 && g_strcmp0 (a->po_box, b->po_box) == 0 &&
	    g_strcmp0 (a->city, b->city) == 0 && g_strcmp0 (a->postcode, b->postcode) == 0)
		return 0;
	return 1;
}

static void
gdata_gd_postal_address_comparable_init (GDataComparableIface *iface)
{
	iface->compare_with = compare_with;
}

static void
gdata_gd_postal_address_init (GDataGDPostalAddress *self)
{
	self->priv = gdata_gd_postal_address_get_instance_private (self);
}

static void
gdata_gd_postal_address_finalize (GObject *object)
{
	GDataGDPostalAddressPrivate *priv = GDATA_GD_POSTAL_ADDRESS (object)->priv;

	g_free (priv->formatted_address);
	g_free (priv->relation_type);
	g_free (priv->label);
	g_free (priv->mail_class);
	g_free (priv->usage);
	g_free (priv->agent);
	g_free (priv->house_name);
	g_free (priv->street);
	g_free (priv->po_box);
	g_free (priv->neighborhood);
	g_free (priv->city);
	g_free (priv->subregion);
	g_free (priv->region);
	g_free (priv->postcode);
	g_free (priv->country);
	g_free (priv->country_code);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gd_postal_address_parent_class)->finalize (object);
}

static void
gdata_gd_postal_address_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataGDPostalAddressPrivate *priv = GDATA_GD_POSTAL_ADDRESS (object)->priv;

	switch (property_id) {
		case PROP_FORMATTED_ADDRESS:
			g_value_set_string (value, priv->formatted_address);
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
		case PROP_MAIL_CLASS:
			g_value_set_string (value, priv->mail_class);
			break;
		case PROP_USAGE:
			g_value_set_string (value, priv->usage);
			break;
		case PROP_AGENT:
			g_value_set_string (value, priv->agent);
			break;
		case PROP_HOUSE_NAME:
			g_value_set_string (value, priv->house_name);
			break;
		case PROP_STREET:
			g_value_set_string (value, priv->street);
			break;
		case PROP_PO_BOX:
			g_value_set_string (value, priv->po_box);
			break;
		case PROP_NEIGHBORHOOD:
			g_value_set_string (value, priv->neighborhood);
			break;
		case PROP_CITY:
			g_value_set_string (value, priv->city);
			break;
		case PROP_SUBREGION:
			g_value_set_string (value, priv->subregion);
			break;
		case PROP_REGION:
			g_value_set_string (value, priv->region);
			break;
		case PROP_POSTCODE:
			g_value_set_string (value, priv->postcode);
			break;
		case PROP_COUNTRY:
			g_value_set_string (value, priv->country);
			break;
		case PROP_COUNTRY_CODE:
			g_value_set_string (value, priv->country_code);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_gd_postal_address_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataGDPostalAddress *self = GDATA_GD_POSTAL_ADDRESS (object);

	switch (property_id) {
		case PROP_FORMATTED_ADDRESS:
			gdata_gd_postal_address_set_address (self, g_value_get_string (value));
			break;
		case PROP_RELATION_TYPE:
			gdata_gd_postal_address_set_relation_type (self, g_value_get_string (value));
			break;
		case PROP_LABEL:
			gdata_gd_postal_address_set_label (self, g_value_get_string (value));
			break;
		case PROP_IS_PRIMARY:
			gdata_gd_postal_address_set_is_primary (self, g_value_get_boolean (value));
			break;
		case PROP_MAIL_CLASS:
			gdata_gd_postal_address_set_mail_class (self, g_value_get_string (value));
			break;
		case PROP_USAGE:
			gdata_gd_postal_address_set_usage (self, g_value_get_string (value));
			break;
		case PROP_AGENT:
			gdata_gd_postal_address_set_agent (self, g_value_get_string (value));
			break;
		case PROP_HOUSE_NAME:
			gdata_gd_postal_address_set_house_name (self, g_value_get_string (value));
			break;
		case PROP_STREET:
			gdata_gd_postal_address_set_street (self, g_value_get_string (value));
			break;
		case PROP_PO_BOX:
			gdata_gd_postal_address_set_po_box (self, g_value_get_string (value));
			break;
		case PROP_NEIGHBORHOOD:
			gdata_gd_postal_address_set_neighborhood (self, g_value_get_string (value));
			break;
		case PROP_CITY:
			gdata_gd_postal_address_set_city (self, g_value_get_string (value));
			break;
		case PROP_SUBREGION:
			gdata_gd_postal_address_set_subregion (self, g_value_get_string (value));
			break;
		case PROP_REGION:
			gdata_gd_postal_address_set_region (self, g_value_get_string (value));
			break;
		case PROP_POSTCODE:
			gdata_gd_postal_address_set_postcode (self, g_value_get_string (value));
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
	xmlChar *rel;
	gboolean primary_bool;
	GDataGDPostalAddressPrivate *priv = GDATA_GD_POSTAL_ADDRESS (parsable)->priv;

	/* Is it the primary postal address? */
	if (gdata_parser_boolean_from_property (root_node, "primary", &primary_bool, 0, error) == FALSE)
		return FALSE;

	rel = xmlGetProp (root_node, (xmlChar*) "rel");
	if (rel != NULL && *rel == '\0') {
		xmlFree (rel);
		return gdata_parser_error_required_property_missing (root_node, "rel", error);
	}

	priv->relation_type = (gchar*) rel;
	priv->label = (gchar*) xmlGetProp (root_node, (xmlChar*) "label");
	priv->mail_class = (gchar*) xmlGetProp (root_node, (xmlChar*) "mailClass");
	priv->usage = (gchar*) xmlGetProp (root_node, (xmlChar*) "usage");
	priv->is_primary = primary_bool;

	return TRUE;
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	gboolean success;
	GDataGDPostalAddressPrivate *priv = GDATA_GD_POSTAL_ADDRESS (parsable)->priv;

	if (gdata_parser_is_namespace (node, "http://schemas.google.com/g/2005") == TRUE) {
		if (gdata_parser_string_from_element (node, "agent", P_NO_DUPES, &(priv->agent), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "housename", P_NO_DUPES, &(priv->house_name), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "pobox", P_NO_DUPES, &(priv->po_box), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "street", P_NO_DUPES, &(priv->street), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "neighborhood", P_NO_DUPES, &(priv->neighborhood), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "city", P_NO_DUPES, &(priv->city), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "subregion", P_NO_DUPES, &(priv->subregion), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "region", P_NO_DUPES, &(priv->region), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "postcode", P_NO_DUPES, &(priv->postcode), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "formattedAddress", P_NO_DUPES, &(priv->formatted_address), &success, error) == TRUE) {
			return success;
		} else if (xmlStrcmp (node->name, (xmlChar*) "country") == 0) {
			/* gd:country */
			priv->country_code = (gchar*) xmlGetProp (node, (xmlChar*) "code");
			priv->country = (gchar*) xmlNodeListGetString (doc, node->children, TRUE);

			return TRUE;
		}
	}

	return GDATA_PARSABLE_CLASS (gdata_gd_postal_address_parent_class)->parse_xml (parsable, doc, node, user_data, error);
}

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGDPostalAddressPrivate *priv = GDATA_GD_POSTAL_ADDRESS (parsable)->priv;

	if (priv->relation_type != NULL)
		gdata_parser_string_append_escaped (xml_string, " rel='", priv->relation_type, "'");
	if (priv->label != NULL)
		gdata_parser_string_append_escaped (xml_string, " label='", priv->label, "'");
	if (priv->mail_class != NULL)
		gdata_parser_string_append_escaped (xml_string, " mailClass='", priv->mail_class, "'");
	if (priv->usage != NULL)
		gdata_parser_string_append_escaped (xml_string, " usage='", priv->usage, "'");

	if (priv->is_primary == TRUE)
		g_string_append (xml_string, " primary='true'");
	else
		g_string_append (xml_string, " primary='false'");
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGDPostalAddressPrivate *priv = GDATA_GD_POSTAL_ADDRESS (parsable)->priv;

#define OUTPUT_STRING_ELEMENT(E,F)									\
	if (priv->F != NULL)										\
		gdata_parser_string_append_escaped (xml_string, "<gd:" E ">", priv->F, "</gd:" E ">");

	OUTPUT_STRING_ELEMENT ("agent", agent)
	OUTPUT_STRING_ELEMENT ("housename", house_name)
	OUTPUT_STRING_ELEMENT ("street", street)
	OUTPUT_STRING_ELEMENT ("pobox", po_box)
	OUTPUT_STRING_ELEMENT ("neighborhood", neighborhood)
	OUTPUT_STRING_ELEMENT ("city", city)
	OUTPUT_STRING_ELEMENT ("subregion", subregion)
	OUTPUT_STRING_ELEMENT ("region", region)
	OUTPUT_STRING_ELEMENT ("postcode", postcode)

	if (priv->country != NULL) {
		if (priv->country_code != NULL)
			gdata_parser_string_append_escaped (xml_string, "<gd:country code='", priv->country_code, "'>");
		else
			g_string_append (xml_string, "<gd:country>");
		gdata_parser_string_append_escaped (xml_string, NULL, priv->country, "</gd:country>");
	}

	OUTPUT_STRING_ELEMENT ("formattedAddress", formatted_address)

#undef OUTPUT_STRING_ELEMENT
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "gd", (gchar*) "http://schemas.google.com/g/2005");
}

/**
 * gdata_gd_postal_address_new:
 * @relation_type: (allow-none): the relationship between the address and its owner, or %NULL
 * @label: (allow-none): a human-readable label for the address, or %NULL
 * @is_primary: %TRUE if this phone number is its owner's primary number, %FALSE otherwise
 *
 * Creates a new #GDataGDPostalAddress. More information is available in the <ulink type="http"
 * url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdStructuredPostalAddress">GData specification</ulink>.
 *
 * Return value: a new #GDataGDPostalAddress, or %NULL; unref with g_object_unref()
 *
 * Since: 0.2.0
 */
GDataGDPostalAddress *
gdata_gd_postal_address_new (const gchar *relation_type, const gchar *label, gboolean is_primary)
{
	g_return_val_if_fail (relation_type == NULL || *relation_type != '\0', NULL);
	return g_object_new (GDATA_TYPE_GD_POSTAL_ADDRESS, "relation-type", relation_type, "label", label, "is-primary", is_primary, NULL);
}

/**
 * gdata_gd_postal_address_get_address:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:address property.
 *
 * Return value: the postal address itself, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_gd_postal_address_get_address (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), NULL);
	return self->priv->formatted_address;
}

/**
 * gdata_gd_postal_address_set_address:
 * @self: a #GDataGDPostalAddress
 * @address: (allow-none): the new postal address, or %NULL
 *
 * Sets the #GDataGDPostalAddress:address property to @address.
 *
 * Since: 0.4.0
 */
void
gdata_gd_postal_address_set_address (GDataGDPostalAddress *self, const gchar *address)
{
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self));

	/* Trim leading and trailing whitespace from the address.
	 * See here: http://code.google.com/apis/gdata/docs/1.0/elements.html#gdPostalAddress */
	g_free (self->priv->formatted_address);
	self->priv->formatted_address = gdata_parser_utf8_trim_whitespace (address);
	g_object_notify (G_OBJECT (self), "address");
}

/**
 * gdata_gd_postal_address_get_relation_type:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:relation-type property.
 *
 * Return value: the postal address' relation type, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_gd_postal_address_get_relation_type (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), NULL);
	return self->priv->relation_type;
}

/**
 * gdata_gd_postal_address_set_relation_type:
 * @self: a #GDataGDPostalAddress
 * @relation_type: (allow-none): the new relation type for the postal_address, or %NULL
 *
 * Sets the #GDataGDPostalAddress:relation-type property to @relation_type.
 *
 * Set @relation_type to %NULL to unset the property in the postal address.
 *
 * Since: 0.4.0
 */
void
gdata_gd_postal_address_set_relation_type (GDataGDPostalAddress *self, const gchar *relation_type)
{
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self));
	g_return_if_fail (relation_type == NULL || *relation_type != '\0');

	g_free (self->priv->relation_type);
	self->priv->relation_type = g_strdup (relation_type);
	g_object_notify (G_OBJECT (self), "relation-type");
}

/**
 * gdata_gd_postal_address_get_label:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:label property.
 *
 * Return value: the postal address' label, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_gd_postal_address_get_label (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), NULL);
	return self->priv->label;
}

/**
 * gdata_gd_postal_address_set_label:
 * @self: a #GDataGDPostalAddress
 * @label: (allow-none): the new label for the postal address, or %NULL
 *
 * Sets the #GDataGDPostalAddress:label property to @label.
 *
 * Set @label to %NULL to unset the property in the postal address.
 *
 * Since: 0.4.0
 */
void
gdata_gd_postal_address_set_label (GDataGDPostalAddress *self, const gchar *label)
{
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self));

	g_free (self->priv->label);
	self->priv->label = g_strdup (label);
	g_object_notify (G_OBJECT (self), "label");
}

/**
 * gdata_gd_postal_address_is_primary:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:is-primary property.
 *
 * Return value: %TRUE if this is the primary postal address, %FALSE otherwise
 *
 * Since: 0.4.0
 */
gboolean
gdata_gd_postal_address_is_primary (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), FALSE);
	return self->priv->is_primary;
}

/**
 * gdata_gd_postal_address_set_is_primary:
 * @self: a #GDataGDPostalAddress
 * @is_primary: %TRUE if this is the primary postal address, %FALSE otherwise
 *
 * Sets the #GDataGDPostalAddress:is-primary property to @is_primary.
 *
 * Since: 0.4.0
 */
void
gdata_gd_postal_address_set_is_primary (GDataGDPostalAddress *self, gboolean is_primary)
{
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self));

	self->priv->is_primary = is_primary;
	g_object_notify (G_OBJECT (self), "is-primary");
}

/**
 * gdata_gd_postal_address_get_mail_class:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:mail-class property.
 *
 * Return value: the postal address' mail class, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_gd_postal_address_get_mail_class (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), NULL);
	return self->priv->mail_class;
}

/**
 * gdata_gd_postal_address_set_mail_class:
 * @self: a #GDataGDPostalAddress
 * @mail_class: (allow-none): the new mail class for the postal address, or %NULL
 *
 * Sets the #GDataGDPostalAddress:mail-class property to @mail_class.
 *
 * Set @mail_class to %NULL to unset the property in the postal address.
 *
 * Since: 0.5.0
 */
void
gdata_gd_postal_address_set_mail_class (GDataGDPostalAddress *self, const gchar *mail_class)
{
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self));
	g_return_if_fail (mail_class == NULL || *mail_class != '\0');

	g_free (self->priv->mail_class);
	self->priv->mail_class = g_strdup (mail_class);
	g_object_notify (G_OBJECT (self), "mail-class");
}

/**
 * gdata_gd_postal_address_get_usage:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:usage property.
 *
 * Return value: the postal address' usage, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_gd_postal_address_get_usage (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), NULL);
	return self->priv->usage;
}

/**
 * gdata_gd_postal_address_set_usage:
 * @self: a #GDataGDPostalAddress
 * @usage: (allow-none): the new usage for the postal address, or %NULL
 *
 * Sets the #GDataGDPostalAddress:usage property to @usage.
 *
 * Set @usage to %NULL to unset the property in the postal address.
 *
 * Since: 0.5.0
 */
void
gdata_gd_postal_address_set_usage (GDataGDPostalAddress *self, const gchar *usage)
{
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self));
	g_return_if_fail (usage == NULL || *usage != '\0');

	g_free (self->priv->usage);
	self->priv->usage = g_strdup (usage);
	g_object_notify (G_OBJECT (self), "usage");
}

/**
 * gdata_gd_postal_address_get_agent:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:agent property.
 *
 * Return value: the postal address' agent, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_gd_postal_address_get_agent (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), NULL);
	return self->priv->agent;
}

/**
 * gdata_gd_postal_address_set_agent:
 * @self: a #GDataGDPostalAddress
 * @agent: (allow-none): the new agent for the postal address, or %NULL
 *
 * Sets the #GDataGDPostalAddress:agent property to @agent.
 *
 * Set @agent to %NULL to unset the property in the postal address.
 *
 * Since: 0.5.0
 */
void
gdata_gd_postal_address_set_agent (GDataGDPostalAddress *self, const gchar *agent)
{
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self));
	g_return_if_fail (agent == NULL || *agent != '\0');

	g_free (self->priv->agent);
	self->priv->agent = g_strdup (agent);
	g_object_notify (G_OBJECT (self), "agent");
}

/**
 * gdata_gd_postal_address_get_house_name:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:house-name property.
 *
 * Return value: the postal address' house name, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_gd_postal_address_get_house_name (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), NULL);
	return self->priv->house_name;
}

/**
 * gdata_gd_postal_address_set_house_name:
 * @self: a #GDataGDPostalAddress
 * @house_name: (allow-none): the new house name for the postal address, or %NULL
 *
 * Sets the #GDataGDPostalAddress:house-name property to @house_name.
 *
 * Set @house_name to %NULL to unset the property in the postal address.
 *
 * Since: 0.5.0
 */
void
gdata_gd_postal_address_set_house_name (GDataGDPostalAddress *self, const gchar *house_name)
{
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self));
	g_return_if_fail (house_name == NULL || *house_name != '\0');

	g_free (self->priv->house_name);
	self->priv->house_name = g_strdup (house_name);
	g_object_notify (G_OBJECT (self), "house-name");
}

/**
 * gdata_gd_postal_address_get_street:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:street property.
 *
 * Return value: the postal address' street, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_gd_postal_address_get_street (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), NULL);
	return self->priv->street;
}

/**
 * gdata_gd_postal_address_set_street:
 * @self: a #GDataGDPostalAddress
 * @street: (allow-none): the new street for the postal address, or %NULL
 *
 * Sets the #GDataGDPostalAddress:street property to @street.
 *
 * Set @street to %NULL to unset the property in the postal address.
 *
 * Since: 0.5.0
 */
void
gdata_gd_postal_address_set_street (GDataGDPostalAddress *self, const gchar *street)
{
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self));
	g_return_if_fail (street == NULL || *street != '\0');

	g_free (self->priv->street);
	self->priv->street = g_strdup (street);
	g_object_notify (G_OBJECT (self), "street");
}

/**
 * gdata_gd_postal_address_get_po_box:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:po-box property.
 *
 * Return value: the postal address' P.O. box, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_gd_postal_address_get_po_box (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), NULL);
	return self->priv->po_box;
}

/**
 * gdata_gd_postal_address_set_po_box:
 * @self: a #GDataGDPostalAddress
 * @po_box: (allow-none): the new P.O. box for the postal address, or %NULL
 *
 * Sets the #GDataGDPostalAddress:po-box property to @po_box.
 *
 * Set @po_box to %NULL to unset the property in the postal address.
 *
 * Since: 0.5.0
 */
void
gdata_gd_postal_address_set_po_box (GDataGDPostalAddress *self, const gchar *po_box)
{
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self));
	g_return_if_fail (po_box == NULL || *po_box != '\0');

	g_free (self->priv->po_box);
	self->priv->po_box = g_strdup (po_box);
	g_object_notify (G_OBJECT (self), "po-box");
}

/**
 * gdata_gd_postal_address_get_neighborhood:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:neighborhood property.
 *
 * Return value: the postal address' neighborhood, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_gd_postal_address_get_neighborhood (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), NULL);
	return self->priv->neighborhood;
}

/**
 * gdata_gd_postal_address_set_neighborhood:
 * @self: a #GDataGDPostalAddress
 * @neighborhood: (allow-none): the new neighborhood for the postal address, or %NULL
 *
 * Sets the #GDataGDPostalAddress:neighborhood property to @neighborhood.
 *
 * Set @neighborhood to %NULL to unset the property in the postal address.
 *
 * Since: 0.5.0
 */
void
gdata_gd_postal_address_set_neighborhood (GDataGDPostalAddress *self, const gchar *neighborhood)
{
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self));
	g_return_if_fail (neighborhood == NULL || *neighborhood != '\0');

	g_free (self->priv->neighborhood);
	self->priv->neighborhood = g_strdup (neighborhood);
	g_object_notify (G_OBJECT (self), "neighborhood");
}

/**
 * gdata_gd_postal_address_get_city:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:city property.
 *
 * Return value: the postal address' city, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_gd_postal_address_get_city (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), NULL);
	return self->priv->city;
}

/**
 * gdata_gd_postal_address_set_city:
 * @self: a #GDataGDPostalAddress
 * @city: (allow-none): the new city for the postal address, or %NULL
 *
 * Sets the #GDataGDPostalAddress:city property to @city.
 *
 * Set @city to %NULL to unset the property in the postal address.
 *
 * Since: 0.5.0
 */
void
gdata_gd_postal_address_set_city (GDataGDPostalAddress *self, const gchar *city)
{
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self));
	g_return_if_fail (city == NULL || *city != '\0');

	g_free (self->priv->city);
	self->priv->city = g_strdup (city);
	g_object_notify (G_OBJECT (self), "city");
}

/**
 * gdata_gd_postal_address_get_subregion:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:subregion property.
 *
 * Return value: the postal address' subregion, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_gd_postal_address_get_subregion (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), NULL);
	return self->priv->subregion;
}

/**
 * gdata_gd_postal_address_set_subregion:
 * @self: a #GDataGDPostalAddress
 * @subregion: (allow-none): the new subregion for the postal address, or %NULL
 *
 * Sets the #GDataGDPostalAddress:subregion property to @subregion.
 *
 * Set @subregion to %NULL to unset the property in the postal address.
 *
 * Since: 0.5.0
 */
void
gdata_gd_postal_address_set_subregion (GDataGDPostalAddress *self, const gchar *subregion)
{
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self));
	g_return_if_fail (subregion == NULL || *subregion != '\0');

	g_free (self->priv->subregion);
	self->priv->subregion = g_strdup (subregion);
	g_object_notify (G_OBJECT (self), "subregion");
}

/**
 * gdata_gd_postal_address_get_region:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:region property.
 *
 * Return value: the postal address' region, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_gd_postal_address_get_region (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), NULL);
	return self->priv->region;
}

/**
 * gdata_gd_postal_address_set_region:
 * @self: a #GDataGDPostalAddress
 * @region: (allow-none): the new region for the postal address, or %NULL
 *
 * Sets the #GDataGDPostalAddress:region property to @region.
 *
 * Set @region to %NULL to unset the property in the postal address.
 *
 * Since: 0.5.0
 */
void
gdata_gd_postal_address_set_region (GDataGDPostalAddress *self, const gchar *region)
{
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self));
	g_return_if_fail (region == NULL || *region != '\0');

	g_free (self->priv->region);
	self->priv->region = g_strdup (region);
	g_object_notify (G_OBJECT (self), "region");
}

/**
 * gdata_gd_postal_address_get_postcode:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:postcode property.
 *
 * Return value: the postal address' postcode, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_gd_postal_address_get_postcode (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), NULL);
	return self->priv->postcode;
}

/**
 * gdata_gd_postal_address_set_postcode:
 * @self: a #GDataGDPostalAddress
 * @postcode: (allow-none): the new postcode for the postal address, or %NULL
 *
 * Sets the #GDataGDPostalAddress:postcode property to @postcode.
 *
 * Set @postcode to %NULL to unset the property in the postal address.
 *
 * Since: 0.5.0
 */
void
gdata_gd_postal_address_set_postcode (GDataGDPostalAddress *self, const gchar *postcode)
{
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self));
	g_return_if_fail (postcode == NULL || *postcode != '\0');

	g_free (self->priv->postcode);
	self->priv->postcode = g_strdup (postcode);
	g_object_notify (G_OBJECT (self), "postcode");
}

/**
 * gdata_gd_postal_address_get_country:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:country property.
 *
 * Return value: the postal address' country, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_gd_postal_address_get_country (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), NULL);
	return self->priv->country;
}

/**
 * gdata_gd_postal_address_get_country_code:
 * @self: a #GDataGDPostalAddress
 *
 * Gets the #GDataGDPostalAddress:country-code property.
 *
 * Return value: the postal address' ISO 3166-1 alpha-2 country code, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_gd_postal_address_get_country_code (GDataGDPostalAddress *self)
{
	g_return_val_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self), NULL);
	return self->priv->country_code;
}

/**
 * gdata_gd_postal_address_set_country:
 * @self: a #GDataGDPostalAddress
 * @country: (allow-none): the new country for the postal address, or %NULL
 * @country_code: (allow-none): the new country code for the postal address, or %NULL
 *
 * Sets the #GDataGDPostalAddress:country property to @country, and #GDataGDPostalAddress:country-code to @country_code.
 *
 * Set @country or @country_code to %NULL to unset the relevant property in the postal address. If a @country_code is provided, a @country must
 * also be provided.
 *
 * Since: 0.5.0
 */
void
gdata_gd_postal_address_set_country (GDataGDPostalAddress *self, const gchar *country, const gchar *country_code)
{
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (self));
	g_return_if_fail (country != NULL || country_code == NULL);
	g_return_if_fail (country == NULL || *country != '\0');
	g_return_if_fail (country_code == NULL || *country_code != '\0');

	g_free (self->priv->country);
	g_free (self->priv->country_code);
	self->priv->country = g_strdup (country);
	self->priv->country_code = g_strdup (country_code);

	g_object_freeze_notify (G_OBJECT (self));
	g_object_notify (G_OBJECT (self), "country");
	g_object_notify (G_OBJECT (self), "country-code");
	g_object_thaw_notify (G_OBJECT (self));
}
