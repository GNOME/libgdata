/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2013 <philip@tecnocode.co.uk>
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
 */

#ifndef GDATA_MOCK_RESOLVER_H
#define GDATA_MOCK_RESOLVER_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define GDATA_TYPE_MOCK_RESOLVER		(gdata_mock_resolver_get_type ())
#define GDATA_MOCK_RESOLVER(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_MOCK_RESOLVER, GDataMockResolver))
#define GDATA_MOCK_RESOLVER_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_MOCK_RESOLVER, GDataMockResolverClass))
#define GDATA_IS_MOCK_RESOLVER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_MOCK_RESOLVER))
#define GDATA_IS_MOCK_RESOLVER_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_MOCK_RESOLVER))
#define GDATA_MOCK_RESOLVER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_MOCK_RESOLVER, GDataMockResolverClass))

typedef struct _GDataMockResolverPrivate	GDataMockResolverPrivate;

typedef struct {
	GResolver parent;
	GDataMockResolverPrivate *priv;
} GDataMockResolver;

typedef struct {
	GResolverClass parent;
} GDataMockResolverClass;

GType gdata_mock_resolver_get_type (void) G_GNUC_CONST;

GDataMockResolver *gdata_mock_resolver_new (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

void gdata_mock_resolver_reset (GDataMockResolver *self);

gboolean gdata_mock_resolver_add_A (GDataMockResolver *self, const gchar *hostname, const gchar *addr);
gboolean gdata_mock_resolver_add_SRV (GDataMockResolver *self, const gchar *service, const gchar *protocol, const gchar *domain, const gchar *addr, guint16 port);

G_END_DECLS

#endif /* !GDATA_MOCK_RESOLVER_H */
