#include <assert.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include "chess/board.h"
#include "chess/game.h"
#include "chess/pgn.h"
#include "board_display.h"

void set_up_gui();

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

	if (argc > 1) {
		// If we get any args, we take them to be a PGN file to open
		char *pgn_filename = argv[1];
		PGN pgn;

		if (!read_pgn(&pgn, pgn_filename, &err)) {
			fprintf(stderr, "Failed to load PGN file '%s'", pgn_filename);
			if (err == NULL)
				putc('\n', stderr);
			else
				fprintf(stderr, ":\n%s", err->message);

			return 1;
		}

		current_game = pgn.game;
	} else {
		current_game = new_game();
		current_game->board = malloc(sizeof(Board));

		bool success = from_fen(current_game->board, start_board_fen);
		// This is a fixed string, it should never fail to be parsed
		assert(success);
	}

	set_up_gui();

	gtk_main();

	return 0;
}

void set_up_gui()
{
	GtkBuilder *builder = gtk_builder_new_from_file("chessboard.ui");

	GObject *window = gtk_builder_get_object(builder, "main_window");
	g_signal_connect(window, "destroy",
			G_CALLBACK(gtk_main_quit), NULL);

	GObject *open_pgn_item = gtk_builder_get_object(builder, "open_pgn_menu_item");
	g_signal_connect(open_pgn_item, "activate",
			G_CALLBACK(open_pgn_callback), window);

	GObject *flip_button = gtk_builder_get_object(builder, "flip_board_button");
	g_signal_connect(flip_button, "clicked",
			G_CALLBACK(flip_button_click_callback), NULL);

	GObject *go_end_button = gtk_builder_get_object(builder, "go_end_button");
	g_signal_connect(go_end_button, "clicked",
			G_CALLBACK(go_end_button_click_callback), NULL);

	go_next_button = GTK_WIDGET(gtk_builder_get_object(builder, "go_next_button"));
	g_signal_connect(G_OBJECT(go_next_button), "clicked",
			G_CALLBACK(go_next_button_click_callback), NULL);

	go_back_button = GTK_WIDGET(gtk_builder_get_object(builder, "go_back_button"));
	g_signal_connect(G_OBJECT(go_back_button), "clicked",
			G_CALLBACK(go_back_button_click_callback), NULL);

	GObject *go_start_button = gtk_builder_get_object(builder, "go_start_button");
	g_signal_connect(go_start_button, "clicked",
			G_CALLBACK(go_start_button_click_callback), NULL);

	board_display = GTK_WIDGET(gtk_builder_get_object(builder, "board_drawing_area"));
	gtk_widget_add_events(board_display,
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
			GDK_POINTER_MOTION_MASK);
	g_signal_connect(G_OBJECT(board_display), "draw",
			G_CALLBACK(board_draw_callback), NULL);
	g_signal_connect(G_OBJECT(board_display), "button-press-event",
			G_CALLBACK(board_mouse_down_callback), NULL);
	g_signal_connect(G_OBJECT(board_display), "button-release-event",
			G_CALLBACK(board_mouse_up_callback), NULL);
	g_signal_connect(G_OBJECT(board_display), "motion-notify-event",
			G_CALLBACK(board_mouse_move_callback), NULL);

	gtk_widget_show_all(GTK_WIDGET(window));

	// If we loaded a PGN file, we need to enable the forward button
	set_button_sensitivity();
}
