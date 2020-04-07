// utils.h
#ifndef _UTILS_
#define _UTILS_

#include "types.h"

// Типы для функций undo - redo ****************************************

typedef struct urs_tag{	// ячейка сохранения состояния
	byte	*t;			// top - верхняя оцифровка
	byte	*l;			// left - левая
	byte	*m;			// matrix
	int		st;			// size top
	int		sl;			// size left
	int		sm;			// size matrix
}urs;

typedef struct ur_tag{	// юнит undo-redo
	urs		*a,*b;		// after-before change
	pr		act;		// action
	bool 	isColor;	// цвет
}ur;
						// структура хранения заголовка при полном сохранении
typedef struct u_fs_tag{size_t t,l,x,y;bool isColor;}u_fs;

// указатели на связанный список undo - redo ***************************
ur** u_stack; // указатель на список отмен
int u_current;// теущая позиция отмен
ur* u_work;

void u_init();
void u_delete();
void u_clear(int indx);
void u_after(bool);
void u_undo();
void u_redo();
void u_clear_stack_redo();
void u_state_free();
void u_change_properties();

bool u_u_compare(byte *one, byte *two, int size);

bool u_digits_updates(bool undos);
void u_digits_change_event(bool falls, bool update);
void u_shift_puzzle(direct dir);

#endif //_UTILS_
