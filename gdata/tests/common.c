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
		    attr1->ns != attr2->ns ||
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
gdata_test_compare_xml (GDataParsable *parsable, const gchar *expected_xml, gboolean print_error)
{
	gboolean success;
	gchar *parsable_xml;
	xmlDoc *parsable_doc, *expected_doc;

	/* Get an XML string for the GDataParsable */
	parsable_xml = gdata_parsable_get_xml (parsable);

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
	g_free (parsable_xml);

	return success;
}
