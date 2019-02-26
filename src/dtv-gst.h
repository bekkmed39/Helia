/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef DTV_GST_H
#define DTV_GST_H


GstElement * dtv_gst_create ( Base *base );

void dtv_stop ( Base *base );
void dtv_gst_record ( Base *base );
void dtv_stop_set_play ( Base *base, const char *name_file );

void dtv_mute_set ( GstElement *dvbplay );
gboolean dtv_mute_get ( GstElement *dvbplay );
void dtv_volume_changed ( G_GNUC_UNUSED GtkScaleButton *button, gdouble value, Base *base );

GstElement * dtv_gst_ret_iterate_element ( GstElement *it_element, const char *name1, const char *name2 );

void dtv_gst_add_audio_track ( Base *base, GtkComboBoxText *combo );
void dtv_gst_changed_audio_track ( Base *base, int changed_track_audio );


#endif /* DTV_GST_H */
