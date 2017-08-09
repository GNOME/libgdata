/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2011 <philip@tecnocode.co.uk>
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

#ifndef GDATA_PICASAWEB_COMMENT_H
#define GDATA_PICASAWEB_COMMENT_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-comment.h>

G_BEGIN_DECLS

#define GDATA_TYPE_PICASAWEB_COMMENT		(gdata_picasaweb_comment_get_type ())
#define GDATA_PICASAWEB_COMMENT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_PICASAWEB_COMMENT, GDataPicasaWebComment))
#define GDATA_PICASAWEB_COMMENT_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_PICASAWEB_COMMENT, GDataPicasaWebCommentClass))
#define GDATA_IS_PICASAWEB_COMMENT(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_PICASAWEB_COMMENT))
#define GDATA_IS_PICASAWEB_COMMENT_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_PICASAWEB_COMMENT))
#define GDATA_PICASAWEB_COMMENT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_PICASAWEB_COMMENT, GDataPicasaWebCommentClass))

typedef struct _GDataPicasaWebCommentPrivate	GDataPicasaWebCommentPrivate;

/**
 * GDataPicasaWebComment:
 *
 * All the fields in the #GDataPicasaWebComment structure are private and should never be accessed directly.
 *
 * Since: 0.10.0
 */
typedef struct {
	GDataComment parent;
	GDataPicasaWebCommentPrivate *priv;
} GDataPicasaWebComment;

/**
 * GDataPicasaWebCommentClass:
 *
 * All the fields in the #GDataPicasaWebCommentClass structure are private and should never be accessed directly.
 *
 * Since: 0.10.0
 */
typedef struct {
	/*< private >*/
	GDataCommentClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataPicasaWebCommentClass;

GType gdata_picasaweb_comment_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataPicasaWebComment, g_object_unref)

GDataPicasaWebComment *gdata_picasaweb_comment_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_PICASAWEB_COMMENT_H */
