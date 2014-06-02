#include <gtk/gtk.h>
#include <stdio.h>
#include "board.h"
#include "board_display.h"
#include "game.h"

char *piece_svgs = "pieces/merida/";

int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);

	GError *err = NULL;
	load_svgs(piece_svgs, &err);
	if (err != NULL) {
		printf("Error loading SVGs:\n%s\n", err->message);
		return 1;
	}

	char *fen;
	if (argc > 1) {
		fen = argv[1];
		puts(fen);
	} else {
		fen = start_board_fen;
	}

	if (!from_fen(&current_board, fen)) {
		printf("Couldn't parse given FEN string:\n%s\n", fen);
		return 1;
	}

	game_root = new_game();
	current_game = game_root;

	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), PROG_NAME);
	gtk_window_set_default_size(GTK_WINDOW(window),
			DEFAULT_SQUARE_SIZE * BOARD_SIZE,
			DEFAULT_SQUARE_SIZE * BOARD_SIZE);
	g_signal_connect(G_OBJECT(window), "destroy",
			G_CALLBACK(gtk_main_quit), NULL);

	GtkWidget *menu_bar = gtk_menu_bar_new();

	GtkWidget *file_menu = gtk_menu_new();
	GtkWidget *file_item = gtk_menu_item_new_with_label("File");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
	GtkWidget *open_item = gtk_menu_item_new_with_label("Open");
	GtkWidget *save_item = gtk_menu_item_new_with_label("Save");
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open_item);
	gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_item);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_item);

	GtkWidget *drawing_area = gtk_drawing_area_new();
	gint width, height;
	gtk_window_get_size(GTK_WINDOW(window), &width, &height);
	gtk_widget_set_size_request(drawing_area, width, height);
	gtk_widget_add_events(drawing_area,
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
			GDK_POINTER_MOTION_MASK);
	g_signal_connect(G_OBJECT(drawing_area), "draw",
			G_CALLBACK(board_draw_callback), &current_board);
	g_signal_connect(G_OBJECT(drawing_area), "button-press-event",
			G_CALLBACK(board_mouse_down_callback), &current_board);
	g_signal_connect(G_OBJECT(drawing_area), "button-release-event",
			G_CALLBACK(board_mouse_up_callback), &current_board);
	g_signal_connect(G_OBJECT(drawing_area), "motion-notify-event",
			G_CALLBACK(board_mouse_move_callback), NULL);

	GtkWidget *tool_bar = gtk_toolbar_new();
	GtkToolItem *back_button = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
	gtk_toolbar_insert(GTK_TOOLBAR(tool_bar), back_button, 0);

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
	gtk_container_add(GTK_CONTAINER(window), box);

	gtk_container_add(GTK_CONTAINER(box), menu_bar);
	gtk_container_add(GTK_CONTAINER(box), tool_bar);
	gtk_container_add(GTK_CONTAINER(box), drawing_area);
	gtk_box_set_child_packing(GTK_BOX(box), drawing_area, TRUE, TRUE, 0, GTK_PACK_START);
	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}
