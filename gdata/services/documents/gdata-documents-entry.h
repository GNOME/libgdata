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

#ifndef GDATA_DOCUMENTS_ENTRY_H
#define GDATA_DOCUMENTS_ENTRY_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-entry.h>
#include <gdata/gdata-service.h>
#include <gdata/atom/gdata-author.h>
#include <gdata/services/documents/gdata-documents-property.h>

G_BEGIN_DECLS

/**
 * GDATA_DOCUMENTS_ACCESS_ROLE_OWNER:
 *
 * The users specified by the #GDataAccessRule have full owner access to the document. This allows them to modify the access rules and delete
 * the document, amongst other things.
 *
 * Since: 0.7.0
 */
#define GDATA_DOCUMENTS_ACCESS_ROLE_OWNER "owner"

/**
 * GDATA_DOCUMENTS_ACCESS_ROLE_WRITER:
 *
 * The users specified by the #GDataAccessRule have write access to the document. They cannot modify the access rules or delete the document.
 *
 * Since: 0.7.0
 */
#define GDATA_DOCUMENTS_ACCESS_ROLE_WRITER "writer"

/**
 * GDATA_DOCUMENTS_ACCESS_ROLE_READER:
 *
 * The users specified by the #GDataAccessRule have read-only access to the document.
 *
 * Since: 0.7.0
 */
#define GDATA_DOCUMENTS_ACCESS_ROLE_READER "reader"

#define GDATA_TYPE_DOCUMENTS_ENTRY		(gdata_documents_entry_get_type ())
#define GDATA_DOCUMENTS_ENTRY(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_DOCUMENTS_ENTRY, GDataDocumentsEntry))
#define GDATA_DOCUMENTS_ENTRY_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_DOCUMENTS_ENTRY, GDataDocumentsEntryClass))
#define GDATA_IS_DOCUMENTS_ENTRY(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_DOCUMENTS_ENTRY))
#define GDATA_IS_DOCUMENTS_ENTRY_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_DOCUMENTS_ENTRY))
#define GDATA_DOCUMENTS_ENTRY_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_DOCUMENTS_ENTRY, GDataDocumentsEntryClass))

typedef struct _GDataDocumentsEntryPrivate	GDataDocumentsEntryPrivate;

/**
 * GDataDocumentsEntry:
 *
 * All the fields in the #GDataDocumentsEntry structure are private and should never be accessed directly.
 *
 * Since: 0.4.0
 */
typedef struct {
	GDataEntry parent;
	GDataDocumentsEntryPrivate *priv;
} GDataDocumentsEntry;

/**
 * GDataDocumentsEntryClass:
 *
 * All the fields in the #GDataDocumentsEntryClass structure are private and should never be accessed directly.
 *
 * Since: 0.4.0
 */
typedef struct {
	/*< private >*/
	GDataEntryClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataDocumentsEntryClass;

GType gdata_documents_entry_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataDocumentsEntry, g_object_unref)

gchar *gdata_documents_entry_get_path (GDataDocumentsEntry *self) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_documents_entry_get_resource_id (GDataDocumentsEntry *self) G_GNUC_PURE;

gint64 gdata_documents_entry_get_last_viewed (GDataDocumentsEntry *self);

void gdata_documents_entry_set_writers_can_invite (GDataDocumentsEntry *self, gboolean writers_can_invite);
gboolean gdata_documents_entry_writers_can_invite (GDataDocumentsEntry *self) G_GNUC_PURE;

GDataAuthor *gdata_documents_entry_get_last_modified_by (GDataDocumentsEntry *self) G_GNUC_PURE;

goffset gdata_documents_entry_get_quota_used (GDataDocumentsEntry *self) G_GNUC_PURE;
goffset gdata_documents_entry_get_file_size (GDataDocumentsEntry *self) G_GNUC_PURE;

gboolean gdata_documents_entry_is_deleted (GDataDocumentsEntry *self) G_GNUC_PURE;

GList *gdata_documents_entry_get_document_properties (GDataDocumentsEntry *self);

gboolean gdata_documents_entry_add_documents_property (GDataDocumentsEntry *self, GDataDocumentsProperty *property);
gboolean gdata_documents_entry_remove_documents_property (GDataDocumentsEntry *self, GDataDocumentsProperty *property);

gint64 gdata_documents_entry_get_shared_with_me_date (GDataDocumentsEntry *self);

gboolean gdata_documents_entry_can_edit (GDataDocumentsEntry *self);

G_END_DECLS

#endif /* !GDATA_DOCUMENTS_ENTRY_H */
