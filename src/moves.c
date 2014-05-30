#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "board.h"

// TODO
// * Pawns capturing
// * Moving into check
// * Castling
// * En passant
// * Initial pawn move
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

	if (PIECE_TYPE(p) != KNIGHT) {
		int path_dx = ax == 0 ? 0 : dx / ax;
		int path_dy = ay == 0 ? 0 : dy / ay;

		int file = SQUARE_FILE(start) + path_dx;
		int rank = SQUARE_RANK(start) + path_dy;
		while (!(file == SQUARE_FILE(end) && rank == SQUARE_RANK(end))) {
			if (PIECE_AT(board, file, rank) != EMPTY)
				return false;

			file += path_dx;
			rank += path_dy;
		}
	}

	switch (PIECE_TYPE(p)) {
	case KNIGHT: return (ax == 1 && ay == 2) || (ax == 2 && ay == 1);
	case PAWN:
		if (dy != (PLAYER(p) == WHITE ? 1 : -1))
			return false;

		if (dx == 0)
			return true;

		if (dx == 1 || dx == -1)
			return at_end_square != EMPTY && PLAYER(at_end_square) != PLAYER(p);

		return false;
	case BISHOP: return ax == ay;
	case ROOK: return dx == 0 || dy == 0;
	case QUEEN: return ax == ay || dx == 0 || dy == 0;
	case KING: return ax <= 1 && ay <= 1;
	case EMPTY: return false;
	}

	return false;
}
