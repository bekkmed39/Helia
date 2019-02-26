/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include <base.h>

#include "tree-view.h"


static void player_slider_label_set_text ( GtkLabel *label, gint64 pos_dur, uint digits )
{
	char *str   = g_strdup_printf ( "%" GST_TIME_FORMAT, GST_TIME_ARGS ( pos_dur ) );
	char *str_l = g_strndup ( str, strlen ( str ) - digits );

		gtk_label_set_text ( label, str_l );

	free ( str_l );
	free ( str   );
}

static void player_slider_set_data_slider ( Slider slider, gint64 pos, uint digits_pos, gint64 dur, uint digits_dur, GtkBox *box_slider, gboolean sensitive )
{
	player_slider_label_set_text ( slider.lab_pos, pos, digits_pos );

	if ( dur > -1 ) player_slider_label_set_text ( slider.lab_dur, dur, digits_dur );

	gtk_widget_set_sensitive ( GTK_WIDGET ( box_slider ), sensitive );
}

void player_slider_set_data ( Base *base, gint64 pos, uint digits_pos, gint64 dur, uint digits_dur, gboolean sensitive )
{
	if ( base->app_quit ) return;

	player_slider_set_data_slider ( base->player->slider_base, pos, digits_pos, dur, digits_dur, 
									base->player->h_box_slider_base, sensitive );

	if ( !base->player->panel_quit && gtk_widget_get_visible ( GTK_WIDGET ( base->player->h_box_slider_panel ) ) )
		player_slider_set_data_slider ( base->player->slider_panel, pos, digits_pos, dur, digits_dur, 
										base->player->h_box_slider_panel, sensitive );
}

void player_slider_update_slider ( Slider slider, gdouble range, gdouble value )
{
	g_signal_handler_block   ( slider.slider, slider.slider_signal_id );

		gtk_range_set_range  ( GTK_RANGE ( slider.slider ), 0, range );
		gtk_range_set_value  ( GTK_RANGE ( slider.slider ),    value );

	g_signal_handler_unblock ( slider.slider, slider.slider_signal_id );
}

static void player_slider_clear_all ( Slider slider, GtkBox *box_slider )
{
	player_slider_update_slider ( slider, 120*60, 0 );

	gtk_label_set_text ( slider.lab_pos, "0:00:00" );
	gtk_label_set_text ( slider.lab_dur, "0:00:00" );

	gtk_widget_set_sensitive ( GTK_WIDGET ( box_slider ), FALSE );
}

void player_slider_clear_data ( Base *base )
{
	if ( base->app_quit ) return;

	player_slider_clear_all ( base->player->slider_base, base->player->h_box_slider_base );

	if ( !base->player->panel_quit && gtk_widget_get_visible ( GTK_WIDGET ( base->player->h_box_slider_panel ) ) )
		player_slider_clear_all ( base->player->slider_panel, base->player->h_box_slider_panel );
}

static void player_slider_panel_changed ( GtkRange *range, Base *base )
{
	if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_NULL ) return;

	gdouble value = gtk_range_get_value ( GTK_RANGE (range) );

	gst_element_seek_simple ( base->player->playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, (gint64)( value * GST_SECOND ) );

	player_slider_label_set_text ( base->player->slider_base.lab_pos, (gint64)( value * GST_SECOND ),  8  );

	if ( !base->player->panel_quit && gtk_widget_get_visible ( GTK_WIDGET ( base->player->h_box_slider_panel ) ) )
		player_slider_label_set_text ( base->player->slider_panel.lab_pos, (gint64)( value * GST_SECOND ),  8  );
}

static void player_slider_update_data ( Base *base, Slider slider )
{
	gboolean dur_b = FALSE;
	gint64 duration = 0, current = 0;

	if ( gst_element_query_position ( base->player->playbin, GST_FORMAT_TIME, &current ) )
	{
		if ( gst_element_query_duration ( base->player->playbin, GST_FORMAT_TIME, &duration ) ) dur_b = TRUE;

		if ( dur_b && duration > 0 )
		{
			player_slider_update_slider ( slider, (gdouble)duration / GST_SECOND, (gdouble)current / GST_SECOND );

			player_slider_label_set_text ( slider.lab_pos, current,  8  );
			player_slider_label_set_text ( slider.lab_dur, duration, 10 );
		}
		else
		{
			player_slider_label_set_text ( slider.lab_pos, current,  8  );
		}
	}
}

static gboolean player_slider_refresh ( Base *base )
{
	if ( base->app_quit ) return FALSE;

	if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state != GST_STATE_PLAYING ) return TRUE;

	gboolean dur_b = FALSE;
	gint64 duration = 0, current = 0;

	if ( gst_element_query_position ( base->player->playbin, GST_FORMAT_TIME, &current ) )
	{
		if ( gst_element_query_duration ( base->player->playbin, GST_FORMAT_TIME, &duration ) ) dur_b = TRUE;

			if ( dur_b && duration > 0 )
			{
				if ( current / GST_SECOND < duration / GST_SECOND )
				{
					player_slider_update_slider ( base->player->slider_base, (gdouble)duration / GST_SECOND, (gdouble)current / GST_SECOND );

					if ( !base->player->panel_quit && gtk_widget_get_visible ( GTK_WIDGET ( base->player->h_box_slider_panel ) ) )
						player_slider_update_slider ( base->player->slider_panel, (gdouble)duration / GST_SECOND, (gdouble)current / GST_SECOND );

					player_slider_set_data ( base, current, 8, duration, 10, TRUE );
				}
				else
					player_next ( base );
			}
			else
			{
				player_slider_set_data ( base, current, 8, -1, 10, FALSE );
			}
	}

	return TRUE;
}

static Slider player_create_slider_label ( Base *base, void (*f)() )
{
	Slider slider;

	slider.lab_pos = (GtkLabel *)gtk_label_new ( "0:00:00" );
	slider.lab_dur = (GtkLabel *)gtk_label_new ( "0:00:00" );
	slider.rec_buf  = (GtkLabel *)gtk_label_new ( "" );

	slider.slider  = (GtkScale *)gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, 0, 120*60, 1 );
	slider.slider_signal_id = g_signal_connect ( slider.slider, "value-changed", G_CALLBACK ( f ), base );

	gtk_scale_set_draw_value ( slider.slider, 0 );
	gtk_range_set_value ( GTK_RANGE ( slider.slider ), 0 );

	return slider;
}

static GtkBox * player_create_slider_box ( Slider slider )
{
	GtkBox *hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gtk_widget_set_margin_start ( GTK_WIDGET ( hbox ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( hbox ), 10 );
	gtk_box_set_spacing ( hbox, 5 );

	gtk_box_pack_start ( hbox, GTK_WIDGET ( slider.lab_pos ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( hbox, GTK_WIDGET ( slider.rec_buf ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( hbox, GTK_WIDGET ( slider.slider  ), TRUE,  TRUE,  0 );
	gtk_box_pack_start ( hbox, GTK_WIDGET ( slider.lab_dur ), FALSE, FALSE, 0 );

	return hbox;
}

void player_slider_panel_create ( Base *base, GtkBox *m_box )
{
	base->player->slider_panel = player_create_slider_label ( base, player_slider_panel_changed );

	base->player->h_box_slider_panel = player_create_slider_box ( base->player->slider_panel );

	if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_PAUSED )
		player_slider_update_data ( base, base->player->slider_panel );

	if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_NULL )
		gtk_widget_set_sensitive ( GTK_WIDGET ( base->player->h_box_slider_panel ), FALSE );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( base->player->h_box_slider_panel ), FALSE, FALSE, 0 );
}

void player_slider_base_create ( Base *base, GtkBox *m_box )
{
	base->player->slider_base = player_create_slider_label ( base, player_slider_panel_changed );

	base->player->h_box_slider_base = player_create_slider_box ( base->player->slider_base );

	gtk_widget_set_sensitive ( GTK_WIDGET ( base->player->h_box_slider_base ), FALSE );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( base->player->h_box_slider_base ), FALSE, FALSE, 0 );

	g_timeout_add ( 100, (GSourceFunc)player_slider_refresh, base );
}
