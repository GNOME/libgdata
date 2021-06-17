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

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdata/gdata.h>
#include <glib.h>
#include <glib-object.h>

#define DEVELOPER_KEY "AI39si7Me3Q7zYs6hmkFvpRBD2nrkVjYYsUO5lh_3HdOkGRc9g6Z4nzxZatk_aAo2EsA21k7vrda0OO6oFg2rnhMedZXPyXoEw"
#define CLIENT_ID "352818697630-nqu2cmt5quqd6lr17ouoqmb684u84l1f.apps.googleusercontent.com"
#define CLIENT_SECRET "-fA4pHQJxR3zJ-FyAMPQsikg"
#define REDIRECT_URI "urn:ietf:wg:oauth:2.0:oob"

#define THUMBNAIL_WIDTH 180
#define MAX_RESULTS 	10

/* how this works is that there's a struct for every window opened
 * they contain that individual window's data
 * and every one of them (except for scrap data, which is for the main window)
 * has a pointer to the one for the main window called main_data */

enum {
	COL_PIXBUF,
	COL_TITLE,
	COL_VIDEO,
	N_COLS
};

enum {
	P_COL_PIXBUF,
	P_COL_TITLE,
	P_COL_USER,
	P_COL_PIC,
	P_N_COLS
};


enum {
	ORIG_COL_PIXBUF,
	ORIG_COL_TITLE,
	ORIG_COL_ENTRY,
	ORIG_N_COLS
};

typedef struct _ScrapYTSearch ScrapYTSearch;
typedef struct _ScrapPSearch  ScrapPSearch;
typedef struct _ScrapPUpload  ScrapPUpload;
typedef struct _ScrapPicSearch ScrapPicSearch;

typedef struct _ScrapData {
	GtkWidget		*window;
	gint			currentCol;
	gint			currentRow[5];
	GtkWidget		*box1;
	GtkWidget *grid;
	GtkWidget		*scrollWindow;
	ScrapYTSearch	*yt_search;
	ScrapPSearch	*p_search;
	ScrapPicSearch *pic_search;
	ScrapPUpload	*p_upload;
	gint			max_rows;
	GtkListStore	*lStore;

	GDataYouTubeService *youtube_service;
	GDataPicasaWebService *picasaweb_service;
} ScrapData;
struct _ScrapPUpload {
	ScrapData			*main_data;
	GDataPicasaWebFile	*file;
	GtkWidget			*dialog;
	GtkWidget			*name;
	GtkWidget			*description;
};

struct _ScrapPicSearch { /* for finding pictures */
	const gchar *title;
	gchar					*uri;
	const gchar *user;
	GdkPixbuf				*thumbnail;
	ScrapData				*main_data;
	ScrapPSearch			*search_data;
	GDataQuery				*query;
	GDataPicasaWebFile		*file;
	GtkListStore			*lStore;
	GtkWidget				*tView;
};

struct _ScrapYTSearch { /* youtube search data */
	GtkWidget 			*txt_entry;
	const gchar *txt;
	GtkWidget			*window;
	GDataQuery		 	*query;
	gchar				*title;
	gchar				*uri;
	GdkPixbuf			*thumbnail;
	GtkWidget 			*box1;
	ScrapData			*main_data; /* <- points to a structure containing main vars */
	GtkListStore		*lStore;
	GtkWidget			*tView;
};

struct _ScrapPSearch { /* for finding albums */
	GtkWidget				*window;
	GDataQuery				*query;
	gchar					*title;
	gchar					*uri;
	GdkPixbuf				*thumbnail;
	GtkWidget				*box1;
	ScrapData				*main_data;
	GtkWidget				*user_entry;
	const gchar *user;
	ScrapPicSearch			*pic;
};

typedef struct _ScrapProps {
	GtkWidget	*window;
	GtkWidget	*box1;
	ScrapData	*main_data;
} ScrapProps;


static void
open_in_web_browser (GtkWidget *widget, gchar *uri);

static void
picture_selected (GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *column, ScrapPicSearch *self);

static void
video_selected (GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *column, ScrapYTSearch *self);

static void
p_display_tree (ScrapPicSearch *self);

static void
yt_display_tree (ScrapYTSearch *self);

GDataMediaThumbnail *
choose_best_thumbnail (GList *thumbnails, gint ideal_size);

static void
find_pictures  (GDataEntry *entry, guint entry_key, guint entry_count, ScrapPicSearch *self);

static void
p_query_element (GDataEntry *entry, guint entry_key, guint entry_count, ScrapPSearch *self);

static void
p_text_callback (GtkWidget *widget, ScrapPSearch *self);

static void
yt_query_element (GDataEntry *entry, guint entry_key, guint entry_count, ScrapYTSearch *self);

static void
yt_text_callback (GtkWidget *widget, ScrapYTSearch *self);

static void
start_new_picasa_search (GtkWidget *widget, ScrapData *first);

static void
start_new_youtube_search (GtkWidget *widget, ScrapData *first);

static void
properties_set (GtkWidget *widget, ScrapProps *self);

static void
properties_show (GtkWidget *widget, ScrapData *first);

static void
upload (GtkWidget *widget, ScrapData *first);



