#include <gtk/gtk.h>
#include "board.h"

int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);

	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), PROG_NAME);
	gtk_window_set_default_size(GTK_WINDOW(window),
			50 * BOARD_SIZE, 50 * BOARD_SIZE);
	g_signal_connect(G_OBJECT(window), "destroy",
			G_CALLBACK(gtk_main_quit), NULL);
	gtk_widget_show(window);

	gtk_main();

	return 0;
}
