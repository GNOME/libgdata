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

#ifndef GDATA_GCONTACT_JOT_H
#define GDATA_GCONTACT_JOT_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

/**
 * GDATA_GCONTACT_JOT_HOME:
 *
 * The relation type URI for a jot about a contact's home.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcJot">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_JOT_HOME "home"

/**
 * GDATA_GCONTACT_JOT_WORK:
 *
 * The relation type URI for a jot about a contact's work.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcJot">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_JOT_WORK "work"

/**
 * GDATA_GCONTACT_JOT_OTHER:
 *
 * The relation type URI for a jot about an other facet of a contact.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcJot">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_JOT_OTHER "other"

/**
 * GDATA_GCONTACT_JOT_KEYWORDS:
 *
 * The relation type URI for a jot with keywords about a contact.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcJot">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_JOT_KEYWORDS "keywords"

/**
 * GDATA_GCONTACT_JOT_USER:
 *
 * The relation type URI for a jot about the relationship between a contact and the user.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcJot">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_JOT_USER "user"

#define GDATA_TYPE_GCONTACT_JOT		(gdata_gcontact_jot_get_type ())
#define GDATA_GCONTACT_JOT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_GCONTACT_JOT, GDataGContactJot))
#define GDATA_GCONTACT_JOT_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_GCONTACT_JOT, GDataGContactJotClass))
#define GDATA_IS_GCONTACT_JOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_GCONTACT_JOT))
#define GDATA_IS_GCONTACT_JOT_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_GCONTACT_JOT))
#define GDATA_GCONTACT_JOT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_GCONTACT_JOT, GDataGContactJotClass))

typedef struct _GDataGContactJotPrivate	GDataGContactJotPrivate;

/**
 * GDataGContactJot:
 *
 * All the fields in the #GDataGContactJot structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	GDataParsable parent;
	GDataGContactJotPrivate *priv;
} GDataGContactJot;

/**
 * GDataGContactJotClass:
 *
 * All the fields in the #GDataGContactJotClass structure are private and should never be accessed directly.
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
} GDataGContactJotClass;

GType gdata_gcontact_jot_get_type (void) G_GNUC_CONST;

GDataGContactJot *gdata_gcontact_jot_new (const gchar *content, const gchar *relation_type) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_gcontact_jot_get_content (GDataGContactJot *self) G_GNUC_PURE;
void gdata_gcontact_jot_set_content (GDataGContactJot *self, const gchar *content);

const gchar *gdata_gcontact_jot_get_relation_type (GDataGContactJot *self) G_GNUC_PURE;
void gdata_gcontact_jot_set_relation_type (GDataGContactJot *self, const gchar *relation_type);

G_END_DECLS

#endif /* !GDATA_GCONTACT_JOT_H */
