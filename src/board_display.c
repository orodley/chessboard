#include <gtk/gtk.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "board_display.h"
#include "misc.h"

RsvgHandle *piece_images[2][6];

void load_svgs(char *dir, GError **err)
{
	uint len = strlen(dir) + 6; // piece letter + ".svg\0"
	char str[len];
	char *piece_letters[] = { "pnbrqk", "PNBRQK" };

	for (uint i = 0; i < 2; i++) {
		for (uint j = 0; piece_letters[i][j] != '\0'; j++) {
			sprintf(str, "%s%c.svg", dir, piece_letters[i][j]);
			printf("Loading '%s'\n", str);

			piece_images[i][j] = rsvg_handle_new_from_file(str, err);
			if (*err != NULL)
				return;
		}
	}
}

// TODO: the non-board bits on the edge of the screen look weird as they are
// the same color as the dark squares.
gboolean board_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	Board *board = (Board *)data;

	// Fill the background
	cairo_set_source_rgb(cr, 1, 1, 1);
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
			if ((p = PIECE_AT(board, file, rank)) != EMPTY) {
				RsvgHandle *piece_image =
					piece_images[PLAYER(p)][PIECE_TYPE(p) - 1];

				// 0.025 is a bit of a magic number. It's basically just
				// the amount by which the pieces must be scaled in order to
				// fit correctly with the default square size. We then scale
				// that based on how big the squares actually are.
				double scale = 0.025 * square_size / DEFAULT_SQUARE_SIZE;
				cairo_scale(cr, scale, scale);

				cairo_set_source_rgb(cr, 0, 0, 0);
				rsvg_handle_render_cairo(piece_image, cr);

				cairo_scale(cr, 1 / scale, 1 / scale);
			}

			cairo_translate(cr, 0, square_size);
		}

		cairo_translate(cr, square_size, -square_size * BOARD_SIZE);
	}

	return FALSE;
}
