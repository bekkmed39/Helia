/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include <base.h>

#include "dtv-gst.h"
#include "dtv-win.h"
#include "player-gst.h"
#include "player-win.h"
#include "tree-view.h"
#include "player-slider.h"
#include "pref.h"
#include "scan.h"
#include "lang.h"

#include <gdk/gdkkeysyms.h>
#include <glib/gstdio.h>


typedef struct _FuncAction FuncAction;

struct _FuncAction
{
	void (*f)(); 
	const char *func_name;

	uint mod_key;
	uint gdk_key;
};


static GtkBox *mn_vbox, *bs_vbox, *mp_vbox, *tv_vbox;


/* Returns a newly-allocated string holding the result. Free with free() */
char * base_uri_get_path ( const char *uri )
{
	char *path = NULL;

	GFile *file = g_file_new_for_uri ( uri );

		path = g_file_get_path ( file );

	g_object_unref ( file );

	return path;
}

void base_message_dialog ( const char *f_error, const char *file_or_info, GtkMessageType mesg_type, GtkWindow *window )
{
	GtkMessageDialog *dialog = ( GtkMessageDialog *)gtk_message_dialog_new (
								 window,    GTK_DIALOG_MODAL,
								 mesg_type, GTK_BUTTONS_CLOSE,
								 "%s\n%s",  f_error, file_or_info );

	gtk_dialog_run     ( GTK_DIALOG ( dialog ) );
	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );
}

GtkImage * base_create_image ( const char *icon, uint size )
{
	GdkPixbuf *pixbuf = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					  icon, size, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	GtkImage *image   = (GtkImage *)gtk_image_new_from_pixbuf ( pixbuf );
	gtk_image_set_pixel_size ( image, size );

	if ( pixbuf ) g_object_unref ( pixbuf );

	return image;
}

GtkButton * base_set_image_button ( const char *icon, uint size )
{
	GtkButton *button = (GtkButton *)gtk_button_new ();

	GtkImage *image = base_create_image ( icon, size );

	gtk_button_set_image ( button, GTK_WIDGET ( image ) );

	return button;
}

void base_create_image_button ( GtkBox *box, const char *icon, uint size, void (*f)(), Base *base )
{
	GtkButton *button = base_set_image_button ( icon, size );

	if ( f ) g_signal_connect ( button, "clicked", G_CALLBACK (f), base );

	gtk_box_pack_start ( box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );
}

static void base_create_image_flip_button ( GtkBox *box, const char *icon, uint size, void (*f)(), Base *base )
{
	GtkButton *button = (GtkButton *)gtk_button_new ();

	GdkPixbuf *pixbuf = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					  icon, size, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	GdkPixbuf *pixbuf_flip = gdk_pixbuf_flip ( pixbuf, TRUE );

	GtkImage *image   = (GtkImage *)gtk_image_new_from_pixbuf ( pixbuf_flip );
	gtk_image_set_pixel_size ( image, size );

	if ( pixbuf ) g_object_unref ( pixbuf );
	if ( pixbuf ) g_object_unref ( pixbuf_flip );

	gtk_button_set_image ( button, GTK_WIDGET ( image ) );

	if ( f ) g_signal_connect ( button, "clicked", G_CALLBACK (f), base );

	gtk_box_pack_start ( box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );
}



static Base * base_init ()
{
	Base *base = g_new ( Base, 1 );

	base->pixbuf_mp = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					    "helia-mp", 128, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	base->pixbuf_tv = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					    "helia-tv", 128, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	base->app_quit = FALSE;

	base->size_icon = 28;
	base->num_lang = lang_get_def ();

	base->opacity_win   = 1.0;
	base->opacity_eq    = 0.85;
	base->opacity_panel = 0.85;

	base->rec_dir    = g_strdup    ( g_get_home_dir () );
	base->helia_conf = g_strconcat ( g_get_user_config_dir (), "/helia/gtv.conf", NULL );
	base->ch_conf    = g_strconcat ( g_get_user_config_dir (), "/helia/gtv-channel.conf", NULL );

	char *dir_conf = g_strdup_printf ( "%s/helia", g_get_user_config_dir () );

	if ( g_file_test ( dir_conf, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR ) )
	{
		if ( g_file_test ( base->helia_conf, G_FILE_TEST_EXISTS ) )
			pref_read_config ( base );
	}
	else
	{
		g_mkdir ( dir_conf, 0777 );
		g_print ( "Creating %s directory. \n", dir_conf );
	}

	g_free ( dir_conf );

	return base;
}

static void base_create_player ( Base *base, GtkBox *box )
{
	base->player = g_new ( Player, 1 );

	base->player->window_hid = 0;
	base->player->volume = 0.5;

	base->player->panel_quit = TRUE;
	base->player->next_repeat = FALSE;
	base->player->state_subtitle = TRUE;
	base->player->record = FALSE;

	base->player->file_play = NULL;

	base->player->pipeline_rec = NULL;

	base->player->playbin = player_gst_create ( base );

	base->player->treeview = create_treeview ( base, lang_set ( base, "Files" ), TRUE );

	base->player->video = player_win_create ( base );

	GtkPaned *paned = player_win_paned_create ( base, 220 );

	gtk_box_pack_start ( box, GTK_WIDGET ( paned ), TRUE, TRUE, 0 );

	player_slider_base_create ( base, box );
}

static void base_create_dtv ( Base *base, GtkBox *box )
{
	base->dtv = g_new ( DigitalTV, 1 );

	base->dtv->window_hid = 0;
	base->dtv->volume  = 0.5;

	base->dtv->panel_quit = TRUE;
	base->dtv->scrambling = FALSE;
	base->dtv->rec_tv  = FALSE;
	base->dtv->rec_ses = FALSE;
	base->dtv->rec_status = TRUE;
	base->dtv->rec_pulse  = FALSE;

	base->dtv->ch_data = NULL;

	scan_gst_create ( base );

	base->dtv->dvbplay = dtv_gst_create ( base );

	base->dtv->treeview = create_treeview ( base, lang_set ( base, "Channels" ), FALSE );

	base->dtv->video = dtv_win_create ( base );

	GtkPaned *paned = dtv_win_paned_create ( base, 220 );

	gtk_box_pack_start ( box, GTK_WIDGET ( paned ), TRUE, TRUE, 0 );
}



void base_set_win_base ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	gtk_widget_show ( GTK_WIDGET ( bs_vbox ) );
	gtk_widget_hide ( GTK_WIDGET ( mp_vbox ) );
	gtk_widget_hide ( GTK_WIDGET ( tv_vbox ) );

	gtk_window_set_title ( base->window, "Media Player & Digital TV" );
}

static void base_set_win_mp ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	gtk_widget_hide ( GTK_WIDGET ( bs_vbox ) );
	gtk_widget_hide ( GTK_WIDGET ( tv_vbox ) );
	gtk_widget_show ( GTK_WIDGET ( mp_vbox ) );

	gtk_window_set_title ( base->window, "Media Player");
	gtk_window_set_icon  ( base->window, base->pixbuf_mp );
}

static void base_set_win_tv ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	gtk_widget_hide ( GTK_WIDGET ( bs_vbox ) );
	gtk_widget_hide ( GTK_WIDGET ( mp_vbox ) );
	gtk_widget_show ( GTK_WIDGET ( tv_vbox ) );

	gtk_window_set_title ( base->window, "Digital TV" );
	gtk_window_set_icon  ( base->window, base->pixbuf_tv );
}

static void base_info ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	about_win ( base->window );
}

static void base_pref ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	pref_win ( base );
}

static void base_keyb ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	keyb_win ( base );
}

static void base_close ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	gtk_widget_destroy ( GTK_WIDGET ( base->window ) ); 
}

static void base_action_play ( G_GNUC_UNUSED GSimpleAction *sl, G_GNUC_UNUSED GVariant *pm, Base *base )
{
	if ( gtk_widget_get_visible ( GTK_WIDGET ( mp_vbox ) ) ) player_play_paused ( base );
}

static void base_action_step ( G_GNUC_UNUSED GSimpleAction *sl, G_GNUC_UNUSED GVariant *pm, Base *base )
{
	if ( gtk_widget_get_visible ( GTK_WIDGET ( mp_vbox ) ) ) player_step_frame ( base );
}

static void base_action_stop ( G_GNUC_UNUSED GSimpleAction *sl, G_GNUC_UNUSED GVariant *pm, Base *base )
{
	if ( gtk_widget_get_visible ( GTK_WIDGET ( mp_vbox ) ) ) { player_stop ( base ); }

	if ( gtk_widget_get_visible ( GTK_WIDGET ( tv_vbox ) ) ) { dtv_stop ( base ); }
}

static void base_action_mute ( G_GNUC_UNUSED GSimpleAction *sl, G_GNUC_UNUSED GVariant *pm, Base *base )
{
	if ( gtk_widget_get_visible ( GTK_WIDGET ( mp_vbox ) ) ) { player_mute_set ( base ); }

	if ( gtk_widget_get_visible ( GTK_WIDGET ( tv_vbox ) ) ) { dtv_mute_set ( base->dtv->dvbplay ); }
}

static void base_action_list ( G_GNUC_UNUSED GSimpleAction *sl, G_GNUC_UNUSED GVariant *pm, Base *base )
{
	if ( gtk_widget_get_visible ( GTK_WIDGET ( mp_vbox ) ) )
	{
		if ( gtk_widget_get_visible ( GTK_WIDGET ( base->player->vbox_sw_mp ) ) )
		{
			gtk_widget_hide ( GTK_WIDGET ( base->player->vbox_sw_mp ) );

			gtk_widget_hide ( GTK_WIDGET ( base->player->h_box_slider_base ) );
		}
		else
		{
			gtk_widget_show ( GTK_WIDGET ( base->player->vbox_sw_mp ) );

			gtk_widget_show ( GTK_WIDGET ( base->player->h_box_slider_base ) );
		}
	}

	if ( gtk_widget_get_visible ( GTK_WIDGET ( tv_vbox ) ) )
	{
		if ( gtk_widget_get_visible ( GTK_WIDGET ( base->dtv->vbox_sw_tv ) ) ) 
			gtk_widget_hide ( GTK_WIDGET ( base->dtv->vbox_sw_tv ) );
		else
			gtk_widget_show ( GTK_WIDGET ( base->dtv->vbox_sw_tv ) );
	}
}

static void base_action_exit ( G_GNUC_UNUSED GSimpleAction *sl, G_GNUC_UNUSED GVariant *pm, Base *base )
{
	gtk_widget_destroy ( GTK_WIDGET ( base->window ) );
}

static void base_action_dir ( G_GNUC_UNUSED GSimpleAction *sl, G_GNUC_UNUSED GVariant *pm, Base *base )
{
	if ( gtk_widget_get_visible ( GTK_WIDGET ( mp_vbox ) ) ) dialog_open_dir ( base );
}

static void base_action_files ( G_GNUC_UNUSED GSimpleAction *sl, G_GNUC_UNUSED GVariant *pm, Base *base )
{
	if ( gtk_widget_get_visible ( GTK_WIDGET ( mp_vbox ) ) ) dialog_open_files ( base );
}

static FuncAction func_action_n[] =
{	
	{ base_action_dir,   "add_dir",   GDK_CONTROL_MASK, GDK_KEY_D },
	{ base_action_files, "add_files", GDK_CONTROL_MASK, GDK_KEY_O },
	{ base_action_list,  "playlist",  GDK_CONTROL_MASK, GDK_KEY_H },

	{ base_action_play,  "play_paused", 0, GDK_KEY_space  },
	{ base_action_step,  "play_step",   0, GDK_KEY_period },	
	{ base_action_stop,  "stop", GDK_CONTROL_MASK, GDK_KEY_X },
	{ base_action_mute,  "mute", GDK_CONTROL_MASK, GDK_KEY_M },
	{ base_action_exit,  "exit", GDK_CONTROL_MASK, GDK_KEY_Q }
};

static void base_app_add_accelerator ( GtkApplication *app, uint i )
{
	char *accel_name = gtk_accelerator_name ( func_action_n[i].gdk_key, func_action_n[i].mod_key );

	const char *accel_str[] = { accel_name, NULL };

	char *text = g_strconcat ( "app.", func_action_n[i].func_name, NULL );

		gtk_application_set_accels_for_action ( app, text, accel_str );

	g_free ( text );

	g_free ( accel_name );
}

static void base_create_gaction_entry ( GtkApplication *app, Base *base )
{	
    GActionEntry entries[ G_N_ELEMENTS ( func_action_n ) ];

    uint i = 0;

    for ( i = 0; i < G_N_ELEMENTS ( func_action_n ); i++ )
    {
        entries[i].name           = func_action_n[i].func_name;
        entries[i].activate       = func_action_n[i].f;
        entries[i].parameter_type = NULL;
        entries[i].state          = NULL;

        base_app_add_accelerator ( app, i );
    }

    g_action_map_add_action_entries ( G_ACTION_MAP ( app ), entries, G_N_ELEMENTS ( entries ), base );
}

static void base_set ()
{
	gtk_icon_theme_add_resource_path ( gtk_icon_theme_get_default (), "/helia/icons" );
	gtk_icon_theme_add_resource_path ( gtk_icon_theme_get_default (), "/helia/data/icons" );

	g_object_set ( gtk_settings_get_default (), "gtk-theme-name", "Adwaita-dark", NULL );
}

static void base_app_quit ( G_GNUC_UNUSED GtkWindow *win, Base *base )
{
	g_print ( "base_app_quit \n" );

	base->app_quit = TRUE;

	player_stop ( base );
	dtv_stop ( base );

	pref_save_config ( base );
	treeview_auto_save_tv ( base->dtv->treeview, base->ch_conf );

	if ( base->pixbuf_tv ) g_object_unref ( base->pixbuf_tv );
	if ( base->pixbuf_mp ) g_object_unref ( base->pixbuf_mp );

	gst_object_unref ( base->player->playbin );
	gst_object_unref ( base->dtv->dvbplay );
	gst_object_unref ( base->dtv->scan.pipeline_scan );

	g_free ( base->rec_dir );
	g_free ( base->ch_conf );
	g_free ( base->helia_conf );

	if ( base->player->file_play ) g_free ( base->player->file_play );
	if ( base->dtv->ch_data      ) g_free ( base->dtv->ch_data );

	g_free ( base );
}

static void base_win ( GtkApplication *app, GFile **files, int n_files )
{
	base_set ();

	Base *base = base_init ();

	base_create_gaction_entry ( app, base );

	base->window = (GtkWindow *)gtk_application_window_new (app);
	gtk_window_set_title ( base->window, "Media Player & Digital TV");
	gtk_window_set_default_size ( base->window, 900, 400 );
	g_signal_connect ( base->window, "destroy", G_CALLBACK ( base_app_quit ), base );

	gtk_window_set_icon_name ( base->window, "display" );

	mn_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	bs_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	tv_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	mp_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

	gtk_container_set_border_width ( GTK_CONTAINER ( bs_vbox ), 25 );

	gtk_box_set_spacing ( mn_vbox, 10 );
	gtk_box_set_spacing ( bs_vbox, 10 );

	GtkBox *bt_hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( bt_hbox,  10 );

	base_create_image_button      ( bt_hbox, "helia-mp", 256, base_set_win_mp, base );
	base_create_image_flip_button ( bt_hbox, "helia-tv", 256, base_set_win_tv, base );

	gtk_box_pack_start ( bs_vbox, GTK_WIDGET ( bt_hbox ), TRUE,  TRUE,  0 );

	GtkBox *bc_hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( bc_hbox,  10 );

	base_create_image_button ( bc_hbox, "helia-pref", 48, base_pref,  base );
	base_create_image_button ( bc_hbox, "helia-keyb", 48, base_keyb,  base );
	base_create_image_button ( bc_hbox, "helia-info", 48, base_info,  base );
	base_create_image_button ( bc_hbox, "helia-quit", 48, base_close, base );

	gtk_box_pack_start ( bs_vbox, GTK_WIDGET ( bc_hbox ), FALSE,  FALSE, 0 );

	gtk_box_pack_start ( mn_vbox, GTK_WIDGET ( bs_vbox ), TRUE, TRUE, 0 );

	base_create_player ( base, mp_vbox );
	base_create_dtv    ( base, tv_vbox );

	gtk_box_pack_start ( mn_vbox, GTK_WIDGET ( mp_vbox ), TRUE, TRUE, 0 );
	gtk_box_pack_start ( mn_vbox, GTK_WIDGET ( tv_vbox ), TRUE, TRUE, 0 );

	gtk_container_add   ( GTK_CONTAINER ( base->window ), GTK_WIDGET ( mn_vbox ) );
	gtk_widget_show_all ( GTK_WIDGET ( base->window ) );

	gtk_widget_hide ( GTK_WIDGET ( mp_vbox ) );
	gtk_widget_hide ( GTK_WIDGET ( tv_vbox ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( base->window ), base->opacity_win );
	gtk_window_resize ( base->window, 900, 400 );

	if ( n_files > 0 ) { base_set_win_mp ( NULL, base ); add_arg ( files, n_files, base ); }

	if ( g_file_test ( base->ch_conf, G_FILE_TEST_EXISTS ) )
		treeview_add_dtv ( base, base->ch_conf );
}

static void open ( GtkApplication *app, GFile **files, int n_files )
{
    base_win ( app, files, n_files );
}

static void activate ( GtkApplication *app )
{
    base_win ( app, NULL, 0 );
}

int main ( int argc, char **argv )
{
	gst_init ( NULL, NULL );

	GtkApplication *app = gtk_application_new ( NULL, G_APPLICATION_HANDLES_OPEN );

	g_signal_connect ( app, "activate", G_CALLBACK ( activate ),  NULL );
    g_signal_connect ( app, "open",     G_CALLBACK ( open     ),  NULL );

	int status = g_application_run ( G_APPLICATION ( app ),  argc, argv );
	g_object_unref (app);

	return status;
}
