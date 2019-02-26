/*
* Copyright 2019 Stepan Perun
* This program is free software.
*
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef PREF_H
#define PREF_H


void about_win ( GtkWindow *window );
void pref_win ( Base *base );
void keyb_win ( Base *base );

void pref_read_config ( Base *base );
void pref_save_config ( Base *base );

/* Returns a newly-allocated string holding the result. Free with free() */
char * pref_open_dir  ( Base *base, const char *path );

/* Returns a newly-allocated string holding the result. Free with free() */
char * pref_open_file ( Base *base, const char *path );

/* Returns a GSList containing the filenames. Free the returned list with g_slist_free(), and the filenames with g_free(). */
GSList * pref_open_files ( Base *base, const char *path );

void dialog_open_dir ( Base *base );
void dialog_open_files ( Base *base );
void add_arg ( GFile **files, int n_files, Base *base );


#endif // PREF_H
