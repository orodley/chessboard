#ifndef BOARD_H_
#define BOARD_H_

#include <stdbool.h>
#include <stdint.h>
#include "misc.h"

#define BOARD_SIZE 8
#define PLAYERS 2

// A move is represented as 4 bytes, with the start square in the two most
// significant bytes, and the end square in the two least significant
// bytes. Each pair of bytes has the file in the most significant byte, and
// the rank in the least significant byte.
typedef uint_least32_t Move;
#define MOVE(start, end) ((Move)(((start) << 16) | (end)))
#define START_SQUARE(m)  ((m) >> 16)
#define END_SQUARE(m)    ((m) & 0xFFFF)

#define NULL_MOVE ((Move)(~((Move)0)))

typedef uint_least16_t Square;
#define SQUARE(file, rank) (((file) << 8) | (rank))
#define SQUARE_FILE(s) ((s) >> 8)
#define SQUARE_RANK(s) ((s) & 0xF)

#define NULL_SQUARE ((Square)(~((Square)0)))

// Pieces are represented as shorts, with the MSB used to store the color, and
// the rest equal to one of a bunch of constants for the type of piece.
typedef unsigned short Piece;

typedef enum Player
{
	BLACK = 0,
	WHITE = 1,
} Player;

#define PIECE_TYPES 6

// The order of these matters
typedef enum Piece_type
{
	EMPTY,
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING,
} Piece_type;

#define PLAYER(x) ((Player)((x) >> (sizeof(Piece) * 8 - 1)))
#define PIECE_TYPE(x) ((Piece_type)((x) & ~(1 << (sizeof(Piece) * 8 - 1))))
#define PIECE(p, t) ((Piece)(((p) << (sizeof(Piece) * 8 - 1)) | (t)))
#define NULL_PIECE ((unsigned short)-1)


typedef struct Castling
{
	bool kingside;
	bool queenside;
} Castling;


// The board is an array of pieces, plus some other information:
// * Whose turn it is
// * Castling availibility
// * En passant target square (if any)
// * Halfmoves since the last capture or pawn advance
// * Move number
//
// This is similar to FEN.
// The intention behind having all this information in the board object is so
// that no other information is required to determine any necessary information
// about a board position.
//
// However, this structure does not contain information necessary to determine
// if a draw can be claimed by threefold repetition. This is unlikely to
// matter, as the most common case of threefold repetition is perpetual check,
// in which case coming in part-way through does not matter. The other cases
// are rare enough that it doesn't seem worthwhile to overly complicate the
// board representation over them.
typedef struct Board
{
	Player turn;
	Castling castling[PLAYERS];
	Square en_passant;
	uint half_move_clock;
	uint move_number;

	Piece pieces[BOARD_SIZE * BOARD_SIZE];
} Board;

#define PIECE_AT(b, file, rank) ((b)->pieces[((rank) * BOARD_SIZE) + (file)])
#define PIECE_AT_SQUARE(b, square) PIECE_AT(b, SQUARE_FILE(square), SQUARE_RANK(square))

void copy_board(Board *dst, Board *src);
Piece piece_from_char(char c);
char char_from_piece(Piece p);
bool from_fen(Board *board, const char *fen_str);
void print_board(Board *b);
bool under_attack(Board *board, Square square, Player attacker);
bool in_check(Board *board, Player p);
bool can_castle_kingside(Board *board, Player p);
bool can_castle_queenside(Board *board, Player p);


extern char *start_board_fen;

#endif // include guard
