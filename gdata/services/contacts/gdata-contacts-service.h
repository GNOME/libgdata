/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009–2010 <philip@tecnocode.co.uk>
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

#ifndef GDATA_CONTACTS_SERVICE_H
#define GDATA_CONTACTS_SERVICE_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-service.h>
#include <gdata/gdata-query.h>

G_BEGIN_DECLS

#define GDATA_TYPE_CONTACTS_SERVICE		(gdata_contacts_service_get_type ())
#define GDATA_CONTACTS_SERVICE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_CONTACTS_SERVICE, GDataContactsService))
#define GDATA_CONTACTS_SERVICE_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_CONTACTS_SERVICE, GDataContactsServiceClass))
#define GDATA_IS_CONTACTS_SERVICE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_CONTACTS_SERVICE))
#define GDATA_IS_CONTACTS_SERVICE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_CONTACTS_SERVICE))
#define GDATA_CONTACTS_SERVICE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_CONTACTS_SERVICE, GDataContactsServiceClass))

typedef struct _GDataContactsServicePrivate	GDataContactsServicePrivate;

/**
 * GDataContactsService:
 *
 * All the fields in the #GDataContactsService structure are private and should never be accessed directly.
 *
 * Since: 0.2.0
 */
typedef struct {
	GDataService parent;
} GDataContactsService;

/**
 * GDataContactsServiceClass:
 *
 * All the fields in the #GDataContactsServiceClass structure are private and should never be accessed directly.
 *
 * Since: 0.2.0
 */
typedef struct {
	/*< private >*/
	GDataServiceClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
	void (*_g_reserved2) (void);
	void (*_g_reserved3) (void);
	void (*_g_reserved4) (void);
	void (*_g_reserved5) (void);
} GDataContactsServiceClass;

GType gdata_contacts_service_get_type (void) G_GNUC_CONST;

GDataContactsService *gdata_contacts_service_new (GDataAuthorizer *authorizer) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataAuthorizationDomain *gdata_contacts_service_get_primary_authorization_domain (void) G_GNUC_CONST;

GDataFeed *gdata_contacts_service_query_contacts (GDataContactsService *self, GDataQuery *query, GCancellable *cancellable,
                                                  GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                  GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_contacts_service_query_contacts_async (GDataContactsService *self, GDataQuery *query, GCancellable *cancellable,
                                                  GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                  GDestroyNotify destroy_progress_user_data,
                                                  GAsyncReadyCallback callback, gpointer user_data);

#include <gdata/services/contacts/gdata-contacts-contact.h>

GDataContactsContact *gdata_contacts_service_insert_contact (GDataContactsService *self, GDataContactsContact *contact,
                                                             GCancellable *cancellable, GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_contacts_service_insert_contact_async (GDataContactsService *self, GDataContactsContact *contact, GCancellable *cancellable,
                                                  GAsyncReadyCallback callback, gpointer user_data);

#include <gdata/services/contacts/gdata-contacts-group.h>

GDataFeed *gdata_contacts_service_query_groups (GDataContactsService *self, GDataQuery *query, GCancellable *cancellable,
                                                GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_contacts_service_query_groups_async (GDataContactsService *self, GDataQuery *query, GCancellable *cancellable,
                                                GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                GDestroyNotify destroy_progress_user_data,
                                                GAsyncReadyCallback callback, gpointer user_data);

GDataContactsGroup *gdata_contacts_service_insert_group (GDataContactsService *self, GDataContactsGroup *group,
                                                         GCancellable *cancellable, GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_contacts_service_insert_group_async (GDataContactsService *self, GDataContactsGroup *group, GCancellable *cancellable,
                                                GAsyncReadyCallback callback, gpointer user_data);

G_END_DECLS

#endif /* !GDATA_CONTACTS_SERVICE_H */
