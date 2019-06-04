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

#ifndef GDATA_H
#define GDATA_H

/* Core files */
#include <gdata/gdata-entry.h>
#include <gdata/gdata-feed.h>
#include <gdata/gdata-service.h>
#include <gdata/gdata-types.h>
#include <gdata/gdata-query.h>
#include <gdata/gdata-enums.h>
#include <gdata/gdata-access-handler.h>
#include <gdata/gdata-access-rule.h>
#include <gdata/gdata-parsable.h>
#include <gdata/gdata-download-stream.h>
#include <gdata/gdata-upload-stream.h>
#include <gdata/gdata-comparable.h>
#include <gdata/gdata-batchable.h>
#include <gdata/gdata-batch-operation.h>
#include <gdata/gdata-authorizer.h>
#include <gdata/gdata-authorization-domain.h>
#include <gdata/gdata-client-login-authorizer.h>
#include <gdata/gdata-oauth1-authorizer.h>
#include <gdata/gdata-oauth2-authorizer.h>
#include <gdata/gdata-property.h>
#ifdef GOA_API_IS_SUBJECT_TO_CHANGE
/* You need to define GOA_API_IS_SUBJECT_TO_CHANGE in order to use the GOA authoriser. */
#include <gdata/gdata-goa-authorizer.h>
#endif /* GOA_API_IS_SUBJECT_TO_CHANGE */
#include <gdata/gdata-commentable.h>
#include <gdata/gdata-comment.h>
#include <gdata/gdata-version.h>

/* Namespaces */

/* Atom */
#include <gdata/atom/gdata-author.h>
#include <gdata/atom/gdata-category.h>
#include <gdata/atom/gdata-generator.h>
#include <gdata/atom/gdata-link.h>

/* APP */
#include <gdata/app/gdata-app-categories.h>

/* GData */
#include <gdata/gd/gdata-gd-email-address.h>
#include <gdata/gd/gdata-gd-im-address.h>
#include <gdata/gd/gdata-gd-name.h>
#include <gdata/gd/gdata-gd-organization.h>
#include <gdata/gd/gdata-gd-phone-number.h>
#include <gdata/gd/gdata-gd-postal-address.h>
#include <gdata/gd/gdata-gd-reminder.h>
#include <gdata/gd/gdata-gd-when.h>
#include <gdata/gd/gdata-gd-where.h>
#include <gdata/gd/gdata-gd-who.h>

/* GContact */
#include <gdata/gcontact/gdata-gcontact-calendar.h>
#include <gdata/gcontact/gdata-gcontact-event.h>
#include <gdata/gcontact/gdata-gcontact-external-id.h>
#include <gdata/gcontact/gdata-gcontact-jot.h>
#include <gdata/gcontact/gdata-gcontact-language.h>
#include <gdata/gcontact/gdata-gcontact-relation.h>
#include <gdata/gcontact/gdata-gcontact-website.h>

/* Media RSS */
#include <gdata/media/gdata-media-category.h>
#include <gdata/media/gdata-media-content.h>
#include <gdata/media/gdata-media-credit.h>
#include <gdata/media/gdata-media-enums.h>
#include <gdata/media/gdata-media-thumbnail.h>

/* Services */

/* YouTube */
#include <gdata/services/youtube/gdata-youtube-service.h>
#include <gdata/services/youtube/gdata-youtube-feed.h>
#include <gdata/services/youtube/gdata-youtube-query.h>
#include <gdata/services/youtube/gdata-youtube-video.h>
#include <gdata/services/youtube/gdata-youtube-content.h>
#include <gdata/services/youtube/gdata-youtube-credit.h>
#include <gdata/services/youtube/gdata-youtube-state.h>
#include <gdata/services/youtube/gdata-youtube-enums.h>
#include <gdata/services/youtube/gdata-youtube-category.h>
#include <gdata/services/youtube/gdata-youtube-comment.h>

/* Google Calendar */
#include <gdata/services/calendar/gdata-calendar-service.h>
#include <gdata/services/calendar/gdata-calendar-feed.h>
#include <gdata/services/calendar/gdata-calendar-calendar.h>
#include <gdata/services/calendar/gdata-calendar-event.h>
#include <gdata/services/calendar/gdata-calendar-query.h>
#include <gdata/services/calendar/gdata-calendar-access-rule.h>

/* Google PicasaWeb */
#include <gdata/services/picasaweb/gdata-picasaweb-service.h>
#include <gdata/services/picasaweb/gdata-picasaweb-feed.h>
#include <gdata/services/picasaweb/gdata-picasaweb-query.h>
#include <gdata/services/picasaweb/gdata-picasaweb-album.h>
#include <gdata/services/picasaweb/gdata-picasaweb-file.h>
#include <gdata/services/picasaweb/gdata-picasaweb-enums.h>
#include <gdata/services/picasaweb/gdata-picasaweb-user.h>
#include <gdata/services/picasaweb/gdata-picasaweb-comment.h>

/* Google Contacts */
#include <gdata/services/contacts/gdata-contacts-service.h>
#include <gdata/services/contacts/gdata-contacts-contact.h>
#include <gdata/services/contacts/gdata-contacts-group.h>
#include <gdata/services/contacts/gdata-contacts-query.h>

/* Google Documents*/
#include <gdata/services/documents/gdata-documents-entry.h>
#include <gdata/services/documents/gdata-documents-document.h>
#include <gdata/services/documents/gdata-documents-text.h>
#include <gdata/services/documents/gdata-documents-spreadsheet.h>
#include <gdata/services/documents/gdata-documents-presentation.h>
#include <gdata/services/documents/gdata-documents-folder.h>
#include <gdata/services/documents/gdata-documents-query.h>
#include <gdata/services/documents/gdata-documents-service.h>
#include <gdata/services/documents/gdata-documents-metadata.h>
#include <gdata/services/documents/gdata-documents-feed.h>
#include <gdata/services/documents/gdata-documents-drawing.h>
#include <gdata/services/documents/gdata-documents-pdf.h>
#include <gdata/services/documents/gdata-documents-enums.h>
#include <gdata/services/documents/gdata-documents-upload-query.h>
#include <gdata/services/documents/gdata-documents-access-rule.h>

/* Google Tasks */
#include <gdata/services/tasks/gdata-tasks-service.h>
#include <gdata/services/tasks/gdata-tasks-query.h>
#include <gdata/services/tasks/gdata-tasks-tasklist.h>
#include <gdata/services/tasks/gdata-tasks-task.h>

/* Google Freebase */
#include <gdata/services/freebase/gdata-freebase-service.h>
#include <gdata/services/freebase/gdata-freebase-query.h>
#include <gdata/services/freebase/gdata-freebase-result.h>
#include <gdata/services/freebase/gdata-freebase-search-query.h>
#include <gdata/services/freebase/gdata-freebase-search-result.h>
#include <gdata/services/freebase/gdata-freebase-topic-query.h>
#include <gdata/services/freebase/gdata-freebase-topic-result.h>
#include <gdata/services/freebase/gdata-freebase-enums.h>

#endif /* !GDATA_H */
