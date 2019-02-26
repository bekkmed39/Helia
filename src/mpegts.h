/*
* Copyright 2019 Stepan Perun
* This program is free software.
*
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef MPEGTS_H
#define MPEGTS_H


void mpegts_initialize ();

void mpegts_clear ( Base *base );

void mpegts_parse_section ( GstMessage *message, Base *base );

void mpegts_pmt_lang_section ( GstMessage *message, Base *base );


#endif // MPEGTS_H
