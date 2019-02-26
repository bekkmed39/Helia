/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#define GST_USE_UNSTABLE_API
#include <gst/mpegts/mpegts.h>

#include <base.h>

#include "scan.h"


void mpegts_initialize ()
{
	gst_mpegts_initialize ();

	g_type_class_ref ( GST_TYPE_MPEGTS_SECTION_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_SECTION_TABLE_ID );
	g_type_class_ref ( GST_TYPE_MPEGTS_RUNNING_STATUS );
	g_type_class_ref ( GST_TYPE_MPEGTS_DESCRIPTOR_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_DVB_DESCRIPTOR_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_ATSC_DESCRIPTOR_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_ISDB_DESCRIPTOR_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_MISC_DESCRIPTOR_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_ISO639_AUDIO_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_DVB_SERVICE_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_DVB_TELETEXT_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_STREAM_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_SECTION_DVB_TABLE_ID );
	g_type_class_ref ( GST_TYPE_MPEGTS_SECTION_ATSC_TABLE_ID );
	g_type_class_ref ( GST_TYPE_MPEGTS_SECTION_SCTE_TABLE_ID );
	g_type_class_ref ( GST_TYPE_MPEGTS_COMPONENT_STREAM_CONTENT );
	g_type_class_ref ( GST_TYPE_MPEGTS_CONTENT_NIBBLE_HI );
}

void mpegts_clear ( Base *base )
{
	base->dtv->scan.mpegts.pat_done = FALSE;
	base->dtv->scan.mpegts.pmt_done = FALSE;
	base->dtv->scan.mpegts.sdt_done = FALSE;
	base->dtv->scan.mpegts.vct_done = FALSE;

	base->dtv->scan.mpegts.pat_count = 0;
	base->dtv->scan.mpegts.pmt_count = 0;
	base->dtv->scan.mpegts.sdt_count = 0;
	base->dtv->scan.mpegts.vct_count = 0;

	uint j = 0;
	for ( j = 0; j < MAX_RUN_PAT; j++ )
	{
		base->dtv->scan.mpegts.ppsv[j].pat_sid = 0;
		base->dtv->scan.mpegts.ppsv[j].pmt_sid = 0;
		base->dtv->scan.mpegts.ppsv[j].sdt_sid = 0;
		base->dtv->scan.mpegts.ppsv[j].vct_sid = 0;

		base->dtv->scan.mpegts.ppsv[j].pmt_apid = 0;
		base->dtv->scan.mpegts.ppsv[j].pmt_vpid = 0;
	
		base->dtv->scan.mpegts.ppsv[j].sdt_name = NULL;
		base->dtv->scan.mpegts.ppsv[j].vct_name = NULL;

		base->dtv->scan.mpegts.ppsv[j].ca_sys = 0;
		base->dtv->scan.mpegts.ppsv[j].ca_pid = 0;
	}
}

static const char * mpegts_enum_name ( GType instance_type, int val )
{
	GEnumValue *en = g_enum_get_value ( G_ENUM_CLASS ( g_type_class_peek ( instance_type ) ), val );

	if ( en == NULL ) return "Unknown";

	return en->value_nick;
}

static void mpegts_pat ( GstMpegtsSection *section, Base *base )
{
	GPtrArray *pat = gst_mpegts_section_get_pat ( section );

	g_debug ( "PAT: %d Programs ", pat->len );

	uint i = 0;
	for ( i = 0; i < pat->len; i++ )
	{
		if ( i >= MAX_RUN_PAT )
		{
			g_print ( "MAX %d: PAT scan stop  \n", MAX_RUN_PAT );
			break;
		}

		GstMpegtsPatProgram *patp = g_ptr_array_index ( pat, i );

		if ( patp->program_number == 0 ) continue;

		base->dtv->scan.mpegts.ppsv[base->dtv->scan.mpegts.pat_count].pat_sid = patp->program_number;
		base->dtv->scan.mpegts.pat_count++;

		g_debug ( "     Program number: %d (0x%04x) | network or pg-map pid: 0x%04x ", 
				patp->program_number, patp->program_number, patp->network_or_program_map_PID );
	}

	g_ptr_array_unref ( pat );

	base->dtv->scan.mpegts.pat_done = TRUE;
	g_debug ( "PAT Done: pat_count %d ", base->dtv->scan.mpegts.pat_count );
}

static void mpegts_pmt ( GstMpegtsSection *section, Base *base )
{
	if ( base->dtv->scan.mpegts.pmt_count >= MAX_RUN_PAT )
	{
		g_print ( "MAX %d: PMT scan stop  \n", MAX_RUN_PAT );
		return;
	}

	uint i = 0, c = 0, len = 0;
	gboolean first_audio = TRUE;

	const GstMpegtsPMT *pmt = gst_mpegts_section_get_pmt ( section );
	len = pmt->streams->len;

	base->dtv->scan.mpegts.ppsv[base->dtv->scan.mpegts.pmt_count].pmt_sid = pmt->program_number;

	g_debug ( "PMT: %d  ( %d )", base->dtv->scan.mpegts.pmt_count+1, len );

	g_debug ( "     Program number     : %d (0x%04x) ", pmt->program_number, pmt->program_number );
	g_debug ( "     Pcr pid            : %d (0x%04x) ", pmt->pcr_pid, pmt->pcr_pid );
	g_debug ( "     %d Streams: ", len );

	for ( i = 0; i < len; i++ )
	{
		GstMpegtsPMTStream *stream = g_ptr_array_index ( pmt->streams, i );

		g_debug ( "       pid: %d (0x%04x), stream_type:0x%02x (%s) ", stream->pid, stream->pid, stream->stream_type,
			mpegts_enum_name (GST_TYPE_MPEGTS_STREAM_TYPE, stream->stream_type) );

		const char *name_t = mpegts_enum_name ( GST_TYPE_MPEGTS_STREAM_TYPE, stream->stream_type );

		if ( g_strrstr ( name_t, "video" ) )
			base->dtv->scan.mpegts.ppsv[base->dtv->scan.mpegts.pmt_count].pmt_vpid = stream->pid;

		if ( g_strrstr ( name_t, "audio" ) )
		{
			if ( first_audio )
				base->dtv->scan.mpegts.ppsv[base->dtv->scan.mpegts.pmt_count].pmt_apid = stream->pid;

			first_audio = FALSE;

			char *lang = NULL;

			GPtrArray *descriptors = stream->descriptors;
			for ( c = 0; c < descriptors->len; c++ )
			{
				GstMpegtsDescriptor *desc = g_ptr_array_index ( descriptors, c );

				GstMpegtsISO639LanguageDescriptor *res;
				if (  gst_mpegts_descriptor_parse_iso_639_language ( desc, &res )  )
				{
					lang = g_strdup ( res->language[0] );
					gst_mpegts_iso_639_language_descriptor_free (res);
				}
			}

			g_debug ( "       lang: %s | %d ", lang ? lang : "none", stream->pid );

			if ( lang ) g_free ( lang );
		}
	}

	base->dtv->scan.mpegts.pmt_count++;

	if ( base->dtv->scan.mpegts.pat_count > 0 && base->dtv->scan.mpegts.pmt_count == base->dtv->scan.mpegts.pat_count )
	{
		base->dtv->scan.mpegts.pmt_done = TRUE;
		g_debug ( "PMT Done: pmt_count %d ", base->dtv->scan.mpegts.pmt_count );
	}
}

static void mpegts_sdt ( GstMpegtsSection *section, Base *base )
{
	if ( base->dtv->scan.mpegts.sdt_count >= MAX_RUN_PAT )
	{
		g_print ( "MAX %d: SDT scan stop  \n", MAX_RUN_PAT );
		return;
	}

	uint i = 0, z = 0, c = 0, len = 0;

	const GstMpegtsSDT *sdt = gst_mpegts_section_get_sdt ( section );

	len = sdt->services->len;
	g_debug ( "Services: %d  ( %d ) ", base->dtv->scan.mpegts.sdt_count+1, len );

	for ( i = 0; i < len; i++ )
	{
		if ( i >= MAX_RUN_PAT ) break;

		GstMpegtsSDTService *service = g_ptr_array_index ( sdt->services, i );

		base->dtv->scan.mpegts.ppsv[base->dtv->scan.mpegts.sdt_count].sdt_name = NULL;
		base->dtv->scan.mpegts.ppsv[base->dtv->scan.mpegts.sdt_count].sdt_sid = service->service_id;

		gboolean get_descr = FALSE;

		if ( base->dtv->scan.mpegts.pat_done )
		{
			for ( z = 0; z < base->dtv->scan.mpegts.pat_count; z++ )
				if ( base->dtv->scan.mpegts.ppsv[z].pat_sid == service->service_id )
					{  get_descr = TRUE; break; }
		}

		if ( !get_descr ) continue;

		g_debug ( "  Service id: count %d | %d  ( 0x%04x ) ", base->dtv->scan.mpegts.sdt_count+1, service->service_id, service->service_id );

		g_debug ( "    Running: 0x%02x ( %s ) ", service->running_status,
        	mpegts_enum_name ( GST_TYPE_MPEGTS_RUNNING_STATUS, service->running_status )  );
		g_debug ( "    CA mode: %d ( %s ) ", 
			service->free_CA_mode, service->free_CA_mode ? "Maybe Scrambled" : "Not Scrambled"  );

		GPtrArray *descriptors = service->descriptors;
		for ( c = 0; c < descriptors->len; c++ )
		{
			GstMpegtsDescriptor *desc = g_ptr_array_index ( descriptors, c );

			char *service_name, *provider_name;
			GstMpegtsDVBServiceType service_type;

			if ( desc->tag == GST_MTS_DESC_DVB_SERVICE )
			{
				if ( gst_mpegts_descriptor_parse_dvb_service ( desc, &service_type, &service_name, &provider_name ) )
				{
					base->dtv->scan.mpegts.ppsv[base->dtv->scan.mpegts.sdt_count].sdt_name = g_strdup ( service_name );

					g_debug ( "    Service Descriptor, type:0x%02x (%s) ",
						service_type, mpegts_enum_name (GST_TYPE_MPEGTS_DVB_SERVICE_TYPE, service_type) );
					g_debug ( "    Service  (name) : %s ", service_name  );
					g_debug ( "    Provider (name) : %s \n", provider_name );

					g_free ( service_name  );
					g_free ( provider_name );
				}
			}
		}

		base->dtv->scan.mpegts.sdt_count++;
	}

	if ( base->dtv->scan.mpegts.pat_count > 0 && base->dtv->scan.mpegts.sdt_count == base->dtv->scan.mpegts.pat_count )
	{
		base->dtv->scan.mpegts.sdt_done = TRUE;
		g_debug ( "SDT Done: sdt_count %d ", base->dtv->scan.mpegts.sdt_count );
	}
}

static void mpegts_vct ( GstMpegtsSection *section, Base *base )
{
	const GstMpegtsAtscVCT *vct;

	if ( GST_MPEGTS_SECTION_TYPE (section) == GST_MPEGTS_SECTION_ATSC_CVCT )
		vct = gst_mpegts_section_get_atsc_cvct ( section );
	else
		vct = gst_mpegts_section_get_atsc_tvct ( section );

	g_assert (vct);
	g_debug ( "VCT " );

	g_debug ( "     transport_stream_id : 0x%04x ", vct->transport_stream_id );
	g_debug ( "     protocol_version    : %u ", vct->protocol_version );
	g_debug ( "     %d Sources: ", vct->sources->len );

	uint i = 0;
	for ( i = 0; i < vct->sources->len; i++ )
	{
		if ( i >= MAX_RUN_PAT )
		{
			g_print ( "MAX %d: VCT scan stop  \n", MAX_RUN_PAT );
			break;
		}

		GstMpegtsAtscVCTSource *source = g_ptr_array_index ( vct->sources, i );

		base->dtv->scan.mpegts.ppsv[base->dtv->scan.mpegts.vct_count].vct_name = g_strdup ( source->short_name );
		base->dtv->scan.mpegts.ppsv[base->dtv->scan.mpegts.vct_count].vct_sid  = source->program_number;

		g_debug ( "     Service id: %d  %d 0x%04x ", base->dtv->scan.mpegts.vct_count+1, source->program_number, source->program_number );
		g_debug ( "     Service  (name) : %s ", source->short_name );

		base->dtv->scan.mpegts.vct_count++;
	}

	base->dtv->scan.mpegts.vct_done = TRUE;
	g_debug ( "VCT Done: vct_count %d ", base->dtv->scan.mpegts.vct_count );
}

static void mpegts_cat ( GstMpegtsSection *section, Base *base )
{
	GPtrArray *cat = gst_mpegts_section_get_cat ( section );

	g_debug ( "CAT: %d CA system ", cat->len );

	uint i = 0;
	for ( i = 0; i < cat->len; i++ )
	{
		if ( i >= MAX_RUN_PAT )
		{
			g_print ( "MAX %d: CAT scan stop  \n", MAX_AUDIO );

			break;
		}

        guint16 ca_pid, ca_system_id;
        const guint8 *private_data;
        gsize private_data_size;

		GstMpegtsDescriptor *desc = g_ptr_array_index ( cat, i );

        if ( gst_mpegts_descriptor_parse_ca ( desc, &ca_system_id, &ca_pid, &private_data, &private_data_size ) ) 
		{
			base->dtv->scan.mpegts.ppsv[i].ca_sys = ca_system_id;
			base->dtv->scan.mpegts.ppsv[i].ca_pid = ca_pid;

			g_debug ( "     CA system id : %d ( 0x%04x ) ", ca_system_id, ca_system_id );
			g_debug ( "     CA PID       : %d ( 0x%04x ) ", ca_pid, ca_pid );

			if ( private_data_size ) g_debug ( "     Private Data " );
		}
	}

	g_ptr_array_unref ( cat );

	g_debug ( "CAT Done " );
}

void mpegts_parse_section ( GstMessage *message, Base *base )
{
	time ( &base->dtv->scan.t_cur_scan );

	if ( ( base->dtv->scan.t_cur_scan - base->dtv->scan.t_start_scan ) >= 5 )
	{
		scan_read_ch_to_treeview ( base );

		g_warning ( "mpegts_parse_section: time stop %ld (sec) \n", base->dtv->scan.t_cur_scan - base->dtv->scan.t_start_scan );

		return;
	}

	if ( base->dtv->scan.mpegts.pat_done && base->dtv->scan.mpegts.pmt_done 
	     && ( base->dtv->scan.mpegts.sdt_done || base->dtv->scan.mpegts.vct_done ) )
	{
		scan_read_ch_to_treeview ( base );

		return;
	}

	GstMpegtsSection *section = gst_message_parse_mpegts_section ( message );

	if ( section )
	{
		switch ( GST_MPEGTS_SECTION_TYPE ( section ) )
		{
			case GST_MPEGTS_SECTION_PAT:
				mpegts_pat ( section, base );
				break;

			case GST_MPEGTS_SECTION_CAT:
				mpegts_cat ( section, base );
				break;

			case GST_MPEGTS_SECTION_PMT:
				mpegts_pmt ( section, base );
				break;

			case GST_MPEGTS_SECTION_SDT:
				mpegts_sdt ( section, base );
				break;

			case GST_MPEGTS_SECTION_ATSC_CVCT:
			case GST_MPEGTS_SECTION_ATSC_TVCT:
				mpegts_vct ( section, base );

			default:
			break;
		}

		gst_mpegts_section_unref ( section );
	}
}

static void mpegts_pmt_info ( GstMpegtsSection *section, Base *base )
{
    uint num = 0, j = 0, c = 0;

    const GstMpegtsPMT *pmt = gst_mpegts_section_get_pmt ( section );
    GPtrArray *streams = pmt->streams;

    if ( base->dtv->sid == pmt->program_number )
    {
        for ( j = 0; j < streams->len; j++ )
        {
            GstMpegtsPMTStream *pmtstream = g_ptr_array_index ( streams, j );
            const char *name_t = mpegts_enum_name ( GST_TYPE_MPEGTS_STREAM_TYPE, pmtstream->stream_type );

            if ( g_strrstr ( name_t, "audio" ) )
            {
                char *lang = NULL;

                GPtrArray *descriptors = pmtstream->descriptors;
                for ( c = 0; c < descriptors->len; c++ )
                {
                    GstMpegtsDescriptor *desc = g_ptr_array_index ( descriptors, c );

                    GstMpegtsISO639LanguageDescriptor *res;
                    if (  gst_mpegts_descriptor_parse_iso_639_language ( desc, &res )  )
                    {
                        lang = g_strdup ( res->language[0] );
                        gst_mpegts_iso_639_language_descriptor_free (res);
                    }
                }

                base->dtv->audio_lang[num] = ( lang ) ? g_strdup ( lang ) : g_strdup_printf ( "%d", pmtstream->pid );

                g_debug ( "mpegts_pmt_info: lang %s | pmtstream->pid %d | num %d ", lang, pmtstream->pid, num );

                if ( lang ) g_free ( lang );

                num++;
            }
        }
    }
}

void mpegts_pmt_lang_section ( GstMessage *message, Base *base )
{
    GstMpegtsSection *section = gst_message_parse_mpegts_section ( message );

    if ( section )
    {
        switch ( GST_MPEGTS_SECTION_TYPE ( section ) )
        {
            case GST_MPEGTS_SECTION_PMT:
                mpegts_pmt_info ( section, base );
                break;

            default:
                break;
        }

        gst_mpegts_section_unref ( section );
    }
}
