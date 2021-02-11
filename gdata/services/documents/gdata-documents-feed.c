/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Thibault Saunier 2009 <saunierthibault@gmail.com>
 * Copyright (C) Philip Withnall 2010 <philip@tecnocode.co.uk>
 * Copyright (C) Red Hat, Inc. 2015
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
 * @stability: Stable
 * @include: gdata/services/documents/gdata-documents-feed.h
 *
 * #GDataDocumentsFeed is a list of entries (#GDataDocumentsEntry subclasses) returned as the result of a query to a #GDataDocumentsService,
 * or given as the input to another operation on the online service.
 *
 * Each #GDataDocumentsEntry represents a single object on the Google Documents online service, such as a text document, presentation document,
 * spreadsheet document or a folder, and the #GDataDocumentsFeed represents a collection of those objects.
 *
 * Since: 0.4.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-documents-feed.h"
#include "gdata-documents-utils.h"
#include "gdata-documents-drive.h"
#include "gdata-types.h"
#include "gdata-private.h"
#include "gdata-service.h"

static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);

G_DEFINE_TYPE (GDataDocumentsFeed, gdata_documents_feed, GDATA_TYPE_FEED)

static void
gdata_documents_feed_class_init (GDataDocumentsFeedClass *klass)
{
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	parsable_class->parse_json = parse_json;
}

static void
gdata_documents_feed_init (GDataDocumentsFeed *self)
{
	/* Why am I writing it? */
}

static void
get_kind_and_mime_type (JsonReader *reader, gchar **out_kind, gchar **out_mime_type, GError **error)
{
	GError *child_error = NULL;
	gboolean success;
	gchar *kind = NULL;
	gchar *mime_type = NULL;
	guint i, members;

	for (i = 0, members = (guint) json_reader_count_members (reader); i < members; i++) {
		json_reader_read_element (reader, i);

		if (gdata_parser_string_from_json_member (reader, "kind", P_REQUIRED | P_NON_EMPTY, &kind, &success, &child_error) == TRUE) {
			if (!success && child_error != NULL) {
				g_propagate_prefixed_error (error, child_error,
				                            /* Translators: the parameter is an error message */
				                            _("Error parsing JSON: %s"),
				                            "Failed to find ‘kind’.");
				json_reader_end_element (reader);
				goto out;
			}
		}

		if (gdata_parser_string_from_json_member (reader, "mimeType", P_DEFAULT, &mime_type, &success, &child_error) == TRUE) {
			if (!success && child_error != NULL) {
				g_propagate_prefixed_error (error, child_error,
				                            /* Translators: the parameter is an error message */
				                            _("Error parsing JSON: %s"),
				                            "Failed to find ‘mimeType’.");
				json_reader_end_element (reader);
				goto out;
			}
		}

		json_reader_end_element (reader);
	}

	if (out_kind != NULL) {
		*out_kind = kind;
		kind = NULL;
	}

	if (out_mime_type != NULL) {
		*out_mime_type = mime_type;
		mime_type = NULL;
	}

 out:
	g_free (kind);
	g_free (mime_type);
}

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	/* JSON format: https://developers.google.com/drive/v2/reference/files/list */

	if (g_strcmp0 (json_reader_get_member_name (reader), "items") == 0) {
		gboolean success = TRUE;
		guint i, elements;

		if (json_reader_is_array (reader) == FALSE) {
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
			             /* Translators: the parameter is an error message */
			             _("Error parsing JSON: %s"),
			             "JSON node ‘items’ is not an array.");
			return FALSE;
		}

		/* Loop through the elements array. */
		for (i = 0, elements = (guint) json_reader_count_elements (reader); success && i < elements; i++) {
			GDataEntry *entry = NULL;
			GError *child_error = NULL;
			GType entry_type = G_TYPE_INVALID;
			gchar *kind = NULL;
			gchar *mime_type = NULL;

			json_reader_read_element (reader, i);

			if (json_reader_is_object (reader) == FALSE) {
				g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
				             /* Translators: the parameter is an error message */
				             _("Error parsing JSON: %s"),
				             "JSON node inside ‘items’ is not an object");
				success = FALSE;
				goto continuation;
			}

			get_kind_and_mime_type (reader, &kind, &mime_type, &child_error);
			if (child_error != NULL) {
				g_propagate_error (error, child_error);
				success = FALSE;
				goto continuation;
			}

			if (g_strcmp0 (kind, "drive#file") == 0) {
				entry_type = gdata_documents_utils_get_type_from_content_type (mime_type);
			} else if (g_strcmp0 (kind, "drive#drive") == 0) {
				entry_type = GDATA_TYPE_DOCUMENTS_DRIVE;
			} else {
				g_warning ("%s files are not handled yet", kind);
			}

			if (entry_type == G_TYPE_INVALID)
				goto continuation;

			entry = GDATA_ENTRY (_gdata_parsable_new_from_json_node (entry_type, reader, NULL, error));
			/* Call the progress callback in the main thread */
			_gdata_feed_call_progress_callback (GDATA_FEED (parsable), user_data, entry);
			_gdata_feed_add_entry (GDATA_FEED (parsable), entry);

		continuation:
			g_clear_object (&entry);
			g_free (kind);
			g_free (mime_type);
			json_reader_end_element (reader);
		}

		return success;
	}

	return GDATA_PARSABLE_CLASS (gdata_documents_feed_parent_class)->parse_json (parsable, reader, user_data, error);
}
