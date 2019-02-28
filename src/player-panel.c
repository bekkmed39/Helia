/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include <base.h>

#include "player-gst.h"
#include "tree-view.h"
#include "player-slider.h"
#include "info.h"
#include "helia-eqa.h"
#include "helia-eqv.h"


static void player_panel_base ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	player_stop ( base );

	base_set_win_base ( button, base );
}

static void player_panel_editor ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	if ( gtk_widget_get_visible ( GTK_WIDGET ( base->player->vbox_sw_mp ) ) )
	{
		gtk_widget_hide ( GTK_WIDGET ( base->player->vbox_sw_mp ) );

		gtk_widget_hide ( GTK_WIDGET ( base->player->h_box_slider_base ) );

		gtk_widget_show ( GTK_WIDGET ( base->player->h_box_slider_panel ) );
	}
	else
	{
		gtk_widget_show ( GTK_WIDGET ( base->player->vbox_sw_mp ) );

		gtk_widget_show ( GTK_WIDGET ( base->player->h_box_slider_base ) );

		gtk_widget_hide ( GTK_WIDGET ( base->player->h_box_slider_panel ) );

		GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( button ) ) );

		gtk_window_resize ( window, 450, base->size_icon * 2 );
	}
}

static void player_panel_eqa ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	GstElement *element = player_gst_ret_iterate_element ( base->player->playbin, "equalizer", NULL );

	if ( base->player->record )
	{
		element = player_gst_ret_iterate_element ( base->player->pipeline_rec, "equalizer", NULL );
	}

	if ( element == NULL ) return;

	if ( GST_ELEMENT_CAST ( element )->current_state != GST_STATE_PLAYING ) return;

	helia_eqa_win ( element, base->window, base->opacity_eq, base );
}

static void player_panel_eqv ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	GstElement *element = player_gst_ret_iterate_element ( base->player->playbin, "videobalance", NULL );

	if ( base->player->record )
	{
		element = player_gst_ret_iterate_element ( base->player->pipeline_rec, "videobalance", NULL );
	}

	if ( element == NULL ) return;

	if ( GST_ELEMENT_CAST ( element )->current_state != GST_STATE_PLAYING ) return;

	helia_eqv_win ( element, base->window, base->opacity_eq, base );
}

static void player_panel_muted ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	player_mute_set ( base );

	gtk_widget_set_sensitive ( GTK_WIDGET ( base->player->volbutton ), !player_mute_get ( base ) );
}

static gboolean player_panel_update_volbutton ( Base *base )
{
	if ( base->app_quit ) return FALSE;

	if ( base->player->panel_quit ) return FALSE;

	gtk_widget_set_sensitive ( GTK_WIDGET ( base->player->volbutton ), !player_mute_get ( base ) );

	return FALSE;
}

static void player_panel_play ( GtkButton *button, Base *base )
{
	if ( !base->player->file_play ) return;

	if ( base->player->record ) return;

	const char *name = NULL;

	if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_PLAYING )
	{
		gst_element_set_state ( base->player->playbin, GST_STATE_PAUSED  );

		name = "helia-play";
	}
	else
	{
		gst_element_set_state ( base->player->playbin, GST_STATE_PLAYING );

		name = "helia-pause";
	}

	GdkPixbuf *pixbuf = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (),
					  name, base->size_icon, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	GtkImage *image   = (GtkImage  *)gtk_image_new_from_pixbuf ( pixbuf );
	gtk_button_set_image ( button, GTK_WIDGET ( image ) );

	if ( pixbuf ) g_object_unref ( pixbuf );

	if ( !base->player->panel_quit )
		g_timeout_add ( 1000, (GSourceFunc)player_panel_update_volbutton, base );
}

static void player_panel_stop_set_icon ( G_GNUC_UNUSED GtkButton *button, GtkButton *button_play )
{
	int size_image = gtk_image_get_pixel_size ( GTK_IMAGE ( gtk_button_get_image ( button ) ) );

	GdkPixbuf *pixbuf = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					  "helia-play", size_image, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	GtkImage *image   = (GtkImage  *)gtk_image_new_from_pixbuf ( pixbuf );
	gtk_button_set_image ( button_play, GTK_WIDGET ( image ) );

	if ( pixbuf ) g_object_unref ( pixbuf );
}

static void player_panel_stop ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	player_stop ( base );

	gtk_widget_set_sensitive ( GTK_WIDGET ( base->player->volbutton ), FALSE );
}

static void player_panel_record ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	player_record ( base );
}

static void player_panel_info ( G_GNUC_UNUSED GtkButton *button, G_GNUC_UNUSED Base *base )
{
	if ( base->player->record ) return;

	if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state != GST_STATE_PLAYING ) return;

	info_win_create ( base, FALSE );
}

static void player_panel_exit ( G_GNUC_UNUSED GtkButton *button, G_GNUC_UNUSED Base *base )
{
	/* swapped */
}

static void player_panel_quit ( G_GNUC_UNUSED GtkWindow *win, Base *base )
{
	base->player->panel_quit = TRUE;
}

void player_panel_win_create ( GtkWindow *base_win, Base *base )
{
	const char *name = ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_NULL ) 
						 ? "helia-play" : ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_PAUSED ) ? "helia-play" : "helia-pause";

	struct NameIcon { const char *name; gboolean swap_close; void (* activate)(); } NameIcon_n[] =
	{
		{ "helia-mp", 			TRUE,  player_panel_base 	},
		{ "helia-editor", 		FALSE, player_panel_editor 	},
		{ "helia-eqa", 			TRUE,  player_panel_eqa 	},
		{ "helia-eqv", 			TRUE,  player_panel_eqv		},
		{ "helia-muted", 		FALSE, player_panel_muted 	},

		{ name, 				FALSE, player_panel_play 	},
		{ "helia-stop",   		FALSE, player_panel_stop 	},
		{ "helia-record", 		FALSE, player_panel_record 	},
		{ "helia-info", 		TRUE,  player_panel_info 	},
		{ "helia-exit", 		TRUE,  player_panel_exit 	}
	};

	GtkWindow *window = (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_transient_for ( window, base_win );
	gtk_window_set_modal     ( window, TRUE   );
	gtk_window_set_decorated ( window, FALSE  );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_default_size ( window, 450, base->size_icon * 2 );
	g_signal_connect ( window, "destroy", G_CALLBACK ( player_panel_quit ), base );

	base->player->panel_quit = FALSE;

	GtkBox *m_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );

	GtkBox *b_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	GtkBox *l_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *r_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *a_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );

	GtkBox *h_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	GtkBox *hm_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gtk_box_set_spacing ( b_box,  5 );
	gtk_box_set_spacing ( l_box,  5 );
	gtk_box_set_spacing ( h_box,  5 );
	gtk_box_set_spacing ( hm_box, 5 );
	gtk_box_set_spacing ( a_box,  5 );

	base->player->volbutton = (GtkVolumeButton *)gtk_volume_button_new ();
	gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( base->player->volbutton ), base->player->volume );
	gtk_widget_set_sensitive ( GTK_WIDGET ( base->player->volbutton ), !player_mute_get ( base ) );
	g_signal_connect ( base->player->volbutton, "value-changed", G_CALLBACK ( player_volume_changed ), base );

	GtkButton *button_play = NULL, *button_stop = NULL;

	uint i = 0;
	for ( i = 0; i < G_N_ELEMENTS ( NameIcon_n ); i++ )
	{
		GtkButton *button = base_set_image_button ( NameIcon_n[i].name, base->size_icon );
		g_signal_connect ( button, "clicked", G_CALLBACK ( NameIcon_n[i].activate ), base );

		if ( NameIcon_n[i].swap_close ) g_signal_connect_swapped ( button, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );

		if ( i == 4 ) gtk_box_pack_start ( l_box, GTK_WIDGET ( h_box ), TRUE, TRUE, 0 );

		gtk_box_pack_start ( ( i < 5 ) ? h_box : hm_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );

		if ( i == 5 ) button_play = button;
		if ( i == 6 ) button_stop = button;
	}

	g_signal_connect ( button_stop, "clicked", G_CALLBACK ( player_panel_stop_set_icon ), button_play );

	gtk_box_pack_start ( l_box, GTK_WIDGET ( hm_box ), TRUE, TRUE, 0 );

	gtk_box_pack_start ( r_box, GTK_WIDGET ( base->player->volbutton ), TRUE, TRUE, 0 );

	gtk_box_pack_start ( b_box, GTK_WIDGET ( l_box ), TRUE,  TRUE,  0 );
	gtk_box_pack_start ( b_box, GTK_WIDGET ( a_box ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( b_box, GTK_WIDGET ( r_box ), FALSE, FALSE, 0 );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( b_box ), TRUE,  TRUE,  0 );

	player_slider_panel_create ( base, m_box );

	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 10 );
	gtk_container_add   ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );
	gtk_widget_show_all ( GTK_WIDGET ( window ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( window ), base->opacity_panel );

	if ( gtk_widget_get_visible ( GTK_WIDGET ( base->player->h_box_slider_base ) ) )
	{
		gtk_widget_hide ( GTK_WIDGET ( base->player->h_box_slider_panel ) );

		gtk_window_resize ( window, 450, base->size_icon * 2 );
	}
}
