/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009, 2010, 2011, 2014 <philip@tecnocode.co.uk>
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
#include "gdata-dummy-authorizer.h"

static UhmServer *mock_server = NULL;

#undef CLIENT_ID  /* from common.h */

#define CLIENT_ID "352818697630-nqu2cmt5quqd6lr17ouoqmb684u84l1f.apps.googleusercontent.com"
#define CLIENT_SECRET "-fA4pHQJxR3zJ-FyAMPQsikg"
#define REDIRECT_URI "urn:ietf:wg:oauth:2.0:oob"

typedef struct {
	GDataContactsContact *contact;
} TempContactData;

static void
set_up_temp_contact (TempContactData *data, gconstpointer service)
{
	GDataContactsContact *contact;

	gdata_test_mock_server_start_trace (mock_server, "setup-temp-contact");

	/* Create a new temporary contact to use for a single test */
	contact = gdata_contacts_contact_new (NULL);
	gdata_contacts_contact_set_nickname (contact, "Test Contact Esq.");

	/* Insert the contact */
	data->contact = gdata_contacts_service_insert_contact (GDATA_CONTACTS_SERVICE (service), contact, NULL, NULL);
	g_assert (GDATA_IS_CONTACTS_CONTACT (data->contact));
	gdata_test_compare_kind (GDATA_ENTRY (data->contact), "http://schemas.google.com/contact/2008#contact", NULL);

	g_object_unref (contact);

	uhm_server_end_trace (mock_server);

	/* HACK. Wait for the server to propagate distributed changes. */
	if (uhm_server_get_enable_online (mock_server) == TRUE) {
		sleep (10);
	}
}

static void
tear_down_temp_contact (TempContactData *data, gconstpointer service)
{
	GDataEntry *updated_contact;

	gdata_test_mock_server_start_trace (mock_server, "teardown-temp-contact");

	/* Re-query for the contact to get any updated ETags */
	updated_contact = gdata_service_query_single_entry (GDATA_SERVICE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                                    gdata_entry_get_id (GDATA_ENTRY (data->contact)), NULL, GDATA_TYPE_CONTACTS_CONTACT,
	                                                    NULL, NULL);
	g_assert (GDATA_IS_CONTACTS_CONTACT (updated_contact));

	g_object_unref (data->contact);

	/* Delete the new/updated contact */
	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                      updated_contact, NULL, NULL) == TRUE);

	g_object_unref (updated_contact);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_CLOSURE_FUNCTIONS (temp_contact, TempContactData);

static void
test_authentication (void)
{
	GDataOAuth2Authorizer *authorizer = NULL;  /* owned */
	gchar *authentication_uri, *authorisation_code;

	gdata_test_mock_server_start_trace (mock_server, "authentication");

	authorizer = gdata_oauth2_authorizer_new (CLIENT_ID, CLIENT_SECRET,
	                                          REDIRECT_URI,
	                                          GDATA_TYPE_CONTACTS_SERVICE);

	/* Get an authentication URI. */
	authentication_uri = gdata_oauth2_authorizer_build_authentication_uri (authorizer, NULL, FALSE);
	g_assert (authentication_uri != NULL);

	/* Get the authorisation code off the user. */
	if (uhm_server_get_enable_online (mock_server)) {
		authorisation_code = gdata_test_query_user_for_verifier (authentication_uri);
	} else {
		/* Hard coded, extracted from the trace file. */
		authorisation_code = g_strdup ("4/OEX-S1iMbOA_dOnNgUlSYmGWh3TK.QrR73axcNMkWoiIBeO6P2m_su7cwkQI");
	}

	g_free (authentication_uri);

	if (authorisation_code == NULL) {
		/* Skip tests. */
		goto skip_test;
	}

	/* Authorise the token */
	g_assert (gdata_oauth2_authorizer_request_authorization (authorizer, authorisation_code, NULL, NULL) == TRUE);

	/* Check all is as it should be */
	g_assert (gdata_authorizer_is_authorized_for_domain (GDATA_AUTHORIZER (authorizer),
	                                                     gdata_contacts_service_get_primary_authorization_domain ()) == TRUE);

skip_test:
	g_free (authorisation_code);
	g_object_unref (authorizer);

	uhm_server_end_trace (mock_server);
}

typedef struct {
	GDataContactsContact *contact1;
	GDataContactsContact *contact2;
	GDataContactsContact *contact3;
} QueryAllContactsData;

static void
set_up_query_all_contacts (QueryAllContactsData *data, gconstpointer service)
{
	GDataContactsContact *contact;

	gdata_test_mock_server_start_trace (mock_server, "setup-query-all-contacts");

	/* Create new temporary contacts to use for the query all contacts tests */
	contact = gdata_contacts_contact_new (NULL);
	gdata_contacts_contact_set_nickname (contact, "Test Contact 1");
	data->contact1 = gdata_contacts_service_insert_contact (GDATA_CONTACTS_SERVICE (service), contact, NULL, NULL);
	g_object_unref (contact);

	contact = gdata_contacts_contact_new (NULL);
	gdata_contacts_contact_set_nickname (contact, "Test Contact 2");
	data->contact2 = gdata_contacts_service_insert_contact (GDATA_CONTACTS_SERVICE (service), contact, NULL, NULL);
	g_object_unref (contact);

	contact = gdata_contacts_contact_new (NULL);
	gdata_contacts_contact_set_nickname (contact, "Test Contact 3");
	data->contact3 = gdata_contacts_service_insert_contact (GDATA_CONTACTS_SERVICE (service), contact, NULL, NULL);
	g_object_unref (contact);

	uhm_server_end_trace (mock_server);

	/* It takes a few seconds for the contacts to reliably propagate around Google's servers. Distributed systems are so fun. Not.
	 * Thankfully, we don't have to wait when running against the mock server. */
	if (uhm_server_get_enable_online (mock_server) == TRUE) {
		g_usleep (G_USEC_PER_SEC * 5);
	}
}

static void
tear_down_query_all_contacts (QueryAllContactsData *data, gconstpointer service)
{
	gdata_test_mock_server_start_trace (mock_server, "teardown-query-all-contacts");

	/* Delete the new contacts */
	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->contact1), NULL, NULL) == TRUE);
	g_object_unref (data->contact1);

	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->contact2), NULL, NULL) == TRUE);
	g_object_unref (data->contact2);

	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->contact3), NULL, NULL) == TRUE);
	g_object_unref (data->contact3);

	uhm_server_end_trace (mock_server);
}

static void
test_query_all_contacts (QueryAllContactsData *data, gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "query-all-contacts");

	feed = gdata_contacts_service_query_contacts (GDATA_CONTACTS_SERVICE (service), NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* TODO: check entries, kinds and feed properties */

	g_object_unref (feed);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_CLOSURE_FUNCTIONS (query_all_contacts, QueryAllContactsData);

GDATA_ASYNC_TEST_FUNCTIONS (query_all_contacts, QueryAllContactsData,
G_STMT_START {
	gdata_contacts_service_query_contacts_async (GDATA_CONTACTS_SERVICE (service), NULL, cancellable, NULL,
	                                             NULL, NULL, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataFeed *feed;

	feed = gdata_service_query_finish (GDATA_SERVICE (obj), async_result, &error);

	if (error == NULL) {
		g_assert (GDATA_IS_FEED (feed));
		/* TODO: Tests? */

		g_object_unref (feed);
	} else {
		g_assert (feed == NULL);
	}
} G_STMT_END);

static void
test_query_all_contacts_async_progress_closure (QueryAllContactsData *query_data, gconstpointer service)
{
	GDataAsyncProgressClosure *data = g_slice_new0 (GDataAsyncProgressClosure);

	gdata_test_mock_server_start_trace (mock_server, "query-all-contacts-async-progress-closure");

	data->main_loop = g_main_loop_new (NULL, TRUE);

	gdata_contacts_service_query_contacts_async (GDATA_CONTACTS_SERVICE (service), NULL, NULL,
	                                             (GDataQueryProgressCallback) gdata_test_async_progress_callback,
	                                             data, (GDestroyNotify) gdata_test_async_progress_closure_free,
	                                             (GAsyncReadyCallback) gdata_test_async_progress_finish_callback, data);
	g_main_loop_run (data->main_loop);
	g_main_loop_unref (data->main_loop);

	/* Check that both callbacks were called exactly once */
	g_assert_cmpuint (data->progress_destroy_notify_count, ==, 1);
	g_assert_cmpuint (data->async_ready_notify_count, ==, 1);

	g_slice_free (GDataAsyncProgressClosure, data);

	uhm_server_end_trace (mock_server);
}

typedef struct {
	GDataContactsContact *new_contact;
} InsertData;

static void
set_up_insert (InsertData *data, gconstpointer service)
{
	data->new_contact = NULL;
}

static void
tear_down_insert (InsertData *data, gconstpointer service)
{
	gdata_test_mock_server_start_trace (mock_server, "teardown-insert");

	/* Delete the new contact */
	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->new_contact), NULL, NULL) == TRUE);

	g_object_unref (data->new_contact);

	uhm_server_end_trace (mock_server);
}

static void
test_contact_insert (InsertData *data, gconstpointer service)
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
	GDataGContactExternalID *external_id;
	GDataGContactLanguage *language;
	GList *list;
	GDate date;
	GHashTable *properties;
	GTimeVal current_time;
	gint64 edited, creation_time;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "contact-insert");

	contact = gdata_contacts_contact_new (NULL);
	g_get_current_time (&current_time);

	/* Check the kind is present and correct */
	g_assert (GDATA_IS_CONTACTS_CONTACT (contact));
	gdata_test_compare_kind (GDATA_ENTRY (contact), "http://schemas.google.com/contact/2008#contact", NULL);

	/* Set and check the name (to check if the title of the entry is updated) */
	gdata_entry_set_title (GDATA_ENTRY (contact), "Elizabeth Bennet");
	name = gdata_contacts_contact_get_name (contact);
	gdata_gd_name_set_full_name (name, "Lizzie Bennet");

	name2 = gdata_gd_name_new ("John", "Smith");
	gdata_gd_name_set_full_name (name2, "John Smith");
	gdata_contacts_contact_set_name (contact, name2);
	g_object_unref (name2);

	gdata_contacts_contact_set_nickname (contact, "Big J");
	gdata_contacts_contact_set_file_as (contact, "J, Big");
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

	external_id = gdata_gcontact_external_id_new ("Number Six", GDATA_GCONTACT_EXTERNAL_ID_ORGANIZATION, NULL);
	gdata_contacts_contact_add_external_id (contact, external_id);
	g_object_unref (external_id);

	gdata_contacts_contact_add_hobby (contact, "Rowing");

	language = gdata_gcontact_language_new ("en-GB", NULL);
	gdata_contacts_contact_add_language (contact, language);
	g_object_unref (language);

	/* Add some extended properties */
	g_assert (gdata_contacts_contact_set_extended_property (contact, "TITLE", NULL) == TRUE);
	g_assert (gdata_contacts_contact_set_extended_property (contact, "ROLE", "") == TRUE);
	g_assert (gdata_contacts_contact_set_extended_property (contact, "CALURI", "http://example.com/") == TRUE);

	/* Add some user-defined fields */
	gdata_contacts_contact_set_user_defined_field (contact, "Favourite colour", "Blue");
	gdata_contacts_contact_set_user_defined_field (contact, "Owes me", "£10");
	gdata_contacts_contact_set_user_defined_field (contact, "My notes", "");
	gdata_contacts_contact_set_user_defined_field (contact, "", "Foo"); /* bgo#648058 */

	/* Insert the contact */
	new_contact = data->new_contact = gdata_contacts_service_insert_contact (GDATA_CONTACTS_SERVICE (service), contact, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CONTACTS_CONTACT (new_contact));
	gdata_test_compare_kind (GDATA_ENTRY (new_contact), "http://schemas.google.com/contact/2008#contact", NULL);
	g_clear_error (&error);

	/* Check its edited date. Yes, we have to allow the edited time to possibly precede the creation time because Google's
	 * servers can allow this to happen. Somehow.
	 * This check isn't run when testing against a mock server because the dates in the trace file may be waaaay out of date. */
	edited = gdata_contacts_contact_get_edited (contact);
	if (uhm_server_get_enable_online (mock_server) == TRUE) {
		creation_time = gdata_contacts_contact_get_edited (new_contact);
		g_assert_cmpint (creation_time + TIME_FUZZINESS, >=, edited);
		g_assert_cmpint (creation_time - TIME_FUZZINESS, <=, edited);
	}

	/* Various properties */
	g_assert_cmpstr (gdata_contacts_contact_get_nickname (new_contact), ==, "Big J");
	g_assert_cmpstr (gdata_contacts_contact_get_file_as (new_contact), ==, "J, Big");
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
	/* FIXME: https://code.google.com/a/google.com/p/apps-api-issues/issues/detail?id=3662
	 * g_assert_cmpstr (gdata_contacts_contact_get_priority (new_contact), ==, GDATA_CONTACTS_PRIORITY_HIGH);
	 * g_assert_cmpstr (gdata_contacts_contact_get_sensitivity (new_contact), ==, GDATA_CONTACTS_SENSITIVITY_PERSONAL); */
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

	/* External IDs */
	list = gdata_contacts_contact_get_external_ids (new_contact);
	g_assert_cmpuint (g_list_length (list), ==, 1);
	g_assert (GDATA_IS_GCONTACT_EXTERNAL_ID (list->data));

	/* Languages */
	list = gdata_contacts_contact_get_languages (new_contact);
	g_assert_cmpuint (g_list_length (list), ==, 1);
	g_assert (GDATA_IS_GCONTACT_LANGUAGE (list->data));

	/* Hobbies */
	list = gdata_contacts_contact_get_hobbies (new_contact);
	g_assert_cmpuint (g_list_length (list), ==, 1);
	g_assert_cmpstr (list->data, ==, "Rowing");

	/* Extended properties */
	g_assert_cmpstr (gdata_contacts_contact_get_extended_property (new_contact, "CALURI"), ==, "http://example.com/");
	g_assert (gdata_contacts_contact_get_extended_property (new_contact, "non-existent") == NULL);

	properties = gdata_contacts_contact_get_extended_properties (new_contact);
	g_assert (properties != NULL);
	g_assert_cmpuint (g_hash_table_size (properties), ==, 1);

	/* User-defined fields */
	g_assert_cmpstr (gdata_contacts_contact_get_user_defined_field (new_contact, "Favourite colour"), ==, "Blue");
	g_assert_cmpstr (gdata_contacts_contact_get_user_defined_field (new_contact, "Owes me"), ==, "£10");
	g_assert_cmpstr (gdata_contacts_contact_get_user_defined_field (new_contact, "My notes"), ==, "");
	g_assert_cmpstr (gdata_contacts_contact_get_user_defined_field (new_contact, ""), ==, "Foo");

	properties = gdata_contacts_contact_get_user_defined_fields (new_contact);
	g_assert (properties != NULL);
	g_assert_cmpuint (g_hash_table_size (properties), ==, 4);

	/* Groups */
	list = gdata_contacts_contact_get_groups (new_contact);
	g_assert (list == NULL);

	/* Deleted? */
	g_assert (gdata_contacts_contact_is_deleted (new_contact) == FALSE);

	/* TODO: check entries and feed properties */

	g_object_unref (contact);

	uhm_server_end_trace (mock_server);
}

static void
test_contact_update (TempContactData *data, gconstpointer service)
{
	GDataContactsContact *new_contact;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "contact-update");

	/* Update the contact's name and add an extended property */
	gdata_entry_set_title (GDATA_ENTRY (data->contact), "John Wilson");
	g_assert (gdata_contacts_contact_set_extended_property (data->contact, "contact-test", "value"));

	/* Update the contact */
	new_contact = GDATA_CONTACTS_CONTACT (gdata_service_update_entry (GDATA_SERVICE (service),
	                                                                  gdata_contacts_service_get_primary_authorization_domain (),
	                                                                  GDATA_ENTRY (data->contact), NULL, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_CONTACTS_CONTACT (new_contact));
	gdata_test_compare_kind (GDATA_ENTRY (new_contact), "http://schemas.google.com/contact/2008#contact", NULL);
	g_clear_error (&error);

	/* Check a few properties */
	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (new_contact)), ==, "John Wilson");
	g_assert_cmpstr (gdata_contacts_contact_get_extended_property (new_contact, "contact-test"), ==, "value");
	g_assert (gdata_contacts_contact_is_deleted (new_contact) == FALSE);

	g_object_unref (new_contact);

	uhm_server_end_trace (mock_server);
}

typedef struct {
	GDataContactsGroup *group1;
	GDataContactsGroup *group2;
	GDataContactsGroup *group3;
} QueryAllGroupsData;

static void
set_up_query_all_groups (QueryAllGroupsData *data, gconstpointer service)
{
	GDataContactsGroup *group;

	gdata_test_mock_server_start_trace (mock_server, "setup-query-all-groups");

	group = gdata_contacts_group_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (group), "Test Group 1");
	data->group1 = gdata_contacts_service_insert_group (GDATA_CONTACTS_SERVICE (service), group, NULL, NULL);
	g_assert (GDATA_IS_CONTACTS_GROUP (data->group1));
	g_object_unref (group);

	group = gdata_contacts_group_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (group), "Test Group 2");
	data->group2 = gdata_contacts_service_insert_group (GDATA_CONTACTS_SERVICE (service), group, NULL, NULL);
	g_assert (GDATA_IS_CONTACTS_GROUP (data->group2));
	g_object_unref (group);

	group = gdata_contacts_group_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (group), "Test Group 3");
	data->group3 = gdata_contacts_service_insert_group (GDATA_CONTACTS_SERVICE (service), group, NULL, NULL);
	g_assert (GDATA_IS_CONTACTS_GROUP (data->group3));
	g_object_unref (group);

	uhm_server_end_trace (mock_server);

	/* HACK! Guess what? Distributed system inconsistency strikes again! */
	if (uhm_server_get_enable_online (mock_server) == TRUE) {
		sleep (10);
	}
}

static void
tear_down_query_all_groups (QueryAllGroupsData *data, gconstpointer service)
{
	gdata_test_mock_server_start_trace (mock_server, "teardown-query-all-groups");

	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->group1), NULL, NULL) == TRUE);
	g_object_unref (data->group1);

	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->group2), NULL, NULL) == TRUE);
	g_object_unref (data->group2);

	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->group3), NULL, NULL) == TRUE);
	g_object_unref (data->group3);

	uhm_server_end_trace (mock_server);
}

static void
test_query_all_groups (QueryAllGroupsData *data, gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "query-all-groups");

	feed = gdata_contacts_service_query_groups (GDATA_CONTACTS_SERVICE (service), NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* TODO: check entries, kinds and feed properties */

	g_object_unref (feed);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_CLOSURE_FUNCTIONS (query_all_groups, QueryAllGroupsData);

GDATA_ASYNC_TEST_FUNCTIONS (query_all_groups, QueryAllGroupsData,
G_STMT_START {
	gdata_contacts_service_query_groups_async (GDATA_CONTACTS_SERVICE (service), NULL, cancellable, NULL, NULL, NULL,
	                                           async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataFeed *feed;

	feed = gdata_service_query_finish (GDATA_SERVICE (obj), async_result, &error);

	if (error == NULL) {
		g_assert (GDATA_IS_FEED (feed));
		/* TODO: Tests? */

		g_object_unref (feed);
	} else {
		g_assert (feed == NULL);
	}
} G_STMT_END);

static void
test_query_all_groups_async_progress_closure (QueryAllGroupsData *query_data, gconstpointer service)
{
	GDataAsyncProgressClosure *data = g_slice_new0 (GDataAsyncProgressClosure);

	gdata_test_mock_server_start_trace (mock_server, "query-all-groups-async-progress-closure");

	data->main_loop = g_main_loop_new (NULL, TRUE);

	gdata_contacts_service_query_groups_async (GDATA_CONTACTS_SERVICE (service), NULL, NULL,
	                                           (GDataQueryProgressCallback) gdata_test_async_progress_callback,
	                                           data, (GDestroyNotify) gdata_test_async_progress_closure_free,
	                                           (GAsyncReadyCallback) gdata_test_async_progress_finish_callback, data);

	g_main_loop_run (data->main_loop);
	g_main_loop_unref (data->main_loop);

	/* Check that both callbacks were called exactly once */
	g_assert_cmpuint (data->progress_destroy_notify_count, ==, 1);
	g_assert_cmpuint (data->async_ready_notify_count, ==, 1);

	g_slice_free (GDataAsyncProgressClosure, data);

	uhm_server_end_trace (mock_server);
}

typedef struct {
	GDataContactsGroup *new_group;
} InsertGroupData;

static void
set_up_insert_group (InsertGroupData *data, gconstpointer service)
{
	data->new_group = NULL;
}

static void
tear_down_insert_group (InsertGroupData *data, gconstpointer service)
{
	/* HACK! Distributed systems suck. */
	if (uhm_server_get_enable_online (mock_server) == TRUE) {
		sleep (10);
	}

	gdata_test_mock_server_start_trace (mock_server, "teardown-insert-group");

	/* Delete the group, just to be tidy */
	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->new_group), NULL, NULL) == TRUE);
	g_object_unref (data->new_group);

	uhm_server_end_trace (mock_server);
}

static void
test_group_insert (InsertGroupData *data, gconstpointer service)
{
	GDataContactsGroup *group, *new_group;
	GTimeVal time_val;
	GHashTable *properties;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "group-insert");

	g_get_current_time (&time_val);

	group = gdata_contacts_group_new (NULL);

	/* Check the kind is present and correct */
	g_assert (GDATA_IS_CONTACTS_GROUP (group));
	gdata_test_compare_kind (GDATA_ENTRY (group), "http://schemas.google.com/contact/2008#group", NULL);

	/* Set various properties */
	gdata_entry_set_title (GDATA_ENTRY (group), "New Group!");
	g_assert (gdata_contacts_group_set_extended_property (group, "foobar", "barfoo") == TRUE);

	/* Insert the group */
	new_group = data->new_group = gdata_contacts_service_insert_group (GDATA_CONTACTS_SERVICE (service), group, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CONTACTS_GROUP (new_group));
	gdata_test_compare_kind (GDATA_ENTRY (new_group), "http://schemas.google.com/contact/2008#group", NULL);
	g_clear_error (&error);

	/* Check the properties. Time-based properties can't be checked when running against a mock server, since
	 * the trace files may be quite old. */
	if (uhm_server_get_enable_online (mock_server) == TRUE) {
		g_assert_cmpint (gdata_contacts_group_get_edited (new_group), >=, time_val.tv_sec);
	}
	g_assert (gdata_contacts_group_is_deleted (new_group) == FALSE);
	g_assert (gdata_contacts_group_get_system_group_id (new_group) == NULL);

	properties = gdata_contacts_group_get_extended_properties (new_group);
	g_assert (properties != NULL);
	g_assert_cmpint (g_hash_table_size (properties), ==, 1);
	g_assert_cmpstr (gdata_contacts_group_get_extended_property (new_group, "foobar"), ==, "barfoo");

	g_object_unref (group);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_CLOSURE_FUNCTIONS (insert_group, InsertGroupData);

GDATA_ASYNC_TEST_FUNCTIONS (group_insert, InsertGroupData,
G_STMT_START {
	GDataContactsGroup *group;

	group = gdata_contacts_group_new (NULL);

	/* Check the kind is present and correct */
	g_assert (GDATA_IS_CONTACTS_GROUP (group));
	gdata_test_compare_kind (GDATA_ENTRY (group), "http://schemas.google.com/contact/2008#group", NULL);

	/* Set various properties */
	gdata_entry_set_title (GDATA_ENTRY (group), "New Group!");
	g_assert (gdata_contacts_group_set_extended_property (group, "foobar", "barfoo") == TRUE);

	gdata_contacts_service_insert_group_async (GDATA_CONTACTS_SERVICE (service), group, cancellable, async_ready_callback, async_data);

	g_object_unref (group);
} G_STMT_END,
G_STMT_START {
	GDataEntry *entry;

	entry = gdata_service_insert_entry_finish (GDATA_SERVICE (obj), async_result, &error);

	if (error == NULL) {
		g_assert (GDATA_IS_CONTACTS_GROUP (entry));
		/* TODO: Tests? */

		data->new_group = GDATA_CONTACTS_GROUP (entry);
	} else {
		g_assert (entry == NULL);
	}
} G_STMT_END);

static void
test_contact_properties (void)
{
	GDataContactsContact *contact;
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
	GDataGContactExternalID *external_id;
	GDataGContactLanguage *language;
	gchar *nickname, *file_as, *billing_information, *directory_server, *gender, *initials, *maiden_name, *mileage, *occupation;
	gchar *priority, *sensitivity, *short_name, *subject, *photo_etag;
	GDate date, *date2;
	GTimeVal current_time;
	gint64 edited;
	gboolean deleted, birthday_has_year;

	contact = gdata_contacts_contact_new (NULL);
	g_get_current_time (&current_time);

	/* Check the kind is present and correct */
	g_assert (GDATA_IS_CONTACTS_CONTACT (contact));
	gdata_test_compare_kind (GDATA_ENTRY (contact), "http://schemas.google.com/contact/2008#contact", NULL);

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
	gdata_contacts_contact_set_file_as (contact, "J, Big");
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

	external_id = gdata_gcontact_external_id_new ("Number Six", GDATA_GCONTACT_EXTERNAL_ID_ORGANIZATION, NULL);
	gdata_contacts_contact_add_external_id (contact, external_id);
	g_object_unref (external_id);

	gdata_contacts_contact_add_hobby (contact, "Rowing");

	language = gdata_gcontact_language_new ("en-GB", NULL);
	gdata_contacts_contact_add_language (contact, language);
	g_object_unref (language);

	/* Add some extended properties */
	g_assert (gdata_contacts_contact_set_extended_property (contact, "TITLE", NULL) == TRUE);
	g_assert (gdata_contacts_contact_set_extended_property (contact, "ROLE", "") == TRUE);
	g_assert (gdata_contacts_contact_set_extended_property (contact, "CALURI", "http://example.com/") == TRUE);

	/* Add some user-defined fields */
	gdata_contacts_contact_set_user_defined_field (contact, "Favourite colour", "Blue");
	gdata_contacts_contact_set_user_defined_field (contact, "Owes me", "£10");
	gdata_contacts_contact_set_user_defined_field (contact, "My notes", "");
	gdata_contacts_contact_set_user_defined_field (contact, "", "Foo"); /* bgo#648058 */

	/* Check the properties of the object */
	g_object_get (G_OBJECT (contact),
	              "edited", &edited,
	              "deleted", &deleted,
	              "photo-etag", &photo_etag,
	              "name", &name,
	              "nickname", &nickname,
	              "file-as", &file_as,
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

	g_assert_cmpint (edited, ==, current_time.tv_sec);
	g_assert (deleted == FALSE);
	g_assert (photo_etag == NULL);
	g_assert (name2 == name);
	g_assert_cmpstr (nickname, ==, "Big J");
	g_assert_cmpstr (file_as, ==, "J, Big");
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
	g_free (file_as);
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
	g_free (photo_etag);

	/* Check the XML */
	gdata_test_assert_xml (contact,
		"<?xml version='1.0' encoding='UTF-8'?>"
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
			"<gContact:externalId value='Number Six' rel='organization'/>"
			"<gContact:language code='en-GB'/>"
			"<gd:extendedProperty name='CALURI'>http://example.com/</gd:extendedProperty>"
			"<gContact:userDefinedField key='Favourite colour' value='Blue'/>"
			"<gContact:userDefinedField key='Owes me' value='£10'/>"
			"<gContact:userDefinedField key='My notes' value=''/>"
			"<gContact:userDefinedField key='' value='Foo'/>" /* bgo#648058 */
			"<gContact:hobby>Rowing</gContact:hobby>"
			"<gContact:nickname>Big J</gContact:nickname>"
			"<gContact:fileAs>J, Big</gContact:fileAs>"
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

	/* Try removing some things from the contact and ensure it works */
	gdata_contacts_contact_remove_all_email_addresses (contact);
	g_assert (gdata_contacts_contact_get_email_addresses (contact) == NULL);
	g_assert (gdata_contacts_contact_get_primary_email_address (contact) == NULL);

	gdata_contacts_contact_remove_all_im_addresses (contact);
	g_assert (gdata_contacts_contact_get_im_addresses (contact) == NULL);
	g_assert (gdata_contacts_contact_get_primary_im_address (contact) == NULL);

	gdata_contacts_contact_remove_all_phone_numbers (contact);
	g_assert (gdata_contacts_contact_get_phone_numbers (contact) == NULL);
	g_assert (gdata_contacts_contact_get_primary_phone_number (contact) == NULL);

	gdata_contacts_contact_remove_all_postal_addresses (contact);
	g_assert (gdata_contacts_contact_get_postal_addresses (contact) == NULL);
	g_assert (gdata_contacts_contact_get_primary_postal_address (contact) == NULL);

	gdata_contacts_contact_remove_all_organizations (contact);
	g_assert (gdata_contacts_contact_get_organizations (contact) == NULL);
	g_assert (gdata_contacts_contact_get_primary_organization (contact) == NULL);

	gdata_contacts_contact_remove_all_jots (contact);
	g_assert (gdata_contacts_contact_get_jots (contact) == NULL);

	gdata_contacts_contact_remove_all_relations (contact);
	g_assert (gdata_contacts_contact_get_relations (contact) == NULL);

	gdata_contacts_contact_remove_all_websites (contact);
	g_assert (gdata_contacts_contact_get_websites (contact) == NULL);
	g_assert (gdata_contacts_contact_get_primary_website (contact) == NULL);

	gdata_contacts_contact_remove_all_events (contact);
	g_assert (gdata_contacts_contact_get_events (contact) == NULL);

	gdata_contacts_contact_remove_all_calendars (contact);
	g_assert (gdata_contacts_contact_get_calendars (contact) == NULL);
	g_assert (gdata_contacts_contact_get_primary_calendar (contact) == NULL);

	gdata_contacts_contact_remove_all_external_ids (contact);
	g_assert (gdata_contacts_contact_get_external_ids (contact) == NULL);

	gdata_contacts_contact_remove_all_languages (contact);
	g_assert (gdata_contacts_contact_get_languages (contact) == NULL);

	gdata_contacts_contact_remove_all_hobbies (contact);
	g_assert (gdata_contacts_contact_get_hobbies (contact) == NULL);
}

static void
test_contact_escaping (void)
{
	GDataContactsContact *contact;

	contact = gdata_contacts_contact_new (NULL);
	gdata_contacts_contact_set_nickname (contact, "Nickname & stuff");
	gdata_contacts_contact_set_file_as (contact, "Stuff, & Nickname");
	gdata_contacts_contact_set_billing_information (contact, "Billing information & stuff");
	gdata_contacts_contact_set_directory_server (contact, "http://foo.com?foo&bar");
	gdata_contacts_contact_set_gender (contact, "Misc. & other");
	gdata_contacts_contact_set_initials (contact, "<AB>");
	gdata_contacts_contact_set_maiden_name (contact, "Maiden & name");
	gdata_contacts_contact_set_mileage (contact, "Over the hills & far away");
	gdata_contacts_contact_set_occupation (contact, "Occupation & stuff");
	gdata_contacts_contact_set_priority (contact, "http://foo.com?foo&priority=bar");
	gdata_contacts_contact_set_sensitivity (contact, "http://foo.com?foo&sensitivity=bar");
	gdata_contacts_contact_set_short_name (contact, "Short name & stuff");
	gdata_contacts_contact_set_subject (contact, "Subject & stuff");
	gdata_contacts_contact_add_hobby (contact, "Escaping &s");
	gdata_contacts_contact_set_extended_property (contact, "extended & prop", "<unescaped>Value should be a pre-escaped XML blob.</unescaped>");
	gdata_contacts_contact_set_user_defined_field (contact, "User defined field & stuff", "Value & stuff");
	gdata_contacts_contact_add_group (contact, "http://foo.com?foo&bar");

	/* Check the outputted XML is escaped properly */
	gdata_test_assert_xml (contact,
		"<?xml version='1.0' encoding='UTF-8'?>"
		"<entry xmlns='http://www.w3.org/2005/Atom' xmlns:gd='http://schemas.google.com/g/2005' "
		       "xmlns:app='http://www.w3.org/2007/app' xmlns:gContact='http://schemas.google.com/contact/2008'>"
			"<title type='text'></title>"
			"<category term='http://schemas.google.com/contact/2008#contact' scheme='http://schemas.google.com/g/2005#kind'/>"
			"<gd:name/>"
			"<gd:extendedProperty name='extended &amp; prop'>"
				"<unescaped>Value should be a pre-escaped XML blob.</unescaped>"
			"</gd:extendedProperty>"
			"<gContact:userDefinedField key='User defined field &amp; stuff' value='Value &amp; stuff'/>"
			"<gContact:groupMembershipInfo href='http://foo.com?foo&amp;bar'/>"
			"<gContact:hobby>Escaping &amp;s</gContact:hobby>"
			"<gContact:nickname>Nickname &amp; stuff</gContact:nickname>"
			"<gContact:fileAs>Stuff, &amp; Nickname</gContact:fileAs>"
			"<gContact:billingInformation>Billing information &amp; stuff</gContact:billingInformation>"
			"<gContact:directoryServer>http://foo.com?foo&amp;bar</gContact:directoryServer>"
			"<gContact:gender value='Misc. &amp; other'/>"
			"<gContact:initials>&lt;AB&gt;</gContact:initials>"
			"<gContact:maidenName>Maiden &amp; name</gContact:maidenName>"
			"<gContact:mileage>Over the hills &amp; far away</gContact:mileage>"
			"<gContact:occupation>Occupation &amp; stuff</gContact:occupation>"
			"<gContact:priority rel='http://foo.com?foo&amp;priority=bar'/>"
			"<gContact:sensitivity rel='http://foo.com?foo&amp;sensitivity=bar'/>"
			"<gContact:shortName>Short name &amp; stuff</gContact:shortName>"
			"<gContact:subject>Subject &amp; stuff</gContact:subject>"
		"</entry>");
	g_object_unref (contact);
}

static void
test_group_escaping (void)
{
	GDataContactsGroup *group;

	group = gdata_contacts_group_new (NULL);
	gdata_contacts_group_set_extended_property (group, "extended & prop", "<unescaped>Value should be a pre-escaped XML blob.</unescaped>");

	/* Check the outputted XML is escaped properly */
	gdata_test_assert_xml (group,
		"<?xml version='1.0' encoding='UTF-8'?>"
		"<entry xmlns='http://www.w3.org/2005/Atom' xmlns:gd='http://schemas.google.com/g/2005' "
		       "xmlns:app='http://www.w3.org/2007/app' xmlns:gContact='http://schemas.google.com/contact/2008'>"
			"<title type='text'></title>"
			"<category term='http://schemas.google.com/contact/2008#group' scheme='http://schemas.google.com/g/2005#kind'/>"
			"<gd:extendedProperty name='extended &amp; prop'>"
				"<unescaped>Value should be a pre-escaped XML blob.</unescaped>"
			"</gd:extendedProperty>"
		"</entry>");
	g_object_unref (group);
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
test_contact_parser_minimal (void)
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
			"<link rel='http://schemas.google.com/contacts/2008/rel#photo' type='image/*' "
			      "href='http://www.google.com/m8/feeds/photos/media/libgdata.test@googlemail.com/1b46cdd20bfbee3b'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' "
			      "href='http://www.google.com/m8/feeds/contacts/libgdata.test@googlemail.com/full/1b46cdd20bfbee3b'/>"
			"<link rel='http://www.iana.org/assignments/relation/edit' type='application/atom+xml' "
			      "href='http://www.google.com/m8/feeds/contacts/libgdata.test@googlemail.com/full/1b46cdd20bfbee3b'/>"
			"<gd:email rel='http://schemas.google.com/g/2005#other' address='bob@example.com'/>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_CONTACTS_CONTACT (contact));
	gdata_test_compare_kind (GDATA_ENTRY (contact), "http://schemas.google.com/contact/2008#contact", NULL);
	g_clear_error (&error);

	/* Check the contact's properties */
	g_assert (gdata_entry_get_title (GDATA_ENTRY (contact)) != NULL);
	g_assert (*gdata_entry_get_title (GDATA_ENTRY (contact)) == '\0');

	/* TODO: Check the other properties */

	g_assert (gdata_contacts_contact_get_nickname (contact) == NULL);
	g_assert (gdata_contacts_contact_get_file_as (contact) == NULL);
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
	g_assert (gdata_contacts_contact_get_external_ids (contact) == NULL);
	g_assert (gdata_contacts_contact_get_languages (contact) == NULL);
	g_assert (gdata_contacts_contact_get_hobbies (contact) == NULL);

	g_object_unref (contact);
}

static void
test_contact_parser_normal (void)
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
			"<link rel='http://schemas.google.com/contacts/2008/rel#photo' type='image/*' "
			      "href='http://www.google.com/m8/feeds/photos/media/libgdata.test@googlemail.com/1b46cdd20bfbee3b'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' "
			      "href='http://www.google.com/m8/feeds/contacts/libgdata.test@googlemail.com/full/1b46cdd20bfbee3b'/>"
			"<link rel='http://www.iana.org/assignments/relation/edit' type='application/atom+xml' "
			      "href='http://www.google.com/m8/feeds/contacts/libgdata.test@googlemail.com/full/1b46cdd20bfbee3b'/>"
			"<gd:email rel='http://schemas.google.com/g/2005#other' address='bob@example.com'/>"
			"<gd:email rel='http://schemas.google.com/g/2005#other' address=''/>" /* https://bugzilla.gnome.org/show_bug.cgi?id=734863 */
			"<gd:extendedProperty name='test' value='test value'/>"
			"<gd:organization rel='http://schemas.google.com/g/2005#work' label='Work' primary='true'/>"
			"<gContact:groupMembershipInfo href='http://www.google.com/feeds/contacts/groups/jo%40gmail.com/base/1234a' "
			                              "deleted='true'/>"
			"<gContact:groupMembershipInfo href='http://www.google.com/feeds/contacts/groups/jo%40gmail.com/base/1234b'/>"
			"<gd:deleted/>"
			"<gContact:nickname>Agent Smith</gContact:nickname>"
			"<gContact:fileAs>Smith, Agent</gContact:fileAs>"
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
			"<gContact:website href='' rel='other'/>" /* empty on purpose; see bgo#790671 */
			"<gContact:event rel='anniversary'><gd:when startTime='2010-03-04'/></gContact:event>"
			"<gContact:event label='Foobar'><gd:when startTime='1900-01-01'/></gContact:event>"
			"<gContact:calendarLink href='http://example.com/' rel='free-busy' primary='true'/>"
			"<gContact:calendarLink href='http://example.com/' label='Gig list' primary='false'/>"
			"<gContact:calendarLink href='http://foo.com/calendar' rel='home'/>"
			"<gContact:externalId value='Number Six' label='The Prisoner'/>"
			"<gContact:externalId value='1545' rel='account'/>"
			"<gContact:language label='Fresian'/>"
			"<gContact:language code='en-US'/>"
			"<gContact:hobby>Programming</gContact:hobby>"
			"<gContact:hobby>Heavy metal</gContact:hobby>"
			"<gContact:hobby>Heavy metal</gContact:hobby>" /* Test that duplicates get merged */
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_CONTACTS_CONTACT (contact));
	gdata_test_compare_kind (GDATA_ENTRY (contact), "http://schemas.google.com/contact/2008#contact", NULL);
	g_clear_error (&error);

	/* TODO: Check the other properties */

	g_assert_cmpstr (gdata_contacts_contact_get_nickname (contact), ==, "Agent Smith");
	g_assert_cmpstr (gdata_contacts_contact_get_file_as (contact), ==, "Smith, Agent");
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
	/* Note the empty website should *not* be present. See bgo#790671. */

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

	/* External IDs */
	list = gdata_contacts_contact_get_external_ids (contact);
	g_assert_cmpuint (g_list_length (list), ==, 2);

	g_assert (GDATA_IS_GCONTACT_EXTERNAL_ID (list->data));
	g_assert (GDATA_IS_GCONTACT_EXTERNAL_ID (list->next->data));

	/* Languages */
	list = gdata_contacts_contact_get_languages (contact);
	g_assert_cmpuint (g_list_length (list), ==, 2);

	g_assert (GDATA_IS_GCONTACT_LANGUAGE (list->data));
	g_assert (GDATA_IS_GCONTACT_LANGUAGE (list->next->data));

	/* Hobbies */
	list = gdata_contacts_contact_get_hobbies (contact);
	g_assert_cmpuint (g_list_length (list), ==, 2);

	g_assert_cmpstr (list->data, ==, "Programming");
	g_assert_cmpstr (list->next->data, ==, "Heavy metal");

	g_object_unref (contact);
}

static void
test_contact_parser_error_handling (void)
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

	/* gContact:userDefinedField */
	TEST_XML_ERROR_HANDLING ("<gContact:userDefinedField/>"); /* no key or value */
	TEST_XML_ERROR_HANDLING ("<gContact:userDefinedField key='foo'/>"); /* no value */
	TEST_XML_ERROR_HANDLING ("<gContact:userDefinedField value='bar'/>"); /* no key */

	/* gContact:groupMembershipInfo */
	TEST_XML_ERROR_HANDLING ("<gContact:groupMembershipInfo/>");
	TEST_XML_ERROR_HANDLING ("<gContact:groupMembershipInfo href='http://foobar.com/base/1234b' deleted='maybe'/>");

	/* gContact:nickname */
	TEST_XML_ERROR_HANDLING ("<gContact:nickname/>"); /* missing content */
	TEST_XML_ERROR_HANDLING ("<gContact:nickname>Nickname 1</gContact:nickname><gContact:nickname>Duplicate!</gContact:nickname>"); /* duplicate */

	/* gContact:fileAs */
	TEST_XML_ERROR_HANDLING ("<gContact:fileAs/>"); /* missing content */
	TEST_XML_ERROR_HANDLING ("<gContact:fileAs>File As 1</gContact:fileAs><gContact:fileAs>Duplicate!</gContact:fileAs>"); /* duplicate */

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

	/* gContact:website errors are ignored (see bgo#790671) */

	/* gContact:event */
	TEST_XML_ERROR_HANDLING ("<gContact:event/>");

	/* gContact:calendar */
	TEST_XML_ERROR_HANDLING ("<gContact:calendarLink/>");

	/* gContact:externalId */
	TEST_XML_ERROR_HANDLING ("<gContact:externalId/>");

	/* gContact:language */
	TEST_XML_ERROR_HANDLING ("<gContact:language/>");

	/* gContact:hobby */
	TEST_XML_ERROR_HANDLING ("<gContact:hobby/>");

#undef TEST_XML_ERROR_HANDLING
}

static void
test_group_properties (void)
{
	GDataContactsGroup *group;
	GTimeVal time_val;
	GHashTable *properties;
	gint64 edited;
	gboolean deleted;
	gchar *system_group_id;

	group = gdata_contacts_group_new (NULL);

	/* Check the kind is present and correct */
	g_assert (GDATA_IS_CONTACTS_GROUP (group));
	gdata_test_compare_kind (GDATA_ENTRY (group), "http://schemas.google.com/contact/2008#group", NULL);

	/* Set various properties */
	gdata_entry_set_title (GDATA_ENTRY (group), "New Group!");
	g_assert (gdata_contacts_group_set_extended_property (group, "foobar", "barfoo") == TRUE);

	/* Check various properties */
	g_get_current_time (&time_val);
	g_assert_cmpint (gdata_contacts_group_get_edited (group), ==, time_val.tv_sec);
	g_assert (gdata_contacts_group_is_deleted (group) == FALSE);
	g_assert (gdata_contacts_group_get_system_group_id (group) == NULL);

	properties = gdata_contacts_group_get_extended_properties (group);
	g_assert (properties != NULL);
	g_assert_cmpint (g_hash_table_size (properties), ==, 1);
	g_assert_cmpstr (gdata_contacts_group_get_extended_property (group, "foobar"), ==, "barfoo");

	/* Check the properties a different way */
	g_object_get (G_OBJECT (group),
	              "edited", &edited,
	              "deleted", &deleted,
	              "system-group-id", &system_group_id,
	              NULL);

	g_assert_cmpint (edited, ==, time_val.tv_sec);
	g_assert (deleted == FALSE);
	g_assert (system_group_id == NULL);

	g_free (system_group_id);

	/* Check the XML */
	gdata_test_assert_xml (group,
		"<?xml version='1.0' encoding='UTF-8'?>"
		"<entry xmlns='http://www.w3.org/2005/Atom' "
		       "xmlns:gd='http://schemas.google.com/g/2005' "
		       "xmlns:app='http://www.w3.org/2007/app' "
		       "xmlns:gContact='http://schemas.google.com/contact/2008'>"
			"<title type='text'>New Group!</title>"
			"<content type='text'>New Group!</content>"
			"<category term='http://schemas.google.com/contact/2008#group' scheme='http://schemas.google.com/g/2005#kind'/>"
			"<gd:extendedProperty name='foobar'>barfoo</gd:extendedProperty>"
		"</entry>");

	g_object_unref (group);
}

static void
test_group_parser_normal (void)
{
	GDataContactsGroup *group;
	GHashTable *properties;
	GError *error = NULL;

	group = GDATA_CONTACTS_GROUP (gdata_parsable_new_from_xml (GDATA_TYPE_CONTACTS_GROUP,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
		       "xmlns:gd='http://schemas.google.com/g/2005' "
		       "xmlns:gContact='http://schemas.google.com/contact/2008' "
		       "gd:etag='&quot;Rno4ezVSLyp7ImA9WxdTEUgNRQU.&quot;'>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://schemas.google.com/contact/2008#group'/>"
			"<id>http://www.google.com/feeds/groups/jo%40gmail.com/base/1234</id>"
			"<published>2005-01-18T21:00:00Z</published>"
			"<updated>2006-01-01T00:00:00Z</updated>"
			"<app:edited xmlns:app='http://www.w3.org/2007/app'>2006-01-01T00:00:00Z</app:edited>"
			"<title>Salsa class members</title>"
			"<content/>"
			"<link rel='self' type='application/atom+xml' href='http://www.google.com/m8/feeds/groups/jo%40gmail.com/full/1234'/>"
			"<link rel='edit' type='application/atom+xml' href='http://www.google.com/m8/feeds/groups/jo%40gmail.com/full/1234'/>"
			"<gd:extendedProperty name='more info'>Some text.</gd:extendedProperty>"
			"<gd:extendedProperty name='extra info'>"
				"<xml>Foobar.</xml>"
			"</gd:extendedProperty>"
			"<gd:deleted/>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_CONTACTS_GROUP (group));
	gdata_test_compare_kind (GDATA_ENTRY (group), "http://schemas.google.com/contact/2008#group", NULL);
	g_clear_error (&error);

	g_assert_cmpint (gdata_contacts_group_get_edited (group), ==, 1136073600);
	g_assert (gdata_contacts_group_is_deleted (group) == TRUE);
	g_assert (gdata_contacts_group_get_system_group_id (group) == NULL);

	g_assert_cmpstr (gdata_contacts_group_get_extended_property (group, "more info"), ==, "Some text.");
	g_assert_cmpstr (gdata_contacts_group_get_extended_property (group, "extra info"), ==, "<xml>Foobar.</xml>");

	properties = gdata_contacts_group_get_extended_properties (group);
	g_assert_cmpint (g_hash_table_size (properties), ==, 2);

	g_object_unref (group);
}

static void
test_group_parser_system (void)
{
	GDataContactsGroup *group;
	GError *error = NULL;

	group = GDATA_CONTACTS_GROUP (gdata_parsable_new_from_xml (GDATA_TYPE_CONTACTS_GROUP,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
		       "xmlns:gd='http://schemas.google.com/g/2005' "
		       "xmlns:gContact='http://schemas.google.com/contact/2008' "
		       "gd:etag='&quot;Rno4ezVSLyp7ImA9WxdTEUgNRQU.&quot;'>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://schemas.google.com/contact/2008#group'/>"
			"<id>http://www.google.com/feeds/groups/jo%40gmail.com/base/1234</id>"
			"<published>2005-01-18T21:00:00Z</published>"
			"<updated>2006-01-01T00:00:00Z</updated>"
			"<app:edited xmlns:app='http://www.w3.org/2007/app'>2006-01-01T00:00:00Z</app:edited>"
			"<title>Salsa class members</title>"
			"<content/>"
			"<link rel='self' type='application/atom+xml' href='http://www.google.com/m8/feeds/groups/jo%40gmail.com/full/1234'/>"
			"<link rel='edit' type='application/atom+xml' href='http://www.google.com/m8/feeds/groups/jo%40gmail.com/full/1234'/>"
			"<gContact:systemGroup id='Contacts'/>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_CONTACTS_GROUP (group));
	gdata_test_compare_kind (GDATA_ENTRY (group), "http://schemas.google.com/contact/2008#group", NULL);
	g_clear_error (&error);

	g_assert_cmpint (gdata_contacts_group_get_edited (group), ==, 1136073600);
	g_assert (gdata_contacts_group_is_deleted (group) == FALSE);
	g_assert_cmpstr (gdata_contacts_group_get_system_group_id (group), ==, GDATA_CONTACTS_GROUP_CONTACTS);

	g_object_unref (group);
}

static void
test_group_parser_error_handling (void)
{
	GDataContactsGroup *group;
	GError *error = NULL;

#define TEST_XML_ERROR_HANDLING(x) \
	group = GDATA_CONTACTS_GROUP (gdata_parsable_new_from_xml (GDATA_TYPE_CONTACTS_GROUP,\
		"<entry xmlns='http://www.w3.org/2005/Atom' "\
		       "xmlns:gd='http://schemas.google.com/g/2005' "\
		       "xmlns:gContact='http://schemas.google.com/contact/2008'>"\
			x\
		"</entry>", -1, &error));\
	g_assert_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR);\
	g_assert (group == NULL);\
	g_clear_error (&error)

	/* app:edited */
	TEST_XML_ERROR_HANDLING ("<app:edited xmlns:app='http://www.w3.org/2007/app'>this shouldn't parse</app:edited>");

	/* gd:deleted */
	TEST_XML_ERROR_HANDLING ("<gd:deleted/><gd:deleted/>");

	/* gd:extendedProperty */
	TEST_XML_ERROR_HANDLING ("<gd:extendedProperty/>");

	/* gContact:systemGroup */
	TEST_XML_ERROR_HANDLING ("<gContact:systemGroup/>");
	TEST_XML_ERROR_HANDLING ("<gContact:systemGroup id='foo'/><gContact:systemGroup id='duplicated'/>");

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
	gdata_test_compare_kind (GDATA_ENTRY (contact), "http://schemas.google.com/contact/2008#contact", NULL);
	g_clear_error (&error);

	/* Check for no photo */
	g_assert (gdata_contacts_contact_get_photo_etag (contact) == NULL);
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
	gdata_test_compare_kind (GDATA_ENTRY (contact), "http://schemas.google.com/contact/2008#contact", NULL);
	g_clear_error (&error);

	g_assert (gdata_contacts_contact_get_photo_etag (contact) != NULL);
	g_object_unref (contact);
}

static void
test_photo_add (TempContactData *data, gconstpointer service)
{
	guint8 *photo_data;
	gsize length;
	gboolean retval;
	gchar *path = NULL;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "photo-add");

	/* Get the photo */
	path = g_test_build_filename (G_TEST_DIST, "photo.jpg", NULL);
	g_assert (g_file_get_contents (path, (gchar**) &photo_data, &length, NULL) == TRUE);
	g_free (path);

	/* Add it to the contact */
	retval = gdata_contacts_contact_set_photo (data->contact, GDATA_CONTACTS_SERVICE (service), photo_data, length, "image/jpeg", NULL, &error);
	g_assert_no_error (error);
	g_assert (retval == TRUE);

	g_clear_error (&error);
	g_free (photo_data);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_TEST_FUNCTIONS (photo_add, TempContactData,
G_STMT_START {
	guint8 *photo_data;
	gsize length;
	gchar *path = NULL;

	/* Get the photo */
	path = g_test_build_filename (G_TEST_DIST, "photo.jpg", NULL);
	g_assert (g_file_get_contents (path, (gchar**) &photo_data, &length, NULL) == TRUE);
	g_free (path);

	/* Add it to the contact asynchronously */
	gdata_contacts_contact_set_photo_async (data->contact, GDATA_CONTACTS_SERVICE (service), photo_data, length, "image/jpeg", cancellable,
	                                        async_ready_callback, async_data);

	g_free (photo_data);
} G_STMT_END,
G_STMT_START {
	GDataContactsContact *contact = GDATA_CONTACTS_CONTACT (obj);
	gboolean success;

	success = gdata_contacts_contact_set_photo_finish (contact, async_result, &error);

	if (error == NULL) {
		g_assert (success == TRUE);
		g_assert (gdata_contacts_contact_get_photo_etag (contact) != NULL);
	} else {
		g_assert (success == FALSE);
		/*g_assert (gdata_contacts_contact_get_photo_etag (contact) == NULL);*/

		/* Bail out on a conflict error, since it means the addition went through
		 * (but not fast enough for libgdata to return success rather than cancellation). */
		if (g_error_matches (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_CONFLICT) == TRUE) {
			g_clear_error (&error);
			async_data->cancellation_successful = FALSE;
		}
	}
} G_STMT_END);

static void
add_photo_to_contact (GDataContactsService *service, GDataContactsContact **contact)
{
	guint8 *photo_data;
	gsize length;
	GDataEntry *updated_contact;
	gchar *path = NULL;

	/* Get the photo and add it to the contact */
	path = g_test_build_filename (G_TEST_DIST, "photo.jpg", NULL);
	g_assert (g_file_get_contents (path, (gchar**) &photo_data, &length, NULL) == TRUE);
	g_assert (gdata_contacts_contact_set_photo (*contact, service, photo_data, length, "image/jpeg", NULL, NULL) == TRUE);

	g_free (path);
	g_free (photo_data);

	/* HACK: It fairly consistently seems to take the Google servers about 4 seconds to process uploaded photos. Before this
	 * time, a query for the photo will return an error. So let's wait for 10.
	 * Helps: bgo#679072 */
	if (uhm_server_get_enable_online (mock_server) == TRUE) {
		sleep (10);
	}

	/* Re-query for the contact to get any updated ETags. */
	updated_contact = gdata_service_query_single_entry (GDATA_SERVICE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                                    gdata_entry_get_id (GDATA_ENTRY (*contact)), NULL, GDATA_TYPE_CONTACTS_CONTACT,
	                                                    NULL, NULL);
	g_assert (GDATA_IS_CONTACTS_CONTACT (updated_contact));

	g_object_unref (*contact);
	*contact = GDATA_CONTACTS_CONTACT (updated_contact);
}

typedef TempContactData TempContactWithPhotoData;

static void
set_up_temp_contact_with_photo (TempContactWithPhotoData *data, gconstpointer service)
{
	set_up_temp_contact ((TempContactData*) data, service);

	gdata_test_mock_server_start_trace (mock_server, "setup-temp-contact-with-photo");
	add_photo_to_contact (GDATA_CONTACTS_SERVICE (service), &data->contact);
	uhm_server_end_trace (mock_server);
}

static void
tear_down_temp_contact_with_photo (TempContactWithPhotoData *data, gconstpointer service)
{
	tear_down_temp_contact ((TempContactData*) data, service);
}

GDATA_ASYNC_CLOSURE_FUNCTIONS (temp_contact_with_photo, TempContactWithPhotoData);

static void
test_photo_get (TempContactData *data, gconstpointer service)
{
	guint8 *photo_data;
	gchar *content_type = NULL;
	gsize length = 0;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "photo-get");

	g_assert (gdata_contacts_contact_get_photo_etag (data->contact) != NULL);

	/* Get the photo from the network */
	photo_data = gdata_contacts_contact_get_photo (data->contact, GDATA_CONTACTS_SERVICE (service), &length, &content_type, NULL, &error);
	g_assert_no_error (error);
	g_assert (photo_data != NULL);
	g_assert (length != 0);
	g_assert_cmpstr (content_type, ==, "image/jpeg");

	g_assert (gdata_contacts_contact_get_photo_etag (data->contact) != NULL);

	g_free (content_type);
	g_free (photo_data);
	g_clear_error (&error);

	uhm_server_end_trace (mock_server);
}

GDATA_ASYNC_TEST_FUNCTIONS (photo_get, TempContactData,
G_STMT_START {
	g_assert (gdata_contacts_contact_get_photo_etag (data->contact) != NULL);

	/* Get the photo from the network asynchronously */
	gdata_contacts_contact_get_photo_async (data->contact, GDATA_CONTACTS_SERVICE (service), cancellable, async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataContactsContact *contact = GDATA_CONTACTS_CONTACT (obj);
	guint8 *photo_data;
	gsize length;
	gchar *content_type;

	/* Finish getting the photo */
	photo_data = gdata_contacts_contact_get_photo_finish (contact, async_result, &length, &content_type, &error);

	if (error == NULL) {
		g_assert (photo_data != NULL);
		g_assert (length != 0);
		g_assert_cmpstr (content_type, ==, "image/jpeg");

		g_assert (gdata_contacts_contact_get_photo_etag (contact) != NULL);
	} else {
		g_assert (photo_data == NULL);
		g_assert (length == 0);
		g_assert (content_type == NULL);
	}

	g_free (content_type);
	g_free (photo_data);
} G_STMT_END);

static void
test_photo_delete (TempContactData *data, gconstpointer service)
{
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "photo-delete");

	g_assert (gdata_contacts_contact_get_photo_etag (data->contact) != NULL);

	/* Remove the contact's photo */
	g_assert (gdata_contacts_contact_set_photo (data->contact, GDATA_CONTACTS_SERVICE (service), NULL, 0, NULL, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_assert (gdata_contacts_contact_get_photo_etag (data->contact) == NULL);

	g_clear_error (&error);

	uhm_server_end_trace (mock_server);
}

static void
test_photo_delete_async_cancellation (GDataAsyncTestData *data,
                                      gconstpointer service) __attribute__((unused));

GDATA_ASYNC_TEST_FUNCTIONS (photo_delete, TempContactData,
G_STMT_START {
	g_assert (gdata_contacts_contact_get_photo_etag (data->contact) != NULL);

	/* Delete it from the contact asynchronously */
	gdata_contacts_contact_set_photo_async (data->contact, GDATA_CONTACTS_SERVICE (service), NULL, 0, NULL, cancellable,
	                                        async_ready_callback, async_data);
} G_STMT_END,
G_STMT_START {
	GDataContactsContact *contact = GDATA_CONTACTS_CONTACT (obj);
	gboolean success;

	success = gdata_contacts_contact_set_photo_finish (contact, async_result, &error);

	if (error == NULL) {
		g_assert (success == TRUE);
		g_assert (gdata_contacts_contact_get_photo_etag (contact) == NULL);
	} else {
		g_assert (success == FALSE);
		g_assert (gdata_contacts_contact_get_photo_etag (contact) != NULL);
	}
} G_STMT_END);

static void
test_batch (gconstpointer service)
{
	GDataBatchOperation *operation;
	GDataService *service2;
	GDataContactsContact *contact, *contact2, *contact3;
	GDataEntry *inserted_entry, *inserted_entry2, *inserted_entry3;
	gchar *feed_uri;
	guint op_id, op_id2, op_id3;
	GError *error = NULL, *entry_error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "batch");

	/* Here we hardcode the feed URI, but it should really be extracted from a contacts feed, as the GDATA_LINK_BATCH link */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                              "https://www.google.com/m8/feeds/contacts/default/full/batch");

	/* Check the properties of the operation */
	g_assert (gdata_batch_operation_get_service (operation) == service);
	g_assert_cmpstr (gdata_batch_operation_get_feed_uri (operation), ==, "https://www.google.com/m8/feeds/contacts/default/full/batch");

	g_object_get (operation,
	              "service", &service2,
	              "feed-uri", &feed_uri,
	              NULL);

	g_assert (service2 == service);
	g_assert_cmpstr (feed_uri, ==, "https://www.google.com/m8/feeds/contacts/default/full/batch");

	g_object_unref (service2);
	g_free (feed_uri);

	/* Run a singleton batch operation to insert a new entry */
	contact = gdata_contacts_contact_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (contact), "Fooish Bar");

	gdata_test_batch_operation_insertion (operation, GDATA_ENTRY (contact), &inserted_entry, NULL);
	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);
	g_object_unref (contact);

	/* Run another batch operation to insert another entry and query the previous one */
	contact2 = gdata_contacts_contact_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (contact2), "Brian");

	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                              "https://www.google.com/m8/feeds/contacts/default/full/batch");
	op_id = gdata_test_batch_operation_insertion (operation, GDATA_ENTRY (contact2), &inserted_entry2, NULL);
	op_id2 = gdata_test_batch_operation_query (operation, gdata_entry_get_id (inserted_entry), GDATA_TYPE_CONTACTS_CONTACT, inserted_entry, NULL,
	                                           NULL);
	g_assert_cmpuint (op_id, !=, op_id2);

	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);
	g_object_unref (contact2);

	/* Run another batch operation to delete the first entry and a fictitious one to test error handling, and update the second entry */
	gdata_entry_set_title (inserted_entry2, "Toby");
	contact3 = gdata_contacts_contact_new ("foobar");

	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                              "https://www.google.com/m8/feeds/contacts/default/full/batch");
	op_id = gdata_test_batch_operation_deletion (operation, inserted_entry, NULL);
	op_id2 = gdata_test_batch_operation_deletion (operation, GDATA_ENTRY (contact3), &entry_error);
	op_id3 = gdata_test_batch_operation_update (operation, inserted_entry2, &inserted_entry3, NULL);
	g_assert_cmpuint (op_id, !=, op_id2);
	g_assert_cmpuint (op_id, !=, op_id3);
	g_assert_cmpuint (op_id2, !=, op_id3);

	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_assert_error (entry_error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR);

	g_clear_error (&error);
	g_clear_error (&entry_error);
	g_object_unref (operation);
	g_object_unref (inserted_entry);
	g_object_unref (contact3);

	/* Run another batch operation to update the second entry with the wrong ETag (i.e. pass the old version of the entry to the batch operation
	 * to test error handling */
#if 0
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                              "https://www.google.com/m8/feeds/contacts/default/full/batch");
	gdata_test_batch_operation_update (operation, inserted_entry2, NULL, &entry_error);
	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_assert_error (entry_error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_CONFLICT);
#endif
	g_clear_error (&error);
	g_clear_error (&entry_error);
/*	g_object_unref (operation);*/
	g_object_unref (inserted_entry2);

	/* Run a final batch operation to delete the second entry */
/*	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                              "https://www.google.com/m8/feeds/contacts/default/full/batch");
	gdata_test_batch_operation_deletion (operation, inserted_entry3, NULL);
	g_assert (gdata_test_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);
*/
	g_clear_error (&error);
	/*g_object_unref (operation);*/
	g_object_unref (inserted_entry3);

	uhm_server_end_trace (mock_server);
}

typedef struct {
	GDataContactsContact *new_contact;
} BatchAsyncData;

static void
setup_batch_async (BatchAsyncData *data, gconstpointer service)
{
	GDataContactsContact *contact;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "setup-batch-async");

	/* Insert a new contact which we can query asyncly */
	contact = gdata_contacts_contact_new (NULL);
	gdata_entry_set_title (GDATA_ENTRY (contact), "Fooish Bar");

	data->new_contact = gdata_contacts_service_insert_contact (GDATA_CONTACTS_SERVICE (service), contact, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_CONTACTS_CONTACT (data->new_contact));
	g_clear_error (&error);

	g_object_unref (contact);

	uhm_server_end_trace (mock_server);
}

static void
test_batch_async_cb (GDataBatchOperation *operation, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GError *error = NULL;

	/* Clear all pending events (such as callbacks for the operations) */
	while (g_main_context_iteration (NULL, FALSE) == TRUE);

	g_assert (gdata_test_batch_operation_run_finish (operation, async_result, &error) == TRUE);
	g_assert_no_error (error);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
}

static void
test_batch_async (BatchAsyncData *data, gconstpointer service)
{
	GDataBatchOperation *operation;
	GMainLoop *main_loop;

	gdata_test_mock_server_start_trace (mock_server, "batch-async");

	/* Run an async query operation on the contact */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                              "https://www.google.com/m8/feeds/contacts/default/full/batch");
	gdata_test_batch_operation_query (operation, gdata_entry_get_id (GDATA_ENTRY (data->new_contact)), GDATA_TYPE_CONTACTS_CONTACT,
	                                  GDATA_ENTRY (data->new_contact), NULL, NULL);

	main_loop = g_main_loop_new (NULL, TRUE);

	gdata_batch_operation_run_async (operation, NULL, (GAsyncReadyCallback) test_batch_async_cb, main_loop);
	g_main_loop_run (main_loop);

	g_main_loop_unref (main_loop);
	g_object_unref (operation);

	uhm_server_end_trace (mock_server);
}

static void
test_batch_async_cancellation_cb (GDataBatchOperation *operation, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GError *error = NULL;

	/* Clear all pending events (such as callbacks for the operations) */
	while (g_main_context_iteration (NULL, FALSE) == TRUE);

	g_assert (gdata_test_batch_operation_run_finish (operation, async_result, &error) == FALSE);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
}

static void
test_batch_async_cancellation (BatchAsyncData *data, gconstpointer service)
{
	GDataBatchOperation *operation;
	GMainLoop *main_loop;
	GCancellable *cancellable;
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "batch-async-cancellation");

	/* Run an async query operation on the contact */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                              "https://www.google.com/m8/feeds/contacts/default/full/batch");
	gdata_test_batch_operation_query (operation, gdata_entry_get_id (GDATA_ENTRY (data->new_contact)), GDATA_TYPE_CONTACTS_CONTACT,
	                                  GDATA_ENTRY (data->new_contact), NULL, &error);

	main_loop = g_main_loop_new (NULL, TRUE);
	cancellable = g_cancellable_new ();

	gdata_batch_operation_run_async (operation, cancellable, (GAsyncReadyCallback) test_batch_async_cancellation_cb, main_loop);
	g_cancellable_cancel (cancellable); /* this should cancel the operation before it even starts, as we haven't run the main loop yet */

	g_main_loop_run (main_loop);

	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_clear_error (&error);

	g_main_loop_unref (main_loop);
	g_object_unref (cancellable);
	g_object_unref (operation);

	uhm_server_end_trace (mock_server);
}

static void
teardown_batch_async (BatchAsyncData *data, gconstpointer service)
{
	GError *error = NULL;

	gdata_test_mock_server_start_trace (mock_server, "teardown-batch-async");

	/* Delete the contact */
	g_assert (gdata_service_delete_entry (GDATA_SERVICE (service), gdata_contacts_service_get_primary_authorization_domain (),
	                                      GDATA_ENTRY (data->new_contact), NULL, &error) == TRUE);
	g_assert_no_error (error);
	g_clear_error (&error);

	g_object_unref (data->new_contact);

	uhm_server_end_trace (mock_server);
}

static void
test_group_membership (void)
{
	GDataContactsContact *contact;
	GList *groups;

	/* Create a new contact with no groups */
	contact = gdata_contacts_contact_new (NULL);
	g_assert (GDATA_IS_CONTACTS_CONTACT (contact));
	g_assert (gdata_contacts_contact_get_groups (contact) == NULL);
	g_assert (gdata_contacts_contact_is_group_deleted (contact, "http://notagroup.com/") == FALSE);

	/* Add a group */
	gdata_contacts_contact_add_group (contact, "http://foo.com/group1");
	g_assert (gdata_contacts_contact_is_group_deleted (contact, "http://foo.com/group1") == FALSE);

	groups = gdata_contacts_contact_get_groups (contact);
	g_assert_cmpint (g_list_length (groups), ==, 1);
	g_assert_cmpstr (groups->data, ==, "http://foo.com/group1");

	/* Add another group */
	gdata_contacts_contact_add_group (contact, "http://foo.com/group2");
	g_assert (gdata_contacts_contact_is_group_deleted (contact, "http://foo.com/group1") == FALSE);
	g_assert (gdata_contacts_contact_is_group_deleted (contact, "http://foo.com/group2") == FALSE);

	groups = gdata_contacts_contact_get_groups (contact);
	g_assert_cmpint (g_list_length (groups), ==, 2);
	if (strcmp (groups->data, "http://foo.com/group1") == 0)
		g_assert_cmpstr (groups->next->data, ==, "http://foo.com/group2");
	else if (strcmp (groups->data, "http://foo.com/group2") == 0)
		g_assert_cmpstr (groups->next->data, ==, "http://foo.com/group1");
	else
		g_assert_not_reached ();

	/* Remove the first group */
	gdata_contacts_contact_remove_group (contact, "http://foo.com/group1");
	g_assert (gdata_contacts_contact_is_group_deleted (contact, "http://foo.com/group1") == FALSE); /* hasn't been propagated to the server */

	groups = gdata_contacts_contact_get_groups (contact);
	g_assert_cmpint (g_list_length (groups), ==, 1);
	g_assert_cmpstr (groups->data, ==, "http://foo.com/group2");

	g_object_unref (contact);
}

static void
test_contact_id (void)
{
	GDataContactsContact *contact;
	GError *error = NULL;

	/* Check that IDs are changed to the full projection when creating a new contact… */
	contact = gdata_contacts_contact_new ("http://www.google.com/m8/feeds/contacts/libgdata.test@googlemail.com/base/1b46cdd20bfbee3b");
	g_assert_cmpstr (gdata_entry_get_id (GDATA_ENTRY (contact)), ==,
	                 "http://www.google.com/m8/feeds/contacts/libgdata.test@googlemail.com/full/1b46cdd20bfbee3b");
	g_object_unref (contact);

	/* …and when creating one from XML. */
	contact = GDATA_CONTACTS_CONTACT (gdata_parsable_new_from_xml (GDATA_TYPE_CONTACTS_CONTACT,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
			"xmlns:gd='http://schemas.google.com/g/2005'>"
			"<id>http://www.google.com/m8/feeds/contacts/libgdata.test@googlemail.com/base/1b46cdd20bfbee3b</id>"
			"<updated>2009-04-25T15:21:53.688Z</updated>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://schemas.google.com/contact/2008#contact'/>"
			"<title>Foobar</title>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_CONTACTS_CONTACT (contact));
	g_clear_error (&error);

	g_assert_cmpstr (gdata_entry_get_id (GDATA_ENTRY (contact)), ==,
	                 "http://www.google.com/m8/feeds/contacts/libgdata.test@googlemail.com/full/1b46cdd20bfbee3b");

	g_object_unref (contact);
}

static void
mock_server_notify_resolver_cb (GObject *object, GParamSpec *pspec, gpointer user_data)
{
	UhmServer *server;
	UhmResolver *resolver;

	server = UHM_SERVER (object);

	/* Set up the expected domain names here. This should technically be split up between
	 * the different unit test suites, but that's too much effort. */
	resolver = uhm_server_get_resolver (server);

	if (resolver != NULL) {
		const gchar *ip_address = uhm_server_get_address (server);

		uhm_resolver_add_A (resolver, "www.google.com", ip_address);
		uhm_resolver_add_A (resolver,
		                    "accounts.google.com", ip_address);
	}
}

/* Set up a global GDataAuthorizer to be used for all the tests. Unfortunately,
 * the Google Contacts API is effectively limited to OAuth1 and OAuth2
 * authorisation, so this requires user interaction when online.
 *
 * If not online, use a dummy authoriser. */
static GDataAuthorizer *
create_global_authorizer (void)
{
	GDataOAuth2Authorizer *authorizer = NULL;  /* owned */
	gchar *authentication_uri, *authorisation_code;
	GError *error = NULL;

	/* If not online, just return a dummy authoriser. */
	if (!uhm_server_get_enable_online (mock_server)) {
		return GDATA_AUTHORIZER (gdata_dummy_authorizer_new (GDATA_TYPE_CONTACTS_SERVICE));
	}

	/* Otherwise, go through the interactive OAuth dance. */
	gdata_test_mock_server_start_trace (mock_server, "global-authentication");
	authorizer = gdata_oauth2_authorizer_new (CLIENT_ID, CLIENT_SECRET,
	                                          REDIRECT_URI,
	                                          GDATA_TYPE_CONTACTS_SERVICE);

	/* Get an authentication URI */
	authentication_uri = gdata_oauth2_authorizer_build_authentication_uri (authorizer, NULL, FALSE);
	g_assert (authentication_uri != NULL);

	/* Get the authorisation code off the user. */
	authorisation_code = gdata_test_query_user_for_verifier (authentication_uri);

	g_free (authentication_uri);

	if (authorisation_code == NULL) {
		/* Skip tests. */
		g_object_unref (authorizer);
		authorizer = NULL;
		goto skip_test;
	}

	/* Authorise the token */
	g_assert (gdata_oauth2_authorizer_request_authorization (authorizer, authorisation_code, NULL, &error));
	g_assert_no_error (error);

skip_test:
	g_free (authorisation_code);

	uhm_server_end_trace (mock_server);

	return GDATA_AUTHORIZER (authorizer);
}

int
main (int argc, char *argv[])
{
	gint retval;
	GDataAuthorizer *authorizer = NULL;  /* owned */
	GDataService *service = NULL;  /* owned */
	GFile *trace_directory = NULL;  /* owned */
	gchar *path = NULL;  /* owned */

	gdata_test_init (argc, argv);

	mock_server = gdata_test_get_mock_server ();
	g_signal_connect (G_OBJECT (mock_server), "notify::resolver",
	                  (GCallback) mock_server_notify_resolver_cb, NULL);
	path = g_test_build_filename (G_TEST_DIST, "traces/contacts", NULL);
	trace_directory = g_file_new_for_path (path);
	g_free (path);
	uhm_server_set_trace_directory (mock_server, trace_directory);
	g_object_unref (trace_directory);

	authorizer = create_global_authorizer ();
	service = GDATA_SERVICE (gdata_contacts_service_new (authorizer));

	g_test_add_func ("/contacts/authentication", test_authentication);

	g_test_add ("/contacts/contact/insert", InsertData, service, set_up_insert, test_contact_insert, tear_down_insert);
	g_test_add ("/contacts/contact/update", TempContactData, service, set_up_temp_contact, test_contact_update, tear_down_temp_contact);

	g_test_add ("/contacts/query/all_contacts", QueryAllContactsData, service, set_up_query_all_contacts, test_query_all_contacts,
	            tear_down_query_all_contacts);
	g_test_add ("/contacts/query/all_contacts/async", GDataAsyncTestData, service, set_up_query_all_contacts_async,
	            test_query_all_contacts_async, tear_down_query_all_contacts_async);
	g_test_add ("/contacts/query/all_contacts/async/progress_closure", QueryAllContactsData, service,
	            set_up_query_all_contacts, test_query_all_contacts_async_progress_closure, tear_down_query_all_contacts);
	g_test_add ("/contacts/query/all_contacts/cancellation", GDataAsyncTestData, service, set_up_query_all_contacts_async,
	            test_query_all_contacts_async_cancellation, tear_down_query_all_contacts_async);

	g_test_add_data_func ("/contacts/photo/has_photo", service, test_photo_has_photo);
	g_test_add ("/contacts/photo/add", TempContactData, service, set_up_temp_contact, test_photo_add, tear_down_temp_contact);
	g_test_add ("/contacts/photo/add/async", GDataAsyncTestData, service, set_up_temp_contact_async, test_photo_add_async,
	            tear_down_temp_contact_async);
	g_test_add ("/contacts/photo/add/async/cancellation", GDataAsyncTestData, service, set_up_temp_contact_async,
	            test_photo_add_async_cancellation, tear_down_temp_contact_async);
	g_test_add ("/contacts/photo/get", TempContactData, service, set_up_temp_contact_with_photo, test_photo_get, tear_down_temp_contact);
	g_test_add ("/contacts/photo/get/async", GDataAsyncTestData, service, set_up_temp_contact_with_photo_async, test_photo_get_async,
	            tear_down_temp_contact_with_photo_async);
	g_test_add ("/contacts/photo/get/async/cancellation", GDataAsyncTestData, service, set_up_temp_contact_with_photo_async,
	            test_photo_get_async_cancellation, tear_down_temp_contact_with_photo_async);

	g_test_add ("/contacts/photo/delete", TempContactData, service, set_up_temp_contact_with_photo, test_photo_delete,
	            tear_down_temp_contact);
	g_test_add ("/contacts/photo/delete/async", GDataAsyncTestData, service, set_up_temp_contact_with_photo_async,
	            test_photo_delete_async, tear_down_temp_contact_with_photo_async);
/*
 Too broken to continue running at the moment.
	g_test_add ("/contacts/photo/delete/async/cancellation", GDataAsyncTestData, service, set_up_temp_contact_with_photo_async,
	            test_photo_delete_async_cancellation, tear_down_temp_contact_with_photo_async);
*/

	g_test_add_data_func ("/contacts/batch", service, test_batch);
	g_test_add ("/contacts/batch/async", BatchAsyncData, service, setup_batch_async, test_batch_async, teardown_batch_async);
	g_test_add ("/contacts/batch/async/cancellation", BatchAsyncData, service, setup_batch_async, test_batch_async_cancellation,
	            teardown_batch_async);

	g_test_add ("/contacts/group/query", QueryAllGroupsData, service, set_up_query_all_groups, test_query_all_groups,
	            tear_down_query_all_groups);
	g_test_add ("/contacts/group/query/async", GDataAsyncTestData, service, set_up_query_all_groups_async,
	            test_query_all_groups_async, tear_down_query_all_groups_async);
	g_test_add ("/contacts/group/query/async/progress_closure", QueryAllGroupsData, service, set_up_query_all_groups,
	            test_query_all_groups_async_progress_closure, tear_down_query_all_groups);
	g_test_add ("/contacts/group/query/async/cancellation", GDataAsyncTestData, service, set_up_query_all_groups_async,
	            test_query_all_groups_async_cancellation, tear_down_query_all_groups_async);

	g_test_add ("/contacts/group/insert", InsertGroupData, service, set_up_insert_group, test_group_insert, tear_down_insert_group);
	g_test_add ("/contacts/group/insert/async", GDataAsyncTestData, service, set_up_insert_group_async, test_group_insert_async,
	            tear_down_insert_group_async);
	g_test_add ("/contacts/group/insert/async/cancellation", GDataAsyncTestData, service, set_up_insert_group_async,
	            test_group_insert_async_cancellation, tear_down_insert_group_async);

	g_test_add_func ("/contacts/contact/properties", test_contact_properties);
	g_test_add_func ("/contacts/contact/escaping", test_contact_escaping);
	g_test_add_func ("/contacts/contact/parser/minimal", test_contact_parser_minimal);
	g_test_add_func ("/contacts/contact/parser/normal", test_contact_parser_normal);
	g_test_add_func ("/contacts/contact/parser/error_handling", test_contact_parser_error_handling);
	g_test_add_func ("/contacts/contact/id", test_contact_id);

	g_test_add_func ("/contacts/query/uri", test_query_uri);
	g_test_add_func ("/contacts/query/etag", test_query_etag);
	g_test_add_func ("/contacts/query/properties", test_query_properties);

	g_test_add_func ("/contacts/group/properties", test_group_properties);
	g_test_add_func ("/contacts/group/escaping", test_group_escaping);
	g_test_add_func ("/contacts/group/parser/normal", test_group_parser_normal);
	g_test_add_func ("/contacts/group/parser/system", test_group_parser_system);
	g_test_add_func ("/contacts/group/parser/error_handling", test_group_parser_error_handling);
	g_test_add_func ("/contacts/group/membership", test_group_membership);

	retval = g_test_run ();

	g_clear_object (&service);
	g_clear_object (&authorizer);

	return retval;
}
