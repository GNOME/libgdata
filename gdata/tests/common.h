/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009 <philip@tecnocode.co.uk>
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
#include <gdata/gdata.h>

#include <uhttpmock/uhm.h>

#ifndef GDATA_TEST_COMMON_H
#define GDATA_TEST_COMMON_H

G_BEGIN_DECLS

#define CLIENT_ID "ytapi-GNOME-libgdata-444fubtt-0"
#define DOCUMENTS_USERNAME "libgdata.documents@gmail.com"

/* These two must match */
#define USERNAME_NO_DOMAIN "libgdata.test"
#define USERNAME USERNAME_NO_DOMAIN "@gmail.com"

/* This must not match the above two */
#define INCORRECT_USERNAME "libgdata.test.invalid@gmail.com"

/* These two must not match (obviously) */
#define PASSWORD "gdata-gdata"
#define INCORRECT_PASSWORD "bad-password"

/* The amount of fuzziness (in seconds) used in comparisons between times which should (theoretically) be equal.
 * Due to the weak consistency used in Google's servers, it's hard to guarantee that timestamps which should be equal,
 * actually are. */
#define TIME_FUZZINESS 5

void gdata_test_init (int argc, char **argv);

UhmServer *gdata_test_get_mock_server (void) G_GNUC_WARN_UNUSED_RESULT;

gboolean gdata_test_interactive (void);

guint gdata_test_batch_operation_query (GDataBatchOperation *operation, const gchar *id, GType entry_type,
                                        GDataEntry *entry, GDataEntry **returned_entry, GError **error);
guint gdata_test_batch_operation_insertion (GDataBatchOperation *operation, GDataEntry *entry, GDataEntry **inserted_entry, GError **error);
guint gdata_test_batch_operation_update (GDataBatchOperation *operation, GDataEntry *entry, GDataEntry **updated_entry, GError **error);
guint gdata_test_batch_operation_deletion (GDataBatchOperation *operation, GDataEntry *entry, GError **error);

gboolean gdata_test_batch_operation_run (GDataBatchOperation *operation, GCancellable *cancellable, GError **error);
gboolean gdata_test_batch_operation_run_finish (GDataBatchOperation *operation, GAsyncResult *async_result, GError **error);

gboolean gdata_test_compare_xml_strings (const gchar *parsable_xml, const gchar *expected_xml, gboolean print_error);
gboolean gdata_test_compare_xml (GDataParsable *parsable, const gchar *expected_xml, gboolean print_error);

gboolean gdata_test_compare_json_strings (const gchar *parsable_json, const gchar *expected_json, gboolean print_error);
gboolean gdata_test_compare_json (GDataParsable *parsable, const gchar *expected_json, gboolean print_error);

gboolean gdata_test_compare_kind (GDataEntry *entry, const gchar *expected_term, const gchar *expected_label);

/* Convenience macros. */
#define gdata_test_assert_xml(Parsable, XML) \
	G_STMT_START { \
		gboolean _test_success = gdata_test_compare_xml (GDATA_PARSABLE (Parsable), XML, TRUE); \
		g_assert (_test_success == TRUE); \
	} G_STMT_END

#define gdata_test_assert_json(Parsable, JSON) \
	G_STMT_START { \
		gboolean _test_success = gdata_test_compare_json (GDATA_PARSABLE (Parsable), JSON, TRUE); \
		g_assert (_test_success == TRUE); \
	} G_STMT_END

/* Common code for tests of async query functions that have progress callbacks */
typedef struct {
    guint progress_destroy_notify_count;
    guint async_ready_notify_count;
    GMainLoop *main_loop;
} GDataAsyncProgressClosure;
void gdata_test_async_progress_callback (GDataEntry *entry, guint entry_key, guint entry_count, GDataAsyncProgressClosure *data);
void gdata_test_async_progress_closure_free (GDataAsyncProgressClosure *data);
void gdata_test_async_progress_finish_callback (GObject *service, GAsyncResult *res, GDataAsyncProgressClosure *data);

typedef struct {
	/*< private >*/
	GMainLoop *main_loop;
	GCancellable *cancellable;
	guint cancellation_timeout; /* timeout period in ms */
	guint cancellation_timeout_id; /* ID of the callback source */
	gboolean cancellation_successful;

	gconstpointer test_data;
} GDataAsyncTestData;

/**
 * GDATA_ASYNC_CLOSURE_FUNCTIONS:
 * @CLOSURE_NAME: the name of the closure
 * @TestStructType: the type of the synchronous closure structure
 *
 * Defines set up and tear down functions for a version of @TestStructType which is wrapped by #GDataAsyncTestData (i.e. allocated and pointed to by
 * the #GDataAsyncTestData.test_data pointer). These functions will be named <function>set_up_<replaceable>CLOSURE_NAME</replaceable>_async</function>
 * and <function>tear_down_<replaceable>CLOSURE_NAME</replaceable>_async</function>.
 *
 * Since: 0.10.0
 */
#define GDATA_ASYNC_CLOSURE_FUNCTIONS(CLOSURE_NAME, TestStructType) \
static void \
set_up_##CLOSURE_NAME##_async (GDataAsyncTestData *async_data, gconstpointer service) \
{ \
	TestStructType *test_data = g_slice_new (TestStructType); \
	set_up_##CLOSURE_NAME (test_data, service); \
	gdata_set_up_async_test_data (async_data, test_data); \
} \
 \
static void \
tear_down_##CLOSURE_NAME##_async (GDataAsyncTestData *async_data, gconstpointer service) \
{ \
	tear_down_##CLOSURE_NAME ((TestStructType*) async_data->test_data, service); \
	g_slice_free (TestStructType, (TestStructType*) async_data->test_data); \
	gdata_tear_down_async_test_data (async_data, async_data->test_data); \
}

/**
 * GDATA_ASYNC_STARTING_TIMEOUT:
 *
 * The initial timeout for cancellation tests, which will be the first timeout used after testing cancelling the operation before it's started.
 * The value is in milliseconds.
 *
 * Since: 0.10.0
 */
#define GDATA_ASYNC_STARTING_TIMEOUT 20 /* ms */

/**
 * GDATA_ASYNC_TIMEOUT_MULTIPLIER:
 *
 * The factor by which the asynchronous cancellation timeout will be multiplied between iterations of the cancellation test.
 *
 * Since: 0.10.0
 */
#define GDATA_ASYNC_TIMEOUT_MULTIPLIER 3

/**
 * GDATA_ASYNC_MAXIMUM_TIMEOUT:
 *
 * The maximum timeout value for cancellation tests before they fail. i.e. If an operation takes longer than this period of time, the asynchronous
 * operation test will fail.
 * The value is in milliseconds.
 *
 * Since: 0.10.0
 */
#define GDATA_ASYNC_MAXIMUM_TIMEOUT 43740 /* ms */

/**
 * GDATA_ASYNC_TEST_FUNCTIONS:
 * @TEST_NAME: the name of the test, excluding the “test_” prefix and the “_async” suffix
 * @TestStructType: type of the closure structure to use, or <type>void</type>
 * @TEST_BEGIN_CODE: code to execute to begin the test and start the asynchronous call
 * @TEST_END_CODE: code to execute once the asynchronous call has completed, which will check the return values and any changed state
 *
 * Defines test and callback functions to test normal asynchronous operation and the cancellation behaviour of the given asynchronous function call.
 *
 * The asynchronous function call should be started in @TEST_BEGIN_CODE, using <varname>cancellable</varname> as its #GCancellable parameter,
 * <varname>async_ready_callback</varname> as its #GAsyncReadyCallback parameter and <varname>async_data</varname> as its <varname>user_data</varname>
 * parameter. There is no need for the code to create its own main loop: that's taken care of by the wrapper code.
 *
 * The code in @TEST_END_CODE will be inserted into the callback function for both the normal asynchronous test and the cancellation test, so should
 * finish the asynchronous function call, using <varname>obj</varname> as the object on which the asynchronous function call was made,
 * <varname>async_result</varname> as its #GAsyncResult parameter and <varname>error</varname> as its #GError parameter. The code should then check
 * <varname>error</code>: if it's %NULL, the code should assert success conditions; if it's non-%NULL, the code should assert failure conditions.
 * The wrapper code will ensure that the error is a %G_IO_ERROR_CANCELLED at the appropriate times.
 *
 * The following functions will be defined, and should be added to the test suite using the #GAsyncTestData closure structure:
 * <function>test_<replaceable>TEST_NAME</replaceable>_async</function> and
 * <function>test_<replaceable>TEST_NAME</replaceable>_async_cancellation</function>.
 *
 * These functions assume the existence of a <varname>mock_server</varname> variable which points to the current #UhmServer instance. They
 * will automatically use traces <varname><replaceable>TEST_NAME</replaceable>-async</varname> and
 * <varname><replaceable>TEST_NAME</replaceable>-async-cancellation</varname>.
 *
 * Since: 0.10.0
 */
#define GDATA_ASYNC_TEST_FUNCTIONS(TEST_NAME, TestStructType, TEST_BEGIN_CODE, TEST_END_CODE) \
static void \
test_##TEST_NAME##_async_cb (GObject *obj, GAsyncResult *async_result, GDataAsyncTestData *async_data) \
{ \
	TestStructType *data = (TestStructType*) async_data->test_data; \
	GError *error = NULL; \
 \
	(void) data; /* hide potential unused variable warning */ \
 \
	{ \
		TEST_END_CODE; \
 \
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED) == TRUE) { \
			g_assert (g_cancellable_is_cancelled (async_data->cancellable) == TRUE); \
			async_data->cancellation_successful = TRUE; \
		} else if (error == NULL) { \
			g_assert (g_cancellable_is_cancelled (async_data->cancellable) == FALSE || async_data->cancellation_timeout > 0); \
			async_data->cancellation_successful = FALSE; \
		} else { \
			/* Unexpected error: explode. */ \
			g_assert_no_error (error); \
			async_data->cancellation_successful = FALSE; \
		} \
	} \
 \
	g_clear_error (&error); \
 \
	g_main_loop_quit (async_data->main_loop); \
} \
\
static void \
test_##TEST_NAME##_async (GDataAsyncTestData *async_data, gconstpointer service) \
{ \
	GAsyncReadyCallback async_ready_callback = (GAsyncReadyCallback) test_##TEST_NAME##_async_cb; \
	TestStructType *data = (TestStructType*) async_data->test_data; \
	GCancellable *cancellable = NULL; /* don't expose the cancellable, so the test proceeds as normal */ \
 \
	(void) data; /* hide potential unused variable warning */ \
 \
	/* Just run the test without doing any cancellation, and assert that it succeeds. */ \
	async_data->cancellation_timeout = 0; \
 \
	g_test_message ("Running normal operation test…"); \
 \
	gdata_test_mock_server_start_trace (mock_server, G_STRINGIFY (TEST_NAME) "-async"); \
 \
	{ \
		TEST_BEGIN_CODE; \
	} \
 \
	g_main_loop_run (async_data->main_loop); \
 \
	uhm_server_end_trace (mock_server); \
} \
 \
static void \
test_##TEST_NAME##_async_cancellation (GDataAsyncTestData *async_data, gconstpointer service) \
{ \
	async_data->cancellation_timeout = 0; \
 \
	/* Starting with a short timeout, repeatedly run the async. operation, cancelling it after the timeout and increasing the timeout until
	 * the operation succeeds for the first time. We then finish the test. This guarantees that if, for example, the test creates an entry on
	 * the server, it only ever creates one; because the test only ever succeeds once. (Of course, this assumes that the server does not change
	 * state if we cancel the operation, which is a fairly optimistic assumption. Sigh.) */ \
	do { \
		GCancellable *cancellable = async_data->cancellable; \
		GAsyncReadyCallback async_ready_callback = (GAsyncReadyCallback) test_##TEST_NAME##_async_cb; \
		TestStructType *data = (TestStructType*) async_data->test_data; \
 \
		gdata_test_mock_server_start_trace (mock_server, G_STRINGIFY (TEST_NAME) "-async-cancellation"); \
 \
		(void) data; /* hide potential unused variable warning */ \
 \
		/* Ensure the timeout remains sane. */ \
		g_assert_cmpuint (async_data->cancellation_timeout, <=, GDATA_ASYNC_MAXIMUM_TIMEOUT); \
 \
		/* Schedule the cancellation after the timeout. */ \
		if (async_data->cancellation_timeout == 0) { \
			/* For the first test, cancel the cancellable before the test code is run */ \
			gdata_async_test_cancellation_cb (async_data); \
		} else { \
			async_data->cancellation_timeout_id = g_timeout_add (async_data->cancellation_timeout, \
			                                                     (GSourceFunc) gdata_async_test_cancellation_cb, async_data); \
		} \
 \
		/* Mark the cancellation as unsuccessful and hope we get proven wrong. */ \
		async_data->cancellation_successful = FALSE; \
 \
		g_test_message ("Running cancellation test with timeout of %u ms…", async_data->cancellation_timeout); \
 \
		{ \
			TEST_BEGIN_CODE; \
		} \
 \
		g_main_loop_run (async_data->main_loop); \
 \
		/* Reset the cancellable for the next iteration and increase the timeout geometrically. */ \
		g_cancellable_reset (cancellable); \
 \
		if (async_data->cancellation_timeout == 0) { \
			async_data->cancellation_timeout = GDATA_ASYNC_STARTING_TIMEOUT; /* ms */ \
		} else { \
			async_data->cancellation_timeout *= GDATA_ASYNC_TIMEOUT_MULTIPLIER; \
		} \
 \
		uhm_server_end_trace (mock_server); \
	} while (async_data->cancellation_successful == TRUE); \
 \
	/* Clean up the last timeout callback */ \
	if (async_data->cancellation_timeout_id != 0) { \
		g_source_remove (async_data->cancellation_timeout_id); \
	} \
}

gboolean gdata_async_test_cancellation_cb (GDataAsyncTestData *async_data);
void gdata_set_up_async_test_data (GDataAsyncTestData *async_data, gconstpointer test_data);
void gdata_tear_down_async_test_data (GDataAsyncTestData *async_data, gconstpointer test_data);

/**
 * GDataTestRequestErrorData:
 * @status_code: HTTP response status code
 * @reason_phrase: HTTP response status phrase
 * @message_body: HTTP response message body
 * @error_domain_func: constant function returning the #GQuark for the expected error domain
 * @error_code: expected error code
 *
 * A mapping between a HTTP response emitted by a #UhmServer and the error expected to be thrown by the HTTP client.
 * This is designed for testing error handling in the client code, typically by running a single request through an array
 * of these such mappings and testing the client code throws the correct error in each case.
 *
 * Since: 0.13.4
 */
typedef struct {
	/* HTTP response. */
	guint status_code;
	const gchar *reason_phrase;
	const gchar *message_body;
	/* Expected GData error. */
	GQuark (*error_domain_func) (void); /* typically gdata_service_error_quark */
	gint error_code;
} GDataTestRequestErrorData;

void gdata_test_set_https_port (UhmServer *server);
void gdata_test_mock_server_start_trace (UhmServer *server, const gchar *trace_filename);
gboolean gdata_test_mock_server_handle_message_error (UhmServer *server, SoupMessage *message, SoupClientContext *client, gpointer user_data);
gboolean gdata_test_mock_server_handle_message_timeout (UhmServer *server, SoupMessage *message, SoupClientContext *client, gpointer user_data);

gchar *gdata_test_query_user_for_verifier (const gchar *authentication_uri) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_TEST_COMMON_H */
