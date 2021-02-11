/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Thibault Saunier 2009 <saunierthibault@gmail.com>
 * Copyright (C) Red Hat, Inc. 2015, 2016
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
 * @stability: Stable
 * @include: gdata/services/documents/gdata-documents-entry.h
 *
 * #GDataDocumentsEntry is a subclass of #GDataEntry to represent a Google Drive entry, which is then further subclassed
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
 * For more details of Google Drive's GData API, see the
 * <ulink type="http" url="https://developers.google.com/drive/v2/web/about-sdk">online documentation</ulink>.
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
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "gdata-comparable.h"
#include "gdata-documents-entry.h"
#include "gdata-documents-entry-private.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "gdata-private.h"
#include "gdata-access-handler.h"
#include "gdata-documents-access-rule.h"
#include "gdata-documents-service.h"

#include "gdata-documents-spreadsheet.h"
#include "gdata-documents-presentation.h"
#include "gdata-documents-text.h"
#include "gdata-documents-folder.h"
#include "gdata-documents-utils.h"

static void gdata_documents_entry_access_handler_init (GDataAccessHandlerIface *iface);
static void gdata_documents_entry_finalize (GObject *object);
static void gdata_entry_dispose (GObject *object);
static const gchar *get_content_type (void);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);
static void gdata_documents_entry_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_documents_entry_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);
static gboolean post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error);
static void get_json (GDataParsable *parsable, JsonBuilder *builder);
static gchar *get_entry_uri (const gchar *id);

struct _GDataDocumentsEntryPrivate {
	gint64 last_viewed;
	gchar *resource_id;
	gboolean writers_can_invite;
	gboolean is_deleted;
	GDataAuthor *last_modified_by;
	goffset quota_used; /* bytes */
	goffset file_size; /* bytes */
	GList *properties; /* GDataDocumentsProperty */
	gint64 shared_with_me_date;
	gboolean can_edit;
};

enum {
	PROP_EDITED = 1,
	PROP_LAST_VIEWED,
	PROP_DOCUMENT_ID,
	PROP_LAST_MODIFIED_BY,
	PROP_IS_DELETED,
	PROP_WRITERS_CAN_INVITE,
	PROP_RESOURCE_ID,
	PROP_QUOTA_USED,
	PROP_FILE_SIZE,
	PROP_SHARED_WITH_ME_DATE,
	PROP_CAN_EDIT,
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

	gobject_class->get_property = gdata_documents_entry_get_property;
	gobject_class->set_property = gdata_documents_entry_set_property;
	gobject_class->finalize = gdata_documents_entry_finalize;
	gobject_class->dispose = gdata_entry_dispose;

	parsable_class->parse_json = parse_json;
	parsable_class->post_parse_json = post_parse_json;
	parsable_class->get_content_type = get_content_type;
	parsable_class->get_json = get_json;
	parsable_class->get_namespaces = get_namespaces;

	entry_class->get_entry_uri = get_entry_uri;

	/**
	 * GDataDocumentsEntry:edited:
	 *
	 * The last time the document was edited. If the document has not been edited yet, the content indicates the time it was created.
	 *
	 * Since: 0.4.0
	 * Deprecated: 0.17.0: This is identical to #GDataEntry:updated.
	 */
	g_object_class_install_property (gobject_class, PROP_EDITED,
	                                 g_param_spec_int64 ("edited",
	                                                     "Edited", "The last time the document was edited.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_DEPRECATED));

	/**
	 * GDataDocumentsEntry:last-viewed:
	 *
	 * The last time the document was viewed.
	 *
	 * Since: 0.4.0
	 */
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
	 */
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
	 */
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
	 * Deprecated: 0.11.0: This a substring of the #GDataDocumentsEntry:resource-id, which is more general and should be used instead.
	 */
	g_object_class_install_property (gobject_class, PROP_DOCUMENT_ID,
	                                 g_param_spec_string ("document-id",
	                                                      "Document ID", "The document ID of the document.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_DEPRECATED));

	/**
	 * GDataDocumentsEntry:last-modified-by:
	 *
	 * Indicates the author of the last modification.
	 *
	 * Since: 0.4.0
	 */
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

	/**
	 * GDataDocumentsEntry:file-size:
	 *
         * The size of the document. This is only set for non-document files.
         * Standard formats, such as #GDataDocumentsText,
         * #GDataDocumentsSpreadsheet and #GDataDocumentsFolder are not binary
         * data and so have no size. Measured in bytes.
	 *
	 * Since: 0.17.7
	 */
	g_object_class_install_property (gobject_class, PROP_FILE_SIZE,
	                                 g_param_spec_int64 ("file-size",
	                                                     "File size", "The size of the document.",
	                                                     0, G_MAXINT64, 0,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsEntry:shared-with-me-date:
	 *
	 * The UNIX timestamp for the time at which this file was shared with the user.
	 *
	 * Since: 0.18.0
	 */
	g_object_class_install_property (gobject_class, PROP_SHARED_WITH_ME_DATE,
	                                 g_param_spec_int64 ("shared-with-me-date",
	                                                     "Shared with me date", "The time at which this file was shared with the user.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsEntry:can-edit:
	 *
	 * Indicates whether the current user can edit this file.
	 *
	 * Since: 0.18.0
	 */
	g_object_class_install_property (gobject_class, PROP_CAN_EDIT,
	                                 g_param_spec_boolean ("can-edit",
	                                                       "Can edit?", "Indicates whether the current user can edit this file.",
	                                                       FALSE,
	                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
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

static GDataFeed *
get_rules (GDataAccessHandler *self,
	   GDataService *service,
	   GCancellable *cancellable,
	   GDataQueryProgressCallback progress_callback,
	   gpointer progress_user_data,
	   GError **error)
{
	GDataAccessHandlerIface *iface;
	GDataAuthorizationDomain *domain = NULL;
	GDataFeed *feed;
	GDataLink *_link;
	SoupMessage *message;

	_link = gdata_entry_look_up_link (GDATA_ENTRY (self), GDATA_LINK_ACCESS_CONTROL_LIST);
	g_assert (_link != NULL);

	iface = GDATA_ACCESS_HANDLER_GET_IFACE (self);
	if (iface->get_authorization_domain != NULL) {
		domain = iface->get_authorization_domain (self);
	}

	message = _gdata_service_query (service, domain, gdata_link_get_uri (_link), NULL, cancellable, error);
	if (message == NULL) {
		return NULL;
	}

	g_assert (message->response_body->data != NULL);

	feed = _gdata_feed_new_from_json (GDATA_TYPE_FEED, message->response_body->data, message->response_body->length, GDATA_TYPE_DOCUMENTS_ACCESS_RULE,
					  progress_callback, progress_user_data, error);

	g_object_unref (message);

	return feed;
}

static void
gdata_documents_entry_access_handler_init (GDataAccessHandlerIface *iface)
{
	iface->is_owner_rule = is_owner_rule;
	iface->get_authorization_domain = get_authorization_domain;
	iface->get_rules = get_rules;
}

static void
gdata_documents_entry_init (GDataDocumentsEntry *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_DOCUMENTS_ENTRY, GDataDocumentsEntryPrivate);
	self->priv->last_viewed = -1;
}

static void
gdata_entry_dispose (GObject *object)
{
	GDataDocumentsEntryPrivate *priv = GDATA_DOCUMENTS_ENTRY (object)->priv;

	if (priv->last_modified_by != NULL)
		g_object_unref (priv->last_modified_by);
	priv->last_modified_by = NULL;

	g_list_free_full (priv->properties, g_object_unref);
	priv->properties = NULL;

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
			g_value_set_string (value, gdata_entry_get_id (GDATA_ENTRY (object)));
			break;
		case PROP_WRITERS_CAN_INVITE:
			g_value_set_boolean (value, priv->writers_can_invite);
			break;
		case PROP_IS_DELETED:
			g_value_set_boolean (value, priv->is_deleted);
			break;
		case PROP_EDITED:
			g_value_set_int64 (value, gdata_entry_get_updated (GDATA_ENTRY (object)));
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
		case PROP_FILE_SIZE:
			g_value_set_int64 (value, priv->file_size);
			break;
		case PROP_SHARED_WITH_ME_DATE:
			g_value_set_int64 (value, priv->shared_with_me_date);
			break;
		case PROP_CAN_EDIT:
			g_value_set_boolean (value, priv->can_edit);
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
		case PROP_QUOTA_USED:
		case PROP_FILE_SIZE:
			/* Read only. */
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
get_key_value_and_visibility (JsonReader *reader, gchar **out_key, gchar **out_value, gchar **out_visibility, GError **error)
{
	g_autoptr(GError) child_error = NULL;
	g_autofree gchar *key = NULL;
	g_autofree gchar *value = NULL;
	g_autofree gchar *visibility = NULL;
	gboolean success = FALSE;
	guint i, members;

	for (i = 0, members = (guint) json_reader_count_members (reader); i < members; i++) {
		json_reader_read_element (reader, i);

		if (gdata_parser_string_from_json_member (reader, "key", P_REQUIRED | P_NON_EMPTY, &key, &success, &child_error) == TRUE) {
			if (!success && child_error != NULL) {
				g_propagate_prefixed_error (error, g_steal_pointer (&child_error),
							    /* Translators: the parameter is an error message */
							    _("Error parsing JSON: %s"),
							    "Failed to find ‘key’.");
				json_reader_end_element (reader);
				return;
			}
		}

		if (gdata_parser_string_from_json_member (reader, "visibility", P_REQUIRED | P_NON_EMPTY, &visibility, &success, &child_error) == TRUE) {
			if (!success && child_error != NULL) {
				g_propagate_prefixed_error (error, g_steal_pointer (&child_error),
							    /* Translators: the parameter is an error message */
							    _("Error parsing JSON: %s"),
							    "Failed to find ‘visibility’.");
				json_reader_end_element (reader);
				return;
			}
		}

		/* A Property can have a value field to be an empty string */
		if (gdata_parser_string_from_json_member (reader, "value", P_DEFAULT, &value, &success, &child_error) == TRUE) {
			if (!success && child_error != NULL) {
				g_propagate_prefixed_error (error, g_steal_pointer (&child_error),
							    /* Translators: the parameter is an error message */
							    _("Error parsing JSON: %s"),
							    "Failed to find ‘value’.");
				json_reader_end_element (reader);
				return;
			}
		}

		json_reader_end_element (reader);
	}

	if (out_key != NULL) {
		*out_key = g_steal_pointer (&key);
	}

	if (out_visibility != NULL) {
		*out_visibility = g_steal_pointer (&visibility);
	}

	if (out_value != NULL) {
		*out_value = g_steal_pointer (&value);
	}
}

static void
get_kind_email_and_name (JsonReader *reader, gchar **out_kind, gchar **out_email, gchar **out_name, GError **error)
{
	GError *child_error = NULL;
	gboolean success;
	gchar *email = NULL;
	gchar *kind = NULL;
	gchar *name = NULL;
	guint i, members;

	for (i = 0, members = (guint) json_reader_count_members (reader); i < members; i++) {
		json_reader_read_element (reader, i);

		if (gdata_parser_string_from_json_member (reader, "kind", P_REQUIRED | P_NON_EMPTY, &kind, &success, &child_error) == TRUE) {
			if (!success && child_error != NULL) {
				g_propagate_prefixed_error (error, child_error,
				                            /* Translators: the parameter is an error message */
				                            _("Error parsing JSON: %s"),
				                            "Failed to find ‘kind’.");
				json_reader_end_element (reader);
				goto out;
			}
		}

		if (gdata_parser_string_from_json_member (reader, "displayName", P_REQUIRED | P_NON_EMPTY, &name, &success, &child_error) == TRUE) {
			if (!success && child_error != NULL) {
				g_propagate_prefixed_error (error, child_error,
				                            /* Translators: the parameter is an error message */
				                            _("Error parsing JSON: %s"),
				                            "Failed to find ‘displayName’.");
				json_reader_end_element (reader);
				goto out;
			}
		}

		if (gdata_parser_string_from_json_member (reader, "emailAddress", P_DEFAULT, &email, &success, &child_error) == TRUE) {
			if (!success && child_error != NULL) {
				g_propagate_prefixed_error (error, child_error,
				                            /* Translators: the parameter is an error message */
				                            _("Error parsing JSON: %s"),
				                            "Failed to find ‘emailAddress’.");
				json_reader_end_element (reader);
				goto out;
			}
		}

		json_reader_end_element (reader);
	}

	if (out_kind != NULL) {
		*out_kind = kind;
		kind = NULL;
	}

	if (out_email != NULL) {
		*out_email = email;
		email = NULL;
	}

	if (out_name != NULL) {
		*out_name = name;
		name = NULL;
	}

 out:
	g_free (kind);
	g_free (email);
	g_free (name);
}

static void
get_kind_and_parent_link (JsonReader *reader, gchar **out_kind, gchar **out_parent_link, GError **error)
{
	GError *child_error = NULL;
	gboolean success;
	gchar *kind = NULL;
	gchar *parent_link = NULL;
	guint i, members;

	for (i = 0, members = (guint) json_reader_count_members (reader); i < members; i++) {
		json_reader_read_element (reader, i);

		if (gdata_parser_string_from_json_member (reader, "kind", P_REQUIRED | P_NON_EMPTY, &kind, &success, &child_error) == TRUE) {
			if (!success && child_error != NULL) {
				g_propagate_prefixed_error (error, child_error,
				                            /* Translators: the parameter is an error message */
				                            _("Error parsing JSON: %s"),
				                            "Failed to find ‘kind’.");
				json_reader_end_element (reader);
				goto out;
			}
		}

		if (gdata_parser_string_from_json_member (reader, "parentLink", P_REQUIRED | P_NON_EMPTY, &parent_link, &success, &child_error) == TRUE) {
			if (!success && child_error != NULL) {
				g_propagate_prefixed_error (error, child_error,
				                            /* Translators: the parameter is an error message */
				                            _("Error parsing JSON: %s"),
				                            "Failed to find ‘parentLink’.");
				json_reader_end_element (reader);
				goto out;
			}
		}

		json_reader_end_element (reader);
	}

	if (out_kind != NULL) {
		*out_kind = kind;
		kind = NULL;
	}

	if (out_parent_link != NULL) {
		*out_parent_link = parent_link;
		parent_link = NULL;
	}

 out:
	g_free (kind);
	g_free (parent_link);
}

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	GDataDocumentsEntryPrivate *priv = GDATA_DOCUMENTS_ENTRY (parsable)->priv;
	GDataCategory *category;
	GError *child_error = NULL;
	gboolean shared;
	gboolean success = TRUE;
	gchar *alternate_uri = NULL;
	gchar *kind = NULL;
	gchar *mime_type = NULL;
	gchar *quota_used = NULL;
	gchar *file_size = NULL;
	gint64 published;
	gint64 updated;

	/* JSON format: https://developers.google.com/drive/v2/reference/files */

	if (gdata_parser_string_from_json_member (reader, "alternateLink", P_DEFAULT, &alternate_uri, &success, error) == TRUE) {
		if (success && alternate_uri != NULL && alternate_uri[0] != '\0') {
			GDataLink *_link;

			_link = gdata_link_new (alternate_uri, GDATA_LINK_ALTERNATE);
			gdata_entry_add_link (GDATA_ENTRY (parsable), _link);
			g_object_unref (_link);
		}

		g_free (alternate_uri);
		return success;
	} else if (gdata_parser_string_from_json_member (reader, "mimeType", P_DEFAULT, &mime_type, &success, error) == TRUE) {
		if (success)
			gdata_documents_utils_add_content_type (GDATA_DOCUMENTS_ENTRY (parsable), mime_type);
		g_free (mime_type);
		return success;
	} else if (gdata_parser_int64_time_from_json_member (reader, "lastViewedByMeDate", P_DEFAULT, &(priv->last_viewed), &success, error) == TRUE ||
		   gdata_parser_string_from_json_member (reader, "kind", P_REQUIRED | P_NON_EMPTY, &kind, &success, error) == TRUE) {
		g_free (kind);
		return success;
	} else if (gdata_parser_int64_time_from_json_member (reader, "createdDate", P_DEFAULT, &published, &success, error) == TRUE) {
		if (success)
			_gdata_entry_set_published (GDATA_ENTRY (parsable), published);
		return success;
	} else if (gdata_parser_int64_time_from_json_member (reader, "modifiedDate", P_DEFAULT, &updated, &success, error) == TRUE) {
		if (success)
			_gdata_entry_set_updated (GDATA_ENTRY (parsable), updated);
		return success;
	} else if (gdata_parser_string_from_json_member (reader, "quotaBytesUsed", P_DEFAULT, &quota_used, &success, error) == TRUE) {
		gchar *end_ptr;
		guint64 val;

		/* Even though ‘quotaBytesUsed’ is documented as long,
		 * it is actually a string in the JSON.
		 */
		val = g_ascii_strtoull (quota_used, &end_ptr, 10);
		if (*end_ptr == '\0')
			priv->quota_used = (goffset) val;
		g_free (quota_used);
		return success;
	} else if (gdata_parser_string_from_json_member (reader, "fileSize", P_DEFAULT, &file_size, &success, error) == TRUE) {
		gchar *end_ptr;
		guint64 val;

		/* like 'quotaBytesUsed', 'fileSize' is also a string
		 * in the JSON.
		 */
		val = g_ascii_strtoull (file_size, &end_ptr, 10);
		if (*end_ptr == '\0')
			priv->file_size = (goffset) val;
		g_free (file_size);
		return success;
	} else if (gdata_parser_boolean_from_json_member (reader, "shared", P_DEFAULT, &shared, &success, error) == TRUE) {
		if (success && shared) {
			category = gdata_category_new ("http://schemas.google.com/g/2005/labels#shared", GDATA_CATEGORY_SCHEMA_LABELS, "shared");
			gdata_entry_add_category (GDATA_ENTRY (parsable), category);
			g_object_unref (category);
		}
		return success;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "labels") == 0) {
		guint i, members;

		if (json_reader_is_object (reader) == FALSE) {
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
			             /* Translators: the parameter is an error message */
			             _("Error parsing JSON: %s"),
			             "JSON node ‘labels’ is not an object.");
			return FALSE;
		}

		for (i = 0, members = (guint) json_reader_count_members (reader); i < members; i++) {
			gboolean starred;
			gboolean viewed;

			json_reader_read_element (reader, i);

			gdata_parser_boolean_from_json_member (reader, "starred", P_DEFAULT, &starred, &success, NULL);
			if (success && starred) {
				category = gdata_category_new (GDATA_CATEGORY_SCHEMA_LABELS_STARRED, GDATA_CATEGORY_SCHEMA_LABELS, "starred");
				gdata_entry_add_category (GDATA_ENTRY (parsable), category);
				g_object_unref (category);
			}

			gdata_parser_boolean_from_json_member (reader, "viewed", P_DEFAULT, &viewed, &success, NULL);
			if (success && viewed) {
				category = gdata_category_new ("http://schemas.google.com/g/2005/labels#viewed", GDATA_CATEGORY_SCHEMA_LABELS, "viewed");
				gdata_entry_add_category (GDATA_ENTRY (parsable), category);
				g_object_unref (category);
			}

			json_reader_end_element (reader);
		}

		return TRUE;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "owners") == 0) {
		guint i, elements;

		if (json_reader_is_array (reader) == FALSE) {
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
			             /* Translators: the parameter is an error message */
			             _("Error parsing JSON: %s"),
			             "JSON node ‘owners’ is not an array.");
			return FALSE;
		}

		/* Loop through the elements array. */
		for (i = 0, elements = json_reader_count_elements (reader); success && i < elements; i++) {
			gchar *email = NULL;
			gchar *name = NULL;

			json_reader_read_element (reader, i);

			if (json_reader_is_object (reader) == FALSE) {
				g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
				             /* Translators: the parameter is an error message */
				             _("Error parsing JSON: %s"),
				             "JSON node inside ‘owners’ is not an object.");
				success = FALSE;
				goto continue_owners;
			}

			get_kind_email_and_name (reader, &kind, &email, &name, &child_error);
			if (child_error != NULL) {
				g_propagate_error (error, child_error);
				success = FALSE;
				goto continue_owners;
			}
			if (name == NULL || name[0] == '\0') {
				g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
				             /* Translators: the parameter is an error message */
				             _("Error parsing JSON: %s"),
				             "Failed to find ‘displayName’.");
				success = FALSE;
				goto continue_owners;
			}

			if (g_strcmp0 (kind, "drive#user") == 0) {
				GDataAuthor *author;

				author = gdata_author_new (name, NULL, email);
				gdata_entry_add_author (GDATA_ENTRY (parsable), author);
				g_object_unref (author);
			} else {
				g_warning ("%s authors are not handled yet", kind);
			}

		continue_owners:
			g_free (email);
			g_free (kind);
			g_free (name);
			json_reader_end_element (reader);
		}

		return success;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "parents") == 0) {
		guint i, elements;

		if (json_reader_is_array (reader) == FALSE) {
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
			             /* Translators: the parameter is an error message */
			             _("Error parsing JSON: %s"),
			             "JSON node ‘parents’ is not an array.");
			return FALSE;
		}

		/* Loop through the elements array. */
		for (i = 0, elements = (guint) json_reader_count_elements (reader); success && i < elements; i++) {
			GDataLink *_link = NULL;
			const gchar *relation_type = NULL;
			gchar *uri = NULL;

			json_reader_read_element (reader, i);

			if (json_reader_is_object (reader) == FALSE) {
				g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
				             /* Translators: the parameter is an error message */
				             _("Error parsing JSON: %s"),
				             "JSON node inside ‘parents’ is not an object.");
				success = FALSE;
				goto continue_parents;
			}

			get_kind_and_parent_link (reader, &kind, &uri, &child_error);
			if (child_error != NULL) {
				g_propagate_error (error, child_error);
				success = FALSE;
				goto continue_parents;
			}

			if (g_strcmp0 (kind, "drive#parentReference") == 0) {
				relation_type = GDATA_LINK_PARENT;
			} else {
				g_warning ("%s parents are not handled yet", kind);
			}

			if (relation_type == NULL)
				goto continue_parents;

			_link = gdata_link_new (uri, relation_type);
			gdata_entry_add_link (GDATA_ENTRY (parsable), _link);

		continue_parents:
			g_clear_object (&_link);
			g_free (kind);
			g_free (uri);
			json_reader_end_element (reader);
		}

		return success;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "properties") == 0) {
		guint i, elements;

		if (!json_reader_is_array (reader)) {
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
				     /* Translators: the parameter is an error message */
				     _("Error parsing JSON: %s"),
				     "JSON node ‘properties’ is not an array.");
			return FALSE;
		}

		/* Loop through the properties array. */
		for (i = 0, elements = (guint) json_reader_count_elements (reader); success && i < elements; i++) {
			g_autofree gchar *key = NULL;
			g_autofree gchar *value = NULL;
			g_autofree gchar *visibility = NULL;
			g_autoptr(GDataDocumentsProperty) property = NULL;

			json_reader_read_element (reader, i);

			if (!json_reader_is_object (reader)) {
				g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
					     /* Translators: the parameter is an error message */
					     _("Error parsing JSON: %s"),
					     "JSON node inside ‘properties’ is not an object.");
				success = FALSE;
				goto continue_properties;
			}

			get_key_value_and_visibility (reader, &key, &value, &visibility, &child_error);

			if (child_error != NULL) {
				g_propagate_error (error, child_error);
				success = FALSE;
				goto continue_properties;
			}

			if (g_strcmp0 (visibility, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC) &&
			    g_strcmp0 (visibility, GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE)) {
				g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
					     /* Translators: the parameter is the invalid value of visibility variable */
					     _("Invalid visibility: unrecognized value ‘%s’"),
					     visibility);
				success = FALSE;
				goto continue_properties;
			}

			g_assert (key != NULL);

			property = gdata_documents_property_new (key);
			gdata_documents_property_set_visibility (property, visibility);
			gdata_documents_property_set_value (property, value);

			gdata_documents_entry_add_documents_property (GDATA_DOCUMENTS_ENTRY (parsable), property);

		continue_properties:
			json_reader_end_element (reader);
		}

		return success;
	} else if (gdata_parser_int64_time_from_json_member (reader, "sharedWithMeDate", P_DEFAULT, &(priv->shared_with_me_date), &success, error) == TRUE) {
		return success;
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "capabilities") == 0) {
		guint i, members;

		if (!json_reader_is_object (reader)) {
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
			             /* Translators: the parameter is an error message */
			             _("Error parsing JSON: %s"),
			             "JSON node ‘capabilities’ is not an object.");
			return FALSE;
		}

		for (i = 0, members = (guint) json_reader_count_members (reader); i < members; i++) {
			json_reader_read_element (reader, i);

			if (gdata_parser_boolean_from_json_member (reader, "canEdit", P_DEFAULT, &(priv->can_edit), &success, error) == TRUE) {
				json_reader_end_element (reader);
				break;
			}

			json_reader_end_element (reader);
		}

		return success;
	}

	return GDATA_PARSABLE_CLASS (gdata_documents_entry_parent_class)->parse_json (parsable, reader, user_data, error);
}

static gboolean
post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error)
{
	GDataLink *_link;
	const gchar *id;
	gchar *uri;

	id = gdata_entry_get_id (GDATA_ENTRY (parsable));

	/* gdata_access_handler_get_rules requires the presence of a GDATA_LINK_ACCESS_CONTROL_LIST link with the
	 * right URI. */
	uri = g_strconcat ("https://www.googleapis.com/drive/v2/files/", id, "/permissions", NULL);
	_link = gdata_link_new (uri, GDATA_LINK_ACCESS_CONTROL_LIST);
	gdata_entry_add_link (GDATA_ENTRY (parsable), _link);
	g_free (uri);
	g_object_unref (_link);

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
	GList *i;
	GList *parent_folders_list, *documents_properties_list;
	const gchar *mime_type;

	GDATA_PARSABLE_CLASS (gdata_documents_entry_parent_class)->get_json (parsable, builder);

	/* Inserting files: https://developers.google.com/drive/v2/reference/files/insert */

	mime_type = gdata_documents_utils_get_content_type (GDATA_DOCUMENTS_ENTRY (parsable));
	if (mime_type != NULL) {
		json_builder_set_member_name (builder, "mimeType");
		json_builder_add_string_value (builder, mime_type);
	}

	/* Upload to a folder: https://developers.google.com/drive/v2/web/folder */

	json_builder_set_member_name (builder, "parents");
	json_builder_begin_array (builder);

	parent_folders_list = gdata_entry_look_up_links (GDATA_ENTRY (parsable), GDATA_LINK_PARENT);
	for (i = parent_folders_list; i != NULL; i = i->next) {
		GDataLink *_link = GDATA_LINK (i->data);
		const gchar *id;

		id = gdata_documents_utils_get_id_from_link (_link);
		if (id != NULL) {
			json_builder_begin_object (builder);
			json_builder_set_member_name (builder, "kind");
			json_builder_add_string_value (builder, "drive#fileLink");
			json_builder_set_member_name (builder, "id");
			json_builder_add_string_value (builder, id);
			json_builder_end_object (builder);
		}
	}

	json_builder_end_array (builder);

	g_list_free (parent_folders_list);

	/* Set all the properties */
	json_builder_set_member_name (builder, "properties");
	json_builder_begin_array (builder);

	documents_properties_list = gdata_documents_entry_get_document_properties (GDATA_DOCUMENTS_ENTRY (parsable));
	for (i = documents_properties_list; i != NULL; i = i->next) {
		GDataDocumentsProperty *property = GDATA_DOCUMENTS_PROPERTY (i->data);
		const gchar *key = NULL;
		const gchar *value = NULL;
		const gchar *visibility = NULL;

		if (property != NULL) {
			key = gdata_documents_property_get_key (property);
			value = gdata_documents_property_get_value (property);
			visibility = gdata_documents_property_get_visibility (property);

			json_builder_begin_object (builder);

			json_builder_set_member_name (builder, "key");
			json_builder_add_string_value (builder, key);

			json_builder_set_member_name (builder, "visibility");
			json_builder_add_string_value (builder, visibility);

			json_builder_set_member_name (builder, "value");
			if (value == NULL) {
				json_builder_add_null_value (builder);
			} else {
				json_builder_add_string_value (builder, value);
			}

			json_builder_end_object (builder);
		}
	}

	json_builder_end_array (builder);
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
	return g_strconcat ("https://www.googleapis.com/drive/v2/files/", id, "?supportsAllDrives=true", NULL);
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
 * Deprecated: 0.17.0: Use gdata_entry_get_updated() instead. See #GDataDocumentsEntry:edited.
 */
gint64
gdata_documents_entry_get_edited (GDataDocumentsEntry *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), -1);
	return gdata_entry_get_updated (GDATA_ENTRY (self));
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
 */
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
 */
gchar *
gdata_documents_entry_get_path (GDataDocumentsEntry *self)
{
	GList *element, *parent_folders_list = NULL;
	GString *path;
	const gchar *id;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), NULL);

	path = g_string_new ("/");
	parent_folders_list = gdata_entry_look_up_links (GDATA_ENTRY (self), GDATA_LINK_PARENT);

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

	/* Append the entry ID */
	id = gdata_entry_get_id (GDATA_ENTRY (self));
	g_string_append (path, id);

	return g_string_free (path, FALSE);
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
 * Deprecated: 0.11.0: Use gdata_documents_entry_get_resource_id() instead. See #GDataDocumentsEntry:document-id.
 */
const gchar *
gdata_documents_entry_get_document_id (GDataDocumentsEntry *self )
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), NULL);
	return gdata_entry_get_id (GDATA_ENTRY (self));
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

void
_gdata_documents_entry_set_resource_id (GDataDocumentsEntry *self, const gchar *resource_id)
{
	g_return_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self));
	g_return_if_fail (resource_id != NULL && resource_id[0] != '\0');

	if (g_strcmp0 (resource_id, self->priv->resource_id) == 0)
		return;

	g_free (self->priv->resource_id);
	self->priv->resource_id = g_strdup (resource_id);

	g_object_notify (G_OBJECT (self), "resource-id");
}

/**
 * gdata_documents_entry_set_writers_can_invite:
 * @self: a #GDataDocumentsEntry
 * @writers_can_invite: %TRUE if writers can invite other people to edit the document, %FALSE otherwise
 *
 * Sets the #GDataDocumentsEntry:writers-can-invite property to @writers_can_invite.
 *
 * Since: 0.4.0
 */
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
 */
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
 */
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
 * gdata_documents_entry_get_file_size:
 * @self: a #GDataDocumentsEntry
 *
 * Gets the #GDataDocumentsEntry:file-size property.
 *
 * Return value: the size of the document in bytes
 *
 * Since: 0.17.7
 */
goffset
gdata_documents_entry_get_file_size (GDataDocumentsEntry *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), 0);

	return self->priv->file_size;
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
 */
gboolean
gdata_documents_entry_is_deleted (GDataDocumentsEntry *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), FALSE);
	return self->priv->is_deleted;
}


/**
 * gdata_documents_entry_get_document_properties:
 * @self: a #GDataDocumentsEntry
 *
 * Gets a list of the #GDataDocumentsPropertys for this entry.
 *
 * Return value: (transfer none) (element-type GDataDocumentsProperty): a #GList of pointers to #GDataDocumentsPropertys
 *
 * Since: 0.17.11
 */
GList *
gdata_documents_entry_get_document_properties (GDataDocumentsEntry *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), NULL);
	return self->priv->properties;
}

/**
 * gdata_documents_entry_add_documents_property:
 * @self: a #GDataDocumentsEntry
 * @property: a #GDataDocumentsProperty
 *
 * Inserts/updates @property on "properties" list in @self. Since, a GDataDocumentsProperty is uniquely identified by #GDataDocumentsProperty:key and #GDataDocumentsProperty:visibility, if no such property exists in the "properties" list, then a @property will be appended to the list after incrementing the reference count.
 *
 * In case that there already exists a @property in "properties", the #GDataDocumentsProperty inside the list will be updated to @property. Note that #GDataDocumentsProperty:value has no role in determining the uniqueness of a #GDataDocumentsProperty.
 *
 * The changes made by this function will be local only and you need to explicitly update @self by calling gdata_service_update_entry().
 *
 * Return value: %TRUE if the @property doesn't exist in the "properties" list, or that #GDataDocumentsProperty:value  of @property has been set to %NULL by a call to gdata_documents_entry_remove_documents_property(). %FALSE if @property exists in "properties" list, or in the case that @self or @property aren't of proper types.
 *
 * Since: 0.17.11
 */
gboolean
gdata_documents_entry_add_documents_property (GDataDocumentsEntry *self, GDataDocumentsProperty *property) {
	GList *l = NULL;
	gboolean ret_val = TRUE;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), FALSE);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_PROPERTY (property), FALSE);

	l = g_list_find_custom (self->priv->properties, property, (GCompareFunc) gdata_comparable_compare);

	if (l == NULL) {
		self->priv->properties = g_list_prepend (self->priv->properties, g_object_ref (property));
	} else {
		GDataDocumentsProperty *_prop = GDATA_DOCUMENTS_PROPERTY (l->data);
		if (gdata_documents_property_get_value (_prop) != NULL)
			ret_val = FALSE;
		gdata_documents_property_set_value (_prop, gdata_documents_property_get_value (property));
	}

	return ret_val;
}

/**
 * gdata_documents_entry_remove_documents_property:
 * @self: a #GDataDocumentsEntry
 * @property: a #GDataDocumentsProperty
 *
 * The property specified by @property will be removed from the "properties" list on @self.
 *
 * Only #GDataDocumentsProperty:key and #GDataDocumentsProperty:visibility will be used to find @property in "properties" list. #GDataDocumentsProperty:value has no role in determining the uniqueness of a #GDataDocumentsProperty.
 *
 * The changes made by this function will be local only and you need to explicitly update @self by calling gdata_service_update_entry().
 *
 * Return value: %TRUE if @property has been successfully removed from "properties" list on @self, %FALSE otherwise.
 *
 * Since: 0.17.11
 */
gboolean
gdata_documents_entry_remove_documents_property (GDataDocumentsEntry *self, GDataDocumentsProperty *property) {
	GList *l;

	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), FALSE);
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_PROPERTY (property), FALSE);

	l = g_list_find_custom (self->priv->properties, property, (GCompareFunc) gdata_comparable_compare);

	if (l == NULL) {
		return FALSE;
	}

	/* Google Drive has this weird behaviour that setting the properties
	 * array to empty with file/update method doesn't empties the array.
	 * For eg. Suppose we initially did an update with properties array
	 * with value [1, 2, 3]. If we again update the properties array with
	 * [1, 5, 6], the final resulting properties array will be [1, 2, 3, 5,
	 * 6] meaning that the values get appended on subsequent updates.
	 *
	 * The way to counter this is to set the "value" field in the Property
	 * Resource to NULL, which removes that Property Resource from the
	 * properties array.
	 */
	gdata_documents_property_set_value ((GDataDocumentsProperty *) l->data, NULL);

	return TRUE;
}

/**
 * gdata_documents_entry_get_shared_with_me_date:
 * @self: a #GDataDocumentsEntry
 *
 * Gets the #GDataDocumentsEntry:shared-with-me-date property. If the property is unset, `-1` will be returned.
 *
 * Return value: the UNIX timestamp for the time at which this file was shared with the user, or `-1`
 *
 * Since: 0.18.0
 */
gint64
gdata_documents_entry_get_shared_with_me_date (GDataDocumentsEntry *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), -1);
	return self->priv->shared_with_me_date;
}

/**
 * gdata_documents_entry_can_edit:
 * @self: a #GDataDocumentsEntry
 *
 * Gets the #GDataDocumentsEntry:can-edit property.
 *
 * Return value: %TRUE if the current user can edit this file, %FALSE otherwise
 *
 * Since: 0.18.0
 */
gboolean
gdata_documents_entry_can_edit (GDataDocumentsEntry *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_ENTRY (self), FALSE);
	return self->priv->can_edit;
}
