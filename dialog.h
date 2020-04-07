// dialog.h
#ifndef _DIALOG_
#define _DIALOG_

#include "types.h"
#include "resource.h"
#include "puzzle.h"
#include "paint.h"
#include "mafile.h"
#include <gdk/gdkkeysyms.h>

bool palette_show;
PangoFontDescription *font_c;

int  d_message				(char*, char*, int);
void d_about_dialog 		(void);
bool d_new_puzzle_dialog	(puzzle_features*, bool);
void d_palette_show			(bool show);
void d_create_palette		(void);
bool d_save_as_dialog		(void);
bool d_open_file_dialog		(void);
void d_invert_palette		(void);
char *d_find_puzzle_dialog	(uint);
void d_update_palette		(void);
int  d_view_list 			(size_t *nums, size_t count);

#endif //_DIALOG_
