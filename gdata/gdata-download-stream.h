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

#ifndef GDATA_DOWNLOAD_STREAM_H
#define GDATA_DOWNLOAD_STREAM_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <gdata/gdata-service.h>

G_BEGIN_DECLS

#define GDATA_TYPE_DOWNLOAD_STREAM		(gdata_download_stream_get_type ())
#define GDATA_DOWNLOAD_STREAM(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_DOWNLOAD_STREAM, GDataDownloadStream))
#define GDATA_DOWNLOAD_STREAM_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_DOWNLOAD_STREAM, GDataDownloadStreamClass))
#define GDATA_IS_DOWNLOAD_STREAM(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_DOWNLOAD_STREAM))
#define GDATA_IS_DOWNLOAD_STREAM_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_DOWNLOAD_STREAM))
#define GDATA_DOWNLOAD_STREAM_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_DOWNLOAD_STREAM, GDataDownloadStreamClass))

typedef struct _GDataDownloadStreamPrivate	GDataDownloadStreamPrivate;

/**
 * GDataDownloadStream:
 *
 * All the fields in the #GDataDownloadStream structure are private and should never be accessed directly.
 *
 * Since: 0.5.0
 */
typedef struct {
	GInputStream parent;
	GDataDownloadStreamPrivate *priv;
} GDataDownloadStream;

/**
 * GDataDownloadStreamClass:
 *
 * All the fields in the #GDataDownloadStreamClass structure are private and should never be accessed directly.
 *
 * Since: 0.5.0
 */
typedef struct {
	/*< private >*/
	GInputStreamClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
	void (*_g_reserved2) (void);
	void (*_g_reserved3) (void);
	void (*_g_reserved4) (void);
	void (*_g_reserved5) (void);
} GDataDownloadStreamClass;

GType gdata_download_stream_get_type (void) G_GNUC_CONST;

GInputStream *gdata_download_stream_new (GDataService *service, GDataAuthorizationDomain *domain, const gchar *download_uri,
                                         GCancellable *cancellable) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

GDataService *gdata_download_stream_get_service (GDataDownloadStream *self) G_GNUC_PURE;
GDataAuthorizationDomain *gdata_download_stream_get_authorization_domain (GDataDownloadStream *self) G_GNUC_PURE;
const gchar *gdata_download_stream_get_download_uri (GDataDownloadStream *self) G_GNUC_PURE;
const gchar *gdata_download_stream_get_content_type (GDataDownloadStream *self) G_GNUC_PURE;
gssize gdata_download_stream_get_content_length (GDataDownloadStream *self) G_GNUC_PURE;
GCancellable *gdata_download_stream_get_cancellable (GDataDownloadStream *self) G_GNUC_PURE;

G_END_DECLS

#endif /* !GDATA_DOWNLOAD_STREAM_H */
