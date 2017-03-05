/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) 2014 Carlos Garnacho <carlosg@gnome.org>
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

#ifndef GDATA_FREEBASE_TOPIC_QUERY_H
#define GDATA_FREEBASE_TOPIC_QUERY_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-query.h>
#include <gdata/gdata-types.h>

G_BEGIN_DECLS

#ifndef LIBGDATA_DISABLE_DEPRECATED

#define GDATA_TYPE_FREEBASE_TOPIC_QUERY			(gdata_freebase_topic_query_get_type ())
#define GDATA_FREEBASE_TOPIC_QUERY(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_FREEBASE_TOPIC_QUERY, GDataFreebaseTopicQuery))
#define GDATA_FREEBASE_TOPIC_QUERY_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_FREEBASE_TOPIC_QUERY, GDataFreebaseTopicQueryClass))
#define GDATA_IS_FREEBASE_TOPIC_QUERY(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_FREEBASE_TOPIC_QUERY))
#define GDATA_IS_FREEBASE_TOPIC_QUERY_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_FREEBASE_TOPIC_QUERY))
#define GDATA_FREEBASE_TOPIC_QUERY_GET_CLASS(o)		(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_FREEBASE_TOPIC_QUERY, GDataFreebaseTopicQueryClass))

typedef struct _GDataFreebaseTopicQueryPrivate	GDataFreebaseTopicQueryPrivate;

/**
 * GDataFreebaseTopicQuery:
 *
 * All the fields in the #GDataFreebaseTopicQuery structure are private and should never be accessed directly.
 *
 * Since: 0.15.1
 */
typedef struct {
	GDataQuery parent;
	GDataFreebaseTopicQueryPrivate *priv;
} GDataFreebaseTopicQuery;

/**
 * GDataFreebaseTopicQueryClass:
 *
 * All the fields in the #GDataFreebaseTopicQueryClass structure are private and should never be accessed directly.
 *
 * Since: 0.15.1
 */
typedef struct {
	/*< private >*/
	GDataQueryClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataFreebaseTopicQueryClass;

GType gdata_freebase_topic_query_get_type (void) G_GNUC_CONST G_GNUC_DEPRECATED;

GDataFreebaseTopicQuery *gdata_freebase_topic_query_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC G_GNUC_DEPRECATED;
void gdata_freebase_topic_query_set_language (GDataFreebaseTopicQuery *self, const gchar *lang) G_GNUC_DEPRECATED;
const gchar *gdata_freebase_topic_query_get_language (GDataFreebaseTopicQuery *self) G_GNUC_DEPRECATED;

void gdata_freebase_topic_query_set_filter (GDataFreebaseTopicQuery *self, const gchar * const *filter) G_GNUC_DEPRECATED;
const gchar * const *gdata_freebase_topic_query_get_filter (GDataFreebaseTopicQuery *self) G_GNUC_DEPRECATED;

#endif /* !LIBGDATA_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* !GDATA_FREEBASE_TOPIC_QUERY_H */
