#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "board.h"
#include "game.h"

Game *new_game()
{
	Game *game = malloc(sizeof(Game));

	game->move = NULL_MOVE;
	game->board = NULL;
	game->parent = NULL;

	game->children = malloc(sizeof(Game_list));
	game->children->game = NULL;
	game->children->next = NULL;

	return game;
}

Game *add_child(Game *game, Move move, Board *board)
{
	game->move = move;

	Game_list *children = game->children;
	while (children->game != NULL && children->next != NULL &&
			children->game->move != move)
		children = children->next;

	if (children->game != NULL && children->game->move == move) {
		return children->game;
	} else {
		Game *new_node = new_game();
		new_node->board = board;
		new_node->parent = game;

		if (children->game == NULL) {
			children->game = new_node;
		} else {
			Game_list *list = malloc(sizeof *list);
			list->game = new_node;
			children->next = list;
		}

		return new_node;
	}
}

Game *first_child(Game *game)
{
	return game->children->game;
}

bool has_children(Game *game)
{
	return game->children != NULL && game->children->game != NULL;
}

void free_game(Game *game)
{
	Game_list *children = game->children;
	while (children->game != NULL && children->next != NULL) {
		free_game(children->game);
	}

	free(game->children);
	free(game->board);
}
