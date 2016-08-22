/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2016 <philip@tecnocode.co.uk>
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

#include "config.h"

#include <glib.h>
#include <unistd.h>

#include "gdata.h"
#include "common.h"

/* gdata-buffer.h is private, so just include the C file for easy testing. */
#include "gdata-buffer.c"


typedef struct {
	gpointer unused;
} Fixture;


static void
set_up (Fixture *f, gconstpointer user_data)
{
	/* Abort if we end up blocking. */
	alarm (30);
}

static void
tear_down (Fixture *f, gconstpointer user_data)
{
	/* Reset the alarm. */
	alarm (0);
}

static void
test_buffer_construction (Fixture *f, gconstpointer user_data)
{
	GDataBuffer *buffer = NULL;  /* owned */

	buffer = gdata_buffer_new ();
	gdata_buffer_free (buffer);
}

static void
test_buffer_instant_eof (Fixture *f, gconstpointer user_data)
{
	GDataBuffer *buffer = NULL;  /* owned */
	gboolean reached_eof = FALSE;
	guint8 buf[1];

	buffer = gdata_buffer_new ();

	g_assert_false (gdata_buffer_push_data (buffer, NULL, 0));
	g_assert_cmpuint (gdata_buffer_pop_data (buffer, buf, sizeof (buf),
	                                         &reached_eof, NULL), ==, 0);
	g_assert_true (reached_eof);

	gdata_buffer_free (buffer);
}

static gpointer
test_buffer_thread_eof_func (gpointer user_data)
{
	GDataBuffer *buffer = user_data;

	/* HACK: Wait for a while to be sure that gdata_buffer_pop_data() has
	 * been already called. */
	g_usleep (G_USEC_PER_SEC / 2);

	g_assert_false (gdata_buffer_push_data (buffer, NULL, 0));

	return NULL;
}

/* The test needs to call gdata_buffer_push_data() from another thread only
 * once gdata_buffer_pop_data() has reached its blocking loop. */
static void
test_buffer_thread_eof (Fixture *f, gconstpointer user_data)
{
	GDataBuffer *buffer = NULL;  /* owned */
	gboolean reached_eof = FALSE;
	guint8 buf[1];

	g_test_bug ("769727");

	buffer = gdata_buffer_new ();

	g_thread_new (NULL, test_buffer_thread_eof_func, buffer);
	g_assert_cmpuint (gdata_buffer_pop_data (buffer, buf, sizeof (buf),
	                                         &reached_eof, NULL), ==, 0);
	g_assert_true (reached_eof);

	gdata_buffer_free (buffer);
}

static void
test_buffer_basic (Fixture *f, gconstpointer user_data)
{
	GDataBuffer *buffer = NULL;  /* owned */
	gboolean reached_eof = FALSE;
	guint8 buf[100];
	guint8 buf2[100];
	gsize i;

	buffer = gdata_buffer_new ();

	for (i = 0; i < sizeof (buf); i++)
		buf[i] = i;

	g_assert_true (gdata_buffer_push_data (buffer, buf, sizeof (buf)));
	g_assert_false (gdata_buffer_push_data (buffer, NULL, 0));

	g_assert_cmpuint (gdata_buffer_pop_data (buffer, buf2,
	                                         sizeof (buf2) / 2,
	                                         &reached_eof, NULL), ==,
	                                         sizeof (buf2) / 2);
	g_assert_false (reached_eof);
	g_assert_cmpuint (gdata_buffer_pop_data (buffer,
	                                         buf2 + sizeof (buf2) / 2,
	                                         sizeof (buf2) / 2,
	                                         &reached_eof, NULL), ==,
	                                         sizeof (buf2) / 2);
	g_assert_true (reached_eof);

	for (i = 0; i < sizeof (buf); i++)
		g_assert_cmpuint (buf[i], ==, buf2[i]);

	gdata_buffer_free (buffer);
}

int
main (int argc, char *argv[])
{
	gdata_test_init (argc, argv);

	/* Only print out headers, since we're sending a lot of data. */
	g_setenv ("LIBGDATA_DEBUG", "2" /* GDATA_LOG_HEADERS */, TRUE);

	g_test_add ("/buffer/construction", Fixture, NULL,
	            set_up, test_buffer_construction, tear_down);
	g_test_add ("/buffer/instant-eof", Fixture, NULL,
	            set_up, test_buffer_instant_eof, tear_down);
	g_test_add ("/buffer/thread-eof", Fixture, NULL,
	            set_up, test_buffer_thread_eof, tear_down);
	g_test_add ("/buffer/basic", Fixture, NULL,
	            set_up, test_buffer_basic, tear_down);

	return g_test_run ();
}
