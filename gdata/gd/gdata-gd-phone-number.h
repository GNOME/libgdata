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

#ifndef GDATA_GD_PHONE_NUMBER_H
#define GDATA_GD_PHONE_NUMBER_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

/**
 * GDATA_GD_PHONE_NUMBER_ASSISTANT:
 *
 * The relation type URI for the phone number of an assistant.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_ASSISTANT "http://schemas.google.com/g/2005#assistant"

/**
 * GDATA_GD_PHONE_NUMBER_CALLBACK:
 *
 * The relation type URI for the phone number of a callback service.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_CALLBACK "http://schemas.google.com/g/2005#callback"

/**
 * GDATA_GD_PHONE_NUMBER_CAR:
 *
 * The relation type URI for the phone number of a car phone.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_CAR "http://schemas.google.com/g/2005#car"

/**
 * GDATA_GD_PHONE_NUMBER_COMPANY_MAIN:
 *
 * The relation type URI for the main phone number of a company.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_COMPANY_MAIN "http://schemas.google.com/g/2005#company_main"

/**
 * GDATA_GD_PHONE_NUMBER_FAX:
 *
 * The relation type URI for the phone number of a fax machine.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_FAX "http://schemas.google.com/g/2005#fax"

/**
 * GDATA_GD_PHONE_NUMBER_HOME:
 *
 * The relation type URI for a home phone number.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_HOME "http://schemas.google.com/g/2005#home"

/**
 * GDATA_GD_PHONE_NUMBER_HOME_FAX:
 *
 * The relation type URI for the phone number of a home fax machine.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_HOME_FAX "http://schemas.google.com/g/2005#home_fax"

/**
 * GDATA_GD_PHONE_NUMBER_ISDN:
 *
 * The relation type URI for the phone number of an ISDN phone.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_ISDN "http://schemas.google.com/g/2005#isdn"

/**
 * GDATA_GD_PHONE_NUMBER_MAIN:
 *
 * The relation type URI for the main phone number of a person.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_MAIN "http://schemas.google.com/g/2005#main"

/**
 * GDATA_GD_PHONE_NUMBER_MOBILE:
 *
 * The relation type URI for the phone number of a mobile phone.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_MOBILE "http://schemas.google.com/g/2005#mobile"

/**
 * GDATA_GD_PHONE_NUMBER_OTHER:
 *
 * The relation type URI for a miscellaneous phone number.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_OTHER "http://schemas.google.com/g/2005#other"

/**
 * GDATA_GD_PHONE_NUMBER_OTHER_FAX:
 *
 * The relation type URI for a miscellaneous fax machine's phone number.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_OTHER_FAX "http://schemas.google.com/g/2005#other_fax"

/**
 * GDATA_GD_PHONE_NUMBER_PAGER:
 *
 * The relation type URI for the phone number of a pager.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_PAGER "http://schemas.google.com/g/2005#pager"

/**
 * GDATA_GD_PHONE_NUMBER_RADIO:
 *
 * The relation type URI for the phone number of a radio phone.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_RADIO "http://schemas.google.com/g/2005#radio"

/**
 * GDATA_GD_PHONE_NUMBER_TELEX:
 *
 * The relation type URI for the phone number of a telex machine.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_TELEX "http://schemas.google.com/g/2005#telex"

/**
 * GDATA_GD_PHONE_NUMBER_TTY_TDD:
 *
 * The relation type URI for the phone number of a TTY TTD.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_TTY_TDD "http://schemas.google.com/g/2005#tty_tdd"

/**
 * GDATA_GD_PHONE_NUMBER_WORK:
 *
 * The relation type URI for the phone number of a work place.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_WORK "http://schemas.google.com/g/2005#work"

/**
 * GDATA_GD_PHONE_NUMBER_WORK_FAX:
 *
 * The relation type URI for the phone number of a work fax machine.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_WORK_FAX "http://schemas.google.com/g/2005#work_fax"

/**
 * GDATA_GD_PHONE_NUMBER_WORK_MOBILE:
 *
 * The relation type URI for the phone number of a work mobile phone.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_WORK_MOBILE "http://schemas.google.com/g/2005#work_mobile"

/**
 * GDATA_GD_PHONE_NUMBER_WORK_PAGER:
 *
 * The relation type URI for the phone number of a work pager.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_PHONE_NUMBER_WORK_PAGER "http://schemas.google.com/g/2005#work_pager"

#define GDATA_TYPE_GD_PHONE_NUMBER		(gdata_gd_phone_number_get_type ())
#define GDATA_GD_PHONE_NUMBER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_GD_PHONE_NUMBER, GDataGDPhoneNumber))
#define GDATA_GD_PHONE_NUMBER_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_GD_PHONE_NUMBER, GDataGDPhoneNumberClass))
#define GDATA_IS_GD_PHONE_NUMBER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_GD_PHONE_NUMBER))
#define GDATA_IS_GD_PHONE_NUMBER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_GD_PHONE_NUMBER))
#define GDATA_GD_PHONE_NUMBER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_GD_PHONE_NUMBER, GDataGDPhoneNumberClass))

typedef struct _GDataGDPhoneNumberPrivate	GDataGDPhoneNumberPrivate;

/**
 * GDataGDPhoneNumber:
 *
 * All the fields in the #GDataGDPhoneNumber structure are private and should never be accessed directly.
 *
 * Since: 0.2.0
 */
typedef struct {
	GDataParsable parent;
	GDataGDPhoneNumberPrivate *priv;
} GDataGDPhoneNumber;

/**
 * GDataGDPhoneNumberClass:
 *
 * All the fields in the #GDataGDPhoneNumberClass structure are private and should never be accessed directly.
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
} GDataGDPhoneNumberClass;

GType gdata_gd_phone_number_get_type (void) G_GNUC_CONST;

GDataGDPhoneNumber *gdata_gd_phone_number_new (const gchar *number, const gchar *relation_type, const gchar *label, const gchar *uri,
                                               gboolean is_primary) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_gd_phone_number_get_number (GDataGDPhoneNumber *self) G_GNUC_PURE;
void gdata_gd_phone_number_set_number (GDataGDPhoneNumber *self, const gchar *number);

const gchar *gdata_gd_phone_number_get_uri (GDataGDPhoneNumber *self) G_GNUC_PURE;
void gdata_gd_phone_number_set_uri (GDataGDPhoneNumber *self, const gchar *uri);

const gchar *gdata_gd_phone_number_get_relation_type (GDataGDPhoneNumber *self) G_GNUC_PURE;
void gdata_gd_phone_number_set_relation_type (GDataGDPhoneNumber *self, const gchar *relation_type);

const gchar *gdata_gd_phone_number_get_label (GDataGDPhoneNumber *self) G_GNUC_PURE;
void gdata_gd_phone_number_set_label (GDataGDPhoneNumber *self, const gchar *label);

gboolean gdata_gd_phone_number_is_primary (GDataGDPhoneNumber *self) G_GNUC_PURE;
void gdata_gd_phone_number_set_is_primary (GDataGDPhoneNumber *self, gboolean is_primary);

G_END_DECLS

#endif /* !GDATA_GD_PHONE_NUMBER_H */
