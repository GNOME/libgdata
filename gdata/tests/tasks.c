/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2014 <philip@tecnocode.co.uk>
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

static UhmServer *mock_server = NULL;  /* owned */

/* Test that building a query URI works with the various parameters. */
static void
test_query_uri (void)
{
	gchar *query_uri;
	GDataTasksQuery *query = gdata_tasks_query_new (NULL);

	/* General properties. */
	gdata_query_set_updated_min (GDATA_QUERY (query), 6789);
	g_assert_cmpint (gdata_query_get_updated_min (GDATA_QUERY (query)), ==,
	                                              6789);

	gdata_query_set_max_results (GDATA_QUERY (query), 10);
	g_assert_cmpint (gdata_query_get_max_results (GDATA_QUERY (query)), ==,
	                                              10);

	/* Google-Tasks-specific properties. */
	gdata_tasks_query_set_completed_max (query, 1234);
	g_assert_cmpint (gdata_tasks_query_get_completed_max (query), ==, 1234);

	gdata_tasks_query_set_completed_min (query, 5678);
	g_assert_cmpint (gdata_tasks_query_get_completed_min (query), ==, 5678);

	gdata_tasks_query_set_due_max (query, 3456);
	g_assert_cmpint (gdata_tasks_query_get_due_max (query), ==, 3456);

	gdata_tasks_query_set_due_min (query, 2345);
	g_assert_cmpint (gdata_tasks_query_get_due_min (query), ==, 2345);

	gdata_tasks_query_set_show_completed (query, TRUE);
	g_assert (gdata_tasks_query_get_show_completed (query));

	gdata_tasks_query_set_show_deleted (query, TRUE);
	g_assert (gdata_tasks_query_get_show_deleted (query));

	gdata_tasks_query_set_show_hidden (query, TRUE);
	g_assert (gdata_tasks_query_get_show_hidden (query));

	/* Test the URI. */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query),
	                                       "http://example.com");
	g_assert_cmpstr (query_uri, ==,
	                 "http://example.com"
	                 "?maxResults=10"
	                 "&updatedMin=1970-01-01T01:53:09.000001+00:00"
	                 "&completedMin=1970-01-01T01:34:38.000001+00:00"
	                 "&completedMax=1970-01-01T00:20:34.000001+00:00"
	                 "&dueMin=1970-01-01T00:39:05.000001+00:00"
	                 "&dueMax=1970-01-01T00:57:36.000001+00:00"
	                 "&showCompleted=true"
	                 "&showDeleted=true"
	                 "&showHidden=true");
	g_free (query_uri);

	/* Flip the booleans and try again. */
	gdata_tasks_query_set_show_completed (query, FALSE);
	g_assert (!gdata_tasks_query_get_show_completed (query));

	gdata_tasks_query_set_show_deleted (query, FALSE);
	g_assert (!gdata_tasks_query_get_show_deleted (query));

	gdata_tasks_query_set_show_hidden (query, FALSE);
	g_assert (!gdata_tasks_query_get_show_hidden (query));

	/* Test the URI. */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query),
	                                       "http://example.com");
	g_assert_cmpstr (query_uri, ==,
	                 "http://example.com"
	                 "?maxResults=10"
	                 "&updatedMin=1970-01-01T01:53:09.000001+00:00"
	                 "&completedMin=1970-01-01T01:34:38.000001+00:00"
	                 "&completedMax=1970-01-01T00:20:34.000001+00:00"
	                 "&dueMin=1970-01-01T00:39:05.000001+00:00"
	                 "&dueMax=1970-01-01T00:57:36.000001+00:00"
	                 "&showCompleted=false"
	                 "&showDeleted=false"
	                 "&showHidden=false");
	g_free (query_uri);

	/* TODO: pageToken */

	g_object_unref (query);
}

/* Test that setting any property will unset the ETag on a query. */
static void
test_query_etag (void)
{
	GDataTasksQuery *query = NULL;  /* owned */

	query = gdata_tasks_query_new (NULL);

#define CHECK_ETAG(C) \
	gdata_query_set_etag (GDATA_QUERY (query), "foobar");		\
	(C);								\
	g_assert (gdata_query_get_etag (GDATA_QUERY (query)) == NULL);

	CHECK_ETAG (gdata_query_set_max_results (GDATA_QUERY (query), 50))
	CHECK_ETAG (gdata_tasks_query_set_show_deleted (query, FALSE))
	CHECK_ETAG (gdata_query_set_updated_min (GDATA_QUERY (query), 1234))
	CHECK_ETAG (gdata_tasks_query_set_completed_min (query, 4567))

#undef CHECK_ETAG

	g_object_unref (query);
}

/* Test that getting/setting query properties works. */
static void
test_query_properties (void)
{
	GDataTasksQuery *query = NULL;  /* owned */
	gint64 completed_min, completed_max, due_min, due_max;
	gboolean show_completed, show_deleted, show_hidden;

	query = gdata_tasks_query_new (NULL);

	/* Set the properties. */
	g_object_set (G_OBJECT (query),
	              "completed-min", (gint64) 1234,
	              "completed-max", (gint64) 2345,
	              "due-min", (gint64) 3456,
	              "due-max", (gint64) 4567,
	              "show-completed", TRUE,
	              "show-deleted", TRUE,
	              "show-hidden", TRUE,
	              NULL);

	/* Check the query’s properties. */
	g_object_get (G_OBJECT (query),
	              "completed-min", &completed_min,
	              "completed-max", &completed_max,
	              "due-min", &due_min,
	              "due-max", &due_max,
	              "show-completed", &show_completed,
	              "show-deleted", &show_deleted,
	              "show-hidden", &show_hidden,
	              NULL);

	g_assert_cmpint (completed_min, ==, 1234);
	g_assert_cmpint (completed_max, ==, 2345);
	g_assert_cmpint (due_min, ==, 3456);
	g_assert_cmpint (due_max, ==, 4567);
	g_assert (show_completed);
	g_assert (show_deleted);
	g_assert (show_hidden);

	g_object_unref (query);
}

/* Test that getting/setting task properties works. */
static void
test_task_properties (void)
{
	GDataTasksTask *task;
	gchar *id, *etag, *title, *parent, *position, *notes, *status;
	gint64 updated, due, completed;
	gboolean is_deleted, is_hidden;

	task = gdata_tasks_task_new (NULL);

	/* Check the kind is present and correct */
	g_assert (GDATA_IS_TASKS_TASK (task));
	gdata_test_compare_kind (GDATA_ENTRY (task), "tasks#task", NULL);

	/* Set all the properties of the object. */
	gdata_entry_set_title (GDATA_ENTRY (task), "some-title");
	gdata_tasks_task_set_notes (task, "some-notes");
	gdata_tasks_task_set_status (task, GDATA_TASKS_STATUS_NEEDS_ACTION);
	gdata_tasks_task_set_due (task, 1409419209);
	gdata_tasks_task_set_completed (task, 1409419200);  /* 9 seconds to spare! */
	gdata_tasks_task_set_is_deleted (task, FALSE);

	/* Check the properties of the object */
	g_object_get (G_OBJECT (task),
	              "id", &id,
	              "etag", &etag,
	              "title", &title,
	              "updated", &updated,
	              "parent", &parent,
	              "position", &position,
	              "notes", &notes,
	              "status", &status,
	              "due", &due,
	              "completed", &completed,
	              "is-deleted", &is_deleted,
	              "is-hidden", &is_hidden,
	              NULL);

	g_assert_cmpstr (id, ==, NULL);
	g_assert_cmpstr (etag, ==, NULL);
	g_assert_cmpstr (title, ==, "some-title");
	g_assert_cmpint (updated, ==, -1);
	g_assert_cmpstr (parent, ==, NULL);
	g_assert_cmpstr (position, ==, NULL);
	g_assert_cmpstr (notes, ==, "some-notes");
	g_assert_cmpstr (status, ==, GDATA_TASKS_STATUS_NEEDS_ACTION);
	g_assert_cmpint (due, ==, 1409419209);
	g_assert_cmpint (completed, ==, 1409419200);
	g_assert (!is_deleted);
	g_assert (!is_hidden);

	g_free (status);
	g_free (notes);
	g_free (position);
	g_free (parent);
	g_free (title);
	g_free (etag);
	g_free (id);

	/* Set the properties another way. */
	g_object_set (G_OBJECT (task),
	              "title", "some-other-title",
	              "notes", "more-notes",
	              "status", GDATA_TASKS_STATUS_COMPLETED,
	              "due", (gint64) 1409419200,
	              "completed", (gint64) 1409419200,  /* no time to spare! */
	              "is-deleted", TRUE,
	              NULL);

	/* Check the properties using the getters. */
	g_assert_cmpstr (gdata_tasks_task_get_parent (task), ==, NULL);
	g_assert_cmpstr (gdata_tasks_task_get_position (task), ==, NULL);
	g_assert_cmpstr (gdata_tasks_task_get_notes (task), ==, "more-notes");
	g_assert_cmpstr (gdata_tasks_task_get_status (task), ==,
	                 GDATA_TASKS_STATUS_COMPLETED);
	g_assert_cmpint (gdata_tasks_task_get_due (task), ==, 1409419200);
	g_assert_cmpint (gdata_tasks_task_get_completed (task), ==, 1409419200);
	g_assert (gdata_tasks_task_is_deleted (task));
	g_assert (!gdata_tasks_task_is_hidden (task));

	/* Check the JSON. */
	gdata_test_assert_json (task,
		"{"
			"\"kind\": \"tasks#task\","
			"\"title\": \"some-other-title\","
			"\"notes\": \"more-notes\","
			"\"status\": \"completed\","
			"\"due\": \"2014-08-30T17:20:00.000001+00:00\","
			"\"completed\": \"2014-08-30T17:20:00.000001+00:00\","
			"\"deleted\": true,"
			"\"hidden\": false"
		"}");

	/* Try again, marking it as undeleted. */
	gdata_tasks_task_set_is_deleted (task, FALSE);

	gdata_test_assert_json (task,
		"{"
			"\"kind\": \"tasks#task\","
			"\"title\": \"some-other-title\","
			"\"notes\": \"more-notes\","
			"\"status\": \"completed\","
			"\"due\": \"2014-08-30T17:20:00.000001+00:00\","
			"\"completed\": \"2014-08-30T17:20:00.000001+00:00\","
			"\"deleted\": false,"
			"\"hidden\": false"
		"}");

	g_object_unref (task);
}

/* Test that escaping task properties for JSON works. */
static void
test_task_escaping (void)
{
	GDataTasksTask *task;

	task = gdata_tasks_task_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (task), "Title \"with quotes\"");
	gdata_tasks_task_set_notes (task, "Notes \"with quotes\" and Emoji 😂.");
	gdata_tasks_task_set_status (task, "invalid status \"with quotes\"");

	/* Check the outputted JSON is escaped properly. */
	gdata_test_assert_json (task,
		"{"
			"\"kind\": \"tasks#task\","
			"\"title\": \"Title \\\"with quotes\\\"\","
			"\"notes\": \"Notes \\\"with quotes\\\" and Emoji 😂.\","
			"\"status\": \"invalid status \\\"with quotes\\\"\","
			"\"deleted\": false,"
			"\"hidden\": false"
		"}");

	g_object_unref (task);
}

/* Test the task parser with the minimal number of properties specified. */
static void
test_task_parser_minimal (void)
{
	GDataTasksTask *task = NULL;  /* owned */
	GDataEntry *entry;  /* unowned */
	GDataLink *self_link;  /* unowned */
	GError *error = NULL;

	task = GDATA_TASKS_TASK (gdata_parsable_new_from_json (GDATA_TYPE_TASKS_TASK,
		"{"
			"\"kind\": \"tasks#task\","
			"\"id\": \"some-id\","
			"\"title\": \"some-title \\\"with quotes\\\"\","
			"\"updated\": \"2014-08-30T19:40:00Z\","
			"\"selfLink\": \"http://some-uri/\","
			"\"position\": \"some-position\","
			"\"status\": \"needsAction\","
			"\"deleted\": true,"
			"\"hidden\": true"
		"}", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_TASKS_TASK (task));
	gdata_test_compare_kind (GDATA_ENTRY (task), "tasks#task", NULL);

	entry = GDATA_ENTRY (task);

	/* Check the task’s properties. */
	g_assert_cmpstr (gdata_entry_get_id (entry), ==, "some-id");
	g_assert_cmpstr (gdata_entry_get_etag (entry), ==, NULL);
	g_assert_cmpstr (gdata_entry_get_title (entry), ==,
	                 "some-title \"with quotes\"");
	g_assert_cmpint (gdata_entry_get_updated (entry), ==, 1409427600);
	g_assert_cmpstr (gdata_tasks_task_get_parent (task), ==, NULL);
	g_assert_cmpstr (gdata_tasks_task_get_notes (task), ==, NULL);
	g_assert_cmpstr (gdata_tasks_task_get_status (task), ==,
	                 GDATA_TASKS_STATUS_NEEDS_ACTION);
	g_assert_cmpint (gdata_tasks_task_get_due (task), ==, -1);
	g_assert_cmpint (gdata_tasks_task_get_completed (task), ==, -1);
	g_assert (gdata_tasks_task_is_deleted (task));
	g_assert (gdata_tasks_task_is_hidden (task));

	self_link = gdata_entry_look_up_link (entry, GDATA_LINK_SELF);
	g_assert (GDATA_IS_LINK (self_link));
	g_assert_cmpstr (gdata_link_get_uri (self_link), ==,
	                 "http://some-uri/");
	g_assert_cmpstr (gdata_link_get_relation_type (self_link), ==,
	                 GDATA_LINK_SELF);
	g_assert_cmpstr (gdata_link_get_content_type (self_link), ==, NULL);
	g_assert_cmpstr (gdata_link_get_language (self_link), ==, NULL);
	g_assert_cmpstr (gdata_link_get_title (self_link), ==, NULL);
	g_assert_cmpint (gdata_link_get_length (self_link), ==, -1);

	g_object_unref (task);
}

/* Test the task parser with a maximal number of properties specified. */
static void
test_task_parser_normal (void)
{
	GDataTasksTask *task = NULL;  /* owned */
	GDataEntry *entry;  /* unowned */
	GDataLink *self_link;  /* unowned */
	GError *error = NULL;

	task = GDATA_TASKS_TASK (gdata_parsable_new_from_json (GDATA_TYPE_TASKS_TASK,
		"{"
			"\"kind\": \"tasks#task\","
			"\"id\": \"some-id\","
			"\"etag\": \"some-etag\","
			"\"title\": \"some-title \\\"with quotes\\\"\","
			"\"updated\": \"2014-08-30T19:40:00Z\","
			"\"selfLink\": \"http://some-uri/\","
			"\"parent\": \"some-parent-id\","
			"\"position\": \"some-position\","
			"\"notes\": \"Some notes!\","
			"\"status\": \"needsAction\","
			"\"due\": \"2014-08-30T20:00:00Z\","
			"\"completed\": \"2014-08-30T20:10:05Z\","
			"\"deleted\": false,"
			"\"hidden\": true,"
			/* Unhandled for the moment: */
			"\"links\": ["
				"{"
					"\"type\": \"email\","
					"\"description\": \"some-email\","
					"\"link\": \"example@example.com\""
				"}"
			"]"
		"}", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_TASKS_TASK (task));
	gdata_test_compare_kind (GDATA_ENTRY (task), "tasks#task", NULL);

	entry = GDATA_ENTRY (task);

	/* Check the task’s properties. */
	g_assert_cmpstr (gdata_entry_get_id (entry), ==, "some-id");
	g_assert_cmpstr (gdata_entry_get_etag (entry), ==, "some-etag");
	g_assert_cmpstr (gdata_entry_get_title (entry), ==,
	                 "some-title \"with quotes\"");
	g_assert_cmpint (gdata_entry_get_updated (entry), ==, 1409427600);
	g_assert_cmpstr (gdata_tasks_task_get_parent (task), ==,
	                 "some-parent-id");
	g_assert_cmpstr (gdata_tasks_task_get_notes (task), ==, "Some notes!");
	g_assert_cmpstr (gdata_tasks_task_get_status (task), ==,
	                 GDATA_TASKS_STATUS_NEEDS_ACTION);
	g_assert_cmpint (gdata_tasks_task_get_due (task), ==, 1409428800);
	g_assert_cmpint (gdata_tasks_task_get_completed (task), ==, 1409429405);
	g_assert (!gdata_tasks_task_is_deleted (task));
	g_assert (gdata_tasks_task_is_hidden (task));

	self_link = gdata_entry_look_up_link (entry, GDATA_LINK_SELF);
	g_assert (GDATA_IS_LINK (self_link));
	g_assert_cmpstr (gdata_link_get_uri (self_link), ==,
	                 "http://some-uri/");
	g_assert_cmpstr (gdata_link_get_relation_type (self_link), ==,
	                 GDATA_LINK_SELF);
	g_assert_cmpstr (gdata_link_get_content_type (self_link), ==, NULL);
	g_assert_cmpstr (gdata_link_get_language (self_link), ==, NULL);
	g_assert_cmpstr (gdata_link_get_title (self_link), ==, NULL);
	g_assert_cmpint (gdata_link_get_length (self_link), ==, -1);

	/* Check that the same JSON is re-generated. */
	gdata_test_assert_json (task,
		"{"
			"\"kind\": \"tasks#task\","
			"\"id\": \"some-id\","
			"\"etag\": \"some-etag\","
			"\"title\": \"some-title \\\"with quotes\\\"\","
			"\"updated\": \"2014-08-30T19:40:00.000001+00:00\","
			"\"selfLink\": \"http://some-uri/\","
			"\"parent\": \"some-parent-id\","
			"\"position\": \"some-position\","
			"\"notes\": \"Some notes!\","
			"\"status\": \"needsAction\","
			"\"due\": \"2014-08-30T20:00:00.000001+00:00\","
			"\"completed\": \"2014-08-30T20:10:05.000001+00:00\","
			"\"deleted\": false,"
			"\"hidden\": true,"
			/* Unhandled for the moment: */
			"\"links\": ["
				"{"
					"\"type\": \"email\","
					"\"description\": \"some-email\","
					"\"link\": \"example@example.com\""
				"}"
			"]"
		"}");

	g_object_unref (task);
}

/* Test that getting/setting tasklist properties works. */
static void
test_tasklist_properties (void)
{
	GDataTasksTasklist *tasklist;
	gchar *id, *etag, *title;
	gint64 updated;

	tasklist = gdata_tasks_tasklist_new (NULL);

	/* Check the kind is present and correct */
	g_assert (GDATA_IS_TASKS_TASKLIST (tasklist));
	gdata_test_compare_kind (GDATA_ENTRY (tasklist),
	                         "tasks#taskList", NULL);

	/* Set all the properties of the object. */
	gdata_entry_set_title (GDATA_ENTRY (tasklist), "some-title");

	/* Check the properties of the object */
	g_object_get (G_OBJECT (tasklist),
	              "id", &id,
	              "etag", &etag,
	              "title", &title,
	              "updated", &updated,
	              NULL);

	g_assert_cmpstr (id, ==, NULL);
	g_assert_cmpstr (etag, ==, NULL);
	g_assert_cmpstr (title, ==, "some-title");
	g_assert_cmpint (updated, ==, -1);

	g_free (title);
	g_free (etag);
	g_free (id);

	/* Check the properties using the getters. */
	g_assert_cmpstr (gdata_entry_get_id (GDATA_ENTRY (tasklist)), ==, NULL);
	g_assert_cmpstr (gdata_entry_get_etag (GDATA_ENTRY (tasklist)), ==,
	                 NULL);
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (tasklist)), ==,
	                 "some-title");
	g_assert_cmpint (gdata_entry_get_updated (GDATA_ENTRY (tasklist)), ==,
	                 -1);

	/* Check the JSON. */
	gdata_test_assert_json (tasklist,
		"{"
			"\"kind\": \"tasks#taskList\","
			"\"title\": \"some-title\""
		"}");

	g_object_unref (tasklist);
}

/* Test that escaping tasklist properties for JSON works. */
static void
test_tasklist_escaping (void)
{
	GDataTasksTasklist *tasklist;

	tasklist = gdata_tasks_tasklist_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (tasklist), "Title \"with quotes\"");

	/* Check the outputted JSON is escaped properly. */
	gdata_test_assert_json (tasklist,
		"{"
			"\"kind\": \"tasks#taskList\","
			"\"title\": \"Title \\\"with quotes\\\"\""
		"}");

	g_object_unref (tasklist);
}

/* Test the tasklist parser with a maximal number of properties specified. */
static void
test_tasklist_parser_normal (void)
{
	GDataTasksTasklist *tasklist = NULL;  /* owned */
	GDataEntry *entry;  /* unowned */
	GDataLink *self_link;  /* unowned */
	GError *error = NULL;

	tasklist = GDATA_TASKS_TASKLIST (gdata_parsable_new_from_json (GDATA_TYPE_TASKS_TASKLIST,
		"{"
			"\"kind\": \"tasks#taskList\","
			"\"id\": \"some-id\","
			"\"etag\": \"some-etag\","
			"\"title\": \"some-title \\\"with quotes\\\"\","
			"\"updated\": \"2014-08-30T19:40:00Z\","
			"\"selfLink\": \"http://some-uri/\""
		"}", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_TASKS_TASKLIST (tasklist));
	gdata_test_compare_kind (GDATA_ENTRY (tasklist),
	                         "tasks#taskList", NULL);

	entry = GDATA_ENTRY (tasklist);

	/* Check the tasklist’s properties. */
	g_assert_cmpstr (gdata_entry_get_id (entry), ==, "some-id");
	g_assert_cmpstr (gdata_entry_get_etag (entry), ==, "some-etag");
	g_assert_cmpstr (gdata_entry_get_title (entry), ==,
	                 "some-title \"with quotes\"");
	g_assert_cmpint (gdata_entry_get_updated (entry), ==, 1409427600);

	self_link = gdata_entry_look_up_link (entry, GDATA_LINK_SELF);
	g_assert (GDATA_IS_LINK (self_link));
	g_assert_cmpstr (gdata_link_get_uri (self_link), ==,
	                 "http://some-uri/");
	g_assert_cmpstr (gdata_link_get_relation_type (self_link), ==,
	                 GDATA_LINK_SELF);
	g_assert_cmpstr (gdata_link_get_content_type (self_link), ==, NULL);
	g_assert_cmpstr (gdata_link_get_language (self_link), ==, NULL);
	g_assert_cmpstr (gdata_link_get_title (self_link), ==, NULL);
	g_assert_cmpint (gdata_link_get_length (self_link), ==, -1);

	g_object_unref (tasklist);
}

static void
mock_server_notify_resolver_cb (GObject *object, GParamSpec *pspec,
                                gpointer user_data)
{
	UhmServer *server;
	UhmResolver *resolver;

	server = UHM_SERVER (object);

	/* Set up the expected domain names here. This should technically be
	 * split up between the different unit test suites, but that's too much
	 * effort. */
	resolver = uhm_server_get_resolver (server);

	if (resolver != NULL) {
		const gchar *ip_address = uhm_server_get_address (server);

		uhm_resolver_add_A (resolver, "www.google.com", ip_address);
		uhm_resolver_add_A (resolver, "www.googleapis.com", ip_address);
	}
}

/* Set up a global GDataAuthorizer to be used for all the tests. Unfortunately,
 * the Google Tasks API is limited to OAuth1 and OAuth2 authorisation, so this
 * requires user interaction when online.
 *
 * If not online, use a dummy authoriser. */
static GDataAuthorizer *
create_global_authorizer (void)
{
	GDataOAuth1Authorizer *authorizer = NULL;  /* owned */
	gchar *authentication_uri, *token, *token_secret, *verifier;
	GError *error = NULL;

	/* If not online, just return a dummy authoriser. */
	if (!uhm_server_get_enable_online (mock_server)) {
		return GDATA_AUTHORIZER (gdata_dummy_authorizer_new (GDATA_TYPE_TASKS_SERVICE));
	}

	/* Otherwise, go through the interactive OAuth dance. */
	gdata_test_mock_server_start_trace (mock_server, "global-authentication");
	authorizer = gdata_oauth1_authorizer_new ("Application name",
	                                          GDATA_TYPE_TASKS_SERVICE);

	/* Get an authentication URI */
	authentication_uri = gdata_oauth1_authorizer_request_authentication_uri (authorizer, &token, &token_secret, NULL, &error);
	g_assert_no_error (error);
	g_assert (authentication_uri != NULL);

	/* Get the verifier off the user.
	 *
	 * FIXME: Won’t work due to nonces in the OAuth protocol. */
	if (uhm_server_get_enable_online (mock_server)) {
		verifier = gdata_test_query_user_for_verifier (authentication_uri);
	}

	g_free (authentication_uri);

	if (verifier == NULL) {
		/* Skip tests. */
		g_object_unref (authorizer);
		authorizer = NULL;
		goto skip_test;
	}

	/* Authorise the token */
	g_assert (gdata_oauth1_authorizer_request_authorization (authorizer, token, token_secret, verifier, NULL, &error));
	g_assert_no_error (error);

skip_test:
	g_free (token);
	g_free (token_secret);
	g_free (verifier);

	uhm_server_end_trace (mock_server);

	return GDATA_AUTHORIZER (authorizer);
}

int
main (int argc, char *argv[])
{
	gint retval;
	GDataAuthorizer *authorizer = NULL;  /* owned */
	GDataService *service = NULL;  /* owned */
	GFile *trace_directory = NULL;  /* owned */

	gdata_test_init (argc, argv);

	mock_server = gdata_test_get_mock_server ();
	g_signal_connect (G_OBJECT (mock_server), "notify::resolver",
	                  (GCallback) mock_server_notify_resolver_cb, NULL);
	trace_directory = g_file_new_for_path (TEST_FILE_DIR "traces/tasks");
	uhm_server_set_trace_directory (mock_server, trace_directory);
	g_object_unref (trace_directory);

	authorizer = create_global_authorizer ();

	service = GDATA_SERVICE (gdata_tasks_service_new (authorizer));

	g_test_add_func ("/tasks/task/properties", test_task_properties);
	g_test_add_func ("/tasks/task/escaping", test_task_escaping);
	g_test_add_func ("/tasks/task/parser/minimal",
	                 test_task_parser_minimal);
	g_test_add_func ("/tasks/task/parser/normal", test_task_parser_normal);

	g_test_add_func ("/tasks/tasklist/properties",
	                 test_tasklist_properties);
	g_test_add_func ("/tasks/tasklist/escaping", test_tasklist_escaping);
	g_test_add_func ("/tasks/tasklist/parser/normal",
	                 test_tasklist_parser_normal);

	g_test_add_func ("/tasks/query/uri", test_query_uri);
	g_test_add_func ("/tasks/query/etag", test_query_etag);
	g_test_add_func ("/tasks/query/properties", test_query_properties);

	retval = g_test_run ();

	g_clear_object (&service);
	g_clear_object (&authorizer);

	return retval;
}
