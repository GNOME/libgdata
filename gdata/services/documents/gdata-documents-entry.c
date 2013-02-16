/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Thibault Saunier 2009 <saunierthibault@gmail.com>
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
 * SECTION:gdata-documents-entry
 * @short_description: GData document object abstract class
 * @stability: Unstable
 * @include: gdata/services/documents/gdata-documents-entry.h
 *
 * #GDataDocumentsEntry is a subclass of #GDataEntry to represent a Google Documents entry, which is then further subclassed
 * to give specific document types.
 *
 * #GDataDocumentsEntry implements #GDataAccessHandler, meaning the access rules to it can be modified using that interface. As well as the
 * access roles defined for the base #GDataAccessRule (e.g. %GDATA_ACCESS_ROLE_NONE), #GDataDocumentsEntry has its own, such as
 * %GDATA_DOCUMENTS_ACCESS_ROLE_OWNER and %GDATA_DOCUMENTS_ACCESS_ROLE_READER.
 *
 * Documents can (confusingly) be referenced by three different types of IDs: their entry ID, their resource ID and their document ID (untyped resource
 * ID). Each is a substring of the previous ones (i.e. the entry ID contains the resource ID, which in turn contains the document ID). The resource ID
 * and document ID should almost always be considered as internal, and thus entry IDs (#GDataEntry:id) should normally be used to uniquely identify
 * documents. For more information, see #GDataDocumentsEntry:resource-id.
 *
 * For more details of Google Documents' GData API, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/">online documentation</ulink>.
 *
 * <example>
 * 	<title>Moving an Entry Between Folders</title>
 * 	<programlisting>
 *	GDataDocumentsService *service;
 *	GDataDocumentsEntry *entry, *intermediate_entry, *updated_entry;
 *	GDataDocumentsFolder *old_folder, *new_folder;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service *<!-- -->/
 *	service = create_documents_service ();
 *
 *	/<!-- -->* Get the entry, the folder it's being moved out of, and the folder it's being moved into. The entry can either be a document or
 *	 * another folder, allowing hierarchies of folders to be constructed. *<!-- -->/
 *	entry = query_user_for_entry (service);
 *	old_folder = query_user_for_old_folder (service);
 *	new_folder = query_user_for_new_folder (service);
 *
 *	/<!-- -->* Add the entry to the new folder *<!-- -->/
 *	intermediate_entry = gdata_documents_service_add_entry_to_folder (service, entry, new_folder, NULL, &error);
 *
 *	g_object_unref (entry);
 *	g_object_unref (new_folder);
 *
 *	if (error != NULL) {
 *		g_error ("Error adding entry to new folder: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (old_folder);
 *		g_object_unref (service);
 *		return;
 *	}
 *
 *	/<!-- -->* Remove the entry from the old folder *<!-- -->/
 *	updated_entry = gdata_documents_service_remove_entry_from_folder (service, intermediate_entry, old_folder, NULL, &error);
 *
 *	g_object_unref (intermediate_entry);
 *	g_object_unref (old_folder);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		/<!-- -->* Note that you might want to attempt to remove the intermediate_entry from the new_folder in this error case, so that
 *		 * the operation is aborted cleanly. *<!-- -->/
 *		g_error ("Error removing entry from previous folder: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	/<!-- -->* Do something with the updated entry *<!-- -->/
 *
 *	g_object_unref (updated_entry);
 * 	</programlisting>
 * </example>
 *
 * Since: 0.4.0
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-documents-entry.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "gdata-private.h"
#include "gdata-access-handler.h"
#include "gdata-documents-service.h"

#include "gdata-documents-spreadsheet.h"
#include "gdata-documents-presentation.h"
#include "gdata-documents-text.h"
#include "gdata-documents-folder.h"

static void gdata_documents_entry_access_handler_init (GDataAccessHandlerIface *iface);
static GObject *gdata_documents_entry_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params);
static void gdata_documents_entry_finalize (GObject *object);
static void gdata_entry_dispose (GObject *object);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static void gdata_documents_entry_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_documents_entry_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static gchar *get_entry_uri (const gchar *id);

static const gchar *_get_untyped_resource_id (GDataDocumentsEntry *self) G_GNUC_PURE;

struct _GDataDocumentsEntryPrivate {
	gint64 edited;
	gint64 last_viewed;
	gchar *resource_id;
	gboolean writers_can_invite;
	gboolean is_deleted;
	GDataAuthor *last_modified_by;
	goffset quota_used; /* bytes */
};

enum {
	PROP_EDITED = 1,
	PROP_LAST_VIEWED,
	PROP_DOCUMENT_ID,
	PROP_LAST_MODIFIED_BY,
	PROP_IS_DELETED,
	PROP_WRITERS_CAN_INVITE,
	PROP_ID,
	PROP_RESOURCE_ID,
	PROP_QUOTA_USED,
};

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GDataDocumentsEntry, gdata_documents_entry, GDATA_TYPE_ENTRY,
                                  G_IMPLEMENT_INTERFACE (GDATA_TYPE_ACCESS_HANDLER, gdata_documents_entry_access_handler_init))

static void
gdata_documents_entry_class_init (GDataDocumentsEntryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataDocumentsEntryPrivate));

	gobject_class->constructor = gdata_documents_entry_constructor;
	gobject_class->get_property = gdata_documents_entry_get_property;
	gobject_class->set_property = gdata_documents_entry_set_property;
	gobject_class->finalize = gdata_documents_entry_finalize;
	gobject_class->dispose = gdata_entry_dispose;

	parsable_class->parse_xml = parse_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;

	entry_class->get_entry_uri = get_entry_uri;

	/**
	 * GDataDocumentsEntry:edited:
	 *
	 * The last time the document was edited. If the document has not been edited yet, the content indicates the time it was created.
	 *
	 * For more information, see the <ulink type="http" url="http://www.atomenabled.org/developers/protocol/#appEdited">
	 * Atom Publishing Protocol specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_EDITED,
	                                 g_param_spec_int64 ("edited",
	                                                     "Edited", "The last time the document was edited.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsEntry:last-viewed:
	 *
	 * The last time the document was viewed.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_LAST_VIEWED,
	                                 g_param_spec_int64 ("last-viewed",
	                                                     "Last viewed", "The last time the document was viewed.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsEntry:writers-can-invite:
	 *
	 * Indicates whether the document entry writers can invite others to edit the document.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_WRITERS_CAN_INVITE,
	                                 g_param_spec_boolean ("writers-can-invite",
	                                                       "Writers can invite?", "Indicates whether writers can invite others to edit.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsEntry:is-deleted:
	 *
	 * Indicates whether the document entry has been deleted (moved to the trash). Deleted documents will only
	 * appear in query results if the #GDataDocumentsQuery:show-deleted property is %TRUE.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_IS_DELETED,
	                                 g_param_spec_boolean ("is-deleted",
	                                                       "Deleted?", "Indicates whether the document entry has been deleted.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsEntry:resource-id:
	 *
	 * The resource ID of the document. This should not normally need to be used in client code, and is mostly for internal use. To uniquely
	 * identify a given document or folder, use its #GDataEntry:id.
	 *
	 * Resource IDs have the form:
	 * <literal><replaceable>document|drawing|pdf|spreadsheet|presentation|folder</replaceable>:<replaceable>untyped resource ID</replaceable></literal>; whereas
	 * entry IDs have the form:
	 * <literal>https://docs.google.com/feeds/id/<replaceable>resource ID</replaceable></literal> in version 3 of the API.
	 *
	 * For more information, see the
	 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#resource_ids_explained">Google Documents
	 * API reference</ulink>.
	 *
	 * Since: 0.11.0
	 */
	g_object_class_install_property (gobject_class, PROP_RESOURCE_ID,
	                                 g_param_spec_string ("resource-id",
	                                                      "Resource ID", "The resource ID of the document.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsEntry:document-id:
	 *
	 * The document ID of the document, which is different from its entry ID (GDataEntry:id). The
	 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#terminology_used_in_this_guide">online GData
	 * Documentation</ulink> refers to these as “untyped resource IDs”.
	 *
	 * Since: 0.4.0
	 * Deprecated: This a substring of the #GDataDocumentsEntry:resource-id, which is more general and should be used instead. (Since: 0.11.0.)
	 **/
	g_object_class_install_property (gobject_class, PROP_DOCUMENT_ID,
	                                 g_param_spec_string ("document-id",
	                                                      "Document ID", "The document ID of the document.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsEntry:last-modified-by:
	 *
	 * Indicates the author of the last modification.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_LAST_MODIFIED_BY,
	                                 g_param_spec_object ("last-modified-by",
	                                                      "Last modified by", "Indicates the author of the last modification.",
	                                                      GDATA_TYPE_AUTHOR,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsEntry:quota-used:
	 *
	 * The amount of user quota the document is occupying. Currently, only arbitrary files consume file space quota (whereas standard document
	 * formats, such as #GDataDocumentsText, #GDataDocumentsSpreadsheet and #GDataDocumentsFolder don't). Measured in bytes.
	 *
	 * This property will be <code class="literal">0</code> for documents which aren't consuming any quota.
	 *
	 * Since: 0.13.0
	 */
	g_object_class_install_property (gobject_class, PROP_QUOTA_USED,
	                                 g_param_spec_int64 ("quota-used",
	                                                     "Quota used", "The amount of user quota the document is occupying.",
	                                                     0, G_MAXINT64, 0,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/* Override the ID property since the server returns two different forms of ID depending on how you form a query on an entry. These two forms
	 * of ID are (for version 3 of the API):
	 *  - Document ID: /feeds/id/[resource_id]
	 *  - Folder ID: /feeds/default/private/full/[folder_id]/[resource_id]
	 * The former is the ID we want; the latter should only ever be used for manipulating the location of documents (i.e. adding them to and
	 * removing them from folders). The latter will, however, work fine for operations such as updating documents. It's only when one comes to
	 * try and delete a document that it becomes a problem: sending a DELETE request to the folder ID will only remove the document from that
	 * folder; it's only if one sends the DELETE request to the document ID that the document actually gets deleted.
	 * Unfortunately, uploading a document directly to a folder results in the server returning us a folder ID. Consequently, we need to override
	 * the property to fix this mess. */
	g_object_class_override_property (gobject_class, PROP_ID, "id");
}

static gboolean
is_owner_rule (GDataAccessRule *rule)
{
	return (strcmp (gdata_access_rule_get_role (rule), GDATA_DOCUMENTS_ACCESS_ROLE_OWNER) == 0) ? TRUE : FALSE;
}

static GDataAuthorizationDomain *
get_authorization_domain (GDataAccessHandler *self)
{
	return gdata_documents_service_get_primary_authorization_domain ();
}

static void
gdata_documents_entry_access_handler_init (GDataAccessHandlerIface *iface)
{
	iface->is_owner_rule = is_owner_rule;
	iface->get_authorization_domain = get_authorization_domain;
}

static void
gdata_documents_entry_init (GDataDocumentsEntry *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_DOCUMENTS_ENTRY, GDataDocumentsEntryPrivate);
	self->priv->edited = -1;
	self->priv->last_viewed = -1;
}

static GObject *
gdata_documents_entry_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
	GObject *object;

	/* Chain up to the parent class */
	object = G_OBJECT_CLASS (gdata_documents_entry_parent_class)->constructor (type, n_construct_params, construct_params);

	/* We can't create these in init, or they would collide with the group and control created when parsing the XML */
	if (_gdata_parsable_is_constructed_from_xml (GDATA_PARSABLE (object)) == FALSE) {
		GDataDocumentsEntryPrivate *priv = GDATA_DOCUMENTS_ENTRY (object)->priv;
		GTimeVal time_val;

		/* This can't be put in the init function of #GDataDocumentsEntry, as it would then be called even for entries parsed from XML from
		 * the server, which would break duplicate element detection for the app:edited element. */
		g_get_current_time (&time_val);
		priv->edited = time_val.tv_sec;
	}

	return object;
}

static void
gdata_entry_dispose (GObject *object)
{
	GDataDocumentsEntryPrivate *priv = GDATA_DOCUMENTS_ENTRY (object)->priv;

	if (priv->last_modified_by != NULL)
		g_object_unref (priv->last_modified_by);
	priv->last_modified_by = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_documents_entry_parent_class)->dispose (object);
}

static void
gdata_documents_entry_finalize (GObject *object)
{
	GDataDocumentsEntryPrivate *priv = GDATA_DOCUMENTS_ENTRY (object)->priv;

	g_free (priv->resource_id);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_documents_entry_parent_class)->finalize (object);
}

static void
gdata_documents_entry_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataDocumentsEntryPrivate *priv = GDATA_DOCUMENTS_ENTRY (object)->priv;

	switch (property_id) {
		case PROP_RESOURCE_ID:
			g_value_set_string (value, priv->resource_id);
			break;
		case PROP_DOCUMENT_ID:
			g_value_set_string (value, _get_untyped_resource_id (GDATA_DOCUMENTS_ENTRY (object)));
			break;
		case PROP_WRITERS_CAN_INVITE:
			g_value_set_boolean (value, priv->writers_can_invite);
			break;
		case PROP_IS_DELETED:
			g_value_set_boolean (value, priv->is_deleted);
			break;
		case PROP_EDITED:
			g_value_set_int64 (value, priv->edited);
			break;
		case PROP_LAST_VIEWED:
			g_value_set_int64 (value, priv->last_viewed);
			break;
		case PROP_LAST_MODIFIED_BY:
			g_value_set_object (value, priv->last_modified_by);
			break;
		case PROP_QUOTA_USED:
			g_value_set_int64 (value, priv->quota_used);
			break;
		case PROP_ID: {
			gchar *uri;

			/* Is it unset? */
			if (priv->resource_id == NULL) {
				g_value_set_string (value, NULL);
				break;
			}

			/* Build the ID */
			uri = _gdata_service_build_uri ("https://docs.google.com/feeds/id/%s", priv->resource_id);
			g_value_take_string (value, uri);

			break;
		}
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_documents_entry_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataDocumentsEntry *self = GDATA_DOCUMENTS_ENTRY (object);

	switch (property_id) {
		case PROP_WRITERS_CAN_INVITE:
			gdata_documents_entry_set_writers_can_invite (self, g_value_get_boolean (value));
			break;
		case PROP_ID:
			/* Never set an ID (note that this doesn't stop it being set in GDataEntry due to XML parsing) */
			break;
		case PROP_QUOTA_USED:
			/* Read only. */
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
	GDataDocumentsEntry *self = GDATA_DOCUMENTS_ENTRY (parsable);

	if (gdata_parser_is_namespace (node, "http://www.w3.org/2007/app") == TRUE &&
	    gdata_parser_int64_time_from_element (node, "edited", P_REQUIRED | P_NO_DUPES, &(self->priv->edited), &success, error) == TRUE) {
		return success;
	} else if (gdata_parser_is_namespace (node, "http://schemas.google.com/g/2005") == TRUE) {
		if (gdata_parser_int64_time_from_element (node, "lastViewed", P_REQUIRED | P_NO_DUPES,
		                                          &(self->priv->last_viewed), &success, error) == TRUE ||
		    gdata_parser_object_from_element_setter (node, "feedLink", P_REQUIRED, GDATA_TYPE_LINK,
		                                             gdata_entry_add_link, self,  &success, error) == TRUE ||
		    gdata_parser_object_from_element (node, "lastModifiedBy", P_REQUIRED, GDATA_TYPE_AUTHOR,
		                                      &(self->priv->last_modified_by), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "resourceId", P_REQUIRED | P_NON_EMPTY | P_NO_DUPES, &(self->priv->resource_id),
		                                      &success, error) == TRUE ||
		    gdata_parser_int64_from_element (node, "quotaBytesUsed", P_REQUIRED | P_NO_DUPES,
		                                     &(self->priv->quota_used), 0, &success, error) == TRUE) {
			return success;
		} else if (xmlStrcmp (node->name, (xmlChar*) "deleted") ==  0) {
			/* <gd:deleted> */
			/* Note that it doesn't have any parameters, so we unconditionally set priv->is_deleted to TRUE */
			self->priv->is_deleted = TRUE;
		} else {
			return GDATA_PARSABLE_CLASS (gdata_documents_entry_parent_class)->parse_xml (parsable, doc, node, user_data, error);
		}
	} else if (gdata_parser_is_namespace (node, "http://schemas.google.com/docs/2007") == TRUE &&
	           xmlStrcmp (node->name, (xmlChar*) "writersCanInvite") ==  0) {
		if (gdata_parser_boolean_from_property (node, "value", &(self->priv->writers_can_invite), -1, error) == FALSE)
			return FALSE;
	} else {
		return GDATA_PARSABLE_CLASS (gdata_documents_entry_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}

	return TRUE;
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataDocumentsEntryPrivate *priv = GDATA_DOCUMENTS_ENTRY (parsable)->priv;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_documents_entry_parent_class)->get_xml (parsable, xml_string);

	/* TODO: Only output "kind" categories? */

	if (priv->writers_can_invite == TRUE)
		g_string_append (xml_string, "<docs:writersCanInvite value='true'/>");
	else
		g_string_append (xml_string, "<docs:writersCanInvite value='false'/>");

	if (priv->resource_id != NULL) {
		gdata_parser_string_append_escaped (xml_string, "<gd:resourceId>", priv->resource_id, "</gd:resourceId>");
	}
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_documents_entry_parent_class)->get_namespaces (parsable, namespaces);

	g_hash_table_insert (namespaces, (gchar*) "docs", (gchar*) "http://schemas.google.com/docs/2007");
}

static gchar *
get_entry_uri (const gchar *id)
{
	const gchar *resource_id;

	/* Version 3: We get an ID similar to “https://docs.google.com/feeds/id/[resource_id]” and want an entry URI
	 * similar to “https://docs.google.com/feeds/default/private/full/[resource_id]”. */
	resource_id = g_strrstr (id, "/");

	if (resource_id == NULL) {
		/* Bail! */
		return g_strdup (id);
	}

	return g_strconcat (_gdata_service_get_scheme (), "://docs.google.com/feeds/default/private/full", resource_id, NULL);
}

/**
 * gdata_documents_entry_get_edited:
 * @self: a #GDataDocumentsEntry
 *
 * Gets the #GDataDocumentsEntry:edited property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the UNIX timestamp for the time the document was last edited, or <code class="literal">-1</code>
 *
 * Since: 0.4.0
 **/
gint64
gdata_documents_entry_get_edited (GDataDocumentsEntry *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), -1);
	return self->priv->edited;
}

/**
 * gdata_documents_entry_get_last_viewed:
 * @self: a #GDataDocumentsEntry
 *
 * Gets the #GDataDocumentsEntry:last-viewed property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the UNIX timestamp for the time the document was last viewed, or <code class="literal">-1</code>
 *
 * Since: 0.4.0
 **/
gint64
gdata_documents_entry_get_last_viewed (GDataDocumentsEntry *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), -1);
	return self->priv->last_viewed;
}

/**
 * gdata_documents_entry_get_path:
 * @self: a #GDataDocumentsEntry
 *
 * Builds a path for the #GDataDocumentsEntry, starting from a root node and traversing the folders containing the document, then ending with the
 * document's ID.
 *
 * An example path would be: <literal>/folder_id1/folder_id2/document_id</literal>.
 *
 * Note: the path is based on the entry/document IDs of the folders (#GDataEntry:id) and document (#GDataDocumentsEntry:document-id),
 * and not the entries' human-readable names (#GDataEntry:title).
 *
 * Return value: the folder hierarchy path containing the document, or %NULL; free with g_free()
 *
 * Since: 0.4.0
 **/
gchar *
gdata_documents_entry_get_path (GDataDocumentsEntry *self)
{
	GList *element, *parent_folders_list = NULL;
	GString *path;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), NULL);

	path = g_string_new ("/");
	parent_folders_list = gdata_entry_look_up_links (GDATA_ENTRY (self), "http://schemas.google.com/docs/2007#parent");

	/* We check all the folders contained that are parents of the GDataDocumentsEntry */
	for (element = parent_folders_list; element != NULL; element = element->next) {
		guint i;
		gchar *folder_id = NULL;
		gchar **link_href_cut = g_strsplit (gdata_link_get_uri (GDATA_LINK (element->data)), "/", 0);

		/* Extract the folder ID from the folder URI, which is of the form:
		 *   http://docs.google.com/feeds/documents/private/full/folder%3Afolder_id
		 * We want the "folder_id" bit. */
		for (i = 0;; i++) {
			gchar **path_cut = NULL;

			if (link_href_cut[i] == NULL)
				break;

			path_cut = g_strsplit (link_href_cut[i], "%3A", 2);
			if (*path_cut != NULL) {
				if (strcmp (path_cut[0], "folder") == 0){
					folder_id = g_strdup (path_cut[1]);
					g_strfreev (path_cut);
					break;
				}
			}
			g_strfreev (path_cut);
		}
		g_strfreev (link_href_cut);
		g_assert (folder_id != NULL);

		/* Append the folder ID to our path */
		g_string_append (path, folder_id);
		g_string_append_c (path, '/');
		g_free (folder_id);
	}

	/* Append the document ID */
	g_string_append (path, _get_untyped_resource_id (self));

	return g_string_free (path, FALSE);
}

/* Static version so that we can use it internally without triggering deprecation warnings.
 * Note that this is what libgdata used to call a "document ID". */
static const gchar *
_get_untyped_resource_id (GDataDocumentsEntry *self)
{
	const gchar *colon;

	/* Untyped resource ID should be NULL iff resource ID is. */
	if (self->priv->resource_id == NULL) {
		return NULL;
	}

	/* Resource ID is of the form "document:[untyped_resource_id]" (or "spreadsheet:[untyped_resource_id]", etc.),
	 * so we want to return the portion after the colon. */
	colon = g_utf8_strchr (self->priv->resource_id, -1, ':');
	g_assert (colon != NULL);

	return colon + 1;
}

/**
 * gdata_documents_entry_get_document_id:
 * @self: a #GDataDocumentsEntry
 *
 * Gets the #GDataDocumentsEntry:document-id property. The
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/#terminology_used_in_this_guide">online GData Documentation</ulink>
 * refers to these as “untyped resource IDs”.
 *
 * Return value: the document's document ID
 *
 * Since: 0.4.0
 * Deprecated: Use gdata_documents_entry_get_resource_id() instead. See #GDataDocumentsEntry:document-id. (Since: 0.11.0.)
 **/
const gchar *
gdata_documents_entry_get_document_id (GDataDocumentsEntry *self )
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), NULL);

	return _get_untyped_resource_id (self);
}

/**
 * gdata_documents_entry_get_resource_id:
 * @self: a #GDataDocumentsEntry
 *
 * Gets the #GDataDocumentsEntry:resource-id property.
 *
 * Return value: the document's resource ID
 *
 * Since: 0.11.0
 */
const gchar *
gdata_documents_entry_get_resource_id (GDataDocumentsEntry *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), NULL);
	return self->priv->resource_id;
}

/**
 * gdata_documents_entry_set_writers_can_invite:
 * @self: a #GDataDocumentsEntry
 * @writers_can_invite: %TRUE if writers can invite other people to edit the document, %FALSE otherwise
 *
 * Sets the #GDataDocumentsEntry:writers-can-invite property to @writers_can_invite.
 *
 * Since: 0.4.0
 **/
void
gdata_documents_entry_set_writers_can_invite (GDataDocumentsEntry *self, gboolean writers_can_invite)
{
	g_return_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self));
	self->priv->writers_can_invite = writers_can_invite;
	g_object_notify (G_OBJECT (self), "writers-can-invite");
}

/**
 * gdata_documents_entry_writers_can_invite:
 * @self: a #GDataDocumentsEntry
 *
 * Gets the #GDataDocumentsEntry:writers-can-invite property.
 *
 * Return value: %TRUE if writers can invite other people to edit the document, %FALSE otherwise
 *
 * Since: 0.4.0
 **/
gboolean
gdata_documents_entry_writers_can_invite (GDataDocumentsEntry *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self ), FALSE);
	return self->priv->writers_can_invite;
}

/**
 * gdata_documents_entry_get_last_modified_by:
 * @self: a #GDataDocumentsEntry
 *
 * Gets the #GDataDocumentsEntry:last-modified-by property.
 *
 * Return value: (transfer none): the author who last modified the document
 *
 * Since: 0.4.0
 **/
GDataAuthor *
gdata_documents_entry_get_last_modified_by (GDataDocumentsEntry *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), NULL);
	return self->priv->last_modified_by;
}

/**
 * gdata_documents_entry_get_quota_used:
 * @self: a #GDataDocumentsEntry
 *
 * Gets the #GDataDocumentsEntry:quota-used property.
 *
 * Return value: the number of quota bytes used by the document
 *
 * Since: 0.13.0
 */
goffset
gdata_documents_entry_get_quota_used (GDataDocumentsEntry *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), 0);

	return self->priv->quota_used;
}

/**
 * gdata_documents_entry_is_deleted:
 * @self: a #GDataDocumentsEntry
 *
 * Gets the #GDataDocumentsEntry:is-deleted property.
 *
 * Return value: %TRUE if the document has been deleted, %FALSE otherwise
 *
 * Since: 0.5.0
 **/
gboolean
gdata_documents_entry_is_deleted (GDataDocumentsEntry *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), FALSE);
	return self->priv->is_deleted;
}
