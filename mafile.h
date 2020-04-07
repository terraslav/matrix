// mafile.h - работа с файлами
#ifndef _MAFILE_
#define _MAFILE_
#include <stdint.h>
#include "puzzle.h"

typedef struct file_buffer_tag{
	long size;	// размер буфера
	void *data;
}file_buffer;

typedef struct JCR_FILE_HEADERtag {
	byte x_size;
	byte y_size;
	byte left_dig_size;
	byte top_dig_size;
	byte data[1];
}__attribute__((__packed__ )) JCR_FILE_HEADER;

typedef struct JCC_DD_tag {
	byte digit;
	byte color;
}__attribute__((__packed__ )) JCC_DD;

typedef struct JCC_FILE_DATA_tag {	// описатель заголовка нового формата JCC
	char header[3];					//jcc
	byte x_size;					//ширина матрицы
	byte y_size;					//высота матрицы
	byte left_dig_size;				//разрядность левой оцифровки
	byte top_dig_size;				//разрядность верхней оцифровки
	byte isEmpty;					//1 Matrix is empty
	byte color;   					//2..MAX_COLOR
	COLORREF color_table[1];		//MAX_COLOR colors
}__attribute__((__packed__ )) JCC_FILE_DATA;

typedef struct JCC_FILE_DATA_OLD_tag {// описатель заголовка устаревшего формата JCC
	char header[5];					//JAPAN
	byte color;   					//2..16
	COLORREF color_table[16];		//16 colors
	byte x_size;					//ширина матрицы
	byte y_size;					//высота матрицы
	byte left_dig_size;				//разрядность левой оцифровки
	byte top_dig_size;				//разрядность верхней оцифровки
	byte isEmpty;					//1 Matrix is empty
	JCC_DD data[1];					//Pointer to digit
}__attribute__((__packed__ )) JCC_FILE_DATA_OLD;

typedef  enum{
	BI_RGB = 0x0000,
	BI_RLE8 = 0x0001,
	BI_RLE4 = 0x0002,
	BI_BITFIELDS = 0x0003,
	BI_JPEG = 0x0004,
	BI_PNG = 0x0005,
	BI_CMYK = 0x000B,
	BI_CMYKRLE8 = 0x000C,
	BI_CMYKRLE4 = 0x000D
} Compression;

typedef struct RGBQUAD_tag {
	byte   rgbBlue;
	byte   rgbGreen;
	byte   rgbRed;
	byte   rgbReserved;
}__attribute__((__packed__ )) RGBQUAD;

typedef struct BITMAPFILEHEADER_tag {//	Смещение	Длина поля	Описание
	uint16_t    bfType;				//	0			2			Код 4D42h - Буквы 'BM'
	uint32_t   bfSize;				//	2			4			Размер файла в байтах
	uint16_t    bfReserved1;		//	6			2			0 (Резервное поле)
	uint16_t    bfReserved2;		//	8			2			0 (Резервное поле)
	uint32_t   bfOffBits;			//	10			4			Смещение, с которого начинается само изображение(растр).
}__attribute__((__packed__ )) BITMAPFILEHEADER;

typedef struct BITMAPINFOHEADER_tag {//	Смещение	Длина поля	Описание
	uint32_t biSize;				//	14			4			Размер заголовка BITMAP (в байтах) равно 40
	long  biWidth;					//	18			4			Ширина изображения в пикселях
	long  biHeight;					//	22			4			Высота изображения в пикселях
	uint16_t  biPlanes;				//	26			2			Число плоскостей, должно быть 1
	uint16_t  biBitCount;			//	28			2			Бит/пиксел: 1, 4, 8 или 24
	uint32_t biCompression;			//	30			4			Тип сжатия
	uint32_t biSizeImage;			//	34			4			0 или размер сжатого изображения в байтах.
	long  biXPelsPerMeter;			//	38			4			Горизонтальное разрешение, пиксел/м
	long  biYPelsPerMeter;			//	42			4			Вертикальное разрешение, пиксел/м
	uint32_t biClrUsed;				//	46			4			Количество используемых цветов
	uint32_t biClrImportant;		//	50			4			Количество "важных" цветов.
}__attribute__((__packed__ )) BITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct FILEBITMAPINFO_tag{
	BITMAPFILEHEADER Fh;
	BITMAPINFOHEADER Bh;
	RGBQUAD Colors[1];
}__attribute__((__packed__ )) FILEBITMAPINFO, *LPFILEBITMAPINFO;

typedef enum place_data_tag{memory, exfile}place_data;

// **************** функции и переменные демо-режима *******************
typedef struct dir_entry_tag{
	char *file_name;
	void *prev_entry;
	uint number;
}dir_entry;

int demo_count, demo_current;
place_data place;	// расположение данных пазла (память - файл)


bool m_open_puzzle			(bool, bool, char*);
bool m_get_puzzle_default	(void);
int  m_save_puzzle			(bool);
bool m_save_puzzle_impl		(void);
bool m_new_puzzle			(void);
bool m_demo_open_folder		(void);
bool m_demo_start			(void);
void m_demo_reset_list		(void);
void m_demo_stop			(void);
void m_demo_next			(void);
void m_enable_items			(void);
gboolean m_demo_timer_tik	(gpointer);
bool m_demo_get_list_files	(void);
bool m_demo_set_next	(int);
bool m_demo_set_prev	();
char *m_get_file_path_name(gboolean);
void m_set_title			(void);
void m_open_url				(void);
bool m_test_lib				(void);
f_type m_get_file_type		(char*);
uint m_get_base_number_file	(char*);

#endif //_MAFILE_
