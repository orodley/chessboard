#include <gtk/gtk.h>
#include "board.h"
#include "board_display.h"

int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);

	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), PROG_NAME);
	gtk_window_set_default_size(GTK_WINDOW(window),
			50 * BOARD_SIZE, 50 * BOARD_SIZE);
	g_signal_connect(G_OBJECT(window), "destroy",
			G_CALLBACK(gtk_main_quit), NULL);

	GtkWidget *drawing_area = gtk_drawing_area_new();
	gint width, height;
	gtk_window_get_size(GTK_WINDOW(window), &width, &height);
	gtk_widget_set_size_request(drawing_area, width, height);
	g_signal_connect(G_OBJECT(drawing_area), "draw",
			G_CALLBACK(board_draw_callback), NULL);

	gtk_container_add(GTK_CONTAINER(window), drawing_area);
	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}
