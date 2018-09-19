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

#ifndef GDATA_TASKS_TASK_H
#define GDATA_TASKS_TASK_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-entry.h>
#include <gdata/gdata-types.h>

G_BEGIN_DECLS

/**
 * GDATA_TASKS_STATUS_NEEDS_ACTION:
 *
 * A value for #GDataTasksTask:status indicating the task still needs action
 * before it is complete.
 *
 * Reference:
 * <ulink type="http" url="https://developers.google.com/google-apps/tasks/v1/reference/tasks#status">
 * Google Tasks documentation</ulink>.
 *
 * Since: 0.16.0
 */
#define GDATA_TASKS_STATUS_NEEDS_ACTION "needsAction"

/**
 * GDATA_TASKS_STATUS_COMPLETED:
 *
 * A value for #GDataTasksTask:status indicating the task has been completed.
 *
 * Reference:
 * <ulink type="http" url="https://developers.google.com/google-apps/tasks/v1/reference/tasks#status">
 * Google Tasks documentation</ulink>.
 *
 * Since: 0.16.0
 */
#define GDATA_TASKS_STATUS_COMPLETED "completed"

#define GDATA_TYPE_TASKS_TASK		(gdata_tasks_task_get_type ())
#define GDATA_TASKS_TASK(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_TASKS_TASK, GDataTasksTask))
#define GDATA_TASKS_TASK_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_TASKS_TASK, GDataTasksTaskClass))
#define GDATA_IS_TASKS_TASK(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_TASKS_TASK))
#define GDATA_IS_TASKS_TASK_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_TASKS_TASK))
#define GDATA_TASKS_TASK_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_TASKS_TASK, GDataTasksTaskClass))

typedef struct _GDataTasksTaskPrivate	GDataTasksTaskPrivate;

/**
 * GDataTasksTask:
 *
 * All the fields in the #GDataTasksTask structure are private and should never be accessed directly.
 *
 * Since: 0.15.0
 */
typedef struct {
	GDataEntry parent;
	GDataTasksTaskPrivate *priv;
} GDataTasksTask;

/**
 * GDataTasksTaskClass:
 *
 * All the fields in the #GDataTasksTaskClass structure are private and should never be accessed directly.
 *
 * Since: 0.15.0
 */
typedef struct {
	/*< private >*/
	GDataEntryClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataTasksTaskClass;

GType gdata_tasks_task_get_type (void) G_GNUC_CONST;

GDataTasksTask *gdata_tasks_task_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_tasks_task_get_parent (GDataTasksTask *self) G_GNUC_PURE;
void gdata_tasks_task_set_parent (GDataTasksTask *self, const gchar *parent);
const gchar *gdata_tasks_task_get_position (GDataTasksTask *self) G_GNUC_PURE;
void gdata_tasks_task_set_position (GDataTasksTask *self, const gchar *position);
const gchar *gdata_tasks_task_get_notes (GDataTasksTask *self) G_GNUC_PURE;
void gdata_tasks_task_set_notes (GDataTasksTask *self, const gchar *notes);
const gchar *gdata_tasks_task_get_status (GDataTasksTask *self) G_GNUC_PURE;
void gdata_tasks_task_set_status (GDataTasksTask *self, const gchar *status);
gint64 gdata_tasks_task_get_due (GDataTasksTask *self) G_GNUC_PURE;
void gdata_tasks_task_set_due (GDataTasksTask *self, gint64 due);
gint64 gdata_tasks_task_get_completed (GDataTasksTask *self) G_GNUC_PURE;
void gdata_tasks_task_set_completed (GDataTasksTask *self, gint64 completed);
gboolean gdata_tasks_task_is_deleted (GDataTasksTask *self) G_GNUC_PURE;
void gdata_tasks_task_set_is_deleted (GDataTasksTask *self, gboolean deleted);
gboolean gdata_tasks_task_is_hidden (GDataTasksTask *self) G_GNUC_PURE;

G_END_DECLS

#endif /* !GDATA_TASKS_TASK_H */
