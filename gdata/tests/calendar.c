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

#include <glib.h>
#include <unistd.h>
#include <string.h>

#include "gdata.h"
#include "common.h"

static GDataCalendarCalendar *
get_calendar (gconstpointer service, GError **error)
{
	GDataFeed *calendar_feed;
	GDataCalendarCalendar *calendar;
	GList *calendars;

	/* Get a calendar */
	calendar_feed = gdata_calendar_service_query_own_calendars (GDATA_CALENDAR_SERVICE (service), NULL, NULL, NULL, NULL, error);
	g_assert_no_error (*error);
	g_assert (GDATA_IS_CALENDAR_FEED (calendar_feed));
	g_clear_error (error);

	calendars = gdata_feed_get_entries (calendar_feed);
	g_assert (calendars != NULL);
	calendar = calendars->data;
	g_assert (GDATA_IS_CALENDAR_CALENDAR (calendar));

	g_object_ref (calendar);
	g_object_unref (calendar_feed);

	return calendar;
}

static void
test_authentication (void)
{
	gboolean retval;
	GDataService *service;
	GError *error = NULL;

	/* Create a service */
	service = GDATA_SERVICE (gdata_calendar_service_new (CLIENT_ID));

	g_assert (service != NULL);
	g_assert (GDATA_IS_SERVICE (service));
	g_assert_cmpstr (gdata_service_get_client_id (service), ==, CLIENT_ID);

	/* Log in */
	retval = gdata_service_authenticate (service, USERNAME, PASSWORD, NULL, &error);
	g_assert_no_error (error);
	g_assert (retval == TRUE);
	g_clear_error (&error);

	/* Check all is as it should be */
	g_assert (gdata_service_is_authenticated (service) == TRUE);
	g_assert_cmpstr (gdata_service_get_username (service), ==, USERNAME);
	g_assert_cmpstr (gdata_service_get_password (service), ==, PASSWORD);

	g_object_unref (service);
}

static void
test_authentication_async_cb (GDataService *service, GAsyncResult *async_result, GMainLoop *main_loop)
{
	gboolean retval;
	GError *error = NULL;

	retval = gdata_service_authenticate_finish (service, async_result, &error);
	g_assert_no_error (error);
	g_assert (retval == TRUE);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);

	/* Check all is as it should be */
	g_assert (gdata_service_is_authenticated (service) == TRUE);
	g_assert_cmpstr (gdata_service_get_username (service), ==, USERNAME);
	g_assert_cmpstr (gdata_service_get_password (service), ==, PASSWORD);
}

static void
test_authentication_async (void)
{
	GDataService *service;
	GMainLoop *main_loop = g_main_loop_new (NULL, TRUE);

	/* Create a service */
	service = GDATA_SERVICE (gdata_calendar_service_new (CLIENT_ID));

	g_assert (service != NULL);
	g_assert (GDATA_IS_SERVICE (service));

	gdata_service_authenticate_async (service, USERNAME, PASSWORD, NULL, (GAsyncReadyCallback) test_authentication_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);

	g_object_unref (service);
}

static void
test_query_all_calendars (gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;

	feed = gdata_calendar_service_query_all_calendars (GDATA_CALENDAR_SERVICE (service), NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_FEED (feed));
	g_clear_error (&error);

	/* TODO: check entries and feed properties */

	g_object_unref (feed);
}

static void
test_query_all_calendars_async_cb (GDataService *service, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GDataFeed *feed;
	GError *error = NULL;

	feed = gdata_service_query_finish (service, async_result, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_FEED (feed));
	g_clear_error (&error);

	/* TODO: Tests? */
	g_main_loop_quit (main_loop);

	g_object_unref (feed);
}

static void
test_query_all_calendars_async (gconstpointer service)
{
	GMainLoop *main_loop = g_main_loop_new (NULL, TRUE);

	gdata_calendar_service_query_all_calendars_async (GDATA_CALENDAR_SERVICE (service), NULL, NULL, NULL,
							  NULL, (GAsyncReadyCallback) test_query_all_calendars_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
}

static void
test_query_own_calendars (gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;

	feed = gdata_calendar_service_query_own_calendars (GDATA_CALENDAR_SERVICE (service), NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_FEED (feed));
	g_clear_error (&error);

	/* TODO: check entries and feed properties */

	g_object_unref (feed);
}

static void
test_query_own_calendars_async_cb (GDataService *service, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GDataFeed *feed;
	GError *error = NULL;

	feed = gdata_service_query_finish (service, async_result, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_FEED (feed));
	g_clear_error (&error);

	/* TODO: Tests? */
	g_main_loop_quit (main_loop);

	g_object_unref (feed);
}

static void
test_query_own_calendars_async (gconstpointer service)
{
	GMainLoop *main_loop = g_main_loop_new (NULL, TRUE);

	gdata_calendar_service_query_own_calendars_async (GDATA_CALENDAR_SERVICE (service), NULL, NULL, NULL,
							  NULL, (GAsyncReadyCallback) test_query_own_calendars_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
}

static void
test_query_events (gconstpointer service)
{
	GDataFeed *feed;
	GDataCalendarCalendar *calendar;
	GError *error = NULL;

	calendar = get_calendar (service, &error);

	/* Get the entry feed */
	feed = gdata_calendar_service_query_events (GDATA_CALENDAR_SERVICE (service), calendar, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_FEED (feed));
	g_clear_error (&error);

	/* TODO: check entries and feed properties */

	g_object_unref (feed);
	g_object_unref (calendar);
}

static void
test_query_events_async_cb (GDataService *service, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GDataFeed *feed;
	GError *error = NULL;

	feed = gdata_service_query_finish (service, async_result, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_FEED (feed));

	/* TODO: Tests? */
	g_main_loop_quit (main_loop);

	g_object_unref (feed);
}

static void
test_query_events_async (gconstpointer service)
{
	GDataCalendarCalendar *calendar;
	GMainLoop *main_loop;
	GError *error = NULL;

	calendar = get_calendar (service, &error);
	main_loop = g_main_loop_new (NULL, TRUE);

	gdata_calendar_service_query_events_async (GDATA_CALENDAR_SERVICE (service), calendar, NULL, NULL, NULL, NULL,
	                                           (GAsyncReadyCallback) test_query_events_async_cb, main_loop);
	g_main_loop_run (main_loop);

	g_main_loop_unref (main_loop);
	g_object_unref (calendar);
}

static void
test_insert_simple (gconstpointer service)
{
	GDataCalendarEvent *event, *new_event;
	GDataGDWhere *where;
	GDataGDWho *who;
	GDataGDWhen *when;
	GTimeVal start_time, end_time;
	gchar *xml;
	GError *error = NULL;

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

	/* Check the XML */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (event));
	g_assert_cmpstr (xml, ==,
			 "<?xml version='1.0' encoding='UTF-8'?>"
			 "<entry xmlns='http://www.w3.org/2005/Atom' "
			 	"xmlns:gd='http://schemas.google.com/g/2005' "
			 	"xmlns:gCal='http://schemas.google.com/gCal/2005' "
			 	"xmlns:app='http://www.w3.org/2007/app'>"
			 	"<title type='text'>Tennis with Beth</title>"
			 	"<content type='text'>Meet for a quick lesson.</content>"
			 	"<category term='http://schemas.google.com/g/2005#event' scheme='http://schemas.google.com/g/2005#kind'/>"
			 	"<gd:eventStatus value='http://schemas.google.com/g/2005#event.confirmed'/>"
			 	"<gd:transparency value='http://schemas.google.com/g/2005#event.opaque'/>"
			 	"<gCal:guestsCanModify value='false'/>"
			 	"<gCal:guestsCanInviteOthers value='false'/>"
			 	"<gCal:guestsCanSeeGuests value='false'/>"
			 	"<gCal:anyoneCanAddSelf value='false'/>"
				"<gd:when startTime='2009-04-17T15:00:00Z' endTime='2009-04-17T17:00:00Z'/>"
			 	"<gd:who email='john.smith@example.com' "
			 		"rel='http://schemas.google.com/g/2005#event.organizer' "
			 		"valueString='John Smith\342\200\275'/>"
			 	"<gd:where valueString='Rolling Lawn Courts'/>"
			 "</entry>");
	g_free (xml);

	/* Insert the event */
	new_event = gdata_calendar_service_insert_event (GDATA_CALENDAR_SERVICE (service), event, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_EVENT (new_event));
	g_clear_error (&error);

	/* TODO: check entries and feed properties */

	g_object_unref (event);
	g_object_unref (new_event);
}

static void
test_xml_dates (void)
{
	GDataCalendarEvent *event;
	GList *times, *i;
	GDataGDWhen *when;
	gint64 _time;
	gchar *xml;
	GError *error = NULL;

	event = GDATA_CALENDAR_EVENT (gdata_parsable_new_from_xml (GDATA_TYPE_CALENDAR_EVENT,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
		 	"xmlns:gd='http://schemas.google.com/g/2005' "
		 	"xmlns:gCal='http://schemas.google.com/gCal/2005' "
		 	"xmlns:app='http://www.w3.org/2007/app'>"
		 	"<title type='text'>Tennis with Beth</title>"
		 	"<content type='text'>Meet for a quick lesson.</content>"
		 	"<category term='http://schemas.google.com/g/2005#event' scheme='http://schemas.google.com/g/2005#kind'/>"
		 	"<gd:when startTime='2009-04-17'/>"
		 	"<gd:when startTime='2009-04-17T15:00:00Z'/>"
		 	"<gd:when startTime='2009-04-27' endTime='20090506'/>"
		 "</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_ENTRY (event));
	g_clear_error (&error);

	/* Check the times */
	times = i = gdata_calendar_event_get_times (event);

	/* First time */
	when = GDATA_GD_WHEN (i->data);
	g_assert (i->next != NULL);
	g_assert (gdata_gd_when_is_date (when) == TRUE);
	_time = gdata_gd_when_get_start_time (when);
	g_assert_cmpint (_time, ==, 1239926400);
	_time = gdata_gd_when_get_end_time (when);
	g_assert_cmpint (_time, ==, -1);
	g_assert (gdata_gd_when_get_value_string (when) == NULL);
	g_assert (gdata_gd_when_get_reminders (when) == NULL);

	/* Second time */
	i = i->next;
	when = GDATA_GD_WHEN (i->data);
	g_assert (i->next != NULL);
	g_assert (gdata_gd_when_is_date (when) == FALSE);
	_time = gdata_gd_when_get_start_time (when);
	g_assert_cmpint (_time, ==, 1239926400 + 54000);
	_time = gdata_gd_when_get_end_time (when);
	g_assert_cmpint (_time, ==, -1);
	g_assert (gdata_gd_when_get_value_string (when) == NULL);
	g_assert (gdata_gd_when_get_reminders (when) == NULL);

	/* Third time */
	i = i->next;
	when = GDATA_GD_WHEN (i->data);
	g_assert (i->next == NULL);
	g_assert (gdata_gd_when_is_date (when) == TRUE);
	_time = gdata_gd_when_get_start_time (when);
	g_assert_cmpint (_time, ==, 1239926400 + 864000);
	_time = gdata_gd_when_get_end_time (when);
	g_assert_cmpint (_time, ==, 1241568000);
	g_assert (gdata_gd_when_get_value_string (when) == NULL);
	g_assert (gdata_gd_when_get_reminders (when) == NULL);

	/* Check the XML */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (event));
	g_assert_cmpstr (xml, ==,
			 "<?xml version='1.0' encoding='UTF-8'?>"
			 "<entry xmlns='http://www.w3.org/2005/Atom' "
			 	"xmlns:gd='http://schemas.google.com/g/2005' "
			 	"xmlns:gCal='http://schemas.google.com/gCal/2005' "
			 	"xmlns:app='http://www.w3.org/2007/app'>"
			 	"<title type='text'>Tennis with Beth</title>"
			 	"<content type='text'>Meet for a quick lesson.</content>"
			 	"<category term='http://schemas.google.com/g/2005#event' scheme='http://schemas.google.com/g/2005#kind'/>"
			 	"<gCal:guestsCanModify value='false'/>"
				"<gCal:guestsCanInviteOthers value='false'/>"
				"<gCal:guestsCanSeeGuests value='false'/>"
				"<gCal:anyoneCanAddSelf value='false'/>"
			 	"<gd:when startTime='2009-04-17'/>"
			 	"<gd:when startTime='2009-04-17T15:00:00Z'/>"
			 	"<gd:when startTime='2009-04-27' endTime='2009-05-06'/>"
			 "</entry>");
	g_free (xml);

	g_object_unref (event);
}

static void
test_xml_recurrence (void)
{
	GDataCalendarEvent *event;
	GError *error = NULL;
	gchar *id, *uri;

	event = GDATA_CALENDAR_EVENT (gdata_parsable_new_from_xml (GDATA_TYPE_CALENDAR_EVENT,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
		 	"xmlns:gd='http://schemas.google.com/g/2005' "
		 	"xmlns:gCal='http://schemas.google.com/gCal/2005' "
		 	"xmlns:app='http://www.w3.org/2007/app'>"
			"<id>http://www.google.com/calendar/feeds/libgdata.test@googlemail.com/events/g5928e82rrch95b25f8ud0dlsg_20090429T153000Z</id>"
			"<published>2009-04-25T15:22:47.000Z</published>"
			"<updated>2009-04-27T17:54:10.000Z</updated>"
			"<app:edited xmlns:app='http://www.w3.org/2007/app'>2009-04-27T17:54:10.000Z</app:edited>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://schemas.google.com/g/2005#event'/>"
			"<title>Test daily instance event</title>"
			"<content></content>"
			"<link rel='http://www.iana.org/assignments/relation/alternate' type='text/html' href='http://www.google.com/calendar/event?eid=ZzU5MjhlODJycmNoOTViMjVmOHVkMGRsc2dfMjAwOTA0MjlUMTUzMDAwWiBsaWJnZGF0YS50ZXN0QGdvb2dsZW1haWwuY29t' title='alternate'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' href='http://www.google.com/calendar/feeds/libgdata.test@googlemail.com/private/full/g5928e82rrch95b25f8ud0dlsg_20090429T153000Z'/>"
			"<link rel='http://www.iana.org/assignments/relation/edit' type='application/atom+xml' href='http://www.google.com/calendar/feeds/libgdata.test@googlemail.com/private/full/g5928e82rrch95b25f8ud0dlsg_20090429T153000Z'/>"
			"<author>"
				"<name>GData Test</name>"
				"<email>libgdata.test@googlemail.com</email>"
			"</author>"
			"<gd:originalEvent id='g5928e82rrch95b25f8ud0dlsg' href='http://www.google.com/calendar/feeds/libgdata.test@googlemail.com/private/full/g5928e82rrch95b25f8ud0dlsg'>"
				"<gd:when startTime='2009-04-29T16:30:00.000+01:00'/>"
			"</gd:originalEvent>"
			"<gCal:guestsCanModify value='false'/>"
			"<gCal:guestsCanInviteOthers value='false'/>"
			"<gCal:guestsCanSeeGuests value='false'/>"
			"<gCal:anyoneCanAddSelf value='false'/>"
			"<gd:comments>"
				"<gd:feedLink href='http://www.google.com/calendar/feeds/libgdata.test@googlemail.com/private/full/g5928e82rrch95b25f8ud0dlsg_20090429T153000Z/comments'/>"
			"</gd:comments>"
			"<gd:eventStatus value='http://schemas.google.com/g/2005#event.confirmed'/>"
			"<gd:visibility value='http://schemas.google.com/g/2005#event.private'/>"
			"<gd:transparency value='http://schemas.google.com/g/2005#event.opaque'/>"
			"<gCal:uid value='g5928e82rrch95b25f8ud0dlsg@google.com'/>"
			"<gCal:sequence value='0'/>"
			"<gd:when startTime='2009-04-29T17:30:00.000+01:00' endTime='2009-04-29T17:30:00.000+01:00'>"
				"<gd:reminder minutes='10' method='email'/>"
				"<gd:reminder minutes='10' method='alert'/>"
			"</gd:when>"
			"<gd:who rel='http://schemas.google.com/g/2005#event.organizer' valueString='GData Test' email='libgdata.test@googlemail.com'/>"
			"<gd:where valueString=''/>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_ENTRY (event));
	g_clear_error (&error);

	/* Check the original event */
	g_assert (gdata_calendar_event_is_exception (event) == TRUE);

	gdata_calendar_event_get_original_event_details (event, &id, &uri);
	g_assert_cmpstr (id, ==, "g5928e82rrch95b25f8ud0dlsg");
	g_assert_cmpstr (uri, ==, "http://www.google.com/calendar/feeds/libgdata.test@googlemail.com/private/full/g5928e82rrch95b25f8ud0dlsg");

	g_free (id);
	g_free (uri);
	g_object_unref (event);
}

static void
test_calendar_escaping (void)
{
	GDataCalendarCalendar *calendar;
	gchar *xml;

	calendar = gdata_calendar_calendar_new (NULL);
	gdata_calendar_calendar_set_timezone (calendar, "<timezone>");

	/* Check the outputted XML is escaped properly */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (calendar));
	g_assert_cmpstr (xml, ==,
	                 "<?xml version='1.0' encoding='UTF-8'?>"
	                 "<entry xmlns='http://www.w3.org/2005/Atom' xmlns:gd='http://schemas.google.com/g/2005' "
	                        "xmlns:gCal='http://schemas.google.com/gCal/2005' xmlns:app='http://www.w3.org/2007/app'>"
				"<title type='text'></title>"
				"<category term='http://schemas.google.com/gCal/2005#calendarmeta' scheme='http://schemas.google.com/g/2005#kind'/>"
				"<gCal:timezone value='&lt;timezone&gt;'/>"
				"<gCal:hidden value='false'/>"
				"<gCal:color value='#000000'/>"
				"<gCal:selected value='false'/>"
	                 "</entry>");
	g_free (xml);
	g_object_unref (calendar);
}

static void
test_event_escaping (void)
{
	GDataCalendarEvent *event;
	gchar *xml;

	event = gdata_calendar_event_new (NULL);
	gdata_calendar_event_set_status (event, "<status>");
	gdata_calendar_event_set_visibility (event, "<visibility>");
	gdata_calendar_event_set_transparency (event, "<transparency>");
	gdata_calendar_event_set_uid (event, "<uid>");
	gdata_calendar_event_set_recurrence (event, "<recurrence>");

	/* Check the outputted XML is escaped properly */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (event));
	g_assert_cmpstr (xml, ==,
	                 "<?xml version='1.0' encoding='UTF-8'?>"
	                 "<entry xmlns='http://www.w3.org/2005/Atom' xmlns:gd='http://schemas.google.com/g/2005' "
	                        "xmlns:gCal='http://schemas.google.com/gCal/2005' xmlns:app='http://www.w3.org/2007/app'>"
				"<title type='text'></title>"
				"<category term='http://schemas.google.com/g/2005#event' scheme='http://schemas.google.com/g/2005#kind'/>"
				"<gd:eventStatus value='&lt;status&gt;'/>"
				"<gd:visibility value='&lt;visibility&gt;'/>"
				"<gd:transparency value='&lt;transparency&gt;'/>"
				"<gCal:uid value='&lt;uid&gt;'/>"
				"<gCal:guestsCanModify value='false'/>"
				"<gCal:guestsCanInviteOthers value='false'/>"
				"<gCal:guestsCanSeeGuests value='false'/>"
				"<gCal:anyoneCanAddSelf value='false'/>"
				"<gd:recurrence>&lt;recurrence&gt;</gd:recurrence>"
	                 "</entry>");
	g_free (xml);
	g_object_unref (event);
}

static void
test_query_uri (void)
{
	gint64 _time;
	GTimeVal time_val;
	gchar *query_uri;
	GDataCalendarQuery *query = gdata_calendar_query_new ("q");

	gdata_calendar_query_set_future_events (query, TRUE);
	g_assert (gdata_calendar_query_get_future_events (query) == TRUE);

	gdata_calendar_query_set_order_by (query, "starttime");
	g_assert_cmpstr (gdata_calendar_query_get_order_by (query), ==, "starttime");

	g_time_val_from_iso8601 ("2009-04-17T15:00:00.000Z", &time_val);
	gdata_calendar_query_set_recurrence_expansion_start (query, time_val.tv_sec);
	_time = gdata_calendar_query_get_recurrence_expansion_start (query);
	g_assert_cmpint (_time, ==, time_val.tv_sec);

	g_time_val_from_iso8601 ("2010-04-17T15:00:00.000Z", &time_val);
	gdata_calendar_query_set_recurrence_expansion_end (query, time_val.tv_sec);
	_time = gdata_calendar_query_get_recurrence_expansion_end (query);
	g_assert_cmpint (_time, ==, time_val.tv_sec);

	gdata_calendar_query_set_single_events (query, TRUE);
	g_assert (gdata_calendar_query_get_single_events (query) == TRUE);

	gdata_calendar_query_set_sort_order (query, "descending");
	g_assert_cmpstr (gdata_calendar_query_get_sort_order (query), ==, "descending");

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

	/* Check the built query URI with a normal feed URI */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&futureevents=true&orderby=starttime&recurrence-expansion-start=2009-04-17T15:00:00Z"
					"&recurrence-expansion-end=2010-04-17T15:00:00Z&singleevents=true&sortorder=descending"
					"&start-min=2009-04-17T15:00:00Z&start-max=2010-04-17T15:00:00Z&ctz=America%2FLos_Angeles");
	g_free (query_uri);

	/* …with a feed URI with a trailing slash */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com/");
	g_assert_cmpstr (query_uri, ==, "http://example.com/?q=q&futureevents=true&orderby=starttime&recurrence-expansion-start=2009-04-17T15:00:00Z"
					"&recurrence-expansion-end=2010-04-17T15:00:00Z&singleevents=true&sortorder=descending"
					"&start-min=2009-04-17T15:00:00Z&start-max=2010-04-17T15:00:00Z&ctz=America%2FLos_Angeles");
	g_free (query_uri);

	/* …with a feed URI with pre-existing arguments */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com/bar/?test=test&this=that");
	g_assert_cmpstr (query_uri, ==, "http://example.com/bar/?test=test&this=that&q=q&futureevents=true&orderby=starttime"
					"&recurrence-expansion-start=2009-04-17T15:00:00Z&recurrence-expansion-end=2010-04-17T15:00:00Z"
					"&singleevents=true&sortorder=descending&start-min=2009-04-17T15:00:00Z&start-max=2010-04-17T15:00:00Z"
					"&ctz=America%2FLos_Angeles");
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
	gdata_query_set_etag (GDATA_QUERY (query), "foobar");		\
	(C);								\
	g_assert (gdata_query_get_etag (GDATA_QUERY (query)) == NULL);

	CHECK_ETAG (gdata_calendar_query_set_future_events (query, FALSE))
	CHECK_ETAG (gdata_calendar_query_set_order_by (query, "shizzle"))
	CHECK_ETAG (gdata_calendar_query_set_recurrence_expansion_start (query, -1))
	CHECK_ETAG (gdata_calendar_query_set_recurrence_expansion_end (query, -1))
	CHECK_ETAG (gdata_calendar_query_set_single_events (query, FALSE))
	CHECK_ETAG (gdata_calendar_query_set_sort_order (query, "shizzle"))
	CHECK_ETAG (gdata_calendar_query_set_start_min (query, -1))
	CHECK_ETAG (gdata_calendar_query_set_start_max (query, -1))
	CHECK_ETAG (gdata_calendar_query_set_timezone (query, "about now"))

#undef CHECK_ETAG

	g_object_unref (query);
}

static void
test_acls_get_rules (gconstpointer service)
{
	GDataFeed *feed;
	GDataCalendarCalendar *calendar;
	GError *error = NULL;

	calendar = get_calendar (service, &error);

	/* Get the rules */
	feed = gdata_access_handler_get_rules (GDATA_ACCESS_HANDLER (calendar), GDATA_SERVICE (service), NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* TODO: check rules and feed properties */

	g_object_unref (feed);
	g_object_unref (calendar);
}

static void
test_acls_insert_rule (gconstpointer service)
{
	GDataCalendarCalendar *calendar;
	GDataAccessRule *rule, *new_rule;
	const gchar *scope_type, *scope_value;
	GDataCategory *category;
	GDataLink *_link;
	GList *categories;
	gchar *xml;
	gint64 edited;
	GError *error = NULL;

	calendar = get_calendar (service, &error);

	rule = gdata_access_rule_new (NULL);

	gdata_access_rule_set_role (rule, GDATA_CALENDAR_ACCESS_ROLE_EDITOR);
	g_assert_cmpstr (gdata_access_rule_get_role (rule), ==, GDATA_CALENDAR_ACCESS_ROLE_EDITOR);

	gdata_access_rule_set_scope (rule, GDATA_ACCESS_SCOPE_USER, "darcy@gmail.com");
	gdata_access_rule_get_scope (rule, &scope_type, &scope_value);
	g_assert_cmpstr (scope_type, ==, GDATA_ACCESS_SCOPE_USER);
	g_assert_cmpstr (scope_value, ==, "darcy@gmail.com");

	/* Check the XML */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (rule));
	g_assert_cmpstr (xml, ==,
			 "<?xml version='1.0' encoding='UTF-8'?>"
			 "<entry xmlns='http://www.w3.org/2005/Atom' "
			 	"xmlns:gd='http://schemas.google.com/g/2005' "
			 	"xmlns:gAcl='http://schemas.google.com/acl/2007'>"
			 	"<title type='text'>http://schemas.google.com/gCal/2005#editor</title>"
			 	"<category term='http://schemas.google.com/acl/2007#accessRule' scheme='http://schemas.google.com/g/2005#kind'/>"
				"<gAcl:role value='http://schemas.google.com/gCal/2005#editor'/>"
				"<gAcl:scope type='user' value='darcy@gmail.com'/>"
			 "</entry>");
	g_free (xml);

	/* Insert the rule */
	_link = gdata_entry_look_up_link (GDATA_ENTRY (calendar), GDATA_LINK_ACCESS_CONTROL_LIST);
	g_assert (_link != NULL);

	new_rule = GDATA_ACCESS_RULE (gdata_service_insert_entry (GDATA_SERVICE (service), gdata_link_get_uri (_link), GDATA_ENTRY (rule),
	                                                          NULL, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_ACCESS_RULE (new_rule));
	g_clear_error (&error);

	/* Check the properties of the returned rule */
	g_assert_cmpstr (gdata_access_rule_get_role (new_rule), ==, GDATA_CALENDAR_ACCESS_ROLE_EDITOR);
	gdata_access_rule_get_scope (new_rule, &scope_type, &scope_value);
	g_assert_cmpstr (scope_type, ==, GDATA_ACCESS_SCOPE_USER);
	g_assert_cmpstr (scope_value, ==, "darcy@gmail.com");
	edited = gdata_access_rule_get_edited (new_rule);
	g_assert_cmpuint (edited, >, 0);

	/* Check it only has the one category and that it's correct */
	categories = gdata_entry_get_categories (GDATA_ENTRY (new_rule));
	g_assert (categories != NULL);
	g_assert_cmpuint (g_list_length (categories), ==, 1);
	category = categories->data;
	g_assert_cmpstr (gdata_category_get_term (category), ==, "http://schemas.google.com/acl/2007#accessRule");
	g_assert_cmpstr (gdata_category_get_scheme (category), ==, "http://schemas.google.com/g/2005#kind");
	g_assert (gdata_category_get_label (category) == NULL);

	/* TODO: Check more properties? */

	g_object_unref (rule);
	g_object_unref (new_rule);
	g_object_unref (calendar);
}

static void
test_acls_update_rule (gconstpointer service)
{
	GDataFeed *feed;
	GDataCalendarCalendar *calendar;
	GDataAccessRule *rule = NULL, *new_rule;
	const gchar *scope_type, *scope_value;
	GList *rules;
	gint64 edited;
	GError *error = NULL;

	calendar = get_calendar (service, &error);

	/* Get a rule */
	feed = gdata_access_handler_get_rules (GDATA_ACCESS_HANDLER (calendar), GDATA_SERVICE (service), NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* Find the rule applying to darcy@gmail.com */
	for (rules = gdata_feed_get_entries (feed); rules != NULL; rules = rules->next) {
		gdata_access_rule_get_scope (GDATA_ACCESS_RULE (rules->data), NULL, &scope_value);
		if (scope_value != NULL && strcmp (scope_value, "darcy@gmail.com") == 0) {
			rule = GDATA_ACCESS_RULE (rules->data);
			break;
		}
	}
	g_assert (GDATA_IS_ACCESS_RULE (rule));

	g_object_ref (rule);
	g_object_unref (feed);

	/* Update the rule */
	gdata_access_rule_set_role (rule, GDATA_CALENDAR_ACCESS_ROLE_READ);
	g_assert_cmpstr (gdata_access_rule_get_role (rule), ==, GDATA_CALENDAR_ACCESS_ROLE_READ);

	/* Send the update to the server */
	new_rule = GDATA_ACCESS_RULE (gdata_service_update_entry (GDATA_SERVICE (service), GDATA_ENTRY (rule), NULL, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_ACCESS_RULE (new_rule));
	g_clear_error (&error);
	g_object_unref (rule);

	/* Check the properties of the returned rule */
	g_assert_cmpstr (gdata_access_rule_get_role (new_rule), ==, GDATA_CALENDAR_ACCESS_ROLE_READ);
	gdata_access_rule_get_scope (new_rule, &scope_type, &scope_value);
	g_assert_cmpstr (scope_type, ==, GDATA_ACCESS_SCOPE_USER);
	g_assert_cmpstr (scope_value, ==, "darcy@gmail.com");
	edited = gdata_access_rule_get_edited (new_rule);
	g_assert_cmpuint (edited, >, 0);

	g_object_unref (new_rule);
	g_object_unref (calendar);
}

static void
test_acls_delete_rule (gconstpointer service)
{
	GDataFeed *feed;
	GDataCalendarCalendar *calendar;
	GDataAccessRule *rule = NULL;
	GList *rules;
	gboolean success;
	GError *error = NULL;

	calendar = get_calendar (service, &error);

	/* Get a rule */
	feed = gdata_access_handler_get_rules (GDATA_ACCESS_HANDLER (calendar), GDATA_SERVICE (service), NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* Find the rule applying to darcy@gmail.com */
	for (rules = gdata_feed_get_entries (feed); rules != NULL; rules = rules->next) {
		const gchar *scope_value;

		gdata_access_rule_get_scope (GDATA_ACCESS_RULE (rules->data), NULL, &scope_value);
		if (scope_value != NULL && strcmp (scope_value, "darcy@gmail.com") == 0) {
			rule = GDATA_ACCESS_RULE (rules->data);
			break;
		}
	}
	g_assert (GDATA_IS_ACCESS_RULE (rule));

	g_object_ref (rule);
	g_object_unref (feed);

	/* Delete the rule */
	success = gdata_service_delete_entry (GDATA_SERVICE (service), GDATA_ENTRY (rule), NULL, &error);
	g_assert_no_error (error);
	g_assert (success == TRUE);
	g_clear_error (&error);

	g_object_unref (rule);
	g_object_unref (calendar);
}

static void
test_batch (gconstpointer service)
{
	GDataBatchOperation *operation;
	GDataService *service2;
	GDataCalendarCalendar *calendar;
	GDataCalendarEvent *event, *event2, *event3;
	GDataEntry *inserted_entry, *inserted_entry2, *inserted_entry3;
	gchar *feed_uri;
	guint op_id, op_id2, op_id3;
	GError *error = NULL, *entry_error = NULL;

	calendar = get_calendar (service, &error);

	/* Here we hardcode the feed URI, but it should really be extracted from an event feed, as the GDATA_LINK_BATCH link */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "https://www.google.com/calendar/feeds/default/private/full/batch");

	/* Check the properties of the operation */
	g_assert (gdata_batch_operation_get_service (operation) == service);
	g_assert_cmpstr (gdata_batch_operation_get_feed_uri (operation), ==, "https://www.google.com/calendar/feeds/default/private/full/batch");

	g_object_get (operation,
	              "service", &service2,
	              "feed-uri", &feed_uri,
	              NULL);

	g_assert (service2 == service);
	g_assert_cmpstr (feed_uri, ==, "https://www.google.com/calendar/feeds/default/private/full/batch");

	g_object_unref (service2);
	g_free (feed_uri);

	/* Run a singleton batch operation to insert a new entry */
	event = gdata_calendar_event_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (event), "Fooish Bar");

	gdata_test_batch_operation_insertion (operation, GDATA_ENTRY (event), &inserted_entry, NULL);
	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);
	g_object_unref (event);

	/* Run another batch operation to insert another entry and query the previous one */
	event2 = gdata_calendar_event_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (event2), "Cow Lunch");

	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "https://www.google.com/calendar/feeds/default/private/full/batch");
	op_id = gdata_test_batch_operation_insertion (operation, GDATA_ENTRY (event2), &inserted_entry2, NULL);
	op_id2 = gdata_test_batch_operation_query (operation, gdata_entry_get_id (inserted_entry), GDATA_TYPE_CALENDAR_EVENT, inserted_entry, NULL,
	                                           NULL);
	g_assert_cmpuint (op_id, !=, op_id2);

	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);
	g_object_unref (event2);

	/* Run another batch operation to delete the first entry and a fictitious one to test error handling, and update the second entry */
	gdata_entry_set_title (inserted_entry2, "Toby");
	event3 = gdata_calendar_event_new ("foobar");

	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "https://www.google.com/calendar/feeds/default/private/full/batch");
	op_id = gdata_test_batch_operation_deletion (operation, inserted_entry, NULL);
	op_id2 = gdata_test_batch_operation_deletion (operation, GDATA_ENTRY (event3), &entry_error);
	op_id3 = gdata_test_batch_operation_update (operation, inserted_entry2, &inserted_entry3, NULL);
	g_assert_cmpuint (op_id, !=, op_id2);
	g_assert_cmpuint (op_id, !=, op_id3);
	g_assert_cmpuint (op_id2, !=, op_id3);

	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_assert_error (entry_error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR);

	g_clear_error (&error);
	g_clear_error (&entry_error);
	g_object_unref (operation);
	g_object_unref (inserted_entry);
	g_object_unref (event3);

	/* Run another batch operation to update the second entry with the wrong ETag (i.e. pass the old version of the entry to the batch operation
	 * to test error handling */
	/* Turns out that we can't run this test, because the Calendar service sucks with ETags and doesn't error. */
	/*operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "https://www.google.com/calendar/feeds/default/private/full/batch");
	gdata_test_batch_operation_update (operation, inserted_entry2, NULL, &entry_error);
	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_assert_error (entry_error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_CONFLICT);

	g_clear_error (&error);
	g_clear_error (&entry_error);
	g_object_unref (operation);*/
	g_object_unref (inserted_entry2);

	/* Run a final batch operation to delete the second entry */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "https://www.google.com/calendar/feeds/default/private/full/batch");
	gdata_test_batch_operation_deletion (operation, inserted_entry3, NULL);
	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);
	g_object_unref (inserted_entry3);
	g_object_unref (calendar);
}

typedef struct {
	GDataCalendarEvent *new_event;
} BatchAsyncData;

static void
setup_batch_async (BatchAsyncData *data, gconstpointer service)
{
	GDataCalendarEvent *event;
	GError *error = NULL;

	/* Insert a new event which we can query asyncly */
	event = gdata_calendar_event_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (event), "Party 'Til You Puke");

	data->new_event = gdata_calendar_service_insert_event (GDATA_CALENDAR_SERVICE (service), event, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CALENDAR_EVENT (data->new_event));
	g_clear_error (&error);

	g_object_unref (event);
}

static void
test_batch_async_cb (GDataBatchOperation *operation, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GError *error = NULL;

	/* Clear all pending events (such as callbacks for the operations) */
	while (g_main_context_iteration (NULL, FALSE) == TRUE);

	g_assert (gdata_test_batch_operation_run_finish (operation, async_result, &error) == TRUE);
	g_assert_no_error (error);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
}

static void
test_batch_async (BatchAsyncData *data, gconstpointer service)
{
	GDataBatchOperation *operation;
	guint op_id;
	GMainLoop *main_loop;

	/* Run an async query operation on the event */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "https://www.google.com/calendar/feeds/default/private/full/batch");
	op_id = gdata_test_batch_operation_query (operation, gdata_entry_get_id (GDATA_ENTRY (data->new_event)), GDATA_TYPE_CALENDAR_EVENT,
	                                          GDATA_ENTRY (data->new_event), NULL, NULL);

	main_loop = g_main_loop_new (NULL, TRUE);

	gdata_batch_operation_run_async (operation, NULL, (GAsyncReadyCallback) test_batch_async_cb, main_loop);
	g_main_loop_run (main_loop);

	g_main_loop_unref (main_loop);
	g_object_unref (operation);
}

static void
test_batch_async_cancellation_cb (GDataBatchOperation *operation, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GError *error = NULL;

	/* Clear all pending events (such as callbacks for the operations) */
	while (g_main_context_iteration (NULL, FALSE) == TRUE);

	g_assert (gdata_test_batch_operation_run_finish (operation, async_result, &error) == FALSE);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
}

static void
test_batch_async_cancellation (BatchAsyncData *data, gconstpointer service)
{
	GDataBatchOperation *operation;
	guint op_id;
	GMainLoop *main_loop;
	GCancellable *cancellable;
	GError *error = NULL;

	/* Run an async query operation on the event */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "https://www.google.com/calendar/feeds/default/private/full/batch");
	op_id = gdata_test_batch_operation_query (operation, gdata_entry_get_id (GDATA_ENTRY (data->new_event)), GDATA_TYPE_CALENDAR_EVENT,
	                                          GDATA_ENTRY (data->new_event), NULL, &error);

	main_loop = g_main_loop_new (NULL, TRUE);
	cancellable = g_cancellable_new ();

	gdata_batch_operation_run_async (operation, cancellable, (GAsyncReadyCallback) test_batch_async_cancellation_cb, main_loop);
	g_cancellable_cancel (cancellable); /* this should cancel the operation before it even starts, as we haven't run the main loop yet */

	g_main_loop_run (main_loop);

	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_clear_error (&error);

	g_main_loop_unref (main_loop);
	g_object_unref (cancellable);
	g_object_unref (operation);
}

static void
teardown_batch_async (BatchAsyncData *data, gconstpointer service)
{
	GError *error = NULL;

	/* Delete the event */
	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), GDATA_ENTRY (data->new_event), NULL, &error) == TRUE);
	g_assert_no_error (error);
	g_clear_error (&error);

	g_object_unref (data->new_event);
}

int
main (int argc, char *argv[])
{
	gint retval;
	GDataService *service = NULL;

	gdata_test_init (argc, argv);

	if (gdata_test_internet () == TRUE) {
		service = GDATA_SERVICE (gdata_calendar_service_new (CLIENT_ID));
		gdata_service_authenticate (service, USERNAME, PASSWORD, NULL, NULL);

		g_test_add_func ("/calendar/authentication", test_authentication);
		g_test_add_func ("/calendar/authentication_async", test_authentication_async);

		g_test_add_data_func ("/calendar/query/all_calendars", service, test_query_all_calendars);
		g_test_add_data_func ("/calendar/query/all_calendars_async", service, test_query_all_calendars_async);
		g_test_add_data_func ("/calendar/query/own_calendars", service, test_query_own_calendars);
		g_test_add_data_func ("/calendar/query/own_calendars_async", service, test_query_own_calendars_async);
		g_test_add_data_func ("/calendar/query/events", service, test_query_events);
		g_test_add_data_func ("/calendar/query/events_async", service, test_query_events_async);

		g_test_add_data_func ("/calendar/insert/simple", service, test_insert_simple);

		g_test_add_data_func ("/calendar/acls/get_rules", service, test_acls_get_rules);
		g_test_add_data_func ("/calendar/acls/insert_rule", service, test_acls_insert_rule);
		g_test_add_data_func ("/calendar/acls/update_rule", service, test_acls_update_rule);
		g_test_add_data_func ("/calendar/acls/delete_rule", service, test_acls_delete_rule);

		g_test_add_data_func ("/calendar/batch", service, test_batch);
		g_test_add ("/calendar/batch/async", BatchAsyncData, service, setup_batch_async, test_batch_async, teardown_batch_async);
		g_test_add ("/calendar/batch/async/cancellation", BatchAsyncData, service, setup_batch_async, test_batch_async_cancellation,
		            teardown_batch_async);
	}

	g_test_add_func ("/calendar/xml/dates", test_xml_dates);
	g_test_add_func ("/calendar/xml/recurrence", test_xml_recurrence);

	g_test_add_func ("/calendar/calendar/escaping", test_calendar_escaping);
	g_test_add_func ("/calendar/event/escaping", test_event_escaping);

	g_test_add_func ("/calendar/query/uri", test_query_uri);
	g_test_add_func ("/calendar/query/etag", test_query_etag);

	retval = g_test_run ();

	if (service != NULL)
		g_object_unref (service);

	return retval;
}
