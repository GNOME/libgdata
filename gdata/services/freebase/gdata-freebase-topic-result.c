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

/**
 * SECTION:gdata-freebase-topic-result
 * @short_description: GData Freebase topic result object
 * @stability: Stable
 * @include: gdata/services/freebase/gdata-freebase-topic-result.h
 *
 * #GDataFreebaseTopicResult is a subclass of #GDataFreebaseResult that contains all or a subset of the information
 * contained in Freebase about the Freebase ID given to the #GDataFreebaseTopicQuery.
 *
 * For more details of Google Freebase API, see the <ulink type="http" url="https://developers.google.com/freebase/v1/">
 * online documentation</ulink>.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */

typedef struct _GDataFreebaseTopicValueArray GDataFreebaseTopicValueArray;

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-freebase-topic-result.h"
#include "gdata-download-stream.h"
#include "gdata-private.h"
#include "gdata-types.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef enum {
	TYPE_NONE,
	TYPE_BOOL,
	TYPE_INT,
	TYPE_DOUBLE,
	TYPE_STRING,
	TYPE_DATETIME,
	TYPE_COMPOUND,
	TYPE_OBJECT,
	TYPE_KEY,
	TYPE_URI
} TopicValueType;

/* Wraps a compound object, either the main object returned by the result, or a
 * complex object within the result values (events are at least composed of
 * location and time at least, for example).
 */
struct _GDataFreebaseTopicObject {
	gchar *id;
	GHashTable *values; /* Hashtable of property->GDataFreebaseTopicValueArray */
	volatile gint ref_count;
};

/* Wraps an array of values, single-valued properties will contain an array with a single value here */
struct _GDataFreebaseTopicValueArray {
	TopicValueType type;
	GPtrArray *values;
	guint64 hits; /* Total number of hits, as opposed to values->len */
};

/* Wraps a single value in the topic result, may be either simple (numbers, strings, Freebase IDs...),
 * or nested compound types (contained by a GDataFreebaseTopicObject, which is what the value would
 * contain in that case).
 */
struct _GDataFreebaseTopicValue {
	gchar *property;
	gchar *text;
	gchar *lang;
	gchar *creator;
	gint64 timestamp;
	GValue value;
	volatile gint ref_count;
};

struct _GDataFreebaseTopicResultPrivate {
	GDataFreebaseTopicObject *object;
};

static void gdata_freebase_topic_result_finalize (GObject *self);
static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);
static GDataFreebaseTopicObject *object_new (const gchar *id);
static void value_free (GDataFreebaseTopicValue *object);
static gboolean reader_get_properties (JsonReader *reader, GDataFreebaseTopicObject *object, GError **error);

G_DEFINE_BOXED_TYPE (GDataFreebaseTopicObject, gdata_freebase_topic_object, gdata_freebase_topic_object_ref, gdata_freebase_topic_object_unref)
G_DEFINE_BOXED_TYPE (GDataFreebaseTopicValue, gdata_freebase_topic_value, gdata_freebase_topic_value_ref, gdata_freebase_topic_value_unref)
G_DEFINE_TYPE (GDataFreebaseTopicResult, gdata_freebase_topic_result, GDATA_TYPE_FREEBASE_RESULT)

static void
gdata_freebase_topic_result_class_init (GDataFreebaseTopicResultClass *klass)
{
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataFreebaseTopicResultPrivate));

	gobject_class->finalize = gdata_freebase_topic_result_finalize;
	parsable_class->parse_json = parse_json;
}

static void
gdata_freebase_topic_result_init (GDataFreebaseTopicResult *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_FREEBASE_TOPIC_RESULT, GDataFreebaseTopicResultPrivate);
}

static void
gdata_freebase_topic_result_finalize (GObject *self)
{
	GDataFreebaseTopicResultPrivate *priv = GDATA_FREEBASE_TOPIC_RESULT (self)->priv;

	gdata_freebase_topic_object_unref (priv->object);

	G_OBJECT_CLASS (gdata_freebase_topic_result_parent_class)->finalize (self);
}

static GDataFreebaseTopicValueArray *
value_array_new (TopicValueType type, guint64 hits)
{
	GDataFreebaseTopicValueArray *array;

	array = g_slice_new0 (GDataFreebaseTopicValueArray);
	array->values = g_ptr_array_new_with_free_func ((GDestroyNotify) gdata_freebase_topic_value_unref);
	array->type = type;
	array->hits = hits;

	return array;
}

/* Takes ownership on @value */
static void
value_array_add (GDataFreebaseTopicValueArray *array, GDataFreebaseTopicValue *value)
{
	g_ptr_array_add (array->values, value);
}

static void
value_array_free (GDataFreebaseTopicValueArray *array)
{
	g_ptr_array_unref (array->values);
	g_slice_free (GDataFreebaseTopicValueArray, array);
}

static guint64
reader_get_item_count (JsonReader *reader)
{
	gint64 count;

	json_reader_read_member (reader, "count");
	count = json_reader_get_int_value (reader);
	json_reader_end_member (reader);

	return (guint64) count;
}

static guint
reader_get_value_type (JsonReader *reader, const gchar *property, GError **error)
{
	TopicValueType type = TYPE_NONE;
	const GError *reader_error;
	const gchar *valuestr;

	json_reader_read_member (reader, "valuetype");
	valuestr = json_reader_get_string_value (reader);

	reader_error = json_reader_get_error (reader);

	if (reader_error != NULL) {
		if (error != NULL && *error == NULL)
			*error = g_error_copy (reader_error);
	} else {
		if (strcmp (valuestr, "key") == 0)
			type = TYPE_KEY;
		else if (strcmp (valuestr, "uri") == 0)
			type = TYPE_URI;
		else if (strcmp (valuestr, "compound") == 0)
			type = TYPE_COMPOUND;
		else if (strcmp (valuestr, "object") == 0)
			type = TYPE_OBJECT;
		else if (strcmp (valuestr, "float") == 0)
			type = TYPE_DOUBLE;
		else if (strcmp (valuestr, "string") == 0)
			type = TYPE_STRING;
		else if (strcmp (valuestr, "int") == 0)
			type = TYPE_INT;
		else if (strcmp (valuestr, "bool") == 0)
			type = TYPE_BOOL;
		else if (strcmp (valuestr, "datetime") == 0)
			type = TYPE_DATETIME;
		else
			gdata_parser_error_required_json_content_missing (reader, error);
	}

	json_reader_end_member (reader);
	return type;
}

static void
value_free (GDataFreebaseTopicValue *value)
{
	if (G_IS_VALUE (&value->value))
		g_value_unset (&value->value);
	g_free (value->text);
	g_free (value->lang);
	g_free (value->creator);
	g_free (value->property);
	g_slice_free (GDataFreebaseTopicValue, value);
}

/* Parsing functions to create GDataFreebaseTopicValues, and arrays of those */
static gchar *
reader_dup_member_string (JsonReader *reader, const gchar *member, GError **error)
{
	const GError *reader_error;
	gchar *str;

	if (error != NULL && *error != NULL)
		return NULL;

	json_reader_read_member (reader, member);
	str = g_strdup (json_reader_get_string_value (reader));
	reader_error = json_reader_get_error (reader);

	if (reader_error != NULL) {
		g_free (str);
		str = NULL;

		if (error != NULL)
			*error = g_error_copy (reader_error);
	}

	json_reader_end_member (reader);

	return str;
}

static gint64
reader_parse_timestamp (JsonReader *reader, const gchar *member, GError **error)
{
	const GError *reader_error;
	const gchar *date_str;
	gint64 timestamp = -1;

	if (error != NULL && *error != NULL)
		return -1;

	json_reader_read_member (reader, member);
	date_str = json_reader_get_string_value (reader);
	reader_error = json_reader_get_error (reader);

	if (reader_error != NULL) {
		if (error != NULL)
			*error = g_error_copy (reader_error);
	} else if (date_str) {
		if (!gdata_parser_int64_from_iso8601 (date_str, &timestamp))
			timestamp = -1;
	}

	json_reader_end_member (reader);

	return timestamp;
}

static gboolean
reader_fill_simple_gvalue (JsonReader *reader, TopicValueType type, GValue *value)
{
	gboolean retval = TRUE;
	gint64 datetime;

	json_reader_read_member (reader, "value");

	if (json_reader_get_error (reader) != NULL) {
		json_reader_end_member (reader);
		return FALSE;
	}

	switch (type) {
	case TYPE_BOOL:
	case TYPE_INT:
	case TYPE_DOUBLE:
	case TYPE_STRING:
		json_node_get_value (json_reader_get_value (reader), value);
		break;
	case TYPE_DATETIME:
		if (gdata_parser_int64_from_iso8601 (json_reader_get_string_value (reader), &datetime) ||
		    gdata_parser_int64_from_date (json_reader_get_string_value (reader), &datetime)) {
			g_value_init (value, G_TYPE_INT64);
			g_value_set_int64 (value, datetime);
		} else {
			retval = FALSE;
		}

		break;
	case TYPE_NONE:
	case TYPE_COMPOUND:
	case TYPE_OBJECT:
	case TYPE_KEY:
	case TYPE_URI:
	default:
		retval = FALSE;
	}

	json_reader_end_member (reader);
	return retval;
}

static GDataFreebaseTopicObject *
reader_create_object (JsonReader *reader, TopicValueType type)
{
	GDataFreebaseTopicObject *object;

	if (type != TYPE_OBJECT && type != TYPE_COMPOUND)
		return NULL;

	json_reader_read_member (reader, "id");

	if (json_reader_get_error (reader) != NULL) {
		json_reader_end_member (reader);
		return NULL;
	}

	object = object_new (json_reader_get_string_value (reader));
	json_reader_end_member (reader);

	return object;
}

static gboolean
reader_fill_object_gvalue (JsonReader *reader, TopicValueType type, GValue *value)
{
	GDataFreebaseTopicObject *object;

	if (type != TYPE_OBJECT)
		return FALSE;

	object = reader_create_object (reader, type);

	if (object != NULL) {
		g_value_init (value, GDATA_TYPE_FREEBASE_TOPIC_OBJECT);
		g_value_take_boxed (value, object);
	}

	return (object != NULL);
}

static gboolean
reader_fill_compound_gvalue (JsonReader *reader, TopicValueType type, GValue *value, GError **error)
{
	GDataFreebaseTopicObject *object;

	if (type != TYPE_COMPOUND)
		return FALSE;

	object = reader_create_object (reader, type);

	if (object == NULL)
		return FALSE;

	json_reader_read_member (reader, "property");

	if (json_reader_get_error (reader) != NULL) {
		json_reader_end_member (reader);
		gdata_freebase_topic_object_unref (object);
		return FALSE;
	}

	reader_get_properties (reader, object, error);
	json_reader_end_member (reader);

	g_value_init (value, GDATA_TYPE_FREEBASE_TOPIC_OBJECT);
	g_value_take_boxed (value, object);
	return TRUE;
}

static GDataFreebaseTopicValue *
reader_create_value (JsonReader *reader, const gchar *property, TopicValueType type, GError **error)
{
	GDataFreebaseTopicValue *value;

	value = g_slice_new0 (GDataFreebaseTopicValue);

	value->ref_count = 1;
	value->property = g_strdup (property);
	value->text = reader_dup_member_string (reader, "text", error);
	value->lang = reader_dup_member_string (reader, "lang", error);

	/* Not all parsed nodes are meant to contain creator/timestamp tags,
	 * do not pass error to those, so parsing continues.
	 */
	value->creator = reader_dup_member_string (reader, "creator", NULL);
	value->timestamp = reader_parse_timestamp (reader, "timestamp", NULL);

	if (reader_fill_simple_gvalue (reader, type, &value->value) ||
	    reader_fill_object_gvalue (reader, type, &value->value) ||
	    reader_fill_compound_gvalue (reader, type, &value->value, error))
		return value;

	value_free (value);
	return NULL;
}

static GDataFreebaseTopicValueArray *
reader_create_value_array (JsonReader  *reader, const gchar *property, GError **error)
{
	GDataFreebaseTopicValueArray *array;
	GDataFreebaseTopicValue *value;
	TopicValueType type;
	guint64 count, i;

	count = reader_get_item_count (reader);

	if (count <= 0)
		return NULL;

	type = reader_get_value_type (reader, property, error);

	if (type == TYPE_NONE || type == TYPE_URI || type == TYPE_KEY)
		return NULL;

	array = value_array_new (type, count);

	json_reader_read_member (reader, "values");
	count = json_reader_count_elements (reader);

	for (i = 0; i < count; i++) {
		json_reader_read_element (reader, i);
		value = reader_create_value (reader, property, type, error);
		json_reader_end_element (reader);

		if (value != NULL)
			value_array_add (array, value);
	}

	json_reader_end_member (reader);

	return array;
}

static GDataFreebaseTopicObject *
object_new (const gchar *id)
{
	GDataFreebaseTopicObject *object;

	object = g_slice_new0 (GDataFreebaseTopicObject);
	object->id = g_strdup (id);
	object->ref_count = 1;
	object->values = g_hash_table_new_full (g_str_hash, g_str_equal,
						(GDestroyNotify) g_free,
						(GDestroyNotify) value_array_free);
	return object;
}

/* Takes ownership on @array */
static void
object_add_value (GDataFreebaseTopicObject *object, const gchar *property, GDataFreebaseTopicValueArray *array)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (property != NULL);
	g_return_if_fail (array != NULL);
	g_hash_table_replace (object->values, g_strdup (property), array);
}

static gboolean
reader_get_properties (JsonReader *reader, GDataFreebaseTopicObject *object, GError **error)
{
	GDataFreebaseTopicValueArray *array;
	gboolean retval = TRUE;
	gint count, i;

	count = json_reader_count_members (reader);

	for (i = 0; i < count; i++) {
		GError *inner_error = NULL;
		const gchar *name;
		gchar *property;

		json_reader_read_element (reader, i);
		property = g_strdup (json_reader_get_member_name (reader));
		name = property;

		/* Reverse properties start with !, display those as
		 * regular properties, and skip that char
		 */
		if (name[0] == '!')
			name++;

		/* All Freebase properties and IDs start with '/' */
		if (name[0] != '/')
			continue;

		/* Parse the value for this property, possibly with nested contents */
		array = reader_create_value_array (reader, name, &inner_error);
		json_reader_end_element (reader);

		if (inner_error != NULL) {
			g_propagate_error (error, inner_error);
			retval = FALSE;
			break;
		} else if (array != NULL) {
			/* Takes ownership of array */
			object_add_value (object, name, array);
		}

		g_free (property);
	}

	return retval;
}

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	GDataFreebaseTopicResultPrivate *priv = GDATA_FREEBASE_TOPIC_RESULT (parsable)->priv;
	const gchar *member_name;

	GDATA_PARSABLE_CLASS (gdata_freebase_topic_result_parent_class)->parse_json (parsable, reader, user_data, error);

	member_name = json_reader_get_member_name (reader);

	if (member_name == NULL)
		return FALSE;

	if (strcmp (member_name, "id") == 0) {
		/* We only expect one member containing information */
		g_assert (priv->object == NULL);
		priv->object = object_new (json_reader_get_string_value (reader));
	} else if (strcmp (member_name, "property") == 0) {
		reader_get_properties (reader, priv->object, error);
	} else {
		return FALSE;
	}

	return TRUE;
}

/**
 * gdata_freebase_topic_result_new:
 *
 * Creates a new #GDataFreebaseTopicResult with the given ID and default properties.
 *
 * Return value: (transfer full): a new #GDataFreebaseTopicResult; unref with g_object_unref()
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GDataFreebaseTopicResult *
gdata_freebase_topic_result_new (void)
{
	return g_object_new (GDATA_TYPE_FREEBASE_TOPIC_RESULT, NULL);
}

/**
 * gdata_freebase_topic_result_dup_object:
 * @self: a #GDataFreebaseTopicResult
 *
 * Returns a reference to the root #GDataFreebaseTopicObject containing the
 * topic query results.
 *
 * Returns: (transfer full): A new reference on the result object, unref with
 *   gdata_freebase_topic_object_unref()
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GDataFreebaseTopicObject *
gdata_freebase_topic_result_dup_object (GDataFreebaseTopicResult *self)
{
	g_return_val_if_fail (GDATA_IS_FREEBASE_TOPIC_RESULT (self), NULL);

	return gdata_freebase_topic_object_ref (self->priv->object);
}

/**
 * gdata_freebase_topic_object_ref:
 * @object: a #GDataFreebaseTopicObject
 *
 * Creates and returns a new reference on @object.
 *
 * Returns: (transfer full): @object, with an extra reference.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GDataFreebaseTopicObject *
gdata_freebase_topic_object_ref (GDataFreebaseTopicObject *object)
{
	g_return_val_if_fail (object != NULL, NULL);

	g_atomic_int_inc (&object->ref_count);
	return object;
}

/**
 * gdata_freebase_topic_object_unref:
 * @object: (transfer full): a #GDataFreebaseTopicResult
 *
 * Removes a reference from @object. If the reference count drops to 0,
 * the object is freed.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
void
gdata_freebase_topic_object_unref (GDataFreebaseTopicObject *object)
{
	g_return_if_fail (object != NULL);

	if (g_atomic_int_dec_and_test (&object->ref_count)) {
		g_hash_table_unref (object->values);
		g_free (object->id);
		g_slice_free (GDataFreebaseTopicObject, object);
	}
}

/**
 * gdata_freebase_topic_object_list_properties:
 * @object: a #GDataFreebaseTopicObject
 *
 * Returns the list of Freebase properties described by @object.
 *
 * Returns: (transfer container) (element-type gchar*): An array of property names, free with g_ptr_array_unref().
 *
 * Since: 0.15.1
Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GPtrArray *
gdata_freebase_topic_object_list_properties (const GDataFreebaseTopicObject *object)
{
	GPtrArray *properties;
	GHashTableIter iter;
	gchar *property;

	g_return_val_if_fail (object != NULL, NULL);

	properties = g_ptr_array_new ();
	g_hash_table_iter_init (&iter, object->values);

	while (g_hash_table_iter_next (&iter, (gpointer *) &property, NULL))
		g_ptr_array_add (properties, property);

	return properties;
}

/**
 * gdata_freebase_topic_object_get_property_count:
 * @object: a #GDataFreebaseTopicObject
 * @property: a property name contained in @object
 *
 * Returns the number of values that @object holds for the given @property. If @object
 * contains no information about @property, 0 is returned.
 *
 * Returns: The number of values contained for @property
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
guint64
gdata_freebase_topic_object_get_property_count (const GDataFreebaseTopicObject *object, const gchar *property)
{
	GDataFreebaseTopicValueArray *array;

	g_return_val_if_fail (object != NULL, 0);
	g_return_val_if_fail (property != NULL, 0);
	array = g_hash_table_lookup (object->values, property);

	if (array == NULL)
		return 0;

	return array->values->len;
}

/**
 * gdata_freebase_topic_object_get_property_hits:
 * @object: a #GDataFreebaseTopicObject
 * @property: a property name contained in @object
 *
 * Returns the total number of hits that the Freebase database stores
 * for this object, this number either equals or is greater than
 * gdata_freebase_topic_object_get_property_count(), the query limit
 * can be controlled through gdata_query_set_max_results() on the topic
 * query.
 *
 * If @object contains no information about @property, 0 is returned.
 *
 * Returns: the total number of hits for this property
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
guint64
gdata_freebase_topic_object_get_property_hits (const GDataFreebaseTopicObject *object, const gchar *property)
{
	GDataFreebaseTopicValueArray *array;

	g_return_val_if_fail (object != NULL, 0);
	g_return_val_if_fail (property != NULL, 0);
	array = g_hash_table_lookup (object->values, property);

	if (array == NULL)
		return 0;

	return array->hits;
}

/**
 * gdata_freebase_topic_object_get_property_value:
 * @object: a #GDataFreebaseTopicObject
 * @property: a property name contained in @object
 * @item: item number to retrieve from @property
 *
 * Gets the value that @object stores for this @property/@item pair, as a generic
 * #GDataFreebaseTopicValue. If @object contains no information about @property,
 * or @item is outside the [0..gdata_freebase_topic_object_get_property_count() - 1]
 * range, %NULL is returned.
 *
 * Returns: (allow-none) (transfer none): the value for this property/item
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GDataFreebaseTopicValue *
gdata_freebase_topic_object_get_property_value (const GDataFreebaseTopicObject *object, const gchar *property, gint64 item)
{
	GDataFreebaseTopicValueArray *array;

	g_return_val_if_fail (object != NULL, NULL);
	g_return_val_if_fail (property != NULL, NULL);
	array = g_hash_table_lookup (object->values, property);

	if (array == NULL)
		return NULL;

	if (item < 0 || item >= array->values->len)
		return NULL;

	return g_ptr_array_index (array->values, item);
}

/**
 * gdata_freebase_topic_object_get_id:
 * @object: a #GDataFreebaseTopicObject
 *
 * Gets the Freebase ID for this specific object.
 *
 * Returns: (transfer none): the Freebase ID of this object
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const gchar *
gdata_freebase_topic_object_get_id (const GDataFreebaseTopicObject *object)
{
	g_return_val_if_fail (object != NULL, NULL);
	return object->id;
}

/**
 * gdata_freebase_topic_value_ref:
 * @value: a #GDataFreebaseTopicValue
 *
 * Creates and returns a new reference on @value.
 *
 * Returns: (transfer full): @value, with an extra reference.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GDataFreebaseTopicValue *
gdata_freebase_topic_value_ref (GDataFreebaseTopicValue *value)
{
	g_return_val_if_fail (value != NULL, NULL);

	g_atomic_int_inc (&value->ref_count);
	return value;
}

/**
 * gdata_freebase_topic_value_unref:
 * @value:  (transfer full): a #GDataFreebaseTopicValue
 *
 * Removes a reference from @value. If the reference count drops to 0,
 * the object is freed.
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
void
gdata_freebase_topic_value_unref (GDataFreebaseTopicValue *value)
{
	g_return_if_fail (value != NULL);

	if (g_atomic_int_dec_and_test (&value->ref_count))
		value_free (value);
}

/**
 * gdata_freebase_topic_value_get_property:
 * @value: a #GDataFreebaseTopicValue
 *
 * Returns the property name that this value describes
 *
 * Returns: the property name of @value
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const gchar *
gdata_freebase_topic_value_get_property (GDataFreebaseTopicValue *value)
{
	g_return_val_if_fail (value != NULL, NULL);
	return value->property;
}

/**
 * gdata_freebase_topic_value_get_text:
 * @value: a #GDataFreebaseTopicValue
 *
 * Returns a textual representation of this value, this is either
 * the value contained transformed to a string, or a concatenation
 * of subvalues for compound types.
 *
 * Returns: a textual representation of @value
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const gchar *
gdata_freebase_topic_value_get_text (GDataFreebaseTopicValue *value)
{
	g_return_val_if_fail (value != NULL, NULL);
	return value->text;
}

/**
 * gdata_freebase_topic_value_get_language:
 * @value: a #GDataFreebaseTopicValue
 *
 * Returns the language used in the content of @value
 *
 * Returns: the language @value is written in
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const gchar *
gdata_freebase_topic_value_get_language (GDataFreebaseTopicValue *value)
{
	g_return_val_if_fail (value != NULL, NULL);
	return value->lang;
}

/**
 * gdata_freebase_topic_value_get_creator:
 * @value: a #GDataFreebaseTopicValue
 *
 * Returns the Freebase ID of the user that created this value.
 *
 * Returns: the creator of this value, as a Freebase ID
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const gchar *
gdata_freebase_topic_value_get_creator (GDataFreebaseTopicValue *value)
{
	g_return_val_if_fail (value != NULL, NULL);
	return value->creator;
}

/**
 * gdata_freebase_topic_value_get_timestamp:
 * @value: #GDataFreebaseTopicValue
 *
 * Returns the time at which this value was created in the Freebase database.
 * It's a UNIX timestamp in seconds since the epoch. If @value has no timestamp,
 * -1 will be returned.
 *
 * Returns: The creation time of @value, or -1
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
gint64
gdata_freebase_topic_value_get_timestamp (GDataFreebaseTopicValue *value)
{
	g_return_val_if_fail (value != NULL, 0);
	return value->timestamp;
}

/**
 * gdata_freebase_topic_value_get_value_type:
 * @value: a #GDataFreebaseTopicValue
 *
 * Returns the #GType of the real value held in @value.
 *
 * Returns: the #GType of the contained value
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
GType
gdata_freebase_topic_value_get_value_type (GDataFreebaseTopicValue *value)
{
	g_return_val_if_fail (value != NULL, G_TYPE_INVALID);
	return G_VALUE_TYPE (&value->value);
}

/**
 * gdata_freebase_topic_value_copy_value:
 * @value: a #GDataFreebaseTopicValue
 * @gvalue: (out caller-allocates) (transfer full): an empty #GValue
 *
 * Copies in @gvalue the value held in @value. the #GValue must be later freed through g_value_unset()
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
void
gdata_freebase_topic_value_copy_value (GDataFreebaseTopicValue *value, GValue *gvalue)
{
	g_return_if_fail (value != NULL);

	g_value_copy (&value->value, gvalue);
}

/**
 * gdata_freebase_topic_value_get_int:
 * @value: a #GDataFreebaseTopicValue
 *
 * Returns a #gint64 value held in @value. It is only valid to call this if the #GType is a %G_TYPE_INT64
 *
 * Returns: the #gint64 value
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
gint64
gdata_freebase_topic_value_get_int (GDataFreebaseTopicValue *value)
{
	g_return_val_if_fail (value != NULL, 0);
	g_return_val_if_fail (G_VALUE_HOLDS_INT64 (&value->value), 0);

	return g_value_get_int64 (&value->value);
}

/**
 * gdata_freebase_topic_value_get_double:
 * @value: a #GDataFreebaseTopicValue
 *
 * Returns a #gdouble value held in @value. It is only valid to call this if the #GType is a %G_TYPE_DOUBLE
 *
 * Returns: the #gdouble value
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
gdouble
gdata_freebase_topic_value_get_double (GDataFreebaseTopicValue *value)
{
	g_return_val_if_fail (value != NULL, 0);
	g_return_val_if_fail (G_VALUE_HOLDS_DOUBLE (&value->value), 0);

	return g_value_get_double (&value->value);
}

/**
 * gdata_freebase_topic_value_get_string:
 * @value: a #GDataFreebaseTopicValue
 *
 * Returns a string value held in @value. It is only valid to call this if the #GType is a %G_TYPE_STRING
 *
 * Returns: the string value
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const gchar *
gdata_freebase_topic_value_get_string (GDataFreebaseTopicValue *value)
{
	g_return_val_if_fail (value != NULL, NULL);
	g_return_val_if_fail (G_VALUE_HOLDS_STRING (&value->value), NULL);

	return g_value_get_string (&value->value);
}

/**
 * gdata_freebase_topic_value_get_object:
 * @value: a #GDataFreebaseTopicValue
 *
 * Returns a compound/complex object held in @value. It is only valid to call this if the #GType is a
 * %GDATA_TYPE_FREEBASE_TOPIC_OBJECT.
 *
 * Returns: (transfer none): the compound value as a #GDataFreebaseTopicObject
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
const GDataFreebaseTopicObject *
gdata_freebase_topic_value_get_object (GDataFreebaseTopicValue *value)
{
	g_return_val_if_fail (value != NULL, NULL);
	g_return_val_if_fail (G_VALUE_HOLDS (&value->value, GDATA_TYPE_FREEBASE_TOPIC_OBJECT), NULL);

	return g_value_get_boxed (&value->value);
}

/**
 * gdata_freebase_topic_value_is_image:
 * @value: a #GDataFreebaseTopicValue
 *
 * Returns true if @value holds a freebase image object, on such values it
 * will be valid to call gdata_freebase_service_get_image() to get a stream
 * to the image itself.
 *
 * Returns: Whether @value holds a Freebase image object
 *
 * Since: 0.15.1
 * Deprecated: 0.17.7: Google Freebase has been permanently shut down.
 */
gboolean
gdata_freebase_topic_value_is_image (GDataFreebaseTopicValue *value)
{
	g_return_val_if_fail (value != NULL, FALSE);

	return (strcmp (value->property, "/common/topic/image") == 0);
}

G_GNUC_END_IGNORE_DEPRECATIONS
