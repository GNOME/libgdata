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
 * SECTION:gdata-documents-text
 * @short_description: GData Documents text object
 * @stability: Unstable
 * @include: gdata/services/documents/gdata-documents-text.h
 *
 * #GDataDocumentsText is a subclass of #GDataDocumentsDocument to represent a text document from Google Documents.
 *
 * For more details of Google Documents' GData API, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/">online documentation</ulink>.
 *
 * Since: 0.4.0
 **/

#include <config.h>
#include <glib.h>

#include "gdata-documents-text.h"
#include "gdata-parser.h"

G_DEFINE_TYPE (GDataDocumentsText, gdata_documents_text, GDATA_TYPE_DOCUMENTS_DOCUMENT)

static void
gdata_documents_text_class_init (GDataDocumentsTextClass *klass)
{
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	entry_class->kind_term = "http://schemas.google.com/docs/2007#document";
}

static void
gdata_documents_text_init (GDataDocumentsText *self)
{
	/* Why am I writing it? */
}

/**
 * gdata_documents_text_new:
 * @id: (allow-none): the entry's ID (not the document ID of the text document), or %NULL
 *
 * Creates a new #GDataDocumentsText with the given entry ID (#GDataEntry:id).
 *
 * Return value: (transfer full): a new #GDataDocumentsText, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 **/
GDataDocumentsText *
gdata_documents_text_new (const gchar *id)
{
	return GDATA_DOCUMENTS_TEXT (g_object_new (GDATA_TYPE_DOCUMENTS_TEXT, "id", id, NULL));
}
