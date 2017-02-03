/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009, 2010, 2014, 2015, 2017 <philip@tecnocode.co.uk>
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

#include <glib.h>
#include <unistd.h>
#include <string.h>

#include "gdata.h"
#include "common.h"
#include "gdata-dummy-authorizer.h"

static UhmServer *mock_server = NULL;

#undef CLIENT_ID  /* from common.h */

#define CLIENT_ID "352818697630-nqu2cmt5quqd6lr17ouoqmb684u84l1f.apps.googleusercontent.com"
#define CLIENT_SECRET "-fA4pHQJxR3zJ-FyAMPQsikg"
#define REDIRECT_URI "urn:ietf:wg:oauth:2.0:oob"

typedef struct {
	GDataCalendarCalendar *calendar;
} TempCalendarData;

static void
set_up_temp_calendar (TempCalendarData *data, gconstpointer service)
{
	GDataCalendarCalendar *calendar;
	GDataColor colour;

	gdata_test_mock_server_start_trace (mock_server, "setup-temp-calendar");

	g_assert (gdata_color_from_hexadecimal ("#7A367A", &colour) == TRUE);

	/* Create a single temporary test calendar */
	calendar = gdata_calendar_calendar_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (calendar), "Temp Test Calendar");
	gdata_calendar_calendar_set_color (calendar, &colour);
	data->calendar = GDATA_CALENDAR_CALENDAR (gdata_service_insert_entry (GDATA_SERVICE (service),
	                                                                      gdata_calendar_service_get_primary_authorization_domain (),
	                                                                      "https://www.googleapis.com/calendar/v3/calendars",
	                                                                      GDATA_ENTRY (calendar), NULL, NULL));
	g_assert (GDATA_IS_CALENDAR_CALENDAR (data->calendar));
	g_object_unref (calendar);

	uhm_server_end_trace (mock_server);
}

static void
tear_down_temp_calendar (TempCalendarData *data, gconstpointer service)
{
	gdata_test_mock_server_start_trace (mock_server, "teardown-temp-calendar");

	/* Delete the calendar */
	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_calendar_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->calendar), NULL, NULL) == TRUE);
	g_object_unref (data->calendar);

	uhm_server_end_trace (mock_server);
}

static void
test_authentication (void)
{
	GDataOAuth2Authorizer *authorizer = NULL;  /* owned */
	gchar *authentication_uri, *authorisation_code;

	gdata_test_mock_server_start_trace (mock_server, "authentication");

	authorizer = gdata_oauth2_authorizer_new (CLIENT_ID, CLIENT_SECRET,
	                                          REDIRECT_URI,
	                                          GDATA_TYPE_CALENDAR_SERVICE);

	/* Get an authentication URI. */
	authentication_uri = gdata_oauth2_authorizer_build_authentication_uri (authorizer, NULL, FALSE);
	g_assert (authentication_uri != NULL);

	/* Get the authorisation code off the user. */
	if (uhm_server_get_enable_online (mock_server)) {
		authorisation_code = gdata_test_query_user_for_verifier (authentication_uri);
	} else {
		/* Hard coded, extracted from the trace file. */
		authorisation_code = g_strdup ("4/OEX-S1iMbOA_dOnNgUlSYmGWh3TK.QrR73axcNMkWoiIBeO6P2m_su7cwkQI");
	}

	g_free (authentication_uri);

	if (authorisation_code == NULL) {
		/* Skip tests. */
		goto skip_test;
	}

	/* Authorise the token */
	g_assert (gdata_oauth2_authorizer_request_authorization (authorizer, authorisation_code, NULL, NULL) == TRUE);

	/* Check all is as it should be */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	                                                     gdata_calendar_service_get_primary_authorization_domain ()) == TRUE);

skip_test:
	g_free (authorisation_code);
	g_object_unref (authorizer);

	uhm_server_end_trace (mock_server);
}

typedef struct {
	GDataCalendarCalendar *calendar1;
	GDataCalendarCalendar *calendar2;
} QueryCalendarsData;

static void
set_up_query_calendars (QueryCalendarsData *data, gconstpointer service)
{
	GDataCalendarCalendar *calendar;
	GDataColor colour;

	gdata_test_mock_server_start_trace (mock_server, "setup-query-calendars");

	g_assert (gdata_color_from_hexadecimal ("#7A367A", &colour) == TRUE);

	/* Create some new calendars for queries */
	calendar = gdata_calendar_calendar_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (calendar), "Test Calendar 1");
	gdata_calendar_calendar_set_color (calendar, &colour);
	data->calendar1 = GDATA_CALENDAR_CALENDAR (gdata_service_insert_entry (GDATA_SERVICE (service),
	                                                                       gdata_calendar_service_get_primary_authorization_domain (),
	                                                                       "https://www.googleapis.com/calendar/v3/calendars",
	                                                                       GDATA_ENTRY (calendar), NULL, NULL));
	g_assert (GDATA_IS_CALENDAR_CALENDAR (data->calendar1));
	g_object_unref (calendar);

	calendar = gdata_calendar_calendar_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (calendar), "Test Calendar 2");
	gdata_calendar_calendar_set_color (calendar, &colour);
	data->calendar2 = GDATA_CALENDAR_CALENDAR (gdata_service_insert_entry (GDATA_SERVICE (service),
	                                                                       gdata_calendar_service_get_primary_authorization_domain (),
	                                                                       "https://www.googleapis.com/calendar/v3/calendars",
	                                                                       GDATA_ENTRY (calendar), NULL, NULL));
	g_assert (GDATA_IS_CALENDAR_CALENDAR (data->calendar2));
	g_object_unref (calendar);

	uhm_server_end_trace (mock_server);
}

static void
tear_down_query_calendars (QueryCalendarsData *data, gconstpointer service)
{
	gdata_test_mock_server_start_trace (mock_server, "teardown-query-calendars");

	/* Delete the calendars */
	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_calendar_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->calendar1), NULL, NULL) == TRUE);
	g_object_unref (data->calendar1);

	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_calendar_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->calendar2), NULL, NULL) == TRUE);
	g_object_unref (data->calendar2);

	uhm_server_end_trace (mock_server);
}

static void
test_query_all_calendars (QueryCalendarsData *data, gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "query-all-calendars");

	feed = gdata_calendar_service_query_all_calendars (GDATA_CALENDAR_SERVICE (service), NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_FEED (feed));
	g_clear_error (&error);

	/* TODO: check entries and feed properties */

	g_object_unref (feed);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_CLOSURE_FUNCTIONS (query_calendars, QueryCalendarsData);

GDATA_ASYNC_TEST_FUNCTIONS (query_all_calendars, QueryCalendarsData,
G_STMT_START {
	gdata_calendar_service_query_all_calendars_async (GDATA_CALENDAR_SERVICE (service), NULL, cancellable, NULL,
	                                                  NULL, NULL, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataFeed *feed;

	feed = gdata_service_query_finish (GDATA_SERVICE (obj), async_result, &error);

	if (error == NULL) {
		/* TODO: Tests? */
		g_assert (GDATA_IS_CALENDAR_FEED (feed));

		g_object_unref (feed);
	} else {
		g_assert (feed == NULL);
	}
} G_STMT_END);

static void
test_query_all_calendars_async_progress_closure (QueryCalendarsData *query_data, gconstpointer service)
{
	GDataAsyncProgressClosure *data = g_slice_new0 (GDataAsyncProgressClosure);

	gdata_test_mock_server_start_trace (mock_server, "query-all-calendars-async-progress-closure");

	data->main_loop = g_main_loop_new (NULL, TRUE);

	gdata_calendar_service_query_all_calendars_async (GDATA_CALENDAR_SERVICE (service), NULL, NULL,
	                                                  (GDataQueryProgressCallback) gdata_test_async_progress_callback,
	                                                  data, (GDestroyNotify) gdata_test_async_progress_closure_free,
	                                                  (GAsyncReadyCallback) gdata_test_async_progress_finish_callback, data);

	g_main_loop_run (data->main_loop);
	g_main_loop_unref (data->main_loop);

	/* Check that both callbacks were called exactly once */
	g_assert_cmpuint (data->progress_destroy_notify_count, ==, 1);
	g_assert_cmpuint (data->async_ready_notify_count, ==, 1);

	g_slice_free (GDataAsyncProgressClosure, data);

	uhm_server_end_trace (mock_server);
}

static void
test_query_own_calendars (QueryCalendarsData *data, gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "query-own-calendars");

	feed = gdata_calendar_service_query_own_calendars (GDATA_CALENDAR_SERVICE (service), NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_FEED (feed));
	g_clear_error (&error);

	/* TODO: check entries and feed properties */

	g_object_unref (feed);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_TEST_FUNCTIONS (query_own_calendars, QueryCalendarsData,
G_STMT_START {
	gdata_calendar_service_query_own_calendars_async (GDATA_CALENDAR_SERVICE (service), NULL, cancellable, NULL,
	                                                  NULL, NULL, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataFeed *feed;

	feed = gdata_service_query_finish (GDATA_SERVICE (obj), async_result, &error);

	if (error == NULL) {
		g_assert (GDATA_IS_CALENDAR_FEED (feed));
		/* TODO: Tests? */

		g_object_unref (feed);
	} else {
		g_assert (feed == NULL);
	}
} G_STMT_END);

static void
test_query_own_calendars_async_progress_closure (QueryCalendarsData *query_data, gconstpointer service)
{
	GDataAsyncProgressClosure *data = g_slice_new0 (GDataAsyncProgressClosure);

	gdata_test_mock_server_start_trace (mock_server, "query-own-calendars-async-progress-closure");

	data->main_loop = g_main_loop_new (NULL, TRUE);

	gdata_calendar_service_query_own_calendars_async (GDATA_CALENDAR_SERVICE (service), NULL, NULL,
	                                               (GDataQueryProgressCallback) gdata_test_async_progress_callback,
	                                               data, (GDestroyNotify) gdata_test_async_progress_closure_free,
	                                               (GAsyncReadyCallback) gdata_test_async_progress_finish_callback, data);

	g_main_loop_run (data->main_loop);
	g_main_loop_unref (data->main_loop);

	/* Check that both callbacks were called exactly once */
	g_assert_cmpuint (data->progress_destroy_notify_count, ==, 1);
	g_assert_cmpuint (data->async_ready_notify_count, ==, 1);

	g_slice_free (GDataAsyncProgressClosure, data);

	uhm_server_end_trace (mock_server);
}

typedef struct {
	TempCalendarData parent;
	GDataCalendarEvent *event1;
	GDataCalendarEvent *event2;
	GDataCalendarEvent *event3;
} QueryEventsData;

static void
set_up_query_events (QueryEventsData *data, gconstpointer service)
{
	GDataCalendarEvent *event;
	GDataGDWhen *when;
	GError *error = NULL;

	/* Set up a temporary calendar */
	set_up_temp_calendar ((TempCalendarData*) data, service);

	gdata_test_mock_server_start_trace (mock_server, "setup-query-events");

	/* Add some test events to it */
	event = gdata_calendar_event_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (event), "Test Event 1");

	when = gdata_gd_when_new (1419113727, 1419113728, FALSE);
	gdata_calendar_event_add_time (event, when);
	g_object_unref (when);

	data->event1 = gdata_calendar_service_insert_calendar_event (GDATA_CALENDAR_SERVICE (service), data->parent.calendar, event, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_EVENT (data->event1));
	g_object_unref (event);

	event = gdata_calendar_event_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (event), "Test Event 2");

	when = gdata_gd_when_new (1419113000, 1419114000, FALSE);
	gdata_calendar_event_add_time (event, when);
	g_object_unref (when);

	data->event2 = gdata_calendar_service_insert_calendar_event (GDATA_CALENDAR_SERVICE (service), data->parent.calendar, event, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_EVENT (data->event2));
	g_object_unref (event);

	event = gdata_calendar_event_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (event), "Test Event 3");

	when = gdata_gd_when_new (1419110000, 1419120000, TRUE);
	gdata_calendar_event_add_time (event, when);
	g_object_unref (when);

	data->event3 = gdata_calendar_service_insert_calendar_event (GDATA_CALENDAR_SERVICE (service), data->parent.calendar, event, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_EVENT (data->event3));
	g_object_unref (event);

	uhm_server_end_trace (mock_server);
}

static void
tear_down_query_events (QueryEventsData *data, gconstpointer service)
{
	gdata_test_mock_server_start_trace (mock_server, "teardown-query-events");

	/* Delete the events */
	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_calendar_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->event1), NULL, NULL) == TRUE);
	g_object_unref (data->event1);

	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_calendar_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->event2), NULL, NULL) == TRUE);
	g_object_unref (data->event2);

	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_calendar_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->event3), NULL, NULL) == TRUE);
	g_object_unref (data->event3);

	uhm_server_end_trace (mock_server);

	/* Delete the calendar */
	tear_down_temp_calendar ((TempCalendarData*) data, service);
}

static void
test_query_events (QueryEventsData *data, gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "query-events");

	/* Get the entry feed */
	feed = gdata_calendar_service_query_events (GDATA_CALENDAR_SERVICE (service), data->parent.calendar, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_FEED (feed));
	g_clear_error (&error);

	/* TODO: check entries and feed properties */

	g_object_unref (feed);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_CLOSURE_FUNCTIONS (query_events, QueryEventsData);

GDATA_ASYNC_TEST_FUNCTIONS (query_events, QueryEventsData,
G_STMT_START {
	gdata_calendar_service_query_events_async (GDATA_CALENDAR_SERVICE (service), data->parent.calendar, NULL, cancellable, NULL, NULL, NULL,
	                                           async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataFeed *feed;

	feed = gdata_service_query_finish (GDATA_SERVICE (obj), async_result, &error);

	if (error == NULL) {
		g_assert (GDATA_IS_CALENDAR_FEED (feed));

		g_object_unref (feed);
	} else {
		g_assert (feed == NULL);
	}
} G_STMT_END);

static void
test_query_events_async_progress_closure (QueryEventsData *query_data, gconstpointer service)
{
	GDataAsyncProgressClosure *data = g_slice_new0 (GDataAsyncProgressClosure);

	gdata_test_mock_server_start_trace (mock_server, "query-events-async-progress-closure");

	data->main_loop = g_main_loop_new (NULL, TRUE);

	gdata_calendar_service_query_events_async (GDATA_CALENDAR_SERVICE (service), query_data->parent.calendar, NULL, NULL,
	                                           (GDataQueryProgressCallback) gdata_test_async_progress_callback,
	                                           data, (GDestroyNotify) gdata_test_async_progress_closure_free,
	                                           (GAsyncReadyCallback) gdata_test_async_progress_finish_callback, data);

	g_main_loop_run (data->main_loop);
	g_main_loop_unref (data->main_loop);

	/* Check that both callbacks were called exactly once */
	g_assert_cmpuint (data->progress_destroy_notify_count, ==, 1);
	g_assert_cmpuint (data->async_ready_notify_count, ==, 1);

	g_slice_free (GDataAsyncProgressClosure, data);

	uhm_server_end_trace (mock_server);
}

typedef struct {
	TempCalendarData parent;
	GDataCalendarEvent *new_event;
} InsertEventData;

static void
set_up_insert_event (InsertEventData *data, gconstpointer service)
{
	set_up_temp_calendar ((TempCalendarData*) data, service);
	data->new_event = NULL;
}

static void
tear_down_insert_event (InsertEventData *data, gconstpointer service)
{
	gdata_test_mock_server_start_trace (mock_server, "teardown-insert-event");

	/* Delete the new event */
	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_calendar_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->new_event), NULL, NULL) == TRUE);
	g_object_unref (data->new_event);

	uhm_server_end_trace (mock_server);

	/* Delete the calendar too */
	tear_down_temp_calendar ((TempCalendarData*) data, service);
}

static void
test_event_insert (InsertEventData *data, gconstpointer service)
{
	GDataCalendarEvent *event, *new_event;
	GDataGDWhere *where;
	GDataGDWho *who;
	GDataGDWhen *when;
	GTimeVal start_time, end_time;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "event-insert");

	event = gdata_calendar_event_new (NULL);

	gdata_entry_set_title (GDATA_ENTRY (event), "Tennis with Beth");
	gdata_entry_set_content (GDATA_ENTRY (event), "Meet for a quick lesson.");
	gdata_calendar_event_set_transparency (event, GDATA_GD_EVENT_TRANSPARENCY_OPAQUE);
	gdata_calendar_event_set_status (event, GDATA_GD_EVENT_STATUS_CONFIRMED);
	where = gdata_gd_where_new (NULL, "Rolling Lawn Courts", NULL);
	gdata_calendar_event_add_place (event, where);
	g_object_unref (where);
	who = gdata_gd_who_new (GDATA_GD_WHO_EVENT_ORGANIZER, "John Smith‽", "john.smith@example.com");
	gdata_calendar_event_add_person (event, who);
	g_object_unref (who);
	g_time_val_from_iso8601 ("2009-04-17T15:00:00.000Z", &start_time);
	g_time_val_from_iso8601 ("2009-04-17T17:00:00.000Z", &end_time);
	when = gdata_gd_when_new (start_time.tv_sec, end_time.tv_sec, FALSE);
	gdata_calendar_event_add_time (event, when);
	g_object_unref (when);

	/* Insert the event */
	new_event = data->new_event = gdata_calendar_service_insert_calendar_event (GDATA_CALENDAR_SERVICE (service),
	                                                                            data->parent.calendar, event, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_EVENT (new_event));
	g_clear_error (&error);

	/* TODO: check entries and feed properties */

	g_object_unref (event);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_CLOSURE_FUNCTIONS (insert_event, InsertEventData);

GDATA_ASYNC_TEST_FUNCTIONS (event_insert, InsertEventData,
G_STMT_START {
	GDataCalendarEvent *event;
	GDataGDWhere *where;
	GDataGDWho *who;
	GDataGDWhen *when;
	GTimeVal start_time;
	GTimeVal end_time;

	event = gdata_calendar_event_new (NULL);

	gdata_entry_set_title (GDATA_ENTRY (event), "Tennis with Beth");
	gdata_entry_set_content (GDATA_ENTRY (event), "Meet for a quick lesson.");
	gdata_calendar_event_set_transparency (event, GDATA_GD_EVENT_TRANSPARENCY_OPAQUE);
	gdata_calendar_event_set_status (event, GDATA_GD_EVENT_STATUS_CONFIRMED);
	where = gdata_gd_where_new (NULL, "Rolling Lawn Courts", NULL);
	gdata_calendar_event_add_place (event, where);
	g_object_unref (where);
	who = gdata_gd_who_new (GDATA_GD_WHO_EVENT_ORGANIZER, "John Smith‽", "john.smith@example.com");
	gdata_calendar_event_add_person (event, who);
	g_object_unref (who);
	g_time_val_from_iso8601 ("2009-04-17T15:00:00.000Z", &start_time);
	g_time_val_from_iso8601 ("2009-04-17T17:00:00.000Z", &end_time);
	when = gdata_gd_when_new (start_time.tv_sec, end_time.tv_sec, FALSE);
	gdata_calendar_event_add_time (event, when);
	g_object_unref (when);

	/* Insert the event */
	gdata_calendar_service_insert_calendar_event_async (GDATA_CALENDAR_SERVICE (service),
	                                                    data->parent.calendar, event, cancellable,
	                                                    async_ready_callback, async_data);

	g_object_unref (event);
} G_STMT_END,
G_STMT_START {
	GDataEntry *event;
	event = gdata_service_insert_entry_finish (GDATA_SERVICE (obj), async_result, &error);
	if (error == NULL) {
		g_assert (GDATA_IS_CALENDAR_EVENT (event));
		data->new_event = GDATA_CALENDAR_EVENT (event);
		g_assert_cmpstr (gdata_entry_get_title (event), ==, "Tennis with Beth");
	} else {
		g_assert (event == NULL);
	}
} G_STMT_END);

static void
test_event_json (void)
{
	GDataCalendarEvent *event;
	GDataGDWhere *where;
	GDataGDWho *who;
	GDataGDWhen *when;
	GTimeVal start_time, end_time;

	event = gdata_calendar_event_new (NULL);

	gdata_entry_set_title (GDATA_ENTRY (event), "Tennis with Beth");
	gdata_entry_set_content (GDATA_ENTRY (event), "Meet for a quick lesson.");
	gdata_calendar_event_set_transparency (event, GDATA_GD_EVENT_TRANSPARENCY_OPAQUE);
	gdata_calendar_event_set_status (event, GDATA_GD_EVENT_STATUS_CONFIRMED);
	where = gdata_gd_where_new (NULL, "Rolling Lawn Courts", NULL);
	gdata_calendar_event_add_place (event, where);
	g_object_unref (where);
	who = gdata_gd_who_new (GDATA_GD_WHO_EVENT_ORGANIZER, "John Smith‽", "john.smith@example.com");
	gdata_calendar_event_add_person (event, who);
	g_object_unref (who);
	g_time_val_from_iso8601 ("2009-04-17T15:00:00.000Z", &start_time);
	g_time_val_from_iso8601 ("2009-04-17T17:00:00.000Z", &end_time);
	when = gdata_gd_when_new (start_time.tv_sec, end_time.tv_sec, FALSE);
	gdata_calendar_event_add_time (event, when);
	g_object_unref (when);

	/* Check the JSON */
	gdata_test_assert_json (event, "{"
		"'summary': 'Tennis with Beth',"
		"'description': 'Meet for a quick lesson.',"
		"'kind': 'calendar#event',"
		"'status': 'confirmed',"
		"'transparency': 'opaque',"
		"'guestsCanModify': false,"
		"'guestsCanInviteOthers': false,"
		"'guestsCanSeeOtherGuests': false,"
		"'anyoneCanAddSelf': false,"
		"'start': {"
			"'dateTime': '2009-04-17T15:00:00Z',"
			"'timeZone': 'UTC'"
		"},"
		"'end': {"
			"'dateTime': '2009-04-17T17:00:00Z',"
			"'timeZone': 'UTC'"
		"},"
		"'attendees': ["
			"{"
				"'email': 'john.smith@example.com',"
				"'displayName': 'John Smith‽',"
				"'organizer': true"
			"}"
		"],"
		"'organizer': {"
			"'email': 'john.smith@example.com',"
			"'displayName': 'John Smith‽'"
		"},"
		"'location': 'Rolling Lawn Courts'"
	"}");
}

static void
test_event_json_attendees (void)
{
	GDataCalendarEvent *event;
	GList/*<unowned GDataGDWho>*/ *l;
	guint n_people;
	const struct {
		const gchar *relation_type;
		const gchar *value_string;
		const gchar *email_address;
	} expected[] = {
		{ GDATA_GD_WHO_EVENT_ATTENDEE, "Joe Hibbs", "person1@gmail.com" },
		{ GDATA_GD_WHO_EVENT_ATTENDEE, "Me McMeeson", "me@gmail.com" },
		{ GDATA_GD_WHO_EVENT_ATTENDEE, NULL, "person2@gmail.com" },
		{ GDATA_GD_WHO_EVENT_ATTENDEE, "Example Person 3", "person3@gmail.com" },
		{ GDATA_GD_WHO_EVENT_ATTENDEE, NULL, "person4@gmail.com" },
		{ GDATA_GD_WHO_EVENT_ORGANIZER, "Ruth Pettut", "blah@example.com" },
	};
	GError *error = NULL;

	event = GDATA_CALENDAR_EVENT (gdata_parsable_new_from_json (GDATA_TYPE_CALENDAR_EVENT, "{"
		"'kind': 'calendar#event',"
		"'id': 'some-id',"
		"'created': '2017-02-04T17:53:47.000Z',"
		"'summary': 'Duff this',"
		"'organizer': {"
			"'email': 'blah@example.com',"
			"'displayName': 'Ruth Pettut'"
		"},"
		"'attendees': ["
			"{"
				"'email': 'person1@gmail.com',"
				"'displayName': 'Joe Hibbs',"
				"'responseStatus': 'accepted'"
			"},"
			"{"
				"'email': 'me@gmail.com',"
				"'displayName': 'Me McMeeson',"
				"'self': true,"
				"'responseStatus': 'needsAction'"
			"},"
			"{"
				"'email': 'person2@gmail.com',"
				"'responseStatus': 'needsAction'"
			"},"
			"{"
				"'email': 'person3@gmail.com',"
				"'displayName': 'Example Person 3',"
				"'responseStatus': 'tentative',"
				"'comment': 'Some poor excuse about not coming.'"
			"},"
			"{"
				"'email': 'person4@gmail.com',"
				"'responseStatus': 'accepted'"
			"},"
			"{"
				"'email': 'blah@example.com',"
				"'displayName': 'Ruth Pettut',"
				"'organizer': true,"
				"'responseStatus': 'accepted'"
			"}"
		"]"
	"}", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_ENTRY (event));
	g_clear_error (&error);

	/* Check attendee details. */
	for (n_people = 0, l = gdata_calendar_event_get_people (event);
	     l != NULL;
	     n_people += 1, l = l->next) {
		GDataGDWho *who = GDATA_GD_WHO (l->data);
		gsize i;

		g_test_message ("Examining attendee: %s",
		                gdata_gd_who_get_email_address (who));

		for (i = 0; i < G_N_ELEMENTS (expected); i++) {
			if (g_strcmp0 (gdata_gd_who_get_email_address (who),
			               expected[i].email_address) == 0) {
				g_assert_cmpstr (gdata_gd_who_get_relation_type (who), ==, expected[i].relation_type);
				g_assert_cmpstr (gdata_gd_who_get_value_string (who), ==, expected[i].value_string);
				break;
			}
		}

		g_assert_cmpuint (i, <, G_N_ELEMENTS (expected));
	}

	g_assert_cmpuint (n_people, ==, G_N_ELEMENTS (expected));

	g_object_unref (event);
}

static void
test_event_json_dates (void)
{
	guint i;

	const struct {
		const gchar *json;
		gboolean is_date;
		gint64 start_time;
		gint64 end_time;
		const gchar *output_json;  /* NULL if equal to @json */
	} test_vectors[] = {
		/* Plain date, single day. */
		{ "'start': {"
			"'date': '2009-04-17',"
			"'timeZone': 'UTC'"
		"},"
		"'end': {"
			"'date': '2009-04-18',"
			"'timeZone': 'UTC'"
		"}", TRUE, 1239926400, 1239926400 + 86400, NULL },
		/* Full date and time. */
		{ "'start': {"
			"'dateTime': '2009-04-17T15:00:00Z',"
			"'timeZone': 'UTC'"
		"},"
		"'end': {"
			"'dateTime': '2009-04-17T16:00:00Z',"
			"'timeZone': 'UTC'"
		"}", FALSE, 1239926400 + 54000, 1239926400 + 54000 + 3600, NULL },
		/* Start and end time. */
		{ "'start': {"
			"'date': '2009-04-27',"
			"'timeZone': 'UTC'"
		"},"
		"'end': {"
			"'date': '20090506',"
			"'timeZone': 'UTC'"
		"}", TRUE, 1239926400 + 864000, 1241568000, "'start': {"
			"'date': '2009-04-27',"
			"'timeZone': 'UTC'"
		"},"
		"'end': {"
			"'date': '2009-05-06',"
			"'timeZone': 'UTC'"
		"}" },
	};

	for (i = 0; i < G_N_ELEMENTS (test_vectors); i++) {
		gchar *json = NULL, *output_json = NULL;  /* owned */
		GDataCalendarEvent *event;
		GList *j;
		GDataGDWhen *when;
		gint64 _time;
		GError *error = NULL;

		json = g_strdup_printf ("{"
			"'summary': 'Tennis with Beth',"
			"'description': 'Meet for a quick lesson.',"
			"'kind': 'calendar#event',"
			"%s"
		"}", test_vectors[i].json);
		output_json = g_strdup_printf ("{"
			"'summary': 'Tennis with Beth',"
			"'description': 'Meet for a quick lesson.',"
			"'kind': 'calendar#event',"
			"'guestsCanModify': false,"
			"'guestsCanInviteOthers': false,"
			"'guestsCanSeeOtherGuests': false,"
			"'anyoneCanAddSelf': false,"
			"'attendees': [],"
			"%s"
		"}", (test_vectors[i].output_json != NULL) ? test_vectors[i].output_json : test_vectors[i].json);

		event = GDATA_CALENDAR_EVENT (gdata_parsable_new_from_json (GDATA_TYPE_CALENDAR_EVENT, json, -1, &error));
		g_assert_no_error (error);
		g_assert (GDATA_IS_ENTRY (event));
		g_clear_error (&error);

		/* Check the times */
		j = gdata_calendar_event_get_times (event);
		g_assert (j != NULL);

		when = GDATA_GD_WHEN (j->data);
		g_assert (gdata_gd_when_is_date (when) == test_vectors[i].is_date);
		_time = gdata_gd_when_get_start_time (when);
		g_assert_cmpint (_time, ==, test_vectors[i].start_time);
		_time = gdata_gd_when_get_end_time (when);
		g_assert_cmpint (_time, ==, test_vectors[i].end_time);
		g_assert (gdata_gd_when_get_value_string (when) == NULL);
		g_assert (gdata_gd_when_get_reminders (when) == NULL);

		/* Should be no other times. */
		g_assert (j->next == NULL);

		/* Check the JSON */
		gdata_test_assert_json (event, output_json);

		g_object_unref (event);
		g_free (output_json);
		g_free (json);
	}
}

static void
test_event_json_organizer (void)
{
	GDataCalendarEvent *event;
	GError *error = NULL;

	event = GDATA_CALENDAR_EVENT (gdata_parsable_new_from_json (GDATA_TYPE_CALENDAR_EVENT, "{"
		"'kind': 'calendar#event',"
		"'id': 'some-id',"
		"'created': '2013-12-22T18:00:00.000Z',"
		"'summary': 'FOSDEM GNOME Beer Event',"
		"'organizer': {"
			"'id': 'another-id',"
			"'displayName': 'Guillaume Desmottes'"
		"},"
		"'attendees': ["
			"{"
				"'id': 'another-id',"
				"'displayName': 'Guillaume Desmottes',"
				"'organizer': true,"
				"'responseStatus': 'accepted'"
			"}"
		"]"
	"}", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_ENTRY (event));
	g_clear_error (&error);

	g_object_unref (event);
}

static void
test_event_json_recurrence (void)
{
	GDataCalendarEvent *event;
	GError *error = NULL;
	gchar *id, *uri;

	event = GDATA_CALENDAR_EVENT (gdata_parsable_new_from_json (GDATA_TYPE_CALENDAR_EVENT, "{"
		"'id': 'https://www.googleapis.com/calendar/v3/calendars/libgdata.test@googlemail.com/events/g5928e82rrch95b25f8ud0dlsg_20090429T153000Z',"
		"'updated': '2009-04-27T17:54:10.000Z',"
		"'summary': 'Test daily instance event',"
		"'kind': 'calendar#event',"
		"'creator': {"
			"'displayName': 'GData Test',"
			"'email': 'libgdata.test@googlemail.com'"
		"},"
		"'recurringEventId': 'g5928e82rrch95b25f8ud0dlsg',"
		"'originalStartTime': {"
			"'dateTime': '2009-04-29T16:30:00.000+01:00',"
			"'timeZone': 'UTC'"
		"},"
		"'guestsCanModify': false,"
		"'guestsCanInviteOthers': false,"
		"'guestsCanSeeOtherGuests': false,"
		"'anyoneCanAddSelf': false,"
		"'status': 'confirmed',"
		"'visibility': 'private',"
		"'transparency': 'opaque',"
		"'iCalUID': 'g5928e82rrch95b25f8ud0dlsg@google.com',"
		"'sequence': '0',"
		"'start': {"
			"'dateTime': '2009-04-29T17:30:00.000+01:00',"
			"'timeZone': 'UTC'"
		"},"
		"'end': {"
			"'dateTime': '2009-04-29T17:30:00.000+01:00',"
			"'timeZone': 'UTC'"
		"},"
		"'reminders': {"
			"'overrides': [{"
				"'method': 'email',"
				"'minutes': 10"
			"}, {"
				"'method': 'popup',"
				"'minutes': 10"
			"}]"
		"},"
		"'attendees': ["
			"{"
				"'email': 'libgdata.test@googlemail.com',"
				"'displayName': 'GData Test',"
				"'organizer': true,"
				"'responseStatus': 'needsAction'"
			"}"
		"],"
		"'organizer': {"
			"'email': 'libgdata.test@googlemail.com',"
			"'displayName': 'GData Test'"
		"}"
	"}", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_ENTRY (event));
	g_clear_error (&error);

	/* Check the original event */
	g_assert (gdata_calendar_event_is_exception (event) == TRUE);

	gdata_calendar_event_get_original_event_details (event, &id, &uri);
	g_assert_cmpstr (id, ==, "g5928e82rrch95b25f8ud0dlsg");
	g_assert_cmpstr (uri, ==, "https://www.googleapis.com/calendar/v3/events/g5928e82rrch95b25f8ud0dlsg");

	g_free (id);
	g_free (uri);
	g_object_unref (event);
}

static void
test_calendar_escaping (void)
{
	GDataCalendarCalendar *calendar;

	calendar = gdata_calendar_calendar_new (NULL);
	gdata_calendar_calendar_set_timezone (calendar, "<timezone>");

	/* Check the outputted JSON is escaped properly */
	gdata_test_assert_json (calendar, "{"
		"'kind': 'calendar#calendar',"
		"'timeZone': '<timezone>',"
		"'hidden': false,"
		"'backgroundColor': '#000000',"
		"'selected': false"
	"}");
	g_object_unref (calendar);
}

static void
test_event_escaping (void)
{
	GDataCalendarEvent *event;

	event = gdata_calendar_event_new (NULL);
	gdata_calendar_event_set_status (event, "\"status\"");
	gdata_calendar_event_set_visibility (event, "\"visibility\"");
	gdata_calendar_event_set_transparency (event, "\"transparency\"");
	gdata_calendar_event_set_uid (event, "\"uid\"");
	gdata_calendar_event_set_recurrence (event, "\"recurrence\"");

	/* Check the outputted JSON is escaped properly */
	gdata_test_assert_json (event, "{"
		"'kind': 'calendar#event',"
		"'status': '\"status\"',"
		"'transparency': '\"transparency\"',"
		"'visibility': '\"visibility\"',"
		"'iCalUID': '\"uid\"',"
		"'recurrence': [ '\"recurrence\"' ],"
		"'guestsCanModify': false,"
		"'guestsCanInviteOthers': false,"
		"'guestsCanSeeOtherGuests': false,"
		"'anyoneCanAddSelf': false,"
		"'attendees': []"
	"}");
	g_object_unref (event);
}

/* Test the event parser with the minimal number of properties specified. */
static void
test_calendar_event_parser_minimal (void)
{
	GDataCalendarEvent *event = NULL;  /* owned */
	GDataEntry *entry;  /* unowned */
	GError *error = NULL;

	event = GDATA_CALENDAR_EVENT (gdata_parsable_new_from_json (GDATA_TYPE_CALENDAR_EVENT,
		"{"
			"\"kind\": \"calendar#event\","
			"\"etag\": \"\\\"2838230136828000\\\"\","
			"\"id\": \"hsfgtc50u68vdai81t6634u7lg\","
			"\"status\": \"confirmed\","
			"\"htmlLink\": \"https://www.google.com/calendar/event?eid=aHNmZ3RjNTB1Njh2ZGFpODF0NjYzNHU3bGcgODk5MWkzNjM0YzRzN3Nwa3NrcjNjZjVuanNAZw\","
			"\"created\": \"2014-12-20T22:37:48.000Z\","
			"\"updated\": \"2014-12-20T22:37:48.414Z\","
			"\"summary\": \"Test Event 1\","
			"\"creator\": {"
				"\"email\": \"libgdata.test@googlemail.com\","
				"\"displayName\": \"GData Test\""
			"},"
			"\"organizer\": {"
				"\"email\": \"8991i3634c4s7spkskr3cf5njs@group.calendar.google.com\","
				"\"displayName\": \"Temp Test Calendar\","
				"\"self\": true"
			"},"
			"\"start\": {"
				"\"dateTime\": \"2014-12-20T22:15:27Z\","
				"\"timeZone\": \"UTC\""
			"},"
			"\"end\": {"
				"\"dateTime\": \"2014-12-20T22:15:28Z\","
				"\"timeZone\": \"UTC\""
			"},"
			"\"iCalUID\": \"hsfgtc50u68vdai81t6634u7lg@google.com\","
			"\"sequence\": 0,"
			"\"guestsCanInviteOthers\": false,"
			"\"guestsCanSeeOtherGuests\": false,"
			"\"reminders\": {"
				"\"useDefault\": true"
			"}"
		"}", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_EVENT (event));
	gdata_test_compare_kind (GDATA_ENTRY (event), "calendar#event", NULL);

	entry = GDATA_ENTRY (event);

	/* Check the event’s properties. */
	g_assert_cmpstr (gdata_entry_get_id (entry), ==,
	                 "hsfgtc50u68vdai81t6634u7lg");
	g_assert_cmpstr (gdata_entry_get_etag (entry), ==,
	                 "\"2838230136828000\"");
	g_assert_cmpstr (gdata_entry_get_title (entry), ==,
	                 "Test Event 1");
	g_assert_cmpint (gdata_entry_get_updated (entry), ==, 1419115068);

	/* TODO: check everything else */

	g_object_unref (event);
}

static void
test_access_rule_properties (void)
{
	GDataCalendarAccessRule *rule;
	const gchar *scope_type, *scope_value;

	rule = gdata_calendar_access_rule_new (NULL);

	gdata_access_rule_set_role (GDATA_ACCESS_RULE (rule), GDATA_CALENDAR_ACCESS_ROLE_EDITOR);
	g_assert_cmpstr (gdata_access_rule_get_role (GDATA_ACCESS_RULE (rule)), ==, GDATA_CALENDAR_ACCESS_ROLE_EDITOR);

	gdata_access_rule_set_scope (GDATA_ACCESS_RULE (rule), GDATA_ACCESS_SCOPE_USER, "darcy@gmail.com");
	gdata_access_rule_get_scope (GDATA_ACCESS_RULE (rule), &scope_type, &scope_value);
	g_assert_cmpstr (scope_type, ==, GDATA_ACCESS_SCOPE_USER);
	g_assert_cmpstr (scope_value, ==, "darcy@gmail.com");
}

static void
test_access_rule_json (void)
{
	GDataCalendarAccessRule *rule;

	rule = gdata_calendar_access_rule_new (NULL);

	gdata_access_rule_set_role (GDATA_ACCESS_RULE (rule), GDATA_CALENDAR_ACCESS_ROLE_EDITOR);
	gdata_access_rule_set_scope (GDATA_ACCESS_RULE (rule), GDATA_ACCESS_SCOPE_USER, "darcy@gmail.com");

	/* Check the JSON */
	gdata_test_assert_json (rule, "{"
		"'kind': 'calendar#aclRule',"
		"'role': 'writer',"
		"'scope': {"
			"'type': 'user',"
			"'value': 'darcy@gmail.com'"
		"}"
	"}");
}

static void
test_query_uri (void)
{
	gint64 _time;
	GTimeVal time_val;
	gchar *query_uri;
	GDataCalendarQuery *query = gdata_calendar_query_new ("q");

	/* Set to false or it will override our startMin below. */
	gdata_calendar_query_set_future_events (query, FALSE);
	g_assert (gdata_calendar_query_get_future_events (query) == FALSE);

	gdata_calendar_query_set_order_by (query, "starttime");
	g_assert_cmpstr (gdata_calendar_query_get_order_by (query), ==, "starttime");

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	g_time_val_from_iso8601 ("2009-04-17T15:00:00.000Z", &time_val);
	gdata_calendar_query_set_recurrence_expansion_start (query, time_val.tv_sec);
	_time = gdata_calendar_query_get_recurrence_expansion_start (query);
	g_assert_cmpint (_time, ==, time_val.tv_sec);

	g_time_val_from_iso8601 ("2010-04-17T15:00:00.000Z", &time_val);
	gdata_calendar_query_set_recurrence_expansion_end (query, time_val.tv_sec);
	_time = gdata_calendar_query_get_recurrence_expansion_end (query);
	g_assert_cmpint (_time, ==, time_val.tv_sec);
G_GNUC_END_IGNORE_DEPRECATIONS

	gdata_calendar_query_set_single_events (query, TRUE);
	g_assert (gdata_calendar_query_get_single_events (query) == TRUE);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	gdata_calendar_query_set_sort_order (query, "descending");
	g_assert_cmpstr (gdata_calendar_query_get_sort_order (query), ==, "descending");
G_GNUC_END_IGNORE_DEPRECATIONS

	g_time_val_from_iso8601 ("2009-04-17T15:00:00.000Z", &time_val);
	gdata_calendar_query_set_start_min (query, time_val.tv_sec);
	_time = gdata_calendar_query_get_start_min (query);
	g_assert_cmpint (_time, ==, time_val.tv_sec);

	g_time_val_from_iso8601 ("2010-04-17T15:00:00.000Z", &time_val);
	gdata_calendar_query_set_start_max (query, time_val.tv_sec);
	_time = gdata_calendar_query_get_start_max (query);
	g_assert_cmpint (_time, ==, time_val.tv_sec);

	gdata_calendar_query_set_timezone (query, "America/Los Angeles");
	g_assert_cmpstr (gdata_calendar_query_get_timezone (query), ==, "America/Los_Angeles");

	gdata_calendar_query_set_max_attendees (query, 15);
	g_assert_cmpuint (gdata_calendar_query_get_max_attendees (query), ==, 15);

	gdata_calendar_query_set_show_deleted (query, TRUE);
	g_assert (gdata_calendar_query_show_deleted (query) == TRUE);

	/* Check the built query URI with a normal feed URI */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&orderBy=startTime&singleEvents=true"
			                "&timeMin=2009-04-17T15:00:00Z&timeMax=2010-04-17T15:00:00Z&timeZone=America%2FLos_Angeles&maxAttendees=15"
			                "&showDeleted=true");
	g_free (query_uri);

	/* …with a feed URI with a trailing slash */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com/");
	g_assert_cmpstr (query_uri, ==, "http://example.com/?q=q&orderBy=startTime&singleEvents=true"
	                                "&timeMin=2009-04-17T15:00:00Z&timeMax=2010-04-17T15:00:00Z&timeZone=America%2FLos_Angeles&maxAttendees=15"
	                                "&showDeleted=true");
	g_free (query_uri);

	/* …with a feed URI with pre-existing arguments */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com/bar/?test=test&this=that");
	g_assert_cmpstr (query_uri, ==, "http://example.com/bar/?test=test&this=that&q=q&orderBy=startTime"
	                                "&singleEvents=true&timeMin=2009-04-17T15:00:00Z&timeMax=2010-04-17T15:00:00Z"
	                                "&timeZone=America%2FLos_Angeles&maxAttendees=15&showDeleted=true");
	g_free (query_uri);

	g_object_unref (query);
}

static void
test_query_etag (void)
{
	GDataCalendarQuery *query = gdata_calendar_query_new (NULL);

	/* Test that setting any property will unset the ETag */
	g_test_bug ("613529");

#define CHECK_ETAG(C) \
	gdata_query_set_etag (GDATA_QUERY (query), "foobar"); \
	(C); \
	g_assert (gdata_query_get_etag (GDATA_QUERY (query)) == NULL);

	CHECK_ETAG (gdata_calendar_query_set_future_events (query, FALSE))
	CHECK_ETAG (gdata_calendar_query_set_order_by (query, "shizzle"))
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	CHECK_ETAG (gdata_calendar_query_set_recurrence_expansion_start (query, -1))
	CHECK_ETAG (gdata_calendar_query_set_recurrence_expansion_end (query, -1))
G_GNUC_END_IGNORE_DEPRECATIONS
	CHECK_ETAG (gdata_calendar_query_set_single_events (query, FALSE))
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	CHECK_ETAG (gdata_calendar_query_set_sort_order (query, "shizzle"))
G_GNUC_END_IGNORE_DEPRECATIONS
	CHECK_ETAG (gdata_calendar_query_set_start_min (query, -1))
	CHECK_ETAG (gdata_calendar_query_set_start_max (query, -1))
	CHECK_ETAG (gdata_calendar_query_set_timezone (query, "about now"))
	CHECK_ETAG (gdata_calendar_query_set_max_attendees (query, 10))
	CHECK_ETAG (gdata_calendar_query_set_show_deleted (query, TRUE))

#undef CHECK_ETAG

	g_object_unref (query);
}

typedef struct {
	TempCalendarData parent;
	GDataCalendarAccessRule *rule;
} TempCalendarAclsData;

static void
calendar_access_rule_set_self_link (GDataCalendarCalendar *parent_calendar,
                                    GDataCalendarAccessRule *rule)
{
	GDataLink *_link = NULL;  /* owned */
	const gchar *calendar_id, *id;
	gchar *uri = NULL;  /* owned */

	/* FIXME: Horrendous hack to set the self link, which is needed for
	 * gdata_service_delete_entry(). Unfortunately, it needs the
	 * ACL ID _and_ the calendar ID.
	 *
	 * Do _not_ copy this code. It needs to be fixed architecturally in
	 * libgdata. */
	calendar_id = gdata_entry_get_id (GDATA_ENTRY (parent_calendar));
	id = gdata_entry_get_id (GDATA_ENTRY (rule));
	uri = g_strconcat ("https://www.googleapis.com"
	                   "/calendar/v3/calendars/",
	                   calendar_id, "/acl/", id, NULL);
	_link = gdata_link_new (uri, GDATA_LINK_SELF);
	gdata_entry_add_link (GDATA_ENTRY (rule), _link);
	g_object_unref (_link);
	g_free (uri);
}

static void
set_up_temp_calendar_acls (TempCalendarAclsData *data, gconstpointer service)
{
	GDataCalendarAccessRule *rule;
	GDataLink *_link;
	GError *error = NULL;

	/* Set up a calendar */
	set_up_temp_calendar ((TempCalendarData*) data, service);

	gdata_test_mock_server_start_trace (mock_server, "setup-temp-calendar-acls");

	/* Add an access rule to the calendar */
	rule = gdata_calendar_access_rule_new (NULL);

	gdata_access_rule_set_role (GDATA_ACCESS_RULE (rule), GDATA_CALENDAR_ACCESS_ROLE_EDITOR);
	gdata_access_rule_set_scope (GDATA_ACCESS_RULE (rule), GDATA_ACCESS_SCOPE_USER, "darcy@gmail.com");

	/* Insert the rule */
	_link = gdata_entry_look_up_link (GDATA_ENTRY (data->parent.calendar), GDATA_LINK_ACCESS_CONTROL_LIST);
	g_assert (_link != NULL);

	data->rule = GDATA_CALENDAR_ACCESS_RULE (gdata_service_insert_entry (GDATA_SERVICE (service),
	                                                                     gdata_calendar_service_get_primary_authorization_domain (),
	                                                                     gdata_link_get_uri (_link), GDATA_ENTRY (rule), NULL,
	                                                                     &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_ACCESS_RULE (data->rule));

	calendar_access_rule_set_self_link (data->parent.calendar, data->rule);

	g_object_unref (rule);

	uhm_server_end_trace (mock_server);
}

static void
set_up_temp_calendar_acls_no_insertion (TempCalendarAclsData *data, gconstpointer service)
{
	set_up_temp_calendar ((TempCalendarData*) data, service);
	data->rule = NULL;
}

static void
tear_down_temp_calendar_acls (TempCalendarAclsData *data, gconstpointer service)
{
	/* Delete the access rule if it still exists */
	if (data->rule != NULL) {
		gdata_test_mock_server_start_trace (mock_server, "teardown-temp-calendar-acls");

		g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_calendar_service_get_primary_authorization_domain (),
		                                      GDATA_ENTRY (data->rule), NULL, NULL) == TRUE);
		g_object_unref (data->rule);

		uhm_server_end_trace (mock_server);
	}

	/* Delete the calendar */
	tear_down_temp_calendar ((TempCalendarData*) data, service);
}

static void
test_access_rule_get (TempCalendarAclsData *data, gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "access-rule-get");

	/* Get the rules */
	feed = gdata_access_handler_get_rules (GDATA_ACCESS_HANDLER (data->parent.calendar), GDATA_SERVICE (service), NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* TODO: check rules and feed properties */

	g_object_unref (feed);

	uhm_server_end_trace (mock_server);
}

static void
test_access_rule_insert (TempCalendarAclsData *data, gconstpointer service)
{
	GDataCalendarAccessRule *rule, *new_rule;
	const gchar *scope_type, *scope_value;
	GDataCategory *category;
	GDataLink *_link;
	GList *categories;
	gint64 edited;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "access-rule-insert");

	rule = gdata_calendar_access_rule_new (NULL);

	gdata_access_rule_set_role (GDATA_ACCESS_RULE (rule), GDATA_CALENDAR_ACCESS_ROLE_EDITOR);
	gdata_access_rule_set_scope (GDATA_ACCESS_RULE (rule), GDATA_ACCESS_SCOPE_USER, "darcy@gmail.com");

	/* Insert the rule */
	_link = gdata_entry_look_up_link (GDATA_ENTRY (data->parent.calendar), GDATA_LINK_ACCESS_CONTROL_LIST);
	g_assert (_link != NULL);

	new_rule = data->rule = GDATA_CALENDAR_ACCESS_RULE (gdata_service_insert_entry (GDATA_SERVICE (service),
	                                                                                gdata_calendar_service_get_primary_authorization_domain (),
	                                                                                gdata_link_get_uri (_link), GDATA_ENTRY (rule), NULL, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_ACCESS_RULE (new_rule));
	g_clear_error (&error);

	calendar_access_rule_set_self_link (data->parent.calendar, data->rule);

	/* Check the properties of the returned rule */
	g_assert_cmpstr (gdata_access_rule_get_role (GDATA_ACCESS_RULE (new_rule)), ==, GDATA_CALENDAR_ACCESS_ROLE_EDITOR);
	gdata_access_rule_get_scope (GDATA_ACCESS_RULE (new_rule), &scope_type, &scope_value);
	g_assert_cmpstr (scope_type, ==, GDATA_ACCESS_SCOPE_USER);
	g_assert_cmpstr (scope_value, ==, "darcy@gmail.com");
	edited = gdata_access_rule_get_edited (GDATA_ACCESS_RULE (new_rule));
	g_assert_cmpuint (edited, >, 0);

	/* Check it only has the one category and that it's correct */
	categories = gdata_entry_get_categories (GDATA_ENTRY (new_rule));
	g_assert (categories != NULL);
	g_assert_cmpuint (g_list_length (categories), ==, 1);
	category = categories->data;
	g_assert_cmpstr (gdata_category_get_term (category), ==, "calendar#aclRule");
	g_assert_cmpstr (gdata_category_get_scheme (category), ==, "http://schemas.google.com/g/2005#kind");
	g_assert (gdata_category_get_label (category) == NULL);

	/* TODO: Check more properties? */

	g_object_unref (rule);

	uhm_server_end_trace (mock_server);
}

static void
test_access_rule_update (TempCalendarAclsData *data, gconstpointer service)
{
	GDataCalendarAccessRule *new_rule;
	const gchar *scope_type, *scope_value;
	gint64 edited;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "access-rule-update");

	/* Update the rule */
	gdata_access_rule_set_role (GDATA_ACCESS_RULE (data->rule), GDATA_CALENDAR_ACCESS_ROLE_READ);

	/* Send the update to the server */
	new_rule = GDATA_CALENDAR_ACCESS_RULE (gdata_service_update_entry (GDATA_SERVICE (service), gdata_calendar_service_get_primary_authorization_domain (),
	                                                                   GDATA_ENTRY (data->rule), NULL, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_ACCESS_RULE (new_rule));
	g_clear_error (&error);

	calendar_access_rule_set_self_link (data->parent.calendar, new_rule);

	/* Check the properties of the returned rule */
	g_assert_cmpstr (gdata_access_rule_get_role (GDATA_ACCESS_RULE (new_rule)), ==, GDATA_CALENDAR_ACCESS_ROLE_READ);
	gdata_access_rule_get_scope (GDATA_ACCESS_RULE (new_rule), &scope_type, &scope_value);
	g_assert_cmpstr (scope_type, ==, GDATA_ACCESS_SCOPE_USER);
	g_assert_cmpstr (scope_value, ==, "darcy@gmail.com");
	edited = gdata_access_rule_get_edited (GDATA_ACCESS_RULE (new_rule));
	g_assert_cmpuint (edited, >, 0);

	g_object_unref (new_rule);

	uhm_server_end_trace (mock_server);
}

static void
test_access_rule_delete (TempCalendarAclsData *data, gconstpointer service)
{
	gboolean success;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "access-rule-delete");

	/* Delete the rule */
	success = gdata_service_delete_entry (GDATA_SERVICE (service), gdata_calendar_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->rule), NULL, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);
	g_clear_error (&error);

	g_object_unref (data->rule);
	data->rule = NULL;

	uhm_server_end_trace (mock_server);
}

static void
mock_server_notify_resolver_cb (GObject *object, GParamSpec *pspec, gpointer user_data)
{
	UhmServer *server;
	UhmResolver *resolver;

	server = UHM_SERVER (object);

	/* Set up the expected domain names here. This should technically be split up between
	 * the different unit test suites, but that's too much effort. */
	resolver = uhm_server_get_resolver (server);

	if (resolver != NULL) {
		const gchar *ip_address = uhm_server_get_address (server);

		uhm_resolver_add_A (resolver, "www.google.com", ip_address);
		uhm_resolver_add_A (resolver, "www.googleapis.com", ip_address);
		uhm_resolver_add_A (resolver,
		                    "accounts.google.com", ip_address);
	}
}

/* Set up a global GDataAuthorizer to be used for all the tests. Unfortunately,
 * the Google Calendar API is limited to OAuth1 and OAuth2 authorisation, so
 * this requires user interaction when online.
 *
 * If not online, use a dummy authoriser. */
static GDataAuthorizer *
create_global_authorizer (void)
{
	GDataOAuth2Authorizer *authorizer = NULL;  /* owned */
	gchar *authentication_uri, *authorisation_code;
	GError *error = NULL;

	/* If not online, just return a dummy authoriser. */
	if (!uhm_server_get_enable_online (mock_server)) {
		return GDATA_AUTHORIZER (gdata_dummy_authorizer_new (GDATA_TYPE_CALENDAR_SERVICE));
	}

	/* Otherwise, go through the interactive OAuth dance. */
	gdata_test_mock_server_start_trace (mock_server, "global-authentication");
	authorizer = gdata_oauth2_authorizer_new (CLIENT_ID, CLIENT_SECRET,
	                                          REDIRECT_URI,
	                                          GDATA_TYPE_CALENDAR_SERVICE);

	/* Get an authentication URI */
	authentication_uri = gdata_oauth2_authorizer_build_authentication_uri (authorizer, NULL, FALSE);
	g_assert (authentication_uri != NULL);

	/* Get the authorisation code off the user. */
	authorisation_code = gdata_test_query_user_for_verifier (authentication_uri);

	g_free (authentication_uri);

	if (authorisation_code == NULL) {
		/* Skip tests. */
		g_object_unref (authorizer);
		authorizer = NULL;
		goto skip_test;
	}

	/* Authorise the token */
	g_assert (gdata_oauth2_authorizer_request_authorization (authorizer, authorisation_code, NULL, &error));
	g_assert_no_error (error);

skip_test:
	g_free (authorisation_code);

	uhm_server_end_trace (mock_server);

	return GDATA_AUTHORIZER (authorizer);
}

int
main (int argc, char *argv[])
{
	gint retval;
	GDataAuthorizer *authorizer = NULL;
	GDataService *service = NULL;
	GFile *trace_directory;
	gchar *path = NULL;

	gdata_test_init (argc, argv);

	mock_server = gdata_test_get_mock_server ();
	g_signal_connect (G_OBJECT (mock_server), "notify::resolver", (GCallback) mock_server_notify_resolver_cb, NULL);
	path = g_test_build_filename (G_TEST_DIST, "traces/calendar", NULL);
	trace_directory = g_file_new_for_path (path);
	g_free (path);
	uhm_server_set_trace_directory (mock_server, trace_directory);
	g_object_unref (trace_directory);

	authorizer = create_global_authorizer ();

	service = GDATA_SERVICE (gdata_calendar_service_new (authorizer));

	g_test_add_func ("/calendar/authentication", test_authentication);

	g_test_add ("/calendar/query/all_calendars", QueryCalendarsData, service, set_up_query_calendars, test_query_all_calendars,
	            tear_down_query_calendars);
	g_test_add ("/calendar/query/all_calendars/async", GDataAsyncTestData, service, set_up_query_calendars_async,
	            test_query_all_calendars_async, tear_down_query_calendars_async);
	g_test_add ("/calendar/query/all_calendars/async/progress_closure", QueryCalendarsData, service, set_up_query_calendars,
	            test_query_all_calendars_async_progress_closure, tear_down_query_calendars);
	g_test_add ("/calendar/query/all_calendars/async/cancellation", GDataAsyncTestData, service, set_up_query_calendars_async,
	            test_query_all_calendars_async_cancellation, tear_down_query_calendars_async);

	g_test_add ("/calendar/query/own_calendars", QueryCalendarsData, service, set_up_query_calendars, test_query_own_calendars,
	            tear_down_query_calendars);
	g_test_add ("/calendar/query/own_calendars/async", GDataAsyncTestData, service, set_up_query_calendars_async,
		            test_query_own_calendars_async, tear_down_query_calendars_async);
	g_test_add ("/calendar/query/own_calendars/async/progress_closure", QueryCalendarsData, service, set_up_query_calendars,
	            test_query_own_calendars_async_progress_closure, tear_down_query_calendars);
	g_test_add ("/calendar/query/own_calendars/async/cancellation", GDataAsyncTestData, service, set_up_query_calendars_async,
	            test_query_own_calendars_async_cancellation, tear_down_query_calendars_async);

	g_test_add ("/calendar/query/events", QueryEventsData, service, set_up_query_events, test_query_events, tear_down_query_events);
	g_test_add ("/calendar/query/events/async", GDataAsyncTestData, service, set_up_query_events_async, test_query_events_async,
	            tear_down_query_events_async);
	g_test_add ("/calendar/query/events/async/progress_closure", QueryEventsData, service, set_up_query_events,
	            test_query_events_async_progress_closure, tear_down_query_events);
	g_test_add ("/calendar/query/events/async/cancellation", GDataAsyncTestData, service, set_up_query_events_async,
	            test_query_events_async_cancellation, tear_down_query_events_async);

	g_test_add ("/calendar/event/insert", InsertEventData, service, set_up_insert_event, test_event_insert, tear_down_insert_event);
	g_test_add ("/calendar/event/insert/async", GDataAsyncTestData, service, set_up_insert_event_async, test_event_insert_async,
	            tear_down_insert_event_async);
	g_test_add ("/calendar/event/insert/async/cancellation", GDataAsyncTestData, service, set_up_insert_event_async,
	            test_event_insert_async_cancellation, tear_down_insert_event_async);

	g_test_add ("/calendar/access-rule/get", TempCalendarAclsData, service, set_up_temp_calendar_acls, test_access_rule_get,
	            tear_down_temp_calendar_acls);
	g_test_add ("/calendar/access-rule/insert", TempCalendarAclsData, service, set_up_temp_calendar_acls_no_insertion,
	            test_access_rule_insert, tear_down_temp_calendar_acls);
	g_test_add ("/calendar/access-rule/update", TempCalendarAclsData, service, set_up_temp_calendar_acls, test_access_rule_update,
	            tear_down_temp_calendar_acls);
	g_test_add ("/calendar/access-rule/delete", TempCalendarAclsData, service, set_up_temp_calendar_acls, test_access_rule_delete,
	            tear_down_temp_calendar_acls);

	g_test_add_func ("/calendar/event/json", test_event_json);
	g_test_add_func ("/calendar/event/json/attendees", test_event_json_attendees);
	g_test_add_func ("/calendar/event/json/dates", test_event_json_dates);
	g_test_add_func ("/calendar/event/json/organizer", test_event_json_organizer);
	g_test_add_func ("/calendar/event/json/recurrence", test_event_json_recurrence);
	g_test_add_func ("/calendar/event/escaping", test_event_escaping);
	g_test_add_func ("/calendar/event/parser/minimal",
	                 test_calendar_event_parser_minimal);

	g_test_add_func ("/calendar/calendar/escaping", test_calendar_escaping);

	g_test_add_func ("/calendar/access-rule/properties", test_access_rule_properties);
	g_test_add_func ("/calendar/access-rule/json", test_access_rule_json);

	g_test_add_func ("/calendar/query/uri", test_query_uri);
	g_test_add_func ("/calendar/query/etag", test_query_etag);

	retval = g_test_run ();

	if (service != NULL)
		g_object_unref (service);

	return retval;
}
