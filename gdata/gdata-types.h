/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2008-2009 <philip@tecnocode.co.uk>
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

#ifndef GDATA_TYPES_H
#define GDATA_TYPES_H

G_BEGIN_DECLS

/**
 * GDataColor:
 * @red: red color intensity, from 0–255
 * @green: green color intensity, from 0–255
 * @blue: blue color intensity, from 0–255
 *
 * Describes a color, such as used in the Google Calendar interface to
 * differentiate calendars.
 */
typedef struct {
	/*< public >*/
	guint16 red;
	guint16 green;
	guint16 blue;
} GDataColor;

#define GDATA_TYPE_COLOR (gdata_color_get_type ())
GType gdata_color_get_type (void) G_GNUC_CONST;
gboolean gdata_color_from_hexadecimal (const gchar *hexadecimal, GDataColor *color);
gchar *gdata_color_to_hexadecimal (const GDataColor *color) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_TYPES_H */
