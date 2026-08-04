CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -g -Iinclude
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = tests

# Source and Object files
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Executable name
TARGET = $(BIN_DIR)/telemetryd

.PHONY: all clean test docs

all: $(TARGET)

$(TARGET): $(OBJ_FILES) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) docs/html docs/latex

test: $(SRC_DIR)/sysfs_ingest.c $(TEST_DIR)/test_sysfs.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/test_sysfs $^
	$(BIN_DIR)/test_sysfs

docs:
	doxygen Doxyfile
