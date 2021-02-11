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

#ifndef GDATA_QUERY_H
#define GDATA_QUERY_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GDATA_TYPE_QUERY		(gdata_query_get_type ())
#define GDATA_QUERY(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_QUERY, GDataQuery))
#define GDATA_QUERY_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_QUERY, GDataQueryClass))
#define GDATA_IS_QUERY(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_QUERY))
#define GDATA_IS_QUERY_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_QUERY))
#define GDATA_QUERY_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_QUERY, GDataQueryClass))

typedef struct _GDataQueryPrivate	GDataQueryPrivate;

/**
 * GDataQuery:
 *
 * All the fields in the #GDataQuery structure are private and should never be accessed directly.
 */
typedef struct {
	GObject parent;
	GDataQueryPrivate *priv;
} GDataQuery;

/**
 * GDataQueryClass:
 *
 * All the fields in the #GDataQueryClass structure are private and should never be accessed directly.
 */
typedef struct {
	/*< private >*/
	GObjectClass parent;

	void (*get_query_uri) (GDataQuery *self, const gchar *feed_uri, GString *query_uri, gboolean *params_started);

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
	void (*_g_reserved2) (void);
	void (*_g_reserved3) (void);
	void (*_g_reserved4) (void);
	void (*_g_reserved5) (void);
} GDataQueryClass;

GType gdata_query_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataQuery, g_object_unref)

GDataQuery *gdata_query_new (const gchar *q) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
GDataQuery *gdata_query_new_with_limits (const gchar *q, guint start_index, guint max_results) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

gchar *gdata_query_get_query_uri (GDataQuery *self, const gchar *feed_uri) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_query_next_page (GDataQuery *self);
gboolean gdata_query_previous_page (GDataQuery *self);

const gchar *gdata_query_get_q (GDataQuery *self) G_GNUC_PURE;
void gdata_query_set_q (GDataQuery *self, const gchar *q);
const gchar *gdata_query_get_categories (GDataQuery *self) G_GNUC_PURE;
void gdata_query_set_categories (GDataQuery *self, const gchar *categories);
const gchar *gdata_query_get_author (GDataQuery *self) G_GNUC_PURE;
void gdata_query_set_author (GDataQuery *self, const gchar *author);
gint64 gdata_query_get_updated_min (GDataQuery *self);
void gdata_query_set_updated_min (GDataQuery *self, gint64 updated_min);
gint64 gdata_query_get_updated_max (GDataQuery *self);
void gdata_query_set_updated_max (GDataQuery *self, gint64 updated_max);
gint64 gdata_query_get_published_min (GDataQuery *self);
void gdata_query_set_published_min (GDataQuery *self, gint64 published_min);
gint64 gdata_query_get_published_max (GDataQuery *self);
void gdata_query_set_published_max (GDataQuery *self, gint64 published_max);
guint gdata_query_get_start_index (GDataQuery *self) G_GNUC_PURE;
void gdata_query_set_start_index (GDataQuery *self, guint start_index);
gboolean gdata_query_is_strict (GDataQuery *self) G_GNUC_PURE;
void gdata_query_set_is_strict (GDataQuery *self, gboolean is_strict);
guint gdata_query_get_max_results (GDataQuery *self) G_GNUC_PURE;
void gdata_query_set_max_results (GDataQuery *self, guint max_results);
const gchar *gdata_query_get_etag (GDataQuery *self) G_GNUC_PURE;
void gdata_query_set_etag (GDataQuery *self, const gchar *etag);

G_END_DECLS

#endif /* !GDATA_QUERY_H */
