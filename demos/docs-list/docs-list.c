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

		for (l = entries; l != NULL; l = l->next) {
			GDataEntry *entry = GDATA_ENTRY (l->data);
			const gchar *title;

			title = gdata_entry_get_title (entry);
			g_message ("%s", title);
		}

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
