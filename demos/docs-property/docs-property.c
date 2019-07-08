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
 *
 * FIXME: Currently, files which are not owned by the user will cause a 403
 * error since only the owner of the file can set properties on it. We need to
 * check the ownership of a file (a simple boolean will suffice) in an
 * efficient manner. We do have GDataDocumentsEntry implementing 
 * GDataAccessHandler but it has to perform an HTTP request for each file,
 * which is very slow.
 */

#include <gio/gio.h>
#include <glib.h>
#include <locale.h>

#define GOA_API_IS_SUBJECT_TO_CHANGE
#include <gdata/gdata.h>
#include <goa/goa.h>

enum {
	UNSET_DUMMY_PROPERTIES = 0,
	SET_DUMMY_PROPERTIES
};

static void print_documents_property (GDataDocumentsProperty *property);
static void set_dummy_properties (GDataDocumentsEntry *entry);
static void unset_dummy_properties (GDataDocumentsEntry *entry);
static void test_dummy_properties (GDataDocumentsService *service, gint set, GDataDocumentsQuery *query, GCancellable *cancellable, GError **error);

gint
main (void)
{
	GDataDocumentsQuery *query = NULL;
	GDataDocumentsService *service = NULL;
	GError *error = NULL;
	GList *accounts = NULL;
	GList *l = NULL;
	GoaClient *client = NULL;
	gint retval;

	setlocale (LC_ALL, "");

	client = goa_client_new_sync (NULL, &error);
	if (error != NULL) {
		g_warning ("%s", error->message);
		g_error_free (error);
		retval = 1;
		goto out;
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
			GDataGoaAuthorizer *authorizer;

			authorizer = gdata_goa_authorizer_new (object);
			service = gdata_documents_service_new (GDATA_AUTHORIZER (authorizer));

			if (service == NULL) {
				g_warning ("Account not found");
				retval = 1;
				goto out;
			}

			g_object_unref (authorizer);
			query = gdata_documents_query_new_with_limits (NULL, 1, 10);
			gdata_documents_query_set_show_folders (query, TRUE);

			g_message ("Setting dummy properties on the files owned by user - %s", account_identity);
			test_dummy_properties (service, SET_DUMMY_PROPERTIES, query, NULL, &error);
			if (error != NULL) {
				g_warning ("Error: %s", error->message);
				retval = 1;
				goto out;
			}

			g_clear_object (&query);

			/* After setting query to NULL, we perform a new query to fetch the updated
			 * documents */
			query = gdata_documents_query_new_with_limits (NULL, 1, 10);
			gdata_documents_query_set_show_folders (query, TRUE);

			g_message ("Removing dummy properties from the files owned by user - %s", account_identity);
			test_dummy_properties (service, UNSET_DUMMY_PROPERTIES, query, NULL, &error);
			if (error != NULL) {
				g_warning ("Error: %s", error->message);
				retval = 1;
				goto out;
			}

			retval = 0;
		}
	 out:
		g_clear_object (&query);
		g_clear_object (&service);
	}

	retval = 0;
	g_clear_object (&client);
	g_list_free_full (accounts, g_object_unref);
	return retval;
}

static void
test_dummy_properties (GDataDocumentsService *service, gint set, GDataDocumentsQuery *query, GCancellable *cancellable, GError **error)
{
	GDataDocumentsFeed *feed = NULL;
	GError *child_error = NULL;
	GList *entries;
	GList *l;
	guint i;

	/* Since our query supports fetching 10 results in one go, we just
	 * perform 1 iteration over the list of files. The below 'for' loop can be
	 * changed from to a `while (TRUE)` loop to fetch all the files in the
	 * user's drive. */
	for (i = 0; i < 1; i++) {
		feed = gdata_documents_service_query_documents (service, query, NULL, NULL, NULL, &child_error);
		if (child_error != NULL) {
			g_propagate_error (error, child_error);
			goto out_func;
		}

		entries = gdata_feed_get_entries (GDATA_FEED (feed));
		if (entries == NULL) {
			goto out_func;
		}

		for (l = entries; l != NULL; l = l->next) {
			const gchar *title;
			GList *properties = NULL, *p = NULL;
			GDataEntry *new_entry = NULL;
			GDataEntry *entry = GDATA_ENTRY (l->data);

			if (set) {
				set_dummy_properties (GDATA_DOCUMENTS_ENTRY (entry));
			} else {
				unset_dummy_properties (GDATA_DOCUMENTS_ENTRY (entry));
			}

			title = gdata_entry_get_title (entry);
			g_message ("File = %s, id = %s", title, gdata_entry_get_id (GDATA_ENTRY (entry)));

			new_entry = gdata_service_update_entry (GDATA_SERVICE (service),
								gdata_documents_service_get_primary_authorization_domain(),
								entry,
								NULL,
								&child_error);

			if (child_error != NULL) {
				g_warning ("Error: %s", child_error->message);
				g_clear_object (&new_entry);
				continue;
			}

			properties = gdata_documents_entry_get_document_properties (GDATA_DOCUMENTS_ENTRY (new_entry));

			for (p = properties; p != NULL; p = p->next) {
				print_documents_property (GDATA_DOCUMENTS_PROPERTY (p->data));
			}

			g_clear_object (&new_entry);
		}

		gdata_query_next_page (GDATA_QUERY (query));
		g_clear_object (&feed);
	}

 out_func:
	g_clear_object (&feed);
}

static void
print_documents_property (GDataDocumentsProperty *property)
{
	g_return_if_fail (GDATA_IS_DOCUMENTS_PROPERTY (property));

	g_message ("\tkey = %s, value = %s, %s",
		   gdata_documents_property_get_key (GDATA_DOCUMENTS_PROPERTY (property)),
		   gdata_documents_property_get_value (GDATA_DOCUMENTS_PROPERTY (property)),
		   gdata_documents_property_get_visibility (GDATA_DOCUMENTS_PROPERTY (property))
	);
}

static void
set_dummy_properties (GDataDocumentsEntry *entry)
{
	GDataDocumentsProperty *p1, *p2, *p3, *p4;

	p1 = gdata_documents_property_new ("1");
	if (p1 != NULL) {
		gdata_documents_property_set_visibility (p1, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC);
		gdata_documents_property_set_value (p1, "ONE");
		gdata_documents_entry_add_documents_property (entry, p1);
	};

	p2 = gdata_documents_property_new ("2");
	if (p2 != NULL) {
		gdata_documents_property_set_visibility (p2, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE);
		gdata_documents_property_set_value (p2, "TWO");
		gdata_documents_entry_add_documents_property (entry, p2);
	};

	p3 = gdata_documents_property_new ("3");
	if (p3 != NULL) {
		gdata_documents_property_set_visibility (p3, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC);
		gdata_documents_entry_add_documents_property (entry, p3);
	};

	p4 = gdata_documents_property_new ("4");
	if (p4 != NULL) {
		gdata_documents_property_set_visibility (p4, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE);
		gdata_documents_entry_add_documents_property (entry, p4);
	};

	g_clear_object (&p1);
	g_clear_object (&p2);
	g_clear_object (&p3);
	g_clear_object (&p4);
}

static void
unset_dummy_properties (GDataDocumentsEntry *entry)
{
	GDataDocumentsProperty *p1, *p2, *p3, *p4;

	p1 = gdata_documents_property_new ("1");
	if (p1 != NULL) {
		gdata_documents_property_set_visibility (p1, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC);
		gdata_documents_entry_remove_documents_property (entry, p1);
	};

	p2 = gdata_documents_property_new ("2");
	if (p2 != NULL) {
		gdata_documents_property_set_visibility (p2, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE);
		gdata_documents_entry_remove_documents_property (entry, p2);
	};

	p3 = gdata_documents_property_new ("3");
	if (p3 != NULL) {
		gdata_documents_property_set_visibility (p3, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC);
		gdata_documents_entry_remove_documents_property (entry, p3);
	};

	p4 = gdata_documents_property_new ("4");
	if (p4 != NULL) {
		gdata_documents_property_set_visibility (p4, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE);
		gdata_documents_entry_remove_documents_property (entry, p4);
	};

	g_clear_object (&p1);
	g_clear_object (&p2);
	g_clear_object (&p3);
	g_clear_object (&p4);
}
