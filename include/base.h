/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef BASE_H
#define BASE_H


#include <gtk/gtk.h>
#include <gst/gst.h>
#include <time.h>


#define MAX_WIN 4
#define MAX_AUDIO 32
#define MAX_RUN_PAT 128

enum cols_n { COL_NUM, COL_FL_CH, COL_DATA, NUM_COLS };


typedef struct _PatPmtSdtVct PatPmtSdtVct;

struct _PatPmtSdtVct
{
	uint pat_sid, pmt_sid, sdt_sid, vct_sid;
	uint pmt_vpid, pmt_apid;

	char *sdt_name, *vct_name;

	uint ca_sys, ca_pid;
};

typedef struct _MpegTs MpegTs;

struct _MpegTs
{
	gboolean pat_done,  pmt_done,  sdt_done,  vct_done;
	uint     pat_count, pmt_count, sdt_count, vct_count;

	PatPmtSdtVct ppsv[MAX_RUN_PAT];
};

typedef struct _Level Level;

struct _Level
{
	GtkLabel *signal_snr;
	GtkProgressBar *bar_sgn, *bar_snr;
};

typedef struct _Scan Scan;

struct _Scan
{
	GstElement *pipeline_scan, *scan_dvbsrc;

	GtkTreeView *treeview_scan;
	GtkLabel *dvb_device, *all_channels;

	GtkComboBoxText *combo_delsys;
	ulong desys_signal_id;

	uint adapter_set, frontend_set, delsys_set, lnb_type;

	Level level_scan;

	gboolean scan_quit;

	GtkEntry *entry_convert;

	MpegTs mpegts;

	time_t t_cur_scan, t_start_scan;
};

typedef struct _DigitalTV DigitalTV;

struct _DigitalTV
{
	GstElement *dvbplay, *e_audio;
	guintptr window_hid;

	GtkDrawingArea *video;

	GtkTreeModel *model;
	GtkTreeView *treeview;

	GtkBox *vbox_sw_tv, *h_box_level_base, *h_box_level_panel;
	GtkVolumeButton *volbutton;

	uint sid;
	double volume;
	gboolean checked_video, panel_quit;
	gboolean scrambling, rec_tv, rec_ses, rec_status, rec_pulse;

	char *ch_data;

	Level level_base, level_panel;

	Scan scan;

	char *audio_lang[MAX_AUDIO];

	uint count_audio_track, set_audio_track;
	GstPad *pad_a_sink[MAX_AUDIO], *pad_a_src[MAX_AUDIO];

	time_t t_cur_tv, t_start_tv;
};


typedef struct _Slider Slider;

struct _Slider
{
	GtkScale *slider;
	GtkLabel *lab_pos, *lab_dur;
	GtkLabel *rec_buf;

	ulong slider_signal_id;
};

typedef struct _Player Player;

struct _Player
{
	GstElement *playbin, *pipeline_rec;
	guintptr window_hid;

	GtkDrawingArea *video;

	GtkTreeModel *model;
	GtkTreeView *treeview;

	GtkBox *vbox_sw_mp, *h_box_slider_base, *h_box_slider_panel;
	GtkVolumeButton *volbutton;

	double volume;
	gboolean next_repeat, state_subtitle, panel_quit;
	gboolean record, rec_status;

	char *file_play;

	Slider slider_base, slider_panel;

	GtkLabel *label_audio, *label_video;

	time_t t_cur_mp, t_start_mp;
};


typedef struct _Base Base;

struct _Base
{
	GtkWindow *window;
	GdkPixbuf *pixbuf_mp, *pixbuf_tv;

	Player *player;

	DigitalTV *dtv;

	double opacity_panel, opacity_eq, opacity_win;

	uint num_lang, size_icon;

	gboolean info_quit, app_quit;

	char *helia_conf, *rec_dir, *ch_conf;
};


/* Returns a newly-allocated string holding the result. Free with free() */
char * base_uri_get_path ( const char *uri );

void base_set_win_base ( GtkButton *button, Base *base );

GtkImage  * base_create_image     ( const char *icon, uint size );
GtkButton * base_set_image_button ( const char *icon, uint size );
void base_create_image_button ( GtkBox *box, const char *icon, uint size, void (*f)(), Base *base );

void base_message_dialog ( const char *f_error, const char *file_or_info, GtkMessageType mesg_type, GtkWindow *window );


#endif /* BASE_H */
