#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "moves.h"

// Assumes the move is legal. This is necessary as the easiest way to test
// whether a move doesn't put the moving player in check (illegal) is to
// perform the move and then test if they are in check.
void perform_move(Board *board, Move move)
{
	Square start = START_SQUARE(move);
	Square end = END_SQUARE(move);
	Piece p = PIECE_AT_SQUARE(board, start);
	Piece_type type = PIECE_TYPE(p);

	board->half_move_clock++;
	if (PLAYER(p) == BLACK)
		board->move_number++;

	// Check if we're capturing en passant
	if (type == PAWN && end == board->en_passant)
		PIECE_AT(board, SQUARE_X(end), PLAYER(p) == WHITE ? 4 : 3) = EMPTY;

	// Check if this move enables our opponent to perform en passant
	uint dy = SQUARE_Y(end) - SQUARE_Y(start);
	if (type == PAWN && abs(dy) == 2) {
		int en_passant_rank = PLAYER(p) == WHITE ? 2 : 5;
		board->en_passant = SQUARE(SQUARE_X(start), en_passant_rank);
	} else {
		// Otherwise reset en passant
		board->en_passant = NULL_SQUARE;
	}

	// Check if we're castling so we can move the rook too
	uint dx = SQUARE_X(end) - SQUARE_X(start);
	if (type == KING && abs(dx) > 1) {
		uint y = PLAYER(p) == WHITE ? 0 : 7;
		bool kingside = SQUARE_X(end) == 6;
		if (kingside) {
			PIECE_AT(board, 7, y) = EMPTY;
			PIECE_AT(board, 5, y) = PIECE(PLAYER(p), ROOK);
		} else {
			PIECE_AT(board, 0, y) = EMPTY;
			PIECE_AT(board, 3, y) = PIECE(PLAYER(p), ROOK);
		}
	}

	// Check if we're depriving ourself of castling rights
	Castling *c = &board->castling[PLAYER(p)];
	if (type == KING) {
		c->kingside = false;
		c->queenside = false;
	} else if (type == ROOK) {
		if (SQUARE_X(start) == 7) {
			c->kingside = false;
		} else if (SQUARE_X(start) == 0) {
			c->queenside = false;
		}
	}

	// Check if we should reset the half-move clock
	if (type == PAWN || PIECE_AT_SQUARE(board, end) != EMPTY)
		board->half_move_clock = 0;

	// Update the turn tracker
	board->turn = 1 - board->turn; // (0, 1) = (BLACK, WHITE) & 0 -> 1, 1 -> 0

	PIECE_AT_SQUARE(board, end) = PIECE_AT_SQUARE(board, start);
	PIECE_AT_SQUARE(board, start) = EMPTY;
}

bool legal_move(Board *board, Move move, bool check_for_check)
{
	Square start = START_SQUARE(move);
	Square end = END_SQUARE(move);
	int dx = SQUARE_X(end) - SQUARE_X(start);
	int dy = SQUARE_Y(end) - SQUARE_Y(start);
	Piece p = PIECE_AT_SQUARE(board, start);
	Piece_type type = PIECE_TYPE(p);
	Piece at_end_square = PIECE_AT_SQUARE(board, end);

	// Can't move a piece that isn't there
	if (p == EMPTY)
		return false;

	// Can only move if it's your turn
	if (PLAYER(p) != board->turn)
		return false;

	// Can't capture your own pieces
	if (at_end_square != EMPTY && PLAYER(at_end_square) == PLAYER(p))
		return false;
	
	// Can't "move" a piece by putting it back onto the same square
	if (dx == 0 && dy == 0)
		return false;

	int ax = abs(dx);
	int ay = abs(dy);

	int x_direction = ax == 0 ? 0 : dx / ax;
	int y_direction = ay == 0 ? 0 : dy / ay;

	// Pieces other than knights are blocked by intervening pieces
	if (type != KNIGHT) {
		uint x = SQUARE_X(start) + x_direction;
		uint y = SQUARE_Y(start) + y_direction;

		while ((!(x == SQUARE_X(end) && y == SQUARE_Y(end))) &&
				x < BOARD_SIZE && y < BOARD_SIZE) {
			if (PIECE_AT(board, x, y) != EMPTY)
				return false;

			x += x_direction;
			y += y_direction;
		}
	}

	// Now handle each type of movement
	bool legal_movement = false;
	switch (type) {
	case PAWN:
		if ((PLAYER(p) == WHITE && SQUARE_Y(start) == 1) ||
			(PLAYER(p) == BLACK && SQUARE_Y(start) == 6)) {
			if (ay != 1 && ay != 2) {
				legal_movement = false;
				break;
			}
		} else if (ay != 1) {
			legal_movement = false;
			break;
		}

		if (y_direction != (PLAYER(p) == WHITE ? 1 : -1)) {
			legal_movement = false;
			break;
		}

		if (dx == 0) {
			legal_movement = at_end_square == EMPTY;
			break;
		}

		if (dx == 1 || dx == -1) {
			legal_movement = (at_end_square != EMPTY &&
					PLAYER(at_end_square) != PLAYER(p)) ||
					end == board->en_passant;
			break;
		}

		legal_movement = false;
		break;
	case KNIGHT:
		legal_movement = (ax == 1 && ay == 2) || (ax == 2 && ay == 1);
		break;
	case BISHOP: legal_movement = ax == ay; break;
	case ROOK:   legal_movement = dx == 0 || dy == 0; break;
	case QUEEN:  legal_movement = ax == ay || dx == 0 || dy == 0; break;
	case KING:
		if (ax <= 1 && ay <= 1) {
			legal_movement = true;
			break;
		}

		if (SQUARE_Y(end) != (PLAYER(p) == WHITE ? 0 : 7)) {
			legal_movement = false;
			break;
		}

		if (SQUARE_X(end) == 6) {
			legal_movement = can_castle_kingside(board, PLAYER(p));
			break;
		} else if (SQUARE_X(end) == 2) {
			legal_movement = can_castle_queenside(board, PLAYER(p));
			break;
		} else {
			legal_movement = false;
			break;
		}
	case EMPTY:  legal_movement = false; break;
	}

	if (!legal_movement)
		return false;

	// At this point everything looks fine. The only thing left to check is
	// whether the move puts us in check. We've checked enough of the move
	// that perform_move should be able to handle it.
	if (check_for_check)
		return !gives_check(board, move, PLAYER(p));
	else
		return legal_movement;
}

bool gives_check(Board *board, Move move, Player player)
{
	Board copy;
	copy_board(&copy, board);
	perform_move(&copy, move);

	return in_check(&copy, player);
}

bool gives_mate(Board *board, Move move, Player player)
{
	Board copy;
	copy_board(&copy, board);
	perform_move(&copy, move);

	return checkmate(&copy, player);
}

// Check whether we need to disambiguate between two pieces for a particular
// move. e.g.: there are two rooks that can move to the square. If so, return
// the location of the other piece that is confusing things.
Square ambiguous_piece(Board *board, Move move)
{
	Piece_type type = PIECE_TYPE(PIECE_AT_SQUARE(board, START_SQUARE(move)));
	for (uint x = 0; x < BOARD_SIZE; x++) {
		for (uint y = 0; y < BOARD_SIZE; y++) {
			Square curr_square = SQUARE(x, y);
			if (curr_square == START_SQUARE(move))
				continue;
			if (PIECE_TYPE(PIECE_AT_SQUARE(board, curr_square)) == type &&
					legal_move(board, MOVE(curr_square, END_SQUARE(move)), true))
				return curr_square;
		}
	}

	return NULL_SQUARE;
}

// str should have space for at least 7 (MAX_NOTATION_LENGTH) characters, to
// be able to fit the longest of moves. e.g.: Raxd1+\0
void move_notation(Board *board, Move move, char *str)
{
	uint i = 0;
	Square start = START_SQUARE(move);
	Square end = END_SQUARE(move);
	Piece p = PIECE_AT_SQUARE(board, start);
	Piece_type type = PIECE_TYPE(p);

	// Castling
	// TODO: Castling can cause a check or a mate - needs +/#
	if (type == KING && abs(SQUARE_X(start) - SQUARE_X(end)) > 1) {
		if (SQUARE_X(end) == 6)
			strcpy(str, "O-O");
		else
			strcpy(str, "O-O-O");

		return;
	}

	bool capture = PIECE_AT_SQUARE(board, END_SQUARE(move)) != EMPTY;

	// Add the letter denoting the type of piece moving
	if (type != PAWN)
		str[i++] = "\0\0NBRQK"[type];

	// Add the number/letter of the rank/file of the moving piece if necessary
	Square ambig = ambiguous_piece(board, move);
	// We always add the file if it's a pawn capture
	if (ambig != NULL_SQUARE || (type == PAWN && capture)) {
		char disambiguate;
		if (SQUARE_X(ambig) == SQUARE_X(start))
			disambiguate = RANK_CHAR(SQUARE_Y(start));
		else
			disambiguate = FILE_CHAR(SQUARE_X(start));

		str[i++] = disambiguate;
	}

	// Add an 'x' if its a capture
	if (capture)
		str[i++] = 'x';

	// Add the target square
	str[i++] = FILE_CHAR(SQUARE_X(end));
	str[i++] = RANK_CHAR(SQUARE_Y(end));

	// Add a '#' if its mate
	if (gives_mate(board, move, OTHER_PLAYER(PLAYER(p))))
		str[i++] = '#';
	// Add a '+' if its check
	else if (gives_check(board, move, OTHER_PLAYER(PLAYER(p))))
		str[i++] = '+';

	str[i++] = '\0';
}
