/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include <base.h>


void dtv_level_set_sgn_snr ( Base *base, Level level, gdouble sgl, gdouble snr, gboolean hlook )
{
	gtk_progress_bar_set_fraction ( level.bar_sgn, sgl/100 );
	gtk_progress_bar_set_fraction ( level.bar_snr, snr/100 );

	char *texta = g_strdup_printf ( "Sgn %d%s", (int)sgl, "%" );
	char *textb = g_strdup_printf ( "Snr %d%s", (int)snr, "%" );

	const char *format = NULL;

	gboolean play = TRUE;
	if ( GST_ELEMENT_CAST ( base->dtv->dvbplay )->current_state != GST_STATE_PLAYING ) play = FALSE;

	if ( hlook )
		format = "%s<span foreground=\"#00ff00\"> ◉ </span>%s<span foreground=\"#FF0000\"> %s</span> %s";
	else
		format = "%s<span foreground=\"#ff0000\"> ◉ </span>%s<span foreground=\"#FF0000\"> %s</span> %s";

	if ( !play )
		format = "%s<span foreground=\"#ff8000\"> ◉ </span>%s<span foreground=\"#FF0000\"> %s</span> %s";

	if ( sgl == 0 && snr == 0 )
		format = "%s<span foreground=\"#bfbfbf\"> ◉ </span>%s<span foreground=\"#FF0000\"> %s</span> %s";

	char *markup = g_markup_printf_escaped ( format, texta, textb, 
				   base->dtv->rec_ses ? base->dtv->rec_status ? " ◉ " : " ◌ " : "", 
				   base->dtv->scrambling ? play ? " 🔓 " : " 🔒 " : "" );

		gtk_label_set_markup ( level.signal_snr, markup );

	g_free ( markup );

	g_free ( texta );
	g_free ( textb );
}

static Level dtv_level_create ()
{
	Level level;

	level.signal_snr = (GtkLabel *)gtk_label_new ( "Signal  &  Quality" );

	level.bar_sgn = (GtkProgressBar *)gtk_progress_bar_new ();
	level.bar_snr = (GtkProgressBar *)gtk_progress_bar_new ();

	return level;
}

static GtkBox * dtv_level_box_create ( Level level )
{
	GtkBox *vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( vbox ), 5 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( vbox ), 5 );

	gtk_box_pack_start ( vbox, GTK_WIDGET ( level.signal_snr  ), FALSE, FALSE, 5 );
	gtk_box_pack_start ( vbox, GTK_WIDGET ( level.bar_sgn  	), FALSE, FALSE, 0 );
	gtk_box_pack_start ( vbox, GTK_WIDGET ( level.bar_snr  	), FALSE, FALSE, 3 );

	return vbox;
}

GtkBox * dtv_level_panel_create ( Base *base )
{
	base->dtv->level_panel = dtv_level_create ();

	GtkBox *vbox = dtv_level_box_create ( base->dtv->level_panel );

	return vbox;
}

GtkBox * dtv_level_base_create ( Base *base )
{
	base->dtv->level_base = dtv_level_create ();

	GtkBox *vbox = dtv_level_box_create ( base->dtv->level_base );

	return vbox;
}

GtkBox * dtv_level_base_scan ( Base *base )
{
	base->dtv->scan.level_scan = dtv_level_create ();

	GtkBox *vbox = dtv_level_box_create ( base->dtv->scan.level_scan );

	return vbox;
}
