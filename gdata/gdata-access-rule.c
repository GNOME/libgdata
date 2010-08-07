/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Thibault Saunier 2009 <saunierthibault@gmail.com>
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

/**
 * SECTION:gdata-access-rule
 * @short_description: GData access rule object
 * @stability: Unstable
 * @include: gdata/gdata-access-rule.h
 *
 * #GDataAccessRule is a subclass of #GDataEntry to represent a generic access rule from an access control list (ACL).
 * It is returned by the ACL methods implemented in the #GDataAccessHandler interface.
 *
 * Since: 0.3.0
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-access-rule.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "gdata-private.h"

static GObject *gdata_access_rule_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params);
static void gdata_access_rule_finalize (GObject *object);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static void gdata_access_rule_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void gdata_access_rule_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);

struct _GDataAccessRulePrivate {
	gchar *role;
	gchar *scope_type;
	gchar *scope_value;
	GTimeVal edited;
};

enum {
	PROP_ROLE = 1,
	PROP_SCOPE_TYPE,
	PROP_SCOPE_VALUE,
	PROP_EDITED
};

G_DEFINE_TYPE (GDataAccessRule, gdata_access_rule, GDATA_TYPE_ENTRY)

static void
gdata_access_rule_class_init (GDataAccessRuleClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataAccessRulePrivate));

	gobject_class->constructor = gdata_access_rule_constructor;
	gobject_class->finalize = gdata_access_rule_finalize;
	gobject_class->get_property = gdata_access_rule_get_property;
	gobject_class->set_property = gdata_access_rule_set_property;

	parsable_class->parse_xml = parse_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;

	entry_class->kind_term = "http://schemas.google.com/acl/2007#accessRule";

	/**
	 * GDataAccessRule:role:
	 *
	 * The role of the person concerned by this ACL. By default, this can only be %GDATA_ACCESS_ROLE_NONE. Services may extend it with
	 * their own namespaced roles.
	 *
	 * Since: 0.3.0
	 **/
	g_object_class_install_property (gobject_class, PROP_ROLE,
	                                 g_param_spec_string ("role",
	                                                      "Role", "The role of the person concerned by this ACL.",
	                                                      GDATA_ACCESS_ROLE_NONE,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataAccessRule:scope-type:
	 *
	 * Specifies to whom this access rule applies. For example, %GDATA_ACCESS_SCOPE_USER or %GDATA_ACCESS_SCOPE_DEFAULT.
	 *
	 * Since: 0.3.0
	 **/
	g_object_class_install_property (gobject_class, PROP_SCOPE_TYPE,
	                                 g_param_spec_string ("scope-type",
	                                                      "Scope type", "Specifies to whom this access rule applies.",
	                                                      GDATA_ACCESS_SCOPE_DEFAULT,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataAccessRule:scope-value:
	 *
	 * A value representing the user who is represented by the access rule, such as an
	 * e-mail address for users, or a domain name for domains.
	 *
	 * Since: 0.3.0
	 **/
	g_object_class_install_property (gobject_class, PROP_SCOPE_VALUE,
	                                 g_param_spec_string ("scope-value",
	                                                      "Scope value", "The scope value for this access rule.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataAccessRule:edited:
	 *
	 * The last time the access rule was edited. If the rule has not been edited yet, the content indicates the time it was created.
	 *
	 * For more information, see the <ulink type="http" url="http://www.atomenabled.org/developers/protocol/#appEdited">
	 * Atom Publishing Protocol specification</ulink>.
	 *
	 * Since: 0.7.0
	 **/
	g_object_class_install_property (gobject_class, PROP_EDITED,
	                                 g_param_spec_boxed ("edited",
	                                                     "Edited", "The last time the access rule was edited.",
	                                                     GDATA_TYPE_G_TIME_VAL,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void notify_role_cb (GDataAccessRule *self, GParamSpec *pspec, gpointer user_data);

static void
notify_title_cb (GDataAccessRule *self, GParamSpec *pspec, gpointer user_data)
{
	/* Update GDataAccessRule:role */
	g_signal_handlers_block_by_func (self, notify_role_cb, self);
	gdata_access_rule_set_role (self, gdata_entry_get_title (GDATA_ENTRY (self)));
	g_signal_handlers_unblock_by_func (self, notify_role_cb, self);
}

static void
notify_role_cb (GDataAccessRule *self, GParamSpec *pspec, gpointer user_data)
{
	/* Update GDataEntry:title */
	g_signal_handlers_block_by_func (self, notify_title_cb, self);
	gdata_entry_set_title (GDATA_ENTRY (self), gdata_access_rule_get_role (self));
	g_signal_handlers_unblock_by_func (self, notify_title_cb, self);
}

static void
gdata_access_rule_init (GDataAccessRule *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_ACCESS_RULE, GDataAccessRulePrivate);

	/* Listen to change notifications for the entry's title, since it's linked to GDataAccessRule:role */
	g_signal_connect (self, "notify::title", (GCallback) notify_title_cb, self);
	g_signal_connect (self, "notify::role", (GCallback) notify_role_cb, self);
}

static GObject *
gdata_access_rule_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
	GObject *object;

	/* Chain up to the parent class */
	object = G_OBJECT_CLASS (gdata_access_rule_parent_class)->constructor (type, n_construct_params, construct_params);

	/* We can't create these in init, or they would collide with the group and control created when parsing the XML */
	if (_gdata_parsable_is_constructed_from_xml (GDATA_PARSABLE (object)) == FALSE) {
		GDataAccessRulePrivate *priv = GDATA_ACCESS_RULE (object)->priv;

		/* Set the edited property to the current time (creation time). We don't do this in *_init() since that would cause
		 * setting it from parse_xml() to fail (duplicate element). */
		g_get_current_time (&(priv->edited));

		/* Set up the role and scope type */
		priv->role = g_strdup (GDATA_ACCESS_ROLE_NONE);
		priv->scope_type = g_strdup (GDATA_ACCESS_SCOPE_DEFAULT);
	}

	return object;
}

static void
gdata_access_rule_finalize (GObject *object)
{
	GDataAccessRulePrivate *priv = GDATA_ACCESS_RULE (object)->priv;

	g_free (priv->role);
	g_free (priv->scope_type);
	g_free (priv->scope_value);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_access_rule_parent_class)->finalize (object);
}

static void
gdata_access_rule_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataAccessRulePrivate *priv = GDATA_ACCESS_RULE (object)->priv;

	switch (property_id) {
		case PROP_ROLE:
			g_value_set_string (value, priv->role);
			break;
		case PROP_SCOPE_TYPE:
			g_value_set_string (value, priv->scope_type);
			break;
		case PROP_SCOPE_VALUE:
			g_value_set_string (value, priv->scope_value);
			break;
		case PROP_EDITED:
			g_value_set_boxed (value, &(priv->edited));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_access_rule_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataAccessRule *self = GDATA_ACCESS_RULE (object);

	switch (property_id) {
		case PROP_ROLE:
			gdata_access_rule_set_role (self, g_value_get_string (value));
			break;
		case PROP_SCOPE_TYPE:
			g_free (self->priv->scope_type);
			self->priv->scope_type = g_value_dup_string (value);
			g_object_notify (object, "scope-type");
			break;
		case PROP_SCOPE_VALUE:
			g_free (self->priv->scope_value);
			self->priv->scope_value = g_value_dup_string (value);
			g_object_notify (object, "scope-value");
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	gboolean success;
	GDataAccessRule *self = GDATA_ACCESS_RULE (parsable);

	if (gdata_parser_is_namespace (node, "http://www.w3.org/2007/app") == TRUE &&
	    gdata_parser_time_val_from_element (node, "edited", P_REQUIRED | P_NO_DUPES, &(self->priv->edited), &success, error) == TRUE) {
		return success;
	} else if (gdata_parser_is_namespace (node, "http://schemas.google.com/acl/2007") == TRUE) {
		if (xmlStrcmp (node->name, (xmlChar*) "role") == 0) {
			/* gAcl:role */
			xmlChar *role = xmlGetProp (node, (xmlChar*) "value");
			if (role == NULL)
				return gdata_parser_error_required_property_missing (node, "value", error);
			self->priv->role = (gchar*) role;
		} else if (xmlStrcmp (node->name, (xmlChar*) "scope") == 0) {
			/* gAcl:scope */
			xmlChar *scope_type, *scope_value;

			scope_type = xmlGetProp (node, (xmlChar*) "type");
			if (scope_type == NULL)
				return gdata_parser_error_required_property_missing (node, "type", error);

			scope_value = xmlGetProp (node, (xmlChar*) "value");

			if (xmlStrcmp (scope_type, (xmlChar*) GDATA_ACCESS_SCOPE_DEFAULT) == 0 && scope_value == NULL) {
				xmlFree (scope_type);
				return gdata_parser_error_required_property_missing (node, "value", error);
			}

			self->priv->scope_type = (gchar*) scope_type;
			self->priv->scope_value = (gchar*) scope_value;
		} else {
			return GDATA_PARSABLE_CLASS (gdata_access_rule_parent_class)->parse_xml (parsable, doc, node, user_data, error);
		}
	} else {
		return GDATA_PARSABLE_CLASS (gdata_access_rule_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}

	return TRUE;
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataAccessRulePrivate *priv = GDATA_ACCESS_RULE (parsable)->priv;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_access_rule_parent_class)->get_xml (parsable, xml_string);

	if (priv->role != NULL)
		/* gAcl:role */
		g_string_append_printf (xml_string, "<gAcl:role value='%s'/>", priv->role);

	if (priv->scope_value != NULL){
		/* gAcl:scope */
		if (priv->scope_type != NULL)
			g_string_append_printf (xml_string, "<gAcl:scope type='%s' value='%s'/>", priv->scope_type, priv->scope_value);
		else
			g_string_append_printf (xml_string, "<gAcl:scope value='%s'/>", priv->scope_value);
	}
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_access_rule_parent_class)->get_namespaces (parsable, namespaces);

	g_hash_table_insert (namespaces, (gchar*) "gAcl", (gchar*) "http://schemas.google.com/acl/2007");
}

/**
 * gdata_access_rule_new:
 * @id: the access rule's ID, or %NULL
 *
 * Creates a new #GDataAccessRule with the given ID and default properties.
 *
 * Return value: a new #GDataAccessRule; unref with g_object_unref()
 *
 * Since: 0.3.0
 **/
GDataAccessRule *
gdata_access_rule_new (const gchar *id)
{
	return GDATA_ACCESS_RULE (g_object_new (GDATA_TYPE_ACCESS_RULE, "id", id, NULL));
}

/**
 * gdata_access_rule_set_role:
 * @self: a #GDataAccessRule
 * @role: a new role, or %NULL
 *
 * Sets the #GDataAccessRule:role property to @role.
 *
 * Set @role to %NULL to unset the property in the access rule.
 *
 * Since: 0.3.0
 **/
void
gdata_access_rule_set_role (GDataAccessRule *self, const gchar *role)
{
	g_return_if_fail (GDATA_IS_ACCESS_RULE (self));

	g_free (self->priv->role);
	self->priv->role = g_strdup (role);
	g_object_notify (G_OBJECT (self), "role");
}

/**
 * gdata_access_rule_get_role:
 * @self: a #GDataAccessRule
 *
 * Gets the #GDataAccessRule:role property.
 *
 * Return value: the access rule's role, or %NULL
 *
 * Since: 0.3.0
 **/
const gchar *
gdata_access_rule_get_role (GDataAccessRule *self)
{
	g_return_val_if_fail (GDATA_IS_ACCESS_RULE (self), NULL);
	return self->priv->role;
}


/**
 * gdata_access_rule_set_scope:
 * @self: a #GDataAccessRule
 * @type: a new scope type
 * @value: a new scope value, or %NULL
 *
 * Sets the #GDataAccessRule:scope-type property to @type and the #GDataAccessRule:scope-value property to @value.
 *
 * Set @scope_value to %NULL to unset the #GDataAccessRule:scope-value property in the access rule. @type cannot
 * be %NULL. @scope_value must be %NULL if @type is <literal>default</literal>, and non-%NULL otherwise.
 *
 * See the <ulink type="http" url="http://code.google.com/apis/calendar/docs/2.0/reference.html#gacl_reference">online documentation</ulink>
 * for more information.
 *
 * Since: 0.3.0
 **/
void
gdata_access_rule_set_scope (GDataAccessRule *self, const gchar *type, const gchar *value)
{
	g_return_if_fail (GDATA_IS_ACCESS_RULE (self));
	g_return_if_fail (type != NULL);
	g_return_if_fail ((strcmp (type, GDATA_ACCESS_SCOPE_DEFAULT) == 0 && value == NULL) || value != NULL);

	g_free (self->priv->scope_type);
	self->priv->scope_type = g_strdup (type);

	g_free (self->priv->scope_value);
	self->priv->scope_value = g_strdup (value);

	g_object_freeze_notify (G_OBJECT (self));
	g_object_notify (G_OBJECT (self), "scope-type");
	g_object_notify (G_OBJECT (self), "scope-value");
	g_object_thaw_notify (G_OBJECT (self));
}

/**
 * gdata_access_rule_get_scope:
 * @self: a #GDataAccessRule
 * @type: (out callee-allocates) (transfer none): return location for the scope type, or %NULL
 * @value: (out callee-allocates) (transfer none): return location for the scope value, or %NULL
 *
 * Gets the #GDataAccessRule:scope-type and #GDataAccessRule:scope-value properties.
 *
 * Since: 0.3.0
 **/
void
gdata_access_rule_get_scope (GDataAccessRule *self, const gchar **type, const gchar **value)
{
	g_return_if_fail (GDATA_IS_ACCESS_RULE (self));
	if (type != NULL)
		*type = self->priv->scope_type;
	if (value != NULL)
		*value = self->priv->scope_value;
}

/**
 * gdata_access_rule_get_edited:
 * @self: a #GDataAccessRule
 * @edited: (out caller-allocates): return location for the edited time
 *
 * Gets the #GDataAccessRule:edited property and puts it in @edited. If the property is unset,
 * both fields in the #GTimeVal will be set to <code class="literal">0</code>.
 *
 * Since: 0.7.0
 **/
void
gdata_access_rule_get_edited (GDataAccessRule *self, GTimeVal *edited)
{
	g_return_if_fail (GDATA_IS_ACCESS_RULE (self));
	g_return_if_fail (edited != NULL);
	*edited = self->priv->edited;
}
