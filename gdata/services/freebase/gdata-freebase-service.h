/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) 2014 Carlos Garnacho <carlosg@gnome.org>
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

#ifndef GDATA_FREEBASE_SERVICE_H
#define GDATA_FREEBASE_SERVICE_H

#include <glib.h>
#include <glib-object.h>
#include <libsoup/soup.h>

#include <gdata/gdata-service.h>
#include <gdata/gdata-download-stream.h>
#include "gdata-freebase-query.h"
#include "gdata-freebase-result.h"
#include "gdata-freebase-search-query.h"
#include "gdata-freebase-search-result.h"
#include "gdata-freebase-topic-query.h"
#include "gdata-freebase-topic-result.h"

G_BEGIN_DECLS

#ifndef LIBGDATA_DISABLE_DEPRECATED

#define GDATA_TYPE_FREEBASE_SERVICE		(gdata_freebase_service_get_type ())
#define GDATA_FREEBASE_SERVICE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_FREEBASE_SERVICE, GDataFreebaseService))
#define GDATA_FREEBASE_SERVICE_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_FREEBASE_SERVICE, GDataFreebaseServiceClass))
#define GDATA_IS_FREEBASE_SERVICE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_FREEBASE_SERVICE))
#define GDATA_IS_FREEBASE_SERVICE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_FREEBASE_SERVICE))
#define GDATA_FREEBASE_SERVICE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_FREEBASE_SERVICE, GDataFreebaseServiceClass))

typedef struct _GDataFreebaseServicePrivate	GDataFreebaseServicePrivate;

/**
 * GDataFreebaseService:
 *
 * All the fields in the #GDataFreebaseService structure are private and should never be accessed directly.
 *
 * Since: 0.15.1
 */
typedef struct {
	GDataService parent;
	/*< private >*/
	GDataFreebaseServicePrivate *priv;
} GDataFreebaseService;

/**
 * GDataFreebaseServiceClass:
 *
 * All the fields in the #GDataFreebaseServiceClass structure are private and should never be accessed directly.
 *
 * Since: 0.15.1
 */
typedef struct {
	/*< private >*/
	GDataServiceClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
	void (*_g_reserved2) (void);
	void (*_g_reserved3) (void);
	void (*_g_reserved4) (void);
	void (*_g_reserved5) (void);
} GDataFreebaseServiceClass;

GType gdata_freebase_service_get_type (void) G_GNUC_CONST G_GNUC_DEPRECATED;

GDataFreebaseService *gdata_freebase_service_new (const gchar *developer_key, GDataAuthorizer *authorizer) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC G_GNUC_DEPRECATED;

GDataAuthorizationDomain *gdata_freebase_service_get_primary_authorization_domain (void) G_GNUC_CONST G_GNUC_DEPRECATED;

GDataFreebaseResult *gdata_freebase_service_query (GDataFreebaseService *self, GDataFreebaseQuery *query,
						   GCancellable *cancellable, GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC G_GNUC_DEPRECATED;
void gdata_freebase_service_query_async (GDataFreebaseService *self, GDataFreebaseQuery *query,
					 GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data) G_GNUC_DEPRECATED;

GDataFreebaseTopicResult *gdata_freebase_service_get_topic (GDataFreebaseService *self, GDataFreebaseTopicQuery *query,
							    GCancellable *cancellable, GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC G_GNUC_DEPRECATED;
void gdata_freebase_service_get_topic_async (GDataFreebaseService *self, GDataFreebaseTopicQuery *query,
					     GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data) G_GNUC_DEPRECATED;

GDataFreebaseSearchResult *gdata_freebase_service_search (GDataFreebaseService *self, GDataFreebaseSearchQuery *query,
							  GCancellable *cancellable, GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC G_GNUC_DEPRECATED;
void gdata_freebase_service_search_async (GDataFreebaseService *self, GDataFreebaseSearchQuery *query,
					  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data) G_GNUC_DEPRECATED;

GInputStream *gdata_freebase_service_get_image (GDataFreebaseService *self, GDataFreebaseTopicValue *value,
						GCancellable *cancellable, guint max_width, guint max_height, GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC G_GNUC_DEPRECATED;

#endif /* !LIBGDATA_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* !GDATA_FREEBASE_SERVICE_H */
