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

#ifndef GDATA_DOCUMENTS_DRIVE_H
#define GDATA_DOCUMENTS_DRIVE_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-entry.h>

G_BEGIN_DECLS

#define GDATA_TYPE_DOCUMENTS_DRIVE gdata_documents_drive_get_type ()
G_DECLARE_DERIVABLE_TYPE (GDataDocumentsDrive, gdata_documents_drive, GDATA, DOCUMENTS_DRIVE, GDataEntry)


/**
 * GDataDocumentsDriveClass:
 *
 * All the fields in the #GDataDocumentsDriveClass structure are private and should never be accessed directly.
 *
 * Since: 0.18.0
 */
struct _GDataDocumentsDriveClass {
	/*< private >*/
	GDataEntryClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
};

const gchar *gdata_documents_drive_get_name (GDataDocumentsDrive *self) G_GNUC_PURE;

G_END_DECLS

#endif /* !GDATA_DOCUMENTS_DRIVE_H */
