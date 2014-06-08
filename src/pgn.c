#include <assert.h>
#include <ctype.h>
#include <gio/gio.h>
#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "pgn.h"

// TODO: either use standard C IO everywhere or use GIO everywhere
// TODO: error messages


bool read_pgn(PGN *pgn, const char *input_filename, GError **error)
{
	assert(*error == NULL);

	bool ret = true;

	GFile *file = g_file_new_for_path(input_filename);
	GFileInputStream *fis = g_file_read(file, NULL, error);
	if (*error != NULL) {
		ret = false;
		goto cleanup;
	}
	GDataInputStream *stream = g_data_input_stream_new(G_INPUT_STREAM(fis));

	IGNORE(pgn);

cleanup:
	g_object_unref(stream);
	g_object_unref(fis);
	g_object_unref(file);

	return ret;
}

static void write_tag(gpointer key, gpointer value, gpointer user_data)
{
	FILE *file = (FILE *)user_data;
	char *tag_name = (char *)key;
	char *tag_value = (char *)value;

	// TODO: handle IO errors
	fprintf(file, "[%s \"%s\"]\n", tag_name, tag_value);
}

// TODO: handle IO errors
bool write_pgn(PGN *pgn, FILE *file)
{
	g_hash_table_foreach(pgn->tags, write_tag, (void *)file);

	return true;
}

void free_pgn(PGN *pgn)
{
	g_hash_table_destroy(pgn->tags);
	free_game(pgn->game);
}
