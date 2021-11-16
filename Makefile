#
# Makefile for the Snake programming assignment.
#

CXX = g++

NCURSES_DIR := $(shell if [ -r /usr/include/ncurses/ncurses.h ]; then echo "yes"; fi)

ifeq ($(NCURSES_DIR),yes)
CXXFLAGS_EXTRA := -DCONS_NCURSES_IS_IN_NCURSES_INCLUDE_DIR
endif

CXXFLAGS = -g -Wall -I./include $(CXXFLAGS_EXTRA)

SRC = Snake.cpp lib/Console.cpp
OBJ = $(SRC:.cpp=.o)
EXE = $(SRC:.cpp=.exe)

$(EXE) : $(OBJ)
	$(CXX) -o $@ $(OBJ) $(LDFLAGS) -lncurses

# This is just a dummy target to force other targets
# to always be out of date.
nonexistent :

# Remove generated files.
clean :
	rm -f *.o lib/*.o *.exe collected-files.txt submit.properties solution.zip
