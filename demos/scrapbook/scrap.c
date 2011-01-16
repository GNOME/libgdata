#include "scrapdata.h"


static void
open_in_web_browser (GtkWidget *widget, gchar *uri) /* quicky wrapper for gtk_show_uri */
{
	gtk_show_uri			(gtk_widget_get_screen (widget), uri, GDK_CURRENT_TIME, NULL);
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
	gtk_list_store_append			(self->main_data->lStore, &(self->main_data->iter));
	gtk_list_store_set				(self->main_data->lStore, &(self->main_data->iter),
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
	gtk_table_attach_defaults 			(GTK_TABLE(self->main_data->table), button,
										 self->main_data->currentRow[self->main_data->currentCol],
										 self->main_data->currentRow[self->main_data->currentCol]+1,
										 self->main_data->currentCol, self->main_data->currentCol+1);
	gtk_widget_show (button);
	self->main_data->currentRow[self->main_data->currentCol]++;
	g_object_unref (self->tView);
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
	gtk_list_store_append			(self->main_data->lStore, &(self->main_data->iter));
	gtk_list_store_set				(self->main_data->lStore, &(self->main_data->iter),
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
	g_signal_connect (button, "clicked", G_CALLBACK (open_in_web_browser), gdata_youtube_video_get_player_uri (GDATA_YOUTUBE_VIDEO (video)));
	gtk_table_attach_defaults (GTK_TABLE(self->main_data->table), button,
										 self->main_data->currentRow[self->main_data->currentCol],
										 self->main_data->currentRow[self->main_data->currentCol]+1,
										 self->main_data->currentCol, self->main_data->currentCol+1);
	gtk_widget_show (button);
	self->main_data->currentRow[self->main_data->currentCol]++;
	g_object_unref (self->tView);
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
	GDataMediaThumbnail *thumbnail;
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
	GList 				*thumbnails;
	GDataMediaThumbnail	*thumbnail;
	GFileInputStream	*input_stream;
	
	gtk_list_store_append	(self->lStore, &(self->iter));
	gtk_list_store_set		(self->lStore, &(self->iter),
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
		gtk_list_store_set 	(self->lStore, &(self->iter), P_COL_PIXBUF, self->thumbnail, -1); /* we can now set the thumbnail ;) */
		g_object_unref		(thumbnail_file);
	}
	gdata_query_set_q (self->query, NULL);
}


static void
p_query_element (GDataEntry *entry, guint entry_key, guint entry_count, ScrapPSearch *self)
{
	GError				*error=NULL;
	ScrapPicSearch		*picture;
	picture = self->pic;
	picture->title				= gdata_entry_get_title	(entry);
	picture->query 				= picture->search_data->query;
	picture->user				= self->user;
	gdata_query_set_q 					(picture->query, picture->title);
	gdata_picasaweb_service_query_files (picture->search_data->service,
										 GDATA_PICASAWEB_ALBUM (entry),
										 picture->query,
										 NULL,
										 (GDataQueryProgressCallback) find_pictures,
										 picture, &error);
	if (error != NULL) {
		g_print ("whoops, somebody raised an error!\n%s", error->message);
		g_error_free (error);
	}
}

static void
p_text_callback (GtkWidget *widget, ScrapPSearch *self)
{
	GError *error 	= NULL;
	self->user		= gtk_entry_get_text 			(GTK_ENTRY (self->user_entry));
	self->pic->lStore = gtk_list_store_new (P_N_COLS, GDK_TYPE_PIXBUF,
											G_TYPE_STRING,
											G_TYPE_STRING,
											GDATA_TYPE_ENTRY);
	self->pic->tView	= gtk_tree_view_new ();
	gdata_picasaweb_service_query_all_albums	(self->service, self->query,
												 self->user, NULL,
											    (GDataQueryProgressCallback) p_query_element,
												 self, &error);
	if (error != NULL) {
		g_print ("someone raised an error\n%s\n",error->message);
		g_error_free (error);
	}
	p_display_tree (self->pic);
}


/* ran as a callback for each individual element queried
 * it takes the video found, a unique entry_key and entry_count and the data structure as arguments
 * parts of this function were inspired (or simply taken) from the totem youtube plugin */
static void
yt_query_element (GDataEntry *entry, guint entry_key, guint entry_count, ScrapYTSearch *self)
{
	GList 				*thumbnails;
	GDataMediaThumbnail *thumbnail;
	gchar				*title; /* the video's title */
	gchar				*uri;   /* the video's URI */
	GFileInputStream	*input_stream; /* this will be used to make a pixbuf to store the thumbnail */
	title	= gdata_entry_get_title 				(entry); /* self-explanatory, I hope */
	uri		= gdata_youtube_video_get_player_uri	(GDATA_YOUTUBE_VIDEO (entry)); /* ditto */
	g_print ("%s %s", title, uri);
	gtk_list_store_append (self->lStore, &(self->iter)); /* make a new entry for this vid */
	gtk_list_store_set	  (self->lStore, &(self->iter),
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
		gtk_list_store_set 	(self->lStore, &(self->iter), COL_PIXBUF, self->thumbnail, -1); /* we can now set the thumbnail ;) */
		g_object_unref		(thumbnail_file);
	}
}


static void
yt_text_callback (GtkWidget *widget, ScrapYTSearch *self)
{
	self->txt = gtk_entry_get_text (GTK_ENTRY (self->txt_entry));
	gdata_query_set_q	 				(self->query, self->txt);	/* set the string we'll be searching for in youtube */
	/* do the actual query, running yt_query_element for each object found */
	gdata_youtube_service_query_videos 	(self->service, self->query, NULL, (GDataQueryProgressCallback) yt_query_element, self, NULL);
	yt_display_tree (self); /* run yt_display_tree to show the results */
}


static void
start_new_picasa_search (GtkWidget *widget, ScrapData *first)
{
	ScrapPSearch 				*self;
	ScrapPicSearch				*picture;
	self = first->p_search;
	picture 					= first->pic_search;
	picture->search_data 		= self;
	picture->search_data->pic	= picture;
	picture->main_data			= self->main_data;
	g_assert			 (GDATA_IS_PICASAWEB_SERVICE (self->service));
	gtk_list_store_clear (self->pic->lStore);
	
	self->window = gtk_window_new 	(GTK_WINDOW_TOPLEVEL);
	gtk_window_resize				(GTK_WINDOW (self->window), 400, 400);
	g_signal_connect 				(self->window, "destroy", G_CALLBACK (gtk_widget_destroy), NULL);
	g_signal_connect_swapped 		(self->window, "delete-event", G_CALLBACK (gtk_widget_destroy), NULL);
	
	/* our two boxes */
	
	self->box1	 = gtk_vbox_new	  	(FALSE, 10); /* this box contains everything in our window */
	gtk_container_add 				(GTK_CONTAINER (self->window), self->box1);
	self->box2	 = gtk_hbox_new	  	(FALSE,  2);
	
	/* search bar */
	
	self->user_entry = gtk_entry_new ();
	gtk_entry_set_text 				(GTK_ENTRY (self->user_entry), "user to search for");
	g_signal_connect				(self->button, "activated", G_CALLBACK (p_text_callback), self);
	gtk_box_pack_start 				(GTK_BOX(self->box2), self->user_entry, TRUE, TRUE, 0);
	gtk_widget_show    				(self->user_entry);
	
	/* button */
		
			
	self->button = gtk_button_new_with_label ("search");
	g_signal_connect	(self->button, "clicked", G_CALLBACK (p_text_callback), self);
	gtk_box_pack_start 	(GTK_BOX (self->box2), self->button, FALSE, FALSE, 0);
	gtk_widget_show		(self->button);
	
	gtk_box_pack_end (GTK_BOX (self->box1), self->box2, FALSE, FALSE, 0); /* pack the box with the button and search bar */
	gtk_widget_show (self->box2);
	
	gtk_widget_show (self->box1);
	gtk_widget_show (self->window);
}

	

static void
start_new_youtube_search (GtkWidget *widget, ScrapData *first) /* *first is a pointer we use to talk to the main window */
{
	ScrapYTSearch *self; /* this struct will be used for all the data in the search, if there's time I'll make it into a GObject */
	self = first->yt_search;

	gtk_list_store_clear 				(self->lStore); /* clear it out */
	self->tView  = gtk_tree_view_new 	();
	/* window stuff */
	self->window = gtk_window_new	(GTK_WINDOW_TOPLEVEL);
	gtk_window_resize				(GTK_WINDOW (self->window), 400, 400);
	g_signal_connect 				(self->window, "destroy", G_CALLBACK (gtk_widget_destroy), NULL);
	g_signal_connect_swapped 		(self->window, "delete-event", G_CALLBACK (gtk_widget_destroy), NULL);
	
	/* our two boxes */
	
	self->box1	 = gtk_vbox_new	  	(FALSE, 10); /* this box contains everything in our window */
	gtk_container_add 				(GTK_CONTAINER (self->window), self->box1);
	self->box2	 = gtk_hbox_new	  	(FALSE,  2);
	
	/* search bar */
	
	self->txt_entry = gtk_entry_new ();
	g_signal_connect 				(self->txt_entry, "activate", G_CALLBACK (yt_text_callback), self);
	gtk_box_pack_start 				(GTK_BOX(self->box2), self->txt_entry, TRUE, TRUE, 0);
	gtk_widget_show    				(self->txt_entry);
	
	/* button */
	
	self->button = gtk_button_new_with_label 	("search");
	g_signal_connect 							(self->button, "clicked", G_CALLBACK (yt_text_callback), self);
	gtk_box_pack_start 							(GTK_BOX (self->box2), self->button, TRUE, TRUE, 0);
	gtk_widget_show								(self->button);
	
	gtk_box_pack_end (GTK_BOX (self->box1), self->box2, FALSE, FALSE, 0); /* pack the box with the button and search bar */
	
	gtk_widget_show (self->box2);
	gtk_widget_show (self->box1);
	gtk_widget_show (self->window);


	/* everything else is implemented somewhere else */
}


static void
properties_set (GtkWidget *widget, ScrapProps *self)
{
	self->main_data->username = gtk_entry_get_text (GTK_ENTRY(self->username_entry));
	self->main_data->password = gtk_entry_get_text (GTK_ENTRY(self->password_entry));
	/* authenticate on youtube */
	{
		GError *error = NULL;
		gdata_service_authenticate (GDATA_SERVICE (self->main_data->yt_search->service),
									self->main_data->username,
									self->main_data->password,
									NULL, &error);
		if (error != NULL) { /* we show this to the user in case he mistyped his password */
			GtkWidget *label;
			label = gtk_label_new (error->message);
			gtk_widget_show (label);
			gtk_box_pack_end (GTK_BOX (self->box1), label, FALSE, FALSE, 0);
			g_print("error\n%s\n", error->message);
			g_error_free(error);
		}
	}	
	/* authenticate on picasa (no time for fun and games, so we assume he's got the same account on both services) */
	{
		GError *error = NULL;
		gdata_service_authenticate (GDATA_SERVICE (self->main_data->p_search->service),
									self->main_data->username,
									self->main_data->password,
									NULL, &error);
		if (error != NULL) {
			GtkWidget *label;
			label = gtk_label_new (error->message);
			gtk_widget_show (label);
			gtk_box_pack_end (GTK_BOX (self->box1), label, FALSE, FALSE, 0);
			g_print("error\n%s\n", error->message);
			g_error_free(error);
		}
	}

	
	/* if the username and password are changed, we re-authenticate */
	self->main_data->title	  = gtk_entry_get_text (GTK_ENTRY(self->title_entry));
	gtk_window_set_title(GTK_WINDOW (self->main_data->window), self->main_data->title);
	gtk_widget_destroy (self->window);
}

static void
properties_show (GtkWidget *widget, ScrapData *first)
{
	ScrapProps	*self;
	self			= g_slice_new (struct _ScrapProps);
	self->main_data	= first;
	self->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_resize (GTK_WINDOW (self->window), 250, 250);
	g_signal_connect (self->window, "destroy", G_CALLBACK (gtk_widget_destroy), NULL);
	g_signal_connect (self->window, "delete-event", G_CALLBACK (gtk_widget_destroy), NULL);
	
	self->box1	= gtk_vbox_new (FALSE, 3);
	self->box2 	= gtk_hbox_new (FALSE, 10);
	
	self->label = gtk_label_new ("username");
	gtk_widget_show		(self->label);
	gtk_box_pack_start 	(GTK_BOX (self->box2), self->label, TRUE, TRUE, 0);
	self->label = gtk_label_new ("password");
	gtk_widget_show		(self->label);
	gtk_box_pack_start 	(GTK_BOX (self->box2), self->label, TRUE, TRUE, 0);
	self->label = gtk_label_new ("title");
	gtk_widget_show		(self->label);
	gtk_box_pack_start	(GTK_BOX (self->box2), self->label, TRUE, TRUE, 0);
	gtk_widget_show		(self->box2);
	gtk_box_pack_start 	(GTK_BOX (self->box1), self->box2, FALSE, FALSE, 0);
	
	self->box2 = gtk_hbox_new (FALSE, 10);
	self->username_entry = gtk_entry_new ();
	
	if (self->main_data->username != NULL)
		gtk_entry_set_text (GTK_ENTRY(self->username_entry), self->main_data->username);
	
	gtk_widget_show	   (self->username_entry);
	gtk_box_pack_start (GTK_BOX (self->box2), self->username_entry, TRUE, TRUE, 0);
	
	self->password_entry = gtk_entry_new ();
	gtk_entry_set_visibility (GTK_ENTRY (self->password_entry), FALSE);
	
	if (self->main_data->password != NULL)
		gtk_entry_set_text (GTK_ENTRY(self->password_entry), self->main_data->password);
	
	gtk_widget_show	   (self->password_entry);
	gtk_box_pack_start (GTK_BOX (self->box2), self->password_entry, TRUE, TRUE, 0);
	
	self->title_entry = gtk_entry_new ();
	if (self->main_data->title != NULL)
		gtk_entry_set_text (GTK_ENTRY (self->title_entry), self->main_data->title);
	
	gtk_widget_show (self->title_entry);
	gtk_box_pack_start (GTK_BOX (self->box2), self->title_entry, TRUE, TRUE, 0);
	
	gtk_box_pack_start 	(GTK_BOX (self->box1), self->box2, FALSE, FALSE, 0);
	gtk_widget_show		(self->box2);
	self->button = gtk_button_new_with_label ("Ok");
	g_signal_connect 	(self->button, "clicked", G_CALLBACK (properties_set), self);
	gtk_widget_show		(self->button);
	gtk_box_pack_start 	(GTK_BOX (self->box1), self->button, FALSE, FALSE, 0);
	gtk_widget_show		(self->box1);
	gtk_container_add	(GTK_CONTAINER (self->window), self->box1);
	gtk_widget_show		(self->window);
}

static void
select_file (GtkWidget *widget, ScrapPUpload *self)
{
	GFile *file;
	GError *error = NULL;
	file = g_file_new_for_path(gtk_file_selection_get_filename (GTK_FILE_SELECTION(self->file_dialog)));
	/* upload our file, using the service we've set up, and metadata
	 * set up in upload ()
	 * no album is specified, but that should be easy to add */
	self->file=gdata_picasaweb_service_upload_file (self->main_data->p_search->service,
										 NULL, /* for now uploading to drop box */
										 self->file, file, NULL, &error);
	if (error != NULL) {
		g_print ("error: %s\n", error->message);
	}
	g_free (error);
	/* since the upload blocks, it's safe to assume the widget won't
	 * be destroyed until we're done */
	gtk_widget_destroy (self->file_dialog);
}
										 
static void
got_name (GtkWidget *widget, ScrapPUpload *self)
{
	gdata_entry_set_title 	(GDATA_ENTRY (self->file),
							 gtk_entry_get_text (GTK_ENTRY (self->name)));
	gdata_entry_set_summary	(GDATA_ENTRY (self->file),
							 gtk_entry_get_text (GTK_ENTRY (self->description)));
	gtk_widget_destroy 	(self->dialog);
	gtk_widget_show		(self->file_dialog);
}
	
static void
upload (GtkWidget *widget, ScrapData *first)
{
	ScrapPUpload 	*self;
	GtkWidget		*label;
	label = gtk_label_new ("Enter photo name and description");
	self = first->p_upload;
	self->file_dialog = gtk_file_selection_new ("upload");
	/* we make a new file, without an id (it will be assigned automatically later on) */
	self->file  = gdata_picasaweb_file_new (NULL);
	/* dialog to get the file's name and description */
	self->dialog = gtk_dialog_new();
	
	gtk_widget_show (label);
	gtk_box_pack_start	(GTK_BOX (GTK_DIALOG(self->dialog)->vbox),
						 label, FALSE, FALSE, 0);
	
	self->name	= gtk_entry_new ();
	g_signal_connect 	(self->name, "activate", G_CALLBACK (got_name), self);
	gtk_widget_show  	(self->name);
	gtk_box_pack_start 	(GTK_BOX (GTK_DIALOG(self->dialog)->action_area),
						 self->name, TRUE, TRUE, 0);
	
	self->description = gtk_entry_new ();
	g_signal_connect 	(self->description, "activate", G_CALLBACK (got_name), self);
	gtk_widget_show  	(self->description);
	gtk_box_pack_start 	(GTK_BOX (GTK_DIALOG (self->dialog)->action_area),
						 self->description, TRUE, TRUE, 0);
	
	gtk_widget_show		(self->dialog);
	g_signal_connect 	(self->file_dialog, "destroy", G_CALLBACK (gtk_widget_destroy), NULL);
	g_signal_connect 	(GTK_FILE_SELECTION(self->file_dialog)->ok_button, "clicked",
						 G_CALLBACK (select_file), self);
	
	g_signal_connect_swapped (GTK_FILE_SELECTION (self->file_dialog)->cancel_button,
							  "clicked", G_CALLBACK (gtk_widget_destroy),
							  self->file_dialog);
}

int
main(int argc, char **argv)
{
	ScrapData 		*scrapbook;
	ScrapPSearch 	*picasaSearch;
	ScrapYTSearch	*youtubeSearch;
	ScrapPicSearch	*photoSearch;
	ScrapPUpload	*fUpload;
	scrapbook = g_slice_new (struct _ScrapData);
	scrapbook->title			= NULL;
	scrapbook->max_rows			= 5;
	g_type_init ();
	gtk_init	(&argc, &argv);
	
	scrapbook->currentCol 							= 0;
	scrapbook->currentRow[scrapbook->currentCol]	= 0;
	
	scrapbook->lStore   = gtk_list_store_new (ORIG_N_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING, GDATA_TYPE_ENTRY);
	scrapbook->window 	= gtk_window_new 	(GTK_WINDOW_TOPLEVEL);
	g_signal_connect (scrapbook->window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
	g_signal_connect (scrapbook->window, "delete-event", G_CALLBACK (gtk_main_quit), NULL);
	
	youtubeSearch 				= g_slice_new (struct _ScrapYTSearch); /* get some memory for the structure */
	youtubeSearch->txt	 		= NULL;
	scrapbook->yt_search 		= youtubeSearch;
	youtubeSearch->main_data 	= scrapbook;
	/* create a new query, without any search text, starting at 0, and search only MAX_RESULTS results */
	youtubeSearch->query = gdata_query_new_with_limits (NULL, 0, MAX_RESULTS);
	/* create a new youtube service, giving it our developer key; google no longer uses client ids so we send in an empty string (NULL gives an error) */
	youtubeSearch->service = gdata_youtube_service_new (DEVELOPER_KEY, "");
	/* create a new list store and tree to show the user the results
	 * it has three columns (two of which are displayed): a pixbuf for the thumbnail, the title, and the video data itself (as a gdata generic entry) */
	youtubeSearch->lStore = gtk_list_store_new 	(N_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING, GDATA_TYPE_ENTRY);

	picasaSearch 				= g_slice_new (struct _ScrapPSearch);
	scrapbook->p_search 		= picasaSearch;
	picasaSearch->main_data 	= scrapbook;
	picasaSearch->query 		= gdata_query_new_with_limits (NULL, 0, MAX_RESULTS);
	picasaSearch->service 		= gdata_picasaweb_service_new ("");

	photoSearch					= g_slice_new (struct _ScrapPicSearch);
	scrapbook->p_search->pic	= photoSearch;
	scrapbook->pic_search		= photoSearch;
	
	fUpload						= g_slice_new (struct _ScrapPUpload);
	scrapbook->p_upload			= fUpload;
	fUpload->main_data			= scrapbook;
	
	gtk_window_resize(GTK_WINDOW (scrapbook->window), 350, 150);
	scrapbook->box1		= gtk_hbox_new		(FALSE, 0);
	scrapbook->box2		= gtk_vbox_new		(FALSE, 2);
	scrapbook->table	= gtk_table_new		(5,5,FALSE);
	
	scrapbook->button = gtk_button_new_with_label ("Add You Tube Video");	
	g_signal_connect 	(scrapbook->button, "clicked", G_CALLBACK (start_new_youtube_search), scrapbook);
	gtk_box_pack_start	(GTK_BOX (scrapbook->box2), scrapbook->button, FALSE, FALSE, 0);
	gtk_widget_show		(scrapbook->button);
		
	scrapbook->button = gtk_button_new_with_label ("Add Picasa Photo");
	g_signal_connect	(scrapbook->button,"clicked", G_CALLBACK (start_new_picasa_search), scrapbook);
	gtk_box_pack_start	(GTK_BOX (scrapbook->box2), scrapbook->button, FALSE, FALSE, 0);
	gtk_widget_show 	(scrapbook->button);
	
	scrapbook->button = gtk_button_new_with_label ("Properties");
	g_signal_connect	(scrapbook->button, "clicked", G_CALLBACK (properties_show), scrapbook);
	gtk_box_pack_start	(GTK_BOX (scrapbook->box2), scrapbook->button, FALSE, FALSE, 0);
	gtk_widget_show		(scrapbook->button);
	
	scrapbook->button = gtk_button_new_with_label ("Upload picture to picasa web");
	g_signal_connect	(scrapbook->button, "clicked", G_CALLBACK (upload), scrapbook);
	gtk_box_pack_start 	(GTK_BOX (scrapbook->box2), scrapbook->button, FALSE, FALSE, 0);
	gtk_widget_show		(scrapbook->button);
	
	gtk_widget_show		(scrapbook->box2);
	gtk_box_pack_start	(GTK_BOX (scrapbook->box1), scrapbook->box2, FALSE, FALSE, 5);
	gtk_box_pack_start	(GTK_BOX (scrapbook->box1), scrapbook->table, TRUE, TRUE, 0);
	gtk_widget_show		(scrapbook->table);
	gtk_widget_show		(scrapbook->box1);
	gtk_container_add	(GTK_CONTAINER (scrapbook->window), scrapbook->box1);
	gtk_widget_show		(scrapbook->window);
	gtk_main			();
	return 0;
}
