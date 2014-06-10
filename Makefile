NAME = chessboard

CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -std=c99 -pedantic -DPROG_NAME=\"$(NAME)\" \
		 -Isrc/
CFLAGS += $(shell pkg-config --cflags --libs gtk+-3.0 librsvg-2.0)

OBJS := $(patsubst %.c,  %.o, $(wildcard src/*.c))
OBJS += $(patsubst %.rl, %.o, $(wildcard src/*.rl))

GENERATED_FILES := $(patsubst %.rl, %.c, $(wildcard src/*.rl))

.PHONY: all clean test test/pgn

all: $(NAME)

$(NAME): $(OBJS) main.o
	$(CC) $(CFLAGS) $^ -o $@

%.c: %.rl
	ragel -C $<

clean:
	rm -f $(NAME) src/*.o
	rm -f $(GENERATED_FILES)

test: test/pgn

test/pgn: test/pgn/run-tests.sh test/pgn/test-pgn
	@test/pgn/run-tests.sh

test/pgn/test-pgn: test/pgn/test-pgn.c $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@
