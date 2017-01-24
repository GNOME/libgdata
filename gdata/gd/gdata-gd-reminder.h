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

#ifndef GDATA_GD_REMINDER_H
#define GDATA_GD_REMINDER_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

/**
 * GDATA_GD_REMINDER_ALERT:
 *
 * The #GDataGDReminder:method for an alert to appear in the user's browser.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_REMINDER_ALERT "alert"

/**
 * GDATA_GD_REMINDER_EMAIL:
 *
 * The #GDataGDReminder:method for an alert to be sent to the user by e-mail.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_REMINDER_EMAIL "email"

/**
 * GDATA_GD_REMINDER_SMS:
 *
 * The #GDataGDReminder:method for an alert to be sent to the user by SMS.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_REMINDER_SMS "sms"

#define GDATA_TYPE_GD_REMINDER		(gdata_gd_reminder_get_type ())
#define GDATA_GD_REMINDER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_GD_REMINDER, GDataGDReminder))
#define GDATA_GD_REMINDER_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_GD_REMINDER, GDataGDReminderClass))
#define GDATA_IS_GD_REMINDER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_GD_REMINDER))
#define GDATA_IS_GD_REMINDER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_GD_REMINDER))
#define GDATA_GD_REMINDER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_GD_REMINDER, GDataGDReminderClass))

typedef struct _GDataGDReminderPrivate	GDataGDReminderPrivate;

/**
 * GDataGDReminder:
 *
 * All the fields in the #GDataGDReminder structure are private and should never be accessed directly.
 *
 * Since: 0.2.0
 */
typedef struct {
	GDataParsable parent;
	GDataGDReminderPrivate *priv;
} GDataGDReminder;

/**
 * GDataGDReminderClass:
 *
 * All the fields in the #GDataGDReminderClass structure are private and should never be accessed directly.
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
} GDataGDReminderClass;

GType gdata_gd_reminder_get_type (void) G_GNUC_CONST;

GDataGDReminder *gdata_gd_reminder_new (const gchar *method, gint64 absolute_time, gint relative_time) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_gd_reminder_get_method (GDataGDReminder *self) G_GNUC_PURE;
void gdata_gd_reminder_set_method (GDataGDReminder *self, const gchar *method);

gint64 gdata_gd_reminder_get_absolute_time (GDataGDReminder *self);
void gdata_gd_reminder_set_absolute_time (GDataGDReminder *self, gint64 absolute_time);
gboolean gdata_gd_reminder_is_absolute_time (GDataGDReminder *self);

gint gdata_gd_reminder_get_relative_time (GDataGDReminder *self) G_GNUC_PURE;
void gdata_gd_reminder_set_relative_time (GDataGDReminder *self, gint relative_time);

G_END_DECLS

#endif /* !GDATA_GD_REMINDER_H */
