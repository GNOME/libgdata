/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Thibault Saunier 2009 <saunierthibault@gmail.com>
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
 *
 * Since: 0.4.0
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
#include "gdata-documents-drawing.h"
#include "gdata-documents-pdf.h"
#include "gdata-types.h"
#include "gdata-private.h"
#include "gdata-service.h"

static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);

G_DEFINE_TYPE (GDataDocumentsFeed, gdata_documents_feed, GDATA_TYPE_FEED)

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

/* NOTE: Cast from (xmlChar*) to (gchar*) (and corresponding change in memory management functions) is safe because we've changed
 * libxml's memory functions. */
static gchar *
get_kind (xmlDoc *doc, xmlNode *node)
{
	xmlNode *entry_node;

	for (entry_node = node->children; entry_node != NULL; entry_node = entry_node->next) {
		if (xmlStrcmp (entry_node->name, (xmlChar*) "category") == 0) {
			xmlChar *scheme = xmlGetProp (entry_node, (xmlChar*) "scheme");

			if (xmlStrcmp (scheme, (xmlChar*) "http://schemas.google.com/g/2005#kind") == 0) {
				xmlFree (scheme);
				return (gchar*) xmlGetProp (entry_node, (xmlChar*) "term");
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

	if (gdata_parser_is_namespace (node, "http://www.w3.org/2005/Atom") == TRUE &&
	    xmlStrcmp (node->name, (xmlChar*) "entry") == 0) {
		GDataEntry *entry = NULL;
		GType entry_type = G_TYPE_INVALID;
		gchar *kind = get_kind (doc, node);

		if (g_strcmp0 (kind, "http://schemas.google.com/docs/2007#spreadsheet") == 0) {
			entry_type = GDATA_TYPE_DOCUMENTS_SPREADSHEET;
		} else if (g_strcmp0 (kind, "http://schemas.google.com/docs/2007#document") == 0) {
			entry_type = GDATA_TYPE_DOCUMENTS_TEXT;
		} else if (g_strcmp0 (kind, "http://schemas.google.com/docs/2007#presentation") == 0) {
			entry_type = GDATA_TYPE_DOCUMENTS_PRESENTATION;
		} else if (g_strcmp0 (kind, "http://schemas.google.com/docs/2007#folder") == 0) {
			entry_type = GDATA_TYPE_DOCUMENTS_FOLDER;
		} else if (g_strcmp0 (kind, "http://schemas.google.com/docs/2007#file") == 0) {
			entry_type = GDATA_TYPE_DOCUMENTS_DOCUMENT;
		} else if (g_strcmp0 (kind, "http://schemas.google.com/docs/2007#drawing") == 0) {
			entry_type = GDATA_TYPE_DOCUMENTS_DRAWING;
		} else if (g_strcmp0 (kind, "http://schemas.google.com/docs/2007#pdf") == 0) {
			entry_type = GDATA_TYPE_DOCUMENTS_PDF;
		} else {
			g_message ("%s documents are not handled yet", kind);
			g_free (kind);
			return TRUE;
		}
		g_free (kind);

		if (g_type_is_a (entry_type, GDATA_TYPE_DOCUMENTS_ENTRY) == FALSE) {
			return FALSE;
		}

		entry = GDATA_ENTRY (_gdata_parsable_new_from_xml_node (entry_type, doc, node, NULL, error));

		/* Call the progress callback in the main thread */
		_gdata_feed_call_progress_callback (GDATA_FEED (self), user_data, entry);
		_gdata_feed_add_entry (GDATA_FEED (self), entry);

		g_object_unref (entry);

		return TRUE;
	}

	return GDATA_PARSABLE_CLASS (gdata_documents_feed_parent_class)->parse_xml (parsable, doc, node, user_data, error);
}
