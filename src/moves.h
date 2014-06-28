#ifndef MOVES_H_
#define MOVES_H_
//
// A move is represented as 4 bytes, with the start square in the two most
// significant bytes, and the end square in the two least significant
// bytes. Each pair of bytes has the file in the most significant byte, and
// the rank in the least significant byte.
typedef uint_fast32_t Move;
#define MOVE(start, end) ((Move)(((start) << 16) | (end)))
#define START_SQUARE(m)  ((m) >> 16)
#define END_SQUARE(m)    ((m) & 0xFFFF)

#define NULL_MOVE ((Move)(~((Move)0)))


#define FILE_CHAR(file) ('a' + (file))
#define CHAR_FILE(c)    ((c) - 'a')
#define RANK_CHAR(rank) ('1' + (rank))
#define CHAR_RANK(c)    ((c) - '1')


void perform_move(Board *board, Move move);
bool legal_move(Board *board, Move move, bool check_for_check);
bool gives_check(Board *board, Move move, Player player);

// Longest possible length of a move in algebraic notation.
// e.g. Raxd1+\0
#define MAX_ALGEBRAIC_NOTATION_LENGTH 7
void algebraic_notation_for(Board *board, Move move, char *str);

#endif // include guard
