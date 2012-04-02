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
 * SECTION:gdata-documents-folder
 * @short_description: GData documents folder object
 * @stability: Unstable
 * @include: gdata/services/documents/gdata-documents-folder.h
 *
 * #GDataDocumentsFolder is a subclass of #GDataDocumentsEntry to represent a folder from Google Documents.
 *
 * For more details of Google Documents' GData API, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/">online documentation</ulink>.
 *
 * <example>
 * 	<title>Adding a Folder</title>
 * 	<programlisting>
 *	GDataDocumentsService *service;
 *	GDataDocumentsFolder *folder, *new_folder;
 *	gchar *upload_uri;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service *<!-- -->/
 *	service = create_documents_service ();
 *
 *	/<!-- -->* Create the new folder *<!-- -->/
 *	folder = gdata_documents_folder_new (NULL);
 *	gdata_entry_set_title (GDATA_ENTRY (folder), "Folder Name");
 *
 *	/<!-- -->* Insert the folder *<!-- -->/
 *	upload_uri = gdata_documents_service_get_upload_uri (NULL);
 *	new_folder = GDATA_DOCUMENTS_FOLDER (gdata_service_insert_entry (GDATA_SERVICE (service), upload_uri, GDATA_ENTRY (folder), NULL, &error));
 *	g_free (upload_uri);
 *
 *	g_object_unref (folder);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error inserting new folder: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	/<!-- -->* Do something with the new folder, such as store its ID for future use *<!-- -->/
 *
 *	g_object_unref (new_folder);
 * 	</programlisting>
 * </example>
 *
 * Since: 0.4.0
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-documents-folder.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "gdata-private.h"

G_DEFINE_TYPE (GDataDocumentsFolder, gdata_documents_folder, GDATA_TYPE_DOCUMENTS_ENTRY)

static void
gdata_documents_folder_class_init (GDataDocumentsFolderClass *klass)
{
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	entry_class->kind_term = "http://schemas.google.com/docs/2007#folder";
}

static void
gdata_documents_folder_init (GDataDocumentsFolder *self)
{
	/* Why am I writing it? */
}

/**
 * gdata_documents_folder_new:
 * @id: (allow-none): the entry's ID (not the document ID of the folder), or %NULL
 *
 * Creates a new #GDataDocumentsFolder with the given entry ID (#GDataEntry:id).
 *
 * Return value: a new #GDataDocumentsFolder, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataDocumentsFolder *
gdata_documents_folder_new (const gchar *id)
{
	return GDATA_DOCUMENTS_FOLDER (g_object_new (GDATA_TYPE_DOCUMENTS_FOLDER, "id", id, NULL));
}
