/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include <base.h>

#include "descr.h"
#include "mpegts.h"

#include "tree-view.h"
#include "dtv-level.h"
#include "pref.h"
#include "lang.h"

#include <stdlib.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>


char * helia_get_dvb_info ( Base *base, uint adapter, uint frontend );
uint helia_get_dvb_delsys ( uint adapter, uint frontend );
void helia_set_dvb_delsys ( uint adapter, uint frontend, uint delsys );

void scan_stop ( GtkButton *button, Base *base );


enum page_n
{
	PAGE_SC,
	PAGE_DT,
	PAGE_DS,
	PAGE_DC,
	PAGE_AT,
	PAGE_DM,
	PAGE_IT,
	PAGE_IS,
	PAGE_CH,
	PAGE_NUM
};

const struct HeliaScanLabel { uint page; const char *name; } scan_label_n[] =
{
	{ PAGE_SC, "Scanner"  },
	{ PAGE_DT, "DVB-T/T2" },
	{ PAGE_DS, "DVB-S/S2" },
	{ PAGE_DC, "DVB-C"    },
	{ PAGE_AT, "ATSC"     },
	{ PAGE_DM, "DTMB"     },
	{ PAGE_IT, "ISDB-T"   },
	{ PAGE_IS, "ISDB-S"   },
	{ PAGE_CH, "Channels" }
};

struct DvbDescrGstParam { const char *name; const char *dvb_v5_name; const char *gst_param; 
						  const DvbDescrAll *dvb_descr; uint cdsc; } gst_param_dvb_descr_n[] =
{
// descr
{ "Inversion",      "INVERSION",         "inversion",        dvb_descr_inversion_type_n,  G_N_ELEMENTS ( dvb_descr_inversion_type_n  ) },
{ "Code Rate HP",   "CODE_RATE_HP",      "code-rate-hp",     dvb_descr_coderate_type_n,   G_N_ELEMENTS ( dvb_descr_coderate_type_n   ) },
{ "Code Rate LP",   "CODE_RATE_LP",      "code-rate-lp",     dvb_descr_coderate_type_n,   G_N_ELEMENTS ( dvb_descr_coderate_type_n   ) },
{ "Inner Fec",      "INNER_FEC",         "code-rate-hp",     dvb_descr_coderate_type_n,   G_N_ELEMENTS ( dvb_descr_coderate_type_n   ) },
{ "Modulation",     "MODULATION",        "modulation",       dvb_descr_modulation_type_n, G_N_ELEMENTS ( dvb_descr_modulation_type_n ) },
{ "Transmission",   "TRANSMISSION_MODE", "trans-mode",       dvb_descr_transmode_type_n,  G_N_ELEMENTS ( dvb_descr_transmode_type_n  ) },
{ "Guard interval", "GUARD_INTERVAL",    "guard",            dvb_descr_guard_type_n,      G_N_ELEMENTS ( dvb_descr_guard_type_n 	 ) },
{ "Hierarchy",      "HIERARCHY",         "hierarchy",        dvb_descr_hierarchy_type_n,  G_N_ELEMENTS ( dvb_descr_hierarchy_type_n  ) },
{ "Pilot",          "PILOT",             "pilot",            dvb_descr_pilot_type_n,      G_N_ELEMENTS ( dvb_descr_pilot_type_n 	 ) },
{ "Rolloff",        "ROLLOFF",           "rolloff",          dvb_descr_roll_type_n,       G_N_ELEMENTS ( dvb_descr_roll_type_n 		 ) },
{ "Polarity",       "POLARIZATION",      "polarity",         dvb_descr_polarity_type_n,   G_N_ELEMENTS ( dvb_descr_polarity_type_n   ) },
{ "LNB",            "LNB",               "lnb-type",         dvb_descr_lnb_type_n,        G_N_ELEMENTS ( dvb_descr_lnb_type_n 		 ) },
{ "DiSEqC",         "SAT_NUMBER",        "diseqc-source",    dvb_descr_lnb_num_n,         G_N_ELEMENTS ( dvb_descr_lnb_num_n 		 ) },
{ "Interleaving",   "INTERLEAVING",      "interleaving",     dvb_descr_ileaving_type_n,   G_N_ELEMENTS ( dvb_descr_ileaving_type_n 	 ) },

// digits
{ "Frequency  MHz", 	"FREQUENCY",         "frequency",        NULL, 0 },
{ "Bandwidth  Hz",  	"BANDWIDTH_HZ",      "bandwidth-hz",     NULL, 0 },
{ "Symbol rate  kBd", 	"SYMBOL_RATE",       "symbol-rate",      NULL, 0 },
{ "Stream ID",      	"STREAM_ID",         "stream-id",        NULL, 0 },
{ "Service Id",     	"SERVICE_ID",        "program-number",   NULL, 0 },
{ "Audio Pid",      	"AUDIO_PID",         "audio-pid",        NULL, 0 },
{ "Video Pid",      	"VIDEO_PID",         "video-pid",        NULL, 0 },

// ISDB
{ "Layer enabled",      "ISDBT_LAYER_ENABLED",            "isdbt-layer-enabled",            NULL, 0 },
{ "Partial",            "ISDBT_PARTIAL_RECEPTION",        "isdbt-partial-reception",        NULL, 0 },
{ "Sound",              "ISDBT_SOUND_BROADCASTING",       "isdbt-sound-broadcasting",       NULL, 0 },
{ "Subchannel  SB",     "ISDBT_SB_SUBCHANNEL_ID",         "isdbt-sb-subchannel-id",         NULL, 0 },
{ "Segment idx  SB",    "ISDBT_SB_SEGMENT_IDX",           "isdbt-sb-segment-idx",           NULL, 0 },
{ "Segment count  SB",  "ISDBT_SB_SEGMENT_COUNT",         "isdbt-sb-segment-count",         NULL, 0 },
{ "Inner Fec  LA",      "ISDBT_LAYERA_FEC",               "isdbt-layera-fec",               dvb_descr_coderate_type_n,   G_N_ELEMENTS ( dvb_descr_coderate_type_n   ) },
{ "Modulation  LA",     "ISDBT_LAYERA_MODULATION",        "isdbt-layera-modulation",        dvb_descr_modulation_type_n, G_N_ELEMENTS ( dvb_descr_modulation_type_n ) },
{ "Segment count  LA",  "ISDBT_LAYERA_SEGMENT_COUNT",     "isdbt-layera-segment-count",     NULL, 0  },
{ "Interleaving  LA",   "ISDBT_LAYERA_TIME_INTERLEAVING", "isdbt-layera-time-interleaving", NULL, 0  },
{ "Inner Fec  LB",      "ISDBT_LAYERB_FEC",               "isdbt-layerb-fec",               dvb_descr_coderate_type_n,   G_N_ELEMENTS ( dvb_descr_coderate_type_n   ) },
{ "Modulation  LB",     "ISDBT_LAYERB_MODULATION",        "isdbt-layerb-modulation",        dvb_descr_modulation_type_n, G_N_ELEMENTS ( dvb_descr_modulation_type_n ) },
{ "Segment count  LB",  "ISDBT_LAYERB_SEGMENT_COUNT",     "isdbt-layerb-segment-count",     NULL, 0  },
{ "Interleaving  LB",   "ISDBT_LAYERB_TIME_INTERLEAVING", "isdbt-layerb-time-interleaving", NULL, 0  },
{ "Inner Fec  LC",      "ISDBT_LAYERC_FEC",               "isdbt-layerc-fec",               dvb_descr_coderate_type_n,   G_N_ELEMENTS ( dvb_descr_coderate_type_n   ) },
{ "Modulation  LC",     "ISDBT_LAYERC_MODULATION",        "isdbt-layerc-modulation",        dvb_descr_modulation_type_n, G_N_ELEMENTS ( dvb_descr_modulation_type_n ) },
{ "Segment count  LC",  "ISDBT_LAYERC_SEGMENT_COUNT",     "isdbt-layerc-segment-count",     NULL, 0  },
{ "Interleaving  LC",   "ISDBT_LAYERC_TIME_INTERLEAVING", "isdbt-layerc-time-interleaving", NULL, 0  }
};


void set_lnb_low_high_switch ( GstElement *element, int type_lnb )
{
	g_object_set ( element, "lnb-lof1", lnb_type_lhs_n[type_lnb].low_val,    NULL );
	g_object_set ( element, "lnb-lof2", lnb_type_lhs_n[type_lnb].high_val,   NULL );
	g_object_set ( element, "lnb-slof", lnb_type_lhs_n[type_lnb].switch_val, NULL );
}


const char * scan_get_info ( const char *data )
{
    const char *res = NULL;

    uint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( gst_param_dvb_descr_n ); c++ )
	{	
		if ( g_str_has_suffix ( data, gst_param_dvb_descr_n[c].gst_param ) )
		{
			res = gst_param_dvb_descr_n[c].name;

			break;
		}
	}

    return res;
}
const char * scan_get_info_descr_vis ( const char *data, int num )
{
    const char *res = NULL;

    uint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( gst_param_dvb_descr_n ); c++ )
	{
		if ( gst_param_dvb_descr_n[c].cdsc == 0 ) continue;

		if ( g_str_has_suffix ( data, gst_param_dvb_descr_n[c].gst_param ) )
		{
			if ( g_str_has_prefix ( data, "diseqc-source" ) ) num += 1;

			res = gst_param_dvb_descr_n[c].dvb_descr[num].text_vis;

			break;
		}
	}

    return res;
}


static void scan_msg_all ( G_GNUC_UNUSED GstBus *bus, GstMessage *message, Base *base )
{
	if ( base->dtv->scan.scan_quit ) return;

	const GstStructure *structure = gst_message_get_structure ( message );

	if ( structure )
	{
		int signal, snr;
		gboolean hlook = FALSE;

		if (  gst_structure_get_int ( structure, "signal", &signal )  )
		{
			gst_structure_get_boolean ( structure, "lock", &hlook );
			gst_structure_get_int ( structure, "snr", &snr);

			dtv_level_set_sgn_snr ( base, base->dtv->scan.level_scan, (signal * 100) / 0xffff, (snr * 100) / 0xffff, hlook );
		}
	}

	mpegts_parse_section ( message, base );
}

static void scan_msg_err ( G_GNUC_UNUSED GstBus *bus, GstMessage *msg, Base *base )
{
	GError *err = NULL;
	char  *dbg = NULL;

	gst_message_parse_error ( msg, &err, &dbg );

	g_critical ( "scan_msg_err:: %s (%s)\n", err->message, (dbg) ? dbg : "no details" );

	base_message_dialog ( "", err->message, GTK_MESSAGE_ERROR, base->window );

	g_error_free ( err );
	g_free ( dbg );

	scan_stop ( NULL, base );
}

static void scan_set_tune_timeout ( GstElement *element, guint64 time_set )
{
	guint64 timeout = 0, timeout_set = 0, timeout_get = 0, timeout_def = 10000000000;

	g_object_get ( element, "tuning-timeout", &timeout, NULL );

	timeout_set = timeout_def / 10 * time_set;

	g_object_set ( element, "tuning-timeout", (guint64)timeout_set, NULL );

	g_object_get ( element, "tuning-timeout", &timeout_get, NULL );

	g_debug ( "scan_set_tune_timeout: timeout %ld | timeout set %ld", timeout, timeout_get );
}
static void scan_set_tune_def ( GstElement *element )
{
	g_object_set ( element, "bandwidth-hz", 8000000,  NULL );
	g_object_set ( element, "modulation",   QAM_AUTO, NULL );
}

void scan_gst_create ( Base *base )
{
	mpegts_initialize ();

	mpegts_clear ( base );

	base->dtv->scan.adapter_set  = 0;
	base->dtv->scan.frontend_set = 0;
	base->dtv->scan.delsys_set   = helia_get_dvb_delsys ( base->dtv->scan.adapter_set, base->dtv->scan.frontend_set );

	base->dtv->scan.lnb_type = 0;

	GstElement *scan_tsparse, *scan_filesink;

	base->dtv->scan.pipeline_scan = gst_pipeline_new ( "pipeline-scan" );
	base->dtv->scan.scan_dvbsrc   = gst_element_factory_make ( "dvbsrc",   NULL );
	scan_tsparse 		 = gst_element_factory_make ( "tsparse",  NULL );
	scan_filesink		 = gst_element_factory_make ( "filesink", NULL );

	if ( !base->dtv->scan.pipeline_scan || !base->dtv->scan.scan_dvbsrc || !scan_tsparse || !scan_filesink )
		g_critical ( "scan_gst_create:: pipeline_scan - not be created.\n" );

	gst_bin_add_many ( GST_BIN ( base->dtv->scan.pipeline_scan ), base->dtv->scan.scan_dvbsrc, scan_tsparse, scan_filesink, NULL );
	gst_element_link_many ( base->dtv->scan.scan_dvbsrc, scan_tsparse, scan_filesink, NULL );

	g_object_set ( scan_filesink, "location", "/dev/null", NULL);

	GstBus *bus_scan = gst_element_get_bus ( base->dtv->scan.pipeline_scan );
	gst_bus_add_signal_watch ( bus_scan );

	g_signal_connect ( bus_scan, "message",          G_CALLBACK ( scan_msg_all ), base );
	g_signal_connect ( bus_scan, "message::error",   G_CALLBACK ( scan_msg_err ), base );

	gst_object_unref ( bus_scan );

	scan_set_tune_timeout ( base->dtv->scan.scan_dvbsrc, 5 );
	scan_set_tune_def ( base->dtv->scan.scan_dvbsrc );
}

static void scan_changed_spin_get_freq ( GtkSpinButton *button, GtkLabel *label )
{
	gtk_spin_button_update ( button );

	long num = gtk_spin_button_get_value  ( button );

	const char *name = gtk_widget_get_name ( GTK_WIDGET ( label ) );

	if ( g_str_has_prefix ( name, "DVB-S" ) || g_str_has_prefix ( name, "ISDB-S" ) )
	{
		if ( num < 30000 )
		{
			gtk_label_set_text ( label, "Frequency  MHz" );
		}
		else
			gtk_label_set_text ( label, "Frequency  KHz" );
	}
	else
	{
		if ( num < 1000 )
		{
			gtk_label_set_text ( label, "Frequency  MHz" );
		}
		else if ( num < 1000000 )
		{
			gtk_label_set_text ( label, "Frequency  KHz" );
		}
	}

	g_debug ( "num = %ld | name %s ", num, name );
}

static void scan_changed_spin_all ( GtkSpinButton *button, Base *base )
{
	gtk_spin_button_update ( button );

	long num = gtk_spin_button_get_value  ( button );
	const char *name = gtk_widget_get_name ( GTK_WIDGET ( button ) );

	if ( g_str_has_prefix ( name, "Frequency" ) )
	{
		if ( base->dtv->scan.delsys_set == SYS_DVBS || base->dtv->scan.delsys_set == SYS_DVBS2 || base->dtv->scan.delsys_set == SYS_ISDBS )
		{
			if ( num < 30000 ) num *= 1000;
		}
		else
		{
			if ( num < 1000 )
			{
				num *= 1000000;
			}
			else if ( num < 1000000 )
			{
				num *= 1000;
			}
		}

		g_object_set ( base->dtv->scan.scan_dvbsrc, "frequency", num, NULL );

		g_debug ( "name = %s | num = %ld | gst_param = %s", name, num, "frequency" );

		return;
	}

	uint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( gst_param_dvb_descr_n ); c++ )
	{
		if ( g_str_has_suffix ( name, gst_param_dvb_descr_n[c].name ) )
		{
			g_object_set ( base->dtv->scan.scan_dvbsrc, gst_param_dvb_descr_n[c].gst_param, num, NULL );

			g_debug ( "name = %s | num = %ld | gst_param = %s", name, 
					num, gst_param_dvb_descr_n[c].gst_param );
		}
	}
}
static void scan_changed_combo_all ( GtkComboBox *combo_box, Base *base )
{
	uint num = gtk_combo_box_get_active ( combo_box );
	const char *name = gtk_widget_get_name ( GTK_WIDGET ( combo_box ) );

	if ( g_str_has_prefix ( name, "LNB" ) )
	{
		base->dtv->scan.lnb_type = num;

		set_lnb_low_high_switch ( base->dtv->scan.scan_dvbsrc, num );

		g_debug ( "name %s | set %s: %d ( low %ld | high %ld | switch %ld )", name, lnb_type_lhs_n[num].name, num, 
				  lnb_type_lhs_n[num].low_val, lnb_type_lhs_n[num].high_val, lnb_type_lhs_n[num].switch_val );

		return;
	}

	uint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( gst_param_dvb_descr_n ); c++ )
	{
		if ( g_str_has_suffix ( name, gst_param_dvb_descr_n[c].name ) )
		{
			if ( g_str_has_prefix ( name, "Polarity" ) )
			{
				if ( num == 1 || num == 3 )
					g_object_set ( base->dtv->scan.scan_dvbsrc, "polarity", "V", NULL );
				else
					g_object_set ( base->dtv->scan.scan_dvbsrc, "polarity", "H", NULL );
			}
			else
				g_object_set ( base->dtv->scan.scan_dvbsrc, gst_param_dvb_descr_n[c].gst_param, 
					gst_param_dvb_descr_n[c].dvb_descr[num].descr_num, NULL );

			g_debug ( "name = %s | num = %d | gst_param = %s | descr_text_vis = %s | descr_num = %d", 
					name, num, gst_param_dvb_descr_n[c].gst_param, 
					gst_param_dvb_descr_n[c].dvb_descr[num].text_vis, 
					gst_param_dvb_descr_n[c].dvb_descr[num].descr_num );
		}
	}
}

static GtkBox * scan_dvb_all  ( Base *base, uint num, const DvbTypes *dvball, const char *type )
{
	g_debug ( "scan_dvb_all:: %s", type );

	GtkBox *g_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( g_box ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( g_box ), 10 );

	GtkGrid *grid = (GtkGrid *)gtk_grid_new();
	gtk_grid_set_column_homogeneous ( GTK_GRID ( grid ), TRUE );
	gtk_box_pack_start ( g_box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );

	GtkLabel *label;
	GtkSpinButton *spinbutton;
	GtkComboBoxText *scan_combo_box;

	gboolean freq = FALSE;
	int d_data = 0, set_freq = 1000000;
	uint d = 0, c = 0, z = 0;

	if ( g_str_has_prefix ( type, "DVB-S" ) || g_str_has_prefix ( type, "ISDB-S" ) ) set_freq = 1000;

	for ( d = 0; d < num; d++ )
	{
		label = (GtkLabel *)gtk_label_new ( dvball[d].param );
		gtk_widget_set_name ( GTK_WIDGET ( label ), type );
		gtk_widget_set_halign ( GTK_WIDGET ( label ), GTK_ALIGN_START );
		gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( label ), 0, d, 1, 1 );

		if ( !dvball[d].descr )
		{
			if ( g_str_has_prefix ( dvball[d].param, "Frequency" ) ) freq = TRUE; else freq = FALSE;

			spinbutton = (GtkSpinButton *) gtk_spin_button_new_with_range ( dvball[d].min, dvball[d].max, 1 );
			gtk_widget_set_name ( GTK_WIDGET ( spinbutton ), dvball[d].param );
			gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( spinbutton ), 1, d, 1, 1 );

			for ( c = 0; c < G_N_ELEMENTS ( gst_param_dvb_descr_n ); c++ )
			{
				if ( g_str_has_suffix ( dvball[d].param, gst_param_dvb_descr_n[c].name ) )
				{
					g_object_get ( base->dtv->scan.scan_dvbsrc, gst_param_dvb_descr_n[c].gst_param, &d_data, NULL );
					gtk_spin_button_set_value ( spinbutton, freq ? d_data / set_freq : d_data );
				}
			}

			if ( freq ) g_signal_connect ( spinbutton, "changed", G_CALLBACK ( scan_changed_spin_get_freq ), label );

			g_signal_connect ( spinbutton, "changed", G_CALLBACK ( scan_changed_spin_all ), base );
		}
		else
		{
			scan_combo_box = (GtkComboBoxText *) gtk_combo_box_text_new ();
			gtk_widget_set_name ( GTK_WIDGET ( scan_combo_box ), dvball[d].param );
			gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( scan_combo_box ), 1, d, 1, 1 );

			for ( c = 0; c < G_N_ELEMENTS ( gst_param_dvb_descr_n ); c++ )
			{
				if ( g_str_has_suffix ( dvball[d].param, gst_param_dvb_descr_n[c].name ) )
				{
					for ( z = 0; z < gst_param_dvb_descr_n[c].cdsc; z++ )
						gtk_combo_box_text_append_text ( scan_combo_box, gst_param_dvb_descr_n[c].dvb_descr[z].text_vis );

					if ( g_str_has_prefix ( dvball[d].param, "Polarity" ) )
					{
						char *pol = NULL;

						g_object_get ( base->dtv->scan.scan_dvbsrc, gst_param_dvb_descr_n[c].gst_param, &pol, NULL );

						if ( g_str_has_prefix ( pol, "V" ) || g_str_has_prefix ( pol, "v" ) )
							gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), 1 );
						else
							gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), 0 );

						g_free ( pol );

						continue;
					}

					if ( g_str_has_prefix ( dvball[d].param, "LNB" ) )
						d_data = base->dtv->scan.lnb_type;
					else
						g_object_get ( base->dtv->scan.scan_dvbsrc, gst_param_dvb_descr_n[c].gst_param, &d_data, NULL );

					if ( g_str_has_prefix ( dvball[d].param, "DiSEqC" ) ) d_data += 1;

					gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), d_data );
				}
			}

			if ( gtk_combo_box_get_active ( GTK_COMBO_BOX ( scan_combo_box ) ) == -1 )
				gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), 0 );

			g_signal_connect ( scan_combo_box, "changed", G_CALLBACK ( scan_changed_combo_all ), base );
		}
	}

	return g_box;
}
static GtkBox * scan_isdb_all  ( Base *base, uint num, const DvbTypes *dvball, const char *type )
{
	g_debug ( "scan_isdb_all:: %s", type );

	GtkBox *g_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( g_box ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( g_box ), 10 );

	GtkGrid *grid = NULL;
	GtkNotebook *notebook = NULL;

	if ( g_str_has_prefix ( type, "ISDB-S" ) )
	{
		grid = (GtkGrid *)gtk_grid_new();
		gtk_grid_set_column_homogeneous ( GTK_GRID ( grid ), TRUE );
		gtk_box_pack_start ( g_box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );
	}

	if ( g_str_has_prefix ( type, "ISDB-T" ) )
	{
		notebook = (GtkNotebook *)gtk_notebook_new ();
		gtk_notebook_set_tab_pos ( notebook, GTK_POS_TOP );
		gtk_notebook_set_scrollable ( notebook, TRUE );
	}

	struct NameLabel { const char *name; const char *label; } nl_n[] =
	{
		{ "Frequency",      "Frequency" },
		{ "Layer enabled",  "Layer"     },
		{ "Subchannel  SB", "SB"        },
		{ "Inner Fec  LA",  "Layer A"   },
		{ "Inner Fec  LB",  "Layer B"   },
		{ "Inner Fec  LC",  "Layer C"   }
	};

	GtkLabel *label;
	GtkSpinButton *spinbutton;
	GtkComboBoxText *scan_combo_box;

	gboolean freq = FALSE;
	int d_data = 0, set_freq = 1000000;
	uint d = 0, c = 0, z = 0, q = 0;

	if ( g_str_has_prefix ( type, "DVB-S" ) || g_str_has_prefix ( type, "ISDB-S" ) ) set_freq = 1000;

	for ( d = 0; d < num; d++ )
	{
		if ( g_str_has_prefix ( type, "ISDB-T" ) )
		{
			if ( g_str_has_suffix ( dvball[d].param, "Inner Fec  LA" ) )
			{
				gtk_box_pack_start ( g_box, GTK_WIDGET ( notebook ), TRUE, TRUE, 10 );

				notebook = (GtkNotebook *)gtk_notebook_new ();
				gtk_notebook_set_tab_pos ( notebook, GTK_POS_TOP );
				gtk_notebook_set_scrollable ( notebook, TRUE );
			}

			for ( q = 0; q < G_N_ELEMENTS ( nl_n ); q++ )
			{
				if ( g_str_has_prefix ( dvball[d].param, nl_n[q].name ) )
				{
					grid = (GtkGrid *)gtk_grid_new();
					gtk_grid_set_column_homogeneous ( GTK_GRID ( grid ), TRUE );
					gtk_notebook_append_page ( notebook, GTK_WIDGET ( grid ),  gtk_label_new ( nl_n[q].label ) );				  
				}
			}
		}

		label = (GtkLabel *)gtk_label_new ( dvball[d].param );
		gtk_widget_set_name ( GTK_WIDGET ( label ), type );
		gtk_widget_set_halign ( GTK_WIDGET ( label ), GTK_ALIGN_START );
		gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( label ), 0, d, 1, 1 );

		if ( !dvball[d].descr )
		{
			if ( g_str_has_prefix ( dvball[d].param, "Frequency" ) ) freq = TRUE; else freq = FALSE;

			spinbutton = (GtkSpinButton *) gtk_spin_button_new_with_range ( dvball[d].min, dvball[d].max, 1 );
			gtk_widget_set_name ( GTK_WIDGET ( spinbutton ), dvball[d].param );
			gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( spinbutton ), 1, d, 1, 1 );	

			for ( c = 0; c < G_N_ELEMENTS ( gst_param_dvb_descr_n ); c++ )
			{
				if ( g_str_has_suffix ( dvball[d].param, gst_param_dvb_descr_n[c].name ) )
				{
					g_object_get ( base->dtv->scan.scan_dvbsrc, gst_param_dvb_descr_n[c].gst_param, &d_data, NULL );
					gtk_spin_button_set_value ( spinbutton, freq ? d_data / set_freq : d_data );
				}
			}

			if ( freq ) g_signal_connect ( spinbutton, "changed", G_CALLBACK ( scan_changed_spin_get_freq ), label );

			g_signal_connect ( spinbutton, "changed", G_CALLBACK ( scan_changed_spin_all ), base );
		}
		else
		{
			scan_combo_box = (GtkComboBoxText *) gtk_combo_box_text_new ();
			gtk_widget_set_name ( GTK_WIDGET ( scan_combo_box ), dvball[d].param );
			gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( scan_combo_box ), 1, d, 1, 1 );

			for ( c = 0; c < G_N_ELEMENTS ( gst_param_dvb_descr_n ); c++ )
			{
				if ( g_str_has_suffix ( dvball[d].param, gst_param_dvb_descr_n[c].name ) )
				{
					for ( z = 0; z < gst_param_dvb_descr_n[c].cdsc; z++ )
						gtk_combo_box_text_append_text ( scan_combo_box, gst_param_dvb_descr_n[c].dvb_descr[z].text_vis );

					if ( g_str_has_prefix ( dvball[d].param, "LNB" ) )
						d_data = base->dtv->scan.lnb_type;
					else
						g_object_get ( base->dtv->scan.scan_dvbsrc, gst_param_dvb_descr_n[c].gst_param, &d_data, NULL );

					if ( g_str_has_prefix ( dvball[d].param, "DiSEqC" ) ) d_data += 1;

					gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), d_data );
				}
			}

			if ( gtk_combo_box_get_active ( GTK_COMBO_BOX ( scan_combo_box ) ) == -1 )
				gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), 0 );

			g_signal_connect ( scan_combo_box, "changed", G_CALLBACK ( scan_changed_combo_all ), base );
		}
	}

	if ( g_str_has_prefix ( type, "ISDB-T" ) )
		gtk_box_pack_start ( g_box, GTK_WIDGET ( notebook ), TRUE, TRUE, 10 );

	return g_box;
}

static void scan_set_all_ch ( Base *base,  gboolean clear )
{
	if ( clear )
	{
		gtk_label_set_text ( base->dtv->scan.all_channels, "⅀" );
	}
	else
	{
		uint c = 0, c_tv = 0, c_ro = 0;

		for ( c = 0; c < base->dtv->scan.mpegts.pmt_count; c++ )
		{
			if ( base->dtv->scan.mpegts.ppsv[c].pmt_vpid > 0 ) c_tv++;
			if ( base->dtv->scan.mpegts.ppsv[c].pmt_apid > 0 && base->dtv->scan.mpegts.ppsv[c].pmt_vpid == 0 ) c_ro++;
		}

		char *text = g_strdup_printf ( "⅀: %d -- 📺: %d -- 📻: %d", c, c_tv, c_ro );

		gtk_label_set_text ( base->dtv->scan.all_channels, text );

		g_free ( text );
	}
}

static void treeview_append_base ( GtkTreeView *tree_view, const char *name, const char *data )
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model ( tree_view );
    uint ind = gtk_tree_model_iter_n_children ( model, NULL );

    gtk_list_store_append ( GTK_LIST_STORE ( model ), &iter);
    gtk_list_store_set    ( GTK_LIST_STORE ( model ), &iter,
                            COL_NUM, ind+1,
                            COL_FL_CH, name,
                            COL_DATA,  data,
                            -1 );	
}

static void scan_get_tp_data ( Base *base, GString *gstring )
{
	uint c = 0, d = 0;
	int  d_data = 0, DVBTYPE = 0;

	g_object_get ( base->dtv->scan.scan_dvbsrc, "delsys", &DVBTYPE, NULL );

	g_debug ( "helia_scan_get_tp_data: num %d | Dvb type - %s | Manual %d", 
		DVBTYPE, dvb_descr_delsys_type_n[DVBTYPE].text_vis, base->dtv->scan.delsys_set );

	DVBTYPE = base->dtv->scan.delsys_set;

	const char *dvb_f[] = { "delsys", "adapter", "frontend" };

	for ( d = 0; d < G_N_ELEMENTS ( dvb_f ); d++ )
	{
		g_object_get ( base->dtv->scan.scan_dvbsrc, dvb_f[d], &d_data, NULL );
		g_string_append_printf ( gstring, ":%s=%d", dvb_f[d], d_data );
	}

	if ( DVBTYPE == SYS_DVBT || DVBTYPE == SYS_DVBT2 )
	{
		for ( c = 0; c < G_N_ELEMENTS ( dvbt_props_n ); c++ )
		{
			if ( DVBTYPE == SYS_DVBT )
				if ( g_str_has_prefix ( dvbt_props_n[c].param, "Stream ID" ) ) continue;

			for ( d = 0; d < G_N_ELEMENTS ( gst_param_dvb_descr_n ); d++ )
			{
				if ( g_str_has_suffix ( dvbt_props_n[c].param, gst_param_dvb_descr_n[d].name ) )
				{
					g_object_get ( base->dtv->scan.scan_dvbsrc, gst_param_dvb_descr_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", gst_param_dvb_descr_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_DTMB )
	{
		for ( c = 0; c < G_N_ELEMENTS ( dtmb_props_n ); c++ )
		{
			for ( d = 0; d < G_N_ELEMENTS ( gst_param_dvb_descr_n ); d++ )
			{
				if ( g_str_has_suffix ( dtmb_props_n[c].param, gst_param_dvb_descr_n[d].name ) )
				{
					g_object_get ( base->dtv->scan.scan_dvbsrc, gst_param_dvb_descr_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", gst_param_dvb_descr_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_DVBC_ANNEX_A || DVBTYPE == SYS_DVBC_ANNEX_C )
	{
		for ( c = 0; c < G_N_ELEMENTS ( dvbc_props_n ); c++ )
		{
			for ( d = 0; d < G_N_ELEMENTS ( gst_param_dvb_descr_n ); d++ )
			{
				if ( g_str_has_suffix ( dvbc_props_n[c].param, gst_param_dvb_descr_n[d].name ) )
				{
					g_object_get ( base->dtv->scan.scan_dvbsrc, gst_param_dvb_descr_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", gst_param_dvb_descr_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_ATSC || DVBTYPE == SYS_DVBC_ANNEX_B )
	{
		for ( c = 0; c < G_N_ELEMENTS ( atsc_props_n ); c++ )
		{
			for ( d = 0; d < G_N_ELEMENTS ( gst_param_dvb_descr_n ); d++ )
			{
				if ( g_str_has_suffix ( atsc_props_n[c].param, gst_param_dvb_descr_n[d].name ) )
				{
					g_object_get ( base->dtv->scan.scan_dvbsrc, gst_param_dvb_descr_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", gst_param_dvb_descr_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_DVBS || DVBTYPE == SYS_TURBO || DVBTYPE == SYS_DVBS2 )
	{
		for ( c = 0; c < G_N_ELEMENTS ( dvbs_props_n ); c++ )
		{
			if ( DVBTYPE == SYS_TURBO )
				if ( g_str_has_prefix ( dvbs_props_n[c].param, "Pilot" ) || g_str_has_prefix ( dvbs_props_n[c].param, "Rolloff" ) || g_str_has_prefix ( dvbs_props_n[c].param, "Stream ID" ) ) continue;

			if ( DVBTYPE == SYS_DVBS )
				if ( g_str_has_prefix ( dvbs_props_n[c].param, "Modulation" ) || g_str_has_prefix ( dvbs_props_n[c].param, "Pilot" ) || g_str_has_prefix ( dvbs_props_n[c].param, "Rolloff" ) || g_str_has_prefix ( dvbs_props_n[c].param, "Stream ID" ) ) continue;

			for ( d = 0; d < G_N_ELEMENTS ( gst_param_dvb_descr_n ); d++ )
			{
				if ( g_str_has_suffix ( dvbs_props_n[c].param, gst_param_dvb_descr_n[d].name ) )
				{
					if ( g_str_has_prefix ( "polarity", gst_param_dvb_descr_n[d].gst_param ) )
					{
						char *pol = NULL;

						g_object_get ( base->dtv->scan.scan_dvbsrc, gst_param_dvb_descr_n[d].gst_param, &pol, NULL );

						g_string_append_printf ( gstring, ":polarity=%s", pol );

						g_free ( pol );

						continue;
					}

					if ( g_str_has_prefix ( "lnb-type", gst_param_dvb_descr_n[d].gst_param ) )
					{
						g_string_append_printf ( gstring, ":%s=%d", "lnb-type", base->dtv->scan.lnb_type );
						continue;
					}

					g_object_get ( base->dtv->scan.scan_dvbsrc, gst_param_dvb_descr_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", gst_param_dvb_descr_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_ISDBT )
	{
		for ( c = 0; c < G_N_ELEMENTS ( isdbt_props_n ); c++ )
		{
			for ( d = 0; d < G_N_ELEMENTS ( gst_param_dvb_descr_n ); d++ )
			{
				if ( g_str_has_suffix ( isdbt_props_n[c].param, gst_param_dvb_descr_n[d].name ) )
				{
					g_object_get ( base->dtv->scan.scan_dvbsrc, gst_param_dvb_descr_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", gst_param_dvb_descr_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_ISDBS )
	{
		for ( c = 0; c < G_N_ELEMENTS ( isdbs_props_n ); c++ )
		{
			for ( d = 0; d < G_N_ELEMENTS ( gst_param_dvb_descr_n ); d++ )
			{
				if ( g_str_has_suffix ( isdbs_props_n[c].param, gst_param_dvb_descr_n[d].name ) )
				{
					g_object_get ( base->dtv->scan.scan_dvbsrc, gst_param_dvb_descr_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", gst_param_dvb_descr_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}	
}



static char * strip_ch_name ( char *name )
{
	uint i = 0;
	for ( i = 0; name[i] != '\0'; i++ )
	{
		if ( name[i] == ':' ) name[i] = ' ';
	}
	return g_strstrip ( name );
}

void scan_read_ch_to_treeview ( Base *base )
{
	if ( base->dtv->scan.mpegts.pmt_count == 0 ) return;

	GString *gstr_data = g_string_new ( NULL );

	scan_get_tp_data ( base, gstr_data );

	g_debug ( "%s ", gstr_data->str );

	uint i = 0, c = 0, sdt_vct_count = base->dtv->scan.mpegts.pat_count;

	if ( base->dtv->scan.mpegts.sdt_count ) sdt_vct_count = base->dtv->scan.mpegts.sdt_count;
	if ( base->dtv->scan.mpegts.vct_count ) sdt_vct_count = base->dtv->scan.mpegts.vct_count;

	for ( i = 0; i < base->dtv->scan.mpegts.pmt_count; i++ )
	{
		char *ch_name = NULL;

		for ( c = 0; c < sdt_vct_count; c++ )
		{
			if ( base->dtv->scan.mpegts.ppsv[i].pmt_sid == base->dtv->scan.mpegts.ppsv[c].sdt_sid )
				{ ch_name = base->dtv->scan.mpegts.ppsv[c].sdt_name; break; }

			if ( base->dtv->scan.mpegts.ppsv[i].pmt_sid == base->dtv->scan.mpegts.ppsv[c].vct_sid )
				{ ch_name = base->dtv->scan.mpegts.ppsv[c].vct_name; break; }
		}

		if ( ch_name )
			ch_name = strip_ch_name ( ch_name );
		else
			ch_name = g_strdup_printf ( "Program-%d", base->dtv->scan.mpegts.ppsv[i].pmt_sid );

		GString *gstring = g_string_new ( ch_name );

		g_string_append_printf ( gstring, ":program-number=%d:video-pid=%d:audio-pid=%d", 
								 base->dtv->scan.mpegts.ppsv[i].pmt_sid, 
								 base->dtv->scan.mpegts.ppsv[i].pmt_vpid,
								 base->dtv->scan.mpegts.ppsv[i].pmt_apid );

		g_string_append_printf ( gstring, "%s", gstr_data->str );

		if ( !base->dtv->scan.scan_quit && base->dtv->scan.mpegts.ppsv[i].pmt_apid != 0 ) // ignore other
			treeview_append_base ( base->dtv->scan.treeview_scan, ch_name, gstring->str );

		g_print ( "%s \n", ch_name );
		g_debug ( "%s", gstring->str );

		g_free ( ch_name );
		g_string_free ( gstring, TRUE );
	}

	g_string_free ( gstr_data, TRUE );

	if ( base->dtv->scan.scan_quit ) return;

	scan_set_all_ch ( base, FALSE );

	gst_element_set_state ( base->dtv->scan.pipeline_scan, GST_STATE_NULL );
	dtv_level_set_sgn_snr ( base, base->dtv->scan.level_scan, 0, 0, FALSE );

	mpegts_clear ( base );
}



static void scan_channels_save ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( base->dtv->scan.treeview_scan );

	gboolean valid;
	for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
		  valid = gtk_tree_model_iter_next ( model, &iter ) )
	{
		char *name, *data;

		gtk_tree_model_get ( model, &iter, COL_DATA,  &data, -1 );
		gtk_tree_model_get ( model, &iter, COL_FL_CH, &name, -1 );

			treeview_append_base ( base->dtv->treeview, name, data );

		g_free ( name );
		g_free ( data );
	}
	
}
static void scan_channels_clear ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	GtkTreeModel *model = gtk_tree_view_get_model ( base->dtv->scan.treeview_scan );
	gtk_list_store_clear ( GTK_LIST_STORE ( model ) );

	scan_set_all_ch ( base, TRUE );
}
static GtkBox * scan_channels ( Base *base )
{
	GtkBox *g_box  = (GtkBox *)gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin_start ( GTK_WIDGET ( g_box ), 0 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( g_box ), 0 );

	base->dtv->scan.all_channels = (GtkLabel *)gtk_label_new ( "⅀" );
	gtk_widget_set_halign ( GTK_WIDGET ( base->dtv->scan.all_channels ), GTK_ALIGN_START );

	base->dtv->scan.treeview_scan = create_treeview ( base, lang_set ( base, "Channels" ), FALSE );

	GtkScrolledWindow *scroll_win = create_scroll_win ( base->dtv->scan.treeview_scan, 220 );

	gtk_box_pack_start ( g_box, GTK_WIDGET ( scroll_win ), TRUE, TRUE, 0 );

	GtkBox *lh_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_pack_start ( lh_box, GTK_WIDGET ( base->dtv->scan.all_channels ),  FALSE, FALSE, 10 );
	gtk_box_pack_start ( g_box,  GTK_WIDGET ( lh_box ),  FALSE, FALSE, 10 );

	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( h_box ), 5 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( h_box ), 5 );
	gtk_box_set_spacing ( h_box, 5 );

	base_create_image_button ( h_box, "helia-clear", 16, scan_channels_clear, base );
	base_create_image_button ( h_box, "helia-save",  16, scan_channels_save,  base );

	gtk_box_pack_start ( g_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );

	return g_box;
}

static void scan_set_label_device ( Base *base, GtkLabel *label, uint adapter, uint frontend )
{
	char *dvb_name = helia_get_dvb_info ( base, adapter, frontend );

	gtk_label_set_text ( label, dvb_name );

	g_free ( dvb_name );
}

static void scan_set_adapter ( GtkSpinButton *button, Base *base )
{
	gtk_spin_button_update ( button );

	base->dtv->scan.adapter_set = gtk_spin_button_get_value_as_int ( button );

	g_object_set ( base->dtv->scan.scan_dvbsrc, "adapter",  base->dtv->scan.adapter_set,  NULL );

	scan_set_label_device ( base, base->dtv->scan.dvb_device, base->dtv->scan.adapter_set, base->dtv->scan.frontend_set );

	base->dtv->scan.delsys_set = helia_get_dvb_delsys ( base->dtv->scan.adapter_set, base->dtv->scan.frontend_set );

	g_signal_handler_block   ( base->dtv->scan.combo_delsys, base->dtv->scan.desys_signal_id );

		gtk_combo_box_set_active ( GTK_COMBO_BOX ( base->dtv->scan.combo_delsys ), base->dtv->scan.delsys_set );

	g_signal_handler_unblock   ( base->dtv->scan.combo_delsys, base->dtv->scan.desys_signal_id );

	g_debug ( "scan_set_adapter" );
}
static void scan_set_frontend ( GtkSpinButton *button, Base *base )
{
	gtk_spin_button_update ( button );

	base->dtv->scan.frontend_set = gtk_spin_button_get_value_as_int ( button );

	g_object_set ( base->dtv->scan.scan_dvbsrc, "frontend",  base->dtv->scan.frontend_set,  NULL );

	scan_set_label_device ( base, base->dtv->scan.dvb_device, base->dtv->scan.adapter_set, base->dtv->scan.frontend_set );

	base->dtv->scan.delsys_set = helia_get_dvb_delsys ( base->dtv->scan.adapter_set, base->dtv->scan.frontend_set );

	g_signal_handler_block   ( base->dtv->scan.combo_delsys, base->dtv->scan.desys_signal_id );

		gtk_combo_box_set_active ( GTK_COMBO_BOX ( base->dtv->scan.combo_delsys ), base->dtv->scan.delsys_set );

	g_signal_handler_unblock   ( base->dtv->scan.combo_delsys, base->dtv->scan.desys_signal_id );

	g_debug ( "scan_set_frontend" );
}

static void scan_set_delsys ( GtkComboBox *combo_box, Base *base )
{
	g_debug ( "scan_set_delsys: delsys_set %d ", base->dtv->scan.delsys_set );

	uint num = gtk_combo_box_get_active ( combo_box );

	base->dtv->scan.delsys_set = num;

	g_object_set ( base->dtv->scan.scan_dvbsrc, "delsys", num, NULL );

	helia_set_dvb_delsys ( base->dtv->scan.adapter_set, base->dtv->scan.frontend_set, base->dtv->scan.delsys_set );
}



static char * strip_ch_name_convert ( char *name )
{
	uint i = 0;
	for ( i = 0; name[i] != '\0'; i++ )
	{
		if ( name[i] == ':' || name[i] == '[' || name[i] == ']' ) name[i] = ' ';
	}
	return g_strstrip ( name );
}

static void helia_strip_dvb5_to_dvbsrc ( Base *base, const char *section, uint num )
{
	// g_debug ( "gmp_strip_dvb5_to_dvbsrc:: section: %s", section );

	uint n = 0, z = 0, x = 0;

	char **lines = g_strsplit ( section, "\n", 0 );

	GString *gstring = g_string_new ( NULL );

	char *ch_name = NULL;

	for ( n = 0; lines[n] != NULL; n++ )
	{
		if ( n == 0 )
		{
			ch_name = strip_ch_name_convert ( lines[n] );
			gstring = g_string_new ( ch_name );
			g_string_append_printf ( gstring, ":delsys=%d:adapter=%d:frontend=%d", base->dtv->scan.delsys_set, base->dtv->scan.adapter_set, base->dtv->scan.frontend_set );

			g_print ( "gmp_convert_dvb5:: Channel: %s ( %d ) \n", lines[n], num );
		}

		for ( z = 0; z < G_N_ELEMENTS ( gst_param_dvb_descr_n ); z++ )
		{
			if ( g_strrstr ( lines[n], gst_param_dvb_descr_n[z].dvb_v5_name ) )
			{
				char **value_key = g_strsplit ( lines[n], " = ", 0 );

				if ( !g_str_has_prefix ( g_strstrip ( value_key[0] ), gst_param_dvb_descr_n[z].dvb_v5_name ) )
				{
					g_strfreev ( value_key );
					continue;
				}

				g_string_append_printf ( gstring, ":%s=", gst_param_dvb_descr_n[z].gst_param );

				// g_debug ( " gst_param -> dvb5: %s ( %s | %s )", gst_param_dvb_descr_n[z].gst_param, lines[n], gst_param_dvb_descr_n[z].dvb_v5_name );

				if ( g_strrstr ( gst_param_dvb_descr_n[z].dvb_v5_name, "SAT_NUMBER" ) )
				{
					g_string_append ( gstring, value_key[1] );

					g_strfreev ( value_key );
					continue;
				}

				if ( gst_param_dvb_descr_n[z].cdsc == 0 )
				{
					g_string_append ( gstring, value_key[1] );
				}
				else
				{
					for ( x = 0; x < gst_param_dvb_descr_n[z].cdsc; x++ )
					{
						if ( g_strrstr ( value_key[1], gst_param_dvb_descr_n[z].dvb_descr[x].dvb_v5_name ) )
						{
							g_string_append_printf ( gstring, "%d", gst_param_dvb_descr_n[z].dvb_descr[x].descr_num );
						}
					}
				}

				g_strfreev ( value_key );
			}
		}
	}

	if ( g_strrstr ( gstring->str, "audio-pid" ) || g_str_has_prefix ( gstring->str, "video-pid" ) ) // ignore other
		treeview_append_base ( base->dtv->treeview, ch_name, gstring->str );

	g_string_free ( gstring, TRUE );

	g_strfreev ( lines );
}

static void helia_convert_dvb5 ( Base *base, const char *file )
{
	uint n = 0;
	char *contents;

	GError *err = NULL;

	if ( g_file_get_contents ( file, &contents, 0, &err ) )
	{
		char **lines = g_strsplit ( contents, "[", 0 );
		uint length = g_strv_length ( lines );

		for ( n = 1; n < length; n++ )
			helia_strip_dvb5_to_dvbsrc ( base, lines[n], n );

		g_strfreev ( lines );
		g_free ( contents );
	}
	else
	{
		base_message_dialog ( "", err->message, GTK_MESSAGE_ERROR, base->window );

		g_error_free ( err );

		return;
	}
}

static void convert_file ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	const char *text = gtk_entry_get_text ( base->dtv->scan.entry_convert );

	if ( text && g_str_has_suffix ( text, ".conf" ) )
	{
		if ( g_file_test ( text, G_FILE_TEST_EXISTS ) )
			helia_convert_dvb5 ( base, text );
		else
			base_message_dialog ( "", g_strerror ( errno ), GTK_MESSAGE_ERROR, base->window );
	}
	else
	{
		g_print ( "convert_file:: no convert %s \n", text );
	}
}

static void  convert_set_file ( GtkEntry *entry, G_GNUC_UNUSED GtkEntryIconPosition icon_pos, G_GNUC_UNUSED GdkEventButton *event, Base *base )
{
	char *file_name = pref_open_file ( base, g_get_home_dir () );

	if ( file_name == NULL ) return;

	gtk_entry_set_text ( entry, file_name );

	g_free ( file_name );
}

static GtkBox * scan_convert ( Base *base )
{
	GtkBox *g_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

	GtkLabel *label = (GtkLabel *)gtk_label_new ( "DVBv5   ⇨  GtvDvb" );
	gtk_box_pack_start ( g_box, GTK_WIDGET ( label ), FALSE, FALSE, 10 );

	//GtkImage *image = base_create_image ( "helia-convert", 48 );
	//gtk_box_pack_start ( g_box, GTK_WIDGET ( image ), TRUE, TRUE, 0 );

	base->dtv->scan.entry_convert = (GtkEntry *)gtk_entry_new ();
	gtk_entry_set_text ( base->dtv->scan.entry_convert, "dvb_channel.conf" );

	g_object_set ( base->dtv->scan.entry_convert, "editable", FALSE, NULL );
	gtk_entry_set_icon_from_icon_name ( base->dtv->scan.entry_convert, GTK_ENTRY_ICON_SECONDARY, "folder" );
	g_signal_connect ( base->dtv->scan.entry_convert, "icon-press", G_CALLBACK ( convert_set_file ), base );

	gtk_box_pack_start ( g_box, GTK_WIDGET ( base->dtv->scan.entry_convert ), FALSE, FALSE, 10 );

	GtkButton *button_convert = (GtkButton *)gtk_button_new_from_icon_name ( "helia-convert", GTK_ICON_SIZE_SMALL_TOOLBAR );
	g_signal_connect ( button_convert, "clicked", G_CALLBACK ( convert_file ), base );

	gtk_box_pack_start ( g_box, GTK_WIDGET ( button_convert ), FALSE, FALSE, 0 );

	return g_box;
}

static GtkBox * scan_device ( Base *base )
{
	base->dtv->scan.delsys_set = helia_get_dvb_delsys ( base->dtv->scan.adapter_set, base->dtv->scan.frontend_set );

	GtkBox *g_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( g_box ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( g_box ), 10 );

	GtkGrid *grid = (GtkGrid *)gtk_grid_new();
	gtk_grid_set_column_homogeneous ( GTK_GRID ( grid ), TRUE );
	gtk_box_pack_start ( g_box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );

	struct data_a { const char *text; uint value; void (* activate)(); } data_a_n[] =
	{
		{ "DVB Device", 0, NULL },
		{ "",           0, NULL },
		{ "Adapter",    base->dtv->scan.adapter_set,  scan_set_adapter  },
		{ "Frontend",   base->dtv->scan.frontend_set, scan_set_frontend },
		{ "DelSys",     base->dtv->scan.delsys_set,   scan_set_delsys   }
	};

	uint d = 0, c = 0;
	for ( d = 0; d < G_N_ELEMENTS ( data_a_n ); d++ )
	{
		GtkLabel *label = (GtkLabel *)gtk_label_new ( data_a_n[d].text );
		gtk_widget_set_halign ( GTK_WIDGET ( label ), ( d == 0 ) ? GTK_ALIGN_CENTER : GTK_ALIGN_START );
		gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( label ), 0, d, ( d == 0 ) ? 2 : 1, 1 );

		if ( d == 0 ) { base->dtv->scan.dvb_device = label; scan_set_label_device ( base, base->dtv->scan.dvb_device, base->dtv->scan.adapter_set, base->dtv->scan.frontend_set ); }

		if ( d == 2 || d == 3 )
		{
			GtkSpinButton *spinbutton = (GtkSpinButton *)gtk_spin_button_new_with_range ( 0, 16, 1 );
			gtk_spin_button_set_value ( spinbutton, data_a_n[d].value );
			g_signal_connect ( spinbutton, "changed", G_CALLBACK ( data_a_n[d].activate ), base );

			gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( spinbutton ), 1, d, 1, 1 );
		}

		if ( d == 4 )
		{
			base->dtv->scan.combo_delsys = (GtkComboBoxText *) gtk_combo_box_text_new ();

			for ( c = 0; c < G_N_ELEMENTS ( dvb_descr_delsys_type_n ); c++ )
				gtk_combo_box_text_append_text ( base->dtv->scan.combo_delsys, dvb_descr_delsys_type_n[c].text_vis );

			gtk_combo_box_set_active ( GTK_COMBO_BOX ( base->dtv->scan.combo_delsys ), data_a_n[d].value );
			base->dtv->scan.desys_signal_id = g_signal_connect ( base->dtv->scan.combo_delsys, "changed", G_CALLBACK ( data_a_n[d].activate ), base );

			gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( base->dtv->scan.combo_delsys ), 1, d, 1, 1 );
		}
	}

	gtk_box_pack_start ( g_box, GTK_WIDGET ( scan_convert ( base ) ), TRUE, TRUE, 0 );

	return g_box;
}

static void scan_start ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	if ( GST_ELEMENT_CAST ( base->dtv->scan.pipeline_scan )->current_state == GST_STATE_PLAYING )
		return;

	mpegts_clear ( base );

	time ( &base->dtv->scan.t_start_scan );

	gst_element_set_state ( base->dtv->scan.pipeline_scan, GST_STATE_PLAYING );
}

void scan_stop ( G_GNUC_UNUSED GtkButton *button, Base *base )
{
	if ( GST_ELEMENT_CAST ( base->dtv->scan.pipeline_scan )->current_state == GST_STATE_NULL )
		return;

	gst_element_set_state ( base->dtv->scan.pipeline_scan, GST_STATE_NULL );

	dtv_level_set_sgn_snr ( base, base->dtv->scan.level_scan, 0, 0, FALSE );
}

static void scan_create_control_battons ( Base *base, GtkBox *b_box )
{
	gtk_box_pack_start ( b_box, GTK_WIDGET ( gtk_label_new ( " " ) ), TRUE, TRUE, 0 );

	gtk_box_pack_start ( b_box, GTK_WIDGET ( dtv_level_base_scan ( base ) ), FALSE, FALSE, 0 );

	GtkBox *hb_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( hb_box ), 5 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( hb_box ), 5 );
	gtk_box_set_spacing ( hb_box, 5 );

	base_create_image_button ( hb_box, "helia-play",  16, scan_start, base );
	base_create_image_button ( hb_box, "helia-stop",  16, scan_stop,  base );

	gtk_box_pack_start ( b_box, GTK_WIDGET ( hb_box ), FALSE, FALSE, 5 );
}

static GtkBox * scan_all_box ( Base *base, uint i )
{
	GtkBox *only_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

	if ( i == PAGE_SC )  { return scan_device   ( base ); }
	if ( i == PAGE_CH )  { return scan_channels ( base ); }
	if ( i == PAGE_DT )  { return scan_dvb_all  ( base, G_N_ELEMENTS ( dvbt_props_n  ), dvbt_props_n,  "DVB-T"  ); }
	if ( i == PAGE_DS )  { return scan_dvb_all  ( base, G_N_ELEMENTS ( dvbs_props_n  ), dvbs_props_n,  "DVB-S"  ); }
	if ( i == PAGE_DC )  { return scan_dvb_all  ( base, G_N_ELEMENTS ( dvbc_props_n  ), dvbc_props_n,  "DVB-C"  ); }
	if ( i == PAGE_AT )  { return scan_dvb_all  ( base, G_N_ELEMENTS ( atsc_props_n  ), atsc_props_n,  "ATSC"   ); }
	if ( i == PAGE_DM )  { return scan_dvb_all  ( base, G_N_ELEMENTS ( dtmb_props_n  ), dtmb_props_n,  "DTMB"   ); }
	if ( i == PAGE_IT )  { return scan_isdb_all ( base, G_N_ELEMENTS ( isdbt_props_n ), isdbt_props_n, "ISDB-T" ); }
	if ( i == PAGE_IS )  { return scan_isdb_all ( base, G_N_ELEMENTS ( isdbs_props_n ), isdbs_props_n, "ISDB-S" ); }

	return only_box;
}

static void scan_quit ( G_GNUC_UNUSED GtkWindow *win, Base *base )
{
	base->dtv->scan.scan_quit = TRUE;

	scan_stop ( NULL, base );
}

void scan_win_create ( Base *base )
{
	GtkWindow *window =      (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_transient_for ( window, base->window );
	gtk_window_set_title     ( window, "" );
	gtk_window_set_modal     ( window, TRUE );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	g_signal_connect ( window, "destroy", G_CALLBACK ( scan_quit ), base );

	GdkPixbuf *pixbuf = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					 "helia-display", 48, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	gtk_window_set_icon ( window, pixbuf );

	if ( pixbuf ) g_object_unref ( pixbuf );

	base->dtv->scan.scan_quit  = FALSE;
	base->dtv->scrambling = FALSE;

	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	GtkNotebook *notebook = (GtkNotebook *)gtk_notebook_new ();
	gtk_notebook_set_scrollable ( notebook, TRUE );

	gtk_widget_set_margin_top    ( GTK_WIDGET ( notebook ), 5 );
	gtk_widget_set_margin_bottom ( GTK_WIDGET ( notebook ), 5 );
	gtk_widget_set_margin_start  ( GTK_WIDGET ( notebook ), 5 );
	gtk_widget_set_margin_end    ( GTK_WIDGET ( notebook ), 5 );

	GtkBox *m_box_n[PAGE_NUM];

	uint j = 0;
	for ( j = 0; j < PAGE_NUM; j++ )
	{
		m_box_n[j] = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
		gtk_box_pack_start ( m_box_n[j], GTK_WIDGET ( scan_all_box ( base, j ) ), TRUE, TRUE, 0 );
		gtk_notebook_append_page ( notebook, GTK_WIDGET ( m_box_n[j] ),  gtk_label_new ( lang_set ( base, scan_label_n[j].name ) ) );

		if ( j == PAGE_SC ) scan_create_control_battons ( base, m_box_n[PAGE_SC] );
	}

	gtk_notebook_set_tab_pos ( notebook, GTK_POS_TOP );
	gtk_box_pack_start ( m_box, GTK_WIDGET (notebook), TRUE, TRUE, 0 );

	GtkButton *button = base_set_image_button ( "helia-exit", 16 );
	g_signal_connect_swapped ( button, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );
	gtk_box_pack_start ( h_box,  GTK_WIDGET ( button ), TRUE, TRUE, 5 );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );
	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 5 );
	gtk_container_add ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );

	gtk_widget_show_all ( GTK_WIDGET ( window ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( window ), base->opacity_win );
}



// ***** Linux Dvb *****

const char * helia_get_dvb_type_str ( int delsys )
{
	const char *dvb_type = NULL;

	uint i = 0;
	for ( i = 0; i < G_N_ELEMENTS ( dvb_descr_delsys_type_n ); i++ )
	{
		if ( dvb_descr_delsys_type_n[i].descr_num == delsys )
			dvb_type = dvb_descr_delsys_type_n[i].text_vis;
	}

	return dvb_type;
}

static void helia_set_dvb_delsys_fd ( int fd, uint del_sys )
{
	struct dtv_property dvb_prop[1];
	struct dtv_properties cmdseq;

	dvb_prop[0].cmd = DTV_DELIVERY_SYSTEM;
	dvb_prop[0].u.data = del_sys;

	cmdseq.num = 1;
	cmdseq.props = dvb_prop;

	const char *type = dvb_descr_delsys_type_n[del_sys].text_vis;

	if ( ioctl ( fd, FE_SET_PROPERTY, &cmdseq ) == -1 )
		base_message_dialog ( g_strerror ( errno ), type, GTK_MESSAGE_ERROR, NULL );
	else
		g_print ( "Set DTV_DELIVERY_SYSTEM - %s Ok \n", type );
}

static uint helia_get_dvb_delsys_fd ( int fd, struct dvb_frontend_info info )
{
	uint dtv_del_sys = SYS_UNDEFINED, dtv_api_ver = 0, SYS_DVBC = SYS_DVBC_ANNEX_A;

	struct dtv_property dvb_prop[2];
	struct dtv_properties cmdseq;

	dvb_prop[0].cmd = DTV_DELIVERY_SYSTEM;
	dvb_prop[1].cmd = DTV_API_VERSION;

	cmdseq.num = 2;
	cmdseq.props = dvb_prop;

	if ( ( ioctl ( fd, FE_GET_PROPERTY, &cmdseq ) ) == -1 )
	{
		perror ( "helia_get_dvb_delsys_fd: ioctl FE_GET_PROPERTY " );

		dtv_api_ver = 0x300;
		gboolean legacy = FALSE;

		switch ( info.type )
		{
			case FE_QPSK:
				legacy = TRUE;
				dtv_del_sys = SYS_DVBS;
				break;

			case FE_OFDM:
				legacy = TRUE;
				dtv_del_sys = SYS_DVBT;
				break;

			case FE_QAM:
				legacy = TRUE;
				dtv_del_sys = SYS_DVBC;
				break;

			case FE_ATSC:
				legacy = TRUE;
				dtv_del_sys = SYS_ATSC;
				break;

			default:
				break;
		}

		if ( legacy )
			g_print ( "DVBv3  Ok \n" );
		else
			g_critical ( "DVBv3  Failed \n" );
	}
	else
	{
		g_print ( "DVBv5  Ok \n" );

		dtv_del_sys = dvb_prop[0].u.data;
		dtv_api_ver = dvb_prop[1].u.data;
	}

	g_print ( "DVB DTV_DELIVERY_SYSTEM: %d | DVB API Version: %d.%d \n", dtv_del_sys, dtv_api_ver / 256, dtv_api_ver % 256 );

	return dtv_del_sys;
}

void helia_set_dvb_delsys ( uint adapter, uint frontend, uint delsys )
{
	int fd = 0, flags = O_RDWR;

	char *fd_name = g_strdup_printf ( "/dev/dvb/adapter%d/frontend%d", adapter, frontend );

	if ( ( fd = g_open ( fd_name, flags ) ) == -1 )
	{
		g_critical ( "helia_set_dvb_delsys: %s %s \n", fd_name, g_strerror ( errno ) );

		g_free  ( fd_name );

		return;
	}

	struct dvb_frontend_info info;

	if ( ( ioctl ( fd, FE_GET_INFO, &info ) ) == -1 )
	{
		perror ( "helia_set_dvb_delsys: ioctl FE_GET_INFO " );
	}
	else
	{
		if ( flags == O_RDWR )
			helia_set_dvb_delsys_fd ( fd, delsys );
		else
			g_print ( "DVBv5 Not Set delivery system: O_RDONLY \n" );
	}

	g_close ( fd, NULL );
	g_free  ( fd_name  );
}

uint helia_get_dvb_delsys ( uint adapter, uint frontend )
{
	uint dtv_delsys = SYS_UNDEFINED;

	int fd = 0, flags = O_RDWR;

	char *fd_name = g_strdup_printf ( "/dev/dvb/adapter%d/frontend%d", adapter, frontend );

	if ( ( fd = g_open ( fd_name, flags ) ) == -1 )
	{
		flags = O_RDONLY;

		if ( ( fd = g_open ( fd_name, flags ) ) == -1 )
		{
			g_critical ( "helia_get_dvb_info: %s %s \n", fd_name, g_strerror ( errno ) );

			g_free  ( fd_name );

			return dtv_delsys;
		}
	}

	struct dvb_frontend_info info;

	if ( ( ioctl ( fd, FE_GET_INFO, &info ) ) == -1 )
	{
		perror ( "helia_get_dvb_delsys: ioctl FE_GET_INFO " );
	}
	else
	{
		dtv_delsys = helia_get_dvb_delsys_fd ( fd, info );
	}

	g_close ( fd, NULL );
	g_free  ( fd_name  );

	return dtv_delsys;
}

char * helia_get_dvb_info ( Base *base, uint adapter, uint frontend )
{
	int fd = 0, flags = O_RDWR;

	char *fd_name = g_strdup_printf ( "/dev/dvb/adapter%d/frontend%d", adapter, frontend );

	if ( ( fd = g_open ( fd_name, flags ) ) == -1 )
	{
		flags = O_RDONLY;

		if ( ( fd = g_open ( fd_name, flags ) ) == -1 )
		{
			g_critical ( "helia_get_dvb_info: %s %s \n", fd_name, g_strerror ( errno ) );

			g_free  ( fd_name );

			return g_strdup ( lang_set ( base, "Undefined" ) );
		}
	}

	struct dvb_frontend_info info;

	if ( ( ioctl ( fd, FE_GET_INFO, &info ) ) == -1 )
	{
		perror ( "helia_get_dvb_info: ioctl FE_GET_INFO " );

		g_close ( fd, NULL );
		g_free  ( fd_name );

		return g_strdup ( lang_set ( base, "Undefined" ) );
	}

	g_debug ( "DVB device: %s ( %s ) \n", info.name, fd_name );

	g_close ( fd, NULL );
	g_free  ( fd_name  );

	return g_strdup ( info.name );
}
