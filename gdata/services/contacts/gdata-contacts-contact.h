/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009, 2010, 2011 <philip@tecnocode.co.uk>
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

#ifndef GDATA_CONTACTS_CONTACT_H
#define GDATA_CONTACTS_CONTACT_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-entry.h>
#include <gdata/gd/gdata-gd-name.h>
#include <gdata/gd/gdata-gd-email-address.h>
#include <gdata/gd/gdata-gd-im-address.h>
#include <gdata/gd/gdata-gd-organization.h>
#include <gdata/gd/gdata-gd-phone-number.h>
#include <gdata/gd/gdata-gd-postal-address.h>
#include <gdata/gcontact/gdata-gcontact-calendar.h>
#include <gdata/gcontact/gdata-gcontact-event.h>
#include <gdata/gcontact/gdata-gcontact-external-id.h>
#include <gdata/gcontact/gdata-gcontact-jot.h>
#include <gdata/gcontact/gdata-gcontact-language.h>
#include <gdata/gcontact/gdata-gcontact-relation.h>
#include <gdata/gcontact/gdata-gcontact-website.h>

G_BEGIN_DECLS

/**
 * GDATA_CONTACTS_GENDER_MALE:
 *
 * The contact is male.
 *
 * Since: 0.7.0
 */
#define GDATA_CONTACTS_GENDER_MALE "male"

/**
 * GDATA_CONTACTS_GENDER_FEMALE:
 *
 * The contact is female.
 *
 * Since: 0.7.0
 */
#define GDATA_CONTACTS_GENDER_FEMALE "female"

/**
 * GDATA_CONTACTS_PRIORITY_LOW:
 *
 * The contact is of low importance.
 *
 * Since: 0.7.0
 */
#define GDATA_CONTACTS_PRIORITY_LOW "low"

/**
 * GDATA_CONTACTS_PRIORITY_NORMAL:
 *
 * The contact is of normal importance.
 *
 * Since: 0.7.0
 */
#define GDATA_CONTACTS_PRIORITY_NORMAL "normal"

/**
 * GDATA_CONTACTS_PRIORITY_HIGH:
 *
 * The contact is of high importance.
 *
 * Since: 0.7.0
 */
#define GDATA_CONTACTS_PRIORITY_HIGH "high"

/**
 * GDATA_CONTACTS_SENSITIVITY_CONFIDENTIAL:
 *
 * The contact's data is confidential.
 *
 * Since: 0.7.0
 */
#define GDATA_CONTACTS_SENSITIVITY_CONFIDENTIAL "confidential"

/**
 * GDATA_CONTACTS_SENSITIVITY_NORMAL:
 *
 * The contact's data is of normal sensitivity.
 *
 * Since: 0.7.0
 */
#define GDATA_CONTACTS_SENSITIVITY_NORMAL "normal"

/**
 * GDATA_CONTACTS_SENSITIVITY_PERSONAL:
 *
 * The contact's data is personal.
 *
 * Since: 0.7.0
 */
#define GDATA_CONTACTS_SENSITIVITY_PERSONAL "personal"

/**
 * GDATA_CONTACTS_SENSITIVITY_PRIVATE:
 *
 * The contact's data is private.
 *
 * Since: 0.7.0
 */
#define GDATA_CONTACTS_SENSITIVITY_PRIVATE "private"

#define GDATA_TYPE_CONTACTS_CONTACT		(gdata_contacts_contact_get_type ())
#define GDATA_CONTACTS_CONTACT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_CONTACTS_CONTACT, GDataContactsContact))
#define GDATA_CONTACTS_CONTACT_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_CONTACTS_CONTACT, GDataContactsContactClass))
#define GDATA_IS_CONTACTS_CONTACT(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_CONTACTS_CONTACT))
#define GDATA_IS_CONTACTS_CONTACT_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_CONTACTS_CONTACT))
#define GDATA_CONTACTS_CONTACT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_CONTACTS_CONTACT, GDataContactsContactClass))

typedef struct _GDataContactsContactPrivate	GDataContactsContactPrivate;

/**
 * GDataContactsContact:
 *
 * All the fields in the #GDataContactsContact structure are private and should never be accessed directly.
 *
 * Since: 0.2.0
 */
typedef struct {
	GDataEntry parent;
	GDataContactsContactPrivate *priv;
} GDataContactsContact;

/**
 * GDataContactsContactClass:
 *
 * All the fields in the #GDataContactsContactClass structure are private and should never be accessed directly.
 *
 * Since: 0.2.0
 */
typedef struct {
	/*< private >*/
	GDataEntryClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataContactsContactClass;

G_DEPRECATED
GType gdata_contacts_contact_get_type (void) G_GNUC_CONST;

G_DEPRECATED
GDataContactsContact *gdata_contacts_contact_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_DEPRECATED
gint64 gdata_contacts_contact_get_edited (GDataContactsContact *self);
G_DEPRECATED
gboolean gdata_contacts_contact_is_deleted (GDataContactsContact *self) G_GNUC_PURE;

G_DEPRECATED
GDataGDName *gdata_contacts_contact_get_name (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_set_name (GDataContactsContact *self, GDataGDName *name);

G_DEPRECATED
const gchar *gdata_contacts_contact_get_nickname (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_set_nickname (GDataContactsContact *self, const gchar *nickname);

G_DEPRECATED
const gchar *gdata_contacts_contact_get_file_as (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_set_file_as (GDataContactsContact *self, const gchar *file_as);

G_DEPRECATED
gboolean gdata_contacts_contact_get_birthday (GDataContactsContact *self, GDate *birthday) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_set_birthday (GDataContactsContact *self, GDate *birthday, gboolean birthday_has_year);

G_DEPRECATED
const gchar *gdata_contacts_contact_get_billing_information (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_set_billing_information (GDataContactsContact *self, const gchar *billing_information);

G_DEPRECATED
const gchar *gdata_contacts_contact_get_directory_server (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_set_directory_server (GDataContactsContact *self, const gchar *directory_server);

G_DEPRECATED
const gchar *gdata_contacts_contact_get_gender (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_set_gender (GDataContactsContact *self, const gchar *gender);

G_DEPRECATED
const gchar *gdata_contacts_contact_get_initials (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_set_initials (GDataContactsContact *self, const gchar *initials);

G_DEPRECATED
const gchar *gdata_contacts_contact_get_maiden_name (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_set_maiden_name (GDataContactsContact *self, const gchar *maiden_name);

G_DEPRECATED
const gchar *gdata_contacts_contact_get_mileage (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_set_mileage (GDataContactsContact *self, const gchar *mileage);

G_DEPRECATED
const gchar *gdata_contacts_contact_get_occupation (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_set_occupation (GDataContactsContact *self, const gchar *occupation);

G_DEPRECATED
const gchar *gdata_contacts_contact_get_priority (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_set_priority (GDataContactsContact *self, const gchar *priority);

G_DEPRECATED
const gchar *gdata_contacts_contact_get_sensitivity (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_set_sensitivity (GDataContactsContact *self, const gchar *sensitivity);

G_DEPRECATED
const gchar *gdata_contacts_contact_get_short_name (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_set_short_name (GDataContactsContact *self, const gchar *short_name);

G_DEPRECATED
const gchar *gdata_contacts_contact_get_subject (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_set_subject (GDataContactsContact *self, const gchar *subject);

G_DEPRECATED
void gdata_contacts_contact_add_email_address (GDataContactsContact *self, GDataGDEmailAddress *email_address);
G_DEPRECATED
GList *gdata_contacts_contact_get_email_addresses (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
GDataGDEmailAddress *gdata_contacts_contact_get_primary_email_address (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_remove_all_email_addresses (GDataContactsContact *self);

G_DEPRECATED
void gdata_contacts_contact_add_im_address (GDataContactsContact *self, GDataGDIMAddress *im_address);
G_DEPRECATED
GList *gdata_contacts_contact_get_im_addresses (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
GDataGDIMAddress *gdata_contacts_contact_get_primary_im_address (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_remove_all_im_addresses (GDataContactsContact *self);

G_DEPRECATED
void gdata_contacts_contact_add_phone_number (GDataContactsContact *self, GDataGDPhoneNumber *phone_number);
G_DEPRECATED
GList *gdata_contacts_contact_get_phone_numbers (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
GDataGDPhoneNumber *gdata_contacts_contact_get_primary_phone_number (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_remove_all_phone_numbers (GDataContactsContact *self);

G_DEPRECATED
void gdata_contacts_contact_add_postal_address (GDataContactsContact *self, GDataGDPostalAddress *postal_address);
G_DEPRECATED
GList *gdata_contacts_contact_get_postal_addresses (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
GDataGDPostalAddress *gdata_contacts_contact_get_primary_postal_address (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_remove_all_postal_addresses (GDataContactsContact *self);

G_DEPRECATED
void gdata_contacts_contact_add_organization (GDataContactsContact *self, GDataGDOrganization *organization);
G_DEPRECATED
GList *gdata_contacts_contact_get_organizations (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
GDataGDOrganization *gdata_contacts_contact_get_primary_organization (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_remove_all_organizations (GDataContactsContact *self);

G_DEPRECATED
void gdata_contacts_contact_add_jot (GDataContactsContact *self, GDataGContactJot *jot);
G_DEPRECATED
GList *gdata_contacts_contact_get_jots (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_remove_all_jots (GDataContactsContact *self);

G_DEPRECATED
void gdata_contacts_contact_add_relation (GDataContactsContact *self, GDataGContactRelation *relation);
G_DEPRECATED
GList *gdata_contacts_contact_get_relations (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_remove_all_relations (GDataContactsContact *self);

G_DEPRECATED
void gdata_contacts_contact_add_website (GDataContactsContact *self, GDataGContactWebsite *website);
G_DEPRECATED
GList *gdata_contacts_contact_get_websites (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
GDataGContactWebsite *gdata_contacts_contact_get_primary_website (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_remove_all_websites (GDataContactsContact *self);

G_DEPRECATED
void gdata_contacts_contact_add_event (GDataContactsContact *self, GDataGContactEvent *event);
G_DEPRECATED
GList *gdata_contacts_contact_get_events (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_remove_all_events (GDataContactsContact *self);

G_DEPRECATED
void gdata_contacts_contact_add_calendar (GDataContactsContact *self, GDataGContactCalendar *calendar);
G_DEPRECATED
GList *gdata_contacts_contact_get_calendars (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
GDataGContactCalendar *gdata_contacts_contact_get_primary_calendar (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_remove_all_calendars (GDataContactsContact *self);

G_DEPRECATED
void gdata_contacts_contact_add_external_id (GDataContactsContact *self, GDataGContactExternalID *external_id);
G_DEPRECATED
GList *gdata_contacts_contact_get_external_ids (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_remove_all_external_ids (GDataContactsContact *self);

G_DEPRECATED
void gdata_contacts_contact_add_hobby (GDataContactsContact *self, const gchar *hobby);
G_DEPRECATED
GList *gdata_contacts_contact_get_hobbies (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_remove_all_hobbies (GDataContactsContact *self);

G_DEPRECATED
void gdata_contacts_contact_add_language (GDataContactsContact *self, GDataGContactLanguage *language);
G_DEPRECATED
GList *gdata_contacts_contact_get_languages (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_remove_all_languages (GDataContactsContact *self);

G_DEPRECATED
const gchar *gdata_contacts_contact_get_extended_property (GDataContactsContact *self, const gchar *name) G_GNUC_PURE;
G_DEPRECATED
GHashTable *gdata_contacts_contact_get_extended_properties (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
gboolean gdata_contacts_contact_set_extended_property (GDataContactsContact *self, const gchar *name, const gchar *value);

G_DEPRECATED
const gchar *gdata_contacts_contact_get_user_defined_field (GDataContactsContact *self, const gchar *name) G_GNUC_PURE;
G_DEPRECATED
GHashTable *gdata_contacts_contact_get_user_defined_fields (GDataContactsContact *self) G_GNUC_PURE;
G_DEPRECATED
void gdata_contacts_contact_set_user_defined_field (GDataContactsContact *self, const gchar *name, const gchar *value);

G_DEPRECATED
void gdata_contacts_contact_add_group (GDataContactsContact *self, const gchar *href);
G_DEPRECATED
void gdata_contacts_contact_remove_group (GDataContactsContact *self, const gchar *href);
G_DEPRECATED
gboolean gdata_contacts_contact_is_group_deleted (GDataContactsContact *self, const gchar *href) G_GNUC_PURE;
G_DEPRECATED
GList *gdata_contacts_contact_get_groups (GDataContactsContact *self) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

#include <gdata/services/contacts/gdata-contacts-service.h>

G_DEPRECATED
const gchar *gdata_contacts_contact_get_photo_etag (GDataContactsContact *self) G_GNUC_PURE;

G_DEPRECATED
guint8 *gdata_contacts_contact_get_photo (GDataContactsContact *self, GDataContactsService *service, gsize *length, gchar **content_type,
                                          GCancellable *cancellable, GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
G_DEPRECATED
void gdata_contacts_contact_get_photo_async (GDataContactsContact *self, GDataContactsService *service, GCancellable *cancellable,
                                             GAsyncReadyCallback callback, gpointer user_data);
G_DEPRECATED
guint8 *gdata_contacts_contact_get_photo_finish (GDataContactsContact *self, GAsyncResult *async_result, gsize *length, gchar **content_type,
                                                 GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_DEPRECATED
gboolean gdata_contacts_contact_set_photo (GDataContactsContact *self, GDataContactsService *service, const guint8 *data, gsize length,
                                           const gchar *content_type, GCancellable *cancellable, GError **error);
G_DEPRECATED
void gdata_contacts_contact_set_photo_async (GDataContactsContact *self, GDataContactsService *service, const guint8 *data, gsize length,
                                             const gchar *content_type, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
G_DEPRECATED
gboolean gdata_contacts_contact_set_photo_finish (GDataContactsContact *self, GAsyncResult *async_result, GError **error);

G_END_DECLS

#endif /* !GDATA_CONTACTS_CONTACT_H */
