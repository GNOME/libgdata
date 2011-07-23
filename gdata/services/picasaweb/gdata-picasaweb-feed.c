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
#include <gxml.h>

#include "gdata-picasaweb-feed.h"
#include "gdata-feed.h"
#include "gdata-private.h"

static gboolean parse_xml (GDataParsable *parsable, GXmlDomDocument *doc, GXmlDomXNode *node, gpointer user_data, GError **error);

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
parse_xml (GDataParsable *parsable, GXmlDomDocument *doc, GXmlDomXNode *node, gpointer user_data, GError **error)
{
	const gchar *node_name = gxml_dom_xnode_get_node_name (node);

	if (gdata_parser_is_namespace (node, "http://schemas.google.com/photos/2007") == TRUE) {
		if (g_strcmp0 (node_name, "user") == 0 ||
		    g_strcmp0 (node_name, "nickname") == 0 ||
		    g_strcmp0 (node_name, "quotacurrent") == 0 ||
		    g_strcmp0 (node_name, "quotalimit") == 0 ||
		    g_strcmp0 (node_name, "maxPhotosPerAlbum") == 0 ||
		    g_strcmp0 (node_name, "thumbnail") == 0 ||
		    g_strcmp0 (node_name, "allowDownloads") == 0 ||
		    g_strcmp0 (node_name, "allowPrints") == 0 ||
		    g_strcmp0 (node_name, "id") == 0 ||
		    g_strcmp0 (node_name, "rights") == 0 ||
		    g_strcmp0 (node_name, "location") == 0 ||
		    g_strcmp0 (node_name, "access") == 0 ||
		    g_strcmp0 (node_name, "timestamp") == 0 ||
		    g_strcmp0 (node_name, "numphotos") == 0 ||
		    g_strcmp0 (node_name, "numphotosremaining") == 0 ||
		    g_strcmp0 (node_name, "bytesUsed") == 0) {
			/* From user's feed of album entries. Redundant with user entry represented by #GDataPicasaWebUser.
			 * Capturing and ignoring. See bgo #589858. */
		} else {
			return GDATA_PARSABLE_CLASS (gdata_picasaweb_feed_parent_class)->parse_xml (parsable, doc, node, user_data, error);
		}
	} else if (gdata_parser_is_namespace (node, "http://www.georss.org/georss") == TRUE && g_strcmp0 (node_name, "where") == 0) {
		/* From user's feed of album entries. Redundant with user entry represented by #GDataPicasaWebUser.
		 * Capturing and ignoring. See bgo #589858. */
	} else {
		return GDATA_PARSABLE_CLASS (gdata_picasaweb_feed_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}

	return TRUE;
}

