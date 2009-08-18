/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Thibault Saunier 2009 <saunierthibault@gmail.com>
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

/**
 * SECTION:gdata-documents-feed
 * @short_description: GData documents feed object
 * @stability: Unstable
 * @include: gdata/services/documents/gdata-documents-feed.h
 *
 * #GDataDocumentsFeed is a list of entries (#GDataDocumentsEntry subclasses) returned as the result of a query to a #GDataDocumentsService,
 * or given as the input to another operation on the online service.
 *
 * Each #GDataDocumentsEntry represents a single object on the Google Documents online service, such as a text document, presentation document,
 * spreadsheet document or a folder, and the #GDataDocumentsFeed represents a collection of those objects.
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-documents-feed.h"
#include "gdata-documents-entry.h"
#include "gdata-documents-spreadsheet.h"
#include "gdata-documents-text.h"
#include "gdata-documents-presentation.h"
#include "gdata-documents-folder.h"
#include "gdata-types.h"
#include "gdata-private.h"
#include "gdata-service.h"

static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);

G_DEFINE_TYPE (GDataDocumentsFeed, gdata_documents_feed, GDATA_TYPE_FEED)
#define GDATA_DOCUMENTS_FEED_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDATA_TYPE_DOCUMENTS_FEED, GDataDocumentsFeedPrivate))

static void
gdata_documents_feed_class_init (GDataDocumentsFeedClass *klass)
{
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	parsable_class->parse_xml = parse_xml;
}

static void
gdata_documents_feed_init (GDataDocumentsFeed *self)
{
	/* Why am I writing it? */
}

/* NOTE: Should be freed with xmlFree(), not g_free() */
static xmlChar *
get_kind (xmlDoc *doc, xmlNode *node)
{
	xmlNode *entry_node;

	for (entry_node = node->children; entry_node != NULL; entry_node = entry_node->next) {
		if (xmlStrcmp (entry_node->name, (xmlChar*) "category") == 0) {
			xmlChar *scheme = xmlGetProp (entry_node, (xmlChar*) "scheme");

			if (xmlStrcmp (scheme, (xmlChar*) "http://schemas.google.com/g/2005#kind") == 0) {
				xmlFree (scheme);
				return xmlGetProp (entry_node, (xmlChar*) "term");
			}
			xmlFree (scheme);
		}
	}

	return NULL;
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	GDataDocumentsFeed *self = GDATA_DOCUMENTS_FEED (parsable);

	if (xmlStrcmp (node->name, (xmlChar*) "entry") == 0) {
		GDataEntry *entry = NULL;
		gchar *kind;

		kind = (gchar*) get_kind (doc, node);

		if (g_strcmp0 (kind, "http://schemas.google.com/docs/2007#spreadsheet") == 0)
			entry = GDATA_ENTRY (_gdata_parsable_new_from_xml_node (GDATA_TYPE_DOCUMENTS_SPREADSHEET, doc, node, NULL, error));
		else if (g_strcmp0 (kind, "http://schemas.google.com/docs/2007#document") == 0)
			entry = GDATA_ENTRY (_gdata_parsable_new_from_xml_node (GDATA_TYPE_DOCUMENTS_TEXT, doc, node, NULL, error));
		else if (g_strcmp0 (kind, "http://schemas.google.com/docs/2007#presentation") == 0)
			entry = GDATA_ENTRY (_gdata_parsable_new_from_xml_node (GDATA_TYPE_DOCUMENTS_PRESENTATION, doc, node, NULL, error));
		else if (g_strcmp0 (kind, "http://schemas.google.com/docs/2007#folder") == 0)
			entry = GDATA_ENTRY (_gdata_parsable_new_from_xml_node (GDATA_TYPE_DOCUMENTS_FOLDER, doc, node, NULL, error));
		else {
			g_message ("%s documents are not handled yet", kind);
			xmlFree (kind);
			return TRUE;
		}
		xmlFree (kind);

		if (entry == NULL)
			return FALSE;

		/* Call the progress callback in the main thread */
		_gdata_feed_call_progress_callback (GDATA_FEED (self), user_data, entry);
		_gdata_feed_add_entry (GDATA_FEED (self), entry);
	} else if (GDATA_PARSABLE_CLASS (gdata_documents_feed_parent_class)->parse_xml (parsable, doc, node, user_data, error) == FALSE) {
		/* Error! */
		return FALSE;
	}

	return TRUE;
}
