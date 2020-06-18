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
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/xmlsave.h>

#include "common.h"

/* %TRUE if interactive tests should be skipped because we're running automatically (for example) */
static gboolean no_interactive = TRUE;

/* declaration of debug handler */
static void gdata_test_debug_handler (const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data);

/* Directory to output network trace files to, if trace output is enabled. (NULL otherwise.) */
static GFile *trace_dir = NULL;

/* TRUE if tests should be run online and a trace file written for each; FALSE if tests should run offline against existing trace files. */
static gboolean write_traces = FALSE;

/* TRUE if tests should be run online and the server's responses compared to the existing trace file for each; FALSE if tests should run offline without comparison. */
static gboolean compare_traces = FALSE;

/* Global mock server instance used by all tests. */
static UhmServer *mock_server = NULL;

void
gdata_test_init (int argc, char **argv)
{
	GTlsCertificate *cert;
	GError *child_error = NULL;
	gchar *cert_path = NULL, *key_path = NULL;
	gint i;

	setlocale (LC_ALL, "");

	/* Parse the custom options */
	for (i = 1; i < argc; i++) {
		if (strcmp ("--no-interactive", argv[i]) == 0 || strcmp ("-ni", argv[i]) == 0) {
			no_interactive = TRUE;
			argv[i] = (char*) "";
		} else if (strcmp ("--interactive", argv[i]) == 0 || strcmp ("-i", argv[i]) == 0) {
			no_interactive = FALSE;
			argv[i] = (char*) "";
		} else if (strcmp ("--trace-dir", argv[i]) == 0 || strcmp ("-t", argv[i]) == 0) {
			if (i >= argc - 1) {
				fprintf (stderr, "Error: Missing directory for --trace-dir option.\n");
				exit (1);
			}

			trace_dir = g_file_new_for_path (argv[i + 1]);

			argv[i] = (char*) "";
			argv[i + 1] = (char*) "";
			i++;
		} else if (strcmp ("--write-traces", argv[i]) == 0 || strcmp ("-w", argv[i]) == 0) {
			write_traces = TRUE;
			argv[i] = (char*) "";
		} else if (strcmp ("--compare-traces", argv[i]) == 0 || strcmp ("-c", argv[i]) == 0) {
			compare_traces = TRUE;
			argv[i] = (char*) "";
		} else if (strcmp ("-?", argv[i]) == 0 || strcmp ("--help", argv[i]) == 0 || strcmp ("-h" , argv[i]) == 0) {
			/* We have to override --help in order to document --no-interactive and the trace flags. */
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
			          "  -ni, --no-interactive          Only execute tests which don't require user interaction\n"
			          "  -i, --interactive              Execute tests including those requiring user interaction\n"
			          "  -t, --trace-dir [directory]    Read/Write trace files in the specified directory\n"
			          "  -w, --write-traces             Work online and write trace files to --trace-dir\n"
			          "  -c, --compare-traces           Work online and compare with existing trace files in --trace-dir\n",
			          argv[0]);
			exit (0);
		}
	}

	/* --[write|compare]-traces are mutually exclusive. */
	if (write_traces == TRUE && compare_traces == TRUE) {
		fprintf (stderr, "Error: --write-traces and --compare-traces are mutually exclusive.\n");
		exit (1);
	}

	g_test_init (&argc, &argv, NULL);
	g_test_bug_base ("http://bugzilla.gnome.org/show_bug.cgi?id=");

	/* Set handler of debug information */
	g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, (GLogFunc) gdata_test_debug_handler, NULL);

	/* Enable full debugging. These options are seriously unsafe, but we don't care for test cases. */
	g_setenv ("LIBGDATA_DEBUG", "4" /* GDATA_LOG_FULL_UNREDACTED */, FALSE);
	g_setenv ("G_MESSAGES_DEBUG", "libgdata", FALSE);
	g_setenv ("LIBGDATA_LAX_SSL_CERTIFICATES", "1", FALSE);

	mock_server = uhm_server_new ();
	uhm_server_set_enable_logging (mock_server, write_traces);
	uhm_server_set_enable_online (mock_server, write_traces || compare_traces);

	/* Build the certificate. */
	cert_path = g_test_build_filename (G_TEST_DIST, "cert.pem", NULL);
	key_path = g_test_build_filename (G_TEST_DIST, "key.pem", NULL);

	cert = g_tls_certificate_new_from_files (cert_path, key_path, &child_error);
	g_assert_no_error (child_error);

	g_free (key_path);
	g_free (cert_path);

	/* Set it as the property. */
	uhm_server_set_tls_certificate (mock_server, cert);
	g_object_unref (cert);
}

/*
 * gdata_test_get_mock_server:
 *
 * Returns the singleton #UhmServer instance used throughout the test suite.
 *
 * Return value: (transfer none): the mock server
 *
 * Since: 0.13.4
 */
UhmServer *
gdata_test_get_mock_server (void)
{
	return mock_server;
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

static gboolean
compare_json_nodes (JsonNode *node1, JsonNode *node2)
{
	if (node1 == node2)
		return TRUE;

	if (JSON_NODE_TYPE (node1) != JSON_NODE_TYPE (node2))
		return FALSE;

	switch (JSON_NODE_TYPE (node1)) {
		case JSON_NODE_OBJECT: {
			JsonObject *object1, *object2;
			guint size1, size2;
			GList *members, *i;

			object1 = json_node_get_object (node1);
			object2 = json_node_get_object (node2);

			size1 = json_object_get_size (object1);
			size2 = json_object_get_size (object2);

			if (size1 != size2)
				return FALSE;

			/* Iterate over the first object, checking that every member is also present in the second object. */
			members = json_object_get_members (object1);

			for (i = members; i != NULL; i = i->next) {
				JsonNode *child_node1, *child_node2;

				child_node1 = json_object_get_member (object1, i->data);
				child_node2 = json_object_get_member (object2, i->data);

				g_assert (child_node1 != NULL);
				if (child_node2 == NULL) {
					g_list_free (members);
					return FALSE;
				}

				if (compare_json_nodes (child_node1, child_node2) == FALSE) {
					g_list_free (members);
					return FALSE;
				}
			}

			g_list_free (members);

			return TRUE;
		}
		case JSON_NODE_ARRAY: {
			JsonArray *array1, *array2;
			guint length1, length2, i;

			array1 = json_node_get_array (node1);
			array2 = json_node_get_array (node2);

			length1 = json_array_get_length (array1);
			length2 = json_array_get_length (array2);

			if (length1 != length2)
				return FALSE;

			/* Iterate over both arrays, checking the elements at each index are identical. */
			for (i = 0; i < length1; i++) {
				JsonNode *child_node1, *child_node2;

				child_node1 = json_array_get_element (array1, i);
				child_node2 = json_array_get_element (array2, i);

				if (compare_json_nodes (child_node1, child_node2) == FALSE)
					return FALSE;
			}

			return TRUE;
		}
		case JSON_NODE_VALUE: {
			GType type1, type2;

			type1 = json_node_get_value_type (node1);
			type2 = json_node_get_value_type (node2);

			if (type1 != type2)
				return FALSE;

			switch (type1) {
				case G_TYPE_BOOLEAN:
					return (json_node_get_boolean (node1) == json_node_get_boolean (node2)) ? TRUE : FALSE;
				case G_TYPE_DOUBLE:
					/* Note: This doesn't need an epsilon-based comparison because we only want to return
					 * true if the string representation of the two values is equal — and if it is, their
					 * parsed values should be binary identical too. */
					return (json_node_get_double (node1) == json_node_get_double (node2)) ? TRUE : FALSE;
				case G_TYPE_INT64:
					return (json_node_get_int (node1) == json_node_get_int (node2)) ? TRUE : FALSE;
				case G_TYPE_STRING:
					return (g_strcmp0 (json_node_get_string (node1), json_node_get_string (node2)) == 0) ? TRUE : FALSE;
				default:
					/* JSON doesn't support any other types. */
					g_assert_not_reached ();
			}

			return TRUE;
		}
		case JSON_NODE_NULL:
			return TRUE;
		default:
			g_assert_not_reached ();
	}
}

gboolean
gdata_test_compare_json_strings (const gchar *parsable_json, const gchar *expected_json, gboolean print_error)
{
	gboolean success;
	JsonParser *parsable_parser, *expected_parser;
	GError *child_error = NULL;

	/* Parse both strings. */
	parsable_parser = json_parser_new ();
	expected_parser = json_parser_new ();

	json_parser_load_from_data (parsable_parser, parsable_json, -1, &child_error);
	if (child_error != NULL) {
		if (print_error == TRUE) {
			g_message ("\n\nParsable: %s\n\nNot valid JSON: %s", parsable_json, child_error->message);
		}

		g_error_free (child_error);
		return FALSE;
	}

	json_parser_load_from_data (expected_parser, expected_json, -1, &child_error);
	g_assert_no_error (child_error); /* this really should never fail; or the test has encoded bad JSON */

	/* Recursively compare the two JSON nodes. */
	success = compare_json_nodes (json_parser_get_root (parsable_parser), json_parser_get_root (expected_parser));
	if (success == FALSE && print_error == TRUE) {
		/* The comparison has failed, so print out the two JSON strings for ease of debugging */
		g_message ("\n\nParsable: %s\n\nExpected: %s\n\n", parsable_json, expected_json);
	}

	g_object_unref (expected_parser);
	g_object_unref (parsable_parser);

	return success;
}

gboolean
gdata_test_compare_json (GDataParsable *parsable, const gchar *expected_json, gboolean print_error)
{
	gboolean success;
	gchar *parsable_json;

	/* Get a JSON string for the GDataParsable. */
	parsable_json = gdata_parsable_get_json (parsable);
	success = gdata_test_compare_json_strings (parsable_json, expected_json, print_error);
	g_free (parsable_json);

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

/* Output a log message. Note the output is prefixed with ‘# ’ so that it
 * doesn’t interfere with TAP output. */
static void
output_commented_lines (const gchar *message)
{
	const gchar *i, *next_newline;

	for (i = message; i != NULL && *i != '\0'; i = next_newline) {
		gchar *line;

		next_newline = strchr (i, '\n');
		if (next_newline != NULL) {
			line = g_strndup (i, next_newline - i);
			next_newline++;
		} else {
			line = g_strdup (i);
		}

		printf ("# %s\n", line);

		g_free (line);
	}
}

static void
output_log_message (const gchar *message)
{
	if (strlen (message) > 2 && message[2] == '<') {
		/* As debug string starts with direction indicator and space, t.i. "< ", */
		/* we need access string starting from third character and see if it's   */
		/* looks like xml - t.i. it starts with '<' */
		xmlChar *xml_buff;
		int buffer_size;
		xmlDocPtr xml_doc;
		/* we need to cut to the beginning of XML string */
		message = message + 2;
		/* create xml document and dump it to string buffer */
		xml_doc = xmlParseDoc ((const xmlChar*) message);
		xmlDocDumpFormatMemory (xml_doc, &xml_buff, &buffer_size, 1);
		/* print out structured xml - if it's not xml, it will get error in output */
		output_commented_lines ((gchar*) xml_buff);
		/* free xml structs */
		xmlFree (xml_buff);
		xmlFreeDoc (xml_doc);
	} else {
		output_commented_lines ((gchar*) message);
	}
}

static void
gdata_test_debug_handler (const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
	output_log_message (message);

	/* Log to the trace file. */
	if ((*message == '<' || *message == '>' || *message == ' ') && *(message + 1) == ' ') {
		uhm_server_received_message_chunk (mock_server, message, strlen (message), NULL);
	}
}

/**
 * gdata_test_set_https_port:
 * @server: a #UhmServer
 *
 * Sets the HTTPS port used for all future libgdata requests to that used by the given mock @server,
 * effectively redirecting all client requests to the mock server.
 *
 * Since: 0.13.4
 */
void
gdata_test_set_https_port (UhmServer *server)
{
	gchar *port_string = g_strdup_printf ("%u", uhm_server_get_port (server));
	g_setenv ("LIBGDATA_HTTPS_PORT", port_string, TRUE);
	g_free (port_string);
}

/**
 * gdata_test_mock_server_start_trace:
 * @server: a #UhmServer
 * @trace_filename: filename of the trace to load
 *
 * Wrapper around uhm_server_start_trace() which additionally sets the <code class="literal">LIBGDATA_HTTPS_PORT</code>
 * environment variable to redirect all libgdata requests to the mock server.
 *
 * Since: 0.13.4
 */
void
gdata_test_mock_server_start_trace (UhmServer *server, const gchar *trace_filename)
{
	GError *child_error = NULL;

	uhm_server_start_trace (server, trace_filename, &child_error);
	g_assert_no_error (child_error);
	gdata_test_set_https_port (server);
}

/**
 * gdata_test_mock_server_handle_message_error:
 * @server: a #UhmServer
 * @message: the message whose response should be filled
 * @client: the currently connected client
 * @user_data: user data provided when connecting the signal
 *
 * Handler for #UhmServer::handle-message which sets the HTTP response for @message to the HTTP error status
 * specified in a #GDataTestRequestErrorData structure passed to @user_data.
 *
 * Since: 0.13.4
 */
gboolean
gdata_test_mock_server_handle_message_error (UhmServer *server, SoupMessage *message, SoupClientContext *client, gpointer user_data)
{
	const GDataTestRequestErrorData *data = user_data;

	soup_message_set_status_full (message, data->status_code, data->reason_phrase);
	soup_message_body_append (message->response_body, SOUP_MEMORY_STATIC, data->message_body, strlen (data->message_body));

	return TRUE;
}

/**
 * gdata_test_mock_server_handle_message_timeout:
 * @server: a #UhmServer
 * @message: the message whose response should be filled
 * @client: the currently connected client
 * @user_data: user data provided when connecting the signal
 *
 * Handler for #UhmServer::handle-message which waits for 2 seconds before returning a %SOUP_STATUS_REQUEST_TIMEOUT status
 * and appropriate error message body. If used in conjunction with a 1 second timeout in the client code under test, this can
 * simulate network error conditions and timeouts, in order to test the error handling code for such conditions.
 *
 * Since: 0.13.4
 */
gboolean
gdata_test_mock_server_handle_message_timeout (UhmServer *server, SoupMessage *message, SoupClientContext *client, gpointer user_data)
{
	/* Sleep for longer than the timeout set on the client. */
	g_usleep (2 * G_USEC_PER_SEC);

	soup_message_set_status_full (message, SOUP_STATUS_REQUEST_TIMEOUT, "Request Timeout");
	soup_message_body_append (message->response_body, SOUP_MEMORY_STATIC, "Request timed out.", strlen ("Request timed out."));

	return TRUE;
}

/**
 * gdata_test_query_user_for_verifier:
 * @authentication_uri: authentication URI to present
 *
 * Given an authentication URI, prompt the user to go to that URI, grant access
 * to the test application and enter the resulting verifier. This is to be used
 * with interactive OAuth authorisation requests.
 *
 * Returns: (transfer full): verifier from the web page
 */
gchar *
gdata_test_query_user_for_verifier (const gchar *authentication_uri)
{
	char verifier[100];

	/* Wait for the user to retrieve and enter the verifier */
	g_print ("Please navigate to the following URI and grant access: %s\n", authentication_uri);
	g_print ("Enter verifier (EOF to skip test): ");
	if (scanf ("%100s", verifier) != 1) {
		/* Skip the test */
		g_test_message ("Skipping test on user request.");
		return NULL;
	}

	g_test_message ("Proceeding with user-provided verifier “%s”.", verifier);

	return g_strdup (verifier);
}
