/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Michael Terry 2017 <mike@mterry.name>
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

#ifndef GDATA_DOCUMENTS_METADATA_H
#define GDATA_DOCUMENTS_METADATA_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS


#define GDATA_TYPE_DOCUMENTS_METADATA		(gdata_documents_metadata_get_type ())
#define GDATA_DOCUMENTS_METADATA(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_DOCUMENTS_METADATA, GDataDocumentsMetadata))
#define GDATA_DOCUMENTS_METADATA_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_DOCUMENTS_METADATA, GDataDocumentsMetadataClass))
#define GDATA_IS_DOCUMENTS_METADATA(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_DOCUMENTS_METADATA))
#define GDATA_IS_DOCUMENTS_METADATA_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_DOCUMENTS_METADATA))
#define GDATA_DOCUMENTS_METADATA_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_DOCUMENTS_METADATA, GDataDocumentsMetadataClass))

typedef struct _GDataDocumentsMetadataPrivate	GDataDocumentsMetadataPrivate;

/**
 * GDataDocumentsMetadata:
 *
 * All the fields in the #GDataDocumentsMetadata structure are private and should never be accessed directly.
 *
 * Since: 0.17.9
 */
typedef struct {
	GDataParsable parent;
	GDataDocumentsMetadataPrivate *priv;
} GDataDocumentsMetadata;

/**
 * GDataDocumentsMetadataClass:
 *
 * All the fields in the #GDataDocumentsMetadataClass structure are private and should never be accessed directly.
 *
 * Since: 0.17.9
 */
typedef struct {
	/*< private >*/
	GDataParsableClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataDocumentsMetadataClass;

GType gdata_documents_metadata_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataDocumentsMetadata, g_object_unref)

goffset gdata_documents_metadata_get_quota_total (GDataDocumentsMetadata *self) G_GNUC_PURE;
goffset gdata_documents_metadata_get_quota_used (GDataDocumentsMetadata *self) G_GNUC_PURE;

G_END_DECLS

#endif /* !GDATA_DOCUMENTS_METADATA_H */
