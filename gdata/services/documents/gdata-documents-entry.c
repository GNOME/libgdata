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
 * For more details of Google Documents' GData API, see the <ulink type="http://code.google.com/apis/document/docs/2.0/developers_guide_protocol.html">
 * online documentation</ulink>.
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
#include "gdata-download-stream.h"

static void gdata_documents_entry_access_handler_init (GDataAccessHandlerIface *iface);
static void gdata_documents_entry_finalize (GObject *object);
static void gdata_entry_dispose (GObject *object);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static void gdata_documents_entry_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_documents_entry_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);

struct _GDataDocumentsEntryPrivate {
	GTimeVal edited;
	GTimeVal last_viewed;
	gchar *document_id;
	gboolean writers_can_invite;
	gboolean is_deleted;
	GDataAuthor *last_modified_by;
};

enum {
	PROP_EDITED = 1,
	PROP_LAST_VIEWED,
	PROP_DOCUMENT_ID,
	PROP_LAST_MODIFIED_BY,
	PROP_IS_DELETED,
	PROP_WRITERS_CAN_INVITE
};

G_DEFINE_TYPE_WITH_CODE (GDataDocumentsEntry, gdata_documents_entry, GDATA_TYPE_ENTRY,
			 G_IMPLEMENT_INTERFACE (GDATA_TYPE_ACCESS_HANDLER, gdata_documents_entry_access_handler_init))
#define GDATA_DOCUMENTS_ENTRY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDATA_TYPE_DOCUMENTS_ENTRY, GDataDocumentsEntryPrivate))

static void
gdata_documents_entry_class_init (GDataDocumentsEntryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataDocumentsEntryPrivate));

	gobject_class->get_property = gdata_documents_entry_get_property;
	gobject_class->set_property = gdata_documents_entry_set_property;
	gobject_class->finalize = gdata_documents_entry_finalize;
	gobject_class->dispose = gdata_entry_dispose;

	parsable_class->parse_xml = parse_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;

	/**
	 * GDataDocumentsEntry:edited
	 *
	 * The last time the document was edited. If the document has not been edited yet, the content indicates the time it was created.
	 *
	 * For more information, see the <ulink type="http" url="http://www.atomenabled.org/developers/protocol/#appEdited">
	 * Atom Publishing Protocol specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_EDITED,
				g_param_spec_boxed ("edited",
					"Edited", "The last time the document was edited.",
					GDATA_TYPE_G_TIME_VAL,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsEntry:last-viewed
	 *
	 * The last time the document was viewed.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_LAST_VIEWED,
				g_param_spec_boxed ("last-viewed",
					"Last viewed", "The last time the document was viewed.",
					GDATA_TYPE_G_TIME_VAL,
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
	 * GDataDocumentsEntry:document-id
	 *
	 * The document ID of the document, which is different from its entry ID (GDataEntry:id).
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_DOCUMENT_ID,
				g_param_spec_string ("document-id",
					"Document ID", "The document ID of the document.",
					NULL,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsEntry:last-modified-by
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

}

static void
gdata_documents_entry_init (GDataDocumentsEntry *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_DOCUMENTS_ENTRY, GDataDocumentsEntryPrivate);

	/* Initialise the edited properties to the current time */
	g_get_current_time (&(self->priv->edited));
}

static gboolean
is_owner_rule (GDataAccessRule *rule)
{
	return (strcmp (gdata_access_rule_get_role (rule), "owner") == 0) ? TRUE : FALSE;
}

static void
gdata_documents_entry_access_handler_init (GDataAccessHandlerIface *iface)
{
	iface->is_owner_rule = is_owner_rule;
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	GDataDocumentsEntry *self = GDATA_DOCUMENTS_ENTRY (parsable);

	if (xmlStrcmp (node->name, (xmlChar*) "edited") == 0) {
		xmlChar *edited = xmlNodeListGetString (doc, node->children, TRUE);
		if (g_time_val_from_iso8601 ((gchar*) edited, &(self->priv->edited)) == FALSE) {
			gdata_parser_error_not_iso8601_format (node, (gchar*) edited, error);
			xmlFree (edited);
			return FALSE;
		}
		xmlFree (edited);
	} else if (xmlStrcmp (node->name, (xmlChar*) "lastViewed") == 0) {
		xmlChar *last_viewed = xmlNodeListGetString (doc, node->children, TRUE);
		if (g_time_val_from_iso8601 ((gchar*) last_viewed, &(self->priv->last_viewed)) == FALSE) {
			gdata_parser_error_not_iso8601_format (node, (gchar*) last_viewed, error);
			xmlFree (last_viewed);
			return FALSE;
		}
		xmlFree (last_viewed);
	} else if (xmlStrcmp (node->name, (xmlChar*) "writersCanInvite") ==  0) {
		xmlChar *writers_can_invite = xmlGetProp (node, (xmlChar*) "value");
		if (xmlStrcmp (writers_can_invite, (xmlChar*) "true") == 0) {
			self->priv->writers_can_invite = TRUE;
		} else if (xmlStrcmp (writers_can_invite, (xmlChar*) "false") == 0) {
			self->priv->writers_can_invite = FALSE;
		} else {
			gdata_parser_error_unknown_property_value (node, "value", (gchar*) writers_can_invite, error);
			xmlFree (writers_can_invite);
			return FALSE;
		}
		xmlFree (writers_can_invite);
	} else if (xmlStrcmp (node->name, (xmlChar*) "deleted") ==  0) {
		/* <gd:deleted> */
		/* Note that it doesn't have any parameters, so we unconditionally set priv->is_deleted to TRUE */
		self->priv->is_deleted = TRUE;
	} else if (xmlStrcmp (node->name, (xmlChar*) "resourceId") ==  0) {
		gchar **document_id_parts;
		xmlChar *resource_id;

		if (self->priv->document_id != NULL)
			return gdata_parser_error_duplicate_element (node, error);

		resource_id = xmlNodeListGetString (doc, node->children, TRUE);
		if (resource_id == NULL || *resource_id == '\0') {
			xmlFree (resource_id);
			return gdata_parser_error_required_content_missing (node, error);
		}

		document_id_parts = g_strsplit ((gchar*) resource_id, ":", 2);
		if (document_id_parts == NULL) {
			gdata_parser_error_unknown_content (node, (gchar*) resource_id, error);
			xmlFree (resource_id);
			return FALSE;
		}
		xmlFree (resource_id);

		self->priv->document_id = g_strdup (document_id_parts[1]);
		g_strfreev (document_id_parts);
	} else if (xmlStrcmp (node->name, (xmlChar*) "feedLink") ==  0) {
		GDataLink *link = GDATA_LINK (_gdata_parsable_new_from_xml_node (GDATA_TYPE_LINK, doc, node, NULL, error));
		if (link == NULL)
			return FALSE;
		gdata_entry_add_link (GDATA_ENTRY (self), link);
		g_object_unref (link);
	} else if (xmlStrcmp (node->name, (xmlChar*) "lastModifiedBy") ==  0) {
		GDataAuthor *last_modified_by = GDATA_AUTHOR (_gdata_parsable_new_from_xml_node (GDATA_TYPE_AUTHOR, doc, node, NULL, error));
		if (last_modified_by == NULL)
			return FALSE;
		self->priv->last_modified_by = last_modified_by;
	} else if (GDATA_PARSABLE_CLASS (gdata_documents_entry_parent_class)->parse_xml (parsable, doc, node, user_data, error) == FALSE) {
		/* Error! */
		return FALSE;
	}

	return TRUE;
}

static void
gdata_documents_entry_finalize (GObject *object)
{
	GDataDocumentsEntryPrivate *priv = GDATA_DOCUMENTS_ENTRY_GET_PRIVATE (object);

	g_free (priv->document_id);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_documents_entry_parent_class)->finalize (object);
}

static void
gdata_entry_dispose (GObject *object)
{
	GDataDocumentsEntryPrivate *priv = GDATA_DOCUMENTS_ENTRY_GET_PRIVATE (object);

	if (priv->last_modified_by != NULL)
		g_object_unref (priv->last_modified_by);
	priv->last_modified_by = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_documents_entry_parent_class)->dispose (object);
}

static void
gdata_documents_entry_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataDocumentsEntryPrivate *priv = GDATA_DOCUMENTS_ENTRY_GET_PRIVATE (object);

	switch (property_id) {
		case PROP_DOCUMENT_ID:
			g_value_set_string (value, priv->document_id);
			break;
		case PROP_WRITERS_CAN_INVITE:
			g_value_set_boolean (value, priv->writers_can_invite);
			break;
		case PROP_IS_DELETED:
			g_value_set_boolean (value, priv->is_deleted);
			break;
		case PROP_EDITED:
			g_value_set_boxed (value, &(priv->edited));
			break;
		case PROP_LAST_VIEWED:
			g_value_set_boxed (value, &(priv->last_viewed));
			break;
		case PROP_LAST_MODIFIED_BY:
			g_value_set_object (value, priv->last_modified_by);
			break;
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
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
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
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_documents_entry_parent_class)->get_namespaces (parsable, namespaces);

	g_hash_table_insert (namespaces, (gchar*) "docs", (gchar*) "http://schemas.google.com/docs/2007");
}

/**
 * gdata_documents_entry_get_edited:
 * @self: a #GDataDocumentsEntry
 * @edited: a #GTimeVal
 *
 * Gets the #GDataDocumentsEntry:edited property and puts it in @edited. If the property is unset,
 * both fields in the #GTimeVal will be set to %0.
 *
 * Since: 0.4.0
 **/
void
gdata_documents_entry_get_edited (GDataDocumentsEntry *self, GTimeVal *edited)
{
	g_return_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self));
	g_return_if_fail (edited != NULL);
	edited = &(self->priv->edited);
}

/**
 * gdata_documents_entry_get_last_viewed:
 * @self: a #GDataDocumentsEntry
 * @last_viewed: a #GTimeVal
 *
 * Gets the #GDataDocumentsEntry:last-viewed property and puts it in @last_viewed. If the property is unset,
 * both fields in the #GTimeVal will be set to %0.
 *
 * Since: 0.4.0
 **/
void
gdata_documents_entry_get_last_viewed (GDataDocumentsEntry *self, GTimeVal *last_viewed)
{
	g_return_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self));
	g_return_if_fail (last_viewed != NULL);
	*last_viewed = self->priv->last_viewed;
}

/**
 * gdata_documents_entry_get_path:
 * @self: a #GDataDocumentsEntry
 *
 * Gets the #GDataDocumentsEntry:path property.
 *
 * Note: the path is based on the entry ID, and not the entry human readable name (#GDataEntry::title).
 *
 * Return value: the folder hierarchy path containing the entry, or %NULL; free with g_free()
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

		g_string_append (path, folder_id);
		g_string_append_c (path, '/');
		g_free (folder_id);
	}

	/* Append the document ID */
	g_string_append (path, self->priv->document_id);

	return g_string_free (path, FALSE);
}

/**
 * gdata_documents_entry_get_document_id:
 * @self: a #GDataDocumentsEntry
 *
 * Gets the #GDataDocumentsEntry:document-id property.
 *
 * Return value: the document's document ID
 *
 * Since: 0.4.0
 **/
const gchar *
gdata_documents_entry_get_document_id (GDataDocumentsEntry *self )
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), NULL);
	return self->priv->document_id;
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
 * Return value: the author who last modified the document
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

static void
notify_content_type_cb (GDataDownloadStream *download_stream, GParamSpec *pspec, gchar **content_type)
{
	*content_type = g_strdup (gdata_download_stream_get_content_type (download_stream));
}

/*
 * _gdata_documents_entry_download_document:
 * @self: a #GDataDocumentsEntry
 * @service: an authenticated #GDataDocumentsService
 * @content_type: return location for the document's content type, or %NULL; free with g_free()
 * @download_uri: the URI to download the document
 * @destination_file: the #GFile into which the document file should be saved
 * @file_extension: the extension with which to save the downloaded file
 * @replace_file_if_exists: %TRUE if you want to replace the file if it exists, %FALSE otherwise
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Downloads and returns the actual file which comprises the document here. If the document doesn't exist,
 * the downloaded document will be an HTML file containing the error explanation.
 * TODO: Is that still true?
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * If @replace_file_if_exists is set to %FALSE and the destination file already exists, a %G_IO_ERROR_EXISTS will be returned.
 *
 * If @service isn't authenticated, a %GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED is returned.
 *
 * If there is an error downloading the document, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned.
 *
 * If @destination_file is a directory, the file will be downloaded to this directory with the #GDataEntry:title and 
 * the appropriate extension as its filename.
 *
 * Return value: a #GFile pointing to the downloaded document, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 */
GFile *
_gdata_documents_entry_download_document (GDataDocumentsEntry *self, GDataService *service, gchar **content_type, const gchar *src_uri,
					  GFile *destination_file, const gchar *file_extension, gboolean replace_file_if_exists,
					  GCancellable *cancellable, GError **error)
{
	const gchar *document_title;
	gchar *default_filename;
	GFileOutputStream *dest_stream;
	GInputStream *src_stream;
	GFile *actual_file = NULL;
	GError *child_error = NULL;

	/* TODO: async version */
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), NULL);
	g_return_val_if_fail (GDATA_IS_SERVICE (service), NULL);
	g_return_val_if_fail (src_uri != NULL, NULL);
	g_return_val_if_fail (G_IS_FILE (destination_file), NULL);
	g_return_val_if_fail (file_extension != NULL, NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Ensure we're authenticated first */
	if (gdata_service_is_authenticated (GDATA_SERVICE (service)) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
				     _("You must be authenticated to download documents."));
		return NULL;
	}

	/* Determine a default filename based on the document's title */
	document_title = gdata_entry_get_title (GDATA_ENTRY (self));
	default_filename = g_strdup_printf ("%s.%s", document_title, file_extension);

	dest_stream = _gdata_download_stream_find_destination (default_filename, destination_file, &actual_file, replace_file_if_exists, cancellable, error);
	g_free (default_filename);

	if (dest_stream == NULL)
		return NULL;

	/* Synchronously splice the data from the download stream to the file stream (network -> disk) */
	src_stream = gdata_download_stream_new (GDATA_SERVICE (service), src_uri);
	g_signal_connect (src_stream, "notify::content-type", (GCallback) notify_content_type_cb, content_type);
	g_output_stream_splice (G_OUTPUT_STREAM (dest_stream), src_stream,
				G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, cancellable, &child_error);
	g_object_unref (src_stream);
	g_object_unref (dest_stream);
	if (child_error != NULL) {
		g_object_unref (actual_file);
		g_propagate_error (error, child_error);
		return NULL;
	}

	return actual_file;
}
