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
 * SECTION:gdata-gcontact-event
 * @short_description: gContact event element
 * @stability: Stable
 * @include: gdata/gcontact/gdata-gcontact-event.h
 *
 * #GDataGContactEvent represents a "event" element from the
 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcEvent">gContact specification</ulink>.
 *
 * Since: 0.7.0
 */

#include <glib.h>
#include <string.h>
#include <libxml/parser.h>

#include "gdata-gcontact-event.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"

static void gdata_gcontact_event_finalize (GObject *object);
static void gdata_gcontact_event_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_gcontact_event_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static gboolean post_parse_xml (GDataParsable *parsable, gpointer user_data, GError **error);
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataGContactEventPrivate {
	GDate date;
	gchar *relation_type;
	gchar *label;
};

enum {
	PROP_DATE = 1,
	PROP_RELATION_TYPE,
	PROP_LABEL
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataGContactEvent, gdata_gcontact_event, GDATA_TYPE_PARSABLE)

static void
gdata_gcontact_event_class_init (GDataGContactEventClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_gcontact_event_get_property;
	gobject_class->set_property = gdata_gcontact_event_set_property;
	gobject_class->finalize = gdata_gcontact_event_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->parse_xml = parse_xml;
	parsable_class->post_parse_xml = post_parse_xml;
	parsable_class->pre_get_xml = pre_get_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "event";
	parsable_class->element_namespace = "gContact";

	/**
	 * GDataGContactEvent:date:
	 *
	 * The date of the event.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcEvent">GContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_DATE,
	                                 g_param_spec_boxed ("date",
	                                                     "Date", "The date of the event.",
	                                                     G_TYPE_DATE,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGContactEvent:relation-type:
	 *
	 * A programmatic value that identifies the type of event. It is mutually exclusive with #GDataGContactEvent:label.
	 * Examples are %GDATA_GCONTACT_EVENT_ANNIVERSARY or %GDATA_GCONTACT_EVENT_OTHER.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcEvent">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_RELATION_TYPE,
	                                 g_param_spec_string ("relation-type",
	                                                      "Relation type", "A programmatic value that identifies the type of website.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGContactEvent:label:
	 *
	 * A simple string value used to name this event. It is mutually exclusive with #GDataGContactEvent:relation-type.
	 * It allows UIs to display a label such as "Wedding anniversary".
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcEvent">gContact specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_LABEL,
	                                 g_param_spec_string ("label",
	                                                      "Label", "A simple string value used to name this event.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_gcontact_event_init (GDataGContactEvent *self)
{
	self->priv = gdata_gcontact_event_get_instance_private (self);

	/* Clear the date to an invalid but sane value */
	g_date_clear (&(self->priv->date), 1);
}

static void
gdata_gcontact_event_finalize (GObject *object)
{
	GDataGContactEventPrivate *priv = GDATA_GCONTACT_EVENT (object)->priv;

	g_free (priv->relation_type);
	g_free (priv->label);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gcontact_event_parent_class)->finalize (object);
}

static void
gdata_gcontact_event_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataGContactEventPrivate *priv = GDATA_GCONTACT_EVENT (object)->priv;

	switch (property_id) {
		case PROP_DATE:
			g_value_set_boxed (value, &(priv->date));
			break;
		case PROP_RELATION_TYPE:
			g_value_set_string (value, priv->relation_type);
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
gdata_gcontact_event_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataGContactEvent *self = GDATA_GCONTACT_EVENT (object);

	switch (property_id) {
		case PROP_DATE:
			gdata_gcontact_event_set_date (self, g_value_get_boxed (value));
			break;
		case PROP_RELATION_TYPE:
			gdata_gcontact_event_set_relation_type (self, g_value_get_string (value));
			break;
		case PROP_LABEL:
			gdata_gcontact_event_set_label (self, g_value_get_string (value));
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
	xmlChar *rel, *label;
	GDataGContactEventPrivate *priv = GDATA_GCONTACT_EVENT (parsable)->priv;

	rel = xmlGetProp (root_node, (xmlChar*) "rel");
	label = xmlGetProp (root_node, (xmlChar*) "label");
	if ((rel == NULL || *rel == '\0') && (label == NULL || *label == '\0')) {
		xmlFree (rel);
		xmlFree (label);
		return gdata_parser_error_required_property_missing (root_node, "rel", error);
	} else if (rel != NULL && label != NULL) {
		/* Can't have both set at once */
		xmlFree (rel);
		xmlFree (label);
		return gdata_parser_error_mutexed_properties (root_node, "rel", "label", error);
	}

	priv->relation_type = (gchar*) rel;
	priv->label = (gchar*) label;

	return TRUE;
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	GDataGContactEventPrivate *priv = GDATA_GCONTACT_EVENT (parsable)->priv;

	if (gdata_parser_is_namespace (node, "http://schemas.google.com/g/2005") == TRUE && xmlStrcmp (node->name, (xmlChar*) "when") == 0) {
		xmlChar *start_time;
		guint year, month, day;

		/* gd:when; note we don't use GDataGDWhen here because gContact:event only uses a limited subset
		 * of gd:when (i.e. only the startTime property in date format) */
		if (g_date_valid (&(priv->date)) == TRUE)
			return gdata_parser_error_duplicate_element (node, error);

		start_time = xmlGetProp (node, (xmlChar*) "startTime");
		if (start_time == NULL)
			return gdata_parser_error_required_property_missing (node, "startTime", error);

		/* Try parsing the date format: YYYY-MM-DD */
		if (strlen ((char*) start_time) == 10 &&
		    sscanf ((char*) start_time, "%4u-%2u-%2u", &year, &month, &day) == 3 && g_date_valid_dmy (day, month, year) == TRUE) {
			/* Store the values in the GDate */
			g_date_set_dmy (&(priv->date), day, month, year);
			xmlFree (start_time);
		} else {
			/* Parsing failed */
			gdata_parser_error_not_iso8601_format (node, (gchar*) start_time, error);
			xmlFree (start_time);
			return FALSE;
		}

		return TRUE;
	}

	return GDATA_PARSABLE_CLASS (gdata_gcontact_event_parent_class)->parse_xml (parsable, doc, node, user_data, error);
}

static gboolean
post_parse_xml (GDataParsable *parsable, gpointer user_data, GError **error)
{
	GDataGContactEventPrivate *priv = GDATA_GCONTACT_EVENT (parsable)->priv;

	/* Check for missing required elements */
	if (g_date_valid (&(priv->date)) == FALSE)
		return gdata_parser_error_required_element_missing ("gd:when", "gContact:event", error);

	return TRUE;
}

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGContactEventPrivate *priv = GDATA_GCONTACT_EVENT (parsable)->priv;

	if (priv->relation_type != NULL)
		gdata_parser_string_append_escaped (xml_string, " rel='", priv->relation_type, "'");
	else if (priv->label != NULL)
		gdata_parser_string_append_escaped (xml_string, " label='", priv->label, "'");
	else
		g_assert_not_reached ();
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGContactEventPrivate *priv = GDATA_GCONTACT_EVENT (parsable)->priv;

	g_string_append_printf (xml_string, "<gd:when startTime='%04u-%02u-%02u'/>",
	                        g_date_get_year (&(priv->date)),
	                        g_date_get_month (&(priv->date)),
	                        g_date_get_day (&(priv->date)));
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "gd", (gchar*) "http://schemas.google.com/g/2005");
	g_hash_table_insert (namespaces, (gchar*) "gContact", (gchar*) "http://schemas.google.com/contact/2008");
}

/**
 * gdata_gcontact_event_new:
 * @date: the date of the event
 * @relation_type: (allow-none): the relationship between the event and its owner, or %NULL
 * @label: (allow-none): a human-readable label for the event, or %NULL
 *
 * Creates a new #GDataGContactEvent. More information is available in the <ulink type="http"
 * url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcEvent">gContact specification</ulink>.
 *
 * Exactly one of @relation_type and @label should be provided; the other must be %NULL.
 *
 * Return value: a new #GDataGContactEvent; unref with g_object_unref()
 *
 * Since: 0.7.0
 */
GDataGContactEvent *
gdata_gcontact_event_new (const GDate *date, const gchar *relation_type, const gchar *label)
{
	g_return_val_if_fail (date != NULL && g_date_valid (date) == TRUE, NULL);
	g_return_val_if_fail ((relation_type != NULL && *relation_type != '\0' && label == NULL) ||
	                      (relation_type == NULL && label != NULL && *label != '\0'), NULL);

	return g_object_new (GDATA_TYPE_GCONTACT_EVENT, "date", date, "relation-type", relation_type, "label", label, NULL);
}

/**
 * gdata_gcontact_event_get_date:
 * @self: a #GDataGContactEvent
 * @date: (out caller-allocates): return location for the date of the event
 *
 * Gets the #GDataGContactEvent:date property.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_event_get_date (GDataGContactEvent *self, GDate *date)
{
	g_return_if_fail (GDATA_IS_GCONTACT_EVENT (self));
	g_return_if_fail (date != NULL);
	*date = self->priv->date;
}

/**
 * gdata_gcontact_event_set_date:
 * @self: a #GDataGContactEvent
 * @date: the new date for the event
 *
 * Sets the #GDataGContactEvent:date property to @date.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_event_set_date (GDataGContactEvent *self, const GDate *date)
{
	g_return_if_fail (GDATA_IS_GCONTACT_EVENT (self));
	g_return_if_fail (date != NULL && g_date_valid (date) == TRUE);

	self->priv->date = *date;
	g_object_notify (G_OBJECT (self), "date");
}

/**
 * gdata_gcontact_event_get_relation_type:
 * @self: a #GDataGContactEvent
 *
 * Gets the #GDataGContactEvent:relation-type property.
 *
 * Return value: the event's relation type, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_event_get_relation_type (GDataGContactEvent *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_EVENT (self), NULL);
	return self->priv->relation_type;
}

/**
 * gdata_gcontact_event_set_relation_type:
 * @self: a #GDataGContactEvent
 * @relation_type: (allow-none): the new relation type for the event, or %NULL
 *
 * Sets the #GDataGContactEvent:relation-type property to @relation_type
 * such as %GDATA_GCONTACT_EVENT_ANNIVERSARY or %GDATA_GCONTACT_EVENT_OTHER.
 *
 * If @relation_type is %NULL, the relation type will be unset. When the #GDataGContactEvent is used in a query, however,
 * exactly one of #GDataGContactEvent:relation-type and #GDataGContactEvent:label must be %NULL.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_event_set_relation_type (GDataGContactEvent *self, const gchar *relation_type)
{
	g_return_if_fail (GDATA_IS_GCONTACT_EVENT (self));
	g_return_if_fail (relation_type == NULL || *relation_type != '\0');

	g_free (self->priv->relation_type);
	self->priv->relation_type = g_strdup (relation_type);
	g_object_notify (G_OBJECT (self), "relation-type");
}

/**
 * gdata_gcontact_event_get_label:
 * @self: a #GDataGContactEvent
 *
 * Gets the #GDataGContactEvent:label property.
 *
 * Return value: the event's label, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_gcontact_event_get_label (GDataGContactEvent *self)
{
	g_return_val_if_fail (GDATA_IS_GCONTACT_EVENT (self), NULL);
	return self->priv->label;
}

/**
 * gdata_gcontact_event_set_label:
 * @self: a #GDataGContactEvent
 * @label: (allow-none): the new label for the event, or %NULL
 *
 * Sets the #GDataGContactEvent:label property to @label.
 *
 * If @label is %NULL, the label will be unset. When the #GDataGContactEvent is used in a query, however,
 * exactly one of #GDataGContactEvent:relation-type and #GDataGContactEvent:label must be %NULL.
 *
 * Since: 0.7.0
 */
void
gdata_gcontact_event_set_label (GDataGContactEvent *self, const gchar *label)
{
	g_return_if_fail (GDATA_IS_GCONTACT_EVENT (self));
	g_return_if_fail (label == NULL || *label != '\0');

	g_free (self->priv->label);
	self->priv->label = g_strdup (label);
	g_object_notify (G_OBJECT (self), "label");
}
