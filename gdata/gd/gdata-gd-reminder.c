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
 * SECTION:gdata-gd-reminder
 * @short_description: GData reminder element
 * @stability: Stable
 * @include: gdata/gd/gdata-gd-reminder.h
 *
 * #GDataGDReminder represents a "reminder" element from the
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdReminder">GData specification</ulink>.
 *
 * Since: 0.4.0
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-gd-reminder.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "gdata-comparable.h"

static void gdata_gd_reminder_comparable_init (GDataComparableIface *iface);
static void gdata_gd_reminder_finalize (GObject *object);
static void gdata_gd_reminder_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_gd_reminder_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataGDReminderPrivate {
	gchar *method;
	gint64 absolute_time;
	gint relative_time;
};

enum {
	PROP_METHOD = 1,
	PROP_ABSOLUTE_TIME,
	PROP_IS_ABSOLUTE_TIME,
	PROP_RELATIVE_TIME
};

G_DEFINE_TYPE_WITH_CODE (GDataGDReminder, gdata_gd_reminder, GDATA_TYPE_PARSABLE,
                         G_ADD_PRIVATE (GDataGDReminder)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMPARABLE, gdata_gd_reminder_comparable_init))

static void
gdata_gd_reminder_class_init (GDataGDReminderClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_gd_reminder_get_property;
	gobject_class->set_property = gdata_gd_reminder_set_property;
	gobject_class->finalize = gdata_gd_reminder_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->pre_get_xml = pre_get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "reminder";
	parsable_class->element_namespace = "gd";

	/**
	 * GDataGDReminder:method:
	 *
	 * The notification method the reminder should use. For example: %GDATA_GD_REMINDER_ALERT or %GDATA_GD_REMINDER_EMAIL.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdReminder">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_METHOD,
	                                 g_param_spec_string ("method",
	                                                      "Method", "The notification method the reminder should use.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDReminder:absolute-time:
	 *
	 * Absolute time at which the reminder should be issued.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdReminder">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_ABSOLUTE_TIME,
	                                 g_param_spec_int64 ("absolute-time",
	                                                     "Absolute time", "Absolute time at which the reminder should be issued.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDReminder:is-absolute-time:
	 *
	 * Whether the reminder is specified as an absolute or relative time.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdReminder">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_IS_ABSOLUTE_TIME,
	                                 g_param_spec_boolean ("is-absolute-time",
	                                                       "Absolute time?", "Whether the reminder is specified as an absolute or relative time.",
	                                                       FALSE,
	                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDReminder:relative-time:
	 *
	 * Time at which the reminder should be issued, in minutes relative to the start time of the corresponding event.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdReminder">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_RELATIVE_TIME,
	                                 g_param_spec_int ("relative-time",
	                                                   "Relative time", "Time at which the reminder should be issued, in minutes.",
	                                                   -1, G_MAXINT, -1,
	                                                   G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static gint
compare_with (GDataComparable *self, GDataComparable *other)
{
	gint method_cmp, time_cmp;
	GDataGDReminder *a = (GDataGDReminder*) self, *b = (GDataGDReminder*) other;

	if (gdata_gd_reminder_is_absolute_time (a) != gdata_gd_reminder_is_absolute_time (b))
		return 1;

	method_cmp = g_strcmp0 (a->priv->method, b->priv->method);
	if (gdata_gd_reminder_is_absolute_time (a) == TRUE) {
		time_cmp = a->priv->absolute_time - b->priv->absolute_time;
	} else {
		time_cmp = a->priv->relative_time - b->priv->relative_time;
	}

	if (method_cmp == 0)
		return time_cmp;
	else
		return method_cmp;
}

static void
gdata_gd_reminder_comparable_init (GDataComparableIface *iface)
{
	iface->compare_with = compare_with;
}

static void
gdata_gd_reminder_init (GDataGDReminder *self)
{
	self->priv = gdata_gd_reminder_get_instance_private (self);
	self->priv->absolute_time = -1;
}

static void
gdata_gd_reminder_finalize (GObject *object)
{
	GDataGDReminderPrivate *priv = GDATA_GD_REMINDER (object)->priv;

	g_free (priv->method);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gd_reminder_parent_class)->finalize (object);
}

static void
gdata_gd_reminder_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataGDReminderPrivate *priv = GDATA_GD_REMINDER (object)->priv;

	switch (property_id) {
		case PROP_METHOD:
			g_value_set_string (value, priv->method);
			break;
		case PROP_ABSOLUTE_TIME:
			g_value_set_int64 (value, priv->absolute_time);
			break;
		case PROP_IS_ABSOLUTE_TIME:
			g_value_set_boolean (value, gdata_gd_reminder_is_absolute_time (GDATA_GD_REMINDER (object)));
			break;
		case PROP_RELATIVE_TIME:
			g_value_set_int (value, priv->relative_time);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_gd_reminder_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataGDReminder *self = GDATA_GD_REMINDER (object);

	switch (property_id) {
		case PROP_METHOD:
			gdata_gd_reminder_set_method (self, g_value_get_string (value));
			break;
		case PROP_ABSOLUTE_TIME:
			gdata_gd_reminder_set_absolute_time (self, g_value_get_int64 (value));
			break;
		case PROP_RELATIVE_TIME:
			gdata_gd_reminder_set_relative_time (self, g_value_get_int (value));
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
	GDataGDReminderPrivate *priv = GDATA_GD_REMINDER (parsable)->priv;
	xmlChar *absolute_time, *relative_time;
	gint64 absolute_time_int64;
	gint relative_time_int = -1;
	gboolean is_absolute_time = FALSE;

	/* Absolute time */
	absolute_time = xmlGetProp (root_node, (xmlChar*) "absoluteTime");
	if (absolute_time != NULL) {
		is_absolute_time = TRUE;
		if (gdata_parser_int64_from_iso8601 ((gchar*) absolute_time, &absolute_time_int64) == FALSE) {
			/* Error */
			gdata_parser_error_not_iso8601_format (root_node, (gchar*) absolute_time, error);
			xmlFree (absolute_time);
			return FALSE;
		}
		xmlFree (absolute_time);
	}

	/* Relative time */
	relative_time = xmlGetProp (root_node, (xmlChar*) "days");
	if (relative_time != NULL) {
		relative_time_int = g_ascii_strtoll ((gchar*) relative_time, NULL, 10) * 60 * 24;
	} else {
		relative_time = xmlGetProp (root_node, (xmlChar*) "hours");
		if (relative_time != NULL) {
			relative_time_int = g_ascii_strtoll ((gchar*) relative_time, NULL, 10) * 60;
		} else {
			relative_time = xmlGetProp (root_node, (xmlChar*) "minutes");
			if (relative_time != NULL)
				relative_time_int = g_ascii_strtoll ((gchar*) relative_time, NULL, 10);
		}
	}
	xmlFree (relative_time);

	if (is_absolute_time == TRUE) {
		priv->absolute_time = absolute_time_int64;
		priv->relative_time = -1;
	} else {
		priv->absolute_time = -1;
		priv->relative_time = relative_time_int;
	}

	priv->method = (gchar*) xmlGetProp (root_node, (xmlChar*) "method");

	return TRUE;
}

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGDReminderPrivate *priv = GDATA_GD_REMINDER (parsable)->priv;

	if (priv->relative_time == -1) {
		gchar *absolute_time = gdata_parser_int64_to_iso8601 (priv->absolute_time);
		g_string_append_printf (xml_string, " absoluteTime='%s'", absolute_time);
		g_free (absolute_time);
	} else {
		g_string_append_printf (xml_string, " minutes='%i'", priv->relative_time);
	}

	if (priv->method != NULL)
		gdata_parser_string_append_escaped (xml_string, " method='", priv->method, "'");
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "gd", (gchar*) "http://schemas.google.com/g/2005");
}

/**
 * gdata_gd_reminder_new:
 * @method: (allow-none): the notification method the reminder should use, or %NULL
 * @absolute_time: the absolute time for the reminder, or <code class="literal">-1</code>
 * @relative_time: the relative time for the reminder, in minutes, or <code class="literal">-1</code>
 *
 * Creates a new #GDataGDReminder. More information is available in the <ulink type="http"
 * url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdReminder">GData specification</ulink>.
 *
 * Return value: a new #GDataGDReminder, or %NULL; unref with g_object_unref()
 *
 * Since: 0.2.0
 */
GDataGDReminder *
gdata_gd_reminder_new (const gchar *method, gint64 absolute_time, gint relative_time)
{
	g_return_val_if_fail (absolute_time == -1 || relative_time == -1, NULL);
	g_return_val_if_fail (absolute_time >= -1, NULL);
	g_return_val_if_fail (relative_time >= -1, NULL);
	return g_object_new (GDATA_TYPE_GD_REMINDER, "absolute-time", absolute_time, "relative-time", relative_time, "method", method, NULL);
}

/**
 * gdata_gd_reminder_get_method:
 * @self: a #GDataGDReminder
 *
 * Gets the #GDataGDReminder:method property.
 *
 * Return value: the method, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_gd_reminder_get_method (GDataGDReminder *self)
{
	g_return_val_if_fail (GDATA_IS_GD_REMINDER (self), NULL);
	return self->priv->method;
}

/**
 * gdata_gd_reminder_set_method:
 * @self: a #GDataGDReminder
 * @method: (allow-none): the new method, or %NULL
 *
 * Sets the #GDataGDReminder:method property to @method.
 *
 * Set @method to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_gd_reminder_set_method (GDataGDReminder *self, const gchar *method)
{
	g_return_if_fail (GDATA_IS_GD_REMINDER (self));

	g_free (self->priv->method);
	self->priv->method = g_strdup (method);
	g_object_notify (G_OBJECT (self), "method");
}

/**
 * gdata_gd_reminder_get_absolute_time:
 * @self: a #GDataGDReminder
 *
 * Gets the #GDataGDReminder:absolute-time property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the UNIX timestamp of the absolute time for the reminder, or <code class="literal">-1</code>
 *
 * Since: 0.4.0
 */
gint64
gdata_gd_reminder_get_absolute_time (GDataGDReminder *self)
{
	g_return_val_if_fail (GDATA_IS_GD_REMINDER (self), -1);
	return self->priv->absolute_time;
}

/**
 * gdata_gd_reminder_set_absolute_time:
 * @self: a #GDataGDReminder
 * @absolute_time: the new absolute time, or <code class="literal">-1</code>
 *
 * Sets the #GDataGDReminder:absolute-time property to @absolute_time.
 *
 * Set @absolute_time to <code class="literal">-1</code> to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_gd_reminder_set_absolute_time (GDataGDReminder *self, gint64 absolute_time)
{
	g_return_if_fail (GDATA_IS_GD_REMINDER (self));
	g_return_if_fail (absolute_time >= -1);

	self->priv->absolute_time = absolute_time;
	g_object_notify (G_OBJECT (self), "absolute-time");
}

/**
 * gdata_gd_reminder_is_absolute_time:
 * @self: a #GDataGDReminder
 *
 * Returns whether the reminder is specified as an absolute time, or as a number of minutes after
 * the corresponding event's start time.
 *
 * Return value: %TRUE if the reminder is absolute, %FALSE otherwise
 *
 * Since: 0.4.0
 */
gboolean
gdata_gd_reminder_is_absolute_time (GDataGDReminder *self)
{
	g_return_val_if_fail (GDATA_IS_GD_REMINDER (self), FALSE);
	return (self->priv->relative_time == -1) ? TRUE : FALSE;
}

/**
 * gdata_gd_reminder_get_relative_time:
 * @self: a #GDataGDReminder
 *
 * Gets the #GDataGDReminder:relative-time property.
 *
 * Return value: the relative time, or <code class="literal">-1</code>
 *
 * Since: 0.4.0
 */
gint
gdata_gd_reminder_get_relative_time (GDataGDReminder *self)
{
	g_return_val_if_fail (GDATA_IS_GD_REMINDER (self), -1);
	return self->priv->relative_time;
}

/**
 * gdata_gd_reminder_set_relative_time:
 * @self: a #GDataGDReminder
 * @relative_time: the new relative time, or <code class="literal">-1</code>
 *
 * Sets the #GDataGDReminder:relative-time property to @relative_time.
 *
 * Set @relative_time to <code class="literal">-1</code> to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_gd_reminder_set_relative_time (GDataGDReminder *self, gint relative_time)
{
	g_return_if_fail (GDATA_IS_GD_REMINDER (self));
	g_return_if_fail (relative_time >= -1);

	self->priv->relative_time = relative_time;
	g_object_notify (G_OBJECT (self), "method");
}
