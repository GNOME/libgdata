/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009 <philip@tecnocode.co.uk>
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

#ifndef GDATA_CALENDAR_CALENDAR_H
#define GDATA_CALENDAR_CALENDAR_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-entry.h>
#include <gdata/gdata-types.h>

G_BEGIN_DECLS

/**
 * GDATA_CALENDAR_ACCESS_ROLE_READ:
 *
 * The users specified by the #GDataAccessRule have read-only access to the calendar.
 *
 * Since: 0.7.0
 **/
#define GDATA_CALENDAR_ACCESS_ROLE_READ "http://schemas.google.com/gCal/2005#read"

/**
 * GDATA_CALENDAR_ACCESS_ROLE_FREE_BUSY:
 *
 * The users specified by the #GDataAccessRule can only see the free/busy information on the calendar; not event details.
 *
 * Since: 0.7.0
 **/
#define GDATA_CALENDAR_ACCESS_ROLE_FREE_BUSY "http://schemas.google.com/gCal/2005#freebusy"

/**
 * GDATA_CALENDAR_ACCESS_ROLE_EDITOR:
 *
 * The users specified by the #GDataAccessRule have full edit access to the calendar, except they can't change the calendar's access rules.
 *
 * Since: 0.7.0
 **/
#define GDATA_CALENDAR_ACCESS_ROLE_EDITOR "http://schemas.google.com/gCal/2005#editor"

/**
 * GDATA_CALENDAR_ACCESS_ROLE_OWNER:
 *
 * The users specified by the #GDataAccessRule have full owner access to the calendar.
 *
 * Since: 0.7.0
 **/
#define GDATA_CALENDAR_ACCESS_ROLE_OWNER "http://schemas.google.com/gCal/2005#owner"

/**
 * GDATA_CALENDAR_ACCESS_ROLE_ROOT:
 *
 * The users specified by the #GDataAccessRule have full administrator access to the calendar server.
 * This is only available in Google Apps For Your Domain.
 *
 * Since: 0.7.0
 **/
#define GDATA_CALENDAR_ACCESS_ROLE_ROOT "http://schemas.google.com/gCal/2005#root"

#define GDATA_TYPE_CALENDAR_CALENDAR		(gdata_calendar_calendar_get_type ())
#define GDATA_CALENDAR_CALENDAR(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_CALENDAR_CALENDAR, GDataCalendarCalendar))
#define GDATA_CALENDAR_CALENDAR_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_CALENDAR_CALENDAR, GDataCalendarCalendarClass))
#define GDATA_IS_CALENDAR_CALENDAR(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_CALENDAR_CALENDAR))
#define GDATA_IS_CALENDAR_CALENDAR_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_CALENDAR_CALENDAR))
#define GDATA_CALENDAR_CALENDAR_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_CALENDAR_CALENDAR, GDataCalendarCalendarClass))

typedef struct _GDataCalendarCalendarPrivate	GDataCalendarCalendarPrivate;

/**
 * GDataCalendarCalendar:
 *
 * All the fields in the #GDataCalendarCalendar structure are private and should never be accessed directly.
 **/
typedef struct {
	GDataEntry parent;
	GDataCalendarCalendarPrivate *priv;
} GDataCalendarCalendar;

/**
 * GDataCalendarCalendarClass:
 *
 * All the fields in the #GDataCalendarCalendarClass structure are private and should never be accessed directly.
 **/
typedef struct {
	/*< private >*/
	GDataEntryClass parent;
} GDataCalendarCalendarClass;

GType gdata_calendar_calendar_get_type (void) G_GNUC_CONST;

GDataCalendarCalendar *gdata_calendar_calendar_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_calendar_calendar_get_timezone (GDataCalendarCalendar *self) G_GNUC_PURE;
void gdata_calendar_calendar_set_timezone (GDataCalendarCalendar *self, const gchar *_timezone);
guint gdata_calendar_calendar_get_times_cleaned (GDataCalendarCalendar *self) G_GNUC_PURE;
gboolean gdata_calendar_calendar_is_hidden (GDataCalendarCalendar *self) G_GNUC_PURE;
void gdata_calendar_calendar_set_is_hidden (GDataCalendarCalendar *self, gboolean is_hidden);
void gdata_calendar_calendar_get_color (GDataCalendarCalendar *self, GDataColor *color);
void gdata_calendar_calendar_set_color (GDataCalendarCalendar *self, const GDataColor *color);
gboolean gdata_calendar_calendar_is_selected (GDataCalendarCalendar *self) G_GNUC_PURE;
void gdata_calendar_calendar_set_is_selected (GDataCalendarCalendar *self, gboolean is_selected);
const gchar *gdata_calendar_calendar_get_access_level (GDataCalendarCalendar *self) G_GNUC_PURE;
gint64 gdata_calendar_calendar_get_edited (GDataCalendarCalendar *self);

G_END_DECLS

#endif /* !GDATA_CALENDAR_CALENDAR_H */
