/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009, 2010, 2014, 2015 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-calendar-feed
 * @short_description: GData Calendar feed object
 * @stability: Stable
 * @include: gdata/services/calendar/gdata-calendar-feed.h
 *
 * #GDataCalendarFeed is a subclass of #GDataFeed to represent a results feed from Google Calendar. It adds a couple of
 * properties which are specific to the Google Calendar API.
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-calendar-feed.h"
#include "gdata-feed.h"
#include "gdata-private.h"

static void gdata_calendar_feed_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);

enum {
	PROP_TIMEZONE = 1,
	PROP_TIMES_CLEANED
};

G_DEFINE_TYPE (GDataCalendarFeed, gdata_calendar_feed, GDATA_TYPE_FEED)

static void
gdata_calendar_feed_class_init (GDataCalendarFeedClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->get_property = gdata_calendar_feed_get_property;

	/**
	 * GDataCalendarFeed:timezone:
	 *
	 * The timezone in which the feed's times are given. This is a timezone name in tz database notation: <ulink type="http"
	 * url="http://en.wikipedia.org/wiki/Tz_database#Names_of_time_zones">reference</ulink>.
	 *
	 * Since: 0.3.0
	 * Deprecated: 0.17.2: Unsupported by the online API any more. There
	 *   is no replacement; this will always return %NULL.
	 */
	g_object_class_install_property (gobject_class, PROP_TIMEZONE,
	                                 g_param_spec_string ("timezone",
	                                                      "Timezone", "The timezone in which the feed's times are given.",
	                                                      NULL,
	                                                      G_PARAM_DEPRECATED |
	                                                      G_PARAM_READABLE |
	                                                      G_PARAM_STATIC_STRINGS));

	/**
	 * GDataCalendarFeed:times-cleaned:
	 *
	 * The number of times the feed has been completely cleared of entries.
	 *
	 * Since: 0.3.0
	 * Deprecated: 0.17.2: Unsupported by the online API any more. There
	 *   is no replacement; this will always return 0.
	 */
	g_object_class_install_property (gobject_class, PROP_TIMES_CLEANED,
	                                 g_param_spec_uint ("times-cleaned",
	                                                    "Times cleaned", "The number of times the feed has been completely cleared of entries.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_DEPRECATED |
	                                                    G_PARAM_READABLE |
	                                                    G_PARAM_STATIC_STRINGS));
}

static void
gdata_calendar_feed_init (GDataCalendarFeed *self)
{
	/* Nothing to see here. */
}

static void
gdata_calendar_feed_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataCalendarFeed *self = GDATA_CALENDAR_FEED (object);

	switch (property_id) {
		case PROP_TIMEZONE:
			G_GNUC_BEGIN_IGNORE_DEPRECATIONS
			g_value_set_string (value,
			                    gdata_calendar_feed_get_timezone (self));
			G_GNUC_END_IGNORE_DEPRECATIONS
			break;
		case PROP_TIMES_CLEANED:
			G_GNUC_BEGIN_IGNORE_DEPRECATIONS
			g_value_set_uint (value,
			                  gdata_calendar_feed_get_times_cleaned (self));
			G_GNUC_END_IGNORE_DEPRECATIONS
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

/**
 * gdata_calendar_feed_get_timezone:
 * @self: a #GDataCalendarFeed
 *
 * Gets the #GDataCalendarFeed:timezone property.
 *
 * Return value: the feed's timezone, or %NULL
 *
 * Since: 0.3.0
 * Deprecated: 0.17.2: Unsupported by the online API any more. There is no
 *   replacement; this will always return %NULL.
 */
const gchar *
gdata_calendar_feed_get_timezone (GDataCalendarFeed *self)
{
	/* Not supported any more by version 3 of the API. */
	g_return_val_if_fail (GDATA_IS_CALENDAR_FEED (self), NULL);
	return NULL;
}

/**
 * gdata_calendar_feed_get_times_cleaned:
 * @self: a #GDataCalendarFeed
 *
 * Gets the #GDataCalendarFeed:times-cleaned property.
 *
 * Return value: the number of times the feed has been totally emptied
 *
 * Since: 0.3.0
 * Deprecated: 0.17.2: Unsupported by the online API any more. There is no
 *   replacement; this will always return %NULL.
 */
guint
gdata_calendar_feed_get_times_cleaned (GDataCalendarFeed *self)
{
	/* Not supported any more by version 3 of the API. */
	g_return_val_if_fail (GDATA_IS_CALENDAR_FEED (self), 0);
	return 0;
}
