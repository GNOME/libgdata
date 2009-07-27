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
 * SECTION:gdata-documents-text
 * @short_description: GData Documents text object
 * @stability: Unstable
 * @include: gdata/services/documents/gdata-documents-text.h
 *
 * #GDataDocumentsText is a subclass of #GDataDocumentsEntry to represent a text document from Google Documents.
 *
 * For more details of Google Documents' GData API, see the <ulink type="http://code.google.com/apis/document/docs/2.0/developers_guide_protocol.html">
 * online documentation</ulink>.
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-documents-text.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "gdata-private.h"

static void get_xml (GDataParsable *parsable, GString *xml_string);

G_DEFINE_TYPE (GDataDocumentsText, gdata_documents_text, GDATA_TYPE_DOCUMENTS_ENTRY)
#define GDATA_DOCUMENTS_TEXT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDATA_TYPE_DOCUMENTS_TEXT, GDataDocumentsTextClass))

static void
gdata_documents_text_class_init (GDataDocumentsTextClass *klass)
{
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	parsable_class->get_xml = get_xml;
}

static void
gdata_documents_text_init (GDataDocumentsText *self)
{
	/* Why am I writing it? */
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	const gchar *document_id;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_documents_text_parent_class)->get_xml (parsable, xml_string);

	document_id = gdata_documents_entry_get_document_id (GDATA_DOCUMENTS_ENTRY (parsable));
	if (document_id != NULL)
		g_string_append_printf (xml_string, "<gd:resourceId>document:%s</gd:resourceId>", document_id);
}

/**
 * gdata_documents_text_new:
 * @id: the entry's ID (not the document ID of the text document), or %NULL
 *
 * Creates a new #GDataDocumentsText with the given entry ID (#GDataEntry:id).
 *
 * Return value: a new #GDataDocumentsText, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataDocumentsText *
gdata_documents_text_new (const gchar *id)
{
	return g_object_new (GDATA_TYPE_DOCUMENTS_TEXT, "id", id, NULL);
}

/**
 * gdata_documents_text_download_document:
 * @self: a #GDataDocumentsText
 * @service: a #GDataDocumentsService
 * @content_type: return location for the document's content type, or %NULL; free with g_free()
 * @export_format: the format in which the text document should be exported
 * @destination_file: the #GFile into which the text file should be saved
 * @replace_file_if_exists: %TRUE if the file should be replaced if it already exists, %FALSE otherwise
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Downloads and returns the text document file represented by the #GDataDocumentsText. If the document doesn't exist,
 * %NULL is returned, but no error is set in @error. TODO: What?
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * If @destination_file is a directory, then the file will be downloaded in this directory with the #GDataEntry:title with 
 * the apropriate extension as name.
 *
 * If there is an error getting the document, a %GDATA_SERVICE_ERROR_WITH_QUERY error will be returned.
 *
 * Return value: the document's data, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GFile *
gdata_documents_text_download_document (GDataDocumentsText *self, GDataDocumentsService *service, gchar **content_type,
					GDataDocumentsTextFormat export_format, GFile *destination_file,
					gboolean replace_file_if_exists, GCancellable *cancellable, GError **error)
{
	const gchar *document_id;
	gchar *link_href;

	const gchar *export_formats[] = {
		"doc", /* GDATA_DOCUMENTS_TEXT_DOC */
		"html", /* GDATA_DOCUMENTS_TEXT_HTML */
		"odt", /* GDATA_DOCUMENTS_TEXT_ODT */
		"pdf", /* GDATA_DOCUMENTS_TEXT_PDF */
		"png", /* GDATA_DOCUMENTS_TEXT_PNG */
		"rtf", /* GDATA_DOCUMENTS_TEXT_RTF */
		"txt", /* GDATA_DOCUMENTS_TEXT_TXT */
		"zip" /* GDATA_DOCUMENTS_TEXT_ZIP */
	};

	/* TODO: async version */
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_TEXT (self), NULL);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_SERVICE (service), NULL);
	g_return_val_if_fail (export_format >= 0 && export_format < G_N_ELEMENTS (export_formats), NULL);
	g_return_val_if_fail (G_IS_FILE (destination_file), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);

	document_id = gdata_documents_entry_get_document_id (GDATA_DOCUMENTS_ENTRY (self));
	g_assert (document_id != NULL);

	link_href = g_strdup_printf ("http://docs.google.com/feeds/download/presentations/Export?exportFormat=%s&docID=%s",
				     export_formats[export_format], document_id);

	/* Download the file */
	destination_file = _gdata_documents_entry_download_document (GDATA_DOCUMENTS_ENTRY (self), GDATA_SERVICE (service),
								     content_type, link_href, destination_file, export_formats[export_format], replace_file_if_exists,
								     cancellable, error);
	g_free (link_href);

	return destination_file;
}
