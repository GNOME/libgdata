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

#ifndef GDATA_DOCUMENTS_PRESENTATION_H
#define GDATA_DOCUMENTS_PRESENTATION_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/services/documents/gdata-documents-document.h>

G_BEGIN_DECLS

/**
 * GDATA_DOCUMENTS_PRESENTATION_PDF:
 *
 * The export format for Portable Document Format (PDF).
 *
 * For more information, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#valid_formats_for_presentations">
 * GData protocol specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_DOCUMENTS_PRESENTATION_PDF "pdf"

/**
 * GDATA_DOCUMENTS_PRESENTATION_PNG:
 *
 * The export format for Portable Network Graphics (PNG) image format.
 *
 * For more information, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#valid_formats_for_presentations">
 * GData protocol specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_DOCUMENTS_PRESENTATION_PNG "png"

/**
 * GDATA_DOCUMENTS_PRESENTATION_PPT:
 *
 * The export format for Microsoft PowerPoint (PPT) format.
 *
 * For more information, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#valid_formats_for_presentations">
 * GData protocol specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_DOCUMENTS_PRESENTATION_PPT "ppt"

/**
 * GDATA_DOCUMENTS_PRESENTATION_TXT:
 *
 * The export format for plain text format.
 *
 * For more information, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#valid_formats_for_presentations">
 * GData protocol specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_DOCUMENTS_PRESENTATION_TXT "txt"

#define GDATA_TYPE_DOCUMENTS_PRESENTATION		(gdata_documents_presentation_get_type ())
#define GDATA_DOCUMENTS_PRESENTATION(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_DOCUMENTS_PRESENTATION, \
                                                         GDataDocumentsPresentation))
#define GDATA_DOCUMENTS_PRESENTATION_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_DOCUMENTS_PRESENTATION, \
                                                         GDataDocumentsPresentationClass))
#define GDATA_IS_DOCUMENTS_PRESENTATION(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_DOCUMENTS_PRESENTATION))
#define GDATA_IS_DOCUMENTS_PRESENTATION_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_DOCUMENTS_PRESENTATION))
#define GDATA_DOCUMENTS_PRESENTATION_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_DOCUMENTS_PRESENTATION, \
                                                         GDataDocumentsPresentationClass))

typedef struct _GDataDocumentsPresentationPrivate	GDataDocumentsPresentationPrivate;

/**
 * GDataDocumentsPresentation:
 *
 * All the fields in the #GDataDocumentsPresentation structure are private and should never be accessed directly.
 *
 * Since: 0.4.0
 */
typedef struct {
	GDataDocumentsDocument parent;
	GDataDocumentsPresentationPrivate *priv;
} GDataDocumentsPresentation;

/**
 * GDataDocumentsPresentationClass:
 *
 * All the fields in the #GDataDocumentsPresentationClass structure are private and should never be accessed directly.
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
} GDataDocumentsPresentationClass;

GType gdata_documents_presentation_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataDocumentsPresentation, g_object_unref)

GDataDocumentsPresentation *gdata_documents_presentation_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_DOCUMENTS_PRESENTATION_H */
