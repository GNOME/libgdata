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

/*
 * SECTION:gdata-batch-feed
 * @short_description: GData batch feed helper object
 * @stability: Unstable
 * @include: gdata/gdata-batch-feed.h
 *
 * Helper class to parse the feed returned from a batch operation and instantiate different types of #GDataEntry according to the batch operation
 * associated with each one. It's tightly coupled with #GDataBatchOperation, and isn't exposed publicly.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/gdata/docs/batch.html">online documentation</ulink>.
 *
 * Since: 0.7.0
 */

#include <glib.h>
#include <gxml.h>

#include "gdata-batch-feed.h"
#include "gdata-private.h"
#include "gdata-batch-private.h"

static gboolean parse_xml (GDataParsable *parsable, GXmlDomDocument *doc, GXmlDomXNode *root_node, gpointer user_data, GError **error);

G_DEFINE_TYPE (GDataBatchFeed, gdata_batch_feed, GDATA_TYPE_FEED)

static void
gdata_batch_feed_class_init (GDataBatchFeedClass *klass)
{
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	parsable_class->parse_xml = parse_xml;
}

static void
gdata_batch_feed_init (GDataBatchFeed *self)
{
	/* Nothing to see here */
}

static gboolean
parse_xml (GDataParsable *parsable, GXmlDomDocument *doc, GXmlDomXNode *node, gpointer user_data, GError **error)
{
	GDataBatchOperation *operation = GDATA_BATCH_OPERATION (user_data);

	if (g_strcmp0 (gxml_dom_xnode_get_node_name (node), "entry") == 0) {
		GDataEntry *entry = NULL;
		gchar *status_response = g_strdup ("");
		gchar *status_response_new = NULL;
		gchar *status_reason = NULL;
		guint id = 0, status_code = 0;
		GXmlDomXNode *entry_node;
		BatchOperation *op;
		const gchar *entry_node_name;

		/* Parse the child nodes of the <entry> to get the batch namespace elements containing information about this operation */
		for (entry_node = gxml_dom_xnode_get_first_child (node); entry_node != NULL; entry_node = gxml_dom_xnode_get_next_sibling (entry_node)) {
			/* We have to be careful about namespaces here, and we can skip text nodes (since none of the nodes we're looking for
			 * are text nodes) */
			if (gxml_dom_xnode_get_node_type (entry_node) == GXML_DOM_NODE_TYPE_TEXT ||  // < TODO:GXML: consider using GXML_DOM_IS_TEXT, but then subclasses (CDATASection) would match? want that?
			    gdata_parser_is_namespace (entry_node, "http://schemas.google.com/gdata/batch") == FALSE)
				continue;

			entry_node_name = gxml_dom_xnode_get_node_name (entry_node);

			if (g_strcmp0 (entry_node_name, "id") == 0) {
				/* batch:id */
				gchar *id_string = gxml_dom_node_list_to_string (gxml_dom_xnode_get_child_nodes (entry_node), TRUE);
				id = strtoul ((char*) id_string, NULL, 10);
				g_free (id_string); // TODO:GXML: do we want to be freeing this?
			} else if (g_strcmp0 (entry_node_name, "status") == 0) {
				/* batch:status */
				gchar *status_code_string;
				GXmlDomXNode *child_node;

				status_code_string = gdata_parser_get_attribute (GXML_DOM_ELEMENT (entry_node), "code");
				status_code = strtoul ((char*) status_code_string, NULL, 10);
				g_free (status_code_string); // TODO:GXML: do we want to free this?

				status_reason = gdata_parser_get_attribute (GXML_DOM_ELEMENT (entry_node), "reason");

				/* Dump the content of the status node, since it's service-specific, and could be anything from plain text to XML */

				for (child_node = gxml_dom_xnode_get_first_child (entry_node); child_node != NULL; child_node = gxml_dom_xnode_get_next_sibling (child_node)) {
					status_response_new = g_strconcat (status_response, gxml_dom_xnode_to_string (child_node, 0, 0), NULL); // TODO:GXML: want to use a string building thing like gstring
					g_free (status_response);
					status_response = status_response_new;
				}
			}

			if (id != 0 && status_code != 0)
				break;
		}

		/* Check we've got all the required data */
		if (id == 0) {
			gdata_parser_error_required_element_missing ("batch:id", "entry", error);
			goto error;
		} else if (status_code == 0) {
			gdata_parser_error_required_element_missing ("batch:status", "entry", error);
			goto error;
		}

		op = _gdata_batch_operation_get_operation (operation, id);

		/* Check for errors */
		if (SOUP_STATUS_IS_SUCCESSFUL (status_code) == FALSE) {
			/* Handle the error */
			GDataService *service = gdata_batch_operation_get_service (operation);
			GDataServiceClass *klass = GDATA_SERVICE_GET_CLASS (service);
			GError *child_error = NULL;

			/* Parse the error (it's returned in a service-specific format */
			g_assert (klass->parse_error_response != NULL);
			klass->parse_error_response (service, op->type, status_code, status_reason, status_response,
			                             strlen (status_response), &child_error);

			/* Run the operation's callback. This takes ownership of @child_error. */
			_gdata_batch_operation_run_callback (operation, op, NULL, child_error);

			g_free (status_reason);
			g_free (status_response);

			/* We return TRUE because we parsed the XML successfully; despite it being an error that we parsed */
			return TRUE;
		}

		/* If there wasn't an error, parse the resulting GDataEntry and run the operation's callback */
		if (op->type == GDATA_BATCH_OPERATION_QUERY)
			entry = GDATA_ENTRY (_gdata_parsable_new_from_xml_node (op->entry_type, doc, node, NULL, error));
		else if (op->type != GDATA_BATCH_OPERATION_DELETION)
			entry = GDATA_ENTRY (_gdata_parsable_new_from_xml_node (G_OBJECT_TYPE (op->entry), doc, node, NULL, error));

		if (op->type != GDATA_BATCH_OPERATION_DELETION && entry == NULL)
			goto error;

		_gdata_batch_operation_run_callback (operation, op, entry, NULL);

		if (entry != NULL)
			g_object_unref (entry);

		g_free (status_reason);
		g_free (status_response);

		return TRUE;

error:
		g_free (status_reason);
		g_free (status_response);

		return FALSE;
	} else if (GDATA_PARSABLE_CLASS (gdata_batch_feed_parent_class)->parse_xml (parsable, doc, node, user_data, error) == FALSE) {
		/* Error! */
		return FALSE;
	}

	return TRUE;
}
