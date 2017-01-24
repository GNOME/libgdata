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

#ifndef GDATA_BUFFER_H
#define GDATA_BUFFER_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct _GDataBufferChunk GDataBufferChunk;

/**
 * GDataBuffer:
 *
 * All the fields in the #GDataBuffer structure are private and should never be accessed directly.
 *
 * Since: 0.5.0
 */
typedef struct {
	/*< private >*/
	GDataBufferChunk *head;
	gsize head_read_offset; /* number of bytes which have already been popped from the head chunk */
	gsize total_length; /* total length of all the chunks available to read (i.e. head_read_offset is already subtracted) */
	gboolean reached_eof; /* set to TRUE only once we've reached EOF */
	GDataBufferChunk **tail; /* pointer to the GDataBufferChunk->next field of the current tail chunk */

	GMutex mutex; /* mutex protecting the entire structure on push and pop */
	GCond cond; /* a GCond to allow a popping thread to block on data being pushed into the buffer */
} GDataBuffer;

GDataBuffer *gdata_buffer_new (void) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
void gdata_buffer_free (GDataBuffer *self);

gboolean gdata_buffer_push_data (GDataBuffer *self, const guint8 *data, gsize length);
gsize gdata_buffer_pop_data (GDataBuffer *self, guint8 *data, gsize length_requested, gboolean *reached_eof, GCancellable *cancellable);
gsize gdata_buffer_pop_data_limited (GDataBuffer *self, guint8 *data, gsize maximum_length, gboolean *reached_eof);

G_END_DECLS

#endif /* !GDATA_BUFFER_H */
