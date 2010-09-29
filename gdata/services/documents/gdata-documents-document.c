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

/**
 * SECTION:gdata-documents-document
 * @short_description: GData documents document object
 * @stability: Unstable
 * @include: gdata/services/documents/gdata-documents-document.h
 *
 * #GDataDocumentsDocument is an abstract subclass of #GDataDocumentsEntry to represent a Google Documents document. It is subclassed by
 * #GDataDocumentsPresentation, #GDataDocumentsText and #GDataDocumentsSpreadsheet, which are instantiable.
 *
 * For more details of Google Documents' GData API, see the
 * <ulink type="http" url="http://code.google.com/apis/document/docs/2.0/developers_guide_protocol.html">online documentation</ulink>.
 *
 * Since: 0.7.0
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "gdata-documents-document.h"
#include "gdata-documents-entry.h"
#include "gdata-documents-spreadsheet.h"
#include "gdata-download-stream.h"
#include "gdata-private.h"
#include "gdata-service.h"

G_DEFINE_ABSTRACT_TYPE (GDataDocumentsDocument, gdata_documents_document, GDATA_TYPE_DOCUMENTS_ENTRY)

static void
gdata_documents_document_class_init (GDataDocumentsDocumentClass *klass)
{
	/* Nothing to see here. */
}

static void
gdata_documents_document_init (GDataDocumentsDocument *self)
{
	/* Nothing to see here. */
}

static void
notify_content_type_cb (GDataDownloadStream *download_stream, GParamSpec *pspec, gchar **content_type)
{
	*content_type = g_strdup (gdata_download_stream_get_content_type (download_stream));
}

/**
 * gdata_documents_document_download:
 * @self: a #GDataDocumentsDocument
 * @service: a #GDataDocumentsService
 * @content_type: (out callee-allocates) (transfer full) (allow-none): return location for the document's content type, or %NULL; free with g_free()
 * @export_format: the format in which the document should be exported
 * @destination_file: the #GFile into which the text file should be saved
 * @replace_file_if_exists: %TRUE if the file should be replaced if it already exists, %FALSE otherwise
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Downloads and returns the document file represented by the #GDataDocumentsDocument. If the document doesn't exist, %NULL is returned, but no error
 * is set in @error.
 *
 * @export_format should be the file extension of the desired output format for the document, from the list accepted by Google Documents. For example:
 * %GDATA_DOCUMENTS_PRESENTATION_PDF, %GDATA_DOCUMENTS_SPREADSHEET_ODS or %GDATA_DOCUMENTS_TEXT_ODT.
 *
 * If @self is a #GDataDocumentsSpreadsheet, only the first grid, or sheet, in the spreadsheet will be downloaded for some export formats. To download
 * a specific a specific grid, use gdata_documents_spreadsheet_get_download_uri() with #GDataDownloadStream to download the grid manually. See the
 * <ulink type="http" url="http://code.google.com/apis/documents/docs/2.0/developers_guide_protocol.html#DownloadingSpreadsheets">GData protocol
 * specification</ulink> for more information.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread. If the operation was
 * cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * If @destination_file is a directory, then the file will be downloaded into this directory with a filename based on the #GDataEntry's title and the
 * export format. If @replace_file_if_exists is set to %FALSE and the destination file already exists, a %G_IO_ERROR_EXISTS will be returned.
 *
 * If @service isn't authenticated, a %GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED will be returned.
 *
 * If there is an error getting the document, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned.
 *
 * Return value: (transfer full): a #GFile pointing to the downloaded document, or %NULL; unref with g_object_unref()
 *
 * Since: 0.7.0
 **/
GFile *
gdata_documents_document_download (GDataDocumentsDocument *self, GDataDocumentsService *service, gchar **content_type, const gchar *export_format,
                                   GFile *destination_file, gboolean replace_file_if_exists, GCancellable *cancellable, GError **error)
{
	const gchar *document_title;
	gchar *default_filename, *download_uri;
	GDataService *_service;
	GFileOutputStream *dest_stream;
	GInputStream *src_stream;
	GFile *output_file = NULL;
	GError *child_error = NULL;

	/* TODO: async version */
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_DOCUMENT (self), NULL);
	g_return_val_if_fail (GDATA_IS_SERVICE (service), NULL);
	g_return_val_if_fail (export_format != NULL && *export_format != '\0', NULL);
	g_return_val_if_fail (G_IS_FILE (destination_file), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Horrible hack to force use of the spreadsheet service if the document we're downloading is a spreadsheet. This is necessary because it's
	 * in a different authentication domain. */
	if (GDATA_IS_DOCUMENTS_SPREADSHEET (self))
		_service = _gdata_documents_service_get_spreadsheet_service (service);
	else
		_service = GDATA_SERVICE (service);

	/* Ensure we're authenticated first */
	if (gdata_service_is_authenticated (_service) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to download documents."));
		return NULL;
	}

	/* Determine a default filename based on the document's title and export format */
	document_title = gdata_entry_get_title (GDATA_ENTRY (self));
	default_filename = g_strdup_printf ("%s.%s", document_title, export_format);

	dest_stream = _gdata_download_stream_find_destination (default_filename, destination_file, &output_file, replace_file_if_exists, cancellable,
	                                                       error);
	g_free (default_filename);

	if (dest_stream == NULL)
		return NULL;

	/* Get the download URI */
	download_uri = gdata_documents_document_get_download_uri (self, export_format);
	g_assert (download_uri != NULL);

	/* Synchronously splice the data from the download stream to the file stream (network -> disk) */
	src_stream = gdata_download_stream_new (_service, download_uri);
	g_signal_connect (src_stream, "notify::content-type", (GCallback) notify_content_type_cb, content_type);
	g_output_stream_splice (G_OUTPUT_STREAM (dest_stream), src_stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
	                        cancellable, &child_error);
	g_object_unref (src_stream);
	g_object_unref (dest_stream);
	g_free (download_uri);

	if (child_error != NULL) {
		g_object_unref (output_file);
		g_propagate_error (error, child_error);
		return NULL;
	}

	return output_file;
}

/**
 * gdata_documents_document_get_download_uri:
 * @self: a #GDataDocumentsDocument
 * @export_format: the format in which the document should be exported when downloaded
 *
 * Builds and returns the download URI for the given #GDataDocumentsDocument in the desired format. Note that directly downloading the document using
 * this URI isn't possible, as authentication is required. You should instead use gdata_download_stream_new() with the URI, and use the resulting
 * #GInputStream.
 *
 * @export_format should be the file extension of the desired output format for the document, from the list accepted by Google Documents. For example:
 * %GDATA_DOCUMENTS_PRESENTATION_PDF, %GDATA_DOCUMENTS_SPREADSHEET_ODS or %GDATA_DOCUMENTS_TEXT_ODT.
 *
 * Return value: the download URI; free with g_free()
 *
 * Since: 0.7.0
 **/
gchar *
gdata_documents_document_get_download_uri (GDataDocumentsDocument *self, const gchar *export_format)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_DOCUMENT (self), NULL);
	g_return_val_if_fail (export_format != NULL && *export_format != '\0', NULL);

	return _gdata_service_build_uri (FALSE, "%p&exportFormat=%s", gdata_entry_get_content_uri (GDATA_ENTRY (self)), export_format);
}
