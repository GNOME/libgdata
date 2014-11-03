/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Richard Schwarting 2010 <aquarichy@gmail.com>
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

#ifndef GDATA_GD_FEED_LINK_H
#define GDATA_GD_FEED_LINK_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

#define GDATA_TYPE_GD_FEED_LINK		(gdata_gd_feed_link_get_type ())
#define GDATA_GD_FEED_LINK(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_GD_FEED_LINK, GDataGDFeedLink))
#define GDATA_GD_FEED_LINK_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_GD_FEED_LINK, GDataGDFeedLinkClass))
#define GDATA_IS_GD_FEED_LINK(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_GD_FEED_LINK))
#define GDATA_IS_GD_FEED_LINK_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_GD_FEED_LINK))
#define GDATA_GD_FEED_LINK_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_GD_FEED_LINK, GDataGDFeedLinkClass))

typedef struct _GDataGDFeedLinkPrivate	GDataGDFeedLinkPrivate;

/**
 * GDataGDFeedLink:
 *
 * All the fields in the #GDataGDFeedLink structure are private and should never be accessed directly.
 *
 * Since: 0.10.0
 */
typedef struct {
	GDataParsable parent;
	GDataGDFeedLinkPrivate *priv;
} GDataGDFeedLink;

/**
 * GDataGDFeedLinkClass:
 *
 * All the fields in the #GDataGDFeedLinkClass structure are private and should never be accessed directly.
 *
 * Since: 0.10.0
 */
typedef struct {
	/*< private >*/
	GDataParsableClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataGDFeedLinkClass;

GType gdata_gd_feed_link_get_type (void) G_GNUC_CONST;

const gchar *gdata_gd_feed_link_get_relation_type (GDataGDFeedLink *self) G_GNUC_PURE;
void gdata_gd_feed_link_set_relation_type (GDataGDFeedLink *self, const gchar *relation_type);

const gchar *gdata_gd_feed_link_get_uri (GDataGDFeedLink *self) G_GNUC_PURE;
void gdata_gd_feed_link_set_uri (GDataGDFeedLink *self, const gchar *uri);

gint gdata_gd_feed_link_get_count_hint (GDataGDFeedLink *self) G_GNUC_PURE;
void gdata_gd_feed_link_set_count_hint (GDataGDFeedLink *self, gint count_hint);

gboolean gdata_gd_feed_link_is_read_only (GDataGDFeedLink *self) G_GNUC_PURE;
void gdata_gd_feed_link_set_is_read_only (GDataGDFeedLink *self, gboolean is_read_only);

G_END_DECLS

#endif /* !GDATA_GD_FEED_LINK_H */
