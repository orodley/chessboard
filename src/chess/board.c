#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "board.h"
#include "misc.h"
#include "moves.h"

void copy_board(Board *dst, Board *src)
{
	dst->turn = src->turn;
	dst->castling[0] = src->castling[0];
	dst->castling[1] = src->castling[1];
	dst->en_passant = src->en_passant;
	dst->half_move_clock = src->half_move_clock;
	dst->move_number = src->move_number;

	for (uint x = 0; x < BOARD_SIZE; x++)
		for (uint y = 0; y < BOARD_SIZE; y++)
			PIECE_AT(dst, x, y) = PIECE_AT(src, x, y);
}

// Converts a character into a piece, using the standard used in PGN and FEN.
Piece piece_from_char(char c)
{
	Player player = isupper(c) ? WHITE : BLACK;
	switch (tolower(c)) {
	case 'p': return PIECE(player, PAWN);
	case 'n': return PIECE(player, KNIGHT);
	case 'b': return PIECE(player, BISHOP);
	case 'r': return PIECE(player, ROOK);
	case 'q': return PIECE(player, QUEEN);
	case 'k': return PIECE(player, KING);
	default: return EMPTY;
	}
}

// Converts a piece into a char, using the standard used in PGN and FEN
char char_from_piece(Piece p)
{
	switch (PIECE_TYPE(p)) {
	case PAWN:   return 'P';
	case KNIGHT: return 'N';
	case BISHOP: return 'B';
	case ROOK:   return 'R';
	case QUEEN:  return 'Q';
	case KING:   return 'K';
	default:     return ' ';
	}
}

char *start_board_fen =
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Initializes a Board given a string containing Forsyth-Edwards Notation (FEN)
// See <http://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation> for a
// description, and <http://kirill-kryukov.com/chess/doc/fen.html> for the spec
// TODO: this is way too fucking long
bool from_fen(Board *board, const char *fen_str)
{
	uint i = 0;

	for (int y = BOARD_SIZE - 1; y >= 0; y--) {
		char c;
		uint x = 0;
		while ((c = fen_str[i++]) != '/' && c != ' ') {
			if (isdigit(c)) {
				if (c == '0') // "Skip zero files" makes no sense
					return false;

				for (int n = 0; n < c - '0'; n++)
					PIECE_AT(board, x + n, y) = EMPTY;

				x += c - '0';
				continue;
			} else {
				PIECE_AT(board, x, y) = piece_from_char(c);
			}

			x++;
		}
	}

	char player = fen_str[i++];
	if (player == 'w')
		board->turn = WHITE;
	else if (player == 'b')
		board->turn = BLACK;
	else
		return false;

	if (fen_str[i++] != ' ')
		return false;

	Castling b_castling = { false, false };
	Castling w_castling = { false, false };
	char c;
	while ((c = fen_str[i++]) != ' ') {
		switch (c) {
		case 'K': w_castling.kingside  = true; break;
		case 'Q': w_castling.queenside = true; break;
		case 'k': b_castling.kingside  = true; break;
		case 'q': b_castling.queenside = true; break;
		case '-': break;
		default: return false;
		}
	}

	board->castling[BLACK] = b_castling;
	board->castling[WHITE] = w_castling;

	char file_char = fen_str[i++];
	if (file_char == '-') {
		board->en_passant = NULL_SQUARE;
	} else {
		if (file_char < 'a' || file_char > 'g')
			return false;

		char rank_char = fen_str[i++];
		if (!isdigit(rank_char))
			return false;

		board->en_passant = SQUARE(file_char - 'a', rank_char - '1');
	}

	if (fen_str[i++] != ' ')
		return false;

	uint half_move_clock;
	if (sscanf(fen_str + i, "%u", &half_move_clock) != 1)
		return false;

	board->half_move_clock = half_move_clock;
	while (fen_str[i++] != ' ')
		;

	uint move_number;
	if (sscanf(fen_str + i, "%u", &move_number) != 1)
		return false;

	board->move_number = move_number;

	// TODO: check there's no trailing shit after a valid FEN string?
	return true;
}

// For debugging
void print_board(Board *b)
{
	puts("..........");
	for (int y = BOARD_SIZE - 1; y >= 0; y--) {
		putchar('.');
		for (uint x = 0; x < BOARD_SIZE; x++) {
			Piece p = PIECE_AT(b, x, y);
			char c = char_from_piece(p);
			putchar(PLAYER(p) == BLACK ? tolower(c) : c);
		}

		puts(".");
	}
	puts("..........");
	printf("%s to move\n", b->turn == WHITE ? "White" : "Black");
}

static Square find_piece_looking_at(Board *board, Square square, Player piece_owner)
{
	// We need to make sure we don't have infinite recursion in legal_move.
	// This can happen with looking for checks - we need to see if there are
	// any moves that put us in check to decide if the move is legal, but to
	// see if we are in check we need to look at all the moves our opponent
	// can make. And checking those moves will follow the same process.
	//
	// However, we don't actually need to see if the moves put us into check in
	// this case, as it doesn't matter if taking their king puts us in check;
	// we've already won.
	bool care_about_check = PIECE_TYPE(PIECE_AT_SQUARE(board, square)) != KING;

	Player initial_turn = board->turn;
	board->turn = piece_owner;

	Square ret = NULL_SQUARE;

	for (uint y = 0; y < BOARD_SIZE; y++) {
		for (uint x = 0; x < BOARD_SIZE; x++) {
			Piece p = PIECE_AT(board, x, y);
			Square s = SQUARE(x, y);
			Move m = MOVE(s, square);

			if (PLAYER(p) == piece_owner &&
					legal_move(board, m, care_about_check)) {
				ret = s;
				goto cleanup;
			}
		}
	}

cleanup:
	board->turn = initial_turn;
	return ret;
}

static Square find_attacking_piece(Board *board, Square square, Player attacker)
{
	// The easiest way to do this without duplicating logic from legal_move
	// is to put an enemy piece there and then check if moving there is legal.
	// This will trigger the logic in legal_move for pawn captures.
	Piece initial_piece = PIECE_AT_SQUARE(board, square);
	PIECE_AT_SQUARE(board, square) =
		PIECE(OTHER_PLAYER(attacker), PAWN);

	Square s = find_piece_looking_at(board, square, attacker);

	PIECE_AT_SQUARE(board, square) = initial_piece;

	return s;
}

static bool under_attack(Board *board, Square square, Player attacker)
{
	return find_attacking_piece(board, square, attacker) != NULL_SQUARE;
}

static Square find_king(Board *board, Player p)
{
	Piece king = PIECE(p, KING);

	for (uint y = 0; y < BOARD_SIZE; y++)
		for (uint x = 0; x < BOARD_SIZE; x++)
			if (PIECE_AT(board, x, y) == king)
				return SQUARE(x, y);

	return NULL_SQUARE;
}

bool in_check(Board *board, Player p)
{
	Square king_location = find_king(board, p);
	assert(king_location != NULL_SQUARE); // both players should have a king
	return under_attack(board, king_location, OTHER_PLAYER(p));
}

// This could be more simply written as "number of legal moves = 0", if the
// enumeration of all legal moves is implemented at some point.
// As it is we don't have that, so this is simpler.
bool checkmate(Board *board, Player p)
{
	// We must be in check
	if (!in_check(board, p))
		return false;

	Square king_location = find_king(board, p);
	Player other = OTHER_PLAYER(p);
	int x = SQUARE_X(king_location);
	int y = SQUARE_Y(king_location);

	// Can the king move out of check?
	for (int dx = -1; dx < 2; dx++) {
		for (int dy = -1; dy < 2; dy++) {
			if (x + dx < 0 || x + dx >= BOARD_SIZE ||
					y + dy < 0 || y + dy >= BOARD_SIZE)
				continue;
			Move m = MOVE(king_location, SQUARE(x + dx, y + dy));
			if (legal_move(board, m, true))
				return false;
		}
	}

	// Can the attacking piece be captured?
	Square attacker = find_attacking_piece(board, king_location, other);
	if (under_attack(board, attacker, p))
		return false;

	// Can we block?
	Piece_type type = PIECE_TYPE(PIECE_AT_SQUARE(board, attacker));
	if (type != KNIGHT) {
		int dx = SQUARE_X(attacker) - x;
		int dy = SQUARE_Y(attacker) - y;

		int ax = abs(dx);
		int ay = abs(dy);

		int x_direction = ax == 0 ? 0 : dx / ax;
		int y_direction = ay == 0 ? 0 : dy / ay;

		uint x = SQUARE_X(king_location) + x_direction;
		uint y = SQUARE_Y(king_location) + y_direction;
		while (!(x == SQUARE_X(attacker) &&
					y == SQUARE_Y(attacker))) {
			Square blocker = find_piece_looking_at(board, SQUARE(x, y), p);
			if (blocker != NULL_SQUARE &&
					PIECE_TYPE(PIECE_AT_SQUARE(board, blocker)) != KING)
				return false;

			x += x_direction;
			y += y_direction;
		}
	}

	// All outta luck
	return true;
}

bool can_castle_kingside(Board *board, Player p)
{
	uint y = p == WHITE ? 0 : BOARD_SIZE - 1;
	Player other = OTHER_PLAYER(p);

	return board->castling[p].kingside && !in_check(board, p) &&
		PIECE_AT(board, 5, y) == EMPTY &&
		PIECE_AT(board, 6, y) == EMPTY &&
		!under_attack(board, SQUARE(5, y), other) &&
		!under_attack(board, SQUARE(6, y), other);
}

bool can_castle_queenside(Board *board, Player p)
{
	uint y = p == WHITE ? 0 : BOARD_SIZE - 1;
	Player other = OTHER_PLAYER(p);

	return board->castling[p].kingside && !in_check(board, p) &&
		PIECE_AT(board, 3, y) == EMPTY &&
		PIECE_AT(board, 2, y) == EMPTY &&
		PIECE_AT(board, 1, y) == EMPTY &&
		!under_attack(board, SQUARE(3, y), other) &&
		!under_attack(board, SQUARE(2, y), other) &&
		!under_attack(board, SQUARE(1, y), other);
}
