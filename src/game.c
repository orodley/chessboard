#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "board.h"
#include "game.h"

Game *new_game()
{
	Game *game = malloc(sizeof(Game));

	game->move     = NULL_MOVE;
	game->board    = NULL;
	game->parent   = NULL;
	game->children = NULL;
	game->sibling  = NULL;

	return game;
}

Game *add_child(Game *game, Move move, Board *board)
{
	Game *children = game->children;

	while (children != NULL && children->sibling != NULL &&
			children->move != move)
		children = children->sibling;

	if (children != NULL && children->move == move) {
		return children;
	} else {
		Game *new_node = new_game();
		new_node->move = move;
		new_node->board = board;
		new_node->parent = game;

		if (children == NULL)
			game->children = new_node;
		else
			children->sibling = new_node;

		return new_node;
	}
}

Game *first_child(Game *game)
{
	return game->children;
}

Game *root_node(Game *game)
{
	while (game->parent != NULL)
		game = game->parent;

	return game;
}

Game *last_node(Game *game)
{
	while (game->children != NULL)
		game = game->children;

	return game;
}

bool has_children(Game *game)
{
	return game->children != NULL;
}

void free_game(Game *game)
{
	Game *children = game->children;
	while (children != NULL && children->sibling != NULL)
		free_game(children);

	free(game->board);
}
