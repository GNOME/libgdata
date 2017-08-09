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

#ifndef GDATA_DOCUMENTS_SPREADSHEET_H
#define GDATA_DOCUMENTS_SPREADSHEET_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/services/documents/gdata-documents-document.h>

G_BEGIN_DECLS

/**
 * GDATA_DOCUMENTS_SPREADSHEET_CSV:
 *
 * The export format for Comma-Separated Values (CSV) format.
 *
 * For more information, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#valid_formats_for_spreadsheets">
 * GData protocol specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_DOCUMENTS_SPREADSHEET_CSV "csv"

/**
 * GDATA_DOCUMENTS_SPREADSHEET_ODS:
 *
 * The export format for OpenDocument Spreadsheet (ODS) format.
 *
 * For more information, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#valid_formats_for_spreadsheets">
 * GData protocol specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_DOCUMENTS_SPREADSHEET_ODS "ods"

/**
 * GDATA_DOCUMENTS_SPREADSHEET_PDF:
 *
 * The export format for Portable Document Format (PDF).
 *
 * For more information, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#valid_formats_for_spreadsheets">
 * GData protocol specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_DOCUMENTS_SPREADSHEET_PDF "pdf"

/**
 * GDATA_DOCUMENTS_SPREADSHEET_TSV:
 *
 * The export format for Tab-Separated Values (TSV) format.
 *
 * For more information, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#valid_formats_for_spreadsheets">
 * GData protocol specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_DOCUMENTS_SPREADSHEET_TSV "tsv"

/**
 * GDATA_DOCUMENTS_SPREADSHEET_XLS:
 *
 * The export format for Microsoft Excel spreadsheet (XLS) format.
 *
 * For more information, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#valid_formats_for_spreadsheets">
 * GData protocol specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_DOCUMENTS_SPREADSHEET_XLS "xls"

/**
 * GDATA_DOCUMENTS_SPREADSHEET_HTML:
 *
 * The export format for HyperText Markup Language (HTML) format.
 *
 * For more information, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#valid_formats_for_spreadsheets">
 * GData protocol specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_DOCUMENTS_SPREADSHEET_HTML "html"

#define GDATA_TYPE_DOCUMENTS_SPREADSHEET		(gdata_documents_spreadsheet_get_type ())
#define GDATA_DOCUMENTS_SPREADSHEET(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_DOCUMENTS_SPREADSHEET, \
                                                         GDataDocumentsSpreadsheet))
#define GDATA_DOCUMENTS_SPREADSHEET_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_DOCUMENTS_SPREADSHEET, \
                                                         GDataDocumentsSpreadsheetClass))
#define GDATA_IS_DOCUMENTS_SPREADSHEET(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_DOCUMENTS_SPREADSHEET))
#define GDATA_IS_DOCUMENTS_SPREADSHEET_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_DOCUMENTS_SPREADSHEET))
#define GDATA_DOCUMENTS_SPREADSHEET_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_DOCUMENTS_SPREADSHEET, \
                                                         GDataDocumentsSpreadsheetClass))

typedef struct _GDataDocumentsSpreadsheetPrivate	GDataDocumentsSpreadsheetPrivate;

/**
 * GDataDocumentsSpreadsheet:
 *
 * All the fields in the #GDataDocumentsSpreadsheet structure are private and should never be accessed directly.
 *
 * Since: 0.4.0
 */
typedef struct {
	GDataDocumentsDocument parent;
	GDataDocumentsSpreadsheetPrivate *priv;
} GDataDocumentsSpreadsheet;

/**
 * GDataDocumentsSpreadsheetClass:
 *
 * All the fields in the #GDataDocumentsSpreadsheetClass structure are private and should never be accessed directly.
 *
 * Since: 0.4.0
 */
typedef struct {
	/*< private >*/
	GDataDocumentsDocumentClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataDocumentsSpreadsheetClass;

GType gdata_documents_spreadsheet_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataDocumentsSpreadsheet, g_object_unref)

GDataDocumentsSpreadsheet *gdata_documents_spreadsheet_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

gchar *gdata_documents_spreadsheet_get_download_uri (GDataDocumentsSpreadsheet *self, const gchar *export_format,
                                                     gint gid) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_DOCUMENTS_SPREADSHEET_H */
