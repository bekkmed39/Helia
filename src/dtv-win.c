/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <base.h>

#include "tree-view.h"
#include "dtv-panel.h"
#include "dtv-level.h"


static void dtv_win_draw_black ( GtkDrawingArea *widget, cairo_t *cr, GdkPixbuf *logo )
{
	GdkRGBA color; color.red = 0; color.green = 0; color.blue = 0; color.alpha = 1.0;

	int width = gtk_widget_get_allocated_width  ( GTK_WIDGET ( widget ) );
	int heigh = gtk_widget_get_allocated_height ( GTK_WIDGET ( widget ) );

	cairo_rectangle ( cr, 0, 0, width, heigh );

	gdk_cairo_set_source_rgba ( cr, &color );

	cairo_fill (cr);

	if ( logo != NULL )
	{
		int widthl  = gdk_pixbuf_get_width  ( logo );
		int heightl = gdk_pixbuf_get_height ( logo );

		cairo_rectangle ( cr, 0, 0, width, heigh );

		gdk_cairo_set_source_pixbuf ( cr, logo,
				( width / 2  ) - ( widthl  / 2 ), ( heigh / 2 ) - ( heightl / 2 ) );

		cairo_fill (cr);
	}
}

static gboolean dtv_win_draw_check ( GstElement *element, Base *base )
{
	if ( GST_ELEMENT_CAST ( element )->current_state == GST_STATE_NULL ) return TRUE;

    if ( GST_ELEMENT_CAST ( element )->current_state != GST_STATE_NULL )
    {
		if ( !base->dtv->checked_video ) return TRUE;
	}

    return FALSE;
}

static gboolean dtv_win_draw ( GtkDrawingArea *widget, cairo_t *cr, Base *base )
{
	if ( dtv_win_draw_check ( base->dtv->dvbplay, base ) ) dtv_win_draw_black ( widget, cr, base->pixbuf_tv );

	return FALSE;
}

static void dtv_win_realize ( GtkDrawingArea *drawingarea, Base *base )
{
    ulong xid = GDK_WINDOW_XID ( gtk_widget_get_window ( GTK_WIDGET ( drawingarea ) ) );

    base->dtv->window_hid = xid;

    g_print ( "GDK_WINDOW_XID: %ld \n", base->dtv->window_hid );
}

static void dtv_win_drag_in ( G_GNUC_UNUSED GtkDrawingArea *draw, GdkDragContext *ct, G_GNUC_UNUSED int x, G_GNUC_UNUSED int y, 
							  GtkSelectionData *s_data, G_GNUC_UNUSED uint info, guint32 time, Base *base )
{
	char **uris = gtk_selection_data_get_uris ( s_data );

	uint c = 0;

	for ( c = 0; uris[c] != NULL; c++ )
	{
		char *path = base_uri_get_path ( uris[c] );

		if ( g_str_has_suffix ( path, "gtv-channel.conf" ) )
		{
			treeview_add_dtv ( base, path );
		}

		free ( path );
	}

	g_strfreev ( uris );

	gtk_drag_finish ( ct, TRUE, FALSE, time );
}

static gboolean dtv_win_fullscreen ( GtkWindow *window )
{
	GdkWindowState state = gdk_window_get_state ( gtk_widget_get_window ( GTK_WIDGET ( window ) ) );

	if ( state & GDK_WINDOW_STATE_FULLSCREEN )
		{ gtk_window_unfullscreen ( window ); return FALSE; }
	else
		{ gtk_window_fullscreen   ( window ); return TRUE;  }

	return TRUE;
}

static gboolean dtv_win_press_event ( GtkDrawingArea *drawing, GdkEventButton *event, Base *base )
{
	if ( event->button == 1 && event->type == GDK_2BUTTON_PRESS )
	{
		GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( drawing ) ) );

		if ( dtv_win_fullscreen ( GTK_WINDOW ( window ) ) )
			gtk_widget_hide ( GTK_WIDGET ( base->dtv->vbox_sw_tv ) );

		return FALSE;
	}

	if ( event->button == 2 )
	{
		if ( gtk_widget_get_visible ( GTK_WIDGET ( base->dtv->vbox_sw_tv ) ) )
			gtk_widget_hide ( GTK_WIDGET ( base->dtv->vbox_sw_tv ) );
		else
			gtk_widget_show ( GTK_WIDGET ( base->dtv->vbox_sw_tv ) );

		return FALSE;
	}

	if ( event->button == 3 )
	{
		GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( drawing ) ) );

		dtv_panel_win_create ( window, base );

		return FALSE;
	}

	return FALSE;
}

GtkDrawingArea * dtv_win_create ( Base *base )
{
	GtkDrawingArea *video_win = (GtkDrawingArea *)gtk_drawing_area_new ();
	g_signal_connect ( video_win, "realize", G_CALLBACK ( dtv_win_realize ), base );
	g_signal_connect ( video_win, "draw",    G_CALLBACK ( dtv_win_draw    ), base );

	gtk_widget_set_events ( GTK_WIDGET ( video_win ), GDK_BUTTON_PRESS_MASK | GDK_SCROLL_MASK  );
	g_signal_connect ( video_win, "button-press-event", G_CALLBACK ( dtv_win_press_event  ), base );

	gtk_drag_dest_set ( GTK_WIDGET ( video_win ), GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY );
	gtk_drag_dest_add_uri_targets  ( GTK_WIDGET ( video_win ) );
	g_signal_connect  ( video_win, "drag-data-received", G_CALLBACK ( dtv_win_drag_in ), base );

	return video_win;
}

GtkPaned * dtv_win_paned_create ( Base *base, uint set_size )
{
	base->dtv->vbox_sw_tv = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

	GtkScrolledWindow *scroll = create_scroll_win ( base->dtv->treeview, set_size );

	GtkBox *b_box = treeview_box ( base, base->window, base->dtv->treeview, base->dtv->vbox_sw_tv, FALSE );

	gtk_box_pack_start ( base->dtv->vbox_sw_tv, GTK_WIDGET ( scroll ), TRUE,  TRUE,  0 );
	gtk_box_pack_end   ( base->dtv->vbox_sw_tv, GTK_WIDGET ( b_box  ), FALSE, FALSE, 0 );

	base->dtv->h_box_level_base = dtv_level_base_create ( base );
	gtk_box_pack_end ( base->dtv->vbox_sw_tv, GTK_WIDGET ( base->dtv->h_box_level_base ), FALSE,  FALSE,  0 );

	base->dtv->video = dtv_win_create ( base );

	GtkPaned *paned = (GtkPaned *)gtk_paned_new ( GTK_ORIENTATION_HORIZONTAL );
	gtk_paned_add1 ( paned, GTK_WIDGET ( base->dtv->vbox_sw_tv ) );
	gtk_paned_add2 ( paned, GTK_WIDGET ( base->dtv->video ) );

	return paned;
}
