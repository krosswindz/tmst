# Copyright (c) 2009 Kross Windz <krosswindz@gmail.com>.
# All rights reserved.
CC=gcc
CFLAGS=-g -Wall -Wextra -O3
MYSQL_CFLAGS=-DBIG_JOINS=1 -DUNIV_LINUX -fno-strict-aliasing -I/usr/include/mysql
MYSQL_LIBS=-Wl,-Bsymbolic-functions -rdynamic -L/usr/lib/mysql -lmysqlclient

OBJS=main.o sql.o
TARGET=tmst

.c.o:
	$(CC) $(CFLAGS) $(MYSQL_CFLAGS) -c $<

all: $(TARGET)

clean:
	rm -f *.o
	rm -f $(TARGET)

$(TARGET): $(MAKEFILE) $(OBJS)
	$(CC) $(OBJS) $(MYSQL_LIBS) -o $(TARGET)
