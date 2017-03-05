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

#ifndef GDATA_FREEBASE_SEARCH_RESULT_H
#define GDATA_FREEBASE_SEARCH_RESULT_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-types.h>
#include "gdata-freebase-result.h"

G_BEGIN_DECLS

#ifndef LIBGDATA_DISABLE_DEPRECATED

#define GDATA_TYPE_FREEBASE_SEARCH_RESULT_ITEM		(gdata_freebase_search_result_item_get_type ())
#define GDATA_TYPE_FREEBASE_SEARCH_RESULT		(gdata_freebase_search_result_get_type ())
#define GDATA_FREEBASE_SEARCH_RESULT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_FREEBASE_SEARCH_RESULT, GDataFreebaseSearchResult))
#define GDATA_FREEBASE_SEARCH_RESULT_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_FREEBASE_SEARCH_RESULT, GDataFreebaseSearchResultClass))
#define GDATA_IS_FREEBASE_SEARCH_RESULT(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_FREEBASE_SEARCH_RESULT))
#define GDATA_IS_FREEBASE_SEARCH_RESULT_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_FREEBASE_SEARCH_RESULT))
#define GDATA_FREEBASE_SEARCH_RESULT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_FREEBASE_SEARCH_RESULT, GDataFreebaseSearchResultClass))

typedef struct _GDataFreebaseSearchResultPrivate GDataFreebaseSearchResultPrivate;

/**
 * GDataFreebaseSearchResultItem:
 *
 * Opaque struct holding data for a single search result item.
 *
 * Since: 0.15.1
 */
typedef struct _GDataFreebaseSearchResultItem GDataFreebaseSearchResultItem;

/**
 * GDataFreebaseSearchResult:
 *
 * All the fields in the #GDataFreebaseSearchResult structure are private and should never be accessed directly.
 *
 * Since: 0.15.1
 */
typedef struct {
	GDataFreebaseResult parent;
	GDataFreebaseSearchResultPrivate *priv;
} GDataFreebaseSearchResult;

/**
 * GDataFreebaseSearchResultClass:
 *
 * All the fields in the #GDataFreebaseSearchResultClass structure are private and should never be accessed directly.
 *
 * Since: 0.15.1
 */

typedef struct {
	/*< private >*/
	GDataFreebaseResultClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataFreebaseSearchResultClass;

GType gdata_freebase_search_result_item_get_type (void) G_GNUC_CONST G_GNUC_DEPRECATED;
GType gdata_freebase_search_result_get_type (void) G_GNUC_CONST G_GNUC_DEPRECATED;

GDataFreebaseSearchResult *gdata_freebase_search_result_new (void) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC G_GNUC_DEPRECATED;

guint gdata_freebase_search_result_get_num_items (GDataFreebaseSearchResult *self) G_GNUC_DEPRECATED;
guint gdata_freebase_search_result_get_total_hits (GDataFreebaseSearchResult *self) G_GNUC_DEPRECATED;

const GDataFreebaseSearchResultItem *gdata_freebase_search_result_get_item (GDataFreebaseSearchResult *self, guint i) G_GNUC_DEPRECATED;

const gchar *gdata_freebase_search_result_item_get_mid (const GDataFreebaseSearchResultItem *item) G_GNUC_DEPRECATED;
const gchar *gdata_freebase_search_result_item_get_id (const GDataFreebaseSearchResultItem *item) G_GNUC_DEPRECATED;
const gchar *gdata_freebase_search_result_item_get_name (const GDataFreebaseSearchResultItem *item) G_GNUC_DEPRECATED;
const gchar *gdata_freebase_search_result_item_get_language (const GDataFreebaseSearchResultItem *item) G_GNUC_DEPRECATED;
const gchar *gdata_freebase_search_result_item_get_notable_id (const GDataFreebaseSearchResultItem *item) G_GNUC_DEPRECATED;
const gchar *gdata_freebase_search_result_item_get_notable_name (const GDataFreebaseSearchResultItem *item) G_GNUC_DEPRECATED;
gdouble gdata_freebase_search_result_item_get_score (const GDataFreebaseSearchResultItem *item) G_GNUC_DEPRECATED;

#endif /* !LIBGDATA_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* !GDATA_FREEBASE_SEARCH_RESULT_H */
