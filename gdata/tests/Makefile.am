include $(top_srcdir)/gtester.mk

# Helper library.
noinst_LTLIBRARIES = libgdata-test.la

libgdata_test_la_SOURCES = \
	common.c \
	common.h \
	gdata-dummy-authorizer.c \
	gdata-dummy-authorizer.h \
	$(NULL)

libgdata_test_la_LIBADD = \
	$(top_builddir)/gdata/libgdata.la \
	$(GDATA_LIBS) \
	$(UHTTPMOCK_LIBS) \
	$(NULL)

# Flags for the helper library and all test binaries
AM_CPPFLAGS = \
	-I$(top_srcdir)/ \
	-I$(top_srcdir)/gdata \
	-DTEST_FILE_DIR="\"$(top_srcdir)/gdata/tests/\"" \
	-DG_LOG_DOMAIN=\"libgdata\" \
	$(DISABLE_DEPRECATED) \
	$(NULL)
AM_CFLAGS = \
	$(GDK_PIXBUF_CFLAGS) \
	$(WARN_CFLAGS) \
	$(GDATA_CFLAGS) \
	$(GNOME_CFLAGS) \
	$(UHTTPMOCK_CFLAGS) \
	$(NULL)

LDADD = \
	libgdata-test.la \
	$(top_builddir)/gdata/libgdata.la \
	$(GDK_PIXBUF_LIBS) \
	$(GDATA_LIBS) \
	$(GNOME_LIBS) \
	$(UHTTPMOCK_LIBS) \
	$(NULL)

noinst_PROGRAMS = $(TESTS) $(DISABLED_TESTS)

TESTS = \
	general \
	calendar \
	contacts \
	picasaweb \
	tasks \
	perf \
	authorization \
	client-login-authorizer \
	oauth1-authorizer \
	oauth2-authorizer \
	$(NULL)

# FIXME: Temporarily disabled until they are ported for the changes in the v2
# to v3 API transitions.
DISABLED_TESTS = youtube documents streams freebase

# FIXME: Temporarily disabled until https://bugzilla.gnome.org/show_bug.cgi?id=705303 is fixed.
#TESTS += memory

EXTRA_DIST += \
	photo.jpg \
	sample.ogg \
	test.doc \
	test.ods \
	test.odt \
	test.ppt \
	test.xls \
	test_updated.odt \
	test_updated_file.ppt \
	cert.pem \
	key.pem \
	\
	traces/calendar/access-rule-delete \
	traces/calendar/access-rule-get \
	traces/calendar/access-rule-insert \
	traces/calendar/access-rule-update \
	traces/calendar/authentication \
	traces/calendar/authentication-async \
	traces/calendar/authentication-async-cancellation \
	traces/calendar/batch \
	traces/calendar/batch-async \
	traces/calendar/batch-async-cancellation \
	traces/calendar/event-insert \
	traces/calendar/event_insert-async \
	traces/calendar/event_insert-async-cancellation \
	traces/calendar/global-authentication \
	traces/calendar/query-all-calendars \
	traces/calendar/query_all_calendars-async \
	traces/calendar/query_all_calendars-async-cancellation \
	traces/calendar/query-all-calendars-async-progress-closure \
	traces/calendar/query-events \
	traces/calendar/query_events-async \
	traces/calendar/query_events-async-cancellation \
	traces/calendar/query-events-async-progress-closure \
	traces/calendar/query-own-calendars \
	traces/calendar/query_own_calendars-async \
	traces/calendar/query_own_calendars-async-cancellation \
	traces/calendar/query-own-calendars-async-progress-closure \
	traces/calendar/setup-batch-async \
	traces/calendar/setup-query-calendars \
	traces/calendar/setup-query-events \
	traces/calendar/setup-temp-calendar \
	traces/calendar/setup-temp-calendar-acls \
	traces/calendar/teardown-batch-async \
	traces/calendar/teardown-insert-event \
	traces/calendar/teardown-query-calendars \
	traces/calendar/teardown-query-events \
	traces/calendar/teardown-temp-calendar \
	traces/calendar/teardown-temp-calendar-acls \
	\
	traces/client-login-authorizer/client-login-authorizer-authenticate-async \
	traces/client-login-authorizer/client-login-authorizer-authenticate-async-cancellation \
	traces/client-login-authorizer/client-login-authorizer-authenticate-async-multiple-domains \
	traces/client-login-authorizer/client-login-authorizer-authenticate-sync \
	traces/client-login-authorizer/client-login-authorizer-authenticate-sync-bad-password \
	traces/client-login-authorizer/client-login-authorizer-authenticate-sync-cancellation \
	traces/client-login-authorizer/client-login-authorizer-authenticate-sync-multiple-domains \
	traces/client-login-authorizer/setup-client-login-authorizer-data-authenticated \
	\
	traces/contacts/authentication \
	traces/contacts/authentication-async \
	traces/contacts/authentication-async-cancellation \
	traces/contacts/batch \
	traces/contacts/batch-async \
	traces/contacts/batch-async-cancellation \
	traces/contacts/contact-insert \
	traces/contacts/contact-update \
	traces/contacts/global-authentication \
	traces/contacts/group-insert \
	traces/contacts/group_insert-async \
	traces/contacts/group_insert-async-cancellation \
	traces/contacts/photo-add \
	traces/contacts/photo_add-async \
	traces/contacts/photo_add-async-cancellation \
	traces/contacts/photo-delete \
	traces/contacts/photo_delete-async \
	traces/contacts/photo_delete-async-cancellation \
	traces/contacts/photo-get \
	traces/contacts/photo_get-async \
	traces/contacts/photo_get-async-cancellation \
	traces/contacts/query-all-contacts \
	traces/contacts/query_all_contacts-async \
	traces/contacts/query_all_contacts-async-cancellation \
	traces/contacts/query-all-contacts-async-progress-closure \
	traces/contacts/query-all-groups \
	traces/contacts/query_all_groups-async \
	traces/contacts/query_all_groups-async-cancellation \
	traces/contacts/query-all-groups-async-progress-closure \
	traces/contacts/setup-batch-async \
	traces/contacts/setup-query-all-contacts \
	traces/contacts/setup-query-all-groups \
	traces/contacts/setup-temp-contact \
	traces/contacts/setup-temp-contact-with-photo \
	traces/contacts/teardown-batch-async \
	traces/contacts/teardown-insert \
	traces/contacts/teardown-insert-group \
	traces/contacts/teardown-query-all-contacts \
	traces/contacts/teardown-query-all-groups \
	traces/contacts/teardown-temp-contact \
	\
	traces/documents/access-rule-insert \
	traces/documents/authentication \
	traces/documents/authentication-async \
	traces/documents/authentication-async-cancellation \
	traces/documents/batch \
	traces/documents/batch-async \
	traces/documents/batch-async-cancellation \
	traces/documents/copy-document \
	traces/documents/delete-document \
	traces/documents/delete-folder \
	traces/documents/download-document \
	traces/documents/folders-add-to-folder \
	traces/documents/folders_add_to_folder-async \
	traces/documents/folders_add_to_folder-async-cancellation \
	traces/documents/folders-remove-from-folder \
	traces/documents/folders_remove_from_folder-async \
	traces/documents/folders_remove_from_folder-async-cancellation \
	traces/documents/folders_remove_from_folder-async-epilogue \
	traces/documents/global-authentication \
	traces/documents/query-all-documents \
	traces/documents/query_all_documents-async \
	traces/documents/query_all_documents-async-cancellation \
	traces/documents/query-all-documents-async-progress-closure \
	traces/documents/query-all-documents-with-folder \
	traces/documents/setup-batch-async \
	traces/documents/setup-folders-add-to-folder \
	traces/documents/setup-folders-remove-from-folder \
	traces/documents/setup-temp-documents \
	traces/documents/setup-temp-document-spreadsheet \
	traces/documents/setup-temp-folder \
	traces/documents/setup-update-document_content-and-metadata-non-resumable \
	traces/documents/setup-update-document_content-and-metadata-resumable \
	traces/documents/setup-update-document_content-only-non-resumable \
	traces/documents/setup-update-document_content-only-resumable \
	traces/documents/setup-update-document_metadata-only-non-resumable \
	traces/documents/setup-upload-document_content-and-metadata-in-folder-non-resumable-odt-convert \
	traces/documents/setup-upload-document_content-and-metadata-in-folder-resumable-bin-no-convert \
	traces/documents/setup-upload-document_content-and-metadata-in-folder-resumable-odt-convert \
	traces/documents/setup-upload-document_content-and-metadata-in-folder-resumable-odt-no-convert \
	traces/documents/setup-upload-document_content-and-metadata-root-folder-non-resumable-odt-convert \
	traces/documents/setup-upload-document_content-and-metadata-root-folder-resumable-bin-no-convert \
	traces/documents/setup-upload-document_content-and-metadata-root-folder-resumable-odt-convert \
	traces/documents/setup-upload-document_content-and-metadata-root-folder-resumable-odt-no-convert \
	traces/documents/setup-upload-document_content-only-in-folder-non-resumable-odt-convert \
	traces/documents/setup-upload-document_content-only-in-folder-resumable-bin-no-convert \
	traces/documents/setup-upload-document_content-only-in-folder-resumable-odt-convert \
	traces/documents/setup-upload-document_content-only-in-folder-resumable-odt-no-convert \
	traces/documents/setup-upload-document_content-only-root-folder-non-resumable-odt-convert \
	traces/documents/setup-upload-document_content-only-root-folder-resumable-bin-no-convert \
	traces/documents/setup-upload-document_content-only-root-folder-resumable-odt-convert \
	traces/documents/setup-upload-document_content-only-root-folder-resumable-odt-no-convert \
	traces/documents/setup-upload-document_metadata-only-in-folder-non-resumable-odt-convert \
	traces/documents/setup-upload-document_metadata-only-root-folder-non-resumable-odt-convert \
	traces/documents/teardown-batch-async \
	traces/documents/teardown-copy-document \
	traces/documents/teardown-folders-add-to-folder \
	traces/documents/teardown-temp-document \
	traces/documents/teardown-temp-documents \
	traces/documents/teardown-temp-folder \
	traces/documents/teardown-update-document_content-and-metadata-non-resumable \
	traces/documents/teardown-update-document_content-and-metadata-resumable \
	traces/documents/teardown-update-document_content-only-non-resumable \
	traces/documents/teardown-update-document_content-only-resumable \
	traces/documents/teardown-update-document_metadata-only-non-resumable \
	traces/documents/teardown-upload-document_content-and-metadata-in-folder-non-resumable-odt-convert \
	traces/documents/teardown-upload-document_content-and-metadata-in-folder-resumable-bin-no-convert \
	traces/documents/teardown-upload-document_content-and-metadata-in-folder-resumable-odt-convert \
	traces/documents/teardown-upload-document_content-and-metadata-in-folder-resumable-odt-no-convert \
	traces/documents/teardown-upload-document_content-and-metadata-root-folder-non-resumable-odt-convert \
	traces/documents/teardown-upload-document_content-and-metadata-root-folder-resumable-bin-no-convert \
	traces/documents/teardown-upload-document_content-and-metadata-root-folder-resumable-odt-convert \
	traces/documents/teardown-upload-document_content-and-metadata-root-folder-resumable-odt-no-convert \
	traces/documents/teardown-upload-document_content-only-in-folder-non-resumable-odt-convert \
	traces/documents/teardown-upload-document_content-only-in-folder-resumable-bin-no-convert \
	traces/documents/teardown-upload-document_content-only-in-folder-resumable-odt-convert \
	traces/documents/teardown-upload-document_content-only-in-folder-resumable-odt-no-convert \
	traces/documents/teardown-upload-document_content-only-root-folder-non-resumable-odt-convert \
	traces/documents/teardown-upload-document_content-only-root-folder-resumable-bin-no-convert \
	traces/documents/teardown-upload-document_content-only-root-folder-resumable-odt-convert \
	traces/documents/teardown-upload-document_content-only-root-folder-resumable-odt-no-convert \
	traces/documents/teardown-upload-document_metadata-only-in-folder-non-resumable-odt-convert \
	traces/documents/teardown-upload-document_metadata-only-root-folder-non-resumable-odt-convert \
	traces/documents/update_content-and-metadata-non-resumable \
	traces/documents/update_content-and-metadata-resumable \
	traces/documents/update_content-only-non-resumable \
	traces/documents/update_content-only-resumable \
	traces/documents/update_metadata-only-non-resumable \
	traces/documents/upload_content-and-metadata-in-folder-non-resumable-odt-convert \
	traces/documents/upload_content-and-metadata-in-folder-resumable-bin-no-convert \
	traces/documents/upload_content-and-metadata-in-folder-resumable-odt-convert \
	traces/documents/upload_content-and-metadata-in-folder-resumable-odt-no-convert \
	traces/documents/upload_content-and-metadata-root-folder-non-resumable-odt-convert \
	traces/documents/upload_content-and-metadata-root-folder-resumable-bin-no-convert \
	traces/documents/upload_content-and-metadata-root-folder-resumable-odt-convert \
	traces/documents/upload_content-and-metadata-root-folder-resumable-odt-no-convert \
	traces/documents/upload_content-only-in-folder-non-resumable-odt-convert \
	traces/documents/upload_content-only-in-folder-resumable-bin-no-convert \
	traces/documents/upload_content-only-in-folder-resumable-odt-convert \
	traces/documents/upload_content-only-in-folder-resumable-odt-no-convert \
	traces/documents/upload_content-only-root-folder-non-resumable-odt-convert \
	traces/documents/upload_content-only-root-folder-resumable-bin-no-convert \
	traces/documents/upload_content-only-root-folder-resumable-odt-convert \
	traces/documents/upload_content-only-root-folder-resumable-odt-no-convert \
	traces/documents/upload_metadata-only-in-folder-non-resumable-odt-convert \
	traces/documents/upload_metadata-only-root-folder-non-resumable-odt-convert \
	\
	traces/oauth1-authorizer/oauth1-authorizer-interactive-data-bad-credentials \
	traces/oauth1-authorizer/oauth1-authorizer-refresh-authorization \
	traces/oauth1-authorizer/oauth1-authorizer-request-authentication-uri-async \
	traces/oauth1-authorizer/oauth1-authorizer-request-authentication-uri-async-cancellation \
	traces/oauth1-authorizer/oauth1-authorizer-request-authentication-uri-sync \
	traces/oauth1-authorizer/oauth1-authorizer-request-authentication-uri-sync-cancellation \
	traces/oauth1-authorizer/oauth1-authorizer-request-authorization-async \
	traces/oauth1-authorizer/oauth1-authorizer-request-authorization-async-bad-credentials \
	traces/oauth1-authorizer/oauth1-authorizer-request-authorization-async-cancellation \
	traces/oauth1-authorizer/oauth1-authorizer-request-authorization-sync \
	traces/oauth1-authorizer/oauth1-authorizer-request-authorization-sync-bad-credentials \
	traces/oauth1-authorizer/oauth1-authorizer-request-authorization-sync-cancellation \
	traces/oauth1-authorizer/setup-oauth1-authorizer-data-authenticated \
	traces/oauth1-authorizer/setup-oauth1-authorizer-interactive-data \
	\
	traces/oauth2-authorizer/oauth2-authorizer-refresh-authorization-authorized \
	traces/oauth2-authorizer/oauth2-authorizer-refresh-authorization-unauthorized \
	traces/oauth2-authorizer/oauth2-authorizer-request-authorization-async \
	traces/oauth2-authorizer/oauth2-authorizer-request-authorization-async-bad-credentials \
	traces/oauth2-authorizer/oauth2-authorizer-request-authorization-async-cancellation \
	traces/oauth2-authorizer/oauth2-authorizer-request-authorization-sync \
	traces/oauth2-authorizer/oauth2-authorizer-request-authorization-sync-bad-credentials \
	traces/oauth2-authorizer/oauth2-authorizer-request-authorization-sync-cancellation \
	traces/oauth2-authorizer/setup-oauth2-authorizer-data-authenticated \
	\
	traces/picasaweb/authentication \
	traces/picasaweb/authentication-async \
	traces/picasaweb/authentication-async-cancellation \
	traces/picasaweb/comment-delete \
	traces/picasaweb/comment_delete-async \
	traces/picasaweb/comment_delete-async-cancellation \
	traces/picasaweb/comment-insert \
	traces/picasaweb/comment_insert-async \
	traces/picasaweb/comment_insert-async-cancellation \
	traces/picasaweb/comment-query \
	traces/picasaweb/comment_query-async \
	traces/picasaweb/comment_query-async-cancellation \
	traces/picasaweb/comment-query-async-progress-closure \
	traces/picasaweb/download-photo \
	traces/picasaweb/download-thumbnails \
	traces/picasaweb/global-authentication \
	traces/picasaweb/insert-album \
	traces/picasaweb/insert_album-async \
	traces/picasaweb/insert_album-async-cancellation \
	traces/picasaweb/query-all-albums \
	traces/picasaweb/query_all_albums-async \
	traces/picasaweb/query_all_albums-async-cancellation \
	traces/picasaweb/query-all-albums-async-progress-closure \
	traces/picasaweb/query-all-albums-with-limits \
	traces/picasaweb/query-files \
	traces/picasaweb/query_files-async \
	traces/picasaweb/query_files-async-cancellation \
	traces/picasaweb/query-files-async-progress-closure \
	traces/picasaweb/query-files-single \
	traces/picasaweb/query-user \
	traces/picasaweb/query_user-async \
	traces/picasaweb/query_user-async-cancellation \
	traces/picasaweb/query_user_by_username-async \
	traces/picasaweb/query_user_by_username-async-cancellation \
	traces/picasaweb/setup-query-all-albums \
	traces/picasaweb/setup-query-comments \
	traces/picasaweb/setup-query-files \
	traces/picasaweb/setup-upload \
	traces/picasaweb/teardown-insert-album \
	traces/picasaweb/teardown-insert-comment \
	traces/picasaweb/teardown-query-all-albums \
	traces/picasaweb/teardown-query-comments \
	traces/picasaweb/teardown-query-files \
	traces/picasaweb/teardown-upload \
	traces/picasaweb/upload-default-album \
	traces/picasaweb/upload_default_album-async \
	traces/picasaweb/upload_default_album-async-cancellation \
	\
	traces/youtube/authentication \
	traces/youtube/authentication-async \
	traces/youtube/authentication-async-cancellation \
	traces/youtube/batch \
	traces/youtube/batch-async \
	traces/youtube/batch-async-cancellation \
	traces/youtube/categories \
	traces/youtube/categories-async \
	traces/youtube/categories-async-cancellation \
	traces/youtube/comment-delete \
	traces/youtube/comment_delete-async \
	traces/youtube/comment_delete-async-cancellation \
	traces/youtube/comment-insert \
	traces/youtube/comment_insert-async \
	traces/youtube/comment_insert-async-cancellation \
	traces/youtube/comment-query \
	traces/youtube/comment_query-async \
	traces/youtube/comment_query-async-cancellation \
	traces/youtube/comment-query-async-progress-closure \
	traces/youtube/global-authentication \
	traces/youtube/query-related \
	traces/youtube/query_related-async \
	traces/youtube/query_related-async-cancellation \
	traces/youtube/query-related-async-progress-closure \
	traces/youtube/query-single \
	traces/youtube/query_single-async \
	traces/youtube/query_single-async-cancellation \
	traces/youtube/query-standard-feed \
	traces/youtube/query_standard_feed-async \
	traces/youtube/query_standard_feed-async-cancellation \
	traces/youtube/query-standard-feed-async-progress-closure \
	traces/youtube/query-standard-feeds \
	traces/youtube/query-standard-feed-with-query \
	traces/youtube/setup-batch \
	traces/youtube/setup-comment \
	traces/youtube/setup-insert-comment \
	traces/youtube/teardown-insert-comment \
	traces/youtube/teardown-upload \
	traces/youtube/upload-async \
	traces/youtube/upload-async-cancellation \
	traces/youtube/upload-simple \
	\
	traces/tasks/authentication \
	traces/tasks/global-authentication \
	traces/tasks/setup-delete-task \
	traces/tasks/setup-delete-tasklist \
	traces/tasks/setup-insert-task \
	traces/tasks/setup-list-task \
	traces/tasks/setup-list-tasklist \
	traces/tasks/setup-update-task \
	traces/tasks/setup-update-tasklist \
	traces/tasks/task-delete \
	traces/tasks/task-insert \
	traces/tasks/task-list \
	traces/tasks/tasklist-delete \
	traces/tasks/tasklist-insert \
	traces/tasks/tasklist-insert-unauthorised \
	traces/tasks/tasklist-list \
	traces/tasks/tasklist-update \
	traces/tasks/task-update \
	traces/tasks/teardown-delete-task \
	traces/tasks/teardown-insert-task \
	traces/tasks/teardown-insert-tasklist \
	traces/tasks/teardown-list-task \
	traces/tasks/teardown-list-tasklist \
	traces/tasks/teardown-update-task \
	traces/tasks/teardown-update-tasklist \
	$(NULL)

-include $(top_srcdir)/git.mk
