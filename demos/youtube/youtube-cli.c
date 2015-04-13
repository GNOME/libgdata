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

#define DEVELOPER_KEY "AI39si7Me3Q7zYs6hmkFvpRBD2nrkVjYYsUO5lh_3HdOkGRc9g6Z4nzxZatk_aAo2EsA21k7vrda0OO6oFg2rnhMedZXPyXoEw"

static int
print_usage (char *argv[])
{
	g_printerr ("%s: Usage — %s search <term>\n", argv[0], argv[0]);
	return -1;
}

static int
command_search (char *argv[])
{
	GDataYouTubeService *service = NULL;
	GDataYouTubeQuery *query = NULL;
	GDataFeed *feed = NULL;
	GList *entries;
	GError *error = NULL;
	gint retval = 0;

	service = gdata_youtube_service_new (DEVELOPER_KEY, NULL);
	query = gdata_youtube_query_new (argv[2]);
	feed = gdata_youtube_service_query_videos (service, GDATA_QUERY (query),
	                                           NULL, NULL, NULL, &error);

	if (error != NULL) {
		g_printerr ("%s: Error querying YouTube: %s\n",
		            argv[0], error->message);
		g_error_free (error);
		retval = 1;
		goto done;
	}

	/* Print results. */
	for (entries = gdata_feed_get_entries (feed); entries != NULL;
	     entries = entries->next) {
		GDataYouTubeVideo *video;
		const gchar *title, *player_uri;

		video = GDATA_YOUTUBE_VIDEO (entries->data);
		title = gdata_entry_get_title (GDATA_ENTRY (video));
		player_uri = gdata_youtube_video_get_player_uri (video);

		g_print ("%s — %s\n", player_uri, title);
	}

	if (gdata_feed_get_entries (feed) == NULL) {
		g_print ("No results.\n");
	}

done:
	g_clear_object (&feed);
	g_clear_object (&query);
	g_clear_object (&service);

	return retval;
}

static const struct {
	const gchar *command;
	int (*handler_fn) (char **argv);
} command_handlers[] = {
	{ "search", command_search },
};

int
main (int argc, char *argv[])
{
	guint i;
	gint retval = -1;

	setlocale (LC_ALL, "");

	if (argc < 3) {
		return print_usage (argv);
	}

	for (i = 0; i < G_N_ELEMENTS (command_handlers); i++) {
		if (strcmp (argv[1], command_handlers[i].command) == 0) {
			retval = command_handlers[i].handler_fn (argv);
		}
	}

	if (retval == -1) {
		retval = print_usage (argv);
	}

	return retval;
}
