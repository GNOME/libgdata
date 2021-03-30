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

#ifndef GDATA_CALENDAR_QUERY_H
#define GDATA_CALENDAR_QUERY_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-query.h>
#include <gdata/gdata-types.h>

G_BEGIN_DECLS

#define GDATA_TYPE_CALENDAR_QUERY		(gdata_calendar_query_get_type ())
#define GDATA_CALENDAR_QUERY(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_CALENDAR_QUERY, GDataCalendarQuery))
#define GDATA_CALENDAR_QUERY_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_CALENDAR_QUERY, GDataCalendarQueryClass))
#define GDATA_IS_CALENDAR_QUERY(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_CALENDAR_QUERY))
#define GDATA_IS_CALENDAR_QUERY_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_CALENDAR_QUERY))
#define GDATA_CALENDAR_QUERY_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_CALENDAR_QUERY, GDataCalendarQueryClass))

typedef struct _GDataCalendarQueryPrivate	GDataCalendarQueryPrivate;

/**
 * GDataCalendarQuery:
 *
 * All the fields in the #GDataCalendarQuery structure are private and should never be accessed directly.
 */
typedef struct {
	GDataQuery parent;
	GDataCalendarQueryPrivate *priv;
} GDataCalendarQuery;

/**
 * GDataCalendarQueryClass:
 *
 * All the fields in the #GDataCalendarQueryClass structure are private and should never be accessed directly.
 */
typedef struct {
	/*< private >*/
	GDataQueryClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataCalendarQueryClass;

GType gdata_calendar_query_get_type (void) G_GNUC_CONST;

GDataCalendarQuery *gdata_calendar_query_new (const gchar *q) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
GDataCalendarQuery *gdata_calendar_query_new_with_limits (const gchar *q, gint64 start_min, gint64 start_max) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

gboolean gdata_calendar_query_get_future_events (GDataCalendarQuery *self) G_GNUC_PURE;
void gdata_calendar_query_set_future_events (GDataCalendarQuery *self, gboolean future_events);
const gchar *gdata_calendar_query_get_order_by (GDataCalendarQuery *self) G_GNUC_PURE;
void gdata_calendar_query_set_order_by (GDataCalendarQuery *self, const gchar *order_by);
gboolean gdata_calendar_query_get_single_events (GDataCalendarQuery *self) G_GNUC_PURE;
void gdata_calendar_query_set_single_events (GDataCalendarQuery *self, gboolean single_events);
gint64 gdata_calendar_query_get_start_min (GDataCalendarQuery *self);
void gdata_calendar_query_set_start_min (GDataCalendarQuery *self, gint64 start_min);
gint64 gdata_calendar_query_get_start_max (GDataCalendarQuery *self);
void gdata_calendar_query_set_start_max (GDataCalendarQuery *self, gint64 start_max);
const gchar *gdata_calendar_query_get_timezone (GDataCalendarQuery *self) G_GNUC_PURE;
void gdata_calendar_query_set_timezone (GDataCalendarQuery *self, const gchar *_timezone);
guint gdata_calendar_query_get_max_attendees (GDataCalendarQuery *self) G_GNUC_PURE;
void gdata_calendar_query_set_max_attendees (GDataCalendarQuery *self, guint max_attendees);
gboolean gdata_calendar_query_show_deleted (GDataCalendarQuery *self) G_GNUC_PURE;
void gdata_calendar_query_set_show_deleted (GDataCalendarQuery *self, gboolean show_deleted);

G_END_DECLS

#endif /* !GDATA_CALENDAR_QUERY_H */
