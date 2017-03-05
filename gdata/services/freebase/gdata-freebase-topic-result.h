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

#ifndef GDATA_FREEBASE_TOPIC_RESULT_H
#define GDATA_FREEBASE_TOPIC_RESULT_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-types.h>
#include "gdata-freebase-result.h"

G_BEGIN_DECLS

#ifndef LIBGDATA_DISABLE_DEPRECATED

#define GDATA_TYPE_FREEBASE_TOPIC_OBJECT		(gdata_freebase_topic_object_get_type ())
#define GDATA_TYPE_FREEBASE_TOPIC_VALUE			(gdata_freebase_topic_value_get_type ())
#define GDATA_TYPE_FREEBASE_TOPIC_RESULT		(gdata_freebase_topic_result_get_type ())
#define GDATA_FREEBASE_TOPIC_RESULT(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_FREEBASE_TOPIC_RESULT, GDataFreebaseTopicResult))
#define GDATA_FREEBASE_TOPIC_RESULT_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_FREEBASE_TOPIC_RESULT, GDataFreebaseTopicResultClass))
#define GDATA_IS_FREEBASE_TOPIC_RESULT(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_FREEBASE_TOPIC_RESULT))
#define GDATA_IS_FREEBASE_TOPIC_RESULT_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_FREEBASE_TOPIC_RESULT))
#define GDATA_FREEBASE_TOPIC_RESULT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_FREEBASE_TOPIC_RESULT, GDataFreebaseTopicResultClass))

typedef struct _GDataFreebaseTopicResultPrivate GDataFreebaseTopicResultPrivate;

/**
 * GDataFreebaseTopicObject:
 *
 * Opaque struct containing a Freebase topic object. This object may contain one or more
 * #GDataFreebaseTopicValue structs, which may in turn contain nested #GDataFreebaseTopicObject
 * structs to express complex data.
 *
 * Since: 0.15.1
 */
typedef struct _GDataFreebaseTopicObject GDataFreebaseTopicObject;

/**
 * GDataFreebaseTopicValue:
 *
 * Opaque struct containing a value of a Freebase topic object. This struct may contain a simple
 * value (integers, doubles, strings...) or complex values, expressed through a #GDataFreebaseTopicObject.
 *
 * Since: 0.15.1
 */
typedef struct _GDataFreebaseTopicValue GDataFreebaseTopicValue;

/**
 * GDataFreebaseTopicResult:
 *
 * All the fields in the #GDataFreebaseTopicResult structure are private and should never be accessed directly.
 *
 * Since: 0.15.1
 */
typedef struct {
	GDataFreebaseResult parent;
	GDataFreebaseTopicResultPrivate *priv;
} GDataFreebaseTopicResult;

/**
 * GDataFreebaseTopicResultClass:
 *
 * All the fields in the #GDataFreebaseTopicResultClass structure are private and should never be accessed directly.
 *
 * Since: 0.15.1
 */
typedef struct {
	/*< private >*/
	GDataFreebaseResultClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataFreebaseTopicResultClass;

GType gdata_freebase_topic_object_get_type (void) G_GNUC_CONST G_GNUC_DEPRECATED;
GType gdata_freebase_topic_value_get_type (void) G_GNUC_CONST G_GNUC_DEPRECATED;
GType gdata_freebase_topic_result_get_type (void) G_GNUC_CONST G_GNUC_DEPRECATED;

GDataFreebaseTopicResult *gdata_freebase_topic_result_new (void) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC G_GNUC_DEPRECATED;

GDataFreebaseTopicObject *gdata_freebase_topic_result_dup_object (GDataFreebaseTopicResult *self) G_GNUC_DEPRECATED;

GDataFreebaseTopicObject *gdata_freebase_topic_object_ref (GDataFreebaseTopicObject *object) G_GNUC_DEPRECATED;
void gdata_freebase_topic_object_unref (GDataFreebaseTopicObject *object) G_GNUC_DEPRECATED;

GPtrArray *gdata_freebase_topic_object_list_properties (const GDataFreebaseTopicObject *object) G_GNUC_DEPRECATED;

const gchar *gdata_freebase_topic_object_get_id (const GDataFreebaseTopicObject *object) G_GNUC_DEPRECATED;
guint64 gdata_freebase_topic_object_get_property_count (const GDataFreebaseTopicObject *object, const gchar *property) G_GNUC_DEPRECATED;
guint64 gdata_freebase_topic_object_get_property_hits (const GDataFreebaseTopicObject *object, const gchar *property) G_GNUC_DEPRECATED;
GDataFreebaseTopicValue *gdata_freebase_topic_object_get_property_value (const GDataFreebaseTopicObject *object, const gchar *property, gint64 item) G_GNUC_DEPRECATED;

GDataFreebaseTopicValue *gdata_freebase_topic_value_ref (GDataFreebaseTopicValue *value) G_GNUC_DEPRECATED;
void gdata_freebase_topic_value_unref (GDataFreebaseTopicValue *value) G_GNUC_DEPRECATED;

const gchar *gdata_freebase_topic_value_get_property (GDataFreebaseTopicValue *value) G_GNUC_DEPRECATED;

const gchar *gdata_freebase_topic_value_get_text (GDataFreebaseTopicValue *value) G_GNUC_DEPRECATED;
const gchar *gdata_freebase_topic_value_get_language (GDataFreebaseTopicValue *value) G_GNUC_DEPRECATED;
const gchar *gdata_freebase_topic_value_get_creator (GDataFreebaseTopicValue *value) G_GNUC_DEPRECATED;
gint64 gdata_freebase_topic_value_get_timestamp (GDataFreebaseTopicValue *value) G_GNUC_DEPRECATED;
GType gdata_freebase_topic_value_get_value_type (GDataFreebaseTopicValue *value) G_GNUC_DEPRECATED;
void gdata_freebase_topic_value_copy_value (GDataFreebaseTopicValue *value, GValue *gvalue) G_GNUC_DEPRECATED;

gint64 gdata_freebase_topic_value_get_int (GDataFreebaseTopicValue *value) G_GNUC_DEPRECATED;
gdouble gdata_freebase_topic_value_get_double (GDataFreebaseTopicValue *value) G_GNUC_DEPRECATED;
const gchar *gdata_freebase_topic_value_get_string (GDataFreebaseTopicValue *value) G_GNUC_DEPRECATED;
const GDataFreebaseTopicObject *gdata_freebase_topic_value_get_object (GDataFreebaseTopicValue *value) G_GNUC_DEPRECATED;

gboolean gdata_freebase_topic_value_is_image (GDataFreebaseTopicValue *value) G_GNUC_DEPRECATED;

#endif /* !LIBGDATA_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* !GDATA_FREEBASE_TOPIC_RESULT_H */
