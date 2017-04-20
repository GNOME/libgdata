/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Peteris Krisjanis 2013 <pecisk@gmail.com>
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
 * SECTION:gdata-tasks-tasklist
 * @short_description: GData Tasks tasklist object
 * @stability: Stable
 * @include: gdata/services/tasks/gdata-tasks-tasklist.h
 *
 * #GDataTasksTasklist is a subclass of #GDataEntry to represent a tasklist from Google Tasks.
 *
 * For more details of Google Tasks API, see the <ulink type="http" url="https://developers.google.com/google-apps/tasks/v1/reference/">
 * online documentation</ulink>.
 *
 * Since: 0.15.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-tasks-tasklist.h"
#include "gdata-private.h"
#include "gdata-types.h"

static void get_json (GDataParsable *parsable, JsonBuilder *builder);
static const gchar *get_content_type (void);

G_DEFINE_TYPE (GDataTasksTasklist, gdata_tasks_tasklist, GDATA_TYPE_ENTRY)

static void
gdata_tasks_tasklist_class_init (GDataTasksTasklistClass *klass)
{
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	parsable_class->get_json = get_json;
	parsable_class->get_content_type = get_content_type;

	entry_class->kind_term = "tasks#taskList";
}

static void
gdata_tasks_tasklist_init (GDataTasksTasklist *self)
{
	/* Empty */
}

static void
get_json (GDataParsable *parsable, JsonBuilder *builder)
{
	GList *i;
	GDataLink *_link;
	GDataEntry *entry = GDATA_ENTRY (parsable);

	/* Add all the general JSON. We can’t chain up to #GDataEntry here
	 * because Google Tasks uses a different date format. */
	json_builder_set_member_name (builder, "title");
	json_builder_add_string_value (builder, gdata_entry_get_title (entry));

	if (gdata_entry_get_id (entry)) {
		json_builder_set_member_name (builder, "id");
		json_builder_add_string_value (builder, gdata_entry_get_id (entry));
	}

	if (gdata_entry_get_updated (entry) != -1) {
		gchar *updated = gdata_parser_int64_to_iso8601_numeric_timezone (gdata_entry_get_updated (entry));
		json_builder_set_member_name (builder, "updated");
		json_builder_add_string_value (builder, updated);
		g_free (updated);
	}

	/* If we have a "kind" category, add that. */
	for (i = gdata_entry_get_categories (entry); i != NULL; i = i->next) {
		GDataCategory *category = GDATA_CATEGORY (i->data);

		if (g_strcmp0 (gdata_category_get_scheme (category), "http://schemas.google.com/g/2005#kind") == 0) {
			json_builder_set_member_name (builder, "kind");
			json_builder_add_string_value (builder, gdata_category_get_term (category));
		}
	}

	/* Add the ETag, if available. */
	if (gdata_entry_get_etag (entry) != NULL) {
		json_builder_set_member_name (builder, "etag");
		json_builder_add_string_value (builder, gdata_entry_get_etag (entry));
	}

	/* Add the self-link. */
	_link = gdata_entry_look_up_link (GDATA_ENTRY (parsable), GDATA_LINK_SELF);
	if (_link != NULL) {
		json_builder_set_member_name (builder, "selfLink");
		json_builder_add_string_value (builder, gdata_link_get_uri (_link));
	}
}

static const gchar *
get_content_type (void)
{
	return "application/json";
}

/**
 * gdata_tasks_tasklist_new:
 * @id: (allow-none): the tasklist's ID, or %NULL
 *
 * Creates a new #GDataTasksTasklist with the given ID and default properties.
 *
 * Return value: (transfer full): a new #GDataTasksTasklist; unref with g_object_unref()
 *
 * Since: 0.15.0
 */
GDataTasksTasklist *
gdata_tasks_tasklist_new (const gchar *id)
{
	return GDATA_TASKS_TASKLIST (g_object_new (GDATA_TYPE_TASKS_TASKLIST, "id", id, NULL));
}
