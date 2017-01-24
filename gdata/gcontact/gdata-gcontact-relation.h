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

#ifndef GDATA_GCONTACT_RELATION_H
#define GDATA_GCONTACT_RELATION_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

/**
 * GDATA_GCONTACT_RELATION_ASSISTANT:
 *
 * The relation type URI for a contact's assistant.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_RELATION_ASSISTANT "assistant"

/**
 * GDATA_GCONTACT_RELATION_BROTHER:
 *
 * The relation type URI for a contact's brother.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_RELATION_BROTHER "brother"

/**
 * GDATA_GCONTACT_RELATION_CHILD:
 *
 * The relation type URI for a contact's child.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_RELATION_CHILD "child"

/**
 * GDATA_GCONTACT_RELATION_DOMESTIC_PARTNER:
 *
 * The relation type URI for a contact's domestic partner.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_RELATION_DOMESTIC_PARTNER "domestic-partner"

/**
 * GDATA_GCONTACT_RELATION_FATHER:
 *
 * The relation type URI for a contact's father.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_RELATION_FATHER "father"

/**
 * GDATA_GCONTACT_RELATION_FRIEND:
 *
 * The relation type URI for a contact's friend.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_RELATION_FRIEND "friend"

/**
 * GDATA_GCONTACT_RELATION_MANAGER:
 *
 * The relation type URI for a contact's manager.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_RELATION_MANAGER "manager"

/**
 * GDATA_GCONTACT_RELATION_MOTHER:
 *
 * The relation type URI for a contact's mother.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_RELATION_MOTHER "mother"

/**
 * GDATA_GCONTACT_RELATION_PARENT:
 *
 * The relation type URI for a contact's parent.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_RELATION_PARENT "parent"

/**
 * GDATA_GCONTACT_RELATION_PARTNER:
 *
 * The relation type URI for a contact's business partner.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_RELATION_PARTNER "partner"

/**
 * GDATA_GCONTACT_RELATION_REFERRER:
 *
 * The relation type URI for a contact's referrer.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_RELATION_REFERRER "referred-by"

/**
 * GDATA_GCONTACT_RELATION_RELATIVE:
 *
 * The relation type URI for a contact's (general) family relative.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_RELATION_RELATIVE "relative"

/**
 * GDATA_GCONTACT_RELATION_SISTER:
 *
 * The relation type URI for a contact's sister.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_RELATION_SISTER "sister"

/**
 * GDATA_GCONTACT_RELATION_SPOUSE:
 *
 * The relation type URI for a contact's spouse.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcRelation">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_RELATION_SPOUSE "spouse"

#define GDATA_TYPE_GCONTACT_RELATION		(gdata_gcontact_relation_get_type ())
#define GDATA_GCONTACT_RELATION(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_GCONTACT_RELATION, GDataGContactRelation))
#define GDATA_GCONTACT_RELATION_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_GCONTACT_RELATION, GDataGContactRelationClass))
#define GDATA_IS_GCONTACT_RELATION(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_GCONTACT_RELATION))
#define GDATA_IS_GCONTACT_RELATION_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_GCONTACT_RELATION))
#define GDATA_GCONTACT_RELATION_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_GCONTACT_RELATION, GDataGContactRelationClass))

typedef struct _GDataGContactRelationPrivate	GDataGContactRelationPrivate;

/**
 * GDataGContactRelation:
 *
 * All the fields in the #GDataGContactRelation structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	GDataParsable parent;
	GDataGContactRelationPrivate *priv;
} GDataGContactRelation;

/**
 * GDataGContactRelationClass:
 *
 * All the fields in the #GDataGContactRelationClass structure are private and should never be accessed directly.
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
} GDataGContactRelationClass;

GType gdata_gcontact_relation_get_type (void) G_GNUC_CONST;

GDataGContactRelation *gdata_gcontact_relation_new (const gchar *name, const gchar *relation_type,
                                                    const gchar *label) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_gcontact_relation_get_name (GDataGContactRelation *self) G_GNUC_PURE;
void gdata_gcontact_relation_set_name (GDataGContactRelation *self, const gchar *name);

const gchar *gdata_gcontact_relation_get_relation_type (GDataGContactRelation *self) G_GNUC_PURE;
void gdata_gcontact_relation_set_relation_type (GDataGContactRelation *self, const gchar *relation_type);

const gchar *gdata_gcontact_relation_get_label (GDataGContactRelation *self) G_GNUC_PURE;
void gdata_gcontact_relation_set_label (GDataGContactRelation *self, const gchar *label);

G_END_DECLS

#endif /* !GDATA_GCONTACT_RELATION_H */
