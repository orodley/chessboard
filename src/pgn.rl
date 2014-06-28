#include <assert.h>
#include <ctype.h>
#include <gio/gio.h>
#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "moves.h"
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

void print_token(Token *t)
{
	switch (t->type) {
	case STRING: printf("STRING: %s\n", t->value.string); break;
	case SYMBOL: printf("SYMBOL: %s\n", t->value.string); break;
	case NAG: printf("NAG: %s\n", t->value.string); break;
	case INTEGER: printf("INTEGER: %d\n", t->value.integer); break;
	case DOT: puts("DOT"); break;
	case ASTERISK: puts("DOT"); break;
	case L_SQUARE_BRACKET: puts("L_SQUARE_BRACKET"); break;
	case R_SQUARE_BRACKET: puts("R_SQUARE_BRACKET"); break;
	case L_BRACKET: puts("L_BRACKET"); break;
	case R_BRACKET: puts("R_BRACKET"); break;
	case L_ANGLE_BRACKET: puts("L_ANGLE_BRACKET"); break;
	case R_ANGLE_BRACKET: puts("R_ANGLE_BRACKET"); break;
	}
}

static bool symbol_is_integer(Token *t)
{
	char *str = t->value.string;
	for (size_t i = 0; str[i] != '\0'; i++)
		if (!isdigit(str[i]))
			return false;

	return true;
}
		

static char *escape_string(char *str, size_t length)
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

		# Token types, as per PGN spec section 7
		# Integers are read as symbols, and we convert them in the second pass

		# We cheat a little bit here.
		# According to the PGN spec, game termination markers are simply symbols.
		# However, according to the definition of the symbol token, symbols
		# cannot contain the '/' character. This seems like a contradiction, so
		# to work around it we allow '/'s in symbols.
		symbol = alnum (alnum | [_+#=:\-/])*;
		string = '"' (('\\' print) | (print - '\\"'))* '"';
		nag = '$' digit+;


		main := |*
			space;
			symbol  => add_symbol;
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

	// Integers are a subset of symbols, and unfortunately Ragel scanners
	// attempt to match longer patterns before shorter ones.
	// So we do a second pass to look for symbols that are integers.
	for (size_t i = 0; i < tokens->len; i++) {
		Token *t = &g_array_index(tokens, Token, i);
		if (t->type == SYMBOL && symbol_is_integer(t)) {
			int n;
			sscanf(t->value.string, "%d", &n);

			t->type = INTEGER;
			t->value.integer = n;
		}
	}

	return tokens;
}

// It is impossible to parse a move without a reference to a particular board,
// as something like Bd5 could start from any square on that diagonal.
static Move parse_move(Board *board, char *notation)
{
	if (strcmp(notation, "O-O") == 0) {
		uint y = board->turn == WHITE ? 0 : 7;
		return MOVE(SQUARE(4, y), SQUARE(6, y));
	}
	if (strcmp(notation, "O-O-O") == 0) {
		uint y = board->turn == WHITE ? 0 : 7;
		return MOVE(SQUARE(4, y), SQUARE(2, y));
	}

	char stripped[5]; // max length without 'x#+'s, + 1 for null terminator
	size_t j = 0;
	for (size_t i = 0; notation[i] != '\0'; i++)
		if (notation[i] != 'x' && notation[i] != '#' && notation[i] != '+')
			stripped[j++] = notation[i];
	stripped[j] = '\0';

	size_t i = 0;
	Piece_type type;
	// If it's a pawn move, the target square starts at the beginning of the
	// string
	if (islower(stripped[0])) {
		type = PAWN;

		// An exception: if it's a move like exd5, the target square starts one 
		// char further on
		if (islower(stripped[1]))
			i++;
	} else {
		type = PIECE_TYPE(piece_from_char(stripped[0]));
		i++;
	}
	
	char disambig = 0;
	if (j == 4)
		// If it's this long, there must be a disambiguation char in there
		disambig = stripped[i++];
	
	uint target_file = CHAR_FILE(stripped[i++]);
	uint target_rank = CHAR_RANK(stripped[i++]);

	if (disambig != 0) {
		uint x = 0, y = 0, dx = 0, dy = 0;
		if (disambig >= 'a' && disambig <= 'g') {
			x = CHAR_FILE(disambig);
			dy = 1;
		} else if (disambig >= '1' && disambig <= '8') {
			y = CHAR_RANK(disambig);
			dx = 1;
		} else {
			return NULL_MOVE;
		}

		for (; x < BOARD_SIZE && y < BOARD_SIZE; x += dx, y += dy) {
			Piece p = PIECE_AT(board, x, y);
			if (PIECE_TYPE(p) != type || PLAYER(p) != board->turn)
				continue;

			Move m = MOVE(SQUARE(x, y), SQUARE(target_file, target_rank));
			if (legal_move(board, m, true))
				return m;
		}

		return NULL_MOVE;
	}

	for (uint x = 0; x < BOARD_SIZE; x++) {
		for (uint y = 0; y < BOARD_SIZE; y++) {
			Piece p = PIECE_AT(board, x, y);
			if (PIECE_TYPE(p) != type || PLAYER(p) != board->turn)
				continue;

			Move m = MOVE(SQUARE(x, y), SQUARE(target_file, target_rank));
			if (legal_move(board, m, true))
				return m;
		}
	}

	return NULL_MOVE;
}

static Result parse_game_termination_marker(Token *t)
{
	if (t->type == ASTERISK)
		return OTHER;
	if (t->type != SYMBOL)
		return NULL_RESULT;

	char *symbol = t->value.string;
	if (strcmp(symbol, "1-0") == 0)
		return WHITE_WINS;
	if (strcmp(symbol, "0-1") == 0)
		return BLACK_WINS;
	if (strcmp(symbol, "1/2-1/2") == 0)
		return DRAW;
	
	return NULL_RESULT;
}

static char *game_termination_marker(Result r)
{
	switch (r) {
	case WHITE_WINS: return "1-0";
	case BLACK_WINS: return "0-1";
	case DRAW: return "1/2-1/2";
	default: return "*";
	}
}

static bool parse_tokens(PGN *pgn, GArray *tokens, GError **err)
{
	// Start with tags
	pgn->tags = g_hash_table_new(g_str_hash, g_str_equal);

	size_t i = 0;
	while ((&g_array_index(tokens, Token, i))->type == L_SQUARE_BRACKET) {
		Token *tag_name_token = &g_array_index(tokens, Token, ++i);
		if (tag_name_token->type != SYMBOL) {
			if (tag_name_token->type == INTEGER) {
				g_set_error(err, 0, 0,
						"Unexpected token: %d", tag_name_token->value.integer);
			} else {
				g_set_error(err, 0, 0,
						"Unexpected token: %s", tag_name_token->value.string);
			}

			return false;
		}

		char *tag_name = tag_name_token->value.string;

		if (g_hash_table_contains(pgn->tags, tag_name)) {
			g_set_error(err, 0, 0, "Duplicate tag: %s", tag_name);
			return false;
		}

		Token *tag_value_token = &g_array_index(tokens, Token, ++i);
		if (tag_value_token->type != STRING) {
			if (tag_name_token->type == INTEGER) {
				g_set_error(err, 0, 0, "Tag values must be strings, %d is not",
						tag_value_token->value.integer);
			} else {
				g_set_error(err, 0, 0, "Tag values must be strings, %s is not",
						tag_value_token->value.string);
			}

			return false;
		}

		char *tag_value = tag_value_token->value.string;

		char *tag_name_copy = malloc(strlen(tag_name) + 1);
		strcpy(tag_name_copy, tag_name);
		char *tag_value_copy = malloc(strlen(tag_value) + 1);
		strcpy(tag_value_copy, tag_value);

		g_hash_table_insert(pgn->tags, tag_name_copy, tag_value_copy);

		Token *close_square_bracket_token = &g_array_index(tokens, Token, ++i);
		if (close_square_bracket_token->type != R_SQUARE_BRACKET) {
			g_set_error(err, 0, 0,
					"Tag %s has no matching close bracket", tag_name);
			return false;
		}

		i++;
	}

	// Now the movetext section
	uint half_move_number = 2;

	Game *game = new_game();
	game->board = malloc(sizeof(Board));
	pgn->game = game;
	// TODO: Use value in start board tag if present.
	from_fen(game->board, start_board_fen);

	// TODO: variations, NAG
	while (i < tokens->len) {
		Token *t = &g_array_index(tokens, Token, i++);
		if (t->type == INTEGER) {
			if (t->value.integer != half_move_number / 2) {
				g_set_error(err, 0, 0,
						"Incorrect move number %d (should be %d)",
						t->value.integer, half_move_number / 2);
				return false;
			}

			while ((t = &g_array_index(tokens, Token, i++))->type == DOT)
				;
		}

		if (t->type != SYMBOL && t->type != ASTERISK) {
			if (t->type == INTEGER) {
				g_set_error(err, 0, 0, "Expected a move, got %d",
						t->value.integer);
			} else {
				g_set_error(err, 0, 0, "Expected a move, got %s",
						t->value.string);
			}

			return false;
		}

		Result r;
		if ((r = parse_game_termination_marker(t)) != NULL_RESULT) {
			pgn->result = r;

			// If we didn't see a result tag, try to fill it in with the value
			// in the game termination marker

			if (!g_hash_table_contains(pgn->tags, "Result")) {
				char *tmp = malloc(strlen(t->value.string));
				strcpy(tmp, t->value.string);
				g_hash_table_insert(pgn->tags, "Result", tmp);
			}

			return true;
		}

		Move m;
		if ((m = parse_move(game->board, t->value.string)) == NULL_MOVE) {
			g_set_error(err, 0, 0, "Expected a move, got %s", t->value.string);
			
			return false;
		}

		game = add_child(game, m);

		half_move_number++;
	}

	return true;
}

static void free_tokens(GArray *tokens)
{
	for (size_t i = 0; i < tokens->len; i++) {
		Token *t = &g_array_index(tokens, Token, i);
		if (t->type == SYMBOL || t->type == STRING || t->type == NAG)
			free(t->value.string);
	}

	g_array_free(tokens, TRUE);
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
	ret = parse_tokens(pgn, tokens, error);
	free_tokens(tokens);

cleanup:
	g_free(buf);
	g_object_unref(file);

	return ret;
}

static const char *seven_tag_roster[] =
{
	"Event", "Site", "Date", "Round", "White", "Black", "Result"
};

static bool in_seven_tag_roster(char *tag_name)
{
	size_t size = sizeof(seven_tag_roster) / sizeof(seven_tag_roster[0]);
	for (size_t i = 0; i < size; i++)
		if (strcmp(tag_name, seven_tag_roster[i]) == 0)
			return true;
	
	return false;
}

static void write_tag(FILE *file, const char *tag_name, const char *tag_value)
{
	// TODO: handle IO errors
	fprintf(file, "[%s \"" , tag_name);

	for (size_t i = 0; tag_value[i] != '\0'; i++) {
		char c = tag_value[i];
		if (c == '\\' || c == '"')
			putc('\\', file);

		putc(c, file);
	}

	fputs("\"]\n", file);
}

static char *default_tag_value(const char *tag_name)
{
	if (strcmp(tag_name, "Date") == 0)
		return "????.??.??";
	if (strcmp(tag_name, "Result") == 0)
		return "*";
	else
		return "?";
}

static void process_tag(gpointer key, gpointer value, gpointer user_data)
{
	FILE *file = (FILE *)user_data;
	char *tag_name = (char *)key;
	char *tag_value = (char *)value;

	// We print the standard seven tags first, in a specified order, so don't
	// print them again.
	if (in_seven_tag_roster(tag_name))
		return;

	write_tag(file, tag_name, tag_value);
}

// TODO: handle IO errors
bool write_pgn(PGN *pgn, FILE *file)
{
	size_t size = sizeof(seven_tag_roster) / sizeof(seven_tag_roster[0]);
	for (size_t i = 0; i < size; i++) {
		const char *tag_name = seven_tag_roster[i];
		const char *tag_value = g_hash_table_contains(pgn->tags, tag_name) ?
			g_hash_table_lookup(pgn->tags, tag_name) :
			default_tag_value(tag_name);
		write_tag(file, tag_name, tag_value);
	}
	g_hash_table_foreach(pgn->tags, process_tag, (void *)file);

	putc('\n', file);
	
	Game *game = pgn->game->children;
	do {
		// A lot of the logic in here seems reversed/offset by one. This is
		// because the board associated with a particular move actually lives
		// at the parent node for that board
		if (game->board->turn == BLACK)
			fprintf(file, game->board->move_number == 1 ?
				"%d." :
				" %d.", game->board->move_number);

		char move_str[MAX_NOTATION_LENGTH];
		move_notation(game->parent->board, game->move, move_str);
		fprintf(file, " %s", move_str);
		
		game = first_child(game);
	} while (game != NULL);

	fprintf(file, " %s\n", game_termination_marker(pgn->result));

	return true;
}

void free_pgn(PGN *pgn)
{
	g_hash_table_destroy(pgn->tags);
	free_game(pgn->game);
}
