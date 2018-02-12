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

#ifndef GDATA_OAUTH1_AUTHORIZER_H
#define GDATA_OAUTH1_AUTHORIZER_H

#include <glib.h>
#include <glib-object.h>

#include "gdata-authorizer.h"

G_BEGIN_DECLS

#define GDATA_TYPE_OAUTH1_AUTHORIZER		(gdata_oauth1_authorizer_get_type ())
#define GDATA_OAUTH1_AUTHORIZER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_OAUTH1_AUTHORIZER, GDataOAuth1Authorizer))
#define GDATA_OAUTH1_AUTHORIZER_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_OAUTH1_AUTHORIZER, GDataOAuth1AuthorizerClass))
#define GDATA_IS_OAUTH1_AUTHORIZER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_OAUTH1_AUTHORIZER))
#define GDATA_IS_OAUTH1_AUTHORIZER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_OAUTH1_AUTHORIZER))
#define GDATA_OAUTH1_AUTHORIZER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_OAUTH1_AUTHORIZER, GDataOAuth1AuthorizerClass))

typedef struct _GDataOAuth1AuthorizerPrivate	GDataOAuth1AuthorizerPrivate;

/**
 * GDataOAuth1Authorizer:
 *
 * All the fields in the #GDataOAuth1Authorizer structure are private and should never be accessed directly.
 *
 * Since: 0.9.0
 */
typedef struct {
	/*< private >*/
	GObject parent;
	GDataOAuth1AuthorizerPrivate *priv;
} GDataOAuth1Authorizer;

/**
 * GDataOAuth1AuthorizerClass:
 *
 * All the fields in the #GDataOAuth1AuthorizerClass structure are private and should never be accessed directly.
 *
 * Since: 0.9.0
 */
typedef struct {
	/*< private >*/
	GObjectClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataOAuth1AuthorizerClass;

GType gdata_oauth1_authorizer_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataOAuth1Authorizer, g_object_unref)

GDataOAuth1Authorizer *gdata_oauth1_authorizer_new (const gchar *application_name, GType service_type) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
GDataOAuth1Authorizer *gdata_oauth1_authorizer_new_for_authorization_domains (const gchar *application_name,
                                                                              GList *authorization_domains) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

gchar *gdata_oauth1_authorizer_request_authentication_uri (GDataOAuth1Authorizer *self, gchar **token, gchar **token_secret,
                                                           GCancellable *cancellable, GError **error);
void gdata_oauth1_authorizer_request_authentication_uri_async (GDataOAuth1Authorizer *self, GCancellable *cancellable,
                                                               GAsyncReadyCallback callback, gpointer user_data);
gchar *gdata_oauth1_authorizer_request_authentication_uri_finish (GDataOAuth1Authorizer *self, GAsyncResult *async_result, gchar **token,
                                                                  gchar **token_secret, GError **error);

gboolean gdata_oauth1_authorizer_request_authorization (GDataOAuth1Authorizer *self, const gchar *token, const gchar *token_secret,
                                                        const gchar *verifier, GCancellable *cancellable, GError **error);
void gdata_oauth1_authorizer_request_authorization_async (GDataOAuth1Authorizer *self, const gchar *token, const gchar *token_secret,
                                                          const gchar *verifier, GCancellable *cancellable, GAsyncReadyCallback callback,
                                                          gpointer user_data);
gboolean gdata_oauth1_authorizer_request_authorization_finish (GDataOAuth1Authorizer *self, GAsyncResult *async_result, GError **error);

const gchar *gdata_oauth1_authorizer_get_application_name (GDataOAuth1Authorizer *self) G_GNUC_PURE;

const gchar *gdata_oauth1_authorizer_get_locale (GDataOAuth1Authorizer *self) G_GNUC_PURE;
void gdata_oauth1_authorizer_set_locale (GDataOAuth1Authorizer *self, const gchar *locale);

#ifndef LIBGDATA_DISABLE_DEPRECATED
SoupURI *gdata_oauth1_authorizer_get_proxy_uri (GDataOAuth1Authorizer *self) G_GNUC_PURE G_GNUC_DEPRECATED_FOR (gdata_oauth1_authorizer_get_proxy_resolver);
void gdata_oauth1_authorizer_set_proxy_uri (GDataOAuth1Authorizer *self, SoupURI *proxy_uri) G_GNUC_DEPRECATED_FOR (gdata_oauth1_authorizer_set_proxy_resolver);
#endif /* !LIBGDATA_DISABLE_DEPRECATED */

GProxyResolver *gdata_oauth1_authorizer_get_proxy_resolver (GDataOAuth1Authorizer *self) G_GNUC_PURE;
void gdata_oauth1_authorizer_set_proxy_resolver (GDataOAuth1Authorizer *self, GProxyResolver *proxy_resolver);

guint gdata_oauth1_authorizer_get_timeout (GDataOAuth1Authorizer *self) G_GNUC_PURE;
void gdata_oauth1_authorizer_set_timeout (GDataOAuth1Authorizer *self, guint timeout);

G_END_DECLS

#endif /* !GDATA_OAUTH1_AUTHORIZER_H */
