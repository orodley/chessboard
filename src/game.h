#ifndef GAME_H_
#define GAME_H_

#include "board.h"
#include "moves.h"

// Games are represented as trees where each node has an arbitrary number of
// children. This is to make variations easy to store. Linked lists are used
// to store the children for ease of inserting new elements.

struct Game;

typedef struct Game_list
{
	struct Game      *game;
	struct Game_list *next;
} Game_list;

typedef struct Game
{
	Move move;
	Board *board;
	struct Game_list *children;
	struct Game *parent;
} Game;

Game *new_game();
Game *add_child(Game *game, Move move, Board *board);

#endif // include guard
