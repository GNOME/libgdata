/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Red Hat, Inc. 2015
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
 * SECTION:gdata-documents-access-rule
 * @short_description: GData Documents access rule object
 * @stability: Stable
 * @include: gdata/services/documents/gdata-documents-access-rule.h
 *
 * #GDataDocumentsAccessRule is a subclass of #GDataAccessRule to represent an
 * access rule affecting users of a Google Documents entry.
 *
 * Since: 0.17.2
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "gdata-documents-access-rule.h"
#include "gdata-parser.h"
#include "gdata-private.h"

static void gdata_documents_access_rule_finalize (GObject *object);
static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);
static gboolean post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error);
static const gchar *get_content_type (void);
static void get_json (GDataParsable *parsable, JsonBuilder *builder);

typedef struct {
	gchar *domain;
	gchar *email;
	gchar *scope_type;
} GDataDocumentsAccessRulePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GDataDocumentsAccessRule, gdata_documents_access_rule, GDATA_TYPE_ACCESS_RULE)

static void
gdata_documents_access_rule_class_init (GDataDocumentsAccessRuleClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->finalize = gdata_documents_access_rule_finalize;
	parsable_class->parse_json = parse_json;
	parsable_class->post_parse_json = post_parse_json;
	parsable_class->get_content_type = get_content_type;
	parsable_class->get_json = get_json;
}

static void
gdata_documents_access_rule_init (GDataDocumentsAccessRule *self)
{
	/* Nothing to do here. */
}

static void
gdata_documents_access_rule_finalize (GObject *object)
{
	GDataDocumentsAccessRulePrivate *priv;

	priv = gdata_documents_access_rule_get_instance_private (GDATA_DOCUMENTS_ACCESS_RULE (object));

	g_free (priv->domain);
	g_free (priv->email);
	g_free (priv->scope_type);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_documents_access_rule_parent_class)->finalize (object);
}

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	GDataDocumentsAccessRulePrivate *priv;
	gboolean success;
	gchar *key = NULL;
	gchar *role = NULL;
	gchar *scope_type = NULL;

	priv = gdata_documents_access_rule_get_instance_private (GDATA_DOCUMENTS_ACCESS_RULE (parsable));

	/* JSON format: https://developers.google.com/drive/v2/reference/permissions */

	if (gdata_parser_string_from_json_member (reader, "emailAddress", P_REQUIRED | P_NON_EMPTY, &(priv->email), &success, error) == TRUE ||
	    gdata_parser_string_from_json_member (reader, "domain", P_REQUIRED | P_NON_EMPTY, &(priv->domain), &success, error) == TRUE) {
		return success;
	} else if (gdata_parser_string_from_json_member (reader, "authKey", P_REQUIRED | P_NON_EMPTY, &key, &success, error) == TRUE) {
		if (success && key != NULL && key[0] != '\0')
			_gdata_access_rule_set_key (GDATA_ACCESS_RULE (parsable), key);

		g_free (key);
		return success;
	} else if (gdata_parser_string_from_json_member (reader, "role", P_REQUIRED | P_NON_EMPTY, &role, &success, error) == TRUE) {
		if (success && role != NULL && role[0] != '\0')
			gdata_access_rule_set_role (GDATA_ACCESS_RULE (parsable), role);

		g_free (role);
		return success;
	} else if (gdata_parser_string_from_json_member (reader, "type", P_REQUIRED | P_NON_EMPTY, &scope_type, &success, error) == TRUE) {
		if (g_strcmp0 (scope_type, "anyone") == 0) {
			priv->scope_type = g_strdup (GDATA_ACCESS_SCOPE_DEFAULT);
		} else {
			priv->scope_type = scope_type;
			scope_type = NULL;
		}

		g_free (scope_type);
		return success;
	}

	return GDATA_PARSABLE_CLASS (gdata_documents_access_rule_parent_class)->parse_json (parsable, reader, user_data, error);
}

static gboolean
post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error)
{
	GDataDocumentsAccessRulePrivate *priv;

	priv = gdata_documents_access_rule_get_instance_private (GDATA_DOCUMENTS_ACCESS_RULE (parsable));

	if (g_strcmp0 (priv->scope_type, GDATA_ACCESS_SCOPE_DEFAULT) == 0) {
		gdata_access_rule_set_scope (GDATA_ACCESS_RULE (parsable), priv->scope_type, NULL);
	} else if (g_strcmp0 (priv->scope_type, "group") == 0 || g_strcmp0 (priv->scope_type, GDATA_ACCESS_SCOPE_USER) == 0) {
		if (priv->email == NULL || priv->email[0] == '\0') {
			g_set_error (error, GDATA_PARSER_ERROR, GDATA_PARSER_ERROR_PARSING_STRING,
			             /* Translators: the parameter is an error message */
			             _("Error parsing JSON: %s"),
			             "Permission type ‘group’ or ‘user’ needs an ‘emailAddress’ property.");
			return FALSE;
		} else {
			gdata_access_rule_set_scope (GDATA_ACCESS_RULE (parsable), priv->scope_type, priv->email);
		}
	} else if (g_strcmp0 (priv->scope_type, GDATA_ACCESS_SCOPE_DOMAIN) == 0) {
		if (priv->domain == NULL || priv->domain[0] == '\0') {
			g_set_error (error, GDATA_PARSER_ERROR, GDATA_PARSER_ERROR_PARSING_STRING,
			             /* Translators: the parameter is an error message */
			             _("Error parsing JSON: %s"),
			             "Permission type ‘domain’ needs a ‘domain’ property.");
			return FALSE;
		} else {
			gdata_access_rule_set_scope (GDATA_ACCESS_RULE (parsable), priv->scope_type, priv->domain);
		}
	}

	return TRUE;
}

static const gchar *
get_content_type (void)
{
	return "application/json";
}

static void
get_json (GDataParsable *parsable, JsonBuilder *builder)
{
	const gchar *key;
	const gchar *role;
	const gchar *scope_type;
	const gchar *scope_value;

	GDATA_PARSABLE_CLASS (gdata_documents_access_rule_parent_class)->get_json (parsable, builder);

	key = gdata_access_rule_get_key (GDATA_ACCESS_RULE (parsable));
	if (key != NULL && key[0] != '\0') {
		json_builder_set_member_name (builder, "authKey");
		json_builder_add_string_value (builder, key);
	}

	role = gdata_access_rule_get_role (GDATA_ACCESS_RULE (parsable));
	if (role != NULL && role[0] != '\0') {
		json_builder_set_member_name (builder, "role");
		json_builder_add_string_value (builder, role);
	}

	gdata_access_rule_get_scope (GDATA_ACCESS_RULE (parsable), &scope_type, &scope_value);

	if (scope_type != NULL && scope_type[0] != '\0') {
		if (g_strcmp0 (scope_type, GDATA_ACCESS_SCOPE_DEFAULT) == 0)
			scope_type = "anyone";

		json_builder_set_member_name (builder, "type");
		json_builder_add_string_value (builder, scope_type);
	}

	if (scope_value != NULL && scope_value[0] != '\0') {
		json_builder_set_member_name (builder, "value");
		json_builder_add_string_value (builder, scope_value);
	}
}

/**
 * gdata_documents_access_rule_new:
 * @id: the access rule's ID, or %NULL
 *
 * Creates a new #GDataDocumentsAccessRule with the given ID and default
 * properties.
 *
 * Return value: (transfer full): a new #GDataDocumentsAccessRule; unref with
 *   g_object_unref()
 *
 * Since: 0.17.2
 */
GDataDocumentsAccessRule *
gdata_documents_access_rule_new (const gchar *id)
{
	GObject *retval = NULL;  /* owned */

	retval = g_object_new (GDATA_TYPE_DOCUMENTS_ACCESS_RULE,
	                       "id", id,
	                       NULL);
	return GDATA_DOCUMENTS_ACCESS_RULE (retval);
}
