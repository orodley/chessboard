#ifndef GAME_H_
#define GAME_H_

#include "board.h"
#include "moves.h"

// Games are represented as trees where each node has an arbitrary number of
// children. This is to make variations easy to store. Linked lists are used
// to store the children for ease of inserting new elements.

struct Game;

typedef struct Game
{
	Move move;
	Board *board;

	struct Game *parent;
	struct Game *children;
	struct Game *sibling;
} Game;

Game *new_game();
Game *add_child(Game *game, Move move);
Game *first_child(Game *game);
Game *root_node(Game *game);
Game *last_node(Game *game);
bool has_children(Game *game);
void free_game(Game *game);

#endif // include guard
