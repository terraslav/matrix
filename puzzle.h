//puzzle.h
#ifndef _PUZZLE_
#define _PUZZLE_

#include "types.h"

#define anim_amp 50
#define chain_s 50

pthread_t	thread_solve_no_logic;

bool validate;											// флаг валидности данных матрицы
ready_category puzzle_status;							// статус пазла |ошибоочный|нелогический|решаемый|
uint stat_difficult;									// статус сложности решения
bool solved;											// флаг решенности матрицы
bool *x_error, *y_error;
bool c_solve_buffers;
static volatile int step_current = -1;					// текущяя строки или колонка при пошаговом решении
puzzle *atom;
byte *c_st_pos, *c_en_pos;
byte *shadow_map, *solved_map;							// теневой и проверочный буфера матрицы
addr_cell *solved_cells;								// список решенных ячеек
unsigned int solv_cell_current, solv_cell_top;			// указатели в списке решенных ячеек

void c_malloc_puzzle_buf(puzzle *);
void c_free_puzzle_buf(puzzle *);
void c_reset_puzzle_pointers();
void c_solve_init(bool);
void c_solve_destroy();

bool c_make_overlapping(byte*,bool);
bool c_make_start_position();
bool c_make_empty();
void c_solver();
bool c_check_digits_summ();
bool c_check_complete(byte*);
int  c_test_validate_matrix();							// тест на валидность данных матрицы
bool c_test_validate_cross(byte x, byte y);				// тест валидности строки и колонки выбранной точки
bool c_test_line(byte*,int,bool,c_test_line_res*);
void c_view_status();
void c_make_hints(byte *map);							//маркировка подсказок
void c_clear_errors();
#endif //_PUZZLE_
