/*
* Copyright 2019 Stepan Perun
* This program is free software.
* 
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include <base.h>


typedef struct _MsgIdStr MsgIdStr;

struct _MsgIdStr
{
	const char *msgid;
	const char *msgstr;
};

static MsgIdStr cs_CZ_msgidstr_n[] =
{
    { "Audio equalizer", 	"Audioekvalizér"	},
    { "Bandwidth Hz", 		"Šířka pásma Hz"    },
    { "Brightness", 		"Jas"     			},
    { "Channels", 			"Kanály"     		},
    { "Contrast", 			"Kontrast"     		},
    { "Files", 				"Soubory"     		},
    { "Frequency Hz", 		"Kmitočet Hz"     	},
    { "Hue", 				"Odstín"     		},
    { "Level dB", 			"Úroveň dB"     	},
    { "Saturation", 		"Sytost"     		},
    { "Scanner", 			"Skener"     		},	
    { "Undefined", 			"Nestanoveno"		},
    { "Video equalizer", 	"Videoekvalizér" 	}
};

static MsgIdStr nl_NL_msgidstr_n[] =
{
    { "Audio equalizer", 	"Audio-equalizer" 	},
    { "Bandwidth Hz", 		"Bandbreedte Hz"	},
    { "Brightness", 		"Helderheid"     	},
    { "Channels",		 	"Kanalen"     		},
    { "Contrast", 			"Contrast"     		},
    { "Files", 				"Bestanden"     	},
    { "Frequency Hz", 		"Frequentie Hz"     },
    { "Hue", 				"Tint"     			},
    { "Level dB", 			"Niveau dB"     	},
    { "Saturation", 		"Verzadiging"     	},
    { "Scanner", 			"Scanner"     		},
    { "Undefined", 			"Niet-opgegeven" 	},
    { "Video equalizer", 	"Video-equalizer"	}
};

static MsgIdStr ru_RU_msgidstr_n[] =
{
    { "Audio equalizer", 	"Аудио эквалайзер"  },
    { "Bandwidth Hz", 		"Полоса Hz"     	},
    { "Brightness", 		"Яркость"     		},
    { "Channels", 			"Каналы"     		},
    { "Contrast", 			"Контрастность"     },
    { "Files", 				"Файлы"     		},
    { "Frequency Hz", 		"Частота Hz"     	},
    { "Hue", 				"Оттенок"     		},
    { "Level dB", 			"Уровень dB"     	},
    { "Saturation", 		"Насыщенность"     	},
    { "Scanner", 			"Сканер"     		},
    { "Undefined", 			"Не определено"     },
    { "Video equalizer", 	"Видео эквалайзер"  },
};

static MsgIdStr uk_UA_msgidstr_n[] =
{
    { "Audio equalizer", 	"Аудіо еквалайзер"  },
    { "Bandwidth Hz", 		"Смуга Hz"     		},
    { "Brightness", 		"Яскравість"     	},
    { "Channels", 			"Канали"     		},
    { "Contrast", 			"Контраст"     		},
    { "Files", 				"Файли"     		},
    { "Frequency Hz", 		"Частота Hz"     	},
    { "Hue", 				"Відтінок"     		},
    { "Level dB", 			"Рівень dB"     	},
    { "Saturation", 		"Насиченість"     	},
    { "Scanner", 			"Сканер"     		},
    { "Undefined", 			"Не визначено"     	},
    { "Video equalizer", 	"Відео еквалайзер"  }
};

typedef struct _Langs Langs;

struct _Langs
{
	const char *lang_name;
	MsgIdStr *msgidstr;
	uint num;
};

static Langs langs_n[] =
{
    { "English",   NULL, 0 },
    { "Czech",     cs_CZ_msgidstr_n, G_N_ELEMENTS ( cs_CZ_msgidstr_n ) },
    { "Dutch",     nl_NL_msgidstr_n, G_N_ELEMENTS ( nl_NL_msgidstr_n ) },
    { "Russian",   ru_RU_msgidstr_n, G_N_ELEMENTS ( ru_RU_msgidstr_n ) },
    { "Ukrainian", uk_UA_msgidstr_n, G_N_ELEMENTS ( uk_UA_msgidstr_n ) }
};

const char * lang_set ( Base *base, const char *text )
{
	if ( base->num_lang == 0 ) return text;

	uint i = 0;

    for ( i = 0; i < langs_n[base->num_lang].num; i++ )
    {
		// g_print ( "lang_set: text %s - %s \n", text, langs_n[base->num_lang].msgidstr[i].msgstr );

		if ( g_str_has_prefix ( langs_n[base->num_lang].msgidstr[i].msgid, text ) ) return langs_n[base->num_lang].msgidstr[i].msgstr;
	}

	return text;
}

void lang_add_combo ( GtkComboBoxText *combo )
{
	uint i = 0;

    for ( i = 0; i < G_N_ELEMENTS ( langs_n ); i++ )
    {
		gtk_combo_box_text_append_text ( combo, langs_n[i].lang_name );
	}
}

uint lang_get_def ()
{
	const gchar *lang = g_getenv ( "LANG" );
	
	g_debug ( "lang_get_def: %s ", lang );

	uint res = 0, i = 0;

    for ( i = 0; i < G_N_ELEMENTS ( langs_n ); i++ )
    {
		if ( g_str_has_prefix ( lang, langs_n[i].lang_name ) ) return i;
	}

	return res;
}
