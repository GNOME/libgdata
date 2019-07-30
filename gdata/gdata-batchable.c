/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2010 <philip@tecnocode.co.uk>
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

/**
 * SECTION:gdata-batchable
 * @short_description: GData batch service interface
 * @stability: Stable
 * @include: gdata/gdata-batchable.h
 *
 * #GDataBatchable is an interface which can be implemented by #GDataServices which support batch operations on their entries. It allows the
 * creation of a #GDataBatchOperation for the service, which allows a set of batch operations to be run.
 *
 * Since: 0.7.0
 */

#include <config.h>
#include <glib.h>

#include "gdata-batchable.h"
#include "gdata-service.h"
#include "gdata-batch-operation.h"

GType
gdata_batchable_get_type (void)
{
	static GType batchable_type = 0;

	if (!batchable_type) {
		batchable_type = g_type_register_static_simple (G_TYPE_INTERFACE, "GDataBatchable",
		                                                sizeof (GDataBatchableIface),
		                                                NULL, 0, NULL, 0);
		g_type_interface_add_prerequisite (batchable_type, GDATA_TYPE_SERVICE);
	}

	return batchable_type;
}

/**
 * gdata_batchable_create_operation:
 * @self: a #GDataBatchable
 * @domain: (allow-none): the #GDataAuthorizationDomain to authorize the operation, or %NULL
 * @feed_uri: the URI to send the batch operation request to
 *
 * Creates a new #GDataBatchOperation for the given #GDataBatchable service, and with the given @feed_uri. @feed_uri is normally the %GDATA_LINK_BATCH
 * link URI in the appropriate #GDataFeed from the service. If authorization will be required to perform any of the requests in the batch operation,
 * @domain must be non-%NULL, and must be an authorization domain which covers all of the requests. Otherwise, @domain may be %NULL if authorization
 * is not required.
 *
 * Return value: (transfer full): a new #GDataBatchOperation; unref with g_object_unref()
 *
 * Since: 0.9.0
 */
GDataBatchOperation *
gdata_batchable_create_operation (GDataBatchable *self, GDataAuthorizationDomain *domain, const gchar *feed_uri)
{
	g_return_val_if_fail (GDATA_IS_BATCHABLE (self), NULL);
	g_return_val_if_fail (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain), NULL);
	g_return_val_if_fail (feed_uri != NULL, NULL);

	return g_object_new (GDATA_TYPE_BATCH_OPERATION,
	                     "service", self,
	                     "authorization-domain", domain,
	                     "feed-uri", feed_uri,
	                     NULL);
}
