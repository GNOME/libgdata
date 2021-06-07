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

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <libxml/parser.h>
#include <json-glib/json-glib.h>

#include "gdata-parser.h"
#include "gdata-service.h"
#include "gdata-types.h"
#include "gdata-private.h"

static gchar *
print_element (xmlNode *node)
{
	gboolean node_has_ns = (node->ns == NULL || node->ns->prefix == NULL ||
	                        xmlStrcmp (node->ns->href, (xmlChar*) "http://www.w3.org/2005/Atom") == 0) ? FALSE : TRUE;

	if (node->parent == NULL) {
		/* No parent node */
		if (node_has_ns == TRUE)
			return g_strdup_printf ("<%s:%s>", node->ns->prefix, node->name);
		else
			return g_strdup_printf ("<%s>", node->name);
	} else {
		/* We have a parent node, which makes things a lot more complex */
		gboolean parent_has_ns = (node->parent->type == XML_DOCUMENT_NODE || node->parent->ns == NULL || node->parent->ns->prefix == NULL ||
		                          xmlStrcmp (node->parent->ns->href, (xmlChar*) "http://www.w3.org/2005/Atom") == 0) ? FALSE : TRUE;

		if (parent_has_ns == TRUE && node_has_ns == TRUE)
			return g_strdup_printf ("<%s:%s/%s:%s>", node->parent->ns->prefix, node->parent->name, node->ns->prefix, node->name);
		else if (parent_has_ns == FALSE && node_has_ns == TRUE)
			return g_strdup_printf ("<%s/%s:%s>", node->parent->name, node->ns->prefix, node->name);
		else
			return g_strdup_printf ("<%s/%s>", node->parent->name, node->name);
	}
}

gboolean
gdata_parser_error_required_content_missing (xmlNode *element, GError **error)
{
	gchar *element_string = print_element (element);

	/* Translators: the parameter is the name of an XML element, including the angle brackets ("<" and ">").
	 *
	 * For example:
	 *  A <title> element was missing required content. */
	g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR, _("A %s element was missing required content."), element_string);
	g_free (element_string);

	return FALSE;
}

gboolean
gdata_parser_error_not_iso8601_format (xmlNode *element, const gchar *actual_value, GError **error)
{
	gchar *element_string = print_element (element);
	g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
	             /* Translators: the first parameter is the name of an XML element (including the angle brackets ("<" and ">")),
	              * and the second parameter is the erroneous value (which was not in ISO 8601 format).
	              *
	              * For example:
	              *  The content of a <media:group/media:uploaded> element (‘2009-05-06 26:30Z’) was not in ISO 8601 format. */
	             _("The content of a %s element (‘%s’) was not in ISO 8601 format."), element_string, actual_value);
	g_free (element_string);

	return FALSE;
}

gboolean
gdata_parser_error_unknown_property_value (xmlNode *element, const gchar *property_name, const gchar *actual_value, GError **error)
{
	gchar *property_string, *element_string;

	property_string = g_strdup_printf ("@%s", property_name);
	element_string = print_element (element);

	g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
	             /* Translators: the first parameter is the name of an XML property, the second is the name of an XML element
	              * (including the angle brackets ("<" and ">")) to which the property belongs, and the third is the unknown value.
	              *
	              * For example:
	              *  The value of the @time property of a <media:group/media:thumbnail> element (‘00:01:42.500’) was unknown. */
	             _("The value of the %s property of a %s element (‘%s’) was unknown."), property_string, element_string, actual_value);
	g_free (property_string);
	g_free (element_string);

	return FALSE;
}

gboolean
gdata_parser_error_unknown_content (xmlNode *element, const gchar *actual_content, GError **error)
{
	gchar *element_string = print_element (element);

	g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
	             /* Translators: the first parameter is the name of an XML element (including the angle brackets ("<" and ">")),
	              * and the second parameter is the unknown content of that element.
	              *
	              * For example:
	              *  The content of a <gphoto:access> element (‘protected’) was unknown. */
	             _("The content of a %s element (‘%s’) was unknown."), element_string, actual_content);
	g_free (element_string);

	return FALSE;
}

gboolean
gdata_parser_error_required_property_missing (xmlNode *element, const gchar *property_name, GError **error)
{
	gchar *property_string, *element_string;

	property_string = g_strdup_printf ("@%s", property_name);
	element_string = print_element (element);

	g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
	             /* Translators: the first parameter is the name of an XML element (including the angle brackets ("<" and ">")),
	              * and the second is the name of an XML property which it should have contained.
	              *
	              * For example:
	              *  A required property of a <entry/gAcl:role> element (@value) was not present. */
	             _("A required property of a %s element (%s) was not present."), element_string, property_string);
	g_free (property_string);
	g_free (element_string);

	return FALSE;
}

gboolean
gdata_parser_error_mutexed_properties (xmlNode *element, const gchar *property1_name, const gchar *property2_name, GError **error)
{
	gchar *property1_string, *property2_string, *element_string;

	property1_string = g_strdup_printf ("@%s", property1_name);
	property2_string = g_strdup_printf ("@%s", property2_name);
	element_string = print_element (element);

	g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
	             /* Translators: the first two parameters are the names of XML properties of an XML element given in the third
	              * parameter (including the angle brackets ("<" and ">")).
	              *
	              * For example:
	              *  Values were present for properties @rel and @label of a <entry/gContact:relation> element when only one of the
	              *  two is allowed. */
	             _("Values were present for properties %s and %s of a %s element when only one of the two is allowed."),
	             property1_string, property2_string, element_string);
	g_free (property1_string);
	g_free (property2_string);
	g_free (element_string);

	return FALSE;
}

gboolean
gdata_parser_error_required_element_missing (const gchar *element_name, const gchar *parent_element_name, GError **error)
{
	/* NOTE: This can't take an xmlNode, since such a node wouldn't exist. */
	gchar *element_string = g_strdup_printf ("<%s/%s>", parent_element_name, element_name);

	/* Translators: the parameter is the name of an XML element, including the angle brackets ("<" and ">").
	 *
	 * For example:
	 *  A required element (<entry/title>) was not present. */
	g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR, _("A required element (%s) was not present."), element_string);
	g_free (element_string);

	return FALSE;
}

gboolean
gdata_parser_error_duplicate_element (xmlNode *element, GError **error)
{
	gchar *element_string = print_element (element);

	/* Translators: the parameter is the name of an XML element, including the angle brackets ("<" and ">").
	 *
	 * For example:
	 *  A singleton element (<feed/title>) was duplicated. */
	g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR, _("A singleton element (%s) was duplicated."), element_string);
	g_free (element_string);

	return FALSE;
}

gboolean
gdata_parser_int64_from_date (const gchar *date, gint64 *_time)
{
	gchar *iso8601_date;
	g_autoptr(GDateTime) time_val = NULL;

	if (strlen (date) != 10 && strlen (date) != 8)
		return FALSE;

	/* Note: This doesn't need translating, as it's outputting an ISO 8601 time string */
	iso8601_date = g_strdup_printf ("%sT00:00:00Z", date);
	time_val = g_date_time_new_from_iso8601 (iso8601_date, NULL);
	g_free (iso8601_date);

	if (time_val) {
		*_time = g_date_time_to_unix (time_val);
		return TRUE;
	}

	return FALSE;
}

gchar *
gdata_parser_date_from_int64 (gint64 _time)
{
	time_t secs;
	struct tm *tm;

	secs = _time;
	tm = gmtime (&secs);

	/* Note: This doesn't need translating, as it's outputting an ISO 8601 date string */
	return g_strdup_printf ("%04d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
}

gchar *
gdata_parser_int64_to_iso8601 (gint64 _time)
{
	g_autoptr(GDateTime) time_val = NULL;

	time_val = g_date_time_new_from_unix_utc (_time);

	if (!time_val)
		return NULL;

	return g_date_time_format_iso8601 (time_val);
}

gboolean
gdata_parser_int64_from_iso8601 (const gchar *date, gint64 *_time)
{
	g_autoptr(GDateTime) time_val = NULL;

	time_val = g_date_time_new_from_iso8601 (date, NULL);
	if (time_val) {
		*_time = g_date_time_to_unix (time_val);
		return TRUE;
	}

	return FALSE;
}

gboolean
gdata_parser_error_required_json_content_missing (JsonReader *reader, GError **error)
{
	const gchar *element_string = json_reader_get_member_name (reader);

	/* Translators: the parameter is the name of an JSON element.
	 *
	 * For example:
	 *  A ‘title’ element was missing required content. */
	g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR, _("A ‘%s’ element was missing required content."), element_string);

	return FALSE;
}

gboolean
gdata_parser_error_duplicate_json_element (JsonReader *reader, GError **error)
{
	const gchar *element_string = json_reader_get_member_name (reader);

	/* Translators: the parameter is the name of an JSON element.
	 *
	 * For example:
	 *  A singleton element (title) was duplicated. */
	g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR, _("A singleton element (%s) was duplicated."), element_string);

	return FALSE;
}

gboolean
gdata_parser_error_not_iso8601_format_json (JsonReader *reader, const gchar *actual_value, GError **error)
{
	const gchar *element_string = json_reader_get_member_name (reader);

	g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
	             /* Translators: the first parameter is the name of an JSON element,
	              * and the second parameter is the erroneous value (which was not in ISO 8601 format).
	              *
	              * For example:
	              *  The content of a ‘uploaded’ element (‘2009-05-06 26:30Z’) was not in ISO 8601 format. */
	             _("The content of a ‘%s’ element (‘%s’) was not in ISO 8601 format."), element_string, actual_value);

	return FALSE;
}

gboolean
gdata_parser_error_from_json_error (JsonReader *reader,
                                    const GError *json_error, GError **error)
{
	g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
	             /* Translators: the parameter is an error message. */
	             _("Invalid JSON was received from the server: %s"), json_error->message);

	return FALSE;
}

/*
 * gdata_parser_boolean_from_property:
 * @element: the XML element which owns the property to parse
 * @property_name: the name of the property to parse
 * @output: the return location for the parsed boolean value
 * @default_output: the default value to return in @output if the property can't be found
 * @error: a #GError, or %NULL
 *
 * Parses a GData boolean value from the property @property_name of @element.
 * The boolean value should be of the form: "<element property_name='[true|false]'/>".
 * A %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned in @error if parsing fails, and @output will not be set.
 *
 * If no property with the name @property_name exists in @element and @default_output is <code class="literal">0</code>,
 * @output will be set to %FALSE.
 * If @default_output is <code class="literal">1</code>, @output will be set to %TRUE. If @default_output is <code class="literal">-1</code>,
 * a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR will be returned in @error.
 *
 * Return value: %TRUE on successful parsing, %FALSE otherwise
 *
 * Since: 0.7.0
 */
gboolean
gdata_parser_boolean_from_property (xmlNode *element, const gchar *property_name, gboolean *output, gint default_output, GError **error)
{
	xmlChar *value = xmlGetProp (element, (xmlChar*) property_name);

	if (value == NULL) {
		/* Missing property */
		if (default_output == -1)
			return gdata_parser_error_required_property_missing (element, property_name, error);
		*output = (default_output == 1) ? TRUE : FALSE;
	} else if (xmlStrcmp (value, (xmlChar*) "false") == 0) {
		*output = FALSE;
	} else if (xmlStrcmp (value, (xmlChar*) "true") == 0) {
		*output = TRUE;
	} else {
		/* Parsing failed */
		gdata_parser_error_unknown_property_value (element, property_name, (gchar*) value, error);
		xmlFree (value);
		return FALSE;
	}

	xmlFree (value);
	return TRUE;
}

/*
 * gdata_parser_is_namespace:
 * @element: the element to check
 * @namespace_uri: the URI of the desired namespace
 *
 * Checks whether @element is in the namespace identified by @namespace_uri. If @element isn't in a namespace,
 * %FALSE is returned.
 *
 * Return value: %TRUE if @element is in @namespace_uri; %FALSE otherwise
 *
 * Since: 0.7.0
 */
gboolean
gdata_parser_is_namespace (xmlNode *element, const gchar *namespace_uri)
{
	if ((element->ns != NULL && xmlStrcmp (element->ns->href, (const xmlChar*) namespace_uri) == 0) ||
	    (element->ns == NULL && strcmp (namespace_uri, "http://www.w3.org/2005/Atom") == 0))
		return TRUE;
	return FALSE;
}

/*
 * gdata_parser_string_from_element:
 * @element: the element to check against
 * @element_name: the name of the element to parse
 * @options: a bitwise combination of parsing options from #GDataParserOptions, or %P_NONE
 * @output: the return location for the parsed string content
 * @success: the return location for a value which is %TRUE if the string was parsed successfully, %FALSE if an error was encountered,
 * and undefined if @element didn't match @element_name
 * @error: a #GError, or %NULL
 *
 * Gets the string content of @element if its name is @element_name, subject to various checks specified by @options.
 *
 * If @element doesn't match @element_name, %FALSE will be returned, @error will be unset and @success will be unset.
 *
 * If @element matches @element_name but one of the checks specified by @options fails, %TRUE will be returned, @error will be set to a
 * %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error and @success will be set to %FALSE.
 *
 * If @element matches @element_name and all of the checks specified by @options pass, %TRUE will be returned, @error will be unset and
 * @success will be set to %TRUE.
 *
 * The reason for returning the success of the parsing in @success is so that calls to gdata_parser_string_from_element() can be chained
 * together in a large "or" statement based on their return values, for the purposes of determining whether any of the calls matched
 * a given @element. If any of the calls to gdata_parser_string_from_element() return %TRUE, the value of @success can be examined.
 *
 * Return value: %TRUE if @element matched @element_name, %FALSE otherwise
 *
 * Since: 0.7.0
 */
gboolean
gdata_parser_string_from_element (xmlNode *element, const gchar *element_name, GDataParserOptions options,
                                  gchar **output, gboolean *success, GError **error)
{
	xmlChar *text;

	/* Check it's the right element */
	if (xmlStrcmp (element->name, (xmlChar*) element_name) != 0)
		return FALSE;

	/* Check if the output string has already been set */
	if (options & P_NO_DUPES && *output != NULL) {
		*success = gdata_parser_error_duplicate_element (element, error);
		return TRUE;
	}

	/* Get the string and check it for NULLness or emptiness */
	text = xmlNodeListGetString (element->doc, element->children, TRUE);
	if ((options & P_REQUIRED && text == NULL) || (options & P_NON_EMPTY && text != NULL && *text == '\0')) {
		xmlFree (text);
		*success = gdata_parser_error_required_content_missing (element, error);
		return TRUE;
	} else if (options & P_DEFAULT && (text == NULL || *text == '\0')) {
		text = (xmlChar*) g_strdup ("");
	}

	/* Success! */
	g_free (*output);
	*output = (gchar*) text;
	*success = TRUE;

	return TRUE;
}

/*
 * gdata_parser_int64_time_from_element:
 * @element: the element to check against
 * @element_name: the name of the element to parse
 * @options: a bitwise combination of parsing options from #GDataParserOptions, or %P_NONE
 * @output: (out caller-allocates): the return location for the parsed time value
 * @success: the return location for a value which is %TRUE if the time val was parsed successfully, %FALSE if an error was encountered,
 * and undefined if @element didn't match @element_name
 * @error: a #GError, or %NULL
 *
 * Gets the time value of @element if its name is @element_name, subject to various checks specified by @options. It expects the text content
 * of @element to be a date or time value in ISO 8601 format. The returned time value will be a UNIX timestamp (seconds since the epoch).
 *
 * If @element doesn't match @element_name, %FALSE will be returned, @error will be unset and @success will be unset.
 *
 * If @element matches @element_name but one of the checks specified by @options fails, %TRUE will be returned, @error will be set to a
 * %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error and @success will be set to %FALSE.
 *
 * If @element matches @element_name and all of the checks specified by @options pass, %TRUE will be returned, @error will be unset and
 * @success will be set to %TRUE.
 *
 * The reason for returning the success of the parsing in @success is so that calls to gdata_parser_int64_time_from_element() can be chained
 * together in a large "or" statement based on their return values, for the purposes of determining whether any of the calls matched
 * a given @element. If any of the calls to gdata_parser_int64_time_from_element() return %TRUE, the value of @success can be examined.
 *
 * Return value: %TRUE if @element matched @element_name, %FALSE otherwise
 *
 * Since: 0.7.0
 */
gboolean
gdata_parser_int64_time_from_element (xmlNode *element, const gchar *element_name, GDataParserOptions options,
                                      gint64 *output, gboolean *success, GError **error)
{
	xmlChar *text;
	g_autoptr(GDateTime) time_val = NULL;

	/* Check it's the right element */
	if (xmlStrcmp (element->name, (xmlChar*) element_name) != 0)
		return FALSE;

	/* Check if the output time val has already been set */
	if (options & P_NO_DUPES && *output != -1) {
		*success = gdata_parser_error_duplicate_element (element, error);
		return TRUE;
	}

	/* Get the string and check it for NULLness */
	text = xmlNodeListGetString (element->doc, element->children, TRUE);
	if (options & P_REQUIRED && (text == NULL || *text == '\0')) {
		xmlFree (text);
		*success = gdata_parser_error_required_content_missing (element, error);
		return TRUE;
	}

	/* Attempt to parse the string as a GDateTune */
	time_val = g_date_time_new_from_iso8601 ((gchar *) text, NULL);
	if (!time_val) {
		*success = gdata_parser_error_not_iso8601_format (element, (gchar*) text, error);
		xmlFree (text);
		return TRUE;
	}

	*output = g_date_time_to_unix (time_val);

	/* Success! */
	xmlFree (text);
	*success = TRUE;

	return TRUE;
}

/*
 * gdata_parser_int64_from_element:
 * @element: the element to check against
 * @element_name: the name of the element to parse
 * @options: a bitwise combination of parsing options from #GDataParserOptions, or %P_NONE
 * @output: (out caller-allocates): the return location for the parsed integer value
 * @default_output: default value for the property, used with %P_NO_DUPES to detect duplicates
 * @success: the return location for a value which is %TRUE if the integer was parsed successfully, %FALSE if an error was encountered,
 * and undefined if @element didn't match @element_name
 * @error: a #GError, or %NULL
 *
 * Gets the integer value of @element if its name is @element_name, subject to various checks specified by @options. It expects the text content
 * of @element to be an integer in base 10 format.
 *
 * If @element doesn't match @element_name, %FALSE will be returned, @error will be unset and @success will be unset.
 *
 * If @element matches @element_name but one of the checks specified by @options fails, %TRUE will be returned, @error will be set to a
 * %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error and @success will be set to %FALSE.
 *
 * If @element matches @element_name and all of the checks specified by @options pass, %TRUE will be returned, @error will be unset and
 * @success will be set to %TRUE.
 *
 * The reason for returning the success of the parsing in @success is so that calls to gdata_parser_int64_from_element() can be chained
 * together in a large "or" statement based on their return values, for the purposes of determining whether any of the calls matched
 * a given @element. If any of the calls to gdata_parser_int64_from_element() return %TRUE, the value of @success can be examined.
 *
 * Return value: %TRUE if @element matched @element_name, %FALSE otherwise
 *
 * Since: 0.13.0
 */
gboolean
gdata_parser_int64_from_element (xmlNode *element, const gchar *element_name, GDataParserOptions options,
                                 gint64 *output, gint64 default_output, gboolean *success, GError **error)
{
	xmlChar *text;
	gchar *end_ptr;
	gint64 val;

	/* Check it's the right element */
	if (xmlStrcmp (element->name, (xmlChar*) element_name) != 0) {
		return FALSE;
	}

	/* Check if the output value has already been set */
	if (options & P_NO_DUPES && *output != default_output) {
		*success = gdata_parser_error_duplicate_element (element, error);
		return TRUE;
	}

	/* Get the string and check it for NULLness */
	text = xmlNodeListGetString (element->doc, element->children, TRUE);
	if (options & P_REQUIRED && (text == NULL || *text == '\0')) {
		xmlFree (text);
		*success = gdata_parser_error_required_content_missing (element, error);
		return TRUE;
	}

	/* Attempt to parse the string as a 64-bit integer */
	val = g_ascii_strtoll ((const gchar*) text, &end_ptr, 10);
	if (*end_ptr != '\0') {
		*success = gdata_parser_error_unknown_content (element, (gchar*) text, error);
		xmlFree (text);
		return TRUE;
	}

	*output = val;

	/* Success! */
	xmlFree (text);
	*success = TRUE;

	return TRUE;
}

/*
 * gdata_parser_object_from_element_setter:
 * @element: the element to check against
 * @element_name: the name of the element to parse
 * @options: a bitwise combination of parsing options from #GDataParserOptions, or %P_NONE
 * @object_type: the type of the object to parse
 * @_setter: a function to call once parsing's finished to return the object (#GDataParserSetterFunc)
 * @_parent_parsable: the first parameter to pass to @_setter (of type #GDataParsable)
 * @success: the return location for a value which is %TRUE if the object was parsed successfully, %FALSE if an error was encountered,
 * and undefined if @element didn't match @element_name
 * @error: a #GError, or %NULL
 *
 * Gets the object content of @element if its name is @element_name, subject to various checks specified by @options. If @element matches
 * @element_name, @element will be parsed as an @object_type, which must extend #GDataParsable. If parsing is successful, @_setter will be called
 * with its first parameter as @_parent_parsable, and its second as the parsed object of type @object_type. @_setter must reference the parsed object
 * it's passed if it wants to keep it, as gdata_parser_object_from_element_setter will unreference it before returning.
 *
 * If @element doesn't match @element_name, %FALSE will be returned, @error will be unset and @success will be unset.
 *
 * If @element matches @element_name but one of the checks specified by @options fails, %TRUE will be returned, @error will be set to a
 * %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error and @success will be set to %FALSE.
 *
 * If @element matches @element_name and all of the checks specified by @options pass, %TRUE will be returned, @error will be unset and
 * @success will be set to %TRUE.
 *
 * The reason for returning the success of the parsing in @success is so that calls to gdata_parser_object_from_element_setter() can be chained
 * together in a large "or" statement based on their return values, for the purposes of determining whether any of the calls matched
 * a given @element. If any of the calls to gdata_parser_object_from_element_setter() return %TRUE, the value of @success can be examined.
 *
 * @_setter and @_parent_parsable are both #gpointers to avoid casts having to be put into calls to gdata_parser_object_from_element_setter().
 * @_setter is actually of type #GDataParserSetterFunc, and @_parent_parsable should be a subclass of #GDataParsable.
 * Neither parameter should be %NULL. No checks are implemented against these conditions (for efficiency reasons), so calling code must be correct.
 *
 * Return value: %TRUE if @element matched @element_name, %FALSE otherwise
 *
 * Since: 0.7.0
 */
gboolean
gdata_parser_object_from_element_setter (xmlNode *element, const gchar *element_name, GDataParserOptions options, GType object_type,
                                         gpointer /* GDataParserSetterFunc */ _setter, gpointer /* GDataParsable * */ _parent_parsable,
                                         gboolean *success, GError **error)
{
	GDataParsable *parsable, *parent_parsable;
	GDataParserSetterFunc setter;
	GError *local_error = NULL;

	g_return_val_if_fail (!(options & P_REQUIRED) || !(options & P_IGNORE_ERROR), FALSE);

	/* We're lax on the types so that we don't have to do loads of casting when calling the function, which makes the parsing code more legible */
	setter = (GDataParserSetterFunc) _setter;
	parent_parsable = (GDataParsable*) _parent_parsable;

	/* Check it's the right element */
	if (xmlStrcmp (element->name, (xmlChar*) element_name) != 0)
		return FALSE;

	/* Get the object and check for instantiation failure */
	parsable = _gdata_parsable_new_from_xml_node (object_type, element->doc, element, NULL, &local_error);
	g_assert ((parsable == NULL) == (local_error != NULL));

	if (options & P_REQUIRED && parsable == NULL) {
		/* The error has already been set by _gdata_parsable_new_from_xml_node() */
		g_propagate_error (error, g_steal_pointer (&local_error));
		*success = FALSE;
		return TRUE;
	} else if ((options & P_IGNORE_ERROR) && parsable == NULL) {
		g_clear_error (&local_error);
		*success = TRUE;
		return TRUE;
	} else if (local_error != NULL) {
		g_propagate_error (error, g_steal_pointer (&local_error));
		*success = FALSE;
		return TRUE;
	}

	/* Success! */
	setter (parent_parsable, parsable);
	g_object_unref (parsable);
	*success = TRUE;

	return TRUE;
}

/*
 * gdata_parser_object_from_element:
 * @element: the element to check against
 * @element_name: the name of the element to parse
 * @options: a bitwise combination of parsing options from #GDataParserOptions, or %P_NONE
 * @object_type: the type of the object to parse
 * @_output: the return location for the parsed object (of type #GDataParsable)
 * @success: the return location for a value which is %TRUE if the object was parsed successfully, %FALSE if an error was encountered,
 * and undefined if @element didn't match @element_name
 * @error: a #GError, or %NULL
 *
 * Gets the object content of @element if its name is @element_name, subject to various checks specified by @options. If @element matches
 * @element_name, @element will be parsed as an @object_type, which must extend #GDataParsable. If parsing is successful, the parsed object will be
 * returned in @_output, which must be of type #GDataParsable (or a subclass). Ownership of the parsed object will pass to the calling code.
 *
 * If @element doesn't match @element_name, %FALSE will be returned, @error will be unset and @success will be unset.
 *
 * If @element matches @element_name but one of the checks specified by @options fails, %TRUE will be returned, @error will be set to a
 * %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error and @success will be set to %FALSE.
 *
 * If @element matches @element_name and all of the checks specified by @options pass, %TRUE will be returned, @error will be unset and
 * @success will be set to %TRUE.
 *
 * The reason for returning the success of the parsing in @success is so that calls to gdata_parser_object_from_element() can be chained
 * together in a large "or" statement based on their return values, for the purposes of determining whether any of the calls matched
 * a given @element. If any of the calls to gdata_parser_object_from_element() return %TRUE, the value of @success can be examined.
 *
 * @_object is a #gpointer to avoid casts having to be put into calls to gdata_parser_object_from_element(). It is actually of type #GDataParsable
 * and must not be %NULL. No check is implemented against this condition (for efficiency reasons), so calling code must be correct.
 *
 * Return value: %TRUE if @element matched @element_name, %FALSE otherwise
 *
 * Since: 0.7.0
 */
gboolean
gdata_parser_object_from_element (xmlNode *element, const gchar *element_name, GDataParserOptions options, GType object_type,
                                  gpointer /* GDataParsable ** */ _output, gboolean *success, GError **error)
{
	GDataParsable *parsable, **output;

	/* We're lax on the types so that we don't have to do loads of casting when calling the function, which makes the parsing code more legible */
	output = (GDataParsable**) _output;

	/* Check it's the right element */
	if (xmlStrcmp (element->name, (xmlChar*) element_name) != 0)
		return FALSE;

	/* If we're not using a setter, check if the output already exists */
	if (options & P_NO_DUPES && *output != NULL) {
		*success = gdata_parser_error_duplicate_element (element, error);
		return TRUE;
	}

	/* Get the object and check for instantiation failure */
	parsable = _gdata_parsable_new_from_xml_node (object_type, element->doc, element, NULL, error);
	if (options & P_REQUIRED && parsable == NULL) {
		/* The error has already been set by _gdata_parsable_new_from_xml_node() */
		*success = FALSE;
		return TRUE;
	}

	/* Success! */
	if (*output != NULL)
		g_object_unref (*output);
	*output = parsable;
	*success = TRUE;

	return TRUE;
}

/*
 * gdata_parser_string_from_json_member:
 * @reader: #JsonReader cursor object to read JSON node from
 * @member_name: the name of the member to parse
 * @options: a bitwise combination of parsing options from #GDataParserOptions, or %P_NONE
 * @output: the return location for the parsed string content
 * @success: the return location for a value which is %TRUE if the string was parsed successfully, %FALSE if an error was encountered,
 * and undefined if @element didn't match @element_name
 * @error: a #GError, or %NULL
 *
 * Gets the string content of @element if its name is @element_name, subject to various checks specified by @options.
 *
 * If @element doesn't match @element_name, %FALSE will be returned, @error will be unset and @success will be unset.
 *
 * If @element matches @element_name but one of the checks specified by @options fails, %TRUE will be returned, @error will be set to a
 * %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error and @success will be set to %FALSE.
 *
 * If @element matches @element_name and all of the checks specified by @options pass, %TRUE will be returned, @error will be unset and
 * @success will be set to %TRUE.
 *
 * The reason for returning the success of the parsing in @success is so that calls to gdata_parser_string_from_element() can be chained
 * together in a large "or" statement based on their return values, for the purposes of determining whether any of the calls matched
 * a given @element. If any of the calls to gdata_parser_string_from_element() return %TRUE, the value of @success can be examined.
 *
 * Return value: %TRUE if @element matched @element_name, %FALSE otherwise
 *
 * Since: 0.15.0
 */
gboolean
gdata_parser_string_from_json_member (JsonReader *reader, const gchar *member_name, GDataParserOptions options,
                                      gchar **output, gboolean *success, GError **error)
{
	const gchar *text;
	const GError *child_error = NULL;

	/* Check if there's such element */
	if (g_strcmp0 (json_reader_get_member_name (reader), member_name) != 0) {
		return FALSE;
	}

	/* Check if the output string has already been set. The JSON parser guarantees this can't happen. */
	g_assert (!(options & P_NO_DUPES) || *output == NULL);

	/* Get the string and check it for NULLness or emptiness. Check for parser errors first. */
	text = json_reader_get_string_value (reader);
	child_error = json_reader_get_error (reader);
	if (child_error != NULL) {
		*success = gdata_parser_error_from_json_error (reader,
		                                               child_error,
		                                               error);
		return TRUE;
	} else if ((options & P_REQUIRED && text == NULL) || (options & P_NON_EMPTY && text != NULL && *text == '\0')) {
		*success = gdata_parser_error_required_json_content_missing (reader, error);
		return TRUE;
	} else if (options & P_DEFAULT && (text == NULL || *text == '\0')) {
		text = "";
	}

	/* Success! */
	g_free (*output);
	*output = g_strdup (text);
	*success = TRUE;

	return TRUE;
}

/*
 * gdata_parser_int_from_json_member:
 * @reader: #JsonReader cursor object to read JSON node from
 * @member_name: the name of the member to parse
 * @options: a bitwise combination of parsing options from #GDataParserOptions,
 *   or %P_NONE
 * @output: the return location for the parsed integer content
 * @success: the return location for a value which is %TRUE if the integer was
 *   parsed successfully, %FALSE if an error was encountered, and undefined if
 *   @element didn't match @element_name
 * @error: a #GError, or %NULL
 *
 * Gets the integer content of @element if its name is @element_name, subject to
 * various checks specified by @options.
 *
 * If @element doesn't match @element_name, %FALSE will be returned, @error will
 * be unset and @success will be unset.
 *
 * If @element matches @element_name but one of the checks specified by @options
 * fails, %TRUE will be returned, @error will be set to a
* %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error and @success will be set to %FALSE.
 *
 * If @element matches @element_name and all of the checks specified by @options
 * pass, %TRUE will be returned, @error will be unset and @success will be set
 * to %TRUE.
 *
 * The reason for returning the success of the parsing in @success is so that
 * calls to gdata_parser_int_from_element() can be chained together in a large
 * "or" statement based on their return values, for the purposes of determining
 * whether any of the calls matched a given @element. If any of the calls to
 * gdata_parser_int_from_element() return %TRUE, the value of @success can be
 * examined.
 *
 * Return value: %TRUE if @element matched @element_name, %FALSE otherwise
 *
 * Since: 0.17.0
 */
gboolean
gdata_parser_int_from_json_member (JsonReader *reader,
                                   const gchar *member_name,
                                   GDataParserOptions options,
                                   gint64 *output, gboolean *success,
                                   GError **error)
{
	gint64 value;
	const GError *child_error = NULL;

	/* Check if there's such element */
	if (g_strcmp0 (json_reader_get_member_name (reader), member_name) != 0) {
		return FALSE;
	}

	value = json_reader_get_int_value (reader);
	child_error = json_reader_get_error (reader);

	if (child_error != NULL) {
		*success = gdata_parser_error_from_json_error (reader,
		                                               child_error,
		                                               error);
		return TRUE;
	}

	/* Success! */
	*output = value;
	*success = TRUE;

	return TRUE;
}

/*
 * gdata_parser_int64_time_from_json_member:
 * @reader: #JsonReader cursor object to read JSON node from
 * @element_name: the name of the element to parse
 * @options: a bitwise combination of parsing options from #GDataParserOptions, or %P_NONE
 * @output: (out caller-allocates): the return location for the parsed time value
 * @success: the return location for a value which is %TRUE if the time val was parsed successfully, %FALSE if an error was encountered,
 * and undefined if @element didn't match @element_name
 * @error: a #GError, or %NULL
 *
 * Gets the time value of @element if its name is @element_name, subject to various checks specified by @options. It expects the text content
 * of @element to be a date or time value in ISO 8601 format. The returned time value will be a UNIX timestamp (seconds since the epoch).
 *
 * If @element doesn't match @element_name, %FALSE will be returned, @error will be unset and @success will be unset.
 *
 * If @element matches @element_name but one of the checks specified by @options fails, %TRUE will be returned, @error will be set to a
 * %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error and @success will be set to %FALSE.
 *
 * If @element matches @element_name and all of the checks specified by @options pass, %TRUE will be returned, @error will be unset and
 * @success will be set to %TRUE.
 *
 * The reason for returning the success of the parsing in @success is so that calls to gdata_parser_int64_time_from_element() can be chained
 * together in a large "or" statement based on their return values, for the purposes of determining whether any of the calls matched
 * a given @element. If any of the calls to gdata_parser_int64_time_from_element() return %TRUE, the value of @success can be examined.
 *
 * Return value: %TRUE if @element matched @element_name, %FALSE otherwise
 *
 * Since: 0.15.0
 */
gboolean
gdata_parser_int64_time_from_json_member (JsonReader *reader, const gchar *member_name, GDataParserOptions options,
                                          gint64 *output, gboolean *success, GError **error)
{
	const gchar *text;
	g_autoptr(GDateTime) time_val = NULL;
	const GError *child_error = NULL;

	/* Check if there's such element */
	if (g_strcmp0 (json_reader_get_member_name (reader), member_name) != 0) {
		return FALSE;
	}

	/* Check if the output time val has already been set. The JSON parser guarantees this can't happen. */
	g_assert (!(options & P_NO_DUPES) || *output == -1);

	/* Get the string and check it for NULLness. Check for errors first. */
	text = json_reader_get_string_value (reader);
	child_error = json_reader_get_error (reader);
	if (child_error != NULL) {
		*success = gdata_parser_error_from_json_error (reader,
		                                               child_error,
		                                               error);
		return TRUE;
	} else if (options & P_REQUIRED && (text == NULL || *text == '\0')) {
		*success = gdata_parser_error_required_json_content_missing (reader, error);
		return TRUE;
	}

	/* Attempt to parse the string as a GDateTime */
	time_val = g_date_time_new_from_iso8601 (text, NULL);
	if (!time_val) {
		*success = gdata_parser_error_not_iso8601_format_json (reader, text, error);
		return TRUE;
	}

	/* Success! */
	*output = g_date_time_to_unix (time_val);
	*success = TRUE;

	return TRUE;
}

/*
 * gdata_parser_boolean_from_json_member:
 * @reader: #JsonReader cursor object to read the JSON node from
 * @member_name: the name of the JSON object member to parse
 * @options: a bitwise combination of parsing options from #GDataParserOptions, or %P_NONE
 * @output: (out caller-allocates): the return location for the parsed boolean value
 * @success: (out caller-allocates): the return location for a value which is %TRUE if the boolean was parsed successfully, %FALSE if an error was encountered,
 * and undefined if @member_name was not found in the current object in @reader
 * @error: (allow-none): a #GError, or %NULL
 *
 * Gets the boolean value of the @member_name member of the current object in the #JsonReader, subject to various checks specified by @options.
 *
 * If no member matching @member_name can be found in the current @reader JSON object, %FALSE will be returned, @error will be unset and @success will be unset. @output will be undefined.
 *
 * If @member_name is found but one of the checks specified by @options fails, %TRUE will be returned, @error will be set to a
 * %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error and @success will be set to %FALSE. @output will be undefined.
 *
 * If @member_name is found and all of the checks specified by @options pass, %TRUE will be returned, @error will be unset and
 * @success will be set to %TRUE. @output will be set to the parsed value.
 *
 * The reason for returning the success of the parsing in @success is so that calls to gdata_parser_boolean_from_json_node() can be chained
 * together in a large "or" statement based on their return values, for the purposes of determining whether any of the calls matched
 * a given JSON node. If any of the calls to gdata_parser_boolean_from_json_node() return %TRUE, the value of @success can be examined.
 *
 * Return value: %TRUE if @member_name was found, %FALSE otherwise
 *
 * Since: 0.15.0
 */
gboolean
gdata_parser_boolean_from_json_member (JsonReader *reader, const gchar *member_name, GDataParserOptions options, gboolean *output, gboolean *success, GError **error)
{
	gboolean val;
	const GError *child_error = NULL;

	/* Check if there's such an element. */
	if (g_strcmp0 (json_reader_get_member_name (reader), member_name) != 0) {
		return FALSE;
	}

	/* Get the boolean. Check for parse errors. */
	val = json_reader_get_boolean_value (reader);
	child_error = json_reader_get_error (reader);
	if (child_error != NULL) {
		*success = gdata_parser_error_from_json_error (reader,
		                                               child_error,
		                                               error);
		return TRUE;
	}

	/* Success! */
	*output = val;
	*success = TRUE;

	return TRUE;
}

/*
 * gdata_parser_strv_from_json_member:
 * @reader: #JsonReader cursor object to read JSON node from
 * @element_name: the name of the element to parse
 * @options: a bitwise combination of parsing options from #GDataParserOptions,
 *   or %P_NONE
 * @output: (out callee-allocates) (transfer full): the return location for the
 *   parsed string array
 * @success: the return location for a value which is %TRUE if the string array
 *   was parsed successfully, %FALSE if an error was encountered, and undefined
 *   if @element didn't match @element_name
 * @error: a #GError, or %NULL
 *
 * Gets the string array of @element if its name is @element_name, subject to
 * various checks specified by @options. It expects the @element to be an array
 * of strings.
 *
 * If @element doesn't match @element_name, %FALSE will be returned, @error will
 * be unset and @success will be unset.
 *
 * If @element matches @element_name but one of the checks specified by @options
 * fails, %TRUE will be returned, @error will be set to a
 * %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error and @success will be set to %FALSE.
 *
 * If @element matches @element_name and all of the checks specified by @options
 * pass, %TRUE will be returned, @error will be unset and @success will be set
 * to %TRUE.
 *
 * The reason for returning the success of the parsing in @success is so that
 * calls to gdata_parser_strv_from_element() can be chained together in a large
 * "or" statement based on their return values, for the purposes of determining
 * whether any of the calls matched a given @element. If any of the calls to
 * gdata_parser_strv_from_element() return %TRUE, the value of @success can be
 * examined.
 *
 * Return value: %TRUE if @element matched @element_name, %FALSE otherwise
 *
 * Since: 0.17.0
 */
gboolean
gdata_parser_strv_from_json_member (JsonReader *reader,
                                    const gchar *member_name,
                                    GDataParserOptions options,
                                    gchar ***output, gboolean *success,
                                    GError **error)
{
	guint i, len;
	GPtrArray *out;
	const GError *child_error = NULL;

	/* Check if there's such element */
	if (g_strcmp0 (json_reader_get_member_name (reader),
	               member_name) != 0) {
		return FALSE;
	}

	/* Check if the output strv has already been set. The JSON parser
	 * guarantees this can't happen. */
	g_assert (!(options & P_NO_DUPES) || *output == NULL);

	len = json_reader_count_elements (reader);
	child_error = json_reader_get_error (reader);

	if (child_error != NULL) {
		*success = gdata_parser_error_from_json_error (reader,
		                                               child_error,
		                                               error);
		return TRUE;
	}

	out = g_ptr_array_new_full (len + 1  /* NULL terminator */, g_free);

	for (i = 0; i < len; i++) {
		const gchar *val;

		json_reader_read_element (reader, i);
		val = json_reader_get_string_value (reader);
		child_error = json_reader_get_error (reader);

		if (child_error != NULL) {
			*success = gdata_parser_error_from_json_error (reader,
			                                               child_error,
			                                               error);
			json_reader_end_element (reader);
			g_ptr_array_unref (out);
			return TRUE;
		}

		g_ptr_array_add (out, g_strdup (val));

		json_reader_end_element (reader);
	}

	/* NULL terminator. */
	g_ptr_array_add (out, NULL);

	/* Success! */
	*output = (gchar **) g_ptr_array_free (out, FALSE);
	*success = TRUE;

	return TRUE;
}

/*
 * gdata_parser_color_from_json_member:
 * @reader: #JsonReader cursor object to read JSON node from
 * @element_name: the name of the element to parse
 * @options: a bitwise combination of parsing options from #GDataParserOptions,
 *   or %P_NONE
 * @output: (out caller-allocates): the return location for the parsed colour
 *   value
 * @success: the return location for a value which is %TRUE if the colour was
 *   parsed successfully, %FALSE if an error was encountered, and undefined if
 *   @element didn't match @element_name
 * @error: a #GError, or %NULL
 *
 * Gets the colour value of @element if its name is @element_name, subject to
 * various checks specified by @options. It expects the text content of
 * @element to be an RGB colour in hexadecimal format, with an optional leading
 * hash symbol (for example, `#RRGGBB` or `RRGGBB`).
 *
 * If @element doesn't match @element_name, %FALSE will be returned, @error
 * will be unset and @success will be unset.
 *
 * If @element matches @element_name but one of the checks specified by
 * @options fails, %TRUE will be returned, @error will be set to a
 * %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error and @success will be set to %FALSE.
 *
 * If @element matches @element_name and all of the checks specified by
 * @options pass, %TRUE will be returned, @error will be unset and @success
 * will be set to %TRUE.
 *
 * The reason for returning the success of the parsing in @success is so that
 * calls to gdata_parser_color_from_json_member() can be chained together in a
 * large "or" statement based on their return values, for the purposes of
 * determining whether any of the calls matched a given @element. If any of the
 * calls to gdata_parser_color_from_json_member() return %TRUE, the value of
 * @success can be examined.
 *
 * Return value: %TRUE if @element matched @element_name, %FALSE otherwise
 *
 * Since: 0.17.2
 */
gboolean
gdata_parser_color_from_json_member (JsonReader *reader,
                                     const gchar *member_name,
                                     GDataParserOptions options,
                                     GDataColor *output,
                                     gboolean *success,
                                     GError **error)
{
	const gchar *text;
	GDataColor colour;
	const GError *child_error = NULL;

	/* Check if there's such an element */
	if (g_strcmp0 (json_reader_get_member_name (reader), member_name) != 0) {
		return FALSE;
	}

	/* Check if the output colour has already been set. The JSON parser
	 * guarantees this can't happen. */
	g_assert (!(options & P_NO_DUPES) ||
	          (output->red == 0 && output->green == 0 && output->blue == 0));

	/* Get the string and check it for NULLness. Check for errors first. */
	text = json_reader_get_string_value (reader);
	child_error = json_reader_get_error (reader);
	if (child_error != NULL) {
		*success = gdata_parser_error_from_json_error (reader, child_error, error);
		return TRUE;
	} else if (options & P_REQUIRED && (text == NULL || *text == '\0')) {
		*success = gdata_parser_error_required_json_content_missing (reader, error);
		return TRUE;
	}

	/* Attempt to parse the string as a hexadecimal colour. */
	if (gdata_color_from_hexadecimal (text, &colour) == FALSE) {
		/* Error */
		g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
		             /* Translators: the first parameter is the name of an XML element (including the angle brackets
		              * ("<" and ">"), and the second parameter is the erroneous value (which was not in hexadecimal
		              * RGB format).
		              *
		              * For example:
		              *  The content of a <entry/gCal:color> element (‘00FG56’) was not in hexadecimal RGB format. */
		             _("The content of a %s element (‘%s’) was not in hexadecimal RGB format."),
		             member_name, text);
		*success = FALSE;

		return TRUE;
	}

	/* Success! */
	*output = colour;
	*success = TRUE;

	return TRUE;
}

void
gdata_parser_string_append_escaped (GString *xml_string, const gchar *pre, const gchar *element_content, const gchar *post)
{
	/* Allocate 10 extra bytes when reallocating the GString, to try and avoid having to reallocate again, by assuming
	 * there will be an increase in the length of element_content when escaped of less than 10 characters. */
/*	#define SIZE_FUZZINESS 10*/

/*	guint new_size;*/
	const gchar *p;

	/* Expand xml_string as necessary */
	/* TODO: There is no way to expand the allocation of a GString if you know in advance how much room
	 * lots of append operations are going to require. */
/*	new_size = xml_string->len + strlen (pre) + strlen (element_content) + strlen (post) + SIZE_FUZZINESS;
	if (new_size > xml_string->allocated_len)
		g_string_set_size (xml_string, new_size);*/

	/* Append the pre content */
	if (pre != NULL)
		g_string_append (xml_string, pre);

	/* Loop through the string to be escaped. Code adapted from GLib's g_markup_escape_text() function.
	 *  Copyright 2000, 2003 Red Hat, Inc.
	 *  Copyright 2007, 2008 Ryan Lortie <desrt@desrt.ca> */
	p = element_content;
	while (p != NULL && *p != '\0') {
		const gchar *next = g_utf8_next_char (p);

		switch (*p) {
			case '&':
				g_string_append (xml_string, "&amp;");
				break;
			case '<':
				g_string_append (xml_string, "&lt;");
				break;
			case '>':
				g_string_append (xml_string, "&gt;");
				break;
			case '\'':
				g_string_append (xml_string, "&apos;");
				break;
			case '"':
				g_string_append (xml_string, "&quot;");
				break;
			default: {
				gunichar c = g_utf8_get_char (p);

				if ((0x1 <= c && c <= 0x8) ||
				    (0xb <= c && c  <= 0xc) ||
				    (0xe <= c && c <= 0x1f) ||
				    (0x7f <= c && c <= 0x84) ||
				    (0x86 <= c && c <= 0x9f)) {
					g_string_append_printf (xml_string, "&#x%x;", c);
				} else {
					g_string_append_len (xml_string, p, next - p);
					break;
				}
			}
		}
		p = next;
	}

	/* Append the post content */
	if (post != NULL)
		g_string_append (xml_string, post);
}

/* TODO: Should be perfectly possible to make this modify the string in-place */
gchar *
gdata_parser_utf8_trim_whitespace (const gchar *s)
{
	glong len;
	const gchar *_s;

	/* Skip the leading whitespace */
	while (*s != '\0' && g_unichar_isspace (g_utf8_get_char (s)))
		s = g_utf8_next_char (s);

	/* Find the end of the string and backtrack until we've passed all the whitespace */
	len = g_utf8_strlen (s, -1);
	_s = g_utf8_offset_to_pointer (s, len - 1);
	while (len > 0 && g_unichar_isspace (g_utf8_get_char (_s))) {
		_s = g_utf8_prev_char (_s);
		len--;
	}
	_s = g_utf8_next_char (_s);

	return g_strndup (s, _s - s);
}
