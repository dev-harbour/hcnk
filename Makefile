CC = $(shell which gcc || which clang || which cc)
CFLAGS = -g -Wall -Wextra -O3
SRC = hcnk.c

UNAME_S := $(shell uname -s || echo Windows_NT)

ifeq ($(UNAME_S),Linux)
    LIBS = -lSDL2 -lm
else ifeq ($(UNAME_S),Darwin)
    LIBS = -lSDL2 -lm
else
    LIBS = -lmingw32 -lSDL2main -lSDL2 -lm
endif

BIN_DIR = bin
OUTPUT = $(BIN_DIR)/hcnk

.PHONY: all clean run directories

all: directories $(OUTPUT)

directories:
	@mkdir -p $(BIN_DIR)

$(OUTPUT): $(SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

clean:
	rm -rf $(BIN_DIR)

run: $(OUTPUT)
	./$(OUTPUT)
