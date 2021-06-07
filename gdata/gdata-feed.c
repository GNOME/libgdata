/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2008–2010 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-feed
 * @short_description: GData feed object
 * @stability: Stable
 * @include: gdata/gdata-feed.h
 *
 * #GDataFeed is a list of entries (#GDataEntry) returned as the result of a query to a #GDataService, or given as the input to another
 * operation on the online service. It also has pieces of data associated with the query on the #GDataService, such as the query title
 * or timestamp when it was last updated.
 *
 * Each #GDataEntry represents a single object on the online service, such as a playlist, video or calendar entry, and the #GDataFeed
 * represents a collection of similar objects.
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/parser.h>
#include <string.h>
#include <json-glib/json-glib.h>

#include "gdata-feed.h"
#include "gdata-entry.h"
#include "gdata-types.h"
#include "gdata-private.h"
#include "gdata-service.h"
#include "gdata-parsable.h"

static void gdata_feed_dispose (GObject *object);
static void gdata_feed_finalize (GObject *object);
static void gdata_feed_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static gboolean post_parse_xml (GDataParsable *parsable, gpointer user_data, GError **error);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

static void _gdata_feed_add_category (GDataFeed *self, GDataCategory *category);
static void _gdata_feed_add_author (GDataFeed *self, GDataAuthor *author);

static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);
static gboolean post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error);

struct _GDataFeedPrivate {
	GList *entries;
	gchar *title;
	gchar *subtitle;
	gchar *id;
	gchar *etag;
	gint64 updated;
	GList *categories; /* GDataCategory */
	gchar *logo;
	gchar *icon;
	GList *links; /* GDataLink */
	GList *authors; /* GDataAuthor */
	GDataGenerator *generator;
	guint items_per_page;
	guint start_index;
	guint total_results;
	gchar *rights;
	gchar *next_page_token;
};

enum {
	PROP_ID = 1,
	PROP_ETAG,
	PROP_UPDATED,
	PROP_TITLE,
	PROP_SUBTITLE,
	PROP_LOGO,
	PROP_ICON,
	PROP_GENERATOR,
	PROP_ITEMS_PER_PAGE,
	PROP_START_INDEX,
	PROP_TOTAL_RESULTS,
	PROP_RIGHTS,
	PROP_NEXT_PAGE_TOKEN,
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataFeed, gdata_feed, GDATA_TYPE_PARSABLE)

static void
gdata_feed_class_init (GDataFeedClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_feed_get_property;
	gobject_class->dispose = gdata_feed_dispose;
	gobject_class->finalize = gdata_feed_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->parse_xml = parse_xml;
	parsable_class->post_parse_xml = post_parse_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "feed";

	parsable_class->parse_json = parse_json;
	parsable_class->post_parse_json = post_parse_json;

	/**
	 * GDataFeed:title:
	 *
	 * The title of the feed.
	 *
	 * API reference:
	 * <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_title">atom:title</ulink>
	 */
	g_object_class_install_property (gobject_class, PROP_TITLE,
	                                 g_param_spec_string ("title",
	                                                      "Title", "The title of the feed.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataFeed:subtitle:
	 *
	 * The subtitle of the feed.
	 *
	 * API reference: <ulink type="http" url="http://atomenabled.org/developers/syndication/">atom:subtitle</ulink>
	 */
	g_object_class_install_property (gobject_class, PROP_SUBTITLE,
	                                 g_param_spec_string ("subtitle",
	                                                      "Subtitle", "The subtitle of the feed.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataFeed:id:
	 *
	 * The unique and permanent URN ID for the feed.
	 *
	 * API reference: <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_id">atom:id</ulink>
	 */
	g_object_class_install_property (gobject_class, PROP_ID,
	                                 g_param_spec_string ("id",
	                                                      "ID", "The unique and permanent URN ID for the feed.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataFeed:etag:
	 *
	 * The unique ETag for this version of the feed. See the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/reference.html#ResourceVersioning">online documentation</ulink> for
	 * more information.
	 *
	 * Since: 0.2.0
	 */
	g_object_class_install_property (gobject_class, PROP_ETAG,
	                                 g_param_spec_string ("etag",
	                                                      "ETag", "The unique ETag for this version of the feed.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataFeed:updated:
	 *
	 * The time the feed was last updated.
	 *
	 * API reference: <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_updated">
	 * atom:updated</ulink>
	 */
	g_object_class_install_property (gobject_class, PROP_UPDATED,
	                                 g_param_spec_int64 ("updated",
	                                                     "Updated", "The time the feed was last updated.",
	                                                     0, G_MAXINT64, 0,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataFeed:logo:
	 *
	 * The URI of a logo for the feed.
	 *
	 * API reference: <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_logo">atom:logo</ulink>
	 */
	g_object_class_install_property (gobject_class, PROP_LOGO,
	                                 g_param_spec_string ("logo",
	                                                      "Logo", "The URI of a logo for the feed.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataFeed:icon:
	 *
	 * The URI of an icon for the feed.
	 *
	 * API reference:
	 * <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.icon">atom:icon</ulink>
	 *
	 * Since: 0.6.0
	 */
	g_object_class_install_property (gobject_class, PROP_ICON,
	                                 g_param_spec_string ("icon",
	                                                      "Icon", "The URI of an icon for the feed.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataFeed:generator:
	 *
	 * Details of the software used to generate the feed.
	 *
	 * API reference: <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_generator">
	 * atom:generator</ulink>
	 */
	g_object_class_install_property (gobject_class, PROP_GENERATOR,
	                                 g_param_spec_object ("generator",
	                                                      "Generator", "Details of the software used to generate the feed.",
	                                                      GDATA_TYPE_GENERATOR,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataFeed:rights:
	 *
	 * The ownership rights pertaining to the entire feed.
	 *
	 * For more information, see the <ulink type="http"
	 * url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.rights">Atom specification</ulink>.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_RIGHTS,
	                                 g_param_spec_string ("rights",
	                                                      "Rights", "The ownership rights pertaining to the entire feed.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataFeed:items-per-page:
	 *
	 * The number of items per results page feed.
	 *
	 * API reference:
	 * <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_openSearch:itemsPerPage">
	 * openSearch:itemsPerPage</ulink>
	 */
	g_object_class_install_property (gobject_class, PROP_ITEMS_PER_PAGE,
	                                 g_param_spec_uint ("items-per-page",
	                                                    "Items per page", "The number of items per results page feed.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataFeed:start-index:
	 *
	 * The one-based index of the first item in the results feed.
	 *
	 * This should <emphasis>not</emphasis> be used manually for pagination. Instead, use a #GDataQuery and call its gdata_query_next_page()
	 * or gdata_query_previous_page() functions before making the query to the service.
	 *
	 * API reference: <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_openSearch:startIndex">
	 * openSearch:startIndex</ulink>
	 */
	g_object_class_install_property (gobject_class, PROP_START_INDEX,
	                                 g_param_spec_uint ("start-index",
	                                                    "Start index", "The one-based index of the first item in the results feed.",
	                                                    1, G_MAXUINT, 1,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataFeed:total-results:
	 *
	 * The number of items in the result set for the feed, including those on other pages. If this is zero, the total number is unknown.
	 *
	 * This should <emphasis>not</emphasis> be used manually for pagination. Instead, use a #GDataQuery and call its gdata_query_next_page()
	 * or gdata_query_previous_page() functions before making the query to the service.
	 *
	 * API reference:
	 * <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_openSearch:totalResults">
	 * openSearch:totalResults</ulink>
	 */
	g_object_class_install_property (gobject_class, PROP_TOTAL_RESULTS,
	                                 g_param_spec_uint ("total-results",
	                                                    "Total results", "The total number of results in the feed.",
	                                                    0, 1000000, 0,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataFeed:next-page-token:
	 *
	 * The next page token for feeds. Pass this to
	 * gdata_query_set_page_token() to advance to the next page when
	 * querying APIs which use page tokens rather than page numbers or
	 * offsets.
	 *
	 * Since: 0.17.7
	 */
	g_object_class_install_property (gobject_class, PROP_NEXT_PAGE_TOKEN,
	                                 g_param_spec_string ("next-page-token",
	                                                      "Next page token", "The next page token for feeds.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_feed_init (GDataFeed *self)
{
	self->priv = gdata_feed_get_instance_private (self);
	self->priv->updated = -1;
}

static void
gdata_feed_dispose (GObject *object)
{
	GDataFeedPrivate *priv = GDATA_FEED (object)->priv;

	g_list_free_full (priv->entries, g_object_unref);
	priv->entries = NULL;

	g_list_free_full (priv->categories, g_object_unref);
	priv->categories = NULL;

	g_list_free_full (priv->links, g_object_unref);
	priv->links = NULL;

	g_list_free_full (priv->authors, g_object_unref);
	priv->authors = NULL;

	if (priv->generator != NULL)
		g_object_unref (priv->generator);
	priv->generator = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_feed_parent_class)->dispose (object);
}

static void
gdata_feed_finalize (GObject *object)
{
	GDataFeedPrivate *priv = GDATA_FEED (object)->priv;

	g_free (priv->title);
	g_free (priv->subtitle);
	g_free (priv->id);
	g_free (priv->etag);
	g_free (priv->logo);
	g_free (priv->icon);
	g_free (priv->rights);
	g_free (priv->next_page_token);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_feed_parent_class)->finalize (object);
}

static void
gdata_feed_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataFeedPrivate *priv = GDATA_FEED (object)->priv;

	switch (property_id) {
		case PROP_TITLE:
			g_value_set_string (value, priv->title);
			break;
		case PROP_SUBTITLE:
			g_value_set_string (value, priv->subtitle);
			break;
		case PROP_ID:
			g_value_set_string (value, priv->id);
			break;
		case PROP_ETAG:
			g_value_set_string (value, priv->etag);
			break;
		case PROP_UPDATED:
			g_value_set_int64 (value, priv->updated);
			break;
		case PROP_LOGO:
			g_value_set_string (value, priv->logo);
			break;
		case PROP_ICON:
			g_value_set_string (value, priv->icon);
			break;
		case PROP_GENERATOR:
			g_value_set_object (value, priv->generator);
			break;
		case PROP_RIGHTS:
			g_value_set_string (value, priv->rights);
			break;
		case PROP_ITEMS_PER_PAGE:
			g_value_set_uint (value, priv->items_per_page);
			break;
		case PROP_START_INDEX:
			g_value_set_uint (value, priv->start_index);
			break;
		case PROP_TOTAL_RESULTS:
			g_value_set_uint (value, priv->total_results);
			break;
		case PROP_NEXT_PAGE_TOKEN:
			g_value_set_string (value, priv->next_page_token);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

typedef struct {
	GType entry_type;
	GDataQueryProgressCallback progress_callback;
	gpointer progress_user_data;
	guint entry_i;
} ParseData;

static gboolean
pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error)
{
	/* Extract the ETag */
	GDATA_FEED (parsable)->priv->etag = (gchar*) xmlGetProp (root_node, (xmlChar*) "etag");
	return TRUE;
}

typedef struct {
	GDataQueryProgressCallback progress_callback;
	gpointer progress_user_data;
	GDataEntry *entry;
	guint entry_i;
	guint total_results;
} ProgressCallbackData;

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	gboolean success;
	GDataFeed *self = GDATA_FEED (parsable);
	ParseData *data = user_data;

	if (gdata_parser_is_namespace (node, "http://www.w3.org/2005/Atom") == TRUE) {
		if (xmlStrcmp (node->name, (xmlChar*) "entry") == 0) {
			/* atom:entry */
			GDataEntry *entry;
			GType entry_type;

			/* Allow @data to be %NULL, and assume we're parsing a vanilla feed, so that we can test #GDataFeed in tests/general.c.
			 * A little hacky, but not too much so, and valuable for testing. */
			entry_type = (data != NULL) ? data->entry_type : GDATA_TYPE_ENTRY;
			entry = GDATA_ENTRY (_gdata_parsable_new_from_xml_node (entry_type, doc, node, NULL, error));
			if (entry == NULL)
				return FALSE;

			/* Calls the callbacks in the main thread */
			if (data != NULL)
				_gdata_feed_call_progress_callback (self, data, entry);
			_gdata_feed_add_entry (self, entry);
			g_object_unref (entry);
		} else if (gdata_parser_string_from_element (node, "title", P_DEFAULT | P_NO_DUPES, &(self->priv->title), &success, error) == TRUE ||
		           gdata_parser_string_from_element (node, "subtitle", P_NO_DUPES, &(self->priv->subtitle), &success, error) == TRUE ||
		           gdata_parser_string_from_element (node, "id", P_REQUIRED | P_NON_EMPTY | P_NO_DUPES,
		                                             &(self->priv->id), &success, error) == TRUE ||
		           gdata_parser_string_from_element (node, "logo", P_NO_DUPES, &(self->priv->logo), &success, error) == TRUE ||
		           gdata_parser_string_from_element (node, "icon", P_NO_DUPES, &(self->priv->icon), &success, error) == TRUE ||
		           gdata_parser_object_from_element_setter (node, "category", P_REQUIRED, GDATA_TYPE_CATEGORY,
		                                                    _gdata_feed_add_category, self, &success, error) == TRUE ||
		           gdata_parser_object_from_element_setter (node, "link", P_REQUIRED, GDATA_TYPE_LINK,
		                                                    _gdata_feed_add_link, self, &success, error) == TRUE ||
		           gdata_parser_object_from_element_setter (node, "author", P_REQUIRED, GDATA_TYPE_AUTHOR,
		                                                    _gdata_feed_add_author, self, &success, error) == TRUE ||
		           gdata_parser_object_from_element (node, "generator", P_REQUIRED | P_NO_DUPES, GDATA_TYPE_GENERATOR,
		                                             &(self->priv->generator), &success, error) == TRUE ||
		           gdata_parser_int64_time_from_element (node, "updated", P_REQUIRED | P_NO_DUPES,
		                                                 &(self->priv->updated), &success, error) == TRUE ||
		           gdata_parser_string_from_element (node, "rights", P_NONE, &(self->priv->rights), &success, error) == TRUE) {
			return success;
		} else {
			return GDATA_PARSABLE_CLASS (gdata_feed_parent_class)->parse_xml (parsable, doc, node, user_data, error);
		}
	} else if (gdata_parser_is_namespace (node, "http://a9.com/-/spec/opensearch/1.1/") == TRUE) {
		if (xmlStrcmp (node->name, (xmlChar*) "totalResults") == 0) {
			/* openSearch:totalResults */
			xmlChar *total_results_string;

			/* Duplicate checking */
			if (self->priv->total_results != 0)
				return gdata_parser_error_duplicate_element (node, error);

			/* Parse the number */
			total_results_string = xmlNodeListGetString (doc, node->children, TRUE);
			if (total_results_string == NULL)
				return gdata_parser_error_required_content_missing (node, error);

			self->priv->total_results = g_ascii_strtoull ((gchar*) total_results_string, NULL, 10);
			xmlFree (total_results_string);
		} else if (xmlStrcmp (node->name, (xmlChar*) "startIndex") == 0) {
			/* openSearch:startIndex */
			xmlChar *start_index_string;

			/* Duplicate checking */
			if (self->priv->start_index != 0)
				return gdata_parser_error_duplicate_element (node, error);

			/* Parse the number */
			start_index_string = xmlNodeListGetString (doc, node->children, TRUE);
			if (start_index_string == NULL)
				return gdata_parser_error_required_content_missing (node, error);

			self->priv->start_index = g_ascii_strtoull ((gchar*) start_index_string, NULL, 10);
			xmlFree (start_index_string);
		} else if (xmlStrcmp (node->name, (xmlChar*) "itemsPerPage") == 0) {
			/* openSearch:itemsPerPage */
			xmlChar *items_per_page_string;

			/* Duplicate checking */
			if (self->priv->items_per_page != 0)
				return gdata_parser_error_duplicate_element (node, error);

			/* Parse the number */
			items_per_page_string = xmlNodeListGetString (doc, node->children, TRUE);
			if (items_per_page_string == NULL)
				return gdata_parser_error_required_content_missing (node, error);

			self->priv->items_per_page = g_ascii_strtoull ((gchar*) items_per_page_string, NULL, 10);
			xmlFree (items_per_page_string);
		} else {
			return GDATA_PARSABLE_CLASS (gdata_feed_parent_class)->parse_xml (parsable, doc, node, user_data, error);
		}
	} else {
		return GDATA_PARSABLE_CLASS (gdata_feed_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}

	return TRUE;
}

static gboolean
post_parse_xml (GDataParsable *parsable, gpointer user_data, GError **error)
{
	GDataFeedPrivate *priv = GDATA_FEED (parsable)->priv;

	/* Check for missing required elements */
	/* FIXME: The YouTube comments feed seems to have lost its <feed/title> element, making it an invalid Atom feed and meaning
	 * the check below has to be commented out.
	 * Filed as: https://code.google.com/p/gdata-issues/issues/detail?id=2908.
	 * Discovered in: https://bugzilla.gnome.org/show_bug.cgi?id=679072#c12. */
	/*if (priv->title == NULL)
		return gdata_parser_error_required_element_missing ("title", "feed", error);*/
	if (priv->id == NULL)
		return gdata_parser_error_required_element_missing ("id", "feed", error);
	if (priv->updated == -1)
		return gdata_parser_error_required_element_missing ("updated", "feed", error);

	/* Reverse our lists of stuff */
	priv->entries = g_list_reverse (priv->entries);
	priv->categories = g_list_reverse (priv->categories);
	priv->links = g_list_reverse (priv->links);
	priv->authors = g_list_reverse (priv->authors);

	return TRUE;
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataFeedPrivate *priv = GDATA_FEED (parsable)->priv;
	GList *entries;
	gchar *updated;

	/* NOTE: Only the required elements are implemented at the moment */
	gdata_parser_string_append_escaped (xml_string, "<title type='text'>", priv->title, "</title>");
	gdata_parser_string_append_escaped (xml_string, "<id>", priv->id, "</id>");

	updated = gdata_parser_int64_to_iso8601 (priv->updated);
	g_string_append_printf (xml_string, "<updated>%s</updated>", updated);
	g_free (updated);

	/* Entries */
	for (entries = priv->entries; entries != NULL; entries = entries->next)
		_gdata_parsable_get_xml (GDATA_PARSABLE (entries->data), xml_string, FALSE);
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	GDataFeedPrivate *priv = GDATA_FEED (parsable)->priv;
	GList *i;

	/* We can't assume that all the entries in the feed have identical namespaces, so we have to call get_namespaces() for all of them.
	 * GDataBatchFeeds, for example, can easily contain entries with differing sets of namespaces. */
	for (i = priv->entries; i != NULL; i = i->next)
		GDATA_PARSABLE_GET_CLASS (i->data)->get_namespaces (GDATA_PARSABLE (i->data), namespaces);
}

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	GDataFeed *self = GDATA_FEED (parsable);
	ParseData *data = user_data;

	if (g_strcmp0 (json_reader_get_member_name (reader), "items") == 0) {
		gint i, elements;

		/* Loop through the elements array. */
		for (i = 0, elements = json_reader_count_elements (reader); i < elements; i++) {
			GDataEntry *entry;
			GType entry_type;

			json_reader_read_element (reader, i);

			/* Allow @data to be %NULL, and assume we're parsing a vanilla feed, so that we can test #GDataFeed in tests/general.c.
			 * A little hacky, but not too much so, and valuable for testing. */
			entry_type = (data != NULL) ? data->entry_type : GDATA_TYPE_ENTRY;

			/* Parse the node, passing it the reader cursor. */
			entry = GDATA_ENTRY (_gdata_parsable_new_from_json_node (entry_type, reader, NULL, error));
			if (entry == NULL) {
				json_reader_end_element (reader);
				return FALSE;
			}

			/* Calls the callbacks in the main thread */
			if (data != NULL)
				_gdata_feed_call_progress_callback (self, data, entry);
			_gdata_feed_add_entry (self, entry);
			g_object_unref (entry);

			json_reader_end_element (reader);
		}
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "selfLink") == 0) {
		GDataLink *_link;
		const gchar *uri;

		/* Empty URI? */
		uri = json_reader_get_string_value (reader);
		if (uri == NULL || *uri == '\0') {
			return gdata_parser_error_required_json_content_missing (reader, error);
		}

		_link = gdata_link_new (uri, GDATA_LINK_SELF);
		_gdata_feed_add_link (self, _link);
		g_object_unref (_link);
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "kind") == 0) {
		/* Ignore. */
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "etag") == 0) {
		GDATA_FEED (parsable)->priv->etag = g_strdup (json_reader_get_string_value (reader));
	} else if (g_strcmp0 (json_reader_get_member_name (reader), "nextPageToken") == 0) {
		GDATA_FEED (parsable)->priv->next_page_token = g_strdup (json_reader_get_string_value (reader));
	} else {
		return GDATA_PARSABLE_CLASS (gdata_feed_parent_class)->parse_json (parsable, reader, user_data, error);
	}

	return TRUE;
}

static gboolean
post_parse_json (GDataParsable *parsable, gpointer user_data, GError **error)
{
	GDataFeedPrivate *priv = GDATA_FEED (parsable)->priv;

	/* Reverse our lists of stuff. */
	priv->entries = g_list_reverse (priv->entries);

	return TRUE;
}

/* Internal helper method to set these properties. */
void
_gdata_feed_set_page_info (GDataFeed *self, guint total_results,
                           guint items_per_page)
{
	g_return_if_fail (GDATA_IS_FEED (self));

	self->priv->total_results = total_results;
	self->priv->items_per_page = items_per_page;
}

/*
 * _gdata_feed_new:
 * @feed_type: the type of #GDataFeed subclass
 * @title: the feed's title
 * @id: the feed's ID
 * @updated: when the feed was last updated
 *
 * Creates a new #GDataFeed or subclass with the bare minimum of data to be
 * valid.
 *
 * Return value: a new #GDataFeed
 *
 * Since: 0.17.0
 */
GDataFeed *
_gdata_feed_new (GType feed_type,
                 const gchar *title,
                 const gchar *id,
                 gint64 updated)
{
	GDataFeed *feed;

	g_return_val_if_fail (g_type_is_a (feed_type, GDATA_TYPE_FEED), NULL);
	g_return_val_if_fail (title != NULL, NULL);
	g_return_val_if_fail (id != NULL, NULL);
	g_return_val_if_fail (updated >= 0, NULL);

	feed = g_object_new (feed_type, NULL);
	feed->priv->title = g_strdup (title);
	feed->priv->id = g_strdup (id);
	feed->priv->updated = updated;

	return feed;
}

GDataFeed *
_gdata_feed_new_from_xml (GType feed_type, const gchar *xml, gint length, GType entry_type,
                          GDataQueryProgressCallback progress_callback, gpointer progress_user_data, GError **error)
{
	ParseData *data;
	GDataFeed *feed;

	g_return_val_if_fail (g_type_is_a (feed_type, GDATA_TYPE_FEED), NULL);
	g_return_val_if_fail (xml != NULL, NULL);
	g_return_val_if_fail (g_type_is_a (entry_type, GDATA_TYPE_ENTRY), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	data = _gdata_feed_parse_data_new (entry_type, progress_callback, progress_user_data);
	feed = GDATA_FEED (_gdata_parsable_new_from_xml (feed_type, xml, length, data, error));
	_gdata_feed_parse_data_free (data);

	return feed;
}

GDataFeed *
_gdata_feed_new_from_json (GType feed_type, const gchar *json, gint length, GType entry_type,
                          GDataQueryProgressCallback progress_callback, gpointer progress_user_data, GError **error)
{
	ParseData *data;
	GDataFeed *feed;

	g_return_val_if_fail (g_type_is_a (feed_type, GDATA_TYPE_FEED), NULL);
	g_return_val_if_fail (json != NULL, NULL);
	g_return_val_if_fail (g_type_is_a (entry_type, GDATA_TYPE_ENTRY), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	data = _gdata_feed_parse_data_new (entry_type, progress_callback, progress_user_data);
	feed = GDATA_FEED (_gdata_parsable_new_from_json (feed_type, json, length, data, error));
	_gdata_feed_parse_data_free (data);

	return feed;
}

/**
 * gdata_feed_get_entries:
 * @self: a #GDataFeed
 *
 * Returns a list of the entries contained in this feed.
 *
 * Return value: (element-type GData.Entry) (transfer none): a #GList of #GDataEntrys
 */
GList *
gdata_feed_get_entries (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), NULL);
	return self->priv->entries;
}

static gint
entry_compare_cb (const GDataEntry *entry, const gchar *id)
{
	return strcmp (gdata_entry_get_id (GDATA_ENTRY (entry)), id);
}

/**
 * gdata_feed_look_up_entry:
 * @self: a #GDataFeed
 * @id: the entry's ID
 *
 * Returns the entry in the feed with the given @id, if found.
 *
 * Return value: (transfer none): the #GDataEntry, or %NULL
 *
 * Since: 0.2.0
 */
GDataEntry *
gdata_feed_look_up_entry (GDataFeed *self, const gchar *id)
{
	GList *element;

	g_return_val_if_fail (GDATA_IS_FEED (self), NULL);
	g_return_val_if_fail (id != NULL, NULL);

	element = g_list_find_custom (self->priv->entries, id, (GCompareFunc) entry_compare_cb);
	if (element == NULL)
		return NULL;
	return GDATA_ENTRY (element->data);
}

/**
 * gdata_feed_get_categories:
 * @self: a #GDataFeed
 *
 * Returns a list of the categories listed in this feed.
 *
 * Return value: (element-type GData.Category) (transfer none): a #GList of #GDataCategorys
 */
GList *
gdata_feed_get_categories (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), NULL);
	return self->priv->categories;
}

static void
_gdata_feed_add_category (GDataFeed *self, GDataCategory *category)
{
	self->priv->categories = g_list_prepend (self->priv->categories, g_object_ref (category));
}

/**
 * gdata_feed_get_links:
 * @self: a #GDataFeed
 *
 * Returns a list of the links listed in this feed.
 *
 * Return value: (element-type GData.Link) (transfer none): a #GList of #GDataLinks
 */
GList *
gdata_feed_get_links (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), NULL);
	return self->priv->links;
}

static gint
link_compare_cb (const GDataLink *_link, const gchar *rel)
{
	return strcmp (gdata_link_get_relation_type ((GDataLink*) _link), rel);
}

/**
 * gdata_feed_look_up_link:
 * @self: a #GDataFeed
 * @rel: the value of the #GDataLink:relation-type property of the desired link
 *
 * Looks up a link by #GDataLink:relation-type value from the list of links in the feed.
 *
 * Return value: (transfer none): a #GDataLink, or %NULL if one was not found
 *
 * Since: 0.1.1
 */
GDataLink *
gdata_feed_look_up_link (GDataFeed *self, const gchar *rel)
{
	GList *element;

	g_return_val_if_fail (GDATA_IS_FEED (self), NULL);
	g_return_val_if_fail (rel != NULL, NULL);

	element = g_list_find_custom (self->priv->links, rel, (GCompareFunc) link_compare_cb);
	if (element == NULL)
		return NULL;
	return GDATA_LINK (element->data);
}

void
_gdata_feed_add_link (GDataFeed *self, GDataLink *_link)
{
	self->priv->links = g_list_prepend (self->priv->links, g_object_ref (_link));
}

/**
 * gdata_feed_get_authors:
 * @self: a #GDataFeed
 *
 * Returns a list of the authors listed in this feed.
 *
 * Return value: (element-type GData.Author) (transfer none): a #GList of #GDataAuthors
 */
GList *
gdata_feed_get_authors (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), NULL);
	return self->priv->authors;
}

static void
_gdata_feed_add_author (GDataFeed *self, GDataAuthor *author)
{
	self->priv->authors = g_list_prepend (self->priv->authors, g_object_ref (author));
}

/**
 * gdata_feed_get_title:
 * @self: a #GDataFeed
 *
 * Returns the title of the feed.
 *
 * Return value: the feed's title
 */
const gchar *
gdata_feed_get_title (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), NULL);
	return self->priv->title;
}

/**
 * gdata_feed_get_subtitle:
 * @self: a #GDataFeed
 *
 * Returns the subtitle of the feed.
 *
 * Return value: the feed's subtitle, or %NULL
 */
const gchar *
gdata_feed_get_subtitle (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), NULL);
	return self->priv->subtitle;
}

/**
 * gdata_feed_get_id:
 * @self: a #GDataFeed
 *
 * Returns the feed's unique and permanent URN ID.
 *
 * Return value: the feed's ID
 */
const gchar *
gdata_feed_get_id (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), NULL);
	return self->priv->id;
}

/**
 * gdata_feed_get_etag:
 * @self: a #GDataFeed
 *
 * Returns the feed's unique ETag for this version.
 *
 * Return value: the feed's ETag
 *
 * Since: 0.2.0
 */
const gchar *
gdata_feed_get_etag (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), NULL);
	return self->priv->etag;
}

/**
 * gdata_feed_get_updated:
 * @self: a #GDataFeed
 *
 * Gets the time the feed was last updated.
 *
 * Return value: the UNIX timestamp for the time the feed was last updated
 */
gint64
gdata_feed_get_updated (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), -1);
	return self->priv->updated;
}

/**
 * gdata_feed_get_logo:
 * @self: a #GDataFeed
 *
 * Returns the logo URI of the feed.
 *
 * Return value: the feed's logo URI, or %NULL
 */
const gchar *
gdata_feed_get_logo (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), NULL);
	return self->priv->logo;
}

/**
 * gdata_feed_get_icon:
 * @self: a #GDataFeed
 *
 * Returns the icon URI of the feed.
 *
 * Return value: the feed's icon URI, or %NULL
 *
 * Since: 0.6.0
 */
const gchar *
gdata_feed_get_icon (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), NULL);
	return self->priv->icon;
}

/**
 * gdata_feed_get_generator:
 * @self: a #GDataFeed
 *
 * Returns details about the software which generated the feed.
 *
 * Return value: (transfer none): a #GDataGenerator, or %NULL
 */
GDataGenerator *
gdata_feed_get_generator (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), NULL);
	return self->priv->generator;
}

/**
 * gdata_feed_get_rights:
 * @self: a #GDataFeed
 *
 * Returns the rights pertaining to the entire feed, or %NULL if not set.
 *
 * Return value: the feed's rights information
 *
 * Since: 0.7.0
 */
const gchar *
gdata_feed_get_rights (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), NULL);
	return self->priv->rights;
}

/**
 * gdata_feed_get_items_per_page:
 * @self: a #GDataFeed
 *
 * Returns the number of items per results page feed.
 *
 * Return value: the number of items per results page feed, or <code class="literal">0</code>
 */
guint
gdata_feed_get_items_per_page (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), 0);
	return self->priv->items_per_page;
}

/**
 * gdata_feed_get_start_index:
 * @self: a #GDataFeed
 *
 * Returns the one-based start index of the results feed in the result set.
 *
 * Return value: the one-based start index, or <code class="literal">0</code>
 */
guint
gdata_feed_get_start_index (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), 0);
	return self->priv->start_index;
}

/**
 * gdata_feed_get_total_results:
 * @self: a #GDataFeed
 *
 * Returns the total number of results in the result set, including results on other
 * pages. If this is zero, the total number is unknown.
 *
 * Return value: the total number of results, or <code class="literal">0</code>
 */
guint
gdata_feed_get_total_results (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), 0);
	return self->priv->total_results;
}

/**
 * gdata_feed_get_next_page_token:
 * @self: a #GDataFeed
 *
 * Returns the next page token for a query result, or %NULL if not set.
 * This is #GDataFeed:next-page-token. The page token might not be set if there
 * is no next page, or if this service does not use token based paging (for
 * example, if it uses page number or offset based paging instead). Most more
 * recent services use token based paging.
 *
 * Return value: (nullable): the next page token
 *
 * Since: 0.17.7
 */
const gchar *
gdata_feed_get_next_page_token (GDataFeed *self)
{
	g_return_val_if_fail (GDATA_IS_FEED (self), NULL);
	return self->priv->next_page_token;
}

void
_gdata_feed_add_entry (GDataFeed *self, GDataEntry *entry)
{
	g_return_if_fail (GDATA_IS_FEED (self));
	g_return_if_fail (GDATA_IS_ENTRY (entry));
	self->priv->entries = g_list_prepend (self->priv->entries, g_object_ref (entry));
}

gpointer
_gdata_feed_parse_data_new (GType entry_type, GDataQueryProgressCallback progress_callback, gpointer progress_user_data)
{
	ParseData *data;
	data = g_slice_new (ParseData);
	data->entry_type = entry_type;
	data->progress_callback = progress_callback;
	data->progress_user_data = progress_user_data;
	data->entry_i = 0;

	return data;
}

void
_gdata_feed_parse_data_free (gpointer data)
{
	g_slice_free (ParseData, data);
}

static gboolean
progress_callback_idle (ProgressCallbackData *data)
{
	data->progress_callback (data->entry, data->entry_i, data->total_results, data->progress_user_data);

	return G_SOURCE_REMOVE;
}

static void
progress_callback_data_free (ProgressCallbackData *data)
{
	g_object_unref (data->entry);
	g_slice_free (ProgressCallbackData, data);
}

void
_gdata_feed_call_progress_callback (GDataFeed *self, gpointer user_data, GDataEntry *entry)
{
	ParseData *data = user_data;

	if (data->progress_callback != NULL) {
		ProgressCallbackData *progress_data;

		/* Build the data for the callback */
		progress_data = g_slice_new (ProgressCallbackData);
		progress_data->progress_callback = data->progress_callback;
		progress_data->progress_user_data = data->progress_user_data;
		progress_data->entry = g_object_ref (entry);
		progress_data->entry_i = data->entry_i;
		progress_data->total_results = MIN (self->priv->items_per_page, self->priv->total_results);

		/* Send the callback; use G_PRIORITY_DEFAULT rather than G_PRIORITY_DEFAULT_IDLE
		 * to contend with the priorities used by the callback functions in GAsyncResult */
		g_main_context_invoke_full (NULL, G_PRIORITY_DEFAULT,
		                            (GSourceFunc) progress_callback_idle,
		                            progress_data,
		                            (GDestroyNotify) progress_callback_data_free);
	}
	data->entry_i++;
}

