
BIN := fantasia
CFLAGS := -Wall -Wextra
LFLAGS := -L lib -I include -lraylib -lgdi32 -lwinmm

all:
	gcc -o bin/$(BIN) src/main.c $(LFLAGS) $(CFLAGS)
