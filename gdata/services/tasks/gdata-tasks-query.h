/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Peteris Krisjanis 2013 <pecisk@gmail.com>
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

#ifndef GDATA_TASKS_QUERY_H
#define GDATA_TASKS_QUERY_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-query.h>
#include <gdata/gdata-types.h>

G_BEGIN_DECLS

#define GDATA_TYPE_TASKS_QUERY		(gdata_tasks_query_get_type ())
#define GDATA_TASKS_QUERY(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_TASKS_QUERY, GDataTasksQuery))
#define GDATA_TASKS_QUERY_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_TASKS_QUERY, GDataTasksQueryClass))
#define GDATA_IS_TASKS_QUERY(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_TASKS_QUERY))
#define GDATA_IS_TASKS_QUERY_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_TASKS_QUERY))
#define GDATA_TASKS_QUERY_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_TASKS_QUERY, GDataTasksQueryClass))

typedef struct _GDataTasksQueryPrivate	GDataTasksQueryPrivate;

/**
 * GDataTasksQuery:
 *
 * All the fields in the #GDataTasksQuery structure are private and should never be accessed directly.
 *
 * Since: 0.15.0
 */
typedef struct {
	GDataQuery parent;
	GDataTasksQueryPrivate *priv;
} GDataTasksQuery;

/**
 * GDataTasksQueryClass:
 *
 * All the fields in the #GDataTasksQueryClass structure are private and should never be accessed directly.
 *
 * Since: 0.15.0
 */
typedef struct {
	/*< private >*/
	GDataQueryClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataTasksQueryClass;

GType gdata_tasks_query_get_type (void) G_GNUC_CONST;

GDataTasksQuery *gdata_tasks_query_new (const gchar *q) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

gint64 gdata_tasks_query_get_completed_max (GDataTasksQuery *self) G_GNUC_PURE;
void gdata_tasks_query_set_completed_max (GDataTasksQuery *self, gint64 completed_max);
gint64 gdata_tasks_query_get_completed_min (GDataTasksQuery *self) G_GNUC_PURE;
void gdata_tasks_query_set_completed_min (GDataTasksQuery *self, gint64 completed_min);
gint64 gdata_tasks_query_get_due_max (GDataTasksQuery *self) G_GNUC_PURE;
void gdata_tasks_query_set_due_max (GDataTasksQuery *self, gint64 due_max);
gint64 gdata_tasks_query_get_due_min (GDataTasksQuery *self) G_GNUC_PURE;
void gdata_tasks_query_set_due_min (GDataTasksQuery *self, gint64 due_min);
gboolean gdata_tasks_query_get_show_completed (GDataTasksQuery *self) G_GNUC_PURE;
void gdata_tasks_query_set_show_completed (GDataTasksQuery *self, gboolean show_completed);
gboolean gdata_tasks_query_get_show_deleted (GDataTasksQuery *self) G_GNUC_PURE;
void gdata_tasks_query_set_show_deleted (GDataTasksQuery *self, gboolean show_deleted);
gboolean gdata_tasks_query_get_show_hidden (GDataTasksQuery *self) G_GNUC_PURE;
void gdata_tasks_query_set_show_hidden (GDataTasksQuery *self, gboolean show_hidden);

G_END_DECLS

#endif /* !GDATA_TASKS_QUERY_H */
