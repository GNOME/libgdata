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
 * SECTION:gdata-gd-organization
 * @short_description: GData organization element
 * @stability: Stable
 * @include: gdata/gd/gdata-gd-organization.h
 *
 * #GDataGDOrganization represents an "organization" element from the
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdOrganization">GData specification</ulink>.
 *
 * Since: 0.4.0
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-gd-organization.h"
#include "gdata-parsable.h"
#include "gdata-parser.h"
#include "gdata-gd-where.h"
#include "gdata-private.h"
#include "gdata-comparable.h"

static void gdata_gd_organization_comparable_init (GDataComparableIface *iface);
static void gdata_gd_organization_dispose (GObject *object);
static void gdata_gd_organization_finalize (GObject *object);
static void gdata_gd_organization_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_gd_organization_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error);
static void pre_get_xml (GDataParsable *parsable, GString *xml_string);
static void get_xml (GDataParsable *parsable, GString *xml_string);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataGDOrganizationPrivate {
	gchar *name;
	gchar *title;
	gchar *relation_type;
	gchar *label;
	gboolean is_primary;
	gchar *department;
	gchar *job_description;
	gchar *symbol;
	GDataGDWhere *location;
};

enum {
	PROP_NAME = 1,
	PROP_TITLE,
	PROP_RELATION_TYPE,
	PROP_LABEL,
	PROP_IS_PRIMARY,
	PROP_DEPARTMENT,
	PROP_JOB_DESCRIPTION,
	PROP_SYMBOL,
	PROP_LOCATION
};

G_DEFINE_TYPE_WITH_CODE (GDataGDOrganization, gdata_gd_organization, GDATA_TYPE_PARSABLE,
                         G_ADD_PRIVATE (GDataGDOrganization)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_COMPARABLE, gdata_gd_organization_comparable_init))

static void
gdata_gd_organization_class_init (GDataGDOrganizationClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_gd_organization_get_property;
	gobject_class->set_property = gdata_gd_organization_set_property;
	gobject_class->dispose = gdata_gd_organization_dispose;
	gobject_class->finalize = gdata_gd_organization_finalize;

	parsable_class->pre_parse_xml = pre_parse_xml;
	parsable_class->parse_xml = parse_xml;
	parsable_class->pre_get_xml = pre_get_xml;
	parsable_class->get_xml = get_xml;
	parsable_class->get_namespaces = get_namespaces;
	parsable_class->element_name = "organization";
	parsable_class->element_namespace = "gd";

	/**
	 * GDataGDOrganization:name:
	 *
	 * The name of the organization.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdOrganization">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_NAME,
	                                 g_param_spec_string ("name",
	                                                      "Name", "The name of the organization.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDOrganization:title:
	 *
	 * The title of a person within the organization.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdOrganization">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_TITLE,
	                                 g_param_spec_string ("title",
	                                                      "Title", "The title of a person within the organization.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDOrganization:relation-type:
	 *
	 * A programmatic value that identifies the type of organization. For example: %GDATA_GD_ORGANIZATION_WORK or %GDATA_GD_ORGANIZATION_OTHER.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdOrganization">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_RELATION_TYPE,
	                                 g_param_spec_string ("relation-type",
	                                                      "Relation type", "A programmatic value that identifies the type of organization.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDOrganization:label:
	 *
	 * A simple string value used to name this organization. It allows UIs to display a label such as "Work", "Volunteer",
	 * "Professional Society", etc.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdOrganization">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_LABEL,
	                                 g_param_spec_string ("label",
	                                                      "Label", "A simple string value used to name this organization.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDOrganization:is-primary:
	 *
	 * Indicates which organization out of a group is primary.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdOrganization">GData specification</ulink>.
	 *
	 * Since: 0.4.0
	 */
	g_object_class_install_property (gobject_class, PROP_IS_PRIMARY,
	                                 g_param_spec_boolean ("is-primary",
	                                                       "Primary?", "Indicates which organization out of a group is primary.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDOrganization:department:
	 *
	 * Specifies a department within the organization.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdOrganization">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_DEPARTMENT,
	                                 g_param_spec_string ("department",
	                                                      "Department", "Specifies a department within the organization.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDOrganization:job-description:
	 *
	 * Description of a job within the organization.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdOrganization">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_JOB_DESCRIPTION,
	                                 g_param_spec_string ("job-description",
	                                                      "Job description", "Description of a job within the organization.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDOrganization:symbol:
	 *
	 * Stock symbol of the organization.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdOrganization">GData specification</ulink>.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_SYMBOL,
	                                 g_param_spec_string ("symbol",
	                                                      "Symbol", "Symbol of the organization.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataGDOrganization:location:
	 *
	 * A place associated with the organization, e.g. office location.
	 *
	 * For more information, see the
	 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdOrganization">GData specification</ulink>.
	 *
	 * Since: 0.6.0
	 */
	g_object_class_install_property (gobject_class, PROP_LOCATION,
	                                 g_param_spec_object ("location",
	                                                      "Location", "A place associated with the organization, e.g. office location.",
	                                                      GDATA_TYPE_GD_WHERE,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static gint
compare_with (GDataComparable *self, GDataComparable *other)
{
	GDataGDOrganizationPrivate *a = ((GDataGDOrganization*) self)->priv, *b = ((GDataGDOrganization*) other)->priv;

	if (g_strcmp0 (a->name, b->name) == 0 && g_strcmp0 (a->title, b->title) == 0 && g_strcmp0 (a->department, b->department) == 0)
		return 0;
	return 1;
}

static void
gdata_gd_organization_comparable_init (GDataComparableIface *iface)
{
	iface->compare_with = compare_with;
}

static void
gdata_gd_organization_init (GDataGDOrganization *self)
{
	self->priv = gdata_gd_organization_get_instance_private (self);
}

static void
gdata_gd_organization_dispose (GObject *object)
{
	GDataGDOrganizationPrivate *priv = GDATA_GD_ORGANIZATION (object)->priv;

	if (priv->location != NULL)
		g_object_unref (priv->location);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gd_organization_parent_class)->dispose (object);
}

static void
gdata_gd_organization_finalize (GObject *object)
{
	GDataGDOrganizationPrivate *priv = GDATA_GD_ORGANIZATION (object)->priv;

	g_free (priv->name);
	g_free (priv->title);
	g_free (priv->relation_type);
	g_free (priv->label);
	g_free (priv->department);
	g_free (priv->job_description);
	g_free (priv->symbol);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_gd_organization_parent_class)->finalize (object);
}

static void
gdata_gd_organization_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataGDOrganizationPrivate *priv = GDATA_GD_ORGANIZATION (object)->priv;

	switch (property_id) {
		case PROP_NAME:
			g_value_set_string (value, priv->name);
			break;
		case PROP_TITLE:
			g_value_set_string (value, priv->title);
			break;
		case PROP_RELATION_TYPE:
			g_value_set_string (value, priv->relation_type);
			break;
		case PROP_LABEL:
			g_value_set_string (value, priv->label);
			break;
		case PROP_IS_PRIMARY:
			g_value_set_boolean (value, priv->is_primary);
			break;
		case PROP_DEPARTMENT:
			g_value_set_string (value, priv->department);
			break;
		case PROP_JOB_DESCRIPTION:
			g_value_set_string (value, priv->job_description);
			break;
		case PROP_SYMBOL:
			g_value_set_string (value, priv->symbol);
			break;
		case PROP_LOCATION:
			g_value_set_object (value, priv->location);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_gd_organization_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataGDOrganization *self = GDATA_GD_ORGANIZATION (object);

	switch (property_id) {
		case PROP_NAME:
			gdata_gd_organization_set_name (self, g_value_get_string (value));
			break;
		case PROP_TITLE:
			gdata_gd_organization_set_title (self, g_value_get_string (value));
			break;
		case PROP_RELATION_TYPE:
			gdata_gd_organization_set_relation_type (self, g_value_get_string (value));
			break;
		case PROP_LABEL:
			gdata_gd_organization_set_label (self, g_value_get_string (value));
			break;
		case PROP_IS_PRIMARY:
			gdata_gd_organization_set_is_primary (self, g_value_get_boolean (value));
			break;
		case PROP_DEPARTMENT:
			gdata_gd_organization_set_department (self, g_value_get_string (value));
			break;
		case PROP_JOB_DESCRIPTION:
			gdata_gd_organization_set_job_description (self, g_value_get_string (value));
			break;
		case PROP_SYMBOL:
			gdata_gd_organization_set_symbol (self, g_value_get_string (value));
			break;
		case PROP_LOCATION:
			gdata_gd_organization_set_location (self, g_value_get_object (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static gboolean
pre_parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *root_node, gpointer user_data, GError **error)
{
	xmlChar *rel;
	gboolean primary_bool;
	GDataGDOrganizationPrivate *priv = GDATA_GD_ORGANIZATION (parsable)->priv;

	/* Is it the primary organisation? */
	if (gdata_parser_boolean_from_property (root_node, "primary", &primary_bool, 0, error) == FALSE)
		return FALSE;

	rel = xmlGetProp (root_node, (xmlChar*) "rel");
	if (rel != NULL && *rel == '\0') {
		xmlFree (rel);
		return gdata_parser_error_required_property_missing (root_node, "rel", error);
	}

	priv->relation_type = (gchar*) rel;
	priv->label = (gchar*) xmlGetProp (root_node, (xmlChar*) "label");
	priv->is_primary = primary_bool;

	return TRUE;
}

static gboolean
parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error)
{
	gboolean success;
	GDataGDOrganizationPrivate *priv = GDATA_GD_ORGANIZATION (parsable)->priv;

	if (gdata_parser_is_namespace (node, "http://schemas.google.com/g/2005") == TRUE && (
	     gdata_parser_string_from_element (node, "orgName", P_NO_DUPES, &(priv->name), &success, error) == TRUE ||
	     gdata_parser_string_from_element (node, "orgTitle", P_NO_DUPES, &(priv->title), &success, error) == TRUE ||
	     gdata_parser_string_from_element (node, "orgDepartment", P_NO_DUPES, &(priv->department), &success, error) == TRUE ||
	     gdata_parser_string_from_element (node, "orgJobDescription", P_NO_DUPES, &(priv->job_description), &success, error) == TRUE ||
	     gdata_parser_string_from_element (node, "orgSymbol", P_NO_DUPES, &(priv->symbol), &success, error) == TRUE ||
	     gdata_parser_object_from_element (node, "where", P_REQUIRED | P_NO_DUPES, GDATA_TYPE_GD_WHERE,
	                                      &(priv->location), &success, error) == TRUE)) {
		return success;
	} else {
		return GDATA_PARSABLE_CLASS (gdata_gd_organization_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}
}

static void
pre_get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGDOrganizationPrivate *priv = GDATA_GD_ORGANIZATION (parsable)->priv;

	if (priv->relation_type != NULL)
		gdata_parser_string_append_escaped (xml_string, " rel='", priv->relation_type, "'");
	if (priv->label != NULL)
		gdata_parser_string_append_escaped (xml_string, " label='", priv->label, "'");

	if (priv->is_primary == TRUE)
		g_string_append (xml_string, " primary='true'");
	else
		g_string_append (xml_string, " primary='false'");
}

static void
get_xml (GDataParsable *parsable, GString *xml_string)
{
	GDataGDOrganizationPrivate *priv = GDATA_GD_ORGANIZATION (parsable)->priv;

	if (priv->name != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gd:orgName>", priv->name, "</gd:orgName>");
	if (priv->title != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gd:orgTitle>", priv->title, "</gd:orgTitle>");
	if (priv->department != NULL && *(priv->department) != '\0')
		gdata_parser_string_append_escaped (xml_string, "<gd:orgDepartment>", priv->department, "</gd:orgDepartment>");
	if (priv->job_description != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gd:orgJobDescription>", priv->job_description, "</gd:orgJobDescription>");
	if (priv->symbol != NULL)
		gdata_parser_string_append_escaped (xml_string, "<gd:orgSymbol>", priv->symbol, "</gd:orgSymbol>");
	if (priv->location != NULL)
		_gdata_parsable_get_xml (GDATA_PARSABLE (priv->location), xml_string, FALSE);
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	g_hash_table_insert (namespaces, (gchar*) "gd", (gchar*) "http://schemas.google.com/g/2005");
}

/**
 * gdata_gd_organization_new:
 * @name: (allow-none): the name of the organization, or %NULL
 * @title: (allow-none): the owner's title within the organization, or %NULL
 * @relation_type: (allow-none): the relationship between the organization and its owner, or %NULL
 * @label: (allow-none): a human-readable label for the organization, or %NULL
 * @is_primary: %TRUE if this organization is its owner's primary organization, %FALSE otherwise
 *
 * Creates a new #GDataGDOrganization. More information is available in the <ulink type="http"
 * url="http://code.google.com/apis/gdata/docs/2.0/elements.html#gdOrganization">GData specification</ulink>.
 *
 * Return value: a new #GDataGDOrganization, or %NULL; unref with g_object_unref()
 *
 * Since: 0.2.0
 */
GDataGDOrganization *
gdata_gd_organization_new (const gchar *name, const gchar *title, const gchar *relation_type, const gchar *label, gboolean is_primary)
{
	g_return_val_if_fail (relation_type == NULL || *relation_type != '\0', NULL);
	return g_object_new (GDATA_TYPE_GD_ORGANIZATION, "name", name, "title", title, "relation-type", relation_type,
	                     "label", label, "is-primary", is_primary, NULL);
}

/**
 * gdata_gd_organization_get_name:
 * @self: a #GDataGDOrganization
 *
 * Gets the #GDataGDOrganization:name property.
 *
 * Return value: the organization's name, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_gd_organization_get_name (GDataGDOrganization *self)
{
	g_return_val_if_fail (GDATA_IS_GD_ORGANIZATION (self), NULL);
	return self->priv->name;
}

/**
 * gdata_gd_organization_set_name:
 * @self: a #GDataGDOrganization
 * @name: (allow-none): the new name for the organization, or %NULL
 *
 * Sets the #GDataGDOrganization:name property to @name.
 *
 * Set @name to %NULL to unset the property in the organization.
 *
 * Since: 0.4.0
 */
void
gdata_gd_organization_set_name (GDataGDOrganization *self, const gchar *name)
{
	g_return_if_fail (GDATA_IS_GD_ORGANIZATION (self));

	g_free (self->priv->name);
	self->priv->name = g_strdup (name);
	g_object_notify (G_OBJECT (self), "name");
}

/**
 * gdata_gd_organization_get_title:
 * @self: a #GDataGDOrganization
 *
 * Gets the #GDataGDOrganization:title property.
 *
 * Return value: the organization's title, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_gd_organization_get_title (GDataGDOrganization *self)
{
	g_return_val_if_fail (GDATA_IS_GD_ORGANIZATION (self), NULL);
	return self->priv->title;
}

/**
 * gdata_gd_organization_set_title:
 * @self: a #GDataGDOrganization
 * @title: (allow-none): the new title for the organization, or %NULL
 *
 * Sets the #GDataGDOrganization:title property to @title.
 *
 * Set @title to %NULL to unset the property in the organization.
 *
 * Since: 0.4.0
 */
void
gdata_gd_organization_set_title (GDataGDOrganization *self, const gchar *title)
{
	g_return_if_fail (GDATA_IS_GD_ORGANIZATION (self));

	g_free (self->priv->title);
	self->priv->title = g_strdup (title);
	g_object_notify (G_OBJECT (self), "title");
}

/**
 * gdata_gd_organization_get_relation_type:
 * @self: a #GDataGDOrganization
 *
 * Gets the #GDataGDOrganization:relation-type property.
 *
 * Return value: the organization's relation type, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_gd_organization_get_relation_type (GDataGDOrganization *self)
{
	g_return_val_if_fail (GDATA_IS_GD_ORGANIZATION (self), NULL);
	return self->priv->relation_type;
}

/**
 * gdata_gd_organization_set_relation_type:
 * @self: a #GDataGDOrganization
 * @relation_type: (allow-none): the new relation type for the organization, or %NULL
 *
 * Sets the #GDataGDOrganization:relation-type property to @relation_type.
 *
 * Set @relation_type to %NULL to unset the property in the organization.
 *
 * Since: 0.4.0
 */
void
gdata_gd_organization_set_relation_type (GDataGDOrganization *self, const gchar *relation_type)
{
	g_return_if_fail (GDATA_IS_GD_ORGANIZATION (self));
	g_return_if_fail (relation_type == NULL || *relation_type != '\0');

	g_free (self->priv->relation_type);
	self->priv->relation_type = g_strdup (relation_type);
	g_object_notify (G_OBJECT (self), "relation-type");
}

/**
 * gdata_gd_organization_get_label:
 * @self: a #GDataGDOrganization
 *
 * Gets the #GDataGDOrganization:label property.
 *
 * Return value: the organization's label, or %NULL
 *
 * Since: 0.4.0
 */
const gchar *
gdata_gd_organization_get_label (GDataGDOrganization *self)
{
	g_return_val_if_fail (GDATA_IS_GD_ORGANIZATION (self), NULL);
	return self->priv->label;
}

/**
 * gdata_gd_organization_set_label:
 * @self: a #GDataGDOrganization
 * @label: (allow-none): the new label for the organization, or %NULL
 *
 * Sets the #GDataGDOrganization:label property to @label.
 *
 * Set @label to %NULL to unset the property in the organization.
 *
 * Since: 0.4.0
 */
void
gdata_gd_organization_set_label (GDataGDOrganization *self, const gchar *label)
{
	g_return_if_fail (GDATA_IS_GD_ORGANIZATION (self));

	g_free (self->priv->label);
	self->priv->label = g_strdup (label);
	g_object_notify (G_OBJECT (self), "label");
}

/**
 * gdata_gd_organization_is_primary:
 * @self: a #GDataGDOrganization
 *
 * Gets the #GDataGDOrganization:is-primary property.
 *
 * Return value: %TRUE if this is the primary organization, %FALSE otherwise
 *
 * Since: 0.4.0
 */
gboolean
gdata_gd_organization_is_primary (GDataGDOrganization *self)
{
	g_return_val_if_fail (GDATA_IS_GD_ORGANIZATION (self), FALSE);
	return self->priv->is_primary;
}

/**
 * gdata_gd_organization_set_is_primary:
 * @self: a #GDataGDOrganization
 * @is_primary: %TRUE if this is the primary organization, %FALSE otherwise
 *
 * Sets the #GDataGDOrganization:is-primary property to @is_primary.
 *
 * Since: 0.4.0
 */
void
gdata_gd_organization_set_is_primary (GDataGDOrganization *self, gboolean is_primary)
{
	g_return_if_fail (GDATA_IS_GD_ORGANIZATION (self));

	self->priv->is_primary = is_primary;
	g_object_notify (G_OBJECT (self), "is-primary");
}

/**
 * gdata_gd_organization_get_department:
 * @self: a #GDataGDOrganization
 *
 * Gets the #GDataGDOrganization:department property.
 *
 * Return value: the department in which the person works in this organization, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_gd_organization_get_department (GDataGDOrganization *self)
{
	g_return_val_if_fail (GDATA_IS_GD_ORGANIZATION (self), NULL);
	return self->priv->department;
}

/**
 * gdata_gd_organization_set_department:
 * @self: a #GDataGDOrganization
 * @department: (allow-none): the new department for the person working in the organization, or %NULL
 *
 * Sets the #GDataGDOrganization:department property to @department.
 *
 * Set @department to %NULL to unset the property in the organization.
 *
 * Since: 0.5.0
 */
void
gdata_gd_organization_set_department (GDataGDOrganization *self, const gchar *department)
{
	g_return_if_fail (GDATA_IS_GD_ORGANIZATION (self));

	g_free (self->priv->department);
	self->priv->department = g_strdup (department);
	g_object_notify (G_OBJECT (self), "department");
}

/**
 * gdata_gd_organization_get_job_description:
 * @self: a #GDataGDOrganization
 *
 * Gets the #GDataGDOrganization:job-description property.
 *
 * Return value: the job description of the person in the organization, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_gd_organization_get_job_description (GDataGDOrganization *self)
{
	g_return_val_if_fail (GDATA_IS_GD_ORGANIZATION (self), NULL);
	return self->priv->job_description;
}

/**
 * gdata_gd_organization_set_job_description:
 * @self: a #GDataGDOrganization
 * @job_description: (allow-none): the new job description for the person in the organization, or %NULL
 *
 * Sets the #GDataGDOrganization:job-description property to @job_description.
 *
 * Set @job_description to %NULL to unset the property in the organization.
 *
 * Since: 0.5.0
 */
void
gdata_gd_organization_set_job_description (GDataGDOrganization *self, const gchar *job_description)
{
	g_return_if_fail (GDATA_IS_GD_ORGANIZATION (self));

	g_free (self->priv->job_description);
	self->priv->job_description = g_strdup (job_description);
	g_object_notify (G_OBJECT (self), "job-description");
}

/**
 * gdata_gd_organization_get_symbol:
 * @self: a #GDataGDOrganization
 *
 * Gets the #GDataGDOrganization:symbol property.
 *
 * Return value: the organization's stock symbol, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_gd_organization_get_symbol (GDataGDOrganization *self)
{
	g_return_val_if_fail (GDATA_IS_GD_ORGANIZATION (self), NULL);
	return self->priv->symbol;
}

/**
 * gdata_gd_organization_set_symbol:
 * @self: a #GDataGDOrganization
 * @symbol: (allow-none): the new stock symbol for the organization, or %NULL
 *
 * Sets the #GDataGDOrganization:symbol property to @symbol.
 *
 * Set @symbol to %NULL to unset the property in the organization.
 *
 * Since: 0.5.0
 */
void
gdata_gd_organization_set_symbol (GDataGDOrganization *self, const gchar *symbol)
{
	g_return_if_fail (GDATA_IS_GD_ORGANIZATION (self));

	g_free (self->priv->symbol);
	self->priv->symbol = g_strdup (symbol);
	g_object_notify (G_OBJECT (self), "symbol");
}

/**
 * gdata_gd_organization_get_location:
 * @self: a #GDataGDOrganization
 *
 * Gets the #GDataGDOrganization:location property.
 *
 * Return value: (transfer none): the organization's location, or %NULL
 *
 * Since: 0.6.0
 */
GDataGDWhere *
gdata_gd_organization_get_location (GDataGDOrganization *self)
{
	g_return_val_if_fail (GDATA_IS_GD_ORGANIZATION (self), NULL);
	return self->priv->location;
}

/**
 * gdata_gd_organization_set_location:
 * @self: a #GDataGDOrganization
 * @location: (allow-none): the new location for the organization, or %NULL
 *
 * Sets the #GDataGDOrganization:location property to @location.
 *
 * Set @location to %NULL to unset the property in the organization.
 *
 * Since: 0.6.0
 */
void
gdata_gd_organization_set_location (GDataGDOrganization *self, GDataGDWhere *location)
{
	g_return_if_fail (GDATA_IS_GD_ORGANIZATION (self));
	g_return_if_fail (location == NULL || GDATA_IS_GD_WHERE (location));

	if (self->priv->location != NULL)
		g_object_unref (self->priv->location);
	self->priv->location = (location != NULL) ? g_object_ref (location) : NULL;
	g_object_notify (G_OBJECT (self), "location");
}
