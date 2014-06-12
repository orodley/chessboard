#include <gtk/gtk.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "board_display.h"
#include "misc.h"
#include "moves.h"
#include "pgn.h"

RsvgHandle *piece_images[2][6];

void load_svgs(char *dir, GError **err)
{
	uint len = strlen(dir) + 6; // piece letter + ".svg\0"
	char str[len];
	char *piece_letters[] = { "pnbrqk", "PNBRQK" };

	for (uint i = 0; i < 2; i++) {
		for (uint j = 0; piece_letters[i][j] != '\0'; j++) {
			sprintf(str, "%s%c.svg", dir, piece_letters[i][j]);

			piece_images[i][j] = rsvg_handle_new_from_file(str, err);
			if (*err != NULL)
				return;
		}
	}
}

uint get_square_size(GtkWidget *board)
{
	guint width  = gtk_widget_get_allocated_width(board);
	guint height = gtk_widget_get_allocated_height(board);

	uint max_square_width = width / BOARD_SIZE;
	uint max_square_height = height / BOARD_SIZE;
	return max_square_width < max_square_height ?
		max_square_width :
		max_square_height;
}

Game *current_game;
GtkWidget *board_display;
GtkWidget *back_button;
GtkWidget *forward_button;


static Square drag_source = NULL_SQUARE;
static uint mouse_x;
static uint mouse_y;

void draw_piece(cairo_t *cr, Piece p, uint size)
{
	RsvgHandle *piece_image =
		piece_images[PLAYER(p)][PIECE_TYPE(p) - 1];

	// 0.025 is a bit of a magic number. It's basically just the amount by
	// which the pieces must be scaled in order to fit correctly with the
	// default square size. We then scale that based on how big the squares
	// actually are.
	double scale = 0.025 * size / DEFAULT_SQUARE_SIZE;
	cairo_scale(cr, scale, scale);

	cairo_set_source_rgb(cr, 0, 0, 0);

	rsvg_handle_render_cairo(piece_image, cr);

	cairo_scale(cr, 1 / scale, 1 / scale);
}

gboolean board_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	IGNORE(data);

	// Unless the width/height of the drawing area is exactly a multiple of 8,
	// there's going to be some leftover space. We want to have the board
	// completely centered, so we pad by half the leftover space.
	// We rely on the widget being perfectly square in these calculations.
	uint square_size = get_square_size(widget);
	uint leftover_space =
		gtk_widget_get_allocated_width(widget) - square_size * BOARD_SIZE;
	uint padding = leftover_space / 2;
	cairo_translate(cr, padding, padding);

	// Color light squares one-by-one
	cairo_set_line_width(cr, 0);

	for (uint file = 0; file < BOARD_SIZE; file++) {
		for (int rank = BOARD_SIZE - 1; rank >= 0; rank--) {
			if ((rank + file) % 2 == 0) {
				// dark squares
				cairo_set_source_rgb(cr, 0.450980, 0.537255, 0.713725);
				cairo_rectangle(cr, 0, 0, square_size, square_size);
				cairo_fill(cr);
			} else {
				// light squares
				cairo_set_source_rgb(cr, 0.952941, 0.952941, 0.952941);
				cairo_rectangle(cr, 0, 0, square_size, square_size);
				cairo_fill(cr);
			}

			// Draw the piece (if any)
			Piece p;
			if ((p = PIECE_AT(current_game->board, file, rank)) != EMPTY &&
					(drag_source == NULL_SQUARE || SQUARE(file, rank) != drag_source)) {
				draw_piece(cr, p, square_size);
			}

			cairo_translate(cr, 0, square_size);
		}

		cairo_translate(cr, square_size, -square_size * BOARD_SIZE);
	}

	if (drag_source != NULL_SQUARE) {
		cairo_identity_matrix(cr);
		cairo_translate(cr, mouse_x - square_size / 2, mouse_y - square_size / 2);
		draw_piece(cr, PIECE_AT_SQUARE(current_game->board, drag_source), square_size);
	}

	return FALSE;
}

Square board_coords_to_square(GtkWidget *drawing_area, uint x, uint y)
{
	uint square_size = get_square_size(drawing_area);
	uint file = x / square_size;
	uint rank = BOARD_SIZE - 1 - y / square_size;

	return SQUARE(file, rank);
}

gboolean board_mouse_down_callback(GtkWidget *widget, GdkEvent *event,
		gpointer user_data)
{
	IGNORE(user_data);
	GdkEventButton *e = (GdkEventButton *)event;

	if (e->button != 1)
		return FALSE;

	Square clicked_square = board_coords_to_square(widget, e->x, e->y);
	if (PIECE_AT_SQUARE(current_game->board, clicked_square) != EMPTY) {
		drag_source = clicked_square;
	}

	return FALSE;
}

void set_button_sensitivity()
{
	gtk_widget_set_sensitive(back_button, current_game->parent != NULL);
	gtk_widget_set_sensitive(forward_button, has_children(current_game));
}

gboolean board_mouse_up_callback(GtkWidget *widget, GdkEvent *event,
		gpointer user_data)
{
	IGNORE(user_data);
	GdkEventButton *e = (GdkEventButton *)event;

	if (e->button != 1 || drag_source == NULL_SQUARE)
		return FALSE;

	Square drag_target = board_coords_to_square(widget, e->x, e->y);
	Move m = MOVE(drag_source, drag_target);
	if (legal_move(current_game->board, m, true)) {
		char notation[MAX_NOTATION_LENGTH];
		move_notation(current_game->board, m, notation);

		if (current_game->board->turn == WHITE)
			printf("%d. %s\n", current_game->board->move_number, notation);
		else
			printf(" ..%s\n", notation);

		Board *copy = malloc(sizeof *copy);
		copy_board(copy, current_game->board);
		perform_move(copy, m);
		current_game = add_child(current_game, m, copy);

		set_button_sensitivity();
	}

	drag_source = NULL_SQUARE;
	gtk_widget_queue_draw(widget);

	return FALSE;
}

gboolean board_mouse_move_callback(GtkWidget *widget, GdkEvent *event,
		gpointer user_data)
{
	IGNORE(user_data);
	GdkEventButton *e = (GdkEventButton *)event;

	mouse_x = e->x;
	mouse_y = e->y;

	if (drag_source != NULL_SQUARE)
		gtk_widget_queue_draw(widget);

	return FALSE;
}

gboolean back_button_click_callback(GtkWidget *widget, gpointer user_data)
{
	IGNORE(widget);
	IGNORE(user_data);

	current_game = current_game->parent;

	set_button_sensitivity();

	gtk_widget_queue_draw(board_display);

	return FALSE;
}

gboolean forward_button_click_callback(GtkWidget *widget, gpointer user_data)
{
	IGNORE(widget);
	IGNORE(user_data);

	current_game = first_child(current_game);

	set_button_sensitivity();

	gtk_widget_queue_draw(board_display);

	return FALSE;
}

void open_pgn_callback(GtkMenuItem *menu_item, gpointer user_data)
{
	IGNORE(menu_item);
	GtkWindow *parent = (GtkWindow *)user_data;

	GtkWidget *dialog = gtk_file_chooser_dialog_new("Open PGN", parent,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			"Cancel", GTK_RESPONSE_CANCEL,
			"Open", GTK_RESPONSE_ACCEPT,
			NULL);
	GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
	gtk_file_chooser_set_local_only(chooser, TRUE);

	GtkFileFilter *just_pgns = gtk_file_filter_new();
	gtk_file_filter_set_name(just_pgns, "PGN files");
	gtk_file_filter_add_pattern(just_pgns, "*.pgn");
	gtk_file_chooser_add_filter(chooser, just_pgns);

	GtkFileFilter *all_files = gtk_file_filter_new();
	gtk_file_filter_set_name(all_files, "All files");
	gtk_file_filter_add_pattern(all_files, "*");
	gtk_file_chooser_add_filter(chooser, all_files);

	int result = gtk_dialog_run(GTK_DIALOG(dialog));
	if (result == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(chooser);
		PGN pgn;
		GError *error = NULL;
		bool success = read_pgn(&pgn, filename, &error);

		g_free(filename);

		if (!success) {
			// TODO: Display an error dialog
			puts("Failed to read the PGN");
			return;
		}

		current_game = pgn.game;

		gtk_widget_queue_draw(board_display);

		set_button_sensitivity();
	}

	gtk_widget_destroy(dialog);
}

gboolean last_button_click_callback(GtkWidget *widget, gpointer user_data)
{
	IGNORE(widget);
	IGNORE(user_data);

	current_game = last_node(current_game);
	set_button_sensitivity();
	gtk_widget_queue_draw(board_display);

	return FALSE;
}
