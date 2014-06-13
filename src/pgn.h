#include <glib.h>
#include "game.h"

// PGN spec, section 7
// Numbers are one higher to include null-termination
#define MAX_SYMBOL_LENGTH 256
#define MAX_STRING_LENGTH 256

typedef enum Result
{
	WHITE_WINS,
	BLACK_WINS,
	DRAW,
	OTHER,
} Result;

#define NULL_RESULT ((Result)~((Result)0))

typedef struct PGN
{
	GHashTable *tags;
	Result result;
	
	Game *game;
} PGN;

bool read_pgn(PGN *pgn, const char *input_filename, GError **error);
bool write_pgn(PGN *pgn, FILE *file);
