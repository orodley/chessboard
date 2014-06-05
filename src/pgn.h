#include <glib.h>
#include "game.h"

#define MAX_SYMBOL_LENGTH 255 // PGN spec, section 7

typedef enum Result
{
	WHITE_WINS,
	BLACK_WINS,
	DRAW,
	OTHER,
} Result;

typedef struct PGN
{
	GHashTable *tags;
	Result result;
	
	Game *game;
} PGN;

bool read_pgn(PGN *pgn, const char *input_filename, GError **error);
