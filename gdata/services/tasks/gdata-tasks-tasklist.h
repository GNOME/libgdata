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

#ifndef GDATA_TASKS_TASKLIST_H
#define GDATA_TASKS_TASKLIST_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-entry.h>
#include <gdata/gdata-types.h>

G_BEGIN_DECLS

#define GDATA_TYPE_TASKS_TASKLIST		(gdata_tasks_tasklist_get_type ())
#define GDATA_TASKS_TASKLIST(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_TASKS_TASKLIST, GDataTasksTasklist))
#define GDATA_TASKS_TASKLIST_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_TASKS_TASKLIST, GDataTasksTasklistClass))
#define GDATA_IS_TASKS_TASKLIST(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_TASKS_TASKLIST))
#define GDATA_IS_TASKS_TASKLIST_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_TASKS_TASKLIST))
#define GDATA_TASKS_TASKLIST_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_TASKS_TASKLIST, GDataTasksTasklistClass))

/**
 * GDataTasksTasklist:
 *
 * All the fields in the #GDataTasksTasklist structure are private and should never be accessed directly.
 *
 * Since: 0.15.0
 */
typedef struct {
	GDataEntry parent;
} GDataTasksTasklist;

/**
 * GDataTasksTasklistClass:
 *
 * All the fields in the #GDataTasksTasklistClass structure are private and should never be accessed directly.
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
} GDataTasksTasklistClass;

GType gdata_tasks_tasklist_get_type (void) G_GNUC_CONST;

GDataTasksTasklist *gdata_tasks_tasklist_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_TASKS_TASKLIST_H */
