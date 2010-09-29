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
 * #GDataDocumentsSpreadsheet is a subclass of #GDataDocumentsEntry to represent a spreadsheet from Google Documents.
 *
 * For more details of Google Documents' GData API, see the <ulink type="http://code.google.com/apis/document/docs/2.0/developers_guide_protocol.html">
 * online documentation</ulink>.
 *
 * Since: 0.4.0
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-documents-spreadsheet.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "gdata-private.h"

static void get_xml (GDataParsable *parsable, GString *xml_string);

static const struct { const gchar *extension; const gchar *fmcmd; } export_formats[] = {
	{ "xls", "4" }, /* GDATA_DOCUMENTS_SPREADSHEET_XLS */
	{ "csv", "5" }, /* GDATA_DOCUMENTS_SPREADSHEET_CSV */
	{ "pdf", "12" }, /* GDATA_DOCUMENTS_SPREADSHEET_PDF */
	{ "ods", "13" }, /* GDATA_DOCUMENTS_SPREADSHEET_ODS */
	{ "tsv", "23" }, /* GDATA_DOCUMENTS_SPREADSHEET_TSV */
	{ "html", "102" } /* GDATA_DOCUMENTS_SPREADSHEET_HTML */
};

G_DEFINE_TYPE (GDataDocumentsSpreadsheet, gdata_documents_spreadsheet, GDATA_TYPE_DOCUMENTS_ENTRY)
#define GDATA_DOCUMENTS_SPREADSHEET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDATA_TYPE_DOCUMENTS_SPREADSHEET, GDataDocumentsSpreadsheetPrivate))

static void
gdata_documents_spreadsheet_class_init (GDataDocumentsSpreadsheetClass *klass)
{
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	parsable_class->get_xml = get_xml;
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
 * Return value: a new #GDataDocumentsSpreadsheet, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataDocumentsSpreadsheet *
gdata_documents_spreadsheet_new (const gchar *id)
{
	return g_object_new (GDATA_TYPE_DOCUMENTS_SPREADSHEET, "id", id, NULL);
}

/**
 * gdata_documents_spreadsheet_download_document:
 * @self: a #GDataDocumentsSpreadsheet
 * @service: a #GDataDocumentsService
 * @content_type: (out callee-allocates) (transfer full) (allow-none): return location for the document's content type, or %NULL; free with g_free()
 * @export_format: the format in which the spreadsheet should be exported
 * @gid: the <code class="literal">0</code>-based sheet ID to download, or <code class="literal">-1</code>
 * @destination_file: the #GFile into which the spreadsheet file should be saved
 * @replace_file_if_exists: %TRUE if the file should be replaced if it already exists, %FALSE otherwise
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Downloads and returns the spreadsheet file represented by the #GDataDocumentsSpreadsheet. If the document doesn't exist,
 * %NULL is returned, but no error is set in @error. TODO: What?
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * When requesting a %GDATA_DOCUMENTS_SPREADSHEET_CSV or %GDATA_DOCUMENTS_SPREADSHEET_TSV file you must specify an additional
 * parameter called @gid which indicates which grid, or sheet, you wish to get (the index is <code class="literal">0</code>-based, so
 * GID <code class="literal">1</code> actually refers to the second sheet on a given spreadsheet).
 *
 * If @destination_file is a directory, then the file will be downloaded in this directory with the #GDataEntry:title with 
 * the apropriate extension as name.
 *
 * If there is an error getting the document, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned.
 *
 * Return value: (transfer full): the document's data, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GFile *
gdata_documents_spreadsheet_download_document (GDataDocumentsSpreadsheet *self, GDataDocumentsService *service, gchar **content_type,
					       GDataDocumentsSpreadsheetFormat export_format, gint gid, GFile *destination_file,
					       gboolean replace_file_if_exists, GCancellable *cancellable, GError **error)
{
	gchar *link_href;
	const gchar *extension;
	GDataService *spreadsheet_service;

	/* TODO: async version */
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SPREADSHEET (self), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (service), NULL);
	g_return_val_if_fail (export_format < G_N_ELEMENTS (export_formats), NULL);
	g_return_val_if_fail (gid >= -1, NULL);
	g_return_val_if_fail ((export_format != GDATA_DOCUMENTS_SPREADSHEET_CSV && export_format != GDATA_DOCUMENTS_SPREADSHEET_TSV) || gid != -1, NULL);
	g_return_val_if_fail (G_IS_FILE (destination_file), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);

	extension = export_formats[export_format].extension;

	/* Get the spreadsheet service */
	spreadsheet_service = _gdata_documents_service_get_spreadsheet_service (service);

	/* Download the document */
	link_href = gdata_documents_spreadsheet_get_download_uri (self, export_format, gid);
	destination_file = _gdata_documents_entry_download_document (GDATA_DOCUMENTS_ENTRY (self), spreadsheet_service, content_type,
								     link_href, destination_file, extension, replace_file_if_exists,
								     cancellable, error);
	g_free (link_href);

	return destination_file;
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
 * When requesting a %GDATA_DOCUMENTS_SPREADSHEET_CSV or %GDATA_DOCUMENTS_SPREADSHEET_TSV file you must specify an additional
 * parameter called @gid which indicates which grid, or sheet, you wish to get (the index is <code class="literal">0</code>-based, so
 * GID <code class="literal">1</code> actually refers to the second sheet on a given spreadsheet).
 *
 * Return value: the download URI; free with g_free()
 *
 * Since: 0.5.0
 **/
gchar *
gdata_documents_spreadsheet_get_download_uri (GDataDocumentsSpreadsheet *self, GDataDocumentsSpreadsheetFormat export_format, gint gid)
{
	const gchar *document_id, *fmcmd;

	g_return_val_if_fail (export_format < G_N_ELEMENTS (export_formats), NULL);
	g_return_val_if_fail (gid >= -1, NULL);
	g_return_val_if_fail ((export_format != GDATA_DOCUMENTS_SPREADSHEET_CSV && export_format != GDATA_DOCUMENTS_SPREADSHEET_TSV) || gid != -1, NULL);

	document_id = gdata_documents_entry_get_document_id (GDATA_DOCUMENTS_ENTRY (self));
	g_assert (document_id != NULL);

	fmcmd = export_formats[export_format].fmcmd;

	if (gid != -1) {
		return g_strdup_printf ("%s://spreadsheets.google.com/feeds/download/spreadsheets/Export?key=%s&fmcmd=%s&gid=%d",
		                        _gdata_service_get_scheme (), document_id, fmcmd, gid);
	} else {
		return g_strdup_printf ("%s://spreadsheets.google.com/feeds/download/spreadsheets/Export?key=%s&fmcmd=%s",
		                        _gdata_service_get_scheme (), document_id, fmcmd);
	}
}
