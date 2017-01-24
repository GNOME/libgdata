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

#ifndef GDATA_GD_WHO_H
#define GDATA_GD_WHO_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

/**
 * GDATA_GD_WHO_EVENT_ATTENDEE:
 *
 * The relation type URI for a general meeting or event attendee.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_WHO_EVENT_ATTENDEE "http://schemas.google.com/g/2005#event.attendee"

/**
 * GDATA_GD_WHO_EVENT_ORGANIZER:
 *
 * The relation type URI for an event organizer (not necessarily an attendee).
 *
 * Since: 0.7.0
 */
#define GDATA_GD_WHO_EVENT_ORGANIZER "http://schemas.google.com/g/2005#event.organizer"

/**
 * GDATA_GD_WHO_EVENT_PERFORMER:
 *
 * The relation type URI for an event performer, which is similar to %GDATA_GD_WHO_EVENT_SPEAKER, but with more emphasis on art rather than speaking.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_WHO_EVENT_PERFORMER "http://schemas.google.com/g/2005#event.performer"

/**
 * GDATA_GD_WHO_EVENT_SPEAKER:
 *
 * The relation type URI for a speaker at an event.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_WHO_EVENT_SPEAKER "http://schemas.google.com/g/2005#event.speaker"

/* TODO: Message kind relation types (http://code.google.com/apis/gdata/docs/2.0/elements.html#gdWho) */

#define GDATA_TYPE_GD_WHO		(gdata_gd_who_get_type ())
#define GDATA_GD_WHO(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_GD_WHO, GDataGDWho))
#define GDATA_GD_WHO_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_GD_WHO, GDataGDWhoClass))
#define GDATA_IS_GD_WHO(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_GD_WHO))
#define GDATA_IS_GD_WHO_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_GD_WHO))
#define GDATA_GD_WHO_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_GD_WHO, GDataGDWhoClass))

typedef struct _GDataGDWhoPrivate	GDataGDWhoPrivate;

/**
 * GDataGDWho:
 *
 * All the fields in the #GDataGDWho structure are private and should never be accessed directly.
 *
 * Since: 0.2.0
 */
typedef struct {
	GDataParsable parent;
	GDataGDWhoPrivate *priv;
} GDataGDWho;

/**
 * GDataGDWhoClass:
 *
 * All the fields in the #GDataGDWhoClass structure are private and should never be accessed directly.
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
} GDataGDWhoClass;

GType gdata_gd_who_get_type (void) G_GNUC_CONST;

GDataGDWho *gdata_gd_who_new (const gchar *relation_type, const gchar *value_string,
                              const gchar *email_address) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_gd_who_get_relation_type (GDataGDWho *self) G_GNUC_PURE;
void gdata_gd_who_set_relation_type (GDataGDWho *self, const gchar *relation_type);

const gchar *gdata_gd_who_get_value_string (GDataGDWho *self) G_GNUC_PURE;
void gdata_gd_who_set_value_string (GDataGDWho *self, const gchar *value_string);

const gchar *gdata_gd_who_get_email_address (GDataGDWho *self) G_GNUC_PURE;
void gdata_gd_who_set_email_address (GDataGDWho *self, const gchar *email_address);

G_END_DECLS

#endif /* !GDATA_GD_WHO_H */
