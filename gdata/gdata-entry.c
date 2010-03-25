/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2008–2010 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-entry
 * @short_description: GData entry object
 * @stability: Unstable
 * @include: gdata/gdata-entry.h
 *
 * #GDataEntry represents a single object on the online service, such as a playlist, video or calendar event. It is a snapshot of the
 * state of that object at the time of querying the service, so modifications made to a #GDataEntry will not be automatically or
 * magically propagated to the server.
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-entry.h"
#include "gdata-types.h"
#include "gdata-service.h"
#include "gdata-private.h"
#include "atom/gdata-category.h"
#include "atom/gdata-link.h"
#include "atom/gdata-author.h"

static void gdata_entry_dispose (GObject *object);
static void gdata_entry_finalize (GObject *object);
static void gdata_entry_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_entry_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static gboolean post_parse_xml (GDataParsable *parsable, gpointer user_data, GError **error);
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataEntryPrivate {
	gchar *title;
	gchar *summary;
	gchar *id;
	gchar *etag;
	GTimeVal updated;
	GTimeVal published;
	GList *categories; /* GDataCategory */
	gchar *content;
	GList *links; /* GDataLink */
	GList *authors; /* GDataAuthor */
	gchar *rights;
};

enum {
	PROP_TITLE = 1,
	PROP_SUMMARY,
	PROP_ETAG,
	PROP_ID,
	PROP_UPDATED,
	PROP_PUBLISHED,
	PROP_CONTENT,
	PROP_IS_INSERTED,
	PROP_RIGHTS
};

G_DEFINE_TYPE (GDataEntry, gdata_entry, GDATA_TYPE_PARSABLE)

static void
gdata_entry_class_init (GDataEntryClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataEntryPrivate));

	gobject_class->get_property = gdata_entry_get_property;
	gobject_class->set_property = gdata_entry_set_property;
	gobject_class->dispose = gdata_entry_dispose;
	gobject_class->finalize = gdata_entry_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->parse_xml = parse_xml;
	parsable_class->post_parse_xml = post_parse_xml;
	parsable_class->pre_get_xml = pre_get_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "entry";

	/**
	 * GDataEntry:title:
	 *
	 * A human-readable title for the entry.
	 *
	 * For more information, see the <ulink type="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.title">
	 * Atom specification</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_TITLE,
				g_param_spec_string ("title",
					"Title", "A human-readable title for the entry.",
					NULL,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataEntry:summary:
	 *
	 * A short summary, abstract, or excerpt of the entry.
	 *
	 * For more information, see the <ulink type="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.summary">
	 * Atom specification</ulink>.
	 *
	 * Since: 0.4.0
	 **/
	g_object_class_install_property (gobject_class, PROP_SUMMARY,
				g_param_spec_string ("summary",
					"Summary", "A short summary, abstract, or excerpt of the entry.",
					NULL,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataEntry:id:
	 *
	 * A permanent, universally unique identifier for the entry, in IRI form.
	 *
	 * For more information, see the <ulink type="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.id">
	 * Atom specification</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_ID,
				g_param_spec_string ("id",
					"ID", "A permanent, universally unique identifier for the entry, in IRI form.",
					NULL,
					G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataEntry:etag:
	 *
	 * An identifier for a particular version of the entry. This changes every time the entry on the server changes, and can be used
	 * for conditional retrieval and locking.
	 *
	 * For more information, see the <ulink type="http://code.google.com/apis/gdata/docs/2.0/reference.html#ResourceVersioning">
	 * GData specification</ulink>.
	 *
	 * Since: 0.2.0
	 **/
	g_object_class_install_property (gobject_class, PROP_ETAG,
				g_param_spec_string ("etag",
					"ETag", "An identifier for a particular version of the entry.",
					NULL,
					G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataEntry:updated:
	 *
	 * The date and time the entry was most recently updated in a significant way.
	 *
	 * For more information, see the <ulink type="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.updated">
	 * Atom specification</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_UPDATED,
				g_param_spec_boxed ("updated",
					"Updated", "The date and time the entry was most recently updated in a significant way.",
					GDATA_TYPE_G_TIME_VAL,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataEntry:published:
	 *
	 * The date and time the entry was first published or made available.
	 *
	 * For more information, see the <ulink type="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.published">
	 * Atom specification</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_PUBLISHED,
				g_param_spec_boxed ("published",
					"Published", "The date and time the entry was first published or made available.",
					GDATA_TYPE_G_TIME_VAL,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataEntry:content:
	 *
	 * The content of the entry.
	 *
	 * For more information, see the <ulink type="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.published">
	 * Atom specification</ulink>.
	 **/
	g_object_class_install_property (gobject_class, PROP_CONTENT,
				g_param_spec_string ("content",
					"Content", "The content of the entry.",
					NULL,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataEntry:is-inserted:
	 *
	 * Whether the entry has been inserted on the server. This is %FALSE for entries which have just been created using gdata_entry_new() and
	 * %TRUE for entries returned from the server by queries. It is set to %TRUE when an entry is inserted using gdata_service_insert_entry().
	 **/
	g_object_class_install_property (gobject_class, PROP_IS_INSERTED,
				g_param_spec_boolean ("is-inserted",
					"Inserted?", "Whether the entry has been inserted on the server.",
					FALSE,
					G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
	/**
	 * GDataEntry:rights:
	 *
	 * The ownership rights pertaining to the entry.
	 *
	 * For more information, see the <ulink type="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.rights">
	 * Atom specification</ulink>.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_RIGHTS,
					 g_param_spec_string ("rights",
							      "Rights", "The ownership rights pertaining to the entry.",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_entry_init (GDataEntry *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_ENTRY, GDataEntryPrivate);
}

static void
gdata_entry_dispose (GObject *object)
{
	GDataEntryPrivate *priv = GDATA_ENTRY (object)->priv;

	if (priv->categories != NULL) {
		g_list_foreach (priv->categories, (GFunc) g_object_unref, NULL);
		g_list_free (priv->categories);
	}
	priv->categories = NULL;

	if (priv->links != NULL) {
		g_list_foreach (priv->links, (GFunc) g_object_unref, NULL);
		g_list_free (priv->links);
	}
	priv->links = NULL;

	if (priv->authors != NULL) {
		g_list_foreach (priv->authors, (GFunc) g_object_unref, NULL);
		g_list_free (priv->authors);
	}
	priv->authors = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_entry_parent_class)->dispose (object);
}

static void
gdata_entry_finalize (GObject *object)
{
	GDataEntryPrivate *priv = GDATA_ENTRY (object)->priv;

	g_free (priv->title);
	g_free (priv->summary);
	g_free (priv->id);
	g_free (priv->etag);
	g_free (priv->rights);
	g_free (priv->content);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_entry_parent_class)->finalize (object);
}

static void
gdata_entry_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataEntryPrivate *priv = GDATA_ENTRY (object)->priv;

	switch (property_id) {
		case PROP_TITLE:
			g_value_set_string (value, priv->title);
			break;
		case PROP_SUMMARY:
			g_value_set_string (value, priv->summary);
			break;
		case PROP_ID:
			g_value_set_string (value, priv->id);
			break;
		case PROP_ETAG:
			g_value_set_string (value, priv->etag);
			break;
		case PROP_UPDATED:
			g_value_set_boxed (value, &(priv->updated));
			break;
		case PROP_PUBLISHED:
			g_value_set_boxed (value, &(priv->published));
			break;
		case PROP_CONTENT:
			g_value_set_string (value, priv->content);
			break;
		case PROP_IS_INSERTED:
			g_value_set_boolean (value, gdata_entry_is_inserted (GDATA_ENTRY (object)));
			break;
		case PROP_RIGHTS:
			g_value_set_string (value, priv->rights);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_entry_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataEntry *self = GDATA_ENTRY (object);

	switch (property_id) {
		case PROP_ID:
			/* Construct only */
			self->priv->id = g_value_dup_string (value);
			break;
		case PROP_ETAG:
			/* Construct only */
			self->priv->etag = g_value_dup_string (value);
			break;
		case PROP_TITLE:
			gdata_entry_set_title (self, g_value_get_string (value));
			break;
		case PROP_SUMMARY:
			gdata_entry_set_summary (self, g_value_get_string (value));
			break;
		case PROP_CONTENT:
			gdata_entry_set_content (self, g_value_get_string (value));
			break;
		case PROP_RIGHTS:
			gdata_entry_set_rights (self, g_value_get_string (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static gboolean
pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error)
{
	g_return_val_if_fail (GDATA_IS_ENTRY (parsable), FALSE);
	g_return_val_if_fail (doc != NULL, FALSE);
	g_return_val_if_fail (root_node != NULL, FALSE);

	/* Extract the ETag */
	GDATA_ENTRY (parsable)->priv->etag = (gchar*) xmlGetProp (root_node, (xmlChar*) "etag");

	return TRUE;
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	gboolean success;
	GDataEntryPrivate *priv = GDATA_ENTRY (parsable)->priv;

	if (gdata_parser_is_namespace (node, "http://www.w3.org/2005/Atom") == TRUE) {
		if (gdata_parser_string_from_element (node, "title", P_DEFAULT, &(priv->title), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "id", P_REQUIRED | P_NON_EMPTY | P_NO_DUPES, &(priv->id), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "summary", P_NONE, &(priv->summary), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "rights", P_NONE, &(priv->rights), &success, error) == TRUE ||
		    gdata_parser_time_val_from_element (node, "updated", P_REQUIRED | P_NO_DUPES, &(priv->updated), &success, error) == TRUE ||
		    gdata_parser_time_val_from_element (node, "published", P_REQUIRED | P_NO_DUPES, &(priv->published), &success, error) == TRUE ||
		    gdata_parser_object_from_element_setter (node, "category", P_REQUIRED, GDATA_TYPE_CATEGORY,
			                                     gdata_entry_add_category, parsable, &success, error) == TRUE ||
		    gdata_parser_object_from_element_setter (node, "link", P_REQUIRED, GDATA_TYPE_LINK,
			                                     gdata_entry_add_link, parsable, &success, error) == TRUE ||
		    gdata_parser_object_from_element_setter (node, "author", P_REQUIRED, GDATA_TYPE_AUTHOR,
			                                     gdata_entry_add_author, parsable, &success, error) == TRUE) {
			return success;
		} else if (xmlStrcmp (node->name, (xmlChar*) "content") == 0) {
			/* atom:content */
			xmlChar *content = xmlNodeListGetString (doc, node->children, TRUE);
			if (content == NULL)
				content = xmlGetProp (node, (xmlChar*) "src");
			priv->content = (gchar*) content;

			return TRUE;
		}
	}

	return GDATA_PARSABLE_CLASS (gdata_entry_parent_class)->parse_xml (parsable, doc, node, user_data, error);
}

static gboolean
post_parse_xml (GDataParsable *parsable, gpointer user_data, GError **error)
{
	GDataEntryPrivate *priv = GDATA_ENTRY (parsable)->priv;

	/* Check for missing required elements */
	/* Can't uncomment it, as things like access rules break the Atom standard */
	/*if (priv->title == NULL)
		return gdata_parser_error_required_element_missing ("title", "entry", error);
	if (priv->id == NULL)
		return gdata_parser_error_required_element_missing ("id", "entry", error);
	if (priv->updated.tv_sec == 0 && priv->updated.tv_usec == 0)
		return gdata_parser_error_required_element_missing ("updated", "entry", error);*/

	/* Reverse our lists of stuff */
	priv->categories = g_list_reverse (priv->categories);
	priv->links = g_list_reverse (priv->links);
	priv->authors = g_list_reverse (priv->authors);

	return TRUE;
}

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataEntryPrivate *priv = GDATA_ENTRY (parsable)->priv;

	/* Add the entry's ETag, if available */
	if (priv->etag != NULL)
		g_string_append_printf (xml_string, " gd:etag='%s'", priv->etag);
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataEntryPrivate *priv = GDATA_ENTRY (parsable)->priv;
	GList *categories, *links, *authors;

	gdata_parser_string_append_escaped (xml_string, "<title type='text'>", priv->title, "</title>");

	if (priv->id != NULL)
		g_string_append_printf (xml_string, "<id>%s</id>", priv->id);

	if (priv->updated.tv_sec != 0 || priv->updated.tv_usec != 0) {
		gchar *updated = g_time_val_to_iso8601 (&(priv->updated));
		g_string_append_printf (xml_string, "<updated>%s</updated>", updated);
		g_free (updated);
	}

	if (priv->published.tv_sec != 0 || priv->published.tv_usec != 0) {
		gchar *published = g_time_val_to_iso8601 (&(priv->published));
		g_string_append_printf (xml_string, "<published>%s</published>", published);
		g_free (published);
	}

	if (priv->summary != NULL)
		gdata_parser_string_append_escaped (xml_string, "<summary type='text'>", priv->summary, "</summary>");

	if (priv->rights != NULL)
		gdata_parser_string_append_escaped (xml_string, "<rights>", priv->rights, "</rights>");

	if (priv->content != NULL)
		gdata_parser_string_append_escaped (xml_string, "<content type='text'>", priv->content, "</content>");

	for (categories = priv->categories; categories != NULL; categories = categories->next)
		_gdata_parsable_get_xml (GDATA_PARSABLE (categories->data), xml_string, FALSE);

	for (links = priv->links; links != NULL; links = links->next)
		_gdata_parsable_get_xml (GDATA_PARSABLE (links->data), xml_string, FALSE);

	for (authors = priv->authors; authors != NULL; authors = authors->next)
		_gdata_parsable_get_xml (GDATA_PARSABLE (authors->data), xml_string, FALSE);
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "gd", (gchar*) "http://schemas.google.com/g/2005");
}

/**
 * gdata_entry_new:
 * @id: the entry's ID, or %NULL
 *
 * Creates a new #GDataEntry with the given ID and default properties.
 *
 * Return value: a new #GDataEntry; unref with g_object_unref()
 **/
GDataEntry *
gdata_entry_new (const gchar *id)
{
	return g_object_new (GDATA_TYPE_ENTRY, "id", id, NULL);
}

/**
 * gdata_entry_get_title:
 * @self: a #GDataEntry
 *
 * Returns the title of the entry.
 *
 * Return value: the entry's title
 **/
const gchar *
gdata_entry_get_title (GDataEntry *self)
{
	g_return_val_if_fail (GDATA_IS_ENTRY (self), NULL);
	return self->priv->title;
}

/**
 * gdata_entry_set_title:
 * @self: a #GDataEntry
 * @title: the new entry title, or %NULL
 *
 * Sets the title of the entry.
 **/
void
gdata_entry_set_title (GDataEntry *self, const gchar *title)
{
	g_return_if_fail (GDATA_IS_ENTRY (self));

	g_free (self->priv->title);
	self->priv->title = g_strdup (title);
	g_object_notify (G_OBJECT (self), "title");
}

/**
 * gdata_entry_get_summary:
 * @self: a #GDataEntry
 *
 * Returns the summary of the entry.
 *
 * Return value: the entry's summary, or %NULL
 *
 * Since: 0.4.0
 **/
const gchar *
gdata_entry_get_summary (GDataEntry *self)
{
	g_return_val_if_fail (GDATA_IS_ENTRY (self), NULL);
	return self->priv->summary;
}

/**
 * gdata_entry_set_summary:
 * @self: a #GDataEntry
 * @summary: the new entry summary, or %NULL
 *
 * Sets the summary of the entry.
 *
 * Since: 0.4.0
 **/
void
gdata_entry_set_summary (GDataEntry *self, const gchar *summary)
{
	g_return_if_fail (GDATA_IS_ENTRY (self));

	g_free (self->priv->summary);
	self->priv->summary = g_strdup (summary);
	g_object_notify (G_OBJECT (self), "summary");
}

/**
 * gdata_entry_get_id:
 * @self: a #GDataEntry
 *
 * Returns the URN ID of the entry; a unique and permanent identifier for the object the entry represents.
 *
 * Return value: the entry's ID
 **/
const gchar *
gdata_entry_get_id (GDataEntry *self)
{
	g_return_val_if_fail (GDATA_IS_ENTRY (self), NULL);
	return self->priv->id;
}

/**
 * gdata_entry_get_etag:
 * @self: a #GDataEntry
 *
 * Returns the ETag of the entry; a unique identifier for each version of the entry. For more information, see the
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/reference.html#ResourceVersioning">online documentation</ulink>.
 *
 * Return value: the entry's ETag
 *
 * Since: 0.2.0
 **/
const gchar *
gdata_entry_get_etag (GDataEntry *self)
{
	g_return_val_if_fail (GDATA_IS_ENTRY (self), NULL);
	return self->priv->etag;
}

/**
 * gdata_entry_get_updated:
 * @self: a #GDataEntry
 * @updated: a #GTimeVal
 *
 * Puts the time the entry was last updated into @updated.
 **/
void
gdata_entry_get_updated (GDataEntry *self, GTimeVal *updated)
{
	g_return_if_fail (GDATA_IS_ENTRY (self));
	g_return_if_fail (updated != NULL);
	*updated = self->priv->updated;
}

/**
 * gdata_entry_get_published:
 * @self: a #GDataEntry
 * @published: a #GTimeVal
 *
 * Puts the time the entry was originally published into @published.
 **/
void
gdata_entry_get_published (GDataEntry *self, GTimeVal *published)
{
	g_return_if_fail (GDATA_IS_ENTRY (self));
	g_return_if_fail (published != NULL);
	*published = self->priv->published;
}

/**
 * gdata_entry_add_category:
 * @self: a #GDataEntry
 * @category: a #GDataCategory to add
 *
 * Adds @category to the list of categories in the given #GDataEntry, and increments its reference count.
 *
 * Duplicate categories will not be added to the list.
 **/
void
gdata_entry_add_category (GDataEntry *self, GDataCategory *category)
{
	g_return_if_fail (GDATA_IS_ENTRY (self));
	g_return_if_fail (GDATA_IS_CATEGORY (category));

	if (g_list_find_custom (self->priv->categories, category, (GCompareFunc) gdata_category_compare) == NULL)
		self->priv->categories = g_list_prepend (self->priv->categories, g_object_ref (category));
}

/**
 * gdata_entry_get_categories:
 * @self: a #GDataEntry
 *
 * Gets a list of the #GDataCategory<!-- -->s containing this entry.
 *
 * Return value: a #GList of #GDataCategory<!-- -->s
 *
 * Since: 0.2.0
 **/
GList *
gdata_entry_get_categories (GDataEntry *self)
{
	g_return_val_if_fail (GDATA_IS_ENTRY (self), NULL);
	return self->priv->categories;
}

/**
 * gdata_entry_get_content:
 * @self: a #GDataEntry
 *
 * Returns the textual content in this entry.
 *
 * Return value: the entry's content, or %NULL
 **/
const gchar *
gdata_entry_get_content (GDataEntry *self)
{
	g_return_val_if_fail (GDATA_IS_ENTRY (self), NULL);
	return self->priv->content;
}

/**
 * gdata_entry_set_content:
 * @self: a #GDataEntry
 * @content: the new content for the entry
 *
 * Sets the entry's content to @content.
 **/
void
gdata_entry_set_content (GDataEntry *self, const gchar *content)
{
	g_return_if_fail (GDATA_IS_ENTRY (self));

	g_free (self->priv->content);
	self->priv->content = g_strdup (content);
	g_object_notify (G_OBJECT (self), "content");
}

/**
 * gdata_entry_add_link:
 * @self: a #GDataEntry
 * @link: a #GDataLink to add
 *
 * Adds @link to the list of links in the given #GDataEntry and increments its reference count.
 *
 * Duplicate links will not be added to the list.
 **/
void
gdata_entry_add_link (GDataEntry *self, GDataLink *link)
{
	/* TODO: More link API */
	g_return_if_fail (GDATA_IS_ENTRY (self));
	g_return_if_fail (GDATA_IS_LINK (link));

	if (g_list_find_custom (self->priv->links, link, (GCompareFunc) gdata_link_compare) == NULL)
		self->priv->links = g_list_prepend (self->priv->links, g_object_ref (link));
}

static gint
link_compare_cb (const GDataLink *link, const gchar *rel)
{
	return strcmp (gdata_link_get_relation_type ((GDataLink*) link), rel);
}

/**
 * gdata_entry_look_up_link:
 * @self: a #GDataEntry
 * @rel: the value of the <structfield>rel</structfield> attribute of the desired link
 *
 * Looks up a link by relation type from the list of links in the entry. If the link has one of the standard Atom relation types,
 * use one of the defined @rel values, instead of a static string. e.g. %GDATA_LINK_EDIT or %GDATA_LINK_SELF.
 *
 * In the rare event of requiring a list of links with the same @rel value, use gdata_entry_look_up_links().
 *
 * Return value: a #GDataLink, or %NULL if one was not found
 *
 * Since: 0.1.1
 **/
GDataLink *
gdata_entry_look_up_link (GDataEntry *self, const gchar *rel)
{
	GList *element;

	g_return_val_if_fail (GDATA_IS_ENTRY (self), NULL);
	g_return_val_if_fail (rel != NULL, NULL);

	element = g_list_find_custom (self->priv->links, rel, (GCompareFunc) link_compare_cb);
	if (element == NULL)
		return NULL;
	return GDATA_LINK (element->data);
}

/**
 * gdata_entry_look_up_links:
 * @self: a #GDataEntry
 * @rel: the value of the <structfield>rel</structfield> attribute of the desired links
 *
 * Looks up a list of links by relation type from the list of links in the entry. If the links have one of the standard Atom
 * relation types, use one of the defined @rel values, instead of a static string. e.g. %GDATA_LINK_EDIT or %GDATA_LINK_SELF.
 *
 * If you will only use the first link found, consider calling gdata_entry_look_up_link() instead.
 *
 * Return value: a #GList of #GDataLink<!-- -->s, or %NULL if none were found; free the list with g_list_free()
 *
 * Since: 0.4.0
 **/
GList *
gdata_entry_look_up_links (GDataEntry *self, const gchar *rel)
{
	GList *i, *results = NULL;

	g_return_val_if_fail (GDATA_IS_ENTRY (self), NULL);
	g_return_val_if_fail (rel != NULL, NULL);

	for (i = self->priv->links; i != NULL; i = i->next) {
		const gchar *relation_type = gdata_link_get_relation_type (((GDataLink*) i->data));
		if (strcmp (relation_type, rel) == 0)
			results = g_list_prepend (results, i->data);
	}

	return g_list_reverse (results);
}

/**
 * gdata_entry_add_author:
 * @self: a #GDataEntry
 * @author: a #GDataAuthor to add
 *
 * Adds @author to the list of authors in the given #GDataEntry and increments its reference count.
 *
 * Duplicate authors will not be added to the list.
 **/
void
gdata_entry_add_author (GDataEntry *self, GDataAuthor *author)
{
	/* TODO: More author API */
	g_return_if_fail (GDATA_IS_ENTRY (self));
	g_return_if_fail (author != NULL);

	if (g_list_find_custom (self->priv->authors, author, (GCompareFunc) gdata_author_compare) == NULL)
		self->priv->authors = g_list_prepend (self->priv->authors, g_object_ref (author));
}

/**
 * gdata_entry_is_inserted:
 * @self: a #GDataEntry
 *
 * Returns whether the entry is marked as having been inserted on (uploaded to) the server already.
 *
 * Return value: %TRUE if the entry has been inserted already, %FALSE otherwise
 **/
gboolean
gdata_entry_is_inserted (GDataEntry *self)
{
	g_return_val_if_fail (GDATA_IS_ENTRY (self), FALSE);

	if (self->priv->id != NULL ||
	    self->priv->updated.tv_sec != 0 || self->priv->updated.tv_usec != 0)
		return TRUE;
	return FALSE;
}

/**
 * gdata_entry_get_rights:
 * @self: a #GDataEntry
 *
 * Returns the rights pertaining to the entry, or %NULL if not set.
 *
 * Return value: the entry's rights information
 *
 * Since: 0.5.0
 **/
const gchar *
gdata_entry_get_rights (GDataEntry *self)
{
	g_return_val_if_fail (GDATA_IS_ENTRY (self), NULL);
	return self->priv->rights;
}

/**
 * gdata_entry_set_rights:
 * @self: a #GDataEntry
 * @rights: the new rights, or %NULL
 *
 * Sets the rights for this entry.
 *
 * Since: 0.5.0
 **/
void
gdata_entry_set_rights (GDataEntry *self, const gchar *rights)
{
	g_return_if_fail (GDATA_IS_ENTRY (self));

	g_free (self->priv->rights);
	self->priv->rights = g_strdup (rights);
	g_object_notify (G_OBJECT (self), "rights");
}
