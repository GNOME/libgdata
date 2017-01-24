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

#ifndef GDATA_GCONTACT_EVENT_H
#define GDATA_GCONTACT_EVENT_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

/**
 * GDATA_GCONTACT_EVENT_ANNIVERSARY:
 *
 * The relation type URI for an anniversary event.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcEvent">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_EVENT_ANNIVERSARY "anniversary"

/**
 * GDATA_GCONTACT_EVENT_OTHER:
 *
 * The relation type URI for a miscellaneous event.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcEvent">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_EVENT_OTHER "other"

#define GDATA_TYPE_GCONTACT_EVENT		(gdata_gcontact_event_get_type ())
#define GDATA_GCONTACT_EVENT(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_GCONTACT_EVENT, GDataGContactEvent))
#define GDATA_GCONTACT_EVENT_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_GCONTACT_EVENT, GDataGContactEventClass))
#define GDATA_IS_GCONTACT_EVENT(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_GCONTACT_EVENT))
#define GDATA_IS_GCONTACT_EVENT_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_GCONTACT_EVENT))
#define GDATA_GCONTACT_EVENT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_GCONTACT_EVENT, GDataGContactEventClass))

typedef struct _GDataGContactEventPrivate	GDataGContactEventPrivate;

/**
 * GDataGContactEvent:
 *
 * All the fields in the #GDataGContactEvent structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	GDataParsable parent;
	GDataGContactEventPrivate *priv;
} GDataGContactEvent;

/**
 * GDataGContactEventClass:
 *
 * All the fields in the #GDataGContactEventClass structure are private and should never be accessed directly.
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
} GDataGContactEventClass;

GType gdata_gcontact_event_get_type (void) G_GNUC_CONST;

GDataGContactEvent *gdata_gcontact_event_new (const GDate *date, const gchar *relation_type,
                                              const gchar *label) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

void gdata_gcontact_event_get_date (GDataGContactEvent *self, GDate *date);
void gdata_gcontact_event_set_date (GDataGContactEvent *self, const GDate *date);

const gchar *gdata_gcontact_event_get_relation_type (GDataGContactEvent *self) G_GNUC_PURE;
void gdata_gcontact_event_set_relation_type (GDataGContactEvent *self, const gchar *relation_type);

const gchar *gdata_gcontact_event_get_label (GDataGContactEvent *self) G_GNUC_PURE;
void gdata_gcontact_event_set_label (GDataGContactEvent *self, const gchar *label);

G_END_DECLS

#endif /* !GDATA_GCONTACT_EVENT_H */
