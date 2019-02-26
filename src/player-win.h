/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef PLAYER_WIN_H
#define PLAYER_WIN_H


GtkDrawingArea * player_win_create ( Base *base );

GtkPaned * player_win_paned_create ( Base *base, uint set_size );


#endif /* PLAYER_WIN_H */

