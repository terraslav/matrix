// utils.c

#include "utils.h"
#include "main.h"
#include "resource.h"
#include "paint.h"
#include "puzzle.h"
#include "library.h"

#define MAX_UNDO 15000

bool u_begining;	// флаг, устанавливаемый между запусками before - after

//**********************************************************************//
//******************функции работы с undo - redo ***********************//
//**********************************************************************//

void u_change_properties(){
	puzzle_features pf={_("Properties"),last_opened_file,puzzle_name,atom->top_dig_size,atom->left_dig_size,atom->x_size,atom->y_size,atom->isColor};
	char *sv = malloc(2048);
	strcpy(sv, last_opened_file);
	if(!d_new_puzzle_dialog(&pf, false)) {free(sv); return;}
	l_set_name();								// проверка наличия изменений
	if(	pf.top_dig_size == atom->top_dig_size && pf.left_dig_size == atom->left_dig_size &&
		pf.x_size == atom->x_size && pf.y_size == atom->y_size && pf.isColor == atom->isColor){
		if(strcmp(last_opened_file, sv))
			toggle_changes(true, full);
		free(sv);
		return;
	}
	free(sv);
	byte 	w_new = pf.x_size, w_old = atom->x_size,				// сокращаю имена
			h_new = pf.y_size, h_old = atom->y_size;
	byte	tc_new = pf.top_dig_size, tc_old = atom->top_dig_size,
			lc_new = pf.left_dig_size, lc_old = atom->left_dig_size;
	byte	*t_d = MALLOC_BYTE(tc_new * w_new),						// выделяю буфера
			*t_c = MALLOC_BYTE(tc_new * w_new),
			*t_n = MALLOC_BYTE(w_new),
			*l_d = MALLOC_BYTE(lc_new * h_new),
			*l_c = MALLOC_BYTE(lc_new * h_new),
			*l_n = MALLOC_BYTE(h_new),
			*matrix = MALLOC_BYTE(w_new * h_new);
	int x,y;

	u_digits_change_event(true, false);
	c_solve_destroy();

	byte vol, size, count;

	vol = MIN(tc_new, tc_old),
	size = MIN(w_new, w_old);
	for(x=0; x<size; x++){										// корректирую верхнюю оцифровку
		count = 0;
		for(y=0; y<vol; y++){
			t_d[x * tc_new + y] = atom->top.digits[x * tc_old + y];
			t_c[x * tc_new + y] = atom->top.colors[x * tc_old + y];
			if(t_d[x * tc_new + y]) count++;
		}
		t_n[x] = count;
	}
	vol = MIN(lc_new, lc_old),
	size= MIN(h_new, h_old);
	for(y=0; y<size; y++){										// корректирую левую оцифровку
		count = 0;
		for(x=0; x<vol; x++){
			l_d[y * lc_new + x] = atom->left.digits[y * lc_old + x];
			l_c[y * lc_new + x] = atom->left.colors[y * lc_old + x];
			if(l_d[y * lc_new + x]) count++;
		}
		l_n[x] = count;
	}
	vol = MIN(w_new, w_old);
	size = MIN(h_new, h_old);
	for(x=0; x<vol; x++)											// корректирую матрицу
		for(y=0; y<size; y++)
			matrix[y * w_new + x] = atom->matrix[y * w_old + x];

	m_free(atom->top.digits); m_free(atom->left.digits); m_free(atom->left.count); // освобождаю прошлую память
	m_free(atom->top.colors); m_free(atom->left.colors); m_free(atom->top.count);
	m_free(atom->matrix);
	atom->top.digits = t_d; atom->left.digits = l_d;				// обновляю переменные пазла
	atom->top.colors = t_c; atom->left.colors = l_c;
	atom->top.count = t_n; atom->left.count = l_n;
	atom->matrix = matrix;
	atom->top_dig_size = pf.top_dig_size; atom->left_dig_size = pf.left_dig_size;
	atom->x_size = pf.x_size; atom->y_size = pf.y_size;
	atom->x_puzzle_size = pf.x_size + pf.left_dig_size;
	atom->y_puzzle_size = pf.y_size + pf.top_dig_size;
	atom->map_size = w_new * h_new;
	atom->curs->x		= 0;
	atom->curs->y		= 0;
	atom->curs->digits	= top_dig;
	atom->curs->status	= false;

	GtkWidget *wig = gtk_item_factory_get_item_by_action(gtk_item_factory_from_widget(menubar),IDM_PALETTE);
	if(atom->isColor != pf.isColor){
		gtk_widget_set_sensitive(tb[IDT_PALETTE-IDT_BEGIN], pf.isColor);
		gtk_widget_set_sensitive(wig, pf.isColor);
		for(x=0; x<pf.x_size*pf.top_dig_size; x++)
			t_c[x] = (t_d[x] && pf.isColor) ? BLACK : WHITE;
		for(y=0; y<pf.y_size*pf.left_dig_size; y++)
			l_c[y] = (l_d[y] && pf.isColor) ? BLACK : WHITE;
	atom->isColor = pf.isColor;
	current_color = BLACK;
	}
	u_digits_change_event(true, false);
	u_after(false);
	c_solve_init(false);
	toggle_changes(true, full);
	set_window_size(true);
}

void u_menu_sensitive(bool why, bool state){	//why: true:undo, false:redo подсвечиваю пункты меню и тулбара
	gtk_widget_set_sensitive(tb[why?IDT_UNDO-IDT_BEGIN:IDT_REDO-IDT_BEGIN], state);
	gtk_widget_set_sensitive(
		gtk_item_factory_get_item_by_action(
			gtk_item_factory_from_widget(menubar), why?IDM_UNDO:IDM_REDO), state);
}

bool u_compare(byte *one, byte *two, int size){
	while(size--) if(*one++ != *two++) return false;
	return true;
}

void u_delete(){
	int indx = 0;
	while(u_stack[indx] != NULL){
		ur *z = u_stack[indx];
		if(z->a->t) m_free(z->a->t);
		if(z->a->l) m_free(z->a->l);
		if(z->a->m) m_free(z->a->m);
		if(z->b->t) m_free(z->b->t);
		if(z->b->l) m_free(z->b->l);
		if(z->b->m) m_free(z->b->m);
		m_free(z->a);
		m_free(z->b);
		u_stack[indx] = NULL;
	}
}

void u_init(){						// инициализация списка отмен-возвратов
	size_t len = MAX_UNDO * sizeof(ur*);
	u_stack = malloc(len);
	memset(u_stack, 0, len);
	u_current=0;
	u_work = NULL;
	painting = 0;
	change_matrix = change_digits = false;
	u_menu_sensitive(true, false);
	u_menu_sensitive(false, false);
	u_begining = false;
}

void u_new(){						// новый юнит
	ur *z = u_work = malloc(sizeof(ur));
	z->act = u_empty;
	z->a = malloc(sizeof(urs));
	z->b = malloc(sizeof(urs));
	z->a->t = z->a->l = z->a->m = NULL;					// обнуляю содержимое
	z->b->t = z->b->l = z->b->m = NULL;
}

//**********************************************************************//
//****************** упорядочивание оцифровки **************************//
//**********************************************************************//
void u_digits_change_event(bool falls, bool update){	//Пересчитываем количество элементов
													//таблицы
	int count, source, dist, limit, x, y;
	byte	*left_dig		= atom->left.digits,
			*top_dig		= atom->top.digits,
			*top_dig_count	= atom->top.count,
			*left_dig_count	= atom->left.count,
			*top_dig_color	= atom->top.colors,
			*left_dig_color	= atom->left.colors;
	byte	x_size			= atom->x_size,
			y_size			= atom->y_size,
			top_dig_size	= atom->top_dig_size,
			left_dig_size	= atom->left_dig_size;

	if(!top_dig || !left_dig) return;
	for(x=0; x<x_size; x++) {
		count=0;
		for(y=0; y<top_dig_size; y++)
			if(top_dig[x*top_dig_size+y]) count++;
	top_dig_count[x]=count;
	}
	for(y=0; y<y_size; y++){
		count=0;
		for(x=0; x<left_dig_size; x++)
		if(left_dig[y*left_dig_size+x]) count++;
		left_dig_count[y]=count;
	}
	if(!falls){
		if(update) p_update_digits();
		return;
	}
	for(x=0; x<x_size; x++){
		dist=x*top_dig_size;
		limit=dist+top_dig_size;
		count=top_dig_size;
		while(top_dig[dist]) {dist++; if(!--count) break;}
		source=dist+1;
		while(source<limit)
			if(top_dig[source]){
				top_dig[dist]=top_dig[source];
				top_dig[source]=0;
				top_dig_color[dist]=top_dig_color[source];
				top_dig_color[source]=BACK;
				dist++;
				source++;
			}
		else source++;
	 }
  for(x=0; x<y_size; x++){
	 dist=x*left_dig_size;
	 limit=dist+left_dig_size;
	 count=left_dig_size;
	 while(left_dig[dist]) {dist++; if(!--count) break;}
	 source=dist+1;
	 while(source<limit)
		if(left_dig[source]){
		   left_dig[dist]=left_dig[source];
		   left_dig[source]=0;
			  left_dig_color[dist]=left_dig_color[source];
			  left_dig_color[source]=BACK;
		   dist++;
		   source++;
		}
		else source++;
	}
	if(update) p_update_digits();
}

//**********************************************************************//
//**************Расчитываю оцифровку по матрице ************************//
//**********************************************************************//
bool u_digits_updates(bool undos){
  	int count, x, y, indx, max_top = 0, max_left = 0;
  	byte color, element;
	int		x_size 				= atom->x_size,
			y_size				= atom->y_size;
  	byte	*matrix				= atom->matrix;

  	bool block = false;
  	//проверка матрицы на пустоту
  	for(x=0; x < atom->map_size; x++)
		if(atom->matrix[x] != ZERO && atom->matrix[x] != MARK)
			block = true;
  	if(!block){ set_status_bar(_("Matrix is empty!")); return false; }

	// Пересчитываем размеры буферов оцифровки
	for(x=0; x<x_size; x++){							// счетчик колонок
		block = false;
		count = 0;
		for(y=0; y<y_size; y++){						// счетчик строк
			element = matrix[y*x_size+x];
			if(element != ZERO && element != MARK){
														// поиск непустой ячейки в столбце "x"
				if(!block) {color = element; block=true;}	// первая непустая ячейка блока
				else if(color != element)				// если ячейка имеет другой цвет
					{count++; color = element;}			// увеличиваю счетчик блоков и сохраняю его цвет
			}
			else if(block){								// если ячейка пуста и перед ней кончается блок
				block = false;							// то сбрасываем признак блока
				count++;								// и увеличиваем счетчик блоков
			}
		}
		if(block) count++;								// если это конец столбца и к нему примыкает блок
		max_top = MAX(count, max_top);					// то увеличиваем счетчик и выбираем максимальный
	}
	for(y=0; y<y_size; y++){							// то-же самое для строк
		block = false;
		count = 0;
		for(x=0; x<x_size; x++){
			element = matrix[y*x_size+x];
			if(element != ZERO && element != MARK){
				if(!block) {color = element; block=true;}
				else if(color != element)
					{count++; color = element;}
			}
			else if(block){
				block=false;
				count++;
			}
		}
		if(block) count++;
		max_left = MAX(count, max_left);
	}

	// удаляю старые и выделяю новые буфера под оцифровку
	atom->top_dig_size				= max_top;
	atom->left_dig_size				= max_left;
	m_free(atom->top.count);
	m_free(atom->top.digits);
	m_free(atom->top.colors);
	x = atom->top_dig_size*x_size;
	atom->top.count					= MALLOC_BYTE(x_size);
	atom->top.digits 				= MALLOC_BYTE(x);
	atom->top.colors				= MALLOC_BYTE(x);
	m_free(atom->left.count);
	m_free(atom->left.digits);
	m_free(atom->left.colors);
	y = atom->left_dig_size*y_size;
	atom->left.count				= MALLOC_BYTE(y_size);
	atom->left.digits 				= MALLOC_BYTE(y);
	atom->left.colors				= MALLOC_BYTE(y);

	//Обсчитываем верхнюю оцифровку
	for(x=0; x<x_size; x++){							//счетчик столбцов
		block = false;
		atom->top.count[x] = 0;
		count = 0;
		color = ZERO;
		for(y=y_size-1; y>=0; y--){						//обратный счетчик столбца матрицы
			element = matrix[y * x_size + x];			//получаю содержимое ячейки
			indx = x * atom->top_dig_size +
							atom->top.count[x];			//указатель на текущую цифру оцифровки
			if(element != ZERO && element != MARK){		//в ячейке какой-то цвет
				if(color != element){					//если цвет неравен сохранённому
					if(block){							//если был блок
						atom->top.digits[indx] = (byte)count;	//то сохраняю длину блока в оцифровке
						atom->top.colors[indx] = (atom->isColor == true ? color : BACKGROUND);
						atom->top.count[x]++;			//увеличиваю указатель на оцифровку
						count = 0;						//его счетчик
						color = element;				//и значение цвета
					}
					else{
						block = true;					//отмечаю что начался блок
						color = element;				//и сохраняю его цвет
					}
				}
				count++;								//увеличиваю счетчик его длинны
			}
			else if(block){								//если ячейка пуста но блок подсчитывался
				atom->top.digits[indx] = (byte)count;	//то сохраняю длину блока в оцифровке
				atom->top.colors[indx] = (atom->isColor == true ? color : BACKGROUND);
				atom->top.count[x]++;			//увеличиваю указатель на оцифровку
				block = false;							//сбрасываю флажок блока
				count = 0;								//его счетчик
				color = ZERO;							//и значение цвета
			}
		}												//и ухожу в следующюю итерацию поска блоков в столбце

		if(block){										//если цикл окончен с флажком блока
			indx = x * atom->top_dig_size +				//то это привязанный к верхнему краю блок
							atom->top.count[x];	//указатель на текущую цифру оцифровки
			atom->top.digits[indx] = (byte)count;
			atom->top.colors[indx] = (atom->isColor == true ? color : BACKGROUND);
			atom->top.count[x]++;
		}
	}

	for(y=0; y<y_size; y++){							// счетчик строк
		block = false;
		atom->left.count[y]=0;
		count = 0;
		color = ZERO;
		for(x=x_size-1; x>=0; x--){
			element = matrix[y*x_size+x];

			indx = y * atom->left_dig_size + atom->left.count[y];

			if(element != ZERO && element != MARK){
				if(color != element){					//если цвет неравен сохранённому
					if(block){							//если был блок
						atom->left.digits[indx] = (byte)count;	//то сохраняю длину блока в оцифровке
						atom->left.colors[indx] = (atom->isColor == true ? color : BACKGROUND);
						atom->left.count[y]++;	//увеличиваю указатель на оцифровку
						count = 0;						//его счетчик
						color = element;				//и значение цвета
					}
					else{
						block = true;					//отмечаю что начался блок
						color = element;				//и сохраняю его цвет
					}
				}
				count++;								//увеличиваю счетчик его длинны
			}
			else if(block){
				atom->left.digits[indx] = (byte)count;
				atom->left.colors[indx] = (atom->isColor == true ? color : BACKGROUND);
				atom->left.count[y]++;
				block = false;							//сбрасываю флажок блока
				count = 0;								//его счетчик
				color = ZERO;							//и значение цвета
			}
		}
		if(block){
			indx = y * atom->left_dig_size +
							atom->left.count[y];	//указатель на текущую цифру оцифровки
			atom->left.digits[indx] = (byte)count;
			atom->left.colors[indx] = (atom->isColor == true ? color : BACKGROUND);
			atom->left.count[y]++;
		}
	}													// обновляю общий размер пазла
	atom->x_puzzle_size = atom->x_size + atom->left_dig_size;
	atom->y_puzzle_size = atom->y_size + atom->top_dig_size;
	c_solve_init(false);
	if(undos) u_after(false);
	return true;
}

void u_shift_puzzle(direct dir){
	int x, y = atom->y_size;
	byte data;
	byte *source = atom->matrix, *dist=atom->matrix;
	switch(dir){
		case left:
			while(y--){
				x = atom->x_size - 1;
				data = *source++;
				while(x--) *dist++ = *source++;
				*dist++ = data;
			}
         break;

		case right:
			dist = source += (atom->map_size - 1);
			while(y--){
				x = atom->x_size - 1;
				data = *source--;
				while(x--) *dist-- = *source--;
				*dist-- = data;
			}
		break;

		case up:
			for(x=0; x < atom->x_size; x++){
				data = source[x];
				for(y = 0; y < (atom->y_size - 1) * atom->x_size; y += atom->x_size)
					source[x + y] = source[x + y + atom->x_size];
				source[x + atom->x_size * (atom->y_size - 1)] = data;
			}
		break;

		case down:
			for(x=0; x<atom->x_size; x++){
				data = source[x + atom->x_size * (atom->y_size - 1)];
				for(y = (atom->y_size -1) * atom->x_size; y > 0; y -= atom->x_size)
					source[x + y]=source[x + y - atom->x_size];
				source[x] = data;
			}
		break;

		default:
			return;
		}
	u_after(false);
	gtk_widget_queue_draw(draw_area);
}


// ******************* undo-redo struct and functions ***************************

typedef enum u_type_tag{zero,all,map,pal,tdig,tclr,ldig,lclr}u_type;

typedef struct u_state_tag{// хранение состояния пазла
	bool isColor;
	byte width, height;
	byte ntop, nleft;
	byte *dtop, *ctop, *dleft, *cleft;
	byte *matrix;
	RGB palette[MAX_COLOR+2];
}u_state;

union el{
	byte b;				// макс.размер данных - RGB (3 байта)
	RGB	 c;
};

typedef struct u_item_tag{	// хранение элемента матрицы-оцифровки
	word indx;				// коорд.
	union el prv;
	union el aft;
} u_item;

typedef struct u_items_tag{ // структ-маска для списка элементов
	word count;				// кол-во
	u_item *ptr;			// список элементов
} u_items;

union un{
		u_items item;
		u_state *ptr;
};

typedef struct u_itemf_tag{ // структ-маска для списка элементов
	u_type		target;		// тип данных: matrix,all,pal,tdig,tclr,ldig,lclr
	union un 	data;
} u_itemf;


u_itemf *u_begin;
u_state state;

void u_set_state_buttom_undo(bool state){
	gtk_widget_set_sensitive(tb[IDT_UNDO-IDT_BEGIN], state);
	gtk_widget_set_sensitive(
		gtk_item_factory_get_item_by_action(
			gtk_item_factory_from_widget(menubar), IDM_UNDO), state);

}

void u_set_state_buttom_redo(bool state){
	gtk_widget_set_sensitive(tb[IDT_REDO-IDT_BEGIN], state);
	gtk_widget_set_sensitive(
		gtk_item_factory_get_item_by_action(
			gtk_item_factory_from_widget(menubar), IDM_REDO), state);
}

bool u_del_item(uint num){
	u_type type = u_begin[num].target;
	if(type == zero) return false;
	if(type == all){
		u_state *s = u_begin[num].data.ptr;
		m_free(s->dtop); m_free(s->ctop);
		m_free(s->dleft); m_free(s->cleft);
		m_free(s->matrix);
		m_free(u_begin[num].data.ptr);
	}
	else m_free(u_begin[num].data.item.ptr);
	u_begin[num].target = zero;
	return true;
}

void u_clear_stack_redo(){
	u_set_state_buttom_redo(false);
	int i = u_current;
	while(u_del_item(i++));
}

void u_clear(int i){					// очищаю стек отмен с определённой позиции
	if(i == 0) u_set_state_buttom_undo(false);
	u_set_state_buttom_redo(false);
	u_current = i;
	if(!(bool)u_begin) return;
	while(u_del_item(i++));
}

void u_save_state(){
	if(	state.width != atom->x_size || state.height != atom->y_size){	// корректирую размер буферов
		m_free(state.matrix);
		state.matrix = malloc(atom->x_size*atom->y_size);
	}
	if(	state.ntop != atom->top_dig_size || state.width != atom->x_size){
		m_free(state.dtop);
		state.dtop	= malloc(atom->x_size * atom->top_dig_size);
		m_free(state.ctop);
		state.ctop	= malloc(atom->x_size * atom->top_dig_size);
	}
	if(state.nleft != atom->left_dig_size || state.height != atom->y_size){
		m_free(state.dleft);
		state.dleft = malloc(atom->y_size * atom->left_dig_size);
		m_free(state.cleft);
		state.cleft = malloc(atom->y_size * atom->left_dig_size);
	}
	state.width = atom->x_size; state.height = atom->y_size;
	state.ntop	= atom->top_dig_size; state.nleft = atom->left_dig_size;
	state.isColor = atom->isColor;

	memcpy(state.matrix, atom->matrix, atom->map_size);
	memcpy(state.dtop, atom->top.digits, state.width*state.ntop);
	memcpy(state.ctop, atom->top.colors, state.width*state.ntop);
	memcpy(state.dleft, atom->left.digits, state.height*state.nleft);
	memcpy(state.cleft, atom->left.colors, state.height*state.nleft);
	memcpy(state.palette, atom->rgb_color_table, (MAX_COLOR+2)*sizeof(RGB));
}

void u_state_free(){
	m_free(state.matrix);
	m_free(state.dtop);m_free(state.ctop);
	m_free(state.dleft);m_free(state.cleft);
}

void u_push_map(size_t count){					// вталкиваю изменённые ячейки матрицы
	uint i;
	u_clear_stack_redo();
	u_begin[u_current].target = map;
	u_begin[u_current].data.item.count = count;
	u_item* items = u_begin[u_current].data.item.ptr = malloc(sizeof(u_item)*count);
	for(i=0; i<atom->map_size; i++)
		if(state.matrix[i] != atom->matrix[i]){
			count--;
			items[count].indx = i;
			items[count].prv.b = state.matrix[i];
			items[count].aft.b = atom->matrix[i];
		}
	u_current++;
}

void u_push_top(size_t count, bool why){		// ячейки верхней оцифровки
	uint i;
	byte *s, *d;
	u_clear_stack_redo();
	u_begin[u_current].target = why ? tdig : tclr;
	u_begin[u_current].data.item.count = count;
	u_item* items = u_begin[u_current].data.item.ptr = malloc(sizeof(u_item*)*count);
	s = why ? atom->top.digits : atom->top.colors;
	d = why ? state.dtop : state.ctop;
	for(i=0; i<state.ntop*state.width; i++)
		if(s[i] != d[i]){
			count--;
			items[count].indx = i;
			items[count].prv.b = d[i];
			items[count].aft.b = s[i];
		}
	u_current++;
}

void u_push_left(size_t count, bool why){		// ячейки левой оцифровки
	uint i;
	byte *s, *d;
	u_clear_stack_redo();
	u_begin[u_current].target = why ? ldig : lclr;
	u_begin[u_current].data.item.count = count;
	u_item* items = u_begin[u_current].data.item.ptr = malloc(sizeof(u_item*)*count);
	s = why ? atom->left.digits : atom->left.colors;
	d = why ? state.dleft : state.cleft;
	for(i=0; i<state.nleft*state.width; i++)
		if(s[i] != d[i]){
			count--;
			items[count].indx = i;
			items[count].prv.b = d[i];
			items[count].aft.b = s[i];
		}
	u_current++;
}

void u_push_palette(size_t count){
	uint i;
	u_clear_stack_redo();
	u_begin[u_current].target = pal;
	u_begin[u_current].data.item.count = count;
	u_item* items = u_begin[u_current].data.item.ptr = malloc(sizeof(u_item*)*count);
	for(i=0; i<MAX_COLOR+2; i++)
		if(	state.palette[i].r != atom->rgb_color_table[i].r ||
			state.palette[i].g != atom->rgb_color_table[i].g ||
			state.palette[i].b != atom->rgb_color_table[i].b){

			count--;
			items[count].indx = i;
			items[count].prv.c = state.palette[i];
			items[count].aft.c = atom->rgb_color_table[i];
		}
	u_current++;
}

void u_push_all(){
	u_clear_stack_redo();
	u_begin[u_current].target = all;
	u_begin[u_current].data.ptr = malloc(sizeof(u_state));
	memcpy(u_begin[u_current].data.ptr, &state, sizeof(u_state));// копирую структуру состояния в список undo
	memset(&state, 0, sizeof(u_state));							// и обновляю данные состояния на текущие
	u_save_state();
	u_current++;
}

void u_swap_all(bool u){
	u_state t;
	memcpy(&t, &state, sizeof(u_state));
	memcpy(&state, u_begin[u_current].data.ptr, sizeof(u_state));
	memcpy(u_begin[u_current].data.ptr, &t, sizeof(u_state));

	if(atom->x_size!=state.width || atom->y_size!=state.height){
		m_free(atom->matrix); atom->matrix			= MALLOC_BYTE(state.width*state.height);
	}
	if(atom->top_dig_size!=state.ntop || atom->x_size!=state.width){
		m_free(atom->top.digits); atom->top.digits	= MALLOC_BYTE(state.width*state.ntop);
		m_free(atom->top.colors);atom->top.colors	= MALLOC_BYTE(state.width*state.ntop);
		m_free(atom->top.count); atom->top.count	= MALLOC_BYTE(state.width);
	}
	if(atom->left_dig_size!=state.nleft || atom->y_size!=state.height){
		m_free(atom->left.digits);atom->left.digits	= MALLOC_BYTE(state.height*state.nleft);
		m_free(atom->left.colors);atom->left.colors	= MALLOC_BYTE(state.height*state.nleft);
		m_free(atom->left.count); atom->left.count	= MALLOC_BYTE(state.height);
	}
	atom->x_size = state.width; atom->y_size = state.height;
	atom->top_dig_size = state.ntop; atom->left_dig_size = state.nleft;
	atom->isColor = state.isColor;

	memcpy(atom->top.digits, state.dtop, state.width*state.ntop);
	memcpy(atom->top.colors, state.ctop, state.width*state.ntop);
	memcpy(atom->left.digits, state.dleft, state.height*state.nleft);
	memcpy(atom->left.colors, state.cleft, state.height*state.nleft);
	memcpy(atom->rgb_color_table, state.palette, (MAX_COLOR+2)*sizeof(RGB));
	u_digits_change_event(false, false);
	c_solve_init(false);
	gtk_widget_queue_draw(draw_area);
}

void u_swap_map(bool u){
	u_item* items = u_begin[u_current].data.item.ptr;
	uint i, count = u_begin[u_current].data.item.count;
	c_clear_errors();
	CAIRO_INIT;
	for(i=0; i<count; i++){
		if(u)atom->matrix[items[i].indx] = items[i].prv.b;	// undo
		else atom->matrix[items[i].indx] = items[i].aft.b; // redo
		if(count < 10 )	p_u_paint_cell(items[i].indx);
	}
	if(count >= 10 ){
		p_update_matrix();
	}
	p_draw_icon(false);
}

void u_swap_pal(bool u){
	u_item* items = u_begin[u_current].data.item.ptr;
	for(uint i=0; i<u_begin[u_current].data.item.count; i++)
		if(u)	atom->rgb_color_table[items[i].indx] = items[i].prv.c;
		else	atom->rgb_color_table[items[i].indx] = items[i].aft.c;
	d_update_palette();
}

void u_swap_top(bool u, bool why){
	u_item* items = u_begin[u_current].data.item.ptr;
	uint x, y, i, count = u_begin[u_current].data.item.count;
	for(i=0; i<count; i++){
		if(why){
			if(u)atom->top.digits[items[i].indx] = items[i].prv.b;	// undo
			else atom->top.digits[items[i].indx] = items[i].aft.b; // redo
		}
		else {
			if(u)atom->top.colors[items[i].indx] = items[i].prv.b;	// undo
			else atom->top.colors[items[i].indx] = items[i].aft.b; // redo
		}
		x = (items[i].indx / atom->top_dig_size + atom->left_dig_size)*
													cell_space + atom->x_begin;
		y = (atom->top_dig_size-1-items[i].indx % atom->top_dig_size) *
													cell_space + atom->y_begin;
		p_redraw_digit(&atom->top, items[i].indx, x, y);
	}
	if(why) c_solve_init(false);
}

void u_swap_left(bool u, bool why){
	u_item* items = u_begin[u_current].data.item.ptr;
	uint x, y, i, count = u_begin[u_current].data.item.count;
	for(i=0; i<count; i++){
		if(why){
			if(u)atom->left.digits[items[i].indx] = items[i].prv.b;	// undo
			else atom->left.digits[items[i].indx] = items[i].aft.b; // redo
		}
		else {
			if(u)atom->left.colors[items[i].indx] = items[i].prv.b;	// undo
			else atom->left.colors[items[i].indx] = items[i].aft.b; // redo
		}
		x = (atom->left_dig_size-1-items[i].indx%atom->left_dig_size)*
													cell_space + atom->x_begin;
		y = (atom->top_dig_size + items[i].indx /atom->left_dig_size)*
													cell_space + atom->y_begin;
		p_redraw_digit(&atom->left, items[i].indx, x, y);
	}
	if(why) c_solve_init(false);
}

void u_swap_event(bool u){
	u_type type = u_begin[u_current].target;
	switch(type){
		case all:
			u_swap_all(u);
			break;
		case map:
			u_swap_map(u);
			break;
		case pal:
			u_swap_pal(u);
			break;
		case tdig:
		case tclr:
			u_swap_top(u, type==tdig);
			break;
		case ldig:
		case lclr:
			u_swap_left(u, type==ldig);
			break;
		default:
			break;
	}
}

void u_undo(){
	if(u_current > 0) --u_current;
	if(!u_current){
		toggle_changes(false, full);
		u_set_state_buttom_undo(false);
	}
	u_swap_event(true);
	if(u_begin[u_current].target) u_set_state_buttom_redo(true);
	if(u_begin[u_current].target != all) u_save_state();
}

void u_redo(){
	if(!u_begin[u_current].target) return;
	u_swap_event(false);
	u_current++;
	if(u_current == 1)toggle_changes(true, full);
	if(!u_begin[u_current].target){
		u_set_state_buttom_redo(false);
	}
	u_set_state_buttom_undo(true);
	if(u_begin[u_current].target != all) u_save_state();
}

void u_after(bool newfile){
#define BEGIN 1234567890
#define WAIT 2229558312
	static long uinit = 0;
	uint i, count, total;

	if(!uinit){									// инициализация
		uinit = BEGIN;
		u_begin = MALLOC_BYTE(sizeof(u_items) * MAX_UNDO);
		u_current = 0;
		memset(&state, zero, sizeof(u_state));
		u_save_state();
		return;
	}

	if(uinit == WAIT){			 				// межпоточная синхронизация
		count = 0;
		while(uinit != BEGIN)
			{printf("wait %d\t", count++);sleep(0.2);}
	}
	else uinit = WAIT;
	if(newfile){
		u_clear(0);
		u_save_state();
		uinit = BEGIN;
		return;
	}
	if(	state.width!=atom->x_size || state.height!=atom->y_size ||
		state.ntop!=atom->top_dig_size || state.nleft!=atom->left_dig_size){
		u_push_all();		// втыкаю в стек состояние целиком если были изменения в размерах
		uinit = BEGIN;
		return;
	}
	// проверяю изменения в матрице
	count = 0; i = atom->map_size;
	byte *s = state.matrix, *d = atom->matrix;
	while(i--) if(*s++ != *d++) count++;
	if(count){
		u_push_map(count);
		toggle_changes(true, matrix);
		u_set_state_buttom_undo(true);
	}

	// в верхней оцифровке
	total = count = 0; i = atom->top_dig_size*atom->x_size;
	s = state.dtop; d = atom->top.digits;
	while(i--) if(*s++ != *d++) count++;
	total += count;
	if(count) u_push_top(count, true);
	count = 0; i = atom->top_dig_size*atom->x_size;
	s = state.ctop; d = atom->top.colors;
	while(i--) if(*s++ != *d++) count++;
	total += count;
	if(count) u_push_top(count, false);
	// в левой оцифровке
	count = 0; i = atom->left_dig_size*atom->y_size;
	s = state.dleft; d = atom->left.digits;
	while(i--) if(*s++ != *d++) count++;
	total += count;
	if(count) u_push_left(count, true);
	count = 0; i = atom->left_dig_size*atom->y_size;
	s = state.cleft; d = atom->left.colors;
	while(i--) if(*s++ != *d++) count++;
	total += count;
	if(count) u_push_left(count, false);
	// в палитре
	count = 0;
	for(i=0; i<MAX_COLOR+2; i++)
		if(	state.palette[i].r != atom->rgb_color_table[i].r ||
			state.palette[i].g != atom->rgb_color_table[i].g ||
			state.palette[i].b != atom->rgb_color_table[i].b)
			count++;
	if(count) u_push_palette(count);

	if(total || count){
		toggle_changes(true, digits);
		u_set_state_buttom_undo(true);
	}
	// обновляю текущее состояние
	u_save_state();
	uinit = BEGIN;
}
