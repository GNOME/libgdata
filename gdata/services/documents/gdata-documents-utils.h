/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Red Hat, Inc. 2015
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

#ifndef GDATA_DOCUMENTS_UTILS_H
#define GDATA_DOCUMENTS_UTILS_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

G_GNUC_INTERNAL GType gdata_documents_utils_get_type_from_content_type (const gchar *content_type);

G_END_DECLS

#endif /* !GDATA_DOCUMENTS_UTILS_H */
