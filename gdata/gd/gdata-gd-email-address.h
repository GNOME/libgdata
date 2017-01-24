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

#ifndef GDATA_GD_EMAIL_ADDRESS_H
#define GDATA_GD_EMAIL_ADDRESS_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

/**
 * GDATA_GD_EMAIL_ADDRESS_HOME:
 *
 * The relation type URI for a home e-mail address.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_EMAIL_ADDRESS_HOME "http://schemas.google.com/g/2005#home"

/**
 * GDATA_GD_EMAIL_ADDRESS_OTHER:
 *
 * The relation type URI for a miscellaneous e-mail address.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_EMAIL_ADDRESS_OTHER "http://schemas.google.com/g/2005#other"

/**
 * GDATA_GD_EMAIL_ADDRESS_WORK:
 *
 * The relation type URI for a work e-mail address.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_EMAIL_ADDRESS_WORK "http://schemas.google.com/g/2005#work"

#define GDATA_TYPE_GD_EMAIL_ADDRESS		(gdata_gd_email_address_get_type ())
#define GDATA_GD_EMAIL_ADDRESS(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_GD_EMAIL_ADDRESS, GDataGDEmailAddress))
#define GDATA_GD_EMAIL_ADDRESS_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_GD_EMAIL_ADDRESS, GDataGDEmailAddressClass))
#define GDATA_IS_GD_EMAIL_ADDRESS(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_GD_EMAIL_ADDRESS))
#define GDATA_IS_GD_EMAIL_ADDRESS_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_GD_EMAIL_ADDRESS))
#define GDATA_GD_EMAIL_ADDRESS_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_GD_EMAIL_ADDRESS, GDataGDEmailAddressClass))

typedef struct _GDataGDEmailAddressPrivate	GDataGDEmailAddressPrivate;

/**
 * GDataGDEmailAddress:
 *
 * All the fields in the #GDataGDEmailAddress structure are private and should never be accessed directly.
 *
 * Since: 0.2.0
 */
typedef struct {
	GDataParsable parent;
	GDataGDEmailAddressPrivate *priv;
} GDataGDEmailAddress;

/**
 * GDataGDEmailAddressClass:
 *
 * All the fields in the #GDataGDEmailAddressClass structure are private and should never be accessed directly.
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
} GDataGDEmailAddressClass;

GType gdata_gd_email_address_get_type (void) G_GNUC_CONST;

GDataGDEmailAddress *gdata_gd_email_address_new (const gchar *address, const gchar *relation_type,
                                                 const gchar *label, gboolean is_primary) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_gd_email_address_get_address (GDataGDEmailAddress *self) G_GNUC_PURE;
void gdata_gd_email_address_set_address (GDataGDEmailAddress *self, const gchar *address);

const gchar *gdata_gd_email_address_get_relation_type (GDataGDEmailAddress *self) G_GNUC_PURE;
void gdata_gd_email_address_set_relation_type (GDataGDEmailAddress *self, const gchar *relation_type);

const gchar *gdata_gd_email_address_get_label (GDataGDEmailAddress *self) G_GNUC_PURE;
void gdata_gd_email_address_set_label (GDataGDEmailAddress *self, const gchar *label);

const gchar *gdata_gd_email_address_get_display_name (GDataGDEmailAddress *self) G_GNUC_PURE;
void gdata_gd_email_address_set_display_name (GDataGDEmailAddress *self, const gchar *display_name);

gboolean gdata_gd_email_address_is_primary (GDataGDEmailAddress *self) G_GNUC_PURE;
void gdata_gd_email_address_set_is_primary (GDataGDEmailAddress *self, gboolean is_primary);

G_END_DECLS

#endif /* !GDATA_GD_EMAIL_ADDRESS_H */
