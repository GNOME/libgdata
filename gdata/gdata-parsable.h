/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009 <philip@tecnocode.co.uk>
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

#ifndef GDATA_PARSABLE_H
#define GDATA_PARSABLE_H

#include <glib.h>
#include <glib-object.h>
#include <libxml/parser.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

/**
 * GDataParserError:
 * @GDATA_PARSER_ERROR_PARSING_STRING: Error parsing the XML or JSON syntax itself
 * @GDATA_PARSER_ERROR_EMPTY_DOCUMENT: Empty document
 *
 * Error codes for XML or JSON parsing operations.
 */
typedef enum {
	GDATA_PARSER_ERROR_PARSING_STRING = 1,
	GDATA_PARSER_ERROR_EMPTY_DOCUMENT
} GDataParserError;

#define GDATA_PARSER_ERROR gdata_parser_error_quark ()
GQuark gdata_parser_error_quark (void) G_GNUC_CONST;

#define GDATA_TYPE_PARSABLE		(gdata_parsable_get_type ())
#define GDATA_PARSABLE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_PARSABLE, GDataParsable))
#define GDATA_PARSABLE_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_PARSABLE, GDataParsableClass))
#define GDATA_IS_PARSABLE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_PARSABLE))
#define GDATA_IS_PARSABLE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_PARSABLE))
#define GDATA_PARSABLE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_PARSABLE, GDataParsableClass))

typedef struct _GDataParsablePrivate	GDataParsablePrivate;

/**
 * GDataParsable:
 *
 * All the fields in the #GDataParsable structure are private and should never be accessed directly.
 *
 * Since: 0.3.0
 */
typedef struct {
	GObject parent;
	GDataParsablePrivate *priv;
} GDataParsable;

/**
 * GDataParsableClass:
 * @parent: the parent class
 * @pre_parse_xml: a function called before parsing of an XML tree is started, which allows properties from the root node to be extracted
 * and used in @parsable
 * @parse_xml: a function to parse an XML representation of the #GDataParsable to set the properties of the @parsable
 * @post_parse_xml: a function called after parsing an XML tree, to allow the @parsable to validate the parsed properties
 * @pre_get_xml: a function called before building the XML representation of the children of the #GDataParsable, which allows attributes of the root
 * XML node to be added to @xml_string
 * @get_xml: a function to build an XML representation of the #GDataParsable in its current state, appending it to the provided #GString
 * @get_namespaces: a function to return a string containing the namespace declarations used by the @parsable when represented in XML form
 * @parse_json: a function to parse a JSON representation of the #GDataParsable to set the properties of the @parsable
 * @post_parse_json: a function called after parsing a JSON object, to allow the @parsable to validate the parsed properties
 * @get_json: a function to build a JSON representation of the #GDataParsable in its current state, appending it to the provided #JsonBuilder
 * @get_content_type: a function which returns content type upon which is #GDataParsable built
 * @element_name: the name of the XML element which represents this parsable
 * @element_namespace: the prefix of the XML namespace used for the parsable
 *
 * The class structure for the #GDataParsable class. Note that JSON and XML functions are mutually exclusive:
 * a given implementation of #GDataParsable is represented as exactly one of JSON and XML.
 *
 * Since: 0.3.0
 */
typedef struct {
	GObjectClass parent;

	gboolean (*pre_parse_xml) (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
	gboolean (*parse_xml) (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
	gboolean (*post_parse_xml) (GDataParsable *parsable, gpointer user_data, GError **error);

	void (*pre_get_xml) (GDataParsable *parsable, GString *xml_string);
	void (*get_xml) (GDataParsable *parsable, GString *xml_string);
	void (*get_namespaces) (GDataParsable *parsable, GHashTable *namespaces);

	gboolean (*parse_json) (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);
	gboolean (*post_parse_json) (GDataParsable *parsable, gpointer user_data, GError **error);
	void (*get_json) (GDataParsable *parsable, JsonBuilder *builder);

	const gchar *(*get_content_type) (void);

	const gchar *element_name;
	const gchar *element_namespace;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
	void (*_g_reserved2) (void);
	void (*_g_reserved3) (void);
	void (*_g_reserved4) (void);
	void (*_g_reserved5) (void);
	void (*_g_reserved6) (void);
	void (*_g_reserved7) (void);
} GDataParsableClass;

GType gdata_parsable_get_type (void) G_GNUC_CONST;

const gchar *gdata_parsable_get_content_type (GDataParsable *self);

GDataParsable *gdata_parsable_new_from_xml (GType parsable_type, const gchar *xml, gint length,
                                            GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
gchar *gdata_parsable_get_xml (GDataParsable *self) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataParsable *gdata_parsable_new_from_json (GType parsable_type, const gchar *json, gint length,
                                             GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
gchar *gdata_parsable_get_json (GDataParsable *self) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_PARSABLE_H */
