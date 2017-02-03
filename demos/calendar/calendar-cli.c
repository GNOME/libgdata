/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) 2015 Philip Withnall <philip@tecnocode.co.uk>
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

#include <gdata/gdata.h>
#include <locale.h>
#include <string.h>

#define CLIENT_ID "1074795795536-necvslvs0pchk65nf6ju4i6mniogg8fr.apps.googleusercontent.com"
#define CLIENT_SECRET "8totRi50eo2Zfr3SD2DeNAzo"
#define REDIRECT_URI "urn:ietf:wg:oauth:2.0:oob"

static int
print_usage (char *argv[])
{
	g_printerr ("%s: Usage — %s <subcommand>\n"
	            "Subcommands:\n"
	            "   calendars [--all|--own]\n"
	            "   events <calendar ID> [query string]\n"
	            "   insert-event <calendar ID> <title> <start time> "
	               "<end time> <attendee 1> [attendee 2 …]\n",
	            argv[0], argv[0]);
	return -1;
}

/* Convert a GTimeVal to an ISO 8601 date string (without a time component). */
static gchar *
tv_to_iso8601_date (GTimeVal *tv)
{
	struct tm *tm;

	tm = gmtime (&tv->tv_sec);

	return g_strdup_printf ("%04d-%02d-%02d",
	                        tm->tm_year + 1900,
	                        tm->tm_mon + 1,
	                        tm->tm_mday);
}

static void
print_calendar (GDataCalendarCalendar *calendar)
{
	const gchar *id, *title, *time_zone, *access_level, *description;
	gboolean is_hidden, is_selected;

	id = gdata_entry_get_id (GDATA_ENTRY (calendar));
	title = gdata_entry_get_title (GDATA_ENTRY (calendar));
	time_zone = gdata_calendar_calendar_get_timezone (calendar);
	is_hidden = gdata_calendar_calendar_is_hidden (calendar);
	is_selected = gdata_calendar_calendar_is_selected (calendar);
	access_level = gdata_calendar_calendar_get_access_level (calendar);
	description = gdata_entry_get_summary (GDATA_ENTRY (calendar));

	g_print ("%s — %s\n", id, title);
	g_print ("   Timezone: %s\n", time_zone);
	g_print ("   Access level: %s\n", access_level);
	g_print ("   Hidden? %s\n", is_hidden ? "Yes" : "No");
	g_print ("   Selected? %s\n", is_selected ? "Yes" : "No");
	g_print ("   Description:\n      %s\n", description);

	g_print ("\n");
}

static void
print_event (GDataCalendarEvent *event)
{
	const gchar *title, *id, *description, *status, *visibility;
	const gchar *transparency, *uid;
	GTimeVal date_published_tv = { 0, };
	GTimeVal date_edited_tv = { 0, };
	gchar *date_published = NULL;  /* owned */
	gchar *date_edited = NULL;  /* owned */
	guint sequence;
	gboolean guests_can_modify, guests_can_invite_others;
	gboolean guests_can_see_guests, anyone_can_add_self;
	GList/*<unowned GDataGDWho>*/ *people;  /* unowned */
	GList/*<unowned GDataGDWhere>*/ *places;  /* unowned */
	GList/*<unowned GDataGDWhen>*/ *times;  /* unowned */

	title = gdata_entry_get_title (GDATA_ENTRY (event));
	id = gdata_entry_get_id (GDATA_ENTRY (event));
	description = gdata_entry_get_content (GDATA_ENTRY (event));
	date_published_tv.tv_sec = gdata_entry_get_published (GDATA_ENTRY (event));
	date_published = g_time_val_to_iso8601 (&date_published_tv);
	date_edited_tv.tv_sec = gdata_calendar_event_get_edited (event);
	date_edited = g_time_val_to_iso8601 (&date_edited_tv);
	status = gdata_calendar_event_get_status (event);
	visibility = gdata_calendar_event_get_visibility (event);
	transparency = gdata_calendar_event_get_transparency (event);
	uid = gdata_calendar_event_get_uid (event);
	sequence = gdata_calendar_event_get_sequence (event);
	guests_can_modify = gdata_calendar_event_get_guests_can_modify (event);
	guests_can_invite_others = gdata_calendar_event_get_guests_can_invite_others (event);
	guests_can_see_guests = gdata_calendar_event_get_guests_can_see_guests (event);
	anyone_can_add_self = gdata_calendar_event_get_anyone_can_add_self (event);
	people = gdata_calendar_event_get_people (event);
	places = gdata_calendar_event_get_places (event);
	times = gdata_calendar_event_get_times (event);

	g_print ("%s — %s\n", id, title);
	g_print ("   UID: %s\n", uid);
	g_print ("   Sequence: %u\n", sequence);
	g_print ("   Published: %s\n", date_published);
	g_print ("   Edited: %s\n", date_edited);
	g_print ("   Status: %s\n", status);
	g_print ("   Visibility: %s\n", visibility);
	g_print ("   Transparency: %s\n", transparency);
	g_print ("   Guests can modify event? %s\n",
	         guests_can_modify ? "Yes" : "No");
	g_print ("   Guests can invite others? %s\n",
	         guests_can_invite_others ? "Yes" : "No");
	g_print ("   Guests can see guest list? %s\n",
	         guests_can_see_guests ? "Yes" : "No");
	g_print ("   Anyone can add themselves? %s\n",
	         anyone_can_add_self ? "Yes" : "No");
	g_print ("   Description:\n      %s\n", description);

	g_print ("   Guests:\n");

	for (; people != NULL; people = people->next) {
		GDataGDWho *who;

		who = GDATA_GD_WHO (people->data);
		g_print ("    • %s — %s (%s)\n",
		         gdata_gd_who_get_value_string (who),
		         gdata_gd_who_get_email_address (who),
		         gdata_gd_who_get_relation_type (who));
	}

	g_print ("   Locations:\n");

	for (; places != NULL; places = places->next) {
		GDataGDWhere *where;

		where = GDATA_GD_WHERE (places->data);
		g_print ("    • %s\n", gdata_gd_where_get_value_string (where));
	}

	g_print ("   Times:\n");

	for (; times != NULL; times = times->next) {
		GDataGDWhen *when;
		GTimeVal start_time = { 0, }, end_time = { 0, };
		gchar *start = NULL, *end = NULL;  /* owned */

		when = GDATA_GD_WHEN (times->data);

		start_time.tv_sec = gdata_gd_when_get_start_time (when);
		end_time.tv_sec = gdata_gd_when_get_end_time (when);

		if (gdata_gd_when_is_date (when)) {
			start = tv_to_iso8601_date (&start_time);
			end = tv_to_iso8601_date (&end_time);
		} else {
			start = g_time_val_to_iso8601 (&start_time);
			end = g_time_val_to_iso8601 (&end_time);
		}

		g_print ("    • %s to %s (%s)\n",
		         start, end, gdata_gd_when_get_value_string (when));

		/* TODO: Reminders are not supported yet. */
	}

	g_print ("\n");

	g_free (date_published);
}

static GDataAuthorizer *
create_authorizer (GError **error)
{
	GDataOAuth2Authorizer *authorizer = NULL;  /* owned */
	gchar *uri = NULL;
	gchar code[100];
	GError *child_error = NULL;

	/* Go through the interactive OAuth dance. */
	authorizer = gdata_oauth2_authorizer_new (CLIENT_ID, CLIENT_SECRET,
	                                          REDIRECT_URI,
	                                          GDATA_TYPE_CALENDAR_SERVICE);

	/* Get an authentication URI */
	uri = gdata_oauth2_authorizer_build_authentication_uri (authorizer,
	                                                        NULL, FALSE);

	/* Wait for the user to retrieve and enter the verifier. */
	g_print ("Please navigate to the following URI and grant access:\n"
	         "   %s\n", uri);
	g_print ("Enter verifier (EOF to abort): ");

	g_free (uri);

	if (scanf ("%100s", code) != 1) {
		/* User chose to abort. */
		g_print ("\n");
		g_clear_object (&authorizer);
		return NULL;
	}

	/* Authorise the token. */
	gdata_oauth2_authorizer_request_authorization (authorizer, code, NULL,
	                                               &child_error);

	if (child_error != NULL) {
		g_propagate_error (error, child_error);
		g_clear_object (&authorizer);
		return NULL;
	}

	return GDATA_AUTHORIZER (authorizer);
}

/* List all the user’s calendars. */
static int
command_calendars (int argc, char *argv[])
{
	GDataCalendarService *service = NULL;
	GDataCalendarQuery *query = NULL;
	GDataFeed *feed = NULL;
	GList/*<unowned GDataCalendarCalendar>*/ *entries;
	GError *error = NULL;
	gint retval = 0;
	gboolean only_own;  /* only query for calendars the user owns */
	GDataAuthorizer *authorizer = NULL;

	if (argc < 2) {
		return print_usage (argv);
	} else if (argc == 2) {
		only_own = FALSE;
	} else if (g_strcmp0 (argv[2], "--all") == 0 ||
	           g_strcmp0 (argv[2], "--own") == 0) {
		only_own = (g_strcmp0 (argv[2], "--own") == 0);
	} else {
		only_own = FALSE;
	}

	/* Authenticate and create a service. */
	authorizer = create_authorizer (&error);

	if (error != NULL) {
		g_printerr ("%s: Error authenticating: %s\n",
		            argv[0], error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	} else if (authorizer == NULL) {
		g_printerr ("%s: User chose to abort authentication.\n",
		            argv[0]);
		retval = 1;
		goto done;
	}

	service = gdata_calendar_service_new (authorizer);
	query = gdata_calendar_query_new (NULL);

	if (only_own) {
		feed = gdata_calendar_service_query_own_calendars (service,
		                                                   GDATA_QUERY (query),
		                                                   NULL, NULL,
		                                                   NULL,
		                                                   &error);
	} else {
		feed = gdata_calendar_service_query_all_calendars (service,
		                                                   GDATA_QUERY (query),
		                                                   NULL, NULL,
		                                                   NULL,
		                                                   &error);
	}

	if (error != NULL) {
		g_printerr ("%s: Error querying calendars: %s\n",
		            argv[0], error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	}

	/* Print results. */
	for (entries = gdata_feed_get_entries (feed); entries != NULL;
	     entries = entries->next) {
		GDataCalendarCalendar *calendar;

		calendar = GDATA_CALENDAR_CALENDAR (entries->data);
		print_calendar (calendar);
	}

	g_print ("Total of %u results.\n",
	         g_list_length (gdata_feed_get_entries (feed)));

done:
	g_clear_object (&feed);
	g_clear_object (&query);
	g_clear_object (&authorizer);
	g_clear_object (&service);

	return retval;
}

/* Query the events in a calendar. */
static int
command_events (int argc, char *argv[])
{
	GDataCalendarService *service = NULL;
	GDataCalendarCalendar *calendar = NULL;
	GDataCalendarQuery *query = NULL;
	GError *error = NULL;
	gint retval = 0;
	const gchar *query_string, *calendar_id;
	GDataAuthorizer *authorizer = NULL;
	guint n_results;

	if (argc < 3) {
		return print_usage (argv);
	}

	calendar_id = argv[2];
	query_string = (argc > 3) ? argv[3] : NULL;

	/* Authenticate and create a service. */
	authorizer = create_authorizer (&error);

	if (error != NULL) {
		g_printerr ("%s: Error authenticating: %s\n",
		            argv[0], error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	} else if (authorizer == NULL) {
		g_printerr ("%s: User chose to abort authentication.\n",
		            argv[0]);
		retval = 1;
		goto done;
	}

	service = gdata_calendar_service_new (authorizer);
	query = gdata_calendar_query_new (query_string);
	gdata_query_set_max_results (GDATA_QUERY (query), 10);
	calendar = gdata_calendar_calendar_new (calendar_id);
	n_results = 0;

	while (TRUE) {
		GList/*<unowned GDataCalendarEvent>*/ *entries, *l;
		GDataFeed *feed = NULL;

		feed = gdata_calendar_service_query_events (service, calendar,
		                                            GDATA_QUERY (query), NULL,
		                                            NULL, NULL, &error);

		if (error != NULL) {
			g_printerr ("%s: Error querying events: %s\n",
			            argv[0], error->message);
			g_error_free (error);
			retval = 1;
			goto done;
		}

		/* Print results. */
		entries = gdata_feed_get_entries (feed);

		if (entries == NULL) {
			retval = 0;
			g_object_unref (feed);
			goto done;
		}

		for (l = entries; l != NULL; l = l->next) {
			GDataCalendarEvent *event;

			event = GDATA_CALENDAR_EVENT (l->data);
			print_event (event);
			n_results++;
		}

		gdata_query_next_page (GDATA_QUERY (query));
		g_object_unref (feed);
	}

	g_print ("Total of %u results.\n", n_results);

done:
	g_clear_object (&query);
	g_clear_object (&authorizer);
	g_clear_object (&calendar);
	g_clear_object (&service);

	return retval;
}

/* Insert a new event into a calendar. */
static int
command_insert_event (int argc, char *argv[])
{
	GDataCalendarService *service = NULL;
	GDataCalendarCalendar *calendar = NULL;
	GDataCalendarEvent *event = NULL;
	GDataCalendarEvent *inserted_event = NULL;
	GError *error = NULL;
	gint retval = 0;
	const gchar *calendar_id, *title, *start, *end;
	GDataAuthorizer *authorizer = NULL;
	GDataGDWhen *when = NULL;
	gboolean is_date;
	gchar *start_with_time = NULL, *end_with_time = NULL;
	GTimeVal start_tv = { 0, }, end_tv = { 0, };
	gint i;

	if (argc < 7) {
		return print_usage (argv);
	}

	calendar_id = argv[2];
	title = argv[3];
	start = argv[4];
	end = argv[5];
	/* subsequent arguments are e-mail addresses of attendees,
	 * with at least one required. */

	/* Authenticate and create a service. */
	authorizer = create_authorizer (&error);

	if (error != NULL) {
		g_printerr ("%s: Error authenticating: %s\n",
		            argv[0], error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	} else if (authorizer == NULL) {
		g_printerr ("%s: User chose to abort authentication.\n",
		            argv[0]);
		retval = 1;
		goto done;
	}

	service = gdata_calendar_service_new (authorizer);
	calendar = gdata_calendar_calendar_new (calendar_id);

	/* Create the event to insert. */
	event = gdata_calendar_event_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (event), title);

	start_with_time = g_strconcat (start, "T00:00:00Z", NULL);
	end_with_time = g_strconcat (end, "T00:00:00Z", NULL);

	if (g_time_val_from_iso8601 (start, &start_tv) &&
	    g_time_val_from_iso8601 (end, &end_tv)) {
		/* Includes time. */
		is_date = FALSE;
	} else if (g_time_val_from_iso8601 (start_with_time, &start_tv) &&
	           g_time_val_from_iso8601 (end_with_time, &end_tv)) {
		/* Does not include time. */
		is_date = TRUE;
	} else {
		g_printerr ("%s: Could not parse start time ‘%s’ and end time "
		            "‘%s’ as ISO 8601.\n", argv[0], start, end);
		retval = 1;
		goto done;
	}

	when = gdata_gd_when_new (start_tv.tv_sec, end_tv.tv_sec, is_date);
	gdata_calendar_event_add_time (event, when);
	g_object_unref (when);

	for (i = 6; i < argc; i++) {
		GDataGDWho *who = NULL;
		const gchar *relation_type, *email_address;

		relation_type = GDATA_GD_WHO_EVENT_ATTENDEE;
		email_address = argv[i];

		who = gdata_gd_who_new (relation_type, NULL, email_address);
		gdata_calendar_event_add_person (event, who);
		g_object_unref (who);
	}

	/* Insert the event. */
	inserted_event = gdata_calendar_service_insert_calendar_event (service,
	                                                               calendar,
	                                                               event,
	                                                               NULL,
	                                                               &error);

	if (error != NULL) {
		g_printerr ("%s: Error inserting event: %s\n",
		            argv[0], error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	}

	/* Print results. */
	print_event (inserted_event);

done:
	g_free (start_with_time);
	g_free (end_with_time);
	g_clear_object (&inserted_event);
	g_clear_object (&event);
	g_clear_object (&authorizer);
	g_clear_object (&calendar);
	g_clear_object (&service);

	return retval;
}

static const struct {
	const gchar *command;
	int (*handler_fn) (int argc, char **argv);
} command_handlers[] = {
	{ "calendars", command_calendars },
	{ "events", command_events },
	{ "insert-event", command_insert_event },
};

int
main (int argc, char *argv[])
{
	guint i;
	gint retval = -1;

	setlocale (LC_ALL, "");

	if (argc < 2) {
		return print_usage (argv);
	}

	for (i = 0; i < G_N_ELEMENTS (command_handlers); i++) {
		if (strcmp (argv[1], command_handlers[i].command) == 0) {
			retval = command_handlers[i].handler_fn (argc, argv);
		}
	}

	if (retval == -1) {
		retval = print_usage (argv);
	}

	return retval;
}
