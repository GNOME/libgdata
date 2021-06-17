/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Joe Cortes 2010 <escozzia@gmail.com>
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

/*
 * This is an extremely simple example program to query youtube videos
 * and picasa pictures and add them to a grid.
 * It was coded as part of the 2010 Google Code-In.
 * Click on Properties to change the window's title, or authenticate
 * yourself (it's assumed that your google and picasa ids are the same)
 * It's also possible to upload files to picasa.
 * Since this is only an example, intended to serve as documentation,
 * it's probably full of bugs and a couple of awful programming practices.
 * The program itself is of no practical use, but it does what it is
 * meant to do and doesn't segfault randomly (I hope so :p).
 */

#include "scrapbook.h"

static void
open_in_web_browser (GtkWidget *widget, gchar *uri) /* quicky wrapper for gtk_show_uri */
{
	gtk_show_uri_on_window (GTK_WINDOW (gtk_widget_get_toplevel (widget)), uri, GDK_CURRENT_TIME, NULL);
}

static void
picture_selected (GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *column, ScrapPicSearch *self)
{
	GtkTreeModel 	*model;
	GtkTreeIter		iter;
	GDataEntry		*pic;
	GdkPixbuf		*thumbnail;
	GtkWidget		*button;
	GtkWidget		*image;

	pic = g_slice_new				(GDataEntry);
	model = gtk_tree_view_get_model	(tree);
	gtk_tree_model_get_iter			(model, &iter, path);
	gtk_tree_model_get				(model, &iter, COL_PIXBUF, &(thumbnail), P_COL_PIC, &(pic), -1);
	gtk_widget_destroy				(self->search_data->window);
	gtk_list_store_append (self->main_data->lStore, &iter);
	gtk_list_store_set (self->main_data->lStore, &iter,
									 ORIG_COL_PIXBUF, 	thumbnail,
									 ORIG_COL_TITLE,	gdata_entry_get_title (pic),
									 ORIG_COL_ENTRY, 	pic, -1);
	if (self->main_data->currentRow[self->main_data->currentCol] > self->main_data->max_rows) {
		self->main_data->currentCol++;
		self->main_data->currentRow[self->main_data->currentCol] = 0;
	}
	image 	= gtk_image_new_from_pixbuf (thumbnail);
	button	= gtk_button_new			();
	self->file	= GDATA_PICASAWEB_FILE  (pic);
	gtk_widget_show 					(image);
	gtk_container_add 					(GTK_CONTAINER (button), image);
	/*g_signal_connect 					(button, "clicked", G_CALLBACK (open_in_web_browser), gdata_entry_get_id (pic));
	 * commented out, I can't seem to find anything that will give me a picture's URI */

	gtk_grid_attach (GTK_GRID (self->main_data->grid), button,
	                 self->main_data->currentCol,
	                 self->main_data->currentRow[self->main_data->currentCol],
	                 1, 1);

	gtk_widget_show (button);
	self->main_data->currentRow[self->main_data->currentCol]++;
}



static void
video_selected (GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *column, ScrapYTSearch *self)
{
	GtkTreeModel *model;
	GtkTreeIter  iter;
	GDataEntry   *video;
	GdkPixbuf	 *thumbnail;
	GtkWidget	 *button;
	GtkWidget	 *image;
	video = g_slice_new 			(GDataEntry);
	model = gtk_tree_view_get_model (tree);
	gtk_tree_model_get_iter 		(model, &iter, path);
	gtk_tree_model_get 				(model, &iter, COL_PIXBUF, &(thumbnail), COL_VIDEO, &(video), -1);
	gtk_widget_destroy 				(self->window);
	gtk_list_store_append (self->main_data->lStore, &iter);
	gtk_list_store_set (self->main_data->lStore, &iter,
									 ORIG_COL_PIXBUF, thumbnail,
									 ORIG_COL_TITLE, gdata_entry_get_title (video),
									 ORIG_COL_ENTRY, video, -1);
	if (self->main_data->currentRow[self->main_data->currentCol] > self->main_data->max_rows) {
		self->main_data->currentCol++;
		self->main_data->currentRow[self->main_data->currentCol] = 0;
	}
	image 	= gtk_image_new_from_pixbuf (thumbnail);
	button 	= gtk_button_new ();
	gtk_widget_show 	(image);
	gtk_container_add 	(GTK_CONTAINER (button), image);
	g_signal_connect (button, "clicked", G_CALLBACK (open_in_web_browser),
	                  (gpointer) gdata_youtube_video_get_player_uri (GDATA_YOUTUBE_VIDEO (video)));

	gtk_grid_attach (GTK_GRID (self->main_data->grid), button,
	                 self->main_data->currentCol,
	                 self->main_data->currentRow[self->main_data->currentCol],
	                 1, 1);

	gtk_widget_show (button);
	self->main_data->currentRow[self->main_data->currentCol]++;
}

static void
p_display_tree (ScrapPicSearch *self)
{
	GtkCellRenderer 	*renderer;
	GtkWidget			*scrollWin;
	scrollWin = gtk_scrolled_window_new 		(NULL, NULL);
	gtk_scrolled_window_set_policy 				(GTK_SCROLLED_WINDOW(scrollWin), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	renderer = gtk_cell_renderer_pixbuf_new 	();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (self->tView), -1,
												 "", renderer, "pixbuf",
												 P_COL_PIXBUF, NULL);

	renderer = gtk_cell_renderer_text_new		();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (self->tView), -1,
												 "", renderer, "text",
												 P_COL_USER, NULL);

	renderer = gtk_cell_renderer_text_new		();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (self->tView), -1,
												 "", renderer, "text",
												 P_COL_TITLE, NULL);

	gtk_tree_view_set_model						(GTK_TREE_VIEW (self->tView), GTK_TREE_MODEL (self->lStore));
	gtk_container_add 							(GTK_CONTAINER (scrollWin), self->tView);

	g_signal_connect							(self->tView, "row-activated", G_CALLBACK (picture_selected), self);

	gtk_widget_show 							(self->tView);
	gtk_widget_show								(scrollWin);
	gtk_box_pack_start							(GTK_BOX(self->search_data->box1), scrollWin, TRUE, TRUE, 2);
}


static void
yt_display_tree (ScrapYTSearch *self)
{
	GtkCellRenderer 	*renderer;
	GtkWidget			*scrollWin;
	scrollWin = gtk_scrolled_window_new 		(NULL, NULL);
	gtk_scrolled_window_set_policy 				(GTK_SCROLLED_WINDOW(scrollWin), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	renderer = gtk_cell_renderer_pixbuf_new 	();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (self->tView), -1,
												 "", renderer, "pixbuf",
												 COL_PIXBUF, NULL);
	renderer = gtk_cell_renderer_text_new		();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (self->tView), -1,
												 "", renderer, "text",
												 COL_TITLE, NULL);
	gtk_tree_view_set_model						(GTK_TREE_VIEW (self->tView), GTK_TREE_MODEL (self->lStore));
	gtk_container_add 							(GTK_CONTAINER (scrollWin), self->tView);
	g_signal_connect							(self->tView, "row-activated", G_CALLBACK (video_selected), self);
	gtk_widget_show 							(self->tView);
	gtk_widget_show								(scrollWin);
	gtk_box_pack_start							(GTK_BOX(self->box1), scrollWin, TRUE, TRUE, 2);
}

GDataMediaThumbnail *
choose_best_thumbnail (GList *thumbnails, gint ideal_size)
{
	gint delta = G_MININT;
	GDataMediaThumbnail *thumbnail = NULL;
	for (; thumbnails != NULL; thumbnails = thumbnails->next) {
		gint new_delta;
		GDataMediaThumbnail *current = (GDataMediaThumbnail *) thumbnails->data;
		new_delta = gdata_media_thumbnail_get_width (current) - ideal_size;
		if (delta == 0) {
			break;
		} else if ((delta == G_MININT)
		|| (delta < 0 && new_delta > delta)
		|| (delta > 0 && new_delta < delta)) {
			delta = new_delta;
			thumbnail = current;
		}
	}
	return thumbnail;
}

static void
find_pictures  (GDataEntry *entry, guint entry_key, guint entry_count, ScrapPicSearch *self)
{
	GtkTreeIter iter;
	GList 				*thumbnails;
	GDataMediaThumbnail	*thumbnail;
	GFileInputStream	*input_stream;

	gtk_list_store_append (self->lStore, &iter);
	gtk_list_store_set (self->lStore, &iter,
							 P_COL_PIXBUF, NULL,
							 P_COL_TITLE,  self->title,
							 P_COL_USER,   self->user,
							 P_COL_PIC,	   entry, -1);
	thumbnails		= gdata_picasaweb_file_get_thumbnails (GDATA_PICASAWEB_FILE (entry));
	thumbnail		= choose_best_thumbnail (thumbnails, THUMBNAIL_WIDTH);
	if (thumbnail != NULL) {
		GFile *thumbnail_file;
		thumbnail_file 	= g_file_new_for_uri 					(gdata_media_thumbnail_get_uri (thumbnail));
		input_stream   	= g_file_read							(thumbnail_file, NULL, NULL);
		self->thumbnail	= gdk_pixbuf_new_from_stream_at_scale 	(G_INPUT_STREAM (input_stream), THUMBNAIL_WIDTH, -1,
																 TRUE, NULL, NULL);
		gtk_list_store_set (self->lStore, &iter, P_COL_PIXBUF, self->thumbnail, -1); /* we can now set the thumbnail ;) */
		g_object_unref		(thumbnail_file);
	}
	gdata_query_set_q (self->query, NULL);
}


static void
p_query_element (GDataEntry *entry, guint entry_key, guint entry_count, ScrapPSearch *self)
{
	GError				*error=NULL;
	ScrapPicSearch		*picture;
	GDataFeed *feed;

	picture = self->pic;
	picture->title				= gdata_entry_get_title	(entry);
	picture->query 				= picture->search_data->query;
	picture->user				= self->user;
	gdata_query_set_q 					(picture->query, picture->title);
	feed = gdata_picasaweb_service_query_files (self->main_data->picasaweb_service, GDATA_PICASAWEB_ALBUM (entry), picture->query, NULL,
	                                            (GDataQueryProgressCallback) find_pictures, picture, &error);

	if (error != NULL) {
		g_print ("whoops, somebody raised an error!\n%s", error->message);
		g_error_free (error);
	}

	if (feed != NULL) {
		g_object_unref (feed);
	}
}

static void
p_text_callback (GtkWidget *widget, ScrapPSearch *self)
{
	GError *error 	= NULL;
	GDataFeed *feed;

	self->user		= gtk_entry_get_text 			(GTK_ENTRY (self->user_entry));
	self->pic->tView	= gtk_tree_view_new ();
	feed = gdata_picasaweb_service_query_all_albums (self->main_data->picasaweb_service, self->query, self->user, NULL,
	                                                 (GDataQueryProgressCallback) p_query_element, self, &error);
	if (error != NULL) {
		g_print ("someone raised an error\n%s\n",error->message);
		g_error_free (error);
	}

	if (feed != NULL) {
		g_object_unref (feed);
	}

	p_display_tree (self->pic);
}


/* ran as a callback for each individual element queried
 * it takes the video found, a unique entry_key and entry_count and the data structure as arguments
 * parts of this function were inspired (or simply taken) from the totem youtube plugin */
static void
yt_query_element (GDataEntry *entry, guint entry_key, guint entry_count, ScrapYTSearch *self)
{
	GtkTreeIter iter;
	GList 				*thumbnails;
	GDataMediaThumbnail *thumbnail;
	const gchar *title; /* the video's title */
	const gchar *uri; /* the video's URI */
	GFileInputStream	*input_stream; /* this will be used to make a pixbuf to store the thumbnail */

	title	= gdata_entry_get_title 				(entry); /* self-explanatory, I hope */
	uri		= gdata_youtube_video_get_player_uri	(GDATA_YOUTUBE_VIDEO (entry)); /* ditto */
	g_print ("%s %s", title, uri);
	gtk_list_store_append (self->lStore, &iter); /* make a new entry for this vid */
	gtk_list_store_set (self->lStore, &iter,
						   COL_PIXBUF, NULL, /* this will be set in a few moments */
						   COL_TITLE, title,
						   COL_VIDEO, entry,
						   -1);
	/* get a GList of thumbnails for the vid */
	thumbnails = gdata_youtube_video_get_thumbnails (GDATA_YOUTUBE_VIDEO (entry));
	thumbnail  = choose_best_thumbnail (thumbnails, THUMBNAIL_WIDTH);
	if (thumbnail != NULL) {
		GFile *thumbnail_file;
		thumbnail_file 	= g_file_new_for_uri 					(gdata_media_thumbnail_get_uri (thumbnail));
		input_stream   	= g_file_read							(thumbnail_file, NULL, NULL);
		self->thumbnail	= gdk_pixbuf_new_from_stream_at_scale 	(G_INPUT_STREAM (input_stream), THUMBNAIL_WIDTH, -1,
																 TRUE, NULL, NULL);
		gtk_list_store_set (self->lStore, &iter, COL_PIXBUF, self->thumbnail, -1); /* we can now set the thumbnail ;) */
		g_object_unref		(thumbnail_file);
	}
}


static void
yt_text_callback (GtkWidget *widget, ScrapYTSearch *self)
{
	GDataFeed *feed;

	self->txt = gtk_entry_get_text (GTK_ENTRY (self->txt_entry));
	gdata_query_set_q	 				(self->query, self->txt);	/* set the string we'll be searching for in youtube */

	/* do the actual query, running yt_query_element for each object found */
	feed = gdata_youtube_service_query_videos (self->main_data->youtube_service, self->query, NULL, (GDataQueryProgressCallback) yt_query_element,
	                                           self, NULL);
	if (feed != NULL) {
		g_object_unref (feed);
	}

	yt_display_tree (self); /* run yt_display_tree to show the results */
}


static void
start_new_picasa_search (GtkWidget *widget, ScrapData *first)
{
	ScrapPSearch 				*self;
	ScrapPicSearch				*picture;
	GtkWidget *button, *box2;

	self = first->p_search;
	picture 					= first->pic_search;
	picture->search_data 		= self;
	picture->search_data->pic	= picture;
	picture->main_data			= self->main_data;
	g_assert (GDATA_IS_PICASAWEB_SERVICE (first->picasaweb_service));
	gtk_list_store_clear (self->pic->lStore);

	self->window = gtk_window_new 	(GTK_WINDOW_TOPLEVEL);
	gtk_window_resize				(GTK_WINDOW (self->window), 400, 400);
	g_signal_connect 				(self->window, "destroy", G_CALLBACK (gtk_widget_destroy), NULL);
	g_signal_connect_swapped 		(self->window, "delete-event", G_CALLBACK (gtk_widget_destroy), NULL);

	/* our two boxes */

	self->box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10); /* this box contains everything in our window */
	gtk_container_add 				(GTK_CONTAINER (self->window), self->box1);
	box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	/* search bar */

	self->user_entry = gtk_entry_new ();
	gtk_entry_set_text 				(GTK_ENTRY (self->user_entry), "User to search for");
	g_signal_connect (self->user_entry, "activate", (GCallback) p_text_callback, self);
	gtk_box_pack_start (GTK_BOX(box2), self->user_entry, TRUE, TRUE, 0);
	gtk_widget_show    				(self->user_entry);

	/* Search button */
	button = gtk_button_new_with_mnemonic ("_Search");
	g_signal_connect (button, "clicked", (GCallback) p_text_callback, self);
	gtk_box_pack_start (GTK_BOX (box2), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	gtk_box_pack_end (GTK_BOX (self->box1), box2, FALSE, FALSE, 0); /* pack the box with the button and search bar */
	gtk_widget_show (box2);

	gtk_widget_show (self->box1);
	gtk_widget_show (self->window);
}



static void
start_new_youtube_search (GtkWidget *widget, ScrapData *first) /* *first is a pointer we use to talk to the main window */
{
	ScrapYTSearch *self; /* this struct will be used for all the data in the search, if there's time I'll make it into a GObject */
	GtkWidget *button, *box2;

	self = first->yt_search;

	gtk_list_store_clear 				(self->lStore); /* clear it out */
	self->tView  = gtk_tree_view_new 	();
	/* window stuff */
	self->window = gtk_window_new	(GTK_WINDOW_TOPLEVEL);
	gtk_window_resize				(GTK_WINDOW (self->window), 400, 400);
	g_signal_connect 				(self->window, "destroy", G_CALLBACK (gtk_widget_destroy), NULL);
	g_signal_connect_swapped 		(self->window, "delete-event", G_CALLBACK (gtk_widget_destroy), NULL);

	/* our two boxes */

	self->box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10); /* this box contains everything in our window */
	gtk_container_add 				(GTK_CONTAINER (self->window), self->box1);
	box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

	/* search bar */

	self->txt_entry = gtk_entry_new ();
	g_signal_connect 				(self->txt_entry, "activate", G_CALLBACK (yt_text_callback), self);
	gtk_box_pack_start (GTK_BOX (box2), self->txt_entry, TRUE, TRUE, 0);
	gtk_widget_show    				(self->txt_entry);

	/* Search button */
	button = gtk_button_new_with_mnemonic ("_Search");
	g_signal_connect (button, "clicked", (GCallback) yt_text_callback, self);
	gtk_box_pack_start (GTK_BOX (box2), button, TRUE, TRUE, 0);
	gtk_widget_show (button);

	gtk_box_pack_end (GTK_BOX (self->box1), box2, FALSE, FALSE, 0); /* pack the box with the button and search bar */

	gtk_widget_show (box2);
	gtk_widget_show (self->box1);
	gtk_widget_show (self->window);


	/* everything else is implemented somewhere else */
}

static GDataAuthorizer *
create_authorizer (GError **error)
{
	GDataOAuth2Authorizer *authorizer = NULL;  /* owned */
	GList *domains = NULL; /* list of GDataAuthorizationDomains */
	gchar *uri = NULL;
	gchar code[100];
	GError *child_error = NULL;

	/* Domains we need to be authorised for */
	domains = g_list_prepend (domains, gdata_youtube_service_get_primary_authorization_domain ());
	domains = g_list_prepend (domains, gdata_picasaweb_service_get_primary_authorization_domain ());

	/* Go through the interactive OAuth dance. */
	authorizer = gdata_oauth2_authorizer_new_for_authorization_domains (CLIENT_ID, CLIENT_SECRET,
	                                                                    REDIRECT_URI,
	                                                                    domains);

	/* Get an authentication URI */
	uri = gdata_oauth2_authorizer_build_authentication_uri (authorizer,
	                                                        NULL, FALSE);

	/* Wait for the user to retrieve and enter the verifier. */
	g_print ("Please navigate to the following URI and grant access:\n"
	         "   %s\n", uri);
	g_print ("Enter verifier (EOF to abort): ");

	g_free (uri);

	if (scanf ("%100s", code) != 1) {
		/* User chose to abort. */
		g_print ("\n");
		g_clear_object (&authorizer);
		return NULL;
	}

	/* Authorise the token. */
	gdata_oauth2_authorizer_request_authorization (authorizer, code, NULL,
	                                               &child_error);

	if (child_error != NULL) {
		g_propagate_error (error, child_error);
		g_clear_object (&authorizer);
		return NULL;
	}

	return GDATA_AUTHORIZER (authorizer);
}

static void
properties_set (GtkWidget *widget, ScrapProps *self)
{
	GDataAuthorizer *authorizer;
	GError *error = NULL;

	authorizer = create_authorizer (&error);

	if (error != NULL) { /* we show this to the user in case they mistyped their password */
		GtkWidget *label;

		label = gtk_label_new (error->message);
		gtk_widget_show (label);
		gtk_box_pack_end (GTK_BOX (self->box1), label, FALSE, FALSE, 0);

		g_print ("error\n%s\n", error->message);

		g_error_free (error);
	}

	gdata_service_set_authorizer (GDATA_SERVICE (self->main_data->youtube_service), authorizer);
	gdata_service_set_authorizer (GDATA_SERVICE (self->main_data->picasaweb_service), authorizer);

	gtk_widget_destroy (self->window);
	g_object_unref (authorizer);
}

static void
properties_show (GtkWidget *widget, ScrapData *first)
{
	ScrapProps	*self;
	GtkWidget *button;

	self			= g_slice_new (struct _ScrapProps);
	self->main_data	= first;
	self->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (self->window, "destroy", G_CALLBACK (gtk_widget_destroy), NULL);
	g_signal_connect (self->window, "delete-event", G_CALLBACK (gtk_widget_destroy), NULL);

	self->box1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 3);

	/* OK button */
	button = gtk_button_new_with_label ("_OK");
	g_signal_connect (button, "clicked", (GCallback) properties_set, self);
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (self->box1), button, FALSE, FALSE, 0);

	gtk_widget_show		(self->box1);
	gtk_container_add	(GTK_CONTAINER (self->window), self->box1);
	gtk_widget_show		(self->window);
}

static void
select_file (ScrapPUpload *self, GtkFileChooser *file_chooser)
{
	GFile *file;
	GError *error = NULL;
	GFileInfo *file_info;
	GDataUploadStream *upload_stream;
	GFileInputStream *file_stream;

	file = gtk_file_chooser_get_file (file_chooser);

	file_info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
	                               G_FILE_QUERY_INFO_NONE, NULL, NULL);

	/* upload our file, using the service we've set up, and metadata
	 * set up in upload ()
	 * no album is specified, but that should be easy to add */
	upload_stream = gdata_picasaweb_service_upload_file (self->main_data->picasaweb_service, NULL /* for now uploading to drop box */,
	                                                     self->file, g_file_info_get_display_name (file_info),
	                                                     g_file_info_get_content_type (file_info), NULL, &error);

	g_object_unref (file_info);
	g_object_unref (self->file);
	self->file = NULL;

	if (error != NULL) {
		g_print ("Error: %s\n", error->message);
		g_error_free (error);

		g_object_unref (file);

		return;
	}

	file_stream = g_file_read (file, NULL, NULL);
	g_object_unref (file);

	g_output_stream_splice (G_OUTPUT_STREAM (upload_stream), G_INPUT_STREAM (file_stream),
	                        G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, NULL);

	self->file = gdata_picasaweb_service_finish_file_upload (self->main_data->picasaweb_service, upload_stream, NULL);

	g_object_unref (file_stream);
	g_object_unref (upload_stream);
}

static void
got_name (GtkWidget *widget, ScrapData *scrap_data)
{
	ScrapPUpload *self;
	GtkWidget *file_dialog;

	self = scrap_data->p_upload;

	gdata_entry_set_title 	(GDATA_ENTRY (self->file),
							 gtk_entry_get_text (GTK_ENTRY (self->name)));
	gdata_entry_set_summary	(GDATA_ENTRY (self->file),
							 gtk_entry_get_text (GTK_ENTRY (self->description)));
	gtk_widget_destroy 	(self->dialog);

	file_dialog = gtk_file_chooser_dialog_new ("Upload Photo", GTK_WINDOW (scrap_data->window), GTK_FILE_CHOOSER_ACTION_SAVE,
	                                           "_Cancel", GTK_RESPONSE_CANCEL,
	                                           "_Open", GTK_RESPONSE_ACCEPT,
	                                           NULL);

	if (gtk_dialog_run (GTK_DIALOG (file_dialog)) == GTK_RESPONSE_ACCEPT) {
		select_file (self, GTK_FILE_CHOOSER (file_dialog));
	}

	/* since the upload blocks, it's safe to assume the widget won't
	 * be destroyed until we're done */
	gtk_widget_destroy (file_dialog);
}

static void
upload (GtkWidget *widget, ScrapData *first)
{
	ScrapPUpload 	*self;
	GtkWidget *label, *content_area;
	label = gtk_label_new ("Enter photo name and description");
	self = first->p_upload;

	/* we make a new file, without an id (it will be assigned automatically later on) */
	self->file  = gdata_picasaweb_file_new (NULL);
	/* dialog to get the file's name and description */
	self->dialog = gtk_dialog_new();
	content_area = gtk_dialog_get_content_area (GTK_DIALOG (self->dialog));

	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (content_area), label, FALSE, FALSE, 0);

	self->name	= gtk_entry_new ();
	g_signal_connect 	(self->name, "activate", G_CALLBACK (got_name), self);
	gtk_widget_show  	(self->name);
	gtk_box_pack_start (GTK_BOX (content_area), self->name, TRUE, TRUE, 0);

	self->description = gtk_entry_new ();
	g_signal_connect 	(self->description, "activate", G_CALLBACK (got_name), first);
	gtk_widget_show  	(self->description);
	gtk_box_pack_start (GTK_BOX (content_area), self->description, TRUE, TRUE, 0);

	gtk_widget_show		(self->dialog);
}

int
main(int argc, char **argv)
{
	ScrapData 		*scrapbook;
	ScrapPSearch 	*picasaSearch;
	ScrapYTSearch	*youtubeSearch;
	ScrapPicSearch	*photoSearch;
	ScrapPUpload	*fUpload;
	GtkWidget *button;
	GtkWidget *vbox;

	scrapbook = g_slice_new (struct _ScrapData);
	scrapbook->max_rows			= 5;

	gtk_init	(&argc, &argv);

	scrapbook->currentCol 							= 0;
	scrapbook->currentRow[scrapbook->currentCol]	= 0;

	scrapbook->lStore   = gtk_list_store_new (ORIG_N_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING, GDATA_TYPE_ENTRY);
	scrapbook->window 	= gtk_window_new 	(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (scrapbook->window), "Scrapbook");
	g_signal_connect (scrapbook->window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
	g_signal_connect (scrapbook->window, "delete-event", G_CALLBACK (gtk_main_quit), NULL);

	youtubeSearch 				= g_slice_new (struct _ScrapYTSearch); /* get some memory for the structure */
	youtubeSearch->txt	 		= NULL;
	scrapbook->yt_search 		= youtubeSearch;
	youtubeSearch->main_data 	= scrapbook;
	/* create a new query, without any search text, starting at 0, and search only MAX_RESULTS results */
	youtubeSearch->query = gdata_query_new_with_limits (NULL, 0, MAX_RESULTS);
	/* create a new youtube service, giving it our developer key; google no longer uses client ids so we send in an empty string (NULL gives an error) */
	scrapbook->youtube_service = gdata_youtube_service_new (DEVELOPER_KEY, NULL);
	/* create a new list store and tree to show the user the results
	 * it has three columns (two of which are displayed): a pixbuf for the thumbnail, the title, and the video data itself (as a gdata generic entry) */
	youtubeSearch->lStore = gtk_list_store_new 	(N_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING, GDATA_TYPE_ENTRY);

	picasaSearch 				= g_slice_new (struct _ScrapPSearch);
	scrapbook->p_search 		= picasaSearch;
	picasaSearch->main_data 	= scrapbook;
	picasaSearch->query 		= gdata_query_new_with_limits (NULL, 0, MAX_RESULTS);
	scrapbook->picasaweb_service = gdata_picasaweb_service_new (NULL);

	photoSearch					= g_slice_new (struct _ScrapPicSearch);
	scrapbook->p_search->pic	= photoSearch;
	scrapbook->pic_search		= photoSearch;
	photoSearch->lStore = gtk_list_store_new (P_N_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, GDATA_TYPE_ENTRY);

	fUpload						= g_slice_new (struct _ScrapPUpload);
	scrapbook->p_upload			= fUpload;
	fUpload->main_data			= scrapbook;

	scrapbook->box1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	scrapbook->grid = gtk_grid_new ();

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

	/* Add buttons to the main window */
	button = gtk_button_new_with_mnemonic ("Add YouTube _Video");
	g_signal_connect (button, "clicked", (GCallback) start_new_youtube_search, scrapbook);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	button = gtk_button_new_with_mnemonic ("Add PicasaWeb _Photo");
	g_signal_connect (button, "clicked", (GCallback) start_new_picasa_search, scrapbook);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	button = gtk_button_new_with_mnemonic ("P_roperties");
	g_signal_connect (button, "clicked", (GCallback) properties_show, scrapbook);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	button = gtk_button_new_with_mnemonic ("_Upload Photo to PicasaWeb");
	g_signal_connect (button, "clicked", (GCallback) upload, scrapbook);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (scrapbook->box1), vbox, FALSE, FALSE, 5);

	gtk_box_pack_start (GTK_BOX (scrapbook->box1), scrapbook->grid, TRUE, TRUE, 0);
	gtk_widget_show (scrapbook->grid);
	gtk_widget_show		(scrapbook->box1);
	gtk_container_add	(GTK_CONTAINER (scrapbook->window), scrapbook->box1);
	gtk_widget_show		(scrapbook->window);
	gtk_main			();
	return 0;
}
