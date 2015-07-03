CC=gcc
CFLAGS=-Wall -g -Wreturn-type
LDFLAGS=-lpthread
SRCS=queue.c file_watch.c file_queue.c
OBJS=queue.o file_watch.o file_queue.o

.c.o:
	$(CC) $(CFLAGS) -c $<

all: file_watch

file_watch: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o dtn2-bytewalla-app $(LDFLAGS)

clean:
	rm -f $(OBJS) *.bak file_watch dtn2-bytewalla-app