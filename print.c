//#include <gtk/gtkunixprint.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <cairo.h>
#include <cairo-ps.h>
#include <cups/cups.h>
#include "paint.h"

#define WIDTH  595	//реально 575
#define HEIGHT 842	//реально 817
#define MM 2.833	// масштаб миллиметра

static GtkPrintSettings *settings = NULL;

static void request_page_setup (GtkPrintOperation *operation,
								GtkPrintContext *context,
								int page_nr,
								GtkPageSetup *setup){
	if (page_nr == 1){
		GtkPaperSize *a4_size = gtk_paper_size_new ("iso_a4");
		gtk_page_setup_set_orientation (setup, GTK_PAGE_ORIENTATION_LANDSCAPE);
		gtk_page_setup_set_paper_size (setup, a4_size);
		gtk_paper_size_free (a4_size);
	}
}

static void draw_page(	GtkPrintOperation	*operation,
				GtkPrintContext		*context,
				gint				page_nr,
				gpointer			user_data){

	// setup
//	GtkPageSetup* page = gtk_print_context_get_page_setup(context);

	char dig[4];
	byte num, color;
	double width, height, top, bottom, x_begin, y_begin, left, right, h_cut, w_cut, x, y, x_font, y_font, e;
	int i, j,indx, d,
	top_size = atom->top_dig_size,
	left_size = atom->left_dig_size,
	x_size = atom->x_size,
	y_size = atom->y_size;

	cairo_t *cr = gtk_print_context_get_cairo_context(context);
	gtk_print_context_get_hard_margins(context, &top, &bottom, &left, &right);
	width = gtk_print_context_get_width(context);
	height = gtk_print_context_get_height(context);

	if(atom->x_puzzle_size > atom->y_puzzle_size){	// ландшафтный режим
		cairo_rotate(cr, 1.57);
		h_cut = top*MM  + bottom*MM,
		w_cut = left*MM + right*MM;
		cell_space = MIN((width-w_cut)/(atom->y_puzzle_size+1),(height-h_cut)/(atom->x_puzzle_size+1)),
		x_begin = (height  - atom->x_puzzle_size * cell_space) / 2;
		y_begin = -((width  - atom->y_puzzle_size * cell_space) / 2) - atom->y_puzzle_size * cell_space;
	}
	else {											// портретный режим
		h_cut = top*MM  + bottom*MM,
		w_cut = left*MM + right*MM;
		cell_space = MIN((width-w_cut)/(atom->x_puzzle_size+1),(height-h_cut)/(atom->y_puzzle_size+1)),
		x_begin = (width  - atom->x_puzzle_size * cell_space) / 2,
		y_begin = top*MM;

	}

	cairo_select_font_face(cr, "sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, cell_space/3*2);

	for(i = 0; i < atom->x_size; i++)				// верхняя оцифровка
        for(j = 0; j < atom->top_dig_size; j++){
			indx = i*atom->top_dig_size+j;
			num = atom->top.digits[indx];
			if(num == 0) continue;
			x = x_begin + (left_size + i) * cell_space;
            y = y_begin + (top_size - j - 1) * cell_space;
			if(num < 10)	// если цифра в один знак
					x_font = x + (double)cell_space * (double)0.3;
			else 	x_font = x + (double)cell_space * (double)0.08;
            y_font = y + (double)cell_space * (double)0.22 + cell_space/2;
			if(atom->isColor){
				color = atom->top.colors[indx]&COL_MARK_MASK;
				SET_COLOR(color);
				cairo_rectangle(cr,x,y,cell_space,cell_space);
				cairo_fill(cr);
				SET_COLOR(contrast_color(color));
			}
			else SET_COLOR(BLACK);
			sprintf(dig,"%d",num);
			cairo_move_to(cr, x_font, y_font);
			cairo_show_text(cr, dig); // the text we got as a parameter
			}

	for(i = 0; i < atom->y_size; i++)					// левая оцифровка
        for(j = 0; j < atom->left_dig_size; j++){
            indx = i*atom->left_dig_size+j;
            num = atom->left.digits[indx];
			if(num == 0) continue;
            x = x_begin + (left_size - j -1) * cell_space;
            y = y_begin + (top_size + i) * cell_space;
            if(num < 10)
					x_font = x + (double)cell_space * (double)0.3;
			else 	x_font = x + (double)cell_space * (double)0.08;
            y_font = y + (double)cell_space * (double)0.22 + cell_space/2;
			if(atom->isColor){
				color = atom->left.colors[indx]&COL_MARK_MASK;
				SET_COLOR(color);
				cairo_rectangle(cr,x,y,cell_space,cell_space);
				cairo_fill(cr);
				SET_COLOR(contrast_color(color));
			}
			else SET_COLOR(BLACK);
			sprintf(dig,"%d",num);
			cairo_move_to(cr, x_font, y_font);
			cairo_show_text(cr, dig); // the text we got as a parameter
		}

	cairo_set_line_width(cr,1);							// отрисовка сетки

	SET_COLOR(BLACK);									// рамка
	cairo_move_to(cr, x_begin, y_begin);	// левая граничная линия
	cairo_line_to(cr, x_begin, cell_space*(y_size+top_size)+y_begin);
	cairo_move_to(cr, x_begin, y_begin);	// верхняя линия
	cairo_line_to(cr, x_begin+cell_space*(x_size+left_size), y_begin);
	cairo_stroke(cr);

	//отрисовка вертикальных линий
	SET_COLOR(LINES);	// левая оцифровка
	if(parameter[3]){
		for(i = 1; i < left_size; i++){
			cairo_move_to(cr, i * cell_space+x_begin,
						cell_space * top_size + y_begin);
			cairo_line_to(cr, i * cell_space + x_begin,
				cell_space * (y_size + top_size) + y_begin);
			cairo_stroke(cr);
		}
	}
	for(i = 0; i <= x_size; i++){
		// последняя линия рабочей матрицы
		d = left_size + i;
		if(i == x_size){
			SET_COLOR(BLACK);
			e = d * cell_space + x_begin;
			cairo_move_to(cr, e, y_begin);
			cairo_line_to(cr, e, cell_space *
						(y_size+top_size)+y_begin);
			cairo_stroke(cr);
			continue;
		}

		// сетка
		if(parameter[4] && !(i % 5)) continue;
		SET_COLOR(LINES);
		if(parameter[3]){
			e = d * cell_space + x_begin;
			cairo_move_to(cr, e, y_begin);
			cairo_line_to(cr, e, cell_space * (y_size + top_size) + y_begin);
			cairo_stroke(cr);
		}
	}

	//отрисовка горизонтальных линий
	SET_COLOR(LINES);	// верхняя оцифровка
	if(parameter[3]){
		for(i = 1; i < top_size; i++){
			e = y_begin + i * cell_space;
			cairo_move_to(cr, x_begin + left_size * cell_space, e);
			cairo_line_to(cr, cell_space * (x_size + left_size) + x_begin, e);
			cairo_stroke(cr);
		}
	}
	for(i = 0; i <= y_size; i++){
		d = top_size + i;
		// последняя линия
		if(i == y_size){
			SET_COLOR(BLACK);
			e = d * cell_space + y_begin;
			cairo_move_to(cr, x_begin, e);
			cairo_line_to(cr, cell_space * (x_size + left_size)	+ x_begin, e);
			cairo_stroke(cr);
			continue;
		}
		// решетка
		if(parameter[4] && !(i % 5)) continue;
		SET_COLOR(LINES);
		if(parameter[3]){
			e = y_begin + d * cell_space;
			cairo_move_to(cr, x_begin, e);
			cairo_line_to(cr, cell_space * (x_size + left_size) + x_begin, e);
			cairo_stroke(cr);
		}
	}

	SET_COLOR(BLACK);	// граница оцифровки и матрицы
	e = left_size * cell_space + x_begin;
	cairo_move_to(cr, e, y_begin);
	cairo_line_to(cr, e, cell_space * (y_size+top_size)+y_begin);
	e = top_size * cell_space + y_begin;
	cairo_move_to(cr, x_begin, e);
	cairo_line_to(cr, cell_space * (x_size + left_size) + x_begin, e);
	cairo_stroke(cr);

	// решетка 5х5
	if(	parameter[4] ){
		SET_COLOR(BOLD);
		for(i = 5; i < x_size; i+=5){
			e = (left_size + i) * cell_space + x_begin;
			cairo_move_to(cr, e, y_begin);
			cairo_line_to(cr, e, cell_space * (y_size + top_size) + y_begin);
			cairo_stroke(cr);
		}
		for(i = 5; i < y_size; i+=5){
			e = y_begin + (top_size + i) * cell_space;
			cairo_move_to(cr, x_begin, e);
			cairo_line_to(cr, cell_space * (x_size + left_size) + x_begin, e);
			cairo_stroke(cr);
		}
	}
	// finish up
	cairo_show_page(cr);
	return;
}

void print_page(GtkWidget* main_window){
	GtkPrintOperation *print;
	GtkPrintOperationResult res;

	print = gtk_print_operation_new ();
	if (settings != NULL)
		gtk_print_operation_set_print_settings (print, settings);
	gtk_print_operation_set_n_pages (print, 1);
	g_signal_connect (print, "draw_page", G_CALLBACK (draw_page), NULL);
	g_signal_connect (print, "request_page_setup", G_CALLBACK (request_page_setup), NULL);
	res = gtk_print_operation_run (print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
											GTK_WINDOW (window), NULL);
	if (res == GTK_PRINT_OPERATION_RESULT_APPLY){
		if (settings != NULL) g_object_unref (settings);
		settings = g_object_ref (gtk_print_operation_get_print_settings (print));
	}
	g_object_unref (print);
	gtk_widget_queue_draw(draw_area);
}

