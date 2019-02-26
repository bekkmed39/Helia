/*
* Copyright 2018 - 2019 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include <base.h>

#include <glib/gi18n.h>


#define BAND_N 10
static gpointer dat_gfb[3][BAND_N];

static struct GstEQAudioData { const char *name; const char *desc; } eqa_n[] =
{
	{ N_("Level  dB"    ),  "gain"      },
	{ N_("Frequency  Hz"),  "freq"      },
	{ N_("Bandwidth  Hz"),  "bandwidth" }
};


static void helia_eqa_all_changed ( GtkRange *range, gpointer data, int ind )
{
	GstObject *band = GST_OBJECT ( data );
	gdouble value = gtk_range_get_value ( range );

	g_object_set ( band, eqa_n[ind].desc, value, NULL );
}

static void helia_eqa_gain_changed ( GtkRange *range, gpointer data )
{
	helia_eqa_all_changed ( range, data, 0 );
}
static void helia_eqa_freq_changed ( GtkRange *range, gpointer data )
{
	helia_eqa_all_changed ( range, data, 1 );
}
static void helia_eqa_band_changed ( GtkRange *range, gpointer data )
{
	helia_eqa_all_changed ( range, data, 2 );
}

static void helia_eqa_default ()
{
	gdouble g = 0.0, f = 30, b = 20;

	uint i = 0;
	for ( i = 0; i < BAND_N; i++ )
	{
		gtk_range_set_value ( GTK_RANGE ( (GtkRange *)dat_gfb[0][i] ), g );
		gtk_range_set_value ( GTK_RANGE ( (GtkRange *)dat_gfb[1][i] ), f );
		gtk_range_set_value ( GTK_RANGE ( (GtkRange *)dat_gfb[2][i] ), b );

		f = f * 2; b = b * 2;
	}
}

static void helia_eqa_clear ()
{
	helia_eqa_default ();
}

static void helia_eqa_create_label ( GtkBox *vbox )
{
	GtkBox *hboxl = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	GtkLabel *label;

	uint i = 0;
	for ( i = 0; i < G_N_ELEMENTS ( eqa_n ); i++ )
	{
		label = (GtkLabel *)gtk_label_new ( _(eqa_n[i].name) );
		gtk_box_pack_start ( hboxl, GTK_WIDGET ( label ), TRUE, TRUE, 10 );
	}

	gtk_box_pack_start ( vbox,  GTK_WIDGET ( hboxl  ), FALSE, FALSE,  0 );
}

static GtkScale * helia_eqa_create_scale_g_f_b ( GtkBox *scales_hbox, gdouble g_f_b, gdouble min_v, gdouble max_v, gdouble step_v )
{
	GtkScale *widget =  (GtkScale *)gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, min_v, max_v, step_v );
	gtk_range_set_value ( GTK_RANGE ( widget ), g_f_b );

	gtk_widget_set_size_request ( GTK_WIDGET ( widget ), 100, -1 );
	gtk_box_pack_start ( scales_hbox, GTK_WIDGET ( widget ), TRUE, TRUE, 5 );

	gtk_scale_set_draw_value ( GTK_SCALE ( widget ), 1 );

	return widget;
}

void helia_eqa_win ( GstElement *element, GtkWindow *parent, gdouble opacity )
{
	GtkBox *vbox_main, *vbox, *h_box;
	GtkScale *widget;

	GtkWindow *window_eq_audio = (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_transient_for ( window_eq_audio, parent );
	gtk_window_set_title    ( window_eq_audio, _("Audio equalizer") );
	gtk_window_set_modal    ( window_eq_audio, TRUE );
	gtk_window_set_position ( window_eq_audio, GTK_WIN_POS_CENTER_ON_PARENT );

	vbox_main = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_margin_top    ( GTK_WIDGET ( vbox_main ), 10 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( vbox_main ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( vbox_main ), 10 );

	vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

	helia_eqa_create_label ( vbox_main );

	g_object_set ( G_OBJECT ( element ), "num-bands", BAND_N, NULL );

	uint c = 0;
	for ( c = 0; c < BAND_N; c++ )
	{
		gdouble freq, bw, gain;
		GObject *band = gst_child_proxy_get_child_by_index ( GST_CHILD_PROXY ( element ), c );

		g_assert ( band != NULL );
		g_object_get ( band, "gain",      &gain, NULL );
		g_object_get ( band, "freq",      &freq, NULL );
		g_object_get ( band, "bandwidth", &bw,   NULL );

		GtkBox *scales_hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

		widget = helia_eqa_create_scale_g_f_b ( scales_hbox, gain, -24.0, 12.0, 1.0 );
		g_signal_connect ( G_OBJECT ( widget ), "value-changed", G_CALLBACK ( helia_eqa_gain_changed ), (gpointer)band );
		dat_gfb[0][c] = (gpointer)widget;

		widget = helia_eqa_create_scale_g_f_b ( scales_hbox, freq, 10.0, 20000.0, 5.0 );
		g_signal_connect ( G_OBJECT ( widget ), "value-changed", G_CALLBACK ( helia_eqa_freq_changed ), (gpointer)band );
		dat_gfb[1][c] = (gpointer)widget;

		widget = helia_eqa_create_scale_g_f_b ( scales_hbox, bw, 10.0, 20000.0, 10.0 );
		g_signal_connect ( G_OBJECT ( widget ), "value-changed", G_CALLBACK ( helia_eqa_band_changed ), (gpointer)band );
		dat_gfb[2][c] = (gpointer)widget;

		gtk_box_pack_start ( vbox, GTK_WIDGET ( scales_hbox ), TRUE, TRUE, 0 );
	}

	gtk_box_pack_start ( vbox_main, GTK_WIDGET ( vbox ), TRUE, TRUE, 0 );

	h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( h_box, 10 );

	GtkButton *button = base_set_image_button ( "helia-clear", 16 );
	g_signal_connect ( button, "clicked", G_CALLBACK ( helia_eqa_clear ), NULL );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );

	button = base_set_image_button ( "helia-exit", 16 );
	g_signal_connect_swapped ( button, "clicked", G_CALLBACK ( gtk_widget_destroy ), window_eq_audio );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );

	gtk_box_pack_start ( vbox_main, GTK_WIDGET ( h_box ), FALSE, FALSE, 10 );

	gtk_container_add ( GTK_CONTAINER ( window_eq_audio ), GTK_WIDGET ( vbox_main ) );
	gtk_container_set_border_width ( GTK_CONTAINER ( vbox_main ), 0 );

	gtk_widget_show_all ( GTK_WIDGET ( window_eq_audio ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( window_eq_audio ), opacity );
}
