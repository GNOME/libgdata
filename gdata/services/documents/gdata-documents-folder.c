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
 * <ulink type="http://code.google.com/apis/document/docs/2.0/developers_guide_protocol.html">online documentation</ulink>.
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

static void get_xml (GDataParsable *parsable, GString *xml_string);

G_DEFINE_TYPE (GDataDocumentsFolder, gdata_documents_folder, GDATA_TYPE_DOCUMENTS_ENTRY)
#define GDATA_DOCUMENTS_FOLDER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDATA_TYPE_DOCUMENTS_ENTRY, GDataDocumentsEntryPrivate))

static void
gdata_documents_folder_class_init (GDataDocumentsFolderClass *klass)
{
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	parsable_class->get_xml = get_xml;
}

/**
 * gdata_documents_folder_new:
 * @id: the entry's ID (not the document ID of the folder), or %NULL
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
	GDataDocumentsFolder *folder = GDATA_DOCUMENTS_FOLDER (g_object_new (GDATA_TYPE_DOCUMENTS_FOLDER, "id", id, NULL));

	/* Set the edited property to the current time (creation time). We don't do this in *_init() since that would cause
	 * setting it from parse_xml() to fail (duplicate element). */
	_gdata_documents_entry_init_edited (GDATA_DOCUMENTS_ENTRY (folder));

	return folder;
}

static void
gdata_documents_folder_init (GDataDocumentsFolder *self)
{
	/* Why am I writing it? */
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	const gchar *document_id;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_documents_folder_parent_class)->get_xml (parsable, xml_string);

	document_id = gdata_documents_entry_get_document_id (GDATA_DOCUMENTS_ENTRY (parsable));
	if (document_id != NULL)
		g_string_append_printf (xml_string, "<gd:resourceId>folder:%s</gd:resourceId>", document_id);
}
