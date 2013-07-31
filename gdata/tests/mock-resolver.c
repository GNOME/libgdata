/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2013 <philip@tecnocode.co.uk>
 * Copyright (C) Collabora Ltd. 2009
 * 
 * GData Client is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GData Client is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GData Client.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Original author: Vivek Dasmohapatra <vivek@collabora.co.uk>
 */

/**
 * GDataMockResolver:
 *
 * A mock DNS resolver which resolves according to specified host-name–IP-address pairs, and raises an error for all non-specified host name requests.
 * This allows network connections for expected services to be redirected to a different server, such as a local mock server on a loopback interface.
 *
 * This code is heavily based on code originally by Vivek Dasmohapatra, found here:
 * http://cgit.collabora.com/git/user/sjoerd/telepathy-gabble.git/plain/tests/twisted/test-resolver.c
 * It was originally licenced under LGPLv2.1+, and implicitly relicenced to LGPLv3+ on copying into libgdata.
 */

#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>

#ifdef G_OS_WIN32
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "mock-resolver.h"

static void gdata_mock_resolver_finalize (GObject *object);

static GList *gdata_mock_resolver_lookup_by_name (GResolver *resolver, const gchar *hostname, GCancellable *cancellable, GError **error);
static void gdata_mock_resolver_lookup_by_name_async (GResolver *resolver, const gchar *hostname, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
static GList *gdata_mock_resolver_lookup_by_name_finish (GResolver *resolver, GAsyncResult *result, GError **error);
static GList *gdata_mock_resolver_lookup_service (GResolver *resolver, const gchar *rrname, GCancellable *cancellable, GError **error);
static void gdata_mock_resolver_lookup_service_async (GResolver *resolver, const gchar *rrname, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
static GList *gdata_mock_resolver_lookup_service_finish (GResolver *resolver, GAsyncResult *result, GError **error);

typedef struct {
	gchar *key;
	gchar *addr;
} FakeHost;

typedef struct {
	char *key;
	GSrvTarget *srv;
} FakeService;

struct _GDataMockResolverPrivate {
	GList *fake_A;
	GList *fake_SRV;
};

G_DEFINE_TYPE (GDataMockResolver, gdata_mock_resolver, G_TYPE_RESOLVER)

static void
gdata_mock_resolver_class_init (GDataMockResolverClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GResolverClass *resolver_class = G_RESOLVER_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataMockResolverPrivate));

	gobject_class->finalize = gdata_mock_resolver_finalize;

	resolver_class->lookup_by_name = gdata_mock_resolver_lookup_by_name;
	resolver_class->lookup_by_name_async = gdata_mock_resolver_lookup_by_name_async;
	resolver_class->lookup_by_name_finish = gdata_mock_resolver_lookup_by_name_finish;
	resolver_class->lookup_service = gdata_mock_resolver_lookup_service;
	resolver_class->lookup_service_async = gdata_mock_resolver_lookup_service_async;
	resolver_class->lookup_service_finish = gdata_mock_resolver_lookup_service_finish;
}

static void
gdata_mock_resolver_init (GDataMockResolver *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_MOCK_RESOLVER, GDataMockResolverPrivate);
}

static void
gdata_mock_resolver_finalize (GObject *object)
{
	gdata_mock_resolver_reset (GDATA_MOCK_RESOLVER (object));

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_mock_resolver_parent_class)->finalize (object);
}

static gchar *
_service_rrname (const char *service, const char *protocol, const char *domain)
{
	gchar *rrname, *ascii_domain;

	ascii_domain = g_hostname_to_ascii (domain);
	rrname = g_strdup_printf ("_%s._%s.%s", service, protocol, ascii_domain);
	g_free (ascii_domain);

	return rrname;
}

static GList *
find_fake_services (GDataMockResolver *self, const char *name)
{
	GList *fake = NULL;
	GList *rval = NULL;

	for (fake = self->priv->fake_SRV; fake != NULL; fake = g_list_next (fake)) {
		FakeService *entry = fake->data;
		if (entry != NULL && !g_strcmp0 (entry->key, name)) {
			rval = g_list_append (rval, g_srv_target_copy (entry->srv));
		}
	}

	return rval;
}

static GList *
find_fake_hosts (GDataMockResolver *self, const char *name)
{
	GList *fake = NULL;
	GList *rval = NULL;

	for (fake = self->priv->fake_A; fake != NULL; fake = g_list_next (fake)) {
		FakeHost *entry = fake->data;
		if (entry != NULL && !g_strcmp0 (entry->key, name)) {
			rval = g_list_append (rval, g_inet_address_new_from_string (entry->addr));
		}
	}

	return rval;
}

static GList *
gdata_mock_resolver_lookup_by_name (GResolver *resolver, const gchar *hostname, GCancellable *cancellable, GError **error)
{
	GList *result;

	result = find_fake_hosts (GDATA_MOCK_RESOLVER (resolver), hostname);

	if (result == NULL) {
		g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND, "No fake hostname record registered for ‘%s’.", hostname);
	}

	return result;
}

static void
gdata_mock_resolver_lookup_by_name_async (GResolver *resolver, const gchar *hostname, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	GSimpleAsyncResult *res;
	GList *addr;
	GError *error = NULL;

	addr = gdata_mock_resolver_lookup_by_name (resolver, hostname, NULL, &error);
	res = g_simple_async_result_new (G_OBJECT (resolver), callback, user_data, NULL);

	if (addr != NULL) {
		g_simple_async_result_set_op_res_gpointer (res, addr, NULL);
	} else {
		g_simple_async_result_set_from_error (res, error);
		g_error_free (error);
	}

	g_simple_async_result_complete_in_idle (res);
	g_object_unref (res);
}

static GList *
gdata_mock_resolver_lookup_by_name_finish (GResolver *resolver, GAsyncResult *result, GError **error)
{
	GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);

	if (g_simple_async_result_propagate_error (simple, error)) {
		return NULL;
	}

	return g_simple_async_result_get_op_res_gpointer (simple);
}

static GList *
gdata_mock_resolver_lookup_service (GResolver *resolver, const gchar *rrname, GCancellable *cancellable, GError **error)
{
	GList *result;

	result = find_fake_services (GDATA_MOCK_RESOLVER (resolver), rrname);

	if (result == NULL) {
		g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND, "No fake service records registered for ‘%s’.", rrname);
	}

	return result;
}

static void
gdata_mock_resolver_lookup_service_async (GResolver *resolver, const gchar *rrname, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	GDataMockResolver *self = GDATA_MOCK_RESOLVER (resolver);
	GList *addr;
	GSimpleAsyncResult *res;

	addr = find_fake_services (self, rrname);
	res = g_simple_async_result_new (G_OBJECT (resolver), callback, user_data, gdata_mock_resolver_lookup_service_async);

	if (addr != NULL) {
		g_simple_async_result_set_op_res_gpointer (res, addr, NULL);
	} else {
		g_simple_async_result_set_error (res, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND, "No fake SRV record registered for ‘%s’.", rrname);
	}

	g_simple_async_result_complete_in_idle (res);
	g_object_unref (res);
}

static GList *
gdata_mock_resolver_lookup_service_finish (GResolver *resolver, GAsyncResult *result, GError **error)
{
	GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);

	if (g_simple_async_result_propagate_error (simple, error)) {
		return NULL;
	}

	return g_simple_async_result_get_op_res_gpointer (simple);
}

/**
 * gdata_mock_resolver_new:
 *
 * Creates a new #GDataMockResolver with default property values.
 *
 * Return value: (transfer full): a new #GDataMockResolver; unref with g_object_unref()
 */
GDataMockResolver *
gdata_mock_resolver_new (void)
{
	return g_object_new (GDATA_TYPE_MOCK_RESOLVER, NULL);
}

/**
 * gdata_mock_resolver_reset:
 * @self: a #GDataMockResolver
 *
 * Resets the state of the #GDataMockResolver, deleting all records added with gdata_mock_resolver_add_A() and gdata_mock_resolver_add_SRV().
 */
void
gdata_mock_resolver_reset (GDataMockResolver *self)
{
	GList *fake = NULL;

	for (fake = self->priv->fake_A; fake != NULL; fake = g_list_next (fake)) {
		FakeHost *entry = fake->data;
		g_free (entry->key);
		g_free (entry->addr);
		g_free (entry);
	}
	g_list_free (self->priv->fake_A);
	self->priv->fake_A = NULL;

	for (fake = self->priv->fake_SRV; fake != NULL; fake = g_list_next (fake)) {
		FakeService *entry = fake->data;
		g_free (entry->key);
		g_srv_target_free (entry->srv);
		g_free (entry);
	}
	g_list_free (self->priv->fake_SRV);
	self->priv->fake_SRV = NULL;
}

/**
 * gdata_mock_resolver_add_A:
 * @self: a #GDataMockResolver
 * @hostname: the hostname to match
 * @addr: the IP address to resolve to
 *
 * Adds a resolution mapping from the host name @hostname to the IP address @addr.
 *
 * Return value: %TRUE on success; %FALSE otherwise
 */
gboolean
gdata_mock_resolver_add_A (GDataMockResolver *self, const gchar *hostname, const gchar *addr)
{
	FakeHost *entry = g_new0 (FakeHost, 1);
	entry->key = g_strdup (hostname);
	entry->addr = g_strdup (addr);
	self->priv->fake_A = g_list_append (self->priv->fake_A, entry);

	return TRUE;
}

/**
 * gdata_mock_resolver_add_SRV:
 * @self: a #GDataMockResolver
 * @hostname: the service name to match
 * @protocol: the protocol name to match
 * @domain: the domain name to match
 * @addr: the IP address to resolve to
 * @port: the port to resolve to
 *
 * Adds a resolution mapping the given @service (on @protocol and @domain) to the IP address @addr and given @port.
 *
 * Return value: %TRUE on success; %FALSE otherwise
 */
gboolean
gdata_mock_resolver_add_SRV (GDataMockResolver *self, const gchar *service, const gchar *protocol, const gchar *domain, const gchar *addr, guint16 port)
{
	gchar *key;
	GSrvTarget *serv;
	FakeService *entry;

	key = _service_rrname (service, protocol, domain);
	entry = g_new0 (FakeService, 1);
	serv = g_srv_target_new (addr, port, 0, 0);
	entry->key = key;
	entry->srv = serv;
	self->priv->fake_SRV = g_list_append (self->priv->fake_SRV, entry);

	return TRUE;
}
