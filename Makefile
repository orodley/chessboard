NAME = simple-chess

CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic -DPROG_NAME=\"$(NAME)\"
CFLAGS += $(shell pkg-config --cflags --libs gtk+-3.0)

OBJS = $(patsubst %.c, %.o, $(wildcard src/*.c))

.PHONY: all clean

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(NAME) src/*.o
