// dialog.c

#include <libnotify/notify.h>

#include "dialog.h"
#include "library.h"

extern const char *color_name[];
size_t c_size = 25;
size_t c_space = 1;
GtkWidget  *color_draw = NULL;
void d_create_palette();

//************************** инверсия палитры **************************//
void d_invert_palette(){	// поиск инверсного цвета
	for(int i = 2; i < MAX_COLOR + 2; i++){
		atom->rgb_color_table[i].r = 255 - atom->rgb_color_table[i].r;	//30% * red
		atom->rgb_color_table[i].g = 255 - atom->rgb_color_table[i].g;	//59% * green
		atom->rgb_color_table[i].b = 255 - atom->rgb_color_table[i].b;	//11% * blue*/
	}
	if(palette_show)
		gtk_widget_queue_draw(palette);
}

void d_update_palette(){
	if(palette_show)
		gtk_widget_queue_draw(palette);
}

bool choose_color(){
	if(ftype == jcr || current_color == WHITE || current_color == BLACK) return false;
	GdkColor color;
	RGB c = atom->rgb_color_table[current_color];
	color.red	= (c.r << 8) + 127;
	color.green	= (c.g << 8) + 127;
	color.blue	= (c.b << 8) + 127;
	GtkWidget *w = gtk_color_selection_dialog_new(_("Change current color"));
	GtkColorSelection *colorsel = GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(w)->colorsel);
	gtk_color_selection_set_current_color(colorsel, &color);
	gint response = gtk_dialog_run (GTK_DIALOG (w));
	if (response == GTK_RESPONSE_OK){
		gtk_color_selection_get_current_color (colorsel, &color);
		atom->rgb_color_table[current_color].r = color.red >> 8;
		atom->rgb_color_table[current_color].g = color.green >> 8;
		atom->rgb_color_table[current_color].b = color.blue >> 8;
		gtk_widget_destroy (GTK_WIDGET(w));
		return true;
	}
	gtk_widget_destroy (GTK_WIDGET(w));
	return false;
}

gboolean d_button_press_pal (GtkWidget *widget, GdkEventButton *event, gpointer user_data){
	if(widget != palette){
		int x = (size_t) event->x;
		int y = (size_t) event->y;
		int block = c_size + c_space;
		x -= block * 2;
		if(event->type==GDK_2BUTTON_PRESS || x < 0){
			if(ftype == jcr){
				if(x < 0) return false;
				d_message(_("choose color"), _("The format JCR doesn't support change of  colors"), false);
				return false;
			}
			if(choose_color())
				gtk_widget_queue_draw(draw_area);
		}
		else {
			x /= block;
			y /= block;
			current_color = x + (y % 2) * (MAX_COLOR/2);
		}
		gtk_widget_queue_draw(widget);
		return true;
	}
	else {
		return true;
	}
	return false;
}

bool d_expose_color(GtkWidget *widget, GdkEventExpose *event, gpointer data){
	int a;
	size_t x, y, half = MAX_COLOR / 2;
	cairo_t *cr = gdk_cairo_create(widget->window);

	SET_COLOR(current_color);
	cairo_rectangle(cr, 0, 0, c_size * 2 + c_space, c_size * 2 + c_space);
	cairo_fill(cr);

	int color = -1;
	RGB r = atom->rgb_color_table[current_color];
	for(a=0; a<20; a++)
		if(color_table[a].r == r.r && color_table[a].g == r.g && color_table[a].b == r.b) color = a;

	char clr_name[]={"        "};
	clr_name[0]=r.r>>4;	clr_name[1]=r.r&0xf;
	clr_name[3]=r.g>>4; clr_name[4]=r.g&0xf;
	clr_name[6]=r.b>>4; clr_name[7]=r.b&0xf;
	for(a=0; a<8; a++) if(clr_name[a]!=' ')
		clr_name[a] += (clr_name[a]<10 ? '0' : 'A'-10);

	if(!layout) layout = pango_cairo_create_layout(cr);
	SET_COLOR(contrast_color(current_color));
	pango_layout_set_font_description(layout,font_c);
	if(color >=0 )
		 pango_layout_set_text(layout, _(color_name[color]), strlen(_(color_name[color])));
	else pango_layout_set_text(layout, clr_name, 8);
	cairo_move_to(cr, 0, 0);
	pango_cairo_show_layout(cr,layout);
	for(a = 0; a < MAX_COLOR; a++){
		y = (a < half ? 0 : c_size + c_space);
		x = (a % half + 2) * (c_size + c_space);
		SET_COLOR(a);
		cairo_rectangle(cr, x, y, c_size, c_size);
		cairo_fill(cr);
	}
	cairo_stroke(cr);
	cairo_destroy(cr);

	return false;
}

void d_palette_show(bool show){
	if(!atom) return;
	if(!atom->isColor){
		gtk_widget_hide (palette);
		current_color = BLACK;
		return;
	}
	if(!color_draw)	d_create_palette();
	else if(show){
		gtk_widget_show (palette);
		palette_show = true;
	}
	else {
		gtk_widget_hide (palette);
		palette_show = false;
	}
}

static gboolean d_hide_palette(GtkWidget *a, void *b, void *c){
	palette_show = false;
	d_palette_show(false);
	GtkItemFactory *item_factory = gtk_item_factory_from_widget(menubar);
	GtkWidget *wig = gtk_item_factory_get_item_by_action(item_factory,IDM_PALETTE);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wig), false);
	return true;
}

void d_create_palette(){
	color_draw = gtk_drawing_area_new();
	palette	= (GtkWidget*)gtk_window_new (GTK_WINDOW_TOPLEVEL);

	gtk_window_set_keep_above((GtkWindow*)palette, true);
	gtk_window_set_position((GtkWindow *)palette, GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_set_resizable ((GtkWindow*)palette, false);
	gtk_widget_set_size_request(palette, (MAX_COLOR/2+3)*c_size*0.98, (c_size+c_space)*2.2);
	gtk_container_set_border_width (GTK_CONTAINER (palette), 3);
	gtk_window_set_title (GTK_WINDOW(palette), _("Palette"));
	gtk_container_add (GTK_CONTAINER (palette), color_draw);
	gtk_widget_set_events (color_draw, gtk_widget_get_events(color_draw)
			| GDK_EXPOSURE_MASK
			| GDK_LEAVE_NOTIFY_MASK
			| GDK_BUTTON_PRESS_MASK
			| GDK_BUTTON_RELEASE_MASK
			| GDK_POINTER_MOTION_MASK
			| GDK_POINTER_MOTION_HINT_MASK);

	g_signal_connect(color_draw, "button-press-event",
									G_CALLBACK(&d_button_press_pal), NULL);

	gtk_signal_connect (GTK_OBJECT (palette), "destroy",
						GTK_SIGNAL_FUNC (d_hide_palette), &palette);
	gtk_signal_connect (GTK_OBJECT (palette), "delete-event",
						GTK_SIGNAL_FUNC (d_hide_palette), &palette);
	gtk_signal_connect (GTK_OBJECT (palette), "delete-event",
						GTK_SIGNAL_FUNC (d_hide_palette), &palette);

	g_signal_connect(color_draw, "expose_event",
									G_CALLBACK(&d_expose_color), NULL);
	char font_name[] = "sans normal 7";
	font_c = pango_font_description_from_string(font_name);

	gtk_widget_show (color_draw);
}

bool d_new_puzzle_dialog(puzzle_features *pf, bool color_checkbox){

	GtkWidget  *dialog, *box, *entry[2], *label[6], *spin[4], *isColor;
	box  =	gtk_table_new(9, 2, FALSE);

	gtk_table_set_row_spacing (GTK_TABLE(box), 0, 5);
	gtk_table_set_row_spacing (GTK_TABLE(box), 1, 5);
	gtk_table_set_row_spacing (GTK_TABLE(box), 2, 5);
	gtk_table_set_row_spacing (GTK_TABLE(box), 3, 20);
	gtk_table_set_row_spacing (GTK_TABLE(box), 4, 5);
	gtk_table_set_row_spacing (GTK_TABLE(box), 5, 20);
	gtk_table_set_row_spacing (GTK_TABLE(box), 6, 5);

	gtk_table_set_col_spacing (GTK_TABLE(box), 0, 20);

	label[0]=gtk_label_new(_("Top digits number"));
	label[1]=gtk_label_new(_("Left digits number"));
	label[2]=gtk_label_new(_("Height matrix"));
	label[3]=gtk_label_new(_("Width matrix"));
	label[4]=gtk_label_new(_("File name:"));
	label[5]=gtk_label_new(_("Puzzle name:"));
	entry[0]=gtk_entry_new ();
	entry[1]=gtk_entry_new ();

	if(color_checkbox)
		isColor=gtk_check_button_new_with_label(_("Color puzzle"));
	spin[0]=gtk_spin_button_new_with_range(4,40,1);	//спин-боксы выбора размеров оцифровки и матрицы
	spin[1]=gtk_spin_button_new_with_range(4,40,1);
	spin[2]=gtk_spin_button_new_with_range(8,99,1);
	spin[3]=gtk_spin_button_new_with_range(8,99,1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin[0]), pf->top_dig_size);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin[1]), pf->left_dig_size);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin[2]), pf->y_size);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin[3]), pf->x_size);
	gtk_entry_set_text(GTK_ENTRY(entry[0]), pf->fname);
	gtk_entry_set_text(GTK_ENTRY(entry[1]), pf->pname);
	if(color_checkbox)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(isColor), pf->isColor);

	gtk_table_attach_defaults(GTK_TABLE(box), label[4], 0, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(box), entry[0], 0, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(box), label[5], 0, 2, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(box), entry[1], 0, 2, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(box), label[0], 0, 1, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(box), spin[0],  0, 1, 5, 6);
	gtk_table_attach_defaults(GTK_TABLE(box), label[1], 0, 1, 6, 7);
	gtk_table_attach_defaults(GTK_TABLE(box), spin[1],  0, 1, 7, 8);
	gtk_table_attach_defaults(GTK_TABLE(box), label[2], 1, 2, 4, 5);
	gtk_table_attach_defaults(GTK_TABLE(box), spin[2],  1, 2, 5, 6);
	gtk_table_attach_defaults(GTK_TABLE(box), label[3], 1, 2, 6, 7);
	gtk_table_attach_defaults(GTK_TABLE(box), spin[3],  1, 2, 7, 8);
	if(color_checkbox)
		gtk_table_attach_defaults(GTK_TABLE(box), isColor, 0, 2, 8, 9);


	dialog=gtk_dialog_new_with_buttons (pf->header, GTK_WINDOW(window),
				GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
	GtkWidget *dialog_array=gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_box_pack_start(GTK_BOX(dialog_array), box, TRUE, TRUE, 0);
	gtk_widget_show_all(dialog);
	gint result = gtk_dialog_run(GTK_DIALOG(dialog));
	if(result == GTK_RESPONSE_ACCEPT){
		pf->top_dig_size = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spin[0]));
		pf->left_dig_size = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spin[1]));
		pf->y_size = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spin[2]));
		pf->x_size = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(spin[3]));
		if(color_checkbox)
			pf->isColor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(isColor));
		const char *r = gtk_entry_get_text(GTK_ENTRY(entry[0]));
		if(strlen(r)) strcpy(last_opened_file, r);
		r = gtk_entry_get_text(GTK_ENTRY(entry[1]));
		if(strlen(r)) strcpy(puzzle_name, r);
		gtk_widget_destroy (GTK_WIDGET(dialog));
		return true;
	}
	gtk_widget_destroy (GTK_WIDGET(dialog));
	return false;
}

char *d_find_puzzle_dialog(uint p){
	if(!l_base_stack_top) {
		d_message(_("Sorry"), _("Library is empty"), false);
		return NULL;
	}
	GtkWidget  *dialog, *box, *label, *entry;
	box  =	gtk_table_new(2, 1, FALSE);

	gtk_table_set_row_spacing (GTK_TABLE(box), 0, 5);
	gtk_table_set_row_spacing (GTK_TABLE(box), 1, 5);
	uint max = l_base_stack_top - 1;
	char *s = malloc(1024);
	sprintf(s, "%s (%s 1 - %d)", _("Select number or name puzzle:"), _("numbers are locally available"), max);
	label=gtk_label_new(s);

	if(p){
		if(!l_get_name_from_item(s, l_get_item_from_base(p)->body))
			sprintf(s, "%d", p);
		}
	else strcpy(s, "enter find text");
	entry=gtk_entry_new ();
	gtk_entry_set_text(GTK_ENTRY(entry), s);

	gtk_table_attach_defaults(GTK_TABLE(box), label, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(box), entry,  0, 1, 1, 2);

	dialog=gtk_dialog_new_with_buttons (_("Search in library"), GTK_WINDOW(window),
				GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);
	GtkWidget *dialog_array=gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_box_pack_start(GTK_BOX(dialog_array), box, TRUE, TRUE, 0);
	gtk_widget_show_all(dialog);
	gint result = gtk_dialog_run(GTK_DIALOG(dialog));
	*s = 0;
	if(result == GTK_RESPONSE_ACCEPT){
		const char *r = gtk_entry_get_text(GTK_ENTRY(entry));
		if(strlen(r)) strcpy(s, r);
	}
	gtk_widget_destroy (GTK_WIDGET(dialog));
	if(!*s) {free(s); return NULL;}
	return s;
}

void d_about_dialog (){
	gchar	  *website = "https://vk.com/id11693344";			//диалог "о программе"
	gchar	  *website_label = _("My page vkontakte");
	gchar	  *authors[] = {"Vlad Terechov <asgrem@gmail.com>", NULL};
	gtk_show_about_dialog (NULL,
							"name", _("About"),
							"program-name", CAPTION_TEXT,
							"version", VERSION,
							"copyright", "\xC2\xA9 2012-2019 Terraslav™ (GPL)",
							"comments", _("Japaness nanogramm manipulate program"),
							"authors", authors,
							"website", website,
							"website-label", website_label,
							"logo", sys_icon,
							NULL);
	return;
}

int d_message(char *caption, char *mess, int buttons){
	if(demo_flag) return false;
	if(!buttons){
		NotifyNotification* nn = notify_notification_new(caption,mess,0);
		notify_notification_set_timeout(nn, 10000);
		if (notify_notification_show(nn, 0))
			return true;
	}
	GtkWidget *hbox, *dialog, *dialog_array, *lb;

	if(buttons == 1) dialog=gtk_dialog_new_with_buttons (caption, GTK_WINDOW(window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_YES, GTK_RESPONSE_YES, GTK_STOCK_NO, GTK_RESPONSE_NO,
		NULL);
	else if(buttons > 1)dialog=gtk_dialog_new_with_buttons (caption, GTK_WINDOW(window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_YES, GTK_RESPONSE_YES, GTK_STOCK_NO, GTK_RESPONSE_NO,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL);
	else dialog=gtk_dialog_new_with_buttons (caption, GTK_WINDOW(window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		NULL);

	gtk_container_set_border_width (GTK_CONTAINER (dialog), 10);

	hbox = gtk_hbox_new(false, 20);
	dialog_array=gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_box_pack_start(GTK_BOX(dialog_array), hbox, true, true, 0);
	gtk_widget_show(hbox);

	lb=gtk_label_new(mess);
	gtk_box_pack_start(GTK_BOX (hbox), lb, true, true, 0);
	gtk_widget_show(lb);

	gint result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy (GTK_WIDGET(dialog));

	if(result==GTK_RESPONSE_YES) return 1;
	else if(result==GTK_RESPONSE_CANCEL) return -1;
	return 0;
}

void d_gtk_my_patter_add(GtkWidget *filechooser, const gchar *title, const gchar *pattern) {
	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, title);
	gtk_file_filter_add_pattern(filter, pattern);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(filechooser), filter);
}

bool d_save_as_dialog(){
	char *pth = NULL, *nam = NULL;
	dialog = gtk_file_chooser_dialog_new (_("Save nanogramm as"),
						(GtkWindow *)window,
						GTK_FILE_CHOOSER_ACTION_SAVE,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
						NULL);
	gtk_file_chooser_set_do_overwrite_confirmation (
										GTK_FILE_CHOOSER (dialog), TRUE);
	if(last_opened_file){
		pth			= m_get_file_path_name(TRUE);
		nam			= m_get_file_path_name(FALSE);
		if(!pth || !strcmp(pth, "db")){
					pth = malloc(MAX_PATH);
					sprintf(pth, "%s/%s", home_dir, WORK_NAME);
					gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), pth);
		}
		else		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), pth);
		if(!nam)	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), NEW_FILE_NAME);
		else		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), nam);
		m_free(pth); m_free(nam);
	}
	else
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog),
														NEW_FILE_NAME);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT){
		pth = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		strcpy(last_opened_file, pth);
		m_free(pth);
		gtk_widget_destroy (dialog);
		return true;
	}
	gtk_widget_destroy (dialog);
	return false;
}

bool d_open_file_dialog(){
	if(0 > m_save_puzzle(true))	return false;
	char *buf = malloc(MAX_PATH);;
	dialog = gtk_file_chooser_dialog_new (_("Open nanogramm"), (GtkWindow *)window,
						GTK_FILE_CHOOSER_ACTION_OPEN,
						GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
						NULL);
	d_gtk_my_patter_add(dialog, WORK_NAME, "*.[j,J,m,M][c,C,e,E][r,R,c,C,h,H,m,M]");
	d_gtk_my_patter_add(dialog, _("Pictures(BMP) non-compessed"), "*.[b,B][m,M][p,P]");
	d_gtk_my_patter_add(dialog, _("All files"), "*");

	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), puzzle_path);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_ACCEPT){
		gtk_widget_destroy (dialog);
		return false;
	}
	buf = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	gtk_widget_destroy (dialog);
	if(g_file_test(buf, G_FILE_TEST_EXISTS) && atom){
		if(strcmp(buf, last_opened_file)){
			if((change_digits || change_matrix) && l_jch_container) l_close_jch();
		}
		else{
			m_free(buf);
			return false;
		}
	}
	strcpy(last_opened_file, buf);
	m_free(buf);
	return true;
}

/****************** Tree View ***************************/
enum{
	COL_NUMBER = 0,
	COL_NAME,
	COL_SIZE,
	COL_COL,
	NUM_COLS
};

size_t select_item;
static GtkTreeModel *create_and_fill_model(size_t *nums, size_t count){
	GtkListStore  *store;
	GtkTreeIter    iter;
	size_t i;
	jch_arch_item *item;
	char numb[10];
	char name[256];
	char size[10];
	char colr[40];
	store = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	for(i = 0; i < count; i++){
		if(nums[i] == last_opened_number) {select_item = i; printf("%d", (int)i);}
		item = l_get_item_from_base(nums[i]);
		sprintf(numb, "%d", (int)nums[i]);
		if(l_get_name_from_item(name, item->body) 	&&
		l_get_size_from_item(size, item->body)		&&
		l_get_colr_from_item(colr, item->body)){
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, COL_NUMBER, numb,
				COL_NAME, name, COL_SIZE, size, COL_COL, colr, -1);
			}
	}
	return GTK_TREE_MODEL (store);
}

static GtkWidget *create_view_and_model (size_t *nums, size_t count){
	GtkCellRenderer		*renderer;
	GtkTreeModel		*model;
	GtkTreeView			*view;
	view = GTK_TREE_VIEW (gtk_tree_view_new ());
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (view, -1, _("Number"),
										renderer, "text", COL_NUMBER, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (view, -1, _("Name"),
										renderer, "text", COL_NAME, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (view, -1, _("Size"),
										renderer, "text", COL_SIZE, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (view, -1, _("Numb.colors"),
										renderer, "text", COL_COL, NULL);
	model = create_and_fill_model (nums, count);
	gtk_tree_view_set_model (view, model);

	gtk_tree_view_set_cursor (view, gtk_tree_path_new_from_indices (select_item, -1), NULL, true);
	g_object_unref (model);
	return GTK_WIDGET(view);
}

void view_onRowActivated(GtkTreeView *treeview, GtkTreePath *path,
					GtkTreeViewColumn *col, gpointer userdata){
	GtkTreeModel *model;
	GtkTreeIter   iter;
	model = gtk_tree_view_get_model(treeview);
	if (gtk_tree_model_get_iter(model, &iter, path)){
		gchar *name;
		gtk_tree_model_get(model, &iter, 0, &name, -1);
		place = memory;
		m_open_puzzle(false, false, (l_get_item_from_base(l_atoi(name)))->body);
		g_free(name);
	}
}

int d_view_list (size_t *nums, size_t count){
	GtkWidget *wnd;
	GtkWidget *view;
	wnd = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (wnd, "delete_event", gtk_main_quit, NULL); /* dirty */
	view = create_view_and_model(nums, count);
	gtk_container_add (GTK_CONTAINER (wnd), view);
	g_signal_connect(view, "row-activated", (GCallback) view_onRowActivated, NULL);
	gtk_widget_show_all(wnd);
	gtk_main ();
	return 0;
}
