/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef PLAYER_GST_H
#define PLAYER_GST_H


GstElement * player_gst_create ( Base *base );

GstElement * player_gst_rec_create ( Base *base, uint n_video, uint n_audio );

void player_stop ( Base *base );
void player_play ( Base *base );
void player_play_paused   ( Base *base );
void player_step_frame    ( Base *base );
void player_stop_set_play ( Base *base, const char *name_file );

void player_record ( Base *base );

void player_gst_new_pos ( Base *base, gint64 set_pos, gboolean up_dwn );

void player_mute_set ( Base *base );
gboolean player_mute_get ( Base *base );
void player_volume_changed ( G_GNUC_UNUSED GtkScaleButton *button, gdouble value, Base *base );

GstElement * player_gst_ret_iterate_element ( GstElement *it_element, const char *name1, const char *name2 );


#endif /* PLAYER_GST_H */
