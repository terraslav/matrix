#include <locale.h>
#include <gdk/gdkkeysyms.h>
#include <libnotify/notify.h>
#include <sys/stat.h>
#include <signal.h>

#include "main.h"
#include "paint.h"
#include "mafile.h"
#include "puzzle.h"
#include "utils.h"
#include "dialog.h"
#include "library.h"

GtkWidget *vbox, *hbox, *toolbar, *handlebox;
void print_page(GtkWidget*);
uint non_area_y_size;

static gboolean quit_program(bool quick){
	if(!quick && 0 > m_save_puzzle(true))	return true;
	if(!quick) {
		gtk_widget_destroy(window);
		gtk_main_quit();
	}
	if(l_base_stack_top) l_base_save_to_file();				// сохраняю базу
	l_list_mfree(false);
	if(timer_id) g_source_remove(timer_id);
	if(timer_demo) g_source_remove(timer_demo);
	cairo_destroy(cr);
	save_parameter();
	l_close_jch();
	m_demo_reset_list();
	if (surface) cairo_surface_destroy (surface);
	u_delete(); 													// удаляю буфера отмен
	c_solve_destroy();												// удаляю буфера "решалки"
	c_free_puzzle_buf(atom);
	m_free(atom);
	if(thread_get_library) pthread_cancel(thread_get_library);
	sprintf(last_opened_file, "%s%s", config_path, LOCK_FILE);
	remove(last_opened_file);
	puts(_("Im i`s dead\n"));
	return false;
}

int momento(){
	puts("\n\
********************  Sorry=(  *********************\n\n\
\"matrix\" received signal SIGSEGV, Segmentation fault\n\n\
******************  Saving data...  ****************");
	if(place == memory) {
		l_save_in_base();
//		l_base_save_to_file();
	}
	else m_save_puzzle(false);
	quit_program(true);
	return 0;
}

void posix_death_signal(int signum){	// обработчик исключений SIGSEGV
	momento();
	signal(signum, SIG_DFL);
	exit(3);
}

void posix_abort_signal(int signum){	// обработчик исключений SIGABRT
	quit_program(false);
//	signal(signum, SIG_DFL);
	exit(3);
}
void scrolled_viewport(double x, double y){
	GtkAdjustment* adjustment = gtk_scrolled_window_get_hadjustment((GtkScrolledWindow*)scroll_win);
	double	c		= gtk_adjustment_get_value(adjustment),
			max		= gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment),
			lower	= gtk_adjustment_get_lower(adjustment);
	c += x;
	if(c > max)			c = max;
	else if(c < lower)	c = lower;
	gtk_adjustment_set_value(adjustment, c);
	gtk_scrolled_window_set_hadjustment((GtkScrolledWindow*)scroll_win, adjustment);

	adjustment = gtk_scrolled_window_get_vadjustment((GtkScrolledWindow*)scroll_win);
	c		= gtk_adjustment_get_value(adjustment),
	max		= gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment),
	lower	= gtk_adjustment_get_lower(adjustment);
	c += y;
	if(c > max)			c = max;
	else if(c < lower)	c = lower;
	gtk_adjustment_set_value(adjustment, c);
	gtk_scrolled_window_set_vadjustment((GtkScrolledWindow*)scroll_win, adjustment);
}

void update_view(char dir){	// центровка вывода в scroll_win
	byte size = (byte)((char)cell_space + dir);
	if(size < 8 ||  size > 40)return;
	cell_space = size;
	uint	y	= cell_space * atom->y_puzzle_size,
			x	= cell_space * atom->x_puzzle_size;
	gtk_widget_set_size_request(draw_area,x, y);
	gtk_widget_queue_draw(draw_area);
}

// Переписываем статусбар **********************************************
void set_status_bar(char *buff){
	gint context_id = gtk_statusbar_get_context_id(
                       GTK_STATUSBAR (statusbar), "Statusbar example");
	gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), context_id);
	gtk_statusbar_push (GTK_STATUSBAR (statusbar),
								GPOINTER_TO_INT(context_id), buff);
}

void clear_status_bar(){
	gint context_id = gtk_statusbar_get_context_id(
                       GTK_STATUSBAR (statusbar), "Statusbar example");
	gtk_statusbar_remove_all (GTK_STATUSBAR (statusbar), context_id);
}

gboolean scroll_event(GtkWidget *a,GdkEventScroll *event,void *b){
	if(atom->curs->status){
		set_digit_value(event->direction > 0 ? -3 : -2);
		return true;
	}
	update_view(event->direction ? -1 : 1);
	return true;
}

gboolean key_press (GtkWidget *widget, GdkEventKey *event, gpointer user_data){
	direct vect;
	if(atom->curs->status){
		if(event->keyval >= GDK_KP_0 && event->keyval <= GDK_KP_9)
			{set_digit_value(event->keyval - GDK_KP_0); return true;}
		else if(event->keyval >= GDK_0 && event->keyval <= GDK_9)
			{set_digit_value(event->keyval - GDK_0); return true;}
		else if(event->keyval == GDK_KP_Decimal || event->keyval == GDK_period)
			{set_digit_value(10); return true;}
		else if(event->keyval == GDK_Delete || event->keyval == GDK_BackSpace)
			{set_digit_value(-1); return true;}
	}
	switch (event->keyval){
		case GDK_Return:
		case GDK_KP_Enter:
			if(!atom->curs->status) p_edit_digits(true);
			else return_cursor();
			return true;

		case GDK_Escape:
			p_edit_digits(false);
			return true;

		case GDK_Left:
			vect=left;
			if(atom->curs->status)
				p_move_cursor(vect);
			else if(event->state & GDK_CONTROL_MASK)
				u_shift_puzzle(vect);
			else if(event->state & GDK_SHIFT_MASK){
				if(0 > m_save_puzzle(true))
				break;
				m_demo_set_next(-1);
			}
			return true;

		case GDK_Right:
			vect=right;
			if(atom->curs->status)
				p_move_cursor(vect);
			else if(event->state & GDK_CONTROL_MASK)
				u_shift_puzzle(vect);
			else if(event->state & GDK_SHIFT_MASK){
				if(0 > m_save_puzzle(true))
				break;
				m_demo_set_next(1);
			}
			return true;

		case GDK_Up:
			vect=up;
			if(atom->curs->status)
				p_move_cursor(vect);
			else if(event->state & GDK_CONTROL_MASK)
				u_shift_puzzle(vect);
			return true;

		case GDK_Down:
			vect=down;
			if(atom->curs->status)
				p_move_cursor(vect);
			else if(event->state & GDK_CONTROL_MASK)
				u_shift_puzzle(vect);
			return true;

		default:
#ifdef DEBUG
			if (event->state & GDK_SHIFT_MASK){
				printf("key pressed: %s%d\n", "shift + ", event->keyval);
			}
			else if (event->state & GDK_CONTROL_MASK){
				printf("Code key pressed: %s%d\n", "ctrl + ", event->keyval);
			}
			else{
				fprintf(stderr, "Code key pressed: %d\n", event->keyval);
			}
#endif
		break;
	}
	return false;
}

void change_tool_style(int btn){
	static char count = 1;
	if(btn == 1) {count ++; if(count > 2) count = 0;}
	else if(btn == 3) { if(count == 0) count = 3; count --;}
	if (!count)
		gtk_toolbar_set_style(GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH);
	else if (count == 1)
		gtk_toolbar_set_style(GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
	else if (count == 2)
		gtk_toolbar_set_style(GTK_TOOLBAR (toolbar), GTK_TOOLBAR_TEXT);
}

gboolean button_release (GtkWidget*a, GdkEventButton *event, gpointer data){
	p_botton_release(event);
	return false;
}

gboolean button_press (GtkWidget* widget, GdkEventButton *event, gpointer data){
	int indx;
	if(atom){
		atom->x_cell = (event->x - atom->x_begin) / cell_space;
		atom->y_cell = (event->y - atom->y_begin) / cell_space;
		// проверка на вхождение в оцифровку слева
		if(	atom->x_cell <	atom->left_dig_size &&
			atom->y_cell >=	atom->top_dig_size &&
			atom->y_cell <	atom->y_puzzle_size){
			indx = (atom->y_cell - atom->top_dig_size) * atom->left_dig_size +
					(atom->left_dig_size - atom->x_cell - 1);
			p_digits_button_event(&atom->left, indx,	event);
			return TRUE;
		}
		// проверка на вхождение в оцифровку сверху
		if(	atom->x_cell >=	atom->left_dig_size &&
			atom->x_cell <	atom->x_puzzle_size &&
			//atom->y_cell >=	0 &&
			atom->y_cell <	atom->top_dig_size){
			indx = (atom->x_cell - atom->left_dig_size) *
					atom->top_dig_size + atom->top_dig_size - atom->y_cell - 1;
			p_digits_button_event(&atom->top, indx,	event);
			return TRUE;
		}
		// завершаю редактирование оцифровки при любом щелчке вне её
		if(atom->curs->status){
			if(event->button == 1)// && event->type==GDK_2BUTTON_PRESS)
				p_edit_digits(false);
		}
		// проверка на вхождение в матрицу
		if(	atom->x_cell >=	atom->left_dig_size &&
			atom->x_cell <	atom->x_puzzle_size &&
			atom->y_cell >=	atom->top_dig_size&&
			atom->y_cell <	atom->y_puzzle_size){
			indx = atom->x_cell - atom->left_dig_size +
				(atom->y_cell - atom->top_dig_size) * atom->x_size;
			p_matrix_button_event(indx, event);
			return TRUE;
		}
		// проверка на вхождение в миниатюру
		if(	atom->x_cell <	atom->left_dig_size &&
			atom->y_cell <	atom->top_dig_size){	// меняю стиль тулбара
			change_tool_style(event->button);
			return TRUE;
		}
	}
	return FALSE;
}

//callback-функция меню ************************************************
static void menu_func(gpointer a, guint data, GtkWidget *wiget){
	GtkItemFactory *item_factory = gtk_item_factory_from_widget(menubar);
	GtkWidget *wig;
	GdkPixbuf * pixbuf;
	int width, height, depth;
	char *buf, *buf1;
	int x, y;
	uint i;
	if(demo_flag) m_demo_stop();	// отключение демо при любой команде
	if(timer_id)		// блокировка некоторых функций меню на время анимации
		switch(data){
			case IDT_SOLVE:
			case IDM_SOLVE:
				break;
			default:
				return;
			break;
		}
	switch(data){
		case IDM_OPEN:
		case IDT_OPEN:
			buf = malloc(256);
			while(!m_open_puzzle(true, false, NULL)){
				sprintf(buf, _("Error opening file: %s\nFile format not suitable!"), last_opened_file);
				d_message(_("Puzzle error!"), buf, 0);
			}
			m_free(buf);
			break;
		case IDM_NEW:
		case IDT_NEW:
			if(0 > m_save_puzzle(true))
				break;
			m_new_puzzle();
			break;
		case IDM_SAVE:
		case IDT_SAVE:
			m_save_puzzle(false);
			break;
		case IDM_SAVE_AS:
			buf1 = l_get_name();
			if(!buf1) {
				i = strlen(last_opened_file);
				if(i){
					buf = malloc(i + 1);
					strcpy(buf, last_opened_file);
				}
				else {
					buf = malloc(20);
					strcpy(buf, "noname.jch");
				}
			}
			else {
				buf = malloc (strlen(buf1) + 1);
				strcpy(buf, buf1);
			}
			i = strlen(buf);
			x = y = 0;
			for(; i > 0; i--){
				if(buf[y]=='"')
					y++;
				if(buf[y]!=' ')
					 buf[x++] = buf[y];
				else buf[x++] = '_';
				y++;
				}
			ftype = m_get_file_type(last_opened_file);
			if(ftype == mem || ftype == empty)
				 sprintf(last_opened_file, "%s/%s/%s.%s", home_dir, WORK_NAME, buf, "jch");
			else sprintf(last_opened_file, "%s/%s/%s", home_dir, WORK_NAME, buf);
			if(d_save_as_dialog()){
				place = exfile;
				m_save_puzzle_impl();
			}
			m_free(buf);
			break;
		case IDM_PRINT:
			print_page(wiget);
			break;
		case IDM_EXPORT:
			g_assert(NULL != window);
			gdk_window_get_geometry(gtk_widget_get_window(GTK_WIDGET(scroll_win)), &x, &y, &width, &height, &depth);
			if(((width-x) < (atom->x_puzzle_size*cell_space) || (height-y) < (atom->y_puzzle_size*cell_space))){
				d_message(_("Export view"),
				_("Only the part seen on the screen nanograms is exported.\nI recommend to choose suitable scale."), 0);
				break;
			}
			pixbuf = gdk_pixbuf_get_from_drawable(NULL , gtk_widget_get_window(GTK_WIDGET(draw_area)),
				gdk_colormap_get_system(), atom->x_begin, atom->y_begin, 0, 0,
					atom->x_puzzle_size*cell_space+2, atom->y_puzzle_size*cell_space+2);
			if (pixbuf != NULL){
				char bufp[] = {"/tmp/nanogamm.png"};
				gdk_pixbuf_save(pixbuf, bufp, "png", NULL, NULL);
				g_object_unref(pixbuf);
				char *bufr = malloc(512);
				sprintf(bufr, _("Export view screen to file: %s completed!"), bufp);
				set_status_bar(bufr);
				sprintf(bufr, "xdg-open %s", bufp);
				system(bufr);
				m_free(bufr);
			}
			else d_message(_("Error export"), _("Don't save expotrs"), 0);
			break;
		case IDM_FEATURES:
			c_view_status();
			break;
		case IDM_PROPERTIS:
			u_change_properties();
			break;
		case IDM_QUIT:
			quit_program(false);
			break;
		case IDM_UNDO:
		case IDT_UNDO:
			u_undo();
			break;
		case IDM_REDO:
		case IDT_REDO:
			u_redo();
			break;
		case IDM_INVERT:
			if(atom->isColor){
				d_invert_palette();
			}
			else {
				for(i=0; i<atom->map_size; i++){
					if(atom->matrix[i] == MARK || atom->matrix[i] == EMPTY)
						 atom->matrix[i] = BLACK;
					else atom->matrix[i] = EMPTY;
				}
				u_digits_updates(false);
				u_after(false);
			}
			c_solve_init(false);
			gtk_widget_queue_draw(draw_area);
			break;
		case IDM_PASTE:
			break;
		case IDM_ROTATE_LEFT:
			u_shift_puzzle(left);
			break;
		case IDM_ROTATE_RIGHT:
			u_shift_puzzle(right);
			break;
		case IDM_ROTATE_UP:
			u_shift_puzzle(up);
			break;
		case IDM_ROTATE_DOWN:
			u_shift_puzzle(down);
			break;
		case IDM_CLEAR_PUZZLE:
		case IDT_CLEAR_PUZZLE:
			for(x=0; (size_t)x < atom->map_size; x++)
				if(atom->matrix[x] != EMPTY || shadow_map[x] != EMPTY) break;
			for(i=0; i<atom->top_dig_size*atom->x_size; i++)// очищаю и отметки оцифровки
				atom->top.colors[i] &= COL_MARK_MASK;
			for(i=0; i<atom->left_dig_size*atom->y_size; i++)
				atom->left.colors[i] &= COL_MARK_MASK;
			c_solve_init(false);							// переинициализация "решателя"
			if((size_t)x != atom->map_size){
				memset(shadow_map, EMPTY, atom->map_size);	// очищаю матрицу
				memset(atom->matrix, EMPTY, atom->map_size);
				u_after(false);
			}
			gtk_widget_queue_draw(draw_area);
			break;
		case IDM_CLEAR_DIGITS:
			for(i=0; i<atom->left_dig_size * atom->y_size; i++)
				atom->left.colors[i] = atom->left.digits[i] = 0;
			for(i=0; i<atom->top_dig_size * atom->x_size; i++)
				atom->top.colors[i] = atom->top.digits[i] = 0;
			atom->digits_status = false;
			u_after(false);
			gtk_widget_queue_draw(draw_area);
			break;
		case IDM_CLEAR_PUZ_MARK:
			for(i=0; i<atom->map_size; i++)
				if(atom->matrix[i] == MARK) atom->matrix[i] = ZERO;
			u_after(false);
			gtk_widget_queue_draw(draw_area);
			break;
		case IDM_CLEAR_DIG_MARK:
			for(i=0; i<atom->top_dig_size*atom->x_size; i++)
				atom->top.colors[i] &= COL_MARK_MASK;
			for(i=0; i<atom->left_dig_size*atom->y_size; i++)
				atom->left.colors[i] &= COL_MARK_MASK;
			u_after(false);
			gtk_widget_queue_draw(draw_area);
			break;
		case IDM_MORE:
			update_view(1);
			break;
		case IDM_LESS:
			update_view(-1);
			break;
		case IDM_NORMAL:
			x = scroll_win->allocation.width;
			y = scroll_win->allocation.height;
			x = (int)((double)x * (double)0.9);
			y = (int)((double)y * (double)0.9);
			gtk_widget_set_size_request(draw_area, x, y);
			break;
		case IDM_TOOLBAR:
			wig = gtk_item_factory_get_item_by_action(item_factory,data);
			parameter[ptoolbar] = GTK_CHECK_MENU_ITEM (wig)->active;
			if (parameter[ptoolbar])	gtk_widget_show(handlebox);
			else				gtk_widget_hide(handlebox);
			break;
		case IDM_STATUSBAR:
			wig = gtk_item_factory_get_item_by_action(item_factory,data);
			parameter[pstatusbar] = GTK_CHECK_MENU_ITEM (wig)->active;
			if (parameter[pstatusbar])	gtk_widget_show(statusbar);
			else				gtk_widget_hide(statusbar);
			break;
		case IDM_PALETTE:
			wig = gtk_item_factory_get_item_by_action(item_factory,data);
			palette_show = GTK_CHECK_MENU_ITEM (wig)->active;
			gtk_toggle_tool_button_set_active((GtkToggleToolButton *)tb[IDT_PALETTE-IDT_BEGIN], palette_show);
			d_palette_show(palette_show);
			break;
		case IDM_GRID:
			wig = gtk_item_factory_get_item_by_action(item_factory,data);
			parameter[pgrid] = GTK_CHECK_MENU_ITEM (wig)->active;
			gtk_toggle_tool_button_set_active((GtkToggleToolButton *)tb[IDT_GRID-IDT_BEGIN], parameter[pgrid]);
			gtk_widget_queue_draw(draw_area);
			break;
		case IDM_GRID5:
			wig = gtk_item_factory_get_item_by_action(item_factory,	data);
			parameter[pgrid5] = GTK_CHECK_MENU_ITEM (wig)->active;
			gtk_toggle_tool_button_set_active((GtkToggleToolButton *)tb[IDT_GRID5-IDT_BEGIN], parameter[pgrid5]);
			gtk_widget_queue_draw(draw_area);
			break;
		case IDM_CALC_DIG:
		case IDT_CALC_DIG:
			if(u_digits_updates(true) == true)
				gtk_widget_queue_draw(draw_area);
			break;
		case IDM_MAKE_OVERLAPPED:
			if(solved) break;
			c_make_overlapping(shadow_map, true);
			u_after(false);
			p_update_matrix();
			break;
		case IDM_SOLVE:
		case IDT_SOLVE:
			c_solver();
			u_after(false);
			toggle_changes(false, matrix);
			break;
		case IDM_ANIMATE:
			wig = gtk_item_factory_get_item_by_action(item_factory,	data);
			parameter[panimate] = GTK_CHECK_MENU_ITEM (wig)->active;
			break;
		case IDM_LESSON:
			wig = gtk_item_factory_get_item_by_action(item_factory,	data);
			parameter[plesson] = GTK_CHECK_MENU_ITEM (wig)->active;
			if(c_solve_buffers) for(i=0;i<atom->map_size;i++) shadow_map[i]=EMPTY;
			gtk_widget_queue_draw(draw_area);
			break;
		case IDM_VNAME:
			wig = gtk_item_factory_get_item_by_action(item_factory,	data);
			parameter[viewname] = GTK_CHECK_MENU_ITEM (wig)->active;
			m_set_title();
			break;
		case IDM_HELP_0:
		case IDM_HELP_1:
		case IDM_HELP_2:
		case IDM_HELP_3:
		case IDM_HELP_4:
			help_level = (byte)(data-IDM_HELP_0);
			break;
		case IDM_LOAD_RANDOM:
			if(0 > m_save_puzzle(true))
				break;
			srand(time(NULL));
			place = memory;
			l_open_random_puzzle(false);
			break;
		case IDM_LOAD_RANDOM_C:
			if(0 > m_save_puzzle(true))
				break;
			srand(time(NULL));
			place = memory;
			l_open_random_puzzle(true);
			break;
		case IDM_SEARCH_IN_LIBRARY:
			if(0 > m_save_puzzle(true))
				break;
			l_find_puzzle();
			break;
		case IDM_DEMO_START:
			if(0 > m_save_puzzle(true))	break;
			if(!m_demo_start())
				d_message(_("Error demo begin"), last_error, 0);
			break;
		case IDM_LIBRARY_NEXT:
			if(0 > m_save_puzzle(true))	break;
			m_demo_set_next(1);
			break;
		case IDM_LIBRARY_PREV:
			if(0 > m_save_puzzle(true))	break;
			m_demo_set_next(-1);
			break;
		case IDM_HTTP_OPEN:
			m_open_url();
			break;
		case IDM_HELP:
			system(_("xdg-open https://ru.wikipedia.org/wiki/Японский_кроссворд"));
			break;
		case IDM_ABOUT:
			d_about_dialog();
			break;
		default:
			break;
		}
}

//callback-функция тулбара *********************************************
static void toolbar_event(GtkToggleToolButton *widget, gpointer data){
#define GTB(a) (GtkToggleToolButton*)(a)
	GtkWidget *wig;
	GtkItemFactory *item_factory = gtk_item_factory_from_widget(menubar);
	switch((long)data){
		case IDT_PALETTE:	// toggle palette
			palette_show = gtk_toggle_tool_button_get_active(widget);
			wig = gtk_item_factory_get_item_by_action(item_factory,IDM_PALETTE);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wig),palette_show);
			d_palette_show(palette_show);
			break;
		case IDT_GRID:	// toggle cells
			parameter[pgrid] = gtk_toggle_tool_button_get_active(widget);
			wig = gtk_item_factory_get_item_by_action(item_factory, IDM_GRID);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wig), parameter[pgrid]);
			gtk_widget_queue_draw(draw_area);
			break;
		case IDT_GRID5:	// toggle cells 5x5
			parameter[pgrid5] = gtk_toggle_tool_button_get_active(widget);
			wig = gtk_item_factory_get_item_by_action(item_factory, IDM_GRID5);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wig), parameter[pgrid5]);
			gtk_widget_queue_draw(draw_area);
			break;
		case IDT_PEN_PAINT:
			ps = pen;
			break;
		case IDT_RECT_PAINT:
			ps = rectangle;
			break;
		case IDT_FILL_PAINT:
			ps = filling;
			break;
	}
}

// Определители меню ***************************************************
static char* paths[]={
//File
	"/_File", "/File/_New...", "/File/_Open...",
	"/File/sep","/File/_Save","/File/Save _As","/File/_Export to PNG",
	"/File/sep", "/File/Print...",
	"/File/sep", "/File/Feat_ures",
	"/File/sep", "/File/_Quit",
// Edit
	"/_Edit", "/Edit/_Undo", "/Edit/_Redo",
	"/Edit/sep", "/Edit/_Invert colors", "/Edit/_Paste",
	"/Edit/sep","/Edit/Slide l_eft", "/Edit/Slide _right",
	"/Edit/Slide _up", "/Edit/Slide _down",
	"/Edit/sep","/Edit/Clear _Puzzle", "/Edit/Clear _Digits",
	"/Edit/Clear p_uzzle markers", "/Edit/Clear d_igits marker",
	"/Edit/sep","/Edit/P_roperty...",
//View
	"/_View", "/View/_More", "/View/_Less", "/View/_Reset",
	"/View/sep", "/View/_Toolbar", "/View/_Statusbar", "/View/_Palette",
	"/View/sep", "/View/_Grid", "/View/Gr_id 5x5",
//Puzzle
	"/_Puzzle", "/Puzzle/_Solve puzzle", "/Puzzle/_Calculate digits",
			"/Puzzle/_Make overlapped",
//Settings
	"/_Settings", "/Settings/_Animate",	"/Settings/_lesson (step by step)","/Settings/_View name",
	"/Settings/Help level","/Settings/manual solved","/Settings/automark digits",
	"/Settings/automark lines","/Settings/advanced help", "/Settings/persuasive mode",
//Library
	"/_Library",
	"/Library/Open random black nanogramm",
	"/Library/Open random color nanogramm",
	"/Library/Find","/Library/Sep",
	"/Library/_Demo","/Library/_Next","/Library/_Prev","/Library/Sep",
	"/Library/_Open_URL",
//Help
	"/_Help", "/Help/_Help", "/Help/_About"
};
static gchar *accelerators[] = {
// File
	NULL, "<control>N", "<control>O",	//File, New, Open
	NULL, "<control>S", "<control>A",	//sep., Save, Save As,
						"<control>E",	//Export
	NULL, "<control>P",					//sep., Print
	NULL, "F11",						//sep., Features
	NULL, "<control>Q",					//sep., Quit
// Edit
	NULL, "<control>Z", "<control>Y",	//Edit, Undo, Redo
	NULL, "<control>I", "<control>V",	//sep., Invert, Paste
	NULL, "<control>Left", "<control>Right",
	"<control>Up", "<control>Down",		//sep., Rotate, Left, Right, Up, Down
	NULL, NULL, NULL, NULL, NULL, 		//sep., Clear, Puzzle, Digits, Pm, Dm
	NULL, "F12",						//sep., Propertis
//View
	NULL, "F4", "F3", "F2",				//View, More, Less, Reset
	NULL, "<control>T", "<control>S", "<control>L",	//sep., Toolbar, Statusbar, Palette
	NULL, "<control>G", "<control>H",	//sep., Grid, Grid5
//Puzzle
	NULL, "F9", "F10", NULL,			//Puzzle, Solve puzzle
										//Calculate digits, Make overlap
//Settings
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
//Library
	NULL, "F5", "F6", "<shift>F", NULL,
	"<shift>D",	"F7", "F8", NULL,		//Lib-Demo
	"<shift>O",
//Help
	NULL, "F1", NULL
};
static gint callback_actions[] = {
// File
	0, IDM_NEW, IDM_OPEN,				//File, New, Open
	0, IDM_SAVE, IDM_SAVE_AS,IDM_EXPORT,//sep., Save, Save As
	0, IDM_PRINT,						//sep., Print
	0, IDM_FEATURES,				//sep., Features
	0, IDM_QUIT,						//sep., Quit
//Edit
	0, IDM_UNDO, IDM_REDO,				//Edit, Undo, Redo
	0, IDM_INVERT, IDM_PASTE,			//sep., Invert, Paste
	0, IDM_ROTATE_LEFT, IDM_ROTATE_RIGHT,
					IDM_ROTATE_UP, IDM_ROTATE_DOWN,
										//Rotate, Left, Right, Up, Down
	0, IDM_CLEAR_PUZZLE, IDM_CLEAR_DIGITS,
					IDM_CLEAR_PUZ_MARK, IDM_CLEAR_DIG_MARK,
										//Clear, Puzzle, Digits, Pm, Dm
	0,  IDM_PROPERTIS,					// Propertis
//View
	0, IDM_MORE, IDM_LESS, IDM_NORMAL,	//View, More, Less, Reset
	0, IDM_TOOLBAR, IDM_STATUSBAR,
						IDM_PALETTE,	//sep., Toolbar, Statusbar, Palette
	0, IDM_GRID, IDM_GRID5,				//sep., Grid, Grid5
//Puzzle
	0, IDM_SOLVE, IDM_CALC_DIG,			//Puzzle, Solve puzzle
					IDM_MAKE_OVERLAPPED,//Calculate digits, Make overlap
//Settings
	0, IDM_ANIMATE, IDM_LESSON, IDM_VNAME, 0,
	IDM_HELP_0, IDM_HELP_1, IDM_HELP_2, IDM_HELP_3, IDM_HELP_4,
//Library
	0, IDM_LOAD_RANDOM, IDM_LOAD_RANDOM_C, IDM_SEARCH_IN_LIBRARY, 0,
	IDM_DEMO_START, IDM_LIBRARY_NEXT, IDM_LIBRARY_PREV, 0,
	IDM_HTTP_OPEN,
//Help
	0, IDM_HELP, IDM_ABOUT
};

static gchar *item_types[] = {
// File
	"<Branch>", "<StockItem>", "<StockItem>",	//File, New, Open
	"<Separator>", "<StockItem>", "<StockItem>",//sep., Save, Save As
										NULL,	//Export
	"<Separator>", "<StockItem>",				//sep., Print
	"<Separator>", "<StockItem>",				//sep., Features
	"<Separator>", "<StockItem>",				//sep., Quit
//Edit
	"<Branch>", "<StockItem>", "<StockItem>",	//Edit, Undo, Redo
	"<Separator>", NULL, "<StockItem>",			//sep., Invert, Paste
	"<Separator>", "<StockItem>", "<StockItem>",
				"<StockItem>", "<StockItem>",	//Rotate...
	"<Separator>", "<StockItem>", "<StockItem>",
				"<StockItem>", "<StockItem>",	//Clear...
	"<Separator>", "<StockItem>",				//sep., Propert
//View
	"<Branch>", "<StockItem>", "<StockItem>",
							   "<StockItem>",	//View, More, Less, Reset
	"<Separator>", "<CheckItem>",
				"<CheckItem>",	"<CheckItem>",	//sep., Toolbar, Statusb, Palette
	"<Separator>", "<CheckItem>",
						"<CheckItem>",			//sep., Grid, Grid5
//Puzzle
	"<Branch>", "<StockItem>", "<StockItem>",
							   "<StockItem>",	//Puzzle...
//Settings
	"<Branch>", "<CheckItem>", "<CheckItem>", "<CheckItem>",
	"<Title>", "<RadioItem>", "/Settings/manual solved", "/Settings/manual solved",
	"/Settings/manual solved", "/Settings/manual solved",
//Library
	"<Branch>", "<StockItem>", "<StockItem>", "<StockItem>",	//GetLib Rand Numb Demo_Begin
	"<Separator>","<StockItem>","<StockItem>","<StockItem>",
	"<Separator>", "<StockItem>",
//Help
	"<LastBranch>", "<StockItem>", "<StockItem>"
};
void *extra_datas[] = {
	NULL, GTK_STOCK_NEW, GTK_STOCK_OPEN,
	NULL, GTK_STOCK_SAVE, GTK_STOCK_SAVE_AS, NULL,
	NULL, GTK_STOCK_PRINT,
	NULL, GTK_STOCK_PREFERENCES,
	NULL, GTK_STOCK_QUIT,
	NULL, GTK_STOCK_UNDO, GTK_STOCK_REDO,
	NULL, GTK_STOCK_COPY, GTK_STOCK_PASTE,
	NULL, GTK_STOCK_GO_BACK, GTK_STOCK_GO_FORWARD, GTK_STOCK_GO_UP,
													GTK_STOCK_GO_DOWN,
	NULL, GTK_STOCK_CLEAR, GTK_STOCK_CLEAR, GTK_STOCK_CLEAR,
													GTK_STOCK_CLEAR,
	NULL, GTK_STOCK_PROPERTIES,
	NULL, GTK_STOCK_ZOOM_IN, GTK_STOCK_ZOOM_OUT, GTK_STOCK_ZOOM_FIT,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, GTK_STOCK_APPLY, GTK_STOCK_JUMP_TO, GTK_STOCK_ADD,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, GTK_STOCK_DIRECTORY, GTK_STOCK_SELECT_COLOR, GTK_STOCK_FIND, NULL,
	GTK_STOCK_MEDIA_PLAY, GTK_STOCK_GO_FORWARD, GTK_STOCK_GO_BACK,
	NULL, GTK_STOCK_NETWORK,
	NULL, GTK_STOCK_HELP, GTK_STOCK_ABOUT
};
// Создаю главное меню *************************************************
void set_main_menu( GtkWidget  *window, GtkWidget **menubar ){
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;
	gint count, nmenu_items = sizeof (callback_actions) / sizeof (gint);
	GtkItemFactoryEntry *menu_items = malloc(nmenu_items * sizeof(GtkItemFactoryEntry));
	//Создаю структуру меню
	for(count=0; count < nmenu_items; count++){
		menu_items[count].path				= _(paths[count]);
		menu_items[count].accelerator		= accelerators[count];
		menu_items[count].callback			= menu_func;
		menu_items[count].callback_action	= callback_actions[count];
		if(item_types[count] != NULL){
			menu_items[count].item_type		= _(item_types[count]);
			if(strstr(item_types[count], "Branch") != NULL)
				 menu_items[count].callback	= NULL;
			else if(strstr(item_types[count], "<StockItem>") !=NULL)
				menu_items[count].extra_data= extra_datas[count];
		}
		else menu_items[count].item_type	= NULL;
	}
	accel_group = gtk_accel_group_new ();
	item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);
	gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
	*menubar = gtk_item_factory_get_widget (item_factory, "<main>");
	free(menu_items);
}

// Создаю тулбар *******************************************************
void create_toolbar( GtkWidget  *a, GtkWidget **tool_bar ){
typedef struct toolbar_item_tag{
	gpointer	item;
	gint		id;
	char*		label;
	char*		ttips;
} toolbar_item;
	GtkWidget *iconw, *toolbar = gtk_toolbar_new();
	gchar *fbuf = g_malloc(512);
	uint i;
	GtkToolItem *item;
	GSList *group = NULL;

	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH);
	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_VERTICAL);
	gtk_handle_box_set_handle_position (GTK_HANDLE_BOX(handlebox), GTK_POS_TOP);// поводок детачборда делаю сверху
//	gtk_widget_set_size_request (handlebox, 32, 200);

	toolbar_item tool_list[] = {
		{GTK_STOCK_UNDO, IDT_UNDO, _("Undo"), _("undo last move")},
		{GTK_STOCK_REDO, IDT_REDO, _("Redo"), _("redo prev move")},
		{NULL, 0, NULL, NULL},
		{GTK_STOCK_EDIT, IDT_SOLVE, _("Solve"), _("to solve a nanogramm")},
		{NULL, IDT_CALC_DIG, _("Counts"), _("to count a matrix")},
		{GTK_STOCK_CLEAR, IDT_CLEAR_PUZZLE, _("Clear"), _("clear matrix")},
		{NULL, 0, NULL, NULL},
		{NULL, IDT_PEN_PAINT, _("Pen"), _("Painting pen")},
		{NULL, IDT_RECT_PAINT, _("Rectangle"), _("Painting rectangles")},
		{NULL, IDT_FILL_PAINT, _("Fill"), _("Filling place")},
		{GTK_STOCK_SELECT_COLOR, IDT_PALETTE, _("Palette"), _("Show palette")},
		{NULL, 0, NULL, NULL},
		{NULL, IDT_GRID, _("Grid"), _("Greed cells")},
		{NULL, IDT_GRID5, _("5x5"), _("5x bold grid")},
		{NULL, 0, NULL, NULL},
		{GTK_STOCK_NEW, IDT_NEW, _("New"), _("create new nanogramm")},
		{GTK_STOCK_OPEN,  IDT_OPEN, _("Open"), _("open nanogramm")},
		{GTK_STOCK_SAVE, IDT_SAVE, _("Save"), _("save nanogramm")}
		};

	uint count = sizeof(tool_list) / sizeof(toolbar_item);
	tb_count = 0;
	tb = malloc(count * sizeof(GtkWidget*));
	for(i=0; i<count; i++){
		int id = tool_list[i].id;
		bool toggled = (id==IDT_PALETTE || id==IDT_GRID || id==IDT_GRID5);
		bool radio = (id==IDT_RECT_PAINT || id==IDT_PEN_PAINT || id==IDT_FILL_PAINT);
		if(tool_list[i].label != NULL){
			if(toggled){
				if(tool_list[i].item)
					item = gtk_toggle_tool_button_new_from_stock(tool_list[i].item);
				else{	item = gtk_toggle_tool_button_new();
					sprintf(fbuf, "%s/tool%d.xpm", SHARE_PATH, id-IDT_GRID);
					iconw = gtk_image_new_from_file (fbuf);
					gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(item), iconw);
				}
			}
			else if(radio){
				item = gtk_radio_tool_button_new(group);
				group = gtk_radio_tool_button_get_group(GTK_RADIO_TOOL_BUTTON (item));
				sprintf(fbuf, "%s/tool%d.xpm", SHARE_PATH, id-IDT_PEN_PAINT+4);
				iconw = gtk_image_new_from_file (fbuf);
				gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(item), iconw);
			}
			else{
				if(tool_list[i].item)
						item = gtk_tool_button_new_from_stock(tool_list[i].item);
				else{	// сюда попадает кнопка "Расчитать оцифровку"
					sprintf(fbuf, "%s/tool%d.xpm", SHARE_PATH, IDT_CALC_DIG-IDT_BEGIN);
					iconw = gtk_image_new_from_file (fbuf);
					item = gtk_tool_button_new(iconw, tool_list[i].label);
				}
			}
			gtk_tool_item_set_tooltip_text (item, tool_list[i].ttips);
			gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, i);
			gtk_tool_button_set_label(GTK_TOOL_BUTTON(item), tool_list[i].label);
			g_signal_connect(G_OBJECT(item), toggled||radio ? "toggled" : "clicked",
					toggled||radio ? G_CALLBACK(toolbar_event) : G_CALLBACK(menu_func),
					(gpointer)(long)tool_list[i].id);
			tb[tb_count++] = GTK_WIDGET(item);
		}
		else if(tool_list[i].id == 0){
			item = gtk_separator_tool_item_new();
			gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, i);
		}
	}
	*tool_bar = toolbar;
	m_free(fbuf);
}

void enable_menu_item(int *ptr, int cnt, gboolean state){
	int i;
	GtkWidget* wig;
	GtkItemFactory *item_factory = gtk_item_factory_from_widget(menubar);

	for(i=0; i<cnt; i++){
		wig=gtk_item_factory_get_item_by_action(item_factory, ptr[i]);
		if(ptr[i] <= IDM_ABOUT) gtk_widget_set_sensitive(wig, state);
	}
}

void enable_toolbar_item(int *ptr, int cnt, gboolean state){
	for(int i=0; i<cnt; i++)
		if(!tool_bar_lock)	toolbar_save_state[i] = state;
		else	gtk_widget_set_sensitive(tb[ptr[i]-IDT_BEGIN], state);
}

void enable_toolbar_animate(bool state){		// блокировка тулбара пока идет анимация false - сохранить\отключить
	if(tool_bar_lock == state) return;			// триггер блокировки повторений
	tool_bar_lock = state;
	uint i;
	for(i=0; i<tb_count; i++) if(i != IDT_SOLVE-IDT_BEGIN){
		if(!state){							// сохраняю состояние тулбара
				toolbar_save_state[i] = gtk_widget_is_sensitive(tb[i]);
				gtk_widget_set_sensitive(tb[i], false);
		}
		else	gtk_widget_set_sensitive(tb[i], toolbar_save_state[i]);
	}
	GtkWidget* wig;
	GtkItemFactory *item_factory = gtk_item_factory_from_widget(menubar);
	for(i=IDM_BEGIN; i<=IDM_ABOUT; i++){
		wig=gtk_item_factory_get_item_by_action(item_factory, i);
		if(!state){
				menu_save_state[i-IDM_BEGIN] = gtk_widget_is_sensitive(wig);
				gtk_widget_set_sensitive(wig, false);
		}
		else 	gtk_widget_set_sensitive(wig, menu_save_state[i-IDM_BEGIN]);
	}
}
					// state		// why: digits, matrix, full
void toggle_changes(bool state, pr why){		// маркировка изменения в файле
	if(why == matrix && state == change_matrix) return;
	else if(why == digits && state == change_digits) return;
	if(why == matrix)		change_matrix = state;
	else if(why == digits)	change_digits = state;
	else					change_matrix = change_digits = state;

	state = change_matrix | change_digits;

	m_set_title();
	GtkItemFactory *item_factory = gtk_item_factory_from_widget(menubar);
	GtkWidget *wig = gtk_item_factory_get_item_by_action(item_factory, IDM_SAVE);
	gtk_widget_set_sensitive(wig, state);
	gtk_widget_set_sensitive(tb[IDT_SAVE-IDT_BEGIN], state);
}

const char *pname[]={"toolbar","statusbar","palette","grid","grid5",
							"animate","lesson","viewname","help_level","last_file"};
char *config_buf;
unsigned long pbuflen;

void save_parameter(){
	char *ptr = config_buf = MALLOC_BYTE(MAX_PATH * 2 + 10);
	for(param i=ptoolbar; i<=viewname; i++){
		sprintf(ptr, "%s = %s\n", pname[i], parameter[i] ? "true" : "false");
		ptr = config_buf + strlen(config_buf);
	}
	sprintf(ptr, "%s = %d\n", pname[help_lev], help_level);
	ptr = config_buf + strlen(config_buf);
	ftype = m_get_file_type(last_opened_file);
	if(ftype == mem || g_file_test(last_opened_file, G_FILE_TEST_EXISTS))
		sprintf(ptr, "%s = %s\n", pname[pfile], last_opened_file);
	ptr = config_buf + strlen(config_buf);
	FILE *ff = fopen(config_file, "w");
	if(ff != NULL){
		fwrite(config_buf, 1, strlen(config_buf), ff);
		fclose(ff);
	}
	else fprintf(stderr,_("Config file: %s opening error:(\n"), config_file);
}

char *get_parameter(param x){
	char *ptr = strstr(config_buf, pname[x]);
	if(!ptr) return NULL;
	ptr = strstr(ptr, "=");
	if(!ptr) return NULL;
	while(*ptr == '=' || *ptr == ' ') ptr++;
	int count=0;
	while(ptr[count] != '\n' && ptr[count] && count < (int)pbuflen)  count++;
	if(!count) return NULL;
	char *ret = malloc(count+1);
	for(int i=0; i<count; i++) ret[i]=ptr[i];
	ret[count]=0;
	return ret;
}

bool get_bool_parameter(param x){
	char *ptr = get_parameter(x);
	if(!ptr) return true;
	bool ret = (bool)strcmp(ptr, "false");
	m_free(ptr);
	return ret;
}

byte get_byte_parameter(param x){
	char *ptr = get_parameter(x);
	if(!ptr) return 0;
	return (byte)l_atoi(ptr);
}

void corrected_last_error(){
	// создаю файл-семафор нормального завершения работы программы
	char *buf = malloc(MAX_PATH);
	char mes[] = "1 fail! =(";
	sprintf(buf, "%s%s", config_path, LOCK_FILE);
	FILE *ff;
	if(g_file_test(buf, G_FILE_TEST_EXISTS)){
		ff = fopen(buf, "r");
		if(ff) fread(mes, 10, 1, ff);
		fclose(ff);
		byte a = mes[0] - '0';
		if(a > 3) *last_opened_file = '\0';		// предупреждаю открытие файла если была ошибка
		if(a > 5)
			if(xz(false)) puts("Decompressing done");
		mes[0]++;
		ff = fopen(buf, "w");
		if(ff) fwrite(mes, 1, 10, ff);
		fclose(ff);
	}
	else {
		ff = fopen(buf, "w");
		if(ff) fwrite(mes, 1, 10, ff);
		fclose(ff);
	}
	free(buf);
}

void load_parameter(){	// Установка сохранённых параметров
	GtkItemFactory *item_factory = gtk_item_factory_from_widget(menubar);
	GtkWidget *wig;
	FILE *ff = fopen(config_file, "r");
	parameter = malloc(10 * sizeof(gboolean));
	int i;
	cell_space = DEF_CELL_SPACE;				// размер ячейки
	demo_flag = false;	//tool  stat  palet grid  grid5 anima  lesson viewname
	gboolean table[10] = {true, true, true, true, true, true,  false, false};
	for(i=0; i<10; i++)	parameter[i] = table[i];

	if(ff != NULL){
		fseek(ff, 0, SEEK_END);	// читаю параметры в буфер
		pbuflen = ftell(ff);
		fseek(ff,0,SEEK_SET);
		config_buf = malloc(pbuflen+1);
		fread(config_buf, 1, pbuflen, ff);
		config_buf[pbuflen]=0;
		for(param i=ptoolbar; i<=viewname; i++) parameter[i] = get_bool_parameter(i);
		help_level = get_byte_parameter(help_lev);
		char *buf;
		if(!strlen(last_opened_file) || !g_file_test(last_opened_file, G_FILE_TEST_EXISTS)){
			buf = get_parameter(pfile);
			if(buf){
				strcpy(last_opened_file, buf);
				if(strstr(last_opened_file, "db/") == last_opened_file)
					last_opened_number = (int)m_get_base_number_file(last_opened_file);
				m_free(buf);
			}
			else strcpy(last_opened_file, home_dir);
		}
		m_free(config_buf);
		fclose(ff);
		}
	else {
		fprintf(stderr, _("Can't opening config file: %s\n"), config_file);
		system("xdg-mime default matrix.desktop application/matrix");	// привязываюсь к MIME
		strcpy(last_opened_file, home_dir);
	}

	wig = gtk_item_factory_get_item_by_action(item_factory,	IDM_TOOLBAR);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wig),parameter[ptoolbar]);

	wig = gtk_item_factory_get_item_by_action(item_factory,IDM_STATUSBAR);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wig),parameter[pstatusbar]);

	wig = gtk_item_factory_get_item_by_action(item_factory,	IDM_GRID);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wig),parameter[pgrid]);

	wig = gtk_item_factory_get_item_by_action(item_factory,	IDM_GRID5);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wig), parameter[pgrid5]);

	wig = gtk_item_factory_get_item_by_action(item_factory, IDM_ANIMATE);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wig),parameter[panimate]);

	wig = gtk_item_factory_get_item_by_action(item_factory, IDM_LESSON);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wig),parameter[plesson]);

	wig = gtk_item_factory_get_item_by_action(item_factory, IDM_VNAME);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wig),parameter[viewname]);

	wig = gtk_item_factory_get_item_by_action(item_factory, IDM_HELP_0 + help_level);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wig), true);

	int tools[] = {IDT_SAVE, IDT_UNDO, IDT_REDO, IDT_PALETTE};
	enable_toolbar_item(tools, 4, false);
	m_test_lib();
}

void set_window_size(bool maximaze){
	if(surface){		// удаляю поверхность рисования миниатюры
		cairo_surface_destroy (surface);
		surface = NULL;
	}
	if(cr) {cairo_destroy(cr); cr = NULL;}
// подгоняю размер окна при большом размере матрицы
	GdkScreen *screen = gdk_screen_get_default ();
	gint width_screen = gdk_screen_get_width (screen);
	gint height_screen = gdk_screen_get_height (screen);
	gint x_window_size	= cell_space * atom->x_puzzle_size - 6;
	gint y_window_size	= cell_space * atom->y_puzzle_size + non_area_y_size - 6;
	int x_space = cell_space, y_space = cell_space;

	if(maximaze || x_window_size > width_screen)
		x_space = width_screen / atom->x_puzzle_size;
	if(maximaze || y_window_size > height_screen)
		y_space = (height_screen - non_area_y_size) / atom->y_puzzle_size;
	cell_space = MIN(x_space, y_space);
	cell_space = MIN(cell_space, DEF_CELL_SPACE);
	x_window_size	= cell_space * atom->x_puzzle_size - 6;
	y_window_size	= cell_space * atom->y_puzzle_size + non_area_y_size - 6;
	gtk_window_resize((GtkWindow *)window, x_window_size, y_window_size);
	gtk_widget_queue_draw(draw_area);// обновляю draw_area
}

int main( int argc, char *argv[] ){
	// инициал. глобальных переменных
	atom = NULL;
	layout = NULL;
	surface = NULL;					// контекст рисования миниатюры
	palette = NULL;
	cr = NULL;
	ps = pen;
	thread_get_library = 0;
	last_opened_number = l_base_stack_top = 0;
	p_block_view_port_x = -1;
	c_solve_buffers = false;
	l_jch_container = false;

	char *lock_db = malloc(MAX_PATH);// удаляю возможно оставщийся файл блокировки библиотеки
	sprintf(lock_db, "%s/%s", config_path, "lock_db");
//	if(g_file_test(lock_db, G_FILE_TEST_EXISTS))
	remove(lock_db);
	free(lock_db);

	// инициал. генератора сл.чисел
	srand (time(NULL));
	// подключаю локализацию
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	textdomain (PACKAGE);

	gdk_threads_init();
    gdk_threads_enter();
	gtk_init (&argc, &argv);
	gdk_init (&argc, &argv);
	// инициал. libnotify
	notify_init(CLASS_NAME);

	// инициализация буферов путей и имён
	last_error = MALLOC_BYTE(2048);
	last_opened_file = MALLOC_BYTE(MAX_PATH);
	puzzle_path= MALLOC_BYTE(MAX_PATH);
	db_path = MALLOC_BYTE(MAX_PATH);
	puzzle_name = MALLOC_BYTE(MAX_PATH);
	home_dir = g_get_home_dir();
	config_path = malloc(strlen(home_dir)+sizeof(CONFIG_PATH)+1);
	config_file = malloc(strlen(home_dir)+sizeof(CONFIG_PATH)+sizeof(CONFIG_NAME)+1);
	sprintf(config_path, "%s%s", home_dir, CONFIG_PATH);
	sprintf(config_file, "%s%s%s", home_dir, CONFIG_PATH, CONFIG_NAME);
	sprintf(puzzle_path, "%s/%s", home_dir, WORK_NAME);
	sprintf(db_path, "%s%s/%s", home_dir, CONFIG_PATH, "db");
	if(!g_file_test(config_path, G_FILE_TEST_IS_DIR)) mkdir(config_path, 0766);
	if(!g_file_test(puzzle_path, G_FILE_TEST_IS_DIR)) mkdir(puzzle_path, 0766);

	GError	 *gerror = NULL;
	GdkPixbuf  *pixbuf = gdk_pixbuf_new_from_file (ICON_PATH, &gerror);
	sys_icon = gdk_pixbuf_scale_simple (pixbuf, ICON_SIZE, ICON_SIZE, GDK_INTERP_BILINEAR);
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW(window), CAPTION_TEXT);
	gtk_window_set_default_icon (sys_icon);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	vbox		= gtk_vbox_new (false, 1);
	hbox		= gtk_hbox_new (false, 0);
	draw_area	= gtk_drawing_area_new();
	statusbar	= gtk_statusbar_new ();
	scroll_win	= gtk_scrolled_window_new(NULL, NULL);
	handlebox	= gtk_handle_box_new ();
	gtk_scrolled_window_set_policy((GtkScrolledWindow*)scroll_win,
							GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_set_border_width (GTK_CONTAINER (window), BORDER_WIDTH);

	set_main_menu (window, &menubar);
    gtk_widget_realize(window);
	create_toolbar(window, &toolbar);
	// добавляю события для draw_area
	gtk_widget_set_events (draw_area, gtk_widget_get_events (draw_area)
			| GDK_EXPOSURE_MASK
			| GDK_LEAVE_NOTIFY_MASK
			| GDK_BUTTON_PRESS_MASK
			| GDK_BUTTON_RELEASE_MASK
			| GDK_POINTER_MOTION_MASK
			| GDK_POINTER_MOTION_HINT_MASK);
	gtk_container_add (GTK_CONTAINER (window), vbox);
	gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(vbox), hbox, true, true, 0);		//верт ящик
		gtk_box_pack_start (GTK_BOX(hbox), handlebox, FALSE, FALSE, 0);	//тулбар
			gtk_container_add (GTK_CONTAINER (handlebox), toolbar);
		gtk_box_pack_start (GTK_BOX (hbox), scroll_win, TRUE, TRUE, 0);	//скроллбар
			gtk_scrolled_window_add_with_viewport((GtkScrolledWindow *)scroll_win, draw_area);
    gtk_box_pack_end (GTK_BOX (vbox), statusbar, FALSE, TRUE, 0);	//статусбар

	gtk_signal_connect (GTK_OBJECT (window), "delete-event", GTK_SIGNAL_FUNC (posix_abort_signal), NULL);
//	g_signal_connect (window, "destroy", G_CALLBACK (quit_program), NULL);

	//сигнал нажатия клавиш в области рисования
	g_signal_connect(window, "key_press_event",
									G_CALLBACK(&key_press),NULL);
	//установка сигнала нажатия кнопки мыши на виджет области рисования
	g_signal_connect(draw_area, "button-press-event",
									G_CALLBACK(&button_press), NULL);
	g_signal_connect(draw_area, "button-release-event",
									G_CALLBACK(&button_release), NULL);
	g_signal_connect(draw_area, "scroll-event",
									G_CALLBACK(&scroll_event), NULL);
	//установка сигнала движения мыши в области рисования
	g_signal_connect(draw_area, "motion_notify_event",
									G_CALLBACK(&p_mouse_motion), NULL);
	g_signal_connect(draw_area, "expose_event",
									G_CALLBACK(&p_expose), NULL);
	set_status_bar("Program starting");
	load_parameter();			// создаю структуру пазла и читаю параметры
	l_list_mfree(true);			// инициализирую переменные библиотеки

	l_lib_lock(true);
	if(!l_base_load_from_file(true))// загружаю библиотеку
		l_get_library(false);		// и подгружаю обновления с сайта
	l_lib_lock(false);

	u_init();					// инициализирую undo-redo
	c_reset_puzzle_pointers();	// инициализация решалки

	GtkAllocation allocation;
	gtk_widget_get_allocation (menubar, &allocation);
	non_area_y_size = allocation.height;
	gtk_widget_get_allocation (toolbar, &allocation);
	non_area_y_size += allocation.height;
	gtk_widget_get_allocation (statusbar, &allocation);
	non_area_y_size += allocation.height;
	corrected_last_error();		// проверка нормального завершения предыдущего сеанса
	if(argc > 1 && g_file_test(argv[1], G_FILE_TEST_EXISTS)){
		strcpy(last_opened_file, argv[1]);
		place = exfile;
		m_open_puzzle(false, false, NULL);
	}
	else if(!*last_opened_file){// открываю файл пазла
		place = memory;
		m_get_puzzle_default();
	}
	else if(last_opened_number){
		jch_arch_item *o = l_get_item_from_base(last_opened_number);
		place = memory;
		if(o) m_open_puzzle(false, false, o->body);
		else m_get_puzzle_default();
	}
	else {
		place = exfile;
		m_open_puzzle(false, false, NULL);
	}
	gtk_widget_show_all(window);
	signal(SIGSEGV, posix_death_signal); // обработчик исключений SIGSEGV
//	signal(SIGABRT, posix_abort_signal); // обработчик исключений SIGABRT
	gtk_main();

	gdk_threads_leave();
	u_state_free();
	notify_uninit();
	free(tb);
	free(last_error);
	free(last_opened_file);
	free(puzzle_path);
	free(db_path);
	free(puzzle_name);
	free(config_path);
	free(config_file);
	free(parameter);
	return(0);
}
