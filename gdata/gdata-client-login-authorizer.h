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

#ifndef GDATA_CLIENT_LOGIN_AUTHORIZER_H
#define GDATA_CLIENT_LOGIN_AUTHORIZER_H

#include <glib.h>
#include <glib-object.h>

#include "gdata-authorizer.h"

G_BEGIN_DECLS

/**
 * GDataClientLoginAuthorizerError:
 * @GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_BAD_AUTHENTICATION: The login request used a username or password that is not recognized.
 * @GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_NOT_VERIFIED: The account email address has not been verified. The user will need to access their Google
 * account directly to resolve the issue before logging in using a non-Google application.
 * @GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_TERMS_NOT_AGREED: The user has not agreed to terms. The user will need to access their Google account directly
 * to resolve the issue before logging in using a non-Google application.
 * @GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_CAPTCHA_REQUIRED: A CAPTCHA is required. (A response with this error code will also contain an image URI and a
 * CAPTCHA token.)
 * @GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_ACCOUNT_DELETED: The user account has been deleted.
 * @GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_ACCOUNT_DISABLED: The user account has been disabled.
 * @GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_SERVICE_DISABLED: The user's access to the specified service has been disabled. (The user account may still be
 * valid.)
 * @GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_ACCOUNT_MIGRATED: The user's account login details have been migrated to a new system. (This is used for the
 * transition from the old YouTube login details to the new ones.)
 * @GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_INVALID_SECOND_FACTOR: The user's account requires an application-specific password to be used.
 *
 * Error codes for authentication and authorization operations on #GDataClientLoginAuthorizer. See the
 * <ulink type="http" url="http://code.google.com/apis/accounts/docs/AuthForInstalledApps.html#Errors">online ClientLogin documentation</ulink> for
 * more information.
 *
 * Since: 0.9.0
 */
typedef enum {
	GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_BAD_AUTHENTICATION = 1,
	GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_NOT_VERIFIED,
	GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_TERMS_NOT_AGREED,
	GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_CAPTCHA_REQUIRED,
	GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_ACCOUNT_DELETED,
	GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_ACCOUNT_DISABLED,
	GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_SERVICE_DISABLED,
	GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_ACCOUNT_MIGRATED,
	GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_INVALID_SECOND_FACTOR
} GDataClientLoginAuthorizerError;

GQuark gdata_client_login_authorizer_error_quark (void) G_GNUC_CONST;

#define GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR		gdata_client_login_authorizer_error_quark ()

#define GDATA_TYPE_CLIENT_LOGIN_AUTHORIZER		(gdata_client_login_authorizer_get_type ())
#define GDATA_CLIENT_LOGIN_AUTHORIZER(o) \
	(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_CLIENT_LOGIN_AUTHORIZER, GDataClientLoginAuthorizer))
#define GDATA_CLIENT_LOGIN_AUTHORIZER_CLASS(k) \
	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_CLIENT_LOGIN_AUTHORIZER, GDataClientLoginAuthorizerClass))
#define GDATA_IS_CLIENT_LOGIN_AUTHORIZER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_CLIENT_LOGIN_AUTHORIZER))
#define GDATA_IS_CLIENT_LOGIN_AUTHORIZER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_CLIENT_LOGIN_AUTHORIZER))
#define GDATA_CLIENT_LOGIN_AUTHORIZER_GET_CLASS(o) \
	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_CLIENT_LOGIN_AUTHORIZER, GDataClientLoginAuthorizerClass))

typedef struct _GDataClientLoginAuthorizerPrivate	GDataClientLoginAuthorizerPrivate;

/**
 * GDataClientLoginAuthorizer:
 *
 * All the fields in the #GDataClientLoginAuthorizer structure are private and should never be accessed directly.
 *
 * Since: 0.9.0
 */
typedef struct {
	/*< private >*/
	GObject parent;
	GDataClientLoginAuthorizerPrivate *priv;
} GDataClientLoginAuthorizer;

/**
 * GDataClientLoginAuthorizerClass:
 *
 * All the fields in the #GDataClientLoginAuthorizerClass structure are private and should never be accessed directly.
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
} GDataClientLoginAuthorizerClass;

GType gdata_client_login_authorizer_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataClientLoginAuthorizer, g_object_unref)

GDataClientLoginAuthorizer *gdata_client_login_authorizer_new (const gchar *client_id, GType service_type) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
GDataClientLoginAuthorizer *gdata_client_login_authorizer_new_for_authorization_domains (const gchar *client_id, GList *authorization_domains)
                                                                                        G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

gboolean gdata_client_login_authorizer_authenticate (GDataClientLoginAuthorizer *self, const gchar *username, const gchar *password,
                                                     GCancellable *cancellable, GError **error);
void gdata_client_login_authorizer_authenticate_async (GDataClientLoginAuthorizer *self, const gchar *username, const gchar *password,
                                                       GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
gboolean gdata_client_login_authorizer_authenticate_finish (GDataClientLoginAuthorizer *self, GAsyncResult *async_result, GError **error);

const gchar *gdata_client_login_authorizer_get_client_id (GDataClientLoginAuthorizer *self) G_GNUC_PURE;
const gchar *gdata_client_login_authorizer_get_username (GDataClientLoginAuthorizer *self) G_GNUC_PURE;
const gchar *gdata_client_login_authorizer_get_password (GDataClientLoginAuthorizer *self) G_GNUC_PURE;

#ifndef LIBGDATA_DISABLE_DEPRECATED
SoupURI *gdata_client_login_authorizer_get_proxy_uri (GDataClientLoginAuthorizer *self) G_GNUC_PURE G_GNUC_DEPRECATED_FOR (gdata_client_login_authorizer_get_proxy_resolver);
void gdata_client_login_authorizer_set_proxy_uri (GDataClientLoginAuthorizer *self, SoupURI *proxy_uri) G_GNUC_DEPRECATED_FOR (gdata_client_login_authorizer_set_proxy_resolver);
#endif /* !LIBGDATA_DISABLE_DEPRECATED */

GProxyResolver *gdata_client_login_authorizer_get_proxy_resolver (GDataClientLoginAuthorizer *self) G_GNUC_PURE;
void gdata_client_login_authorizer_set_proxy_resolver (GDataClientLoginAuthorizer *self, GProxyResolver *proxy_resolver);

guint gdata_client_login_authorizer_get_timeout (GDataClientLoginAuthorizer *self) G_GNUC_PURE;
void gdata_client_login_authorizer_set_timeout (GDataClientLoginAuthorizer *self, guint timeout);

G_END_DECLS

#endif /* !GDATA_CLIENT_LOGIN_AUTHORIZER_H */
