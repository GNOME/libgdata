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

/**
 * SECTION:gdata-parsable
 * @short_description: GData parsable object
 * @stability: Stable
 * @include: gdata/gdata-parsable.h
 *
 * #GDataParsable is an abstract class allowing easy implementation of an extensible parser. It is primarily extended by #GDataFeed and #GDataEntry,
 * both of which require XML parsing which can be extended by subclassing.
 *
 * It allows methods to be defined for handling the root XML node, each of its child nodes, and a method to be called after parsing is complete.
 *
 * Since: 0.3.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>
#include <libxml/parser.h>
#include <json-glib/json-glib.h>

#include "gdata-parsable.h"
#include "gdata-private.h"
#include "gdata-parser.h"

GQuark
gdata_parser_error_quark (void)
{
	return g_quark_from_static_string ("gdata-parser-error-quark");
}

static void gdata_parsable_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_parsable_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void gdata_parsable_finalize (GObject *object);
static gboolean real_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static gboolean real_parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);
static const gchar *get_content_type (void);

struct _GDataParsablePrivate {
	/* XML stuff. */
	GString *extra_xml;
	GHashTable *extra_namespaces;

	/* JSON stuff. */
	GHashTable/*<gchar*, owned JsonNode*>*/ *extra_json;

	gboolean constructed_from_xml;
};

enum {
	PROP_CONSTRUCTED_FROM_XML = 1,
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (GDataParsable, gdata_parsable, G_TYPE_OBJECT)

static void
gdata_parsable_class_init (GDataParsableClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->get_property = gdata_parsable_get_property;
	gobject_class->set_property = gdata_parsable_set_property;
	gobject_class->finalize = gdata_parsable_finalize;
	klass->parse_xml = real_parse_xml;
	klass->parse_json = real_parse_json;
	klass->get_content_type = get_content_type;

	/**
	 * GDataParsable:constructed-from-xml:
	 *
	 * Specifies whether the object was constructed by parsing XML or manually.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_CONSTRUCTED_FROM_XML,
	                                 g_param_spec_boolean ("constructed-from-xml",
	                                                       "Constructed from XML?",
	                                                       "Specifies whether the object was constructed by parsing XML or manually.",
	                                                       FALSE,
	                                                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_parsable_init (GDataParsable *self)
{
	self->priv = gdata_parsable_get_instance_private (self);

	self->priv->extra_xml = g_string_new ("");
	self->priv->extra_namespaces = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

	self->priv->extra_json = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) json_node_free);

	self->priv->constructed_from_xml = FALSE;
}


static void
gdata_parsable_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataParsablePrivate *priv = GDATA_PARSABLE (object)->priv;

	switch (property_id) {
		case PROP_CONSTRUCTED_FROM_XML:
			g_value_set_boolean (value, priv->constructed_from_xml);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_parsable_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataParsablePrivate *priv = GDATA_PARSABLE (object)->priv;

	switch (property_id) {
		case PROP_CONSTRUCTED_FROM_XML:
			priv->constructed_from_xml = g_value_get_boolean (value);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_parsable_finalize (GObject *object)
{
	GDataParsablePrivate *priv = GDATA_PARSABLE (object)->priv;

	g_string_free (priv->extra_xml, TRUE);
	g_hash_table_destroy (priv->extra_namespaces);

	g_hash_table_destroy (priv->extra_json);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_parsable_parent_class)->finalize (object);
}

static gboolean
real_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	xmlBuffer *buffer;
	xmlNs **namespaces, **namespace;

	/* Unhandled XML */
	buffer = xmlBufferCreate ();
	xmlNodeDump (buffer, doc, node, 0, 0);
	g_string_append (parsable->priv->extra_xml, (gchar*) xmlBufferContent (buffer));
	g_debug ("Unhandled XML in %s: %s", G_OBJECT_TYPE_NAME (parsable), (gchar*) xmlBufferContent (buffer));
	xmlBufferFree (buffer);

	/* Get the namespaces */
	namespaces = xmlGetNsList (doc, node);
	if (namespaces == NULL)
		return TRUE;

	for (namespace = namespaces; *namespace != NULL; namespace++) {
		if ((*namespace)->prefix != NULL) {
			/* NOTE: These two g_strdup()s leak, but it's probably acceptable, given that it saves us
			 * g_strdup()ing every other namespace we put in @extra_namespaces. */
			g_hash_table_insert (parsable->priv->extra_namespaces,
			                     g_strdup ((gchar*) ((*namespace)->prefix)),
			                     g_strdup ((gchar*) ((*namespace)->href)));
		}
	}
	xmlFree (namespaces);

	return TRUE;
}

/* Extract the member node. This would be a lot easier if JsonReader had an API to return
 * the current node (regardless of whether it's a value, object or array). FIXME: bgo#707100. */
static JsonNode * /* transfer full */
_json_reader_dup_current_node (JsonReader *reader)
{
	JsonNode *value;

	if (json_reader_is_value (reader) == TRUE) {
		/* Value nodes are easy. Well, ignoring the complication of nulls. */
		if (json_reader_get_null_value (reader) == TRUE) {
			value = json_node_new (JSON_NODE_NULL);
		} else {
			value = json_node_copy (json_reader_get_value (reader));
		}
	} else if (json_reader_is_object (reader) == TRUE) {
		/* Object nodes require deep copies. */
		gint i;
		gchar **members;
		JsonObject *obj;

		obj = json_object_new ();

		for (i = 0, members = json_reader_list_members (reader); members[i] != NULL; i++) {
			json_reader_read_member (reader, members[i]);
			json_object_set_member (obj, members[i], _json_reader_dup_current_node (reader));
			json_reader_end_member (reader);
		}

		g_strfreev (members);

		value = json_node_new (JSON_NODE_OBJECT);
		json_node_take_object (value, obj);
	} else if (json_reader_is_array (reader) == TRUE) {
		/* Array nodes require deep copies. */
		gint i, elements;
		JsonArray *arr;

		arr = json_array_new ();

		for (i = 0, elements = json_reader_count_elements (reader); i < elements; i++) {
			json_reader_read_element (reader, i);
			json_array_add_element (arr, _json_reader_dup_current_node (reader));
			json_reader_end_element (reader);
		}

		value = json_node_new (JSON_NODE_ARRAY);
		json_node_take_array (value, arr);
	} else {
		/* Uh-oh. */
		g_assert_not_reached ();
	}

	return value;
}

static gboolean
real_parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	gchar *json, *member_name;
	JsonGenerator *generator;
	JsonNode *value;

	/* Unhandled JSON member. Save it and its value to ->extra_xml so that it's not lost if we
	 * re-upload this Parsable to the server. */
	member_name = g_strdup (json_reader_get_member_name (reader));
	g_assert (member_name != NULL);

	/* Extract a copy of the current node. */
	value = _json_reader_dup_current_node (reader);
	g_assert (value != NULL);

	/* Serialise the value for debugging. */
	generator = json_generator_new ();
	json_generator_set_root (generator, value);

	json = json_generator_to_data (generator, NULL);
	g_debug ("Unhandled JSON member ‘%s’ in %s: %s", member_name, G_OBJECT_TYPE_NAME (parsable), json);
	g_free (json);

	g_object_unref (generator);

	/* Save the value. Transfer ownership of the member_name and value. */
	g_hash_table_replace (parsable->priv->extra_json, (gpointer) member_name, (gpointer) value);

	return TRUE;
}

static const gchar *
get_content_type (void) {
	return "application/atom+xml";
}

/**
 * gdata_parsable_new_from_xml:
 * @parsable_type: the type of the class represented by the XML
 * @xml: the XML for just the parsable object, with full namespace declarations
 * @length: the length of @xml, or -1
 * @error: a #GError, or %NULL
 *
 * Creates a new #GDataParsable subclass (of the given @parsable_type) from the given @xml.
 *
 * An object of the given @parsable_type is created, and its <function>pre_parse_xml</function>, <function>parse_xml</function> and
 * <function>post_parse_xml</function> class functions called on the XML tree obtained from @xml. <function>pre_parse_xml</function> and
 * <function>post_parse_xml</function> are called once each on the root node of the tree, while <function>parse_xml</function> is called for
 * each of the child nodes of the root node.
 *
 * If @length is -1, @xml will be assumed to be null-terminated.
 *
 * If an error occurs during parsing, a suitable error from #GDataParserError will be returned.
 *
 * Return value: a new #GDataParsable, or %NULL; unref with g_object_unref()
 *
 * Since: 0.4.0
 */
GDataParsable *
gdata_parsable_new_from_xml (GType parsable_type, const gchar *xml, gint length, GError **error)
{
	g_return_val_if_fail (g_type_is_a (parsable_type, GDATA_TYPE_PARSABLE), NULL);
	g_return_val_if_fail (xml != NULL && *xml != '\0', NULL);
	g_return_val_if_fail (length >= -1, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return _gdata_parsable_new_from_xml (parsable_type, xml, length, NULL, error);
}

GDataParsable *
_gdata_parsable_new_from_xml (GType parsable_type, const gchar *xml, gint length, gpointer user_data, GError **error)
{
	xmlDoc *doc;
	xmlNode *node;
	GDataParsable *parsable;
	static gboolean libxml_initialised = FALSE;

	g_return_val_if_fail (g_type_is_a (parsable_type, GDATA_TYPE_PARSABLE), NULL);
	g_return_val_if_fail (xml != NULL && *xml != '\0', NULL);
	g_return_val_if_fail (length >= -1, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Set up libxml. We do this here to avoid introducing a libgdata setup function, which would be unnecessary hassle. This is the only place
	 * that libxml can be initialised in the library. */
	if (libxml_initialised == FALSE) {
		/* Change the libxml memory allocation functions to be GLib's. This means we don't have to re-allocate all the strings we get from
		 * libxml, which cuts down on strdup() calls dramatically. */
		xmlMemSetup ((xmlFreeFunc) g_free, (xmlMallocFunc) g_malloc, (xmlReallocFunc) g_realloc, (xmlStrdupFunc) g_strdup);
		libxml_initialised = TRUE;
	}

	if (length == -1)
		length = strlen (xml);

	/* Parse the XML */
	doc = xmlReadMemory (xml, length, "/dev/null", NULL, 0);
	if (doc == NULL) {
		xmlError *xml_error = xmlGetLastError ();
		g_set_error (error, GDATA_PARSER_ERROR, GDATA_PARSER_ERROR_PARSING_STRING,
		             /* Translators: the parameter is an error message */
		             _("Error parsing XML: %s"),
		             (xml_error != NULL) ? xml_error->message : NULL);
		return NULL;
	}

	/* Get the root element */
	node = xmlDocGetRootElement (doc);
	if (node == NULL) {
		/* XML document's empty */
		xmlFreeDoc (doc);
		g_set_error (error, GDATA_PARSER_ERROR, GDATA_PARSER_ERROR_EMPTY_DOCUMENT,
		             _("Error parsing XML: %s"),
		             /* Translators: this is a dummy error message to be substituted into "Error parsing XML: %s". */
		             _("Empty document."));
		return NULL;
	}

	parsable = _gdata_parsable_new_from_xml_node (parsable_type, doc, node, user_data, error);
	xmlFreeDoc (doc);

	return parsable;
}

GDataParsable *
_gdata_parsable_new_from_xml_node (GType parsable_type, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	GDataParsable *parsable;
	GDataParsableClass *klass;

	g_return_val_if_fail (g_type_is_a (parsable_type, GDATA_TYPE_PARSABLE), NULL);
	g_return_val_if_fail (doc != NULL, NULL);
	g_return_val_if_fail (node != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	parsable = g_object_new (parsable_type, "constructed-from-xml", TRUE, NULL);

	klass = GDATA_PARSABLE_GET_CLASS (parsable);
	if (klass->parse_xml == NULL) {
		g_object_unref (parsable);
		return NULL;
	}

	g_assert (klass->element_name != NULL);
	/* TODO: See gdata-documents-entry.c:260 for an example of where the code below doesn't work */
	/*if (xmlStrcmp (node->name, (xmlChar*) klass->element_name) != 0 ||
	    (node->ns != NULL && xmlStrcmp (node->ns->prefix, (xmlChar*) klass->element_namespace) != 0)) {
		* No <entry> element (required) *
		gdata_parser_error_required_element_missing (klass->element_name, "root", error);
		return NULL;
	}*/

	/* Call the pre-parse function first */
	if (klass->pre_parse_xml != NULL &&
	    klass->pre_parse_xml (parsable, doc, node, user_data, error) == FALSE) {
		g_object_unref (parsable);
		return NULL;
	}

	/* Parse each child element */
	node = node->children;
	while (node != NULL) {
		if (klass->parse_xml (parsable, doc, node, user_data, error) == FALSE) {
			g_object_unref (parsable);
			return NULL;
		}
		node = node->next;
	}

	/* Call the post-parse function */
	if (klass->post_parse_xml != NULL &&
	    klass->post_parse_xml (parsable, user_data, error) == FALSE) {
		g_object_unref (parsable);
		return NULL;
	}

	return parsable;
}

/**
 * gdata_parsable_new_from_json:
 * @parsable_type: the type of the class represented by the JSON
 * @json: the JSON for just the parsable object
 * @length: the length of @json, or -1
 * @error: a #GError, or %NULL
 *
 * Creates a new #GDataParsable subclass (of the given @parsable_type) from the given @json.
 *
 * An object of the given @parsable_type is created, and its <function>parse_json</function> and
 * <function>post_parse_json</function> class functions called on the JSON node obtained from @json.
 * <function>post_parse_json</function> is called once on the root node, while <function>parse_json</function> is called for
 * each of the node's members.
 *
 * If @length is -1, @json will be assumed to be nul-terminated.
 *
 * If an error occurs during parsing, a suitable error from #GDataParserError will be returned.
 *
 * Return value: a new #GDataParsable, or %NULL; unref with g_object_unref()
 *
 * Since: 0.15.0
 */
GDataParsable *
gdata_parsable_new_from_json (GType parsable_type, const gchar *json, gint length, GError **error)
{
	g_return_val_if_fail (g_type_is_a (parsable_type, GDATA_TYPE_PARSABLE), NULL);
	g_return_val_if_fail (json != NULL && *json != '\0', NULL);
	g_return_val_if_fail (length >= -1, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	return _gdata_parsable_new_from_json (parsable_type, json, length, NULL, error);
}

GDataParsable *
_gdata_parsable_new_from_json (GType parsable_type, const gchar *json, gint length, gpointer user_data, GError **error)
{
	JsonParser *parser;
	JsonReader *reader;
	GDataParsable *parsable;
	GError *child_error = NULL;

	g_return_val_if_fail (g_type_is_a (parsable_type, GDATA_TYPE_PARSABLE), NULL);
	g_return_val_if_fail (json != NULL && *json != '\0', NULL);
	g_return_val_if_fail (length >= -1, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	if (length == -1)
		length = strlen (json);

	parser = json_parser_new ();
	if (!json_parser_load_from_data (parser, json, length, &child_error)) {
		g_set_error (error, GDATA_PARSER_ERROR, GDATA_PARSER_ERROR_PARSING_STRING,
		             /* Translators: the parameter is an error message */
		             _("Error parsing JSON: %s"), child_error->message);
		g_error_free (child_error);
		g_object_unref (parser);

		return NULL;
	}

	reader = json_reader_new (json_parser_get_root (parser));
	parsable = _gdata_parsable_new_from_json_node (parsable_type, reader, user_data, error);

	g_object_unref (reader);
	g_object_unref (parser);

	return parsable;
}

GDataParsable *
_gdata_parsable_new_from_json_node (GType parsable_type, JsonReader *reader, gpointer user_data, GError **error)
{
	GDataParsable *parsable;
	GDataParsableClass *klass;
	gint i;

	g_return_val_if_fail (g_type_is_a (parsable_type, GDATA_TYPE_PARSABLE), NULL);
	g_return_val_if_fail (reader != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Indicator property which allows distinguishing between locally created and server based objects
	 * as it is used for non-XML tasks, and adding another one for JSON would be a bit pointless. */
	parsable = g_object_new (parsable_type, "constructed-from-xml", TRUE, NULL);

	klass = GDATA_PARSABLE_GET_CLASS (parsable);
	g_assert (klass->parse_json != NULL);

	/* Check that the outermost node is an object. */
	if (json_reader_is_object (reader) == FALSE) {
		g_set_error (error, GDATA_PARSER_ERROR, GDATA_PARSER_ERROR_PARSING_STRING,
		             /* Translators: the parameter is an error message */
		             _("Error parsing JSON: %s"),
		             _("Outermost JSON node is not an object."));
		g_object_unref (parsable);
		return NULL;
	}

	/* Parse each child member. This assumes the outermost node is an object. */
	for (i = 0; i < json_reader_count_members (reader); i++) {
		g_return_val_if_fail (json_reader_read_element (reader, i), NULL);

		if (klass->parse_json (parsable, reader, user_data, error) == FALSE) {
			json_reader_end_element (reader);
			g_object_unref (parsable);
			return NULL;
		}

		json_reader_end_element (reader);
	}

	/* Call the post-parse function */
	if (klass->post_parse_json != NULL &&
	    klass->post_parse_json (parsable, user_data, error) == FALSE) {
		g_object_unref (parsable);
		return NULL;
	}

	return parsable;
}

/**
 * gdata_parsable_get_content_type:
 * @self: a #GDataParsable
 *
 * Returns the content type upon which the #GDataParsable is built. For example, `application/atom+xml` or `application/json`.
 *
 * Return value: the parsable's content type
 *
 * Since: 0.17.7
 */
const gchar *
gdata_parsable_get_content_type (GDataParsable *self)
{
	GDataParsableClass *klass;

	g_return_val_if_fail (GDATA_IS_PARSABLE (self), NULL);

	klass = GDATA_PARSABLE_GET_CLASS (self);
	g_assert (klass->get_content_type != NULL);

	return klass->get_content_type ();
}

static void
build_namespaces_cb (gchar *prefix, gchar *href, GString *output)
{
	g_string_append_printf (output, " xmlns:%s='%s'", prefix, href);
}

static gboolean
filter_namespaces_cb (gchar *prefix, gchar *href, GHashTable *canonical_namespaces)
{
	if (g_hash_table_lookup (canonical_namespaces, prefix) != NULL)
		return TRUE;
	return FALSE;
}

/**
 * gdata_parsable_get_xml:
 * @self: a #GDataParsable
 *
 * Builds an XML representation of the #GDataParsable in its current state, such that it could be inserted on the server. The XML is guaranteed
 * to have all its namespaces declared properly in a self-contained fashion, and is valid for stand-alone use.
 *
 * Return value: the object's XML; free with g_free()
 *
 * Since: 0.4.0
 */
gchar *
gdata_parsable_get_xml (GDataParsable *self)
{
	GString *xml_string;

	g_return_val_if_fail (GDATA_IS_PARSABLE (self), NULL);

	xml_string = g_string_sized_new (1000);
	g_string_append (xml_string, "<?xml version='1.0' encoding='UTF-8'?>");
	_gdata_parsable_get_xml (self, xml_string, TRUE);
	return g_string_free (xml_string, FALSE);
}

/*
 * _gdata_parsable_get_xml:
 * @self: a #GDataParsable
 * @xml_string: a #GString to build the XML in
 * @declare_namespaces: %TRUE if all the namespaces used in the outputted XML should be declared in the opening tag of the root element,
 * %FALSE otherwise
 *
 * Builds an XML representation of the #GDataParsable in its current state, such that it could be inserted on the server. If @declare_namespaces is
 * %TRUE, the XML is guaranteed to have all its namespaces declared properly in a self-contained fashion, and is valid for stand-alone use. If
 * @declare_namespaces is %FALSE, none of the used namespaces are declared, and the XML is suitable for insertion into a larger XML tree.
 *
 * Return value: the object's XML; free with g_free()
 *
 * Since: 0.4.0
 */
void
_gdata_parsable_get_xml (GDataParsable *self, GString *xml_string, gboolean declare_namespaces)
{
	GDataParsableClass *klass;
	guint length;
	GHashTable *namespaces = NULL; /* shut up, gcc */

	g_return_if_fail (GDATA_IS_PARSABLE (self));
	g_return_if_fail (xml_string != NULL);

	klass = GDATA_PARSABLE_GET_CLASS (self);
	g_assert (klass->element_name != NULL);

	/* Get the namespaces the class uses */
	if (declare_namespaces == TRUE && klass->get_namespaces != NULL) {
		namespaces = g_hash_table_new (g_str_hash, g_str_equal);
		klass->get_namespaces (self, namespaces);

		/* Remove any duplicate extra namespaces */
		g_hash_table_foreach_remove (self->priv->extra_namespaces, (GHRFunc) filter_namespaces_cb, namespaces);
	}

	/* Build up the namespace list */
	if (klass->element_namespace != NULL)
		g_string_append_printf (xml_string, "<%s:%s", klass->element_namespace, klass->element_name);
	else
		g_string_append_printf (xml_string, "<%s", klass->element_name);

	/* We only include the normal namespaces if we're not at the top level of XML building */
	if (declare_namespaces == TRUE) {
		g_string_append (xml_string, " xmlns='http://www.w3.org/2005/Atom'");
		if (namespaces != NULL) {
			g_hash_table_foreach (namespaces, (GHFunc) build_namespaces_cb, xml_string);
			g_hash_table_destroy (namespaces);
		}
	}

	g_hash_table_foreach (self->priv->extra_namespaces, (GHFunc) build_namespaces_cb, xml_string);

	/* Add anything the class thinks is suitable */
	if (klass->pre_get_xml != NULL)
		klass->pre_get_xml (self, xml_string);
	g_string_append_c (xml_string, '>');

	/* Store the length before we close the opening tag, so we can determine whether to self-close later on */
	length = xml_string->len;

	/* Add the rest of the XML */
	if (klass->get_xml != NULL)
		klass->get_xml (self, xml_string);

	/* Any extra XML? */
	if (self->priv->extra_xml != NULL && self->priv->extra_xml->str != NULL)
		g_string_append (xml_string, self->priv->extra_xml->str);

	/* Close the element; either by self-closing the opening tag, or by writing out a closing tag */
	if (xml_string->len == length)
		g_string_overwrite (xml_string, length - 1, "/>");
	else if (klass->element_namespace != NULL)
		g_string_append_printf (xml_string, "</%s:%s>", klass->element_namespace, klass->element_name);
	else
		g_string_append_printf (xml_string, "</%s>", klass->element_name);
}

/**
 * gdata_parsable_get_json:
 * @self: a #GDataParsable
 *
 * Builds a JSON representation of the #GDataParsable in its current state, such that it could be inserted on the server. The JSON
 * is valid for stand-alone use.
 *
 * Return value: the object's JSON; free with g_free()
 *
 * Since: 0.15.0
 */
gchar *
gdata_parsable_get_json (GDataParsable *self)
{
	JsonGenerator *generator;
	JsonBuilder *builder;
	JsonNode *root;
	gchar *output;

	g_return_val_if_fail (GDATA_IS_PARSABLE (self), NULL);

	/* Build the JSON tree. */
	builder = json_builder_new ();
	_gdata_parsable_get_json (self, builder);
	root = json_builder_get_root (builder);
	g_object_unref (builder);

	/* Serialise it to a string. */
	generator = json_generator_new ();
	json_generator_set_root (generator, root);
	output = json_generator_to_data (generator, NULL);
	g_object_unref (generator);

	json_node_free (root);

	return output;
}

/*
 * _gdata_parsable_get_json:
 * @self: a #GDataParsable
 * @builder: a #JsonBuilder to build the JSON in
 *
 * Builds a JSON representation of the #GDataParsable in its current state, such that it could be inserted on the server.
 *
 * Since: 0.15.0
 */
void
_gdata_parsable_get_json (GDataParsable *self, JsonBuilder *builder)
{
	GDataParsableClass *klass;
	GHashTableIter iter;
	gchar *member_name;
	JsonNode *value;

	g_return_if_fail (GDATA_IS_PARSABLE (self));
	g_return_if_fail (JSON_IS_BUILDER (builder));

	klass = GDATA_PARSABLE_GET_CLASS (self);

	json_builder_begin_object (builder);

	/* Add the JSON. */
	if (klass->get_json != NULL)
		klass->get_json (self, builder);

	/* Any extra JSON which we couldn't parse before? */
	g_hash_table_iter_init (&iter, self->priv->extra_json);
	while (g_hash_table_iter_next (&iter, (gpointer *) &member_name, (gpointer *) &value) == TRUE) {
		json_builder_set_member_name (builder, member_name);
		json_builder_add_value (builder, json_node_copy (value)); /* transfers ownership */
	}

	json_builder_end_object (builder);
}

/*
 * _gdata_parsable_is_constructed_from_xml:
 * @self: a #GDataParsable
 *
 * Returns the value of #GDataParsable:constructed-from-xml.
 *
 * Return value: %TRUE if the #GDataParsable was constructed from XML, %FALSE otherwise
 *
 * Since: 0.7.0
 */
gboolean
_gdata_parsable_is_constructed_from_xml (GDataParsable *self)
{
	g_return_val_if_fail (GDATA_IS_PARSABLE (self), FALSE);
	return self->priv->constructed_from_xml;
}
