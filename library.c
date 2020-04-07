// library.с - работа с библиотекой кроссвордов
#include "library.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h> //hostent
#include <arpa/inet.h>
#include <unistd.h>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define buflen 25000

char *cl = "Content-Length: ";
char *host = "www.jcross-world.ru";
char *host_path = "/api/users/0/jcrosswords";
//static char rnda, rndb, rndc;
byte *table = NULL;							// таблица преобразования
const int table_size = 256;					// размерность таблицы

uint l_atoi(char *p){
	uint akk = 0;
	while(*p>='0' && *p<='9'){
		akk *=10;
		akk += *p++ - '0';
	}
	return akk;
}
/*
void l_srand(char *key){					// инициализация псевдослучайной последовательности
	rnda = rndb = rndc = 11;
	for(int i=0; i<strlen(key); i++){
		rnda -= key[i]; rndb ^= key[i]; rndc *= key[i];
	}
}

byte l_rand(){								// псевдослучайное число
	byte temp = rnda; rnda = rndb; rndb += rndc; rndc = temp;
	rndc >>= 1; rndc ^= 0xff; temp+=rndb; rnda+=temp; return temp;
}

void *h_rest(char *fname){
	uint i, x, y, lptr, sptr, len;
	long result;
	byte tmpc, *key, *row;
	FILE *file;
	char *pass = "4868";
	long fsize;
	file = fopen(fname, "rb");				// открываю файл
	if(file == NULL){
		fputs ("Error opening file data", stderr); exit (1);
	}
	fseek(file, 0, SEEK_END); 				// определяю длину файла
	fsize = ftell(file);
	rewind (file);

	byte *buf = malloc(fsize+1);			// читаю файл в буфер
	if (buf == NULL) {fputs ("Memory error",stderr); exit (2);}

	result = fread(buf, 1, fsize, file);
	if (result != fsize) {fputs ("Reading error",stderr); exit (3);}
	fclose (file);
	buf[fsize]='\0';

	l_srand(pass);							// инициализация случайных чисел

	key = malloc(fsize+1);
	if (key == NULL) {fputs ("Memory error",stderr); exit (2);}

	lptr=0; sptr=0;	len = strlen((char*)pass);
	while(lptr < fsize){					// формирую крипт-буфер из пароля
		key[lptr++] = pass[sptr++];			// повторяя пароль всю длину буфера
		if(sptr >= len) sptr=0;				// равную размеру файла
	}
	key[fsize]=0;
	for(i=0; i<fsize; i++)					// чуток энтропии
		key[i] ^= l_rand();
											// создаю крипт-таблицу
	table = malloc(sizeof(char)*(table_size*table_size));

	for(y=0; y<table_size; y++){			// построчный цикл
		row = table + y * table_size;
		for(x=0; x<table_size; x++) 		// записываю строку
			row[x] = x;
		for(x=0; x<table_size; x++){		// энропия
			i = (int)l_rand();
			tmpc = row[x];
			row[x] = row[i];
			row[i] = tmpc;
		}}


	for(i=0; i<fsize; i++){					// восстанавливаю буфер
		row = table + (key[i] * table_size);
		for(x=0; x<table_size; x++)
			if(row[x] == buf[i]){
				buf[i]=(char)x;
				break;
			}
	}
	for(i=0; i<fsize; i++)
		buf[i] ^= l_rand();
	free(table); free(key);
	return buf;
}
*/
void l_free_buf(l_item* s){
	ig_ptr = s;
	while(ig_ptr){
		l_item* next = ig_ptr->next;
		m_free(ig_ptr->name);
		m_free(ig_ptr->parameter);
		m_free(ig_ptr);
		ig_ptr = next;
	}
	if(s == ig_start){
		l_jch_container = false;
		ig_start = ig_ptr = NULL;
	}
}

void l_close_jch(){
	l_free_buf(ig_start);
}

uint l_get_unit_len(char *p){
	size_t b = 0, c = 0;
	while(p[c]){
		if(p[c] == '{')		b++;
		else if(p[c] == '}')b--;
		if(!b) return c + 1;
		c++;
	}
	return 0;
}

uint l_strlen(char *p){
	uint c = 0;
	bool ignor = true;
	if(p[0] == '"') {c++; ignor = false;}
	while(p[c] != '"' && p[c]){
		if(p[c] == '{') return c + l_get_unit_len(p + c);
		if(ignor && (p[c] == ',' || p[c]=='}')) break; else c++;
	}
	if(p[c] == '"') c++;
	return c;
}

l_item* l_find_item(char *n){
	ig_ptr = ig_start;
	while(ig_ptr){
		if(!strcmp(ig_ptr->name, n)) break;
		ig_ptr = ig_ptr->next;
	}
	return ig_ptr;
}

char *l_get_name(){
	if(!l_jch_container) return NULL;
	l_item* p = l_find_item("\"name\"");
	return p->parameter;
}

void l_set_name(){
	if(!l_jch_container) return;
	l_item* p = l_find_item("\"name\"");
	m_free(p->parameter);
	p->parameter = malloc(strlen(puzzle_name)+1);
	strcpy(p->parameter, puzzle_name);
}

bool l_get_items(void *file_buf){
	char *p = (char *)file_buf;
	int i;
	uint l;
	bool nam_par = false;
	while(*p){
		switch (*p){
		case '{':
			ig_start = ig_ptr = MALLOC_BYTE(sizeof(l_item));
			ig_ptr->name = malloc(10);
			ig_ptr->parameter = NULL;
			ig_ptr->next = NULL;
			strcpy(ig_ptr->name, "general");
			break;
		case '"':
		case ':':
			if(!nam_par){		// если новый параметр выделяю для него хедер
				ig_ptr->next = malloc(sizeof(l_item));
				ig_ptr = ig_ptr->next;
				ig_ptr->next = NULL;
			}
			else{				// иначе поиск разделителя :
				while(*p && *p != ':') p++;
				if(*p != ':'){
					l_free_buf(ig_start);
					return false;
				}
				else p++;
			}
			l = l_strlen(p);
			char *s = MALLOC_BYTE(l + 1);
			for(i=0; i<l; i++)
				s[i] = *p++;
			if(!nam_par)	ig_ptr->name = s;
			else			ig_ptr->parameter = s;
			nam_par = !nam_par;
			continue;
		case '}':
			return true;
		case ',':
			nam_par = false;
			break;
		default:
			break;
		}
		p++;
	}
	return false;
}

byte l_get_hex(char *p){
	if(!*p || *p==',' || *p=='|') return 0;
	byte akk, a = *p++ | 0x20;
	if(a >= '0' && a <= '9')		akk = a-'0';
	else if(a >= 'a' && a <= 'f')	akk = a-'a'+10;
	else return 0;
	if(!*p || *p==',' || *p=='|') return akk;
	akk <<= 4;
	a = *p | 0x20;
	if(a >= '0' && a <= '9')		akk |= a-'0';
	else if(a >= 'a' && a <= 'f')	akk |= a-'a'+10;
	return akk;
}

int l_get_dec(char *p, byte num){
	if(num)	while(*p++){
		if(*p == ',') num--;
		else if(*p == '|') return -1;
		if(!num) {p++; break;}
	}
b:
	if(!*p || *p=='|') return -1;
	if(*p==',') {p++; goto b;}
	byte akk, a = *p++ | 0x20;
	if(a >= '0' && a <= '9')		akk = a-'0';
	else return -1;
	if(!*p || *p==',' || *p=='|') return (int)akk;
	akk *= 10;
	a = *p | 0x20;
	if(a >= '0' && a <= '9')		akk += a-'0';
	return (int)akk;
}

size_t l_count_param(char *p){
	size_t c=0;
	while(*p) if(*p++ == ',') c++;
	return ++c;
}

typedef struct CLR_tag{
	char r[2], g[2], b[2]; byte end;
}CLR;

void l_parse_jch(char *p){
	uint c=0, d=0, i, x, y;
	byte black = 0xff;
	while(p[d]) if(p[d++] == '#') c++;
	size_t total_count = ++c;
	char **item = MALLOC_BYTE(c * sizeof(char*));
	size_t *count = MALLOC_BYTE(c * sizeof(size_t));
	c=0, d=0;
	while(p[++d]){	// досчет длинны строк содержимого
		if(p[d] == '#')	c++;
		else			count[c]++;
	}
	c++;
	for(i=0; i<c; i++)	// выделяю под строки буфера
		item[i] = MALLOC_BYTE(count[i]+1);
	d=0; c=0;
	while(*p){
		if(*p == '#')  {c++; d=0; p++;}
		else 			item[c][d++] = *p++;
	}
	c++;

// размеры: высота, ширина, ширина левой оцифровки, высота верхней оцифр.
// цвета RGB
	current_color = BLACK;
//	toggle_changes(false, false);		// сброс флага изменений в пазле
	if(atom){
		u_clear(0);				// стираю отмены
		c_solve_destroy();		// зачищаю буфера решения
		c_free_puzzle_buf(atom);
		m_free(atom);
	}
	size_t colors = l_count_param(item[2]);
 	atom = MALLOC_BYTE(sizeof(puzzle));	// создаю пазл
 	atom->x_size	= (byte)l_get_dec(item[1], 1);
  	atom->y_size	= (byte)l_get_dec(item[1], 0);
	atom->top_dig_size	= l_get_dec(item[1], 3);
	atom->left_dig_size	= l_get_dec(item[1], 2);
	atom->isColor		= (colors > 1);
	atom->map_size		= atom->x_size * atom->y_size;
	atom->x_puzzle_size	= atom->x_size + atom->left_dig_size;
	atom->y_puzzle_size	= atom->y_size + atom->top_dig_size;
	c_malloc_puzzle_buf(atom);

// копирую цвета
	if(colors > 1){
		CLR *a = (CLR*)item[2];
		RGB v;
		byte ptr = 2;
		for(byte i=0; i<colors; i++){
			v.r=l_get_hex(a->r);
			v.g=l_get_hex(a->g);
			v.b=l_get_hex(a->b);
			if(!v.r && !v.g && !v.b)
				black = i;
			else atom->rgb_color_table[ptr++] = v;
			a++;
		}
	}

// копирую оцифровку слева - направо сверху вниз
	p = item[3];									// левую слева - направо
	for(y=0; y<atom->y_size; y++){
		for(x=0; x<atom->left_dig_size; x++){
			i = (y+1) * atom->left_dig_size - x - 1;
			int m = l_get_dec(p, x);
			if(m < 0) break;
			atom->left.digits[i] = (byte)m;
		}
		while(*p && *p++ != '|');
		if(!*p) break;
	}
	p = item[4];									// верхнюю сверху - вниз
	for(x=0; x<atom->x_size; x++){
		for(y=0; y<atom->top_dig_size; y++){
			i = (x+1) * atom->top_dig_size - y - 1;
			int m = l_get_dec(p, y);
			if(m < 0) break;
			atom->top.digits[i] = (byte)m;
		}
		while(*p && *p++ != '|');
		if(!*p) break;
	}
	if(colors > 1){
		p = item[5];									// цвета левой оцифровки
		for(y=0; y<atom->y_size; y++){
			for(x=0; x<atom->left_dig_size; x++){
				i = (y+1) * atom->left_dig_size - x - 1;
				int m = l_get_dec(p, x);
				if(m < 0) break;
				if(black == 0xff) m+=2;
				else {
					if(m == black) m=1;
					else if(m<black) m+=2;
					else m++;
				}
				atom->left.colors[i] = (byte)m;
			}
			while(*p && *p++ != '|');
			if(!*p) break;
		}
		p = item[6];									// цвета верхней оцифровки
		for(x=0; x<atom->x_size; x++){
			for(y=0; y<atom->top_dig_size; y++){
				i = (x+1)*atom->top_dig_size-y-1;
				int m = l_get_dec(p, y);
				if(m < 0) break;
				if(black == 0xff) m+=2;
				else {
					if(m == black) m=1;
					else if(m<black) m+=2;
					else m++;
				}
				atom->top.colors[i] = (byte)m;
			}
			while(*p && *p++ != '|');
			if(!*p) break;
		}
	}
	for(i=0; i<total_count; i++)					// освобождаю буфера
		m_free(item[i]);
	m_free(item);
}

char *l_digs_to_string(byte *s, size_t c){
	char r[4]; r[3]='\0';
	byte a;
	uint d;
	char *buf = MALLOC_BYTE(MAX_PATH);
	for(uint i=0; i<c; i++){
		a = s[i];
		for(d=2; d>=0; d--){
			r[d] = (a % 10) + '0';
			a /= 10;
			if(!a) break;
		}
		strcat(buf, r+d);
		if((i+1) < c) strcat(buf, ",");
	}
	return buf;
}

char *l_dig_to_string(uint a){
	char r[21];
	uint i;
	memset(r, 0, 21);
	for(i=19; i>=0; i--){
		r[i] = (a % 10) + '0';
		a /= 10;
		if(!a) break;
	}
	char *res = MALLOC_BYTE(21 - i);
	strcpy(res, r+i);
	return res;
}

char *l_get_color_string(byte color){
	byte	r = atom->rgb_color_table[color].r,
			g = atom->rgb_color_table[color].g,
			b = atom->rgb_color_table[color].b;
	char *res = MALLOC_BYTE(7);
	res[0] = r>>4; if(res[0]>9) res[0]+='a'-10; else res[0]+='0';
	res[1] = r&0xf;if(res[1]>9) res[1]+='a'-10; else res[1]+='0';
	res[2] = g>>4; if(res[2]>9) res[2]+='a'-10; else res[2]+='0';
	res[3] = g&0xf;if(res[3]>9) res[3]+='a'-10; else res[3]+='0';
	res[4] = b>>4; if(res[4]>9) res[4]+='a'-10; else res[4]+='0';
	res[5] = b&0xf;if(res[5]>9) res[5]+='a'-10; else res[5]+='0';
	return res;
}

char *l_set_left_digit_string(uint y){
	byte *line = atom->left.digits + y*atom->left_dig_size;
	char *res = MALLOC_BYTE(atom->left.count[y]*3);
	for(int i=atom->left.count[y]-1; i>=0; i--){
		char *c = l_dig_to_string(line[i]);
		strcat(res, c);
		if(i>0) strcat(res,",");
		m_free(c);
	}
	strcat(res,"|");
	return res;
}

char *l_set_top_digit_string(uint x){
	byte *line = atom->top.digits + x*atom->top_dig_size;
	char *res = MALLOC_BYTE(atom->top.count[x]*3);
	for(int i=atom->top.count[x]-1; i>=0; i--){
		char *c = l_dig_to_string(line[i]);
		strcat(res, c);
		if(i>0) strcat(res,",");
		m_free(c);
	}
	strcat(res,"|");
	return res;
}

uint color_counts[MAX_COLOR];

char *l_set_left_color_string(uint y){
	byte *line = atom->left.colors + y*atom->left_dig_size;
	char *res = MALLOC_BYTE(atom->left.count[y]*3);
	for(int i=atom->left.count[y]-1; i>=0; i--){
		if(atom->isColor){
			byte d = line[i]&COL_MARK_MASK;
			d = color_counts[d];
			char *c = l_dig_to_string(d);
			strcat(res, c);
			m_free(c);
		}
		else strcat(res, "0");
		if(i>0) strcat(res,",");
	}
	strcat(res,"|");
	return res;
}

char *l_set_top_color_string(uint x){
	byte *line = atom->top.colors + x*atom->top_dig_size;
	char *res = MALLOC_BYTE(atom->top.count[x]*3);
	for(int i=atom->top.count[x]-1; i>=0; i--){
		if(atom->isColor){
			byte d = line[i]&COL_MARK_MASK;
			d = color_counts[d];
			char *c = l_dig_to_string(d);
			strcat(res, c);
			m_free(c);
		}
		else strcat(res, "0");
		if(i>0) strcat(res,",");
	}
	strcat(res,"|");
	return res;
}

void *l_set_content(){
	uint x, y;
	char *buf = MALLOC_BYTE(123 +(atom->top_dig_size*2) *atom->x_size*2 +
									(atom->left_dig_size*2)*atom->y_size*2);
	strcpy(buf, "\"v2#");
	byte p[4];
	p[0] = atom->y_size; p[1] = atom->x_size;
	p[2] = atom->left_dig_size; p[3] = atom->top_dig_size;
	char *tmp = l_digs_to_string(p, 4);
	strcat(buf, tmp);
	m_free(tmp);
	strcat(buf,"#000000");
	if(atom->isColor){	// подсчитываю используемые цвета
		memset(color_counts, 0, 18*sizeof(uint));
		for(x=0; x<atom->x_size; x++)
			for(y=0; y<atom->top_dig_size; y++){
				byte c = atom->top.colors[x*atom->top_dig_size+y];
				c &= COL_MARK_MASK;
				if(c) color_counts[c]++;
			}
		uint use_colors[MAX_COLOR];
		size_t ptr = 0;
		color_counts[1] = 0;
		for(x=2; x<MAX_COLOR; x++)
			if(color_counts[x]){
				use_colors[ptr++] = x;
				color_counts[x] = ptr;
			}
		for(x=0; x<ptr; x++){
			strcat(buf, ",");
			char *c = l_get_color_string(use_colors[x]);
			strcat(buf, c);
			m_free(c);
		}
	}
	strcat(buf,"#");
	for(y=0; y<atom->y_size; y++){
		char *c = l_set_left_digit_string(y);
		strcat(buf, c);
		m_free(c);
	}
	strcat(buf,"#");
	for(x=0; x<atom->x_size; x++){
		char *c = l_set_top_digit_string(x);
		strcat(buf, c);
		m_free(c);
	}
	strcat(buf,"#");
	for(y=0; y<atom->y_size; y++){
		char *c = l_set_left_color_string(y);
		strcat(buf, c);
		m_free(c);
	}
	strcat(buf,"#");
	for(x=0; x<atom->x_size; x++){
		char *c = l_set_top_color_string(x);
		strcat(buf, c);
		m_free(c);
	}
	strcat(buf,"\"");
	return buf;
}

char *l_get_time_string(){
	time_t rawtime;
	struct tm * timeptr;
	char *buf = malloc(80);
	time (&rawtime);
	timeptr = localtime(&rawtime);
	sprintf(buf, "\"%d-%.2d-%.2dT%.2d:%.2d:%.2d.000Z\"",
		1900 + timeptr->tm_year, timeptr->tm_mon, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);
	return buf;
}

void l_get_hints(){
	if(!atom) return;
	atom->c_hints = 0;
	l_item *item = l_find_item("\"hints\"");
	if(!item || !item->parameter || !strlen(item->parameter)) return;
	char *u = item->parameter, *max = u+strlen(u)-4;
	uint count = 0;
	while(u < max){
		atom->hints[count].x = (byte)l_atoi(u);
		while (*u >= '0' && *u <= '9'){ u++;}; u++;
		atom->hints[count].y = (byte)l_atoi(u);
		while (*u >= '0' && *u <= '9'){ u++;}; u++;
		byte c = (byte)l_atoi(u);
		c = c == HINT_MARK ? MARK : c + 1;
		atom->hints[count++].color = c;
		while (*u >= '0' && *u <= '9'){ u++;}; u++;
	}
	atom->c_hints = count;
}

void l_get_save(){
	if(!atom) return;
	l_item *item = l_find_item("\"save\"");
	if(!item || !item->parameter || !strlen(item->parameter)) return;
	char *u = item->parameter, *max = u+strlen(u);
	byte a, last=0;
	uint c, cnt = 0;
	while(u < max){
		a = *u++;
		if(a == 'i')				last = MARK;
		else if(a>='0' && a<='9')	last = a - '0';
		else if(a>='a' && a<='f')	last = a - 'a' + 10;
		else {
			c = 0;
			while(a == 'Z'){
				c += 26;
				if(*u >= 'A' && *u <= 'Z') a = *u++;
				else a = 'A' - 1;
				}
			c += a - 'A';
			while(c--) atom->matrix[cnt++] = last;
		}
		atom->matrix[cnt++] = last;
	}
}

void l_set_save(){
	char *s = "\"save\"";
	l_item *ig = l_find_item(s);
	uint cnt = 0;
	for(uint i=0; i<atom->map_size; i++)
		if(atom->matrix[i]!=EMPTY)
			cnt++;
	if(!cnt && ig){	m_free(ig->parameter);}		// удаляю предыдущее сохранение
	if(cnt<=MAX_HINTS) return;

	if(!ig){
		ig = ig_start;
		while(ig->next) ig = ig->next;
		ig->next = MALLOC_BYTE(sizeof(l_item));
		ig = ig->next;
		ig->name = malloc(strlen(s)); strcpy(ig->name, s);
	}
	m_free(ig->parameter);
	char *buf = MALLOC_BYTE(atom->map_size);
	uint bp = 0; cnt = 0;
	byte last=0x80;
	for(int i=0; i<atom->map_size; i++){
		byte a = atom->matrix[i];
		if(a == MARK) a = 'i';
		if(a!=last){
			if(cnt > 1){
				while(cnt > 27){	// 'A' + количество повторений - 1
					buf[bp++] = 'Z';
					cnt -= 26;		// 26 - количество букав латиницы
				}
				buf[bp++] = cnt - 2 + 'A';
			}
			cnt = 1;
			last = a;
			if(a != 'i') a = a<10 ? a+'0' : a-10+'a';
			buf[bp++] = a;
		}
		else cnt++;
	}
	if(cnt > 1){ // подбиваю до конца матрицы
		while(cnt > 27){ buf[bp++] = 'Z'; cnt -= 26;}
		buf[bp++] = cnt - 2 + 'A';
	}
	ig->parameter = malloc(strlen(buf)+1);
	strcpy(ig->parameter, buf);
	free(buf);
}

void l_set_hints(){
	char *s = "\"hints\"";
	l_item *ig = l_find_item(s), *g;
	uint cnt = 0;
	for(uint i=0; i<atom->map_size; i++)
		if(atom->matrix[i]!=EMPTY)
			cnt++;
	if(cnt > MAX_HINTS) return;
	if(!cnt){
//		if(ig){ m_free(ig->parameter);}		// удаление подсказок при пустой матрице
		return;
	}
	g = l_find_item("\"save\"");			// удаляю сохранение
	if(g){ m_free(g->parameter);}

	if(!ig){
		ig = ig_start;
		while(ig->next) ig = ig->next;
		ig->next = MALLOC_BYTE(sizeof(l_item));
		ig = ig->next;
		ig->name = malloc(strlen(s)); strcpy(ig->name, s);
	}
	m_free(ig->parameter);
	char *ptr = ig->parameter = MALLOC_BYTE(cnt*11);
	char *sx, *sy, *sc;
	cnt = 0;
	for(uint p=0; cnt<MAX_HINTS && p < atom->map_size; p++){
		byte a = atom->matrix[p];
		if(a == EMPTY) continue;
		sx = l_dig_to_string(p % atom->x_size);
		sy = l_dig_to_string(p / atom->x_size);
		sc = l_dig_to_string(a==MARK ? HINT_MARK : a-1);
		sprintf(ptr, "%s.%s.%s|", sx, sy, sc);
		ptr = ig->parameter + strlen(ig->parameter);
		m_free(sx); m_free(sy); m_free(sc);
		cnt++;
	}
	l_get_hints();// сохранение hints в структуре atom
}

//#define SHORT_JCH
void l_create_jch_content(){
	char *n[] = {"\"id\"","\"name\"","\"content\"","\"rows\"","\"cols\"","\"difficult\"","\"createdAt\""};
	l_free_buf(ig_start);
	int ptr=0;
	ig_start = ig_ptr = malloc(sizeof(l_item));
	ig_ptr->name = malloc(strlen(n[ptr])+1); strcpy(ig_ptr->name, n[ptr++]);
	ig_ptr->parameter = l_dig_to_string(time(NULL));
	// name
	ig_ptr->next = malloc(sizeof(l_item));
	ig_ptr = ig_ptr->next;
	ig_ptr->name = malloc(strlen(n[ptr])+1); strcpy(ig_ptr->name, n[ptr++]);
	ig_ptr->parameter = malloc(strlen(puzzle_name)+1);
	strcpy(ig_ptr->parameter, puzzle_name);
	// content
	ig_ptr->next = malloc(sizeof(l_item));
	ig_ptr = ig_ptr->next;
	ig_ptr->name = malloc(strlen(n[ptr])+1); strcpy(ig_ptr->name, n[ptr++]);
	ig_ptr->parameter = l_set_content();
#ifdef SHORT_JCH
	// rows
	ig_ptr->next = malloc(sizeof(l_item));
	ig_ptr = ig_ptr->next;
	ig_ptr->name = malloc(strlen(n[ptr])+1); strcpy(ig_ptr->name, n[ptr++]);
	ig_ptr->parameter = l_dig_to_string(atom->y_size);
	// cols
	ig_ptr->next = malloc(sizeof(l_item));
	ig_ptr = ig_ptr->next;
	ig_ptr->name = malloc(strlen(n[ptr])+1); strcpy(ig_ptr->name, n[ptr++]);
	ig_ptr->parameter = l_dig_to_string(atom->x_size);
	// difficult
	ig_ptr->next = malloc(sizeof(l_item));
	ig_ptr = ig_ptr->next;
	ig_ptr->name = malloc(strlen(n[ptr])+1); strcpy(ig_ptr->name, n[ptr++]);
	ig_ptr->parameter = l_dig_to_string(stat_difficult);
	// createdAt
	ig_ptr->next = malloc(sizeof(l_item));
	ig_ptr = ig_ptr->next;
	ig_ptr->name = malloc(strlen(n[ptr])+1); strcpy(ig_ptr->name, n[ptr++]);
	ig_ptr->parameter = l_get_time_string();
#endif
	ig_ptr->next = NULL;
	// hints (подсказки)
	l_set_hints();
	l_set_save();
	l_jch_container = true;
}

char *l_jcr_content_to_string(){
	size_t count=0;
	ig_ptr = ig_start;
	while(ig_ptr){			// считаю требуемый размер буфера
		if(ig_ptr->parameter && strlen(ig_ptr->parameter)){
			if(ig_ptr->name){			count += strlen(ig_ptr->name) + 1;
				if(ig_ptr->parameter)	count += strlen(ig_ptr->parameter) + 1;
			}
		}
		ig_ptr = ig_ptr->next;
	}
	char *buf = MALLOC_BYTE(count+5);
	buf[0] = '{';
	ig_ptr = ig_start;
	while(ig_ptr){
		if(ig_ptr->name && ig_ptr->parameter && strlen(ig_ptr->parameter)){
			if(buf[1])
				strcat(buf, ",");
			strcat(buf, ig_ptr->name);
			strcat(buf, ":");
			strcat(buf, ig_ptr->parameter);
		}
		ig_ptr = ig_ptr->next;
	}
	strcat(buf, "}");
	return buf;
}

file_buffer *l_prepare_jch(){
	static file_buffer f_data;
	if(!l_jch_container)
		l_create_jch_content();
	else {
		ig_ptr = l_find_item("\"content\"");
		m_free(ig_ptr->parameter);
		ig_ptr->parameter = l_set_content();
		l_set_hints();
		l_set_save();
	}

	f_data.data = l_jcr_content_to_string();
	f_data.size = strlen(f_data.data);
	return &f_data;
}

void l_base_cut_excess_content(){
	char *n[] = {"\"id\"","\"name\"","\"content\"","\"hint\"","\"save\""};
	char *buf = malloc(buflen), *p;
	jch_arch_item *l;
	l_item *s;
	uint len;

	for(uint i=1; i<l_base_stack_top; i++){
//		if(i == 7776  || i == 10750 || i == 11407 || i == 11450 || i == 11458 || i == 11569 || i == 11571){
			printf("Processing %d item\n", i);
//puts("step");
//continue;
//		}
		l = l_get_item_from_base(i);
		if(l){
			if(!strcmp(l->body, ZEROFILE))
				continue;
			if(!l_get_items(l->body))
				continue;
			memset(buf, 0, buflen);
			p = buf;
			*p++ = '{';
			for(uint a = 0; a < 5; a++){
				s = l_find_item(n[a]);
				if(s){
					strcpy(p, s->name);
					p += strlen(p);
					*p++ = ':';
					strcpy(p, s->parameter);
					p += strlen(p);
					*p++ = ',';
				}
			}
//if(i == 11571){
//puts("step1");
			l_free_buf(ig_start);
//puts("step2");
//}
//else l_free_buf(ig_start);

			if(*(p - 1) == ','){
				*(p - 1) = '}';
				p = '\0';
			}
			else strcpy(p, "}");
			len = strlen(buf);
			l->size = len;
			m_free(l->body);
			l->body = malloc(len+1);
			strcpy(l->body, buf);
		}
	}
	l_jch_container = false;
}

bool l_open_jch(byte* f_data){
	if(!l_get_items(f_data))
		return l_jch_container = false;
	l_item *s;
	s = l_find_item("\"content\"");
	l_parse_jch(s->parameter);
	l_get_hints();									// загружаю подсказки(если они есть)
	l_get_save();									// загружаю сохранения
	l_jch_container = true;
//	s = l_find_item("\"id\"");
//	sprintf(last_opened_file, "%s/%s.jch", puzzle_path, s>parameter);
	return l_jch_container;
}

/*bool l_get_file(uint n){
	char *fn = malloc(MAX_PATH);
	sprintf(fn, "%s/%d.jch", puzzle_path, n);
	bool rs = g_file_test(fn, G_FILE_TEST_EXISTS);
	if(rs){
		strcpy(last_opened_file, fn);
		free(fn);
		return true;
	}
	free(fn);
	return false;
}*/


void l_list_mfree(bool init){
	if(!init){
		jch_arch_item *bank = l_base_stack;
		while (bank != NULL){
			void *d = bank;
			if(bank->body) free(bank->body);
			bank = bank[multiplicity-1].next;
			free(d);
		}
	}
	l_base_stack = NULL;
	l_base_stack_top = 0;
}

void l_base_clear_top_stack(){
	while(true){
		l_base_stack_top--;
		jch_arch_item *s = l_get_item_from_base(l_base_stack_top);
		if(strcmp(s->body, ZEROFILE)){
			l_base_stack_top++;
			break;
		}
		m_free(s->body);
		s->body		= NULL;
		s->next		= NULL;
		s->number	= s->size = 0;
	}
}

jch_arch_item *l_base_alloc_item(size_t size){
	const uint len=sizeof(jch_arch_item) * multiplicity;	// длина выделямого блока памяти под банк
	uint hundred = l_base_stack_top / multiplicity;			// текущая сотня обработки
	uint indx = l_base_stack_top % multiplicity;				// идекс элемента в банке

	jch_arch_item *bank = l_base_stack;
	if(indx == 0){
		if(hundred == 0){
			bank = l_base_stack = malloc(len);
			memset(l_base_stack, 0, len);
		}
		else {
			while(bank[multiplicity-1].next != 0)
				bank = bank[multiplicity-1].next;
			bank[multiplicity-1].next = malloc(len);
			memset(bank[multiplicity-1].next, 0, len);
			bank = bank[multiplicity-1].next;
		}
	}
	if(hundred > 0){
		for(int i = 0; i < hundred; i++){
			if((jch_arch_item *)bank[multiplicity-1].next != 0)
				bank = (jch_arch_item *)bank[multiplicity-1].next;
			else break;
		}
	}
	bank[indx].size = size;
	l_base_stack_top++;
	if(size > 0) bank[indx].body = malloc(size+1);
	return &bank[indx];
}

jch_arch_item *l_get_item_from_base(uint numb){
	numb--;
	if(numb >= l_base_stack_top)
		return NULL;
	uint bank = numb / multiplicity;
	uint indx = numb % multiplicity;
	jch_arch_item *pool = l_base_stack;
	while(bank--) pool = pool[multiplicity-1].next;
	if(pool != NULL) return &pool[indx];
	return NULL;
}

int l_save_in_base(){
	file_buffer *f_data = l_prepare_jch();
	jch_arch_item *item = l_get_item_from_base(last_opened_number);
	item->size = (uint) f_data->size;
	m_free(item->body);
	item->body = (char*)f_data->data;
	l_base_save_to_file();
	return 0;
}

void l_lib_lock(bool state){			// создаю файл-семафор процесса загрузки библиотеки из web
	FILE *ff;
	char *buf = malloc(MAX_PATH);
	char mes[] = "lib is bad";

	sprintf(buf, "%s%s", config_path, "liblock");
	if(!state) remove("buf");
	else if(g_file_test(buf, G_FILE_TEST_EXISTS)){
		sprintf(buf, "%s/%s", config_path, "db");
		remove("buf");
		sprintf(buf, "%s/%s", config_path, "lzw");
		remove("buf");
	}
	else {
		ff = fopen(buf, "w");
		if(ff) fwrite(mes, 1, 10, ff);
		fclose(ff);
	}
	free(buf);
}

bool l_base_load_from_file(bool update){
	if(l_base_stack != NULL && !update) return true;
	char *fn = malloc(MAX_PATH);
	sprintf(fn, "%s/%s", config_path, "db");
	if(!g_file_test(fn, G_FILE_TEST_EXISTS)){
		puts("Data file not found!\n");
		last_opened_number = 0;
		free(fn);
		return false;
	}
	FILE *file = fopen(fn, "rb");
	free(fn);
	if(file == NULL) {
		puts("Data file could not opened!\n");
		return false;
	}
	char *buf = malloc(buflen);
	jch_arch_item *litem;
	if(l_base_stack_top != 0) l_list_mfree(false);

	fseek(file, 0, SEEK_END);
	uint pbuflen = ftell(file);
	fseek(file,0,SEEK_SET);

	uint size, number, total=0;
	fread(&size, sizeof(uint), 1, file);
	fread(&number, sizeof(uint), 1, file);
	while(size > 0){
		total+=size;
		size -= sizeof(uint) * 2;
		if(size >= buflen){
			puts("Error in format data file!\n");
			l_list_mfree(false);
			fclose(file);
			free(buf);
			return false;
		}
		if(fread(buf, 1, size, file) != size) break;
		litem = l_base_alloc_item(size);
		litem->number = number;
		buf[size] = 0;
		strcpy(litem->body, buf);
		if(total >= pbuflen) break;
		fread(&size, sizeof(uint), 1, file);
		fread(&number, sizeof(uint), 1, file);
	}
	fclose(file);
	free(buf);
	m_test_lib();
	return true;
}

bool l_base_save_to_file(){
	if(l_base_stack == NULL || l_base_stack_top == 0) return false;
	char *fn = malloc(MAX_PATH);
	sprintf(fn, "%s/%s", config_path, "db");
	FILE *file = fopen(fn, "wb");
	if(file == NULL) {
		puts("Data file could not opened!");
		free(fn);
		return false;
	}

	jch_arch_item *pool = l_base_stack;
	uint hundred = l_base_stack_top / multiplicity, h, i, size;
	for(h=0; h <= hundred; h++){
		for(i=0; i < multiplicity; i++){
			if(pool[i].size > 0 && pool[i].body != NULL){
				size = pool[i].size + sizeof(uint)* 2;
				fwrite(&size, sizeof(uint), 1, file);
				fwrite(&pool[i].number, sizeof(uint), 1, file);
				fwrite(pool[i].body, pool[i].size, 1, file);
			}
		}
		pool = pool[multiplicity-1].next;
		if(pool == NULL) break;
	}
	fclose(file);
	free(fn);
	return true;
}

bool l_get_archive_from_URI(char *fname){
	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL *ssl;
	SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
	if(ctx == NULL) return false;

	BIO *bio = BIO_new_ssl_connect(ctx);
	BIO_get_ssl(bio, &ssl);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
	BIO_set_conn_hostname(bio, "raw.githubusercontent.com:https");

	if(BIO_do_connect(bio) <= 0){
		printf("Error: %s\n", ERR_reason_error_string(ERR_get_error()));
		puts("Handle BIO_do_connect failed connection!\n");
		BIO_free_all(bio);
		return false;
	}
	char *request = "GET /terraslav/matrix/master/lzw HTTP/1.1\x0D\x0AHost: raw.githubusercontent.com\x0D\x0A\x43onnection: Close\x0D\x0A\x0D\x0A";
	BIO_write(bio, request, strlen(request));
	char *buf = malloc(1024);
	if(!buf) return false;
	BIO_read(bio, buf, 1023);
	char *ptr = strstr(buf, "\r\n\r\n");
	if(ptr != NULL){
		uint total_len = l_atoi(strstr(buf, cl) + strlen(cl));
		free(buf);
		buf=malloc(total_len+1024);
		if(!buf) return false;
		printf("total_len: %d\n",  total_len);
		BIO_reset(bio);
		BIO_write(bio, request, strlen(request));

		int x;
		uint pos=0;
		while(true){
			x = BIO_read(bio, buf+pos, total_len+1023-pos);
			printf("Archive gets: %d byte\n", pos);
			if(x <= 0) break;
			pos+=x;
		}
		ptr = strstr(buf, "\r\n\r\n") + 4;
		FILE* file = fopen(fname, "ab");
		if(file == NULL) return false;
		fwrite(ptr, total_len, 1, file);
		fclose(file);
	}
	else return false;
	return true;
}

bool l_get_archive(){
	if(l_base_stack_top > 14000) return true; // выход если архив уже был скачан-распакован
	char *buf = malloc(MAX_PATH * 2);
	char fname[MAX_PATH];
	sprintf(fname, "%s/lzw", config_path);
	bool rs = g_file_test(fname, G_FILE_TEST_EXISTS);
	if(!rs){	// скачиваю архив при его отсутствии
		if(!l_get_archive_from_URI(fname)){
			sprintf(buf, "wget %s -O %s", "https://raw.githubusercontent.com/terraslav/matrix/master/lzw", fname);
			system(buf);
			free(buf);
		}
	}
	chdir(config_path);
	sprintf(buf, "%s/%s", config_path, "db");
	if(!g_file_test((buf), G_FILE_TEST_EXISTS) && xz(false)) puts("Decompressing done");
	else return false;
	free(buf);
	return true;
}

void *l_get_library_imp(void *a){
	l_get_archive();
	l_base_load_from_file(false);

	char *buf = malloc(buflen);
	char *fn = malloc(MAX_PATH);
	uint failure = 0;
	uint total_len;
	bool changes = false;
	jch_arch_item *e, *litem;

	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL *ssl;
	SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
	if(ctx == NULL){// || !SSL_CTX_load_verify_locations(ctx, NULL, "/etc/ssl/serts")){
		free(buf);
		free(fn);
		thread_get_library = 0;
		return NULL;
	}
	sprintf(buf, "%s:https", host);
	BIO *bio = BIO_new_ssl_connect(ctx);
	BIO_get_ssl(bio, &ssl);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
	BIO_set_conn_hostname(bio, buf);
	if(BIO_do_connect(bio) <= 0){
		printf("Error: %s\n", ERR_reason_error_string(ERR_get_error()));
		puts("Handle BIO_do_connect failed connection!\n");
		free(buf);
		free(fn);
		BIO_free_all(bio);
		thread_get_library = 0;
		return NULL;
	}
	uint count = 0;
	for(uint n=1;;n++){
		e = l_get_item_from_base(n);
		if(e != NULL && strcmp(e->body, ZEROFILE))
			continue;
		sprintf(buf, "Get: %d", n);
		gdk_threads_enter();
		set_status_bar(buf);
		gdk_threads_leave();

		sprintf(buf, "GET %s/%d HTTP/1.1\x0D\x0AHost: %s\x0D\x0A\x43onnection: Close\x0D\x0A\x0D\x0A", host_path, n, host);
		int cnt=0;
		while(BIO_write(bio, buf, strlen(buf)) <= 0){
			if(!BIO_should_retry(bio) || ++cnt > 3) break;
 			else fprintf(stderr, "%s%d", "Retry GETs: ", cnt);
 		}
		int x;
		while(true){
			x = BIO_read(bio, buf, buflen-1);
			if(x <= 0) break;
			buf[x] = 0;
#ifdef DEBUG
puts(buf);
#endif
		}
		char *ptr = strstr(buf, "{\"id");
		if(ptr != NULL){
			total_len = strlen(ptr);
			if(e){								// если номер не занят перезаписываю его
				free(e->body);
				e->body = malloc(total_len);
				strcpy(e->body, ptr);
				e->size = total_len;
			}
			else {
				litem = l_base_alloc_item(total_len);
				litem->number = n;
				strcpy(litem->body, ptr);
			}
			if((++count % 100) == 0) l_base_save_to_file();	// сохраняю базу при каждой загруженной сотне
			changes = true;
			failure = 0;
		}
		else {
			if(changes){
				if(e){
					free(e->body);
					e->body = malloc(10);
					strcpy(e->body, ZEROFILE);
					e->size = 10;
				}
				else {
					litem = l_base_alloc_item(10);
					strcpy(litem->body, ZEROFILE);
					litem->number = n;
				}
			}
			if(failure > 2) break;
			failure++;
		}
		BIO_reset(bio);
	}
	l_base_clear_top_stack();	// зачистка верхушки стека БД
	BIO_free_all(bio);
//	thread_get_library = 0;
	if(changes)
		l_base_cut_excess_content();
	if(changes && !l_base_save_to_file())
		puts("Error save library to file!");
	if(changes){
		if(xz(true)) puts("Compressing done");
	}
	m_test_lib();
	free(buf);
	free(fn);
	thread_get_library = 0;
	return NULL;
}

void l_get_library(bool dmes){
	if(thread_get_library){
		pthread_cancel(thread_get_library);
		thread_get_library = 0;
		if(dmes) d_message(_("Library"), _("Updates library stoped!"), false);
	}
	else {
		int result = pthread_create(&thread_get_library, NULL, l_get_library_imp, NULL);
		if (result != 0) perror("Creating thread");
		else if(dmes) d_message(_("Library"), _("Updates library started!"), false);
	}
}

void l_open_random_puzzle(bool color){
	char *a;
	uint fs, st, en, i;
	jch_arch_item *puzz;
	do{
		i = rand()%l_base_stack_top;
		puzz = l_get_item_from_base(i);
		a = puzz->body;
		fs = st = en = 0;
		for(i = 0; i < puzz->size; i++) // определяю длину описателей цветов
			if(a[i] == '#'){
				if(!fs) fs = i;
				else if(!st) st = i;
				else if(!en) en = i;
				else break;
			}
		i = en - st;
		if((!color && i < 8) || (color && i > 7)) break;
	} while(1);
	l_get_name_from_item(puzzle_name, puzz->body);
	strcat(puzzle_name, ".mem");
	strcpy(last_opened_file, puzzle_name);
	m_open_puzzle(false, false, puzz->body);
}

/******************** Процедуры поиска в библеотеке ************************/
#define MAX_LIST 35

char *str, *fnd;
size_t *flist;

void l_lower_register_UTF8(char *dd, char *ss){		// понижаю регистр UTF-8 без BOM
	byte *s = (byte*)ss, *d = (byte*)dd;
	while (*s){
		if(*s == 0xd0 || *s == 0xd1){				// дбухбайтовые буквы
			byte a = *(s+1);
			if(*s == 0xd0 && *(s+1) == 0x81){		// буква Ё ё-маё!
				*d++ = 0xd1; *d++ = 0x91; s+=2;
			}
			else if((a >= 0xb0 && a <= 0xbf) ||		// маленькие букафки
					(a >= 0x80 && a <= 0x8f) ||
					(*s == 0xd1 && a == 0x91))
				{*d++ = *s++; *d++ = *s++;}
			else  if(*(s+1) < 0xa0){
				*d++ = *s++;
				*d++ = *s++ | 0x20;
			}
			else {
				*d++ = 0xd1; s++;
				*d++ = *s++ & 0xdf;
			}
		}
		else if(*s >= 'A' && *s <= 'Z')				// однобайтовые буквы
			*d++ = *s++ | 0x20;
		else *d++ = *s++;
	}
	*d = 0;
}

bool l_get_name_from_item(char *dist, char *body){	// поиск имени в теле описания пазла
	if(!body || !dist || !*body) return false;
	char *s = body, *d = dist;
	size_t len, ln, cnt = 2;
	ln = len = strlen(body);
	while (len--){
		if(*s++ == ':') cnt--;
		if(!cnt) break;
	}
	if(!len) return false;
	while(*s != ',' && ln--)
		if(*s != '"') *d++ = *s++;
		else s++;
	*d = 0;
	return true;
}

bool l_get_size_from_item(char *d, char *s){
	size_t len, ln = len = strlen(s);
	while(ln--)
		if(*s++ == '#') break;
	if(!ln) return false;
	ln = 2;
	while(len--)
		if(*s != ',' && *s <= '9' && *s >= '0') *d++ = *s++;
		else if(--ln) {*d++ = 'x'; s++;}
		else break;
	if(!len) return false;
	*d = 0;
	return true;
}

bool l_get_colr_from_item(char *d, char *s){
	size_t len, ln = len = strlen(s), i = 2;
	while(ln--)
		if(*s++ == '#')
			if(!--i) break;
	if(!ln) return false;
	i = 0;
	while(len--)
		if(*s != '#') {if(*s++ == ',') i++;}
		else break;
	if(!len) return false;
	i++;
	if(i == 1) strcpy(d, _("monochrome"));
	else sprintf(d, "%d %s", (int)i, i > 4 ? _("colores") : _("colors"));
	return true;
}

void l_find_puzzle_string(char *string){
	jch_arch_item *item;
	size_t i, fcount = 0;

	flist	= malloc(MAX_LIST*sizeof(size_t));
	str		= malloc(2048);
	fnd		= malloc(strlen(string)+1);

	l_lower_register_UTF8(fnd, string);

	for(i=1; i < l_base_stack_top-1; i++){
		item = l_get_item_from_base(i);
		if(item && item->body)
			if(l_get_name_from_item(str, item->body)){
				l_lower_register_UTF8(str, str);
				if(NULL != strstr(str, fnd))
					if(fcount < MAX_LIST)
						flist[fcount++] = i;
				if(fcount >= MAX_LIST) break;
			}
	}
	sprintf(str, "%s%s", _("Not found: "), string);
	if(!fcount) d_message(_("Search result"), str, false);
	else if(fcount == 1) m_open_puzzle(false, false, (l_get_item_from_base(flist[0]))->body);
	else d_view_list (flist, fcount);
	free(str); free(fnd); free(flist);
}

void l_find_puzzle(){
	char *buf = m_get_file_path_name(false);
	uint i, x, y = l_atoi(buf);
	m_free(buf);
	buf = d_find_puzzle_dialog(y);
	if(buf != NULL){
		x = 0;
		for(i = 0; i < strlen(buf); i++)	// проверка номер это или стринг
			if(buf[i]<'0' || buf[i] > '9') x = 1;
		if(!x){
			i = l_atoi(buf);
			if(i != y){
				if(i < l_base_stack_top){
					last_opened_number = y;
					place = memory;
					m_open_puzzle(false, false, ((jch_arch_item*)l_get_item_from_base(i))->body);
				}
				else d_message(_("Find error!"), _("Sorry, its big number!"), 0);
			}
		}
		else l_find_puzzle_string(buf);
		free(buf);
	}
}
