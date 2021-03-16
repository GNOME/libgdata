/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2010, 2015 <philip@tecnocode.co.uk>
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
#include <stdio.h>

#include "gdata.h"
#include "common.h"

static void
test_parse_feed (void)
{
	GDataFeed *feed;
	GError *error = NULL;

	feed = GDATA_FEED (gdata_parsable_new_from_xml (GDATA_TYPE_FEED,
		"<feed xmlns='http://www.w3.org/2005/Atom' "
		      "xmlns:openSearch='http://a9.com/-/spec/opensearch/1.1/' "
		      "xmlns:gd='http://schemas.google.com/g/2005' "
		      "gd:etag='W/\"D08FQn8-eil7ImA9WxZbFEw.\"'>"
			"<id>http://example.com/id</id>"
			"<updated>2009-02-25T14:07:37.880860Z</updated>"
			"<title type='text'>Test feed</title>"
			"<subtitle type='text'>Test subtitle</subtitle>"
			"<logo>http://example.com/logo.png</logo>"
			"<icon>http://example.com/icon.png</icon>"
			"<link rel='alternate' type='text/html' href='http://alternate.example.com/'/>"
			"<link rel='http://schemas.google.com/g/2005#feed' type='application/atom+xml' href='http://example.com/id'/>"
			"<link rel='http://schemas.google.com/g/2005#post' type='application/atom+xml' href='http://example.com/post'/>"
			"<link rel='self' type='application/atom+xml' href='http://example.com/id'/>"
			"<category scheme='http://example.com/categories' term='feed'/>"
			"<author>"
				"<name>Joe Smith</name>"
				"<email>j.smith@example.com</email>"
			"</author>"
			"<generator version='0.6' uri='http://example.com/'>Example Generator</generator>"
			"<openSearch:totalResults>2</openSearch:totalResults>"
			"<openSearch:startIndex>0</openSearch:startIndex>"
			"<openSearch:itemsPerPage>50</openSearch:itemsPerPage>"
			"<entry>"
				"<id>entry1</id>"
				"<title type='text'>Testing unhandled XML</title>"
				"<updated>2009-01-25T14:07:37.880860Z</updated>"
				"<published>2009-01-23T14:06:37.880860Z</published>"
				"<content type='text'>Here we test unhandled XML elements.</content>"
			"</entry>"
			"<entry>"
				"<id>entry2</id>"
				"<title type='text'>Testing unhandled XML 2</title>"
				"<updated>2009-02-25T14:07:37.880860Z</updated>"
				"<published>2009-02-23T14:06:37.880860Z</published>"
				"<content type='text'>Here we test unhandled XML elements again.</content>"
			"</entry>"
		"</feed>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	g_object_unref (feed);
}

static void
test_perf_parsing (void)
{
	GDateTime *start_time, *end_time;
	GTimeSpan total_time, per_iteration_time;
	guint i;

	#define ITERATIONS 10000

	/* Test feed parsing time */
	start_time = g_date_time_new_now_utc ();
	for (i = 0; i < ITERATIONS; i++)
		test_parse_feed ();
	end_time = g_date_time_new_now_utc ();

	total_time = g_date_time_difference (end_time, start_time);
	per_iteration_time = total_time / ITERATIONS;

	g_date_time_unref (start_time);
	g_date_time_unref (end_time);

	/* Prefix with hashes to avoid the output being misinterpreted as TAP
	 * commands. */
	printf ("# Parsing a feed %u times took:\n"
	        "#  • Total: %.4fs\n"
	        "#  • Per iteration: %.4fs\n",
	        ITERATIONS,
	        (gdouble) total_time / (gdouble) G_USEC_PER_SEC,
	        (gdouble) per_iteration_time / (gdouble) G_USEC_PER_SEC);

	g_assert_cmpuint (per_iteration_time, <, 2000);  /* 2ms */
}

int
main (int argc, char *argv[])
{
	gdata_test_init (argc, argv);

	g_test_add_func ("/perf/parsing", test_perf_parsing);

	return g_test_run ();
}
