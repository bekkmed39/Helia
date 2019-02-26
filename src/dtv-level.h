/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef DTV_LEVEL_H
#define DTV_LEVEL_H


GtkBox * dtv_level_base_create   ( Base *base );
GtkBox * dtv_level_panel_create  ( Base *base );
GtkBox * dtv_level_base_scan     ( Base *base );

void dtv_level_set_sgn_snr ( Base *base, Level level, gdouble sgl, gdouble snr, gboolean hlook );


#endif // DTV_LEVEL_H
