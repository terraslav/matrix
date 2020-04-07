// mafile.c
#include <time.h>
#include <sys/stat.h>

#include "resource.h"
#include "main.h"
#include "paint.h"
#include "mafile.h"
#include "utils.h"
#include "dialog.h"
#include "library.h"

file_buffer *m_prepare_jcc(void);
file_buffer *m_prepare_jcr(void);
bool m_open_jcr (byte*);
bool m_open_jcc (file_buffer*);
bool m_open_bmp (byte*);

//******** процедура поиска индекса ближайшего цвета в таблице *********//
byte m_find_nearest_color_index(COLORREF color){
    int i, nearest = 0;
    double fi, min=3866624;
    byte r = GetRValue(color), g = GetGValue(color), b = GetBValue(color);
	for(i = 1; i < MAX_COLOR + 2; i++){
		double dr = atom->rgb_color_table[i].r-r; dr = (dr < 0) ? -dr : dr; dr*=30;	//30% * red
		double dg = atom->rgb_color_table[i].g-g; dg = (dg < 0) ? -dg : dg; dg*=59;	//59% * green
		double db = atom->rgb_color_table[i].b-b; db = (db < 0) ? -db : db; db*=11;	//11% * blue*/
		fi = dr + dg + db;
		if(fi<=min){nearest = i, min = fi;}
	}
    return (byte)nearest;
}

byte m_get_nearest_color_index(RGBQUAD a){
	byte r = a.rgbRed, g = a.rgbGreen, b = a.rgbBlue;
	if(!r && !g && !b) return 1;						// черный
	if(r == 0xff && g == 0xff && b == 0xff) return 0;	// белый(фон)
	return m_find_nearest_color_index(RGBtoCOLORREF(r, g, b));
}

//******************* Инициализация элементов управления **********************//

void m_enable_palette_items(){
	GtkItemFactory *item_factory = gtk_item_factory_from_widget(menubar);
	GtkWidget *wig = gtk_item_factory_get_item_by_action(item_factory, IDM_PALETTE);
	gtk_widget_set_sensitive(wig, atom->isColor);
	gtk_widget_set_sensitive(tb[IDT_PALETTE-IDT_BEGIN], atom->isColor);
}

void m_enable_items(){
	int mitems[] = {	//активирую пункты меню и тулбара
		IDM_SAVE_AS, IDM_PRINT, IDM_FEATURES, IDM_PROPERTIS,
		IDM_INVERT, IDM_ROTATE_LEFT, IDM_ROTATE_RIGHT, IDM_ROTATE_UP,
		IDM_ROTATE_DOWN, IDM_CLEAR_PUZZLE, IDM_CLEAR_DIGITS,
		IDM_CLEAR_PUZ_MARK, IDM_CLEAR_DIG_MARK, IDM_MORE, IDM_LESS,
		IDM_NORMAL, IDM_GRID, IDM_GRID5, IDM_SOLVE, IDM_CALC_DIG,
		IDM_MAKE_OVERLAPPED, IDM_HELP, IDM_ABOUT
	};
	enable_menu_item(mitems, sizeof(mitems)/sizeof(int), TRUE);
	m_enable_palette_items();
}

//******************* Работа с файлами **********************//

bool m_new_puzzle(){
	puzzle_features pf={_("New puzzle"),NEW_FILE_NAME,_("Noname"),5,5,25,25,false};
	if(!d_new_puzzle_dialog(&pf, true)) return false;
	if(atom){
		c_free_puzzle_buf(atom);
		m_free(atom);
		l_close_jch();
	}

 	atom = MALLOC_BYTE(sizeof(puzzle));	// создаю пазл
 	atom->x_size	= pf.x_size;
  	atom->y_size	= pf.y_size;
	atom->top_dig_size	= pf.top_dig_size;
	atom->left_dig_size	= pf.left_dig_size;
	atom->isColor		= pf.isColor;
	atom->map_size		= atom->x_size * atom->y_size;
	atom->x_puzzle_size	= atom->x_size + atom->left_dig_size;
	atom->y_puzzle_size	= atom->y_size + atom->top_dig_size;
	c_malloc_puzzle_buf(atom);

	ftype = jcc;
	place = exfile;
	current_color = BLACK;
	m_enable_items();
	set_status_bar("_(Created new puzzle)");
	toggle_changes(false, full);		// сброс флага изменений в пазле
	c_solve_init(false);
	u_after(true);
	set_window_size(false);
	return true;
}

f_type m_get_file_type(char *lof){
	char *exts[] = {"jcr", "jcc", "jch", "bmp", "mem"};
	char ext[10];										// получаю расширение файла
	char *ptr = lof + strlen(lof) - 4;
	while(*ptr != '.') return empty;					// отсеиваю ненужное
	strcpy(ext, ++ptr);
	for(int i = 0; ext[i] != '\0'; i++) ext[i] |= 0x20;	// понижаю регистр
	if(		!strcmp(ext, exts[0])) return jcr;
	else if(!strcmp(ext, exts[1])) return jcc;
	else if(!strcmp(ext, exts[2])) return jch;
	else if(!strcmp(ext, exts[3])) return bmp;
	else if(!strcmp(ext, exts[4])) return mem;
	return empty;
}

void m_set_title(){
	bool action = change_matrix | change_digits;
	*puzzle_name = '\0';
	char *buf = malloc(512);
	char *nm = m_get_file_path_name(false);
	if(atom && atom->curs->status){
		strcpy(buf, _("Digits editing"));
	}
	else if((ftype == jch || ftype == mem)  && ig_start && parameter[viewname]){
		strcpy(puzzle_name, l_get_name());
		if(nm) sprintf(buf, "%s%s%s%s - %s", place == memory ? _("Library") : _("File"), ": ", action ? "*" : "", nm, puzzle_name);
		else sprintf(buf, "%s%s", action ? "*" : "", puzzle_name);
	}
	else if(nm){
		strcpy(puzzle_name, nm);
		sprintf(buf, "%s%s", action ? "*" : "", puzzle_name);
	}
	else sprintf(buf, "%s%s", action ? "*" : "", "name is empty");
	gtk_window_set_title(GTK_WINDOW(window), buf);
	m_free(nm);
	m_free(buf);
}

bool m_get_puzzle_default(){
	byte sz[4] = {10,21,10,4};
	char *def =
"113113|1111111|321111|111333|3|1312|11311111|1111111111|22112111|1131132|\
5|41|11|41|4|5|1114|1111|4|4|15|111|31|4|15|11|1|4|115|411|3|";
	char *p = def;
	int x, y;
	if(atom){
		c_solve_destroy();
		c_free_puzzle_buf(atom);
		m_free(atom);
	}
 	atom = MALLOC_BYTE(sizeof(puzzle));
 	atom->x_size	= sz[1];
  	atom->y_size	= sz[0];
	atom->top_dig_size	= (int)sz[3];
	atom->left_dig_size	= (int)sz[2];
	atom->isColor		= false;
	atom->map_size		= atom->x_size * atom->y_size;
	atom->x_puzzle_size	= atom->x_size + atom->left_dig_size;
	atom->y_puzzle_size	= atom->y_size + atom->top_dig_size;
	c_malloc_puzzle_buf(atom);
	for(y=0; y<sz[0]; y++)
		for(x=0; x<=sz[2]; x++){
			if(*p != '|') atom->left.digits[(y+1)*sz[2]-x-1] = (*p++ - '0');
			else {p++; break;}
		}
	for(x=0; x<sz[1]; x++)
		for(y=0; y<=sz[3]; y++){
			if(*p != '|') atom->top.digits[(x+1)*atom->top_dig_size-y-1] = (*p++ - '0');
			else {p++; break;}
		}
	current_color = BLACK;
	u_digits_change_event(true, false);
	strcpy(puzzle_name, "Hello World");
	strcpy(last_opened_file, "Hello_World.jch");
	m_set_title();
	set_window_size(true);
	c_solve_init(true);
	u_after(true);
	return true;
}

uint m_get_base_number_file(char *data){
	char *p = data;
	while(*p != 0 && (*p > '9' || *p < '0')) p++;
	if(!*p) return 0;
	uint a = 0;
	while(*p >= '0' && *p <= '9')
		{ a *= 10; a += *p++ - '0'; }
	return a;
}

bool m_set_base_number_file(char *data){
	char *p = strstr(data, "{\"id");
	if(!p) return false;
	p += 6;
	uint a = 0;
	while(*p >= '0' && *p <= '9')
		{ a *= 10; a += *p++ - '0'; }
	if(a >= l_base_stack_top){
		last_opened_number = 0;
		return false;
	}
	last_opened_number = a;
	sprintf(last_opened_file, "db/%d.mem", a);
	return true;
}

bool m_open_puzzle(bool new, bool view, char *data){
	if(thread_solve_no_logic) pthread_cancel(thread_solve_no_logic);
	srand(time(NULL));
begin:
	ftype = m_get_file_type(last_opened_file);
	if(new){
		bool ofd = d_open_file_dialog();
		ftype = m_get_file_type(last_opened_file);
		if(!ofd && ftype != empty)
			return true;							// выход если буфер не пуст и была отмена выбора нового файла
	}
	else if(data == NULL && (ftype == empty || (*last_opened_file && (!g_file_test(last_opened_file,
			G_FILE_TEST_EXISTS) || g_file_test(last_opened_file, G_FILE_TEST_IS_DIR))))){
		if(!l_base_stack_top) return m_get_puzzle_default();
		last_opened_number = rand()%l_base_stack_top;	// выбираю случайный файл из библиотеки
		data = l_get_item_from_base(last_opened_number)->body;
		ftype = mem;
		place = memory;
	}
	new = true;
	l_close_jch();
	current_color = BLACK;
	file_buffer f_data;
	if(data == NULL && *last_opened_file && g_file_test(last_opened_file, G_FILE_TEST_EXISTS)){
		FILE* file = fopen(last_opened_file, "rb");
		if(!file) goto begin;
		fseek(file,0,SEEK_END);
		f_data.size = ftell(file);
		if(f_data.size == 0){fclose(file); return false;}

		f_data.data = MALLOC_BYTE(f_data.size + 3);

		fseek(file,0,SEEK_SET);
		fread(f_data.data, f_data.size, 1, file);
		fclose(file);
		ftype = m_get_file_type(last_opened_file);
		place = exfile;
	}
	else {
		f_data.data = data;
		if(!m_set_base_number_file(data)){
			d_message(_("Puzzle number is empty!"), _("Sorry, puzzle number is empty! Select other number."), 0);
			return false;
		}
	}
	if(atom){
		c_solve_destroy();		// зачищаю буфера решения
		c_free_puzzle_buf(atom);
		m_free(atom);
	}
	bool res;
	switch(ftype){
		case jcr:
			res = m_open_jcr(f_data.data);
			break;
		case jcc:
			res = m_open_jcc(&f_data);
			break;
		case mem:
		case jch:
			res = l_open_jch(f_data.data);
			break;
		case bmp:
			res = m_open_bmp(f_data.data);
			if(res){
				char *ptr = last_opened_file + strlen(last_opened_file) - 3;
				strcpy(ptr, "jcc");							// подменяю расширение имени файла
				ftype = jcc;
			}
			break;
		default:
			*last_opened_file = 0;
			res = false;
	}
	if(!res){
		c_free_puzzle_buf(atom);
		if(data != f_data.data) m_free(f_data.data);
		m_free(atom);
		gtk_window_set_title (GTK_WINDOW(window), CAPTION_TEXT);
		ftype = empty;
		goto begin;
	}
	m_enable_items();
	char *sbuf = malloc(MAX_PATH);
	sprintf(sbuf, _("Opened file: %s"), last_opened_file);
	set_status_bar(sbuf);
	free(sbuf);
	sbuf = m_get_file_path_name(false);
	if(sbuf){
		last_opened_number = l_atoi(sbuf);
		free(sbuf);
	}
	if(data != f_data.data) m_free(f_data.data);
	set_window_size(true);
	c_solve_init(view);										// инициализирую "решалку"
	toggle_changes(false, full);							// сброс флага изменений в пазле
	u_after(true);
	save_parameter();
	return true;
}

bool m_open_jcr (byte *buf){
	int i,j;
	JCR_FILE_HEADER *file = (JCR_FILE_HEADER *) buf;

 	atom = MALLOC_BYTE(sizeof(puzzle));	// создаю пазл
 	atom->x_size	= (byte)file->x_size;
  	atom->y_size	= (byte)file->y_size;
	atom->top_dig_size	= (int)(file->top_dig_size & 0x7f);
	atom->left_dig_size	= (int)(file->left_dig_size & 0x7f);
	atom->isColor		= ((file->top_dig_size & 0x80) >> 7);
	atom->map_size		= atom->x_size * atom->y_size;
	atom->x_puzzle_size	= atom->x_size + atom->left_dig_size;
	atom->y_puzzle_size	= atom->y_size + atom->top_dig_size;
	byte *ptr			= file->data;						// Указатель на данные оцифровки
	c_malloc_puzzle_buf(atom);
// Заполнение массивов оцифровки данными из файла
// сперва идет верхняя оцифровка заполняется снизу вверх слева направо
// затем нижняя оцифровка справа налево снизу вверх
	if(!atom->isColor){
		for(i=0; i<atom->x_size; i++)
			for(j=0; j<atom->top_dig_size; j++)
				atom->top.digits[i * atom->top_dig_size + j] = *(ptr++);
		for(i=0; i<atom->y_size; i++)
			for(j=0; j<atom->left_dig_size; j++)
				atom->left.digits[atom->left_dig_size*atom->y_size-((i+1)*atom->left_dig_size)+j] = *(ptr++);
	}
	else { 													// заливка цветного формата
															// заливаем оцифровку
		for(i=0; i<atom->x_size; i++)						// счетчик столбцов
			for(j=atom->top.count[i]=*ptr++; j>0; j--){
				if(j>(int)atom->top_dig_size) return false;
				atom->top.digits[(i+1)*atom->top_dig_size-j]=*(ptr++);
			}
	  	for(i=0; i<atom->y_size; i++)						// счетчик строк
  			for(j=atom->left.count[i]=*ptr++; j>0; j--){
				if(j>atom->left_dig_size) return false;
				atom->left.digits[(i+1)*atom->left_dig_size-j]=*ptr++;
            }
									// заливаем цвета оцифровки
	  	for(i=0; i<atom->x_size; i++)						// счетчик столбцов
  			for(j=atom->top.count[i]; j>0; j--){
				if(j>atom->top_dig_size) return false;
         	atom->top.colors[(i+1)*atom->top_dig_size-j] = *ptr++ + 1;
             }
	  	for(i=0; i<atom->y_size; i++)						// счетчик строк
  			for(j=atom->left.count[i]; j>0; j--){
				if(j>atom->left_dig_size) return false;
            atom->left.colors[(i+1)*atom->left_dig_size-j] = *ptr++ + 1;
             }
		}
	u_digits_change_event(true, false);
	return true;
}

bool m_open_jcc (file_buffer *f_data){
	if(atom) return false;

	int i, j, c, indx, color;
	JCC_DD *ptr;
	byte isEmpty;
	JCC_FILE_DATA_OLD *older = (JCC_FILE_DATA_OLD *)	f_data->data;
	JCC_FILE_DATA *news = (JCC_FILE_DATA *)				f_data->data;
	COLORREF *color_table;

	atom					= MALLOC_BYTE(sizeof(puzzle));	// создаю хэдер пазла

	char old_head[] = {"JAPAN"};	// определение старого формата
	char *cur_head = (char*)f_data->data;
	for(i = 0; i < 5; i++) if(cur_head[i] != old_head[i]) break;
	bool old_format = (i == 5);

	if(old_format) {		// если старый формат файла в 16 цветов
		atom->x_size			= (byte)older->x_size;
		atom->y_size			= (byte)older->y_size;
		atom->top_dig_size		= (int)(older->top_dig_size);
		atom->left_dig_size		= (int)(older->left_dig_size);
		isEmpty					= older->isEmpty;
		atom->isColor			= older->color > 2 ? true : false;
		color					= older->color;
		color_table				= older->color_table;
		ptr						= older->data;				// Указатель на оцифровку
	}
	else{
		atom->x_size			= (byte)news->x_size;
		atom->y_size			= (byte)news->y_size;
		atom->top_dig_size		= (int)(news->top_dig_size);
		atom->left_dig_size		= (int)(news->left_dig_size);
		isEmpty					= news->isEmpty;
		atom->isColor			= news->color > 0 ? true : false;
		color					= news->color;
		color_table				= news->color_table;
		ptr						= f_data->data + sizeof(JCC_FILE_DATA) - sizeof(COLORREF) + sizeof(COLORREF) * color;
	}
	atom->map_size			= atom->x_size * atom->y_size;
	atom->x_puzzle_size		= atom->x_size + atom->left_dig_size;
	atom->y_puzzle_size		= atom->y_size + atom->top_dig_size;
	c_malloc_puzzle_buf(atom);	// выделение памяти буферам текущего пазла

//	byte convert_table[MAX_COLOR + 2];
//	convert_table[0] = 0; convert_table[1] = 1;
	if(atom->isColor){
		if(old_format)
			for(i = 2; i < color; i++){					// заполнение палитры ближайших цветов
				atom->rgb_color_table[i].r = GetRValue(color_table[i]);
				atom->rgb_color_table[i].g = GetRValue(color_table[i]);
				atom->rgb_color_table[i].b = GetRValue(color_table[i]);
//				convert_table[i] = m_find_nearest_color_index(color_table[i]);
			}
		else
			for(i=0; i < color; i++){
				atom->rgb_color_table[i+2].r = GetRValue(color_table[i]);
				atom->rgb_color_table[i+2].g = GetGValue(color_table[i]);
				atom->rgb_color_table[i+2].b = GetBValue(color_table[i]);
//				convert_table[i+2] = m_find_nearest_color_index(color_table[i]);
			}
	}

//Заполнение массивов оцифровки данными из файла
	for(j=0; j<atom->x_size; j++)
		for(i=0; i<atom->top_dig_size; i++){
//			if(convert_table[ptr[i + atom->top_dig_size * j].color]!=18){
				c = atom->top_dig_size * (j + 1) - 1 - i;	// адрес в пазле
				indx = i + atom->top_dig_size * j;			// адрес в файле
				atom->top.digits[c] = ptr[indx].digit & COL_MARK_MASK;
//				atom->top.colors[c]	= convert_table[ptr[indx].color & COL_MARK_MASK];
				atom->top.colors[c]	= ptr[indx].color & COL_MARK_MASK;
				if(!demo_flag)
					atom->top.colors[c]	|= (byte)(ptr[indx].color & COLOR_MARK);
			}
	ptr += (atom->x_size * atom->top_dig_size);
	for(j=0; j<atom->y_size; j++)
		for(i=0; i<atom->left_dig_size; i++){
//			if(convert_table[ptr[i + atom->left_dig_size * j].color]!=18){
				c = atom->left_dig_size * (j + 1) - i - 1;
				indx = i + atom->left_dig_size * j;
				atom->left.digits[c] 	= ptr[indx].digit & COL_MARK_MASK;
//				atom->left.colors[c]	= convert_table[ptr[indx].color & COL_MARK_MASK];
				atom->left.colors[c]	= ptr[indx].color & COL_MARK_MASK;
				if(!demo_flag)
					atom->left.colors[c] 	|= (byte)(ptr[indx].color & COLOR_MARK);
//			}
		}
	if(!demo_flag && !isEmpty){			// копирую матрицу
		byte *bptr = (byte *)(atom->y_size * atom->left_dig_size + ptr);
		for(i=0; i<atom->map_size; i++)
//			if(bptr[i] < MAX_COLOR + 2)
//				atom->matrix[i] = convert_table[bptr[i]];
//			else
				atom->matrix[i] = bptr[i];
	}
	u_digits_change_event(true, false);
	return true;
}

bool m_open_bmp (byte *data){
	static char caption[]={"Error opening bitmap pictures."};
	FILEBITMAPINFO *bmp = (FILEBITMAPINFO *)data;
	bool UpToDown = bmp->Bh.biHeight>0 ? false : true;
	int x,y,i, color;
	byte pixel;

	atom				= MALLOC_BYTE(sizeof(puzzle));	// создаю пазл
  	atom->top_dig_size	= atom->left_dig_size = 0;
	atom->x_size		= (int)bmp->Bh.biWidth;
	atom->y_size		= (int)(bmp->Bh.biHeight>0 ? bmp->Bh.biHeight : -(bmp->Bh.biHeight));
	atom->map_size		= atom->x_size * atom->y_size;
	atom->x_begin		= atom->y_begin = 0;
	atom->top_col		= atom->left_row = -1;
	atom->curs			= MALLOC_BYTE(sizeof (cursor));
	atom->curs->x		= 0;
	atom->curs->y		= 0;
	atom->curs->digits	= top_dig;
	atom->curs->status	= false;

	int count_bits		= bmp->Bh.biBitCount;// Количество бит на пиксел 1, 4, 8 или 24
	if(bmp->Bh.biClrUsed == 0)	color = 2^count_bits;	//((byte)0x01 << count_bits);
	else						color = (int)bmp->Bh.biClrUsed;
	if(color == 2) color--;
	atom->isColor		= (count_bits > 1) ? true : false;

 	if(bmp->Bh.biCompression != BI_RGB){	// нет поодержки сжатия
		d_message(_(caption), _("The compressed bitmap is not supported!"), 0);
		return false;
	}										// слишком большие размеры картинки >127
	if(bmp->Bh.biWidth > MAX_MATRIX_SIZE || atom->y_size > MAX_MATRIX_SIZE){
		d_message(_(caption), _("Width and height image more that 127 cells!"), 0);
		return false;
    }

	atom->left.count = atom->top.count = atom->top.digits = atom->left.digits = atom->top.colors = atom->left.colors = NULL;
	atom->matrix			= malloc(atom->map_size);
	int pixels_per_byte = 8/count_bits;				// количество пикселей описанных одним байтом
													// выравниваем данные на uint32 (количесвтво данных типа int32 в строке)
	int x_size_per_32bit = (atom->x_size * count_bits + 31) / 32;
													// временный буфер матрицы
	byte *map_buf = malloc((atom->x_size + 7) * atom->y_size );

	if(bmp->Bh.biBitCount >= 24){
		byte *ptr = data + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		for(int v = atom->y_size - 1; v >= 0; v--)
		for(int h = 0; h < atom->x_size; h++){
			RGBQUAD rq;
			if(bmp->Bh.biBitCount == 24){
				int i = (atom->y_size - v - 1) * ((atom->x_size * 3 + 3) / 4 * 4) + h * 3;
				rq.rgbRed = ptr[i+2];
				rq.rgbGreen = ptr[i+1];
				rq.rgbBlue = ptr[i];
			}
			else {int i=(atom->y_size-v-1)*(atom->x_size*4)+h*4;
				rq.rgbRed = ptr[i+2];
				rq.rgbGreen = ptr[i+1];
				rq.rgbBlue = ptr[i];
			}
         	map_buf[v * atom->x_size + h] = m_get_nearest_color_index(rq);
		}
	}
	else if(bmp->Bh.biBitCount == 8){		// определяю указатель на таблицу палитры
		RGBQUAD *color_table = (RGBQUAD *)(data	+ sizeof(BITMAPFILEHEADER)
												+ sizeof(BITMAPINFOHEADER));
											// указатель на данные
		byte *ptr = data	+ sizeof(BITMAPFILEHEADER)
      						+ sizeof(BITMAPINFOHEADER)
                            + color * sizeof(RGBQUAD);
		for(int v=atom->y_size-1; v>=0; v--)
			for(int h=0; h<atom->x_size; h++){
				int i = (atom->y_size - v - 1) * ((atom->x_size + 3) / 4 * 4) + h;
				map_buf[v * atom->x_size + h] = m_get_nearest_color_index(color_table[ptr[i]]);
			}
	}
	else if(bmp->Bh.biBitCount <= 4){
									// Заполняем матрицу
									// соотношение бит на пиксел в байте
  		byte rotsave, operand, mask = (byte)((count_bits > 1) ? 0xf : 0x1);
									// Начало массива индексов
  		byte *oper = ((byte *)data) + bmp->Fh.bfOffBits;

		for(y=0; y<atom->y_size; y++)				// строки
			for(x=0; x < x_size_per_32bit; x++)		// размер выровненный на 32 бита
				for(int l=0; l < 4 ;l++){			// байт в 32 битах
					rotsave = *oper++;
					operand = 0;
					if(count_bits > 1){
						operand = (byte)(rotsave & 0xf);
						rotsave >>= 4;
						operand <<= 4;
						operand |= (byte)(rotsave & 0xf);
					}
					else
						for(int n=0; n < 8; n++){	//переворачиваем байт
							operand <<= 1;
							if(rotsave & 0x1) operand|=0x1;
							rotsave >>= 1;
						}
					for(i=0; i < pixels_per_byte; i++){			// пикселей в байте
						pixel = (operand & mask);
						map_buf[y * atom->x_size + x * 4 * pixels_per_byte + l * pixels_per_byte + i] =
								m_get_nearest_color_index(bmp->Colors[pixel]);
						operand >>= count_bits;
					}
				}
		UpToDown=!UpToDown;
	}
										// переворачиваем, если требуется, картинку
	if(!UpToDown) memcpy(atom->matrix, map_buf, atom->map_size);
	else
		for(i=0; i<atom->y_size; i++)
			for(x=0; x<atom->x_size; x++)
				atom->matrix[(atom->y_size - i - 1) * atom->x_size + x] = map_buf[i * atom->x_size + x];
	m_free(map_buf);
  	u_digits_updates(false);			// расчитываю оцифровку
 	u_digits_change_event(true, false);
	toggle_changes(false, full);
 	return true;
}

file_buffer *m_prepare_jcr(void){
	if(!atom) return NULL;

	static file_buffer f_data;
	int j, i;
	JCR_FILE_HEADER *file;

	f_data.size=atom->x_size * atom->top_dig_size +
				atom->y_size * atom->left_dig_size + sizeof(JCR_FILE_HEADER) - 1;
	if(atom->isColor)
		f_data.size = f_data.size * 2 + atom->x_size + atom->y_size;

	f_data.data = file = (JCR_FILE_HEADER *)MALLOC_BYTE(f_data.size);

//Записываю размеры
	file->x_size		= (byte)atom->x_size;
	file->y_size		= (byte)atom->y_size;
	file->top_dig_size	= (byte)atom->top_dig_size;
	file->left_dig_size	= (byte)atom->left_dig_size;
	byte *ptr			= file->data;
	int offset			= atom->x_size * atom->top_dig_size;
	u_digits_change_event(true, false);

	if(!atom->isColor){
//Записываю оцифровку
		for(i=0; i<atom->x_size; i++)
			for(j=0; j<atom->top_dig_size; j++)
				file->data[ i * atom->top_dig_size + j] =
					atom->top.digits[i * atom->top_dig_size + j];
		for(i=0; i<atom->y_size; i++)
			for(j=0; j<atom->left_dig_size; j++)
				file->data[i * atom->left_dig_size + j + offset]=
					atom->left.digits[atom->left_dig_size * atom->y_size-
						((i + 1) * atom->left_dig_size) + j];
		}
	else {
		file->top_dig_size |= 0x80;
		file->left_dig_size|= 0x80;

	  	for(i=0; i<atom->x_size; i++){				// счетчик столбцов
         *(ptr++) = (byte)atom->top.count[i];
  			for(int j=0; j<atom->top.count[i]; j++)
        	   *(ptr++) = atom->top.digits[i * atom->top_dig_size + j];
         }
	  	for(i=0; i<atom->y_size; i++){				// счетчик строк
         *(ptr++) = (byte)atom->left.count[i];
  			for(int j=0; j<atom->left.count[i]; j++)
        	   *(ptr++) = atom->left.digits[i * atom->left_dig_size + j];
         }

// заливаем цвета оцифровки
		for(i=0; i<atom->x_size; i++)
			for(j=0; j<atom->top.count[i]; j++)
				*(ptr++) = (byte)(atom->top.colors[i * atom->top_dig_size + j] - 1);
		for(i=0; i<atom->y_size; i++)
			for(j=0; j<atom->left.count[i]; j++)
				*(ptr++) = (byte)(atom->left.colors[i * atom->left_dig_size + j] - 1);
		}
	return &f_data;
}

file_buffer *m_prepare_jcc(void){
	if(!atom) return NULL;

	static file_buffer f_data;
 	JCC_FILE_DATA *file;
	int j, i, c;
	byte color = (byte)(atom->isColor ? MAX_COLOR : 0);

	bool empty = true;
	for(i=0; i<atom->map_size; i++)	// проверяю матрицу на пустоту
		if(atom->matrix[i]!=ZERO) {empty = false; break;}

	int header_len = sizeof(JCC_FILE_DATA) - sizeof(COLORREF) + sizeof(COLORREF) * color;
	f_data.size = header_len +
				atom->x_size * atom->top_dig_size * 2 +
				atom->y_size * atom->left_dig_size * 2 +
				(empty ? 0 : atom->map_size);
	f_data.data = file = MALLOC_BYTE(f_data.size);
	JCC_DD *ptr = (JCC_DD*)((void*)file + header_len);

//Заполняю структуру
	strcpy(file->header, "jcc");
	file->x_size		= (byte)atom->x_size;
	file->y_size		= (byte)atom->y_size;
  	file->top_dig_size	= (byte)atom->top_dig_size;
	file->left_dig_size	= (byte)atom->left_dig_size;
	file->isEmpty		= (byte)empty;
	file->color			= color;
	for(i=0; i < color; i++)
		file->color_table[i] = RGBtoCOLORREF(	atom->rgb_color_table[i+2].r,
												atom->rgb_color_table[i+2].g,
												atom->rgb_color_table[i+2].b);
	u_digits_change_event(true, false);

//Записываю оцифровку
	for(j=0; j<atom->x_size; j++)
		for(i=0; i<atom->y_size; i++){// расчитываю адрес цифры
			c = atom->top_dig_size * (j + 1) - i - 1;
			ptr[i + atom->top_dig_size * j].color = atom->top.colors[c];
			ptr[i + atom->top_dig_size * j].digit = atom->top.digits[c];
		}
	ptr += (atom->x_size * atom->top_dig_size);
	for(j=0; j<atom->y_size; j++)
		for(i=0; i<atom->left_dig_size; i++){
			c = atom->left_dig_size * (j + 1) - i - 1;
			ptr[i + atom->left_dig_size * j].color = atom->left.colors[c];
			ptr[i + atom->left_dig_size * j].digit = atom->left.digits[c];
		}

// записываю матрицу
	if(!empty){
		byte *bptr = (byte *)(atom->y_size * atom->left_dig_size + ptr);
		for(i=0; i<atom->map_size; i++)
//			bptr[i] = (atom->matrix[i] == MARK) ? 0xff : atom->matrix[i];
			bptr[i] = atom->matrix[i];
	}
	return &f_data;
}

char *m_get_file_path_name(gboolean why){ //why =	true  - path
	char *ret, *path = last_opened_file;
	if(path == NULL || !strlen(path)) return NULL;

	int i, len = strlen(path);						//		false - name
	int pointer = 0;
	for(i=0; i<len; i++)
		if(path[i] == '/') pointer = i;
	if(pointer == 0){
		if(why) return NULL;
		else{
			ret=malloc(strlen(path)+1);
			strcpy(ret, path);
			return ret;
		}
	}
	if(why){	// path
		ret = malloc(pointer+1);
		path[pointer] = '\0';
		strcpy(ret, path);
		path[pointer] = '/';
	}
	else {		// name
		ret = malloc((len-pointer)*sizeof(char));
		strcpy(ret, path+pointer+1);
	}
	return ret;
}

bool m_save_puzzle_impl(){
	file_buffer *f_data = NULL;
	FILE *file;

	ftype = m_get_file_type(last_opened_file);
	if(ftype == empty || ftype == bmp) ftype=jcc;
	if(ftype == jcc)		f_data = m_prepare_jcc();
	else if(ftype == jcr)	f_data = m_prepare_jcr();
	else					f_data = l_prepare_jch();
	if(f_data){
		file = fopen(last_opened_file, "wb");
		if(!file){
			char *buf = malloc(512);
			sprintf(buf, "%s \"%s\" %s", _("Access denied!\nDon`t save"), last_opened_file, _("file."));
			d_message(_("System error"), buf, 0);
			m_free(buf);
		}
		else{
			fwrite(f_data->data, 1, f_data->size, file);
			fclose(file);
			u_clear(0);
			toggle_changes(false, full);
		}
		m_free(f_data->data);
		return true;
	}
	return false;
}

int m_changing_saved_dialog(){
	return d_message(_("Puzzle changes"), _("Save changes?"), 2);
}

int m_save_puzzle(bool ask){
	if(!change_digits && !change_matrix) return 1;
	int r;
	if(last_opened_number && place == memory){ // если это библиотечный пазл
		if(ask) r = m_changing_saved_dialog();
		else r = 1;
		if(r < 0) return r;
		else if(r > 0){
			u_clear(0);
			toggle_changes(false, full);
			return l_save_in_base();
		}
		return 0;
	}
	if(ask){
		r = m_changing_saved_dialog();
		if(r <= 0) return r;
	}
	if(ftype == jcr && change_matrix){
		do{
			r = d_message(_("Save changes"),
			_("JCR-file format preservation doesn't support changes in a matrix body!\nChoose new name?"), 1);
			if(0 == r) return r;
			if(!d_save_as_dialog()) return 0;
			ftype = m_get_file_type(last_opened_file);
		}	while(ftype != jcc && ftype != jch);
	}
	char *pth = m_get_file_path_name(true);
	if(!pth){
		if(!d_save_as_dialog()) return 0;
	}
	else m_free(pth);
	return (int)m_save_puzzle_impl();
}

// **************** функции и переменные демо-режима *******************
dir_entry **demo_ptr = NULL;

void m_demo_reset_list(){
	if(demo_ptr) m_free(demo_ptr);
	demo_ptr = NULL;
}

/*bool m_lib_error(){
	d_message(_("Library error!"), last_error, 0);
	return false;
}*/

bool m_demo_get_list_files(){
	demo_current = demo_count = 0;
	uint i, r;
	void *save;
	if(l_base_stack_top){
		m_demo_reset_list();
		demo_count = l_base_stack_top;
		demo_ptr = malloc(demo_count * sizeof(void*));
		i = 0;
		r = 1;
		uint count = 0;
		while(true){
			void *p = l_get_item_from_base(r);
			if(p != NULL && ((jch_arch_item *)p)->size > 10){
				i = 0;
				demo_ptr[count++] = p;
			}
			else i++;
			if(i > 5) break;
			r++;
			if(r > demo_count) break;
		}
		demo_count = count;
		srand(time(NULL));
		for(i=0; i<demo_count; i++){	// перетасовываю список
			r = rand()% demo_count;
			save = demo_ptr[i];
			demo_ptr[i] = demo_ptr[r];
			demo_ptr[r] = save;
		}
	}
	return true;
}

bool m_test_lib(){
	GtkItemFactory *item_factory = gtk_item_factory_from_widget(menubar);
	GtkWidget *w;
	uint i;
	if((!m_demo_get_list_files()) || demo_count == 0){
		sprintf(last_error, _("No puzzle files found in folder: %s\n%s"), puzzle_path, _("Please update library!"));
		for(i=IDM_LOAD_RANDOM; i<=IDM_HTTP_OPEN; i++){
			w=gtk_item_factory_get_item_by_action(item_factory, i);
			gtk_widget_set_sensitive(w, false);
		}
		return false; //m_lib_error();
	}
	for(i=IDM_LOAD_RANDOM; i<=IDM_HTTP_OPEN; i++){
		w=gtk_item_factory_get_item_by_action(item_factory, i);
		gtk_widget_set_sensitive(w, true);
	}
	return true;
}

bool m_demo_set_next(int delta){
	if(!l_base_stack_top) { l_get_library(true); return false; }
	last_opened_number += delta;
	if(last_opened_number >= l_base_stack_top) last_opened_number = 1;
	if(last_opened_number < 1) last_opened_number = l_base_stack_top - 1;
	place = memory;
	m_open_puzzle(false, false, l_get_item_from_base(last_opened_number)->body);
	return true;
}

bool m_demo_start(void){
	if(!m_test_lib()) return false;
	if(demo_current >= demo_count) demo_current = 0;
	demo_flag = true;
	last_opened_number = ((jch_arch_item*)demo_ptr[demo_current])->number;
	place = memory;
	m_open_puzzle(false, true, ((jch_arch_item*)demo_ptr[demo_current++])->body);
	return true;
}

void m_demo_stop(){
	if(timer_demo) g_source_remove(timer_demo);
	demo_flag = false;
	timer_demo = 0;
}

gboolean m_demo_timer_tik(gpointer i){
	if(timer_id || !demo_flag) return true;
	if(demo_current >= demo_count) demo_current = 0;
	timer_demo = 0;
	void *ptr = ((jch_arch_item*)(demo_ptr[demo_current++]))->body;
	place = memory;
	m_open_puzzle(false, true, ptr);
	return false;
}

void m_demo_next(){
	if(!demo_flag || timer_demo) return;
	timer_demo = g_timeout_add_seconds(demo_timeout, m_demo_timer_tik, NULL);
}

void m_open_url(){
	if(!last_opened_number) {d_message(_("Library error!"), _("Number is not select!"), 0); return;}
	char buf[MAX_PATH];;
	sprintf(buf, "xdg-open http://www.jcross-world.ru/#!jcrosswords/%d", last_opened_number);
	system(buf);
}
