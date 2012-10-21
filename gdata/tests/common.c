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
#include <libxml/parser.h>
#include <libxml/xmlsave.h>

#include "common.h"

/* %TRUE if there's no Internet connection, so we should only run local tests */
static gboolean no_internet = FALSE;

/* %TRUE if interactive tests should be skipped because we're running automatically (for example) */
static gboolean no_interactive = FALSE;

void
gdata_test_init (int argc, char **argv)
{
	gint i;

#if !GLIB_CHECK_VERSION (2, 35, 0)
	g_type_init ();
#endif

	/* Parse the --no-internet and --no-interactive options */
	for (i = 1; i < argc; i++) {
		if (strcmp ("--no-internet", argv[i]) == 0 || strcmp ("-n", argv[i]) == 0) {
			no_internet = TRUE;
			argv[i] = (char*) "";
		} else if (strcmp ("--no-interactive", argv[i]) == 0 || strcmp ("-i", argv[i]) == 0) {
			no_interactive = TRUE;
			argv[i] = (char*) "";
		} else if (strcmp ("-?", argv[i]) == 0 || strcmp ("--help", argv[i]) == 0 || strcmp ("-h" , argv[i]) == 0) {
			/* We have to override --help in order to document --no-internet and --no-interactive */
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
			          "  -n, --no-internet              Only execute tests which don't require Internet connectivity\n"
			          "  -i, --no-interactive           Only execute tests which don't require user interaction\n",
			          argv[0]);
			exit (0);
		}
	}

	g_test_init (&argc, &argv, NULL);
	g_test_bug_base ("http://bugzilla.gnome.org/show_bug.cgi?id=");

	/* Enable full debugging */
	g_setenv ("LIBGDATA_DEBUG", "3" /* GDATA_LOG_FULL */, FALSE);
	g_setenv ("G_MESSAGES_DEBUG", "libgdata", FALSE);
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

/*
 * gdata_test_interactive:
 *
 * Returns whether tests which require interactivity should be run.
 *
 * Return value: %TRUE if interactive tests should be run, %FALSE otherwise
 *
 * Since: 0.9.0
 */
gboolean
gdata_test_interactive (void)
{
	return (no_interactive == FALSE) ? TRUE : FALSE;
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

static gboolean
compare_xml_namespaces (xmlNs *ns1, xmlNs *ns2)
{
	if (ns1 == ns2)
		return TRUE;

	/* Compare various simple properties */
	if (ns1->type != ns2->type ||
	    xmlStrcmp (ns1->href, ns2->href) != 0 ||
	    xmlStrcmp (ns1->prefix, ns2->prefix) != 0 ||
	    ns1->context != ns2->context) {
		return FALSE;
	}

	return TRUE;
}

static gboolean compare_xml_nodes (xmlNode *node1, xmlNode *node2);

static gboolean
compare_xml_node_lists (xmlNode *list1, xmlNode *list2)
{
	GHashTable *table;
	xmlNode *child;

	/* Compare their child elements. We iterate through the first linked list and, for each child node, iterate through the second linked list
	 * comparing it against each node there. We keep a hashed set of nodes in the second linked list which have already been visited and compared
	 * successfully, both for speed and to guarantee that one element in the second linked list doesn't match more than one in the first linked
	 * list. We take this approach because we can't modify the second linked list in place to remove matched nodes.
	 * Finally, we iterate through the second node list and check that all its elements are in the hash table (i.e. they've all been visited
	 * exactly once).
	 * This approach is O(n^2) in the number of nodes in the linked lists, but since we should be dealing with fairly narrow XML trees this should
	 * be OK. */
	table = g_hash_table_new (g_direct_hash, g_direct_equal);

	for (child = list1; child != NULL; child = child->next) {
		xmlNode *other_child;
		gboolean matched = FALSE;

		for (other_child = list2; other_child != NULL; other_child = other_child->next) {
			if (g_hash_table_lookup (table, other_child) != NULL)
				continue;

			if (compare_xml_nodes (child, other_child) == TRUE) {
				g_hash_table_insert (table, other_child, other_child);
				matched = TRUE;
				break;
			}
		}

		if (matched == FALSE) {
			g_hash_table_destroy (table);
			return FALSE;
		}
	}

	for (child = list2; child != NULL; child = child->next) {
		if (g_hash_table_lookup (table, child) == NULL) {
			g_hash_table_destroy (table);
			return FALSE;
		}
	}

	g_hash_table_destroy (table);

	return TRUE;
}

static gboolean
compare_xml_nodes (xmlNode *node1, xmlNode *node2)
{
	GHashTable *table;
	xmlAttr *attr1, *attr2;
	xmlNs *ns;

	if (node1 == node2)
		return TRUE;

	/* Compare various simple properties */
	if (node1->type != node2->type ||
	    xmlStrcmp (node1->name, node2->name) != 0 ||
	    compare_xml_namespaces (node1->ns, node2->ns) == FALSE ||
	    xmlStrcmp (node1->content, node2->content) != 0) {
		return FALSE;
	}

	/* Compare their attributes. This is done in document order, which isn't strictly correct, since XML specifically does not apply an ordering
	 * over attributes. However, it suffices for our needs. */
	for (attr1 = node1->properties, attr2 = node2->properties; attr1 != NULL && attr2 != NULL; attr1 = attr1->next, attr2 = attr2->next) {
		/* Compare various simple properties */
		if (attr1->type != attr2->type ||
		    xmlStrcmp (attr1->name, attr2->name) != 0 ||
		    compare_xml_namespaces (attr1->ns, attr2->ns) == FALSE ||
		    attr1->atype != attr2->atype) {
			return FALSE;
		}

		/* Compare their child nodes (values represented as text and entity nodes) */
		if (compare_xml_node_lists (attr1->children, attr2->children) == FALSE)
			return FALSE;
	}

	/* Stragglers? */
	if (attr1 != NULL || attr2 != NULL)
		return FALSE;

	/* Compare their namespace definitions regardless of order. Do this by inserting all the definitions from node1 into a hash table, then running
	 * through the  definitions in node2 and ensuring they exist in the hash table, removing each one from the table as we go. Check there aren't
	 * any left in the hash table afterwards. */
	table = g_hash_table_new (g_str_hash, g_str_equal);

	for (ns = node1->nsDef; ns != NULL; ns = ns->next) {
		/* Prefixes should be unique, but I trust libxml about as far as I can throw it. */
		if (g_hash_table_lookup (table, ns->prefix ? ns->prefix : (gpointer) "") != NULL) {
			g_hash_table_destroy (table);
			return FALSE;
		}

		g_hash_table_insert (table, ns->prefix ? (gpointer) ns->prefix : (gpointer) "", ns);
	}

	for (ns = node2->nsDef; ns != NULL; ns = ns->next) {
		xmlNs *original_ns = g_hash_table_lookup (table, ns->prefix ? ns->prefix : (gpointer) "");

		if (original_ns == NULL ||
		    compare_xml_namespaces (original_ns, ns) == FALSE) {
			g_hash_table_destroy (table);
			return FALSE;
		}

		g_hash_table_remove (table, ns->prefix ? ns->prefix : (gpointer) "");
	}

	if (g_hash_table_size (table) != 0) {
		g_hash_table_destroy (table);
		return FALSE;
	}

	g_hash_table_destroy (table);

	/* Compare their child nodes */
	if (compare_xml_node_lists (node1->children, node2->children) == FALSE)
		return FALSE;

	/* Success! */
	return TRUE;
}

gboolean
gdata_test_compare_xml_strings (const gchar *parsable_xml, const gchar *expected_xml, gboolean print_error)
{
	gboolean success;
	xmlDoc *parsable_doc, *expected_doc;

	/* Parse both the XML strings */
	parsable_doc = xmlReadMemory (parsable_xml, strlen (parsable_xml), "/dev/null", NULL, 0);
	expected_doc = xmlReadMemory (expected_xml, strlen (expected_xml), "/dev/null", NULL, 0);

	g_assert (parsable_doc != NULL && expected_doc != NULL);

	/* Recursively compare the two XML trees */
	success = compare_xml_nodes (xmlDocGetRootElement (parsable_doc), xmlDocGetRootElement (expected_doc));
	if (success == FALSE && print_error == TRUE) {
		/* The comparison has failed, so print out the two XML strings for ease of debugging */
		g_message ("\n\nParsable: %s\n\nExpected: %s\n\n", parsable_xml, expected_xml);
	}

	xmlFreeDoc (expected_doc);
	xmlFreeDoc (parsable_doc);

	return success;
}

gboolean
gdata_test_compare_xml (GDataParsable *parsable, const gchar *expected_xml, gboolean print_error)
{
	gboolean success;
	gchar *parsable_xml;

	/* Get an XML string for the GDataParsable */
	parsable_xml = gdata_parsable_get_xml (parsable);
	success = gdata_test_compare_xml_strings (parsable_xml, expected_xml, print_error);
	g_free (parsable_xml);

	return success;
}

gboolean
gdata_test_compare_kind (GDataEntry *entry, const gchar *expected_term, const gchar *expected_label)
{
	GList *list;

	/* Check the entry's kind category is present and correct */
	for (list = gdata_entry_get_categories (entry); list != NULL; list = list->next) {
		GDataCategory *category = GDATA_CATEGORY (list->data);

		if (g_strcmp0 (gdata_category_get_scheme (category), "http://schemas.google.com/g/2005#kind") == 0) {
			/* Found the kind category; check its term and label. */
			return (g_strcmp0 (gdata_category_get_term (category), expected_term) == 0) &&
			       (g_strcmp0 (gdata_category_get_label (category), expected_label) == 0);
		}
	}

	/* No kind! */
	return FALSE;
}

/* Common code for tests of async query functions that have progress callbacks */

void
gdata_test_async_progress_callback (GDataEntry *entry, guint entry_key, guint entry_count, GDataAsyncProgressClosure *data)
{
    /* No-op */
}

void
gdata_test_async_progress_closure_free (GDataAsyncProgressClosure *data)
{
    /* Check that this callback is called first */
    g_assert_cmpuint (data->async_ready_notify_count, ==, 0);
    data->progress_destroy_notify_count++;
}

void
gdata_test_async_progress_finish_callback (GObject *service, GAsyncResult *res, GDataAsyncProgressClosure *data)
{
    /* Check that this callback is called second */
    g_assert_cmpuint (data->progress_destroy_notify_count, ==, 1);
    data->async_ready_notify_count++;

    g_main_loop_quit (data->main_loop);
}

gboolean
gdata_async_test_cancellation_cb (GDataAsyncTestData *async_data)
{
	g_cancellable_cancel (async_data->cancellable);
	async_data->cancellation_timeout_id = 0;
	return FALSE;
}

void
gdata_set_up_async_test_data (GDataAsyncTestData *async_data, gconstpointer test_data)
{
	async_data->main_loop = g_main_loop_new (NULL, FALSE);
	async_data->cancellable = g_cancellable_new ();
	async_data->cancellation_timeout = 0;
	async_data->cancellation_successful = FALSE;

	async_data->test_data = test_data;
}

void
gdata_tear_down_async_test_data (GDataAsyncTestData *async_data, gconstpointer test_data)
{
	g_assert (async_data->test_data == test_data); /* sanity check */

	g_object_unref (async_data->cancellable);
	g_main_loop_unref (async_data->main_loop);
}
