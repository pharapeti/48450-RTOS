# Makefile
# By Patrice Harapeti

CC = gcc
CFLAGS := -Wall -pthread -O2
OBJFILES = queue.c program_2.c
TARGET = program_2

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES)

clean:
	@rm -vf $(TARGET) output.txt
