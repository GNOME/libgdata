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

G_DEFINE_TYPE (GDataCalendarFeed, gdata_calendar_feed, GDATA_TYPE_FEED)

static void
gdata_calendar_feed_class_init (GDataCalendarFeedClass *klass)
{
}

static void
gdata_calendar_feed_init (GDataCalendarFeed *self)
{
	/* Nothing to see here. */
}
