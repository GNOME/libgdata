/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) 2019 Mayank Sharma <mayank8019@gmail.com>
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

/*
 * Simple example program to fetch a list of all the files from a user's Google Drive,
 * then set the documents property and finally remove those set properties.
 *
 * This program is just meant to show how to set/get/remove GDataDocumentsProperty
 * , i.e. the Property Resource on a file.
 */

#include <gio/gio.h>
#include <glib.h>
#include <locale.h>

#define GOA_API_IS_SUBJECT_TO_CHANGE
#include <gdata/gdata.h>
#include <goa/goa.h>

#define SET_DUMMY_PROPERTIES TRUE

static void print_documents_properties (GDataDocumentsEntry *entry);
static void set_dummy_properties (GDataDocumentsEntry *entry);
static void unset_dummy_properties (GDataDocumentsEntry *entry);
static gboolean is_owner (GDataService *service, GDataEntry *entry);

static void test_dummy_properties (GDataDocumentsService *service, gboolean set, GCancellable *cancellable, GError **error);

/* FIXME: Work around https://gitlab.gnome.org/GNOME/gnome-online-accounts/issues/73 */
typedef GoaObject AutoGoaObject;
G_DEFINE_AUTOPTR_CLEANUP_FUNC(AutoGoaObject, g_object_unref)

gint
main (void)
{
	g_autoptr(GDataDocumentsService) service = NULL;
	g_autoptr(GError) error = NULL;
	g_autolist(AutoGoaObject) accounts = NULL;
	GList *l = NULL;
	g_autoptr(GoaClient) client = NULL;
	gint retval = 0;

	setlocale (LC_ALL, "");

	client = goa_client_new_sync (NULL, &error);
	if (error != NULL) {
		g_warning ("%s", error->message);
		return 1;
	}

	accounts = goa_client_get_accounts (client);
	for (l = accounts; l != NULL; l = l->next) {
		GoaAccount *account;
		GoaObject *object = GOA_OBJECT (l->data);
		const gchar *provider_type, *account_identity;

		account = goa_object_peek_account (object);
		provider_type = goa_account_get_provider_type (account);
		account_identity = goa_account_get_identity (account);

		if (g_strcmp0 (provider_type, "google") == 0) {
			g_autoptr(GDataGoaAuthorizer) authorizer = NULL;

			authorizer = gdata_goa_authorizer_new (object);
			service = gdata_documents_service_new (GDATA_AUTHORIZER (authorizer));

			if (service == NULL) {
				g_warning ("Account not found");
				retval = 1;
				continue;
			}

			g_message ("Setting dummy properties on the files owned by user - %s", account_identity);
			test_dummy_properties (service, SET_DUMMY_PROPERTIES, NULL, &error);
			if (error != NULL) {
				g_warning ("Error: %s", error->message);
				retval = 1;
				continue;
			}

			g_message ("Removing dummy properties from the files owned by user - %s", account_identity);
			test_dummy_properties (service, !SET_DUMMY_PROPERTIES, NULL, &error);
			if (error != NULL) {
				g_warning ("Error: %s", error->message);
				retval = 1;
				continue;
			}
		}
	}

	return retval;
}

static void
test_dummy_properties (GDataDocumentsService *service, gboolean set, GCancellable *cancellable, GError **error)
{
	g_autoptr(GDataDocumentsQuery) query = NULL;
	g_autoptr(GDataDocumentsFeed) feed = NULL;
	g_autoptr(GError) child_error = NULL;
	GList *entries;
	GList *l;

	query = gdata_documents_query_new_with_limits (NULL, 1, 10);
	gdata_documents_query_set_show_folders (query, TRUE);

	/* Since our query supports fetching 10 results in one go, we just
	 * perform fetch a single page of query. You can use pagination here
	 * and call gdata_query_next_page (GDATA_QUERY (query)) inside a  while
	 * loop to set/unset properties on all the files.
	 * */
	feed = gdata_documents_service_query_documents (service, query, NULL, NULL, NULL, &child_error);
	if (child_error != NULL) {
		g_propagate_error (error, g_steal_pointer (&child_error));
		return;
	}

	entries = gdata_feed_get_entries (GDATA_FEED (feed));
	if (entries == NULL) {
		return;
	}

	for (l = entries; l != NULL; l = l->next) {
		const gchar *title;
		g_autoptr(GDataEntry) new_entry = NULL;
		GDataEntry *entry = GDATA_ENTRY (l->data);

		title = gdata_entry_get_title (entry);
		g_message ("File = %s, id = %s", title, gdata_entry_get_id (GDATA_ENTRY (entry)));

		if (!is_owner (GDATA_SERVICE (service), entry)) {
			g_message ("\t**NOT OWNED**");
			continue;
		}

		if (set) {
			set_dummy_properties (GDATA_DOCUMENTS_ENTRY (entry));
		} else {
			unset_dummy_properties (GDATA_DOCUMENTS_ENTRY (entry));
		}

		new_entry = gdata_service_update_entry (GDATA_SERVICE (service),
							gdata_documents_service_get_primary_authorization_domain(),
							entry,
							NULL,
							&child_error);

		if (child_error != NULL) {
			g_propagate_error (error, g_steal_pointer (&child_error));
			return;
		}

		print_documents_properties (GDATA_DOCUMENTS_ENTRY (new_entry));
	}
}

static gboolean
is_owner (GDataService *service, GDataEntry *entry) {
	GList *l;
	GDataGoaAuthorizer *goa_authorizer;
	GoaAccount *account;
	const gchar *account_identity;

	goa_authorizer = GDATA_GOA_AUTHORIZER (gdata_service_get_authorizer (service));
	account = goa_object_peek_account (gdata_goa_authorizer_get_goa_object (goa_authorizer));
	account_identity = goa_account_get_identity (account);

	for (l = gdata_entry_get_authors (entry); l != NULL; l = l->next) {
		GDataAuthor *author = GDATA_AUTHOR (l->data);

		if (g_strcmp0 (gdata_author_get_email_address (author), account_identity) == 0) {
			return TRUE;
		}
	}

	return FALSE;
}

static void
print_documents_properties (GDataDocumentsEntry *entry)
{
	GList *l, *properties;

	g_return_if_fail (GDATA_IS_DOCUMENTS_ENTRY (entry));

	properties = gdata_documents_entry_get_document_properties (GDATA_DOCUMENTS_ENTRY (entry));
	for (l = properties; l != NULL; l = l->next) {
		GDataDocumentsProperty *property = GDATA_DOCUMENTS_PROPERTY (l->data);

		g_message ("\tkey = %s, value = %s, %s",
			   gdata_documents_property_get_key (GDATA_DOCUMENTS_PROPERTY (property)),
			   gdata_documents_property_get_value (GDATA_DOCUMENTS_PROPERTY (property)),
			   gdata_documents_property_get_visibility (GDATA_DOCUMENTS_PROPERTY (property))
		);
	}

}

static void
set_dummy_properties (GDataDocumentsEntry *entry)
{
	g_autoptr(GDataDocumentsProperty) p1 = NULL, p2 = NULL, p3 = NULL, p4 = NULL;

	p1 = gdata_documents_property_new ("1");
	gdata_documents_property_set_visibility (p1, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC);
	gdata_documents_property_set_value (p1, "ONE");
	gdata_documents_entry_add_documents_property (entry, p1);

	p2 = gdata_documents_property_new ("2");
	gdata_documents_property_set_visibility (p2, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE);
	gdata_documents_property_set_value (p2, "TWO");
	gdata_documents_entry_add_documents_property (entry, p2);

	p3 = gdata_documents_property_new ("3");
	gdata_documents_property_set_visibility (p3, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC);
	gdata_documents_entry_add_documents_property (entry, p3);

	p4 = gdata_documents_property_new ("4");
	gdata_documents_property_set_visibility (p4, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE);
	gdata_documents_entry_add_documents_property (entry, p4);
}

static void
unset_dummy_properties (GDataDocumentsEntry *entry)
{
	g_autoptr(GDataDocumentsProperty) p1 = NULL, p2 = NULL, p3 = NULL, p4 = NULL;

	p1 = gdata_documents_property_new ("1");
	gdata_documents_property_set_visibility (p1, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC);
	gdata_documents_entry_remove_documents_property (entry, p1);

	p2 = gdata_documents_property_new ("2");
	gdata_documents_property_set_visibility (p2, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE);
	gdata_documents_entry_remove_documents_property (entry, p2);

	p3 = gdata_documents_property_new ("3");
	gdata_documents_property_set_visibility (p3, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC);
	gdata_documents_entry_remove_documents_property (entry, p3);

	p4 = gdata_documents_property_new ("4");
	gdata_documents_property_set_visibility (p4, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE);
	gdata_documents_entry_remove_documents_property (entry, p4);
}
