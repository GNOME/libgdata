/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2012 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-documents-upload-query
 * @short_description: GData Documents upload query object
 * @stability: Stable
 * @include: gdata/services/documents/gdata-documents-upload-query.h
 *
 * #GDataDocumentsUploadQuery is a collection of parameters for document uploads to Google Documents, allowing various options to be set when uploading
 * a document for the first time. For example, the destination folder for the uploaded document may be specified; or whether to automatically convert
 * the document to a common format.
 *
 * #GDataDocumentsUploadQuery is designed as an object (rather than a fixed struct or set of function arguments) to allow for easy additions of new
 * Google Documents features in the future.
 *
 * <example>
 * 	<title>Uploading an Arbitrary File from Disk</title>
 * 	<programlisting>
 *	GDataDocumentsService *service;
 *	GDataDocumentsDocument *document, *uploaded_document;
 *	GFile *arbitrary_file;
 *	GFileInfo *file_info;
 *	const gchar *slug, *content_type;
 *	goffset file_size;
 *	GDataDocumentsUploadQuery *upload_query;
 *	GFileInputStream *file_stream;
 *	GDataUploadStream *upload_stream;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service. *<!-- -->/
 *	service = create_documents_service ();
 *
 *	/<!-- -->* Get the file to upload. *<!-- -->/
 *	arbitrary_file = g_file_new_for_path ("arbitrary-file.bin");
 *
 *	/<!-- -->* Get the file's display name, content type and size. *<!-- -->/
 *	file_info = g_file_query_info (arbitrary_file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
 *	                               G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE, NULL, &error);
 *
 *	if (error != NULL) {
 *		g_error ("Error getting arbitrary file information: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (arbitrary_file);
 *		g_object_unref (service);
 *		return;
 *	}
 *
 *	slug = g_file_info_get_display_name (file_info);
 *	content_type = g_file_info_get_content_type (file_info);
 *	file_size = g_file_info_get_size (file_info);
 *
 *	/<!-- -->* Get an input stream for the file. *<!-- -->/
 *	file_stream = g_file_read (arbitrary_file, NULL, &error);
 *
 *	g_object_unref (arbitrary_file);
 *
 *	if (error != NULL) {
 *		g_error ("Error getting arbitrary file stream: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (file_info);
 *		g_object_unref (service);
 *		return;
 *	}
 *
 *	/<!-- -->* Create the file metadata to upload. *<!-- -->/
 *	document = gdata_documents_document_new (NULL);
 *	gdata_entry_set_title (GDATA_ENTRY (document), "Title for My Arbitrary File");
 *
 *	/<!-- -->* Build the upload query and set the upload to not be converted to a standard format. *<!-- -->/
 *	upload_query = gdata_documents_upload_query_new ();
 *	gdata_documents_upload_query_set_convert (upload_query, FALSE);
 *
 *	/<!-- -->* Get an upload stream for the file. *<!-- -->/
 *	upload_stream = gdata_documents_service_upload_document_resumable (service, document, slug, content_type, file_size, upload_query, NULL, &error);
 *
 *	g_object_unref (upload_query);
 *	g_object_unref (document);
 *	g_object_unref (file_info);
 *
 *	if (error != NULL) {
 *		g_error ("Error getting upload stream: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (file_stream);
 *		g_object_unref (service);
 *		return;
 *	}
 *
 *	/<!-- -->* Upload the document. This is a blocking operation, and should normally be done asynchronously. *<!-- -->/
 *	g_output_stream_splice (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (file_stream),
 *	                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, &error);
 *
 *	g_object_unref (file_stream);
 *
 *	if (error != NULL) {
 *		g_error ("Error splicing streams: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (upload_stream);
 *		g_object_unref (service);
 *		return;
 *	}
 *
 *	/<!-- -->* Finish off the upload by parsing the returned updated document metadata entry. *<!-- -->/
 *	uploaded_document = gdata_documents_service_finish_upload (service, upload_stream, &error);
 *
 *	g_object_unref (upload_stream);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error uploading file: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	/<!-- -->* Do something with the uploaded document. *<!-- -->/
 *
 *	g_object_unref (uploaded_document);
 * 	</programlisting>
 * </example>
 *
 * Since: 0.13.0
 */

#include <glib.h>

#include "gdata-documents-upload-query.h"
#include "gdata-private.h"
#include "gdata-upload-stream.h"

static void gdata_documents_upload_query_dispose (GObject *object);
static void gdata_documents_upload_query_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_documents_upload_query_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

struct _GDataDocumentsUploadQueryPrivate {
	GDataDocumentsFolder *folder;
	gboolean convert;
};

enum {
	PROP_FOLDER = 1,
	PROP_CONVERT,
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataDocumentsUploadQuery, gdata_documents_upload_query, G_TYPE_OBJECT)

static void
gdata_documents_upload_query_class_init (GDataDocumentsUploadQueryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->get_property = gdata_documents_upload_query_get_property;
	gobject_class->set_property = gdata_documents_upload_query_set_property;
	gobject_class->dispose = gdata_documents_upload_query_dispose;

	/**
	 * GDataDocumentsUploadQuery:folder:
	 *
	 * Folder to upload the document into. If this is %NULL, the document will be uploaded into the root folder.
	 *
	 * Since: 0.13.0
	 */
	g_object_class_install_property (gobject_class, PROP_FOLDER,
	                                 g_param_spec_object ("folder",
	                                                      "Folder", "Folder to upload the document into.",
	                                                      GDATA_TYPE_DOCUMENTS_FOLDER,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsUploadQuery:convert:
	 *
	 * %TRUE to automatically convert the uploaded document into a standard format (such as a text document, spreadsheet, presentation, etc.).
	 * %FALSE to upload the document without converting it; this allows for arbitrary files to be uploaded to Google Documents.
	 *
	 * For more information, see the
	 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#creating_or_uploading_files">online documentation</ulink>.
	 *
	 * Note that uploading with this property set to %FALSE will only have an effect when using gdata_documents_service_update_document_resumable()
	 * and not gdata_documents_service_update_document(). Additionally, the #GDataDocumentsDocument passed to
	 * gdata_documents_service_update_document_resumable() must be a #GDataDocumentsDocument if this property is %FALSE, and a subclass of it
	 * otherwise.
	 *
	 * Since: 0.13.0
	 */
	g_object_class_install_property (gobject_class, PROP_CONVERT,
	                                 g_param_spec_boolean ("convert",
	                                                       "Convert?", "Whether to automatically convert uploaded documents into a standard format.",
	                                                       TRUE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_documents_upload_query_init (GDataDocumentsUploadQuery *self)
{
	self->priv = gdata_documents_upload_query_get_instance_private (self);
	self->priv->convert = TRUE;
}

static void
gdata_documents_upload_query_dispose (GObject *object)
{
	GDataDocumentsUploadQueryPrivate *priv = GDATA_DOCUMENTS_UPLOAD_QUERY (object)->priv;

	g_clear_object (&priv->folder);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_documents_upload_query_parent_class)->dispose (object);
}

static void
gdata_documents_upload_query_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataDocumentsUploadQueryPrivate *priv = GDATA_DOCUMENTS_UPLOAD_QUERY (object)->priv;

	switch (property_id) {
		case PROP_FOLDER:
			g_value_set_object (value, priv->folder);
			break;
		case PROP_CONVERT:
			g_value_set_boolean (value, priv->convert);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_documents_upload_query_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataDocumentsUploadQuery *self = GDATA_DOCUMENTS_UPLOAD_QUERY (object);

	switch (property_id) {
		case PROP_FOLDER:
			gdata_documents_upload_query_set_folder (self, g_value_get_object (value));
			break;
		case PROP_CONVERT:
			gdata_documents_upload_query_set_convert (self, g_value_get_boolean (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

/**
 * gdata_documents_upload_query_new:
 *
 * Constructs a new empty #GDataDocumentsUploadQuery.
 *
 * Return value: (transfer full): a new #GDataDocumentsUploadQuery; unref with g_object_unref()
 *
 * Since: 0.13.0
 */
GDataDocumentsUploadQuery *
gdata_documents_upload_query_new (void)
{
	return g_object_new (GDATA_TYPE_DOCUMENTS_UPLOAD_QUERY, NULL);
}

/**
 * gdata_documents_upload_query_build_uri:
 * @self: a #GDataDocumentsUploadQuery
 *
 * Builds an upload URI suitable for passing to gdata_upload_stream_new_resumable() in order to upload a document to Google Documents as described in
 * the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#uploading_a_new_document_or_file_with_both_metadata_and_content">
 * online documentation</ulink>.
 *
 * Return value: (transfer full): a complete upload URI; free with g_free()
 *
 * Since: 0.13.0
 */
gchar *
gdata_documents_upload_query_build_uri (GDataDocumentsUploadQuery *self)
{
	gchar *base_uri;
	GString *upload_uri;
	GDataDocumentsUploadQueryPrivate *priv;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_UPLOAD_QUERY (self), NULL);

	priv = self->priv;

	/* Construct the base URI. */
	if (priv->folder != NULL) {
		GDataLink *upload_link;

		/* Get the folder's upload URI. */
		upload_link = gdata_entry_look_up_link (GDATA_ENTRY (priv->folder), GDATA_LINK_RESUMABLE_CREATE_MEDIA);

		if (upload_link == NULL) {
			/* Fall back to building a URI manually. */
			base_uri = _gdata_service_build_uri ("%s://docs.google.com/feeds/upload/create-session/default/private/full/%s/contents",
			                                     _gdata_service_get_scheme (),
			                                     gdata_documents_entry_get_resource_id (GDATA_DOCUMENTS_ENTRY (priv->folder)));
		} else {
			base_uri = g_strdup (gdata_link_get_uri (upload_link));
		}
	} else {
		base_uri = g_strconcat (_gdata_service_get_scheme (), "://docs.google.com/feeds/upload/create-session/default/private/full", NULL);
	}

	/* Document format conversion. See: https://developers.google.com/google-apps/documents-list/#creating_or_uploading_files */
	upload_uri = g_string_new (base_uri);
	g_free (base_uri);

	if (priv->convert == TRUE) {
		/* Convert documents to standard formats on upload. */
		g_string_append (upload_uri, "?convert=true");
	} else {
		/* Don't convert them — this permits uploading of arbitrary files. */
		g_string_append (upload_uri, "?convert=false");
	}

	return g_string_free (upload_uri, FALSE);
}

/**
 * gdata_documents_upload_query_get_folder:
 * @self: a #GDataDocumentsUploadQuery
 *
 * Gets #GDataDocumentsUploadQuery:folder.
 *
 * Return value: (allow-none) (transfer none): the folder to upload into, or %NULL
 *
 * Since: 0.13.0
 */
GDataDocumentsFolder *
gdata_documents_upload_query_get_folder (GDataDocumentsUploadQuery *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_UPLOAD_QUERY (self), NULL);

	return self->priv->folder;
}

/**
 * gdata_documents_upload_query_set_folder:
 * @self: a #GDataDocumentsUploadQuery
 * @folder: (allow-none) (transfer none): a new folder to upload into, or %NULL
 *
 * Sets #GDataDocumentsUploadQuery:folder to @folder.
 *
 * Since: 0.13.0
 */
void
gdata_documents_upload_query_set_folder (GDataDocumentsUploadQuery *self, GDataDocumentsFolder *folder)
{
	g_return_if_fail (GDATA_IS_DOCUMENTS_UPLOAD_QUERY (self));
	g_return_if_fail (folder == NULL || GDATA_IS_DOCUMENTS_FOLDER (folder));

	if (folder == self->priv->folder) {
		return;
	}

	if (folder != NULL) {
		g_object_ref (folder);
	}

	g_clear_object (&self->priv->folder);
	self->priv->folder = folder;

	g_object_notify (G_OBJECT (self), "folder");
}

/**
 * gdata_documents_upload_query_get_convert:
 * @self: a #GDataDocumentsUploadQuery
 *
 * Gets #GDataDocumentsUploadQuery:convert.
 *
 * Return value: %TRUE to convert documents to common formats, %FALSE to upload them unmodified
 *
 * Since: 0.13.0
 */
gboolean
gdata_documents_upload_query_get_convert (GDataDocumentsUploadQuery *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_UPLOAD_QUERY (self), TRUE);

	return self->priv->convert;
}

/**
 * gdata_documents_upload_query_set_convert:
 * @self: a #GDataDocumentsUploadQuery
 * @convert: %TRUE to convert documents to common formats, %FALSE to upload them unmodified
 *
 * Sets #GDataDocumentsUploadQuery:convert to @convert.
 *
 * Since: 0.13.0
 */
void
gdata_documents_upload_query_set_convert (GDataDocumentsUploadQuery *self, gboolean convert)
{
	g_return_if_fail (GDATA_IS_DOCUMENTS_UPLOAD_QUERY (self));

	if (convert == self->priv->convert) {
		return;
	}

	self->priv->convert = convert;
	g_object_notify (G_OBJECT (self), "convert");
}
