/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Thibault Saunier 2009 <saunierthibault@gmail.com>
 * Copyright (C) Philip Withnall 2010 <philip@tecnocode.co.uk>
 * Copyright (C) Red Hat, Inc. 2016
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
 * @stability: Stable
 * @include: gdata/services/documents/gdata-documents-spreadsheet.h
 *
 * #GDataDocumentsSpreadsheet is a subclass of #GDataDocumentsDocument to represent a spreadsheet from Google Documents.
 *
 * For more details of Google Drive's GData API, see the
 * <ulink type="http" url="https://developers.google.com/drive/v2/web/about-sdk">online documentation</ulink>.
 *
 * <example>
 * 	<title>Downloading a Specific Sheet of a Spreadsheet</title>
 * 	<programlisting>
 *	GDataDocumentsService *service;
 *	GDataDocumentsSpreadsheet *spreadsheet;
 *	GFile *destination_file;
 *	guint gid;
 *	gchar *download_uri;
 *	GDataDownloadStream *download_stream;
 *	GFileOutputStream *output_stream;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service and retrieve the spreadsheet and sheet index (GID) to download and the file to save the download in *<!-- -->/
 *	service = create_youtube_service ();
 *	spreadsheet = get_document_to_download (service);
 *	destination_file = query_user_for_destination_file (spreadsheet);
 *	gid = query_user_for_gid (spreadsheet);
 *
 *	/<!-- -->* Create the download stream *<!-- -->/
 *	download_uri = gdata_documents_spreadsheet_get_download_uri (spreadsheet, GDATA_DOCUMENTS_SPREADSHEET_CSV, gid);
 *	download_stream = GDATA_DOWNLOAD_STREAM (gdata_download_stream_new (service, gdata_documents_service_get_spreadsheet_authorization_domain (),
 *	                                                                    download_uri, NULL));
 *	g_free (download_uri);
 *
 *	g_object_unref (spreadsheet);
 *	g_object_unref (service);
 *
 *	/<!-- -->* Create the file output stream *<!-- -->/
 *	output_stream = g_file_replace (destination_file, NULL, FALSE, G_FILE_CREATE_REPLACE_DESTINATION, NULL, &error);
 *
 *	g_object_unref (destination_file);
 *
 *	if (error != NULL) {
 *		g_error ("Error creating destination file: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (download_stream);
 *		return;
 *	}
 *
 *	/<!-- -->* Download the document. This should almost always be done asynchronously. *<!-- -->/
 *	g_output_stream_splice (G_OUTPUT_STREAM (output_stream), G_INPUT_STREAM (download_stream),
 *	                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &error);
 *
 *	g_object_unref (output_stream);
 *	g_object_unref (download_stream);
 *
 *	if (error != NULL) {
 *		g_error ("Error downloading spreadsheet: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 * 	</programlisting>
 * </example>
 *
 * Since: 0.4.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-documents-spreadsheet.h"
#include "gdata-documents-utils.h"
#include "gdata-parser.h"
#include "gdata-private.h"

static void gdata_documents_spreadsheet_constructed (GObject *object);

G_DEFINE_TYPE (GDataDocumentsSpreadsheet, gdata_documents_spreadsheet, GDATA_TYPE_DOCUMENTS_DOCUMENT)

static void
gdata_documents_spreadsheet_class_init (GDataDocumentsSpreadsheetClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	gobject_class->constructed = gdata_documents_spreadsheet_constructed;
	entry_class->kind_term = "http://schemas.google.com/docs/2007#spreadsheet";
}

static void
gdata_documents_spreadsheet_init (GDataDocumentsSpreadsheet *self)
{
	/* Why am I writing it? */
}

static void
gdata_documents_spreadsheet_constructed (GObject *object)
{
	G_OBJECT_CLASS (gdata_documents_spreadsheet_parent_class)->constructed (object);

	if (!_gdata_parsable_is_constructed_from_xml (GDATA_PARSABLE (object)))
		gdata_documents_utils_add_content_type (GDATA_DOCUMENTS_ENTRY (object), "application/vnd.google-apps.spreadsheet");
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
 */
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
 */
gchar *
gdata_documents_spreadsheet_get_download_uri (GDataDocumentsSpreadsheet *self, const gchar *export_format, gint gid)
{
	const gchar *resource_id, *document_id;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SPREADSHEET (self), NULL);
	g_return_val_if_fail (export_format != NULL && *export_format != '\0', NULL);
	g_return_val_if_fail (gid >= -1, NULL);

	/* Extract the document ID from the resource ID. */
	resource_id = gdata_documents_entry_get_resource_id (GDATA_DOCUMENTS_ENTRY (self));
	g_assert (resource_id != NULL);

	document_id = g_utf8_strchr (resource_id, -1, ':');
	g_assert (document_id != NULL);
	document_id++; /* skip over the colon */

	if (gid != -1) {
		return _gdata_service_build_uri ("http://spreadsheets.google.com/feeds/download/spreadsheets/Export?key=%s&exportFormat=%s&gid=%d",
		                                 document_id, export_format, gid);
	} else {
		return _gdata_service_build_uri ("http://spreadsheets.google.com/feeds/download/spreadsheets/Export?key=%s&exportFormat=%s",
		                                 document_id, export_format);
	}
}
