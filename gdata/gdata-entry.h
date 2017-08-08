/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2008–2010 <philip@tecnocode.co.uk>
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

#ifndef GDATA_ENTRY_H
#define GDATA_ENTRY_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>
#include <gdata/atom/gdata-category.h>
#include <gdata/atom/gdata-link.h>
#include <gdata/atom/gdata-author.h>

G_BEGIN_DECLS

#define GDATA_TYPE_ENTRY		(gdata_entry_get_type ())
#define GDATA_ENTRY(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_ENTRY, GDataEntry))
#define GDATA_ENTRY_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_ENTRY, GDataEntryClass))
#define GDATA_IS_ENTRY(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_ENTRY))
#define GDATA_IS_ENTRY_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_ENTRY))
#define GDATA_ENTRY_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_ENTRY, GDataEntryClass))

typedef struct _GDataEntryPrivate	GDataEntryPrivate;

/**
 * GDataEntry:
 *
 * All the fields in the #GDataEntry structure are private and should never be accessed directly.
 */
typedef struct {
	GDataParsable parent;
	GDataEntryPrivate *priv;
} GDataEntry;

/**
 * GDataEntryClass:
 * @parent: the parent class
 * @get_entry_uri: a function to build the entry URI for the entry, given its entry ID; free the URI with g_free()
 * @kind_term: the term for this entry's kind category (see the
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#Introduction">documentation on kinds</ulink>)
 *
 * The class structure for the #GDataEntry type.
 */
typedef struct {
	GDataParsableClass parent;

	gchar *(*get_entry_uri) (const gchar *id); /* G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC */
	const gchar *kind_term;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
	void (*_g_reserved2) (void);
	void (*_g_reserved3) (void);
	void (*_g_reserved4) (void);
	void (*_g_reserved5) (void);
	void (*_g_reserved6) (void);
	void (*_g_reserved7) (void);
} GDataEntryClass;

GType gdata_entry_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataEntry, g_object_unref)

GDataEntry *gdata_entry_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_entry_get_title (GDataEntry *self) G_GNUC_PURE;
void gdata_entry_set_title (GDataEntry *self, const gchar *title);
const gchar *gdata_entry_get_summary (GDataEntry *self) G_GNUC_PURE;
void gdata_entry_set_summary (GDataEntry *self, const gchar *summary);
const gchar *gdata_entry_get_id (GDataEntry *self) G_GNUC_PURE;
const gchar *gdata_entry_get_etag (GDataEntry *self) G_GNUC_PURE;
gint64 gdata_entry_get_updated (GDataEntry *self);
gint64 gdata_entry_get_published (GDataEntry *self);
void gdata_entry_add_category (GDataEntry *self, GDataCategory *category);
GList *gdata_entry_get_categories (GDataEntry *self) G_GNUC_PURE;
const gchar *gdata_entry_get_content (GDataEntry *self) G_GNUC_PURE;
void gdata_entry_set_content (GDataEntry *self, const gchar *content);
const gchar *gdata_entry_get_content_uri (GDataEntry *self) G_GNUC_PURE;
void gdata_entry_set_content_uri (GDataEntry *self, const gchar *content_uri);
void gdata_entry_add_link (GDataEntry *self, GDataLink *_link);
gboolean gdata_entry_remove_link (GDataEntry *self, GDataLink *_link);
GDataLink *gdata_entry_look_up_link (GDataEntry *self, const gchar *rel) G_GNUC_PURE;
GList *gdata_entry_look_up_links (GDataEntry *self, const gchar *rel) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_entry_add_author (GDataEntry *self, GDataAuthor *author);
GList *gdata_entry_get_authors (GDataEntry *self) G_GNUC_PURE;
const gchar *gdata_entry_get_rights (GDataEntry *self) G_GNUC_PURE;
void gdata_entry_set_rights (GDataEntry *self, const gchar *rights);

gboolean gdata_entry_is_inserted (GDataEntry *self) G_GNUC_PURE;

G_END_DECLS

#endif /* !GDATA_ENTRY_H */
