// paint.h
#ifndef _PAINT_
#define _PAINT_

#include "types.h"
#include "dialog.h"

PangoLayout *layout;								// указатель на объект графического текста
PangoFontDescription *font_i, *font_b, *font_n;		// шрифты
cairo_surface_t *surface;							// контекст рисования миниатюры
cairo_t *cr;
bool painting;										// флажок рисования
int p_block_view_port_x, p_block_view_port_y;		// Координаты вывода подсказки размеров блока
byte *save_map;										// буфер сохранения мартицы при рисовании квадратами

byte contrast_color(byte indx);
void draw(GtkWidget *widget, GdkEventExpose *event);
void p_draw_matrix(cairo_t *cr);
void p_draw_icon_point(cairo_t *cr, int x, int y);
bool p_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data);
void p_redraw_digit(digitable *items, int indx, int x, int y);
void p_update_line(int numb/*номер строки*/, bool orient/*true - колонки, false - строки*/);
void p_paint_cell(cairo_t *cr, size_t ix, size_t iy);
void p_u_paint_cell(word indx);						// для utils-undo/redo
void p_digits_button_event(digitable *items, int indx, GdkEventButton *event);
void p_matrix_button_event(int indx, GdkEventButton *event);
void p_botton_release(GdkEventButton *event);
bool p_mouse_motion(GtkWidget *widget, GdkEventMotion *event);
void p_update_cursor(bool);
void p_set_mouse_cursor();
void p_move_cursor(direct);
void set_digit_value(int);
void p_edit_digits(bool);
void return_cursor();
void p_update_digits();
void p_update_matrix();
void p_draw_icon(bool);
void p_animate_cell(int, size_t, size_t, cairo_t *);
void p_make_row_imp(int, bool);
void p_make_column_imp(int, bool);
void p_mark_cell(int, int);
void p_mark_cell_in(byte, int, int);
#endif //_PAINT_
