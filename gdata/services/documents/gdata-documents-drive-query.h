/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Ondrej Holy 2020 <oholy@redhat.com>
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

#ifndef GDATA_DOCUMENTS_DRIVE_QUERY_H
#define GDATA_DOCUMENTS_DRIVE_QUERY_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-types.h>
#include <gdata/gdata-query.h>

G_BEGIN_DECLS

#define GDATA_TYPE_DOCUMENTS_DRIVE_QUERY gdata_documents_drive_query_get_type ()
G_DECLARE_DERIVABLE_TYPE (GDataDocumentsDriveQuery, gdata_documents_drive_query, GDATA, DOCUMENTS_DRIVE_QUERY, GDataQuery)

/**
 * GDataDocumentsDriveQueryClass:
 *
 * All the fields in the #GDataDocumentsDriveQueryClass structure are private and should never be accessed directly.
 *
 * Since: 0.18.0
 */
struct _GDataDocumentsDriveQueryClass {
	/*< private >*/
	GDataQueryClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
};

GDataDocumentsDriveQuery *gdata_documents_drive_query_new (const gchar *q) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_DOCUMENTS_DRIVE_QUERY_H */
