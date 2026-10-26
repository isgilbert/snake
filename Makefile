#
# Makefile for the Snake game.
#

CC      = gcc
CFLAGS  = -std=c11 -O2 -Wall -Wextra -I./include
LDLIBS  = -lncurses

# If ncurses.h lives under /usr/include/ncurses/, tell Console.c where it is:
NCURSES_DIR := $(shell if [ -r /usr/include/ncurses/ncurses.h ]; then echo "yes"; fi)
ifeq ($(NCURSES_DIR),yes)
CFLAGS += -DCONS_NCURSES_IS_IN_NCURSES_INCLUDE_DIR
endif

EXE = snake
SRC = Snake.c lib/Console.c
OBJ = $(SRC:.c=.o)

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(OBJ) $(EXE)