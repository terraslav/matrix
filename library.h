// library.h - загрузка кроссвордов из интернета
#ifndef _HTTP_GETS_
#define _HTTP_GETS_

#include "types.h"
#include "mafile.h"
#include "main.h"
#include "paint.h"
#include "utils.h"

#define LZW

#define multiplicity 100			// кратность блоков выделяемой списку памяти
#define ZEROFILE "zero_file"

typedef struct l_item_tag{
	char 					*name;
	char 					*parameter;
	void 					*next;
} l_item;

typedef struct jch_arch_item_tag{
	char					*body;
	uint					size;
	uint					number;
	void					*next;
} jch_arch_item;

l_item 						*ig_start,
							*ig_ptr;
bool						l_jch_container;
pthread_t					thread_get_library;
uint						l_base_stack_top;
jch_arch_item 				*l_base_stack;

uint l_atoi					(char*);
char *l_get_name			(void);
bool l_open_jch				(byte*);
void l_close_jch			(void);
file_buffer *l_prepare_jch	(void);
//bool l_get_file				(uint);
void l_get_library			(bool);
bool l_base_save_to_file	(void);
void*l_get_library_imp		(void*);
void l_set_name				(void);
bool l_base_load_from_file	(bool);
void l_list_mfree			(bool);
jch_arch_item *l_get_item_from_base(uint);
int l_save_in_base			(void);
bool xz(bool action);
void l_open_random_puzzle	(bool);
void l_find_puzzle			(void);
bool l_get_name_from_item	(char *dist, char *body);
bool l_get_size_from_item	(char *d, char *s);
bool l_get_colr_from_item	(char *d, char *s);
void l_lib_lock				(bool state);
#endif //_HTTP_GETS_
