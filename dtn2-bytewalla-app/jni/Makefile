CC=gcc
CFLAGS=-Wall -g -Wreturn-type
LDFLAGS=-lpthread
SRCS=queue.c file_watch.c file_queue.c
OBJS=queue.o file_watch.o file_queue.o

.c.o:
	$(CC) $(CFLAGS) -c $<

all: file_watch

file_watch: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o file_watch $(LDFLAGS)

clean:
	rm -f $(OBJS) *.bak file_watch