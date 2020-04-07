// Paint.c - прорисовка экрана
#include "main.h"
#include "paint.h"
#include "puzzle.h"
#include "utils.h"

#include <cairo.h>
#include <gdk/gdkkeysyms.h>

const int space = 1;

#define DMARK	2
#define CRECT cr,x,y,cell_space,cell_space
#define RECT cr,x+space,y+space,cell_space-space,cell_space-space	// режим зачеркивания
#define DRAW_SLASH cairo_move_to(cr,x+space+1.5,y+space+1.5);cairo_line_to(cr,x+cell_space-space-0.5,y+cell_space-space-0.5)

int fsize;										// размер шрифта
cairo_t *cr;
bool p_make_cell(uint, byte);
double last_mouse_x, last_mouse_y;					// позиция курсора мыши для скроллинга ViewPort

//*********** процедура поиска цвета контрастного заданному ************//
byte contrast_color(byte indx){	// поиск контрастного цвета
    if(indx == MARK) return BLUE;
    int fi =30*(atom->rgb_color_table[indx].r)*(atom->rgb_color_table[indx].r)+	//30% * red * red
			59*(atom->rgb_color_table[indx].g)*(atom->rgb_color_table[indx].g)+	//59% * green * green
			11*(atom->rgb_color_table[indx].b)*(atom->rgb_color_table[indx].b);	//11% * blue * blue
	if(fi > 1638400)	return BLACK;
	else 				return WHITE;
}

//**********************************************************************//
//********************* Процедуры прорисовки ***************************//
//**********************************************************************//
void p_paint_cell(cairo_t *cr, size_t ix, size_t iy){	//рисую ячейку
	float x = atom->x_begin + (atom->left_dig_size + ix) * cell_space + space;
	float y = atom->y_begin + (atom->top_dig_size + iy) * cell_space + space;
	int indx = iy * atom->x_size + ix;
	float size = cell_space - space;
	float dw, dh, ax, ay;
	if(!parameter[plesson] || (!x_error[ix] && !y_error[iy]))
			SET_COLOR(BACKGROUND);
	else	SET_COLOR(HIDE);
	byte color = atom->matrix[indx], hcolor = solved_map[indx];
	// ошибка если маркируется определённо закрашенная ячейка
	bool err = color == MARK && color!=EMPTY && hcolor!=MARK;
	// или закрашивается определённо маркированная ячейка
	err |= color != MARK && color != EMPTY && hcolor==MARK;
	// или цвет не соответствует определённому
	err |= color != MARK && color != EMPTY && hcolor!=MARK && hcolor!=EMPTY && color !=  hcolor;
	bool help = parameter[plesson] && c_solve_buffers && !demo_flag && puzzle_status!=bad;

	if(color == MARK){
		cairo_rectangle(cr,x,y,size,size);
		cairo_fill(cr);
		if(help && err) SET_COLOR(BLUE);
		else SET_SYS_COLOR(CROSS);					// и рисую в ней крестик
		cairo_set_line_width(cr,1);
		cairo_move_to(cr, x + 0.5, y + 0.5);
		cairo_line_to(cr, x + size - 0.5, y + size - 0.5);
		cairo_move_to(cr, x + 0.5, y + size - 0.5);
		cairo_line_to(cr, x + size - 0.5, y + 0.5);
		cairo_stroke(cr);
	}
	else if(help && color==EMPTY && shadow_map[indx]!=EMPTY){
		cairo_rectangle(cr,x,y,size,size);
		cairo_fill(cr);
		p_mark_cell_in(hcolor, x+cell_space/2, y+cell_space/2);
	}
	else {
		if(color!= ZERO) SET_COLOR(color);
		if(parameter[pgrid])
			cairo_rectangle(cr,x,y,size,size);
		else if(parameter[pgrid5]){
			dw = cell_space; dh = cell_space;
			ax = x-space; ay = y-space;
			if(!(ix % 5)) {ax = x; dw=cell_space-space;}
			if(!(iy % 5)) {ay = y; dh=cell_space-space;}
			cairo_rectangle(cr, ax, ay, dw, dh);
		}
		else {
			dw = cell_space; dh = cell_space;
			ax = x-space; ay = y-space;
			if(!ix){dw-=space; ax+=space;}
			if(!iy){dh-=space; ay+=space;}
			cairo_rectangle(cr, ax, ay, dw, dh);
		}
		cairo_fill(cr);
		if(help && err){
			cairo_fill(cr);
			x+=cell_space/2; y+=cell_space/2;
			p_mark_cell_in(hcolor, x, y);
		}
	}
}

void p_draw_matrix(cairo_t *cr){							// отрисовка элементов матрицы
	// рисую содержимое матрицы
	int ax, ay, dw, dh, indx;
	byte color, hcolor;
	int s_x = atom->x_begin + atom->left_dig_size * cell_space;
	int s_y = atom->y_begin + atom->top_dig_size * cell_space;
	int size = cell_space-space;
	bool help = parameter[plesson] && c_solve_buffers && !demo_flag && puzzle_status!=bad;
	for(int y=0; y<atom->y_size; y++)
		for(int x=0; x<atom->x_size; x++){
			indx = y * atom->x_size + x;
			color = atom->matrix[indx]; hcolor = solved_map[indx];
			ax = s_x+x*cell_space; ay = s_y+y*cell_space;
			if(!parameter[plesson] || (!x_error[x] && !y_error[y]))
					SET_SYS_COLOR(BACKGROUND);
			else	SET_SYS_COLOR(HIDE);
			// ошибка если маркируется определённо закрашенная ячейка
			bool err = color == MARK && color!=EMPTY && hcolor!=MARK;
			// или закрашивается определённо маркированная ячейка
			err |= color != MARK && color != EMPTY && hcolor==MARK;
			// или цвет не соответствует определённому
			err |= color != MARK && color != EMPTY && hcolor!=MARK && hcolor!=EMPTY && color !=  hcolor;

			if(color == MARK){
				ax+=space; ay+=space;
				cairo_rectangle(cr,ax,ay,size,size);
				cairo_fill(cr);
				if(help && err) SET_SYS_COLOR(BLUE);
				else SET_SYS_COLOR(CROSS);					// и рисую в ней крестик
				cairo_set_line_width(cr,1);
				cairo_move_to(cr, ax + 0.5, ay + 0.5);
				cairo_line_to(cr, ax + size - 0.5, ay + size - 0.5);
				cairo_move_to(cr, ax + 0.5, ay + size - 0.5);
				cairo_line_to(cr, ax + size - 0.5, ay + 0.5);
				cairo_stroke(cr);
			}
			else if(help && color==EMPTY && shadow_map[indx]!=EMPTY){
				ax+=space; ay+=space;
				cairo_rectangle(cr,ax,ay,size,size);
				cairo_fill(cr);
				p_mark_cell_in(hcolor, ax+cell_space/2, ay+cell_space/2);
			}
			else {
				if(color != ZERO) SET_COLOR(color);
				if(parameter[pgrid])
					cairo_rectangle(cr,ax+space,ay+space,size,size);
				else if(parameter[pgrid5]){
					dw = cell_space; dh = cell_space;
					if(!(x % 5)) {ax += space; dw-=space;}
					if(!(y % 5)) {ay += space; dh-=space;}
					cairo_rectangle(cr, ax, ay, dw, dh);
				}
				else {
					dw = cell_space; dh = cell_space;
					if(!x){dw-=space; ax+=space;}
					if(!y){dh-=space; ay+=space;}
					cairo_rectangle(cr, ax, ay, dw, dh);
				}
				cairo_fill(cr);
				if(help && err){
					ax+=cell_space/2+space; ay+=cell_space/2+space;
					p_mark_cell_in(hcolor, ax, ay);
				}
			}
		}
}

void p_update_matrix(){
	CAIRO_INIT;
	p_draw_matrix(cr);
}

void p_u_paint_cell(word indx){
	size_t x=indx%atom->x_size, y=indx/atom->x_size;
	p_paint_cell(cr, x, y);
	p_draw_icon_point(cr, x, y);
//	p_draw_icon(false);
}

void p_animate_cell(int phase, size_t ix, size_t iy, cairo_t *cr){	//рисую ячейку
	int x = atom->x_begin + (atom->left_dig_size + ix) * cell_space + space;
	int y = atom->y_begin + (atom->top_dig_size + iy) * cell_space + space;
	int indx = iy * atom->x_size + ix;
	int size = cell_space - space;
	float r, g, b;

	if(atom->matrix[indx] == MARK){
		SET_SYS_COLOR(BACKGROUND);
		cairo_rectangle(cr,x,y,size,size);							// очищаю клетку
		cairo_fill(cr);
		r = (float)0.75 + (float)0.75 * ((float)phase / (float)anim_amp) ;
		cairo_set_source_rgb(cr, r, r, r);
		cairo_set_line_width(cr,1);									// и рисую в ней крестик
		cairo_move_to(cr, x + 0.5, y + 0.5);
		cairo_line_to(cr, x + size - 0.5, y + size - 0.5);
		cairo_move_to(cr, x + 0.5, y + size - 0.5);
		cairo_line_to(cr, x + size - 0.5, y + 0.5);
		cairo_stroke(cr);
		return;
	}
	else if(atom->matrix[indx] != EMPTY){
		float percent = (float)phase / (float)anim_amp ; // 1 > 0
		r = (float)atom->rgb_color_table[atom->matrix[indx]].r / (float)0xff;
		r += (1-r) * percent;
		g = (float)atom->rgb_color_table[atom->matrix[indx]].g / (float)0xff;
		g += (1-g) * percent;
		b = (float)atom->rgb_color_table[atom->matrix[indx]].b / (float)0xff;
		b += (1-b) * percent;
		cairo_set_source_rgb(cr, r, g, b);
	}
	if(parameter[pgrid])
		cairo_rectangle(cr,x,y,size,size);
	else if(parameter[pgrid5]){
		int dw = cell_space, dh = cell_space,
		ax = x-space, ay = y-space;
		if((ix % 5) == 4) dw-=space;
		if((iy % 5) == 4) dh-=space;
		if(!(ix % 5)) {ax = x; dw=cell_space-space;}
		if(!(iy % 5)) {ay = y; dh=cell_space-space;}
		cairo_rectangle(cr, ax, ay, dw, dh);
	}
	else
		cairo_rectangle(cr, x-space, y-space, cell_space, cell_space);
	cairo_fill(cr);
}

void p_update_line(int numb, bool orient/*true - колонки, false - строки*/){
    CAIRO_INIT;
	if(orient){											// перерисовываю столбец
		for(int y=0; y<atom->y_size; y++)
			p_paint_cell(cr, numb, y);
	}
	else {												// перерисовываю строку
		for(int x=0; x<atom->x_size; x++)
			p_paint_cell(cr, x, numb);
	}
}

void p_edit_digits(bool sets){
	if(sets && !atom->curs->status){
//		u_before(digits);
		atom->curs->status=true;
		p_update_digits();
		p_update_cursor(true);
		m_set_title();
	}
	else if(!sets && atom->curs->status){
		u_digits_change_event(true, true);
		atom->curs->status=false;
		p_update_digits();
		c_check_digits_summ();
		m_set_title();
		u_after(digits);
	}
}

void p_set_mouse_cursor(){
		// проверка на вхождение в оцифровку слева
	if(	atom->x_cell <	atom->left_dig_size &&
		atom->y_cell >=	atom->top_dig_size &&
		atom->y_cell <	atom->y_puzzle_size){

		atom->curs->digits	= left_dig;
		atom->curs->x		= atom->x_cell;
		atom->curs->y		= atom->y_cell-atom->top_dig_size;
	}	// сверху
	if( atom->x_cell >=	atom->left_dig_size &&
		atom->x_cell <	atom->x_puzzle_size &&
		atom->y_cell <	atom->top_dig_size){

		atom->curs->digits	= top_dig;
		atom->curs->x		= atom->x_cell-atom->left_dig_size;
		atom->curs->y		= atom->y_cell;
	}
	p_update_digits();
}

void return_cursor(){
	int	max=(atom->curs->digits==top_dig ? atom->y_size : atom->x_size);
	if(atom->curs->pointer->digits[atom->curs->indx] > max){
		atom->curs->pointer->digits[atom->curs->indx] = 0;
		set_status_bar(_("Error! Too big digit."));
	}
	if(atom->curs->pointer->digits[atom->curs->indx] != 0){
		if(atom->curs->digits == left_dig) p_move_cursor(right);
		else p_move_cursor(down);
	}
	else{
		p_update_cursor(false);
		if(atom->curs->digits == top_dig){
			if(atom->curs->x < atom->x_size-1)atom->curs->x++;
			atom->curs->y=0;
		}
		else{
			if(atom->curs->y < atom->y_size-1)atom->curs->y++;
			atom->curs->x=0;
		}
		u_digits_change_event(true, true);
		p_update_cursor(true);
	}
}

void p_move_cursor(direct dir){
	p_update_cursor(false);
	if(atom->curs->digits == top_dig){	// курсор сверху
		switch(dir){
		case left:
			if(atom->curs->x > 0)	atom->curs->x--;
			else{					atom->curs->x=atom->x_size-1;
				if(atom->curs->y>0)	atom->curs->y--;
				else 				atom->curs->y=atom->top_dig_size-1;
			}
			break;
		case right:
			atom->curs->x++;
			if(atom->curs->x >= atom->x_size){
									atom->curs->x=0;
									atom->curs->y++;
				if(atom->curs->y >= atom->top_dig_size)	atom->curs->y=0;
			}
			break;
		case up:
			if(atom->curs->y > 0)	atom->curs->y--;
			else{					atom->curs->y=atom->top_dig_size-1;
				if(atom->curs->x>0)	atom->curs->x--;
				else				atom->curs->x=atom->x_size-1;
			}
			break;
		case down:
			atom->curs->y++;
			if(atom->curs->y >= atom->top_dig_size){ atom->curs->y=0;
									atom->curs->x++;
				if(atom->curs->x >= atom->x_size) atom->curs->x=0;
			}
			break;
		}
	}
	else {								// курсор слева
		switch(dir){
		case left:
			if(atom->curs->x > 0)	atom->curs->x--;
			else{					atom->curs->x=atom->left_dig_size-1;
				if(atom->curs->y>0)	atom->curs->y--;
				else 				atom->curs->y=atom->y_size-1;
			}
			break;
		case right:
			atom->curs->x++;
			if(atom->curs->x >= atom->left_dig_size){ atom->curs->x=0;
									atom->curs->y++;
				if(atom->curs->y >= atom->y_size) atom->curs->y=0;
			}
			break;
		case up:
			if(atom->curs->y > 0)	atom->curs->y--;
			else{					atom->curs->y=atom->y_size-1;
				if(atom->curs->x>0)	atom->curs->x--;
				else				atom->curs->x=atom->left_dig_size-1;
			}
			break;
		case down:
			atom->curs->y++;
			if(atom->curs->y >= atom->y_size){ atom->curs->y=0;
									atom->curs->x++;
				if(atom->curs->x >= atom->left_dig_size) atom->curs->x=0;
			}
			break;
		}

	}
	p_update_cursor(true);
}

void set_digit_value(int val){	// Values: -1 delete, -2/-3 inc/dec, 0-10 conveer-set
	static uint count = 0;
	byte *ptr=atom->curs->pointer->digits;
	int indx=atom->curs->indx;
	int	max=(atom->curs->digits==top_dig ? atom->y_size : atom->x_size);
	if(count == 1){
		if(val > max) return;
		ptr[indx] = val;
		count++;
	}
	else if(count > 1){
		if((ptr[indx] * 10 + val) > max){
			ptr[indx] = 0;
			p_update_cursor(true);
			count = 1;
			return;
		}
		ptr[indx] *= 10;
		ptr[indx] += val;
		count = 0;
		return_cursor();
	}
	else if(val == 10){
		ptr[indx] = 0;
		count = 1;
	}
	else if(val == -1) {
		ptr[indx] = 0;			// обнуляю цифру ячейки
		atom->curs->pointer->colors[indx]=WHITE;
	}
	else if(val >= 0 && val <= 9){	// конвеерно с клавиатуры устанавливаю значение
		ptr[indx]=val;
		return_cursor();
	}
	else if(val < 1){			// инкремент-декремент при вводе прокруткой колеса
		ptr=atom->curs->pointer->digits;
		indx=atom->curs->indx;
		if(val == -3){
			if(ptr[indx] != 0)	ptr[indx]--;
			else 				ptr[indx]=max;
		}
		else{
			ptr[indx]++;
			if(ptr[indx] > max) ptr[indx]=0;
		}
	}
	p_update_cursor(true);
//	if(ptr[indx] != save) toggle_changes(true, digits);
}

void p_update_cursor(bool sets){
	int indx, x_font, y_font;
	int x = atom->curs->x, y = atom->curs->y;
	byte digit, color;
	char dig[4];
	if(!atom->curs->status && !sets) return;
	if(sets){	// расчитываю и сохраняю координаты
		atom->curs->pointer = (atom->curs->digits == left_dig ? &atom->left : &atom->top);
		atom->curs->indx = (atom->curs->digits == left_dig ?\
		(y+1)*atom->left_dig_size-x-1 :	(x+1)*atom->top_dig_size-y-1);
	}
	indx	= atom->curs->indx;
	digit	= atom->curs->pointer->digits[indx];
	x = atom->x_begin+atom->curs->x*cell_space+(atom->curs->digits==top_dig ?\
												atom->left_dig_size*cell_space : 0);
	y = atom->y_begin+atom->curs->y*cell_space+(atom->curs->digits==left_dig ?\
												atom->top_dig_size*cell_space : 0);

	CAIRO_INIT;
	if(atom->isColor) color = atom->curs->pointer->colors[indx] & COL_MARK_MASK;
	else color = WHITE;
	SET_COLOR(color);

	cairo_rectangle(RECT);
	cairo_fill(cr);							// закрашиваю фон

		// определяю отступы от верхнего-левого угла клетки
	if(digit < 10)
			x_font = x + (double)cell_space * (double)0.3;
	else 	x_font = x + (double)cell_space * (double)0.1;
	y_font = y + (double)cell_space * (double)0.2;

	SET_COLOR(contrast_color(color));
	if(sets)
		if(digit < 10)	pango_layout_set_font_description(layout,font_b);
		else			pango_layout_set_font_description(layout,font_i);
	else				pango_layout_set_font_description(layout,font_n);

	if(sets || digit!=0){
		// перерисовываю циферь
		sprintf(dig, "%d", digit);
		//установка текста
		pango_layout_set_text(layout, dig, strlen(dig));
		//перемещение курсора контекста рисования
		cairo_move_to(cr, x_font, y_font);
		//отрисовка текста
		pango_cairo_show_layout(cr,layout);
	}
}

void p_redraw_digit(digitable *items, int indx, int x, int y){			// перерисовываю цифру оцифровки
	char dig[4];
	int x_font, y_font;
	byte 	digit	= items->digits[indx],
			mark	= (items->colors[indx] & COLOR_MARK),
			color;
	if(digit == 0) return;

	CAIRO_INIT;
	if(atom->isColor) color = items->colors[indx] & COL_MARK_MASK;
	else color = WHITE;
	SET_COLOR(color);
	cairo_rectangle(RECT);
	cairo_fill(cr);
	SET_COLOR(contrast_color(color));

	// определяю отступы от верхнего-левого угла клетки
	if(digit < 10)
			x_font = x + (double)cell_space * (double)0.3;
	else 	x_font = x + (double)cell_space * (double)0.1;
			y_font = y + (double)cell_space * (double)0.2;

 	sprintf(dig, "%d", digit);							// перерисовываю циферь
	pango_layout_set_text(layout, dig, strlen(dig));	//установка текста
	cairo_move_to(cr, x_font, y_font);					//перемещение курсора контекста рисования
	pango_cairo_show_layout(cr,layout);					//отрисовка текста
	if (mark != 0){										// зачеркиваю циферь если надо
		cairo_set_line_width(cr,DMARK);
		DRAW_SLASH;
		cairo_stroke(cr);
	}
}

void p_digits_button_event(digitable *items, int indx, GdkEventButton *event){
	if(timer_id) return;
	if (event->button == 1){
		if(atom->curs->status){
			if(atom->isColor)
				atom->curs->pointer->colors[indx]=current_color;
			p_update_cursor(false);
			p_set_mouse_cursor();
			p_update_cursor(true);
		}
		else{
			if(atom->isColor){
				current_color = items->colors[indx]&COL_MARK_MASK;
				if(palette_show)
					gtk_widget_queue_draw(palette);
			}
			else current_color = BLACK;
			}
	}
	else if (event->button == 2){
		if(!atom->curs->status) p_edit_digits(true);
		p_update_cursor(false);
		p_set_mouse_cursor();
		set_digit_value(-1);
	}
	if (event->button == 3 || event->type == GDK_2BUTTON_PRESS){
		if(items->digits[indx] == 0){
			p_set_mouse_cursor();
			p_edit_digits(true);
		}
		else{
			items->colors[indx] ^= COLOR_MARK;	// инвертирую маркер
			p_redraw_digit(items, indx,
				atom->x_begin + atom->x_cell * cell_space, 	// определяю координаты клетки
				atom->y_begin + atom->y_cell * cell_space);
		}
	}
}

void p_make_column_imp(int column, bool invert){// прорисовываю выделенные курсором колонки оцифровки
	if(atom->curs->status || !layout) return;	// выход если идет редактирование оцифровки
	CAIRO_INIT;
	int x_space = cell_space, y_space = cell_space;
	int x_begin = atom->x_begin, y_begin = atom->y_begin;
	int l_s	= atom->left_dig_size,
		t_s	= atom->top_dig_size;
	int indx;
	byte	*t_d = atom->top.digits;
	byte	digit, mark, color;
	int x, y, x_font, y_font, i;
	char dig[4];

	// цикл перерисовки цифр столбца
	for(i=0; i<t_s; i++){
		indx = column * t_s + t_s - i - 1;
		digit	= t_d[indx];
		mark	= atom->top.colors[indx] & COLOR_MARK;

		// расчитываю прямоугольник заливки
		x = x_begin + x_space * (column+l_s);
		y = y_begin + i * y_space;

		if(digit==0 && invert) color = MARKER;	// устанавливаю цвет фона
		else if(atom->isColor) color = atom->top.colors[indx] & COL_MARK_MASK;
		else color = WHITE;
		SET_COLOR(color);
		cairo_rectangle(RECT);
		cairo_fill(cr);

		if(digit == 0) continue;

		// определяю отступы от верхнего-левого угла клетки
		if(digit < 10)
				x_font = x + (double)cell_space * (double)0.3;
		else 	x_font = x + (double)cell_space * (double)0.1;
				y_font = y + (double)cell_space * (double)0.2;

		SET_COLOR(contrast_color(color));
		if(invert)
			if(digit < 10)	pango_layout_set_font_description(layout,font_b);
			else			pango_layout_set_font_description(layout,font_i);
		else				pango_layout_set_font_description(layout,font_n);

		// перерисовываю циферь
		sprintf(dig, "%d", digit);
		pango_layout_set_text(layout, dig, strlen(dig));
		cairo_move_to(cr, x_font, y_font);
		pango_cairo_show_layout(cr,layout);
		// зачеркиваю циферь если надо
		if (mark != 0){
			cairo_set_line_width(cr,DMARK);
			DRAW_SLASH;
			cairo_stroke(cr);
		}
	}
}

void p_make_column(int x){
	p_make_column_imp(x, atom->top_col != x);
}

void p_make_row_imp(int row, bool invert){		// прорисовываю выделенные курсором ряды оцифровки
	CAIRO_INIT;
	if(atom->curs->status || !layout) return;	// выход если идет редактирование оцифровки
	int x_space = cell_space, y_space = cell_space;
	int x_begin = atom->x_begin, y_begin = atom->y_begin;
	int left_dig_size	= atom->left_dig_size,
		top_dig_size	= atom->top_dig_size;
	int indx;
	byte	*dig_left = atom->left.digits/*,
			*mark_left= atom->left.digits_marker*/;
	byte	digit, mark, color;
	int x, y, x_font, y_font, i;
	char dig[4];

	// цикл перерисовки цифр ряда
	for(i=0; i<left_dig_size; i++){
		indx = row * left_dig_size + left_dig_size - i - 1;
		digit	= dig_left [indx];
		mark	= atom->left.colors[indx] & COLOR_MARK;

		// расчитываю прямоугольник заливки
		x = x_begin + x_space * i;
		y = y_begin + y_space * (row + top_dig_size);

		if(digit==0 && invert) color = MARKER;	// устанавливаю цвет фона
		else if(atom->isColor) color = atom->left.colors[indx] & COL_MARK_MASK;
		else color = WHITE;
		SET_COLOR(color);
		cairo_rectangle(RECT);
		cairo_fill(cr);

		if(digit == 0) continue;
				// определяю отступы от верхнего-левого угла клетки
		if(digit < 10)
				x_font = x + (double)cell_space * (double)0.3;
		else 	x_font = x + (double)cell_space * (double)0.1;
				y_font = y + (double)cell_space * (double)0.2;
												//установка шрифта текста
		SET_COLOR(contrast_color(color));
		if(invert)
			if(digit < 10)	pango_layout_set_font_description(layout,font_b);
			else			pango_layout_set_font_description(layout,font_i);
		else				pango_layout_set_font_description(layout,font_n);

		sprintf(dig, "%d", digit);
		pango_layout_set_text(layout, dig, strlen(dig));
		cairo_move_to(cr, x_font, y_font);
		pango_cairo_show_layout(cr,layout);
		if (mark != 0){
			cairo_set_line_width(cr,DMARK);
			DRAW_SLASH;
			cairo_stroke(cr);
		}
	}
}

void p_make_row(int y){
	p_make_row_imp(y, atom->left_row != y);
}

void p_draw_block_len(int x_cell, int y_cell){
	uint size_y = 1, size_x = 1, xs = atom->x_size, ys = atom->y_size;
	int i, x1, x2, y1, y2;
	byte *p = atom->matrix;
	// восстанавливаю содержимое ячеек
	if(p_block_view_port_x >=0){
		x1 = p_block_view_port_x ? p_block_view_port_x - 1 : 1;
		y1 = p_block_view_port_y;
		x2 = p_block_view_port_x;
		y2 = p_block_view_port_y ? p_block_view_port_y - 1 : 1;
		p_paint_cell(cr, x1, y1);
		p_paint_cell(cr, x2, y2);
	}
	p_block_view_port_x = x_cell;
	p_block_view_port_y = y_cell;
	x1 = p_block_view_port_x ? p_block_view_port_x - 1 : 1;
	y1 = p_block_view_port_y;
	x2 = p_block_view_port_x;
	y2 = p_block_view_port_y ? p_block_view_port_y - 1 : 1;
	// оперативный подсчет длины рисуемой линии
	i = y_cell;	while(--i >= 0){if(p[x_cell + xs * i] != current_color) break; size_y++;}
	i = y_cell; while(++i < ys){if(p[x_cell + xs * i] != current_color) break; size_y++;}
	i = x_cell; while(--i >= 0){if(p[y_cell * xs + i] != current_color) break; size_x++;}
	i = x_cell;	while(++i < xs){if(p[y_cell * xs + i] != current_color) break; size_x++;}
	// расчитываю координаты вывода
	double	dx1 = atom->x_begin + (atom->left_dig_size + x1 + (size_x<10 ? 0.3 : 0.1)) * cell_space,
			dy1 = atom->y_begin + (atom->top_dig_size  + y1 + 0.15) * cell_space,
			dx2 = atom->x_begin + (atom->left_dig_size + x2 + (size_y<10 ? 0.3 : 0.1)) * cell_space,
			dy2 = atom->y_begin + (atom->top_dig_size  + y2 + 0.15) * cell_space;
	SET_SYS_COLOR(contrast_color(atom->matrix[x1+xs*y1]));
	pango_layout_set_font_description(layout,font_n);
	char dig[4];
	cairo_move_to(cr, dx1, dy1);					//перемещение курсора контекста рисования
	sprintf(dig, "%d", size_x);						// перерисовываю цифру размера линии
	pango_layout_set_text(layout, dig, strlen(dig));//установка текста
	pango_cairo_show_layout(cr,layout);				//отрисовка текста
	SET_SYS_COLOR(contrast_color(atom->matrix[x2+xs*y2]));
	cairo_move_to(cr, dx2, dy2);					//перемещение курсора контекста рисования
	sprintf(dig, "%d", size_y);						// перерисовываю цифру размера линии
	pango_layout_set_text(layout, dig, strlen(dig));//установка текста
	pango_cairo_show_layout(cr,layout);				//отрисовка текста
}

byte paint_rect_x_bgn, paint_rect_y_bgn;			// координаты начала рисуемого прямоугольника
byte paint_rect_x_lst, paint_rect_y_lst;			// текущие координаты рисуемого прямоугольника
byte ccolor = MAX_COLOR;							// маркер или пустота для заливки

void p_botton_release(GdkEventButton *event){
	if(	painting){
		if(event != NULL) u_after(false);
		if((event == NULL || event->button == 1) && p_block_view_port_x >= 0){
			// восстанавливаю вывод ячеек
			int	x1 = p_block_view_port_x ? p_block_view_port_x - 1 : 1,
				y1 = p_block_view_port_y, x2 = p_block_view_port_x,
				y2 = p_block_view_port_y ? p_block_view_port_y - 1 : 1;
			p_paint_cell(cr, x1, y1);
			p_paint_cell(cr, x2, y2);
		}
	paint_rect_x_lst = paint_rect_y_lst = paint_rect_x_bgn = paint_rect_y_bgn = 255;
	p_draw_icon(false);
	}
	painting = false;
	ccolor = MAX_COLOR;
	// проверка отпуска кнопки именно в границах матрицы
	int x = (event->x - atom->x_begin) / cell_space - atom->left_dig_size,
		y = (event->y - atom->y_begin) / cell_space - atom->top_dig_size;
	if(parameter[plesson] && x >= 0 && x < atom->x_size && y >= 0 && y < atom->y_size){
//		c_test_validate_matrix();	// подсвечиваю строки с ошибкой
		c_test_validate_cross(x, y);
	}
}

bool p_make_cell(uint indx, byte button){					// общяя процедура отметки ячейки
	byte *c = atom->matrix + indx;
	if(button == 1){
		*c = current_color;
		if(c_check_complete(atom->matrix)){
			CAIRO_INIT;
			p_paint_cell(cr, indx % atom->x_size, indx / atom->x_size);
			if(!solved){
				set_status_bar(_("Puzzle solved."));
				d_message(_("Congratulation!"), _("Puzzle solved."), 0);
				c_view_status();
				solved = true;
				return false;
			}
		}
	}
	else if (button == 3){
		if(ccolor == MAX_COLOR)
			ccolor =(*c == MARK || *c == current_color) ? EMPTY : MARK;
		*c = ccolor;
	}
	return true;
}

bool p_make_cell_simple(uint indx, byte button){
	byte *c = atom->matrix + indx;
	if(button == 1){
		if(*c != current_color)	{*c = current_color;return true;}
	}
	else {
		if(ccolor == MAX_COLOR)
			ccolor =(*c == MARK || *c == current_color) ? EMPTY : MARK;
		*c = ccolor;
		return true;
	}
	return false;
}

void p_make_rect(byte x, byte y, byte button){	// рисование прямоугольника
	CAIRO_INIT;
	uint indx;
	byte	s_x  = MIN(paint_rect_x_bgn, x),				// размеры текущего прямоуг.
			s_y  = MIN(paint_rect_y_bgn, y),
			e_x  = MAX(paint_rect_x_bgn, x),
			e_y  = MAX(paint_rect_y_bgn, y),
			ls_x = MIN(paint_rect_x_bgn, paint_rect_x_lst),	// размеры последнего прямоуг.
			ls_y = MIN(paint_rect_y_bgn, paint_rect_y_lst),
			le_x = MAX(paint_rect_x_bgn, paint_rect_x_lst),
			le_y = MAX(paint_rect_y_bgn, paint_rect_y_lst),
			fs_x = MIN(s_x, ls_x),							// общая площадь изменяемого выделения
			fs_y = MIN(s_y, ls_y),
			fe_x = MAX(e_x, le_x),
			fe_y = MAX(e_y, le_y);
	paint_rect_x_lst = x; paint_rect_y_lst = y;
	byte *map = MALLOC_BYTE(atom->map_size);

	for(y=ls_y; y <= le_y; y++)								// "рисую" в буфере предыдущий контур
		for(x=ls_x; x<=le_x; x++)
			map[y*atom->x_size+x] = MARK;
	for(y=s_y; y <= e_y; y++)								// "накладываю" на него новый контур
		for(x=s_x; x<=e_x; x++)
			map[y*atom->x_size+x] = BLACK;
	for(y=fs_y; y<=fe_y; y++)
		for(x=fs_x; x<=fe_x; x++){
			indx = y*atom->x_size+x;
			if(map[indx] == BLACK){							// отмечаю различия на видимой поверхности
				if(p_make_cell_simple(indx, button))
					p_paint_cell(cr, x, y);
			}
			else if(map[indx] == MARK){
				if(save_map[indx] != atom->matrix[indx]){
					atom->matrix[indx] = save_map[indx];
					p_paint_cell(cr, x, y);
		}	}	}
}

typedef struct f_t{byte x, y;} f;
f *pfill;

void p_fill_push_cell(byte x, byte y){
	uint ptr = 0;
	while(pfill[ptr].x < 0xff)						// пробегаю весь список
		if(pfill[ptr].x == x && pfill[ptr].y == y)
			return;
		else ptr++;
	pfill[ptr].x = x; pfill[ptr].y = y;
}

void p_make_fill(byte x, byte y, byte button){		// заливка
	byte *	map = atom->matrix,
			wdth= atom->x_size,
			hght= atom->y_size,
			src = map[x+wdth*y];
	uint len = (atom->map_size + 1) * sizeof(f);	// +1 завершающий 0xff для прерывания поиска
	pfill = malloc(len);
	memset(pfill, 0xff, len);
	uint ptr_cur = 0;
	pfill[ptr_cur].x = x; pfill[ptr_cur].y = y;
	while(pfill[ptr_cur].x < 0xff){
		x = pfill[ptr_cur].x; y = pfill[ptr_cur].y;
		if(x > 0 && map[(x-1)+wdth*y] == src)		// проверка ячейки слева
			p_fill_push_cell(x-1, y);
		if(x < wdth-1 && map[(x+1)+wdth*y] == src)	// справа
			p_fill_push_cell(x+1, y);
		if(y > 0 && map[x+wdth*(y-1)] == src)		// сверху
			p_fill_push_cell(x, y-1);
		if(y < hght-1 && map[x+wdth*(y+1)] == src)	// снизу
			p_fill_push_cell(x, y+1);
		if(p_make_cell_simple(x+wdth*y, button))
			p_paint_cell(cr, x, y);
		ptr_cur++;
	}
	free(pfill);
}

void p_matrix_button_event(int indx, GdkEventButton *event){// изменения в матрице
	last_mouse_x = event->x_root; last_mouse_y = event->y_root;// сохраняю позицию курсора
	int column	= indx % atom->x_size;
	int line	= indx / atom->x_size;
	byte *p		= atom->matrix;
	byte save	= p[indx];

	// lesson
	if(event->type == GDK_2BUTTON_PRESS && parameter[plesson]){
		uint ptr;
		p[indx] = EMPTY;
		for(int y=0; y < atom->y_size; y++){
			ptr = column + atom->x_size * y;
			shadow_map[ptr] = solved_map[ptr];
		}
		for(int x=0; x < atom->x_size; x++){
			ptr = line * atom->x_size + x;
			shadow_map[ptr] = solved_map[ptr];
		}
		p_update_line(column, true);
		p_update_line(line, false);
		return;
	}
	if(timer_id	|| painting || event->button == 2) return;
//	u_before(matrix);
	painting=true;
	CAIRO_INIT;

	if(ps == filling){
		p_make_fill(column, line, event->button);
		return;
	}
	else if(ps == rectangle){							// если рисование квадратами
		memcpy(save_map, atom->matrix, atom->map_size);
		paint_rect_x_lst = paint_rect_x_bgn = column;
		paint_rect_y_lst = paint_rect_y_bgn = line;
		p_make_rect(column, line, event->button);
		if(p[indx] != EMPTY && p[indx] != MARK)
			p_draw_block_len(column, line);
		return;
	}
	if(p[indx]==EMPTY || p[indx]==MARK)
		p_make_cell(indx, event->button);				// отмечаю байт-ячейку в матрице
	else {
		if(event->button == 1){
			current_color = p[indx];
			if(palette_show) gtk_widget_queue_draw(palette);
		}
		if(!p_make_cell(indx, event->button))			// отмечаю байт-ячейку в матрице
			return;
	}

	if(save	!= p[indx]){
		p_draw_icon(false);								// перерисовка иконки
		p_paint_cell(cr, column, line);					// прорисовка ячейки
	}
	if(event->button == 1 && (p[indx]!=EMPTY || p[indx]!=MARK)){
		CAIRO_INIT;
		p_draw_block_len(column, line);
	}
}

bool p_mouse_motion(GtkWidget *widget, GdkEventMotion *event){
	int indx, top_col, left_row;
	GdkModifierType state = event->state;
	if(state & GDK_BUTTON2_MASK){	// выполняю сроллинг если зажата правая кнопка
		scrolled_viewport(last_mouse_x-event->x_root, last_mouse_y-event->y_root);
		last_mouse_x = event->x_root; last_mouse_y = event->y_root;
		return true;
	}
	if(timer_id || !atom || ps==filling) return false;
	int x_cell = (event->x - atom->x_begin) / cell_space;
	int y_cell = (event->y - atom->y_begin) / cell_space;
													// выход если адрес ячейки не изменился
	if(x_cell == atom->x_cell && y_cell == atom->y_cell) return false;
													// выход если курсор не в пределах матрицы
	if(	x_cell < atom->left_dig_size || x_cell >= atom->left_dig_size + atom->x_size ||
		y_cell < atom->top_dig_size  || y_cell >= atom->top_dig_size + atom->y_size){

		if(atom->top_col < 0) return true;				// выход если курсор не первый раз вне матрицы
														// очищаю "подсветку" оцифровки при выходе курсора из матрицы
		CAIRO_INIT;
		x_cell = atom->x_cell, y_cell = atom->y_cell;
		p_make_column(atom->top_col);
		p_make_row(atom->left_row);
		atom->x_cell = atom->y_cell = -1;
		atom->top_col = atom->left_row = -1;
		if(validate) clear_status_bar();
		return true;
	}

	char *buf=malloc(100);							// обновляю статусбар
	sprintf(buf, _("Cursor position: %d/-%d x %d/-%d"),
		(int)(x_cell - atom->left_dig_size+1),
		(int)(atom->x_puzzle_size - x_cell),
		(int)(y_cell-atom->top_dig_size+1),
		(int)(atom->y_puzzle_size - y_cell)
		);
	set_status_bar(buf);
	m_free(buf);
// "Подсветка" оцифровки ***********************************************
	top_col		= x_cell - atom->left_dig_size;
	left_row	= y_cell - atom->top_dig_size;

	if(atom->top_col != top_col && !atom->curs->status){
		CAIRO_INIT;
		// если изменилась позиция курсора колонки
		if(atom->top_col >= 0)
		// стираю фон в предыдуще выделенном столбце
			p_make_column(atom->top_col);
		// отмечаю новый столбец
		p_make_column(top_col);
	}
	if(atom->left_row != left_row && !atom->curs->status){	// если изменилась позиция курсора ряда
		CAIRO_INIT;
		if(atom->left_row >= 0)								// стираю фон в предыдуще выделенном ряду
			p_make_row(atom->left_row);				// отмечаю новый ряд
		p_make_row(left_row);
	}

	atom->top_col = top_col; atom->left_row = left_row;		// сохраняю текущую строку и столбец
	atom->x_cell = x_cell; atom->y_cell = y_cell;			// сохраняю координаты последней используемой ячейки

// Выход если ненажата ни одна из кнопок *******************************
	if( !(state & GDK_BUTTON1_MASK)	&&
		!(state & GDK_BUTTON3_MASK)){
		return true;
	}

	indx = left_row * atom->x_size + top_col;					// расчитываю индекс в матрице

	byte button = 0;
	byte *p = atom->matrix;

	if(state & GDK_BUTTON1_MASK)		button = 1;			// изменяю состояние ячейки
	else if(state & GDK_BUTTON2_MASK)	button = 2;
	else if(state & GDK_BUTTON3_MASK)	button = 3;
	if(ps == rectangle){									// если рисование квадратами
		p_make_rect(top_col, left_row, button);
		if(p[indx] != EMPTY && p[indx] != MARK)
			p_draw_block_len(top_col, left_row);
		return true;
	}
	else {
		byte save = p[indx];
		p_make_cell(indx, button);
		if(save != p[indx]){
			CAIRO_INIT;
			p_paint_cell(cr, top_col, left_row);
			p_draw_icon(false);
		}
	}
	if(parameter[plesson]){
		c_test_validate_cross(top_col, left_row);
	}
	if(p[indx] != EMPTY && p[indx] != MARK)
		p_draw_block_len(top_col, left_row);
	return true;
}

void p_draw_grid(cairo_t *cr){							// отрисовка решётки
	size_t d,i;
	float e;
	cairo_set_line_width(cr,1);

	SET_SYS_COLOR(BOLD);									// рамка
	cairo_move_to(cr, atom->x_begin + 0.5, atom->y_begin + 0.5);	// левая граничная линия
	cairo_line_to(cr, atom->x_begin + 0.5, cell_space*(atom->y_size+atom->top_dig_size)+atom->y_begin + 0.5);
	cairo_move_to(cr, atom->x_begin + 0.5, atom->y_begin + 0.5);	// верхняя линия
	cairo_line_to(cr, atom->x_begin+cell_space*(atom->x_size+atom->left_dig_size) + 0.5, atom->y_begin + 0.5);
	cairo_stroke(cr);

	//отрисовка вертикальных линий
	SET_SYS_COLOR(LINES);	// левая оцифровка
	if(parameter[pgrid]){
		for(i = 1; i < atom->left_dig_size; i++){
			cairo_move_to(cr, i * cell_space+atom->x_begin + 0.5,
						cell_space * atom->top_dig_size + atom->y_begin + 0.5);
			cairo_line_to(cr, i * cell_space + atom->x_begin + 0.5,
				cell_space * (atom->y_size + atom->top_dig_size) + atom->y_begin - 0.5);
			cairo_stroke(cr);
		}
	}
	for(i = 0; i <= atom->x_size; i++){
		// последняя линия рабочей матрицы
		d = atom->left_dig_size + i;
		if(i == atom->x_size){
			SET_SYS_COLOR(BLACK);
			e = d * cell_space + atom->x_begin + 0.5;
			cairo_move_to(cr, e, atom->y_begin + 0.5);
			cairo_line_to(cr, e, cell_space *
						(atom->y_size+atom->top_dig_size)+atom->y_begin + 0.5);
			cairo_stroke(cr);
			continue;
		}

		// сетка
		if(parameter[pgrid5] && !(i % 5)) continue;
		SET_SYS_COLOR(LINES);
		if(parameter[pgrid]){
			e = d * cell_space + atom->x_begin + 0.5;
			cairo_move_to(cr, e, atom->y_begin + 1.5);
			cairo_line_to(cr, e, cell_space * (atom->y_size + atom->top_dig_size) + atom->y_begin - 0.5);
			cairo_stroke(cr);
		}
	}

	//отрисовка горизонтальных линий
	SET_SYS_COLOR(LINES);	// верхняя оцифровка
	if(parameter[pgrid]){
		for(i = 1; i < atom->top_dig_size; i++){
			e = atom->y_begin + i * cell_space + 0.5;
			cairo_move_to(cr, atom->x_begin + atom->left_dig_size * cell_space + 0.5, e);
			cairo_line_to(cr, cell_space * (atom->x_size + atom->left_dig_size) + atom->x_begin - 0.5, e);
			cairo_stroke(cr);
		}
	}
	for(i = 0; i <= atom->y_size; i++){
		d = atom->top_dig_size + i;
		// последняя линия
		if(i == atom->y_size){
			SET_SYS_COLOR(BLACK);
			e = d * cell_space + atom->y_begin + 0.5;
			cairo_move_to(cr, atom->x_begin, e);
			cairo_line_to(cr, cell_space * (atom->x_size + atom->left_dig_size)	+ atom->x_begin + 0.5, e);
			cairo_stroke(cr);
			continue;
		}
		// решетка
		if(parameter[pgrid5] && !(i % 5)) continue;
		SET_SYS_COLOR(LINES);
		if(parameter[pgrid]){
			e = atom->y_begin + d * cell_space + 0.5;
			cairo_move_to(cr, atom->x_begin + 1.5, e);
			cairo_line_to(cr, cell_space * (atom->x_size + atom->left_dig_size) + atom->x_begin - 0.5, e);
			cairo_stroke(cr);
		}
	}

	SET_SYS_COLOR(BLACK);	// граница оцифровки и матрицы
	e = atom->left_dig_size * cell_space + atom->x_begin + 0.5;
	cairo_move_to(cr, e, atom->y_begin + 0.5);
	cairo_line_to(cr, e, cell_space * (atom->y_size+atom->top_dig_size)+atom->y_begin + 0.5);
	e = atom->top_dig_size * cell_space + atom->y_begin + 0.5;
	cairo_move_to(cr, atom->x_begin + 0.5, e);
	cairo_line_to(cr, cell_space * (atom->x_size + atom->left_dig_size) + atom->x_begin + 0.5, e);
	cairo_stroke(cr);

	// решетка 5х5
	if(	parameter[pgrid5] ){
		SET_SYS_COLOR(BOLD);
		for(i = 5; i < atom->x_size; i+=5){
			e = (atom->left_dig_size + i) * cell_space + atom->x_begin + 0.5;
			cairo_move_to(cr, e, atom->y_begin + 0.5);
			cairo_line_to(cr, e, cell_space * (atom->y_size + atom->top_dig_size) + atom->y_begin + 0.5);
			cairo_stroke(cr);
		}
		for(i = 5; i < atom->y_size; i+=5){
			e = atom->y_begin + (atom->top_dig_size + i) * cell_space + 0.5;
			cairo_move_to(cr, atom->x_begin + 0.5, e);
			cairo_line_to(cr, cell_space * (atom->x_size + atom->left_dig_size) + atom->x_begin + 0.5, e);
			cairo_stroke(cr);
		}
	}
}

void p_draw_top_digits(cairo_t *cr){	// верхняя оцифровка
	int i, j, x, y, x_font, y_font, indx;
	char dig[4];
	byte color;
	for(i = 0; i < atom->x_size; i++)
        for(j = 0; j < atom->top_dig_size; j++){
            x = atom->x_begin + (atom->left_dig_size + i) * cell_space;
            y = atom->y_begin + (atom->top_dig_size - j - 1) * cell_space;
            indx = i*atom->top_dig_size+j;
			if(atom->top.digits[indx] < 10)	// если цифра в один знак
					x_font = x + (double)cell_space * (double)0.3;
			else 	x_font = x + (double)cell_space * (double)0.1;
            y_font = y + (double)cell_space * (double)0.2;
			if(atom->isColor)	color = atom->top.colors[indx]&COL_MARK_MASK;
			else				color = WHITE;
			if(atom->curs->status && atom->curs->digits	== top_dig && !atom->top.digits[indx])
								color = HIDE;
			SET_COLOR(color);
			cairo_rectangle(RECT);
			cairo_fill(cr);
            if(atom->top.digits[indx] != 0){
				SET_COLOR(contrast_color(color));
				sprintf(dig,"%d",atom->top.digits[indx]);
				//установка текста
				pango_layout_set_text(layout, dig, strlen(dig));
				//перемещение курсора контекста рисования
				cairo_move_to(cr, x_font, y_font);
				//установка шрифта текста
				pango_layout_set_font_description(layout,font_n);
				//отрисовка текста
				pango_cairo_show_layout(cr,layout);
				if (atom->top.colors[indx] & COLOR_MARK){
					cairo_set_line_width(cr,DMARK);
					DRAW_SLASH;
					cairo_stroke(cr);
				}
			}
		}
}

void p_draw_left_digits(cairo_t *cr){	// левая оцифровка
	int i, j, x, y, x_font, y_font, indx;
	char dig[4];
	byte color;
	for(i = 0; i < atom->y_size; i++)
        for(j = 0; j < atom->left_dig_size; j++){
            x = atom->x_begin + (atom->left_dig_size - j -1) * cell_space;
            y = atom->y_begin + (atom->top_dig_size + i) * cell_space;
            indx = i*atom->left_dig_size+j;
            if(atom->left.digits[indx] < 10)
					x_font = x + (double)cell_space * (double)0.3;
			else 	x_font = x + (double)cell_space * (double)0.1;
            y_font = y + (double)cell_space * (double)0.2;
			if(atom->isColor)	color = atom->left.colors[indx]&COL_MARK_MASK;
			else				color = WHITE;
			if(atom->curs->status && atom->curs->digits	== left_dig && !atom->left.digits[indx])
								color = HIDE;
			SET_COLOR(color);
			cairo_rectangle(RECT);
			cairo_fill(cr);
            if(atom->left.digits[indx] != 0){
				SET_COLOR(contrast_color(color));
				sprintf(dig,"%d",atom->left.digits[indx]);
				//установка текста
				pango_layout_set_text(layout, dig, strlen(dig));
				//перемещение курсора контекста рисования
				cairo_move_to(cr, x_font, y_font);
				//установка шрифта текста
				pango_layout_set_font_description(layout,font_n);
				//отрисовка текста
				pango_cairo_show_layout(cr,layout);
				if (atom->left.colors[indx] & COLOR_MARK){
				cairo_set_line_width(cr,DMARK);
					DRAW_SLASH;
					cairo_stroke(cr);
				}
			}
		}
}

void p_draw_numbers(cairo_t *cr){		// отрисовка оцифровки
	p_draw_top_digits(cr);
	p_draw_left_digits(cr);
}

void p_update_digits(){
	CAIRO_INIT;
	p_draw_numbers(cr);
}

void p_mark_cell_in(byte color, int x, int y){
	float dev = ERRM/2 + 0.5;
	if(color == MARK){
		SET_SYS_COLOR(CROSS);
		cairo_set_line_width(cr,1);
		cairo_move_to(cr, x - dev, y - dev);
		cairo_line_to(cr, x + dev, y + dev);
		cairo_move_to(cr, x - dev, y + dev);
		cairo_line_to(cr, x + dev, y - dev);
		cairo_stroke(cr);
	}
	else {
		SET_COLOR(color);
		cairo_rectangle(cr,x-ERRM/2,y-ERRM/2,ERRM,ERRM);
		cairo_fill(cr);
	}

}

void p_mark_cell(int ix, int iy){
	CAIRO_INIT;
	int x = atom->x_begin + (atom->left_dig_size + ix + 0.5) * cell_space + space;
	int y = atom->y_begin + (atom->top_dig_size + iy + 0.5) * cell_space + space;
	byte color = shadow_map[iy*atom->x_size+ix];
	p_mark_cell_in(color, x, y);
}

void draw(GtkWidget *widget, GdkEventExpose *event){	// освежевание вывода
	CAIRO_INIT;
	if(layout == NULL) layout = pango_cairo_create_layout(cr);
	cairo_rectangle(cr, event->area.x, event->area.y, event->area.width, event->area.height);
	cairo_clip(cr);

	if(!atom) return;
	//определение размеров области рисования
	int w = widget->allocation.width;
	int h = widget->allocation.height;

	//определение размеров ячейки
	int y = (double)w/(double)(atom->x_size+atom->left_dig_size); // вертикальный шаг
	int x = (double)h/(double)(atom->y_size+atom->top_dig_size);// горизонтальный шаг
	cell_space = MIN(x, y);

	// размер отступа матрицы от края окна
	atom->x_begin = (w - cell_space * (atom->x_size+atom->left_dig_size)) / 2;
	atom->y_begin = (h - cell_space * (atom->y_size+atom->top_dig_size)) / 2;

	//отрисовка фона
	SET_COLOR(BACKGROUND);
	cairo_rectangle(cr,0,0,w,h);
	cairo_fill(cr);
	cairo_stroke(cr);

	//создание шрифтов
	static int save_x_space = -1;
	static int save_y_space = -1;
	if(save_x_space != cell_space || save_y_space != cell_space){
		fsize = MIN(cell_space, cell_space) / 2;
		char font_name[] = "sans italic 10\'0'";
		sprintf(font_name, "sans italic %d", fsize);
		font_i = pango_font_description_from_string(font_name);
		sprintf(font_name, "sans bold %d", fsize);
		font_b = pango_font_description_from_string(font_name);
		sprintf(font_name, "sans normal %d", fsize);
		font_n = pango_font_description_from_string(font_name);
		//создание указателя на объект графического текста
		save_x_space = cell_space; save_y_space = cell_space;
	}
	//рисую сетку
	p_draw_grid(cr);
	//рисую цифры
	p_draw_numbers(cr);
	//прорисовка матрицы
	p_draw_matrix(cr);
	p_draw_icon(true);
}

bool p_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data){
    if(!c_solve_buffers) return false;
    draw(widget, event);	// перерисовка всего пазла
	return FALSE;
}

void p_draw_icon_point(cairo_t *cr, int x, int y){
	byte c = atom->matrix[y * atom->x_size + x];
	if(c != EMPTY && c != MARK){
		size_t s_x = atom->x_begin, s_y = atom->y_begin;
		size_t h = atom->y_size, w = atom->x_size;
		size_t h_array = atom->top_dig_size * cell_space;
		size_t w_array = atom->left_dig_size * cell_space;
		size_t s = MIN((int)(w_array/w),(int)(h_array/h));
		cairo_set_source_rgb(cr, ((float)atom->rgb_color_table[c].r/255),
			((float)atom->rgb_color_table[c].g/255), ((float)atom->rgb_color_table[c].b/255));
		cairo_rectangle(cr, x*s+1+s_x+(w_array-w*s)/2, y*s+1+s_y+(h_array-h*s)/2, s, s);
		cairo_fill(cr);
	}
}

void p_draw_icon(bool new){
	byte *p = atom->matrix;
	size_t s_x = atom->x_begin, s_y = atom->y_begin;
	size_t h = atom->y_size, w = atom->x_size;
	size_t h_array = atom->top_dig_size * cell_space;
	size_t w_array = atom->left_dig_size * cell_space;
	size_t s = MIN((int)(w_array/w),(int)(h_array/h));
	int x, y, c;
	cairo_t *i;

	if(new && (bool)surface){
		cairo_surface_destroy (surface);
		surface = NULL;
	}
	if(!surface){
		x = w*s; y = h*s;
		if(x >= w_array) x = w_array - 1;
		if(y >= h_array) y = h_array - 1;
		surface = gdk_window_create_similar_surface(draw_area->window,
												CAIRO_CONTENT_COLOR, x, y);
		}
	i = cairo_create (surface);
	cairo_set_source_rgb (i, 1, 1, 1);						// зачистка фона
	cairo_paint (i);

	c = 0;
	for(y=0; y < h; y++)
		for(x=0; x < w; x++){
			if(p[c] != EMPTY && p[c] != MARK){
				cairo_set_source_rgb(i,	((float)atom->rgb_color_table[p[c]].r/255),
					((float)atom->rgb_color_table[p[c]].g/255), ((float)atom->rgb_color_table[p[c]].b/255));
				cairo_rectangle(i, x*s, y*s, s, s);
				cairo_fill(i);
			}
			c++;
		}
	cairo_destroy(i);

	CAIRO_INIT;												// прорисовываю миниатюру
	cairo_set_source_surface (cr, surface, s_x + 1 + (w_array - w*s)/2, s_y + 1 + (h_array - h*s)/2);
	cairo_paint (cr);
}

