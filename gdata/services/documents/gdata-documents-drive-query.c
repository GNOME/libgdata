/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Ondrej Holy 2020 <oholy@redhat.com>
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
 * SECTION:gdata-documents-drive-query
 * @short_description: GData Documents Drive query object
 * @stability: Stable
 * @include: gdata/services/documents/gdata-documents-drive-query.h
 *
 * #GDataDocumentsDriveQuery represents a collection of query parameters specific to shared drives, which go above and beyond
 * those catered for by #GDataQuery.
 *
 * For more information on the custom GData query parameters supported by #GDataDocumentsDriveQuery, see the
 * [online documentation](https://developers.google.com/drive/api/v2/ref-search-terms#drive_properties).
 *
 * Since: 0.18.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-documents-drive-query.h"
#include "gdata-private.h"

static void get_query_uri (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started);

G_DEFINE_TYPE (GDataDocumentsDriveQuery, gdata_documents_drive_query, GDATA_TYPE_QUERY)

static void
gdata_documents_drive_query_class_init (GDataDocumentsDriveQueryClass *klass)
{
	GDataQueryClass *query_class = GDATA_QUERY_CLASS (klass);

	query_class->get_query_uri = get_query_uri;
}

static void
gdata_documents_drive_query_init (GDataDocumentsDriveQuery *self)
{
	/* https://developers.google.com/drive/api/v2/reference/drives/list#parameters */
	_gdata_query_set_pagination_type (GDATA_QUERY (self),
	                                  GDATA_QUERY_PAGINATION_TOKENS);
}

static void
get_query_uri (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started)
{
	guint max_results;

	#define APPEND_SEP g_string_append_c (query_uri, (*params_started == FALSE) ? '?' : '&'); *params_started = TRUE;

	_gdata_query_clear_q_internal (self);

	/* Chain up to the parent class */
	GDATA_QUERY_CLASS (gdata_documents_drive_query_parent_class)->get_query_uri (self, feed_uri, query_uri, params_started);

	/* https://developers.google.com/drive/api/v2/reference/drives/list */
	max_results = gdata_query_get_max_results (self);
	if (max_results > 0) {
		APPEND_SEP
		max_results = max_results > 100 ? 100 : max_results;
		g_string_append_printf (query_uri, "maxResults=%u", max_results);
	}
}

/**
 * gdata_documents_drive_query_new:
 * @q: (nullable): a query string, or %NULL
 *
 * Creates a new #GDataDocumentsDriveQuery with its #GDataQuery:q property set to @q.
 *
 * Return value: (transfer full): a new #GDataDocumentsDriveQuery
 *
 * Since: 0.18.0
 */
GDataDocumentsDriveQuery *
gdata_documents_drive_query_new (const gchar *q)
{
	return g_object_new (GDATA_TYPE_DOCUMENTS_DRIVE_QUERY, "q", q, NULL);
}
