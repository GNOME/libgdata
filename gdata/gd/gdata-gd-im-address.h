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

#ifndef GDATA_GD_IM_ADDRESS_H
#define GDATA_GD_IM_ADDRESS_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

/**
 * GDATA_GD_IM_ADDRESS_HOME:
 *
 * The relation type URI for a home IM address.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_IM_ADDRESS_HOME "http://schemas.google.com/g/2005#home"

/**
 * GDATA_GD_IM_ADDRESS_NETMEETING:
 *
 * The relation type URI for a Microsoft NetMeeting IM address.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_IM_ADDRESS_NETMEETING "http://schemas.google.com/g/2005#netmeeting"

/**
 * GDATA_GD_IM_ADDRESS_OTHER:
 *
 * The relation type URI for a miscellaneous IM address.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_IM_ADDRESS_OTHER "http://schemas.google.com/g/2005#other"

/**
 * GDATA_GD_IM_ADDRESS_WORK:
 *
 * The relation type URI for a work IM address.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_IM_ADDRESS_WORK "http://schemas.google.com/g/2005#work"

/**
 * GDATA_GD_IM_PROTOCOL_AIM:
 *
 * The protocol type URI for an AIM IM address.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_IM_PROTOCOL_AIM "http://schemas.google.com/g/2005#AIM"

/**
 * GDATA_GD_IM_PROTOCOL_LIVE_MESSENGER:
 *
 * The protocol type URI for an Windows Live Messenger IM address.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_IM_PROTOCOL_LIVE_MESSENGER "http://schemas.google.com/g/2005#MSN"

/**
 * GDATA_GD_IM_PROTOCOL_YAHOO_MESSENGER:
 *
 * The protocol type URI for a Yahoo! Messenger IM address.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_IM_PROTOCOL_YAHOO_MESSENGER "http://schemas.google.com/g/2005#YAHOO"

/**
 * GDATA_GD_IM_PROTOCOL_SKYPE:
 *
 * The protocol type URI for a Skype IM address.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_IM_PROTOCOL_SKYPE "http://schemas.google.com/g/2005#SKYPE"

/**
 * GDATA_GD_IM_PROTOCOL_QQ:
 *
 * The protocol type URI for a QQ IM address.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_IM_PROTOCOL_QQ "http://schemas.google.com/g/2005#QQ"

/**
 * GDATA_GD_IM_PROTOCOL_GOOGLE_TALK:
 *
 * The protocol type URI for a Google Talk IM address.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_IM_PROTOCOL_GOOGLE_TALK "http://schemas.google.com/g/2005#GOOGLE_TALK"

/**
 * GDATA_GD_IM_PROTOCOL_ICQ:
 *
 * The protocol type URI for an ICQ IM address.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_IM_PROTOCOL_ICQ "http://schemas.google.com/g/2005#ICQ"

/**
 * GDATA_GD_IM_PROTOCOL_JABBER:
 *
 * The protocol type URI for a Jabber IM address.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_IM_PROTOCOL_JABBER "http://schemas.google.com/g/2005#JABBER"

#define GDATA_TYPE_GD_IM_ADDRESS		(gdata_gd_im_address_get_type ())
#define GDATA_GD_IM_ADDRESS(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_GD_IM_ADDRESS, GDataGDIMAddress))
#define GDATA_GD_IM_ADDRESS_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_GD_IM_ADDRESS, GDataGDIMAddressClass))
#define GDATA_IS_GD_IM_ADDRESS(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_GD_IM_ADDRESS))
#define GDATA_IS_GD_IM_ADDRESS_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_GD_IM_ADDRESS))
#define GDATA_GD_IM_ADDRESS_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_GD_IM_ADDRESS, GDataGDIMAddressClass))

typedef struct _GDataGDIMAddressPrivate		GDataGDIMAddressPrivate;

/**
 * GDataGDIMAddress:
 *
 * All the fields in the #GDataGDIMAddress structure are private and should never be accessed directly.
 *
 * Since: 0.2.0
 */
typedef struct {
	GDataParsable parent;
	GDataGDIMAddressPrivate *priv;
} GDataGDIMAddress;

/**
 * GDataGDIMAddressClass:
 *
 * All the fields in the #GDataGDIMAddressClass structure are private and should never be accessed directly.
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
} GDataGDIMAddressClass;

GType gdata_gd_im_address_get_type (void) G_GNUC_CONST;

GDataGDIMAddress *gdata_gd_im_address_new (const gchar *address, const gchar *protocol, const gchar *relation_type, const gchar *label,
                                           gboolean is_primary) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_gd_im_address_get_address (GDataGDIMAddress *self) G_GNUC_PURE;
void gdata_gd_im_address_set_address (GDataGDIMAddress *self, const gchar *address);

const gchar *gdata_gd_im_address_get_protocol (GDataGDIMAddress *self) G_GNUC_PURE;
void gdata_gd_im_address_set_protocol (GDataGDIMAddress *self, const gchar *protocol);

const gchar *gdata_gd_im_address_get_relation_type (GDataGDIMAddress *self) G_GNUC_PURE;
void gdata_gd_im_address_set_relation_type (GDataGDIMAddress *self, const gchar *relation_type);

const gchar *gdata_gd_im_address_get_label (GDataGDIMAddress *self) G_GNUC_PURE;
void gdata_gd_im_address_set_label (GDataGDIMAddress *self, const gchar *label);

gboolean gdata_gd_im_address_is_primary (GDataGDIMAddress *self) G_GNUC_PURE;
void gdata_gd_im_address_set_is_primary (GDataGDIMAddress *self, gboolean is_primary);

G_END_DECLS

#endif /* !GDATA_GD_IM_ADDRESS_H */
