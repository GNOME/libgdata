/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009 <philip@tecnocode.co.uk>
 * Copyright (C) Richard Schwarting 2009–2010 <aquarichy@gmail.com>
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
 * SECTION:gdata-picasaweb-feed
 * @short_description: GData PicasaWeb Feed object
 * @stability: Unstable
 * @include: gdata/services/picasaweb/gdata-picasaweb-feed.h
 *
 * #GDataPicasaWebFeed is a subclass of #GDataFeed to represent properties for a PicasaWeb feed. It adds a couple of
 * properties which are specific to the Google PicasaWeb API.
 *
 * Since: 0.6.0
 **/

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-picasaweb-feed.h"
#include "gdata-feed.h"
#include "gdata-private.h"

static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);

G_DEFINE_TYPE (GDataPicasaWebFeed, gdata_picasaweb_feed, GDATA_TYPE_FEED)

static void
gdata_picasaweb_feed_class_init (GDataPicasaWebFeedClass *klass)
{
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	parsable_class->parse_xml = parse_xml;

}

static void
gdata_picasaweb_feed_init (GDataPicasaWebFeed *self)
{
	/* Nothing to see here */
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	if (xmlStrcmp (node->name, (xmlChar*) "user") == 0) {
		/* gphoto:user */
		/* From user's feed of album entries.  Redundant with user entry represented by GDataPicasaWebUser.  Capturing and ignoring.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "nickname") == 0) {
		/* gphoto:nickname */
		/* From user's feed of album entries.  Redundant with user entry represented by GDataPicasaWebUser.  Capturing and ignoring.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "quotacurrent") == 0) {
		/* gphoto:quota-current */
		/* From user's feed of album entries.  Redundant with user entry represented by GDataPicasaWebUser.  Capturing and ignoring.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "quotalimit") == 0) {
		/* gphoto:quota-limit */
		/* From user's feed of album entries.  Redundant with user entry represented by GDataPicasaWebUser.  Capturing and ignoring.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "maxPhotosPerAlbum") == 0) {
		/* gphoto:max-photos-per-album */
		/* From user's feed of album entries.  Redundant with user entry represented by GDataPicasaWebUser.  Capturing and ignoring.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "thumbnail") == 0) {
		/* gphoto:thumbnail */
		/* From user's feed of album entries.  Redundant with user entry represented by GDataPicasaWebUser.  Capturing and ignoring.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "allowDownloads") == 0) {
		/* gphoto:allowDownloads */
		/* Not part of public API so we're capturing and ignoring for now.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "allowPrints") == 0) {
		/* gphoto:allowPrints */
		/* Not part of public API so we're capturing and ignoring for now.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "id") == 0 && xmlStrcmp (node->ns->href, (xmlChar*) "http://schemas.google.com/photos/2007") == 0) {
		/* gphoto:id */
		/* From album's feed of file entries.  Redundant with album entries represented by GDataPicasaWebAlbum.  Capturing and ignoring.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "rights") == 0) {
		/* gphoto:rights */
		/* From album's feed of file entries.  Redundant with album entries represented by GDataPicasaWebAlbum.  Capturing and ignoring.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "location") == 0) {
		/* gphoto:location */
		/* From album's feed of file entries.  Redundant with album entries represented by GDataPicasaWebAlbum.  Capturing and ignoring.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "access") == 0) {
		/* gphoto:access */
		/* From album's feed of file entries.  Redundant with album entries represented by GDataPicasaWebAlbum.  Capturing and ignoring.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "timestamp") == 0) {
		/* gphoto:timestamp */
		/* From album's feed of file entries.  Redundant with album entries represented by GDataPicasaWebAlbum.  Capturing and ignoring.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "numphotos") == 0) {
		/* gphoto:numphotos */
		/* From album's feed of file entries.  Redundant with album entries represented by GDataPicasaWebAlbum.  Capturing and ignoring.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "numphotosremaining") == 0) {
		/* gphoto:numphotosremaining */
		/* From album's feed of file entries.  Redundant with album entries represented by GDataPicasaWebAlbum.  Capturing and ignoring.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "bytesUsed") == 0) {
		/* gphoto:bytesUsed */
		/* From album's feed of file entries.  Redundant with album entries represented by GDataPicasaWebAlbum.  Capturing and ignoring.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "where") == 0) {
		/* georss:where */
		/* From album's feed of file entries.  Redundant with album entries represented by GDataPicasaWebAlbum.  Capturing and ignoring.  See bgo #589858. */
	} else if (GDATA_PARSABLE_CLASS (gdata_picasaweb_feed_parent_class)->parse_xml (parsable, doc, node, user_data, error) == FALSE) {
		/* Error! */
		return FALSE;
	}

	return TRUE;
}

