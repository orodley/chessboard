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

typedef union Token_value {
	char *string;
	uint integer;
} Token_value;

typedef struct Token
{
	Token_type type;
	Token_value value;
} Token;

char *escape_string(char *str, size_t length)
{
	char *out = malloc(length + 1);
	size_t j = 0;
	bool escaping = false;

	for (size_t i = 0; i < length; i++) {
		char c = str[i];
		switch (c) {
		case '\\':
			if (escaping) {
				out[j++] = '\\';
				escaping = false;
			} else {
				escaping = true;
			}

			break;
		case '"':
			// We should never come across an unescaped quote in a string, as
			// the tokenizer should have terminated the string in this case.
			assert(escaping);

			out[j++] = '"';
			escaping = false;
			break;
		default:
			out[j++] = c;
			escaping = false;
			break;
		}
	}

	out[j] = '\0';

	return out;
}

static GArray *tokenize_pgn(char *buf, gsize length)
{
	// The initial size (140) is just a rough estimate of the average number of
	// tokens, based on 4 tokens for each of the 7 required tags, and 4 tokens
	// for each of 30 moves.
	GArray *tokens = g_array_sized_new(FALSE, FALSE, sizeof(Token), 140);

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
		action add_string {
			// - 2 for quotes
			size_t length = te - ts - 2;
			char *token = escape_string(ts + 1, length);

			Token t = { STRING, { token } };

			g_array_append_val(tokens, t);
		}

		action add_symbol {
			// + 1 for null terminator
			size_t length = te - ts + 1;
			char *token = malloc(length); 
			strncpy(token, ts, length - 1);
			token[length - 1] = '\0';

			Token t = { SYMBOL, { token } };

			g_array_append_val(tokens, t);
		}

		action add_nag {
			// + 1 for null terminator
			size_t length = te - ts + 1;
			char *token = malloc(length); 
			strncpy(token, ts, length - 1);
			token[length - 1] = '\0';

			Token t = { NAG, { token } };

			g_array_append_val(tokens, t);
		}

		action add_integer {
			int n;
			sscanf(t_s, "%d", &n);

			Token t = { INTEGER, { n } }
			
			g_array_append_val(tokens, t);
		}

		# Token types, as per PGN spec section 7
		# Integers are read as tokens, and we convert them in the second pass

		# We cheat a little bit here.
		# According to the PGN spec, game termination markers are simply symbols.
		# However, according to the definition of the symbol token, symbols
		# cannot contain the '/' character. This seems like a contradiction, so
		# to work around it we allow '/'s in symbols.
		symbol = alnum (alnum | [_+#=:\-/])*;
		string = '"' (('\\' print) | (print - '\\"'))* '"';
		integer = digit+;
		nag = '$' digit+;


		main := |*
			space;
			symbol  => add_symbol;
			integer => add_symbol;
			string  => add_string;
			nag     => add_nag;
			'.'     => { Token t = { DOT,              { 0 } }; g_array_append_val(tokens, t); };
			'*'     => { Token t = { ASTERISK,         { 0 } }; g_array_append_val(tokens, t); };
			'['     => { Token t = { L_SQUARE_BRACKET, { 0 } }; g_array_append_val(tokens, t); };
			']'     => { Token t = { R_SQUARE_BRACKET, { 0 } }; g_array_append_val(tokens, t); };
			'('     => { Token t = { L_BRACKET,        { 0 } }; g_array_append_val(tokens, t); };
			')'     => { Token t = { R_BRACKET,        { 0 } }; g_array_append_val(tokens, t); };
			'<'     => { Token t = { L_ANGLE_BRACKET,  { 0 } }; g_array_append_val(tokens, t); };
			'>'     => { Token t = { R_ANGLE_BRACKET,  { 0 } }; g_array_append_val(tokens, t); };
		*|;


		write exec;
	}%%

	return tokens;
}

static bool parse_tokens(PGN *pgn, GArray *tokens)
{
	// Start with tags
	pgn->tags = g_hash_table_new(g_str_hash, g_str_equal);

	size_t i = 0;
	while ((&g_array_index(tokens, Token, i++))->type == L_SQUARE_BRACKET) {
		Token *tag_name_token = &g_array_index(tokens, Token, i++);
		if (tag_name_token->type != SYMBOL)
			return false;

		char *tag_name = tag_name_token->value.string;

		if (g_hash_table_contains(pgn->tags, tag_name))
			return false;

		Token *tag_value_token = &g_array_index(tokens, Token, i++);
		if (tag_value_token->type != STRING)
			return false;

		char *tag_value = tag_value_token->value.string;

		g_hash_table_insert(pgn->tags, tag_name, tag_value);

		Token *close_square_bracket_token = &g_array_index(tokens, Token, i++);
		if (close_square_bracket_token->type != R_SQUARE_BRACKET)
			return false;
	}

	return true;
}


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

	GArray *tokens = tokenize_pgn(buf, length);
	parse_tokens(pgn, tokens);

	IGNORE(pgn);

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
