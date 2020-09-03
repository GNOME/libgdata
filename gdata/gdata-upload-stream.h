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

#ifndef GDATA_UPLOAD_STREAM_H
#define GDATA_UPLOAD_STREAM_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <gdata/gdata-service.h>
#include <gdata/gdata-entry.h>

G_BEGIN_DECLS

/**
 * GDATA_LINK_RESUMABLE_CREATE_MEDIA:
 *
 * The relation type URI of the resumable upload location for resources attached to this resource.
 *
 * For more information, see the
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/resumable_upload.html#ResumableUploadInitiate">GData resumable upload protocol
 * specification</ulink>.
 *
 * Since: 0.13.0
 */
#define GDATA_LINK_RESUMABLE_CREATE_MEDIA "http://schemas.google.com/g/2005#resumable-create-media"

/**
 * GDATA_LINK_RESUMABLE_EDIT_MEDIA:
 *
 * The relation type URI of the resumable update location for resources attached to this resource.
 *
 * For more information, see the
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/resumable_upload.html#ResumableUploadInitiate">GData resumable upload protocol
 * specification</ulink>.
 *
 * Since: 0.13.0
 */
#define GDATA_LINK_RESUMABLE_EDIT_MEDIA "http://schemas.google.com/g/2005#resumable-edit-media"

#define GDATA_TYPE_UPLOAD_STREAM		(gdata_upload_stream_get_type ())
#define GDATA_UPLOAD_STREAM(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_UPLOAD_STREAM, GDataUploadStream))
#define GDATA_UPLOAD_STREAM_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_UPLOAD_STREAM, GDataUploadStreamClass))
#define GDATA_IS_UPLOAD_STREAM(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_UPLOAD_STREAM))
#define GDATA_IS_UPLOAD_STREAM_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_UPLOAD_STREAM))
#define GDATA_UPLOAD_STREAM_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_UPLOAD_STREAM, GDataUploadStreamClass))

typedef struct _GDataUploadStreamPrivate	GDataUploadStreamPrivate;

/**
 * GDataUploadStream:
 *
 * All the fields in the #GDataUploadStream structure are private and should never be accessed directly.
 *
 * Since: 0.5.0
 */
typedef struct {
	GOutputStream parent;
	GDataUploadStreamPrivate *priv;
} GDataUploadStream;

/**
 * GDataUploadStreamClass:
 *
 * All the fields in the #GDataUploadStreamClass structure are private and should never be accessed directly.
 *
 * Since: 0.5.0
 */
typedef struct {
	/*< private >*/
	GOutputStreamClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
	void (*_g_reserved2) (void);
	void (*_g_reserved3) (void);
	void (*_g_reserved4) (void);
	void (*_g_reserved5) (void);
} GDataUploadStreamClass;

GType gdata_upload_stream_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataUploadStream, g_object_unref)

GOutputStream *gdata_upload_stream_new (GDataService *service, GDataAuthorizationDomain *domain, const gchar *method, const gchar *upload_uri,
                                        GDataEntry *entry, const gchar *slug, const gchar *content_type,
                                        GCancellable *cancellable) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
GOutputStream *gdata_upload_stream_new_resumable (GDataService *service, GDataAuthorizationDomain *domain, const gchar *method, const gchar *upload_uri,
                                                  GDataEntry *entry, const gchar *slug, const gchar *content_type, goffset content_length,
                                                  GCancellable *cancellable) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_upload_stream_get_response (GDataUploadStream *self, gssize *length);

GDataService *gdata_upload_stream_get_service (GDataUploadStream *self) G_GNUC_PURE;
GDataAuthorizationDomain *gdata_upload_stream_get_authorization_domain (GDataUploadStream *self) G_GNUC_PURE;
const gchar *gdata_upload_stream_get_method (GDataUploadStream *self) G_GNUC_PURE;
const gchar *gdata_upload_stream_get_upload_uri (GDataUploadStream *self) G_GNUC_PURE;
GDataEntry *gdata_upload_stream_get_entry (GDataUploadStream *self) G_GNUC_PURE;
const gchar *gdata_upload_stream_get_slug (GDataUploadStream *self) G_GNUC_PURE;
const gchar *gdata_upload_stream_get_content_type (GDataUploadStream *self) G_GNUC_PURE;
goffset gdata_upload_stream_get_content_length (GDataUploadStream *self) G_GNUC_PURE;
GCancellable *gdata_upload_stream_get_cancellable (GDataUploadStream *self) G_GNUC_PURE;

G_END_DECLS

#endif /* !GDATA_UPLOAD_STREAM_H */
