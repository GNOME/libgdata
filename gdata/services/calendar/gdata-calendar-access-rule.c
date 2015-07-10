/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2015 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-calendar-access-rule
 * @short_description: GData Calendar access rule object
 * @stability: Stable
 * @include: gdata/services/calendar/gdata-calendar-access-rule.h
 *
 * #GDataCalendarAccessRule is a subclass of #GDataAccessRule to represent an
 * access rule affecting users of a shared calendar in Google Calendar.
 *
 * Since: 0.17.2
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-access-rule.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "gdata-private.h"
#include "gdata-calendar-access-rule.h"

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data,
            GError **error);
static gboolean
post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error);
static void
get_json (GDataParsable *parsable, JsonBuilder *builder);
static const gchar *
get_content_type (void);

G_DEFINE_TYPE (GDataCalendarAccessRule, gdata_calendar_access_rule,
               GDATA_TYPE_ACCESS_RULE)

static void
gdata_calendar_access_rule_class_init (GDataCalendarAccessRuleClass *klass)
{
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	parsable_class->parse_json = parse_json;
	parsable_class->post_parse_json = post_parse_json;
	parsable_class->get_json = get_json;
	parsable_class->get_content_type = get_content_type;

	entry_class->kind_term = "calendar#aclRule";
}

static void
gdata_calendar_access_rule_init (GDataCalendarAccessRule *self)
{
	/* Nothing to do here. */
}

/* V3 reference:
 * https://developers.google.com/google-apps/calendar/v3/reference/acl#role
 * V2 reference is no longer available.
 */
const struct {
	const gchar *v3;
	const gchar *v2;
} role_pairs[] = {
	{ "none", "none" },
	{ "freeBusyReader", "http://schemas.google.com/gCal/2005#freebusy" },
	{ "reader", "http://schemas.google.com/gCal/2005#read" },
	{ "writer", "http://schemas.google.com/gCal/2005#editor" },
	{ "owner", "http://schemas.google.com/gCal/2005#owner" },
};

static const gchar *
role_v3_to_v2 (const gchar *v3_role)
{
	guint i;

	for (i = 0; i < G_N_ELEMENTS (role_pairs); i++) {
		if (g_strcmp0 (v3_role, role_pairs[i].v3) == 0) {
			return role_pairs[i].v2;
		}
	}

	/* Fallback. */
	return v3_role;
}

static const gchar *
role_v2_to_v3 (const gchar *v2_role)
{
	guint i;

	for (i = 0; i < G_N_ELEMENTS (role_pairs); i++) {
		if (g_strcmp0 (v2_role, role_pairs[i].v2) == 0) {
			return role_pairs[i].v3;
		}
	}

	/* Fallback. */
	return v2_role;
}

static const gchar *
scope_type_v3_to_v2 (const gchar *v3_scope_type)
{
	/* Surprisingly, they have not changed from v2 to v3. */
	return v3_scope_type;
}

static const gchar *
scope_type_v2_to_v3 (const gchar *v2_scope_type)
{
	/* Surprisingly, they have not changed from v2 to v3. */
	return v2_scope_type;
}

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	gboolean success;

	if (g_strcmp0 (json_reader_get_member_name (reader), "role") == 0) {
		gchar *role = NULL;  /* owned */

		g_assert (gdata_parser_string_from_json_member (reader, "role",
		                                                P_REQUIRED |
		                                                P_NON_EMPTY,
		                                                &role, &success,
		                                                error));

		if (!success) {
			return FALSE;
		}

		gdata_access_rule_set_role (GDATA_ACCESS_RULE (parsable),
		                            role_v3_to_v2 (role));
		g_free (role);

		return TRUE;
	} else if (g_strcmp0 (json_reader_get_member_name (reader),
	                                                   "scope") == 0) {
		const gchar *scope_type;
		const gchar *scope_value;

		/* Check this is an object. */
		if (!json_reader_is_object (reader)) {
			return gdata_parser_error_required_json_content_missing (reader,
			                                                         error);
		}

		json_reader_read_member (reader, "type");
		scope_type = json_reader_get_string_value (reader);
		json_reader_end_member (reader);

		json_reader_read_member (reader, "value");
		scope_value = json_reader_get_string_value (reader);
		json_reader_end_member (reader);

		/* Scope type is required. */
		if (scope_type == NULL) {
			return gdata_parser_error_required_json_content_missing (reader,
			                                                         error);
		}

		gdata_access_rule_set_scope (GDATA_ACCESS_RULE (parsable),
		                             scope_type_v3_to_v2 (scope_type),
		                             scope_value);

		return TRUE;
	}

	return GDATA_PARSABLE_CLASS (gdata_calendar_access_rule_parent_class)->parse_json (parsable, reader, user_data, error);
}

static gboolean
post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error)
{
	/* Do _not_ chain up. */
	return TRUE;
}

static void
get_json (GDataParsable *parsable, JsonBuilder *builder)
{
	GDataAccessRule *access_rule;
	const gchar *id, *etag, *role, *scope_type, *scope_value;

	access_rule = GDATA_ACCESS_RULE (parsable);

	id = gdata_entry_get_id (GDATA_ENTRY (parsable));
	if (id != NULL) {
		json_builder_set_member_name (builder, "id");
		json_builder_add_string_value (builder, id);
	}

	json_builder_set_member_name (builder, "kind");
	json_builder_add_string_value (builder, "calendar#aclRule");

	/* Add the ETag, if available. */
	etag = gdata_entry_get_etag (GDATA_ENTRY (parsable));
	if (etag != NULL) {
		json_builder_set_member_name (builder, "etag");
		json_builder_add_string_value (builder, etag);
	}

	role = gdata_access_rule_get_role (access_rule);
	if (role != NULL) {
		json_builder_set_member_name (builder, "role");
		json_builder_add_string_value (builder, role_v2_to_v3 (role));
	}

	gdata_access_rule_get_scope (access_rule, &scope_type, &scope_value);
	if (scope_type != NULL || scope_value != NULL) {
		json_builder_set_member_name (builder, "scope");
		json_builder_begin_object (builder);

		if (scope_type != NULL) {
			json_builder_set_member_name (builder, "type");
			json_builder_add_string_value (builder,
			                               scope_type_v2_to_v3 (scope_type));
		}

		if (scope_value != NULL) {
			json_builder_set_member_name (builder, "value");
			json_builder_add_string_value (builder, scope_value);
		}

		json_builder_end_object (builder);
	}
}

static const gchar *
get_content_type (void)
{
	return "application/json";
}

/**
 * gdata_calendar_access_rule_new:
 * @id: the access rule's ID, or %NULL
 *
 * Creates a new #GDataCalendarAccessRule with the given ID and default
 * properties.
 *
 * Return value: (transfer full): a new #GDataCalendarAccessRule; unref with
 *   g_object_unref()
 *
 * Since: 0.17.2
 */
GDataCalendarAccessRule *
gdata_calendar_access_rule_new (const gchar *id)
{
	GObject *retval = NULL;  /* owned */

	retval = g_object_new (GDATA_TYPE_CALENDAR_ACCESS_RULE,
	                       "id", id,
	                       NULL);
	return GDATA_CALENDAR_ACCESS_RULE (retval);
}
