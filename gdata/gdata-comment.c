/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2011 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-comment
 * @short_description: GData comment object
 * @stability: Stable
 * @include: gdata/gdata-comment.h
 *
 * #GDataComment is a subclass of #GDataEntry to represent a generic comment on an entry. It is returned by the methods implemented in the
 * #GDataCommentable interface.
 *
 * Any class which implements #GDataCommentable should have its own concrete subclass of #GDataComment which provides service-specific functionality.
 *
 * All subclasses of #GDataComment should ensure that the body of a comment is accessible using gdata_entry_get_content(), and that each comment has
 * at least one #GDataAuthor object representing the person who wrote the comment, accessible using gdata_entry_get_authors().
 *
 * Since: 0.10.0
 */

#include <config.h>
#include <glib.h>

#include "gdata-comment.h"

G_DEFINE_ABSTRACT_TYPE (GDataComment, gdata_comment, GDATA_TYPE_ENTRY)

static void
gdata_comment_class_init (GDataCommentClass *klass)
{
	/* Nothing to see here */
}

static void
gdata_comment_init (GDataComment *self)
{
	/* Nothing to see here */
}
