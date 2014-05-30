#ifndef _BOARD_DISPLAY_H
#define _BOARD_DISPLAY_H

#include <stdbool.h>
#include <gtk/gtk.h>

#define DEFAULT_SQUARE_SIZE 50

void load_svgs(char *dir, GError **err);
gboolean board_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data);
gboolean board_mouse_down_callback(GtkWidget *widget, GdkEvent *event,
		gpointer user_data);
gboolean board_mouse_up_callback(GtkWidget *widget, GdkEvent *event,
		gpointer user_data);

#endif
