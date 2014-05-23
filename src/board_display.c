#include <gtk/gtk.h>
#include "board.h"
#include "misc.h"

// TODO: the non-board bits on the edge of the screen look weird as they are
// the same color as the dark squares.
gboolean board_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	Board *board = (Board *)data;

	// Color dark squares by setting the background
	cairo_set_source_rgb(cr, 0.450980, 0.537255, 0.713725);
	cairo_paint(cr);

	guint width  = gtk_widget_get_allocated_width(widget);
	guint height = gtk_widget_get_allocated_height(widget);

	uint max_square_width = width / BOARD_SIZE;
	uint max_square_height = height / BOARD_SIZE;
	uint square_size = max_square_width < max_square_height ?
		max_square_width :
		max_square_height;

	// Color light squares one-by-one
	cairo_set_line_width(cr, 0);

	for (uint file = 0; file < BOARD_SIZE; file++) {
		for (uint rank = 0; rank < BOARD_SIZE; rank++) {
			uint left = file * square_size;
			uint top = rank * square_size;

			if ((rank + file) % 2 == 0) { // only draw the light squares
				// Fill in the square
				cairo_set_source_rgb(cr, 0.952941, 0.952941, 0.952941);
				cairo_rectangle(cr, left, top, square_size, square_size);
				cairo_fill(cr);
			}

			// Draw the piece (if any)
			Piece p;
			if ((p = PIECE_AT(board, file, rank)) != EMPTY) {
				char c = char_from_piece(p);
				char str[2];
				str[0] = c;
				str[1] = '\0';

				cairo_move_to(cr, left + square_size / 2, top + square_size / 2);
				cairo_set_source_rgb(cr, 0, 0, 0);
				cairo_show_text(cr, str);
			}
		}
	}

	return FALSE;
}
