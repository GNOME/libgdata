/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009, 2015 <philip@tecnocode.co.uk>
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

#ifndef GDATA_ACCESS_HANDLER_H
#define GDATA_ACCESS_HANDLER_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-feed.h>
#include <gdata/gdata-service.h>
#include <gdata/gdata-access-rule.h>
#include <gdata/gdata-authorization-domain.h>

G_BEGIN_DECLS

/**
 * GDATA_LINK_ACCESS_CONTROL_LIST:
 *
 * The relation type URI of the access control list location for this resource.
 *
 * For more information, see the
 * <ulink type="http" url="http://code.google.com/apis/calendar/data/2.0/developers_guide_protocol.html#SharingACalendar">ACL specification</ulink>.
 *
 * Since: 0.7.0
 */
#define GDATA_LINK_ACCESS_CONTROL_LIST "http://schemas.google.com/acl/2007#accessControlList"

#define GDATA_TYPE_ACCESS_HANDLER		(gdata_access_handler_get_type ())
#define GDATA_ACCESS_HANDLER(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_ACCESS_HANDLER, GDataAccessHandler))
#define GDATA_ACCESS_HANDLER_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_ACCESS_HANDLER, GDataAccessHandlerIface))
#define GDATA_IS_ACCESS_HANDLER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_ACCESS_HANDLER))
#define GDATA_ACCESS_HANDLER_GET_IFACE(o)	(G_TYPE_INSTANCE_GET_INTERFACE ((o), GDATA_TYPE_ACCESS_HANDLER, GDataAccessHandlerIface))

/**
 * GDataAccessHandler:
 *
 * All the fields in the #GDataAccessHandler structure are private and should never be accessed directly.
 *
 * Since: 0.3.0
 */
typedef struct _GDataAccessHandler		GDataAccessHandler; /* dummy typedef */

/**
 * GDataAccessHandlerIface:
 * @parent: the parent type
 * @is_owner_rule: a function to return whether the given #GDataAccessRule has the role of an owner (of a #GDataAccessHandler).
 * @get_authorization_domain: (allow-none): a function to return the #GDataAuthorizationDomain to be used for all operations on the access rules
 * belonging to this access handler; not implementing this function is equivalent to returning %NULL from it, which signifies that operations on the
 * access rules don't require authorization; new in version 0.9.0
 * @get_rules: (nullable): a function to query, parse and return a #GDataFeed of
 *   #GDataAccessRules for a given entry — the virtual function for
 *   gdata_access_handler_get_rules(); new in version 0.17.2
 *
 * The class structure for the #GDataAccessHandler interface.
 *
 * Since: 0.9.0
 */
typedef struct {
	GTypeInterface parent;

	gboolean (*is_owner_rule) (GDataAccessRule *rule);
	GDataAuthorizationDomain *(*get_authorization_domain) (GDataAccessHandler *self);

	GDataFeed *(*get_rules) (GDataAccessHandler *self,
	                         GDataService *service,
	                         GCancellable *cancellable,
	                         GDataQueryProgressCallback progress_callback,
	                         gpointer progress_user_data,
	                         GError **error);

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved1) (void);
	void (*_g_reserved2) (void);
	void (*_g_reserved3) (void);
	void (*_g_reserved4) (void);
	void (*_g_reserved5) (void);
} GDataAccessHandlerIface;

GType gdata_access_handler_get_type (void) G_GNUC_CONST;

GDataFeed *gdata_access_handler_get_rules (GDataAccessHandler *self, GDataService *service, GCancellable *cancellable,
                                           GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                           GError **error) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_access_handler_get_rules_async (GDataAccessHandler *self, GDataService *service, GCancellable *cancellable,
                                           GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                           GDestroyNotify destroy_progress_user_data,
                                           GAsyncReadyCallback callback, gpointer user_data);

G_END_DECLS

#endif /* !GDATA_ACCESS_HANDLER_H */
