/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
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

/*
 * SECTION:gdata-media-group
 * @short_description: Media RSS group element
 * @stability: Stable
 * @include: gdata/media/gdata-media-group.h
 *
 * #GDataMediaGroup represents a "group" element from the
 * <ulink type="http" url="http://video.search.yahoo.com/mrss">Media RSS specification</ulink>.
 *
 * It is private API, since implementing classes are likely to proxy the properties and functions
 * of #GDataMediaGroup as appropriate; most entry types which implement #GDataMediaGroup have no use
 * for most of its properties, and it would be unnecessary and confusing to expose #GDataMediaGroup itself.
 *
 * For this reason, properties have not been implemented on #GDataMediaGroup (yet).
 */

#include <glib.h>
#include <libxml/parser.h>
#include <string.h>

#include "gdata-media-group.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-private.h"
#include "media/gdata-media-category.h"
#include "media/gdata-media-credit.h"
#include "media/gdata-media-thumbnail.h"

static void gdata_media_group_dispose (GObject *object);
static void gdata_media_group_finalize (GObject *object);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataMediaGroupPrivate {
	gchar **keywords;
	gchar *player_uri;
	GHashTable *restricted_countries;
	gchar *simple_rating;
	gchar *mpaa_rating;
	gchar *v_chip_rating;
	GList *thumbnails; /* GDataMediaThumbnail */
	gchar *title;
	GDataMediaCategory *category;
	GList *contents; /* GDataMediaContent */
	GDataMediaCredit *credit;
	gchar *description;
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataMediaGroup, gdata_media_group, GDATA_TYPE_PARSABLE)

static void
gdata_media_group_class_init (GDataMediaGroupClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->dispose = gdata_media_group_dispose;
	gobject_class->finalize = gdata_media_group_finalize;

	parsable_class->parse_xml = parse_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "group";
	parsable_class->element_namespace = "media";
}

static void
gdata_media_group_init (GDataMediaGroup *self)
{
	self->priv = gdata_media_group_get_instance_private (self);
	self->priv->restricted_countries = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

static void
gdata_media_group_dispose (GObject *object)
{
	GDataMediaGroupPrivate *priv = GDATA_MEDIA_GROUP (object)->priv;

	if (priv->category != NULL)
		g_object_unref (priv->category);
	priv->category = NULL;

	if (priv->credit != NULL)
		g_object_unref (priv->credit);
	priv->credit = NULL;

	g_list_free_full (priv->contents, g_object_unref);
	priv->contents = NULL;

	g_list_free_full (priv->thumbnails, g_object_unref);
	priv->thumbnails = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_media_group_parent_class)->dispose (object);
}

static void
gdata_media_group_finalize (GObject *object)
{
	GDataMediaGroupPrivate *priv = GDATA_MEDIA_GROUP (object)->priv;

	g_strfreev (priv->keywords);
	g_free (priv->player_uri);
	g_free (priv->v_chip_rating);
	g_free (priv->mpaa_rating);
	g_free (priv->simple_rating);
	g_hash_table_destroy (priv->restricted_countries);
	g_free (priv->title);
	g_free (priv->description);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_media_group_parent_class)->finalize (object);
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	gboolean success;
	GDataMediaGroup *self = GDATA_MEDIA_GROUP (parsable);

	if (gdata_parser_is_namespace (node, "http://search.yahoo.com/mrss/") == TRUE) {
		if (gdata_parser_string_from_element (node, "title", P_NONE, &(self->priv->title), &success, error) == TRUE ||
		    gdata_parser_string_from_element (node, "description", P_NONE, &(self->priv->description), &success, error) == TRUE ||
		    gdata_parser_object_from_element_setter (node, "category", P_REQUIRED, GDATA_TYPE_MEDIA_CATEGORY,
		                                             gdata_media_group_set_category, self, &success, error) == TRUE ||
		    gdata_parser_object_from_element_setter (node, "content", P_REQUIRED, GDATA_TYPE_MEDIA_CONTENT,
		                                             _gdata_media_group_add_content, self, &success, error) == TRUE ||
		    gdata_parser_object_from_element_setter (node, "thumbnail", P_REQUIRED, GDATA_TYPE_MEDIA_THUMBNAIL,
		                                             _gdata_media_group_add_thumbnail, self, &success, error) == TRUE ||
		    gdata_parser_object_from_element (node, "credit", P_REQUIRED | P_NO_DUPES, GDATA_TYPE_MEDIA_CREDIT,
		                                      &(self->priv->credit), &success, error) == TRUE) {
			return success;
		} else if (xmlStrcmp (node->name, (xmlChar*) "keywords") == 0) {
			/* media:keywords */
			guint i;
			xmlChar *text = xmlNodeListGetString (node->doc, node->children, TRUE);

			g_strfreev (self->priv->keywords);
			if (text == NULL) {
				self->priv->keywords = NULL;
				return TRUE;
			}

			self->priv->keywords = g_strsplit ((gchar*) text, ",", -1);
			xmlFree (text);

			for (i = 0; self->priv->keywords[i] != NULL; i++) {
				gchar *comma, *start = self->priv->keywords[i];
				gchar *end = start + strlen (start);

				/* Strip any whitespace from the ends of the keyword */
				g_strstrip (start);

				/* Unescape any %2Cs in the keyword to commas in-place */
				while ((comma = g_strstr_len (start, -1, "%2C")) != NULL) {
					/* Unescape the comma */
					*comma = ',';

					/* Move forwards, skipping the comma */
					comma++;
					end -= 2;

					/* Shift the remainder of the string downwards */
					memmove (comma, comma + 2, end - comma);
					*end = '\0';
				}
			}
		} else if (xmlStrcmp (node->name, (xmlChar*) "player") == 0) {
			/* media:player */
			xmlChar *player_uri = xmlGetProp (node, (xmlChar*) "url");
			g_free (self->priv->player_uri);
			self->priv->player_uri = (gchar*) player_uri;
		} else if (xmlStrcmp (node->name, (xmlChar*) "rating") == 0) {
			/* media:rating */
			xmlChar *scheme;

			/* The possible schemes are defined here:
			 *  • http://video.search.yahoo.com/mrss
			 *  • http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_media:rating
			 */
			scheme = xmlGetProp (node, (xmlChar*) "scheme");

			if (scheme == NULL || xmlStrcmp (scheme, (xmlChar*) "urn:simple") == 0) {
				/* Options: adult, nonadult */
				gdata_parser_string_from_element (node, "rating", P_REQUIRED | P_NON_EMPTY, &(self->priv->simple_rating),
				                                  &success, error);
			} else if (xmlStrcmp (scheme, (xmlChar*) "urn:mpaa") == 0) {
				/* Options: g, pg, pg-13, r, nc-17 */
				gdata_parser_string_from_element (node, "rating", P_REQUIRED | P_NON_EMPTY, &(self->priv->mpaa_rating),
				                                  &success, error);
			} else if (xmlStrcmp (scheme, (xmlChar*) "urn:v-chip") == 0) {
				/* Options: tv-y, tv-y7, tv-y7-fv, tv-g, tv-pg, tv-14, tv-ma */
				gdata_parser_string_from_element (node, "rating", P_REQUIRED | P_NON_EMPTY, &(self->priv->v_chip_rating),
				                                  &success, error);
			} else if (xmlStrcmp (scheme, (xmlChar*) "http://gdata.youtube.com/schemas/2007#mediarating") == 0) {
				/* No content, but we do get a list of countries. There's nothing like overloading the semantics of XML elements
				 * to brighten up one's day. */
				xmlChar *countries;

				countries = xmlGetProp (node, (xmlChar*) "country");

				if (countries != NULL) {
					gchar **country_list, **country;

					/* It's either a comma-separated list of countries, or the value "all" */
					country_list = g_strsplit ((const gchar*) countries, ",", -1);
					xmlFree (countries);

					/* Add all the listed countries to the restricted countries table */
					for (country = country_list; *country != NULL; country++) {
						g_hash_table_insert (self->priv->restricted_countries, *country, GUINT_TO_POINTER (TRUE));
					}

					g_free (country_list);
				} else {
					/* Assume it's restricted in all countries */
					g_hash_table_insert (self->priv->restricted_countries, g_strdup ("all"), GUINT_TO_POINTER (TRUE));
				}

				success = TRUE;
			} else {
				/* Error */
				gdata_parser_error_unknown_property_value (node, "scheme", (gchar*) scheme, error);
				success = FALSE;
			}

			xmlFree (scheme);

			return success;
		} else if (xmlStrcmp (node->name, (xmlChar*) "restriction") == 0) {
			/* media:restriction */
			xmlChar *type, *countries, *relationship;
			gchar **country_list, **country;
			gboolean relationship_bool;

			/* Check the type property is "country" */
			type = xmlGetProp (node, (xmlChar*) "type");
			if (xmlStrcmp (type, (xmlChar*) "country") != 0) {
				gdata_parser_error_unknown_property_value (node, "type", (gchar*) type, error);
				xmlFree (type);
				return FALSE;
			}
			xmlFree (type);

			relationship = xmlGetProp (node, (xmlChar*) "relationship");
			if (xmlStrcmp (relationship, (xmlChar*) "allow") == 0) {
				relationship_bool = FALSE; /* it's *not* a restricted country */
			} else if (xmlStrcmp (relationship, (xmlChar*) "deny") == 0) {
				relationship_bool = TRUE; /* it *is* a restricted country */
			} else {
				gdata_parser_error_unknown_property_value (node, "relationship", (gchar*) relationship, error);
				xmlFree (relationship);
				return FALSE;
			}
			xmlFree (relationship);

			countries = xmlNodeListGetString (doc, node->children, TRUE);
			country_list = g_strsplit ((const gchar*) countries, " ", -1);
			xmlFree (countries);

			/* Add "all" to the table, since it's an exception table */
			g_hash_table_insert (self->priv->restricted_countries, g_strdup ("all"), GUINT_TO_POINTER (!relationship_bool));

			/* Add all the listed countries to the restricted countries table */
			for (country = country_list; *country != NULL; country++)
				g_hash_table_insert (self->priv->restricted_countries, *country, GUINT_TO_POINTER (relationship_bool));
			g_free (country_list);
		} else {
			return GDATA_PARSABLE_CLASS (gdata_media_group_parent_class)->parse_xml (parsable, doc, node, user_data, error);
		}
	} else {
		return GDATA_PARSABLE_CLASS (gdata_media_group_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}

	return TRUE;
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataMediaGroupPrivate *priv = GDATA_MEDIA_GROUP (parsable)->priv;

	/* Media category */
	if (priv->category != NULL)
		_gdata_parsable_get_xml (GDATA_PARSABLE (priv->category), xml_string, FALSE);

	if (priv->title != NULL)
		gdata_parser_string_append_escaped (xml_string, "<media:title type='plain'>", priv->title, "</media:title>");

	if (priv->description != NULL)
		gdata_parser_string_append_escaped (xml_string, "<media:description type='plain'>", priv->description, "</media:description>");

	if (priv->keywords != NULL) {
		guint i;

		g_string_append (xml_string, "<media:keywords>");

		/* Add each keyword to the text content, comma-separated from the previous one */
		for (i = 0; priv->keywords[i] != NULL; i++) {
			const gchar *comma, *start = priv->keywords[i];

			/* Delimit the previous keyword */
			if (i != 0)
				g_string_append_c (xml_string, ',');

			/* Escape any commas in the keyword to %2C */
			while ((comma = g_utf8_strchr (start, -1, ',')) != NULL) {
				/* Copy the span */
				gchar *span = g_strndup (start, comma - start);
				gdata_parser_string_append_escaped (xml_string, NULL, span, NULL);
				g_free (span);

				/* Add an escaped comma */
				g_string_append (xml_string, "%2C");

				/* Move forwards, skipping the comma */
				start = comma + 1;
			}

			/* Append the rest of the string (the entire string if there were no commas) */
			gdata_parser_string_append_escaped (xml_string, NULL, start, NULL);
		}

		g_string_append (xml_string, "</media:keywords>");
	}
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "media", (gchar*) "http://search.yahoo.com/mrss/");
}

/**
 * gdata_media_group_get_title:
 * @self: a #GDataMediaGroup
 *
 * Gets the #GDataMediaGroup:title property.
 *
 * Return value: the group's title, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_media_group_get_title (GDataMediaGroup *self)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_GROUP (self), NULL);
	return self->priv->title;
}

/**
 * gdata_media_group_set_title:
 * @self: a #GDataMediaGroup
 * @title: (allow-none): the group's new title, or %NULL
 *
 * Sets the #GDataMediaGroup:title property to @title.
 *
 * Set @title to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_media_group_set_title (GDataMediaGroup *self, const gchar *title)
{
	g_return_if_fail (GDATA_IS_MEDIA_GROUP (self));
	g_free (self->priv->title);
	self->priv->title = g_strdup (title);
}

/**
 * gdata_media_group_get_description:
 * @self: a #GDataMediaGroup
 *
 * Gets the #GDataMediaGroup:description property.
 *
 * Return value: the group's description, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_media_group_get_description (GDataMediaGroup *self)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_GROUP (self), NULL);
	return self->priv->description;
}

/**
 * gdata_media_group_set_description:
 * @self: a #GDataMediaGroup
 * @description: (allow-none): the group's new description, or %NULL
 *
 * Sets the #GDataMediaGroup:description property to @description.
 *
 * Set @description to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_media_group_set_description (GDataMediaGroup *self, const gchar *description)
{
	g_return_if_fail (GDATA_IS_MEDIA_GROUP (self));
	g_free (self->priv->description);
	self->priv->description = g_strdup (description);
}

/**
 * gdata_media_group_get_keywords:
 * @self: a #GDataMediaGroup
 *
 * Gets the #GDataMediaGroup:keywords property.
 *
 * Return value: (array zero-terminated=1): a %NULL-terminated array of the group's keywords, or %NULL
 *
 * Since: 0.4.0
 */
const gchar * const *
gdata_media_group_get_keywords (GDataMediaGroup *self)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_GROUP (self), NULL);
	return (const gchar * const *) self->priv->keywords;
}

/**
 * gdata_media_group_set_keywords:
 * @self: a #GDataMediaGroup
 * @keywords: (array zero-terminated=1) (allow-none): a %NULL-terminated array of the group's new keywords, or %NULL
 *
 * Sets the #GDataMediaGroup:keywords property to @keywords.
 *
 * Set @keywords to %NULL to unset the property.
 *
 * Since: 0.4.0
 */
void
gdata_media_group_set_keywords (GDataMediaGroup *self, const gchar * const *keywords)
{
	g_return_if_fail (GDATA_IS_MEDIA_GROUP (self));
	g_strfreev (self->priv->keywords);
	self->priv->keywords = g_strdupv ((gchar**) keywords);
}

/**
 * gdata_media_group_get_category:
 * @self: a #GDataMediaGroup
 *
 * Gets the #GDataMediaGroup:category property.
 *
 * Return value: a #GDataMediaCategory giving the group's category, or %NULL
 */
GDataMediaCategory *
gdata_media_group_get_category (GDataMediaGroup *self)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_GROUP (self), NULL);
	return self->priv->category;
}

/**
 * gdata_media_group_set_category:
 * @self: a #GDataMediaGroup
 * @category: (allow-none): a new #GDataMediaCategory, or %NULL
 *
 * Sets the #GDataMediaGroup:category property to @category, and increments its reference count.
 */
void
gdata_media_group_set_category (GDataMediaGroup *self, GDataMediaCategory *category)
{
	g_return_if_fail (GDATA_IS_MEDIA_GROUP (self));
	g_return_if_fail (category == NULL || GDATA_IS_MEDIA_CATEGORY (category));

	if (self->priv->category != NULL)
		g_object_unref (self->priv->category);
	self->priv->category = (category == NULL) ? NULL : g_object_ref (category);
}

static gint
content_compare_cb (const GDataMediaContent *content, const gchar *type)
{
	return strcmp (gdata_media_content_get_content_type ((GDataMediaContent*) content), type);
}

/**
 * gdata_media_group_look_up_content:
 * @self: a #GDataMediaGroup
 * @type: the MIME type of the content desired
 *
 * Looks up a #GDataMediaContent from the group with the given MIME type. The group's list of contents is
 * a list of URIs to various formats of the group content itself, such as the SWF URI or RTSP stream for a video.
 *
 * Return value: (transfer none): a #GDataMediaContent matching @type, or %NULL
 */
GDataMediaContent *
gdata_media_group_look_up_content (GDataMediaGroup *self, const gchar *type)
{
	GList *element;

	g_return_val_if_fail (GDATA_IS_MEDIA_GROUP (self), NULL);
	g_return_val_if_fail (type != NULL, NULL);

	/* TODO: If type is required, and is unique, the contents can be stored in a hash table rather than a linked list */
	element = g_list_find_custom (self->priv->contents, type, (GCompareFunc) content_compare_cb);
	if (element == NULL)
		return NULL;
	return GDATA_MEDIA_CONTENT (element->data);
}

/**
 * gdata_media_group_get_contents:
 * @self: a #GDataMediaGroup
 *
 * Returns a list of #GDataMediaContents, giving the content enclosed by the group.
 *
 * Return value: (element-type GData.MediaContent) (transfer none): a #GList of #GDataMediaContents,  or %NULL
 */
GList *
gdata_media_group_get_contents (GDataMediaGroup *self)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_GROUP (self), NULL);
	return self->priv->contents;
}

void
_gdata_media_group_add_content (GDataMediaGroup *self, GDataMediaContent *content)
{
	g_return_if_fail (GDATA_IS_MEDIA_GROUP (self));
	g_return_if_fail (GDATA_IS_MEDIA_CONTENT (content));
	self->priv->contents = g_list_prepend (self->priv->contents, g_object_ref (content));
}

/**
 * gdata_media_group_get_credit:
 * @self: a #GDataMediaGroup
 *
 * Gets the #GDataMediaGroup:credit property.
 *
 * Return value: a #GDataMediaCredit giving information on whom to credit for the media group, or %NULL
 */
GDataMediaCredit *
gdata_media_group_get_credit (GDataMediaGroup *self)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_GROUP (self), NULL);
	return self->priv->credit;
}

void
_gdata_media_group_set_credit (GDataMediaGroup *self, GDataMediaCredit *credit)
{
	g_return_if_fail (GDATA_IS_MEDIA_GROUP (self));
	g_return_if_fail (credit == NULL ||GDATA_IS_MEDIA_CREDIT (credit));

	if (self->priv->credit != NULL)
		g_object_unref (self->priv->credit);
	self->priv->credit = g_object_ref (credit);
}

/**
 * gdata_media_group_get_media_group:
 * @self: a #GDataMediaGroup
 *
 * Gets the #GDataMediaGroup:player-uri property.
 *
 * Return value: a URI where the media group is playable in a web browser, or %NULL
 */
const gchar *
gdata_media_group_get_player_uri (GDataMediaGroup *self)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_GROUP (self), NULL);
	return self->priv->player_uri;
}

/**
 * gdata_media_group_is_restricted_in_country:
 * @self: a #GDataMediaGroup
 * @country: an ISO 3166 two-letter country code to check
 *
 * Checks whether viewing of the media is restricted in @country, either by its content rating, or by the request of the producer.
 * The return value from this function is purely informational, and no obligation is assumed.
 *
 * Return value: %TRUE if the media is restricted in @country, %FALSE otherwise
 */
gboolean
gdata_media_group_is_restricted_in_country (GDataMediaGroup *self, const gchar *country)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_GROUP (self), FALSE);
	g_return_val_if_fail (country != NULL && *country != '\0', FALSE);

	if (GPOINTER_TO_UINT (g_hash_table_lookup (self->priv->restricted_countries, country)) == TRUE)
		return TRUE;

	return GPOINTER_TO_UINT (g_hash_table_lookup (self->priv->restricted_countries, "all"));
}

/**
 * gdata_media_group_get_media_rating:
 * @self: a #GDataMediaGroup
 * @rating_type: the type of rating to retrieve
 *
 * Returns the rating of the given type for the media, if one exists. For example, this could be a film rating awarded by the MPAA.
 * The valid values for @rating_type are: <code class="literal">simple</code>, <code class="literal">mpaa</code> and
 * <code class="literal">v-chip</code>.
 *
 * The rating values returned for each of these rating types are string as defined in the
 * <ulink type="http" url="http://code.google.com/apis/youtube/2.0/reference.html#youtube_data_api_tag_media:rating">YouTube documentation</ulink> and
 * <ulink type="http" url="http://video.search.yahoo.com/mrss">MRSS specification</ulink>.
 *
 * Return value: rating for the given rating type, or %NULL if the media has no rating for that type (or the type is invalid)
 */
const gchar *
gdata_media_group_get_media_rating (GDataMediaGroup *self, const gchar *rating_type)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_GROUP (self), NULL);
	g_return_val_if_fail (rating_type != NULL && *rating_type != '\0', NULL);

	if (strcmp (rating_type, "simple") == 0) {
		return self->priv->simple_rating;
	} else if (strcmp (rating_type, "mpaa") == 0) {
		return self->priv->mpaa_rating;
	} else if (strcmp (rating_type, "v-chip") == 0) {
		return self->priv->v_chip_rating;
	}

	return NULL;
}

/**
 * gdata_media_group_get_thumbnails:
 * @self: a #GDataMediaGroup
 *
 * Gets a list of the thumbnails available for the group.
 *
 * Return value: (element-type GData.MediaThumbnail) (transfer none): a #GList of #GDataMediaThumbnails, or %NULL
 */
GList *
gdata_media_group_get_thumbnails (GDataMediaGroup *self)
{
	g_return_val_if_fail (GDATA_IS_MEDIA_GROUP (self), NULL);
	return self->priv->thumbnails;
}

void
_gdata_media_group_add_thumbnail (GDataMediaGroup *self, GDataMediaThumbnail *thumbnail)
{
	g_return_if_fail (GDATA_IS_MEDIA_GROUP (self));
	g_return_if_fail (GDATA_IS_MEDIA_THUMBNAIL (thumbnail));
	self->priv->thumbnails = g_list_prepend (self->priv->thumbnails, g_object_ref (thumbnail));
}
