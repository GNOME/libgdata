/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009 <philip@tecnocode.co.uk>
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

#ifndef GDATA_LINK_H
#define GDATA_LINK_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

/**
 * GDATA_LINK_ALTERNATE:
 *
 * The relation type URI for alternate resources to the current one.
 *
 * For more information, see the <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#rel_attribute">
 * Atom specification</ulink>.
 *
 * Since: 0.4.0
 */
#define GDATA_LINK_ALTERNATE "http://www.iana.org/assignments/relation/alternate"

/**
 * GDATA_LINK_RELATED:
 *
 * The relation type URI for resources related to the current one.
 *
 * For more information, see the <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#rel_attribute">
 * Atom specification</ulink>.
 *
 * Since: 0.4.0
 */
#define GDATA_LINK_RELATED "http://www.iana.org/assignments/relation/related"

/**
 * GDATA_LINK_SELF:
 *
 * The relation type URI for the current resource.
 *
 * For more information, see the <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#rel_attribute">
 * Atom specification</ulink>.
 *
 * Since: 0.4.0
 */
#define GDATA_LINK_SELF "http://www.iana.org/assignments/relation/self"

/**
 * GDATA_LINK_ENCLOSURE:
 *
 * The relation type URI for attached objects which may be large in size.
 *
 * For more information, see the <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#rel_attribute">
 * Atom specification</ulink>.
 *
 * Since: 0.4.0
 */
#define GDATA_LINK_ENCLOSURE "http://www.iana.org/assignments/relation/enclosure"

/**
 * GDATA_LINK_VIA:
 *
 * The relation type URI for the source document of the current resource.
 *
 * For more information, see the <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#rel_attribute">
 * Atom specification</ulink>.
 *
 * Since: 0.4.0
 */
#define GDATA_LINK_VIA "http://www.iana.org/assignments/relation/via"

/**
 * GDATA_LINK_EDIT:
 *
 * The relation type URI of the edit location for this resource.
 *
 * For more information, see the <ulink type="http" url="http://www.atomenabled.org/developers/protocol/atom-protocol-spec.php#new-link-relation">
 * Atom Publishing Protocol specification</ulink>.
 *
 * Since: 0.4.0
 */
#define GDATA_LINK_EDIT "http://www.iana.org/assignments/relation/edit"

/**
 * GDATA_LINK_EDIT_MEDIA:
 *
 * The relation type URI of the edit location for media resources attached to this resource.
 *
 * For more information, see the
 * <ulink type="http" url="http://www.atomenabled.org/developers/protocol/atom-protocol-spec.php#new-media-link-relation">
 * Atom Publishing Protocol specification</ulink>.
 *
 * Since: 0.4.0
 */
#define GDATA_LINK_EDIT_MEDIA "http://www.iana.org/assignments/relation/edit-media"

/**
 * GDATA_LINK_PARENT:
 *
 * The relation type URI of the of the location of the parent resource in a
 * hierarchy of entries.
 *
 * This is an undocumented GData-specific addition to the Atom specification,
 * and is not included in the GData documentation except in examples and in the
 * <ulink type="http" url="https://developers.google.com/google-apps/documents-list/v2/schema/document_list_atom.rnc">
 * RelaxNG schema</ulink>.
 *
 * Since: 0.15.1
 */
#define GDATA_LINK_PARENT "http://schemas.google.com/docs/2007#parent"

#define GDATA_TYPE_LINK			(gdata_link_get_type ())
#define GDATA_LINK(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_LINK, GDataLink))
#define GDATA_LINK_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_LINK, GDataLinkClass))
#define GDATA_IS_LINK(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_LINK))
#define GDATA_IS_LINK_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_LINK))
#define GDATA_LINK_GET_CLASS(o)		(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_LINK, GDataLinkClass))

typedef struct _GDataLinkPrivate	GDataLinkPrivate;

/**
 * GDataLink:
 *
 * All the fields in the #GDataLink structure are private and should never be accessed directly.
 */
typedef struct {
	GDataParsable parent;
	GDataLinkPrivate *priv;
} GDataLink;

/**
 * GDataLinkClass:
 *
 * All the fields in the #GDataLinkClass structure are private and should never be accessed directly.
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
} GDataLinkClass;

GType gdata_link_get_type (void) G_GNUC_CONST;

GDataLink *gdata_link_new (const gchar *uri, const gchar *relation_type) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_link_get_uri (GDataLink *self) G_GNUC_PURE;
void gdata_link_set_uri (GDataLink *self, const gchar *uri);

const gchar *gdata_link_get_relation_type (GDataLink *self) G_GNUC_PURE;
void gdata_link_set_relation_type (GDataLink *self, const gchar *relation_type);

const gchar *gdata_link_get_content_type (GDataLink *self) G_GNUC_PURE;
void gdata_link_set_content_type (GDataLink *self, const gchar *content_type);

const gchar *gdata_link_get_language (GDataLink *self) G_GNUC_PURE;
void gdata_link_set_language (GDataLink *self, const gchar *language);

const gchar *gdata_link_get_title (GDataLink *self) G_GNUC_PURE;
void gdata_link_set_title (GDataLink *self, const gchar *title);

gint gdata_link_get_length (GDataLink *self) G_GNUC_PURE;
void gdata_link_set_length (GDataLink *self, gint length);

G_END_DECLS

#endif /* !GDATA_LINK_H */
