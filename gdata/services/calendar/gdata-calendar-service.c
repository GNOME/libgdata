/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009, 2014, 2015 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-calendar-service
 * @short_description: GData Calendar service object
 * @stability: Stable
 * @include: gdata/services/calendar/gdata-calendar-service.h
 *
 * #GDataCalendarService is a subclass of #GDataService for communicating with the GData API of Google Calendar. It supports querying
 * for, inserting, editing and deleting events from calendars, as well as operations on the calendars themselves.
 *
 * For more details of Google Calendar's GData API, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/calendar/v3/reference/">
 * online documentation</ulink>.
 *
 * Each calendar accessible through the service has an access control list (ACL) which defines the level of access to the calendar to each user, and
 * which users the calendar is shared with. For more information about ACLs for calendars, see the
 * <ulink type="http" url="https://developers.google.com/google-apps/calendar/v3/reference/acl">online documentation on
 * sharing calendars</ulink>.
 *
 * <example>
 * 	<title>Retrieving the Access Control List for a Calendar</title>
 * 	<programlisting>
 *	GDataCalendarService *service;
 *	GDataCalendarCalendar *calendar;
 *	GDataFeed *acl_feed;
 *	GDataAccessRule *rule, *new_rule;
 *	GDataLink *acl_link;
 *	GList *i;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Create a service and retrieve a calendar to work on *<!-- -->/
 *	service = create_calendar_service ();
 *	calendar = get_calendar (service);
 *
 *	/<!-- -->* Query the service for the ACL for the given calendar *<!-- -->/
 *	acl_feed = gdata_access_handler_get_rules (GDATA_ACCESS_HANDLER (calendar), GDATA_SERVICE (service), NULL, NULL, NULL, &error);
 *
 *	if (error != NULL) {
 *		g_error ("Error getting ACL feed for calendar: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (calendar);
 *		g_object_unref (service);
 *		return;
 *	}
 *
 *	/<!-- -->* Iterate through the ACL *<!-- -->/
 *	for (i = gdata_feed_get_entries (acl_feed); i != NULL; i = i->next) {
 *		const gchar *scope_value;
 *
 *		rule = GDATA_ACCESS_RULE (i->data);
 *
 *		/<!-- -->* Do something with the access rule here. As an example, we update the rule applying to test@gmail.com and delete all
 *		 * the other rules. We then insert another rule for example@gmail.com below. *<!-- -->/
 *		gdata_access_rule_get_scope (rule, NULL, &scope_value);
 *		if (scope_value != NULL && strcmp (scope_value, "test@gmail.com") == 0) {
 *			GDataAccessRule *updated_rule;
 *
 *			/<!-- -->* Update the rule to make test@gmail.com an editor (full read/write access to the calendar, but they can't change
 *			 * the ACL). *<!-- -->/
 *			gdata_access_rule_set_role (rule, GDATA_CALENDAR_ACCESS_ROLE_EDITOR);
 *			updated_rule = GDATA_ACCESS_RULE (gdata_service_update_entry (GDATA_SERVICE (service), GDATA_ENTRY (rule), NULL, &error));
 *
 *			if (error != NULL) {
 *				g_error ("Error updating access rule for %s: %s", scope_value, error->message);
 *				g_error_free (error);
 *				g_object_unref (acl_feed);
 *				g_object_unref (calendar);
 *				g_object_unref (service);
 *				return;
 *			}
 *
 *			g_object_unref (updated_rule);
 *		} else {
 *			/<!-- -->* Delete any rule which doesn't apply to test@gmail.com *<!-- -->/
 *			gdata_service_delete_entry (GDATA_SERVICE (service), GDATA_ENTRY (rule), NULL, &error);
 *
 *			if (error != NULL) {
 *				g_error ("Error deleting access rule for %s: %s", scope_value, error->message);
 *				g_error_free (error);
 *				g_object_unref (acl_feed);
 *				g_object_unref (calendar);
 *				g_object_unref (service);
 *				return;
 *			}
 *		}
 *	}
 *
 *	g_object_unref (acl_feed);
 *
 *	/<!-- -->* Create and insert a new access rule for example@gmail.com which allows them to view free/busy information for events in the
 *	 * calendar, but doesn't allow them to view the full event details. *<!-- -->/
 *	rule = gdata_access_rule_new (NULL);
 *	gdata_access_rule_set_role (rule, GDATA_CALENDAR_ACCESS_ROLE_FREE_BUSY);
 *	gdata_access_rule_set_scope (rule, GDATA_ACCESS_SCOPE_USER, "example@gmail.com");
 *
 *	acl_link = gdata_entry_look_up_link (GDATA_ENTRY (calendar), GDATA_LINK_ACCESS_CONTROL_LIST);
 *	new_rule = GDATA_ACCESS_RULE (gdata_service_insert_entry (GDATA_SERVICE (service), gdata_link_get_uri (acl_link), GDATA_ENTRY (rule),
 *	                                                          NULL, &error));
 *
 *	g_object_unref (rule);
 *	g_object_unref (calendar);
 *	g_object_unref (service);
 *
 *	if (error != NULL) {
 *		g_error ("Error inserting access rule: %s", error->message);
 *		g_error_free (error);
 *		return;
 *	}
 *
 *	g_object_unref (acl_link);
 * 	</programlisting>
 * </example>
 *
 * Before version 0.17.2, the Calendar service could be manipulated using
 * batch operations. That is no longer supported, and any batch operations
 * created on the calendar will fail.
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libsoup/soup.h>
#include <string.h>

#include "gdata-calendar-service.h"
#include "gdata-batchable.h"
#include "gdata-service.h"
#include "gdata-private.h"
#include "gdata-query.h"
#include "gdata-calendar-feed.h"

/* Standards reference here:
 * https://developers.google.com/google-apps/calendar/v3/reference/ */

static void
parse_error_response (GDataService *self,
                      GDataOperationType operation_type,
                      guint status,
                      const gchar *reason_phrase,
                      const gchar *response_body,
                      gint length,
                      GError **error);
static GList *get_authorization_domains (void);

_GDATA_DEFINE_AUTHORIZATION_DOMAIN (calendar, "cl", "https://www.google.com/calendar/feeds/")
G_DEFINE_TYPE_WITH_CODE (GDataCalendarService, gdata_calendar_service, GDATA_TYPE_SERVICE, G_IMPLEMENT_INTERFACE (GDATA_TYPE_BATCHABLE, NULL))

static void
gdata_calendar_service_class_init (GDataCalendarServiceClass *klass)
{
	GDataServiceClass *service_class = GDATA_SERVICE_CLASS (klass);
	service_class->feed_type = GDATA_TYPE_CALENDAR_FEED;
	service_class->parse_error_response = parse_error_response;
	service_class->get_authorization_domains = get_authorization_domains;
}

static void
gdata_calendar_service_init (GDataCalendarService *self)
{
	/* Nothing to see here */
}

/* The error format used by the Google Calendar API doesn’t seem to be
 * documented anywhere, which is a little frustrating. Here’s an example of it:
 *     {
 *      "error": {
 *       "errors": [
 *        {
 *         "domain": "global",
 *         "reason": "parseError",
 *         "message": "Parse Error",
 *        }
 *       ],
 *       "code": 400,
 *       "message": "Parse Error"
 *      }
 *     }
 * or:
 *     {
 *      "error": {
 *       "errors": [
 *        {
 *         "domain": "global",
 *         "reason": "required",
 *         "message": "Missing end time."
 *        }
 *       ],
 *       "code": 400,
 *       "message": "Missing end time."
 *      }
 *     }
 */
static void
parse_error_response (GDataService *self,
                      GDataOperationType operation_type,
                      guint status,
                      const gchar *reason_phrase,
                      const gchar *response_body,
                      gint length,
                      GError **error)
{
	JsonParser *parser = NULL;  /* owned */
	JsonReader *reader = NULL;  /* owned */
	gint i;
	GError *child_error = NULL;

	if (response_body == NULL) {
		goto parent;
	}

	if (length == -1) {
		length = strlen (response_body);
	}

	parser = json_parser_new ();
	if (!json_parser_load_from_data (parser, response_body, length,
	                                 &child_error)) {
		goto parent;
	}

	reader = json_reader_new (json_parser_get_root (parser));

	/* Check that the outermost node is an object. */
	if (!json_reader_is_object (reader)) {
		goto parent;
	}

	/* Grab the ‘error’ member, then its ‘errors’ member. */
	if (!json_reader_read_member (reader, "error") ||
	    !json_reader_is_object (reader) ||
	    !json_reader_read_member (reader, "errors") ||
	    !json_reader_is_array (reader)) {
		goto parent;
	}

	/* Parse each of the errors. Return the first one, and print out any
	 * others. */
	for (i = 0; i < json_reader_count_elements (reader); i++) {
		const gchar *domain, *reason, *message, *extended_help;
		const gchar *location_type, *location;

		/* Parse the error. */
		if (!json_reader_read_element (reader, i) ||
		    !json_reader_is_object (reader)) {
			goto parent;
		}

		json_reader_read_member (reader, "domain");
		domain = json_reader_get_string_value (reader);
		json_reader_end_member (reader);

		json_reader_read_member (reader, "reason");
		reason = json_reader_get_string_value (reader);
		json_reader_end_member (reader);

		json_reader_read_member (reader, "message");
		message = json_reader_get_string_value (reader);
		json_reader_end_member (reader);

		json_reader_read_member (reader, "extendedHelp");
		extended_help = json_reader_get_string_value (reader);
		json_reader_end_member (reader);

		json_reader_read_member (reader, "locationType");
		location_type = json_reader_get_string_value (reader);
		json_reader_end_member (reader);

		json_reader_read_member (reader, "location");
		location = json_reader_get_string_value (reader);
		json_reader_end_member (reader);

		/* End the error element. */
		json_reader_end_element (reader);

		/* Create an error message, but only for the first error */
		if (error == NULL || *error == NULL) {
			if (g_strcmp0 (domain, "usageLimits") == 0 &&
			    g_strcmp0 (reason,
			               "dailyLimitExceededUnreg") == 0) {
				/* Daily Limit for Unauthenticated Use
				 * Exceeded. */
				g_set_error (error, GDATA_SERVICE_ERROR,
				             GDATA_SERVICE_ERROR_API_QUOTA_EXCEEDED,
				             _("You have made too many API "
				               "calls recently. Please wait a "
				               "few minutes and try again."));
			} else if (g_strcmp0 (domain, "global") == 0 &&
			           g_strcmp0 (reason, "notFound") == 0) {
				/* Calendar not found. */
				g_set_error (error, GDATA_SERVICE_ERROR,
				             GDATA_SERVICE_ERROR_NOT_FOUND,
				             /* Translators: the parameter is an
				              * error message returned by the
				              * server. */
				             _("The requested resource was not found: %s"),
				             message);
			} else if ((g_strcmp0 (domain, "global") == 0 &&
			            g_strcmp0 (reason, "required") == 0) ||
			           (g_strcmp0 (domain, "global") == 0 &&
			            g_strcmp0 (reason, "conditionNotMet") == 0)) {
				/* Client-side protocol error. */
				g_set_error (error, GDATA_SERVICE_ERROR,
				             GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
				             /* Translators: the parameter is an
				              * error message returned by the
				              * server. */
				             _("Invalid request URI or header, "
				               "or unsupported nonstandard "
				               "parameter: %s"), message);
			} else if (g_strcmp0 (domain, "global") == 0 &&
			           (g_strcmp0 (reason, "authError") == 0 ||
			            g_strcmp0 (reason, "required") == 0)) {
				/* Authentication problem */
				g_set_error (error, GDATA_SERVICE_ERROR,
				             GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
				             _("You must be authenticated to "
				               "do this."));
			} else if (g_strcmp0 (domain, "global") == 0 &&
			           g_strcmp0 (reason, "forbidden") == 0) {
				g_set_error (error, GDATA_SERVICE_ERROR,
				             GDATA_SERVICE_ERROR_FORBIDDEN,
				             _("Access was denied by the user "
				               "or server."));
			} else {
				/* Unknown or validation (protocol) error. Fall
				 * back to working off the HTTP status code. */
				g_warning ("Unknown error code ‘%s’ in domain "
				           "‘%s’ received with location type "
				           "‘%s’, location ‘%s’, extended help "
				           "‘%s’ and message ‘%s’.",
				           reason, domain, location_type,
				           location, extended_help, message);

				goto parent;
			}
		} else {
			/* For all errors after the first, log the error in the
			 * terminal. */
			g_debug ("Error message received in response: domain "
			         "‘%s’, reason ‘%s’, extended help ‘%s’, "
			         "message ‘%s’, location type ‘%s’, location "
			         "‘%s’.",
			         domain, reason, extended_help, message,
			         location_type, location);
		}
	}

	/* End the ‘errors’ and ‘error’ members. */
	json_reader_end_element (reader);
	json_reader_end_element (reader);

	g_clear_object (&reader);
	g_clear_object (&parser);

	/* Ensure we’ve actually set an error message. */
	g_assert (error == NULL || *error != NULL);

	return;

parent:
	g_clear_object (&reader);
	g_clear_object (&parser);

	/* Chain up to the parent class */
	GDATA_SERVICE_CLASS (gdata_calendar_service_parent_class)->parse_error_response (self, operation_type, status, reason_phrase,
	                                                                                 response_body, length, error);
}

static GList *
get_authorization_domains (void)
{
	return g_list_prepend (NULL, get_calendar_authorization_domain ());
}

/**
 * gdata_calendar_service_new:
 * @authorizer: (allow-none): a #GDataAuthorizer to authorize the service's requests, or %NULL
 *
 * Creates a new #GDataCalendarService using the given #GDataAuthorizer. If @authorizer is %NULL, all requests are made as an unauthenticated user.
 *
 * Return value: a new #GDataCalendarService, or %NULL; unref with g_object_unref()
 *
 * Since: 0.9.0
 */
GDataCalendarService *
gdata_calendar_service_new (GDataAuthorizer *authorizer)
{
	g_return_val_if_fail (authorizer == NULL || GDATA_IS_AUTHORIZER (authorizer), NULL);

	return g_object_new (GDATA_TYPE_CALENDAR_SERVICE,
	                     "authorizer", authorizer,
	                     NULL);
}

/**
 * gdata_calendar_service_get_primary_authorization_domain:
 *
 * The primary #GDataAuthorizationDomain for interacting with Google Calendar. This will not normally need to be used, as it's used internally
 * by the #GDataCalendarService methods. However, if using the plain #GDataService methods to implement custom queries or requests which libgdata
 * does not support natively, then this domain may be needed to authorize the requests.
 *
 * The domain never changes, and is interned so that pointer comparison can be used to differentiate it from other authorization domains.
 *
 * Return value: (transfer none): the service's authorization domain
 *
 * Since: 0.9.0
 */
GDataAuthorizationDomain *
gdata_calendar_service_get_primary_authorization_domain (void)
{
	return get_calendar_authorization_domain ();
}

/**
 * gdata_calendar_service_query_all_calendars:
 * @self: a #GDataCalendarService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (scope call) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service to return a list of all calendars from the authenticated account which match the given
 * @query. It will return all calendars the user has read access to, including primary, secondary and imported
 * calendars.
 *
 * For more details, see gdata_service_query().
 *
 * Return value: (transfer full): a #GDataFeed of query results; unref with g_object_unref()
 */
GDataFeed *
gdata_calendar_service_query_all_calendars (GDataCalendarService *self, GDataQuery *query, GCancellable *cancellable,
                                            GDataQueryProgressCallback progress_callback, gpointer progress_user_data, GError **error)
{
	GDataFeed *feed;
	gchar *request_uri;

	g_return_val_if_fail (GDATA_IS_CALENDAR_SERVICE (self), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Ensure we're authenticated first */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_calendar_authorization_domain ()) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to query all calendars."));
		return NULL;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.googleapis.com/calendar/v3/users/me/calendarList", NULL);
	feed = gdata_service_query (GDATA_SERVICE (self), get_calendar_authorization_domain (), request_uri, query, GDATA_TYPE_CALENDAR_CALENDAR,
	                            cancellable, progress_callback, progress_user_data, error);
	g_free (request_uri);

	return feed;
}

/**
 * gdata_calendar_service_query_all_calendars_async:
 * @self: a #GDataCalendarService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @destroy_progress_user_data: (allow-none): the function to call when @progress_callback will not be called any more, or %NULL. This function will be
 * called with @progress_user_data as a parameter and can be used to free any memory allocated for it.
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Queries the service to return a list of all calendars from the authenticated account which match the given
 * @query. @self and @query are all reffed when this function is called, so can safely be unreffed after
 * this function returns.
 *
 * For more details, see gdata_calendar_service_query_all_calendars(), which is the synchronous version of
 * this function, and gdata_service_query_async(), which is the base asynchronous query function.
 *
 * Since: 0.9.1
 */
void
gdata_calendar_service_query_all_calendars_async (GDataCalendarService *self, GDataQuery *query, GCancellable *cancellable,
                                                  GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                  GDestroyNotify destroy_progress_user_data,
                                                  GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *request_uri;

	g_return_if_fail (GDATA_IS_CALENDAR_SERVICE (self));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	/* Ensure we're authenticated first */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_calendar_authorization_domain ()) == FALSE) {
		g_autoptr(GTask) task = NULL;

		task = g_task_new (self, cancellable, callback, user_data);
		g_task_set_source_tag (task, gdata_service_query_async);
		g_task_return_new_error (task, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED, "%s",
		                         _("You must be authenticated to query all calendars."));

		return;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.googleapis.com/calendar/v3/users/me/calendarList", NULL);
	gdata_service_query_async (GDATA_SERVICE (self), get_calendar_authorization_domain (), request_uri, query, GDATA_TYPE_CALENDAR_CALENDAR,
	                           cancellable, progress_callback, progress_user_data, destroy_progress_user_data, callback, user_data);
	g_free (request_uri);
}

/**
 * gdata_calendar_service_query_own_calendars:
 * @self: a #GDataCalendarService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (scope call) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service to return a list of calendars from the authenticated account which match the given
 * @query, and the authenticated user owns. (i.e. They have full read/write access to the calendar, as well
 * as the ability to set permissions on the calendar.)
 *
 * For more details, see gdata_service_query().
 *
 * Return value: (transfer full): a #GDataFeed of query results; unref with g_object_unref()
 */
GDataFeed *
gdata_calendar_service_query_own_calendars (GDataCalendarService *self, GDataQuery *query, GCancellable *cancellable,
                                            GDataQueryProgressCallback progress_callback, gpointer progress_user_data, GError **error)
{
	GDataFeed *feed;
	gchar *request_uri;

	g_return_val_if_fail (GDATA_IS_CALENDAR_SERVICE (self), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Ensure we're authenticated first */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_calendar_authorization_domain ()) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to query your own calendars."));
		return NULL;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.googleapis.com/calendar/v3/users/me/calendarList?minAccessRole=owner", NULL);
	feed = gdata_service_query (GDATA_SERVICE (self), get_calendar_authorization_domain (), request_uri, query, GDATA_TYPE_CALENDAR_CALENDAR,
	                            cancellable, progress_callback, progress_user_data, error);
	g_free (request_uri);

	return feed;
}

/**
 * gdata_calendar_service_query_own_calendars_async:
 * @self: a #GDataCalendarService
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @destroy_progress_user_data: (allow-none): the function to call when @progress_callback will not be called any more, or %NULL. This function will be
 * called with @progress_user_data as a parameter and can be used to free any memory allocated for it.
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Queries the service to return a list of calendars from the authenticated account which match the given
 * @query, and the authenticated user owns. @self and @query are all reffed when this function is called,
 * so can safely be unreffed after this function returns.
 *
 * For more details, see gdata_calendar_service_query_own_calendars(), which is the synchronous version of
 * this function, and gdata_service_query_async(), which is the base asynchronous query function.
 *
 * Since: 0.9.1
 */
void
gdata_calendar_service_query_own_calendars_async (GDataCalendarService *self, GDataQuery *query, GCancellable *cancellable,
                                                  GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                                  GDestroyNotify destroy_progress_user_data,
                                                  GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *request_uri;

	g_return_if_fail (GDATA_IS_CALENDAR_SERVICE (self));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	/* Ensure we're authenticated first */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_calendar_authorization_domain ()) == FALSE) {
		g_autoptr(GTask) task = NULL;

		task = g_task_new (self, cancellable, callback, user_data);
		g_task_set_source_tag (task, gdata_service_query_async);
		g_task_return_new_error (task, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED, "%s",
		                         _("You must be authenticated to query your own calendars."));

		return;
	}

	request_uri = g_strconcat (_gdata_service_get_scheme (), "://www.googleapis.com/calendar/v3/users/me/calendarList?minAccessRole=owner", NULL);
	gdata_service_query_async (GDATA_SERVICE (self), get_calendar_authorization_domain (), request_uri, query, GDATA_TYPE_CALENDAR_CALENDAR,
	                           cancellable, progress_callback, progress_user_data, destroy_progress_user_data, callback, user_data);
	g_free (request_uri);
}

static gchar *
build_events_uri (GDataCalendarCalendar *calendar)
{
	GString *uri;
	const gchar *calendar_id;

	calendar_id = (calendar != NULL) ? gdata_entry_get_id (GDATA_ENTRY (calendar)) : "default";

	uri = g_string_new (_gdata_service_get_scheme ());
	g_string_append (uri, "://www.googleapis.com/calendar/v3/calendars/");
	g_string_append_uri_escaped (uri, calendar_id, NULL, FALSE);
	g_string_append (uri, "/events");

	return g_string_free (uri, FALSE);
}

/**
 * gdata_calendar_service_query_events:
 * @self: a #GDataCalendarService
 * @calendar: a #GDataCalendarCalendar
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (scope call) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service to return a list of events in the given @calendar, which match @query.
 *
 * For more details, see gdata_service_query().
 *
 * Return value: (transfer full): a #GDataFeed of query results; unref with g_object_unref()
 */
GDataFeed *
gdata_calendar_service_query_events (GDataCalendarService *self, GDataCalendarCalendar *calendar, GDataQuery *query, GCancellable *cancellable,
                                     GDataQueryProgressCallback progress_callback, gpointer progress_user_data, GError **error)
{
	gchar *request_uri;
	GDataFeed *feed;

	g_return_val_if_fail (GDATA_IS_CALENDAR_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_CALENDAR_CALENDAR (calendar), NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Ensure we're authenticated first */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_calendar_authorization_domain ()) == FALSE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
		                     _("You must be authenticated to query your own calendars."));
		return NULL;
	}

	/* Execute the query. */
	request_uri = build_events_uri (calendar);
	feed = gdata_service_query (GDATA_SERVICE (self),
	                            get_calendar_authorization_domain (),
	                            request_uri, query,
	                            GDATA_TYPE_CALENDAR_EVENT, cancellable,
	                            progress_callback, progress_user_data,
	                            error);
	g_free (request_uri);

	return feed;
}

/**
 * gdata_calendar_service_query_events_async:
 * @self: a #GDataCalendarService
 * @calendar: a #GDataCalendarCalendar
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @destroy_progress_user_data: (allow-none): the function to call when @progress_callback will not be called any more, or %NULL. This function will be
 * called with @progress_user_data as a parameter and can be used to free any memory allocated for it.
 * @callback: a #GAsyncReadyCallback to call when the query is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Queries the service to return a list of events in the given @calendar, which match @query. @self, @calendar and @query are all reffed when this
 * function is called, so can safely be unreffed after this function returns.
 *
 * Get the results of the query using gdata_service_query_finish() in the @callback.
 *
 * For more details, see gdata_calendar_service_query_events(), which is the synchronous version of this function, and gdata_service_query_async(),
 * which is the base asynchronous query function.
 *
 * Since: 0.9.1
 */
void
gdata_calendar_service_query_events_async (GDataCalendarService *self, GDataCalendarCalendar *calendar, GDataQuery *query, GCancellable *cancellable,
                                           GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                                           GDestroyNotify destroy_progress_user_data,
                                           GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *request_uri;

	g_return_if_fail (GDATA_IS_CALENDAR_SERVICE (self));
	g_return_if_fail (GDATA_IS_CALENDAR_CALENDAR (calendar));
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	/* Ensure we're authenticated first */
	if (gdata_authorizer_is_authorized_for_domain (gdata_service_get_authorizer (GDATA_SERVICE (self)),
	                                               get_calendar_authorization_domain ()) == FALSE) {
		g_autoptr(GTask) task = NULL;

		task = g_task_new (self, cancellable, callback, user_data);
		g_task_set_source_tag (task, gdata_service_query_async);
		g_task_return_new_error (task, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED, "%s",
		                         _("You must be authenticated to query your own calendars."));

		return;
	}

	/* Execute the query. */
	request_uri = build_events_uri (calendar);
	gdata_service_query_async (GDATA_SERVICE (self),
	                           get_calendar_authorization_domain (),
	                           request_uri, query,
	                           GDATA_TYPE_CALENDAR_EVENT, cancellable,
	                           progress_callback, progress_user_data,
	                           destroy_progress_user_data, callback,
	                           user_data);
	g_free (request_uri);
}

/**
 * gdata_calendar_service_insert_event:
 * @self: a #GDataCalendarService
 * @event: the #GDataCalendarEvent to insert
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Inserts @event by uploading it to the online calendar service.
 *
 * For more details, see gdata_service_insert_entry().
 *
 * Return value: (transfer full): an updated #GDataCalendarEvent, or %NULL; unref with g_object_unref()
 *
 * Since: 0.2.0
 * Deprecated: 0.17.2: Use gdata_calendar_service_insert_calendar_event()
 *   instead to be able to specify the calendar to add the event to; otherwise
 *   the default calendar will be used.
 */
GDataCalendarEvent *
gdata_calendar_service_insert_event (GDataCalendarService *self, GDataCalendarEvent *event, GCancellable *cancellable, GError **error)
{
	gchar *uri;
	GDataEntry *entry;

	g_return_val_if_fail (GDATA_IS_CALENDAR_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (event), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	uri = build_events_uri (NULL);
	entry = gdata_service_insert_entry (GDATA_SERVICE (self), get_calendar_authorization_domain (), uri, GDATA_ENTRY (event), cancellable, error);
	g_free (uri);

	return GDATA_CALENDAR_EVENT (entry);
}

/**
 * gdata_calendar_service_insert_calendar_event:
 * @self: a #GDataCalendarService
 * @calendar: the #GDataCalendarCalendar to insert the event into
 * @event: the #GDataCalendarEvent to insert
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Inserts @event by uploading it to the online calendar service, adding it to
 * the specified @calendar.
 *
 * For more details, see gdata_service_insert_entry().
 *
 * Return value: (transfer full): an updated #GDataCalendarEvent, or %NULL;
 * unref with g_object_unref()
 *
 * Since: 0.17.2
 */
GDataCalendarEvent *
gdata_calendar_service_insert_calendar_event (GDataCalendarService *self,
                                              GDataCalendarCalendar *calendar,
                                              GDataCalendarEvent *event,
                                              GCancellable *cancellable,
                                              GError **error)
{
	gchar *uri;
	GDataEntry *entry;

	g_return_val_if_fail (GDATA_IS_CALENDAR_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_CALENDAR_CALENDAR (calendar), NULL);
	g_return_val_if_fail (GDATA_IS_CALENDAR_EVENT (event), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	uri = build_events_uri (calendar);
	entry = gdata_service_insert_entry (GDATA_SERVICE (self),
	                                    get_calendar_authorization_domain (),
	                                    uri, GDATA_ENTRY (event),
	                                    cancellable, error);
	g_free (uri);

	return GDATA_CALENDAR_EVENT (entry);
}

/**
 * gdata_calendar_service_insert_event_async:
 * @self: a #GDataCalendarService
 * @event: the #GDataCalendarEvent to insert
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when insertion is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Inserts @event by uploading it to the online calendar service. @self and @event are both reffed when this function is called, so can safely be
 * unreffed after this function returns.
 *
 * @callback should call gdata_service_insert_entry_finish() to obtain a #GDataCalendarEvent representing the inserted event and to check for possible
 * errors.
 *
 * For more details, see gdata_calendar_service_insert_event(), which is the synchronous version of this function, and
 * gdata_service_insert_entry_async(), which is the base asynchronous insertion function.
 *
 * Since: 0.8.0
 * Deprecated: 0.17.2: Use
 *   gdata_calendar_service_insert_calendar_event_async() instead to be able to
 *   specify the calendar to add the event to; otherwise the default calendar
 *   will be used.
 */
void
gdata_calendar_service_insert_event_async (GDataCalendarService *self, GDataCalendarEvent *event, GCancellable *cancellable,
                                           GAsyncReadyCallback callback, gpointer user_data)
{
	gchar *uri;

	g_return_if_fail (GDATA_IS_CALENDAR_SERVICE (self));
	g_return_if_fail (GDATA_IS_CALENDAR_EVENT (event));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	uri = build_events_uri (NULL);
	gdata_service_insert_entry_async (GDATA_SERVICE (self), get_calendar_authorization_domain (), uri, GDATA_ENTRY (event), cancellable,
	                                  callback, user_data);
	g_free (uri);
}

/**
 * gdata_calendar_service_insert_calendar_event_async:
 * @self: a #GDataCalendarService
 * @calendar: the #GDataCalendarCalendar to insert the event into
 * @event: the #GDataCalendarEvent to insert
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when insertion is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Inserts @event by uploading it to the online calendar service, adding it to
 * the specified @calendar. @self and @event are both reffed when this function
 * is called, so can safely be unreffed after this function returns.
 *
 * @callback should call gdata_service_insert_entry_finish() to obtain a
 * #GDataCalendarEvent representing the inserted event and to check for possible
 * errors.
 *
 * For more details, see gdata_calendar_service_insert_event(), which is the
 * synchronous version of this function, and gdata_service_insert_entry_async(),
 * which is the base asynchronous insertion function.
 *
 * Since: 0.17.2
 */
void
gdata_calendar_service_insert_calendar_event_async (GDataCalendarService *self,
                                                    GDataCalendarCalendar *calendar,
                                                    GDataCalendarEvent *event,
                                                    GCancellable *cancellable,
                                                    GAsyncReadyCallback callback,
                                                    gpointer user_data)
{
	gchar *uri;

	g_return_if_fail (GDATA_IS_CALENDAR_SERVICE (self));
	g_return_if_fail (GDATA_IS_CALENDAR_EVENT (event));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	uri = build_events_uri (calendar);
	gdata_service_insert_entry_async (GDATA_SERVICE (self),
	                                  get_calendar_authorization_domain (),
	                                  uri, GDATA_ENTRY (event), cancellable,
	                                  callback, user_data);
	g_free (uri);
}
