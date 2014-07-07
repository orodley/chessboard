#include <gtk/gtk.h>
#include <librsvg/rsvg.h>
#ifndef RSVG_CAIRO_H
#include <librsvg/rsvg-cairo.h>
#endif
#include <stdbool.h>
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

static uint get_square_size(GtkWidget *board)
{
	uint width  = gtk_widget_get_allocated_width(board);
	uint height = gtk_widget_get_allocated_height(board);

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
// TODO: this should flip on the x-axis too
bool board_flipped = false;

static Square drag_source = NULL_SQUARE;
static uint mouse_x;
static uint mouse_y;

void set_button_sensitivity()
{
	gtk_widget_set_sensitive(back_button, current_game->parent != NULL);
	gtk_widget_set_sensitive(forward_button, has_children(current_game));
}

static void draw_piece(cairo_t *cr, Piece p, uint size)
{
	RsvgHandle *piece_image =
		piece_images[PLAYER(p)][PIECE_TYPE(p) - 1];

	// 0.025 is a bit of a magic number. It's basically just the factor by
	// which the pieces must be scaled in order to fit correctly with the
	// default square size. We then scale that based on how big the squares
	// actually are.
	double scale = 0.025 * size / DEFAULT_SQUARE_SIZE;
	cairo_scale(cr, scale, scale);

	rsvg_handle_render_cairo(piece_image, cr);

	cairo_scale(cr, 1 / scale, 1 / scale);
}

// This should be divisible by 2 so as not to leave a one pixel gap
#define HIGHLIGHT_LINE_WIDTH 4

gboolean board_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	IGNORE(data);

	// Unless the width/height of the drawing area is exactly a multiple of 8,
	// there's going to be some leftover space. We want to have the board
	// completely centered, so we pad by half the leftover space.
	// We rely on the widget being perfectly square in these calculations, as
	// this is enforced by its containing aspect frame.
	uint square_size = get_square_size(widget);
	uint leftover_space =
		gtk_widget_get_allocated_width(widget) - square_size * BOARD_SIZE;
	uint padding = leftover_space / 2;
	cairo_translate(cr, padding, padding);

	// Color light squares one-by-one
	cairo_set_line_width(cr, 0);
	for (uint x = 0; x < BOARD_SIZE; x++) {
		for (int rank = BOARD_SIZE - 1; rank >= 0; rank--) {
			int y = board_flipped ? BOARD_SIZE - rank - 1 : rank;
			if ((y + x) % 2 == 0) {
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

			// Highlight the source and target squares of the last move
			Move last_move = current_game->move;
			Square s = SQUARE(x, y);
			if (last_move != NULL_MOVE &&
					(s == START_SQUARE(last_move) || s == END_SQUARE(last_move))) {
				cairo_set_source_rgb(cr, 0.225, 0.26, 0.3505);
				cairo_set_line_width(cr, HIGHLIGHT_LINE_WIDTH);
				cairo_translate(cr, HIGHLIGHT_LINE_WIDTH / 2, HIGHLIGHT_LINE_WIDTH / 2);
				cairo_rectangle(cr, 0, 0, square_size - HIGHLIGHT_LINE_WIDTH,
						square_size - HIGHLIGHT_LINE_WIDTH);
				cairo_stroke(cr);

				cairo_set_line_width(cr, 1);
				cairo_translate(cr, -HIGHLIGHT_LINE_WIDTH / 2, -HIGHLIGHT_LINE_WIDTH / 2);
			}

			// Draw the piece, if there is one
			Piece p;
			if ((p = PIECE_AT(current_game->board, x, y)) != EMPTY &&
					(drag_source == NULL_SQUARE || SQUARE(x, y) != drag_source)) {
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

static Square board_coords_to_square(GtkWidget *drawing_area, uint x, uint y)
{
	uint square_size = get_square_size(drawing_area);
	uint board_x = x / square_size;
	uint board_y = y / square_size;
	if (!board_flipped)
		board_y = BOARD_SIZE - 1 - board_y;

	return SQUARE(board_x, board_y);
}

// Start dragging a piece if the mouse is over one.
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

// Try to move a piece if we're currently dragging one
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
		char notation[MAX_ALGEBRAIC_NOTATION_LENGTH];
		algebraic_notation_for(current_game->board, m, notation);

		// TODO: Stop printing this when we have a proper move list GUI
		if (current_game->board->turn == WHITE)
			printf("%d. %s\n", current_game->board->move_number, notation);
		else
			printf(" ..%s\n", notation);

		current_game = add_child(current_game, m);

		set_button_sensitivity();
	}

	drag_source = NULL_SQUARE;
	gtk_widget_queue_draw(widget);

	return FALSE;
}

// Redraw if we're dragging a piece
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

// Back up one move
gboolean back_button_click_callback(GtkWidget *widget, gpointer user_data)
{
	IGNORE(widget);
	IGNORE(user_data);

	current_game = current_game->parent;

	set_button_sensitivity();

	gtk_widget_queue_draw(board_display);

	return FALSE;
}

// Go forward one move
gboolean forward_button_click_callback(GtkWidget *widget, gpointer user_data)
{
	IGNORE(widget);
	IGNORE(user_data);

	current_game = first_child(current_game);

	set_button_sensitivity();

	gtk_widget_queue_draw(board_display);

	return FALSE;
}

// Go to the start of the game
gboolean first_button_click_callback(GtkWidget *widget, gpointer user_data)
{
	IGNORE(widget);
	IGNORE(user_data);

	current_game = root_node(current_game);
	set_button_sensitivity();
	gtk_widget_queue_draw(board_display);

	return FALSE;
}

// Go to the end of the game
gboolean last_button_click_callback(GtkWidget *widget, gpointer user_data)
{
	IGNORE(widget);
	IGNORE(user_data);

	current_game = last_node(current_game);
	set_button_sensitivity();
	gtk_widget_queue_draw(board_display);

	return FALSE;
}

// Flip the board
gboolean flip_button_click_callback(GtkWidget *widget, gpointer user_data)
{
	IGNORE(widget);
	IGNORE(user_data);

	board_flipped = !board_flipped;
	gtk_widget_queue_draw(board_display);

	return FALSE;
}

// Load a new game from a PGN file
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
