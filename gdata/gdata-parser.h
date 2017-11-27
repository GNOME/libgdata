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

#include <glib.h>

#include "gdata-parsable.h"
#include "gdata-types.h"

#ifndef GDATA_PARSER_H
#define GDATA_PARSER_H

G_BEGIN_DECLS

gboolean gdata_parser_error_required_content_missing (xmlNode *element, GError **error);
gboolean gdata_parser_error_not_iso8601_format (xmlNode *element, const gchar *actual_value, GError **error);
gboolean gdata_parser_error_unknown_property_value (xmlNode *element, const gchar *property_name, const gchar *actual_value, GError **error);
gboolean gdata_parser_error_unknown_content (xmlNode *element, const gchar *actual_content, GError **error);
gboolean gdata_parser_error_required_property_missing (xmlNode *element, const gchar *property_name, GError **error);
gboolean gdata_parser_error_mutexed_properties (xmlNode *element, const gchar *property1_name, const gchar *property2_name, GError **error);
gboolean gdata_parser_error_required_element_missing (const gchar *element_name, const gchar *parent_element_name, GError **error);
gboolean gdata_parser_error_duplicate_element (xmlNode *element, GError **error);

gboolean gdata_parser_error_duplicate_json_element (JsonReader *reader, GError **error);
gboolean gdata_parser_error_required_json_content_missing (JsonReader *reader, GError **error);
gboolean gdata_parser_error_not_iso8601_format_json (JsonReader *reader, const gchar *actual_value, GError **error);
gboolean
gdata_parser_error_from_json_error (JsonReader *reader,
                                    const GError *json_error, GError **error);

gboolean gdata_parser_int64_from_date (const gchar *date, gint64 *_time);
gchar *gdata_parser_date_from_int64 (gint64 _time) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
gchar *gdata_parser_int64_to_iso8601 (gint64 _time) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
gboolean gdata_parser_int64_from_iso8601 (const gchar *date, gint64 *_time);

/*
 * GDataParserOptions:
 * @P_NONE: no special options; the content of the element will be outputted directly without any checks
 * @P_NO_DUPES: the element must be encountered at most once; if encountered more than once, an error will be returned
 * @P_REQUIRED: the element content must not be %NULL if the element exists
 * @P_NON_EMPTY: the element content must not be empty (i.e. a zero-length string) if the element exists;
 * this only applies to gdata_parser_string_from_element()
 * @P_DEFAULT: if the element content is %NULL or empty, return an empty value instead of erroring (this is mutually exclusive with %P_REQUIRED
 * and %P_NON_EMPTY)
 * @P_IGNORE_ERROR: ignore any error when the parse fails; can be used to skip empty values (this is mutually exclusive with %P_REQUIRED)
 *
 * Parsing options to be passed in a bitwise fashion to gdata_parser_string_from_element() or gdata_parser_object_from_element().
 * Their names aren't namespaced as they aren't public, and brevity is important, since they're used frequently in the parsing code.
 *
 * Since: 0.7.0
 */
typedef enum {
	P_NONE = 0,
	P_NO_DUPES = 1 << 0,
	P_REQUIRED = 1 << 1,
	P_NON_EMPTY = 1 << 2,
	P_DEFAULT = 1 << 3,
	P_IGNORE_ERROR = 1 << 4
} GDataParserOptions;

typedef void (*GDataParserSetterFunc) (GDataParsable *parent_parsable, GDataParsable *parsable);

gboolean gdata_parser_boolean_from_property (xmlNode *element, const gchar *property_name, gboolean *output, gint default_output, GError **error);

gboolean gdata_parser_is_namespace (xmlNode *element, const gchar *namespace_uri);

gboolean gdata_parser_string_from_element (xmlNode *element, const gchar *element_name, GDataParserOptions options,
                                           gchar **output, gboolean *success, GError **error);
gboolean gdata_parser_int64_time_from_element (xmlNode *element, const gchar *element_name, GDataParserOptions options,
                                               gint64 *output, gboolean *success, GError **error);
gboolean gdata_parser_int64_from_element (xmlNode *element, const gchar *element_name, GDataParserOptions options,
                                          gint64 *output, gint64 default_output, gboolean *success, GError **error);
gboolean gdata_parser_object_from_element_setter (xmlNode *element, const gchar *element_name, GDataParserOptions options, GType object_type,
                                                  gpointer /* GDataParserSetterFunc */ _setter, gpointer /* GDataParsable * */ _parent_parsable,
                                                  gboolean *success, GError **error);
gboolean gdata_parser_object_from_element (xmlNode *element, const gchar *element_name, GDataParserOptions options, GType object_type,
                                           gpointer /* GDataParsable ** */ _output, gboolean *success, GError **error);
gboolean gdata_parser_string_from_json_member (JsonReader *reader, const gchar *member_name, GDataParserOptions options,
                                               gchar **output, gboolean *success, GError **error);
gboolean
gdata_parser_int_from_json_member (JsonReader *reader,
                                   const gchar *member_name,
                                   GDataParserOptions options,
                                   gint64 *output, gboolean *success,
                                   GError **error);
gboolean gdata_parser_int64_time_from_json_member (JsonReader *reader, const gchar *member_name, GDataParserOptions options,
                                                   gint64 *output, gboolean *success, GError **error);
gboolean gdata_parser_boolean_from_json_member (JsonReader *reader, const gchar *member_name, GDataParserOptions options,
                                                gboolean *output, gboolean *success, GError **error);
gboolean
gdata_parser_strv_from_json_member (JsonReader *reader,
                                    const gchar *member_name,
                                    GDataParserOptions options,
                                    gchar ***output, gboolean *success,
                                    GError **error);
gboolean
gdata_parser_color_from_json_member (JsonReader *reader,
                                     const gchar *member_name,
                                     GDataParserOptions options,
                                     GDataColor *output,
                                     gboolean *success,
                                     GError **error);

void gdata_parser_string_append_escaped (GString *xml_string, const gchar *pre, const gchar *element_content, const gchar *post);
gchar *gdata_parser_utf8_trim_whitespace (const gchar *s) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_PARSER_H */
