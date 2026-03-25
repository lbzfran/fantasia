.POSIX:
.SUFFIXES:

PLATFORM ?= windows

BIN := fantasia
EXT :=
LIBEXT :=

BUILD_DIR := build
BIN_DIR := bin

CC := ccache gcc

CFLAGS := -std=c23 -Wall -Wextra -I include -L lib -g3 -O1 -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion -fno-trapping-math -fno-math-errno
LDFLAGS := -L bin -lplatform
MAIN_FLAGS := -Wl,-rpath,'$$ORIGIN'
PLATFORM_FLAGS :=

ifeq ($(DEBUG),1)
	CFLAGS += -fsanitize=undefined -fsanitize-trap -DDEBUG
endif

ifeq ($(PLATFORM),linux)
	CFLAGS += -DOS_LINUX
	PLATFORM_FLAGS := -lraylib -lm
	LIBFLAGS := -shared -fPIC
	LIBEXT := .so
endif

ifeq ($(PLATFORM),windows)
	CC := ccache x86_64-w64-mingw32-gcc
	CFLAGS += -DOS_WINDOWS
	PLATFORM_FLAGS := -lraylib -lgdi32 -lwinmm
	LIBFLAGS := -shared
	LIBEXT := .dll
	EXT := .exe
endif

BINARY := $(BIN_DIR)/$(BIN)$(EXT)

GAME_LIB := libgame$(LIBEXT)
PLATFORM_LIB := libplatform$(LIBEXT)

all: src/main.c $(BIN_DIR)/$(GAME_LIB) $(BIN_DIR)/$(PLATFORM_LIB)
	$(CC) $(CFLAGS) -o $(BINARY) ./src/main.c $(LDFLAGS) $(MAIN_FLAGS)

$(BIN_DIR)/$(GAME_LIB): src/game.c $(BIN_DIR)/$(PLATFORM_LIB)
	$(CC) $(CFLAGS) $(LIBFLAGS) -o $(BIN_DIR)/$(GAME_LIB) $< $(LDFLAGS)

$(BIN_DIR)/$(PLATFORM_LIB): src/platform.c
	$(CC) $(CFLAGS) $(LIBFLAGS) -DPLATFORM_BUILD_SHARED -o $(BIN_DIR)/$(PLATFORM_LIB) src/platform.c src/os.c $(PLATFORM_FLAGS)

game:
	rm $(BIN_DIR)/$(GAME_LIB)
	$(CC) $(CFLAGS) $(LIBFLAGS) -o $(BIN_DIR)/$(GAME_LIB) src/game.c $(LDFLAGS)

clean:
	rm -f $(BINARY) $(BIN_DIR)/$(GAME_LIB) $(BIN_DIR)/$(PLATFORM_LIB)
