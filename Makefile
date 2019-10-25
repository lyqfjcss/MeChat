CC = gcc
CFLAGS = -Wall
LFLAGS = -lpthread -lsqlite3

SRCS = client.c

OBJS = client.o

EXEC = client

RMFILE = local.txt

all:$(OBJS)
		$(CC) $(CFLAGS) $(OBJS) -o $(EXEC) $(LFLAGS)
		
clean:
		rm -rf $(EXEC) $(OBJS) $(RMFILE)