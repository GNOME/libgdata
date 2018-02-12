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

#ifndef GDATA_AUTHORIZER_H
#define GDATA_AUTHORIZER_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <libsoup/soup.h>

#include <gdata/gdata-authorization-domain.h>

G_BEGIN_DECLS

#define GDATA_TYPE_AUTHORIZER		(gdata_authorizer_get_type ())
#define GDATA_AUTHORIZER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_AUTHORIZER, GDataAuthorizer))
#define GDATA_AUTHORIZER_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_AUTHORIZER, GDataAuthorizerInterface))
#define GDATA_IS_AUTHORIZER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_AUTHORIZER))
#define GDATA_AUTHORIZER_GET_IFACE(o)	(G_TYPE_INSTANCE_GET_INTERFACE ((o), GDATA_TYPE_AUTHORIZER, GDataAuthorizerInterface))

/**
 * GDataAuthorizer:
 *
 * All the fields in the #GDataAuthorizer structure are private and should never be accessed directly.
 *
 * Since: 0.9.0
 */
typedef struct _GDataAuthorizer		GDataAuthorizer; /* dummy typedef */

/**
 * GDataAuthorizerInterface:
 * @parent: the parent type
 * @process_request: a function to append authorization headers to queries before they are submitted to the online service under the given
 * authorization domain (which may be %NULL); this must be implemented and must be thread safe, and must also handle being called multiple times on
 * the same #SoupMessage instance (so must be careful to replace headers rather than append them, for example)
 * @is_authorized_for_domain: a function to check whether the authorizer is authorized against the given domain; this must be implemented and must
 * be thread safe
 * @refresh_authorization: (allow-none): a function to force a refresh of any authorization tokens the authorizer holds, returning %TRUE if a refresh
 * was attempted and was successful, or %FALSE if a refresh wasn't attempted or was unsuccessful; if this isn't implemented it's assumed %FALSE
 * would've been returned, if it is implemented it must be thread safe
 * @refresh_authorization_async: (allow-none): an asynchronous version of @refresh_authorization; if this isn't implemented and @refresh_authorization
 * is, @refresh_authorization will be called in a thread to simulate this function, whereas if this is implemented @refresh_authorization_finish must
 * also be implemented and both functions must be thread safe
 * @refresh_authorization_finish: (allow-none): a finish function for the asynchronous version of @refresh_authorization; this must be implemented
 * exactly if @refresh_authorization_async is implemented, and must be thread safe if it is implemented
 *
 * The class structure for the #GDataAuthorizer interface.
 *
 * Since: 0.9.0
 */
typedef struct {
	GTypeInterface parent;

	void (*process_request) (GDataAuthorizer *self, GDataAuthorizationDomain *domain, SoupMessage *message);
	gboolean (*is_authorized_for_domain) (GDataAuthorizer *self, GDataAuthorizationDomain *domain);
	gboolean (*refresh_authorization) (GDataAuthorizer *self, GCancellable *cancellable, GError **error);
	void (*refresh_authorization_async) (GDataAuthorizer *self, GCancellable *cancellable,
	                                     GAsyncReadyCallback callback, gpointer user_data);
	gboolean (*refresh_authorization_finish) (GDataAuthorizer *self, GAsyncResult *async_result, GError **error);
} GDataAuthorizerInterface;

GType gdata_authorizer_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataAuthorizer, g_object_unref)

void gdata_authorizer_process_request (GDataAuthorizer *self, GDataAuthorizationDomain *domain, SoupMessage *message);
gboolean gdata_authorizer_is_authorized_for_domain (GDataAuthorizer *self, GDataAuthorizationDomain *domain);

gboolean gdata_authorizer_refresh_authorization (GDataAuthorizer *self, GCancellable *cancellable, GError **error);
void gdata_authorizer_refresh_authorization_async (GDataAuthorizer *self, GCancellable *cancellable,
                                                   GAsyncReadyCallback callback, gpointer user_data);
gboolean gdata_authorizer_refresh_authorization_finish (GDataAuthorizer *self, GAsyncResult *async_result, GError **error);

G_END_DECLS

#endif /* !GDATA_AUTHORIZER_H */
