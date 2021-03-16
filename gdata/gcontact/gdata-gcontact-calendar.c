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
 * SECTION:gdata-gcontact-calendar
 * @short_description: gContact calendar element
 * @stability: Stable
 * @include: gdata/gcontact/gdata-gcontact-calendar.h
 *
 * #GDataGContactCalendar represents a "calendarLink" element from the
 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcCalendarLink">gContact specification</ulink>.
 *
 * Since: 0.7.0
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-gcontact-calendar.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-comparable.h"

static void gdata_gcontact_calendar_comparable_init (GDataComparableIface *iface);
static void gdata_gcontact_calendar_finalize (GObject *object);
static void gdata_gcontact_calendar_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_gcontact_calendar_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataGContactCalendarPrivate {
	gchar *uri;
	gchar *relation_type;
	gchar *label;
	gboolean is_primary;
};

enum {
	PROP_URI = 1,
	PROP_RELATION_TYPE,
	PROP_LABEL,
	PROP_IS_PRIMARY
};

G_DEFINE_TYPE_WITH_CODE (GDataGContactCalendar, gdata_gcontact_calendar, GDATA_TYPE_PARSABLE,
                         G_ADD_PRIVATE (GDataGContactCalendar)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMPARABLE, gdata_gcontact_calendar_comparable_init))

static void
gdata_gcontact_calendar_class_init (GDataGContactCalendarClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_gcontact_calendar_get_property;
	gobject_class->set_property = gdata_gcontact_calendar_set_property;
	gobject_class->finalize = gdata_gcontact_calendar_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->pre_get_xml = pre_get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "calendarLink";
	parsable_class->element_namespace = "gContact";

	/**
	 * GDataGContactCalendar:uri:
	 *
	 * The URI of the calendar.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcCalendarLink">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_URI,
	                                 g_param_spec_string ("uri",
	                                                      "URI", "The URI of the calendar.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGContactCalendar:relation-type:
	 *
	 * A programmatic value that identifies the type of calendar. It is mutually exclusive with #GDataGContactCalendar:label.
	 * Examples are %GDATA_GCONTACT_CALENDAR_HOME or %GDATA_GCONTACT_CALENDAR_FREE_BUSY.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcCalendarLink">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_RELATION_TYPE,
	                                 g_param_spec_string ("relation-type",
	                                                      "Relation type", "A programmatic value that identifies the type of calendar.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGContactCalendar:label:
	 *
	 * A free-form string that identifies the type of calendar. It is mutually exclusive with #GDataGContactCalendar:relation-type.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcCalendarLink">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_LABEL,
	                                 g_param_spec_string ("label",
	                                                      "Label", "A free-form string that identifies the type of calendar.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGContactCalendar:is-primary:
	 *
	 * Indicates which calendar out of a group is primary.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcCalendarLink">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_IS_PRIMARY,
	                                 g_param_spec_boolean ("is-primary",
	                                                       "Primary?", "Indicates which calendar out of a group is primary.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static gint
compare_with (GDataComparable *self, GDataComparable *other)
{
	GDataGContactCalendarPrivate *a = ((GDataGContactCalendar*) self)->priv, *b = ((GDataGContactCalendar*) other)->priv;

	if (g_strcmp0 (a->uri, b->uri) == 0 && g_strcmp0 (a->relation_type, b->relation_type) == 0 && g_strcmp0 (a->label, b->label) == 0)
		return 0;
	return 1;
}

static void
gdata_gcontact_calendar_comparable_init (GDataComparableIface *iface)
{
	iface->compare_with = compare_with;
}

static void
gdata_gcontact_calendar_init (GDataGContactCalendar *self)
{
	self->priv = gdata_gcontact_calendar_get_instance_private (self);
}

static void
gdata_gcontact_calendar_finalize (GObject *object)
{
	GDataGContactCalendarPrivate *priv = GDATA_GCONTACT_CALENDAR (object)->priv;

	g_free (priv->uri);
	g_free (priv->relation_type);
	g_free (priv->label);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gcontact_calendar_parent_class)->finalize (object);
}

static void
gdata_gcontact_calendar_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataGContactCalendarPrivate *priv = GDATA_GCONTACT_CALENDAR (object)->priv;

	switch (property_id) {
		case PROP_URI:
			g_value_set_string (value, priv->uri);
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
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_gcontact_calendar_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataGContactCalendar *self = GDATA_GCONTACT_CALENDAR (object);

	switch (property_id) {
		case PROP_URI:
			gdata_gcontact_calendar_set_uri (self, g_value_get_string (value));
			break;
		case PROP_RELATION_TYPE:
			gdata_gcontact_calendar_set_relation_type (self, g_value_get_string (value));
			break;
		case PROP_LABEL:
			gdata_gcontact_calendar_set_label (self, g_value_get_string (value));
			break;
		case PROP_IS_PRIMARY:
			gdata_gcontact_calendar_set_is_primary (self, g_value_get_boolean (value));
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
	xmlChar *uri, *rel, *label;
	gboolean primary_bool;
	GDataGContactCalendarPrivate *priv = GDATA_GCONTACT_CALENDAR (parsable)->priv;

	/* Is it the primary calendar? */
	if (gdata_parser_boolean_from_property (root_node, "primary", &primary_bool, 0, error) == FALSE)
		return FALSE;

	uri = xmlGetProp (root_node, (xmlChar*) "href");
	if (uri == NULL || *uri == '\0') {
		xmlFree (uri);
		return gdata_parser_error_required_property_missing (root_node, "href", error);
	}

	rel = xmlGetProp (root_node, (xmlChar*) "rel");
	label = xmlGetProp (root_node, (xmlChar*) "label");
	if ((rel == NULL || *rel == '\0') && (label == NULL || *label == '\0')) {
		xmlFree (uri);
		xmlFree (rel);
		xmlFree (label);
		return gdata_parser_error_required_property_missing (root_node, "rel", error);
	} else if (rel != NULL && label != NULL) {
		/* Can't have both set at once */
		xmlFree (uri);
		xmlFree (rel);
		xmlFree (label);
		return gdata_parser_error_mutexed_properties (root_node, "rel", "label", error);
	}

	priv->uri = (gchar*) uri;
	priv->relation_type = (gchar*) rel;
	priv->label = (gchar*) label;
	priv->is_primary = primary_bool;

	return TRUE;
}

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGContactCalendarPrivate *priv = GDATA_GCONTACT_CALENDAR (parsable)->priv;

	gdata_parser_string_append_escaped (xml_string, " href='", priv->uri, "'");

	if (priv->relation_type != NULL)
		gdata_parser_string_append_escaped (xml_string, " rel='", priv->relation_type, "'");
	else if (priv->label != NULL)
		gdata_parser_string_append_escaped (xml_string, " label='", priv->label, "'");
	else
		g_assert_not_reached ();

	if (priv->is_primary == TRUE)
		g_string_append (xml_string, " primary='true'");
	else
		g_string_append (xml_string, " primary='false'");
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "gContact", (gchar*) "http://schemas.google.com/contact/2008");
}

/**
 * gdata_gcontact_calendar_new:
 * @uri: the URI of the calendar
 * @relation_type: (allow-none): the type of calendar, or %NULL
 * @label: (allow-none): a free-form label for the calendar, or %NULL
 * @is_primary: %TRUE if this calendar is its owner's primary calendar, %FALSE otherwise
 *
 * Creates a new #GDataGContactCalendar. More information is available in the <ulink type="http"
 * url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcCalendarLink">gContact specification</ulink>.
 *
 * Exactly one of @relation_type and @label should be provided; the other must be %NULL.
 *
 * Return value: a new #GDataGContactCalendar; unref with g_object_unref()
 *
 * Since: 0.7.0
 */
GDataGContactCalendar *
gdata_gcontact_calendar_new (const gchar *uri, const gchar *relation_type, const gchar *label, gboolean is_primary)
{
	g_return_val_if_fail (uri != NULL && *uri != '\0', NULL);
	g_return_val_if_fail ((relation_type != NULL && *relation_type != '\0' && label == NULL) ||
	                      (relation_type == NULL && label != NULL && *label != '\0'), NULL);

	return g_object_new (GDATA_TYPE_GCONTACT_CALENDAR,
	                     "uri", uri, "relation-type", relation_type, "label", label, "is-primary", is_primary, NULL);
}

/**
 * gdata_gcontact_calendar_get_uri:
 * @self: a #GDataGContactCalendar
 *
 * Gets the #GDataGContactCalendar:uri property.
 *
 * Return value: the calendar's URI
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_calendar_get_uri (GDataGContactCalendar *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_CALENDAR (self), NULL);
	return self->priv->uri;
}

/**
 * gdata_gcontact_calendar_set_uri:
 * @self: a #GDataGContactCalendar
 * @uri: the new URI for the calendar
 *
 * Sets the #GDataGContactCalendar:uri property to @uri.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_calendar_set_uri (GDataGContactCalendar *self, const gchar *uri)
{
	g_return_if_fail (GDATA_IS_GCONTACT_CALENDAR (self));
	g_return_if_fail (uri != NULL && *uri != '\0');

	g_free (self->priv->uri);
	self->priv->uri = g_strdup (uri);
	g_object_notify (G_OBJECT (self), "uri");
}

/**
 * gdata_gcontact_calendar_get_relation_type:
 * @self: a #GDataGContactCalendar
 *
 * Gets the #GDataGContactCalendar:relation-type property.
 *
 * Return value: the type of the relation, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_calendar_get_relation_type (GDataGContactCalendar *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_CALENDAR (self), NULL);
	return self->priv->relation_type;
}

/**
 * gdata_gcontact_calendar_set_relation_type:
 * @self: a #GDataGContactCalendar
 * @relation_type: (allow-none): the new type for the calendar, or %NULL
 *
 * Sets the #GDataGContactCalendar:relation-type property to @relation_type,
 * such as %GDATA_GCONTACT_CALENDAR_HOME or %GDATA_GCONTACT_CALENDAR_FREE_BUSY.
 *
 * If @relation_type is %NULL, the relation type will be unset. When the #GDataGContactCalendar is used in a query, however,
 * exactly one of #GDataGContactCalendar:relation-type and #GDataGContactCalendar:label must be %NULL.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_calendar_set_relation_type (GDataGContactCalendar *self, const gchar *relation_type)
{
	g_return_if_fail (GDATA_IS_GCONTACT_CALENDAR (self));
	g_return_if_fail (relation_type == NULL || *relation_type != '\0');

	g_free (self->priv->relation_type);
	self->priv->relation_type = g_strdup (relation_type);
	g_object_notify (G_OBJECT (self), "relation-type");
}

/**
 * gdata_gcontact_calendar_get_label:
 * @self: a #GDataGContactCalendar
 *
 * Gets the #GDataGContactCalendar:label property.
 *
 * Return value: a free-form label for the calendar, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_calendar_get_label (GDataGContactCalendar *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_CALENDAR (self), NULL);
	return self->priv->label;
}

/**
 * gdata_gcontact_calendar_set_label:
 * @self: a #GDataGContactCalendar
 * @label: (allow-none): the new free-form label for the calendar, or %NULL
 *
 * Sets the #GDataGContactCalendar:label property to @label.
 *
 * If @label is %NULL, the label will be unset. When the #GDataGContactCalendar is used in a query, however,
 * exactly one of #GDataGContactCalendar:relation-type and #GDataGContactCalendar:label must be %NULL.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_calendar_set_label (GDataGContactCalendar *self, const gchar *label)
{
	g_return_if_fail (GDATA_IS_GCONTACT_CALENDAR (self));
	g_return_if_fail (label == NULL || *label != '\0');

	g_free (self->priv->label);
	self->priv->label = g_strdup (label);
	g_object_notify (G_OBJECT (self), "label");
}

/**
 * gdata_gcontact_calendar_is_primary:
 * @self: a #GDataGContactCalendar
 *
 * Gets the #GDataGContactCalendar:is-primary property.
 *
 * Return value: %TRUE if this is the contact's primary calendar, %FALSE otherwise
 *
 * Since: 0.7.0
 */
gboolean
gdata_gcontact_calendar_is_primary (GDataGContactCalendar *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_CALENDAR (self), FALSE);
	return self->priv->is_primary;
}

/**
 * gdata_gcontact_calendar_set_is_primary:
 * @self: a #GDataGContactCalendar
 * @is_primary: %TRUE if this is the contact's primary calendar, %FALSE otherwise
 *
 * Sets the #GDataGContactCalendar:is-primary property to @is_primary.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_calendar_set_is_primary (GDataGContactCalendar *self, gboolean is_primary)
{
	g_return_if_fail (GDATA_IS_GCONTACT_CALENDAR (self));

	self->priv->is_primary = is_primary;
	g_object_notify (G_OBJECT (self), "is-primary");
}
