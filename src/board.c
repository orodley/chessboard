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

	for (uint file = 0; file < BOARD_SIZE; file++)
		for (uint rank = 0; rank < BOARD_SIZE; rank++)
			PIECE_AT(dst, file, rank) = PIECE_AT(src, file, rank);
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

	for (int rank = 7; rank >= 0; rank--) {
		char c;
		uint file = 0;
		while ((c = fen_str[i++]) != '/' && c != ' ') {
			if (isdigit(c)) {
				if (c == '0') // "Skip zero files" makes no sense
					return false;

				for (int n = 0; n < c - '0'; n++)
					PIECE_AT(board, file + n, rank) = EMPTY;

				file += c - '0';
				continue;
			} else {
				PIECE_AT(board, file, rank) = piece_from_char(c);
			}

			file++;
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
		char rank_char = fen_str[i++];
		if (rank_char < 'a' || rank_char > 'g')
			return false;

		rank_char = fen_str[i++];
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
	for (int rank = 7; rank >= 0; rank--) {
		putchar('.');
		for (uint file = 0; file < BOARD_SIZE; file++) {
			putchar(char_from_piece(PIECE_AT(b, file, rank)));
		}

		puts(".");
	}
	puts("..........");
	printf("%s to move\n", b->turn == WHITE ? "White" : "Black");
}

Square find_piece_looking_at(Board *board, Square square, Player piece_owner)
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

	for (uint rank = 0; rank < BOARD_SIZE; rank++) {
		for (uint file = 0; file < BOARD_SIZE; file++) {
			Piece p = PIECE_AT(board, file, rank);
			Square s = SQUARE(file, rank);
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

Square find_attacking_piece(Board *board, Square square, Player attacker)
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

bool under_attack(Board *board, Square square, Player attacker)
{
	return find_attacking_piece(board, square, attacker) != NULL_SQUARE;
}

Square find_king(Board *board, Player p)
{
	Piece king = PIECE(p, KING);

	for (uint rank = 0; rank < BOARD_SIZE; rank++)
		for (uint file = 0; file < BOARD_SIZE; file++)
			if (PIECE_AT(board, file, rank) == king)
				return SQUARE(file, rank);

	return NULL_SQUARE;
}

bool in_check(Board *board, Player p)
{
	Square king_location = find_king(board, p);
	assert(king_location != NULL_SQUARE); // both players should have a king
	return under_attack(board, king_location, OTHER_PLAYER(p));
}

bool checkmate(Board *board, Player p)
{
	// We must be in check
	if (!in_check(board, p))
		return false;

	Square king_location = find_king(board, p);
	Player other = OTHER_PLAYER(p);
	int file = SQUARE_FILE(king_location);
	int rank = SQUARE_RANK(king_location);

	// Can the king move out of check?
	for (int dx = -1; dx < 2; dx++) {
		for (int dy = -1; dy < 2; dy++) {
			if (file + dx < 0 || file + dx >= BOARD_SIZE ||
					rank + dy < 0 || rank + dy >= BOARD_SIZE)
				continue;
			Move m = MOVE(king_location, SQUARE(file + dx, rank + dy));
			if (legal_move(board, m, true))
				return false;
		}
	}

	// Can the attacking piece be taken?
	Square attacker = find_attacking_piece(board, king_location, other);
	if (under_attack(board, attacker, p))
		return false;

	// Can we block?
	Piece_type type = PIECE_TYPE(PIECE_AT_SQUARE(board, attacker));
	if (type != KNIGHT) {
		int dx = SQUARE_FILE(attacker) - file;
		int dy = SQUARE_RANK(attacker) - rank;

		int ax = abs(dx);
		int ay = abs(dy);

		int x_direction = ax == 0 ? 0 : dx / ax;
		int y_direction = ay == 0 ? 0 : dy / ay;

		uint file = SQUARE_FILE(king_location) + x_direction;
		uint rank = SQUARE_RANK(king_location) + y_direction;
		while (!(file == SQUARE_FILE(attacker) &&
					rank == SQUARE_RANK(attacker))) {
			Square blocker = find_piece_looking_at(board, SQUARE(file, rank), p);
			if (blocker != NULL_SQUARE &&
					PIECE_TYPE(PIECE_AT_SQUARE(board, blocker)) != KING)
				return false;

			file += x_direction;
			rank += y_direction;
		}
	}

	// All outta luck
	return true;
}

bool can_castle_kingside(Board *board, Player p)
{
	uint rank = p == WHITE ? 0 : 7;
	Player other = OTHER_PLAYER(p);

	return board->castling[p].kingside && !in_check(board, p) &&
		PIECE_AT(board, 5, rank) == EMPTY &&
		PIECE_AT(board, 6, rank) == EMPTY &&
		!under_attack(board, SQUARE(5, rank), other) &&
		!under_attack(board, SQUARE(6, rank), other);
}

bool can_castle_queenside(Board *board, Player p)
{
	uint rank = p == WHITE ? 0 : 7;
	Player other = OTHER_PLAYER(p);

	return board->castling[p].kingside && !in_check(board, p) &&
		PIECE_AT(board, 3, rank) == EMPTY &&
		PIECE_AT(board, 2, rank) == EMPTY &&
		PIECE_AT(board, 1, rank) == EMPTY &&
		!under_attack(board, SQUARE(3, rank), other) &&
		!under_attack(board, SQUARE(2, rank), other) &&
		!under_attack(board, SQUARE(1, rank), other);
}
