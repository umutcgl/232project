# Makefile for SMPL Assembler
# CSE 232 Systems Programming - Term Project
# Use: make (to build) or make clean (to remove)

CC = gcc
CFLAGS = -Wall -std=c99
TARGET = assembler
SOURCES = main.c parser.c pass1_codegen.c pass2.c
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGET)

# Link object files
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Compile source files
%.o: %.c asm_common.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET) *.s *.o *.t

# Run tests
test: $(TARGET)
	./$(TARGET) main_prog.asm
	./$(TARGET) add_module.asm
	./$(TARGET) data_module.asm

.PHONY: all clean test
