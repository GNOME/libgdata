/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Thibault Saunier 2009 <saunierthibault@gmail.com>
 * Copyright (C) Philip Withnall 2010 <philip@tecnocode.co.uk>
 * Copyright (C) Cosimo Cecchi 2012 <cosimoc@gnome.org>
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

#ifndef GDATA_DOCUMENTS_PDF_H
#define GDATA_DOCUMENTS_PDF_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/services/documents/gdata-documents-document.h>

G_BEGIN_DECLS

#define GDATA_TYPE_DOCUMENTS_PDF		(gdata_documents_pdf_get_type ())
#define GDATA_DOCUMENTS_PDF(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_DOCUMENTS_PDF, GDataDocumentsPdf))
#define GDATA_DOCUMENTS_PDF_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_DOCUMENTS_PDF, GDataDocumentsPdfClass))
#define GDATA_IS_DOCUMENTS_PDF(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_DOCUMENTS_PDF))
#define GDATA_IS_DOCUMENTS_PDF_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_DOCUMENTS_PDF))
#define GDATA_DOCUMENTS_PDF_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_DOCUMENTS_PDF, GDataDocumentsPdfClass))

typedef struct _GDataDocumentsPdfPrivate	GDataDocumentsPdfPrivate;

/**
 * GDataDocumentsPdf:
 *
 * All the fields in the #GDataDocumentsPdf structure are private and should never be accessed directly.
 *
 * Since: 0.13.3
 */
typedef struct {
	GDataDocumentsDocument parent;
	GDataDocumentsPdfPrivate *priv;
} GDataDocumentsPdf;

/**
 * GDataDocumentsPdfClass:
 *
 * All the fields in the #GDataDocumentsPdfClass structure are private and should never be accessed directly.
 *
 * Since: 0.13.3
 */
typedef struct {
	/*< private >*/
	GDataDocumentsDocumentClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataDocumentsPdfClass;

GType gdata_documents_pdf_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataDocumentsPdf, g_object_unref)

GDataDocumentsPdf *gdata_documents_pdf_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_DOCUMENTS_PDF_H */
