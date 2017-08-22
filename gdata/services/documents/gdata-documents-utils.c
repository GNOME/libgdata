/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Red Hat, Inc. 2015, 2016
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

#include <string.h>

#include "gdata-documents-spreadsheet.h"
#include "gdata-documents-text.h"
#include "gdata-documents-presentation.h"
#include "gdata-documents-folder.h"
#include "gdata-documents-drawing.h"
#include "gdata-documents-pdf.h"
#include "gdata-documents-utils.h"

/*
 * gdata_documents_utils_add_content_type:
 * @entry: a #GDataDocumentsEntry
 * @content_type: the new entry content-type
 *
 * Adds a #GDataCategory representing @content_type to @entry.
 *
 * Since: 0.17.7
 */
void
gdata_documents_utils_add_content_type (GDataDocumentsEntry *entry, const gchar *content_type)
{
	GDataCategory *category;
	GDataEntryClass *klass = GDATA_ENTRY_GET_CLASS (entry);

	if (content_type == NULL || content_type[0] == '\0')
		return;

	category = gdata_category_new (klass->kind_term, "http://schemas.google.com/g/2005#kind", content_type);
	gdata_entry_add_category (GDATA_ENTRY (entry), category);
	g_object_unref (category);
}

/*
 * gdata_documents_utils_get_type_from_content_type:
 * @content_type: the content type
 *
 * Maps @content_type to a #GType representing a #GDataDocumentsEntry
 * sub-class.
 *
 * Return value: a #GType corresponding to @content_type
 *
 * Since: 0.17.2
 */
GType
gdata_documents_utils_get_type_from_content_type (const gchar *content_type)
{
	GType retval;

	/* MIME types: https://developers.google.com/drive/web/mime-types */

	if (g_strcmp0 (content_type, "application/vnd.google-apps.folder") == 0) {
		retval = GDATA_TYPE_DOCUMENTS_FOLDER;
	} else if (g_strcmp0 (content_type, "application/pdf") == 0) {
		retval = GDATA_TYPE_DOCUMENTS_PDF;
	} else if (g_strcmp0 (content_type, "application/vnd.google-apps.document") == 0) {
		retval = GDATA_TYPE_DOCUMENTS_TEXT;
	} else if (g_strcmp0 (content_type, "application/vnd.google-apps.drawing") == 0) {
		retval = GDATA_TYPE_DOCUMENTS_DRAWING;
	} else if (g_strcmp0 (content_type, "application/vnd.google-apps.presentation") == 0) {
		retval = GDATA_TYPE_DOCUMENTS_PRESENTATION;
	} else if (g_strcmp0 (content_type, "application/vnd.google-apps.spreadsheet") == 0) {
		retval = GDATA_TYPE_DOCUMENTS_SPREADSHEET;
	} else {
		retval = GDATA_TYPE_DOCUMENTS_DOCUMENT;
	}

	return retval;
}

/*
 * gdata_documents_utils_get_content_type:
 * @entry: a #GDataDocumentsEntry
 *
 * Returns the content type of @entry, if any.
 *
 * Return value: (nullable): content type of @entry, %NULL otherwise
 *
 * Since: 0.17.5
 */
const gchar *
gdata_documents_utils_get_content_type (GDataDocumentsEntry *entry)
{
	GList *categories;
	GList *i;
	const gchar *retval = NULL;

	categories = gdata_entry_get_categories (GDATA_ENTRY (entry));
	for (i = categories; i != NULL; i = i->next) {
		GDataCategory *category = GDATA_CATEGORY (i->data);
		const gchar *label;
		const gchar *scheme;

		label = gdata_category_get_label (category);
		scheme = gdata_category_get_scheme (category);
		if (label != NULL && label[0] != '\0' && g_strcmp0 (scheme, "http://schemas.google.com/g/2005#kind") == 0) {
			retval = label;
			break;
		}
	}

	return retval;
}

/*
 * gdata_documents_utils_get_id_from_link:
 * @_link: a #GDataLink
 *
 * Returns the ID, if any, of the #GDataEntry pointed to by @_link.
 *
 * Return value: (nullable): ID of @_link, %NULL otherwise
 *
 * Since: 0.17.9
 */
const gchar *
gdata_documents_utils_get_id_from_link (GDataLink *_link)
{
	const gchar *retval = NULL;
	const gchar *uri;

	/* HACK: Extract the ID from the GDataLink:uri by removing the prefix. Ignore links which
	 * don't have the prefix. */
	uri = gdata_link_get_uri (_link);
	if (g_str_has_prefix (uri, GDATA_DOCUMENTS_URI_PREFIX)) {
		const gchar *id;
		gsize uri_prefix_len;

		uri_prefix_len = strlen (GDATA_DOCUMENTS_URI_PREFIX);
		id = uri + uri_prefix_len;
		if (id[0] != '\0')
			retval = id;
	}

	return retval;
}
