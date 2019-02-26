/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef PLAYER_SLIDER_H
#define PLAYER_SLIDER_H


void player_slider_base_create  ( Base *base, GtkBox *m_box );
void player_slider_panel_create ( Base *base, GtkBox *m_box );
void player_slider_clear_data ( Base *base );

void player_slider_update_slider ( Slider slider, gdouble range, gdouble value );
void player_slider_set_data ( Base *base, gint64 pos, uint digits_pos, gint64 dur, uint digits_dur, gboolean sensitive );


#endif /* PLAYER_SLIDER_H */

