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
 * <ulink type="http://code.google.com/apis/document/docs/2.0/developers_guide_protocol.html">online documentation</ulink>.
 *
 * Since: 0.7.0
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "gdata-documents-document.h"
#include "gdata-documents-entry.h"
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

/*
 * _gdata_documents_document_download_document:
 * @self: a #GDataDocumentsDocument
 * @service: an authenticated #GDataDocumentsService
 * @content_type: (out callee-allocates) (transfer full) (allow-none): return location for the document's content type, or %NULL; free with g_free()
 * @download_uri: the URI to download the document
 * @destination_file: the #GFile into which the document file should be saved
 * @file_extension: the extension with which to save the downloaded file
 * @replace_file_if_exists: %TRUE if you want to replace the file if it exists, %FALSE otherwise
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Downloads and returns the actual file which comprises the document here. If the document doesn't exist, the downloaded document will be an HTML
 * file containing the error explanation.
 * TODO: Is that still true?
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.  If the operation was
 * cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * If @replace_file_if_exists is set to %FALSE and the destination file already exists, a %G_IO_ERROR_EXISTS will be returned.
 *
 * If @service isn't authenticated, a %GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED is returned.
 *
 * If there is an error downloading the document, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned.
 *
 * If @destination_file is a directory, the file will be downloaded to this directory with the #GDataEntry:title and the appropriate extension as its
 * filename.
 *
 * Return value: a #GFile pointing to the downloaded document, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 */
GFile *
_gdata_documents_document_download_document (GDataDocumentsDocument *self, GDataService *service, gchar **content_type, const gchar *src_uri,
                                             GFile *destination_file, const gchar *file_extension, gboolean replace_file_if_exists,
                                             GCancellable *cancellable, GError **error)
{
	const gchar *document_title;
	gchar *default_filename;
	GFileOutputStream *dest_stream;
	GInputStream *src_stream;
	GFile *actual_file = NULL;
	GError *child_error = NULL;

	/* TODO: async version */
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_DOCUMENT (self), NULL);
	g_return_val_if_fail (GDATA_IS_SERVICE (service), NULL);
	g_return_val_if_fail (src_uri != NULL, NULL);
	g_return_val_if_fail (G_IS_FILE (destination_file), NULL);
	g_return_val_if_fail (file_extension != NULL, NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Ensure we're authenticated first */
	if (gdata_service_is_authenticated (GDATA_SERVICE (service)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to download documents."));
		return NULL;
	}

	/* Determine a default filename based on the document's title */
	document_title = gdata_entry_get_title (GDATA_ENTRY (self));
	default_filename = g_strdup_printf ("%s.%s", document_title, file_extension);

	dest_stream = _gdata_download_stream_find_destination (default_filename, destination_file, &actual_file, replace_file_if_exists, cancellable,
	                                                       error);
	g_free (default_filename);

	if (dest_stream == NULL)
		return NULL;

	/* Synchronously splice the data from the download stream to the file stream (network -> disk) */
	src_stream = gdata_download_stream_new (GDATA_SERVICE (service), src_uri);
	g_signal_connect (src_stream, "notify::content-type", (GCallback) notify_content_type_cb, content_type);
	g_output_stream_splice (G_OUTPUT_STREAM (dest_stream), src_stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
	                        cancellable, &child_error);
	g_object_unref (src_stream);
	g_object_unref (dest_stream);

	if (child_error != NULL) {
		g_object_unref (actual_file);
		g_propagate_error (error, child_error);
		return NULL;
	}

	return actual_file;
}
