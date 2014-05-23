NAME = simple-chess

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic
CFLAGS += $(shell pkg-config --cflags --libs gtk+-2.0)

OBJS = $(patsubst %.c, %.o, $(wildcard src/*.c))

.PHONY: all clean

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(NAME) src/*.o
