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

#ifndef GDATA_GCONTACT_WEBSITE_H
#define GDATA_GCONTACT_WEBSITE_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

/**
 * GDATA_GCONTACT_WEBSITE_HOME_PAGE:
 *
 * The relation type URI for a contact's home page.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcWebsite">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_WEBSITE_HOME_PAGE "home-page"

/**
 * GDATA_GCONTACT_WEBSITE_BLOG:
 *
 * The relation type URI for a contact's blog.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcWebsite">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_WEBSITE_BLOG "blog"

/**
 * GDATA_GCONTACT_WEBSITE_PROFILE:
 *
 * The relation type URI for a contact's online profile.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcWebsite">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_WEBSITE_PROFILE "profile"

/**
 * GDATA_GCONTACT_WEBSITE_HOME:
 *
 * The relation type URI for a contact's home website.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcWebsite">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_WEBSITE_HOME "home"

/**
 * GDATA_GCONTACT_WEBSITE_WORK:
 *
 * The relation type URI for a contact's work website.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcWebsite">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_WEBSITE_WORK "work"

/**
 * GDATA_GCONTACT_WEBSITE_OTHER:
 *
 * The relation type URI for a miscellaneous website of the contact.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcWebsite">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_WEBSITE_OTHER "other"

/**
 * GDATA_GCONTACT_WEBSITE_FTP:
 *
 * The relation type URI for a contact's FTP site.
 *
 * For more information, see the <ulink type="http" url="http://code.google.com/apis/contacts/docs/3.0/reference.html#gcWebsite">
 * gContact specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_GCONTACT_WEBSITE_FTP "ftp"

#define GDATA_TYPE_GCONTACT_WEBSITE		(gdata_gcontact_website_get_type ())
#define GDATA_GCONTACT_WEBSITE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_GCONTACT_WEBSITE, GDataGContactWebsite))
#define GDATA_GCONTACT_WEBSITE_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_GCONTACT_WEBSITE, GDataGContactWebsiteClass))
#define GDATA_IS_GCONTACT_WEBSITE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_GCONTACT_WEBSITE))
#define GDATA_IS_GCONTACT_WEBSITE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_GCONTACT_WEBSITE))
#define GDATA_GCONTACT_WEBSITE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_GCONTACT_WEBSITE, GDataGContactWebsiteClass))

typedef struct _GDataGContactWebsitePrivate	GDataGContactWebsitePrivate;

/**
 * GDataGContactWebsite:
 *
 * All the fields in the #GDataGContactWebsite structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	GDataParsable parent;
	GDataGContactWebsitePrivate *priv;
} GDataGContactWebsite;

/**
 * GDataGContactWebsiteClass:
 *
 * All the fields in the #GDataGContactWebsiteClass structure are private and should never be accessed directly.
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
} GDataGContactWebsiteClass;

GType gdata_gcontact_website_get_type (void) G_GNUC_CONST;

GDataGContactWebsite *gdata_gcontact_website_new (const gchar *uri, const gchar *relation_type,
                                                  const gchar *label, gboolean is_primary) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_gcontact_website_get_uri (GDataGContactWebsite *self) G_GNUC_PURE;
void gdata_gcontact_website_set_uri (GDataGContactWebsite *self, const gchar *uri);

const gchar *gdata_gcontact_website_get_relation_type (GDataGContactWebsite *self) G_GNUC_PURE;
void gdata_gcontact_website_set_relation_type (GDataGContactWebsite *self, const gchar *relation_type);

const gchar *gdata_gcontact_website_get_label (GDataGContactWebsite *self) G_GNUC_PURE;
void gdata_gcontact_website_set_label (GDataGContactWebsite *self, const gchar *label);

gboolean gdata_gcontact_website_is_primary (GDataGContactWebsite *self) G_GNUC_PURE;
void gdata_gcontact_website_set_is_primary (GDataGContactWebsite *self, gboolean is_primary);

G_END_DECLS

#endif /* !GDATA_GCONTACT_WEBSITE_H */
