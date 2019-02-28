/*
* Copyright 2019 Stepan Perun
* This program is free software.
*
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include <base.h>

#include "tree-view.h"
#include "lang.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static char * pref_get_prop ( const char *prop );


void about_win ( GtkWindow *window )
{
	GtkAboutDialog *dialog = (GtkAboutDialog *)gtk_about_dialog_new ();
	gtk_window_set_transient_for ( (GtkWindow *)dialog, window );

	const char *authors[]   = { "Stepan Perun",   " ", NULL };
	const char *artists[]   = { "Itzik Gur",      " ", NULL };
	const char *translators = "Anton Midyukov \nHeimen Stoffels \nPavel Fric \n";
	const char *license     = "This program is free software. \n\nGNU Lesser General Public License \nwww.gnu.org/licenses/lgpl.html";

	gtk_about_dialog_set_program_name ( dialog, "Helia" );
	gtk_about_dialog_set_version ( dialog, "5.5" );
	gtk_about_dialog_set_license ( dialog, license );
	gtk_about_dialog_set_authors ( dialog, authors );
	gtk_about_dialog_set_artists ( dialog, artists );
	gtk_about_dialog_set_translator_credits ( dialog, translators );
	gtk_about_dialog_set_website ( dialog,   "https://git.opendesktop.org/vl-nix/helia" );
	gtk_about_dialog_set_copyright ( dialog, "Copyright 2019 Helia" );
	gtk_about_dialog_set_comments  ( dialog, "Media Player & IPTV & Digital TV \nDVB-T2/S2/C, ATSC, DTMB, ISDB" );

	gtk_about_dialog_set_logo_icon_name ( dialog, "helia-logo" );

	gtk_dialog_run ( GTK_DIALOG (dialog) );

	gtk_widget_destroy ( GTK_WIDGET (dialog) );
}

/* Returns a newly-allocated string holding the result. Free with free() */
char * pref_open_file ( Base *base, const char *path )
{
	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
									" ",  base->window, GTK_FILE_CHOOSER_ACTION_OPEN,
									"gtk-cancel", GTK_RESPONSE_CANCEL,
									"gtk-open",   GTK_RESPONSE_ACCEPT,
									NULL );

	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), "document-open" );

	gtk_file_chooser_set_current_folder  ( GTK_FILE_CHOOSER ( dialog ), path );
	gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER ( dialog ), FALSE );

	char *filename = NULL;

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
		filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

	return filename;
}

/* Returns a GSList containing the filenames. Free the returned list with g_slist_free(), and the filenames with g_free(). */
GSList * pref_open_files ( Base *base, const char *path )
{
	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
									" ",  base->window, GTK_FILE_CHOOSER_ACTION_OPEN,
									"gtk-cancel", GTK_RESPONSE_CANCEL,
									"gtk-open",   GTK_RESPONSE_ACCEPT,
									NULL );

	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), "document-open" );

	gtk_file_chooser_set_current_folder  ( GTK_FILE_CHOOSER ( dialog ), path );
	gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER ( dialog ), TRUE );

	GSList *files = NULL;

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
		files = gtk_file_chooser_get_filenames ( GTK_FILE_CHOOSER ( dialog ) );

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

	return files;
}

/* Returns a newly-allocated string holding the result. Free with free() */
char * pref_open_dir ( Base *base, const char *path )
{
	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
									" ",  base->window, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
									"gtk-cancel", GTK_RESPONSE_CANCEL,
									"gtk-apply",  GTK_RESPONSE_ACCEPT,
									NULL );

	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), "folder-open" );

	gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( dialog ), path );

	char *dirname = NULL;

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
		dirname = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

	return dirname;
}

void add_arg ( GFile **files, int n_files, Base *base )
{
	int i = 0;

	for ( i = 0; i < n_files; i++ )
	{
		char *path = g_file_get_path ( files[i] );

		if ( g_file_test ( path, G_FILE_TEST_IS_DIR ) )
		{
			treeview_add_dir ( base, path );
		}

		if ( g_file_test ( path, G_FILE_TEST_IS_REGULAR ) )
		{
			treeview_add_file ( base, path, TRUE, ( i == 0 ) ? TRUE : FALSE );
		}

		g_free ( path );
	}	
}

void dialog_open_dir ( Base *base )
{
	char *path = pref_open_dir ( base, g_get_home_dir () );

		if ( path == NULL ) return;

		treeview_add_dir ( base, path );

	g_free ( path );
}

void dialog_open_files ( Base *base )
{
	GSList *files = pref_open_files ( base, g_get_home_dir () );
	uint i = 0;

	if ( files == NULL ) return;

	while ( files != NULL )
	{
		treeview_add_file ( base, files->data, FALSE, ( i == 0 ) ? TRUE : FALSE );

		files = files->next;
		i++;
	}

	g_slist_free_full ( files, (GDestroyNotify) g_free );
}

void pref_read_config ( Base *base )
{
	uint n = 0;
	char *contents;

	GError *err = NULL;

	if ( g_file_get_contents ( base->helia_conf, &contents, 0, &err ) )
	{
		char **lines = g_strsplit ( contents, "\n", 0 );

		for ( n = 0; lines[n] != NULL; n++ )
		{
			if ( !g_strrstr ( lines[n], "=" ) ) continue;

			char **key_val = g_strsplit ( lines[n], "=", 0 );

			if ( g_strrstr ( lines[n], "theme" ) )
				g_object_set ( gtk_settings_get_default (), key_val[0], key_val[1], NULL );

			if ( g_strrstr ( lines[n], "record-dir" ) )
			{
				if ( base->rec_dir ) g_free ( base->rec_dir );
				base->rec_dir = g_strdup ( key_val[1] );
			}

			if ( g_strrstr ( lines[n], "opacity-control" ) )
				base->opacity_panel = (double)( atoi ( key_val[1] ) ) / 100;

			if ( g_strrstr ( lines[n], "opacity-equalizer" ) )
				base->opacity_eq = (double)( atoi ( key_val[1] ) ) / 100;

			if ( g_strrstr ( lines[n], "opacity-window" ) )
				base->opacity_win = (double)( atoi ( key_val[1] ) ) / 100;

			if ( g_strrstr ( lines[n], "resize-icon" ) )
				base->size_icon = atoi ( key_val[1] );

			// g_debug ( "pref_read_config:: Set %s -> %s", key_val[0], key_val[1]);

			g_strfreev ( key_val );
		}

		g_strfreev ( lines );
		g_free ( contents );
	}
	else
	{
		g_critical ( "pref_read_config:: %s\n", err->message );
		g_error_free ( err );
	}
}

void pref_save_config ( Base *base )
{
	char *conf_t = pref_get_prop ( "gtk-theme-name" );

	GString *gstring = g_string_new ( "# Gtv-Dvb conf \n" );

	g_string_append_printf ( gstring, "record-dir=%s\n",          base->rec_dir  );
	g_string_append_printf ( gstring, "gtk-theme-name=%s\n",      conf_t  );
	g_string_append_printf ( gstring, "opacity-control=%d\n",     (int)( base->opacity_panel * 100 ) );
	g_string_append_printf ( gstring, "opacity-equalizer=%d\n",   (int)( base->opacity_eq  * 100 ) );
	g_string_append_printf ( gstring, "opacity-window=%d\n",      (int)( base->opacity_win * 100 ) );
	g_string_append_printf ( gstring, "resize-icon=%d\n",         base->size_icon );

	GError *err = NULL;

	if ( !g_file_set_contents ( base->helia_conf, gstring->str, -1, &err ) )
	{
		g_critical ( "pref_save_config:: %s\n", err->message );
		g_error_free ( err );
	}

	g_string_free ( gstring, TRUE );

	g_free ( conf_t );
}



static char * pref_get_prop ( const char *prop )
{
	char *name = NULL;

	g_object_get ( gtk_settings_get_default (), prop, &name, NULL );

	return name;
}
static void pref_set_prop ( const char *prop, char *path )
{
	char *i_file = g_strconcat ( path, "/index.theme", NULL );

	if ( g_file_test ( i_file, G_FILE_TEST_EXISTS ) )
	{
		char *name = g_path_get_basename ( path );

			g_object_set ( gtk_settings_get_default (), prop, name, NULL );

		g_free ( name );
	}

	g_free ( i_file );
}

static void pref_set_theme ( GtkEntry *entry, G_GNUC_UNUSED GtkEntryIconPosition icon_pos, G_GNUC_UNUSED GdkEvent *event, Base *base )
{
	char *path = pref_open_dir ( base, "/usr/share/themes" );

	if ( path == NULL ) return;

	pref_set_prop ( "gtk-theme-name", path );

	char *name = g_path_get_basename ( path );

		gtk_entry_set_text ( entry, name );

	g_free ( name );

	g_free ( path );
}

static void pref_set_rec_dir ( GtkEntry *entry, G_GNUC_UNUSED GtkEntryIconPosition icon_pos, G_GNUC_UNUSED GdkEvent *event, Base *base )
{
	char *path = pref_open_dir ( base, g_get_home_dir () );

	if ( path == NULL ) return;

	if ( base->rec_dir ) g_free ( base->rec_dir );

	base->rec_dir = path;

	gtk_entry_set_text ( entry, base->rec_dir );
}

static void pref_changed_opacity_panel ( G_GNUC_UNUSED GtkRange *range, Base *base )
{
	base->opacity_panel = gtk_range_get_value ( range );
}
static void pref_changed_opacity_eq ( G_GNUC_UNUSED GtkRange *range, Base *base )
{
	base->opacity_eq = gtk_range_get_value ( range );
}
static void pref_changed_opacity_win ( G_GNUC_UNUSED GtkRange *range, Base *base )
{
	base->opacity_win = gtk_range_get_value ( range );
	gtk_widget_set_opacity ( GTK_WIDGET ( base->window ), base->opacity_win );
}
static void pref_changed_opacity_base_win ( G_GNUC_UNUSED GtkRange *range, GtkWindow *window )
{
	gdouble opacity = gtk_range_get_value ( range );
	gtk_widget_set_opacity ( GTK_WIDGET ( window ), opacity );
}
static void pref_changed_resize_icon ( GtkRange *range, Base *base )
{
	base->size_icon = (guint)gtk_range_get_value ( range );
}

static void pref_create_entry ( Base *base, const char *text, const char *set_text, void (*f)(), GtkBox *h_box )
{
	GtkImage *image = base_create_image ( text, 32 );

	gtk_widget_set_halign ( GTK_WIDGET ( image ), GTK_ALIGN_START );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( image ), TRUE, TRUE, 0 );

	GtkEntry *entry = (GtkEntry *)gtk_entry_new ();
	gtk_entry_set_text ( entry, set_text );

	g_object_set ( entry, "editable", FALSE, NULL );
	gtk_entry_set_icon_from_icon_name ( entry, GTK_ENTRY_ICON_SECONDARY, "folder" );
	g_signal_connect ( entry, "icon-press", G_CALLBACK ( f ), base );

	gtk_widget_set_size_request ( GTK_WIDGET ( entry ), 250, -1 );
	gtk_widget_set_halign ( GTK_WIDGET ( entry ), GTK_ALIGN_END );
	gtk_box_pack_start ( h_box, GTK_WIDGET ( entry ), TRUE, TRUE, 0 );
}

static void pref_create_scale ( Base *base, const char *text, double val, double min, double max, double step, 
									void (*f)(), GtkBox *h_box, GtkWindow *window )
{
	GtkImage *image = base_create_image ( text, 32 );
	gtk_widget_set_halign ( GTK_WIDGET ( image ), GTK_ALIGN_START );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( image ), TRUE, TRUE, 0 );

	GtkLabel *label = (GtkLabel *)gtk_label_new ( "" );
	gtk_widget_set_size_request ( GTK_WIDGET ( label ), 50, -1 );
	gtk_box_pack_start ( h_box, GTK_WIDGET ( label ), TRUE, TRUE, 0 );

	GtkScale *scale = (GtkScale *)gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, min, max, step );
	gtk_range_set_value ( GTK_RANGE ( scale ), val );
	g_signal_connect ( scale, "value-changed", G_CALLBACK ( f ), base  );
	if ( window ) g_signal_connect ( scale, "value-changed", G_CALLBACK ( pref_changed_opacity_base_win ), window );

	gtk_widget_set_size_request ( GTK_WIDGET ( scale ), 250, -1 );
	gtk_widget_set_halign ( GTK_WIDGET ( scale ), GTK_ALIGN_END );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( scale ), TRUE, TRUE, 0 );
}

static void pref_changed_combo_lang ( GtkComboBoxText *combo_box, Base *base )
{
	base->num_lang = gtk_combo_box_get_active ( GTK_COMBO_BOX ( combo_box ) );

	GtkTreeViewColumn *column = gtk_tree_view_get_column ( base->player->treeview, COL_FL_CH );
	gtk_tree_view_column_set_title ( column, lang_set ( base, "Files" ) );

	column = gtk_tree_view_get_column ( base->dtv->treeview, COL_FL_CH );
	gtk_tree_view_column_set_title ( column, lang_set ( base, "Channels" ) );
}


static void pref_create_combo ( Base *base, const char *text, void (*f)(), GtkBox *h_box )
{
	GtkImage *image = base_create_image ( text, 32 );
	gtk_widget_set_halign ( GTK_WIDGET ( image ), GTK_ALIGN_START );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( image ), TRUE, TRUE, 0 );

	GtkLabel *label = (GtkLabel *)gtk_label_new ( "" );
	gtk_widget_set_size_request ( GTK_WIDGET ( label ), 50, -1 );
	gtk_box_pack_start ( h_box, GTK_WIDGET ( label ), TRUE, TRUE, 0 );

	GtkComboBoxText *combo = (GtkComboBoxText *)gtk_combo_box_text_new ();
	
	lang_add_combo ( combo );
	
	gtk_combo_box_set_active ( GTK_COMBO_BOX ( combo ), base->num_lang );
	g_signal_connect ( combo, "changed", G_CALLBACK ( f ), base );

	gtk_widget_set_size_request ( GTK_WIDGET ( combo ), 250, -1 );
	gtk_widget_set_halign ( GTK_WIDGET ( combo ), GTK_ALIGN_END );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( combo ), TRUE, TRUE, 0 );
}


void pref_win ( Base *base )
{
	GtkWindow *window =      (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_transient_for ( window, base->window );
	gtk_window_set_modal     ( window, TRUE );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_title     ( window, "" );

	GdkPixbuf *pixbuf = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					 "helia-pref", 48, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	gtk_window_set_icon ( window, pixbuf );

	if ( pixbuf ) g_object_unref ( pixbuf );

	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );	
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( gtk_label_new ( " " ) ), FALSE, FALSE, 0 );

	h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

		pref_create_entry ( base, "helia-folder-rec", base->rec_dir, pref_set_rec_dir, h_box );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 0 );

	h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

		char *set_text = pref_get_prop ( "gtk-theme-name" );

		pref_create_entry ( base, "helia-style", set_text, pref_set_theme, h_box );

		g_free ( set_text );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 0 );

	h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	
		pref_create_combo ( base, "helia-locale", pref_changed_combo_lang, h_box );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 0 );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( gtk_label_new ( " " ) ), FALSE, FALSE, 0 );

	struct data_a { const char *text; double val; double min; double max; double step; void (*f)(); GtkWindow *win; } data_a_n[] =
	{
		{ "helia-panel",  base->opacity_panel, 0.4, 1.0, 0.01, pref_changed_opacity_panel, NULL   },
		{ "helia-eqa",    base->opacity_eq,  0.4, 1.0, 0.01, pref_changed_opacity_eq,  NULL   },
		{ "helia-window", base->opacity_win, 0.4, 1.0, 0.01, pref_changed_opacity_win, window },
		{ "helia-size",   base->size_icon,   8,   48,     1, pref_changed_resize_icon, NULL   }
	};

	uint d = 0;
	for ( d = 0; d < G_N_ELEMENTS ( data_a_n ); d++ )
	{
		h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

		pref_create_scale ( base, data_a_n[d].text, data_a_n[d].val, data_a_n[d].min, data_a_n[d].max, data_a_n[d].step, data_a_n[d].f, h_box, data_a_n[d].win );

		gtk_box_pack_start ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 0 );
	}

	gtk_box_pack_start ( m_box, GTK_WIDGET ( gtk_label_new ( " " ) ), FALSE, FALSE, 0 );

	h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	GtkButton *button_close = (GtkButton *)gtk_button_new_from_icon_name ( "helia-exit", GTK_ICON_SIZE_BUTTON );
	g_signal_connect_swapped ( button_close, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );
	gtk_box_pack_end ( h_box, GTK_WIDGET ( button_close ), TRUE, TRUE, 0 );

	gtk_box_pack_end ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );

	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 10 );
	gtk_container_add   ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );
	gtk_widget_show_all ( GTK_WIDGET ( window ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( window ), base->opacity_win );
}

void keyb_win ( Base *base )
{
	GtkWindow *window =      (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_transient_for ( window, base->window );
	gtk_window_set_modal     ( window, TRUE );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_title     ( window, "" );
	gtk_window_set_default_size ( window, 300, 400 );

	GdkPixbuf *pixbuf = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					 "helia-keyb", 48, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	gtk_window_set_icon ( window, pixbuf );

	if ( pixbuf ) g_object_unref ( pixbuf );


	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );	
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	GtkBox *g_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( g_box ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( g_box ), 10 );

	GtkGrid *grid = (GtkGrid *)gtk_grid_new();
	gtk_grid_set_column_homogeneous ( GTK_GRID ( grid ), TRUE );
	gtk_grid_set_row_homogeneous    ( GTK_GRID ( grid ) ,TRUE );
	gtk_box_pack_start ( g_box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );

	const struct data_a { const char *image; const char *accel; } data_a_n[] =
	{
		{ "helia-editor", 		"Ctrl + H" },
		{ "helia-stop", 		"Ctrl + X" },
		{ "helia-muted", 		"Ctrl + M" },
		{ NULL, NULL },
		{ "helia-add",          "Ctrl + O" },
		{ "helia-add-folder",   "Ctrl + D" },
		{ "helia-play",   		"␣"        },
		{ "helia-pause",  		"␣"        },
		{ "helia-frame", 		"."        }
	};

	uint d = 0;
	for ( d = 0; d < G_N_ELEMENTS ( data_a_n ); d++ )
	{
		if ( !data_a_n[d].image )
		{
			gtk_grid_attach ( GTK_GRID ( grid ), gtk_label_new ( "" ), 0, d, 1, 1 );
			continue;
		}

		GtkImage *image =  base_create_image ( data_a_n[d].image, 32 );
		gtk_widget_set_halign ( GTK_WIDGET ( image ), GTK_ALIGN_START );
		gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( image ), 0, d, 1, 1 );

		GtkLabel *label = (GtkLabel *)gtk_label_new ( data_a_n[d].accel );
		gtk_widget_set_halign ( GTK_WIDGET ( label ), GTK_ALIGN_END );
		gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( label ), 1, d, 1, 1 );
	}

	gtk_box_pack_start ( m_box, GTK_WIDGET ( g_box ), TRUE, TRUE, 0 );

	h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	GtkButton *button_close = (GtkButton *)gtk_button_new_from_icon_name ( "helia-exit", GTK_ICON_SIZE_BUTTON );
	g_signal_connect_swapped ( button_close, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );
	gtk_box_pack_end ( h_box, GTK_WIDGET ( button_close ), TRUE, TRUE, 0 );

	gtk_box_pack_end ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );

	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 10 );
	gtk_container_add   ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );
	gtk_widget_show_all ( GTK_WIDGET ( window ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( window ), base->opacity_win );
}
