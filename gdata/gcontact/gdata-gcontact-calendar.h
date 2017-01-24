/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2010 <philip@tecnocode.co.uk>
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

#ifndef GDATA_GCONTACT_CALENDAR_H
#define GDATA_GCONTACT_CALENDAR_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

/**
 * GDATA_GCONTACT_CALENDAR_WORK:
 *
 * The relation type URI for a contact's work calendar.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcCalendarLink">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_CALENDAR_WORK "work"

/**
 * GDATA_GCONTACT_CALENDAR_HOME:
 *
 * The relation type URI for a contact's home calendar.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcCalendarLink">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_CALENDAR_HOME "home"

/**
 * GDATA_GCONTACT_CALENDAR_FREE_BUSY:
 *
 * The relation type URI for a contact's free/busy calendar.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcCalendarLink">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_CALENDAR_FREE_BUSY "free-busy"

#define GDATA_TYPE_GCONTACT_CALENDAR		(gdata_gcontact_calendar_get_type ())
#define GDATA_GCONTACT_CALENDAR(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_GCONTACT_CALENDAR, GDataGContactCalendar))
#define GDATA_GCONTACT_CALENDAR_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_GCONTACT_CALENDAR, GDataGContactCalendarClass))
#define GDATA_IS_GCONTACT_CALENDAR(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_GCONTACT_CALENDAR))
#define GDATA_IS_GCONTACT_CALENDAR_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_GCONTACT_CALENDAR))
#define GDATA_GCONTACT_CALENDAR_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_GCONTACT_CALENDAR, GDataGContactCalendarClass))

typedef struct _GDataGContactCalendarPrivate	GDataGContactCalendarPrivate;

/**
 * GDataGContactCalendar:
 *
 * All the fields in the #GDataGContactCalendar structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	GDataParsable parent;
	GDataGContactCalendarPrivate *priv;
} GDataGContactCalendar;

/**
 * GDataGContactCalendarClass:
 *
 * All the fields in the #GDataGContactCalendarClass structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	/*< private >*/
	GDataParsableClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataGContactCalendarClass;

GType gdata_gcontact_calendar_get_type (void) G_GNUC_CONST;

GDataGContactCalendar *gdata_gcontact_calendar_new (const gchar *uri, const gchar *relation_type,
                                                    const gchar *label, gboolean is_primary) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_gcontact_calendar_get_uri (GDataGContactCalendar *self) G_GNUC_PURE;
void gdata_gcontact_calendar_set_uri (GDataGContactCalendar *self, const gchar *uri);

const gchar *gdata_gcontact_calendar_get_relation_type (GDataGContactCalendar *self) G_GNUC_PURE;
void gdata_gcontact_calendar_set_relation_type (GDataGContactCalendar *self, const gchar *relation_type);

const gchar *gdata_gcontact_calendar_get_label (GDataGContactCalendar *self) G_GNUC_PURE;
void gdata_gcontact_calendar_set_label (GDataGContactCalendar *self, const gchar *label);

gboolean gdata_gcontact_calendar_is_primary (GDataGContactCalendar *self) G_GNUC_PURE;
void gdata_gcontact_calendar_set_is_primary (GDataGContactCalendar *self, gboolean is_primary);

G_END_DECLS

#endif /* !GDATA_GCONTACT_CALENDAR_H */
