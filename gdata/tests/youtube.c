/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2008–2010 <philip@tecnocode.co.uk>
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

#include <glib.h>
#include <unistd.h>

#include "gdata.h"
#include "common.h"

#define DEVELOPER_KEY "AI39si7Me3Q7zYs6hmkFvpRBD2nrkVjYYsUO5lh_3HdOkGRc9g6Z4nzxZatk_aAo2EsA21k7vrda0OO6oFg2rnhMedZXPyXoEw"
#define YT_USERNAME "GDataTest"

static void
test_authentication (void)
{
	gboolean retval;
	GDataService *service;
	GError *error = NULL;

	/* Create a service */
	service = GDATA_SERVICE (gdata_youtube_service_new (DEVELOPER_KEY, CLIENT_ID));

	g_assert (service != NULL);
	g_assert (GDATA_IS_SERVICE (service));
	g_assert_cmpstr (gdata_service_get_client_id (service), ==, CLIENT_ID);
	g_assert_cmpstr (gdata_youtube_service_get_developer_key (GDATA_YOUTUBE_SERVICE (service)), ==, DEVELOPER_KEY);

	/* Log in */
	retval = gdata_service_authenticate (service, USERNAME, PASSWORD, NULL, &error);
	g_assert_no_error (error);
	g_assert (retval == TRUE);
	g_clear_error (&error);

	/* Check all is as it should be */
	g_assert (gdata_service_is_authenticated (service) == TRUE);
	g_assert_cmpstr (gdata_service_get_username (service), ==, USERNAME);
	g_assert_cmpstr (gdata_service_get_password (service), ==, PASSWORD);
	g_assert_cmpstr (gdata_youtube_service_get_youtube_user (GDATA_YOUTUBE_SERVICE (service)), ==, YT_USERNAME);

	g_object_unref (service);
}

static void
test_authentication_async_cb (GDataService *service, GAsyncResult *async_result, GMainLoop *main_loop)
{
	gboolean retval;
	GError *error = NULL;

	retval = gdata_service_authenticate_finish (service, async_result, &error);
	g_assert_no_error (error);
	g_assert (retval == TRUE);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);

	/* Check all is as it should be */
	g_assert (gdata_service_is_authenticated (service) == TRUE);
	g_assert_cmpstr (gdata_service_get_username (service), ==, USERNAME);
	g_assert_cmpstr (gdata_service_get_password (service), ==, PASSWORD);
	g_assert_cmpstr (gdata_youtube_service_get_youtube_user (GDATA_YOUTUBE_SERVICE (service)), ==, YT_USERNAME);
}

static void
test_authentication_async (void)
{
	GMainLoop *main_loop;
	GDataService *service;

	/* Create a service */
	service = GDATA_SERVICE (gdata_youtube_service_new (DEVELOPER_KEY, CLIENT_ID));

	g_assert (service != NULL);
	g_assert (GDATA_IS_SERVICE (service));

	main_loop = g_main_loop_new (NULL, TRUE);
	gdata_service_authenticate_async (service, USERNAME, PASSWORD, NULL, (GAsyncReadyCallback) test_authentication_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
	g_object_unref (service);
}

static void
test_query_standard_feed (gconstpointer service)
{
	GDataFeed *feed;
	GError *error = NULL;

	feed = gdata_youtube_service_query_standard_feed (GDATA_YOUTUBE_SERVICE (service), GDATA_YOUTUBE_TOP_RATED_FEED, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* TODO: check entries and feed properties */

	g_object_unref (feed);
}

static void
test_query_standard_feed_async_cb (GDataService *service, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GDataFeed *feed;
	GError *error = NULL;

	feed = gdata_service_query_finish (service, async_result, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* TODO: Tests? */
	g_main_loop_quit (main_loop);

	g_object_unref (feed);
}

static void
test_query_standard_feed_async (gconstpointer service)
{
	GMainLoop *main_loop = g_main_loop_new (NULL, TRUE);

	gdata_youtube_service_query_standard_feed_async (GDATA_YOUTUBE_SERVICE (service), GDATA_YOUTUBE_TOP_RATED_FEED, NULL,
							 NULL, NULL, NULL, (GAsyncReadyCallback) test_query_standard_feed_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
}

static GDataYouTubeVideo *
get_video_for_related (void)
{
	GDataYouTubeVideo *video;
	GError *error = NULL;

	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
			"xmlns:media='http://search.yahoo.com/mrss/' "
			"xmlns:yt='http://gdata.youtube.com/schemas/2007' "
			"xmlns:georss='http://www.georss.org/georss' "
			"xmlns:gd='http://schemas.google.com/g/2005' "
			"xmlns:gml='http://www.opengis.net/gml'>"
			"<id>http://gdata.youtube.com/feeds/api/videos/q1UPMEmCqZo</id>"
			"<published>2009-02-12T20:34:08.000Z</published>"
			"<updated>2009-02-21T13:00:13.000Z</updated>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/keywords.cat' term='part one'/>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/categories.cat' term='Film' label='Film &amp; Animation'/>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'/>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/keywords.cat' term='ian purchase'/>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/keywords.cat' term='purchase brothers'/>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/keywords.cat' term='half life 2'/>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/keywords.cat' term='escape from city 17'/>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/keywords.cat' term='Half Life'/>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/keywords.cat' term='david purchase'/>"
			"<category scheme='http://gdata.youtube.com/schemas/2007/keywords.cat' term='half-life'/>"
			"<title type='text'>Escape From City 17 - Part One</title>"
			"<content type='text'>Directed by The Purchase Brothers. *snip*</content>"
			"<link rel='http://www.iana.org/assignments/relation/alternate' type='text/html' href='http://www.youtube.com/watch?v=q1UPMEmCqZo'/>"
			"<link rel='http://gdata.youtube.com/schemas/2007#video.related' type='application/atom+xml' href='http://gdata.youtube.com/feeds/api/videos/q1UPMEmCqZo/related'/>"
			"<link rel='http://gdata.youtube.com/schemas/2007#mobile' type='text/html' href='http://m.youtube.com/details?v=q1UPMEmCqZo'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' href='http://gdata.youtube.com/feeds/api/standardfeeds/top_rated/v/q1UPMEmCqZo'/>"
			"<author>"
				"<name>PurchaseBrothers</name>"
				"<uri>http://gdata.youtube.com/feeds/api/users/purchasebrothers</uri>"
			"</author>"
			"<media:group>"
				"<media:title type='plain'>Escape From City 17 - Part One</media:title>"
				"<media:description type='plain'>Directed by The Purchase Brothers. *snip*</media:description>"
				"<media:keywords>Half Life, escape from city 17, half-life, half life 2, part one, purchase brothers, david purchase, ian purchase</media:keywords>"
				"<yt:duration seconds='330'/>"
				"<media:category label='Film &amp; Animation' scheme='http://gdata.youtube.com/schemas/2007/categories.cat'>Film</media:category>"
				"<media:content url='http://www.youtube.com/v/q1UPMEmCqZo&amp;f=standard&amp;app=youtube_gdata' type='application/x-shockwave-flash' medium='video' isDefault='true' expression='full' duration='330' yt:format='5'/>"
				"<media:content url='rtsp://rtsp2.youtube.com/CiQLENy73wIaGwmaqYJJMA9VqxMYDSANFEgGUghzdGFuZGFyZAw=/0/0/0/video.3gp' type='video/3gpp' medium='video' expression='full' duration='330' yt:format='1'/>"
				"<media:content url='rtsp://rtsp2.youtube.com/CiQLENy73wIaGwmaqYJJMA9VqxMYESARFEgGUghzdGFuZGFyZAw=/0/0/0/video.3gp' type='video/3gpp' medium='video' expression='full' duration='330' yt:format='6'/>"
				"<media:thumbnail url='http://i.ytimg.com/vi/q1UPMEmCqZo/2.jpg' height='97' width='130' time='00:02:45'/>"
				"<media:thumbnail url='http://i.ytimg.com/vi/q1UPMEmCqZo/1.jpg' height='97' width='130' time='00:01:22.500'/>"
				"<media:thumbnail url='http://i.ytimg.com/vi/q1UPMEmCqZo/3.jpg' height='97' width='130' time='00:04:07.500'/>"
				"<media:thumbnail url='http://i.ytimg.com/vi/q1UPMEmCqZo/0.jpg' height='240' width='320' time='00:02:45'/>"
				"<media:player url='http://www.youtube.com/watch?v=q1UPMEmCqZo'/>"
			"</media:group>"
			"<yt:statistics viewCount='1683289' favoriteCount='29963'/>"
			"<gd:rating min='1' max='5' numRaters='24550' average='4.95'/>"
			"<georss:where>"
				"<gml:Point>"
					"<gml:pos>43.661911057260674 -79.37759399414062</gml:pos>"
				"</gml:Point>"
			"</georss:where>"
			"<gd:comments>"
				"<gd:feedLink href='http://gdata.youtube.com/feeds/api/videos/q1UPMEmCqZo/comments' countHint='13021'/>"
			"</gd:comments>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	return video;
}

static void
test_query_related (gconstpointer service)
{
	GDataFeed *feed;
	GDataYouTubeVideo *video;
	GError *error = NULL;

	video = get_video_for_related ();
	feed = gdata_youtube_service_query_related (GDATA_YOUTUBE_SERVICE (service), video, NULL, NULL, NULL, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* TODO: check entries and feed properties */

	g_object_unref (video);
	g_object_unref (feed);
}

static void
test_query_related_async_cb (GDataService *service, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GDataFeed *feed;
	GError *error = NULL;

	feed = gdata_service_query_finish (service, async_result, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_FEED (feed));
	g_clear_error (&error);

	/* TODO: Tests? */
	g_main_loop_quit (main_loop);

	g_object_unref (feed);
}

static void
test_query_related_async (gconstpointer service)
{
	GDataYouTubeVideo *video;
	GMainLoop *main_loop = g_main_loop_new (NULL, TRUE);

	video = get_video_for_related ();
	gdata_youtube_service_query_related_async (GDATA_YOUTUBE_SERVICE (service), video, NULL, NULL, NULL,
						   NULL, (GAsyncReadyCallback) test_query_related_async_cb, main_loop);
	g_object_unref (video);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
}

static void
test_upload_simple (gconstpointer service)
{
	GDataYouTubeVideo *video, *new_video;
	GDataMediaCategory *category;
	GFile *video_file;
	gchar *xml;
	const gchar * const tags[] = { "toast", "wedding", NULL };
	GError *error = NULL;

	video = gdata_youtube_video_new (NULL);

	gdata_entry_set_title (GDATA_ENTRY (video), "Bad Wedding Toast");
	gdata_youtube_video_set_description (video, "I gave a bad toast at my friend's wedding.");
	category = gdata_media_category_new ("People", "http://gdata.youtube.com/schemas/2007/categories.cat", NULL);
	gdata_youtube_video_set_category (video, category);
	g_object_unref (category);
	gdata_youtube_video_set_keywords (video, tags);

	/* Check the XML */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (video));
	g_assert_cmpstr (xml, ==,
			 "<?xml version='1.0' encoding='UTF-8'?>"
			 "<entry xmlns='http://www.w3.org/2005/Atom' "
				"xmlns:media='http://search.yahoo.com/mrss/' "
				"xmlns:gd='http://schemas.google.com/g/2005' "
				"xmlns:yt='http://gdata.youtube.com/schemas/2007' "
				"xmlns:app='http://www.w3.org/2007/app'>"
				"<title type='text'>Bad Wedding Toast</title>"
				"<category term='http://gdata.youtube.com/schemas/2007#video' scheme='http://schemas.google.com/g/2005#kind'/>"
				"<media:group>"
					"<media:category scheme='http://gdata.youtube.com/schemas/2007/categories.cat'>People</media:category>"
					"<media:title type='plain'>Bad Wedding Toast</media:title>"
					"<media:description type='plain'>I gave a bad toast at my friend&apos;s wedding.</media:description>"
					"<media:keywords>toast,wedding</media:keywords>"
				"</media:group>"
				"<app:control>"
					"<app:draft>no</app:draft>"
				"</app:control>"
			 "</entry>");
	g_free (xml);

	/* TODO: fix the path */
	video_file = g_file_new_for_path (TEST_FILE_DIR "sample.ogg");

	/* Upload the video */
	new_video = gdata_youtube_service_upload_video (GDATA_YOUTUBE_SERVICE (service), video, video_file, NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (new_video));
	g_clear_error (&error);

	/* TODO: check entries and feed properties */

	g_object_unref (video);
	g_object_unref (new_video);
	g_object_unref (video_file);
}

static void
test_upload_async_cb (GDataService *service, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GDataYouTubeVideo *new_video;
	GError *error = NULL;

	new_video = gdata_youtube_service_upload_video_finish (GDATA_YOUTUBE_SERVICE (service), async_result, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (new_video));
	g_clear_error (&error);

	g_assert_cmpstr (gdata_entry_get_title (GDATA_ENTRY (new_video)), ==, "Bad Wedding Toast");

	g_main_loop_quit (main_loop);
	g_object_unref (new_video);
}

static void
test_upload_async (gconstpointer service)
{
	GDataYouTubeVideo *video;
	GDataMediaCategory *category;
	GFile *video_file;
	const gchar * const tags[] = { "toast", "wedding", NULL };
	GMainLoop *main_loop;

	main_loop = g_main_loop_new (NULL, TRUE);

	video = gdata_youtube_video_new (NULL);

	gdata_entry_set_title (GDATA_ENTRY (video), "Bad Wedding Toast");
	gdata_youtube_video_set_description (video, "I gave a bad toast at my friend's wedding.");
	category = gdata_media_category_new ("People", "http://gdata.youtube.com/schemas/2007/categories.cat", NULL);
	gdata_youtube_video_set_category (video, category);
	g_object_unref (category);
	gdata_youtube_video_set_keywords (video, tags);

	/* TODO: fix the path */
	video_file = g_file_new_for_path (TEST_FILE_DIR "sample.ogg");

	/* Upload the video */
	gdata_youtube_service_upload_video_async (GDATA_YOUTUBE_SERVICE (service), video, video_file, NULL,
	                                          (GAsyncReadyCallback) test_upload_async_cb, main_loop);

	g_object_unref (video);
	g_object_unref (video_file);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
}


static void
test_parsing_app_control (void)
{
	GDataYouTubeVideo *video;
	GDataYouTubeState *state;
	GError *error = NULL;

	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
			"xmlns:media='http://search.yahoo.com/mrss/' "
			"xmlns:yt='http://gdata.youtube.com/schemas/2007' "
			"xmlns:gd='http://schemas.google.com/g/2005' "
			"gd:etag='W/\"CEMFSX47eCp7ImA9WxVUGEw.\"'>"
			"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
			"<published>2006-05-16T14:06:37.000Z</published>"
			"<updated>2009-03-23T12:46:58.000Z</updated>"
			"<app:control xmlns:app='http://www.w3.org/2007/app'>"
				"<app:draft>yes</app:draft>"
				"<yt:state name='blacklisted'>This video is not available in your country</yt:state>"
			"</app:control>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'/>"
			"<title>Judas Priest - Painkiller</title>"
			"<link rel='http://www.iana.org/assignments/relation/alternate' type='text/html' href='http://www.youtube.com/watch?v=JAagedeKdcQ'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ?client=ytapi-google-jsdemo'/>"
			"<author>"
				"<name>eluves</name>"
				"<uri>http://gdata.youtube.com/feeds/api/users/eluves</uri>"
			"</author>"
			"<media:group>"
				"<media:title type='plain'>Judas Priest - Painkiller</media:title>"
				"<media:credit role='uploader' scheme='urn:youtube'>eluves</media:credit>"
				"<media:category label='Music' scheme='http://gdata.youtube.com/schemas/2007/categories.cat'>Music</media:category>"
			"</media:group>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	/* Test the app:control values */
	g_assert (gdata_youtube_video_is_draft (video) == TRUE);

	state = gdata_youtube_video_get_state (video);
	g_assert_cmpstr (gdata_youtube_state_get_name (state), ==, "blacklisted");
	g_assert_cmpstr (gdata_youtube_state_get_message (state), ==, "This video is not available in your country");
	g_assert (gdata_youtube_state_get_reason_code (state) == NULL);
	g_assert (gdata_youtube_state_get_help_uri (state) == NULL);

	/* TODO: more tests on entry properties */

	g_object_unref (video);
}

static void
test_parsing_yt_recorded (void)
{
	GDataYouTubeVideo *video;
	gint64 recorded;
	gchar *xml;
	GError *error = NULL;

	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
			"xmlns:media='http://search.yahoo.com/mrss/' "
			"xmlns:yt='http://gdata.youtube.com/schemas/2007' "
			"xmlns:gd='http://schemas.google.com/g/2005' "
			"gd:etag='W/\"CEMFSX47eCp7ImA9WxVUGEw.\"'>"
			"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
			"<published>2006-05-16T14:06:37.000Z</published>"
			"<updated>2009-03-23T12:46:58.000Z</updated>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'/>"
			"<title>Judas Priest - Painkiller</title>"
			"<link rel='http://www.iana.org/assignments/relation/alternate' type='text/html' href='http://www.youtube.com/watch?v=JAagedeKdcQ'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ?client=ytapi-google-jsdemo'/>"
			"<author>"
				"<name>eluves</name>"
				"<uri>http://gdata.youtube.com/feeds/api/users/eluves</uri>"
			"</author>"
			"<media:group>"
				"<media:title type='plain'>Judas Priest - Painkiller</media:title>"
				"<media:credit role='uploader' scheme='urn:youtube'>eluves</media:credit>"
				"<media:category label='Music' scheme='http://gdata.youtube.com/schemas/2007/categories.cat'>Music</media:category>"
			"</media:group>"
			"<yt:recorded>2003-08-03</yt:recorded>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	/* Test the recorded date */
	recorded = gdata_youtube_video_get_recorded (video);
	g_assert_cmpint (recorded, ==, 1059868800);

	/* Update the state and see if the XML's written out OK */
	recorded = 1128229200;
	gdata_youtube_video_set_recorded (video, recorded);

	/* Check the XML */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (video));
	g_assert_cmpstr (xml, ==,
			 "<?xml version='1.0' encoding='UTF-8'?>"
			 "<entry xmlns='http://www.w3.org/2005/Atom' "
				"xmlns:media='http://search.yahoo.com/mrss/' "
				"xmlns:gd='http://schemas.google.com/g/2005' "
				"xmlns:yt='http://gdata.youtube.com/schemas/2007' "
				"xmlns:app='http://www.w3.org/2007/app' "
				"gd:etag='W/\"CEMFSX47eCp7ImA9WxVUGEw.\"'>"
				"<title type='text'>Judas Priest - Painkiller</title>"
				"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
				"<updated>2009-03-23T12:46:58Z</updated>"
				"<published>2006-05-16T14:06:37Z</published>"
				"<category term='http://gdata.youtube.com/schemas/2007#video' scheme='http://schemas.google.com/g/2005#kind'/>"
				"<link href='http://www.youtube.com/watch?v=JAagedeKdcQ' rel='http://www.iana.org/assignments/relation/alternate' type='text/html'/>"
				"<link href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ?client=ytapi-google-jsdemo' rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml'/>"
				"<author>"
					"<name>eluves</name>"
					"<uri>http://gdata.youtube.com/feeds/api/users/eluves</uri>"
				"</author>"
				"<media:group>"
					"<media:category scheme='http://gdata.youtube.com/schemas/2007/categories.cat' label='Music'>Music</media:category>"
					"<media:title type='plain'>Judas Priest - Painkiller</media:title>"
				"</media:group>"
				"<yt:recorded>2005-10-02</yt:recorded>"
				"<app:control>"
					"<app:draft>no</app:draft>"
				"</app:control>"
			 "</entry>");
	g_free (xml);

	/* TODO: more tests on entry properties */

	g_object_unref (video);
}

static void
test_parsing_yt_access_control (void)
{
	GDataYouTubeVideo *video;
	gchar *xml;
	GError *error = NULL;

	video = GDATA_YOUTUBE_VIDEO (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
			"xmlns:media='http://search.yahoo.com/mrss/' "
			"xmlns:yt='http://gdata.youtube.com/schemas/2007' "
			"xmlns:gd='http://schemas.google.com/g/2005' "
			"gd:etag='W/\"CEMFSX47eCp7ImA9WxVUGEw.\"'>"
			"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
			"<published>2006-05-16T14:06:37.000Z</published>"
			"<updated>2009-03-23T12:46:58.000Z</updated>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'/>"
			"<title>Judas Priest - Painkiller</title>"
			"<link rel='http://www.iana.org/assignments/relation/alternate' type='text/html' href='http://www.youtube.com/watch?v=JAagedeKdcQ'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ?client=ytapi-google-jsdemo'/>"
			"<author>"
				"<name>eluves</name>"
				"<uri>http://gdata.youtube.com/feeds/api/users/eluves</uri>"
			"</author>"
			"<media:group>"
				"<media:title type='plain'>Judas Priest - Painkiller</media:title>"
				"<media:credit role='uploader' scheme='urn:youtube'>eluves</media:credit>"
				"<media:category label='Music' scheme='http://gdata.youtube.com/schemas/2007/categories.cat'>Music</media:category>"
			"</media:group>"
			"<yt:accessControl action='rate' permission='allowed'/>"
			"<yt:accessControl action='comment' permission='moderated'/>"
			"<yt:accessControl action='commentVote' permission='denied'/>"
			"<yt:accessControl action='videoRespond' permission='allowed'/>"
			"<yt:accessControl action='syndicate' permission='denied'/>"
			"<yt:accessControl action='random' permission='moderated'/>"
		"</entry>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	/* Test the access controls */
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_RATE), ==, GDATA_YOUTUBE_PERMISSION_ALLOWED);
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_COMMENT), ==, GDATA_YOUTUBE_PERMISSION_MODERATED);
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_COMMENT_VOTE), ==, GDATA_YOUTUBE_PERMISSION_DENIED);
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_VIDEO_RESPOND), ==, GDATA_YOUTUBE_PERMISSION_ALLOWED);
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_EMBED), ==, GDATA_YOUTUBE_PERMISSION_DENIED);
	g_assert_cmpint (gdata_youtube_video_get_access_control (video, GDATA_YOUTUBE_ACTION_SYNDICATE), ==, GDATA_YOUTUBE_PERMISSION_DENIED);

	/* Update some of them and see if the XML's written out OK */
	gdata_youtube_video_set_access_control (video, GDATA_YOUTUBE_ACTION_RATE, GDATA_YOUTUBE_PERMISSION_MODERATED);
	gdata_youtube_video_set_access_control (video, GDATA_YOUTUBE_ACTION_EMBED, GDATA_YOUTUBE_PERMISSION_DENIED);

	/* Check the XML */
	xml = gdata_parsable_get_xml (GDATA_PARSABLE (video));
	g_assert_cmpstr (xml, ==,
			 "<?xml version='1.0' encoding='UTF-8'?>"
			 "<entry xmlns='http://www.w3.org/2005/Atom' "
				"xmlns:media='http://search.yahoo.com/mrss/' "
				"xmlns:gd='http://schemas.google.com/g/2005' "
				"xmlns:yt='http://gdata.youtube.com/schemas/2007' "
				"xmlns:app='http://www.w3.org/2007/app' "
				"gd:etag='W/\"CEMFSX47eCp7ImA9WxVUGEw.\"'>"
				"<title type='text'>Judas Priest - Painkiller</title>"
				"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
				"<updated>2009-03-23T12:46:58Z</updated>"
				"<published>2006-05-16T14:06:37Z</published>"
				"<category term='http://gdata.youtube.com/schemas/2007#video' scheme='http://schemas.google.com/g/2005#kind'/>"
				"<link href='http://www.youtube.com/watch?v=JAagedeKdcQ' rel='http://www.iana.org/assignments/relation/alternate' type='text/html'/>"
				"<link href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ?client=ytapi-google-jsdemo' rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml'/>"
				"<author>"
					"<name>eluves</name>"
					"<uri>http://gdata.youtube.com/feeds/api/users/eluves</uri>"
				"</author>"
				"<media:group>"
					"<media:category scheme='http://gdata.youtube.com/schemas/2007/categories.cat' label='Music'>Music</media:category>"
					"<media:title type='plain'>Judas Priest - Painkiller</media:title>"
				"</media:group>"
				"<yt:accessControl action='embed' permission='denied'/>"
				"<yt:accessControl action='random' permission='moderated'/>"
				"<yt:accessControl action='commentVote' permission='denied'/>"
				"<yt:accessControl action='rate' permission='moderated'/>"
				"<yt:accessControl action='comment' permission='moderated'/>"
				"<yt:accessControl action='syndicate' permission='denied'/>"
				"<yt:accessControl action='videoRespond' permission='allowed'/>"
				"<app:control>"
					"<app:draft>no</app:draft>"
				"</app:control>"
			 "</entry>");
	g_free (xml);

	g_object_unref (video);
}

static void
test_parsing_yt_category (void)
{
	GDataYouTubeCategory *category;
	gboolean assignable, deprecated;
	GError *error = NULL;

	/* Test a non-deprecated category */
	category = GDATA_YOUTUBE_CATEGORY (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_CATEGORY,
		"<category xmlns='http://www.w3.org/2005/Atom' xmlns:yt='http://gdata.youtube.com/schemas/2007' "
			"scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'>"
			"<yt:assignable/>"
			"<yt:browsable regions='CZ AU HK'/>"
		"</category>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_CATEGORY (category));
	g_clear_error (&error);

	/* Test the category's properties */
	g_assert (gdata_youtube_category_is_assignable (category) == TRUE);
	g_assert (gdata_youtube_category_is_browsable (category, "CZ") == TRUE);
	g_assert (gdata_youtube_category_is_browsable (category, "AU") == TRUE);
	g_assert (gdata_youtube_category_is_browsable (category, "HK") == TRUE);
	g_assert (gdata_youtube_category_is_browsable (category, "GB") == FALSE);
	g_assert (gdata_youtube_category_is_deprecated (category) == FALSE);

	/* Test the properties the other way */
	g_object_get (category, "is-assignable", &assignable, "is-deprecated", &deprecated, NULL);
	g_assert (assignable == TRUE);
	g_assert (deprecated == FALSE);

	g_object_unref (category);

	/* Test a deprecated category */
	category = GDATA_YOUTUBE_CATEGORY (gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_CATEGORY,
		"<category xmlns='http://www.w3.org/2005/Atom' xmlns:yt='http://gdata.youtube.com/schemas/2007' "
			"scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'>"
			"<yt:deprecated/>"
		"</category>", -1, &error));
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_CATEGORY (category));
	g_clear_error (&error);

	/* Test the category's properties */
	g_assert (gdata_youtube_category_is_assignable (category) == FALSE);
	g_assert (gdata_youtube_category_is_browsable (category, "CZ") == FALSE);
	g_assert (gdata_youtube_category_is_browsable (category, "AU") == FALSE);
	g_assert (gdata_youtube_category_is_browsable (category, "HK") == FALSE);
	g_assert (gdata_youtube_category_is_browsable (category, "GB") == FALSE);
	g_assert (gdata_youtube_category_is_deprecated (category) == TRUE);

	g_object_unref (category);
}

/*static void
test_parsing_comments_feed_link (void)
{
	GDataYouTubeVideo *video;
	GDataGDFeedLink *feed_link;
	GError *error = NULL;

	video = gdata_parsable_new_from_xml (GDATA_TYPE_YOUTUBE_VIDEO,
		"<entry xmlns='http://www.w3.org/2005/Atom' "
			"xmlns:media='http://search.yahoo.com/mrss/' "
			"xmlns:yt='http://gdata.youtube.com/schemas/2007' "
			"xmlns:gd='http://schemas.google.com/g/2005' "
			"gd:etag='W/\"CEMFSX47eCp7ImA9WxVUGEw.\"'>"
			"<id>tag:youtube.com,2008:video:JAagedeKdcQ</id>"
			"<published>2006-05-16T14:06:37.000Z</published>"
			"<updated>2009-03-23T12:46:58.000Z</updated>"
			"<category scheme='http://schemas.google.com/g/2005#kind' term='http://gdata.youtube.com/schemas/2007#video'/>"
			"<title>Judas Priest - Painkiller</title>"
			"<link rel='http://www.iana.org/assignments/relation/alternate' type='text/html' href='http://www.youtube.com/watch?v=JAagedeKdcQ'/>"
			"<link rel='http://www.iana.org/assignments/relation/self' type='application/atom+xml' href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ?client=ytapi-google-jsdemo'/>"
			"<author>"
				"<name>eluves</name>"
				"<uri>http://gdata.youtube.com/feeds/api/users/eluves</uri>"
			"</author>"
			"<media:group>"
				"<media:title type='plain'>Judas Priest - Painkiller</media:title>"
				"<media:credit role='uploader' scheme='urn:youtube'>eluves</media:credit>"
				"<media:category label='Music' scheme='http://gdata.youtube.com/schemas/2007/categories.cat'>Music</media:category>"
			"</media:group>"
			"<gd:comments>"
				"<gd:feedLink href='http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ/comments' countHint='13021'/>"
			"</gd:comments>"
		"</entry>", -1, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_clear_error (&error);

	* Test the feed link *
	feed_link = gdata_youtube_video_get_comments_feed_link (video);
	g_assert (feed_link != NULL);
	g_assert (feed_link->rel == NULL);
	g_assert_cmpstr (feed_link->href, ==, "http://gdata.youtube.com/feeds/api/videos/JAagedeKdcQ/comments");
	g_assert_cmpuint (feed_link->count_hint, ==, 13021);
	g_assert (feed_link->read_only == FALSE);

	* TODO: more tests on entry properties *

	g_object_unref (video);
}*/

static void
test_query_uri (void)
{
	gdouble latitude, longitude, radius;
	gboolean has_location;
	gchar *query_uri;
	GDataYouTubeQuery *query = gdata_youtube_query_new ("q");

	gdata_youtube_query_set_format (query, GDATA_YOUTUBE_FORMAT_RTSP_H263_AMR);
	g_assert_cmpuint (gdata_youtube_query_get_format (query), ==, 1);

	/* Location */
	gdata_youtube_query_set_location (query, 45.01364, -97.12356, 112.5, TRUE);
	gdata_youtube_query_get_location (query, &latitude, &longitude, &radius, &has_location);

	g_assert_cmpfloat (latitude, ==, 45.01364);
	g_assert_cmpfloat (longitude, ==, -97.12356);
	g_assert_cmpfloat (radius, ==, 112.5);
	g_assert (has_location == TRUE);

	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&time=all_time&safeSearch=none&format=1&location=45.013640000000002,-97.123559999999998!&location-radius=112.5m");
	g_free (query_uri);

	gdata_youtube_query_set_location (query, G_MAXDOUBLE, 0.6672, 52.8, TRUE);

	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&time=all_time&safeSearch=none&format=1&location=!");
	g_free (query_uri);

	gdata_youtube_query_set_location (query, G_MAXDOUBLE, G_MAXDOUBLE, 0.0, FALSE);

	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&time=all_time&safeSearch=none&format=1");
	g_free (query_uri);

	/* Language */
	gdata_youtube_query_set_language (query, "fr");
	g_assert_cmpstr (gdata_youtube_query_get_language (query), ==, "fr");

	gdata_youtube_query_set_order_by (query, "relevance_lang_fr");
	g_assert_cmpstr (gdata_youtube_query_get_order_by (query), ==, "relevance_lang_fr");

	gdata_youtube_query_set_restriction (query, "192.168.0.1");
	g_assert_cmpstr (gdata_youtube_query_get_restriction (query), ==, "192.168.0.1");

	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&time=all_time&safeSearch=none&format=1&lr=fr&orderby=relevance_lang_fr&restriction=192.168.0.1");
	g_free (query_uri);

	gdata_youtube_query_set_safe_search (query, GDATA_YOUTUBE_SAFE_SEARCH_STRICT);
	g_assert_cmpuint (gdata_youtube_query_get_safe_search (query), ==, GDATA_YOUTUBE_SAFE_SEARCH_STRICT);

	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&time=all_time&safeSearch=strict&format=1&lr=fr&orderby=relevance_lang_fr&restriction=192.168.0.1");
	g_free (query_uri);

	gdata_youtube_query_set_sort_order (query, GDATA_YOUTUBE_SORT_ASCENDING);
	g_assert_cmpuint (gdata_youtube_query_get_sort_order (query), ==, GDATA_YOUTUBE_SORT_ASCENDING);

	gdata_youtube_query_set_age (query, GDATA_YOUTUBE_AGE_THIS_WEEK);
	g_assert_cmpuint (gdata_youtube_query_get_age (query), ==, GDATA_YOUTUBE_AGE_THIS_WEEK);

	gdata_youtube_query_set_uploader (query, GDATA_YOUTUBE_UPLOADER_PARTNER);
	g_assert_cmpuint (gdata_youtube_query_get_uploader (query), ==, GDATA_YOUTUBE_UPLOADER_PARTNER);

	/* Check the built URI with a normal feed URI */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com");
	g_assert_cmpstr (query_uri, ==, "http://example.com?q=q&time=this_week&safeSearch=strict&format=1&lr=fr&orderby=relevance_lang_fr&restriction=192.168.0.1&sortorder=ascending&uploader=partner");
	g_free (query_uri);

	/* …and with a feed URI with pre-existing arguments */
	query_uri = gdata_query_get_query_uri (GDATA_QUERY (query), "http://example.com?foobar=shizzle");
	g_assert_cmpstr (query_uri, ==, "http://example.com?foobar=shizzle&q=q&time=this_week&safeSearch=strict&format=1&lr=fr&orderby=relevance_lang_fr&restriction=192.168.0.1&sortorder=ascending&uploader=partner");
	g_free (query_uri);

	g_object_unref (query);
}

static void
test_query_etag (void)
{
	GDataYouTubeQuery *query = gdata_youtube_query_new (NULL);

	/* Test that setting any property will unset the ETag */
	g_test_bug ("613529");

#define CHECK_ETAG(C) \
	gdata_query_set_etag (GDATA_QUERY (query), "foobar");		\
	(C);								\
	g_assert (gdata_query_get_etag (GDATA_QUERY (query)) == NULL);

	CHECK_ETAG (gdata_youtube_query_set_format (query, GDATA_YOUTUBE_FORMAT_RTSP_H263_AMR))
	CHECK_ETAG (gdata_youtube_query_set_location (query, 0.0, 65.0, 15.0, TRUE))
	CHECK_ETAG (gdata_youtube_query_set_language (query, "British English"))
	CHECK_ETAG (gdata_youtube_query_set_order_by (query, "shizzle"))
	CHECK_ETAG (gdata_youtube_query_set_restriction (query, "restriction"))
	CHECK_ETAG (gdata_youtube_query_set_safe_search (query, GDATA_YOUTUBE_SAFE_SEARCH_MODERATE))
	CHECK_ETAG (gdata_youtube_query_set_sort_order (query, GDATA_YOUTUBE_SORT_DESCENDING))
	CHECK_ETAG (gdata_youtube_query_set_age (query, GDATA_YOUTUBE_AGE_THIS_WEEK))
	CHECK_ETAG (gdata_youtube_query_set_uploader (query, GDATA_YOUTUBE_UPLOADER_PARTNER))

#undef CHECK_ETAG

	g_object_unref (query);
}

static void
test_query_single (gconstpointer service)
{
	GDataYouTubeVideo *video;
	GError *error = NULL;

	video = GDATA_YOUTUBE_VIDEO (gdata_service_query_single_entry (GDATA_SERVICE (service), "tag:youtube.com,2008:video:_LeQuMpwbW4", NULL,
	                                                               GDATA_TYPE_YOUTUBE_VIDEO, NULL, &error));

	g_assert_no_error (error);
	g_assert (video != NULL);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_assert_cmpstr (gdata_youtube_video_get_video_id (video), ==, "_LeQuMpwbW4");
	g_assert_cmpstr (gdata_entry_get_id (GDATA_ENTRY (video)), ==, "tag:youtube.com,2008:video:_LeQuMpwbW4");
	g_clear_error (&error);

	g_object_unref (video);
}

static void
test_query_single_async_cb (GDataService *service, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GDataYouTubeVideo *video;
	GError *error = NULL;

	video = GDATA_YOUTUBE_VIDEO (gdata_service_query_single_entry_finish (GDATA_SERVICE (service), async_result, &error));

	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));
	g_assert_cmpstr (gdata_youtube_video_get_video_id (video), ==, "_LeQuMpwbW4");
	g_assert_cmpstr (gdata_entry_get_id (GDATA_ENTRY (video)), ==, "tag:youtube.com,2008:video:_LeQuMpwbW4");
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
	g_object_unref (video);
}

static void
test_query_single_async (gconstpointer service)
{
	GMainLoop *main_loop = g_main_loop_new (NULL, TRUE);

	gdata_service_query_single_entry_async (GDATA_SERVICE (service), "tag:youtube.com,2008:video:_LeQuMpwbW4", NULL, GDATA_TYPE_YOUTUBE_VIDEO,
	                                        NULL, (GAsyncReadyCallback) test_query_single_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
}

static void
test_parsing_video_id_from_uri (void)
{
	gchar *video_id;

	video_id = gdata_youtube_video_get_video_id_from_uri ("http://www.youtube.com/watch?v=BH_vwsyCrTc&feature=featured");
	g_assert_cmpstr (video_id, ==, "BH_vwsyCrTc");
	g_free (video_id);

	video_id = gdata_youtube_video_get_video_id_from_uri ("http://www.youtube.es/watch?v=foo");
	g_assert_cmpstr (video_id, ==, "foo");
	g_free (video_id);

	video_id = gdata_youtube_video_get_video_id_from_uri ("http://foobar.com/watch?v=foo");
	g_assert (video_id == NULL);

	video_id = gdata_youtube_video_get_video_id_from_uri ("http://foobar.com/not/real");
	g_assert (video_id == NULL);

	video_id = gdata_youtube_video_get_video_id_from_uri ("http://www.youtube.com/watch#!v=ylLzyHk54Z0");
	g_assert_cmpstr (video_id, ==, "ylLzyHk54Z0");
	g_free (video_id);

	video_id = gdata_youtube_video_get_video_id_from_uri ("http://www.youtube.com/watch#!foo=bar!v=ylLzyHk54Z0");
	g_assert_cmpstr (video_id, ==, "ylLzyHk54Z0");
	g_free (video_id);

	video_id = gdata_youtube_video_get_video_id_from_uri ("http://www.youtube.com/watch#!foo=bar");
	g_assert (video_id == NULL);

	video_id = gdata_youtube_video_get_video_id_from_uri ("http://www.youtube.com/watch#random-fragment");
	g_assert (video_id == NULL);
}

static void
test_categories (gconstpointer service)
{
	GDataAPPCategories *app_categories;
	GList *categories;
	GError *error = NULL;
	gchar *category_label, *old_locale;

	app_categories = gdata_youtube_service_get_categories (GDATA_YOUTUBE_SERVICE (service), NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_APP_CATEGORIES (app_categories));
	g_clear_error (&error);

	categories = gdata_app_categories_get_categories (app_categories);
	g_assert_cmpint (g_list_length (categories), >, 0);
	g_assert (GDATA_IS_YOUTUBE_CATEGORY (categories->data));

	/* Save a label for comparison against a different locale */
	category_label = g_strdup (gdata_category_get_label (GDATA_CATEGORY (categories->data)));

	g_object_unref (app_categories);

	/* Test with a different locale */
	old_locale = g_strdup (gdata_service_get_locale (GDATA_SERVICE (service)));
	gdata_service_set_locale (GDATA_SERVICE (service), "it");

	app_categories = gdata_youtube_service_get_categories (GDATA_YOUTUBE_SERVICE (service), NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_APP_CATEGORIES (app_categories));
	g_clear_error (&error);

	categories = gdata_app_categories_get_categories (app_categories);
	g_assert_cmpint (g_list_length (categories), >, 0);
	g_assert (GDATA_IS_YOUTUBE_CATEGORY (categories->data));

	/* Compare the labels */
	g_assert_cmpstr (category_label, !=, gdata_category_get_label (GDATA_CATEGORY (categories->data)));

	g_object_unref (app_categories);
	g_free (category_label);

	/* Reset the locale */
	gdata_service_set_locale (GDATA_SERVICE (service), old_locale);
	g_free (old_locale);
}

static void
test_categories_async_cb (GDataService *service, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GDataAPPCategories *app_categories;
	GList *categories;
	GError *error = NULL;

	app_categories = gdata_youtube_service_get_categories_finish (GDATA_YOUTUBE_SERVICE (service), async_result, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_APP_CATEGORIES (app_categories));
	g_clear_error (&error);

	categories = gdata_app_categories_get_categories (app_categories);
	g_assert_cmpint (g_list_length (categories), >, 0);
	g_assert (GDATA_IS_YOUTUBE_CATEGORY (categories->data));

	g_main_loop_quit (main_loop);
	g_object_unref (app_categories);
}

static void
test_categories_async (gconstpointer service)
{
	GMainLoop *main_loop = g_main_loop_new (NULL, TRUE);

	gdata_youtube_service_get_categories_async (GDATA_YOUTUBE_SERVICE (service), NULL, (GAsyncReadyCallback) test_categories_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
}

typedef struct {
	GDataEntry *new_video;
	GDataEntry *new_video2;
} BatchData;

static void
setup_batch (BatchData *data, gconstpointer service)
{
	GDataEntry *video;
	GError *error = NULL;

	/* We can't insert new videos as they'd just hit the moderation queue and cause tests to fail. Instead, we rely on two videos already existing
	 * on the server with the given IDs. */
	video = gdata_service_query_single_entry (GDATA_SERVICE (service), "tag:youtube.com,2008:video:RzR2k8yo4NY", NULL, GDATA_TYPE_YOUTUBE_VIDEO,
	                                          NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));

	data->new_video = video;

	video = gdata_service_query_single_entry (GDATA_SERVICE (service), "tag:youtube.com,2008:video:VppEcVz8qaI", NULL, GDATA_TYPE_YOUTUBE_VIDEO,
	                                          NULL, &error);
	g_assert_no_error (error);
	g_assert (GDATA_IS_YOUTUBE_VIDEO (video));

	data->new_video2 = video;
}

static void
test_batch (BatchData *data, gconstpointer service)
{
	GDataBatchOperation *operation;
	GDataService *service2;
	gchar *feed_uri;
	guint op_id, op_id2;
	GError *error = NULL;

	/* Here we hardcode the feed URI, but it should really be extracted from a video feed, as the GDATA_LINK_BATCH link.
	 * It looks like this feed is read-only, so we can only test querying. */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "http://gdata.youtube.com/feeds/api/videos/batch");

	/* Check the properties of the operation */
	g_assert (gdata_batch_operation_get_service (operation) == service);
	g_assert_cmpstr (gdata_batch_operation_get_feed_uri (operation), ==, "http://gdata.youtube.com/feeds/api/videos/batch");

	g_object_get (operation,
	              "service", &service2,
	              "feed-uri", &feed_uri,
	              NULL);

	g_assert (service2 == service);
	g_assert_cmpstr (feed_uri, ==, "http://gdata.youtube.com/feeds/api/videos/batch");

	g_object_unref (service2);
	g_free (feed_uri);

	/* Run a singleton batch operation to query one of the entries */
	gdata_test_batch_operation_query (operation, gdata_entry_get_id (data->new_video), GDATA_TYPE_YOUTUBE_VIDEO, data->new_video, NULL, NULL);

	g_assert (gdata_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);

	/* Run another batch operation to query the two entries */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "http://gdata.youtube.com/feeds/api/videos/batch");
	op_id = gdata_test_batch_operation_query (operation, gdata_entry_get_id (data->new_video), GDATA_TYPE_YOUTUBE_VIDEO, data->new_video, NULL,
	                                          NULL);
	op_id2 = gdata_test_batch_operation_query (operation, gdata_entry_get_id (data->new_video2), GDATA_TYPE_YOUTUBE_VIDEO, data->new_video2, NULL,
	                                           NULL);
	g_assert_cmpuint (op_id, !=, op_id2);

	g_assert (gdata_batch_operation_run (operation, NULL, &error) == TRUE);
	g_assert_no_error (error);

	g_clear_error (&error);
	g_object_unref (operation);
}

static void
test_batch_async_cb (GDataBatchOperation *operation, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GError *error = NULL;

	g_assert (gdata_batch_operation_run_finish (operation, async_result, &error) == TRUE);
	g_assert_no_error (error);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
}

static void
test_batch_async (BatchData *data, gconstpointer service)
{
	GDataBatchOperation *operation;
	guint op_id;
	GMainLoop *main_loop;

	/* Run an async query operation on the video */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "http://gdata.youtube.com/feeds/api/videos/batch");
	op_id = gdata_test_batch_operation_query (operation, gdata_entry_get_id (data->new_video), GDATA_TYPE_YOUTUBE_VIDEO, data->new_video, NULL,
	                                          NULL);

	main_loop = g_main_loop_new (NULL, TRUE);

	gdata_batch_operation_run_async (operation, NULL, (GAsyncReadyCallback) test_batch_async_cb, main_loop);

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
}

static void
test_batch_async_cancellation_cb (GDataBatchOperation *operation, GAsyncResult *async_result, GMainLoop *main_loop)
{
	GError *error = NULL;

	g_assert (gdata_batch_operation_run_finish (operation, async_result, &error) == FALSE);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
	g_clear_error (&error);

	g_main_loop_quit (main_loop);
}

static void
test_batch_async_cancellation (BatchData *data, gconstpointer service)
{
	GDataBatchOperation *operation;
	guint op_id;
	GMainLoop *main_loop;
	GCancellable *cancellable;

	/* Run an async query operation on the video */
	operation = gdata_batchable_create_operation (GDATA_BATCHABLE (service), "http://gdata.youtube.com/feeds/api/videos/batch");
	op_id = gdata_test_batch_operation_query (operation, gdata_entry_get_id (data->new_video), GDATA_TYPE_YOUTUBE_VIDEO, data->new_video, NULL,
	                                          NULL);

	main_loop = g_main_loop_new (NULL, TRUE);
	cancellable = g_cancellable_new ();

	gdata_batch_operation_run_async (operation, cancellable, (GAsyncReadyCallback) test_batch_async_cancellation_cb, main_loop);
	g_cancellable_cancel (cancellable); /* this should cancel the operation before it even starts, as we haven't run the main loop yet */

	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);
	g_object_unref (cancellable);
}

static void
teardown_batch (BatchData *data, gconstpointer service)
{
	g_object_unref (data->new_video);
	g_object_unref (data->new_video2);
}

int
main (int argc, char *argv[])
{
	gint retval;
	GDataService *service = NULL;

	gdata_test_init (argc, argv);

	if (gdata_test_internet () == TRUE) {
		service = GDATA_SERVICE (gdata_youtube_service_new (DEVELOPER_KEY, CLIENT_ID));
		gdata_service_authenticate (service, USERNAME, PASSWORD, NULL, NULL);

		g_test_add_func ("/youtube/authentication", test_authentication);
		g_test_add_func ("/youtube/authentication_async", test_authentication_async);

		g_test_add_data_func ("/youtube/query/standard_feed", service, test_query_standard_feed);
		g_test_add_data_func ("/youtube/query/standard_feed_async", service, test_query_standard_feed_async);
		g_test_add_data_func ("/youtube/query/related", service, test_query_related);
		g_test_add_data_func ("/youtube/query/related_async", service, test_query_related_async);

		g_test_add_data_func ("/youtube/upload/simple", service, test_upload_simple);
		g_test_add_data_func ("/youtube/upload/async", service, test_upload_async);

		g_test_add_data_func ("/youtube/query/single", service, test_query_single);
		g_test_add_data_func ("/youtube/query/single_async", service, test_query_single_async);

		g_test_add_data_func ("/youtube/categories", service, test_categories);
		g_test_add_data_func ("/youtube/categories/async", service, test_categories_async);

		g_test_add ("/youtube/batch", BatchData, service, setup_batch, test_batch, teardown_batch);
		g_test_add ("/youtube/batch/async", BatchData, service, setup_batch, test_batch_async, teardown_batch);
		g_test_add ("/youtube/batch/async/cancellation", BatchData, service, setup_batch, test_batch_async_cancellation, teardown_batch);
	}

	g_test_add_func ("/youtube/parsing/app:control", test_parsing_app_control);
	/*g_test_add_func ("/youtube/parsing/comments/feedLink", test_parsing_comments_feed_link);*/
	g_test_add_func ("/youtube/parsing/yt:recorded", test_parsing_yt_recorded);
	g_test_add_func ("/youtube/parsing/yt:accessControl", test_parsing_yt_access_control);
	g_test_add_func ("/youtube/parsing/yt:category", test_parsing_yt_category);
	g_test_add_func ("/youtube/parsing/video_id_from_uri", test_parsing_video_id_from_uri);

	g_test_add_func ("/youtube/query/uri", test_query_uri);
	g_test_add_func ("/youtube/query/etag", test_query_etag);

	retval = g_test_run ();

	if (service != NULL)
		g_object_unref (service);

	return retval;
}
