/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include <gst/video/videooverlay.h>

#include <base.h>

#include "player-slider.h"
#include "tree-view.h"


static void player_stop_record ( Base *base, gboolean play );


GstElement * player_gst_ret_iterate_element ( GstElement *it_element, const char *name1, const char *name2 )
{
	GstIterator *it = gst_bin_iterate_recurse ( GST_BIN ( it_element ) );

	GValue item = { 0, };
	gboolean done = FALSE;

	GstElement *element_ret = NULL;

	while ( !done )
	{
		switch ( gst_iterator_next ( it, &item ) )
		{
			case GST_ITERATOR_OK:
			{
				GstElement *element = GST_ELEMENT ( g_value_get_object (&item) );

				char *object_name = gst_object_get_name ( GST_OBJECT ( element ) );

				if ( g_strrstr ( object_name, name1 ) )
				{
					if ( name2 && g_strrstr ( object_name, name2 ) )
					{
						element_ret = element;
					}
					else
						element_ret = element;
				}

				g_debug ( "Object name: %s ", object_name );

				g_free ( object_name );
				g_value_reset (&item);

				break;
			}

			case GST_ITERATOR_RESYNC:
				gst_iterator_resync (it);
				break;

			case GST_ITERATOR_ERROR:
				done = TRUE;
				break;

			case GST_ITERATOR_DONE:
				done = TRUE;
				break;
		}
	}

	g_value_unset ( &item );
	gst_iterator_free ( it );

	return element_ret;
}

gboolean player_mute_get ( Base *base )
{
	GstElement *element = base->player->playbin;

	if ( base->player->record )
	{
		element = player_gst_ret_iterate_element ( base->player->pipeline_rec, "autoaudiosink", "actual-sink" );

		if ( element == NULL ) return FALSE;
	}

	if ( GST_ELEMENT_CAST ( element )->current_state != GST_STATE_PLAYING ) return TRUE;

	gboolean mute = FALSE;

	g_object_get ( element, "mute", &mute, NULL );

	return mute;
}

void player_mute_set ( Base *base )
{
	GstElement *element = base->player->playbin;

	if ( base->player->record )
	{
		element = player_gst_ret_iterate_element ( base->player->pipeline_rec, "autoaudiosink", "actual-sink" );

		if ( element == NULL ) return;
	}

	if ( GST_ELEMENT_CAST ( element )->current_state != GST_STATE_PLAYING ) return;

	gboolean mute = FALSE;

	g_object_get ( element, "mute", &mute, NULL );
	g_object_set ( element, "mute", !mute, NULL );
}

void player_volume_changed ( G_GNUC_UNUSED GtkScaleButton *button, gdouble value, Base *base )
{
	GstElement *element = base->player->playbin;

	if ( base->player->record )
	{
		element = player_gst_ret_iterate_element ( base->player->pipeline_rec, "autoaudiosink", "actual-sink" );

		if ( element == NULL ) return;
	}

	if ( GST_ELEMENT_CAST ( element )->current_state != GST_STATE_PLAYING ) return;

	base->player->volume = value;

	g_object_set ( element, "volume", value, NULL );
}

static void player_gst_step_pos ( Base *base, gint64 am )
{
    gst_element_send_event ( base->player->playbin, gst_event_new_step ( GST_FORMAT_BUFFERS, am, 1.0, TRUE, FALSE ) );

    gint64 current = 0;

	if ( gst_element_query_position ( base->player->playbin, GST_FORMAT_TIME, &current ) )
	{
		player_slider_set_data ( base, current, 7, -1, 10, TRUE );
	}
}
void player_step_frame ( Base *base )
{
	if ( base->player->record ) return;

	if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_NULL ) return;

    gint n_video = 0;
    g_object_get ( base->player->playbin, "n-video", &n_video, NULL );

    if ( n_video == 0 ) return;

    if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_PLAYING )
		gst_element_set_state ( base->player->playbin, GST_STATE_PAUSED );

	player_gst_step_pos ( base, 1 );
}

void player_gst_new_pos ( Base *base, gint64 set_pos, gboolean up_dwn )
{
	gboolean dur_b = FALSE;
	gint64 current = 0, duration = 0, new_pos = 0, skip = (gint64)( set_pos * GST_SECOND );

	if ( gst_element_query_position ( base->player->playbin, GST_FORMAT_TIME, &current ) )
	{
		if ( gst_element_query_duration ( base->player->playbin, GST_FORMAT_TIME, &duration ) ) dur_b = TRUE;

		if ( !dur_b || duration == 0 ) return;

		if ( up_dwn ) new_pos = ( duration > ( current + skip ) ) ? ( current + skip ) : duration;

		if ( !up_dwn ) new_pos = ( current > skip ) ? ( current - skip ) : 0;

		gst_element_seek_simple ( base->player->playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, new_pos );

		if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_PAUSED )
		{
			player_slider_set_data ( base, new_pos, 8, -1, 10, TRUE );

			player_slider_update_slider ( base->player->slider_base, (gdouble)duration / GST_SECOND, (gdouble)new_pos / GST_SECOND );
		}
	}
}

void player_stop ( Base *base )
{
	if ( base->player->record )
	{
		player_stop_record ( base, FALSE );
		return;
	}

	if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state != GST_STATE_NULL )
	{
		gst_element_set_state ( base->player->playbin, GST_STATE_NULL );

		player_slider_clear_data ( base );

		gtk_widget_queue_draw ( GTK_WIDGET ( base->window ) );
	}
}

static gboolean player_update_win ( Base *base )
{
	if ( base->app_quit ) return FALSE;

	if ( base->player->record ) return FALSE;

	if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_NULL    ) return FALSE;

	if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_PAUSED  ) return FALSE;

	if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_PLAYING )
	{
		gtk_widget_queue_draw ( GTK_WIDGET ( base->window ) );

		return FALSE;
	}
	else
	{
		time ( &base->player->t_cur_mp );

		if ( ( base->player->t_cur_mp - base->player->t_start_mp ) >= 25 )
		{
			g_warning ( "Time stop %ld (sec) ", base->player->t_cur_mp - base->player->t_start_mp );

			player_stop ( base );

			return FALSE;
		}
	}

	return TRUE;
}

void player_play_paused ( Base *base )
{
	if ( base->player->record ) return;

	if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_PAUSED )
		gst_element_set_state ( base->player->playbin, GST_STATE_PLAYING );
	else
		if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_PLAYING )
			gst_element_set_state ( base->player->playbin, GST_STATE_PAUSED );
}

void player_play ( Base *base )
{
	if ( !base->player->file_play ) return;

	if ( base->player->record ) return;

	if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state != GST_STATE_PLAYING )
	{
		gst_element_set_state ( base->player->playbin, GST_STATE_PLAYING );

		g_object_set ( base->player->playbin, "volume", base->player->volume, NULL );

		g_timeout_add ( 250, (GSourceFunc)player_update_win, base );

		time ( &base->player->t_start_mp );
	}
}

void player_stop_set_play ( Base *base, const char *name_file )
{
	player_stop ( base );

	if ( base->player->file_play ) g_free ( base->player->file_play );
	base->player->file_play = g_strdup ( name_file );

    if ( g_strrstr ( name_file, "://" ) )
        g_object_set ( base->player->playbin, "uri", name_file, NULL );
    else
    {
        char *uri = gst_filename_to_uri ( name_file, NULL );

            g_object_set ( base->player->playbin, "uri", uri, NULL );

        g_free ( uri );
    }

	player_play ( base );
}

static GstBusSyncReply player_gst_bus_sync_handler ( G_GNUC_UNUSED GstBus *bus, GstMessage *message, Base *base )
{
    if ( !gst_is_video_overlay_prepare_window_handle_message ( message ) ) return GST_BUS_PASS;

    if ( base->player->window_hid != 0 )
    {
        GstVideoOverlay *xoverlay = GST_VIDEO_OVERLAY ( GST_MESSAGE_SRC ( message ) );
        gst_video_overlay_set_window_handle ( xoverlay, base->player->window_hid );

    } else { g_warning ( "Should have obtained window_handle by now!" ); }

    gst_message_unref ( message );

    return GST_BUS_DROP;
}

static void player_gst_msg_err ( G_GNUC_UNUSED GstBus *bus, GstMessage *msg, Base *base )
{
    GError *err = NULL;
    char  *dbg = NULL;

    gst_message_parse_error ( msg, &err, &dbg );

    g_printerr ( "player_gst_msg_err: %s (%s)\n", err->message, (dbg) ? dbg : "no details" );

    base_message_dialog ( "", err->message, GTK_MESSAGE_ERROR, base->window );

    g_error_free ( err );
    g_free ( dbg );

    player_stop ( base );
}

static void player_gst_msg_eos ( G_GNUC_UNUSED GstBus *bus, G_GNUC_UNUSED GstMessage *msg, Base *base )
{
	player_next ( base );
}

static void player_gst_msg_cll ( G_GNUC_UNUSED GstBus *bus, G_GNUC_UNUSED GstMessage *msg, Base *base )
{
	if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_PLAYING )
	{
		gst_element_set_state ( base->player->playbin, GST_STATE_PAUSED  );
		gst_element_set_state ( base->player->playbin, GST_STATE_PLAYING );
	}
}

static void player_gst_msg_buf ( G_GNUC_UNUSED GstBus *bus, GstMessage *msg, Base *base )
{
	if ( base->app_quit ) return;

	int percent;
	gst_message_parse_buffering ( msg, &percent );

	if ( percent == 100 )
	{
		if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_PAUSED )
			gst_element_set_state ( base->player->playbin, GST_STATE_PLAYING );

		gtk_label_set_text ( base->player->slider_base.rec_buf, "" );

		if ( !base->player->panel_quit && gtk_widget_get_visible ( GTK_WIDGET ( base->player->h_box_slider_panel ) ) )
				gtk_label_set_text ( base->player->slider_panel.rec_buf, "" );
	}
	else
	{
		if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state == GST_STATE_PLAYING )
			gst_element_set_state ( base->player->playbin, GST_STATE_PAUSED );

		char *str = g_strdup_printf ( " %d%s ", percent, "%" );

			gtk_label_set_text ( base->player->slider_base.rec_buf, str );

			if ( !base->player->panel_quit && gtk_widget_get_visible ( GTK_WIDGET ( base->player->h_box_slider_panel ) ) )
				gtk_label_set_text ( base->player->slider_panel.rec_buf, str );

		free ( str );

		// g_print ( "buffering: %d %s \n", percent, "%" );
	}
}

GstElement * player_gst_create ( Base *base )
{
    GstElement *playbin = gst_element_factory_make ( "playbin", "playbin" );

	GstElement *bin_audio, *bin_video, *asink, *vsink, *videoblnc, *equalizer;

    vsink     = gst_element_factory_make ( "autovideosink",     NULL );
    asink     = gst_element_factory_make ( "autoaudiosink",     NULL );

    videoblnc = gst_element_factory_make ( "videobalance",      NULL );
    equalizer = gst_element_factory_make ( "equalizer-nbands",  NULL );

    if ( !playbin || !vsink || !asink || !videoblnc || !equalizer )
    {
        g_printerr ( "player_gst_create - not all elements could be created.\n" );

        return NULL;
    }

    bin_audio = gst_bin_new ( "audio_sink_bin" );
	gst_bin_add_many ( GST_BIN ( bin_audio ), equalizer, asink, NULL );
	gst_element_link_many ( equalizer, asink, NULL );

	GstPad *pad = gst_element_get_static_pad ( equalizer, "sink" );
	gst_element_add_pad ( bin_audio, gst_ghost_pad_new ( "sink", pad ) );
	gst_object_unref ( pad );

	bin_video = gst_bin_new ( "video_sink_bin" );
	gst_bin_add_many ( GST_BIN ( bin_video ), videoblnc, vsink, NULL );
	gst_element_link_many ( videoblnc, vsink, NULL );

	GstPad *padv = gst_element_get_static_pad ( videoblnc, "sink" );
	gst_element_add_pad ( bin_video, gst_ghost_pad_new ( "sink", padv ) );
	gst_object_unref ( padv );

    g_object_set ( playbin, "video-sink", bin_video, NULL );
    g_object_set ( playbin, "audio-sink", bin_audio, NULL );

    g_object_set ( playbin, "volume", base->player->volume, NULL );

    GstBus *bus = gst_element_get_bus ( playbin);

    gst_bus_add_signal_watch_full ( bus, G_PRIORITY_DEFAULT );
    gst_bus_set_sync_handler ( bus, (GstBusSyncHandler)player_gst_bus_sync_handler, base, NULL );

    g_signal_connect ( bus, "message::eos",   		G_CALLBACK ( player_gst_msg_eos ), base );
    g_signal_connect ( bus, "message::error", 		G_CALLBACK ( player_gst_msg_err ), base );
    g_signal_connect ( bus, "message::clock-lost",  G_CALLBACK ( player_gst_msg_cll ), base );
    g_signal_connect ( bus, "message::buffering",   G_CALLBACK ( player_gst_msg_buf ), base );

    gst_object_unref ( bus );

    return playbin;
}


// Record IPTV

static void player_gst_pad_link ( GstPad *pad, GstElement *element, const char *name, G_GNUC_UNUSED GstElement *element_n )
{
	GstPad *pad_va_sink = gst_element_get_static_pad ( element, "sink" );

	if ( gst_pad_link ( pad, pad_va_sink ) == GST_PAD_LINK_OK )
		gst_object_unref ( pad_va_sink );
	else
		g_debug ( "player_gst_pad_link:: linking demux/decode name %s video/audio pad failed", name );
}

static void player_gst_pad_demux ( GstElement *element, GstPad *pad, GstElement *element_l )
{
	player_gst_pad_link ( pad, element_l, "hlsdemux", element );
}

static void player_gst_pad_demux_audio ( GstElement *element, GstPad *pad, GstElement *element_audio )
{
	const char *name = gst_structure_get_name ( gst_caps_get_structure ( gst_pad_query_caps ( pad, NULL ), 0 ) );

	if ( g_str_has_prefix ( name, "audio" ) ) player_gst_pad_link ( pad, element_audio, name, element );
}

static void player_gst_pad_demux_video ( GstElement *element, GstPad *pad, GstElement *element_video )
{
	const char *name = gst_structure_get_name ( gst_caps_get_structure ( gst_pad_query_caps ( pad, NULL ), 0 ) );

	if ( g_str_has_prefix ( name, "video" ) ) player_gst_pad_link ( pad, element_video, name, element );
}

static void player_gst_pad_decode ( GstElement *element, GstPad *pad, GstElement *element_va )
{
	const char *name = gst_structure_get_name ( gst_caps_get_structure ( gst_pad_query_caps ( pad, NULL ), 0 ) );

	player_gst_pad_link ( pad, element_va, name, element );
}

static void player_gst_msg_buf_rec ( G_GNUC_UNUSED GstBus *bus, GstMessage *msg, Base *base )
{
	if ( base->app_quit ) return;

	int percent;
	gst_message_parse_buffering ( msg, &percent );

	if ( percent == 100 )
	{
		if ( GST_ELEMENT_CAST ( base->player->pipeline_rec )->current_state == GST_STATE_PAUSED )
			gst_element_set_state ( base->player->pipeline_rec, GST_STATE_PLAYING );

		gtk_label_set_text ( base->player->slider_base.rec_buf, "" );

		if ( !base->player->panel_quit && gtk_widget_get_visible ( GTK_WIDGET ( base->player->h_box_slider_panel ) ) )
				gtk_label_set_text ( base->player->slider_panel.rec_buf, "" );
	}
	else
	{
		if ( GST_ELEMENT_CAST ( base->player->pipeline_rec )->current_state == GST_STATE_PLAYING )
			gst_element_set_state ( base->player->pipeline_rec, GST_STATE_PAUSED );

		char *str = g_strdup_printf ( " %d%s ", percent, "%" );

			gtk_label_set_text ( base->player->slider_base.rec_buf, str );

			if ( !base->player->panel_quit && gtk_widget_get_visible ( GTK_WIDGET ( base->player->h_box_slider_panel ) ) )
				gtk_label_set_text ( base->player->slider_panel.rec_buf, str );

		free ( str );

		// g_print ( "Rec buffering: %d %s \n", percent, "%" );
	}
}

static gboolean player_record_refresh ( Base *base )
{
	if ( base->app_quit ) return FALSE;

	if ( !base->player->record )
	{
		gtk_label_set_text ( base->player->slider_base.rec_buf, "" );

		if ( !base->player->panel_quit && gtk_widget_get_visible ( GTK_WIDGET ( base->player->h_box_slider_panel ) ) )
			gtk_label_set_text ( base->player->slider_panel.rec_buf, "" );

		return FALSE;
	}

	if ( GST_ELEMENT_CAST ( base->player->pipeline_rec )->current_state == GST_STATE_PAUSED ) return TRUE;

	gint64 duration = 0, current = 0;

	if ( gst_element_query_position ( base->player->pipeline_rec, GST_FORMAT_TIME, &current ) )
	{
		if ( gst_element_query_duration ( base->player->pipeline_rec, GST_FORMAT_TIME, &duration ) )
		{
			if ( duration > 0 )
				player_slider_set_data ( base, current, 10, duration, 10, TRUE );
			else
				player_slider_set_data ( base, current, 10, -1, 10, FALSE );

			if ( duration > 0 && current / GST_SECOND == duration / GST_SECOND )
			{
				player_stop_record ( base, FALSE );
			}
		}
		else
			player_slider_set_data ( base, current, 10, -1, 10, FALSE );
	}

	const char *format = "<span foreground=\"#FF0000\">%s</span>";

	char *markup = g_markup_printf_escaped ( format, ( base->player->rec_status ) ? " ◉ " : " ◌ " );

		gtk_label_set_markup ( base->player->slider_base.rec_buf, markup );

		if ( !base->player->panel_quit && gtk_widget_get_visible ( GTK_WIDGET ( base->player->h_box_slider_panel ) ) )
			gtk_label_set_markup ( base->player->slider_panel.rec_buf, markup );

		base->player->rec_status = !base->player->rec_status;

	g_free ( markup );

	return TRUE;
}

static void player_gst_rec_remove ( GstElement *pipeline )
{
	GstIterator *it = gst_bin_iterate_elements ( GST_BIN ( pipeline ) );
	GValue item = { 0, };
	gboolean done = FALSE;

	while ( !done )
	{
		switch ( gst_iterator_next ( it, &item ) )
		{
			case GST_ITERATOR_OK:
			{
				GstElement *element = GST_ELEMENT ( g_value_get_object (&item) );

				char *object_name = gst_object_get_name ( GST_OBJECT ( element ) );

				g_debug ( "Object remove: %s", object_name );

				gst_bin_remove ( GST_BIN ( pipeline ), element );

				g_free ( object_name );
				g_value_reset (&item);

				break;
			}				

			case GST_ITERATOR_RESYNC:
				gst_iterator_resync (it);
				break;

			case GST_ITERATOR_ERROR:
				done = TRUE;
				break;

			case GST_ITERATOR_DONE:
				done = TRUE;
				break;
		}
	}

	g_value_unset ( &item );
	gst_iterator_free ( it );
}

static char * player_time_to_str ()
{
	GDateTime *date = g_date_time_new_now_local ();

	char *str_time = g_date_time_format ( date, "%j-%Y-%T" );

	g_date_time_unref ( date );

	return str_time;
}

static void player_rec_set_location ( GstElement *element, char *rec_dir )
{
	char *date_str = player_time_to_str ();
	char *file_rec = g_strdup_printf ( "%s/Record-iptv-%s", rec_dir, date_str );

	g_object_set ( element, "location", file_rec, NULL );

	g_free ( file_rec );
	g_free ( date_str );
}

static GstElement * player_gst_create_rec_pipeline ( Base *base )
{
	GstElement *pipeline_rec = gst_pipeline_new ( "pipeline-record" );

    if ( !pipeline_rec )
    {
        g_printerr ( "pipeline_rec - not created.\n" );

        return NULL;
    }

    GstBus *bus = gst_element_get_bus ( pipeline_rec );

    gst_bus_add_signal_watch_full ( bus, G_PRIORITY_DEFAULT );
    gst_bus_set_sync_handler ( bus, (GstBusSyncHandler)player_gst_bus_sync_handler, base, NULL );

    g_signal_connect ( bus, "message::error",     G_CALLBACK ( player_gst_msg_err     ), base );
    g_signal_connect ( bus, "message::buffering", G_CALLBACK ( player_gst_msg_buf_rec ), base );

    gst_object_unref ( bus );

    return pipeline_rec;
}

static GstElement * player_gst_rec_create ( Base *base, uint n_video, uint n_audio )
{
	GstElement *pipeline_rec = player_gst_create_rec_pipeline ( base );

    if ( !pipeline_rec ) return NULL;

	struct rec_all { const char *name; } rec_all_n[] =
	{
		{ "souphttpsrc" }, { "tee" }, { "queue" }, { "decodebin" },
		{ "queue" }, { "decodebin" }, { "audioconvert" }, { "equalizer-nbands" }, { "autoaudiosink" },
		{ "queue" }, { "decodebin" }, { "videoconvert" }, { "videobalance"     }, { "autovideosink" },
		{ "queue" }, { "filesink"  }
	};

	GstElement *elements[ G_N_ELEMENTS ( rec_all_n ) ];

	uint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( rec_all_n ); c++ )
	{
		if ( n_video == 0 && ( c == 9 || c == 10 || c == 11 || c == 12 || c == 13 ) ) continue;

		if (  c == 0 )
			elements[c] = gst_element_make_from_uri ( GST_URI_SRC, base->player->file_play, NULL, NULL );
		else
			elements[c] = gst_element_factory_make ( rec_all_n[c].name, NULL );

		if ( !elements[c] )
		{
			g_critical ( "player_gst_rec_create:: element (factory make) - %s not created. \n", rec_all_n[c].name );

			return NULL;
		}

		gst_bin_add ( GST_BIN ( pipeline_rec ), elements[c] );

		if (  c == 0 || c == 4 || c == 6 || c == 9 || c == 11 || c == 14 ) continue;

		gst_element_link ( elements[c-1], elements[c] );
	}

	gst_element_link ( elements[1], elements[14] );

	if ( n_audio > 0 ) g_signal_connect ( elements[3],  "pad-added", G_CALLBACK ( player_gst_pad_demux_audio ), elements[4] );
	if ( n_video > 0 ) g_signal_connect ( elements[3],  "pad-added", G_CALLBACK ( player_gst_pad_demux_video ), elements[9] );

	if ( n_audio > 0 ) g_signal_connect ( elements[5],  "pad-added", G_CALLBACK ( player_gst_pad_decode ), elements[6]  );
	if ( n_video > 0 ) g_signal_connect ( elements[10], "pad-added", G_CALLBACK ( player_gst_pad_decode ), elements[11] );

	/* g_object_set ( element, "volume", base->player->volume, NULL ); ? */

    g_object_set ( elements[3],  "use-buffering", TRUE, NULL );

	if ( g_object_class_find_property ( G_OBJECT_GET_CLASS ( elements[0] ), "location" ) )
		g_object_set ( elements[0],  "location", base->player->file_play, NULL );
	else
		g_object_set ( elements[0],  "uri", base->player->file_play, NULL );

	player_rec_set_location ( elements[15], base->rec_dir );

    return pipeline_rec;
}

static GstElement * player_gst_rec_create_hls ( Base *base, uint n_video, uint n_audio )
{
	GstElement *pipeline_rec = player_gst_create_rec_pipeline ( base );

    if ( !pipeline_rec ) return NULL;

	struct rec_all { const char *name; } rec_all_n[] =
	{
		{ "souphttpsrc" }, { "hlsdemux" }, { "tee" }, { "queue" }, { "decodebin" },
		{ "queue" }, { "decodebin" }, { "audioconvert" }, { "equalizer-nbands" }, { "autoaudiosink" },
		{ "queue" }, { "decodebin" }, { "videoconvert" }, { "videobalance"     }, { "autovideosink" },
		{ "queue" }, { "filesink"  }
	};

	GstElement *elements[ G_N_ELEMENTS ( rec_all_n ) ];

	uint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( rec_all_n ); c++ )
	{
		if ( n_video == 0 && ( c == 10 || c == 11 || c == 12 || c == 13 || c == 14 ) ) continue;

		if (  c == 0 )
			elements[c] = gst_element_make_from_uri ( GST_URI_SRC, base->player->file_play, NULL, NULL );
		else
			elements[c] = gst_element_factory_make ( rec_all_n[c].name, NULL );

		if ( !elements[c] )
		{
			g_critical ( "player_gst_rec_create:: element (factory make) - %s not created. \n", rec_all_n[c].name );

			return NULL;
		}

		gst_bin_add ( GST_BIN ( pipeline_rec ), elements[c] );

		if (  c == 0 || c == 2 || c == 5 || c == 7 || c == 10 || c == 12 || c == 15 ) continue;

		gst_element_link ( elements[c-1], elements[c] );
	}

	gst_element_link ( elements[2], elements[15] );

	g_signal_connect ( elements[1], "pad-added", G_CALLBACK ( player_gst_pad_demux ), elements[2] );

	if ( n_audio > 0 ) g_signal_connect ( elements[4], "pad-added", G_CALLBACK ( player_gst_pad_demux_audio ), elements[5] );
	if ( n_video > 0 ) g_signal_connect ( elements[4], "pad-added", G_CALLBACK ( player_gst_pad_demux_video ), elements[10] );

	if ( n_audio > 0 ) g_signal_connect ( elements[6],  "pad-added", G_CALLBACK ( player_gst_pad_decode ), elements[7]  );
	if ( n_video > 0 ) g_signal_connect ( elements[11], "pad-added", G_CALLBACK ( player_gst_pad_decode ), elements[12] );

    /* g_object_set ( element, "volume", base->player->volume, NULL ); ? */

    g_object_set ( elements[4],  "use-buffering", TRUE, NULL );

	if ( g_object_class_find_property ( G_OBJECT_GET_CLASS ( elements[0] ), "location" ) )
		g_object_set ( elements[0],  "location", base->player->file_play, NULL );
	else
		g_object_set ( elements[0],  "uri", base->player->file_play, NULL );

	player_rec_set_location ( elements[16], base->rec_dir );

    return pipeline_rec;
}

static void player_stop_record ( Base *base, gboolean play )
{
    if ( base->player->record )
    {
		gst_element_set_state ( base->player->pipeline_rec, GST_STATE_NULL );

		player_gst_rec_remove ( base->player->pipeline_rec );

		gst_object_unref ( base->player->pipeline_rec );

		base->player->pipeline_rec = NULL;

		gtk_widget_queue_draw ( GTK_WIDGET ( base->window ) );

		base->player->record = !base->player->record;

		if ( play ) player_play ( base );
	}
}

void player_record ( Base *base )
{
    if ( base->player->record )
    {
		player_stop_record ( base, TRUE );
	}
	else
	{
		if ( GST_ELEMENT_CAST ( base->player->playbin )->current_state != GST_STATE_PLAYING ) return;

		uint n_video, n_audio;
		g_object_get ( base->player->playbin, "n-video", &n_video, NULL );
		g_object_get ( base->player->playbin, "n-audio", &n_audio, NULL );

		// if ( n_video == 0 && n_audio == 0 ) return;

		if ( !gst_uri_is_valid ( base->player->file_play ) || g_str_has_prefix ( base->player->file_play, "file://" ) )
		{
			base_message_dialog ( "", "Only IPTV.", GTK_MESSAGE_WARNING, base->window );

			return;
		}

		char *uri_protocol = gst_uri_get_protocol ( base->player->file_play );	
		gboolean is_supported = gst_uri_protocol_is_supported ( GST_URI_SRC, uri_protocol );

		if ( !is_supported )
		{
			base_message_dialog ( "", "The protocol is not supported.", GTK_MESSAGE_WARNING, base->window );

			g_free ( uri_protocol );

			return;
		}

		g_free ( uri_protocol );

		GstElement *element = player_gst_ret_iterate_element ( base->player->playbin, "hlsdemux", NULL );

		if ( element == NULL )
			base->player->pipeline_rec = player_gst_rec_create ( base, n_video, n_audio );
		else
			base->player->pipeline_rec = player_gst_rec_create_hls ( base, n_video, n_audio );

		if ( base->player->pipeline_rec != NULL )
		{
			player_stop ( base );

			gst_element_set_state ( base->player->pipeline_rec, GST_STATE_PLAYING );

			base->player->record = !base->player->record;

			g_timeout_add_seconds ( 1, (GSourceFunc)player_record_refresh, base );
		}
	}
}
