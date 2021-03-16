/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright © 2017 Philip Withnall <philip@tecnocode.co.uk>
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
	            "   tasklists\n"
	            "   tasks <tasklist ID> [query string]\n",
	            argv[0], argv[0]);
	return -1;
}

static void
print_tasklist (GDataTasksTasklist *tasklist)
{
	const gchar *id, *title, *description;

	id = gdata_entry_get_id (GDATA_ENTRY (tasklist));
	title = gdata_entry_get_title (GDATA_ENTRY (tasklist));
	description = gdata_entry_get_summary (GDATA_ENTRY (tasklist));

	g_print ("%s — %s\n", id, title);
	g_print ("   Description:\n      %s\n", description);

	g_print ("\n");
}

static const gchar *
format_status (const gchar *status)
{
	if (g_strcmp0 (status, GDATA_TASKS_STATUS_NEEDS_ACTION) == 0)
		return "needs action";
	else if (g_strcmp0 (status, GDATA_TASKS_STATUS_COMPLETED) == 0)
		return "completed";
	else
		return status;
}

static void
print_task (GDataTasksTask *task)
{
	const gchar *title, *id, *description, *parent_id, *position, *notes;
	const gchar *status;
	GDateTime *tmp;
	gint64 date_published_tv;
	gchar *date_published = NULL;  /* owned */
	gint64 due_tv;
	gchar *due = NULL;  /* owned */
	gint64 completed_tv;
	gchar *completed = NULL;  /* owned */
	gboolean is_deleted, is_hidden;

	title = gdata_entry_get_title (GDATA_ENTRY (task));
	id = gdata_entry_get_id (GDATA_ENTRY (task));
	description = gdata_entry_get_content (GDATA_ENTRY (task));
	date_published_tv = gdata_entry_get_published (GDATA_ENTRY (task));
	tmp = g_date_time_new_from_unix_utc (date_published_tv);
	date_published = g_date_time_format_iso8601 (tmp);
	g_date_time_unref (tmp);
	parent_id = gdata_tasks_task_get_parent (task);
	position = gdata_tasks_task_get_position (task);
	notes = gdata_tasks_task_get_notes (task);
	status = gdata_tasks_task_get_status (task);
	due_tv = gdata_tasks_task_get_due (task);
	tmp = g_date_time_new_from_unix_utc (due_tv);
	due = g_date_time_format_iso8601 (tmp);
	g_date_time_unref (tmp);
	completed_tv = gdata_tasks_task_get_completed (task);
	tmp = g_date_time_new_from_unix_utc (completed_tv);
	completed = g_date_time_format_iso8601 (tmp);
	g_date_time_unref (tmp);
	is_deleted = gdata_tasks_task_is_deleted (task);
	is_hidden = gdata_tasks_task_is_hidden (task);

	g_print ("%s — %s\n", id, title);
	g_print ("   Published: %s\n", date_published_tv != 0 ? date_published : "unknown");
	g_print ("   Status: %s\n", format_status (status));
	g_print ("   Due: %s\n", due_tv != 0 ? due : "not set");
	g_print ("   Completed: %s\n", completed_tv != 0 ? completed : "not yet");
	g_print ("   Deleted? %s\n", is_deleted ? "Yes" : "No");
	g_print ("   Hidden? %s\n", is_hidden ? "Yes" : "No");
	g_print ("   Position: %s\n", position);
	g_print ("   Parent ID: %s\n", parent_id);
	g_print ("   Description:\n      %s\n", description);
	g_print ("   Notes:\n      %s\n", notes);

	g_print ("\n");

	g_free (completed);
	g_free (due);
	g_free (date_published);
}

/* FIXME: Factor all this code out of all the demos */
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
	                                          GDATA_TYPE_TASKS_SERVICE);

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

/* List all the user’s task-lists. */
static int
command_tasklists (int argc, char *argv[])
{
	GDataTasksService *service = NULL;
	GDataTasksQuery *query = NULL;
	GDataFeed *feed = NULL;
	GList/*<unowned GDataTasksTasklist>*/ *entries;
	GError *error = NULL;
	gint retval = 0;
	GDataAuthorizer *authorizer = NULL;

	if (argc != 2) {
		return print_usage (argv);
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

	service = gdata_tasks_service_new (authorizer);
	query = gdata_tasks_query_new (NULL);

	feed = gdata_tasks_service_query_all_tasklists (service,
	                                                GDATA_QUERY (query),
	                                                NULL, NULL,
	                                                NULL,
	                                                &error);

	if (error != NULL) {
		g_printerr ("%s: Error querying tasklists: %s\n",
		            argv[0], error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	}

	/* Print results. */
	for (entries = gdata_feed_get_entries (feed); entries != NULL;
	     entries = entries->next) {
		GDataTasksTasklist *tasklist;

		tasklist = GDATA_TASKS_TASKLIST (entries->data);
		print_tasklist (tasklist);
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

/* Query the tasks in a tasklist. */
static int
command_tasks (int argc, char *argv[])
{
	GDataTasksService *service = NULL;
	GDataTasksTasklist *tasklist = NULL;
	GDataTasksQuery *query = NULL;
	GError *error = NULL;
	gint retval = 0;
	const gchar *query_string, *tasklist_id;
	GDataAuthorizer *authorizer = NULL;
	guint n_results;

	if (argc < 3) {
		return print_usage (argv);
	}

	tasklist_id = argv[2];
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

	service = gdata_tasks_service_new (authorizer);
	query = gdata_tasks_query_new (query_string);
	gdata_query_set_max_results (GDATA_QUERY (query), 10);
	tasklist = gdata_tasks_tasklist_new (tasklist_id);
	n_results = 0;

	while (TRUE) {
		GList/*<unowned GDataTasksTask>*/ *entries, *l;
		GDataFeed *feed = NULL;

		feed = gdata_tasks_service_query_tasks (service, tasklist,
		                                        GDATA_QUERY (query),
		                                        NULL, NULL, NULL,
		                                        &error);

		if (error != NULL) {
			g_printerr ("%s: Error querying tasks: %s\n",
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
			break;
		}

		for (l = entries; l != NULL; l = l->next) {
			GDataTasksTask *task;

			task = GDATA_TASKS_TASK (l->data);
			print_task (task);
			n_results++;
		}

		gdata_query_next_page (GDATA_QUERY (query));
		g_object_unref (feed);
	}

	g_print ("Total of %u results.\n", n_results);

done:
	g_clear_object (&query);
	g_clear_object (&authorizer);
	g_clear_object (&tasklist);
	g_clear_object (&service);

	return retval;
}

static const struct {
	const gchar *command;
	int (*handler_fn) (int argc, char **argv);
} command_handlers[] = {
	{ "tasklists", command_tasklists },
	{ "tasks", command_tasks },
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
