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

#ifndef GDATA_GD_POSTAL_ADDRESS_H
#define GDATA_GD_POSTAL_ADDRESS_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

/**
 * GDATA_GD_POSTAL_ADDRESS_WORK:
 *
 * The relation type URI for the postal address of a workplace.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_POSTAL_ADDRESS_WORK "http://schemas.google.com/g/2005#work"

/**
 * GDATA_GD_POSTAL_ADDRESS_HOME:
 *
 * The relation type URI for the postal address of a home.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_POSTAL_ADDRESS_HOME "http://schemas.google.com/g/2005#home"

/**
 * GDATA_GD_POSTAL_ADDRESS_OTHER:
 *
 * The relation type URI for a miscellaneous postal address.
 *
 * Since: 0.7.0
 */
#define GDATA_GD_POSTAL_ADDRESS_OTHER "http://schemas.google.com/g/2005#other"

/**
 * GDATA_GD_MAIL_CLASS_BOTH:
 *
 * Parcels and letters can be sent to the address. Value for #GDataGDPostalAddress:mail-class.
 *
 * Since: 0.5.0
 */
#define GDATA_GD_MAIL_CLASS_BOTH "http://schemas.google.com/g/2005#both"

/**
 * GDATA_GD_MAIL_CLASS_LETTERS:
 *
 * Only letters can be sent to the address. Value for #GDataGDPostalAddress:mail-class.
 *
 * Since: 0.5.0
 */
#define GDATA_GD_MAIL_CLASS_LETTERS "http://schemas.google.com/g/2005#letters"

/**
 * GDATA_GD_MAIL_CLASS_PARCELS:
 *
 * Only parcels can be sent to the address. Value for #GDataGDPostalAddress:mail-class.
 *
 * Since: 0.5.0
 */
#define GDATA_GD_MAIL_CLASS_PARCELS "http://schemas.google.com/g/2005#parcels"

/**
 * GDATA_GD_MAIL_CLASS_NEITHER:
 *
 * Address is purely locational and cannot be used for mail. Value for #GDataGDPostalAddress:mail-class.
 *
 * Since: 0.5.0
 */
#define GDATA_GD_MAIL_CLASS_NEITHER "http://schemas.google.com/g/2005#neither"

/**
 * GDATA_GD_ADDRESS_USAGE_GENERAL:
 *
 * The address is for general usage. Value for #GDataGDPostalAddress:usage.
 *
 * Since: 0.5.0
 */
#define GDATA_GD_ADDRESS_USAGE_GENERAL "http://schemas.google.com/g/2005#general"

/**
 * GDATA_GD_ADDRESS_USAGE_LOCAL:
 *
 * The address is for local usage. Value for #GDataGDPostalAddress:usage.
 *
 * Since: 0.5.0
 */
#define GDATA_GD_ADDRESS_USAGE_LOCAL "http://schemas.google.com/g/2005#local"

#define GDATA_TYPE_GD_POSTAL_ADDRESS		(gdata_gd_postal_address_get_type ())
#define GDATA_GD_POSTAL_ADDRESS(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_GD_POSTAL_ADDRESS, GDataGDPostalAddress))
#define GDATA_GD_POSTAL_ADDRESS_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_GD_POSTAL_ADDRESS, GDataGDPostalAddressClass))
#define GDATA_IS_GD_POSTAL_ADDRESS(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_GD_POSTAL_ADDRESS))
#define GDATA_IS_GD_POSTAL_ADDRESS_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_GD_POSTAL_ADDRESS))
#define GDATA_GD_POSTAL_ADDRESS_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_GD_POSTAL_ADDRESS, GDataGDPostalAddressClass))

typedef struct _GDataGDPostalAddressPrivate	GDataGDPostalAddressPrivate;

/**
 * GDataGDPostalAddress:
 *
 * All the fields in the #GDataGDPostalAddress structure are private and should never be accessed directly.
 *
 * Since: 0.2.0
 */
typedef struct {
	GDataParsable parent;
	GDataGDPostalAddressPrivate *priv;
} GDataGDPostalAddress;

/**
 * GDataGDPostalAddressClass:
 *
 * All the fields in the #GDataGDPostalAddressClass structure are private and should never be accessed directly.
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
} GDataGDPostalAddressClass;

GType gdata_gd_postal_address_get_type (void) G_GNUC_CONST;

GDataGDPostalAddress *gdata_gd_postal_address_new (const gchar *relation_type, const gchar *label,
                                                   gboolean is_primary) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_gd_postal_address_get_address (GDataGDPostalAddress *self) G_GNUC_PURE;
void gdata_gd_postal_address_set_address (GDataGDPostalAddress *self, const gchar *address);

const gchar *gdata_gd_postal_address_get_relation_type (GDataGDPostalAddress *self) G_GNUC_PURE;
void gdata_gd_postal_address_set_relation_type (GDataGDPostalAddress *self, const gchar *relation_type);

const gchar *gdata_gd_postal_address_get_label (GDataGDPostalAddress *self) G_GNUC_PURE;
void gdata_gd_postal_address_set_label (GDataGDPostalAddress *self, const gchar *label);

gboolean gdata_gd_postal_address_is_primary (GDataGDPostalAddress *self) G_GNUC_PURE;
void gdata_gd_postal_address_set_is_primary (GDataGDPostalAddress *self, gboolean is_primary);

const gchar *gdata_gd_postal_address_get_mail_class (GDataGDPostalAddress *self) G_GNUC_PURE;
void gdata_gd_postal_address_set_mail_class (GDataGDPostalAddress *self, const gchar *mail_class);

const gchar *gdata_gd_postal_address_get_usage (GDataGDPostalAddress *self) G_GNUC_PURE;
void gdata_gd_postal_address_set_usage (GDataGDPostalAddress *self, const gchar *usage);

const gchar *gdata_gd_postal_address_get_agent (GDataGDPostalAddress *self) G_GNUC_PURE;
void gdata_gd_postal_address_set_agent (GDataGDPostalAddress *self, const gchar *agent);

const gchar *gdata_gd_postal_address_get_house_name (GDataGDPostalAddress *self) G_GNUC_PURE;
void gdata_gd_postal_address_set_house_name (GDataGDPostalAddress *self, const gchar *house_name);

const gchar *gdata_gd_postal_address_get_street (GDataGDPostalAddress *self) G_GNUC_PURE;
void gdata_gd_postal_address_set_street (GDataGDPostalAddress *self, const gchar *street);

const gchar *gdata_gd_postal_address_get_po_box (GDataGDPostalAddress *self) G_GNUC_PURE;
void gdata_gd_postal_address_set_po_box (GDataGDPostalAddress *self, const gchar *po_box);

const gchar *gdata_gd_postal_address_get_neighborhood (GDataGDPostalAddress *self) G_GNUC_PURE;
void gdata_gd_postal_address_set_neighborhood (GDataGDPostalAddress *self, const gchar *neighborhood);

const gchar *gdata_gd_postal_address_get_city (GDataGDPostalAddress *self) G_GNUC_PURE;
void gdata_gd_postal_address_set_city (GDataGDPostalAddress *self, const gchar *city);

const gchar *gdata_gd_postal_address_get_subregion (GDataGDPostalAddress *self) G_GNUC_PURE;
void gdata_gd_postal_address_set_subregion (GDataGDPostalAddress *self, const gchar *subregion);

const gchar *gdata_gd_postal_address_get_region (GDataGDPostalAddress *self) G_GNUC_PURE;
void gdata_gd_postal_address_set_region (GDataGDPostalAddress *self, const gchar *region);

const gchar *gdata_gd_postal_address_get_postcode (GDataGDPostalAddress *self) G_GNUC_PURE;
void gdata_gd_postal_address_set_postcode (GDataGDPostalAddress *self, const gchar *postcode);

const gchar *gdata_gd_postal_address_get_country (GDataGDPostalAddress *self) G_GNUC_PURE;
const gchar *gdata_gd_postal_address_get_country_code (GDataGDPostalAddress *self) G_GNUC_PURE;
void gdata_gd_postal_address_set_country (GDataGDPostalAddress *self, const gchar *country, const gchar *country_code);

G_END_DECLS

#endif /* !GDATA_GD_POSTAL_ADDRESS_H */
