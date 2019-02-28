/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef LANG_H
#define LANG_H


uint lang_get_def ();

const char * lang_set ( Base *base, const char *text );

void lang_add_combo ( GtkComboBoxText *combo );


#endif // LANG_H
