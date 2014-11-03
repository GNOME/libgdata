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

#ifndef GDATA_TASKS_SERVICE_H
#define GDATA_TASKS_SERVICE_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-service.h>
#include <gdata/gdata-query.h>
#include <gdata/services/tasks/gdata-tasks-tasklist.h>
#include <gdata/services/tasks/gdata-tasks-task.h>

G_BEGIN_DECLS

#define GDATA_TYPE_TASKS_SERVICE		(gdata_tasks_service_get_type ())
#define GDATA_TASKS_SERVICE(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_TASKS_SERVICE, GDataTasksService))
#define GDATA_TASKS_SERVICE_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_TASKS_SERVICE, GDataTasksServiceClass))
#define GDATA_IS_TASKS_SERVICE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_TASKS_SERVICE))
#define GDATA_IS_TASKS_SERVICE_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_TASKS_SERVICE))
#define GDATA_TASKS_SERVICE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_TASKS_SERVICE, GDataTasksServiceClass))

/**
 * GDataTasksService:
 *
 * All the fields in the #GDataTasksService structure are private and should never be accessed directly.
 *
 * Since: 0.15.0
 */
typedef struct {
	GDataService parent;
} GDataTasksService;

/**
 * GDataTasksServiceClass:
 *
 * All the fields in the #GDataTasksServiceClass structure are private and should never be accessed directly.
 *
 * Since: 0.15.0
 */
typedef struct {
	/*< private >*/
	GDataServiceClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataTasksServiceClass;

GType gdata_tasks_service_get_type (void) G_GNUC_CONST;

GDataTasksService *gdata_tasks_service_new (GDataAuthorizer *authorizer) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataAuthorizationDomain *gdata_tasks_service_get_primary_authorization_domain (void) G_GNUC_CONST;

GDataFeed *gdata_tasks_service_query_all_tasklists (GDataTasksService *self, GDataQuery *query, GCancellable *cancellable,
                                                    GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                    GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_tasks_service_query_all_tasklists_async (GDataTasksService *self, GDataQuery *query, GCancellable *cancellable,
                                                    GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                    GDestroyNotify destroy_progress_user_data, GAsyncReadyCallback callback, gpointer user_data);

GDataFeed *gdata_tasks_service_query_tasks (GDataTasksService *self, GDataTasksTasklist *tasklist, GDataQuery *query,
                                            GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                            GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_tasks_service_query_tasks_async (GDataTasksService *self, GDataTasksTasklist *tasklist, GDataQuery *query,
                                            GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                            GDestroyNotify destroy_progress_user_data, GAsyncReadyCallback callback, gpointer user_data);

GDataTasksTask *gdata_tasks_service_insert_task (GDataTasksService *self, GDataTasksTask *task, GDataTasksTasklist *tasklist,
                                                 GCancellable *cancellable, GError **error) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
void gdata_tasks_service_insert_task_async (GDataTasksService *self, GDataTasksTask *task, GDataTasksTasklist *tasklist,
                                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
GDataTasksTasklist *gdata_tasks_service_insert_tasklist (GDataTasksService *self, GDataTasksTasklist *tasklist,
                                                         GCancellable *cancellable, GError **error) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
void gdata_tasks_service_insert_tasklist_async (GDataTasksService *self, GDataTasksTasklist *tasklist,
                                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
gboolean gdata_tasks_service_delete_task (GDataTasksService *self, GDataTasksTask *task,
                                          GCancellable *cancellable, GError **error);
void gdata_tasks_service_delete_task_async (GDataTasksService *self, GDataTasksTask *task,
                                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
gboolean gdata_tasks_service_delete_tasklist (GDataTasksService *self, GDataTasksTasklist *tasklist,
                                              GCancellable *cancellable, GError **error);
void gdata_tasks_service_delete_tasklist_async (GDataTasksService *self, GDataTasksTasklist *tasklist,
                                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
GDataTasksTask *gdata_tasks_service_update_task (GDataTasksService *self, GDataTasksTask *task,
                                                 GCancellable *cancellable, GError **error) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
void gdata_tasks_service_update_task_async (GDataTasksService *self, GDataTasksTask *task,
                                            GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
GDataTasksTasklist *gdata_tasks_service_update_tasklist (GDataTasksService *self, GDataTasksTasklist *tasklist,
                                                         GCancellable *cancellable, GError **error) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;
void gdata_tasks_service_update_tasklist_async (GDataTasksService *self, GDataTasksTasklist *tasklist,
                                                GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);

G_END_DECLS

#endif /* !GDATA_TASKS_SERVICE_H */
