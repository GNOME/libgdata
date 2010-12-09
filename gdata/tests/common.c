/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2010 <philip@tecnocode.co.uk>
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
#include <glib-object.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

/* %TRUE if there's no Internet connection, so we should only run local tests */
static gboolean no_internet = FALSE;

void
gdata_test_init (int argc, char **argv)
{
	gint i;

	g_type_init ();
	g_thread_init (NULL);

	/* Parse the --no-internet option */
	for (i = 1; i < argc; i++) {
		if (strcmp ("--no-internet", argv[i]) == 0 || strcmp ("-n", argv[i]) == 0) {
			no_internet = TRUE;
			argv[i] = (char*) "";
		} else if (strcmp ("-?", argv[i]) == 0 || strcmp ("--help", argv[i]) == 0 || strcmp ("-h" , argv[i]) == 0) {
			/* We have to override --help in order to document --no-internet */
			printf ("Usage:\n"
			          "  %s [OPTION...]\n\n"
			          "Help Options:\n"
			          "  -?, --help                     Show help options\n"
			          "Test Options:\n"
			          "  -l                             List test cases available in a test executable\n"
			          "  -seed=RANDOMSEED               Provide a random seed to reproduce test\n"
			          "                                 runs using random numbers\n"
			          "  --verbose                      Run tests verbosely\n"
			          "  -q, --quiet                    Run tests quietly\n"
			          "  -p TESTPATH                    Execute all tests matching TESTPATH\n"
			          "  -m {perf|slow|thorough|quick}  Execute tests according modes\n"
			          "  --debug-log                    Debug test logging output\n"
			          "  -n, --no-internet              Only execute tests which don't require Internet connectivity\n",
			          argv[0]);
			exit (0);
		}
	}

	g_test_init (&argc, &argv, NULL);
	g_test_bug_base ("http://bugzilla.gnome.org/show_bug.cgi?id=");

	/* Enable full debugging */
	g_setenv ("LIBGDATA_DEBUG", "3", FALSE);
}

/*
 * gdata_test_internet:
 *
 * Returns whether tests which require Internet access should be run.
 *
 * Return value: %TRUE if Internet-requiring tests should be run, %FALSE otherwise
 *
 * Since: 0.7.0
 */
gboolean
gdata_test_internet (void)
{
	return (no_internet == FALSE) ? TRUE : FALSE;
}

typedef struct {
	GDataBatchOperation *operation;
	guint op_id;
	GDataBatchOperationType operation_type;
	GDataEntry *entry;
	GDataEntry **returned_entry;
	gchar *id;
	GType entry_type;
	GError **error;
} BatchOperationData;

static void
batch_operation_data_free (BatchOperationData *data)
{
	if (data->operation != NULL)
		g_object_unref (data->operation);
	if (data->entry != NULL)
		g_object_unref (data->entry);
	g_free (data->id);

	/* We don't free data->error, as it's owned by the calling code */

	g_slice_free (BatchOperationData, data);
}

static void
test_batch_operation_query_cb (guint operation_id, GDataBatchOperationType operation_type, GDataEntry *entry, GError *error, gpointer user_data)
{
	BatchOperationData *data = user_data;

	/* Mark the callback as having been run */
	g_object_set_data (G_OBJECT (data->operation), "test::called-callbacks",
	                   GUINT_TO_POINTER (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->operation), "test::called-callbacks")) + 1));

	/* Check that the @operation_type and @operation_id matches those stored in @data */
	g_assert_cmpuint (operation_id, ==, data->op_id);
	g_assert_cmpuint (operation_type, ==, data->operation_type);

	/* If data->error is set, we're expecting the operation to fail; otherwise, we're expecting it to succeed */
	if (data->error != NULL) {
		g_assert (error != NULL);
		*(data->error) = g_error_copy (error);
		g_assert (entry == NULL);

		if (data->returned_entry != NULL)
			*(data->returned_entry) = NULL;
	} else {
		g_assert_no_error (error);
		g_assert (entry != NULL);
		g_assert (entry != data->entry); /* check that the pointers aren't the same */
		g_assert (gdata_entry_is_inserted (entry) == TRUE);

		/* Check the ID and type of the returned entry */
		/* TODO: We can't check this, because the Contacts service is stupid with IDs
		 * g_assert_cmpstr (gdata_entry_get_id (entry), ==, data->id); */
		g_assert (G_TYPE_CHECK_INSTANCE_TYPE (entry, data->entry_type));

		/* Check the entries match */
		if (data->entry != NULL) {
			g_assert_cmpstr (gdata_entry_get_title (entry), ==, gdata_entry_get_title (data->entry));
			g_assert_cmpstr (gdata_entry_get_summary (entry), ==, gdata_entry_get_summary (data->entry));
			g_assert_cmpstr (gdata_entry_get_content (entry), ==, gdata_entry_get_content (data->entry));
			g_assert_cmpstr (gdata_entry_get_content_uri (entry), ==, gdata_entry_get_content_uri (data->entry));
			g_assert_cmpstr (gdata_entry_get_rights (entry), ==, gdata_entry_get_rights (data->entry));
		}

		/* Copy the returned entry for the calling test code to prod later */
		if (data->returned_entry != NULL)
			*(data->returned_entry) = g_object_ref (entry);
	}

	/* Free the data */
	batch_operation_data_free (data);
}

guint
gdata_test_batch_operation_query (GDataBatchOperation *operation, const gchar *id, GType entry_type, GDataEntry *entry, GDataEntry **returned_entry,
                                  GError **error)
{
	guint op_id;
	BatchOperationData *data;

	data = g_slice_new (BatchOperationData);
	data->operation = g_object_ref (operation);
	data->op_id = 0;
	data->operation_type = GDATA_BATCH_OPERATION_QUERY;
	data->entry = g_object_ref (entry);
	data->returned_entry = returned_entry;
	data->id = g_strdup (id);
	data->entry_type = entry_type;
	data->error = error;

	op_id = gdata_batch_operation_add_query (operation, id, entry_type, test_batch_operation_query_cb, data);

	data->op_id = op_id;

	/* We expect a callback to be called when the operation is run */
	g_object_set_data (G_OBJECT (operation), "test::expected-callbacks",
	                   GUINT_TO_POINTER (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (operation), "test::expected-callbacks")) + 1));

	return op_id;
}

static void
test_batch_operation_insertion_update_cb (guint operation_id, GDataBatchOperationType operation_type, GDataEntry *entry, GError *error,
                                          gpointer user_data)
{
	BatchOperationData *data = user_data;

	/* Mark the callback as having been run */
	g_object_set_data (G_OBJECT (data->operation), "test::called-callbacks",
	                   GUINT_TO_POINTER (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->operation), "test::called-callbacks")) + 1));

	/* Check that the @operation_type and @operation_id matches those stored in @data */
	g_assert_cmpuint (operation_id, ==, data->op_id);
	g_assert_cmpuint (operation_type, ==, data->operation_type);

	/* If data->error is set, we're expecting the operation to fail; otherwise, we're expecting it to succeed */
	if (data->error != NULL) {
		g_assert (error != NULL);
		*(data->error) = g_error_copy (error);
		g_assert (entry == NULL);

		if (data->returned_entry != NULL)
			*(data->returned_entry) = NULL;
	} else {
		g_assert_no_error (error);
		g_assert (entry != NULL);
		g_assert (entry != data->entry); /* check that the pointers aren't the same */
		g_assert (gdata_entry_is_inserted (entry) == TRUE);

		/* Check the entries match */
		g_assert_cmpstr (gdata_entry_get_title (entry), ==, gdata_entry_get_title (data->entry));
		g_assert_cmpstr (gdata_entry_get_summary (entry), ==, gdata_entry_get_summary (data->entry));
		g_assert_cmpstr (gdata_entry_get_content (entry), ==, gdata_entry_get_content (data->entry));
		g_assert_cmpstr (gdata_entry_get_rights (entry), ==, gdata_entry_get_rights (data->entry));

		/* Only test for differences in content URI if we had one to begin with, since the inserted entry could feasibly generate and return
		 * new content. */
		if (gdata_entry_get_content_uri (data->entry) != NULL)
			g_assert_cmpstr (gdata_entry_get_content_uri (entry), ==, gdata_entry_get_content_uri (data->entry));

		/* Copy the inserted entry for the calling test code to prod later */
		if (data->returned_entry != NULL)
			*(data->returned_entry) = g_object_ref (entry);
	}

	/* Free the data */
	batch_operation_data_free (data);
}

guint
gdata_test_batch_operation_insertion (GDataBatchOperation *operation, GDataEntry *entry, GDataEntry **inserted_entry, GError **error)
{
	guint op_id;
	BatchOperationData *data;

	data = g_slice_new (BatchOperationData);
	data->operation = g_object_ref (operation);
	data->op_id = 0;
	data->operation_type = GDATA_BATCH_OPERATION_INSERTION;
	data->entry = g_object_ref (entry);
	data->returned_entry = inserted_entry;
	data->id = NULL;
	data->entry_type = G_TYPE_INVALID;
	data->error = error;

	op_id = gdata_batch_operation_add_insertion (operation, entry, test_batch_operation_insertion_update_cb, data);

	data->op_id = op_id;

	/* We expect a callback to be called when the operation is run */
	g_object_set_data (G_OBJECT (operation), "test::expected-callbacks",
	                   GUINT_TO_POINTER (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (operation), "test::expected-callbacks")) + 1));

	return op_id;
}

guint
gdata_test_batch_operation_update (GDataBatchOperation *operation, GDataEntry *entry, GDataEntry **updated_entry, GError **error)
{
	guint op_id;
	BatchOperationData *data;

	data = g_slice_new (BatchOperationData);
	data->operation = g_object_ref (operation);
	data->op_id = 0;
	data->operation_type = GDATA_BATCH_OPERATION_UPDATE;
	data->entry = g_object_ref (entry);
	data->returned_entry = updated_entry;
	data->id = NULL;
	data->entry_type = G_TYPE_INVALID;
	data->error = error;

	op_id = gdata_batch_operation_add_update (operation, entry, test_batch_operation_insertion_update_cb, data);

	data->op_id = op_id;

	/* We expect a callback to be called when the operation is run */
	g_object_set_data (G_OBJECT (operation), "test::expected-callbacks",
	                   GUINT_TO_POINTER (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (operation), "test::expected-callbacks")) + 1));

	return op_id;
}

static void
test_batch_operation_deletion_cb (guint operation_id, GDataBatchOperationType operation_type, GDataEntry *entry, GError *error, gpointer user_data)
{
	BatchOperationData *data = user_data;

	/* Mark the callback as having been run */
	g_object_set_data (G_OBJECT (data->operation), "test::called-callbacks",
	                   GUINT_TO_POINTER (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (data->operation), "test::called-callbacks")) + 1));

	/* Check that the @operation_type and @operation_id matches those stored in @data */
	g_assert_cmpuint (operation_id, ==, data->op_id);
	g_assert_cmpuint (operation_type, ==, data->operation_type);
	g_assert (entry == NULL);

	/* If data->error is set, we're expecting the operation to fail; otherwise, we're expecting it to succeed */
	if (data->error != NULL) {
		g_assert (error != NULL);
		*(data->error) = g_error_copy (error);
	} else {
		g_assert_no_error (error);
	}

	/* Free the data */
	batch_operation_data_free (data);
}

guint
gdata_test_batch_operation_deletion (GDataBatchOperation *operation, GDataEntry *entry, GError **error)
{
	guint op_id;
	BatchOperationData *data;

	data = g_slice_new (BatchOperationData);
	data->operation = g_object_ref (operation);
	data->op_id = 0;
	data->operation_type = GDATA_BATCH_OPERATION_DELETION;
	data->entry = g_object_ref (entry);
	data->returned_entry = NULL;
	data->id = NULL;
	data->entry_type = G_TYPE_INVALID;
	data->error = error;

	op_id = gdata_batch_operation_add_deletion (operation, entry, test_batch_operation_deletion_cb, data);

	data->op_id = op_id;

	/* We expect a callback to be called when the operation is run */
	g_object_set_data (G_OBJECT (operation), "test::expected-callbacks",
	                   GUINT_TO_POINTER (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (operation), "test::expected-callbacks")) + 1));

	return op_id;
}

gboolean
gdata_test_batch_operation_run (GDataBatchOperation *operation, GCancellable *cancellable, GError **error)
{
	gboolean success = gdata_batch_operation_run (operation, cancellable, error);

	/* Assert that callbacks were called exactly once for each operation in the batch operation */
	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (operation), "test::expected-callbacks")), ==,
	                  GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (operation), "test::called-callbacks")));

	return success;
}

gboolean
gdata_test_batch_operation_run_finish (GDataBatchOperation *operation, GAsyncResult *async_result, GError **error)
{
	gboolean success = gdata_batch_operation_run_finish (operation, async_result, error);

	/* Assert that callbacks were called exactly once for each operation in the batch operation */
	g_assert_cmpuint (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (operation), "test::expected-callbacks")), ==,
	                  GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (operation), "test::called-callbacks")));

	return success;
}
