/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Thibault Saunier 2009 <saunierthibault@gmail.com>
 * Copyright (C) Philip Withnall 2010 <philip@tecnocode.co.uk>
 * Copyright (C) Red Hat, Inc. 2015, 2016, 2017
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
 * @stability: Stable
 * @include: gdata/services/documents/gdata-documents-folder.h
 *
 * #GDataDocumentsFolder is a subclass of #GDataDocumentsEntry to represent a folder from Google Documents.
 *
 * For more details of Google Drive's GData API, see the
 * <ulink type="http" url="https://developers.google.com/drive/v2/web/about-sdk">online documentation</ulink>.
 *
 * <example>
 * 	<title>Adding a Folder</title>
 * 	<programlisting>
 * 	GDataAuthorizationDomain *domain;
 *	GDataDocumentsService *service;
 *	GDataDocumentsFolder *folder, *new_folder, *parent_folder;
 *	GError *error = NULL;
 *
 *	domain = gdata_documents_service_get_primary_authorization_domain ();
 *
 *	/<!-- -->* Create a service *<!-- -->/
 *	service = create_documents_service ();
 *
 *	parent_folder = GDATA_DOCUMENTS_FOLDER (gdata_service_query_single_entry (GDATA_SERVICE (service), domain, "root", NULL,
 *	                                                                          GDATA_TYPE_DOCUMENTS_FOLDER, NULL, &error));
 *	if (error != NULL) {
 *		g_error ("Error getting root folder");
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	/<!-- -->* Create the new folder *<!-- -->/
 *	folder = gdata_documents_folder_new (NULL);
 *	gdata_entry_set_title (GDATA_ENTRY (folder), "Folder Name");
 *
 *	/<!-- -->* Insert the folder *<!-- -->/
 *	new_folder = GDATA_DOCUMENTS_FOLDER (gdata_documents_service_add_entry_to_folder (GDATA_SERVICE (service), GDATA_DOCUMENTS_ENTRY (folder),
 *	parent_folder, NULL, &error));
 *
 *	g_object_unref (folder);
 *	g_object_unref (parent_folder);
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
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-documents-entry-private.h"
#include "gdata-documents-folder.h"
#include "gdata-documents-utils.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "gdata-private.h"

static void gdata_documents_folder_constructed (GObject *object);
static gboolean post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error);

G_DEFINE_TYPE (GDataDocumentsFolder, gdata_documents_folder, GDATA_TYPE_DOCUMENTS_ENTRY)

static void
gdata_documents_folder_class_init (GDataDocumentsFolderClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	gobject_class->constructed = gdata_documents_folder_constructed;
	parsable_class->post_parse_json = post_parse_json;
	entry_class->kind_term = "http://schemas.google.com/docs/2007#folder";
}

static void
gdata_documents_folder_init (GDataDocumentsFolder *self)
{
	/* Why am I writing it? */
}

static void
gdata_documents_folder_constructed (GObject *object)
{
	G_OBJECT_CLASS (gdata_documents_folder_parent_class)->constructed (object);

	if (!_gdata_parsable_is_constructed_from_xml (GDATA_PARSABLE (object)))
		gdata_documents_utils_add_content_type (GDATA_DOCUMENTS_ENTRY (object), "application/vnd.google-apps.folder");
}

static gboolean
post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error)
{
	const gchar *id;
	gchar *resource_id;

	id = gdata_entry_get_id (GDATA_ENTRY (parsable));

	/* Since the document-id is identical to GDataEntry:id, which is parsed by the parent class, we can't
	 * create the resource-id while parsing. */
	resource_id = g_strconcat ("folder:", id, NULL);
	_gdata_documents_entry_set_resource_id (GDATA_DOCUMENTS_ENTRY (parsable), resource_id);

	g_free (resource_id);
	return GDATA_PARSABLE_CLASS (gdata_documents_folder_parent_class)->post_parse_json (parsable, user_data, error);
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
 */
GDataDocumentsFolder *
gdata_documents_folder_new (const gchar *id)
{
	return GDATA_DOCUMENTS_FOLDER (g_object_new (GDATA_TYPE_DOCUMENTS_FOLDER, "id", id, NULL));
}
