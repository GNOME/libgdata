/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Richard Schwarting 2009 <aquarichy@gmail.com>
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

#ifndef GDATA_PICASAWEB_FILE_H
#define GDATA_PICASAWEB_FILE_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-entry.h>
#include <gdata/gdata-types.h>

G_BEGIN_DECLS

/**
 * GDATA_PICASAWEB_VIDEO_STATUS_PENDING:
 *
 * The video is still being processed.
 *
 * Since: 0.7.0
 */
#define GDATA_PICASAWEB_VIDEO_STATUS_PENDING "pending"

/**
 * GDATA_PICASAWEB_VIDEO_STATUS_READY:
 *
 * The video has been processed, but still needs thumbnailing.
 *
 * Since: 0.7.0
 */
#define GDATA_PICASAWEB_VIDEO_STATUS_READY "ready"

/**
 * GDATA_PICASAWEB_VIDEO_STATUS_FINAL:
 *
 * The video has been processed and thumbnailed.
 *
 * Since: 0.7.0
 */
#define GDATA_PICASAWEB_VIDEO_STATUS_FINAL "final"

/**
 * GDATA_PICASAWEB_VIDEO_STATUS_FAILED:
 *
 * There was an error while processing or thumbnailing the video and it should be deleted.
 *
 * Since: 0.7.0
 */
#define GDATA_PICASAWEB_VIDEO_STATUS_FAILED "failed"

#define GDATA_TYPE_PICASAWEB_FILE 		(gdata_picasaweb_file_get_type ())
#define GDATA_PICASAWEB_FILE(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_PICASAWEB_FILE, GDataPicasaWebFile))
#define GDATA_PICASAWEB_FILE_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_PICASAWEB_FILE, GDataPicasaWebFileClass))
#define GDATA_IS_PICASAWEB_FILE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_PICASAWEB_FILE))
#define GDATA_IS_PICASAWEB_FILE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_PICASAWEB_FILE))
#define GDATA_PICASAWEB_FILE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_PICASAWEB_FILE, GDataPicasaWebFileClass))

typedef struct _GDataPicasaWebFilePrivate	GDataPicasaWebFilePrivate;

/**
 * GDataPicasaWebFile:
 *
 * All the fields in the #GDataPicasaWebFile structure are private and should never be accessed directly.
 *
 * Since: 0.4.0
 */
typedef struct {
	GDataEntry parent;
	GDataPicasaWebFilePrivate *priv;
} GDataPicasaWebFile;

/**
 * GDataPicasaWebFileClass:
 *
 * All the fields in the #GDataPicasaWebFileClass structure are private and should never be accessed directly.
 *
 * Since: 0.4.0
 */
typedef struct {
	/*< private >*/
	GDataEntryClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataPicasaWebFileClass;

GType gdata_picasaweb_file_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataPicasaWebFile, g_object_unref)

GDataPicasaWebFile *gdata_picasaweb_file_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_picasaweb_file_get_id (GDataPicasaWebFile *self) G_GNUC_PURE;
gint64 gdata_picasaweb_file_get_edited (GDataPicasaWebFile *self);
const gchar *gdata_picasaweb_file_get_version (GDataPicasaWebFile *self) G_GNUC_PURE;
const gchar *gdata_picasaweb_file_get_album_id (GDataPicasaWebFile *self) G_GNUC_PURE;
/* TODO should we have a more obvious _move() API too? nah */
void gdata_picasaweb_file_set_album_id (GDataPicasaWebFile *self, const gchar *album_id);
guint gdata_picasaweb_file_get_width (GDataPicasaWebFile *self) G_GNUC_PURE;
guint gdata_picasaweb_file_get_height (GDataPicasaWebFile *self) G_GNUC_PURE;
gsize gdata_picasaweb_file_get_size (GDataPicasaWebFile *self) G_GNUC_PURE;
const gchar *gdata_picasaweb_file_get_checksum (GDataPicasaWebFile *self) G_GNUC_PURE;
void gdata_picasaweb_file_set_checksum (GDataPicasaWebFile *self, const gchar *checksum);
gint64 gdata_picasaweb_file_get_timestamp (GDataPicasaWebFile *self);
void gdata_picasaweb_file_set_timestamp (GDataPicasaWebFile *self, gint64 timestamp);
gboolean gdata_picasaweb_file_is_commenting_enabled (GDataPicasaWebFile *self) G_GNUC_PURE;
void gdata_picasaweb_file_set_is_commenting_enabled (GDataPicasaWebFile *self, gboolean is_commenting_enabled);
guint gdata_picasaweb_file_get_comment_count (GDataPicasaWebFile *self) G_GNUC_PURE;
guint gdata_picasaweb_file_get_rotation (GDataPicasaWebFile *self) G_GNUC_PURE;
void gdata_picasaweb_file_set_rotation (GDataPicasaWebFile *self, guint rotation);
const gchar *gdata_picasaweb_file_get_video_status (GDataPicasaWebFile *self) G_GNUC_PURE;
const gchar * const *gdata_picasaweb_file_get_tags (GDataPicasaWebFile *self) G_GNUC_PURE;
void gdata_picasaweb_file_set_tags (GDataPicasaWebFile *self, const gchar * const *tags);
const gchar *gdata_picasaweb_file_get_credit (GDataPicasaWebFile *self) G_GNUC_PURE;
const gchar *gdata_picasaweb_file_get_caption (GDataPicasaWebFile *self) G_GNUC_PURE;
void gdata_picasaweb_file_set_caption (GDataPicasaWebFile *self, const gchar *caption);
GList *gdata_picasaweb_file_get_contents (GDataPicasaWebFile *self) G_GNUC_PURE;
GList *gdata_picasaweb_file_get_thumbnails (GDataPicasaWebFile *self) G_GNUC_PURE;
gdouble gdata_picasaweb_file_get_distance (GDataPicasaWebFile *self) G_GNUC_PURE;
gdouble gdata_picasaweb_file_get_exposure (GDataPicasaWebFile *self) G_GNUC_PURE;
gboolean gdata_picasaweb_file_get_flash (GDataPicasaWebFile *self) G_GNUC_PURE;
gdouble gdata_picasaweb_file_get_focal_length (GDataPicasaWebFile *self) G_GNUC_PURE;
gdouble gdata_picasaweb_file_get_fstop (GDataPicasaWebFile *self) G_GNUC_PURE;
const gchar *gdata_picasaweb_file_get_image_unique_id (GDataPicasaWebFile *self) G_GNUC_PURE;
gint gdata_picasaweb_file_get_iso (GDataPicasaWebFile *self) G_GNUC_PURE;
const gchar *gdata_picasaweb_file_get_make (GDataPicasaWebFile *self) G_GNUC_PURE;
const gchar *gdata_picasaweb_file_get_model (GDataPicasaWebFile *self) G_GNUC_PURE;
void gdata_picasaweb_file_get_coordinates (GDataPicasaWebFile *self, gdouble *latitude, gdouble *longitude);
void gdata_picasaweb_file_set_coordinates (GDataPicasaWebFile *self, gdouble latitude, gdouble longitude);

/* TODO implement is video */
/* TODO implement get comments */

G_END_DECLS

#endif /* !GDATA_PICASAWEB_FILE_H */
