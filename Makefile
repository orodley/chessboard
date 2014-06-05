NAME = chessboard

CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -std=c99 -pedantic -DPROG_NAME=\"$(NAME)\" \
		 -Isrc/
CFLAGS += $(shell pkg-config --cflags --libs gtk+-3.0 librsvg-2.0)

OBJS := $(patsubst %.c, %.o, $(wildcard src/*.c))

.PHONY: all clean test test/pgn

all: $(NAME)

$(NAME): $(OBJS) main.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(NAME) src/*.o

test: test/pgn

test/pgn: test/pgn/run-tests.sh test/pgn/test-pgn
	test/pgn/run-tests.sh

test/pgn/test-pgn: test/pgn/test-pgn.c $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@
