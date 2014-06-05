#include <assert.h>
#include <ctype.h>
#include <gio/gio.h>
#include <glib.h>
#include <stdbool.h>
#include "pgn.h"

static bool is_symbol_char(char c)
{
	return isalnum(c) ||
		c == '_' || c == '+' || c == '#' || c == '=' || c == ':' || c == '-';
}

static bool read_symbol(GDataInputStream *stream, char *symbol, GError **error)
{
	assert(*error == NULL);

	unsigned char c = g_data_input_stream_read_byte(stream, NULL, error);
	if (*error != NULL)
		return false;
	if (!isalnum(c)) // symbols must start with an alphanumeric character
		return false;

	size_t i = 0;
	do {
		symbol[i++] = c;

		c = g_data_input_stream_read_byte(stream, NULL, error);
		if (*error != NULL)
			return false;
	} while (is_symbol_char(c));

	return true;
}

bool read_pgn(PGN *pgn, const char *input_filename, GError **error)
{
	// This shouldn't already have an error in it
	assert(*error == NULL);

	bool ret = true;

	GFile *file = g_file_new_for_path(input_filename);
	GFileInputStream *fis = g_file_read(file, NULL, error);
	if (*error != NULL) {
		ret = false;
		goto cleanup;
	}
	GDataInputStream *stream = g_data_input_stream_new(G_INPUT_STREAM(fis));

	GHashTable *tags = g_hash_table_new(g_str_hash, g_str_equal);
	pgn->tags = tags;

	const char *token;
	char symbol[MAX_SYMBOL_LENGTH];
	for (;;) {
		// Find the next tag
		token =
			g_data_input_stream_read_until(stream, "[", NULL, NULL, error);
		if (*error != NULL) {
			ret = false;
			goto cleanup;
		}

		if (!read_symbol(stream, symbol, error)) {
			ret = false;
			goto cleanup;
		}
	}

	IGNORE(token);

cleanup:
	g_object_unref(stream);
	g_object_unref(fis);
	g_object_unref(file);

	return ret;
}

void free_pgn(PGN *pgn)
{
	g_hash_table_destroy(pgn->tags);
	free_game(pgn->game);
}
