//types.h

#ifndef _TYPES_
#define _TYPES_

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms-compat.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>

// Глобальные виджеты **************************************************
GtkWidget *window, *draw_area, *statusbar, *toolbar, *palette, *menubar, *dialog, *scroll_win;
GdkPixbuf		*sys_icon;						//системная иконка

// типы данных *********************************************************
typedef unsigned char byte;
typedef unsigned short int word;
typedef unsigned int COLORREF;
typedef enum bool_tag{false = 0, true} bool;
typedef enum f_type_tag{empty, jcr, jcc, jch, bmp, mem} f_type;		// типы файлов
typedef enum direct_tag{left, right, up, down} direct;				// направление сдвига пазла
typedef enum digits_enum_tag{left_dig, top_dig} digits_enum;		// определитель оцифровки
typedef enum ready_category_tag{bad, half, ready} ready_category;	// категория пазла |ошибочный|нелогический|решаемый|
typedef enum pr_tag{matrix,digits,full,u_empty}pr;					// перечислитель ситуаций отмен возвратов
typedef enum paint_style_tag{pen, rectangle, filling} paint_style;	// определение режимов рисования

typedef struct hint_tag{
	byte x, y, color;
} hint;

typedef struct addr_cell_tag{	// адрес ячкейки
	int x, y, count;
} addr_cell;

typedef struct RGB_tag{
	byte r,g,b;
} RGB;

typedef enum color_index_tag{
	WHITE,
	BLACK,
	BLUE,
	RED,
	MAGENTA,
	GREEN,
	CYAN,
	YELLOW,
	SILVER,
	GRAY,
	MAROON,
	NAVY,
	PURPLE,
	LIME,
	TEAL,
	OLIVE,
	AQUAMARINE,
	ORANGE,
	PINK,
	HIDE
} color_index;

typedef struct color_tag{
	double red;
	double green;
	double blue;
} color_t;

typedef struct digitable_tag{
	byte *count;				// указатель на количества цифр
	byte *digits;				// оцифровка
	byte *colors;				// цвета оцифровки
}digitable;

typedef struct cursor_tag{
	digitable *pointer;			// указатель на оцифровку
	digits_enum digits;			// определитель оцифровки
	int	indx;					// индекс в оцифровке
	int x, y;					// координаты курсора
	gboolean status;			// статус ввода включен/отключен
}cursor;

typedef struct puzzle_features_tag{
	char *header;
	char *fname;
	char *pname;
	byte top_dig_size;
	byte left_dig_size;
	byte x_size;
	byte y_size;
	bool isColor;
} puzzle_features;

// Структура пазла *****************************************************
typedef struct puzzle_tag{
	digitable	top;			// структуры указателей
	digitable	left;
	byte *matrix;				// указатель на матрицу
	bool isColor;				// цветность пазла
	size_t y_size;				// высота матрицы
	size_t x_size;				// ширина матрицы
	size_t x_puzzle_size;		// ширина пазла с оцифровкой
	size_t y_puzzle_size;		// высота пазла с оцифровкой
	size_t top_dig_size;		// высота верхней оцифровки
	size_t left_dig_size;		// ширина левой оцифровки
	size_t map_size;			// размер матрицы
	int x_begin, y_begin;		// отступ матрицы от края окна
	int x_cell, y_cell;			// адрес выбранной ячейки
	int top_col, left_row;		// столбец и строка подсветки оцифровки
	cursor *curs;				// текущая позиция курсора ввода оцифровки
	hint *hints;				// буфер подсказок для нерешаемых пазлов(не более MAX_HINTS)
	byte c_hints;
	bool digits_status;
	RGB rgb_color_table[20];	// палитра
} puzzle;

// флаги процедуры проверки строки
typedef struct c_test_line_res_tag{
	bool	complete,			// строка решена
			digits,				// изменения в оцифровке
			matrix,				// изменения в матрице
			legal;				// строка не имеет ошибок
} c_test_line_res;


// Глобальные переменные ***********************************************
f_type ftype;					// тип файла пазла
const char *home_dir;			// путь к домашней папке
char *config_path;				// путь к конфиг папке
char *config_file;				// путь к конфиг файлу
char *last_opened_file;			// актуальный файл пазла
int last_opened_number;			// номер в БД последнего открытого пазла
char *puzzle_path;				// путь к каталогу библиотеки
char *db_path;					// путь к базе данных
char *puzzle_name;				// название кроссворда
char *last_error;				// стринг последней ошибки
bool *parameter;				// параметры проги
bool demo_flag;					// флаг-признак демо-режима
bool change_matrix, change_digits;	// флаги изменений в файле
byte current_color;				// текущий цвет
byte cell_space;				// размер ячейки
uint timer_id;					// таймер визуализации решения
uint timer_demo;				// таймер демо-режима
paint_style ps;					// стиль рисования pen-rectangle-filling
byte help_level;				// уровень помощи при решении

// Макросы *************************************************************
#define ERR d_message("Test","Control point!",0);
#define error(a,b) fprintf(stderr,"Error %d: %s\n",(a),(b));
#define mess(a) fprintf(stderr,(a))	// отладочная инфа
#ifdef DEBUG
#define c_error fprintf(stderr, "%s\n", last_error)
#define LOCALEDIR "locale"
#else
#define c_error ;
#endif
#define LOCALEDIR "/usr/share/locale"
#define _(x) gettext(x)

#define MALLOC_BYTE(a) memset(g_malloc((a)*sizeof(byte)+1),0,(a)*sizeof(byte)+1)
#define COPY(a,b,c) {if(a)g_free(a);(a)=g_malloc((c)*sizeof(byte));memcpy((a),(b),(c))}
#define m_free(a) {if(a)free(a);(a)=NULL;}
#define CAIRO_INIT if(cr)cairo_destroy(cr);cr=gdk_cairo_create(draw_area->window)

#define RGBtoCOLORREF(r,g,b) ((COLORREF)(((uint)(b))<<16|((uint)(g))<<8|(uint)(r)))
#define GetRValue(a) ((byte)((a)&0xff))
#define GetGValue(a) ((byte)(((a)>>8)&0xff))
#define GetBValue(a) ((byte)(((a)>>16)&0xff))
#define SET_COLOR(a) cairo_set_source_rgb(cr,\
			((float)atom->rgb_color_table[(a)].r/255),\
			((float)atom->rgb_color_table[(a)].g/255),\
			((float)atom->rgb_color_table[(a)].b/255))
#define SET_SYS_COLOR(a) cairo_set_source_rgb(cr,\
			((float)color_table[(a)].r/255),\
			((float)color_table[(a)].g/255),\
			((float)color_table[(a)].b/255))

// Константы ***********************************************************
#define CLASS_NAME "Matrix"
#define EXEFILENAME "matrix"
#define PACKAGE EXEFILENAME
#define CAPTION_TEXT _("Nanogramm puzzles")
#define NEW_FILE_NAME "new_puzzle.jcc"
#define CONFIG_PATH "/.config/matrix"
#define CONFIG_NAME "/matrix.cfg"
#define LOCK_FILE "/matrix.run"
#define SHARE_PATH "/usr/share/matrix/res"
#define EXAMPLE_PATH "/usr/share/matrix/example"
#define ICON_PATH "/usr/share/matrix/res/matrix.png"
#define WORK_NAME _("nanogramms")

#define VERSION	"0.7"
#define MAX_PATH 2048

#define DEF_CELL_SPACE	20	// размер ячйки
#define WINDOW_SIZE_X	512
#define WINDOW_SIZE_Y	384
#define ICON_SIZE		24
#define BORDER_WIDTH		2
#define MAX_MATRIX_SIZE 127		// максимальный размер поля матрицы(без оцифровки)
#define MAX_HINTS		5		// максимальное количество сохраняемых ячеек-подсказок
#define BACKGROUND		WHITE	// цвет заднего плана
#define LINES			SILVER	// цвет сетки
#define CROSS			GRAY	// цвет крестиков маркировки
#define BOLD			GRAY	// цвет утолщенной линии
#define MARKER			RED	// цвет подсветки курсора оцифровки
#define MARK_ER			BLACK	// цвет маркера при ошибке
#define ERROR_DAGGER 	BLUE	// цвет крестика ошибки
#define COLOR_MARK		0x80	// маска маркера
#define COL_MARK_MASK	0x7f	// маска выделения цвета из байта
#define HINT_MARK		17		// определение маркера для сохранений подсказок(hints)
#define MAX_COLOR		18
#define ZERO			0
#define EMPTY			0
#define MARK			0xff
#define BACK			0
#define BLACK			1
#define ERRM 			4	// размер точки-ошибки

#define demo_timeout	5		// таймаут для демо (сек)
#define timeout			15		// таймаут анимации (мсек)

static const RGB color_table[] = {
	{0xff,0xff,0xff},	//WHITE		0
	{   0,   0,   0},	//BLACK		1
	{   0,   0,0xff},	//BLUE		2
	{0xff,   0,   0},	//RED		3
	{0xff,   0,0xff},	//MAGENTA	4
	{   0,0xb5,   0},	//GREEN		5
	{   0,0xff,0xff},	//CYAN		6
	{0xff,0xff,   0},	//YELLOW	7
	{0xc0,0xc0,0xc0},	//SILVER	8
	{0x80,0x80,0x80},	//GRAY		9
	{0xb5,   0,   0},	//MAROON	10
	{   0,   0,0xb5},	//NAVY		11
	{0xb5,   0,0xb5},	//PURPLE	12
	{   0,0xff,   0},	//LIME		13
	{   0,0xb5,0xb5},	//TEAL		14
	{0x80,0x80,   0},	//OLIVE		15
	{0x7f,0xff,0xd4},	//AQUAMARINE16
	{0xff,0xa5,   0},	//ORANGE	17
	{0xff,0xc0,0xcb},	//PINK		18
	{0xf0,0xf0,0xf0}};	//HIDE		19
#endif //_TYPES_
