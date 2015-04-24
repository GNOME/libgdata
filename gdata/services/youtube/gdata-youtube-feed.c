/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2015 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-youtube-feed
 * @short_description: GData YouTube feed object
 * @stability: Stable
 * @include: gdata/services/youtube/gdata-youtube-feed.h
 *
 * #GDataYouTubeFeed is a list of entries (#GDataYouTubeVideo subclasses)
 * returned as the result of a query to a #GDataYouTubeService, or given as the
 * input to another operation on the online service.
 *
 * Each #GDataYouTubeVideo represents a single video on YouTube, and the
 * #GDataYouTubeFeed represents a collection of those objects.
 *
 * Since: 0.17.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-youtube-feed.h"
#include "gdata-youtube-video.h"
#include "gdata-types.h"
#include "gdata-private.h"
#include "gdata-service.h"

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data,
            GError **error);

G_DEFINE_TYPE (GDataYouTubeFeed, gdata_youtube_feed, GDATA_TYPE_FEED)

static void
gdata_youtube_feed_class_init (GDataYouTubeFeedClass *klass)
{
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	parsable_class->parse_json = parse_json;
}

static void
gdata_youtube_feed_init (GDataYouTubeFeed *self)
{
	/* Move along now. */
}

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data,
            GError **error)
{
	if (g_strcmp0 (json_reader_get_member_name (reader), "pageInfo") == 0) {
		guint total_results, items_per_page;
		const GError *child_error = NULL;

		/* Check this is an object. */
		if (!json_reader_is_object (reader)) {
			return gdata_parser_error_required_json_content_missing (reader, error);
		}

		/* https://developers.google.com/youtube/v3/docs/playlists/list/ */
		json_reader_read_member (reader, "totalResults");
		total_results = json_reader_get_int_value (reader);
		json_reader_end_member (reader);

		json_reader_read_member (reader, "resultsPerPage");
		items_per_page = json_reader_get_int_value (reader);
		json_reader_end_member (reader);

		child_error = json_reader_get_error (reader);
		if (child_error != NULL) {
			return gdata_parser_error_from_json_error (reader,
			                                           child_error,
			                                           error);
		}

		_gdata_feed_set_page_info (GDATA_FEED (parsable), total_results,
		                           items_per_page);

		return TRUE;
	} else {
		return GDATA_PARSABLE_CLASS (gdata_youtube_feed_parent_class)->parse_json (parsable, reader, user_data, error);
	}
}
