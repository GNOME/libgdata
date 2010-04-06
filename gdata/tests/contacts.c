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

#include <glib.h>
#include <unistd.h>
#include <string.h>

#include "gdata.h"
#include "common.h"

static void
check_kind (GDataEntry *entry)
{
	GList *list;
	gboolean has_kind = FALSE;

	/* Check the contact's kind category is present and correct */
	for (list = gdata_entry_get_categories (entry); list != NULL; list = list->next) {
		GDataCategory *category = GDATA_CATEGORY (list->data);

		if (strcmp (gdata_category_get_scheme (category), "http://schemas.google.com/g/2005#kind") == 0) {
			g_assert_cmpstr (gdata_category_get_term (category), ==, "http://schemas.google.com/contact/2008#contact");
			has_kind = TRUE;
		}
	}
	g_assert (has_kind == TRUE);
}

static GDataContactsContact *
get_contact (gconstpointer service)
{
	GDataFeed *feed;
	GDataEntry *entry;
	GList *entries;
	GError *error = NULL;
	static gchar *entry_id = NULL;

	/* Make sure we use the same contact throughout */
	feed = gdata_contacts_service_query_contacts (GDATA_CONTACTS_SERVICE (service), NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	entries = gdata_feed_get_entries (feed);
	g_assert (entries != NULL);
	entry = entries->data;
	g_assert (GDATA_IS_CONTACTS_CONTACT (entry));
	check_kind (entry);

	g_object_ref (entry);
	g_object_unref (feed);

	if (entry_id == NULL)
		entry_id = g_strdup (gdata_entry_get_id (entry));

	return GDATA_CONTACTS_CONTACT (entry);
}

static void
test_authentication (void)
{
	gboolean retval;
	GDataService *service;
	GError *error = NULL;

	/* Create a service */
	service = GDATA_SERVICE (gdata_contacts_service_new (CLIENT_ID));

	g_assert (service != NULL);
	g_assert (GDATA_IS_SERVICE (service));
	g_assert_cmpstr (gdata_service_get_client_id (service), ==, CLIENT_ID);

	/* Log in */
	retval = gdata_service_authenticate (service, USERNAME, PASSWORD, NULL, &error);
	g_assert_no_error (error);
	g_assert (retval == TRUE);
	g_clear_error (&error);

	/* Check all is as it should be */
	g_assert (gdata_service_is_authenticated (service) == TRUE);
	g_assert_cmpstr (gdata_service_get_username (service), ==, USERNAME);
	g_assert_cmpstr (gdata_service_get_password (service), ==, PASSWORD);

	g_object_unref (service);
}

static void
test_query_all_contacts (gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;

	feed = gdata_contacts_service_query_contacts (GDATA_CONTACTS_SERVICE (service), NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* TODO: check entries, kinds and feed properties */

	g_object_unref (feed);
}

static void
test_query_all_contacts_async_cb (GDataService *service, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GDataFeed *feed;
	GError *error = NULL;

	feed = gdata_service_query_finish (service, async_result, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* TODO: Tests? */
	g_main_loop_quit (main_loop);

	g_object_unref (feed);
}

static void
test_query_all_contacts_async (gconstpointer service)
{
	GMainLoop *main_loop = g_main_loop_new (NULL, TRUE);

	gdata_contacts_service_query_contacts_async (GDATA_CONTACTS_SERVICE (service), NULL, NULL, NULL,
						     NULL, (GAsyncReadyCallback) test_query_all_contacts_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
}

static void
test_insert_simple (gconstpointer service)
{
	GDataContactsContact *contact, *new_contact;
	GDataGDName *name, *name2;
	GDataGDEmailAddress *email_address1, *email_address2;
	GDataGDPhoneNumber *phone_number1, *phone_number2;
	GDataGDIMAddress *im_address;
	GDataGDOrganization *org;
	GDataGDPostalAddress *postal_address;
	GDataGContactJot *jot;
	GDataGContactRelation *relation;
	GDataGContactWebsite *website;
	GDataGContactEvent *event;
	GDataGContactCalendar *calendar;
	gchar *xml, *nickname, *billing_information, *directory_server, *gender, *initials, *maiden_name, *mileage, *occupation;
	gchar *priority, *sensitivity, *short_name, *subject;
	GList *list;
	GDate date, *date2;
	GHashTable *properties;
	GTimeVal *edited, creation_time;
	gboolean deleted, has_photo, birthday_has_year;
	GError *error = NULL;

	contact = gdata_contacts_contact_new (NULL);
	g_get_current_time (&creation_time);

	/* Check the kind is present and correct */
	g_assert (GDATA_IS_CONTACTS_CONTACT (contact));
	check_kind (GDATA_ENTRY (contact));

	/* Set and check the name (to check if the title of the entry is updated) */
	gdata_entry_set_title (GDATA_ENTRY (contact), "Elizabeth Bennet");
	name = gdata_contacts_contact_get_name (contact);
	g_assert_cmpstr (gdata_gd_name_get_full_name (name), ==, "Elizabeth Bennet");
	gdata_gd_name_set_full_name (name, "Lizzie Bennet");
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (contact)), ==, "Lizzie Bennet");

	name2 = gdata_gd_name_new ("John", "Smith");
	gdata_gd_name_set_full_name (name2, "John Smith");
	gdata_contacts_contact_set_name (contact, name2);
	g_object_unref (name2);
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (contact)), ==, "John Smith");

	gdata_contacts_contact_set_nickname (contact, "Big J");
	g_date_set_dmy (&date, 1, 1, 1900);
	gdata_contacts_contact_set_birthday (contact, &date, FALSE);
	gdata_entry_set_content (GDATA_ENTRY (contact), "Notes");
	gdata_contacts_contact_set_billing_information (contact, "Big J Enterprises, Ltd.");
	gdata_contacts_contact_set_directory_server (contact, "This is a server");
	gdata_contacts_contact_set_gender (contact, GDATA_CONTACTS_GENDER_MALE);
	gdata_contacts_contact_set_initials (contact, "A. B. C.");
	gdata_contacts_contact_set_maiden_name (contact, "Smith");
	gdata_contacts_contact_set_mileage (contact, "12km");
	gdata_contacts_contact_set_occupation (contact, "Professional bum");
	gdata_contacts_contact_set_priority (contact, GDATA_CONTACTS_PRIORITY_HIGH);
	gdata_contacts_contact_set_sensitivity (contact, GDATA_CONTACTS_SENSITIVITY_PERSONAL);
	gdata_contacts_contact_set_short_name (contact, "Jon");
	gdata_contacts_contact_set_subject (contact, "Charity work");

	email_address1 = gdata_gd_email_address_new ("liz@gmail.com", GDATA_GD_EMAIL_ADDRESS_WORK, NULL, FALSE);
	gdata_contacts_contact_add_email_address (contact, email_address1);
	g_object_unref (email_address1);

	email_address2 = gdata_gd_email_address_new ("liz@example.org", GDATA_GD_EMAIL_ADDRESS_HOME, NULL, FALSE);
	gdata_contacts_contact_add_email_address (contact, email_address2);
	g_object_unref (email_address2);

	phone_number1 = gdata_gd_phone_number_new ("(206)555-1212", GDATA_GD_PHONE_NUMBER_WORK, NULL, NULL, TRUE);
	gdata_contacts_contact_add_phone_number (contact, phone_number1);
	g_object_unref (phone_number1);

	phone_number2 = gdata_gd_phone_number_new ("(206)555-1213", GDATA_GD_PHONE_NUMBER_HOME, NULL, NULL, FALSE);
	gdata_contacts_contact_add_phone_number (contact, phone_number2);
	g_object_unref (phone_number2);

	im_address = gdata_gd_im_address_new ("liz@gmail.com", GDATA_GD_IM_PROTOCOL_GOOGLE_TALK, GDATA_GD_IM_ADDRESS_HOME, NULL, FALSE);
	gdata_contacts_contact_add_im_address (contact, im_address);
	g_object_unref (im_address);

	postal_address = gdata_gd_postal_address_new (GDATA_GD_POSTAL_ADDRESS_WORK, NULL, TRUE);
	gdata_gd_postal_address_set_street (postal_address, "1600 Amphitheatre Pkwy Mountain View");
	gdata_contacts_contact_add_postal_address (contact, postal_address);
	g_object_unref (postal_address);

	org = gdata_gd_organization_new ("OrgCorp", "President", GDATA_GD_ORGANIZATION_WORK, NULL, FALSE);
	gdata_contacts_contact_add_organization (contact, org);
	g_object_unref (org);

	jot = gdata_gcontact_jot_new ("This is a jot.", GDATA_GCONTACT_JOT_OTHER);
	gdata_contacts_contact_add_jot (contact, jot);
	g_object_unref (jot);

	relation = gdata_gcontact_relation_new ("Brian Haddock", GDATA_GCONTACT_RELATION_FRIEND, NULL);
	gdata_contacts_contact_add_relation (contact, relation);
	g_object_unref (relation);

	website = gdata_gcontact_website_new ("http://example.com/", GDATA_GCONTACT_WEBSITE_PROFILE, NULL, TRUE);
	gdata_contacts_contact_add_website (contact, website);
	g_object_unref (website);

	event = gdata_gcontact_event_new (&date, GDATA_GCONTACT_EVENT_ANNIVERSARY, NULL);
	gdata_contacts_contact_add_event (contact, event);
	g_object_unref (event);

	calendar = gdata_gcontact_calendar_new ("http://calendar.example.com/", GDATA_GCONTACT_CALENDAR_HOME, NULL, TRUE);
	gdata_contacts_contact_add_calendar (contact, calendar);
	g_object_unref (calendar);

	/* Add some extended properties */
	g_assert (gdata_contacts_contact_set_extended_property (contact, "TITLE", NULL) == TRUE);
	g_assert (gdata_contacts_contact_set_extended_property (contact, "ROLE", "") == TRUE);
	g_assert (gdata_contacts_contact_set_extended_property (contact, "CALURI", "http://example.com/") == TRUE);

	/* Check the properties of the object */
	g_object_get (G_OBJECT (contact),
	              "edited", &edited,
	              "deleted", &deleted,
	              "has-photo", &has_photo,
	              "name", &name,
	              "nickname", &nickname,
	              "birthday", &date2,
	              "birthday-has-year", &birthday_has_year,
	              "billing-information", &billing_information,
	              "directory-server", &directory_server,
	              "gender", &gender,
	              "initials", &initials,
	              "maiden-name", &maiden_name,
	              "mileage", &mileage,
	              "occupation", &occupation,
	              "priority", &priority,
	              "sensitivity", &sensitivity,
	              "short-name", &short_name,
	              "subject", &subject,
	              NULL);

	g_assert_cmpint (edited->tv_sec, ==, creation_time.tv_sec);
	/*g_assert_cmpint (edited->tv_usec, ==, creation_time.tv_usec); --- testing to the nearest microsecond is too precise, and always fails */
	g_assert (deleted == FALSE);
	g_assert (has_photo == FALSE);
	g_assert (name2 == name);
	g_assert_cmpstr (nickname, ==, "Big J");
	g_assert (g_date_valid (date2) == TRUE);
	g_assert_cmpuint (g_date_get_month (date2), ==, 1);
	g_assert_cmpuint (g_date_get_day (date2), ==, 1);
	g_assert (birthday_has_year == FALSE);
	g_assert_cmpstr (billing_information, ==, "Big J Enterprises, Ltd.");
	g_assert_cmpstr (directory_server, ==, "This is a server");
	g_assert_cmpstr (gender, ==, GDATA_CONTACTS_GENDER_MALE);
	g_assert_cmpstr (initials, ==, "A. B. C.");
	g_assert_cmpstr (maiden_name, ==, "Smith");
	g_assert_cmpstr (mileage, ==, "12km");
	g_assert_cmpstr (occupation, ==, "Professional bum");
	g_assert_cmpstr (priority, ==, GDATA_CONTACTS_PRIORITY_HIGH);
	g_assert_cmpstr (sensitivity, ==, GDATA_CONTACTS_SENSITIVITY_PERSONAL);
	g_assert_cmpstr (short_name, ==, "Jon");
	g_assert_cmpstr (subject, ==, "Charity work");

	g_object_unref (name2);
	g_free (date2);
	g_free (nickname);
	g_free (billing_information);
	g_free (directory_server);
	g_free (gender);
	g_free (initials);
	g_free (maiden_name);
	g_free (mileage);
	g_free (occupation);
	g_free (priority);
	g_free (sensitivity);
	g_free (short_name);
	g_free (subject);

	/* Check the XML */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (contact));
	g_assert_cmpstr (xml, ==,
			 "<entry xmlns='http://www.w3.org/2005/Atom' "
				"xmlns:gd='http://schemas.google.com/g/2005' "
				"xmlns:app='http://www.w3.org/2007/app' "
				"xmlns:gContact='http://schemas.google.com/contact/2008'>"
				"<title type='text'>John Smith</title>"
				"<content type='text'>Notes</content>"
				"<category term='http://schemas.google.com/contact/2008#contact' scheme='http://schemas.google.com/g/2005#kind'/>"
				"<gd:name>"
					"<gd:givenName>John</gd:givenName>"
					"<gd:familyName>Smith</gd:familyName>"
					"<gd:fullName>John Smith</gd:fullName>"
				"</gd:name>"
				"<gd:email address='liz@gmail.com' rel='http://schemas.google.com/g/2005#work' primary='false'/>"
				"<gd:email address='liz@example.org' rel='http://schemas.google.com/g/2005#home' primary='false'/>"
				"<gd:im address='liz@gmail.com' protocol='http://schemas.google.com/g/2005#GOOGLE_TALK' "
					"rel='http://schemas.google.com/g/2005#home' primary='false'/>"
				"<gd:phoneNumber rel='http://schemas.google.com/g/2005#work' primary='true'>(206)555-1212</gd:phoneNumber>"
				"<gd:phoneNumber rel='http://schemas.google.com/g/2005#home' primary='false'>(206)555-1213</gd:phoneNumber>"
				"<gd:structuredPostalAddress rel='http://schemas.google.com/g/2005#work' primary='true'>"
					"<gd:street>1600 Amphitheatre Pkwy Mountain View</gd:street>"
				"</gd:structuredPostalAddress>"
				"<gd:organization rel='http://schemas.google.com/g/2005#work' primary='false'>"
					"<gd:orgName>OrgCorp</gd:orgName>"
					"<gd:orgTitle>President</gd:orgTitle>"
				"</gd:organization>"
				"<gContact:jot rel='other'>This is a jot.</gContact:jot>"
				"<gContact:relation rel='friend'>Brian Haddock</gContact:relation>"
				"<gContact:website href='http://example.com/' rel='profile' primary='true'/>"
				"<gContact:event rel='anniversary'><gd:when startTime='1900-01-01'/></gContact:event>"
				"<gContact:calendarLink href='http://calendar.example.com/' rel='home' primary='true'/>"
				"<gd:extendedProperty name='CALURI'>http://example.com/</gd:extendedProperty>"
				"<gContact:nickname>Big J</gContact:nickname>"
				"<gContact:birthday when='--01-01'/>"
				"<gContact:billingInformation>Big J Enterprises, Ltd.</gContact:billingInformation>"
				"<gContact:directoryServer>This is a server</gContact:directoryServer>"
				"<gContact:gender value='male'/>"
				"<gContact:initials>A. B. C.</gContact:initials>"
				"<gContact:maidenName>Smith</gContact:maidenName>"
				"<gContact:mileage>12km</gContact:mileage>"
				"<gContact:occupation>Professional bum</gContact:occupation>"
				"<gContact:priority rel='high'/>"
				"<gContact:sensitivity rel='personal'/>"
				"<gContact:shortName>Jon</gContact:shortName>"
				"<gContact:subject>Charity work</gContact:subject>"
			 "</entry>");
	g_free (xml);

	/* Insert the contact */
	new_contact = gdata_contacts_service_insert_contact (GDATA_CONTACTS_SERVICE (service), contact, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CONTACTS_CONTACT (new_contact));
	check_kind (GDATA_ENTRY (new_contact));
	g_clear_error (&error);

	/* Check its edited date */
	gdata_contacts_contact_get_edited (new_contact, &creation_time);
	g_assert_cmpint (creation_time.tv_sec, >=, edited->tv_sec);

	/* Various properties */
	g_assert_cmpstr (gdata_contacts_contact_get_nickname (new_contact), ==, "Big J");
	g_assert (gdata_contacts_contact_get_birthday (new_contact, &date) == FALSE);
	g_assert (g_date_valid (&date) == TRUE);
	g_assert_cmpuint (g_date_get_month (&date), ==, 1);
	g_assert_cmpuint (g_date_get_day (&date), ==, 1);
	g_assert_cmpstr (gdata_contacts_contact_get_billing_information (new_contact), ==, "Big J Enterprises, Ltd.");
	g_assert_cmpstr (gdata_contacts_contact_get_directory_server (new_contact), ==, "This is a server");
	g_assert_cmpstr (gdata_contacts_contact_get_gender (new_contact), ==, GDATA_CONTACTS_GENDER_MALE);
	g_assert_cmpstr (gdata_contacts_contact_get_initials (new_contact), ==, "A. B. C.");
	g_assert_cmpstr (gdata_contacts_contact_get_maiden_name (new_contact), ==, "Smith");
	g_assert_cmpstr (gdata_contacts_contact_get_mileage (new_contact), ==, "12km");
	g_assert_cmpstr (gdata_contacts_contact_get_occupation (new_contact), ==, "Professional bum");
	g_assert_cmpstr (gdata_contacts_contact_get_priority (new_contact), ==, GDATA_CONTACTS_PRIORITY_HIGH);
	g_assert_cmpstr (gdata_contacts_contact_get_sensitivity (new_contact), ==, GDATA_CONTACTS_SENSITIVITY_PERSONAL);
	g_assert_cmpstr (gdata_contacts_contact_get_short_name (new_contact), ==, "Jon");
	g_assert_cmpstr (gdata_contacts_contact_get_subject (new_contact), ==, "Charity work");

	/* E-mail addresses */
	list = gdata_contacts_contact_get_email_addresses (new_contact);
	g_assert_cmpuint (g_list_length (list), ==, 2);
	g_assert (GDATA_IS_GD_EMAIL_ADDRESS (list->data));

	g_assert (gdata_contacts_contact_get_primary_email_address (new_contact) == NULL);

	/* IM addresses */
	list = gdata_contacts_contact_get_im_addresses (new_contact);
	g_assert_cmpuint (g_list_length (list), ==, 1);
	g_assert (GDATA_IS_GD_IM_ADDRESS (list->data));

	g_assert (gdata_contacts_contact_get_primary_im_address (new_contact) == NULL);

	/* Phone numbers */
	list = gdata_contacts_contact_get_phone_numbers (new_contact);
	g_assert_cmpuint (g_list_length (list), ==, 2);
	g_assert (GDATA_IS_GD_PHONE_NUMBER (list->data));

	g_assert (GDATA_IS_GD_PHONE_NUMBER (gdata_contacts_contact_get_primary_phone_number (new_contact)));

	/* Postal addresses */
	list = gdata_contacts_contact_get_postal_addresses (new_contact);
	g_assert_cmpuint (g_list_length (list), ==, 1);
	g_assert (GDATA_IS_GD_POSTAL_ADDRESS (list->data));

	g_assert (GDATA_IS_GD_POSTAL_ADDRESS (gdata_contacts_contact_get_primary_postal_address (new_contact)));

	/* Organizations */
	list = gdata_contacts_contact_get_organizations (new_contact);
	g_assert_cmpuint (g_list_length (list), ==, 1);
	g_assert (GDATA_IS_GD_ORGANIZATION (list->data));

	g_assert (gdata_contacts_contact_get_primary_organization (new_contact) == NULL);

	/* Jots */
	list = gdata_contacts_contact_get_jots (new_contact);
	g_assert_cmpuint (g_list_length (list), ==, 1);
	g_assert (GDATA_IS_GCONTACT_JOT (list->data));

	/* Relations */
	list = gdata_contacts_contact_get_relations (new_contact);
	g_assert_cmpuint (g_list_length (list), ==, 1);
	g_assert (GDATA_IS_GCONTACT_RELATION (list->data));

	/* Websites */
	list = gdata_contacts_contact_get_websites (new_contact);
	g_assert_cmpuint (g_list_length (list), ==, 1);
	g_assert (GDATA_IS_GCONTACT_WEBSITE (list->data));

	g_assert (GDATA_IS_GCONTACT_WEBSITE (gdata_contacts_contact_get_primary_website (new_contact)));

	/* Events */
	list = gdata_contacts_contact_get_events (new_contact);
	g_assert_cmpuint (g_list_length (list), ==, 1);
	g_assert (GDATA_IS_GCONTACT_EVENT (list->data));

	/* Calendars */
	list = gdata_contacts_contact_get_calendars (new_contact);
	g_assert_cmpuint (g_list_length (list), ==, 1);
	g_assert (GDATA_IS_GCONTACT_CALENDAR (list->data));

	g_assert (GDATA_IS_GCONTACT_CALENDAR (gdata_contacts_contact_get_primary_calendar (new_contact)));

	/* Extended properties */
	g_assert_cmpstr (gdata_contacts_contact_get_extended_property (new_contact, "CALURI"), ==, "http://example.com/");
	g_assert (gdata_contacts_contact_get_extended_property (new_contact, "non-existent") == NULL);

	properties = gdata_contacts_contact_get_extended_properties (new_contact);
	g_assert (properties != NULL);
	g_assert_cmpuint (g_hash_table_size (properties), ==, 1);

	/* Groups */
	list = gdata_contacts_contact_get_groups (new_contact);
	g_assert (list == NULL);

	/* Deleted? */
	g_assert (gdata_contacts_contact_is_deleted (new_contact) == FALSE);

	/* TODO: check entries and feed properties */

	/* Try removing some things from the new contact and ensure it works */
	gdata_contacts_contact_remove_all_email_addresses (new_contact);
	g_assert (gdata_contacts_contact_get_email_addresses (new_contact) == NULL);
	g_assert (gdata_contacts_contact_get_primary_email_address (new_contact) == NULL);

	gdata_contacts_contact_remove_all_im_addresses (new_contact);
	g_assert (gdata_contacts_contact_get_im_addresses (new_contact) == NULL);
	g_assert (gdata_contacts_contact_get_primary_im_address (new_contact) == NULL);

	gdata_contacts_contact_remove_all_phone_numbers (new_contact);
	g_assert (gdata_contacts_contact_get_phone_numbers (new_contact) == NULL);
	g_assert (gdata_contacts_contact_get_primary_phone_number (new_contact) == NULL);

	gdata_contacts_contact_remove_all_postal_addresses (new_contact);
	g_assert (gdata_contacts_contact_get_postal_addresses (new_contact) == NULL);
	g_assert (gdata_contacts_contact_get_primary_postal_address (new_contact) == NULL);

	gdata_contacts_contact_remove_all_organizations (new_contact);
	g_assert (gdata_contacts_contact_get_organizations (new_contact) == NULL);
	g_assert (gdata_contacts_contact_get_primary_organization (new_contact) == NULL);

	gdata_contacts_contact_remove_all_jots (new_contact);
	g_assert (gdata_contacts_contact_get_jots (new_contact) == NULL);

	gdata_contacts_contact_remove_all_relations (new_contact);
	g_assert (gdata_contacts_contact_get_relations (new_contact) == NULL);

	gdata_contacts_contact_remove_all_websites (new_contact);
	g_assert (gdata_contacts_contact_get_websites (new_contact) == NULL);
	g_assert (gdata_contacts_contact_get_primary_website (new_contact) == NULL);

	gdata_contacts_contact_remove_all_events (new_contact);
	g_assert (gdata_contacts_contact_get_events (new_contact) == NULL);

	gdata_contacts_contact_remove_all_calendars (new_contact);
	g_assert (gdata_contacts_contact_get_calendars (new_contact) == NULL);
	g_assert (gdata_contacts_contact_get_primary_calendar (new_contact) == NULL);

	g_free (edited);
	g_object_unref (contact);
	g_object_unref (new_contact);
}

static void
test_query_uri (void)
{
	gchar *query_uri;
	GDataContactsQuery *query = gdata_contacts_query_new ("q");

	gdata_contacts_query_set_order_by (query, "lastmodified");
	g_assert_cmpstr (gdata_contacts_query_get_order_by (query), ==, "lastmodified");

	gdata_contacts_query_set_show_deleted (query, FALSE);
	g_assert (gdata_contacts_query_show_deleted (query) == FALSE);

	/* Test it with both values of show-deleted */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&orderby=lastmodified&showdeleted=false");
	g_free (query_uri);

	gdata_contacts_query_set_show_deleted (query, TRUE);
	g_assert (gdata_contacts_query_show_deleted (query) == TRUE);

	gdata_contacts_query_set_sort_order (query, "descending");
	g_assert_cmpstr (gdata_contacts_query_get_sort_order (query), ==, "descending");

	gdata_contacts_query_set_group (query, "http://www.google.com/feeds/contacts/groups/jo@gmail.com/base/1234a");
	g_assert_cmpstr (gdata_contacts_query_get_group (query), ==, "http://www.google.com/feeds/contacts/groups/jo@gmail.com/base/1234a");

	/* Check the built query URI with a normal feed URI */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&orderby=lastmodified&showdeleted=true&sortorder=descending"
					"&group=http%3A%2F%2Fwww.google.com%2Ffeeds%2Fcontacts%2Fgroups%2Fjo%40gmail.com%2Fbase%2F1234a");
	g_free (query_uri);

	/* …with a feed URI with a trailing slash */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com/");
	g_assert_cmpstr (query_uri, ==, "http://example.com/?q=q&orderby=lastmodified&showdeleted=true&sortorder=descending"
					"&group=http%3A%2F%2Fwww.google.com%2Ffeeds%2Fcontacts%2Fgroups%2Fjo%40gmail.com%2Fbase%2F1234a");
	g_free (query_uri);

	/* …with a feed URI with pre-existing arguments */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com/bar/?test=test&this=that");
	g_assert_cmpstr (query_uri, ==, "http://example.com/bar/?test=test&this=that&q=q&orderby=lastmodified&showdeleted=true&sortorder=descending"
					"&group=http%3A%2F%2Fwww.google.com%2Ffeeds%2Fcontacts%2Fgroups%2Fjo%40gmail.com%2Fbase%2F1234a");
	g_free (query_uri);

	g_object_unref (query);
}

static void
test_query_etag (void)
{
	GDataContactsQuery *query = gdata_contacts_query_new (NULL);

	/* Test that setting any property will unset the ETag */
	g_test_bug ("613529");

#define CHECK_ETAG(C) \
	gdata_query_set_etag (GDATA_QUERY (query), "foobar");		\
	(C);								\
	g_assert (gdata_query_get_etag (GDATA_QUERY (query)) == NULL);

	CHECK_ETAG (gdata_contacts_query_set_order_by (query, "foobar"))
	CHECK_ETAG (gdata_contacts_query_set_show_deleted (query, FALSE))
	CHECK_ETAG (gdata_contacts_query_set_sort_order (query, "shizzle"))
	CHECK_ETAG (gdata_contacts_query_set_group (query, "support group"))

#undef CHECK_ETAG

	g_object_unref (query);
}

static void
test_query_properties (void)
{
	gchar *order_by, *sort_order, *group;
	gboolean show_deleted;
	guint start_index, max_results;
	GDataContactsQuery *query = gdata_contacts_query_new_with_limits ("q", 1, 10);

	/* Set the properties */
	g_object_set (G_OBJECT (query),
	              "order-by", "lastmodified",
	              "show-deleted", TRUE,
	              "sort-order", "descending",
	              "group", "http://www.google.com/feeds/contacts/groups/jo@gmail.com/base/1234a",
	              NULL);

	/* Check the query's properties */
	g_object_get (G_OBJECT (query),
	              "order-by", &order_by,
	              "show-deleted", &show_deleted,
	              "sort-order", &sort_order,
	              "group", &group,
	              "start-index", &start_index,
	              "max-results", &max_results,
	              NULL);

	g_assert_cmpstr (order_by, ==, "lastmodified");
	g_assert (show_deleted == TRUE);
	g_assert_cmpstr (sort_order, ==, "descending");
	g_assert_cmpstr (group, ==, "http://www.google.com/feeds/contacts/groups/jo@gmail.com/base/1234a");
	g_assert_cmpuint (start_index, ==, 1);
	g_assert_cmpuint (max_results, ==, 10);

	g_free (order_by);
	g_free (sort_order);
	g_free (group);

	g_object_unref (query);
}

static void
test_parser_minimal (gconstpointer service)
{
	GDataContactsContact *contact;
	GDate birthday;
	GError *error = NULL;

	g_test_bug ("580330");

	contact = GDATA_CONTACTS_CONTACT (gdata_parsable_new_from_xml (GDATA_TYPE_CONTACTS_CONTACT,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
			"xmlns:gd='http://schemas.google.com/g/2005' "
			"gd:etag='&quot;QngzcDVSLyp7ImA9WxJTFkoITgU.&quot;'>"
			"<id>http://www.google.com/m8/feeds/contacts/libgdata.test@googlemail.com/base/1b46cdd20bfbee3b</id>"
			"<updated>2009-04-25T15:21:53.688Z</updated>"
			"<app:edited xmlns:app='http://www.w3.org/2007/app'>2009-04-25T15:21:53.688Z</app:edited>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://schemas.google.com/contact/2008#contact'/>"
			"<title></title>" /* Here's where it all went wrong */
			"<link rel='http://schemas.google.com/contacts/2008/rel#photo' type='image/*' href='http://www.google.com/m8/feeds/photos/media/libgdata.test@googlemail.com/1b46cdd20bfbee3b'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' href='http://www.google.com/m8/feeds/contacts/libgdata.test@googlemail.com/full/1b46cdd20bfbee3b'/>"
			"<link rel='http://www.iana.org/assignments/relation/edit' type='application/atom+xml' href='http://www.google.com/m8/feeds/contacts/libgdata.test@googlemail.com/full/1b46cdd20bfbee3b'/>"
			"<gd:email rel='http://schemas.google.com/g/2005#other' address='bob@example.com'/>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_CONTACTS_CONTACT (contact));
	check_kind (GDATA_ENTRY (contact));
	g_clear_error (&error);

	/* Check the contact's properties */
	g_assert (gdata_entry_get_title (GDATA_ENTRY (contact)) != NULL);
	g_assert (*gdata_entry_get_title (GDATA_ENTRY (contact)) == '\0');

	/* TODO: Check the other properties */

	g_assert (gdata_contacts_contact_get_nickname (contact) == NULL);
	g_assert (gdata_contacts_contact_get_birthday (contact, &birthday) == FALSE);
	g_assert (g_date_valid (&birthday) == FALSE);
	g_assert (gdata_contacts_contact_get_billing_information (contact) == NULL);
	g_assert (gdata_contacts_contact_get_directory_server (contact) == NULL);
	g_assert (gdata_contacts_contact_get_gender (contact) == NULL);
	g_assert (gdata_contacts_contact_get_initials (contact) == NULL);
	g_assert (gdata_contacts_contact_get_maiden_name (contact) == NULL);
	g_assert (gdata_contacts_contact_get_mileage (contact) == NULL);
	g_assert (gdata_contacts_contact_get_occupation (contact) == NULL);
	g_assert (gdata_contacts_contact_get_priority (contact) == NULL);
	g_assert (gdata_contacts_contact_get_sensitivity (contact) == NULL);
	g_assert (gdata_contacts_contact_get_short_name (contact) == NULL);
	g_assert (gdata_contacts_contact_get_subject (contact) == NULL);
	g_assert (gdata_contacts_contact_get_jots (contact) == NULL);
	g_assert (gdata_contacts_contact_get_relations (contact) == NULL);
	g_assert (gdata_contacts_contact_get_websites (contact) == NULL);
	g_assert (gdata_contacts_contact_get_primary_website (contact) == NULL);
	g_assert (gdata_contacts_contact_get_events (contact) == NULL);
	g_assert (gdata_contacts_contact_get_calendars (contact) == NULL);
	g_assert (gdata_contacts_contact_get_primary_calendar (contact) == NULL);

	g_object_unref (contact);
}

static void
test_parser_normal (gconstpointer service)
{
	GDataContactsContact *contact;
	GDate date;
	GList *list;
	GError *error = NULL;

	contact = GDATA_CONTACTS_CONTACT (gdata_parsable_new_from_xml (GDATA_TYPE_CONTACTS_CONTACT,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
			"xmlns:gd='http://schemas.google.com/g/2005' "
			"xmlns:gContact='http://schemas.google.com/contact/2008' "
			"gd:etag='&quot;QngzcDVSLyp7ImA9WxJTFkoITgU.&quot;'>"
			"<id>http://www.google.com/m8/feeds/contacts/libgdata.test@googlemail.com/base/1b46cdd20bfbee3b</id>"
			"<updated>2009-04-25T15:21:53.688Z</updated>"
			"<app:edited xmlns:app='http://www.w3.org/2007/app'>2009-04-25T15:21:53.688Z</app:edited>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://schemas.google.com/contact/2008#contact'/>"
			"<title></title>" /* Here's where it all went wrong */
			"<link rel='http://schemas.google.com/contacts/2008/rel#photo' type='image/*' href='http://www.google.com/m8/feeds/photos/media/libgdata.test@googlemail.com/1b46cdd20bfbee3b'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' href='http://www.google.com/m8/feeds/contacts/libgdata.test@googlemail.com/full/1b46cdd20bfbee3b'/>"
			"<link rel='http://www.iana.org/assignments/relation/edit' type='application/atom+xml' href='http://www.google.com/m8/feeds/contacts/libgdata.test@googlemail.com/full/1b46cdd20bfbee3b'/>"
			"<gd:email rel='http://schemas.google.com/g/2005#other' address='bob@example.com'/>"
			"<gd:extendedProperty name='test' value='test value'/>"
			"<gd:organization rel='http://schemas.google.com/g/2005#work' label='Work' primary='true'/>"
			"<gContact:groupMembershipInfo href='http://www.google.com/feeds/contacts/groups/jo%40gmail.com/base/1234a' "
				"deleted='true'/>"
			"<gContact:groupMembershipInfo href='http://www.google.com/feeds/contacts/groups/jo%40gmail.com/base/1234b'/>"
			"<gd:deleted/>"
			"<gContact:nickname>Agent Smith</gContact:nickname>"
			"<gContact:birthday when='2010-12-03'/>"
			"<gContact:billingInformation>Foo &amp; Bar Inc.</gContact:billingInformation>"
			"<gContact:directoryServer>Directory &amp; server</gContact:directoryServer>"
			"<gContact:gender value='female'/>"
			"<gContact:initials>X. Y. Z.</gContact:initials>"
			"<gContact:maidenName>Foo</gContact:maidenName>"
			"<gContact:mileage>15km</gContact:mileage>"
			"<gContact:occupation>Occupied</gContact:occupation>"
			"<gContact:priority rel='low'/>"
			"<gContact:sensitivity rel='confidential'/>"
			"<gContact:shortName>Smith</gContact:shortName>"
			"<gContact:subject>Film buddy</gContact:subject>"
			"<gContact:jot rel='home'>Moved house on 2010-02-14 to the North Pole.</gContact:jot>"
			"<gContact:jot rel='user'>Owes me ten pounds.</gContact:jot>"
			"<gContact:jot rel='other'></gContact:jot>" /* Empty on purpose */
			"<gContact:relation rel='father'>Darth Vader</gContact:relation>"
			"<gContact:relation label='Favourite singer'>Rob Halford</gContact:relation>"
			"<gContact:website href='http://example.com' rel='home-page' label='Home tab #1' primary='true'/>"
			"<gContact:website href='http://example.com' rel='work'/>"
			"<gContact:website href='http://bar.com' rel='profile' primary='false'/>"
			"<gContact:event rel='anniversary'><gd:when startTime='2010-03-04'/></gContact:event>"
			"<gContact:event label='Foobar'><gd:when startTime='1900-01-01'/></gContact:event>"
			"<gContact:calendarLink href='http://example.com/' rel='free-busy' primary='true'/>"
			"<gContact:calendarLink href='http://example.com/' label='Gig list' primary='false'/>"
			"<gContact:calendarLink href='http://foo.com/calendar' rel='home'/>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_CONTACTS_CONTACT (contact));
	check_kind (GDATA_ENTRY (contact));
	g_clear_error (&error);

	/* TODO: Check the other properties */

	g_assert_cmpstr (gdata_contacts_contact_get_nickname (contact), ==, "Agent Smith");
	g_assert_cmpstr (gdata_contacts_contact_get_billing_information (contact), ==, "Foo & Bar Inc.");
	g_assert_cmpstr (gdata_contacts_contact_get_directory_server (contact), ==, "Directory & server");
	g_assert_cmpstr (gdata_contacts_contact_get_gender (contact), ==, GDATA_CONTACTS_GENDER_FEMALE);
	g_assert_cmpstr (gdata_contacts_contact_get_initials (contact), ==, "X. Y. Z.");
	g_assert_cmpstr (gdata_contacts_contact_get_maiden_name (contact), ==, "Foo");
	g_assert_cmpstr (gdata_contacts_contact_get_mileage (contact), ==, "15km");
	g_assert_cmpstr (gdata_contacts_contact_get_occupation (contact), ==, "Occupied");
	g_assert_cmpstr (gdata_contacts_contact_get_priority (contact), ==, GDATA_CONTACTS_PRIORITY_LOW);
	g_assert_cmpstr (gdata_contacts_contact_get_sensitivity (contact), ==, GDATA_CONTACTS_SENSITIVITY_CONFIDENTIAL);
	g_assert_cmpstr (gdata_contacts_contact_get_short_name (contact), ==, "Smith");
	g_assert_cmpstr (gdata_contacts_contact_get_subject (contact), ==, "Film buddy");

	/* Birthday */
	g_assert (gdata_contacts_contact_get_birthday (contact, &date) == TRUE);
	g_assert (g_date_valid (&date) == TRUE);
	g_assert_cmpuint (g_date_get_year (&date), ==, 2010);
	g_assert_cmpuint (g_date_get_month (&date), ==, 12);
	g_assert_cmpuint (g_date_get_day (&date), ==, 3);

	/* Jots */
	list = gdata_contacts_contact_get_jots (contact);
	g_assert_cmpuint (g_list_length (list), ==, 3);

	g_assert (GDATA_IS_GCONTACT_JOT (list->data));
	g_assert_cmpstr (gdata_gcontact_jot_get_content (GDATA_GCONTACT_JOT (list->data)), ==, "Moved house on 2010-02-14 to the North Pole.");
	g_assert_cmpstr (gdata_gcontact_jot_get_relation_type (GDATA_GCONTACT_JOT (list->data)), ==, GDATA_GCONTACT_JOT_HOME);

	list = list->next;
	g_assert (GDATA_IS_GCONTACT_JOT (list->data));
	g_assert_cmpstr (gdata_gcontact_jot_get_content (GDATA_GCONTACT_JOT (list->data)), ==, "Owes me ten pounds.");
	g_assert_cmpstr (gdata_gcontact_jot_get_relation_type (GDATA_GCONTACT_JOT (list->data)), ==, GDATA_GCONTACT_JOT_USER);

	list = list->next;
	g_assert (GDATA_IS_GCONTACT_JOT (list->data));
	g_assert (gdata_gcontact_jot_get_content (GDATA_GCONTACT_JOT (list->data)) == NULL);
	g_assert_cmpstr (gdata_gcontact_jot_get_relation_type (GDATA_GCONTACT_JOT (list->data)), ==, GDATA_GCONTACT_JOT_OTHER);

	/* Relations */
	list = gdata_contacts_contact_get_relations (contact);
	g_assert_cmpuint (g_list_length (list), ==, 2);

	g_assert (GDATA_IS_GCONTACT_RELATION (list->data));
	g_assert_cmpstr (gdata_gcontact_relation_get_name (GDATA_GCONTACT_RELATION (list->data)), ==, "Darth Vader");
	g_assert_cmpstr (gdata_gcontact_relation_get_relation_type (GDATA_GCONTACT_RELATION (list->data)), ==, GDATA_GCONTACT_RELATION_FATHER);
	g_assert (gdata_gcontact_relation_get_label (GDATA_GCONTACT_RELATION (list->data)) == NULL);

	list = list->next;
	g_assert (GDATA_IS_GCONTACT_RELATION (list->data));
	g_assert_cmpstr (gdata_gcontact_relation_get_name (GDATA_GCONTACT_RELATION (list->data)), ==, "Rob Halford");
	g_assert (gdata_gcontact_relation_get_relation_type (GDATA_GCONTACT_RELATION (list->data)) == NULL);
	g_assert_cmpstr (gdata_gcontact_relation_get_label (GDATA_GCONTACT_RELATION (list->data)), ==, "Favourite singer");

	/* Websites */
	list = gdata_contacts_contact_get_websites (contact);
	g_assert_cmpuint (g_list_length (list), ==, 3);

	g_assert (GDATA_IS_GCONTACT_WEBSITE (list->data));
	g_assert_cmpstr (gdata_gcontact_website_get_uri (GDATA_GCONTACT_WEBSITE (list->data)), ==, "http://example.com");
	g_assert_cmpstr (gdata_gcontact_website_get_relation_type (GDATA_GCONTACT_WEBSITE (list->data)), ==, GDATA_GCONTACT_WEBSITE_HOME_PAGE);
	g_assert_cmpstr (gdata_gcontact_website_get_label (GDATA_GCONTACT_WEBSITE (list->data)), ==, "Home tab #1");
	g_assert (gdata_gcontact_website_is_primary (GDATA_GCONTACT_WEBSITE (list->data)) == TRUE);

	g_assert (gdata_contacts_contact_get_primary_website (contact) == list->data);

	list = list->next;
	g_assert (GDATA_IS_GCONTACT_WEBSITE (list->data));
	g_assert_cmpstr (gdata_gcontact_website_get_uri (GDATA_GCONTACT_WEBSITE (list->data)), ==, "http://example.com");
	g_assert_cmpstr (gdata_gcontact_website_get_relation_type (GDATA_GCONTACT_WEBSITE (list->data)), ==, GDATA_GCONTACT_WEBSITE_WORK);
	g_assert (gdata_gcontact_website_get_label (GDATA_GCONTACT_WEBSITE (list->data)) == NULL);
	g_assert (gdata_gcontact_website_is_primary (GDATA_GCONTACT_WEBSITE (list->data)) == FALSE);

	list = list->next;
	g_assert (GDATA_IS_GCONTACT_WEBSITE (list->data));
	g_assert_cmpstr (gdata_gcontact_website_get_uri (GDATA_GCONTACT_WEBSITE (list->data)), ==, "http://bar.com");
	g_assert_cmpstr (gdata_gcontact_website_get_relation_type (GDATA_GCONTACT_WEBSITE (list->data)), ==, GDATA_GCONTACT_WEBSITE_PROFILE);
	g_assert (gdata_gcontact_website_get_label (GDATA_GCONTACT_WEBSITE (list->data)) == NULL);
	g_assert (gdata_gcontact_website_is_primary (GDATA_GCONTACT_WEBSITE (list->data)) == FALSE);

	/* Events */
	list = gdata_contacts_contact_get_events (contact);
	g_assert_cmpuint (g_list_length (list), ==, 2);

	g_assert (GDATA_IS_GCONTACT_EVENT (list->data));
	gdata_gcontact_event_get_date (GDATA_GCONTACT_EVENT (list->data), &date);
	g_assert (g_date_valid (&date) == TRUE);
	g_assert_cmpuint (g_date_get_year (&date), ==, 2010);
	g_assert_cmpuint (g_date_get_month (&date), ==, 3);
	g_assert_cmpuint (g_date_get_day (&date), ==, 4);
	g_assert_cmpstr (gdata_gcontact_event_get_relation_type (GDATA_GCONTACT_EVENT (list->data)), ==, GDATA_GCONTACT_EVENT_ANNIVERSARY);
	g_assert (gdata_gcontact_event_get_label (GDATA_GCONTACT_EVENT (list->data)) == NULL);

	list = list->next;
	g_assert (GDATA_IS_GCONTACT_EVENT (list->data));
	gdata_gcontact_event_get_date (GDATA_GCONTACT_EVENT (list->data), &date);
	g_assert (g_date_valid (&date) == TRUE);
	g_assert_cmpuint (g_date_get_year (&date), ==, 1900);
	g_assert_cmpuint (g_date_get_month (&date), ==, 1);
	g_assert_cmpuint (g_date_get_day (&date), ==, 1);
	g_assert (gdata_gcontact_event_get_relation_type (GDATA_GCONTACT_EVENT (list->data)) == NULL);
	g_assert_cmpstr (gdata_gcontact_event_get_label (GDATA_GCONTACT_EVENT (list->data)), ==, "Foobar");

	/* Calendars */
	list = gdata_contacts_contact_get_calendars (contact);
	g_assert_cmpuint (g_list_length (list), ==, 3);

	g_assert (GDATA_IS_GCONTACT_CALENDAR (list->data));
	g_assert_cmpstr (gdata_gcontact_calendar_get_uri (GDATA_GCONTACT_CALENDAR (list->data)), ==, "http://example.com/");
	g_assert_cmpstr (gdata_gcontact_calendar_get_relation_type (GDATA_GCONTACT_CALENDAR (list->data)), ==, GDATA_GCONTACT_CALENDAR_FREE_BUSY);
	g_assert (gdata_gcontact_calendar_get_label (GDATA_GCONTACT_CALENDAR (list->data)) == NULL);
	g_assert (gdata_gcontact_calendar_is_primary (GDATA_GCONTACT_CALENDAR (list->data)) == TRUE);

	g_assert (gdata_contacts_contact_get_primary_calendar (contact) == list->data);

	list = list->next;
	g_assert (GDATA_IS_GCONTACT_CALENDAR (list->data));
	g_assert_cmpstr (gdata_gcontact_calendar_get_uri (GDATA_GCONTACT_CALENDAR (list->data)), ==, "http://example.com/");
	g_assert (gdata_gcontact_calendar_get_relation_type (GDATA_GCONTACT_CALENDAR (list->data)) == NULL);
	g_assert_cmpstr (gdata_gcontact_calendar_get_label (GDATA_GCONTACT_CALENDAR (list->data)), ==, "Gig list");
	g_assert (gdata_gcontact_calendar_is_primary (GDATA_GCONTACT_CALENDAR (list->data)) == FALSE);

	list = list->next;
	g_assert (GDATA_IS_GCONTACT_CALENDAR (list->data));
	g_assert_cmpstr (gdata_gcontact_calendar_get_uri (GDATA_GCONTACT_CALENDAR (list->data)), ==, "http://foo.com/calendar");
	g_assert_cmpstr (gdata_gcontact_calendar_get_relation_type (GDATA_GCONTACT_CALENDAR (list->data)), ==, GDATA_GCONTACT_CALENDAR_HOME);
	g_assert (gdata_gcontact_calendar_get_label (GDATA_GCONTACT_CALENDAR (list->data)) == NULL);
	g_assert (gdata_gcontact_calendar_is_primary (GDATA_GCONTACT_CALENDAR (list->data)) == FALSE);

	g_object_unref (contact);
}

static void
test_parser_error_handling (gconstpointer service)
{
	GDataContactsContact *contact;
	GError *error = NULL;

#define TEST_XML_ERROR_HANDLING(x) \
	contact = GDATA_CONTACTS_CONTACT (gdata_parsable_new_from_xml (GDATA_TYPE_CONTACTS_CONTACT,\
		"<entry xmlns='http://www.w3.org/2005/Atom' "\
		"xmlns:gd='http://schemas.google.com/g/2005' "\
		"xmlns:gContact='http://schemas.google.com/contact/2008'>"\
			x\
		"</entry>", -1, &error));\
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR);\
	g_assert (contact == NULL);\
	g_clear_error (&error)

	/* app:edited */
	TEST_XML_ERROR_HANDLING ("<app:edited xmlns:app='http://www.w3.org/2007/app'>this shouldn't parse</app:edited>");

	/* gd:name */
	TEST_XML_ERROR_HANDLING ("<gd:name><gd:givenName>Spartacus</gd:givenName><gd:givenName>Spartacus</gd:givenName></gd:name>");

	/* gd:email */
	TEST_XML_ERROR_HANDLING ("<gd:email>neither should this</gd:email>");

	/* gd:im */
	TEST_XML_ERROR_HANDLING ("<gd:im>nor this</gd:im>");

	/* gd:phoneNumber */
	TEST_XML_ERROR_HANDLING ("<gd:phoneNumber/>");

	/* gd:structuredPostalAddress */
	TEST_XML_ERROR_HANDLING ("<gd:structuredPostalAddress rel=''/>");

	/* gd:organization */
	TEST_XML_ERROR_HANDLING ("<gd:organization rel=''/>");

	/* gd:extendedProperty */
	TEST_XML_ERROR_HANDLING ("<gd:extendedProperty/>");

	/* gContact:groupMembershipInfo */
	TEST_XML_ERROR_HANDLING ("<gContact:groupMembershipInfo/>");
	TEST_XML_ERROR_HANDLING ("<gContact:groupMembershipInfo href='http://foobar.com/base/1234b' deleted='maybe'/>");

	/* gContact:nickname */
	TEST_XML_ERROR_HANDLING ("<gContact:nickname/>"); /* missing content */
	TEST_XML_ERROR_HANDLING ("<gContact:nickname>Nickname 1</gContact:nickname><gContact:nickname>Duplicate!</gContact:nickname>"); /* duplicate */

	/* gContact:birthday */
	TEST_XML_ERROR_HANDLING ("<gContact:birthday/>"); /* missing "when" attribute */
	TEST_XML_ERROR_HANDLING ("<gContact:birthday when='foobar'/>"); /* invalid date */
	TEST_XML_ERROR_HANDLING ("<gContact:birthday when='2000-01-01'/><gContact:birthday when='--01-01'/>"); /* duplicate */

	/* gContact:billingInformation */
	TEST_XML_ERROR_HANDLING ("<gContact:billingInformation/>"); /* missing content */
	TEST_XML_ERROR_HANDLING ("<gContact:billingInformation>foo</gContact:billingInformation>"
	                         "<gContact:billingInformation>Dupe!</gContact:billingInformation>"); /* duplicate */

	/* gContact:directoryServer */
	TEST_XML_ERROR_HANDLING ("<gContact:directoryServer/>"); /* missing content */
	TEST_XML_ERROR_HANDLING ("<gContact:directoryServer>foo</gContact:directoryServer>"
	                         "<gContact:directoryServer>Dupe!</gContact:directoryServer>"); /* duplicate */

	/* gContact:gender */
	TEST_XML_ERROR_HANDLING ("<gContact:gender/>"); /* missing content */
	TEST_XML_ERROR_HANDLING ("<gContact:gender value='male'/><gContact:gender value='female'/>"); /* duplicate */

	/* gContact:initials */
	TEST_XML_ERROR_HANDLING ("<gContact:initials/>"); /* missing content */
	TEST_XML_ERROR_HANDLING ("<gContact:initials>A</gContact:initials><gContact:initials>B</gContact:initials>"); /* duplicate */

	/* gContact:maidenName */
	TEST_XML_ERROR_HANDLING ("<gContact:maidenName/>"); /* missing content */
	TEST_XML_ERROR_HANDLING ("<gContact:maidenName>A</gContact:maidenName><gContact:maidenName>B</gContact:maidenName>"); /* duplicate */

	/* gContact:mileage */
	TEST_XML_ERROR_HANDLING ("<gContact:mileage/>"); /* missing content */
	TEST_XML_ERROR_HANDLING ("<gContact:mileage>12 mi</gContact:mileage><gContact:mileage>12 mi</gContact:mileage>"); /* duplicate */

	/* gContact:occupation */
	TEST_XML_ERROR_HANDLING ("<gContact:occupation/>"); /* missing content */
	TEST_XML_ERROR_HANDLING ("<gContact:occupation>Foo</gContact:occupation><gContact:occupation>Bar</gContact:occupation>"); /* duplicate */

	/* gContact:priority */
	TEST_XML_ERROR_HANDLING ("<gContact:priority/>"); /* missing rel param */
	TEST_XML_ERROR_HANDLING ("<gContact:priority rel=''/>"); /* empty rel param */
	TEST_XML_ERROR_HANDLING ("<gContact:priority rel='high'/><gContact:priority rel='low'/>"); /* duplicate */

	/* gContact:sensitivity */
	TEST_XML_ERROR_HANDLING ("<gContact:sensitivity/>"); /* missing rel param */
	TEST_XML_ERROR_HANDLING ("<gContact:sensitivity rel=''/>"); /* empty rel param */
	TEST_XML_ERROR_HANDLING ("<gContact:sensitivity rel='private'/><gContact:sensitivity rel='normal'/>"); /* duplicate */

	/* gContact:shortName */
	TEST_XML_ERROR_HANDLING ("<gContact:shortName/>"); /* missing content */
	TEST_XML_ERROR_HANDLING ("<gContact:shortName>Foo</gContact:shortName><gContact:shortName>Bar</gContact:shortName>"); /* duplicate */

	/* gContact:subject */
	TEST_XML_ERROR_HANDLING ("<gContact:subject/>"); /* missing content */
	TEST_XML_ERROR_HANDLING ("<gContact:subject>Foo</gContact:subject><gContact:subject>Bar</gContact:subject>"); /* duplicate */

	/* gContact:jot */
	TEST_XML_ERROR_HANDLING ("<gContact:jot/>");

	/* gContact:relation */
	TEST_XML_ERROR_HANDLING ("<gContact:relation/>");

	/* gContact:website */
	TEST_XML_ERROR_HANDLING ("<gContact:website/>");

	/* gContact:event */
	TEST_XML_ERROR_HANDLING ("<gContact:event/>");

	/* gContact:calendar */
	TEST_XML_ERROR_HANDLING ("<gContact:calendarLink/>");

#undef TEST_XML_ERROR_HANDLING
}

static void
test_photo_has_photo (gconstpointer service)
{
	GDataContactsContact *contact;
	gsize length = 0;
	gchar *content_type = NULL;
	GError *error = NULL;

	contact = GDATA_CONTACTS_CONTACT (gdata_parsable_new_from_xml (GDATA_TYPE_CONTACTS_CONTACT,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
			"xmlns:gd='http://schemas.google.com/g/2005'>"
			"<id>http://www.google.com/m8/feeds/contacts/libgdata.test@googlemail.com/base/1b46cdd20bfbee3b</id>"
			"<updated>2009-04-25T15:21:53.688Z</updated>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://schemas.google.com/contact/2008#contact'/>"
			"<title></title>" /* Here's where it all went wrong */
			"<link rel='http://schemas.google.com/contacts/2008/rel#photo' type='image/*' "
				"href='http://www.google.com/m8/feeds/photos/media/libgdata.test@googlemail.com/1b46cdd20bfbee3b'/>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_CONTACTS_CONTACT (contact));
	check_kind (GDATA_ENTRY (contact));
	g_clear_error (&error);

	/* Check for no photo */
	g_assert (gdata_contacts_contact_has_photo (contact) == FALSE);
	g_assert (gdata_contacts_contact_get_photo (contact, GDATA_CONTACTS_SERVICE (service), &length, &content_type, NULL, &error) == NULL);
	g_assert_cmpint (length, ==, 0);
	g_assert (content_type == NULL);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_free (content_type);
	g_object_unref (contact);

	/* Try again with a photo */
	contact = GDATA_CONTACTS_CONTACT (gdata_parsable_new_from_xml (GDATA_TYPE_CONTACTS_CONTACT,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
			"xmlns:gd='http://schemas.google.com/g/2005'>"
			"<id>http://www.google.com/m8/feeds/contacts/libgdata.test@googlemail.com/base/1b46cdd20bfbee3b</id>"
			"<updated>2009-04-25T15:21:53.688Z</updated>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://schemas.google.com/contact/2008#contact'/>"
			"<title></title>" /* Here's where it all went wrong */
			"<link rel='http://schemas.google.com/contacts/2008/rel#photo' type='image/*' "
				"href='http://www.google.com/m8/feeds/photos/media/libgdata.test@googlemail.com/1b46cdd20bfbee3b' "
				"gd:etag='&quot;QngzcDVSLyp7ImA9WxJTFkoITgU.&quot;'/>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_CONTACTS_CONTACT (contact));
	check_kind (GDATA_ENTRY (contact));
	g_clear_error (&error);

	g_assert (gdata_contacts_contact_has_photo (contact) == TRUE);
	g_object_unref (contact);
}

static void
test_photo_add (gconstpointer service)
{
	GDataContactsContact *contact;
	gchar *data;
	gsize length;
	gboolean retval;
	GError *error = NULL;

	/* Get the photo */
	g_assert (g_file_get_contents (TEST_FILE_DIR "photo.jpg", &data, &length, NULL) == TRUE);

	/* Add it to the contact */
	contact = get_contact (service);
	retval = gdata_contacts_contact_set_photo (contact, GDATA_SERVICE (service), data, length, NULL, &error);
	g_assert_no_error (error);
	g_assert (retval == TRUE);

	g_clear_error (&error);
	g_object_unref (contact);
	g_free (data);
}

static void
test_photo_get (gconstpointer service)
{
	GDataContactsContact *contact;
	gchar *data, *content_type = NULL;
	gsize length = 0;
	GError *error = NULL;

	contact = get_contact (service);
	g_assert (gdata_contacts_contact_has_photo (contact) == TRUE);

	/* Get the photo from the network */
	data = gdata_contacts_contact_get_photo (contact, GDATA_CONTACTS_SERVICE (service), &length, &content_type, NULL, &error);
	g_assert_no_error (error);
	g_assert (data != NULL);
	g_assert (length != 0);
	g_assert_cmpstr (content_type, ==, "image/jpeg");

	g_assert (gdata_contacts_contact_has_photo (contact) == TRUE);

	g_free (content_type);
	g_free (data);
	g_object_unref (contact);
	g_clear_error (&error);
}

static void
test_photo_delete (gconstpointer service)
{
	GDataContactsContact *contact;
	GError *error = NULL;

	contact = get_contact (service);
	g_assert (gdata_contacts_contact_has_photo (contact) == TRUE);

	/* Remove the contact's photo */
	g_assert (gdata_contacts_contact_set_photo (contact, GDATA_SERVICE (service), NULL, 0, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_assert (gdata_contacts_contact_has_photo (contact) == FALSE);

	g_clear_error (&error);
	g_object_unref (contact);
}

int
main (int argc, char *argv[])
{
	GDataService *service;
	gint retval;

	gdata_test_init (&argc, &argv);

	service = GDATA_SERVICE (gdata_contacts_service_new (CLIENT_ID));
	gdata_service_authenticate (service, USERNAME, PASSWORD, NULL, NULL);

	g_test_add_func ("/contacts/authentication", test_authentication);
	g_test_add_data_func ("/contacts/insert/simple", service, test_insert_simple);
	g_test_add_data_func ("/contacts/query/all_contacts", service, test_query_all_contacts);
	if (g_test_thorough () == TRUE)
		g_test_add_data_func ("/contacts/query/all_contacts_async", service, test_query_all_contacts_async);
	g_test_add_func ("/contacts/query/uri", test_query_uri);
	g_test_add_func ("/contacts/query/etag", test_query_etag);
	g_test_add_func ("/contacts/query/properties", test_query_properties);
	g_test_add_data_func ("/contacts/parser/minimal", service, test_parser_minimal);
	g_test_add_data_func ("/contacts/parser/normal", service, test_parser_normal);
	g_test_add_data_func ("/contacts/parser/error_handling", service, test_parser_error_handling);
	g_test_add_data_func ("/contacts/photo/has_photo", service, test_photo_has_photo);
	g_test_add_data_func ("/contacts/photo/add", service, test_photo_add);
	g_test_add_data_func ("/contacts/photo/get", service, test_photo_get);
	g_test_add_data_func ("/contacts/photo/delete", service, test_photo_delete);

	retval = g_test_run ();
	g_object_unref (service);

	return retval;
}
