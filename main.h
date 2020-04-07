// main.h
#ifndef _MAIN_
#define _MAIN_

#include "types.h"
#include "resource.h"

typedef enum param_tag{	ptoolbar,pstatusbar,ppalette,pgrid,pgrid5,panimate,
						plesson,viewname,help_lev,pfile} param;

static volatile bool tool_bar_lock = true;
static volatile bool toolbar_save_state[13];
static volatile bool menu_save_state[IDM_ABOUT+1];
GtkWidget** tb;				// кнопки тулбара
uint tb_count;				// количество кнопок тулбара

// процедуры ***********************************************************
void scrolled_viewport(double x, double y);
void enable_menu_item(int *ptr, int cnt, gboolean state);
void enable_toolbar_item(int *ptr, int cnt, gboolean state);
void enable_toolbar_animate(bool state);						// блокировка тулбара пока идет анимация
void set_status_bar(char *buff);
void clear_status_bar();
void toggle_changes(bool, pr);
void set_window_size(bool);
void save_parameter();
#endif //_MAIN_
