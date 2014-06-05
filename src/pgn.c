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

static bool is_symbol_char(char c)
{
	return isalnum(c) ||
		c == '_' || c == '+' || c == '#' || c == '=' || c == ':' || c == '-';
}

static bool is_printable(char c)
{
	return c > 31;
}

// Consumes whitespace from the stream until the first non-whitespace char,
// which it returns
static char eat_whitespace(GDataInputStream *stream, GError **error)
{
	assert(*error == NULL);

	for (;;) {
		unsigned char c = g_data_input_stream_read_byte(stream, NULL, error);
		if (*error != NULL)
			return 0;

		if (!isspace(c))
			return c;
	}
}

static bool read_symbol(GDataInputStream *stream, char *symbol, GError **error)
{
	assert(*error == NULL);

	char c = eat_whitespace(stream, error);
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

static bool read_string(GDataInputStream *stream, char *string, GError **error)
{
	assert(*error == NULL);

	char c = eat_whitespace(stream, error);
	if (*error != NULL)
		return false;
	if (c != '"')
		return false;

	size_t i = 0;
	bool escaping = false;
	for (;;) {
		unsigned char c = g_data_input_stream_read_byte(stream, NULL, error);
		if (*error != NULL)
			return false;

		if (escaping) {
			switch (c) {
			case '\\': string[i++] = '\\'; break;
			case '"':  string[i++] = '"'; break;
			default: return false;
			}
		} else if (c == '\\') {
			escaping = true;
		} else {
			if (c == '"')
				break;
			else if (is_printable(c))
				string[i++] = c;
			else
				return false;
		}
	}

	return true;
}

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

	GHashTable *tags = g_hash_table_new(g_str_hash, g_str_equal);
	pgn->tags = tags;

	for (;;) {
		char c = eat_whitespace(stream, error);
		if (*error != NULL) {
			ret = false;
			goto cleanup;
		}
		if (c != '[') {
			ret = false;
			goto cleanup;
		}

		char *symbol = malloc(MAX_SYMBOL_LENGTH);
		if (!read_symbol(stream, symbol, error)) {
			ret = false;
			free(symbol);

			goto cleanup;
		}

		char *string = malloc(MAX_STRING_LENGTH);
		if (!read_string(stream, string, error)) {
			ret = false;
			free(symbol);
			free(string);

			goto cleanup;
		}

		c = eat_whitespace(stream, error);
		if (*error != NULL) {
			ret = false;
			free(symbol);
			free(string);

			goto cleanup;
		}
		if (c != ']') {
			ret = false;
			free(symbol);
			free(string);

			goto cleanup;
		}


		g_hash_table_insert(tags, symbol, string);
	}

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
