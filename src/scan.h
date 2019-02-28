/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef SCAN_H
#define SCAN_H


void scan_gst_create ( Base *base );
void scan_win_create ( Base *base );

void set_lnb_low_high_switch ( GstElement *element, int type_lnb );

uint helia_get_dvb_delsys ( uint adapter, uint frontend );
void helia_set_dvb_delsys ( uint adapter, uint frontend, uint delsys );

/* Returns a newly-allocated string holding the result. Free with free() */
char * helia_get_dvb_info ( Base *base, uint adapter, uint frontend );

const char * helia_get_dvb_type_str ( int delsys );
const char * scan_get_info ( const char *data );
const char * scan_get_info_descr_vis ( const char *data, int num );

void scan_read_ch_to_treeview ( Base *base );


#endif // SCAN_H
