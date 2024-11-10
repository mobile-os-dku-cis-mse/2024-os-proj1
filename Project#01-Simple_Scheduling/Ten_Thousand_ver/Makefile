CC = gcc

CFLAGS = -O2 -Wall -g

TARGET = scheduler

SRC = main.c log.c process.c queue.c scheduling.c timer.c

OBJ = $(SRC:.c=.o)

HEADERS = common.h log.h process.h queue.h scheduling.h timer.h

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
