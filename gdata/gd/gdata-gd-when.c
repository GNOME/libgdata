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
 * SECTION:gdata-gd-when
 * @short_description: GData when element
 * @stability: Stable
 * @include: gdata/gd/gdata-gd-when.h
 *
 * #GDataGDWhen represents a "when" element from the
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdWhen">GData specification</ulink>.
 *
 * Since: 0.4.0
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-gd-when.h"
#include "gdata-gd-reminder.h"
#include "gdata-private.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "gdata-comparable.h"

static void gdata_gd_when_comparable_init (GDataComparableIface *iface);
static void gdata_gd_when_dispose (GObject *object);
static void gdata_gd_when_finalize (GObject *object);
static void gdata_gd_when_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_gd_when_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static gboolean post_parse_xml (GDataParsable *parsable, gpointer user_data, GError **error);
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataGDWhenPrivate {
	gint64 start_time;
	gint64 end_time;
	gboolean is_date;
	gchar *value_string;
	GList *reminders;
};

enum {
	PROP_START_TIME = 1,
	PROP_END_TIME,
	PROP_IS_DATE,
	PROP_VALUE_STRING
};

G_DEFINE_TYPE_WITH_CODE (GDataGDWhen, gdata_gd_when, GDATA_TYPE_PARSABLE,
                         G_ADD_PRIVATE (GDataGDWhen)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMPARABLE, gdata_gd_when_comparable_init))

static void
gdata_gd_when_class_init (GDataGDWhenClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_gd_when_get_property;
	gobject_class->set_property = gdata_gd_when_set_property;
	gobject_class->dispose = gdata_gd_when_dispose;
	gobject_class->finalize = gdata_gd_when_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->parse_xml = parse_xml;
	parsable_class->post_parse_xml = post_parse_xml;
	parsable_class->pre_get_xml = pre_get_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "when";
	parsable_class->element_namespace = "gd";

	/**
	 * GDataGDWhen:start-time:
	 *
	 * The name of the when.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdWhen">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_START_TIME,
	                                 g_param_spec_int64 ("start-time",
	                                                     "Start time", "The name of the when.",
	                                                     0, G_MAXINT64, 0,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDWhen:end-time:
	 *
	 * The title of a person within the when.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdWhen">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_END_TIME,
	                                 g_param_spec_int64 ("end-time",
	                                                     "End time", "The title of a person within the when.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDWhen:is-date:
	 *
	 * A programmatic value that identifies the type of when.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdWhen">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_IS_DATE,
	                                 g_param_spec_boolean ("is-date",
	                                                       "Date?", "A programmatic value that identifies the type of when.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDWhen:value-string:
	 *
	 * A simple string value used to name this when. It allows UIs to display a label such as "Work", "Volunteer",
	 * "Professional Society", etc.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdWhen">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_VALUE_STRING,
	                                 g_param_spec_string ("value-string",
	                                                      "Value string", "A simple string value used to name this when.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static gint
compare_with (GDataComparable *self, GDataComparable *other)
{
	gint64 start_diff, end_diff;
	GDataGDWhenPrivate *a = ((GDataGDWhen*) self)->priv, *b = ((GDataGDWhen*) other)->priv;

	if (a->is_date != b->is_date)
		return CLAMP (b->is_date - a->is_date, -1, 1);

	start_diff = b->start_time - a->start_time;
	end_diff = b->end_time - a->end_time;

	if (start_diff == 0)
		return CLAMP (end_diff, -1, 1);
	return CLAMP (start_diff, -1, 1);
}

static void
gdata_gd_when_comparable_init (GDataComparableIface *iface)
{
	iface->compare_with = compare_with;
}

static void
gdata_gd_when_init (GDataGDWhen *self)
{
	self->priv = gdata_gd_when_get_instance_private (self);
	self->priv->end_time = -1;
}

static void
gdata_gd_when_dispose (GObject *object)
{
	GDataGDWhenPrivate *priv = GDATA_GD_WHEN (object)->priv;

	g_list_free_full (priv->reminders, g_object_unref);
	priv->reminders = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gd_when_parent_class)->dispose (object);
}

static void
gdata_gd_when_finalize (GObject *object)
{
	GDataGDWhenPrivate *priv = GDATA_GD_WHEN (object)->priv;

	g_free (priv->value_string);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gd_when_parent_class)->finalize (object);
}

static void
gdata_gd_when_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataGDWhenPrivate *priv = GDATA_GD_WHEN (object)->priv;

	switch (property_id) {
		case PROP_START_TIME:
			g_value_set_int64 (value, priv->start_time);
			break;
		case PROP_END_TIME:
			g_value_set_int64 (value, priv->end_time);
			break;
		case PROP_IS_DATE:
			g_value_set_boolean (value, priv->is_date);
			break;
		case PROP_VALUE_STRING:
			g_value_set_string (value, priv->value_string);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_gd_when_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataGDWhen *self = GDATA_GD_WHEN (object);

	switch (property_id) {
		case PROP_START_TIME:
			gdata_gd_when_set_start_time (self, g_value_get_int64 (value));
			break;
		case PROP_END_TIME:
			gdata_gd_when_set_end_time (self, g_value_get_int64 (value));
			break;
		case PROP_IS_DATE:
			gdata_gd_when_set_is_date (self, g_value_get_boolean (value));
			break;
		case PROP_VALUE_STRING:
			gdata_gd_when_set_value_string (self, g_value_get_string (value));
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
	GDataGDWhenPrivate *priv = GDATA_GD_WHEN (parsable)->priv;
	xmlChar *start_time, *end_time;
	gint64 start_time_int64, end_time_int64;
	gboolean is_date = FALSE;

	/* Start time */
	start_time = xmlGetProp (root_node, (xmlChar*) "startTime");
	if (gdata_parser_int64_from_date ((gchar*) start_time, &start_time_int64) == TRUE) {
		is_date = TRUE;
	} else if (gdata_parser_int64_from_iso8601 ((gchar*) start_time, &start_time_int64) == FALSE) {
		/* Error */
		gdata_parser_error_not_iso8601_format (root_node, (gchar*) start_time, error);
		xmlFree (start_time);
		return FALSE;
	}
	xmlFree (start_time);

	/* End time (optional) */
	end_time = xmlGetProp (root_node, (xmlChar*) "endTime");
	if (end_time != NULL) {
		gboolean success;

		if (is_date == TRUE)
			success = gdata_parser_int64_from_date ((gchar*) end_time, &end_time_int64);
		else
			success = gdata_parser_int64_from_iso8601 ((gchar*) end_time, &end_time_int64);

		if (success == FALSE) {
			/* Error */
			gdata_parser_error_not_iso8601_format (root_node, (gchar*) end_time, error);
			xmlFree (end_time);
			return FALSE;
		}
		xmlFree (end_time);
	} else {
		/* Give a default */
		end_time_int64 = -1;
	}

	priv->start_time = start_time_int64;
	priv->end_time = end_time_int64;
	priv->is_date = is_date;
	priv->value_string = (gchar*) xmlGetProp (root_node, (xmlChar*) "valueString");

	return TRUE;
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	gboolean success;

	if (gdata_parser_is_namespace (node, "http://schemas.google.com/g/2005") == TRUE &&
	    gdata_parser_object_from_element_setter (node, "reminder", P_REQUIRED, GDATA_TYPE_GD_REMINDER,
	                                             gdata_gd_when_add_reminder, parsable, &success, error) == TRUE) {
		return success;
	} else {
		return GDATA_PARSABLE_CLASS (gdata_gd_when_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}
}

static gboolean
post_parse_xml (GDataParsable *parsable, gpointer user_data, GError **error)
{
	GDataGDWhenPrivate *priv = GDATA_GD_WHEN (parsable)->priv;

	/* Reverse our lists of stuff */
	priv->reminders = g_list_reverse (priv->reminders);

	return TRUE;
}

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGDWhenPrivate *priv = GDATA_GD_WHEN (parsable)->priv;
	gchar *start_time;

	if (priv->is_date == TRUE)
		start_time = gdata_parser_date_from_int64 (priv->start_time);
	else
		start_time = gdata_parser_int64_to_iso8601 (priv->start_time);

	g_string_append_printf (xml_string, " startTime='%s'", start_time);
	g_free (start_time);

	if (priv->end_time != -1) {
		gchar *end_time;

		if (priv->is_date == TRUE)
			end_time = gdata_parser_date_from_int64 (priv->end_time);
		else
			end_time = gdata_parser_int64_to_iso8601 (priv->end_time);

		g_string_append_printf (xml_string, " endTime='%s'", end_time);
		g_free (end_time);
	}

	if (priv->value_string != NULL)
		gdata_parser_string_append_escaped (xml_string, " valueString='", priv->value_string, "'");
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GList *reminders;
	GDataGDWhenPrivate *priv = GDATA_GD_WHEN (parsable)->priv;

	for (reminders = priv->reminders; reminders != NULL; reminders = reminders->next)
		_gdata_parsable_get_xml (GDATA_PARSABLE (reminders->data), xml_string, FALSE);
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "gd", (gchar*) "http://schemas.google.com/g/2005");
}

/**
 * gdata_gd_when_new:
 * @start_time: when the event starts or (for zero-duration events) when it occurs
 * @end_time: when the event ends, or <code class="literal">-1</code>
 * @is_date: %TRUE if @start_time and @end_time specify dates rather than times, %FALSE otherwise
 *
 * Creates a new #GDataGDWhen. More information is available in the <ulink type="http"
 * url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdWhen">GData specification</ulink>.
 *
 * Return value: a new #GDataGDWhen, or %NULL; unref with g_object_unref()
 *
 * Since: 0.2.0
 */
GDataGDWhen *
gdata_gd_when_new (gint64 start_time, gint64 end_time, gboolean is_date)
{
	g_return_val_if_fail (start_time >= 0, NULL);
	g_return_val_if_fail (end_time >= -1, NULL);

	return g_object_new (GDATA_TYPE_GD_WHEN, "start-time", start_time, "end-time", end_time, "is-date", is_date, NULL);
}

/**
 * gdata_gd_when_get_start_time:
 * @self: a #GDataGDWhen
 *
 * Gets the #GDataGDWhen:start-time property.
 *
 * Return value: the UNIX timestamp for the start time of the event
 *
 * Since: 0.4.0
 */
gint64
gdata_gd_when_get_start_time (GDataGDWhen *self)
{
	g_return_val_if_fail (GDATA_IS_GD_WHEN (self), -1);
	return self->priv->start_time;
}

/**
 * gdata_gd_when_set_start_time:
 * @self: a #GDataGDWhen
 * @start_time: the new start time
 *
 * Sets the #GDataGDWhen:start-time property to @start_time.
 *
 * Since: 0.4.0
 */
void
gdata_gd_when_set_start_time (GDataGDWhen *self, gint64 start_time)
{
	g_return_if_fail (GDATA_IS_GD_WHEN (self));
	g_return_if_fail (start_time >= 0);

	self->priv->start_time = start_time;
	g_object_notify (G_OBJECT (self), "start-time");
}

/**
 * gdata_gd_when_get_end_time:
 * @self: a #GDataGDWhen
 *
 * Gets the #GDataGDWhen:end-time property.
 *
 * If the end time is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the UNIX timestamp for the end time of the event, or
 * <code class="literal">-1</code>
 *
 * Since: 0.4.0
 */
gint64
gdata_gd_when_get_end_time (GDataGDWhen *self)
{
	g_return_val_if_fail (GDATA_IS_GD_WHEN (self), -1);
	return self->priv->end_time;
}

/**
 * gdata_gd_when_set_end_time:
 * @self: a #GDataGDWhen
 * @end_time: the new end time, or <code class="literal">-1</code>
 *
 * Sets the #GDataGDWhen:end-time property to @end_time.
 *
 * Set @end_time to <code class="literal">-1</code> to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_gd_when_set_end_time (GDataGDWhen *self, gint64 end_time)
{
	g_return_if_fail (GDATA_IS_GD_WHEN (self));
	g_return_if_fail (end_time >= -1);

	self->priv->end_time = end_time;
	g_object_notify (G_OBJECT (self), "end-time");
}

/**
 * gdata_gd_when_is_date:
 * @self: a #GDataGDWhen
 *
 * Gets the #GDataGDWhen:is-date property.
 *
 * Return value: %TRUE if #GDataGDWhen:start-time and #GDataGDWhen:end-time are dates rather than times, %FALSE otherwise
 *
 * Since: 0.4.0
 */
gboolean
gdata_gd_when_is_date (GDataGDWhen *self)
{
	g_return_val_if_fail (GDATA_IS_GD_WHEN (self), FALSE);
	return self->priv->is_date;
}

/**
 * gdata_gd_when_set_is_date:
 * @self: a #GDataGDWhen
 * @is_date: %TRUE if #GDataGDWhen:start-time and #GDataGDWhen:end-time should be dates rather than times, %FALSE otherwise
 *
 * Sets the #GDataGDWhen:is-date property to @is_date.
 *
 * Since: 0.4.0
 */
void
gdata_gd_when_set_is_date (GDataGDWhen *self, gboolean is_date)
{
	g_return_if_fail (GDATA_IS_GD_WHEN (self));

	self->priv->is_date = is_date;
	g_object_notify (G_OBJECT (self), "is-date");
}

/**
 * gdata_gd_when_get_value_string:
 * @self: a #GDataGDWhen
 *
 * Gets the #GDataGDWhen:value-string property.
 *
 * Return value: the value string, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_gd_when_get_value_string (GDataGDWhen *self)
{
	g_return_val_if_fail (GDATA_IS_GD_WHEN (self), NULL);
	return self->priv->value_string;
}

/**
 * gdata_gd_when_set_value_string:
 * @self: a #GDataGDWhen
 * @value_string: (allow-none): the new value string, or %NULL
 *
 * Sets the #GDataGDWhen:value-string property to @value_string.
 *
 * Set @value_string to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_gd_when_set_value_string (GDataGDWhen *self, const gchar *value_string)
{
	g_return_if_fail (GDATA_IS_GD_WHEN (self));

	g_free (self->priv->value_string);
	self->priv->value_string = g_strdup (value_string);
	g_object_notify (G_OBJECT (self), "value-string");
}

/**
 * gdata_gd_when_get_reminders:
 * @self: a #GDataGDWhen
 *
 * Returns a list of the #GDataGDReminders which are associated with this #GDataGDWhen.
 *
 * Return value: (element-type GData.GDReminder) (transfer none): a #GList of #GDataGDReminders, or %NULL
 *
 * Since: 0.4.0
 */
GList *
gdata_gd_when_get_reminders (GDataGDWhen *self)
{
	g_return_val_if_fail (GDATA_IS_GD_WHEN (self), NULL);
	return self->priv->reminders;
}

/**
 * gdata_gd_when_add_reminder:
 * @self: a #GDataGDWhen
 * @reminder: a #GDataGDReminder to add
 *
 * Adds a reminder to the #GDataGDWhen's list of reminders and increments its reference count.
 *
 * Duplicate reminders will not be added to the list.
 *
 * Since: 0.7.0
 */
void
gdata_gd_when_add_reminder (GDataGDWhen *self, GDataGDReminder *reminder)
{
	g_return_if_fail (GDATA_IS_GD_WHEN (self));
	g_return_if_fail (GDATA_IS_GD_REMINDER (reminder));

	if (g_list_find_custom (self->priv->reminders, reminder, (GCompareFunc) gdata_comparable_compare) == NULL)
		self->priv->reminders = g_list_append (self->priv->reminders, g_object_ref (reminder));
}
