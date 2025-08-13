.POSIX:
.SUFFIXES:

PLATFORM ?= windows

BIN := fantasia
EXT :=

BUILD_DIR := build
BIN_DIR := bin

CFLAGS := -Wall -Wextra -I include -L lib -g -O1
LDFLAGS :=
LIBFLAGS :=
LIBEXT :=
CC := gcc

ifeq ($(PLATFORM),linux)
	CFLAGS += -DOS_LINUX
	LDFLAGS := -lraylib -lm
	LIBFLAGS := -shared -fPIC
	LIBEXT := .so
endif

ifeq ($(PLATFORM),windows)
	CC := x86_64-w64-mingw32-gcc
	CFLAGS += -DOS_WINDOWS
	LDFLAGS := -lraylib -lgdi32 -lwinmm
	LIBFLAGS := -shared
	LIBEXT := .dll
	EXT := .exe
endif

OBJS := $(BUILD_DIR)/main.o $(BUILD_DIR)/platform.o
BINARY := $(BIN_DIR)/$(BIN)$(EXT)

RAYLIB_VERSION ?= 5.5
TARGET ?= win64_mingw-w64

all: $(BINARY) game

$(BINARY): ./src/main.c ./src/platform.c ./src/os.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Game DLL
game:
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(LIBFLAGS) -DBUILD_SHARED -o $(BIN_DIR)/game$(LIBEXT) ./src/game.c

clean:
	rm -f $(BINARY) $(BIN_DIR)/game$(LIBEXT)
