#ifndef MOVES_H_
#define MOVES_H_

void perform_move(Board *board, Move move);
bool legal_move(Board *board, Move move, bool check_for_check);

#endif // include guard
