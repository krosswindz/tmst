# Copyright (c) 2009 Kross Windz <krosswindz@gmail.com>.
# All rights reserved.
CC=gcc
CFLAGS=-g -Wall -Wextra -O3

OBJS=main.o tcp.o
TARGET=tmst

.c.o:
	$(CC) $(CFLAGS) -c $<

all: $(TARGET)

clean:
	rm -f *.o
	rm -f $(TARGET)

$(TARGET): $(MAKEFILE) $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)
