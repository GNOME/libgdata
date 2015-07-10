/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2015 <philip@tecnocode.co.uk>
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

#ifndef GDATA_CALENDAR_ACCESS_RULE_H
#define GDATA_CALENDAR_ACCESS_RULE_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-access-rule.h>

G_BEGIN_DECLS

/**
 * GDATA_CALENDAR_ACCESS_ROLE_READ:
 *
 * The users specified by the #GDataCalendarAccessRule have read-only access to
 * the calendar.
 *
 * Since: 0.7.0
 */
#define GDATA_CALENDAR_ACCESS_ROLE_READ "http://schemas.google.com/gCal/2005#read"

/**
 * GDATA_CALENDAR_ACCESS_ROLE_FREE_BUSY:
 *
 * The users specified by the #GDataCalendarAccessRule can only see the
 * free/busy information on the calendar; not event details.
 *
 * Since: 0.7.0
 */
#define GDATA_CALENDAR_ACCESS_ROLE_FREE_BUSY "http://schemas.google.com/gCal/2005#freebusy"

/**
 * GDATA_CALENDAR_ACCESS_ROLE_EDITOR:
 *
 * The users specified by the #GDataCalendarAccessRule have full edit access to
 * the calendar, except they can’t change the calendar’s access rules.
 *
 * Since: 0.7.0
 */
#define GDATA_CALENDAR_ACCESS_ROLE_EDITOR "http://schemas.google.com/gCal/2005#editor"

/**
 * GDATA_CALENDAR_ACCESS_ROLE_OWNER:
 *
 * The users specified by the #GDataCalendarAccessRule have full owner access
 * to the calendar.
 *
 * Since: 0.7.0
 */
#define GDATA_CALENDAR_ACCESS_ROLE_OWNER "http://schemas.google.com/gCal/2005#owner"

/**
 * GDATA_CALENDAR_ACCESS_ROLE_ROOT:
 *
 * The users specified by the #GDataCalendarAccessRule have full administrator
 * access to the calendar server. This is only available in Google Apps For
 * Your Domain.
 *
 * Since: 0.7.0
 */
#define GDATA_CALENDAR_ACCESS_ROLE_ROOT "http://schemas.google.com/gCal/2005#root"

#define GDATA_TYPE_CALENDAR_ACCESS_RULE		(gdata_calendar_access_rule_get_type ())
#define GDATA_CALENDAR_ACCESS_RULE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_CALENDAR_ACCESS_RULE, GDataCalendarAccessRule))
#define GDATA_CALENDAR_ACCESS_RULE_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_CALENDAR_ACCESS_RULE, GDataCalendarAccessRuleClass))
#define GDATA_IS_CALENDAR_ACCESS_RULE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_CALENDAR_ACCESS_RULE))
#define GDATA_IS_CALENDAR_ACCESS_RULE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_CALENDAR_ACCESS_RULE))
#define GDATA_CALENDAR_ACCESS_RULE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_CALENDAR_ACCESS_RULE, GDataCalendarAccessRuleClass))

/**
 * GDataCalendarAccessRule:
 *
 * All the fields in the #GDataCalendarAccessRule structure are private and
 * should never be accessed directly.
 *
 * Since: 0.17.2
 */
typedef struct {
	GDataAccessRule parent;
} GDataCalendarAccessRule;

/**
 * GDataCalendarAccessRuleClass:
 *
 * All the fields in the #GDataCalendarAccessRuleClass structure are private
 * and should never be accessed directly.
 *
 * Since: 0.17.2
 */
typedef struct {
	/*< private >*/
	GDataAccessRuleClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataCalendarAccessRuleClass;

GType gdata_calendar_access_rule_get_type (void) G_GNUC_CONST;

GDataCalendarAccessRule *
gdata_calendar_access_rule_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_CALENDAR_ACCESS_RULE_H */
