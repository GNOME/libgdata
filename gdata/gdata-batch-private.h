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

#ifndef GDATA_BATCH_PRIVATE_H
#define GDATA_BATCH_PRIVATE_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct {
	guint id;
	GDataBatchOperationType type;
	GDataBatchOperationCallback callback;
	gpointer user_data;
	gchar *query_id; /* only used for queries */
	GType entry_type; /* only used for queries */
	GError *error;
	GDataEntry *entry; /* used for anything except queries, and to store the results of all operations */
} BatchOperation;

G_GNUC_INTERNAL BatchOperation *_gdata_batch_operation_get_operation (GDataBatchOperation *self, guint id) G_GNUC_PURE;
G_GNUC_INTERNAL void _gdata_batch_operation_run_callback (GDataBatchOperation *self, BatchOperation *op, GDataEntry *entry, GError *error);

G_END_DECLS

#endif /* !GDATA_BATCH_PRIVATE_H */
