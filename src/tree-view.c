/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include <base.h>

#include "dtv-gst.h"
#include "player-gst.h"
#include "player-panel.h"
#include "pref.h"
#include "scan.h"


void player_next ( Base *base )
{
	if ( base->player->next_repeat )
	{
		gst_element_set_state ( base->player->playbin, GST_STATE_NULL );

		gst_element_set_state ( base->player->playbin, GST_STATE_PLAYING );

		return;
	}

	GtkTreeModel *model = gtk_tree_view_get_model ( base->player->treeview );
	uint ind = gtk_tree_model_iter_n_children ( model, NULL );

	if ( ind < 2 )
	{
		player_stop ( base );

		return;
	}

	GtkTreeIter iter;

	gboolean valid, found = FALSE, paly = FALSE;
	for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
		  valid = gtk_tree_model_iter_next ( model, &iter ) )
	{
		char *name, *data;

		gtk_tree_model_get ( model, &iter, COL_DATA,  &data, -1 );
		gtk_tree_model_get ( model, &iter, COL_FL_CH, &name, -1 );

			if ( found )
			{
				player_stop_set_play ( base, data );

				gtk_tree_selection_select_iter ( gtk_tree_view_get_selection ( base->player->treeview ), &iter );

				g_free ( name );
				g_free ( data );

				paly = TRUE;

				g_debug ( "player_next: %s \n", data );

				break;
			}

			if ( g_str_has_suffix ( base->player->file_play, name ) )
			{
				found = TRUE;
			}

		g_free ( name );
		g_free ( data );
	}

	if ( !paly ) player_stop ( base );
}



static void treeview_to_file ( GtkTreeView *tree_view, const char *filename, gboolean tv_pl )
{
	GString *gstring = g_string_new ( ( tv_pl ) ? "# Gtv-Dvb channel format \n" : "#EXTM3U \n" );

	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

	gboolean valid;
	for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
		  valid = gtk_tree_model_iter_next ( model, &iter ) )
	{
		char *data = NULL;
		char *name = NULL;

		gtk_tree_model_get ( model, &iter, COL_DATA,  &data, -1 );
		gtk_tree_model_get ( model, &iter, COL_FL_CH, &name, -1 );

		if ( !tv_pl ) g_string_append_printf ( gstring, "#EXTINF:-1,%s\n", name );

		g_string_append_printf ( gstring, "%s\n", data );

		g_free ( name );
		g_free ( data );
	}

	GError *err = NULL;

	if ( !g_file_set_contents ( filename, gstring->str, -1, &err ) )
	{
		g_printerr ( "treeview_to_file: %s \n", err->message );

		g_error_free ( err );
	}

	g_string_free ( gstring, TRUE );
}

static void dialod_add_filter ( GtkFileChooserDialog *dialog, const char *name, const char *filter_set )
{
	GtkFileFilter *filter = gtk_file_filter_new ();

	gtk_file_filter_set_name ( filter, name );
	gtk_file_filter_add_pattern ( filter, filter_set );
	gtk_file_chooser_add_filter ( GTK_FILE_CHOOSER ( dialog ), filter );
}

static void treeview_save ( GtkTreeView *tree_view, const char *dir, const char *file, GtkWindow *window, gboolean tv_pl )
{
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );
	int ind = gtk_tree_model_iter_n_children ( model, NULL );

	if ( ind == 0 ) return;

	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
									 " ", window,   GTK_FILE_CHOOSER_ACTION_SAVE,
									 "gtk-cancel",  GTK_RESPONSE_CANCEL,
									 "gtk-save",    GTK_RESPONSE_ACCEPT,
									 NULL );

	if ( tv_pl )
		dialod_add_filter ( dialog, "conf", "*.conf" );
	else
		dialod_add_filter ( dialog, "m3u",  "*.m3u"  );

	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), "document-save" );

	gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( dialog ), dir );
	gtk_file_chooser_set_do_overwrite_confirmation ( GTK_FILE_CHOOSER ( dialog ), TRUE );
	gtk_file_chooser_set_current_name   ( GTK_FILE_CHOOSER ( dialog ), file );

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
	{
		char *filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );

			treeview_to_file ( tree_view, filename, tv_pl );

		g_free ( filename );
	}

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );
}

static void treeview_win_save_mp ( G_GNUC_UNUSED GtkButton *button, GtkTreeView *tree_view )
{
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( button ) ) );

	treeview_save ( tree_view, g_get_home_dir (), "playlist-001.m3u", window, FALSE );
}
static void treeview_win_save_tv ( G_GNUC_UNUSED GtkButton *button, GtkTreeView *tree_view )
{
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( button ) ) );

	treeview_save ( tree_view, g_get_home_dir (), "gtv-channel.conf", window, TRUE );
}
void treeview_auto_save_tv ( GtkTreeView *tree_view, const char *filename )
{	
	treeview_to_file ( tree_view, filename, TRUE );
}


static void treeview_clear_win_create ( GtkWindow *win_base, GtkTreeView *tree_view )
{
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );
	uint ind = gtk_tree_model_iter_n_children ( model, NULL );

	if ( ind == 0 ) return;

	GtkWindow *window =      (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_transient_for ( window, win_base );
	gtk_window_set_modal     ( window, TRUE );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_title     ( window, "" );

	GdkPixbuf *pixbuf = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					 "helia-clear", 48, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	gtk_window_set_icon ( window, pixbuf );

	if ( pixbuf ) g_object_unref ( pixbuf );

	gtk_widget_set_size_request ( GTK_WIDGET ( window ), 400, 150 );

	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *i_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	pixbuf = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					 "helia-warning", 48, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	GtkImage *image   = (GtkImage  *)gtk_image_new_from_pixbuf ( pixbuf );

	if ( pixbuf ) g_object_unref ( pixbuf );

	gtk_box_pack_start ( i_box, GTK_WIDGET ( image ), TRUE, TRUE, 0 );

	GtkLabel *label = (GtkLabel *)gtk_label_new ( "" );

	char *text = g_strdup_printf ( "%d", ind );

		gtk_label_set_text ( label, text );

	g_free  ( text );

	gtk_box_pack_start ( i_box, GTK_WIDGET ( label ), TRUE, TRUE, 10 );

	pixbuf = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
			"helia-clear", 48, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	image = (GtkImage  *)gtk_image_new_from_pixbuf ( pixbuf );

	if ( pixbuf ) g_object_unref ( pixbuf );

	gtk_box_pack_start ( i_box, GTK_WIDGET ( image ), TRUE, TRUE, 0 );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( i_box ), TRUE, TRUE, 5 );

	GtkButton *button_clear = (GtkButton *)gtk_button_new_from_icon_name ( "helia-ok", GTK_ICON_SIZE_SMALL_TOOLBAR );
	g_signal_connect_swapped ( button_clear, "clicked", G_CALLBACK ( gtk_list_store_clear ), GTK_LIST_STORE ( model ) );
	g_signal_connect_swapped ( button_clear, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );

	gtk_box_pack_end ( h_box, GTK_WIDGET ( button_clear ), TRUE, TRUE, 5 );

	GtkButton *button_close = (GtkButton *)gtk_button_new_from_icon_name ( "helia-exit", GTK_ICON_SIZE_SMALL_TOOLBAR );
	g_signal_connect_swapped ( button_close, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );

	gtk_box_pack_end ( h_box, GTK_WIDGET ( button_close ), TRUE, TRUE, 5 );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );

	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 5 );
	gtk_container_add ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );

	gtk_widget_show_all ( GTK_WIDGET ( window ) );
}

static void treeview_win_clear ( G_GNUC_UNUSED GtkButton *button, GtkTreeView *tree_view )
{
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( button ) ) );

	treeview_clear_win_create ( window, tree_view );
}


static void treeview_reread ( GtkTreeView *tree_view )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

	int row_count = 1;
	gboolean valid;

	for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
		  valid = gtk_tree_model_iter_next ( model, &iter ) )
	{
		gtk_list_store_set ( GTK_LIST_STORE ( model ), &iter, COL_NUM, row_count++, -1 );
	}
}

static void treeview_up_down ( GtkTreeView *tree_view, gboolean up_dw )
{
	GtkTreeIter iter, iter_c;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );
	int ind = gtk_tree_model_iter_n_children ( model, NULL );

	if ( ind < 2 ) return;

	if ( gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( tree_view ), NULL, &iter ) )
	{
		gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( tree_view ), NULL, &iter_c );

		if ( up_dw )
		if ( gtk_tree_model_iter_previous ( model, &iter ) )
			gtk_list_store_move_before ( GTK_LIST_STORE ( model ), &iter_c, &iter );

		if ( !up_dw )
		if ( gtk_tree_model_iter_next ( model, &iter ) )
			gtk_list_store_move_after ( GTK_LIST_STORE ( model ), &iter_c, &iter );

		treeview_reread ( tree_view );
	}
	else if ( gtk_tree_model_get_iter_first ( model, &iter ) )
	{
		gtk_tree_selection_select_iter ( gtk_tree_view_get_selection (tree_view), &iter);
	}
}

static void treeview_remv ( GtkTreeView *tree_view )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

	if ( gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( tree_view ), NULL, &iter ) )
	{
		gtk_list_store_remove ( GTK_LIST_STORE ( model ), &iter );

		treeview_reread ( tree_view );
	}
}

static void treeview_win_goup ( G_GNUC_UNUSED GtkButton *button, GtkTreeView *tree_view )
{
	treeview_up_down ( tree_view, TRUE  );
}

static void treeview_win_down ( G_GNUC_UNUSED GtkButton *button, GtkTreeView *tree_view )
{
	treeview_up_down ( tree_view, FALSE );
}

static void treeview_win_remv ( G_GNUC_UNUSED GtkButton *button, GtkTreeView *tree_view )
{
	treeview_remv ( tree_view );
}

static void treeview_open_dir ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	dialog_open_dir ( base );
}

static void treeview_repeat ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	base->player->next_repeat = !base->player->next_repeat;

	GdkPixbuf *pixbuf = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					  base->player->next_repeat ? "helia-set" : "helia-repeat", 16, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	GtkImage *image = (GtkImage  *)gtk_image_new_from_pixbuf ( pixbuf );

	gtk_button_set_image ( button, GTK_WIDGET ( image ) );

	if ( pixbuf ) g_object_unref ( pixbuf );
}

static void treeview_scan ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	scan_win_create ( base );
}

static void treeview_ve ( G_GNUC_UNUSED GtkButton *button, G_GNUC_UNUSED Base *base )
{
	
}

GtkBox * treeview_box ( Base *base, G_GNUC_UNUSED GtkWindow *window, GtkTreeView *tree_view, GtkBox *box, gboolean player_tv )
{
	GtkBox *v_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

	GtkBox *h_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( h_box,  5 );

	gtk_widget_set_margin_start  ( GTK_WIDGET ( h_box ), 5 );
	gtk_widget_set_margin_end    ( GTK_WIDGET ( h_box ), 5 );
	gtk_widget_set_margin_top    ( GTK_WIDGET ( h_box ), 5 );
	gtk_widget_set_margin_bottom ( GTK_WIDGET ( h_box ), 5 );

	const struct PanelTrwNameIcon { const char *name; void (* activate)(); } NameIcon_n[] =
	{
		{ "helia-up", 			treeview_win_goup 	},
		{ "helia-down", 		treeview_win_down 	},
		{ "helia-remove", 		treeview_win_remv 	},
		{ "helia-clear", 		treeview_win_clear  },

		{ "helia-save", 		( player_tv ) ? treeview_win_save_mp : treeview_win_save_tv }
	};

	uint i = 0;
	for ( i = 0; i < G_N_ELEMENTS ( NameIcon_n ); i++ )
	{
		if ( i == 4 )
		{
			h_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
			gtk_box_set_spacing ( h_box,  5 );

			gtk_widget_set_margin_start  ( GTK_WIDGET ( h_box ), 5 );
			gtk_widget_set_margin_end    ( GTK_WIDGET ( h_box ), 5 );
			gtk_widget_set_margin_bottom ( GTK_WIDGET ( h_box ), 5 );
		}

		if ( i == 0 ) gtk_box_pack_start ( v_box, GTK_WIDGET ( h_box  ), FALSE, FALSE, 0 );

		GtkButton *button = base_set_image_button ( NameIcon_n[i].name, 16 );
		g_signal_connect ( button, "clicked", G_CALLBACK ( NameIcon_n[i].activate ), tree_view );

		gtk_box_pack_start ( h_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );
	}

	if ( player_tv )
	{
		GtkButton *button = base_set_image_button ( "helia-add-folder", 16 );
		g_signal_connect ( button, "clicked", G_CALLBACK ( treeview_open_dir ), base );

		gtk_box_pack_start ( h_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );

		button = base_set_image_button ( "helia-repeat", 16 );
		g_signal_connect ( button, "clicked", G_CALLBACK ( treeview_repeat ), base );

		gtk_box_pack_start ( h_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );
	}
	else
	{
		GtkButton *button = base_set_image_button ( "helia-display", 16 );
		g_signal_connect ( button, "clicked", G_CALLBACK ( treeview_scan ), base );

		gtk_box_pack_start ( h_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );

		button = base_set_image_button ( "helia-window", 16 );
		g_signal_connect ( button, "clicked", G_CALLBACK ( treeview_ve ), base );

		gtk_box_pack_start ( h_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );
	}

	GtkButton *button = base_set_image_button ( "helia-exit", 16 );
	g_signal_connect_swapped ( button, "clicked", G_CALLBACK ( gtk_widget_hide ), box );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );

	gtk_box_pack_start ( v_box, GTK_WIDGET ( h_box  ), FALSE, FALSE, 0 );

	return v_box;
}



static void treeview_append ( GtkTreeView *tree_view, Base *base, gboolean play, const char *name, const char *data )
{
    GtkTreeIter iter;

    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

    uint ind = gtk_tree_model_iter_n_children ( model, NULL );

    gtk_list_store_append ( GTK_LIST_STORE ( model ), &iter);
    gtk_list_store_set    ( GTK_LIST_STORE ( model ), &iter,
                            COL_NUM, ind+1,
                            COL_FL_CH, name,
                            COL_DATA,  data,
                            -1 );

	if ( play && base->player->playbin && GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_NULL )
	{
		player_stop_set_play ( base, data );

		gtk_tree_selection_select_iter ( gtk_tree_view_get_selection ( GTK_TREE_VIEW ( tree_view ) ), &iter );

		g_debug ( "treeview_append: PLAY %s \n", data );
	}
}

static void treeview_add_m3u ( Base *base, const char *file )
{
    char  *contents = NULL;
    GError *err     = NULL;

    if ( g_file_get_contents ( file, &contents, 0, &err ) )
    {
        char **lines = g_strsplit ( contents, "\n", 0 );

        uint i = 0; for ( i = 0; lines[i] != NULL; i++ )
        //for ( i = 0; lines[i] != NULL && *lines[i]; i++ )
        {
			if ( g_str_has_prefix ( lines[i], "#EXTM3U" ) || strlen ( lines[i] ) < 2 ) continue;

			if ( g_str_has_prefix ( lines[i], "#EXTINF" ) )
			{
				char **lines_info = g_strsplit ( lines[i], ",", 0 );

					treeview_append ( base->player->treeview, base, ( i == 1 ) ? TRUE : FALSE, 
									  g_strstrip ( lines_info[1] ), g_strstrip ( lines[i+1] ) );

				g_strfreev ( lines_info );
				i++;
			}
			else
			{
				if ( g_str_has_prefix ( lines[i], "#" ) ) continue;

				char *name = g_path_get_basename ( lines[i] );

					treeview_append ( base->player->treeview, base, ( i == 1 ) ? TRUE : FALSE, 
									  g_strstrip ( name ), g_strstrip ( lines[i] ) );

				g_free ( name );
			}
        }

        g_strfreev ( lines );
        free ( contents );
    }
    else
    {
        base_message_dialog ( "", err->message, GTK_MESSAGE_ERROR, base->window );

		g_error_free ( err );
	}
}

static gboolean treeview_media_filter ( const char *file_name )
{
	gboolean res  = FALSE;
	GError *error = NULL;

	GFile *file = g_file_new_for_path ( file_name );
	GFileInfo *file_info = g_file_query_info ( file, "standard::*", 0, NULL, &error );

	const char *content_type = g_file_info_get_content_type ( file_info );

	if ( g_str_has_prefix ( content_type, "audio" ) || g_str_has_prefix ( content_type, "video" ) ) res =  TRUE;

	char *text = g_utf8_strdown ( file_name, -1 );

	if ( g_str_has_suffix ( text, "asx" ) || g_str_has_suffix ( text, "pls" ) ) res = FALSE;

	g_free ( text );

	g_object_unref ( file_info );
	g_object_unref ( file );

	return res;
}

void treeview_add_file ( Base *base, const char *path, gboolean filter_set, gboolean play )
{
	if ( filter_set && !treeview_media_filter ( path ) ) return;

	char *name_down = g_utf8_strdown ( path, -1 );

	if ( g_str_has_suffix ( name_down, "m3u" ) )
	{
		treeview_add_m3u ( base, path );
	}
	else
	{
		char *basename = g_path_get_basename ( path );

			treeview_append ( base->player->treeview, base, play, basename, path );

		free ( basename );
	}

	free ( name_down );
}

static int _sort_func_list ( gconstpointer a, gconstpointer b )
{
	return g_utf8_collate ( a, b );
}

static void treeview_slist_sort ( GList *list, Base *base )
{
	GList *list_sort = g_list_sort ( list, _sort_func_list );
	uint i = 0;

	while ( list_sort != NULL )
	{
		treeview_add_file ( base, (char *)list_sort->data, TRUE, ( i == 0 ) ? TRUE : FALSE );

		list_sort = list_sort->next;
		i++;
	}

	g_list_free_full ( list_sort, (GDestroyNotify) g_free );
}

void treeview_add_dir ( Base *base, const char *dir_path )
{
	GDir *dir = g_dir_open ( dir_path, 0, NULL );

	GList *list = NULL; // on sort

	if ( dir )
	{
		const char *name = NULL;

		while ( ( name = g_dir_read_name ( dir ) ) != NULL )
		{
			char *path_name = g_strconcat ( dir_path, "/", name, NULL );

			if ( g_file_test ( path_name, G_FILE_TEST_IS_DIR ) )
			{
				treeview_add_dir ( base, path_name ); // Recursion!
			}

			if ( g_file_test ( path_name, G_FILE_TEST_IS_REGULAR ) )
			{
				list = g_list_append ( list, path_name ); // on sort

				// treeview_add_file ( base, path_name, TRUE, TRUE ); // of sort
			}

			// free ( path_name ); // of sort
		}

		g_dir_close ( dir );
	}
	else
	{
		g_printerr ( "treeview_add_dir: opening directory %s failed\n", dir_path );
	}

	treeview_slist_sort ( list, base ); // on sort

	g_list_free_full ( list, (GDestroyNotify) g_free ); // on sort
}



//

static void treeview_append_dtv ( GtkTreeView *tree_view, const char *name, const char *data )
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );
    uint ind = gtk_tree_model_iter_n_children ( model, NULL );

    gtk_list_store_append ( GTK_LIST_STORE ( model ), &iter);
    gtk_list_store_set    ( GTK_LIST_STORE ( model ), &iter,
                            COL_NUM, ind+1,
                            COL_FL_CH, name,
                            COL_DATA,  data,
                            -1 );	
}

void treeview_add_dtv ( Base *base, const char *file )
{
    char  *contents = NULL;
    GError *err     = NULL;

    if ( g_file_get_contents ( file, &contents, 0, &err ) )
    {
        char **lines = g_strsplit ( contents, "\n", 0 );

        uint i = 0; for ( i = 0; lines[i] != NULL; i++ )
        //for ( i = 0; lines[i] != NULL && *lines[i]; i++ )
        {
			if ( g_str_has_prefix ( lines[i], "#" ) || strlen ( lines[i] ) < 2 ) continue;

			char **data = g_strsplit ( lines[i], ":", 0 );

				treeview_append_dtv ( base->dtv->treeview, data[0], lines[i] );

			g_strfreev ( data );
        }

        g_strfreev ( lines );
        free ( contents );
    }
    else
    {
        base_message_dialog ( "", err->message, GTK_MESSAGE_ERROR, base->window );

		g_error_free ( err );
	}
}

static void treeview_row_activated_dtv ( GtkTreeView *tree_view, GtkTreePath *path, G_GNUC_UNUSED GtkTreeViewColumn *column, Base *base )
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

    if ( gtk_tree_model_get_iter ( model, &iter, path ) )
    {
        char *data = NULL;

            gtk_tree_model_get ( model, &iter, COL_DATA, &data, -1 );

            dtv_stop_set_play ( base, data );

        free ( data );
    }
}

//



static void treeview_row_activated ( GtkTreeView *tree_view, GtkTreePath *path, G_GNUC_UNUSED GtkTreeViewColumn *column, Base *base )
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

    if ( gtk_tree_model_get_iter ( model, &iter, path ) )
    {
        char *data = NULL;

            gtk_tree_model_get ( model, &iter, COL_DATA, &data, -1 );

            player_stop_set_play ( base, data );

        free ( data );
    }
}

static void create_columns ( GtkTreeView *tree_view, GtkCellRenderer *renderer, const char *name, int column_id, gboolean col_vis )
{
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ( name, renderer, "text", column_id, NULL );

    gtk_tree_view_append_column ( tree_view, column );
    gtk_tree_view_column_set_visible ( column, col_vis );
}

static void add_columns ( GtkTreeView *tree_view, const char *title )
{
    struct col_title_list { const char *title; gboolean vis; } col_title_list_n[] = 
    {
		{ " № ",  TRUE  },
		{ title,  TRUE  },
		{ "Data", FALSE }
	};

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();

	uint c = 0;

    for ( c = 0; c < NUM_COLS; c++ )
    {
        create_columns ( tree_view, renderer, col_title_list_n[c].title, c, col_title_list_n[c].vis );
	}
}

GtkTreeView * create_treeview ( Base *base, const char *title, gboolean player_tv )
{
	GtkListStore *store   = (GtkListStore *)gtk_list_store_new ( 3, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING );

    GtkTreeView *treeview = (GtkTreeView *)gtk_tree_view_new_with_model ( GTK_TREE_MODEL ( store ) );

	gtk_tree_view_set_search_column ( treeview, COL_FL_CH );

    add_columns ( treeview, title );

	if ( player_tv )
		g_signal_connect ( treeview, "row-activated", G_CALLBACK ( treeview_row_activated     ), base );
	else
		g_signal_connect ( treeview, "row-activated", G_CALLBACK ( treeview_row_activated_dtv ), base );

    return treeview;
}

GtkScrolledWindow * create_scroll_win ( GtkTreeView *tree_view, uint set_size )
{
    GtkScrolledWindow *scroll = (GtkScrolledWindow *)gtk_scrolled_window_new ( NULL, NULL );

    gtk_scrolled_window_set_policy ( scroll, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
    gtk_widget_set_size_request ( GTK_WIDGET ( scroll ), set_size, -1 );

    gtk_container_add ( GTK_CONTAINER ( scroll ), GTK_WIDGET ( tree_view ) );

    return scroll;
}
