CC = gcc
CFLAGS = -pthread -g -Wall
TARGET = ser

all: $(TARGET)

$(TARGET): ser.c
	$(CC) $(CFLAGS) -o $(TARGET) ser.c

clean:
	rm -f $(TARGET)
