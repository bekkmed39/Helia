/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include <gst/video/videooverlay.h>

#include <base.h>

#include "dtv-level.h"
#include "scan.h"
#include "mpegts.h"


GstElement * dtv_gst_ret_iterate_element ( GstElement *it_element, const char *name1, const char *name2 )
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

gboolean dtv_mute_get ( GstElement *dvbplay )
{
	if ( GST_ELEMENT_CAST ( dvbplay )->current_state != GST_STATE_PLAYING ) return TRUE;

	GstElement *element = dtv_gst_ret_iterate_element ( dvbplay, "autoaudiosink", "actual-sink" );

	if ( element == NULL ) return FALSE;

	gboolean mute = FALSE;

	g_object_get ( element, "mute", &mute, NULL );

	return mute;
}

void dtv_mute_set ( GstElement *dvbplay )
{
	if ( GST_ELEMENT_CAST ( dvbplay )->current_state != GST_STATE_PLAYING ) return;

	GstElement *element = dtv_gst_ret_iterate_element ( dvbplay, "autoaudiosink", "actual-sink" );

	if ( element == NULL ) return;

	gboolean mute = FALSE;

	g_object_get ( element, "mute", &mute, NULL );
	g_object_set ( element, "mute", !mute, NULL );
}

static void dtv_gst_volume_set ( GstElement *dvbplay, gdouble value )
{
	if ( GST_ELEMENT_CAST ( dvbplay )->current_state != GST_STATE_PLAYING ) return;

	GstElement *element = dtv_gst_ret_iterate_element ( dvbplay, "autoaudiosink", "actual-sink" );

	if ( element == NULL ) return;

	g_object_set ( element, "volume", value, NULL );
}

void dtv_volume_changed ( G_GNUC_UNUSED GtkScaleButton *button, gdouble value, Base *base )
{
	if ( GST_ELEMENT_CAST ( base->dtv->dvbplay )->current_state != GST_STATE_PLAYING ) return;

	base->dtv->volume = value;

	dtv_gst_volume_set ( base->dtv->dvbplay, base->dtv->volume );
}


static gboolean dtv_gst_set_audio_track ( GstPad *pad, GstElement *element, int set_track_audio, const char *name, GstElement *element_n, Base *base )
{
	gboolean audio_changed = TRUE;

	if ( pad )
	{
		if ( base->dtv->count_audio_track >= MAX_AUDIO )
		{
			g_debug ( "dtv_gst_set_audio_track:: MAX_AUDIO %d", MAX_AUDIO );
			return FALSE;
		}

		base->dtv->pad_a_sink[base->dtv->count_audio_track] = gst_element_get_static_pad ( element, "sink" );
		base->dtv->pad_a_src [base->dtv->count_audio_track] = pad;

		if ( gst_pad_link ( pad, base->dtv->pad_a_sink[base->dtv->count_audio_track] ) == GST_PAD_LINK_OK )
			gst_object_unref ( base->dtv->pad_a_sink[base->dtv->count_audio_track] );
		else
		{
			char *object_name = gst_object_get_name ( GST_OBJECT ( element_n ) );
				g_debug ( "Linking demux name: %s & audio pad failed - %s", object_name, name );
			g_free ( object_name );
		}

		base->dtv->count_audio_track++;
	}
	else
	{
		if ( !gst_pad_unlink ( base->dtv->pad_a_src[base->dtv->set_audio_track], base->dtv->pad_a_sink[base->dtv->set_audio_track] ) )
			audio_changed = FALSE;

		if ( gst_pad_link ( base->dtv->pad_a_src[set_track_audio], base->dtv->pad_a_sink[set_track_audio] ) != GST_PAD_LINK_OK )
			audio_changed = FALSE;
	}

	return audio_changed;
}

void dtv_gst_changed_audio_track ( Base *base, int changed_track_audio )
{
	dtv_gst_set_audio_track ( NULL, NULL, changed_track_audio, NULL, NULL, base );

	base->dtv->set_audio_track = changed_track_audio;
}

void dtv_gst_add_audio_track ( Base *base, GtkComboBoxText *combo )
{
	uint i;
	for ( i = 0; i < base->dtv->count_audio_track; i++ )
	{
		if ( base->dtv->audio_lang[i] )
		{
			gtk_combo_box_text_append_text ( combo, base->dtv->audio_lang[i] );
		}
		else
		{
			char *text = g_strdup_printf ( "Audio %d", i + 1 );

				gtk_combo_box_text_append_text ( combo, text );

			g_free ( text );
		}
	}

	gtk_combo_box_set_active ( GTK_COMBO_BOX ( combo ), base->dtv->set_audio_track );
}

static void dtv_gst_pad_link ( GstPad *pad, GstElement *element, const char *name, G_GNUC_UNUSED GstElement *element_n )
{
	GstPad *pad_va_sink = gst_element_get_static_pad ( element, "sink" );

	if ( gst_pad_link ( pad, pad_va_sink ) == GST_PAD_LINK_OK )
		gst_object_unref ( pad_va_sink );
	else
		g_debug ( "tv_gst_pad_link:: linking demux/decode name %s video/audio pad failed", name );
}

static void dtv_gst_pad_demux_audio ( GstElement *element, GstPad *pad, Base *base /* GstElement *element_audio */ )
{
	const char *name = gst_structure_get_name ( gst_caps_get_structure ( gst_pad_query_caps ( pad, NULL ), 0 ) );

	if ( g_str_has_prefix ( name, "audio" ) )
		//dtv_gst_pad_link ( pad, element_audio, name, element );
		dtv_gst_set_audio_track ( pad, base->dtv->e_audio, base->dtv->count_audio_track, name, element, base );
}

static void dtv_gst_pad_demux_video ( GstElement *element, GstPad *pad, GstElement *element_video )
{
	const char *name = gst_structure_get_name ( gst_caps_get_structure ( gst_pad_query_caps ( pad, NULL ), 0 ) );

	if ( g_str_has_prefix ( name, "video" ) ) dtv_gst_pad_link ( pad, element_video, name, element );
}

static void dtv_gst_pad_decode ( GstElement *element, GstPad *pad, GstElement *element_va )
{
	const char *name = gst_structure_get_name ( gst_caps_get_structure ( gst_pad_query_caps ( pad, NULL ), 0 ) );

	dtv_gst_pad_link ( pad, element_va, name, element );
}

static void dtv_gst_create_dvb_bin ( GstElement *element, gboolean video_enable, Base *base )
{
	struct dvb_all_list { const char *name; } dvb_all_list_n[] =
	{
		{ "dvbsrc" }, { "tsdemux" },
		{ "tee"    }, { "queue2"  }, { "decodebin" }, { "audioconvert" }, { "equalizer-nbands" }, { "autoaudiosink" },
		{ "tee"    }, { "queue2"  }, { "decodebin" }, { "videoconvert" }, { "videobalance"     }, { "autovideosink" }
	};

	GstElement *elements[ G_N_ELEMENTS ( dvb_all_list_n ) ];

	uint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( dvb_all_list_n ); c++ )
	{
		if ( !video_enable && c > 7 ) continue;

		const char *unique_name = NULL;

		if ( c == 2  ) unique_name = "tee-audio";
		if ( c == 8  ) unique_name = "tee-video";
		if ( c == 4  ) unique_name = "decodebin-audio";
		if ( c == 10 ) unique_name = "decodebin-video";

		elements[c] = gst_element_factory_make ( dvb_all_list_n[c].name, unique_name );

		if ( !elements[c] )
			g_critical ( "dtv_gst_create_dvb_bin:: element (factory make) - %s not created. \n", dvb_all_list_n[c].name );

		gst_bin_add ( GST_BIN ( element ), elements[c] );

		if (  c == 0 || c == 2 || c == 5 || c == 8 || c == 11 ) continue;

		gst_element_link ( elements[c-1], elements[c] );
	}

	base->dtv->e_audio = elements[2];

	g_signal_connect ( elements[1], "pad-added", G_CALLBACK ( dtv_gst_pad_demux_audio ), base );
	if ( video_enable ) g_signal_connect ( elements[1], "pad-added", G_CALLBACK ( dtv_gst_pad_demux_video ), elements[8] );

	g_signal_connect ( elements[4], "pad-added", G_CALLBACK ( dtv_gst_pad_decode ), elements[5] );
	if ( video_enable ) g_signal_connect ( elements[10], "pad-added", G_CALLBACK ( dtv_gst_pad_decode ), elements[11] );
}

static void dtv_gst_remove_dvb_bin ( GstElement *pipeline )
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

struct list_types { const char *type; const char *parser; };

struct list_types list_type_video_n[] =
{
	{ "mpeg",   "mpegvideoparse"  },
	{ "h264",   "h264parse"       },
	{ "h265",   "h265parse"       },
	{ "vc1",    "vc1parse"        }
};
struct list_types list_type_audio_n[] =
{
	{ "mpeg", "mpegaudioparse" 	},
	{ "ac3",  "ac3parse" 		},
	{ "aac",  "aacparse" 		}
};

static const char * dtv_gst_ret_name_iterate ( GstElement *it_element, struct list_types list_types_all[], uint num )
{
	GstIterator *it = gst_bin_iterate_recurse ( GST_BIN ( it_element ) );
	GValue item = { 0, };
	gboolean done = FALSE;

	const char *ret = NULL;
	uint c = 0;

	while ( !done )
	{
		switch ( gst_iterator_next ( it, &item ) )
		{
			case GST_ITERATOR_OK:
			{
				g_debug ( "GST_ITERATOR_OK" );
				GstElement *element = GST_ELEMENT ( g_value_get_object (&item) );

				char *object_name = gst_object_get_name ( GST_OBJECT ( element ) );

				if ( g_strrstr ( object_name, "parse" ) )
				{
					for ( c = 0; c < num; c++ )
						if ( g_strrstr ( object_name, list_types_all[c].type ) )
							ret = list_types_all[c].parser;
				}

				g_debug ( "Object name: %s ", object_name );

				g_free ( object_name );
				g_value_reset (&item);

				break;
			}

			case GST_ITERATOR_RESYNC:
				g_debug ( "GST_ITERATOR_RESYNC" );
				gst_iterator_resync (it);
				break;

			case GST_ITERATOR_ERROR:
				g_debug ( "GST_ITERATOR_ERROR" );
				done = TRUE;
				break;

			case GST_ITERATOR_DONE:
				g_debug ( "GST_ITERATOR_DONE" );
				done = TRUE;
				break;
		}
	}

	g_value_unset ( &item );
	gst_iterator_free ( it );

	return ret;
}

static char * dtv_time_to_str ()
{
	GDateTime *date = g_date_time_new_now_local ();

	char *str_time = g_date_time_format ( date, "%j-%Y-%T" );

	g_date_time_unref ( date );

	return str_time;
}

static char * dtv_str_split ( char *data )
{
	char *ret_ch = NULL;

	char **lines = g_strsplit ( data, ":", 0 );

		ret_ch = g_strdup ( lines[0] );

	g_strfreev ( lines );

	return ret_ch;
}

static void dtv_rec_set_location ( GstElement *element, char *rec_dir, char *ch_data )
{
	char *date_str = dtv_time_to_str ();
	char *name     = dtv_str_split ( ch_data );
	char *file_rec = g_strdup_printf ( "%s/%s_%s.%s", rec_dir, name, date_str, "m2ts" );

	g_object_set ( element, "location", file_rec, NULL );

	g_free ( file_rec );
	g_free ( name     );
	g_free ( date_str );
}

static gboolean dtv_gst_create_rec ( Base *base )
{
	GstElement *decodebin_video = NULL;
	GstElement *decodebin_audio = dtv_gst_ret_iterate_element ( base->dtv->dvbplay, "decodebin-audio", NULL );

	const char *video_parser = NULL;
	const char *audio_parser = dtv_gst_ret_name_iterate ( decodebin_audio, list_type_audio_n, G_N_ELEMENTS ( list_type_audio_n ) );

	if ( base->dtv->checked_video )
	{
		decodebin_video = dtv_gst_ret_iterate_element ( base->dtv->dvbplay, "decodebin-video", NULL );
		video_parser = dtv_gst_ret_name_iterate ( decodebin_video, list_type_video_n, G_N_ELEMENTS ( list_type_video_n ) );
	}

	if ( !audio_parser || ( !video_parser && base->dtv->checked_video ) ) return FALSE;

	struct dvb_rec_all_list { const char *name; } dvb_all_rec_list_n[] =
	{
		{ "queue2"    }, { video_parser },
		{ "queue2"    }, { audio_parser },
		{ "mpegtsmux" }, { "filesink"   }
	};

	g_debug ( "dtv_gst_create_rec:: video parser: %s | audio parser: %s", video_parser, audio_parser );

	GstElement *element_video = dtv_gst_ret_iterate_element ( base->dtv->dvbplay, "tee-video", NULL );
	GstElement *element_audio = dtv_gst_ret_iterate_element ( base->dtv->dvbplay, "tee-audio", NULL );

	gst_element_set_state ( base->dtv->dvbplay, GST_STATE_PAUSED );  // GST_STATE_NULL

	GstElement *elements[ G_N_ELEMENTS ( dvb_all_rec_list_n ) ];

	uint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( dvb_all_rec_list_n ); c++ )
	{
		const char *unique_name = NULL;

		if ( c == 4  ) unique_name = "mpegtsmux";

		if ( !base->dtv->checked_video && ( c == 0 || c == 1 ) ) continue;

		elements[c] = gst_element_factory_make ( dvb_all_rec_list_n[c].name, unique_name );

		if ( !elements[c] )
			g_critical ( "dvb_all_list:: element (factory make)  - %s not created. \n", dvb_all_rec_list_n[c].name );

		gst_bin_add ( GST_BIN ( base->dtv->dvbplay ), elements[c] );

		if ( c == 1 || c == 3 || c == 5 )
			gst_element_link ( elements[c-1], elements[c] );
	}

	if ( base->dtv->checked_video )
	{
		gst_element_link ( element_video, elements[0] );
		gst_element_link ( elements[1],   elements[4] );
	}

	gst_element_link ( element_audio, elements[2] );
	gst_element_link ( elements[3], elements[4] );

	dtv_rec_set_location ( elements[5], base->rec_dir, base->dtv->ch_data );

	gst_element_set_state ( base->dtv->dvbplay, GST_STATE_PLAYING );

	return TRUE;
}
/*
static GstPadProbeReturn dtv_gst_blockpad_probe_event ( GstPad * pad, GstPadProbeInfo * info, gpointer data )
{
	if ( GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_EOS )
		return GST_PAD_PROBE_PASS;

	Base *base = (Base *) data;

	GstElement *element = dtv_gst_ret_iterate_element ( base->dtv->dvbplay, "filesink", NULL );

	if ( element == NULL ) return GST_PAD_PROBE_PASS;

	char *file_name = NULL;
	g_object_get ( element, "location", &file_name, NULL );

	gst_element_set_state ( element, GST_STATE_NULL );

	if ( g_str_has_prefix ( file_name, "/dev/null" ) )
	{
		dtv_rec_set_location ( element, base->rec_dir, base->dtv->ch_data );

		base->dtv->rec_ses = TRUE;
	}
	else
	{
		g_object_set ( element, "location", "/dev/null", NULL );

		base->dtv->rec_ses = FALSE;
	}

	gst_pad_remove_probe ( pad, GST_PAD_PROBE_INFO_ID (info) );

	gst_element_set_state ( element, GST_STATE_PLAYING );

	g_free ( file_name );

	return GST_PAD_PROBE_DROP;
}
*/
static GstPadProbeReturn dtv_gst_blockpad_probe ( GstPad * pad, GstPadProbeInfo * info, gpointer data )
{
	Base *base = (Base *) data;

	GstElement *element = dtv_gst_ret_iterate_element ( base->dtv->dvbplay, "filesink", NULL );

	if ( element == NULL ) return GST_PAD_PROBE_PASS;
	
	char *file_name = NULL;
	g_object_get ( element, "location", &file_name, NULL );

	gst_element_set_state ( element, GST_STATE_NULL );

	if ( g_str_has_prefix ( file_name, "/dev/null" ) )
	{
		dtv_rec_set_location ( element, base->rec_dir, base->dtv->ch_data );

		base->dtv->rec_ses = TRUE;
	}
	else
	{
		g_object_set ( element, "location", "/dev/null", NULL );

		base->dtv->rec_ses = FALSE;
	}

	gst_pad_remove_probe ( pad, GST_PAD_PROBE_INFO_ID (info) );

	gst_element_set_state ( element, GST_STATE_PLAYING );

	g_free ( file_name );

	/*
	GstPad *sinkpad = gst_element_get_static_pad ( element, "sink" );

	gst_pad_add_probe ( sinkpad, GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
						dtv_gst_blockpad_probe_event, base, NULL );

	gst_pad_remove_probe ( pad, GST_PAD_PROBE_INFO_ID (info) );

	gst_object_unref ( sinkpad );
	*/

	return GST_PAD_PROBE_OK;
}

static gboolean dtv_check_rec ( Base *base )
{
	if ( base->app_quit ) return FALSE;

	if ( GST_ELEMENT_CAST ( base->dtv->dvbplay )->current_state == GST_STATE_PLAYING ) return FALSE;

	if ( GST_ELEMENT_CAST ( base->dtv->dvbplay )->current_state != GST_STATE_PLAYING )
	{
		gst_element_set_state ( base->dtv->dvbplay, GST_STATE_NULL );

		gst_element_set_state ( base->dtv->dvbplay, GST_STATE_PLAYING );
	}

	return FALSE;
}

static gboolean dtv_gst_checked_video ( const char *data )
{
	gboolean video_enable = TRUE;

	if ( !g_strrstr ( data, "video-pid" ) || g_strrstr ( data, "video-pid=0" ) ) video_enable = FALSE;

	return video_enable;
}

static gboolean dtv_gst_find_property_scrambling ( GstElement *element )
{
	gboolean scrambling = FALSE;

	if ( element && g_object_class_find_property ( G_OBJECT_GET_CLASS ( element ), "scrambling" ) )
		scrambling = TRUE;
	else
		scrambling = FALSE;

	return scrambling;
}
static gboolean dtv_gst_get_property_scrambling ( Base *base )
{
	GstElement *element = dtv_gst_ret_iterate_element ( base->dtv->dvbplay, "tsdemux", NULL );

	if ( element == NULL ) return FALSE;

	gboolean scrambling, property_scrambling = dtv_gst_find_property_scrambling ( element );

	if ( property_scrambling ) g_object_get ( element, "scrambling", &scrambling, NULL );

	return scrambling;
}
static void dtv_gst_scrambling ( GstElement *element, char *name )
{
	gboolean scrambling = dtv_gst_find_property_scrambling ( element );

	g_debug ( "GstTSDemux property scrambling: %s ", scrambling ? "TRUE": "FALSE" );

	if ( scrambling ) g_object_set ( element, "prog-name", name, NULL );
}

static void dtv_gst_set_tuning_timeout ( GstElement *element )
{
	guint64 timeout = 0;
	g_object_get ( element, "tuning-timeout", &timeout, NULL );
	g_object_set ( element, "tuning-timeout", (guint64)timeout / 5, NULL );
}

static void dtv_gst_tv_delsys ( GstElement *element )
{
	uint adapter = 0, frontend = 0, delsys = 0;
	g_object_get ( element, "adapter",  &adapter,  NULL );
	g_object_get ( element, "frontend", &frontend, NULL );
	g_object_get ( element, "delsys",   &delsys,   NULL );

	char *dvb_name = helia_get_dvb_info ( adapter, frontend );
	const char *dvb_type = helia_get_dvb_type_str ( delsys );

	g_print ( "DVB device: %s ( %s ) | adapter %d frontend %d \n\n", dvb_name, dvb_type, adapter, frontend );

	g_free ( dvb_name );
}

static void dtv_gst_data_set ( GstElement *pipeline, const char *data, Base *base )
{
	GstElement *element = dtv_gst_ret_iterate_element ( pipeline, "dvbsrc", NULL );
	dtv_gst_set_tuning_timeout ( element );

	char **fields = g_strsplit ( data, ":", 0 );
	uint numfields = g_strv_length ( fields );

	uint j = 0;
	for ( j = 1; j < numfields; j++ )
	{
		if ( g_strrstr ( fields[j], "audio-pid" ) || g_strrstr ( fields[j], "video-pid" ) ) continue;

		if ( !g_strrstr ( fields[j], "=" ) ) continue;

		char **splits = g_strsplit ( fields[j], "=", 0 );

		g_debug ( "tv_gst_data_set: gst-param %s | gst-value %s", splits[0], splits[1] );

		if ( g_strrstr ( splits[0], "polarity" ) )
		{
			if ( splits[1][0] == 'v' || splits[1][0] == 'V' || splits[1][0] == '0' )
				g_object_set ( element, "polarity", "V", NULL );
			else
				g_object_set ( element, "polarity", "H", NULL );

			g_strfreev (splits);

			continue;
		}

		long dat = atol ( splits[1] );

		if ( g_strrstr ( splits[0], "program-number" ) )
		{
			base->dtv->sid = dat;

			GstElement *demux = dtv_gst_ret_iterate_element ( pipeline, "tsdemux", NULL );

			g_object_set ( demux, "program-number", base->dtv->sid, NULL );

			dtv_gst_scrambling ( demux, fields[0] );
		}
		else if ( g_strrstr ( splits[0], "symbol-rate" ) )
		{
			g_object_set ( element, "symbol-rate", ( dat > 100000) ? dat/1000 : dat, NULL );
		}
		else if ( g_strrstr ( splits[0], "lnb-type" ) )
		{
			set_lnb_low_high_switch ( element, dat );
		}
		else
		{
			g_object_set ( element, splits[0], dat, NULL );
		}

		g_strfreev (splits);
	}

	g_strfreev (fields);

	dtv_gst_tv_delsys ( element );
}

void dtv_stop ( Base *base )
{
	if ( GST_ELEMENT_CAST ( base->dtv->dvbplay )->current_state != GST_STATE_NULL )
	{
		gst_element_set_state ( base->dtv->dvbplay, GST_STATE_NULL );

		dtv_gst_remove_dvb_bin ( base->dtv->dvbplay );

		base->dtv->rec_tv     = FALSE;
		base->dtv->rec_ses    = FALSE;
		base->dtv->scrambling = FALSE;

		base->dtv->count_audio_track = 0;
		base->dtv->set_audio_track   = 0;

		uint i; for ( i = 0; i < MAX_AUDIO; i++ )
			{ if ( base->dtv->audio_lang[i] ) g_free ( base->dtv->audio_lang[i] ); base->dtv->audio_lang[i] = NULL; }

		dtv_level_set_sgn_snr ( base, base->dtv->level_base, 0, 0, FALSE );

		if ( !base->dtv->panel_quit && gtk_widget_get_visible ( GTK_WIDGET ( base->dtv->h_box_level_panel ) ) )
		{
			dtv_level_set_sgn_snr ( base, base->dtv->level_panel, 0, 0, FALSE );
		}

		gtk_widget_queue_draw ( GTK_WIDGET ( base->window ) );
	}
}

static gboolean dtv_update_win ( Base *base )
{
	if ( base->app_quit ) return FALSE;

	if ( GST_ELEMENT_CAST ( base->dtv->dvbplay )->current_state == GST_STATE_NULL ) return FALSE;

	if ( GST_ELEMENT_CAST ( base->dtv->dvbplay )->current_state == GST_STATE_PLAYING )
	{
		base->dtv->scrambling = dtv_gst_get_property_scrambling ( base );

		dtv_gst_volume_set ( base->dtv->dvbplay, base->dtv->volume );

		gtk_widget_queue_draw ( GTK_WIDGET ( base->window ) );

		return FALSE;
	}
	else
	{
		base->dtv->scrambling = dtv_gst_get_property_scrambling ( base );

		time ( &base->dtv->t_cur_tv );

		if ( ( base->dtv->t_cur_tv - base->dtv->t_start_tv ) >= 25 )
		{
			g_warning ( "Time stop %ld (sec) ", base->dtv->t_cur_tv - base->dtv->t_start_tv );

			dtv_stop ( base );

			return FALSE;
		}
	}

	return TRUE;
}

static void dtv_play ( Base *base, const char *data )
{
	if ( GST_ELEMENT_CAST ( base->dtv->dvbplay )->current_state != GST_STATE_PLAYING )
	{
		base->dtv->checked_video = dtv_gst_checked_video ( data );

		dtv_gst_create_dvb_bin ( base->dtv->dvbplay, base->dtv->checked_video, base );

		dtv_gst_data_set ( base->dtv->dvbplay, data, base );

		gst_element_set_state ( base->dtv->dvbplay, GST_STATE_PLAYING );

		g_timeout_add ( 250, (GSourceFunc)dtv_update_win, base );

		time ( &base->dtv->t_start_tv );
	}
}

void dtv_stop_set_play ( Base *base, const char *data )
{
	if ( base->dtv->ch_data ) g_free ( base->dtv->ch_data );
	base->dtv->ch_data = g_strdup ( data );

	dtv_stop ( base );

	dtv_play ( base, data );
}

void dtv_gst_record ( Base *base )
{
	if ( GST_ELEMENT_CAST ( base->dtv->dvbplay )->current_state != GST_STATE_PLAYING ) return;

	if ( base->dtv->rec_tv )
	{
		GstElement *mpegtsmux = dtv_gst_ret_iterate_element ( base->dtv->dvbplay, "mpegtsmux", NULL );

		if ( mpegtsmux == NULL ) return;

		GstPad *blockpad = gst_element_get_static_pad ( mpegtsmux, "src" );

		gst_pad_add_probe ( blockpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
							dtv_gst_blockpad_probe, base, NULL );

		gst_object_unref ( blockpad );
	}
	else
	{
		if ( dtv_gst_create_rec ( base ) )
		{
			base->dtv->rec_tv = !base->dtv->rec_tv;
			base->dtv->rec_ses = TRUE;

			g_timeout_add ( 2500, (GSourceFunc)dtv_check_rec, base );
		}
		else
		{
			dtv_stop ( base );

			g_warning ( "Record failed!" );
		}
	}
}

static GstBusSyncReply dtv_gst_bus_sync_handler ( G_GNUC_UNUSED GstBus *bus, GstMessage *message, Base *base )
{
    if ( !gst_is_video_overlay_prepare_window_handle_message ( message ) ) return GST_BUS_PASS;

    if ( base->dtv->window_hid != 0 )
    {
        GstVideoOverlay *xoverlay = GST_VIDEO_OVERLAY ( GST_MESSAGE_SRC ( message ) );
        gst_video_overlay_set_window_handle ( xoverlay, base->dtv->window_hid );

    } else { g_warning ( "Should have obtained window_handle by now!" ); }

    gst_message_unref ( message );

    return GST_BUS_DROP;
}

static void dtv_gst_msg_err ( G_GNUC_UNUSED GstBus *bus, GstMessage *msg, Base *base )
{
    GError *err = NULL;
    char  *dbg = NULL;

    gst_message_parse_error ( msg, &err, &dbg );

    g_printerr ( "dtv_gst_msg_err: %s (%s)\n", err->message, (dbg) ? dbg : "no details" );

    base_message_dialog ( "", err->message, GTK_MESSAGE_ERROR, base->window );

    g_error_free ( err );
    g_free ( dbg );

    dtv_stop ( base );
}

static void dtv_gst_msg_all ( G_GNUC_UNUSED GstBus *bus, GstMessage *msg, Base *base )
{
	if ( base->app_quit ) return;

	const GstStructure *structure = gst_message_get_structure ( msg );

	if ( structure )
	{
		int signal, snr;
		gboolean hlook = FALSE;

		if (  gst_structure_get_int ( structure, "signal", &signal )  )
		{
			gst_structure_get_int     ( structure, "snr",  &snr   );
			gst_structure_get_boolean ( structure, "lock", &hlook );

			dtv_level_set_sgn_snr ( base, base->dtv->level_base, (signal * 100) / 0xffff, (snr * 100) / 0xffff, hlook );

			if ( !base->dtv->panel_quit && gtk_widget_get_visible ( GTK_WIDGET ( base->dtv->h_box_level_panel ) ) )
			{
				dtv_level_set_sgn_snr ( base, base->dtv->level_panel, (signal * 100) / 0xffff, (snr * 100) / 0xffff, hlook );
			}

			base->dtv->rec_pulse = !base->dtv->rec_pulse;

			if ( base->dtv->rec_pulse ) base->dtv->rec_status = !base->dtv->rec_status;
		}
	}

	mpegts_pmt_lang_section ( msg, base );
}

GstElement * dtv_gst_create ( Base *base )
{
    GstElement *dvbplay = gst_pipeline_new ( "pipeline" );

    if ( !dvbplay )
    {
        g_printerr ( "dvbplay - not created.\n" );

        return NULL;
    }

    base->dtv->count_audio_track = 0;
	base->dtv->set_audio_track   = 0;

	uint i; for ( i = 0; i < MAX_AUDIO; i++ ) base->dtv->audio_lang[i] = NULL;

    GstBus *bus = gst_element_get_bus ( dvbplay );

    gst_bus_add_signal_watch_full ( bus, G_PRIORITY_DEFAULT );
    gst_bus_set_sync_handler ( bus, (GstBusSyncHandler)dtv_gst_bus_sync_handler, base, NULL );

	g_signal_connect ( bus, "message",        G_CALLBACK ( dtv_gst_msg_all ), base );
    g_signal_connect ( bus, "message::error", G_CALLBACK ( dtv_gst_msg_err ), base );

	gst_object_unref (bus);

    return dvbplay;
}
