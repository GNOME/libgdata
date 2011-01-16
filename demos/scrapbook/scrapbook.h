#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdata/gdata.h>
#include <glib.h>
#include <glib-object.h>
#define DEVELOPER_KEY "AI39si5MkSF-0bzTmP5WETk1D-Z7inHaQJzX13PeG_5Uzeu8mz3vo40cFoqnxjejB-UqzYFrqzOSlsqJvHuPNEGqdycqnPo30A"
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

typedef struct _ScrapData {
	GtkWidget		*window;
	GtkWidget		*button;
	gint			currentCol;
	gint			currentRow[5];
	GtkWidget		*box1, *box2;
	GtkWidget		*table;
	GtkWidget		*scrollWindow;
	gchar			*title;
	ScrapYTSearch	*yt_search;
	ScrapPSearch	*p_search;
	ScrapPSearch	*pic_search;
	ScrapPUpload	*p_upload;
	gint			max_rows;
	gchar			*username;
	gchar			*password;
	GtkListStore	*lStore;
	GtkTreeIter		iter;
} ScrapData;
struct _ScrapPUpload {
	ScrapData			*main_data;
	GtkWidget			*file_dialog;
	GDataPicasaWebFile	*file;
	GtkWidget			*dialog;
	GtkWidget			*name;
	GtkWidget			*description;
};

typedef struct _ScrapPicSearch { /* for finding pictures */
	gchar					*title;
	gchar					*uri;
	gchar					*user;
	GdkPixbuf				*thumbnail;
	ScrapData				*main_data;
	ScrapPSearch			*search_data;
	GDataQuery				*query;
	GDataPicasaWebFile		*file;
	GtkListStore			*lStore;
	GtkTreeIter				iter;
	GtkWidget				*tView;
} ScrapPicSearch;

struct _ScrapYTSearch { /* youtube search data */
	GtkWidget 			*txt_entry;
	gchar 	  			*txt;
	GtkWidget			*window;
	GDataQuery		 	*query;
	GDataYouTubeService	*service;
	gchar				*title;
	gchar				*uri;
	GdkPixbuf			*thumbnail;
	GtkWidget 			*box1, *box2;
	ScrapData			*main_data; /* <- points to a structure containing main vars */
	GtkWidget			*button;
	GtkListStore		*lStore;
	GtkTreeIter			iter;
	GtkWidget			*tView;
};

struct _ScrapPSearch { /* for finding albums */
	GtkWidget				*window;
	GDataQuery				*query;
	GDataPicasaWebService	*service;
	gchar					*title;
	gchar					*uri;
	GdkPixbuf				*thumbnail;
	GtkWidget				*box1, *box2;
	ScrapData				*main_data;
	GtkWidget				*button;
	GtkWidget				*user_entry;
	gchar					*user;
	ScrapPicSearch			*pic;
};

typedef struct _ScrapProps {
	GtkWidget	*window;
	GtkWidget	*button;
	GtkWidget	*box1, *box2;
	GtkWidget	*label;
	GtkWidget	*username_entry, *password_entry, *title_entry;
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
select_file (GtkWidget *widget, ScrapPUpload *self);

static void
got_name (GtkWidget *widget, ScrapPUpload *self);

static void
upload (GtkWidget *widget, ScrapData *first);



