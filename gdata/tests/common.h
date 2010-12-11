/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009 <philip@tecnocode.co.uk>
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

#include <glib.h>
#include <gdata/gdata.h>

#ifndef GDATA_TEST_COMMON_H
#define GDATA_TEST_COMMON_H

G_BEGIN_DECLS

#define CLIENT_ID "ytapi-GNOME-libgdata-444fubtt-0"
#define USERNAME "libgdata.test@gmail.com"
#define DOCUMENTS_USERNAME "libgdata.documents@gmail.com"
#define PASSWORD "gdata-libgdata"

gboolean gdata_test_compare_xml (GDataParsable *parsable, const gchar *expected_xml, gboolean print_error);

/* Convenience macro */
#define gdata_test_assert_xml(Parsable, XML) \
	G_STMT_START { \
		gboolean _test_success = gdata_test_compare_xml (GDATA_PARSABLE (Parsable), XML, TRUE); \
		g_assert (_test_success == TRUE); \
	} G_STMT_END

G_END_DECLS

#endif /* !GDATA_TEST_COMMON_H */
