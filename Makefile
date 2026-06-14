CC = gcc
CFLAGS = -Wall -Wextra
LIBS = -lSDL2 -lm
TARGET = chip8
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)
