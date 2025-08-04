.POSIX:
.SUFFIXES:

PLATFORM ?= windows

BIN := fantasia
EXT :=

BUILD_DIR := build
BIN_DIR := bin

CFLAGS := -Wall -Wextra -I include -L lib -g
LDFLAGS :=
CC := gcc

ifeq ($(PLATFORM),linux)
	CFLAGS += -DOS_LINUX
	LDFLAGS := -lraylib -lm
endif

ifeq ($(PLATFORM),windows)
	CC := x86_64-w64-mingw32-gcc
	CFLAGS += -DOS_WINDOWS
	LDFLAGS := -lraylib -lgdi32 -lwinmm
	EXT := .exe
endif

RAYLIB_VERSION ?= 5.5
TARGET ?= win64_mingw-w64

all: $(BIN_DIR)/$(BIN)$(EXT)
$(BIN_DIR)/$(BIN)$(EXT): $(BUILD_DIR)/main.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
$(BUILD_DIR)/main.o: src/main.c
	$(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)
clean:
	rm -f $(BIN_DIR)/$(BIN)$(EXT) $(BUILD_DIR)/main.o
