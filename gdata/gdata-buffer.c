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

/**
 * SECTION:gdata-buffer
 * @short_description: GData buffer to allow threadsafe buffering
 * @stability: Unstable
 * @include: gdata/gdata-buffer.h
 *
 * #GDataBuffer is a simple object which allows threadsafe buffering of data meaning, for example, data can be received from
 * the network in a "push" fashion, buffered, then sent out to an output stream in a "pull" fashion.
 **/

#include <config.h>
#include <glib.h>
#include <string.h>

#include "gdata-buffer.h"

struct _GDataBufferChunk {
	/*< private >*/
	guint8 *data;
	gsize length;
	GDataBufferChunk *next;
	/* Note: the data is actually allocated in the same memory block, so it's inside this comment right now.
	 * We simply set chunk->data to point to chunk + sizeof (GDataBufferChunk). */
};

/**
 * gdata_buffer_new:
 *
 * Creates a new empty #GDataBuffer.
 *
 * Return value: a new #GDataBuffer; free with gdata_buffer_free()
 *
 * Since: 0.5.0
 **/
GDataBuffer *
gdata_buffer_new (void)
{
	GDataBuffer *buffer = g_slice_new0 (GDataBuffer);
	g_static_mutex_init (&(buffer->mutex));
	buffer->cond = g_cond_new ();

	return buffer;
}

/**
 * gdata_buffer_free:
 *
 * Frees a #GDataBuffer. The function isn't threadsafe, so should only be called once
 * use of the buffer has been reduced to only one thread (the reading thread, after
 * the EOF has been reached).
 *
 * Since: 0.5.0
 **/
void
gdata_buffer_free (GDataBuffer *self)
{
	GDataBufferChunk *chunk, *next_chunk;

	for (chunk = self->head; chunk != NULL; chunk = next_chunk) {
		next_chunk = chunk->next;
		g_free (chunk);
	}

	g_cond_free (self->cond);
	g_static_mutex_free (&(self->mutex));
	g_slice_free (GDataBuffer, self);
}

/**
 * gdata_buffer_push_data:
 * @self: a #GDataBuffer
 * @data: the data to push onto the buffer
 * @length: the length of @data
 *
 * Pushes @length bytes of @data onto the buffer, taking a copy of the data. If @data is %NULL and @length is %0,
 * the buffer will be marked as having reached the EOF, and subsequent calls to gdata_buffer_push_data()
 * will fail and return %FALSE.
 *
 * Assuming the buffer hasn't reached EOF, this operation is guaranteed to succeed (unless memory allocation fails).
 *
 * This function holds the lock on the #GDataBuffer, and signals any waiting calls to gdata_buffer_pop_data() once
 * the new data has been pushed onto the buffer. This function is threadsafe.
 *
 * Return value: %TRUE on success, %FALSE otherwise
 *
 * Since: 0.5.0
 **/
gboolean
gdata_buffer_push_data (GDataBuffer *self, const guint8 *data, gsize length)
{
	GDataBufferChunk *chunk;

	g_static_mutex_lock (&(self->mutex));

	if (G_UNLIKELY (self->reached_eof == TRUE)) {
		/* If we're marked as having reached EOF, don't accept any more data */
		g_static_mutex_unlock (&(self->mutex));
		return FALSE;
	} else if (G_UNLIKELY (data == NULL && length == 0)) {
		/* If @data is NULL and @length is 0, mark the buffer as having reached EOF,
		 * and signal any waiting threads. */
		self->reached_eof = TRUE;
		g_cond_signal (self->cond);
		g_static_mutex_unlock (&(self->mutex));
		return FALSE;
	}

	/* Create the chunk */
	chunk = g_malloc (sizeof (GDataBufferChunk) + length);
	chunk->data = (gpointer) chunk + (gsize) sizeof (GDataBufferChunk);
	chunk->length = length;
	chunk->next = NULL;

	/* Copy the data to the chunk */
	memcpy (chunk->data, data, length);

	/* Add it to the buffer's tail */
	if (self->tail != NULL)
		*(self->tail) = chunk;
	else
		self->head = chunk;
	self->tail = &(chunk->next);
	self->total_length += length;

	/* Signal any threads waiting to pop that data is available */
	g_cond_signal (self->cond);

	g_static_mutex_unlock (&(self->mutex));

	return TRUE;
}

typedef struct {
	GDataBuffer *buffer;
	gboolean *cancelled;
} CancelledData;

static void
pop_cancelled_cb (GCancellable *cancellable, CancelledData *data)
{
	/* Signal the pop_data function that it should stop blocking and cancel */
	*(data->cancelled) = TRUE;
	g_cond_signal (data->buffer->cond);
}

/**
 * gdata_buffer_pop_data:
 * @self: a #GDataBuffer
 * @data: return location for the popped data
 * @length_requested: the number of bytes of data requested
 * @cancellable: a #GCancellable, or %NULL
 *
 * Pops up to @length_requested bytes off the head of the buffer and copies them to @data, which must be allocated by
 * the caller and have enough space to store at most @length_requested bytes of output.
 *
 * If the buffer contains enough data to satisfy @length_requested, this function returns immediately.
 * Otherwise, this function blocks until data is pushed onto the head of the buffer with gdata_buffer_pop_data(). If
 * the buffer is marked as having reached the EOF, this function will not block, and will instead return the
 * remaining data in the buffer.
 *
 * This function holds the lock on the #GDataBuffer, and will automatically be signalled of new data pushed onto the
 * buffer if it's blocking.
 *
 * If @cancellable is provided, calling g_cancellable_cancel() on it from another thread will cause the call to
 * gdata_buffer_pop_data() to return immediately with whatever data it can find.
 *
 * Return value: the number of bytes returned in @data
 *
 * Since: 0.5.0
 **/
gsize
gdata_buffer_pop_data (GDataBuffer *self, guint8 *data, gsize length_requested, GCancellable *cancellable)
{
	GDataBufferChunk *chunk;
	gsize return_length = 0, length_remaining;

	/* In the case:
	 *  - length_requested < amount available: return length_requested
	 *  - length_requested > amount available: block until more is available, return length_requested
	 *  - length_requested > amount available and we've reached EOF: don't block, return all remaining data
	 *  - length_requested is a whole number of chunks: remove those chunks, return length_requested
	 *  - length_requested is less than one chunk: remove no chunks, return length_requested, set head_read_offset
	 *  - length_requested is a fraction of multiple chunks: remove whole chunks, return length_requested, set head_read_offset for remaining fraction
	 */

	g_static_mutex_lock (&(self->mutex));

	if (self->reached_eof == TRUE && length_requested > self->total_length) {
		/* Return data up to the EOF */
		return_length = self->total_length;
	} else if (length_requested > self->total_length) {
		gulong cancelled_signal = 0;
		gboolean cancelled = FALSE;

		/* Set up a handler so we can stop if we're cancelled */
		if (cancellable != NULL) {
			CancelledData cancelled_data;

			cancelled_data.buffer = self;
			cancelled_data.cancelled = &cancelled;

			cancelled_signal = g_signal_connect (cancellable, "cancelled", (GCallback) pop_cancelled_cb, &cancelled_data);
		}

		/* Block until more data is available */
		while (length_requested > self->total_length) {
			g_cond_wait (self->cond, g_static_mutex_get_mutex (&(self->mutex)));

			/* If the g_cond_wait() returned because it was signalled from the GCancellable callback (rather than from
			 * data being pushed into the buffer), stop blocking for data and make do with what we have so far. */
			if (cancelled == TRUE || self->reached_eof == TRUE) {
				return_length = MIN (length_requested, self->total_length);
				break;
			} else {
				return_length = length_requested;
			}
		}

		/* Disconnect from the cancelled signal */
		if (cancellable != NULL)
			g_signal_handler_disconnect (cancellable, cancelled_signal);
	} else {
		return_length = length_requested;
	}

	/* Return if we haven't got any data to pop (i.e. if we were cancelled before even one chunk arrived) */
	if (return_length == 0)
		return 0;

	/* Otherwise, get on with things */
	length_remaining = return_length;

	/* We can't assume we'll have enough data, since we may have reached EOF */
	chunk = self->head;
	while (chunk != NULL && length_remaining >= chunk->length) {
		GDataBufferChunk *next_chunk;
		gsize chunk_length = chunk->length - self->head_read_offset;

		/* Copy the data to the output */
		length_remaining -= chunk_length;
		memcpy (data, chunk->data + self->head_read_offset, chunk_length);
		data += chunk_length;

		/* Free the chunk and move on */
		next_chunk = chunk->next;
		g_free (chunk);
		chunk = next_chunk;

		/* Reset the head read offset, since we've processed at least the first chunk now */
		self->head_read_offset = 0;
	}

	/* If the requested length is still > 0, it must be < chunk->length, and chunk must != NULL (if it does, the cached total_length has
	 * been corrupted somewhere). */
	if (G_LIKELY (length_remaining > 0)) {
		/* Copy the requested data to the output */
		memcpy (data, chunk->data, length_remaining);
		data += length_remaining;
		self->head_read_offset = length_remaining;
	}

	self->head = chunk;
	if (self->head == NULL)
		self->tail = NULL;
	self->total_length -= return_length;

	g_static_mutex_unlock (&(self->mutex));

	return return_length;
}
