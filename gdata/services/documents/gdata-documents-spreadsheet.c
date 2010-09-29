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
 * SECTION:gdata-documents-spreadsheet
 * @short_description: GData Documents spreadsheet object
 * @stability: Unstable
 * @include: gdata/services/documents/gdata-documents-spreadsheet.h
 *
 * #GDataDocumentsSpreadsheet is a subclass of #GDataDocumentsDocument to represent a spreadsheet from Google Documents.
 *
 * For more details of Google Documents' GData API, see the
 * <ulink type="http" url="http://code.google.com/apis/document/docs/2.0/developers_guide_protocol.html">online documentation</ulink>.
 *
 * Since: 0.4.0
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-documents-spreadsheet.h"
#include "gdata-private.h"

static void get_xml (GDataParsable *parsable, GString *xml_string);

G_DEFINE_TYPE (GDataDocumentsSpreadsheet, gdata_documents_spreadsheet, GDATA_TYPE_DOCUMENTS_DOCUMENT)

static void
gdata_documents_spreadsheet_class_init (GDataDocumentsSpreadsheetClass *klass)
{
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	parsable_class->get_xml = get_xml;
	entry_class->kind_term = "http://schemas.google.com/docs/2007#spreadsheet";
}

static void
gdata_documents_spreadsheet_init (GDataDocumentsSpreadsheet *self)
{
	/* Why am I writing it? */
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	const gchar *document_id;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_documents_spreadsheet_parent_class)->get_xml (parsable, xml_string);

	document_id = gdata_documents_entry_get_document_id (GDATA_DOCUMENTS_ENTRY (parsable));
	if (document_id != NULL)
		g_string_append_printf (xml_string, "<gd:resourceId>spreadsheet:%s</gd:resourceId>", document_id);
}

/**
 * gdata_documents_spreadsheet_new:
 * @id: (allow-none): the entry's ID (not the document ID of the spreadsheet), or %NULL
 *
 * Creates a new #GDataDocumentsSpreadsheet with the given entry ID (#GDataEntry:id).
 *
 * Return value: (transfer full): a new #GDataDocumentsSpreadsheet, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataDocumentsSpreadsheet *
gdata_documents_spreadsheet_new (const gchar *id)
{
	return GDATA_DOCUMENTS_SPREADSHEET (g_object_new (GDATA_TYPE_DOCUMENTS_SPREADSHEET, "id", id, NULL));
}

/**
 * gdata_documents_spreadsheet_get_download_uri:
 * @self: a #GDataDocumentsSpreadsheet
 * @export_format: the format in which the spreadsheet should be exported when downloaded
 * @gid: the <code class="literal">0</code>-based sheet ID to download, or <code class="literal">-1</code>
 *
 * Builds and returns the download URI for the given #GDataDocumentsSpreadsheet in the desired format. Note that directly downloading
 * the document using this URI isn't possible, as authentication is required. You should instead use gdata_download_stream_new() with
 * the URI, and use the resulting #GInputStream.
 *
 * When requesting a <code class="literal">"csv"</code>, <code class="literal">"tsv"</code>, <code class="literal">"pdf"</code> or
 * <code class="literal">"html"</code> file you may specify an additional parameter called @gid which indicates which grid, or sheet, you wish to get
 * (the index is <code class="literal">0</code>-based, so GID <code class="literal">1</code> actually refers to the second sheet on a given
 * spreadsheet).
 *
 * Return value: the download URI; free with g_free()
 *
 * Since: 0.5.0
 **/
gchar *
gdata_documents_spreadsheet_get_download_uri (GDataDocumentsSpreadsheet *self, const gchar *export_format, gint gid)
{
	const gchar *document_id;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SPREADSHEET (self), NULL);
	g_return_val_if_fail (export_format != NULL && *export_format != '\0', NULL);
	g_return_val_if_fail (gid >= -1, NULL);

	document_id = gdata_documents_entry_get_document_id (GDATA_DOCUMENTS_ENTRY (self));
	g_assert (document_id != NULL);

	if (gid != -1) {
		return _gdata_service_build_uri (FALSE,
		                                 "http://spreadsheets.google.com/feeds/download/spreadsheets/Export?key=%s&exportFormat=%s&gid=%d",
		                                 document_id, export_format, gid);
	} else {
		return _gdata_service_build_uri (FALSE,
		                                 "http://spreadsheets.google.com/feeds/download/spreadsheets/Export?key=%s&exportFormat=%s",
		                                 document_id, export_format);
	}
}
