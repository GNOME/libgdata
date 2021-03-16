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

/**
 * SECTION:gdata-tasks-task
 * @short_description: GData Tasks task object
 * @stability: Stable
 * @include: gdata/services/tasks/gdata-tasks-task.h
 *
 * #GDataTasksTask is a subclass of #GDataEntry to represent a task in a tasklist from Google Tasks.
 *
 * All functionality of Tasks is currently supported except
 * <ulink type="http" url="https://developers.google.com/google-apps/tasks/v1/reference/tasks#links">links</ulink>.
 *
 * For more details of Google Tasks API, see the <ulink type="http" url="https://developers.google.com/google-apps/tasks/v1/reference/">
 * online documentation</ulink>.
 *
 * Since: 0.15.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-tasks-task.h"
#include "gdata-private.h"
#include "gdata-service.h"
#include "gdata-parser.h"
#include "gdata-types.h"
#include "gdata-comparable.h"

static void gdata_tasks_task_finalize (GObject *object);
static void gdata_tasks_task_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_tasks_task_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void get_json (GDataParsable *parsable, JsonBuilder *builder);
static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);
static const gchar *get_content_type (void);

struct _GDataTasksTaskPrivate {
	gchar *parent;
	gchar *position;
	gchar *notes;
	gchar *status;
	gint64 due;
	gint64 completed;
	gboolean deleted;
	gboolean hidden;
};

enum {
	PROP_PARENT = 1,
	PROP_POSITION,
	PROP_NOTES,
	PROP_STATUS,
	PROP_DUE,
	PROP_COMPLETED,
	PROP_DELETED,
	PROP_HIDDEN,
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataTasksTask, gdata_tasks_task, GDATA_TYPE_ENTRY)

static void
gdata_tasks_task_class_init (GDataTasksTaskClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	gobject_class->get_property = gdata_tasks_task_get_property;
	gobject_class->set_property = gdata_tasks_task_set_property;
	gobject_class->finalize = gdata_tasks_task_finalize;

	parsable_class->parse_json = parse_json;
	parsable_class->get_json = get_json;
	parsable_class->get_content_type = get_content_type;

	entry_class->kind_term = "tasks#task";

	/**
	 * GDataTasksTask:parent:
	 *
	 * Parent task identifier. This field is omitted if it is a top-level task.
	 *
	 * Since 0.17.10, this property is writable.
	 *
	 * Since: 0.15.0
	 */
	g_object_class_install_property (gobject_class, PROP_PARENT,
	                                 g_param_spec_string ("parent",
	                                 "Parent of task", "Identifier of parent task.",
	                                 NULL,
	                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataTasksTask:position:
	 *
	 * String indicating the position of the task among its sibling tasks under the same parent task
	 * or at the top level. If this string is greater than another task's corresponding position string
	 * according to lexicographical ordering, the task is positioned after the other task under the same
	 * parent task (or at the top level).
	 *
	 * Since 0.17.10, this property is writable.
	 *
	 * Since: 0.15.0
	 */
	g_object_class_install_property (gobject_class, PROP_POSITION,
	                                 g_param_spec_string ("position",
	                                 "Position of task", "Position of the task among sibling tasks using lexicographical order.",
	                                 NULL,
	                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataTasksTask:notes:
	 *
	 * This is where the description of what needs to be done in the task is stored.
	 *
	 * Since: 0.15.0
	 */
	g_object_class_install_property (gobject_class, PROP_NOTES,
	                                 g_param_spec_string ("notes",
	                                 "Notes of task", "Notes describing the task.",
	                                 NULL,
	                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataTasksTask:status:
	 *
	 * Status of the task. This is either %GDATA_TASKS_STATUS_NEEDS_ACTION
	 * or %GDATA_TASKS_STATUS_COMPLETED.
	 *
	 * Since: 0.15.0
	 */
	g_object_class_install_property (gobject_class, PROP_STATUS,
	                                 g_param_spec_string ("status",
	                                 "Status of task", "Status of the task.",
	                                 NULL,
	                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataTasksTask:due:
	 *
	 * Due date of the task (as a RFC 3339 timestamp; seconds since the UNIX
	 * epoch).
	 *
	 * This field is <code class="literal">-1</code> if the task has no due
	 * date assigned.
	 *
	 * Since: 0.15.0
	 */
	g_object_class_install_property (gobject_class, PROP_DUE,
	                                 g_param_spec_int64 ("due",
	                                 "Due date of the task", "Due date of the task.",
	                                 -1, G_MAXINT64, -1,
	                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataTasksTask:completed:
	 *
	 * Completion date of the task (as a RFC 3339 timestamp; seconds since
	 * the UNIX epoch).
	 *
	 * This field is <code class="literal">-1</code> if the task has not
	 * been completed.
	 *
	 * Since: 0.15.0
	 */
	g_object_class_install_property (gobject_class, PROP_COMPLETED,
	                                 g_param_spec_int64 ("completed",
	                                 "Completion date of task", "Completion date of the task.",
	                                 -1, G_MAXINT64, -1,
	                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataTasksTask:is-deleted:
	 *
	 * Flag indicating whether the task has been deleted. The default is %FALSE.
	 *
	 * Since: 0.15.0
	 */
	g_object_class_install_property (gobject_class, PROP_DELETED,
	                                 g_param_spec_boolean ("is-deleted",
	                                 "Deleted?", "Indicated whatever task is deleted.",
	                                 FALSE,
	                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataTasksTask:is-hidden:
	 *
	 * Flag indicating whether the task is hidden. This is the case if the task
	 * had been marked completed when the task list was last cleared.
	 * The default is %FALSE. This field is read-only.
	 *
	 * Since: 0.15.0
	 */
	g_object_class_install_property (gobject_class, PROP_HIDDEN,
	                                 g_param_spec_boolean ("is-hidden",
	                                 "Hidden?", "Indicated whatever task is hidden.",
	                                 FALSE,
	                                 G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_tasks_task_init (GDataTasksTask *self)
{
	self->priv = gdata_tasks_task_get_instance_private (self);
	self->priv->due = -1;
	self->priv->completed = -1;
}

static void
gdata_tasks_task_finalize (GObject *object)
{
	GDataTasksTaskPrivate *priv = GDATA_TASKS_TASK (object)->priv;

	g_free (priv->parent);
	g_free (priv->position);
	g_free (priv->notes);
	g_free (priv->status);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_tasks_task_parent_class)->finalize (object);
}

static void
gdata_tasks_task_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataTasksTaskPrivate *priv = GDATA_TASKS_TASK (object)->priv;

	switch (property_id) {
		case PROP_PARENT:
			g_value_set_string (value, priv->parent);
			break;
		case PROP_POSITION:
			g_value_set_string (value, priv->position);
			break;
		case PROP_NOTES:
			g_value_set_string (value, priv->notes);
			break;
		case PROP_STATUS:
			g_value_set_string (value, priv->status);
			break;
		case PROP_DUE:
			g_value_set_int64 (value, priv->due);
			break;
		case PROP_COMPLETED:
			g_value_set_int64 (value, priv->completed);
			break;
		case PROP_DELETED:
			g_value_set_boolean (value, priv->deleted);
			break;
		case PROP_HIDDEN:
			g_value_set_boolean (value, priv->hidden);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_tasks_task_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataTasksTask *self = GDATA_TASKS_TASK (object);

	switch (property_id) {
		case PROP_NOTES:
			gdata_tasks_task_set_notes (self, g_value_get_string (value));
			break;
		case PROP_STATUS:
			gdata_tasks_task_set_status (self, g_value_get_string (value));
			break;
		case PROP_DUE:
			gdata_tasks_task_set_due (self, g_value_get_int64 (value));
			break;
		case PROP_COMPLETED:
			gdata_tasks_task_set_completed (self, g_value_get_int64 (value));
			break;
		case PROP_DELETED:
			gdata_tasks_task_set_is_deleted (self, g_value_get_boolean (value));
			break;
		case PROP_PARENT:
			gdata_tasks_task_set_parent (self, g_value_get_string (value));
			break;
		case PROP_POSITION:
			gdata_tasks_task_set_position (self, g_value_get_string (value));
			break;
		case PROP_HIDDEN:
		/* Read-only */
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	gboolean success;
	GDataTasksTask *self = GDATA_TASKS_TASK (parsable);

	if (gdata_parser_string_from_json_member (reader, "parent", P_DEFAULT, &(self->priv->parent), &success, error) == TRUE ||
	    gdata_parser_string_from_json_member (reader, "position", P_DEFAULT, &(self->priv->position), &success, error) == TRUE ||
	    gdata_parser_string_from_json_member (reader, "notes", P_DEFAULT, &(self->priv->notes), &success, error) == TRUE ||
	    gdata_parser_string_from_json_member (reader, "status", P_DEFAULT, &(self->priv->status), &success, error) == TRUE ||
	    gdata_parser_int64_time_from_json_member (reader, "due", P_DEFAULT, &(self->priv->due), &success, error) == TRUE ||
	    gdata_parser_int64_time_from_json_member (reader, "completed", P_DEFAULT, &(self->priv->completed), &success, error) == TRUE ||
	    gdata_parser_boolean_from_json_member (reader, "deleted", P_DEFAULT, &(self->priv->deleted), &success, error) == TRUE ||
	    gdata_parser_boolean_from_json_member (reader, "hidden", P_DEFAULT, &(self->priv->hidden), &success, error) == TRUE) {
		return success;
	} else {
		return GDATA_PARSABLE_CLASS (gdata_tasks_task_parent_class)->parse_json (parsable, reader, user_data, error);
	}

	return TRUE;
}

static void
get_json (GDataParsable *parsable, JsonBuilder *builder)
{
	gchar *due;
	gchar *completed;

	GDataTasksTaskPrivate *priv = GDATA_TASKS_TASK (parsable)->priv;

	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_tasks_task_parent_class)->get_json (parsable, builder);

	/* Add all the task specific JSON */

	if (priv->parent != NULL) {
		json_builder_set_member_name (builder, "parent");
		json_builder_add_string_value (builder, priv->parent);
	}
	if (priv->position != NULL) {
		json_builder_set_member_name (builder, "position");
		json_builder_add_string_value (builder, priv->position);
	}
	if (priv->notes != NULL) {
		json_builder_set_member_name (builder, "notes");
		json_builder_add_string_value (builder, priv->notes);
	}
	if (priv->status != NULL) {
		json_builder_set_member_name (builder, "status");
		json_builder_add_string_value (builder, priv->status);
	}
	if (priv->due != -1) {
		due = gdata_parser_int64_to_iso8601 (priv->due);
		json_builder_set_member_name (builder, "due");
		json_builder_add_string_value (builder, due);
		g_free (due);
	}
	if (priv->completed != -1) {
		completed = gdata_parser_int64_to_iso8601 (priv->completed);
		json_builder_set_member_name (builder, "completed");
		json_builder_add_string_value (builder, completed);
		g_free (completed);
	}

	json_builder_set_member_name (builder, "deleted");
	json_builder_add_boolean_value (builder, priv->deleted);

	json_builder_set_member_name (builder, "hidden");
	json_builder_add_boolean_value (builder, priv->hidden);
}

static const gchar *
get_content_type (void)
{
	return "application/json";
}

/**
 * gdata_tasks_task_new:
 * @id: (allow-none): the task's ID, or %NULL
 *
 * Creates a new #GDataTasksTask with the given ID and default properties.
 *
 * Return value: a new #GDataTasksTask; unref with g_object_unref()
 *
 * Since: 0.15.0
 */
GDataTasksTask *
gdata_tasks_task_new (const gchar *id)
{
	return GDATA_TASKS_TASK (g_object_new (GDATA_TYPE_TASKS_TASK, "id", id, NULL));
}

/**
 * gdata_tasks_task_get_parent:
 * @self: a #GDataTasksTask
 *
 * Gets the #GDataTasksTask:parent property.
 *
 * Return value: (allow-none): the parent of the task, or %NULL
 *
 * Since: 0.15.0
 */
const gchar *
gdata_tasks_task_get_parent (GDataTasksTask *self)
{
	g_return_val_if_fail (GDATA_IS_TASKS_TASK (self), NULL);
	return self->priv->parent;
}

/**
 * gdata_tasks_task_set_parent:
 * @self: a #GDataTasksTask
 * @parent: (nullable): parent of the task
 *
 * Sets the #GDataTasksTask:parent property.
 *
 * Since: 0.17.10
 */
void
gdata_tasks_task_set_parent (GDataTasksTask *self, const gchar *parent)
{
	gchar *local_parent;
	g_return_if_fail (GDATA_IS_TASKS_TASK (self));

	if (g_strcmp0 (self->priv->parent, parent) == 0)
		return;

	local_parent = self->priv->parent;
	self->priv->parent = g_strdup (parent);
	g_free (local_parent);
	g_object_notify (G_OBJECT (self), "parent");
}

/**
 * gdata_tasks_task_get_position:
 * @self: a #GDataTasksTask
 *
 * Gets the #GDataTasksTask:position property.
 *
 * Return value: (allow-none): the position of the task, or %NULL
 *
 * Since: 0.15.0
 */
const gchar *
gdata_tasks_task_get_position (GDataTasksTask *self)
{
	g_return_val_if_fail (GDATA_IS_TASKS_TASK (self), NULL);
	return self->priv->position;
}

/**
 * gdata_tasks_task_set_position:
 * @self: a #GDataTasksTask
 * @position: position of the task in the list
 *
 * Sets the #GDataTasksTask:position property.
 *
 * Since: 0.17.10
 */
void
gdata_tasks_task_set_position (GDataTasksTask *self, const gchar *position)
{
	gchar *local_position;
	g_return_if_fail (GDATA_IS_TASKS_TASK (self));

	if (g_strcmp0 (self->priv->position, position) == 0)
		return;

	local_position = self->priv->position;
	self->priv->position = g_strdup (position);
	g_free (local_position);
	g_object_notify (G_OBJECT (self), "position");
}

/**
 * gdata_tasks_task_get_notes:
 * @self: a #GDataTasksTask
 *
 * Gets the #GDataTasksTask:notes property.
 *
 * Return value: (allow-none): notes of the task, or %NULL
 *
 * Since: 0.15.0
 */
const gchar *
gdata_tasks_task_get_notes (GDataTasksTask *self)
{
	g_return_val_if_fail (GDATA_IS_TASKS_TASK (self), NULL);
	return self->priv->notes;
}

/**
 * gdata_tasks_task_set_notes:
 * @self: a #GDataTasksTask
 * @notes: (allow-none): a new notes of the task, or %NULL
 *
 * Sets the #GDataTasksTask:notes property to the new notes, @notes.
 *
 * Set @notes to %NULL to unset the property in the task.
 *
 * Since: 0.15.0
 */
void
gdata_tasks_task_set_notes (GDataTasksTask *self, const gchar *notes)
{
	gchar *local_notes;
	g_return_if_fail (GDATA_IS_TASKS_TASK (self));

	local_notes = self->priv->notes;
	self->priv->notes = g_strdup (notes);
	g_free (local_notes);
	g_object_notify (G_OBJECT (self), "notes");
}

/**
 * gdata_tasks_task_get_status:
 * @self: a #GDataTasksTask
 *
 * Gets the #GDataTasksTask:status property.
 *
 * Return value: (allow-none): the status of the task, or %NULL
 *
 * Since: 0.15.0
 */
const gchar *
gdata_tasks_task_get_status (GDataTasksTask *self)
{
	g_return_val_if_fail (GDATA_IS_TASKS_TASK (self), NULL);
	return self->priv->status;
}

/**
 * gdata_tasks_task_set_status:
 * @self: a #GDataTasksTask
 * @status: (allow-none): a new status of the task, or %NULL
 *
 * Sets the #GDataTasksTask:status property to the new status, @status.
 *
 * Set @status to %NULL to unset the property in the task.
 *
 * Since: 0.15.0
 */
void
gdata_tasks_task_set_status (GDataTasksTask *self, const gchar *status)
{
	gchar *local_status;
	g_return_if_fail (GDATA_IS_TASKS_TASK (self));

	local_status = self->priv->status;
	self->priv->status = g_strdup (status);
	g_free (local_status);
	g_object_notify (G_OBJECT (self), "status");
}

/**
 * gdata_tasks_task_get_due:
 * @self: a #GDataTasksTask
 *
 * Gets the #GDataTasksTask:due property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the due property, or <code class="literal">-1</code>
 *
 * Since: 0.15.0
 */
gint64
gdata_tasks_task_get_due (GDataTasksTask *self)
{
	g_return_val_if_fail (GDATA_IS_TASKS_TASK (self), -1);
	return self->priv->due;
}

/**
 * gdata_tasks_task_set_due:
 * @self: a #GDataTasksTask
 * @due: due time of the task, or <code class="literal">-1</code>
 *
 * Sets the #GDataTasksTask:due property of the #GDataTasksTask to the new due time of the task, @due.
 *
 * Set @due to <code class="literal">-1</code> to unset the property in the due time of the task
 *
 * Since: 0.15.0
 */
void
gdata_tasks_task_set_due (GDataTasksTask *self, gint64 due)
{
	g_return_if_fail (GDATA_IS_TASKS_TASK (self));
	g_return_if_fail (due >= -1);

	self->priv->due = due;
	g_object_notify (G_OBJECT (self), "due");
}

/**
 * gdata_tasks_task_get_completed:
 * @self: a #GDataTasksTask
 *
 * Gets the #GDataTasksTask:completed property. If the property is unset, <code class="literal">-1</code> will be returned.
 *
 * Return value: the completed property, or <code class="literal">-1</code>
 *
 * Since: 0.15.0
 */
gint64
gdata_tasks_task_get_completed (GDataTasksTask *self)
{
	g_return_val_if_fail (GDATA_IS_TASKS_TASK (self), -1);
	return self->priv->completed;
}

/**
 * gdata_tasks_task_set_completed:
 * @self: a #GDataTasksTask
 * @completed: completion time of the task, or <code class="literal">-1</code>
 *
 * Sets the #GDataTasksTask:completed property of the #GDataTasksTask to the new completion time of the task, @completed.
 *
 * Set @completed to <code class="literal">-1</code> to unset the property in the completion time of the task
 *
 * Since: 0.15.0
 */
void
gdata_tasks_task_set_completed (GDataTasksTask *self, gint64 completed)
{
	g_return_if_fail (GDATA_IS_TASKS_TASK (self));
	g_return_if_fail (completed >= -1);

	self->priv->completed = completed;
	g_object_notify (G_OBJECT (self), "completed");
}

/**
 * gdata_tasks_task_is_deleted:
 * @self: a #GDataTasksTask
 *
 * Gets the #GDataTasksTask:is-deleted property.
 *
 * Return value: %TRUE if task is deleted, %FALSE otherwise
 *
 * Since: 0.15.0
 */
gboolean
gdata_tasks_task_is_deleted (GDataTasksTask *self)
{
	g_return_val_if_fail (GDATA_IS_TASKS_TASK (self), FALSE);
	return self->priv->deleted;
}

/**
 * gdata_tasks_task_set_is_deleted:
 * @self: a #GDataTasksTask
 * @deleted: %TRUE if task is deleted, %FALSE otherwise
 *
 * Sets the #GDataTasksTask:is-deleted property to @deleted.
 *
 * Since: 0.15.0
 */
void
gdata_tasks_task_set_is_deleted (GDataTasksTask *self, gboolean deleted)
{
	g_return_if_fail (GDATA_IS_TASKS_TASK (self));
	self->priv->deleted = deleted;
	g_object_notify (G_OBJECT (self), "is-deleted");
}

/**
 * gdata_tasks_task_is_hidden:
 * @self: a #GDataTasksTask
 *
 * Gets the #GDataTasksTask:is-hidden property.
 *
 * Return value: %TRUE if task is hidden, %FALSE otherwise
 *
 * Since: 0.15.0
 */
gboolean
gdata_tasks_task_is_hidden (GDataTasksTask *self)
{
	g_return_val_if_fail (GDATA_IS_TASKS_TASK (self), FALSE);
	return self->priv->hidden;
}
