#include <gtk/gtk.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "board_display.h"
#include "misc.h"
#include "moves.h"

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

Square drag_source = NULL_SQUARE;
uint mouse_x;
uint mouse_y;

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

// TODO: the non-board bits on the edge of the screen look weird as they are
// the same color as the dark squares.
gboolean board_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	Board *board = (Board *)data;

	// Fill the background
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);

	uint square_size = get_square_size(widget);

	// Color light squares one-by-one
	cairo_set_line_width(cr, 0);

	for (uint file = 0; file < BOARD_SIZE; file++) {
		for (int rank = BOARD_SIZE - 1; rank >= 0; rank--) {
			if ((rank + file) % 2 == 0) {
				// light squares
				cairo_set_source_rgb(cr, 0.952941, 0.952941, 0.952941);
				cairo_rectangle(cr, 0, 0, square_size, square_size);
				cairo_fill(cr);
			} else {
				// dark squares
				cairo_set_source_rgb(cr, 0.450980, 0.537255, 0.713725);
				cairo_rectangle(cr, 0, 0, square_size, square_size);
				cairo_fill(cr);
			}

			// Draw the piece (if any)
			Piece p;
			if ((p = PIECE_AT(board, file, rank)) != EMPTY &&
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
		draw_piece(cr, PIECE_AT_SQUARE(board, drag_source), square_size);
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
	GdkEventButton *e = (GdkEventButton *)event;
	Board *board = (Board *)user_data;

	if (e->button != 1)
		return FALSE;

	Square clicked_square = board_coords_to_square(widget, e->x, e->y);
	if (PIECE_AT_SQUARE(board, clicked_square) != EMPTY) {
		drag_source = clicked_square;
	}

	return FALSE;
}

gboolean board_mouse_up_callback(GtkWidget *widget, GdkEvent *event,
		gpointer user_data)
{
	GdkEventButton *e = (GdkEventButton *)event;
	Board *board = (Board *)user_data;

	if (e->button != 1 || drag_source == NULL_SQUARE)
		return FALSE;

	Square drag_target = board_coords_to_square(widget, e->x, e->y);
	Move m = MOVE(drag_source, drag_target);

	if (legal_move(board, m)) {
		PIECE_AT_SQUARE(board, END_SQUARE(m)) =
			PIECE_AT_SQUARE(board, START_SQUARE(m));
		PIECE_AT_SQUARE(board, START_SQUARE(m)) = EMPTY;
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
		// TODO: We only need to redraw within square_size around the mouse
		gtk_widget_queue_draw(widget);

	return FALSE;
}
