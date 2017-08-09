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

#ifndef GDATA_DOCUMENTS_DRAWING_H
#define GDATA_DOCUMENTS_DRAWING_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/services/documents/gdata-documents-document.h>

G_BEGIN_DECLS

/**
 * GDATA_DOCUMENTS_DRAWING_JPEG:
 *
 * The export format for JPEG image format.
 *
 * For more information, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#valid_download_formats_for_drawings">
 * GData protocol specification</ulink>.
 *
 * Since: 0.13.1
 */
#define GDATA_DOCUMENTS_DRAWING_JPEG "jpeg"

/**
 * GDATA_DOCUMENTS_DRAWING_PDF:
 *
 * The export format for Portable Document Format (PDF).
 *
 * For more information, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#valid_download_formats_for_drawings">
 * GData protocol specification</ulink>.
 *
 * Since: 0.13.1
 */
#define GDATA_DOCUMENTS_DRAWING_PDF "pdf"

/**
 * GDATA_DOCUMENTS_DRAWING_PNG:
 *
 * The export format for Portable Network Graphics (PNG) image format.
 *
 * For more information, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#valid_download_formats_for_drawings">
 * GData protocol specification</ulink>.
 *
 * Since: 0.13.1
 */
#define GDATA_DOCUMENTS_DRAWING_PNG "png"

/**
 * GDATA_DOCUMENTS_DRAWING_SVG:
 *
 * The export format for Scalable Vector Graphics (SVG) image format.
 *
 * For more information, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#valid_download_formats_for_drawings">
 * GData protocol specification</ulink>.
 *
 * Since: 0.13.1
 */
#define GDATA_DOCUMENTS_DRAWING_SVG "svg"


#define GDATA_TYPE_DOCUMENTS_DRAWING		(gdata_documents_drawing_get_type ())
#define GDATA_DOCUMENTS_DRAWING(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_DOCUMENTS_DRAWING, GDataDocumentsDrawing))
#define GDATA_DOCUMENTS_DRAWING_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_DOCUMENTS_DRAWING, GDataDocumentsDrawingClass))
#define GDATA_IS_DOCUMENTS_DRAWING(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_DOCUMENTS_DRAWING))
#define GDATA_IS_DOCUMENTS_DRAWING_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_DOCUMENTS_DRAWING))
#define GDATA_DOCUMENTS_DRAWING_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_DOCUMENTS_DRAWING, GDataDocumentsDrawingClass))

typedef struct _GDataDocumentsDrawingPrivate	GDataDocumentsDrawingPrivate;

/**
 * GDataDocumentsDrawing:
 *
 * All the fields in the #GDataDocumentsDrawing structure are private and should never be accessed directly.
 *
 * Since: 0.13.1
 */
typedef struct {
	GDataDocumentsDocument parent;
	GDataDocumentsDrawingPrivate *priv;
} GDataDocumentsDrawing;

/**
 * GDataDocumentsDrawingClass:
 *
 * All the fields in the #GDataDocumentsDrawingClass structure are private and should never be accessed directly.
 *
 * Since: 0.13.1
 */
typedef struct {
	/*< private >*/
	GDataDocumentsDocumentClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataDocumentsDrawingClass;

GType gdata_documents_drawing_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataDocumentsDrawing, g_object_unref)

GDataDocumentsDrawing *gdata_documents_drawing_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_DOCUMENTS_DRAWING_H */
