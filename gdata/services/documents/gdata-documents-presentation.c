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
 * SECTION:gdata-documents-presentation
 * @short_description: GData documents presentation object
 * @stability: Unstable
 * @include: gdata/services/documents/gdata-documents-presentation.h
 *
 * #GDataDocumentsPresentation is a subclass of #GDataDocumentsDocument to represent a Google Documents presentation.
 *
 * For more details of Google Documents' GData API, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/">online documentation</ulink>.
 *
 * Since: 0.4.0
 **/

#include <config.h>
#include <glib.h>

#include "gdata-documents-presentation.h"
#include "gdata-parser.h"

G_DEFINE_TYPE (GDataDocumentsPresentation, gdata_documents_presentation, GDATA_TYPE_DOCUMENTS_DOCUMENT)

static void
gdata_documents_presentation_class_init (GDataDocumentsPresentationClass *klass)
{
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	entry_class->kind_term = "http://schemas.google.com/docs/2007#presentation";
}

static void
gdata_documents_presentation_init (GDataDocumentsPresentation *self)
{
	/* Why am I writing it? */
}

/**
 * gdata_documents_presentation_new:
 * @id: (allow-none): the entry's ID (not the document ID of the presentation), or %NULL
 *
 * Creates a new #GDataDocumentsPresentation with the given entry ID (#GDataEntry:id).
 *
 * Return value: (transfer full): a new #GDataDocumentsPresentation, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataDocumentsPresentation *
gdata_documents_presentation_new (const gchar *id)
{
	return GDATA_DOCUMENTS_PRESENTATION (g_object_new (GDATA_TYPE_DOCUMENTS_PRESENTATION, "id", id, NULL));
}
