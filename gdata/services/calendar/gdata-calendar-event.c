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
 * SECTION:gdata-calendar-event
 * @short_description: GData Calendar event object
 * @stability: Stable
 * @include: gdata/services/calendar/gdata-calendar-event.h
 *
 * #GDataCalendarEvent is a subclass of #GDataEntry to represent an event on a calendar from Google Calendar.
 *
 * For more details of Google Calendar's GData API, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/calendar/v3/reference/">
 * online documentation</ulink>.
 *
 * <example>
 * 	<title>Adding a New Event to the Default Calendar</title>
 * 	<programlisting>
 *	GDataCalendarService *service;
 *	GDataCalendarEvent *event, *new_event;
 *	GDataGDWhere *where;
 *	GDataGDWho *who;
 *	GDataGDWhen *when;
 *	gint64 current_time;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service *<!-- -->/
 *	service = create_calendar_service ();
 *
 *	/<!-- -->* Create the new event *<!-- -->/
 *	event = gdata_calendar_event_new (NULL);
 *
 *	gdata_entry_set_title (GDATA_ENTRY (event), "Event Title");
 *	gdata_entry_set_content (GDATA_ENTRY (event), "Event description. This should be a few sentences long.");
 *	gdata_calendar_event_set_status (event, GDATA_GD_EVENT_STATUS_CONFIRMED);
 *
 *	where = gdata_gd_where_new (NULL, "Description of the location", NULL);
 *	gdata_calendar_event_add_place (event, where);
 *	g_object_unref (where);
 *
 *	who = gdata_gd_who_new (GDATA_GD_WHO_EVENT_ORGANIZER, "John Smith", "john.smith@gmail.com");
 *	gdata_calendar_event_add_person (event, who);
 *	g_object_unref (who);
 *
 *	current_time = g_get_real_time () / G_USEC_PER_SEC;
 *	when = gdata_gd_when_new (current_time, current_time + 3600, FALSE);
 *	gdata_calendar_event_add_time (event, when);
 *	g_object_unref (when);
 *
 *	/<!-- -->* Insert the event in the calendar *<!-- -->/
 *	new_event = gdata_calendar_service_insert_event (service, event, NULL, &error);
 *
 *	g_object_unref (event);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error inserting event: %s", error->message);
 *		g_error_free (error);
 *		return NULL;
 *	}
 *
 *	/<!-- -->* Do something with the new_event here, such as return it to the user or store its ID for later usage *<!-- -->/
 *
 *	g_object_unref (new_event);
 * 	</programlisting>
 * </example>
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-calendar-event.h"
#include "gdata-private.h"
#include "gdata-service.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "gdata-comparable.h"

static GObject *gdata_calendar_event_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params);
static void gdata_calendar_event_dispose (GObject *object);
static void gdata_calendar_event_finalize (GObject *object);
static void gdata_calendar_event_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_calendar_event_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void get_json (GDataParsable *parsable, JsonBuilder *builder);
static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);
static gboolean post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error);
static const gchar *get_content_type (void);

struct _GDataCalendarEventPrivate {
	gint64 edited;
	gchar *status;
	gchar *visibility;
	gchar *transparency;
	gchar *uid;
	gint64 sequence;
	GList *times; /* GDataGDWhen */
	gboolean guests_can_modify;
	gboolean guests_can_invite_others;
	gboolean guests_can_see_guests;
	gboolean anyone_can_add_self;
	GList *people; /* GDataGDWho */
	GList *places; /* GDataGDWhere */
	gchar *recurrence;
	gchar *original_event_id;
	gchar *original_event_uri;
	gchar *organiser_email;  /* owned */

	/* Parsing state. */
	struct {
		gint64 start_time;
		gint64 end_time;
		gboolean seen_start;
		gboolean seen_end;
		gboolean start_is_date;
		gboolean end_is_date;
	} parser;
};

enum {
	PROP_EDITED = 1,
	PROP_STATUS,
	PROP_VISIBILITY,
	PROP_TRANSPARENCY,
	PROP_UID,
	PROP_SEQUENCE,
	PROP_GUESTS_CAN_MODIFY,
	PROP_GUESTS_CAN_INVITE_OTHERS,
	PROP_GUESTS_CAN_SEE_GUESTS,
	PROP_ANYONE_CAN_ADD_SELF,
	PROP_RECURRENCE,
	PROP_ORIGINAL_EVENT_ID,
	PROP_ORIGINAL_EVENT_URI
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataCalendarEvent, gdata_calendar_event, GDATA_TYPE_ENTRY)

static void
gdata_calendar_event_class_init (GDataCalendarEventClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	gobject_class->constructor = gdata_calendar_event_constructor;
	gobject_class->get_property = gdata_calendar_event_get_property;
	gobject_class->set_property = gdata_calendar_event_set_property;
	gobject_class->dispose = gdata_calendar_event_dispose;
	gobject_class->finalize = gdata_calendar_event_finalize;

	parsable_class->parse_json = parse_json;
	parsable_class->post_parse_json = post_parse_json;
	parsable_class->get_json = get_json;
	parsable_class->get_content_type = get_content_type;

	entry_class->kind_term = "calendar#event";

	/**
	 * GDataCalendarEvent:edited:
	 *
	 * The last time the event was edited. If the event has not been edited yet, the content indicates the time it was created.
	 *
	 * For more information, see the <ulink type="http" url="http://www.atomenabled.org/developers/protocol/#appEdited">
	 * Atom Publishing Protocol specification</ulink>.
	 */
	g_object_class_install_property (gobject_class, PROP_EDITED,
	                                 g_param_spec_int64 ("edited",
	                                                     "Edited", "The last time the event was edited.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarEvent:status:
	 *
	 * The scheduling status of the event. For example: %GDATA_GD_EVENT_STATUS_CANCELED or %GDATA_GD_EVENT_STATUS_CONFIRMED.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/gdata/elements.html#gdEventStatus">
	 * GData specification</ulink>.
	 *
	 * Since: 0.2.0
	 */
	g_object_class_install_property (gobject_class, PROP_STATUS,
	                                 g_param_spec_string ("status",
	                                                      "Status", "The scheduling status of the event.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarEvent:visibility:
	 *
	 * The event's visibility to calendar users. For example: %GDATA_GD_EVENT_VISIBILITY_PUBLIC or %GDATA_GD_EVENT_VISIBILITY_DEFAULT.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/gdata/elements.html#gdVisibility">
	 * GData specification</ulink>.
	 */
	g_object_class_install_property (gobject_class, PROP_VISIBILITY,
	                                 g_param_spec_string ("visibility",
	                                                      "Visibility", "The event's visibility to calendar users.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarEvent:transparency:
	 *
	 * How the event is marked as consuming time on a calendar. For example: %GDATA_GD_EVENT_TRANSPARENCY_OPAQUE or
	 * %GDATA_GD_EVENT_TRANSPARENCY_TRANSPARENT.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/gdata/elements.html#gdTransparency">
	 * GData specification</ulink>.
	 */
	g_object_class_install_property (gobject_class, PROP_TRANSPARENCY,
	                                 g_param_spec_string ("transparency",
	                                                      "Transparency", "How the event is marked as consuming time on a calendar.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarEvent:uid:
	 *
	 * The globally unique identifier (UID) of the event as defined in Section 4.8.4.7 of <ulink type="http"
	 * url="http://www.ietf.org/rfc/rfc2445.txt">RFC 2445</ulink>.
	 */
	g_object_class_install_property (gobject_class, PROP_UID,
	                                 g_param_spec_string ("uid",
	                                                      "UID", "The globally unique identifier (UID) of the event.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarEvent:sequence:
	 *
	 * The revision sequence number of the event as defined in Section 4.8.7.4 of <ulink type="http"
	 * url="http://www.ietf.org/rfc/rfc2445.txt">RFC 2445</ulink>.
	 */
	g_object_class_install_property (gobject_class, PROP_SEQUENCE,
	                                 g_param_spec_uint ("sequence",
	                                                    "Sequence", "The revision sequence number of the event.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarEvent:guests-can-modify:
	 *
	 * Indicates whether attendees may modify the original event, so that changes are visible to organizers and other attendees.
	 * Otherwise, any changes made by attendees will be restricted to that attendee's calendar.
	 *
	 * For more information, see the
	 * <ulink type="http" url="https://developers.google.com/google-apps/calendar/v3/reference/events#guestsCanInviteOthers">
	 * GData specification</ulink>.
	 */
	g_object_class_install_property (gobject_class, PROP_GUESTS_CAN_MODIFY,
	                                 g_param_spec_boolean ("guests-can-modify",
	                                                       "Guests can modify", "Indicates whether attendees may modify the original event.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarEvent:guests-can-invite-others:
	 *
	 * Indicates whether attendees may invite others to the event.
	 *
	 * For more information, see the <ulink type="http"
	 * url="https://developers.google.com/google-apps/calendar/v3/reference/events#guestsCanInviteOthers">GData specification</ulink>.
	 */
	g_object_class_install_property (gobject_class, PROP_GUESTS_CAN_INVITE_OTHERS,
	                                 g_param_spec_boolean ("guests-can-invite-others",
	                                                       "Guests can invite others", "Indicates whether attendees may invite others.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarEvent:guests-can-see-guests:
	 *
	 * Indicates whether attendees can see other people invited to the event.
	 *
	 * For more information, see the
	 * <ulink type="http" url="https://developers.google.com/google-apps/calendar/v3/reference/events#guestsCanSeeOtherGuests">
	 * GData specification</ulink>.
	 */
	g_object_class_install_property (gobject_class, PROP_GUESTS_CAN_SEE_GUESTS,
	                                 g_param_spec_boolean ("guests-can-see-guests",
	                                                       "Guests can see guests", "Indicates whether attendees can see other people invited.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarEvent:anyone-can-add-self:
	 *
	 * Indicates whether anyone can invite themselves to the event, by adding themselves to the attendee list.
	 */
	g_object_class_install_property (gobject_class, PROP_ANYONE_CAN_ADD_SELF,
	                                 g_param_spec_boolean ("anyone-can-add-self",
	                                                       "Anyone can add self", "Indicates whether anyone can invite themselves to the event.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarEvent:recurrence:
	 *
	 * Represents the dates and times when a recurring event takes place. The returned string is in iCal format, as a list of properties.
	 *
	 * For more information, see the <ulink type="http" url="http://code.google.com/apis/gdata/elements.html#gdRecurrence">
	 * GData specification</ulink>.
	 *
	 * Note: gdata_calendar_event_add_time() and gdata_calendar_event_set_recurrence() are mutually
	 * exclusive. See the documentation for gdata_calendar_event_add_time() for details.
	 *
	 * Since: 0.3.0
	 */
	g_object_class_install_property (gobject_class, PROP_RECURRENCE,
	                                 g_param_spec_string ("recurrence",
	                                                      "Recurrence", "Represents the dates and times when a recurring event takes place.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarEvent:original-event-id:
	 *
	 * The event ID for the original event, if this event is an exception to a recurring event.
	 *
	 * Since: 0.3.0
	 */
	g_object_class_install_property (gobject_class, PROP_ORIGINAL_EVENT_ID,
	                                 g_param_spec_string ("original-event-id",
	                                                      "Original event ID", "The event ID for the original event.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarEvent:original-event-uri:
	 *
	 * The event URI for the original event, if this event is an exception to a recurring event.
	 *
	 * Since: 0.3.0
	 */
	g_object_class_install_property (gobject_class, PROP_ORIGINAL_EVENT_URI,
	                                 g_param_spec_string ("original-event-uri",
	                                                      "Original event URI", "The event URI for the original event.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_calendar_event_init (GDataCalendarEvent *self)
{
	self->priv = gdata_calendar_event_get_instance_private (self);
	self->priv->edited = -1;
}

static GObject *
gdata_calendar_event_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
	GObject *object;

	/* Chain up to the parent class */
	object = G_OBJECT_CLASS (gdata_calendar_event_parent_class)->constructor (type, n_construct_params, construct_params);

	if (_gdata_parsable_is_constructed_from_xml (GDATA_PARSABLE (object)) == FALSE) {
		GDataCalendarEventPrivate *priv = GDATA_CALENDAR_EVENT (object)->priv;

		/* Set the edited property to the current time (creation time). We don't do this in *_init() since that would cause
		 * setting it from parse_xml() to fail (duplicate element). */
		priv->edited = g_get_real_time () / G_USEC_PER_SEC;
	}

	return object;
}

static void
gdata_calendar_event_dispose (GObject *object)
{
	GDataCalendarEventPrivate *priv = GDATA_CALENDAR_EVENT (object)->priv;

	g_list_free_full(priv->times, g_object_unref);
	priv->times = NULL;

	g_list_free_full (priv->people, g_object_unref);
	priv->people = NULL;

	g_list_free_full (priv->places, g_object_unref);
	priv->places = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_calendar_event_parent_class)->dispose (object);
}

static void
gdata_calendar_event_finalize (GObject *object)
{
	GDataCalendarEventPrivate *priv = GDATA_CALENDAR_EVENT (object)->priv;

	g_free (priv->status);
	g_free (priv->visibility);
	g_free (priv->transparency);
	g_free (priv->uid);
	g_free (priv->recurrence);
	g_free (priv->original_event_id);
	g_free (priv->original_event_uri);
	g_free (priv->organiser_email);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_calendar_event_parent_class)->finalize (object);
}

static void
gdata_calendar_event_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataCalendarEventPrivate *priv = GDATA_CALENDAR_EVENT (object)->priv;

	switch (property_id) {
		case PROP_EDITED:
			g_value_set_int64 (value, priv->edited);
			break;
		case PROP_STATUS:
			g_value_set_string (value, priv->status);
			break;
		case PROP_VISIBILITY:
			g_value_set_string (value, priv->visibility);
			break;
		case PROP_TRANSPARENCY:
			g_value_set_string (value, priv->transparency);
			break;
		case PROP_UID:
			g_value_set_string (value, priv->uid);
			break;
		case PROP_SEQUENCE:
			g_value_set_uint (value, CLAMP (priv->sequence, 0, G_MAXUINT));
			break;
		case PROP_GUESTS_CAN_MODIFY:
			g_value_set_boolean (value, priv->guests_can_modify);
			break;
		case PROP_GUESTS_CAN_INVITE_OTHERS:
			g_value_set_boolean (value, priv->guests_can_invite_others);
			break;
		case PROP_GUESTS_CAN_SEE_GUESTS:
			g_value_set_boolean (value, priv->guests_can_see_guests);
			break;
		case PROP_ANYONE_CAN_ADD_SELF:
			g_value_set_boolean (value, priv->anyone_can_add_self);
			break;
		case PROP_RECURRENCE:
			g_value_set_string (value, priv->recurrence);
			break;
		case PROP_ORIGINAL_EVENT_ID:
			g_value_set_string (value, priv->original_event_id);
			break;
		case PROP_ORIGINAL_EVENT_URI:
			g_value_set_string (value, priv->original_event_uri);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_calendar_event_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataCalendarEvent *self = GDATA_CALENDAR_EVENT (object);

	switch (property_id) {
		case PROP_STATUS:
			gdata_calendar_event_set_status (self, g_value_get_string (value));
			break;
		case PROP_VISIBILITY:
			gdata_calendar_event_set_visibility (self, g_value_get_string (value));
			break;
		case PROP_TRANSPARENCY:
			gdata_calendar_event_set_transparency (self, g_value_get_string (value));
			break;
		case PROP_UID:
			gdata_calendar_event_set_uid (self, g_value_get_string (value));
			break;
		case PROP_SEQUENCE:
			gdata_calendar_event_set_sequence (self, g_value_get_uint (value));
			break;
		case PROP_GUESTS_CAN_MODIFY:
			gdata_calendar_event_set_guests_can_modify (self, g_value_get_boolean (value));
			break;
		case PROP_GUESTS_CAN_INVITE_OTHERS:
			gdata_calendar_event_set_guests_can_invite_others (self, g_value_get_boolean (value));
			break;
		case PROP_GUESTS_CAN_SEE_GUESTS:
			gdata_calendar_event_set_guests_can_see_guests (self, g_value_get_boolean (value));
			break;
		case PROP_ANYONE_CAN_ADD_SELF:
			gdata_calendar_event_set_anyone_can_add_self (self, g_value_get_boolean (value));
			break;
		case PROP_RECURRENCE:
			gdata_calendar_event_set_recurrence (self, g_value_get_string (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static gboolean
date_object_from_json (JsonReader *reader,
                       const gchar *member_name,
                       GDataParserOptions options,
                       gint64 *date_time_output,
                       gboolean *is_date_output,
                       gboolean *success,
                       GError **error)
{
	gint64 date_time;
	gboolean is_date = FALSE;
	gboolean found_member = FALSE;

	/* Check if there’s such an element */
	if (g_strcmp0 (json_reader_get_member_name (reader), member_name) != 0) {
		return FALSE;
	}

	/* Check that it’s an object. */
	if (!json_reader_is_object (reader)) {
		const GError *child_error;

		/* Manufacture an error. */
		json_reader_read_member (reader, "dateTime");
		child_error = json_reader_get_error (reader);
		g_assert (child_error != NULL);
		*success = gdata_parser_error_from_json_error (reader,
		                                               child_error,
		                                               error);
		json_reader_end_member (reader);

		return TRUE;
	}

	/* Try to parse either the dateTime or date member. */
	if (json_reader_read_member (reader, "dateTime")) {
		const gchar *date_string;
		const GError *child_error;
		GDateTime *time_val;

		date_string = json_reader_get_string_value (reader);
		child_error = json_reader_get_error (reader);

		if (child_error != NULL) {
			*success = gdata_parser_error_from_json_error (reader,
			                                               child_error,
			                                               error);
			json_reader_end_member (reader);
			return TRUE;
		}

		time_val = g_date_time_new_from_iso8601 (date_string, NULL);
		if (!time_val) {
			*success = gdata_parser_error_not_iso8601_format_json (reader, date_string, error);
			json_reader_end_member (reader);
			return TRUE;
		}

		date_time = g_date_time_to_unix (time_val);
		g_date_time_unref (time_val);
		is_date = FALSE;
		found_member = TRUE;
	}
	json_reader_end_member (reader);

	if (json_reader_read_member (reader, "date")) {
		const gchar *date_string;
		const GError *child_error;

		date_string = json_reader_get_string_value (reader);
		child_error = json_reader_get_error (reader);

		if (child_error != NULL) {
			*success = gdata_parser_error_from_json_error (reader,
			                                               child_error,
			                                               error);
			json_reader_end_member (reader);
			return TRUE;
		}

		if (!gdata_parser_int64_from_date (date_string, &date_time)) {
			*success = gdata_parser_error_not_iso8601_format_json (reader, date_string, error);
			json_reader_end_member (reader);
			return TRUE;
		}

		is_date = TRUE;
		found_member = TRUE;
	}
	json_reader_end_member (reader);

	/* Ignore timeZone; it should be specified in dateTime. */
	if (!found_member) {
		*success = gdata_parser_error_required_json_content_missing (reader, error);
		return TRUE;
	}

	*date_time_output = date_time;
	*is_date_output = is_date;
	*success = TRUE;

	return TRUE;
}

/* Convert between v2 and v3 versions of various enum values. v2 uses a URI
 * style with a constant prefix; v3 simply drops this prefix, and changes the
 * spelling of ‘canceled’ to ‘cancelled’. */
#define V2_PREFIX "http://schemas.google.com/g/2005#event."

static gchar *
add_v2_prefix (const gchar *in)
{
	return g_strconcat (V2_PREFIX, in, NULL);
}

static const gchar *
strip_v2_prefix (const gchar *uri)
{
	/* Convert to v3 format. */
	if (g_str_has_prefix (uri, V2_PREFIX)) {
		return uri + strlen (V2_PREFIX);
	} else {
		return uri;
	}
}

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	gboolean success;
	GDataCalendarEvent *self = GDATA_CALENDAR_EVENT (parsable);
	GDataCalendarEventPrivate *priv = self->priv;

	/* FIXME: Currently unsupported:
	 *  - htmlLink
	 *  - colorId
	 *  - endTimeUnspecified
	 *  - originalStartTime
	 *  - attendeesOmitted
	 *  - extendedProperties
	 *  - hangoutLink
	 *  - gadget
	 *  - privateCopy
	 *  - locked
	 *  - reminders
	 *  - source
	 */

	if (g_strcmp0 (json_reader_get_member_name (reader), "start") == 0) {
		self->priv->parser.seen_start = TRUE;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "end") == 0) {
		self->priv->parser.seen_end = TRUE;
	}

	if (gdata_parser_string_from_json_member (reader, "recurringEventId", P_DEFAULT, &self->priv->original_event_id, &success, error) ||
	    gdata_parser_boolean_from_json_member (reader, "guestsCanModify", P_DEFAULT, &self->priv->guests_can_modify, &success, error) ||
	    gdata_parser_boolean_from_json_member (reader, "guestsCanInviteOthers", P_DEFAULT, &self->priv->guests_can_invite_others, &success, error) ||
	    gdata_parser_boolean_from_json_member (reader, "guestsCanSeeOtherGuests", P_DEFAULT, &self->priv->guests_can_see_guests, &success, error) ||
	    gdata_parser_boolean_from_json_member (reader, "anyoneCanAddSelf", P_DEFAULT, &self->priv->anyone_can_add_self, &success, error) ||
	    gdata_parser_string_from_json_member (reader, "iCalUID", P_DEFAULT, &self->priv->uid, &success, error) ||
	    gdata_parser_int_from_json_member (reader, "sequence", P_DEFAULT, &self->priv->sequence, &success, error) ||
	    gdata_parser_int64_time_from_json_member (reader, "updated", P_DEFAULT, &self->priv->edited, &success, error) ||
	    date_object_from_json (reader, "start", P_DEFAULT, &self->priv->parser.start_time, &self->priv->parser.start_is_date, &success, error) ||
	    date_object_from_json (reader, "end", P_DEFAULT, &self->priv->parser.end_time, &self->priv->parser.end_is_date, &success, error)) {
		if (success) {
			if (self->priv->edited != -1) {
				_gdata_entry_set_updated (GDATA_ENTRY (parsable),
				                          self->priv->edited);
			}

			if (self->priv->original_event_id != NULL) {
				g_free (self->priv->original_event_uri);
				self->priv->original_event_uri = g_strconcat ("https://www.googleapis.com/calendar/v3/events/",
				                                              self->priv->original_event_id, NULL);
			}

			if (self->priv->parser.seen_start && self->priv->parser.seen_end) {
				GDataGDWhen *when;

				when = gdata_gd_when_new (self->priv->parser.start_time,
				                          self->priv->parser.end_time,
				                          self->priv->parser.start_is_date ||
				                          self->priv->parser.end_is_date);
				self->priv->times = g_list_prepend (self->priv->times, when);  /* transfer ownership */

				self->priv->parser.seen_start = FALSE;
				self->priv->parser.seen_end = FALSE;
			}
		}

		return success;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "transparency") == 0) {
		gchar *transparency = NULL;  /* owned */

		g_assert (gdata_parser_string_from_json_member (reader,
		                                                "transparency",
		                                                P_DEFAULT,
		                                                &transparency,
		                                                &success,
		                                                error));

		if (success) {
			priv->transparency = add_v2_prefix (transparency);
		}

		g_free (transparency);

		return success;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "visibility") == 0) {
		gchar *visibility = NULL;  /* owned */

		g_assert (gdata_parser_string_from_json_member (reader,
		                                                "visibility",
		                                                P_DEFAULT,
		                                                &visibility,
		                                                &success,
		                                                error));

		if (success) {
			priv->visibility = add_v2_prefix (visibility);
		}

		g_free (visibility);

		return success;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "status") == 0) {
		gchar *status = NULL;  /* owned */

		g_assert (gdata_parser_string_from_json_member (reader,
		                                                "status",
		                                                P_DEFAULT,
		                                                &status,
		                                                &success,
		                                                error));

		if (success) {
			if (g_strcmp0 (status, "cancelled") == 0) {
				/* Those damned British Englishes. */
				priv->status = add_v2_prefix ("canceled");
			} else {
				priv->status = add_v2_prefix (status);
			}
		}

		g_free (status);

		return success;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "summary") == 0) {
		const gchar *summary;
		const GError *child_error = NULL;

		summary = json_reader_get_string_value (reader);
		child_error = json_reader_get_error (reader);

		if (child_error != NULL) {
			gdata_parser_error_from_json_error (reader,
			                                    child_error, error);
			return FALSE;
		}

		gdata_entry_set_title (GDATA_ENTRY (parsable), summary);
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "description") == 0) {
		const gchar *description;
		const GError *child_error = NULL;

		description = json_reader_get_string_value (reader);
		child_error = json_reader_get_error (reader);

		if (child_error != NULL) {
			gdata_parser_error_from_json_error (reader,
			                                    child_error, error);
			return FALSE;
		}

		gdata_entry_set_content (GDATA_ENTRY (parsable), description);
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "location") == 0) {
		const gchar *location;
		GDataGDWhere *where = NULL;  /* owned */
		const GError *child_error = NULL;

		location = json_reader_get_string_value (reader);
		child_error = json_reader_get_error (reader);

		if (child_error != NULL) {
			gdata_parser_error_from_json_error (reader,
			                                    child_error, error);
			return FALSE;
		}

		where = gdata_gd_where_new (GDATA_GD_WHERE_EVENT,
		                            location, NULL);
		priv->places = g_list_prepend (priv->places, where);  /* transfer ownership */
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "created") == 0) {
		gint64 created;

		g_assert (gdata_parser_int64_time_from_json_member (reader,
		                                                    "created",
		                                                    P_DEFAULT,
		                                                    &created,
		                                                    &success,
		                                                    error));

		if (success) {
			_gdata_entry_set_published (GDATA_ENTRY (parsable),
			                            created);
		}

		return success;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "recurrence") == 0) {
		guint i, j;
		GString *recurrence = NULL;  /* owned */

		/* In the JSON API, the recurrence is given as an array of
		 * strings, each giving an RFC 2445 property such as RRULE,
		 * EXRULE, RDATE or EXDATE. Concatenate them all to form a
		 * recurrence string as used in v2 of the API. */
		if (self->priv->recurrence != NULL) {
			return gdata_parser_error_duplicate_json_element (reader,
			                                                  error);
		}

		recurrence = g_string_new ("");

		for (i = 0, j = json_reader_count_elements (reader); i < j; i++) {
			const gchar *line;
			const GError *child_error;

			json_reader_read_element (reader, i);

			line = json_reader_get_string_value (reader);
			child_error = json_reader_get_error (reader);
			if (child_error != NULL) {
				gdata_parser_error_from_json_error (reader, child_error, error);
				json_reader_end_element (reader);
				return FALSE;
			}

			g_string_append (recurrence, line);
			g_string_append (recurrence, "\n");

			json_reader_end_element (reader);
		}

		g_assert (self->priv->recurrence == NULL);
		self->priv->recurrence = g_string_free (recurrence, FALSE);

		return TRUE;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "attendees") == 0) {
		guint i, j;

		if (priv->people != NULL) {
			return gdata_parser_error_duplicate_json_element (reader,
			                                                  error);
		}

		for (i = 0, j = json_reader_count_elements (reader); i < j; i++) {
			GDataGDWho *who = NULL;  /* owned */
			const gchar *email_address, *value_string;
			const gchar *relation_type;
			gboolean is_organizer, is_resource;
			const GError *child_error;

			json_reader_read_element (reader, i);

			json_reader_read_member (reader, "responseStatus");
			child_error = json_reader_get_error (reader);
			if (child_error != NULL) {
				gdata_parser_error_from_json_error (reader,
				                                    child_error,
				                                    error);
				json_reader_end_member (reader);
				return FALSE;
			}
			json_reader_end_member (reader);

			json_reader_read_member (reader, "email");
			email_address = json_reader_get_string_value (reader);
			json_reader_end_member (reader);

			json_reader_read_member (reader, "displayName");
			value_string = json_reader_get_string_value (reader);
			json_reader_end_member (reader);

			json_reader_read_member (reader, "organizer");
			is_organizer = json_reader_get_boolean_value (reader);
			json_reader_end_member (reader);

			json_reader_read_member (reader, "resource");
			is_resource = json_reader_get_boolean_value (reader);
			json_reader_end_member (reader);

			/* FIXME: Currently unsupported:
			 *  - id
			 *  - self
			 *  - optional (writeble)
			 *  - responseStatus (writeble)
			 *  - comment (writeble)
			 *  - additionalGuests (writeble)
			 */

			if (is_organizer) {
				relation_type = GDATA_GD_WHO_EVENT_ORGANIZER;
			} else if (!is_resource) {
				relation_type = GDATA_GD_WHO_EVENT_ATTENDEE;
			} else {
				/* FIXME: Add support for resources. */
				relation_type = NULL;
			}

			who = gdata_gd_who_new (relation_type, value_string,
			                        email_address);
			priv->people = g_list_prepend (priv->people, who);  /* transfer ownership */

			json_reader_end_element (reader);
		}
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "organizer") == 0) {
		/* This actually gives the parent calendar. Optional. */
		g_clear_pointer (&priv->organiser_email, g_free);
		if (json_reader_read_member (reader, "email"))
			priv->organiser_email = g_strdup (json_reader_get_string_value (reader));
		json_reader_end_member (reader);

		return TRUE;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "creator") == 0) {
		/* These are read-only and already handled as part of
		 * ‘attendees’, so ignore them. */
		return TRUE;
	} else {
		return GDATA_PARSABLE_CLASS (gdata_calendar_event_parent_class)->parse_json (parsable, reader, user_data, error);
	}

	return TRUE;
}

static gboolean
post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error)
{
	GDataLink *_link = NULL;  /* owned */
	const gchar *id, *calendar_id;
	gchar *uri = NULL;  /* owned */
	GDataCalendarEventPrivate *priv;

	priv = GDATA_CALENDAR_EVENT (parsable)->priv;

	/* Set the self link, which is needed for gdata_service_delete_entry().
	 * Unfortunately, it needs the event ID _and_ the calendar ID — which
	 * is perversely only available as the organiser e-mail address. */
	id = gdata_entry_get_id (GDATA_ENTRY (parsable));
	calendar_id = priv->organiser_email;

	if (id == NULL || calendar_id == NULL) {
		return TRUE;
	}

	uri = g_strconcat ("https://www.googleapis.com/calendar/v3/calendars/",
	                   calendar_id, "/events/", id, NULL);
	_link = gdata_link_new (uri, GDATA_LINK_SELF);
	gdata_entry_add_link (GDATA_ENTRY (parsable), _link);
	g_object_unref (_link);
	g_free (uri);

	return TRUE;
}

static void
get_json (GDataParsable *parsable, JsonBuilder *builder)
{
	GList *l;
	const gchar *id, *etag, *title, *description;
	GDataGDWho *organiser_who = NULL;  /* unowned */
	GDataCalendarEventPrivate *priv = GDATA_CALENDAR_EVENT (parsable)->priv;

	/* FIXME: Support:
	 *  - colorId
	 *  - attendeesOmitted
	 *  - extendedProperties
	 *  - gadget
	 *  - reminders
	 *  - source
	 */

	id = gdata_entry_get_id (GDATA_ENTRY (parsable));
	if (id != NULL) {
		json_builder_set_member_name (builder, "id");
		json_builder_add_string_value (builder, id);
	}

	json_builder_set_member_name (builder, "kind");
	json_builder_add_string_value (builder, "calendar#event");

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

	description = gdata_entry_get_content (GDATA_ENTRY (parsable));
	if (description != NULL) {
		json_builder_set_member_name (builder, "description");
		json_builder_add_string_value (builder, description);
	}

	/* Add all the calendar-specific JSON */
	json_builder_set_member_name (builder, "anyoneCanAddSelf");
	json_builder_add_boolean_value (builder, priv->anyone_can_add_self);

	json_builder_set_member_name (builder, "guestsCanInviteOthers");
	json_builder_add_boolean_value (builder, priv->guests_can_invite_others);

	json_builder_set_member_name (builder, "guestsCanModify");
	json_builder_add_boolean_value (builder, priv->guests_can_modify);

	json_builder_set_member_name (builder, "guestsCanSeeOtherGuests");
	json_builder_add_boolean_value (builder, priv->guests_can_see_guests);

	if (priv->transparency != NULL) {
		json_builder_set_member_name (builder, "transparency");
		json_builder_add_string_value (builder,
		                               strip_v2_prefix (priv->transparency));
	}

	if (priv->visibility != NULL) {
		json_builder_set_member_name (builder, "visibility");
		json_builder_add_string_value (builder,
		                               strip_v2_prefix (priv->visibility));
	}

	if (priv->uid != NULL) {
		json_builder_set_member_name (builder, "iCalUID");
		json_builder_add_string_value (builder, priv->uid);
	}

	if (priv->sequence > 0) {
		json_builder_set_member_name (builder, "sequence");
		json_builder_add_int_value (builder, priv->sequence);
	}

	if (priv->status != NULL) {
		const gchar *status;

		/* Convert to v3 format. */
		status = strip_v2_prefix (priv->status);
		if (g_strcmp0 (status, "canceled") == 0) {
			status = "cancelled";
		}

		json_builder_set_member_name (builder, "status");
		json_builder_add_string_value (builder, status);
	}

	if (priv->recurrence != NULL) {
		gchar **parts;
		guint i;

		json_builder_set_member_name (builder, "recurrence");
		json_builder_begin_array (builder);

		parts = g_strsplit (priv->recurrence, "\n", -1);

		for (i = 0; parts[i] != NULL; i++) {
			json_builder_add_string_value (builder, parts[i]);
		}

		g_strfreev (parts);

		json_builder_end_array (builder);
	}

	if (priv->original_event_id != NULL) {
		json_builder_set_member_name (builder, "recurringEventId");
		json_builder_add_string_value (builder, priv->original_event_id);
	}

	/* Times. */
	for (l = priv->times; l != NULL; l = l->next) {
		GDataGDWhen *when;  /* unowned */
		gchar *val = NULL;  /* owned */
		const gchar *member_name;
		gint64 start_time, end_time;

		when = l->data;

		/* Start time. */
		start_time = gdata_gd_when_get_start_time (when);
		json_builder_set_member_name (builder, "start");
		json_builder_begin_object (builder);

		if (gdata_gd_when_is_date (when)) {
			member_name = "date";
			val = gdata_parser_date_from_int64 (start_time);
		} else {
			member_name = "dateTime";
			val = gdata_parser_int64_to_iso8601 (start_time);
		}

		json_builder_set_member_name (builder, member_name);
		json_builder_add_string_value (builder, val);
		g_free (val);

		json_builder_set_member_name (builder, "timeZone");
		json_builder_add_string_value (builder, "UTC");

		json_builder_end_object (builder);

		/* End time. */
		end_time = gdata_gd_when_get_end_time (when);

		if (end_time > -1) {
			json_builder_set_member_name (builder, "end");
			json_builder_begin_object (builder);

			if (gdata_gd_when_is_date (when)) {
				member_name = "date";
				val = gdata_parser_date_from_int64 (end_time);
			} else {
				member_name = "dateTime";
				val = gdata_parser_int64_to_iso8601 (end_time);
			}

			json_builder_set_member_name (builder, member_name);
			json_builder_add_string_value (builder, val);
			g_free (val);

			json_builder_set_member_name (builder, "timeZone");
			json_builder_add_string_value (builder, "UTC");

			json_builder_end_object (builder);
		} else {
			json_builder_set_member_name (builder, "endTimeUnspecified");
			json_builder_add_boolean_value (builder, TRUE);
		}

		/* Only use the first time. :-(
		 * FIXME: There must be a better solution. */
		if (l->next != NULL) {
			g_warning ("Ignoring secondary times; they are no "
			           "longer supported by the server-side API.");
			break;
		}
	}

	/* Locations. */
	for (l = priv->places; l != NULL; l = l->next) {
		GDataGDWhere *where;  /* unowned */
		const gchar *location;

		where = l->data;
		location = gdata_gd_where_get_value_string (where);

		json_builder_set_member_name (builder, "location");
		json_builder_add_string_value (builder, location);

		/* Only use the first location. :-(
		 * FIXME: There must be a better solution. */
		if (l->next != NULL) {
			g_warning ("Ignoring secondary locations; they are no "
			           "longer supported by the server-side API.");
			break;
		}
	}

	/* People. */
	json_builder_set_member_name (builder, "attendees");
	json_builder_begin_array (builder);

	for (l = priv->people; l != NULL; l = l->next) {
		GDataGDWho *who;  /* unowned */
		const gchar *display_name, *email_address;

		who = l->data;

		json_builder_begin_object (builder);

		display_name = gdata_gd_who_get_value_string (who);
		if (display_name != NULL) {
			json_builder_set_member_name (builder, "displayName");
			json_builder_add_string_value (builder, display_name);
		}

		email_address = gdata_gd_who_get_email_address (who);
		if (email_address != NULL) {
			json_builder_set_member_name (builder, "email");
			json_builder_add_string_value (builder, email_address);
		}

		if (g_strcmp0 (gdata_gd_who_get_relation_type (who),
		               GDATA_GD_WHO_EVENT_ORGANIZER) == 0) {
			json_builder_set_member_name (builder, "organizer");
			json_builder_add_boolean_value (builder, TRUE);

			organiser_who = who;
		}

		json_builder_end_object (builder);
	}

	json_builder_end_array (builder);

	if (organiser_who != NULL) {
		const gchar *display_name, *email_address;

		json_builder_set_member_name (builder, "organizer");
		json_builder_begin_object (builder);

		display_name = gdata_gd_who_get_value_string (organiser_who);
		if (display_name != NULL) {
			json_builder_set_member_name (builder, "displayName");
			json_builder_add_string_value (builder, display_name);
		}

		email_address = gdata_gd_who_get_email_address (organiser_who);
		if (email_address != NULL) {
			json_builder_set_member_name (builder, "email");
			json_builder_add_string_value (builder, email_address);
		}

		json_builder_end_object (builder);
	}
}

static const gchar *
get_content_type (void)
{
	return "application/json";
}

/**
 * gdata_calendar_event_new:
 * @id: (allow-none): the event's ID, or %NULL
 *
 * Creates a new #GDataCalendarEvent with the given ID and default properties.
 *
 * Return value: a new #GDataCalendarEvent; unref with g_object_unref()
 */
GDataCalendarEvent *
gdata_calendar_event_new (const gchar *id)
{
	return GDATA_CALENDAR_EVENT (g_object_new (GDATA_TYPE_CALENDAR_EVENT, "id", id, NULL));
}

/**
 * gdata_calendar_event_get_edited:
 * @self: a #GDataCalendarEvent
 *
 * Gets the #GDataCalendarEvent:edited property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the UNIX timestamp for the time the event was last edited, or <code class="literal">-1</code>
 */
gint64
gdata_calendar_event_get_edited (GDataCalendarEvent *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (self), -1);
	return self->priv->edited;
}

/**
 * gdata_calendar_event_get_status:
 * @self: a #GDataCalendarEvent
 *
 * Gets the #GDataCalendarEvent:status property.
 *
 * Return value: the event status, or %NULL
 *
 * Since: 0.2.0
 */
const gchar *
gdata_calendar_event_get_status (GDataCalendarEvent *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (self), NULL);
	return self->priv->status;
}

/**
 * gdata_calendar_event_set_status:
 * @self: a #GDataCalendarEvent
 * @status: (allow-none): a new event status, or %NULL
 *
 * Sets the #GDataCalendarEvent:status property to the new status, @status.
 *
 * Set @status to %NULL to unset the property in the event.
 *
 * Since: 0.2.0
 */
void
gdata_calendar_event_set_status (GDataCalendarEvent *self, const gchar *status)
{
	g_return_if_fail (GDATA_IS_CALENDAR_EVENT (self));

	g_free (self->priv->status);
	self->priv->status = g_strdup (status);
	g_object_notify (G_OBJECT (self), "status");
}

/**
 * gdata_calendar_event_get_visibility:
 * @self: a #GDataCalendarEvent
 *
 * Gets the #GDataCalendarEvent:visibility property.
 *
 * Return value: the event visibility, or %NULL
 */
const gchar *
gdata_calendar_event_get_visibility (GDataCalendarEvent *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (self), NULL);
	return self->priv->visibility;
}

/**
 * gdata_calendar_event_set_visibility:
 * @self: a #GDataCalendarEvent
 * @visibility: (allow-none): a new event visibility, or %NULL
 *
 * Sets the #GDataCalendarEvent:visibility property to the new visibility, @visibility.
 *
 * Set @visibility to %NULL to unset the property in the event.
 */
void
gdata_calendar_event_set_visibility (GDataCalendarEvent *self, const gchar *visibility)
{
	g_return_if_fail (GDATA_IS_CALENDAR_EVENT (self));

	g_free (self->priv->visibility);
	self->priv->visibility = g_strdup (visibility);
	g_object_notify (G_OBJECT (self), "visibility");
}

/**
 * gdata_calendar_event_get_transparency:
 * @self: a #GDataCalendarEvent
 *
 * Gets the #GDataCalendarEvent:transparency property.
 *
 * Return value: the event transparency, or %NULL
 */
const gchar *
gdata_calendar_event_get_transparency (GDataCalendarEvent *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (self), NULL);
	return self->priv->transparency;
}

/**
 * gdata_calendar_event_set_transparency:
 * @self: a #GDataCalendarEvent
 * @transparency: (allow-none): a new event transparency, or %NULL
 *
 * Sets the #GDataCalendarEvent:transparency property to the new transparency, @transparency.
 *
 * Set @transparency to %NULL to unset the property in the event.
 */
void
gdata_calendar_event_set_transparency (GDataCalendarEvent *self, const gchar *transparency)
{
	g_return_if_fail (GDATA_IS_CALENDAR_EVENT (self));

	g_free (self->priv->transparency);
	self->priv->transparency = g_strdup (transparency);
	g_object_notify (G_OBJECT (self), "transparency");
}

/**
 * gdata_calendar_event_get_uid:
 * @self: a #GDataCalendarEvent
 *
 * Gets the #GDataCalendarEvent:uid property.
 *
 * Return value: the event's UID, or %NULL
 */
const gchar *
gdata_calendar_event_get_uid (GDataCalendarEvent *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (self), NULL);
	return self->priv->uid;
}

/**
 * gdata_calendar_event_set_uid:
 * @self: a #GDataCalendarEvent
 * @uid: (allow-none): a new event UID, or %NULL
 *
 * Sets the #GDataCalendarEvent:uid property to the new UID, @uid.
 *
 * Set @uid to %NULL to unset the property in the event.
 */
void
gdata_calendar_event_set_uid (GDataCalendarEvent *self, const gchar *uid)
{
	/* TODO: is modifying this allowed? */
	g_return_if_fail (GDATA_IS_CALENDAR_EVENT (self));

	g_free (self->priv->uid);
	self->priv->uid = g_strdup (uid);
	g_object_notify (G_OBJECT (self), "uid");
}

/**
 * gdata_calendar_event_get_sequence:
 * @self: a #GDataCalendarEvent
 *
 * Gets the #GDataCalendarEvent:sequence property.
 *
 * Return value: the event's sequence number
 */
guint
gdata_calendar_event_get_sequence (GDataCalendarEvent *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (self), 0);
	return CLAMP (self->priv->sequence, 0, G_MAXUINT);
}

/**
 * gdata_calendar_event_set_sequence:
 * @self: a #GDataCalendarEvent
 * @sequence: a new sequence number, or <code class="literal">0</code>
 *
 * Sets the #GDataCalendarEvent:sequence property to the new sequence number, @sequence.
 */
void
gdata_calendar_event_set_sequence (GDataCalendarEvent *self, guint sequence)
{
	g_return_if_fail (GDATA_IS_CALENDAR_EVENT (self));
	self->priv->sequence = sequence;
	g_object_notify (G_OBJECT (self), "sequence");
}

/**
 * gdata_calendar_event_get_guests_can_modify:
 * @self: a #GDataCalendarEvent
 *
 * Gets the #GDataCalendarEvent:guests-can-modify property.
 *
 * Return value: %TRUE if attendees can modify the original event, %FALSE otherwise
 */
gboolean
gdata_calendar_event_get_guests_can_modify (GDataCalendarEvent *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (self), FALSE);
	return self->priv->guests_can_modify;
}

/**
 * gdata_calendar_event_set_guests_can_modify:
 * @self: a #GDataCalendarEvent
 * @guests_can_modify: %TRUE if attendees can modify the original event, %FALSE otherwise
 *
 * Sets the #GDataCalendarEvent:guests-can-modify property to @guests_can_modify.
 */
void
gdata_calendar_event_set_guests_can_modify (GDataCalendarEvent *self, gboolean guests_can_modify)
{
	g_return_if_fail (GDATA_IS_CALENDAR_EVENT (self));
	self->priv->guests_can_modify = guests_can_modify;
	g_object_notify (G_OBJECT (self), "guests-can-modify");
}

/**
 * gdata_calendar_event_get_guests_can_invite_others:
 * @self: a #GDataCalendarEvent
 *
 * Gets the #GDataCalendarEvent:guests-can-invite-others property.
 *
 * Return value: %TRUE if attendees can invite others to the event, %FALSE otherwise
 */
gboolean
gdata_calendar_event_get_guests_can_invite_others (GDataCalendarEvent *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (self), FALSE);
	return self->priv->guests_can_invite_others;
}

/**
 * gdata_calendar_event_set_guests_can_invite_others:
 * @self: a #GDataCalendarEvent
 * @guests_can_invite_others: %TRUE if attendees can invite others to the event, %FALSE otherwise
 *
 * Sets the #GDataCalendarEvent:guests-can-invite-others property to @guests_can_invite_others.
 */
void
gdata_calendar_event_set_guests_can_invite_others (GDataCalendarEvent *self, gboolean guests_can_invite_others)
{
	g_return_if_fail (GDATA_IS_CALENDAR_EVENT (self));
	self->priv->guests_can_invite_others = guests_can_invite_others;
	g_object_notify (G_OBJECT (self), "guests-can-invite-others");
}

/**
 * gdata_calendar_event_get_guests_can_see_guests:
 * @self: a #GDataCalendarEvent
 *
 * Gets the #GDataCalendarEvent:guests-can-see-guests property.
 *
 * Return value: %TRUE if attendees can see who's attending the event, %FALSE otherwise
 */
gboolean
gdata_calendar_event_get_guests_can_see_guests (GDataCalendarEvent *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (self), FALSE);
	return self->priv->guests_can_see_guests;
}

/**
 * gdata_calendar_event_set_guests_can_see_guests:
 * @self: a #GDataCalendarEvent
 * @guests_can_see_guests: %TRUE if attendees can see who's attending the event, %FALSE otherwise
 *
 * Sets the #GDataCalendarEvent:guests-can-see-guests property to @guests_can_see_guests.
 */
void
gdata_calendar_event_set_guests_can_see_guests (GDataCalendarEvent *self, gboolean guests_can_see_guests)
{
	g_return_if_fail (GDATA_IS_CALENDAR_EVENT (self));
	self->priv->guests_can_see_guests = guests_can_see_guests;
	g_object_notify (G_OBJECT (self), "guests-can-see-guests");
}

/**
 * gdata_calendar_event_get_anyone_can_add_self:
 * @self: a #GDataCalendarEvent
 *
 * Gets the #GDataCalendarEvent:anyone-can-add-self property.
 *
 * Return value: %TRUE if anyone can add themselves as an attendee to the event, %FALSE otherwise
 */
gboolean
gdata_calendar_event_get_anyone_can_add_self (GDataCalendarEvent *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (self), FALSE);
	return self->priv->anyone_can_add_self;
}

/**
 * gdata_calendar_event_set_anyone_can_add_self:
 * @self: a #GDataCalendarEvent
 * @anyone_can_add_self: %TRUE if anyone can add themselves as an attendee to the event, %FALSE otherwise
 *
 * Sets the #GDataCalendarEvent:anyone-can-add-self property to @anyone_can_add_self.
 */
void
gdata_calendar_event_set_anyone_can_add_self (GDataCalendarEvent *self, gboolean anyone_can_add_self)
{
	g_return_if_fail (GDATA_IS_CALENDAR_EVENT (self));
	self->priv->anyone_can_add_self = anyone_can_add_self;
	g_object_notify (G_OBJECT (self), "anyone-can-add-self");
}

/**
 * gdata_calendar_event_add_person:
 * @self: a #GDataCalendarEvent
 * @who: a #GDataGDWho to add
 *
 * Adds the person @who to the event as a guest (attendee, organiser, performer, etc.), and increments its reference count.
 *
 * Duplicate people will not be added to the list.
 */
void
gdata_calendar_event_add_person (GDataCalendarEvent *self, GDataGDWho *who)
{
	g_return_if_fail (GDATA_IS_CALENDAR_EVENT (self));
	g_return_if_fail (GDATA_IS_GD_WHO (who));

	if (g_list_find_custom (self->priv->people, who, (GCompareFunc) gdata_comparable_compare) == NULL)
		self->priv->people = g_list_append (self->priv->people, g_object_ref (who));
}

/**
 * gdata_calendar_event_get_people:
 * @self: a #GDataCalendarEvent
 *
 * Gets a list of the people attending the event.
 *
 * Return value: (element-type GData.GDWho) (transfer none): a #GList of #GDataGDWhos, or %NULL
 *
 * Since: 0.2.0
 */
GList *
gdata_calendar_event_get_people (GDataCalendarEvent *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (self), NULL);
	return self->priv->people;
}

/**
 * gdata_calendar_event_add_place:
 * @self: a #GDataCalendarEvent
 * @where: a #GDataGDWhere to add
 *
 * Adds the place @where to the event as a location and increments its reference count.
 *
 * Duplicate places will not be added to the list.
 */
void
gdata_calendar_event_add_place (GDataCalendarEvent *self, GDataGDWhere *where)
{
	g_return_if_fail (GDATA_IS_CALENDAR_EVENT (self));
	g_return_if_fail (GDATA_IS_GD_WHERE (where));

	if (g_list_find_custom (self->priv->places, where, (GCompareFunc) gdata_comparable_compare) == NULL)
		self->priv->places = g_list_append (self->priv->places, g_object_ref (where));
}

/**
 * gdata_calendar_event_get_places:
 * @self: a #GDataCalendarEvent
 *
 * Gets a list of the locations associated with the event.
 *
 * Return value: (element-type GData.GDWhere) (transfer none): a #GList of #GDataGDWheres, or %NULL
 *
 * Since: 0.2.0
 */
GList *
gdata_calendar_event_get_places (GDataCalendarEvent *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (self), NULL);
	return self->priv->places;
}

/**
 * gdata_calendar_event_add_time:
 * @self: a #GDataCalendarEvent
 * @when: a #GDataGDWhen to add
 *
 * Adds @when to the event as a time period when the event happens, and increments its reference count.
 *
 * Duplicate times will not be added to the list.
 *
 * Note: gdata_calendar_event_add_time() and gdata_calendar_event_set_recurrence() are mutually
 * exclusive, as the server doesn't support positive exceptions to recurrence rules. If recurrences
 * are required, use gdata_calendar_event_set_recurrence(). Note that this means reminders cannot
 * be set for the event, as they are only supported by #GDataGDWhen. No checks are performed for
 * these forbidden conditions, as to do so would break libgdata's API; if both a recurrence is set
 * and a specific time is added, the server will return an error when the #GDataCalendarEvent is
 * inserted using gdata_service_insert_entry().
 *
 * Since: 0.2.0
 */
void
gdata_calendar_event_add_time (GDataCalendarEvent *self, GDataGDWhen *when)
{
	g_return_if_fail (GDATA_IS_CALENDAR_EVENT (self));
	g_return_if_fail (GDATA_IS_GD_WHEN (when));

	if (g_list_find_custom (self->priv->times, when, (GCompareFunc) gdata_comparable_compare) == NULL)
		self->priv->times = g_list_append (self->priv->times, g_object_ref (when));
}

/**
 * gdata_calendar_event_get_times:
 * @self: a #GDataCalendarEvent
 *
 * Gets a list of the time periods associated with the event.
 *
 * Return value: (element-type GData.GDWhen) (transfer none): a #GList of #GDataGDWhens, or %NULL
 *
 * Since: 0.2.0
 */
GList *
gdata_calendar_event_get_times (GDataCalendarEvent *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (self), NULL);
	return self->priv->times;
}

/**
 * gdata_calendar_event_get_primary_time:
 * @self: a #GDataCalendarEvent
 * @start_time: (out caller-allocates): a #gint64 for the start time, or %NULL
 * @end_time: (out caller-allocates): a #gint64 for the end time, or %NULL
 * @when: (out callee-allocates) (transfer none): a #GDataGDWhen for the primary time structure, or %NULL
 *
 * Gets the first time period associated with the event, conveniently returning just its start and
 * end times if required.
 *
 * If there are no time periods, or more than one time period, associated with the event, %FALSE will
 * be returned, and the parameters will remain unmodified.
 *
 * Return value: %TRUE if there is only one time period associated with the event, %FALSE otherwise
 *
 * Since: 0.2.0
 */
gboolean
gdata_calendar_event_get_primary_time (GDataCalendarEvent *self, gint64 *start_time, gint64 *end_time, GDataGDWhen **when)
{
	GDataGDWhen *primary_when;

	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (self), FALSE);

	if (self->priv->times == NULL || self->priv->times->next != NULL)
		return FALSE;

	primary_when = GDATA_GD_WHEN (self->priv->times->data);
	if (start_time != NULL)
		*start_time = gdata_gd_when_get_start_time (primary_when);
	if (end_time != NULL)
		*end_time = gdata_gd_when_get_end_time (primary_when);
	if (when != NULL)
		*when = primary_when;

	return TRUE;
}

/**
 * gdata_calendar_event_get_recurrence:
 * @self: a #GDataCalendarEvent
 *
 * Gets the #GDataCalendarEvent:recurrence property.
 *
 * Return value: the event recurrence patterns, or %NULL
 *
 * Since: 0.3.0
 */
const gchar *
gdata_calendar_event_get_recurrence (GDataCalendarEvent *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (self), NULL);
	return self->priv->recurrence;
}

/**
 * gdata_calendar_event_set_recurrence:
 * @self: a #GDataCalendarEvent
 * @recurrence: (allow-none): a new event recurrence, or %NULL
 *
 * Sets the #GDataCalendarEvent:recurrence property to the new recurrence, @recurrence.
 *
 * Set @recurrence to %NULL to unset the property in the event.
 *
 * Note: gdata_calendar_event_add_time() and gdata_calendar_event_set_recurrence() are mutually
 * exclusive. See the documentation for gdata_calendar_event_add_time() for details.
 *
 * Since: 0.3.0
 */
void
gdata_calendar_event_set_recurrence (GDataCalendarEvent *self, const gchar *recurrence)
{
	g_return_if_fail (GDATA_IS_CALENDAR_EVENT (self));

	g_free (self->priv->recurrence);
	self->priv->recurrence = g_strdup (recurrence);
	g_object_notify (G_OBJECT (self), "recurrence");
}

/**
 * gdata_calendar_event_get_original_event_details:
 * @self: a #GDataCalendarEvent
 * @event_id: (out callee-allocates) (transfer full): return location for the original event's ID, or %NULL
 * @event_uri: (out callee-allocates) (transfer full): return location for the original event's URI, or %NULL
 *
 * Gets details of the original event, if this event is an exception to a recurring event. The original
 * event's ID and the URI of the event's XML are returned in @event_id and @event_uri, respectively.
 *
 * If this event is not an exception to a recurring event, @event_id and @event_uri will be set to %NULL.
 * See gdata_calendar_event_is_exception() to determine more simply whether an event is an exception to a
 * recurring event.
 *
 * If both @event_id and @event_uri are %NULL, this function is a no-op. Otherwise, they should both be
 * freed with g_free().
 *
 * Since: 0.3.0
 */
void
gdata_calendar_event_get_original_event_details (GDataCalendarEvent *self, gchar **event_id, gchar **event_uri)
{
	g_return_if_fail (GDATA_IS_CALENDAR_EVENT (self));

	if (event_id != NULL)
		*event_id = g_strdup (self->priv->original_event_id);
	if (event_uri != NULL)
		*event_uri = g_strdup (self->priv->original_event_uri);
}

/**
 * gdata_calendar_event_is_exception:
 * @self: a #GDataCalendarEvent
 *
 * Determines whether the event is an exception to a recurring event. If it is, details of the original event
 * can be retrieved using gdata_calendar_event_get_original_event_details().
 *
 * Return value: %TRUE if the event is an exception, %FALSE otherwise
 *
 * Since: 0.3.0
 */
gboolean
gdata_calendar_event_is_exception (GDataCalendarEvent *self)
{
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (self), FALSE);
	return (self->priv->original_event_id != NULL && self->priv->original_event_uri != NULL) ? TRUE : FALSE;
}
