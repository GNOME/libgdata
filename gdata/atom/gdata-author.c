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
 * SECTION:gdata-author
 * @short_description: Atom author element
 * @stability: Stable
 * @include: gdata/atom/gdata-author.h
 *
 * #GDataAuthor represents a "author" element from the
 * <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php">Atom specification</ulink>.
 *
 * Since: 0.4.0
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-author.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-comparable.h"

static void gdata_author_comparable_init (GDataComparableIface *iface);
static void gdata_author_finalize (GObject *object);
static void gdata_author_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_author_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static gboolean post_parse_xml (GDataParsable *parsable, gpointer user_data, GError **error);
static void get_xml (GDataParsable *parsable, GString *xml_string);

struct _GDataAuthorPrivate {
	gchar *name;
	gchar *uri;
	gchar *email_address;
};

enum {
	PROP_NAME = 1,
	PROP_URI,
	PROP_EMAIL_ADDRESS
};

G_DEFINE_TYPE_WITH_CODE (GDataAuthor, gdata_author, GDATA_TYPE_PARSABLE,
                         G_ADD_PRIVATE (GDataAuthor)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMPARABLE, gdata_author_comparable_init))

static void
gdata_author_class_init (GDataAuthorClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->set_property = gdata_author_set_property;
	gobject_class->get_property = gdata_author_get_property;
	gobject_class->finalize = gdata_author_finalize;

	parsable_class->parse_xml = parse_xml;
	parsable_class->post_parse_xml = post_parse_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->element_name = "author";

	/**
	 * GDataAuthor:name:
	 *
	 * A human-readable name for the person.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.author">Atom specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_NAME,
	                                 g_param_spec_string ("name",
	                                                      "Name", "A human-readable name for the person.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataAuthor:uri:
	 *
	 * An IRI associated with the person.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.author">Atom specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_URI,
	                                 g_param_spec_string ("uri",
	                                                      "URI", "An IRI associated with the person.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataAuthor:email-address:
	 *
	 * An e-mail address associated with the person.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.author">Atom specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_EMAIL_ADDRESS,
	                                 g_param_spec_string ("email-address",
	                                                      "E-mail address", "An e-mail address associated with the person.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static gint
compare_with (GDataComparable *self, GDataComparable *other)
{
	return g_strcmp0 (((GDataAuthor*) self)->priv->name, ((GDataAuthor*) other)->priv->name);
}

static void
gdata_author_comparable_init (GDataComparableIface *iface)
{
	iface->compare_with = compare_with;
}

static void
gdata_author_init (GDataAuthor *self)
{
	self->priv = gdata_author_get_instance_private (self);
}

static void
gdata_author_finalize (GObject *object)
{
	GDataAuthorPrivate *priv = GDATA_AUTHOR (object)->priv;

	g_free (priv->name);
	g_free (priv->uri);
	g_free (priv->email_address);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_author_parent_class)->finalize (object);
}

static void
gdata_author_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataAuthorPrivate *priv = GDATA_AUTHOR (object)->priv;

	switch (property_id) {
		case PROP_NAME:
			g_value_set_string (value, priv->name);
			break;
		case PROP_URI:
			g_value_set_string (value, priv->uri);
			break;
		case PROP_EMAIL_ADDRESS:
			g_value_set_string (value, priv->email_address);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_author_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataAuthor *self = GDATA_AUTHOR (object);

	switch (property_id) {
		case PROP_NAME:
			gdata_author_set_name (self, g_value_get_string (value));
			break;
		case PROP_URI:
			gdata_author_set_uri (self, g_value_get_string (value));
			break;
		case PROP_EMAIL_ADDRESS:
			gdata_author_set_email_address (self, g_value_get_string (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	gboolean success;
	GDataAuthorPrivate *priv = GDATA_AUTHOR (parsable)->priv;

	if (gdata_parser_is_namespace (node, "http://www.w3.org/2005/Atom") == TRUE &&
	    (gdata_parser_string_from_element (node, "name", P_NO_DUPES | P_REQUIRED | P_NON_EMPTY, &(priv->name), &success, error) == TRUE ||
	     gdata_parser_string_from_element (node, "uri", P_NO_DUPES | P_REQUIRED | P_NON_EMPTY, &(priv->uri), &success, error) == TRUE ||
	     gdata_parser_string_from_element (node, "email", P_NO_DUPES | P_REQUIRED | P_NON_EMPTY, &(priv->email_address), &success, error) == TRUE
	    )) {
		return success;
	} else {
		return GDATA_PARSABLE_CLASS (gdata_author_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}
}

static gboolean
post_parse_xml (GDataParsable *parsable, gpointer user_data, GError **error)
{
	GDataAuthorPrivate *priv = GDATA_AUTHOR (parsable)->priv;

	if (priv->name == NULL || *(priv->name) == '\0')
		return gdata_parser_error_required_element_missing ("name", "author", error);

	return TRUE;
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataAuthorPrivate *priv = GDATA_AUTHOR (parsable)->priv;

	gdata_parser_string_append_escaped (xml_string, "<name>", priv->name, "</name>");

	if (priv->uri != NULL)
		gdata_parser_string_append_escaped (xml_string, "<uri>", priv->uri, "</uri>");

	if (priv->email_address != NULL)
		gdata_parser_string_append_escaped (xml_string, "<email>", priv->email_address, "</email>");
}

/**
 * gdata_author_new:
 * @name: the author's name
 * @uri: (allow-none): an IRI describing the author, or %NULL
 * @email_address: (allow-none): the author's e-mail address, or %NULL
 *
 * Creates a new #GDataAuthor. More information is available in the <ulink type="http"
 * url="http://www.atomenabled.org/developers/syndication/atom-format-spec.php#element.author">Atom specification</ulink>.
 *
 * @name must be non-%NULL and non-empty.
 *
 * Return value: a new #GDataAuthor, or %NULL; unref with g_object_unref()
 */
GDataAuthor *
gdata_author_new (const gchar *name, const gchar *uri, const gchar *email_address)
{
	g_return_val_if_fail (name != NULL && *name != '\0', NULL);
	return g_object_new (GDATA_TYPE_AUTHOR, "name", name, "uri", uri, "email-address", email_address, NULL);
}

/**
 * gdata_author_get_name:
 * @self: a #GDataAuthor
 *
 * Gets the #GDataAuthor:name property. The name will always be a non-%NULL, non-empty string.
 *
 * Return value: the author's name
 *
 * Since: 0.4.0
 */
const gchar *
gdata_author_get_name (GDataAuthor *self)
{
	g_return_val_if_fail (GDATA_IS_AUTHOR (self), NULL);
	return self->priv->name;
}

/**
 * gdata_author_set_name:
 * @self: a #GDataAuthor
 * @name: the new name for the author
 *
 * Sets the #GDataAuthor:name property to @name. @name must be non-%NULL and non-empty.
 *
 * Since: 0.4.0
 */
void
gdata_author_set_name (GDataAuthor *self, const gchar *name)
{
	g_return_if_fail (GDATA_IS_AUTHOR (self));
	g_return_if_fail (name != NULL && *name != '\0');

	g_free (self->priv->name);
	self->priv->name = g_strdup (name);
	g_object_notify (G_OBJECT (self), "name");
}

/**
 * gdata_author_get_uri:
 * @self: a #GDataAuthor
 *
 * Gets the #GDataAuthor:uri property. If the URI is non-%NULL, it will be non-empty.
 *
 * Return value: the author's URI, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_author_get_uri (GDataAuthor *self)
{
	g_return_val_if_fail (GDATA_IS_AUTHOR (self), NULL);
	return self->priv->uri;
}

/**
 * gdata_author_set_uri:
 * @self: a #GDataAuthor
 * @uri: (allow-none): the new URI for the author, or %NULL
 *
 * Sets the #GDataAuthor:uri property to @uri. @uri must be %NULL or non-empty.
 *
 * Set @uri to %NULL to unset the property in the author.
 *
 * Since: 0.4.0
 */
void
gdata_author_set_uri (GDataAuthor *self, const gchar *uri)
{
	g_return_if_fail (GDATA_IS_AUTHOR (self));

	g_free (self->priv->uri);
	self->priv->uri = g_strdup (uri);
	g_object_notify (G_OBJECT (self), "uri");
}

/**
 * gdata_author_get_email_address:
 * @self: a #GDataAuthor
 *
 * Gets the #GDataAuthor:email-address property. If the e-mail address is non-%NULL, it will be non-empty.
 *
 * Return value: the author's e-mail address, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_author_get_email_address (GDataAuthor *self)
{
	g_return_val_if_fail (GDATA_IS_AUTHOR (self), NULL);
	return self->priv->email_address;
}

/**
 * gdata_author_set_email_address:
 * @self: a #GDataAuthor
 * @email_address: (allow-none): the new e-mail address for the author, or %NULL
 *
 * Sets the #GDataAuthor:email-address property to @email_address. @email_address must be %NULL or non-empty.
 *
 * Set @email_address to %NULL to unset the property in the author.
 *
 * Since: 0.4.0
 */
void
gdata_author_set_email_address (GDataAuthor *self, const gchar *email_address)
{
	g_return_if_fail (GDATA_IS_AUTHOR (self));

	g_free (self->priv->email_address);
	self->priv->email_address = g_strdup (email_address);
	g_object_notify (G_OBJECT (self), "email-address");
}
