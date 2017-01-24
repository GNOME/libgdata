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

/**
 * SECTION:gdata-comparable
 * @short_description: GData comparable interface
 * @stability: Stable
 * @include: gdata/gdata-comparable.h
 *
 * #GDataComparable is an interface which can be implemented by any object which needs to be compared to another object of the same type or of a
 * derived type.
 *
 * When implementing the interface, classes must implement the <function>compare_with</function> function, and the implementation must be
 * <ulink type="http" url="http://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html#index-g_t_0040code_007bpure_007d-function-attribute-2413">pure
 * </ulink>.
 *
 * Since: 0.7.0
 */

#include <glib.h>
#include <glib-object.h>

#include "gdata-comparable.h"

GType
gdata_comparable_get_type (void)
{
	static GType comparable_type = 0;

	if (!comparable_type) {
		comparable_type = g_type_register_static_simple (G_TYPE_INTERFACE, "GDataComparable",
		                                                 sizeof (GDataComparableIface),
		                                                 NULL, 0, NULL, 0);
	}

	return comparable_type;
}

/**
 * gdata_comparable_compare:
 * @self: (allow-none): a #GDataComparable, or %NULL
 * @other: (allow-none): another #GDataComparable of the same type, or %NULL
 *
 * Compares the two objects, returning <code class="literal">-1</code> if @self is "less than" @other by some metric, <code class="literal">0</code>
 * if they're equal, or <code class="literal">1</code> if @self is "greater than" @other.
 *
 * %NULL values are handled gracefully, with <code class="literal">0</code> returned if both @self and @other are %NULL,
 * <code class="literal">-1</code> if @self is %NULL and <code class="literal">1</code> if @other is %NULL.
 *
 * The @other object must be of the same type as @self, or of a type derived from @self's type.
 *
 * Return value: %TRUE on success, %FALSE otherwise
 *
 * Since: 0.7.0
 */
gint
gdata_comparable_compare (GDataComparable *self, GDataComparable *other)
{
	GDataComparableIface *iface;

	g_return_val_if_fail (self == NULL || GDATA_IS_COMPARABLE (self), 0);
	g_return_val_if_fail (other == NULL || GDATA_IS_COMPARABLE (other), 0);
	g_return_val_if_fail (self == NULL || other == NULL || g_type_is_a (G_OBJECT_TYPE (other), G_OBJECT_TYPE (self)), 0);

	/* Deal with NULL values */
	if (self == NULL && other != NULL)
		return -1;
	else if (self != NULL && other == NULL)
		return 1;

	if (self == other)
		return 0;

	/* Use the comparator method for non-NULL values */
	iface = GDATA_COMPARABLE_GET_IFACE (self);
	g_assert (iface->compare_with != NULL);

	return iface->compare_with (self, other);
}
