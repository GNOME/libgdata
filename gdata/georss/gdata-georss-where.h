/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Richard Schwarting 2009 <aquarichy@gmail.com>
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

#ifndef GDATA_GEORSS_WHERE_H
#define GDATA_GEORSS_WHERE_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

#define GDATA_TYPE_GEORSS_WHERE		(gdata_georss_where_get_type ())
#define GDATA_GEORSS_WHERE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_GEORSS_WHERE, GDataGeoRSSWhere))
#define GDATA_GEORSS_WHERE_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_GEORSS_WHERE, GDataGeoRSSWhereClass))
#define GDATA_IS_GEORSS_WHERE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_GEORSS_WHERE))
#define GDATA_IS_GEORSS_WHERE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_GEORSS_WHERE))
#define GDATA_GEORSS_WHERE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_GEORSS_WHERE, GDataGeoRSSWhereClass))

typedef struct _GDataGeoRSSWherePrivate	GDataGeoRSSWherePrivate;

/**
 * GDataGeoRSSWhere:
 *
 * All the fields in the #GDataGeoRSSWhere structure are private and should never be accessed directly.
 *
 * Since: 0.5.0
 */
typedef struct {
	GDataParsable parent;
	GDataGeoRSSWherePrivate *priv;
} GDataGeoRSSWhere;

/**
 * GDataGeoRSSWhereClass:
 *
 * All the fields in the #GDataGeoRSSWhereClass structure are private and should never be accessed directly.
 *
 * Since: 0.5.0
 */
typedef struct {
	/*< private >*/
	GDataParsableClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataGeoRSSWhereClass;

GType gdata_georss_where_get_type (void) G_GNUC_CONST;

gdouble gdata_georss_where_get_latitude (GDataGeoRSSWhere *self) G_GNUC_PURE;
gdouble gdata_georss_where_get_longitude (GDataGeoRSSWhere *self) G_GNUC_PURE;
void gdata_georss_where_set_latitude (GDataGeoRSSWhere *self, gdouble latitude);
void gdata_georss_where_set_longitude (GDataGeoRSSWhere *self, gdouble longitude);

G_END_DECLS

#endif /* !GDATA_GEORSS_WHERE_H */
