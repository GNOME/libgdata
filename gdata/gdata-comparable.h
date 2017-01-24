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

#ifndef GDATA_COMPARABLE_H
#define GDATA_COMPARABLE_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GDATA_TYPE_COMPARABLE		(gdata_comparable_get_type ())
#define GDATA_COMPARABLE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_COMPARABLE, GDataComparable))
#define GDATA_COMPARABLE_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_COMPARABLE, GDataComparableIface))
#define GDATA_IS_COMPARABLE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_COMPARABLE))
#define GDATA_COMPARABLE_GET_IFACE(o)	(G_TYPE_INSTANCE_GET_INTERFACE ((o), GDATA_TYPE_COMPARABLE, GDataComparableIface))

/**
 * GDataComparable:
 *
 * All the fields in the #GDataComparable structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct _GDataComparable		GDataComparable; /* dummy typedef */

/**
 * GDataComparableIface:
 * @parent: the parent type
 * @compare_with: compares the object with an @other object of the same type, returning <code class="literal">-1</code> if the object is "less than"
 * the other object, <code class="literal">0</code> if they're equal, or <code class="literal">1</code> if the object is "greater than" the other. The
 * function can assume that neither @self or @other will be %NULL, and that both have correct types. The function must be pure.
 *
 * The class structure for the #GDataComparable interface.
 *
 * Since: 0.7.0
 */
typedef struct {
	GTypeInterface parent;

	gint (*compare_with) (GDataComparable *self, GDataComparable *other);
} GDataComparableIface;

GType gdata_comparable_get_type (void) G_GNUC_CONST;

gint gdata_comparable_compare (GDataComparable *self, GDataComparable *other) G_GNUC_PURE;

G_END_DECLS

#endif /* !GDATA_COMPARABLE_H */
