.POSIX:
.SUFFIXES:

PLATFORM ?= windows

BIN := fantasia
EXT :=
LIBEXT :=

BUILD_DIR := build
BIN_DIR := bin

CC := gcc

CFLAGS := -Wall -Wextra -I include -L lib -g -O1
LDFLAGS := -L bin -lplatform
MAIN_FLAGS := -Wl,-rpath,'$$ORIGIN'
PLATFORM_FLAGS :=


ifeq ($(PLATFORM),linux)
	CFLAGS += -DOS_LINUX
	PLATFORM_FLAGS := -lraylib -lm
	LIBFLAGS := -shared -fPIC
	LIBEXT := .so
endif

ifeq ($(PLATFORM),windows)
	CC := x86_64-w64-mingw32-gcc
	CFLAGS += -DOS_WINDOWS
	PLATFORM_FLAGS := -lraylib -lgdi32 -lwinmm
	LIBFLAGS := -shared
	LIBEXT := .dll
	EXT := .exe
endif

OBJS := $(BUILD_DIR)/main.o $(BUILD_DIR)/platform.o
BINARY := $(BIN_DIR)/$(BIN)$(EXT)

GAME_LIB := libgame$(LIBEXT)
PLATFORM_LIB := libplatform$(LIBEXT)

all: platform game
	$(CC) $(CFLAGS) -o $(BINARY) ./src/main.c ./src/os.c $(LDFLAGS) $(MAIN_FLAGS)

game: platform
	$(CC) $(CFLAGS) $(LIBFLAGS) -o $(BIN_DIR)/$(GAME_LIB) ./src/game.c $(LDFLAGS)

platform:
	$(CC) $(CFLAGS) $(LIBFLAGS) -DPLATFORM_BUILD_SHARED -o $(BIN_DIR)/$(PLATFORM_LIB) ./src/platform.c $(PLATFORM_FLAGS)

clean:
	rm -f $(BINARY) $(BIN_DIR)/$(GAME_LIB) $(BIN_DIR)/$(PLATFORM_LIB)
