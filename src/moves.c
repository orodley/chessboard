#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include "moves.h"

void perform_move(Board *board, Move move)
{
	if (!legal_move(board, move))
		return;

	Square start = START_SQUARE(move);
	Square end = END_SQUARE(move);

	Piece p = PIECE_AT_SQUARE(board, start);
	if (PIECE_TYPE(p) == PAWN && end == board->en_passant)
		PIECE_AT(board, SQUARE_FILE(end), PLAYER(p) == WHITE ? 4 : 3) = EMPTY;

	uint dy = SQUARE_RANK(end) - SQUARE_RANK(start);
	if (PIECE_TYPE(p) == PAWN && abs(dy) == 2) {
		int en_passant_rank = PLAYER(p) == WHITE ? 2 : 5;
		board->en_passant = SQUARE(SQUARE_FILE(start), en_passant_rank);
	} else {
		board->en_passant = NULL_SQUARE;
	}

	PIECE_AT_SQUARE(board, end) = PIECE_AT_SQUARE(board, start);
	PIECE_AT_SQUARE(board, start) = EMPTY;
}

// TODO
// * Moving into check
// * Castling
bool legal_move(Board *board, Move move)
{
	Square start = START_SQUARE(move);
	Square end = END_SQUARE(move);
	int dx = SQUARE_FILE(end) - SQUARE_FILE(start);
	int dy = SQUARE_RANK(end) - SQUARE_RANK(start);
	Piece p = PIECE_AT_SQUARE(board, start);
	Piece at_end_square = PIECE_AT_SQUARE(board, end);

	if (p == EMPTY || (at_end_square != EMPTY && PLAYER(at_end_square) == PLAYER(p)))
		return false;
	
	if (dx == 0 && dy == 0)
		return false;

	int ax = abs(dx);
	int ay = abs(dy);

	int x_direction = ax == 0 ? 0 : dx / ax;
	int y_direction = ay == 0 ? 0 : dy / ay;

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

	switch (PIECE_TYPE(p)) {
	case KNIGHT: return (ax == 1 && ay == 2) || (ax == 2 && ay == 1);
	case PAWN:
		if ((PLAYER(p) == WHITE && SQUARE_RANK(start) == 1) ||
			(PLAYER(p) == BLACK && SQUARE_RANK(start) == 6)) {
			if (ay != 1 && ay != 2)
				return false;
		} else if (ay != 1) {
			return false;
		}

		if (y_direction != (PLAYER(p) == WHITE ? 1 : -1))
			return false;

		if (dx == 0)
			return true;

		if (dx == 1 || dx == -1)
			return (at_end_square != EMPTY &&
						PLAYER(at_end_square) != PLAYER(p)) ||
					end == board->en_passant;

		return false;
	case BISHOP: return ax == ay;
	case ROOK: return dx == 0 || dy == 0;
	case QUEEN: return ax == ay || dx == 0 || dy == 0;
	case KING: return ax <= 1 && ay <= 1;
	case EMPTY: return false;
	}

	return false;
}
