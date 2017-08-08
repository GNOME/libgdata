/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2008-2009 <philip@tecnocode.co.uk>
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

#ifndef GDATA_FEED_H
#define GDATA_FEED_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>
#include <gdata/gdata-entry.h>
#include <gdata/atom/gdata-link.h>
#include <gdata/atom/gdata-generator.h>

G_BEGIN_DECLS

#define GDATA_TYPE_FEED			(gdata_feed_get_type ())
#define GDATA_FEED(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_FEED, GDataFeed))
#define GDATA_FEED_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_FEED, GDataFeedClass))
#define GDATA_IS_FEED(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_FEED))
#define GDATA_IS_FEED_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_FEED))
#define GDATA_FEED_GET_CLASS(o)		(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_FEED, GDataFeedClass))

typedef struct _GDataFeedPrivate	GDataFeedPrivate;

/**
 * GDataFeed:
 *
 * All the fields in the #GDataFeed structure are private and should never be accessed directly.
 */
typedef struct {
	/*< private >*/
	GDataParsable parent;
	GDataFeedPrivate *priv;
} GDataFeed;

/**
 * GDataFeedClass:
 *
 * All the fields in the #GDataFeedClass structure are private and should never be accessed directly.
 */
typedef struct {
	/*< private >*/
	GDataParsableClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
	void (*_g_reserved2) (void);
	void (*_g_reserved3) (void);
	void (*_g_reserved4) (void);
	void (*_g_reserved5) (void);
} GDataFeedClass;

GType gdata_feed_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataFeed, g_object_unref)

GList *gdata_feed_get_entries (GDataFeed *self) G_GNUC_PURE;
GDataEntry *gdata_feed_look_up_entry (GDataFeed *self, const gchar *id) G_GNUC_PURE;
GList *gdata_feed_get_categories (GDataFeed *self) G_GNUC_PURE;
GList *gdata_feed_get_links (GDataFeed *self) G_GNUC_PURE;
GDataLink *gdata_feed_look_up_link (GDataFeed *self, const gchar *rel) G_GNUC_PURE;
GList *gdata_feed_get_authors (GDataFeed *self) G_GNUC_PURE;

const gchar *gdata_feed_get_title (GDataFeed *self) G_GNUC_PURE;
const gchar *gdata_feed_get_subtitle (GDataFeed *self) G_GNUC_PURE;
const gchar *gdata_feed_get_id (GDataFeed *self) G_GNUC_PURE;
const gchar *gdata_feed_get_etag (GDataFeed *self) G_GNUC_PURE;
gint64 gdata_feed_get_updated (GDataFeed *self);
const gchar *gdata_feed_get_logo (GDataFeed *self) G_GNUC_PURE;
GDataGenerator *gdata_feed_get_generator (GDataFeed *self) G_GNUC_PURE;
const gchar *gdata_feed_get_rights (GDataFeed *self) G_GNUC_PURE;
guint gdata_feed_get_items_per_page (GDataFeed *self) G_GNUC_PURE;
guint gdata_feed_get_start_index (GDataFeed *self) G_GNUC_PURE;
guint gdata_feed_get_total_results (GDataFeed *self) G_GNUC_PURE;
const gchar *gdata_feed_get_icon (GDataFeed *self) G_GNUC_PURE;
const gchar *gdata_feed_get_next_page_token (GDataFeed *self) G_GNUC_PURE;

G_END_DECLS

#endif /* !GDATA_FEED_H */
