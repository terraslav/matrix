//puzzle.c
#include "puzzle.h"
#include "paint.h"
#include "utils.h"
#include "main.h"
#include "mafile.h"
#include "dialog.h"

//#define calcerrors
#define ADVANCE

#define animate		parameter[panimate]	// анимация рисования
#define lesson		parameter[plesson]	// режим обучения

const char *color_name[] = {"White", "Black", "Blue", "Red", "Magenta", "Green", "Cyan", "Yallow", "Silver",
						"Gray", "Maroon", "Navy", "Purple", "Lime", "Teal", "Olive", "Aquamarine", "Orange", "Pink"};

int c_width, c_height, c_top, c_left, fild_offset, map_size, xy_size;
byte *t_d, *l_d, *t_c, *l_c, *t_clr, *l_clr, *c_map, *cblock, *block, *orig, *variants, *c_buffer, *access_color;
byte *x_complete, *y_complete;
bool calcerror;
static int start = 0;				// счетчик для плавного начала анимации
bool	no_logic;					// решение при отсутствии логики
bool	view_mark = false;			// нужно-ли выводить маркеры

bool c_scan_line(byte*, int, bool, bool);
// определяю цветные функции
bool c_update_solved_buffer();
bool c_make_access_color();
void c_solver_color();
bool c_check_digits_summ_color();
bool c_check_complete_color(byte*);
bool c_make_pos_and_access_color(void);
bool c_make_overlapping_color();
bool c_scan_line_color(byte *, int, bool, bool);
bool c_recalc_access_color();
void c_no_logic_color();
bool c_test_line_color(byte*,int,bool,c_test_line_res*);

typedef enum calcres_tag{ALL_OK,CALCERROR,OVERFLOW,USERSTOPED,NOLOGIK} calcres;

void c_clear_mark(){
	int i;
	for(i=0; i<atom->map_size; i++)						// удаляю маркеры матрицы
		if(atom->matrix[i] == MARK) atom->matrix[i] = ZERO;
	for(i=0; i<atom->top_dig_size*atom->x_size; i++)	// удаляю маркеры оцифровки
		atom->top.colors[i] &= COL_MARK_MASK;
	for(i=0; i<atom->left_dig_size*atom->y_size; i++)
		atom->left.colors[i] &= COL_MARK_MASK;
}

gboolean c_timer_tik(gpointer i){
	if(!animate && !demo_flag){
		if(timer_id) g_source_remove(timer_id);
		timer_id = 0;
		start = 0;
		memcpy(c_map, shadow_map, map_size);
		p_update_matrix();
		p_draw_icon(false);
		enable_toolbar_animate(true);
		return true;
	}
	int ptr = solv_cell_current;
	int cnt = 0;
	CAIRO_INIT;
	while(cnt < chain_s && ptr < solv_cell_top){
		if(solved_cells[ptr].count == anim_amp){
			int indx = solved_cells[ptr].y *  c_width + solved_cells[ptr].x;
			c_map[indx] = shadow_map[indx];					// "обналичиваю" ячейку
			p_draw_icon_point(cr, solved_cells[ptr].x, solved_cells[ptr].y);
		}
		if(solved_cells[ptr].count > 0){
			if(!i)	solved_cells[ptr].count--;
			else	solved_cells[ptr].count = 0;
			p_animate_cell(solved_cells[ptr].count, solved_cells[ptr].x, solved_cells[ptr].y, cr);
			cnt++;
		}
		if(cnt > start){
			start++;
			break;
			}
		ptr++;
	}
	if(!solved_cells[solv_cell_current].count)
		while(!solved_cells[solv_cell_current].count && solv_cell_current < solv_cell_top){
			solv_cell_current++;
		}
	if(solv_cell_current >= solv_cell_top){
		g_source_remove(timer_id);
		timer_id = 0;
		start = 0;
		if(c_check_complete(shadow_map))c_clear_mark();
		p_draw_icon(false);
		p_draw_matrix(cr);
		enable_toolbar_animate(true);
		if(demo_flag) m_demo_next();
	}
	return true;
}

void c_reg_solved_cell(int x, int y){
	if(lesson && animate) return;
	if(!c_solve_buffers || (!animate /*&& !demo_flag*/)){
		int indx = y*c_width+x;
		if(!lesson || demo_flag) c_map[indx] = shadow_map[indx];	// "обналичиваю" ячейку
		return;
	}
	for(int i=0; i<solv_cell_top; i++)	// проверка регистрации в конвейере
		if(solved_cells[i].x == x && solved_cells[i].y == y) return;
	solved_cells[solv_cell_top].x = x;
	solved_cells[solv_cell_top].y = y;
	solved_cells[solv_cell_top].count = anim_amp;
	solv_cell_top++;
	if(!timer_id)
		if((demo_flag)||(animate && !lesson)){
			timer_id = g_timeout_add(timeout, c_timer_tik, NULL);
			enable_toolbar_animate(false);					// деактивирую тулбар
		}
}

void c_add_solved_cell(int indx, int number, bool that){
	c_reg_solved_cell(that ? number : indx, that ? indx : number);
}

void c_reset_puzzle_pointers(){
		c_st_pos = c_en_pos = NULL;							// обнуляю буферы позиций
		t_d = l_d = t_c = l_c = block = orig = variants = c_buffer = NULL;
		x_error = y_error =  NULL;
		y_complete = x_complete = NULL;
		c_solve_buffers = false;
		atom = NULL;
		solved_cells = NULL;
}

void c_malloc_puzzle_buf(puzzle *atom){							// выделяю память для матрицы и оцифровки
	atom->top.digits		= MALLOC_BYTE(atom->x_size * atom->top_dig_size);
	atom->left.digits		= MALLOC_BYTE(atom->y_size * atom->left_dig_size);
	atom->top.colors		= MALLOC_BYTE(atom->x_size * atom->top_dig_size);
	atom->left.colors		= MALLOC_BYTE(atom->y_size * atom->left_dig_size);
	atom->top.count			= MALLOC_BYTE(atom->x_size);
	atom->left.count		= MALLOC_BYTE(atom->y_size);
	atom->matrix			= MALLOC_BYTE(atom->map_size);
	atom->x_begin			= atom->y_begin = 0;
	atom->top_col			= atom->left_row = -1;
	atom->curs				= MALLOC_BYTE(sizeof (cursor));
	atom->curs->x			= 0;
	atom->curs->y			= 0;
	atom->curs->digits		= top_dig;
	atom->curs->status		= false;
	atom->hints				= MALLOC_BYTE(MAX_HINTS*sizeof(hint)+1);
	atom->c_hints			= 0;
	save_map				= MALLOC_BYTE(atom->map_size);
	for(int i=0; i<20; i++)
		atom->rgb_color_table[i] = color_table[i];	// инициализация палитры
}

void c_free_puzzle_buf(puzzle *atom){				// освобождаю память пазла
	if(!atom) return;
	if(timer_id) g_source_remove(timer_id);
	timer_id = 0;
	u_clear(0);								// очищаю отмены
	m_free(atom->top.digits);
	m_free(atom->top.count);
	m_free(atom->top.colors);
	m_free(atom->left.digits);
	m_free(atom->left.count);
	m_free(atom->left.colors);
	m_free(atom->matrix);
	m_free(atom->curs);
	m_free(atom->hints);
	m_free(save_map);
}

bool c_check_digits_summ(){
	if(atom->isColor) return c_check_digits_summ_color();
	u_digits_change_event(true, false);
	int topsum = 0, leftsum = 0;
	size_t count = c_top * c_width;
	byte *ptr = t_d;
	while(count) topsum += ptr[--count];
	if(!topsum) return atom->digits_status = false;
	count = c_left * c_height;
	ptr = l_d;
	while(count) leftsum += ptr[--count];
	if(topsum != leftsum){									// проверяю эквивалентность оцифровок
		strcpy(last_error, _("Horizontal and vertical numbers sums nonequivalent!\n"));
		sprintf(last_error+strlen(last_error), _("Top digits summ: %d\nLeft digits summ: %d"), topsum, leftsum);
//		d_message(_("Puzzle digits error!"), last_error, 0);
		return atom->digits_status = false;
	}

	return atom->digits_status = true;
}

bool c_check_complete(byte *map){
	if(!c_solve_buffers) return false;
	if(atom->isColor) return c_check_complete_color(map);
	int top = 0, left = 0, matt = 0;
	size_t count = c_top * c_width;
	while(count) top += t_d[--count];
	count = c_left * c_height;
	while(count) left += l_d[--count];
	if(top != left){
		sprintf(last_error, _("Summ horisontal & vertical digits is not equivalent! Top summ = %d, left summ = %d"), top, left);
		return false;
	}
	count = map_size;
	while(count){
		count--;
		if(map[count] != MARK && map[count] != EMPTY) matt++;
	}
	if(top != matt){
		strcpy(last_error, _("Summ digits and summ maked marix cells is not equivalent!"));
		return false;
	}
	// проверка на правильность заполнения столбцов-строк
	c_test_line_res r;
	for(int x=0; x<c_width; x++){
		c_test_line(solved_map, x, true, &r);
		if(!r.complete){
			sprintf(last_error, _("C_check_complete: error in column %d"), x+1);
			return false;
		}
	}
	for(int y=0; y<c_height; y++){
		c_test_line(solved_map, y, false, &r);
		if(!r.complete){
			sprintf(last_error, _("C_check_complete: error in line %d"), y+1);
			return false;
		}
	}
	return true;
}

void c_view_status(){
	char *buf = malloc(2048);
	uint i, dsumm=0, tnum=0, lnum=0, snum=0;
	double fill, solv;
	bool res;
//	c_test_line_res r;
	sprintf(buf,_("Name:\t\t\t%s\n"), puzzle_name);
	strcat(buf,_("Status:\t\t\t"));

	for(i=0; i<c_width*c_top; i++)
		if(t_d[i]!=0){ dsumm+=t_d[i]; tnum++; }
	for(i=0; i<c_height*c_left; i++)
		if(l_d[i]!=0) lnum++;
	fill = (double)dsumm / ((double)(c_width * c_height) / 100);

//	int numb = c_test_validate_matrix(false);
//validate
	bool s_res = c_update_solved_buffer();
	res = true;
	for(i=0; i<map_size; i++)
		if(c_map[i]!=MARK && c_map[i]!=EMPTY){
			if(solved_map[i]!=MARK){
				if(atom->isColor && solved_map[i]!=EMPTY && solved_map[i]!=c_map[i]){
					res=false; break;}
				snum++;
			}
			else
				{res=false; break;}
		}
	solv = (double)snum / ((double)dsumm / 100);

	if(!s_res)
		strcat(buf, last_error);
	else if(!res)
		strcat(buf, _("error in matrix!\n"));
	else if(solv == 100)
		strcat(buf, _("solved!\n"));
	else if(puzzle_status == bad)
		strcat(buf, _("mistake in digitization!\n"));
	else if(puzzle_status == half)
		strcat(buf, _("no logical decision!\n"));
	else if(puzzle_status == ready)
		strcat(buf, _("OK!\n"));
	if(res){
		char *ptr = buf+strlen(buf);
		sprintf(ptr,_("Difficulties:\t\t%d\n"), stat_difficult);
		ptr = buf+strlen(buf);
		sprintf(ptr,
_("Digits summ:\t\t%d\nTop digits number:\t%d\nLeft digits number:\t%d\nFilling fild on:\t\t%.1f%%\nGuessed on:\t\t%.1f%%"),
		dsumm, tnum, lnum, fill, solv);
	}
	d_message(_("Nanogramm features"), buf, 0);
}

void c_solve_destroy(){
	if(thread_solve_no_logic){
		pthread_cancel(thread_solve_no_logic);
		thread_solve_no_logic = 0;
	}
	if(!c_solve_buffers) return;
	if(timer_id) g_source_remove(timer_id);
	timer_id = 0;
	m_free(c_st_pos);
	m_free(c_en_pos);
	m_free(c_buffer);
	m_free(block);
	m_free(cblock);
	m_free(orig);
	m_free(variants);
	m_free(x_complete);
	m_free(y_complete);
	m_free(x_error);
	m_free(y_error);
	m_free(solved_cells);
	m_free(shadow_map);
	m_free(solved_map);
	m_free(access_color);
	c_solve_buffers = false;
}

void c_make_hints(byte *map){
	if(!atom->c_hints) return;
	for(int i=0; i<atom->c_hints; i++)
		map[atom->hints[i].x + atom->hints[i].y * c_width] = atom->hints[i].color;
}

void c_clear_errors(){
	if(!c_solve_buffers) return;
	for(uint x=0; x < c_width; x++)	{x_complete[x] = true; x_error[x] = false;}
	for(uint y=0; y < c_height; y++)	{y_complete[y] = true; y_error[y] = false;}
}

bool c_update_solved_buffer(){						// решаю для буфера проверки
	int x, y;
	bool px, py, res;
	byte *map = demo_flag ? shadow_map : solved_map;
	puzzle_status = ready;
	stat_difficult = 0;
	memset(solved_map, 0, map_size);
	c_make_hints(solved_map);
	c_clear_errors();

	if(!c_check_digits_summ()){
		puzzle_status = bad;
		calcerror = true;
//		strcpy(last_error, _("digitizations non equivalent!"));
		return false;
	}
	if(!c_make_start_position()){
		calcerror = true;
		c_error;
		puzzle_status = bad;
		strcpy(last_error, _("mistake in digitization!\n"));
		return false;
	}
	if(!c_make_overlapping(map, demo_flag))			// запускаю make_overlap
		c_error;
	byte ads = help_level; help_level = 0;
	do{res = false;
		if(atom->isColor) c_recalc_access_color();
		x=0, y=0;
		px=false, py=false;
		do{
			if(x<c_width && x_complete[x]) res |= c_scan_line(map, x, true, demo_flag);
			if(calcerror){c_error; break;}
			if(y<c_height && y_complete[y]) res |= c_scan_line(map, y, false, demo_flag);
			if(calcerror){c_error; break;}
			x++; y++;
			if(x == c_width) {x = 0; px = true;}
			if(y == c_height){y = 0; py = true;}
		} while(!px || !py);
		stat_difficult++;
		if(calcerror) break;
	} while(res);
	if(!calcerror){
		if(demo_flag){ memcpy(solved_map, shadow_map, map_size);// перебрасываю буфер
			if(!animate){
				m_demo_next();
			}
		}
		puzzle_status = c_check_complete(solved_map) ? ready : half; // bad|half|ready
	}
	else {
		puzzle_status = bad;
		strcpy(last_error, _("mistake in attempt of the decision!"));
		c_error;
		help_level = ads;
		return false;
	}
	help_level = ads;
	return true;
}

void c_solve_init(bool view){ // view - показывать решение или нет
	calcerror = false;
	if(timer_id) {g_source_remove(timer_id);	timer_id = 0;}
	if(c_solve_buffers) c_solve_destroy();
	u_digits_change_event(true, false);				// "подбиваю" оцифровку

// создание альясов
	c_width = atom->x_size; c_height = atom->y_size; c_top = atom->top_dig_size; c_left = atom->left_dig_size;
	t_d = atom->top.digits; l_d = atom->left.digits; t_c = atom->top.count; l_c = atom->left.count;
	t_clr = atom->top.colors; l_clr = atom->left.colors;
	fild_offset = c_width * c_top;					// смещение на левую оцифровку в массиве
	xy_size = c_height + c_width;
	map_size = atom->map_size;
	c_map = atom->matrix;
	no_logic = solved = false;
	solv_cell_current = solv_cell_top = 0;
	step_current = -1;
	atom->digits_status = false;

// инициализация буферов
	size_t len = (c_top * c_width + c_left * c_height) + 1;
	c_st_pos 				= MALLOC_BYTE(len);						// выделяем буфер для хранения стартовых
	c_en_pos 				= MALLOC_BYTE(len);						// конечных и временных

	len 					= MAX(MAX(c_top, c_left)+1,5);
	block					= MALLOC_BYTE(len);						// буфер для копии оцифровки строки
	cblock					= MALLOC_BYTE(len);						// буфер для копии цветов оцифровки строки

	len						= MAX(c_width, c_height);
	c_buffer 				= MALLOC_BYTE(len);						// создаем буфер вариантов
	orig					= MALLOC_BYTE(len);						// буфер для копирования строки
	variants				= malloc (len * sizeof(int)+1);			// массив счетчиков для ячеек строки

	x_complete				= malloc (c_width * sizeof(bool)+1);	// буфера отгаданных колонок
	y_complete				= malloc (c_height * sizeof(bool)+1);	// и строк

	x_error					= malloc (c_width * sizeof(bool)+1);	// буфера строк с ошибками
	y_error					= malloc (c_height * sizeof(bool)+1);

	solved_cells			= malloc (map_size * sizeof(addr_cell)+1);// список решенных ячеек
	shadow_map				= MALLOC_BYTE(map_size);				// теневая копия матрицы
	solved_map				= MALLOC_BYTE(map_size);				// матрица проверки
	access_color			= MALLOC_BYTE(map_size * MAX_COLOR + 1);// таблица доступных ячейкам цветов

	c_solve_buffers = true;

// подготовка к решению
	c_update_solved_buffer();
	c_clear_errors();
	if(atom->isColor)	c_make_pos_and_access_color();
	else 				c_make_start_position();
	if(!demo_flag) {
		memset(shadow_map, 0, map_size);
		if(view) memcpy(c_map, solved_map, map_size);
	}
	c_make_hints(shadow_map);										// маркирую подсказки
	c_make_hints(c_map);

	strcpy(last_error, calcerror?_("Puzzle error!"):_("Solved ready"));
	if(!demo_flag) c_error;
}

bool c_make_start_position(){								// определение начальных начальных и конечных позхиций блоков
	if(atom->isColor) return c_make_pos_and_access_color();
	int end, x, y;
	for (x=0; x < c_width; x++){ // счетчик столбцов
		end = 0;
		for(y = 0; y < t_c[x]; y++){ // счетчик цифр в строчке
			c_st_pos[x * c_top + y] = end;
			end += t_d[x * c_top + t_c[x] - y - 1] + 1;
		}
		if(end > c_height + 1){
			sprintf(last_error, _("\nC_make_start_position: Error overflow in column: %d, calculate column len: %d\n"), x+1, end);
			return false;
		}
	}
	for (y=0; y < c_height; y++){ // счетчик строк
		end = 0;
		for(x=0; x < l_c[y]; x++){ // счетчик цифр в строчке
			c_st_pos[y * c_left + x + fild_offset] = end;
			end += l_d[y * c_left + l_c[y] - x - 1] + 1;
		}
		if(end > c_width + 1){
			sprintf(last_error, _("\nC_make_start_position: Error overflow in column: %d, calculate column len: %d\n"), y+1, end);
			return false;
		}
	}
// заполняем таблицу конечных позиций элементов
	for (int x=0; x < c_width; x++){	// счетчик столбцов
		end = c_height;
		for (y=0; y < t_c[x]; y++){	// счетчик цифр в строчке
			end -= t_d[x * c_top + y]; // end = начало последнего элемента.
			c_en_pos[x * c_top + t_c[x] - y - 1] = end;// переносим в позицию последнего элемента
			end--;	// отступ в 1 клетку
		}
	}
	for (int y=0; y < c_height; y++){	// счетчик строк
		end = c_width;
		for (x=0; x < l_c[y]; x++){	// счетчик цифр в строчке
			end -= l_d[y * c_left + x]; // end = начало последнего элемента.
			c_en_pos[(y * c_left + l_c[y] - x - 1) + fild_offset] = end;// переносим в позицию последнего элемента
			end--;	// отступ в 1 клетку
		}
	}
	c_clear_errors();
	return true;
}

void c_reset_start_position(){
	size_t len = (c_top * c_width + c_left * c_height) + 1;
	memset(c_st_pos, 0, len);
	memset(c_en_pos, 0, len);
	c_make_start_position();
}

bool c_make_overlapping(byte *map, bool view){										// Быстрое заполнение "перекрытых" ячеек
	if(atom->isColor) return c_make_overlapping_color(map, view);
	c_make_start_position();
	int end, ptr, x, y;
	size_t offset, addr;
	bool res = false;
	for (x=0; x < c_width; x++)		// счетчик столбцов
		for(y=0; y < t_c[x]; y++){	// счетчик цифр в строчке
			offset = x * c_top + y;
			addr = x * c_top + t_c[x] - y - 1;
									// если    старт + длина > конец
				// старт			длина		конечная позиция
			if(c_st_pos[offset] + t_d[addr] > c_en_pos[offset])
				// то заполняем ячейки: конец - (старт+длина)
				for(end=c_en_pos[offset]; end < c_st_pos[offset] + t_d[addr]; end++){
					ptr = end * c_width + x;
					if(map[ptr] != BLACK){
						map[ptr] = BLACK;
						if(view) c_reg_solved_cell(x, end);
						res = true;
					}
				}
		}
	for (y=0; y < c_height; y++) // счетчик строк
		for(x=0; x < l_c[y]; x++){ // счетчик цифр в строчке
			// если    старт + длина > конец
			offset = fild_offset + y * c_left + x;	// адрес позиций во временном буфере
			addr = y * c_left + l_c[y] - x - 1;		// адрес цифры в оцифровке
			// если    старт + длина > конец
			if(c_st_pos[offset] + l_d[addr] > c_en_pos[offset])
				// то заполняем ячейки: конец - (старт+длина)
				for(end=c_en_pos[offset]; end < (c_st_pos[offset] + l_d[addr]); end++){
					ptr = end + c_width * y;
					if(map[ptr] != BLACK){
						map[ptr] = BLACK;
						if(view) c_reg_solved_cell(end, y);
						res = true;
					}
				}
		}
	if(!res) strcpy(last_error, _("C_make_overlapping: make nothing!\n"));
	return res;
}

void c_lesson_err_mess(int n, bool s){
	sprintf(last_error, _("Error in %s - %d"), s ? _("column") : _("line"), n+1);
	d_message(_("Lesson message"), last_error, false);
	calcerror = false;
	if(atom->isColor)
		 c_make_pos_and_access_color();
	else c_make_start_position();	// сброс стартовых позиций дабы не наступать  на устаревшие грабли
}

void c_lesson_mark(){
	for(int i=0; i<map_size; i++)
		if(shadow_map[i] != EMPTY && c_map[i] == EMPTY){
			p_mark_cell(i%c_width,i/c_width);
		}
}

void c_lesson(){
	if(solved) return;

	bool res = false;
	if(!animate){
		if(cr) cairo_destroy(cr);
		cr = NULL;
		p_draw_icon(false);
	}
	if(step_current >= 0 && step_current < xy_size){
		if(step_current < c_width)	p_make_column_imp	(step_current, false);	// очищаю выделение колонки-строки
		else						p_make_row_imp		(step_current-c_width, false);
	}
	if(c_check_complete(c_map))	{ step_current = -1; return;}

	if(step_current == -1){
		if(!c_check_digits_summ()) return;
		step_current = 0;
		for(int x=0; x < c_width; x++)	{x_complete[x] = true; x_error[x] = false;}	// сброс буферов отгаданных строк
		for(int y=0; y < c_height; y++)	{y_complete[y] = true; y_error[y] = false;}
	}

	for(int i=0; i<map_size; i++) // проецирую видимую матрицу на решаемую
		if(c_map[i] != EMPTY) shadow_map[i] = c_map[i];

	if(atom->isColor) c_recalc_access_color();
	else c_make_start_position();

	for(;step_current < c_width; step_current++)
		if(x_complete[step_current]){
			res=c_scan_line(shadow_map, step_current, true, true);
			if(calcerror) {c_lesson_err_mess(step_current+1, true); return;}
			if(res) break;
		}
	if(!res){
		for(;step_current < xy_size; step_current++)
			if(y_complete[step_current-c_width]){
				res=c_scan_line(shadow_map, step_current-c_width, false, true);
				if(calcerror) {c_lesson_err_mess(step_current-c_width+1, false); return;}
				if(res) break;
			}
		if(res)	{
			p_make_row_imp(step_current-c_width, true);
			c_lesson_mark();
		}
		else{
			step_current = 0;
			for(;step_current < c_width; step_current++)
				if(x_complete[step_current]){
					res=c_scan_line(shadow_map, step_current, true, true);
					if(calcerror) {c_lesson_err_mess(step_current, true); return;}
					if(res) break;
				}
			if(!res){
				for(;step_current < xy_size; step_current++)
					if(y_complete[step_current-c_width]){
						res=c_scan_line(shadow_map, step_current-c_width, false, true);
						if(calcerror) {c_lesson_err_mess(step_current-c_width, false); return;}
						if(res) break;
					}
				if(res){
					p_make_row_imp(step_current-c_width, true);
					c_lesson_mark();
				}
				else fprintf(stderr, _("No more logic continued!\n"));
			}
			else {
				p_make_column_imp(step_current, true);
				c_lesson_mark();
			}
		}
	}
	else {
		p_make_column_imp(step_current, true);
		c_lesson_mark();
	}
	u_after(false);
	if(res) {
		if(animate){
			if(!timer_id) timer_id = gtk_timeout_add(timeout, c_timer_tik, NULL);
		}
	}
	else {
		step_current = -1;
		if(c_check_complete(c_map)) return;
		d_message(_("Lesson message"), _("No more logic continued!"), false);
		return;
	}
}

void c_update_line(byte line, bool why){
	if(help_level > 1) p_update_line(line, why);
}

bool c_test_validate_cross(byte x, byte y){					// тест валидности строки и колонки выбранной точки
	if(!c_solve_buffers	|| !atom->digits_status) return false;
	bool upd;
	c_test_line_res r;
	validate = true;
	if(!c_test_line(c_map, x, true, &r)){
		if(!x_error[x]){
			x_error[x] = true;
			c_update_line(x, true);					// подсветка строки с ошибкой
		}
		validate = false;
	}
	else {
		upd = false;
		if(x_error[x]){
			x_error[x] = false;
			c_update_line(x, true);
			upd = true;
		}
		if(r.digits)	// освежаю оцифровку колонки
			p_make_column_imp(x, atom->top_col == x);
		if(r.matrix && !upd)
			c_update_line(x, true);
	}
	if(!c_test_line(c_map, y, false, &r)){
		if(!y_error[y]){
			y_error[y] = true;
			c_update_line(y, false);
		}
		validate = false;
	}
	else {
		upd = false;
		if(y_error[y]){
			y_error[y] = false;
			c_update_line(y, false);
			upd = true;
		}
		if(r.digits)	// освежаю оцифровку строки
			p_make_row_imp(y, atom->left_row == y);
		if(r.matrix && !upd)
			c_update_line(y, false);
	}
	return validate;
}

int  c_test_validate_matrix(){						// тест на валидность данных матрицы
	if(!c_solve_buffers	|| !atom->digits_status) return false;
	validate = true;
	bool upd;
	c_test_line_res r;
	int res = -1;
	int e_cnt = 0;									// номер столбца-строки
	for(int x=0; x<c_width; x++){					// на выходе номер столбца или строки+c_width
		if(!c_test_line(c_map, x, true, &r)){
			if(!x_error[x]){
				x_error[x] = true;
				c_update_line(x, true);				// подсветка строки с ошибкой
			}
			res = e_cnt;
			validate = false;
		}
		else {
			upd = false;
			if(x_error[x]){
				x_error[x] = false;
				c_update_line(x, true);
				upd = true;
			}
			if(r.digits)	// освежаю оцифровку колонки
				p_make_column_imp(x, atom->top_col == x);
			if(r.matrix && !upd)
				c_update_line(x, true);
		}
		e_cnt++;
	}
	for(int y=0; y<c_height; y++){
		if(!c_test_line(c_map, y, false, &r)){
			if(!y_error[y]){
				y_error[y] = true;
				c_update_line(y, false);
			}
			res = e_cnt;
			validate = false;
		}
		else {
			upd = false;
			if(y_error[y]){
				y_error[y] = false;
				c_update_line(y, false);
				upd = true;
			}
			if(r.digits)	// освежаю оцифровку строки
				p_make_row_imp(y, atom->left_row == y);
			if(r.matrix && !upd)
				c_update_line(y, false);
		}
		e_cnt++;
	}
	return res;										// return -1 All OK
}

int c_test_variant(byte *map, bool update){
	bool r, px, py;
	size_t x, y;
	int count = 0;
	do{	r = false;
		x=0, y=0;
		px=false, py=false;
		do{
			if(!px && x < c_width && x_complete[x]){
				r |= c_scan_line(map, x, true, update);
				if(calcerror) break;
			}
			if(!py && y < c_height && y_complete[y]){
				r |= c_scan_line(map, y, false, update);
				if(calcerror) break;
			}
			if(!px) {if(x >= c_width) {px = true;} else x++;}
			if(!py) {if(y >= c_height){py = true;} else y++;}
		} while(!px || !py);
		if(calcerror){
			puts("calcerror");
			return -1;
		}
//		printf("%d\t", count);
		if(count++ > 100) break;
	} while(r);
	if(!c_check_complete(shadow_map))
		return 1;
	return 0;
}

void *c_solve_no_logic_thread(){
	size_t cell;
	byte ads = help_level; help_level = 0;
	int res=true;
	bool repeat;
	memcpy(solved_map, c_map, map_size);
	do{
		repeat = false;
		cell = 0;
		for(; cell < map_size; cell++){
			if(solved_map[cell] != EMPTY) continue;
			memcpy(shadow_map, solved_map, map_size);
			printf("Mark cell: %d\n", (int)cell);
			shadow_map[cell] = BLACK;
			c_reset_start_position();
			res = c_test_variant(shadow_map, false);
			if(-1 == res) {
				solved_map[cell] = MARK;
				repeat = true;
			}
			else if(!res) break;
			memcpy(shadow_map, solved_map, map_size);
			shadow_map[cell] = MARK;
			c_reset_start_position();
			res = c_test_variant(shadow_map, false);
			if(-1 == res) {
				solved_map[cell] = BLACK;
				repeat = true;
			}
			else if(!res) break;
		}
		if(!res) break;
	} while(repeat);
	if(c_check_complete(shadow_map))
				memcpy(c_map, shadow_map, map_size);
	else {
				c_test_variant(solved_map, true);
				memcpy(c_map, solved_map, map_size);
				d_message("Sorry", "Do not solved", false);
	}
//	if(!animate){
	gdk_threads_enter();
	p_update_matrix();
	p_draw_icon(false);
	u_after(false);
	gdk_threads_leave();
//	}
	help_level = ads;
	thread_solve_no_logic = 0;
	puts("Thread solve_no_logic stopped");
	return NULL;
}

void c_solve_no_logic(){
	d_message(_("Seerching solved"), _("please wait"), false);
	if(thread_solve_no_logic){
		pthread_cancel(thread_solve_no_logic);
		thread_solve_no_logic = 0;
	}
	else {
		int result = pthread_create(&thread_solve_no_logic, NULL, c_solve_no_logic_thread, NULL);
		if (result != 0){
			perror("Creating thread");
			c_solve_no_logic_thread();		// если не вышло запустить вторым потоком
		}
	}
}

void c_solver(){
	if(thread_solve_no_logic){
		pthread_cancel(thread_solve_no_logic);
		thread_solve_no_logic = false;
		return;
	}
	int x, y;
	bool r;
	if(timer_id)
		if(!animate ||								// если анимация отменена
			(solv_cell_current != solv_cell_top)){	// если повторный вызов при действии анимации
		g_source_remove(timer_id);
		timer_id = 0;
		c_timer_tik(&x);
		start=0;
		memcpy(c_map, shadow_map, map_size);
		p_update_matrix();
		p_draw_icon(false);
		enable_toolbar_animate(true);
		return;
	}
	if(!demo_flag && lesson){c_lesson(); return;}

	c_reset_start_position();

	if(!c_check_digits_summ()){						// проверяю эквивалентность оцифровок
		d_message(_("Puzzle digits error!"), last_error, false);
		enable_toolbar_animate(true);
		if(demo_flag) m_demo_next();
		return;
	}

	if(c_check_complete(c_map)){					// проверяю окончание расчета
		enable_toolbar_animate(true);
		if(demo_flag) m_demo_next();
		return;
	}
	start = 0;

	c_clear_errors();
	r=false; for(x=0;x<map_size;x++)if(c_map[x]!=EMPTY){r=true;break;}
	if (r){
		memcpy(shadow_map,c_map,map_size);
//#define OTLADKA
#ifdef OTLADKA
		c_test_line_res res;
#endif //OTLADKA
		y = -1;
		for(x=0; x<c_width; x++){
#ifdef OTLADKA
			c_test_line(shadow_map, x, true, &res);
			if(calcerror || !res.legal) break;
#else
			c_scan_line(shadow_map, x, true, true);
			if(calcerror) break;
#endif //OTLADKA
		}
		for(y=0; y<c_height; y++){
#ifdef OTLADKA
			c_test_line(shadow_map, y, false, &res);
			if(calcerror || !res.legal) break;
#else
			c_scan_line(shadow_map, y, false, true);
			if(calcerror) break;
#endif //OTLADKA
		}
#ifdef OTLADKA
		if(calcerror || !res.legal){
#endif //OTLADKA
		if(calcerror){
			char *mess=malloc(256);
			sprintf(mess, _("Error in %s: %d\nContinued with cleared puzzle?"), y>0 ? _("line"):_("column"), y>0 ? y : x);
			if(d_message(_("Solver error!"), mess, true)){
				c_solve_init(false);
			}
			else {m_free(mess); return;}
			c_make_start_position();
			m_free(mess);
		}
	}
	else {
		memset(shadow_map, 0, map_size);
	}
	c_make_overlapping(shadow_map, true);							// запускаю make_overlap

	byte ads = help_level; help_level = 0;
	do{	r = false;
		if(atom->isColor) c_recalc_access_color();
		x=0, y=0;
		bool px=false, py=false;
		do{	if(x<c_width && x_complete[x])
				r |= c_scan_line(shadow_map, x, true, true);
			if(calcerror){c_error; break;}
			if(y<c_height && y_complete[y])
				r |= c_scan_line(shadow_map, y, false, true);
			if(calcerror){c_error; break;}
			x++; y++;
			if(x == c_width) {x = 0; px = true;}
			if(y == c_height){y = 0; py = true;}
		} while(!px || !py);
		if(calcerror) break;
	} while(r);

	if(!animate && !demo_flag){
		if(c_check_complete(shadow_map))c_clear_mark();
		memcpy(c_map, shadow_map, map_size);
		p_update_matrix();
		p_draw_icon(false);
	}

	if(!c_check_complete(shadow_map)){
//		if(atom->isColor) c_no_logic_color();
//	else
		thread_solve_no_logic = 0;
		c_solve_no_logic();
	}
	help_level = ads;
	return;
}

bool c_scan_line(byte *map, int number, bool that, bool source_update){
	if(atom->isColor) return c_scan_line_color(map, number, that, source_update);

	int bc;	// определитель колич. цифр оцифровки в строке оцифровки
	int volatile i, pos, cells, x;
	bool compl;

	int x_size = c_width, y_size = c_height;
	int line_len = that ? y_size : x_size;		// длина строки

	if(that)
		for(i = 0; i<line_len; i++)				// копируем строку в буфер
			orig[i] = map[i * x_size + number];
		else for(i = 0; i<line_len; i++)
			orig[i] = map[i + x_size * number];

	byte *pointer = that ? t_d + number * c_top : l_d + number * c_left;
	cells = bc = that ? t_c[number] : l_c[number];

	for (i=0; i < bc; i++)						// копируем текущую оцифровку в буфер
//		block[i] = pointer[i];
		block[i] = pointer[--cells];
	// orig 	- копия строки
	// line_len	- длина строки
	// block	- копия оцифровки строки
	// bc		- колич. цифр оцифровки

	byte *st_pos = c_st_pos, *en_pos = c_en_pos;
	pos = that ? number * c_top : number * c_left + x_size * c_top;
	st_pos += pos;	// определяю смещение на данные выбранной строки(столбца) в буферах позиций
	en_pos += pos;

// определяем начальные стартовые позиции
	int strt_curr = 0;
	compl=true;
	for(i=0; i < bc; i++){						// счетчик элементов
		int a;									// проверяем от стартовой до стоповой позиции
		byte b, c;
		calcerror = true;
		for(pos=strt_curr; pos<line_len; pos++){
												// i	 = номер элемента
            									// pos = указ. на начало
												// a   = указ. начало + длина
												// b   = значение первой ячейки за элементом
			a = pos+block[i];					// c   = значение первой ячейки перед элементом
			if(a>line_len || pos<0) {calcerror = true; break;}
			b = (byte)((a < line_len) ? orig[a] : EMPTY);
			c = (byte)((pos > 0) ? orig[pos-1] : EMPTY);
			int d = 0;
			if((b == MARK || b == EMPTY) && (c == MARK || c == EMPTY)){
				for(cells = pos; cells < a; cells++){ // от старта до конца
												// вынуждаем цыкл окончится, если внутри маркер
					if(orig[cells] == MARK) cells = line_len+1;
					else if(orig[cells] == BLACK) d++;
				}
			}
            else cells = line_len + 1;

            if(cells <= line_len){				// если вариант подходящий то обновляем
				if(d == block[i] && compl)		// стартовые значения в массиве
					st_pos[i] = en_pos[i] = pos;
				else compl = false;
				strt_curr = pos + block[i] + 1;
				for(x=i; x < bc; x++){			// счетчик оставшихся элем.
					if(st_pos[x] < pos) st_pos[x] = pos;
					else pos = st_pos[x];
//					pos += (i < bc-1 ? (int)block[x] + 1 : 0);
					pos += (x < bc-1 ? (int)block[x] + 1 : 0);
				}
				pos = line_len+1;				// позиция подходящая
				calcerror = false;
				break;							// вынуждаем цыкл закончится
			}
		}
		if(calcerror) {return false;}
	}

// определяем начальные стоповые позиции
	strt_curr = line_len - block[bc-1];
	compl = true;
	for(i = bc-1; i >= 0; i--){		// обратный счетчик элементов
		calcerror = true;// проверяем от стоповой до стартовой позиции
		for(pos=strt_curr; pos>=0; pos--){
										// i	 = номер элемента
										// pos = указ. на начало
										// c   = значение первой ячейки перед элементом
										// b   = значение первой ячейки за элементом
			int a = pos+(int)block[i];	// a   = указ. начало + длина
			if(a>line_len || pos<0) {calcerror = true; break;}
			byte b = (byte)((a < line_len) ? orig[a] : EMPTY);
			byte c = (byte)((pos > 0) ? orig[pos-1] : EMPTY);
			int d = 0;
			if((b == MARK || b == EMPTY) && (c == MARK || c == EMPTY)){
				for(cells = pos; cells < a; cells++) // от старта до конца
										// вынуждаем цыкл окончится, если внутри маркер
					if(orig[cells] == MARK) cells = line_len+1;
					else if(orig[cells] == BLACK) d++;
			}
			else cells = line_len+1;

			if(cells <= line_len){			// если вариант подходящий то обновляем
				if(d == block[i] && compl)
					st_pos[i] = en_pos[i] = pos;
				else compl = false;
				strt_curr = (i>0 ? pos - block[i-1] - 1 : strt_curr);
				for(x=i; x >= 0; x--){	// счетчик оставшихся элем.
					if(en_pos[x] > pos) en_pos[x] = pos;
					else pos = en_pos[x];
					pos -= (x > 0 ? ((int)block[x-1]+1) : 0);
				}
				pos = line_len+1;
				calcerror = false;
				break;	// вынуждаем цыкл окончится
			}
		}
		if(calcerror) {return false;}
	}

// небольшое добавление ограничивающее диапазон действия элемента от конца
// диапазона предыдущего - первой непустой ячейкой
	for(i=0; i<bc; i++){
		int a = i>0 ? en_pos[i-1]+block[i-1] : 0;	// a - граница диапазона пред. элемента
		for(pos=a; pos<line_len; pos++)
			if(orig[pos] == BLACK)
				if(en_pos[i] > pos)
					{ en_pos[i] = pos; break; }
	}

	for(i=bc-1; i>=0; i--){
		int a = i<bc-1 ? st_pos[i+1]-1 : line_len-1;	// a - граница диапазона пред. элемента
		for(pos=a; pos > 0; pos--)
			if(orig[pos] == BLACK)
				if(st_pos[i]+block[i]-1 < pos)
					{ st_pos[i] = pos-block[i]+1; break; }
	}

// Следующий этап - сканирование всех ячеек строки. При нахождении заполненной,
// определяется ее длина и происходит поиск элементов, которые могут ее перекрыть
// (по длине и местоположению), если оных всего один, то обновляются стартовые
// и стоповые индексы этого элемента, а затем и всех остальных.
// Если же длина совпадает со всеми перекрывающими элементами, то маркируем ее
// границы.
// P.S. Параллельно вышеописаному данный блок при нахождении подходящего элемента
// методом подстановки ищет непротиворечащее оригиналу положение этого элемента,
// с включением этой ячейки (или ячеек).
// При нахождении он увеличивает счетчики соответствующие каждой заполняемой ячейки
// и увеличиват общий счетчик вариантов. В итоге если к концу проверки величина
// какого-либо из счетчиков ячеек совпадает с общим счетчиком значит однозначно
// заливаем эту ячейку.

	int n_elem, elem, elem_len=0;
	bool repeat;//, skips = false;
	do	{
		repeat = false;
		for(i=0; i<line_len; i++)      // счетчик ячеек
		if(orig[i] != EMPTY && orig[i] != MARK){	// если ячейка окрашена
			n_elem = 0;
			x = i;                  // определяем длину
			while(x<line_len && orig[++x] == BLACK);
			x -= i;                 // х = длина
// здесь и далее отмечено то, что описано в P.S.
			int counter=0;	                                 		// обнуляем общий счетчик
			for(cells=0; cells<line_len; cells++) variants[cells] = 0;// обнуляем счетчики ячеек
			for(cells=0; cells<bc; cells++){	// счетчик элементов
								// если элем. длинее,   старт. раньше       и  кончается позже то он подходит
				if((block[cells] >= x) && (st_pos[cells] <= i) && (en_pos[cells]+block[cells] >= i+x)){
					n_elem++;       	// увеличиваем счетчик подходящих элементов
					elem = cells;  	// сохраняем номер элемента
					if(n_elem > 1 && elem_len != block[cells]) elem_len = line_len+1;
					else elem_len = block[cells];
					int d = MIN(MIN(i, line_len-block[cells]),en_pos[cells]);
					int e = MAX(MAX(i+x-block[cells], 0), st_pos[cells]);
					for(int a = e; a <= d; a++){// проверяем положение
						bool r = false;
						if((a > 0) && (orig[a-1]==BLACK)) r = true;
						if((a+block[cells] < line_len) && (orig[a+block[cells]]==BLACK)) r = true;
						if(r) continue; 	// прокрутка цикла если сбоку закрашеная ячейка

						for(int b=a; b<a+block[cells]; b++)
							{if(orig[b] == MARK) {r = true; break;}}
						if(r) continue; 	// прокрутка цикла если внутри попался маркер
                     // Ok - увеличиваем счетчики
						counter++;
						for(int b=a; b<a+block[cells]; b++)
							variants[b]++;
					}
				}
			}
            // сравниваем счетчики
			if(!counter) {calcerror = true; return false;}
			for(cells=0; cells<line_len; cells++)
				if(variants[cells] == counter){
					orig[cells] = BLACK;
					if(source_update) c_add_solved_cell(cells, number, that);
				}
			if(elem_len == x){       // если длина совпадает со всеми доступными элементами
				if(i>0){
					orig[i-1] = MARK;// то маркируем границы
					if(source_update && view_mark) c_add_solved_cell(i-1, number, that);
				}
				if((i+x)<line_len){
					orig[i+x] = MARK;
					if(source_update && view_mark) c_add_solved_cell(i+x, number, that);
				}
			}
			if(n_elem == 1){			// если элемент единственный
										// обновляем стартовые и стоповые позиции
				if(st_pos[elem] < (i+x-block[elem])) {st_pos[elem] = (i+x-block[elem]); repeat = true;}
				if(en_pos[elem] > i) {en_pos[elem] = i; repeat = true;}
				// обновляем стартовые и стоповые позиции остальных элементов
				for(cells=elem-1;cells>=0;cells--)
					if(en_pos[cells]+block[cells]+1>en_pos[cells+1])
						{en_pos[cells]=en_pos[cells+1]-block[cells]-1; repeat = true;}
				for(cells=elem+1;cells<bc;cells++)
					if(st_pos[cells]<st_pos[cells-1]+block[cells-1]+1)
						{st_pos[cells]=st_pos[cells-1]+block[cells-1]+1; repeat = true;}
			}
			else if(!n_elem) {calcerror = true; return false;}
			i += x;
		}
	} while(repeat);
// Данный блок проверяет количество одинаковых по длине блоков и сканирует
// строку в поиске однозначно определеных блоков данной длины. Если количество
// блоков в оцифровке соответствует количеству в строке то присваиваются и
// отождествляются их стартовые и конечные индексы
	int last_elem_len = 0;
	for(x=0; x<bc; x++){
		n_elem = 0;        											// подсчитываем количество одинаковых блоков в оцифровке
		if(last_elem_len == block[x]) continue;
		else last_elem_len = block[x];
		for(i=0; i<bc; i++) if(block[i] == block[x]) n_elem++;
		for(i=0; i<line_len; i++)									// сканируем строку в поиска заполненых ячеек
			if(orig[i] != EMPTY && orig[i] != MARK){				// если ячейка окрашена
				int a = i;                 							// определяем длину
				byte c = (byte)(i>0 ? orig[i-1] : MARK);			// c = значение ячейки перед элем.
				while(a<line_len && orig[++a] == BLACK);
				byte b = (byte)(a<line_len ? orig[a] : MARK);		// b = значение ячейки за элем.
				a -= i;                 							// a = длина
				if((b == MARK) && (c == MARK) && (a == block[x])) n_elem --;
				i += a;                    							// проверяем на совпадение длины
			}
		if(!n_elem){												// Если количество совпадает
			pos = 0;												// pos = позиция в строке
			int a = 0;   											// a = позиция в оцифровке
			while(a<bc && block[a]!=block[x]) a++;
			while(1){
				for(;pos<line_len;pos++)
					if(orig[pos] != EMPTY && orig[pos] != MARK){	// если ячейка окрашена
						int d = pos;                 // определяем длину
						byte c = (byte)(pos>0 ? orig[pos-1] : MARK);// c = значение ячейки перед элем.
						while(d<line_len && orig[++d] == BLACK);
						byte b = (byte)(d<line_len ? orig[d] : MARK);// b = значение ячейки за элем.
						d -= pos;                 	// a = длина
						if((b == MARK) && (c == MARK) && (d == block[x])){
							st_pos[a] = en_pos[a] = pos;
							while(a<bc && block[++a]!=block[x]);
							if(a == bc) break;
						}
						pos += d;
					}
				if(a == bc) break;
			}
		}
	}

// Теперь производится поиск элементов прижатых к маркеровке, но
// неизвесной длины и если она меньше наименьшего значения оцифровки, то элемент
// удлиняется.
// на практике не заметил результата деятельности этого блока
#ifdef ADVANCE
	int min_elem_len = 255;               // определяем длину минимального элемента
	for(i=0; i<bc; i++) if(min_elem_len > block[i]) min_elem_len = block[i];
	for(i=0; i<line_len; i++)				// сканируем строку в поиска заполненых ячеек
		if(orig[i] != EMPTY && orig[i] != MARK){	// если ячейка окрашена
			int a = i;                 	// определяем длину
			byte c = (byte)(i>0 ? orig[i-1] : MARK);		// c = значение ячейки перед элем.
			while(a<line_len && orig[++a] == BLACK);
			byte b = (byte)(a<line_len ? orig[a] : MARK);	// b = значение ячейки за элем.
			a -= i;                 		// a = длина
			if(((b == MARK && c == EMPTY) || (c == MARK && b == EMPTY)) && (a < min_elem_len)){
				if(b == MARK) x=i+a-min_elem_len;
				else            x=i;
				for(pos = 0; pos<min_elem_len; pos++)
					if(pos+x < line_len){
						orig[pos+x] = BLACK;
						if(source_update) c_add_solved_cell(pos+x, number, that);
						mess("Trey point\n");
					}
					else {calcerror = true; return false;}
			}
			i += a;                    	// проверяем на совпадение длины
		}
#endif //ADVANCE
// Теперь обновляем старт - стоп позиции методом подстановки
	strt_curr = st_pos[0];
// определяем начальные стартовые позиции
	for(i=0; i<bc; i++){					// счетчик элементов
		calcerror = true;					// проверяем от стартовой до стоповой позиции
		for(pos=strt_curr; pos<line_len; pos++){
				                       	// i	 = номер элемента
										// pos = указ. на начало
										// a   = указ. начало + длина
										// b   = значение первой ячейки за элементом
			int a = pos+block[i];  		// c   = значение первой ячейки перед элементом
			byte b = (byte)((a < line_len) ? orig[a] : EMPTY);
			byte c = (byte)((pos > 0) ? orig[pos-1] : EMPTY);
			if((b == MARK || b == EMPTY) && (c == MARK || c == EMPTY)){
				for(cells = pos; cells < a; cells++) // от старта до конца
										// вынуждаем цыкл окончится, если внутри маркер
					if(orig[cells] == MARK) cells = line_len+1;
               }
			else cells = line_len+1;

			if(cells <= line_len){			// если вариант подходящий то обновляем
               strt_curr = pos + block[i] + 1;	// стартовые значения в массиве
					for(x=i; x<bc; x++){	// счетчик оставшихся элем.
						if(st_pos[x] < pos) st_pos[x] = pos;
						else pos = st_pos[x];
						pos += (i<bc-1 ? (int)block[x]+1 : 0);
					}
				pos = line_len+1;
				calcerror = false;
				break;	// вынуждаем цыкл окончится
			}
		}
		if(calcerror) {return false;}
	}

// определяем начальные стоповые позиции
	strt_curr = en_pos[bc-1];
	for(i = bc-1; i >= 0; i--){		// обратный счетчик элементов
		calcerror = true;// проверяем от стоповой до стартовой позиции
		for(pos=strt_curr; pos>=0; pos--){	// i	 = номер элемента
											// pos = указ. на начало
											// c   = значение первой ячейки перед элементом
											// b   = значение первой ячейки за элементом
			int a = pos+(int)block[i];		// a   = указ. начало + длина
			byte b = (byte)((a < line_len) ? orig[a] : EMPTY);
			byte c = (byte)((pos > 0) ? orig[pos-1] : EMPTY);
			if((b == MARK || b == EMPTY) && (c == MARK || c == EMPTY)){
				for(cells = pos; cells < a; cells++) // от старта до конца
											// вынуждаем цыкл окончится, если внутри маркер
					if(orig[cells] == MARK) cells = line_len+1;
			}
			else cells = line_len+1;

			if(cells <= line_len){			// если вариант подходящий то обновляем
				strt_curr = (i>0 ? pos - block[i-1] - 1 : strt_curr);
					for(x=i; x >= 0; x--){	// счетчик оставшихся элем.
						if(en_pos[x] > pos) en_pos[x] = pos;
						else pos = en_pos[x];
					pos -= (x > 0 ? ((int)block[x-1]+1) : 0);
					}
				pos = line_len+1;
				calcerror = false;
				break;	// вынуждаем цыкл окончится
			}
		}
		if(calcerror) {return false;}
	}

// Теперь для каждого элемента отмечаются ячейки, которые могут быть заполнены
// только этим элементом. Проверяем эти ячейки на заполнение, при этом первая-же
// заполненая ячейка сравнивается со стоповой позицией и если она меньше то
// обновляется местоположением этой ячейки. Аналогично для последней заполненной
// ячейки диапазона действия этого элемента.

	int st_priv, en_priv;	// индексы начала и конца "частной зоны"
	for(i=0; i<bc; i++){			// счетчик элементов
									// st_priv = конечная позиция предыдущ. элем.
		st_priv = ((i>0) ? (en_pos[i-1] + block[i-1]) : st_pos[0]);
		en_priv   = ((i<(bc-1)) ? st_pos[i+1]-1 : (en_pos[i]+block[i]-1));
									// en_priv   = стартовая позиция след. ячейки
		for(x=st_priv; x<=en_priv; x++){// Проверяем ячейки на заполнение
			if(orig[x]!= EMPTY && orig[x]!= MARK){
				if(en_pos[i] > x) en_pos[i] = x;
				x = line_len+1;
				break;
			}
		}
		for(x=en_priv; x>=st_priv; x--){// Проверяем ячейки на заполнение
			if(orig[x]!= EMPTY && orig[x]!= MARK){
				if(st_pos[i] < (x-block[i])) st_pos[i] = x-block[i];
				x = line_len+1;
				break;
			}
		}
	}

// Теперь обобщим полученные данные прогоном от стартовой до стоповой позиции и
// если подходящий вариант один однозначно определим старт и стоп индексы
	for(i=0; i<bc; i++){
		elem = elem_len = 0;
		for(pos = st_pos[i]; pos <=en_pos[i]; pos++){
											// i	 = номер элемента
											// pos = указ. на начало
											// a   = указ. начало + длина
											// b   = значение первой ячейки за элементом
			int a = pos+block[i];  			// c   = значение первой ячейки перед элементом
			byte b = (byte)((a < line_len) ? orig[a] : EMPTY);
			byte c = (byte)((pos > 0) ? orig[pos-1] : EMPTY);
			cells = -1;
			if((b == MARK || b == EMPTY) && (c == MARK || c == EMPTY)){
				for(cells = pos; cells < a; cells++) // от старта до конца
											// вынуждаем цыкл окончится, если внутри маркер
					if(orig[cells] == MARK) cells = line_len+1;
			}
			else cells = line_len+1;

			if(cells <= line_len){
				elem = pos;
				elem_len++;
			}
		}
		if(elem_len == 1) st_pos[i] = en_pos[i] = elem;
		else if(!elem_len) {calcerror=true; return false;}
	}

// Далее отмечаются все перекрытые ячейки, а затем создается пустая строка, заново
// прогоняются все варианты от стартовой до стоповой позиции для всех элементов
// и при правильном варианте маркируются соответствующие элементы этой строки.
// Те ячейки, которые останутся неотмечеными маркируются в оригинале как пустые.

	for(i=0; i<bc; i++)				// отмечаем перекрытые ячейки
		for(x=en_pos[i]; x<(st_pos[i]+block[i]); x++){
			orig[x] = BLACK;
			if(source_update) c_add_solved_cell(x, number, that);
		}
									// обнуляем буфер (пустая строка)
	for(i=0; i<line_len; i++) c_buffer[i] = EMPTY;
	for(i=0; i < bc; i++)				// счетчик элементов
		for(pos=st_pos[i]; pos<=en_pos[i]; pos++){	// счетчик доступных позиций
			int a = pos+(int)block[i]; // a   = указ. начало + длина
			byte b = (byte)((a < line_len) ? orig[a] : EMPTY);
			byte c = (byte)((pos > 0) ? orig[pos-1] : EMPTY);
			if((b == MARK || b == EMPTY) && (c == MARK || c == EMPTY)){
				for(cells = pos; cells < a; cells++) // от старта до конца
									// вынуждаем цыкл окончится, если внутри маркер
         			if(orig[cells] == MARK) cells = line_len+1;
			}
			else cells = line_len+1;
			if(cells<=line_len)		// если позиция непротиворечащая - заливаем ячейки строки
				for(cells = pos; cells < a; cells++) c_buffer[cells] = BLACK;
		}							// маркируем пустые ячейки в строке оригинала
	for(i=0; i<line_len; i++) if(c_buffer[i] == EMPTY){
		orig[i] = MARK;
		if(source_update && view_mark) c_add_solved_cell(i, number, that);
	}
	bool res = false;				// копируем полученную строку назад в матрицу
	pos = 0;
	for(i=0; i<line_len; i++){
		if(orig[i] != EMPTY) pos ++;
		x = that ? (i*x_size+number) : (i+x_size*number);
		if((map[x]) != orig[i]){
			if(that)	y_complete[i]=true;
			else		x_complete[i]=true;
			map[x] = orig[i];
			res = true;
		}
	}								// сбрасываем флажки для дальнейшего игнорирования
	if(/*!no_logic && */(!res || pos == line_len)){
		if(that) x_complete[number] = false;	// отгаданых строк
		else     y_complete[number] = false;
	}
//	byte	*markers= that ?	atom->top.colors + number * c_top:
//								atom->left.colors+ number * c_left;

//	if(!no_logic && help_level > 0)	// зачеркивание цифр
//		for(i=0; i<bc; i++)
//			if(st_pos[i] == en_pos[i])
//				markers[bc-i-1] |= COLOR_MARK;
	return res;
}

bool c_test_line(byte *map,
				int numb,					//номер строки
				bool orient,				//true - колонки, false - строки
				c_test_line_res *result		//указка на результат
				){
	if(atom->isColor) return c_test_line_color(map, numb, orient,result);
	static c_test_line_res res;
	if(result == NULL) result = &res;
	result->legal = result->digits = result->matrix = result->complete = false;
	int var, numb_var, v1, i, pos, el, cell;
	bool cur_place;
	byte *line = orig;
	int bc = orient ? t_c[numb] : l_c[numb];
	int line_len = orient ? c_height: c_width;
	byte	*digits = orient ?	t_d + numb * c_top :
									l_d + numb * c_left,
			*markers= orient ?	atom->top.colors + numb * c_top:
									atom->left.colors+ numb * c_left;
	if(orient)
		for(int c=0; c < c_height; c++)				// копирование столбца в буфер
			orig[c_height - c - 1] = map[numb + c_width * c];
	else
		for(int c=0; c < c_width; c++)				// копирование строки в буфер
			orig[c_width - c - 1] = map[numb * c_width + c];

	if(bc == 0){									// Выход, если пустая строка
		for(i=0; i<line_len; i++)
			if(line[i]!=EMPTY && line[i]!=MARK) return false;
		if(help_level > 1){
			for(i=0; i<line_len; i++)
				if(line[i] != MARK){
					line[i] = MARK;
					result->matrix = true;
				}
		}
		result->complete = true;
		return result->legal = true;
	}
// Часть 1 - расчет начальных-конечных позиций элементов и маркировка оцифровки
	// делаю копию старт-стоп позиций строки
	// определяю смещение на данные выбранной строки(столбца) в буферах позиций
	pos =  orient ? numb * c_top : numb * c_left + c_width * c_top;
	size_t len = (orient ? c_top : c_left) + 1;
	byte *start = MALLOC_BYTE(len),
		 *end	= MALLOC_BYTE(len);
	memcpy(start, c_st_pos + pos, len);
	memcpy(end, c_en_pos + pos, len);

	int space = line_len - bc + 1,					// количество пустоты в строке
		count = 0;

	for(i=0; i < bc; i++){							// Расчитываем начальные стартовые позиции
		space -= digits[i];
		start[i] = count++;
		count += digits[i];
	}
	for(i=0; i < bc; i++)							// начальные стоповые позиции
		end[i] = start[i] + space;

// прогоняем все элементы подстановкой и обн.старт.поз
	for(el=0; el<bc; el++){							// счетчик блоков
		for(int pos=start[el]; pos<=end[el]; pos++){// счетчик свободного пространства подстановки
			cur_place = true;
			if(pos > 0)								// проверка ячеек перед и за элементом
				if(line[pos-1]!=EMPTY && line[pos-1]!=MARK){
					cur_place=false; continue; }
			var = pos  + digits[el];
			if(var > line_len){
				free(start); free(end);
				return result->legal = false;		// выход если блок вылазит за границы матрицы
			}
			if(var < line_len)
				if(line[var]!=EMPTY && line[var]!=MARK){
					cur_place=false; continue;}
			for(int ptr=pos; ptr < var; ptr++){		// счетчик тела элемента
				if(line[ptr] == MARK){
					cur_place=false; break;}
			}
			if(cur_place){							// если место подходящее
				var = pos - start[el];				// обновляем стартовые позиции, на первом подх.месте
				for(i=el; i<bc; i++){
					start[i] += var;
					if(start[i] >= line_len){
						free(start); free(end);
						return result->legal = false;
					}
				}
				break;
			}
		}
	if(!cur_place){
		free(start); free(end);
		return result->legal = false;
		}	// выход если для блока не нашлось места
	}
// теперь то-же самое для стоповых позиций
	for(int el=bc-1; el>=0; el--){					// счетчик элементов
		for(int pos=end[el]; pos>=start[el]; pos--){// счетчик свободного пространства подстановки
			cur_place = true;
			if(pos > 0)								// проверка ячеек перед и за элементом
				if(line[pos-1]!=EMPTY && line[pos-1]!=MARK){
					cur_place=false; continue;}
			var = pos  + digits[el];
			if(var > line_len){
				free(start); free(end);
				return result->legal = false;		// брэк если блок вылазит за границы матрицы
			}
			if(var < line_len)
				if(line[var]!=EMPTY && line[var]!=MARK){
					cur_place=false; continue;}
			for(int ptr = pos; ptr < var; ptr++){	// счетчик тела элемента
				if(line[ptr]==MARK){
					cur_place=false; break;}
			}
			if(cur_place){							// если место подходящее
				var = end[el] - pos;
				for(i=el; i>=0; i--){
					end[i]-=var;
					if(end[i] < 0){
						free(start); free(end);
						return result->legal = false;
					}
				}
				break;
			}
		}
	if(!cur_place){
		free(start); free(end);
		return result->legal = false;
		}	// выход если для блока не нашлось места
	}
// ограничиваю диапазон действия блока от конца
// диапазона предыдущего - первой непустой ячейкой
	for(el=0; el<bc; el++){
		int a = el>0 ? end[el-1]+digits[el-1] : 0;	// a - граница диапазона пред. элемента
		for(pos=a; pos<line_len; pos++)
			if(line[pos] == BLACK)
				if(end[el] > pos){
					end[el] = pos;
					if(end[el] < start[el]){
						free(start); free(end);
						return result->legal = false;
					}
					break;
				}
	}

	for(el=bc-1; el>=0; el--){
		int a = el<bc-1 ? start[el+1]-1 : line_len-1;	// a - граница диапазона пред. элемента
		for(pos=a; pos > 0; pos--)
			if(line[pos] == BLACK)
				if(start[el]+digits[el]-1 < pos){
					start[el] = pos-digits[el]+1;
					if(end[el] < start[el]){
						free(start); free(end);
						return result->legal = false;
					}
					break;
				}
	}

// **************** шаг 2 - проверка отмеченых от краев элементов **************
	int s=0, e=line_len-1;
	for(numb_var=0; numb_var<bc; numb_var++){		// с начала строки
		while((line[s]==MARK) && (s<line_len)) s++;	// подгонка к первой ячейке,
		if((s <line_len)&&(line[s]==BLACK)){		// не отмеченной маркером
			cur_place=true;
			for(v1=s; v1<s+digits[numb_var]; v1++)
				if(line[v1]!=BLACK) cur_place=false;
			if(cur_place && (line[v1++]!=BLACK)){
				end[numb_var] = start[numb_var];
			}
			else break;
			s=v1;
		}
		else break;
	}
	for(numb_var=bc-1; numb_var>=0; numb_var--){	// с конца строки
		while((line[e]==MARK) && (e>=0)) e--;
		if((e>=0) && (line[e]==BLACK)){
			cur_place=true;
			for(v1=e; v1>e-digits[numb_var]; v1--)
				if(line[v1]!=BLACK) cur_place=false;
			if(cur_place && (line[v1--]!=BLACK)){
				start[numb_var] = end[numb_var];
			}
			else break;
			e=v1;
		}
		else break;
	}
// ********** Шаг 3 поиск единственного самого длинного элемента ***************
	if(help_level > 1){
		int save = 0;
		numb_var = var = 0;
		for(v1 = 0; v1 < bc; v1++)					// ищем самый длинный элемент
			if(digits[v1] > var) var = digits[v1];
		for(v1 = 0; v1 < bc; v1++)					// проверяем его единственность
			if(digits[v1] == var)
				{numb_var++; save = v1;}
		if(numb_var == 1){							// выполняем поиск его отметки в строке
			for(s = 0; s < line_len; s++)			// поиск ближайшей занятой ячейки
				if(line[s]==BLACK) break;
			if(s + var <= line_len){				// если позиция + длина элем. не вылазит за пределы строки
				cur_place = true;					// var - длина блока
				for(e = s; e < s + var; e++)		// s - индекс начала элемента в строке
					if(line[e]!=BLACK)
						{s = e; cur_place = false; break;}
				if(e < line_len && line[e]==BLACK) cur_place=false;	// что за блоком?

				if(cur_place){
					if(s >= start[save] && s <= end[save]){		// вписывается-ли найденный блок в свои рамки
						start[save] = end[save];
		}	}	}	}

/*/ при нахождении точного месторасположения блока
		for(el=0; el < bc; el++){
			if(start[el] == end[el]){
				v1 = start[el]+digits[el];
				cur_place = true;
				for(i=start[el]; i<v1; i++)
					if(line[i] != BLACK){
						cur_place = false;
						if(help_level > 3){
							line[i] = BLACK;
							result->matrix = true;
						}
					}
				if(help_level > 0 && cur_place && !(markers[el] & COLOR_MARK)){
						markers[el] |= COLOR_MARK;
						result->digits = true;
					}
				if(help_level > 1){
					if( start[el] > 0 && line[start[el]-1] != MARK){
						line[start[el]-1] = MARK;
						result->matrix = true;
					}
					if(v1 < line_len && line[v1] != MARK){
						line[v1] = MARK;
						result->matrix = true;
					}
				}
			}
			else if(help_level > 0 && (markers[el] & COLOR_MARK)){
					markers[el] &= COL_MARK_MASK;	// снимаю маркер оцировки если позиция неоднозначна
					result->digits = true;
				}
		}*/
	}
// ******* Шаг 4 поиск однозначно пустых и однозначно заполненных ячеек *********
	if(help_level > 3){
		for(el=0; el<bc; el++){
			if(digits[el] > end[el]-start[el]){	// если длина блока перекрывает его конечную позицию
				for(int cell = end[el]; cell < start[el] + digits[el]; cell++)
					if(line[cell] != BLACK){
						line[cell] = BLACK;
						result->matrix = true;
					}
			}
		}
	}
	if(help_level > 2){
		for(i=0; i<line_len; c_buffer[i++] = 0);
		for(el=0; el < bc; el++)
			for(cell=start[el]; cell < end[el] + digits[el]; cell++)
				c_buffer[cell]++;
		for(i=0; i<line_len; i++)
			if(c_buffer[i] == 0 && line[i] != MARK){
				line[i] = MARK;
				result->matrix = true;
			}
	}

// ***** Шаг 5 - проверка строки на правильную прорисовку всех элементов *******
	cur_place = true;
	var = 0;										// счетчик ячеек строки
	for(numb_var=0; numb_var<bc; numb_var++){		// счетчик элементов
		while(line[var] != BLACK)					// поиск первой залитой ячейки
			if(++var >= line_len) break;			// прерывание если конец строки
		if(var >= line_len) break;
		v1 = var + digits[numb_var];
		while(var < v1)								// проверка верности прорисовки
			if(line[var++] != BLACK){
				cur_place = false;
				break;
			}
		if(!cur_place) break;						// прерывание если несоответсвие прорисовки элементу
		if(var < line_len){							// если не конец строки
			if(line[var++] == BLACK){				// поправка на межелементный разрыв
				cur_place = false;
				break;
			}
		}
	}
	if(cur_place){
		while(var < line_len)						// проверка остатка строки на отсутствие зарисовки
			if(line[var++]==BLACK) cur_place=false;
		if(cur_place && (numb_var == bc)){
			result->complete = true;
			if(help_level > 0){
				for(v1=0; v1<bc; v1++){				// отмечаю всю оцифровку
					if(!(markers[v1] & COLOR_MARK)){
						markers[v1] |= COLOR_MARK;
						result->digits = true;
					}
				}
			}
			if(help_level > 1){
				for(v1=0; v1<line_len; v1++)		// дополняю отгаданную строку маркерами
					if(line[v1] == EMPTY){
						line[v1] = MARK;
						result->matrix = true;
					}
			}
		}
	}
	if(help_level > 0){
		for(v1=0; v1<bc; v1++){						// обновляю данные
			cur_place = false;
			if(start[v1] == end[v1]){
				cur_place = true;
				for(var = start[v1]; var < (start[v1]+digits[v1]); var++)
					if(line[var] != BLACK) cur_place = false;
			}
			if(cur_place){
				if(help_level > 0 && !(markers[v1] & COLOR_MARK)){
					markers[v1] |= COLOR_MARK; result->digits = true;}
				if(help_level > 2){
					if(start[v1] > 0 && line[start[v1]-1] != MARK){
						line[start[v1]-1] = MARK;
						result->matrix = true;
					}
					if(start[v1]+digits[v1] < line_len &&\
									line[start[v1]+digits[v1]] != MARK){
						line[start[v1]+digits[v1]] = MARK;
						result->matrix = true;
					}
				}
			}
			else if(help_level > 0 && (markers[v1] & COLOR_MARK)){
				markers[v1] &= COL_MARK_MASK; result->digits = true;}
		}
		if(orient){
			for(int c=0; c < c_height; c++)			// возврат столбца в матрицу
				map[numb + c_width * c] = orig[c_height - c - 1];
		}
		else{
			for(int c=0; c < c_width; c++)
				map[numb * c_width + c] = orig[c_width - c - 1];
		}
	}
	free(start); free(end);
	return result->legal = true;
}

uint64_t c_calc_numb_var(int module, int range){
/*
Следующая процедура расчитывает количество вариантов подстановки в пустой строке.
Искомое равно сумме значений чисел снизу и слева от искомого плюс 1
(см. таблицу)
6,		27,		83,		209,	461,	923,
5,		20,		55,		125,	251,	461,
4,		14,		34,		69,		125,	209,	Модуль /амплитуда/
3,		9,		19,		34,		55,		83,   (количество пространства строки для перемещения элементов
2,		5,		9,		14,		20,		27,    = длина стр. - суммарн. длина всех эл. - колич.эл. + 1)
1,		2,		3,		4,		5,		6,
			разрядность (количество элементов в строке)
*/
	if(!module || !range) return 0;
	uint64_t *mass = malloc(sizeof(uint64_t) * module);	// массив хранения промежуточных результатов
	int i,m;
	for(i=0; i<module; i++)
		mass[i]=((uint64_t)i)+1;			// заполняем массив начальными данными
	for(i=1; i<range; i++){
		(*mass)++;
		for(m=1; m<module; m++)
			mass[m] += mass[m-1]+1;
	}
	uint64_t res = mass[module-1];
	free(mass);
	return res;
}

//**********************************************************************//
//**********************************************************************//
//*********************** Colors function ******************************//
//**********************************************************************//
//**********************************************************************//
bool c_test_cell_color_in_matrix(int adr, byte clr){
	byte *ac = access_color + adr * MAX_COLOR;
	byte *map = shadow_map;
	byte color = clr & COL_MARK_MASK;
	if((map[adr]!=EMPTY) && (map[adr] != color))
			return false;
	for(int i=0; i<MAX_COLOR; i++){
		if(ac[i] == color)
			return true;
		else if(ac[i] == 0)
			return false;
		}
	return false;
}

bool c_test_cell_color_in_buffer(int adr, int clrnumb, bool that, int number){	// проверка ячейки на доступный цвет
	byte *ac = access_color + ((that ? number + c_width * adr : number * c_width + adr) * MAX_COLOR);
	byte color = cblock[clrnumb] & COL_MARK_MASK;

	if((orig[adr] != EMPTY && orig[adr] != color))		return false;
	for(int i=0; i<MAX_COLOR; i++) if(ac[i] == color)	return true;
	return false;
}

void c_copy_dig_buff(int num, bool that){	// копирование оцифровки и цветов в буфера block cblock
	int indx;
	if(that){
		for(int i=0; i < t_c[num]; i++){			// копируем текущую оцифровку в буфер
			indx = num * c_top + t_c[num] - 1 - i;	// и ее цвет
			block[i] = t_d[indx];					// block - оцифровка
			cblock[i] = t_clr[indx]&COL_MARK_MASK;// cblock - цвет оцифровки
		}
	}
	else {
		for(int i=0; i < l_c[num]; i++){			// копируем текущую оцифровку в буфер
			indx = num * c_left + l_c[num] - 1 - i;// и ее цвет
			block[i] = l_d[indx];					// block - оцифровка
			cblock[i] = l_clr[indx]&COL_MARK_MASK;// cblock - цвет оцифровки
		}
	}
}

/*bool c_recalc_position_color(void){		// функция перерасчета стартовых и стоповых позиций цветного кроссворда
	int bc, elem, pos, cells, indx;
	bool a;
	byte *st_pos, *en_pos;

	for(int row = 0; row < c_width; row++){			// счетчик колонок
		c_copy_dig_buff(row, true);					// копирую оцифровку в буфер
		bc = t_c[row];								// bc - кол. цифр
		indx = row * c_top;
		st_pos = c_st_pos + indx;
		en_pos = c_en_pos + indx;

		// Сканируем стартовые позиции
		for(elem = 0; elem < bc; elem++){			// сч. элем.
			for(pos=st_pos[elem];pos <= en_pos[elem]; pos++){ // от начальной до конечной позиции
				a=true;
				for(cells = pos; cells < pos+block[elem]; cells++)
					if(!c_test_cell_color_in_matrix(row + c_width * cells, cblock[elem])){
						a = false;					// если в границах варианта посторонний цвет
						break;
					}
				if(a) {
					if(st_pos[elem] < pos){
						st_pos[elem] = pos;
						for(int el = elem+1; el < bc; el++){// обновляем поз. сл. эл-в
							int off = cblock[el-1] == cblock[el] ?
								st_pos[el-1] + block[el-1]+1 :
								st_pos[el-1] + block[el-1];
							st_pos[el] = MAX(st_pos[el], off);
						}
						if(st_pos[bc - 1] + block[bc-1] > c_height){
							sprintf(last_error, "C_make_start_position_color: выход за границы матрицы в столбце %d\n", row);
							return false;				// выход за границы матрицы
						}
					}
					break;								// брек если найдено подходящее место
				}
			}
		}

		// Сканируем стоповые позиции
		for(elem = bc-1; elem>=0; elem--){						// сч. элем.
			for(pos=en_pos[elem]; pos>=st_pos[elem]; pos--){// пространство блока
				a=true;
				for(cells = pos; cells < pos + block[elem]; cells++)// пространство блока в позиции
					if(!c_test_cell_color_in_matrix(row + c_width * cells, cblock[elem])){
						a = false;
						break;
						}
				if(a) {
					if(en_pos[elem] > pos){
						en_pos[elem] = pos;
						for(int el=elem-1; el >= 0; el--){// обновляем поз. сл. эл-в
							int off = cblock[el+1]==cblock[el] ?
									en_pos[el+1]-block[el]-1 :
									en_pos[el+1]-block[el];
							en_pos[el] = MIN(en_pos[el], off);
						}
						if(en_pos[0] < 0){
							sprintf(last_error, "C_make_start_position_color: выход за границы матрицы в столбце %d\n", row);
							return false;
						}
					}
					break;
				}
			}
		}
	}
// Делаем тоже самое для строк
	for(int line = 0; line < c_height; line++){
		c_copy_dig_buff(line, false);						// копирую оцифровку в буфер
		bc = l_c[line];										// bc - кол. цифр
		indx = line * c_left + fild_offset;
		st_pos = c_st_pos + indx;
		en_pos = c_en_pos + indx;

		// Сканируем стартовые позиции
		for(elem = 0; elem < bc; elem++){			// сч. элем.
			for(pos=st_pos[elem]; pos<=en_pos[elem]; pos++){
				a=true;
				for(cells = pos; cells < pos + block[elem]; cells++)
					if(!c_test_cell_color_in_matrix(line * c_width + cells, cblock[elem])){
						a=false;
						break;
						}
				if(a){
					if(st_pos[elem]<pos){
						st_pos[elem] = pos;
						for(int el = elem+1; el < bc; el++){	// обновляем поз. сл. эл-в
							int off = cblock[el-1]==cblock[el] ?
								st_pos[el - 1] + block[el-1] + 1 :
								st_pos[el - 1] + block[el-1];
							st_pos[el] = MAX(st_pos[el], off);
						}
						if(st_pos[bc - 1] + block[bc-1] > c_width){
							sprintf(last_error, _("C_make_start_position_color: выход за границы матрицы в строке %d\n"), line);
							return false;
						}
					}
					break;
				}
			}
		}
		// Сканируем стоповые позиции
		for(elem = bc-1; elem>=0; elem--){// сч. элем.
			for(pos=en_pos[elem];	pos>=st_pos[elem]; pos--){
				a=true;
				for(cells = pos; cells < pos + block[elem]; cells++)
					if(!c_test_cell_color_in_matrix(line * c_width + cells, cblock[elem])){
						a=false;
						break;
						}
				if(a) {
					if(en_pos[elem]>pos){
						en_pos[elem] = pos;
						for(int el=elem-1; el>=0; el--){	// обновляем поз. сл. эл-в
							int off = cblock[el+1] == cblock[el] ?
								en_pos[el + 1] - block[el] - 1 :
								en_pos[el + 1] - block[el];
							en_pos[el] = MIN(en_pos[el], off);
						}
						if(en_pos[indx] < 0){
							sprintf(last_error, _("C_make_start_position_color: выход за границы матрицы в строке %d\n"), line);
							return false;
						}
					}
					break;
				}
			}
		}
	}
	return true;
}
*/

bool c_recalc_access_color(void){// определение доступных цветов ячейки после цикла обсчета
	int bc, elem, i, c, cells;
	byte *st_pos, *en_pos;
	int colors_count = 0;
	for(i = 0; i<map_size*MAX_COLOR; i++) if(access_color[i]) colors_count++;
	memset(access_color, 0, map_size*MAX_COLOR);

	for(int row = 0; row < c_width; row++){				// обрабатываю столбцы
		c_copy_dig_buff(row, true);
		bc = t_c[row];
		st_pos = c_st_pos + row*c_top;
		en_pos = c_en_pos + row*c_top;

		for(elem = 0; elem < bc; elem++){				// Прогоняю все элементы
														// от стартовой до финишной позиции
			for(cells=st_pos[elem]; cells<en_pos[elem]+block[elem]; cells++){
				byte *cptr = access_color+(cells*c_width+row)*MAX_COLOR;
				for(i=0; i<MAX_COLOR; i++){				// если цвет уже доступен
					if((int)cptr[i]==cblock[elem]) break;
					else if(!cptr[i]){
						cptr[i] = cblock[elem];
						break;
					}
				}
			}
		}
	}
	byte *acc_clr = malloc(c_width*MAX_COLOR);			// обрабатываю строки
	for(int line = 0; line < c_height; line++){
		memset(acc_clr, 0, c_width * MAX_COLOR);
		c_copy_dig_buff(line, false);
		bc = l_c[line];
		st_pos = c_st_pos + line*c_left + fild_offset;
		en_pos = c_en_pos + line*c_left + fild_offset;

		for(elem = 0; elem < bc; elem++){				// Прогоняю все элементы
														// от стартовой до финишной позиции
			for(cells=st_pos[elem]; cells < en_pos[elem]+block[elem]; cells++){
				byte *cptr = acc_clr + cells * MAX_COLOR;
				for(i=0; i<MAX_COLOR; i++){				// если цвет уже доступен
					if((int)cptr[i] == cblock[elem]) break;
					else if(!cptr[i]){
						cptr[i] = cblock[elem];
						break;
					}
				}
			}
		}

		for(int cells=0; cells<c_width; cells++){		// исключаю недоступные при "скрещивании" цвета
			byte *orig = access_color + (line * c_width + cells) * MAX_COLOR;
			byte *test = acc_clr + cells * MAX_COLOR;
			int ocount = -1; while(orig[++ocount]);
			int tcount = -1; while(test[++tcount]);
			for(i=0; i<ocount; i++){
				bool r = false;
				for(c=0; c<tcount; c++)
					if(orig[i] == test[c]) {r=true; break;}
				if(!r){
					orig[i]=0;
					x_complete[cells]=true;
					y_complete[line]=true;
				}
			}
			test = orig;
			for(i=0; i<MAX_COLOR; i++) // уплотняем цвета
				if(*test) *(orig++)=*(test++);
				else test++;
			while(orig<test) *(orig++)=0;
		}
	}
	m_free(acc_clr);
	for(i = 0; i < c_width*c_height*MAX_COLOR; i++)
		if(access_color[i]) colors_count--;
   return (bool)colors_count;
}

bool c_make_pos_and_access_color(void){					// определение доступных цветов ячеек
	int ptr, elem, i, c, bc;
	memset(access_color, 0, map_size * MAX_COLOR);
	byte *st_pos, *en_pos;
	for(int row = 0; row < c_width; row++){				// обрабатываем столбцы
		c = c_top;
		while (c)	if(t_d[(c-1) + row * c_top]) break; // копируем оцифровку в буфер block
					else c--;
		bc = c;											// заполняем переменную количества блоков
		for (i=0; i<bc; i++){							// копируем текущую оцифровку в буфер
			block[i]	= t_d[(--c) + row * c_top];
			cblock[i]	= t_clr[c + row * c_top] & COL_MARK_MASK;
		}
		st_pos = c_st_pos + row * c_top;
		en_pos = c_en_pos + row * c_top;
		// Подсчитываем количество свободного места в столбце
		int a_free = block[0];
		for(i=1; i < bc; i++){
			a_free += block[i];
			if(cblock[i] == cblock[i-1]) a_free++;
		}
		a_free = c_height - a_free;

		// определяем стартовые и конечные позиции столбцов
		ptr = 0;
		for(elem = 0; elem < bc; elem++){
			if(((bool)elem) && cblock[elem] == cblock[elem-1]) ptr++;
			st_pos[elem] = ptr;
			ptr += block[elem];
			if(ptr>c_height){
				sprintf(last_error, "C_make_access_color: Error make c_st_pos, column %d overflow", row);
				return false;				// переполнение строки
			}
		}
		ptr = c_height;
		for(elem = bc - 1; elem >=0; elem--){
			if((elem < (bc - 1)) && cblock[elem] == cblock[elem + 1]) ptr--;
			ptr -= block[elem];
			if(ptr >= 0) en_pos[elem]=ptr;
			else{
				sprintf(last_error, "C_make_access_color: Error make c_en_pos, column %d overflow", row);
				return false;
			}
		}

		// Прогоняем подстановку от начала до конца строки
		for(int var = 0; var <= a_free; var++){
			memset(variants, 0, sizeof(int) * c_height);
			ptr = var;
			for(elem = 0; elem < bc; elem++){			// создаем вариант в буфере
				if(((bool)elem) && cblock[elem] == cblock[elem-1]) ptr++;
				for(int elemlen=0; elemlen < block[elem]; elemlen++)
					variants[ptr++]=(int)cblock[elem];
			}
			for(int cells=0; cells < c_height; cells++)	// опр. цвета ячеек
				if(variants[cells]){						// указатель на строку доступных цветов
					byte *cptr = access_color + (cells * c_width + row) * MAX_COLOR;
					for(i=0; i<MAX_COLOR; i++){				// если цвет уже доступен
						if((int)cptr[i] == variants[cells]) break;
						else if(!cptr[i]){
							cptr[i] = variants[cells];
							break;
						}
					}
				}
		}
	}

	// обрабатываем строки
	byte *acc_clr = malloc(c_width * MAX_COLOR + 1);
	for(int line = 0; line < c_height; line++){				// счетчик строк
		c = c_left;
		memset(acc_clr, 0, c_width * MAX_COLOR);
		while (c)	if(l_d[(c-1) + line * c_left]) break;	// копируем оцифровку в буфер block
					else c--;
		bc = c;												// заполняем переменную количества блоков
		for (i=0; i < bc; i++){								// копируем текущую оцифровку в буфер
			--c;
			block[i] = (int)l_d[c + line * c_left];
			cblock[i] = l_clr[c + line * c_left] & COL_MARK_MASK;
		}
		st_pos = c_st_pos + fild_offset + line * c_left;
		en_pos = c_en_pos + fild_offset + line * c_left;
		// Подсчитываем количество свободного места в строке
		int a_free = block[0];
		for(i=1; i < bc; i++){
			a_free += block[i];
			if(cblock[i] == cblock[i-1]) a_free++;
		}
		a_free = c_width - a_free;

		// определяем стартовые и конечные позиции строк
		ptr = 0;
		for(elem = 0; elem < bc; elem++){
			if(((bool)elem) && cblock[elem] == cblock[elem-1]) ptr++;
			st_pos[elem] = ptr;
			ptr += block[elem];
			if(ptr > c_width){
				sprintf(last_error, _("C_make_access_color: Error make c_st_pos, line %d overflow"), line);
				return false;
			}
		}
		ptr = c_width;
		for(elem = bc-1; elem >=0; elem--){
			if((elem < (bc - 1)) && cblock[elem] == cblock[elem+1]) ptr--;
			ptr -= block[elem];
			if(ptr >= 0) en_pos[elem] = ptr;
			else{
				sprintf(last_error, _("C_make_access_color: Error make c_en_pos, line %d overflow"), line);
				return false;
			}
		}

		// Прогоняем подстановку от начала до конца строки
		for(int var = 0; var <= a_free; var++){
			memset(variants, 0, sizeof(int) * c_width);
			ptr = var;
			for(elem = 0; elem < bc; elem++){		// создаем вариант в буфере
				if(((bool)elem) && cblock[elem] == cblock[elem-1]) ptr++;
				for(int elemlen=0; elemlen < block[elem]; elemlen++)
					variants[ptr++] = (int)cblock[elem];
			}
			for(int cells=0; cells<c_width; cells++)// опр. цвета ячеек
				if(variants[cells]){				// указатель на строку доступных цветов
					byte *cptr = acc_clr + cells * MAX_COLOR;
					for(i=0; i<MAX_COLOR; i++){		// если цвет уже доступен
						if((int)cptr[i] == variants[cells]) break;
						else if(!cptr[i]) {cptr[i] = (byte)variants[cells]; break;}
					}
				}
		}

		// исключаем недоступные при "скрещивании" цвета
		for(int cells=0; cells<c_width; cells++){	//
			byte *org = access_color + (line * c_width + cells) * MAX_COLOR;
			byte *test = acc_clr + cells * MAX_COLOR;
			int ocount = -1; while(org[++ocount] && ocount < MAX_COLOR);
			int tcount = -1; while(test[++tcount] && ocount < MAX_COLOR);
			for(i=0; i<ocount; i++){
				bool r=false;
				for(c=0; c<tcount; c++)
					if(org[i]==test[c]) {r=true; break;}
				if(!r) org[i] = 0;
			}
			test = org;
			for(i=0; i<MAX_COLOR; i++) // уплотняем цвета
				if(*test) *(org++)=*(test++);
				else test++;
			while(org<test) *(org++) = 0;
		}
	}
	m_free(acc_clr);

	// корректирую старт-стоп позиции согласно доступных цветов
	for(int row = 0; row < c_width; row++){				// обрабатываем столбцы
		c = c_top;
		while (c)	if(t_d[(c-1) + row * c_top]) break; // копируем оцифровку в буфер block
					else c--;
		bc = c;											// заполняем переменную количества блоков
		for (i=0; i<bc; i++){							// копируем текущую оцифровку в буфер
			block[i]	= t_d[(--c) + row * c_top];
			cblock[i]	= t_clr[c + row * c_top] & COL_MARK_MASK;
		}
		st_pos = c_st_pos + row * c_top;
		en_pos = c_en_pos + row * c_top;
		for(elem = 0; elem < bc; elem++){
			for(byte a=st_pos[elem]; a<=en_pos[elem]; a++){
				byte *clr = access_color + (a * c_width + row) * MAX_COLOR;
				bool r = false;
				for(c = 0; c < block[elem]; c++){
					for(i=0; i<MAX_COLOR; i++){
						if(clr[i] == cblock[elem]){
							r=true;
							break;
						}
					}
				if(!r) break;
				}
				if(r){
					st_pos[elem] = a;
					break;
				}
			}
			for(byte a=en_pos[elem]; a>=st_pos[elem]; a--){
				byte *clr = access_color + (a * c_width + row) * MAX_COLOR;
				bool r = false;
				for(c = 0; c < block[elem]; c++){
					for(i=0; i<MAX_COLOR; i++){
						if(clr[i] == cblock[elem]){
							r=true;
							break;
						}
					}
				if(!r) break;
				}
				if(r){
					en_pos[elem] = a;
					break;
				}
			}
		}
	}
	for(int line = 0; line < c_height; line++){				// счетчик строк
		c = c_left;
		while (c)	if(l_d[(c-1) + line * c_left]) break;	// копируем оцифровку в буфер block
					else c--;
		bc = c;												// заполняем переменную количества блоков
		for (i=0; i < bc; i++){								// копируем текущую оцифровку в буфер
			--c;
			block[i] = (int)l_d[c + line * c_left];
			cblock[i] = l_clr[c + line * c_left] & COL_MARK_MASK;
		}
		st_pos = c_st_pos + fild_offset + line * c_left;
		en_pos = c_en_pos + fild_offset + line * c_left;
		for(elem = 0; elem < bc; elem++){
			for(byte a=st_pos[elem]; a<=en_pos[elem]; a++){
				byte *clr = access_color + (a + c_width * line) * MAX_COLOR;
				bool r = false;
				for(c = 0; c < block[elem]; c++){
					for(i=0; i<MAX_COLOR; i++){
						if(clr[i] == cblock[elem]){
							r=true;
							break;
						}
					}
				if(!r) break;
				}
				if(r){
					st_pos[elem] = a;
					break;
				}
			}
			for(byte a=en_pos[elem]; a>=st_pos[elem]; a--){
				byte *clr = access_color + (a + c_width * line) * MAX_COLOR;
				bool r = false;
				for(c = 0; c < block[elem]; c++){
					for(i=0; i<MAX_COLOR; i++){
						if(clr[i] == cblock[elem]){
							r=true;
							break;
						}
					}
				if(!r) break;
				}
				if(r){
					en_pos[elem] = a;
					break;
				}
			}
		}
	}
	return true;
}

bool c_check_digits_summ_color(){
	u_digits_change_event(true, false);
	int *colors_counts = MALLOC_BYTE(MAX_COLOR * sizeof(int) + 1);// счетчики цветов
	int x, y, c;
	for(x=0; x < c_top * c_width; x++)
		if(t_d[x]) colors_counts[t_clr[x] & COL_MARK_MASK] += t_d[x];
	for(c=1; c < MAX_COLOR; c++)
		if(colors_counts[c]) break;
	if(c == MAX_COLOR) return atom->digits_status = false;
	for(y=0; y < c_left * c_height; y++)
		if(l_d[y]) colors_counts[l_clr[y] & COL_MARK_MASK] -= l_d[y];
	for(c=1; c < MAX_COLOR; c++)
		if(colors_counts[c]) break;
	if(c < MAX_COLOR){										// проверяю эквивалентность оцифровок
		sprintf(last_error, "%s%s %s %s %d\n", _("Horizontal and vertical numbers sums nonequivalent!\n"),
						_("Summ digit color:"),  _(color_name[c]), _("different on:"), colors_counts[c]);
//		d_message(_("Puzzle digits error!"), last_error, false);
		m_free(colors_counts);
		return atom->digits_status = false;
	}
	m_free(colors_counts);
	return atom->digits_status = true;
}

bool c_check_complete_color(byte *map){
	if(!c_solve_buffers) return false;
	int *colors_counts = MALLOC_BYTE(MAX_COLOR * sizeof(int) + 1);// счетчики цветов
	int x, m, c;
	for(x=0; x < c_top * c_width; x++)
		if(t_d[x]) colors_counts[t_clr[x] & COL_MARK_MASK] += t_d[x];
	for(m=0; m < map_size; m++)
		if(map[m] > EMPTY && map[m] < MAX_COLOR)
			colors_counts[map[m]]--;
	for(c=0; c < MAX_COLOR; c++)
		if(colors_counts[c]) break;

	if(c < MAX_COLOR) return false;

/*	// проверка на правильность заполнения столбцов-строк
	c_test_line_res r;
	for(int x=0; x<c_width; x++){
		c_test_line(c_map, x, true, &r);
		if(!r.complete){
			return false;
		}
	}
	for(int y=0; y<c_height; y++){
		c_test_line(c_map, y, false, &r);
		if(!r.complete){
			return false;
		}
	}*/
	return true;
}

bool c_make_overlapping_color(byte *map, bool source_update){						// Быстрое заполнение "перекрытых" ячеек
	c_make_pos_and_access_color();
	int end, ptr, x, y;
	size_t indx, addr;
	bool res = false;

	for (x=0; x < c_width; x++)							// счетчик столбцов
		for(y=0; y < t_c[x]; y++){						// счетчик блоков в строчке
			indx = x * c_top + y;						// индекс в началых-конечных позициях
			addr = x * c_top + t_c[x] - y - 1;			// индекс в оцифровке
				// если    старт + длина > конец
				// старт			длина		конечная позиция
			if(c_st_pos[indx] + t_d[addr] > c_en_pos[indx])
				// то заполняем ячейки: конец - (старт+длина)
				for(end=c_en_pos[indx]; end < (c_st_pos[indx] + t_d[addr]); end++){
					ptr = end * c_width + x;
					if(map[ptr] != (t_clr[addr] & COL_MARK_MASK)){
						map[ptr] = t_clr[addr] & COL_MARK_MASK;
						if(source_update) c_reg_solved_cell(x, end);
						res = true;
					}
				}
		}
	for (y=0; y < c_height; y++) // счетчик строк
		for(x=0; x < l_c[y]; x++){ // счетчик цифр в строчке
			// если    старт + длина > конец
			indx = fild_offset + y * c_left + x;		// индекс позиций
			addr = y * c_left + l_c[y] - x - 1;			// индекс оцифровки
			// если    старт + длина > конец
			if(c_st_pos[indx] + l_d[addr] > c_en_pos[indx])
				// то заполняем ячейки: конец - (старт+длина)
				for(end=c_en_pos[indx]; end < (c_st_pos[indx] + l_d[addr]); end++){
					ptr = end + c_width * y;
					if(map[ptr] != (l_clr[addr] & COL_MARK_MASK)){
						map[ptr] = l_clr[addr] & COL_MARK_MASK;
						if(source_update) c_reg_solved_cell(end, y);
						res = true;
					}
				}
		}
	if(!res) strcpy(last_error, _("C_make_overlapping_color: make nothing!"));
	return res;
}

bool c_scan_line_color(byte *map, int number, bool that, bool source_update){
	int bc;	// определитель колич. цифр оцифровки в строке оцифровки
	int volatile i, pos, cells, x;
	bool compl;
	int x_size = c_width, y_size = c_height;
	int line_len = that ? y_size : x_size;		// длина строки

	if(that)
		for(i = 0; i<line_len; i++)				// копируем строку в буфер
			orig[i] = map[i * x_size + number];
		else for(i = 0; i<line_len; i++)
			orig[i] = map[i + x_size * number];

	c_copy_dig_buff(number, that);				// копируем текущую оцифровку и цвета в буфер
	bc = that ? t_c[number] : l_c[number];
	// orig 	- копия строки
	// line_len	- длина строки
	// block	- копия оцифровки строки
	// cblock   - цвета оцифровки
	// bc		- колич. цифр оцифровки

	byte *st_pos = c_st_pos, *en_pos = c_en_pos;
	pos = that ? number * c_top : number * c_left + fild_offset;
	st_pos += pos;	// определяю смещение на данные выбранной строки(столбца) в буферах позиций
	en_pos += pos;

// определяем начальные стартовые позиции
	int strt_curr = 0;
	compl=true;
	for(i=0; i < bc; i++){						// счетчик элементов
		int a;									// проверяем от стартовой до стоповой позиции
		byte b, c;
		calcerror = true;
		for(pos = strt_curr; pos < line_len; pos++){
												// i   = номер элемента
            									// pos = указ. на начало
												// a   = указ. начало + длина
												// b   = значение первой ячейки за элементом
			a = pos + block[i];					// c   = значение первой ячейки перед элементом
			if(a > line_len || pos < 0) {calcerror = true; break;}
			b = (byte)((a < line_len) ? orig[a] : EMPTY);
			c = (byte)((pos > 0) ? orig[pos-1] : EMPTY);
			int d = 0;
			if((b != cblock[i]) && (c != cblock[i])){
				for(cells = pos; cells < a; cells++){ // от старта до конца
													// вынуждаем цикл окончится...
					if(!c_test_cell_color_in_buffer(cells, i, that, number)) cells = line_len + 1;
					else if(orig[cells] == cblock[i]) d++;
				}
			}
            else cells = line_len + 1;

            if(cells <= line_len){				// если вариант подходящий то обновляем
				if(d == block[i] && compl)		// стартовые значения в массиве
					st_pos[i] = en_pos[i] = pos;// блок определён весь
				else compl = false;
				strt_curr = pos + block[i];		// наращивание текущей позиции
				if(i < bc-1 && cblock[i] == cblock[i + 1]) strt_curr++;

				for(x = i; x < bc; x++){		// счетчик оставшихся элем.
					if(st_pos[x] < pos) st_pos[x] = pos;
					else pos = st_pos[x];
					if(x < bc-1){
						pos += (int)block[x];
						if(cblock[x] == cblock[x + 1]) pos++;
					}
				}
				pos = line_len + 1;				// позиция подходящая
				calcerror = false;
				break;							// вынуждаем цикл закончится
			}
		}
		if(calcerror) {
			sprintf(last_error, _("c_scan_line_color: st_pos. not defined unit place:%d в %s: %d\n"), i+1,
				that?_("column"):_("line"), number+1);
			c_error;
			return false;
		}
	}

// определяем начальные стоповые позиции
	strt_curr = line_len - block[bc-1];
	compl = true;
	for(i = bc-1; i >= 0; i--){		// обратный счетчик элементов
		calcerror = true;// проверяем от стоповой до стартовой позиции
		for(pos=strt_curr; pos>=0; pos--){
										// i	 = номер элемента
										// pos = указ. на начало
										// c   = значение первой ячейки перед элементом
										// b   = значение первой ячейки за элементом
			int a = pos + (int)block[i];// a   = указ. начало + длина
			if(a>line_len || pos<0) {calcerror = true; break;}
			byte b = (byte)((a < line_len) ? orig[a] : EMPTY);
			byte c = (byte)((pos > 0) ? orig[pos-1] : EMPTY);
			int d = 0;
//			if((b == MARK || b == EMPTY || b != cblock[i]) && (c == MARK || c == EMPTY || c != cblock[i])){
			if((b != cblock[i]) && (c != cblock[i])){
				for(cells = pos; cells < a; cells++) // от старта до конца
										// вынуждаем цыкл окончится, если внутри маркер
//					if(orig[cells] == MARK) cells = line_len+1;
//					else if(orig[cells] != EMPTY &&
//							orig[cells] != cblock[i]) cells = line_len + 1;
					if(!c_test_cell_color_in_buffer(cells, i, that, number)) cells = line_len + 1;
					else if(orig[cells] == cblock[i]) d++;
			}
			else cells = line_len + 1;

			if(cells <= line_len){
				if(d == block[i] && compl)
					st_pos[i] = en_pos[i] = pos;
				else compl = false;

				if(i > 0){
					strt_curr = pos - block[i-1];
					if(cblock[i] == cblock[i - 1]) strt_curr--;
				}
				for(x=i; x >= 0; x--){	// счетчик оставшихся элем.
					if(en_pos[x] > pos) en_pos[x] = pos;
					else pos = en_pos[x];
					if(x > 0){
						pos -= (int)block[x - 1];
						if(cblock[x] == cblock[x - 1]) pos--;
					}
				}
				pos = line_len + 1;
				calcerror = false;
				break;	// вынуждаем цыкл окончится
			}
		}
		if(calcerror) {
			sprintf(last_error, _("c_scan_line_color: en_pos. not defined unit place:%d в %s: %d\n"), i+1,
				that?_("column"):_("line"), number+1);
			return false;
		}
	}

// небольшое добавление ограничивающее диапазон действия элемента от конца
// диапазона предыдущего - первой непустой ячейкой
	for(i=0; i < bc; i++){
		int a = i > 0 ? en_pos[i-1] + block[i-1] : 0;	// a - граница диапазона пред. элемента
		for(pos = a; pos < line_len; pos++)
			if(orig[pos] == cblock[i])
				if(en_pos[i] > pos)
					{ en_pos[i] = pos; break; }
	}

	for(i=bc-1; i>=0; i--){
		int a = line_len-1;								// a - граница диапазона пред. элемента
		if(i < bc-1){
			a=st_pos[i+1];
			if(cblock[i] == cblock[i+1]) a--;
		}
		for(pos=a; pos > 0; pos--)
			if(orig[pos] == cblock[i]) {
				int f=0;
				if((i < bc-1) && (cblock[i] == cblock[i+1])) f++;
				if(st_pos[i] + block[i]-f < pos)
					{ st_pos[i] = pos - block[i] + f; break; }
				}
	}

// Следующий этап - сканирование всех ячеек строки. При нахождении заполненной,
// определяется ее длина и происходит поиск элементов, которые могут ее перекрыть
// (по длине и местоположению), если оных всего один, то обновляются стартовые
// и стоповые индексы этого элемента, а затем и всех остальных.
// Если же длина совпадает со всеми перекрывающими элементами, то маркируем ее
// границы.
// P.S. Параллельно вышеописаному данный блок при нахождении подходящего элемента
// методом подстановки ищет непротиворечащее оригиналу положение этого элемента,
// с включением этой ячейки (или ячеек).
// При нахождении он увеличивает счетчики соответствующие каждой заполняемой ячейки
// и увеличиват общий счетчик вариантов. В итоге если к концу проверки величина
// какого-либо из счетчиков ячеек совпадает с общим счетчиком значит однозначно
// заливаем эту ячейку.

	int n_elem, elem, elem_len=0;
	byte color;
	bool repeat;//, skips = false;
	do	{
		repeat = false;								// поиск первого блока в матрице
		for(i=0; i<line_len; i++)     				// счетчик ячеек
			if(orig[i] != EMPTY && orig[i] != MARK){	// если ячейка окрашена
				color = orig[i];						// запоминаю цвет ячейки
				n_elem = 0;
				x = i;                  				// определяем длину
				while(x < line_len && orig[++x] == color);// блок найден
				x -= i;									// х = его длина
	// здесь и далее отмечено то, что описано в P.S.
				int counter=0;							// обнуляем общий счетчик
				for(cells=0; cells<line_len; cells++) variants[cells] = 0;// обнуляем счетчики ячеек
				for(cells=0; cells<bc; cells++){		// счетчик элементов
				// если элем. нужного цвета, длинее, старт. раньше и кончается позже то он подходит
					if(	(cblock[cells] == color) && (block[cells] >= x) &&
						(st_pos[cells] <= i) && (en_pos[cells]+block[cells] >= i+x)){
						n_elem++;						// увеличиваем счетчик подходящих элементов
						elem = cells;					// сохраняем номер элемента
						// если блок не первый и его длина не равна сохраненной
						if(n_elem > 1 && elem_len != block[cells]) elem_len = line_len + 1;
						else elem_len = block[cells];

						// i - адрес ячейки; x - длина найденого блока
						// d - конечный адрес диапазона, е  - начальный
						int d = MIN(MIN(i, line_len - block[cells]), en_pos[cells]);
						int e = MAX(MAX(i + x - block[cells], 0), st_pos[cells]);
						for(int a = e; a <= d; a++){	// проверяем положение
							bool r = false;

							if((a > 0) && (orig[a-1] == color))		r = true;
							if((a + block[cells] < line_len) &&
								(orig[a + block[cells]] == color))	r = true;
							if(r) continue; 			// прокрутка цикла если сбоку закрашеная ячейка

							for(int b=a; b < a + block[cells]; b++)
								if(!c_test_cell_color_in_buffer(b, cells, that, number)) {r = true; break;}
							if(r) continue; 			// прокрутка цикла если внутри попался маркер
														// Ok - увеличиваем счетчики
							counter++;
							for(int b=a; b<a+block[cells]; b++)
								variants[b]++;
						}
					}
				}
				// сравниваем счетчики
				if(!counter) {
					calcerror = true;
					sprintf(last_error, "\nCalcError: 3. %s: %d\n",
									that ? _("column") : _("line"), number);
					return false;
				}
				for(cells=0; cells<line_len; cells++)
					if(variants[cells] == counter){
						orig[cells] = color;
					}
				if(elem_len == x){       				// если длина совпадает со всеми доступными элементами
					bool res = true;
					for(cells=0; cells < bc; cells++)
						if(cblock[cells] != color) res = false;
					if(res){
						if(i>0){
							orig[i-1] = MARK;			// то маркируем границы
						}
						if((i+x)<line_len){
							orig[i+x] = MARK;
						}
					}
				}
				if(n_elem == 1){			// если элемент единственный
										// обновляем стартовые и стоповые позиции
					if(st_pos[elem] < (i + x - block[elem])){
						st_pos[elem] = (i + x - block[elem]);
						repeat = true;
					}
					if(en_pos[elem] > i){
						en_pos[elem] = i;
						repeat = true;
					}
				// обновляем стартовые и стоповые позиции остальных элементов
					for(cells = elem-1; cells >= 0; cells--)
						if(en_pos[cells] + block[cells] +
							(cblock[cells] == cblock[cells + 1] ? 1 : 0) > en_pos[cells+1]){
							en_pos[cells] = en_pos[cells + 1] - block[cells] -
								(cblock[cells] == cblock[cells + 1] ? 1 : 0);
							repeat = true;
						}
					for(cells = elem+1; cells < bc; cells++)
						if(st_pos[cells] < (st_pos[cells - 1] + block[cells - 1] +
								(cblock[cells] == cblock[cells-1] ? 1 : 0))){
							st_pos[cells] = st_pos[cells - 1] + block[cells - 1] +
								(cblock[cells] == cblock[cells-1] ? 1 : 0);
							repeat = true;
						}
				}
				else if(!n_elem) {
					calcerror = true;
					sprintf(last_error, "\nCalcError: 4. %s: %d\n", that ? _("column") : _("line"), number);
					return false;
				}
				i += x-1;
			}
	} while(repeat);

// Данный блок проверяет количество одинаковых по длине блоков и сканирует
// строку в поиске однозначно определеных блоков данной длины. Если количество
// блоков в оцифровке соответствует количеству в строке то присваиваются и
// отождествляются их стартовые и конечные индексы
// пы.сы. по тестам ненашел практической пользы от этого куска кода
#ifdef ADVANCE
	int last_elem_len = 0, last_color = 0, a, d;
	for(x=0; x < bc; x++){
		n_elem = 0;

		if(last_elem_len == block[x] || last_color == cblock[x]) continue;
		else{ last_elem_len = block[x]; last_color = cblock[x];	}

		for(i=0; i<bc; i++)											// подсчитываем количество одинаковых блоков
			if(block[i] == block[x] && cblock[i] == cblock[x])		// в оцифровке
				n_elem++;

		for(i=0; i<line_len; i++)									// сканируем строку в поиска заполненых ячеек
			if(orig[i] != EMPTY && orig[i] != MARK){				// если ячейка окрашена
				color = orig[i];
				if(color != cblock[x]) continue;					// next если есть дифференциация цвета
				d = i;												// определяем длину
				while(d<line_len && orig[++d] == color);
				byte c = i > 0 ? orig[i-1] 		: MARK;				// c = значение ячейки перед элем.
				byte b = d < line_len ? orig[d]	: MARK;				// b = значение ячейки за элем.
				d -= i;												// a = длина
				if(d == block[x] &&	c != EMPTY && b != EMPTY)		// проверяем на совпадение длины и привязки к месту
						n_elem --;
				i += d - 1;
			}
		if(!n_elem){												// Если количество совпадает
			for(a = 0; a <bc; a++){									// a = позиция в оцифровке
				if (block[a] != block[x] || cblock[a] != cblock[x]) continue;
				for(pos = st_pos[a]; pos <= en_pos[a]; pos++)		// pos = позиция в строке
					if(orig[pos] != EMPTY && orig[pos] != MARK){	// если ячейка окрашена
						color = orig[pos];							// беру цвет ячейки
						if(color != cblock[x]) continue;
						d = pos;									// определяю длину блока
						byte c = pos>0 ? orig[pos-1] : MARK;		// c = значение ячейки перед элем.
						while(d<line_len && orig[++d] == color);	// ищю конец блока
						byte b = d<line_len ? orig[d] : MARK;		// b = значение ячейки за элем.
						d -= pos;									// d = длина блока в мартице
						if(d == block[x] && b != EMPTY && c != EMPTY)
							st_pos[a] = en_pos[a] = pos;
					}
			}
		}
	}
#endif //ADVANCE

// Теперь производится поиск элементов прижатых к маркеровке, но
// неизвесной длины и если она меньше наименьшего значения оцифровки, то элемент
// удлиняется.

	for(i=0; i<line_len; i++)								// сканируем строку в поиска заполненых ячеек
		if(orig[i] != EMPTY && orig[i] != MARK){			// если ячейка окрашена
			byte color = orig[i];
			int min_elem_len = line_len + 1;
			for(cells=0; cells<bc; cells++)						// определяем длину минимального элемента
				if(cblock[cells] == color && min_elem_len > block[cells])
					min_elem_len = block[cells];
			if(min_elem_len > line_len) break;

			int a = i;                 	// определяем длину
			byte c = (byte)(i>0 ? orig[i-1] : MARK);			// c = значение ячейки перед элем.
			while(a<line_len && orig[++a] == color);
			byte b = (byte)(a<line_len ? orig[a] : MARK);	// b = значение ячейки за элем.
			a -= i;                 						// a = длина
			if(((b != EMPTY && c == EMPTY) || (c != EMPTY && b == EMPTY)) && (a < min_elem_len)){
				if(b != EMPTY) x = i + a - min_elem_len;
				else           x = i;
				for(pos = 0; pos < min_elem_len; pos++)
					if(pos + x < line_len){
						orig[pos+x] = color;
					}
					else{
#ifdef calcerrors
fprintf(stderr, "\nCalcError: 5. %s: %d\n", that ? _("column"):_("line"), number);
#endif
						return false;
					}
			}
		i += a - 1;
		}

// Теперь обновляем старт - стоп позиции методом подстановки
	strt_curr = st_pos[0];
// определяем начальные стартовые позиции
	for(i=0; i<bc; i++){					// счетчик элементов
		calcerror = true;					// проверяем от стартовой до стоповой позиции
		for(pos=strt_curr; pos<line_len; pos++){
											// i   = номер элемента
											// pos = указ. на начало
											// a   = указ. начало + длина
											// b   = значение первой ячейки за элементом
			int a = pos+block[i];			// c   = значение первой ячейки перед элементом
			byte b = (byte)((a < line_len) ? orig[a] : MARK);
			byte c = (byte)((pos > 0) ? orig[pos-1] : MARK);
			if((b != cblock[i]) && (c != cblock[i])){
				for(cells = pos; cells < a; cells++) // от старта до конца
											// вынуждаем цыкл окончится, если внутри маркер
					if(!c_test_cell_color_in_buffer(cells, i, that, number)) cells = line_len + 1;
               }
			else cells = line_len+1;

			if(cells <= line_len){						// если вариант подходящий то обновляем
				strt_curr = pos + block[i];				// стартовые значения в массиве
				if(i < bc-1 && cblock[i] == cblock[i+1]) strt_curr++;
				for(x=i; x < bc; x++){					// счетчик оставшихся элем.
					if(st_pos[x] < pos) st_pos[x] = pos;
					else pos = st_pos[x];
					if(i < bc-1){
						pos += (int)block[x];
						if(cblock[x] == cblock[x+1]) pos++;
					}
				}
				pos = line_len+1;
				calcerror = false;
				break;									// вынуждаем цыкл окончится
			}
		}
		if(calcerror) {
#ifdef calcerrors
fprintf(stderr, "\nCalcError: 6. %s: %d\n", that ? _("column"):_("line"), number);
#endif
			return false;
		}
	}

// определяем начальные стоповые позиции
	strt_curr = en_pos[bc-1];
	for(i = bc-1; i >= 0; i--){		// обратный счетчик элементов
		calcerror = true;// проверяем от стоповой до стартовой позиции
		for(pos=strt_curr; pos>=0; pos--){	// i	 = номер элемента
											// pos = указ. на начало
											// c   = значение первой ячейки перед элементом
											// b   = значение первой ячейки за элементом
			int a = pos+(int)block[i];		// a   = указ. начало + длина
			byte b = (byte)((a < line_len) ? orig[a] : EMPTY);
			byte c = (byte)((pos > 0) ? orig[pos-1] : EMPTY);
			if((b != cblock[i]) && (c != cblock[i])){
				for(cells = pos; cells < a; cells++) // от старта до конца
											// вынуждаем цыкл окончится, если внутри маркер
					if(!c_test_cell_color_in_buffer(cells, i, that, number)) cells = line_len + 1;
//					if(orig[cells] == MARK) cells = line_len+1;
			}
			else cells = line_len + 1;

			if(cells <= line_len){					// если вариант подходящий то обновляем
				if(i>0){
					strt_curr = pos - block[i-1];
					if(cblock[i] == cblock[i-1]) strt_curr--;
				}
				for(x=i; x >= 0; x--){			// счетчик оставшихся элем.
					if(en_pos[x] > pos) en_pos[x] = pos;
					else pos = en_pos[x];
					if(x>0){
						pos -= (int)block[x-1];
						if(cblock[x] == cblock[x-1]) pos--;
					}
				}
				pos = line_len + 1;
				calcerror = false;
				break;								// вынуждаем цыкл окончится
			}
		}
		if(calcerror) {
#ifdef calcerrors
	fprintf(stderr, "/rCalcError: 7\n");
#endif
			return false;
		}
	}

// Теперь для каждого элемента отмечаются ячейки, которые могут быть заполнены
// только этим элементом. Проверяем эти ячейки на заполнение, при этом первая-же
// заполненая ячейка сравнивается со стоповой позицией и если она меньше то
// обновляется местоположением этой ячейки. Аналогично для последней заполненной
// ячейки диапазона действия этого элемента.

	int st_priv, en_priv;								// индексы начала и конца "частной зоны"
	for(i=0; i<bc; i++){								// счетчик элементов
														// st_priv = конечная позиция предыдущ. элем.
		int f=0;
		if(i < bc-1 && cblock[i] == cblock[i+1]) f++;
		st_priv = ((i>0) ? (en_pos[i-1] + block[i-1]) : st_pos[0]);
		en_priv = ((i < (bc-1)) ? st_pos[i+1] - f - 1: (en_pos[i] + block[i] - 1));
														// en_priv   = стартовая позиция след. элем.
		for(x = st_priv; x <= en_priv; x++){				// Проверяем ячейки на заполнение
			if(orig[x] == cblock[i]){
				if(en_pos[i] > x) en_pos[i] = x;
				x = line_len+1;
				break;
			}
		}
		for(x=en_priv; x>=st_priv; x--){// Проверяем ячейки на заполнение
			if(orig[x] == cblock[i]){
				if(st_pos[i] < (x-block[i])) st_pos[i] = x-block[i];
				x = line_len+1;
				break;
			}
		}
	}

// Теперь обобщим полученные данные прогоном от стартовой до стоповой позиции и
// если подходящий вариант один однозначно определим старт и стоп индексы
	for(i=0; i<bc; i++){
		elem = elem_len = 0;
		for(pos = st_pos[i]; pos <=en_pos[i]; pos++){
											// i	 = номер элемента
											// pos = указ. на начало
											// a   = указ. начало + длина
											// b   = значение первой ячейки за элементом
			int a = pos+block[i];  			// c   = значение первой ячейки перед элементом
			byte b = (byte)((a < line_len) ? orig[a] : EMPTY);
			byte c = (byte)((pos > 0) ? orig[pos-1] : EMPTY);
			if((b != cblock[i]) && (c != cblock[i])){
				for(cells = pos; cells < a; cells++)// от старта до конца
					if(!c_test_cell_color_in_buffer(cells, i, that, number))
						cells = line_len+1;	// вынуждаем цыкл окончится, если внутри инородный цвет
			}
			else cells = line_len+1;

			if(cells <= line_len){
				elem = pos;
				elem_len++;
			}
		}
		if(elem_len == 1) st_pos[i] = en_pos[i] = elem;
		else if(!elem_len) {
#ifdef calcerrors
	fprintf(stderr, "\nCalcError: not place in %s: %d for block: %d lenght: %d color: %s\t", that ? _("column"):_("line"), number, i, block[i], color_name[cblock[i]]);
#endif
			calcerror=true;
			return false;
		}
	}

// Далее отмечаются все перекрытые ячейки, а затем создается пустая строка, заново
// прогоняются все варианты от стартовой до стоповой позиции для всех элементов
// и при правильном варианте маркируются соответствующие элементы этой строки.
// Те ячейки, которые останутся неотмечеными маркируются в оригинале как пустые.

	for(i=0; i<bc; i++)				// отмечаем перекрытые ячейки
		for(x=en_pos[i]; x<(st_pos[i]+block[i]); x++){
			orig[x] = cblock[i];
		}
									// обнуляем буфер (пустая строка)
	for(i=0; i<line_len; i++) c_buffer[i] = EMPTY;
	for(i=0; i < bc; i++)				// счетчик элементов
		for(pos=st_pos[i]; pos<=en_pos[i]; pos++){	// счетчик доступных позиций
			int a = pos+(int)block[i]; // a   = указ. начало + длина
			byte b = (byte)((a < line_len) ? orig[a] : MARK);
			byte c = (byte)((pos > 0) ? orig[pos-1] : MARK);
			if((b != cblock[i]) && (c != cblock[i])){
				for(cells = pos; cells < a; cells++) // от старта до конца
									// вынуждаем цыкл окончится, если внутри маркер
         			if(!c_test_cell_color_in_buffer(cells, i, that, number)) cells = line_len + 1;
			}
			else cells = line_len + 1;
			if(cells<=line_len)		// если позиция непротиворечащая - заливаем ячейки строки
				for(cells = pos; cells < a; cells++) c_buffer[cells] = cblock[i];
		}							// маркируем пустые ячейки в строке оригинала
	for(i=0; i<line_len; i++) if(c_buffer[i] == EMPTY){
		orig[i] = MARK;
//#ifdef DEBUG
//fprintf(stderr, "\r2825:Mark, %s %d, cell %d\n", that ? _("column"):_("line"), number+1, i+1);
//#endif
		}
	bool res = false;				// копируем полученную строку назад в матрицу
	pos = 0;
	for(i=0; i<line_len; i++){
		if(orig[i] != EMPTY) pos ++;
		x = that ? (i * c_width + number) : (i + c_width * number);
		if((map[x]) != orig[i]){
			if(that)	y_complete[i]=true;
			else		x_complete[i]=true;
			map[x] = orig[i];
			if(view_mark){
				if(source_update) c_add_solved_cell(i, number, that);
			}
			else if(source_update && map[x] != MARK)
				c_add_solved_cell(i, number, that);
			res = true;
		}
	}								// сбрасываем флажки для дальнейшего игнорирования
	if(!no_logic && (!res || pos == line_len)){
		if(that) x_complete[number] = false;	// отгаданых строк
		else     y_complete[number] = false;
	}
	return res;
}

bool c_test_line_color(byte *map,
						int numb,					//номер строки
						bool orient,				//true - колонки, false - строки
						c_test_line_res *result)	//указка на результат
{
	static c_test_line_res res;
	if(result == NULL) result = &res;
	result->legal = result->digits = result->matrix = result->complete = false;
	int var, numb_var, v1, i, pos, el, cell;
	byte clr;
	bool cur_place;
	byte *line = orig;
	int bc = orient ? t_c[numb] : l_c[numb];
	int line_len = orient ? c_height: c_width;
	byte *digits = orient ?	t_d + numb * c_top : l_d + numb * c_left;
	byte *markers= orient ?	atom->top.colors + numb * c_top:atom->left.colors+ numb * c_left;

	byte dig_len = orient ? atom->top_dig_size : atom->left_dig_size;
	byte *colors= MALLOC_BYTE(dig_len);
	byte *p = orient ? atom->top.colors+numb*c_top : atom->left.colors+numb*c_left;
	for(i=0; i<dig_len; i++) colors[i] = p[i] & COL_MARK_MASK;

	if(orient)
		for(int c=0; c < c_height; c++)				// копирование столбца в буфер
			orig[c_height - c - 1] = map[numb + c_width * c];
	else
		for(int c=0; c < c_width; c++)				// копирование строки в буфер
			orig[c_width - c - 1] = map[numb * c_width + c];

	if(bc == 0){									// Выход, если пустая строка
		for(i=0; i<line_len; i++)
			if(line[i]!=EMPTY && line[i]!=MARK) return false;
		if(help_level > 1){
			for(i=0; i<line_len; i++)
				if(line[i] != MARK){
					line[i] = MARK;
					result->matrix = true;
				}
		}
		free(colors);
		result->complete = true;
		return result->legal = true;
	}
// Часть 1 - расчет начальных-конечных позиций элементов и маркировка оцифровки
	// делаю копию старт-стоп позиций строки
	pos =  orient ? numb * c_top : numb * c_left + c_width * c_top;
	size_t 	len = (orient ? c_top : c_left) + 1,
			llen= (orient ? c_height : c_width);
	byte *start = MALLOC_BYTE(len),
		 *end	= MALLOC_BYTE(len),
		 *acc_clr = MALLOC_BYTE(llen*MAX_COLOR);
	memcpy(start, c_st_pos + pos, len);
	memcpy(end, c_en_pos + pos, len);
	for(i=0; i<llen; i++)							// копирую доступные цвета
		if(orient)	memcpy(acc_clr+(llen-i-1)*MAX_COLOR, access_color+(i*c_width+numb)*MAX_COLOR, MAX_COLOR);
		else		memcpy(acc_clr+(llen-i-1)*MAX_COLOR, access_color+(i+c_width*numb)*MAX_COLOR, MAX_COLOR);

	int space = line_len, count = 0;				// количество пустоты в строке
	for(i=0; i < bc; i++){							// Расчитываем начальные стартовые позиции
		space -= digits[i];
		start[i] = count;
		if(i<bc-1 && colors[i]==colors[i+1])
				{space--; count++;}
		count += digits[i];
	}
	for(i=0; i < bc; i++)							// начальные стоповые позиции
		end[i] = start[i] + space;

// прогоняем все элементы подстановкой и обн.старт.поз
	bool r; byte c;
	for(el=0; el<bc; el++){							// счетчик блоков
		clr = colors[el];
		for(int pos=start[el]; pos<=end[el]; pos++){// счетчик свободного пространства подстановки
			cur_place = true;
			if(pos > 0)								// проверка ячеек перед и за элементом
				if(line[pos-1]==clr){
					cur_place = false; continue; }
			var = pos  + digits[el];
			if(var > line_len){
				free(start); free(end); free(colors);
				return result->legal = false;		// выход если блок вылазит за границы матрицы
			}
			if(var < line_len)
				if(line[var]==clr){
					cur_place=false; continue;}
			for(v1=pos; v1 < var; v1++){		// счетчик тела элемента
				r=false;
				for(i=0; i<MAX_COLOR; i++)
					{c = acc_clr[v1*MAX_COLOR+i]; if(!c) break; if(c == clr){r=true; break;}}
				if(r && (line[v1] == EMPTY || line[v1] == clr)) continue;
				else {cur_place=false; break;}
			}
			if(cur_place){							// если место подходящее
				var = pos - start[el];				// обновляем стартовые позиции, на первом подх.месте
				for(i=el; i<bc; i++){
					start[i] += var;
					if(start[i] >= line_len){
						result->legal = false;
						goto end;
					}
				}
				break;
			}
		}
		if(!cur_place){
			result->legal = false;
			goto end;
		}	// выход если для блока не нашлось места
	}
// теперь то-же самое для стоповых позиций
	for(int el=bc-1; el>=0; el--){					// счетчик элементов
		clr = colors[el];
		for(int pos=end[el]; pos>=start[el]; pos--){// счетчик свободного пространства подстановки
			cur_place = true;
			if(pos > 0)								// проверка ячеек перед и за элементом
				if(line[pos-1] == clr){
					cur_place=false; continue;}
			var = pos  + digits[el];
			if(var > line_len){
				result->legal = false;		// брэк если блок вылазит за границы матрицы
				goto end;
			}
			if(var < line_len)
				if(line[var] == clr){
					cur_place=false; continue;}
			for(v1 = pos; v1 < var; v1++){	// счетчик тела элемента
				r=false;
				for(i=0; i<MAX_COLOR; i++)
					{c = acc_clr[v1*MAX_COLOR+i]; if(!c) break; if(c == clr){r=true; break;}}
				if(r && (line[v1] == EMPTY || line[v1] == clr)) continue;
				else {cur_place=false; break;}
			}
			if(cur_place){							// если место подходящее
				var = end[el] - pos;
				for(i=el; i>=0; i--){
					end[i]-=var;
					if(end[i] < 0){
						result->legal = false;
						goto end;
					}
				}
				break;
			}
		}
	if(!cur_place){
		result->legal = false;
		goto end;
		}	// выход если для блока не нашлось места
	}
// ограничиваю диапазон действия блока от конца
// диапазона предыдущего - первой непустой ячейкой
	for(el=0; el<bc; el++){
		clr = colors[el];
		int a = el>0 ? end[el-1]+digits[el-1] : 0;	// a - граница диапазона пред. элемента
		for(pos=a; pos<line_len; pos++)
			if(line[pos] == clr)
				if(end[el] > pos){
					end[el] = pos;
					if(end[el] < start[el]){
						result->legal = false;
						goto end;
					}
					break;
				}
	}

	for(el=bc-1; el>=0; el--){
		clr = colors[el];
		int a = el<bc-1 ? start[el+1]-1 : line_len-1;	// a - граница диапазона пред. элемента
		for(pos=a; pos > 0; pos--)
			if(line[pos] == clr)
				if(start[el]+digits[el]-1 < pos){
					start[el] = pos-digits[el]+1;
					if(end[el] < start[el]){
						result->legal = false;
						goto end;
					}
					break;
				}
	}

// **************** шаг 2 - проверка отмеченых от краев элементов **************
	int s=0, e=line_len-1;
	for(numb_var=0; numb_var<bc; numb_var++){		// с начала строки
		clr = colors[numb_var];
		while((line[s]==MARK) && (s<line_len)) s++;	// подгонка к первой ячейке,
		if((s <line_len)&&(line[s]==clr)){		// не отмеченной маркером
			cur_place=true;
			for(v1=s; v1<s+digits[numb_var]; v1++)
				if(line[v1]!=clr) cur_place=false;
			if(cur_place && (line[v1++]!=clr)){
				end[numb_var] = start[numb_var];
			}
			else break;
			s=v1;
		}
		else break;
	}
	for(numb_var=bc-1; numb_var>=0; numb_var--){	// с конца строки
		clr = colors[numb_var];
		while((line[e]==MARK) && (e>=0)) e--;
		if((e>=0) && (line[e]==clr)){
			cur_place=true;
			for(v1=e; v1>e-digits[numb_var]; v1--)
				if(line[v1]!=clr) cur_place=false;
			if(cur_place && (line[v1--]!=clr)){
				start[numb_var] = end[numb_var];
			}
			else break;
			e=v1;
		}
		else break;
	}
// ********** Шаг 3 поиск единственного самого длинного элемента ***************
	if(help_level > 0){
		int save;
		numb_var = var = 0;
		for(v1 = 0; v1 < bc; v1++)					// ищем самый длинный элемент
			if(digits[v1] > var) var = digits[v1];
		for(v1 = 0; v1 < bc; v1++)
			if(digits[v1] == var)					// проверяю его уникальность
				{numb_var++; save = v1; clr = colors[v1];}
		if(numb_var == 1){							// выполняю поиск его отметки в строке
			for(s = 0; s < line_len; s++)			// поиск первой занятой этим цветом ячейки
				if(line[s]==clr) break;
			if((s+var) <= line_len){				// если позиция + длина элем. не вылазит за пределы строки
				cur_place = true;					// var - длина блока
				for(e = s; e < (s+var); e++)		// s - индекс начала элемента в строке
					if(line[e]!=clr)
						{s = e; cur_place = false; break;}// брек если в теле блока что-то инородное
				if(e < line_len && line[e]==clr)	// что за блоком?
					cur_place=false;

				if(cur_place){
					if(s >= start[save] && s <= end[save]){		// вписывается-ли найденный блок в свои рамки
						start[save] = end[save];
	}	}	}	}	}
// ******* Шаг 4 поиск однозначно пустых и однозначно заполненных ячеек *********
	if(help_level > 3){
		for(el=0; el<bc; el++){
			clr = colors[el];
			if(digits[el] > end[el]-start[el]){	// если длина блока перекрывает его конечную позицию
				for(int cell = end[el]; cell < start[el] + digits[el]; cell++)
					if(line[cell] != clr){
						line[cell] = clr;
						result->matrix = true;
					}
			}
		}
	}
	if(help_level > 2){
		for(i=0; i<line_len; c_buffer[i++] = 0);
		for(el=0; el < bc; el++)
			for(cell=start[el]; cell < end[el] + digits[el]; cell++)
				c_buffer[cell]++;
		for(i=0; i<line_len; i++)
			if(c_buffer[i] == 0 && line[i] != MARK){
				line[i] = MARK;
				result->matrix = true;
			}
	}

// ***** Шаг 5 - проверка строки на правильную прорисовку всех элементов *******
	cur_place = true;
	var = 0;										// счетчик ячеек строки
	for(numb_var=0; numb_var<bc; numb_var++){		// счетчик элементов
		clr = colors[numb_var];
		while(var < line_len && line[var] != clr)	// поиск первой залитой ячейки
			var++;
		if(var >= line_len) break;
		v1 = var + digits[numb_var];
		while(var < v1)								// проверка верности прорисовки
			if(line[var++] != clr){
				cur_place = false;
				break;
			}
		if(!cur_place) break;						// прерывание если несоответсвие прорисовки элементу
		if(var < line_len){							// если не конец строки
			if(line[var++] == clr){					// поправка на межелементный разрыв
				cur_place = false;
				break;
			}
		}
	}
	if(cur_place && (numb_var == bc)){
		while(var < line_len)						// проверка остатка строки на отсутствие зарисовки
			if(line[var]!=EMPTY && line[var]!=MARK)
				{cur_place=false; break;}
			else var++;
		if(cur_place){
			result->complete = true;
			if(help_level > 0){
				for(v1=0; v1<bc; v1++){				// отмечаю всю оцифровку
					if(!(markers[v1] & COLOR_MARK)){
						markers[v1] |= COLOR_MARK;
						result->digits = true;
					}
				}
			}
			if(help_level > 1){
				for(v1=0; v1<line_len; v1++)		// дополняю отгаданную строку маркерами
					if(line[v1] == EMPTY){
						line[v1] = MARK;
						result->matrix = true;
					}
			}
		}
	}
	for(v1=0; v1<bc; v1++){								// корректируем изменения в источниках
		clr = colors[v1];
		cur_place = false;
		if(start[v1] == end[v1]){
			cur_place = true;
			for(var = start[v1]; var < (start[v1]+digits[v1]); var++){
				if(line[var] != clr) cur_place = false;
				if(help_level > 3){						// прорисовка блока
					line[i] = clr;
					result->matrix = true;
				}
			}
		}
		if(cur_place){									// маркировка цифры в оцифровке
			if(help_level > 0 && !(markers[v1] & COLOR_MARK)){
				markers[v1] |= COLOR_MARK; result->digits = true;}
			if(help_level > 2){
				if(start[v1] > 0 && line[start[v1]-1] != MARK &&
												!(v1 > 0 && colors[v1-1]!=clr)){
					line[start[v1]-1] = MARK;			// маркировка ячейки перед блоком
					result->matrix = true;
				}
				if(start[v1]+digits[v1] < line_len && line[start[v1]+digits[v1]] != MARK &&
												!(v1 < bc-1 && colors[v1+1]!=clr)){
					line[start[v1]+digits[v1]] = MARK;	// маркировка ячейки за блоком
					result->matrix = true;
				}
			}
		}												// снятие отметки с цифры в оцифровке
		else if(help_level > 0 && (markers[v1] & COLOR_MARK)){
			markers[v1] &= COL_MARK_MASK; result->digits = true;}
	}

	if(help_level > 1){
		if(orient){
			for(int c=0; c < c_height; c++)					// возврат столбца в матрицу
				map[numb + c_width * c] = orig[c_height - c - 1];
		}
		else{
			for(int c=0; c < c_width; c++)
				map[numb * c_width + c] = orig[c_width - c - 1];
		}
	}
	result->legal = true;
end:
	free(start); free(end); free(colors); free(acc_clr);
	return result->legal;
}

void **sds;
int recurce, deep_recurse;			// текущая вложенность рекурсии для find_decision_color

typedef struct STATE_DUMP_tag{
	byte*st_pos, *en_pos,
		*map, *colors,
		*sequence,									// каскад строк по количеству пустых ячеек
		*access_colors;
	int recurce_skip, line_ptr,
		line, pos, elem, ocount, err_col_coun, col_count;
	byte *x_complete, *y_complete;
} STATE_DUMP;

void *dump_state(){
	STATE_DUMP *sd = MALLOC_BYTE(sizeof(STATE_DUMP)+1);
	sds[++recurce] = sd;
	sd->st_pos = malloc(xy_size + 1);
	sd->en_pos = malloc(xy_size + 1);
	sd->map = malloc(map_size+1);
	sd->sequence = malloc(c_width + 1);
	sd->access_colors = malloc(map_size * MAX_COLOR + 1);
	sd->x_complete = malloc(c_width +1);
	sd->y_complete = malloc(c_height +1);
	memcpy(sd->st_pos, c_st_pos, xy_size);
	memcpy(sd->en_pos, c_en_pos, xy_size);
	memcpy(sd->map, shadow_map, map_size);
	memcpy(sd->access_colors, access_color, map_size * MAX_COLOR);
	memcpy(sd->x_complete, x_complete, c_width);
	memcpy(sd->y_complete, y_complete, c_height);
	return (void *)sd;
}

void restore_dump(){
	STATE_DUMP *sd = sds[recurce];
	memcpy(c_st_pos, sd->st_pos, xy_size);
	memcpy(c_en_pos, sd->en_pos, xy_size);
	memcpy(shadow_map, sd->map, map_size);
	memcpy(access_color, sd->access_colors, map_size * MAX_COLOR);
	memcpy(x_complete, sd->x_complete, c_width * sizeof(bool));
	memcpy(y_complete, sd->y_complete, c_height * sizeof(bool));
}

void free_dump(){
	STATE_DUMP *sd = sds[recurce];
	if(!sd){
		if(recurce) recurce--;
		return;
	}
	m_free(sd->st_pos);
	m_free(sd->en_pos);
	m_free(sd->map);
	m_free(sd->sequence);
	m_free(sd->access_colors);
	m_free(sd->x_complete);
	m_free(sd->y_complete);
	m_free(sd);
	sds[recurce--] = NULL;
}

void get_cascade_line_color(void *sts){					// создание каскада строк по количеству пустых ячеек
	STATE_DUMP *scs = (STATE_DUMP *)sts;
	byte *seq = scs->sequence;
	byte *ptr = malloc(sizeof(int) * c_height);
	byte i, x, y;
	memset(seq, 0xff, c_width);
	for(y=0; y < c_height; y++){
		byte counter=0;
		for(x = 0; x < c_width; x++)
			if(shadow_map[y * c_width + x] == EMPTY) counter++;
		ptr[y]= counter ? counter : 0xff;
	}
	for(i=0; i<c_height; i++){							// Построение последовательности строк
		byte tmp = 0xff;
		for(x=0; x<c_height; x++)
			if(ptr[x] < tmp){
				tmp = ptr[x];
				seq[i] = x;
			}
		ptr[seq[i]] = 0xff;
	}
	m_free(ptr);
}

char find_decision_color(){
	bool res;
	int x, y;
	STATE_DUMP *scs = dump_state();
fprintf(stderr, "Recurce: %d\n", recurce);
	scs->recurce_skip = 0;
	get_cascade_line_color(scs);
	for(scs->line_ptr=0; scs->sequence[scs->line_ptr] != 0xff; scs->line_ptr++){// Цикл строк
		scs->line = scs->sequence[scs->line_ptr];
		for(scs->elem=0; scs->elem<c_width; scs->elem++){		// Цикл ячеек
			scs->pos = scs->line * c_width + scs->elem;
			if(shadow_map[scs->pos] != EMPTY) continue;
			scs->colors = access_color + scs->pos * MAX_COLOR;	// scs->colors доступные ячейке цвета
			scs->ocount = -1; while(scs->colors[++scs->ocount]);// scs->ocount количество доступных ячейке цветов(от 0)
			scs->err_col_coun = 0;
fprintf(stderr, "Position: %d\n", scs->elem);
			for(scs->col_count=0; scs->col_count < scs->ocount; scs->col_count++){// счетчик цветов
fprintf(stderr, "Color: %s\t", color_name[scs->colors[scs->col_count]]);
				restore_dump();
				shadow_map[scs->pos] = scs->colors[scs->col_count];// заполняю в цикле ячейку всеми доступными её цветами
				x_complete[scs->elem] = true;					// объявляю строку и столбец нерешенными
				y_complete[scs->line] = true;
  				res = false;									// прогоняю строку и столбец через сканер
				res |= c_scan_line_color(shadow_map, scs->elem, true, false);
				if(calcerror) continue;
				res |= c_scan_line_color(shadow_map, scs->line, false, false);
				if(calcerror) continue;
				if(res)	do {									// если есть изменения то прогоняю через сканер всю матрицу
						res = false;
//						c_make_pos_and_access_color();
						c_recalc_access_color();				// обновляю старт-стоп и глубину цветов
						x = y = 0;
						bool px=false, py=false;
						do{
							if(x<c_width && x_complete[x]) res |= c_scan_line(shadow_map, x, true, false);
							if(calcerror){break;}
							if(y<c_height && y_complete[y]) res |= c_scan_line(shadow_map, y, false, false);
							if(calcerror){break;}
							x++; y++;
							if(x == c_width) {x = 0; px = true;}
							if(y == c_height){y = 0; py = true;}
fprintf(stderr, "X: %d\tY: %d\t", x, y);
						} while(!px || !py);
					} while (res);
				if(calcerror){									// если ошибка
					scs->err_col_coun++;						// увеличиваю счетчик ошибок
				}
				else if(c_check_complete_color(shadow_map)){			// если решено
fprintf(stderr, "Exit Recurce: %d\tSolved1!\n", recurce);
					free_dump();						// удаляю дамп
					return 0; 									// анализ окончен
				}
				else {											// если решениее не окончено
					int volatile acount=0, bcount=0;
					for(int i=0; i<map_size; i++)
						if(shadow_map[i] == EMPTY) acount++;	// acount - количество нерешенных ячеек
						else if(shadow_map[i] != scs->map[i]) bcount++;	// bcount - количество решенных посл. прогоном ячеек
					if(acount < deep_recurse){
						char res = find_decision_color();		// рекурсивный вызов этой-же процедуры
						if(res == 0){							// если решено
fprintf(stderr, "Exit Recurce: %d\tSolved2!\n", recurce);
							free_dump();
							return 0;
						}
						else if(res == -1){
mess("error solver, loop next color2\n");
							scs->err_col_coun++;
						}
					}
					else scs->recurce_skip++;					// если решение не найдено
				}
fprintf(stderr, "loop color: %s\n", color_name[scs->colors[scs->col_count]]);
			}													// next color
			if(scs->err_col_coun == scs->ocount){				// если не подошел ни один цвет
				scs->map[scs->pos] = MARK;						// маркирую ячейку
			}
fprintf(stderr, "loop next cell, pos Y: %d\n", scs->elem+1);
		}														// next cell
fprintf(stderr, "loop next line: %d\n", scs->line_ptr+1);
	}
	if(scs->recurce_skip){
fprintf(stderr, "End Recurce: %d\t Not solved\tfrom\n", recurce);
		free_dump();
		return 1;
	}
fprintf(stderr, "End Recurce: %d\t Solved error\tfrom\n", recurce);
	free_dump();
	return -1;
}

void c_no_logic_color(){
	set_status_bar(_("Begining search solved..."));
	no_logic = true;
	memcpy(shadow_map, c_map, map_size);				// копирую матрицу в теневой буфер
	recurce = -1;
	sds = MALLOC_BYTE(512*sizeof(void *));
	deep_recurse = 150;
	byte res;
	while(true){
		res = find_decision_color();
		if(!res) break;	// решено
		int count=0;
		for(int i=0; i < map_size; i++)
			if(c_map[i] != shadow_map[i]){
				count++;
				c_map[i] = shadow_map[i];
				c_reg_solved_cell(i % c_width, i / c_width);
			}
		if(!count) deep_recurse += 30; // увеличение глубины рекурсий если нет результата
		if(deep_recurse >= 300){
			strcpy(last_error, _("No more solve!"));
			set_status_bar(last_error);
			res = -1;
			break;
		}
	}
	if(!res)
		for(int i=0; i<map_size; i++)
			if(c_map[i]!=shadow_map[i]){
				c_map[i]=shadow_map[i];
				c_reg_solved_cell(i % c_width, i / c_width);
			}
	no_logic = false;
	m_free(sds);
}

/*
void SearchSequenceColor(void *sts)
{
	SAVE_CALC_STATE *scs = (SAVE_CALC_STATE *)sts;
	int *Seq = scs->Sequence;
   int *ptr = malloc(sizeof(int) * c_height);
   int volatile i, x, HX;
	for(HX=0; HX<c_height; HX++){
      int Counter=0;
		for(i = 0; i<c_width; i++){
         if(c_map[HX*c_width+i]==EMPTY) Counter++;
			}
      ptr[HX]=Counter ? Counter : Overflow-1;
      }
   for(i=0; i<c_height; i++){                        // Построение последовательности строк
      int tmp=Overflow;
      for(x=0; x<c_height; x++)
      	if(ptr[x]<tmp){
				tmp=ptr[x];
            Seq[i]=x;
            }
      ptr[Seq[i]]=Overflow;
      }
   m_free(ptr);
}

calcres FindDecisionColor(void *sts)
{
   Recurce++;
   bool res;
	int X, Y;
   if(!sts) sts = (void *)malloc(sizeof(SAVE_CALC_STATE));
	SAVE_CALC_STATE *scs = (SAVE_CALC_STATE *)sts;
   scs->RecurceSkip=0;
	SearchSequenceColor(scs);
   for(scs->LinePtr=0; scs->LinePtr<c_height; scs->LinePtr++){// Цикл строк
   	scs->Line = scs->Sequence[scs->LinePtr];
      for(scs->Elem=0; scs->Elem<c_width; scs->Elem++){		// Цикл ячеек
        	scs->pos = scs->Line*c_width+scs->Elem;
         if(c_map[scs->pos]!=EMPTY) continue;
      	scs->orig = access_color+scs->pos*MAX_COLOR;
         scs->ocount = -1; while(scs->orig[++scs->ocount]);
         scs->ErrColCount=0;
			for(scs->ColCount=0; scs->ColCount<scs->ocount; scs->ColCount++){
	         c_map[scs->pos]=scs->orig[scs->ColCount];
   	      x_complete[scs->Elem]=true;
      	   y_complete[scs->Line]=true;

  				res = false;
	         Number=scs->Elem;
				res |= ScanColorLine(That = true);
      	   Number=scs->Line;
	   		res |= ScanColorLine(That = false);
				if(res)
		  			do { // цикл прогона всех строк
   		  			res = false;
                  BeginAccessColor();
						X=0; Y=0;
  						while(X<c_width || Y<c_height){
            		   calcerror = false;
  							if(x_complete[Number=X] && X<c_width) res |= ScanColorLine(That = true);
		      			if(calcerror)	break;
   			   		if(y_complete[Number=Y] && Y<c_height) res |= ScanColorLine(That = false);
  	   					if(calcerror) break;
         		      X++; Y++;
							}
	   				if(Semaphore) {
   	         		scs->RestoreData();
	      	        	delete scs;
   	      	      Recurce--;
      	      	   return USERSTOPED;} // Выход, по требованию
	  	   			} while (res);
			   if(calcerror){
					scs->ErrColCount++;
               scs->RestoreData();
   	         }
				else if(CheckCompleteColor()) {
         	  	delete (SAVE_CALC_STATE *)scs;
      	      Recurce--;
   	         return ALL_OK; // если анализ окончен
	            }
  				else {
      	      int volatile Acount=0, Bcount=0;
   	         for(int i=0; i<map_size; i++)
	            	if(c_map[i]==EMPTY) Acount++;
               	else if(c_map[i]!=scs->SaveMaps[i]) Bcount++;
            	if(Acount<DeepRecurce){
						calcres rfd = FindDecisionColor();
         	   	scs = (SAVE_CALC_STATE *)sts;
      	  			if(rfd==ALL_OK){
   	            	delete scs;
	                  Recurce--;
                  	return rfd;}
               	else if(rfd==USERSTOPED){
            	      scs->RestoreData();
         	      	delete scs;
      	            Recurce--;
   	               return rfd;}
	               else if(rfd==CALCERROR){
                     scs->ErrColCount++;
                     }
      	         }
					else scs->RecurceSkip++;
               scs->RestoreData();
					}
            }
			if(scs->ErrColCount==scs->ocount){
           	scs->SaveMaps[scs->pos]=MARK;
         	scs->Savex_complete[scs->Elem]=true;
      	  	scs->Savey_complete[scs->Line]=true;
				scs->RestoreData();
	     		res = false;
           	Number=scs->pos%c_width;
  				res |= ScanColorLine(That = true);
      	     Number=scs->pos/c_width;
   		  	res |= ScanColorLine(That = false);
	         if(res) {
	      	  	scs->SaveAttr();
   	        	scs->SaveMap();
      	     	}
            }
         }
      }
   delete scs;
	Recurce--;
   if(scs->RecurceSkip)
   	return NOLOGIK;
   return CALCERROR;
}
*/
