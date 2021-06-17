/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009, 2010, 2014, 2015 <philip@tecnocode.co.uk>
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
 * @stability: Stable
 * @include: gdata/services/calendar/gdata-calendar-calendar.h
 *
 * #GDataCalendarCalendar is a subclass of #GDataEntry to represent a calendar from Google Calendar.
 *
 * #GDataCalendarCalendar implements #GDataAccessHandler, meaning the access rules to it can be modified using that interface. As well as the
 * access roles defined for the base #GDataAccessRule (e.g. %GDATA_ACCESS_ROLE_NONE), #GDataCalendarCalendar has its own, such as
 * %GDATA_CALENDAR_ACCESS_ROLE_EDITOR and %GDATA_CALENDAR_ACCESS_ROLE_FREE_BUSY.
 *
 * For more details of Google Calendar's GData API, see the <ulink type="http" url="https://developers.google.com/google-apps/calendar/v3/reference/">
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
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-calendar-calendar.h"
#include "gdata-private.h"
#include "gdata-service.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "gdata-access-handler.h"
#include "gdata-calendar-service.h"
#include "gdata-calendar-access-rule.h"

static void gdata_calendar_calendar_access_handler_init (GDataAccessHandlerIface *iface);
static void gdata_calendar_calendar_finalize (GObject *object);
static void gdata_calendar_calendar_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_calendar_calendar_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void get_json (GDataParsable *parsable, JsonBuilder *builder);
static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);
static const gchar *get_content_type (void);

struct _GDataCalendarCalendarPrivate {
	gchar *timezone;
	gboolean is_hidden;
	GDataColor colour;
	gboolean is_selected;
	gchar *access_level;
};

enum {
	PROP_TIMEZONE = 1,
	PROP_IS_HIDDEN,
	PROP_COLOR,
	PROP_IS_SELECTED,
	PROP_ACCESS_LEVEL,
	PROP_ETAG,
};

G_DEFINE_TYPE_WITH_CODE (GDataCalendarCalendar, gdata_calendar_calendar, GDATA_TYPE_ENTRY,
                         G_ADD_PRIVATE (GDataCalendarCalendar)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_ACCESS_HANDLER, gdata_calendar_calendar_access_handler_init))

static void
gdata_calendar_calendar_class_init (GDataCalendarCalendarClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	gobject_class->set_property = gdata_calendar_calendar_set_property;
	gobject_class->get_property = gdata_calendar_calendar_get_property;
	gobject_class->finalize = gdata_calendar_calendar_finalize;

	parsable_class->parse_json = parse_json;
	parsable_class->get_json = get_json;
	parsable_class->get_content_type = get_content_type;

	entry_class->kind_term = "calendar#calendarListEntry";

	/**
	 * GDataCalendarCalendar:timezone:
	 *
	 * The timezone in which the calendar's times are given. This is a timezone name in tz database notation: <ulink type="http"
	 * url="http://en.wikipedia.org/wiki/Tz_database#Names_of_time_zones">reference</ulink>.
	 */
	g_object_class_install_property (gobject_class, PROP_TIMEZONE,
	                                 g_param_spec_string ("timezone",
	                                                      "Timezone", "The timezone in which the calendar's times are given.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarCalendar:is-hidden:
	 *
	 * Indicates whether the calendar is visible.
	 *
	 * Since: 0.2.0
	 */
	g_object_class_install_property (gobject_class, PROP_IS_HIDDEN,
	                                 g_param_spec_boolean ("is-hidden",
	                                                       "Hidden?", "Indicates whether the calendar is visible.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarCalendar:color:
	 *
	 * The background color used to highlight the calendar in the user’s
	 * browser. This used to be restricted to a limited set of colours, but
	 * since 0.17.2 may be any RGB colour.
	 */
	g_object_class_install_property (gobject_class, PROP_COLOR,
	                                 g_param_spec_boxed ("color",
	                                                     "Color", "The background color used to highlight the calendar in the user's browser.",
	                                                     GDATA_TYPE_COLOR,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarCalendar:is-selected:
	 *
	 * Indicates whether the calendar is selected.
	 *
	 * Since: 0.2.0
	 */
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
	 */
	g_object_class_install_property (gobject_class, PROP_ACCESS_LEVEL,
	                                 g_param_spec_string ("access-level",
	                                                      "Access level", "Indicates the access level the current user has to the calendar.",
	                                                      NULL,
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

static GDataFeed *
get_rules (GDataAccessHandler *self,
           GDataService *service,
           GCancellable *cancellable,
           GDataQueryProgressCallback progress_callback,
           gpointer progress_user_data,
           GError **error)
{
	GDataAccessHandlerIface *iface;
	GDataAuthorizationDomain *domain = NULL;
	GDataFeed *feed;
	GDataLink *_link;
	SoupMessage *message;
	GList/*<unowned GDataCalendarAccessRule>*/ *rules, *i;
	const gchar *calendar_id;

	_link = gdata_entry_look_up_link (GDATA_ENTRY (self),
	                                  GDATA_LINK_ACCESS_CONTROL_LIST);
	g_assert (_link != NULL);

	iface = GDATA_ACCESS_HANDLER_GET_IFACE (self);
	if (iface->get_authorization_domain != NULL) {
		domain = iface->get_authorization_domain (self);
	}

	message = _gdata_service_query (service, domain,
	                                gdata_link_get_uri (_link), NULL,
	                                cancellable, error);
	if (message == NULL) {
		return NULL;
	}

	g_assert (message->response_body->data != NULL);

	feed = _gdata_feed_new_from_json (GDATA_TYPE_FEED,
	                                  message->response_body->data,
	                                  message->response_body->length,
	                                  GDATA_TYPE_CALENDAR_ACCESS_RULE,
	                                  progress_callback, progress_user_data,
	                                  error);

	/* Set the self link on all the ACL rules so they can be deleted.
	 * Sigh. */
	rules = gdata_feed_get_entries (feed);
	calendar_id = gdata_entry_get_id (GDATA_ENTRY (self));

	for (i = rules; i != NULL; i = i->next) {
		const gchar *id;
		gchar *uri = NULL;  /* owned */

		/* Set the self link, which is needed for
		 * gdata_service_delete_entry(). Unfortunately, it needs the
		 * ACL ID _and_ the calendar ID. */
		id = gdata_entry_get_id (GDATA_ENTRY (i->data));

		if (id == NULL || calendar_id == NULL) {
			continue;
		}

		uri = g_strconcat ("https://www.googleapis.com"
		                   "/calendar/v3/calendars/",
		                   calendar_id, "/acl/", id, NULL);
		_link = gdata_link_new (uri, GDATA_LINK_SELF);
		gdata_entry_add_link (GDATA_ENTRY (i->data), _link);
		g_object_unref (_link);
		g_free (uri);
	}

	g_object_unref (message);

	return feed;
}

static void
gdata_calendar_calendar_access_handler_init (GDataAccessHandlerIface *iface)
{
	iface->is_owner_rule = is_owner_rule;
	iface->get_authorization_domain = get_authorization_domain;
	iface->get_rules = get_rules;
}

static void
gdata_calendar_calendar_init (GDataCalendarCalendar *self)
{
	self->priv = gdata_calendar_calendar_get_instance_private (self);
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
	GDataCalendarCalendar *self = GDATA_CALENDAR_CALENDAR (object);
	GDataCalendarCalendarPrivate *priv = self->priv;

	switch (property_id) {
		case PROP_TIMEZONE:
			g_value_set_string (value, priv->timezone);
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
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	gboolean success;
	GDataCalendarCalendar *self = GDATA_CALENDAR_CALENDAR (parsable);

	/* FIXME: Unimplemented:
	 *  - location
	 *  - summaryOverride
	 *  - colorId
	 *  - foregroundColor
	 *  - defaultReminders
	 *  - notificationSettings
	 *  - primary
	 *  - deleted
	 */

	if (gdata_parser_string_from_json_member (reader, "timeZone", P_DEFAULT, &self->priv->timezone, &success, error) ||
	    gdata_parser_color_from_json_member (reader, "backgroundColor", P_DEFAULT, &self->priv->colour, &success, error) ||
	    gdata_parser_boolean_from_json_member (reader, "hidden", P_DEFAULT, &self->priv->is_hidden, &success, error) ||
	    gdata_parser_boolean_from_json_member (reader, "selected", P_DEFAULT, &self->priv->is_selected, &success, error)) {
		return success;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "summary") == 0) {
		gchar *summary = NULL;

		g_assert (gdata_parser_string_from_json_member (reader,
		                                                "summary",
		                                                P_DEFAULT,
		                                                &summary,
		                                                &success,
		                                                error));

		if (summary != NULL) {
			gdata_entry_set_title (GDATA_ENTRY (parsable), summary);
		}

		g_free (summary);

		return success;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "description") == 0) {
		gchar *description = NULL;

		g_assert (gdata_parser_string_from_json_member (reader,
		                                                "description",
		                                                P_DEFAULT,
		                                                &description,
		                                                &success,
		                                                error));

		if (description != NULL) {
			gdata_entry_set_summary (GDATA_ENTRY (parsable),
			                         description);
		}

		g_free (description);

		return success;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "accessRole") == 0) {
		gchar *access_role = NULL;

		g_assert (gdata_parser_string_from_json_member (reader,
		                                                "accessRole",
		                                                P_DEFAULT,
		                                                &access_role,
		                                                &success,
		                                                error));

		if (access_role != NULL) {
			const gchar *level;

			/* Convert from v3 format to v2. */
			if (g_strcmp0 (access_role, "freeBusyReader") == 0) {
				level = GDATA_CALENDAR_ACCESS_ROLE_FREE_BUSY;
			} else if (g_strcmp0 (access_role, "reader") == 0) {
				level = GDATA_CALENDAR_ACCESS_ROLE_READ;
			} else if (g_strcmp0 (access_role, "writer") == 0) {
				level = GDATA_CALENDAR_ACCESS_ROLE_EDITOR;
			} else if (g_strcmp0 (access_role, "owner") == 0) {
				level = GDATA_CALENDAR_ACCESS_ROLE_OWNER;
			} else {
				level = access_role;
			}

			self->priv->access_level = g_strdup (level);
		}

		g_free (access_role);

		return success;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "id") == 0) {
		GDataLink *_link;
		const gchar *id;
		gchar *uri;

		id = json_reader_get_string_value (reader);
		if (id != NULL && *id != '\0') {
			/* Calendar entries don’t contain their own selfLink,
			 * so we have to add one manually. */
			uri = g_strconcat ("https://www.googleapis.com/calendar/v3/calendars/", id, NULL);
			_link = gdata_link_new (uri, GDATA_LINK_SELF);
			gdata_entry_add_link (GDATA_ENTRY (parsable), _link);
			g_object_unref (_link);
			g_free (uri);

			/* Similarly for the ACL link. */
			uri = g_strconcat ("https://www.googleapis.com"
			                   "/calendar/v3/calendars/", id,
			                   "/acl", NULL);
			_link = gdata_link_new (uri,
			                        GDATA_LINK_ACCESS_CONTROL_LIST);
			gdata_entry_add_link (GDATA_ENTRY (parsable), _link);
			g_object_unref (_link);
			g_free (uri);
		}

		return GDATA_PARSABLE_CLASS (gdata_calendar_calendar_parent_class)->parse_json (parsable, reader, user_data, error);
	} else {
		return GDATA_PARSABLE_CLASS (gdata_calendar_calendar_parent_class)->parse_json (parsable, reader, user_data, error);
	}

	return TRUE;
}

static void
get_json (GDataParsable *parsable, JsonBuilder *builder)
{
	const gchar *id, *etag, *title, *description;
	gchar *colour;
	GDataCalendarCalendarPrivate *priv = GDATA_CALENDAR_CALENDAR (parsable)->priv;

	id = gdata_entry_get_id (GDATA_ENTRY (parsable));
	if (id != NULL) {
		json_builder_set_member_name (builder, "id");
		json_builder_add_string_value (builder, id);
	}

	json_builder_set_member_name (builder, "kind");
	json_builder_add_string_value (builder, "calendar#calendar");

	/* Add the ETag, if available. */
	etag = gdata_entry_get_etag (GDATA_ENTRY (parsable));
	if (etag != NULL) {
		json_builder_set_member_name (builder, "etag");
		json_builder_add_string_value (builder, etag);
	}

	/* Calendar labels titles as ‘summary’. */
	title = gdata_entry_get_title (GDATA_ENTRY (parsable));
	if (title != NULL) {
		json_builder_set_member_name (builder, "summary");
		json_builder_add_string_value (builder, title);
	}

	description = gdata_entry_get_summary (GDATA_ENTRY (parsable));
	if (description != NULL) {
		json_builder_set_member_name (builder, "description");
		json_builder_add_string_value (builder, description);
	}

	/* Add all the calendar-specific JSON */
	if (priv->timezone != NULL) {
		json_builder_set_member_name (builder, "timeZone");
		json_builder_add_string_value (builder, priv->timezone);
	}

	json_builder_set_member_name (builder, "hidden");
	json_builder_add_boolean_value (builder, priv->is_hidden);

	colour = gdata_color_to_hexadecimal (&priv->colour);
	json_builder_set_member_name (builder, "backgroundColor");
	json_builder_add_string_value (builder, colour);
	g_free (colour);

	json_builder_set_member_name (builder, "selected");
	json_builder_add_boolean_value (builder, priv->is_selected);
}

static const gchar *
get_content_type (void)
{
	return "application/json";
}

/**
 * gdata_calendar_calendar_new:
 * @id: (allow-none): the calendar's ID, or %NULL
 *
 * Creates a new #GDataCalendarCalendar with the given ID and default properties.
 *
 * Return value: a new #GDataCalendarCalendar; unref with g_object_unref()
 */
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
 */
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
 */
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
 * gdata_calendar_calendar_is_hidden:
 * @self: a #GDataCalendarCalendar
 *
 * Gets the #GDataCalendarCalendar:is-hidden property.
 *
 * Return value: %TRUE if the calendar is hidden, %FALSE otherwise
 *
 * Since: 0.2.0
 */
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
 */
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
 */
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
 */
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
 */
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
 */
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
 */
const gchar *
gdata_calendar_calendar_get_access_level (GDataCalendarCalendar *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_CALENDAR (self), NULL);
	return self->priv->access_level;
}
