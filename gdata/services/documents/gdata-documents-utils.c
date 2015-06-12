/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Red Hat, Inc. 2015
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

#include "gdata-documents-entry.h"
#include "gdata-documents-spreadsheet.h"
#include "gdata-documents-text.h"
#include "gdata-documents-presentation.h"
#include "gdata-documents-folder.h"
#include "gdata-documents-drawing.h"
#include "gdata-documents-pdf.h"
#include "gdata-documents-utils.h"

/*
 * gdata_documents_utils_get_type_from_content_type:
 * @content_type: the content type
 *
 * Maps @content_type to a #GType representing a #GDataDocumentsEntry
 * sub-class.
 *
 * Return value: a #GType corresponding to @content_type
 *
 * Since: UNRELEASED
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
