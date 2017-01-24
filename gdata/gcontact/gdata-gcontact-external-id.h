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

#ifndef GDATA_GCONTACT_EXTERNAL_ID_H
#define GDATA_GCONTACT_EXTERNAL_ID_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

/**
 * GDATA_GCONTACT_EXTERNAL_ID_ACCOUNT:
 *
 * The relation type URI for an account number identifier.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcExternalId">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_EXTERNAL_ID_ACCOUNT "account"

/**
 * GDATA_GCONTACT_EXTERNAL_ID_CUSTOMER:
 *
 * The relation type URI for a customer identifier.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcExternalId">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_EXTERNAL_ID_CUSTOMER "customer"

/**
 * GDATA_GCONTACT_EXTERNAL_ID_NETWORK:
 *
 * The relation type URI for a network identifier.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcExternalId">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_EXTERNAL_ID_NETWORK "network"

/**
 * GDATA_GCONTACT_EXTERNAL_ID_ORGANIZATION:
 *
 * The relation type URI for an identifier related to an organization the contact is associated with.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcExternalId">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_EXTERNAL_ID_ORGANIZATION "organization"

#define GDATA_TYPE_GCONTACT_EXTERNAL_ID		(gdata_gcontact_external_id_get_type ())
#define GDATA_GCONTACT_EXTERNAL_ID(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_GCONTACT_EXTERNAL_ID, GDataGContactExternalID))
#define GDATA_GCONTACT_EXTERNAL_ID_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_GCONTACT_EXTERNAL_ID, GDataGContactExternalIDClass))
#define GDATA_IS_GCONTACT_EXTERNAL_ID(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_GCONTACT_EXTERNAL_ID))
#define GDATA_IS_GCONTACT_EXTERNAL_ID_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_GCONTACT_EXTERNAL_ID))
#define GDATA_GCONTACT_EXTERNAL_ID_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_GCONTACT_EXTERNAL_ID, GDataGContactExternalIDClass))

typedef struct _GDataGContactExternalIDPrivate	GDataGContactExternalIDPrivate;

/**
 * GDataGContactExternalID:
 *
 * All the fields in the #GDataGContactExternalID structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	GDataParsable parent;
	GDataGContactExternalIDPrivate *priv;
} GDataGContactExternalID;

/**
 * GDataGContactExternalIDClass:
 *
 * All the fields in the #GDataGContactExternalIDClass structure are private and should never be accessed directly.
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
} GDataGContactExternalIDClass;

GType gdata_gcontact_external_id_get_type (void) G_GNUC_CONST;

GDataGContactExternalID *gdata_gcontact_external_id_new (const gchar *value, const gchar *relation_type,
                                                         const gchar *label) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_gcontact_external_id_get_value (GDataGContactExternalID *self) G_GNUC_PURE;
void gdata_gcontact_external_id_set_value (GDataGContactExternalID *self, const gchar *value);

const gchar *gdata_gcontact_external_id_get_relation_type (GDataGContactExternalID *self) G_GNUC_PURE;
void gdata_gcontact_external_id_set_relation_type (GDataGContactExternalID *self, const gchar *relation_type);

const gchar *gdata_gcontact_external_id_get_label (GDataGContactExternalID *self) G_GNUC_PURE;
void gdata_gcontact_external_id_set_label (GDataGContactExternalID *self, const gchar *label);

G_END_DECLS

#endif /* !GDATA_GCONTACT_EXTERNAL_ID_H */
