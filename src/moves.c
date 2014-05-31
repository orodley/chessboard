#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include "moves.h"

// TODO: Increment/reset half-move clock
// Assumes the move is legal. This is necessary as the easiest way to test
// whether a move doesn't put the moving player in check (illegal) is to
// perform the move and then test if they are in check.
void perform_move(Board *board, Move move)
{
	Square start = START_SQUARE(move);
	Square end = END_SQUARE(move);

	// Check if we're capturing en passant
	Piece p = PIECE_AT_SQUARE(board, start);
	if (PIECE_TYPE(p) == PAWN && end == board->en_passant)
		PIECE_AT(board, SQUARE_FILE(end), PLAYER(p) == WHITE ? 4 : 3) = EMPTY;

	// Check if this move enables our opponent to perform en passant
	uint dy = SQUARE_RANK(end) - SQUARE_RANK(start);
	if (PIECE_TYPE(p) == PAWN && abs(dy) == 2) {
		int en_passant_rank = PLAYER(p) == WHITE ? 2 : 5;
		board->en_passant = SQUARE(SQUARE_FILE(start), en_passant_rank);
	} else {
		// Otherwise reset en passant
		board->en_passant = NULL_SQUARE;
	}

	// Check if we're castling so we can move the rook too
	uint dx = SQUARE_FILE(end) - SQUARE_FILE(start);
	if (PIECE_TYPE(p) == KING && abs(dx) > 1) {
		uint rank = PLAYER(p) == WHITE ? 0 : 7;
		bool kingside = SQUARE_FILE(end) == 6;
		if (kingside) {
			PIECE_AT(board, 7, rank) = EMPTY;
			PIECE_AT(board, 5, rank) = PIECE(PLAYER(p), ROOK);
		} else {
			PIECE_AT(board, 0, rank) = EMPTY;
			PIECE_AT(board, 3, rank) = PIECE(PLAYER(p), ROOK);
		}
	}

	// Check if we're depriving ourself of castling rights
	Castling *c = &board->castling[PLAYER(p)];
	if (PIECE_TYPE(p) == KING) {
		c->kingside = false;
		c->queenside = false;
	} else if (PIECE_TYPE(p) == ROOK) {
		if (SQUARE_FILE(start) == 7) {
			c->kingside = false;
		} else if (SQUARE_FILE(start) == 0) {
			c->queenside = false;
		}
	}

	// Update the turn tracker
	board->turn = 1 - board->turn; // (0, 1) = (BLACK, WHITE) & 0 -> 1, 1 -> 0

	PIECE_AT_SQUARE(board, end) = PIECE_AT_SQUARE(board, start);
	PIECE_AT_SQUARE(board, start) = EMPTY;
}

bool legal_move(Board *board, Move move, bool check_for_check)
{
	Square start = START_SQUARE(move);
	Square end = END_SQUARE(move);
	int dx = SQUARE_FILE(end) - SQUARE_FILE(start);
	int dy = SQUARE_RANK(end) - SQUARE_RANK(start);
	Piece p = PIECE_AT_SQUARE(board, start);
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
	if (PIECE_TYPE(p) != KNIGHT) {
		int file = SQUARE_FILE(start) + x_direction;
		int rank = SQUARE_RANK(start) + y_direction;
		while (!(file == SQUARE_FILE(end) && rank == SQUARE_RANK(end))) {
			if (PIECE_AT(board, file, rank) != EMPTY)
				return false;

			file += x_direction;
			rank += y_direction;
		}
	}

	// Now handle each type of movement
	bool legal_movement = false;
	switch (PIECE_TYPE(p)) {
	case PAWN:
		if ((PLAYER(p) == WHITE && SQUARE_RANK(start) == 1) ||
			(PLAYER(p) == BLACK && SQUARE_RANK(start) == 6)) {
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

		if (SQUARE_RANK(end) != (PLAYER(p) == WHITE ? 0 : 7)) {
			legal_movement = false;
			break;
		}

		if (SQUARE_FILE(end) == 6) {
			legal_movement = can_castle_kingside(board, PLAYER(p));
			break;
		} else if (SQUARE_FILE(end) == 2) {
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
	if (check_for_check) {
		Board copy;
		copy_board(&copy, board);
		perform_move(&copy, move);

		return !in_check(&copy, PLAYER(p));
	} else {
		return legal_movement;
	}
}
