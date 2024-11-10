# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -I./util -fcommon  # -fcommon 옵션 포함

# Target executable
TARGET = os_scheduler

# Source files and object files
UTIL_SRC = util/ipc_msg.c util/proc.c util/queue.c
MAIN_SRC = main.c

UTIL_OBJ = $(UTIL_SRC:.c=.o)
MAIN_OBJ = $(MAIN_SRC:.c=.o)

# All object files
OBJ = $(UTIL_OBJ) $(MAIN_OBJ)

# Default target
all: $(TARGET)

# Link the object files to create the final executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Compile each .c file into a .o file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object files and executable
clean:
	rm -f $(OBJ) $(TARGET)

# Phony targets
.PHONY: all clean
