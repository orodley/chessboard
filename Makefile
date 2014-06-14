NAME := chessboard

CC ?= gcc
CFLAGS := -Wall -Wextra -Werror -std=c99 -pedantic -DPROG_NAME=\"$(NAME)\" \
		 -Isrc/
CFLAGS += $(shell pkg-config --cflags --libs gtk+-3.0 librsvg-2.0)

ifdef DEBUG
	CFLAGS += -g
endif

OBJS := $(patsubst %.c,  %.o, $(wildcard src/*.c))
OBJS += $(patsubst %.rl, %.o, $(wildcard src/*.rl))

GENERATED_FILES := $(patsubst %.rl, %.c, $(wildcard src/*.rl))

.PHONY: all clean test test/pgn

all: $(NAME)

$(NAME): $(OBJS) main.o
	$(CC) $^ $(CFLAGS) -o $@
	[ -z "$$DEBUG" ] || ctags -R

%.c: %.rl
	ragel -C $<

clean:
	rm -f $(NAME) src/*.o
	rm -f $(GENERATED_FILES)
	rm -f tags

test: test/pgn

test/pgn: test/pgn/run-tests.sh test/pgn/test-pgn
	@test/pgn/run-tests.sh

test/pgn/test-pgn: test/pgn/test-pgn.c $(OBJS)
	$(CC) $^ $(CFLAGS) -o $@
