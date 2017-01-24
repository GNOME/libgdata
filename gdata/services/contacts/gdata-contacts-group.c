/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2010 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-contacts-group
 * @short_description: GData Contacts group object
 * @stability: Stable
 * @include: gdata/services/contacts/gdata-contacts-group.h
 *
 * #GDataContactsGroup is a subclass of #GDataEntry to represent a group from a Google address book.
 *
 * For more details of Google Contacts' GData API, see the
 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/developers_guide_protocol.html#Groups">online documentation</ulink>.
 *
 * The user-set name of the group is stored in the #GDataEntry:title property, retrievable using gdata_entry_get_title(). Note that for system groups
 * (see #GDataContactsGroup:system-group-id), this group name is provided by Google, and is not localised. Clients should provide their own localised
 * group names for the system groups.
 *
 * In addition to all the standard properties available for a group, #GDataContactsGroup supports an additional kind of property: extended
 * properties. Extended properties, set with gdata_contacts_group_set_extended_property() and retrieved with
 * gdata_contacts_group_get_extended_property(), are provided as a method of storing client-specific data which shouldn't be seen or be editable
 * by the user, such as IDs and cache times.
 *
 * <example>
 * 	<title>Adding a New Group</title>
 * 	<programlisting>
 *	GDataContactsService *service;
 *	GDataContactsGroup *group, *updated_group;
 *	GDataContactsContact *contact, *updated_contact;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service and return a contact to add to the new group. *<!-- -->/
 *	service = create_contacts_service ();
 *	contact = query_user_for_contact (service);
 *
 *	/<!-- -->* Create the new group *<!-- -->/
 *	group = gdata_contacts_group_new (NULL);
 *	gdata_entry_set_title (GDATA_ENTRY (group), "Group Name");
 *
 *	/<!-- -->* Insert the group on the server *<!-- -->/
 *	updated_group = gdata_contacts_service_insert_group (service, group, NULL, &error);
 *
 *	g_object_unref (group);
 *
 *	if (error != NULL) {
 *		g_error ("Error adding a group: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (contact);
 *		g_object_unref (service);
 *		return;
 *	}
 *
 *	/<!-- -->* Add the contact to the new group. *<!-- -->/
 *	gdata_contacts_contact_add_group (contact, gdata_entry_get_id (GDATA_ENTRY (updated_group)));
 *
 *	g_object_unref (updated_group);
 *
 *	/<!-- -->* Update the contact on the server *<!-- -->/
 *	updated_contact = GDATA_CONTACTS_CONTACT (gdata_service_update_entry (GDATA_SERVICE (service), GDATA_ENTRY (contact), NULL, &error));
 *
 *	g_object_unref (contact);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error updating contact: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	/<!-- -->* Do something with the updated contact, such as update them in the UI, or store their ID for future use. *<!-- -->/
 *
 *	g_object_unref (updated_contact);
 * 	</programlisting>
 * </example>
 *
 * Since: 0.7.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-contacts-group.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "gdata-private.h"

/* The maximum number of extended properties the server allows us. See
 * http://code.google.com/apis/contacts/docs/3.0/reference.html#ProjectionsAndExtended.
 * When updating this, make sure to update the API documentation for gdata_contacts_group_get_extended_property() and
 * gdata_contacts_group_set_extended_property(). */
#define MAX_N_EXTENDED_PROPERTIES 10

static GObject *gdata_contacts_group_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params);
static void gdata_contacts_group_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_contacts_group_finalize (GObject *object);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);
static gchar *get_entry_uri (const gchar *id) G_GNUC_WARN_UNUSED_RESULT;

struct _GDataContactsGroupPrivate {
	gint64 edited;
	GHashTable *extended_properties;
	gboolean deleted;
	gchar *system_group_id;
};

enum {
	PROP_EDITED = 1,
	PROP_DELETED,
	PROP_SYSTEM_GROUP_ID
};

G_DEFINE_TYPE (GDataContactsGroup, gdata_contacts_group, GDATA_TYPE_ENTRY)

static void
gdata_contacts_group_class_init (GDataContactsGroupClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataContactsGroupPrivate));

	gobject_class->constructor = gdata_contacts_group_constructor;
	gobject_class->get_property = gdata_contacts_group_get_property;
	gobject_class->finalize = gdata_contacts_group_finalize;

	parsable_class->parse_xml = parse_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;

	entry_class->get_entry_uri = get_entry_uri;
	entry_class->kind_term = "http://schemas.google.com/contact/2008#group";

	/**
	 * GDataContactsGroup:edited:
	 *
	 * The last time the group was edited. If the group has not been edited yet, the content indicates the time it was created.
	 *
	 * For more information, see the <ulink type="http" url="http://www.atomenabled.org/developers/protocol/#appEdited">
	 * Atom Publishing Protocol specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_EDITED,
	                                 g_param_spec_int64 ("edited",
	                                                     "Edited", "The last time the group was edited.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsGroup:deleted:
	 *
	 * Whether the entry has been deleted.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_DELETED,
	                                 g_param_spec_boolean ("deleted",
	                                                       "Deleted", "Whether the entry has been deleted.",
	                                                       FALSE,
	                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataContactsGroup:system-group-id:
	 *
	 * The system group ID for this group, if it's a system group. If the group is not a system group, this is %NULL. Otherwise, it is one of the
	 * four system group IDs: %GDATA_CONTACTS_GROUP_CONTACTS, %GDATA_CONTACTS_GROUP_FRIENDS, %GDATA_CONTACTS_GROUP_FAMILY and
	 * %GDATA_CONTACTS_GROUP_COWORKERS.
	 *
	 * If this is non-%NULL, the group name stored in #GDataEntry:title will not be localised, so clients should provide localised group names of
	 * their own for each of the system groups. Whether a group is a system group should be detected solely on the basis of the value of this
	 * property, not by comparing the group name (#GDataEntry:title) or entry ID (#GDataEntry:id). The entry ID is not the same as the system
	 * group ID.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_SYSTEM_GROUP_ID,
	                                 g_param_spec_string ("system-group-id",
	                                                      "System group ID", "The system group ID for this group, if it's a system group.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void notify_content_cb (GObject *gobject, GParamSpec *pspec, GDataContactsGroup *self);

static void
notify_title_cb (GObject *gobject, GParamSpec *pspec, GDataContactsGroup *self)
{
	/* Update GDataEntry:content */
	g_signal_handlers_block_by_func (self, notify_content_cb, self);
	gdata_entry_set_content (GDATA_ENTRY (self), gdata_entry_get_title (GDATA_ENTRY (self)));
	g_signal_handlers_unblock_by_func (self, notify_content_cb, self);
}

static void
notify_content_cb (GObject *gobject, GParamSpec *pspec, GDataContactsGroup *self)
{
	/* Update GDataEntry:title */
	g_signal_handlers_block_by_func (self, notify_title_cb, self);
	gdata_entry_set_title (GDATA_ENTRY (self), gdata_entry_get_content (GDATA_ENTRY (self)));
	g_signal_handlers_unblock_by_func (self, notify_title_cb, self);
}

static void
gdata_contacts_group_init (GDataContactsGroup *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_CONTACTS_GROUP, GDataContactsGroupPrivate);
	self->priv->extended_properties = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	self->priv->edited = -1;

	/* Listen to change notifications for the entry's title and content, since they're linked */
	g_signal_connect (self, "notify::title", (GCallback) notify_title_cb, self);
	g_signal_connect (self, "notify::content", (GCallback) notify_content_cb, self);
}

static GObject *
gdata_contacts_group_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
	GObject *object;
	guint i;

	/* Find the "id" property and ensure it's sane */
	for (i = 0; i < n_construct_params; i++) {
		GParamSpec *pspec = construct_params[i].pspec;
		GValue *value = construct_params[i].value;

		if (strcmp (g_param_spec_get_name (pspec), "id") == 0) {
			gchar *base, *id;

			id = g_value_dup_string (value);

			/* Fix the ID to refer to the full projection, rather than the base projection. */
			if (id != NULL) {
				base = strstr (id, "/base/");
				if (base != NULL)
					memcpy (base, "/full/", 6);
			}

			g_value_take_string (value, id);

			break;
		}
	}

	/* Chain up to the parent class */
	object = G_OBJECT_CLASS (gdata_contacts_group_parent_class)->constructor (type, n_construct_params, construct_params);

	if (_gdata_parsable_is_constructed_from_xml (GDATA_PARSABLE (object)) == FALSE) {
		GDataContactsGroupPrivate *priv = GDATA_CONTACTS_GROUP (object)->priv;
		GTimeVal time_val;

		/* Set the edited property to the current time (creation time). We don't do this in *_init() since that would cause setting it from
		 * parse_xml() to fail (duplicate element). */
		g_get_current_time (&time_val);
		priv->edited = time_val.tv_sec;
	}

	return object;
}

static void
gdata_contacts_group_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataContactsGroupPrivate *priv = GDATA_CONTACTS_GROUP (object)->priv;

	switch (property_id) {
		case PROP_EDITED:
			g_value_set_int64 (value, priv->edited);
			break;
		case PROP_DELETED:
			g_value_set_boolean (value, priv->deleted);
			break;
		case PROP_SYSTEM_GROUP_ID:
			g_value_set_string (value, priv->system_group_id);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_contacts_group_finalize (GObject *object)
{
	GDataContactsGroupPrivate *priv = GDATA_CONTACTS_GROUP (object)->priv;

	g_hash_table_destroy (priv->extended_properties);
	g_free (priv->system_group_id);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_contacts_group_parent_class)->finalize (object);
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	gboolean success;
	GDataContactsGroup *self = GDATA_CONTACTS_GROUP (parsable);

	if (gdata_parser_is_namespace (node, "http://www.w3.org/2007/app") == TRUE &&
	    gdata_parser_int64_time_from_element (node, "edited", P_REQUIRED | P_NO_DUPES, &(self->priv->edited), &success, error) == TRUE) {
		return success;
	} else if (gdata_parser_is_namespace (node, "http://www.w3.org/2005/Atom") == TRUE && xmlStrcmp (node->name, (xmlChar*) "id") == 0) {
		/* We have to override <id> parsing to fix the projection. Modify it in-place so that the parser in GDataEntry will pick up the
		 * changes. This fixes bugs caused by referring to contacts by the base projection, rather than the full projection; such as
		 * http://code.google.com/p/gdata-issues/issues/detail?id=2129. */
		gchar *base;
		gchar *id = (gchar*) xmlNodeListGetString (doc, node->children, TRUE);

		if (id != NULL) {
			base = strstr (id, "/base/");
			if (base != NULL) {
				memcpy (base, "/full/", 6);
				xmlNodeSetContent (node, (xmlChar*) id);
			}
		}

		xmlFree (id);

		return GDATA_PARSABLE_CLASS (gdata_contacts_group_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	} else if (gdata_parser_is_namespace (node, "http://schemas.google.com/g/2005") == TRUE) {
		if (xmlStrcmp (node->name, (xmlChar*) "extendedProperty") == 0) {
			/* gd:extendedProperty */
			xmlChar *name, *value;
			xmlBuffer *buffer = NULL;

			name = xmlGetProp (node, (xmlChar*) "name");
			if (name == NULL)
				return gdata_parser_error_required_property_missing (node, "name", error);

			/* Get either the value property, or the element's content */
			value = xmlGetProp (node, (xmlChar*) "value");
			if (value == NULL) {
				xmlNode *child_node;

				/* Use the element's content instead (arbitrary XML) */
				buffer = xmlBufferCreate ();
				for (child_node = node->children; child_node != NULL; child_node = child_node->next)
					xmlNodeDump (buffer, doc, child_node, 0, 0);
				value = (xmlChar*) xmlBufferContent (buffer);
			}

			gdata_contacts_group_set_extended_property (self, (gchar*) name, (gchar*) value);

			xmlFree (name);
			if (buffer != NULL)
				xmlBufferFree (buffer);
			else
				xmlFree (value);
		} else if (xmlStrcmp (node->name, (xmlChar*) "deleted") == 0) {
			/* gd:deleted */
			if (self->priv->deleted == TRUE)
				return gdata_parser_error_duplicate_element (node, error);

			self->priv->deleted = TRUE;
		} else {
			return GDATA_PARSABLE_CLASS (gdata_contacts_group_parent_class)->parse_xml (parsable, doc, node, user_data, error);
		}
	} else if (gdata_parser_is_namespace (node, "http://schemas.google.com/contact/2008") == TRUE) {
		if (xmlStrcmp (node->name, (xmlChar*) "systemGroup") == 0) {
			/* gContact:systemGroup */
			xmlChar *value;

			if (self->priv->system_group_id != NULL)
				return gdata_parser_error_duplicate_element (node, error);

			value = xmlGetProp (node, (xmlChar*) "id");
			if (value == NULL || *value == '\0') {
				xmlFree (value);
				return gdata_parser_error_required_property_missing (node, "id", error);
			}

			self->priv->system_group_id = (gchar*) value;
		} else {
			return GDATA_PARSABLE_CLASS (gdata_contacts_group_parent_class)->parse_xml (parsable, doc, node, user_data, error);
		}
	} else {
		return GDATA_PARSABLE_CLASS (gdata_contacts_group_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}

	return TRUE;
}

static void
get_extended_property_xml_cb (const gchar *name, const gchar *value, GString *xml_string)
{
	/* Note that the value *isn't* escaped (see http://code.google.com/apis/gdata/docs/2.0/elements.html#gdExtendedProperty) */
	gdata_parser_string_append_escaped (xml_string, "<gd:extendedProperty name='", name, "'>");
	g_string_append_printf (xml_string, "%s</gd:extendedProperty>", value);
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataContactsGroupPrivate *priv = GDATA_CONTACTS_GROUP (parsable)->priv;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_contacts_group_parent_class)->get_xml (parsable, xml_string);

	/* Extended properties */
	g_hash_table_foreach (priv->extended_properties, (GHFunc) get_extended_property_xml_cb, xml_string);
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_contacts_group_parent_class)->get_namespaces (parsable, namespaces);

	g_hash_table_insert (namespaces, (gchar*) "gd", (gchar*) "http://schemas.google.com/g/2005");
	g_hash_table_insert (namespaces, (gchar*) "gContact", (gchar*) "http://schemas.google.com/contact/2008");
	g_hash_table_insert (namespaces, (gchar*) "app", (gchar*) "http://www.w3.org/2007/app");
}

static gchar *
get_entry_uri (const gchar *id)
{
	const gchar *base_pos;
	gchar *uri = g_strdup (id);

	/* The service API sometimes stubbornly insists on using the "base" view instead of the "full" view, which we have to fix, or our extended
	 * attributes are never visible */
	base_pos = strstr (uri, "/base/");
	if (base_pos != NULL)
		memcpy ((char*) base_pos, "/full/", 6);

	return uri;
}

/**
 * gdata_contacts_group_new:
 * @id: (allow-none): the group's ID, or %NULL
 *
 * Creates a new #GDataContactsGroup with the given ID and default properties.
 *
 * Return value: a new #GDataContactsGroup; unref with g_object_unref()
 *
 * Since: 0.7.0
 */
GDataContactsGroup *
gdata_contacts_group_new (const gchar *id)
{
	return GDATA_CONTACTS_GROUP (g_object_new (GDATA_TYPE_CONTACTS_GROUP, "id", id, NULL));
}

/**
 * gdata_contacts_group_get_edited:
 * @self: a #GDataContactsGroup
 *
 * Gets the #GDataContactsGroup:edited property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the UNIX timestamp for the time the file was last edited, or <code class="literal">-1</code>
 *
 * Since: 0.7.0
 */
gint64
gdata_contacts_group_get_edited (GDataContactsGroup *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_GROUP (self), -1);
	return self->priv->edited;
}

/**
 * gdata_contacts_group_get_system_group_id:
 * @self: a #GDataContactsGroup
 *
 * Gets the #GDataContactsGroup:system-group-id property. If the group is not a system group, %NULL will be returned.
 *
 * Return value: the group's system group ID, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_contacts_group_get_system_group_id (GDataContactsGroup *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_GROUP (self), NULL);
	return self->priv->system_group_id;
}

/**
 * gdata_contacts_group_get_extended_property:
 * @self: a #GDataContactsGroup
 * @name: the property name; an arbitrary, unique string
 *
 * Gets the value of an extended property of the group. Each group can have up to 10 client-set extended properties to store data of the client's
 * choosing.
 *
 * Return value: the property's value, or %NULL
 *
 * Since: 0.7.0
 */
const gchar *
gdata_contacts_group_get_extended_property (GDataContactsGroup *self, const gchar *name)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_GROUP (self), NULL);
	g_return_val_if_fail (name != NULL && *name != '\0', NULL);
	return g_hash_table_lookup (self->priv->extended_properties, name);
}

/**
 * gdata_contacts_group_get_extended_properties:
 * @self: a #GDataContactsGroup
 *
 * Gets the full list of extended properties of the group; a hash table mapping property name to value.
 *
 * Return value: (transfer none): a #GHashTable of extended properties
 *
 * Since: 0.7.0
 */
GHashTable *
gdata_contacts_group_get_extended_properties (GDataContactsGroup *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_GROUP (self), NULL);
	return self->priv->extended_properties;
}

/**
 * gdata_contacts_group_set_extended_property:
 * @self: a #GDataContactsGroup
 * @name: the property name; an arbitrary, unique string
 * @value: (allow-none): the property value, or %NULL
 *
 * Sets the value of a group's extended property. Extended property names are unique (but of the client's choosing), and reusing the same property
 * name will result in the old value of that property being overwritten.
 *
 * To unset a property, set @value to %NULL or an empty string.
 *
 * A group may have up to 10 extended properties, and each should be reasonably small (i.e. not a photo or ringtone). For more information, see the
 * <ulink type="http" url="http://code.google.com/apis/contacts/docs/2.0/reference.html#ProjectionsAndExtended">online documentation</ulink>.
 * %FALSE will be returned if you attempt to add more than 10 extended properties.
 *
 * Return value: %TRUE if the property was updated or deleted successfully, %FALSE otherwise
 *
 * Since: 0.7.0
 */
gboolean
gdata_contacts_group_set_extended_property (GDataContactsGroup *self, const gchar *name, const gchar *value)
{
	GHashTable *extended_properties = self->priv->extended_properties;

	g_return_val_if_fail (GDATA_IS_CONTACTS_GROUP (self), FALSE);
	g_return_val_if_fail (name != NULL && *name != '\0', FALSE);

	if (value == NULL || *value == '\0') {
		/* Removing a property */
		g_hash_table_remove (extended_properties, name);
		return TRUE;
	}

	/* We can't add more than MAX_N_EXTENDED_PROPERTIES */
	if (g_hash_table_lookup (extended_properties, name) == NULL && g_hash_table_size (extended_properties) >= MAX_N_EXTENDED_PROPERTIES)
		return FALSE;

	/* Updating an existing property or adding a new one */
	g_hash_table_insert (extended_properties, g_strdup (name), g_strdup (value));

	return TRUE;
}

/**
 * gdata_contacts_group_is_deleted:
 * @self: a #GDataContactsGroup
 *
 * Returns whether the group has recently been deleted. This will always return %FALSE unless #GDataContactsQuery:show-deleted has been set to %TRUE
 * for the query which returned the group; then this function will return %TRUE only if the group has been deleted.
 *
 * If a group has been deleted, no other information is available about it. This is designed to allow groups to be deleted from local address
 * books using incremental updates from the server (e.g. with #GDataQuery:updated-min and #GDataContactsQuery:show-deleted).
 *
 * Return value: %TRUE if the group has been deleted, %FALSE otherwise
 *
 * Since: 0.7.0
 */
gboolean
gdata_contacts_group_is_deleted (GDataContactsGroup *self)
{
	g_return_val_if_fail (GDATA_IS_CONTACTS_GROUP (self), FALSE);
	return self->priv->deleted;
}
