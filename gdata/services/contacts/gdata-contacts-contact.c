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

/**
 * SECTION:gdata-contacts-contact
 * @short_description: GData Contacts contact object
 * @stability: Stable
 * @include: gdata/services/contacts/gdata-contacts-contact.h
 *
 * #GDataContactsContact is a subclass of #GDataEntry to represent a contact from a Google address book.
 *
 * For more details of Google Contacts' GData API, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/2.0/reference.html">
 * online documentation</ulink>.
 *
 * In addition to all the standard properties available for a contact, #GDataContactsContact supports two kinds of additional property: extended
 * properties and user-defined fields. Extended properties, set with gdata_contacts_contact_set_extended_property() and retrieved with
 * gdata_contacts_contact_get_extended_property(), are provided as a method of storing client-specific data which shouldn't be seen  or be editable
 * by the user, such as IDs and cache times. User-defined fields, set with gdata_contacts_contact_set_user_defined_field() and retrieved with
 * gdata_contacts_contact_get_user_defined_field(), store fields defined by the user, and editable by them in the interface (both the interface of
 * the appliation using libgdata, and the Google Contacts web interface).
 *
 * <example>
 * 	<title>Getting a Contact's Photo</title>
 * 	<programlisting>
 *	GDataContactsService *service;
 *	GDataContactsContact *contact;
 *	guint8 *data;
 *	gchar *content_type = NULL;
 *	gsize length = 0;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service and return the contact whose photo we're getting. *<!-- -->/
 *	service = create_contacts_service ();
 *	contact = query_user_for_contact (service);
 *
 *	/<!-- -->* Get the photo. This should almost always be done asynchronously. *<!-- -->/
 *	data = gdata_contacts_contact_get_photo (contact, service, &length, &content_type, NULL, &error);
 *
 *	g_object_unref (contact);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error getting a contact's photo: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	/<!-- -->* Do something with the photo, stored in data, length and content_type. *<!-- -->/
 *
 *	g_free (content_type);
 *	g_free (data);
 * 	</programlisting>
 * </example>
 *
 * <example>
 * 	<title>Setting a Contact's Photo</title>
 * 	<programlisting>
 *	GDataContactsService *service;
 *	GDataContactsContact *contact;
 *	guint8 *data;
 *	gchar *content_type = NULL;
 *	gsize length = 0;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service and return the contact whose photo we're getting, as well as the details of the new photo. *<!-- -->/
 *	service = create_contacts_service ();
 *	contact = query_user_for_contact (service);
 *	data = query_user_for_new_photo (contact, &content_type, &length);
 *
 *	/<!-- -->* Set the photo. This should almost always be done asynchronously. To delete the photo, just pass NULL as the photo data. *<!-- -->/
 *	gdata_contacts_contact_set_photo (contact, service, data, length, content_type, NULL, &error);
 *
 *	g_free (data);
 *	g_free (content_type);
 *	g_object_unref (contact);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error setting a contact's photo: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 * 	</programlisting>
 * </example>
 *
 * <example>
 * 	<title>Updating a Contact's Details</title>
 * 	<programlisting>
 *	GDataContactsService *service;
 *	GDataContactsContact *contact, *updated_contact;
 *	GDataGDEmailAddress *email_address;
 *	GDataGDIMAddress *im_address;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service and return the contact whose details we're updating. *<!-- -->/
 *	service = create_contacts_service ();
 *	contact = query_user_for_contact (service);
 *
 *	/<!-- -->* Update the contact's details. We set their nickname to "Fat Tony", add a new e-mail address, and replace all their existing IM
 *	 * addresses with a single new one. *<!-- -->/
 *	gdata_contacts_contact_set_nickname (contact, "Fat Tony");
 *
 *	email_address = gdata_gd_email_address_new ("tony@gmail.com", GDATA_GD_EMAIL_ADDRESS_HOME, NULL, FALSE);
 *	gdata_contacts_contact_add_email_address (contact, email_address);
 *	g_object_unref (email_address);
 *
 *	gdata_contacts_contact_remove_all_im_addresses (contact);
 *	im_address = gdata_gd_im_address_new ("tony.work@gmail.com", GDATA_GD_IM_PROTOCOL_GOOGLE_TALK, GDATA_GD_IM_ADDRESS_WORK, NULL, TRUE);
 *	gdata_contacts_contact_add_im_address (contact, im_address);
 *	g_object_unref (im_address);
 *
 *	/<!-- -->* Send the updated contact to the server *<!-- -->/
 *	updated_contact = GDATA_CONTACTS_CONTACT (gdata_service_update_entry (GDATA_SERVICE (service), GDATA_ENTRY (contact), NULL, &error));
 *
 *	g_object_unref (contact);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error updating a contact's details: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	/<!-- -->* Do something with the updated contact, such as update them in the UI, or store their ID for future use. *<!-- -->/
 *
 *	g_object_unref (updated_contact);
 * 	</programlisting>
 * </example>
 *
 * Since: 0.2.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-contacts-contact.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "gdata-private.h"
#include "gdata-comparable.h"

/* The maximum number of extended properties the server allows us. See
 * http://code.google.com/apis/contacts/docs/2.0/reference.html#ProjectionsAndExtended.
 * When updating this, make sure to update the API documentation for gdata_contacts_contact_get_extended_property()
 * and gdata_contacts_contact_set_extended_property(). */
#define MAX_N_EXTENDED_PROPERTIES 10

static GObject *gdata_contacts_contact_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params);
static void gdata_contacts_contact_dispose (GObject *object);
static void gdata_contacts_contact_finalize (GObject *object);
static void gdata_contacts_contact_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_contacts_contact_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);
static gchar *get_entry_uri (const gchar *id) G_GNUC_WARN_UNUSED_RESULT;

struct _GDataContactsContactPrivate {
	gint64 edited;
	GDataGDName *name;
	GList *email_addresses; /* GDataGDEmailAddress */
	GList *im_addresses; /* GDataGDIMAddress */
	GList *phone_numbers; /* GDataGDPhoneNumber */
	GList *postal_addresses; /* GDataGDPostalAddress */
	GList *organizations; /* GDataGDOrganization */
	GHashTable *extended_properties;
	GHashTable *user_defined_fields;
	GHashTable *groups;
	gboolean deleted;
	gchar *photo_etag;
	GList *jots; /* GDataGContactJot */
	gchar *nickname;
	gchar *file_as;
	GDate birthday;
	gboolean birthday_has_year; /* contacts can choose to just give the month and day of their birth */
	GList *relations; /* GDataGContactRelation */
	GList *websites; /* GDataGContactWebsite */
	GList *events; /* GDataGContactEvent */
	GList *calendars; /* GDataGContactCalendar */
	GList *external_ids; /* GDataGContactExternalID */
	gchar *billing_information;
	gchar *directory_server;
	gchar *gender;
	gchar *initials;
	gchar *maiden_name;
	gchar *mileage;
	gchar *occupation;
	gchar *priority;
	gchar *sensitivity;
	gchar *short_name;
	gchar *subject;
	GList *hobbies; /* gchar* */
	GList *languages; /* GDataGContactLanguage */
};

enum {
	PROP_EDITED = 1,
	PROP_DELETED,
	PROP_NAME,
	PROP_NICKNAME,
	PROP_BIRTHDAY,
	PROP_BIRTHDAY_HAS_YEAR,
	PROP_BILLING_INFORMATION,
	PROP_DIRECTORY_SERVER,
	PROP_GENDER,
	PROP_INITIALS,
	PROP_MAIDEN_NAME,
	PROP_MILEAGE,
	PROP_OCCUPATION,
	PROP_PRIORITY,
	PROP_SENSITIVITY,
	PROP_SHORT_NAME,
	PROP_SUBJECT,
	PROP_PHOTO_ETAG,
	PROP_FILE_AS,
};

G_DEFINE_TYPE (GDataContactsContact, gdata_contacts_contact, GDATA_TYPE_ENTRY)

static void
gdata_contacts_contact_class_init (GDataContactsContactClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataContactsContactPrivate));

	gobject_class->constructor = gdata_contacts_contact_constructor;
	gobject_class->get_property = gdata_contacts_contact_get_property;
	gobject_class->set_property = gdata_contacts_contact_set_property;
	gobject_class->dispose = gdata_contacts_contact_dispose;
	gobject_class->finalize = gdata_contacts_contact_finalize;

	parsable_class->parse_xml = parse_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;

	entry_class->get_entry_uri = get_entry_uri;
	entry_class->kind_term = "http://schemas.google.com/contact/2008#contact";

	/**
	 * GDataContactsContact:edited:
	 *
	 * The last time the contact was edited. If the contact has not been edited yet, the content indicates the time it was created.
	 *
	 * For more information, see the <ulink type="http" url="http://www.atomenabled.org/developers/protocol/#appEdited">
	 * Atom Publishing Protocol specification</ulink>.
	 *
	 * Since: 0.2.0
	 */
	g_object_class_install_property (gobject_class, PROP_EDITED,
	                                 g_param_spec_int64 ("edited",
	                                                     "Edited", "The last time the contact was edited.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:deleted:
	 *
	 * Whether the entry has been deleted.
	 *
	 * Since: 0.2.0
	 */
	g_object_class_install_property (gobject_class, PROP_DELETED,
	                                 g_param_spec_boolean ("deleted",
	                                                       "Deleted", "Whether the entry has been deleted.",
	                                                       FALSE,
	                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:photo-etag:
	 *
	 * The ETag of the contact's photo, if the contact has a photo; %NULL otherwise.
	 *
	 * Since: 0.9.0
	 */
	g_object_class_install_property (gobject_class, PROP_PHOTO_ETAG,
	                                 g_param_spec_string ("photo-etag",
	                                                       "Photo ETag", "The ETag of the contact's photo.",
	                                                       NULL,
	                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:name:
	 *
	 * The contact's name in a structured representation.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_NAME,
	                                 g_param_spec_object ("name",
	                                                      "Name", "The contact's name in a structured representation.",
	                                                      GDATA_TYPE_GD_NAME,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:nickname:
	 *
	 * The contact's chosen nickname.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_NICKNAME,
	                                 g_param_spec_string ("nickname",
	                                                      "Nickname", "The contact's chosen nickname.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:file-as:
	 *
	 * The name to file the contact under for sorting purposes.
	 *
	 * Since: 0.11.0
	 */
	g_object_class_install_property (gobject_class, PROP_FILE_AS,
	                                 g_param_spec_string ("file-as",
	                                                      "File As", "The name to file the contact under for sorting purposes.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:birthday:
	 *
	 * The contact's birthday.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_BIRTHDAY,
	                                 g_param_spec_boxed ("birthday",
	                                                     "Birthday", "The contact's birthday.",
	                                                     G_TYPE_DATE,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:birthday-has-year:
	 *
	 * Whether the contact's birthday includes their year of birth.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_BIRTHDAY_HAS_YEAR,
	                                 g_param_spec_boolean ("birthday-has-year",
	                                                       "Birthday has year?", "Whether the contact's birthday includes their year of birth.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:billing-information:
	 *
	 * Billing information for the contact, such as their billing name and address.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_BILLING_INFORMATION,
	                                 g_param_spec_string ("billing-information",
	                                                      "Billing information", "Billing information for the contact.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:directory-server:
	 *
	 * The name or address of a directory server associated with the contact.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_DIRECTORY_SERVER,
	                                 g_param_spec_string ("directory-server",
	                                                      "Directory server", "The name or address of an associated directory server.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:gender:
	 *
	 * The gender of the contact. For example: %GDATA_CONTACTS_GENDER_MALE or %GDATA_CONTACTS_GENDER_FEMALE.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_GENDER,
	                                 g_param_spec_string ("gender",
	                                                      "Gender", "The gender of the contact.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:initials:
	 *
	 * The initials of the contact.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_INITIALS,
	                                 g_param_spec_string ("initials",
	                                                      "Initials", "The initials of the contact.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:maiden-name:
	 *
	 * The maiden name of the contact.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_MAIDEN_NAME,
	                                 g_param_spec_string ("maiden-name",
	                                                      "Maiden name", "The maiden name of the contact.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:mileage:
	 *
	 * A mileage associated with the contact, such as one for reimbursement purposes. It can be in any format.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_MILEAGE,
	                                 g_param_spec_string ("mileage",
	                                                      "Mileage", "A mileage associated with the contact.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:occupation:
	 *
	 * The contact's occupation.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_OCCUPATION,
	                                 g_param_spec_string ("occupation",
	                                                      "Occupation", "The contact's occupation.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:priority:
	 *
	 * The contact's importance. For example: %GDATA_CONTACTS_PRIORITY_NORMAL or %GDATA_CONTACTS_PRIORITY_HIGH.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_PRIORITY,
	                                 g_param_spec_string ("priority",
	                                                      "Priority", "The contact's importance.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:sensitivity:
	 *
	 * The sensitivity of the contact's data. For example: %GDATA_CONTACTS_SENSITIVITY_NORMAL or %GDATA_CONTACTS_SENSITIVITY_PRIVATE.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_SENSITIVITY,
	                                 g_param_spec_string ("sensitivity",
	                                                      "Sensitivity", "The sensitivity of the contact's data.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:short-name:
	 *
	 * A short name for the contact. This should be used for contracted versions of the contact's actual name,
	 * whereas #GDataContactsContact:nickname should be used for nicknames.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_SHORT_NAME,
	                                 g_param_spec_string ("short-name",
	                                                      "Short name", "A short name for the contact.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsContact:subject:
	 *
	 * The subject of the contact. (i.e. The contact's relevance to the address book.)
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_SUBJECT,
	                                 g_param_spec_string ("subject",
	                                                      "Subject", "The subject of the contact.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void notify_full_name_cb (GObject *gobject, GParamSpec *pspec, GDataContactsContact *self);

static void
notify_title_cb (GObject *gobject, GParamSpec *pspec, GDataContactsContact *self)
{
	/* Update GDataGDName:full-name */
	g_signal_handlers_block_by_func (self->priv->name, notify_full_name_cb, self);
	gdata_gd_name_set_full_name (self->priv->name, gdata_entry_get_title (GDATA_ENTRY (self)));
	g_signal_handlers_unblock_by_func (self->priv->name, notify_full_name_cb, self);
}

static void
notify_full_name_cb (GObject *gobject, GParamSpec *pspec, GDataContactsContact *self)
{
	/* Update GDataEntry:title */
	g_signal_handlers_block_by_func (self, notify_title_cb, self);
	gdata_entry_set_title (GDATA_ENTRY (self), gdata_gd_name_get_full_name (self->priv->name));
	g_signal_handlers_unblock_by_func (self, notify_title_cb, self);
}

static void
gdata_contacts_contact_init (GDataContactsContact *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_CONTACTS_CONTACT, GDataContactsContactPrivate);
	self->priv->extended_properties = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	self->priv->user_defined_fields = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	self->priv->groups = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	self->priv->edited = -1;

	/* Create a default name, so the name's properties can be set for a blank contact */
	self->priv->name = gdata_gd_name_new (NULL, NULL);

	/* Listen to change notifications for the entry's title, since it's linked to GDataGDName:full-name */
	g_signal_connect (self, "notify::title", (GCallback) notify_title_cb, self);
	g_signal_connect (self->priv->name, "notify::full-name", (GCallback) notify_full_name_cb, self);

	/* Initialise the contact's birthday to a sane but invalid date */
	g_date_clear (&(self->priv->birthday), 1);
}

static GObject *
gdata_contacts_contact_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
	GObject *object;
	guint i;

	/* Find the "id" property and ensure it's sane */
	for (i = 0; i < n_construct_params; i++) {
		GParamSpec *pspec = construct_params[i].pspec;
		GValue *value = construct_params[i].value;

		if (strcmp (g_param_spec_get_name (pspec), "id") == 0) {
			gchar *base, *id;

			id = g_value_dup_string (value);

			/* Fix the ID to refer to the full projection, rather than the base projection. */
			if (id != NULL) {
				base = strstr (id, "/base/");
				if (base != NULL)
					memcpy (base, "/full/", 6);
			}

			g_value_take_string (value, id);

			break;
		}
	}

	/* Chain up to the parent class */
	object = G_OBJECT_CLASS (gdata_contacts_contact_parent_class)->constructor (type, n_construct_params, construct_params);

	if (_gdata_parsable_is_constructed_from_xml (GDATA_PARSABLE (object)) == FALSE) {
		GDataContactsContactPrivate *priv = GDATA_CONTACTS_CONTACT (object)->priv;
		GTimeVal time_val;

		/* Set the edited property to the current time (creation time). We don't do this in *_init() since that would cause
		 * setting it from parse_xml() to fail (duplicate element). */
		g_get_current_time (&time_val);
		priv->edited = time_val.tv_sec;
	}

	return object;
}

static void
gdata_contacts_contact_dispose (GObject *object)
{
	GDataContactsContact *self = GDATA_CONTACTS_CONTACT (object);

	if (self->priv->name != NULL)
		g_object_unref (self->priv->name);
	self->priv->name = NULL;

	gdata_contacts_contact_remove_all_organizations (self);
	gdata_contacts_contact_remove_all_email_addresses (self);
	gdata_contacts_contact_remove_all_im_addresses (self);
	gdata_contacts_contact_remove_all_postal_addresses (self);
	gdata_contacts_contact_remove_all_phone_numbers (self);
	gdata_contacts_contact_remove_all_jots (self);
	gdata_contacts_contact_remove_all_relations (self);
	gdata_contacts_contact_remove_all_websites (self);
	gdata_contacts_contact_remove_all_events (self);
	gdata_contacts_contact_remove_all_calendars (self);
	gdata_contacts_contact_remove_all_external_ids (self);
	gdata_contacts_contact_remove_all_languages (self);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_contacts_contact_parent_class)->dispose (object);
}

static void
gdata_contacts_contact_finalize (GObject *object)
{
	GDataContactsContactPrivate *priv = GDATA_CONTACTS_CONTACT (object)->priv;

	g_hash_table_destroy (priv->extended_properties);
	g_hash_table_destroy (priv->user_defined_fields);
	g_hash_table_destroy (priv->groups);
	g_free (priv->photo_etag);
	g_free (priv->nickname);
	g_free (priv->file_as);
	g_free (priv->billing_information);
	g_free (priv->directory_server);
	g_free (priv->gender);
	g_free (priv->initials);
	g_free (priv->maiden_name);
	g_free (priv->mileage);
	g_free (priv->occupation);
	g_free (priv->priority);
	g_free (priv->sensitivity);
	g_free (priv->short_name);
	g_free (priv->subject);

	g_list_free_full (priv->hobbies, g_free);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_contacts_contact_parent_class)->finalize (object);
}

static void
gdata_contacts_contact_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataContactsContactPrivate *priv = GDATA_CONTACTS_CONTACT (object)->priv;

	switch (property_id) {
		case PROP_EDITED:
			g_value_set_int64 (value, priv->edited);
			break;
		case PROP_DELETED:
			g_value_set_boolean (value, priv->deleted);
			break;
		case PROP_PHOTO_ETAG:
			g_value_set_string (value, priv->photo_etag);
			break;
		case PROP_NAME:
			g_value_set_object (value, priv->name);
			break;
		case PROP_NICKNAME:
			g_value_set_string (value, priv->nickname);
			break;
		case PROP_FILE_AS:
			g_value_set_string (value, priv->file_as);
			break;
		case PROP_BIRTHDAY:
			g_value_set_boxed (value, &(priv->birthday));
			break;
		case PROP_BIRTHDAY_HAS_YEAR:
			g_value_set_boolean (value, priv->birthday_has_year);
			break;
		case PROP_BILLING_INFORMATION:
			g_value_set_string (value, priv->billing_information);
			break;
		case PROP_DIRECTORY_SERVER:
			g_value_set_string (value, priv->directory_server);
			break;
		case PROP_GENDER:
			g_value_set_string (value, priv->gender);
			break;
		case PROP_INITIALS:
			g_value_set_string (value, priv->initials);
			break;
		case PROP_MAIDEN_NAME:
			g_value_set_string (value, priv->maiden_name);
			break;
		case PROP_MILEAGE:
			g_value_set_string (value, priv->mileage);
			break;
		case PROP_OCCUPATION:
			g_value_set_string (value, priv->occupation);
			break;
		case PROP_PRIORITY:
			g_value_set_string (value, priv->priority);
			break;
		case PROP_SENSITIVITY:
			g_value_set_string (value, priv->sensitivity);
			break;
		case PROP_SHORT_NAME:
			g_value_set_string (value, priv->short_name);
			break;
		case PROP_SUBJECT:
			g_value_set_string (value, priv->subject);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_contacts_contact_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataContactsContact *self = GDATA_CONTACTS_CONTACT (object);

	switch (property_id) {
		case PROP_NAME:
			gdata_contacts_contact_set_name (self, g_value_get_object (value));
			break;
		case PROP_NICKNAME:
			gdata_contacts_contact_set_nickname (self, g_value_get_string (value));
			break;
		case PROP_FILE_AS:
			gdata_contacts_contact_set_file_as (self, g_value_get_string (value));
			break;
		case PROP_BIRTHDAY:
			gdata_contacts_contact_set_birthday (self, g_value_get_boxed (value), self->priv->birthday_has_year);
			break;
		case PROP_BIRTHDAY_HAS_YEAR:
			gdata_contacts_contact_set_birthday (self, &(self->priv->birthday), g_value_get_boolean (value));
			break;
		case PROP_BILLING_INFORMATION:
			gdata_contacts_contact_set_billing_information (self, g_value_get_string (value));
			break;
		case PROP_DIRECTORY_SERVER:
			gdata_contacts_contact_set_directory_server (self, g_value_get_string (value));
			break;
		case PROP_GENDER:
			gdata_contacts_contact_set_gender (self, g_value_get_string (value));
			break;
		case PROP_INITIALS:
			gdata_contacts_contact_set_initials (self, g_value_get_string (value));
			break;
		case PROP_MAIDEN_NAME:
			gdata_contacts_contact_set_maiden_name (self, g_value_get_string (value));
			break;
		case PROP_MILEAGE:
			gdata_contacts_contact_set_mileage (self, g_value_get_string (value));
			break;
		case PROP_OCCUPATION:
			gdata_contacts_contact_set_occupation (self, g_value_get_string (value));
			break;
		case PROP_PRIORITY:
			gdata_contacts_contact_set_priority (self, g_value_get_string (value));
			break;
		case PROP_SENSITIVITY:
			gdata_contacts_contact_set_sensitivity (self, g_value_get_string (value));
			break;
		case PROP_SHORT_NAME:
			gdata_contacts_contact_set_short_name (self, g_value_get_string (value));
			break;
		case PROP_SUBJECT:
			gdata_contacts_contact_set_subject (self, g_value_get_string (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	gboolean success;
	GDataContactsContact *self = GDATA_CONTACTS_CONTACT (parsable);

	if (gdata_parser_is_namespace (node, "http://www.w3.org/2007/app") == TRUE &&
	    gdata_parser_int64_time_from_element (node, "edited", P_REQUIRED | P_NO_DUPES, &(self->priv->edited), &success, error) == TRUE) {
		return success;
	} else if (gdata_parser_is_namespace (node, "http://www.w3.org/2005/Atom") == TRUE && xmlStrcmp (node->name, (xmlChar*) "id") == 0) {
		/* We have to override <id> parsing to fix the projection. Modify it in-place so that the parser in GDataEntry will pick up
		 * the changes. This fixes bugs caused by referring to contacts by the base projection, rather than the full projection;
		 * such as http://code.google.com/p/gdata-issues/issues/detail?id=2129. */
		gchar *base;
		gchar *id = (gchar*) xmlNodeListGetString (doc, node->children, TRUE);

		if (id != NULL) {
			base = strstr (id, "/base/");
			if (base != NULL) {
				memcpy (base, "/full/", 6);
				xmlNodeSetContent (node, (xmlChar*) id);
			}
		}

		xmlFree (id);

		return GDATA_PARSABLE_CLASS (gdata_contacts_contact_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	} else if (gdata_parser_is_namespace (node, "http://schemas.google.com/g/2005") == TRUE) {
		if (gdata_parser_object_from_element_setter (node, "im", P_REQUIRED, GDATA_TYPE_GD_IM_ADDRESS,
		                                             gdata_contacts_contact_add_im_address, self, &success, error) == TRUE ||
		    gdata_parser_object_from_element_setter (node, "phoneNumber", P_REQUIRED, GDATA_TYPE_GD_PHONE_NUMBER,
		                                             gdata_contacts_contact_add_phone_number, self, &success, error) == TRUE ||
		    gdata_parser_object_from_element_setter (node, "structuredPostalAddress", P_REQUIRED, GDATA_TYPE_GD_POSTAL_ADDRESS,
		                                             gdata_contacts_contact_add_postal_address, self, &success, error) == TRUE ||
		    gdata_parser_object_from_element_setter (node, "organization", P_REQUIRED, GDATA_TYPE_GD_ORGANIZATION,
		                                             gdata_contacts_contact_add_organization, self, &success, error) == TRUE ||
		    gdata_parser_object_from_element (node, "name", P_REQUIRED, GDATA_TYPE_GD_NAME, &(self->priv->name), &success, error) == TRUE) {
			return success;
		} else if (xmlStrcmp (node->name, (xmlChar*) "email") == 0) {
			/* gd:email */
			GDataParsable *_parsable;
			xmlChar *address;

			/* Check its address attribute is non-empty. Empty address attributes are apparently allowed, and make the
			 * gd:email element a no-op. See: https://bugzilla.gnome.org/show_bug.cgi?id=734863 */
			address = xmlGetProp (node, (xmlChar *) "address");
			if (address == NULL) {
				return gdata_parser_error_required_property_missing (node, "address", error);
			} else if (*address == '\0') {
				xmlFree (address);
				success = TRUE;
				return TRUE;
			}

			xmlFree (address);

			/* Parse the e-mail address. */
			_parsable = _gdata_parsable_new_from_xml_node (GDATA_TYPE_GD_EMAIL_ADDRESS, node->doc, node, NULL, error);
			if (_parsable == NULL) {
				/* The error has already been set by _gdata_parsable_new_from_xml_node() */
				success = FALSE;
				return TRUE;
			}

			/* Success! */
			gdata_contacts_contact_add_email_address (self, GDATA_GD_EMAIL_ADDRESS (_parsable));
			g_object_unref (_parsable);
			success = TRUE;

			return TRUE;
		} else if (xmlStrcmp (node->name, (xmlChar*) "extendedProperty") == 0) {
			/* gd:extendedProperty */
			xmlChar *name, *value;
			xmlBuffer *buffer = NULL;

			name = xmlGetProp (node, (xmlChar*) "name");
			if (name == NULL)
				return gdata_parser_error_required_property_missing (node, "name", error);

			/* Get either the value property, or the element's content */
			value = xmlGetProp (node, (xmlChar*) "value");
			if (value == NULL) {
				xmlNode *child_node;

				/* Use the element's content instead (arbitrary XML) */
				buffer = xmlBufferCreate ();
				for (child_node = node->children; child_node != NULL; child_node = child_node->next)
					xmlNodeDump (buffer, doc, child_node, 0, 0);
				value = (xmlChar*) xmlBufferContent (buffer);
			}

			gdata_contacts_contact_set_extended_property (self, (gchar*) name, (gchar*) value);

			xmlFree (name);
			if (buffer != NULL)
				xmlBufferFree (buffer);
			else
				xmlFree (value);
		} else if (xmlStrcmp (node->name, (xmlChar*) "deleted") == 0) {
			/* gd:deleted */
			self->priv->deleted = TRUE;
		} else {
			return GDATA_PARSABLE_CLASS (gdata_contacts_contact_parent_class)->parse_xml (parsable, doc, node, user_data, error);
		}
	} else if (gdata_parser_is_namespace (node, "http://schemas.google.com/contact/2008") == TRUE) {
		if (gdata_parser_object_from_element_setter (node, "jot", P_REQUIRED, GDATA_TYPE_GCONTACT_JOT,
		                                             gdata_contacts_contact_add_jot, self, &success, error) == TRUE ||
		    gdata_parser_object_from_element_setter (node, "relation", P_REQUIRED, GDATA_TYPE_GCONTACT_RELATION,
		                                             gdata_contacts_contact_add_relation, self, &success, error) == TRUE ||
		    gdata_parser_object_from_element_setter (node, "event", P_REQUIRED, GDATA_TYPE_GCONTACT_EVENT,
		                                             gdata_contacts_contact_add_event, self, &success, error) == TRUE ||
		    gdata_parser_object_from_element_setter (node, "website", P_IGNORE_ERROR, GDATA_TYPE_GCONTACT_WEBSITE,
		                                             gdata_contacts_contact_add_website, self, &success, error) == TRUE ||
		    gdata_parser_object_from_element_setter (node, "calendarLink", P_REQUIRED, GDATA_TYPE_GCONTACT_CALENDAR,
		                                             gdata_contacts_contact_add_calendar, self, &success, error) == TRUE ||
		    gdata_parser_object_from_element_setter (node, "externalId", P_REQUIRED, GDATA_TYPE_GCONTACT_EXTERNAL_ID,
		                                             gdata_contacts_contact_add_external_id, self, &success, error) == TRUE ||
		    gdata_parser_object_from_element_setter (node, "language", P_REQUIRED, GDATA_TYPE_GCONTACT_LANGUAGE,
		                                             gdata_contacts_contact_add_language, self, &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "nickname", P_REQUIRED | P_NO_DUPES, &(self->priv->nickname), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "fileAs", P_REQUIRED | P_NO_DUPES, &(self->priv->file_as), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "billingInformation", P_REQUIRED | P_NO_DUPES | P_NON_EMPTY,
		                                      &(self->priv->billing_information), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "directoryServer", P_REQUIRED | P_NO_DUPES | P_NON_EMPTY,
		                                      &(self->priv->directory_server), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "initials", P_REQUIRED | P_NO_DUPES, &(self->priv->initials), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "maidenName", P_REQUIRED | P_NO_DUPES,
		                                      &(self->priv->maiden_name), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "mileage", P_REQUIRED | P_NO_DUPES, &(self->priv->mileage), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "occupation", P_REQUIRED | P_NO_DUPES,
		                                      &(self->priv->occupation), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "shortName", P_REQUIRED | P_NO_DUPES,
		                                      &(self->priv->short_name), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "subject", P_REQUIRED | P_NO_DUPES, &(self->priv->subject), &success, error) == TRUE) {
			return success;
		} else if (xmlStrcmp (node->name, (xmlChar*) "gender") == 0) {
			/* gContact:gender */
			xmlChar *value;

			if (self->priv->gender != NULL)
				return gdata_parser_error_duplicate_element (node, error);

			value = xmlGetProp (node, (xmlChar*) "value");
			if (value == NULL || *value == '\0') {
				xmlFree (value);
				return gdata_parser_error_required_content_missing (node, error);
			}

			self->priv->gender = (gchar*) value;
		} else if (xmlStrcmp (node->name, (xmlChar*) "hobby") == 0) {
			/* gContact:hobby */
			xmlChar *hobby;

			hobby = xmlNodeListGetString (doc, node->children, TRUE);
			if (hobby == NULL || *hobby == '\0') {
				xmlFree (hobby);
				return gdata_parser_error_required_content_missing (node, error);
			}

			gdata_contacts_contact_add_hobby (self, (gchar*) hobby);
			xmlFree (hobby);
		} else if (xmlStrcmp (node->name, (xmlChar*) "userDefinedField") == 0) {
			/* gContact:userDefinedField */
			xmlChar *name, *value;

			/* Note that while we require the property to be present, we don't require it to be non-empty. See bgo#648058 */
			name = xmlGetProp (node, (xmlChar*) "key");
			if (name == NULL) {
				return gdata_parser_error_required_property_missing (node, "key", error);
			}

			/* Get either the value property, or the element's content */
			value = xmlGetProp (node, (xmlChar*) "value");
			if (value == NULL) {
				xmlFree (name);
				return gdata_parser_error_required_property_missing (node, "value", error);
			}

			gdata_contacts_contact_set_user_defined_field (self, (gchar*) name, (gchar*) value);

			xmlFree (name);
			xmlFree (value);
		} else if (xmlStrcmp (node->name, (xmlChar*) "priority") == 0) {
			/* gContact:priority */
			xmlChar *rel;

			if (self->priv->priority != NULL)
				return gdata_parser_error_duplicate_element (node, error);

			rel = xmlGetProp (node, (xmlChar*) "rel");
			if (rel == NULL || *rel == '\0') {
				xmlFree (rel);
				return gdata_parser_error_required_content_missing (node, error);
			}

			self->priv->priority = (gchar*) rel;
		} else if (xmlStrcmp (node->name, (xmlChar*) "sensitivity") == 0) {
			/* gContact:sensitivity */
			xmlChar *rel;

			if (self->priv->sensitivity != NULL)
				return gdata_parser_error_duplicate_element (node, error);

			rel = xmlGetProp (node, (xmlChar*) "rel");
			if (rel == NULL || *rel == '\0') {
				xmlFree (rel);
				return gdata_parser_error_required_content_missing (node, error);
			}

			self->priv->sensitivity = (gchar*) rel;
		} else if (xmlStrcmp (node->name, (xmlChar*) "groupMembershipInfo") == 0) {
			/* gContact:groupMembershipInfo */
			xmlChar *href;
			gboolean deleted_bool;

			href = xmlGetProp (node, (xmlChar*) "href");
			if (href == NULL)
				return gdata_parser_error_required_property_missing (node, "href", error);

			/* Has it been deleted? */
			if (gdata_parser_boolean_from_property (node, "deleted", &deleted_bool, 0, error) == FALSE) {
				xmlFree (href);
				return FALSE;
			}

			/* Insert it into the hash table */
			g_hash_table_insert (self->priv->groups, (gchar*) href, GUINT_TO_POINTER (deleted_bool));
		} else if (xmlStrcmp (node->name, (xmlChar*) "birthday") == 0) {
			/* gContact:birthday */
			xmlChar *birthday;
			guint length = 0, year = 666, month, day;

			if (g_date_valid (&(self->priv->birthday)) == TRUE)
				return gdata_parser_error_duplicate_element (node, error);

			birthday = xmlGetProp (node, (xmlChar*) "when");
			if (birthday == NULL)
				return gdata_parser_error_required_property_missing (node, "when", error);
			length = strlen ((char*) birthday);

			/* Try parsing the two possible formats: YYYY-MM-DD and --MM-DD */
			if (((length == 10 && sscanf ((char*) birthday, "%4u-%2u-%2u", &year, &month, &day) == 3) ||
			     (length == 7 && sscanf ((char*) birthday, "--%2u-%2u", &month, &day) == 2)) &&
			    g_date_valid_dmy (day, month, year) == TRUE) {
				/* Store the values in the GDate */
				g_date_set_dmy (&(self->priv->birthday), day, month, year);
				self->priv->birthday_has_year = (length == 10) ? TRUE : FALSE;
				xmlFree (birthday);
			} else {
				/* Parsing failed */
				gdata_parser_error_not_iso8601_format (node, (gchar*) birthday, error);
				xmlFree (birthday);
				return FALSE;
			}
		} else {
			return GDATA_PARSABLE_CLASS (gdata_contacts_contact_parent_class)->parse_xml (parsable, doc, node, user_data, error);
		}
	} else {
		/* If we haven't yet found a photo, check to see if it's a photo <link> element */
		if (self->priv->photo_etag == NULL && xmlStrcmp (node->name, (xmlChar*) "link") == 0) {
			xmlChar *rel = xmlGetProp (node, (xmlChar*) "rel");
			if (xmlStrcmp (rel, (xmlChar*) "http://schemas.google.com/contacts/2008/rel#photo") == 0) {
				/* It's the photo link (http://code.google.com/apis/contacts/docs/2.0/reference.html#Photos), whose ETag we should
				 * note down, then pass onto the parent class to parse properly */
				self->priv->photo_etag = (gchar*) xmlGetProp (node, (xmlChar*) "etag");
			}
			xmlFree (rel);
		}

		return GDATA_PARSABLE_CLASS (gdata_contacts_contact_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}

	return TRUE;
}

static void
get_child_xml (GList *list, GString *xml_string)
{
	GList *i;

	for (i = list; i != NULL; i = i->next)
		_gdata_parsable_get_xml (GDATA_PARSABLE (i->data), xml_string, FALSE);
}

static void
get_extended_property_xml_cb (const gchar *name, const gchar *value, GString *xml_string)
{
	/* Note that the value *isn't* escaped (see http://code.google.com/apis/gdata/docs/2.0/elements.html#gdExtendedProperty) */
	gdata_parser_string_append_escaped (xml_string, "<gd:extendedProperty name='", name, "'>");
	g_string_append_printf (xml_string, "%s</gd:extendedProperty>", value);
}

static void
get_user_defined_field_xml_cb (const gchar *name, const gchar *value, GString *xml_string)
{
	gdata_parser_string_append_escaped (xml_string, "<gContact:userDefinedField key='", name, "' ");
	gdata_parser_string_append_escaped (xml_string, "value='", value, "'/>");
}

static void
get_group_xml_cb (const gchar *href, gpointer deleted, GString *xml_string)
{
	gchar *full_pos, *uri = g_strdup (href);

	/* The service API sometimes stubbornly insists on using the "full" view instead of the "base" view, which we have
	 * to fix, or it complains about an invalid group ID. */
	full_pos = strstr (uri, "/full/");
	if (full_pos != NULL)
		memcpy ((char*) full_pos, "/base/", 6);

	gdata_parser_string_append_escaped (xml_string, "<gContact:groupMembershipInfo href='", uri, "'/>");

	g_free (uri);
}

static void
get_hobby_xml_cb (const gchar *hobby, GString *xml_string)
{
	gdata_parser_string_append_escaped (xml_string, "<gContact:hobby>", hobby, "</gContact:hobby>");
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataContactsContactPrivate *priv = GDATA_CONTACTS_CONTACT (parsable)->priv;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_contacts_contact_parent_class)->get_xml (parsable, xml_string);

	/* Name */
	_gdata_parsable_get_xml (GDATA_PARSABLE (priv->name), xml_string, FALSE);

	/* Lists of stuff */
	get_child_xml (priv->email_addresses, xml_string);
	get_child_xml (priv->im_addresses, xml_string);
	get_child_xml (priv->phone_numbers, xml_string);
	get_child_xml (priv->postal_addresses, xml_string);
	get_child_xml (priv->organizations, xml_string);
	get_child_xml (priv->jots, xml_string);
	get_child_xml (priv->relations, xml_string);
	get_child_xml (priv->websites, xml_string);
	get_child_xml (priv->events, xml_string);
	get_child_xml (priv->calendars, xml_string);
	get_child_xml (priv->external_ids, xml_string);
	get_child_xml (priv->languages, xml_string);

	/* Extended properties */
	g_hash_table_foreach (priv->extended_properties, (GHFunc) get_extended_property_xml_cb, xml_string);

	/* User defined fields */
	g_hash_table_foreach (priv->user_defined_fields, (GHFunc) get_user_defined_field_xml_cb, xml_string);

	/* Group membership info */
	g_hash_table_foreach (priv->groups, (GHFunc) get_group_xml_cb, xml_string);

	/* Hobbies */
	g_list_foreach (priv->hobbies, (GFunc) get_hobby_xml_cb, xml_string);

	/* gContact:nickname */
	if (priv->nickname != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gContact:nickname>", priv->nickname, "</gContact:nickname>");

	/* gContact:fileAs */
	if (priv->file_as != NULL) {
		gdata_parser_string_append_escaped (xml_string, "<gContact:fileAs>", priv->file_as, "</gContact:fileAs>");
	}

	/* gContact:birthday */
	if (g_date_valid (&(priv->birthday)) == TRUE) {
		if (priv->birthday_has_year == TRUE) {
			g_string_append_printf (xml_string, "<gContact:birthday when='%04u-%02u-%02u'/>",
			                        g_date_get_year (&(priv->birthday)),
			                        g_date_get_month (&(priv->birthday)),
			                        g_date_get_day (&(priv->birthday)));
		} else {
			g_string_append_printf (xml_string, "<gContact:birthday when='--%02u-%02u'/>",
			                        g_date_get_month (&(priv->birthday)),
			                        g_date_get_day (&(priv->birthday)));
		}
	}

	/* gContact:billingInformation */
	if (priv->billing_information != NULL) {
		gdata_parser_string_append_escaped (xml_string,
		                                    "<gContact:billingInformation>", priv->billing_information, "</gContact:billingInformation>");
	}

	/* gContact:directoryServer */
	if (priv->directory_server != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gContact:directoryServer>", priv->directory_server, "</gContact:directoryServer>");

	/* gContact:gender */
	if (priv->gender != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gContact:gender value='", priv->gender, "'/>");

	/* gContact:initials */
	if (priv->initials != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gContact:initials>", priv->initials, "</gContact:initials>");

	/* gContact:maidenName */
	if (priv->maiden_name != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gContact:maidenName>", priv->maiden_name, "</gContact:maidenName>");

	/* gContact:mileage */
	if (priv->mileage != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gContact:mileage>", priv->mileage, "</gContact:mileage>");

	/* gContact:occupation */
	if (priv->occupation != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gContact:occupation>", priv->occupation, "</gContact:occupation>");

	/* gContact:priority */
	if (priv->priority != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gContact:priority rel='", priv->priority, "'/>");

	/* gContact:sensitivity */
	if (priv->sensitivity != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gContact:sensitivity rel='", priv->sensitivity, "'/>");

	/* gContact:shortName */
	if (priv->short_name != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gContact:shortName>", priv->short_name, "</gContact:shortName>");

	/* gContact:subject */
	if (priv->subject != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gContact:subject>", priv->subject, "</gContact:subject>");

	/* TODO:
	 * - Finish supporting all tags
	 */
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_contacts_contact_parent_class)->get_namespaces (parsable, namespaces);

	g_hash_table_insert (namespaces, (gchar*) "gd", (gchar*) "http://schemas.google.com/g/2005");
	g_hash_table_insert (namespaces, (gchar*) "gContact", (gchar*) "http://schemas.google.com/contact/2008");
	g_hash_table_insert (namespaces, (gchar*) "app", (gchar*) "http://www.w3.org/2007/app");
}

static gchar *
get_entry_uri (const gchar *id)
{
	const gchar *base_pos;
	gchar *uri;

	/* Ensure it uses the HTTPS protocol */
	if (g_str_has_prefix (id, "http://") == TRUE) {
		guint id_length = strlen (id);

		uri = g_malloc (id_length + 2);
		strcpy (uri, "https://");
		strcpy (uri + strlen ("https://"), id + strlen ("http://"));
	} else {
		uri = g_strdup (id);
	}

	/* The service API sometimes stubbornly insists on using the "base" view instead of the "full" view, which we have
	 * to fix, or our extended attributes are never visible */
	base_pos = strstr (uri, "/base/");
	if (base_pos != NULL)
		memcpy ((char*) base_pos, "/full/", 6);

	return uri;
}

/**
 * gdata_contacts_contact_new:
 * @id: (allow-none): the contact's ID, or %NULL
 *
 * Creates a new #GDataContactsContact with the given ID and default properties.
 *
 * Return value: a new #GDataContactsContact; unref with g_object_unref()
 *
 * Since: 0.2.0
 */
GDataContactsContact *
gdata_contacts_contact_new (const gchar *id)
{
	return GDATA_CONTACTS_CONTACT (g_object_new (GDATA_TYPE_CONTACTS_CONTACT, "id", id, NULL));
}

/**
 * gdata_contacts_contact_get_edited:
 * @self: a #GDataContactsContact
 *
 * Gets the #GDataContactsContact:edited property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the UNIX timestamp for the time the contact was last edited, or <code class="literal">-1</code>
 *
 * Since: 0.2.0
 */
gint64
gdata_contacts_contact_get_edited (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), -1);
	return self->priv->edited;
}

/**
 * gdata_contacts_contact_get_name:
 * @self: a #GDataContactsContact
 *
 * Gets the #GDataContactsContact:name property.
 *
 * Return value: (transfer none): the contact's name, or %NULL
 *
 * Since: 0.5.0
 */
GDataGDName *
gdata_contacts_contact_get_name (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->name;
}

/**
 * gdata_contacts_contact_set_name:
 * @self: a #GDataContactsContact
 * @name: the new #GDataGDName
 *
 * Sets the #GDataContactsContact:name property to @name, and increments its reference count.
 *
 * @name must not be %NULL, though all its properties may be %NULL.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_set_name (GDataContactsContact *self, GDataGDName *name)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (GDATA_IS_GD_NAME (name));

	if (self->priv->name != NULL)
		g_object_unref (self->priv->name);
	self->priv->name = g_object_ref (name);
	g_object_notify (G_OBJECT (self), "name");

	/* Notify the change in #GDataGDName:full-name explicitly, so that our #GDataEntry:title gets updated */
	notify_full_name_cb (G_OBJECT (name), NULL, self);
}

/**
 * gdata_contacts_contact_get_nickname:
 * @self: a #GDataContactsContact
 *
 * Gets the #GDataContactsContact:nickname property.
 *
 * Return value: the contact's nickname, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_contacts_contact_get_nickname (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->nickname;
}

/**
 * gdata_contacts_contact_set_nickname:
 * @self: a #GDataContactsContact
 * @nickname: (allow-none): the new nickname, or %NULL
 *
 * Sets the #GDataContactsContact:nickname property to @nickname.
 *
 * If @nickname is %NULL, the contact's nickname will be removed.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_set_nickname (GDataContactsContact *self, const gchar *nickname)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_free (self->priv->nickname);
	self->priv->nickname = g_strdup (nickname);
	g_object_notify (G_OBJECT (self), "nickname");
}

/**
 * gdata_contacts_contact_get_file_as:
 * @self: a #GDataContactsContact
 *
 * Gets the #GDataContactsContact:file-as property.
 *
 * Return value: the name the contact's filed under, or %NULL
 *
 * Since: 0.11.0
 */
const gchar *
gdata_contacts_contact_get_file_as (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->file_as;
}

/**
 * gdata_contacts_contact_set_file_as:
 * @self: a #GDataContactsContact
 * @file_as: (allow-none): the new name to file the contact under, or %NULL
 *
 * Sets the #GDataContactsContact:file-as property to @file_as.
 *
 * If @file_as is %NULL, the contact will be filed under their full name.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_set_file_as (GDataContactsContact *self, const gchar *file_as)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_free (self->priv->file_as);
	self->priv->file_as = g_strdup (file_as);
	g_object_notify (G_OBJECT (self), "file-as");
}

/**
 * gdata_contacts_contact_get_birthday:
 * @self: a #GDataContactsContact
 * @birthday: (allow-none) (out caller-allocates): return location for the birthday, or %NULL
 *
 * Gets the #GDataContactsContact:birthday and #GDataContactsContact:birthday-has-year properties. If @birthday is non-%NULL,
 * #GDataContactsContact:birthday is returned in it. The function returns the value of #GDataContactsContact:birthday-has-year,
 * which specifies whether the year in @birthday is meaningful. Contacts may not have the year of their birth set, in which
 * case, the function would return %FALSE, and the year in @birthday should be ignored.
 *
 * Return value: whether the contact's birthday has the year set
 *
 * Since: 0.7.0
 */
gboolean
gdata_contacts_contact_get_birthday (GDataContactsContact *self, GDate *birthday)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), FALSE);

	if (birthday != NULL)
		*birthday = self->priv->birthday;
	return self->priv->birthday_has_year;
}

/**
 * gdata_contacts_contact_set_birthday:
 * @self: a #GDataContactsContact
 * @birthday: (allow-none): the new birthday, or %NULL
 * @birthday_has_year: %TRUE if @birthday's year is relevant, %FALSE otherwise
 *
 * Sets the #GDataContactsContact:birthday property to @birthday and the #GDataContactsContact:birthday-has-year property to @birthday_has_year.
 * See gdata_contacts_contact_get_birthday() for an explanation of the interaction between these two properties.
 *
 * If @birthday is %NULL, the contact's birthday will be removed.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_set_birthday (GDataContactsContact *self, GDate *birthday, gboolean birthday_has_year)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (birthday == NULL || g_date_valid (birthday));

	if (birthday != NULL)
		self->priv->birthday = *birthday;
	else
		g_date_clear (&(self->priv->birthday), 1);

	self->priv->birthday_has_year = birthday_has_year;

	g_object_freeze_notify (G_OBJECT (self));
	g_object_notify (G_OBJECT (self), "birthday");
	g_object_notify (G_OBJECT (self), "birthday-has-year");
	g_object_thaw_notify (G_OBJECT (self));
}

/**
 * gdata_contacts_contact_get_billing_information:
 * @self: a #GDataContactsContact
 *
 * Gets the #GDataContactsContact:billing-information property.
 *
 * Return value: the contact's billing information, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_contacts_contact_get_billing_information (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->billing_information;
}

/**
 * gdata_contacts_contact_set_billing_information:
 * @self: a #GDataContactsContact
 * @billing_information: (allow-none): the new billing information for the contact, or %NULL
 *
 * Sets the #GDataContactsContact:billing-information property to @billing_information.
 *
 * If @billing_information is %NULL, the contact's billing information will be removed.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_set_billing_information (GDataContactsContact *self, const gchar *billing_information)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (billing_information == NULL || *billing_information != '\0');

	g_free (self->priv->billing_information);
	self->priv->billing_information = g_strdup (billing_information);
	g_object_notify (G_OBJECT (self), "billing-information");
}

/**
 * gdata_contacts_contact_get_directory_server:
 * @self: a #GDataContactsContact
 *
 * Gets the #GDataContactsContact:directory-server property.
 *
 * Return value: the name or address of a directory server associated with the contact, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_contacts_contact_get_directory_server (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->directory_server;
}

/**
 * gdata_contacts_contact_set_directory_server:
 * @self: a #GDataContactsContact
 * @directory_server: (allow-none): the new name or address of a directory server associated with the contact, or %NULL
 *
 * Sets the #GDataContactsContact:directory-server property to @directory_server.
 *
 * If @directory_server is %NULL, the contact's directory server will be removed.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_set_directory_server (GDataContactsContact *self, const gchar *directory_server)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (directory_server == NULL || *directory_server != '\0');

	g_free (self->priv->directory_server);
	self->priv->directory_server = g_strdup (directory_server);
	g_object_notify (G_OBJECT (self), "directory-server");
}

/**
 * gdata_contacts_contact_get_gender:
 * @self: a #GDataContactsContact
 *
 * Gets the #GDataContactsContact:gender property.
 *
 * Return value: the gender of the contact, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_contacts_contact_get_gender (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->gender;
}

/**
 * gdata_contacts_contact_set_gender:
 * @self: a #GDataContactsContact
 * @gender: (allow-none): the new gender of the contact, or %NULL
 *
 * Sets the #GDataContactsContact:gender property to @gender.
 *
 * If @gender is %NULL, the contact's gender will be removed.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_set_gender (GDataContactsContact *self, const gchar *gender)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (gender == NULL || *gender != '\0');

	g_free (self->priv->gender);
	self->priv->gender = g_strdup (gender);
	g_object_notify (G_OBJECT (self), "gender");
}

/**
 * gdata_contacts_contact_get_initials:
 * @self: a #GDataContactsContact
 *
 * Gets the #GDataContactsContact:initials property.
 *
 * Return value: the initials of the contact, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_contacts_contact_get_initials (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->initials;
}

/**
 * gdata_contacts_contact_set_initials:
 * @self: a #GDataContactsContact
 * @initials: (allow-none): the new initials of the contact, or %NULL
 *
 * Sets the #GDataContactsContact:initials property to @initials.
 *
 * If @initials is %NULL, the contact's initials will be removed.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_set_initials (GDataContactsContact *self, const gchar *initials)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_free (self->priv->initials);
	self->priv->initials = g_strdup (initials);
	g_object_notify (G_OBJECT (self), "initials");
}

/**
 * gdata_contacts_contact_get_maiden_name:
 * @self: a #GDataContactsContact
 *
 * Gets the #GDataContactsContact:maiden-name property.
 *
 * Return value: the maiden name of the contact, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_contacts_contact_get_maiden_name (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->maiden_name;
}

/**
 * gdata_contacts_contact_set_maiden_name:
 * @self: a #GDataContactsContact
 * @maiden_name: (allow-none): the new maiden name of the contact, or %NULL
 *
 * Sets the #GDataContactsContact:maiden-name property to @maiden_name.
 *
 * If @maiden_name is %NULL, the contact's maiden name will be removed.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_set_maiden_name (GDataContactsContact *self, const gchar *maiden_name)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_free (self->priv->maiden_name);
	self->priv->maiden_name = g_strdup (maiden_name);
	g_object_notify (G_OBJECT (self), "maiden-name");
}

/**
 * gdata_contacts_contact_get_mileage:
 * @self: a #GDataContactsContact
 *
 * Gets the #GDataContactsContact:mileage property.
 *
 * Return value: a mileage associated with the contact, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_contacts_contact_get_mileage (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->mileage;
}

/**
 * gdata_contacts_contact_set_mileage:
 * @self: a #GDataContactsContact
 * @mileage: (allow-none): the new mileage associated with the contact, or %NULL
 *
 * Sets the #GDataContactsContact:mileage property to @mileage.
 *
 * If @mileage is %NULL, the contact's mileage will be removed.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_set_mileage (GDataContactsContact *self, const gchar *mileage)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_free (self->priv->mileage);
	self->priv->mileage = g_strdup (mileage);
	g_object_notify (G_OBJECT (self), "mileage");
}

/**
 * gdata_contacts_contact_get_occupation:
 * @self: a #GDataContactsContact
 *
 * Gets the #GDataContactsContact:occupation property.
 *
 * Return value: the contact's occupation, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_contacts_contact_get_occupation (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->occupation;
}

/**
 * gdata_contacts_contact_set_occupation:
 * @self: a #GDataContactsContact
 * @occupation: (allow-none): the contact's new occupation, or %NULL
 *
 * Sets the #GDataContactsContact:occupation property to @occupation.
 *
 * If @occupation is %NULL, the contact's occupation will be removed.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_set_occupation (GDataContactsContact *self, const gchar *occupation)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_free (self->priv->occupation);
	self->priv->occupation = g_strdup (occupation);
	g_object_notify (G_OBJECT (self), "occupation");
}

/**
 * gdata_contacts_contact_get_priority:
 * @self: a #GDataContactsContact
 *
 * Gets the #GDataContactsContact:priority property.
 *
 * Return value: the contact's priority, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_contacts_contact_get_priority (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->priority;
}

/**
 * gdata_contacts_contact_set_priority:
 * @self: a #GDataContactsContact
 * @priority: (allow-none): the contact's new priority, or %NULL
 *
 * Sets the #GDataContactsContact:priority property to @priority.
 *
 * If @priority is %NULL, the contact's priority will be removed.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_set_priority (GDataContactsContact *self, const gchar *priority)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (priority == NULL || *priority != '\0');

	g_free (self->priv->priority);
	self->priv->priority = g_strdup (priority);
	g_object_notify (G_OBJECT (self), "priority");
}

/**
 * gdata_contacts_contact_get_sensitivity:
 * @self: a #GDataContactsContact
 *
 * Gets the #GDataContactsContact:sensitivity property.
 *
 * Return value: the contact's sensitivity, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_contacts_contact_get_sensitivity (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->sensitivity;
}

/**
 * gdata_contacts_contact_set_sensitivity:
 * @self: a #GDataContactsContact
 * @sensitivity: (allow-none): the contact's new sensitivity, or %NULL
 *
 * Sets the #GDataContactsContact:sensitivity property to @sensitivity.
 *
 * If @sensitivity is %NULL, the contact's sensitivity will be removed.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_set_sensitivity (GDataContactsContact *self, const gchar *sensitivity)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (sensitivity == NULL || *sensitivity != '\0');

	g_free (self->priv->sensitivity);
	self->priv->sensitivity = g_strdup (sensitivity);
	g_object_notify (G_OBJECT (self), "sensitivity");
}

/**
 * gdata_contacts_contact_get_short_name:
 * @self: a #GDataContactsContact
 *
 * Gets the #GDataContactsContact:short-name property.
 *
 * Return value: the contact's short name, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_contacts_contact_get_short_name (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->short_name;
}

/**
 * gdata_contacts_contact_set_short_name:
 * @self: a #GDataContactsContact
 * @short_name: (allow-none): the contact's new short name, or %NULL
 *
 * Sets the #GDataContactsContact:short-name property to @short_name.
 *
 * If @short_name is %NULL, the contact's short name will be removed.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_set_short_name (GDataContactsContact *self, const gchar *short_name)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_free (self->priv->short_name);
	self->priv->short_name = g_strdup (short_name);
	g_object_notify (G_OBJECT (self), "short-name");
}

/**
 * gdata_contacts_contact_get_subject:
 * @self: a #GDataContactsContact
 *
 * Gets the #GDataContactsContact:subject property.
 *
 * Return value: the contact's subject, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_contacts_contact_get_subject (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->subject;
}

/**
 * gdata_contacts_contact_set_subject:
 * @self: a #GDataContactsContact
 * @subject: (allow-none): the contact's new subject, or %NULL
 *
 * Sets the #GDataContactsContact:subject property to @subject.
 *
 * If @subject is %NULL, the contact's subject will be removed.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_set_subject (GDataContactsContact *self, const gchar *subject)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_free (self->priv->subject);
	self->priv->subject = g_strdup (subject);
	g_object_notify (G_OBJECT (self), "subject");
}

/**
 * gdata_contacts_contact_add_email_address:
 * @self: a #GDataContactsContact
 * @email_address: a #GDataGDEmailAddress to add
 *
 * Adds an e-mail address to the contact's list of e-mail addresses and increments its reference count.
 *
 * Note that only one e-mail address per contact may be marked as "primary". Insertion and update operations
 * (with gdata_contacts_service_insert_contact()) will return an error if more than one e-mail address
 * is marked as primary.
 *
 * Duplicate e-mail addresses will not be added to the list.
 *
 * Since: 0.2.0
 */
void
gdata_contacts_contact_add_email_address (GDataContactsContact *self, GDataGDEmailAddress *email_address)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (GDATA_IS_GD_EMAIL_ADDRESS (email_address));

	if (g_list_find_custom (self->priv->email_addresses, email_address, (GCompareFunc) gdata_comparable_compare) == NULL)
		self->priv->email_addresses = g_list_append (self->priv->email_addresses, g_object_ref (email_address));
}

/**
 * gdata_contacts_contact_get_email_addresses:
 * @self: a #GDataContactsContact
 *
 * Gets a list of the e-mail addresses owned by the contact.
 *
 * Return value: (element-type GData.GDEmailAddress) (transfer none): a #GList of #GDataGDEmailAddress<!-- -->es, or %NULL
 *
 * Since: 0.2.0
 */
GList *
gdata_contacts_contact_get_email_addresses (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->email_addresses;
}

/**
 * gdata_contacts_contact_get_primary_email_address:
 * @self: a #GDataContactsContact
 *
 * Gets the contact's primary e-mail address, if one exists.
 *
 * Return value: (transfer none): a #GDataGDEmailAddress, or %NULL
 *
 * Since: 0.2.0
 */
GDataGDEmailAddress *
gdata_contacts_contact_get_primary_email_address (GDataContactsContact *self)
{
	GList *i;

	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);

	for (i = self->priv->email_addresses; i != NULL; i = i->next) {
		if (gdata_gd_email_address_is_primary (GDATA_GD_EMAIL_ADDRESS (i->data)) == TRUE)
			return GDATA_GD_EMAIL_ADDRESS (i->data);
	}

	return NULL;
}

/**
 * gdata_contacts_contact_remove_all_email_addresses:
 * @self: a #GDataContactsContact
 *
 * Removes all e-mail addresses from the contact.
 *
 * Since: 0.4.0
 */
void
gdata_contacts_contact_remove_all_email_addresses (GDataContactsContact *self)
{
	GDataContactsContactPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_list_free_full (priv->email_addresses, g_object_unref);
	priv->email_addresses = NULL;
}

/**
 * gdata_contacts_contact_add_im_address:
 * @self: a #GDataContactsContact
 * @im_address: a #GDataGDIMAddress to add
 *
 * Adds an IM (instant messaging) address to the contact's list of IM addresses and increments its reference count.
 *
 * Note that only one IM address per contact may be marked as "primary". Insertion and update operations
 * (with gdata_contacts_service_insert_contact()) will return an error if more than one IM address
 * is marked as primary.
 *
 * Duplicate IM addresses will not be added to the list.
 *
 * Since: 0.2.0
 */
void
gdata_contacts_contact_add_im_address (GDataContactsContact *self, GDataGDIMAddress *im_address)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (GDATA_IS_GD_IM_ADDRESS (im_address));

	if (g_list_find_custom (self->priv->im_addresses, im_address, (GCompareFunc) gdata_comparable_compare) == NULL)
		self->priv->im_addresses = g_list_append (self->priv->im_addresses, g_object_ref (im_address));
}

/**
 * gdata_contacts_contact_get_im_addresses:
 * @self: a #GDataContactsContact
 *
 * Gets a list of the IM addresses owned by the contact.
 *
 * Return value: (element-type GData.GDIMAddress) (transfer none): a #GList of #GDataGDIMAddress<!-- -->es, or %NULL
 *
 * Since: 0.2.0
 */
GList *
gdata_contacts_contact_get_im_addresses (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->im_addresses;
}

/**
 * gdata_contacts_contact_get_primary_im_address:
 * @self: a #GDataContactsContact
 *
 * Gets the contact's primary IM address, if one exists.
 *
 * Return value: (transfer none): a #GDataGDIMAddress, or %NULL
 *
 * Since: 0.2.0
 */
GDataGDIMAddress *
gdata_contacts_contact_get_primary_im_address (GDataContactsContact *self)
{
	GList *i;

	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);

	for (i = self->priv->im_addresses; i != NULL; i = i->next) {
		if (gdata_gd_im_address_is_primary (GDATA_GD_IM_ADDRESS (i->data)) == TRUE)
			return GDATA_GD_IM_ADDRESS (i->data);
	}

	return NULL;
}

/**
 * gdata_contacts_contact_remove_all_im_addresses:
 * @self: a #GDataContactsContact
 *
 * Removes all IM addresses from the contact.
 *
 * Since: 0.4.0
 */
void
gdata_contacts_contact_remove_all_im_addresses (GDataContactsContact *self)
{
	GDataContactsContactPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_list_free_full (priv->im_addresses, g_object_unref);
	priv->im_addresses = NULL;
}

/**
 * gdata_contacts_contact_add_phone_number:
 * @self: a #GDataContactsContact
 * @phone_number: a #GDataGDPhoneNumber to add
 *
 * Adds a phone number to the contact's list of phone numbers and increments its reference count
 *
 * Note that only one phone number per contact may be marked as "primary". Insertion and update operations
 * (with gdata_contacts_service_insert_contact()) will return an error if more than one phone number
 * is marked as primary.
 *
 * Duplicate phone numbers will not be added to the list.
 *
 * Since: 0.2.0
 */
void
gdata_contacts_contact_add_phone_number (GDataContactsContact *self, GDataGDPhoneNumber *phone_number)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (GDATA_IS_GD_PHONE_NUMBER (phone_number));

	if (g_list_find_custom (self->priv->phone_numbers, phone_number, (GCompareFunc) gdata_comparable_compare) == NULL)
		self->priv->phone_numbers = g_list_append (self->priv->phone_numbers, g_object_ref (phone_number));
}

/**
 * gdata_contacts_contact_get_phone_numbers:
 * @self: a #GDataContactsContact
 *
 * Gets a list of the phone numbers owned by the contact.
 *
 * Return value: (element-type GData.GDPhoneNumber) (transfer none): a #GList of #GDataGDPhoneNumbers, or %NULL
 *
 * Since: 0.2.0
 */
GList *
gdata_contacts_contact_get_phone_numbers (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->phone_numbers;
}

/**
 * gdata_contacts_contact_get_primary_phone_number:
 * @self: a #GDataContactsContact
 *
 * Gets the contact's primary phone number, if one exists.
 *
 * Return value: (transfer none): a #GDataGDPhoneNumber, or %NULL
 *
 * Since: 0.2.0
 */
GDataGDPhoneNumber *
gdata_contacts_contact_get_primary_phone_number (GDataContactsContact *self)
{
	GList *i;

	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);

	for (i = self->priv->phone_numbers; i != NULL; i = i->next) {
		if (gdata_gd_phone_number_is_primary (GDATA_GD_PHONE_NUMBER (i->data)) == TRUE)
			return GDATA_GD_PHONE_NUMBER (i->data);
	}

	return NULL;
}

/**
 * gdata_contacts_contact_remove_all_phone_numbers:
 * @self: a #GDataContactsContact
 *
 * Removes all phone numbers from the contact.
 *
 * Since: 0.4.0
 */
void
gdata_contacts_contact_remove_all_phone_numbers (GDataContactsContact *self)
{
	GDataContactsContactPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_list_free_full (priv->phone_numbers, g_object_unref);
	priv->phone_numbers = NULL;
}

/**
 * gdata_contacts_contact_add_postal_address:
 * @self: a #GDataContactsContact
 * @postal_address: a #GDataGDPostalAddress to add
 *
 * Adds a postal address to the contact's list of postal addresses and increments its reference count.
 *
 * Note that only one postal address per contact may be marked as "primary". Insertion and update operations
 * (with gdata_contacts_service_insert_contact()) will return an error if more than one postal address
 * is marked as primary.
 *
 * Duplicate postal addresses will not be added to the list.
 *
 * Since: 0.2.0
 */
void
gdata_contacts_contact_add_postal_address (GDataContactsContact *self, GDataGDPostalAddress *postal_address)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (GDATA_IS_GD_POSTAL_ADDRESS (postal_address));

	if (g_list_find_custom (self->priv->postal_addresses, postal_address, (GCompareFunc) gdata_comparable_compare) == NULL)
		self->priv->postal_addresses = g_list_append (self->priv->postal_addresses, g_object_ref (postal_address));
}

/**
 * gdata_contacts_contact_get_postal_addresses:
 * @self: a #GDataContactsContact
 *
 * Gets a list of the postal addresses owned by the contact.
 *
 * Return value: (element-type GData.GDPostalAddress) (transfer none): a #GList of #GDataGDPostalAddress<!-- -->es, or %NULL
 *
 * Since: 0.2.0
 */
GList *
gdata_contacts_contact_get_postal_addresses (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->postal_addresses;
}

/**
 * gdata_contacts_contact_get_primary_postal_address:
 * @self: a #GDataContactsContact
 *
 * Gets the contact's primary postal address, if one exists.
 *
 * Return value: (transfer none): a #GDataGDPostalAddress, or %NULL
 *
 * Since: 0.2.0
 */
GDataGDPostalAddress *
gdata_contacts_contact_get_primary_postal_address (GDataContactsContact *self)
{
	GList *i;

	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);

	for (i = self->priv->postal_addresses; i != NULL; i = i->next) {
		if (gdata_gd_postal_address_is_primary (GDATA_GD_POSTAL_ADDRESS (i->data)) == TRUE)
			return GDATA_GD_POSTAL_ADDRESS (i->data);
	}

	return NULL;
}

/**
 * gdata_contacts_contact_remove_all_postal_addresses:
 * @self: a #GDataContactsContact
 *
 * Removes all postal addresses from the contact.
 *
 * Since: 0.4.0
 */
void
gdata_contacts_contact_remove_all_postal_addresses (GDataContactsContact *self)
{
	GDataContactsContactPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_list_free_full (priv->postal_addresses, g_object_unref);
	priv->postal_addresses = NULL;
}

/**
 * gdata_contacts_contact_add_organization:
 * @self: a #GDataContactsContact
 * @organization: a #GDataGDOrganization to add
 *
 * Adds an organization to the contact's list of organizations (e.g. employers) and increments its reference count.
 *
 * Note that only one organization per contact may be marked as "primary". Insertion and update operations
 * (with gdata_contacts_service_insert_contact()) will return an error if more than one organization
 * is marked as primary.
 *
 * Duplicate organizations will not be added to the list.
 *
 * Since: 0.2.0
 */
void
gdata_contacts_contact_add_organization (GDataContactsContact *self, GDataGDOrganization *organization)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (organization != NULL);

	if (g_list_find_custom (self->priv->organizations, organization, (GCompareFunc) gdata_comparable_compare) == NULL)
		self->priv->organizations = g_list_append (self->priv->organizations, g_object_ref (organization));
}

/**
 * gdata_contacts_contact_get_organizations:
 * @self: a #GDataContactsContact
 *
 * Gets a list of the organizations to which the contact belongs.
 *
 * Return value: (element-type GData.GDOrganization) (transfer none): a #GList of #GDataGDOrganizations, or %NULL
 *
 * Since: 0.2.0
 */
GList *
gdata_contacts_contact_get_organizations (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->organizations;
}

/**
 * gdata_contacts_contact_get_primary_organization:
 * @self: a #GDataContactsContact
 *
 * Gets the contact's primary organization, if one exists.
 *
 * Return value: (transfer none): a #GDataGDOrganization, or %NULL
 *
 * Since: 0.2.0
 */
GDataGDOrganization *
gdata_contacts_contact_get_primary_organization (GDataContactsContact *self)
{
	GList *i;

	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);

	for (i = self->priv->organizations; i != NULL; i = i->next) {
		if (gdata_gd_organization_is_primary (GDATA_GD_ORGANIZATION (i->data)) == TRUE)
			return GDATA_GD_ORGANIZATION (i->data);
	}

	return NULL;
}

/**
 * gdata_contacts_contact_remove_all_organizations:
 * @self: a #GDataContactsContact
 *
 * Removes all organizations from the contact.
 *
 * Since: 0.4.0
 */
void
gdata_contacts_contact_remove_all_organizations (GDataContactsContact *self)
{
	GDataContactsContactPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_list_free_full (priv->organizations, g_object_unref);
	priv->organizations = NULL;
}

/**
 * gdata_contacts_contact_add_jot:
 * @self: a #GDataContactsContact
 * @jot: a #GDataGContactJot to add
 *
 * Adds a jot to the contact's list of jots and increments its reference count.
 *
 * Duplicate jots will be added to the list, and multiple jots with the same relation type can be added to a single contact.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_add_jot (GDataContactsContact *self, GDataGContactJot *jot)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (GDATA_IS_GCONTACT_JOT (jot));

	self->priv->jots = g_list_append (self->priv->jots, g_object_ref (jot));
}

/**
 * gdata_contacts_contact_get_jots:
 * @self: a #GDataContactsContact
 *
 * Gets a list of the jots attached to the contact.
 *
 * Return value: (element-type GData.GContactJot) (transfer none): a #GList of #GDataGContactJots, or %NULL
 *
 * Since: 0.7.0
 */
GList *
gdata_contacts_contact_get_jots (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->jots;
}

/**
 * gdata_contacts_contact_remove_all_jots:
 * @self: a #GDataContactsContact
 *
 * Removes all jots from the contact.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_remove_all_jots (GDataContactsContact *self)
{
	GDataContactsContactPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_list_free_full (priv->jots, g_object_unref);
	priv->jots = NULL;
}

/**
 * gdata_contacts_contact_add_relation:
 * @self: a #GDataContactsContact
 * @relation: a #GDataGContactRelation to add
 *
 * Adds a relation to the contact's list of relations and increments its reference count.
 *
 * Duplicate relations will be added to the list, and multiple relations with the same relation type can be added to a single contact.
 * Though it may not make sense for some relation types to be repeated, adding them is allowed.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_add_relation (GDataContactsContact *self, GDataGContactRelation *relation)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (GDATA_IS_GCONTACT_RELATION (relation));

	self->priv->relations = g_list_append (self->priv->relations, g_object_ref (relation));
}

/**
 * gdata_contacts_contact_get_relations:
 * @self: a #GDataContactsContact
 *
 * Gets a list of the relations of the contact.
 *
 * Return value: (element-type GData.GContactRelation) (transfer none): a #GList of #GDataGContactRelations, or %NULL
 *
 * Since: 0.7.0
 */
GList *
gdata_contacts_contact_get_relations (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->relations;
}

/**
 * gdata_contacts_contact_remove_all_relations:
 * @self: a #GDataContactsContact
 *
 * Removes all relations from the contact.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_remove_all_relations (GDataContactsContact *self)
{
	GDataContactsContactPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_list_free_full (priv->relations, g_object_unref);
	priv->relations = NULL;
}

/**
 * gdata_contacts_contact_add_website:
 * @self: a #GDataContactsContact
 * @website: a #GDataGContactWebsite to add
 *
 * Adds a website to the contact's list of websites and increments its reference count.
 *
 * Duplicate websites will not be added to the list, though the same URI may appear in several #GDataGContactWebsites with different
 * relation types or labels.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_add_website (GDataContactsContact *self, GDataGContactWebsite *website)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (GDATA_IS_GCONTACT_WEBSITE (website));

	if (g_list_find_custom (self->priv->websites, website, (GCompareFunc) gdata_comparable_compare) == NULL)
		self->priv->websites = g_list_append (self->priv->websites, g_object_ref (website));
}

/**
 * gdata_contacts_contact_get_websites:
 * @self: a #GDataContactsContact
 *
 * Gets a list of the websites of the contact.
 *
 * Return value: (element-type GData.GContactWebsite) (transfer none): a #GList of #GDataGContactWebsites, or %NULL
 *
 * Since: 0.7.0
 */
GList *
gdata_contacts_contact_get_websites (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->websites;
}

/**
 * gdata_contacts_contact_get_primary_website:
 * @self: a #GDataContactsContact
 *
 * Gets the contact's primary website, if one exists.
 *
 * Return value: (transfer none): a #GDataGContactWebsite, or %NULL
 *
 * Since: 0.7.0
 */
GDataGContactWebsite *
gdata_contacts_contact_get_primary_website (GDataContactsContact *self)
{
	GList *i;

	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);

	for (i = self->priv->websites; i != NULL; i = i->next) {
		if (gdata_gcontact_website_is_primary (GDATA_GCONTACT_WEBSITE (i->data)) == TRUE)
			return GDATA_GCONTACT_WEBSITE (i->data);
	}

	return NULL;
}

/**
 * gdata_contacts_contact_remove_all_websites:
 * @self: a #GDataContactsContact
 *
 * Removes all websites from the contact.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_remove_all_websites (GDataContactsContact *self)
{
	GDataContactsContactPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_list_free_full (priv->websites, g_object_unref);
	priv->websites = NULL;
}

/**
 * gdata_contacts_contact_add_event:
 * @self: a #GDataContactsContact
 * @event: a #GDataGContactEvent to add
 *
 * Adds an event to the contact's list of events and increments its reference count.
 *
 * Duplicate events will be added to the list, and multiple events with the same event type can be added to a single contact.
 * Though it may not make sense for some event types to be repeated, adding them is allowed.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_add_event (GDataContactsContact *self, GDataGContactEvent *event)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (GDATA_IS_GCONTACT_EVENT (event));

	self->priv->events = g_list_append (self->priv->events, g_object_ref (event));
}

/**
 * gdata_contacts_contact_get_events:
 * @self: a #GDataContactsContact
 *
 * Gets a list of the events of the contact.
 *
 * Return value: (element-type GData.GContactEvent) (transfer none): a #GList of #GDataGContactEvents, or %NULL
 *
 * Since: 0.7.0
 */
GList *
gdata_contacts_contact_get_events (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->events;
}

/**
 * gdata_contacts_contact_remove_all_events:
 * @self: a #GDataContactsContact
 *
 * Removes all events from the contact.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_remove_all_events (GDataContactsContact *self)
{
	GDataContactsContactPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_list_free_full (priv->events, g_object_unref);
	priv->events = NULL;
}

/**
 * gdata_contacts_contact_add_calendar:
 * @self: a #GDataContactsContact
 * @calendar: a #GDataGContactCalendar to add
 *
 * Adds a calendar to the contact's list of calendars and increments its reference count.
 *
 * Duplicate calendars will not be added to the list, though the same URI may appear in several #GDataGContactCalendars with different
 * relation types or labels.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_add_calendar (GDataContactsContact *self, GDataGContactCalendar *calendar)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (GDATA_IS_GCONTACT_CALENDAR (calendar));

	if (g_list_find_custom (self->priv->calendars, calendar, (GCompareFunc) gdata_comparable_compare) == NULL)
		self->priv->calendars = g_list_append (self->priv->calendars, g_object_ref (calendar));
}

/**
 * gdata_contacts_contact_get_calendars:
 * @self: a #GDataContactsContact
 *
 * Gets a list of the calendars of the contact.
 *
 * Return value: (element-type GData.GContactCalendar) (transfer none): a #GList of #GDataGContactCalendars, or %NULL
 *
 * Since: 0.7.0
 */
GList *
gdata_contacts_contact_get_calendars (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->calendars;
}

/**
 * gdata_contacts_contact_get_primary_calendar:
 * @self: a #GDataContactsContact
 *
 * Gets the contact's primary calendar, if one exists.
 *
 * Return value: (transfer none): a #GDataGContactCalendar, or %NULL
 *
 * Since: 0.7.0
 */
GDataGContactCalendar *
gdata_contacts_contact_get_primary_calendar (GDataContactsContact *self)
{
	GList *i;

	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);

	for (i = self->priv->calendars; i != NULL; i = i->next) {
		if (gdata_gcontact_calendar_is_primary (GDATA_GCONTACT_CALENDAR (i->data)) == TRUE)
			return GDATA_GCONTACT_CALENDAR (i->data);
	}

	return NULL;
}

/**
 * gdata_contacts_contact_remove_all_calendars:
 * @self: a #GDataContactsContact
 *
 * Removes all calendars from the contact.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_remove_all_calendars (GDataContactsContact *self)
{
	GDataContactsContactPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_list_free_full (priv->calendars, g_object_unref);
	priv->calendars = NULL;
}

/**
 * gdata_contacts_contact_add_external_id:
 * @self: a #GDataContactsContact
 * @external_id: a #GDataGContactExternalID to add
 *
 * Adds an external ID to the contact's list of external IDs and increments its reference count.
 *
 * Duplicate IDs will not be added to the list.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_add_external_id (GDataContactsContact *self, GDataGContactExternalID *external_id)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (GDATA_IS_GCONTACT_EXTERNAL_ID (external_id));

	if (g_list_find_custom (self->priv->external_ids, external_id, (GCompareFunc) gdata_comparable_compare) == NULL)
		self->priv->external_ids = g_list_append (self->priv->external_ids, g_object_ref (external_id));
}

/**
 * gdata_contacts_contact_get_external_ids:
 * @self: a #GDataContactsContact
 *
 * Gets a list of the external IDs of the contact.
 *
 * Return value: (element-type GData.GContactExternalID) (transfer none): a #GList of #GDataGContactExternalIDs, or %NULL
 *
 * Since: 0.7.0
 */
GList *
gdata_contacts_contact_get_external_ids (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->external_ids;
}

/**
 * gdata_contacts_contact_remove_all_external_ids:
 * @self: a #GDataContactsContact
 *
 * Removes all external IDs from the contact.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_remove_all_external_ids (GDataContactsContact *self)
{
	GDataContactsContactPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_list_free_full (priv->external_ids, g_object_unref);
	priv->external_ids = NULL;
}

/**
 * gdata_contacts_contact_add_hobby:
 * @self: a #GDataContactsContact
 * @hobby: a hobby to add
 *
 * Adds a hobby to the contact's list of hobbies, copying it in the process.
 *
 * Duplicate hobbies will not be added to the list.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_add_hobby (GDataContactsContact *self, const gchar *hobby)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (hobby != NULL && *hobby != '\0');

	if (g_list_find_custom (self->priv->hobbies, hobby, (GCompareFunc) g_strcmp0) == NULL)
		self->priv->hobbies = g_list_append (self->priv->hobbies, g_strdup (hobby));
}

/**
 * gdata_contacts_contact_get_hobbies:
 * @self: a #GDataContactsContact
 *
 * Gets a list of the hobbies of the contact.
 *
 * Return value: (element-type utf8) (transfer none): a #GList of hobby strings, or %NULL
 *
 * Since: 0.7.0
 */
GList *
gdata_contacts_contact_get_hobbies (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->hobbies;
}

/**
 * gdata_contacts_contact_remove_all_hobbies:
 * @self: a #GDataContactsContact
 *
 * Removes all hobbies from the contact.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_remove_all_hobbies (GDataContactsContact *self)
{
	GDataContactsContactPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_list_free_full (priv->hobbies, g_free);
	priv->hobbies = NULL;
}

/**
 * gdata_contacts_contact_add_language:
 * @self: a #GDataContactsContact
 * @language: a #GDataGContactLanguage to add
 *
 * Adds a language to the contact's list of languages and increments its reference count.
 *
 * Duplicate languages will not be added to the list.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_add_language (GDataContactsContact *self, GDataGContactLanguage *language)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (GDATA_IS_GCONTACT_LANGUAGE (language));

	if (g_list_find_custom (self->priv->languages, language, (GCompareFunc) gdata_comparable_compare) == NULL)
		self->priv->languages = g_list_append (self->priv->languages, g_object_ref (language));
}

/**
 * gdata_contacts_contact_get_languages:
 * @self: a #GDataContactsContact
 *
 * Gets a list of the languages of the contact.
 *
 * Return value: (element-type GData.GContactLanguage) (transfer none): a #GList of #GDataGContactLanguages, or %NULL
 *
 * Since: 0.7.0
 */
GList *
gdata_contacts_contact_get_languages (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->languages;
}

/**
 * gdata_contacts_contact_remove_all_languages:
 * @self: a #GDataContactsContact
 *
 * Removes all languages from the contact.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_remove_all_languages (GDataContactsContact *self)
{
	GDataContactsContactPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));

	g_list_free_full (priv->languages, g_object_unref);
	priv->languages = NULL;
}

/**
 * gdata_contacts_contact_get_extended_property:
 * @self: a #GDataContactsContact
 * @name: the property name; an arbitrary, unique string
 *
 * Gets the value of an extended property of the contact. Each contact can have up to 10 client-set extended
 * properties to store data of the client's choosing.
 *
 * Return value: the property's value, or %NULL
 *
 * Since: 0.2.0
 */
const gchar *
gdata_contacts_contact_get_extended_property (GDataContactsContact *self, const gchar *name)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	g_return_val_if_fail (name != NULL && *name != '\0', NULL);
	return g_hash_table_lookup (self->priv->extended_properties, name);
}

/**
 * gdata_contacts_contact_get_extended_properties:
 * @self: a #GDataContactsContact
 *
 * Gets the full list of extended properties of the contact; a hash table mapping property name to value.
 *
 * Return value: (transfer none): a #GHashTable of extended properties
 *
 * Since: 0.4.0
 */
GHashTable *
gdata_contacts_contact_get_extended_properties (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->extended_properties;
}

/**
 * gdata_contacts_contact_set_extended_property:
 * @self: a #GDataContactsContact
 * @name: the property name; an arbitrary, unique string
 * @value: (allow-none): the property value, or %NULL
 *
 * Sets the value of a contact's extended property. Extended property names are unique (but of the client's choosing),
 * and reusing the same property name will result in the old value of that property being overwritten.
 *
 * To unset a property, set @value to %NULL or an empty string.
 *
 * A contact may have up to 10 extended properties, and each should be reasonably small (i.e. not a photo or ringtone).
 * For more information, see the <ulink type="http"
 * url="http://code.google.com/apis/contacts/docs/2.0/reference.html#ProjectionsAndExtended">online documentation</ulink>.
 * %FALSE will be returned if you attempt to add more than 10 extended properties.
 *
 * Return value: %TRUE if the property was updated or deleted successfully, %FALSE otherwise
 *
 * Since: 0.2.0
 */
gboolean
gdata_contacts_contact_set_extended_property (GDataContactsContact *self, const gchar *name, const gchar *value)
{
	GHashTable *extended_properties = self->priv->extended_properties;

	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), FALSE);
	g_return_val_if_fail (name != NULL && *name != '\0', FALSE);

	if (value == NULL || *value == '\0') {
		/* Removing a property */
		g_hash_table_remove (extended_properties, name);
		return TRUE;
	}

	/* We can't add more than MAX_N_EXTENDED_PROPERTIES */
	if (g_hash_table_lookup (extended_properties, name) == NULL &&
	    g_hash_table_size (extended_properties) >= MAX_N_EXTENDED_PROPERTIES)
		return FALSE;

	/* Updating an existing property or adding a new one */
	g_hash_table_insert (extended_properties, g_strdup (name), g_strdup (value));

	return TRUE;
}

/**
 * gdata_contacts_contact_get_user_defined_field:
 * @self: a #GDataContactsContact
 * @name: the field name; an arbitrary, case-sensitive, unique string
 *
 * Gets the value of a user-defined field of the contact. User-defined fields are settable by the user through the Google Contacts web interface,
 * in contrast to extended properties, which are visible and settable only through the GData interface.
 *
 * The @name of the field may not be %NULL, but may be an empty string.
 *
 * Return value: the field's value, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_contacts_contact_get_user_defined_field (GDataContactsContact *self, const gchar *name)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	g_return_val_if_fail (name != NULL, NULL);
	return g_hash_table_lookup (self->priv->user_defined_fields, name);
}

/**
 * gdata_contacts_contact_get_user_defined_fields:
 * @self: a #GDataContactsContact
 *
 * Gets the full list of user-defined fields of the contact; a hash table mapping field name to value.
 *
 * Return value: (transfer none): a #GHashTable of user-defined fields
 *
 * Since: 0.7.0
 */
GHashTable *
gdata_contacts_contact_get_user_defined_fields (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->user_defined_fields;
}

/**
 * gdata_contacts_contact_set_user_defined_field:
 * @self: a #GDataContactsContact
 * @name: the field name; an arbitrary, case-sensitive, unique string
 * @value: (allow-none): the field value, or %NULL
 *
 * Sets the value of a contact's user-defined field. User-defined field names are unique (but of the client's choosing),
 * and reusing the same field name will result in the old value of that field being overwritten.
 *
 * The @name of the field may not be %NULL, but may be an empty string.
 *
 * To unset a field, set @value to %NULL.
 *
 * Since: 0.7.0
 */
void
gdata_contacts_contact_set_user_defined_field (GDataContactsContact *self, const gchar *name, const gchar *value)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (name != NULL);

	if (value == NULL) {
		/* Removing a field */
		g_hash_table_remove (self->priv->user_defined_fields, name);
	} else {
		/* Updating an existing field or adding a new one */
		g_hash_table_insert (self->priv->user_defined_fields, g_strdup (name), g_strdup (value));
	}
}

/**
 * gdata_contacts_contact_add_group:
 * @self: a #GDataContactsContact
 * @href: the group's ID URI
 *
 * Adds the contact to the given group. @href should be a URI.
 *
 * Since: 0.2.0
 */
void
gdata_contacts_contact_add_group (GDataContactsContact *self, const gchar *href)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (href != NULL);
	g_hash_table_insert (self->priv->groups, g_strdup (href), GUINT_TO_POINTER (FALSE));
}

/**
 * gdata_contacts_contact_remove_group:
 * @self: a #GDataContactsContact
 * @href: the group's ID URI
 *
 * Removes the contact from the given group. @href should be a URI.
 *
 * Since: 0.2.0
 */
void
gdata_contacts_contact_remove_group (GDataContactsContact *self, const gchar *href)
{
	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (href != NULL);
	g_hash_table_remove (self->priv->groups, href);
}

/**
 * gdata_contacts_contact_is_group_deleted:
 * @self: a #GDataContactsContact
 * @href: the group's ID URI
 *
 * Returns whether the contact has recently been removed from the given group on the server. This
 * will always return %FALSE unless #GDataContactsQuery:show-deleted has been set to
 * %TRUE for the query which returned the contact.
 *
 * If you've just removed a contact from a group locally using gdata_contacts_contact_remove_group(), %FALSE will still be returned by this function,
 * as the change hasn't been sent to the server.
 *
 * Return value: %TRUE if the contact has recently been removed from the group, %FALSE otherwise
 *
 * Since: 0.2.0
 */
gboolean
gdata_contacts_contact_is_group_deleted (GDataContactsContact *self, const gchar *href)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), FALSE);
	g_return_val_if_fail (href != NULL, FALSE);
	return GPOINTER_TO_UINT (g_hash_table_lookup (self->priv->groups, href));
}

/**
 * gdata_contacts_contact_get_groups:
 * @self: a #GDataContactsContact
 *
 * Gets a list of the groups to which the contact belongs.
 *
 * Return value: (element-type utf8) (transfer container): a #GList of constant group ID URIs, or %NULL; free with g_list_free()
 *
 * Since: 0.2.0
 */
GList *
gdata_contacts_contact_get_groups (GDataContactsContact *self)
{
	GHashTableIter iter;
	const gchar *href;
	gpointer value;
	GList *groups = NULL;

	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);

	g_hash_table_iter_init (&iter, self->priv->groups);
	while (g_hash_table_iter_next (&iter, (gpointer*) &href, &value) == TRUE) {
		/* Add the group to the list as long as it hasn't been deleted */
		if (GPOINTER_TO_UINT (value) == FALSE)
			groups = g_list_prepend (groups, (gpointer) href);
	}

	return g_list_reverse (groups);
}

/**
 * gdata_contacts_contact_is_deleted:
 * @self: a #GDataContactsContact
 *
 * Returns whether the contact has recently been deleted. This will always return
 * %FALSE unless #GDataContactsQuery:show-deleted has been set to
 * %TRUE for the query which returned the contact; then this function will return
 * %TRUE only if the contact has been deleted.
 *
 * If a contact has been deleted, no other information is available about it. This
 * is designed to allow contacts to be deleted from local address books using
 * incremental updates from the server (e.g. with #GDataQuery:updated-min and
 * #GDataContactsQuery:show-deleted).
 *
 * Return value: %TRUE if the contact has been deleted, %FALSE otherwise
 *
 * Since: 0.2.0
 */
gboolean
gdata_contacts_contact_is_deleted (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), FALSE);
	return self->priv->deleted;
}

/**
 * gdata_contacts_contact_get_photo_etag:
 * @self: a #GDataContactsContact
 *
 * Returns the ETag for the contact's attached photo, if it exists. If it does exist, the contact's photo can be retrieved using
 * gdata_contacts_contact_get_photo(). If it doesn't exist, %NULL will be returned, and the contact doesn't have a photo (so calling
 * gdata_contacts_contact_get_photo() will also return %NULL)
 *
 * Return value: the contact's photo's ETag if it exists, %NULL otherwise
 *
 * Since: 0.9.0
 */
const gchar *
gdata_contacts_contact_get_photo_etag (GDataContactsContact *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	return self->priv->photo_etag;
}

/**
 * gdata_contacts_contact_get_photo:
 * @self: a #GDataContactsContact
 * @service: a #GDataContactsService
 * @length: (out caller-allocates): return location for the image length, in bytes
 * @content_type: (out callee-allocates) (transfer full) (allow-none): return location for the image's content type, or %NULL; free with g_free()
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Downloads and returns the contact's photo, if they have one. If the contact doesn't
 * have a photo (i.e. gdata_contacts_contact_get_photo_etag() returns %NULL), %NULL is returned, but
 * no error is set in @error.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * If there is an error getting the photo, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned.
 *
 * Return value: (transfer full) (array length=length): the image data, or %NULL; free with g_free()
 *
 * Since: 0.8.0
 */
guint8 *
gdata_contacts_contact_get_photo (GDataContactsContact *self, GDataContactsService *service, gsize *length, gchar **content_type,
                                  GCancellable *cancellable, GError **error)
{
	GDataLink *_link;
	SoupMessage *message;
	guint status;
	guint8 *data;

	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	g_return_val_if_fail (GDATA_IS_CONTACTS_SERVICE (service), NULL);
	g_return_val_if_fail (length != NULL, NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Return if there is no photo */
	if (gdata_contacts_contact_get_photo_etag (self) == NULL)
		return NULL;

	/* Get the photo URI */
	/* TODO: ETag support */
	_link = gdata_entry_look_up_link (GDATA_ENTRY (self), "http://schemas.google.com/contacts/2008/rel#photo");
	g_assert (_link != NULL);
	message = _gdata_service_build_message (GDATA_SERVICE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                        SOUP_METHOD_GET, gdata_link_get_uri (_link), NULL, FALSE);

	/* Send the message */
	status = _gdata_service_send_message (GDATA_SERVICE (service), message, cancellable, error);

	if (status == SOUP_STATUS_NONE || status == SOUP_STATUS_CANCELLED) {
		/* Redirect error or cancelled */
		g_object_unref (message);
		return NULL;
	} else if (status != SOUP_STATUS_OK) {
		/* Error */
		GDataServiceClass *klass = GDATA_SERVICE_GET_CLASS (service);
		g_assert (klass->parse_error_response != NULL);
		klass->parse_error_response (GDATA_SERVICE (service), GDATA_OPERATION_DOWNLOAD, status, message->reason_phrase,
		                             message->response_body->data, message->response_body->length, error);
		g_object_unref (message);
		return NULL;
	}

	g_assert (message->response_body->data != NULL);

	/* Sort out the return values */
	if (content_type != NULL)
		*content_type = g_strdup (soup_message_headers_get_content_type (message->response_headers, NULL));
	*length = message->response_body->length;
	data = g_memdup (message->response_body->data, message->response_body->length);

	/* Update the stored photo ETag */
	g_free (self->priv->photo_etag);
	self->priv->photo_etag = g_strdup (soup_message_headers_get_one (message->response_headers, "ETag"));
	g_object_unref (message);

	return data;
}

typedef struct {
	GDataContactsService *service;
	guint8 *data;
	gsize length;
	gchar *content_type;
} PhotoData;

static void
photo_data_free (PhotoData *data)
{
	if (data->service != NULL)
		g_object_unref (data->service);
	g_free (data->data);
	g_free (data->content_type);
	g_slice_free (PhotoData, data);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (PhotoData, photo_data_free)

static void
get_photo_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	GDataContactsContact *contact = GDATA_CONTACTS_CONTACT (source_object);
	GDataContactsService *service = GDATA_CONTACTS_SERVICE (task_data);
	g_autoptr(PhotoData) data = NULL;
	g_autoptr(GError) error = NULL;

	/* Input and output */
	data = g_slice_new0 (PhotoData);

	/* Get the photo */
	data->data = gdata_contacts_contact_get_photo (contact, service, &(data->length), &(data->content_type), cancellable, &error);
	if (error != NULL)
		g_task_return_error (task, g_steal_pointer (&error));
	else
		g_task_return_pointer (task, g_steal_pointer (&data), (GDestroyNotify) photo_data_free);
}

/**
 * gdata_contacts_contact_get_photo_async:
 * @self: a #GDataContactsContact
 * @service: a #GDataContactsService
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the photo has been retrieved, or %NULL
 * @user_data: (closure): data to pass to the @callback function
 *
 * Downloads and returns the contact's photo, if they have one, asynchronously. @self and @service are both reffed when this function is called, so
 * can safely be unreffed after this function returns.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_contacts_contact_get_photo_finish() to get the results of the
 * operation.
 *
 * For more details, see gdata_contacts_contact_get_photo(), which is the synchronous version of this function.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled, the error %G_IO_ERROR_CANCELLED will be returned by gdata_contacts_contact_get_photo_finish().
 *
 * If there is an error getting the photo, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned by gdata_contacts_contact_get_photo_finish().
 *
 * Since: 0.8.0
 */
void
gdata_contacts_contact_get_photo_async (GDataContactsContact *self, GDataContactsService *service, GCancellable *cancellable,
                                        GAsyncReadyCallback callback, gpointer user_data)
{
	g_autoptr(GTask) task = NULL;

	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (GDATA_IS_CONTACTS_SERVICE (service));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_contacts_contact_get_photo_async);
	g_task_set_task_data (task, g_object_ref (service), (GDestroyNotify) g_object_unref);
	g_task_run_in_thread (task, get_photo_thread);
}

/**
 * gdata_contacts_contact_get_photo_finish:
 * @self: a #GDataContactsContact
 * @async_result: a #GAsyncResult
 * @length: (out caller-allocates): return location for the image length, in bytes
 * @content_type: (out callee-allocates) (transfer full) (allow-none): return location for the image's content type, or %NULL; free with g_free()
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous contact photo retrieval operation started with gdata_contacts_contact_get_photo_async(). If the contact doesn't have a
 * photo (i.e. gdata_contacts_contact_get_photo_etag() returns %NULL), %NULL is returned, but no error is set in @error.
 *
 * If there is an error getting the photo, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned. @length will be set to
 * <code class="literal">0</code> and @content_type will be set to %NULL.
 *
 * Return value: (transfer full) (array length=length): the image data, or %NULL; free with g_free()
 *
 * Since: 0.8.0
 */
guint8 *
gdata_contacts_contact_get_photo_finish (GDataContactsContact *self, GAsyncResult *async_result, gsize *length, gchar **content_type, GError **error)
{
	g_autoptr(PhotoData) data = NULL;

	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_return_val_if_fail (length != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);
	g_return_val_if_fail (g_task_is_valid (async_result, self), NULL);
	g_return_val_if_fail (g_async_result_is_tagged (async_result, gdata_contacts_contact_get_photo_async), NULL);

	data = g_task_propagate_pointer (G_TASK (async_result), error);
	if (data == NULL) {
		/* Error */
		*length = 0;

		if (content_type != NULL) {
			*content_type = NULL;
		}

		return NULL;
	}

	/* Return the photo (steal the data from the PhotoData struct so we don't have to copy it again) */
	*length = data->length;
	if (content_type != NULL)
		*content_type = g_steal_pointer (&data->content_type);

	return g_steal_pointer (&data->data);
}

/**
 * gdata_contacts_contact_set_photo:
 * @self: a #GDataContactsContact
 * @service: a #GDataContactsService
 * @data: (allow-none): the image data, or %NULL
 * @length: the image length, in bytes
 * @content_type: (allow-none): the content type of the image, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Sets the contact's photo to @data or, if @data is %NULL, deletes the contact's photo. @content_type must be specified if @data is non-%NULL.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * If there is an error setting the photo, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned.
 *
 * Return value: %TRUE on success, %FALSE otherwise
 *
 * Since: 0.8.0
 */
gboolean
gdata_contacts_contact_set_photo (GDataContactsContact *self, GDataContactsService *service, const guint8 *data, gsize length,
                                  const gchar *content_type, GCancellable *cancellable, GError **error)
{
	GDataLink *_link;
	SoupMessage *message;
	guint status;
	gboolean deleting_photo = FALSE;
	const gchar *etag;

	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), FALSE);
	g_return_val_if_fail (GDATA_IS_CONTACTS_SERVICE (service), FALSE);
	g_return_val_if_fail (data == NULL || content_type != NULL, FALSE);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	if (self->priv->photo_etag != NULL && data == NULL)
		deleting_photo = TRUE;

	/* Get the photo URI */
	_link = gdata_entry_look_up_link (GDATA_ENTRY (self), "http://schemas.google.com/contacts/2008/rel#photo");
	g_assert (_link != NULL);

	/* We always have to set an If-Match header. */
	etag = self->priv->photo_etag;
	if (etag == NULL || *etag == '\0') {
		etag = "*";
	}

	message = _gdata_service_build_message (GDATA_SERVICE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                        (deleting_photo == TRUE) ? SOUP_METHOD_DELETE : SOUP_METHOD_PUT,
	                                        gdata_link_get_uri (_link), etag, TRUE);

	/* Append the data */
	if (deleting_photo == FALSE)
		soup_message_set_request (message, content_type, SOUP_MEMORY_STATIC, (gchar*) data, length);

	/* Send the message */
	status = _gdata_service_send_message (GDATA_SERVICE (service), message, cancellable, error);

	if (status == SOUP_STATUS_NONE || status == SOUP_STATUS_CANCELLED) {
		/* Redirect error or cancelled */
		g_object_unref (message);
		return FALSE;
	} else if (status != SOUP_STATUS_OK) {
		/* Error */
		GDataServiceClass *klass = GDATA_SERVICE_GET_CLASS (service);
		g_assert (klass->parse_error_response != NULL);
		klass->parse_error_response (GDATA_SERVICE (service), GDATA_OPERATION_UPLOAD, status, message->reason_phrase,
		                             message->response_body->data, message->response_body->length, error);
		g_object_unref (message);
		return FALSE;
	}

	/* Update the stored photo ETag */
	g_free (self->priv->photo_etag);
	self->priv->photo_etag = g_strdup (soup_message_headers_get_one (message->response_headers, "ETag"));
	g_object_notify (G_OBJECT (self), "photo-etag");

	g_object_unref (message);

	return TRUE;
}

static void
set_photo_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	GDataContactsContact *contact = GDATA_CONTACTS_CONTACT (source_object);
	PhotoData *data = task_data;
	g_autoptr(GError) error = NULL;

	/* Set the photo */
	if (!gdata_contacts_contact_set_photo (contact, data->service, data->data, data->length, data->content_type, cancellable, &error))
		g_task_return_error (task, g_steal_pointer (&error));
	else
		g_task_return_boolean (task, TRUE);
}

/**
 * gdata_contacts_contact_set_photo_async:
 * @self: a #GDataContactsContact
 * @service: a #GDataContactsService
 * @data: (allow-none): the image data, or %NULL
 * @length: the image length, in bytes
 * @content_type: (allow-none): the content type of the image, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the photo has been set, or %NULL
 * @user_data: (closure): data to pass to the @callback function
 *
 * Sets the contact's photo to @data or, if @data is %NULL, deletes the contact's photo. @content_type must be specified if @data is non-%NULL. @self,
 * @service, @data and @content_type are all reffed and copied when this function is called, so can safely be unreffed after this function returns.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_contacts_contact_set_photo_finish() to get the results of the
 * operation.
 *
 * For more details, see gdata_contacts_contact_set_photo(), which is the synchronous version of this function.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled, the error %G_IO_ERROR_CANCELLED will be returned by gdata_contacts_contact_set_photo_finish().
 *
 * If there is an error setting the photo, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned by gdata_contacts_contact_set_photo_finish().
 *
 * Since: 0.8.0
 */
void
gdata_contacts_contact_set_photo_async (GDataContactsContact *self, GDataContactsService *service, const guint8 *data, gsize length,
                                        const gchar *content_type, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	g_autoptr(GTask) task = NULL;
	g_autoptr(PhotoData) photo_data = NULL;

	g_return_if_fail (GDATA_IS_CONTACTS_CONTACT (self));
	g_return_if_fail (GDATA_IS_CONTACTS_SERVICE (service));
	g_return_if_fail (data == NULL || content_type != NULL);
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	/* Prepare the data to be passed to the thread */
	photo_data = g_slice_new (PhotoData);
	photo_data->service = g_object_ref (service);
	photo_data->data = g_memdup (data, length);
	photo_data->length = length;
	photo_data->content_type = g_strdup (content_type);

	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_contacts_contact_set_photo_async);
	g_task_set_task_data (task, g_steal_pointer (&photo_data), (GDestroyNotify) photo_data_free);
	g_task_run_in_thread (task, set_photo_thread);
}

/**
 * gdata_contacts_contact_set_photo_finish:
 * @self: a #GDataContactsContact
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous contact photo setting operation started with gdata_contacts_contact_set_photo_async().
 *
 * If there is an error setting the photo, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned.
 *
 * Return value: %TRUE on success, %FALSE otherwise
 *
 * Since: 0.8.0
 */
gboolean
gdata_contacts_contact_set_photo_finish (GDataContactsContact *self, GAsyncResult *async_result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_CONTACT (self), FALSE);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	g_return_val_if_fail (g_task_is_valid (async_result, self), FALSE);
	g_return_val_if_fail (g_async_result_is_tagged (async_result, gdata_contacts_contact_set_photo_async), FALSE);

	return g_task_propagate_boolean (G_TASK (async_result), error);
}
