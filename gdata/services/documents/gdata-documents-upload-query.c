/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2012 <philip@tecnocode.co.uk>
 *
 * GData Client is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GData Client is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GData Client.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * SECTION:gdata-documents-upload-query
 * @short_description: GData Documents upload query object
 * @stability: Unstable
 * @include: gdata/services/documents/gdata-documents-upload-query.h
 *
 * #GDataDocumentsUploadQuery is a collection of parameters for document uploads to Google Documents, allowing various options to be set when uploading
 * a document for the first time. For example, the destination folder for the uploaded document may be specified; or whether to automatically convert
 * the document to a common format.
 *
 * #GDataDocumentsUploadQuery is designed as an object (rather than a fixed struct or set of function arguments) to allow for easy additions of new
 * Google Documents features in the future.
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
};

enum {
	PROP_FOLDER = 1,
};

G_DEFINE_TYPE (GDataDocumentsUploadQuery, gdata_documents_upload_query, G_TYPE_OBJECT)

static void
gdata_documents_upload_query_class_init (GDataDocumentsUploadQueryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataDocumentsUploadQueryPrivate));

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
}

static void
gdata_documents_upload_query_init (GDataDocumentsUploadQuery *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_DOCUMENTS_UPLOAD_QUERY, GDataDocumentsUploadQueryPrivate);
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

	return base_uri;
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
