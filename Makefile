CC = gcc
CFLAGS = -Wall -Wextra
LIBS = -lSDL2 -lm
TARGET = chip8
SRC = main.c chip8.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(TARGET)
test:
	$(CC) -Itests tests/unity.c tests/test_chip8.c chip8.c -o tests/test_runner && ./tests/test_runner

clean:
	rm -f $(TARGET)
