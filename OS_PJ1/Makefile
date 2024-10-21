# 

CC = gcc
CFLAGS = -Wall -g -I./include
LDFLAGS = -lrt

SRC = src/main.c src/timer.c src/child_process.c
OBJ = $(SRC:.c=.o)
EXEC = scheduler

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)