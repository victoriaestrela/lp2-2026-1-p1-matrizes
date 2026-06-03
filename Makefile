CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -std=c11 -pthread
LDFLAGS = -pthread -lm
TARGET  = matrix_multiply

SRCS = src/main.c src/matrix_multiply.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf build/

run: $(TARGET)
	./$(TARGET) 1200 4 6
