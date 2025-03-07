CC = gcc
CFLAGS = -Wall -O2 -Iinclude

INCLUDE_DIR = include
SRC_DIR = src
EXAMPLES_DIR = examples
LIB_DIR = lib

LIB_HDR = $(INCLUDE_DIR)/ssd1306.h
LIB_SRC = $(SRC_DIR)/ssd1306.c
LIB_OBJ = $(SRC_DIR)/ssd1306.o
LIB_STATIC = $(LIB_DIR)/libssd1306.a
LIB_SHARED = $(LIB_DIR)/libssd1306.so

EXAMPLES = hello_world scroll_demo cpu_usage

.PHONY: all clean static shared

all: static $(EXAMPLES)

static: $(LIB_STATIC)

shared: $(LIB_SHARED)

$(LIB_OBJ): $(LIB_SRC)
	@mkdir -p $(LIB_DIR)
	$(CC) $(CFLAGS) -c -o $(LIB_OBJ) $(LIB_SRC)

$(LIB_STATIC): $(LIB_OBJ)
	ar rcs $(LIB_STATIC) $(LIB_OBJ)

$(LIB_SHARED): $(LIB_OBJ)
	$(CC) -shared -o $(LIB_SHARED) $(LIB_OBJ)

hello_world: $(EXAMPLES_DIR)/hello_world.c $(LIB_SRC) $(LIB_HDR)
	$(CC) $(CFLAGS) -o hello_world $(EXAMPLES_DIR)/hello_world.c $(LIB_SRC)

scroll_demo: $(EXAMPLES_DIR)/scroll_demo.c $(LIB_SRC) $(LIB_HDR)
	$(CC) $(CFLAGS) -o scroll_demo $(EXAMPLES_DIR)/scroll_demo.c $(LIB_SRC)

cpu_usage: $(EXAMPLES_DIR)/cpu_usage.c $(LIB_SRC) $(LIB_HDR)
	$(CC) $(CFLAGS) -o cpu_usage $(EXAMPLES_DIR)/cpu_usage.c $(LIB_SRC)

clean:
	rm -f $(LIB_OBJ) $(LIB_STATIC) $(LIB_SHARED) hello_world scroll_demo cpu_usage

