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

#ifndef GDATA_DOCUMENTS_ACCESS_RULE_H
#define GDATA_DOCUMENTS_ACCESS_RULE_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-access-rule.h>

G_BEGIN_DECLS

#define GDATA_TYPE_DOCUMENTS_ACCESS_RULE		(gdata_documents_access_rule_get_type ())
#define GDATA_DOCUMENTS_ACCESS_RULE(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_DOCUMENTS_ACCESS_RULE, GDataDocumentsAccessRule))
#define GDATA_DOCUMENTS_ACCESS_RULE_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_DOCUMENTS_ACCESS_RULE, GDataDocumentsAccessRuleClass))
#define GDATA_IS_DOCUMENTS_ACCESS_RULE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_DOCUMENTS_ACCESS_RULE))
#define GDATA_IS_DOCUMENTS_ACCESS_RULE_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_DOCUMENTS_ACCESS_RULE))
#define GDATA_DOCUMENTS_ACCESS_RULE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_DOCUMENTS_ACCESS_RULE, GDataDocumentsAccessRuleClass))

/**
 * GDataDocumentsAccessRule:
 *
 * All the fields in the #GDataDocumentsAccessRule structure are private and
 * should never be accessed directly.
 *
 * Since: 0.17.2
 */
typedef struct {
	GDataAccessRule parent;
} GDataDocumentsAccessRule;

/**
 * GDataDocumentsAccessRuleClass:
 *
 * All the fields in the #GDataDocumentsAccessRuleClass structure are private
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
} GDataDocumentsAccessRuleClass;

GType gdata_documents_access_rule_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataDocumentsAccessRule, g_object_unref)

GDataDocumentsAccessRule *
gdata_documents_access_rule_new (const gchar *id) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_DOCUMENTS_ACCESS_RULE_H */
