/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2011, 2014, 2015 <philip@tecnocode.co.uk>
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

#ifndef GDATA_OAUTH2_AUTHORIZER_H
#define GDATA_OAUTH2_AUTHORIZER_H

#include <glib.h>
#include <glib-object.h>

#include "gdata-authorizer.h"

G_BEGIN_DECLS

/**
 * GDATA_OAUTH2_REDIRECT_URI_OOB:
 *
 * OAuth 2 redirect URI for out-of-band authorisation code transfer, where the
 * user is shown the authorisation code and asked to copy it.
 *
 * See
 * <ulink type="http" url="https://developers.google.com/accounts/docs/OAuth2InstalledApp#choosingredirecturi">reference
 * documentation</ulink> for details.
 *
 * Since: 0.17.0
 */
#define GDATA_OAUTH2_REDIRECT_URI_OOB "urn:ietf:wg:oauth:2.0:oob"

/**
 * GDATA_OAUTH2_REDIRECT_URI_OOB_AUTO:
 *
 * OAuth 2 redirect URI for out-of-band authorisation code transfer, where the
 * user is not shown the authorisation code or asked to copy it.
 *
 * See
 * <ulink type="http" url="https://developers.google.com/accounts/docs/OAuth2InstalledApp#choosingredirecturi">reference
 * documentation</ulink> for details.
 *
 * Since: 0.17.0
 */
#define GDATA_OAUTH2_REDIRECT_URI_OOB_AUTO "urn:ietf:wg:oauth:2.0:oob:auto"

#define GDATA_TYPE_OAUTH2_AUTHORIZER		(gdata_oauth2_authorizer_get_type ())
#define GDATA_OAUTH2_AUTHORIZER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_OAUTH2_AUTHORIZER, GDataOAuth2Authorizer))
#define GDATA_OAUTH2_AUTHORIZER_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_OAUTH2_AUTHORIZER, GDataOAuth2AuthorizerClass))
#define GDATA_IS_OAUTH2_AUTHORIZER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_OAUTH2_AUTHORIZER))
#define GDATA_IS_OAUTH2_AUTHORIZER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_OAUTH2_AUTHORIZER))
#define GDATA_OAUTH2_AUTHORIZER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_OAUTH2_AUTHORIZER, GDataOAuth2AuthorizerClass))

typedef struct _GDataOAuth2AuthorizerPrivate	GDataOAuth2AuthorizerPrivate;

/**
 * GDataOAuth2Authorizer:
 *
 * All the fields in the #GDataOAuth2Authorizer structure are private and should never be accessed directly.
 *
 * Since: 0.17.0
 */
typedef struct {
	/*< private >*/
	GObject parent;
	GDataOAuth2AuthorizerPrivate *priv;
} GDataOAuth2Authorizer;

/**
 * GDataOAuth2AuthorizerClass:
 *
 * All the fields in the #GDataOAuth2AuthorizerClass structure are private and should never be accessed directly.
 *
 * Since: 0.17.0
 */
typedef struct {
	/*< private >*/
	GObjectClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataOAuth2AuthorizerClass;

GType gdata_oauth2_authorizer_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataOAuth2Authorizer, g_object_unref)

GDataOAuth2Authorizer *gdata_oauth2_authorizer_new (const gchar *client_id,
                                                    const gchar *client_secret,
                                                    const gchar *redirect_uri,
                                                    GType service_type) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
GDataOAuth2Authorizer *gdata_oauth2_authorizer_new_for_authorization_domains (const gchar *client_id,
                                                                              const gchar *client_secret,
                                                                              const gchar *redirect_uri,
                                                                              GList *authorization_domains) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

gchar *gdata_oauth2_authorizer_build_authentication_uri (GDataOAuth2Authorizer *self,
                                                         const gchar *login_hint,
                                                         gboolean include_granted_scopes) G_GNUC_PURE;

gboolean gdata_oauth2_authorizer_request_authorization (GDataOAuth2Authorizer *self, const gchar *authorization_code,
                                                        GCancellable *cancellable, GError **error);
void gdata_oauth2_authorizer_request_authorization_async (GDataOAuth2Authorizer *self, const gchar *authorization_code,
                                                          GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
gboolean gdata_oauth2_authorizer_request_authorization_finish (GDataOAuth2Authorizer *self, GAsyncResult *async_result, GError **error);

const gchar *gdata_oauth2_authorizer_get_client_id (GDataOAuth2Authorizer *self) G_GNUC_PURE;
const gchar *gdata_oauth2_authorizer_get_redirect_uri (GDataOAuth2Authorizer *self) G_GNUC_PURE;
const gchar *gdata_oauth2_authorizer_get_client_secret (GDataOAuth2Authorizer *self) G_GNUC_PURE;

gchar *gdata_oauth2_authorizer_dup_refresh_token (GDataOAuth2Authorizer *self) G_GNUC_PURE;
void gdata_oauth2_authorizer_set_refresh_token (GDataOAuth2Authorizer *self, const gchar *refresh_token);

const gchar *gdata_oauth2_authorizer_get_locale (GDataOAuth2Authorizer *self) G_GNUC_PURE;
void gdata_oauth2_authorizer_set_locale (GDataOAuth2Authorizer *self, const gchar *locale);

guint gdata_oauth2_authorizer_get_timeout (GDataOAuth2Authorizer *self) G_GNUC_PURE;
void gdata_oauth2_authorizer_set_timeout (GDataOAuth2Authorizer *self, guint timeout);

GProxyResolver *gdata_oauth2_authorizer_get_proxy_resolver (GDataOAuth2Authorizer *self) G_GNUC_PURE;
void gdata_oauth2_authorizer_set_proxy_resolver (GDataOAuth2Authorizer *self, GProxyResolver *proxy_resolver);

G_END_DECLS

#endif /* !GDATA_OAUTH2_AUTHORIZER_H */
