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

/**
 * SECTION:gdata-documents-drawing
 * @short_description: GData Documents drawing object
 * @stability: Unstable
 * @include: gdata/services/documents/gdata-documents-drawing.h
 *
 * #GDataDocumentsDrawing is a subclass of #GDataDocumentsDocument to represent a drawing document from Google Documents.
 *
 * For more details of Google Documents' GData API, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/">online documentation</ulink>.
 *
 * Since: 0.13.1
 **/

#include <config.h>
#include <glib.h>

#include "gdata-documents-drawing.h"
#include "gdata-parser.h"

G_DEFINE_TYPE (GDataDocumentsDrawing, gdata_documents_drawing, GDATA_TYPE_DOCUMENTS_DOCUMENT)

static void
gdata_documents_drawing_class_init (GDataDocumentsDrawingClass *klass)
{
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	entry_class->kind_term = "http://schemas.google.com/docs/2007#drawing";
}

static void
gdata_documents_drawing_init (GDataDocumentsDrawing *self)
{
	/* Why am I writing it? */
}

/**
 * gdata_documents_drawing_new:
 * @id: (allow-none): the entry's ID (not the document ID of the drawing document), or %NULL
 *
 * Creates a new #GDataDocumentsDrawing with the given entry ID (#GDataEntry:id).
 *
 * Return value: (transfer full): a new #GDataDocumentsDrawing, or %NULL; unref with g_object_unref()
 *
 * Since: 0.13.1
 **/
GDataDocumentsDrawing *
gdata_documents_drawing_new (const gchar *id)
{
	return GDATA_DOCUMENTS_DRAWING (g_object_new (GDATA_TYPE_DOCUMENTS_DRAWING, "id", id, NULL));
}
