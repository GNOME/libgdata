/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) 2014 Debarshi Ray <rishi.is@lostca.se>
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
 * Simple example program to list all documents in the user’s Google Documents
 * account, retrieving the account information from GOA.
 */

#include <gio/gio.h>
#include <glib.h>
#include <locale.h>

#define GOA_API_IS_SUBJECT_TO_CHANGE
#include <gdata/gdata.h>
#include <goa/goa.h>

/* TODO: Change the below functions to static */
void add_dummy_properties (GDataDocumentsEntry *entry);
void remove_dummy_properties (GDataDocumentsEntry *entry);
void print_documents_property (GDataDocumentsProperty *property);

gint
main (void)
{
	GDataDocumentsFeed *feed = NULL;
	GDataDocumentsQuery *query = NULL;
	GDataDocumentsService *service = NULL;
	GError *error = NULL;
	GList *accounts = NULL;
	GList *entries;
	GList *l;
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
		const gchar *provider_type;

		account = goa_object_peek_account (object);
		provider_type = goa_account_get_provider_type (account);

		if (g_strcmp0 (provider_type, "google") == 0) {
			GDataGoaAuthorizer *authorizer;

			authorizer = gdata_goa_authorizer_new (object);
			service = gdata_documents_service_new (GDATA_AUTHORIZER (authorizer));
			g_object_unref (authorizer);
		}
	}

	if (service == NULL) {
		g_warning ("Account not found");
		retval = 1;
		goto out;
	}

	query = gdata_documents_query_new_with_limits (NULL, 1, 10);
	gdata_documents_query_set_show_folders (query, TRUE);

	while (TRUE) {
		feed = gdata_documents_service_query_documents (service, query, NULL, NULL, NULL, &error);
		if (error != NULL) {
			g_warning ("%s", error->message);
			g_error_free (error);
			retval = 1;
			goto out;
		}

		entries = gdata_feed_get_entries (GDATA_FEED (feed));
		if (entries == NULL) {
			retval = 0;
			goto out;
		}

		g_message ("Setting Dummy Properties on all the files!");
                for (l = entries; l != NULL; l = l->next) {
			const gchar *title;
			GDataEntry *entry = GDATA_ENTRY (l->data);
			GDataEntry *new_entry = NULL;
			GList *properties = NULL;
			/* GList *properties *p = NULL; */

			/* add_dummy_properties (GDATA_DOCUMENTS_ENTRY (entry)); */

			title = gdata_entry_get_title (entry);
			g_message ("File = %s, id = %s", title, gdata_entry_get_id (GDATA_ENTRY (entry)));

			new_entry = gdata_service_update_entry (GDATA_SERVICE (service),
								gdata_documents_service_get_primary_authorization_domain(),
								entry,
								NULL,
			                                        NULL);

			properties = gdata_documents_entry_get_properties (GDATA_DOCUMENTS_ENTRY (new_entry));

/*                         for (p = properties; p != NULL; p = p->next) {
 *                                 print_documents_property (GDATA_DOCUMENTS_PROPERTY (p->data));
 *                         }
 *  */
			g_list_free_full (properties, g_object_unref);
			g_object_unref (new_entry);
                }

		g_message ("Now Removing all the Dummy Properties!");
		gdata_query_next_page (GDATA_QUERY (query));
		g_object_unref (feed);
	}

	retval = 0;

out:
	g_clear_object (&feed);
	g_clear_object (&query);
	g_clear_object (&service);
	g_clear_object (&client);
	g_list_free_full (accounts, g_object_unref);

	return retval;
}

void
print_documents_property (GDataDocumentsProperty *property) {
	g_return_if_fail (GDATA_IS_DOCUMENTS_PROPERTY (property));

	if (property != NULL) {
		g_message ("key = %s, value = %s, %s",
			   gdata_documents_property_get_key (GDATA_DOCUMENTS_PROPERTY (property)),
			   gdata_documents_property_get_value (GDATA_DOCUMENTS_PROPERTY (property)),
			   gdata_documents_property_get_visibility (GDATA_DOCUMENTS_PROPERTY (property))
		);
	}
}

void
add_dummy_properties (GDataDocumentsEntry *entry) {
	GDataDocumentsProperty *p1, *p2, *p3, *p4;

	p1 = gdata_documents_property_new ("1");
	if (p1 != NULL) {
		gdata_documents_property_set_visibility (p1, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC);
		gdata_documents_property_set_value (p1, "ONE");
		gdata_documents_entry_add_property (entry, p1);
	};

	p2 = gdata_documents_property_new ("2");
	if (p2 != NULL) {
		gdata_documents_property_set_visibility (p2, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE);
		gdata_documents_property_set_value (p2, "TWO");
		gdata_documents_entry_add_property (entry, p2);
	};

	p3 = gdata_documents_property_new ("3");
	if (p3 != NULL) {
		gdata_documents_property_set_visibility (p3, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC);
		gdata_documents_entry_add_property (entry, p3);
	};

	p4 = gdata_documents_property_new ("4");
	if (p4 != NULL) {
		gdata_documents_property_set_visibility (p4, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE);
		gdata_documents_entry_add_property (entry, p4);
	};

	g_clear_object (&p1);
	g_clear_object (&p2);
	g_clear_object (&p3);
	g_clear_object (&p4);
}

void
remove_dummy_properties (GDataDocumentsEntry *entry) {
	GDataDocumentsProperty *p1, *p2, *p3, *p4;

	p1 = gdata_documents_property_new ("1");
	if (p1 != NULL) {
		gdata_documents_property_set_visibility (p1, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC);
		gdata_documents_entry_remove_property (entry, p1);
	};

	p2 = gdata_documents_property_new ("2");
	if (p2 != NULL) {
		gdata_documents_property_set_visibility (p2, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE);
		gdata_documents_entry_remove_property (entry, p2);
	};

	p3 = gdata_documents_property_new ("3");
	if (p3 != NULL) {
		gdata_documents_property_set_visibility (p3, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC);
		gdata_documents_entry_remove_property (entry, p3);
	};

	p4 = gdata_documents_property_new ("4");
	if (p4 != NULL) {
		gdata_documents_property_set_visibility (p4, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE);
		gdata_documents_entry_remove_property (entry, p4);
	};

	g_clear_object (&p1);
	g_clear_object (&p2);
	g_clear_object (&p3);
	g_clear_object (&p4);
}
