/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009–2010 <philip@tecnocode.co.uk>
 * Copyright (C) Richard Schwarting 2009 <aquarichy@gmail.com>
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
 * SECTION:gdata-picasaweb-user
 * @short_description: GData PicasaWeb User object
 * @stability: Stable
 * @include: gdata/services/picasaweb/gdata-picasaweb-user.h
 *
 * #GDataPicasaWebUser is a subclass of #GDataEntry to represent properties for a PicasaWeb user. It adds a couple of
 * properties which are specific to the Google PicasaWeb API.
 *
 * Since: 0.6.0
 */

#include <glib.h>
#include <libxml/parser.h>

#include "gdata-picasaweb-user.h"
#include "gdata-entry.h"
#include "gdata-private.h"

static void gdata_picasaweb_user_finalize (GObject *object);
static void gdata_picasaweb_user_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static gboolean parse_xml (GDataParsable *parsable, xmlDoc *doc, xmlNode *node, gpointer user_data, GError **error);
static void get_namespaces (GDataParsable *parsable, GHashTable *namespaces);

struct _GDataPicasaWebUserPrivate {
	gchar *user;
	gchar *nickname;
	gint64 quota_limit;
	gint64 quota_current;
	gint max_photos_per_album;
	gchar *thumbnail_uri;
};

enum {
	PROP_USER = 1,
	PROP_NICKNAME,
	PROP_QUOTA_LIMIT,
	PROP_QUOTA_CURRENT,
	PROP_MAX_PHOTOS_PER_ALBUM,
	PROP_THUMBNAIL_URI
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataPicasaWebUser, gdata_picasaweb_user, GDATA_TYPE_ENTRY)

static void
gdata_picasaweb_user_class_init (GDataPicasaWebUserClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	gobject_class->get_property = gdata_picasaweb_user_get_property;
	gobject_class->finalize = gdata_picasaweb_user_finalize;

	parsable_class->parse_xml = parse_xml;
	parsable_class->get_namespaces = get_namespaces;

	entry_class->kind_term = "http://schemas.google.com/photos/2007#user";

	/**
	 * GDataPicasaWebUser:user:
	 *
	 * The username of the user, as seen in feed URLs.
	 * http://code.google.com/apis/picasaweb/docs/2.0/reference.html#gphoto_user
	 *
	 * Since: 0.6.0
	 */
	g_object_class_install_property (gobject_class, PROP_USER,
	                                 g_param_spec_string ("user",
	                                                      "User", "The username of the user.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebUser:nickname:
	 *
	 * The user's nickname. This is a user-specified value that should be used when referring to the user by name.
	 * http://code.google.com/apis/picasaweb/docs/2.0/reference.html#gphoto_nickname
	 *
	 * Since: 0.6.0
	 */
	g_object_class_install_property (gobject_class, PROP_NICKNAME,
	                                 g_param_spec_string ("nickname",
	                                                      "Nickname", "The user's nickname.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebUser:quota-limit:
	 *
	 * The total amount of space, in bytes, available to the user.
	 * http://code.google.com/apis/picasaweb/docs/2.0/reference.html#gphoto_quotalimit
	 *
	 * If the #GDataPicasaWebUser does not represent the currently authenticated user, this will be <code class="literal">-1</code>.
	 *
	 * Since: 0.6.0
	 */
	g_object_class_install_property (gobject_class, PROP_QUOTA_LIMIT,
	                                 g_param_spec_int64 ("quota-limit",
	                                                     "Quota Limit", "The total amount of space, in bytes, available to the user.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebUser:quota-current:
	 *
	 * The current amount of space, in bytes, already used by the user.
	 * http://code.google.com/apis/picasaweb/docs/2.0/reference.html#gphoto_quotacurrent
	 *
	 * If the #GDataPicasaWebUser does not represent the currently authenticated user, this will be <code class="literal">-1</code>.
	 *
	 * Since: 0.6.0
	 */
	g_object_class_install_property (gobject_class, PROP_QUOTA_CURRENT,
	                                 g_param_spec_int64 ("quota-current",
	                                                     "Quota Current", "The current amount of space, in bytes, already used by the user.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebUser:max-photos-per-album:
	 *
	 * The maximum number of photos allowed in an album.
	 * http://code.google.com/apis/picasaweb/docs/2.0/reference.html#gphoto_maxPhotosPerAlbum
	 *
	 * If the #GDataPicasaWebUser does not represent the currently authenticated user, this will be <code class="literal">-1</code>.
	 *
	 * Since: 0.6.0
	 */
	g_object_class_install_property (gobject_class, PROP_MAX_PHOTOS_PER_ALBUM,
	                                 g_param_spec_int ("max-photos-per-album",
	                                                   "Max Photos Per Album", "The maximum number of photos allowed in an album.",
	                                                   -1, G_MAXINT, -1,
	                                                   G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataPicasaWebUser:thumbnail-uri:
	 *
	 * The URI of a thumbnail-sized portrait of the user.
	 * http://code.google.com/apis/picasaweb/docs/2.0/reference.html#gphoto_thumbnail
	 *
	 * Since: 0.6.0
	 */
	g_object_class_install_property (gobject_class, PROP_THUMBNAIL_URI,
	                                 g_param_spec_string ("thumbnail-uri",
	                                                      "Thumbnail URI", "The URI of a thumbnail-sized portrait of the user.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_picasaweb_user_init (GDataPicasaWebUser *self)
{
	self->priv = gdata_picasaweb_user_get_instance_private (self);

	/* Initialise the properties whose values we can theoretically not know */
	self->priv->quota_limit = self->priv->quota_current = self->priv->max_photos_per_album = -1;
}

static void
gdata_picasaweb_user_finalize (GObject *object)
{
	GDataPicasaWebUserPrivate *priv = GDATA_PICASAWEB_USER (object)->priv;

	g_free (priv->user);
	g_free (priv->nickname);
	g_free (priv->thumbnail_uri);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_picasaweb_user_parent_class)->finalize (object);
}

static void
gdata_picasaweb_user_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataPicasaWebUserPrivate *priv = GDATA_PICASAWEB_USER (object)->priv;

	switch (property_id) {
		case PROP_USER:
			g_value_set_string (value, priv->user);
			break;
		case PROP_NICKNAME:
			g_value_set_string (value, priv->nickname);
			break;
		case PROP_QUOTA_LIMIT:
			g_value_set_int64 (value, priv->quota_limit);
			break;
		case PROP_QUOTA_CURRENT:
			g_value_set_int64 (value, priv->quota_current);
			break;
		case PROP_MAX_PHOTOS_PER_ALBUM:
			g_value_set_int (value, priv->max_photos_per_album);
			break;
		case PROP_THUMBNAIL_URI:
			g_value_set_string (value, priv->thumbnail_uri);
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
	GDataPicasaWebUser *self = GDATA_PICASAWEB_USER (parsable);

	if (gdata_parser_is_namespace (node, "http://schemas.google.com/photos/2007") == FALSE)
		return GDATA_PARSABLE_CLASS (gdata_picasaweb_user_parent_class)->parse_xml (parsable, doc, node, user_data, error);

	if (gdata_parser_string_from_element (node, "user", P_REQUIRED | P_NON_EMPTY, &(self->priv->user), &success, error) == TRUE ||
	    gdata_parser_string_from_element (node, "nickname", P_REQUIRED | P_NON_EMPTY, &(self->priv->nickname), &success, error) == TRUE ||
	    gdata_parser_string_from_element (node, "thumbnail", P_REQUIRED | P_NON_EMPTY, &(self->priv->thumbnail_uri), &success, error) == TRUE ||
	    gdata_parser_int64_from_element (node, "quotacurrent", P_REQUIRED | P_NO_DUPES, &(self->priv->quota_current), -1, &success, error) == TRUE ||
	    gdata_parser_int64_from_element (node, "quotalimit", P_REQUIRED | P_NO_DUPES, &(self->priv->quota_limit), -1, &success, error) == TRUE) {
		return success;
	} else if (xmlStrcmp (node->name, (xmlChar*) "maxPhotosPerAlbum") == 0) {
		/* gphoto:max-photos-per-album */
		xmlChar *max_photos_per_album = xmlNodeListGetString (doc, node->children, TRUE);
		self->priv->max_photos_per_album = g_ascii_strtoll ((char*) max_photos_per_album, NULL, 10);
		xmlFree (max_photos_per_album);
	} else if (xmlStrcmp (node->name, (xmlChar*) "x-allowDownloads") == 0) { /* RHSTODO: see if this comes with the user */
		/* gphoto:allowDownloads */
		/* TODO: Not part of public API so we're capturing and ignoring for now.  See bgo #589858. */
	} else if (xmlStrcmp (node->name, (xmlChar*) "x-allowPrints") == 0) { /* RHSTODO: see if this comes with the user */
		/* gphoto:allowPrints */
		/* TODO: Not part of public API so we're capturing and ignoring for now.  See bgo #589858. */
	} else {
		return GDATA_PARSABLE_CLASS (gdata_picasaweb_user_parent_class)->parse_xml (parsable, doc, node, user_data, error);
	}

	return TRUE;
}

static void
get_namespaces (GDataParsable *parsable, GHashTable *namespaces)
{
	/* Chain up to the parent class */
	GDATA_PARSABLE_CLASS (gdata_picasaweb_user_parent_class)->get_namespaces (parsable, namespaces);

	g_hash_table_insert (namespaces, (gchar*) "gphoto", (gchar*) "http://schemas.google.com/photos/2007");
}

/**
 * gdata_picasaweb_user_get_user:
 * @self: a #GDataPicasaWebUser
 *
 * Gets the #GDataPicasaWebUser:user property.
 *
 * Return value: the feed's user, or %NULL
 *
 * Since: 0.6.0
 */
const gchar *
gdata_picasaweb_user_get_user (GDataPicasaWebUser *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_USER (self), NULL);
	return self->priv->user;
}

/**
 * gdata_picasaweb_user_get_nickname:
 * @self: a #GDataPicasaWebUser
 *
 * Gets the #GDataPicasaWebUser:nickname property.
 *
 * Return value: the nickname of the feed's user's nickname, or %NULL
 *
 * Since: 0.6.0
 */
const gchar *
gdata_picasaweb_user_get_nickname (GDataPicasaWebUser *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_USER (self), NULL);
	return self->priv->nickname;
}

/**
 * gdata_picasaweb_user_get_quota_limit:
 * @self: a #GDataPicasaWebUser
 *
 * Gets the #GDataPicasaWebUser:quota-limit property. Note that
 * this information is not available when accessing feeds which we
 * haven't authenticated, and <code class="literal">0</code> is returned.
 *
 * Return value: the maximum capacity in bytes for this feed's account, or <code class="literal">-1</code>
 *
 * Since: 0.6.0
 */
gint64
gdata_picasaweb_user_get_quota_limit (GDataPicasaWebUser *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_USER (self), -1);
	return self->priv->quota_limit;
}

/**
 * gdata_picasaweb_user_get_quota_current:
 * @self: a #GDataPicasaWebUser
 *
 * Gets the #GDataPicasaWebUser:quota-current property.  Note that
 * this information is not available when accessing feeds which we
 * haven't authenticated, and <code class="literal">0</code> is returned.
 *
 * Return value: the current number of bytes in use by this feed's account, or <code class="literal">-1</code>
 *
 * Since: 0.6.0
 */
gint64
gdata_picasaweb_user_get_quota_current (GDataPicasaWebUser *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_USER (self), -1);
	return self->priv->quota_current;
}

/**
 * gdata_picasaweb_user_get_max_photos_per_album:
 * @self: a #GDataPicasaWebUser
 *
 * Gets the #GDataPicasaWebUser:max-photos-per-album property.  Note that
 * this information is not available when accessing feeds which we
 * haven't authenticated, and <code class="literal">0</code> is returned.
 *
 * Return value: the maximum number of photos an album for this account can hold, or <code class="literal">-1</code>
 *
 * Since: 0.6.0
 */
gint
gdata_picasaweb_user_get_max_photos_per_album (GDataPicasaWebUser *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_USER (self), -1);
	return self->priv->max_photos_per_album;
}

/**
 * gdata_picasaweb_user_get_thumbnail_uri:
 * @self: a #GDataPicasaWebUser
 *
 * Gets the #GDataPicasaWebUser:thumbnail-uri property.
 *
 * Return value: the URI for the thumbnail of the account, or %NULL
 *
 * Since: 0.6.0
 */
const gchar *
gdata_picasaweb_user_get_thumbnail_uri (GDataPicasaWebUser *self)
{
	g_return_val_if_fail (GDATA_IS_PICASAWEB_USER (self), NULL);
	return self->priv->thumbnail_uri;
}

/* TODO: in the future, see if we can change things like the user's nickname and thumbnail/avatar */
