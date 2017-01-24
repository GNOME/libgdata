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

#ifndef GDATA_GD_WHEN_H
#define GDATA_GD_WHEN_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>
#include <gdata/gd/gdata-gd-reminder.h>

G_BEGIN_DECLS

/**
 * GDATA_GD_EVENT_STATUS_CANCELED:
 *
 * The event has been canceled.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_EVENT_STATUS_CANCELED "http://schemas.google.com/g/2005#event.canceled"

/**
 * GDATA_GD_EVENT_STATUS_CONFIRMED:
 *
 * The event has been planned and confirmed.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_EVENT_STATUS_CONFIRMED "http://schemas.google.com/g/2005#event.confirmed"

/**
 * GDATA_GD_EVENT_STATUS_TENTATIVE:
 *
 * The event has been planned, but only tentatively scheduled.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_EVENT_STATUS_TENTATIVE "http://schemas.google.com/g/2005#event.tentative"

/**
 * GDATA_GD_EVENT_VISIBILITY_CONFIDENTIAL:
 *
 * The event is visible to only certain people.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_EVENT_VISIBILITY_CONFIDENTIAL "http://schemas.google.com/g/2005#event.confidential"

/**
 * GDATA_GD_EVENT_VISIBILITY_DEFAULT:
 *
 * The event's visibility is inherited from the preferences of its owner.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_EVENT_VISIBILITY_DEFAULT "http://schemas.google.com/g/2005#event.default"

/**
 * GDATA_GD_EVENT_VISIBILITY_PRIVATE:
 *
 * The event is visible to very few people.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_EVENT_VISIBILITY_PRIVATE "http://schemas.google.com/g/2005#event.private"

/**
 * GDATA_GD_EVENT_VISIBILITY_PUBLIC:
 *
 * The event is visible to most people.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_EVENT_VISIBILITY_PUBLIC "http://schemas.google.com/g/2005#event.public"

/**
 * GDATA_GD_EVENT_TRANSPARENCY_OPAQUE:
 *
 * The event consumes time in calendars; its time will be marked as busy in a free/busy search.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_EVENT_TRANSPARENCY_OPAQUE "http://schemas.google.com/g/2005#event.opaque"

/**
 * GDATA_GD_EVENT_TRANSPARENCY_TRANSPARENT:
 *
 * The event does not consume time in calendars; its time will be not marked as busy in a free/busy search.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_EVENT_TRANSPARENCY_TRANSPARENT "http://schemas.google.com/g/2005#event.transparent"

#define GDATA_TYPE_GD_WHEN		(gdata_gd_when_get_type ())
#define GDATA_GD_WHEN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_GD_WHEN, GDataGDWhen))
#define GDATA_GD_WHEN_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_GD_WHEN, GDataGDWhenClass))
#define GDATA_IS_GD_WHEN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_GD_WHEN))
#define GDATA_IS_GD_WHEN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_GD_WHEN))
#define GDATA_GD_WHEN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_GD_WHEN, GDataGDWhenClass))

typedef struct _GDataGDWhenPrivate	GDataGDWhenPrivate;

/**
 * GDataGDWhen:
 *
 * All the fields in the #GDataGDWhen structure are private and should never be accessed directly.
 *
 * Since: 0.2.0
 */
typedef struct {
	GDataParsable parent;
	GDataGDWhenPrivate *priv;
} GDataGDWhen;

/**
 * GDataGDWhenClass:
 *
 * All the fields in the #GDataGDWhenClass structure are private and should never be accessed directly.
 *
 * Since: 0.4.0
 */
typedef struct {
	/*< private >*/
	GDataParsableClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataGDWhenClass;

GType gdata_gd_when_get_type (void) G_GNUC_CONST;

GDataGDWhen *gdata_gd_when_new (gint64 start_time, gint64 end_time, gboolean is_date) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

gint64 gdata_gd_when_get_start_time (GDataGDWhen *self);
void gdata_gd_when_set_start_time (GDataGDWhen *self, gint64 start_time);

gint64 gdata_gd_when_get_end_time (GDataGDWhen *self);
void gdata_gd_when_set_end_time (GDataGDWhen *self, gint64 end_time);

gboolean gdata_gd_when_is_date (GDataGDWhen *self) G_GNUC_PURE;
void gdata_gd_when_set_is_date (GDataGDWhen *self, gboolean is_date);

const gchar *gdata_gd_when_get_value_string (GDataGDWhen *self) G_GNUC_PURE;
void gdata_gd_when_set_value_string (GDataGDWhen *self, const gchar *value_string);

GList *gdata_gd_when_get_reminders (GDataGDWhen *self) G_GNUC_PURE;
void gdata_gd_when_add_reminder (GDataGDWhen *self, GDataGDReminder *reminder);
/* TODO: More reminder API */

G_END_DECLS

#endif /* !GDATA_GD_WHEN_H */
