/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Michael Terry 2017 <mike@mterry.name>
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
 * SECTION:gdata-documents-metadata
 * @short_description: GData document service metadata class
 * @stability: Stable
 * @include: gdata/services/documents/gdata-documents-metadata.h
 *
 * #GDataDocumentsMetadata is a subclass of #GDataParsable to represent Google Drive service metadata.
 *
 * For more details of Google Documents' GData API, see the
 * <ulink type="http" url="https://developers.google.com/drive/v2/web/about-sdk">online documentation</ulink>.
 *
 * Since: 0.17.9
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-documents-metadata.h"
#include "gdata-parser.h"
#include "gdata-types.h"

static const gchar *get_content_type (void);
static void gdata_documents_metadata_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_documents_metadata_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);

struct _GDataDocumentsMetadataPrivate {
	goffset quota_total; /* bytes */
	goffset quota_used; /* bytes */
	gboolean quota_unlimited;
};

enum {
	PROP_QUOTA_TOTAL = 1,
	PROP_QUOTA_USED,
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataDocumentsMetadata, gdata_documents_metadata, GDATA_TYPE_PARSABLE)

static void
gdata_documents_metadata_class_init (GDataDocumentsMetadataClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_documents_metadata_get_property;
	gobject_class->set_property = gdata_documents_metadata_set_property;

	parsable_class->parse_json = parse_json;
	parsable_class->get_content_type = get_content_type;

	/**
	 * GDataDocumentsMetadata:quota-total:
	 *
	 * The user quota limit across all services. Measured in bytes.
	 *
	 * Since: 0.17.9
	 */
	g_object_class_install_property (gobject_class, PROP_QUOTA_TOTAL,
	                                 g_param_spec_int64 ("quota-total",
	                                                     "Quota total", "The user quota limit across all services.",
	                                                     -1, G_MAXINT64, 0,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDocumentsMetadata:quota-used:
	 *
	 * The amount of user quota used up across all services. Measured in bytes.
	 *
	 * Since: 0.17.9
	 */
	g_object_class_install_property (gobject_class, PROP_QUOTA_USED,
	                                 g_param_spec_int64 ("quota-used",
	                                                     "Quota used", "The amount of user quota used up across all services.",
	                                                     0, G_MAXINT64, 0,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_documents_metadata_init (GDataDocumentsMetadata *self)
{
	self->priv = gdata_documents_metadata_get_instance_private (self);
}

static void
gdata_documents_metadata_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataDocumentsMetadata *self = GDATA_DOCUMENTS_METADATA (object);

	switch (property_id) {
		case PROP_QUOTA_TOTAL:
			g_value_set_int64 (value, gdata_documents_metadata_get_quota_total (self));
			break;
		case PROP_QUOTA_USED:
			g_value_set_int64 (value, gdata_documents_metadata_get_quota_used (self));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_documents_metadata_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_QUOTA_TOTAL:
		case PROP_QUOTA_USED:
			/* Read only. */
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	GDataDocumentsMetadataPrivate *priv = GDATA_DOCUMENTS_METADATA (parsable)->priv;
	gboolean success = TRUE;
	gchar *quota_total = NULL;
	gchar *quota_used = NULL;
	gchar *quota_type = NULL;

	/* JSON format: https://developers.google.com/drive/v2/reference/about */

	if (gdata_parser_string_from_json_member (reader, "quotaBytesTotal", P_DEFAULT, &quota_total, &success, error) == TRUE) {
		gchar *end_ptr;
		guint64 val;

		/* Even though ‘quotaBytesTotal’ is documented as long,
		 * it is actually a string in the JSON.
		 */
		val = g_ascii_strtoull (quota_total, &end_ptr, 10);
		if (*end_ptr == '\0')
			priv->quota_total = (goffset) val;
		g_free (quota_total);
		return success;
	} else if (gdata_parser_string_from_json_member (reader, "quotaBytesUsedAggregate", P_DEFAULT, &quota_used, &success, error) == TRUE) {
		gchar *end_ptr;
		guint64 val;

		/* Even though ‘quotaBytesUsedAggregate’ is documented as long,
		 * it is actually a string in the JSON.
		 */
		val = g_ascii_strtoull (quota_used, &end_ptr, 10);
		if (*end_ptr == '\0')
			priv->quota_used = (goffset) val;
		g_free (quota_used);
		return success;
	} else if (gdata_parser_string_from_json_member (reader, "quotaType", P_DEFAULT, &quota_type, &success, error) == TRUE) {
		if (g_strcmp0 (quota_type, "UNLIMITED") == 0)
			priv->quota_unlimited = TRUE;
		g_free (quota_type);
		return success;
	}

	return GDATA_PARSABLE_CLASS (gdata_documents_metadata_parent_class)->parse_json (parsable, reader, user_data, error);
}

static const gchar *
get_content_type (void)
{
	return "application/json";
}

/**
 * gdata_documents_metadata_get_quota_total:
 * @self: a #GDataDocumentsMetadata
 *
 * Gets the #GDataDocumentsMetadata:quota-total property.
 *
 * Return value: the number of quota bytes available in total. Returns -1 if
 *               there is no quota limit.
 *
 * Since: 0.17.9
 */
goffset
gdata_documents_metadata_get_quota_total (GDataDocumentsMetadata *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_METADATA (self), 0);

	if (self->priv->quota_unlimited)
		return -1;
	else
		return self->priv->quota_total;
}

/**
 * gdata_documents_metadata_get_quota_used:
 * @self: a #GDataDocumentsMetadata
 *
 * Gets the #GDataDocumentsMetadata:quota-used property.
 *
 * Return value: the number of quota bytes used by the documents service
 *
 * Since: 0.17.9
 */
goffset
gdata_documents_metadata_get_quota_used (GDataDocumentsMetadata *self)
{
	g_return_val_if_fail (GDATA_IS_DOCUMENTS_METADATA (self), 0);

	return self->priv->quota_used;
}
