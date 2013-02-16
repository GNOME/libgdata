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
 * #GDataDocumentsDocument is a subclass of #GDataDocumentsEntry to represent an arbitrary Google Documents document (i.e. an arbitrary file which
 * isn't a Google Documents presentation, text document, PDF, drawing or spreadsheet). It is subclassed by #GDataDocumentsPresentation, #GDataDocumentsText,
 * #GDataDocumentsPdf,  #GDataDocumentsDrawing and #GDataDocumentsSpreadsheet, which represent those specific types of Google Document, respectively.
 *
 * #GDataDocumentsDocument used to be abstract, but was made instantiable in version 0.13.0 to allow for arbitrary file uploads. This can be achieved
 * by setting #GDataDocumentsUploadQuery:convert to %FALSE when making an upload using gdata_documents_service_upload_document_resumable(). See the
 * documentation for #GDataDocumentsUploadQuery for an example.
 *
 * It should be noted that #GDataDocumentsDocument should only be used to represent arbitrary files; its subclasses should be used any time a standard
 * Google Document (spreadsheet, text document, presentation, etc.) is to be represented.
 *
 * For more details of Google Documents' GData API, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/">online documentation</ulink>.
 *
 * <example>
 * 	<title>Downloading a Document</title>
 * 	<programlisting>
 *	GDataDocumentsService *service;
 *	GDataDocumentsDocument *document;
 *	GFile *destination_file;
 *	const gchar *download_format;
 *	GDataDownloadStream *download_stream;
 *	GFileOutputStream *output_stream;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service and retrieve the document to download and the file and format to save the download in *<!-- -->/
 *	service = create_youtube_service ();
 *	document = get_document_to_download (service);
 *	destination_file = query_user_for_destination_file (document);
 *	download_format = query_user_for_download_format (document);
 *
 *	/<!-- -->* Create the download stream *<!-- -->/
 *	download_stream = gdata_documents_document_download (document, service, download_format, NULL, &error);
 *
 *	g_object_unref (document);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error creating download stream: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (destination_file);
 *		return;
 *	}
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
 *		g_error ("Error downloading document: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 * 	</programlisting>
 * </example>
 *
 * Each document accessible through the service has an access control list (ACL) which defines the level of access to the document to each user, and
 * which users the document is shared with. For more information about ACLs for documents, see the <ulink type="http"
 * url="https://developers.google.com/google-apps/documents-list/#managing_sharing_permissions_of_resources_via_access_control_lists_acls">online
 * documentation on sharing documents</ulink>.
 *
 * <example>
 * 	<title>Retrieving the Access Control List for a Document</title>
 * 	<programlisting>
 *	GDataDocumentsService *service;
 *	GDataDocumentsDocument *document;
 *	GDataFeed *acl_feed;
 *	GDataAccessRule *rule, *new_rule;
 *	GDataLink *acl_link;
 *	GList *i;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service and retrieve a document to work on *<!-- -->/
 *	service = create_documents_service ();
 *	document = get_document (service);
 *
 *	/<!-- -->* Query the service for the ACL for the given document *<!-- -->/
 *	acl_feed = gdata_access_handler_get_rules (GDATA_ACCESS_HANDLER (document), GDATA_SERVICE (service), NULL, NULL, NULL, &error);
 *
 *	if (error != NULL) {
 *		g_error ("Error getting ACL feed for document: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (document);
 *		g_object_unref (service);
 *		return;
 *	}
 *
 *	/<!-- -->* Iterate through the ACL *<!-- -->/
 *	for (i = gdata_feed_get_entries (acl_feed); i != NULL; i = i->next) {
 *		const gchar *scope_value;
 *
 *		rule = GDATA_ACCESS_RULE (i->data);
 *
 *		/<!-- -->* Do something with the access rule here. As an example, we update the rule applying to test@gmail.com and delete all
 *		 * the other rules. We then insert another rule for example@gmail.com below. *<!-- -->/
 *		gdata_access_rule_get_scope (rule, NULL, &scope_value);
 *		if (scope_value != NULL && strcmp (scope_value, "test@gmail.com") == 0) {
 *			GDataAccessRule *updated_rule;
 *
 *			/<!-- -->* Update the rule to make test@gmail.com a writer (full read/write access to the document, but they can't change
 *			 * the ACL or delete the document). *<!-- -->/
 *			gdata_access_rule_set_role (rule, GDATA_DOCUMENTS_ACCESS_ROLE_WRITER);
 *			updated_rule = GDATA_ACCESS_RULE (gdata_service_update_entry (GDATA_SERVICE (service), GDATA_ENTRY (rule), NULL, &error));
 *
 *			if (error != NULL) {
 *				g_error ("Error updating access rule for %s: %s", scope_value, error->message);
 *				g_error_free (error);
 *				g_object_unref (acl_feed);
 *				g_object_unref (document);
 *				g_object_unref (service);
 *				return;
 *			}
 *
 *			g_object_unref (updated_rule);
 *		} else {
 *			/<!-- -->* Delete any rule which doesn't apply to test@gmail.com *<!-- -->/
 *			gdata_service_delete_entry (GDATA_SERVICE (service), GDATA_ENTRY (rule), NULL, &error);
 *
 *			if (error != NULL) {
 *				g_error ("Error deleting access rule for %s: %s", scope_value, error->message);
 *				g_error_free (error);
 *				g_object_unref (acl_feed);
 *				g_object_unref (document);
 *				g_object_unref (service);
 *				return;
 *			}
 *		}
 *	}
 *
 *	g_object_unref (acl_feed);
 *
 *	/<!-- -->* Create and insert a new access rule for example@gmail.com which allows them read-only access to the document. *<!-- -->/
 *	rule = gdata_access_rule_new (NULL);
 *	gdata_access_rule_set_role (rule, GDATA_DOCUMENTS_ACCESS_ROLE_READER);
 *	gdata_access_rule_set_scope (rule, GDATA_ACCESS_SCOPE_USER, "example@gmail.com");
 *
 *	acl_link = gdata_entry_look_up_link (GDATA_ENTRY (document), GDATA_LINK_ACCESS_CONTROL_LIST);
 *	new_rule = GDATA_ACCESS_RULE (gdata_service_insert_entry (GDATA_SERVICE (service), gdata_link_get_uri (acl_link), GDATA_ENTRY (rule),
 *	                                                          NULL, &error));
 *
 *	g_object_unref (rule);
 *	g_object_unref (document);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error inserting access rule: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	g_object_unref (acl_link);
 * 	</programlisting>
 * </example>
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

G_DEFINE_TYPE (GDataDocumentsDocument, gdata_documents_document, GDATA_TYPE_DOCUMENTS_ENTRY)

static void
gdata_documents_document_class_init (GDataDocumentsDocumentClass *klass)
{
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	entry_class->kind_term = "http://schemas.google.com/docs/2007#file";
}

static void
gdata_documents_document_init (GDataDocumentsDocument *self)
{
	/* Nothing to see here. */
}

/**
 * gdata_documents_document_new:
 * @id: (allow-none): the entry's ID (not the document ID), or %NULL
 *
 * Creates a new #GDataDocumentsDocument with the given entry ID (#GDataEntry:id).
 *
 * Return value: (transfer full): a new #GDataDocumentsDocument, or %NULL; unref with g_object_unref()
 *
 * Since: 0.13.0
 */
GDataDocumentsDocument *
gdata_documents_document_new (const gchar *id)
{
	return GDATA_DOCUMENTS_DOCUMENT (g_object_new (GDATA_TYPE_DOCUMENTS_DOCUMENT, "id", id, NULL));
}

/**
 * gdata_documents_document_download:
 * @self: a #GDataDocumentsDocument
 * @service: a #GDataDocumentsService
 * @export_format: the format in which the document should be exported
 * @cancellable: (allow-none): a #GCancellable for the entire download stream, or %NULL
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
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#downloading_spreadsheets">GData protocol
 * specification</ulink> for more information.
 *
 * To get the content type of the downloaded file, gdata_download_stream_get_content_type() can be called on the returned #GDataDownloadStream.
 * Calling gdata_download_stream_get_content_length() on the stream will not return a meaningful result, however, as the stream is encoded in chunks,
 * rather than by content length.
 *
 * In order to cancel the download, a #GCancellable passed in to @cancellable must be cancelled using g_cancellable_cancel(). Cancelling the individual
 * #GInputStream operations on the #GDataDownloadStream will not cancel the entire download; merely the read or close operation in question. See the
 * #GDataDownloadStream:cancellable for more details.
 *
 * If @service isn't authenticated, a %GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED will be returned.
 *
 * If there is an error getting the document, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned.
 *
 * Return value: (transfer full): a #GDataDownloadStream to download the document with, or %NULL; unref with g_object_unref()
 *
 * Since: 0.8.0
 **/
GDataDownloadStream *
gdata_documents_document_download (GDataDocumentsDocument *self, GDataDocumentsService *service, const gchar *export_format, GCancellable *cancellable,
                                   GError **error)
{
	gchar *download_uri;
	GDataAuthorizationDomain *domain;
	GDataDownloadStream *download_stream;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_DOCUMENT (self), NULL);
	g_return_val_if_fail (GDATA_IS_SERVICE (service), NULL);
	g_return_val_if_fail (export_format != NULL && *export_format != '\0', NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* If we're downloading a spreadsheet we have to use a different authorization domain. */
	if (GDATA_IS_DOCUMENTS_SPREADSHEET (self)) {
		domain = gdata_documents_service_get_spreadsheet_authorization_domain ();
	} else {
		domain = gdata_documents_service_get_primary_authorization_domain ();
	}

	/* Ensure we're authenticated first */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (service)), domain) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to download documents."));
		return NULL;
	}

	/* Get the download URI and create a stream for it */
	download_uri = gdata_documents_document_get_download_uri (self, export_format);
	download_stream = GDATA_DOWNLOAD_STREAM (gdata_download_stream_new (GDATA_SERVICE (service), domain, download_uri, cancellable));
	g_free (download_uri);

	return download_stream;
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

	return _gdata_service_build_uri ("%p&exportFormat=%s", gdata_entry_get_content_uri (GDATA_ENTRY (self)), export_format);
}

/**
 * gdata_documents_document_get_thumbnail_uri:
 * @self: a #GDataDocumentsDocument
 *
 * Gets the URI of the thumbnail for the #GDataDocumentsDocument. If no thumbnail exists for the document, %NULL will be returned.
 *
 * The thumbnail may then be downloaded using a #GDataDownloadStream.
 *
 * <example>
 *	<title>Downloading a Document Thumbnail</title>
 *	<programlisting>
 *	GDataDocumentsService *service;
 *	const gchar *thumbnail_uri;
 *	GCancellable *cancellable;
 *	GdkPixbuf *pixbuf;
 *	GError *error = NULL;
 *
 *	service = get_my_documents_service ();
 *	thumbnail_uri = gdata_documents_document_get_thumbnail_uri (my_document);
 *	cancellable = g_cancellable_new ();
 *
 *	/<!-- -->* Prepare a download stream *<!-- -->/
 *	download_stream = GDATA_DOWNLOAD_STREAM (gdata_download_stream_new (GDATA_SERVICE (service), NULL, thumbnail_uri, cancellable));
 *
 *	/<!-- -->* Download into a new GdkPixbuf. This can be cancelled using 'cancellable'. *<!-- -->/
 *	pixbuf = gdk_pixbuf_new_from_stream (G_INPUT_STREAM (download_stream), NULL, &error);
 *
 *	if (error != NULL) {
 *		/<!-- -->* Handle the error. *<!-- -->/
 *		g_error_free (error);
 *	}
 *
 *	g_object_unref (download_stream);
 *	g_object_unref (cancellable);
 *
 *	/<!-- -->* Do something with the GdkPixbuf. *<!-- -->/
 *
 *	g_object_unref (pixbuf);
 *	</programlisting>
 * </example>
 *
 * Return value: (allow-none): the URI of the document's thumbnail, or %NULL
 *
 * Since: 0.13.1
 */
const gchar *
gdata_documents_document_get_thumbnail_uri (GDataDocumentsDocument *self)
{
	GDataLink *thumbnail_link;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_DOCUMENT (self), NULL);

	/* Get the thumbnail URI (if it exists). */
	thumbnail_link = gdata_entry_look_up_link (GDATA_ENTRY (self), "http://schemas.google.com/docs/2007/thumbnail");

	if (thumbnail_link == NULL) {
		return NULL;
	}

	return gdata_link_get_uri (thumbnail_link);
}
