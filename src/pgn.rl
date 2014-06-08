#include <assert.h>
#include <ctype.h>
#include <gio/gio.h>
#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pgn.h"

// TODO: error messages

%%{
	machine pgn_tokenizer;
	write data;
}%%

typedef enum Token_type
{
	// Tokens with different values
	STRING, INTEGER, NAG, SYMBOL,
	// Fixed tokens - always just a single character
	DOT, ASTERISK, L_SQUARE_BRACKET, R_SQUARE_BRACKET,
	L_BRACKET, R_BRACKET, L_ANGLE_BRACKET, R_ANGLE_BRACKET, 
} Token_type;

typedef struct Token
{
	Token_type type;
	union {
		char *string;
		uint integer;
	} value;
} Token;


bool read_pgn(PGN *pgn, const char *input_filename, GError **error)
{
	assert(*error == NULL);

	bool ret = true;
	char *buf;
	gsize length;

	GFile *file = g_file_new_for_path(input_filename);
	if (!g_file_load_contents(file, NULL, &buf, &length, NULL, error)) {
		ret = false;
		goto cleanup;
	}

	// The initial size (140) is just a rough estimate of the average number of
	// tokens, based on 4 tokens for each of the 7 required tags, and 4 tokens
	// for each of 30 moves.
	GArray *tokens = g_array_sized_new(FALSE, FALSE, sizeof(char *), 140);

	// Variables that Ragel needs
	char *p = buf, *pe = buf + length;
	char *eof = NULL;
	char *ts, *te;
	int cs, act;
	// act is initialized for scanners, so it must be declared, but in this
	// scanner we don't actually use it. This silences the compiler warning.
	IGNORE(act);

	%%write init;
	%%{
		action add_to_array {
			size_t length = te - ts + 1; // + 1 for null terminator
			char *token = malloc(length); 
			strncpy(token, ts, length - 1);
			token[length - 1] = '\0';

			g_array_append_val(tokens, token);
		}

		# Token types, as per PGN spec section 7
		# Integers are read as tokens, and we convert them in the second pass

		# We cheat a little bit here.
		# According to the PGN spec, game termination markers are simply symbols.
		# However, according to the definition of the symbol token, symbols
		# cannot contain the '/' character. This seems like a contradiction, so
		# to work around it we allow '/'s in symbols.
		symbol = alnum (alnum | [_+#=:\-/])*;
		# TODO: Escaping in strings. This should probably be done in a second stage.
		string = '"' (print - '"')* '"';
		nag = '$' digit+;


		main := |*
			space;
			symbol  => add_to_array;
			string  => add_to_array;
			nag     => add_to_array;
			'.'     => { char *v = "."; g_array_append_val(tokens, v); };
			'*'     => { char *v = "*"; g_array_append_val(tokens, v); };
			'['     => { char *v = "["; g_array_append_val(tokens, v); };
			']'     => { char *v = "]"; g_array_append_val(tokens, v); };
			'('     => { char *v = "("; g_array_append_val(tokens, v); };
			')'     => { char *v = ")"; g_array_append_val(tokens, v); };
			'<'     => { char *v = "<"; g_array_append_val(tokens, v); };
			'>'     => { char *v = ">"; g_array_append_val(tokens, v); };
		*|;


		write exec;
	}%%

	IGNORE(pgn);

	for (size_t i = 0; i < tokens->len; i++)
		printf("Token: %s\n", g_array_index(tokens, char *, i));

cleanup:
	g_free(buf);
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
