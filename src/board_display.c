#include <gtk/gtk.h>
#include "board.h"
#include "misc.h"

gboolean board_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	IGNORE(data);

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
	cairo_set_source_rgb(cr, 0.952941, 0.952941, 0.952941);
	cairo_set_line_width(cr, 0);

	for (uint rank = 0; rank < BOARD_SIZE; rank++) {
		for (uint file = 0; file < BOARD_SIZE; file++) {
			if ((rank + file) % 2 != 0)
				continue;
			
			cairo_rectangle(cr, file * square_size, rank * square_size,
					square_size, square_size);

			cairo_fill(cr);
		}
	}

	return FALSE;
}
