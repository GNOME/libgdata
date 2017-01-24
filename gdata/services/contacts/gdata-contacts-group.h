/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
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

#ifndef GDATA_CONTACTS_GROUP_H
#define GDATA_CONTACTS_GROUP_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-entry.h>

G_BEGIN_DECLS

/**
 * GDATA_CONTACTS_GROUP_CONTACTS:
 *
 * The system group ID for the "My Contacts" system group.
 *
 * Since: 0.7.0
 */
#define GDATA_CONTACTS_GROUP_CONTACTS "Contacts"

/**
 * GDATA_CONTACTS_GROUP_FRIENDS:
 *
 * The system group ID for the "Friends" system group.
 *
 * Since: 0.7.0
 */
#define GDATA_CONTACTS_GROUP_FRIENDS "Friends"

/**
 * GDATA_CONTACTS_GROUP_FAMILY:
 *
 * The system group ID for the "Family" system group.
 *
 * Since: 0.7.0
 */
#define GDATA_CONTACTS_GROUP_FAMILY "Family"

/**
 * GDATA_CONTACTS_GROUP_COWORKERS:
 *
 * The system group ID for the "Coworkers" system group.
 *
 * Since: 0.7.0
 */
#define GDATA_CONTACTS_GROUP_COWORKERS "Coworkers"

#define GDATA_TYPE_CONTACTS_GROUP		(gdata_contacts_group_get_type ())
#define GDATA_CONTACTS_GROUP(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_CONTACTS_GROUP, GDataContactsGroup))
#define GDATA_CONTACTS_GROUP_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_CONTACTS_GROUP, GDataContactsGroupClass))
#define GDATA_IS_CONTACTS_GROUP(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_CONTACTS_GROUP))
#define GDATA_IS_CONTACTS_GROUP_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_CONTACTS_GROUP))
#define GDATA_CONTACTS_GROUP_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_CONTACTS_GROUP, GDataContactsGroupClass))

typedef struct _GDataContactsGroupPrivate	GDataContactsGroupPrivate;

/**
 * GDataContactsGroup:
 *
 * All the fields in the #GDataContactsGroup structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	GDataEntry parent;
	GDataContactsGroupPrivate *priv;
} GDataContactsGroup;

/**
 * GDataContactsGroupClass:
 *
 * All the fields in the #GDataContactsGroupClass structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	/*< private >*/
	GDataEntryClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataContactsGroupClass;

GType gdata_contacts_group_get_type (void) G_GNUC_CONST;

GDataContactsGroup *gdata_contacts_group_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

gint64 gdata_contacts_group_get_edited (GDataContactsGroup *self);
gboolean gdata_contacts_group_is_deleted (GDataContactsGroup *self) G_GNUC_PURE;
const gchar *gdata_contacts_group_get_system_group_id (GDataContactsGroup *self) G_GNUC_PURE;

const gchar *gdata_contacts_group_get_extended_property (GDataContactsGroup *self, const gchar *name) G_GNUC_PURE;
GHashTable *gdata_contacts_group_get_extended_properties (GDataContactsGroup *self) G_GNUC_PURE;
gboolean gdata_contacts_group_set_extended_property (GDataContactsGroup *self, const gchar *name, const gchar *value);

G_END_DECLS

#endif /* !GDATA_CONTACTS_GROUP_H */
