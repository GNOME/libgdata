/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Ondrej Holy 2020 <oholy@redhat.com>
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
 * SECTION:gdata-documents-drive
 * @short_description: GData documents drive object
 * @stability: Stable
 * @include: gdata/services/documents/gdata-documents-drive.h
 *
 * #GDataDocumentsDrive is a subclass of #GDataEntry to represent an arbitrary Google Drive shared drive.
 *
 * For more details of Google Drive’s GData API, see the [online documentation](https://developers.google.com/drive/v2/web/about-sdk).
 *
 * Since: 0.18.0
 */

#include <config.h>
#include <glib.h>

#include "gdata-documents-drive.h"
#include "gdata-private.h"

static void gdata_documents_drive_finalize (GObject *object);
static void gdata_documents_drive_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);

typedef struct {
	gchar *name;
} GDataDocumentsDrivePrivate;

enum {
	PROP_NAME = 1,
};

G_DEFINE_TYPE_WITH_CODE (GDataDocumentsDrive, gdata_documents_drive, GDATA_TYPE_ENTRY,
			 G_ADD_PRIVATE (GDataDocumentsDrive));

static void
gdata_documents_drive_class_init (GDataDocumentsDriveClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);

	gobject_class->get_property = gdata_documents_drive_get_property;
	gobject_class->finalize = gdata_documents_drive_finalize;

	parsable_class->parse_json = parse_json;

	/**
	 * GDataDocumentsDrive:name:
	 *
	 * The human-readable name of this shared drive.
	 *
	 * Since: 0.18.0
	 */
	g_object_class_install_property (gobject_class, PROP_NAME,
	                                 g_param_spec_string ("name",
	                                                      "Name", "The human-readable name of this shared drive.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_documents_drive_init (GDataDocumentsDrive *self)
{
}

static void
gdata_documents_drive_finalize (GObject *object)
{
	GDataDocumentsDrivePrivate *priv = gdata_documents_drive_get_instance_private (GDATA_DOCUMENTS_DRIVE (object));

	g_free (priv->name);

	G_OBJECT_CLASS (gdata_documents_drive_parent_class)->finalize (object);
}

static void
gdata_documents_drive_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataDocumentsDrivePrivate *priv = gdata_documents_drive_get_instance_private (GDATA_DOCUMENTS_DRIVE (object));

	switch (property_id) {
		case PROP_NAME:
			g_value_set_string (value, priv->name);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

/**
 * gdata_documents_drive_get_name:
 * @self: a #GDataDocumentsDrive
 *
 * Returns the human-readable name of this shared drive.
 *
 * Return value: (nullable): the drives's human-readable name, or %NULL if not set
 *
 * Since: 0.18.0
 */
const gchar *
gdata_documents_drive_get_name (GDataDocumentsDrive *self)
{
	GDataDocumentsDrivePrivate *priv = gdata_documents_drive_get_instance_private (self);

	return priv->name;
}

static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	GDataDocumentsDrivePrivate *priv = gdata_documents_drive_get_instance_private (GDATA_DOCUMENTS_DRIVE (parsable));
	gboolean success = TRUE;
	gchar *name = NULL;

	/* JSON format: https://developers.google.com/drive/v2/reference/drives */

	if (gdata_parser_string_from_json_member (reader, "name", P_DEFAULT, &name, &success, error) == TRUE) {
		if (success)
			priv->name = name;
		return success;
	}

	return GDATA_PARSABLE_CLASS (gdata_documents_drive_parent_class)->parse_json (parsable, reader, user_data, error);
}
