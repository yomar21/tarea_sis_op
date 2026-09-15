CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -Iinclude
LDFLAGS = -lpthread

SRCS = src/main.c src/parser.c src/dag.c
OBJS = $(SRCS:.c=.o)
TARGET = planificador

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)

.PHONY: all clean