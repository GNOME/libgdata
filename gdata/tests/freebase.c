/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData freebase tests
 * Copyright (C) Carlos Garnacho 2014 <carlosg@gnome.org>
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
#include "gdata.h"
#include "common.h"

static UhmServer *mock_server = NULL;  /* owned */
static GDataService *service = NULL; /* owned */

typedef struct {
	GMainLoop *main_loop;
	gpointer data;
	GError *error;
} AsyncClosure;

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
		uhm_resolver_add_A (resolver, "www.googleapis.com", ip_address);
	}
}

static void
async_ready_callback (GObject      *object,
		      GAsyncResult *result,
		      gpointer      user_data)
{
	AsyncClosure *closure = user_data;

	closure->data = gdata_service_query_single_entry_finish (GDATA_SERVICE (object), result, &closure->error);
	g_main_loop_quit (closure->main_loop);
}

/* Topic */
static GDataFreebaseTopicResult *
freebase_topic (GDataFreebaseTopicQuery *query, const gchar *trace)
{
	GDataFreebaseTopicResult *result;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, trace);
	result = gdata_freebase_service_get_topic (GDATA_FREEBASE_SERVICE (service), query, NULL, &error);
	g_assert_no_error (error);
	g_assert (result != NULL);
	uhm_server_end_trace (mock_server);

	return result;
}

static GDataFreebaseTopicResult *
freebase_topic_async (GDataFreebaseTopicQuery *query, const gchar *trace)
{
	AsyncClosure closure = { 0 };

	gdata_test_mock_server_start_trace (mock_server, trace);
	closure.main_loop = g_main_loop_new (NULL, TRUE);

	gdata_freebase_service_get_topic_async (GDATA_FREEBASE_SERVICE (service), query, NULL,
						async_ready_callback, &closure);

	g_main_loop_run (closure.main_loop);
	g_assert_no_error (closure.error);
	g_assert (closure.data != NULL);

	g_main_loop_unref (closure.main_loop);
	uhm_server_end_trace (mock_server);

	return closure.data;
}

static GDataFreebaseTopicObject *
create_topic_reply_object (gboolean async)
{
	GDataFreebaseTopicQuery *query; /* owned */
	GDataFreebaseTopicResult *result; /* owned */
	GDataFreebaseTopicObject *object;

	query = gdata_freebase_topic_query_new ("/en/prado_museum");
	gdata_freebase_topic_query_set_language (query, "en");

	if (async) {
		result = freebase_topic_async (query, "topic");
	} else {
		result = freebase_topic (query, "topic");
	}

	object = gdata_freebase_topic_result_dup_object (result);
	g_assert (object != NULL);

	g_object_unref (result);
	g_object_unref (query);

	return object;
}

static void
test_freebase_topic_query_sync (void)
{
	GDataFreebaseTopicObject *object;

	object = create_topic_reply_object (FALSE);
	g_assert_cmpstr (gdata_freebase_topic_object_get_id (object), ==, "/m/01hlq3");
	gdata_freebase_topic_object_unref (object);
}

static void
test_freebase_topic_query_async (void)
{
	GDataFreebaseTopicObject *object;

	object = create_topic_reply_object (TRUE);
	g_assert_cmpstr (gdata_freebase_topic_object_get_id (object), ==, "/m/01hlq3");
	gdata_freebase_topic_object_unref (object);
}

static void
test_freebase_topic_reply_simple (void)
{
	GDataFreebaseTopicObject *object = NULL; /* owned */
	GDataFreebaseTopicValue *value;

	object = create_topic_reply_object (FALSE);

	value = gdata_freebase_topic_object_get_property_value (object, "/book/author/openlibrary_id", 0);
	g_assert (value != NULL);
	g_assert (gdata_freebase_topic_value_get_value_type (value) == G_TYPE_STRING);
	g_assert_cmpstr (gdata_freebase_topic_value_get_string (value), ==, "OL2349017A");

	gdata_freebase_topic_object_unref (object);
}

static void
test_freebase_topic_reply_simple_nested (void)
{
	GDataFreebaseTopicObject *object = NULL; /* owned */
	const GDataFreebaseTopicObject *child;
	GDataFreebaseTopicValue *value;

	object = create_topic_reply_object (FALSE);

	value = gdata_freebase_topic_object_get_property_value (object, "/location/location/geolocation", 0);
	g_assert (value != NULL);
	g_assert (gdata_freebase_topic_value_get_value_type (value) == GDATA_TYPE_FREEBASE_TOPIC_OBJECT);
	child = gdata_freebase_topic_value_get_object (value);
	g_assert (child != NULL);

	value = gdata_freebase_topic_object_get_property_value (child, "/location/geocode/latitude", 0);
	g_assert (value != NULL);
	g_assert (gdata_freebase_topic_value_get_value_type (value) == G_TYPE_DOUBLE);
	g_assert_cmpfloat (gdata_freebase_topic_value_get_double (value), ==, 40.413889);

	gdata_freebase_topic_object_unref (object);
}

static void
test_freebase_topic_reply_object (void)
{
	GDataFreebaseTopicObject *object = NULL; /* owned */
	const GDataFreebaseTopicObject *child;
	GDataFreebaseTopicValue *value;

	object = create_topic_reply_object (FALSE);

	value = gdata_freebase_topic_object_get_property_value (object, "/architecture/building/building_function", 0);
	g_assert (value != NULL);
	g_assert_cmpstr (gdata_freebase_topic_value_get_text (value), ==, "Museum");
	g_assert (gdata_freebase_topic_value_get_value_type (value) == GDATA_TYPE_FREEBASE_TOPIC_OBJECT);
	child = gdata_freebase_topic_value_get_object (value);
	g_assert (child != NULL);
	g_assert_cmpstr (gdata_freebase_topic_object_get_id (child), ==, "/m/09cmq");

	gdata_freebase_topic_object_unref (object);
}

static void
test_freebase_topic_reply_object_nested (void)
{
	GDataFreebaseTopicObject *object = NULL; /* owned */
	const GDataFreebaseTopicObject *child;
	GDataFreebaseTopicValue *value;

	object = create_topic_reply_object (FALSE);

	value = gdata_freebase_topic_object_get_property_value (object, "/architecture/museum/address", 0);
	g_assert (value != NULL);
	g_assert (gdata_freebase_topic_value_get_value_type (value) == GDATA_TYPE_FREEBASE_TOPIC_OBJECT);
	child = gdata_freebase_topic_value_get_object (value);
	g_assert (child != NULL);

	value = gdata_freebase_topic_object_get_property_value (child, "/location/mailing_address/citytown", 0);
	g_assert_cmpstr (gdata_freebase_topic_value_get_text (value), ==, "Madrid");
	g_assert (gdata_freebase_topic_value_get_value_type (value) == GDATA_TYPE_FREEBASE_TOPIC_OBJECT);
	child = gdata_freebase_topic_value_get_object (value);
	g_assert (child != NULL);
	g_assert_cmpstr (gdata_freebase_topic_object_get_id (child), ==, "/m/056_y");

	gdata_freebase_topic_object_unref (object);
}

static void
test_freebase_topic_reply_arrays (void)
{
	GDataFreebaseTopicObject *object = NULL; /* owned */
	const GDataFreebaseTopicObject *child;
	GDataFreebaseTopicValue *value;

	object = create_topic_reply_object (FALSE);

	g_assert_cmpint (gdata_freebase_topic_object_get_property_count (object, "/visual_art/art_owner/artworks_owned"), ==, 10);
	g_assert_cmpint (gdata_freebase_topic_object_get_property_hits (object, "/visual_art/art_owner/artworks_owned"), ==, 75);

	/* Not a fetched item, we expect this to be NULL */
	value = gdata_freebase_topic_object_get_property_value (object, "/visual_art/art_owner/artworks_owned", 40);
	g_assert (value == NULL);

	/* Get a fetched item, check contents */
	value = gdata_freebase_topic_object_get_property_value (object, "/visual_art/art_owner/artworks_owned", 2);
	g_assert (value != NULL);
	g_assert (gdata_freebase_topic_value_get_value_type (value) == GDATA_TYPE_FREEBASE_TOPIC_OBJECT);

	child = gdata_freebase_topic_value_get_object (value);
	g_assert (child != NULL);
	value = gdata_freebase_topic_object_get_property_value (child, "/visual_art/artwork_owner_relationship/artwork", 0);
	g_assert (value != NULL);
	g_assert_cmpstr (gdata_freebase_topic_value_get_text (value), ==, "Las Meninas");
	g_assert (gdata_freebase_topic_value_get_value_type (value) == GDATA_TYPE_FREEBASE_TOPIC_OBJECT);

	child = gdata_freebase_topic_value_get_object (value);
	g_assert (child != NULL);
	g_assert_cmpstr (gdata_freebase_topic_object_get_id (child), ==, "/m/01gd_c");

	gdata_freebase_topic_object_unref (object);
}

int
main (int argc, char *argv[])
{
	GFile *trace_directory;
	gint retval;

	gdata_test_init (argc, argv);

	mock_server = gdata_test_get_mock_server ();
	g_signal_connect (G_OBJECT (mock_server), "notify::resolver",
	                  (GCallback) mock_server_notify_resolver_cb, NULL);
	trace_directory = g_file_new_for_path (TEST_FILE_DIR "traces/freebase");
	uhm_server_set_trace_directory (mock_server, trace_directory);
	g_object_unref (trace_directory);

	service = GDATA_SERVICE (gdata_freebase_service_new (NULL, NULL));

	/* Topic */
	g_test_add_func ("/freebase/topic/query/sync",
			 test_freebase_topic_query_sync);
	g_test_add_func ("/freebase/topic/query/async",
			 test_freebase_topic_query_async);
	g_test_add_func ("/freebase/topic/reply/simple",
	                 test_freebase_topic_reply_simple);
	g_test_add_func ("/freebase/topic/reply/simple-nested",
	                 test_freebase_topic_reply_simple_nested);
	g_test_add_func ("/freebase/topic/reply/object",
	                 test_freebase_topic_reply_object);
	g_test_add_func ("/freebase/topic/reply/object-nested",
	                 test_freebase_topic_reply_object_nested);
	g_test_add_func ("/freebase/topic/reply/arrays",
	                 test_freebase_topic_reply_arrays);

	retval = g_test_run ();

	g_clear_object (&service);
	g_clear_object (&mock_server);

	return retval;
}
