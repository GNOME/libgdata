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
 * SECTION:gdata-calendar-calendar
 * @short_description: GData Calendar calendar object
 * @stability: Unstable
 * @include: gdata/services/calendar/gdata-calendar-calendar.h
 *
 * #GDataCalendarCalendar is a subclass of #GDataEntry to represent a calendar from Google Calendar.
 *
 * #GDataCalendarCalendar implements #GDataAccessHandler, meaning the access rules to it can be modified using that interface. As well as the
 * access roles defined for the base #GDataAccessRule (e.g. %GDATA_ACCESS_ROLE_NONE), #GDataCalendarCalendar has its own, such as
 * %GDATA_CALENDAR_ACCESS_ROLE_EDITOR and %GDATA_CALENDAR_ACCESS_ROLE_FREE_BUSY.
 *
 * For more details of Google Calendar's GData API, see the <ulink type="http" url="http://code.google.com/apis/calendar/docs/2.0/reference.html">
 * online documentation</ulink>.
 *
 * <example>
 * 	<title>Listing Calendars</title>
 * 	<programlisting>
 *	GDataCalendarService *service;
 *	GDataFeed *feed;
 *	GList *i;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service *<!-- -->/
 *	service = create_calendar_service ();
 *
 *	/<!-- -->* Query for all of the calendars the currently authenticated user has access to, including those which they have read-only
 *	 * access to. *<!-- -->/
 *	feed = gdata_calendar_service_query_all_calendars (service, NULL, NULL, NULL, NULL, &error);
 *
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error querying for calendars: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	/<!-- -->* Iterate through the returned calendars and do something with them *<!-- -->/
 *	for (i = gdata_feed_get_entries (feed); i != NULL; i = i->next) {
 *		const gchar *access_level;
 *		gboolean has_write_access;
 *		GDataCalendarCalendar *calendar = GDATA_CALENDAR_CALENDAR (i->data);
 *
 *		/<!-- -->* Determine whether we have write access to the calendar, or just read-only or free/busy access. Note that the access levels
 *		 * are more detailed than this; see the documentation for gdata_calendar_calendar_get_access_level() for more information. *<!-- -->/
 *		access_level = gdata_calendar_calendar_get_access_level (calendar);
 *		has_write_access = (access_level != NULL && strcmp (access_level, GDATA_CALENDAR_ACCESS_ROLE_EDITOR) == 0) ? TRUE : FALSE;
 *
 *		/<!-- -->* Do something with the calendar here, such as insert it into a UI *<!-- -->/
 *	}
 *
 *	g_object_unref (feed);
 * 	</programlisting>
 * </example>
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-calendar-calendar.h"
#include "gdata-private.h"
#include "gdata-service.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "gdata-access-handler.h"
#include "gdata-calendar-service.h"

static void gdata_calendar_calendar_access_handler_init (GDataAccessHandlerIface *iface);
static GObject *gdata_calendar_calendar_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params);
static void gdata_calendar_calendar_finalize (GObject *object);
static void gdata_calendar_calendar_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_calendar_calendar_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataCalendarCalendarPrivate {
	gchar *timezone;
	guint times_cleaned;
	gboolean is_hidden;
	GDataColor colour;
	gboolean is_selected;
	gchar *access_level;

	gint64 edited;
};

enum {
	PROP_TIMEZONE = 1,
	PROP_TIMES_CLEANED,
	PROP_IS_HIDDEN,
	PROP_COLOR,
	PROP_IS_SELECTED,
	PROP_ACCESS_LEVEL,
	PROP_EDITED,
	PROP_ETAG,
};

G_DEFINE_TYPE_WITH_CODE (GDataCalendarCalendar, gdata_calendar_calendar, GDATA_TYPE_ENTRY,
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_ACCESS_HANDLER, gdata_calendar_calendar_access_handler_init))

static void
gdata_calendar_calendar_class_init (GDataCalendarCalendarClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataCalendarCalendarPrivate));

	gobject_class->constructor = gdata_calendar_calendar_constructor;
	gobject_class->set_property = gdata_calendar_calendar_set_property;
	gobject_class->get_property = gdata_calendar_calendar_get_property;
	gobject_class->finalize = gdata_calendar_calendar_finalize;

	parsable_class->parse_xml = parse_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;

	entry_class->kind_term = "http://schemas.google.com/gCal/2005#calendarmeta";

	/**
	 * GDataCalendarCalendar:timezone:
	 *
	 * The timezone in which the calendar's times are given. This is a timezone name in tz database notation: <ulink type="http"
	 * url="http://en.wikipedia.org/wiki/Tz_database#Names_of_time_zones">reference</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_TIMEZONE,
	                                 g_param_spec_string ("timezone",
	                                                      "Timezone", "The timezone in which the calendar's times are given.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarCalendar:times-cleaned:
	 *
	 * The number of times the calendar has been cleared of events.
	 **/
	g_object_class_install_property (gobject_class, PROP_TIMES_CLEANED,
	                                 g_param_spec_uint ("times-cleaned",
	                                                    "Times cleaned", "The number of times the calendar has been cleared of events.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarCalendar:is-hidden:
	 *
	 * Indicates whether the calendar is visible.
	 *
	 * Since: 0.2.0
	 **/
	g_object_class_install_property (gobject_class, PROP_IS_HIDDEN,
	                                 g_param_spec_boolean ("is-hidden",
	                                                       "Hidden?", "Indicates whether the calendar is visible.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarCalendar:color:
	 *
	 * The color used to highlight the calendar in the user's browser. This must be one of a limited set of colors listed in the
	 * <ulink type="http" url="http://code.google.com/apis/calendar/data/2.0/reference.html#gCalcolor">online documentation</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_COLOR,
	                                 g_param_spec_boxed ("color",
	                                                     "Color", "The color used to highlight the calendar in the user's browser.",
	                                                     GDATA_TYPE_COLOR,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarCalendar:is-selected:
	 *
	 * Indicates whether the calendar is selected.
	 *
	 * Since: 0.2.0
	 **/
	g_object_class_install_property (gobject_class, PROP_IS_SELECTED,
	                                 g_param_spec_boolean ("is-selected",
	                                                       "Selected?", "Indicates whether the calendar is selected.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarCalendar:access-level:
	 *
	 * Indicates the access level the current user has to the calendar. For example: %GDATA_CALENDAR_ACCESS_ROLE_READ or
	 * %GDATA_CALENDAR_ACCESS_ROLE_FREE_BUSY. The "current user" is the one authenticated against the service's #GDataService:authorizer,
	 * or the guest user.
	 **/
	g_object_class_install_property (gobject_class, PROP_ACCESS_LEVEL,
	                                 g_param_spec_string ("access-level",
	                                                      "Access level", "Indicates the access level the current user has to the calendar.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarCalendar:edited:
	 *
	 * The last time the calendar was edited. If the calendar has not been edited yet, the content indicates the time it was created.
	 *
	 * For more information, see the <ulink type="http" url="http://www.atomenabled.org/developers/protocol/#appEdited">
	 * Atom Publishing Protocol specification</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_EDITED,
	                                 g_param_spec_int64 ("edited",
	                                                     "Edited", "The last time the calendar was edited.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/* Override the ETag property since ETags don't seem to be supported for calendars. */
	g_object_class_override_property (gobject_class, PROP_ETAG, "etag");
}

static gboolean
is_owner_rule (GDataAccessRule *rule)
{
	return (strcmp (gdata_access_rule_get_role (rule), GDATA_CALENDAR_ACCESS_ROLE_OWNER) == 0) ? TRUE : FALSE;
}

static GDataAuthorizationDomain *
get_authorization_domain (GDataAccessHandler *self)
{
	return gdata_calendar_service_get_primary_authorization_domain ();
}

static void
gdata_calendar_calendar_access_handler_init (GDataAccessHandlerIface *iface)
{
	iface->is_owner_rule = is_owner_rule;
	iface->get_authorization_domain = get_authorization_domain;
}

static void
gdata_calendar_calendar_init (GDataCalendarCalendar *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_CALENDAR_CALENDAR, GDataCalendarCalendarPrivate);
	self->priv->edited = -1;
}

static GObject *
gdata_calendar_calendar_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
	GObject *object;

	/* Chain up to the parent class */
	object = G_OBJECT_CLASS (gdata_calendar_calendar_parent_class)->constructor (type, n_construct_params, construct_params);

	if (_gdata_parsable_is_constructed_from_xml (GDATA_PARSABLE (object)) == FALSE) {
		GDataCalendarCalendarPrivate *priv = GDATA_CALENDAR_CALENDAR (object)->priv;
		GTimeVal time_val;

		/* Set the edited property to the current time (creation time). We don't do this in *_init() since that would cause
		 * setting it from parse_xml() to fail (duplicate element). */
		g_get_current_time (&time_val);
		priv->edited = time_val.tv_sec;
	}

	return object;
}

static void
gdata_calendar_calendar_finalize (GObject *object)
{
	GDataCalendarCalendarPrivate *priv = GDATA_CALENDAR_CALENDAR (object)->priv;

	g_free (priv->timezone);
	g_free (priv->access_level);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_calendar_calendar_parent_class)->finalize (object);
}

static void
gdata_calendar_calendar_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataCalendarCalendarPrivate *priv = GDATA_CALENDAR_CALENDAR (object)->priv;

	switch (property_id) {
		case PROP_TIMEZONE:
			g_value_set_string (value, priv->timezone);
			break;
		case PROP_TIMES_CLEANED:
			g_value_set_uint (value, priv->times_cleaned);
			break;
		case PROP_IS_HIDDEN:
			g_value_set_boolean (value, priv->is_hidden);
			break;
		case PROP_COLOR:
			g_value_set_boxed (value, &(priv->colour));
			break;
		case PROP_IS_SELECTED:
			g_value_set_boolean (value, priv->is_selected);
			break;
		case PROP_ACCESS_LEVEL:
			g_value_set_string (value, priv->access_level);
			break;
		case PROP_EDITED:
			g_value_set_int64 (value, priv->edited);
			break;
		case PROP_ETAG:
			/* Never return an ETag */
			g_value_set_string (value, NULL);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_calendar_calendar_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataCalendarCalendar *self = GDATA_CALENDAR_CALENDAR (object);

	switch (property_id) {
		case PROP_TIMEZONE:
			gdata_calendar_calendar_set_timezone (self, g_value_get_string (value));
			break;
		case PROP_IS_HIDDEN:
			gdata_calendar_calendar_set_is_hidden (self, g_value_get_boolean (value));
			break;
		case PROP_COLOR:
			gdata_calendar_calendar_set_color (self, g_value_get_boxed (value));
			break;
		case PROP_IS_SELECTED:
			gdata_calendar_calendar_set_is_selected (self, g_value_get_boolean (value));
			break;
		case PROP_ETAG:
			/* Never set an ETag (note that this doesn't stop it being set in GDataEntry due to XML parsing) */
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	gboolean success;
	GDataCalendarCalendar *self = GDATA_CALENDAR_CALENDAR (parsable);

	if (gdata_parser_is_namespace (node, "http://www.w3.org/2007/app") == TRUE &&
	    gdata_parser_int64_time_from_element (node, "edited", P_REQUIRED | P_NO_DUPES, &(self->priv->edited), &success, error) == TRUE) {
		return success;
	} else if (gdata_parser_is_namespace (node, "http://schemas.google.com/gCal/2005") == TRUE) {
		if (xmlStrcmp (node->name, (xmlChar*) "timezone") == 0) {
			/* gCal:timezone */
			xmlChar *_timezone = xmlGetProp (node, (xmlChar*) "value");
			if (_timezone == NULL)
				return gdata_parser_error_required_property_missing (node, "value", error);
			self->priv->timezone = (gchar*) _timezone;
		} else if (xmlStrcmp (node->name, (xmlChar*) "timesCleaned") == 0) {
			/* gCal:timesCleaned */
			xmlChar *times_cleaned = xmlGetProp (node, (xmlChar*) "value");
			if (times_cleaned == NULL)
				return gdata_parser_error_required_property_missing (node, "value", error);
			self->priv->times_cleaned = strtoul ((gchar*) times_cleaned, NULL, 10);
			xmlFree (times_cleaned);
		} else if (xmlStrcmp (node->name, (xmlChar*) "hidden") == 0) {
			/* gCal:hidden */
			if (gdata_parser_boolean_from_property (node, "value", &(self->priv->is_hidden), -1, error) == FALSE)
				return FALSE;
		} else if (xmlStrcmp (node->name, (xmlChar*) "color") == 0) {
			/* gCal:color */
			xmlChar *value;
			GDataColor colour;

			value = xmlGetProp (node, (xmlChar*) "value");
			if (value == NULL)
				return gdata_parser_error_required_property_missing (node, "value", error);
			if (gdata_color_from_hexadecimal ((gchar*) value, &colour) == FALSE) {
				/* Error */
				g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
				             /* Translators: the first parameter is the name of an XML element (including the angle brackets
				              * ("<" and ">"), and the second parameter is the erroneous value (which was not in hexadecimal
				              * RGB format).
				              *
				              * For example:
				              *  The content of a <entry/gCal:color> element ("00FG56") was not in hexadecimal RGB format. */
				             _("The content of a %s element (\"%s\") was not in hexadecimal RGB format."),
				             "<entry/gCal:color>", value);
				xmlFree (value);

				return FALSE;
			}

			gdata_calendar_calendar_set_color (self, &colour);
			xmlFree (value);
		} else if (xmlStrcmp (node->name, (xmlChar*) "selected") == 0) {
			/* gCal:selected */
			if (gdata_parser_boolean_from_property (node, "value", &(self->priv->is_selected), -1, error) == FALSE)
				return FALSE;
		} else if (xmlStrcmp (node->name, (xmlChar*) "accesslevel") == 0) {
			/* gCal:accesslevel */
			self->priv->access_level = (gchar*) xmlGetProp (node, (xmlChar*) "value");
			if (self->priv->access_level == NULL)
				return gdata_parser_error_required_property_missing (node, "value", error);
		} else {
			return GDATA_PARSABLE_CLASS (gdata_calendar_calendar_parent_class)->parse_xml (parsable, doc, node, user_data, error);
		}
	} else {
		return GDATA_PARSABLE_CLASS (gdata_calendar_calendar_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}

	return TRUE;
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	gchar *colour;
	GDataCalendarCalendarPrivate *priv = GDATA_CALENDAR_CALENDAR (parsable)->priv;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_calendar_calendar_parent_class)->get_xml (parsable, xml_string);

	/* Add all the Calendar-specific XML */
	if (priv->timezone != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gCal:timezone value='", priv->timezone, "'/>");

	if (priv->is_hidden == TRUE)
		g_string_append (xml_string, "<gCal:hidden value='true'/>");
	else
		g_string_append (xml_string, "<gCal:hidden value='false'/>");

	colour = gdata_color_to_hexadecimal (&(priv->colour));
	g_string_append_printf (xml_string, "<gCal:color value='%s'/>", colour);
	g_free (colour);

	if (priv->is_selected == TRUE)
		g_string_append (xml_string, "<gCal:selected value='true'/>");
	else
		g_string_append (xml_string, "<gCal:selected value='false'/>");
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_calendar_calendar_parent_class)->get_namespaces (parsable, namespaces);

	g_hash_table_insert (namespaces, (gchar*) "gCal", (gchar*) "http://schemas.google.com/gCal/2005");
	g_hash_table_insert (namespaces, (gchar*) "app", (gchar*) "http://www.w3.org/2007/app");
}

/**
 * gdata_calendar_calendar_new:
 * @id: (allow-none): the calendar's ID, or %NULL
 *
 * Creates a new #GDataCalendarCalendar with the given ID and default properties.
 *
 * Return value: a new #GDataCalendarCalendar; unref with g_object_unref()
 **/
GDataCalendarCalendar *
gdata_calendar_calendar_new (const gchar *id)
{
	return GDATA_CALENDAR_CALENDAR (g_object_new (GDATA_TYPE_CALENDAR_CALENDAR, "id", id, NULL));
}

/**
 * gdata_calendar_calendar_get_timezone:
 * @self: a #GDataCalendarCalendar
 *
 * Gets the #GDataCalendarCalendar:timezone property.
 *
 * Return value: the calendar's timezone, or %NULL
 **/
const gchar *
gdata_calendar_calendar_get_timezone (GDataCalendarCalendar *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_CALENDAR (self), NULL);
	return self->priv->timezone;
}

/**
 * gdata_calendar_calendar_set_timezone:
 * @self: a #GDataCalendarCalendar
 * @_timezone: (allow-none): a new timezone, or %NULL
 *
 * Sets the #GDataCalendarCalendar:timezone property to the new timezone, @_timezone.
 *
 * Set @_timezone to %NULL to unset the property in the calendar.
 **/
void
gdata_calendar_calendar_set_timezone (GDataCalendarCalendar *self, const gchar *_timezone)
{
	/* Blame "timezone" in /usr/include/time.h:291 for the weird parameter naming */
	g_return_if_fail (GDATA_IS_CALENDAR_CALENDAR (self));

	g_free (self->priv->timezone);
	self->priv->timezone = g_strdup (_timezone);
	g_object_notify (G_OBJECT (self), "timezone");
}

/**
 * gdata_calendar_calendar_get_times_cleaned:
 * @self: a #GDataCalendarCalendar
 *
 * Gets the #GDataCalendarCalendar:times-cleaned property.
 *
 * Return value: the number of times the calendar has been totally emptied
 **/
guint
gdata_calendar_calendar_get_times_cleaned (GDataCalendarCalendar *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_CALENDAR (self), 0);
	return self->priv->times_cleaned;
}

/**
 * gdata_calendar_calendar_is_hidden:
 * @self: a #GDataCalendarCalendar
 *
 * Gets the #GDataCalendarCalendar:is-hidden property.
 *
 * Return value: %TRUE if the calendar is hidden, %FALSE otherwise
 *
 * Since: 0.2.0
 **/
gboolean
gdata_calendar_calendar_is_hidden (GDataCalendarCalendar *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_CALENDAR (self), 0);
	return self->priv->is_hidden;
}

/**
 * gdata_calendar_calendar_set_is_hidden:
 * @self: a #GDataCalendarCalendar
 * @is_hidden: %TRUE to hide the calendar, %FALSE otherwise
 *
 * Sets the #GDataCalendarCalendar:is-hidden property to @is_hidden.
 *
 * Since: 0.2.0
 **/
void
gdata_calendar_calendar_set_is_hidden (GDataCalendarCalendar *self, gboolean is_hidden)
{
	g_return_if_fail (GDATA_IS_CALENDAR_CALENDAR (self));
	self->priv->is_hidden = is_hidden;
	g_object_notify (G_OBJECT (self), "is-hidden");
}

/**
 * gdata_calendar_calendar_get_color:
 * @self: a #GDataCalendarCalendar
 * @color: (out caller-allocates): a #GDataColor
 *
 * Gets the #GDataCalendarCalendar:color property and puts it in @color.
 **/
void
gdata_calendar_calendar_get_color (GDataCalendarCalendar *self, GDataColor *color)
{
	g_return_if_fail (GDATA_IS_CALENDAR_CALENDAR (self));
	g_return_if_fail (color != NULL);
	*color = self->priv->colour;
}

/**
 * gdata_calendar_calendar_set_color:
 * @self: a #GDataCalendarCalendar
 * @color: a new #GDataColor
 *
 * Sets the #GDataCalendarCalendar:color property to @color.
 **/
void
gdata_calendar_calendar_set_color (GDataCalendarCalendar *self, const GDataColor *color)
{
	g_return_if_fail (GDATA_IS_CALENDAR_CALENDAR (self));
	g_return_if_fail (color != NULL);
	self->priv->colour = *color;
	g_object_notify (G_OBJECT (self), "color");
}

/**
 * gdata_calendar_calendar_is_selected:
 * @self: a #GDataCalendarCalendar
 *
 * Gets the #GDataCalendarCalendar:is-selected property.
 *
 * Return value: %TRUE if the calendar is selected, %FALSE otherwise
 *
 * Since: 0.2.0
 **/
gboolean
gdata_calendar_calendar_is_selected (GDataCalendarCalendar *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_CALENDAR (self), 0);
	return self->priv->is_selected;
}

/**
 * gdata_calendar_calendar_set_is_selected:
 * @self: a #GDataCalendarCalendar
 * @is_selected: %TRUE to select the calendar, %FALSE otherwise
 *
 * Sets the #GDataCalendarCalendar:is-selected property to @is_selected.
 *
 * Since: 0.2.0
 **/
void
gdata_calendar_calendar_set_is_selected (GDataCalendarCalendar *self, gboolean is_selected)
{
	g_return_if_fail (GDATA_IS_CALENDAR_CALENDAR (self));
	self->priv->is_selected = is_selected;
	g_object_notify (G_OBJECT (self), "is-selected");
}

/**
 * gdata_calendar_calendar_get_access_level:
 * @self: a #GDataCalendarCalendar
 *
 * Gets the #GDataCalendarCalendar:access-level property.
 *
 * Return value: the authenticated user's access level to the calendar, or %NULL
 **/
const gchar *
gdata_calendar_calendar_get_access_level (GDataCalendarCalendar *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_CALENDAR (self), NULL);
	return self->priv->access_level;
}

/**
 * gdata_calendar_calendar_get_edited:
 * @self: a #GDataCalendarCalendar
 *
 * Gets the #GDataCalendarCalendar:edited property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the UNIX timestamp for the time the calendar was last edited, or <code class="literal">-1</code>
 **/
gint64
gdata_calendar_calendar_get_edited (GDataCalendarCalendar *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_CALENDAR (self), -1);
	return self->priv->edited;
}
