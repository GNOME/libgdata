/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009 <philip@tecnocode.co.uk>
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

#ifndef GDATA_PICASAWEB_USER_H
#define GDATA_PICASAWEB_USER_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>
#include <gdata/gdata-entry.h>

G_BEGIN_DECLS

#define GDATA_TYPE_PICASAWEB_USER		(gdata_picasaweb_user_get_type ())
#define GDATA_PICASAWEB_USER(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_PICASAWEB_USER, GDataPicasaWebUser))
#define GDATA_PICASAWEB_USER_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_PICASAWEB_USER, GDataPicasaWebUserClass))
#define GDATA_IS_PICASAWEB_USER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_PICASAWEB_USER))
#define GDATA_IS_PICASAWEB_USER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_PICASAWEB_USER))
#define GDATA_PICASAWEB_USER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_PICASAWEB_USER, GDataPicasaWebUserClass))

typedef struct _GDataPicasaWebUserPrivate	GDataPicasaWebUserPrivate;

/**
 * GDataPicasaWebUser:
 *
 * All the fields in the #GDataPicasaWebUser structure are private and should never be accessed directly.
 *
 * Since: 0.6.0
 */
typedef struct {
	GDataEntry parent;
	GDataPicasaWebUserPrivate *priv;
} GDataPicasaWebUser;

/**
 * GDataPicasaWebUserClass:
 *
 * All the fields in the #GDataPicasaWebUserClass structure are private and should never be accessed directly.
 *
 * Since: 0.6.0
 */
typedef struct {
	/*< private >*/
	GDataEntryClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataPicasaWebUserClass;

GType gdata_picasaweb_user_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataPicasaWebUser, g_object_unref)

const gchar *gdata_picasaweb_user_get_user (GDataPicasaWebUser *self) G_GNUC_PURE;
const gchar *gdata_picasaweb_user_get_nickname (GDataPicasaWebUser *self) G_GNUC_PURE;
gint64 gdata_picasaweb_user_get_quota_limit (GDataPicasaWebUser *self) G_GNUC_PURE;
gint64 gdata_picasaweb_user_get_quota_current (GDataPicasaWebUser *self) G_GNUC_PURE;
gint gdata_picasaweb_user_get_max_photos_per_album (GDataPicasaWebUser *self) G_GNUC_PURE;
const gchar *gdata_picasaweb_user_get_thumbnail_uri (GDataPicasaWebUser *self) G_GNUC_PURE;

G_END_DECLS

#endif /* !GDATA_PICASAWEB_USER_H */
