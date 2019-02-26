/*
* Copyright 2018 - 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include "systems.h"

#include <stdio.h>

#include <dvbcsa/dvbcsa.h>


typedef struct _Systems Systems;

struct _Systems
{
	guchar cw[8];
};

static Systems systems;


void systems_get_key ( const gchar *prog_name, guint prog_num )
{
	guint n = 0;
	gboolean break_e = FALSE;
	gchar *contents;
	GError *err = NULL;

	gchar *cam_key = g_strconcat ( g_get_user_config_dir (), "/helia/cam.key", NULL );

	if ( g_file_get_contents ( cam_key, &contents, 0, &err ) )
	{
		gchar *prog_name_down = g_utf8_strdown ( prog_name, -1 );

		gchar **lines = g_strsplit ( contents, "\n", 0 );

		for ( n = 0; lines[n] != NULL; n++ )
		{
			if ( !g_strrstr ( lines[n], ";" ) ) continue;

			gchar *line = g_utf8_strdown ( lines[n], -1 );

			gchar **line_name = g_strsplit ( line, ";", 0 );

			if ( line_name[1] && g_str_has_prefix ( g_strstrip ( line_name[1] ), prog_name_down ) )
			{
				gchar **linef = g_strsplit ( line, " ", 0 );

				g_debug ( " line: %s", line );

				guint sid = 0;
				sscanf ( linef[1], "%4x ", &sid );

				g_debug ( " sid line: %d", sid );

				if ( sid == prog_num ) break_e = TRUE;

//

				guint i = 0, s = 8, cw[8];

				if ( strlen ( linef[3] ) == 16 || strlen ( linef[3] ) == 23 || strlen ( linef[3] ) == 39 )
				{
					g_debug ( "key: %s | len: %ld", linef[3], strlen ( linef[3] ) );

					if ( strlen ( linef[3] ) == 16 ) s = 2;
					if ( strlen ( linef[3] ) == 23 ) s = 3;
					if ( strlen ( linef[3] ) == 39 ) s = 5;

					for ( i = 0; i < 8; i++ ) 
					{ 
						if ( s < 5 ) 
							{ sscanf ( linef[3] + ( i * s ), ( s == 2 ) ? "%2x" : "%2x ", &cw[i] ); systems.cw[i] = cw[i]; }
						else
							{ sscanf ( linef[3] + ( i * s ), "0x%2x ", &cw[i] ); systems.cw[i] = cw[i]; }
					}
				}
				else
				{
					g_critical ( "systems_get_key: len != 8 | strlen: %ld | %s ", strlen ( linef[3] ), linef[3] );
				}

//

				g_strfreev ( linef );
			}

			g_strfreev ( line_name );
			g_free ( line );

			if ( break_e ) break;
		}
		
		g_strfreev ( lines );
		g_free ( prog_name_down );
		g_free ( contents );
		g_free ( cam_key );
	}
	else
	{
		g_critical ( "Cam key:: %s\n", err->message );
		g_error_free ( err );
	}
}

void systems_set_key ( guint8 *data, guint size )
{
	struct dvbcsa_key_s *key = dvbcsa_key_alloc ();

		dvbcsa_key_set ( systems.cw, key );

		dvbcsa_decrypt ( key, data, size );

	dvbcsa_key_free ( key );
}

