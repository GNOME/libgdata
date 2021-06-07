/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Thibault Saunier 2009 <saunierthibault@gmail.com>
 * Copyright (C) Philip Withnall 2009–2010, 2014 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-access-rule
 * @short_description: GData access rule object
 * @stability: Stable
 * @include: gdata/gdata-access-rule.h
 *
 * #GDataAccessRule is a subclass of #GDataEntry to represent a generic access rule from an access control list (ACL).
 * It is returned by the ACL methods implemented in the #GDataAccessHandler interface.
 *
 * Access rules should be inserted to the %GDATA_LINK_ACCESS_CONTROL_LIST URI of the feed or entry they should be applied to. This will return a
 * %GDATA_SERVICE_ERROR_CONFLICT error if a rule already exists on that feed or entry for that scope type and value.
 *
 * <example>
 * 	<title>Adding a Rule to the Access Control List for an Entry</title>
 * 	<programlisting>
 * 	GDataAuthorizationDomain *domain;
 *	GDataService *service;
 *	GDataEntry *entry;
 *	GDataFeed *acl_feed;
 *	GDataAccessRule *rule, *new_rule;
 *	GError *error = NULL;
 *
 *	domain = gdata_documents_service_get_primary_authorization_domain ();
 *
 *	/<!-- -->* Retrieve a GDataEntry which will have a new rule inserted into its ACL. *<!-- -->/
 *	service = build_my_service ();
 *	entry = get_the_entry (service);
 *
 *	/<!-- -->* Create and insert a new access rule for example@gmail.com which grants them _no_ permissions on the entry.
 *	 * In a real application, the GDataEntry subclass would define its own access roles which are more useful. For example,
 *	 * GDataDocumentsEntry defines access roles for users who can read (but not write) a Google Document, and users who
 *	 * can also write to the document. *<!-- -->/
 *	rule = gdata_access_rule_new (NULL);
 *	gdata_access_rule_set_role (rule, GDATA_ACCESS_ROLE_NONE); /<!-- -->* or, for example, GDATA_DOCUMENTS_ACCESS_ROLE_READER *<!-- -->/
 *	gdata_access_rule_set_scope (rule, GDATA_ACCESS_SCOPE_USER, "example@gmail.com"); /<!-- -->* e-mail address of the user the ACL applies to *<!-- -->/
 *
 *	acl_link = gdata_entry_look_up_link (entry, GDATA_LINK_ACCESS_CONTROL_LIST);
 *	new_rule = GDATA_ACCESS_RULE (gdata_service_insert_entry (GDATA_SERVICE (service), domain, gdata_link_get_uri (acl_link),
 *	                                                          GDATA_ENTRY (rule), NULL, &error));
 *
 *	g_object_unref (rule);
 *	g_object_unref (entry);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error inserting access rule: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	/<!-- -->* Potentially do something with the new_rule here, such as store its ID for later use. *<!-- -->/
 *
 *	g_object_unref (new_rule);
 * 	</programlisting>
 * </example>
 *
 * Since: 0.3.0
 */

#include <config.h>
#include <glib.h>
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
static gboolean post_parse_xml (GDataParsable *parsable, gpointer user_data, GError **error);

struct _GDataAccessRulePrivate {
	gchar *role;
	gchar *scope_type;
	gchar *scope_value;
	gint64 edited;
	gchar *key;
};

enum {
	PROP_ROLE = 1,
	PROP_SCOPE_TYPE,
	PROP_SCOPE_VALUE,
	PROP_EDITED,
	PROP_ETAG,
	PROP_KEY,
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataAccessRule, gdata_access_rule, GDATA_TYPE_ENTRY)

static void
gdata_access_rule_class_init (GDataAccessRuleClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	gobject_class->constructor = gdata_access_rule_constructor;
	gobject_class->finalize = gdata_access_rule_finalize;
	gobject_class->get_property = gdata_access_rule_get_property;
	gobject_class->set_property = gdata_access_rule_set_property;

	parsable_class->parse_xml = parse_xml;
	parsable_class->post_parse_xml = post_parse_xml;
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
	 */
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
	 */
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
	 * This must be %NULL if and only if #GDataAccessRule:scope-type is %GDATA_ACCESS_SCOPE_DEFAULT.
	 *
	 * Since: 0.3.0
	 */
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
	 */
	g_object_class_install_property (gobject_class, PROP_EDITED,
	                                 g_param_spec_int64 ("edited",
	                                                     "Edited", "The last time the access rule was edited.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataAccessRule:key:
	 *
	 * An optional authorisation key required to access this item with the given scope. If set, this restricts
	 * access to those principals who have a copy of the key. The key is generated server-side and cannot be
	 * modified by the client. If no authorisation key is set (and hence none is needed for access to the item),
	 * this will be %NULL.
	 *
	 * Since: 0.16.0
	 */
	g_object_class_install_property (gobject_class, PROP_KEY,
	                                 g_param_spec_string ("key",
	                                                      "Key", "An optional authorisation key required to access this item.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/* Override the ETag property since ETags don't seem to be supported for ACL entries. TODO: Investigate this further (might only be
	 * unsupported for Google Calendar). */
	g_object_class_override_property (gobject_class, PROP_ETAG, "etag");
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
	self->priv = gdata_access_rule_get_instance_private (self);
	self->priv->edited = -1;

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
		priv->edited = g_get_real_time () / G_USEC_PER_SEC;

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
	g_free (priv->key);

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
			g_value_set_int64 (value, priv->edited);
			break;
		case PROP_ETAG:
			/* Never return an ETag */
			g_value_set_string (value, NULL);
			break;
		case PROP_KEY:
			g_value_set_string (value, priv->key);
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
		case PROP_ETAG:
			/* Never set an ETag (note that this doesn't stop it being set in GDataEntry due to XML parsing) */
			break;
		case PROP_KEY:
			/* Read only; fall through */
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
	    gdata_parser_int64_time_from_element (node, "edited", P_REQUIRED | P_NO_DUPES, &(self->priv->edited), &success, error) == TRUE) {
		return success;
	} else if (gdata_parser_is_namespace (node, "http://schemas.google.com/acl/2007") == TRUE) {
		if (xmlStrcmp (node->name, (xmlChar*) "role") == 0) {
			/* gAcl:role */
			xmlChar *role = xmlGetProp (node, (xmlChar*) "value");
			if (role == NULL || *role == '\0') {
				xmlFree (role);
				return gdata_parser_error_required_property_missing (node, "value", error);
			}
			self->priv->role = (gchar*) role;
		} else if (xmlStrcmp (node->name, (xmlChar*) "scope") == 0) {
			/* gAcl:scope */
			xmlChar *scope_type, *scope_value;

			scope_type = xmlGetProp (node, (xmlChar*) "type");
			if (scope_type == NULL || *scope_type == '\0') {
				xmlFree (scope_type);
				return gdata_parser_error_required_property_missing (node, "type", error);
			}

			scope_value = xmlGetProp (node, (xmlChar*) "value");

			/* The @value property is required for all scope types except "default".
			 * See: https://developers.google.com/google-apps/calendar/v2/reference#gacl_reference */
			if (xmlStrcmp (scope_type, (xmlChar*) GDATA_ACCESS_SCOPE_DEFAULT) != 0 && scope_value == NULL) {
				xmlFree (scope_type);
				return gdata_parser_error_required_property_missing (node, "value", error);
			}

			self->priv->scope_type = (gchar*) scope_type;
			self->priv->scope_value = (gchar*) scope_value;
		} else if (xmlStrcmp (node->name, (xmlChar*) "withKey") == 0) {
			/* gAcl:withKey */
			gboolean found_role = FALSE;
			xmlNode *child;
			xmlChar *key;

			/* Extract the key. */
			key = xmlGetProp (node, (xmlChar *) "key");
			if (key == NULL) {
				return gdata_parser_error_required_property_missing (node, "key", error);
			}

			self->priv->key = (gchar *) key;

			/* Look for a gAcl:role child element. */
			for (child = node->children; child != NULL; child = child->next) {
				if (xmlStrcmp (child->name, (xmlChar*) "role") == 0) {
					xmlChar *role = xmlGetProp (child, (xmlChar *) "value");
					if (role == NULL) {
						return gdata_parser_error_required_property_missing (child, "value", error);
					}

					self->priv->role = (gchar *) role;
					found_role = TRUE;
				} else {
					/* TODO: this logic copied from gdata-parsable.c.  Re-evaluate this at some point in the future.
					 * If GeoRSS and GML support were to be used more widely, it might due to implement GML objects. */
					xmlBuffer *buffer;

					/* Unhandled XML */
					buffer = xmlBufferCreate ();
					xmlNodeDump (buffer, doc, child, 0, 0);
					g_debug ("Unhandled XML in <gAcl:withKey>: %s", (gchar *) xmlBufferContent (buffer));
					xmlBufferFree (buffer);
				}
			}

			if (!found_role) {
				return gdata_parser_error_required_element_missing ("role", "gAcl:withKey", error);
			}
		} else {
			return GDATA_PARSABLE_CLASS (gdata_access_rule_parent_class)->parse_xml (parsable, doc, node, user_data, error);
		}
	} else {
		return GDATA_PARSABLE_CLASS (gdata_access_rule_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}

	return TRUE;
}

static gboolean
post_parse_xml (GDataParsable *parsable, gpointer user_data, GError **error)
{
	GDataAccessRulePrivate *priv = GDATA_ACCESS_RULE (parsable)->priv;

	/* Check for missing required elements */
	if (gdata_entry_get_title (GDATA_ENTRY (parsable)) == NULL || *gdata_entry_get_title (GDATA_ENTRY (parsable)) == '\0')
		return gdata_parser_error_required_element_missing ("role", "entry", error);
	if (priv->scope_type == NULL)
		return gdata_parser_error_required_element_missing ("scope", "entry", error);

	return TRUE;
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataAccessRulePrivate *priv = GDATA_ACCESS_RULE (parsable)->priv;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_access_rule_parent_class)->get_xml (parsable, xml_string);

	if (priv->key != NULL) {
		/* gAcl:withKey; has to wrap gAcl:role */
		gdata_parser_string_append_escaped (xml_string, "<gAcl:withKey key='", priv->key, "'>");
	}

	if (priv->role != NULL) {
		/* gAcl:role */
		gdata_parser_string_append_escaped (xml_string, "<gAcl:role value='", priv->role, "'/>");
	}

	if (priv->key != NULL) {
		g_string_append (xml_string, "</gAcl:withKey>");
	}

	if (priv->scope_value != NULL) {
		/* gAcl:scope */
		if (priv->scope_type != NULL) {
			gdata_parser_string_append_escaped (xml_string, "<gAcl:scope type='", priv->scope_type, "'");
			gdata_parser_string_append_escaped (xml_string, " value='", priv->scope_value, "'/>");
		} else {
			gdata_parser_string_append_escaped (xml_string, "<gAcl:scope value='", priv->scope_value, "'/>");
		}
	} else {
		/* gAcl:scope of type GDATA_ACCESS_SCOPE_DEFAULT. */
		g_assert (priv->scope_type != NULL && strcmp (priv->scope_type, GDATA_ACCESS_SCOPE_DEFAULT) == 0);
		g_string_append (xml_string, "<gAcl:scope type='default'/>");
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
 */
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
 * Sets the #GDataAccessRule:role property to @role. @role must be a non-empty string, such as %GDATA_ACCESS_ROLE_NONE.
 *
 * Set @role to %NULL to unset the property in the access rule.
 *
 * Since: 0.3.0
 */
void
gdata_access_rule_set_role (GDataAccessRule *self, const gchar *role)
{
	g_return_if_fail (GDATA_IS_ACCESS_RULE (self));
	g_return_if_fail (role == NULL || *role != '\0');

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
 */
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
 * @value: (allow-none): a new scope value, or %NULL
 *
 * Sets the #GDataAccessRule:scope-type property to @type and the #GDataAccessRule:scope-value property to @value.
 *
 * Set @scope_value to %NULL to unset the #GDataAccessRule:scope-value property in the access rule. @type cannot
 * be %NULL. @scope_value must be %NULL if @type is <literal>default</literal>, and non-%NULL otherwise.
 *
 * See the
 * <ulink type="http" url="https://developers.google.com/google-apps/calendar/v3/reference/acl">online
 * documentation</ulink> for more information.
 *
 * Since: 0.3.0
 */
void
gdata_access_rule_set_scope (GDataAccessRule *self, const gchar *type, const gchar *value)
{
	g_return_if_fail (GDATA_IS_ACCESS_RULE (self));
	g_return_if_fail (type != NULL && *type != '\0');
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
 * @type: (out callee-allocates) (transfer none) (allow-none): return location for the scope type, or %NULL
 * @value: (out callee-allocates) (transfer none) (allow-none): return location for the scope value, or %NULL
 *
 * Gets the #GDataAccessRule:scope-type and #GDataAccessRule:scope-value properties.
 *
 * Since: 0.3.0
 */
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
 *
 * Gets the #GDataAccessRule:edited property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the UNIX timestamp for the time the access rule was last edited, or <code class="literal">-1</code>
 *
 * Since: 0.7.0
 */
gint64
gdata_access_rule_get_edited (GDataAccessRule *self)
{
	g_return_val_if_fail (GDATA_IS_ACCESS_RULE (self), -1);
	return self->priv->edited;
}

void
_gdata_access_rule_set_key (GDataAccessRule *self, const gchar *key)
{
	g_return_if_fail (GDATA_IS_ACCESS_RULE (self));

	if (g_strcmp0 (key, self->priv->key) == 0)
		return;

	g_free (self->priv->key);
	self->priv->key = g_strdup (key);

	g_object_notify (G_OBJECT (self), "key");
}

/**
 * gdata_access_rule_get_key:
 * @self: a #GDataAccessRule
 *
 * Gets the #GDataAccessRule:key property.
 *
 * Return value: the access rule's authorisation key, or %NULL
 *
 * Since: 0.16.0
 */
const gchar *
gdata_access_rule_get_key (GDataAccessRule *self)
{
	g_return_val_if_fail (GDATA_IS_ACCESS_RULE (self), NULL);
	return self->priv->key;
}
