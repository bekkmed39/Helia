/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef TREE_VIEW_H
#define TREE_VIEW_H


GtkTreeView * create_treeview ( Base *base, const char *title, gboolean player_tv );

GtkScrolledWindow * create_scroll_win ( GtkTreeView *tree_view, uint set_size );

GtkBox * treeview_box ( Base *base, GtkWindow *window, GtkTreeView *tree_view, GtkBox *box, gboolean player_tv );

void treeview_add_dtv  ( Base *base, const char *path );
void treeview_add_dir  ( Base *base, const char *dir_path );
void treeview_add_file ( Base *base, const char *path, gboolean filter_set, gboolean play );

void player_next ( Base *base );

void treeview_auto_save_tv ( GtkTreeView *tree_view, const char *filename );


#endif /* TREE_VIEW_H */
