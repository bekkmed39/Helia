/*
* Copyright 2018 - 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/

#ifndef _SYSTEMS_
#define _SYSTEMS_


#include <gst/gst.h>
#include <glib.h>


void systems_get_key ( const gchar *prog_name, guint prog_num );
void systems_set_key ( guint8 *data, guint size );


#endif /* _SYSTEMS_ */

