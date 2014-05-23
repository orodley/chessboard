#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include "board.h"
#include "misc.h"

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
	for (uint rank = 0; rank < BOARD_SIZE; rank++) {
		putchar('.');
		for (uint file = 0; file < BOARD_SIZE; file++) {
			putchar(char_from_piece(PIECE_AT(b, file, rank)));
		}

		puts(".");
	}
	puts("..........");
	printf("%s to move\n", b->turn == WHITE ? "White" : "Black");
}
