#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "pgn.h"

int main(int argc, char *argv[])
{
#if GLIB_MAJOR_VERION <= 2 && GLIB_MINOR_VERSION <= 34
	g_type_init();
#endif

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <pgn file>\n", argv[0]);
		return 1;
	}

	PGN pgn;
	GError *error = NULL;
	if (!read_pgn(&pgn, argv[1], &error)) {
		fprintf(stderr, "Failed to read PGN '%s'\n", argv[1]);
		if (error != NULL)
			fprintf(stderr, "%s\n", error->message);

		return 1;
	}

	if (!write_pgn(&pgn, stdout)) {
		fprintf(stderr, "Failed to write PGN '%s'\n", argv[1]);

		return 1;
	}

	return 0;
}
