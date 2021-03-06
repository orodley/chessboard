#ifndef BOARD_DISPLAY_H_
#define BOARD_DISPLAY_H_

#include <stdbool.h>
#include <gtk/gtk.h>
#include "chess/board.h"
#include "chess/game.h"
#include "chess/moves.h"

#define DEFAULT_SQUARE_SIZE 50

void load_svgs(char *dir, GError **err);

void set_button_sensitivity();

gboolean board_draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data);
gboolean board_mouse_down_callback(GtkWidget *widget, GdkEvent *event,
		gpointer user_data);
gboolean board_mouse_up_callback(GtkWidget *widget, GdkEvent *event,
		gpointer user_data);
gboolean board_mouse_move_callback(GtkWidget *widget, GdkEvent *event,
		gpointer user_data);

gboolean go_back_button_click_callback(GtkWidget *widget, gpointer user_data);
gboolean go_next_button_click_callback(GtkWidget *widget, gpointer user_data);
gboolean go_start_button_click_callback(GtkWidget *widget, gpointer user_data);
gboolean go_end_button_click_callback(GtkWidget *widget, gpointer user_data);
gboolean flip_button_click_callback(GtkWidget *widget, gpointer user_data);

void open_pgn_callback(GtkMenuItem *menu_item, gpointer user_data);

extern Game *current_game;
extern GtkWidget *board_display;
extern GtkWidget *go_back_button;
extern GtkWidget *go_next_button;

#endif // include guard
