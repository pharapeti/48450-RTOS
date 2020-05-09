# Makefile
# By Patrice Harapeti

CC = gcc
CFLAGS := -Wall -pthread -O2
OBJFILES = fifo.c program_2.c
TARGET = program_2

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES)

clean:
	@rm -vf $(OBJFILES) $(TARGET) output.txt
