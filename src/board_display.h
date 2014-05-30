#ifndef _BOARD_DISPLAY_H
#define _BOARD_DISPLAY_H

#include <stdbool.h>
#include <gtk/gtk.h>

#define DEFAULT_SQUARE_SIZE 50

gboolean board_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data);
void load_svgs(char *dir, GError **err);

#endif
