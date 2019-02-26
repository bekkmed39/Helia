/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include <base.h>

#include "dtv-gst.h"
#include "tree-view.h"
#include "dtv-level.h"
#include "info.h"
#include "helia-eqa.h"
#include "helia-eqv.h"

#include "scan.h"


static void dtv_panel_base ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	dtv_stop ( base );

	base_set_win_base ( button, base );
}

static void dtv_panel_editor ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	if ( gtk_widget_get_visible ( GTK_WIDGET ( base->dtv->vbox_sw_tv ) ) )
	{
		gtk_widget_hide ( GTK_WIDGET ( base->dtv->vbox_sw_tv ) );

		gtk_widget_show ( GTK_WIDGET ( base->dtv->h_box_level_panel ) );
	}
	else
	{
		gtk_widget_show ( GTK_WIDGET ( base->dtv->vbox_sw_tv ) );

		gtk_widget_hide ( GTK_WIDGET ( base->dtv->h_box_level_panel ) );

		GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( button ) ) );

		gtk_window_resize ( window, 450, base->size_icon * 2 );
	}
}

static void dtv_panel_eqa ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	if ( GST_ELEMENT_CAST ( base->dtv->dvbplay )->current_state != GST_STATE_PLAYING ) return;

	GstElement *element = dtv_gst_ret_iterate_element ( base->dtv->dvbplay, "equalizer", NULL );

	if ( element == NULL ) return;

	helia_eqa_win ( element, base->window, base->opacity_eq );
}

static void dtv_panel_eqv ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	if ( GST_ELEMENT_CAST ( base->dtv->dvbplay )->current_state != GST_STATE_PLAYING ) return;

	GstElement *element = dtv_gst_ret_iterate_element ( base->dtv->dvbplay, "videobalance", NULL );

	if ( element == NULL ) return;

	helia_eqv_win ( element, base->window, base->opacity_eq );
}

static void dtv_panel_muted ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	dtv_mute_set ( base->dtv->dvbplay );

	gtk_widget_set_sensitive ( GTK_WIDGET ( base->dtv->volbutton ), !dtv_mute_get ( base->dtv->dvbplay ) );
}

static void dtv_panel_rec ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	dtv_gst_record ( base );
}

static void dtv_panel_stop ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	dtv_stop ( base );

	gtk_widget_set_sensitive ( GTK_WIDGET ( base->dtv->volbutton ), FALSE );
}

static void dtv_panel_scan ( G_GNUC_UNUSED GtkButton *button, G_GNUC_UNUSED Base *base )
{
	scan_win_create ( base );
}

static void dtv_panel_info ( G_GNUC_UNUSED GtkButton *button, G_GNUC_UNUSED Base *base )
{
	if ( GST_ELEMENT_CAST ( base->dtv->dvbplay )->current_state != GST_STATE_PLAYING ) return;

	info_win_create ( base, TRUE );
}

static void dtv_panel_exit ( G_GNUC_UNUSED GtkButton *button, G_GNUC_UNUSED Base *base )
{
	/* swapped */
}

static void dtv_panel_quit ( G_GNUC_UNUSED GtkWindow *win, Base *base )
{
	base->dtv->panel_quit = TRUE;
}

void dtv_panel_win_create ( GtkWindow *base_win, Base *base )
{
	struct NameIcon { const char *name; gboolean swap_close; void (* activate)(); } NameIcon_n[] =
	{
		{ "helia-tv", 			TRUE,  dtv_panel_base 	},
		{ "helia-editor", 		FALSE, dtv_panel_editor },
		{ "helia-eqa", 			TRUE,  dtv_panel_eqa 	},
		{ "helia-eqv",	 		TRUE,  dtv_panel_eqv	},
		{ "helia-muted", 		FALSE, dtv_panel_muted 	},

		{ "helia-stop",   		FALSE, dtv_panel_stop 	},
		{ "helia-record", 		FALSE, dtv_panel_rec  	},
		{ "helia-display", 		TRUE,  dtv_panel_scan 	},
		{ "helia-info", 		TRUE,  dtv_panel_info 	},
		{ "helia-exit", 		TRUE,  dtv_panel_exit 	}
	};

	GtkWindow *window = (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_transient_for ( window, base_win );
	gtk_window_set_modal     ( window, TRUE   );
	gtk_window_set_decorated ( window, FALSE  );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_default_size ( window, 450, base->size_icon * 2 );
	g_signal_connect ( window, "destroy", G_CALLBACK ( dtv_panel_quit ), base );

	base->dtv->panel_quit = FALSE;

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

	base->dtv->volbutton = (GtkVolumeButton *)gtk_volume_button_new ();
	gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( base->dtv->volbutton ), base->dtv->volume );
	gtk_widget_set_sensitive ( GTK_WIDGET ( base->dtv->volbutton ), !dtv_mute_get ( base->dtv->dvbplay ) );
	g_signal_connect ( base->dtv->volbutton, "value-changed", G_CALLBACK ( dtv_volume_changed ), base );

	uint i = 0;
	for ( i = 0; i < G_N_ELEMENTS ( NameIcon_n ); i++ )
	{
		GtkButton *button = base_set_image_button ( NameIcon_n[i].name, base->size_icon );
		g_signal_connect ( button, "clicked", G_CALLBACK ( NameIcon_n[i].activate ), base );

		if ( NameIcon_n[i].swap_close ) g_signal_connect_swapped ( button, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );

		if ( i == 4 ) gtk_box_pack_start ( l_box, GTK_WIDGET ( h_box ), TRUE, TRUE, 0 );

		gtk_box_pack_start ( ( i < 5 ) ? h_box : hm_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );
	}

	gtk_box_pack_start ( l_box, GTK_WIDGET ( hm_box ), TRUE, TRUE, 0 );

	gtk_box_pack_start ( r_box, GTK_WIDGET ( base->dtv->volbutton ), TRUE, TRUE, 0 );

	gtk_box_pack_start ( b_box, GTK_WIDGET ( l_box ), TRUE,  TRUE,  0 );
	gtk_box_pack_start ( b_box, GTK_WIDGET ( a_box ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( b_box, GTK_WIDGET ( r_box ), FALSE, FALSE, 0 );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( b_box ), TRUE,  TRUE,  0 );

	base->dtv->h_box_level_panel = dtv_level_panel_create ( base );
	gtk_box_pack_start ( m_box, GTK_WIDGET ( base->dtv->h_box_level_panel ), FALSE,  FALSE,  0 );

	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 10 );
	gtk_container_add   ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );
	gtk_widget_show_all ( GTK_WIDGET ( window ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( window ), base->opacity_panel );

	if ( gtk_widget_get_visible ( GTK_WIDGET ( base->dtv->vbox_sw_tv ) ) )
	{
		gtk_widget_hide ( GTK_WIDGET ( base->dtv->h_box_level_panel ) );

		gtk_window_resize ( window, 450, base->size_icon * 2 );
	}
}
