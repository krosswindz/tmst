# Copyright (c) 2009 Kross Windz <krosswindz@gmail.com>.
# All rights reserved.
CC=gcc
CFLAGS=-g -Wall -Wextra -O3
DEFINES=
LIBMICROHTTPD_LIBS=-lmicrohttpd
MYSQL_CFLAGS=-DBIG_JOINS=1 -DUNIV_LINUX -fno-strict-aliasing -I/usr/include/mysql
MYSQL_LIBS=-Wl,-Bsymbolic-functions -rdynamic -L/usr/lib/mysql -lmysqlclient

OBJS=http.o main.o sql.o tracker.o
TARGET=tmst

# Enable debugging.
ifeq ($(DEBUG), 1)
	DEFINES += -DDEBUG
endif

.c.o:
	$(CC) $(CFLAGS) $(DEFINES) $(MYSQL_CFLAGS) -c $<

all: $(TARGET)

clean:
	rm -f *.o
	rm -f $(TARGET)

$(TARGET): $(MAKEFILE) $(OBJS)
	$(CC) $(OBJS) $(LIBMICROHTTPD_LIBS) $(MYSQL_LIBS) -o $(TARGET)
