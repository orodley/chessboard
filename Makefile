PROG_NAME := chessboard

CC ?= gcc
CFLAGS := -Wall -Wextra -Werror -std=c99 -pedantic \
          -DPROG_NAME=\"$(PROG_NAME)\" -Isrc/
CFLAGS += $(shell pkg-config --cflags gtk+-3.0 librsvg-2.0)

LINK_FLAGS := $(shell pkg-config --libs gtk+-3.0 librsvg-2.0)

ifdef DEBUG
	CFLAGS += -g
endif

OBJS := $(patsubst %.c,  %.o, $(wildcard src/*.c))
OBJS += $(patsubst %.rl, %.o, $(wildcard src/*.rl))

GENERATED_FILES := $(patsubst %.rl, %.c, $(wildcard src/*.rl))

.PHONY: all clean test test/pgn

all: $(PROG_NAME)

$(PROG_NAME): $(OBJS) main.o
	$(CC) $^ $(CFLAGS) $(LINK_FLAGS) -o $@
	[ -z "$$DEBUG" ] || ctags -R

%.c: %.rl
	ragel -C $<

clean:
	rm -f $(PROG_NAME) src/*.o
	rm -f $(GENERATED_FILES)
	rm -f tags

test: test/pgn

test/pgn: test/pgn/run-tests.sh test/pgn/test-pgn
	@test/pgn/run-tests.sh

test/pgn/test-pgn: test/pgn/test-pgn.c $(OBJS)
	$(CC) $^ $(CFLAGS) $(LINK_FLAGS) -o $@
